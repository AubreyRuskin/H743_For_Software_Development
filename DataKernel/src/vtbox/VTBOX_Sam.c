/* VTBOX_Sam.c - subroutine library for virtual box interface */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 07nov08, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for virtual box interface.
INCLUDES: VTBOX_Interface.h
*/

/* includes */

#include "vxWorks.h"			/* always first */
#include "realdata.h"
#include "VTBOX_Interface.h"
#include "VTBOX_SamInterface.h"
#include "logic.h"
#include "errtest.h"
#include "dspai.h"
#include "VTBOX_Data.h"
#include "dsp.h"
#include <intLib.h>

/* defines */

/* globals */

VIRT_BOX_DRIVER VirtBoxInitDriver[MAX_VT_BOX_COUNT];
uint32_t ulDriverNums=0;		/* number of drivers. */
VTBOX_INFO apVtBoxCfgArr_g[MAX_VT_BOX_COUNT]; /* virtual box configuration. */
int iVtBoxCfgNum_g=0;        /* number of virtual box. */

uint8_t **pucVirtBoxAoDataPreByteBaseSamp_g[MAX_VT_BOX_COUNT];       /* data base address of processing varialbe as AO. */
int16_t VirtBoxAiPreBuf[MAX_VT_BOX_COUNT][VIRTBOX_AOofAI_BUF_LENGTH]; /* processing sending buffer. */
int iVirtBoxPreTotalNumofTrans[MAX_VT_BOX_COUNT];
int16_t *ptmpVirtBoxPreBuf[MAX_VT_BOX_COUNT];
int16_t tmpVirtBoxPreBuf[MAX_VT_BOX_COUNT][2*VIRTBOX_AOofAI_BUF_LENGTH]; /* temporary buffer for processing. */

VIRT_BOX_AI_MOD VirtBoxAIMod_g[MAX_VT_BOX_COUNT];	/* information of AO from AI. */
uint8_t **pucVirtBoxAoDataByteBaseSamp_g[MAX_VT_BOX_COUNT];       /* data base address of AI as AO. */
int16_t VirtBoxAiBuf[MAX_VT_BOX_COUNT][VIRTBOX_AOofAI_BUF_LENGTH]; /* sending buffer. */
int16_t *pVirtBoxAdcData[MAX_VT_BOX_COUNT]= {NULL}; 			/* Sampling data in a cycle. */
BOOL bVirtBoxAoSendFlag[MAX_VT_BOX_COUNT]= {FALSE}; 	/* permitting flag. */
int iVirtBoxTotalNumofTrans[MAX_VT_BOX_COUNT];
int16_t *ptmpVirtBoxBuf[MAX_VT_BOX_COUNT];
int16_t tmpVirtBoxBuf[MAX_VT_BOX_COUNT][2*VIRTBOX_AOofAI_BUF_LENGTH]; /* temporary buffer. */
BOOL VirtBoxAdjustFlag[MAX_VT_BOX_COUNT] = {FALSE}; 		/* adjusting flag. */

/* static functions */

/* fill in the cycled buffer of virtual box AO from AI, called in interrupt.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
static void WrVirtBoxCycledBuf(void);

/* fill in the cycled buffer of virtual box AO from processing variable, called in task.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
static void WrVirtBoxPreCycledBuf(void);

/* globals functions */

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

/* Initialize the virtual box, including AO and DO.
 * Para:
 *     ucVtBoxPos, position of virtual box.
 *     uiSmplRate, sampling point per cycle.
 *     uiSysFreq, system frequency.
 *     pvAiMod, virtual box handle.
 *     uiLgcCh, number of origin channel.
 *     plgccfg, pointer to origin channel.
 *     uiCalcCfg, number of processing channel.
 *     pcalccfg, pointer to processing channel.
 *     iVirtBoxAoCh, number of AO.
 *     pVirtBoxAoCfg, pointer to AO.
 *     iVirtBoxDiCh, number of DI.
 *     pucVirtBoxDiStsBase, pointer to DI.
 *     iVirtBoxDoCh, number of DO.
 *     pucVirtBoxDoStsBase, pointer to DO.
 *     uiLgcChLocal, number of logic channel in local box.
 *     plgccfgLocal, first element of logic channel in local box.
 *     uiCalcChLocal, number of proprocessing in local box.
 *     pcalccfgLocal, first element of proprocessing in local box.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_CFG_ERR, fail.
 */
EP_STATUS Init_Virt_Box(uint8_t ucVtBoxPos, u_int uiSmplRate, u_int uiSysFreq, void *pvAiMod,
                        u_int uiLgcCh, DSP_LGC_AI_CFG *plgccfg,
                        u_int uiCalcCfg, DSP_CALC_AI_CFG *pcalccfg, VIRT_BOX_AO_CFG *pBoxAoCfg,
                        u_int uiLgcChLocal,
                        DSP_LGC_AI_CFG *plgccfgLocal,
                        u_int uiCalcChLocal,
                        DSP_CALC_AI_CFG *pcalccfgLocal
                       )
{
    int i;
    int j;
    EP_STATUS stsResult = EP_SUCCESS;
    VIRT_AO_CFG *pVirtBoxAoCfg = NULL;
    DSP_LGC_AI_CFG *tempLoInfo = plgccfg; 		/* Temporary logic pointer. */
    DSP_CALC_AI_CFG *pCalConf = pcalccfg; 		/* Temporary preprocessing pointer. */
    uint16_t iy = 0;
    uint16_t ix = 0;
    uint8_t ucLogicChnNum = 0; 			/* The number of logic channels. */
    uint8_t ucFreqCalNum = 0; 	/* The number of channel requiring frequency calculation. */
    uint8_t ucOriginNum = 0; 			/* channel number of origin value. */
    uint16_t DataNumofPreProcess = 0; 			/* The number of the data transformed after the preprocessing. */
    uint8_t ucSn = 0;
    uint8_t ucPreSn = 0;
    uint8_t ucMidSn = 0;

    apVtBoxCfgArr_g[ucVtBoxPos].iOriginLogicNum = uiLgcCh;
    apVtBoxCfgArr_g[ucVtBoxPos].iPreProNum = uiCalcCfg;
    apVtBoxCfgArr_g[ucVtBoxPos].iAiNum=uiLgcCh+uiCalcCfg;		/* total. */

    /* Initialize the AI. */

    while (iy<uiLgcCh)
    {
        /* logic channel. */
        apVtBoxCfgArr_g[ucVtBoxPos].ucAiHwChArr[iy] = tempLoInfo->ucHdCh - 1;	/* begin from 0. */
        apVtBoxCfgArr_g[ucVtBoxPos].fAiHwCoffArr[iy] = tempLoInfo->fCoff;

        if ((tempLoInfo->ucFiltNum == 0) || (tempLoInfo->ucFiltNum == 2))
        {
            /* instantaneous value */
            ucLogicChnNum++;
            iy++;
            tempLoInfo++;
            continue;
        }
        else if(tempLoInfo->ucFiltNum == 1)
        {
            /* frequency value. */
            ucFreqCalNum++;
            iy++;
            tempLoInfo++;
            continue;
        }
        else if(tempLoInfo->ucFiltNum == 6)
        {
            /* origin value. */
            ucOriginNum++;
            iy++;
            tempLoInfo++;
            continue;
        }
        else
            assert(FALSE);
    }

    DataNumofPreProcess = 0;
    for (ix=0; ix<uiCalcCfg; ix++)
    {
        /* processing value. */
        DataNumofPreProcess=DataNumofPreProcess+(((pCalConf[ix].ucArithParm ) & 0x30)>>4)*pCalConf[ix].ucChNum;
    }

    apVtBoxCfgArr_g[ucVtBoxPos].LogicChnNumber_8 = (ucOriginNum+ucLogicChnNum+ucFreqCalNum)/8;		  	/* logic value. */
    apVtBoxCfgArr_g[ucVtBoxPos].LogicChnNumber_m = (ucOriginNum+ucLogicChnNum+ucFreqCalNum)%8;

    apVtBoxCfgArr_g[ucVtBoxPos].DataNumofPreProcess_8 = DataNumofPreProcess/8; 		/* processing value. */
    apVtBoxCfgArr_g[ucVtBoxPos].DataNumofPreProcess_m = DataNumofPreProcess%8;

    /* Initialize the AO from AI. */

    pVirtBoxAoCfg = pBoxAoCfg->aVirtBoxAoCfg_g;
    for (i=0; i<pBoxAoCfg->iVirtBoxAONum; i++)
    {
        if (((pVirtBoxAoCfg+i)->iAOSrcType == DA_AI_SRC)
                && (((pVirtBoxAoCfg+i)->ucAOHdCh) >= MAX_VIRT_BOX_ALLOW_AIO_NUM))
        {
            /* the first is origin value. */
            assert(FALSE);
        }
    }

    for (i=0; i<MAX_VTBOX_AO_NUM; i++)
    {
        apVtBoxCfgArr_g[ucVtBoxPos].afOrgAISrcAoCoff[i]=1.0;
        apVtBoxCfgArr_g[ucVtBoxPos].afPreAISrcAoCoff[i]=1000.0;
    }

    for (i=0; i<pBoxAoCfg->iVirtBoxAONum; i++)
    {
        if (pBoxAoCfg->aVirtBoxAoCfg_g[i].iAOSrcType == DA_AI_SRC)
        {
            for (j=0; j<uiLgcChLocal; j++)
            {
                if (pBoxAoCfg->aVirtBoxAoCfg_g[i].ucSrcAIHdCh == (plgccfgLocal+j)->ucHdCh-1)
                {

                    /* 传统采样时需处理模拟/数字转换系数 */
                    if (appType_g == APP_TYPE_TRAD)
                    {
                        apVtBoxCfgArr_g[ucVtBoxPos].afOrgAISrcAoCoff[ucSn]=(plgccfgLocal+j)->fCoff*5.0/32768.0;

                    }
                    else if (appType_g == APP_TYPE_DIG)
                    {
                        apVtBoxCfgArr_g[ucVtBoxPos].afOrgAISrcAoCoff[ucSn]=(plgccfgLocal+j)->fCoff;
                    }
                    else
                    {
                        assert (FALSE);
                    }

                    apVtBoxCfgArr_g[ucVtBoxPos].ucOrgAISrcAoDataSN[ucSn] = pBoxAoCfg->aVirtBoxAoCfg_g[i].ucAOHdCh;
                    ucSn++;
                    break;
                }
            }
        }
        else if (pBoxAoCfg->aVirtBoxAoCfg_g[i].iAOSrcType == DA_PRE_SRC)
        {
            apVtBoxCfgArr_g[ucVtBoxPos].pucPreAISrcAoDataSN[ucPreSn] = pBoxAoCfg->aVirtBoxAoCfg_g[i].ucAOHdCh;
            ucPreSn++;
        }
        else if (pBoxAoCfg->aVirtBoxAoCfg_g[i].iAOSrcType == DA_MID_SRC)
        {
            apVtBoxCfgArr_g[ucVtBoxPos].pucPreAISrcAoDataSN[ucMidSn] = pBoxAoCfg->aVirtBoxAoCfg_g[i].ucAOHdCh;
            ucMidSn++;
        }
    }

    stsResult=Init_VirtBox_AOofAIPre(ucVtBoxPos, pBoxAoCfg->iVirtBoxAONum, pVirtBoxAoCfg,
                                     pBoxAoCfg->iVirtBoxAISrcAONum*2, &apVtBoxCfgArr_g[ucVtBoxPos].pucOrgAISrcAoDataByteNextBase,
                                     pBoxAoCfg->iVirtBoxPreSrcAONum*2, &apVtBoxCfgArr_g[ucVtBoxPos].pucPreAISrcAoDataByteNextBase,
                                     uiLgcChLocal, plgccfgLocal, uiCalcChLocal, pcalccfgLocal);

    if (stsResult != EP_SUCCESS)
    {
        return EP_CFG_ERR;
    }

    return EP_SUCCESS;
}

/* Initialize the AO from AI and Preprocessing.
 * Para:
 *     ucVtBoxPos, positon of virtual box.
 *     iVirtBoxAONum, local number of AO.
 *     pVirtBoxAoCfg, configuration of AO.
 *     iAoDataByteLen, byte length of AO from AI.
 *     pucOrgAISrc, pointer of pointer to address saving the AI data.
 *     iAoDataByteLenPre, byte length of AO from processing.
 *     pucOrgPreSrc, pointer of pointer to address saving the processing data.
 *     uiLgcChLocal, number of logic channel in local box.
 *     plgccfgLocal, first element of logic channel in local box.
 *     uiCalcChLocal, number of proprocessing in local box.
 *     pcalccfgLocal, first element of proprocessing in local box.
 * Return:
 *     EP_SUSSESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS Init_VirtBox_AOofAIPre(uint8_t ucVtBoxPos, int iVirtBoxAONum, VIRT_AO_CFG *pVirtBoxAoCfg, int32_t iAoDataByteLen, uint8_t **pucOrgAISrc,
                                 int32_t iAoDataByteLenPre, uint8_t **pucOrgPreSrc, u_int uiLgcChLocal,
                                 DSP_LGC_AI_CFG *plgccfgLocal,
                                 u_int uiCalcChLocal,
                                 DSP_CALC_AI_CFG *pcalccfgLocal)
{
    uint16_t i;
    uint16_t j = 0;
    int16_t ntmp;
    VIRT_AO_CFG *tmppAoCfg;
    VIRT_BOX_AI_CFG tmpAI_CFG;
    DSP_LGC_AI_CFG *tmpplgccfg;
    DSP_CALC_AI_CFG *tmppcalccfg;
    uint16_t uiChnNum=0;
    int32_t itmpPrePos;
    EP_STATUS stsRet;
    BOOL bFind=FALSE;

    stsRet = EP_SUCCESS;

    assert (iVirtBoxAONum<MAX_VTBOX_AO_NUM);

    VirtBoxAIMod_g[ucVtBoxPos].uiTxPts = 1;		/* default: 1. must process the muti-point status. */
    VirtBoxAIMod_g[ucVtBoxPos].iVirtBoxAoDataByteLen = iAoDataByteLen;		/* length of data. */
    VirtBoxAIMod_g[ucVtBoxPos].iVirtBoxAoPreDataByteLen = iAoDataByteLenPre;

    tmppAoCfg = pVirtBoxAoCfg;
    VirtBoxAIMod_g[ucVtBoxPos].AINum = 0;
    VirtBoxAIMod_g[ucVtBoxPos].PreNum = 0;
    for (i=0; i<iVirtBoxAONum; i++)
    {
        if(tmppAoCfg->iAOSrcType == DA_AI_SRC)
        {
            VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxAiCfg_g[VirtBoxAIMod_g[ucVtBoxPos].AINum].ucAOHdCh = tmppAoCfg->ucAOHdCh;
            VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxAiCfg_g[VirtBoxAIMod_g[ucVtBoxPos].AINum].ucSrcAIHdCh = tmppAoCfg->ucSrcAIHdCh;
            for (j=0; j<iHwAiChNum_g; j++)
            {
                /* Get the coefficient of physical channel. */
                if(VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxAiCfg_g[VirtBoxAIMod_g[ucVtBoxPos].AINum].ucSrcAIHdCh == phwaich_g[j].ucModCh)
                {
                    VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxAiCfg_g[VirtBoxAIMod_g[ucVtBoxPos].AINum].fCoff = phwaich_g[j].fCoff;
                    break;
                }
            }

            if (j == iHwAiChNum_g)
            {
                assert(FALSE);

                return EP_ERROR;
            }

            VirtBoxAIMod_g[ucVtBoxPos].AINum++;
        }
        else if (tmppAoCfg->iAOSrcType == DA_PRE_SRC)
        {
            VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxPreCfg_g[VirtBoxAIMod_g[ucVtBoxPos].PreNum].ucAOHdCh = tmppAoCfg->ucAOHdCh;

            switch (tmppAoCfg->ucFiltTp)
            {
                case 0:
                case 8:
                case 0x0A:
                    if (tmppAoCfg->ucFiltTp == 0)
                        tmppAoCfg->ucFiltNum=0;
                    else if (tmppAoCfg->ucFiltTp == 0x0A)
                        tmppAoCfg->ucFiltNum=1;
                    else
                        tmppAoCfg->ucFiltNum=2;

                    for (tmpplgccfg=plgccfgLocal; tmpplgccfg<plgccfgLocal+uiLgcChLocal; tmpplgccfg++)
                    {
                        if (((tmpplgccfg->ucHdCh-1) == tmppAoCfg->ucSrcAIHdCh) && (tmpplgccfg->ucFiltNum == tmppAoCfg->ucFiltNum))
                        {
                            break;
                        }
                    }
                    VirtBoxAIMod_g[ucVtBoxPos].iPrePos[VirtBoxAIMod_g[ucVtBoxPos].PreNum]=tmpplgccfg-plgccfgLocal;
                    break;

                case 0x18:
                    tmppAoCfg->ucFiltNum = 6;
                    for (tmpplgccfg=plgccfgLocal; tmpplgccfg<plgccfgLocal+uiLgcChLocal; tmpplgccfg++)
                    {
                        if (((tmpplgccfg->ucHdCh-1) == tmppAoCfg->ucSrcAIHdCh) && (tmpplgccfg->ucFiltNum == tmppAoCfg->ucFiltNum))
                        {
                            break;
                        }
                    }
                    VirtBoxAIMod_g[ucVtBoxPos].iPrePos[VirtBoxAIMod_g[ucVtBoxPos].PreNum]=tmpplgccfg-plgccfgLocal;
                    break;

                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 0x11:
                case 0x13:
                case 0x15:
                    if (IS_RI_CPLX_AI(tmppAoCfg->ucUnit))
                    {
                        if (tmppAoCfg->ucFiltTp<0x10)
                            tmppAoCfg->ucFiltNum=3;
                        else
                            tmppAoCfg->ucFiltNum=5;

                        tmppAoCfg->ucArithParm=0x20 | (tmppAoCfg->ucFiltTp & 0x07);
                    }
                    else if (IS_MA_CPLX_AI(tmppAoCfg->ucUnit))
                    {
                        tmppAoCfg->ucFiltNum=4;
                        tmppAoCfg->ucArithParm=0x20 | (tmppAoCfg->ucFiltTp & 0x07);
                    }
                    else
                        assert(FALSE);

                    bFind=FALSE;
                    uiChnNum=0;
                    for (tmppcalccfg=pcalccfgLocal; tmppcalccfg<pcalccfgLocal+uiCalcChLocal; tmppcalccfg++)
                    {
                        if (tmppcalccfg->ucArithNum == tmppAoCfg->ucFiltNum)
                        {
                            for (j=tmppcalccfg->ucBgnLgcCh; j<tmppcalccfg->ucBgnLgcCh+tmppcalccfg->ucChNum; j++)
                            {
                                if ((plgccfgLocal[j].ucHdCh-1) == tmppAoCfg->ucSrcAIHdCh)
                                {
                                    bFind=TRUE;
                                    break;
                                }
                            }

                            if (bFind)
                            {
                                break;
                            }
                        }
                        uiChnNum += 2*tmppcalccfg->ucChNum;
                    }
                    VirtBoxAIMod_g[ucVtBoxPos].iPrePos[VirtBoxAIMod_g[ucVtBoxPos].PreNum]=uiLgcChLocal+uiChnNum+2*(j-tmppcalccfg->ucBgnLgcCh);
                    if (tmppAoCfg->ucSrcType == 2)
                    {
                        VirtBoxAIMod_g[ucVtBoxPos].iPrePos[VirtBoxAIMod_g[ucVtBoxPos].PreNum]++;
                    }

                    break;

                default:
                    assert(FALSE);
                    break;
            }
            VirtBoxAIMod_g[ucVtBoxPos].PreNum++;
        }

        tmppAoCfg++;
    }

    /* data pointer. */

    pucVirtBoxAoDataByteBaseSamp_g[ucVtBoxPos]=pucOrgAISrc;
    pucVirtBoxAoDataPreByteBaseSamp_g[ucVtBoxPos]=pucOrgPreSrc;

    assert (2*VirtBoxAIMod_g[ucVtBoxPos].uiTxPts*VirtBoxAIMod_g[ucVtBoxPos].AINum == iAoDataByteLen);			/* length. */
    assert (2*VirtBoxAIMod_g[ucVtBoxPos].uiTxPts*VirtBoxAIMod_g[ucVtBoxPos].PreNum == iAoDataByteLenPre);			/* length. */

    *pucOrgAISrc = (uint8_t *)VirtBoxAiBuf[ucVtBoxPos];
    VirtBoxAIMod_g[ucVtBoxPos].pVirtBoxAIResult = VirtBoxAiBuf[ucVtBoxPos];

    *pucOrgPreSrc = (uint8_t *)VirtBoxAiPreBuf[ucVtBoxPos];
    VirtBoxAIMod_g[ucVtBoxPos].pVirtBoxPreResult = VirtBoxAiPreBuf[ucVtBoxPos];

    /* composite. */
    for (i=0; i<VirtBoxAIMod_g[ucVtBoxPos].AINum-1; i++)
        for (j=0; j<VirtBoxAIMod_g[ucVtBoxPos].AINum-1-i; j++)
        {
            if (VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxAiCfg_g[j].ucAOHdCh>VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxAiCfg_g[j+1].ucAOHdCh)
            {
                tmpAI_CFG = VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxAiCfg_g[j];
                VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxAiCfg_g[j] = VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxAiCfg_g[j+1];
                VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxAiCfg_g[j+1] = tmpAI_CFG;
            }
        }

    for (i=0; i<VirtBoxAIMod_g[ucVtBoxPos].PreNum-1; i++)
        for (j=0; j<VirtBoxAIMod_g[ucVtBoxPos].PreNum-1-i; j++)
        {
            if (VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxPreCfg_g[j].ucAOHdCh>VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxPreCfg_g[j+1].ucAOHdCh)
            {
                tmpAI_CFG = VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxPreCfg_g[j];
                VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxPreCfg_g[j] = VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxPreCfg_g[j+1];
                VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxPreCfg_g[j+1] = tmpAI_CFG;

                itmpPrePos=VirtBoxAIMod_g[ucVtBoxPos].iPrePos[j];
                VirtBoxAIMod_g[ucVtBoxPos].iPrePos[j]=VirtBoxAIMod_g[ucVtBoxPos].iPrePos[j+1];
                VirtBoxAIMod_g[ucVtBoxPos].iPrePos[j+1]=itmpPrePos;
            }
        }

    VirtBoxAIMod_g[ucVtBoxPos].ppArray=&pAdc_Data;
    VirtBoxAIMod_g[ucVtBoxPos].ppfPreArray=&pPreBufMain;

    for (i=0; i<VirtBoxAIMod_g[ucVtBoxPos].AINum; i++)
    {
        ntmp = Sam_to_ana[VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxAiCfg_g[i].ucSrcAIHdCh];
        ntmp = ntmp-1;

        VirtBoxAIMod_g[ucVtBoxPos].iPos[i]=ntmp;
        VirtBoxAIMod_g[ucVtBoxPos].iZeroCurPos[i]=DspInfo.LogtoAna[VirtBoxAIMod_g[ucVtBoxPos].aVirtBoxAiCfg_g[i].ucSrcAIHdCh];
    }

#if defined(EDP_01_02_BUILD)
    VirtBoxAIMod_g[ucVtBoxPos].RegNum = sysInputFreq_g/16/Sample_Rate-1;
    VirtBoxAIMod_g[ucVtBoxPos].BackupRegNum = sysInputFreq_g/16/Sample_Rate-1;
#endif

#if defined(EDP03_BUILD)
    VirtBoxAIMod_g[ucVtBoxPos].RegNum = sysInputFreq_g/Sample_Rate-1;
    VirtBoxAIMod_g[ucVtBoxPos].BackupRegNum = sysInputFreq_g/Sample_Rate-1;
#endif

    /* permitting flag. */
    bVirtBoxAoSendFlag[ucVtBoxPos] = TRUE;

    iVirtBoxTotalNumofTrans[ucVtBoxPos] = VirtBoxAIMod_g[ucVtBoxPos].AINum*VirtBoxAIMod_g[ucVtBoxPos].uiTxPts;
    iVirtBoxPreTotalNumofTrans[ucVtBoxPos] = VirtBoxAIMod_g[ucVtBoxPos].PreNum*VirtBoxAIMod_g[ucVtBoxPos].uiTxPts;

    ptmpVirtBoxBuf[ucVtBoxPos] = tmpVirtBoxBuf[ucVtBoxPos];
    ptmpVirtBoxPreBuf[ucVtBoxPos] = tmpVirtBoxPreBuf[ucVtBoxPos]; 		/* temporary buffer. */

    return stsRet;
}

/* fill in the cycled buffer of virtual box AO from AI, called in interrupt.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
static void WrVirtBoxCycledBuf(void)
{
    uint8_t i, j;

    for (i=0; i<iVtBoxCfgNum_g; i++)
    {
        if (bVirtBoxAoSendFlag[i])
        {
            for (j=0; j<VirtBoxAIMod_g[i].AINum; j++)
            {
                *ptmpVirtBoxBuf[i]=*((*VirtBoxAIMod_g[i].ppArray)+VirtBoxAIMod_g[i].iPos[j]);
                *(ptmpVirtBoxBuf[i]+VIRTBOX_AOofAI_BUF_LENGTH)=*((*VirtBoxAIMod_g[i].ppArray)+VirtBoxAIMod_g[i].iPos[j]);

                if (ptmpVirtBoxBuf[i] == tmpVirtBoxBuf[i]+VIRTBOX_AOofAI_BUF_LENGTH-1)		/* Buffer circulation, to the beginning of the buffer. */
                {
                    ptmpVirtBoxBuf[i]=tmpVirtBoxBuf[i];
                }
                else
                {
                    ptmpVirtBoxBuf[i]++;
                }
            }
        }
    }
}

/* fill in the cycled buffer of virtual box AO from processing variables, called in task.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
static void WrVirtBoxPreCycledBuf(void)
{
    uint8_t i, j;
    float ftmpData;

    for (i=0; i<iVtBoxCfgNum_g; i++)
    {
        /* the actual virtual box number. */
        for (j=0; j<VirtBoxAIMod_g[i].PreNum; j++)
        {
            ftmpData = *((*VirtBoxAIMod_g[i].ppfPreArray)+VirtBoxAIMod_g[i].iPrePos[j]);

            *ptmpVirtBoxPreBuf[i]=1000*ftmpData;
            *(ptmpVirtBoxPreBuf[i]+VIRTBOX_AOofAI_BUF_LENGTH)=1000*ftmpData;

            if (ptmpVirtBoxPreBuf[i] == tmpVirtBoxPreBuf[i]+VIRTBOX_AOofAI_BUF_LENGTH-1)		/* Buffer circulation, to the beginning of the buffer. */
            {
                ptmpVirtBoxPreBuf[i]=tmpVirtBoxPreBuf[i];
            }
            else
            {
                ptmpVirtBoxPreBuf[i]++;
            }
        }
    }
}

/* update the AO coefficient from AI.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS UpdateVirtBoxAcCoff(void)
{
    int i, j, k;

    for (i=0; i<MAX_VT_BOX_COUNT; i++)
    {
        for (j=0; j<VirtBoxAIMod_g[i].AINum; j++)
        {
            for (k=0; k<iHwAiChNum_g; k++)
            {
                /* search the coefficient. */
                if (VirtBoxAIMod_g[i].aVirtBoxAiCfg_g[j].ucSrcAIHdCh == phwaich_g[k].ucModCh)
                {
                    VirtBoxAIMod_g[i].aVirtBoxAiCfg_g[j].fCoff = phwaich_g[k].fCoff;
                    break;
                }
            }

            if (j == iHwAiChNum_g)
            {
                assert(FALSE);

                return EP_ERROR;
            }
        }
    }

    return EP_SUCCESS;
}

/* register initialization driver.
 * Para:
 *     pDriverFd, devices descriptor.
 *     pDriver, driver function.
 *     p_Show_Sts_Fun, Show status of virtual box on MMI.
 *     p_Clr_Sts_Fun, clear status.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VTBOX_Driver_Reg(uint8_t *pDriverFd, P_INIT_DRV_FUN pDriver, P_SHOW_STS_FUNC p_Show_Sts_Fun, P_CLR_STS_FUNC p_Clr_Sts_Fun)
{
    VIRT_BOX_DRIVER *pDrvEntry = NULL;
    int drvnum;

    /* Find a free driver table entry. */

    for (drvnum = 0; drvnum < MAX_VT_BOX_COUNT; drvnum++)
        if (! VirtBoxInitDriver [drvnum].bInuse)
        {
            /* We've got a free entry */

            pDrvEntry = &VirtBoxInitDriver [drvnum];
            break;
        }

    if (pDrvEntry == NULL)
    {
        /* we couldn't find a free driver table entry */

        return (EP_ERROR);
    }

    pDrvEntry->bInuse = TRUE;
    pDrvEntry->p_Init_Drv_Fun = pDriver;

    strncpy(pDrvEntry->ucDevDescPrefix, pDriverFd, strlen(pDriverFd));
    pDrvEntry->ucDevDescPrefix[strlen(pDriverFd)]='\0';
    ulDriverNums++;

    return (EP_SUCCESS);
}

/* register periodly called function for driver.
 * Para:
 *     pucVtBoxDevID, device describe.
 *     pfUser, function called periodly.
 *     iPeriodIntvlType, calling type.
 *     iPeriodIntvl, calling interval.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VTBOX_Reg_Period_Func(uint8_t *pucVtBoxDevID, P_PERIOD_USER_FUNC pfUser, VTBOX_PERIOD_FUNC_INTVL_TYPE iPeriodIntvlType, int iPeriodIntvl)
{
    VIRT_BOX_DRIVER *pDrvEntry = NULL;
    int drvnum;

    /* Find a driver table entry. */

    for (drvnum = 0; drvnum < MAX_VT_BOX_COUNT; drvnum++)
        if (!strcmp(pucVtBoxDevID, VirtBoxInitDriver[drvnum].ucDevDescPrefix) && VirtBoxInitDriver [drvnum].bInuse)
        {
            /* get a entry. */

            pDrvEntry = &VirtBoxInitDriver [drvnum];
            break;
        }

    if (pDrvEntry == NULL)
    {
        /* we couldn't find a free driver table entry */

        return (EP_ERROR);
    }

    pDrvEntry->PeriodScan[iPeriodIntvlType].pPeriodUserFunc = pfUser;
    pDrvEntry->PeriodScan[iPeriodIntvlType].iPeriodIntvlType = iPeriodIntvlType;
    pDrvEntry->PeriodScan[iPeriodIntvlType].iPeriodIntvl = iPeriodIntvl;

    return (EP_SUCCESS);
}

/* Get the synchronizated local AI counter for specialized virtual box.
 * Para:
 *     iPos, sequence number of virtual box.
 * Return:
 *     AI counter.
 */
uint32_t VTBOX_AI_Cnt(int iPos)
{
    if (iPos<iVtBoxCfgNum_g)
    {
        return aimodVtBox_g[iPos].ulVirtRefreshedCnt;
    }
    else
    {
        assert(FALSE);

        return 0;
    }
}

/* Get the most delayed synchronizated local AI counter of the virtual box.
 * Para:
 *     NONE.
 * Return:
 *     AI counter.
 */
uint32_t get_VTBOX_MaxDelay_AI_Cnt(void)
{
    int32_t i;
    uint32_t ulMaxCnt;
    int32_t lDif;
    int32_t lCurDif;

    ulMaxCnt = aimodVtBox_g[0].ulVirtRefreshedCnt;
    lDif = RD_AI_Cnt() - aimodVtBox_g[0].ulVirtRefreshedCnt;

    for (i=0; i<iVtBoxCfgNum_g; i++)
    {
        lCurDif = RD_AI_Cnt() - aimodVtBox_g[i].ulVirtRefreshedCnt;
        if (lCurDif > lDif)
        {
            ulMaxCnt = aimodVtBox_g[i].ulVirtRefreshedCnt;
            lDif = lCurDif;
        }
    }

    return ulMaxCnt;
}

/* Get the pointer to realtime AI channel and preprocessing AI channel.
 * Para:
 *     pvAiMod, handle of virtual box.
 *     ulSmplClk, sampling clock.
 *     ppfRtWr, pointer for AI channel.
 *     ppxRtWr, pointer for preprocessing channel.
 *     ppstsWr, statuc of vitrual box.
 *     ppbRtAiValidWr, if valid.
 *     pulRtAiCnt, AI counter.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VTBOX_AI_Dat_P(void *pvAiMod, uint32_t ulSmplClk, float **ppfRtWr,
                    COMPLEX **ppxRtWr, VIRT_BOX_CH_STS **ppstsWr, BOOL **ppbRtAiValidWr, uint32_t *pulRtAiCnt)
{
    uint32_t ulNextCntWork;
    uint32_t ulHeadClkWork;
    float *pfWork;
    COMPLEX *pxWork;
    BOOL *pbValidWork;
    VIRT_BOX_CH_STS *pstsWork;
    float *pfReturn;
    COMPLEX *pxReturn;
    BOOL *pbAiValid;
    uint32_t ulClkDiff = 0;
    int iLockKey;
    RD_AI_MOD *pCurAiMod;
    static uint32_t ulCnt = 0;
    static BOOL bFstFlag = TRUE;
    uint32_t ulRtNextCnt;
    int iRtDbOfst;

    ulCnt++;

    assert (pvAiMod);
    pCurAiMod = (RD_AI_MOD *)pvAiMod;

    assert ((pCurAiMod >= &(aimodVtBox_g[0])) && (pCurAiMod < &(aimodVtBox_g[iVtBoxCfgNum_g])));

    iLockKey=intLock();        /* data integrality. */
    ulNextCntWork=pCurAiMod->ulNextCnt;		/* next counter. */
    ulHeadClkWork=pCurAiMod->ulHeadClk;				/* next counter in 10 cycles. */
    pfWork=pCurAiMod->pfWork;
    pxWork=pCurAiMod->pxWork;
    pbValidWork=pCurAiMod->pbAiValidDbWork;
    pstsWork=pCurAiMod->pVirtChStsDbWork;
    intUnlock(iLockKey);

    if (bFstFlag)
    {
        bFstFlag = FALSE;
        assert (RD_Get_DSP_MOD_Info(&ulRtNextCnt, &iRtDbOfst) == EP_SUCCESS);
        assert ((iRtDbOfst) >= 0 && (iRtDbOfst)<lgcaidb_g.ulBufLen);

        pCurAiMod->ulClkDiff = ulRtNextCnt-1;
        LOG_Dbg_Msg("ulRtNextCnt = %d iRtDbOfst = %d\n", ulRtNextCnt, iRtDbOfst, 0, 0, 0, 0);

        pfReturn=(float *)((uint8_t*)pfWork-ulClkDiff*lgcaidb_g.uiChBytes);
        pxReturn=(COMPLEX *)((uint8_t*)pxWork-ulClkDiff*calcaidb_g.uiChBytes);
        pbAiValid=pbValidWork-ulClkDiff*aivaliddb_g.uiTotalCh;
        *ppstsWr=(VIRT_BOX_CH_STS *)((uint8_t *)pstsWork-ulClkDiff*virtstsdb_g.uiChBytes);

        if (pfReturn<lgcaidb_g.pfBufBgn)
        {
            pfReturn=(float *)((uint8_t *)pfReturn+lgcaidb_g.ulBufBytes);
            pxReturn=(COMPLEX *)((uint8_t *)pxReturn+calcaidb_g.ulBufBytes);
            pbAiValid=pbAiValid+aivaliddb_g.ulBufLen;
            *ppstsWr=(VIRT_BOX_CH_STS *)((uint8_t *)(*ppstsWr)+virtstsdb_g.ulBufBytes);
        }

        *ppfRtWr=pfReturn;
        *ppxRtWr=pxReturn;
        *pbAiValid=TRUE;

        *pulRtAiCnt=ulNextCntWork-1-ulClkDiff;		/* the updating counter in fact. */
    }
    else
    {
        ulSmplClk = SynSamAdjust(ulSmplClk, pCurAiMod->ulClkDiff);

        if (ulHeadClkWork >= ulSmplClk)
        {
            /* compare between the local AI counter and virtual box counter of 10 cycles. */
            ulClkDiff=ulHeadClkWork-ulSmplClk;
        }
        else
        {
            ulClkDiff=ulHeadClkWork-ulSmplClk+RD_SAM_SYN_CLK;
        }

        /* debug information. */
        if (ulClkDiff>80)
        {
            static uint32_t ulTestCnt2_s=0;

            ulTestCnt2_s++;
            if ((ulTestCnt2_s&0x3FFFF) == 1 || (ulTestCnt2_s&0x3FFFF) == 2 || (ulTestCnt2_s&0x3FFFF) == 3 || (ulTestCnt2_s&0x3FFFF) == 4)
            {
                LOG_Dbg_Msg("ERR, VTBOX_AI_Dat_P, CLK DIFF IS %u, HeadClk is %u, SampClk is %u \n", ulClkDiff, ulHeadClkWork, ulSmplClk, 0, 0, 0);
            }
        }

        if (ulCnt %6123 == 1)
        {
            LOG_Dbg_Msg("ulClkDiff = %d ulHeadClkWork = %d\n", ulClkDiff, ulHeadClkWork, 0, 0, 0, 0);
        }

        if (ulClkDiff>80)
        {
            /* The difference is too big. */
            /* return FALSE; */
        }

        pfReturn=(float *)((uint8_t*)pfWork-ulClkDiff*lgcaidb_g.uiChBytes);
        pxReturn=(COMPLEX *)((uint8_t*)pxWork-ulClkDiff*calcaidb_g.uiChBytes);
        pbAiValid=pbValidWork-ulClkDiff*aivaliddb_g.uiTotalCh;
        *ppstsWr=(VIRT_BOX_CH_STS *)((uint8_t *)pstsWork-ulClkDiff*virtstsdb_g.uiChBytes);

        if (pfReturn<lgcaidb_g.pfBufBgn)
        {
            pfReturn=(float *)((uint8_t *)pfReturn+lgcaidb_g.ulBufBytes);
            pxReturn=(COMPLEX *)((uint8_t *)pxReturn+calcaidb_g.ulBufBytes);
            pbAiValid=pbAiValid+aivaliddb_g.ulBufLen;
            *ppstsWr=(VIRT_BOX_CH_STS *)((uint8_t *)(*ppstsWr)+virtstsdb_g.ulBufBytes);
        }

        *ppfRtWr=pfReturn;
        *ppxRtWr=pxReturn;
        *pbAiValid=TRUE;

        *pulRtAiCnt=ulNextCntWork-1-ulClkDiff;		/* the updating counter in fact. */
    }

    return TRUE;
}

/* Get the pointer to realtime AI channel and preprocessing AI channel.
 * Para:
 *     pvAiMod, handle of virtual box.
 *     ulSmplClk, sampling clock.
 *     ppfRtWr, pointer for AI channel.
 *     ppxRtWr, pointer for preprocessing channel.
 *     ppstsWr, statuc of vitrual box.
 *     ppbRtAiValidWr, if valid.
 *     pulRtAiCnt, AI counter.
 * Return:
 *     TRUE, or FALSE.
 */
float *VTBOX_AI_Dat_P_ts(void *pvAiMod, uint32_t ulSmplClk, float **ppfRtWr,
                         COMPLEX **ppxRtWr, VIRT_BOX_CH_STS **ppstsWr, BOOL **ppbRtAiValidWr, uint32_t *pulRtAiCnt)
{
    RD_AI_MOD *paimod;
    int i;
    uint32_t ulMissedClk;
    static BOOL bFirstEnter_s=TRUE;
    char TempInfo[256];

    assert (pvAiMod);

    paimod = (RD_AI_MOD *)pvAiMod;

    if (!bFirstEnter_s)
    {
        /* first. */
        if ((ulSmplClk == paimod->ulHeadClk+1) || ulSmplClk == 0)
        {
            /* nornal. */
            paimod->pfWork = (float *)((uint8_t *)paimod->pfWork+lgcaidb_g.uiChBytes);
            if (paimod->pfWork >= lgcaidb_g.pfBufEnd)
            {
                paimod->pfWork=paimod->pfDbBgn;
                paimod->pxWork=paimod->pxDbBgn;
                paimod->pbAiValidDbWork=paimod->pbAiValidDbBgn;
            }
            else
            {
                paimod->pxWork=(COMPLEX *)((uint8_t *)paimod->pxWork+calcaidb_g.uiChBytes);
                paimod->pbAiValidDbWork=paimod->pbAiValidDbWork+aivaliddb_g.uiTotalCh;
            }

            paimod->ulHeadClk=ulSmplClk;
            paimod->ulNextCnt++;
        }
        else if ((ulSmplClk == (paimod->ulHeadClk+2)%RD_SAM_SYN_CLK)
                 || (ulSmplClk == (paimod->ulHeadClk+3)%RD_SAM_SYN_CLK))
        {
            /* miss 1 or 2. */
            if (ulSmplClk == (paimod->ulHeadClk+2)%RD_SAM_SYN_CLK)
            {
                ulMissedClk=1;
            }
            else
            {
                ulMissedClk=2;
            }

            if ((paimod >= &(aimodVtBox_g[0])) && (paimod < &(aimodVtBox_g[iVtBoxCfgNum_g])))
            {
                static uint32_t ulErrCnt=0;

                ulErrCnt++;
                if (ulErrCnt%20000 == 1)
                {
                    LOG_Dbg_Msg("WARNING: Virtual Box sample AI data %d clock missed.\n", ulMissedClk, 0, 0, 0, 0, 0);

                    if (ENG_MODE == 1)
                    {
                        LOG_Write(LOG_KERNEL, "Hint: PTL(Parallel Transmission Lines) sampling AI data is missed .\n", NULL);
                    }
                    else if (ENG_MODE == 0)
                    {
                        LOG_Write(LOG_KERNEL, "提示: 虚拟机箱AI数据丢失.\n", NULL);
                    }
                }
            }
            else
            {
                assert(FALSE);
            }

            for (i=0; i<=ulMissedClk; i++)
            {
                /* return to the true position. */
                paimod->pfWork=(float *)((uint8_t *)paimod->pfWork+lgcaidb_g.uiChBytes);
                if (paimod->pfWork >= lgcaidb_g.pfBufEnd)
                {
                    paimod->pfWork=paimod->pfDbBgn;
                    paimod->pxWork=paimod->pxDbBgn;
                    paimod->pbAiValidDbWork=paimod->pbAiValidDbBgn;
                }
                else
                {
                    paimod->pxWork=(COMPLEX *)((uint8_t *)paimod->pxWork+calcaidb_g.uiChBytes);
                    paimod->pbAiValidDbWork=paimod->pbAiValidDbWork+aivaliddb_g.uiTotalCh;
                }
                paimod->ulNextCnt++;
            }
            paimod->ulHeadClk=ulSmplClk;
        }
        else
        {
            /* miss points more than 2. */

            if ((paimod >= &(aimodVtBox_g[0])) && (paimod < &(aimodVtBox_g[iVtBoxCfgNum_g])))
            {
                static uint32_t ulErrCnt=0;

                ulErrCnt++;

                if (ulErrCnt%20000 == 1)
                {
                    if (ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SAMPLE_ERR, ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                                   "Error code:%02d\n",
                                   XUNI_LOST_DATA, 0);
                    }
                    else if (ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SAMPLE_ERR, ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                                   "错误码:%02d\n", XUNI_LOST_DATA, 0);
                    }

                    sprintf(TempInfo, "虚拟机箱数据异常,前次采样节拍是%d,本次采样节拍是%d\n",
                            (int)paimod->ulHeadClk, (int)ulSmplClk);
                    LOG_Write(LOG_KERNEL, TempInfo, NULL);

                }
            }
            paimod->ulHeadClk=ulSmplClk;
        }

    }
    else
    {
        uint32_t ulCurDspModNextCnt;
        int iCurDspModDbOfst;

        assert ((paimod >= &(aimodVtBox_g[0])) && (paimod < &(aimodVtBox_g[iVtBoxCfgNum_g])));
        bFirstEnter_s=FALSE;

        assert (RD_Get_DSP_MOD_Info(&ulCurDspModNextCnt, &iCurDspModDbOfst) == EP_SUCCESS);
        assert ((iCurDspModDbOfst) >= 0&&(iCurDspModDbOfst)<lgcaidb_g.ulBufLen);

        paimod->pfWork=paimod->pfDbBgn+iCurDspModDbOfst;
        paimod->pxWork=paimod->pxDbBgn+iCurDspModDbOfst;
        paimod->ulNextCnt=ulCurDspModNextCnt;
        paimod->pbAiValidDbWork=paimod->pbAiValidDbBgn+iCurDspModDbOfst;

        paimod->pfWork=(float*)((uint8_t*)paimod->pfWork+lgcaidb_g.uiChBytes);
        if (paimod->pfWork >= lgcaidb_g.pfBufEnd)
        {
            paimod->pfWork=paimod->pfDbBgn;
            paimod->pxWork=paimod->pxDbBgn;
            paimod->pbAiValidDbWork=paimod->pbAiValidDbBgn;
        }
        else
        {
            paimod->pxWork=(COMPLEX*)((uint8_t*)paimod->pxWork+calcaidb_g.uiChBytes);
            paimod->pbAiValidDbWork=paimod->pbAiValidDbWork+aivaliddb_g.uiTotalCh;
        }

        paimod->ulHeadClk=ulSmplClk;
        paimod->ulNextCnt++;
    }

    *ppxRtWr=paimod->pxWork;

    return paimod->pfWork;
}

/* Get the current AI counter in 10 cycles on local box.
 * Para:
 *     NONE.
 * Return:
 *     counter.
 */
uint32_t VTBOX_GetLocalSamClk(void)
{
    return (uint8_t)Sam_Counter_Int_g;		/* local counter of 10 cycles. */
}

/* Get the interrupt time corresponding to the current AI counter in 10 cycles on the local box.
 * Para:
 *     ulSamCnt, sampling counter.
 *     pulRtTimeBaseH, high word.
 *     pulRtTimeBaseL, low word.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VTBOX_GetSamTimeBase(uint32_t ulSamCnt, uint32_t *pulRtTimeBaseH, uint32_t *pulRtTimeBaseL)
{

    return EP_SUCCESS;
}

/* adjust the local sampling.
 * Para:
 *     ucVtBoxPos, positon of virtual box.
 *     iAdjMode, adjusting mode, 0: accelerate, 1: decelerate, 2: normal.
 *     iAdjSamPeriod, period after adjusting.
 *     iAdjSamCnt, times of adjusting.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VTBOX_AdjustSamMode(uint8_t ucVtBoxPos, int iAdjMode, int iAdjSamPeriod, int iAdjSamCnt)
{
    EP_STATUS stsRet;

    stsRet = EP_SUCCESS;
    VirtBoxAIMod_g[ucVtBoxPos].iAdjMode = iAdjMode;

    /* set the reference register of timer. */
    if (bdType_g == BOARD_TYPE_E02)
    {
        VirtBoxAIMod_g[ucVtBoxPos].RegNum = ((sysInputFreq_g/1000000)*iAdjSamPeriod)/16/1000-1;
    }
    else if (bdType_g == BOARD_TYPE_E03)
    {
        VirtBoxAIMod_g[ucVtBoxPos].RegNum = ((sysInputFreq_g/1000000)*iAdjSamPeriod)/1000-1;
    }
    VirtBoxAIMod_g[ucVtBoxPos].iAdjSamCnt = iAdjSamCnt;

    VirtBoxAdjustFlag[ucVtBoxPos] = TRUE;

    return stsRet;
}

/* Get the adjusting mode of local sampling.
 * Para:
 *     ucVtBoxPos, positon of virtual box.
 *     piRtLeftAdjCnt, left adjusting counter.
 * Return:
 *     sampling adjusting mode, 0: accelerate, 1: decelerate, 2: normal.
 */
int VTBOX_GetAdjSamMode(uint8_t ucVtBoxPos, int *piRtLeftAdjCnt)
{
    *piRtLeftAdjCnt = VirtBoxAIMod_g[ucVtBoxPos].iAdjSamCnt+1;

    return VirtBoxAIMod_g[ucVtBoxPos].iAdjMode;
}

/* matching the device and the driver.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void matchDevDrv(void)
{
    int i, j;

    for (i=0; i<MAX_VT_BOX_COUNT; i++)
    {
        for (j=0; j<ulDriverNums; j++)
        {
            if (!strcmp(aimodVtBox_g[i].aucVtBoxFd, VirtBoxInitDriver[j].ucDevDescPrefix))
            {
                aimodVtBox_g[i].pBoxDrv = &VirtBoxInitDriver[j];
                break;
            }

            if (!strcmp(apVtBoxCfgArr_g[i].aucVtBoxDevID, VirtBoxInitDriver[j].ucDevDescPrefix))
            {
                apVtBoxCfgArr_g[i].pBoxDrv = &VirtBoxInitDriver[j];
                break;
            }

        }
    }
}

/* calling the initialization driver of virtual box.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void VirtBoxInitDrvCall(void)
{
    int i;

    for (i=0; i<ulDriverNums; i++)
    {
        VirtBoxInitDriver[i].p_Init_Drv_Fun();
    }
}

/* calling the periodly called driver of virtual box.
 * Para:
 *     ucCallType, type of calling.
 *     ulCnt, counter.
 * Return:
 *     NONE.
 */
void VirtBoxPeriodDrvCall(uint8_t ucCallType, uint32_t ulCnt)
{
    int i;

    for (i=0; i<ulDriverNums; i++)
    {
        if (!(ulCnt%VirtBoxInitDriver[i].PeriodScan[ucCallType].iPeriodIntvl))
        {
            if(VirtBoxInitDriver[i].PeriodScan[ucCallType].pPeriodUserFunc)
            {
                VirtBoxInitDriver[i].PeriodScan[ucCallType].pPeriodUserFunc();
            }
        }
    }
}

/* showing the status of all virtual box.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void ShowAllVirtBoxSts(void)
{
    uint32_t i;
    uint8_t *pData;
    int32_t len;

    for (i=0; i<iVtBoxCfgNum_g; i++)
    {
        apVtBoxCfgArr_g[i].pBoxDrv->p_Show_Sts_Fun(apVtBoxCfgArr_g[i].iVtBoxAddrInSameKind, &pData, &len);
        LOG_Dbg_Msg("%s\n", (int32_t)pData, 0, 0, 0, 0, 0);
    }
}

/* clearing the status of all virtual box.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void ClearAllVirtBoxSts(void)
{
    uint32_t i;

    for (i=0; i<iVtBoxCfgNum_g; i++)
    {
        apVtBoxCfgArr_g[i].pBoxDrv->p_Clr_Sts_Fun(apVtBoxCfgArr_g[i].iVtBoxAddrInSameKind);
    }
}

/* Get the origin sampling data.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void GetOriginSampData(void)
{
    static uint32_t ulCnt=0;

    ulCnt++;
    VirtBoxPeriodDrvCall(VTBOX_FUNC_BY_SAMP_HEAD_INTVL, ulCnt);
}

/* fill in the buffer of virtual box AO from AI, called in interrupt.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void WrVirtBoxBuf(void)
{
    int i;
    static int16_t SamCount[MAX_VT_BOX_COUNT]= {0};
    static uint32_t ulCnt=0;

    WrVirtBoxCycledBuf();
    for (i=0; i<iVtBoxCfgNum_g; i++)
    {
        /* counter. */
        SamCount[i]++;

        if ((SamCount[i] == VirtBoxAIMod_g[i].uiTxPts) && bVirtBoxAoSendFlag[i])
        {
            SamCount[i]=0;
            /* first address. */
            VirtBoxAIMod_g[i].pVirtBoxAIResult=VirtBoxAiBuf[i];
            *pucVirtBoxAoDataByteBaseSamp_g[i]=(uint8_t *)(ptmpVirtBoxBuf[i]+VIRTBOX_AOofAI_BUF_LENGTH-iVirtBoxTotalNumofTrans[i]);
        }

        if(VirtBoxAdjustFlag[i])
        {
            /* adjusting the sampling timer. */
            SetTimer2 (VirtBoxAIMod_g[i].RegNum);
            VirtBoxAIMod_g[i].iAdjSamCnt--;
            if (VirtBoxAIMod_g[i].iAdjSamCnt<0)
            {
                /* adjusting over. */
                VirtBoxAdjustFlag[i] = FALSE;
                SetTimer2(VirtBoxAIMod_g[i].BackupRegNum);
            }
        }
    }

    ulCnt++;
    VirtBoxPeriodDrvCall(VTBOX_FUNC_BY_SAMP_TAIL_INTVL, ulCnt);
}

/* read the data from substation.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void readDataSub(void)
{
    static uint32_t ulCnt=0;

    ulCnt++;
    VirtBoxPeriodDrvCall(VTBOX_FUNC_BY_PRE_PRC_TAIL_INTVL, ulCnt);
}

/* fill in the buffer of virtual box AO from processing variables, called in task.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void WrVirtBoxPreBuf(void)
{
    int i;
    static int16_t SamCount[MAX_VT_BOX_COUNT]= {0};
    static uint32_t ulCnt=0;

    WrVirtBoxPreCycledBuf();
    for (i=0; i<iVtBoxCfgNum_g; i++)
    {
        /* the actual virtual box number. */
        /* counter. */
        SamCount[i]++;

        if (SamCount[i] == VirtBoxAIMod_g[i].uiTxPts)
        {
            SamCount[i]=0;
            /* first address. */
            VirtBoxAIMod_g[i].pVirtBoxPreResult=VirtBoxAiPreBuf[i];
            *pucVirtBoxAoDataPreByteBaseSamp_g[i]=(uint8_t *)((ptmpVirtBoxPreBuf[i]+VIRTBOX_AOofAI_BUF_LENGTH-iVirtBoxPreTotalNumofTrans[i]));
        }
    }

    ulCnt++;
    VirtBoxPeriodDrvCall(VTBOX_FUNC_BY_PRE_PRC_HEAD_INTVL, ulCnt);
}

/* set the DO to substation.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void setDoDataSub(void)
{
    static uint32_t ulCnt=0;

    ulCnt++;
    VirtBoxPeriodDrvCall(VTBOX_FUNC_BY_FST_LGRP_TAIL_INTVL, ulCnt);
}
