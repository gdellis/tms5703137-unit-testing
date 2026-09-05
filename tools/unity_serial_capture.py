#!/usr/bin/env python3
"""Relay Unity test output from a serial port and exit with the run's verdict.

Reads lines from the port and prints them until Unity's closing sequence

    -----------------------
    N Tests M Failures K Ignored
    OK            (or FAIL)

has been seen. Exit status: 0 for OK, 1 for FAIL, 2 on timeout, 3 if the port
cannot be opened. Needs pyserial (pip install pyserial).

Used by tools/run_on_target.sh; also handy on its own with a board flashed from CCS:

    python3 tools/unity_serial_capture.py --port /dev/ttyUSB0
"""
import argparse
import re
import sys
import time

try:
    import serial
except ImportError:  # pragma: no cover
    sys.stderr.write("unity_serial_capture: pyserial is required (pip install pyserial)\n")
    sys.exit(3)

SUMMARY_RE = re.compile(r"^\d+ Tests \d+ Failures \d+ Ignored")


def capture(port: str, baud: int, timeout: float) -> int:
    try:
        ser = serial.Serial(port, baud, timeout=0.2)
    except (serial.SerialException, OSError) as exc:
        sys.stderr.write(f"unity_serial_capture: cannot open {port}: {exc}\n")
        return 3

    deadline = time.monotonic() + timeout
    pending = b""
    summary_seen = False

    with ser:
        while time.monotonic() < deadline:
            pending += ser.read(256)
            while b"\n" in pending:
                raw, pending = pending.split(b"\n", 1)
                line = raw.decode("utf-8", errors="replace").rstrip("\r")
                print(line, flush=True)
                if SUMMARY_RE.match(line):
                    summary_seen = True
                elif summary_seen and line in ("OK", "FAIL"):
                    return 0 if line == "OK" else 1

    sys.stderr.write(
        f"unity_serial_capture: no Unity summary within {timeout:g}s "
        f"({'summary line seen' if summary_seen else 'nothing conclusive received'})\n"
    )
    return 2


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n", 1)[0])
    parser.add_argument("--port", required=True, help="serial device, e.g. /dev/ttyUSB0 or COM5")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=60.0, help="seconds to wait for the verdict")
    args = parser.parse_args()
    return capture(args.port, args.baud, args.timeout)


if __name__ == "__main__":
    sys.exit(main())
