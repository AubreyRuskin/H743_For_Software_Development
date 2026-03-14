#include "bsp.h"
#include <time_compat.h>
#include <stdio_compat.h>
#include <string_compat.h>

uint32_t hdlcRecvNum;

void vxTimeBaseGet(UINT32 *pTbu, UINT32 *pTbl)
{
    /* TODO: 用 DWT->CYCCNT 或 HAL_GetTick 实现高精度时间戳 */
    if (pTbu) *pTbu = 0;
    if (pTbl) *pTbl = 0;
}

unsigned char Get_AD_Chip_Count(void) { return 0; }
short Get_AD_Value(unsigned char chipId) { (void)chipId; return 0; }
void Init_Net(void) { }
void Init_Telnet(void) { }
int IoPinInputHigh(IO_PIN_IN_FUN_TYPE funtype) { (void)funtype; return 0; }
int IoPinOutputHigh(IO_PIN_OUT_FUN_TYPE funtype, int outVal) { (void)funtype; (void)outVal; return 0; }
int Set_EthIP(unsigned char port, unsigned char *addr) { (void)port; (void)addr; return 0; }
int Set_EthMacAdrs(unsigned char port, unsigned char *addr) { (void)port; (void)addr; return 0; }
int Set_HdlcIP(unsigned char port, unsigned char *addr) { (void)port; (void)addr; return 0; }
void Set_Hdlc_Out_Bit(unsigned char settingBit) { (void)settingBit; }
int goose_send_raw(uint8_t portNum, uint8_t *sendBuf, int sendNum) { (void)portNum; (void)sendBuf; (void)sendNum; return 0; }
int sysClkRateGet(void) { return 100; }
void Write_FPGA_Program(void) { }

int16_t Get_Boot_Context(void) { return -1; }
int Get_Boot_Info(void) { return -1; }
BOOL IS_Boot_From_Net(void) { return 0; }

int read_ram_data(unsigned short addr, unsigned char *pBuf, unsigned short length)
{
    (void)addr; (void)pBuf; (void)length;
    return 0;
}

uint16_t GetBspVer(void) { return 1; }

int write_ram_data(unsigned short addr, unsigned char *pBuf, unsigned short length)
{
    (void)addr; (void)pBuf; (void)length;
    return 0;
}

BOOL SIO_GetIOExsitSts(int iModAddr) { (void)iModAddr; return TRUE; }
BOOL SIO_Is_Open_QD(void) { return 0; }
int Get_Sys_Hw_Clock(UINT8 *buf) { (void)buf; return 0; }
int Set_Sys_Hw_Clock(UINT8 *buf) { (void)buf; return 0; }

char *sysModel(void) { return "STM32H743"; }
STATUS sysToMonitor(int startType) { (void)startType; return ERROR; }
int sysProcNumGet(void) { return 0; }
unsigned int Ffx_Get_Nand_Size_In_MegaByte(void) { return 0; }

int mCastAddrAdd(unsigned char port, unsigned char *addr)
{
    /* TODO: 用 lwIP netif 接口实现多播地址添加 */
    (void)port; (void)addr;
    return 0;
}

int set_i2c_mux_val(unsigned char index) { (void)index; return 0; }
int get_sfp_status_val(unsigned char begin_adrs, unsigned char byte_cnt, unsigned char *buf)
{
    (void)begin_adrs; (void)byte_cnt; (void)buf;
    return 0;
}
