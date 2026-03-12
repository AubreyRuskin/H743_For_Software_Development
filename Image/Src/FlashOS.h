/*
 * FlashOS.h — CMSIS Flash Algorithm interface header.
 * Standard structure/prototypes that pyocd (and Keil) expect.
 */

#ifndef FLASH_OS_H
#define FLASH_OS_H

#include <stdint.h>

/* Device type codes */
#define UNKNOWN    0
#define ONCHIP     1
#define EXT8BIT    2
#define EXT16BIT   3
#define EXT32BIT   4
#define EXTSPI     5

/* Sector end marker */
#define SECTOR_END 0xFFFFFFFF, 0xFFFFFFFF

struct FlashSectors {
    uint32_t szSector;
    uint32_t AddrSector;
};

/* pyocd reads this structure from the "DevDsc" ELF section */
struct FlashDevice {
    uint16_t            Vers;
    char                DevName[128];
    uint16_t            DevType;
    uint32_t            DevAdr;
    uint32_t            szDev;
    uint32_t            szPage;
    uint32_t            Res;
    uint8_t             valEmpty;
    uint32_t            toProg;
    uint32_t            toErase;
    struct FlashSectors sectors[512];
};

/* Function codes passed to Init() */
#define FNC_ERASE  1
#define FNC_PROGRAM 2
#define FNC_VERIFY 3

/*
 * Flash Algorithm API — return 0 = OK, non-zero = failed.
 */
extern int  Init        (uint32_t adr, uint32_t clk, uint32_t fnc);
extern int  UnInit      (uint32_t fnc);
extern int  EraseChip   (void);
extern int  EraseSector (uint32_t adr);
extern int  ProgramPage (uint32_t adr, uint32_t sz, uint8_t *buf);
extern uint32_t Verify  (uint32_t adr, uint32_t sz, uint8_t *buf);

#endif /* FLASH_OS_H */
