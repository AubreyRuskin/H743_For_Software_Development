/* VTBOX_Data.c - subroutine library for virtual box data */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 07nov08, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for virtual box data.
INCLUDES: VTBOX_Data.h
*/

/* includes */

#include "vxWorks.h"			/* always first */
#include "realdata.h"
#include "VTBOX_Data.h"
#include "logic.h"
#include "errtest.h"
#include "VTBOX_Box.h"
#include "view.h"
#include <intLib.h>
#include <taskLib.h>

/* globals */

VIRT_BOX_CH_STS_DB virtstsdb_g;    /* virtual box DB status. */
VIRT_BOX_IO_INFO aVirtBoxIoInfo_g[MAX_VT_BOX_COUNT];		/* IO module information in virtual box. */

/* local functions. */

/* update the DI in virtual box.
 * Para:
 *     plgcdi, handle of DI.
 *     iNowVal, current value.
 * Return: NONE.
 */
static void VirtBox_Modify_DI(RD_LGC_DI_CH *plgcdi, int iNowVal);

/* Refresh the DI in virtual box.
 * Para:
 *     pvAiMod, handle of virtual box.
 *     ulAiCnt, current local box AI counter.
 * Return: NONE.
 */
static void VirtBox_Refresh_DI(void *pvAiMod, uint32_t ulAiCnt);

/* functions */

/* Initialize the AO channel configuration using middle variables in the virtual box
 * Para:
 *     iPos, position of the virtual box, begin from 0.
 *     iSrcType, type of the data source, middle or AI.
 *     ucSrcValType, type of value.
 *     uiCh, channel number in the module, begin from 0.
 *     pElemIOSrc, the pointer as the middle output in the logic.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS Virt_Init_Mid_Src_AO(int iPos, int iSrcType, uint8_t ucSrcValType, u_int uiCh, void *pElemIOSrc)
{
    VIRT_AO_CFG *pAoCfg;

    if (iPos<MAX_VT_BOX_COUNT)
    {
        pAoCfg=&(BoxAoCfgVirt_g[iPos].aVirtBoxAoCfg_g[BoxAoCfgVirt_g[iPos].iVirtBoxAISrcAONum+BoxAoCfgVirt_g[iPos].iVirtBoxPreSrcAONum+BoxAoCfgVirt_g[iPos].iVirtBoxMidSrcAONum]);
        pAoCfg->iAOSrcType=iSrcType;

        assert (uiCh<BoxAoCfgVirt_g[iPos].iVirtBoxAONum);
        pAoCfg->ucAOHdCh=uiCh;
        pAoCfg->pElemSrc=pElemIOSrc;
        pAoCfg->ucSrcType=ucSrcValType;

        BoxAoCfgVirt_g[iPos].iVirtBoxMidSrcAONum++;
        apVtBoxCfgArr_g[iPos].iMidSrcAoNum++;
        assert ((BoxAoCfgVirt_g[iPos].iVirtBoxAISrcAONum+BoxAoCfgVirt_g[iPos].iVirtBoxPreSrcAONum+BoxAoCfgVirt_g[iPos].iVirtBoxMidSrcAONum) <= BoxAoCfgVirt_g[iPos].iVirtBoxAONum);
    }
    else
    {
        assert(FALSE);

        return EP_ERROR;
    }

    return EP_SUCCESS;
}

/* Initialize the AO channel configuration using origin AI in the virtual box
 * Para:
 *     iPos, position of the virtual box, begin from 0.
 *     iSrcType, type of the data source, middle or AI.
 *     uiCh, channel number in the module, begin from 0.
 *     uiSrcAiCh, channel number of AO in local AI box, begin from 0.
 *     fSrcAiPhyCoff, coefficient of AI.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VirtBox_Init_AI_Src_AO(int iPos, int iSrcType, u_int uiCh, u_int uiSrcAiCh, float fSrcAiPhyCoff)
{
    VIRT_AO_CFG *pAoCfg;

    if (iPos<MAX_VT_BOX_COUNT)
    {
        pAoCfg=&(BoxAoCfgVirt_g[iPos].aVirtBoxAoCfg_g[BoxAoCfgVirt_g[iPos].iVirtBoxAISrcAONum+BoxAoCfgVirt_g[iPos].iVirtBoxPreSrcAONum+BoxAoCfgVirt_g[iPos].iVirtBoxMidSrcAONum]);
        pAoCfg->iAOSrcType=iSrcType;
        assert (uiCh<BoxAoCfgVirt_g[iPos].iVirtBoxAONum);
        pAoCfg->ucAOHdCh=uiCh;
        pAoCfg->ucSrcAIHdCh=uiSrcAiCh;		/* physical channel number. */
        pAoCfg->ucSrcType=0;

        /* 传统采样时需处理模拟/数字转换系数 */
        if (appType_g == APP_TYPE_TRAD)
        {
            pAoCfg->SrcAIPhyCoffUnion.fVal = fSrcAiPhyCoff*5.0/32768.0;
        }
        else if (appType_g == APP_TYPE_DIG)
        {
            pAoCfg->SrcAIPhyCoffUnion.fVal=fSrcAiPhyCoff;
        }
        else
        {
            assert (FALSE);
        }

        BoxAoCfgVirt_g[iPos].iVirtBoxAISrcAONum++;
        apVtBoxCfgArr_g[iPos].iOrgAISrcAoNum++;

        assert ((BoxAoCfgVirt_g[iPos].iVirtBoxAISrcAONum+BoxAoCfgVirt_g[iPos].iVirtBoxPreSrcAONum+BoxAoCfgVirt_g[iPos].iVirtBoxMidSrcAONum) <= BoxAoCfgVirt_g[iPos].iVirtBoxAONum);
    }
    else
    {
        assert(FALSE);

        return EP_ERROR;
    }

    return EP_SUCCESS;
}

/* Initialize the AO channel configuration using processing varibles in the virtual box
 * Para:
 *     iPos, position of the virtual box, begin from 0.
 *     iSrcType, type of the data source, AI, processing, or middle.
 *     ucSrcValType, type of value.
 *     uiCh, channel number in the module, begin from 0.
 *     uiSrcAiCh, channel number of AO in local AI box, begin from 0.
 *     ucFiltTp, number of filter arithmetic.
 *     ucUnit, unit.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VirtBox_Init_Pro_Src_AO(int iPos, int iSrcType, uint8_t ucSrcValType, u_int uiCh, u_int uiSrcAiCh, uint8_t ucFiltTp, uint8_t ucUnit)
{
    VIRT_AO_CFG *pAoCfg;

    if (iPos<MAX_VT_BOX_COUNT)
    {
        pAoCfg=&(BoxAoCfgVirt_g[iPos].aVirtBoxAoCfg_g[BoxAoCfgVirt_g[iPos].iVirtBoxAISrcAONum+BoxAoCfgVirt_g[iPos].iVirtBoxPreSrcAONum+BoxAoCfgVirt_g[iPos].iVirtBoxMidSrcAONum]);
        pAoCfg->iAOSrcType=iSrcType;

        assert (uiCh<BoxAoCfgVirt_g[iPos].iVirtBoxAONum);
        pAoCfg->ucAOHdCh=uiCh;
        pAoCfg->ucSrcAIHdCh=uiSrcAiCh;		/* physical channel number. */
        pAoCfg->ucSrcType=ucSrcValType;
        pAoCfg->ucFiltTp=ucFiltTp;		/* physical channel number. */
        pAoCfg->ucUnit=ucUnit;

        BoxAoCfgVirt_g[iPos].iVirtBoxPreSrcAONum++;
        apVtBoxCfgArr_g[iPos].iPreAISrcAoNum++;

        assert ((BoxAoCfgVirt_g[iPos].iVirtBoxAISrcAONum+BoxAoCfgVirt_g[iPos].iVirtBoxPreSrcAONum+BoxAoCfgVirt_g[iPos].iVirtBoxMidSrcAONum) <= BoxAoCfgVirt_g[iPos].iVirtBoxAONum);
    }
    else
    {
        assert(FALSE);

        return EP_ERROR;
    }

    return EP_SUCCESS;
}

/* Change the coefficient of AO using local AI.
 * Para:
 *     iPos, position of the virtual box, begin from 0.
 *     fSrcAiPhyCoff, coefficient of AI.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VirtBox_Chg_AI_Src_AO_Coff(int iPos, float fSrcAiPhyCoff)
{
    VIRT_AO_CFG *pAoCfg;

    if (iPos<MAX_VT_BOX_COUNT)
    {
        pAoCfg=&(BoxAoCfgVirt_g[iPos].aVirtBoxAoCfg_g[BoxAoCfgVirt_g[iPos].iVirtBoxAISrcAONumTmp]);		/* configuration AO from AI, begin from 0. */

        /* 传统采样时需处理模拟/数字转换系数 */
        if (appType_g == APP_TYPE_TRAD)
        {
            pAoCfg->SrcAIPhyCoffUnion.fVal = fSrcAiPhyCoff*5.0/32768.0;
        }
        else if (appType_g == APP_TYPE_DIG)
        {
            pAoCfg->SrcAIPhyCoffUnion.fVal=fSrcAiPhyCoff;
        }
        else
        {
            assert (FALSE);
        }

        BoxAoCfgVirt_g[iPos].iVirtBoxAISrcAONumTmp++;
    }
    else
    {
        assert(FALSE);

        return EP_ERROR;
    }

    return EP_SUCCESS;
}

/* Initialize the count of virtual box AO.
 * Para:
 *     iPos, position of the virtual box, begin from 0.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VirtBox_InitAOCfgCount(int iPos)
{
    VIRT_BOX_AO_CFG *pBoxAOCfg = NULL;

    if (iPos<MAX_VT_BOX_COUNT)
    {
        pBoxAOCfg=BoxAoCfgVirt_g+iPos;
    }
    else
    {
        assert(FALSE);
    }

    pBoxAOCfg->iVirtBoxAISrcAONumTmp=0;

    return EP_SUCCESS;
}

/* Initialize the IO of virtual box.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VirtBox_IO_Initialize(void)
{
    int i, j;
    VIRT_BOX_IO_INFO *pBoxInfo;
    VTBOX_INFO *pVirtBox;

    for (i=0; i<iVtBoxCfgNum_g; i++)
    {
        pBoxInfo=aVirtBoxIoInfo_g+i;
        pVirtBox=&apVtBoxCfgArr_g[i];		/* Virtual box interface. */

        for (j=0; j<MAX_MOD_NUM; j++)
        {
            /* every IO module. */
            pBoxInfo->aVirtBoxIOModInfo[j].type=IDLE_MODULE;
            pBoxInfo->aVirtBoxIOModInfo[j].unDiChNum=0;
            pBoxInfo->aVirtBoxIOModInfo[j].unDoChNum=0;
        }

        for (j=0; j<VTBOX_MAX_DI_BUF_LEN; j++)
        {
            pVirtBox->aucDiSts[j] = 0;
        }

        for (j=0; j<VTBOX_MAX_DO_BUF_LEN; j++)
        {
            pVirtBox->aucDoSts[j] = 0;
        }

        pBoxInfo->iVirtBoxDiNum_g=0;
        pBoxInfo->iVirtBoxDoNum_g=0;

        pVirtBox->iDiNum = 0;
        pVirtBox->iDoNum = 0;
    }

    return EP_SUCCESS;
}

/* Initialize the DI of virtual box.
 * Para:
 *     iPos, position of the virtual box, begin from 0.
 *     iModAddr, physical address of module, begin from 0.
 *     uiCh, channel number in this module, begin from 0.
 *     ulFilt, value of filter.
 * Return:
 *     pointer of DI index, or NULL.
 */
void *VirtBox_Init_DI(int iPos, int iModAddr, u_int uiCh, uint32_t ulFilt)
{
    VIRT_BOX_IO_INFO *pBoxInfo;
    VIRT_BOX_DI_HND *pDiHdl;
    VTBOX_INFO *pVirtBox;

    assert ((iModAddr >= MAX_VIRT_DI_MOD_BASE_ADDR)
            && (iModAddr<(MAX_VIRT_DI_MOD_BASE_ADDR+MAX_VIRT_BOX_DI_MOD_NUM)));

    assert ((uiCh >= 0) && (uiCh<MAX_DI_PER_MOD));

    if (iPos<MAX_VT_BOX_COUNT)
    {
        pBoxInfo=aVirtBoxIoInfo_g+iPos;
        pVirtBox=&apVtBoxCfgArr_g[iPos];		/* Virtual box interface. */
    }
    else
    {
        assert(FALSE);

        return NULL;
    }

    pBoxInfo->aVirtBoxIOModInfo[iModAddr].unDiChNum++;

    if(pBoxInfo->aVirtBoxIOModInfo[iModAddr].type == IDLE_MODULE)
    {
        pBoxInfo->aVirtBoxIOModInfo[iModAddr].type=DI_MODULE;
    }
    else if(pBoxInfo->aVirtBoxIOModInfo[iModAddr].type != DI_MODULE)
    {
        assert(FALSE);

        return NULL;
    }

    pDiHdl=pBoxInfo->ahVirtBoxDiHandle+pBoxInfo->iVirtBoxDiNum_g;		/* total number of DI. */

    pDiHdl->iPos=iPos;
    pDiHdl->ucMod=iModAddr;
    pDiHdl->ucHdCh=uiCh;
    pDiHdl->aucFilt[0]=HH8(ulFilt);
    pDiHdl->aucFilt[1]=HL8(ulFilt);
    pDiHdl->aucFilt[2]=LH8(ulFilt);
    pDiHdl->aucFilt[3]=LL8(ulFilt);

    pDiHdl->pucDiStsPos=pVirtBox->aucDiSts+(pDiHdl->ucMod-MAX_VIRT_DI_MOD_BASE_ADDR)*4+(pDiHdl->ucHdCh)/8;		/* every Byte. */
    pDiHdl->ucDiStsMsk=BV8((pDiHdl->ucHdCh)%8);		/* every bit. */

    pBoxInfo->iVirtBoxDiNum_g++;
    pVirtBox->iDiNum++;

    return pDiHdl;
}

/* Initialize the DO of virtual box.
 * Para:
 *     iPos, position of the virtual box, begin from 0.
 *     iModAddr, physical address of module, begin from 0.
 *     uiCh, channel number in this module, begin from 0.
 * Return:
 *     pointer of DO index, or NULL.
 */
void *VirtBox_Init_DO(int iPos, int iModAddr, u_int uiCh)
{
    VIRT_BOX_IO_INFO *pBoxInfo;
    VIRT_BOX_DO_HND *pDoHdl;
    VTBOX_INFO *pVirtBox;

    assert ((iModAddr >= VIRT_BOX_DO_MOD_BASE_ADDR)
            && (iModAddr<(VIRT_BOX_DO_MOD_BASE_ADDR+MAX_VIRT_BOX_DO_MOD_NUM)));

    assert ((uiCh >= 0) && (uiCh<MAX_DO_PER_MOD));

    if (iPos<MAX_VT_BOX_COUNT)
    {
        pBoxInfo=aVirtBoxIoInfo_g+iPos;
        pVirtBox=&apVtBoxCfgArr_g[iPos];		/* Virtual box interface. */
    }
    else
    {
        assert(FALSE);

        return NULL;
    }

    pBoxInfo->aVirtBoxIOModInfo[iModAddr].unDoChNum++;

    if (pBoxInfo->aVirtBoxIOModInfo[iModAddr].type == IDLE_MODULE)
    {
        pBoxInfo->aVirtBoxIOModInfo[iModAddr].type=DO_MODULE;
    }
    else if (pBoxInfo->aVirtBoxIOModInfo[iModAddr].type != DO_MODULE)
    {
        assert(FALSE);

        return NULL;
    }

    pDoHdl=pBoxInfo->ahVirtBoxDoHandle+pBoxInfo->iVirtBoxDoNum_g;

    pDoHdl->iPos=iPos;
    pDoHdl->ucMod=iModAddr;
    pDoHdl->ucHdCh=uiCh;

    pDoHdl->pucDoStsPos=pVirtBox->aucDoSts+(pDoHdl->ucMod-VIRT_BOX_DO_MOD_BASE_ADDR)*4+(pDoHdl->ucHdCh)/8;   /* every Byte. */
    pDoHdl->ucDoStsMsk=BV8(pDoHdl->ucHdCh%8);    /* every bit. */

    pBoxInfo->iVirtBoxDoNum_g++;
    pVirtBox->iDoNum++;

    return pDoHdl;
}

/* Get the status of DI in virtual box.
 * Para:
 *     pvDiCh, handle of DI.
 * Return:
 *     current status, TRUE=close or FALSE=close.
 */
BOOL VirtBox_Get_DI(void *pvDiCh)
{
    VIRT_BOX_DI_HND *pDiHdl;

    assert (pvDiCh);
    pDiHdl=(VIRT_BOX_DI_HND *)pvDiCh;

    if ((*(pDiHdl->pucDiStsPos)) & (pDiHdl->ucDiStsMsk))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/* Set the status of DO in virtual box.
 * Para:
 *     pvDoCh, handle of DO.
 *     bClose, current status, TRUE=close or FALSE=close
 * Return: NONE.
 */
void VirtBox_Set_DO(void *pvDoCh, BOOL bClose)
{
    VIRT_BOX_DO_HND *pDoHdl;
    uint8_t ucStsVal;
    uint8_t ucStsMask;
    int iLockKey;

    assert (pvDoCh);
    pDoHdl=(VIRT_BOX_DO_HND *)pvDoCh;

    ucStsVal=*(pDoHdl->pucDoStsPos);
    ucStsMask=pDoHdl->ucDoStsMsk;
    if(bClose)
    {
        ucStsVal=ucStsVal|ucStsMask;
    }
    else
    {
        ucStsVal=ucStsVal&(~ucStsMask);
    }

    iLockKey=intLock();        /* data integrality. */
    *(pDoHdl->pucDoStsPos)=ucStsVal;
    intUnlock(iLockKey);

    return;
}

/* If having the IO in this virtual box.
 * Para:
 *     iPos, position of the virtual box.
 * Return: TRUE, or FALSE.
 */
BOOL Virt_BoxIsCfgDIO(int iPos)
{
    VIRT_BOX_IO_INFO *pBoxInfo = NULL;

    if (iPos<iVtBoxCfgNum_g)
    {
        pBoxInfo=aVirtBoxIoInfo_g+iPos;
    }
    else
    {
        assert(FALSE);
    }

    if ((pBoxInfo->iVirtBoxDiNum_g>0) || (pBoxInfo->iVirtBoxDoNum_g>0))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/* report the updating of virtual box.
 * Para:
 *     pvAiMod, handle of the virtual box.
 *     ulAiCnt, AI counter.
 * Return: NONE.
 */
void VTBox_End_Ai_Wr(void *pvAiMod, uint32_t ulAiCnt)
{
    RD_AI_MOD *pCurAiMod;

    pCurAiMod = (RD_AI_MOD *)pvAiMod;
    assert ((pCurAiMod >= &(aimodVtBox_g[0])) && (pCurAiMod < &(aimodVtBox_g[iVtBoxCfgNum_g])));

    if (Virt_BoxIsCfgDIO(pCurAiMod-aimodVtBox_g))
    {
        VirtBox_Refresh_DI(pCurAiMod, ulAiCnt);
    }

    taskLock();

    pCurAiMod->ulVirtRefreshedCnt=ulAiCnt; 		/* the true counter. */

    taskUnlock();

    return;
}

/* update the DI in virtual box.
 * Para:
 *     plgcdi, handle of DI.
 *     iNowVal, current value.
 * Return: NONE.
 */
static void VirtBox_Modify_DI(RD_LGC_DI_CH *plgcdi, int iNowVal)
{

    if (!(iNowVal & 0x7FFF))
    {
        /* 0 */
        plgcdi->p_part->ulDIModCurVaule &= (~(ulDwordBitArr_g[plgcdi->ucModCh]));
    }
    else
    {
        /* 1 */
        plgcdi->p_part->ulDIModCurVaule |= (ulDwordBitArr_g[plgcdi->ucModCh]);
    }

    if (plgcdi->iVal != iNowVal)
    {
        RE_SetLogDIUpdateCnt();
        if ((plgcdi->iVal & 0x7FFF) != (iNowVal & 0x7FFF))
        {
            /* Report SOE. */
            if ((plgcdi->iVal | iNowVal) & 0x8000)  /* force. */
            {
                /*2013-5-23 ZY */
                TM_High_Get_Sys_Us_UTC_Time(&plgcdi->utChgTime,&plgcdi->ulChgTime);
                if (plgcdi->iMeaCh != -1)
                {
                    VI_New_SOE(plgcdi->iMeaCh, (iNowVal & 0x7FFF), plgcdi->ulChgTime, plgcdi->bSOE, 0);
                }
            }
            else
            {
                plgcdi->ulChgTime=RD_AI_Cnt_To_us(aimodVtBox_g[plgcdi->ucVtBoxPos].ulNextCnt-1);
                if (plgcdi->iMeaCh != -1)
                {
                    VI_New_SOE(plgcdi->iMeaCh, iNowVal, plgcdi->ulChgTime, plgcdi->bSOE, 0);
                }
            }
        }
        plgcdi->iVal=iNowVal;
    }
}

/* Refresh the DI in virtual box.
 * Para:
 *     pvAiMod, handle of virtual box.
 *     ulAiCnt, current local box AI counter.
 * Return: NONE.
 */
static void VirtBox_Refresh_DI(void *pvAiMod, uint32_t ulAiCnt)
{
    BOOL bCurAiIsFast = FALSE;
    BOOL bCurAiIsMid = FALSE;
    BOOL bCurAiIsSlow = FALSE;
    static uint32_t ulRefreshCnt = 0;

    assert (DI_SLOW_REFRESH_INTERVAL == 12);
    assert (DI_MID_REFRESH_INTERVAL == 6);
    assert (DI_FAST_REFRESH_INTERVAL == 2);

    ulRefreshCnt++;

    if ((ulRefreshCnt%DI_SLOW_REFRESH_INTERVAL) == 1)
    {
        bCurAiIsSlow=TRUE;
    }
    if ((ulRefreshCnt%DI_MID_REFRESH_INTERVAL) == 1)
    {
        bCurAiIsMid=TRUE;
    }
    if((ulRefreshCnt%DI_FAST_REFRESH_INTERVAL) == 1
            ||DI_FAST_REFRESH_INTERVAL == 1)
    {
        bCurAiIsFast=TRUE;
    }

    if (bCurAiIsFast||bCurAiIsMid||bCurAiIsSlow)
    {
        /* refresh */
        BOOL *pBaseWork;
        BOOL *pbFirst = NULL;
        BOOL *pbSecond = NULL;
        BOOL *pbThird = NULL;
        BOOL *pbForth = NULL;
        BOOL *pbFifth = NULL;
        BOOL *pbSixth = NULL;
        BOOL *pbSeventh = NULL;
        BOOL *pbEighth = NULL;
        BOOL *pbNinth = NULL;
        BOOL *pbTenth = NULL;
        BOOL *pbEleventh = NULL;
        BOOL *pbTwelvth = NULL;
        RD_LGC_DI_CH *plgcdi;
        int i;
        int iNow;

        if (bCurAiIsSlow)
        {
            /*slow */
            pBaseWork=didb_g.pbWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFirst=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSecond=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbThird=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbForth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFifth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSixth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSeventh=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbEighth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbNinth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbTenth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbEleventh=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbTwelvth=pBaseWork;
        }
        else if (bCurAiIsMid)
        {
            /* middle. */
            pBaseWork=didb_g.pbWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFirst=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSecond=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbThird=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbForth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFifth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSixth=pBaseWork;
        }
        else
        {
            /* fast. */
            pBaseWork=didb_g.pbWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFirst=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSecond=pBaseWork;
        }

        for (plgcdi=plgcdich_g, i=0; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++, i++)
        {
            /* If need to fresh */
            if(plgcdi->ucDIRefreshRate == DI_FAST_REFRESH_RATE
                    && bCurAiIsFast)
            {
                /*fast */
                if ((plgcdi->iForceSts == -1)	/* not force. */
                        && (plgcdi->mod == RD_VT_BOX_DI
                            && ((pvAiMod >= (void *)&(aimodVtBox_g[0])) && (pvAiMod < (void *)&(aimodVtBox_g[iVtBoxCfgNum_g])))))
                {
                    /* refresh DI buffer. */
                    iNow=VirtBox_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    VirtBox_Modify_DI(plgcdi, iNow);
                }
            }
            else if (bCurAiIsMid
                     && plgcdi->ucDIRefreshRate == DI_MID_REFRESH_RATE)
            {
                /* middle. */
                if ((plgcdi->iForceSts == -1)  /* not force. */
                        &&(plgcdi->mod == RD_VT_BOX_DI
                           && ((pvAiMod >= (void *)&(aimodVtBox_g[0])) && (pvAiMod < (void *)&(aimodVtBox_g[iVtBoxCfgNum_g])))))
                {
                    iNow=VirtBox_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    VirtBox_Modify_DI(plgcdi,iNow);
                }
            }
            else if (bCurAiIsSlow
                     && plgcdi->ucDIRefreshRate == DI_SLOW_REFRESH_RATE)
            {
                /* slow */
                if ((plgcdi->iForceSts == -1)  /* not force. */
                        && (plgcdi->mod == RD_VT_BOX_DI
                            && ((pvAiMod >= (void *)&(aimodVtBox_g[0])) && (pvAiMod < (void *)&(aimodVtBox_g[iVtBoxCfgNum_g])))))
                {
                    iNow=VirtBox_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    *(pbSeventh+i)=iNow;
                    *(pbEighth+i)=iNow;
                    *(pbNinth+i)=iNow;
                    *(pbTenth+i)=iNow;
                    *(pbEleventh+i)=iNow;
                    *(pbTwelvth+i)=iNow;
                    VirtBox_Modify_DI(plgcdi,iNow);
                }
            }
        }
    }

    return;
}

/* Get the DO buffer base address of virtual box.
 * Para:
 *     iPos, position of virtual box.
 * Return:
 *     base address of DO buffer,
 *     the driver will read the every bit status of DO buffer according to physical address of module.
 */
uint8_t *getVirtBoxDoBufBaseAddr(uint8_t iPos)
{
    return apVtBoxCfgArr_g[iPos].aucDoSts;
}

/* Set the DI buffer base address of virtual box.
 * Para:
 *     iPos, position of virtual box.
 * Return:
 *     base address of DI buffer,
 *     the driver will fill in the every bit status of DI buffer according to physical address of module.
 */
uint8_t *SetVirtBoxDiBufBaseAddr(uint8_t iPos)
{
    return apVtBoxCfgArr_g[iPos].aucDiSts;
}

/* If the data is valid on the virtual box.
 * Para:
 *     iPos, position of virtual box.
 *     ulAiCnt, the newest valid AI counter of virtual box.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VTBOX_Get_AI_Data_Valid(int iPos, uint32_t ulAiCnt)
{
    uint32_t ulNextCntWork = 0;
    VIRT_BOX_CH_STS *pstsWork = NULL;
    uint32_t ulCntDiff;
    VIRT_BOX_CH_STS *pstsReturn;
    int iLockKey;

    if (iPos<iVtBoxCfgNum_g)
    {
        iLockKey=intLock();        /* data integrality. */
        ulNextCntWork=aimodVtBox_g[iPos].ulNextCnt;			/* the newest of virtual box, equal to local. */
        pstsWork=aimodVtBox_g[iPos].pVirtChStsDbWork;
        intUnlock(iLockKey);
    }
    else
    {
        assert(FALSE);
    }

    ulCntDiff = ulNextCntWork-1-ulAiCnt;

    if (ulCntDiff>0x7fffffff)
    {
        return FALSE;
    }

    pstsReturn=(VIRT_BOX_CH_STS *)((uint8_t*)pstsWork-ulCntDiff*virtstsdb_g.uiChBytes);

    if (pstsReturn<virtstsdb_g.pBufBgn)
    {
        pstsReturn=(VIRT_BOX_CH_STS *)((uint8_t*)pstsReturn+virtstsdb_g.ulBufBytes);
    }

    if (!(pstsReturn >= virtstsdb_g.pBufBgn && pstsReturn<virtstsdb_g.pBufEnd))
    {
        static uint32_t ulCnt1=0;

        if ((ulCnt1&0xFFFFFF) == 0)
        {
            LOG_Dbg_Msg("ERROR: Get virtual box(name %s, address %d) valid status is OverTime，access is beyond, Dif is %d\n",
                        (int)aimodVtBox_g[iPos].aucVtBoxFd, aimodVtBox_g[iPos].iVtBoxAddrInSameKind, ulCntDiff, 0, 0, 0);
        }
        ulCnt1++;

        return FALSE;
    }

    return pstsReturn->bValid;
}

/* Get the sampling clock according to the virtual box AI counter.
 * Para:
 *     pvAiMod, handle of virtual box.
 *     ulScanAiCnt, local virtual box counter.
 * Return:
 *     match sampling clock in 10 cycles counter.
 * Alert:
 *     The parameter AI counter must be smaller than the newest AI counter.
 */
uint8_t VirtBox_GetMatchSamClkByAiCnt(void *pvAiMod, uint32_t ulScanAiCnt)
{
    uint8_t ucClkDiff;
    uint32_t ulClkDiff;
    uint8_t ucMatchSamClk = 0;
    uint8_t ucHeadClk;
    RD_AI_MOD *pCurAiMod;
    int iLockKey;

    assert (pvAiMod);

    pCurAiMod = (RD_AI_MOD *)pvAiMod;
    assert (((int)(pCurAiMod->ulNextCnt-1-ulScanAiCnt)) >= 0);

    if ((pCurAiMod >= &(aimodVtBox_g[0])) && (pCurAiMod < &(aimodVtBox_g[MAX_VT_BOX_COUNT])))
    {
        iLockKey=intLock();        /* data integrality. */
        ulClkDiff=(pCurAiMod->ulNextCnt-1-ulScanAiCnt);
        ucClkDiff=(uint8_t)ulClkDiff;
        ucHeadClk=(uint8_t)(pCurAiMod->ulHeadClk);

        ucMatchSamClk=SynSamAdjust(ucHeadClk, -(int)ucClkDiff);

        intUnlock(iLockKey);
    }
    else
    {
        assert(FALSE);
    }

    return ucMatchSamClk;
}