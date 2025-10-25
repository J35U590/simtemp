# Simulated Temperature Sensor (`nxp_simtemp`)

**Author:** Jesús García  
**License:** GPL  
**Description:**  
This project implements a **simulated temperature sensor kernel module** (`nxp_simtemp.ko`) with support for sysfs attributes, high-resolution periodic sampling, and event signaling.  
Three example user-space applications are provided to demonstrate **different read mechanisms** (blocking, non-blocking, and polling).  
A Python CLI (`main.py`) is also included to automate configuration and testing.

---

## Build Instructions

Ensure your environment includes:

- Ubuntu 24.04 or later
- Kernel headers installed (`linux-headers-$(uname -r)`)
- GCC and Make
- Python 3.10+  
- Multipass / Virtual Machine (if running in isolated environment)

To build the module:
 ```bash
    cd kernel
    make
  ```
This generates the kernel object:
    - nxp_simtemp.ko

To clean: 
```bash
    make clean
  ```
🧩 Load and Verify the Kernel Module

Insert the module:
```bash
    sudo insmod nxp_simtemp.ko
  ```
Verify device and sysfs entries:
```bash
    ls /dev/simtemp
    ls /sys/class/simtemp_class/simtemp
  ```
Check kernel logs:
```bash
    sudo dmesg | tail
  ```
Remove module:
```bash
   sudo rmmod nxp_simtemp
  ```
---

## Sysfs Controls
The following sysfs attributes can be read or modified at runtime:

| Attribute     | Path                                           | Description                                             |
| ------------- | ---------------------------------------------- | ------------------------------------------------------- |
| `threshold`   | `/sys/class/simtemp_class/simtemp/threshold`   | Threshold temperature in millidegrees Celsius           |
| `sampling_ms` | `/sys/class/simtemp_class/simtemp/sampling_ms` | Sampling period in milliseconds                         |
| `mode`        | `/sys/class/simtemp_class/simtemp/mode`        | Can be `"normal"`, `"noisy"`, or `"ramp"`               |
| `stats`       | `/sys/class/simtemp_class/simtemp/stats`       | Shows live stats: updates, alerts, last temp, threshold |

Example:

```bash
    sudo echo 30000 | sudo tee /sys/class/simtemp_class/simtemp/threshold
    cat /sys/class/simtemp_class/simtemp/stats
  ```
---

## Running the CLI Test

The automation script (run_demo.sh) performs:

1. Kernel module build (make).

2. Insert module (insmod).

3. Configure attributes (threshold, sampling, mode).

4. Run Python CLI test.

5. Unload module (rmmod).

Run:

```bash
    chmod +x run_demo.sh
    ./run_demo.sh
  ```
If successful, you should see periodic temperature updates and alert events printed by the kernel log or CLI.

---

## User-Space Test Programs

1. blocking_read.c

Demonstrates a blocking read() that waits for new samples.

Compile:

```bash
    gcc -o blocking_read blocking_read.c
  ```
Run:
```bash
    sudo ./blocking_read
  ```
Ctr+ c para salir

2. nonblocking_read.c

Demonstrates non-blocking read returning immediately when no data is available.

Compile:
```bash
    gcc -o nonblocking_read nonblocking_read.c
  ```
Run:
```bash
    sudo ./nonblocking_read
  ```
Ctr+ c para salir

3. poll_read.c

Uses poll() to wait for events (including threshold alerts).

Compile:
```bash
    gcc -o poll_read poll_read.c
  ```
Run:
```bash
    sudo ./poll_read
  ```
Ctr+ c para salir

---

## 📎 Links

🎥 Video Demo: Pending

💻 Git Repository: https://github.com/J35U590/simtemp.git

