#include "w25q64.h"

#include <string.h>

#include "quadspi.h"

#define W25Q_CMD_READ_JEDEC_ID  0x9FU
#define W25Q_CMD_WRITE_ENABLE   0x06U
#define W25Q_CMD_READ_SR1       0x05U
#define W25Q_CMD_READ_SR2       0x35U
#define W25Q_CMD_WRITE_SR2      0x31U
#define W25Q_CMD_SECTOR_ERASE   0x20U
#define W25Q_CMD_QUAD_PP        0x32U
#define W25Q_CMD_QUAD_READ      0x6BU

#define W25Q_SR1_BUSY           0x01U
#define W25Q_SR1_WEL            0x02U
#define W25Q_SR2_QE             0x02U

static int w25q_read_status(uint8_t instruction, uint8_t *value)
{
    QSPI_CommandTypeDef cmd = {0};

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = instruction;
    cmd.AddressMode = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = QSPI_DATA_1_LINE;
    cmd.NbData = 1;
    cmd.DummyCycles = 0;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY) != HAL_OK) {
        return -1;
    }

    return (HAL_QSPI_Receive(&hqspi, value, HAL_MAX_DELAY) == HAL_OK) ? 0 : -2;
}

static int w25q_write_enable(void)
{
    QSPI_CommandTypeDef cmd = {0};
    uint8_t sr1 = 0;

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = W25Q_CMD_WRITE_ENABLE;
    cmd.AddressMode = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = QSPI_DATA_NONE;
    cmd.DummyCycles = 0;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY) != HAL_OK) {
        return W25Q64_ERR_WREN_CMD;
    }

    if (w25q_read_status(W25Q_CMD_READ_SR1, &sr1) != 0) {
        return W25Q64_ERR_WREN_SR1;
    }

    return ((sr1 & W25Q_SR1_WEL) != 0U) ? W25Q64_OK : W25Q64_ERR_WREN_LATCH;
}

static int w25q_wait_busy_clear(uint32_t timeout_ms)
{
    QSPI_CommandTypeDef cmd = {0};
    QSPI_AutoPollingTypeDef cfg = {0};

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = W25Q_CMD_READ_SR1;
    cmd.AddressMode = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = QSPI_DATA_1_LINE;
    cmd.NbData = 1;
    cmd.DummyCycles = 0;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    cfg.Match = 0;
    cfg.Mask = W25Q_SR1_BUSY;
    cfg.MatchMode = QSPI_MATCH_MODE_AND;
    cfg.StatusBytesSize = 1;
    cfg.Interval = 0x10;
    cfg.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

    return (HAL_QSPI_AutoPolling(&hqspi, &cmd, &cfg, timeout_ms) == HAL_OK) ? W25Q64_OK : -1;
}

static int w25q_enable_quad_mode(void)
{
    QSPI_CommandTypeDef cmd = {0};
    uint8_t sr2 = 0;

    if (w25q_read_status(W25Q_CMD_READ_SR2, &sr2) != 0) {
        return W25Q64_ERR_QE_SR2_READ;
    }

    if ((sr2 & W25Q_SR2_QE) != 0U) {
        return W25Q64_OK;
    }

    if (w25q_write_enable() != W25Q64_OK) {
        return W25Q64_ERR_QE_WREN;
    }

    sr2 |= W25Q_SR2_QE;

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = W25Q_CMD_WRITE_SR2;
    cmd.AddressMode = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = QSPI_DATA_1_LINE;
    cmd.NbData = 1;
    cmd.DummyCycles = 0;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY) != HAL_OK) {
        return W25Q64_ERR_QE_CMD;
    }

    if (HAL_QSPI_Transmit(&hqspi, &sr2, HAL_MAX_DELAY) != HAL_OK) {
        return W25Q64_ERR_QE_TX;
    }

    if (w25q_wait_busy_clear(200U) != W25Q64_OK) {
        return W25Q64_ERR_QE_BUSY;
    }

    if (w25q_read_status(W25Q_CMD_READ_SR2, &sr2) != 0) {
        return W25Q64_ERR_QE_VERIFY;
    }

    return ((sr2 & W25Q_SR2_QE) != 0U) ? W25Q64_OK : W25Q64_ERR_QE_VERIFY;
}

int W25Q64_EnableQuadMode(void)
{
    return w25q_enable_quad_mode();
}

int W25Q64_ReadJedecId(uint32_t *jedec_id)
{
    QSPI_CommandTypeDef cmd = {0};
    uint8_t id[3] = {0};

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = W25Q_CMD_READ_JEDEC_ID;
    cmd.AddressMode = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = QSPI_DATA_1_LINE;
    cmd.NbData = 3;
    cmd.DummyCycles = 0;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY) != HAL_OK) {
        return W25Q64_ERR_JEDEC_CMD;
    }

    if (HAL_QSPI_Receive(&hqspi, id, HAL_MAX_DELAY) != HAL_OK) {
        return W25Q64_ERR_JEDEC_RX;
    }

    *jedec_id = ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | (uint32_t)id[2];
    return W25Q64_OK;
}

int W25Q64_Read(uint32_t addr, void *data, uint32_t len)
{
    QSPI_CommandTypeDef cmd = {0};

    if ((data == NULL) || (len == 0U) || ((addr + len) > W25Q64_TOTAL_SIZE)) {
        return W25Q64_ERR_QREAD_CMD;
    }

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = W25Q_CMD_QUAD_READ;
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = addr;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = QSPI_DATA_4_LINES;
    cmd.NbData = len;
    cmd.DummyCycles = 8;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY) != HAL_OK) {
        return W25Q64_ERR_QREAD_CMD;
    }

    return (HAL_QSPI_Receive(&hqspi, (uint8_t *)data, HAL_MAX_DELAY) == HAL_OK)
        ? W25Q64_OK
        : W25Q64_ERR_QREAD_RX;
}

int W25Q64_EraseSector4K(uint32_t addr)
{
    QSPI_CommandTypeDef cmd = {0};

    if ((addr >= W25Q64_TOTAL_SIZE) || ((addr % W25Q64_SECTOR_SIZE) != 0U)) {
        return W25Q64_ERR_ERASE_CMD;
    }

    if (w25q_write_enable() != W25Q64_OK) {
        return W25Q64_ERR_ERASE_WREN;
    }

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = W25Q_CMD_SECTOR_ERASE;
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = addr;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = QSPI_DATA_NONE;
    cmd.DummyCycles = 0;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY) != HAL_OK) {
        return W25Q64_ERR_ERASE_CMD;
    }

    return (w25q_wait_busy_clear(3000U) == W25Q64_OK)
        ? W25Q64_OK
        : W25Q64_ERR_ERASE_BUSY;
}

int W25Q64_Program(uint32_t addr, const void *data, uint32_t len)
{
    QSPI_CommandTypeDef cmd = {0};
    const uint8_t *src = (const uint8_t *)data;

    if ((data == NULL) || (len == 0U) || ((addr + len) > W25Q64_TOTAL_SIZE)) {
        return W25Q64_ERR_QPP_CMD;
    }

    while (len > 0U) {
        uint32_t page_off = addr % W25Q64_PAGE_SIZE;
        uint32_t chunk = W25Q64_PAGE_SIZE - page_off;
        if (chunk > len) {
            chunk = len;
        }

        if (w25q_write_enable() != W25Q64_OK) {
            return W25Q64_ERR_QPP_WREN;
        }

        cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        cmd.Instruction = W25Q_CMD_QUAD_PP;
        cmd.AddressMode = QSPI_ADDRESS_1_LINE;
        cmd.AddressSize = QSPI_ADDRESS_24_BITS;
        cmd.Address = addr;
        cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
        cmd.DataMode = QSPI_DATA_4_LINES;
        cmd.NbData = chunk;
        cmd.DummyCycles = 0;
        cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
        cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
        cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

        if (HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY) != HAL_OK) {
            return W25Q64_ERR_QPP_CMD;
        }

        if (HAL_QSPI_Transmit(&hqspi, (uint8_t *)src, HAL_MAX_DELAY) != HAL_OK) {
            return W25Q64_ERR_QPP_TX;
        }

        if (w25q_wait_busy_clear(1000U) != W25Q64_OK) {
            return W25Q64_ERR_QPP_BUSY;
        }

        addr += chunk;
        src += chunk;
        len -= chunk;
    }

    return W25Q64_OK;
}

int W25Q64_QuadRWTest(uint32_t test_addr, uint32_t test_len)
{
    QSPI_CommandTypeDef cmd = {0};
    uint8_t tx[32];
    uint8_t rx[32];

    if (test_len > sizeof(tx)) {
        return W25Q64_ERR_COMPARE;
    }

    for (uint32_t i = 0; i < test_len; i++) {
        tx[i] = (uint8_t)(0x5AU + i);
    }
    memset(rx, 0, sizeof(rx));

    int ret = w25q_enable_quad_mode();
    if (ret != W25Q64_OK) {
        return ret;
    }

    if (w25q_write_enable() != W25Q64_OK) {
        return W25Q64_ERR_ERASE_WREN;
    }

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = W25Q_CMD_SECTOR_ERASE;
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = test_addr;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = QSPI_DATA_NONE;
    cmd.DummyCycles = 0;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY) != HAL_OK) {
        return W25Q64_ERR_ERASE_CMD;
    }

    if (w25q_wait_busy_clear(3000U) != W25Q64_OK) {
        return W25Q64_ERR_ERASE_BUSY;
    }

    if (w25q_write_enable() != W25Q64_OK) {
        return W25Q64_ERR_QPP_WREN;
    }

    cmd.Instruction = W25Q_CMD_QUAD_PP;
    cmd.DataMode = QSPI_DATA_4_LINES;
    cmd.NbData = test_len;

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY) != HAL_OK) {
        return W25Q64_ERR_QPP_CMD;
    }

    if (HAL_QSPI_Transmit(&hqspi, tx, HAL_MAX_DELAY) != HAL_OK) {
        return W25Q64_ERR_QPP_TX;
    }

    if (w25q_wait_busy_clear(500U) != W25Q64_OK) {
        return W25Q64_ERR_QPP_BUSY;
    }

    cmd.Instruction = W25Q_CMD_QUAD_READ;
    cmd.DataMode = QSPI_DATA_4_LINES;
    cmd.NbData = test_len;
    cmd.DummyCycles = 8;

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_MAX_DELAY) != HAL_OK) {
        return W25Q64_ERR_QREAD_CMD;
    }

    if (HAL_QSPI_Receive(&hqspi, rx, HAL_MAX_DELAY) != HAL_OK) {
        return W25Q64_ERR_QREAD_RX;
    }

    return (memcmp(tx, rx, test_len) == 0) ? W25Q64_OK : W25Q64_ERR_COMPARE;
}
