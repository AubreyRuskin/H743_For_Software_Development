SHELL := /bin/bash

# Paths
IMAGE_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT_DIR := $(IMAGE_DIR)/root
BIN_DIR := $(IMAGE_DIR)/bin
SRC_DIR := $(IMAGE_DIR)/Src
BUILD_DIR := $(IMAGE_DIR)/build
STLDR_BUILD_DIR := $(BUILD_DIR)/stldr

# Project root (one level up from Image/)
PROJ_DIR := $(abspath $(IMAGE_DIR)/..)

# littlefs image
IMAGE_SCRIPT := $(IMAGE_DIR)/build_littlefs_image.sh
IMAGE_BIN := $(BIN_DIR)/littlefs_w25q64.bin

# stldr build outputs
FLM_NAME ?= W25Q64_STM32H743
FLM_ELF  := $(STLDR_BUILD_DIR)/$(FLM_NAME).elf
FLM_OUT  := $(BIN_DIR)/$(FLM_NAME).FLM
FLM_MAP  := $(STLDR_BUILD_DIR)/$(FLM_NAME).map

# Toolchain
CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

# HAL / CMSIS paths
HAL_DRV  := $(PROJ_DIR)/Drivers/STM32H7xx_HAL_Driver
CMSIS_DEV := $(PROJ_DIR)/Drivers/CMSIS/Device/ST/STM32H7xx
CMSIS_INC := $(PROJ_DIR)/Drivers/CMSIS/Include

# MCU flags
CFLAGS := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
CFLAGS += -O2 -ffunction-sections -fdata-sections -Wall -Wextra
CFLAGS += -DSTM32H743xx -DUSE_HAL_DRIVER
CFLAGS += -I$(SRC_DIR)
CFLAGS += -I$(PROJ_DIR)/Core/Inc
CFLAGS += -I$(HAL_DRV)/Inc
CFLAGS += -I$(CMSIS_DEV)/Include
CFLAGS += -I$(CMSIS_INC)
LDFLAGS := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
LDFLAGS += -nostartfiles -Wl,--gc-sections -Wl,-Map,$(FLM_MAP)
# Keep all CMSIS Flash Algorithm entry points
LDFLAGS += -Wl,--undefined=Init -Wl,--undefined=UnInit
LDFLAGS += -Wl,--undefined=EraseChip -Wl,--undefined=EraseSector
LDFLAGS += -Wl,--undefined=ProgramPage -Wl,--undefined=Verify

# Linker script
LD_SCRIPT := $(SRC_DIR)/stldr.ld

# HAL source files required by the external loader
HAL_SRCS := \
	$(HAL_DRV)/Src/stm32h7xx_hal.c \
	$(HAL_DRV)/Src/stm32h7xx_hal_cortex.c \
	$(HAL_DRV)/Src/stm32h7xx_hal_rcc.c \
	$(HAL_DRV)/Src/stm32h7xx_hal_rcc_ex.c \
	$(HAL_DRV)/Src/stm32h7xx_hal_gpio.c \
	$(HAL_DRV)/Src/stm32h7xx_hal_qspi.c \
	$(HAL_DRV)/Src/stm32h7xx_hal_pwr.c \
	$(HAL_DRV)/Src/stm32h7xx_hal_pwr_ex.c

# system_stm32h7xx.c provides SystemInit, SystemCoreClock, etc.
SYS_SRCS := $(PROJ_DIR)/Core/Src/system_stm32h7xx.c

STLDR_SRCS := $(wildcard $(SRC_DIR)/*.c)

ALL_SRCS  := $(STLDR_SRCS) $(HAL_SRCS) $(SYS_SRCS)
STLDR_OBJS := $(patsubst $(SRC_DIR)/%.c,$(STLDR_BUILD_DIR)/%.o,$(STLDR_SRCS))
HAL_OBJS   := $(patsubst $(HAL_DRV)/Src/%.c,$(STLDR_BUILD_DIR)/hal_%.o,$(HAL_SRCS))
SYS_OBJS   := $(STLDR_BUILD_DIR)/system_stm32h7xx.o
ALL_OBJS   := $(STLDR_OBJS) $(HAL_OBJS) $(SYS_OBJS)

# pyOCD options
PYOCD ?= pyocd
TARGET ?= stm32h743xx
FREQ ?= 4000000
ERASE ?= sector
QSPI_ADDR ?= 0x90000000

# pyocd user script
PYOCD_SCRIPT := $(IMAGE_DIR)/pyocd_user.py

# Incremental flash
DIFF_SCRIPT := $(IMAGE_DIR)/diff_flash.py
LAST_IMAGE  := $(BIN_DIR)/littlefs_w25q64.bin.last

.PHONY: help image flm flash flash-diff clean

help:
	@echo "Targets:"
	@echo "  make image      - Build littlefs image from Image/root to Image/bin"
	@echo "  make flm        - Build CMSIS Flash Algorithm (.FLM) for W25Q64"
	@echo "  make flash      - Flash littlefs image to W25Q64 (full, 8MB)"
	@echo "  make flash-diff - Incremental flash (only changed sectors)"
	@echo "  make clean      - Remove build artifacts"
	@echo ""
	@echo "Variables (override like: make flash TARGET=stm32h743xx):"
	@echo "  TARGET=$(TARGET) FREQ=$(FREQ) ERASE=$(ERASE) QSPI_ADDR=$(QSPI_ADDR)"

image: $(IMAGE_BIN)

$(IMAGE_BIN): $(IMAGE_SCRIPT)
	@mkdir -p "$(BIN_DIR)"
	@"$(IMAGE_SCRIPT)"

$(STLDR_BUILD_DIR):
	@mkdir -p "$(STLDR_BUILD_DIR)"

$(STLDR_BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(STLDR_BUILD_DIR)
	@$(CC) $(CFLAGS) -c "$<" -o "$@"

$(STLDR_BUILD_DIR)/hal_%.o: $(HAL_DRV)/Src/%.c | $(STLDR_BUILD_DIR)
	@$(CC) $(CFLAGS) -c "$<" -o "$@"

$(STLDR_BUILD_DIR)/system_stm32h7xx.o: $(PROJ_DIR)/Core/Src/system_stm32h7xx.c | $(STLDR_BUILD_DIR)
	@$(CC) $(CFLAGS) -c "$<" -o "$@"

flm: | $(STLDR_BUILD_DIR)
	@if [[ ! -f "$(LD_SCRIPT)" ]]; then \
		echo "Error: missing linker script: $(LD_SCRIPT)"; \
		exit 1; \
	fi
	@if [[ -z "$(STLDR_SRCS)" ]]; then \
		echo "Error: no source files found in $(SRC_DIR)"; \
		exit 1; \
	fi
	@$(MAKE) --no-print-directory $(ALL_OBJS)
	@$(CC) $(CFLAGS) $(ALL_OBJS) -T "$(LD_SCRIPT)" $(LDFLAGS) -o "$(FLM_ELF)"
	@mkdir -p "$(BIN_DIR)"
	@cp "$(FLM_ELF)" "$(FLM_OUT)"
	@$(SIZE) "$(FLM_ELF)" || true
	@echo "Generated: $(FLM_OUT)"

flash: image flm
	@command -v "$(PYOCD)" >/dev/null 2>&1 || { echo "Error: pyocd not found"; exit 1; }
	@echo "Flashing $(IMAGE_BIN) to W25Q64 via pyOCD (chip erase)"
	@$(PYOCD) load -t "$(TARGET)" -f "$(FREQ)" -e chip \
		--script "$(PYOCD_SCRIPT)" \
		--format bin -a "$(QSPI_ADDR)" "$(IMAGE_BIN)"

flash-diff: image flm
	@command -v "$(PYOCD)" >/dev/null 2>&1 || { echo "Error: pyocd not found"; exit 1; }
	@echo "Incremental flash: comparing with previous image..."
	@python3 "$(DIFF_SCRIPT)" "$(IMAGE_BIN)" \
		--last "$(LAST_IMAGE)" \
		--base "$(QSPI_ADDR)" \
		--target "$(TARGET)" \
		--freq "$(FREQ)" \
		--script "$(PYOCD_SCRIPT)"

clean:
	@rm -rf "$(BUILD_DIR)"
	@rm -f "$(FLM_OUT)"
