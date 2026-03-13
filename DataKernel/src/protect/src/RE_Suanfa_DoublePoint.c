
#include "logic.h"
#include "errtest.h"
#include "realdata.h"

#include "math_compat.h"
#include "AppInterface.h"

/* defines */

#define MAX_CHN_NUM 100  /* 最大允许通道数 */
#define ASDU_CHN_NUM 6       /* 单个ASDU通道数 */

/* globals */

/* 模拟量(AI)采样周期,次/秒 */
extern u_int uiAiRate_g;

/* Scan function of the example logic part. */
static void EDP_DoublePointProduce_Scan(struct tag_EP_ELEMENT *pelm);
static void EDP_DoublePointTreat_Scan(struct tag_EP_ELEMENT *pelm);
static void EDP_DoublePointTreat6_Scan(struct tag_EP_ELEMENT *pelm);
static void EDP_FT3Map_Scan(struct tag_EP_ELEMENT *pelm);
static void EDP_SuanfaWarnning_Scan(struct tag_EP_ELEMENT *pelm);

/* 平台算法测试扫描函数.
 * Para:
 *     pelm,图元.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, error.
 */
static void EDP_Alg_Test_Scan(struct tag_EP_ELEMENT *pelm);

static void EDP_Send_SMV_Scan(struct tag_EP_ELEMENT *pelm);

/* 数据发送 */
extern void smv92STD_SubsendLogic(int smvNo,UINT8 *pData);

/* FT3_1状态 */
extern BOOL Get_Ft3Pt1Sts(void);

/* FT3_2状态 */
extern BOOL Get_Ft3Pt2Sts(void);

/* FT3_3状态 */
extern BOOL Get_Ft3Pt3Sts(void);

/* FT3_4状态 */
extern BOOL Get_Ft3Pt4Sts(void);

/* 发送算法初始化.
 * Para:
 *     pelm, 图元.
 * Return:
 *     NONE.
 */
EP_STATUS EDP_Send_SMV(EP_ELEMENT *pelm)
{
    assert (pelm && (pelm->ucType == 0));

    /* 最大个数不能越限
     */
    if ((pelm->unInNum == 0) || (pelm->unInNum>MAX_CHN_NUM))
    {
        return EP_BAD_DATA;
    }

    pelm->Scan_Func=EDP_Send_SMV_Scan;
    return EP_SUCCESS;
}

/* 发送算法.
 * Para:
 *     pelm, 图元.
 * Return:
 *     NONE.
 */
static void EDP_Send_SMV_Scan(EP_ELEMENT *pelm)
{
    int chnnum = pelm->unInNum;  /* 实际通道数,初始化时已经进行限制 */
    int asdunum = chnnum/ASDU_CHN_NUM;  /* 每个ASDU包括6个通道 */
    int yushu = chnnum%ASDU_CHN_NUM;
    int i;
    INT16 data[MAX_CHN_NUM];

    if(yushu>0)
    {
        /* 总的ASDU数 */
        asdunum++;
    }

    /* 保证fVal传送的数据不超过16位
     * 进行范围判断,超限赋0
     */
    for(i=0; i<chnnum; i++)
    {
        if (pelm->ppioIn[i]->now.fVal > (32767.0-FLT_PRECISION))
        {
            data[i] = 32767;
        }
        else if (pelm->ppioIn[i]->now.fVal < (-32768.0+FLT_PRECISION))
        {
            data[i] = -32768;
        }
        else
        {
            data[i] = (INT16)(pelm->ppioIn[i]->now.fVal);
        }
    }

    smv92STD_SubsendLogic(asdunum-1, (UINT8 *)data);
}
EP_STATUS EDP_SuanfaWarnning(EP_ELEMENT *pelm)
{
    int i=0;
    assert(pelm && pelm->ucType==0);

    if (pelm->unInNum!=0)
    {
        LOG_Dbg_Msg("EDP_SuanfaWarnning input number error.\n", 0, 0, 0, 0, 0, 0);
        return EP_BAD_DATA;
    }

    for(i=0; i<16; i++)
        /* Set initial output. */
        pelm->aioOut[i].now.bVal=0;


    pelm->Scan_Func=EDP_SuanfaWarnning_Scan;


    return EP_SUCCESS;
}

/* Scan function of the logic part.
 Input:   pelm, working data area.
 Return:  None.

输出点号定义：
00 子cpu告警总信号
01 装置频繁上电
02 存储器错误
03 无效定值区
04 定值校验错误
05 开入开出异常
06 AD异常
07 程序校验错误
08 监视CPU告警
09 扩展机箱告警
10 FPGA配置错误
11 内部goose通信中断

*/
static void EDP_SuanfaWarnning_Scan(EP_ELEMENT *pelm)
{
    uint64_t ulSysErrFlag=0;

    ulSysErrFlag=GetSysErrFlag();

    if(ulSysErrFlag)
        pelm->aioOut[0].now.bVal=TRUE;

    if(ulSysErrFlag&(1L<<EV_POWER_ON))
        pelm->aioOut[1].now.bVal=TRUE;
    if(ulSysErrFlag&(1L<<EV_STORAGE_ERR))
        pelm->aioOut[2].now.bVal=TRUE;
    if(ulSysErrFlag&(1L<<EV_SECT_ERR))
        pelm->aioOut[3].now.bVal=TRUE;
    if(ulSysErrFlag&(1L<<EV_SET_ERR))
        pelm->aioOut[4].now.bVal=TRUE;
    if(ulSysErrFlag&(1L<<EV_DIDO_ERR))
        pelm->aioOut[5].now.bVal=TRUE;
    if(ulSysErrFlag&(1L<<EV_SAMPLE_ERR))
        pelm->aioOut[6].now.bVal=TRUE;

#ifndef SUBUNIT
    if(ulSysErrFlag&(1L<<EV_SOFTWARE_CHECK_ERR))
        pelm->aioOut[7].now.bVal=TRUE;
#endif

    if(ulSysErrFlag&(1L<<EV_WATCH_CPU_ALARM))
        pelm->aioOut[8].now.bVal=TRUE;
    if(ulSysErrFlag&(1L<<EV_EXT_COM_ALARM))
        pelm->aioOut[9].now.bVal=TRUE;
    if(ulSysErrFlag&(1L<<EV_FPGA_ERR))
        pelm->aioOut[10].now.bVal=TRUE;
    if(ulSysErrFlag&(1L<<EV_REL_GSE_A_NET_HALT))
        pelm->aioOut[11].now.bVal=TRUE;
}

EP_STATUS EDP_DoublePointProduce(EP_ELEMENT *pelm)
{
    assert(pelm && pelm->ucType==0);

    if (pelm->unInNum!=3)
    {
        LOG_Dbg_Msg("EDP_DoublePointProduce input number error.\n", 0, 0, 0, 0, 0, 0);
        return EP_BAD_DATA;
    }

    assert(pelm->ppioIn[0]);
    assert(pelm->ppioIn[1]);
    assert(pelm->ppioIn[2]);

    /* Set initial output. */
    pelm->aioOut[0].now.bVal=0;


    pelm->Scan_Func=EDP_DoublePointProduce_Scan;


    return EP_SUCCESS;
}

/* Scan function of the logic part.
 * Input:   pelm, working data area.
 * Return:  None. */
static void EDP_DoublePointProduce_Scan(EP_ELEMENT *pelm)
{
    BOOL bCloseInput1; /*合位继电器*/
    BOOL bOpenInput2; /*分位继电器*/
    int32_t  iSetInput3;
    BOOL bOutput=0;

    bCloseInput1=(BOOL)(pelm->ppioIn[0]->now.bVal);
    bOpenInput2=(BOOL)(pelm->ppioIn[1]->now.bVal);
    iSetInput3=(int)(pelm->ppioIn[2]->now.ulVal);
    if((bCloseInput1==TRUE)&&(bOpenInput2==FALSE))
    {
        bOutput=DP_TRUE;
    }
    else if((bCloseInput1==FALSE)&&(bOpenInput2==TRUE))
    {
        bOutput=DP_FALSE;
    }
    else if((bCloseInput1==TRUE)&&(bOpenInput2==TRUE))
    {
        bOutput=DP_INVALID_11;
    }
    else if((bCloseInput1==FALSE)&&(bOpenInput2==FALSE))
    {
        bOutput=DP_INVALID_00;
    }
    else
    {
        bOutput=DP_INVALID_11;
        LOG_Dbg_Msg("=======DoublePointProduce Input signal type Error!\n",0,0,0,0,0,0);
    }

    pelm->aioOut[0].now.bVal=bOutput;
}

EP_STATUS EDP_DoublePointTreat(EP_ELEMENT *pelm)
{
    assert(pelm && pelm->ucType==0);

    if (pelm->unInNum!=2)
    {
        LOG_Dbg_Msg("EDP_DoublePointTreat input number error.\n", 0, 0, 0, 0, 0, 0);
        return EP_BAD_DATA;
    }
    else
    {
        logMsg("EDP_DoublePointTreat init OK!.\n", 0, 0, 0, 0, 0, 0);
    }

    assert(pelm->ppioIn[0]);
    assert(pelm->ppioIn[1]);

    assert(pelm->ppioIn[0]->pvCh);

    /* Set initial output. */
    pelm->aioOut[0].now.bVal=FALSE;
    pelm->aioOut[1].now.bVal=FALSE;

    pelm->Scan_Func=EDP_DoublePointTreat_Scan;

    return EP_SUCCESS;
}

/* Scan function of the logic part.
 * Input:   pelm, working data area.
 * Return:  None. */
static void EDP_DoublePointTreat_Scan(EP_ELEMENT *pelm)
{
    BOOL bCloseOutput1;  /* 合位继电器 */
    BOOL bOpenOutput2;   /* 分位继电器 */
    BOOL bDpInput1;      /* 输入双点 */
    int32_t iSetInput2;  /* 定值输入 */

    bDpInput1=(BOOL)(pelm->ppioIn[0]->now.bVal);
    iSetInput2=(int)(pelm->ppioIn[1]->now.ulVal);
    if (bDpInput1 == DP_TRUE)
    {
        bCloseOutput1=TRUE;
        bOpenOutput2=FALSE;
    }
    else if (bDpInput1 == TRUE)
    {
        if(uiAppType_g == APP_INTEL_BOX)
        {
            bCloseOutput1=TRUE;
            bOpenOutput2=FALSE;
        }
        else if (bdType_g == BOARD_TYPE_E02)
        {
            bCloseOutput1=FALSE;
            bOpenOutput2=TRUE;
        }
        else
        {
            bCloseOutput1=TRUE;
            bOpenOutput2=FALSE;
        }
    }
    else if (bDpInput1 == DP_FALSE)
    {
        bCloseOutput1=FALSE;
        bOpenOutput2=TRUE;
    }
    else if (bDpInput1==FALSE)
    {
        if(uiAppType_g == APP_INTEL_BOX)
        {
            bCloseOutput1=FALSE;
            bOpenOutput2=TRUE;
        }
        if (bdType_g == BOARD_TYPE_E02)
        {
            bCloseOutput1=TRUE;
            bOpenOutput2=FALSE;
        }
        else
        {
            bCloseOutput1=FALSE;
            bOpenOutput2=TRUE;
        }
    }
    else if(bDpInput1==DP_INVALID_11)
    {
        bCloseOutput1=TRUE;
        bOpenOutput2=TRUE;
    }
    else if(bDpInput1==DP_INVALID_00)
    {
        bCloseOutput1=FALSE;
        bOpenOutput2=FALSE;
    }
    else
    {
        bCloseOutput1=FALSE;
        bOpenOutput2=FALSE;
        /* assert(0); */
        return;
    }

    pelm->aioOut[0].now.bVal=bCloseOutput1;
    pelm->aioOut[1].now.bVal=bOpenOutput2;

}


EP_STATUS EDP_DoublePointTreat6(EP_ELEMENT *pelm)
{
    assert(pelm && pelm->ucType==0);

    if (pelm->unInNum!=6)
    {
        LOG_Dbg_Msg("EDP_DoublePointTreat6 input number error.\n", 0, 0, 0, 0, 0, 0);
        return EP_BAD_DATA;
    }
    else
    {
        logMsg("EDP_DoublePointTreat init OK!.\n", 0, 0, 0, 0, 0, 0);
    }

    assert(pelm->ppioIn[0]);
    assert(pelm->ppioIn[1]);
    assert(pelm->ppioIn[2]);
    assert(pelm->ppioIn[3]);
    assert(pelm->ppioIn[4]);
    assert(pelm->ppioIn[5]);

    assert(pelm->ppioIn[0]->pvCh);
    assert(pelm->ppioIn[1]->pvCh);
    assert(pelm->ppioIn[2]->pvCh);
    assert(pelm->ppioIn[3]->pvCh);
    assert(pelm->ppioIn[4]->pvCh);
    assert(pelm->ppioIn[5]->pvCh);

    /* Set initial output. */
    pelm->aioOut[0].now.bVal=FALSE;
    pelm->aioOut[1].now.bVal=FALSE;
    pelm->aioOut[2].now.bVal=FALSE;
    pelm->aioOut[3].now.bVal=FALSE;
    pelm->aioOut[4].now.bVal=FALSE;
    pelm->aioOut[5].now.bVal=FALSE;
    pelm->aioOut[6].now.bVal=FALSE;
    pelm->aioOut[7].now.bVal=FALSE;
    pelm->aioOut[8].now.bVal=FALSE;
    pelm->aioOut[9].now.bVal=FALSE;
    pelm->aioOut[10].now.bVal=FALSE;
    pelm->aioOut[11].now.bVal=FALSE;
    pelm->aioOut[12].now.bVal=FALSE;

    pelm->Scan_Func=EDP_DoublePointTreat6_Scan;

    return EP_SUCCESS;
}

/* Scan function of the logic part.
 * Input:   pelm, working data area.
 * Return:  None. */
static void EDP_DoublePointTreat6_Scan(EP_ELEMENT *pelm)
{
    BOOL bCloseOutput[6];  /*合位继电器*/
    BOOL bOpenOutput[6];   /*分位继电器*/
    BOOL bDpInput[6];
    int i;

    for(i=0; i<6; i++)
    {
#if 0
        if(bDpInput[i]!=(pelm->ppioIn[i]->now.bVal))
        {
            sprintf(aucLogInfo, "双点%d 输入变位,从%d到%d!\n",i,bDpInput[i],(pelm->ppioIn[i]->now.bVal));
            LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
        }
#endif
        bDpInput[i]=(BOOL)(pelm->ppioIn[i]->now.bVal);
        if (bDpInput[i] == DP_TRUE)
        {
            bCloseOutput[i]=TRUE;
            bOpenOutput[i]=FALSE;
        }
        else if (bDpInput[i]==TRUE)
        {
            if(uiAppType_g == APP_INTEL_BOX)
            {
                bCloseOutput[i]=TRUE;
                bOpenOutput[i]=FALSE;
            }
            else if (bdType_g == BOARD_TYPE_E02)
            {
                bCloseOutput[i]=FALSE;
                bOpenOutput[i]=TRUE;
            }
            else
            {
                bCloseOutput[i]=TRUE;
                bOpenOutput[i]=FALSE;
            }
        }
        else if (bDpInput[i] == DP_FALSE)
        {
            bCloseOutput[i]=FALSE;
            bOpenOutput[i]=TRUE;
        }
        else if (bDpInput[i] == FALSE)
        {
            if(uiAppType_g == APP_INTEL_BOX)
            {
                bCloseOutput[i]=FALSE;
                bOpenOutput[i]=TRUE;
            }
            else if (bdType_g == BOARD_TYPE_E02)
            {
                bCloseOutput[i]=TRUE;
                bOpenOutput[i]=FALSE;
            }
            else
            {
                bCloseOutput[i]=FALSE;
                bOpenOutput[i]=TRUE;
            }
        }
        else if(bDpInput[i]==DP_INVALID_11)
        {
            bCloseOutput[i]=TRUE;
            bOpenOutput[i]=TRUE;
        }
        else if(bDpInput[i]==DP_INVALID_00)
        {
            bCloseOutput[i]=FALSE;
            bOpenOutput[i]=FALSE;
        }
        else
        {
            bCloseOutput[i]=FALSE;
            bOpenOutput[i]=FALSE;
            /* assert(0); */
            return;
        }
        pelm->aioOut[i*2].now.bVal=bCloseOutput[i];
        pelm->aioOut[i*2+1].now.bVal=bOpenOutput[i];
    }
}

/**
 * FT3转遥信的算法
 */
EP_STATUS EDP_FT3Map(EP_ELEMENT *pelm)
{
    assert(pelm && pelm->ucType==0);

    if((pelm->unInNum!=0)||(pelm->ucOutNum != 4))
    {
        LOG_Dbg_Msg("EDP_FT3Map input/output number error.\n", 0, 0, 0, 0, 0, 0);
        return EP_BAD_DATA;
    }
    else
    {
        logMsg("EDP_FT3Map init OK!.\n", 0, 0, 0, 0, 0, 0);
    }

    /* Set initial output. */
    pelm->aioOut[0].now.bVal = Get_Ft3Pt1Sts();
    pelm->aioOut[1].now.bVal = Get_Ft3Pt2Sts();
    pelm->aioOut[2].now.bVal = Get_Ft3Pt3Sts();
    pelm->aioOut[3].now.bVal = Get_Ft3Pt4Sts();

    pelm->Scan_Func = EDP_FT3Map_Scan;

    return EP_SUCCESS;
}

static void EDP_FT3Map_Scan(struct tag_EP_ELEMENT *pelm)
{
    pelm->aioOut[0].now.bVal = Get_Ft3Pt1Sts();
    pelm->aioOut[1].now.bVal = Get_Ft3Pt2Sts();
    pelm->aioOut[2].now.bVal = Get_Ft3Pt3Sts();
    pelm->aioOut[3].now.bVal = Get_Ft3Pt4Sts();
}

/* 平台算法测试入口函数.
 * Para:
 *     pelm,图元.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, error.
 */
EP_STATUS EDP_Alg_Test(EP_ELEMENT *pelm)
{
    LOG_Dbg_Msg("平台算法测试初始化!\n", 0, 0, 0, 0, 0, 0);
    pelm->Scan_Func = EDP_Alg_Test_Scan;

    return EP_SUCCESS;
}

/* 平台算法测试扫描函数.
 * Para:
 *     pelm,图元.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, error.
 */
static void EDP_Alg_Test_Scan(struct tag_EP_ELEMENT *pelm)
{
#define OUTPUT_INTERVAL  30  /* 秒钟 */
#define PERIOD_POINT 2  /* 周波点数 */

    static uint32_t ulCnt = 0;
#define MAX_ALT 5.0

    float fAlt;
    uint32_t *pChnStsAll = NULL;
    uint8_t *pChnSts = NULL;
    uint8_t *pForeSts = NULL;
    float *pf0;  /* 采样通道 */
    uint8_t *pSts;
    int i;
    float *pfOptValLocal;
    float *pfOptValCur;
    float *pfOptValLocal2;
    float *pfOptValCur2;

    /* 线路保护测试(本侧) */
#if 1
    pChnStsAll = RD_Get_Chn_Sts_All(pelm->ppioIn[1]->pvCh, pelm->pchart->ulScnAiCnt);
    pChnSts = RD_Get_Chn_Sts(pelm->ppioIn[1]->pvCh, pelm->pchart->ulScnAiCnt);
    pf0 = pelm->pchart->pfBase;
    pSts = RD_Cnvrt_AI_P_to_Sts_P(pf0);

    pfOptValLocal = RD_Lgc_AI_P(pelm->ppioIn[3]->pvCh, pelm->pchart->ulScnAiCnt);
    pfOptValCur = RD_Lgc_AI_P(pelm->ppioIn[3]->pvCh, pelm->pchart->ulOptCh1ValidAiCnt);

    pfOptValLocal2 = RD_Lgc_AI_P(pelm->ppioIn[3]->pvCh, pelm->pchart->ulScnAiCnt-1);
    pfOptValCur2 = RD_Lgc_AI_P(pelm->ppioIn[3]->pvCh, pelm->pchart->ulOptCh1ValidAiCnt-1);

    ulCnt++;
    if ((ulCnt % (uiAiRate_g*OUTPUT_INTERVAL)) == 1)
    {
        LOG_Dbg_Msg("EDP_Alg_Test_Scan Enter, Sts = %x %x!\n",
                    *pChnStsAll, *pChnSts, 0, 0, 0, 0);

        pForeSts = pChnSts;
        RD_SUB_LGC_AI_STS_P(pForeSts, 1);
        LOG_Dbg_Msg("前一点状态%x %x %x\n", *pForeSts, (int)pChnSts, (int)pForeSts, 0, 0, 0);

        LOG_Dbg_Msg("所有通道状态!\n", 0, 0, 0, 0, 0, 0);
        for (i = 0; i<iLgcAiChNum_g; i++)
        {
            LOG_Dbg_Msg("%x\n", *(pSts+4*i), 0, 0, 0, 0, 0);
        }

        LOG_Dbg_Msg("光差数据 %d %d %d %d %d %d\n",
                    pelm->pchart->ulScnAiCnt,
                    (int)((*pfOptValLocal)*1000),
                    pelm->pchart->ulOptCh1ValidAiCnt,
                    (int)((*pfOptValCur)*1000),
                    pelm->pchart->ulOptCh1MidResutValidAiCnt,
                    0);
    }

    ulCnt++;
    if ((ulCnt % (uiAiRate_g*OUTPUT_INTERVAL)) == 1)
    {
        LOG_Dbg_Msg("EDP_Alg_Test_Scan Enter!\n", 0, 0, 0, 0, 0, 0);
    }

    fAlt = RI_CPLX_MOD(pelm->ppioIn[0]->now.xVal);
    if (fAlt > MAX_ALT)
    {
        pelm->aioOut[0].now.bVal = TRUE;
        pelm->aioOut[2].now.bVal = TRUE;
    }
    else
    {
        pelm->aioOut[0].now.bVal = FALSE;
        pelm->aioOut[2].now.bVal = FALSE;
    }

    /* 状态标启动录波 */
    if (*pChnSts)
    {
        pelm->aioOut[3].now.bVal = TRUE;
    }
    else
    {
        pelm->aioOut[3].now.bVal = FALSE;
    }

    pelm->aioOut[1].now.fVal = fAlt;

    /* 输入至输出 */
    pelm->aioOut[4].now.bVal = pelm->ppioIn[2]->now.bVal;

    pelm->aioOut[5].now.fVal = *pfOptValLocal;
    pelm->aioOut[6].now.fVal = *pfOptValCur;

    pelm->aioOut[5].recbuf[0].fVal = *pfOptValLocal2;
    pelm->aioOut[6].recbuf[0].fVal = *pfOptValCur2;

    /* 快速事件处理 */
    if (fAlt > (MAX_ALT/5))
    {
        static uint32_t ulLstTime = 0;
        uint32_t ulCurTime = 0;
        static BOOL bSts = FALSE;

        ulCurTime = TM_Get_usCnt();
        if ((ulCurTime - ulLstTime) >= 500000)
        {
            bSts = !bSts;
            ulLstTime = ulCurTime;
        }
        pelm->aioOut[7].now.bVal = bSts;
    }
#endif

    /* 线路保护测试(对侧) */

#if 0

    ulCnt++;
    if ((ulCnt % (uiAiRate_g*OUTPUT_INTERVAL)) == 1)
    {
        LOG_Dbg_Msg("光差数据 %d %d %d %d %d %d\n",
                    pelm->pchart->ulScnAiCnt,
                    pelm->pchart->ulOptCh1ValidAiCnt,
                    pelm->pchart->ulOptCh1MidResutValidAiCnt,
                    0, 0, 0);
    }

#endif

    /* 主变保护测试 */
#if 0
    pChnSts = RD_Get_Chn_Sts(pelm->ppioIn[0]->pvCh, pelm->pchart->ulScnAiCnt);

    /* 状态标启动录波 */
    if (*pChnSts)
    {
        pelm->aioOut[0].now.bVal = TRUE;
    }
    else
    {
        pelm->aioOut[0].now.bVal = FALSE;
    }

    ulCnt++;
    if ((ulCnt % uiAiRate_g) == 1)
    {
        LOG_Dbg_Msg("EDP_Alg_Test_Scan Enter, Sts = %x!\n",
                    *pChnSts, 0, 0, 0, 0, 0);
    }
#endif
}
