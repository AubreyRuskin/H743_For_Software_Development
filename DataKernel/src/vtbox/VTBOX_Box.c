/* VTBOX_Box.c - subroutine library for virtual box */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 07nov08, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for virtual box.
INCLUDES: VTBOX_Box.h
*/

/* includes */

#include "vxWorks.h"			/* always first */
#include "realdata.h"
#include "VTBOX_Box.h"
#include "logic.h"
#include "errtest.h"

/* globals */

VIRT_BOX_AO_CFG BoxAoCfgVirt_g[MAX_VT_BOX_COUNT];  /* AO configuration. */

BOOL abVirtBoxChIsInitOver_g[MAX_VT_BOX_COUNT];  		/* initialization finishing flag. */

EP_ELEM_IO *apVirtBoxMidSrcAOPt_g[MAX_VT_BOX_COUNT][MAX_VTBOX_AO_NUM];  /* AO from logic middle variable. */

/* functions */

/* Initialize the virtual box AO configuration
 * Para:
 *     ucVtBoxPos, position of the virtual box, begin from 0.
 *     ucAoNum, number of channel.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS VtBox_InitAOCfg(uint8_t ucVtBoxPos, uint8_t ucAoNum)
{
    VIRT_BOX_AO_CFG *pBoxAOCfg;
    int i;
    VIRT_AO_CFG *pAOCfg;
    VTBOX_INFO *pVirtBox;

    if (ucVtBoxPos<iVtBoxCfgNum_g)
    {
        pBoxAOCfg = BoxAoCfgVirt_g+ucVtBoxPos;
        pVirtBox = &apVtBoxCfgArr_g[ucVtBoxPos];		/* Virtual box interface. */
    }
    else
    {
        assert(FALSE);

        return EP_ERROR;
    }

    assert (ucAoNum <= MAX_VTBOX_AO_NUM);

    pBoxAOCfg->iVirtBoxAONum = ucAoNum;		/* number of AO. */
    pVirtBox->iAoNum = ucAoNum;

    pBoxAOCfg->iVirtBoxAISrcAONum=0;
    pBoxAOCfg->iVirtBoxAISrcAONumTmp=0;
    pBoxAOCfg->iVirtBoxMidSrcAONum=0;
    pBoxAOCfg->iVirtBoxPreSrcAONum=0;

    pVirtBox->iOrgAISrcAoNum = 0;
    pVirtBox->iPreAISrcAoNum = 0;
    pVirtBox->iMidSrcAoNum = 0;

    pAOCfg=pBoxAOCfg->aVirtBoxAoCfg_g;

    for (i=0; i<MAX_VTBOX_AO_NUM; i++)
    {
        pAOCfg->ucAOHdCh = 0xff;
        pAOCfg->iAOSrcType=DA_VOID_SRC;
        pAOCfg->ucSrcAIHdCh=0xff;
        pAOCfg->pElemSrc=NULL;
        pAOCfg++;
    }

    return EP_SUCCESS;
}

/* Finish the AO configuration from logic graph variables.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, error.
 */
EP_STATUS VirtBox_AOCfgInitFinish(void)
{
    int i, j, k;
    VIRT_AO_CFG *pAOCfg;
    float fVirtualValue=0.0;
    uint8_t aucZerofloat[4];
    int16_t ucSn = 0;
    EP_ELEM_IO *ptmpIOElem;
    uint8_t tmpSn;

    FLT_TO_BYTES(aucZerofloat, fVirtualValue);

    for (i=0; i<iVtBoxCfgNum_g; i++)
    {
        ucSn = 0;
        /* configuration of AO from logic variable. */
        pAOCfg=BoxAoCfgVirt_g[i].aVirtBoxAoCfg_g;

        for (j=0; j<apVtBoxCfgArr_g[i].iAoNum; j++)
        {
            if (pAOCfg->iAOSrcType == DA_MID_SRC)
            {
                assert (pAOCfg->ucAOHdCh<BoxAoCfgVirt_g[i].iVirtBoxAONum);
                apVtBoxCfgArr_g[i].apMidSrcAOPt[ucSn]=pAOCfg->pElemSrc;
                apVtBoxCfgArr_g[i].pucMidSrcAoDataSN[ucSn] = pAOCfg->ucAOHdCh;
                ucSn++;
            }

            pAOCfg++;
        }

        /* composite. */
        for (j=0; j<apVtBoxCfgArr_g[i].iMidSrcAoNum-1; j++)
            for (k=0; k<apVtBoxCfgArr_g[i].iMidSrcAoNum-1-j; k++)
            {
                if (apVtBoxCfgArr_g[i].pucMidSrcAoDataSN[k]>apVtBoxCfgArr_g[i].pucMidSrcAoDataSN[k+1])
                {
                    ptmpIOElem = apVtBoxCfgArr_g[i].apMidSrcAOPt[k];
                    apVtBoxCfgArr_g[i].apMidSrcAOPt[k] = apVtBoxCfgArr_g[i].apMidSrcAOPt[k+1];
                    apVtBoxCfgArr_g[i].apMidSrcAOPt[k+1] = ptmpIOElem;

                    tmpSn = apVtBoxCfgArr_g[i].pucMidSrcAoDataSN[k];
                    apVtBoxCfgArr_g[i].pucMidSrcAoDataSN[k] = apVtBoxCfgArr_g[i].pucMidSrcAoDataSN[k+1];
                    apVtBoxCfgArr_g[i].pucMidSrcAoDataSN[k+1] = tmpSn;
                }
            }

    }

    return EP_SUCCESS;
}