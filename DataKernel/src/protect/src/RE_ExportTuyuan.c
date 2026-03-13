/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_ExportTuyuan.C                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的端口引出元件的代码实现                       */
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
/*         张云       2005.11.1              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#include <vxWorks.h>

#include  "RE_ExportTuyuan.h"
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

EP_STATUS   RE_ExportTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                        TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
                                        NODE ** pRtElemInitNodePointer)
{
    NODE  *pNode;
    Export_Init_Node_Type    *pInitNode;
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
        LOG_Dbg_Msg("Export  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    pInitNode=(Export_Init_Node_Type    *)malloc(sizeof(Export_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("Export  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_EXPORT;
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
        /*读取输入信号类型  */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,pucCurInSignalType);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
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

    /* 设置该扫描节点的输出个数和信号类型 */
    pInitNode->PublicElemData.elem.ucOutNum=1;
    pInitNode->PublicElemData.elem.aioOut[0].ucAttrib
        =pInitNode->aucSourceSignalTypeArr[0];

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


    /* 读取4个保留字节  */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,4);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    /*读取端口引出原始值  */
    bReadSuccess=
        RE_ExportTuyuanGetExternExportDestInitValueFileReadInit
        (fp,pInitNode);
    if(!bReadSuccess)
    {
        assert(FALSE);
        return  EP_SYS_ERR;
    }

    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_ExportTuyuanReadFileOtherInit
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




/*　当外部输出目的地为端口引出时,读取原始值,并进行相应的初始化
    参数   fp  ,文件当前指针
           pElemInitNode,初始化节点指针
*/
BOOL   RE_ExportTuyuanGetExternExportDestInitValueFileReadInit(
    FILE  *fp,
    Export_Init_Node_Type *  pElemInitNode
)
{
    BOOL  bReadSuccess;
    EP_ELEM_IO  *  pCurElemOutput;
    unsigned  char   ucConstValue;
    unsigned  short    unConstValue;
    unsigned  long   ulConstValue;
    float   fConstValue;


    pCurElemOutput=pElemInitNode->PublicElemData.elem.aioOut;

    /*根据信号类型来读取相应数据,并将数据赋给输出 */
    if((pCurElemOutput->ucAttrib==XIANGBIE_SIGAL)
            ||(pCurElemOutput->ucAttrib==CONTROL_WORD_SIGNAL))
    {
        /* 若信号类型是相别或控制字 */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&ucConstValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /* 读取7字节保留 */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,7);
        if(!bReadSuccess)
        {
            return   FALSE;
        }

        /*  赋给输出,并进行输出初始化*/
        pCurElemOutput->now.ulVal=(unsigned  long)ucConstValue;
        pCurElemOutput->ucType=0xFF;
        pCurElemOutput->pvCh=NULL;
    }
    else  if(pCurElemOutput->ucAttrib==SHORT_INT_SIGNAL)
    {

        unsigned long  ulValue;
        bReadSuccess=ReadUnsignedShortFromResloveSeqFile
                     (fp,&unConstValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /* 读取6字节保留 */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,6);
        if(!bReadSuccess)
        {
            return   FALSE;
        }

        /*  赋给输出,并进行输出初始化*/
        ulValue=(unsigned  long )unConstValue;
        pCurElemOutput->now.lVal=(long)ulValue;
        pCurElemOutput->ucType=0xFF;
        pCurElemOutput->pvCh=NULL;
    }
    else  if(pCurElemOutput->ucAttrib==LONG_INT_SIGNAL)
    {
        bReadSuccess=ReadUnsignedLongFromResloveSeqFile
                     (fp,&ulConstValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /* 读取4字节保留 */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /*  赋给输出,并进行输出初始化*/
        pCurElemOutput->now.lVal=(long)ulConstValue;
        pCurElemOutput->ucType=0xFF;
        pCurElemOutput->pvCh=NULL;
    }

    else  if(pCurElemOutput->ucAttrib==HEX_MODE_WORD_SIGNAL)
    {
        /* 若是32位16进制方式字 */
        bReadSuccess=ReadUnsignedLongFromResloveSeqFile
                     (fp,&ulConstValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /* 读取4字节保留 */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /*  赋给输出,并进行输出初始化*/
        pCurElemOutput->now.ulVal=(unsigned long)ulConstValue;
        pCurElemOutput->ucType=0xFF;
        pCurElemOutput->pvCh=NULL;
    }
    else  if((pCurElemOutput->ucAttrib==COMPLEX_FORM_DIANLIU_SIGNAL)
             ||(pCurElemOutput->ucAttrib==VALUE_ANGLE_FORM_DIANLIU_SIGNAL)
             ||(pCurElemOutput->ucAttrib==COMPLEX_FORM_DIANLIU_SIGNAL_KA)
             ||(pCurElemOutput->ucAttrib==VALUE_ANGLE_FORM_DIANLIU_SIGNAL_KA)
             ||(pCurElemOutput->ucAttrib==COMPLEX_FORM_DIANYA_SIGNAL)
             ||(pCurElemOutput->ucAttrib==VALUE_ANGLE_FORM_DIANYA_SIGNAL)
             ||(pCurElemOutput->ucAttrib==COMPLEX_FORM_DIANYA_SIGNAL_KV)
             ||(pCurElemOutput->ucAttrib==VALUE_ANGLE_FORM_DIANYA_SIGNAL_KV)
             ||(pCurElemOutput->ucAttrib==COMPLEX_FORM_ZUKANG_SIGNAL)
             ||(pCurElemOutput->ucAttrib==VALUE_ANGLE_FORM_ZUKANG_SIGNAL)
             ||(pCurElemOutput->ucAttrib==COMPLEX_FORM_ZUKANG_SIGNAL_KO)
             ||(pCurElemOutput->ucAttrib==VALUE_ANGLE_FORM_ZUKANG_SIGNAL_KO))
    {
        /*读取实部  */

        float   fVirValue;
        bReadSuccess=ReadFloatFromResloveSeqFileInMotorolaType
                     (fp,&fConstValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /* 读取虚部 */
        bReadSuccess=ReadFloatFromResloveSeqFileInMotorolaType
                     (fp,&fVirValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /*  赋给输出,并进行输出初始化*/
        pCurElemOutput->now.xVal=fConstValue+fVirValue*1i;
        pCurElemOutput->ucType=0xFF;
        pCurElemOutput->pvCh=NULL;

    }
    else  if(pCurElemOutput->ucAttrib==LOGIC_SIGNAL)
    {
        /* 若是逻辑信号，  */

        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&ucConstValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }

        /* 读取7字节保留 */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,7);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        if(ucConstValue==1)
        {
            pCurElemOutput->now.bVal=TRUE;
        }
        else  if(ucConstValue==0)
        {
            pCurElemOutput->now.bVal=FALSE;
        }
        else
        {
            assert(FALSE);
            return  FALSE;

        }
        /*  赋给输出,并进行输出初始化*/
        pCurElemOutput->ucType=0xFF;
        pCurElemOutput->pvCh=NULL;

    }
    else
    {
        /*若是其他，则作为实数方式处理  */
        bReadSuccess=ReadFloatFromResloveSeqFileInMotorolaType
                     (fp,&fConstValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /* 读取4字节保留 */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /*  赋给输出,并进行输出初始化*/
        pCurElemOutput->now.fVal=fConstValue;
        pCurElemOutput->ucType=0xFF;
        pCurElemOutput->pvCh=NULL;
    }

    return  TRUE;

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

EP_STATUS   RE_ExportTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                    LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    Export_Init_Node_Type    *pInitNode;
    NODE  *  pCurSourceNode;
    int  i;
    uint8_t  * pucCurInSignalType;
    uint16_t   * punCurInSignalSourceSeqNo;
    uint8_t   *  pucCurInSignalSourceOutNo;
    EP_ELEM_IO *  pInElem;

    EP_STATUS  SetCurOutAttribResult;

    uint8_t   ucCurInSignalType;


    pInitNode=(Export_Init_Node_Type    *)pElemInitNode->pTuyuan;
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
            LOG_Dbg_Msg("error,Get  Export  Tuyuan  Input Source  Init  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }

        /* 获得并设置当前输入源的指针 */
        pInElem=(*  RE_GetScanNodeGetOutIOFunc(pCurSourceNode))
                ((*pucCurInSignalSourceOutNo),pCurSourceNode);

        /*  确保该图元输入在来源图元的输出号没有出界 */
        if(pInElem==NULL)
        {
            LOG_Dbg_Msg("error,Get  Export  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }
        /*  设置该IO指针到扫描节点相应的输入  */

        SetCurOutAttribResult=RE_ExportTuyuanSetInputIO
                              (i,pInElem,pElemScanNode);
        if(SetCurOutAttribResult!=EP_SUCCESS)
        {
            LOG_Dbg_Msg("error,Set  Export  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }

        /*比较信号类型是否匹配  */
        ucCurInSignalType=pInElem->ucAttrib;

        if(ucCurInSignalType!=(*pucCurInSignalType))
        {
            LOG_Dbg_Msg("error,Export Tuyuan One  Input  Signal Type  can't  Match!\n",0,0,0,0,0,0);


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
BOOL      RE_ExportTuyuanReadFileOtherInit
(Export_Init_Node_Type * pElemInitNodePointer)
{
    pElemInitNodePointer->PublicElemData.elem.ucType=RE_EXPORT;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_ExportTuyuanScanInit;

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

EP_STATUS   RE_ExportCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{

    NODE  *pNode;
    Export_Init_Node_Type    *pInitNode;
    ExternExportOuterOutput_Scan_Node_Type    *pScanNode;
    pInitNode=(Export_Init_Node_Type    *)pElemInitNode->pTuyuan;

    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("Export  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pScanNode=(ExternExportOuterOutput_Scan_Node_Type    *)
              malloc(sizeof(ExternExportOuterOutput_Scan_Node_Type));
    if(pScanNode==NULL)
    {
        LOG_Dbg_Msg("Export  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pNode->ulTuyuanType=
        RE_EXTERN_EXPORT_OUTEROUTPUT_SCAN;
    pNode->pTuyuan=(void  *)pScanNode;
    pScanNode->ioOut=pInitNode->
                     PublicElemData.elem.aioOut[0];
    /*2011-7-27  ZY  */
    pScanNode->pCollectOut=NULL;

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

EP_STATUS   RE_ExportTuyuanSetInputIO
(uint8_t  ucInputNum,EP_ELEM_IO *  pElemIO,
 NODE  * pScanNode)
{

    ExternExportOuterOutput_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(ExternExportOuterOutput_Scan_Node_Type    *)
                pScanNode->pTuyuan;

    if(ucInputNum>=1)
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Set  Export Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  EP_SYS_ERR;

    }
    pTuyuanNode->pInArr0=pElemIO;
    return   EP_SUCCESS;
}



/*
     功能:获得外部端口输出目的地图元的扫描节点的某个输出的指针

*/
/****参数：
           unOutNum,输出的序号
           pScanNode，扫描节点指针
*/
/*   返回值，相应序号的输出IO的指针 ，若失败，则返回NULL*/

EP_ELEM_IO *  RE_ExportTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode)
{

    ExternExportOuterOutput_Scan_Node_Type   *pTuyuanNode;
    pTuyuanNode=(ExternExportOuterOutput_Scan_Node_Type   *)
                pScanNode->pTuyuan;
    if(unOutNum>=1)
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Get  Export Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  NULL;

    }
    return   &(pTuyuanNode->ioOut);

}




/*   2011-8-8  ZY
     功能:获得外部端口输出图元集中后的扫描节点的输出指针(注意不是IO)

*/
/****参数：
           unOutNum,输出的序号
           pScanNode，扫描节点指针
*/
/*   返回值，相应序号的输出的指针 ，若失败，则返回NULL*/

void *  RE_ExportTuyuanGetCollectOut
(uint16_t   unOutNum,NODE  * pScanNode)
{

    ExternExportOuterOutput_Scan_Node_Type   *pTuyuanNode;
    pTuyuanNode=(ExternExportOuterOutput_Scan_Node_Type   *)
                pScanNode->pTuyuan;
    if(unOutNum>=1)
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Get  Export Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  NULL;

    }

    return   pTuyuanNode->pCollectOut;
}
