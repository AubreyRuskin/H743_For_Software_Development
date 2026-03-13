/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_DIModTuyuan.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的开入摸件元件的代码文件                        */
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
/*         张云       2005.11.13              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#include <vxWorks.h>

#include  "RE_DIModTuyuan.h"


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

EP_STATUS   RE_DIModTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                       TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
                                       NODE ** pRtElemInitNodePointer)
{

    NODE  *pNode;
    DIMod_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    int  i;
    BOOL  bOtherInitSuccess;
    unsigned  long  ulReadStrLen;
    EP_ELEM_IO  *  pCurElemOutput;
    void  * pvModHandle;

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
        LOG_Dbg_Msg("DIMod  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    pInitNode=(DIMod_Init_Node_Type    *)malloc(sizeof(DIMod_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("DIMod  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_DIMOD;
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


    /* 该扫描节点的输出个数 */
    pInitNode->PublicElemData.elem.ucOutNum=1;
    /*循环读取所有输出信息   */
    pCurElemOutput=pInitNode->PublicElemData.elem.aioOut;

    for(i=0; i<1; i++)
    {
        /* 读取4保留字节  */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        /* 设置输出信号类型为32位无符号方式字 */
        pCurElemOutput->ucAttrib=HEX_MODE_WORD_SIGNAL;

        bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                     (fp,pInitNode->strInputSourceIDArr[i],&ulReadStrLen);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        /* 获得DI摸件句柄  */
        pvModHandle=RD_Get_Handle(pInitNode->strInputSourceIDArr[i],RD_LGC_DI_MOD_HDL);

        if(pvModHandle==NULL)
        {
            LOG_Dbg_Msg("Get DIMod Tuyuan Mod logic ID \'%s\'   Handle  Error!\n"
                        ,(int)(pInitNode->strInputSourceIDArr[i]),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得开入摸件图元摸件逻辑标识\'%s\'的句柄错误\n"
                           ,(int)(pInitNode->strInputSourceIDArr[i]),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get di module   element mod id\'%s\' handle err\n"
                           ,(int)(pInitNode->strInputSourceIDArr[i]),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return   EP_SYS_ERR;
        }
        /* 赋初值给输出 */
        pCurElemOutput->now.ulVal=0;
        pCurElemOutput->ucType=3;/* DI模件 */
        pCurElemOutput->pvCh=pvModHandle;/* 将摸件句柄赋给属性 */

        pCurElemOutput++;
    }/*所有输出循环处理结束   */


    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_DIModTuyuanReadFileOtherInit
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

EP_STATUS   RE_DIModTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                   LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    return  EP_SUCCESS;

}


/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_DIModTuyuanReadFileOtherInit
(DIMod_Init_Node_Type * pElemInitNodePointer)
{
    pElemInitNodePointer->PublicElemData.elem.ucType=RE_DIMOD;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_DIModTuyuanScanInit;

    /* 设置获得IO输出的函数 */
    pElemInitNodePointer->PublicElemData.pfGetScanNodeOutIOFunc
        =RE_DIModTuyuanGetOutIO;

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

EP_STATUS   RE_DIModCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{
    NODE  *pNode;
    DIMod_Init_Node_Type    *pInitNode;
    DIMod_Scan_Node_Type    *pScanNode;
    int  i;

    pInitNode=(DIMod_Init_Node_Type    *)pElemInitNode->pTuyuan;

    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("DIMod  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pScanNode=(DIMod_Scan_Node_Type    *)malloc
              (sizeof(DIMod_Scan_Node_Type));
    if(pScanNode==NULL)
    {
        LOG_Dbg_Msg("DIMod  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pNode->ulTuyuanType=RE_DI_MOD_SCAN;
    pNode->pTuyuan=(void  *)pScanNode;
    for(i=0; i<1; i++)
    {
        pScanNode->ioOut=pInitNode->
                         PublicElemData.elem.aioOut[i];
    }
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

EP_ELEM_IO *  RE_DIModTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode)
{
    DIMod_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(DIMod_Scan_Node_Type    *)pScanNode->pTuyuan;
    if(unOutNum>=1)
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Get DIMod Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  NULL;
    }
    return  &(pTuyuanNode->ioOut);
}




