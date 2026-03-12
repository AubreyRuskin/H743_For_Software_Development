#ifndef W25Q64_H
#define W25Q64_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define W25Q64_JEDEC_ID (0xEF4017UL)
#define W25Q64_TOTAL_SIZE (8UL * 1024UL * 1024UL)
#define W25Q64_SECTOR_SIZE (4UL * 1024UL)
#define W25Q64_PAGE_SIZE (256UL)

typedef enum {
    W25Q64_OK = 0,
    W25Q64_ERR_JEDEC_CMD = -1,
    W25Q64_ERR_JEDEC_RX = -2,
    W25Q64_ERR_WREN_CMD = -3,
    W25Q64_ERR_WREN_SR1 = -4,
    W25Q64_ERR_WREN_LATCH = -5,
    W25Q64_ERR_QE_SR2_READ = -6,
    W25Q64_ERR_QE_WREN = -7,
    W25Q64_ERR_QE_CMD = -8,
    W25Q64_ERR_QE_TX = -9,
    W25Q64_ERR_QE_BUSY = -10,
    W25Q64_ERR_QE_VERIFY = -11,
    W25Q64_ERR_ERASE_WREN = -12,
    W25Q64_ERR_ERASE_CMD = -13,
    W25Q64_ERR_ERASE_BUSY = -14,
    W25Q64_ERR_QPP_WREN = -15,
    W25Q64_ERR_QPP_CMD = -16,
    W25Q64_ERR_QPP_TX = -17,
    W25Q64_ERR_QPP_BUSY = -18,
    W25Q64_ERR_QREAD_CMD = -19,
    W25Q64_ERR_QREAD_RX = -20,
    W25Q64_ERR_COMPARE = -21,
    W25Q64_ERR_JEDEC_MISMATCH = -22
} w25q64_status_t;

int W25Q64_ReadJedecId(uint32_t *jedec_id);
int W25Q64_EnableQuadMode(void);
int W25Q64_Read(uint32_t addr, void *data, uint32_t len);
int W25Q64_Program(uint32_t addr, const void *data, uint32_t len);
int W25Q64_EraseSector4K(uint32_t addr);
int W25Q64_QuadRWTest(uint32_t test_addr, uint32_t test_len);

#ifdef __cplusplus
}
#endif

#endif /* W25Q64_H */
