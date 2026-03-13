/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_LuboTuyuan.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的录波元件的代码实现                        */
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

#include   "RE_LuboTuyuan.h"

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

EP_STATUS   RE_LuboTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                      TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
                                      NODE ** pRtElemInitNodePointer)
{
    NODE  *pNode;
    Lubo_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    int  i;
    unsigned  char   *   pucCurInSignalType;
    unsigned  short   *  punCurInSourceSeqNo;
    unsigned  char   *   pucInSourceOutputNo;
    unsigned  char   ucStartStopType;
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
        LOG_Dbg_Msg("Lubo  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    pInitNode=(Lubo_Init_Node_Type    *)malloc(sizeof(Lubo_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("Lubo  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_LUBO;
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

        /*设置信号类型  */
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



    /*赋值输出给节点 ， */

    /* 该扫描节点的输出个数 */
    pInitNode->PublicElemData.elem.ucOutNum=0;



    /* 读取启停操作类型 */
    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&(pInitNode->ucLuboStartStopType));
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    ucStartStopType=pInitNode->ucLuboStartStopType;
    /*判断启停类型是所期望的类型  */

    if((ucStartStopType!=LUBO_ONLY_START)
            &&(ucStartStopType!=LUBO_ONLY_STOP)
            &&(ucStartStopType!=LUBO_STARTSTOP))
    {

        LOG_Dbg_Msg("Lubo Tuyuan StartStop  Type  isn't  Expected!\n",0,0,0,0,0,0);
        assert(FALSE);  /* 若非以上类型,则告警   */
        return  EP_SYS_ERR;

    }

    /* 读取4个保留字节 */

    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,4);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    /* 若是启动和启停类型,则读取启动录波触发类型 */
    if(((pInitNode->ucLuboStartStopType)==LUBO_ONLY_START)
            ||((pInitNode->ucLuboStartStopType)==LUBO_STARTSTOP))
    {
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&(pInitNode->ucLuboStartTriggerType));
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        if((pInitNode->ucLuboStartTriggerType!=ADVANCING_EDGE)
                &&(pInitNode->ucLuboStartTriggerType!=FALLING_EDGE))
        {
            LOG_Dbg_Msg("Lubo Tuyuan Start Trigger  Type  isn't  Expected!\n",0,0,0,0,0,0);
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }

    }

    /* 读取4个保留字节 */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,4);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    /*若是启动和启停类型,则读取录波启动信息  */

    if(((pInitNode->ucLuboStartStopType)==LUBO_ONLY_START)
            ||((pInitNode->ucLuboStartStopType)==LUBO_STARTSTOP))
    {
        /* 读取前向录波持续时间 */
        unsigned  long   ulLuboTime;
        uint8_t   ucBackwardLuboTimeType;
        bReadSuccess=ReadUnsignedLongFromResloveSeqFile
                     (fp,&ulLuboTime);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        pInitNode->LuboStartInfo.unForwardLuboTime=
            (unsigned  short)ulLuboTime;

        /*  读取后向录波持续时间类型 */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&(pInitNode->LuboStartInfo.ucBackwardLuboTimeType));
        ucBackwardLuboTimeType=pInitNode->LuboStartInfo.
                               ucBackwardLuboTimeType;
        /* 确保读取的后向录波持续时间类型是所期望的类型 */

        if((ucBackwardLuboTimeType!=LUBO_PERSIST_FINITETIME)
                &&(ucBackwardLuboTimeType!=LUBO_PERSIST_INFINITETIME))
        {
            LOG_Dbg_Msg("Lubo Tuyuan Start Backward Lubo  Time  \nType  isn't  Expected!\n",0,0,0,0,0,0);
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        /*若后向录波持续时间类型为有限时间类型,则读取录波后向持续时间  */
        if(ucBackwardLuboTimeType==LUBO_PERSIST_FINITETIME)
        {
            bReadSuccess=ReadUnsignedLongFromResloveSeqFile(fp,(long unsigned int *)&(pInitNode->LuboStartInfo.ulBackwardLuboTime));
            if(!bReadSuccess)
            {
                return   EP_SYS_ERR;
            }
        }

        /*读取录波记录频率  */
        bReadSuccess=ReadUnsignedShortFromResloveSeqFile
                     (fp,&(pInitNode->LuboStartInfo.unLuboFreq));
    }

    /* 若是停止或启停类型,则读取录波停止触发类型  */

    if(((pInitNode->ucLuboStartStopType)==LUBO_ONLY_STOP)
            ||((pInitNode->ucLuboStartStopType)==LUBO_STARTSTOP))
    {
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&(pInitNode->ucLuboStopTriggerType));
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        if((pInitNode->ucLuboStopTriggerType!=ADVANCING_EDGE)
                &&(pInitNode->ucLuboStopTriggerType!=FALLING_EDGE))
        {
            LOG_Dbg_Msg("Lubo Tuyuan Stop Trigger  Type  isn't  Expected!\n",0,0,0,0,0,0);
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }

    }

    /* 读取4个保留字节 */

    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,4);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }


    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_LuboTuyuanReadFileOtherInit
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

EP_STATUS   RE_LuboTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                  LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    Lubo_Init_Node_Type    *pInitNode;
    NODE  *  pCurSourceNode;
    int  i;
    uint8_t  * pucCurInSignalType;
    uint16_t   * punCurInSignalSourceSeqNo;
    uint8_t   *  pucCurInSignalSourceOutNo;
    EP_ELEM_IO *  pInElem;

    uint8_t   ucCurInSignalType;


    EP_STATUS  SetCurOutAttribResult;


    pInitNode=(Lubo_Init_Node_Type    *)pElemInitNode->pTuyuan;

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
            LOG_Dbg_Msg("error,Get  Lubo  Tuyuan  Input Source  Init  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }
        /* 获得并设置当前输入源的指针 */

        pInElem=(* RE_GetScanNodeGetOutIOFunc(pCurSourceNode))
                ((*pucCurInSignalSourceOutNo),pCurSourceNode);


        /*  确保该图元输入在来源图元的输出号没有出界 */
        if(pInElem==NULL)
        {

            LOG_Dbg_Msg("error,Get  Lubo  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }
        /*  设置该IO指针到扫描节点相应的输入  */

        SetCurOutAttribResult=RE_LuboTuyuanSetInputIO
                              (i,pInElem,pElemScanNode);
        if(SetCurOutAttribResult!=EP_SUCCESS)
        {

            LOG_Dbg_Msg("error,Set  Lubo  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }

        /*比较信号类型是否匹配  */

        ucCurInSignalType=pInElem->ucAttrib;

        if(ucCurInSignalType!=(*pucCurInSignalType))
        {
            LOG_Dbg_Msg("error,Lubo Tuyuan One  Input  Signal Type  can't  Match!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        pucCurInSignalType++;
        punCurInSignalSourceSeqNo++;
        pucCurInSignalSourceOutNo++;
    }


    return  EP_SUCCESS;


}





/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_LuboTuyuanReadFileOtherInit
(Lubo_Init_Node_Type * pElemInitNodePointer)
{

    pElemInitNodePointer->PublicElemData.elem.ucType=RE_LUBO;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_LuboTuyuanScanInit;

    /*若是录波启停类型,确保录波启动和停止的触发条件不同  */
    if((pElemInitNodePointer->ucLuboStartStopType)==
            LUBO_STARTSTOP)
    {

        if((pElemInitNodePointer->ucLuboStartTriggerType)
                ==(pElemInitNodePointer->ucLuboStopTriggerType))
        {
            LOG_Dbg_Msg("error,StartStop  Lubo  Tuyuan  Start Trigger Type\n Can't Equal  Stop  Trigger  Type!\n",0,0,0,0,0,0);
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  FALSE;
        }

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

EP_STATUS   RE_LuboCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{

    NODE  *pNode;
    Lubo_Init_Node_Type    *pInitNode;
    pInitNode=(Lubo_Init_Node_Type    *)pElemInitNode->pTuyuan;

    /*  设置图元类型*/
    switch(pInitNode->ucLuboStartStopType)
    {
        case  LUBO_ONLY_START:
        {
            OnlyStartLubo_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("OnlyStart  Lubo   Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(OnlyStartLubo_Scan_Node_Type   *)
                      malloc
                      (sizeof(OnlyStartLubo_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("OnlyStart  Lubo  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_ONLYSTART_LUBO_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->ucLuboStartTriggerType=pInitNode->
                                              ucLuboStartTriggerType;
            pScanNode->LuboStartInfo=pInitNode->
                                     LuboStartInfo;
            pScanNode->pChartMsg=
                pInitNode->PublicElemData.elem.pchart;
            pScanNode->bLastScanInputValue=FALSE;

            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  LUBO_ONLY_STOP:
        {
            OnlyStopLubo_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("OnlyStop  Lubo   Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(OnlyStopLubo_Scan_Node_Type    *)
                      malloc
                      (sizeof(OnlyStopLubo_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("OnlyStop  Lubo  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_ONLYSTOP_LUBO_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->ucLuboStopTriggerType=pInitNode->
                                             ucLuboStopTriggerType;
            pScanNode->pChartMsg=
                pInitNode->PublicElemData.elem.pchart;
            pScanNode->bLastScanInputValue=FALSE;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  LUBO_STARTSTOP:
        {
            StartStopLubo_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("StartStop  Lubo   Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(StartStopLubo_Scan_Node_Type    *)
                      malloc
                      (sizeof(StartStopLubo_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("StartStop  Lubo  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_STARTSTOP_LUBO_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->ucLuboStartTriggerType=pInitNode->
                                              ucLuboStartTriggerType;
            pScanNode->LuboStartInfo=pInitNode->
                                     LuboStartInfo;
            pScanNode->ucLuboStopTriggerType=pInitNode->
                                             ucLuboStopTriggerType;
            pScanNode->pChartMsg=
                pInitNode->PublicElemData.elem.pchart;
            pScanNode->bLastScanInputValue=FALSE;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        default:

            assert(FALSE);

            return  EP_SYS_ERR;
            break;
    }


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

EP_STATUS   RE_LuboTuyuanSetInputIO
(uint8_t  ucInputNum,EP_ELEM_IO *  pElemIO,
 NODE  * pScanNode)
{

    switch(pScanNode->ulTuyuanType)
    {
        case   RE_ONLYSTART_LUBO_SCAN:
        {

            OnlyStartLubo_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(OnlyStartLubo_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set One  Lubo Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_ONLYSTOP_LUBO_SCAN:
        {

            OnlyStopLubo_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(OnlyStopLubo_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set One  Lubo Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;
        }
        break;

        case   RE_STARTSTOP_LUBO_SCAN:
        {

            StartStopLubo_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(StartStopLubo_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set One   Lubo  Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;
        }
        break;

        default  :

            LOG_Dbg_Msg("Set One Lubo  Tuyuan  InputputIO  Error!\n",0,0,0,0,0,0);
            assert(FALSE);
            return  EP_SYS_ERR;
            break;

    }

    return  EP_SUCCESS;

}


/*    启动型录波图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_OnlyStartLuboTuyuanScan(NODE *pElemScanNode)
{

    BOOL   bCurInputValue;
    OnlyStartLubo_Scan_Node_Type    *pScanNode;

    pScanNode=(OnlyStartLubo_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    bCurInputValue=pScanNode->pInArr0->now.bVal;

    if(pScanNode->ucLuboStartTriggerType==ADVANCING_EDGE)
    {
        /* 若是上升沿触发条件 */
        if(!((!(pScanNode->bLastScanInputValue))&&
                bCurInputValue))
        {
            /* 若非上升沿，这是大部分情形  */

            /* 保存此次的输入,供下次使用 */
            pScanNode->bLastScanInputValue=bCurInputValue;
            return;
        }
        else
        {
            /*若当前处于上升沿,则启动录波 */
            SCI_Set_Lubo_Start_Flag(
                &(pScanNode->LuboStartInfo)
                ,pScanNode->pChartMsg->ulScnAiCnt);
            /* 保存此次的输入,供下次使用 */
            pScanNode->bLastScanInputValue=bCurInputValue;
            return;

        }
    }
    else
    {
        /* 若是下降沿触发条件 */

        if(!((pScanNode->bLastScanInputValue)&&
                (!bCurInputValue)))
        {
            /*  若非下降沿，这是大部分情形 */
            /* 保存此次的输入,供下次使用 */
            pScanNode->bLastScanInputValue=bCurInputValue;
            return;
        }
        else
        {
            /*若当前处于下降沿,则启动录波 */
            SCI_Set_Lubo_Start_Flag(
                &(pScanNode->LuboStartInfo)
                ,pScanNode->pChartMsg->ulScnAiCnt);
            /* 保存此次的输入,供下次使用 */
            pScanNode->bLastScanInputValue=bCurInputValue;
            return;

        }
    }

}

/* 扫描多个启动型录波节点.
 * Para:
 *     PartGrpAttrib, 分图属性.
 * Return:
 *     NONE.
 */
void RE_MultiOnlyStartLuboTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib)
{
    NODE **ppCurOnlyStartLuboNode;
    int i;
    OnlyStartLubo_Scan_Node_Type *pScanNode;
    BOOL bCurInputValue;

    ppCurOnlyStartLuboNode = PartGrpAttrib->ppOnlyStartLuboNode;
    for (i = 0; i<PartGrpAttrib->OnlyStartLuboScanNodeNum; i++, ppCurOnlyStartLuboNode++)
    {
        pScanNode = (OnlyStartLubo_Scan_Node_Type *)(*ppCurOnlyStartLuboNode)->pTuyuan;
        bCurInputValue = pScanNode->pInArr0->now.bVal;

        if (pScanNode->bLastScanInputValue == bCurInputValue)
        {
            continue;
        }

        if (pScanNode->ucLuboStartTriggerType == ADVANCING_EDGE)
        {
            /* 若是上升沿触发条件 */
            if ((!(pScanNode->bLastScanInputValue)) && bCurInputValue)
            {
                /* 若当前处于上升沿,则启动录波 */
                SCI_Set_Lubo_Start_Flag(
                    &(pScanNode->LuboStartInfo), pScanNode->pChartMsg->ulScnAiCnt);
            }
        }
        else
        {
            /* 若是下降沿触发条件 */
            if ((pScanNode->bLastScanInputValue) && (!bCurInputValue))
            {
                /* 若当前处于下降沿,则启动录波 */
                SCI_Set_Lubo_Start_Flag(&(pScanNode->LuboStartInfo), pScanNode->pChartMsg->ulScnAiCnt);
            }
        }
        pScanNode->bLastScanInputValue=bCurInputValue;
    }
}

/*    停止型录波图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_OnlyStopLuboTuyuanScan(NODE *pElemScanNode)
{

    BOOL   bCurInputValue;
    OnlyStopLubo_Scan_Node_Type    *pScanNode;

    pScanNode=(OnlyStopLubo_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    bCurInputValue=pScanNode->pInArr0->now.bVal;

    if(pScanNode->ucLuboStopTriggerType==ADVANCING_EDGE)
    {
        /* 若是上升沿触发条件 */
        if(!((!(pScanNode->bLastScanInputValue))&&
                bCurInputValue))
        {
            /* 保存此次的输入,供下次使用 */
            pScanNode->bLastScanInputValue=bCurInputValue;
            return;
        }
        else
        {
            /*若当前处于上升沿,则停止录波 */
            SCI_Set_Lubo_Stop_Flag(pScanNode->pChartMsg->ulScnAiCnt);
            /* 保存此次的输入,供下次使用 */
            pScanNode->bLastScanInputValue=bCurInputValue;
            return;

        }
    }
    else
    {
        /* 若是下降沿触发条件 */

        if(!((pScanNode->bLastScanInputValue)&&
                (!bCurInputValue)))
        {
            /* 保存此次的输入,供下次使用 */
            pScanNode->bLastScanInputValue=bCurInputValue;
            return;
        }
        else
        {
            /*若当前处于下降沿,则停止录波 */
            SCI_Set_Lubo_Stop_Flag(pScanNode->pChartMsg->ulScnAiCnt);
            /* 保存此次的输入,供下次使用 */
            pScanNode->bLastScanInputValue=bCurInputValue;
            return;

        }
    }

}

/* 扫描多个停止型录波节点.
 * Para:
 *     PartGrpAttrib, 分图属性.
 * Return:
 *     NONE.
 */
void RE_MultiOnlyStopLuboTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib)
{
    NODE **ppCurOnlyStopLuboNode;
    int i;
    OnlyStopLubo_Scan_Node_Type *pScanNode;
    BOOL bCurInputValue;

    ppCurOnlyStopLuboNode = PartGrpAttrib->ppOnlyStopLuboNode;
    for (i = 0; i<PartGrpAttrib->OnlyStopLuboScanNodeNum; i++, ppCurOnlyStopLuboNode++)
    {
        pScanNode = (OnlyStopLubo_Scan_Node_Type *)(*ppCurOnlyStopLuboNode)->pTuyuan;
        bCurInputValue = pScanNode->pInArr0->now.bVal;

        if (pScanNode->bLastScanInputValue == bCurInputValue)
        {
            continue;
        }

        if (pScanNode->ucLuboStopTriggerType == ADVANCING_EDGE)
        {
            /* 若是上升沿触发条件 */
            if ((!(pScanNode->bLastScanInputValue)) && bCurInputValue)
            {
                /* 若当前处于上升沿,则停止录波 */
                SCI_Set_Lubo_Stop_Flag(pScanNode->pChartMsg->ulScnAiCnt);
            }
        }
        else
        {
            /* 若是下降沿触发条件 */
            if ((pScanNode->bLastScanInputValue) && (!bCurInputValue))
            {
                /* 若当前处于下降沿,则停止录波 */
                SCI_Set_Lubo_Stop_Flag(pScanNode->pChartMsg->ulScnAiCnt);
            }
        }
        pScanNode->bLastScanInputValue=bCurInputValue;
    }
}

/*    启动停止类型的录波图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_StartStopLuboTuyuanScan(NODE *pElemScanNode)
{

    BOOL   bCurInputValue;
    StartStopLubo_Scan_Node_Type    *pScanNode;

    pScanNode=(StartStopLubo_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    bCurInputValue=pScanNode->pInArr0->now.bVal;

    if((!(pScanNode->bLastScanInputValue))&&
            (bCurInputValue))
    {
        /*若当前处于上升沿 */
        if(pScanNode->ucLuboStartTriggerType==ADVANCING_EDGE)
        {
            /*若是启动条件为上升沿,则启动录波 */
            SCI_Set_Lubo_Start_Flag(
                &(pScanNode->LuboStartInfo)
                ,pScanNode->pChartMsg->ulScnAiCnt);

        }
        if(pScanNode->ucLuboStopTriggerType==ADVANCING_EDGE)
        {
            /* 若停止条件为上升沿,则停止录波 */
            SCI_Set_Lubo_Stop_Flag(pScanNode->pChartMsg->ulScnAiCnt);
        }
    }
    else  if((pScanNode->bLastScanInputValue)&&
             (!(bCurInputValue)))
    {
        /* 若当前处于下降沿 */
        if(pScanNode->ucLuboStartTriggerType==FALLING_EDGE)
        {
            /* 若是启动条件为下降沿,则启动录波 */
            SCI_Set_Lubo_Start_Flag(
                &(pScanNode->LuboStartInfo)
                ,pScanNode->pChartMsg->ulScnAiCnt);
        }
        if(pScanNode->ucLuboStopTriggerType==FALLING_EDGE)
        {
            /*若停止条件为下降沿,则停止录波*/
            SCI_Set_Lubo_Stop_Flag(pScanNode->pChartMsg->ulScnAiCnt);
        }
    }
    /* 保存此次的输入,供下次使用 */
    pScanNode->bLastScanInputValue=bCurInputValue;



}

/* 扫描多个启动停止型录波节点.
 * Para:
 *     PartGrpAttrib, 分图属性.
 * Return:
 *     NONE.
 */
void RE_MultiStartStopLuboTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib)
{
    NODE **ppCurStartStopLuboNode;
    int i;
    StartStopLubo_Scan_Node_Type *pScanNode;
    BOOL bCurInputValue;

    ppCurStartStopLuboNode = PartGrpAttrib->ppStratStopLuboNode;
    for (i = 0; i<PartGrpAttrib->StartStopLuboScanNodeNum; i++, ppCurStartStopLuboNode++)
    {
        pScanNode = (StartStopLubo_Scan_Node_Type *)(*ppCurStartStopLuboNode)->pTuyuan;
        bCurInputValue = pScanNode->pInArr0->now.bVal;

        if (pScanNode->bLastScanInputValue == bCurInputValue)
        {
            continue;
        }

        if ((!(pScanNode->bLastScanInputValue)) && (bCurInputValue))
        {
            /* 若当前处于上升沿 */
            if (pScanNode->ucLuboStartTriggerType == ADVANCING_EDGE)
            {
                /* 若是启动条件为上升沿,则启动录波 */
                SCI_Set_Lubo_Start_Flag(&(pScanNode->LuboStartInfo), pScanNode->pChartMsg->ulScnAiCnt);
            }
            if (pScanNode->ucLuboStopTriggerType == ADVANCING_EDGE)
            {
                /* 若停止条件为上升沿,则停止录波 */
                SCI_Set_Lubo_Stop_Flag(pScanNode->pChartMsg->ulScnAiCnt);
            }
        }
        else if ((pScanNode->bLastScanInputValue) && (!(bCurInputValue)))
        {
            /* 若当前处于下降沿 */
            if (pScanNode->ucLuboStartTriggerType == FALLING_EDGE)
            {
                /* 若是启动条件为下降沿,则启动录波 */
                SCI_Set_Lubo_Start_Flag(&(pScanNode->LuboStartInfo),
                                        pScanNode->pChartMsg->ulScnAiCnt);
            }
            if (pScanNode->ucLuboStopTriggerType == FALLING_EDGE)
            {
                /* 若停止条件为下降沿,则停止录波 */
                SCI_Set_Lubo_Stop_Flag(pScanNode->pChartMsg->ulScnAiCnt);
            }
        }
        /* 保存此次的输入,供下次使用 */
        pScanNode->bLastScanInputValue=bCurInputValue;
    }
}