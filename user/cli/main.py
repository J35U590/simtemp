#!/usr/bin/env python3
# simtemp_cli.py
# Usage: sudo ./simtemp_cli.py [--threshold N] [--sampling N] [--test] [--device /dev/simtemp] [--sysfs /sys/class/simtemp_class/simtemp]
#
# - threshold in milli-Celsius (e.g. 45000)
# - sampling in ms (e.g. 1000)
# - --test : set low threshold and expect an alert within 2 periods

import os
import sys
import argparse
import struct
import select
import time
from datetime import datetime, timezone

DEFAULT_SYSFS = "/sys/class/simtemp_class/simtemp"
DEFAULT_DEV = "/dev/simtemp"

SAMPLE_STRUCT_FMT = "<q i"   # little-endian: int64 (ts_ns), int32 (temp_mC)
SAMPLE_SIZE = struct.calcsize(SAMPLE_STRUCT_FMT)  # should be 12

def iso_from_ns(ns):
    sec = ns // 1_000_000_000
    nsec = ns % 1_000_000_000
    dt = datetime.fromtimestamp(sec, tz=timezone.utc)
    # milliseconds
    ms = nsec // 1_000_000
    return dt.strftime("%Y-%m-%dT%H:%M:%S.") + f"{ms:03d}Z"

def write_sysfs(path, value):
    try:
        with open(path, "w") as f:
            f.write(str(value))
        return True
    except Exception as e:
        print(f"ERROR writing {value} to {path}: {e}", file=sys.stderr)
        return False

def read_sysfs_int(path, default=None):
    try:
        with open(path, "r") as f:
            s = f.read().strip()
            return int(s)
    except Exception:
        return default

def open_device(path):
    # open in non-blocking mode for poll and reads
    flags = os.O_RDONLY | os.O_NONBLOCK
    try:
        fd = os.open(path, flags)
        return fd
    except Exception as e:
        print(f"ERROR opening {path}: {e}", file=sys.stderr)
        return None

def perform_poll_loop(dev_fd, polling_timeout_ms, test_mode=False, max_periods=2):
    poller = select.poll()
    POLLIN = select.POLLIN
    POLLPRI = getattr(select, "POLLPRI", 0x002)
    poller.register(dev_fd, POLLIN | POLLPRI)

    iterations = 0
    alert_seen = False

    while True:
        events = poller.poll(polling_timeout_ms)
        if not events:
            # timeout
            print("[poll] timeout")
            if test_mode and iterations >= max_periods:
                break
            continue

        for fd, event in events:
            # read sample
            try:
                data = os.read(dev_fd, SAMPLE_SIZE)
            except BlockingIOError:
                continue
            except Exception as e:
                print("read error:", e, file=sys.stderr)
                return False, alert_seen

            if len(data) != SAMPLE_SIZE:
                print(f"read: unexpected size {len(data)} (expected {SAMPLE_SIZE})", file=sys.stderr)
                # continue; maybe partial / try again
                continue

            ts_ns, temp_mC = struct.unpack(SAMPLE_STRUCT_FMT, data)
            ts = iso_from_ns(ts_ns)
            tempC = temp_mC / 1000.0
            is_alert = bool(event & POLLPRI)
            print(f"{ts} temp={tempC:.2f}C alert={1 if is_alert else 0}")

            if is_alert:
                alert_seen = True

        if test_mode:
            iterations += 1
            if iterations >= max_periods:
                break

    return True, alert_seen

def main():
    parser = argparse.ArgumentParser(description="simtemp CLI (Python)")
    parser.add_argument("--threshold", type=int, help="threshold in milli-Celsius (e.g. 45000)")
    parser.add_argument("--sampling", type=int, help="sampling period in ms (e.g. 1000)")
    parser.add_argument("--test", action="store_true", help="test mode: set low threshold and expect alert within 2 periods")
    parser.add_argument("--device", default=DEFAULT_DEV, help="device path (default /dev/simtemp)")
    parser.add_argument("--sysfs", default=DEFAULT_SYSFS, help=f"sysfs path (default {DEFAULT_SYSFS})")
    args = parser.parse_args()

    sysfs_path = args.sysfs
    dev_path = args.device

    # Map attribute names based on your sysfs layout
    sampling_attr = os.path.join(sysfs_path, "sampling_ms")
    threshold_attr = os.path.join(sysfs_path, "threshold")

    # If user passed values, write them
    if args.sampling is not None:
        if not write_sysfs(sampling_attr, args.sampling):
            return 1
        print(f"Wrote sampling_ms={args.sampling} to {sampling_attr}")

    if args.threshold is not None:
        if not write_sysfs(threshold_attr, args.threshold):
            return 1
        print(f"Wrote threshold={args.threshold} to {threshold_attr}")

    # If test mode, set a low threshold (XX.X°C) to force alert quickly
    if args.test:
        low = 24000
        if not write_sysfs(threshold_attr, low):
            return 1
        print(f"[TEST] wrote threshold={low/1000:.1f} °C to {threshold_attr}")

    # Read sampling period to determine poll timeout
    sampling = read_sysfs_int(sampling_attr, default=1000)
    if sampling <= 0:
        sampling = 1000
    # polling timeout: 1.1 * 2 * sampling (ms) for the test duration window
    timeout_ms = int((sampling * 1.2))  # poll timeout per call; iterations controlled in loop
    # For test mode we'll run for 2 periods (handled inside loop)
    dev_fd = open_device(dev_path)
    if dev_fd is None:
        return 1

    ok, alert_seen = perform_poll_loop(dev_fd, timeout_ms, test_mode=args.test, max_periods=2)
    os.close(dev_fd)

    if not ok:
        return 1

    if args.test:
        if alert_seen:
            print("[TEST] OK: alert detected")
            return 0
        else:
            print("[TEST] : no alert detected within 2 periods", file=sys.stderr)
            return 2

    return 0

if __name__ == "__main__":
    sys.exit(main())
