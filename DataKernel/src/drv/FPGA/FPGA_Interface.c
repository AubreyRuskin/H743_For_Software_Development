
#include "string_compat.h"
#include <semLib.h>
#include <stdio_compat.h>
#include <intLib.h>
#include "riscTimer.h"
#include "FPGA_Interface.h"
//#include "GooseInterface.h"
#include "miscfunc.h"
#include "eth_callback.h"
#include "logmsg.h"
#include "edpbase.h"
#include "EdpVer.h"
#include "intLib.h"

BOOL bFPGAGoRcvWithCopy=TRUE;

BOOL bUseFPGAnsTime=FALSE;

BOOL bUseHSBStyleRxInterface=FALSE;

static int MaxReceivePacketsOneTimeHSBStyle=24;

#define RISCTIMERNUMFORGOOSERESEND 14		/* GOOSE重发使用的Risc Timer */

#define MAX_COM_ERR_LOG_TIMES 5

#define MAX_RECEIVE_PACKETS_ONE_TIME 6
#define MAX_SEND_CHECK_TBD_TIMES 4

#define GMRP_SEND_INTERVAL 2000

static uint8_t MaxRBD[MAX_FPGA_TO_CPU_PORT_NUM];
static uint8_t MaxTBD[MAX_FPGA_TO_CPU_PORT_NUM];
static uint8_t RxCnt[MAX_FPGA_TO_CPU_PORT_NUM];
static uint8_t TxCnt[MAX_FPGA_TO_CPU_PORT_NUM];

static FPGA_GOOSE_ADDR FPGAGooseRegAddr[MAX_FPGA_TO_CPU_PORT_NUM];
static FPGA_HSB_STYLE_GOOSE_RX_ADDR FPGAHSBStyleGoRxAddr[MAX_FPGA_TO_CPU_PORT_NUM];
static FPGA_TRANS_AD_MAP_ADDR FPGATransAdRegAddr[FPGA_TRANSMITTER_MAX_MULTI_ADDR_NUM];
static FPGA_TRANS_PORT_SRC_AD_ADDR FPGATransSrcAdRegAddr[MAX_FPGA_TRANSMITTER_GOOSE_PORT_NUM];
static FPGA_ADD_ON_INFO_ADDR FPGAAddOnInfoAddr;
static FPGA_GMRP_REG_ADDR FPGAGmrpRegAddr;
static FPGA_GMRP_SEND_BUFFER_ADDR FPGAGmrpSendBufAddr[MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM];

static SEM_ID  GOOSE_RESEND_SEM_ID;

static SEM_ID RxSem[MAX_FPGA_TO_CPU_PORT_NUM];
static SEM_ID TxSem[MAX_FPGA_TO_CPU_PORT_NUM];

extern uint32_t Get_FPGA_Mem_Val(uint32_t *addr);

extern void Set_FPGA_Mem_Val();

extern uint16_t fpgaGetProtocolVer();

extern uint32_t fpgaGetFPGAnsTime();

#define LONG_INT_BYTES 4

eth_cb_func FPGA_Goose_Recv_Callback_Func[MAX_FPGA_TO_CPU_PORT_NUM];

/* CRC constants.  Note: CRC-32 polynomial is bit-reversed. */

/* crc32() Takes the array of bytes, macaddr[], representing an
   Ethernet MAC address and returns the CRC-32 result over these bytes,
   where each byte is used in bit-reversed form (Ethernet bit order).
   Index 0 of macaddr[] is the first byte of the address on the wire.
   Test case: the result of crc32 on {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}
   should be 0xad0c28f3.
 */
uint32_t MAC_Addr_Crc32(char macaddr[MAC_ADDRLEN])
{
    uint32_t crc, result;
    int byte, i;
    /* CRC-32 algorithm starts by inverting first 4 bytes */
    crc = CRC_INITIAL;
    /* add each byte to running CRC accumulator */
    for (byte = 0; byte < MAC_ADDRLEN; ++byte)
    {
        crc ^= macaddr[byte];
        /* shift CRC right to perform but reversal on byte of address */
        for (i = 0; i < BITS_PER_BYTE; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ CRC_POLYNOMIAL;
            else
                crc >>= 1;
        }
    }
    /* finally, reverse bits of result to get CRC in normal bit order */
    for (result = 0, i = 4*BITS_PER_BYTE-1; i >= 0; crc >>= 1, --i)
        result |= (crc & 1) << i;
    return result;
}

void Goose_Resend_RiscTimer_Callback()
{
    // rtmDisable(RISCTIMERNUMFORGOOSERESEND);
    semGive(GOOSE_RESEND_SEM_ID);
}


static uint32_t fpgaMemGet(uint32_t* addr)
{
    return (*((volatile uint32_t *) addr));
    //return Get_FPGA_Mem_Val(addr);
}

static void fpgaMemSet(uint32_t* addr, uint32_t val)
{
    *((volatile uint32_t *)addr) = val;
    //Set_FPGA_Mem_Val(addr,val);
}

BOOL bCheckBroadcastAddr(char *pAddr)
{
    char BroadcastAddr[6]= {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

    if(pAddr==NULL)
        return FALSE;

    if(strncmp(pAddr,BroadcastAddr,6)==0)
        return TRUE;
    else
        return FALSE;
}

BOOL bCheckMulticastAddr(char *pAddr)
{
    if(pAddr==NULL)
        return FALSE;

    if(pAddr[0]&0x01)
        return TRUE;
    else
        return FALSE;
}

BOOL Init_BD_Num(uint8_t portNumtoCPU)
{
    uint32_t *pBDNum=NULL;
    uint32_t BDNum=0;

    //if (appType_g != APP_TYPE_DIG)
    //    return FALSE;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pBDNum = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pBDNumAddr;
    }
    else
        return FALSE;

    BDNum=fpgaMemGet(pBDNum);

    MaxRBD[portNumtoCPU-NET_FPGA_TO_CPU_A]=(uint8_t)(BDNum&0xFF);

    MaxTBD[portNumtoCPU-NET_FPGA_TO_CPU_A]=(uint8_t)((BDNum&0xFF00)>>8);

    if((MaxRBD[portNumtoCPU-NET_FPGA_TO_CPU_A]>0)&&(MaxTBD[portNumtoCPU-NET_FPGA_TO_CPU_A]>0))
        return TRUE;
    else
        return FALSE;
}

STATUS Set_Promiscuous_Receive_Enable(uint8_t portNumtoCPU)
{
    uint32_t *pModeR=NULL;
    uint32_t ModeR=0;

    //if (appType_g != APP_TYPE_DIG)
    //    return ERROR;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pModeR = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pModerAddr;
    }
    else
        return ERROR;


    ModeR=fpgaMemGet(pModeR);

    ModeR|=0x04;

    fpgaMemSet(pModeR,ModeR);

    return OK;
}

STATUS Set_Promiscuous_Receive_Disable(uint8_t portNumtoCPU)
{
    uint32_t *pModeR=NULL;
    uint32_t ModeR=0;

    //if (appType_g != APP_TYPE_DIG)
    //    return ERROR;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pModeR = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pModerAddr;
    }
    else
        return ERROR;

    ModeR=fpgaMemGet(pModeR);

    ModeR&=0xFFFFFFFB;

    fpgaMemSet(pModeR,ModeR);

    return OK;
}

STATUS Set_Broadcast_Receive_Enable(uint8_t portNumtoCPU)
{
    uint32_t *pModeR=NULL;
    uint32_t ModeR=0;

    //if (appType_g != APP_TYPE_DIG)
    //   return ERROR;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pModeR = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pModerAddr;
    }
    else
        return ERROR;

    ModeR=fpgaMemGet(pModeR);

    ModeR|=0x02;

    fpgaMemSet(pModeR,ModeR);

    return OK;
}

STATUS Set_Broadcast_Receive_Disable(uint8_t portNumtoCPU)
{
    uint32_t *pModeR=NULL;
    uint32_t ModeR=0;

    //if (appType_g != APP_TYPE_DIG)
    //    return ERROR;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pModeR = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pModerAddr;
    }
    else
        return ERROR;


    ModeR=fpgaMemGet(pModeR);

    ModeR&=0xFFFFFFFD;

    fpgaMemSet(pModeR,ModeR);

    return OK;
}

STATUS Set_Multicast_Receive_Enable(uint8_t portNumtoCPU)
{
    uint32_t *pModeR=NULL;
    uint32_t ModeR=0;

    //if (appType_g != APP_TYPE_DIG)
    //    return ERROR;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pModeR = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pModerAddr;
    }
    else
        return ERROR;

    ModeR=fpgaMemGet(pModeR);

    ModeR|=0x01;

    fpgaMemSet(pModeR,ModeR);

    return OK;
}

STATUS Set_Multicast_Receive_Disable(uint8_t portNumtoCPU)
{
    uint32_t *pModeR=NULL;
    uint32_t ModeR=0;

    //if (appType_g != APP_TYPE_DIG)
    //    return ERROR;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pModeR = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pModerAddr;
    }
    else
        return ERROR;

    ModeR=fpgaMemGet(pModeR);

    ModeR&=0xFFFFFFFE;

    fpgaMemSet(pModeR,ModeR);

    return OK;
}

STATUS Set_Interrupt_Enable(uint8_t portNumtoCPU)
{
    uint32_t *pModeR=NULL;
    uint32_t ModeR=0;

    //if (appType_g != APP_TYPE_DIG)
    //    return ERROR;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pModeR = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pModerAddr;
    }
    else
        return ERROR;

    ModeR=fpgaMemGet(pModeR);

    ModeR|=0x08;

    fpgaMemSet(pModeR,ModeR);

    return OK;
}

STATUS Set_Interrupt_Disable(uint8_t portNumtoCPU)
{
    uint32_t *pModeR=NULL;
    uint32_t ModeR=0;

    //if (appType_g != APP_TYPE_DIG)
    //    return ERROR;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pModeR = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pModerAddr;
    }
    else
        return ERROR;

    ModeR=fpgaMemGet(pModeR);

    ModeR&=0xFFFFFFF7;

    fpgaMemSet(pModeR,ModeR);

    return OK;
}

int Receive_Goose_from_HSB_Style_Interface(uint8_t portNumtoCPU)
{
    uint16_t i=0;
    uint16_t j=0;
    uint32_t *pExchangeReg=NULL;
    uint32_t ExchangeReg=0;
    uint32_t *pRxBufAddr=NULL;
    int ReceivedThisTime=0;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pExchangeReg = FPGAHSBStyleGoRxAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pExchangeReg;
        pRxBufAddr = FPGAHSBStyleGoRxAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pRxBufAddr;
    }
    else
        return -1;

    for(i=0; i<MaxReceivePacketsOneTimeHSBStyle; i++)
    {
        ExchangeReg=fpgaMemGet(pExchangeReg);

        if((ExchangeReg&0x20000000)
                &&((ExchangeReg&0x40000000)==0))
        {
            uint32_t *pDst=NULL;
            uint32_t *pSrc=NULL;
            uint16_t CopyTimes=0;
            uint8_t RcvBufferTmp[MAX_RX_MSG_LEN+2];
            uint32_t basens=0;
            uint32_t packns=0;
            uint32_t intervalus=0;
            US_CNT_UTC_TIME uscntutc;
            uint32_t sysuscnt=0;
            MMS_UTC_TIME mmsutc;
            int iLockKey;

            pDst=(uint32_t *)RcvBufferTmp;
            pSrc=pRxBufAddr;

            CopyTimes=(ExchangeReg&0x7FF)/LONG_INT_BYTES;
            if((ExchangeReg&0x7FF)%LONG_INT_BYTES)
                CopyTimes++;

            for(j=0; j<CopyTimes; j++)
            {
                pDst[j]=pSrc[j];
            }

            if(bUseFPGAnsTime)
            {
                /*packns=(uint32_t)(*(&(RcvBufferTmp[6])));*/
                packns=U8_TO_U32(RcvBufferTmp[6],RcvBufferTmp[7],RcvBufferTmp[8],RcvBufferTmp[9]);

                iLockKey=intLock();
                basens=fpgaGetFPGAnsTime();

                if(basens>=packns)
                    intervalus=basens-packns;
                else
                    intervalus=basens+1000000000-packns;

                intervalus=intervalus/1000;

                TM_Get_Sys_Us_UTC_Time(&uscntutc,&sysuscnt);
                uscntutc.ullusCntFrom1970-=intervalus;
                Us_UTC_Time_To_MMS_UTC_Time(&uscntutc,&mmsutc);
                intUnlock(iLockKey);

                RcvBufferTmp[6]=0;
                RcvBufferTmp[7]=0;
                RcvBufferTmp[8]=0;
                RcvBufferTmp[9]=0;

                /*printf("Get Goose Packet Time %llu delay=%lu\n",uscntutc.ullusCntFrom1970,intervalus);*/

                (*(FPGA_Goose_Recv_Callback_Func[portNumtoCPU-NET_FPGA_TO_CPU_A]))
                (portNumtoCPU,
                 RcvBufferTmp,
                 (int)(ExchangeReg&0x7FF),
                 (int)TRUE,
                 (int)(&mmsutc));
            }
            else
            {
                (*(FPGA_Goose_Recv_Callback_Func[portNumtoCPU-NET_FPGA_TO_CPU_A]))
                (portNumtoCPU,
                 RcvBufferTmp,
                 (int)(ExchangeReg&0x7FF),
                 (int)FALSE,
                 (int)NULL);
            }

            fpgaMemSet(pExchangeReg,0x40000000);
            ReceivedThisTime++;
        }
        else
        {
            break;
        }
    }

    return ReceivedThisTime;
}

int Receive_Goose_from_Mac_Ctrler(uint8_t portNumtoCPU)
{
    uint32_t *pBaseRxBD=NULL;
    uint32_t RxBD=0;
    uint32_t *pStatus=NULL;
    uint32_t Status=0;
    uint32_t *pGooseBase=NULL;

    uint16_t i=0;
    uint16_t j=0;

    //if (appType_g != APP_TYPE_DIG)
    //    return -1;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pGooseBase = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pBaseAddr;
        pBaseRxBD = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pRxBDAddr;
        pStatus = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pStatusAddr;
    }
    else
        return -1;

    if(MaxRBD[portNumtoCPU-NET_FPGA_TO_CPU_A]==0)
    {
        return -1;
    }

    if(FPGA_Goose_Recv_Callback_Func[portNumtoCPU-NET_FPGA_TO_CPU_A]==NULL)
        return 0;

    if(semTake(RxSem[portNumtoCPU-NET_FPGA_TO_CPU_A],NO_WAIT)!=OK)
        return -1;

    Status=fpgaMemGet(pStatus);

    //对错误标志标志进行处理
    if(Status&0xF8)
    {
        if(Status&0x80)
        {
            char strtmp[128];
            static uint8_t logcnt[MAX_FPGA_TO_CPU_PORT_NUM]= {0};
            if(logcnt[portNumtoCPU-NET_FPGA_TO_CPU_A]<MAX_COM_ERR_LOG_TIMES)
            {
                sprintf(strtmp,"FPGA接收goose帧错误,Receive Buffer Overflow\n");
                LOG_Write(LOG_KERNEL, strtmp, NULL);
                logcnt[portNumtoCPU-NET_FPGA_TO_CPU_A]++;
            }
        }
        if(Status&0x40)
        {
            char strtmp[128];
            static uint8_t logcnt[MAX_FPGA_TO_CPU_PORT_NUM]= {0};
            if(logcnt[portNumtoCPU-NET_FPGA_TO_CPU_A]<MAX_COM_ERR_LOG_TIMES)
            {
                sprintf(strtmp,"FPGA接收goose帧错误,PHY Receive Error\n");
                LOG_Write(LOG_KERNEL, strtmp, NULL);
                logcnt[portNumtoCPU-NET_FPGA_TO_CPU_A]++;
            }
        }
        if(Status&0x20)
        {
            char strtmp[128];
            static uint8_t logcnt[MAX_FPGA_TO_CPU_PORT_NUM]= {0};
            if(logcnt[portNumtoCPU-NET_FPGA_TO_CPU_A]<MAX_COM_ERR_LOG_TIMES)
            {
                sprintf(strtmp,"FPGA接收goose帧错误,Receive Non-Octet Error\n");
                LOG_Write(LOG_KERNEL, strtmp, NULL);
                logcnt[portNumtoCPU-NET_FPGA_TO_CPU_A]++;
            }
        }
        if(Status&0x10)
        {
            char strtmp[128];
            static uint8_t logcnt[MAX_FPGA_TO_CPU_PORT_NUM]= {0};
            if(logcnt[portNumtoCPU-NET_FPGA_TO_CPU_A]<MAX_COM_ERR_LOG_TIMES)
            {
                sprintf(strtmp,"FPGA接收goose帧错误,Receive Length Error");
                LOG_Write(LOG_KERNEL, strtmp, NULL);
                logcnt[portNumtoCPU-NET_FPGA_TO_CPU_A]++;
            }
        }
        if(Status&0x08)
        {
            char strtmp[128];
            static uint8_t logcnt[MAX_FPGA_TO_CPU_PORT_NUM]= {0};
            if(logcnt[portNumtoCPU-NET_FPGA_TO_CPU_A]<MAX_COM_ERR_LOG_TIMES)
            {
                sprintf(strtmp,"FPGA接收goose帧错误,Receive CRC Error\n");
                LOG_Write(LOG_KERNEL, strtmp, NULL);
                logcnt[portNumtoCPU-NET_FPGA_TO_CPU_A]++;
            }
        }

        //清标志
        fpgaMemSet(pStatus,0xF8);
    }

    //if((Status&0x04)!=0x04)//收到数据标志
    //{
    //    return 0;//未收到数据
    //}

    if(bUseHSBStyleRxInterface)
    {
        Receive_Goose_from_HSB_Style_Interface(portNumtoCPU);
    }
    else
    {
        for(i=0; i<MAX_RECEIVE_PACKETS_ONE_TIME; i++)
        {
            RxBD=fpgaMemGet(pBaseRxBD+RxCnt[portNumtoCPU-NET_FPGA_TO_CPU_A]);

            if(RxBD&0x80000000)
            {
                if(bFPGAGoRcvWithCopy)
                {
                    uint32_t *pDst=NULL;
                    uint32_t *pSrc=NULL;
                    uint16_t CopyTimes=0;
                    uint8_t RcvBufferTmp[MAX_RX_MSG_LEN+2];
                    uint32_t basens=0;
                    uint32_t packns=0;
                    uint32_t intervalus=0;
                    US_CNT_UTC_TIME uscntutc;
                    uint32_t sysuscnt=0;
                    MMS_UTC_TIME mmsutc;
                    int iLockKey;


                    pDst=(uint32_t *)RcvBufferTmp;
                    pSrc=(uint32_t *)(pGooseBase+(RxBD&0xFFFF));

                    CopyTimes=((RxBD&0x7FF0000)>>16)/LONG_INT_BYTES;
                    if(((RxBD&0x7FF0000)>>16)%LONG_INT_BYTES)
                        CopyTimes++;

                    for(j=0; j<CopyTimes; j++)
                    {
                        pDst[j]=pSrc[j];
                    }

                    if(bUseFPGAnsTime)
                    {
                        /*packns=(uint32_t)(*(&(RcvBufferTmp[6])));*/
                        packns=U8_TO_U32(RcvBufferTmp[6],RcvBufferTmp[7],RcvBufferTmp[8],RcvBufferTmp[9]);

                        iLockKey=intLock();
                        basens=fpgaGetFPGAnsTime();

                        if(basens>=packns)
                            intervalus=basens-packns;
                        else
                            intervalus=basens+1000000000-packns;

                        intervalus=intervalus/1000;

                        TM_Get_Sys_Us_UTC_Time(&uscntutc,&sysuscnt);
                        uscntutc.ullusCntFrom1970-=intervalus;
                        Us_UTC_Time_To_MMS_UTC_Time(&uscntutc,&mmsutc);
                        intUnlock(iLockKey);

                        RcvBufferTmp[6]=0;
                        RcvBufferTmp[7]=0;
                        RcvBufferTmp[8]=0;
                        RcvBufferTmp[9]=0;

                        /*printf("Get Goose Packet Time %llu delay=%lu\n",uscntutc.ullusCntFrom1970,intervalus);*/

                        (*(FPGA_Goose_Recv_Callback_Func[portNumtoCPU-NET_FPGA_TO_CPU_A]))
                        (portNumtoCPU,
                         RcvBufferTmp,
                         (int)((RxBD&0x7FF0000)>>16),
                         (int)TRUE,
                         (int)(&mmsutc));
                    }
                    else
                    {
                        (*(FPGA_Goose_Recv_Callback_Func[portNumtoCPU-NET_FPGA_TO_CPU_A]))
                        (portNumtoCPU,
                         RcvBufferTmp,
                         (int)((RxBD&0x7FF0000)>>16),
                         (int)FALSE,
                         (int)NULL);
                    }
                }
                else
                {
                    //logMsg("RxCnt=%u\n",RxCnt[portNum-FPGA_GOOSE_NET_A] , 0, 0, 0, 0, 0);
                    (*(FPGA_Goose_Recv_Callback_Func[portNumtoCPU-NET_FPGA_TO_CPU_A]))(portNumtoCPU,(uint8_t *)(pGooseBase+(RxBD&0xFFFF)),(int)((RxBD&0x7FF0000)>>16),(int)FALSE,(int)NULL);
                }
                //清标志
                fpgaMemSet(pBaseRxBD+RxCnt[portNumtoCPU-NET_FPGA_TO_CPU_A],RxBD);

            }
            else
                break;

            RxCnt[portNumtoCPU-NET_FPGA_TO_CPU_A]=(RxCnt[portNumtoCPU-NET_FPGA_TO_CPU_A]+1)%MaxRBD[portNumtoCPU-NET_FPGA_TO_CPU_A];
        }

#if 0
        //清接收标志
        if((i==MAX_RECEIVE_PACKETS_ONE_TIME)&&(fpgaMemGet(pBaseRxBD+(RxCnt[portNum-FPGA_GOOSE_NET_A]+1)%MaxTBD[portNum-FPGA_GOOSE_NET_A])&0x80000000))
        {
            //不清
            semGive(RxSem[portNum-FPGA_GOOSE_NET_A]);
            return i;
        }
        else
        {
            //清
            fpgaMemSet(pStatus,0x04);
        }
#endif
    }

    semGive(RxSem[portNumtoCPU-NET_FPGA_TO_CPU_A]);
    return i;
}

int Receive_Goose_from_FPGA(uint8_t portNum)
{
    if((portNum>=FPGA_GOOSE_NET_A)&&(portNum<(FPGA_GOOSE_NET_A+MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM)))
    {
        portNum=NET_FPGA_TO_CPU_A;
    }
    else
    {
        return -1;
    }

    return Receive_Goose_from_Mac_Ctrler(NET_FPGA_TO_CPU_A);
}

int Send_Goose_to_Mac_Ctrler(uint8_t portNumtoCPU, uint8_t *sendBuf, int sendNum)
{
    /*该函数增加了重发机制,但只有一个定时器,所以不可重入,须注意/2012/4/23/xdf*/
    uint32_t *pBaseTxBD=NULL;
    uint32_t TxBD=0;
    uint32_t *pStatus=NULL;
    uint32_t Status=0;
    uint32_t *pGooseBase=NULL;

    uint16_t i,j,k;
    uint32_t *pTmpSrc32=NULL;
    uint32_t *pTmpDst32=NULL;
    int m,n;
    BOOL bResendFlag=FALSE;

    //if (appType_g != APP_TYPE_DIG)
    //    return -1;

    if(sendBuf==NULL)
    {
        return -1;
    }

    if((sendNum<=0)||(sendNum>MAX_RX_MSG_LEN))
    {
        return -1;
    }

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pGooseBase = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pBaseAddr;
        pBaseTxBD = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pTxBDAddr;
        pStatus = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pStatusAddr;
    }
    else
        return -1;


    if(MaxTBD[portNumtoCPU-NET_FPGA_TO_CPU_A]==0)
    {
        return -1;
    }

resend:
    if(semTake(TxSem[portNumtoCPU-NET_FPGA_TO_CPU_A],NO_WAIT)!=OK)
        return -1;

    //处理发送状态位
    Status=fpgaMemGet(pStatus);
    if(Status&0x02)//发送异常
    {
        char strtmp[128];
        static uint8_t logcnt[MAX_FPGA_TO_CPU_PORT_NUM]= {0};
        if(logcnt[portNumtoCPU-NET_FPGA_TO_CPU_A]<MAX_COM_ERR_LOG_TIMES)
        {
            sprintf(strtmp,"FPGA发送goose帧长度错误,状态寄存器值为%#lX\n",Status);
            LOG_Write(LOG_KERNEL, strtmp, NULL);
            logcnt[portNumtoCPU-NET_FPGA_TO_CPU_A]++;
        }
        //清标志
        fpgaMemSet(pStatus,0x02);
    }

    if(Status&0x01)//上一帧发送成功
    {
        //      logMsg();
        //清标志
        fpgaMemSet(pStatus,0x01);
    }

    for(i=0; i<MAX_SEND_CHECK_TBD_TIMES; i++)
    {
        uint32_t *ptmp=NULL;
        ptmp=pBaseTxBD+TxCnt[portNumtoCPU-NET_FPGA_TO_CPU_A];
        TxBD=fpgaMemGet(pBaseTxBD+TxCnt[portNumtoCPU-NET_FPGA_TO_CPU_A]);
        if(TxBD&0x80000000)
        {
            TxCnt[portNumtoCPU-NET_FPGA_TO_CPU_A]=(TxCnt[portNumtoCPU-NET_FPGA_TO_CPU_A]+1)%MaxTBD[portNumtoCPU-NET_FPGA_TO_CPU_A];
            //logMsg("Send fail TxCnt=%u MaxTBD=%u\n",TxCnt[portNumtoCPU-NET_FPGA_TO_CPU_A],MaxTBD[portNumtoCPU-NET_FPGA_TO_CPU_A],3,4,5,6);
            continue;
        }
        else
        {
            //logMsg("a.ptmp=%lX, TxCnt=%u, TxBD=%lX, sendNum=%d\n",(uint32_t)ptmp,TxCnt[portNum-NET_FPGA_TO_CPU_A],TxBD,sendNum);
            pTmpDst32=(uint32_t *)(pGooseBase+(TxBD&0xFFFF));
            pTmpSrc32=(uint32_t *)sendBuf;
            m=sendNum/4;
            n=sendNum%4;
            if(n!=0)
                m++;
            for(j=0; j<m; j++)
            {
                if((j==(m-1))&&(n!=0))
                {
                    uint32_t tmp32=0;

                    for(k=0; k<n; k++)
                    {
                        tmp32|=(((uint32_t)0xFF)<<((3-k)*8));
                    }
                    tmp32&=(*pTmpSrc32);

                    fpgaMemSet(pTmpDst32,tmp32);
                }
                else
                    fpgaMemSet(pTmpDst32,*pTmpSrc32);

                pTmpDst32++;
                pTmpSrc32++;
            }
            TxBD&=0xF800FFFF;
            TxBD|=0x80000000;
            TxBD|=((sendNum&0x7FF)<<16);
            //logMsg("b.ptmp=%lX, TxCnt=%u, TxBD=%lX, sendNum=%d\n",(uint32_t)ptmp,TxCnt[portNum-NET_FPGA_TO_CPU_A],TxBD,sendNum);
            fpgaMemSet(pBaseTxBD+TxCnt[portNumtoCPU-NET_FPGA_TO_CPU_A],TxBD);
            TxCnt[portNumtoCPU-NET_FPGA_TO_CPU_A]=(TxCnt[portNumtoCPU-NET_FPGA_TO_CPU_A]+1)%MaxTBD[portNumtoCPU-NET_FPGA_TO_CPU_A];
            //logMsg("Send OK TxCnt=%u MaxTBD=%u\n",TxCnt[portNumtoCPU-NET_FPGA_TO_CPU_A],MaxTBD[portNumtoCPU-NET_FPGA_TO_CPU_A],3,4,5,6);
            break;
        }
    }

    semGive(TxSem[portNumtoCPU-NET_FPGA_TO_CPU_A]);

    if(i==MAX_SEND_CHECK_TBD_TIMES)
    {
        char strtmp[128];
        if(bResendFlag)
        {
            strcpy(strtmp,"FPGA发送BD满,放弃发送Goose\n");
            LOG_Write(LOG_KERNEL, strtmp, NULL);
            return -1;
        }
        else
        {
            bResendFlag=TRUE;
            strcpy(strtmp,"FPGA发送BD满,等待重发Goose\n");
            LOG_Dbg_Msg(strtmp,1,2,3,4,5,6);
            // regRiscTimer(RISCTIMERNUMFORGOOSERESEND,Goose_Resend_RiscTimer_Callback,0,125);/*等待125微秒*/
            // rtmEnable(RISCTIMERNUMFORGOOSERESEND);
            // startRiscTimer(RISCTIMERNUMFORGOOSERESEND);
            semTake(GOOSE_RESEND_SEM_ID,WAIT_FOREVER);/*等待riscTimer定时器重发*/
            goto resend;
        }

    }

    //LOG_Dbg_Msg("发送goose成功,sendNum=%d\n",sendNum,2,3,4,5,6);

    return sendNum;
}

int Send_Goose_to_FPGA(uint8_t portNum, uint8_t *sendBuf, int sendNum)
{
    if((portNum>=FPGA_GOOSE_NET_A)&&(portNum<(FPGA_GOOSE_NET_A+MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM)))
    {
        portNum=NET_FPGA_TO_CPU_A;
    }
    else
    {
        return -1;
    }

    return Send_Goose_to_Mac_Ctrler(portNum, sendBuf, sendNum);
}

STATUS Add_Goose_Multi_Addr_to_Mac_Ctrler(uint8_t portNumtoCPU, char *pAddr)
{
    uint32_t *pHash=NULL;
    uint64_t Hash=0;
    uint32_t Crc32=0;

    //if (appType_g != APP_TYPE_DIG)
    //    return ERROR;

    if(pAddr==NULL)
        return ERROR;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pHash = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pHash0Addr;
    }
    else
        return ERROR;


    if(bCheckBroadcastAddr(pAddr))
    {
        return Set_Broadcast_Receive_Enable(portNumtoCPU);
    }
    else if(bCheckMulticastAddr(pAddr))
    {
        uint32_t tmp=0;
        Hash=(uint64_t)fpgaMemGet(pHash)+(((uint64_t)fpgaMemGet(pHash+1))<<32);

        //Crc32=EP_CRC32(pAddr,6,0); //计算结果与FPGA不同,为什么?
        Crc32=MAC_Addr_Crc32(pAddr);

        tmp=(uint32_t)((Crc32&0xFC000000)>>26);

        Hash|=(((uint64_t)0x01)<<tmp);

        fpgaMemSet(pHash,((uint32_t)(Hash&0xFFFFFFFF)));
        fpgaMemSet(pHash+1,((uint32_t)((Hash&0xFFFFFFFF00000000)>>32)));

        return Set_Multicast_Receive_Enable(portNumtoCPU);
    }
    else
    {
        return ERROR;
    }

    return ERROR;
}

STATUS Add_Goose_Multi_Addr_to_FPGA(uint8_t portNum, char *pAddr)
{
    uint8_t portNumtoCPU;

    if((portNum>=FPGA_GOOSE_NET_A)&&(portNum<(FPGA_GOOSE_NET_A+MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM)))
    {
        portNumtoCPU=NET_FPGA_TO_CPU_A;
    }
    else
        return ERROR;

    if(Add_Goose_Multi_Addr_to_Mac_Ctrler(portNumtoCPU, pAddr)==OK)
    {
        return Add_Goose_Multi_Addr_to_Transmitter(portNum,pAddr,FPGA_GOOSE_NET_CPU);
    }
    else
        return ERROR;

}

STATUS Del_Goose_Multi_Addr_from_Mac_Ctrler(uint8_t portNumtoCPU, char *pAddr)
{
    uint32_t *pHash=NULL;
    uint64_t Hash=0;
    uint32_t Crc32=0;

    //if (appType_g != APP_TYPE_DIG)
    //    return ERROR;

    if(pAddr==NULL)
        return ERROR;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pHash = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pHash0Addr;
    }
    else
        return ERROR;


    Hash=(uint64_t)fpgaMemGet(pHash)+(((uint64_t)fpgaMemGet(pHash+1))<<32);

    //Crc32=EP_CRC32(pAddr,6,0);
    Crc32=MAC_Addr_Crc32(pAddr);

    Hash&=~(((uint64_t)1)<<((Crc32&0xFC000000)>>26));

    fpgaMemSet(pHash,((uint32_t)(Hash&0xFFFFFFFF)));
    fpgaMemSet(pHash+1,((uint32_t)((Hash&0xFFFFFFFF00000000)>>32)));

    return OK;
}

STATUS Get_Goose_Mac_Addr_from_Mac_Ctrler(uint8_t portNumtoCPU, uint8_t *addr)
{
    uint32_t *pMacAddr=NULL;
    uint32_t MacAddr0=0;
    uint32_t MacAddr1=0;

    //if (appType_g != APP_TYPE_DIG)
    //    return ERROR;

    if(addr==NULL)
        return ERROR;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pMacAddr = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pMacAddr0Addr;
    }
    else
        return ERROR;

    MacAddr0=fpgaMemGet(pMacAddr);
    MacAddr1=fpgaMemGet(pMacAddr+1);

    addr[0]=(uint8_t)((MacAddr1&0xFF00)>>8);
    addr[1]=(uint8_t)(MacAddr1&0xFF);
    addr[2]=(uint8_t)((MacAddr0&0xFF000000)>>24);
    addr[3]=(uint8_t)((MacAddr0&0xFF0000)>>16);
    addr[4]=(uint8_t)((MacAddr0&0xFF00)>>8);
    addr[5]=(uint8_t)(MacAddr0&0xFF);

    return OK;
}

STATUS Set_Goose_Mac_Addr_to_Mac_Ctrler(uint8_t portNumtoCPU, uint8_t *addr)
{
    uint32_t *pMacAddr=NULL;
    uint32_t MacAddr0=0;
    uint32_t MacAddr1=0;

    //if (appType_g != APP_TYPE_DIG)
    //    return ERROR;

    if(addr==NULL)
        return ERROR;

    if((portNumtoCPU>=NET_FPGA_TO_CPU_A)&&(portNumtoCPU<(NET_FPGA_TO_CPU_A+MAX_FPGA_TO_CPU_PORT_NUM)))
    {
        pMacAddr = FPGAGooseRegAddr[portNumtoCPU-NET_FPGA_TO_CPU_A].pMacAddr0Addr;
    }
    else
        return ERROR;

    MacAddr1=((((uint32_t)addr[0])<<8)+(uint32_t)addr[1])&0xFFFF;
    MacAddr0=((((uint32_t)addr[2])<<24)+(((uint32_t)addr[3])<<16)+(((uint32_t)addr[4])<<8)+(uint32_t)addr[5])&0xFFFFFFFF;

    fpgaMemSet(pMacAddr,MacAddr0);
    fpgaMemSet(pMacAddr+1,MacAddr1);

    return OK;
}

STATUS Add_Goose_Multi_Addr_to_Transmitter(uint8_t SrcPortNum, char *pAddr, uint8_t DestPortNum)
{
    static uint8_t ucAddrNum=0;
    static char TransmitterAddrArry[FPGA_TRANSMITTER_MAX_MULTI_ADDR_NUM][6];
    uint8_t i=0;
    char GooseOrigMultiAddr[6]= {0x01,0x0c,0xcd,0x01,0x00,0x00};
    uint32_t AddrMapR0=0;
    uint32_t AddrMapR1=0;

    if(pAddr==NULL)
    {
        return ERROR;
    }

    if(memcmp(pAddr,GooseOrigMultiAddr,4)!=0)
    {
        LOG_Dbg_Msg("地址%s不是标准Goose组播地址，加入Transmitter失败\n",(int)pAddr,0,0,0,0,0);
        return ERROR;
    }

    if((SrcPortNum>=(FPGA_GOOSE_NET_A+MAX_FPGA_TRANSMITTER_GOOSE_PORT_NUM))
            ||(SrcPortNum<FPGA_GOOSE_NET_A))
    {
        LOG_Dbg_Msg("源端口%u超出范围，加入Transmitter失败\n",SrcPortNum,0,0,0,0,0);
        return ERROR;
    }
    SrcPortNum-=FPGA_GOOSE_NET_A;

    if((DestPortNum>=(FPGA_GOOSE_NET_A+MAX_FPGA_TRANSMITTER_GOOSE_PORT_NUM))
            ||(DestPortNum<FPGA_GOOSE_NET_A))
    {
        LOG_Dbg_Msg("目的端口%u超出范围，加入Transmitter失败\n",DestPortNum,0,0,0,0,0);
        return ERROR;
    }
    DestPortNum-=FPGA_GOOSE_NET_A;

    for(i=0; i<ucAddrNum; i++)
    {
        if(memcmp(TransmitterAddrArry[i],pAddr,6)==0)
        {
            //在该地址寄存器置接收端口号
            AddrMapR1=fpgaMemGet(FPGATransAdRegAddr[i].pAddrMapAddr1);
            AddrMapR1|=((uint32_t)0x01)<<(16+SrcPortNum);
            AddrMapR1|=((uint32_t)0x01)<<DestPortNum;
            fpgaMemSet(FPGATransAdRegAddr[i].pAddrMapAddr1,AddrMapR1);
            return OK;
        }
    }

    if(i==ucAddrNum)//未曾加入过该地址
    {
        if(ucAddrNum>=16)
        {
            LOG_Dbg_Msg("Transmitter地址数目已达上限，不能再增加\n",0,0,0,0,0,0);
            return ERROR;
        }

        //在地址寄存器加入该地址，并置该接收端口号
        AddrMapR0=fpgaMemGet(FPGATransAdRegAddr[ucAddrNum].pAddrMapAddr0);
        AddrMapR0&=0xFFFF0000;
        AddrMapR0|=(((((uint32_t)pAddr[4])<<8)+(uint32_t)pAddr[5])&0xFFFF);
        fpgaMemSet(FPGATransAdRegAddr[ucAddrNum].pAddrMapAddr0,AddrMapR0);

        AddrMapR1=fpgaMemGet(FPGATransAdRegAddr[ucAddrNum].pAddrMapAddr1);
        AddrMapR1|=(((uint32_t)0x01)<<(16+SrcPortNum));
        AddrMapR1|=(((uint32_t)0x01)<<DestPortNum);
        fpgaMemSet(FPGATransAdRegAddr[ucAddrNum].pAddrMapAddr1,AddrMapR1);

        //将该地址存入本地数据结构
        memcpy(TransmitterAddrArry[ucAddrNum],pAddr,6);

        ucAddrNum++;
    }

    return OK;
}

STATUS Add_Goose_Mac_Addr_to_Transmitter(uint8_t PortNum, char *pSrcAddr)
{
    uint32_t SrcAddrR0=0;
    uint32_t SrcAddrR1=0;

    if(pSrcAddr==NULL)
    {
        return ERROR;
    }

    if((PortNum>=(FPGA_GOOSE_NET_A+MAX_FPGA_TRANSMITTER_GOOSE_PORT_NUM))
            ||(PortNum<FPGA_GOOSE_NET_A))
    {
        LOG_Dbg_Msg("端口%u超出范围，源地址加入Transmitter失败\n",PortNum,0,0,0,0,0);
        return ERROR;
    }
    PortNum-=FPGA_GOOSE_NET_A;

    SrcAddrR0=((((uint32_t)pSrcAddr[0])<<8)+(uint32_t)pSrcAddr[1])&0xFFFF;
    SrcAddrR1=((((uint32_t)pSrcAddr[2])<<24)+(((uint32_t)pSrcAddr[3])<<16)+(((uint32_t)pSrcAddr[4])<<8)+(uint32_t)pSrcAddr[5])&0xFFFFFFFF;

    fpgaMemSet(FPGATransSrcAdRegAddr[PortNum].pSourceAddrAddr0,SrcAddrR0);
    fpgaMemSet(FPGATransSrcAdRegAddr[PortNum].pSourceAddrAddr1,SrcAddrR1);

    return OK;
}


STATUS Get_Goose_Mac_Addr_from_Transmitter(uint8_t PortNum, uint8_t *addr)
{
    uint32_t SrcAddrR0=0;
    uint32_t SrcAddrR1=0;

    if(addr==NULL)
    {
        return ERROR;
    }

    if((PortNum>=(FPGA_GOOSE_NET_A+MAX_FPGA_TRANSMITTER_GOOSE_PORT_NUM))
            ||(PortNum<FPGA_GOOSE_NET_A))
    {
        LOG_Dbg_Msg("端口%u超出范围，从Transmitter获取源地址失败\n",PortNum,0,0,0,0,0);
        return ERROR;
    }
    PortNum-=FPGA_GOOSE_NET_A;

    SrcAddrR0=fpgaMemGet(FPGATransSrcAdRegAddr[PortNum].pSourceAddrAddr0);
    SrcAddrR1=fpgaMemGet(FPGATransSrcAdRegAddr[PortNum].pSourceAddrAddr1);

    addr[0]=(uint8_t)((SrcAddrR0&0xFF00)>>8);
    addr[1]=(uint8_t)(SrcAddrR0&0xFF);
    addr[2]=(uint8_t)((SrcAddrR1&0xFF000000)>>24);
    addr[3]=(uint8_t)((SrcAddrR1&0xFF0000)>>16);
    addr[4]=(uint8_t)((SrcAddrR1&0xFF00)>>8);
    addr[5]=(uint8_t)(SrcAddrR1&0xFF);

    return OK;
}

STATUS Get_Goose_Shared_Mem_Status(BOOL *pbOverflow)
{
    uint32_t SharedMemStatus=0;

    if(pbOverflow==NULL)
        return ERROR;

    SharedMemStatus=fpgaMemGet(FPGAAddOnInfoAddr.pSharedMemStatAddr);

    if(SharedMemStatus&0x01)
    {
        *pbOverflow=TRUE;
    }
    else
    {
        *pbOverflow=FALSE;
    }

    return OK;
}

STATUS Clear_Goose_Shared_Mem_Status()
{
    uint32_t SharedMemStatus=0;

    SharedMemStatus=fpgaMemGet(FPGAAddOnInfoAddr.pSharedMemStatAddr);

    SharedMemStatus|=0x01;

    fpgaMemSet(FPGAAddOnInfoAddr.pSharedMemStatAddr,SharedMemStatus);

    return OK;
}

STATUS Set_Receive_Port_Start_Num()
{
    uint32_t RcvPortStatNum=0x03;

    fpgaMemSet(FPGAAddOnInfoAddr.pReceivePortStartNumAddr,RcvPortStatNum);

    return OK;
}

STATUS Set_Source_Mac_Replace()
{
    uint32_t SrcMacRepR=0;
    int i;

    for(i=0; i<MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM; i++) //接收Source Mac替换
    {
        SrcMacRepR|=((uint32_t)1<<i);
    }

    SrcMacRepR=SrcMacRepR<<16;

    for(i=0; i<MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM; i++) //发送Source Mac替换
    {
        SrcMacRepR|=((uint32_t)1<<i);
    }

    fpgaMemSet(FPGAAddOnInfoAddr.pSourceMacReplaceAddr,SrcMacRepR);

    return OK;
}

BOOL Poll_FPGA_Goose_Receive()
{
    BOOL bResult=TRUE;
    BOOL bSharedMemOverflow=FALSE;
    uint8_t i;

    /* 非F-A板不处理
     */
    if (VER_GetHwBoardSN() != E02_CPU_F_BORAD)
    {
        return bResult;
    }

    for(i=0; i<MAX_FPGA_TO_CPU_PORT_NUM; i++)
    {
        if(Receive_Goose_from_Mac_Ctrler(NET_FPGA_TO_CPU_A+i)<0)
        {
            bResult=FALSE;
        }
    }

    if(Get_Goose_Shared_Mem_Status(&bSharedMemOverflow)==OK)
    {
        if(bSharedMemOverflow)
        {
            char strtmp[128];
            static uint32_t ulCnt = 0;

            ulCnt++;
            if (ulCnt<MAX_COM_ERR_LOG_TIMES)
            {
                strcpy(strtmp,"FPGA Goose共享内存溢出!\n");
                LOG_Write(LOG_KERNEL, strtmp, NULL);
            }
            Clear_Goose_Shared_Mem_Status();
        }
    }
    else
    {
        bResult=FALSE;
        LOG_Dbg_Msg("获取FPGA Goose共享内存状态失败!\n",1,2,3,4,5,6);
    }

    return bResult;
}

BOOL Init_FPGA_Goose()
{
    BOOL bResult=TRUE;
    uint8_t i;

    if(VER_GetHwBoardSN() != E02_CPU_F_BORAD)
    {
        return TRUE;
    }

    //if (appType_g != APP_TYPE_DIG)
    //   return FALSE;

    //初始化寄存器地址
    for(i=0; i<MAX_FPGA_TO_CPU_PORT_NUM; i++)
    {
        FPGAGooseRegAddr[i].pBaseAddr=(uint32_t *)(FPGA_BASE_ADDR+FPGA_GOOSE_OFFSET*(i+1));
        FPGAGooseRegAddr[i].pModerAddr=(uint32_t *)((uint8_t *)FPGAGooseRegAddr[i].pBaseAddr+FPGA_GOOSE_MODER_OFFSET);
        FPGAGooseRegAddr[i].pStatusAddr=(uint32_t *)((uint8_t *)FPGAGooseRegAddr[i].pBaseAddr+FPGA_GOOSE_STATUS_OFFSET);
        FPGAGooseRegAddr[i].pIntMaskAddr=(uint32_t *)((uint8_t *)FPGAGooseRegAddr[i].pBaseAddr+FPGA_GOOSE_INTMASK_OFFSET);
        FPGAGooseRegAddr[i].pMacAddr0Addr=(uint32_t *)((uint8_t *)FPGAGooseRegAddr[i].pBaseAddr+FPGA_GOOSE_MAC_ADDR0_OFFSET);
        FPGAGooseRegAddr[i].pMacAddr1Addr=(uint32_t *)((uint8_t *)FPGAGooseRegAddr[i].pBaseAddr+FPGA_GOOSE_MAC_ADDR1_OFFSET);
        FPGAGooseRegAddr[i].pHash0Addr=(uint32_t *)((uint8_t *)FPGAGooseRegAddr[i].pBaseAddr+FPGA_GOOSE_HASH0_OFFSET);
        FPGAGooseRegAddr[i].pHash1Addr=(uint32_t *)((uint8_t *)FPGAGooseRegAddr[i].pBaseAddr+FPGA_GOOSE_HASH1_OFFSET);
        FPGAGooseRegAddr[i].pBDNumAddr=(uint32_t *)((uint8_t *)FPGAGooseRegAddr[i].pBaseAddr+FPGA_GOOSE_BD_NUM_OFFSET);
        FPGAGooseRegAddr[i].pTxBDAddr=(uint32_t *)((uint8_t *)FPGAGooseRegAddr[i].pBaseAddr+FPGA_GOOSE_TX_BD_OFFSET);
        FPGAGooseRegAddr[i].pRxBDAddr=(uint32_t *)((uint8_t *)FPGAGooseRegAddr[i].pBaseAddr+FPGA_GOOSE_RX_BD_OFFSET);

        FPGAHSBStyleGoRxAddr[i].pExchangeReg=(uint32_t *)((uint8_t *)FPGAGooseRegAddr[i].pBaseAddr+FPGA_HSB_STYLE_GOOSE_RX_REG_OFFSET);
        FPGAHSBStyleGoRxAddr[i].pRxBufAddr=(uint32_t *)((uint8_t *)FPGAGooseRegAddr[i].pBaseAddr+FPGA_HSB_STYLE_GOOSE_RX_BUFFER_OFFSET);

        RxSem[i] = semMCreate(SEM_Q_PRIORITY);
        TxSem[i] = semMCreate(SEM_Q_PRIORITY);
    }

    for(i=0; i<FPGA_TRANSMITTER_MAX_MULTI_ADDR_NUM; i++)
    {
        FPGATransAdRegAddr[i].pAddrMapAddr0=(uint32_t *)(FPGA_BASE_ADDR+FPGA_TRANSMITTER_OFFSET+FPGA_TRANSMITTER_ADDR_MAP_OFFSET+8*i);
        FPGATransAdRegAddr[i].pAddrMapAddr1=(uint32_t *)(FPGA_BASE_ADDR+FPGA_TRANSMITTER_OFFSET+FPGA_TRANSMITTER_ADDR_MAP_OFFSET+8*i+4);
    }

    for(i=0; i<MAX_FPGA_TRANSMITTER_GOOSE_PORT_NUM; i++)
    {
        FPGATransSrcAdRegAddr[i].pSourceAddrAddr0=(uint32_t *)(FPGA_BASE_ADDR+FPGA_TRANSMITTER_OFFSET+FPGA_TRANSMITTER_PORT_SRC_ADDR_OFFSET+8*i);
        FPGATransSrcAdRegAddr[i].pSourceAddrAddr1=(uint32_t *)(FPGA_BASE_ADDR+FPGA_TRANSMITTER_OFFSET+FPGA_TRANSMITTER_PORT_SRC_ADDR_OFFSET+8*i+4);
    }

    FPGAAddOnInfoAddr.pReceivePortStartNumAddr=(uint32_t *)(FPGA_BASE_ADDR+FPGA_TRANSMITTER_OFFSET+FPGA_TRANSMITTER_RCV_PORT_START_NUM_OFFSET);
    FPGAAddOnInfoAddr.pSourceMacReplaceAddr=(uint32_t *)(FPGA_BASE_ADDR+FPGA_TRANSMITTER_OFFSET+FPGA_TRANSMITTER_SRC_MAC_REPLACE_OFFSET);
    FPGAAddOnInfoAddr.pSharedMemStatAddr=(uint32_t *)(FPGA_BASE_ADDR+FPGA_TRANSMITTER_OFFSET+FPGA_SHARED_MEM_STATUS_OFFSET);
    /*
    	if(fpgaGetProtocolVer()>=0x0109)
    	{
    		bUseFPGAnsTime=TRUE;
    	}
    	else
    	{
    		bUseFPGAnsTime=FALSE;
    	}
    */
    if(fpgaGetProtocolVer()>=0x0110)
    {
        bUseHSBStyleRxInterface=TRUE;
    }
    else
    {
        bUseHSBStyleRxInterface=FALSE;
    }

    // if(FPGAMacAddrSet()!=OK)
    // {
    //     bResult=FALSE;
    // }

    if(Set_Receive_Port_Start_Num()!=OK)
    {
        bResult=FALSE;
    }

    if(Set_Source_Mac_Replace()!=OK)
    {
        bResult=FALSE;
    }

    for(i=0; i<MAX_FPGA_TO_CPU_PORT_NUM; i++)
    {
        if(!Init_BD_Num(NET_FPGA_TO_CPU_A+i))
        {
            bResult=FALSE;
        }

        if(Set_Interrupt_Disable(NET_FPGA_TO_CPU_A+i)!=OK)
        {
            bResult=FALSE;
        }
    }

    GOOSE_RESEND_SEM_ID = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
    // rtmDisable(RISCTIMERNUMFORGOOSERESEND);

    //Set_Promiscuous_Receive_Enable(FPGA_GOOSE_NET_A);

    return bResult;
}

/***********************************************
FPGA GMRP报文发送相关接口
************************************************/

BOOL Set_GMRP_Send_Interval(uint32_t SendInterval)
{
    uint32_t *pGmrpR0=NULL;
    uint32_t GmrpR0=0;

    if((SendInterval&0xFFFFE000)!=0)
        SendInterval=0x1FFF;

    pGmrpR0=FPGAGmrpRegAddr.pGmrpReg0Addr;

    GmrpR0=fpgaMemGet(pGmrpR0);

    GmrpR0&=0xFFFFE000;
    GmrpR0|=(uint32_t)(SendInterval&0x1FFF);

    fpgaMemSet(pGmrpR0,GmrpR0);

    return TRUE;
}

BOOL Get_Gmrp_Send_Flag(uint8_t portNum)
{
    uint32_t *pGmrpR1=NULL;
    uint32_t GmrpR1=0;
    uint32_t *pGmrpR2=NULL;
    uint32_t GmrpR2=0;
    uint64_t GmrpR64=0;
    uint8_t bitoffset=0;

    if((portNum>=FPGA_GOOSE_NET_A)&&(portNum<(FPGA_GOOSE_NET_A+MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM_WITHOUT_SPT)))
    {
        pGmrpR1 = FPGAGmrpRegAddr.pGmrpReg1Addr;
        pGmrpR2 = FPGAGmrpRegAddr.pGmrpReg2Addr;
    }
    else
        return FALSE;

    GmrpR1=fpgaMemGet(pGmrpR1);
    GmrpR2=fpgaMemGet(pGmrpR2);
    GmrpR64=(((uint64_t)GmrpR1)<<32)|((uint64_t)(GmrpR2));
    bitoffset=64-((portNum-FPGA_GOOSE_NET_A)*8)-1;

    if(GmrpR64&(((uint64_t)0x01)<<bitoffset))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL Set_Gmrp_Send_Flag(uint8_t portNum)
{
    uint32_t *pGmrpR1=NULL;
    uint32_t GmrpR1=0;
    uint32_t *pGmrpR2=NULL;
    uint32_t GmrpR2=0;
    uint64_t GmrpR64=0;
    uint8_t bitoffset=0;

    if((portNum>=FPGA_GOOSE_NET_A)&&(portNum<(FPGA_GOOSE_NET_A+MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM_WITHOUT_SPT)))
    {
        pGmrpR1 = FPGAGmrpRegAddr.pGmrpReg1Addr;
        pGmrpR2 = FPGAGmrpRegAddr.pGmrpReg2Addr;
    }
    else
        return FALSE;

    GmrpR1=fpgaMemGet(pGmrpR1);
    GmrpR2=fpgaMemGet(pGmrpR2);
    GmrpR64=(((uint64_t)GmrpR1)<<32)|((uint64_t)(GmrpR2));
    bitoffset=64-((portNum-FPGA_GOOSE_NET_A)*8)-1;
    GmrpR64|=(((uint64_t)0x01)<<bitoffset);

    fpgaMemSet(pGmrpR2,((uint32_t)(GmrpR64&0xFFFFFFFF)));
    fpgaMemSet(pGmrpR1,((uint32_t)((GmrpR64&0xFFFFFFFF00000000)>>32)));

    return TRUE;
}

int Get_Gmrp_Send_Len(uint8_t portNum)
{
    uint32_t *pGmrpR1=NULL;
    uint32_t GmrpR1=0;
    uint32_t *pGmrpR2=NULL;
    uint32_t GmrpR2=0;
    uint64_t GmrpR64=0;
    uint8_t bitoffset=0;

    if((portNum>=FPGA_GOOSE_NET_A)&&(portNum<(FPGA_GOOSE_NET_A+MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM_WITHOUT_SPT)))
    {
        pGmrpR1 = FPGAGmrpRegAddr.pGmrpReg1Addr;
        pGmrpR2 = FPGAGmrpRegAddr.pGmrpReg2Addr;
    }
    else
        return -1;

    GmrpR1=fpgaMemGet(pGmrpR1);
    GmrpR2=fpgaMemGet(pGmrpR2);
    GmrpR64=(((uint64_t)GmrpR1)<<32)|((uint64_t)(GmrpR2));
    bitoffset=64-((portNum-FPGA_GOOSE_NET_A)*8)-8;

    return (int)((GmrpR64>>bitoffset)&0x7F);
}

BOOL Set_Gmrp_Send_Len(uint8_t portNum, int sendNum)
{
    uint32_t *pGmrpR1=NULL;
    uint32_t GmrpR1=0;
    uint32_t *pGmrpR2=NULL;
    uint32_t GmrpR2=0;
    uint64_t GmrpR64=0;
    uint8_t bitoffset=0;

    if((sendNum<=0)&&(sendNum>=FPGA_GMRP_SEND_BUFFER_LEN))
    {
        return FALSE;
    }

    if((portNum>=FPGA_GOOSE_NET_A)&&(portNum<(FPGA_GOOSE_NET_A+MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM_WITHOUT_SPT)))
    {
        pGmrpR1 = FPGAGmrpRegAddr.pGmrpReg1Addr;
        pGmrpR2 = FPGAGmrpRegAddr.pGmrpReg2Addr;
    }
    else
        return FALSE;

    GmrpR1=fpgaMemGet(pGmrpR1);
    GmrpR2=fpgaMemGet(pGmrpR2);
    GmrpR64=(((uint64_t)GmrpR1)<<32)|((uint64_t)(GmrpR2));
    bitoffset=64-((portNum-FPGA_GOOSE_NET_A)*8)-8;
    GmrpR64&=(~(((uint64_t)0x7F)<<bitoffset));
    GmrpR64|=(((uint64_t)(sendNum&0x7F))<<bitoffset);

    fpgaMemSet(pGmrpR2,((uint32_t)(GmrpR64&0xFFFFFFFF)));
    fpgaMemSet(pGmrpR1,((uint32_t)((GmrpR64&0xFFFFFFFF00000000)>>32)));

    return TRUE;
}

int Send_Gmrp_to_FPGA(uint8_t portNum, uint8_t *sendBuf, int sendNum)
{
    uint32_t *pGmrpSendBuf=NULL;
    uint16_t j,k;
    int m,n;
    uint32_t *pTmpSrc32=NULL;
    uint32_t *pTmpDst32=NULL;


    //LOG_Dbg_Msg("Enter Send_Gmrp_to_FPGA port=%u,sendnum=%d",portNum,sendNum,0,0,0,0);

    if(sendBuf==NULL)
    {
        return -1;
    }

    if((sendNum<=0)||(sendNum>=FPGA_GMRP_SEND_BUFFER_LEN))
    {
        return -1;
    }

    if((portNum>=FPGA_GOOSE_NET_A)&&(portNum<(FPGA_GOOSE_NET_A+MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM_WITHOUT_SPT)))
    {
        pGmrpSendBuf = FPGAGmrpSendBufAddr[portNum-FPGA_GOOSE_NET_A].pGmrpSendBufAddr;
    }
    else
        return -1;


    if(Get_Gmrp_Send_Flag(portNum)&&(Get_Gmrp_Send_Len(portNum)==sendNum))
    {
        return sendNum;
    }

    pTmpDst32=(uint32_t *)pGmrpSendBuf;
    pTmpSrc32=(uint32_t *)sendBuf;
    m=sendNum/4;
    n=sendNum%4;
    if(n!=0)
        m++;
    for(j=0; j<m; j++)
    {
        if((j==(m-1))&&(n!=0))
        {
            uint32_t tmp32=0;

            for(k=0; k<n; k++)
            {
                tmp32|=(((uint32_t)0xFF)<<((3-k)*8));
            }
            tmp32&=(*pTmpSrc32);

            fpgaMemSet(pTmpDst32,tmp32);
        }
        else
            fpgaMemSet(pTmpDst32,*pTmpSrc32);

        pTmpDst32++;
        pTmpSrc32++;
    }

    if(!Set_Gmrp_Send_Len(portNum,sendNum))
    {
        return -1;
    }

    if(!Set_Gmrp_Send_Flag(portNum))
    {
        return -1;
    }

    return sendNum;
}

BOOL Init_FPGA_GMRP()
{
    BOOL bResult=TRUE;
    int i;

    if(VER_GetHwBoardSN() != E02_CPU_F_BORAD)
    {
        return TRUE;
    }

    FPGAGmrpRegAddr.pGmrpReg0Addr=(uint32_t *)(FPGA_BASE_ADDR+FPGA_GMRP_SEND_OFFSET+FPGA_GMRP_REG0_OFFSET);
    FPGAGmrpRegAddr.pGmrpReg1Addr=(uint32_t *)(FPGA_BASE_ADDR+FPGA_GMRP_SEND_OFFSET+FPGA_GMRP_REG1_OFFSET);
    FPGAGmrpRegAddr.pGmrpReg2Addr=(uint32_t *)(FPGA_BASE_ADDR+FPGA_GMRP_SEND_OFFSET+FPGA_GMRP_REG2_OFFSET);

    for(i=0; i<MAX_EXTERNAL_FPGA_GOOSE_PORT_NUM_WITHOUT_SPT; i++)
    {
        FPGAGmrpSendBufAddr[i].pGmrpSendBufAddr=(uint32_t *)(FPGA_BASE_ADDR+FPGA_GMRP_SEND_OFFSET+FPGA_GMRP_SEND_BUFFER_OFFSET+FPGA_GMRP_SEND_BUFFER_LEN*i);
    }

    if(!Set_GMRP_Send_Interval(GMRP_SEND_INTERVAL))
    {
        bResult=FALSE;
    }

    return bResult;
}

int Get_Max_FPGA_Goose_Receive_Poll_One_Time(void)
{
    return MaxReceivePacketsOneTimeHSBStyle;
}

int Set_Max_FPGA_Goose_Receive_Poll_One_Time(int pollnum)
{
    MaxReceivePacketsOneTimeHSBStyle=pollnum;

    return Get_Max_FPGA_Goose_Receive_Poll_One_Time();
}
