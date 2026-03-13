/* measure.c - This file contains interface to measuring */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 27feb07, dy modifying the transfering method to station.
01a, 8nov05, ghx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains interface to measuring.
INCLUDE: measure.h
*/

/* includes */

#include "measure.h"
#include "logic.h"
#include "miscfunc.h"
#include "realdata.h"
#include "swcfg.h"
#include "view.h"
#include "filetool.h"
#include "FileCRC.h"
#include "sysinfo.h"

#include <stdio_compat.h>
#include "string_compat.h"
#include <taskLib.h>
#include <ioLib.h>
#include <dirent_compat.h>
#include "RE_PublicDataDef.h"		/* DY 11/18/2006 */
#include "detailoperate_log.h"

/* define */

#define MEA_AI_DB_NUM 32    /* 遥测上送缓冲数组大小 */
#define MAX_MEA_NUM 2048

#define MEA_61850_QUESTIONABLE_FLAG 0xC000  /* 61850遥测品质,不可信 */
#define MEA_61850_OUTORANGE_FLAG 0x1000     /* 61850遥测品质,超量程 */

/* globals */

int iMeaValueNum_g;
int iCkSetNum_g;

SC_SET_ITEM *pCksetWr_g; /* 待写入测控定值 */
SC_SET_ITEM * pCkset_g;

/* locals */

static int iRunMeaAi_g;          /* Number of really running Mea_AIs. 实际使用的测量AI总数*/

/*090421 by xts 去除相关的static属性*/
ME_MEA_VALUE_CFG *pmeacfg_g;
ME_MEA_AI_DB *pmeadb_g;
ME_MEA_AI_DATA_DB **ppmeadatadb_g;         /* 遥测上送循环缓冲队列 */
static ME_MEA_AI_DATA_DB ***pppmeadatadb_g; /*指针二维数组,测量量越限上送使用,存放每次上送的测量量信息指针的循环缓冲队列,大小为iMeaValueNum_g*MEA_AI_DB_NUM */
static SEM_ID semMsuAi;

static uint32_t s_ulMeaCalcTm = 0;  /* 遥测量计算时间 */

/* global functions */

EP_STATUS ME_Cfg_Measure_Gain(int iFd);

EP_STATUS ME_Cfg_Measure_Value(uint8_t *pucCfg, uint32_t ulLen)
{
    uint8_t *puc;
    uint8_t aucModId[MAX_ID_LEN+1];
    ME_MEA_VALUE_CFG *pmeacfg;
    ME_MEA_AI_DB *pmeadb;
    int iItemCfgLen;
    BOOL bNeedCfg;
    int iFd;		/* DY 11/18/2006 */
    STATUS sts;
    STATUS vxsts;
    int i;

    puc=pucCfg;

    iMeaValueNum_g=U8_TO_U16(puc[1], puc[0]);
    assert(iMeaValueNum_g<MAX_MEA_NUM);
    puc+=6;

    if ((pmeacfg_g=calloc(iMeaValueNum_g, sizeof(*pmeacfg_g)))==NULL)
        return EP_BUF_ERR;

    if ((pmeadb_g=calloc(iMeaValueNum_g, sizeof(*pmeadb_g)))==NULL)
        return EP_BUF_ERR;

    if((ppmeadatadb_g = calloc(MEA_AI_DB_NUM, sizeof(*ppmeadatadb_g))) == NULL)
    {
        return EP_BUF_ERR;
    }
    if ((pppmeadatadb_g = calloc(MEA_AI_DB_NUM, sizeof(*pppmeadatadb_g))) == NULL)
    {
        return EP_BUF_ERR;
    }

    for (i = 0; i < MEA_AI_DB_NUM; i++)
    {
        if ((ppmeadatadb_g[i]=calloc(iMeaValueNum_g, sizeof(**ppmeadatadb_g)))==NULL)
            return EP_BUF_ERR;
        if ((pppmeadatadb_g[i]=calloc(iMeaValueNum_g, sizeof(**pppmeadatadb_g)))==NULL)
            return EP_BUF_ERR;
    }
    for (pmeacfg=pmeacfg_g,pmeadb=pmeadb_g;
            pmeacfg<pmeacfg_g+iMeaValueNum_g; pmeacfg++,pmeadb++)
    {
        iItemCfgLen=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pmeacfg->aucId, puc+1, puc[0]);
        puc+=1+puc[0];

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pmeacfg->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        pmeacfg->ucArith=*puc++;

        pmeacfg->ucParaSetMode=*puc++;
        if(pmeacfg->ucParaSetMode&0x01)
        {
            iItemCfgLen-=1+puc[0];
            EP_ID_Copy(pmeacfg->aucRtMaxSettingId,  puc+1, puc[0]);
            puc+=1+puc[0];
        }
        if(pmeacfg->ucParaSetMode&0x02)
        {
            iItemCfgLen-=1+puc[0];
            EP_ID_Copy(pmeacfg->aucRtMinSettingId,  puc+1, puc[0]);
            puc+=1+puc[0];
        }
        if(pmeacfg->ucParaSetMode&0x04)
        {
            iItemCfgLen-=1+puc[0];
            EP_ID_Copy(pmeacfg->aucOvMaxSettingId,  puc+1, puc[0]);
            puc+=1+puc[0];
        }
        if(pmeacfg->ucParaSetMode&0x08)
        {
            iItemCfgLen-=1+puc[0];
            EP_ID_Copy(pmeacfg->aucOvMinSettingId,  puc+1, puc[0]);
            puc+=1+puc[0];
        }
        pmeacfg->ucHmSeq=*puc++;		/* 谐波次数 */
        puc+=1;

        pmeacfg->ucUnit=*puc++;

        pmeacfg->fRtMax=BYTES_TO_FLT(puc);
        puc+=4;

        pmeacfg->fRtMin=BYTES_TO_FLT(puc);
        puc+=4;

        pmeacfg->fOvMax=BYTES_TO_FLT(puc);
        puc+=4;

        pmeacfg->fOvMin=BYTES_TO_FLT(puc);
        puc+=4;

        pmeacfg->aucABRV[0]=*puc++;
        pmeacfg->aucABRV[1]=*puc++;
        pmeacfg->aucABRV[2]=*puc++;
        pmeacfg->aucABRV[3]=*puc++;
        pmeacfg->bNotNeedUpSend=((*puc)&0x01)?TRUE:FALSE ;
        puc+=1;

        puc++;

        pmeacfg->fChgCoff=BYTES_TO_FLT(puc);
        puc+=4;

        assert(iItemCfgLen==33);

        pmeacfg->fPlusCoff=1.0;
        pmeacfg->fOffCoff=0.0;		/* 初始应该是零 */

        pmeadb->ucArith=pmeacfg->ucArith;
        pmeadb->ucUnit=pmeacfg->ucUnit;
        pmeadb->uiCode=pmeadb-pmeadb_g;
        pmeadb->pvAiHnd=RD_Msu_AI_Hnd(pmeacfg->aucId);
        if (pmeadb->pvAiHnd)
        {
            assert(IS_MA_CPLX_AI(AI_HND_TO_UNIT(pmeadb->pvAiHnd)) /*|| AI_HND_TO_UNIT(pmeadb->pvAiHnd)==0x28*/);
            assert(/*pmeadb->ucArith==0 ||*/pmeadb->ucArith==1 || pmeadb->ucArith==2);

            /*if (!pmeadb->ucArith)
            {
                assert( pmeacfg->ucUnit==0x28);
            }*/
        }

    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    bNeedCfg=FALSE;
    if ((i=FT_Rd_Sys_INI("[SYSTEM]", "NeedClCof", aucModId, 30))==1)
    {
        for (puc=aucModId; *puc; puc++)
            *puc=tolower(*puc);

        if (!strcmp(aucModId, "1") ||
                !strcmp(aucModId, "true") || !strcmp(aucModId, "yes"))
            bNeedCfg=TRUE;
    }
    else if(i==0)
    {
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "因[SYSTEM] NeedClCof值读取失败，创建新的系统INI文件.\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Because of failing ro read [SYSTEM] NeedClCof, create new ini file.\n", NULL);
        }

        FT_New_SYS_INI_File();
    }

    if ((iFd=open(EP_CL_GAIN_FILE, O_RDONLY, 0))!=ERROR)
    {
        sts=ME_Cfg_Measure_Gain(iFd);

        vxsts=close(iFd);
        assert(vxsts==OK);

        if (sts==EP_SUCCESS)
        {
            if (!bNeedCfg)
            {
                i=FT_Wr_Sys_INI("[SYSTEM]", "NeedClCof", "1");
                assert(i!=EP_ERROR);
            }
        }
        else if(bNeedCfg)
        {
            /* 防止程序中断 */

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                           "测量量增益系数文件无效\n",
                           0, 0);
                //LOG_Write(LOG_KERNEL, "文件错误(丢失/破坏): 无效的测量量增益系数文件!\n", NULL);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                           "invalid measurement quantity gain coefficient file\n",
                           0, 0);
                //LOG_Write(LOG_KERNEL, "File error(missing/corrupted): invalid measurement quantity gain coefficient file!\n", NULL);
            }

            SI_New_CL_Gain_Set();		/* 生成新的测量校准文件 */

            /* return EP_FILE_ERR; */
        }
        else
        {
            SI_New_CL_Gain_Set();		/* 生成新的测量校准文件 */
        }
    }
    else if (bNeedCfg)
    {
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                       "测量量增益系数文件无效\n",
                       0, 0);
            //LOG_Write(LOG_KERNEL, "文件错误(丢失/破坏): 不能打开测量量增益系数文件!\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                       "can't open measurement quantity gain coefficient file\n",
                       0, 0);
            //LOG_Write(LOG_KERNEL, "File error(missing/corrupted): can't open measurement quantity gain coefficient file!\n", NULL);
        }

        SI_New_CL_Gain_Set();		/* 生成新的测量校准文件 */

        /* return EP_FILE_ERR; */
    }
    semMsuAi=semMCreate(SEM_Q_PRIORITY);
    assert(semMsuAi!=NULL);
    return EP_SUCCESS;
}


EP_STATUS ME_Cfg_Measure_Gain(int iFd)
{
    uint8_t aucBuf[10];
    int i;
    uint8_t aucFlag[MAX_MEA_NUM];

    assert(iFd>=0);

    lseek(iFd, -4, SEEK_END);

    if (read(iFd, aucBuf, 4)!=4 ||
            aucBuf[0]!=0xA9 || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x58)
        return EP_FILE_ERR;

    lseek(iFd, 0, SEEK_SET);

    if (read(iFd, aucBuf, 10)!=10 ||
            aucBuf[0]!=0xD3 || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x2D|| (aucBuf[9]+aucBuf[8]*256)!=iMeaValueNum_g)
    {
        if((aucBuf[9]+aucBuf[8]*256) != iMeaValueNum_g)
        {

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                           "测量量增益系数文件无效\n",
                           0, 0);
                //LOG_Write(LOG_KERNEL, "文件错误(丢失/破坏): 因配置更新导致测量量配置个数改变，应重新整定系数文件!\n", NULL);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                           "Hardware AI channel gain coefficient file is invalid\n",
                           0, 0);
                // LOG_Write(LOG_KERNEL, "File error(missing/corrupted): Because of configuration change, the number of measure channel is changed, the coefficient must be set again!\n", NULL);
            }
        }
        return EP_FILE_ERR;
    }

    memset(aucFlag, 0, sizeof(aucFlag));
    for (i=0; i<iMeaValueNum_g; i++)
    {
        if (read(iFd, aucBuf, 9)==9 && i<iMeaValueNum_g && !aucFlag[i])
        {
            aucFlag[i]=0xFF;
            pmeacfg_g[i].fOffCoff=BYTES_TO_FLT(aucBuf+1);
            pmeacfg_g[i].fPlusCoff=BYTES_TO_FLT(aucBuf+5);
        }
        else
        {
            assert(FALSE);
            return EP_FILE_ERR;
        }
    }

    return EP_SUCCESS;
}


/* This function should be called after logic intialization. 测量模块初始化*/
EP_STATUS ME_Initialize(void)
{
    /*在逻辑图初始化之后，逻辑图运行之前，被调用，此时有些数据结构已申请过了  */
    ME_MEA_AI_DB *pmeadb;

    /*获得真正使用的测量AI量，未被投入的任务的测量AI量未被计入  */
    iRunMeaAi_g=iMeaValueNum_g;
    for (pmeadb=pmeadb_g; pmeadb<pmeadb_g+iMeaValueNum_g; pmeadb++)
    {
        if (!pmeadb->pvAiHnd || !pmeadb->pelmSrc)
            iRunMeaAi_g--;
    }

    return EP_SUCCESS;
}

EP_STATUS SCI_Init_Add_New_Measure_Signal(uint8_t *strID, EP_ELEM_IO *pMeasureSignal,
        uint32_t ulScanTaskNo)
{
    ME_MEA_VALUE_CFG *pmeacfg;
    ME_MEA_AI_DB *pmeadb;
    BOOL bFind;
    static uint32_t s_ulOnlyScanTaskNo = 0xFFFFFFFF; /* 遥测量计算任务号 */

    assert(strID && strlen(strID)<=MAX_ID_LEN);
    assert(pMeasureSignal);
    assert(ulScanTaskNo<MAX_SUB_LGC_NUM);

    /* 遥测量仅能配置在一个任务中,
     * 否则时标不准确
     * 已配置,后续应配置在一个扫描任务中
     */
    if (s_ulOnlyScanTaskNo != 0xFFFFFFFF)
    {
        if (ulScanTaskNo != s_ulOnlyScanTaskNo)
        {
            return EP_NOT_INIT;
        }
    }
    s_ulOnlyScanTaskNo = ulScanTaskNo;

    bFind=FALSE;
    for (pmeacfg=pmeacfg_g; pmeacfg<pmeacfg_g+iMeaValueNum_g; pmeacfg++)
    {
        if (!strcmp(strID, pmeacfg->aucId))
        {
            pmeadb=pmeadb_g+(pmeacfg-pmeacfg_g);
            if (pmeadb->pelmSrc || pmeadb->pvAiHnd)
            {
                /* 若已经用过了，出错返回 */
                LOG_Dbg_Msg("ERROR: using Measure_AI \"%s\" more then once.\n",
                            (int)strID, 0, 0, 0, 0, 0);

                return EP_NOT_INIT;
            }
            pmeadb->pelmSrc=pMeasureSignal;
            pmeadb->ulScanTaskNo=ulScanTaskNo;
            if (IS_CPLX_AI(pMeasureSignal->ucAttrib))
            {
                bFind=TRUE;
                if (IS_RI_CPLX_AI(pMeasureSignal->ucAttrib))
                    pmeadb->bIsRiCplx=TRUE;

                if (!pmeadb->ucArith)
                {
                    LOG_Dbg_Msg("ERROR: arithmetic mismatch for Measure_AI \"%s\".\n",
                                (int)strID, 0, 0, 0, 0, 0);

                    return EP_PARM_ERR;
                }
            }
            else if (!bFind)
            {
                bFind=TRUE;

                if (pmeadb->ucArith || pmeacfg->ucUnit!=pMeasureSignal->ucAttrib)
                {
                    LOG_Dbg_Msg("ERROR: arithmetic mismatch for Measure_AI \"%s\".\n",
                                (int)strID, 0, 0, 0, 0, 0);

                    return EP_PARM_ERR;
                }
            }
            else
            {
                LOG_Dbg_Msg("ERROR: repeat logic ID \"%s\" in Measure_AI.\n",
                            (int)strID, 0, 0, 0, 0, 0);

                return EP_NOT_INIT;
            }
        }
    }
    if (bFind)
        return EP_SUCCESS;
    else
    {
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\" of Measure_AI.\n",
                    (int)strID, 0, 0, 0, 0, 0);

        return EP_BAD_DATA;
    }

}

void   SCI_Process_Cur_Logrp_Period_Measure(
    uint32_t  ulScanTaskNo,
    uint32_t ulGrpScanDriveInterval,		/* Scanning Interval. */
    uint32_t ulScnAiCnt		/* 进行本次逻辑图扫描时的AI采样计数器值 */
)
{
    ME_MEA_AI_DB *pmeadb;
    ME_MEA_AI_DATA_DB *pmeadatadb = NULL;
    ME_MEA_VALUE_CFG *pmeacfg;
    COMPLEX * pmeaTemp;
    STATUS vxsts;
    int iChgNum;  /*实际有越限的测量量个数*/
    static BOOL bTask[MAX_CREATE_RELAYFUNC_TASK_COUNT]; /*每个任务的测量量必须先处理过一次才能判断越限与否*/
    static uint32_t uiCount[MAX_CREATE_RELAYFUNC_TASK_COUNT]= {0};
    BOOL bFlag;
    VI_RUN_INFO *pinf;
    uint32_t ulCalcPeriod;
    static uint16_t s_uMeaCfgCnt = 0;   /* 遥测上送循环缓冲队列计数维护 */

    if (!iMeaValueNum_g)
        return;

    if (uiAppType_g != APP_PROT_MEA_MERGE)
    {
        ulCalcPeriod = uiAiRate_g;
    }
    else
    {
        /* 对于测控装置有时标准确度要求 */
        ulCalcPeriod = uiAiRate_g/100;
    }

    if (uiCount[ulScanTaskNo]++ != 0)
    {
        if ((uiCount[ulScanTaskNo]*ulGrpScanDriveInterval) > ulCalcPeriod)
        {
            uiCount[ulScanTaskNo] = 0;
        }
        return;
    }

    vxsts=taskLock();
    assert(vxsts==OK);

    for (pmeadb=pmeadb_g; pmeadb<pmeadb_g+iMeaValueNum_g; pmeadb++)
    {
        if (pmeadb->pelmSrc && pmeadb->ulScanTaskNo==ulScanTaskNo)
        {
            if (pmeadb->ucArith)
                pmeadb->xVal=pmeadb->pelmSrc->now.xVal;
            else
            {
                pmeadb->fVal=pmeadb->pelmSrc->now.fVal;

                /* 保护测控一体化装置, 读入品质 */
                if (uiAppType_g == APP_PROT_MEA_MERGE)
                {
                    uint16_t *pData;

                    pData = (uint16_t *)&pmeadb->pelmSrc->now.xVal;
                    pData +=  2;
                    pmeadb->usQuality = *pData;
                }
            }
        }
        else if (pmeadb->pvAiHnd)
        {
            if (pmeadb->ucArith)
            {
                pmeaTemp=RD_Msuc_R(pmeadb->pvAiHnd);
                pmeadb->xVal=*RD_Msuc_AI(pmeaTemp,0);
            }
            else
                assert(FALSE);
        }
    }
    vxsts=taskUnlock();
    assert(vxsts==OK);
    iChgNum=0;
    for (pmeadb=pmeadb_g,pmeacfg=pmeacfg_g,pmeadatadb=ppmeadatadb_g[s_uMeaCfgCnt]; pmeadb<pmeadb_g+iMeaValueNum_g; pmeadb++,pmeacfg++,pmeadatadb++)
    {
        if ((pmeadb->pelmSrc && pmeadb->ulScanTaskNo==ulScanTaskNo) ||pmeadb->pvAiHnd)
        {
            /*这些是有变化的值*/
            switch (pmeadb->ucArith)
            {
                case 0:
                    break;
                case 1:
                    if (pmeadb->bIsRiCplx)
                        /* pmeadb->fVal=pmeacfg->fPlusCoff*RI_CPLX_MOD(pmeadb->xVal); */
                        pmeadb->fVal=RI_CPLX_MOD(pmeadb->xVal);		/* 由应用层处理增益系数 */
                    else
                        /* pmeadb->fVal=pmeacfg->fPlusCoff*REAL(pmeadb->xVal);	*/	/* 增加pmeacfg->fPlusCoff* DY 1/9/2006 */
                        pmeadb->fVal=REAL(pmeadb->xVal);		/* 由应用层处理增益系数 */
                    break;

                case 2:  /*相对相角和绝对相角对测量来说统一处理成相角，对原始通道测量，
	                          这个值求出来就是相对相角，对中间测量就是求相角值*/
                    if (pmeadb->bIsRiCplx)
                        pmeadb->fVal=RI_CPLX_ANG(pmeadb->xVal);
                    else
                        pmeadb->fVal=IMAGE(pmeadb->xVal);
                    break;
                default:
                    assert(FALSE);
                    break;
            }
            pmeadb->ucAttr=0;
            bFlag=FALSE;

            if((pmeadb->fVal > pmeacfg->fOvMax)
                    || (pmeadb->fVal < pmeacfg->fOvMin))
            {
                pmeadb->usQuality |= MEA_61850_QUESTIONABLE_FLAG;
                pmeadb->usQuality |= MEA_61850_OUTORANGE_FLAG;
            }

            if(bTask[ulScanTaskNo] && fabs(pmeadb->fVal-pmeadb->fLstVal)>(pmeacfg->fOvMax*pmeacfg->fChgCoff))
            {
                pmeadb->ucAttr |=0x04;
                bFlag=TRUE;
            }

            /* 保护测控一体化装置, 品质位变化更新 */
            if (uiAppType_g == APP_PROT_MEA_MERGE)
            {
                if (bTask[ulScanTaskNo] && (pmeadb->usQuality != pmeadb->usLstQuality))
                {
                    bFlag = TRUE;
                    pmeadb->usLstQuality = pmeadb->usQuality;
                    pmeadb->ucAttr |= 0x08; /* 品质位变化上送 */
                }
            }

            if(pmeadb->fVal>0.0)
                pmeadb->ucAttr &=0xEF;
            else
                pmeadb->ucAttr |=0x10 ;
            if(bFlag && (uiAppType_g != APP_PROT_MEA_MERGE))
            {
                /* 应测控需求,遥测越限由应用自己触发 */
                pmeadatadb->ucAttr = pmeadb->ucAttr;
                pmeadatadb->fVal = pmeadb->fVal;
                pmeadatadb->ucUnit = pmeadb->ucUnit;
                pmeadatadb->uiCode = pmeadb->uiCode;
                pmeadatadb->usQuality = pmeadb->usQuality;
                pppmeadatadb_g[s_uMeaCfgCnt][iChgNum++]=pmeadatadb;
                pmeadb->fLstVal=pmeadb->fVal;			/* 上传后才更新 */
            }
        }
        /* pmeadb->fLstVal=pmeadb->fVal; */
    }

    bTask[ulScanTaskNo]=TRUE;

    s_ulMeaCalcTm = RD_AI_Cnt_To_us(ulScnAiCnt);

    if(iChgNum && (uiAppType_g != APP_PROT_MEA_MERGE))
    {
        pinf=VI_Run_Info_Wr_P();

        pinf->type=MEA_OVER;

        if(bViewModIsInit_g)				/* VI模块是否完成标志 */
        {
            pinf->bViewModIsInit=TRUE;
        }
        else
        {
            pinf->bViewModIsInit=FALSE;
        }

        pinf->msg.mea.ulTime=s_ulMeaCalcTm;

        pinf->msg.mea.ppcfg=pppmeadatadb_g[s_uMeaCfgCnt];
        pinf->msg.mea.ucCOT=1;
        pinf->msg.mea.ucMode=1;
        pinf->msg.mea.uiNum=(uint16_t)iChgNum;

        VI_End_Wr_Run_Info();
    }

    s_uMeaCfgCnt++;

    if(s_uMeaCfgCnt == MEA_AI_DB_NUM)
    {
        s_uMeaCfgCnt = 0;
    }
}

/* 功能:
 *      接口函数,用于应用触发遥测越限事件
 * 参数:
 *      iMeaCh, 遥测通道序号(从0开始)
 *      fMeaVal, 遥测值
 *      usMeaQuality, 遥测品质
 *      ulMeaCalcTm, 遥测越限事件时标
 * 返回:
 *      无
 */
void ME_New_Mea_Over(int iMeaCh, float fMeaVal, uint16_t usMeaQuality, uint32_t ulMeaCalcTm)
{
    VI_RUN_INFO *pinf;
    ME_MEA_AI_DATA_DB *pmeadatadb = NULL;
    ME_MEA_VALUE_CFG *pmeacfg;
    ME_MEA_AI_DB *pmeadb;

    static uint16_t s_uMeaCfgCnt = 0;   /* 遥测上送循环缓冲队列计数维护 */

    if(uiAppType_g != APP_PROT_MEA_MERGE)
    {
        /* 该函数为测控专用 */
        return;
    }

    pmeadatadb=ppmeadatadb_g[s_uMeaCfgCnt];
    pmeadb = pmeadb_g+iMeaCh;
    pmeacfg = pmeacfg_g+iMeaCh;

    pmeadb->ucAttr = 0;
    pmeadb->ucAttr |=0x04;

    pmeadb->usQuality = usMeaQuality;
    if((pmeadb->fVal > pmeacfg->fOvMax)
            || (pmeadb->fVal < pmeacfg->fOvMin))
    {
        pmeadb->usQuality |= MEA_61850_QUESTIONABLE_FLAG;
        pmeadb->usQuality |= MEA_61850_OUTORANGE_FLAG;
    }

    if(pmeadb->usLstQuality != pmeadb->usQuality)
    {
        pmeadb->usLstQuality = pmeadb->usQuality;
        pmeadb->ucAttr |= 0x08; /* 品质位变化上送 */
    }

    pmeadb->fVal = fMeaVal;
    if(pmeadb->fVal>0.0)
        pmeadb->ucAttr &=0xEF;
    else
        pmeadb->ucAttr |=0x10 ;

    pmeadatadb->ucAttr = pmeadb->ucAttr;
    pmeadatadb->fVal = pmeadb->fVal;
    pmeadatadb->ucUnit = pmeadb->ucUnit;
    pmeadatadb->uiCode = pmeadb->uiCode;
    pmeadatadb->usQuality = pmeadb->usQuality;

    pppmeadatadb_g[s_uMeaCfgCnt][0] = pmeadatadb;

    pinf=VI_Run_Info_Wr_P();

    pinf->type=MEA_OVER;

    if(bViewModIsInit_g)				/* VI模块是否完成标志 */
    {
        pinf->bViewModIsInit=TRUE;
    }
    else
    {
        pinf->bViewModIsInit=FALSE;
    }

    pinf->msg.mea.ulTime=ulMeaCalcTm;

    pinf->msg.mea.ppcfg=pppmeadatadb_g[s_uMeaCfgCnt];
    pinf->msg.mea.ucCOT=1;
    pinf->msg.mea.ucMode=1;
    pinf->msg.mea.uiNum=1;

    VI_End_Wr_Run_Info();

    s_uMeaCfgCnt++;
    if(s_uMeaCfgCnt == MEA_AI_DB_NUM)
    {
        s_uMeaCfgCnt = 0;
    }

    return;
}

/* Read all measurement AIs' value.
 * Parameter:
 *      pfRslt, to save all Measure_AIs' current value.
 * Return value:
 *      None.
 * Alert:
 *      pfRslt must contains space to save iMeaValueNum_g float numbers. */
void ME_Rd_Mea_AI_Val(float *pfRslt)
{

    ME_MEA_AI_DB *pmeadb;
    STATUS vxsts;

    if (!iMeaValueNum_g)
        return;

    vxsts=semTake(semMsuAi, WAIT_FOREVER);
    assert(vxsts==OK);
    for (pmeadb=pmeadb_g; pmeadb<pmeadb_g+iMeaValueNum_g; pmeadb++)
    {
        *pfRslt++=pmeadb->fVal;
    }
    vxsts=semGive(semMsuAi);
    assert(vxsts==OK);
}

/***********************************************************************
* RD_Mea_AI - Read all measurment value
*
* RETURNS: None
*
* Alert:
*        pmeaRslt must contains space to save iMeaValueNum_g members.
*/
void RD_Mea_AI(
    RD_AI_MEA *pmeaRslt		/* to save all measurement value */
)
{
    ME_MEA_AI_DB *pmeadb;
    STATUS vxsts;

    if (!iMeaValueNum_g)
        return;

    vxsts=semTake(semMsuAi, WAIT_FOREVER);
    assert(vxsts==OK);
    for (pmeadb=pmeadb_g; pmeadb<pmeadb_g+iMeaValueNum_g; pmeadb++)
    {
        pmeaRslt->fVal=pmeadb->fVal;
        pmeaRslt->ucUnit=pmeadb->ucUnit;
        pmeaRslt->ucAttr=pmeadb->ucAttr;
        pmeaRslt->usQuality = pmeadb->usQuality; /* 品质位 */
        pmeaRslt++;
    }
    vxsts=semGive(semMsuAi);
    assert(vxsts==OK);
}

int ME_Get_Msu_Num(void)
{
    return iMeaValueNum_g;
}

/* Get measurement DI attribution.
 * Parameters:
 *      iIdx, index of the Measure value(from 0).
 * Return value:
 *      Pointer to the Measure value attribution structure.
 *      NULL if iIdx is invalid(>=iMeaVauleNum_g). */
const ME_MEA_VALUE_CFG *ME_Get_Msu_Value_Attr(int iIdx)
{
    if (iIdx<iMeaValueNum_g)
        return pmeacfg_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

int ME_Get_Msu_Idx(uint8_t *pStrID)
{
    ME_MEA_VALUE_CFG *pmeacfg;
    BOOL bFind;

    assert(pStrID && strlen(pStrID)<=MAX_ID_LEN);

    bFind=FALSE;
    for (pmeacfg=pmeacfg_g; pmeacfg<pmeacfg_g+iMeaValueNum_g; pmeacfg++)
    {
        if (!strcmp(pStrID, pmeacfg->aucId))
        {
            return pmeacfg-pmeacfg_g;
        }
    }
    return -1;
}

/*根据测量量序号返回测量量的增益系数*/
void ME_Get_Msu_PlusCoff(int iIdx, float *pfRt)
{

    ME_MEA_VALUE_CFG *pCfg;

    pCfg=pmeacfg_g+iIdx;
    *pfRt=pCfg->fPlusCoff;
}

/*根据测量量序号设置测量量的增益系数*/
void ME_Set_Msu_PlusCoff(int iIdx, float fCoff)
{
    ME_MEA_VALUE_CFG *pCfg;

    pCfg=pmeacfg_g+iIdx;
    pCfg->fPlusCoff=fCoff;
}

/*根据测量量序号返回测量量的增益系数*/
void ME_Get_Msu_OffCoff(int iIdx, float *pfRt)
{

    ME_MEA_VALUE_CFG *pCfg;

    pCfg=pmeacfg_g+iIdx;
    *pfRt=pCfg->fOffCoff;
}

/*根据测量量序号设置测量量的增益系数*/
void ME_Set_Msu_OffCoff(int iIdx, float fCoff)
{
    ME_MEA_VALUE_CFG *pCfg;

    pCfg=pmeacfg_g+iIdx;
    pCfg->fOffCoff=fCoff;
}

/*根据测量量序号设置测量量的越限系数*/
void ME_Set_Msu_ChgCoff(int iIdx, float fCoff)
{
    ME_MEA_VALUE_CFG *pCfg;

    pCfg=pmeacfg_g+iIdx;
    pCfg->fChgCoff=fCoff;
}

BOOL ME_Create_CoffFile(int nType)
{
    int iFd;
    uint8_t aucBuf[10];
    EP_STATUS sts;
    ME_MEA_VALUE_CFG *pmeacfg;
    int i,j;

    uint16_t ulCrc=0;

    iFd=FT_Bgn_Update(EP_CL_GAIN_FILE);
    assert(iFd>=0);

    aucBuf[0]=0xD3;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x2D;
    aucBuf[4]=LO8(GetInnerProtocolVer());
    aucBuf[5]=HI8(GetInnerProtocolVer());

    aucBuf[8]=HI8(iMeaValueNum_g);
    aucBuf[9]=LO8(iMeaValueNum_g);

    i=write(iFd, aucBuf, 10);
    assert(i==10);
    ulCrc =EP_CCITT_CRC16(aucBuf,10,ulCrc);

    j=0;
    for (pmeacfg=pmeacfg_g; pmeacfg<pmeacfg_g+iMeaValueNum_g; pmeacfg++)
    {
        aucBuf[0]=j++;
        FLT_TO_BYTES(aucBuf+1,pmeacfg->fOffCoff);
        FLT_TO_BYTES(aucBuf+5,pmeacfg->fPlusCoff);
        i=write(iFd, aucBuf, 9);
        assert(i==9);
        ulCrc =EP_CCITT_CRC16(aucBuf,9,ulCrc);
    }

    aucBuf[0]=0xA9;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x58;

    i=write(iFd, aucBuf, 4);
    assert(i==4);
    ulCrc =EP_CCITT_CRC16(aucBuf,4,ulCrc);

    sts=FT_End_Update(EP_CL_GAIN_FILE, iFd);
    assert(sts==EP_SUCCESS);

    sts=FT_Wr_INI_CRC("[CRC]",CRC_ITEM_CLCOF,ulCrc);
    assert (sts != EP_ERROR);

    if(nType == 0)
    {
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_OPRATE, "创建新的测量量增益系数文件.\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_OPRATE, "The new gain coefficients of measurement quantity is created!\n", NULL);
        }
    }
    else if(nType == 1)
    {
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_OPRATE, "创建新的测量量偏置系数文件.\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_OPRATE, "The new offset coefficient of measurement quantity is created!\n", NULL);
        }
    }
    else
        assert(FALSE);

    return EP_SUCCESS;
}

void ME_Chg_Msu_Some_Attrs(void)
{
    ME_MEA_VALUE_CFG *pmeacfg;
    SCI_SIGNAL_VALUE_TYPE settingvalue;
    EP_STATUS stsResult;
    STATUS vxsts;
    vxsts=taskLock();
    for (pmeacfg=pmeacfg_g;
            pmeacfg<pmeacfg_g+iMeaValueNum_g; pmeacfg++)
    {
        if(pmeacfg->ucParaSetMode&0x01)
        {
            stsResult=SCI_Get_Inner_Setting_BySettingStrBase(pmeacfg->aucRtMaxSettingId,&settingvalue);
            if(stsResult==EP_SUCCESS)
            {
                if(IS_INT32_SET(settingvalue.ucAttrib))		/* DY 11/18/2006 以下同 */
                    pmeacfg->fRtMax=settingvalue.Value.lVal;
                else if(IS_UINT32_SET(settingvalue.ucAttrib))
                    pmeacfg->fRtMax=settingvalue.Value.ulVal;
                else
                    pmeacfg->fRtMax=settingvalue.Value.fVal;
            }
            else
            {
                assert(FALSE);
            }
        }
        if(pmeacfg->ucParaSetMode&0x02)
        {
            stsResult=SCI_Get_Inner_Setting_BySettingStrBase(pmeacfg->aucRtMinSettingId,&settingvalue);
            if(stsResult==EP_SUCCESS)
            {
                if(IS_INT32_SET(settingvalue.ucAttrib))
                    pmeacfg->fRtMin=settingvalue.Value.lVal;
                else if(IS_UINT32_SET(settingvalue.ucAttrib))
                    pmeacfg->fRtMin=settingvalue.Value.ulVal;
                else
                    pmeacfg->fRtMin=settingvalue.Value.fVal;
            }
            else
            {
                assert(FALSE);
            }
        }
        if(pmeacfg->ucParaSetMode&0x04)
        {
            stsResult=SCI_Get_Inner_Setting_BySettingStrBase(pmeacfg->aucOvMaxSettingId,&settingvalue);
            if(stsResult==EP_SUCCESS)
            {
                if(IS_INT32_SET(settingvalue.ucAttrib))
                    pmeacfg->fOvMax=settingvalue.Value.lVal;
                else if(IS_UINT32_SET(settingvalue.ucAttrib))
                    pmeacfg->fOvMax=settingvalue.Value.ulVal;
                else
                    pmeacfg->fOvMax=settingvalue.Value.fVal;
            }
            else
            {
                assert(FALSE);
            }
        }
        if(pmeacfg->ucParaSetMode&0x08)
        {
            stsResult=SCI_Get_Inner_Setting_BySettingStrBase(pmeacfg->aucOvMinSettingId,&settingvalue);
            if(stsResult==EP_SUCCESS)
            {
                if(IS_INT32_SET(settingvalue.ucAttrib))
                    pmeacfg->fOvMin=settingvalue.Value.lVal;
                else if(IS_UINT32_SET(settingvalue.ucAttrib))
                    pmeacfg->fOvMin=settingvalue.Value.ulVal;
                else
                    pmeacfg->fOvMin=settingvalue.Value.fVal;
            }
            else
            {
                assert(FALSE);
            }
        }

    }
    vxsts=taskUnlock();
}

EP_STATUS ME_CK_Mea_Attrs(void)
{
    ME_MEA_VALUE_CFG *pmeacfg;
    EP_STATUS stsResult;
    BOOL bUsed;
    for (pmeacfg=pmeacfg_g;
            pmeacfg<pmeacfg_g+iMeaValueNum_g; pmeacfg++)
    {
        if(pmeacfg->ucParaSetMode&0x01)
        {
            stsResult=SC_Find_Setbase(pmeacfg->aucRtMaxSettingId,&bUsed);
            if(stsResult!=EP_SUCCESS)
            {
                logMsg("Error, No %d measure RtMaxsetbase %s  isn't cofigured in sy set !\n",
                       pmeacfg-pmeacfg_g,(int)pmeacfg->aucRtMaxSettingId,0,0,0,0);
                return EP_CFG_ERR;
            }
        }
        if(pmeacfg->ucParaSetMode&0x02)
        {
            stsResult=SC_Find_Setbase(pmeacfg->aucRtMinSettingId,&bUsed);
            if(stsResult!=EP_SUCCESS)
            {
                logMsg("Error, No %d measure RtMinsetbase %s  isn't cofigured in sy set !\n",
                       pmeacfg-pmeacfg_g,(int)pmeacfg->aucRtMinSettingId,0,0,0,0);
                return EP_CFG_ERR;
            }
        }
        if(pmeacfg->ucParaSetMode&0x04)
        {
            stsResult=SC_Find_Setbase(pmeacfg->aucOvMaxSettingId,&bUsed);
            if(stsResult!=EP_SUCCESS)
            {
                logMsg("Error, No %d measure OvMaxsetbase %s  isn't cofigured in sy set !\n",
                       pmeacfg-pmeacfg_g,(int)pmeacfg->aucOvMaxSettingId,0,0,0,0);
                return EP_CFG_ERR;
            }
        }
        if(pmeacfg->ucParaSetMode&0x08)
        {
            stsResult=SC_Find_Setbase(pmeacfg->aucOvMinSettingId,&bUsed);
            if(stsResult!=EP_SUCCESS)
            {
                logMsg("Error, No %d measure OvMinsetbase %s  isn't cofigured in sy set !\n",
                       pmeacfg-pmeacfg_g,(int)pmeacfg->aucOvMinSettingId,0,0,0,0);
                return EP_CFG_ERR;
            }
        }
    }
    return EP_SUCCESS;
}

EP_STATUS SC_Cfg_Ck_Set(uint8_t *pucCfg, uint32_t ulLen)
{
    int iItemCfgLen;
    int i;
    SC_SET_ITEM *pset;
    SC_SET_ITEM *psetWr;
    uint8_t *puc;


    puc=pucCfg;

    iCkSetNum_g=U8_TO_U16(puc[1], puc[0]);

    if ((pCkset_g=calloc(iCkSetNum_g, sizeof(*pCkset_g)))==NULL)
        return EP_BUF_ERR;

    /* 固化测控 */
    if ((pCksetWr_g = calloc(iCkSetNum_g, sizeof(*pCksetWr_g))) == NULL)
        return EP_BUF_ERR;

    puc+=6;

    for (pset=pCkset_g, psetWr = pCksetWr_g; pset<pCkset_g+iCkSetNum_g; pset++, psetWr++)
    {
        iItemCfgLen=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        pset->ucUnit=*puc++;

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pset->aucId, puc+1, puc[0]);
        puc+=1+puc[0];
        if(!strcmp(DESIDE_COEF_VALUE_SETTING_ID,pset->aucId) && pset->ucUnit==0)
        {
            if(!bCoefTailChg)
            {
                pCoefTailSet_g=pset;
                bCoefTailChg=TRUE;
            }
            else
            {
                LOG_Dbg_Msg("ERROR: control word setting of deciding ai model type has configured before.\n",
                            0, 0, 0, 0, 0, 0);

                assert(FALSE);
            }
        }

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pset->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        pset->aucABRV[0]=*puc++;
        pset->aucABRV[1]=*puc++;
        pset->aucABRV[2]=*puc++;
        pset->aucABRV[3]=*puc++;

        pset->bStdSet=(*puc&0x01)?TRUE:FALSE;		/* 国网标准 */
        pset->bAutoSet=(*puc&0x02)?TRUE:FALSE;				/* 自动整定 */
        puc+=16;
        iItemCfgLen-=puc[0];
        puc+=1+puc[0];
        puc+=4;
        iItemCfgLen-=puc[0];
        puc+=1+puc[0];
        puc+=4;
        iItemCfgLen-=puc[0];
        puc+=1+puc[0];

        puc+=2;                         /* TODO: is this SEQ. useful? */

        pset->valMax.ulVal=BYTES_TO_U32(puc);
        pset->valMaxOrg.ulVal=pset->valMax.ulVal;
        puc+=4;

        pset->valMin.ulVal=BYTES_TO_U32(puc);
        pset->valMinOrg.ulVal = pset->valMin.ulVal;
        puc+=4;

        pset->valDft.ulVal=BYTES_TO_U32(puc);
        pset->valDftOrg.ulVal = pset->valDft.ulVal;
        puc+=4;


        if(pset->ucUnit==0x68)
        {
            /*2008-7-16日，支持字符串定值，张云  */
            assert(pset->valMax.ulVal<=MAX_ID_LEN);
            assert(pset->valMin.ulVal<=MAX_ID_LEN);
            assert(pset->valDft.ulVal<=MAX_ID_LEN);

            strncpy(pset->aucDftStr,puc,pset->valDft.ulVal);
            strncpy(pset->aucNowStr,puc,pset->valDft.ulVal);
            puc+=pset->valDft.ulVal;
            iItemCfgLen-=pset->valDft.ulVal;
        }
        /* Init now value same as default.  It will be modified in
         * SC_Updt_Inner_Set called by SC_Chk_Set if inner setting file OK. */
        pset->valNow.ulVal=pset->valDft.ulVal;

        pset->valStep.ulVal=BYTES_TO_U32(puc);   /* 步长 */
        pset->valStepOrg.ulVal = pset->valStep.ulVal;
        puc+=4;

        i=U8_TO_U16(puc[1], puc[0]);
        assert(i<=1024);
        if ((pset->pucUnitName=malloc(i+1))==NULL)
            return EP_BUF_ERR;
        iItemCfgLen-=i;
        memcpy(pset->pucUnitName, puc+2, i);
        pset->pucUnitName[i]='\0';
        puc+=2+i;

        *psetWr = *pset;

        assert(iItemCfgLen==54);
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
        return EP_SUCCESS;
}

SC_SET_ITEM *SC_Rd_CK_Set(int iFd, u_int *piNum,BOOL *bhascrc)
{
    /*2013-8-21日 ZY 去掉assert调用*/
    SC_SET_ITEM *psetRet;
    SC_SET_ITEM *pset;
    uint8_t aucBuf[35];
    int iRdLen;
    int iIdx;
    int i;

    //assert(iFd>=0);
    if(!(iFd>=0))
    {
        return  NULL;
    }

    psetRet=NULL;
    *piNum=0;
    lseek(iFd, 0, SEEK_SET);

    if (read(iFd, aucBuf, 10)!=10 ||
            aucBuf[0]!=0xD1 || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x1F)
        goto reterr;

    if(aucBuf[6]==0x01)
        *bhascrc=TRUE;
    else
        *bhascrc=FALSE;
    *piNum=aucBuf[9];

    if(*bhascrc)
    {
        lseek(iFd, -6, SEEK_END);
        if (read(iFd, aucBuf, 4)!=4 ||
                aucBuf[0]!=0x6F || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x36)
            goto reterr;
    }
    else
    {
        lseek(iFd, -4, SEEK_END);
        if (read(iFd, aucBuf, 4)!=4 ||
                aucBuf[0]!=0x6F || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x36)
            goto reterr;

    }
    lseek(iFd, 10, SEEK_SET);

    if ((psetRet=calloc(*piNum, sizeof(*psetRet)))==NULL)
        goto reterr;

    for (iIdx=0; iIdx<*piNum; iIdx++)
    {
        iRdLen=read(iFd, aucBuf, 2);
        //assert(iRdLen==2 && aucBuf[0]<*piNum);
        if(!(iRdLen==2 && aucBuf[0]<*piNum))
        {
            goto reterr;
        }

        pset=psetRet+aucBuf[0];
        //assert(!pset->aucName[0]);
        if(!(!pset->aucName[0]))
        {
            goto reterr;
        }

        //assert(aucBuf[1]<MAX_ID_LEN);
        if(!(aucBuf[1]<MAX_ID_LEN))
        {
            goto reterr;
        }
        iRdLen=read(iFd, pset->aucName, aucBuf[1]);
        //assert(iRdLen==aucBuf[1]);
        if(!(iRdLen==aucBuf[1]))
        {
            goto reterr;
        }
        pset->aucName[iRdLen]='\0';

        iRdLen=read(iFd, aucBuf, 21);
        if(!(iRdLen==21))
        {
            goto reterr;
        }

        pset->aucABRV[0]=aucBuf[0];
        pset->aucABRV[1]=aucBuf[1];
        pset->aucABRV[2]=aucBuf[2];
        pset->aucABRV[3]=aucBuf[3];

        iRdLen=read(iFd, aucBuf, 14);
        if(!(iRdLen==14))
        {
            goto reterr;
        }
        pset->valStep.ulVal = BYTES_TO_U32(aucBuf);    /* 定值修改步长 */

        iRdLen=read(iFd, aucBuf, 13);/*2008-7-18日，支持字符串定值，张云修改  */
        //assert(iRdLen==13);
        if(!(iRdLen==13))
        {
            goto reterr;
        }

        pset->ucUnit=aucBuf[0];

        pset->valMax.ulVal=BYTES_TO_U32(aucBuf+1);

        pset->valMin.ulVal=BYTES_TO_U32(aucBuf+5);

        pset->valDft.ulVal=BYTES_TO_U32(aucBuf+9);


        if(pset->ucUnit==0x68)
        {
            /*2008-7-18日，支持字符串定值，张云  */
            if(pset->valDft.ulVal>MAX_ID_LEN)
            {
                LOG_Dbg_Msg("错误，字符串默认定值长度超出允许范围!",0,0,0,0,0,0);
                goto reterr;
            }
            iRdLen=read(iFd, pset->aucDftStr, pset->valDft.ulVal);
            //assert(iRdLen==pset->valDft.ulVal);
            if(!(iRdLen==pset->valDft.ulVal))
            {
                goto reterr;
            }

        }

        iRdLen=read(iFd, aucBuf, 4);
        //assert(iRdLen==4);
        if(!(iRdLen==4))
        {
            goto reterr;
        }
        pset->valNow.ulVal=BYTES_TO_U32(aucBuf);
        if(pset->ucUnit==0x68)
        {
            /*2008-7-18日，支持字符串定值，张云  */
            if(pset->valNow.ulVal>MAX_ID_LEN)
            {
                LOG_Dbg_Msg("错误，字符串当前定值长度超出允许范围!",0,0,0,0,0,0);
                goto reterr;
            }
            iRdLen=read(iFd,pset->aucNowStr, pset->valNow.ulVal);
            //assert(iRdLen==pset->valNow.ulVal);
            if(!(iRdLen==pset->valNow.ulVal))
            {
                goto reterr;
            }
        }

        iRdLen=read(iFd, aucBuf, 2); /*2008-7-18日，支持字符串定值，张云修改  */
        //assert(iRdLen==2);
        if(!(iRdLen==2))
        {
            goto reterr;
        }
        i=U8_TO_U16(aucBuf[1], aucBuf[0]);

        //assert(i<=1024);
        if(!(i<=1024))
        {
            goto reterr;
        }
        if ((pset->pucUnitName=malloc(i+1))==NULL)
            goto reterr;

        iRdLen=read(iFd, pset->pucUnitName, i);
        //assert(iRdLen==i);
        if(!(iRdLen==i))
        {
            goto reterr;
        }
        pset->pucUnitName[iRdLen]='\0';
    }

    return psetRet;

reterr:
    if (psetRet)
        SC_Free_Set_Mem(psetRet, *piNum);

    return NULL;
}
EP_STATUS SC_CK_Is_Valid(int iFd)
{
    SC_SET_ITEM *psetRd;
    int iNum;
    SC_SET_ITEM *psetOrg;
    SC_SET_ITEM *pset;
    SC_SET_ITEM *psetWr;
    BOOL bHasCRC;

    assert(iFd>=0);

    psetRd=SC_Rd_CK_Set(iFd, &iNum,&bHasCRC);

    if (!psetRd)
        return EP_BAD_DATA;
    if(bHasCRC)
    {
        if(!SC_Check_CRC(iFd))
        {
            SC_Free_Set_Mem(psetRd, iNum);
            return EP_BAD_DATA;
        }
    }
    if (iNum!=iCkSetNum_g)
    {
        SC_Free_Set_Mem(psetRd, iNum);
        return EP_BAD_DATA;
    }

    for (psetOrg=pCkset_g, pset=psetRd, psetWr = pCksetWr_g;
            pset<psetRd+iNum; psetOrg++, pset++, psetWr++)
    {
        /*原来代码 BUG，张云改过，2006-8-21  */
        if(!psetOrg->bAutoSet)
        {
            if (pset->ucUnit!=psetOrg->ucUnit ||
                    pset->aucABRV[0]!=psetOrg->aucABRV[0] ||
                    pset->aucABRV[1]!=psetOrg->aucABRV[1] ||
                    pset->aucABRV[2]!=psetOrg->aucABRV[2] ||
                    pset->aucABRV[3]!=psetOrg->aucABRV[3] ||
                    strcmp(pset->aucName, psetOrg->aucName) ||
                    strcmp(pset->pucUnitName, psetOrg->pucUnitName) ||
                    (IS_INT32_SET(pset->ucUnit) &&
                     ((pset->valNow.lVal>psetOrg->valMax.lVal ||
                       pset->valNow.lVal<psetOrg->valMin.lVal))) ||
                    (IS_UINT32_SET(pset->ucUnit) &&
                     ((pset->valNow.ulVal>psetOrg->valMax.ulVal ||
                       pset->valNow.ulVal<psetOrg->valMin.ulVal))) ||
                    (IS_FLT_SET(pset->ucUnit) &&
                     (pset->valNow.fVal>psetOrg->valMax.fVal+FLT_PRECISION ||
                      pset->valNow.fVal<psetOrg->valMin.fVal-FLT_PRECISION)))
            {
                SC_Free_Set_Mem(psetRd, iNum);
                return EP_BAD_DATA;
            }
            else
            {
                *psetWr = *pset;
            }
        }
    }
    SC_Free_Set_Mem(psetRd, iNum);
    return EP_SUCCESS;

}

/* 复位写入测控定值
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SC_Reset_CK_Set(void)
{
    int i;
    SC_SET_ITEM *psetWr = NULL;
    SC_SET_ITEM *pset = NULL;

    for (i = 0, psetWr = pCksetWr_g, pset = pCkset_g;
            i < iCkSetNum_g; i++, psetWr++, pset++)
    {
        psetWr->valNow = pset->valNow;
    }
}

EP_STATUS SC_Chg_Mem_CK_Set(int iFd)
{
    SC_SET_ITEM *psetRd;
    int iNum;
    SC_SET_ITEM *psetOrg;
    SC_SET_ITEM *pset;
    BOOL bHasCRC;

    assert(iFd>=0);

    psetRd=SC_Rd_CK_Set(iFd, &iNum,&bHasCRC);

    if (!psetRd)
        return EP_BAD_DATA;

    if (iNum!=iCkSetNum_g)
    {
        SC_Free_Set_Mem(psetRd, iNum);
        return EP_BAD_DATA;
    }
    CKSetModifiesToLog(psetRd,iNum);
    for (psetOrg=pCkset_g, pset=psetRd;
            pset<psetRd+iNum; psetOrg++, pset++)
    {
        /*如果用于修改比例系数等的字符串尾控制字被修改，需要重新
         修改ai、ao、遥测、测量配置和db中的这些值*/
        if(pCoefTailSet_g==psetOrg &&  psetOrg->valNow.ulVal!=pset->valNow.ulVal)
            bCoefTailChg=TRUE;
        if(!(psetOrg->bAutoSet))
            psetOrg->valNow.ulVal=pset->valNow.ulVal;

        /*2008-7-18日 张云 字符串定值  */
        if(psetOrg->ucUnit==0x68)
        {
            if(psetOrg->valNow.ulVal>MAX_ID_LEN)
            {
                SC_Free_Set_Mem(psetRd, iNum);
                return EP_BAD_DATA;
            }
            strncpy(psetOrg->aucNowStr,pset->aucNowStr,psetOrg->valNow.ulVal);
        }
    }

    SC_Free_Set_Mem(psetRd, iNum);

    SC_Updt_Each_GivenSetting_Decided_Item();
    return EP_SUCCESS;

}
EP_STATUS SC_Chg_CK_Set(int iFd)
{
    SC_SET_ITEM *psetRd;
    int iNum;
    SC_SET_ITEM *psetOrg;
    SC_SET_ITEM *pset;
    BOOL bHasCRC;

    assert(iFd>=0);

    psetRd=SC_Rd_CK_Set(iFd, &iNum,&bHasCRC);

    if (!psetRd)
        return EP_BAD_DATA;

    if (iNum!=iCkSetNum_g)
    {
        SC_Free_Set_Mem(psetRd, iNum);
        return EP_BAD_DATA;
    }

    if(bHasCRC)
    {
        if(!SC_Check_CRC(iFd))
        {
            SC_Free_Set_Mem(psetRd, iNum);
            return EP_BAD_DATA;
        }
    }

    for (psetOrg=pCkset_g, pset=psetRd;
            pset<psetRd+iNum; psetOrg++, pset++)
    {
        /*原来代码 BUG，张云改过，2006-8-21  */
        if(!psetOrg->bAutoSet)
            if (pset->ucUnit!=psetOrg->ucUnit ||
                    pset->aucABRV[0]!=psetOrg->aucABRV[0] ||
                    pset->aucABRV[1]!=psetOrg->aucABRV[1] ||
                    pset->aucABRV[2]!=psetOrg->aucABRV[2] ||
                    pset->aucABRV[3]!=psetOrg->aucABRV[3] ||
                    strcmp(pset->aucName, psetOrg->aucName) ||
                    strcmp(pset->pucUnitName, psetOrg->pucUnitName) ||
                    (IS_INT32_SET(pset->ucUnit) &&
                     ((pset->valNow.lVal>psetOrg->valMax.lVal ||
                       pset->valNow.lVal<psetOrg->valMin.lVal))) ||
                    (IS_UINT32_SET(pset->ucUnit) &&
                     ((pset->valNow.ulVal>psetOrg->valMax.ulVal ||
                       pset->valNow.ulVal<psetOrg->valMin.ulVal))) ||
                    (IS_FLT_SET(pset->ucUnit) &&
                     (pset->valNow.fVal>psetOrg->valMax.fVal+FLT_PRECISION ||
                      pset->valNow.fVal<psetOrg->valMin.fVal-FLT_PRECISION)))
            {
                SC_Free_Set_Mem(psetRd, iNum);
                return EP_BAD_DATA;
            }
    }

    for (psetOrg=pCkset_g, pset=psetRd;
            pset<psetRd+iNum; psetOrg++, pset++)
    {
        /*如果用于修改比例系数等的字符串尾控制字被修改，需要重新
         修改ai、ao、遥测、测量配置和db中的这些值*/
        if(pCoefTailSet_g==psetOrg &&  psetOrg->valNow.ulVal!=pset->valNow.ulVal)
            bCoefTailChg=TRUE;
        /*if(!(psetOrg->bAutoSet))*/
        psetOrg->valNow.ulVal=pset->valNow.ulVal;

        /*2008-7-18日 张云 字符串定值  */
        if(psetOrg->ucUnit==0x68)
        {
            if(psetOrg->valNow.ulVal>MAX_ID_LEN)
            {
                SC_Free_Set_Mem(psetRd, iNum);
                return EP_BAD_DATA;
            }
            strncpy(psetOrg->aucNowStr,pset->aucNowStr,psetOrg->valNow.ulVal);
        }
    }

    SC_Free_Set_Mem(psetRd, iNum);

    RE_SetLogSetChgCnt();

    SC_Updt_Each_GivenSetting_Decided_Item();
    return EP_SUCCESS;
}

SC_SET_ITEM *SC_Get_CK_Set(void)
{
    return pCkset_g;
}

EP_STATUS SCI_Get_CK_Setting(int16_t nNumInPage,
                             SCI_SIGNAL_VALUE_TYPE  *pRtSettingValue)
{
    SC_SET_ITEM *pset;

    pset=pCkset_g+nNumInPage;
    pRtSettingValue->Value.ulVal=pset->valNow.ulVal;
    pRtSettingValue->ucAttrib=pset->ucUnit;

    if(pset->ucUnit == VAR_STRING)
    {
        pRtSettingValue->pvSrc=pset->aucNowStr;/* 2008-7-16日 支持字符串定值，张云 */
    }
    else
    {
        pRtSettingValue->pvSrc = (void *)pset;
    }

    return EP_SUCCESS;
}

/***********************************************************************
* GetMsuCoff - 获取测量量相关系数，MMI调用
*
* RETURNS: NONE
*
*/
void GetMsuCoff(
    VI_AI_COFF *pMsuCoff		/* 分配iMeaValueNum_g个 */
)
{
    ME_MEA_VALUE_CFG *pmeacfg;
    STATUS vxsts;

    assert(pMsuCoff);
    vxsts=taskLock();
    assert(vxsts==OK);

    for (pmeacfg=pmeacfg_g;
            pmeacfg<pmeacfg_g+iMeaValueNum_g; pmeacfg++, pMsuCoff++)
    {
        pMsuCoff->fRtMax=pmeacfg->fRtMax;
        pMsuCoff->fRtMin=pmeacfg->fRtMin;
        pMsuCoff->fOvMax=pmeacfg->fOvMax;
        pMsuCoff->fOvMin=pmeacfg->fOvMin;
        pMsuCoff->fChgCoff=pmeacfg->fChgCoff;
    }

    vxsts=taskUnlock();
}

/***********************************************************************
* SCI_Init_Get_Set_Dest - 获取相关定值更新地址
*
* RETURNS: NONE
*
*/
EP_STATUS SCI_Init_Get_Set_Dest(
    char *pDestSetName,		/* 定值名 */
    FLT_U32_UNION **ppDataSrc				/* 定值数据指针 */
)
{
    SC_SET_ITEM *pset;
    BOOL bFindFlag=FALSE;

    for (pset=pCkset_g; pset<pCkset_g+iCkSetNum_g; pset++)
    {
        if((!strcmp(pDestSetName, pset->aucName)) && pset->bAutoSet)
        {
            /* 自动整定 */
            bFindFlag=TRUE;
            *ppDataSrc=&pset->valNow;
            break;
        }
    }

    if(!bFindFlag)
    {
        return EP_ERROR;
    }

    return EP_SUCCESS;
}

/* 获取遥测量计算时刻的时间
 * Para:
 *     NONE.
 * Return:
 *     计算时间.
 */
uint32_t ME_GetCalcTm(void)
{
    return s_ulMeaCalcTm;
}
