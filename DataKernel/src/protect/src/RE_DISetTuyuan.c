/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_DISetTuyuan.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的开入集元件的代码文件                        */
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

#include  "RE_DISetTuyuan.h"


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

EP_STATUS   RE_DISetTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                       TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
                                       NODE ** pRtElemInitNodePointer)
{

    NODE  *pNode;
    DISet_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    int  i;
    BOOL  bOtherInitSuccess;
    unsigned  long  ulReadStrLen;
    EP_ELEM_IO  *  pCurElemOutput;
    void  *  pvDiHandle;

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
        LOG_Dbg_Msg("DISet  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    pInitNode=(DISet_Init_Node_Type    *)malloc(sizeof(DISet_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("DISet  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_DISET;
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
    /*赋值输入个数给节点  */
    pInitNode->PublicElemData.elem.unInNum=0;

    /* 读取1字节输出个数 */
    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&(pInitNode->ucOutputCnt));
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    assert(pInitNode->ucOutputCnt<=MAX_DISET_OUTPUT_COUNT&&pInitNode->ucOutputCnt>0);

    /*赋值输出给节点 ， */

    /* 该扫描节点的输出个数 */
    pInitNode->PublicElemData.elem.ucOutNum=pInitNode->ucOutputCnt;
    /*循环读取所有输出信息   */
    pCurElemOutput=pInitNode->PublicElemData.elem.aioOut;

    for(i=0; i<pInitNode->ucOutputCnt; i++)
    {
        /* 设置输出信号类型 */
        pCurElemOutput->ucAttrib=LOGIC_SIGNAL;

        bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                     (fp,pInitNode->strInputSourceIDArr[i],&ulReadStrLen);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        /* 获得DI句柄  */
        pvDiHandle=RD_Get_Handle(pInitNode->strInputSourceIDArr[i],RD_LGC_DI_HDL);

        if(pvDiHandle==NULL)
        {
            LOG_Dbg_Msg("Get DISet Tuyuan DI logic ID \'%s\'   Handle  Error!\n"
                        ,(int)(pInitNode->strInputSourceIDArr[i]),0,0,0,0,0);
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得开入集图元DI逻辑标识\'%s\'的句柄错误\n"
                           ,(int)(pInitNode->strInputSourceIDArr[i]),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get diset   element  di id\'%s\' handle err\n"
                           ,(int)(pInitNode->strInputSourceIDArr[i]),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return   EP_SYS_ERR;
        }
        /* 赋初值给输出 */
        pCurElemOutput->now.bVal=FALSE;
        pCurElemOutput->ucType=1;/* DI通道 */
        pCurElemOutput->pvCh=pvDiHandle;/* 将DI句柄赋给属性 */
        pInitNode->DISourceChOffsetArr[i] = RD_DI_Ofst(pvDiHandle);

        /* 读取4保留字节  */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        pCurElemOutput++;

    }/*所有输出循环处理结束   */


    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_DISetTuyuanReadFileOtherInit
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

EP_STATUS   RE_DISetTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                   LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    return  EP_SUCCESS;

}


/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_DISetTuyuanReadFileOtherInit
(DISet_Init_Node_Type * pElemInitNodePointer)
{
    pElemInitNodePointer->PublicElemData.elem.ucType=RE_DISET;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_DISetTuyuanScanInit;

    /* 设置获得IO输出的函数 */
    pElemInitNodePointer->PublicElemData.pfGetScanNodeOutIOFunc
        =RE_DISetTuyuanGetOutIO;

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

EP_STATUS   RE_DISetCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{
    NODE  *pNode;
    DISet_Init_Node_Type    *pInitNode;
    DISet_Scan_Node_Type    *pScanNode;
    int  i;

    pInitNode=(DISet_Init_Node_Type    *)pElemInitNode->pTuyuan;

    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("DISet  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pScanNode=(DISet_Scan_Node_Type    *)malloc
              (sizeof(DISet_Scan_Node_Type));
    if(pScanNode==NULL)
    {
        LOG_Dbg_Msg("DISet  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pNode->ulTuyuanType=RE_DI_SET_SCAN;
    pNode->pTuyuan=(void  *)pScanNode;
    for(i=0; i<pInitNode->ucOutputCnt; i++)
    {
        pScanNode->ioOutArr[i]=pInitNode->
                               PublicElemData.elem.aioOut[i];
        /*2011-7-27  zy  */
        pScanNode->apCollectOutArr[i]=NULL;
        pScanNode->DISourceChOffsetArr[i] = pInitNode->DISourceChOffsetArr[i];
    }
    pScanNode->ucOutputCnt_8=(pInitNode->ucOutputCnt)/8;
    pScanNode->ucOutputCnt_m=(pInitNode->ucOutputCnt)%8;
    pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                      PublicElemData.pfGetScanNodeOutIOFunc;
    pScanNode->pchart=pInitNode->
                      PublicElemData.elem.pchart;
    /* 设定返回的节点地址  */
    *pReturnElemScanNodePointer=(NODE  *)pNode;
    return  EP_SUCCESS;
}


/*
     功能:获得图元的扫描节点的某个输出的指针

*/
/****参数：
           unOutNum,输出的序号
           pScanNode，扫描节点指针
*/
/*   返回值，相应序号的输出IO的指针 ，若失败，则返回NULL*/

EP_ELEM_IO *  RE_DISetTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode)
{
    DISet_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(DISet_Scan_Node_Type    *)pScanNode->pTuyuan;
    if(unOutNum>=(pTuyuanNode->ucOutputCnt_8*8+pTuyuanNode->ucOutputCnt_m))
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Get DISet Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  NULL;
    }
    /* 2011-7-27  ZY */
    if(pTuyuanNode->apCollectOutArr[unOutNum])
    {
        return   pTuyuanNode->apCollectOutArr[unOutNum];
    }
    else
    {
        return   &(pTuyuanNode->ioOutArr[unOutNum]);
    }
}


/*    DI集图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

void     RE_DISetTuyuanScan(NODE *pElemScanNode)
{
    DISet_Scan_Node_Type    *pTuyuanNode;
    EP_ELEM_IO  * pCurOut;
    uint32_t *pChOff;
    int i;
    BOOL *pDIBase;

    pTuyuanNode=(DISet_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    pCurOut=pTuyuanNode->ioOutArr;
    pChOff = pTuyuanNode->DISourceChOffsetArr;
    pDIBase = RD_Base_His_DI_P(pTuyuanNode->pchart->ulScnAiCnt);
    pCurOut--;
    pChOff--;

    for(i=0; i<pTuyuanNode->ucOutputCnt_8; i++)
    {
        (++pCurOut)->now.bVal = *(pDIBase+*(++pChOff));
        (++pCurOut)->now.bVal = *(pDIBase+*(++pChOff));
        (++pCurOut)->now.bVal = *(pDIBase+*(++pChOff));
        (++pCurOut)->now.bVal = *(pDIBase+*(++pChOff));
        (++pCurOut)->now.bVal = *(pDIBase+*(++pChOff));
        (++pCurOut)->now.bVal = *(pDIBase+*(++pChOff));
        (++pCurOut)->now.bVal = *(pDIBase+*(++pChOff));
        (++pCurOut)->now.bVal = *(pDIBase+*(++pChOff));
    }
    for(i=0; i<pTuyuanNode->ucOutputCnt_m; i++)
    {
        (++pCurOut)->now.bVal = *(pDIBase+*(++pChOff));
    }
}
