/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_ReportStartTuyuan.C                                    1.0           */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的外部输出元件的代码实现                       */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                     */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                 */
/*                                                                              */
/*         作者           日期                    说明                          */
/*                                                                              */
/*         张云       2005.10.25              创建文件1.0版本                   */

/*                                                                              */
/********************************************************************************/

#include <vxWorks.h>

#include  "RE_ReportStartTuyuan.h"
#include   "string_compat.h"

/****图元的逻辑图文件读取初始化函数****************************/
/*
     功能:从文件中读取图元相关数据,此时已读完图元类型字节
          申请初始化数据节点
          供上层程序添加两节点到连表中
          并进行数据节点内容的部分初始化

*/
/****参数：fp,逻辑图文件指针***************************/
/*         ulReadOffsetToBegain,相对于文件起始,读取的文件偏移位置
           pTuyuanInitData,图元节点初始化数据指针
           pRtElemInitNodePointer,返回申请的图元初始化数据节点内存地址

*/
/*   返回值，EP_STATUS */

EP_STATUS   RE_ReportStartTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
        TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
        NODE ** pRtElemInitNodePointer)
{
    NODE  *pNode;
    ReportStart_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    int  i;
    unsigned  char   *   pucCurInSignalType;
    unsigned  short   *  punCurInSourceSeqNo;
    unsigned  char   *   pucInSourceOutputNo;

    unsigned  long  ulIDStrLen;
    BOOL  bOtherInitSuccess;

    /* 文件指针重新定位  */
    int  OpeSuccess;
    OpeSuccess=fseek(fp,ulReadOffsetToBegain,0);
    if(OpeSuccess!=0)
    {
        LOG_Dbg_Msg("File  Pointer  Seek  Failure  !\n",0,0,0,0,0,0);
        return  EP_SYS_ERR;
    }


    /*申请节点空间   */
    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("ReportStart  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    pInitNode=(ReportStart_Init_Node_Type    *)malloc(sizeof(ReportStart_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("ReportStart  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_REPORTSTART;
    pNode->pTuyuan=(void  *)pInitNode;

    /* 将逻辑图信息指针赋给扫描节点  */
    pInitNode->PublicElemData.elem.pchart=pTuyuanInitData->pchart;

    /* 将扫描时的定值刷新标志指针赋给扫描节点  */

    pInitNode->PublicElemData.pbCurScanDingzhiRreshFlag=
        pTuyuanInitData->pbScanRefreshDingzhiFlag;

    /*　将该图元所在的扫描任务号赋给扫描节点　*/
    pInitNode->PublicElemData.nScanTaskNo=
        pTuyuanInitData->nScanTaskNo;


    /*************读取文件的图元内容*****************/

    /* 读取4保留字节  */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,4);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    /*赋值给节点  */
    pInitNode->PublicElemData.elem.unInNum=1;
    /*循环读取每个输入信息*/
    pucCurInSignalType=pInitNode->aucSourceSignalTypeArr;
    punCurInSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucInSourceOutputNo=pInitNode->aucInSourceOutputNumArr;

    for(i=0; i<1; i++)
    {
        *pucCurInSignalType=LOGIC_SIGNAL;

        /* 读取信号来源   */
        bReadSuccess=ReadUnsignedShortFromResloveSeqFile
                     (fp,punCurInSourceSeqNo);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        /* 读取信号源输出号  */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,pucInSourceOutputNo);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        /* 读取2个报告启动前推时间  */
        bReadSuccess=ReadUnsignedShortFromResloveSeqFile
                     (fp,&(pInitNode->iForwordTime));
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        if(pInitNode->iForwordTime > 1)
        {
            /* 根据北京测试需求,减少回推点数目,保证启动时间不超前故障发生时间 */
            pInitNode->iForwordTime -= 2;
        }

        /* 读取2个保留字节  */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,2);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        /* 读取4个保留字节  */
        /*  bReadSuccess=ReadReserveBytesFromResloveSeqFile
            (fp ,4);
          if(!bReadSuccess)
          {
               return   EP_SYS_ERR;
          }*/
        /* 为了提高效率,循环使用指针操作   */
        pucCurInSignalType++;
        punCurInSourceSeqNo++;
        pucInSourceOutputNo++;
    }/*所有输入循环处理结束  */

    /* 读取4个保留字节  */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,4);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    /*  读取输出信号目的地逻辑标识 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 ( fp,pInitNode->strOutputDestID,&ulIDStrLen);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    /* 读取1个保留字节  */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,1);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    /* 读取3个保留字节  */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,3);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 (fp,pInitNode->strTripTriggerEventID,&ulIDStrLen);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_ReportStartTuyuanReadFileOtherInit
                      (pInitNode);
    assert(bOtherInitSuccess);
    if(!bOtherInitSuccess)
    {
        return  EP_SYS_ERR;
    }

    /* 设定返回的节点地址  */
    *pRtElemInitNodePointer=pNode;

    return   EP_SUCCESS;
}


/****图元的扫描初始化函数****************************/
/*   功能:进行完全初始化.
           首先进行扫描节点的输入来源指针的获得
           然后调用用户开发的算法图元初始化函数
           最后进行录波,标志,遥信,遥测的初始化
           当连表中的所有图元都完成了初始化操作后,
           上层程序,会释放掉初始化节点的所有内存.
*/
/****参数：pElemInitNode  , 图元的操纵的初始化数据节点指针***************************/
/*           pElemScanNode   ,图元操纵的扫描数据节点指针
           pGrpScanNodeList   图元待访问的逻辑分图扫描数据节点连表
*/
/*   返回值，EP_STATUS */

EP_STATUS   RE_ReportStartTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
        LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    ReportStart_Init_Node_Type    *pInitNode;
    NODE  *  pCurSourceNode;
    int  i;
    uint8_t  * pucCurInSignalType;
    uint16_t   * punCurInSignalSourceSeqNo;
    uint8_t   *  pucCurInSignalSourceOutNo;
    EP_ELEM_IO *  pInElem;

    ReportEnableOuterOutput_Scan_Node_Type  *pTripEnableScanNode;

    EP_STATUS  SetCurOutAttribResult;

    uint8_t   ucCurInSignalType;


    pInitNode=(ReportStart_Init_Node_Type    *)pElemInitNode->pTuyuan;
    pucCurInSignalType=pInitNode->aucSourceSignalTypeArr;
    punCurInSignalSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucCurInSignalSourceOutNo=pInitNode->aucInSourceOutputNumArr;
    /* 循环设定图元的输入源指针  */
    for(i=0; i<(pInitNode->PublicElemData.elem.unInNum); i++)
    {

        pCurSourceNode=RE_LstNth(pGrpScanNodeList,
                                 ((*punCurInSignalSourceSeqNo)+1));/* 注意序号从1开始 */

        if(pCurSourceNode==NULL)
        {
            LOG_Dbg_Msg("error,Get  ReportStart  Tuyuan  Input Source  Init  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }

        /* 获得并设置当前输入源的指针 */

        pInElem=(*  RE_GetScanNodeGetOutIOFunc(pCurSourceNode))
                ((*pucCurInSignalSourceOutNo),pCurSourceNode);


        /*  确保该图元输入在来源图元的输出号没有出界 */
        if(pInElem==NULL)
        {

            LOG_Dbg_Msg("error,Get  ReportStart  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }
        /*  设置该IO指针到扫描节点相应的输入  */

        SetCurOutAttribResult=RE_ReportStartTuyuanSetInputIO
                              (i,pInElem,pElemScanNode);
        if(SetCurOutAttribResult!=EP_SUCCESS)
        {
            LOG_Dbg_Msg("error,Set  ReportStart  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }

        /*比较信号类型是否匹配  */

        ucCurInSignalType=pInElem->ucAttrib;

        if(ucCurInSignalType!=(*pucCurInSignalType))
        {
            LOG_Dbg_Msg("error,ReportStart Tuyuan One  Input  Signal Type  can't  Match!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        pucCurInSignalType++;
        punCurInSignalSourceSeqNo++;
        pucCurInSignalSourceOutNo++;
    }

    pTripEnableScanNode=(ReportEnableOuterOutput_Scan_Node_Type *)
                        pElemScanNode->pTuyuan;
    if(pTripEnableScanNode->bDefEventToTrigger)
    {
        /* 若定义了保护启停触发的事件，则获取该事件码 */
        /* 设置由逻辑图上得到的保护启停事件配置信息,供调试配置模块进行比较判别查错
        并获得事件号*/

        EP_STATUS  InitEventResult;
        SCI_EVENT_INFO_TYPE   EventInfo;
        EventInfo.strID=pInitNode->strTripTriggerEventID;
        EventInfo.ucParaCount=0;
        EventInfo.ppParaSourceSignal=pInitNode->apInParaArr;

        InitEventResult=SCI_Init_Get_Event_Info
                        (&EventInfo,&(pTripEnableScanNode->nEventNum));

        if(InitEventResult!=EP_SUCCESS)
        {
            LOG_Dbg_Msg("error,Get ReportStart Tuyuan Trigger \'%s\' EventID Info Failure!\n"
                        ,(int)(pInitNode->strTripTriggerEventID),0,0,0,0,0);


            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,"逻辑图解析错误:获得报告启停图元触发的事件\'%s\'的信息错误\n"
                           ,(int)(pInitNode->strTripTriggerEventID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,"logic grp rslv err:get report startstop  element tirgger event id \'%s\' info err\n"
                           ,(int)(pInitNode->strTripTriggerEventID),0);
            }
            assert(FALSE);
            return  InitEventResult;
        }
    }

    return  EP_SUCCESS;

}


/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_ReportStartTuyuanReadFileOtherInit
(ReportStart_Init_Node_Type * pElemInitNodePointer)
{

    int  nTriggerEventIDLen;
    pElemInitNodePointer->PublicElemData.elem.ucType=RE_REPORTSTART;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_ReportStartTuyuanScanInit;
    nTriggerEventIDLen=strlen
                       (pElemInitNodePointer->strTripTriggerEventID);
    if(nTriggerEventIDLen==0)
    {
        /* 若触发事件字符串为空，则表示无事件需要触发 */
        pElemInitNodePointer->bDefEventToTrigger=FALSE;
    }
    else
    {
        /* 否则，就需要事件进行触发 */
        pElemInitNodePointer->bDefEventToTrigger=TRUE;
    }
    return   TRUE;
}


/*
     功能:根据初始化节点数据，申请扫描节点，并进行扫描节点的部分初始化
     返回扫描节点指针

*/
/****参数：
           pRtElemScanNodePointer,返回申请的图元扫描数据节点内存地址
           pElemInitNode,图元初始化数据节点指针
*/
/*   返回值，EP_STATUS */

EP_STATUS   RE_ReportStartCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{

    NODE  *pNode;
    ReportStart_Init_Node_Type    *pInitNode;
    ReportEnableOuterOutput_Scan_Node_Type   *pScanNode;
    pInitNode=(ReportStart_Init_Node_Type    *)pElemInitNode->pTuyuan;

    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("ReportStart  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pScanNode=(ReportEnableOuterOutput_Scan_Node_Type    *)
              malloc
              (sizeof(ReportEnableOuterOutput_Scan_Node_Type));
    if(pScanNode==NULL)
    {
        LOG_Dbg_Msg("ReportStart  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pNode->ulTuyuanType=RE_REPORT_ENABLE_OUTEROUTPUT_SCAN;
    pNode->pTuyuan=(void  *)pScanNode;
    pScanNode->pChartMsg=
        pInitNode->PublicElemData.elem.pchart;
    pScanNode->bLastScanInputValue=FALSE;
    pScanNode->bDefEventToTrigger=
        pInitNode->bDefEventToTrigger;
    pScanNode->iForwordTime=pInitNode->iForwordTime;
    /* 设定返回的节点地址  */
    *pReturnElemScanNodePointer=(NODE  *)pNode;

    return  EP_SUCCESS;

}


/*
     功能:设置图元的扫描节点的某个输入的指针
*/
/****参数：
           ucInputNum,输入的序号
           pElemIO,输入IO指针
           pScanNode，扫描节点指针
*/
/*   返回值，返回成功与否*/

EP_STATUS   RE_ReportStartTuyuanSetInputIO
(uint8_t  ucInputNum,EP_ELEM_IO *  pElemIO,
 NODE  * pScanNode)
{

    ReportEnableOuterOutput_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(ReportEnableOuterOutput_Scan_Node_Type    *)
                pScanNode->pTuyuan;

    if(ucInputNum>=1)
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Set ReportStart  Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  EP_SYS_ERR;

    }
    pTuyuanNode->pInArr0=pElemIO;
    return   EP_SUCCESS;
}


/***********************************************************************
* RE_ReportEnableOuterOutputTuyuanScan - 扫描处理, 报告允许输出目的的外部输出图元的扫描函数
*
* RETURNS: 无
*
*/
void RE_ReportEnableOuterOutputTuyuanScan(
    NODE *pElemScanNode		/* 图元的操纵的扫描数据节点指针 */
)
{

    BOOL bCurInputValue;
    ReportEnableOuterOutput_Scan_Node_Type *pScanNode;

    pScanNode=(ReportEnableOuterOutput_Scan_Node_Type *)
              pElemScanNode->pTuyuan;
    bCurInputValue=pScanNode->pInArr0->now.bVal;

    /*************************************/

    if((!(pScanNode->bLastScanInputValue))
            &&bCurInputValue)
    {
        VI_Bgn_Fault(pScanNode->pChartMsg->ulScnAiCnt,pScanNode->iForwordTime);
        /* 保存此次的输入,供下次使用 */
        pScanNode->bLastScanInputValue=bCurInputValue;
        if(pScanNode->bDefEventToTrigger)
        {
            /* 若定义了保护启停触发事件，则触发保护启动事件,
            	 这里要先设置启动标志，后触发 */
            SCI_Trigger_Event(pScanNode->nEventNum,
                              pScanNode->pChartMsg->ulScnTime - (pScanNode->iForwordTime)*rdinfo_g.uiSmplPeriod,TRUE);
        }
        return;
    }
    else if((pScanNode->bLastScanInputValue)
            &&(!(bCurInputValue)))
    {
        if(pScanNode->bDefEventToTrigger)
        {
            /* 若定义了保护启停触发事件，则触发保护启动返回事件，
                这里要先触发事件，再设置整组复归标志 */
            SCI_Trigger_Event(pScanNode->nEventNum,
                              pScanNode->pChartMsg->ulScnTime,FALSE);
        }
        /* 若是下降沿，则整组复归 */

        VI_End_Fault(pScanNode->pChartMsg->ulScnAiCnt);

        pScanNode->bLastScanInputValue=bCurInputValue;
        return;
    }

}