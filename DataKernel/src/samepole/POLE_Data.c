/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*       POLE_Data.c                                    1.0                      */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了同杆并架数据模块的代码文件                                     */
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
/*         张云       2007.3.28                创建文件1.0版本                 */
/*                                                                             */
/*                                                                              */
/********************************************************************************/


#include   "vxWorks.h"
#include  "GooseInterface.h"
//#include  "mutual_61850.h"
#include  "realdata.h"
#include  "POLE_Data.h"
#include  "POLE_VtBox.h"
#include  "logic.h"
#include  "errtest.h"
#include  "logmsg.h"
#include "view.h"

static char *sGcRef_g;

static BOOL bPoleIsUsedInApp_g=FALSE;

#define POLE_AO_DATA_PUB_CHG_TH 0.01

/* static functions */

/* 写入同杆并架虚拟机箱DI的开入板状态，处理变位等信息
 * 参数：   plgcdi，  DI句柄
 *          iNowVal,  此时DI值
 * 返回值： 无 */
/* */
static  void POLE_Modify_DI(RD_LGC_DI_CH *plgcdi,int  iNowVal);

/* 控制同杆并架虚拟机箱DO数据实时输出
 * 参数：   pvDoCh，用来索引DO数据元素的void指针，应该是POLE_Init_DO的返回值
 *          bClose，TRUE=控合；FALSE=控分
 * 返回值： 无 */
void POLE_Set_DO(void *pvDoCh, BOOL bClose)
{

    POLE_DO_HND  *pHdl;

    assert(pvDoCh);
    pHdl=(POLE_DO_HND  *)pvDoCh;
    if(pHdl->ulPassWd!=POLE_DO_PASSWORD)
    {
        LOG_Write(LOG_KERNEL,"同杆并架机箱准备非法进行开出, 复位CPU\n",NULL);

        EP_Set_Sts_Bit(REBOOT_DLY|SYS_LOCK_DO);
        return;
    }

    pHdl->bCurVal=bClose;
    return ;
}


/* 读取同杆并架虚拟机箱DI数据实时状态
 * 参数：   pvDiCh，用来索引DI数据元素的void指针，应该是POLE_Init_DI的返回值
 * 返回值： 此DI通道的当前状态，TRUE=闭合；FALSE=打开 */
BOOL POLE_Get_DI(void *pvDiCh)
{

    POLE_DI_HND  *pHdl;
    BOOL   bDIVal;
    BOOL   bGetSuccess;

    assert(pvDiCh);
    pHdl=(POLE_DI_HND  *)pvDiCh;

    if(pHdl->iSubDaIdx>=0)
    {
        bGetSuccess=GO_GetActiveGoDIValByDaIndx
                    (pHdl->iSubDaIdx, pHdl->pSubMapData, &bDIVal);
        if(bGetSuccess)
        {
            return   bDIVal;
        }
        else
        {
            return  FALSE;
        }
    }
    else
    {
        return  FALSE;
    }
}


/* 读取同杆并架虚拟机箱AI数据实时状态
 * 参数：   pvAiCh，用来索引AI数据元素的void指针
 * 返回值： 此DI通道的当前状态，TRUE=闭合；FALSE=打开 */
float POLE_Get_AI(void *pvAiCh)
{

    POLE_AI_HND  *pHdl;
    float   fAIVal;
    BOOL   bGetSuccess;

    assert(pvAiCh);
    pHdl=(POLE_AI_HND  *)pvAiCh;

    /* 用Da端子号判断 */
    if(pHdl->iSubDaIdx<0)
    {
        /*若句柄无效  */
        static  uint32_t   ulTestCnt_s3=0;
        ulTestCnt_s3++;

        /*测试代码 2007-5-21日  */
        if((ulTestCnt_s3&0x3fffff)==1)
        {
            LOG_Dbg_Msg("POLE AI Get Val is failure! \n",0,0,0,0,0,0);
        }
        return  0.0;
    }
    bGetSuccess=GO_GetActiveGoAIValByDaIndx
                (pHdl->iSubDaIdx, pHdl->pSubMapData, &fAIVal);
    if(bGetSuccess)
    {
        static  uint32_t   ulTestCnt_s=0;
        ulTestCnt_s++;

        /*测试代码 2007-5-21日  */
        if((ulTestCnt_s&0x3fffff)==1)
        {
            LOG_Dbg_Msg("POLE AI Get Val is %d \n",(int)fAIVal,0,0,0,0,0);
        }
        return   fAIVal;
    }
    else
    {
        static  uint32_t   ulTestCnt_s1=0;
        ulTestCnt_s1++;

        /*测试代码 2007-5-21日  */
        if((ulTestCnt_s1&0x3fffff)==1)
        {
            LOG_Dbg_Msg("POLE AI Get Val is failure! \n",0,0,0,0,0,0);
        }
        return  0.0;
    }
}



/* 功能：获得同杆并架数据源的active goose,某AO通道输出的数据  2007-3-27日 张云
   参数：
        uiAONum：		在该数据源active goose的AO序号，序号从1开始，
        pfRtData:               返回的该AO通道的数据
   返回：
        TRUE； 若有匹配数据，则操作成功
        FALSE：操作失败
  	*/

BOOL  POLE_GetActiveGoAoData(uint16_t uiAONum,
                             float *pfRtData)
{
    EP_ELEM_IO *  pElemIO;

    if(uiAONum>PoleBoxAoCfg_g.iPoleAONum||uiAONum<1)
    {
        static   uint32_t  ulTestCnt_s=0;

        ulTestCnt_s++;
        if((ulTestCnt_s&0x3ffff)==1)
        {
            char TempInfo[256];

            if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
            {
                ER_Set_Err(EV_GSE_CONFI_ERR,
                           ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                           "Parallel device GOOSE AO config error\n", 0, 0);
            }
            else if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_GSE_CONFI_ERR,
                           ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                           "同杆装置 GOOSE AO 配置错误\n", 0, 0);
            }

            sprintf(TempInfo,
                    "active goose 获得同杆装置AO值时,AO序号越界,AO序号是%d!!\n",
                    uiAONum);
            LOG_Write(LOG_KERNEL, TempInfo, NULL);

        }

        *pfRtData=0.0;
        return   FALSE;
    }
    pElemIO=(EP_ELEM_IO *)PoleBoxAoCfg_g.apPoleBoxAoIdx[uiAONum-1]->pElemSrc;
    assert(pElemIO);
    *pfRtData=pElemIO->now.fVal;

    return   TRUE;
}


/* 功能：获得同杆并架数据源的active goose,某DO通道输出的数据  2007-3-27日 张云
   参数：
        uiDONum：		在该数据源active goose的DO序号，序号从1开始，
        pbRtData:               返回的该DO通道的数据
   返回：
        TRUE； 若有匹配数据，则操作成功
        FALSE：操作失败
  	*/

BOOL  POLE_GetActiveGoDoData(uint16_t uiDONum,
                             BOOL *pbRtData)
{
    BOOL   bCurVal;

    if(uiDONum>PoleBoxIOInfo_g.iPoleDoNum||uiDONum<1)
    {
        static   uint32_t  ulTestCnt_s=0;
        ulTestCnt_s++;
        if((ulTestCnt_s&0x3ffff)==1)
        {
            char TempInfo[256];

            if(ENG_MODE == 1)             /*2007-4-19日 张云修改，为了支持英文版  */
            {
                ER_Set_Err(EV_GSE_CONFI_ERR,
                           ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                           "Parallel device GOOSE DO config error\n", 0, 0);
            }
            else if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_GSE_CONFI_ERR,
                           ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                           "同杆装置 GOOSE DO 配置错误\n", 0, 0);
            }

            sprintf(TempInfo,
                    "active goose 获得同杆装置DO值时,DO序号越界,DO序号是%d!!\n",
                    uiDONum);
            LOG_Write(LOG_KERNEL, TempInfo, NULL);

        }

        *pbRtData=FALSE;
        return  FALSE;
    }

    bCurVal=PoleBoxIOInfo_g.aphPoleDoIdx[uiDONum-1]->bCurVal;
    *pbRtData=bCurVal;

    return   TRUE;
}




/* 取得同杆并架机箱AI逻辑通道和预处理数据指针,必须要求和本机的AD采样刷新同时调用
 * 参数：   pvAiMod，用来索引同杆并架机箱AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulSmplClk，本机的递增采样时钟，
 *          ppxWr，用来返回指向该同杆AI引擎的第0个预处理通道数据的指针
 * 返回值： 指向该同杆AI引擎的第0个逻辑采样通道数据的指针
 * 注意：   要求AI引擎按照采样次序调用此函数获得指针来写实时数据，
 *          AI引擎可以对指针进行直接的写操作（如果没有预处理通道，则*ppxWr无效），
 *          内部数据按照通道优先的方式进行组织 */
float *POLE_AI_Dat_P(void *pvAiMod, uint32_t ulSmplClk,COMPLEX **ppxWr)
{

    RD_AI_MOD *paimod;
    int i;
    uint32_t  ulMissedClk;
    static  BOOL   bFirstEnter_s=TRUE;

    assert(pvAiMod);

    paimod=(RD_AI_MOD*)pvAiMod;

    if (!bFirstEnter_s)
    {
        /* 若是非首次进去 */

        if((ulSmplClk==paimod->ulHeadClk+1) ||ulSmplClk==0)
        {
            /*若未丢点  */
            paimod->pfWork=(float*)((uint8_t*)paimod->pfWork+lgcaidb_g.uiChBytes);
            if (paimod->pfWork>=lgcaidb_g.pfBufEnd)
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
            /*AI有效标志可以由goose的接收状态决定  */
            if(POLE_Data_Is_Valid())
            {
                *paimod->pbAiValidDbWork=TRUE;/*表明该点的AI数据有效  */
            }
            else
            {
                *paimod->pbAiValidDbWork=FALSE;/*表明该点的AI数据有效  */
            }
            paimod->ulHeadClk=ulSmplClk;
            paimod->ulNextCnt++;
        }
        else  if((ulSmplClk==(paimod->ulHeadClk+2)%RD_SAM_SYN_CLK)
                 ||(ulSmplClk==(paimod->ulHeadClk+3)%RD_SAM_SYN_CLK))
        {
            /*若丢1点或2点  */
            if(ulSmplClk==(paimod->ulHeadClk+2)%RD_SAM_SYN_CLK)
            {
                ulMissedClk=1;
            }
            else
            {
                ulMissedClk=2;
            }
            if (paimod==&aimodPole_g)
            {
                static uint32_t  ulErrCnt=0;
                ulErrCnt++;
                if(ulErrCnt%20000==1)
                {
                    /*每20000次报一次，第1次必须报   */
                    LOG_Write(LOG_KERNEL, "提示: 同杆并架AI数据丢失 .\n", NULL);
                }
            }
            else
            {
                assert(FALSE);
            }
            for(i=0; i<=ulMissedClk; i++)
            {
                /*返回正确的写位置 */
                paimod->pfWork=(float*)((uint8_t*)paimod->pfWork+lgcaidb_g.uiChBytes);
                if (paimod->pfWork>=lgcaidb_g.pfBufEnd)
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

                if(i==ulMissedClk)
                {
                    /*若是有效点  *//*AI有效标志可以由goose的接收状态决定  */
                    if(POLE_Data_Is_Valid())
                    {
                        *paimod->pbAiValidDbWork=TRUE;
                    }
                    else
                    {
                        *paimod->pbAiValidDbWork=FALSE;
                    }

                }
                else
                {
                    /*若是无效点  */
                    *paimod->pbAiValidDbWork=FALSE;
                }

                paimod->ulNextCnt++;
            }

            paimod->ulHeadClk=ulSmplClk;
        }
        else
        {
            /*若丢多点,则设置错误  */

            if (paimod==&aimodPole_g)
            {
                static uint32_t  ulErrCnt=0;
                ulErrCnt++;
                if(ulErrCnt%20000==1)
                {
                    /*每20000次报一次，第1次必须报，但不闭锁  */
                    char TempInfo[256];
                    if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
                    {
                        ER_Set_Err(EV_SAMPLE_ERR, ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                                   "Parallel device receive error\n"
                                   ,0,0);
                    }
                    else if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SAMPLE_ERR, ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                                   "同杆装置接收异常\n"
                                   ,0,0);
                    }
                    sprintf(TempInfo,"同杆装置数据连续丢点,前次采样点是%d,本次采样点是%d!!\n"
                            , (int)paimod->ulHeadClk, (int)ulSmplClk);
                    LOG_Write(LOG_KERNEL, TempInfo, NULL);
                }
            }
            paimod->ulHeadClk=ulSmplClk;
        }

    }
    else
    {
        /*必须要求和本机的AD采样刷新同时调用和刷新*/
        uint32_t  ulCurDspModNextCnt;
        int  iCurDspModDbOfst;

        assert(paimod==&aimodPole_g);

        bFirstEnter_s=FALSE;

        /*此时根据DSP MOD的信息，来获得同杆并架机箱初始化时在DB中的相应位置，
          注意此时调用时，DSP MOD还没有进行此轮刷新  */
        assert(RD_Get_DSP_MOD_Info(&ulCurDspModNextCnt,&iCurDspModDbOfst)==EP_SUCCESS);
        assert((iCurDspModDbOfst)>=0&&(iCurDspModDbOfst)<lgcaidb_g.ulBufLen);

        /*前一点的MOD信息  */
        paimod->pfWork=paimod->pfDbBgn+iCurDspModDbOfst;
        paimod->pxWork=paimod->pxDbBgn+iCurDspModDbOfst;
        paimod->ulNextCnt=ulCurDspModNextCnt;
        paimod->pbAiValidDbWork=paimod->pbAiValidDbBgn+iCurDspModDbOfst;

        paimod->pfWork=(float*)((uint8_t*)paimod->pfWork+lgcaidb_g.uiChBytes);
        if (paimod->pfWork>=lgcaidb_g.pfBufEnd)
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
        /*AI有效标志可以由goose的接收状态决定  */

        if(POLE_Data_Is_Valid())
        {
            *paimod->pbAiValidDbWork=TRUE;
        }
        else
        {

            *paimod->pbAiValidDbWork=FALSE;
        }
        paimod->ulHeadClk=ulSmplClk;
        paimod->ulNextCnt++;
    }

    *ppxWr=paimod->pxWork;
    return paimod->pfWork;

}



/* 报告同杆并架虚拟机箱AI引擎完成一次数据刷新 要求此时rdinfo_g.ulCurrAiCnt还没有被本机的采样来刷新
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulAiCnt,该数据刷新对应的本机AI采样时刻
 * 返回值： 无 */
void  POLE_End_Ai_Wr(void *pvAiMod)
{
    uint32_t ulAiCnt;
    int iNewPts;

    if(!iPoleDiNum_g)   /*2007-10-29 DQ: 当同杆未配di时，不进行di刷新*/
        return ;
    assert(pvAiMod==(void*)&aimodPole_g);
    ulAiCnt=rdinfo_g.ulCurrAiCnt;
    iNewPts=aimodPole_g.ulNextCnt-1-ulAiCnt;/*要求此时rdinfo_g.ulCurrAiCnt还没有被本机的采样来刷新  */

    if (iNewPts)
    {
        while (iNewPts--)
        {
            POLE_Refresh_DI(pvAiMod);
        }
    }
    return   ;

}


/* 刷新同杆并架虚拟机箱的DI数据
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 * 返回值： 无 */
void POLE_Refresh_DI(void *pvAiMod)
{
    /*快速改成单点扫描  2007-4-12日*/
    BOOL  bCurAiIsFast=FALSE;
    BOOL  bCurAiIsMid=FALSE;
    BOOL  bCurAiIsSlow=FALSE;
    static   uint32_t   ulRefreshCnt=0;

    /*2007-4-13日 张云 ，目前该版本实现只支持如下设置  */
    assert(DI_SLOW_REFRESH_INTERVAL==12);
    assert(DI_MID_REFRESH_INTERVAL==6);
    assert(DI_FAST_REFRESH_INTERVAL==2);

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
        BOOL  *pBaseWork;
        BOOL  *pbFirst;
        BOOL *pbSecond;
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
        int  i;
        int iNow;

        if(bCurAiIsSlow)
        {
            /*若是慢速，则获得相应的多次存储的基址  */
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
        else  if(bCurAiIsMid)
        {
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
            pBaseWork=didb_g.pbWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFirst=pBaseWork;

            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSecond=pBaseWork;
        }

        for (plgcdi=plgcdich_g,i=0; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++,i++)
        {
            /*对每个ＤＩ通道判定，是否需要刷新  */

            if(plgcdi->ucDIRefreshRate==DI_FAST_REFRESH_RATE
                    &&bCurAiIsFast)
            {
                /*若是快速通道，且此次是快速刷新节拍，则快速刷新３点  */
                if ((plgcdi->iForceSts==-1)
                        &&(plgcdi->mod==RD_SAME_POLE_DI&&pvAiMod==&aimodPole_g))
                {
                    /*若是相应同杆并架机箱的DI，且不是强制，则更新缓冲，若是强制，则在RD_Refresh_DI中进行了更新  */
                    iNow=POLE_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    /*快速改成两点扫描
                    *(pbThird+i)=iNow;
                    */
                    POLE_Modify_DI(plgcdi,iNow);
                }

            }
            else  if(bCurAiIsMid
                     &&plgcdi->ucDIRefreshRate==DI_MID_REFRESH_RATE)
            {
                /*若是中速通道，且此次是中速刷新节拍，则中速刷新６点  */

                if ((plgcdi->iForceSts==-1)
                        &&(plgcdi->mod==RD_SAME_POLE_DI&&pvAiMod==&aimodPole_g))
                {
                    iNow=POLE_Get_DI(plgcdi->pvSrc);
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    POLE_Modify_DI(plgcdi,iNow);
                }
            }
            else if(bCurAiIsSlow
                    &&plgcdi->ucDIRefreshRate==DI_SLOW_REFRESH_RATE)
            {
                /*若是慢速通道，且此次是慢速刷新节拍，则慢速刷新１２点，若是其他，则对该通道空操作  */
                if ((plgcdi->iForceSts==-1)
                        &&(plgcdi->mod==RD_SAME_POLE_DI&&pvAiMod==&aimodPole_g))
                {
                    iNow=POLE_Get_DI(plgcdi->pvSrc);
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
                    POLE_Modify_DI(plgcdi,iNow);
                }
            }/*else if(bCurAiIsSlow
        &&plgcdi->ucDIRefreshRate==DI_SLOW_REFRESH_RATE)结束  */
        }/*for循环结束  */

    }/*if结束 */

    return;
}



/* 写入同杆并架虚拟机箱DI的开入板状态，处理变位等信息
 * 参数：   plgcdi，  DI句柄
 *          iNowVal,  此时DI值
 * 返回值： 无 */
/* */
static  void POLE_Modify_DI(RD_LGC_DI_CH *plgcdi,int  iNowVal)
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
        if ((plgcdi->iVal & 0x7FFF)!=(iNowVal & 0x7FFF))
        {
            /* Report SOE. */
            if ((plgcdi->iVal | iNowVal) & 0x8000)
            {
                /*2013-5-23  ZY */
                TM_High_Get_Sys_Us_UTC_Time(&plgcdi->utChgTime,&plgcdi->ulChgTime);
                if (plgcdi->iMeaCh != -1)
                {
                    VI_New_SOE(plgcdi->iMeaCh, (iNowVal & 0x7FFF), plgcdi->ulChgTime, plgcdi->bSOE, 0);
                }
            }
            else
            {
                plgcdi->ulChgTime=RD_AI_Cnt_To_us(aimodPole_g.ulNextCnt-1);
                if (plgcdi->iMeaCh!=-1)
                {
                    VI_New_SOE(plgcdi->iMeaCh, iNowVal, plgcdi->ulChgTime, plgcdi->bSOE, 0);
                }
            }
        }
        plgcdi->iVal=iNowVal;
    }
}




/*刷新新的同杆并架数据,必须在本机当地的数据刷新RD_END_AI_WR函数完成之前被调用
  参数：ulSmplClk，本机的递增采样时钟，
  返回   无*/
void  POLE_Read_AI_Data(uint32_t ulSmplClk)
{
    /*注意，没有复数式数据,要求每点数据都更新  */
    int  i;
    static uint32_t ulReadCnt=0;
    float *hdx_float;
#ifndef NO_DBL_BUF
    float *d_buf_float;
#endif
    COMPLEX * temp_p_complex;
    float  fTempVal;
    void  *  pvMod;

    if(!bPoleBoxIsInit_g)
    {
        /*若同杆并架没有被初始化好，则空操作  */
        return;
    }

    pvMod=&aimodPole_g;
    hdx_float = (float*)POLE_AI_Dat_P(pvMod,ulSmplClk, (COMPLEX**)&temp_p_complex);

    ++ulReadCnt;
#ifndef NO_DBL_BUF 					/*若是双缓冲  */

    d_buf_float=(float*)((uint8_t*)hdx_float-lgcaidb_g.ulBufBytes);
    for(i=0; i<PoleBoxAICfg_g.iPoleAINum; i++)
    {
        if(ulReadCnt&0x01)    /*2007-10-30 DQ: 改为每两次进行一次POLE_Get_AI，另一次则读上次的缓存值*/
        {
            fTempVal=POLE_Get_AI(PoleBoxAICfg_g.apPoleBoxAIIdx[i]);
            *hdx_float++=fTempVal;
            *d_buf_float++=fTempVal;
            PoleBoxAICfg_g.apPoleBoxAIIdx[i]->fBufVal=fTempVal;
        }
        else
        {
            fTempVal=PoleBoxAICfg_g.apPoleBoxAIIdx[i]->fBufVal;
            *hdx_float++=fTempVal;
            *d_buf_float++=fTempVal;
        }
    }

#else                                                  /*若是单缓冲  */
    for(i=0; i<PoleBoxAICfg_g.iPoleAINum; i++)
    {
        if(ulReadCnt&0x01)
        {
            fTempVal=POLE_Get_AI(PoleBoxAICfg_g.apPoleBoxAIIdx[i]);
            *hdx_float++=fTempVal;
            PoleBoxAICfg_g.apPoleBoxAIIdx[i]->fBufVal=fTempVal;
        }
        else
        {
            fTempVal=PoleBoxAICfg_g.apPoleBoxAIIdx[i]->fBufVal;
            *hdx_float++=fTempVal;
        }
    }
#endif
    POLE_End_Ai_Wr(pvMod);
}



/*同杆并架机箱数据是否有效
  参数：无
  返回值：TRUE：数据有效
          FALSE：数据无效
  注意：只有当所有组SUB数据都有效时，才数据有效
        而对某组SUB数据而言，只要该组SUB的任何一个网络的数据有效，则该组SUB数据有效*/
BOOL   POLE_Data_Is_Valid()
{

    int   k;
    int   j;
    BOOL   bDataIsValid;
    BOOL   bCurSubDataIsValid;
    uint8_t   aucAllNetSubStat[MAX_GSE_NET_CNT];
    uint8_t   aucAllNetSubStatOrigin[MAX_GSE_NET_CNT];
    static uint32_t ulSubErrorStat[MAX_ALLOW_SUB_GO_NUM]= {0};
    static uint32_t usErrorStat[MAX_ALLOW_SUB_GO_NUM][MAX_GSE_NET_CNT];                /* Error state. */

    int  iValidNetCnt;
    static   BOOL   bIsFstEnter_s=TRUE;
    static   BOOL   bPoleNowUsedByApp_s=FALSE;
    static   uint32_t   ulPoleSubErrCnt_s0[MAX_GSE_NET_CNT];
    static   uint32_t   ulPoleSubErrCnt_s1[MAX_GSE_NET_CNT];
    static   uint32_t   ulPoleSubErrCnt_s2[MAX_GSE_NET_CNT];
    static   uint32_t   ulPoleSubErrCnt_s3[MAX_GSE_NET_CNT];
    static   uint32_t   ulPoleSubErrCnt_s4[MAX_GSE_NET_CNT];
    static   uint32_t   ulPoleSubErrCnt_s5[MAX_GSE_NET_CNT];
    static   uint32_t   ulPoleSubErrCnt_s6[MAX_GSE_NET_CNT];
    static   uint32_t   ulPoleSubErrCnt_s7[MAX_GSE_NET_CNT];
    uint16_t unErrCode=0;
    uint8_t aucPrompt_g[200];

    if(bIsFstEnter_s)
    {
        /*若是首次进入，则初始化  */
        bIsFstEnter_s=FALSE;
        for(k=0; k<MAX_GSE_NET_CNT; k++)
        {
            ulPoleSubErrCnt_s0[k]=0;
            ulPoleSubErrCnt_s1[k]=0;
            ulPoleSubErrCnt_s2[k]=0;
            ulPoleSubErrCnt_s3[k]=0;
            ulPoleSubErrCnt_s4[k]=0;
            ulPoleSubErrCnt_s5[k]=0;
            ulPoleSubErrCnt_s6[k]=0;
            ulPoleSubErrCnt_s7[k]=0;
        }
    }

    if(!bPoleBoxIsInit_g)
    {
        /*若同杆并架没有被初始化好，则数据无效  */
        return   FALSE;
    }

    if(iPoleSubGoNum_g==0)/*若sub index 无效，则数据无效  */
    {
        return   FALSE;
    }

    if(bPoleIsUsedInApp_g&&(!bPoleNowUsedByApp_s))
    {
        memset(usErrorStat,0,sizeof(usErrorStat));
        memset(ulSubErrorStat,0,sizeof(ulSubErrorStat));
    }

    if(!bPoleIsUsedInApp_g)
    {
        bPoleNowUsedByApp_s=FALSE;
    }
    else
    {
        bPoleNowUsedByApp_s=TRUE;
    }

    bDataIsValid=TRUE;
    for(k=0; k<iPoleSubGoNum_g; k++)
    {
        /*对每组sub GOOSE进行数据有效状态查询  */
        iValidNetCnt=0;
        if(!ReadSubStat(aiPoleSubGoIdx_g[k],&iValidNetCnt,aucAllNetSubStat, aucAllNetSubStatOrigin))/*若读取状态失败，则数据无效  */
        {
            /*2007-6-28日 张云 发现的BUG  */
            assert(iValidNetCnt>0&&iValidNetCnt<=MAX_GSE_NET_CNT);
            if(!(ulSubErrorStat[k]&0x00000001))     /* 仅仅发一次 */
            {
                /*2007-6-28日 张云   */
                ulSubErrorStat[k] |= 0x00000001;
                assert(sGcRef_g=QuerySubGcRefByIdx(aiPoleSubGoIdx_g[k]));

                /* 2007-8-11日 张云， 为了满足外城变，对上送goose异常时，后台获得SUB号和NET号，
                     和MMI约定，必须在错误字符串中用引号包括两个参数，依次是SUB 号，net 号，MMI需要根据该串获得这两个信息
                     但MMI要能处理当未包含该两个参数的情况，此时默认处理为SUB 0，NET 0
                     对同杆并架处理和智能操作箱并不一致，同杆并架，当通信中断时，并不需要闭锁，由保护程序处理数据无效情况。*/
                if(bPoleIsUsedInApp_g)
                {
                    if(ENG_MODE==1)
                    {
                        sprintf(aucPrompt_g, "PTL(Parallel Transmission Lines) box sub goose(%s) read sub stat failure，Data err, sub go index is \'%d\', net num is \'%d\'.\n",
                                sGcRef_g, aiPoleSubGoIdx_g[k],0);
                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);

                        sprintf(aucPrompt_g, "PTL(Parallel Transmission Lines) box sub goose(%s) read sub stat failure，Data err, sub go index is \'%d\', net num is \'%d\'.\n",
                                sGcRef_g, aiPoleSubGoIdx_g[k],1);
                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                    }
                    else if(ENG_MODE==0)
                    {
                        sprintf(aucPrompt_g, "同杆并架机箱sub goose(%s)读取sub状态失败，数据无效, sub序号是 \'%d\', 网络号是 \'%d\'.\n",
                                sGcRef_g, aiPoleSubGoIdx_g[k], 0);
                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);

                        sprintf(aucPrompt_g, "同杆并架机箱sub goose(%s)读取sub状态失败,数据无效,sub序号是 \'%d\',网络号是 \'%d\'.\n",
                                sGcRef_g, aiPoleSubGoIdx_g[k], 1);
                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                    }
                }
            }
            bDataIsValid=FALSE;
        }
        else
        {
            if(ulSubErrorStat[k]&0x00000001)
            {
                ulSubErrorStat[k] &= ~0x00000001;
                if(bPoleIsUsedInApp_g)
                {
                    if(ENG_MODE==1)
                    {
                        sprintf(aucPrompt_g, "PTL(Parallel Transmission Lines) box sub goose(%s) read sub stat  get back，Data valid,sub go index is \'%d\',net num is \'%d\'  !\n", sGcRef_g, aiPoleSubGoIdx_g[k],0);
                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                        sprintf(aucPrompt_g, "PTL(Parallel Transmission Lines) box sub goose(%s) read sub stat  get back，Data valid,sub go index is \'%d\',net num is \'%d\'  !\n", sGcRef_g, aiPoleSubGoIdx_g[k],1);
                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                    }
                    else if(ENG_MODE==0)
                    {
                        sprintf(aucPrompt_g, "同杆并架机箱sub goose(%s) 读取sub状态恢复，数据有效,sub序号是 \'%d\',网络号是 \'%d\'  !\n", sGcRef_g, aiPoleSubGoIdx_g[k],0);
                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                        sprintf(aucPrompt_g, "同杆并架机箱sub goose(%s) 读取sub状态恢复，数据有效,sub序号是 \'%d\',网络号是 \'%d\'  !\n", sGcRef_g, aiPoleSubGoIdx_g[k],1);
                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                    }
                }
            }
            assert(iValidNetCnt>0&&iValidNetCnt<=MAX_GSE_NET_CNT);

            bCurSubDataIsValid=FALSE;
            for(j=0; j<iValidNetCnt; j++)
            {
                switch(aucAllNetSubStat[j])
                {
                    case  SUB_IED_OK:
                    {
                        bCurSubDataIsValid=TRUE;  /*任何一个网络数据有效，则该组SUB数据有效  */
                    }
                    break;
                    case  SUB_IED_YABAN_EXIT:
                    {
                        bCurSubDataIsValid=TRUE;  /*若压板退出，，则还认为该组SUB数据有效，只是默认值  */
                    }
                    break;
                }
            }

            if(!bCurSubDataIsValid)
            {
                /*若该组SUB数据无效，则总的数据无效  */
                bDataIsValid=FALSE;
            }

            /*输出每组SUB，每个网口异常信息  */
            for(j=0; j<iValidNetCnt; j++)
            {
                switch(aucAllNetSubStat[j])
                {
                    case   SUB_IED_OK:
                    {
                        ulPoleSubErrCnt_s0[j]++;
                        if((ulPoleSubErrCnt_s0[j]&0x3fffffff)==0xf)
                        {
                            LOG_Dbg_Msg("same pole sub goose ok, sub go index is %d,net num is %d!\n"
                                        , aiPoleSubGoIdx_g[k],j,0,0,0,0);
                        }
                        if(usErrorStat[k][j])
                        {
                            if(usErrorStat[k][j]&0x00000001)
                            {
                                usErrorStat[k][j] &= ~0x00000001;
                                assert(sGcRef_g=QuerySubGcRefByIdx(aiPoleSubGoIdx_g[k]));
                                unErrCode = (j==0 ? EV_REL_GSE_A_NET_HALT : EV_REL_GSE_B_NET_HALT);
                                if(bPoleIsUsedInApp_g)
                                {
                                    sprintf(aucPrompt_g, "同杆装置SUB goose(%s)通信恢复,数据有效,SUB序号是\'%d\',网络号\'%d\'.\n",
                                            sGcRef_g, aiPoleSubGoIdx_g[k], j);
                                    LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                                }
                            }

                            if(usErrorStat[k][j]&0x00000008)
                            {
                                usErrorStat[k][j] &= ~0x00000008;
                                assert(sGcRef_g=QuerySubGcRefByIdx(aiPoleSubGoIdx_g[k]));
                                unErrCode = (j==0 ? EV_REL_GSE_A_NET_HALT : EV_REL_GSE_B_NET_HALT);

                                if(bPoleIsUsedInApp_g)
                                {
                                    sprintf(aucPrompt_g, "同杆装置SUB goose(%s)接收恢复,数据有效,SUB序号是\'%d\',网络号\'%d\'.\n",
                                            sGcRef_g, aiPoleSubGoIdx_g[k], j);
                                    LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                                }
                            }

                            if(usErrorStat[k][j]&0x00000020)
                            {
                                usErrorStat[k][j] &= ~0x00000020;
                                assert(sGcRef_g=QuerySubGcRefByIdx(aiPoleSubGoIdx_g[k]));

                                if(bPoleIsUsedInApp_g)
                                {
                                    if(ENG_MODE==0)
                                    {
                                        sprintf(aucPrompt_g, "同杆并架sub goose(%s) 检修状态数据恢复，数据有效, sub序号是 \'%d\',网络号\'%d\'!\n", sGcRef_g,aiPoleSubGoIdx_g[k],j);
                                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                                    }
                                    else
                                    {
                                        sprintf(aucPrompt_g, "PTL(Parallel Transmission Lines) sub goose(%s) repair data get back，data valid,sub go index is \'%d\',net num is \'%d\'!\n", sGcRef_g,aiPoleSubGoIdx_g[k],j);
                                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                                    }
                                }
                            }

                            if(usErrorStat[k][j]&0x00000040)
                            {
                                usErrorStat[k][j] &= ~0x00000040;
                                assert(sGcRef_g=QuerySubGcRefByIdx(aiPoleSubGoIdx_g[k]));

                                if(bPoleIsUsedInApp_g)
                                {
                                    if(ENG_MODE==0)
                                    {
                                        sprintf(aucPrompt_g, "同杆并架sub goose(%s) ConfRev版本错误数据恢复，数据有效, sub序号是 \'%d\',网络号\'%d\'!\n", sGcRef_g,aiPoleSubGoIdx_g[k],j);
                                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                                    }
                                    else
                                    {
                                        sprintf(aucPrompt_g, "PTL(Parallel Transmission Lines) sub goose(%s) err ConfRev data get back，data valid,sub go index is \'%d\',net num is \'%d\'!\n", sGcRef_g,aiPoleSubGoIdx_g[k],j);
                                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);;
                                    }
                                }
                            }

                            if(usErrorStat[k][j]&0x00000100)
                            {
                                usErrorStat[k][j] &= ~0x00000100;
                                assert(sGcRef_g=QuerySubGcRefByIdx(aiPoleSubGoIdx_g[k]));
                                unErrCode = (j==0 ? EV_REL_GSE_A_NET_HALT : EV_REL_GSE_B_NET_HALT);

                                if(bPoleIsUsedInApp_g)
                                {
                                    sprintf(aucPrompt_g, "同杆装置SUB goose(%s)未知状态数据恢复,数据有效,SUB序号是\'%d\',网络号\'%d\'.\n",
                                            sGcRef_g, aiPoleSubGoIdx_g[k], j);
                                    LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                                }
                            }
                        }
                    }
                    break;
                    case   SUB_IED_COM_ERR:
                    case   SUB_IED_OVERTIME :/*就是通信中断 2007-10-4日 */
                    {
                        ulPoleSubErrCnt_s1[j]++;
                        {
                            if(bCurSubDataIsValid)
                            {
                            }
                            else
                            {
                                /*若该组SUB没有任何一个网络数据有效，且有网络通信中断，则我们需要呼唤  */

                                if((!(usErrorStat[k][j]&0x00000001)))
                                {
                                    assert(sGcRef_g=QuerySubGcRefByIdx(aiPoleSubGoIdx_g[k]));
                                    usErrorStat[k][j] |= 0x00000001;
                                    unErrCode = (j==0 ? EV_REL_GSE_A_NET_HALT : EV_REL_GSE_B_NET_HALT);

                                    if(bPoleIsUsedInApp_g)
                                    {
                                        sprintf(aucPrompt_g, "同杆装置SUB goose(%s)通信错误,所有网络数据无效,SUB序号是\'%d\',网络号\'%d\'.\n",
                                                sGcRef_g, aiPoleSubGoIdx_g[k], j);
                                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                                    }
                                }
                            }
                        }
                    }
                    break;
                    case   SUB_IED_FAULT:
                    {
                        ulPoleSubErrCnt_s3[j]++;
                        {
                            if(bCurSubDataIsValid)
                            {

                            }
                            else
                            {
                                /*若该组SUB没有任何一个网络数据有效，且有SUB 异常，则我们需要呼唤  */
                                if((!(usErrorStat[k][j]&0x00000008)))
                                {
                                    usErrorStat[k][j] |= 0x00000008;
                                    assert(sGcRef_g=QuerySubGcRefByIdx(aiPoleSubGoIdx_g[k]));
                                    unErrCode = (j==0 ? EV_REL_GSE_A_NET_HALT : EV_REL_GSE_B_NET_HALT);

                                    if(bPoleIsUsedInApp_g)
                                    {
                                        sprintf(aucPrompt_g, "同杆装置SUB goose(%s)接收错误,数据无效,SUB序号是\'%d\',网络号\'%d\'.\n",
                                                sGcRef_g, aiPoleSubGoIdx_g[k], j);
                                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                                    }
                                }
                            }
                        }
                    }
                    break;
                    case   SUB_IED_IN_REPAIR:
                    {
                        /*若是检修数据，我们只是报事件，但是不呼唤 */
                        ulPoleSubErrCnt_s4[j]++;
                        if(!(usErrorStat[k][j]&0x00000020))
                        {
                            usErrorStat[k][j] |= 0x00000020;
                            assert(sGcRef_g=QuerySubGcRefByIdx(aiPoleSubGoIdx_g[k]));
                            if(bPoleIsUsedInApp_g)
                            {
                                sprintf(aucPrompt_g, "同杆装置SUB goose(%s)接收到检修状态数据,数据无效,SUB序号是\'%d\',网络号\'%d\'.\n",
                                        sGcRef_g, aiPoleSubGoIdx_g[k], j);
                                LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                            }
                        }
                    }
                    break;
                    case   SUB_IED_CONFREV_ERR:
                    {
                        ulPoleSubErrCnt_s5[j]++;
                        {

                            if(bCurSubDataIsValid)
                            {

                            }
                            else
                            {
                                /*若该组SUB没有任何一个网络数据有效，且有ConfRev 不匹配，对同杆并架应用，则我们需要呼唤，但不告警闭锁，因为同杆应用不闭锁此种情况  */

                                if((!(usErrorStat[k][j]&0x00000040)))
                                {
                                    usErrorStat[k][j] |= 0x00000040;
                                    assert(sGcRef_g=QuerySubGcRefByIdx(aiPoleSubGoIdx_g[k]));

                                    if(bPoleIsUsedInApp_g)
                                    {
                                        if(ENG_MODE==0)
                                        {
                                            ER_Set_Err_Stat(EV_GSE_CONFI_ERR, ER_ALARM | ER_LOCK | ER_REPORT | ER_NOLOGWRITE,
                                                            "同杆装置GOOSE异常\n", 0, 0, 1, 0);
                                        }
                                        else
                                        {
                                            ER_Set_Err_Stat(EV_GSE_CONFI_ERR, ER_ALARM | ER_LOCK | ER_REPORT | ER_NOLOGWRITE,
                                                            "Parallel device GOOSE error\n", 0, 0, 1, 0);
                                        }
                                        sprintf(aucPrompt_g, "同杆装置 SUB goose(%s) 接收到ConfRev版本错误数据,所有网络数据无效,SUB序号是\'%d\',网络号\'%d\'!!\n",
                                                sGcRef_g, aiPoleSubGoIdx_g[k], j);
                                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);

                                    }
                                }

                            }
                        }
                    }
                    break;
                    case   SUB_IED_YABAN_EXIT:
                    {
                        ulPoleSubErrCnt_s6[j]++;
                        if((ulPoleSubErrCnt_s6[j]&0x3fffff)==0xf)/*该现象，要求快速报出来,2007-7-10 */
                        {
                            static  uint8_t aucLogInfo[256];/*2007-8-30日 张云  */
                            if(bPoleIsUsedInApp_g)
                            {
                                if(ENG_MODE == 1)
                                {
                                    sprintf(aucLogInfo,"same  pole box sub goose recv  switch exit，data invalid,sub  index is \'%d\',net num is \'%d\'!\n",
                                            aiPoleSubGoIdx_g[k],j);
                                }
                                else if(ENG_MODE == 0)
                                {
                                    sprintf(aucLogInfo,"同杆并架sub goose 接收压板退出，网络数据无效, sub序号是 \'%d\',网络号\'%d\'!\n",
                                            aiPoleSubGoIdx_g[k],j);
                                }
                                LOG_Write(LOG_KERNEL,aucLogInfo,NULL );
                            }
                        }
                    }
                    break;
                    default:
                    {
                        ulPoleSubErrCnt_s7[j]++;
                        if((ulPoleSubErrCnt_s7[j]&0x3ffff)==0x3fffE)
                        {

                            if(bCurSubDataIsValid)
                            {
                            }
                            else
                            {
                                /*若该组SUB没有任何一个网络数据有效，且有其他异常，则我们需要告警  */
                                if((!(usErrorStat[k][j]&0x00000100)))
                                {
                                    usErrorStat[k][j] |= 0x00000100;
                                    assert(sGcRef_g=QuerySubGcRefByIdx(aiPoleSubGoIdx_g[k]));
                                    unErrCode = (j==0 ? EV_REL_GSE_A_NET_HALT : EV_REL_GSE_B_NET_HALT);

                                    if(bPoleIsUsedInApp_g)
                                    {
                                        sprintf(aucPrompt_g, "同杆装置SUB goose(%s)数据状态未知,所有网络数据无效,SUB序号是\'%d\',网络号\'%d\'.\n",
                                                sGcRef_g, aiPoleSubGoIdx_g[k], j);
                                        LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);
                                    }
                                }
                            }
                        }
                    }
                    break;

                }/* switch结束 */
            }/* for(j=0;j<iValidNetCnt;j++)结束*/

        }/*else */
    }/*for结束  */

    return   bDataIsValid;
}



/*查询同杆并架数据发布时，数据是否发生变化
  参数  无
  返回,TRUE,数据发生变化
       FALSE，数据未变化  2007-6-25日 张云 */
BOOL  POLE_DataPubIsChgAndSave()
{

    int  i;
    BOOL  bPoleDataIsChg=FALSE;
    float  fAoCurVal;
    float  fAoLstPubVal;
    static   BOOL  bPoleIsFstPub=TRUE;

    if(bPoleIsFstPub)
    {
        /*若是首次发布  */
        bPoleIsFstPub=FALSE;
        bPoleDataIsChg=TRUE;
    }

    for(i=0; i<PoleBoxIOInfo_g.iPoleDoNum; i++)
    {
        /*查询PUB时，DO数据是否发生变化  */
        if(PoleBoxIOInfo_g.aphPoleDoIdx[i]->bCurVal
                !=PoleBoxIOInfo_g.aphPoleDoIdx[i]->bLstPubVal)
        {
            bPoleDataIsChg=TRUE;
        }
        PoleBoxIOInfo_g.aphPoleDoIdx[i]->bLstPubVal
            =PoleBoxIOInfo_g.aphPoleDoIdx[i]->bCurVal;/*保存此次PUB值  */
    }

    for(i=0; i<PoleBoxAoCfg_g.iPoleAONum; i++)
    {
        /*查询PUB时，AO数据是否发生变化  */
        assert(PoleBoxAoCfg_g.apPoleBoxAoIdx[i]->pElemSrc);
        fAoCurVal=((EP_ELEM_IO *)PoleBoxAoCfg_g.apPoleBoxAoIdx[i]->pElemSrc)->now.fVal;
        fAoLstPubVal=PoleBoxAoCfg_g.apPoleBoxAoIdx[i]->fLstPubVal;
        if(fabs(fAoCurVal-fAoLstPubVal)>POLE_AO_DATA_PUB_CHG_TH)/*若超过允许的门槛  */
        {
            bPoleDataIsChg=TRUE;
        }
        PoleBoxAoCfg_g.apPoleBoxAoIdx[i]->fLstPubVal=
            ((EP_ELEM_IO *)PoleBoxAoCfg_g.apPoleBoxAoIdx[i]->pElemSrc)->now.fVal;
    }

    return  bPoleDataIsChg;
}

/*2008-3-07 DQ
  设置同杆并架模块在保护应用中是否使用,
  平台根据该函数设置状态确定是否提示同杆通信状态,平台程序默认该状态为不使用;
  参数  bIsUsed: TRUE
  返回,TRUE,数据发生变化
       FALSE，数据未变化  2007-6-25日 张云 */
void  POLE_SetUsedState(BOOL bIsUsed)
{
    if(bIsUsed!=bPoleIsUsedInApp_g)
    {
        if(ENG_MODE == 1)
        {
            (bIsUsed) ? LOG_Write(LOG_KERNEL, "POLE Box is Enabled By App.\n", NULL) : \
            LOG_Write(LOG_KERNEL, "POLE Box is Disabled By App.\n", NULL);
        }
        else if(ENG_MODE == 0)
        {
            (bIsUsed) ? LOG_Write(LOG_KERNEL, "同杆并架被应用使能 .\n", NULL) : \
            LOG_Write(LOG_KERNEL, "同杆并架被应用禁止 .\n", NULL) ;
        }
    }
    bPoleIsUsedInApp_g=bIsUsed;
    return ;
}

