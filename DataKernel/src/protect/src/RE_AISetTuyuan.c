/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_AISetTuyuan.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的AI输入集元件的代码文件                        */
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
/*         张云       2005.11.8              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/


#include <vxWorks.h>

#include  "RE_AISetTuyuan.h"


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

EP_STATUS   RE_AISetTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                       TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
                                       NODE ** pRtElemInitNodePointer)
{

    NODE  *pNode;
    AISet_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    uint8_t   ucSignalType;	/* 信号输入类型 */
    int  i;
    BOOL  bOtherInitSuccess;

    EP_ELEM_IO  *  pCurElemOutput;
    void  *  pvAiHandle;
    uint8_t  ucHandleAttrib;
    unsigned long  *pulAISourceChOffset;
    unsigned  long  ulReadStrLen;
    uint8_t   ucTemp;

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
        LOG_Dbg_Msg("AISet  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    pInitNode=(AISet_Init_Node_Type    *)malloc(sizeof(AISet_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("AISet  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_AISET;
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

    /* 读取1字节AI属性*/
    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&(ucTemp));
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    if(ucTemp==0)
    {
        pInitNode->ucSignalValueType=FLOAT_VALUE_TYPE;
    }
    else  if(ucTemp==1)
    {
        pInitNode->ucSignalValueType=COMPLEX_VALUE_TYPE;
    }
    else
    {
        assert(FALSE);
    }
    /* 读取3保留字节  */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,3);
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
    assert(pInitNode->ucOutputCnt<=MAX_AISET_OUTPUT_COUNT&&pInitNode->ucOutputCnt>0);

    /*赋值输出给节点 ， */

    /* 该扫描节点的输出个数 */
    pInitNode->PublicElemData.elem.ucOutNum=pInitNode->ucOutputCnt;
    /*循环读取所有输出信息   */
    pCurElemOutput=pInitNode->PublicElemData.elem.aioOut;
    pulAISourceChOffset=pInitNode->AISourceChOffsetArr;
    for(i=0; i<pInitNode->ucOutputCnt; i++)
    {
        /* 设置输出信号类型 */

        /* 读取信号类型  */

        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&ucSignalType);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                     (fp,pInitNode->strInputSourceIDArr[i],&ulReadStrLen);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        /* 获得AI句柄  */
        pvAiHandle=RD_Get_Handle(pInitNode->strInputSourceIDArr[i],RD_LGC_AI_HDL);

        if(pvAiHandle==NULL)
        {
            LOG_Dbg_Msg("Get AISet diagram element AI logic ID \'%s\'   Handle  Error!\n"
                        ,(int)(pInitNode->strInputSourceIDArr[i]),0,0,0,0,0);
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误,获得AI输入集图元AI逻辑标识\'%s\'的句柄错误\n"
                           ,(int)(pInitNode->strInputSourceIDArr[i]),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic diagram rslv err,get AISet  metafile ai logic id \'%s\' handle err\n"
                           ,(int)(pInitNode->strInputSourceIDArr[i]),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return   EP_SYS_ERR;
        }

        /*判定信号类型是否一致  */
        ucHandleAttrib=AI_HND_TO_UNIT(pvAiHandle);

        //todo
        if(ucHandleAttrib!=ucSignalType)
        {
            LOG_Dbg_Msg(" AISet   element  AI logic  ID\'%s\' Signal  Type  isn't  Match  with  Config  Handle!\n"
                        ,(int)(pInitNode->strInputSourceIDArr[i]),0,0,0,0,0);
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误,AI输入集图元ID号为\'%s\'的AI通道信号类型同配置不一致\n"
                           ,(int)(pInitNode->strInputSourceIDArr[i]),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err, AISet  element ia logic id \'%s\' signal type is not match witch config\n"
                           ,(int)(pInitNode->strInputSourceIDArr[i]),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return   EP_SYS_ERR;

        }

        /* 若若AI集是实数式AI集,且信号类型是实数式电流,电压,阻抗,则是*/
        if((pInitNode->ucSignalValueType==FLOAT_VALUE_TYPE)
                &&((ucSignalType==REAL_FORM_DIANLIU_SIGNAL)
                   ||(ucSignalType==REAL_FORM_DIANLIU_SIGNAL_KA)
                   ||(ucSignalType==REAL_FORM_DIANLIU_SIGNAL_MA)
                   ||(ucSignalType==REAL_FORM_DIANYA_SIGNAL)
                   ||(ucSignalType==REAL_FORM_DIANYA_SIGNAL_KV)
                   ||(ucSignalType==REAL_FORM_ZUKANG_SIGNAL)
                   ||(ucSignalType==REAL_FORM_ZUKANG_SIGNAL_KO)
                  ))
        {
            /* 判定能否访问第2个采样节拍的物理AI通道数据指针 */
            float  *pfLgcAi;
            /* 获得访问AI数据指针 */
            pfLgcAi=RD_Lgc_AI_P(pvAiHandle, (RD_AI_Cnt()-10));/* 这里要求实时数据模块实现时
                                                  返回非空,出错,则返回空 */
            if(pfLgcAi==NULL)
            {
                LOG_Dbg_Msg(" AISet  Tuyuan  AI logic  ID\'%s\' Data  Error  By  Handle!\n"
                            ,(int)(pInitNode->strInputSourceIDArr[i]),0,0,0,0,0);
                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:AI输入集图元ID号为\'%s\'的数据错误\n"
                               ,(int)(pInitNode->strInputSourceIDArr[i]),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err: AISet  element id \'%s\' data err\n"
                               ,(int)(pInitNode->strInputSourceIDArr[i]),0);
                }
                assert(FALSE);  /* 若非以上类型,则告警   */
                return   EP_SYS_ERR;
            }


            /* 赋初值给输出 */
            pCurElemOutput->ucAttrib=ucSignalType;
            pCurElemOutput->now.fVal=0.0;
            pCurElemOutput->ucType=0;/* AI通道 */
            pCurElemOutput->pvCh=pvAiHandle;/* 将AI句柄赋给属性 */
            *pulAISourceChOffset=
                RD_Lgc_AI_Ofst(pvAiHandle);
        }
        /* 否则信号类型若是复数式和幅角式电流,电压,阻抗,则是滤波逻辑处理通道 */
        else  if(pInitNode->ucSignalValueType==COMPLEX_VALUE_TYPE
                 &&((ucSignalType==COMPLEX_FORM_DIANLIU_SIGNAL)
                    ||(ucSignalType==VALUE_ANGLE_FORM_DIANLIU_SIGNAL)
                    ||(ucSignalType==COMPLEX_FORM_DIANLIU_SIGNAL_KA)
                    ||(ucSignalType==VALUE_ANGLE_FORM_DIANLIU_SIGNAL_KA)
                    ||(ucSignalType==COMPLEX_FORM_DIANYA_SIGNAL)
                    ||(ucSignalType==VALUE_ANGLE_FORM_DIANYA_SIGNAL)
                    ||(ucSignalType==COMPLEX_FORM_DIANYA_SIGNAL_KV)
                    ||(ucSignalType==VALUE_ANGLE_FORM_DIANYA_SIGNAL_KV)
                    ||(ucSignalType==COMPLEX_FORM_ZUKANG_SIGNAL)
                    ||(ucSignalType==VALUE_ANGLE_FORM_ZUKANG_SIGNAL)
                    ||(ucSignalType==COMPLEX_FORM_ZUKANG_SIGNAL_KO)
                    ||(ucSignalType==VALUE_ANGLE_FORM_ZUKANG_SIGNAL_KO)
                   ))
        {
            /*判定能否访问第2个采样节拍的预处理AI数据指针  */
            COMPLEX *  pxCalcAi;
            pxCalcAi=RD_Calc_AI_P(pvAiHandle, (RD_AI_Cnt()-10));

            if(pxCalcAi==NULL)
            {
                LOG_Dbg_Msg(" AISet  Tuyuan  AI logic  ID\'%s\' Data  Error  By  Handle!\n"
                            ,(int)(pInitNode->strInputSourceIDArr[i]),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:AI输入集图元ID号为\'%s\'的数据错误\n"
                               ,(int)(pInitNode->strInputSourceIDArr[i]),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err: AISet  element id \'%s\' data err\n"
                               ,(int)(pInitNode->strInputSourceIDArr[i]),0);
                }
                // todo
                assert(FALSE);  /* 若非以上类型,则告警   */
                return   EP_SYS_ERR;
            }
            /* 赋初值给输出 */
            pCurElemOutput->ucAttrib=ucSignalType;
            pCurElemOutput->now.xVal=0.0+0.0i;
            pCurElemOutput->ucType=0;/* AI通道 */
            pCurElemOutput->pvCh=pvAiHandle;/* 将AI句柄赋给属性 */
            *pulAISourceChOffset=
                RD_Calc_AI_Ofst(pvAiHandle);

        }
        else
        {
            LOG_Dbg_Msg("Error,  AISet  Tuyuan AI  logic ID  \'%s\'   Signal  Type  isn't  Expected!\n"
                        ,(int)(pInitNode->strInputSourceIDArr[i]),0,0,0,0,0);
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得AI输入集图元逻辑标识为\'%s\'的AI通道信号类型不对,不是预期类型\n"
                           ,(int)(pInitNode->strInputSourceIDArr[i]),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get AISet  element ai logic id \'%s\' signal type is not  expect type\n"
                           ,(int)(pInitNode->strInputSourceIDArr[i]),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return   EP_SYS_ERR;
        }
        /* 读取4保留字节  */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        pCurElemOutput++;
        pulAISourceChOffset++;

    }/*所有输出循环处理结束   */


    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_AISetTuyuanReadFileOtherInit
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

EP_STATUS   RE_AISetTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                   LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    return  EP_SUCCESS;

}


/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_AISetTuyuanReadFileOtherInit
(AISet_Init_Node_Type * pElemInitNodePointer)
{
    pElemInitNodePointer->PublicElemData.elem.ucType=RE_AISET;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_AISetTuyuanScanInit;

    /* 设置获得IO输出的函数 */
    pElemInitNodePointer->PublicElemData.pfGetScanNodeOutIOFunc
        =RE_AISetTuyuanGetOutIO;

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

EP_STATUS   RE_AISetCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{
    NODE  *pNode;
    AISet_Init_Node_Type    *pInitNode;
    AISet_Scan_Node_Type    *pScanNode;
    int  i;

    pInitNode=(AISet_Init_Node_Type    *)pElemInitNode->pTuyuan;
    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("AISet  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pScanNode=(AISet_Scan_Node_Type    *)
              malloc
              (sizeof(AISet_Scan_Node_Type));
    if(pScanNode==NULL)
    {
        LOG_Dbg_Msg("AISet  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    if(pInitNode->ucSignalValueType==FLOAT_VALUE_TYPE)
    {
        pNode->ulTuyuanType=RE_FLOAT_AI_SET_SCAN;
    }
    else  if(pInitNode->ucSignalValueType==COMPLEX_VALUE_TYPE)
    {
        pNode->ulTuyuanType=RE_CMPLX_AI_SET_SCAN;
    }
    else
    {
        assert(FALSE);
    }

    pNode->pTuyuan=(void  *)pScanNode;

    for(i=0; i<pInitNode->ucOutputCnt; i++)
    {
        pScanNode->ioOutArr[i]=pInitNode->
                               PublicElemData.elem.aioOut[i];
        /*2011-7-27  zy  */
        pScanNode->apCollectOutArr[i]=NULL;
        pScanNode->AISourceChOffsetArr[i]=pInitNode->
                                          AISourceChOffsetArr[i];
    }
    pScanNode->ucOutputCnt_8=pInitNode->ucOutputCnt/8;
    pScanNode->ucOutputCnt_m=pInitNode->ucOutputCnt%8;

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

EP_ELEM_IO *  RE_AISetTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode)
{

    AISet_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(AISet_Scan_Node_Type    *)pScanNode->pTuyuan;
    if(unOutNum>=((pTuyuanNode->ucOutputCnt_8)*8+(pTuyuanNode->ucOutputCnt_m)))
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Get AISet Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
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


/*    FLOAT型的AI来源的输入集图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_FloatAISetTuyuanScan(NODE *pElemScanNode)
{
    /* 访问当前采样节拍的物理AI通道数据指针 */

    AISet_Scan_Node_Type    *pTuyuanNode;
    EP_ELEM_IO  * pCurOut;
    unsigned long *pulChOffset;
    int i;
    float   *pfBase;
    int  iOutputCnt_8;
    int  iOutputCnt_m;

    pTuyuanNode=(AISet_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    pfBase=pTuyuanNode->pchart->pfBase;
    iOutputCnt_8=pTuyuanNode->ucOutputCnt_8;
    iOutputCnt_m=pTuyuanNode->ucOutputCnt_m;
    pCurOut=pTuyuanNode->ioOutArr;
    pulChOffset=pTuyuanNode->AISourceChOffsetArr;
    pCurOut--;
    pulChOffset--;
    for(i=0; i<iOutputCnt_8; i++)
    {
        (++pCurOut)->now.fVal=*(pfBase+*++pulChOffset);
        (++pCurOut)->now.fVal=*(pfBase+*++pulChOffset);
        (++pCurOut)->now.fVal=*(pfBase+*++pulChOffset);
        (++pCurOut)->now.fVal=*(pfBase+*++pulChOffset);
        (++pCurOut)->now.fVal=*(pfBase+*++pulChOffset);
        (++pCurOut)->now.fVal=*(pfBase+*++pulChOffset);
        (++pCurOut)->now.fVal=*(pfBase+*++pulChOffset);
        (++pCurOut)->now.fVal=*(pfBase+*++pulChOffset);
    }
    for(i=0; i<iOutputCnt_m; i++)
    {
        (++pCurOut)->now.fVal=*(pfBase+*++pulChOffset);
    }
}

/*    Complex型的AI来源的输入集图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_ComplexAISetTuyuanScan(NODE *pElemScanNode)
{
    /* 访问当前采样节拍的物理AI通道数据指针 */

    AISet_Scan_Node_Type    *pTuyuanNode;
    EP_ELEM_IO  * pCurOut;
    unsigned long *pulChOffset;
    int i;
    COMPLEX  *pxBase;
    int  iOutputCnt_8;
    int  iOutputCnt_m;

    pTuyuanNode=(AISet_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    pxBase=pTuyuanNode->pchart->pxBase;
    iOutputCnt_8=pTuyuanNode->ucOutputCnt_8;
    iOutputCnt_m=pTuyuanNode->ucOutputCnt_m;
    pCurOut=pTuyuanNode->ioOutArr;
    pulChOffset=pTuyuanNode->AISourceChOffsetArr;
    pCurOut--;
    pulChOffset--;
    for(i=0; i<iOutputCnt_8; i++)
    {
        (++pCurOut)->now.xVal=*(pxBase+*++pulChOffset);
        (++pCurOut)->now.xVal=*(pxBase+*++pulChOffset);
        (++pCurOut)->now.xVal=*(pxBase+*++pulChOffset);
        (++pCurOut)->now.xVal=*(pxBase+*++pulChOffset);
        (++pCurOut)->now.xVal=*(pxBase+*++pulChOffset);
        (++pCurOut)->now.xVal=*(pxBase+*++pulChOffset);
        (++pCurOut)->now.xVal=*(pxBase+*++pulChOffset);
        (++pCurOut)->now.xVal=*(pxBase+*++pulChOffset);
    }
    for(i=0; i<iOutputCnt_m; i++)
    {
        (++pCurOut)->now.xVal=*(pxBase+*++pulChOffset);
    }

}
