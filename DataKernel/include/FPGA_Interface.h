
#include "vxworks_type.h"

#define FPGA_BASE_ADDR	(0xa2000000)

#define FPGA_TRANSMITTER_OFFSET (0x04000)
#define FPGA_TRANSMITTER_ADDR_MAP_OFFSET (0x00)
#define FPGA_TRANSMITTER_PORT_SRC_ADDR_OFFSET (0x80)
#define FPGA_TRANSMITTER_RCV_PORT_START_NUM_OFFSET (0x100)
#define FPGA_TRANSMITTER_SRC_MAC_REPLACE_OFFSET (0x104)
#define FPGA_SHARED_MEM_STATUS_OFFSET (0x108)

#define FPGA_GMRP_SEND_OFFSET (0x4400)
#define FPGA_GMRP_REG0_OFFSET (0x00)
#define FPGA_GMRP_REG1_OFFSET (0x04)
#define FPGA_GMRP_REG2_OFFSET (0x08)
#define FPGA_GMRP_SEND_BUFFER_OFFSET (0x80)

#define FPGA_GMRP_SEND_BUFFER_LEN (0x80)

#define FPGA_GOOSE_OFFSET (0x08000)
#define FPGA_GOOSE_MODER_OFFSET (0x00)
#define FPGA_GOOSE_STATUS_OFFSET (0x04)
#define FPGA_GOOSE_INTMASK_OFFSET (0x08)
#define FPGA_GOOSE_MAC_ADDR0_OFFSET (0x0C)
#define FPGA_GOOSE_MAC_ADDR1_OFFSET (0x10)
#define FPGA_GOOSE_HASH0_OFFSET (0x14)
#define FPGA_GOOSE_HASH1_OFFSET (0x18)
#define FPGA_GOOSE_BD_NUM_OFFSET (0x1C)
#define FPGA_GOOSE_TX_BD_OFFSET (0x80)
#define FPGA_GOOSE_RX_BD_OFFSET (0x100)

#define FPGA_HSB_STYLE_GOOSE_RX_REG_OFFSET (0x2000)
#define FPGA_HSB_STYLE_GOOSE_RX_BUFFER_OFFSET (0x2100)

#define FPGA_TRANSMITTER_MAX_MULTI_ADDR_NUM (16)

typedef struct
{
    uint32_t *pBaseAddr;
    uint32_t *pModerAddr;
    uint32_t *pStatusAddr;
    uint32_t *pIntMaskAddr;
    uint32_t *pMacAddr0Addr;
    uint32_t *pMacAddr1Addr;
    uint32_t *pHash0Addr;
    uint32_t *pHash1Addr;
    uint32_t *pBDNumAddr;
    uint32_t *pTxBDAddr;
    uint32_t *pRxBDAddr;
} FPGA_GOOSE_ADDR;

typedef struct
{
    uint32_t *pExchangeReg;
    uint32_t *pRxBufAddr;
} FPGA_HSB_STYLE_GOOSE_RX_ADDR;

typedef struct
{
    uint32_t *pAddrMapAddr0;
    uint32_t *pAddrMapAddr1;
} FPGA_TRANS_AD_MAP_ADDR;

typedef struct
{
    uint32_t *pSourceAddrAddr0;
    uint32_t *pSourceAddrAddr1;
} FPGA_TRANS_PORT_SRC_AD_ADDR;

typedef struct
{
    uint32_t *pReceivePortStartNumAddr;
    uint32_t *pSourceMacReplaceAddr;
    uint32_t *pSharedMemStatAddr;
} FPGA_ADD_ON_INFO_ADDR;

typedef struct
{
    uint32_t *pGmrpReg0Addr;
    uint32_t *pGmrpReg1Addr;
    uint32_t *pGmrpReg2Addr;
} FPGA_GMRP_REG_ADDR;

typedef struct
{
    uint32_t *pGmrpSendBufAddr;
} FPGA_GMRP_SEND_BUFFER_ADDR;

int Receive_Goose_from_FPGA(uint8_t portNum);

int Send_Goose_to_FPGA(uint8_t portNum, uint8_t *sendBuf, int sendNum);

STATUS Add_Goose_Multi_Addr_to_FPGA(uint8_t portNum, char *pAddr);

STATUS Del_Goose_Multi_Addr_from_Mac_Ctrler(uint8_t portNumtoCPU, char *pAddr);

STATUS Get_Goose_Mac_Addr_from_Mac_Ctrler(uint8_t portNum, uint8_t *addr);

STATUS Set_Goose_Mac_Addr_to_FPGA(uint8_t portNum, uint8_t *addr);

STATUS Add_Goose_Multi_Addr_to_Transmitter(uint8_t SrcPortNum, char *pAddr, uint8_t DestPortNum);

STATUS Add_Goose_Mac_Addr_to_Transmitter(uint8_t PortNum, char *pSrcAddr);

STATUS Get_Goose_Mac_Addr_from_Transmitter(uint8_t PortNum, uint8_t *addr);

BOOL Poll_FPGA_Goose_Receive();

BOOL Init_FPGA_Goose();

int Send_Gmrp_to_FPGA(uint8_t portNum, uint8_t *sendBuf, int sendNum);

BOOL Init_FPGA_GMRP();