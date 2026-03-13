/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*       OPT_SamSyn.c                                    1.0                      */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了光纵采样同步模块的代码文件                                     */
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

#include  "OPT_MmiInterface.h"
#include   "OPT_VtBox.h"
#include  "OPT_SamSyn.h"
#include  "realdata.h"
#include  "RE_PublicDataDef.h"
#include  "scc_hdlc_raw.h"
#include  "datetime.h"
#include  "math_compat.h"
#include   "OPT_SynAdapt.h"
#include  "RE_RelayEngine.h"
#include  "OPT_Com.h"
/***************************光纵机箱通信同步结构  ****************************/



BOOL   bOptHdlcClkIsMaster_g[2];        /*若设置HDLC时钟同步是主，则为真(当是直通时)，否则为假（当是接复接设备） 2009-4-14日 ZY */
OPT_TIME_BASE   aOptChInitTimeBase_g[2];  /* 光纵通道初始化TIMEBASE时间数组 */
OPT_CH_STS_REPORT  aOptChStsRpt_g[2];/*光纵通道状态报告数组，给MMI  */

OPT_CH_STS_DATA       OptChStsData_g[2];         /*光纵通道的状态数据  */
OPT_FR_STS            OptFrSts_g[2];             /*光纵帧接收状态  */
OPT_COM_SYN_DATA     aOptComSynData_g[2];      /*光纵通信接收同步数据 */
OPT_CH_RECV_DATA_VALID_STS   aOptChDataValidSts[2];     /*光纵通道接收数据有效状态  */
OPT_TIME_BASE  aulOptChLastRecvValidDataTime_g[2]= {{0,0},{0,0}};    /*光纵通道上次接收有效数据时间  2012-5-20日 张云 */
OPT_CH_LAST_COM_STABLE_SAM_SYN_INFO   aLastComStableSamSynInfo_g[2];   /*光纵通道最近的通信稳定且采样同步时的相关信息。2006-11-15  */
OPT_CH_LAST_COM_STABLE_INFO   aLastComStableInfo_g[2];   /*光纵通道最近的通信稳定时的相关信息。2009-2-9  ZY*/

OPT_SEND_COMMON_CMD   OptSendCommonCmd;             /*光纵发送时的公共数据  */
OPT_CH_SEND_CMD       OptChSendCmd[2];              /*光纵发送时的通道数据  */
OPT_CH_LST_RCV_INFO      OptChLstRcvInfo_g[2];         /*光纵通道最近接收的信息，供发送时访问  */

/* 获取采样通道延时(点数表示)
 * Para:
 *     NONE.
 * Return:
 *     点数.
 */
extern uint8_t adcGetDelayTime(void);

/*计算本帧的本次的即时采样同步误差
  参数：pComSynData，接收到的数据
        pChStsData，通道状态数据
        ucRcvSamCnt ,该帧接收时对应本机的采样计数 ，2007-11-28日 张云修改
        bPeerRcvIsStable, 对侧接收是否稳定.
  返回，成功与否    */
BOOL    OPT_CalcChInstSynDiffTime(OPT_COM_SYN_DATA  *pComSynData
                                  ,OPT_CH_STS_DATA  *pChStsData
                                  ,uint8_t   ucRcvSamCnt,
                                  BOOL bPeerRcvIsStable)
{
    /*减少变量数，进行优化，2006-5-27  */
    OPT_TIME_BASE  SrcSamCntBase;
    int   iComTime;
    int  iRtLeftAdjCnt;
    int  iCurSamPeriod = 0;/* ns 为单位*/
    int  iCurSamPeriod_us = 0;/* us 为单位*/
    uint8_t   ucCurSamCnt;
    static  int   iTemp;
    static  uint32_t  ulCurSamBaseL;
    uint8_t   ucLstRcvSynLocalCnt;
    int iAdjMode;

    OPT_GetSamTimeBase(pComSynData->ucLastRecvPeerRecvSamCnt
                       ,&SrcSamCntBase.ulTimeBaseH,&SrcSamCntBase.ulTimeBaseL);
    iComTime=OptGetUsIntvlByBase(&pComSynData->tLastRecvTime,&SrcSamCntBase);

    ucCurSamCnt=ucRcvSamCnt;/*2007-11-28日 张云修改  */
    OPT_GetSamTimeBase(ucCurSamCnt,&SrcSamCntBase.ulTimeBaseH,&SrcSamCntBase.ulTimeBaseL);
    ulCurSamBaseL=SrcSamCntBase.ulTimeBaseL;

    /*2013-5-28  ZY 恢复为原状，不能仅低32位比较 */
    if(OptBaseUnion(pComSynData->tLastRecvTime.ulTimeBaseH,pComSynData->tLastRecvTime.ulTimeBaseL)
            >=OptBaseUnion(SrcSamCntBase.ulTimeBaseH,SrcSamCntBase.ulTimeBaseL))
    {
        /*获得最接近的采样计数  */
        pComSynData->ucLastRecvCloseSamCnt=ucCurSamCnt;
    }
    else
    {

        pComSynData->ucLastRecvCloseSamCnt=SynSamAdjust(ucCurSamCnt, -1);

    }

    /* 延时判断及对侧接收是否稳定判断 */
    if (iComTime>100000
            || iComTime <= 0
            || iComTime <= (int)(pComSynData->uiLastRecvPeerRecvtoSendIntvl)
            || (!bPeerRcvIsStable))
    {
        /* 若算出的通信时间异常，包括: 总的通信时间>100毫秒，小于等于0, 或上边小于下边, 则抛弃，比如中断后重新通信 */
        pComSynData->uiLastRecvPeerRecvtoSendIntvl = DEFAULT_TIME_WHEN_INST_HALT;
        pComSynData->ucLastRecvPeerRecvSamCnt = SynSamAdjust(pComSynData->ucLastRecvCloseSamCnt, -pComSynData->ucSimTick);
        pComSynData->ucSimTick++;
        if (pComSynData->ucSimTick>MAX_SIM_TICK)
        {
            pComSynData->ucSimTick = MIN_SIM_TICK;
        }

        OPT_GetSamTimeBase(pComSynData->ucLastRecvPeerRecvSamCnt,
                           &SrcSamCntBase.ulTimeBaseH, &SrcSamCntBase.ulTimeBaseL);
        iComTime = OptGetUsIntvlByBase(&pComSynData->tLastRecvTime, &SrcSamCntBase);
    }

    pComSynData->ulLastChInstTime = (iComTime-pComSynData->uiLastRecvPeerRecvtoSendIntvl)/2;

    /*2006-6-9，修改了  */
    if(pComSynData->ulLastChInstTime>50000)
    {
        /*若算出的通信时间异常,大于50毫秒，则抛弃，比如中断后重新通信  2006-11-14日张云扩大，为了做误码实验 */

        logMsg("链路通信时间%d\n", pComSynData->ulLastChInstTime, 0, 0, 0, 0, 0);
        return  FALSE;
    }

    /* 计算与对侧采样点同步到本机的采样点 */

    OPT_GetSamTimeBase(pComSynData->ucLastRecvCloseSamCnt,&SrcSamCntBase.ulTimeBaseH,&SrcSamCntBase.ulTimeBaseL);
    pComSynData->iLastRecvtoCloseCntIntvl=OptGetUsIntvlByBase(&pComSynData->tLastRecvTime,&SrcSamCntBase);

    /* 调整方式 */
    iAdjMode = OPT_GetAdjSamMode(&iRtLeftAdjCnt);

    if(iAdjMode==OPT_SAM_SPEED)
    {
        iCurSamPeriod=iSpeedSamPeriod_g;/*单位为NS  */
        iCurSamPeriod_us = iSpeedSamPeriod_us_g;
    }
    else  if(iAdjMode==OPT_SAM_SLOW)
    {
        iCurSamPeriod=iSlowSamPeriod_g;
        iCurSamPeriod_us = iSlowSamPeriod_us_g;
    }
    else   if(iAdjMode==OPT_SAM_NORMAL)
    {

        iCurSamPeriod=iNormalSamPeriod_g;
        iCurSamPeriod_us = iNormalSamPeriod_us_g;
    }
    else
    {
        assert(FALSE);
    }
    iTemp=((((int)pComSynData->ulLastChInstTime)-pComSynData->iLastRecvtoCloseCntIntvl)*1000+(iCurSamPeriod>>1))/iCurSamPeriod;
    if(iTemp<0)
    {
        pComSynData->ucTemp=0;
    }
    else
    {
        pComSynData->ucTemp=iTemp;
    }

    /* 处理延迟点差 */
    pComSynData->ucLocalDelay = adcGetDelayTime();
    if (pComSynData->ucDelay>pComSynData->ucLocalDelay)
    {
        pComSynData->ucRltDif = -(pComSynData->ucDelay-pComSynData->ucLocalDelay);
        pComSynData->ucAllRltDif = pComSynData->ucTemp+pComSynData->ucDelay-pComSynData->ucLocalDelay;
    }
    else
    {
        pComSynData->ucRltDif = pComSynData->ucLocalDelay-pComSynData->ucDelay;
        pComSynData->ucAllRltDif = pComSynData->ucTemp-(pComSynData->ucLocalDelay-pComSynData->ucDelay);
    }

    ucLstRcvSynLocalCnt=pComSynData->ucLastRecvSynLocalSamCnt;
    pComSynData->ucLastRecvSynLocalSamCnt=SynSamAdjust(pComSynData->ucLastRecvCloseSamCnt, -(int)pComSynData->ucTemp);
    pComSynData->ucLastSynSamCntIntvl=SynSamAdjust
                                      (pComSynData->ucLastRecvSynLocalSamCnt, -(int)ucLstRcvSynLocalCnt);

    /*计算同步误差,本侧领先为正，需要慢采样，本侧落后，要快采样  */
    if(pComSynData->ucLastRecvCloseSamCnt>=pComSynData->ucLastRecvSynLocalSamCnt)
    {
        pComSynData->ulLastClosetoSynSamCntDiff=pComSynData->ucLastRecvCloseSamCnt-pComSynData->ucLastRecvSynLocalSamCnt;
    }
    else
    {
        pComSynData->ulLastClosetoSynSamCntDiff=((uint32_t)pComSynData->ucLastRecvCloseSamCnt)
                                                -((uint32_t)pComSynData->ucLastRecvSynLocalSamCnt)+RD_SAM_SYN_CLK;
    }

    /* 2006-6-9 */
    if(pComSynData->ulLastClosetoSynSamCntDiff>80)
    {
        /*若同步差过大，则出错   2006-11-14日张云扩大，为了做误码实验 */
        return  FALSE;
    }

    pComSynData->iLastSamSynDiffInstTime=pComSynData->iLastRecvtoCloseCntIntvl
                                         +pComSynData->ulLastClosetoSynSamCntDiff*iCurSamPeriod_us
                                         -pComSynData->ulLastChInstTime;

    /*判定TSSE与TS/2的差值，若差值太小，则需要防止TSSE计算振荡  */
    if(abs(abs(pComSynData->iLastSamSynDiffInstTime)-(iCurSamPeriod_us>>1))
            <OPT_INST_TSSE_TO_HALF_TS_DIFF_THRESH)
    {
        if(pComSynData->iLastSamSynDiffInstTime>0&&pChStsData->iSamSynDiffAvrgTime<0&&(ucRcvSamCnt!=pComSynData->ucLastRecvSynLocalSamCnt))
        {
            /* 2006-10-7日，防止出现当通信时间太短，导致同步到将来的采样点2007-11-28日 张云修改 */
            pComSynData->iLastSamSynDiffInstTime=-(iCurSamPeriod_us-pComSynData->iLastSamSynDiffInstTime);
            pComSynData->ucLastRecvSynLocalSamCnt=SynSamAdjust(pComSynData->ucLastRecvSynLocalSamCnt, 1);
            pComSynData->ucLastSynSamCntIntvl=SynSamAdjust(pComSynData->ucLastSynSamCntIntvl, 1);
            pComSynData->ulLastClosetoSynSamCntDiff=SynSamAdjust(pComSynData->ulLastClosetoSynSamCntDiff, -1);

        }
        else  if(pComSynData->iLastSamSynDiffInstTime<0&&pChStsData->iSamSynDiffAvrgTime>0)
        {
            pComSynData->iLastSamSynDiffInstTime=iCurSamPeriod_us+pComSynData->iLastSamSynDiffInstTime;
            pComSynData->ucLastRecvSynLocalSamCnt=SynSamAdjust(pComSynData->ucLastRecvSynLocalSamCnt, -1);
            pComSynData->ucLastSynSamCntIntvl=SynSamAdjust(pComSynData->ucLastSynSamCntIntvl, -1);
            pComSynData->ulLastClosetoSynSamCntDiff=SynSamAdjust(pComSynData->ulLastClosetoSynSamCntDiff, 1);

        }
    }
    return   TRUE;

}



/*更新通道帧状态缓冲数据
  参数：pComSynData，接收到的数据
        pFrSts,   帧状态
        iLostFrNum,丢失的帧数目
        iOptChNum,通道号，0为1号通道，1为2号通道
        bInstDataIsValid,瞬时数据有效与否标志
  返回      */
void    OPT_RefreshFrSts(OPT_COM_SYN_DATA  *pComSynData
                         ,OPT_CH_STS_DATA  *pChStsData
                         ,OPT_FR_STS* pFrSts
                         ,int  iLostFrNum
                         ,int  iOptChNum
                         ,BOOL   bInstDataIsValid)
{
    /*优化一下代码，，将正常情况提前，保证正常代码不被打断，2006-5-27日  */
    /*2006-6-9  */

    int  i;
    OPT_ONE_FR_STS  *pOneFrSts;


    aLostFrmStatInfo_g[iOptChNum].ulLastStatLostFrmNum=
        aLostFrmStatInfo_g[iOptChNum].ulLastStatLostFrmNum+iLostFrNum;/*2006-11-16日 进行了丢帧统计 张云  */
    aOptChStsRpt_g[iOptChNum].ulTotalFrLostNum=aOptChStsRpt_g[iOptChNum].ulTotalFrLostNum+iLostFrNum;/*2007-1-7日 张云进行了修改  */
    if((abs((int)pComSynData->ulLastChInstTime-(int)pChStsData->ulChAvrgTime)>=OPT_INST_DELAY_STABLE_FLUCT_THRESH))
    {
        /*2009-3-1日 张云进行了修改，若出现帧延迟,进行统计  */
        aOptChStsRpt_g[iOptChNum].ulTotalFrDelayNum++;
    }

    if(iLostFrNum==0)
    {
        pFrSts->uiCurFrPos=(++pFrSts->uiCurFrPos)&(OPT_FR_STS_BUF_NUM-1);
        pOneFrSts=pFrSts->FrStsBuf+pFrSts->uiCurFrPos;

        if((abs((int)pComSynData->ulLastChInstTime-(int)pChStsData->ulChAvrgTime)<OPT_INST_DELAY_STABLE_FLUCT_THRESH)
                &&(bInstDataIsValid))
        {
            /*2009-3-1日正常情况下，用瞬时值，代码命中率最高  */

            pOneFrSts->iFrLost=0;
            pOneFrSts->iFrDelay=0;
            pOneFrSts->ulCurChInstTime=pComSynData->ulLastChInstTime;
            pOneFrSts->iCurSamSynDiffInstTime=pComSynData->iLastSamSynDiffInstTime;
            pOneFrSts->ulCurChAvrgTime=pChStsData->ulChAvrgTime;
        }
        else  if((pComSynData->ulLastChInstTime>=(pChStsData->ulChAvrgTime+OPT_INST_DELAY_STABLE_ABNORMAL_THRESH))
                 ||(!bInstDataIsValid))
        {
            /*数据无效和延迟失真的帧,则处理为帧丢失，用平均值代替   */
            pOneFrSts->iFrLost=1;
            pOneFrSts->iFrDelay=0;
            pOneFrSts->ulCurChInstTime=pChStsData->ulChAvrgTime;/*用平均值代替  */
            pOneFrSts->iCurSamSynDiffInstTime=pChStsData->iSamSynDiffAvrgTime;
            pOneFrSts->ulCurChAvrgTime=pChStsData->ulChAvrgTime;
        }
        else
        {
            /*处理延迟的帧状态用瞬时值代替   */
            pOneFrSts->iFrLost=0;
            pOneFrSts->iFrDelay=1;
            pOneFrSts->ulCurChInstTime=pComSynData->ulLastChInstTime;
            pOneFrSts->iCurSamSynDiffInstTime=pComSynData->iLastSamSynDiffInstTime;
            pOneFrSts->ulCurChAvrgTime=pChStsData->ulChAvrgTime;
        }

    }/*if结束  */
    else
    {
        if(iLostFrNum>OPT_FR_STS_BUF_NUM)
        {
            iLostFrNum=OPT_FR_STS_BUF_NUM;
        }
        for(i=0; i<iLostFrNum; i++)
        {
            /*处理丢失的帧状态  */
            /*测试代码  */

            pFrSts->uiCurFrPos=(++pFrSts->uiCurFrPos)&(OPT_FR_STS_BUF_NUM-1);
            pOneFrSts=pFrSts->FrStsBuf+pFrSts->uiCurFrPos;
            pOneFrSts->iFrLost=1;

            pOneFrSts->iFrDelay=0;
            pOneFrSts->ulCurChInstTime=pChStsData->ulChAvrgTime;/*用平均值代替  */
            pOneFrSts->iCurSamSynDiffInstTime=pChStsData->iSamSynDiffAvrgTime;
            pOneFrSts->ulCurChAvrgTime=pChStsData->ulChAvrgTime;
        }

        pFrSts->uiCurFrPos=(++pFrSts->uiCurFrPos)&(OPT_FR_STS_BUF_NUM-1);
        pOneFrSts=pFrSts->FrStsBuf+pFrSts->uiCurFrPos;

        if((abs((int)pComSynData->ulLastChInstTime-(int)pChStsData->ulChAvrgTime)<OPT_INST_DELAY_STABLE_FLUCT_THRESH)
                &&(bInstDataIsValid))
        {
            /*2009-3-1日正常情况下，用瞬时值，代码命中率最高  */

            pOneFrSts->iFrLost=0;
            pOneFrSts->iFrDelay=0;
            pOneFrSts->ulCurChInstTime=pComSynData->ulLastChInstTime;
            pOneFrSts->iCurSamSynDiffInstTime=pComSynData->iLastSamSynDiffInstTime;
            pOneFrSts->ulCurChAvrgTime=pChStsData->ulChAvrgTime;
        }
        else  if((pComSynData->ulLastChInstTime>=(pChStsData->ulChAvrgTime+OPT_INST_DELAY_STABLE_ABNORMAL_THRESH))
                 ||(!bInstDataIsValid))
        {
            /*数据无效和延迟失真的帧,则处理为帧丢失，用平均值代替   */
            pOneFrSts->iFrLost=1;
            pOneFrSts->iFrDelay=0;
            pOneFrSts->ulCurChInstTime=pChStsData->ulChAvrgTime;/*用平均值代替  */
            pOneFrSts->iCurSamSynDiffInstTime=pChStsData->iSamSynDiffAvrgTime;
            pOneFrSts->ulCurChAvrgTime=pChStsData->ulChAvrgTime;
        }
        else
        {
            /*处理延迟的帧状态用瞬时值代替   */
            pOneFrSts->iFrLost=0;
            pOneFrSts->iFrDelay=1;
            pOneFrSts->ulCurChInstTime=pComSynData->ulLastChInstTime;
            pOneFrSts->iCurSamSynDiffInstTime=pComSynData->iLastSamSynDiffInstTime;
            pOneFrSts->ulCurChAvrgTime=pChStsData->ulChAvrgTime;
        }
    }/*else结束  */
}


/*更新通道状态数据
  参数：pComSynData，接收到的数据
        pChStsData,通道状态
        pChStsRpt,状态报告
        pFrSts,   帧接收状态
  返回：  是否需要写数据到缓冲，真，表示需要写缓冲，否则不需要写缓冲  */
BOOL   OPT_RefreshChSts(OPT_COM_SYN_DATA  *pComSynData
                        ,OPT_CH_STS_DATA  *pChStsData
                        ,OPT_CH_STS_REPORT  *pChStsRpt
                        ,OPT_FR_STS* pFrSts
                        ,int  iOptChNum)
{
    /* 将函数的变量减少，2006-5-27 */

    OPT_CH_RCV_ABNORMAL_STAT  RcvStat;

    static  BOOL   abDataIsPrepare_s[2]= {FALSE,FALSE};
    /*测试代码  */
    static   uint32_t  ulRefreshCnt11_s[2]= {0,0};

    ulRefreshCnt11_s[iOptChNum]++;

    if(!abDataIsPrepare_s[iOptChNum])
    {
        /*让FRSTS BUF中准备通道状态经验数据  */
        if(ulRefreshCnt11_s[iOptChNum]<(iSynAverageFrCnt_g<<1))
        {
            return  FALSE;
        }
        else
        {
            abDataIsPrepare_s[iOptChNum]=TRUE;
        }
    }

    /* 对帧缓冲中的数据进行汇总处理 */
    OPT_GatherFrSts(pComSynData,pChStsData,pChStsRpt
                    ,pFrSts,&RcvStat,iOptChNum);

    /*进行通道通信状态变迁图处理  */
    OPT_ComStsDeal(pComSynData,pChStsData,
                   pChStsRpt,&RcvStat,iOptChNum);

    /* 进行通道采样同步状态判定处理 */
    OPT_SamSynDeal(pComSynData,pChStsData,pChStsRpt,pFrSts,&RcvStat,iOptChNum);

    /* 进行采样同步调整处理 */
    return(OPT_SamSynAdjust(pComSynData,pChStsData,pChStsRpt,iOptChNum));


}


/*光纵帧数据汇总处理  2009-1-19日 ZY
  参数： pChStsData,    通道状态
         pChStsData,    通道状态
         pChStsRpt，    通道状态报告
         pFrSts,        帧接收状态
         pRtRcvStat,    供返回本帧接收异常统计
         iOptCh,通道号
 */
void   OPT_GatherFrSts(OPT_COM_SYN_DATA  *pComSynData
                       ,OPT_CH_STS_DATA  *pChStsData
                       ,OPT_CH_STS_REPORT  *pChStsRpt
                       ,OPT_FR_STS* pFrSts
                       ,OPT_CH_RCV_ABNORMAL_STAT  *pRtRcvStat
                       ,int  iOptCh)
{

    int  i;
    uint32_t  uiCurPos;
    uint32_t  uiPastAvrgPos;

    OPT_ONE_FR_STS   *pCurFrSts;
    OPT_ONE_FR_STS   *pPastFrSts;

    static   uint32_t  ulRefreshCnt12_s[2]= {0,0};

    ulRefreshCnt12_s[iOptCh]++;

    pRtRcvStat->bShortLostFrm=FALSE;
    pRtRcvStat->bLongLostFrm=FALSE;
    pRtRcvStat->bShortDelayFrm=FALSE;
    pRtRcvStat->bLongDelayFrm=FALSE;
    pRtRcvStat->bSendTimeChange=FALSE;
    pRtRcvStat->bInstLostFrm=FALSE;
    pRtRcvStat->bInstDelayFrm=FALSE;

    uiPastAvrgPos=(pFrSts->uiCurFrPos-(uint32_t)iSynAverageFrCnt_g)&(OPT_FR_STS_BUF_NUM-1);
    pPastFrSts=pFrSts->FrStsBuf+uiPastAvrgPos;

    if((ulRefreshCnt12_s[iOptCh]&0x3F)!=1)
    {
        /*递归式求和，降低对资源的要求，提高效率  */

        uiCurPos=pFrSts->uiCurFrPos;

        pCurFrSts=pFrSts->FrStsBuf+uiCurPos;

        pChStsData->iLostFrmPerWave=pChStsData->iLostFrmPerWave
                                    +pCurFrSts->iFrLost-pPastFrSts->iFrLost;
        pChStsData->iDelayFrmPerWave=pChStsData->iDelayFrmPerWave
                                     +pCurFrSts->iFrDelay-pPastFrSts->iFrDelay;
        pChStsData->ulChTotalTime=pChStsData->ulChTotalTime
                                  +pCurFrSts->ulCurChInstTime-pPastFrSts->ulCurChInstTime;
        pChStsData->iSamSynDiffTotalTime=pChStsData->iSamSynDiffTotalTime
                                         +pCurFrSts->iCurSamSynDiffInstTime-pPastFrSts->iCurSamSynDiffInstTime;
    }/*if结束  */
    else
    {
        /*每32次，重新初始化一次，消除浮点计算的误差，第1次一定要计算  */

        if((pFrSts->uiCurFrPos+1)>=iSynAverageFrCnt_g)
        {
            uiCurPos=(pFrSts->uiCurFrPos+1-(uint32_t)iSynAverageFrCnt_g)&(OPT_FR_STS_BUF_NUM-1);
        }
        else
        {
            uiCurPos=(pFrSts->uiCurFrPos+1+OPT_FR_STS_BUF_NUM-(uint32_t)iSynAverageFrCnt_g)&(OPT_FR_STS_BUF_NUM-1);
        }

        pChStsData->iLostFrmPerWave=0;
        pChStsData->iDelayFrmPerWave=0;
        pChStsData->ulChTotalTime=0;
        pChStsData->iSamSynDiffTotalTime=0;

        for(i=0; i<iSynAverageFrCnt_g; i++)
        {
            /*static   uint32_t   uiOneFrLostNum=0;*/

            pCurFrSts=pFrSts->FrStsBuf+uiCurPos;
            pChStsData->iLostFrmPerWave=pChStsData->iLostFrmPerWave
                                        +pCurFrSts->iFrLost;

            pChStsData->iDelayFrmPerWave=pChStsData->iDelayFrmPerWave
                                         +pCurFrSts->iFrDelay;
            pChStsData->ulChTotalTime=pChStsData->ulChTotalTime
                                      +pCurFrSts->ulCurChInstTime;
            pChStsData->iSamSynDiffTotalTime=pChStsData->iSamSynDiffTotalTime
                                             +pCurFrSts->iCurSamSynDiffInstTime;

            uiCurPos=(++uiCurPos)&(OPT_FR_STS_BUF_NUM-1);/*注意要先加，再与，否则错误  */
        }/*for循环结束  */

    }/*else结束  */

    /*保存帧缓冲当前和前推指针，给后面使用  */
    pRtRcvStat->pCurFrSts=pFrSts->FrStsBuf+pFrSts->uiCurFrPos;
    pRtRcvStat->pPastFrSts=pPastFrSts;

    /*处理帧丢失,判定求平均值期间有无帧丢失  */
    if(pChStsData->iLostFrmPerWave>0)
    {
        if(pChStsData->iLostFrmPerWave<OPT_INSTABLE_THRSH_FOR_FR_LOST)
        {
            /*若少数丢帧  */
            pRtRcvStat->bShortLostFrm=TRUE;
        }
        else
        {
            /*若连续丢帧  */
            pRtRcvStat->bLongLostFrm=TRUE;
        }
    }

    /*处理帧延迟情况 ,判定求平均值期间有无帧延迟 */
    if(pChStsData->iDelayFrmPerWave>0)
    {
        if(pChStsData->iDelayFrmPerWave<OPT_INSTABLE_THRSH_FOR_FR_DELAY)
        {
            /*若少数帧延迟  */
            pRtRcvStat->bShortDelayFrm=TRUE;
        }
        else
        {
            /*若连续帧长时间延迟 */
            pRtRcvStat->bLongDelayFrm=TRUE;
        }
    }

    /*求T的平均值  */
    pChStsData->ulChAvrgTime=pChStsData->ulChTotalTime/iSynAverageFrCnt_g;

    pChStsRpt->ulFrComTime=OPT_GetChRealComTime(iOptCh);/*2006-12-22日，要外部通道时间  */

    if(abs(pChStsData->ulChAvrgTime-pPastFrSts->ulCurChAvrgTime)
            >=OPT_DELAY_STABLE_FLUCT_THRESH)
    {
        /*若平均通信时间发生变化，超过门槛，  */

        pRtRcvStat->bSendTimeChange=TRUE;
    }
    /*将新的实际平均时间记录到缓冲  */
    pFrSts->FrStsBuf[pFrSts->uiCurFrPos].ulCurChAvrgTime=pChStsData->ulChAvrgTime;
    if(pFrSts->FrStsBuf[pFrSts->uiCurFrPos].iFrLost!=0)
    {
        pRtRcvStat->bInstLostFrm=TRUE;
    }
    if(pFrSts->FrStsBuf[pFrSts->uiCurFrPos].iFrDelay!=0)
    {
        pRtRcvStat->bInstDelayFrm=TRUE;
    }

    return;

}

/*光纵通信状态机处理  2006-11-15日
  参数： pChStsData,    通道状态
         pChStsData,    通道状态
         pChStsRpt，    通道状态报告
         pRcvStat,      供本帧接收异常统计
         iOptCh,通道号
 */
void   OPT_ComStsDeal(OPT_COM_SYN_DATA  *pComSynData
                      ,OPT_CH_STS_DATA  *pChStsData
                      ,OPT_CH_STS_REPORT  *pChStsRpt
                      ,OPT_CH_RCV_ABNORMAL_STAT  *pRcvStat
                      ,int  iOptCh)
{

    /*处理最经常的情形  */
    if((!pRcvStat->bShortLostFrm)
            &&(!pRcvStat->bLongLostFrm)
            &&(!pRcvStat->bShortDelayFrm)
            &&(!pRcvStat->bLongDelayFrm)
            &&(!pRcvStat->bSendTimeChange)
            &&(pChStsData->iComSts==OPT_CH_COM_STABLE))
    {
        /*若没有任何异常情况，且以前是稳定状态，则直接返回，这是最常见的情况  */
        return  ;
    }
    else
    {
        if(pRcvStat->bSendTimeChange)
        {
            if(aOptChDataValidSts[iOptCh].bFirstSamSynFlag_s)
            {
                /*2006-11-15日　张云  */
                pChStsRpt->ulTotalChComeTimeChangeNum++;
            }
        }
        /*进行通信状态变迁图的处理  这里必须进行优化*/
        switch(pChStsData->iComSts)
        {

            case   OPT_CH_COM_STABLE:/*若目前处于稳态  */
                if(pRcvStat->bInstLostFrm)
                {
                    pChStsData->TempInstableStartTime=pComSynData->tLastRecvTime;
                    pChStsData->iComSts=OPT_CH_TEMP_INSTABLE;
                }
                else  if(pRcvStat->bLongLostFrm)
                {
                    pChStsData->TempInstableStartTime=pComSynData->tLastRecvTime;

                    if(aOptChDataValidSts[iOptCh].bFirstSamSynFlag_s)
                    {
                        /*2006-11-15日　张云  */
                        pChStsRpt->ulTotalComInstableNum++;
                    }
                    pChStsData->iComSts=OPT_CH_COM_INSTABLE;
                }
                else   if(pRcvStat->bInstDelayFrm)
                {
                    pChStsData->TempInstableStartTime=pComSynData->tLastRecvTime;
                    pChStsData->iComSts=OPT_CH_TEMP_INSTABLE;
                }
                else   if(pRcvStat->bLongDelayFrm)
                {
                    pChStsData->TempInstableStartTime=pComSynData->tLastRecvTime;
                    if(aOptChDataValidSts[iOptCh].bFirstSamSynFlag_s)
                    {
                        /*2006-11-15日　张云  */
                        pChStsRpt->ulTotalComInstableNum++;
                    }
                    pChStsData->iComSts=OPT_CH_COM_INSTABLE;
                }
                else  if(pRcvStat->bSendTimeChange)
                {
                    pChStsData->TempInstableStartTime=pComSynData->tLastRecvTime;
                    pChStsData->iComSts=OPT_CH_TEMP_INSTABLE;
                }
                break;
            case   OPT_CH_TEMP_STABLE:/*若目前处于暂稳态  */

                if((!pRcvStat->bShortLostFrm)
                        &&(!pRcvStat->bLongLostFrm)
                        &&(!pRcvStat->bShortDelayFrm)
                        &&(!pRcvStat->bLongDelayFrm)
                        &&(!pRcvStat->bSendTimeChange))
                {
                    /*若此次稳定  */
                    if(OptGetUsIntvlByBase(&(pComSynData->tLastRecvTime),&(pChStsData->TempStableStartTime))
                            >OPT_DELAY_STABLE_PERSIST_THRESH)
                    {
                        /*若稳定持续足够时间长，则处于稳态  */
                        pChStsData->iComSts=OPT_CH_COM_STABLE;
                    }
                }
                else
                {
                    /*若此次不稳定，则又回到不稳态  */
                    pChStsData->iComSts=OPT_CH_COM_INSTABLE;

                    pChStsData->TempInstableStartTime=pComSynData->tLastRecvTime;
                }
                break;
            case   OPT_CH_TEMP_INSTABLE:
                /*若目前处于暂不稳态  */
                if((!pRcvStat->bInstLostFrm)
                        &&(!pRcvStat->bLongLostFrm)
                        &&(!pRcvStat->bInstDelayFrm)
                        &&(!pRcvStat->bLongDelayFrm)
                        &&(!pRcvStat->bSendTimeChange))
                {
                    /*若此次稳定,则又恢复到稳态  */
                    pChStsData->iComSts=OPT_CH_COM_STABLE;
                    pChStsData->TempStableStartTime=pComSynData->tLastRecvTime;
                }
                else
                {
                    /*若此次不稳定，  */
                    if(pRcvStat->bLongLostFrm)
                    {
                        pChStsData->iComSts=OPT_CH_COM_INSTABLE;
                        if(aOptChDataValidSts[iOptCh].bFirstSamSynFlag_s)
                        {
                            /*2006-11-15日　张云  */
                            pChStsRpt->ulTotalComInstableNum++;
                        }
                    }
                    else   if(pRcvStat->bLongDelayFrm)
                    {
                        pChStsData->iComSts=OPT_CH_COM_INSTABLE;

                        if(aOptChDataValidSts[iOptCh].bFirstSamSynFlag_s)
                        {
                            /*2006-11-15日　张云  */
                            pChStsRpt->ulTotalComInstableNum++;
                        }
                    }
                    else  if(OptGetUsIntvlByBase(&(pComSynData->tLastRecvTime),&(pChStsData->TempInstableStartTime))
                             >OPT_DELAY_INSTABLE_PERSIST_THRESH)
                    {

                        if(pRcvStat->bSendTimeChange)
                        {
                            /*若长时间通信延时变化，则认为进入不稳态  */
                            pChStsData->iComSts=OPT_CH_COM_INSTABLE;

                            if(aOptChDataValidSts[iOptCh].bFirstSamSynFlag_s)
                            {
                                /*2006-11-15日　张云  */
                                pChStsRpt->ulTotalComInstableNum++;
                            }
                        }
                    }
                }
                break;
            case   OPT_CH_COM_INSTABLE:/*若目前处于不稳态  */
                if((!pRcvStat->bShortLostFrm)
                        &&(!pRcvStat->bLongLostFrm)
                        &&(!pRcvStat->bShortDelayFrm)
                        &&(!pRcvStat->bLongDelayFrm)
                        &&(!pRcvStat->bSendTimeChange))
                {
                    /*若此次稳定  */
                    pChStsData->iComSts=OPT_CH_TEMP_STABLE;
                    pChStsData->TempStableStartTime=pComSynData->tLastRecvTime;
                }
                break;
            default:
                assert(FALSE);
                break;
        }/*switch结束  */
    }/*else结束  */

}




/*光纵采样同步状态判定  2009-1-19日  ZY
  参数： pComSynData，通道瞬时状态
         pChStsData,    通道状态
         pChStsRpt，   通道状态报告
         pFrSts,        帧接收状态
         pRcvStat,      供本帧接收异常统计
         iOptCh,通道号
 */
void   OPT_SamSynDeal(OPT_COM_SYN_DATA  *pComSynData
                      ,OPT_CH_STS_DATA  *pChStsData
                      ,OPT_CH_STS_REPORT  *pChStsRpt
                      ,OPT_FR_STS* pFrSts
                      ,OPT_CH_RCV_ABNORMAL_STAT  *pRcvStat
                      ,int  iOptCh)
{
    int  iABSSamSynDiffAvrgTime;

    /*求同步误差TSSE的平均误差  */
    pChStsData->iSamSynDiffAvrgTime=pChStsData->iSamSynDiffTotalTime/iSynAverageFrCnt_g;

    /*刷新通道同步状态 优化一下，2006-5-27*/
    iABSSamSynDiffAvrgTime=abs(pChStsData->iSamSynDiffAvrgTime);
    if(iABSSamSynDiffAvrgTime<OPT_SAM_RELAY_SYN_THRESH)/*张云改过，2009-3-8 */
    {
        /*  */
        pChStsData->iSamSynSts=OPT_CH_SAM_RELAY_SYN;
    }
    else
    {
        static  uint32_t   ulTestCnt101_s=0;
        char str[100]="";

        ulTestCnt101_s++;
        if((ulTestCnt101_s&0x1FFFFF)==1)
        {
            sprintf(str,"同步差过大%d!\n",iABSSamSynDiffAvrgTime);
            LOG_Write(LOG_KERNEL, str, NULL);
            /*LOG_Dbg_Msg("因TSSE同步差过大，导致的设置采样失步标志!\n",0,0,0,0,0,0);   */
        }

        pChStsData->iSamSynSts=OPT_CH_SAM_MISS_SYN;
    }

    /*添加了采样点同步模式的判定  2009-1-19  ZY  */
    if(pChStsData->iComSts!=OPT_CH_COM_STABLE)
    {
        /*若通信不稳定，则进行采样点同步的判定  */
        /*2013-5-20 ZY */
        uint64_t  ullBaseDiff;
        uint32_t  ulBaseTh;

        ullBaseDiff=OptGetBaseDiff(&(pComSynData->tLastRecvTime)
                                   ,&(aLastComStableSamSynInfo_g[iOptCh].LastRcvPeerBaseTime));
        ulBaseTh=CPU_POINT_SYN_ALLOW_TIME*(OPT_TIME_BASE_FREQ/1000000);
        if(ullBaseDiff<((uint64_t)ulBaseTh))
        {
            /*距离上次通信稳定且采样同步的时间<允许采样点同步的门槛，则认为采样点同步处理  */

            pChStsData->iSamSynSts=OPT_CH_SAM_POINT_SYN;
        }
    }
}



/*光纵采样同步调整  2009-1-19日  ZY
  参数： pComSynData，通道瞬时状态
         pChStsData,    通道状态
         pChStsRpt，   通道状态报告
         iOptChNum,通道号

  返回  是否需要写数据到缓冲，真，表示需要写缓冲，否则不需要写缓冲    */
BOOL   OPT_SamSynAdjust(OPT_COM_SYN_DATA  *pComSynData
                        ,OPT_CH_STS_DATA  *pChStsData
                        ,OPT_CH_STS_REPORT  *pChStsRpt
                        ,int  iOptChNum)
{

    int   iAdjCnt;
    static  int    aiLastAdjSetCnt[2]= {0,0};
    static  int    aiLastAdjLeftCnt[2]= {0,0};





    /*只有当上次的采样调整已经调整1/4后，才允许重新设置，否则设置太频繁 2007-11-28日 张云 */
    if(aiLastAdjLeftCnt[iOptChNum]>0)
    {
        aiLastAdjLeftCnt[iOptChNum]=aiLastAdjLeftCnt[iOptChNum]-iOptTxPts_g;
    }
    if(aiLastAdjLeftCnt[iOptChNum]<=((aiLastAdjSetCnt[iOptChNum]*3)>>2))
    {

        if((!(aNodeMasterInfo_g[0].bIsMaster))
                &&(aNodeMasterInfo_g[iOptChNum+1].bIsMaster)
                &&(pChStsData->iComSts==OPT_CH_COM_STABLE)
                &&(!(aChValidInfo_g[iOptChNum].bChIsSelfCircle)))
        {
            /*若本侧是从,对侧是主，且处于通信稳态，且不是自环则允许调整采样 2011-11-25 ZY */

            if((iOptChNum==1)
                    &&(!(aNodeMasterInfo_g[0].bIsMaster))
                    &&(aNodeMasterInfo_g[0+1].bIsMaster)
                    &&(OptChStsData_g[0].iComSts==OPT_CH_COM_STABLE)
                    &&(!(aChValidInfo_g[0].bChIsSelfCircle)))
            {
                /*且当前是通道2， 若通道1已经调整采样，则通道2不调整采样， 2011-11-25 ZY */

            }
            else
            {
                if(pChStsData->iSamSynDiffAvrgTime>OPT_SAM_EXACT_SYN_THRESH
                        &&(pChStsData->iSamSynDiffAvrgTime<(((iOptTxPts_g*iNormalSamPeriod_us_g)<<1))))
                {
                    /*若同步差>正门槛，则需要慢采样 ，2006-12-12日 张云修改一个BUG */

                    iAdjCnt=pChStsData->iSamSynDiffAvrgTime*iNormalSamCntPerWave_g/OPT_SAM_ADJ_TIME_PER_PERIOD;

                    OPT_AdjustSamMode(OPT_SAM_SLOW,iSlowSamPeriod_g,iAdjCnt);
                    aiLastAdjSetCnt[iOptChNum]=iAdjCnt;
                    aiLastAdjLeftCnt[iOptChNum]=iAdjCnt;
                }
                else  if(pChStsData->iSamSynDiffAvrgTime<-(OPT_SAM_EXACT_SYN_THRESH)
                         &&(pChStsData->iSamSynDiffAvrgTime>(-((iOptTxPts_g*iNormalSamPeriod_us_g)<<1))))
                {
                    /*若同步差<负门槛，则需要快采样 2006-12-12日 张云修改一个BUG */

                    iAdjCnt=(-pChStsData->iSamSynDiffAvrgTime)*iNormalSamCntPerWave_g/OPT_SAM_ADJ_TIME_PER_PERIOD;

                    OPT_AdjustSamMode(OPT_SAM_SPEED,iSpeedSamPeriod_g,iAdjCnt);
                    aiLastAdjSetCnt[iOptChNum]=iAdjCnt;
                    aiLastAdjLeftCnt[iOptChNum]=iAdjCnt;

                }/* else  if结束 */
            }/* else结束 */
        }/*if结束  */
    }/*if结束 */

    if(pChStsData->iComSts==OPT_CH_COM_STABLE)
    {
        /*当处于稳态 */

        pChStsRpt->bChComStableFlag=TRUE;

        if(pChStsData->iSamSynSts==OPT_CH_SAM_RELAY_SYN)
        {
            /*且数据同步上，*/

            pChStsRpt->bChSamSynFlag=TRUE;

        }
        else
        {
            /*数据未同步上*/

            static  uint32_t   ulSamSynCnt_s[2]= {0,0};
            static uint8_t aucLogInfo[256];

            ulSamSynCnt_s[iOptChNum]++;
            if((ulSamSynCnt_s[iOptChNum]&0x1FFFFFFF)==0x3ff0)
            {
                sprintf(aucLogInfo, "光纵通道%d,通信稳定，采样失步!\n",iOptChNum+1);
                LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
            }
            pChStsRpt->bChSamSynFlag=FALSE;
        }
    }
    else
    {
        /* 若通信不稳定，张云*/


        static  uint32_t   ulComErrCnt_s[2]= {0,0};
        static uint8_t aucLogInfo[256];

        pChStsRpt->bChComStableFlag=FALSE;
        ulComErrCnt_s[iOptChNum]++;
        if((ulComErrCnt_s[iOptChNum]&0x1FFFFFFF)==0x3ff0)
        {
            sprintf(aucLogInfo, "光纵通道%d通信不稳定!\n",iOptChNum+1);

            LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
        }

        if(pChStsData->iSamSynSts==OPT_CH_SAM_POINT_SYN)
        {
            /*且数据同步上，*/
            pChStsRpt->bChSamSynFlag=TRUE;

        }
        else
        {
            /*数据未同步上*/

            static  uint32_t   ulSamSynCnt_s[2]= {0,0};
            static uint8_t aucLogInfo[256];

            ulSamSynCnt_s[iOptChNum]++;
            if((ulSamSynCnt_s[iOptChNum]&0x1FFFFFFF)==0x3ff0)
            {
                sprintf(aucLogInfo, "光纵通道%d,通信不稳定，采样失步!\n",iOptChNum+1);
                LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
            }
            pChStsRpt->bChSamSynFlag=FALSE;
        }

    }
    return  TRUE;/* 只要有数据接收，就需要写到缓冲中，满足距离保护的要求  张云*/
}



/*光纵收发路由差的变化判定  2009-2-9日  ZY
  参数： pComSynData，通道瞬时状态
         pChStsData,    通道状态
         iOptCh,通道号
   注意：只有在通信稳定，才进行判定，才被调用
 */
void   OPT_RouteDifChgDeal(OPT_COM_SYN_DATA  *pComSynData
                           ,OPT_CH_STS_DATA  *pChStsData
                           ,int  iOptCh)
{
    uint8_t  ucNewSamCnt;
    int  iRcvSndDiffChgTime;

    /* 先清零 */
    iRcvSndDiffChgTime=0;

    ucNewSamCnt=SynSamAdjust(pComSynData->ucLastRecvPeerSamCnt,
                             -(int)aLastComStableInfo_g[iOptCh].ucRecvPeerSamCnt);
    if(ucNewSamCnt!=iOptTxPts_g)
    {
        /* 若是两次通信稳定中间，发生过通信不稳定变化，才进行判定 */

        /*2013-5-20 ZY */
        uint64_t  ullBaseDiff;
        uint32_t  ulBaseTh;

        ullBaseDiff=OptGetBaseDiff(&(pComSynData->tLastRecvTime)
                                   ,&(aLastComStableInfo_g[iOptCh].LastRcvPeerBaseTime));
        ulBaseTh=OPT_RCV_SND_DIF_COM_STABLE_INTVL_THRESH*(OPT_TIME_BASE_FREQ/1000000);
        if(ullBaseDiff<((uint64_t)ulBaseTh))
        {
            /*此次通信稳定距离上次通信稳定的时间间隔<相应门槛，才进行路由差变化的判定  */
            OPT_TIME_BASE  LstStablePeerSndBaseTime;
            OPT_TIME_BASE  CurStablePeerSndBaseTime;
            OPT_TIME_BASE  CurStableCalcPeerSndBaseTime;
            OPT_CALC_SAM_INTVL_INFO   CalcSamIntvlInfo;

            OptTimebaseAddUsTime(
                &(aLastComStableInfo_g[iOptCh].LastRcvPeerBaseTime),
                &(LstStablePeerSndBaseTime),
                -(int)(aLastComStableInfo_g[iOptCh].ulLastComTime));

            OptTimebaseAddUsTime(
                &(pComSynData->tLastRecvTime),
                &(CurStablePeerSndBaseTime),
                -(int)(pChStsData->ulChAvrgTime));

            CalcSamIntvlInfo.OldPointBaseTime=
                LstStablePeerSndBaseTime;
            CalcSamIntvlInfo.ucOldPointCnt=
                aLastComStableInfo_g[iOptCh].ucRecvPeerSamCnt;
            CalcSamIntvlInfo.NewPointBaseTime=
                CurStablePeerSndBaseTime;
            CalcSamIntvlInfo.ucNewPointCnt=
                pComSynData->ucLastRecvPeerSamCnt;
            OptCalcSamIntvl(&(CalcSamIntvlInfo));

            if(CalcSamIntvlInfo.iCalcSamPointIntvl>=0)
            {
                /*用对侧发送时刻来计算，可以去掉计算同步导致的不准确导致的误差  */
                int32_t    iCalcTimeDiffUs;
                int32_t    iIntvlUs;
                int32_t    iRcvSndDifChgUs;

                /* 不考虑溢出情况 */
                iCalcTimeDiffUs = CalcSamIntvlInfo.iCalcSamPointIntvl*iNormalSamPeriod_us_g;

                OptTimebaseAddUsTime(&LstStablePeerSndBaseTime,
                                     &CurStableCalcPeerSndBaseTime,
                                     iCalcTimeDiffUs);
                iIntvlUs=OptGetUsIntvlByBase(&CurStablePeerSndBaseTime,
                                             &CurStableCalcPeerSndBaseTime);
                iRcvSndDifChgUs=iIntvlUs*2;
                if(abs(iRcvSndDifChgUs)>=OPT_RCV_SND_DIF_CHG_THRESH)
                {
                    /*若计算出来的收发时间差变化大于门槛，则认为收发时间差发生变化，这里的变化值是指：
                       	      本次稳定时真实收发时间差-上次稳定时真实收发时间差  */
                    iRcvSndDiffChgTime=iRcvSndDifChgUs;
                }

            }/*if(OptChLstRcvInfo_g[iOptCh].iCalcSamPointIntvl>=0)结束  */
        }/*if(ulIntvlSec<...结束　　*/
    }/* if(ucNewSamCn!=iOptTxPts_g)结束 */

    /*保存本次收发时间差变化  */
    pChStsData->iRcvSndDiffChgTime=iRcvSndDiffChgTime;

    /*保存此次通信稳定时的信息  */
    aLastComStableInfo_g[iOptCh].LastRcvPeerBaseTime=
        pComSynData->tLastRecvTime;
    aLastComStableInfo_g[iOptCh].ulLastComTime=
        pChStsData->ulChAvrgTime;
    aLastComStableInfo_g[iOptCh].ucRecvPeerSamCnt=
        pComSynData->ucLastRecvPeerSamCnt;

    return;

}





/* 功能：根据pCalcInfo相关信息，计算采样节拍差
     返回信息在pCalcInfo->iCalcSamPointIntvl
   参数：pCalcInfo
   返回：无*/
void    OptCalcSamIntvl(OPT_CALC_SAM_INTVL_INFO  *pCalcInfo)
{
    uint64_t   ullCalcDiff;
    uint8_t    ucDiff;
    uint32_t   ulTurnCnt;


    /* 改为仅用低32位计算,2013-5-20  ZY */
    ullCalcDiff = ((pCalcInfo->NewPointBaseTime.ulTimeBaseL-pCalcInfo->OldPointBaseTime.ulTimeBaseL)*OPT_TIME_BASE_PERIOD)/iNormalSamPeriod_g;

    if((int)pCalcInfo->ucNewPointCnt>=(int)pCalcInfo->ucOldPointCnt)
    {
        ucDiff=pCalcInfo->ucNewPointCnt-pCalcInfo->ucOldPointCnt;
    }
    else
    {
        ucDiff=RD_SAM_SYN_CLK-pCalcInfo->ucOldPointCnt+pCalcInfo->ucNewPointCnt;

    }
    ulTurnCnt=((ullCalcDiff+RD_SAM_SYN_CLK/2)-ucDiff)/RD_SAM_SYN_CLK;
    pCalcInfo->iCalcSamPointIntvl=ulTurnCnt*RD_SAM_SYN_CLK+ucDiff;
    return;
}


/* 某光差通道当前是否和对侧采样同步 2009-3-5日  ZY
 * 参数：   iOptCh ,光纵通道号，0为光纵通道1，1为光纵通道2，其他无效
 *
 * 返回值：TRUE，当前该通道和对侧采样同步
           FALSE，当前该通道和对侧采样失步
   注意：  供RD_AI_Dat_P函数中调用 */
BOOL  OPT_ChCurIsSamSyn(int  iOptCh)
{
    if(!(abOptChIsInitOver_g[iOptCh]))
    {
        /* 若未初始化 */
        return  FALSE;
    }
    else
    {
        uint64_t  ullBaseDiff;
        uint32_t  ulBaseTh;
        OPT_TIME_BASE  CurTimeBase;

        CurTimeBase=OptGetBaseTimerCnt();
        ullBaseDiff=OptGetBaseDiff(&CurTimeBase
                                   ,&(aLastComStableSamSynInfo_g[iOptCh].LastRcvPeerBaseTime));
        ulBaseTh=CPU_POINT_SYN_ALLOW_TIME*(OPT_TIME_BASE_FREQ/1000000);
        if(ullBaseDiff<((uint64_t)ulBaseTh))
        {
            /*距离上次通信稳定且采样同步的时间<允许采样点同步的门槛，则认为处于采样同步 */

            return  TRUE;
        }
        else
        {
            return  FALSE;
        }
    }
}


/* 初始化光纵通道通信同步数据
 * 参数：   iOptChNum ,光纵通道号，0为光纵通道1，1为光纵通道2，其他无效
 *          pvAiMod，该模块（光纵虚拟机箱负责的所有AI采集/计算通道）的句柄
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS   OPT_InitChComSynData(int  iOptChNum,
                                 void *pvAiMod)
{

    assert(iOptChNum==0||iOptChNum==1);
    assert(pvAiMod);

    if(iOptChNum==0)
    {
        OptSendCommonCmd.ucLocalSamCnt=0;
    }

    bOptHdlcClkIsMaster_g[iOptChNum]=TRUE;/*2009-4-14日 张云  */
    OptFrSts_g[iOptChNum].uiCurFrPos=OPT_FR_STS_BUF_NUM-1;
    OptChSendCmd[iOptChNum].ucSendControl=0;

    aOptComSynData_g[iOptChNum].ucSimTick = MIN_SIM_TICK; /* 模拟节拍赋值 */

    aOptComSynData_g[iOptChNum].tLastRecvTime.ulTimeBaseH=0;
    aOptComSynData_g[iOptChNum].tLastRecvTime.ulTimeBaseL=0;

    OptChStsData_g[iOptChNum].iComSts=OPT_CH_COM_INSTABLE;
    OptChStsData_g[iOptChNum].iSamSynSts=OPT_CH_SAM_MISS_SYN;
    /*2009-2-13日 ZY  */
    OptChStsData_g[iOptChNum].iRcvSndDiffChgTime=0;

    OptChStsData_g[iOptChNum].iLostFrmPerWave=0;
    OptChStsData_g[iOptChNum].iDelayFrmPerWave=0;
    OptChStsData_g[iOptChNum].ulChAvrgTime=4000;
    OptChStsData_g[iOptChNum].iSamSynDiffAvrgTime=400;
    OptChStsData_g[iOptChNum].TempInstableStartTime.ulTimeBaseH=0;
    OptChStsData_g[iOptChNum].TempInstableStartTime.ulTimeBaseL=0;
    OptChStsData_g[iOptChNum].ulChTotalTime=0;
    OptChStsData_g[iOptChNum].iSamSynDiffTotalTime=0;

    /*设置光纵通道 数据 有效状态初始值   */
    aOptChDataValidSts[iOptChNum].iInValidRecvCnt_s=0;
    aOptChDataValidSts[iOptChNum].bDataIsValid_s=FALSE;
    aOptChDataValidSts[iOptChNum].bLastSynFlag_s=FALSE;
    aOptChDataValidSts[iOptChNum].bFirstRecvFlag_s=TRUE;
    aOptChDataValidSts[iOptChNum].bFirstSamSynFlag_s=FALSE;/*2006-11-15日　张云  */

    /*设置上送给MMI的光纵通道报告的部分初始状态  */

    aOptChStsRpt_g[iOptChNum].ucOptChNum=iOptChNum+1;
    aOptChStsRpt_g[iOptChNum].bChInitFlag=FALSE;
    aOptChStsRpt_g[iOptChNum].bChComStableFlag=FALSE;
    aOptChStsRpt_g[iOptChNum].bChSamSynFlag=FALSE;
    aOptChStsRpt_g[iOptChNum].ulFrComTime=0;
    aOptChStsRpt_g[iOptChNum].ulTotalComInstableNum=0;
    aOptChStsRpt_g[iOptChNum].ulTotalFrLostNum=0;
    aOptChStsRpt_g[iOptChNum].ulTotalFrDelayNum=0;
    aOptChStsRpt_g[iOptChNum].ulTotalFrErrNum=0;
    aOptChStsRpt_g[iOptChNum].ulTotalChComeTimeChangeNum=0;
    aOptChStsRpt_g[iOptChNum].ulTotalSamMissSynNum=0;
    aOptChStsRpt_g[iOptChNum].ulHdlcCrcErr=0;
    aOptChStsRpt_g[iOptChNum].ulHdlcDpllErr=0;
    aOptChStsRpt_g[iOptChNum].ulHdlcAddrErr=0;
    aOptChStsRpt_g[iOptChNum].ulHdlcCpmBusy=0;
    aOptChStsRpt_g[iOptChNum].ulHdlcRcvBusy=0;
    aOptChStsRpt_g[iOptChNum].ulFrLostNumPerSec=0;
    aOptChStsRpt_g[iOptChNum].ulFrDelayNumPerSec=0;
    aOptChStsRpt_g[iOptChNum].ulFrCRCErrNumPerSec=0;
    aOptChStsRpt_g[iOptChNum].bLocalIsMaster=FALSE;
    aOptChStsRpt_g[iOptChNum].bPeerIsMaster=FALSE;
    aOptChStsRpt_g[iOptChNum].uiLocalRandCode=0;
    aOptChStsRpt_g[iOptChNum].uiPeerRandCode=0;

    aOptChInitTimeBase_g[iOptChNum]=OptGetBaseTimerCnt();

    /*2013-5-20日　张云添加  */
    aulOptChLastRecvValidDataTime_g[iOptChNum].ulTimeBaseH=0;
    aulOptChLastRecvValidDataTime_g[iOptChNum].ulTimeBaseL=0;

    aLastComStableSamSynInfo_g[iOptChNum].LastRcvPeerBaseTime.ulTimeBaseH=0;/*2009-1-19 ZY  */
    aLastComStableSamSynInfo_g[iOptChNum].LastRcvPeerBaseTime.ulTimeBaseL=0;
    aLastComStableSamSynInfo_g[iOptChNum].ulLastComTime=0;
    aLastComStableSamSynInfo_g[iOptChNum].ucRecvPeerSamCnt=0;
    aLastComStableSamSynInfo_g[iOptChNum].ucSamSynLocalSamCnt=0;
    aLastComStableSamSynInfo_g[iOptChNum].ulTsse=0;

    /*2006-11-16日 张云添加  */
    aLostFrmStatInfo_g[iOptChNum].ulLastStatLostFrmUsCnt=OptGetBaseTimerCnt();//ZY,2013-5-20
    aLostFrmStatInfo_g[iOptChNum].ulLastStatLostFrmNum=0;
    aLostFrmStatInfo_g[iOptChNum].bNewComIsLongHaltInStat=FALSE;

    /*2006-11-27日 张云修改  */
    aOptChNeedDealFrmInfo[iOptChNum].ulNeedDealCnt=0;
    aOptChNeedDealFrmInfo[iOptChNum].ulCurDealPos=OPT_DEAL_FR_BUF_NUM-1;
    aOptChNeedDealFrmInfo[iOptChNum].ulNewRecvPos=OPT_DEAL_FR_BUF_NUM-1;

    /*2009-2-9日 张云修改  */
    aLastComStableInfo_g[iOptChNum].LastRcvPeerBaseTime.ulTimeBaseH=0;
    aLastComStableInfo_g[iOptChNum].LastRcvPeerBaseTime.ulTimeBaseL=0;
    aLastComStableInfo_g[iOptChNum].ulLastComTime=0;
    aLastComStableInfo_g[iOptChNum].ucRecvPeerSamCnt=0;

    return   EP_SUCCESS;
}



/*在TIMEBASE上加减DELTA的US时间，然后得到新的BASE时间  */
void OptTimebaseAddUsTime(OPT_TIME_BASE  *pBaseSrc,OPT_TIME_BASE  *pBaseDst,int32_t   iUsDelta)
{
    /*以前实现
         int64_t   llBaseDelta;
         uint64_t   ullA;
         llBaseDelta=(int64_t)iUsDelta*(int64_t)1000/(int64_t)(OPT_TIME_BASE_PERIOD);
         ullA=(((uint64_t)(pBaseSrc->ulTimeBaseH))<<32)+(uint64_t)(pBaseSrc->ulTimeBaseL);
         if(llBaseDelta<0)
         {
             ullA=ullA-(uint64_t)(-llBaseDelta);
         }
         else
         {
             ullA=ullA+(uint64_t)(llBaseDelta);
         }
         pBaseDst->ulTimeBaseH=(uint32_t)((ullA>>32)&((uint64_t)(0xFFFFFFFF)));
         pBaseDst->ulTimeBaseL=(uint32_t)(ullA&((uint64_t)(0xFFFFFFFF)));
    */
    /*2013-5-21日  ZY优化 */
    if(iUsDelta>=0)
    {
        uint64_t  ullLowSum;
        uint32_t  ulBaseDelta;
        ulBaseDelta=((uint32_t)iUsDelta)*1000/(OPT_TIME_BASE_PERIOD);/*不考虑溢出 */
        ullLowSum=((uint64_t)pBaseSrc->ulTimeBaseL)+((uint64_t)ulBaseDelta);
        if(ullLowSum>(0xFFFFFFFF))
        {
            /*要进位*/
            pBaseDst->ulTimeBaseH=pBaseSrc->ulTimeBaseH;
            pBaseDst->ulTimeBaseH++;
            pBaseDst->ulTimeBaseL=(uint32_t)(ullLowSum-(0x100000000));
        }
        else
        {
            pBaseDst->ulTimeBaseH=pBaseSrc->ulTimeBaseH;
            pBaseDst->ulTimeBaseL=(uint32_t)ullLowSum;
        }

    }
    else
    {
        uint32_t   ulusDelta;
        uint32_t  ulBaseDelta;
        ulusDelta=(uint32_t)(0-iUsDelta);
        ulBaseDelta=(ulusDelta)*1000/(OPT_TIME_BASE_PERIOD);/*不考虑溢出 */
        if(pBaseSrc->ulTimeBaseL<ulBaseDelta)
        {
            /*要借位*/
            pBaseDst->ulTimeBaseH=pBaseSrc->ulTimeBaseH;
            pBaseDst->ulTimeBaseH--;
            pBaseDst->ulTimeBaseL=pBaseSrc->ulTimeBaseL-ulBaseDelta;
        }
        else
        {
            pBaseDst->ulTimeBaseH=pBaseSrc->ulTimeBaseH;
            pBaseDst->ulTimeBaseL=pBaseSrc->ulTimeBaseL-ulBaseDelta;
        }
    }
}
/*TIMEBASE的相减,获得秒时间差  ,只能是正*/

uint32_t    OptGetSecIntvlByBase(OPT_TIME_BASE  *pBaseA,OPT_TIME_BASE  *pBaseB)
{
    uint64_t   ullA;
    uint64_t   ullB;
    uint64_t   ullDiff;
    uint64_t   ullSecDiff;
    uint32_t   ulSecDiff;

    ullA=(((uint64_t)(pBaseA->ulTimeBaseH))<<32)+(uint64_t)(pBaseA->ulTimeBaseL);
    ullB=(((uint64_t)(pBaseB->ulTimeBaseH))<<32)+(uint64_t)(pBaseB->ulTimeBaseL);
    ullDiff=ullA-ullB;
    ullSecDiff=(ullDiff*(uint64_t)OPT_TIME_BASE_PERIOD)/((uint64_t)1000000000);
    ulSecDiff=(uint32_t)(ullSecDiff&0xFFFFFFFF);
    return   ulSecDiff;
}

/* 设置通道非选配(不影响界面显示通道数目).
 * Para:
 *     iOptChNum, 通道号, 取值为0或1.
 * Return:
 *     TRUE or FALSE.
 */
BOOL OPT_SetChNotUsed(int iOptChNum)
{
    if ((iOptChNum == 0) || (iOptChNum == 1))
    {
        OPT_DisableRecv(iOptChNum);
        abOptChIsInitOver_g[iOptChNum] = FALSE;
        OPT_ClrRpt(iOptChNum);

        return TRUE;
    }

    return FALSE;
}

/* 设置通道未配置,可影响界面显示通道数目,保测一体使用.
 * Para:
 *     iOptChNum, 通道号, 取值为0或1.
 * Return:
 *     TRUE or FALSE.
 */
BOOL OPT_SetChInitFlagFlase(int iOptChNum)
{
    if((iOptChNum == 0) || (iOptChNum == 1))
    {
        aOptChStsRpt_g[iOptChNum].bChInitFlag = FALSE;
    }

    return FALSE;
}

/* 设置通道选配(用于控制界面显示).
 * Para:
 *     iOptChNum, 通道号, 取值为0或1.
 * Return:
 *     TRUE or FALSE.
 */
BOOL OPT_SetChUsed(int iOptChNum)
{
    if ((iOptChNum == 0) || (iOptChNum == 1))
    {
        if (iOptChNum == 1)
        {
            InitOptBoxChn2();
        }
        OPT_ClrRpt(iOptChNum);

        return TRUE;
    }

    return FALSE;
}

/* 清除报告信息.
 * Para:
 *     iOptChNum, 通道号, 取值为0或1.
 * Return:
 *     TRUE or FALSE.
 */
BOOL OPT_ClrRpt(int32_t iOptChNum)
{
    OPT_CH_STS_REPORT *pChStsRpt = NULL;

    pChStsRpt = aOptChStsRpt_g+iOptChNum;
    pChStsRpt->bChComStableFlag = FALSE;
    pChStsRpt->bChSamSynFlag = FALSE;
    pChStsRpt->ulFrComTime = 0;
    pChStsRpt->ulTotalComInstableNum = 0;
    pChStsRpt->ulTotalFrLostNum = 0;
    pChStsRpt->ulTotalFrDelayNum = 0;
    pChStsRpt->ulTotalFrErrNum = 0;
    pChStsRpt->ulTotalChComeTimeChangeNum = 0;
    pChStsRpt->ulTotalSamMissSynNum = 0;
    pChStsRpt->ulHdlcCrcErr = 0;
    pChStsRpt->ulHdlcDpllErr = 0;
    pChStsRpt->ulHdlcAddrErr = 0;
    pChStsRpt->ulHdlcCpmBusy = 0;
    pChStsRpt->ulHdlcRcvBusy = 0;
    pChStsRpt->ulFrLostNumPerSec = 0;
    pChStsRpt->ulFrDelayNumPerSec = 0;
    pChStsRpt->ulFrCRCErrNumPerSec = 0;
    pChStsRpt->bLocalIsMaster = FALSE;
    pChStsRpt->bPeerIsMaster = FALSE;
    pChStsRpt->uiLocalRandCode = 0;
    pChStsRpt->uiPeerRandCode = 0;

    return TRUE;
}
