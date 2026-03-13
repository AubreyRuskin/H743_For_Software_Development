/* ext_box.c - This file contains interface to extend box data acq. system. */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 10sep03, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains interface to extend box data acq. system.
INCLUDES: ext_box.h
*/

/* includes */

#include "string_compat.h"

#include <semLib.h>
#include <logLib.h>
#include <taskLib.h>

#include "sysinfo.h"
#include "ext_box.h"
#include "ext_eth.h"
#include "dspai.h"
#include "errtest.h"
#include "datetime.h"
#include "realdata.h"
#include "logmsg.h"
#include "miscfunc.h"
#include "POLE_Data.h"
#include "HDL_Data.h"

/* defines */

typedef struct
{
    uint8_t ucMod;
    uint8_t ucHdCh;
    uint8_t aucFilt[4];
} EXT_DI_CFG;

/* globals */

int iExtDiNum_g;
void *pvExtAiMod_g;
int nExtRecvTaskID_g=-1; /* 扩展机相通信任务初始化标志 */
BOOL bExtRecvTaskStartFlag_g=FALSE;
uint32_t ulExtAccessCounter_g=0; 			/* Monitor counter for sampling task based on photoelectron current transition.
                                               the initial value is 0, then it will be accumulated in the sampling task.
                                               If the accumulation don't happen, alarm will be reported. */

extern BOOL bExtBoxCoffUpdate; /* 扩展机箱系数更新标志 */
extern uint16_t usCoffUpdateCount;  /* 系数更新时计数,与bExtBoxCoffUpdate变量配合使用 */

/* locals */

static EXT_DI_CFG adicfgExt_g[MAX_MOD_NUM*MAX_DI_PER_MOD];
static uint32_t aulExtDiSts_g[(MAX_MOD_NUM*MAX_DI_PER_MOD+31)/32];
static EXT_DI_HND ahextdi_g[MAX_MOD_NUM*MAX_DI_PER_MOD];

static u_int uiExtLgcCh_g;
static u_int uiExtCalcCh_g;
static u_int uiTxDiLong_g;

static u_int uiExtLgcCh_g_8;
static u_int uiExtCalcCh_g_4;

static int iBurstSendSts=1;		/* 发送开始标志 */
static int iAcCofBurstSendSts = 1;		/* 通道系数发送开始标志 */
static u_int uiCalcCfg_s;  /* 预处理通道数,函数调用传入本模块 */

char acExtHwVer[32];    /*扩展机箱硬件版本号*/
/*char acExtBspVer[32];
char acExtEdpVer[32];
uint16_t unExtEdpCrc;*/
uint8_t ucExtIoSum;    /*IO模块数量*/
uint8_t aExtSubModInfo[MAX_MOD_NUM*5]; /* 最多支持MAX_MOD_NUM个模件 */


/* local functions */

static int EX_Send_Wait_Ack(uint8_t *pucSnd, int iLen);
/***********************************************************************
* EXBurstSendWaitAck - 瞬时发送时应答
*
* RETURNS: 无
*
*/
static int EXBurstSendWaitAck(
    uint8_t *pucSnd,
    int iLen
);

/***********************************************************************
* ExtRecvCmd - 光CT采样接收任务
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS ExtRecvCmd(
    RD_AI_MOD *pAiMod, 	/* 扩展机箱模件信息 */
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
);

/* functions */

/***********************************************************************
* EX_Init_DI - 初始化DI通道
*
* RETURNS: 用来索引DI通道的void指针，或者NULL表示调用出错
*
*/
void *EX_Init_DI(
    int iModAddr, 		/* 模块硬件地址 */
    u_int uiCh, 		/* 在本模件内的DO物理通道号，从0开始 */
    uint32_t ulFilt					/* 去抖动时间，单位us */
)
{
    EXT_DI_CFG *pcfg;
    EXT_DI_HND *phnd;

    assert(iModAddr<MAX_MOD_NUM && uiCh<MAX_DI_PER_MOD);

    assert(iExtDiNum_g<sizeof(adicfgExt_g)/sizeof(adicfgExt_g[0]));
    pcfg=adicfgExt_g+iExtDiNum_g;

    pcfg->ucMod=iModAddr;
    pcfg->ucHdCh=uiCh;

    pcfg->aucFilt[0]=HH8(ulFilt);
    pcfg->aucFilt[1]=HL8(ulFilt);
    pcfg->aucFilt[2]=LH8(ulFilt);
    pcfg->aucFilt[3]=LL8(ulFilt);

    phnd=ahextdi_g+iExtDiNum_g;

    phnd->pulStsPos=aulExtDiSts_g+iExtDiNum_g/32;
    phnd->ulStsMsk=BV32(iExtDiNum_g%32);

    iExtDiNum_g++;

    return (void*)phnd;
}

/***********************************************************************
* ExtRecvInit - 光CT采样通信接收初始化
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS ExtRecvInit(
    RD_AI_MOD *pAiMod			/* 扩展机箱模件信息 */
)
{
    /* 有浮点操作 */

    nExtRecvTaskID_g=taskSpawn("tExtRcvCmd",TSK_PRI_DSP, VX_FP_TASK, 100000, ExtRecvCmd,
                               (int)pAiMod,0,0, 0, 0, 0, 0, 0, 0, 0);
    assert(nExtRecvTaskID_g != ERROR);
    bExtRecvTaskStartFlag_g=TRUE;

    return EP_SUCCESS;
}

/***********************************************************************
* Init_Ext_Box - 初始化（并启动）扩展机箱
*
* RETURNS:
*               EP_SUCCESS，正常返回
*               EP_BUF_ERR，内存错误
*               EP_COM_ERR，扩展机箱通信出错
*
*/
EP_STATUS Init_Ext_Box(
    u_int uiSmplRate, 		/* 采样速率 */
    u_int uiSysFreq, 							/* 系统频率 */
    u_int uiTxPts,		/* 每次传送采样点数 */
    void *pvAiMod, 						/* 该模块（扩展机箱负责的所有AI采集/计算通道）的句柄 */
    u_int uiLgcCh, 				/* 采样的逻辑通道数 */
    DSP_LGC_AI_CFG *plgccfg,				/* 指向逻辑通道配置数组第0个元素的指针，数组元素有unLgcCh个 */
    u_int uiCalcCfg, 			/* 预处理通道配置数 */
    DSP_CALC_AI_CFG *pcalccfg						/* 指向预处理通道配置数组第0个元素的指针，数组元素uiCalcCfg个 */
)
{
    uint8_t aucEthPacket[ETH_BD_HEAD+ETH_MAX_DAT];
#define aucSndBuf   (aucEthPacket+ETH_BD_HEAD)				/* 数据地址 */
    uint8_t *puc;
    DSP_LGC_AI_CFG *plcfg;
    uint8_t *pucContinue;
    int iMsgSN;
    int i;
    uint32_t   ulTryComCnt=0;
    float fDefaultChnCoff = 1.0; /* 缺省通道系数,扩展机箱不处理通道系数 */

    pvExtAiMod_g=pvAiMod;
    uiExtLgcCh_g=uiLgcCh;

    uiCalcCfg_s = uiCalcCfg; /* 预处理通道数 */

    uiExtCalcCh_g=0;
    for (i=0; i<uiCalcCfg; i++)
    {
        if (pcalccfg[i].ucArithParm & 0x20)			/* 预处理上传，且为2个浮点数 */
            uiExtCalcCh_g+=pcalccfg[i].ucChNum;				/* 预处理通道数 */
    }
    uiExtLgcCh_g_8=uiExtLgcCh_g/8;			/* 按8求商 */
    uiExtCalcCh_g_4=uiExtCalcCh_g/4;						/* 按4求商 */
    uiTxDiLong_g=(iExtDiNum_g+31)/32;		/* 开入字数 */

reinit:
    LOG_Dbg_Msg("Try to communicate with extend box.\n", 0, 0, 0, 0, 0, 0);
    ulTryComCnt++;

    if(ulTryComCnt>90)
    {
        /*2006-11-23日　等待1.5分钟，张云　修改  */
        return   EP_COM_ERR;
    }

    plcfg=plgccfg;
    pucContinue=(uint8_t*)pcalccfg;
    assert(sizeof(*pcalccfg)==4);

    for (iMsgSN=0; ; iMsgSN++)
    {
        /* 重复传递 */
        aucSndBuf[0]=0xFF;
        aucSndBuf[1]=0xFF;
        aucSndBuf[3]=0x01;

        if (!iMsgSN)
        {
            /* 第一次 */
            aucSndBuf[2]=0x80;

            assert(uiSmplRate<=255);
            aucSndBuf[4]=uiSmplRate;

            assert(uiSysFreq==50 || uiSysFreq==60);
            aucSndBuf[5]=uiSysFreq;

            assert(uiTxPts<uiSmplRate);
            aucSndBuf[6]=uiTxPts;

            if(ENG_MODE == 1)
            {
                aucSndBuf[6] |= 0x08;
            }

            assert(uiLgcCh<=255);
            aucSndBuf[7]=uiLgcCh;

            aucSndBuf[8]=HI8(uiCalcCfg);
            aucSndBuf[9]=LO8(uiCalcCfg);

            for (i=0, puc=aucSndBuf+10; i<uiLgcCh; i++, plcfg++)
            {
                assert(plcfg->ucHdCh);
                *puc++=plcfg->ucHdCh-1;

                /* ALERT: assume Big endian and sizeof(float)==4 here!!! */
                *puc++ = *(uint8_t *)&fDefaultChnCoff;
                *puc++ = *((uint8_t *)&fDefaultChnCoff+1);
                *puc++ = *((uint8_t *)&fDefaultChnCoff+2);
                *puc++ = *((uint8_t *)&fDefaultChnCoff+3);

                *puc++=plcfg->ucFiltNum;
            }

            while (puc<aucSndBuf+ETH_MAX_DAT)
            {
                if (pucContinue<(uint8_t*)(pcalccfg+uiCalcCfg))			/* 预处理 */
                    *puc++=*pucContinue++;
                else
                {
                    aucSndBuf[2] |= 0x40;		/* 结束 */
                    break;
                }
            }
            if (EX_Send_Wait_Ack(aucSndBuf, puc-aucSndBuf))
            {
                /* 发送并等待应答 */
                taskDelay(SYS_SEC);
                goto reinit;
            }
        }
        else if (pucContinue<(uint8_t*)(pcalccfg+uiCalcCfg))
        {
            aucSndBuf[2]=iMsgSN & 0x0F;		/* 保留次数 */

            puc=aucSndBuf+4;
            while (puc<aucSndBuf+ETH_MAX_DAT)
            {
                if (pucContinue<(uint8_t*)(pcalccfg+uiCalcCfg))
                    *puc++=*pucContinue++;
                else
                {
                    aucSndBuf[2] |= 0x40;
                    break;
                }
            }
            if (EX_Send_Wait_Ack(aucSndBuf, puc-aucSndBuf))
            {
                taskDelay(SYS_SEC);
                goto reinit;
            }
        }
        else
            break;
    }

    LOG_Dbg_Msg("Init Ext-box AI OK.\n", 0, 0, 0, 0, 0, 0);

    pucContinue=(uint8_t*)adicfgExt_g;
    assert(sizeof(*adicfgExt_g)==6);

    /*20120227 sdm 增加召唤扩展机箱版本号报文，
    在初始化AI后DI前发本报文，已经考虑到了
    各个版本的兼容性，不管通信两端是否支持
    本报文都不会影响扩展机箱初始化的状态机制。*/
    for (iMsgSN=0; iMsgSN<20; iMsgSN++)
    {
        aucSndBuf[0]=0xFF;
        aucSndBuf[1]=0xFF;
        aucSndBuf[2]=0xC0;
        aucSndBuf[3]=0x0A;
        aucSndBuf[4]=0x00;
        aucSndBuf[5]=0x00;

        if (EX_Send_Wait_Ack(aucSndBuf, 6))
        {
            LOG_Dbg_Msg("Get ext version err.\n", 0, 0, 0, 0, 0, 0);
            taskDelay(SYS_SEC);
        }
        else
            break;
    }

    for (iMsgSN=0; ; iMsgSN++)
    {
        aucSndBuf[0]=0xFF;
        aucSndBuf[1]=0xFF;
        aucSndBuf[3]=0x04;

        if (!iMsgSN)
        {
            /* 第一次 */
            aucSndBuf[2]=0x80;

            aucSndBuf[4]=HI8(iExtDiNum_g);
            aucSndBuf[5]=LO8(iExtDiNum_g);

            puc=aucSndBuf+6;

            while (puc<aucSndBuf+ETH_MAX_DAT)
            {
                if (pucContinue<(uint8_t*)(adicfgExt_g+iExtDiNum_g))
                    *puc++=*pucContinue++;
                else
                {
                    aucSndBuf[2] |= 0x40;
                    break;
                }
            }
            if (EX_Send_Wait_Ack(aucSndBuf, puc-aucSndBuf))
            {
                LOG_Dbg_Msg("Send ext DI cfg err.\n", 0, 0, 0, 0, 0, 0);
                taskDelay(SYS_SEC);
                goto reinit;
            }
            else
            {
                LOG_Dbg_Msg("Send ext DI cfg ok.\n", 0, 0, 0, 0, 0, 0);
            }
        }
        else if (pucContinue<(uint8_t*)(adicfgExt_g+iExtDiNum_g))
        {
            aucSndBuf[2]=iMsgSN & 0x0F;

            puc=aucSndBuf+4;
            while (puc<aucSndBuf+ETH_MAX_DAT)
            {
                if (pucContinue<(uint8_t*)(adicfgExt_g+iExtDiNum_g))
                    *puc++=*pucContinue++;
                else
                {
                    aucSndBuf[2] |= 0x40;
                    break;
                }
            }
            if (EX_Send_Wait_Ack(aucSndBuf, puc-aucSndBuf))
            {
                taskDelay(SYS_SEC);
                goto reinit;
            }
            else
            {
                LOG_Dbg_Msg("Send ext DI cfg ok(2).\n", 0, 0, 0, 0, 0, 0);
            }
        }
        else
            break;
    }

    LOG_Dbg_Msg("Ext-box begins running.\n", 0, 0, 0, 0, 0, 0);

#ifdef EDP01_CA_OPT_BUILD		/* EDP01平台C-A版本，使用光CT，屏蔽本机采样，建立数据接收任务 */
    if(ExtRecvInit(pvAiMod) != EP_SUCCESS)
    {
        LOG_Dbg_Msg("Opt Sample Recv Task Init failure!\n", 0, 0, 0, 0, 0, 0);
        return  EP_CFG_ERR;
    }
#endif

    return EP_SUCCESS;
}

/* 发送扩展机箱通道系数
 * Para:
 *     uiLgcCh, 通道数.
 *     plgccfg, 通道配置.
 * Return:
 *     result.
*/
int UpdateExtAcCoff(u_int uiLgcCh, DSP_LGC_AI_CFG *plgccfg)
{
//     uint8_t aucEthPacket[ETH_BD_HEAD+ETH_MAX_DAT];

//     /* 数据地址 */
// #define aucSndBuf (aucEthPacket+ETH_BD_HEAD)

//     int i;
//     uint8_t *puc;
//     DSP_LGC_AI_CFG *plcfg;
//     int iLen;
//     int iLockKey;

//     /* 无通道配置则不处理 */
//     if (!aimodExt_g.iLgcNum)
//     {
//         return 0;
//     }

//     aucSndBuf[0]=0xFF;
//     aucSndBuf[1]=0xFF;
//     aucSndBuf[2]=0x80;		/* 起始帧 */
//     aucSndBuf[3]=0x05;				/* 下传AI变比系数 */


//     aucSndBuf[4]=0x0;
//     aucSndBuf[5]=0x0;
//     aucSndBuf[6]=0x0;

//     /* 个数限制 */
//     assert(uiLgcCh <= 255);
//     aucSndBuf[7]=uiLgcCh;

//     aucSndBuf[8]=0x0;
//     aucSndBuf[9]=0x0;

//     plcfg=plgccfg;

//     for (i=0, puc=aucSndBuf+10; i<uiLgcCh; i++, plcfg++)
//     {
//         assert(plcfg->ucHdCh);
//         *puc++=plcfg->ucHdCh-1;

//         /* ALERT: assume Big endian and sizeof(float)==4 here!!! */
//         *puc++=*(uint8_t*)&plcfg->fCoff;
//         *puc++=*((uint8_t*)&plcfg->fCoff+1);
//         *puc++=*((uint8_t*)&plcfg->fCoff+2);
//         *puc++=*((uint8_t*)&plcfg->fCoff+3);

//         *puc++=0;
//     }

//     aucSndBuf[2] |= 0x40;		/* 置结束标志 */


//     iLen = puc-aucSndBuf;

//     /* 数据发送 */
//     iLockKey = intLock();  /* 闭锁中断 */
//     i = ext_send_raw(aucSndBuf, iLen);
//     intUnlock(iLockKey);

//     /* 调用发送函数失败 */
//     if (i != iLen)
//     {
//         return -1;
//     }

//     return 0;
}

/* 设置发送标志
 * Para:
 *     iVal, 设置值.
 * Return:
 *     NONE.
 */
void SetExtSndFlag(int iVal)
{
    iAcCofBurstSendSts = iVal;
}

/* 获取发送标志
 * Para:
 *     NONE.
 * Return:
 *     当前标志.
 */
int GetExtSndFlag(void)
{
    return iAcCofBurstSendSts;
}

/***********************************************************************
* SetExtboxLanguageType - 设置扩展机箱语言类型
*
* RETURNS: 无
*
*/
void SetExtboxLanguageType(void)
{
    uint8_t aucEthPacket[ETH_BD_HEAD+ETH_MAX_DAT];
#define aucSndBuf (aucEthPacket+ETH_BD_HEAD)				/* 数据地址 */

    if (!(aimodExt_g.iLgcNum || iExtDiNum_g))
    {
        /* 不配置则不处理 */
        return;
    }

reinit:
    LOG_Dbg_Msg("Try to update the extend box AI language type.\n", 0, 0, 0, 0, 0, 0);

    aucSndBuf[0]=0xFF;
    aucSndBuf[1]=0xFF;
    aucSndBuf[2]=0x80;		/* 起始帧 */
    aucSndBuf[3]=0x06;				/* 下传语言类型 */


    aucSndBuf[4]=0x0;
    aucSndBuf[5]=0x0;
    aucSndBuf[6]=0x0;
    aucSndBuf[7]=0x0;
    aucSndBuf[8]=0x0;
    aucSndBuf[9]=0x0;
    aucSndBuf[10]=0x0;
    aucSndBuf[11]=0x0;

    if(ENG_MODE == 1)
    {
        aucSndBuf[12]=1;
    }
    else
    {
        aucSndBuf[12]=0;
    }

    aucSndBuf[13]=0x0;
    aucSndBuf[14]=0x0;
    aucSndBuf[15]=0x0;
    aucSndBuf[16]=0x0;

    aucSndBuf[2] |= 0x40;		/* 置结束标志 */

    if (EXBurstSendWaitAck(aucSndBuf, 17))
    {
        /* 发送并等待应答 */
        taskDelay(SYS_SEC);
        goto reinit;
    }
    LOG_Dbg_Msg("更改扩展机箱语言类型成功!\n", 0, 0, 0, 0, 0, 0);
}

/***********************************************************************
* EXBurstSendWaitAck - 瞬时发送时应答
*
* RETURNS: 无
*
*/
static int EXBurstSendWaitAck(
    uint8_t *pucSnd,
    int iLen
)
{
    // uint8_t *puc;
    // int i;
    // int iAck;
    // int iLockKey;

    // assert(pucSnd[1]!=0xFF || pucSnd[0]==0xFF);
    // assert(iLen>=4);

    // assert(pucSnd[2]==0xC0);		/* 开始帧和结束帧是同一帧 */

    // iBurstSendSts=-1;

    // iLockKey = intLock();  /* 闭锁中断 */
    // i=ext_send_raw(pucSnd, iLen);
    // intUnlock(iLockKey);

    // if (i!=iLen)
    // {
    //     LOG_Dbg_Msg("eth_send_raw %d bytes(msg=0x%02X) err, ret %d.\n",
    //                 iLen, pucSnd[3], i, 0, 0, 0);
    //     return -1;
    // }

    // iAck=0;
    // puc=NULL;


    // /* Try several times to read the sign. */
    // for (i=0; i<20; i++)
    // {
    //     taskDelay(SYS_SEC/5);
    //     if (iBurstSendSts == 1)
    //     {
    //         return 0;
    //     }
    // }

    // iBurstSendSts=1;

    // return -1;
}

static int EX_Send_Wait_Ack(uint8_t *pucSnd, int iLen)
{
    // uint8_t *pucRcv;
    // uint8_t *puc;
    // int i;
    // int iAck;
    // int iRcv;
    // int iLockKey;

    // assert(pucSnd[1]!=0xFF || pucSnd[0]==0xFF);
    // assert(iLen>=4);

    // assert(pucSnd[2]==0xC0);

    // /* #ifndef EDP_DEBUG */

    // iLockKey = intLock();  /* 闭锁中断 */
    // i=ext_send_raw(pucSnd, iLen);
    // intUnlock(iLockKey);
    // /* #endif */

    // if (i!=iLen)
    // {
    //     LOG_Dbg_Msg("eth_send_raw %d bytes(msg=0x%02X) err, ret %d.\n",
    //                 iLen, pucSnd[3], i, 0, 0, 0);
    //     return -1;
    // }

    // iAck=0;
    // puc=NULL;
    // /* Try several times to flush receive buffer and get the last one. */
    // for (i=0; i<20; i++)
    // {
    //     /* #ifndef EDP_DEBUG */
    //     iRcv=ext_recv_raw(&pucRcv, SYS_SEC/5);
    //     /* #endif */
    //     if (iRcv>0)
    //     {
    //         iAck=iRcv;
    //         puc=pucRcv;

    //         if (iAck==iLen && puc[0]==pucSnd[1] && puc[1]==pucSnd[0] &&
    //                 puc[2]==pucSnd[2] && puc[3]==(pucSnd[3]^0x80) &&
    //                 !memcmp(puc+4, pucSnd+4, iLen-4))
    //             return 0;
    //         else if (iAck>10 && pucRcv[3]==0x4A)
    //         {
    //             extern UNITE_VER_INFO UnVerInfo_g;
    //             uint8_t ucArraySize;
    //             uint8_t ucLen;
    //             uint8_t ucPos;

    //             /*{
    //                 int j;
    //                 for (j=0;j<iRcv;j++)
    //                 {
    //                     if (j%8==0)
    //                         printf("\n ");
    //                     printf("%02X ",pucRcv[j]);
    //                 }
    //                 printf("\n ");
    //             }*/

    //             ucPos=4;
    //             ucArraySize=sizeof(acExtHwVer);
    //             if (pucRcv[ucPos]>ucArraySize)
    //                 ucLen=ucArraySize;
    //             else
    //                 ucLen=pucRcv[ucPos];
    //             strncpy(acExtHwVer,pucRcv+ucPos+1,ucLen);
    //             ucPos=ucPos+pucRcv[ucPos]+1;

    //             ucArraySize=sizeof(UnVerInfo_g.acExtBspVer);
    //             if (pucRcv[ucPos]>ucArraySize)
    //                 ucLen=ucArraySize;
    //             else
    //                 ucLen=pucRcv[ucPos];
    //             strncpy(UnVerInfo_g.acExtBspVer,pucRcv+ucPos+1,ucLen);
    //             ucPos=ucPos+pucRcv[ucPos]+1;

    //             ucArraySize=sizeof(UnVerInfo_g.acExtEdpVer);
    //             if (pucRcv[ucPos]>ucArraySize)
    //                 ucLen=ucArraySize;
    //             else
    //                 ucLen=pucRcv[ucPos];
    //             strncpy(UnVerInfo_g.acExtEdpVer,pucRcv+ucPos+1,ucLen);
    //             ucPos=ucPos+pucRcv[ucPos]+1;

    //             UnVerInfo_g.unExtEdpCrc=pucRcv[ucPos]+((uint16_t)pucRcv[ucPos+1]*0x100);
    //             ucPos+=2;

    //             ucExtIoSum=pucRcv[ucPos++];
    //             if (iRcv-ucPos<ucExtIoSum*5)
    //                 ucExtIoSum=0;
    //             memcpy(aExtSubModInfo,pucRcv+ucPos,sizeof(uint8_t)*ucExtIoSum*5);
    //             /*memset(UnVerInfo_g.aucExtIOVer,0,sizeof(UnVerInfo_g.aucExtIOVer));
    //             for (j=0;(j<ucExtIoSum && j<MAX_MOD_NUM);j++)
    //             {
    //                 uint8_t ucIOIndex;
    //                 uint16_t unIOVer;
    //                 ucPos+=1;
    //                 ucIOIndex=pucRcv[ucPos];
    //                 ucPos+=2;
    //                 unIOVer=pucRcv[ucPos]+((uint16_t)pucRcv[ucPos+1]*0x100);
    //                 ucPos+=2;
    //                 sprintf(UnVerInfo_g.aucExtIOVer[ucIOIndex],"%04X",unIOVer);
    //             }*/
    //             return 0;
    //         }
    //     }
    //     else
    //         return -1;
    // }

    return -3;
}

/***********************************************************************
* EX_Rd_Data - 读取扩展机箱实时AI/DI数据
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS EX_Rd_Data(void)
{
    static int iFrmSN=0x40;
    static uint32_t *pulLgcDb;
    static uint32_t *pulCalcDb;
    static uint32_t *pulDiSts;
    static u_int uiLeftLgc;
    static u_int uiLeftCalc;
    static u_int uiLeftDi;

    uint8_t *pucRcv;
    uint8_t *puc;
    int iRcv;
    uint32_t ulSmplClk;
    uint32_t *pulSrc;
    uint32_t *pulEnd;
    uint32_t *pulDst;
    int  i;
    int j;
    DSP_LGC_AI_CFG *plcfg = NULL; /* 逻辑通道配置 */
    DSP_CALC_AI_CFG *pcalccfgl = NULL;  /* 预处理通道配置 */
    float *pfLgcVal = NULL;  /* 逻辑通道计算结果存储 */
    float *pfCalcVal = NULL;  /* 预处理通道计算结果存储 */

#ifndef NO_DBL_BUF
    uint32_t *pulTemp;
    uint32_t ul;
    float *pfTempLgcVal;  /* 逻辑通道计算结果双缓冲存储 */
    float *pfTempCalcVal = NULL;  /* 预处理通道计算结果双缓冲存储 */
#endif

//     {
//         uint8_t aucEthPacket[ETH_BD_HEAD+ETH_MAX_DAT];
// #define aucSndBuf (aucEthPacket+ETH_BD_HEAD)
//         int iLockKey;
//         static uint8_t ucThreshhold=3;
//         static uint8_t ucLastMin=0;
//         EP_DATE_TIME dttmNow;
//         aucSndBuf[0]=0xFF;
//         aucSndBuf[1]=0xFF;
//         aucSndBuf[2]=0xC0;
//         aucSndBuf[3]=0x0B;
//         TM_Get_Sys_Time(&dttmNow);
//         if ((dttmNow.ucMinute+60-ucLastMin)%60>=ucThreshhold)
//         {
//             ucThreshhold=20;
//             aucSndBuf[4]=dttmNow.unYear-2000;
//             aucSndBuf[5]=dttmNow.ucMonth;
//             aucSndBuf[6]=dttmNow.ucDate;
//             aucSndBuf[7]=dttmNow.ucHour;
//             aucSndBuf[8]=dttmNow.ucMinute;
//             aucSndBuf[9]=dttmNow.ucSec;
//             aucSndBuf[10]=LO8(dttmNow.unMSEL);
//             aucSndBuf[11]=HI8(dttmNow.unMSEL);
//             ucLastMin=dttmNow.ucMinute;
//             iLockKey = intLock();
//             i=ext_send_raw(aucSndBuf, 12);
//             intUnlock(iLockKey);
//             //LOG_Dbg_Msg("sdmtime:%d,%d,%d,%d,%d,%d.\n", aucSndBuf[4], aucSndBuf[5], aucSndBuf[6],aucSndBuf[7], aucSndBuf[8], aucSndBuf[9]);
//         }
//     }
//     /* #ifndef EDP_DEBUG */
//     iRcv=ext_recv_raw(&pucRcv, NO_WAIT);
//     /* #endif */

//     if (iRcv>=4)
//     {
//         puc=pucRcv;
//         if (puc[3]==0x42)
//         {
//             if(!(!((uint32_t)puc%4) && !(iRcv%4)))
//             {
//                 static uint32_t ulCnt=0;
//                 ulCnt++;
//                 /* if(ulCnt%0x3ff == 1) */
//                 {
//                     if(ENG_MODE ==1)
//                         LOG_Write(LOG_KERNEL, "alarm: The address and the length of  data received error.\n", NULL);
//                     else if(ENG_MODE ==0 )
//                         LOG_Write(LOG_KERNEL, "告警: 接收到的数据长度出错.\n", NULL);
//                     logMsg("接收地址和接收数据长度出错!puc=%x iRcv=%d\n", (int)puc, iRcv, 0, 0, 0, 0);
//                 }
//             }

//             assert(!((uint32_t)puc%4) && !(iRcv%4));

//             if (puc[2] & 0x80)
//             {
//                 /* SN in the same application message begins from 0.
//                  * First frame contains sample clk. */
//                 assert(!(puc[2] & 0x0F) && iRcv>8);

//                 if (!(iFrmSN & 0x40) || uiLeftLgc || uiLeftCalc || uiLeftDi)
//                     goto comerr;
//             }
//             else
//             {
//                 if ((puc[2] & 0x0F)!=((iFrmSN+1) & 0x0F))
//                     goto comerr;
//             }

//             iFrmSN=puc[2];

//             /* To use ++p with maximal optimization on PPC, all the
//              * pointer is decreased first. */
//             pulSrc=(uint32_t*)(puc+4)-1;
//             iRcv-=4;

//             if (!iRcv)
//                 goto msgend;

//             if (!uiLeftLgc && !uiLeftCalc && !uiLeftDi)
//             {
// nextpts:
//                 ulSmplClk=*++pulSrc;
//                 iRcv-=4;

//                 /* To use ++p with maximal optimization on PPC, all the
//                  * pointer is decreased first. */
//                 pulLgcDb=(uint32_t*)RD_AI_Dat_P(pvExtAiMod_g,
//                                                 ulSmplClk, (COMPLEX**)&pulCalcDb, NULL)-1;
//                 pulCalcDb--;

//                 /* To use ++p with maximal optimization on PPC, all the
//                  * pointer is decreased first. */
//                 pulDiSts=aulExtDiSts_g-1;

//                 uiLeftLgc=uiExtLgcCh_g;
//                 uiLeftCalc=uiExtCalcCh_g;
//                 uiLeftDi=uiTxDiLong_g;
//                 /* 更新扩展机箱系数 */
//                 if (bExtBoxCoffUpdate)
//                 {
//                     /* 计数同步 */
//                     if (ulSmplClk == usCoffUpdateCount)
//                     {
//                         bExtBoxCoffUpdate = FALSE;
//                         /* 更新系数 */
//                         DspExtBoxCoeRunUpdate();
//                     }
//                 }
//             }

//             if (!iRcv)
//                 goto msgend;

//             if (uiLeftLgc)
//             {
//                 if (4*uiLeftLgc<=iRcv)
//                 {
//                     pulEnd=pulSrc+uiLeftLgc;
//                     iRcv-=4*uiLeftLgc;
//                     uiLeftLgc=0;
//                 }
//                 else
//                 {
//                     {
//                         static uint32_t ulCnt=0;
//                         ulCnt++;
//                         /* if(ulCnt%0x3ff == 1) */
//                         {
//                             if(ENG_MODE ==1)
//                                 LOG_Write(LOG_KERNEL, "alarm: The length of the data received is too short.\n", NULL);
//                             else if(ENG_MODE ==0)
//                                 LOG_Write(LOG_KERNEL, "告警: 接收到的数据太短.\n", NULL);
//                             logMsg("接收帧过短!uiLeftLgc=%d iRcv=%d ulCnt=%d\n", uiLeftLgc, iRcv, ulCnt, 0, 0, 0);
//                         }
//                     }
//                     assert(FALSE);/*目前不可能出现,没那么大帧,张云   */
//                     pulEnd=pulSrc+iRcv/4;
//                     uiLeftLgc-=iRcv/4;
//                     iRcv=0;
//                 }

//                 pulDst=pulLgcDb;

//                 pfLgcVal = (float *)pulDst;  /* 浮点存储地址 */
//                 plcfg = pdspl_cfg_ext_g;   /* 逻辑通道配置 */
// #ifndef NO_DBL_BUF
//                 pulTemp=(uint32_t*)((uint8_t*)pulDst-lgcaidb_g.ulBufBytes);
//                 pfTempLgcVal = (float *)pulTemp;  /* 浮点双缓冲存储地址 */

//                 for(i=0; i<uiExtLgcCh_g_8; i++)
//                 {
//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;

//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;

//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;

//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;

//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;

//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;

//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;

//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;
//                 }
//                 while (pulSrc<pulEnd)
//                 {
//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;
//                 }

//                 /* 乘以通道系数 */
//                 for (i = 0; i<uiExtLgcCh_g; i++, plcfg++)
//                 {
//                     /* 所有通道 */
//                     ++pfLgcVal;
//                     if ((plcfg->ucFiltNum == 0) || (plcfg->ucFiltNum == 2))
//                     {
//                         /* 通用瞬时值和三取一 */
//                         *pfLgcVal *= plcfg->fCoff;
//                         *++pfTempLgcVal = *pfLgcVal;
//                     }
//                 }

// #else
//                 for(i=0; i<uiExtLgcCh_g_8; i++)
//                 {
//                     *++pulDst=*++pulSrc;
//                     *++pulDst=*++pulSrc;
//                     *++pulDst=*++pulSrc;
//                     *++pulDst=*++pulSrc;
//                     *++pulDst=*++pulSrc;
//                     *++pulDst=*++pulSrc;
//                     *++pulDst=*++pulSrc;
//                     *++pulDst=*++pulSrc;
//                 }
//                 while (pulSrc<pulEnd)
//                     *++pulDst=*++pulSrc;

//                 /* 乘以通道系数 */
//                 for (i = 0; i<uiExtLgcCh_g; i++, plcfg++)
//                 {
//                     /* 所有通道 */
//                     ++pfLgcVal;
//                     if ((plcfg->ucFiltNum == 0) || (plcfg->ucFiltNum == 2))
//                     {
//                         /* 通用瞬时值和三取一 */
//                         *pfLgcVal *= plcfg->fCoff;
//                     }
//                 }

// #endif

//                 if (!iRcv)
//                 {
//                     pulLgcDb=pulDst;
//                     goto msgend;
//                 }
//             }

//             /* Continue copy CalcAI. */
//             if (uiLeftCalc)
//             {
//                 if (8*uiLeftCalc<=iRcv)
//                 {
//                     pulEnd=pulSrc+2*uiLeftCalc;
//                     iRcv-=8*uiLeftCalc;
//                     uiLeftCalc=0;
//                 }
//                 else
//                 {
//                     assert(FALSE);/*目前不可能出现,没那么大帧,张云   */
//                     assert(!(iRcv%8));
//                     pulEnd=pulSrc+iRcv/8;
//                     uiLeftCalc-=iRcv/8;
//                     iRcv=0;
//                 }

//                 pulDst=pulCalcDb;

//                 pfCalcVal = (float *)pulCalcDb;
//                 plcfg = pdspl_cfg_ext_g;  /* 逻辑通道 */
//                 pcalccfgl = pdspc_cfg_ext_g;  /* 预处理通道 */

// #ifndef NO_DBL_BUF
//                 pulTemp=(uint32_t*)
//                         ((uint8_t*)pulDst-calcaidb_g.ulBufBytes);

//                 pfTempCalcVal = (float *)pulTemp; /* 双缓冲 */

//                 for(i=0; i<uiExtCalcCh_g_4; i++)
//                 {
//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;
//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;

//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;
//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;

//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;
//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;

//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;
//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;


//                 }
//                 while (pulSrc<pulEnd)
//                 {
//                     ul=*++pulSrc;
//                     *++pulDst=ul;
//                     *++pulTemp=ul;
//                 }

//                 /* 预处理结果乘以通道系数 */
//                 for (i = 0; i<uiCalcCfg_s; i++, pcalccfgl++)
//                 {
//                     if (pcalccfgl[i].ucArithParm & 0x20)
//                     {
//                         /* 预处理上传，且为2个浮点数 */
//                         if ((pcalccfgl->ucArithNum == 3) || (pcalccfgl->ucArithNum == 5))
//                         {
//                             /* 递归傅氏,或递归差分傅氏 */
//                             for (j = 0; j<pcalccfgl->ucChNum; j++)
//                             {
//                                 /* 包含通道数量 */

//                                 /* 实部 */
//                                 ++pfCalcVal;
//                                 *pfCalcVal *= (plcfg+pcalccfgl->ucBgnLgcCh+j)->fCoff;
//                                 *++pfTempCalcVal = *pfCalcVal;

//                                 /* 虚部 */
//                                 ++pfCalcVal;
//                                 *pfCalcVal *= (plcfg+pcalccfgl->ucBgnLgcCh+j)->fCoff;
//                                 *++pfTempCalcVal = *pfCalcVal;
//                             }
//                         }
//                         else if (pcalccfgl->ucArithNum == 4)
//                         {
//                             /* 辐角值 */
//                             for (j = 0; j<pcalccfgl->ucChNum; j++)
//                             {
//                                 /* 包含通道数量 */

//                                 /* 幅值 */
//                                 ++pfCalcVal;
//                                 *pfCalcVal *= (plcfg+pcalccfgl->ucBgnLgcCh+j)->fCoff;
//                                 *++pfTempCalcVal = *pfCalcVal;

//                                 /* 相位 */
//                                 ++pfCalcVal;
//                                 ++pfTempCalcVal;
//                             }
//                         }
//                         else
//                         {
//                             assert(FALSE);
//                         }
//                     }
//                 }

// #else

//                 for(i=0; i<uiExtCalcCh_g_4; i++)
//                 {
//                     *++pulDst=*++pulSrc;
//                     *++pulDst=*++pulSrc;

//                     *++pulDst=*++pulSrc;
//                     *++pulDst=*++pulSrc;

//                     *++pulDst=*++pulSrc;
//                     *++pulDst=*++pulSrc;

//                     *++pulDst=*++pulSrc;
//                     *++pulDst=*++pulSrc;

//                 }
//                 while (pulSrc<pulEnd)
//                     *++pulDst=*++pulSrc;

//                 /* 预处理结果乘以通道系数 */
//                 for (i = 0; i<uiCalcCfg_s; i++, pcalccfgl++)
//                 {
//                     if (pcalccfgl[i].ucArithParm & 0x20)
//                     {
//                         /* 预处理上传，且为2个浮点数 */
//                         if ((pcalccfgl->ucArithNum == 3) || (pcalccfgl->ucArithNum == 5))
//                         {
//                             /* 递归傅氏,或递归差分傅氏 */
//                             for (j = 0; j<pcalccfgl->ucChNum; j++)
//                             {
//                                 /* 包含通道数量 */

//                                 /* 实部 */
//                                 ++pfCalcVal;
//                                 *pfCalcVal *= (plcfg+pcalccfgl->ucBgnLgcCh+j)->fCoff;

//                                 /* 虚部 */
//                                 ++pfCalcVal;
//                                 *pfCalcVal *= (plcfg+pcalccfgl->ucBgnLgcCh+j)->fCoff;
//                             }
//                         }
//                         else if (pcalccfgl->ucArithNum == 4)
//                         {
//                             /* 辐角值 */
//                             for (j = 0; j<pcalccfgl->ucChNum; j++)
//                             {
//                                 /* 包含通道数量 */

//                                 /* 幅值 */
//                                 ++pfCalcVal;
//                                 *pfCalcVal *= (plcfg+pcalccfgl->ucBgnLgcCh+j)->fCoff;

//                                 /* 相位 */
//                                 ++pfCalcVal;
//                             }
//                         }
//                         else
//                         {
//                             assert(FALSE);
//                         }
//                     }
//                 }

// #endif

//                 if (!iRcv)
//                 {
//                     pulCalcDb=pulDst;
//                     goto msgend;
//                 }
//             }

//             /* Continue deal with DI. */
//             if (uiLeftDi)
//             {
//                 if (4*uiLeftDi<=iRcv)
//                 {
//                     pulEnd=pulSrc+uiLeftDi;
//                     iRcv-=4*uiLeftDi;
//                     uiLeftDi=0;
//                 }
//                 else
//                 {
//                     pulEnd=pulSrc+iRcv/4;
//                     uiLeftDi-=iRcv/4;
//                     iRcv=0;
//                 }

//                 pulDst=pulDiSts;

//                 while (pulSrc<pulEnd)
//                     *++pulDst=*++pulSrc;

//                 if (!iRcv)              /* Receive data not enough. */
//                 {
//                     pulDiSts=pulDst;    /* Save for next time continue. */
//                     goto msgend;
//                 }
//             }

//             if (iRcv)
//             {
//                 assert(FALSE);      /* No possible of multi-frame now. */
//                 goto nextpts;
//             }

// msgend:
//             assert(!(puc[2] & 0x40) ||
//                    (!uiLeftLgc && !uiLeftCalc && !uiLeftDi));

//             return EP_SUCCESS;
//         }
//         else if (puc[3]==0x41)
//         {
//             assert(puc[2]==0xC0);
//             ER_Set_Err(puc[4]*0x100+puc[5], puc[6]*0x100+puc[7], puc+8, 0, 0);

//             return EP_HARD_ERR;
//         }
//         else if (puc[3] == 0x85)
//         {
//             assert(puc[2]==0xc0);

//             if(CmpExtAcCoff(puc+10) == EP_ERROR)
//             {
//                 goto comerr;
//             }

//             iAcCofBurstSendSts = 1;

//             return EP_SUCCESS;
//         }
//         else if (puc[3] == 0x86)
//         {
//             assert(puc[2]==0xc0);

//             if((int32_t)puc[12] != ENG_MODE)
//             {
//                 goto comerr;
//             }

//             iBurstSendSts=1;

//             return EP_SUCCESS;
//         }
//     }
//     else if (!iRcv)
//         return EP_TIMEOUT;

//     /* iRcv<0 or message format error. */
// comerr:

//     if(ENG_MODE == 0)
//     {
//         ER_Set_Err(EV_EXT_COM_ALARM, ER_LOCK | ER_ALARM | ER_REPORT,
//                    "", 0, 0);


//     }
//     else if(ENG_MODE == 1)
//     {
//         ER_Set_Err(EV_EXT_COM_ALARM, ER_LOCK | ER_ALARM | ER_REPORT,
//                    "", 0, 0);
//     }
    return EP_COM_ERR;
}

/***********************************************************************
* ExtRecvCmd - 光CT采样接收任务
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS ExtRecvCmd(
    RD_AI_MOD *pAiMod, 			/* 扩展机箱模件信息 */
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
)
{


    static int iFrmSN=0x40;
    static uint32_t *pulLgcDb;
    static uint32_t *pulCalcDb;
    static uint32_t *pulDiSts;
    static u_int uiLeftLgc;
    static u_int uiLeftCalc;
    static u_int uiLeftDi;

    uint8_t *pucRcv;
    uint8_t *puc;
    int iRcv;
    uint32_t ulSmplClk;
    uint32_t *pulSrc;
    uint32_t *pulEnd;
    uint32_t *pulDst;
    int  i;

#ifndef NO_DBL_BUF
    uint32_t *pulTemp;
    uint32_t ul;
#endif

    while (1)
    {

OPT_CT_RECV_NEW_FRAME:

        // iRcv=ext_recv_raw(&pucRcv, 5*SYS_TICK);
        iRcv=0;
        {
            static uint32_t ulCnt=0;
            ulCnt++;
            if(ulCnt%0x3fff == 1)
            {
                logMsg("iRcv=%d\n", iRcv, 0, 0, 0, 0, 0);
            }
        }

        if (iRcv>=4)
        {
            puc=pucRcv;
            if (puc[3]==0x42)
            {
                assert(!((uint32_t)puc%4) && !(iRcv%4));

                if (puc[2] & 0x80)
                {
                    /* SN in the same application message begins from 0.
                     * First frame contains sample clk. */
                    assert(!(puc[2] & 0x0F) && iRcv>8);

                    if (!(iFrmSN & 0x40) || uiLeftLgc || uiLeftCalc || uiLeftDi)
                        goto comerr;
                }
                else
                {
                    if ((puc[2] & 0x0F)!=((iFrmSN+1) & 0x0F))
                        goto comerr;
                }

                iFrmSN=puc[2];

                /* To use ++p with maximal optimization on PPC, all the
                 * pointer is decreased first. */
                pulSrc=(uint32_t*)(puc+4)-1;
                iRcv-=4;

                if (!iRcv)
                    goto msgend;

                if (!uiLeftLgc && !uiLeftCalc && !uiLeftDi)
                {
nextpts:
                    ulSmplClk=*++pulSrc;
                    iRcv-=4;

#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)
                    POLE_Read_AI_Data(ulSmplClk);  /*2007-4-3日　张云，添加同杆并架的数据刷新，必须在本机read_AI_Data之前被调用  */
                    HDL_Read_AI_Data(ulSmplClk);   /*2007-4-4日　张云，添加智能操作箱的数据刷新，必须在本机read_AI_Data之前被调用  */
#endif

                    /* To use ++p with maximal optimization on PPC, all the
                     * pointer is decreased first. */
                    pulLgcDb=(uint32_t*)RD_AI_Dat_P(pAiMod,
                                                    ulSmplClk, (COMPLEX**)&pulCalcDb, NULL)-1;

                    ulExtAccessCounter_g++;		/* If the task is active, it wil be accumulated. */

                    pulCalcDb--;

                    /* To use ++p with maximal optimization on PPC, all the
                     * pointer is decreased first. */
                    pulDiSts=aulExtDiSts_g-1;

                    uiLeftLgc=uiExtLgcCh_g;
                    uiLeftCalc=uiExtCalcCh_g;
                    uiLeftDi=uiTxDiLong_g;
                }

                if (!iRcv)
                    goto msgend;

                if (uiLeftLgc)
                {
                    if (4*uiLeftLgc<=iRcv)
                    {
                        pulEnd=pulSrc+uiLeftLgc;
                        iRcv-=4*uiLeftLgc;
                        uiLeftLgc=0;
                    }
                    else
                    {
                        assert(FALSE);/*目前不可能出现,没那么大帧,张云   */
                        pulEnd=pulSrc+iRcv/4;
                        uiLeftLgc-=iRcv/4;
                        iRcv=0;
                    }

                    pulDst=pulLgcDb;
#ifndef NO_DBL_BUF
                    pulTemp=(uint32_t*)((uint8_t*)pulDst-lgcaidb_g.ulBufBytes);

                    for(i=0; i<uiExtLgcCh_g_8; i++)
                    {
                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;

                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;

                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;

                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;

                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;

                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;

                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;

                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;
                    }
                    while (pulSrc<pulEnd)
                    {
                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;
                    }

#else
                    for(i=0; i<uiExtLgcCh_g_8; i++)
                    {
                        *++pulDst=*++pulSrc;
                        *++pulDst=*++pulSrc;
                        *++pulDst=*++pulSrc;
                        *++pulDst=*++pulSrc;
                        *++pulDst=*++pulSrc;
                        *++pulDst=*++pulSrc;
                        *++pulDst=*++pulSrc;
                        *++pulDst=*++pulSrc;
                    }
                    while (pulSrc<pulEnd)
                        *++pulDst=*++pulSrc;

#endif

                    if (!iRcv)
                    {
                        pulLgcDb=pulDst;
                        goto msgend;
                    }
                }

                /* Continue copy CalcAI. */
                if (uiLeftCalc)
                {
                    if (8*uiLeftCalc<=iRcv)
                    {
                        pulEnd=pulSrc+2*uiLeftCalc;
                        iRcv-=8*uiLeftCalc;
                        uiLeftCalc=0;
                    }
                    else
                    {
                        assert(FALSE);/*目前不可能出现,没那么大帧,张云   */
                        assert(!(iRcv%8));
                        pulEnd=pulSrc+iRcv/8;
                        uiLeftCalc-=iRcv/8;
                        iRcv=0;
                    }

                    pulDst=pulCalcDb;
#ifndef NO_DBL_BUF
                    pulTemp=(uint32_t*)
                            ((uint8_t*)pulDst-calcaidb_g.ulBufBytes);

                    for(i=0; i<uiExtCalcCh_g_4; i++)
                    {
                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;
                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;

                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;
                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;

                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;
                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;

                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;
                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;


                    }
                    while (pulSrc<pulEnd)
                    {
                        ul=*++pulSrc;
                        *++pulDst=ul;
                        *++pulTemp=ul;
                    }

#else

                    for(i=0; i<uiExtCalcCh_g_4; i++)
                    {
                        *++pulDst=*++pulSrc;
                        *++pulDst=*++pulSrc;

                        *++pulDst=*++pulSrc;
                        *++pulDst=*++pulSrc;

                        *++pulDst=*++pulSrc;
                        *++pulDst=*++pulSrc;

                        *++pulDst=*++pulSrc;
                        *++pulDst=*++pulSrc;

                    }
                    while (pulSrc<pulEnd)
                        *++pulDst=*++pulSrc;

#endif

                    if (!iRcv)
                    {
                        pulCalcDb=pulDst;
                        goto msgend;
                    }
                }

                /* Continue deal with DI. */
                if (uiLeftDi)
                {
                    if (4*uiLeftDi<=iRcv)
                    {
                        pulEnd=pulSrc+uiLeftDi;
                        iRcv-=4*uiLeftDi;
                        uiLeftDi=0;
                    }
                    else
                    {
                        pulEnd=pulSrc+iRcv/4;
                        uiLeftDi-=iRcv/4;
                        iRcv=0;
                    }

                    pulDst=pulDiSts;

                    while (pulSrc<pulEnd)
                        *++pulDst=*++pulSrc;

                    if (!iRcv)              /* Receive data not enough. */
                    {
                        pulDiSts=pulDst;    /* Save for next time continue. */
                        goto msgend;
                    }
                }

                if (iRcv)
                {
                    assert(FALSE);       /* No possible of multi-frame now. */
                    goto nextpts;
                }

msgend:    /*若接收到正确的帧  */
                assert(!(puc[2] & 0x40) ||
                       (!uiLeftLgc && !uiLeftCalc && !uiLeftDi));

                /*针对光纵，添加，修改了，2006－5－21  */
                RD_End_Ai_Wr(pAiMod);

                goto  OPT_CT_RECV_NEW_FRAME ;
            }
            else if (puc[3]==0x41)
            {
                /*若接收到异常信息帧  */
                assert(puc[2]==0xC0);
                ER_Set_Err(puc[4]*0x100+puc[5], puc[6]*0x100+puc[7], puc+8, 0, 0);

                goto  OPT_CT_RECV_NEW_FRAME;
            }
            else if (puc[3] == 0x85)
            {
                assert(puc[2]==0xc0);

                if(CmpExtAcCoff(puc+10) == EP_ERROR)
                {
                    goto comerr;
                }

                iAcCofBurstSendSts = 1;

                return EP_SUCCESS;
            }
            else if (puc[3] == 0x86)
            {
                assert(puc[2]==0xc0);

                if((int32_t)puc[12] != ENG_MODE)
                {
                    goto comerr;
                }

                iBurstSendSts=1;

                return EP_SUCCESS;
            }
        }
        else if (!iRcv)/* 若没接收到信息 */
            goto  OPT_CT_RECV_NEW_FRAME ;

        /* iRcv<0 or message format error. */
comerr:  /*若接收信息格式不对  */
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_EXT_COM_ALARM, ER_LOCK | ER_ALARM | ER_REPORT,
                       "数据接收异常\n",0, 0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_EXT_COM_ALARM, ER_LOCK | ER_ALARM | ER_REPORT,
                       "Data receive error\n", 0, 0);
        }
        goto  OPT_CT_RECV_NEW_FRAME;


    }/*while结束  */

}

/***********************************************************************
* GetExtRecvTaskStatus - 判定光CT采样通信任务状态
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL GetExtRecvTaskStatus()
{
    /*获得ExtRecvTask的状态,若正常,则返回真,否则,返回假  */
    static  char  strTaskStatus[128];
    if(taskIdVerify(nExtRecvTaskID_g)==ERROR)
    {
        /*首先判定该任务是否有效  */
        return   FALSE;
    }
    taskStatusString(nExtRecvTaskID_g,strTaskStatus);
    if(strcmp(strTaskStatus,"SUSPEND")==0
            ||strcmp(strTaskStatus,"DELAY+S")==0
            ||strcmp(strTaskStatus,"PEND+S")==0
            ||strcmp(strTaskStatus,"PEND+S+T")==0
            ||strcmp(strTaskStatus,"SUSPEND+I")==0
            ||strcmp(strTaskStatus,"DELAY+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+T+I")==0
            ||strcmp(strTaskStatus,"DEAD")==0)
    {
        return   FALSE;
    }
    else
    {
        return   TRUE;
    }
}