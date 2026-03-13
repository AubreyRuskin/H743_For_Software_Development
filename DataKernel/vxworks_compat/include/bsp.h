#ifndef BSP_H
#define BSP_H

#include "bspinterface.h"



void vxTimeBaseGet (UINT32 * pTbu, UINT32 * pTbl);

unsigned char Get_AD_Chip_Count();
short Get_AD_Value(unsigned char chipId);
void Init_Net();
void Init_Telnet();
int IoPinInputHigh(IO_PIN_IN_FUN_TYPE funtype);
int IoPinOutputHigh(IO_PIN_OUT_FUN_TYPE funtype, int outVal);
int Set_EthIP(unsigned char port, unsigned char *addr);
int Set_EthMacAdrs(unsigned char port, unsigned char *addr);
int Set_HdlcIP(unsigned char port, unsigned char *addr);
void Set_Hdlc_Out_Bit(unsigned char settingBit);
int goose_send_raw(uint8_t portNum, uint8_t *sendBuf, int sendNum);
int sysClkRateGet (void);
void    Write_FPGA_Program();
int set_i2c_mux_val(unsigned char index);
int get_sfp_status_val(unsigned char begin_adrs, unsigned char byte_cnt, unsigned char *buf);
int16_t Get_Boot_Context();
int Get_Boot_Info();
 BOOL IS_Boot_From_Net();		

 int read_ram_data(
    unsigned short addr,
    unsigned char *pBuf,
    unsigned short length
);

uint16_t GetBspVer();
int write_ram_data(
    unsigned short addr,
    unsigned char *pBuf,
    unsigned short length
);

BOOL SIO_GetIOExsitSts(int iModAddr);
BOOL SIO_Is_Open_QD();
int Get_Sys_Hw_Clock(UINT8 *buf);
int Set_Sys_Hw_Clock(UINT8 *buf);
char * sysModel(void);
STATUS sysToMonitor(    int startType      );

int sysProcNumGet(void);

unsigned int Ffx_Get_Nand_Size_In_MegaByte();

int mCastAddrAdd(unsigned char port, unsigned char *addr);


#endif 