/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*       OPT_COM.c                                    1.0                      */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了光纵通信收发模块的代码文件                                     */
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
/*         张云       2006.12.3                创建文件1.0版本                 */
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
#include  "intLib.h"
#include   "OPT_Data.h"
#include  "RE_RelayEngineEntry.h"
#include  "smvcfg.h"
/***************************光纵机箱通信结构  ****************************/

OPT_CH_LOST_FRM_STAT_INFO   aLostFrmStatInfo_g[2]; /*光纵通道丢帧统计数据信息  */
OPT_CH_NEED_DEAL_FRM_INFO   aOptChNeedDealFrmInfo[2];/*通道需要处理的帧信息  2006-11-27日 张云  */

uint8_t  aucSndDataBuf_g[2][MAX_OPT_SND_BUF_LEN];    /*光纵发送通道数据缓冲  */
uint32_t   aulLastQueryTotalFrLostNum_s[2]= {0,0};
uint32_t   aulLastQueryTotalFrDelayNum_s[2]= {0,0};
uint32_t   aulLastQueryHdlcCrcErr_s[2]= {0,0};


uint32_t   aulLastClearHdlcCrcErr_s[2]= {0,0}; /*保存上次清零时的HDLC异常信息，因为HDLC异常，没有清零功能  */
uint32_t   aulLastClearHdlcDpllErr_s[2]= {0,0};
uint32_t   aulLastClearHdlcAddrErr_s[2]= {0,0};
uint32_t   aulLastClearHdlcCpmBusy_s[2]= {0,0};
uint32_t   aulLastClearHdlcRcvBusy_s[2]= {0,0};
uint8_t *pucFrNewAISrAI_g[2];  /* 接收数据帧 */

/*测试代码  */
/*uint8_t    aucRecvBuf_g[1000000];*/
extern   uint32_t   hdlcRecvNum;
extern   uint32_t   hdlcSendNum;
extern   void Set_Hdlc_Out_Bit(unsigned char settingBit);/* 刷新HDLC的设置，由BSP提供，参数为0，表示照以前的值再写一遍。
                                                              防止小附板出错，需周期性调用 2007-8-16  张云*/

/* 获取采样通道延时(点数表示)
 * Para:
 *     NONE.
 * Return:
 *     点数.
 */
extern uint8_t adcGetDelayTime(void);

uint8_t   *apucSaveRecvDataPt_g[2]; /*2009-1-21日 张云  临时保存的本次接收到的通道数据指针  */

/* 发送光纵数据,供采样模块调用
 * 参数      ucSamCnt， 本次采样计数
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS   OPT_Send( uint8_t      ucSamCnt)
{
    OptSendCommonCmd.ucLocalSamCnt=ucSamCnt;
    /* 直接发送数据  2006-7-29 */
    OptSendCmd();

    return  EP_SUCCESS;
}



/* 处理光纵通道新接收到的帧，供逻辑图任务调用  2007-11-28日 张云
   参数   iOptCh
   返回   EP_SUCCESS,操作成功
          其他，操作失败*/
EP_STATUS   OPT_DealChNewRecvFrm(int  iOptCh)
{
    /*若该通道未初始化，返回失败  */
    int  j;
    int iLockKey;
    int iCurRecvCnt;
    uint32_t  ulCurDealPos;
    uint8_t   * pucRecv;
    OPT_CH_NEED_DEAL_FRM_INFO  *pDealInfo;
    UINT32 ulRcvBaseH;
    UINT32  ulRcvBaseL;
    uint8_t  ucRcvLocalSamCnt;
    int  iRcvByteLen;

    if(!(abOptChIsInitOver_g[iOptCh]))
    {
        return  EP_SUCCESS;
    }

    pDealInfo=aOptChNeedDealFrmInfo+iOptCh;

    if(pDealInfo->ulNeedDealCnt>=OPT_DEAL_FR_BUF_NUM)
    {
        /*说明异常,逻辑图来不及处理  */
        iLockKey=intLock();
        pDealInfo->ulNeedDealCnt=0;
        pDealInfo->ulCurDealPos=pDealInfo->ulNewRecvPos;
        intUnlock(iLockKey);
        return  EP_ERROR;
    }

    iCurRecvCnt=pDealInfo->ulNeedDealCnt;

    for(j=0; j<iCurRecvCnt; j++)
    {
        /*对每个接收帧处理  */

        /*这里必须保证数据的完整性  */
        iLockKey=intLock();
        pDealInfo->ulNeedDealCnt--;
        pDealInfo->ulCurDealPos++;
        if(pDealInfo->ulCurDealPos>=OPT_DEAL_FR_BUF_NUM)
        {
            pDealInfo->ulCurDealPos=pDealInfo->ulCurDealPos-OPT_DEAL_FR_BUF_NUM;
        }
        intUnlock(iLockKey);

        ulCurDealPos=pDealInfo->ulCurDealPos;
        pucRecv=pDealInfo->apucFrmBuf[ulCurDealPos];
        ulRcvBaseH=pDealInfo->ulRcvBaseHBuf[ulCurDealPos];
        ulRcvBaseL=pDealInfo->ulRcvBaseLBuf[ulCurDealPos];
        ucRcvLocalSamCnt=pDealInfo->ucRcvLocalSamCntBuf[ulCurDealPos];
        iRcvByteLen=pDealInfo->iRcvByteLenBuf[ulCurDealPos];
        Opt_DealOneFrm(iOptCh,
                       pucRecv,
                       iRcvByteLen,
                       ulRcvBaseH,
                       ulRcvBaseL,
                       ucRcvLocalSamCnt);
    }
    return  EP_SUCCESS;
}


/*光纵接收函数  2007-11-28日 张云修改
   参数  ucChNum，HDLC通道编号，CHAN_UP or CHAN_DOWN
         pucRcvBuf， 接收缓冲基址，实际是在BD中
         iRcvByteLen，接收数据长度
         ulRcvBaseH， 接收数据时的64位TIMEBASE的高32位
         ulRcvBaseL，接收数据时的64位TIMEBASE的低32位
   返回，OK 或 ERROR
         目前BSP这里要求都返回OK*/
int  OptRecvCmd(UINT8 ucChNum,
                uint8_t *pucRcvBuf,
                int  iRcvByteLen,
                UINT32 ulRcvBaseH,
                UINT32  ulRcvBaseL)
{
    OPT_CH_NEED_DEAL_FRM_INFO  *pDealInfo;
    int  iOptCh;
    uint32_t  ulNewRecvPos;
    int iLockKey;

    /* 如果长度为0,或数据无效
     * 则不缓冲本帧
     */
    if ((iRcvByteLen <= 0)
            || (pucRcvBuf == NULL))
    {
        return OK;
    }

    if(ucChNum==CHAN_UP)
    {
        iOptCh=0;
    }
    else  if(ucChNum==CHAN_DOWN)
    {
        iOptCh=1;
    }
    else
    {
        static  uint32_t  ulTestCnt_s1000=0;
        ulTestCnt_s1000++;
        if((ulTestCnt_s1000&0x3fffff)==1)
        {
            LOG_Write(LOG_KERNEL, "光差HDLC通信异常:有非预期通信接收!\n", NULL);
        }
        return  OK;
    }


    pDealInfo=aOptChNeedDealFrmInfo+iOptCh;

    /* 2007-11-28日 张云 保存缓冲的数据帧信息 */
    if(RE_FastTaskIsDrived())
    {
        /*若快速保护已经驱动起来了，则可以让快速保护处理光纵接收数据  */
        iLockKey=intLock();        /*保持数据完整性  */
        pDealInfo->ulNeedDealCnt++;
        pDealInfo->ulNewRecvPos++;
        if(pDealInfo->ulNewRecvPos>=OPT_DEAL_FR_BUF_NUM)
        {
            pDealInfo->ulNewRecvPos-=OPT_DEAL_FR_BUF_NUM;
        }
        ulNewRecvPos=pDealInfo->ulNewRecvPos;
        pDealInfo->apucFrmBuf[ulNewRecvPos]=pucRcvBuf;  /*帧指针缓冲数组  */
        pDealInfo->ulRcvBaseHBuf[ulNewRecvPos]=ulRcvBaseH;
        pDealInfo->ulRcvBaseLBuf[ulNewRecvPos]=ulRcvBaseL;
        pDealInfo->ucRcvLocalSamCntBuf[ulNewRecvPos]=OPT_GetSamClk();
        pDealInfo->iRcvByteLenBuf[ulNewRecvPos]=iRcvByteLen;
        intUnlock(iLockKey);
    }
    return  OK;
}


/*光纵单帧内容处理函数  2007-11-28日 张云添加
   参数  iOptCh，  光差通道号，0或者1，代表通道1或者通道2
         pucRcvBuf， 接收缓冲基址，实际是在BD中
         iRcvByteLen，接收数据长度
         ulRcvBaseH， 接收数据时的64位TIMEBASE的高32位
         ulRcvBaseL，接收数据时的64位TIMEBASE的低32位
         ulRcvLocalSamCnt,接收数据时的本机此时的采样计数
   返回，OK 或 ERROR
          */
int  Opt_DealOneFrm(int  iOptCh,
                    uint8_t *pucRcvBuf,
                    int  iRcvByteLen,
                    UINT32 ulRcvBaseH,
                    UINT32  ulRcvBaseL,
                    uint8_t  ulRcvLocalSamCnt)
{

    OPT_COM_SYN_DATA  *pChSynData;
    OPT_CH_STS_DATA   *pChStsData;
    OPT_FR_STS   *pChFrSts;
    OPT_CH_STS_REPORT  *pChStsRpt;
    OPT_CH_RECV_DATA_VALID_STS  *pChDataValidSts;
    OPT_CH_NEED_DEAL_FRM_INFO  *pDealInfo;
    OPT_CH_LAST_COM_STABLE_SAM_SYN_INFO  * pChSamSynInfo; /*2006-11-15日 张云  */
    OPT_TIME_BASE   LastRecvTimeBase;

    uint32_t   *pRecvCnt;

    uint8_t    ucLostFrmNum;
    BOOL   bNeedStoreBuf;


    static   uint32_t   ulRecvCnt_s[2]= {0,0};
    static   uint32_t   uiLastHdlcRecvCnt=0;
    RD_AI_MOD  *pAiMod;  /*2007-10-30日 张云修改  */


    pAiMod=&(aimodOpt_g[iOptCh]);
    uiLastHdlcRecvCnt=hdlcRecvNum;

    pChSynData=aOptComSynData_g+iOptCh;
    pChStsData=OptChStsData_g+iOptCh;
    pChFrSts=OptFrSts_g+iOptCh;
    pChStsRpt=aOptChStsRpt_g+iOptCh;
    pChDataValidSts=aOptChDataValidSts+iOptCh;
    pChSamSynInfo=aLastComStableSamSynInfo_g+iOptCh;/*2006-11-15日 张云  */
    pDealInfo=aOptChNeedDealFrmInfo+iOptCh;

    pRecvCnt=ulRecvCnt_s+iOptCh;

    LastRecvTimeBase=pChSynData->tLastRecvTime;
    pChSynData->tLastRecvTime.ulTimeBaseH=ulRcvBaseH;
    pChSynData->tLastRecvTime.ulTimeBaseL=ulRcvBaseL;

    if(iRcvByteLen==iOptDataByteLen_g)
    {

        (*pRecvCnt)++;/*接收计数  */

        if(Opt_DealOneFrmHead(iOptCh,pucRcvBuf,ulRcvLocalSamCnt,
                              pChSynData,pChStsData,pChStsRpt,pChDataValidSts,
                              &ucLostFrmNum,LastRecvTimeBase)!=OK)
        {
            return  ERROR;
        }

        /*更新该帧接收的数据  2007-11-28 张云*/
        if(OPT_CalcChInstSynDiffTime(pChSynData,pChStsData,ulRcvLocalSamCnt,
                                     aChValidInfo_g[iOptCh].bChPeerRecvIsStable))
        {
            /*用有效瞬时值更新该帧的帧状态缓冲数据  */
            OPT_RefreshFrSts(pChSynData
                             ,pChStsData,pChFrSts,ucLostFrmNum,iOptCh,TRUE);/*  */
        }
        else
        {
            /*若原始数据错误，则用平均值处理  */
            /*用平均值更新该帧的帧状态缓冲数据  */
            OPT_RefreshFrSts(pChSynData
                             ,pChStsData,pChFrSts,ucLostFrmNum,iOptCh,FALSE);/*  */
            return  ERROR;
        }

        /*更新该帧通道状态数据，当有接收时，就需要写接收的数据到 缓冲  */
        bNeedStoreBuf=OPT_RefreshChSts(pChSynData
                                       ,pChStsData,pChStsRpt,pChFrSts,iOptCh);/*可以考虑减少时间  */

        /* 考虑采样通道延迟时最近接收点对应本地采样节拍 */
        pChSynData->ucDelaySynLocalSamCnt = SynSamAdjust(pChSynData->ucLastRecvSynLocalSamCnt, (int)pChSynData->ucRltDif);

        if(bNeedStoreBuf)
        {
            if((pChStsData->iComSts==OPT_CH_COM_STABLE)
                    &&(pChStsData->iSamSynSts==OPT_CH_SAM_RELAY_SYN))
            {
                /*若是通信稳定且计算同步  */

                /*保存此次通信稳定且采样同步时的相关信息  */
                aLastComStableSamSynInfo_g[iOptCh].LastRcvPeerBaseTime=
                    pChSynData->tLastRecvTime;
                aLastComStableSamSynInfo_g[iOptCh].ucRecvPeerSamCnt=
                    pChSynData->ucLastRecvPeerSamCnt;
                aLastComStableSamSynInfo_g[iOptCh].ucSamSynLocalSamCnt=
                    pChSynData->ucDelaySynLocalSamCnt;
                aLastComStableSamSynInfo_g[iOptCh].ulLastComTime=
                    pChStsData->ulChAvrgTime;
                aLastComStableSamSynInfo_g[iOptCh].ulTsse=
                    pChStsData->iSamSynDiffAvrgTime;

                /*保存数据到缓冲 2007-11-28日 张云 */
                OPT_ChStoreNewRecvFrm(iOptCh
                                      ,apucSaveRecvDataPt_g[iOptCh]
                                      ,pChSynData->ucDelaySynLocalSamCnt
                                      ,pChStsData->iSamSynDiffAvrgTime
                                      ,pChSynData->ucLastRecvPeerSamCnt);
            }
            else  if((pChStsData->iComSts!=OPT_CH_COM_STABLE)
                     &&(pChStsData->iSamSynSts==OPT_CH_SAM_POINT_SYN))
            {
                /*若是通信不稳定且采样点同步  */
                int  iIntvl;
                int  iPointSynLocalSamCnt;

                iIntvl=(int)(pChSynData->ucLastRecvPeerSamCnt)
                       -(int)(aLastComStableSamSynInfo_g[iOptCh].ucRecvPeerSamCnt);

                iPointSynLocalSamCnt=SynSamAdjust(
                                         aLastComStableSamSynInfo_g[iOptCh].ucSamSynLocalSamCnt,iIntvl);

                OPT_ChStoreNewRecvFrm(iOptCh
                                      ,apucSaveRecvDataPt_g[iOptCh]
                                      ,iPointSynLocalSamCnt
                                      ,aLastComStableSamSynInfo_g[iOptCh].ulTsse
                                      ,pChSynData->ucLastRecvPeerSamCnt);
            }
            else
            {
                /*若是采样失步 也需要记录 */
                OPT_ChStoreNewRecvFrm(iOptCh
                                      ,apucSaveRecvDataPt_g[iOptCh]
                                      ,pChSynData->ucDelaySynLocalSamCnt
                                      ,pChStsData->iSamSynDiffAvrgTime
                                      ,pChSynData->ucLastRecvPeerSamCnt);
            }
        }
    }
    else
    {
        /* 进行通信异常处理 */
        Opt_DealOneFrmErr(iOptCh,iRcvByteLen);
    }

    return  OK;

}


/*处理光纵单帧的帧头内容处理函数  2009-1-20日 张云添加
   参数  iOptCh，  光差通道号，0或者1，代表通道1或者通道2
         pucRcvBuf， 接收缓冲基址，实际是在BD中
         ulRcvLocalSamCnt,接收数据时的本机此时的采样计数
         pChSynData，接收到的数据
         pChStsData,通道状态
         pChStsRpt,状态报告
         pChDataValidSts,数据有效状态
         pucRtLostFrmNum，返回的丢帧记数
         LastRecvTimeBase,上一帧接收的时刻
   返回，OK 或 ERROR
          */
int  Opt_DealOneFrmHead(int  iOptCh,
                        uint8_t *pucRcvBuf,
                        uint8_t  ulRcvLocalSamCnt,
                        OPT_COM_SYN_DATA  *pChSynData,
                        OPT_CH_STS_DATA  *pChStsData,
                        OPT_CH_STS_REPORT  *pChStsRpt,
                        OPT_CH_RECV_DATA_VALID_STS  *pChDataValidSts,
                        uint8_t  * pucRtLostFrmNum,
                        OPT_TIME_BASE  LastRecvTimeBase)
{

    OPT_TIME_BASE   newRecvTimeBase;
    uint8_t  *pucRecv;
    int  iZeroTime;
    uint8_t    ucPeerSamCnt;
    uint8_t    ucNewSamCnt;
    int iLockKey;

    char TempInfo[256];
    uint8_t ucLastRecvControl;
    static uint32_t uiaEnterCnt_s[2]= {0,0};

    uiaEnterCnt_s[iOptCh]++;

    /*计算本帧插零时间并去掉  这里消耗时间可以考虑减少*/

    iZeroTime=OPT_FrZeroTime(pucRcvBuf,iOptDataByteLen_g,iOptChType_g);

    OptTimebaseAddUsTime(&(pChSynData->tLastRecvTime),&newRecvTimeBase,0-iZeroTime);
    pChSynData->tLastRecvTime=newRecvTimeBase;

    /* 判断校验和正确与否　2009-2-15 ZY */

    /* 解析帧头 */
    pucRecv=pucRcvBuf;

    /* 与发送端匹配 */
    if ((uiaEnterCnt_s[iOptCh]&0x1F) == 1)
    {
        pChSynData->ucLastRecvControl=*pucRecv++;

        /* 控制字解析, 修改为临时变量位判断, 采取非0即1的模式处理布尔量 */
        ucLastRecvControl = pChSynData->ucLastRecvControl;

        aNodeMasterInfo_g[iOptCh+1].bIsMaster=ucLastRecvControl&OPT_CH_MASTER_MSK;
        aChValidInfo_g[iOptCh].bChPeerRecvIsStable=ucLastRecvControl&OPT_CH_RECV_STABLE;
        aChValidInfo_g[iOptCh].bChPeerRelayYabanIsRun=ucLastRecvControl&OPT_CH_RELAY_YABAN;
        aChValidInfo_g[iOptCh].bChRecvThirdChIsValid=ucLastRecvControl&OPT_OTHER_CH_VALID;

        if(pChSynData->ucLastRecvControl&OPT_CH_DATA_UNCREDIBLE)
        {
            /*2009-2-15 ZY  */
            if(!aChValidInfo_g[iOptCh].bChPeerSndDataIsUncredible)
            {
                sprintf(TempInfo, "提示，光纵通道%d对侧数据不可信.\n", iOptCh+1);
                LOG_Write(LOG_OPRATE, TempInfo, NULL);
            }
            aChValidInfo_g[iOptCh].bChPeerSndDataIsUncredible=TRUE;
        }
        else
        {
            if(aChValidInfo_g[iOptCh].bChPeerSndDataIsUncredible)
            {
                sprintf(TempInfo, "提示，光纵通道%d对侧数据可信.\n", iOptCh+1);
                LOG_Write(LOG_OPRATE, TempInfo, NULL);
            }
            aChValidInfo_g[iOptCh].bChPeerSndDataIsUncredible=FALSE;
        }

        /* 延迟点数 */
        pChSynData->ucDelay = (uint8_t)((pChSynData->ucLastRecvControl & DELAY_MASK) >> DELAY_POS);
        if (pChSynData->ucDelay>MAX_OPS_DELAY_NUM)
        {
            static BOOL bWrLogFlag = FALSE;

            if (!bWrLogFlag)
            {
                bWrLogFlag = TRUE;
                sprintf(TempInfo, "通道%d对侧采样通道延时过长,实际延迟点数为%d.\n",
                        iOptCh+1, pChSynData->ucDelay);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
            }

            return ERROR;
        }
    }
    else
    {
        pucRecv++;
    }


    /*获得随机编码  */

    aNodeMasterInfo_g[iOptCh+1].uiRandCode=((uint16_t)(*(pucRecv+1))<<8)
                                           |(uint16_t)(*pucRecv);/*是按INTEL低字节在前，  */

    pucRecv=pucRecv+2;

    ucPeerSamCnt=*pucRecv++;
    /*根据采样点，判断是否丢帧  */

    /* 2009-2-15 张云修改  */
    iLockKey=intLock();
    OptChLstRcvInfo_g[iOptCh].tLastRecvTime.ulTimeBaseH=pChSynData->tLastRecvTime.ulTimeBaseH;
    OptChLstRcvInfo_g[iOptCh].tLastRecvTime.ulTimeBaseL=pChSynData->tLastRecvTime.ulTimeBaseL;
    OptChLstRcvInfo_g[iOptCh].ucLastRecvPeerSamCnt=ucPeerSamCnt;

    /* 获取最近接收的ms数, 不是精确的接收时刻, 但可用于间隔计算 */
    OptChLstRcvInfo_g[iOptCh].ulLstRcvMsCnt = TM_High_Get_msCnt();
    intUnlock(iLockKey);

    ucNewSamCnt=SynSamAdjust(ucPeerSamCnt, -(int)pChSynData->ucLastRecvPeerSamCnt);

    if((ucNewSamCnt%iOptTxPts_g)==0)
    {
        /*若帧序未乱，检查有无丢帧情况  */
        /*static   uint32_t   ulLostCnt_s=0;*/

        *pucRtLostFrmNum=ucNewSamCnt/iOptTxPts_g-1;

    }
    else
    {
        /*若帧序号乱掉，则丢掉该帧,重新通信，此时多半是对侧重启或串扰  */

        pChSynData->ucLastRecvPeerSamCnt=ucPeerSamCnt;

        pChStsRpt->bChComStableFlag=FALSE ;
        pChStsRpt->bChSamSynFlag=FALSE;

        return  ERROR;
    }

    pChSynData->ucLastRecvPeerSamCnt=ucPeerSamCnt;


    pChSynData->ucLastRecvPeerRecvSamCnt=*pucRecv++;
    pChSynData->uiLastRecvPeerRecvtoSendIntvl=U8_TO_U16(*(pucRecv+1),*pucRecv);
    pucRecv=pucRecv+2;


    apucSaveRecvDataPt_g[iOptCh]=pucRecv;/*2006-11-27日 张云  保存数据指针，供保存数据时访问*/
    pucRecv++;/*跳过保护滞后点数字节  */
    pucRecv++;  /*跳过传送比例系数通道号字节  */
    pucRecv=pucRecv+4;/*跳过传送比例系数4字节  */


    /*若中断通信大于采样点同步允许时间，或是头一次通信，将前一段时间的无效数据去掉，
      若短时中断30毫秒，不需要去掉无效数据 ，2009-1-9日ZY */
    if((OptGetUsIntvlByBase(&pChSynData->tLastRecvTime,&LastRecvTimeBase)>30000)
            ||(pChDataValidSts->bFirstRecvFlag_s))
    {
        if(pChDataValidSts->bFirstRecvFlag_s)
        {
            /*若第1次通信  */
            pChDataValidSts->bFirstRecvFlag_s=FALSE;
            pChDataValidSts->bDataIsValid_s=FALSE;/*2006-11-14日  张云修改  */
            pChDataValidSts->iInValidRecvCnt_s=0;
            OPT_SetAllPhyCoffInValid(iOptCh);/*设置所有比例系数无效 2006-12-3日张云 */
        }
        else  if((OptGetUsIntvlByBase(&pChSynData->tLastRecvTime,&LastRecvTimeBase)>CPU_POINT_SYN_ALLOW_TIME))
        {
            /*若长时间通信中断，大于采样点同步允许时间　  */

            static  uint32_t  uiComLongHaltCnt_s[2]= {0,0};
            uiComLongHaltCnt_s[iOptCh]++;
            if((uiComLongHaltCnt_s[iOptCh]&0x3FFFF)==0x3FF0)
            {
                if(iOptCh==0)
                {
                    LOG_Write(LOG_OPRATE,"提示，光纵通道1 通信长时间中断后，开始重新通信!\n", NULL);
                }
                else
                {

                    LOG_Write(LOG_OPRATE, "提示，光纵通道2 通信长时间中断后，开始重新通信!\n", NULL);
                }
            }
            pChDataValidSts->bDataIsValid_s=FALSE;/*2006-11-14日  张云修改  */
            pChDataValidSts->iInValidRecvCnt_s=0;
            aLostFrmStatInfo_g[iOptCh].bNewComIsLongHaltInStat=TRUE; /*2006-11-16 张云  */
            OPT_SetAllPhyCoffInValid(iOptCh);/*设置所有比例系数无效 2006-12-3日张云 */
        }
        else
        {
            /*若短时间中断通信，提示，但不去掉以前的数据　2006-11-14日  张云*/

            static  uint32_t  uiComShortHaltCnt_s[2]= {0,0};
            uiComShortHaltCnt_s[iOptCh]++;
            if((uiComShortHaltCnt_s[iOptCh]&0x3FFFFF)==0x3FFF0)
            {
                if(iOptCh==0)
                {
                    LOG_Write(LOG_OPRATE, "提示，光纵通道1 短时间中断后，重新开始通信!\n", NULL);
                }
                else
                {

                    LOG_Write(LOG_OPRATE,"提示，光纵通道2 短时间中断后，重新开始通信!\n", NULL);
                }
            }
        }

    }
    pChDataValidSts->iInValidRecvCnt_s++;
    if(!pChDataValidSts->bDataIsValid_s)
    {
        if(pChDataValidSts->iInValidRecvCnt_s>2*iSynAverageFrCnt_g)
        {
            pChDataValidSts->bDataIsValid_s=TRUE;
        }

    }

    return  OK;
}


/*处理光纵单帧的帧异常处理函数  2009-1-20日 张云添加
   参数  iOptCh，  光差通道号，0或者1，代表通道1或者通道2
         iRcvResult,接收帧错误
   返回，OK 或 ERROR
          */
int  Opt_DealOneFrmErr(int  iOptCh,int  iRcvResult)
{

    /* 进行通信异常处理 */
    char TempInfo[256];
    static  uint32_t  uiComErrCnt_s[2]= {0,0};

    assert(iOptCh==0||iOptCh==1);
    uiComErrCnt_s[iOptCh]++;

    if((uiComErrCnt_s[iOptCh]&0x1FFFFF)==1)
    {
        /*约5分钟报一下  */
        switch(iRcvResult)
        {
            case  R_LEN_ERR:

                sprintf(TempInfo, "光纵通道%d接收帧长度错误.\n", iOptCh+1);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

                break;
            case  R_FIBERCHN_ERR:

                sprintf(TempInfo, "光纵通道%d接收时检测光纤附板错误.\n", iOptCh+1);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

                break;
            case  R_CHANNEL_NUMERR:

                sprintf(TempInfo, "光纵通道%d接收时地址串扰错误.\n", iOptCh+1);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

                break;

            case  R_CRITICAL_ERR:

                sprintf(TempInfo, "光纵通道%d接收时有严重错误.\n", iOptCh+1);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
                break;
            default:
                break;

        }
    }
    if(aOptChDataValidSts[iOptCh].bFirstSamSynFlag_s)
    {
        /*2006-11-15日　张云  */
        aOptChStsRpt_g[iOptCh].ulTotalFrErrNum++;
    }
    aOptChStsRpt_g[iOptCh].bChComStableFlag=FALSE ;
    aOptChStsRpt_g[iOptCh].bChSamSynFlag=FALSE;

    return  OK;

}




/*光纵发送函数，2007-11-28日修改 ，直接调用 */
EP_STATUS OptSendCmd()
{

    /*STATUS vxsts;*/
    uint8_t   *pucSndBuf;
    uint8_t   *pucAoDataSrc;
    uint8_t   *pucDoDataSrc;
    int  i;
    int  k;
    int  iAoDataCnt;
    int  iSendResult;
    int  iOptInitChNum;
    EP_ELEM_IO  *pElemIO;
    OPT_TIME_BASE   CurSamTimeBase;
    uint16_t    uiUsDiff;

    uint8_t   ucFastLogrpTaskMatchSamClk;  /*2006-11-12  */
    uint8_t   ucFastLogrpTaskSamClkDiff;

    uint8_t   aucSendAoCoff[4];/*发送的AO通道的比例系数 2006-11-26日  */
    uint8_t   ucCurSendCoffAoCh;

    uint8_t   ucChkSum;
    int  iLenQuot_8;
    int  iLenRemd_8;
    int  m;

    /*测试代码  */

    static   uint32_t   uiEnterCnt11_s=0;

    uiEnterCnt11_s++;

    Set_Hdlc_Out_Bit(0);/*刷新hdlc的原有设置状态  2007-8-16日 张云*/

    if(abOptChIsInitOver_g[1])
    {
        iOptInitChNum=2;
    }
    else
    {
        iOptInitChNum=1;
    }

    /*处理帧头信息  */

    if(OptSendCmdFrmHead(iOptInitChNum)!=EP_SUCCESS)
    {
        return  EP_CFG_ERR;
    }

    /*2006-11-27日 张云修改  */
    ucCurSendCoffAoCh=(uint8_t)(uiEnterCnt11_s%((uint32_t)OptAllAiSrcAoCfg_g.iAISrcAoNum));
    U32_TO_BYTES(aucSendAoCoff, (OptAllAiSrcAoCfg_g.ppAISrcAoCfgArr[ucCurSendCoffAoCh])->SrcAIPhyCoffUnion.ulVal);

    for(i=0; i<iOptInitChNum; i++)
    {
        /*对每个光纵通道都测试 */
        /*写帧头部  */
        pucSndBuf=aucSndDataBuf_g[i]+OPT_SND_BUF_HEAD_RSV_LEN;
        *pucSndBuf++=OptChSendCmd[i].ucSendControl;
        *pucSndBuf++=LO8(aNodeMasterInfo_g[0].uiRandCode);/*Intel  次序 */
        *pucSndBuf++=HI8(aNodeMasterInfo_g[0].uiRandCode);

        *pucSndBuf++=OptSendCommonCmd.ucLocalSamCnt;
        *pucSndBuf++=OptChLstRcvInfo_g[i].ucLastRecvPeerSamCnt;/*2007-11-28日 张云修改*/

        /*写INTERVAL  */
        OPT_GetSamTimeBase(OptSendCommonCmd.ucLocalSamCnt,&(CurSamTimeBase.ulTimeBaseH),&(CurSamTimeBase.ulTimeBaseL));
        uiUsDiff=(uint16_t)OptGetUsIntvlByBase(&CurSamTimeBase,&(OptChLstRcvInfo_g[i].tLastRecvTime));/*2007-11-28日 张云修改*/
        *pucSndBuf++=LO8(uiUsDiff);
        *pucSndBuf++=HI8(uiUsDiff);

        /*2006-12-3日修改  张云 添加快速任务当前采样点差*/
        if(OptFastLogrpTaskSendInfo_g.bFastLogrpTaskIsDrive)
        {
            /* 若是快速任务运行设置 */
            ucFastLogrpTaskMatchSamClk=
                OPT_GetMatchSamClkByAiCnt(&aimodOpt_g[i],OptFastLogrpTaskSendInfo_g.ulFastLogrpTaskAiCnt);
            ucFastLogrpTaskSamClkDiff=
                SynSamAdjust(OptSendCommonCmd.ucLocalSamCnt, -(int)ucFastLogrpTaskMatchSamClk);

        }
        else
        {
            ucFastLogrpTaskSamClkDiff=iOptTxPts_g;/*否则用默认值  */
        }
        *pucSndBuf++=ucFastLogrpTaskSamClkDiff;

        /*2006-11-27日 张云 修改，发送AI来源的AO比例系数，循环发送，每次发送一个AI来源的AO通道  */
        *pucSndBuf++=ucCurSendCoffAoCh;
        *pucSndBuf++=aucSendAoCoff[0];/*是按INTEL低字节在前，  */
        *pucSndBuf++=aucSendAoCoff[1];
        *pucSndBuf++=aucSendAoCoff[2];
        *pucSndBuf++=aucSendAoCoff[3];

        /*写AI来源的AO数据  2006-6-14*/
        pucAoDataSrc=pucOptAoDataByteBase_g;
        iAoDataCnt=iOptTxPts_g*iOptAISrcAoCh_g;
        for(k=0; k<iAoDataCnt; k++)
        {

            *pucSndBuf++=*pucAoDataSrc++;/* 按MOTO次序，高字节在前 发送定点，张云 2006-11-27日修改*/
            *pucSndBuf++=*pucAoDataSrc++;
        }

        /*写中间结果来源的AO数据，用相应通道的中间结果AO数据  */
        for(k=iOptAISrcAoCh_g; k<iOptAoCh_g; k++)
        {
            pElemIO=apMidSrcAOPt_g[i][k];
            if(pElemIO!=NULL)
            {
                *pucSndBuf++=LL8(pElemIO->now.ulVal);/*是按INTEL低字节在前，  */
                *pucSndBuf++=LH8(pElemIO->now.ulVal);/*需要考虑对齐，所以不能直接4字节赋值  */
                *pucSndBuf++=HL8(pElemIO->now.ulVal);
                *pucSndBuf++=HH8(pElemIO->now.ulVal);
            }
            else
            {

                *pucSndBuf++=aucZerofloat_g[0];/*2006-6-14 按INTEL次序，存入浮点0 */
                *pucSndBuf++=aucZerofloat_g[1];
                *pucSndBuf++=aucZerofloat_g[2];
                *pucSndBuf++=aucZerofloat_g[3];
            }
        }

        /*写相应通道的DO数据  */
        pucDoDataSrc=apucOptDoStsBase_g[i];
        for(k=0; k<iOptDioDataByteLen_g; k++)
        {
            *pucSndBuf++=*pucDoDataSrc++;
        }

        /* 计算校验和　2009-2-15 ZY */
        ucChkSum=0;
        iLenQuot_8=(iOptDataByteLen_g-1)/8;
        iLenRemd_8=(iOptDataByteLen_g-1)-iLenQuot_8*8;
        pucSndBuf=aucSndDataBuf_g[i]+OPT_SND_BUF_HEAD_RSV_LEN;
        for(m=0; m<iLenQuot_8; m++) /*需要提高效率  */
        {
            ucChkSum=ucChkSum+*pucSndBuf++;
            ucChkSum=ucChkSum+*pucSndBuf++;
            ucChkSum=ucChkSum+*pucSndBuf++;
            ucChkSum=ucChkSum+*pucSndBuf++;
            ucChkSum=ucChkSum+*pucSndBuf++;
            ucChkSum=ucChkSum+*pucSndBuf++;
            ucChkSum=ucChkSum+*pucSndBuf++;
            ucChkSum=ucChkSum+*pucSndBuf++;
        }
        for(m=0; m<iLenRemd_8; m++)
        {
            ucChkSum=ucChkSum+*pucSndBuf++;
        }
        *pucSndBuf=(uint8_t)(~ucChkSum);

        /*调用发送命令  */
        if(i==0)
        {
            iSendResult=hdlc_send_raw(CHAN_UP, aucSndDataBuf_g[i]+OPT_SND_BUF_HEAD_RSV_LEN, iOptDataByteLen_g);
            if(iSendResult!=0)
            {
                /*进行通信的异常处理  */
                OptSendCmdFrmErr(i,iSendResult);
            }
        }
        else  if(i==1)
        {
            iSendResult=hdlc_send_raw(CHAN_DOWN, aucSndDataBuf_g[i]+OPT_SND_BUF_HEAD_RSV_LEN, iOptDataByteLen_g);
            if(iSendResult!=0)
            {
                /*进行通信的异常处理  */

                OptSendCmdFrmErr(i,iSendResult);
            }
        }

    }/*for结束  */

    return EP_SUCCESS;
}



/*光纵发送帧头处理函数，2008-1-20日修改 ,ZY
  参数，iOptInitChNum,配置的通道个数，1
  返回， */
EP_STATUS  OptSendCmdFrmHead(int  iOptInitChNum)
{
#define MAX_TIME_SAME_CNT  19 /* 1s, 600/32=19 */
    static   uint32_t   uiEnterCnt_s=0;
    static  BOOL   bCurHdlcClkIsMaster[2]= {TRUE,TRUE}; /*2009-4-14日  张云  */
    static  OPT_TIME_BASE	tLastRecvTimeBak[2];/*光纵最近接收时的TIMEBASE时间备份 */
    static uint32_t ulTimeSameCnt[2] = {0,0};

    uiEnterCnt_s++;
    if((uiEnterCnt_s&0x1F)==1)
    {
        /*每32次检测更新一次所有通道和节点的状态*/

        OPT_TIME_BASE   CurTimeBase;
        uint32_t ulCurTimeMs;

        /*检测通道1自环定值  */
        /*获得通道1HDLC的时钟主从设置  */
        if((bCurHdlcClkIsMaster[0]&&(!bOptHdlcClkIsMaster_g[0]))
                ||((!bCurHdlcClkIsMaster[0])&&bOptHdlcClkIsMaster_g[0]))
        {
            /*若通道1HDLC时钟设置发生变化   2009-4-14日  ZY*/
            bCurHdlcClkIsMaster[0]=bOptHdlcClkIsMaster_g[0];
            /*重新设置光纵通道的HDLC时钟主从设置  */
            if(hdlc_clk_master_set(CHAN_UP, bOptHdlcClkIsMaster_g[0])!=0)
            {
                static   uint32_t   ulTestCnt12_s=0;
                ulTestCnt12_s++;

                if((ulTestCnt12_s&0x3FFF)==1)
                {
                    /* */
                    LOG_Write(LOG_KERNEL, "错误，光纵通道1的HDLC时钟主从设置失败!\n", NULL);
                }

                return  EP_CFG_ERR;
            }
        }

        if(iOptInitChNum==2)
        {
            /*检测通道2自环定值  */
            /*获得通道2HDLC的时钟主从设置 2009-4-14日 ZY */
            if((bCurHdlcClkIsMaster[1]&&(!bOptHdlcClkIsMaster_g[1]))
                    ||((!bCurHdlcClkIsMaster[1])&&bOptHdlcClkIsMaster_g[1]))
            {
                /*若通道2的HDLC时钟设置发生变化   2009-4-14日  ZY*/
                bCurHdlcClkIsMaster[1]=bOptHdlcClkIsMaster_g[1];
                if(hdlc_clk_master_set(CHAN_DOWN, bOptHdlcClkIsMaster_g[1])!=0)
                {
                    static   uint32_t   ulTestCnt12_s=0;
                    ulTestCnt12_s++;

                    if((ulTestCnt12_s&0x3FFF)==1)
                    {
                        /* */
                        LOG_Write(LOG_KERNEL, "错误，光纵通道2的HDLC时钟主从设置失败!\n", NULL);
                    }
                    return  EP_CFG_ERR;
                }
            }
        }/* if(iOptInitChNum==2)结束  */

        /*判定光纵通道1是否长时间未接收，若长时间未接收，则修改OptChStsData_g[0].iComSts的状态*/
        /* 如果tLastRecvTime 超过1s没有变化，同样认为通信长时间中断*/
        if((tLastRecvTimeBak[0].ulTimeBaseH == aOptComSynData_g[0].tLastRecvTime.ulTimeBaseH)
                && (tLastRecvTimeBak[0].ulTimeBaseL == aOptComSynData_g[0].tLastRecvTime.ulTimeBaseL))
        {
            if(ulTimeSameCnt[0] <= MAX_TIME_SAME_CNT)
            {
                ulTimeSameCnt[0]++;
            }
        }
        else
        {
            ulTimeSameCnt[0] = 0;
        }
        tLastRecvTimeBak[0].ulTimeBaseH = aOptComSynData_g[0].tLastRecvTime.ulTimeBaseH;
        tLastRecvTimeBak[0].ulTimeBaseL = aOptComSynData_g[0].tLastRecvTime.ulTimeBaseL;

        CurTimeBase=OptGetBaseTimerCnt();
        if((OptGetUsIntvlByBase(&CurTimeBase,&(aOptComSynData_g[0].tLastRecvTime))>CPU_POINT_SYN_ALLOW_TIME)
                ||(ulTimeSameCnt[0] >= MAX_TIME_SAME_CNT))
        {
            /*若长时间未接收，则认为通道1不正常，且报事件出来，告警 2009-2-19日 ZY修改 */
            static   uint32_t   ulTestCnt11_s=0;
            ulTestCnt11_s++;
            if(OptChStsData_g[0].iComSts!=OPT_CH_COM_INSTABLE)
            {
                OptChStsData_g[0].iComSts=OPT_CH_COM_INSTABLE;
                aOptChStsRpt_g[0].bChComStableFlag=FALSE ;
                aOptChStsRpt_g[0].bChSamSynFlag=FALSE;
            }

            if((ulTestCnt11_s&0x7FFFFFF)==0x3FF0)
            {
                /* 60天报一下，第1次迟些报 2006-11-15*/
                LOG_Write(LOG_KERNEL,"提示，光纵通道1 通信长时间未接收!\n",NULL);
            }

            aLostFrmStatInfo_g[0].bNewComIsLongHaltInStat=TRUE; /*2006-11-16 张云  */
            OPT_SetAllPhyCoffInValid(0);/*设置通道1所有比例系数无效 2006-12-3日张云 */
        }
        else
        {
            aLostFrmStatInfo_g[0].bNewComIsLongHaltInStat = FALSE; /*2006-11-16 张云  */
        }

        /* 瞬时接收中断判断 */
        ulCurTimeMs = TM_High_Get_msCnt();
        if (ulCurTimeMs-OptChLstRcvInfo_g[0].ulLstRcvMsCnt>CPU_RCV_INST_HALT)
        {
            aLostFrmStatInfo_g[0].bRcvInstHalt = TRUE;
        }
        else
        {
            aLostFrmStatInfo_g[0].bRcvInstHalt = FALSE;
        }

        if(OptChStsData_g[0].iComSts!=OPT_CH_COM_INSTABLE)
        {
            aChValidInfo_g[0].bChLocalRecvIsStable=TRUE;
        }
        else
        {
            aChValidInfo_g[0].bChLocalRecvIsStable=FALSE;
        }

        if(iOptInitChNum==2)
        {
            /*判定光纵通道2是否长时间未接收，若长时间未接收，则修改OptChStsData_g[1].iComSts的状态*/
            /* 如果tLastRecvTime 超过1s没有变化，同样认为通信长时间中断*/
            if((tLastRecvTimeBak[1].ulTimeBaseH == aOptComSynData_g[1].tLastRecvTime.ulTimeBaseH)
                    && (tLastRecvTimeBak[1].ulTimeBaseL == aOptComSynData_g[1].tLastRecvTime.ulTimeBaseL))
            {
                if(ulTimeSameCnt[1] <= MAX_TIME_SAME_CNT)
                {
                    ulTimeSameCnt[1]++;
                }
            }
            else
            {
                ulTimeSameCnt[1] = 0;
            }
            tLastRecvTimeBak[1].ulTimeBaseH = aOptComSynData_g[1].tLastRecvTime.ulTimeBaseH;
            tLastRecvTimeBak[1].ulTimeBaseL = aOptComSynData_g[1].tLastRecvTime.ulTimeBaseL;

            CurTimeBase=OptGetBaseTimerCnt();
            if((OptGetUsIntvlByBase(&CurTimeBase,&(aOptComSynData_g[1].tLastRecvTime))>CPU_POINT_SYN_ALLOW_TIME)
                    ||(ulTimeSameCnt[1] >= MAX_TIME_SAME_CNT))
            {
                /*若长时间未接收，则认为通道2不正常，且报事件出来，告警  */
                static   uint32_t   ulTestCnt12_s=0;
                ulTestCnt12_s++;
                if(OptChStsData_g[1].iComSts!=OPT_CH_COM_INSTABLE)
                {
                    OptChStsData_g[1].iComSts=OPT_CH_COM_INSTABLE;
                    aOptChStsRpt_g[1].bChComStableFlag=FALSE ;
                    aOptChStsRpt_g[1].bChSamSynFlag=FALSE;
                }

                if((ulTestCnt12_s&0x7FFFFFF)==0x3FF0)
                {
                    /* 60天报一下，第1次迟些报  2006-9-23*/
                    LOG_Write(LOG_KERNEL,"提示，光纵通道2 通信长时间未接收!\n",NULL);
                }
                aLostFrmStatInfo_g[1].bNewComIsLongHaltInStat=TRUE; /*2006-11-16 张云  */
                OPT_SetAllPhyCoffInValid(1);/*设置通道2所有比例系数无效 2006-12-3日张云 */
            }
            else
            {
                aLostFrmStatInfo_g[1].bNewComIsLongHaltInStat = FALSE;
            }

            /* 瞬时接收中断判断, 通道1判断已读取当前时标 */
            if (ulCurTimeMs-OptChLstRcvInfo_g[1].ulLstRcvMsCnt>CPU_RCV_INST_HALT)
            {
                aLostFrmStatInfo_g[1].bRcvInstHalt = TRUE;
            }
            else
            {
                aLostFrmStatInfo_g[1].bRcvInstHalt = FALSE;
            }

            if(OptChStsData_g[1].iComSts==OPT_CH_COM_STABLE)
            {
                aChValidInfo_g[1].bChLocalRecvIsStable=TRUE;
            }
            else
            {
                aChValidInfo_g[1].bChLocalRecvIsStable=FALSE;
            }
        }

        /*处理光纵主从自适应状态机，只有在更新所有通道和节点状态后，才进行状态处理  */
        OPT_SynAdaptDeal();

        /*获得最新通道状态后，才进行发送控制字填写  */
        /*设置通道1发送控制字  */
        /*设置主从  */
        OptChSendCmd[0].ucSendControl=0;
        if(aNodeMasterInfo_g[0].bIsMaster)
        {
            OptChSendCmd[0].ucSendControl=(OptChSendCmd[0].ucSendControl)|OPT_CH_MASTER_MSK;
        }

        /*设置通道1本侧接收稳定性  */
        if (!aLostFrmStatInfo_g[0].bRcvInstHalt)
        {
            OptChSendCmd[0].ucSendControl=(OptChSendCmd[0].ucSendControl)|OPT_CH_RECV_STABLE;
        }

        if(aChValidInfo_g[0].bChLocalRelayYabanIsRun)
        {
            OptChSendCmd[0].ucSendControl=(OptChSendCmd[0].ucSendControl)|OPT_CH_RELAY_YABAN;

        }

        /*获得另外一个通道,即通道2的通道有效状态  */
        if(aChValidInfo_g[1].bChIsValid)
        {
            OptChSendCmd[0].ucSendControl=(OptChSendCmd[0].ucSendControl)|OPT_OTHER_CH_VALID;
        }

        /*设置通道1本侧发送数据的可信性　　2009-2-15 ZY  */
        if(bOptSndDataIsUncredible(0))
        {
            OptChSendCmd[0].ucSendControl=(OptChSendCmd[0].ucSendControl)|OPT_CH_DATA_UNCREDIBLE;
        }

        /* 延迟点数传递 */
        OptChSendCmd[0].ucSendControl = OptChSendCmd[0].ucSendControl | (((uint8_t)adcGetDelayTime() << DELAY_POS) & DELAY_MASK);

        /*设置通道2的发送控制字 */
        if(iOptInitChNum==2)
        {
            OptChSendCmd[1].ucSendControl=0;
            if(aNodeMasterInfo_g[0].bIsMaster)
            {
                OptChSendCmd[1].ucSendControl=(OptChSendCmd[1].ucSendControl)|OPT_CH_MASTER_MSK;
            }

            /*设置通道2本侧接收稳定性  */
            if (!aLostFrmStatInfo_g[1].bRcvInstHalt)
            {
                OptChSendCmd[1].ucSendControl=(OptChSendCmd[1].ucSendControl)|OPT_CH_RECV_STABLE;
            }


            if(aChValidInfo_g[1].bChLocalRelayYabanIsRun)
            {
                OptChSendCmd[1].ucSendControl=(OptChSendCmd[1].ucSendControl)|OPT_CH_RELAY_YABAN;

            }

            /*获得另外一个通道,即通道1的通道有效状态  */
            if(aChValidInfo_g[0].bChIsValid)
            {
                OptChSendCmd[1].ucSendControl=(OptChSendCmd[1].ucSendControl)|OPT_OTHER_CH_VALID;
            }

            /*设置通道2本侧发送数据的可信性　　2009-2-15 ZY  */
            if(bOptSndDataIsUncredible(1))
            {
                OptChSendCmd[1].ucSendControl=(OptChSendCmd[1].ucSendControl)|OPT_CH_DATA_UNCREDIBLE;
            }

            /* 延迟点数传递 */
            OptChSendCmd[1].ucSendControl = OptChSendCmd[1].ucSendControl | (((uint8_t)adcGetDelayTime() << DELAY_POS) & DELAY_MASK);

        }/*if(iOptInitChNum==2)结束  */
    }/*if((uiEnterCnt_s&0x1F)==1)结束  */

    return  EP_SUCCESS;
}



/*光纵发送帧异常函数，2008-1-20日修改 ,ZY
  参数，iOptCh,光纵通道号
        iSendResult,发送异常代码
  返回， */
EP_STATUS  OptSendCmdFrmErr(int  iOptCh,int iSendResult)
{

    static  uint32_t  uiSend1ErrCnt_s[2]= {0,0};
    char TempInfo[256];

    assert((iOptCh==0)||(iOptCh==1));

    uiSend1ErrCnt_s[iOptCh]++;
    if((uiSend1ErrCnt_s[iOptCh]&0x1FFFFF)==1)
    {
        /*约5分钟报一下  */
        switch(iSendResult)
        {
            case  S_LEN_ERR:
                sprintf(TempInfo, "光纵通道%d发送帧长度错误.\n", iOptCh+1);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
                break;

            case  S_FIBERCHN_ERR:
                sprintf(TempInfo, "光纵通道%d发送时检测光纤附板错误.\n", iOptCh+1);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
                break;

            case  S_CHANNEL_NUMERR:
                sprintf(TempInfo, "光纵通道%d发送时地址串扰错误.\n", iOptCh+1);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
                break;

            case  S_CRITICAL_ERR:/*2007-9-3日张云修改  */
                sprintf(TempInfo, "光纵通道%d发送时有严重错误.\n", iOptCh+1);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
                break;
            default:
                break;
        }
    }

    aOptChStsRpt_g[iOptCh].bChComStableFlag=FALSE ;
    aOptChStsRpt_g[iOptCh].bChSamSynFlag=FALSE;
    return  EP_SUCCESS;
}


/* 保存光纵通道新接收到的帧的数据到缓冲，  2007-11-28日 张云
   参数   iOptCh,0或1，通道号
          pucRecv，接收的帧相关数据地址
          ucCurSynSamCnt 同步的采样点号
          iSamSynDiffTime  同步点的同步差，US
          ucLastRecvPeerSamCnt 接收的对方采样点号
   返回   EP_SUCCESS,操作成功
          其他，操作失败*/
EP_STATUS   OPT_ChStoreNewRecvFrm(int  iOptCh
                                  ,uint8_t  * pucRecv
                                  ,uint8_t  ucCurSynSamCnt
                                  ,int      iSamSynDiffTime
                                  ,uint8_t  ucLastRecvPeerSamCnt)
{


    int  i;
    int  k;
    uint8_t   ucRecvPhyCoffAiCh;

    uint8_t  *pucFrDi;
    uint8_t  *pucWrDi;
    uint8_t   *pucFrNewAISrAI;
    uint8_t   *pucFrMidSrAI;
    float     *pfWrData;
    float     *pfWrDouble;
    uint32_t     *pulWrData;
    uint32_t     *pulWrDouble;
    uint32_t  *pulCplxWrData;
    OPT_CH_STS *  pstsWrOptCh;

    int16_t  iFixData;
    uint32_t  ulData;
    float   fData;
    uint32_t   aulRsvMidSrcAiDataBuf[MAX_OPT_ALLOW_AIO_NUM];/*保存的中间结果来源的AI数据缓冲  */


    int   iOptDioDwordLen;
    int   iOptDioLeftByteLen;
    OPT_HWAI_COFF_CFG  *  pHwCofCfg;

    OPT_COM_SYN_DATA  *pChSynData;
    OPT_CH_STS_DATA   *pChStsData;
    OPT_CH_STS_REPORT  *pChStsRpt;
    OPT_CH_RECV_DATA_VALID_STS  *pChDataValidSts;
    OPT_CH_LAST_COM_STABLE_SAM_SYN_INFO  * pChSamSynInfo; /*2006-11-15日 张云  */
    RD_AI_MOD  *pAiMod;
    static BOOL bFstFlag[2]= {TRUE, TRUE};
    uint8_t  ucCalcSynSamCnt;

    pAiMod=&aimodOpt_g[iOptCh];/*2006-12-7日 张云  */
    pChSynData=aOptComSynData_g+iOptCh;
    pChStsData=OptChStsData_g+iOptCh;
    pChStsRpt=aOptChStsRpt_g+iOptCh;
    pChDataValidSts=aOptChDataValidSts+iOptCh;
    pChSamSynInfo=aLastComStableSamSynInfo_g+iOptCh;/*2006-12-3日 张云  */

    iOptDioDwordLen=iOptDioDataByteLen_g>>2;
    iOptDioLeftByteLen=iOptDioDataByteLen_g-(iOptDioDwordLen<<2);

    /*2006-12-3日张云添加  接收的对端快速任务滞后的采样点差  */
    pChSynData->ucPeerFastTaskClkDiff=*pucRecv++;

    /*注意这里可以用浮点 必须放到逻辑图任务中，206-11-27  */
    /*获得通道的比例系数  */
    ucRecvPhyCoffAiCh=*pucRecv++;
    if(ucRecvPhyCoffAiCh>=OptAllAiSrcAoCfg_g.iAISrcAoNum)
    {
        /* 若接收到错误的帧，则直接返回 2007-11-28日 张云 */

        static  uint32_t  ulTestCnt_s1000=0;
        ulTestCnt_s1000++;
        if((ulTestCnt_s1000&0x3fffff)==1)
        {
            LOG_Write(LOG_KERNEL, "光差HDLC通信异常:有非预期数据接收!\n", NULL);
        }
        return  EP_ERROR;
    }
    aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].fPhyCoff=
        U8_TO_FLT(*(pucRecv+3),*(pucRecv+2),*(pucRecv+1), *pucRecv);

    if(!bFstFlag[iOptCh])
    {
        aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].uPhyCoff.fVal=aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].fPhyCoff;
        if((aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].uPhyCoff.ulVal != aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].uPhyCoffBak.ulVal))
        {
            /* 如果前次和本次不一致，则置所有无效  */
            OPT_SetAllPhyCoffInValid(iOptCh);
        }
        else
        {
            aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].bPhyCoffIsValid=TRUE;
        }

        aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].uPhyCoffBak.fVal=aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].uPhyCoff.fVal;

    }
    else
    {
        /* 第一次执行 */
        bFstFlag[iOptCh]=FALSE;
        aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].uPhyCoff.fVal=aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].fPhyCoff;
        aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].uPhyCoffBak.fVal=aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].fPhyCoff;
        aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].bPhyCoffIsValid=TRUE;
    }

    /* 统一处理传统系数和数字化系数 */
    aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].fCoff=
        (aBoxAiCoffTable[iOptCh][ucRecvPhyCoffAiCh].fPhyCoff);

    pucRecv=pucRecv+4;

    if(OPT_GetAllPhyCoffValidFlag(iOptCh))
    {
        /*若所有比例系数有效，则记录  */
        /*写DI的数据到缓冲中 可以优化一下，，2006-12-2 */
        pucFrDi=pucRecv+iOptAioDataByteLen_g;
        pucWrDi=apucOptDiStsBase_g[iOptCh];
        for(i=0; i<iOptDioDwordLen; i++)
        {
            /*为了防止出现地址对齐的问题，只用字节拷贝，不用整数拷贝  */
            *pucWrDi++=*pucFrDi++;
            *pucWrDi++=*pucFrDi++;
            *pucWrDi++=*pucFrDi++;
            *pucWrDi++=*pucFrDi++;
        }
        for(i=0; i<iOptDioLeftByteLen; i++)
        {
            *pucWrDi++=*pucFrDi++;
        }

        /*获得原始AI通道来源的虚拟AI的新的采样点数据地址  */
        pucFrNewAISrAI=pucRecv;
        /*获得中间结果的虚拟AI的帧数据地址  2006-12-2*/
        pucFrMidSrAI=pucRecv+iOptTxPts_g*iOptAISrcAoCh_g*2;

        ucCalcSynSamCnt=SynSamAdjust(ucCurSynSamCnt, -iOptTxPts_g);

        /*写AI数据到缓冲  */
        for(i=0; i<iOptTxPts_g; i++)
        {
            /*处理AI采样数据  */

            ucCalcSynSamCnt=SynSamAdjust(ucCalcSynSamCnt, 1);

            pfWrData=(float  *)OPT_AI_Dat_P(pAiMod,
                                            ucCalcSynSamCnt,
                                            (COMPLEX**)&pulCplxWrData,
                                            &pstsWrOptCh,
                                            &pChSynData->ulLastRecvSynLocalAiCnt,
                                            pChSynData->ucAllRltDif,
                                            pChSynData->ucLastRecvCloseSamCnt);
            if(!pfWrData)
            {
                /*2006-6-9 若返回结果不对，则返回 */
                pChStsData->iSamSynSts=OPT_CH_SAM_MISS_SYN;      /*2006-10-7日张云修改  */
                return   EP_SUCCESS;
            }

            pfWrDouble=(float *)((uint8_t *)pfWrData-lgcaidb_g.ulBufBytes);
            pulWrData=(uint32_t  *)pfWrData;
            pulWrDouble=(uint32_t *)pfWrDouble;
            pucRecv=pucFrNewAISrAI;
            pucFrNewAISrAI_g[iOptCh] = pucFrNewAISrAI;  /* 用于测试 */
            /*读取AI来源的虚拟AI，每个2字节  */
            for(k=0,pHwCofCfg=aBoxAiCoffTable[iOptCh]+0 ; k<iOptAISrcAoCh_g ; k++,pHwCofCfg++)
            {
                /*不能按物理通道次序，要按逻辑通道次序写入缓冲  */

                /*2006-12-2日 注意是带符号 */
                iFixData=(int16_t)((((uint16_t)(*pucRecv))<<8)
                                   |((uint16_t)(*(pucRecv+1))));/*是按MOTO次序高字节在前，和INTEL是反的  */
                fData=((float)iFixData)*aBoxAiCoffTable[iOptCh][k].fCoff;

#ifndef NO_DBL_BUF
                *(pfWrData+pHwCofCfg->ucLgcAI)=fData;
                *(pfWrDouble+pHwCofCfg->ucLgcAI)=fData;
#else
                *(pfWrData+pHwCofCfg->ucLgcAI)=fData;
#endif
                pucRecv=pucRecv+2;/*2006-12-2  */

            }
            pucFrNewAISrAI=pucRecv;

            /*读取中间结果来源的虚拟AI，每个4字节,防止转换错误，用32位保存  按INTEL次序*/
            pucRecv=pucFrMidSrAI;
            for(k=iOptAISrcAoCh_g,pHwCofCfg=aBoxAiCoffTable[iOptCh]+iOptAISrcAoCh_g; k<iOptAoCh_g; k++,pHwCofCfg++)
            {
                /*不能按物理通道次序，要按逻辑通道次序写入缓冲 ,这里不用操作两次，1次就可以 了 */
                if(i==0)
                {
                    /*若是第1点，则组合一下，否则用前一次计算的数据  */
                    ulData=(((uint32_t)(*(pucRecv+3))<<24)
                            |((uint32_t)(*(pucRecv+2))<<16)
                            |((uint32_t)(*(pucRecv+1))<<8)
                            |(uint32_t)(*pucRecv));/*是按INTEL低字节在前，  */
                    aulRsvMidSrcAiDataBuf[k]=ulData;
                }
                else
                {
                    ulData=aulRsvMidSrcAiDataBuf[k];
                }
#ifndef NO_DBL_BUF
                *(pulWrData+pHwCofCfg->ucLgcAI)=ulData;
                *(pulWrDouble+pHwCofCfg->ucLgcAI)=ulData;
#else
                *(pulWrData+pHwCofCfg->ucLgcAI)=ulData;
#endif
                pucRecv=pucRecv+4;
            }

            /*设置该点光纵数据有效性  */
            pstsWrOptCh->lTsse=iSamSynDiffTime;
            pstsWrOptCh->iRcvSndDiffChgTime=
                pChStsData->iRcvSndDiffChgTime;/* 2009-2-13 ZY */
            pstsWrOptCh->bComValid=TRUE;/* 设置通信有效标志 ，只要有接收，则有效*/

            if(pChStsData->iComSts==OPT_CH_COM_STABLE)
            {
                pstsWrOptCh->bComStable=TRUE;
            }
            else
            {
                pstsWrOptCh->bComStable=FALSE;

            }

            if(!(aChValidInfo_g[iOptCh].bChPeerSndDataIsUncredible))
            {
                /*2009-3-5日 ZY 若对方送来的数据可信  */
                pstsWrOptCh->bDataIsCredible=TRUE;
            }
            else
            {
                pstsWrOptCh->bDataIsCredible=FALSE;
            }

            if(((pChStsData->iComSts==OPT_CH_COM_STABLE)
                    &&(pChStsData->iSamSynSts==OPT_CH_SAM_RELAY_SYN))
                    ||((pChStsData->iComSts!=OPT_CH_COM_STABLE)
                       &&(pChStsData->iSamSynSts==OPT_CH_SAM_POINT_SYN))
              )
            {
                /* 若通信稳定，且数据采样同步，或同步不稳定，且数据采样点同步。则设置同步有效标志
                　　　　　　　　　　　　　　　同时记录此时的相关信息,供通信不稳定时，用采样点同步使用 2009-1-21 张云*/
                pstsWrOptCh->bValid=TRUE;
                pChDataValidSts->bLastSynFlag_s=TRUE;

                if(!(pChDataValidSts->bFirstSamSynFlag_s))
                {
                    /* 若是首次同步，则置首次同步标志　2006-11-15日　张云 */
                    pChDataValidSts->bFirstSamSynFlag_s=TRUE;
                }
            }
            else
            {
                /*若完全失步  */

                if(pChDataValidSts->bLastSynFlag_s)
                {
                    /*若上次是同步，而本次是完全失步，则记录失步次数  2006-11-15日 张云 */
                    if(aOptChDataValidSts[iOptCh].bFirstSamSynFlag_s)
                    {
                        /*2006-11-15日　张云  */
                        pChStsRpt->ulTotalSamMissSynNum++;
                    }
                }

                pstsWrOptCh->bValid=FALSE;
                pChDataValidSts->bLastSynFlag_s=FALSE;
            }

            /*刷新DI缓冲， 2006-11-12日 张云修改 */

            OPT_End_Ai_Wr(pAiMod,
                          pChSynData->ulLastRecvSynLocalAiCnt,
                          (pChSynData->ulLastRecvSynLocalAiCnt-pChSynData->ucPeerFastTaskClkDiff));

            aulOptChLastRecvValidDataTime_g[iOptCh]=OptGetBaseTimerCnt();/*2013-5-20 日张云修改 */

        }/*for(i=0;i<iOptTxPts_g;i++)结束  */

    }/* if(OPT_GetAllPhyCoffValidFlag(iOptCh))结束 */

    return   EP_SUCCESS;
}



/*获得光纵通道的上次有效接收时间    2013-5-20日 张云
  参数: iOptChNum, 光纵通道号
  返回: 上次有效接收时间BASE */
OPT_TIME_BASE   OPT_GetOptChLastRecvValidDataTime(int  iOptChNum)
{
    return  aulOptChLastRecvValidDataTime_g[iOptChNum];
}


/*计算数据的插零时间
  参数: pucData,数据基址
        iDataLen,数据长度
        iOptType,通道类型
  返回:返回插零时间 */
int  OPT_FrZeroTime(uint8_t  *pucData,int  iDataLen,int  iOptType)
{
    /*这里必须进行优化，对2M，可以用简化算法，对64K，可以用这里的标准算法，，2006-5-30　*/

#if 0
    int  i=0;
    int  k=0;
    uint8_t   ucCurCh;
    uint8_t   *pucCurCh;
    int  iLeftOneNum;  /*连续1的个数 */
    int  iInsertZeroNum;  /*插零个数 */
#endif

    /* 2M直接返回缺省值 */
    return 5;

#if 0
    if(iOptType==1)
    {
        /*若是2M通道 ,默认返回值，因为最终是求平均值，误差可以忽略，则无所谓 ，可以节省CPU时间*/
        return  5;
    }
    else  if(iOptType==0)
    {
        /*若是64K，EDP02实际是1M,直接返回  2011-12-23  ZY*/
        return 5;

        iInsertZeroNum=0;
        iLeftOneNum=0;
        pucCurCh=pucData;

        for(i=0; i<iDataLen; i++)
        {
            ucCurCh=~(*pucCurCh++);/*这里取反，主要是为了后面提高效率  */
            for(k=0; k<8; k++)
            {
                if(ucCurCh&0x1)
                {
                    iLeftOneNum=0;
                    ucCurCh>>=1;
                }
                else
                {
                    iLeftOneNum++;
                    ucCurCh>>=1;
                    if(iLeftOneNum==5)
                    {
                        iInsertZeroNum++;
                        iLeftOneNum=0;
                    }
                }
            }
        }

        if(iOptType==1)
        {
            /*若是2M通道  */
            return  iInsertZeroNum/2;
        }
        else  if(iOptType==0)
        {
            return  iInsertZeroNum*1000/64;
        }
        else
        {
            assert(FALSE);
        }
    }
    else
    {
        assert(FALSE);
    }
    return  0;
#endif
}


/*获得光纵通道的总数
  参数: 无
  返回:光纵通道个数  */
int OPT_GetOptChTotalNum()
{
    return  2;
}


/*获得光纵通道的状态报告
  参数: iOptChNum,光纵通道号,从0开始
        pRtOptChStsRpt, 返回光纵通道的状态报告
  返回:成功与否  */
EP_STATUS  OPT_GetOptChStsRpt(int  iOptChNum,OPT_CH_STS_REPORT  * pRtOptChStsRpt)
{
    /* 2006-11-15日　张云修改 */
    HDLC_RECV_STATUS  ChnRecvSts;
    static  OPT_TIME_BASE   aLastQueryTimeBase_s[2]= {{0,0},{0,0}};

    assert(pRtOptChStsRpt);
    assert(iOptChNum==0||iOptChNum==1);
    if(abOptChIsInitOver_g[iOptChNum])
    {
        /*计算通道初始化到现在的秒时间差  */
        OPT_TIME_BASE  CurTimeBase;
        uint32_t   ulSecTime;
        uint32_t   ulMinTime;
        CurTimeBase=OptGetBaseTimerCnt();
        ulSecTime=OptGetSecIntvlByBase(&CurTimeBase,&(aLastQueryTimeBase_s[iOptChNum]));
        if(ulSecTime>=60)
        {
            /*若1分钟到时  */
            aLastQueryTimeBase_s[iOptChNum]=CurTimeBase;
            ulMinTime=ulSecTime/60;
        }
        else
        {
            ulMinTime=0;
        }


        if(iOptChNum==0)
        {
            hdlc_chnstatus_get(CHAN_UP, &ChnRecvSts);
        }
        else if(iOptChNum==1)
        {
            hdlc_chnstatus_get(CHAN_DOWN, &ChnRecvSts);
        }


        aOptChStsRpt_g[iOptChNum].ulHdlcCrcErr=ChnRecvSts.crc_counter-aulLastClearHdlcCrcErr_s[iOptChNum];/*减去上次清零时 2007-1-3日 张云 */
        aOptChStsRpt_g[iOptChNum].ulHdlcDpllErr=ChnRecvSts.dplle_counter-aulLastClearHdlcDpllErr_s[iOptChNum];
        aOptChStsRpt_g[iOptChNum].ulHdlcAddrErr=ChnRecvSts.nmar_counter-aulLastClearHdlcAddrErr_s[iOptChNum];
        aOptChStsRpt_g[iOptChNum].ulHdlcCpmBusy=ChnRecvSts.ovrun_counter-aulLastClearHdlcCpmBusy_s[iOptChNum];
        aOptChStsRpt_g[iOptChNum].ulHdlcRcvBusy=ChnRecvSts.disf_counter-aulLastClearHdlcRcvBusy_s[iOptChNum];

        if(ulMinTime>0)
        {
            /*若1分钟到时  */
            aOptChStsRpt_g[iOptChNum].ulFrLostNumPerSec=
                (aOptChStsRpt_g[iOptChNum].ulTotalFrLostNum-aulLastQueryTotalFrLostNum_s[iOptChNum])
                /ulMinTime;
            aulLastQueryTotalFrLostNum_s[iOptChNum]=aOptChStsRpt_g[iOptChNum].ulTotalFrLostNum;

            aOptChStsRpt_g[iOptChNum].ulFrDelayNumPerSec=
                (aOptChStsRpt_g[iOptChNum].ulTotalFrDelayNum-aulLastQueryTotalFrDelayNum_s[iOptChNum])
                /ulMinTime;
            aulLastQueryTotalFrDelayNum_s[iOptChNum]=aOptChStsRpt_g[iOptChNum].ulTotalFrDelayNum;

            aOptChStsRpt_g[iOptChNum].ulFrCRCErrNumPerSec=
                (aOptChStsRpt_g[iOptChNum].ulHdlcCrcErr-aulLastQueryHdlcCrcErr_s[iOptChNum])
                /ulMinTime;
            aulLastQueryHdlcCrcErr_s[iOptChNum]=aOptChStsRpt_g[iOptChNum].ulHdlcCrcErr;
        }

        /*2006-6-20日添加修改  */
        aOptChStsRpt_g[iOptChNum].bLocalIsMaster=aNodeMasterInfo_g[0].bIsMaster;      /*本机同步主从信息，TRUE，本机为主，FALSE，本机为从  */
        aOptChStsRpt_g[iOptChNum].uiLocalRandCode=aNodeMasterInfo_g[0].uiRandCode;   /* 本侧随机编码 */
        if(iOptChNum==0)
        {
            aOptChStsRpt_g[iOptChNum].bPeerIsMaster=aNodeMasterInfo_g[1].bIsMaster;      /*该通道对侧的同步主从信息，TRUE，对侧为主，FALSE，对侧为从  */
            aOptChStsRpt_g[iOptChNum].uiPeerRandCode=aNodeMasterInfo_g[1].uiRandCode;    /* 该通道对侧随机编码 */
        }
        else  if(iOptChNum==1)
        {
            aOptChStsRpt_g[iOptChNum].bPeerIsMaster=aNodeMasterInfo_g[2].bIsMaster;      /*该通道对侧的同步主从信息，TRUE，对侧为主，FALSE，对侧为从  */
            aOptChStsRpt_g[iOptChNum].uiPeerRandCode=aNodeMasterInfo_g[2].uiRandCode;    /* 该通道对侧随机编码 */
        }

    }

    *pRtOptChStsRpt=aOptChStsRpt_g[iOptChNum];

    /* 将所有累计和平均值　张云2006-12-8日 */
    pRtOptChStsRpt->ulTotalComInstableNum/=1;
    pRtOptChStsRpt->ulTotalFrLostNum/=1;
    pRtOptChStsRpt->ulTotalFrDelayNum/=1;

    /*2008-8-14日 张云  满足线路的要求，对帧错误个数，包括帧长错误，还包括CRC错误统计  */
    pRtOptChStsRpt->ulTotalFrErrNum=pRtOptChStsRpt->ulTotalFrErrNum
                                    +aOptChStsRpt_g[iOptChNum].ulHdlcCrcErr;
    pRtOptChStsRpt->ulTotalFrErrNum/=1;
    pRtOptChStsRpt->ulTotalChComeTimeChangeNum/=1;
    pRtOptChStsRpt->ulTotalSamMissSynNum/=1;

    pRtOptChStsRpt->ulHdlcCrcErr/=1;  /*2007-1-3日　张云修改。　 */
    pRtOptChStsRpt->ulHdlcDpllErr/=1;
    pRtOptChStsRpt->ulHdlcAddrErr/=1;
    pRtOptChStsRpt->ulHdlcCpmBusy/=1;
    pRtOptChStsRpt->ulHdlcRcvBusy/=1;

    pRtOptChStsRpt->ulFrCRCErrNumPerSec/=1;
    pRtOptChStsRpt->ulFrLostNumPerSec/=1;
    pRtOptChStsRpt->ulFrDelayNumPerSec/=1;

    return  EP_SUCCESS;
}




/*清除光纵通道状态
  参数: iOptChNum,光纵通道号,从0开始
  返回:成功与否  */
void    OPT_ClearOptChStsRpt(int  iOptChNum)
{
    HDLC_RECV_STATUS  ChnRecvSts;/*2007-1-3日　张云  */

    if(iOptChNum==0)
    {
        hdlc_chnstatus_get(CHAN_UP, &ChnRecvSts);
    }
    else if(iOptChNum==1)
    {
        hdlc_chnstatus_get(CHAN_DOWN, &ChnRecvSts);
    }
    if(iOptChNum==0||iOptChNum==1)
    {
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

        aulLastQueryTotalFrLostNum_s[iOptChNum]=0;
        aulLastQueryTotalFrDelayNum_s[iOptChNum]=0;
        aulLastQueryHdlcCrcErr_s[iOptChNum]=0;

        aulLastClearHdlcCrcErr_s[iOptChNum]=ChnRecvSts.crc_counter;/*保存上次清零时的HDLC异常信息，因为HDLC异常，没有清零功能 2007-1-3日 张云 */
        aulLastClearHdlcDpllErr_s[iOptChNum]=ChnRecvSts.dplle_counter;
        aulLastClearHdlcAddrErr_s[iOptChNum]=ChnRecvSts.nmar_counter;
        aulLastClearHdlcCpmBusy_s[iOptChNum]=ChnRecvSts.ovrun_counter;
        aulLastClearHdlcRcvBusy_s[iOptChNum]=ChnRecvSts.disf_counter;

    }

}
/*获得某通道的通信时间，给逻辑图用
  参数： iOptChNum，通道号
  返回： 该通道的通信时间 单位US 2006-12-21 张云 */
uint32_t   OPT_GetChComTime(int  iOptChNum)
{

    return  OptChStsData_g[iOptChNum].ulChAvrgTime;
}


/*获得某通道的实际通信时间，供显示用，不做其他用途
  参数： iOptChNum，通道号
  返回： 该通道的通信时间 单位US 2006-12-21 张云 */
uint32_t   OPT_GetChRealComTime(int  iOptChNum)
{
    uint32_t  ulNormChTime;
    uint32_t  ulRealChComTime;

    ulNormChTime=ulOptFrBaudSendTime_g+OPT_UNKNOWN_COM_TIME;
    if(OptChStsData_g[iOptChNum].ulChAvrgTime>ulNormChTime)
    {
        ulRealChComTime=OptChStsData_g[iOptChNum].ulChAvrgTime-ulNormChTime;/*减去固定时间 */
    }
    else
    {
        ulRealChComTime=10;
    }

    return  ulRealChComTime ;

}


/*功能： 判定光差通道的当前发送数据是否可信
  参数： iOptChNum，通道号
  返回： 当前发送数据可信与否
         TRUE，不可信
         FALSE，可信
  2009-2-15 张云 */
BOOL   bOptSndDataIsUncredible(int  iOptChNum)
{
    assert(iOptChNum==0||iOptChNum==1);
    if(EP_IN_HW_TEST())
    {
        /*若处于测试模式  */
        return  TRUE;
    }
    if(EP_Is_Enable_Alarm())
    {
        /*若严重错误*/
        return  TRUE;
    }
    if(!(RE_FastTaskIsDrived()))
    {
        /*若快速保护任务未驱动  */
        return   TRUE;
    }

    return  FALSE;
}

/* 获取光差通道延迟点数
 * Para:
 *     iOptChNum, 通道号.
 * Return:
 *     延迟点数.
 */
uint8_t OPT_GetOptChDelay(int iOptChNum)
{
    if (iOptChNum >= 2)
    {
        return 0;
    }
    LOG_Dbg_Msg("通信整点延迟 %d \n",
                aOptComSynData_g[iOptChNum].ucTemp, 0, 0, 0, 0, 0);

    return aOptComSynData_g[iOptChNum].ucDelay;
}

/* 显示光差接收数据.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void OPT_ShowData(void)
{
    int iOptCh, k;
    uint8_t *pucRecv = NULL;
    OPT_HWAI_COFF_CFG *pHwCofCfg = NULL;
    int16_t iFixData;
    float fData;

    taskLock();
    for (iOptCh = 0; iOptCh<2; iOptCh++)
    {
        if (!abOptChIsInitOver_g[iOptCh])
        {
            continue;
        }

        printf("光差通道%d-%d\n", iOptCh, aLostFrmStatInfo_g[iOptCh].bNewComIsLongHaltInStat);
        pucRecv = pucFrNewAISrAI_g[iOptCh];

        /* 读取AI来源的虚拟AI,每个2字节 */
        for (k = 0, pHwCofCfg=aBoxAiCoffTable[iOptCh]; k<iOptAISrcAoCh_g; k++, pHwCofCfg++)
        {
            /* 不能按物理通道次序,要按逻辑通道次序写入缓冲 */

            iFixData = (int16_t)((((uint16_t)(*pucRecv)) << 8)
                                 + ((uint16_t)(*(pucRecv+1)))); /* 是按MOTO次序高字节在前,和INTEL是反的 */
            fData = ((float)iFixData)*aBoxAiCoffTable[iOptCh][k].fCoff;

            printf("%d 二次值%f 数字量%d 系数%f\n", k, fData, iFixData, aBoxAiCoffTable[iOptCh][k].fCoff);
            pucRecv = pucRecv+2;
        }
    }
    taskUnlock();
}
