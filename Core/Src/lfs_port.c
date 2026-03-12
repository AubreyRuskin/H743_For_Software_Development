#include "lfs_port.h"

#include <stdbool.h>

#include "cmsis_os2.h"
#include "w25q64.h"

#define LFS_FLASH_BASE_ADDR      (0UL)
#define LFS_FLASH_BLOCK_SIZE     (W25Q64_SECTOR_SIZE)
#define LFS_FLASH_BLOCK_COUNT    (W25Q64_TOTAL_SIZE / W25Q64_SECTOR_SIZE)
#define LFS_FLASH_PROG_SIZE      (W25Q64_PAGE_SIZE)
#define LFS_FLASH_READ_SIZE      (16UL)

#define LFS_CACHE_SIZE           (256U)
#define LFS_LOOKAHEAD_SIZE       (256U)
#define LFS_BLOCK_CYCLES         (500)

static lfs_t g_lfs;
static osMutexId_t g_lfs_mutex;
static bool g_lfs_mounted;

static uint8_t g_read_buffer[LFS_CACHE_SIZE];
static uint8_t g_prog_buffer[LFS_CACHE_SIZE];
static uint8_t g_lookahead_buffer[LFS_LOOKAHEAD_SIZE];

static uint32_t lfs_flash_address(lfs_block_t block, lfs_off_t off)
{
    return LFS_FLASH_BASE_ADDR + (block * LFS_FLASH_BLOCK_SIZE) + off;
}

static int lfs_port_lock(const struct lfs_config *c)
{
    (void)c;

    if (g_lfs_mutex == NULL) {
        return LFS_ERR_IO;
    }

    return (osMutexAcquire(g_lfs_mutex, osWaitForever) == osOK) ? 0 : LFS_ERR_IO;
}

static int lfs_port_unlock(const struct lfs_config *c)
{
    (void)c;

    if (g_lfs_mutex == NULL) {
        return LFS_ERR_IO;
    }

    return (osMutexRelease(g_lfs_mutex) == osOK) ? 0 : LFS_ERR_IO;
}

static int lfs_port_read(const struct lfs_config *c, lfs_block_t block,
                         lfs_off_t off, void *buffer, lfs_size_t size)
{
    (void)c;

    if (block >= LFS_FLASH_BLOCK_COUNT || (off + size) > LFS_FLASH_BLOCK_SIZE) {
        return LFS_ERR_INVAL;
    }

    return (W25Q64_Read(lfs_flash_address(block, off), buffer, size) == W25Q64_OK)
        ? 0
        : LFS_ERR_IO;
}

static int lfs_port_prog(const struct lfs_config *c, lfs_block_t block,
                         lfs_off_t off, const void *buffer, lfs_size_t size)
{
    (void)c;

    if (block >= LFS_FLASH_BLOCK_COUNT || (off + size) > LFS_FLASH_BLOCK_SIZE) {
        return LFS_ERR_INVAL;
    }

    if ((off % LFS_FLASH_PROG_SIZE) != 0U || (size % LFS_FLASH_PROG_SIZE) != 0U) {
        return LFS_ERR_INVAL;
    }

    return (W25Q64_Program(lfs_flash_address(block, off), buffer, size) == W25Q64_OK)
        ? 0
        : LFS_ERR_IO;
}

static int lfs_port_erase(const struct lfs_config *c, lfs_block_t block)
{
    (void)c;

    if (block >= LFS_FLASH_BLOCK_COUNT) {
        return LFS_ERR_INVAL;
    }

    return (W25Q64_EraseSector4K(lfs_flash_address(block, 0U)) == W25Q64_OK)
        ? 0
        : LFS_ERR_IO;
}

static int lfs_port_sync(const struct lfs_config *c)
{
    (void)c;
    return 0;
}

static struct lfs_config g_lfs_cfg = {
    .context = NULL,
    .read = lfs_port_read,
    .prog = lfs_port_prog,
    .erase = lfs_port_erase,
    .sync = lfs_port_sync,
#ifdef LFS_THREADSAFE
    .lock = lfs_port_lock,
    .unlock = lfs_port_unlock,
#endif
    .read_size = LFS_FLASH_READ_SIZE,
    .prog_size = LFS_FLASH_PROG_SIZE,
    .block_size = LFS_FLASH_BLOCK_SIZE,
    .block_count = LFS_FLASH_BLOCK_COUNT,
    .block_cycles = LFS_BLOCK_CYCLES,
    .cache_size = LFS_CACHE_SIZE,
    .lookahead_size = LFS_LOOKAHEAD_SIZE,
    .compact_thresh = 0,
    .read_buffer = g_read_buffer,
    .prog_buffer = g_prog_buffer,
    .lookahead_buffer = g_lookahead_buffer,
    .name_max = 0,
    .file_max = 0,
    .attr_max = 0,
    .metadata_max = 0,
    .inline_max = 0,
};

int lfs_port_init(void)
{
    if (g_lfs_mounted) {
        return 0;
    }

    if (g_lfs_mutex == NULL) {
        const osMutexAttr_t attr = {
            .name = "lfs_mutex",
            .attr_bits = osMutexRecursive | osMutexPrioInherit,
            .cb_mem = NULL,
            .cb_size = 0U,
        };

        g_lfs_mutex = osMutexNew(&attr);
        if (g_lfs_mutex == NULL) {
            return LFS_ERR_NOMEM;
        }
    }

    uint32_t jedec_id = 0U;
    if (W25Q64_ReadJedecId(&jedec_id) != W25Q64_OK || jedec_id != W25Q64_JEDEC_ID) {
        return LFS_ERR_IO;
    }

    if (W25Q64_EnableQuadMode() != W25Q64_OK) {
        return LFS_ERR_IO;
    }

    int err = lfs_mount(&g_lfs, &g_lfs_cfg);
    if (err == LFS_ERR_CORRUPT || err == LFS_ERR_NOENT) {
        err = lfs_format(&g_lfs, &g_lfs_cfg);
        if (err != 0) {
            return err;
        }

        err = lfs_mount(&g_lfs, &g_lfs_cfg);
    }

    if (err == 0) {
        g_lfs_mounted = true;
    }

    return err;
}

int lfs_port_deinit(void)
{
    if (!g_lfs_mounted) {
        return 0;
    }

    int err = lfs_unmount(&g_lfs);
    if (err == 0) {
        g_lfs_mounted = false;
    }

    return err;
}

lfs_t *lfs_port_fs(void)
{
    return g_lfs_mounted ? &g_lfs : NULL;
}
