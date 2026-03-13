/* bspinterface.h - subroutine library for interface to BSP */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 19may09, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for interface to BSP.
*/

#ifndef BSPINTERFACE_H
#define BSPINTERFACE_H

#include "taskLib.h"
// #include "bsp.h"

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

// #include "config04.h"
// #include "hal.h"
#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
#include "config05.h"
#endif

#if defined(EDP_01_02_BUILD)

/*
 * 该文件若干定义与scc_hdlc_raw.h文件重复，暂时屏蔽
 * #include "smc_uart_raw.h"
 */

#endif

// #include "goose_eth.h"

#include "scc_hdlc_raw.h"

/* global functions */

extern uint32_t hdlcRecvNum;
extern uint32_t hdlcSendNum;

/* defines */



typedef enum IO_PIN_OUT_ARRAY
{
    IO_OUT_10PPS = 0,  /*"10PPS"*/
    IO_OUT_GJ,              /*"GJ_OUT"*/
    IO_OUT_GJ2,            /*"GJ_OUT2"*/
    IO_OUT_GJ_EN,              /*"GJ_EN"*/
    IO_OUT_QD,              /*"QD_OUT"*/
    IO_OUT_QD2,              /*"QD_OUT2"*/
    IO_OUT_QD_EN,	    /*"QD_EN"*/
    IO_OUT_HH,			/*"HH_OUT"*/
    IO_OUT_HH_EN,   		/*"HH_EN*/
    IO_OUT_PULSE,  	/*"PULSE_OUT"*/
    IO_OUT_LED1,		/*indicator*/
    IO_OUT_LED2,		/*indicator*/
    IO_OUT_LED3,		/*indicator*/
    IO_OUT_QD_TEST,		/*"QD_TEST"*/
    IO_OUT_PPS,				/*"PPS"*/
    IO_OUT_RESET_MEGA,		/*"RESET_MEGA"*/
    IO_OUT_RESET_PANEL,	/*"RESET_PANEL"*/
    IO_OUT_FG,  				/*"FG_OUT"*/
    IO_OUT_MASTER,  		       /*"MASTER_OUT"*/
    IO_OUT_MASTER2,  		/*"MASTER_OUT2"*/
    IO_OUT_TRIP1,  		/*"TRIP1"*/
    IO_OUT_TRIP2,  		/*"TRIP2"*/
    IO_OUT_TRIP3,  		/*"TRIP3"*/

    IO_PIN_OUT_COUNT,       /*GPIO output num*/
} IO_PIN_OUT_FUN_TYPE;


/*IO引脚功能*/
typedef enum IO_PIN_IN_ARRAY
{
    IO_IN_GJ = 0,      		/*"GJ_IN",GJ feed back*/
    IO_IN_QD_RET,           /*"QD_IN",QD feed back*/
    IO_IN_CPU1_PULSE,    /*"CPU_PULSE_IN1",接收CPU1的心跳脉冲*/
    IO_IN_CPU2_PULSE,     /*"CPU_PULSE_IN2",接收CPU2的心跳脉冲*/
    IO_IN_HH_FB,			/*"HH_IN",HH feed back*/
    IO_IN_IRIGB,			/*"IRIGB",IRIGB*/
    IO_IN_DO_FB_IN1,		/*"DO_FB_IN1",LPS-CPU.A-A*/
    IO_IN_DO_FB_IN2,		/*"DO_FB_IN2",LPS-CPU.A-A*/
    IO_IN_DO_EN_FB,		/*"DO_EN_FB",LPS-CPU.A-A*/
    IO_IN_CC_ON,			/*"CC_ON",LPS-CPU.A-A*/
    IO_IN_DO_ON,		/*"DO_ON",LPS-CPU.A-A*/
    IO_IN_TRIP_ON,		/*"TRIP_ON",LPS-CPU.A-A*/
    IO_IN_DI_ON,			/*"DI_ON",LPS-CPU.A-A*/
    IO_IN_FPGA_PPS,		/*"FPGA_PPS",LPS-CPU.A-A*/
    IO_IN_SPI_MASTER,           /*"MASTER_IN",SPI 主模式反馈*/

    IO_PIN_IN_COUNT,        /*GPIO input num*/
} IO_PIN_IN_FUN_TYPE;

/* SFP光口功率读取 */
#define	I2C_MUX_ADRS	0xEE
#define	SFP_STATUS_ADRS	0xA2
#define	SFP_DATA_ADRS	0xA0

#define	SFP_TEMP_ADRS			96
#define	SFP_INTER_VCC_ADRS		98
#define	SFP_VCC_TO_TMP_OFFSET	(SFP_INTER_VCC_ADRS - SFP_TEMP_ADRS)
#define	SFP_TX_PWR_ADRS			102
#define	SFP_TX_P_TO_TMP_OFFSET	(SFP_TX_PWR_ADRS-SFP_TEMP_ADRS)
#define	SFP_RX_PWR_ADRS			104
#define	SFP_RX_P_TO_TMP_OFFSET	(SFP_RX_PWR_ADRS-SFP_TEMP_ADRS)

#define	SFP_ALARM_AND_WARM_ADRS			112 /* 告警信息 */

#define	VCC_SCALE				(100E-6)	/*100uV*/
#define	PWR_SCALE				(0.1E-6)	/*0.1uW*/
#define TEMP_SCALE 256  /* 温度码值分频 */
#define CPU_SFP_NUM 4  /* CPU板SFP光口数量 */



#define IO_PIN_HIGH 1
#define IO_PIN_LOW 0
#define PD21  21
#define IO_OUT_RUN_LED_PULSE 23
#define IO_OUT_RUN_LED_ON 24
/* global functions */

// extern void motFccRawInit(void);		/* 扩展机箱接口初始化,BSP提供 */
// extern void externBSP();		/* 网络的初始化 */
// extern void disablePitKickDog();	/* BSP提供函数 */
// extern void swWatchDogInit();
// extern void kickSwDog();
// extern void kickHwDog();
// extern STATUS set_date(int year, int month, int day, int hour, int minute, int second);
// extern STATUS show_date(void);

// #if defined(EDP_01_02_BUILD)
// // extern int Only_Init_Nand(void);
// #endif

/***********************************************************************
* boot_reason - 功能，返回BOOT的原因
*
* RETURNS: boot的原因
*
*/
// extern int boot_reason();

// extern BOOL IS_Quick_Boot_Mode();		/* 获取BOOT_SEL管脚状态 */
extern BOOL IS_Boot_From_Net();		/* 获取QUICK_BOOT管脚状态 */

extern void vxTimeBaseGet (UINT32 * pTbu, UINT32 * pTbl);		/* 获取64位计数器时间 */
// extern int SecondECardConfig(void);	/* 初始化第2块网卡 */
// extern void beginHdlc();	/* 初始化HDLC */
// extern int fm24cl64chk(void);	/* 铁电自检 */
extern unsigned int vxImmrGet(void);
extern UINT32 sysInputFreqGet
(
    void
);

/*  Function:   get system hardware clock
    Parameter:  buf: buffer to hold time information
                time is in BCD format
        index of time:  0;  Second
                        1;  Minute
                        2;  Hour
                        3;  Day of week
                        4;  Date
                        5;  Month
                        6;  Year
    Return: OK, ERROR
*/
extern int Get_Sys_Hw_Clock(UINT8 *buf);

/*  Function:   set system hardware clock
    Parameter:  buf: buffer to hold time information
                time is in BCD format
        index of time:  0;  Second
                        1;  Minute
                        2;  Hour
                        3;  Day of week
                        4;  Date
                        5;  Month
                        6;  Year
    Return: OK, ERROR
*/
extern int Set_Sys_Hw_Clock(UINT8 *buf);

extern int16_t Get_Boot_Context();

#if defined(EDP_01_02_BUILD)

/***********************************************************************
* Ffx_Get_Nand_Size_In_MegaByte - 获得nandFlash的大小
*
* RETURNS: NANDFLASH大小，以M为单位
*
*/
extern unsigned int Ffx_Get_Nand_Size_In_MegaByte();

#endif

extern uint16_t GetBootromVer();
extern uint16_t GetBspVer();
extern char *sysBspRev(void);
extern int sysProcNumGet(void);
extern STATUS sysToMonitor
(
    int startType             /* parameter passed to ROM to tell it how */
    /* to boot */
);

extern TASK_ID taskIdSelf (void);

/*  Function:   get AD chip number
    return value: AD chip number
*/
extern unsigned char Get_AD_Chip_Count();

/*  Function: get AD value
    Parameter: chipId; 1 to Get_AD_Chip_Count()
    return value: AD Value
*/
extern short Get_AD_Value(unsigned char chipId);

/* 功能: 获取系统温度 */
extern int8_t Get_Sys_Temperature();

/* 初始化下方的FCC以太网口,要使用网络必须首先调用改函数 */
extern void Init_Net();



/* 初始化Telnet */
extern void Init_Telnet();

/*  功能:   获得指定引脚为高或低
    参数:   funtype:    从IO_PIN_IN_FUN_TYPE中取值
    返回值: 从IO_PIN_VAL
            没有相应的输入函数,返回值为IO_FUN_NULL
*/
extern int IoPinInputHigh(IO_PIN_IN_FUN_TYPE funtype);

/*  功能:   输出指定引脚为高或低
    参数:   funtype:    从IO_PIN_OUT_FUN_TYPE中取值
            outVal:     从IO_PIN_VAL中取值
    返回值: 控制成功,返回值和outVal一致
            没有相应的输出函数,返回值为IO_FUN_NULL
*/
extern int IoPinOutputHigh(IO_PIN_OUT_FUN_TYPE funtype, int outVal);

/*  Function:   Set ethernet IP
    Parameter:  port: begin from 0
                addr: pointer to buffer holding IP address
    Return value:   ERROR;  no IP set
                    OK;     set IP ok
    Note:   if you want set the IP address as
        172.30.20.56
        you should set addr[0] = 172; addr[1] = 30; addr[2] = 20; addr[3] = 56
*/
extern int Set_EthIP(unsigned char port, unsigned char *addr);

/*  Function:   Set ethernet mac address
    Parameter:  port: begin from 0
                addr: pointer to buffer holding last three bytes of mac address
    Return value:   ERROR;  no mac address set
                    OK;     set mac address ok
    Note:   if you want set the MAC address as
        xx.xx.xx.12.34.56
        you should set addr[0] = 12; addr[1] = 34; addr[2] = 56
*/
extern int Set_EthMacAdrs(unsigned char port, unsigned char *addr);

/*  Function:   Set HDLC IP
    Parameter:  port: begin from 0
                addr: pointer to buffer holding IP address
    Return value:   ERROR;  no IP set
                    OK;     set IP ok
    Note:   if you want set the IP address as
        10.10.10.3
        you should set addr[0] = 10; addr[1] = 10; addr[2] = 10; addr[3] = 3
*/
extern int Set_HdlcIP(unsigned char port, unsigned char *addr);

/*CLEI 2008.0508
    以下两个函数Set_Hdlc_Out_Bit和Clear_Hdlc_Out_Bit是空函数,
    为了满足嵌软平台的要求。

*/
/*  功能:   设置HDLC要置高的开出
    参数:   要置高的开出,从上面define的值中取
*/
extern void Set_Hdlc_Out_Bit(unsigned char settingBit);

extern int goose_send_raw(uint8_t portNum, uint8_t *sendBuf, int sendNum);

/* judge if it is the second board.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL isNumber_2_04CPU(void);

extern int smc_recv_raw(UINT8 smcNum, uint8_t **ppucBuf,int iTimeout);
extern int smc_send_raw(UINT8 smcNum, UINT8 *pSend, UINT16 len);
extern int sysClkRateGet (void);
extern char * sysModel
(
    void
);

extern void    Write_FPGA_Program();

/*  Read data from RAM
    Parameter:  addr;   address of RAM to read data
                pBuf;   Pointer to buffer
                length; length of data to read
    return value: ERROR, OK
*/
extern int read_ram_data(
    unsigned short addr,
    unsigned char *pBuf,
    unsigned short length
);

/*  Write data from RAM
    Parameter:  addr;   address of RAM to read data
                pBuf;   Pointer to buffer
                length; length of data to read
    return value: ERROR, OK
*/
extern int write_ram_data(
    unsigned short addr,
    unsigned char *pBuf,
    unsigned short length
);

/*
stop the scc,then init. the hdlc(call the funtion "STATUS m8260SccHdlcInit(char sccNum)")
Parameters:
	sccNum =3, for CHAN_DOWN;
	sccNum =4, for CHAN_UP.
Return value:0 means OK while -1 means error.

*/
extern STATUS hdlc_channel_reset(UINT8  sccNum);

extern void hdlc_chnstatus_get(UINT8 channelNum, HDLC_RECV_STATUS *channel_status);

/*channel clk master or slave
Parameters:
    channelNum:CHAN_UP or CHAN_DOWN
    clkMstSet: clkMstSet=TRUE, then master

Return value:0 means OK while -1 means error.
*/
extern STATUS  hdlc_clk_master_set(UINT8 channelNum, BOOL clkMstSet);

/*channel clk select
Parameters:
    channelNum:CHAN_UP or CHAN_DOWN
    clk: CLK_2MHZ or CLK_64KHZ
Return value:0 means OK while -1 means error.
*/
extern int  hdlc_clk_select(UINT8 channelNum, UINT8 clk);

/* Send raw data via hdlc.
 * Parameters:
 *      sendData, pointer to user data to be send.This buffer must contain
 *          HDLC_HEAD_LEN bytes space before the user data area.
 *      len, length of user data.  It should<=HDLC_FRAME_LEN and !=0.
 * Return value:
 *      HDLC_S_STATUS */
extern int hdlc_send_raw(UINT8 channelNum, UINT8 *, UINT16 len);

extern void ext_recv_flush_recvbuf(void);		/* flush the HDLC received data buffer. */

/* 设置信号量互锁 */
extern int set_i2c_mux_val(unsigned char index);

/* 获取光口状态 */
extern int get_sfp_status_val(unsigned char begin_adrs, unsigned char byte_cnt, unsigned char *buf);

extern int Get_Boot_Info();

extern int mCastAddrAdd(unsigned char port, unsigned char *addr);

/*
 * __assert
 * __ctype
 * __divdi3
 * __errno
 * __stderr
 * __stdout
 * __udivdi3
 * atan
 * atan2
 * atof
 * atoi
 * atol
 * abs
 * accept
 * bind
 * bsearch
 * taskSpawn
 * kernelTimeSlice
 * bzero
 * calloc
 * clock_gettime
 * clock_settime
 * close
 * closedir
 * connect
 * connectWithTimeout
 * cos
 * creat
 * ctime
 * difftime
 * errnoGet
 * etherMacAddrGet
 * excConnect
 * exit
 * exp
 * fabs
 * fclose
 * fflush
 * fgets
 * floor
 * fopen
 * fpClassId
 * fprintf
 * fputs
 * fread
 * fscanf
 * fseek
 * fstat
 * ftell
 * ftruncate
 * fwrite
 * getpeername
 * getsockopt
 * gmtime
 * hostGetByName
 * inet_addr
 * inet_ntoa
 * intCnt
 * intConnect
 * intDisable
 * intEnable
 * intLock
 * intUnlock
 * ioctl
 * kill
 * ldiv
 * listen
 * localtime
 * localtime_r
 * logMsg
 * lseek
 * lstAdd
 * lstCount
 * lstDelete
 * lstFirst
 * lstFree
 * lstGet
 * lstInit
 * lstInsert
 * lstNext
 * lstNth
 * m8260InumToIvec
 * m8260SccHdlcInit
 * m8260SmcUartInit_Parity
 * mCastAddrAdd
 * mCastAddrGet
 * malloc
 * memcmp
 * memcpy
 * memmove
 * memset
 * mkdir
 * mktime
 * nanosleep
 * open
 * opendir
 * pow
 * printErrno
 * printf
 * puts
 * qsort
 * read
 * readdir
 * realloc
 * reboot
 * rebootHookAdd
 * recv
 * reg_Goose_Recv_Fun
 * reg_Hdlc_Recv_Fun
 * remove
 * rename
 * rmdir
 *
 */

/*
 * msgQCreate
 * msgQDelete
 * msgQReceive
 * msgQSend
 */

/*
 * pthread_attr_init
 * pthread_attr_setdetachstate
 * pthread_cond_broadcast
 * pthread_cond_destroy
 * pthread_cond_init
 * pthread_cond_signal
 * pthread_cond_timedwait
 * pthread_cond_wait
 * pthread_create
 * pthread_join
 * pthread_mutex_destroy
 * pthread_mutex_init
 * pthread_mutex_lock
 * pthread_mutex_unlock
 * pthread_mutexattr_destroy
 * pthread_mutexattr_init
 * pthread_self
*/

#ifdef  __cplusplus
}
#endif

#endif
