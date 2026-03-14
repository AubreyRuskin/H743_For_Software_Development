/************************************************************************/
/*                                                                      */
/*      Copyright (c) 2006 SNAC(Guodian Nanjing Automation Co., Ltd.)   */
/*      All Rights Reserved.                                            */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* FILE NAME                                            VERSION         */
/*                                                                      */
/*      OptChanTest.c                             EDPx-04-0.1           */
/*                                                                      */
/* COMPONENT                                                            */
/*                                                                      */
/*      Optical Channel Simulating tester                               */
/*                                                                      */
/* DESCRIPTION                                                          */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/* AUTHOR                                                               */
/*                                                                      */
/*      Chen, Xinzhi, SNAC                                              */
/*                                                                      */
/* DATA STRUCTURES                                                      */
/*                                                                      */
/*      None                                                            */
/*                                                                      */
/* FUNCTIONS                                                            */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/* DEPENDENCIES                                                         */
/*                                                                      */
/*      None                                                            */
/*                                                                      */
/* HISTORY                                                              */
/*                                                                      */
/*         NAME            DATE                    REMARKS              */
/*                                                                      */
/*      Chen, Xinzhi      2006/11/14                1.00                */
/*                                                                      */
/************************************************************************/

#include "vxWorks.h"			/* always first */
#include "vxworks_type.h"

typedef volatile UINT8 VUINT8;
typedef volatile UINT16 VUINT16;
typedef volatile INT32 VINT32;

#include "logLib.h"
#include "taskLib.h"
#include "semLib.h"
// #include "risctimer.h"
#include "string_compat.h"
#include "bspinterface.h"

#include "scc_hdlc_raw.h"

// #include "VTBOX_Drv.h"

#define HDLC_TEST_BUFFER_LEN 256
#define HDLC_TEST_SEND_COUNT 50

SEM_ID  semHdlcwait;
int hdlcTestSendCount = 0;
int hdlcTestRecvCount = 0;
int hdlcTestRecvErr = 0;
BOOL    enableTxRxInfo = FALSE;
BOOL    enableClearCount = FALSE;
BOOL    enableChangeTxLen = FALSE;
unsigned char hdlcTestBuf[HDLC_TEST_BUFFER_LEN];
int hdlcSendLen = HDLC_TEST_SEND_COUNT;
int resvereSendLen = 0;
int lenErrCount = 0;
int dataErrCount = 0;
unsigned char hdlcReserveData[HDLC_TEST_BUFFER_LEN];

#undef  INCLUDE_LOG_ERR

#ifdef  INCLUDE_LOG_ERR
extern void	vxTimeBaseGet (UINT32 * pTbu, UINT32 * pTbl);
#endif  /*INCLUDE_LOG_ERR*/

int Opt_Init()
{
    BOOL    hasInit = FALSE;

    if(hasInit)
    {
        return OK;
    }

    // if(OK != m8260SccHdlcInit(CHAN_SCC_DOWN))
    // {
    //     return ERROR;
    // }

    hasInit = TRUE;

    return OK;
}

void    wait_hdlc_Operation()
{
    // startRiscTimer(HDLC_WAIT_TIMER);
    semTake(semHdlcwait, WAIT_FOREVER);
}

void    hdlc_Wait_Timeout(int arg)
{
    semGive(semHdlcwait);
}

void    regHdlcTimer()
{
    // regRiscTimer(HDLC_WAIT_TIMER,&hdlc_Wait_Timeout,
    //              0, HDLC_WAIT_PER_TICK);
    // rtmEnable(HDLC_WAIT_TIMER);
}

void    hdlc_tx_rx()
{
    int i;
    int recvLen;
    unsigned char *pData;
    UINT32 timeStartH, timeStartL;
    UINT32 timeH, timeL;
    int maxDelay = 0;

    semHdlcwait = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
    regHdlcTimer();

    for(i=0; i<HDLC_TEST_BUFFER_LEN; i++)
    {
        hdlcTestBuf[i] = i;
    }

    while(1)
    {
        vxTimeBaseGet(&timeStartH, &timeStartL);
        hdlcTestBuf[0] = hdlcSendLen-1;
        hdlc_send_raw(CHAN_DOWN, hdlcTestBuf, hdlcSendLen);
        hdlcTestSendCount++;
        /*wait_hdlc_Operation();*/
        recvLen = hdlc_recv_raw(CHAN_DOWN, &pData, &timeH, &timeL, 2);
        if(recvLen > 0)
        {
            hdlcTestRecvCount++;
            if((hdlcSendLen != recvLen)
                    ||(0 != memcmp(pData, hdlcTestBuf, hdlcSendLen)))
            {
                if(hdlcSendLen != recvLen)
                {
#ifdef  INCLUDE_LOG_ERR
                    logMsg("Recv len err, len = %d, Th is %d, Tl is %d \n",
                           recvLen, timeH,timeL,0,0,0);
#endif  /*INCLUDE_LOG_ERR*/
                    lenErrCount++;
                }
                else if (0 != memcmp(pData, hdlcTestBuf, hdlcSendLen))
                {
                    dataErrCount++;
                }
                if(recvLen <= HDLC_TEST_BUFFER_LEN)
                {
                    memcpy(hdlcReserveData, pData, recvLen);
                }
                else
                {
                    hdlcReserveData[0] = recvLen;
                }
                hdlcTestRecvErr++;
            }
        }
        if((int)(timeL-timeStartL) > maxDelay)
        {
            maxDelay = timeL-timeStartL;
        }
        if(enableTxRxInfo)
        {
            logMsg("Hdlc Tx %d, Rx %d, Err %d, Recv time delay %d\n",
                   hdlcTestSendCount, hdlcTestRecvCount, hdlcTestRecvErr, maxDelay*4*100,0,0);
            logMsg("Hdlc len err %d, data err %d\n",
                   lenErrCount, dataErrCount,0,0,0,0);

            enableTxRxInfo = FALSE;
        }
        if(enableClearCount)
        {
            hdlcTestSendCount = 0;
            hdlcTestRecvCount = 0;
            hdlcTestRecvErr = 0;
            lenErrCount = 0;
            dataErrCount = 0;
            enableClearCount = FALSE;
        }
        if(enableChangeTxLen)
        {
            hdlcSendLen = resvereSendLen;
            enableChangeTxLen = FALSE;
        }
    }
}

int Set_Hdlc_Send_Count(int sendLen)
{
    if(sendLen > HDLC_TEST_BUFFER_LEN)
    {
        return ERROR;
    }

    resvereSendLen = sendLen;
    enableChangeTxLen = TRUE;

    return hdlcSendLen;
}

void    Clear_Hdlc_Counter()
{
    enableClearCount = TRUE;
}

void    Enable_Hdlc_Info()
{
    enableTxRxInfo = TRUE;
}

void    Begin_Hdlc_Tx_Rx_Test()
{
    Opt_Init();
    taskSpawn(
        "tHdlcTxRx",					/*Task Name*/
        100,                			/*Task Priority*/
        0,								/*Task Option*/
        0x1000,		                    /*Task Stack Size*/
        (FUNCPTR)hdlc_tx_rx,				/*Task Function*/
        0,0,0,0,0,0,0,0,0,0				/*Parameter*/
    );
}

void Start_PSB_BU(void)
{
    Init_Net();
    Init_Telnet();
    Begin_Hdlc_Tx_Rx_Test();
}
