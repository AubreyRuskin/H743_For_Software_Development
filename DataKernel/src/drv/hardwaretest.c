/* hardwaretest.c - This file contains hardware testing code. */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 9may08, dy first created.
*/

/*
DESCRIPTION
This file contains hardware testing code.
*/

/* includes */

#include "edpbase.h"

#include "realdata.h"
#include "logmsg.h"
#include "errtest.h"
#include "hwcfg.h"
#include "swcfg.h"
#include "dspai.h"
#include "filetool.h"
#include "RE_RelayEngine.h"
#include "sysinfo.h"
#include "view.h"
#include "rec.h"
#include "measure.h"
#include "auto_upload.h"
#include "man_fram.h"
#include "VoltageWatch.h"

#ifdef EXCITE_BUILD
#include "Ao_Drv.h"
#include "adc.h"
#include "HardClock.h"
#endif

#include <stdio_compat.h>
#include <taskLib.h>
#include <logLib.h>
#include <intLib.h>
// #include <rebootLib.h>
// #include <drv/mem/m8260Siu.h>
// #include <drv/sio/m8260Sio.h>
#include "UsbMmiInterface.h"
#include "configerrordisp.h"
#include "protectmmiinterface.h"
#include "edp_asst.h"

/* The following is used for exception processing. */
/* #include "sbcm8260Siu.h" */
#include "m8260IntrCtl.h"

#include "excLib.h"
// #include "arch\ppc\esfPpc.h"

// #include "config04.h"
#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
#include "CONFIG05.h"
#endif

/* 合并版本所有平台包含 */
#include "POLE_VtBox.h"				/* Use for same pole equipments. */
#include "HDL_VtBox.h"				/* Intellegent operation box. */
#include "GO_Interface.h"
// #include "goose_rslv.h" 						/* Used for goose */

#include "EdpNetCfg.h"			/* Net configuration. */
#include "EdpVer.h"      	/* Version control. */

#include "scc_hdlc_raw.h"

// #include "symbol.h"

/* defines */

#define HARDWAREDEBUG 0
/* #define INCLUDE_E03_BA_RS485_TEST */		/* 485 test macro */

/* structs */

/*
 * Allocated block header.
 * NOTE: The size of this data structure must be aligned on _ALLOC_ALIGN_SIZE.
 */

typedef struct memblockHdr		/* BLOCK_HDR */
{
    struct memblockHdr *	pPrevHdr;	/* pointer to previous block hdr */
    unsigned		nWords : 31;	/* size in words of this block */
    unsigned		free   : 1;	/* TRUE = this block is free */
#if (_ALLOC_ALIGN_SIZE == 16)
    UINT32		pad[2];		/* 8 byte pad for round up */
#endif	/* _ALLOC_ALIGN_SIZE == 16 */
} MEM_BLOCK_HDR;

/* globals */

#if HARDWAREDEBUG
extern ULONG standTblSize;
extern SYMBOL standTbl [];
#endif

ULONG statTblSize;
// extern SYMBOL statTbl[];

/* functions */

/***********************************************************************
* FmTest - 铁电自检
*
* RETURNS: 无
*
*/
void FmTest(void)
{
    int32_t iFmChkRst;
    int i;

#if defined(EDP03_BUILD)

    for(i=0; i<1000; i++)
    {
        iFmChkRst=fm24cl64chk();

        if(iFmChkRst == FRAM_DEVICE_OK)
        {
            static uint32_t ulCnt=0;

            ulCnt++;
            if(ulCnt%1000 == 1)
            {
                if(ENG_MODE == 0)
                    LOG_Dbg_Msg("铁电自检成功!\n", 0, 0, 0, 0, 0, 0);
                else if(ENG_MODE == 1)
                    LOG_Dbg_Msg(" fm24cl64 is OK!\n", 0, 0, 0, 0, 0, 0);
            }
        }
        else if(iFmChkRst == FRAM_DEVICE_ERR)
        {
            if(ENG_MODE == 0)
                LOG_Dbg_Msg("铁电自检失败!\n", 0, 0, 0, 0, 0, 0);
            else if(ENG_MODE == 1)
                LOG_Dbg_Msg("fm24cl64 is Error!\n", 0, 0, 0, 0, 0, 0);
        }
        else if(iFmChkRst == FRAM_DEVICE_LOST)
        {
            if(ENG_MODE == 0)
                LOG_Dbg_Msg("系统中没有铁电或自检失败!\n", 0, 0, 0, 0, 0, 0);
            else if(ENG_MODE == 1)
                LOG_Dbg_Msg("fm24cl64 is lost or error!\n", 0, 0, 0, 0, 0, 0);
        }

        taskDelay(10);
    }
#endif
}

/***********************************************************************
* GetSysSymbl - 获取系统符号表
*
* RETURNS: 无
*
*/
void GetSysSymbl(void)
{
    int i;

    LOG_OpenLog();

#if HARDWAREDEBUG
    logMsg("standTblSize=%d\\n", standTblSize, 0, 0, 0, 0, 0);

    for(i=0; i<standTblSize; i++)
    {
        LOG_Dbg_Msg("%d name=%s address=%x\n", (int)i, (int)standTbl[i].name,  (int)standTbl[i].value, 0, 0, 0);
        taskDelay(10);
    }
#endif

    logMsg("statTblSize=%d\\n", statTblSize, 0, 0, 0, 0, 0);

    // for(i=0; i<statTblSize; i++)
    // {
    //     LOG_Dbg_Msg("%d name=%s address=%x\n", (int)i, (int)statTbl[i].name,  (int)statTbl[i].value, 0, 0, 0);
    //     taskDelay(10);
    // }
}

/* testing size of variables.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void TestSize(void)
{
    MEM_BLOCK_HDR Hdr;

    printf("Hdr = %d\n", (int)sizeof(Hdr));
}

#ifdef INCLUDE_E03_BA_RS485_TEST

/***********************************************************************
* E03_BA_RS485_TEST - 485 test
*
* RETURNS: 无
*
*/
void E03_BA_RS485_TEST (
    uint8_t ucComBoardType
)
{
    unsigned char testbuf1[]= {0x12,0x23,0x34,0x45,0x56,0x67,0x78,0x90,0x91,0x92,0x93};
    unsigned char testbuf2[11];
    int i;

    if(ucComBoardType == 0)		/* scc1 RS485_3 */
    {
        serial_Init(M68681_CHANNEL_D);
        serial_681_ioctl(M68681_CHANNEL_D,M681_IOCTL_SET_BAUD,9600);
        serial_681_ioctl(M68681_CHANNEL_D,M681_IOCTL_SET_PARITY,M681_IOCTL_PARITY_NONE);

        taskDelay(4);
        com_Send(M68681_CHANNEL_D, testbuf1, 11);
        taskDelay(4);

        FOREVER
        {
            if(com_Recevie(M68681_CHANNEL_D, testbuf2) == 11)
            {
                com_Send(M68681_CHANNEL_D, testbuf1, 11);
                taskDelay(4);
                com_Send(M68681_CHANNEL_D, testbuf2, 11);
                taskDelay(4);
            }

        }
    }		/* scc1 RS485_3 */

    if(ucComBoardType == 1)			/* smc1 RS485_1 */
    {
        serial_Init(M68681_CHANNEL_B);
        serial_681_ioctl(M68681_CHANNEL_B,M681_IOCTL_SET_BAUD,9600);
        serial_681_ioctl(M68681_CHANNEL_B,M681_IOCTL_SET_PARITY,M681_IOCTL_PARITY_NONE);

        taskDelay(4);
        com_Send(M68681_CHANNEL_B, testbuf1, 11);
        taskDelay(4);

        FOREVER
        {
            if(com_Recevie(M68681_CHANNEL_B, testbuf2) == 11)
            {
                com_Send(M68681_CHANNEL_B, testbuf2, 11);
                taskDelay(4);
                com_Send(M68681_CHANNEL_B, testbuf1, 11);
                taskDelay(4);
            }

        }
    }

    if(ucComBoardType == 2)	/* smc2 RS485_2 */
    {
        serial_Init(M68681_CHANNEL_A);
        serial_681_ioctl(M68681_CHANNEL_A,M681_IOCTL_SET_BAUD,9600);
        serial_681_ioctl(M68681_CHANNEL_A,M681_IOCTL_SET_PARITY,M681_IOCTL_PARITY_NONE);

        taskDelay(4);
        com_Send(M68681_CHANNEL_A, testbuf1, 11);
        taskDelay(4);

        FOREVER
        {
            if(com_Recevie(M68681_CHANNEL_A, testbuf2) == 11)
            {
                com_Send(M68681_CHANNEL_A, testbuf2, 11);
                taskDelay(4);
                com_Send(M68681_CHANNEL_A, testbuf1, 11);
                taskDelay(4);
            }

        }
    }


#if 0	/* smc1此时作为全双工的rs232. 测试附板E03-COM.H-A */
    serial_Init(M68681_CHANNEL_B);
    serial_681_ioctl(M68681_CHANNEL_B,M681_IOCTL_SET_BAUD,9600);
    serial_681_ioctl(M68681_CHANNEL_B,M681_IOCTL_SET_PARITY,M681_IOCTL_PARITY_NONE);

    taskDelay(4);
    com_Send(M68681_CHANNEL_B, testbuf1, 11);
    taskDelay(4);

    FOREVER
    {
        com_Send(M68681_CHANNEL_B, testbuf1, 11);;
        taskDelay(14);


        if(com_Recevie(M68681_CHANNEL_B, testbuf2) == 11)
        {
            for(i=0; i<11; i++)
            {
                printf("%2.2x", testbuf2[i]);

            }

            printf("\n");
            taskDelay(4);
        }
    }
#endif
}

/***********************************************************************
* E03_BA_RS485_TEST_Task - 485 test task
*
* RETURNS: 无
*
*/
void E03_BA_RS485_TEST_Task (
    uint8_t ucComBoardType	/* Com board type. */
)
{
    taskSpawn("E03BA_RS485TEST", 20, 0, 2000, (FUNCPTR)E03_BA_RS485_TEST, ucComBoardType, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

void BA485TEST(void)
{
    if(serial_Init(M68681_CHANNEL_D) == ERROR)
    {
        logMsg("第%d个485通讯口初始化失败!\n", M68681_CHANNEL_D, 0, 0, 0, 0, 0);
    }

    if(serial_Init(M68681_CHANNEL_B) == ERROR)
    {
        logMsg("第%d个485通讯口初始化失败!\n", M68681_CHANNEL_B, 0, 0, 0, 0, 0);
    }

    Init_Net();

    Init_Wdb();						/* 初始化wdb */
    Init_Telnet();		/* 初始化telnet */
    Init_Expand_Eth_Port();
}
#endif		/* INCLUDE_E03_BA_RS485_TEST */