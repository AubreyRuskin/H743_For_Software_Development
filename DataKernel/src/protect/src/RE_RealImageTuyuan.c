/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_RealImageTuyuan.c                                    1.0                  */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该源代码文件定义了保护功能模块中的实部虚部元件的实现                       */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                   */
/*                                                                              */
/*         作者           日期                    说明                    */
/*                                                                              */
/*         张云       2005.11.10              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#include   <vxWorks.h>
#include   "RE_RealImageTuyuan.h"


/****算法图元的逻辑图文件读取初始化函数****************************/
/*
     功能:从文件中读取图元相关数据,,此时已读完图元类型字节
          申请初始化数据节点和
          供上层程序添加两节点到连表中
          并进行数据节点内容的部分初始化
*/
/****参数：fp,逻辑图文件指针***************************/
/*         ulReadOffsetToBegain,相对于文件起始,读取的文件偏移位置
           ,不包括图元类型字节
           pTuyuanInitData,图元节点初始化数据指针
           pRtElemInitNodePointer,返回申请的图元初始化数据节点内存地址

*/
/*   返回值，EP_STATUS */

EP_STATUS   RE_RealImageTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
        TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
        NODE ** pRtElemInitNodePointer)
{

    NODE  *pNode;
    BOOL  bOtherInitSuccess;
    unsigned   long   TempLong;
    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoceID)[MAX_LOGID_STR_LEN+1];
    char (* pstrCurAOID)[MAX_LOGID_STR_LEN+1];		/* AO logic symbol. */

    unsigned  char      ucCurLuboSetFlag;
    unsigned   char     ucCurFlagSetFlag;
    unsigned   char     ucCurYaoceSetFlag;
    unsigned char       ucCurMeasureSetFlag;
    unsigned char ucCurAOChSetFlag;		/* AO channel flag. */

    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *   pbCurYaoceSetFlag;
    BOOL *pbCurAOChSetFlag;	/* AO channel setting flag. */
    char   (*  pstrCurMeasureID)[MAX_LOGID_STR_LEN+1];	/* 定义临时变量 */
    BOOL  *   pbCurMeasureSetFlag;
    EP_ELEM_IO  *  pCurElemOutput;

    unsigned  char   *   pucCurInSignalType;
    unsigned  short   *  punCurInSourceSeqNo;
    unsigned  char   *   pucInSourceOutputNo;
    RealImage_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    int  i;
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
        LOG_Dbg_Msg("RealImage  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }

    pInitNode=(RealImage_Init_Node_Type    *)malloc(sizeof(RealImage_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("RealImage  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_REALIMAGE;
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
        /*读取信号类型  */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,pucCurInSignalType);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        if(!(IS_CPLX_AI(*pucCurInSignalType)))
        {
            LOG_Dbg_Msg("Error, RealImage Tuyuan Input Signal Type  is not  expected!\n"
                        ,0,0,0,0,0,0);
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取实部虚部图元的输入信号类型不是预期类型\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:read realimage  element input signal type is not expected\n"
                           ,0,0);
            }
            assert(FALSE);
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


    pInitNode->PublicElemData.elem.ucOutNum=2;

    /*循环读取所有输出信息   */

    pCurElemOutput=pInitNode->PublicElemData.elem.aioOut;

    pbCurLuboSetFlag=pInitNode->PublicElemData.abLuboFlagArr;
    pbCurFlagSetFlag=pInitNode->PublicElemData.abFlagSetFlagArr;
    pbCurYaoceSetFlag=pInitNode->PublicElemData.abYaoceFlagArr;
    pbCurAOChSetFlag=pInitNode->PublicElemData.abAOFlagArr;  /* If setting. */

    /* 定义用指向数组的指针，这里不能用双重指针，因为2维字符数组还未初始化 */

    pstrCurLuboID=pInitNode->aStrLuboIDArr;
    pstrCurFlagID=pInitNode->aStrFlagIDArr;
    pstrCurYaoceID=pInitNode->aStrYaoceIDArr;

    pbCurMeasureSetFlag=pInitNode->PublicElemData.abMeasureFlagArr;
    pstrCurMeasureID=pInitNode->aStrMeasureIDArr;

    pstrCurAOID=pInitNode->aStrAOChIDArr;  /* ID */

    for(i=0; i<2; i++)
    {
        /*读取信号类型,进行输出数据的初始化
          该信号类型数据可能会被用户的算法初始化函数覆盖掉  */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&(pCurElemOutput->ucAttrib));
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        if(!(IS_REAL_SIG(pCurElemOutput->ucAttrib)))
        {
            LOG_Dbg_Msg("Error, RealImage Tuyuan Output Signal Type  is not  expected!\n"
                        ,0,0,0,0,0,0);
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取实部虚部图元的输出信号类型不是预期类型\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT | ER_ALARM | ER_LOCK,
                           "logic grp rslv err: read realimage  element output signal type is not expected!\n"
                           ,0,0);
            }
            assert(FALSE);
            return   EP_SYS_ERR;
        }

        pCurElemOutput->now.fVal=0.0;
        pCurElemOutput->pvCh=NULL;/*通道句柄为空*/
        pCurElemOutput->ucType=0xFF;/*中间结果  */

        /* 读取3个保留字节  */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,3);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        /* 读取AO通道设置 */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp, &ucCurAOChSetFlag);

        if (!bReadSuccess)
        {
            return EP_SYS_ERR;
        }

        if ((ucCurAOChSetFlag) == 1)
        {
            *pbCurAOChSetFlag=TRUE;
            bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                         (fp, (*pstrCurAOID), &TempLong);

            if (!bReadSuccess)
            {
                return EP_SYS_ERR;
            }

        }
        else if ((ucCurAOChSetFlag) == 0)
        {
            *pbCurAOChSetFlag=FALSE;
            strcpy((*pstrCurAOID), "");
        }
        else
        {
            LOG_Dbg_Msg("Error, Read RealImage Tuyuan AO Flag Error!\n",
                        0, 0, 0, 0, 0, 0);

            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误: 读取实部虚部图元AO设置错误!\n",
                           0, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err: read RealImage Tuyuan AO set err!\n",
                           0, 0);
            }

            assert(FALSE);  /* 若非以上类型,则告警 */

            return EP_SYS_ERR;
        }

        /*读取录波设置   */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&ucCurLuboSetFlag);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        if((ucCurLuboSetFlag)==1)
        {
            *pbCurLuboSetFlag=TRUE;
            bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                         (fp,(*pstrCurLuboID),&TempLong);
            if(!bReadSuccess)
            {
                return   EP_SYS_ERR;
            }

        }
        else  if((ucCurLuboSetFlag)==0)
        {
            *pbCurLuboSetFlag=FALSE;
            /* 将1个空串赋给字符数组,注意直接赋值非法 */
            strcpy((*pstrCurLuboID),"");
        }
        else
        {
            LOG_Dbg_Msg("Error, Read RealImage  Tuyuan  Lubo  Flag  Error!\n"
                        ,0,0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取实部虚部图元的录波设置错误\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic  rslv err:read realimage  element disturbance record set err\n"
                           ,0,0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        /*读取标志设置   */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&ucCurFlagSetFlag);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        if((ucCurFlagSetFlag)==1)
        {
            *pbCurFlagSetFlag=TRUE;
            bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                         (fp,(*pstrCurFlagID),&TempLong);
            if(!bReadSuccess)
            {
                return   EP_SYS_ERR;
            }

        }
        else  if((ucCurFlagSetFlag)==0)
        {
            *pbCurFlagSetFlag=FALSE;
            strcpy((*pstrCurFlagID),"");
        }
        else
        {
            LOG_Dbg_Msg("Error, Read RealImage  Tuyuan  FlagSet  Flag  Error!\n"
                        ,0,0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取实部虚部图元的标志设置错误\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic  rslv err:read realimage diagram element flag set err\n"
                           ,0,0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        /*读取遥测设置   */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&ucCurYaoceSetFlag);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        if((ucCurYaoceSetFlag)==1)
        {
            *pbCurYaoceSetFlag=TRUE;
            bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                         (fp,(*pstrCurYaoceID),&TempLong);
            if(!bReadSuccess)
            {
                return   EP_SYS_ERR;
            }

        }
        else  if((ucCurYaoceSetFlag)==0)
        {
            *pbCurYaoceSetFlag=FALSE;
            strcpy((*pstrCurYaoceID),"");
        }
        else
        {
            LOG_Dbg_Msg("Error, Read RealImage  Tuyuan  Yaoce  Flag  Error!\n"
                        ,0,0,0,0,0,0);
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取实部虚部图元的遥测设置错误\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic  rslv err:read realimage  element remote measurement set err\n"
                           ,0,0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        /*读取测量设置   */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&ucCurMeasureSetFlag);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        if((ucCurMeasureSetFlag)==1)
        {
            *pbCurMeasureSetFlag=TRUE;
            bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                         (fp,(*pstrCurMeasureID),&TempLong);
            if(!bReadSuccess)
            {
                return   EP_SYS_ERR;
            }

        }
        else  if((ucCurMeasureSetFlag)==0)
        {
            *pbCurMeasureSetFlag=FALSE;
            strcpy((*pstrCurMeasureID),"");
        }
        else
        {
            LOG_Dbg_Msg("Error, Read RealImage   element  MeasureValue  Flag  Error!\n"
                        ,0,0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取实部虚部图元的测量设置错误\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic  rslv err:read realimage  element  measurement set err\n",0,0);
            }

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }
        pbCurMeasureSetFlag++;
        pstrCurMeasureID++;

        /* 为了提高效率,不用数组 */
        pCurElemOutput++;

        pbCurLuboSetFlag++;
        pbCurFlagSetFlag++;
        pbCurYaoceSetFlag++;
        pbCurAOChSetFlag++;	/* AO symbol. */

        pstrCurLuboID++;
        pstrCurFlagID++;
        pstrCurYaoceID++;
        pstrCurAOID++;       /* ID of AO. */
    }/*所有输出循环处理结束   */
    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_RealImageTuyuanReadFileOtherInit
                      (pInitNode);

    if(!bOtherInitSuccess)
    {
        return  EP_SYS_ERR;
    }
    /* 设定返回的节点地址  */
    *pRtElemInitNodePointer=(NODE  *)pNode;

    return   EP_SUCCESS;
}


/*  在读取文件后,进行算法图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/

BOOL      RE_RealImageTuyuanReadFileOtherInit
(RealImage_Init_Node_Type * pElemInitNodePointer)
{
    pElemInitNodePointer->PublicElemData.elem.ucType=RE_REALIMAGE;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_RealImageTuyuanScanInit;
    /* 设置获取扫描节点输出的指针的函数 */
    pElemInitNodePointer->PublicElemData.pfGetScanNodeOutIOFunc=
        RE_RealImageTuyuanGetOutIO;

    return   TRUE;
}


/****算法图元的扫描初始化函数****************************/
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

EP_STATUS   RE_RealImageTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                       LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    RealImage_Init_Node_Type    *pInitNode;
    RealImage_Scan_Node_Type    *pScanNode;
    NODE  *  pCurSourceNode;

    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *   pbCurYaoceSetFlag;
    BOOL *pbCurAOSetFlag;		/* AO symbol. */

    EP_STATUS  SetCurOutAttribResult;

    EP_ELEM_IO  *  pCurElemOutput;

    uint8_t  * pucCurInSignalType;
    uint16_t   * punCurInSignalSourceSeqNo;
    uint8_t   *  pucCurInSignalSourceOutNo;

    /* 定义用指向数组的指针，这里未用双重指针, */
    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoceID)[MAX_LOGID_STR_LEN+1];
    char (* pstrCurAOID)[MAX_LOGID_STR_LEN+1];

    BOOL  *   pbCurMeasureSetFlag;	/* 定义临时变量 */
    char   (*  pstrCurMeasureID)[MAX_LOGID_STR_LEN+1];
    int  i;
    uint8_t   ucCurInSignalType;

    pInitNode=(RealImage_Init_Node_Type    *)pElemInitNode->pTuyuan;
    pScanNode=(RealImage_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    pucCurInSignalType=pInitNode->aucSourceSignalTypeArr;
    punCurInSignalSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucCurInSignalSourceOutNo=pInitNode->aucInSourceOutputNumArr;
    /* 循环设定图元的输入源指针  */
    for(i=0; i<1; i++)
    {

        pCurSourceNode=RE_LstNth(pGrpScanNodeList,
                                 ((*punCurInSignalSourceSeqNo)+1));/* 注意序号从1开始 */

        if(pCurSourceNode==NULL)
        {
            LOG_Dbg_Msg("Error,Get  RealImage  Tuyuan  Input Source  Init  failure!\n"
                        ,0,0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得实部虚部图元输入来源错误\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get realimage  element input source err\n"
                           ,0,0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }
        /* 获得并设置当前输入源的指针 */

        pScanNode->pInArr0=(*  RE_GetScanNodeGetOutIOFunc(pCurSourceNode))
                           ((*pucCurInSignalSourceOutNo),pCurSourceNode);

        /*  确保该图元输入在来源图元的输出号没有出界 */
        if(pScanNode->pInArr0==NULL)
        {
            LOG_Dbg_Msg("Error,Get RealImage Tuyuan  input  info  failure!\n"
                        ,0,0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得实部虚部图元输入来源失败!\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get realimage  element input source failure\n"
                           ,0,0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        /*比较信号类型是否匹配  */

        ucCurInSignalType=pScanNode->pInArr0->ucAttrib;

        if(ucCurInSignalType!=(*pucCurInSignalType))
        {
            LOG_Dbg_Msg("Error,RealImage  Tuyuan Input  Signal Type  can't  Match!\n"
                        ,0,0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:实部虚部图元的输入的信号类型错误\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:realimage  element input signal type err\n"
                           ,0,0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        pucCurInSignalType++;
        punCurInSignalSourceSeqNo++;
        pucCurInSignalSourceOutNo++;
    }

    pCurElemOutput=pScanNode->ioOutArr;
    pbCurLuboSetFlag=pInitNode->PublicElemData.abLuboFlagArr;
    pbCurFlagSetFlag=pInitNode->PublicElemData.abFlagSetFlagArr;
    pbCurYaoceSetFlag=pInitNode->PublicElemData.abYaoceFlagArr;
    pbCurAOSetFlag=pInitNode->PublicElemData.abAOFlagArr;

    pstrCurLuboID=pInitNode->aStrLuboIDArr;
    pstrCurFlagID=pInitNode->aStrFlagIDArr;
    pstrCurYaoceID=pInitNode->aStrYaoceIDArr;
    pstrCurAOID=pInitNode->aStrAOChIDArr;


    pbCurMeasureSetFlag=pInitNode->PublicElemData.abMeasureFlagArr;
    pstrCurMeasureID=pInitNode->aStrMeasureIDArr;

    for(i=0; i<2; i++)
    {
        {
            extern VALUE_TYPE gTmpRecbuf[MAX_REC_TEMP_BUFSIZE];
            pCurElemOutput->recbuf=gTmpRecbuf;
        }

        if(bPartGrpRunFlag)
        {
            /*若该图元所在的分图被投入，则设置录波，标志，遥测，遥信
             ，否则不设置  */

            /*设置输出为录波量  */
            if((*pbCurLuboSetFlag))
            {
                /* 若录波标志为真 */
                SetCurOutAttribResult=SCI_Init_Add_New_Lubo_Signal
                                      ((*pstrCurLuboID),pCurElemOutput,
                                       pInitNode->PublicElemData.nScanTaskNo);

                if(SetCurOutAttribResult!=EP_SUCCESS)
                {
                    LOG_Dbg_Msg(" Error,Set  RealImage  Tuyuan   Output  Lubo  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(*pstrCurLuboID),0,0,0,0,0);

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:实部虚部图元的输出的录波标识为\'%s\'的录波设置错误\n"
                                   ,(int)(*pstrCurLuboID),0);
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp rslv err:realimage  element disturbance record id\'%s\' err\n"
                                   ,(int)(*pstrCurLuboID),0);
                    }
                    assert(FALSE);  /* 若非以上类型,则告警   */
                    return   SetCurOutAttribResult;
                }
            }

            /*设置输出为标志量  */

            if((*pbCurFlagSetFlag))
            {
                /* 若录波标志为真 */
                SetCurOutAttribResult=SCI_Init_Add_New_Flag_Signal
                                      ((*pstrCurFlagID),pCurElemOutput,
                                       pInitNode->PublicElemData.nScanTaskNo);

                if(SetCurOutAttribResult!=EP_SUCCESS)
                {
                    LOG_Dbg_Msg(" Error,Set  RealImage  Tuyuan Output  Flag  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(*pstrCurFlagID),0,0,0,0,0);

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:实部虚部图元的输出的标志标识为\'%s\'的标志设置错误\n"
                                   ,(int)(*pstrCurFlagID),0);
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic  rslv grp err:realimage  element flag id \'%s\' err\n"
                                   ,(int)(*pstrCurFlagID),0);
                    }

                    assert(FALSE);  /* 若非以上类型,则告警   */
                    return   SetCurOutAttribResult;
                }
            }

            /*设置输出为遥测量  */

            if((*pbCurYaoceSetFlag))
            {
                /* 若录波标志为真 */
                SetCurOutAttribResult=SCI_Init_Add_New_Yaoce_Signal
                                      ((*pstrCurYaoceID),pCurElemOutput,
                                       pInitNode->PublicElemData.nScanTaskNo);

                if(SetCurOutAttribResult!=EP_SUCCESS)
                {
                    LOG_Dbg_Msg(" Error,Set  RealImage  Tuyuan Output  Yaoce  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(*pstrCurYaoceID),0,0,0,0,0);

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:实部虚部图元的输出的遥测标识为\'%s\'的遥测设置错误\n"
                                   ,(int)(*pstrCurYaoceID),0);
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp rslv err:realimage  element remote measurement id \'%s\' err\n"
                                   ,(int)(*pstrCurYaoceID),0);
                    }
                    assert(FALSE);  /* 若非以上类型,则告警   */
                    return   SetCurOutAttribResult;
                }
            }
            /*设置输出为测量量  */

            if((*pbCurMeasureSetFlag))
            {
                /* 若录波标志为真 */
                SetCurOutAttribResult=SCI_Init_Add_New_Measure_Signal
                                      ((*pstrCurMeasureID),pCurElemOutput,
                                       pInitNode->PublicElemData.nScanTaskNo);

                if(SetCurOutAttribResult!=EP_SUCCESS)
                {
                    LOG_Dbg_Msg(" Error,Set  RealImage  Tuyuan Output  Measure  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(*pstrCurMeasureID),0,0,0,0,0);

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:实部虚部图元的输出的测量标识为\'%s\'的测量设置错误\n"
                                   ,(int)(*pstrCurMeasureID),0);
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic diagram rslv err:realimage  element measurement id \'%s\' err\n"
                                   ,(int)(*pstrCurMeasureID),0);
                    }
                    assert(FALSE);  /* 若非以上类型,则告警   */
                    return   SetCurOutAttribResult;
                }
            }


            /* 设置AO */
            if ((*pbCurAOSetFlag))
            {
                /* 若标志为真 */
                SetCurOutAttribResult=RD_VirtBoxAddMidSrcAo((*pstrCurAOID), pCurElemOutput->ucAttrib,
                                      pCurElemOutput);

                if (SetCurOutAttribResult != EP_SUCCESS)
                {
                    LOG_Dbg_Msg("Error, Set RealImage Tuyuan Elem Output AO ID is \'%s\' Value Error!\n",
                                (int)(*pstrCurAOID), 0, 0, 0, 0, 0);

                    if (ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:实部虚部图元输出的AO标识为\'%s\'的AO设置错误!\n",
                                   (int)(*pstrCurAOID), 0);
                    }
                    else if (ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp  rslv err:RealImage Tuyuan element output measurement id \'%s\' set err!\n",
                                   (int)(*pstrCurAOID), 0);
                    }

                    assert(FALSE);  /* 若非以上类型, 则告警 */

                    return SetCurOutAttribResult;
                }
            }

        }
        /* 为了提高效率,不用数组 */
        pCurElemOutput++;
        pbCurLuboSetFlag++;
        pbCurFlagSetFlag++;
        pbCurYaoceSetFlag++;
        pbCurAOSetFlag++;


        pstrCurLuboID++;
        pstrCurFlagID++;
        pstrCurYaoceID++;
        pbCurMeasureSetFlag++;
        pstrCurMeasureID++;


        pstrCurAOID++;
    }

    return  EP_SUCCESS;
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

EP_STATUS   RE_RealImageCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{
    NODE  *pNode;
    int  i;
    RealImage_Scan_Node_Type    *pScanNode;
    RealImage_Init_Node_Type    *pInitNode;

    pInitNode=(RealImage_Init_Node_Type    *)pElemInitNode->pTuyuan;

    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("RealImage  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pScanNode=(RealImage_Scan_Node_Type    *)malloc(sizeof(RealImage_Scan_Node_Type));
    if(pScanNode==NULL)
    {
        LOG_Dbg_Msg("RealImage  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }

    /* 设置实际扫描图元类型 */
    pNode->ulTuyuanType=RE_REALIMAGE_SCAN;
    pNode->pTuyuan=(void  *)pScanNode;
    pScanNode->pchart=pInitNode->PublicElemData.elem.pchart;
    for(i=0; i<2; i++)
    {
        pScanNode->ioOutArr[i]=pInitNode->
                               PublicElemData.elem.aioOut[i];

    }
    pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                      PublicElemData.pfGetScanNodeOutIOFunc;

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

EP_ELEM_IO *  RE_RealImageTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode)
{

    RealImage_Scan_Node_Type    *pTuyuanScanNode;

    pTuyuanScanNode=(RealImage_Scan_Node_Type    *)pScanNode->pTuyuan;

    if(unOutNum>=2)
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Get One RealImage  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  NULL;
    }
    return   &(pTuyuanScanNode->ioOutArr[unOutNum]);

}



/* 实虚部图元
 * Para:
 *     pElemScanNode, 扫描节点.
 * Return:
 *     NONE.
 */
void RE_RealImageTuyuanScan2(NODE *pElemScanNode)
{
    RealImage_Scan_Node_Type *pTuyuanNode;
    int iInterval;

    iInterval = pElemScanNode->nScanInterval;
    pTuyuanNode=(RealImage_Scan_Node_Type   *)pElemScanNode->pTuyuan;

    pTuyuanNode->ioOutArr[0].now.fVal = REAL(pTuyuanNode->pInArr0->now.xVal);
    pTuyuanNode->ioOutArr[1].now.fVal = IMAGE(pTuyuanNode->pInArr0->now.xVal);

    while (--iInterval)
    {
        pTuyuanNode->ioOutArr[0].recbuf[iInterval-1] =
            pTuyuanNode->ioOutArr[0].now;
        pTuyuanNode->ioOutArr[1].recbuf[iInterval-1] =
            pTuyuanNode->ioOutArr[1].now;
    }
}

/*    算法图元包裹化后的扫描函数**/
/*    功能:首先调用用户开发的算法扫描函数
           然后处理其他工作.
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***********************/
/*    返回值，无   */


void     RE_RealImageTuyuanScan(int  iInterval,NODE  *pElemScanNode)
{
    RealImage_Scan_Node_Type  *  pTuyuanNode;
    pTuyuanNode=(RealImage_Scan_Node_Type   *)pElemScanNode->pTuyuan;

    pTuyuanNode->ioOutArr[0].now.fVal=REAL(pTuyuanNode->pInArr0->now.xVal);
    pTuyuanNode->ioOutArr[1].now.fVal=IMAGE(pTuyuanNode->pInArr0->now.xVal);

    //if(iInterval<=MAX_REC_RESULT_NUM)
    {
        while(--iInterval)
        {
            pTuyuanNode->ioOutArr[0].recbuf[iInterval-1]=
                pTuyuanNode->ioOutArr[0].now;
            pTuyuanNode->ioOutArr[1].recbuf[iInterval-1]=
                pTuyuanNode->ioOutArr[1].now;
        }
    }
}
