/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*       OPT_Data.c                                    1.0                      */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了光纵数据模块的代码文件                                     */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                        */
/*                                                                              */
/*         张云       2006.2.8                创建文件1.0版本                 */
/*                                                                             */
/*                                                                              */
/********************************************************************************/


#include   "OPT_Data.h"
#include   "realdata.h"
#include   "OPT_VtBox.h"
#include   "OPT_Com.h"
#include   "intLib.h"
#include   "view.h"

OPT_BOX_IO_INFO   aOptBoxIoInfo_g[2];/*光纵通道虚拟机箱IO板信息，0为光纵通道1，1为光纵通道2  */

/* 逻辑图快速任务的光纵发送所需信息变量定义 ，需要赋初值，2006-11-11日  张云 */
OPT_FAST_LOGRP_TASK_SEND_INFO  OptFastLogrpTaskSendInfo_g= {FALSE,0};

OPT_CH_STS_DB    optstsdb_g;    /*2006-2-10  为光纵添加  */

/* 写入光纵虚拟机箱DI的开入板状态，处理变位等信息
 * 参数：   plgcdi，  DI句柄
 *          iNowVal,  此时DI值
 *          uiAiCnt,该数据刷新对应的本机AI采样时刻
 * 返回值： 无 */
/* */
static  void OPT_Modify_DI(RD_LGC_DI_CH *plgcdi,int  iNowVal,uint32_t   ulAiCnt);



/* 功能：获得光纵虚拟机箱当前最新有效的通道采样点AICNT号,  2006-11-12日张云修改
   参数：iOptCh  光纵通道号，0代表光纵通道1，1代表光纵通道2，其他无效
         pRtFastTaskMatchAiCnt,供返回对侧快速任务的匹配的AICNT
         ulLocalCnt,本地采样节拍
         pulOptNewestValidAiCnt, 最新有效节拍
   返回值：返回该光纵通道最新有效的AI CNT
*/
uint32_t OPT_AI_Cnt(int iOptCh, uint32_t *pRtFastTaskMatchAiCnt, uint32_t ulLocalCnt, uint32_t *pulOptNewestValidAiCnt)
{
    BOOL bCntDifIsPositive = FALSE; /* 对侧AICnt是否超前于本次AICnt标志 */

    *pulOptNewestValidAiCnt = aimodOpt_g[iOptCh].ulOptRefreshedCnt;
    *pRtFastTaskMatchAiCnt = aimodOpt_g[iOptCh].ulOptFastTaskMatchCnt;

    if(aimodOpt_g[iOptCh].ulOptRefreshedCnt > ulLocalCnt)
    {
        /* 对侧数据超前于本侧数据 */
        bCntDifIsPositive = TRUE;
    }
    else
    {
        bCntDifIsPositive = FALSE;
    }

    /* 对侧最新数据超前
     */
    if ((!aLostFrmStatInfo_g[iOptCh].bNewComIsLongHaltInStat) && bCntDifIsPositive)
    {
        return  ulLocalCnt;
    }
    else
    {
        return aimodOpt_g[iOptCh].ulOptRefreshedCnt;
    }
}




/* 功能：获得光纵虚拟机箱某时刻通道数据接收有效与否和采样同步与否标志，张云2006-7-28日改动，2006-11-14日 张云
   参数：iOptCh  光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
         ulAiCnt 采样点号
         plRtTsse  用来返回通道有效时的采样同步误差（微秒）,
　　　　　　　　　　为正，表示本机采样领先，需要减慢采样，为负，表示本机采样落后，需要加快采样
         pbRtComValid  用来返回通道此时的通信是否有效标志，为TRUE，表示此刻最近有接收，通道接收正常，
                                                           为FALSE，表示通道接收异常
   返回值：该光纵通道数据采样同步有效与否
           若此刻通道通信正常并且接收到的同步数据正常，则返回TRUE
           若此刻光纵通道异常，或此刻同步数据无效，则返回FALSE
   注意：
          保护人员可以调用
*/
BOOL       OPT_Ch_Is_Valid(
    int  iOptCh,
    uint32_t   ulAiCnt,
    int32_t  *plRtTsse,
    BOOL  *pbRtComValid
)
{
    /* 2006-11-14日 张云修改 */
    uint32_t   ulNextCntWork;
    OPT_CH_STS *pstsWork;
    int32_t lCntDiff;
    OPT_CH_STS     *  pstsReturn;
    OPT_TIME_BASE  curTimeBase,lastTimeBase;/*2013-5-20  ZY  */
    int iLockKey;

    iLockKey = intLock();
    ulNextCntWork=aimodOpt_g[iOptCh].ulNextCnt;
    pstsWork=aimodOpt_g[iOptCh].pOptChStsDbWork;
    intUnlock(iLockKey);

    lCntDiff = ulNextCntWork-1-ulAiCnt;
    pstsReturn = (OPT_CH_STS *)((uint8_t *)pstsWork-lCntDiff*optstsdb_g.uiChBytes);
    if(pstsReturn<optstsdb_g.pBufBgn)
    {
        pstsReturn=(OPT_CH_STS *)((uint8_t*)pstsReturn+optstsdb_g.ulBufBytes);
    }
    else if (pstsReturn >= optstsdb_g.pBufEnd)
    {
        /* lCntDiff为负时存在超前可能性 */
        pstsReturn = (OPT_CH_STS *)((uint8_t *)pstsReturn-optstsdb_g.ulBufBytes);
    }

    /* 范围调整判断 */
    if (!(pstsReturn >= optstsdb_g.pBufBgn && pstsReturn<optstsdb_g.pBufEnd))
    {
        /* 若访问数据出界 */
        *plRtTsse = 30000;
        *pbRtComValid = FALSE;

        return FALSE;
    }

    *plRtTsse=pstsReturn->lTsse;
    /* 2013-5-20  ZY*/
    curTimeBase=OptGetBaseTimerCnt();
    lastTimeBase=OPT_GetOptChLastRecvValidDataTime(iOptCh);
    if((OptGetUsIntvlByBase(&curTimeBase,&lastTimeBase)>20000)
            || aLostFrmStatInfo_g[iOptCh].bNewComIsLongHaltInStat)
    {
        /*2006-11-14日 张云  若上次的接收是在20毫秒之前，则认为通信异常*/
        *pbRtComValid=FALSE;
    }
    else
    {
        /*最近有接收，则认为通信正常  */

        *pbRtComValid=TRUE;
    }

    /*2009-3-8日 ZY，允许无数据接收时，也判同步  */
    return   pstsReturn->bValid;
}


/* 功能：获得光纵虚拟机箱某时刻通道数据的通信稳定与否标志
         即通道通信时间是否稳定
   参数：iOptCh  光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
         ulAiCnt 采样点号
   返回:
 		TRUE，通信稳定
    FALSE，通信不稳定

  注意：
       保护人员可以调用
*/
BOOL OPT_Ch_Com_Sts(
    int iOptCh,
    uint32_t ulAiCnt
)
{
    uint32_t   ulNextCntWork;
    OPT_CH_STS *pstsWork;
    int32_t lCntDiff;
    OPT_CH_STS     *  pstsReturn;
    int iLockKey;

    iLockKey = intLock();
    ulNextCntWork=aimodOpt_g[iOptCh].ulNextCnt;
    pstsWork=aimodOpt_g[iOptCh].pOptChStsDbWork;
    intUnlock(iLockKey);

    lCntDiff = ulNextCntWork-1-ulAiCnt;
    pstsReturn = (OPT_CH_STS *)((uint8_t *)pstsWork-lCntDiff*optstsdb_g.uiChBytes);
    if(pstsReturn<optstsdb_g.pBufBgn)
    {
        pstsReturn=(OPT_CH_STS *)((uint8_t*)pstsReturn+optstsdb_g.ulBufBytes);
    }
    else if (pstsReturn >= optstsdb_g.pBufEnd)
    {
        /* lCntDiff为负时存在超前可能性 */
        pstsReturn = (OPT_CH_STS *)((uint8_t *)pstsReturn-optstsdb_g.ulBufBytes);
    }

    /* 调整之后范围判断 */
    if (!(pstsReturn >= optstsdb_g.pBufBgn && pstsReturn<optstsdb_g.pBufEnd))
    {
        /* 若访问数据出界 */
        return FALSE;
    }

    return pstsReturn->bComStable;
}



/* 功能：获得光纵虚拟机箱某时刻，通道的接收数据可信状态  2009-3-5 ZY
                                 和收发时间差是否变化的状态

   参数：iOptCh  光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
         ulAiCnt 采样点号
         piRtRcvSndChgDif,供返回前后两次通信稳定状态变化期间，通道的收发时间差变化值，单位US
            Value=本次稳定时真实收发时间差-上次稳定时真实收发时间差
            若为0，表示没有发生变化，或无法判定
            若非0，表示此次判定出来的收发时间差的变化值
   返回值：TRUE,表示对方送过来的数据内容可信，
           FALSE，表示对方送过来的数据内容不可信，
   注意：保护人员可以调用
*/
BOOL  OPT_Ch_Data_Is_Credible(
    int iOptCh,
    uint32_t ulAiCnt,
    int *  piRtRcvSndChgDif
)
{
    uint32_t   ulNextCntWork;
    OPT_CH_STS *pstsWork;
    int32_t lCntDiff;
    OPT_CH_STS     *  pstsReturn;
    int iLockKey;

    iLockKey = intLock();
    ulNextCntWork=aimodOpt_g[iOptCh].ulNextCnt;
    pstsWork=aimodOpt_g[iOptCh].pOptChStsDbWork;
    intUnlock(iLockKey);

    lCntDiff=ulNextCntWork-1-ulAiCnt;
    pstsReturn = (OPT_CH_STS *)((uint8_t *)pstsWork-lCntDiff*optstsdb_g.uiChBytes);
    if(pstsReturn<optstsdb_g.pBufBgn)
    {
        pstsReturn=(OPT_CH_STS *)((uint8_t*)pstsReturn+optstsdb_g.ulBufBytes);
    }
    else if (pstsReturn >= optstsdb_g.pBufEnd)
    {
        /* lCntDiff为负时存在超前可能性 */
        pstsReturn = (OPT_CH_STS *)((uint8_t *)pstsReturn-optstsdb_g.ulBufBytes);
    }

    /* 调整之后需另外判断 */
    if (!(pstsReturn >= optstsdb_g.pBufBgn && pstsReturn<optstsdb_g.pBufEnd))
    {
        /* 若访问数据出界 */
        *piRtRcvSndChgDif = 0;

        return 0;
    }

    *piRtRcvSndChgDif=pstsReturn->iRcvSndDiffChgTime;
    return pstsReturn->bDataIsCredible;
}



/*功能：获得光纵虚拟机箱的每秒丢帧统计
  参数： iOptCh  光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
         pulRtCalcLostFrmNumPerSec，供返回新的每秒丢帧统计数据
  返回：为真，表示本次有新的统计数据，为FALSE，表示本次无新统计数据，为脉冲信号，只供平台使用,只能由快速保护任务调用*/
BOOL       OPT_Ch_CalcLostFrm(int  iOptCh,uint32_t  * pulRtCalcLostFrmNumPerSec)
{
    BOOL   bNewCalcLostFrmFlag=FALSE;
    OPT_TIME_BASE  curTimeBase,lastTimeBase;

    /*2013-5-20日 ZY */
    curTimeBase=OptGetBaseTimerCnt();
    lastTimeBase=aLostFrmStatInfo_g[iOptCh].ulLastStatLostFrmUsCnt;
    if(OptGetUsIntvlByBase(&curTimeBase,&lastTimeBase)>1000000)
    {
        /*若每秒统计数据到时，  */
        bNewCalcLostFrmFlag=TRUE;
        if(aLostFrmStatInfo_g[iOptCh].bNewComIsLongHaltInStat)
        {
            /*若有长时间中断通信  */
            *pulRtCalcLostFrmNumPerSec=500;
        }
        else
        {
            /*若无长时间中断通信，  */
            *pulRtCalcLostFrmNumPerSec=aLostFrmStatInfo_g[iOptCh].ulLastStatLostFrmNum;
        }
        /*清零  */
        aLostFrmStatInfo_g[iOptCh].ulLastStatLostFrmUsCnt=OptGetBaseTimerCnt();//ZY,2013-5-20
        aLostFrmStatInfo_g[iOptCh].ulLastStatLostFrmNum=0;
        /* aLostFrmStatInfo_g[iOptCh].bNewComIsLongHaltInStat=FALSE; */
    }
    else
    {
        bNewCalcLostFrmFlag=FALSE;
        *pulRtCalcLostFrmNumPerSec=0;
    }

    return   bNewCalcLostFrmFlag;
}


/* 取得光纵虚拟机箱AI逻辑通道和预处理数据指针
 * 参数：   pvAiMod，用来索引光纵AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulSmplClk，同步到本机的递增采样时钟，该时钟在同步脉冲到来的时刻清零
 *          ppxWr，用来返回指向该光纵AI引擎的第0个预处理通道数据的指针
 *          ppstsWr,用来返回指向该光纵AI引擎的光纵通道状态的指针
 *          pAiCnt,用来返回指向该光纵AI引擎的光纵通道数据此刻对应的AICNT
 *          ucLastRecvCloseSamCnt, 最接近点.
 *          ucAllRltDif,延迟点数.
 * 返回值： 指向该光纵AI引擎的第0个逻辑采样通道数据的指针
 * 注意：   要求AI引擎按照采样次序调用此函数获得指针来写实时数据，
 *          AI引擎可以对指针进行直接的写操作（如果没有预处理通道，则*ppxWr无效），
 *          内部数据按照通道优先的方式进行组织 */
float *OPT_AI_Dat_P(void *pvAiMod, uint32_t ulSmplClk, COMPLEX **ppxWr,OPT_CH_STS **ppstsWr,uint32_t *pAiCnt, int8_t ucAllRltDif, uint8_t ucLastRecvCloseSamCnt)
{
    uint32_t   ulNextCntWork;
    uint32_t   ulHeadClkWork;
    float      *pfWork;
    COMPLEX    *pxWork;
    BOOL       *pbValidWork;
    OPT_CH_STS *pstsWork;
    float    *  pfReturn;
    COMPLEX  *  pxReturn;
    OPT_CH_STS     *  pstsReturn;
    BOOL    *  pbAiValid;
    int32_t lClkDiff;
    int iLockKey;
    RD_AI_MOD  *pCurAiMod;
    /*static  uint32_t  ulTestCnt_s=0;*/
    uint8_t ucDifCloseToCurCnt;

    pCurAiMod=(RD_AI_MOD  *)pvAiMod;
    iLockKey=intLock();        /*保持数据完整性  */
    ulNextCntWork=pCurAiMod->ulNextCnt;
    ulHeadClkWork=pCurAiMod->ulHeadClk;
    pfWork=pCurAiMod->pfWork;
    pxWork=pCurAiMod->pxWork;
    pbValidWork=pCurAiMod->pbAiValidDbWork;
    pstsWork=pCurAiMod->pOptChStsDbWork;
    intUnlock(iLockKey);

    /* 计算最接近点与当前处理点之差
     * 与已算出的延时累加得出总延时
     */
    if (ulHeadClkWork >= ucLastRecvCloseSamCnt)
    {
        ucDifCloseToCurCnt = ulHeadClkWork-ucLastRecvCloseSamCnt;
    }
    else
    {
        ucDifCloseToCurCnt = ulHeadClkWork-ucLastRecvCloseSamCnt+RD_SAM_SYN_CLK;
    }

    pCurAiMod->ucRltDif = ucAllRltDif+ucDifCloseToCurCnt;
    /* 本侧是否超前 */
    if (pCurAiMod->ucRltDif >= 0)
    {
        if(ulHeadClkWork>=ulSmplClk)
        {
            lClkDiff = ulHeadClkWork-ulSmplClk;
        }
        else
        {
            lClkDiff = ulHeadClkWork-ulSmplClk+RD_SAM_SYN_CLK;
        }
    }
    else
    {
        if (ulSmplClk >= ulHeadClkWork)
        {
            lClkDiff = -(ulSmplClk - ulHeadClkWork);
        }
        else
        {
            lClkDiff = -(ulSmplClk+RD_SAM_SYN_CLK-ulHeadClkWork);
        }
    }

    /*2006-6-9日，调试信息,很有必要 */
    if(fabs(lClkDiff)>80)  /*2006-11-14日张云扩大，为了做误码实验*/
    {
        /*若计算的采样差太大，说明计算出错，返回错误OPT_AI_Dat_P  *//*2006-11-14日张云扩大，为了做误码实验*/
        return  NULL;
    }

    pfReturn=(float*)((uint8_t*)pfWork-lClkDiff*lgcaidb_g.uiChBytes);
    pxReturn=(COMPLEX*)((uint8_t*)pxWork-lClkDiff*calcaidb_g.uiChBytes);
    pbAiValid=pbValidWork-lClkDiff*aivaliddb_g.uiTotalCh;
    pstsReturn=(OPT_CH_STS *)((uint8_t*)pstsWork-lClkDiff*optstsdb_g.uiChBytes);

    if(pfReturn<lgcaidb_g.pfBufBgn)
    {
        pfReturn=(float*)((uint8_t*)pfReturn+lgcaidb_g.ulBufBytes);
        pxReturn=(COMPLEX*)((uint8_t*)pxReturn+calcaidb_g.ulBufBytes);
        pbAiValid=pbAiValid+aivaliddb_g.ulBufLen;
        pstsReturn=(OPT_CH_STS *)((uint8_t*)pstsReturn+optstsdb_g.ulBufBytes);
    }
    else if (pfReturn >= lgcaidb_g.pfBufEnd)
    {
        /* lCntDiff为负时存在超前可能性 */
        pfReturn = (float *)((uint8_t *)pfReturn-lgcaidb_g.ulBufBytes);
        pxReturn = (COMPLEX *)((uint8_t *)pxReturn-calcaidb_g.ulBufBytes);
        pbAiValid = pbAiValid-aivaliddb_g.ulBufLen;
        pstsReturn = (OPT_CH_STS *)((uint8_t *)pstsReturn-optstsdb_g.ulBufBytes);
    }

    *pbAiValid=TRUE;
    *ppxWr=pxReturn;
    *ppstsWr=pstsReturn;
    *pAiCnt=ulNextCntWork-1-lClkDiff;

    return   pfReturn;
}


/* 报告光纵通道虚拟机箱AI引擎完成一次数据刷新  2006-11-12日 张云修改
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulAiCnt,该数据刷新对应的本机AI采样时刻
            ulFastTaskMatchAiCnt,对侧快速任务中间结果对应到本侧的AICNT
 * 返回值： 无 */
void OPT_End_Ai_Wr(void *pvAiMod,uint32_t   ulAiCnt,uint32_t  ulFastTaskMatchAiCnt)
{
    /*2006-11-12日 张云修改  */
    RD_AI_MOD   *pCurAiMod;

    pCurAiMod=(RD_AI_MOD   *)pvAiMod;

    taskLock();
    pCurAiMod->ulOptRefreshedCnt=ulAiCnt;/*设置光纵机箱数据有效AICNT */
    pCurAiMod->ulOptFastTaskMatchCnt=ulFastTaskMatchAiCnt;/*2006-11-12日 张云修改  */
    taskUnlock();

    return;
}


/* 刷新光纵通道虚拟机箱的DI数据
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          uiAiCnt,该数据刷新对应的本机AI采样时刻
 * 返回值： 无 */
void OPT_Refresh_DI(void *pvAiMod,uint32_t   ulAiCnt)
{
    /*这个函数需要优化  2006-5-27*/
    BOOL  bCurAiIsFast=FALSE;
    BOOL  bCurAiIsMid=FALSE;
    BOOL  bCurAiIsSlow=FALSE;
    static   uint32_t   ulRefreshCnt=0;

    assert(FALSE);
    assert(pvAiMod==&aimodOpt_g[0]||pvAiMod==&aimodOpt_g[1]);

    ulRefreshCnt++;

    if((ulRefreshCnt%DI_SLOW_REFRESH_INTERVAL)==1)
    {
        bCurAiIsSlow=TRUE;
    }
    if((ulRefreshCnt%DI_MID_REFRESH_INTERVAL)==1)
    {
        bCurAiIsMid=TRUE;
    }
    if((ulRefreshCnt%DI_FAST_REFRESH_INTERVAL)==1
            ||DI_FAST_REFRESH_INTERVAL==1)
    {
        bCurAiIsFast=TRUE;
    }

    if(bCurAiIsFast||bCurAiIsMid||bCurAiIsSlow)
    {
        /* 若需要刷新 */
        BOOL  *pbFirst;
        BOOL *pbSecond;
        BOOL *pbThird;
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
        int  i;
        int iNow;

        if(bCurAiIsSlow)
        {
            /*若是慢速，则获得相应的多次存储的基址  */
            pbFirst=RD_Base_His_DI_P(ulAiCnt-11);
            pbSecond=RD_Base_His_DI_P(ulAiCnt-10);
            pbThird=RD_Base_His_DI_P(ulAiCnt-9);
            pbForth=RD_Base_His_DI_P(ulAiCnt-8);
            pbFifth=RD_Base_His_DI_P(ulAiCnt-7);
            pbSixth=RD_Base_His_DI_P(ulAiCnt-6);
            pbSeventh=RD_Base_His_DI_P(ulAiCnt-5);
            pbEighth=RD_Base_His_DI_P(ulAiCnt-4);
            pbNinth=RD_Base_His_DI_P(ulAiCnt-3);
            pbTenth=RD_Base_His_DI_P(ulAiCnt-2);
            pbEleventh=RD_Base_His_DI_P(ulAiCnt-1);
            pbTwelvth=RD_Base_His_DI_P(ulAiCnt);

        }
        else  if(bCurAiIsMid)
        {
            pbFirst=RD_Base_His_DI_P(ulAiCnt-5);
            pbSecond=RD_Base_His_DI_P(ulAiCnt-4);
            pbThird=RD_Base_His_DI_P(ulAiCnt-3);
            pbForth=RD_Base_His_DI_P(ulAiCnt-2);
            pbFifth=RD_Base_His_DI_P(ulAiCnt-1);
            pbSixth=RD_Base_His_DI_P(ulAiCnt);

        }
        else
        {
            pbFirst=RD_Base_His_DI_P(ulAiCnt-2);
            pbSecond=RD_Base_His_DI_P(ulAiCnt-1);
            pbThird=RD_Base_His_DI_P(ulAiCnt);
        }

        for (plgcdi=plgcdich_g,i=0; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++,i++)
        {
            /*对每个ＤＩ通道判定，是否需要刷新  */

            if(plgcdi->ucDIRefreshRate==DI_FAST_REFRESH_RATE
                    &&bCurAiIsFast)
            {
                /*若是快速通道，且此次是快速刷新节拍，则快速刷新３点  */
                if ((plgcdi->iForceSts==-1)
                        &&((plgcdi->mod==RD_OPT1_DI&&pvAiMod==&aimodOpt_g[0])
                           ||(plgcdi->mod==RD_OPT2_DI&&pvAiMod==&aimodOpt_g[1])))
                {
                    /*若是相应光纵机箱的DI，且不是强制，则更新缓冲，若是强制，则在RD_Refresh_DI中进行了更新  */
                    iNow=OPT_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    OPT_Modify_DI(plgcdi,iNow,ulAiCnt);
                }

            }
            else  if(bCurAiIsMid
                     &&plgcdi->ucDIRefreshRate==DI_MID_REFRESH_RATE)
            {
                /*若是中速通道，且此次是中速刷新节拍，则中速刷新６点  */

                if ((plgcdi->iForceSts==-1)
                        &&((plgcdi->mod==RD_OPT1_DI&&pvAiMod==&aimodOpt_g[0])
                           ||(plgcdi->mod==RD_OPT2_DI&&pvAiMod==&aimodOpt_g[1])))
                {
                    iNow=OPT_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    OPT_Modify_DI(plgcdi,iNow,ulAiCnt);
                }
            }
            else if(bCurAiIsSlow
                    &&plgcdi->ucDIRefreshRate==DI_SLOW_REFRESH_RATE)
            {
                /*若是慢速通道，且此次是慢速刷新节拍，则慢速刷新１２点，若是其他，则对该通道空操作  */
                if ((plgcdi->iForceSts==-1)
                        &&((plgcdi->mod==RD_OPT1_DI&&pvAiMod==&aimodOpt_g[0])
                           ||(plgcdi->mod==RD_OPT2_DI&&pvAiMod==&aimodOpt_g[1])))
                {
                    iNow=OPT_Get_DI(plgcdi->pvSrc);
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
                    OPT_Modify_DI(plgcdi,iNow,ulAiCnt);
                }
            }/*else if(bCurAiIsSlow
        &&plgcdi->ucDIRefreshRate==DI_SLOW_REFRESH_RATE)结束  */
        }/*for循环结束  */

    }/*if结束 */

    return;
}



/* 写入光纵虚拟机箱DI的开入板状态，处理变位等信息
 * 参数：   plgcdi，  DI句柄
 *          iNowVal,  此时DI值
 *          uiAiCnt,该数据刷新对应的本机AI采样时刻
 * 返回值： 无 */
/* */
static  void OPT_Modify_DI(RD_LGC_DI_CH *plgcdi,int  iNowVal,uint32_t   ulAiCnt)
{
    /*设置开入摸件的32位开入值中的相关位  */
    if(!(iNowVal & 0x7FFF))
    {
        /*若为0  */
        plgcdi->p_part->ulDIModCurVaule &= (~(ulDwordBitArr_g[plgcdi->ucModCh]));
    }
    else
    {
        /*若为1  */
        plgcdi->p_part->ulDIModCurVaule |= (ulDwordBitArr_g[plgcdi->ucModCh]);
    }

    if (plgcdi->iVal!=iNowVal)
    {
        RE_SetLogDIUpdateCnt();
        if ((plgcdi->iVal & 0x7FFF)!=(iNowVal & 0x7FFF))
        {
            /* Report SOE. */
            if ((plgcdi->iVal | iNowVal) & 0x8000)
            {
                /*2013-5-23  ZY */
                TM_High_Get_Sys_Us_UTC_Time(&plgcdi->utChgTime,&plgcdi->ulChgTime);
                if (plgcdi->iMeaCh!=-1)
                {
                    VI_New_SOE(plgcdi->iMeaCh, (iNowVal & 0x7FFF), plgcdi->ulChgTime, plgcdi->bSOE, 0);
                }
            }
            else
            {
                plgcdi->ulChgTime=RD_AI_Cnt_To_us(ulAiCnt);

                if (plgcdi->iMeaCh!=-1)
                {
                    VI_New_SOE(plgcdi->iMeaCh, iNowVal, plgcdi->ulChgTime, plgcdi->bSOE, 0);
                }
            }
        }
        plgcdi->iVal=iNowVal;
    }
}



/* 初始化光纵虚拟机箱的IO
 * 参数：   iOptChNum ,光纵通道号，1为光纵通道1，2为光纵通道2，其他无效
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS  OPT_IO_Initialize(int  iOptCh)
{
    int   i;
    OPT_BOX_IO_INFO  *pBoxInfo;

    if(iOptCh==0)
    {
        pBoxInfo=aOptBoxIoInfo_g;
    }
    else if(iOptCh==1)
    {
        pBoxInfo=aOptBoxIoInfo_g+1;
    }
    else
    {
        assert(FALSE);
        return  EP_COM_ERR;
    }

    for(i=0; i<MAX_MOD_NUM; i++)
    {
        pBoxInfo->aOptIOModInfo[i].type=IDLE_MODULE;
        pBoxInfo->aOptIOModInfo[i].unDiChNum=0;
        pBoxInfo->aOptIOModInfo[i].unDoChNum=0;
    }
    for(i=0; i<OPT_MAX_DI_BUF_LEN; i++)
    {
        pBoxInfo->aucOptDiSts_g[i]=0;
    }
    for(i=0; i<OPT_MAX_DO_BUF_LEN; i++)
    {
        pBoxInfo->aucOptDoSts_g[i]=0;
    }
    pBoxInfo->iOptDiNum_g=0;
    pBoxInfo->iOptDoNum_g=0;
    return  EP_SUCCESS;
}

/* 初始化光纵虚拟机箱DI通道
 * 参数：   iOptCh,光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
            iModAddr，模块硬件地址
 *          uiCh，在本模件内的DO物理通道号，从0开始
 *          ulFilt，去抖动时间，单位us
 * 返回值： 用来索引DI通道的void指针，或者NULL表示调用出错 */
void *OPT_Init_DI(int iOptCh,int iModAddr, u_int uiCh, uint32_t ulFilt)
{
    OPT_BOX_IO_INFO  *pBoxInfo;
    OPT_DI_HND   *pDiHdl;

    assert((iModAddr>=OPT_DI_MOD_BASE_ADDR)
           &&(iModAddr<(OPT_DI_MOD_BASE_ADDR+MAX_OPT_DI_MOD_NUM)));
    assert(uiCh>=0&&uiCh<MAX_DI_PER_MOD);

    if(iOptCh==0)
    {
        pBoxInfo=aOptBoxIoInfo_g;
    }
    else if(iOptCh==1)
    {
        pBoxInfo=aOptBoxIoInfo_g+1;
    }
    else
    {
        assert(FALSE);
        return  NULL;
    }

    pBoxInfo->aOptIOModInfo[iModAddr].unDiChNum++;

    if(pBoxInfo->aOptIOModInfo[iModAddr].type==IDLE_MODULE)
    {
        pBoxInfo->aOptIOModInfo[iModAddr].type=DI_MODULE;
    }
    else  if(pBoxInfo->aOptIOModInfo[iModAddr].type!=DI_MODULE)
    {
        assert(FALSE);
        return  NULL;
    }

    pDiHdl=pBoxInfo->ahOptDiHandle+pBoxInfo->iOptDiNum_g;
    pDiHdl->iOptCh=iOptCh;
    pDiHdl->ucMod=iModAddr;
    pDiHdl->ucHdCh=uiCh;
    pDiHdl->aucFilt[0]=HH8(ulFilt);
    pDiHdl->aucFilt[1]=HL8(ulFilt);
    pDiHdl->aucFilt[2]=LH8(ulFilt);
    pDiHdl->aucFilt[3]=LL8(ulFilt);
    pDiHdl->pucDiStsPos=pBoxInfo->aucOptDiSts_g+(pDiHdl->ucMod-OPT_DI_MOD_BASE_ADDR)*4+(pDiHdl->ucHdCh)/8;
    pDiHdl->ucDiStsMsk=BV8((pDiHdl->ucHdCh)%8);

    pBoxInfo->iOptDiNum_g++;

    return   pDiHdl;
}

/* 初始化光纵虚拟机箱DO通道
 * 参数：   iOptCh,光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
 *          iModAddr，模件硬件地址,
 *          uiCh，在本模件内的DO物理通道号，从0开始
 * 返回值： 用来索引DO通道的void指针，或者NULL表示调用出错 */
void *OPT_Init_DO(int iOptCh,int iModAddr, u_int uiCh)
{
    OPT_BOX_IO_INFO  *pBoxInfo;
    OPT_DO_HND   *pDoHdl;

    assert((iModAddr>=OPT_DO_MOD_BASE_ADDR)
           &&(iModAddr<(OPT_DO_MOD_BASE_ADDR+MAX_OPT_DO_MOD_NUM)));
    assert(uiCh>=0&&uiCh<MAX_DO_PER_MOD);

    if(iOptCh==0)
    {
        pBoxInfo=aOptBoxIoInfo_g;
    }
    else if(iOptCh==1)
    {
        pBoxInfo=aOptBoxIoInfo_g+1;
    }
    else
    {
        assert(FALSE);
        return  NULL;
    }

    pBoxInfo->aOptIOModInfo[iModAddr].unDoChNum++;

    if(pBoxInfo->aOptIOModInfo[iModAddr].type==IDLE_MODULE)
    {
        pBoxInfo->aOptIOModInfo[iModAddr].type=DO_MODULE;
    }
    else  if(pBoxInfo->aOptIOModInfo[iModAddr].type!=DO_MODULE)
    {
        assert(FALSE);
        return  NULL;
    }

    pDoHdl=pBoxInfo->ahOptDoHandle+pBoxInfo->iOptDoNum_g;
    pDoHdl->iOptCh=iOptCh;
    pDoHdl->ucMod=iModAddr;
    pDoHdl->ucHdCh=uiCh;
    pDoHdl->pucDoStsPos=pBoxInfo->aucOptDoSts_g+(pDoHdl->ucMod-OPT_DO_MOD_BASE_ADDR)*4+(pDoHdl->ucHdCh)/8;
    pDoHdl->ucDoStsMsk=BV8(pDoHdl->ucHdCh%8);

    pBoxInfo->iOptDoNum_g++;

    return   pDoHdl;

}

/* 初始化光纵虚拟机箱AI源的AO通道
 * 参数：   iOptCh,光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
 *          uiCh，在光纵虚拟机箱AO模件内的AO物理通道号，从0开始
            uiSrcAiCh,该AO所代表的本地机箱AI的物理通道号
            fSrcAiPhyCoff,该AO所对应的本机AI的物理比例系数，已经乘过增益,2006-11-26 张云
 * 返回值： EP_STATUS  成功与否
            EP_SUCCESS,成功，其他失败
*/
EP_STATUS  OPT_Init_AI_Src_AO(int iOptCh,int  iSrcType, u_int uiCh,u_int  uiSrcAiCh,float fSrcAiPhyCoff)
{
    OPT_AO_CFG *   pAoCfg;
    if(iOptCh==0)
    {
        pAoCfg=&(BoxAoCfgOpt_g[0].aOptBoxAoCfg_g[BoxAoCfgOpt_g[0].iOptAISrcAONum+BoxAoCfgOpt_g[0].iOptMidSrcAONum]);
        pAoCfg->iAOSrcType=iSrcType;
        assert(uiCh<BoxAoCfgOpt_g[0].iOptAONum);
        pAoCfg->ucAOHdCh=uiCh;
        pAoCfg->ucSrcAIHdCh=uiSrcAiCh;

        /* 传统采样时需处理模拟/数字转换系数 */
        if (appType_g == APP_TYPE_TRAD)
        {
            pAoCfg->SrcAIPhyCoffUnion.fVal = fSrcAiPhyCoff*5.0/32768.0;
        }
        else if (appType_g == APP_TYPE_DIG)
        {
            pAoCfg->SrcAIPhyCoffUnion.fVal=fSrcAiPhyCoff*2.0;
        }
        else
        {
            assert (FALSE);
        }
        BoxAoCfgOpt_g[0].iOptAISrcAONum++;
        assert((BoxAoCfgOpt_g[0].iOptAISrcAONum+BoxAoCfgOpt_g[0].iOptMidSrcAONum)<=BoxAoCfgOpt_g[0].iOptAONum);
    }
    else  if(iOptCh==1)
    {
        /*OPT2不允许设置  */
        assert(FALSE);

    }
    else
    {
        assert(FALSE);
        return  EP_COM_ERR;
    }
    return  EP_SUCCESS;
}

/***********************************************************************
* OPT_Chg_AI_Src_AO_Coff - 更新OPT AO系数
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS OPT_Chg_AI_Src_AO_Coff(
    int iOptCh,		/* 光纵通道号 */
    float fSrcAiPhyCoff				/* 通道系数 */
)
{
    OPT_AO_CFG *pAoCfg;

    if(iOptCh == 0)
    {
        LOG_Dbg_Msg("通道%d系数更新,当前系数为%d\n", BoxAoCfgOpt_g[0].iOptAISrcAONumTmp, (int)(fSrcAiPhyCoff*1000), 0,0,0,0);
        pAoCfg=&(BoxAoCfgOpt_g[0].aOptBoxAoCfg_g[BoxAoCfgOpt_g[0].iOptAISrcAONumTmp]);		/* 先配置AI来源的AO，从0开始 */

        /* 传统采样时需处理模拟/数字转换系数 */
        if (appType_g == APP_TYPE_TRAD)
        {
            pAoCfg->SrcAIPhyCoffUnion.fVal = fSrcAiPhyCoff*5.0/32768.0;
        }
        else if (appType_g == APP_TYPE_DIG)
        {
            pAoCfg->SrcAIPhyCoffUnion.fVal=fSrcAiPhyCoff*2.0;
        }
        else
        {
            assert (FALSE);
        }

        BoxAoCfgOpt_g[0].iOptAISrcAONumTmp++;
    }
    else if(iOptCh == 1)
    {
        /* OPT2不允许设置  */
        assert(FALSE);
    }
    else
    {
        assert(FALSE);
        return EP_COM_ERR;
    }

    return EP_SUCCESS;
}

/***********************************************************************
* OPT_InitAOCfgCoff - 初始化光纵AO计数
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS OPT_InitAOCfgCount(
    int iOptChNum		/* 光纵通道号，0为光纵通道1，1为光纵通道2，其他无效 */
)
{
    OPT_BOX_AO_CFG *pBoxAOCfg = NULL;

    if(iOptChNum == 0)
    {
        pBoxAOCfg=BoxAoCfgOpt_g;
    }
    else if(iOptChNum == 1)
    {
        pBoxAOCfg=BoxAoCfgOpt_g+1;
    }
    else
    {
        assert(FALSE);
    }

    pBoxAOCfg->iOptAISrcAONumTmp=0;

    return  EP_SUCCESS;
}

/* 初始化光纵虚拟机箱中间结果源的AO通道
 * 参数：   iOptCh,光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
 *          pElemIOSrc,逻辑图中间结果输出指针
 * 返回值： EP_STATUS  成功与否
            EP_SUCCESS,成功，其他失败
*/
EP_STATUS  OPT_Init_Mid_Src_AO(int iOptCh,int  iSrcType, u_int uiCh, void  *pElemIOSrc)
{

    OPT_AO_CFG *   pAoCfg;
    if(iOptCh==0)
    {
        pAoCfg=&(BoxAoCfgOpt_g[0].aOptBoxAoCfg_g[BoxAoCfgOpt_g[0].iOptAISrcAONum+BoxAoCfgOpt_g[0].iOptMidSrcAONum]);
        pAoCfg->iAOSrcType=iSrcType;
        assert(uiCh<BoxAoCfgOpt_g[0].iOptAONum);
        pAoCfg->ucAOHdCh=uiCh;
        pAoCfg->pElemSrc=pElemIOSrc;
        BoxAoCfgOpt_g[0].iOptMidSrcAONum++;
        assert((BoxAoCfgOpt_g[0].iOptAISrcAONum+BoxAoCfgOpt_g[0].iOptMidSrcAONum)<=BoxAoCfgOpt_g[0].iOptAONum);
    }
    else  if(iOptCh==1)
    {
        /*OPT2允许设置  */

        pAoCfg=&(BoxAoCfgOpt_g[1].aOptBoxAoCfg_g[BoxAoCfgOpt_g[1].iOptAISrcAONum+BoxAoCfgOpt_g[1].iOptMidSrcAONum]);
        pAoCfg->iAOSrcType=iSrcType;
        assert(uiCh<BoxAoCfgOpt_g[1].iOptAONum);
        pAoCfg->ucAOHdCh=uiCh;
        pAoCfg->pElemSrc=pElemIOSrc;
        BoxAoCfgOpt_g[1].iOptMidSrcAONum++;
        assert((BoxAoCfgOpt_g[1].iOptAISrcAONum+BoxAoCfgOpt_g[1].iOptMidSrcAONum)<=BoxAoCfgOpt_g[1].iOptAONum);

    }
    else
    {
        assert(FALSE);
        return  EP_COM_ERR;
    }
    return  EP_SUCCESS;

}


/* 读取光纵虚拟机箱DI数据实时状态
 * 参数：   pvDiCh，用来索引DI数据元素的void指针，应该是OPT_Init_DI的返回值
 *          pulChgTime，变化到此状态的时间（us计数器值）
 * 返回值： 此DI通道的当前状态，TRUE=闭合；FALSE=打开 */
BOOL OPT_Get_DI(void *pvDiCh)
{
    OPT_DI_HND   *pDiHdl;

    assert(pvDiCh);
    pDiHdl=(OPT_DI_HND   *)pvDiCh;
    if((*(pDiHdl->pucDiStsPos))&(pDiHdl->ucDiStsMsk))
    {
        return  TRUE;
    }
    else
    {
        return  FALSE;
    }
}

/* 控制光纵虚拟机箱DO数据实时输出
 * 参数：   pvDoCh，用来索引DO数据元素的void指针，应该是OPT_Init_DO的返回值
 *          bClose，TRUE=控合；FALSE=控分
 * 返回值： 无 */
void OPT_Set_DO(void *pvDoCh, BOOL bClose)
{
    OPT_DO_HND   *pDoHdl;
    uint8_t   ucStsVal;
    uint8_t   ucStsMask;
    int iLockKey;

    assert(pvDoCh);
    pDoHdl=(OPT_DO_HND   *)pvDoCh;
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
    iLockKey=intLock();        /*保持数据完整性  */
    *(pDoHdl->pucDoStsPos)=ucStsVal;
    intUnlock(iLockKey);
    return;
}

/*光纵虚拟机箱是否配置了DI，DO
  参数，iOptCh,光纵通道号，0代表通道1，1代表通道2
  返回：TRUE，代表该机箱配置了DI，DO
        FALSE，代表该机箱未配置任何DI，DO*/
BOOL   OPT_BoxIsCfgDIO(int iOptCh)
{
    OPT_BOX_IO_INFO  *pBoxInfo = NULL;

    if(iOptCh==0)
    {
        pBoxInfo=aOptBoxIoInfo_g;
    }
    else if(iOptCh==1)
    {
        pBoxInfo=aOptBoxIoInfo_g+1;
    }
    else
    {
        assert(FALSE);
    }

    if((pBoxInfo->iOptDiNum_g)>0||(pBoxInfo->iOptDoNum_g>0))
    {
        return  TRUE;
    }
    else
    {
        return   FALSE;
    }
}



/* 设置快速逻辑图任务的所需的光纵发送信息  2006-11-11日 张云
      iScanTaskNo,扫描任务号，0为快速保护任务
      ulTaskScanAiCnt，该任务此时对应的采样点
    返回：EP_SUCCESS,成功
          其他，错误
    */
EP_STATUS  OPT_SetFastLogrpTaskSendInfo(int  iScanTaskNo,uint32_t  ulTaskScanAiCnt)
{
    OptFastLogrpTaskSendInfo_g.bFastLogrpTaskIsDrive=TRUE;
    OptFastLogrpTaskSendInfo_g.ulFastLogrpTaskAiCnt=ulTaskScanAiCnt;
    return  EP_SUCCESS;
}

/*由AICNT计数器，得到该计数器对应的SAM CLK， 2006-11-12日 张云
  参数  pvAiMod，用来索引光纵AI引擎的void指针，应该由本模块在初始
               化AI通道的时候提供给底层I/O
        ulScanAiCnt,待查询的采样计数器AICNT
  返回  该采样计数器对应的SAMCLK
注意：要求该AICNT比最新的AICNT小  */
uint8_t  OPT_GetMatchSamClkByAiCnt(void *pvAiMod,uint32_t  ulScanAiCnt)
{
    uint8_t  ucClkDiff;
    uint32_t ulClkDiff;
    uint8_t  ucMatchSamClk = 0;
    uint8_t  ucHeadClk;
    RD_AI_MOD  *pCurAiMod;
    int iLockKey;

    pCurAiMod=(RD_AI_MOD  *)pvAiMod;

    iLockKey=intLock();        /*保持数据访问完整性  */
    ulClkDiff=(pCurAiMod->ulNextCnt-1-ulScanAiCnt);
    ucClkDiff=(uint8_t)ulClkDiff;
    ucHeadClk=(uint8_t)(pCurAiMod->ulHeadClk);
    ucMatchSamClk=SynSamAdjust(ucHeadClk, -(int)ucClkDiff);
    intUnlock(iLockKey);

    return  ucMatchSamClk;
}




/* 功能：获得光纵虚拟机箱当前通信是否正常,2010-1-11日 张云
   参数：iOptCh  光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
         pRtChValidAiCnt,供返回通信正常时,最新接收同步后的采样节拍
   返回值：该光纵通道当前通信正常与否
           最近40毫秒有接收,通信正常，则返回TRUE
           否则返回FALSE
   注意：
          供其他模块调用,比如录波
*/
BOOL       OPT_ChIsComNormal(
    int  iOptCh,
    uint32_t  *pRtChValidAiCnt
)
{

    uint32_t    ulLocAICnt,ulOptAICnt,ulFstTskMatchAiCnt, ulOptNewestValidAiCnt;
    int  iAICntDif;

    *pRtChValidAiCnt=0;

    if(!(abOptChIsInitOver_g[iOptCh]))
    {
        //若通道未初始化
        return  FALSE;
    }
    ulLocAICnt=RD_AI_Cnt();
    ulOptAICnt=OPT_AI_Cnt(iOptCh,&ulFstTskMatchAiCnt, ulLocAICnt, &ulOptNewestValidAiCnt);

    iAICntDif=(int)(ulLocAICnt-ulOptAICnt);
    if(iAICntDif<0||iAICntDif>=2*uiAiPts_g)
    {
        //若当前长时间无接收数据更新
        return  FALSE;
    }
    /* 通信正常 */
    *pRtChValidAiCnt=ulOptAICnt;
    return  TRUE;
}

/* 设置OPT最新节拍 */
void OPT_SetCnt(void)
{
    /* 设置为提前1天的节拍 */
    aimodOpt_g[0].ulOptRefreshedCnt = RD_AI_Cnt()+103680000;
    aimodOpt_g[1].ulOptRefreshedCnt = RD_AI_Cnt()+103680000;
}

/* 设置OPT最新节拍 */
void OPT_DifTest(void)
{
    int64_t lLongUs;
    int32_t lUs;

    lLongUs = INT_MAX*(int64_t)OPT_TIME_BASE_FREQ/(int64_t)1000000+1000;
    lUs = OptGetUsIntvlByBaseDiff(lLongUs);
    printf("(1)lUs = %d\n", lUs);

    lLongUs = INT_MAX*(int64_t)OPT_TIME_BASE_FREQ/(int64_t)1000000-1000;
    lUs = OptGetUsIntvlByBaseDiff(lLongUs);
    printf("(2)lUs = %d\n", lUs);

    lLongUs = INT_MIN*(int64_t)OPT_TIME_BASE_FREQ/(int64_t)1000000+1000;
    lUs = OptGetUsIntvlByBaseDiff(lLongUs);
    printf("(3)lUs = %d\n", lUs);

    lLongUs = INT_MIN*(int64_t)OPT_TIME_BASE_FREQ/(int64_t)1000000-1000;
    lUs = OptGetUsIntvlByBaseDiff(lLongUs);
    printf("(4)lUs = %d\n", lUs);
}