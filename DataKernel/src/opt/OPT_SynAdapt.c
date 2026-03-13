/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*       OPT_SynAdapt.c                                    1.0                      */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了光纵采样同步自适应模块的代码文件                                     */
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
/*         张云       2006.6.17                创建文件1.0版本                 */
/*                                                                             */
/*                                                                              */
/********************************************************************************/


#include  "OPT_MmiInterface.h"
#include   "OPT_VtBox.h"
#include  "OPT_SamSyn.h"
#include   "OPT_SynAdapt.h"
#include   "swcfg.h"
#include   "datetime.h"
#include  "RE_PublicDataDef.h"
#include  "OPT_Com.h"


OPT_NODE_MASTER_INFO_TYPE    aNodeMasterInfo_g[3];/*获得的三端主从同步状态  */
OPT_CH_VALID_INFO_TYPE       aChValidInfo_g[3];   /*获得的3个通道有效状态  */
uint32_t   ulChValidSts_g;    /*3个通道有效性状态，第0个BIT代表通道1，第1个BIT代表通道2，第2个BIT代表通道3  */

BOOL   bOptIs3EndRunMode_g=FALSE;                          /*光纵是否是3端运行模式，TRUE为3端运行模式，FALSE为2端运行模式  */
BOOL   bOptChIsRedundMode_g=FALSE;                           /*光纵是否是冗余2通道模式，TRUE为冗余2通道模式，FALSE为单通道模式    2006-7-12 */


/*获得新的随机数，
  参数 无
  返回  16位无符号随机数 */
uint16_t  OPT_GetNewRadom()
{
    uint32_t  ulCurUsCnt;
    uint16_t  uiLowUsCnt;

    /*由TIMEBASE计数器来获得 2013-5-20 ZY */
    ulCurUsCnt=OptGetBaseTimerLowCnt();
    uiLowUsCnt=(uint16_t)(ulCurUsCnt&0xFFFF);
    return   uiLowUsCnt;
}

/*初始化光纵同步自适应模块
   参数，无；
   返回 EP_SUCCESS,操作成功
        否则，操作失败  */
EP_STATUS   OPT_InitSynAdapt()
{
    /*2006-7-12*/
    int  i;

    for(i=0; i<3; i++)
    {
        aNodeMasterInfo_g[i].bIsMaster=FALSE;
        aNodeMasterInfo_g[i].uiRandCode=OPT_GetNewRadom();
    }

    for(i=0; i<3; i++)
    {
        aChValidInfo_g[i].bChLocalRecvIsStable=FALSE;
        aChValidInfo_g[i].bChPeerRecvIsStable=FALSE;
        aChValidInfo_g[i].bChLocalRelayYabanIsRun=TRUE;
        aChValidInfo_g[i].bChPeerRelayYabanIsRun=TRUE;/*对方的差动保护压板默认是投入的  */
        aChValidInfo_g[i].bChPeerSndDataIsUncredible=TRUE; /*2009-2-15 ZY,默认对侧送来的数据是不可信的  */
        aChValidInfo_g[i].bChIsSelfCircle=FALSE;
        aChValidInfo_g[i].bChRecvThirdChIsValid=FALSE;
        aChValidInfo_g[i].bChIsValid=FALSE;
    }

    bOptIs3EndRunMode_g=FALSE;
    bOptChIsRedundMode_g=FALSE;


    ulChValidSts_g=0;

    return  EP_SUCCESS;
}


/*光纵同步主从自适应处理
   参数，无
   返回  EP_SUCCESS,成功操作
         其他，失败*/
EP_STATUS   OPT_SynAdaptDeal()
{
    EP_STATUS   RtSts;

    if(bOptIs3EndRunMode_g)
    {
        RtSts=OPT_3EndSynAdaptDeal();
        return   RtSts;
    }
    else
    {
        RtSts=OPT_2EndSynAdaptDeal();
        return   RtSts;
    }
}



/*更新2端光纵通道有效性状态
   参数   无
   返回，EP_SUCCESS,操作成功
         其他，操作失败
     */
EP_STATUS   OPT_2EndUpdateChValidSts()
{
    /*2006-7-12 */

    OPT_CH_VALID_INFO_TYPE  *pCh1ValidInfo;
    OPT_CH_VALID_INFO_TYPE  *pCh2ValidInfo;

    pCh1ValidInfo=aChValidInfo_g;
    pCh2ValidInfo=aChValidInfo_g+1;

    /*获得通道1的有效性状态  */
    if((!(pCh1ValidInfo->bChLocalRecvIsStable))
            ||(!(pCh1ValidInfo->bChPeerRecvIsStable))
            ||(pCh1ValidInfo->bChIsSelfCircle))
    {
        /*当该通道接收不稳定或对方接收不稳定或自环,则认为该通道无效,2011-12-12  ZY*/
        pCh1ValidInfo->bChIsValid=FALSE;
    }
    else
    {
        /*否则认为该通道有效*/
        pCh1ValidInfo->bChIsValid=TRUE;
    }

    /*获得通道2的有效性状态  */
    if((!(pCh2ValidInfo->bChLocalRecvIsStable))
            ||(!(pCh2ValidInfo->bChPeerRecvIsStable))
            ||(pCh2ValidInfo->bChIsSelfCircle))
    {
        /*当该通道接收不稳定或对方接收不稳定或自环 ,则认为该通道无效,2011-12-12  ZY*/
        pCh2ValidInfo->bChIsValid=FALSE;
    }
    else
    {
        /*否则认为该通道有效*/
        pCh2ValidInfo->bChIsValid=TRUE;
    }


    return  EP_SUCCESS;

}

/*光纵2端同步主从自适应处理
   参数，无
   返回  EP_SUCCESS,成功操作
         其他，失败*/
EP_STATUS   OPT_2EndSynAdaptDeal()
{
    /*2006-7-12 */
    EP_STATUS   RtSts;

    /* 更新通道有效性的最新状态 */
    OPT_2EndUpdateChValidSts();

    if(bOptChIsRedundMode_g)
    {
        /*若是支持冗余通道模式  */
        RtSts=OPT_2EndRedundChDeal();
        return   RtSts;
    }
    else
    {
        RtSts=OPT_2EndSingleChDeal();
        return   RtSts;
    }
}


/*光纵2端单通道主从自适应处理
   参数，无
   返回  EP_SUCCESS,成功操作
         其他，失败*/
EP_STATUS   OPT_2EndSingleChDeal()
{
    /*2006-7-12 */

    if((aChValidInfo_g[0].bChIsValid))
    {
        /*若通道1正常  */
        if(aNodeMasterInfo_g[0].bIsMaster&&aNodeMasterInfo_g[1].bIsMaster)
        {
            /*若两主，则将自己置为从  */
            aNodeMasterInfo_g[0].bIsMaster=FALSE;
        }
        else  if((!(aNodeMasterInfo_g[0].bIsMaster))&&(!(aNodeMasterInfo_g[1].bIsMaster)))
        {
            /*若两从，  */

            if(aNodeMasterInfo_g[0].uiRandCode>aNodeMasterInfo_g[1].uiRandCode)
            {
                /* 若自己大，则设置为主 */
                aNodeMasterInfo_g[0].bIsMaster=TRUE;
            }
            else  if(aNodeMasterInfo_g[0].uiRandCode==aNodeMasterInfo_g[1].uiRandCode)
            {
                /* 若相等，则产生新的随机数 */
                static  uint32_t  ulTestCnt1_s;
                ulTestCnt1_s++;
                if(((ulTestCnt1_s&0xFFFFFF)==0x0F))
                {
                    LOG_Write(LOG_KERNEL, "提示，不同光纵通道出现相同的随机编码，请检查是否自环!\n", NULL);

                }
                aNodeMasterInfo_g[0].uiRandCode=OPT_GetNewRadom();
            }
        }
        /*若一主一从，则不变　*/
    }
    else
    {
        /*若通道1无效，则将自己置为从  */
        aNodeMasterInfo_g[0].bIsMaster=FALSE;
    }

    return  EP_SUCCESS;

}


/*光纵2端冗余双通道主从自适应处理
   参数，无
   返回  EP_SUCCESS,成功操作
         其他，失败*/
EP_STATUS   OPT_2EndRedundChDeal()
{
    /*2006-7-12 */
    OPT_NODE_MASTER_INFO_TYPE   *pLocalInfo;
    OPT_NODE_MASTER_INFO_TYPE   *pPeerInfo;

    if(aChValidInfo_g[0].bChIsValid)
    {
        pLocalInfo=aNodeMasterInfo_g;
        pPeerInfo=aNodeMasterInfo_g+1;
    }
    else  if(aChValidInfo_g[1].bChIsValid)
    {
        pLocalInfo=aNodeMasterInfo_g;
        pPeerInfo=aNodeMasterInfo_g+2;
    }
    else
    {
        /*若通道1,2无效，则将自己置为从  ,直接返回*/
        aNodeMasterInfo_g[0].bIsMaster=FALSE;
        return  EP_SUCCESS;
    }

    if(pLocalInfo->bIsMaster&&pPeerInfo->bIsMaster)
    {
        /*若两主，则将自己置为从  */
        pLocalInfo->bIsMaster=FALSE;
    }
    else  if((!(pLocalInfo->bIsMaster))&&(!(pPeerInfo->bIsMaster)))
    {
        /*若两从，  */

        if(pLocalInfo->uiRandCode>pPeerInfo->uiRandCode)
        {
            /* 若自己大，则设置为主 */
            pLocalInfo->bIsMaster=TRUE;
        }
        else  if(pLocalInfo->uiRandCode==pPeerInfo->uiRandCode)
        {
            /* 若相等，则产生新的随机数 */
            static  uint32_t  ulTestCnt1_s;
            ulTestCnt1_s++;
            if(((ulTestCnt1_s&0xFFFFFF)==0x0F))
            {
                LOG_Write(LOG_KERNEL, "提示，不同光纵通道出现相同的随机编码，请检查是否自环!\n", NULL);
            }
            pLocalInfo->uiRandCode=OPT_GetNewRadom();
        }
    }
    /*若一主一从，则不变　*/

    return  EP_SUCCESS;
}





/*更新3端光纵通道有效性状态
   参数   无
   返回，EP_SUCCESS,操作成功
         其他，操作失败
     */
EP_STATUS   OPT_3EndUpdateChValidSts()
{
    /*2006-7-12 */
    OPT_CH_VALID_INFO_TYPE  *pCh1ValidInfo;
    OPT_CH_VALID_INFO_TYPE  *pCh2ValidInfo;
    OPT_CH_VALID_INFO_TYPE  *pCh3ValidInfo;

    pCh1ValidInfo=aChValidInfo_g;
    pCh2ValidInfo=aChValidInfo_g+1;
    pCh3ValidInfo=aChValidInfo_g+2;

    /*获得通道1的有效性状态  */
    if((!(pCh1ValidInfo->bChLocalRecvIsStable))
            ||(!(pCh1ValidInfo->bChPeerRecvIsStable))
            ||((!(pCh1ValidInfo->bChLocalRelayYabanIsRun))&&(!(pCh1ValidInfo->bChPeerRelayYabanIsRun)))
            ||(pCh1ValidInfo->bChIsSelfCircle))
    {
        /*当该通道接收不稳定或对方接收不稳定或该通道双方保护压板退出 ,则认为该通道无效  2011-12-12  zy*/
        pCh1ValidInfo->bChIsValid=FALSE;
    }
    else
    {
        /*否则认为该通道有效*/
        pCh1ValidInfo->bChIsValid=TRUE;
    }

    /*获得通道2的有效性状态  */
    if((!(pCh2ValidInfo->bChLocalRecvIsStable))
            ||(!(pCh2ValidInfo->bChPeerRecvIsStable))
            ||((!(pCh2ValidInfo->bChLocalRelayYabanIsRun))&&(!(pCh2ValidInfo->bChPeerRelayYabanIsRun)))
            ||(pCh2ValidInfo->bChIsSelfCircle))
    {
        /*当该通道接收不稳定或对方接收不稳定或该通道双方保护压板退出或该通道自环 ,则认为该通道无效 2011-12-12  ZY*/
        pCh2ValidInfo->bChIsValid=FALSE;
    }
    else
    {
        /*否则认为该通道有效*/
        pCh2ValidInfo->bChIsValid=TRUE;
    }

    /*要求通道1,通道2的获得,必须在通道3的前面  */
    /*获得通道3的有效性状态  */
    if((!(pCh1ValidInfo->bChIsValid))
            &&(!(pCh2ValidInfo->bChIsValid)))
    {
        /*若通道1,通道2都无效,则认为通道3无效  */
        pCh3ValidInfo->bChIsValid=FALSE;
    }
    else  if((pCh1ValidInfo->bChIsValid
              &&(!(pCh1ValidInfo->bChRecvThirdChIsValid)))
             ||(pCh2ValidInfo->bChIsValid
                &&(!(pCh2ValidInfo->bChRecvThirdChIsValid))))
    {
        /*若通道1有效,但得到的通道3无效,或通道2有效,但得到的通道3无效,则认为通道3无效  */
        pCh3ValidInfo->bChIsValid=FALSE;
    }
    else
    {
        /*否则认为通道3有效  */
        pCh3ValidInfo->bChIsValid=TRUE;
    }

    return  EP_SUCCESS;
}


/*光纵3端同步主从自适应处理
   参数，无
   返回  EP_SUCCESS,成功操作
         其他，失败*/
EP_STATUS   OPT_3EndSynAdaptDeal()
{
    /*2006-7-12 */
    int  i;
    uint32_t  ulTemp;
    /* 测试代码 */
    /*static  uint32_t   uiTestCnt10_s=0;*/

    /* 获得3通道的状态 */

    /* 更新通道有效性的最新状态 */
    OPT_3EndUpdateChValidSts();

    ulChValidSts_g=0;
    ulTemp=1;
    for(i=0; i<3; i++)
    {
        if(aChValidInfo_g[i].bChIsValid)
        {
            ulChValidSts_g=ulChValidSts_g|ulTemp;
        }
        else
        {
            ulChValidSts_g=ulChValidSts_g&(~ulTemp);
        }
        ulTemp=ulTemp<<1;
    }

    /*测试代码 2006-6-21日  */
    /*
       uiTestCnt10_s++;
       if((uiTestCnt10_s&0x2ff)==1)
       {
            LOG_Dbg_Msg("Cur AlL  Ch Sts is  %d  \n",ulChValidSts_g,0,0,0,0,0);
       }
    */


    /* 根据通道相应状态，进行相应处理 */
    switch(ulChValidSts_g)
    {
        case  CH111_STS:/*通道1，2，3都有效，3端完全运行态  */
            OPT_Adapt111StsDeal();
            break;
        case  CH110_STS:/*通道1无效，3端非完全运行态  */
            OPT_Adapt110StsDeal();
            break;
        case  CH101_STS:/*通道2无效，3端非完全运行态   */
            OPT_Adapt101StsDeal();
            break;
        case  CH100_STS:/*通道1，2无效，独立运行态  */
            OPT_Adapt100StsDeal();
            break;
        case  CH011_STS:/*通道3无效，3端非完全运行态  */
            OPT_Adapt011StsDeal();
            break;
        case  CH010_STS:/*通道1，3无效， 2端运行态  */
            OPT_Adapt010StsDeal();
            break;
        case  CH001_STS:/*通道2，3无效， 2端运行态  */
            OPT_Adapt001StsDeal();
            break;
        case  CH000_STS:/*通道1，2，3无效，独立运行态  */
            OPT_Adapt000StsDeal();
            break;
        default:
            assert(FALSE);
            break;

    }
    return   EP_SUCCESS;
}



/* 光纵通道1，2，3都有效，3端完全运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt111StsDeal()
{
    int  iMasterCnt;
    int  i;

    /*求得3端的主个数  */
    iMasterCnt=0;
    for(i=0; i<3; i++)
    {
        if(aNodeMasterInfo_g[i].bIsMaster)
        {
            iMasterCnt++;
        }
    }
    /*若只有一个主，则不用修改主从状态,这是正常状态  */
    if(iMasterCnt==1)
    {
        return  EP_SUCCESS;
    }
    else/*若不止一个主，或无主  */
    {
        uint16_t   uiNode1Code;
        uint16_t   uiNode2Code;
        uint16_t   uiNode3Code;

        uiNode1Code=aNodeMasterInfo_g[0].uiRandCode;
        uiNode2Code=aNodeMasterInfo_g[1].uiRandCode;
        uiNode3Code=aNodeMasterInfo_g[2].uiRandCode;

        if(iMasterCnt==0)/*若没有一个主　*/
        {
            if((uiNode1Code>uiNode2Code)&&(uiNode1Code>uiNode3Code))
            {
                /*若自己是最大的，则将自己置为主  */
                aNodeMasterInfo_g[0].bIsMaster=TRUE;
                return  EP_SUCCESS;
            }
            else  if((uiNode1Code==uiNode2Code)||(uiNode1Code==uiNode3Code))
            {
                /*若自己和别人相同，则重新设置随机编码  */
                static  uint32_t  ulTestCnt1_s;
                ulTestCnt1_s++;
                if(((ulTestCnt1_s&0xFFFFFF)==0x0F))
                {
                    LOG_Write(LOG_KERNEL, "提示，不同光纵通道出现相同的随机编码，请检查是否自环!\n", NULL);
                }

                aNodeMasterInfo_g[0].uiRandCode=OPT_GetNewRadom();
                return  EP_SUCCESS;
            }
        }
        else/*若有2到3个主  */
        {
            if(!(aNodeMasterInfo_g[0].bIsMaster))
            {
                /*若自己不是主，则返回  */
                return  EP_SUCCESS;
            }
            else
            {
                if((uiNode1Code>uiNode2Code)&&(uiNode1Code>uiNode3Code))
                {
                    /*若自己是主，且是最大的，则返回  */
                    return   EP_SUCCESS;
                }
                else
                {
                    /*若自己不是最大的，则自己置为从  */
                    aNodeMasterInfo_g[0].bIsMaster=FALSE;
                    if((uiNode1Code==uiNode2Code)||(uiNode1Code==uiNode3Code))
                    {
                        /*若自己和别人相同，则重新设置随机编码  */
                        static  uint32_t  ulTestCnt2_s;
                        ulTestCnt2_s++;
                        if(((ulTestCnt2_s&0xFFFFFF)==0x0F))
                        {
                            LOG_Write(LOG_KERNEL, "提示，不同光纵通道出现相同的随机编码，请检查是否自环!\n", NULL);

                        }
                        aNodeMasterInfo_g[0].uiRandCode=OPT_GetNewRadom();
                        return  EP_SUCCESS;
                    }
                }/*else结束  */

            }/*结束  */
        }/*结束  */

    }/*else结束  */

    return  EP_SUCCESS;
}


/* 通道1无效，3端非完全运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt110StsDeal()
{
    if(aNodeMasterInfo_g[0].bIsMaster)
    {
        /*若自己是主，则置为从  */
        aNodeMasterInfo_g[0].bIsMaster=FALSE;
    }
    return  EP_SUCCESS;
}


/* 通道2无效，3端非完全运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt101StsDeal()
{
    if(aNodeMasterInfo_g[0].bIsMaster)
    {
        /*若自己是主，则置为从  */
        aNodeMasterInfo_g[0].bIsMaster=FALSE;
    }
    return  EP_SUCCESS;
}


/* 通道1，2无效，独立运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt100StsDeal()
{
    if(aNodeMasterInfo_g[0].bIsMaster)
    {
        /*若自己是主，则置为从  */
        aNodeMasterInfo_g[0].bIsMaster=FALSE;
    }
    return  EP_SUCCESS;
}


/*  通道3无效，3端非完全运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt011StsDeal()
{

    /* 测试代码 */
    /*  static  uint32_t   uiTestCnt10_s=0;*/
    /*测试代码 2006-6-21日  */
    /*
    uiTestCnt10_s++;
    if((uiTestCnt10_s&0x2ff)==1)
    {
         LOG_Dbg_Msg("Cur Local Master is  %d  \n",aNodeMasterInfo_g[0].bIsMaster,0,0,0,0,0);
    }
    */
    if(!(aNodeMasterInfo_g[0].bIsMaster))
    {
        /*若自己是从，则置为主  */
        aNodeMasterInfo_g[0].bIsMaster=TRUE;
    }
    return  EP_SUCCESS;
}

/* 通道1，3无效， 2端运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt010StsDeal()
{
    int  iMasterCnt;

    iMasterCnt=0;
    if(aNodeMasterInfo_g[0].bIsMaster)
    {
        iMasterCnt++;
    }
    if(aNodeMasterInfo_g[2].bIsMaster)
    {
        iMasterCnt++;
    }

    if(iMasterCnt==1)
    {
        /*若只有一主，则返回  */
        return  EP_SUCCESS;
    }
    else  if(iMasterCnt==0)
    {
        /*若无主  */
        if(aNodeMasterInfo_g[0].uiRandCode>aNodeMasterInfo_g[2].uiRandCode)
        {
            /* 若自己大，则设置为主 */
            aNodeMasterInfo_g[0].bIsMaster=TRUE;
        }
        else  if(aNodeMasterInfo_g[0].uiRandCode==aNodeMasterInfo_g[2].uiRandCode)
        {
            /* 若相等，则产生新的随机数 */

            static  uint32_t  ulTestCnt1_s;
            ulTestCnt1_s++;
            if(((ulTestCnt1_s&0xFFFFFF)==0x0F))
            {
                LOG_Write(LOG_KERNEL, "提示，不同光纵通道出现相同的随机编码，请检查是否自环!\n", NULL);
            }
            aNodeMasterInfo_g[0].uiRandCode=OPT_GetNewRadom();
            return  EP_SUCCESS;
        }
    }
    else
    {
        /*若有2个主，  */
        if(aNodeMasterInfo_g[0].uiRandCode<aNodeMasterInfo_g[2].uiRandCode)
        {
            /* 若自己小，则设置为从 */
            aNodeMasterInfo_g[0].bIsMaster=FALSE;
        }
        else  if(aNodeMasterInfo_g[0].uiRandCode==aNodeMasterInfo_g[2].uiRandCode)
        {
            /* 若相等，则产生新的随机数 */
            static  uint32_t  ulTestCnt2_s;
            ulTestCnt2_s++;
            if(((ulTestCnt2_s&0xFFFFFF)==0x0F))
            {
                LOG_Write(LOG_KERNEL, "提示，不同光纵通道出现相同的随机编码，请检查是否自环!\n", NULL);
            }
            aNodeMasterInfo_g[0].uiRandCode=OPT_GetNewRadom();
            return  EP_SUCCESS;
        }

    }
    return  EP_SUCCESS;
}

/* 通道2，3无效， 2端运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt001StsDeal()
{
    int  iMasterCnt;

    iMasterCnt=0;
    if(aNodeMasterInfo_g[0].bIsMaster)
    {
        iMasterCnt++;
    }
    if(aNodeMasterInfo_g[1].bIsMaster)
    {
        iMasterCnt++;
    }

    if(iMasterCnt==1)
    {
        /*若只有一主，则返回  */
        return  EP_SUCCESS;
    }
    else  if(iMasterCnt==0)
    {
        /*若无主  */
        if(aNodeMasterInfo_g[0].uiRandCode>aNodeMasterInfo_g[1].uiRandCode)
        {
            /* 若自己大，则设置为主 */
            aNodeMasterInfo_g[0].bIsMaster=TRUE;
        }
        else  if(aNodeMasterInfo_g[0].uiRandCode==aNodeMasterInfo_g[1].uiRandCode)
        {
            /* 若相等，则产生新的随机数 */
            static  uint32_t  ulTestCnt1_s;
            ulTestCnt1_s++;
            if(((ulTestCnt1_s&0xFFFFFF)==0x0F))
            {
                LOG_Write(LOG_KERNEL, "提示，不同光纵通道出现相同的随机编码，请检查是否自环!\n", NULL);
            }
            aNodeMasterInfo_g[0].uiRandCode=OPT_GetNewRadom();
            return  EP_SUCCESS;
        }
    }
    else
    {
        /*若有2个主，  */
        if(aNodeMasterInfo_g[0].uiRandCode<aNodeMasterInfo_g[1].uiRandCode)
        {
            /* 若自己小，则设置为从 */
            aNodeMasterInfo_g[0].bIsMaster=FALSE;
        }
        else  if(aNodeMasterInfo_g[0].uiRandCode==aNodeMasterInfo_g[1].uiRandCode)
        {
            /* 若相等，则产生新的随机数 */
            static  uint32_t  ulTestCnt2_s;
            ulTestCnt2_s++;
            if(((ulTestCnt2_s&0xFFFFFF)==0x0F))
            {
                LOG_Write(LOG_KERNEL, "提示，不同光纵通道出现相同的随机编码，请检查是否自环!\n", NULL);
            }
            aNodeMasterInfo_g[0].uiRandCode=OPT_GetNewRadom();
            return  EP_SUCCESS;
        }

    }
    return  EP_SUCCESS;

}

/* 通道1，2，3无效，独立运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt000StsDeal()
{
    if(aNodeMasterInfo_g[0].bIsMaster)
    {
        /*若自己是主，则置为从  */
        aNodeMasterInfo_g[0].bIsMaster=FALSE;
    }
    return   EP_SUCCESS;
}



/*****************************************光纵模块的由应用调用的相关函数，由应用层进行调用****************************************************/
/*功能：设置光纵hdlc clock的主从方式 2009-4-14日 张云,缺省处理是主方式
  参数，  bSetIsMaster，TRUE,为主，FALSE为从
  返回，无*/
void  OPT_SetHdlcClkMasterMode(BOOL  bSetIsMaster)
{
    bOptHdlcClkIsMaster_g[0]=bSetIsMaster;
    bOptHdlcClkIsMaster_g[1]=bSetIsMaster;
}


/*功能：设置单通道的光纵hdlc clock的主从方式,缺省处理是主方式,2009-4-14日　ZY
  参数， iOptChNum,光差通道编号，0代表通道1,1代表通道2,其他值无效
         bSetIsMaster，TRUE,为主，FALSE为从
  返回，无*/
void  OPT_SetSingleHdlcClkMasterMode(int  iOptChNum,BOOL  bSetIsMaster)
{
    if(iOptChNum==0||iOptChNum==1)
    {
        bOptHdlcClkIsMaster_g[iOptChNum]=bSetIsMaster;
    }
}

/*功能：设置光纵的运行模式,缺省处理是2端运行模式
  参数，  iRunMode，0为2端运行模式，1为3端运行模式，其他保留
  返回，无*/
void OPT_SetRunMode(int  iRunMode)
{
    if(iRunMode==0)
    {
        bOptIs3EndRunMode_g=FALSE;
    }
    else  if(iRunMode==1)
    {
        bOptIs3EndRunMode_g=TRUE;
    }
    else
    {
        assert(FALSE);
    }
}


/*功能：设置光纵通道的冗余运行模式,缺省处理是单通道不冗余模式
  参数，  iRedundMode，0为单通道不冗余模式，1为双通道冗余模式，其他保留
  返回，  无*/
void  OPT_SetRedundMode(int  iRedundMode)
{
    if(iRedundMode==0)
    {
        bOptChIsRedundMode_g=FALSE;
    }
    else  if(iRedundMode==1)
    {
        bOptChIsRedundMode_g=TRUE;
    }
    else
    {
        assert(FALSE);
    }
}


/*功能：设置光纵通道1自环模式,缺省处理是正常模式
  参数，bSetCh1IsSelfCircle，TRUE，设置为自环模式，FALSE，设置为正常模式  */
void  OPT_SetCh1SelfCircleMode(BOOL  bSetCh1IsSelfCircle)
{
    aChValidInfo_g[0].bChIsSelfCircle=bSetCh1IsSelfCircle;
}


/*功能：设置光纵通道2自环模式,缺省处理是正常模式
  参数，bSetCh2IsSelfCircle，TRUE，设置为自环模式，FALSE，设置为正常模式  */
void  OPT_SetCh2SelfCircleMode(BOOL  bSetCh2IsSelfCircle)
{

    aChValidInfo_g[1].bChIsSelfCircle=bSetCh2IsSelfCircle;
}


/*功能：通知平台，光纵通道1差动保护是否已经投入。,缺省处理是差动保护已投入
  参数， bCh1DiffIsRun，TRUE，通道1差动保护已经投入，FALSE，通道1差动保护已退出
  返回，无
  注意：只有当是3端模式时，有意义 */
void  OPT_NotifyCh1DiffRunExit(BOOL  bCh1DiffIsRun)
{
    aChValidInfo_g[0].bChLocalRelayYabanIsRun=bCh1DiffIsRun;
}


/*功能：通知平台，光纵通道2差动保护是否已经投入。,缺省处理是差动保护已投入
  参数， bCh2DiffIsRun，TRUE，通道2差动保护已经投入，FALSE，通道2差动保护已退出
  返回，无
  注意：只有当是3端模式时，有意义 */
void  OPT_NotifyCh2DiffRunExit(BOOL  bCh2DiffIsRun)
{
    aChValidInfo_g[1].bChLocalRelayYabanIsRun=bCh2DiffIsRun;
}

/*功能：获得本机节点的光差主从同步相关信息 2007-11-28-张云
  参数， pbRtNodeIsMaster，供返回该节点的同步主从标志的BOOL变量的地址(变量由调用方分配)
           ，TRUE为主，FALSE为从
         puiRtNodeRandCode，供返回该节点的16位无符号随机通道编码变量的地址(变量由调用方分配)
  返回，TRUE，表示获得信息有效
        FALSE，表示获得信息无效(比如未配置)
  注意：*/
BOOL  OPT_GetLocalNodeInfo(BOOL  * pbRtNodeIsMaster,uint16_t * puiRtNodeRandCode)
{
    assert(pbRtNodeIsMaster);
    assert(puiRtNodeRandCode);
    *pbRtNodeIsMaster=aNodeMasterInfo_g[0].bIsMaster;
    *puiRtNodeRandCode=aNodeMasterInfo_g[0].uiRandCode;
    if(aOptChStsRpt_g[0].bChInitFlag||aOptChStsRpt_g[1].bChInitFlag)
    {
        /*任何一个通道配置后，则返回信息有效  */
        return  TRUE;
    }
    else
    {
        return  FALSE;
    }
}


/*功能：获得通道1对端节点的光差主从同步相关信息 2007-11-28-张云
  参数， pbRtNodeIsMaster，供返回该节点的同步主从标志的BOOL变量的地址(变量由调用方分配)
           ，TRUE为主，FALSE为从
         puiRtNodeRandCode，供返回该节点的16位无符号随机通道编码变量的地址(变量由调用方分配)
  返回，TRUE，表示获得信息有效
        FALSE，表示获得信息无效(比如未配置或通信中断)
  注意：*/
BOOL  OPT_GetCh1PeerNodeInfo(BOOL  * pbRtNodeIsMaster,uint16_t * puiRtNodeRandCode)
{

    assert(pbRtNodeIsMaster);
    assert(puiRtNodeRandCode);
    *pbRtNodeIsMaster=aNodeMasterInfo_g[1].bIsMaster;
    *puiRtNodeRandCode=aNodeMasterInfo_g[1].uiRandCode;
    if(aOptChStsRpt_g[0].bChInitFlag&&aOptChStsRpt_g[0].bChComStableFlag)
    {
        /*通道1已配置且通信是稳定的，则返回信息有效  */
        return  TRUE;
    }
    else
    {
        return  FALSE;
    }

}


/*功能：获得通道2对端节点的光差主从同步相关信息 2007-11-28-张云
  参数， pbRtNodeIsMaster，供返回该节点的同步主从标志的BOOL变量的地址(变量由调用方分配)
           ，TRUE为主，FALSE为从
         puiRtNodeRandCode，供返回该节点的16位无符号随机通道编码变量的地址(变量由调用方分配)
  返回，TRUE，表示获得信息有效
        FALSE，表示获得信息无效(比如未配置或通信中断)
  注意：*/
BOOL  OPT_GetCh2PeerNodeInfo(BOOL  * pbRtNodeIsMaster,uint16_t * puiRtNodeRandCode)
{

    assert(pbRtNodeIsMaster);
    assert(puiRtNodeRandCode);
    *pbRtNodeIsMaster=aNodeMasterInfo_g[2].bIsMaster;
    *puiRtNodeRandCode=aNodeMasterInfo_g[2].uiRandCode;
    if(aOptChStsRpt_g[1].bChInitFlag&&aOptChStsRpt_g[1].bChComStableFlag)
    {
        /*通道2已配置且通信是稳定的，则返回信息有效  */
        return  TRUE;
    }
    else
    {
        return  FALSE;
    }

}

