/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*       HDL_VtBox.c                                    1.0                      */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了智能操作箱初始化模块的代码文件                                     */
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
#include  "HDL_Data.h"
#include  "HDL_VtBox.h"
#include  "logic.h"
#include  "errtest.h"
#include "GO_Interface.h"
#include "EdpVer.h"

int iHdlDiNum_g=0;
int iHdlDoNum_g=0;
int iHDLAiNum_g=0;
int iHDLAoNum_g=0;

HDL_BOX_AO_CFG    HdlBoxAoCfg_g;  /*智能操作机箱AO配置  */
HDL_BOX_IO_INFO   HdlBoxIOInfo_g; /*智能操作机箱IO配置  */
HDL_BOX_AI_CFG    HdlBoxAICfg_g;  /*智能操作机箱AI配置  */

BOOL   bHdlBoxIsInit_g=FALSE;  /*智能操作机箱被初始化标志  */

int   iHdlSubGoNum_g=0;   /*智能操作箱对应的sub GOOSE 个数 2007-7-3   (有效的goose个数)*/
int iHdlNetNum_g = 0;  /* 最多网口数量 */

int   iHdlCfgSubGoNum_g=0;	/*所有goose配置文件配置的sub个数，用于goose告警*/

/* globals */

/* 智能操作箱对应的SUB GOOSE index,index的实际值没有界限 */
int aiHdlSubGoIdx_g[MAX_ALLOW_SUB_GO_NUM];

/* GOOSE接收状态 */
int iHdlSubGoStat[MAX_ALLOW_SUB_GO_NUM] = {0};

/* GOOSE SUB压板状态 */
BOOL bSubGoYabanStat[MAX_ALLOW_SUB_GO_NUM] = {TRUE};

/* GOOSE SUB所对应压板序号 */
int16_t iSubGoYabanSn[MAX_ALLOW_SUB_GO_NUM];

extern BOOL   bInit61850BfRelayIsSuccess_g;

BOOL bAllSnowFlag = TRUE;  /* 全部为慢速处理DI */

/* 初始化（并启动）智能操作箱,目前只支持DI，DO
 * 参数：   uiSmplRate，采样速率
 *          uiSysFreq，系统频率
 *          pvAiMod，该模块（机箱负责的所有AI采集/计算通道）的句柄
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，机箱出错 */
EP_STATUS Init_Hdl_Box(u_int uiSmplRate, u_int uiSysFreq,
                       void *pvAiMod, u_int uiLgcCh, DSP_LGC_AI_CFG *plgccfg,
                       u_int uiCalcCfg, DSP_CALC_AI_CFG *pcalccfg)
{
    int i;
    DSP_LGC_AI_CFG * pCurLgcCfg;
    HDL_AI_HND  *   pAiHdl;
    RD_HW_AI_CH * pch;
    BOOL   bQuerySuccess;

    assert(pvAiMod);
    assert(uiLgcCh<=MAX_HDL_AI_NUM);
    assert(plgccfg);
    assert(uiCalcCfg==0);

    // if(!bInit61850BfRelayIsSuccess_g)
    // {
    //     LOG_Dbg_Msg("goose配置错误，若goose初始化没有成功，不能初始化智能操作箱！\n",0,0,0,0,0,0);
    //     if(ENG_MODE == 1)
    //     {
    //         ER_Set_Err(EV_GSE_CONFI_ERR,ER_REPORT|ER_ALARM|ER_LOCK
    //                    ,"if goose init failure，can not init  handl box\n",0,0);

    //     }
    //     else if(ENG_MODE == 0)
    //     {
    //         ER_Set_Err(EV_GSE_CONFI_ERR,ER_REPORT|ER_ALARM|ER_LOCK
    //                    ,"若goose初始化没有成功，不能初始化智能操作箱\n",0,0);
    //     }
    // }

    HdlBoxAICfg_g.iHdlAINum=0;

    pCurLgcCfg=plgccfg;
    for(i=0; i<uiLgcCh; i++)
    {
        assert(pCurLgcCfg->ucFiltNum==0);/*只允许配置原始滤波算法  */

        pAiHdl=&(HdlBoxAICfg_g.aHdlBoxAiHdl[i]);
        pAiHdl->ucHdCh=pCurLgcCfg->ucHdCh;
        pAiHdl->fCoff=pCurLgcCfg->fCoff;
        pAiHdl->ucFiltNum=pCurLgcCfg->ucFiltNum;

        /* 初始化 */
        pAiHdl->iSubDaIdx = -1;
        pAiHdl->pSubMapData = NULL;

        bQuerySuccess=GO_QueryActiveGoDaIdxByAiNum/*查询sub goose  */
                      (HDL_BOX_GO_SRC_TYPE,
                       HdlBoxAICfg_g.iHdlAINum+1,
                       &(pAiHdl->iSubDaIdx),
                       &(pAiHdl->pSubMapData));
#if 0
        if(!bQuerySuccess)
        {
            /*若查询获得sub goose中的DA INDEX失败*/
            if(ENG_MODE == 1)
                ER_Set_Err(EV_GSE_CONFI_ERR,ER_REPORT|ER_ALARM
                           ,"goose config err，query hdl vt box AI in sub goose da  failure,AI num is %d !\n"
                           ,HdlBoxAICfg_g.iHdlAINum+1,0);
            else if(ENG_MODE == 0)
                ER_Set_Err(EV_GSE_CONFI_ERR,ER_REPORT|ER_ALARM
                           ,"goose 配置错误:在智能操作箱的sub goose中查询AI失败,AI num 是 %d  !\n"
                           ,HdlBoxAICfg_g.iHdlAINum+1,0);
        }
#endif
        HdlBoxAICfg_g.apHdlBoxAIIdx[i]=pAiHdl;

        for (pch=phwaich_g; pch<phwaich_g+iHwAiChNum_g; pch++)
        {
            /* 仅处理来源于智能操作箱的模拟量 */
            if((pch->ucModCh == (pAiHdl->ucHdCh - 1))
                    && (pch->paimod == &aimodHdl_g))
            {
                pch->pAiHdl = pAiHdl;
                break;
            }
        }

        HdlBoxAICfg_g.iHdlAINum++;
        iHDLAiNum_g++;

        pCurLgcCfg++;
    }

    return   EP_SUCCESS;

}


/*通知智能操作箱初始化完成,在逻辑图初始化完成之后，逻辑图运行之前调用
  参数：无
  返回：成功与否
*/
EP_STATUS   HDL_AOCfgInitFinish()
{


    if(iHdlDiNum_g!=0
            ||iHdlDoNum_g!=0
            ||iHDLAiNum_g!=0
            ||iHDLAoNum_g!=0)
    {
        bHdlBoxIsInit_g=TRUE;/*2007-6-15日 ，只有设置，且没有任何错误，设置智能操作相初始化成功标志  */
    }

    return  EP_SUCCESS;
}

/* 初始化智能操作箱的IO
 * 参数：
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS  HDL_IO_Initialize()
{

    int  i;
    for(i=0; i<MAX_MOD_NUM; i++)
    {
        HdlBoxIOInfo_g.aHdlIOModInfo[i].type=IDLE_MODULE;
        HdlBoxIOInfo_g.aHdlIOModInfo[i].unDiChNum=0;
        HdlBoxIOInfo_g.aHdlIOModInfo[i].unDoChNum=0;
    }

    HdlBoxIOInfo_g.iHdlDiNum=0;
    HdlBoxIOInfo_g.iHdlDoNum=0;

    for (i = 0; i<MAX_ALLOW_SUB_GO_NUM; i++)
    {
        iHdlSubGoStat[i] = -1;/*-1代表初始状态*/
        bSubGoYabanStat[i] = TRUE;
    }

    return  EP_SUCCESS;
}


/* 初始化智能操作箱DI通道
 * Para:
 *     iModAddr, 模块硬件地址.
 *     uiCh, 在本模件内的DI物理通道号,从0开始.
 *     ulFilt, 去抖动时间,单位us.
 *     bInvalidDftVal, 通信无效时缺省值.
 *     ucDIRefreshRate, 调用速度(快/中/慢).
 *     bpPended, 是否悬空.
 * Return:
 *     用来索引DI通道的void指针,或者NULL表示调用出错.
 */
void *HDL_Init_DI(int iModAddr, u_int uiCh, uint32_t ulFilt, BOOL bInvalidDftVal, uint8_t ucDIRefreshRate, BOOL *bpPended)
{

    HDL_DI_HND  *pHdl;
    BOOL   bQuerySuccess;
    int i;

    assert(iModAddr<MAX_MOD_NUM);
    assert(uiCh<MAX_DI_PER_MOD);

    if(HdlBoxIOInfo_g.aHdlIOModInfo[iModAddr].type==DO_MODULE)
    {
        assert(FALSE);
    }

    if(HdlBoxIOInfo_g.aHdlIOModInfo[iModAddr].type==IDLE_MODULE)
    {
        HdlBoxIOInfo_g.aHdlIOModInfo[iModAddr].type=DI_MODULE;
    }
    HdlBoxIOInfo_g.aHdlIOModInfo[iModAddr].unDiChNum++;

    pHdl=&(HdlBoxIOInfo_g.ahHdlDiHandle[iModAddr*MAX_DI_PER_MOD+uiCh]);
    pHdl->ucMod=iModAddr;
    pHdl->ucHdCh=uiCh;
    pHdl->aucFilt[0]=HH8(ulFilt);
    pHdl->aucFilt[1]=HL8(ulFilt);
    pHdl->aucFilt[2]=LH8(ulFilt);
    pHdl->aucFilt[3]=LL8(ulFilt);

    /* 非悬空 */
    *bpPended = FALSE;

    /* 增加初始化内容
     * 分快/中/慢速按采样节拍折算得到消抖次数
     * DI状态初始化
     * 相关计数初始化
     */
    if (ucDIRefreshRate == DI_FAST_REFRESH_RATE)
    {

        pHdl->ulFltCfg = (ulFilt/(1000000L/uiAiRate_g))/DI_FAST_REFRESH_INTERVAL;


        bAllSnowFlag = FALSE;
    }
    else if (ucDIRefreshRate == DI_MID_REFRESH_RATE)
    {
        pHdl->ulFltCfg = (ulFilt/(1000000L/uiAiRate_g))/DI_MID_REFRESH_INTERVAL;
        bAllSnowFlag = FALSE;
    }
    else if (ucDIRefreshRate == DI_SLOW_REFRESH_RATE)
    {
        pHdl->ulFltCfg = (ulFilt/(1000000L/uiAiRate_g))/DI_SLOW_REFRESH_INTERVAL;
    }
    else
    {
        bAllSnowFlag = FALSE;
        return NULL;
    }
    pHdl->bSts = FALSE;
    pHdl->bLstSts = FALSE;
    pHdl->ulFltCnt = pHdl->ulFltCfg;
    pHdl->ulTmpNextCnt = 0;
    pHdl->bChgFlag = FALSE;
    pHdl->utChgTime.ullusCntFrom1970=0;
    pHdl->utChgTime.ucQflag=0x60;

    pHdl->bInvalidDftVal = bInvalidDftVal;  /* 缺省值类型 */
    pHdl->bValueBfInvalid = FALSE;  /* 缺省值 */
    for(i=0; i<HDL_DI_MAX_RECV_NUM; i++)
    {
        pHdl->iSubDaIdx[i]=-1;
        pHdl->ppSubMapData[i] = NULL;
        pHdl->iSubTStampDaIdx[i]=-1;
        pHdl->ppSubMapTStamp[i] = NULL;
        pHdl->vtValType[i] = UNKNOW_TYPE;
    }
    HdlBoxIOInfo_g.pahHdlDiHandle[HdlBoxIOInfo_g.iHdlDiNum] = pHdl; /* 句柄索引 */

    bQuerySuccess=GO_QueryActiveGoDaIdxByDiNum
                  (HDL_BOX_GO_SRC_TYPE,
                   HdlBoxIOInfo_g.iHdlDiNum+1,
                   pHdl->iSubDaIdx,
                   pHdl->iSubNum,
                   pHdl->ppSubMapData,
                   pHdl->iSubTStampDaIdx,
                   pHdl->ppSubMapTStamp,
                   pHdl->vtValType,
                   pHdl->ppiSubYabanIndex,
                   pHdl->iSubDaVtIdx);
    if(!bQuerySuccess)
    {
        /*若查询获得sub goose中的DA INDEX失败，  */

        /* 六统一要求, 过程层没有配置时不报错 */
#if 0
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_GSE_CONFI_ERR,ER_REPORT|ER_ALARM|ER_LOCK
                       ,"在sub goose da中查询智能操作相 DI 失败,DI编号是 %d\n",HdlBoxIOInfo_g.iHdlDiNum+1,0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_GSE_CONFI_ERR,ER_REPORT|ER_ALARM|ER_LOCK
                       ,"query  handle box DI in sub goose failed,DI num is %d\n",HdlBoxIOInfo_g.iHdlDiNum+1,0);
        }
        return  (void  *)pHdl;
#endif

        /* 作悬空处理 */
        for (i = 0; i<HDL_DI_MAX_RECV_NUM; i++)
        {
            pHdl->iSubDaIdx[i] = -2;
            pHdl->iSubTStampDaIdx[i] = -2;
        }
    }

    for(i=0; i<HDL_DI_MAX_RECV_NUM; i++)
    {
        if(pHdl->vtValType[i]==DPC_TYPE)
        {
            pHdl->bValueBfInvalid = DP_INVALID_00;
            break;
        }
    }

    HdlBoxIOInfo_g.iHdlDiNum++;
    iHdlDiNum_g++;

    /* 判断是否悬空 */
    for (i = 0; i<HDL_DI_MAX_RECV_NUM; i++)
    {
        if (pHdl->iSubDaIdx[i]>0)
        {
            return (void *)pHdl;
        }
    }

    *bpPended = TRUE;

    return  (void  *)pHdl;
}

/* 初始化智能操作相DO通道
 * 参数：
 *          iModAddr，模件硬件地址,
 *          uiCh，在本模件内的DO物理通道号，从0开始
 * 返回值： 用来索引DO通道的void指针，或者NULL表示调用出错 */
void *HDL_Init_DO(int iModAddr, u_int uiCh)
{

    BOOL   bQuerySuccess;
    HDL_DO_HND  *pHdl;

    assert(iModAddr<MAX_MOD_NUM);
    assert(uiCh<MAX_DO_PER_MOD);

    if(HdlBoxIOInfo_g.aHdlIOModInfo[iModAddr].type==DI_MODULE)
    {
        assert(FALSE);
    }

    if(HdlBoxIOInfo_g.aHdlIOModInfo[iModAddr].type==IDLE_MODULE)
    {
        HdlBoxIOInfo_g.aHdlIOModInfo[iModAddr].type=DO_MODULE;
    }
    HdlBoxIOInfo_g.aHdlIOModInfo[iModAddr].unDoChNum++;

    pHdl=&(HdlBoxIOInfo_g.ahHdlDoHandle[iModAddr*MAX_DO_PER_MOD+uiCh]);
    pHdl->ucMod=iModAddr;
    pHdl->ucHdCh=uiCh;
    pHdl->ulPassWd=HDL_DO_PASSWORD;
    pHdl->iCurVal=0;
    pHdl->iLstPubVal=0;/*   2007-6-25日 张云  */
    pHdl->iHdlDoNum = iHdlDoNum_g; /* 总序号 */
    pHdl->linkNum = -1; /* 压板序号 */

    HdlBoxIOInfo_g.aphHdlDoIdx[HdlBoxIOInfo_g.iHdlDoNum]=pHdl;

    bQuerySuccess=GO_QueryActiveGoTimeSourceDiIdxByDoNum
                  (HDL_BOX_GO_SRC_TYPE,
                   HdlBoxIOInfo_g.iHdlDoNum+1,
                   pHdl->iTimeSrcDiIndex
                  );

    HdlBoxIOInfo_g.iHdlDoNum++;
    iHdlDoNum_g++;

    return  (void  *)pHdl;
}

/* 开出关联压板.
 * Para:
 *     pvDoCh, 开出驱动级句柄.
 * Return:
 *     压板序号(从0开始), or -1.
 */
int16_t HDL_CfgLinkofDo(void *pvDoCh)
{
    HDL_DO_HND *pHdl;

    assert(pvDoCh);
    pHdl = (HDL_DO_HND *)pvDoCh;

    pHdl->linkNum = GetPubGoLinkIndexByDoNum(1, pHdl->iHdlDoNum+1, HDL_BOX_GO_SRC_TYPE, &pHdl->vtValType);

    return pHdl->linkNum;
}

/*  初始化同杆并架AO所有配置
     参数：
          iAOCfgNum:所有AO配置个数
     返回：成功与否*/
EP_STATUS   HDL_InitAOCfg(int  iAOCfgNum)
{
    assert(iAOCfgNum<=MAX_HDL_AO_NUM);

    HdlBoxAoCfg_g.iHDLAONum=iAOCfgNum;
    HdlBoxAoCfg_g.iOptMidSrcAONum=0;

    return  EP_SUCCESS;
}

/* 初始化中间结果源的AO通道
 * 参数：   iSrcType,来源类型
            uiCh，模件中的通道号
 *          pElemIOSrc,逻辑图中间结果输出指针
 * 返回值： EP_STATUS  成功与否
            EP_SUCCESS,成功，其他失败
*/
EP_STATUS  HDL_Init_Mid_Src_AO(int  iSrcType, u_int uiCh, void  *pElemIOSrc)
{
    assert(uiCh<MAX_HDL_AO_NUM);
    assert(iSrcType==DA_MID_SRC);
    assert(pElemIOSrc);

    HdlBoxAoCfg_g.aHdlBoxAoCfg[uiCh].ucAOHdCh=uiCh;
    HdlBoxAoCfg_g.aHdlBoxAoCfg[uiCh].iAOSrcType=iSrcType;
    HdlBoxAoCfg_g.aHdlBoxAoCfg[uiCh].pElemSrc=pElemIOSrc;
    HdlBoxAoCfg_g.aHdlBoxAoCfg[uiCh].fLstPubVal=0.0;
    HdlBoxAoCfg_g.apHdlBoxAoIdx[uiCh]
        =&(HdlBoxAoCfg_g.aHdlBoxAoCfg[uiCh]);
    HdlBoxAoCfg_g.iOptMidSrcAONum++;
    iHDLAoNum_g++;

    assert(HdlBoxAoCfg_g.iOptMidSrcAONum<=HdlBoxAoCfg_g.iHDLAONum);

    return  EP_SUCCESS;
}

/* 查看通道属性
 * 参数：
 *     NONE.
 * 返回值：
 *     NONE.
*/
void HDL_ShowChnAttr(void)
{
    uint32_t i;
    uint32_t j;
    HDL_DI_HND *pHdl;
    HDL_DO_HND *pDoHdl = NULL;

    for (i = 0; i<HdlBoxIOInfo_g.iHdlDiNum; i++)
    {
        pHdl = &(HdlBoxIOInfo_g.ahHdlDiHandle[i]);
        printf("模件%d 通道%d 第一个点号%d 状态%d 消抖时间%d 消抖次数%d\n",
               pHdl->ucMod, pHdl->ucHdCh, pHdl->iSubDaIdx[0],
               pHdl->bSts,
               (int)U8_TO_U32(pHdl->aucFilt[0], pHdl->aucFilt[1], pHdl->aucFilt[2], pHdl->aucFilt[3]),
               (int)pHdl->ulFltCfg);
    }

    printf("总DO通道数%d\n", HdlBoxIOInfo_g.iHdlDoNum);
    for (i = 0; i<MAX_MOD_NUM; i++)
    {
        for (j = 0; j<MAX_DO_PER_MOD; j++)
        {
            pDoHdl = &(HdlBoxIOInfo_g.ahHdlDoHandle[i*MAX_DO_PER_MOD+j]);
            if (pDoHdl->ulPassWd == HDL_DO_PASSWORD)
            {
                printf("模件%d 通道%d 状态%d 总序号%d\n",
                       pDoHdl->ucMod, pDoHdl->ucHdCh, pDoHdl->iCurVal, pDoHdl->iHdlDoNum);
            }
        }
    }
}
