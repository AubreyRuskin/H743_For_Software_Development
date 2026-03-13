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
/*      FPGA_Com.c                                EDPx-04-0.1           */
/*                                                                      */
/* COMPONENT                                                            */
/*                                                                      */
/*      PSB main unit, CPU communicate with FPGA                        */
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
/*      Chen, Xinzhi      2008/11/13                1.00                */
/*                                                                      */
/************************************************************************/

#include "vxWorks.h"			/* always first */

#include "m8260IntrCtl.h"
#include "intLib.h"
#include "taskLib.h"
#include "semLib.h"
#include "logmsg.h"
// #include "VTBOX_Drv.h"


// unsigned char intStatusBuf[FPGA_HDLC_CHAN_COUNT];
// unsigned char hdlcRecvBuf[FPGA_HDLC_CHAN_COUNT][FPGA_RECV_MAX_VALID_BUF_COUNT][FPGA_RECV_BUFFER_SIZE];
// unsigned char hdlcRecvFrameCount[FPGA_HDLC_CHAN_COUNT];

// unsigned int hdlcCmdOffsetAry[HDLC_SEND_CMD_COUNT] = {FPGA_UNICAST_CMD_REG, FPGA_BROADCAST_CMD_REG};

#ifdef INCLUDE_HDLC_TEST_FUN
static SEM_ID sem_RecvData;
#endif  /*INCLUDE_HDLC_TEST_FUN*/

void Fpga_Recv_ISR();

/*  Initailize communication with FPGA
    Return value: OK, ERROR
*/
int Fpga_Com_Init()
{
    int retVal = ERROR;

    // retVal = intConnect (INUM_TO_IVEC(INUM_IRQ6), Fpga_Recv_ISR, (int)NULL);
    // if(OK == retVal)
    // {
    //     retVal = intEnable(INUM_IRQ6);
    // }

    return retVal;
}

/*  FPGA receive ISR
*/
void Fpga_Recv_ISR()
{
//     int intKey;
//     int i;
//     int j;
//     int k;
//     unsigned char validBufIndex;
//     unsigned char readByteLoopCount;
//     unsigned char validFrameCount;
//     unsigned int *pIntBuf;
//     unsigned char *pCharBuf;
//     volatile unsigned int *pSrcBuf;
//     static uint32_t ulCnt = 0;

//     ulCnt++;

//     if (ulCnt % 0xFFFF == 1)
//     {
//         LOG_Dbg_Msg("Fpga_Recv_ISR Enter!\n", 0, 0, 0, 0, 0, 0);
//     }

//     intKey = intLock();

//     pIntBuf = (unsigned int*)intStatusBuf;

//     /*Read int status*/
//     pSrcBuf = (volatile unsigned int *)(FPGA_MEM_BASE_ADRS + FPGA_INT_STATUS_BEGIN);
//     for(i=0; i<(FPGA_HDLC_CHAN_COUNT/FPGA_BUS_WIDTH_IN_BYTE); i++)
//     {
//         *pIntBuf++ = *pSrcBuf++;
//     }

//     /*Read receive buffer data*/
//     pCharBuf = intStatusBuf;

//     for(i=0; i<FPGA_HDLC_CHAN_COUNT; i++)
//     {
//         hdlcRecvFrameCount[i] = 0;
//         if((*pCharBuf & FPGA_INT_STATUS_BUFFER_VALID) > 0)
//         {
//             validBufIndex = FPGA_INT_STATUS_BUF_1_VALID;
//             validFrameCount = 0;
//             for(j=0; j<FPGA_CHAN_BUFFER_COUNT; j++)
//             {
//                 if((*pCharBuf & validBufIndex) > 0)
//                 {
//                     pSrcBuf = (volatile unsigned int *)(FPGA_MEM_BASE_ADRS +
//                                                         FPGA_RECV_BUFFER_BEGIN
//                                                         + i*FPGA_RECV_BUFFER_SIZE*FPGA_CHAN_BUFFER_COUNT
//                                                         + j*FPGA_RECV_BUFFER_SIZE);
//                     pIntBuf = (unsigned int*)&hdlcRecvBuf[i][validFrameCount][0];

//                     *pIntBuf++ = *pSrcBuf++;
//                     *pIntBuf++ = *pSrcBuf++;
//                     readByteLoopCount = (hdlcRecvBuf[i][validFrameCount][FPGA_RECV_FRAME_LEN_INDEX] - 2 + 3) >> 2;
//                     for(k=0; k<readByteLoopCount; k++)
//                     {
//                         *pIntBuf++ = *pSrcBuf++;
//                     }
//                     validFrameCount++;
//                 }
//                 validBufIndex = validBufIndex << 1;
//                 if(validFrameCount >= FPGA_RECV_MAX_VALID_BUF_COUNT)
//                 {
//                     break;
//                 }
//             }
//             hdlcRecvFrameCount[i] = validFrameCount;
//         }
//         pCharBuf++;
//     }

//     intUnlock(intKey);
// #ifdef INCLUDE_HDLC_TEST_FUN
//     semGive(sem_RecvData);
// #endif  /*INCLUDE_HDLC_TEST_FUN*/
}

/*  Write unicast data to FPGA send buf
channel, hdlc channel index, begin with 0
sendBuf, pointer to send buffer
sendLen, send data length
return value: OK, ERROR
*/
int    Fpga_Write_Unicast_Data(unsigned char channel, unsigned char *sendBuf, unsigned char sendLen)
{
    // int i;
    int retVal = ERROR;
    // unsigned char writeByteLoopCount;
    // unsigned int *pIntBuf;
    // volatile unsigned int *pDstBuf;

    // if(channel < FPGA_HDLC_CHAN_COUNT)
    // {
    //     writeByteLoopCount = (sendLen + 3 + 1) >> 2;
    //     pIntBuf = (unsigned int*)sendBuf;
    //     pDstBuf = (volatile unsigned int*)(FPGA_MEM_BASE_ADRS
    //                                        + FPGA_UNICAST_BUFFER_BEGIN
    //                                        + channel * FPGA_SEND_BUFFER_SIZE);
    //     for(i=0; i<writeByteLoopCount; i++)
    //     {
    //         *pDstBuf++ = *pIntBuf++;
    //     }
    //     retVal = OK;
    // }

    return retVal;
}

/*  Write broadcast data to FPGA send buf
channel, not use
sendBuf, pointer to send buffer
sendLen, send data length
return value: OK, ERROR
*/
int    Fpga_Write_Broadcast_Data(unsigned char channel, unsigned char *sendBuf, unsigned char sendLen)
{
    // int i;
    // unsigned char writeByteLoopCount;
    // unsigned int *pIntBuf;
    // volatile unsigned int *pDstBuf;

    // writeByteLoopCount = (sendLen + 3 + 1) >> 2;
    // pIntBuf = (unsigned int*)sendBuf;
    // pDstBuf = (volatile unsigned int*)(FPGA_MEM_BASE_ADRS
    //                                    + FPGA_BROADCAST_BUFFER_BEGIN);
    // for(i=0; i<writeByteLoopCount; i++)
    // {
    //     *pDstBuf++ = *pIntBuf++;
    // }

    return OK;
}

/*  Write send command
cmdType, HDLC_UNICAST_CMD or FPGA_BROADCAST_CMD_REG
cmd, FPGA_HDLC_CHAN_INDEX_31..0, or combination of FPGA_HDLC_CHAN_INDEX_31..0
    e.g. FPGA_HDLC_CHAN_INDEX_5 | FPGA_HDLC_CHAN_INDEX_10
return value: OK, ERROR
*/
int    Fpga_Write_Send_Cmd(unsigned int comType, unsigned int cmd)
{
    int retVal = ERROR;

    // if(comType < HDLC_SEND_CMD_COUNT)
    // {
    //     *(volatile unsigned int*)(FPGA_MEM_BASE_ADRS + hdlcCmdOffsetAry[comType]) = cmd;
    //     retVal = OK;
    // }

    return retVal;
}

#ifdef INCLUDE_HDLC_TEST_FUN
unsigned char testHdlcBuf[FPGA_SEND_BUFFER_SIZE];
int Test_Send_Hdlc_Unicast_Data(unsigned char channel, unsigned char dataLen)
{
    int i;
    int retVal = ERROR;
    int semGetStatus;

    semGetStatus = semTake(sem_RecvData, WAIT_FOREVER);

    if((OK == semGetStatus)
            && (dataLen < FPGA_SEND_BUFFER_SIZE)
            && (channel < FPGA_HDLC_CHAN_COUNT))
    {
        testHdlcBuf[0] = dataLen;
        for(i=1; i<= dataLen; i++)
        {
            testHdlcBuf[i] = i;
        }
        Fpga_Write_Unicast_Data(channel, testHdlcBuf, testHdlcBuf[0]);
        Fpga_Write_Send_Cmd(HDLC_UNICAST_CMD, 1<<channel);
        retVal = OK;
    }

    return retVal;
}

int Test_Send_Broadcast_Hdlc_Data(unsigned char channel, unsigned char dataLen)
{
    int i;
    int retVal = ERROR;
    int semGetStatus;

    semGetStatus = semTake(sem_RecvData, WAIT_FOREVER);

    if((dataLen < FPGA_SEND_BUFFER_SIZE) && (OK == semGetStatus))
    {
        testHdlcBuf[0] = dataLen;
        for(i=1; i<= dataLen; i++)
        {
            testHdlcBuf[i] = i;
        }
        Fpga_Write_Broadcast_Data(channel, testHdlcBuf, testHdlcBuf[0]);
        Fpga_Write_Send_Cmd(HDLC_BROADCAST_CMD, channel);
        retVal = OK;
    }

    return retVal;
}

void    Set_Test_Send_Back_Data()
{
    int i;
    unsigned int sendCmd;

    while(1)
    {
        semTake(sem_RecvData, WAIT_FOREVER);
        sendCmd = 0;
        for(i=0; i<FPGA_HDLC_CHAN_COUNT; i++)
        {
            /*Has received data frame*/
            if(hdlcRecvFrameCount[i]>0)
            {
                /*for test, only send back one datagram*/
                Fpga_Write_Unicast_Data(i,
                                        &hdlcRecvBuf[i][0][FPGA_RECV_FRAME_LEN_INDEX],
                                        hdlcRecvBuf[i][0][FPGA_RECV_FRAME_LEN_INDEX]);
                sendCmd |= (1<<i);
            }
        }
        Fpga_Write_Send_Cmd(HDLC_UNICAST_CMD, sendCmd);
    }
}

void    Begin_Test_Send_Back_Data()
{
    sem_RecvData = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
    taskSpawn(
        "tSendBack",					/*Task Name*/
        10,                			    /*Task Priority*/
        /*VX_FP_TASK,*/0,				/*Task Option*/
        0x1000,		                    /*Task Stack Size*/
        (FUNCPTR)Set_Test_Send_Back_Data,	/*Task Function*/
        0,0,0,0,0,0,0,0,0,0				/*Parameter*/
    );
}
#endif  /*INCLUDE_HDLC_TEST_FUN*/

void Start_PSB_Main(void)
{
    // Init_Net();
    // Init_Telnet();
    // Begin_Test_Send_Back_Data();
    // Write_FPGA_HDLC_Program();
    Fpga_Com_Init();
}
