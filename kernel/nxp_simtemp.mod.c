#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const char ____versions[]
__used __section("__versions") =
	"\x1c\x00\x00\x00\x2b\x2f\xec\xe3"
	"alloc_chrdev_region\0"
	"\x14\x00\x00\x00\x43\xde\xd6\x22"
	"cdev_init\0\0\0"
	"\x14\x00\x00\x00\xa9\x57\xc9\x0e"
	"cdev_add\0\0\0\0"
	"\x18\x00\x00\x00\x6a\xb8\xa9\x6c"
	"class_create\0\0\0\0"
	"\x24\x00\x00\x00\x77\xa5\x4f\xea"
	"device_create_with_groups\0\0\0"
	"\x18\x00\x00\x00\x49\xd3\x82\xea"
	"hrtimer_init\0\0\0\0"
	"\x20\x00\x00\x00\x97\xc1\xb7\xc0"
	"hrtimer_start_range_ns\0\0"
	"\x14\x00\x00\x00\x32\xf3\x31\x94"
	"_dev_info\0\0\0"
	"\x24\x00\x00\x00\x9e\xba\x2a\x19"
	"platform_device_unregister\0\0"
	"\x24\x00\x00\x00\x76\x82\x3d\xf5"
	"platform_driver_unregister\0\0"
	"\x24\x00\x00\x00\x97\x70\x48\x65"
	"__x86_indirect_thunk_rax\0\0\0\0"
	"\x2c\x00\x00\x00\xc6\xfa\xb1\x54"
	"__ubsan_handle_load_invalid_value\0\0\0"
	"\x20\x00\x00\x00\x5d\x7b\xc1\xe2"
	"__SCT__might_resched\0\0\0\0"
	"\x18\x00\x00\x00\x75\x79\x48\xfe"
	"init_wait_entry\0"
	"\x14\x00\x00\x00\x51\x0e\x00\x01"
	"schedule\0\0\0\0"
	"\x20\x00\x00\x00\x95\xd4\x26\x8c"
	"prepare_to_wait_event\0\0\0"
	"\x14\x00\x00\x00\xbf\x0f\x54\x92"
	"finish_wait\0"
	"\x20\x00\x00\x00\x12\xda\xf0\xc4"
	"ktime_get_with_offset\0\0\0"
	"\x18\x00\x00\x00\xe1\xbe\x10\x6b"
	"_copy_to_user\0\0\0"
	"\x24\x00\x00\x00\x26\xd4\xce\x69"
	"__platform_driver_register\0\0"
	"\x28\x00\x00\x00\x26\x8b\x56\xf7"
	"platform_device_register_full\0\0\0"
	"\x18\x00\x00\x00\x0c\xc1\x6d\xd3"
	"get_random_u32\0\0"
	"\x14\x00\x00\x00\x44\x43\x96\xe2"
	"__wake_up\0\0\0"
	"\x18\x00\x00\x00\xec\xb7\x5b\x13"
	"hrtimer_forward\0"
	"\x18\x00\x00\x00\xbb\x90\x9b\x47"
	"param_ops_int\0\0\0"
	"\x14\x00\x00\x00\xbb\x6d\xfb\xbd"
	"__fentry__\0\0"
	"\x1c\x00\x00\x00\xca\x39\x82\x5b"
	"__x86_return_thunk\0\0"
	"\x18\x00\x00\x00\xde\xe6\x2f\x10"
	"hrtimer_cancel\0\0"
	"\x1c\x00\x00\x00\xc4\x23\xe6\xdd"
	"device_unregister\0\0\0"
	"\x18\x00\x00\x00\x47\x67\x64\x75"
	"class_destroy\0\0\0"
	"\x14\x00\x00\x00\xc9\xcf\xb9\xb1"
	"cdev_del\0\0\0\0"
	"\x24\x00\x00\x00\x33\xb3\x91\x60"
	"unregister_chrdev_region\0\0\0\0"
	"\x10\x00\x00\x00\xfd\xf9\x3f\x3c"
	"sprintf\0"
	"\x28\x00\x00\x00\xb3\x1c\xa2\x87"
	"__ubsan_handle_out_of_bounds\0\0\0\0"
	"\x14\x00\x00\x00\xe2\x7c\x2e\x22"
	"sysfs_streq\0"
	"\x10\x00\x00\x00\x7e\x3a\x2c\x12"
	"_printk\0"
	"\x14\x00\x00\x00\xcb\x69\x85\x8c"
	"kstrtoint\0\0\0"
	"\x1c\x00\x00\x00\xcb\xf6\xfd\xf0"
	"__stack_chk_fail\0\0\0\0"
	"\x1c\x00\x00\x00\x48\x9f\xdb\x88"
	"__check_object_size\0"
	"\x18\x00\x00\x00\xc2\x9c\xc4\x13"
	"_copy_from_user\0"
	"\x18\x00\x00\x00\x21\xb7\x93\xa1"
	"devm_kmalloc\0\0\0\0"
	"\x20\x00\x00\x00\x54\xea\xa5\xd9"
	"__init_waitqueue_head\0\0\0"
	"\x18\x00\x00\x00\xeb\x7b\x33\xe1"
	"module_layout\0\0\0"
	"\x00\x00\x00\x00\x00\x00\x00\x00";

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cnxp,simtemp");
MODULE_ALIAS("of:N*T*Cnxp,simtempC*");

MODULE_INFO(srcversion, "740355A01EE59E75E06B1E3");
