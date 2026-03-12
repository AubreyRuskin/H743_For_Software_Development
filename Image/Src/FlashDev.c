/*
 * FlashDev.c — CMSIS Flash Algorithm device descriptor for W25Q64 on QSPI.
 * pyocd reads the FlashDevice structure from the "DevDscr" section of the ELF.
 */

#include "FlashOS.h"

struct FlashDevice const FlashDevice __attribute__((used, section("DevDscr"))) = {
    0x0101,                         /* Vers:   CMSIS Flash Algorithm spec 1.01 */
    "W25Q64_STM32H743_QSPI",       /* DevName */
    EXTSPI,                         /* DevType: external SPI flash */
    0x90000000,                     /* DevAdr:  QSPI memory-mapped base */
    0x00800000,                     /* szDev:   8 MB */
    256,                            /* szPage:  256 B (W25Q64 page program) */
    0,                              /* Res:     reserved */
    0xFF,                           /* valEmpty */
    1000,                           /* toProg:  page program timeout (ms) */
    6000,                           /* toErase: sector erase timeout (ms) */
    { { 0x1000, 0x000000 },        /* Sector 0: 4 KB (uniform across 8 MB) */
      { SECTOR_END } }
};
