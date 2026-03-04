#!/bin/bash

# Flash script for STM32H743 using pyOCD

# Usage: ./flash.sh [build_type]
# Default build_type is Debug

BUILD_TYPE=${1:-Debug}
ELF_FILE="build/${BUILD_TYPE}/H743_For_Software_Development.elf"

# Check if pyocd is installed
if ! command -v pyocd &> /dev/null; then
    echo "Error: pyocd is not installed. Please install it first."
    exit 1
fi

# Check if the ELF file exists
if [ ! -f "$ELF_FILE" ]; then
    echo "Error: ELF file '$ELF_FILE' not found. Please build the project first."
    exit 1
fi

echo "Flashing $ELF_FILE to STM32H743 using pyOCD..."

# Run pyOCD to flash the hex file
pyocd flash "$ELF_FILE" -t stm32h743xx

if [ $? -eq 0 ]; then
    echo "Flashing completed successfully!"
else
    echo "Error: Flashing failed!"
    exit 1
fi