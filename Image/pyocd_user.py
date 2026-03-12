"""
pyocd user script: register W25Q64 external QSPI flash at 0x90000000.

pyocd will invoke will_init_target() before target init, allowing us
to add an external FlashRegion with our custom .FLM algo to the memory map.
"""

import os
from pyocd.core.memory_map import FlashRegion, RamRegion

FLM_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        "bin", "W25Q64_STM32H743.FLM")

def will_init_target(target, init_sequence):
    # Add AXI SRAM (D1, 512 KB) — not in pyocd built-in H743 target.
    # Flash algo + page buffers + stack will be placed here.
    target.memory_map.add_region(
        RamRegion(
            name="axi_sram",
            start=0x24000000,
            length=0x80000,          # 512 KB
            is_default=False,
        )
    )

    # Add external W25Q64 flash region with our FLM algo.
    target.memory_map.add_region(
        FlashRegion(
            name="w25q64",
            start=0x90000000,
            length=0x00800000,       # 8 MB
            sector_size=0x1000,      # 4 KB
            page_size=0x10000,       # 64 KB
            is_boot_memory=False,
            erased_byte_value=0xFF,
            flm=FLM_PATH,
        )
    )
