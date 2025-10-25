// nxp_simtemp.c 

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/wait.h>
#include <linux/random.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/of.h>
#include <linux/of_device.h>

#define DEVICE_NAME "simtemp"
#define CLASS_NAME  "simtemp_class"

static int sampling_ms = 1000; /* default sampling period */
module_param(sampling_ms, int, 0644);
MODULE_PARM_DESC(sampling_ms, "Sampling period in ms");

enum simtemp_mode {
    MODE_NORMAL = 0,
    MODE_NOISY,
    MODE_RAMP
};

static const char * const mode_names[] = { "normal", "noisy", "ramp" };

struct simtemp_sample {
    s64 ts_ns;
    s32 temp_mC;
} __packed;

struct simtemp_dev {
    struct device *pdev_dev;
    struct class *class;
    struct device *device;
    struct cdev cdev;
    dev_t devno;

    /* data */
    s32 temperature_mC;
    s32 threshold_mC;
    bool above_threshold;
    bool new_sample;
    bool threshold_event;
    enum simtemp_mode mode;

    /* sync */
    wait_queue_head_t wq;
    wait_queue_head_t poll_wq;

    /* timer */
    struct hrtimer timer;

    /* stats */
    atomic_long_t updates;
    atomic_long_t alerts;
};

static struct platform_device *sim_pdev;

/* --- sample generation --- */
static void simtemp_generate(struct simtemp_dev *s)
{
    u32 r = get_random_u32();

    switch (s->mode) {
    case MODE_NORMAL:
        s->temperature_mC += (s32)(r % 101) - 50; /* ±50 mC jitter */
        break;
    case MODE_NOISY:
        s->temperature_mC += (s32)(r % 2001) - 1000; /* ±1°C ruido */
        break;
    case MODE_RAMP:
        s->temperature_mC += 500; /* sube 0.5°C cada muestra */
        if (s->temperature_mC > 100000)
            s->temperature_mC = -40000;
        break;
    }

    if (s->temperature_mC < -40000) s->temperature_mC = -40000;
    if (s->temperature_mC > 150000) s->temperature_mC = 150000;

    atomic_long_inc(&s->updates);
}

/* --- timer callback --- */
static enum hrtimer_restart simtemp_timer_cb(struct hrtimer *t)
{
    struct simtemp_dev *s = container_of(t, struct simtemp_dev, timer);

    simtemp_generate(s);

    /* Update status */
    s->above_threshold = (s->temperature_mC >= s->threshold_mC);
    s->new_sample = true;

    /* Keep active alert while over the threshold */
    if (s->above_threshold) {
        s->threshold_event = true;
        atomic_long_inc(&s->alerts);
        pr_info("%s: ALERT! temp=%d mC >= threshold=%d mC\n",
                DEVICE_NAME, s->temperature_mC, s->threshold_mC);
    } else {
        s->threshold_event = false;
    }

    wake_up_interruptible(&s->wq);
    wake_up_interruptible(&s->poll_wq);

    hrtimer_forward_now(t, ms_to_ktime(sampling_ms));
    return HRTIMER_RESTART;
}


/* --- file ops --- */
static int simtemp_open(struct inode *inode, struct file *file)
{
    struct simtemp_dev *s = container_of(inode->i_cdev, struct simtemp_dev, cdev);
    file->private_data = s;
    return 0;
}

static int simtemp_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t simtemp_read(struct file *file, char __user *buf,
                            size_t count, loff_t *ppos)
{
    struct simtemp_dev *s = file->private_data;
    struct simtemp_sample samp;
    int ret;

    if ((file->f_flags & O_NONBLOCK) && !s->new_sample)
        return -EAGAIN;

    ret = wait_event_interruptible(s->wq, s->new_sample);
    if (ret)
        return ret;

    samp.ts_ns = ktime_get_real_ns();
    samp.temp_mC = s->temperature_mC;
    s->new_sample = false;

    if (count < sizeof(samp))
        return -EMSGSIZE;
    if (copy_to_user(buf, &samp, sizeof(samp)))
        return -EFAULT;

    return sizeof(samp);
}

static ssize_t simtemp_write(struct file *file, const char __user *buf,
                             size_t count, loff_t *ppos)
{
    struct simtemp_dev *s = file->private_data;
    char kbuf[32];
    int val;

    if (count >= sizeof(kbuf))
        return -EINVAL;
    if (copy_from_user(kbuf, buf, count))
        return -EFAULT;
    kbuf[count] = '\0';

    if (kstrtoint(kbuf, 10, &val))
        return -EINVAL;

    if (val > 10000)
        s->temperature_mC = val;
    else
        s->temperature_mC = val * 1000;

    return count;
}

static unsigned int simtemp_poll(struct file *file, poll_table *wait)
{
    struct simtemp_dev *s = file->private_data;
    unsigned int mask = 0;

    poll_wait(file, &s->poll_wq, wait);

    if (s->new_sample)
        mask |= POLLIN | POLLRDNORM;
    if (s->threshold_event)
        mask |= POLLPRI;

    return mask;
}

static const struct file_operations simtemp_fops = {
    .owner   = THIS_MODULE,
    .open    = simtemp_open,
    .release = simtemp_release,
    .read    = simtemp_read,
    .write   = simtemp_write,
    .poll    = simtemp_poll,
};

/* --- sysfs attributes --- */
static ssize_t threshold_show(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
    struct simtemp_dev *s = dev_get_drvdata(dev);
    return sprintf(buf, "%d\n", s->threshold_mC);
}

static ssize_t threshold_store(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
    struct simtemp_dev *s = dev_get_drvdata(dev);
    int val;
    if (kstrtoint(buf, 10, &val))
        return -EINVAL;
    s->threshold_mC = val;
    pr_info("%s: new threshold = %d mC\n", DEVICE_NAME, val);
    return count;
}
static DEVICE_ATTR_RW(threshold);

static ssize_t sampling_ms_show(struct device *dev,
                             struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", sampling_ms);
}

static ssize_t sampling_ms_store(struct device *dev,
                              struct device_attribute *attr,
                              const char *buf, size_t count)
{
    int val;
    if (kstrtoint(buf, 10, &val))
        return -EINVAL;
    sampling_ms = clamp_val(val, 10, 10000);
    pr_info("%s: new sampling period = %d ms\n", DEVICE_NAME, sampling_ms);
    return count;
}
static DEVICE_ATTR_RW(sampling_ms);

static ssize_t mode_show(struct device *dev,
                         struct device_attribute *attr, char *buf)
{
    struct simtemp_dev *s = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", mode_names[s->mode]);
}

static ssize_t mode_store(struct device *dev,
                          struct device_attribute *attr,
                          const char *buf, size_t count)
{
    struct simtemp_dev *s = dev_get_drvdata(dev);

    if (sysfs_streq(buf, "normal"))
        s->mode = MODE_NORMAL;
    else if (sysfs_streq(buf, "noisy"))
        s->mode = MODE_NOISY;
    else if (sysfs_streq(buf, "ramp"))
        s->mode = MODE_RAMP;
    else
        return -EINVAL;

    pr_info("%s: mode set to %s\n", DEVICE_NAME, mode_names[s->mode]);
    return count;
}
static DEVICE_ATTR_RW(mode);

static ssize_t stats_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    struct simtemp_dev *s = dev_get_drvdata(dev);
    return sprintf(buf,
                   "updates: %ld\nalerts: %ld\nlast_temp: %d mC\nthreshold: %d mC\n",
                   atomic_long_read(&s->updates),
                   atomic_long_read(&s->alerts),
                   s->temperature_mC,
                   s->threshold_mC);
}
static DEVICE_ATTR_RO(stats);

static struct attribute *simtemp_attrs[] = {
    &dev_attr_threshold.attr,
    &dev_attr_sampling_ms.attr,
    &dev_attr_mode.attr,
    &dev_attr_stats.attr,
    NULL,
};
ATTRIBUTE_GROUPS(simtemp);

/* --- probe/remove --- */
static int simtemp_probe(struct platform_device *pdev)
{
    struct simtemp_dev *s;
    int ret;

    s = devm_kzalloc(&pdev->dev, sizeof(*s), GFP_KERNEL);
    if (!s)
        return -ENOMEM;
    platform_set_drvdata(pdev, s);
    s->pdev_dev = &pdev->dev;

    s->temperature_mC = 25000;
    s->threshold_mC   = 30000;
    s->mode = MODE_NORMAL;
    s->above_threshold = (s->temperature_mC >= s->threshold_mC);
    s->new_sample = false;
    s->threshold_event = false;
    atomic_long_set(&s->updates, 0);
    atomic_long_set(&s->alerts, 0);
    init_waitqueue_head(&s->wq);
    init_waitqueue_head(&s->poll_wq);

    ret = alloc_chrdev_region(&s->devno, 0, 1, DEVICE_NAME);
    if (ret)
        return ret;
    cdev_init(&s->cdev, &simtemp_fops);
    ret = cdev_add(&s->cdev, s->devno, 1);
    if (ret)
        goto err_chrdev;

    s->class = class_create(CLASS_NAME);
    if (IS_ERR(s->class)) {
        ret = PTR_ERR(s->class);
        goto err_cdev;
    }

    s->device = device_create_with_groups(s->class, &pdev->dev,
                                          s->devno, NULL, simtemp_groups,
                                          DEVICE_NAME);
    if (IS_ERR(s->device)) {
        ret = PTR_ERR(s->device);
        goto err_class;
    }

    dev_set_drvdata(s->device, s);

    hrtimer_init(&s->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    s->timer.function = simtemp_timer_cb;
    hrtimer_start(&s->timer, ms_to_ktime(sampling_ms), HRTIMER_MODE_REL);

    dev_info(&pdev->dev, "simtemp ready at /dev/%s\n", DEVICE_NAME);
    return 0;

err_class:
    class_destroy(s->class);
err_cdev:
    cdev_del(&s->cdev);
err_chrdev:
    unregister_chrdev_region(s->devno, 1);
    return ret;
}

static int simtemp_remove(struct platform_device *pdev)
{
    struct simtemp_dev *s = platform_get_drvdata(pdev);

    hrtimer_cancel(&s->timer);
    device_unregister(s->device);
    class_destroy(s->class);
    cdev_del(&s->cdev);
    unregister_chrdev_region(s->devno, 1);
    return 0;
}

static const struct of_device_id simtemp_of_match[] = {
    { .compatible = "nxp,simtemp" },
    { }
};
MODULE_DEVICE_TABLE(of, simtemp_of_match);

static struct platform_driver sim_driver = {
    .probe  = simtemp_probe,
    .remove = simtemp_remove,
    .driver = {
        .name = "simtemp",
        .of_match_table = simtemp_of_match,
    },
};

static int __init simtemp_module_init(void)
{
    int ret = platform_driver_register(&sim_driver);
    if (ret)
        return ret;

    sim_pdev = platform_device_register_simple("simtemp", -1, NULL, 0);
    if (IS_ERR(sim_pdev)) {
        platform_driver_unregister(&sim_driver);
        return PTR_ERR(sim_pdev);
    }

    pr_info("simtemp: module loaded (sysfs controls enabled)\n");
    return 0;
}

static void __exit simtemp_module_exit(void)
{
    platform_device_unregister(sim_pdev);
    platform_driver_unregister(&sim_driver);
    pr_info("simtemp: module unloaded\n");
}

module_init(simtemp_module_init);
module_exit(simtemp_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jesús García");
MODULE_DESCRIPTION("Simulated temperature sensor");


