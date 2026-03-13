/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_OuterInputTuyuan.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的外部通道输入元件的代码文件                        */
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
/*                    2005.10.24              修改文件1.1版本                  */
/*                                                                              */
/********************************************************************************/

#include <vxWorks.h>

#include  "RE_OuterInputTuyuan.h"



EP_ELEM_IO *  RE_OuterInputTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode);

EP_STATUS   RE_OuterInputTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                        LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag);

BOOL      RE_OuterInputTuyuanReadFileOtherInit
(OuterInput_Init_Node_Type * pElemInitNodePointer);

BOOL   RE_OuterInputTuyuanGetAISourceFileReadInit(
    FILE  *fp,
    OuterInput_Init_Node_Type *  pElemInitNode
);

BOOL   RE_OuterInputTuyuanGetDISourceFileReadInit(
    FILE  *fp,
    OuterInput_Init_Node_Type *  pElemInitNode
);

BOOL   RE_OuterInputTuyuanGetMeaAISourceFileReadInit(
    FILE  *fp,
    OuterInput_Init_Node_Type *  pElemInitNode
);

BOOL   RE_OuterInputTuyuanGetPoSourceFileReadInit(
    FILE  *fp,
    OuterInput_Init_Node_Type *  pElemInitNode
);

BOOL   RE_OuterInputTuyuanGetAICofSourceFileReadInit(
    FILE  *fp,
    OuterInput_Init_Node_Type *  pElemInitNode
);

BOOL   RE_OuterInputTuyuanGetClCofSourceFileReadInit(
    FILE  *fp,
    OuterInput_Init_Node_Type *  pElemInitNode
);
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

EP_STATUS   RE_OuterInputTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
        TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
        NODE ** pRtElemInitNodePointer)
{

    NODE  *pNode;
    OuterInput_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    uint8_t   ucSignalType;	/* 信号输入类型 */
    int  i;
    BOOL  bOtherInitSuccess;

    EP_ELEM_IO  *  pCurElemOutput;


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
        LOG_Dbg_Msg("OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    pInitNode=(OuterInput_Init_Node_Type    *)malloc(sizeof(OuterInput_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_OUTERINPUT;
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

    /* 读取信号类型  */

    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&ucSignalType);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    /* 读取信号来源类型 */

    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&(pInitNode->ucSignalSourceType));
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    /* 读取4字节保留 */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,4);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }


    /*赋值输出给节点 ， */

    /* 该扫描节点的输出个数 */
    pInitNode->PublicElemData.elem.ucOutNum=1;

    /*循环读取所有输出信息   */
    pCurElemOutput=pInitNode->PublicElemData.elem.aioOut;

    for(i=0; i<1; i++)
    {

        /* 设置输出信号类型 */
        pCurElemOutput->ucAttrib=ucSignalType;

        /* 根据信号来源类型读取和初始化信息 */
        switch(pInitNode->ucSignalSourceType)
        {
            case  ANALOG_INPUT_SOURCE:  /*  若是AI来源*/

                bReadSuccess=
                    RE_OuterInputTuyuanGetAISourceFileReadInit(
                        fp,pInitNode);
                if(!bReadSuccess)
                {
                    return   EP_SYS_ERR;
                }
                break;
            case  DIGITAL_INPUT_SOURCE:/* 若是DI来源 */

                bReadSuccess=
                    RE_OuterInputTuyuanGetDISourceFileReadInit(
                        fp,pInitNode);
                if(!bReadSuccess)
                {
                    return   EP_SYS_ERR;
                }
                break;
            case  PULSE_INPUT_SOURCE:/* 若是PI来源 */

                LOG_Dbg_Msg("OuterInput Tuyuan  Pulse  Input Source  Type  isn't  Implemented!\n",0,0,0,0,0,0);
                assert(FALSE);
                return   EP_SYS_ERR;

                break;
#if defined(EDP03_BUILD) || defined(EDP_01_02_BUILD)	/* 增加EDP01平台用于测试*/
            case MEASURE_AI_SOURCE:/*测量AI通道输入来源*/
                bReadSuccess=
                    RE_OuterInputTuyuanGetMeaAISourceFileReadInit(
                        fp,pInitNode);
                if(!bReadSuccess)
                {
                    return   EP_SYS_ERR;
                }
                break;
#endif
            case PULSE_OUTPUT_SOURCE:
                bReadSuccess=
                    RE_OuterInputTuyuanGetPoSourceFileReadInit(
                        fp,pInitNode);
                if(!bReadSuccess)
                {
                    return   EP_SYS_ERR;
                }
                break;
            case AI_PLUSCOF_SOURCE:
            case AI_OFFCOF_SOURCE:
            case AI_PROCOF_SOURCE:
                bReadSuccess=
                    RE_OuterInputTuyuanGetAICofSourceFileReadInit(
                        fp,pInitNode);
                if(!bReadSuccess)
                {
                    return   EP_SYS_ERR;
                }
                break;
            case CELIANG_PLUSCOF_SOURCE:
            case CELIANG_OFFCOF_SOURCE:
                bReadSuccess=
                    RE_OuterInputTuyuanGetClCofSourceFileReadInit(
                        fp,pInitNode);
                if(!bReadSuccess)
                {
                    return   EP_SYS_ERR;
                }
                break;
            default:

                LOG_Dbg_Msg("OuterInput Tuyuan Input Source  Type  isn't  Expected!\n",0,0,0,0,0,0);
                return  EP_SYS_ERR;
                break;

        }

        /* 为了提高效率,不用数组 */
        pCurElemOutput++;

    }/*所有输出循环处理结束   */


    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_OuterInputTuyuanReadFileOtherInit
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

EP_STATUS   RE_OuterInputTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                        LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    return  EP_SUCCESS;

}


/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_OuterInputTuyuanReadFileOtherInit
(OuterInput_Init_Node_Type * pElemInitNodePointer)
{
    pElemInitNodePointer->PublicElemData.elem.ucType=RE_OUTERINPUT;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_OuterInputTuyuanScanInit;

    /* 设置获得IO输出的函数 */
    pElemInitNodePointer->PublicElemData.pfGetScanNodeOutIOFunc
        =RE_OuterInputTuyuanGetOutIO;

    return   TRUE;

}




/*　当外部输入来源为AI时,读取逻辑标识,并进行相应的初始化
    参数   fp  ,文件当前指针
           pElemInitNode,初始化节点指针
*/
BOOL   RE_OuterInputTuyuanGetAISourceFileReadInit(
    FILE  *fp,
    OuterInput_Init_Node_Type  *  pElemInitNode
)
{

    uint8_t   ucSignalType;
    BOOL  bReadSuccess;
    EP_ELEM_IO  *  pCurElemOutput;
    unsigned  long  ulStrLen;
    void  *  pvAiHandle;

    unsigned  char  ucHandleAttrib;

    pCurElemOutput=pElemInitNode->PublicElemData.elem.aioOut;
    /* 获得AI逻辑标识 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 (fp,pElemInitNode->strInputSourceID,&ulStrLen);
    if(!bReadSuccess)
    {
        return   FALSE;
    }

    /* 获得AI句柄  */
    pvAiHandle=RD_Get_Handle(pElemInitNode->strInputSourceID,RD_LGC_AI_HDL);


    if(pvAiHandle==NULL)
    {
        LOG_Dbg_Msg("Get AI Source  OuterInput  Tuyuan  \'%s\'   Handle  Error!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得外部AI通道输入图元\'%s\'的句柄错误\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);

        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get AI channel  element id\'%s\' handle err\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;
    }

    ucSignalType=pCurElemOutput->ucAttrib;
    /*判定信号类型是否一致  */
    ucHandleAttrib=AI_HND_TO_UNIT(pvAiHandle);

    if(ucHandleAttrib!=ucSignalType)
    {
        LOG_Dbg_Msg(" AI Source  OuterInput  Tuyuan  \'%s\' Signal  Type  isn't  Match  with  Config  Handle!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:外部AI通道输入图元\'%s\'的信号类型同配置不一致\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:AI  element id \'%s\' signal type is not match with config\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;

    }
    /* 若信号类型是实数式电流,电压,阻抗,则是物理通道 */
    if((ucSignalType==REAL_FORM_DIANLIU_SIGNAL)
            ||(ucSignalType==REAL_FORM_DIANLIU_SIGNAL_KA)
            ||(ucSignalType==REAL_FORM_DIANLIU_SIGNAL_MA)
            ||(ucSignalType==REAL_FORM_DIANYA_SIGNAL)
            ||(ucSignalType==REAL_FORM_DIANYA_SIGNAL_KV)
            ||(ucSignalType==REAL_FORM_ZUKANG_SIGNAL)
            ||(ucSignalType==REAL_FORM_ZUKANG_SIGNAL_KO)
            ||(ucSignalType == PINLV_SIGNAL)
      )
    {
        /* 判定能否访问第2个采样节拍的物理AI通道数据指针 */
        float  *pfLgcAi;
        /* 获得访问AI数据指针 */
        pfLgcAi=RD_Lgc_AI_P(pvAiHandle, (RD_AI_Cnt()-10));/* 这里要求实时数据模块实现时
                                                  返回非空,出错,则返回空 */

        if(pfLgcAi==NULL)
        {
            LOG_Dbg_Msg("Get AI Source OuterInput  Tuyuan  \'%s\' Data  Error  By  Handle!\n"
                        ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得AI通道输入图元\'%s\'的数据错误\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic rslv err: get ai  element id \'%s\' data err\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }

            assert(FALSE);  /* 若非以上类型,则告警   */
            return   FALSE;
        }


        /* 赋初值给输出 */
        pCurElemOutput->now.fVal=0.0;
        pCurElemOutput->ucType=0;/* AI通道 */
        pCurElemOutput->pvCh=pvAiHandle;/* 将AI句柄赋给属性 */
        pElemInitNode->ucSignalValueType=FLOAT_VALUE_TYPE;
        pElemInitNode->AISourceChOffset=
            RD_Lgc_AI_Ofst(pvAiHandle);
    }
    /* 否则信号类型若是复数式和幅角式电流,电压,阻抗,则是滤波逻辑处理通道 */
    else  if
    ((ucSignalType==COMPLEX_FORM_DIANLIU_SIGNAL)
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
    )
    {
        /*判定能否访问第2个采样节拍的预处理AI数据指针  */
        COMPLEX *  pxCalcAi;
        pxCalcAi=RD_Calc_AI_P(pvAiHandle, (RD_AI_Cnt()-10));

        if(pxCalcAi==NULL)
        {
            LOG_Dbg_Msg("Get AI Source  OuterInput  Tuyuan  \'%s\'  Data  Error  By  Handle!\n"
                        ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得外部AI通道输入图元\'%s\'的数据错误\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get ai  element \'%s\' data err\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }


            assert(FALSE);  /* 若非以上类型,则告警   */
            return   FALSE;
        }
        /* 赋初值给输出 */

        pCurElemOutput->now.xVal=0.0+0.0i;

        pCurElemOutput->ucType=0;/* AI通道 */
        pCurElemOutput->pvCh=pvAiHandle;/* 将AI句柄赋给属性 */
        pElemInitNode->ucSignalValueType=COMPLEX_VALUE_TYPE;
        pElemInitNode->AISourceChOffset=
            RD_Calc_AI_Ofst(pvAiHandle);

    }
    else
    {

        LOG_Dbg_Msg("Error,  AI Source OuterInput  Tuyuan   \'%s\'   Signal  Type  isn't  Expected!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得AI通道输入图元\'%s\'的信号类型不对\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get ai  element id \'%s\' signal type err\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;

    }

    return  TRUE;

}



/*　当外部输入来源为DI时,读取逻辑标识,并进行相应的初始化
    参数   fp  ,文件当前指针
           pElemInitNode,初始化节点指针
*/
BOOL   RE_OuterInputTuyuanGetDISourceFileReadInit(
    FILE  *fp,
    OuterInput_Init_Node_Type *  pElemInitNode
)
{

    BOOL  bReadSuccess;
    EP_ELEM_IO  *  pCurElemOutput;
    unsigned  long  ulStrLen;
    void  *  pvDiHandle;
    uint8_t   ucSignalType;

    pCurElemOutput=pElemInitNode->PublicElemData.elem.aioOut;
    /* 获得DI逻辑标识 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 (fp,pElemInitNode->strInputSourceID,&ulStrLen);
    if(!bReadSuccess)
    {
        return   FALSE;
    }

    /* 获得DI句柄  */
    pvDiHandle=RD_Get_Handle(pElemInitNode->strInputSourceID,RD_LGC_DI_HDL);
    /* 实时模块提供的realdata.h关于此函数的定义是错误 */

    if(pvDiHandle==NULL)
    {
        LOG_Dbg_Msg("Get DI Source  OuterInput  Tuyuan  \'%s\'   Handle  Error!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得外部DI通道输入图元\'%s\'的句柄错误\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get di  element id \'%s\' handle err\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;
    }
    /* 若信号类型是逻辑信号 */
    ucSignalType=pCurElemOutput->ucAttrib;
    if((ucSignalType==LOGIC_SIGNAL))
    {
        /* 需要判定是否是正确的句柄 */

        /* 赋初值给输出 */
        pCurElemOutput->now.bVal=FALSE;
        pCurElemOutput->ucType=1;/* DI通道 */
        pCurElemOutput->pvCh=pvDiHandle;/* 将DI句柄赋给属性 */
        pElemInitNode->ucSignalValueType=BOOL_VALUE_TYPE;
        pElemInitNode->AISourceChOffset = RD_DI_Ofst(pvDiHandle);

    }
    else
    {

        LOG_Dbg_Msg("Error,  DI Source OuterInput  Tuyuan   \'%s\'  Signal  Type  isn't  Expected!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得外部DI通道输入图元\'%s\'的信号类型不对\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err: get di  element id \'%s\' signal type  err\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }


        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;

    }

    return  TRUE;


}


/*　当外部输入来源为测量AI通道时,读取逻辑标识,并进行相应的初始化
    参数   fp  ,文件当前指针
           pElemInitNode,初始化节点指针
*/
BOOL   RE_OuterInputTuyuanGetMeaAISourceFileReadInit(
    FILE  *fp,
    OuterInput_Init_Node_Type *  pElemInitNode
)
{
    uint8_t   ucSignalType;
    BOOL  bReadSuccess;
    EP_ELEM_IO  *  pCurElemOutput;
    unsigned  long  ulStrLen;
    void  *  pvMeaAiHandle;

    unsigned  char  ucHandleAttrib;

    pCurElemOutput=pElemInitNode->PublicElemData.elem.aioOut;
    /* 获得测量AI逻辑标识 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 (fp,pElemInitNode->strInputSourceID,&ulStrLen);
    if(!bReadSuccess)
    {
        return   FALSE;
    }

    /* 获得AI句柄  */
    pvMeaAiHandle=RD_Get_Handle(pElemInitNode->strInputSourceID,RD_MSU_AI_HDL);


    if(pvMeaAiHandle==NULL)
    {
        LOG_Dbg_Msg("Get MeaAI Source  OuterInput  Tuyuan  \'%s\'   Handle  Error!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得测量AI通道输入图元\'%s\'的句柄错误\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get ai channel  element id \'%s\' handle err\n",	(int)(pElemInitNode->strInputSourceID),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;
    }

    ucSignalType=pCurElemOutput->ucAttrib;
    /*判定信号类型是否一致  */
    ucHandleAttrib=MSU_HND_TO_UNIT(pvMeaAiHandle);

    if(ucHandleAttrib!=ucSignalType)
    {
        LOG_Dbg_Msg(" MeaAI Source  OuterInput  Tuyuan  \'%s\' Signal  Type  isn't  Match  with  Config  Handle!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:测量AI通道输入图元\'%s\'的信号类型同配置不一致\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get ai   element id \'%s\' signal type is different with configure\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;

    }

    /* 信号类型若是幅角式电流,电压,阻抗,则是滤波逻辑处理通道 */
    if
    ((ucSignalType==VALUE_ANGLE_FORM_DIANLIU_SIGNAL)
            ||(ucSignalType==VALUE_ANGLE_FORM_DIANLIU_SIGNAL_KA)
            ||(ucSignalType==VALUE_ANGLE_FORM_DIANYA_SIGNAL)
            ||(ucSignalType==VALUE_ANGLE_FORM_DIANYA_SIGNAL_KV)
            ||(ucSignalType==VALUE_ANGLE_FORM_ZUKANG_SIGNAL)
            ||(ucSignalType==VALUE_ANGLE_FORM_ZUKANG_SIGNAL_KO)
    )
    {
        /* 判定能否访问第2个采样节拍的物理AI通道数据指针 */
        COMPLEX  *pxMeaAi;
        /* 获得访问AI数据指针 */
        pxMeaAi=RD_Msuc_R(pvMeaAiHandle);/* 这里要求实时数据模块实现时
                                                  返回非空,出错,则返回空 */

        if(pxMeaAi==NULL)
        {
            LOG_Dbg_Msg("Get MeaAI Source OuterInput  Tuyuan  \'%s\' Data  Error  By  Handle!\n"
                        ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得测量AI通道输入图元\'%s\'的数据错误\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get ai  element id \'%s\' date error\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }

            assert(FALSE);  /* 若非以上类型,则告警   */
            return   FALSE;
        }
        /* 赋初值给输出 */

        pCurElemOutput->now.xVal=0.0+0.0i;
        pCurElemOutput->ucType=4;/* AI测量通道 */
        pCurElemOutput->pvCh=pvMeaAiHandle;/* 将测量AI句柄赋给属性 */
        pElemInitNode->ucSignalValueType=COMPLEX_VALUE_TYPE;
        pElemInitNode->AISourceChOffset=
            RD_Msuc_AI_Ofst(pvMeaAiHandle);  /*根据测量通道的指针得到该通道数据指针的偏移量*/

    }
    else
    {

        LOG_Dbg_Msg("Error,  MeaAI Source OuterInput  Tuyuan   \'%s\'   Signal  Type  isn't  Expected!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得测量AI通道输入图元\'%s\'的信号类型不对\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get ai   element id \'%s\' signal type error\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;

    }

    return  TRUE;

}

/*　当外部输入来源为脉冲电度输出来源时，,读取逻辑标识,并进行相应的初始化
    参数   fp  ,文件当前指针
           pElemInitNode,初始化节点指针
*/
BOOL   RE_OuterInputTuyuanGetPoSourceFileReadInit(
    FILE  *fp,
    OuterInput_Init_Node_Type *  pElemInitNode
)
{
    uint8_t   ucSignalType;
    BOOL  bReadSuccess;
    EP_ELEM_IO  *  pCurElemOutput;
    unsigned  long  ulStrLen;
    void  *  pvPoHandle;

    unsigned  char  ucHandleAttrib;

    pCurElemOutput=pElemInitNode->PublicElemData.elem.aioOut;
    /* 获得po逻辑标识 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 (fp,pElemInitNode->strInputSourceID,&ulStrLen);
    if(!bReadSuccess)
    {
        return   FALSE;
    }

    /* 获得po句柄  */
    pvPoHandle=RD_Get_Handle(pElemInitNode->strInputSourceID,RD_LGC_PO_HDL);


    if(pvPoHandle==NULL)
    {
        LOG_Dbg_Msg("Get Po Source  OuterInput  Tuyuan  \'%s\'   Handle  Error!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得脉冲电度输出来源的输入图元\'%s\'的句柄错误\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get Pulse output  element id \'%s\' handle error\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;
    }

    ucSignalType=pCurElemOutput->ucAttrib;
    /*判定信号类型是否一致  */
    ucHandleAttrib=PO_HND_TO_UNIT(pvPoHandle); /*需要丁毅提供*/

    if(ucHandleAttrib!=ucSignalType)
    {
        LOG_Dbg_Msg(" Po Source  OuterInput  Tuyuan  \'%s\' Signal  Type  isn't  Match  with  Config  Handle!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:脉冲电度输出来源的输入图元\'%s\'的信号类型同配置不一致\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get Pulse output  element id \'%s\' signal type is different with configurre\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;

    }
    if(IS_CPLX_AI(ucSignalType)||IS_BOOL_SIG(ucSignalType))
    {

        LOG_Dbg_Msg("Error,  Po Source OuterInput  Tuyuan   \'%s\'   Signal  Type  isn't  Expected!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得脉冲电度输出来源的输入图元\'%s\'的信号类型不对\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get Pulse output  element id \'%s\' signal type error\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;

    }

    /* 赋初值给输出 */
    pCurElemOutput->now.fVal=0.0;
    pCurElemOutput->ucType=5;/* po通道 */
    pCurElemOutput->pvCh=pvPoHandle;/* 将AI句柄赋给属性 */

    return TRUE;

}
/*　当外部输入来源为AI所对应的物理通道的系数来源时,读取逻辑标识,并进行相应的初始化
    参数   fp  ,文件当前指针
           pElemInitNode,初始化节点指针
*/
BOOL   RE_OuterInputTuyuanGetAICofSourceFileReadInit(
    FILE  *fp,
    OuterInput_Init_Node_Type  *  pElemInitNode
)
{

    uint8_t   ucSignalType;
    BOOL  bReadSuccess;
    EP_ELEM_IO  *  pCurElemOutput;
    unsigned  long  ulStrLen;
    void  *  pvAiHandle;

    pCurElemOutput=pElemInitNode->PublicElemData.elem.aioOut;
    /* 获得AI逻辑标识 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 (fp,pElemInitNode->strInputSourceID,&ulStrLen);
    if(!bReadSuccess)
    {
        return   FALSE;
    }

    /* 获得AI句柄  */
    pvAiHandle=RD_Get_Handle(pElemInitNode->strInputSourceID,RD_LGC_AI_HDL);


    if(pvAiHandle==NULL)
    {
        LOG_Dbg_Msg("Get AICof Source  OuterInput  Tuyuan  \'%s\'   Handle  Error!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得AI通道所对应的物理通道的系数输入图元\'%s\'的句柄错误\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get Physical channel input  element id \'%s\' handle error\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;
    }

    ucSignalType=pCurElemOutput->ucAttrib;
    if(0x50!=ucSignalType && 0x48!=ucSignalType )
    {
        LOG_Dbg_Msg(" AICof Source  OuterInput  Tuyuan  \'%s\' Signal  Type  isn't  Match  with  Config  Handle!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:AI通道所对应的物理通道的系数输入图元\'%s\'的信号类型不符合要求\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get Physical channel input  element id \'%s\' signal type error\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;

    }


    /* 赋初值给输出 */
    pCurElemOutput->now.fVal=0.0;
    pCurElemOutput->ucType=0;/* AI通道 */
    pCurElemOutput->pvCh=pvAiHandle;/* 将AI句柄赋给属性 */


    return  TRUE;

}


/*　当外部输入来源为测量量的系数来源时,读取逻辑标识,并进行相应的初始化
    参数   fp  ,文件当前指针
           pElemInitNode,初始化节点指针
*/
BOOL   RE_OuterInputTuyuanGetClCofSourceFileReadInit(
    FILE  *fp,
    OuterInput_Init_Node_Type  *  pElemInitNode
)
{

    uint8_t   ucSignalType;
    BOOL  bReadSuccess;
    EP_ELEM_IO  *  pCurElemOutput;
    unsigned  long  ulStrLen;
    int nNum;

    pCurElemOutput=pElemInitNode->PublicElemData.elem.aioOut;
    /* 获得AI逻辑标识 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 (fp,pElemInitNode->strInputSourceID,&ulStrLen);
    if(!bReadSuccess)
    {
        return   FALSE;
    }

    /* 获得AI句柄  */
    nNum=ME_Get_Msu_Idx(pElemInitNode->strInputSourceID);


    if(nNum==-1)
    {
        LOG_Dbg_Msg("Get ClCof Source  OuterInput  Tuyuan  \'%s\'   Handle  Error!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得测量量的系数输入图元\'%s\'的在整个测量量配置表中的序号错误\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get Measurement input  element id \'%s\' has wrong num in Measurement configure table\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;
    }

    ucSignalType=pCurElemOutput->ucAttrib;
    if(0x50!=ucSignalType)
    {
        LOG_Dbg_Msg(" ClCof Source  OuterInput  Tuyuan  \'%s\' Signal  Type  isn't  Match  with  Config  Handle!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:测量量的系数输入图元\'%s\'的信号类型不符合要求\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get Measurement input  element id \'%s\' signal type error\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;

    }


    /* 赋初值给输出 */
    pCurElemOutput->now.fVal=0.0;
    pCurElemOutput->ucType=0xFF;/* */
    pCurElemOutput->pvCh=NULL;/* 将AI句柄赋给属性 */
    pElemInitNode->AISourceChOffset=nNum;

    return  TRUE;

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

EP_STATUS   RE_OuterInputCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{
    NODE  *pNode;
    OuterInput_Init_Node_Type    *pInitNode;
    pInitNode=(OuterInput_Init_Node_Type    *)pElemInitNode->pTuyuan;

    /*  设置图元类型*/
    switch(pInitNode->ucSignalSourceType)
    {
        case  ANALOG_INPUT_SOURCE:
            if(pInitNode->ucSignalValueType
                    ==FLOAT_VALUE_TYPE)
            {

                FloatAIOuterInput_Scan_Node_Type    *pScanNode;
                pNode=(NODE  *)malloc(sizeof(NODE));
                if(pNode==NULL)
                {
                    LOG_Dbg_Msg("Float  AI OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                    return  EP_BUF_ERR;
                }
                pScanNode=(FloatAIOuterInput_Scan_Node_Type    *)
                          malloc
                          (sizeof(FloatAIOuterInput_Scan_Node_Type));
                if(pScanNode==NULL)
                {
                    LOG_Dbg_Msg("Float  AI  OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                    return  EP_BUF_ERR;
                }
                pNode->ulTuyuanType=RE_FLOAT_AI_OUTERINPUT_SCAN;
                pNode->pTuyuan=(void  *)pScanNode;
                pScanNode->ioOut=pInitNode->
                                 PublicElemData.elem.aioOut[0];
                pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                                  PublicElemData.pfGetScanNodeOutIOFunc;
                pScanNode->AISourceChOffset=pInitNode->
                                            AISourceChOffset;
                pScanNode->pchart=pInitNode->
                                  PublicElemData.elem.pchart;
                /*2011-7-27  ZY  */
                pScanNode->pCollectOut=NULL;
                /* 设定返回的节点地址  */
                *pReturnElemScanNodePointer=(NODE  *)pNode;
                return  EP_SUCCESS;


            }
            else  if(pInitNode->ucSignalValueType
                     ==COMPLEX_VALUE_TYPE)
            {

                ComplexAIOuterInput_Scan_Node_Type    *pScanNode;
                pNode=(NODE  *)malloc(sizeof(NODE));
                if(pNode==NULL)
                {
                    LOG_Dbg_Msg("Complex  AI OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                    return  EP_BUF_ERR;
                }
                pScanNode=(ComplexAIOuterInput_Scan_Node_Type    *)
                          malloc
                          (sizeof(ComplexAIOuterInput_Scan_Node_Type));
                if(pScanNode==NULL)
                {
                    LOG_Dbg_Msg("Complex  AI  OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                    return  EP_BUF_ERR;
                }
                pNode->ulTuyuanType=RE_COMPLEX_AI_OUTERINPUT_SCAN;
                pNode->pTuyuan=(void  *)pScanNode;
                pScanNode->ioOut=pInitNode->
                                 PublicElemData.elem.aioOut[0];
                pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                                  PublicElemData.pfGetScanNodeOutIOFunc;
                pScanNode->AISourceChOffset=pInitNode->
                                            AISourceChOffset;
                pScanNode->pchart=pInitNode->
                                  PublicElemData.elem.pchart;
                /*2011-7-27  ZY  */
                pScanNode->pCollectOut=NULL;
                /* 设定返回的节点地址  */
                *pReturnElemScanNodePointer=(NODE  *)pNode;
                return  EP_SUCCESS;

            }
            else
            {

                LOG_Dbg_Msg("Error,  AI Source OuterInput  Tuyuan  \'%s\'   Signal  Type  isn't  Expected !\n"
                            ,(int)(pInitNode->strInputSourceID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得外部AI通道输入图元\'%s\'的信号类型错误\n"
                               ,(int)(pInitNode->strInputSourceID),0);

                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:get ai  element id \'%s\' signal type err\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }

                assert(FALSE);
                return   EP_SYS_ERR;
            }
            break;
        case  DIGITAL_INPUT_SOURCE:
        {
            DIOuterInput_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("DI  OuterInput    Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(DIOuterInput_Scan_Node_Type    *)
                      malloc
                      (sizeof(DIOuterInput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("DI  OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_DI_OUTERINPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->ioOut=pInitNode->
                             PublicElemData.elem.aioOut[0];
            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                              PublicElemData.pfGetScanNodeOutIOFunc;

            pScanNode->DISourceChOffset=pInitNode->
                                        AISourceChOffset;

            pScanNode->pchart=pInitNode->
                              PublicElemData.elem.pchart;
            /*2011-7-27  ZY  */
            pScanNode->pCollectOut=NULL;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  MEASURE_AI_SOURCE:
            if(pInitNode->ucSignalValueType
                    ==COMPLEX_VALUE_TYPE)
            {

                MeaAIOuterInput_Scan_Node_Type    *pScanNode;
                pNode=(NODE  *)malloc(sizeof(NODE));
                if(pNode==NULL)
                {
                    LOG_Dbg_Msg("Measure AI OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                    return  EP_BUF_ERR;
                }
                pScanNode=(MeaAIOuterInput_Scan_Node_Type    *)
                          malloc
                          (sizeof(MeaAIOuterInput_Scan_Node_Type));
                if(pScanNode==NULL)
                {
                    LOG_Dbg_Msg("Measure  AI  OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                    return  EP_BUF_ERR;
                }
                pNode->ulTuyuanType=RE_MEA_AI_OUTERINPUT_SCAN;
                pNode->pTuyuan=(void  *)pScanNode;
                pScanNode->ioOut=pInitNode->
                                 PublicElemData.elem.aioOut[0];
                pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                                  PublicElemData.pfGetScanNodeOutIOFunc;
                pScanNode->AISourceChOffset=pInitNode->
                                            AISourceChOffset;
                pScanNode->pchart=pInitNode->
                                  PublicElemData.elem.pchart;
                /* 设定返回的节点地址  */
                *pReturnElemScanNodePointer=(NODE  *)pNode;
                return  EP_SUCCESS;

            }
            else
            {

                LOG_Dbg_Msg("Error,  MeaAI Source OuterInput  Tuyuan  \'%s\'   Signal  Type  isn't  Expected !\n"
                            ,(int)(pInitNode->strInputSourceID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得测量AI通道输入图元\'%s\'的信号类型错误\n"
                               ,(int)(pInitNode->strInputSourceID),0);

                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:get ai  element id \'%s\' signal type err\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }

                assert(FALSE);
                return   EP_SYS_ERR;
            }
            break;

        case  PULSE_OUTPUT_SOURCE:
        {

            PulseOutput_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("Float  PoOutput OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(PulseOutput_Scan_Node_Type    *)
                      malloc
                      (sizeof(PulseOutput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("Float  PoOutput  OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_PO_OUTERINPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->ioOut=pInitNode->
                             PublicElemData.elem.aioOut[0];
            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                              PublicElemData.pfGetScanNodeOutIOFunc;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;


        }

        break;
        case  AI_PLUSCOF_SOURCE:
        {
            AIPlusCofInput_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg(" AI PlusCof OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(AIPlusCofInput_Scan_Node_Type    *)
                      malloc
                      (sizeof(AIPlusCofInput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("AI PlusCof  OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_AI_PLUSCOF_OUTERINPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->ioOut=pInitNode->
                             PublicElemData.elem.aioOut[0];
            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                              PublicElemData.pfGetScanNodeOutIOFunc;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;

        }

        break;
        case  AI_OFFCOF_SOURCE:
        {
            AIOffCofInput_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg(" AI OffCof OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(AIOffCofInput_Scan_Node_Type    *)
                      malloc
                      (sizeof(AIOffCofInput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("AI OffCof  OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_AI_OFFCOF_OUTERINPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->ioOut=pInitNode->
                             PublicElemData.elem.aioOut[0];
            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                              PublicElemData.pfGetScanNodeOutIOFunc;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;

        }

        break;
        case  CELIANG_PLUSCOF_SOURCE:
        {
            ClPlusCofInput_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg(" Cl PlusCof OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(ClPlusCofInput_Scan_Node_Type    *)
                      malloc
                      (sizeof(ClPlusCofInput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("Cl PlusCof OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_CL_PLUSCOF_OUTERINPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->ioOut=pInitNode->
                             PublicElemData.elem.aioOut[0];
            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                              PublicElemData.pfGetScanNodeOutIOFunc;
            pScanNode->clNum=pInitNode->
                             AISourceChOffset;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;

        }

        break;
        case  CELIANG_OFFCOF_SOURCE:
        {
            ClOffCofInput_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg(" Cl OffCof OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(ClOffCofInput_Scan_Node_Type    *)
                      malloc
                      (sizeof(ClOffCofInput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("Cl OffCof  OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_CL_OFFCOF_OUTERINPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->ioOut=pInitNode->
                             PublicElemData.elem.aioOut[0];
            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                              PublicElemData.pfGetScanNodeOutIOFunc;
            pScanNode->clNum=pInitNode->
                             AISourceChOffset;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;

        }

        break;
        case  AI_PROCOF_SOURCE:
        {
            AIProCofInput_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg(" AI ProCof OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(AIProCofInput_Scan_Node_Type    *)
                      malloc
                      (sizeof(AIProCofInput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("AI ProCof  OuterInput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_AI_PROCOF_OUTERINPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->ioOut=pInitNode->
                             PublicElemData.elem.aioOut[0];
            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                              PublicElemData.pfGetScanNodeOutIOFunc;
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
     功能:获得图元的扫描节点的某个输出的指针

*/
/****参数：
           unOutNum,输出的序号
           pScanNode，扫描节点指针
*/
/*   返回值，相应序号的输出IO的指针 ，若失败，则返回NULL*/

EP_ELEM_IO *  RE_OuterInputTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode)
{

    switch(pScanNode->ulTuyuanType)
    {
        case   RE_FLOAT_AI_OUTERINPUT_SCAN:
        {

            FloatAIOuterInput_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(FloatAIOuterInput_Scan_Node_Type   *)pScanNode->pTuyuan;
            if(unOutNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get  Float AI  Source  OuterInput Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            /*2011-7-27 zy  */
            if(pTuyuanNode->pCollectOut)
            {
                /*若是图元输出被集中后，则返回集中后的IO  */
                return  	pTuyuanNode->pCollectOut;
            }
            else
            {
                return   &(pTuyuanNode->ioOut);
            }

        }
        break;
        case   RE_COMPLEX_AI_OUTERINPUT_SCAN:
        {

            ComplexAIOuterInput_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(ComplexAIOuterInput_Scan_Node_Type    *)pScanNode->pTuyuan;
            if(unOutNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get Complex AI  Source  OuterInput Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            /*2011-7-27 zy  */
            if(pTuyuanNode->pCollectOut)
            {
                /*若是图元输出被集中后，则返回集中后的IO  */
                return  	pTuyuanNode->pCollectOut;
            }
            else
            {
                return   &(pTuyuanNode->ioOut);
            }

        }
        break;
        case   RE_DI_OUTERINPUT_SCAN:
        {

            DIOuterInput_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(DIOuterInput_Scan_Node_Type    *)pScanNode->pTuyuan;
            if(unOutNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get DI  Source   OuterInput Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            /*2011-7-27 zy  */
            if(pTuyuanNode->pCollectOut)
            {
                /*若是图元输出被集中后，则返回集中后的IO  */
                return  	pTuyuanNode->pCollectOut;
            }
            else
            {
                return   &(pTuyuanNode->ioOut);
            }

        }
        break;
        case RE_MEA_AI_OUTERINPUT_SCAN:
        {
            MeaAIOuterInput_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(MeaAIOuterInput_Scan_Node_Type    *)pScanNode->pTuyuan;
            if(unOutNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get Mea AI  Source  OuterInput Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOut);
        }
        break;

        case RE_PO_OUTERINPUT_SCAN:
        {
            PulseOutput_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(PulseOutput_Scan_Node_Type   *)pScanNode->pTuyuan;
            if(unOutNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get  Po  Source  OuterInput Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOut);
        }
        break;
        case   RE_AI_PLUSCOF_OUTERINPUT_SCAN:
        {

            AIPlusCofInput_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(AIPlusCofInput_Scan_Node_Type   *)pScanNode->pTuyuan;
            if(unOutNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get  AI PlusCof  OuterInput Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOut);

        }
        break;
        case   RE_AI_OFFCOF_OUTERINPUT_SCAN:
        {

            AIOffCofInput_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(AIOffCofInput_Scan_Node_Type   *)pScanNode->pTuyuan;
            if(unOutNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get  AI Offcof  OuterInput Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOut);

        }
        break;
        case   RE_CL_PLUSCOF_OUTERINPUT_SCAN:
        {

            ClPlusCofInput_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(ClPlusCofInput_Scan_Node_Type   *)pScanNode->pTuyuan;
            if(unOutNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get  Cl PlusCof Source  OuterInput Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOut);

        }
        break;
        case   RE_CL_OFFCOF_OUTERINPUT_SCAN:
        {

            ClOffCofInput_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(ClOffCofInput_Scan_Node_Type   *)pScanNode->pTuyuan;
            if(unOutNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get  Cl OffCof  OuterInput Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOut);

        }
        break;
        case   RE_AI_PROCOF_OUTERINPUT_SCAN:
        {

            AIProCofInput_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(AIProCofInput_Scan_Node_Type   *)pScanNode->pTuyuan;
            if(unOutNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get  AI ProCof  OuterInput Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOut);

        }
        break;
        default  :

            LOG_Dbg_Msg("Get One OuterInput  Tuyuan  OutputIO  Error!\n",0,0,0,0,0,0);
            assert(FALSE);
            return  NULL;
            break;
    }


}

/* 多个DI来源的外部输入图元的扫描函数.
 * Para:
 *     PartGrpAttrib, 分图.
 * Return:
 *     NONE.
 */
void RE_MultiDIOuterInputTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib)
{
    NODE **ppCurDINode;
    int i;
    DIOuterInput_Scan_Node_Type *pScanNode;
    BOOL *pDIBase;

    if (!PartGrpAttrib->DIScanNodeNum)
    {
        /* 无开入图元 */
        return;
    }

    ppCurDINode = PartGrpAttrib->ppDINode;
    pDIBase = RD_Base_His_DI_P(((DIOuterInput_Scan_Node_Type *)(*ppCurDINode)->pTuyuan)->pchart->ulScnAiCnt);
    for (i = 0; i<PartGrpAttrib->DIScanNodeNum; i++, ppCurDINode++)
    {
        pScanNode = (DIOuterInput_Scan_Node_Type *)(*ppCurDINode)->pTuyuan;
        pScanNode->ioOut.now.bVal = *(pDIBase+pScanNode->DISourceChOffset);
    }
}
