#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/root"
OUT_DIR="${SCRIPT_DIR}/bin"
OUT_FILE="${OUT_DIR}/littlefs_w25q64.bin"

# Keep these defaults aligned with Core/Src/lfs_port.c
BLOCK_SIZE=4096
PAGE_SIZE=256
FS_SIZE=$((8 * 1024 * 1024))

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Pack Image/root into a littlefs image for W25Q64.

Options:
  -r <dir>     Source root dir (default: ${ROOT_DIR})
  -o <file>    Output image path (default: ${OUT_FILE})
  -b <bytes>   littlefs block size (default: ${BLOCK_SIZE})
  -p <bytes>   littlefs page/prog size (default: ${PAGE_SIZE})
  -s <bytes>   littlefs image size (default: ${FS_SIZE})
  -h           Show help

Example:
  ./Image/build_littlefs_image.sh
EOF
}

while getopts ":r:o:b:p:s:h" opt; do
  case "${opt}" in
    r) ROOT_DIR="${OPTARG}" ;;
    o) OUT_FILE="${OPTARG}" ;;
    b) BLOCK_SIZE="${OPTARG}" ;;
    p) PAGE_SIZE="${OPTARG}" ;;
    s) FS_SIZE="${OPTARG}" ;;
    h)
      usage
      exit 0
      ;;
    :) echo "Error: -${OPTARG} requires an argument" >&2; usage; exit 2 ;;
    \?) echo "Error: invalid option -${OPTARG}" >&2; usage; exit 2 ;;
  esac
done

if ! command -v mklittlefs >/dev/null 2>&1; then
  echo "Error: mklittlefs not found in PATH." >&2
  exit 1
fi

if [[ ! -d "${ROOT_DIR}" ]]; then
  echo "Error: source dir not found: ${ROOT_DIR}" >&2
  exit 1
fi

mkdir -p "$(dirname "${OUT_FILE}")"

echo "Packing littlefs image..."
echo "  source     : ${ROOT_DIR}"
echo "  output     : ${OUT_FILE}"
echo "  block size : ${BLOCK_SIZE}"
echo "  page size  : ${PAGE_SIZE}"
echo "  fs size    : ${FS_SIZE}"

mklittlefs \
  -c "${ROOT_DIR}" \
  -b "${BLOCK_SIZE}" \
  -p "${PAGE_SIZE}" \
  -s "${FS_SIZE}" \
  "${OUT_FILE}"

echo "Done: ${OUT_FILE}"
