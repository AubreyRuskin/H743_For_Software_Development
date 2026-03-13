/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_EventTuyuan.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的事件元件的代码实现                        */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                    */
/*                                                                              */
/*         张云       2002.12.12              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/


#include <vxWorks.h>

#include  "RE_EventTuyuan.h"



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

EP_STATUS   RE_EventTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                       TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
                                       NODE ** pRtElemInitNodePointer)
{
    NODE   *pNode;
    Event_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    unsigned  long   nEventIDStrLen;
    int  i;
    unsigned  char   ucInputCount;
    unsigned  char   *   pucCurInSignalType;
    unsigned  short   *  punCurInSourceSeqNo;
    unsigned  char   *   pucInSourceOutputNo;
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
        LOG_Dbg_Msg("Event  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    pInitNode=(Event_Init_Node_Type    *)malloc(sizeof(Event_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("Event  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_EVENT;
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
    /* 读取图元输入个数 */
    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&ucInputCount);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    if((ucInputCount>MAX_EVENT_INPUT_COUNT)
            ||(ucInputCount<1))
    {
        LOG_Dbg_Msg("error,Event  Tuyuan  Input  Count  is  out of  range!\n",0,0,0,0,0,0);
        assert(FALSE);

        return  EP_SYS_ERR;

    }
    /*赋值输入数给节点  */
    pInitNode->PublicElemData.elem.unInNum=(unsigned  short)ucInputCount;

    /* 读取1保留字节  */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,1);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    /* 读取事件逻辑标识  */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 ( fp,pInitNode->strEventID,&nEventIDStrLen);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    /*循环读取每个输入信息,包括触发信号和事件参数*/
    pucCurInSignalType=pInitNode->aucSourceSignalTypeArr;
    punCurInSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucInSourceOutputNo=pInitNode->aucInSourceOutputNumArr;


    for(i=0; i<ucInputCount; i++)
    {
        if(i==0)
        {
            /* 若是触发信号,则不需读取信号类型,直接设置为逻辑信号 */
            *pucCurInSignalType=LOGIC_SIGNAL;
        }
        else
        {
            /*若是事件参数  */
            /*读取信号类型  */
            bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                         (fp,pucCurInSignalType);
            if(!bReadSuccess)
            {
                return   EP_SYS_ERR;
            }
        }
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
        /* 读取4个保留字节  */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        /* 为了提高效率,循环使用指针操作   */
        pucCurInSignalType++;
        punCurInSourceSeqNo++;
        pucInSourceOutputNo++;
    }/*所有输入循环处理结束  */


    /*赋值输出个数给节点 ， */

    pInitNode->PublicElemData.elem.ucOutNum=0;

    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_EventTuyuanReadFileOtherInit
                      (pInitNode);
    assert(bOtherInitSuccess);
    if(!bOtherInitSuccess)
    {
        return  EP_SYS_ERR;
    }

    /* 设定返回的节点地址  */
    *pRtElemInitNodePointer=(NODE  *)pNode;

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

EP_STATUS   RE_EventTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                   LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{


    Event_Init_Node_Type    *pInitNode;
    Event_Scan_Node_Type    *pScanNode;
    NODE  *  pCurSourceNode;
    int  i;
    uint8_t  * pucCurInSignalType;
    uint16_t   * punCurInSignalSourceSeqNo;
    uint8_t   *  pucCurInSignalSourceOutNo;
    EP_ELEM_IO **  pInElemAddr;
    EP_ELEM_IO *  pInElem;


    EP_STATUS  SetCurOutAttribResult;

    SCI_EVENT_INFO_TYPE   EventInfo;
    /* 设置由逻辑图上得到的事件配置信息,供调试配置模块进行比较判别查错
       并获得事件号*/
    uint8_t   ucCurInSignalType;

    EP_STATUS  InitEventResult;

    pInitNode=(Event_Init_Node_Type    *)pElemInitNode->pTuyuan;
    pScanNode=(Event_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    pucCurInSignalType=pInitNode->aucSourceSignalTypeArr;
    punCurInSignalSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucCurInSignalSourceOutNo=pInitNode->aucInSourceOutputNumArr;
    pInElemAddr=pInitNode->apInParaArr;/*事件参数指针  */
    /* 循环设定图元的输入源指针  */
    for(i=0; i<(pInitNode->PublicElemData.elem.unInNum); i++)
    {

        pCurSourceNode=RE_LstNth(pGrpScanNodeList,
                                 ((*punCurInSignalSourceSeqNo)+1));/* 注意序号从1开始 */

        if(pCurSourceNode==NULL)
        {
            LOG_Dbg_Msg("error,Get  Event Tuyuan  Input Source  Init  failure!\n",0,0,0,0,0,0);

            assert(FALSE);
            return  EP_SYS_ERR;

        }

        /* 获得并设置当前输入源的指针 */

        /*  确保该图元输入在来源图元的输出号没有出界 */
        if(i==0)
        {
            /* 若是事件触发输入 */
            pInElem=(* RE_GetScanNodeGetOutIOFunc(pCurSourceNode))
                    ((*pucCurInSignalSourceOutNo),pCurSourceNode);


            /*  确保该图元输入在来源图元的输出号没有出界 */
            if(pInElem==NULL)
            {

                LOG_Dbg_Msg("error,Get  Event  Tuyuan Trigger input info  failure!\n",0,0,0,0,0,0);

                assert(FALSE);  /* 若非以上类型,则告警   */
                return  EP_SYS_ERR;

            }
            /*  设置该IO指针到扫描节点相应的输入  */

            SetCurOutAttribResult=RE_EventTuyuanSetInputIO
                                  (i,pInElem,pElemScanNode);
            if(SetCurOutAttribResult!=EP_SUCCESS)
            {

                LOG_Dbg_Msg("error,Set  Event  Tuyuan Trigger input  info  failure!\n",0,0,0,0,0,0);

                assert(FALSE);  /* 若非以上类型,则告警   */
                return  EP_SYS_ERR;

            }

            ucCurInSignalType=pInElem->ucAttrib;

            if(ucCurInSignalType!=(*pucCurInSignalType))
            {
                LOG_Dbg_Msg("error,Event Tuyuan One Trigger Input  Signal Type  can't  Match!\n",0,0,0,0,0,0);

                assert(FALSE);
                return  EP_SYS_ERR;
            }

        }
        else
        {
            /* 若是事件参数输入*/

            (*pInElemAddr)=(* RE_GetScanNodeGetOutIOFunc(pCurSourceNode))
                           ((*pucCurInSignalSourceOutNo),pCurSourceNode);


            /*  确保该图元输入在来源图元的输出号没有出界 */
            if((*pInElemAddr)==NULL)
            {

                LOG_Dbg_Msg("error,Get  Event  Tuyuan  Para  input  info  failure!\n",0,0,0,0,0,0);

                assert(FALSE);  /* 若非以上类型,则告警   */
                return  EP_SYS_ERR;

            }
            ucCurInSignalType=(*pInElemAddr)->ucAttrib;

            if(ucCurInSignalType!=(*pucCurInSignalType))
            {
                LOG_Dbg_Msg("error,Event Tuyuan One Para Input  Signal Type  can't  Match!\n",0,0,0,0,0,0);

                assert(FALSE);
                return  EP_SYS_ERR;
            }

            pInElemAddr++;

        }
        /*比较信号类型是否匹配  */

        pucCurInSignalType++;
        punCurInSignalSourceSeqNo++;
        pucCurInSignalSourceOutNo++;
    }


    /*根据逻辑标识和事件信息获得事件的操作代号  */

    EventInfo.strID=pInitNode->strEventID;
    EventInfo.ucParaCount=pInitNode->PublicElemData.elem.unInNum-1;
    EventInfo.ppParaSourceSignal=pInitNode->apInParaArr;

    InitEventResult=SCI_Init_Get_Event_Info
                    (&EventInfo,&(pScanNode->nEventNum));

    if(InitEventResult!=EP_SUCCESS)
    {
        LOG_Dbg_Msg("error,Get  Event  Tuyuan  \'%s\'  EventID  Info  Failure!\n"
                    ,(int)(pInitNode->strEventID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得事件图元标识为\'%s\'的事件信息失败\n"
                       ,(int)(pInitNode->strEventID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get event  element  id\'%s\' event info err\n"
                       ,(int)(pInitNode->strEventID),0);
        }
        assert(FALSE);
        return  InitEventResult;
    }
    return  EP_SUCCESS;

}






/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_EventTuyuanReadFileOtherInit
(Event_Init_Node_Type * pElemInitNodePointer)
{

    pElemInitNodePointer->PublicElemData.elem.ucType=RE_EVENT;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_EventTuyuanScanInit;


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

EP_STATUS   RE_EventCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{
    NODE  *pNode;
    Event_Init_Node_Type    *pInitNode;
    Event_Scan_Node_Type   *pScanNode;

    pInitNode=(Event_Init_Node_Type    *)pElemInitNode->pTuyuan;
    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("Event  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pScanNode=(Event_Scan_Node_Type   *)malloc
              (sizeof(Event_Scan_Node_Type));
    if(pScanNode==NULL)
    {
        LOG_Dbg_Msg("Event  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    /*  设置图元类型*/

    pNode->ulTuyuanType=RE_EVENT_SCAN;
    pNode->pTuyuan=(void  *)pScanNode;

    pScanNode->pChartMsg=pInitNode->PublicElemData.elem.pchart;
    pScanNode->bLastScanInputValue=FALSE;
    pScanNode->bFstEnter = TRUE;

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

EP_STATUS   RE_EventTuyuanSetInputIO
(uint8_t  ucInputNum,EP_ELEM_IO *  pElemIO,
 NODE  * pScanNode)
{

    Event_Scan_Node_Type    *pTuyuanNode;

    pTuyuanNode=(Event_Scan_Node_Type    *)pScanNode->pTuyuan;
    if(ucInputNum>=1)
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Set One  Event Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  EP_SYS_ERR;

    }
    pTuyuanNode->pInArr0=pElemIO;
    return   EP_SUCCESS;


}

/*    事件图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_EventTuyuanScan(NODE *pElemScanNode)
{

    Event_Scan_Node_Type    *pScanNode;
    BOOL   bIsAdvance,bIsFall;

    pScanNode=(Event_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    bIsAdvance=(!(pScanNode->bLastScanInputValue))&&
               (pScanNode->pInArr0->now.bVal);
    bIsFall=(pScanNode->bLastScanInputValue)&&
            (!(pScanNode->pInArr0->now.bVal));

    /* 处理事件的呼唤属性 */
    SCI_Deal_Event_Alert(pScanNode->nEventNum, pScanNode->pInArr0->now.bVal);

    if(!(bIsAdvance||bIsFall))
    {
        /* 若非上升沿,也非下降沿 */
        /* 保存此次的输入,供下次触发使用 */
        pScanNode->bLastScanInputValue=
            pScanNode->pInArr0->now.bVal;
        return ;
    }
    else  if(bIsAdvance)
    {
        /*若当前处于上升沿,则触发事件 */
        SCI_Trigger_Event(pScanNode->nEventNum,
                          pScanNode->pChartMsg->ulScnTime,TRUE);
        /* 保存此次的输入,供下次触发使用 */
        pScanNode->bLastScanInputValue=
            pScanNode->pInArr0->now.bVal;
        return;
    }
    else
    {
        /*若当前处于下降沿,则触发事件 */
        SCI_Trigger_Event(pScanNode->nEventNum,
                          pScanNode->pChartMsg->ulScnTime,FALSE);
        /* 保存此次的输入,供下次触发使用 */
        pScanNode->bLastScanInputValue=
            pScanNode->pInArr0->now.bVal;
        return;
    }
}

/* 扫描多个事件节点.
 * Para:
 *     PartGrpAttrib, 分图属性.
 * Return:
 *     NONE.
 */
void RE_MultiEventTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib)
{
    NODE **ppCurEventNode;
    int i;
    Event_Scan_Node_Type *pScanNode;
    BOOL bIsAdvance;
    BOOL bIsFall;

    ppCurEventNode = PartGrpAttrib->ppEventNode;
    for (i = 0; i<PartGrpAttrib->EventScanNodeNum; i++, ppCurEventNode++)
    {
        pScanNode = (Event_Scan_Node_Type *)(*ppCurEventNode)->pTuyuan;

        /* 处理事件的呼唤属性 */
        if (pScanNode->pInArr0->now.bVal)
            SCI_Deal_Event_Alert(pScanNode->nEventNum, pScanNode->pInArr0->now.bVal);

        if (pScanNode->bLastScanInputValue == pScanNode->pInArr0->now.bVal)
        {
            continue;
        }

        bIsAdvance = (!(pScanNode->bLastScanInputValue)) &&
                     (pScanNode->pInArr0->now.bVal);
        bIsFall = (pScanNode->bLastScanInputValue)&&
                  (!(pScanNode->pInArr0->now.bVal));

        if (bIsAdvance)
        {
            /* 若当前处于上升沿,则触发事件 */
            SCI_Trigger_Event(pScanNode->nEventNum,
                              pScanNode->pChartMsg->ulScnTime, TRUE);
            /* 保存此次的输入,供下次触发使用 */
        }
        else if (bIsFall)
        {
            /* 若当前处于下降沿,则触发事件 */
            SCI_Trigger_Event(pScanNode->nEventNum,
                              pScanNode->pChartMsg->ulScnTime, FALSE);
            /* 保存此次的输入,供下次触发使用 */
        }
        pScanNode->bLastScanInputValue =
            pScanNode->pInArr0->now.bVal;
    }
}