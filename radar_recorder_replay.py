#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# @Copyright: Copyright (C) 2025 Gahan Ai Pvt Ltd
# @Author: Aswin Kumar M
# @Date: 2025-11-07 13:02:51
# @Last Modified by: Aswin Kumar M
# @Last Modified time: 2025-11-07 13:56:27
# @Description: 

#!/usr/bin/env python3
import os
import sys
import time
import argparse
import serial
import datetime
import pty

CHUNK = 4096

def record(port: str, out_dir: str):
    """Record raw radar bytes into timestamped .bin file"""
    os.makedirs(out_dir, exist_ok=True)

    timestamp = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    filename = os.path.join(out_dir, f"radar_{timestamp}.bin")

    try:
        ser = serial.Serial(port, baudrate=921600, timeout=1)
        print(f"[REC] Recording from {port} -> {filename}")

        with open(filename, "wb") as f:
            while True:
                data = ser.read(CHUNK)
                if data:
                    f.write(data)
                else:
                    time.sleep(0.001)

    except KeyboardInterrupt:
        print("\n[REC] Stopped")
    except Exception as e:
        print(f"[ERROR] {e}")

def replay(binfile: str, speed: float):
    """Replay raw data into a PTY virtual port"""

    if not os.path.exists(binfile):
        print(f"[ERR] bin file not found: {binfile}")
        sys.exit(1)

    if speed <= 0:
        print("[ERR] --speed must be > 0")
        sys.exit(1)

    # Create PTY pair
    master_fd, slave_fd = pty.openpty()
    slave_name = os.ttyname(slave_fd)

    print("==========================================")
    print("[REPLAY] Virtual Radar Active")
    print(f"[REPLAY] Write side:  {os.ttyname(master_fd)}")
    print(f"[REPLAY] Read side :  {slave_name}")
    print(f"[REPLAY] Playback speed: {speed}x")
    print("Connect your parser to:")
    print(f"    serial_port: {slave_name}")
    print("==========================================")

    # Base timing: ~0.5ms per chunk was realistic for 921600 baud raw feed
    base_delay = 0.0005
    adjusted_delay = base_delay / speed

    master = os.fdopen(master_fd, "wb", buffering=0)

    try:
        with open(binfile, "rb") as f:
            while True:
                chunk = f.read(CHUNK)
                if not chunk:
                    f.seek(0)  # loop forever
                    continue

                master.write(chunk)
                master.flush()
                time.sleep(adjusted_delay)

    except KeyboardInterrupt:
        print("\n[REPLAY] stopped")
    except Exception as e:
        print(f"[ERROR] {e}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Radar Raw Data Recorder & Replayer")
    parser.add_argument("mode", choices=["record", "replay"], help="record or replay radar data")
    parser.add_argument("--port", help="serial port for recording (e.g. /dev/ttyACM1)")
    parser.add_argument("--out", default="data", help="directory to store .bin files")
    parser.add_argument("--binfile", help="bin file for replay mode")
    parser.add_argument("--speed", type=float, default=1.0, help="Playback speed multiplier (default=1.0)")

    args = parser.parse_args()

    if args.mode == "record":
        if not args.port:
            print("ERROR: --port required in record mode")
            sys.exit(1)
        record(args.port, args.out)

    elif args.mode == "replay":
        if not args.binfile:
            print("ERROR: --binfile required in replay mode")
            sys.exit(1)
        replay(args.binfile, args.speed)
