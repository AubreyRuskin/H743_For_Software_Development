#!/usr/bin/env bash
set -euo pipefail

TARGET=${1:-stm32h743xx}
GDB_PORT=${2:-3333}
TELNET_PORT=${3:-4444}
FREQUENCY=${4:-4000000}

if ! command -v pyocd >/dev/null 2>&1; then
  echo "Error: pyocd is not installed."
  echo "Install with: pip install pyocd"
  exit 1
fi

echo "Starting pyOCD gdbserver"
echo "  target      : ${TARGET}"
echo "  gdb port    : ${GDB_PORT}"
echo "  telnet port : ${TELNET_PORT}"
echo "  frequency   : ${FREQUENCY} Hz"

exec pyocd gdbserver \
  -t "${TARGET}" \
  --port "${GDB_PORT}" \
  --telnet-port "${TELNET_PORT}" \
  --frequency "${FREQUENCY}"
