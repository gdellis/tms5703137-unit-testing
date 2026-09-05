#!/usr/bin/env bash
#
# CTest launcher for on-target runs (CMAKE_CROSSCOMPILING_EMULATOR in the 'target'
# preset). CTest invokes it as:  run_on_target.sh <path/to/test_xxx.out>
#
# It opens the serial port, flashes the image with a command you supply, then relays
# Unity's output until the final "OK" / "FAIL" line and exits 0 / 1 accordingly.
#
# Environment:
#   TMS570_FLASH_CMD    required. Command that flashes AND starts the image; the token
#                       {image} is replaced by the .out path. Examples in
#                       docs/03-on-target.md (UniFlash dslite, CCS DSS).
#   TMS570_SERIAL_PORT  serial device the board's SCI is on   (default /dev/ttyUSB0)
#   TMS570_BAUD         baud rate, must match HALCoGen         (default 115200)
#   TMS570_TIMEOUT      seconds to wait for the Unity summary  (default 60)

set -euo pipefail

image=${1:?usage: run_on_target.sh <test.out>}
here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

: "${TMS570_SERIAL_PORT:=/dev/ttyUSB0}"
: "${TMS570_BAUD:=115200}"
: "${TMS570_TIMEOUT:=60}"

if [[ -z ${TMS570_FLASH_CMD:-} ]]; then
    echo "run_on_target.sh: set TMS570_FLASH_CMD to a command that flashes and runs {image}" >&2
    echo "  e.g. TMS570_FLASH_CMD='dslite.sh --config=board.ccxml --flash --verbose {image}'" >&2
    exit 2
fi

if [[ ! -f $image ]]; then
    echo "run_on_target.sh: no such image: $image" >&2
    exit 2
fi

# Start listening before the flash tool resets the board so nothing is missed.
python3 "$here/unity_serial_capture.py" \
    --port "$TMS570_SERIAL_PORT" --baud "$TMS570_BAUD" --timeout "$TMS570_TIMEOUT" &
capture=$!
sleep 1

flash_cmd=${TMS570_FLASH_CMD//\{image\}/$image}
echo "run_on_target.sh: $flash_cmd"
if ! eval "$flash_cmd"; then
    kill "$capture" 2>/dev/null || true
    echo "run_on_target.sh: flashing failed" >&2
    exit 3
fi

wait "$capture"
