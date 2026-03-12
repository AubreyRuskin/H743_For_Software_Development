/*
 * FlashPrg.c — CMSIS Flash Algorithm implementation for W25Q64 on QSPI.
 *
 * pyocd loads this ELF into MCU RAM, then calls:
 *   Init()        → configure clocks, QSPI, enable quad mode
 *   EraseSector()  → erase 4 KB at given address (repeated for all sectors)
 *   ProgramPage() → write up to 256 B from RAM buffer to flash (repeated)
 *   UnInit()      → cleanup
 *
 * All addresses are memory-mapped (0x9000_xxxx).
 * Return 0 = OK, non-zero = error.
 *
 * QSPI pin-out (matches quadspi.c / CubeMX):
 *   PB2  = CLK      PB10 = NCS
 *   PD11 = IO0      PD12 = IO1
 *   PE2  = IO2      PF6  = IO3
 *
 * After reset MCU runs on HSI 64 MHz.
 * QSPI clock = D1HCLK / (Prescaler+1) = 64 / 3 ≈ 21 MHz — safe for W25Q64.
 */

#include "FlashOS.h"
#include "stm32h7xx_hal.h"
#include <string.h>

/* ---------- QSPI mapped base ---------- */
#define QSPI_BASE_ADDR         0x90000000U

/* ---------- W25Q64 commands ---------- */
#define CMD_WRITE_ENABLE        0x06U
#define CMD_READ_SR1            0x05U
#define CMD_READ_SR2            0x35U
#define CMD_WRITE_SR2           0x31U
#define CMD_SECTOR_ERASE_4K     0x20U
#define CMD_CHIP_ERASE          0xC7U
#define CMD_QUAD_PAGE_PROGRAM   0x32U
#define CMD_QUAD_FAST_READ      0x6BU

#define SR1_BUSY                0x01U
#define SR2_QE                  0x02U

/* ---------- BSS-resident handle ---------- */
static QSPI_HandleTypeDef hqspi;

/* ================================================================
 * Override weak HAL tick — use DWT CYCCNT (no SysTick IRQ needed).
 * ================================================================ */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    (void)TickPriority;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
    return HAL_OK;
}

uint32_t HAL_GetTick(void)
{
    return DWT->CYCCNT / (SystemCoreClock / 1000U);
}

/* ================================================================
 * Low-level QSPI / GPIO init  (mirrors quadspi.c from CubeMX)
 * ================================================================ */
static void QSPI_GpioInit(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF9_QUADSPI;

    /* PB2 = CLK, PB10 = NCS */
    gpio.Pin = GPIO_PIN_2 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* PD11 = IO0, PD12 = IO1 */
    gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    HAL_GPIO_Init(GPIOD, &gpio);

    /* PE2 = IO2 */
    gpio.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOE, &gpio);

    /* PF6 = IO3 */
    gpio.Pin = GPIO_PIN_6;
    HAL_GPIO_Init(GPIOF, &gpio);
}

static int QSPI_Configure(void)
{
    RCC_PeriphCLKInitTypeDef clk = {0};

    clk.PeriphClockSelection = RCC_PERIPHCLK_QSPI;
    clk.QspiClockSelection   = RCC_QSPICLKSOURCE_D1HCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&clk) != HAL_OK)
        return 1;

    __HAL_RCC_QSPI_CLK_ENABLE();
    QSPI_GpioInit();

    memset(&hqspi, 0, sizeof(hqspi));
    hqspi.Instance                = QUADSPI;
    hqspi.Init.ClockPrescaler     = 2;        /* same as quadspi.c */
    hqspi.Init.FifoThreshold      = 4;
    hqspi.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    hqspi.Init.FlashSize          = 22;       /* 2^(22+1) = 8 MB */
    hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_2_CYCLE;
    hqspi.Init.ClockMode          = QSPI_CLOCK_MODE_0;
    hqspi.Init.FlashID            = QSPI_FLASH_ID_1;
    hqspi.Init.DualFlash          = QSPI_DUALFLASH_DISABLE;

    return (HAL_QSPI_Init(&hqspi) == HAL_OK) ? 0 : 1;
}

/* ================================================================
 * W25Q64 SPI helpers
 * ================================================================ */
static int W25Q_ReadSR(uint8_t reg_cmd, uint8_t *val)
{
    QSPI_CommandTypeDef c = {0};
    c.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    c.Instruction     = reg_cmd;
    c.DataMode        = QSPI_DATA_1_LINE;
    c.NbData          = 1;

    if (HAL_QSPI_Command(&hqspi, &c, HAL_MAX_DELAY) != HAL_OK)
        return 1;
    return (HAL_QSPI_Receive(&hqspi, val, HAL_MAX_DELAY) == HAL_OK) ? 0 : 1;
}

static int W25Q_WriteEnable(void)
{
    QSPI_CommandTypeDef c = {0};
    c.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    c.Instruction     = CMD_WRITE_ENABLE;

    if (HAL_QSPI_Command(&hqspi, &c, HAL_MAX_DELAY) != HAL_OK)
        return 1;
    return 0;
}

static int W25Q_WaitBusy(uint32_t timeout_ms)
{
    QSPI_CommandTypeDef c   = {0};
    QSPI_AutoPollingTypeDef p = {0};

    c.InstructionMode  = QSPI_INSTRUCTION_1_LINE;
    c.Instruction      = CMD_READ_SR1;
    c.DataMode         = QSPI_DATA_1_LINE;
    c.NbData           = 1;

    p.Match            = 0;
    p.Mask             = SR1_BUSY;
    p.MatchMode        = QSPI_MATCH_MODE_AND;
    p.StatusBytesSize  = 1;
    p.Interval         = 0x10;
    p.AutomaticStop    = QSPI_AUTOMATIC_STOP_ENABLE;

    return (HAL_QSPI_AutoPolling(&hqspi, &c, &p, timeout_ms) == HAL_OK) ? 0 : 1;
}

static int W25Q_EnableQuad(void)
{
    uint8_t sr2 = 0;
    if (W25Q_ReadSR(CMD_READ_SR2, &sr2) != 0)
        return 1;
    if (sr2 & SR2_QE)
        return 0;   /* already enabled */

    if (W25Q_WriteEnable() != 0)
        return 1;

    sr2 |= SR2_QE;
    QSPI_CommandTypeDef c = {0};
    c.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    c.Instruction     = CMD_WRITE_SR2;
    c.DataMode        = QSPI_DATA_1_LINE;
    c.NbData          = 1;

    if (HAL_QSPI_Command(&hqspi, &c, HAL_MAX_DELAY) != HAL_OK)
        return 1;
    if (HAL_QSPI_Transmit(&hqspi, &sr2, HAL_MAX_DELAY) != HAL_OK)
        return 1;
    return W25Q_WaitBusy(200);
}

/* ================================================================
 * CMSIS Flash Algorithm API (return 0 = OK)
 * ================================================================ */

int Init(uint32_t adr, uint32_t clk, uint32_t fnc)
{
    (void)adr;
    (void)clk;
    (void)fnc;

    SystemInit();
    HAL_Init();

    if (QSPI_Configure() != 0)
        return 1;
    if (W25Q_EnableQuad() != 0)
        return 1;
    return 0;
}

int UnInit(uint32_t fnc)
{
    (void)fnc;
    HAL_QSPI_DeInit(&hqspi);
    __HAL_RCC_QSPI_CLK_DISABLE();
    return 0;
}

int EraseChip(void)
{
    if (W25Q_WriteEnable() != 0)
        return 1;

    QSPI_CommandTypeDef c = {0};
    c.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    c.Instruction     = CMD_CHIP_ERASE;

    if (HAL_QSPI_Command(&hqspi, &c, HAL_MAX_DELAY) != HAL_OK)
        return 1;

    return W25Q_WaitBusy(200000);   /* chip erase ≤ 100 s */
}

int EraseSector(uint32_t adr)
{
    uint32_t offset = adr - QSPI_BASE_ADDR;

    if (W25Q_WriteEnable() != 0)
        return 1;

    QSPI_CommandTypeDef c = {0};
    c.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    c.Instruction     = CMD_SECTOR_ERASE_4K;
    c.AddressMode     = QSPI_ADDRESS_1_LINE;
    c.AddressSize     = QSPI_ADDRESS_24_BITS;
    c.Address         = offset;

    if (HAL_QSPI_Command(&hqspi, &c, HAL_MAX_DELAY) != HAL_OK)
        return 1;

    return W25Q_WaitBusy(3000);
}

int ProgramPage(uint32_t adr, uint32_t sz, uint8_t *buf)
{
    uint32_t offset = adr - QSPI_BASE_ADDR;

    /* W25Q64 hardware page = 256B; pyocd may pass up to 64KB.
     * Split into 256B chunks respecting page boundaries. */
    while (sz > 0) {
        uint32_t page_remain = 256U - (offset & 0xFFU);
        uint32_t chunk = (sz < page_remain) ? sz : page_remain;

        if (W25Q_WriteEnable() != 0)
            return 1;

        QSPI_CommandTypeDef c = {0};
        c.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        c.Instruction     = CMD_QUAD_PAGE_PROGRAM;
        c.AddressMode     = QSPI_ADDRESS_1_LINE;
        c.AddressSize     = QSPI_ADDRESS_24_BITS;
        c.Address         = offset;
        c.DataMode        = QSPI_DATA_4_LINES;
        c.NbData          = chunk;

        if (HAL_QSPI_Command(&hqspi, &c, HAL_MAX_DELAY) != HAL_OK)
            return 1;
        if (HAL_QSPI_Transmit(&hqspi, buf, HAL_MAX_DELAY) != HAL_OK)
            return 1;
        if (W25Q_WaitBusy(1000) != 0)
            return 1;

        offset += chunk;
        buf    += chunk;
        sz     -= chunk;
    }

    return 0;
}

uint32_t Verify(uint32_t adr, uint32_t sz, uint8_t *buf)
{
    uint32_t offset = adr - QSPI_BASE_ADDR;
    uint8_t  tmp[256];

    while (sz > 0) {
        uint32_t chunk = (sz > sizeof(tmp)) ? sizeof(tmp) : sz;

        QSPI_CommandTypeDef c = {0};
        c.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        c.Instruction     = CMD_QUAD_FAST_READ;
        c.AddressMode     = QSPI_ADDRESS_1_LINE;
        c.AddressSize     = QSPI_ADDRESS_24_BITS;
        c.Address         = offset;
        c.DataMode        = QSPI_DATA_4_LINES;
        c.NbData          = chunk;
        c.DummyCycles     = 8;

        if (HAL_QSPI_Command(&hqspi, &c, HAL_MAX_DELAY) != HAL_OK)
            return offset + QSPI_BASE_ADDR;
        if (HAL_QSPI_Receive(&hqspi, tmp, HAL_MAX_DELAY) != HAL_OK)
            return offset + QSPI_BASE_ADDR;

        for (uint32_t i = 0; i < chunk; i++) {
            if (tmp[i] != buf[i])
                return offset + i + QSPI_BASE_ADDR;
        }

        offset += chunk;
        buf    += chunk;
        sz     -= chunk;
    }

    return offset + QSPI_BASE_ADDR;   /* success: return end address */
}
