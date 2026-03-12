#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

ROOT_DIR="${PROJECT_DIR}/root"
OUT_IMG="${PROJECT_DIR}/build/littlefs.bin"

# Default values aligned with Core/Src/lfs_port.c
BASE_ADDR="0x08100000"
BLOCK_SIZE=131072
BLOCK_COUNT=8
PROG_SIZE=32
READ_SIZE=32

TARGET="${TARGET:-stm32h743xx}"
FREQUENCY="${FREQUENCY:-4000000}"
ERASE="${ERASE:-sector}"
BUILD_ONLY=0

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Build littlefs image from root/ and flash to MCU with pyOCD.

Options:
  -r <dir>     Source directory (default: ${ROOT_DIR})
  -o <file>    Output image file (default: ${OUT_IMG})
  -a <addr>    Flash base address (default: ${BASE_ADDR})
  -b <bytes>   littlefs block size (default: ${BLOCK_SIZE})
  -c <count>   littlefs block count (default: ${BLOCK_COUNT})
  -p <bytes>   littlefs prog/page size (default: ${PROG_SIZE})
  -t <target>  pyOCD target (default: ${TARGET})
  -f <hz>      SWD frequency in Hz (default: ${FREQUENCY})
  -e <mode>    Erase mode: auto|chip|sector (default: ${ERASE})
  -n           Build image only, do not flash
  -h           Show this help

Examples:
  ./scripts/flash_littlefs.sh
  ./scripts/flash_littlefs.sh -n
  ./scripts/flash_littlefs.sh -a 0x08100000 -t stm32h743xx
EOF
}

while getopts ":r:o:a:b:c:p:t:f:e:nh" opt; do
  case "${opt}" in
    r) ROOT_DIR="${OPTARG}" ;;
    o) OUT_IMG="${OPTARG}" ;;
    a) BASE_ADDR="${OPTARG}" ;;
    b) BLOCK_SIZE="${OPTARG}" ;;
    c) BLOCK_COUNT="${OPTARG}" ;;
    p) PROG_SIZE="${OPTARG}" ;;
    t) TARGET="${OPTARG}" ;;
    f) FREQUENCY="${OPTARG}" ;;
    e) ERASE="${OPTARG}" ;;
    n) BUILD_ONLY=1 ;;
    h)
      usage
      exit 0
      ;;
    :) echo "Error: -${OPTARG} requires an argument" >&2; usage; exit 2 ;;
    \?) echo "Error: invalid option -${OPTARG}" >&2; usage; exit 2 ;;
  esac
done

FS_SIZE=$((BLOCK_SIZE * BLOCK_COUNT))

if [[ ! -d "${ROOT_DIR}" ]]; then
  echo "Error: source dir not found: ${ROOT_DIR}" >&2
  exit 1
fi

if ! command -v mklittlefs >/dev/null 2>&1; then
  echo "Error: mklittlefs not found in PATH." >&2
  exit 1
fi

if [[ ${BUILD_ONLY} -eq 0 ]] && ! command -v pyocd >/dev/null 2>&1; then
  echo "Error: pyocd not found in PATH." >&2
  exit 1
fi

mkdir -p "$(dirname "${OUT_IMG}")"

echo "[1/2] Build littlefs image"
echo "  source      : ${ROOT_DIR}"
echo "  output      : ${OUT_IMG}"
echo "  fs size     : ${FS_SIZE}"
echo "  block size  : ${BLOCK_SIZE}"
echo "  block count : ${BLOCK_COUNT}"
echo "  prog/read   : ${PROG_SIZE}/${READ_SIZE}"

MKLFS_LOG="$(mktemp)"
set +e
mklittlefs \
  -c "${ROOT_DIR}" \
  -b "${BLOCK_SIZE}" \
  -p "${PROG_SIZE}" \
  -s "${FS_SIZE}" \
  "${OUT_IMG}" 2>&1 | tee "${MKLFS_LOG}"
MKLFS_RC=${PIPESTATUS[0]}
set -e

if [[ ${MKLFS_RC} -ne 0 ]] || grep -qiE "error|no more free space|filesystem is full" "${MKLFS_LOG}"; then
  rm -f "${MKLFS_LOG}"
  echo "Error: mklittlefs failed or image is incomplete due to insufficient space." >&2
  echo "Hint: reduce root content or increase littlefs partition size/parameters." >&2
  exit 1
fi

rm -f "${MKLFS_LOG}"

echo "Image generated: ${OUT_IMG}"

if [[ ${BUILD_ONLY} -eq 1 ]]; then
  exit 0
fi

echo "[2/2] Flash image with pyOCD"
echo "  target      : ${TARGET}"
echo "  address     : ${BASE_ADDR}"
echo "  frequency   : ${FREQUENCY}"
echo "  erase mode  : ${ERASE}"

pyocd load \
  -t "${TARGET}" \
  -f "${FREQUENCY}" \
  -e "${ERASE}" \
  --format bin \
  -a "${BASE_ADDR}" \
  "${OUT_IMG}"

echo "Done. littlefs image flashed at ${BASE_ADDR}."
