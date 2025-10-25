## Overview

This project implements a **simulated temperature sensor** divided into two main components:
- A **Linux kernel module** (`nxp_simtemp.c`) that emulates a temperature device driver.
- A **userspace CLI tool** (`main.py`) that interacts with the kernel driver through **sysfs**, **character device**, and **polling** interfaces.

 ## Block Diagram

The following diagram illustrates the interaction between user-space and kernel-space components.

![Block Diagram](docs/simtemp_architecture.png)

## Interaction Description

### 1. **Device Initialization**
When the module is inserted (`insmod nxp_simtemp.ko`):
- It registers a **platform device** and a **character device** (`/dev/simtemp`).
- Creates a **sysfs directory** under `/sys/class/simtemp_class/simtemp/` with attributes:
  - `threshold` — temperature alert limit in milli-Celsius.
  - `sampling_ms` — sampling interval.
  - `mode` — simulation behavior (`normal`, `noisy`, `ramp`).
  - `stats` — read-only statistics summary.

A **high-resolution timer (hrtimer)** triggers periodic sampling events based on `sampling_ms`.

### 2. **Temperature Sampling**
Each timer tick calls `simtemp_generate()`:
- Generates a pseudo-random temperature value depending on the mode.
- Updates `temperature_mC` (current temperature).
- Compares it with the configured `threshold_mC`.
- If above the threshold:
  - Sets `threshold_event = true`.
  - Increments the `alerts` counter.
  - Wakes up any waiting process via `wake_up_interruptible()`.

### 3. **User-space Communication**
The CLI (in Python) interacts with the driver in three ways:

#### a) **Character Device Interface (`/dev/simtemp`)**
- **Read (`read`)** → gets the latest temperature sample (`simtemp_read()`).
- **Write (`write`)** → can manually set a new temperature in °C or m°C (`simtemp_write()`).
- **Poll (`poll`)** → allows the CLI to wait for new data or threshold events asynchronously.

#### b) **Sysfs Interface**
The CLI configures and monitors the driver by reading and writing sysfs attributes:
- Example:  
  ```bash
  echo 27500 > /sys/class/simtemp_class/simtemp/threshold

#### c) **Event Notification**

The kernel driver uses poll_wait() and wake_up_interruptible() to notify user-space when:

- A new temperature sample is available.

- A threshold alert occurs (POLLPRI flag).

- This allows the Python CLI to react immediately to alerts (e.g., print warnings or adjust settings).

### 4. **Module Lifecycle**

Load: insmod nxp_simtemp.ko

    - Initializes device, class, sysfs attributes, and timer.

Run: python3 main.py --test

    - CLI configures threshold, reads samples, and tests alerts.

Unload: rmmod nxp_simtemp

    - Cancels timer and unregisters device/class.

### 5. Summary of Interactions

| Layer           | Mechanism        | Purpose                      | Function(s)                                           |
| :-------------- | :--------------- | :--------------------------- | :---------------------------------------------------- |
| Kernel          | hrtimer          | Periodic sample generation   | `simtemp_timer_cb()`                                  |
| Kernel          | Character device | Read/write/poll temperature  | `simtemp_read()`, `simtemp_write()`, `simtemp_poll()` |
| Kernel          | Sysfs            | Configure parameters         | `threshold_store()`, `mode_store()`, etc.             |
| User-space      | CLI              | Test, monitor, and configure | `main.py`                                             |
| Synchronization | wait queues      | Notify user-space of updates | `wake_up_interruptible()`                             |

### 6. Events and Signaling Summary

- Temperature update: triggers wake_up_interruptible(&wq).

- Threshold exceeded: triggers POLLPRI for poll/select.

- Sampling timer: restarts continuously with hrtimer_forward_now().

### 7. Design Intent

- This architecture mirrors a realistic embedded sensor driver with:

- Configurable sysfs attributes.

- Poll-based event signaling.

- Separate kernel and user-space logic.

- Extensible design for integrating real hardware in the future.

---

## 8. Architecture

The driver follows a **modular layered architecture**:

- **Platform driver layer**  
  Registers a virtual platform device (`sim_pdev`) and its driver (`sim_driver`).
  Provides the entry points `probe()` and `remove()` to manage lifecycle and resources.

- **Device model layer**  
  Uses the Linux device model to create:
  - `/dev/simtemp` (character device interface)
  - `/sys/class/simtemp_class/simtemp/` (sysfs control attributes)

- **Sampling core**  
  Periodic sampling handled by a **high-resolution timer (`hrtimer`)**, which simulates new temperature data at every tick.  
  The sampling loop updates internal state (`temperature_mC`, `threshold_mC`, `alerts`) and triggers event notifications.

- **User interaction layer**  
  Exposed through:
  - **Character device** for reading, writing, and polling temperature data.
  - **Sysfs attributes** for configuration and monitoring.

## 9. API Contract

### Sysfs API

| Path | Access | Description |
|------|---------|-------------|
| `/sys/class/simtemp_class/simtemp/threshold` | RW | Temperature threshold in milli-Celsius. Alerts trigger when current temperature ≥ threshold. |
| `/sys/class/simtemp_class/simtemp/sampling_ms` | RW | Sampling interval (10–10000 ms). |
| `/sys/class/simtemp_class/simtemp/mode` | RW | Simulation mode: `"normal"`, `"noisy"`, `"ramp"`. |
| `/sys/class/simtemp_class/simtemp/stats` | RO | Diagnostic stats (updates, alerts, last temperature, etc.). |

### Character Device API

| Operation | Function | Description |
|------------|-----------|-------------|
| `read()` | `simtemp_read()` | Returns struct `simtemp_sample` (timestamp + temperature). Blocks until a new sample is available. |
| `write()` | `simtemp_write()` | Allows manual override of temperature value (in °C or m°C). |
| `poll()` | `simtemp_poll()` | Enables event-driven I/O. Returns `POLLIN` when new sample ready, `POLLPRI` on alert. |

All I/O operations are **synchronous** and protected by kernel wait queues (`wait_event_interruptible()`).

## 10. Threading and Locking Model

The driver operates primarily under **single-threaded timer callbacks** and **process context** (for file I/O and sysfs operations).  
Synchronization primitives ensure consistency without complex locking:

- **Wait Queues:**  
  - `wq`: used by blocking `read()` to wait for new samples.  
  - `poll_wq`: used by `poll()` to notify user-space of readable or alert events.  

- **Atomic Variables:**  
  - `updates` and `alerts` counters are updated safely across timer and user contexts using `atomic_long_t`.

- **Implicit Locking:**  
  - The high-resolution timer callback (`simtemp_timer_cb`) runs atomically; it updates the sample and notifies user-space without requiring explicit mutexes.
  - Sysfs store operations are serialized by the kernel, so concurrent updates to configuration (threshold, mode, etc.) are safe by design.

Thus, the driver avoids deadlocks or races by leveraging atomic counters and event queues rather than spinlocks or mutexes.

## 11. Device Tree (DT) Mapping

The driver supports Device Tree (DT) registration via the following compatible string:

```dts
simtemp@0 {
    compatible = "nxp,simtemp";
};
```