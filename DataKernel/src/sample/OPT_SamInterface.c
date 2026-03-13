/* OPT_SamInterface.c - subroutine library for interface between OPT module and sampling module  */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 27mar06, dy realize the interface to sampling module.
01a, 8feb06, zy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for interface between OPT module and sampling module.
INCLUDES: OPT_SamInterface.h
*/

/* includes */

#include "vxWorks.h"
#include <intLib.h>
#include <taskLib.h>
#include "adc.h"
#include "dspai.h"
// #include <drv/intrCtl/m8260IntrCtl.h>
// #include <drv/parallel/m8260IOPort.h>
// #include "target.h"
// #include "sbcm8260Cpm.h"
// #include "sbcm8260Siu.h"
// #include "Adc.h"

#include <semLib.h>

#include "OPT_SamInterface.h"

/* globals */

OPT_AI_MOD OptAIMod_g;
BOOL OPTAD_flag = FALSE; 		/* 光纵采样允许标志 */
BOOL Adjust_flag = FALSE; 	/* 调整允许标志 */
BOOL Send_flag = FALSE; 			/* 启动发送标志 */
int16_t OptAiBuf[OPT_BUF_LENGTH]; 			/* OPT发送缓冲区 */
float fOptAiBuf[OPT_BUF_LENGTH]; 		/* OPT发送缓冲区，浮点型 */
int16_t tmpOptBuf[2*OPT_BUF_LENGTH]; 			/* 采样整型临时缓冲区 */
float ftmpOptBuf[2*OPT_BUF_LENGTH];  /* 采样整型临时缓冲区, 浮点型 */
int16_t *ptmpOptBuf;
float *pftmpOptBuf;   		/* 浮点型 */
int TotalNumofTrans;
uint8_t **pucOptAoDataByteBaseSamp_g;       /* 光纵AI来源的AO发送数据基址 */

/* 获取扫描任务最小扫描周期点数.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern uint16_t SC_GetTaskMinPeriod (void);

/* global functions */

/***********************************************************************
* SetTimer2 -设置定时器定时时间
*
* RETURNS: 无
*
*/
void SetTimer2(
    uint32_t SamRate		/* 采样速率 */
);

/* functions */

/* Initailize AO in virtual box, realized by sampling module, called by OPT module.
 * Para:
 *     uiTxPts, points number every time by OPT.
 *     iOptAoCh, number of AO in this virtual box.
 *     pOptAoCfg, pointer to the AO configuration.
 *     iOptAoDataByteLen, the length of buffer for AO from AI.
 *     ppucRtOptAoDataByteBase, the base address of buffer for AO from AI.
 * Return:
 *     EP_SUCCESS, EP_BUF_ERR, EP_COM_ERR.
 */
EP_STATUS Init_OptBox_AO(u_int uiTxPts, int iOptAoCh, OPT_AO_CFG *pOptAoCfg, int iOptAoDataByteLen, uint8_t **ppucRtOptAoDataByteBase)
{
    uint16_t i,j;
    int16_t ntmp;
    OPT_AO_CFG *tmppOptAoCfg;
    OPT_AI_CFG tmpAI_CFG;
    EP_STATUS stsRet;

    stsRet = EP_SUCCESS;

    assert(iOptAoCh<MAX_OPT_AO_NUM);

    OptAIMod_g.uiTxPts = uiTxPts;
    assert (OptAIMod_g.uiTxPts == DspHandle.ucProcessInterval);  /* 发送间隔与配置一致 */
    assert (OptAIMod_g.uiTxPts == SC_GetTaskMinPeriod());  /* 逻辑图扫描间隔与配置一致 */

    OptAIMod_g.iOptAoDataByteLen = iOptAoDataByteLen;

    tmppOptAoCfg = pOptAoCfg;
    OptAIMod_g.AINum = 0;
    for(i=0; i<iOptAoCh; i++)
    {
        if(tmppOptAoCfg->iAOSrcType == 1)
        {
            OptAIMod_g.aOptBoxAiCfg_g[OptAIMod_g.AINum].ucAOHdCh = tmppOptAoCfg->ucAOHdCh;
            OptAIMod_g.aOptBoxAiCfg_g[OptAIMod_g.AINum].ucSrcAIHdCh = tmppOptAoCfg->ucSrcAIHdCh;
            for(j=0; j<iHwAiChNum_g; j++)
            {
                /* 从物理通道配置中寻找通道系数 */
                if(OptAIMod_g.aOptBoxAiCfg_g[OptAIMod_g.AINum].ucSrcAIHdCh == phwaich_g[j].ucModCh)
                {
                    OptAIMod_g.aOptBoxAiCfg_g[OptAIMod_g.AINum].fCoff = phwaich_g[j].fCoff;
                    break;
                }
            }
            if(j == iHwAiChNum_g)
                assert(FALSE);

            tmppOptAoCfg++;
            OptAIMod_g.AINum++;
        }
    }

    /* 光纵数据存储区指针给定 */

    pucOptAoDataByteBaseSamp_g=ppucRtOptAoDataByteBase;		/* 指针地址 */

#ifdef OPTFLOATSEND
    assert(4*uiTxPts*OptAIMod_g.AINum == iOptAoDataByteLen);			/* 判断存储字节数 */
    *ppucRtOptAoDataByteBase = (uint8_t *)fOptAiBuf;		/* 用于浮点传送 */
    OptAIMod_g.pfOptAIResult = fOptAiBuf;
#else
    assert(2*uiTxPts*OptAIMod_g.AINum == iOptAoDataByteLen);			/* 判断存储字节数 */
    *ppucRtOptAoDataByteBase = (uint8_t *)OptAiBuf;			/* 用于定点 */
    OptAIMod_g.pOptAIResult = OptAiBuf;
#endif

    /* 按照AO机箱物理通道用冒泡法排序 */
    for(i=0; i<OptAIMod_g.AINum-1; i++)
        for(j=0; j<OptAIMod_g.AINum-1-i; j++)
        {
            if(OptAIMod_g.aOptBoxAiCfg_g[j].ucAOHdCh>OptAIMod_g.aOptBoxAiCfg_g[j+1].ucAOHdCh)
            {
                tmpAI_CFG = OptAIMod_g.aOptBoxAiCfg_g[j];
                OptAIMod_g.aOptBoxAiCfg_g[j] = OptAIMod_g.aOptBoxAiCfg_g[j+1];
                OptAIMod_g.aOptBoxAiCfg_g[j+1] = tmpAI_CFG;
            }
        }

    OptAIMod_g.ppArray=&pAdc_Data;		/* 指针的地址 */
    for(i=0; i<OptAIMod_g.AINum; i++)
    {
        ntmp = Sam_to_ana[OptAIMod_g.aOptBoxAiCfg_g[i].ucSrcAIHdCh];
        ntmp = ntmp -1;

        OptAIMod_g.iPos[i]=ntmp;			/* 指针位置*/
        OptAIMod_g.fCoff[i] = DspInfo.fSmvInOut[OptAIMod_g.aOptBoxAiCfg_g[i].ucSrcAIHdCh];
        OptAIMod_g.iCoff[i] = 1.0/OptAIMod_g.fCoff[i];
        OptAIMod_g.iZeroCurPos[i]=DspInfo.LogtoAna[OptAIMod_g.aOptBoxAiCfg_g[i].ucSrcAIHdCh];		/* 物理通道到采样通道 */
    }

    if (bdType_g == BOARD_TYPE_E02)
    {
        /* EDP02平台 */
        /* 定时器定时数据 */
        OptAIMod_g.RegNum = sysInputFreq_g/16/Sample_Rate-1;/*2007-6-15日张云修改原来的BUG，因为时钟信号频率变位BUSFRQ/16  */
        OptAIMod_g.BackupRegNum = sysInputFreq_g/16/Sample_Rate-1;
    }
    else if (bdType_g == BOARD_TYPE_E03)
    {
        /* 定时器定时数据 */
        OptAIMod_g.RegNum = sysInputFreq_g/Sample_Rate-1;/*2007-6-15日张云修改原来的BUG，因为时钟信号频率变位BUSFRQ/16  */
        OptAIMod_g.BackupRegNum = sysInputFreq_g/Sample_Rate-1;
    }

    /* 光纵采样允许标志 */
    OPTAD_flag = TRUE;

    TotalNumofTrans = OptAIMod_g.AINum*OptAIMod_g.uiTxPts;		/* 传输点数 */

#ifdef OPTFLOATSEND
    /* 缓冲区首指针给定, 浮点型 */
    pftmpOptBuf = ftmpOptBuf;
#else
    /* 缓冲区首指针给定 */
    ptmpOptBuf = tmpOptBuf; 				/* 用于定点传送 */
#endif

    logMsg("Init_OptBox_AO Over! %d\n", OptAIMod_g.AINum, 0, 0, 0, 0, 0);

    return stsRet;
}

/* update the coefficient of OPT AI channel .
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void UpdateOptAcCoff(void)
{
    int i, j;

    for(i=0; i<OptAIMod_g.AINum; i++)
    {
        for(j=0; j<iHwAiChNum_g; j++)
        {
            /* 从物理通道配置中寻找通道系数 */
            if(OptAIMod_g.aOptBoxAiCfg_g[i].ucSrcAIHdCh == phwaich_g[j].ucModCh)
            {
                OptAIMod_g.aOptBoxAiCfg_g[i].fCoff=phwaich_g[j].fCoff;
                break;
            }
        }

        if(j == iHwAiChNum_g)
            assert(FALSE);
    }
}

/* writing the data buffer of OPT AI channel using interger data, called in  ISR.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void WrOptBuf(void)
{
    /* 首地址给定 */
    OptAIMod_g.pOptAIResult = OptAiBuf;
    *pucOptAoDataByteBaseSamp_g=(uint8_t *)(ptmpOptBuf+OPT_BUF_LENGTH-TotalNumofTrans);

    /* 通知数据发送 */
    if(Send_flag)
    {
        OPT_Send(Sam_Counter_Int_g);
    }
}

/* get the sampling counter of sampling module.
 * Para:
 *     NONE.
 * Return:
 *     the current counter.
 */
uint8_t OPT_GetSamClk()
{
    return (uint8_t)Sam_Counter_Int_g;		/* 本机节拍 */
}

/* get the sampling time based on the sampling counter.
 * Para:
 *     ucSamCnt, sampling counter.
 *     pulRtTimeBaseH, high 32 bits of time.
 *     pulRtTimeBaseL, low 32 bits of time.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS OPT_GetSamTimeBase(uint8_t ucSamCnt, uint32_t *pulRtTimeBaseH, uint32_t *pulRtTimeBaseL)
{
    *pulRtTimeBaseH = OptAIMod_g.SamTimebuf[2*ucSamCnt];
    *pulRtTimeBaseL = OptAIMod_g.SamTimebuf[2*ucSamCnt+1];

    return EP_SUCCESS;
}

/* adjust the sampling mode.
 * Para:
 *     iAdjMode, sampling mode, 0: quick mode; 1: slow mode; 2: normal mode; other reserved.
 *     iAdjSamPeriod, period after adjusting, unit is ns.
 *     iAdjSamCnt, times of adjusting.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 *
 * alert: the times will be subtracted by 1 after every adjusting; the adjusting stop when the times equal to 0.
 *        permit to adjust again before the times equal to 0; using the new mode after adjusting again.
 */
EP_STATUS OPT_AdjustSamMode(int iAdjMode, int iAdjSamPeriod, int iAdjSamCnt)
{
    EP_STATUS stsRet;

    stsRet = EP_SUCCESS;
    OptAIMod_g.iAdjMode = iAdjMode;

    /* 定时器给定 */
    if (bdType_g == BOARD_TYPE_E02)
    {
        OptAIMod_g.RegNum = ((sysInputFreq_g/1000000)*iAdjSamPeriod)/16/1000-1;
    }
    else if (bdType_g == BOARD_TYPE_E03)
    {
        OptAIMod_g.RegNum = ((sysInputFreq_g/1000000)*iAdjSamPeriod)/1000-1;
    }
    OptAIMod_g.iAdjSamCnt = iAdjSamCnt;
    Adjust_flag  = TRUE;

    return stsRet;
}

/* get the sampling mode.
 * Para:
 *     piRtLeftAdjCnt, the left adjusting times.
 * Return:
 *     adjusting mode, 0: quick mode; 1: slow mode; 2: normal mode; other reserved.
 */
int OPT_GetAdjSamMode(int *piRtLeftAdjCnt)
{
    *piRtLeftAdjCnt = OptAIMod_g.iAdjSamCnt+1;

    return OptAIMod_g.iAdjMode;
}

/* OPT module inform the sampling mode to send AO.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void OPT_NotifyStartSendAO()
{
    /* 光纵模块初始化后调用一次 */
    Send_flag = TRUE;
}

/* writing cycled buffer using interger data.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void WrBuf(void)
{
    uint8_t i;
    int32_t tmpSampVal;

    for(i=0; i<OptAIMod_g.AINum; i++)
    {
        tmpSampVal = (*((*OptAIMod_g.ppArray)+OptAIMod_g.iPos[i]))/OptAIMod_g.iCoff[i];
        if (tmpSampVal > 32767)
        {
            tmpSampVal = 32767;
        }
        else if (tmpSampVal < -32768)
        {
            tmpSampVal = -32768;
        }

        *ptmpOptBuf = tmpSampVal;
        *(ptmpOptBuf+OPT_BUF_LENGTH) = tmpSampVal;

        if(ptmpOptBuf==tmpOptBuf+OPT_BUF_LENGTH-1)
            /* Buffer circulation,to the beginning of the buffer */
            ptmpOptBuf=tmpOptBuf;
        else
            ptmpOptBuf++;
    }
}
