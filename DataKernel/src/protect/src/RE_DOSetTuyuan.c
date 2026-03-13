/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_DOSetTuyuan.C                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的DO输出集元件的代码实现                       */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                    */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                */
/*                                                                              */
/*         作者           日期                    说明                          */
/*                                                                              */
/*         张云       2005.11.9              创建文件1.0版本                    */

/*                                                                              */
/********************************************************************************/

#include <vxWorks.h>

#include  "RE_DOSetTuyuan.h"
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

EP_STATUS   RE_DOSetTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                       TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
                                       NODE ** pRtElemInitNodePointer)
{
    NODE  *pNode;
    DOSet_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    int  i;
    unsigned  short   *  punCurInSourceSeqNo;
    unsigned  char   *   pucInSourceOutputNo;

    unsigned  long  ulIDStrLen;
    BOOL  bOtherInitSuccess;
    unsigned  char       nTempChar;
    int   iTest;

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
        LOG_Dbg_Msg("DOSet  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    pInitNode=(DOSet_Init_Node_Type    *)malloc(sizeof(DOSet_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("DOSet  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_DOSET;
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
    punCurInSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucInSourceOutputNo=pInitNode->aucInSourceOutputNumArr;

    for(i=0; i<1; i++)
    {
        /* 读取信号来源   */
        bReadSuccess=ReadUnsignedShortFromResloveSeqFile
                     (fp,punCurInSourceSeqNo);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        iTest=(*punCurInSourceSeqNo)+1;

        /* 读取信号源输出号  */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,pucInSourceOutputNo);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        iTest=*pucInSourceOutputNo;

        /* 读取4个保留字节  */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        /* 为了提高效率,循环使用指针操作   */
        punCurInSourceSeqNo++;
        pucInSourceOutputNo++;
    }/*所有输入循环处理结束  */


    /* 读取DO个数*/

    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&(nTempChar));
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    assert(nTempChar<=MAX_DOSET_INPUT_COUNT&&nTempChar>0);

    pInitNode->iDOCnt=nTempChar;
    /* 设置该扫描节点的输出个数 */
    pInitNode->PublicElemData.elem.ucOutNum=0;

    for(i=0; i<pInitNode->iDOCnt; i++)
    {
        /*  读取输出信号目的地逻辑标识 */
        bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                     (fp,pInitNode->strOutputDestIDArr[i],&ulIDStrLen);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        pInitNode->pvDestHandleArr[i]=
            RD_Get_Handle(pInitNode->strOutputDestIDArr[i],RD_LGC_DO_HDL);

        if((pInitNode->pvDestHandleArr[i])==NULL)
        {
            LOG_Dbg_Msg("Get DOSet Tuyuan \'%s\' Handle Error!\n"
                        ,(int)(pInitNode->strOutputDestIDArr[i]),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得DO输出集图元ID号为\'%s\' DO的句柄错误\n"
                           ,(int)(pInitNode->strOutputDestIDArr[i]),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get doset   element  id\'%s\' DO handle  err\n"
                           ,(int)(pInitNode->strOutputDestIDArr[i]),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return   EP_SYS_ERR;
        }


        /* 读取4个保留字节  */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
    }

    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_DOSetTuyuanReadFileOtherInit
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

EP_STATUS   RE_DOSetTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                   LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    DOSet_Init_Node_Type    *pInitNode;
    NODE  *  pCurSourceNode;
    int  i;
    uint16_t   * punCurInSignalSourceSeqNo;
    uint8_t   *  pucCurInSignalSourceOutNo;
    EP_ELEM_IO *  pInElem;

    EP_STATUS  SetCurOutAttribResult;

    uint8_t   ucCurInSignalType;


    pInitNode=(DOSet_Init_Node_Type    *)pElemInitNode->pTuyuan;
    punCurInSignalSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucCurInSignalSourceOutNo=pInitNode->aucInSourceOutputNumArr;
    /* 循环设定图元的输入源指针  */
    for(i=0; i<(pInitNode->PublicElemData.elem.unInNum); i++)
    {
        int   itemp;
        pCurSourceNode=RE_LstNth(pGrpScanNodeList,
                                 ((*punCurInSignalSourceSeqNo)+1));/* 注意序号从1开始 */

        itemp=(*punCurInSignalSourceSeqNo)+1;

        if(pCurSourceNode==NULL)
        {
            LOG_Dbg_Msg("error,Get  DOSet  Tuyuan  Input Source  Init  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        /* 获得并设置当前输入源的指针 */
        pInElem=(*  RE_GetScanNodeGetOutIOFunc(pCurSourceNode))
                ((*pucCurInSignalSourceOutNo),pCurSourceNode);

        /*  确保该图元输入在来源图元的输出号没有出界 */
        if(pInElem==NULL)
        {

            LOG_Dbg_Msg("error,Get  DOSet  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }
        /*  设置该IO指针到扫描节点相应的输入  */

        SetCurOutAttribResult=RE_DOSetTuyuanSetInputIO
                              (i,pInElem,pElemScanNode);
        if(SetCurOutAttribResult!=EP_SUCCESS)
        {
            LOG_Dbg_Msg("error,Set  DOSet  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        /*比较信号类型是否匹配  */

        ucCurInSignalType=pInElem->ucAttrib;

        if(ucCurInSignalType!=LOGIC_SIGNAL)
        {
            LOG_Dbg_Msg("error,DOSet Tuyuan One  Input  Signal Type  can't  Match!\n",0,0,0,0,0,0);
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        punCurInSignalSourceSeqNo++;
        pucCurInSignalSourceOutNo++;
    }
    return  EP_SUCCESS;
}





/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_DOSetTuyuanReadFileOtherInit
(DOSet_Init_Node_Type * pElemInitNodePointer)
{
    pElemInitNodePointer->PublicElemData.elem.ucType=RE_DOSET;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_DOSetTuyuanScanInit;

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

EP_STATUS   RE_DOSetCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{

    NODE  *pNode;
    DOSet_Init_Node_Type    *pInitNode;
    DOSet_Scan_Node_Type   *pScanNode;
    int  i;

    pInitNode=(DOSet_Init_Node_Type    *)pElemInitNode->pTuyuan;
    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("DOSet  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pScanNode=(DOSet_Scan_Node_Type   *)
              malloc
              (sizeof(DOSet_Scan_Node_Type));
    if(pScanNode==NULL)
    {
        LOG_Dbg_Msg("DOSet  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }

    pNode->ulTuyuanType=RE_DO_SET_SCAN;
    pNode->pTuyuan=(void  *)pScanNode;

    pScanNode->iDOCnt=pInitNode->iDOCnt;
    for(i=0; i<pInitNode->iDOCnt; i++)
    {
        pScanNode->pvDestHandleArr[i]=pInitNode->
                                      pvDestHandleArr[i];
        pScanNode->bLastValueArr[i]=FALSE;
    }
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

EP_STATUS   RE_DOSetTuyuanSetInputIO
(uint8_t  ucInputNum,EP_ELEM_IO *  pElemIO,
 NODE  * pScanNode)
{

    DOSet_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(DOSet_Scan_Node_Type    *)pScanNode->pTuyuan;

    if(ucInputNum>=1)
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Set DOSet Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  EP_SYS_ERR;

    }
    pTuyuanNode->pInArr0=pElemIO;
    return   EP_SUCCESS;
}


/* 扫描多个DO集节点.
 * Para:
 *     PartGrpAttrib, 分图属性.
 * Return:
 *     NONE.
 */
void RE_MultiDOSetTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib)
{
    NODE **ppCurDOSetNode;
    int m;
    BOOL bCurValue;
    int iCurValue;
    int i;
    DOSet_Scan_Node_Type *pScanNode;
    RD_LGC_DO_CH *plgcdo;
    int iDOCnt;
    void **ppvDestHandle;
    BOOL bLastValue;
    BOOL bIsAdvance;
    BOOL bIsFall;

    ppCurDOSetNode = PartGrpAttrib->ppDOSetNode;
    for (m = 0; m<PartGrpAttrib->DOSetScanNodeNum; m++, ppCurDOSetNode++)
    {
        pScanNode = (DOSet_Scan_Node_Type *)(*ppCurDOSetNode)->pTuyuan;

        /* 一个输入多个输出，上一个输入只有一种状态 */
        /*		if (pScanNode->pInArr0->now.bVal == pScanNode->bLastValueArr[0])
        		{
        			continue;
        		}
        */
        iDOCnt = pScanNode->iDOCnt;
        ppvDestHandle = pScanNode->pvDestHandleArr;

        if((pScanNode->pInArr0->ucAttrib==SHORT_INT_SIGNAL)||(pScanNode->pInArr0->ucAttrib==LONG_INT_SIGNAL))
        {
            iCurValue = pScanNode->pInArr0->now.lVal;

            for (i = 0; i<iDOCnt; i++)
            {
                plgcdo = (RD_LGC_DO_CH *)(*ppvDestHandle);

                taskLock();

                RD_Set_DO(plgcdo, iCurValue);

                taskUnlock();

                ppvDestHandle++;
            }
        }
        else
        {
            bCurValue = pScanNode->pInArr0->now.bVal;

            if(bCurValue&0x80)
            {
                for (i = 0; i<iDOCnt; i++)
                {
                    plgcdo = (RD_LGC_DO_CH *)(*ppvDestHandle);

                    taskLock();

                    RD_Set_DO(plgcdo, bCurValue&0x7F);

                    taskUnlock();

                    ppvDestHandle++;
                }
            }
            else if((bCurValue==DP_TRUE)||(bCurValue==DP_FALSE)
                    ||(bCurValue==DP_INVALID_11)||(bCurValue==DP_INVALID_00))
            {
                for (i = 0; i<iDOCnt; i++)
                {
                    plgcdo = (RD_LGC_DO_CH *)(*ppvDestHandle);

                    taskLock();

                    RD_Set_DO(plgcdo, bCurValue);

                    taskUnlock();

                    ppvDestHandle++;
                }
            }
            else
            {
                bLastValue = pScanNode->bLastValueArr[0];

                bIsAdvance = (!(bLastValue)) && bCurValue;
                bIsFall = bLastValue && (!(bCurValue));

                for (i = 0; i<iDOCnt; i++)
                {
                    plgcdo = (RD_LGC_DO_CH *)(*ppvDestHandle);

                    taskLock();

                    if (bIsAdvance)
                    {
                        /* 上升沿 */
                        (plgcdo->iTripDOCnt)++;
                    }
                    else if (bIsFall)
                    {
                        /* 下降沿 */
                        --(plgcdo->iTripDOCnt);
                    }
                    if (plgcdo->iTripDOCnt>0)
                    {
                        RD_Set_DO(plgcdo, TRUE);
                    }
                    else
                    {
                        RD_Set_DO(plgcdo, FALSE);
                    }

                    taskUnlock();

                    ppvDestHandle++;
                }
                pScanNode->bLastValueArr[0] = bCurValue;
            }
        }
    }
}

/*    DO输出集目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_DOSetTuyuanScan(NODE *pElemScanNode)
{
    DOSet_Scan_Node_Type    *  pDOTuyuan;
    RD_LGC_DO_CH *plgcdo;
    BOOL   bCurValue;
    int iCurValue;
    int  i;
    int iDOCnt;
    void  **ppvDestHandle;
    BOOL   *pbLastValue;

    pDOTuyuan=(DOSet_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    iDOCnt=pDOTuyuan->iDOCnt;


    if((pDOTuyuan->pInArr0->ucAttrib==SHORT_INT_SIGNAL)||(pDOTuyuan->pInArr0->ucAttrib==LONG_INT_SIGNAL))
    {
        iCurValue=pDOTuyuan->pInArr0->now.lVal;
        ppvDestHandle=pDOTuyuan->pvDestHandleArr;
        for(i=0; i<iDOCnt; i++)
        {
            plgcdo=(RD_LGC_DO_CH*)(*ppvDestHandle);

            taskLock();

            RD_Set_DO(plgcdo,iCurValue);

            taskUnlock();

            ppvDestHandle++;

        }
    }
    else
    {
        bCurValue=pDOTuyuan->pInArr0->now.bVal;

        if(bCurValue&0x80)
        {
            ppvDestHandle=pDOTuyuan->pvDestHandleArr;
            for(i=0; i<iDOCnt; i++)
            {
                plgcdo=(RD_LGC_DO_CH*)(*ppvDestHandle);

                taskLock();

                RD_Set_DO(plgcdo,bCurValue&0x7F);

                taskUnlock();

                ppvDestHandle++;

            }
        }
        else if((bCurValue==DP_TRUE)||(bCurValue==DP_FALSE)
                ||(bCurValue==DP_INVALID_11)||(bCurValue==DP_INVALID_00))
        {
            ppvDestHandle=pDOTuyuan->pvDestHandleArr;
            for(i=0; i<iDOCnt; i++)
            {
                plgcdo=(RD_LGC_DO_CH*)(*ppvDestHandle);

                taskLock();

                RD_Set_DO(plgcdo,bCurValue);

                taskUnlock();

                ppvDestHandle++;

            }
        }
        else
        {
            ppvDestHandle=pDOTuyuan->pvDestHandleArr;
            pbLastValue=pDOTuyuan->bLastValueArr;

            for(i=0; i<iDOCnt; i++)
            {
                plgcdo=(RD_LGC_DO_CH*)(*ppvDestHandle);

                taskLock();

                if((!(*pbLastValue))&&bCurValue)
                {
                    /*上升沿  */
                    (plgcdo->iTripDOCnt)++;
                }
                else if((*pbLastValue)&&(!bCurValue))
                {
                    /*下降沿  */
                    --(plgcdo->iTripDOCnt);
                }
                if(plgcdo->iTripDOCnt>0)
                {
                    RD_Set_DO(plgcdo,TRUE);
                }
                else
                {
                    RD_Set_DO(plgcdo,FALSE);
                }

                taskUnlock();

                *pbLastValue=bCurValue;

                pbLastValue++;
                ppvDestHandle++;

            }
        }
    }
}
