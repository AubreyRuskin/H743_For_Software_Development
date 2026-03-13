/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_SuanfaTuyuan.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该源代码文件定义了保护功能模块中的算法元件的实现                       */
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
/*         张云       2002.11.28              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#include   <vxWorks.h>
#include   "RE_SuanfaTuyuan.h"

EP_STATUS EDP_DoublePointProduce(EP_ELEMENT *pelm);
EP_STATUS EDP_DoublePointTreat(EP_ELEMENT *pelm);
EP_STATUS EDP_DoublePointTreat6(EP_ELEMENT *pelm);
EP_STATUS EDP_FT3Map(EP_ELEMENT *pelm);
EP_STATUS EDP_SuanfaWarnning(EP_ELEMENT *pelm);

EP_STATUS EDP_Send_SMV(EP_ELEMENT *pelm);

EP_STATUS EDP_AiGain_Adj(EP_ELEMENT *pelm);
/* 平台算法测试入口函数.
 * Para:
 *     pelm,图元.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, error.
 */
extern EP_STATUS EDP_Alg_Test(EP_ELEMENT *pelm);

EP_EXT_ELEM_MAP aextmap_sys[]=
{
    {"EDP_DoublePointProduce", EDP_DoublePointProduce},
    {"EDP_DoublePointTreat",	EDP_DoublePointTreat},
    {"EDP_DoublePointTreat6",EDP_DoublePointTreat6},
    {"EDP_FT3Map", EDP_FT3Map},
    {"EDP_SuanfaWarnning",EDP_SuanfaWarnning},
    {"EDP_Send_SMV",EDP_Send_SMV},
    {"EDP_Alg_Test", EDP_Alg_Test},  /* 平台算法测试 */
    {"EDP_AiGain_Adj",EDP_AiGain_Adj}
};

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

EP_STATUS   RE_SuanfaTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                        TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
                                        NODE ** pRtElemInitNodePointer)
{

    NODE  *pNode;
    EP_STATUS    InitResult;
    BOOL  bOtherInitSuccess;
    unsigned   long   TempLong;
    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoceID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoxinID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurVirtualChID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurMeasureID)[MAX_LOGID_STR_LEN+1];
    char (* pstrCurAOID)[MAX_LOGID_STR_LEN+1];		/* AO logic symbol. */

    uint8_t   *pucGrpOutSignalType;
    unsigned  char      ucCurLuboSetFlag;
    unsigned   char     ucCurFlagSetFlag;
    unsigned   char     ucCurYaoceSetFlag;
    unsigned   char     ucCurYaoxinSetFlag;
    unsigned   char     ucCurVirtualChSetFlag;
    unsigned   char     ucCurMeasureSetFlag;
    unsigned char ucCurAOChSetFlag;		/* AO channel flag. */

    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *   pbCurYaoceSetFlag;
    BOOL  *    pbCurYaoxinSetFlag;
    BOOL  *    pbCurVirtualChSetFlag;
    BOOL  *   pbCurMeasureSetFlag;
    BOOL *pbCurAOChSetFlag;	/* AO channel setting flag. */

    EP_ELEM_IO  *  pCurElemOutput;

    unsigned  char   *   pucCurInSignalType;
    unsigned  short   *  punCurInSourceSeqNo;
    unsigned  char   *   pucInSourceOutputNo;
    Suanfa_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    unsigned  long   nRealSuanfaStrLen;
    int  i;
    unsigned  char   ucBreakPointSetFlag;
    unsigned  char   ucOutputCount;
    unsigned  char   ucInputCount;
    /* 文件指针重新定位  */
    int  OpeSuccess;

    /* int  k; *//*2007-4-25日 张云修改，初始化RECBUF中内容的值，解决录波无效数据的BUG  */

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
        LOG_Dbg_Msg("Suanfa  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }


    pInitNode=(Suanfa_Init_Node_Type    *)malloc(sizeof(Suanfa_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("suanfa  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_SUANFA;
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

    /* 获得算法元件名长度 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 (fp,pInitNode->strSuanfaName,&nRealSuanfaStrLen);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    assert(nRealSuanfaStrLen<=MAX_ID_LEN);
    if(nRealSuanfaStrLen>MAX_ID_LEN)
    {
        return   EP_SYS_ERR;
    }
    /* 设置算法调试态断点设置标志  */
    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&ucBreakPointSetFlag);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    if(ucBreakPointSetFlag==1)
    {
        /*若标志为1,设置断点标志为真  */
        pInitNode->bBreakPointSetFlag=TRUE;
    }
    else
    {
        /*其他为假  */
        pInitNode->bBreakPointSetFlag=FALSE;
    }

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

    if(ucInputCount>MAX_SUANFA_INPUT_COUNT)
    {
        LOG_Dbg_Msg("Error,Suanfa  Tuyuan  Elem  Name\'%s\'  Input  Count  is  out of  range!\n"
                    ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:算法图元\'%s\'的输入个数越界\n"
                       ,(int)(pInitNode->strSuanfaName),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err: Algorithm diagram  \'%s\' input num overrun\n"
                       ,(int)(pInitNode->strSuanfaName),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */

        return  EP_SYS_ERR;

    }
    /*赋值给节点  */
    pInitNode->PublicElemData.elem.unInNum=(unsigned  short)ucInputCount;
    /*循环读取每个输入信息*/

    pucCurInSignalType=pInitNode->aucSourceSignalTypeArr;
    punCurInSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucInSourceOutputNo=pInitNode->aucInSourceOutputNumArr;


    for(i=0; i<ucInputCount; i++)
    {
        /*读取信号类型  */
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


    /* 读取图元输出个数 */
    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&ucOutputCount);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    if(ucOutputCount>MAX_OUTPUT_NUM)
    {

        LOG_Dbg_Msg("Error,Suanfa  Tuyuan Elem  Name\'%s\'  Output  Count  is  out of  range!\n"
                    ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图初始化错误:算法图元\'%s\'的输出个数越界\n"
                       ,(int)(pInitNode->strSuanfaName),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "Logic graph initialization error: Algorithm element %s output number is out of limited\n", (int)(pInitNode->strSuanfaName),0);
        }
        assert(FALSE);
        return  EP_SYS_ERR;

    }
    /*赋值给节点 ， */

    /* 该扫描节点的输出个数有可能还会被用户开发的算法初始化函数覆盖 */
    pInitNode->PublicElemData.elem.ucOutNum=ucOutputCount;
    /* 在初始化节点中保存逻辑图上的输出个数,以便和用户开发的算法
       设定的输出匹配 */
    pInitNode->unGrpTuyuanOutCount=ucOutputCount;

    /*循环读取所有输出信息   */

    pCurElemOutput=pInitNode->PublicElemData.elem.aioOut;


    pbCurLuboSetFlag=pInitNode->PublicElemData.abLuboFlagArr;
    pbCurFlagSetFlag=pInitNode->PublicElemData.abFlagSetFlagArr;
    pbCurYaoceSetFlag=pInitNode->PublicElemData.abYaoceFlagArr;
    pbCurYaoxinSetFlag=pInitNode->PublicElemData.abYaoxinFlagArr;
    pbCurMeasureSetFlag=pInitNode->PublicElemData.abMeasureFlagArr;
    pbCurAOChSetFlag=pInitNode->PublicElemData.abAOFlagArr;  /* If setting. */
    pbCurVirtualChSetFlag=pInitNode->abVirtualChFlagArr;


    /* 逻辑图上绘制的输出信号类型指针  */
    pucGrpOutSignalType=pInitNode->aucGrpOutSignalTypeArr;

    /* 定义用指向数组的指针，这里不能用双重指针，因为2维字符数组还未初始化 */

    pstrCurLuboID=pInitNode->aStrLuboIDArr;
    pstrCurFlagID=pInitNode->aStrFlagIDArr;
    pstrCurYaoceID=pInitNode->aStrYaoceIDArr;
    pstrCurYaoxinID=pInitNode->aStrYaoxinIDArr;
    pstrCurMeasureID=pInitNode->aStrMeasureIDArr;
    pstrCurAOID=pInitNode->aStrAOChIDArr;  /* ID */
    pstrCurVirtualChID=pInitNode->aStrVirtualChIDArr;


    for(i=0; i<ucOutputCount; i++)
    {
        /*读取信号类型,进行输出数据的初始化
          该信号类型数据可能会被用户的算法初始化函数覆盖掉  */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&(pCurElemOutput->ucAttrib));
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        /*保存逻辑图上的算法图元输出信号类型,
        用于和用户的定义进行比较  */
        *pucGrpOutSignalType=pCurElemOutput->ucAttrib;

        RE_InitElemIONowValue
        (pCurElemOutput);/*初始化该输出当前值*/
        pCurElemOutput->pvCh=NULL;/*通道句柄为空*/
        pCurElemOutput->ucType=0xFF;/*中间结果  */

        /*2008-1-25日 张云merge修改，初始化RECBUF中内容的值，解决录波无效数据的BUG ,注意不要越界 */
        /*已经在修改动态申请内存时初始化过了
        for(k=0;k<MAX_REC_RESULT_NUM-1;k++)
        {
             pCurElemOutput->recbuf[k].xVal=0.0+0.0i;
        }    */


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
            LOG_Dbg_Msg("Error, Read Suanfa Tuyuan Elem Name \'%s\' AO Flag Error!\n",
                        (int)(pInitNode->strSuanfaName), 0, 0, 0, 0, 0);

            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误: 读取算法图元\'%s\'的AO设置错误!\n",
                           (int)(pInitNode->strSuanfaName),0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err: read Algorithm element \'%s\' AO set err!\n",
                           (int)(pInitNode->strSuanfaName), 0);
            }

            assert(FALSE);  /* 若非以上类型,则告警 */

            return EP_SYS_ERR;
        }

        /*读取虚拟通道设置  */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&ucCurVirtualChSetFlag);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        if((ucCurVirtualChSetFlag)==1)
        {
            uint8_t   ucAISignalType,ucHandleAttrib;
            *pbCurVirtualChSetFlag=TRUE;
            bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                         (fp,(*pstrCurVirtualChID),&TempLong);
            if(!bReadSuccess)
            {
                return   EP_SYS_ERR;
            }
            /* 若是虚拟通道，则设置通道句柄 */
            pCurElemOutput->pvCh=RD_Get_Handle((*pstrCurVirtualChID),RD_LGC_AI_HDL);
            if(pCurElemOutput->pvCh==NULL)
            {
                /* 若句柄获得不成功，则出错  */

                LOG_Dbg_Msg("Error,Suanfa  Tuyuan  Elem  Name  \'%s\'  \n  One  Virtual  Channel  Output \'%s\'   Handle  Error!\n"
                            ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurVirtualChID),0,0,0,0);


                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得算法图元\'%s\'  \n的一个虚拟输出通道\'%s\'句柄错误\n"
                               ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurVirtualChID));
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp  rslv err:get Algorithm  element \'%s\'  \n one vt output ch \'%s\' handle err\n"
                               ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurVirtualChID));
                }
                assert(FALSE);
                return  EP_SYS_ERR;
            }
            ucAISignalType=pCurElemOutput->ucAttrib;

            /*判定信号类型是否一致  */
            ucHandleAttrib=AI_HND_TO_UNIT(pCurElemOutput->pvCh);

            if(ucHandleAttrib!=ucAISignalType)
            {
                LOG_Dbg_Msg("Error,Suanfa Tuyuan\'%s\'  \n  One  Virtual Channel Output \'%s\' Signal Type  isn't  Match  with  Config  Handle!\n"
                            ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurVirtualChID),0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:算法图元\'%s\'  \n的一个虚拟输出通道\'%s\'的信号类型错误\n"
                               ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurVirtualChID));
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:Algorithm  element \'%s\'  \n one vt output ch \'%s\' signal type err\n"
                               ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurVirtualChID));
                }

                assert(FALSE);  /* 若非以上类型,则告警   */
                return   EP_SYS_ERR;

            }

            pCurElemOutput->ucType=0;/*设置该虚拟通道为AI通道类型*/

            /* 判别当前设置为虚拟通道属性的输出的信号类型是否为允许的类型  */
            InitResult=RE_SuanfaTuyuanVirtualChOutSignalTypeCheck
                       (pCurElemOutput)  ;

            if(InitResult!=EP_SUCCESS)
            {
                LOG_Dbg_Msg("Error,Suanfa  Tuyuan  Elem  Name  \'%s\'  \n  One  Virtual  Channel  Output \'%s\'  Signal Type isn't  Expected !\n"
                            ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurVirtualChID),0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:算法图元\'%s\'  \n的一个虚拟输出通道\'%s\'的信号类型错误\n"
                               ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurVirtualChID));
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp  rslv err:Algorithm  element \'%s\'  \n one vt output ch \'%s\' signal type\n"
                               ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurVirtualChID));
                }
                assert(FALSE);  /* 若非以上类型,则告警   */

                return  EP_SYS_ERR;
            }

        }
        else  if((ucCurVirtualChSetFlag)==0)
        {
            *pbCurVirtualChSetFlag=FALSE;
            /* 将1个空串赋给字符数组,注意直接赋值非法 */
            strcpy((*pstrCurVirtualChID),"");

        }
        else
        {

            LOG_Dbg_Msg("Error, Read  Suanfa  Tuyuan  Elem  Name \'%s\'  \n  Virtual  Channel  Output   Flag  Error!\n"
                        ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取算法图元\'%s\'\n的虚拟输出通道的设置错误\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp  rslv  err: read Algorithm  element \'%s\'\n vt output channel set err\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }


            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
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

            LOG_Dbg_Msg("Error, Read  Suanfa  Tuyuan  Elem  Name \'%s\'    Lubo  Flag  Error!\n"
                        ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);


            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取算法图元\'%s\'的录波设置错误\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp  rslv  err:read Algorithm  element \'%s\' disturbance record set err\n"
                           ,(int)(pInitNode->strSuanfaName),0);
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



            LOG_Dbg_Msg("Error, Read  Suanfa  Tuyuan  Elem  Name \'%s\'    FlagSet  Flag  Error!\n"
                        ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取算法图元\'%s\'的标志设置错误\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp  rslv err:read Algorithm  element \'%s\' flag set  err\n"
                           ,(int)(pInitNode->strSuanfaName),0);
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


            LOG_Dbg_Msg("Error, Read  Suanfa  Tuyuan  Elem  Name \'%s\'    Yaoce  Flag  Error!\n"
                        ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取算法图元\'%s\'的遥测设置错误\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp  rslv err: read Algorithm  element \'%s\' remote measurement set  err\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }

            assert(FALSE);  /* 若非以上类型,则告警   */

            return  EP_SYS_ERR;
        }

        /*读取遥信设置   */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&ucCurYaoxinSetFlag);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        if((ucCurYaoxinSetFlag)==1)
        {
            *pbCurYaoxinSetFlag=TRUE;
            bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                         (fp,(*pstrCurYaoxinID),&TempLong);
            if(!bReadSuccess)
            {
                return   EP_SYS_ERR;
            }

        }
        else  if((ucCurYaoxinSetFlag)==0)
        {
            *pbCurYaoxinSetFlag=FALSE;
            strcpy((*pstrCurYaoxinID),"");
        }
        else
        {

            LOG_Dbg_Msg("Error, Read  Suanfa  Tuyuan  Elem  Name \'%s\'    Yaoxin  Flag  Error!\n"
                        ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:设置算法图元\'%s\'遥信设置错误\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp  rslv err: set Algorithm  element \'%s\' remote signal set  err\n"
                           ,(int)(pInitNode->strSuanfaName),0);
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


            LOG_Dbg_Msg("Error, Read  Suanfa  Tuyuan  Elem  Name \'%s\'    MeasureValue  Flag  Error!\n"
                        ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);


            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取算法图元\'%s\'的测量设置错误\n"
                           ,(int)(pInitNode->strSuanfaName),0);

            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp  rslv err: read Algorithm  element \'%s\' measurement set  err\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */

            return  EP_SYS_ERR;
        }


        /* 为了提高效率,不用数组 */
        pCurElemOutput++;

        pbCurLuboSetFlag++;
        pbCurFlagSetFlag++;
        pbCurYaoceSetFlag++;
        pbCurYaoxinSetFlag++;
        pbCurMeasureSetFlag++;
        pbCurAOChSetFlag++;	/* AO symbol. */
        pbCurVirtualChSetFlag++;


        pucGrpOutSignalType++;

        pstrCurLuboID++;
        pstrCurFlagID++;
        pstrCurYaoceID++;
        pstrCurYaoxinID++;
        pstrCurMeasureID++;
        pstrCurAOID++;       /* ID of AO. */
        pstrCurVirtualChID++;


    }/*所有输出循环处理结束   */


    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_SuanfaTuyuanReadFileOtherInit
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

BOOL      RE_SuanfaTuyuanReadFileOtherInit
(Suanfa_Init_Node_Type * pElemInitNodePointer)
{
    EP_STATUS   OpeResult;
    char   *  strSuanfaName;
    /*  设置图元类型*/

    pElemInitNodePointer->PublicElemData.elem.ucType=RE_SUANFA;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_SuanfaTuyuanScanInit;
    /* 设置获取扫描节点输出的指针的函数 */
    pElemInitNodePointer->PublicElemData.pfGetScanNodeOutIOFunc=
        RE_SuanfaTuyuanGetOutIO;
    /* 设置用户定义的扫描函数指针为空,等待用户初始化  */
    pElemInitNodePointer->PublicElemData.elem.Scan_Func=NULL;


    /* 查找并设置用户的算法入口函数指针 */
    strSuanfaName=pElemInitNodePointer->strSuanfaName;
    OpeResult=RE_SearchUserDevInitFunc
              (strSuanfaName,&pElemInitNodePointer->pfUserInit);

    if(OpeResult!=EP_SUCCESS)
    {
        /*  若不成功,则失败*/
        return  FALSE;
    }

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

EP_STATUS   RE_SuanfaTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                    LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    Suanfa_Init_Node_Type    *pInitNode;
    Suanfa_Scan_Node_Type    *pScanNode;
    NODE  *  pCurSourceNode;

    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *   pbCurYaoceSetFlag;
    BOOL  *   pbCurYaoxinSetFlag;
    BOOL  *   pbCurMeasureSetFlag;
    BOOL *pbCurAOSetFlag;		/* AO symbol. */

    uint8_t   *pucGrpOutSignalType;

    EP_STATUS  SetCurOutAttribResult;

    EP_ELEM_IO  *  pCurElemOutput;

    uint8_t  * pucCurInSignalType;
    uint16_t   * punCurInSignalSourceSeqNo;
    uint8_t   *  pucCurInSignalSourceOutNo;
    EP_ELEM_IO **  pInElemAddr;

    /* 定义用指向数组的指针，这里未用双重指针, */
    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoceID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoxinID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurMeasureID)[MAX_LOGID_STR_LEN+1];
    char (* pstrCurAOID)[MAX_LOGID_STR_LEN+1];

    void (*pfUserScanFunc)(struct tag_EP_ELEMENT *pelm);

    int  i;
    uint8_t   ucCurInSignalType;

    EP_STATUS   UserInitResult;

    pInitNode=(Suanfa_Init_Node_Type    *)pElemInitNode->pTuyuan;
    pScanNode=(Suanfa_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    pucCurInSignalType=pInitNode->aucSourceSignalTypeArr;
    punCurInSignalSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucCurInSignalSourceOutNo=pInitNode->aucInSourceOutputNumArr;
    pInElemAddr=pScanNode->apInArr;
    /* 循环设定图元的输入源指针  */
    for(i=0; i<(pScanNode->elem.unInNum); i++)
    {

        pCurSourceNode=RE_LstNth(pGrpScanNodeList,
                                 ((*punCurInSignalSourceSeqNo)+1));/* 注意序号从1开始 */

        if(pCurSourceNode==NULL)
        {
            LOG_Dbg_Msg("Error,Get  Suanfa  Tuyuan  Elem  Name \'%s\'  Input Source  Init  failure!\n"
                        ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得算法图元\'%s\'输入来源错误\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic  grp rslv err:get Algorithm  element \'%s\' input source err\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }
        /* 获得并设置当前输入源的指针 */

        *pInElemAddr=(*  RE_GetScanNodeGetOutIOFunc(pCurSourceNode))
                     ((*pucCurInSignalSourceOutNo),pCurSourceNode);

        /*  确保该图元输入在来源图元的输出号没有出界 */
        if((*pInElemAddr)==NULL)
        {
            LOG_Dbg_Msg("Error,Get  Suanfa  Tuyuan  Elem  Name \'%s\'  input  info  failure!\n"
                        ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得算法图元\'%s\'输入来源失败\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp  rslv err:get  Algorithm  element \'%s\' input source failure\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }

        /*比较信号类型是否匹配  */

        ucCurInSignalType=(*pInElemAddr)->ucAttrib;

        if(ucCurInSignalType!=(*pucCurInSignalType))
        {

            LOG_Dbg_Msg("Error,Suanfa  Tuyuan  Elem  Name  \'%s\'   One  Input  Signal Type  can't  Match!\n"
                        ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:算法图元\'%s\'的一个输入的信号类型错误\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic  grp  rslv  err:Algorithm  element \'%s\' one input signal type err\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }



            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        pucCurInSignalType++;
        punCurInSignalSourceSeqNo++;
        pucCurInSignalSourceOutNo++;
        pInElemAddr++;
    }

    /* 进行用户定义的算法入口函数初始化调用  */

    UserInitResult=(*  pInitNode->pfUserInit)(&(pScanNode->elem));

    if(UserInitResult!=EP_SUCCESS)
    {
        LOG_Dbg_Msg("Error,Suanfa  Tuyuan  Elem  Name \'%s\'  User  Develeped  Entry  Init  Function  Return  Error !\n"
                    ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:算法图元\'%s\'的用户入口函数返回错误\n"
                       ,(int)(pInitNode->strSuanfaName),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp  rslv err:Algorithm  element \'%s\' user entry init func return err\n"
                       ,(int)(pInitNode->strSuanfaName),0);
        }


        assert(FALSE);  /* 若非以上类型,则告警   */
        return  UserInitResult;
    }

    /* 检查用户是否将自己开发的扫描函数指针赋给节点*/
    pfUserScanFunc=pScanNode->elem.Scan_Func;

    if(pfUserScanFunc==NULL)
    {
        LOG_Dbg_Msg("Error,Develop  User  doesn't  assign  Scan  Function  Pointer \n to  Suanfa  Tuyuan  Elem  Name \'%s\' !\n"
                    ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);

        if(ENG_MODE == 0)
        {

            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:算法图元\'%s\'没有给算法扫描函数指针赋初值\n"
                       ,(int)(pInitNode->strSuanfaName),0);

        }

        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp  rslv err:Algorithm   element \'%s\' do not assign scan  func pointer\n"
                       ,(int)(pInitNode->strSuanfaName),0);
        }
        assert(FALSE);
        return  EP_SYS_ERR;
    }


    pCurElemOutput=pScanNode->elem.aioOut;


    pbCurLuboSetFlag=pInitNode->PublicElemData.abLuboFlagArr;
    pbCurFlagSetFlag=pInitNode->PublicElemData.abFlagSetFlagArr;
    pbCurYaoceSetFlag=pInitNode->PublicElemData.abYaoceFlagArr;
    pbCurYaoxinSetFlag=pInitNode->PublicElemData.abYaoxinFlagArr;
    pbCurMeasureSetFlag=pInitNode->PublicElemData.abMeasureFlagArr;
    pbCurAOSetFlag=pInitNode->PublicElemData.abAOFlagArr;

    /*逻辑图上绘制的输出信号类型指针  */
    pucGrpOutSignalType=pInitNode->aucGrpOutSignalTypeArr;

    pstrCurLuboID=pInitNode->aStrLuboIDArr;
    pstrCurFlagID=pInitNode->aStrFlagIDArr;
    pstrCurYaoceID=pInitNode->aStrYaoceIDArr;
    pstrCurYaoxinID=pInitNode->aStrYaoxinIDArr;
    pstrCurMeasureID=pInitNode->aStrMeasureIDArr;
    pstrCurAOID=pInitNode->aStrAOChIDArr;

    /* 如果算法的逻辑图绘制的输出个数和
       算法内部定义的输出个数不匹配,则出错 */

    if((pInitNode->unGrpTuyuanOutCount)!=
            (pScanNode->elem.ucOutNum))
    {
        LOG_Dbg_Msg("Error,Suanfa  Tuyuan  Elem  Name \'%s\' \n Output  Count  can't  Match  Logrp   Defined  Output  Count!\n"
                    ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:算法图元\'%s\'的输出个数不能同逻辑图中定义的输出个数匹配\n"
                       ,(int)(pInitNode->strSuanfaName),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:Algorithm  element \'%s\' output num can not match with config\n"
                       ,(int)(pInitNode->strSuanfaName),0);
        }

        assert(FALSE);  /* 若非以上类型,则告警   */
        return  EP_SYS_ERR;

    }


    for(i=0; i<pScanNode->elem.ucOutNum; i++)
    {
        /* 判定逻辑图上绘制的输出信号类型
           和用户自己开发定义的输出信号类型
           比较,若不匹配,则出错  */

        if((*pucGrpOutSignalType)!=(pCurElemOutput->ucAttrib))
        {
            /* 若该输出的信号类型不匹配,则出错 */

            LOG_Dbg_Msg("Error,  Suanfa  Tuyuan  Elem  Name \'%s\'  \n   One  Output  Signal Type  can't  Match!\n"
                        ,(int)(pInitNode->strSuanfaName),0,0,0,0,0);


            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:算法图元\'%s\'  \n的一个输出信号类型不匹配\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp  rslv err:Algorithm  element \'%s\'  \n one output signal type not match\n"
                           ,(int)(pInitNode->strSuanfaName),0);
            }

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }

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
                    LOG_Dbg_Msg(" Error,Set  Suanfa  Tuyuan  Elem  Name \'%s\'  \n Output  Lubo  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurLuboID),0,0,0,0);

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:算法图元\'%s\'\n的输出的录波标识为\'%s\'的录波设置错误\n"
                                   ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurLuboID));
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp  rslv err:Algorithm  element \'%s\'\n output disturbance record id \'%s\'  set failure\n"
                                   ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurLuboID));
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
                    LOG_Dbg_Msg(" Error,Set  Suanfa  Tuyuan  Elem  Name \'%s\'  \n Output  Flag  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurFlagID),0,0,0,0);

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:算法图元\'%s\'的输出的标志标识为\'%s\'的标志设置错误\n"
                                   ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurFlagID));
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp rslv err: Algorithm  element \'%s\' output flag  id \'%s\'  set  err\n"
                                   ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurFlagID));
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
                    LOG_Dbg_Msg(" Error,Set  Suanfa  Tuyuan  Elem  Name \'%s\'  \n Output  Yaoce  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurYaoceID),0,0,0,0);
                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:算法图元\'%s\'的输出的遥测标识为\'%s\'的遥测设置错误\n"
                                   ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurYaoceID));
                    }

                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp  rslv err:Algorithm  element \'%s\' out remote measurement id \'%s\' set  err\n"
                                   ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurYaoceID));
                    }


                    assert(FALSE);  /* 若非以上类型,则告警   */
                    return   SetCurOutAttribResult;
                }
            }

            /*设置输出为遥信量  */

            if((*pbCurYaoxinSetFlag))
            {
                /* 若录波标志为真 */
                SetCurOutAttribResult=SCI_Init_Add_New_Yaoxin_Signal
                                      ((*pstrCurYaoxinID),pCurElemOutput,
                                       pInitNode->PublicElemData.nScanTaskNo);

                if(SetCurOutAttribResult!=EP_SUCCESS)
                {
                    LOG_Dbg_Msg(" Error,Set  Suanfa  Tuyuan  Elem  Name \'%s\'  \n Output  Yaoxin  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurYaoxinID),0,0,0,0);
                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:算法图元\'%s\'的输出的遥信标识为\'%s\'的遥信设置错误\n"
                                   ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurYaoxinID));
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp  rslv err:Algorithm  element \'%s\' output remote signal id \'%s\' set err\n"
                                   ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurYaoxinID));
                    }
                    assert(FALSE);  /* 若非以上类型,则告警   */
                    return   SetCurOutAttribResult;
                }
            }

            /*设置输出为测量量  */

            if((*pbCurMeasureSetFlag))
            {
                /* 若标志为真 */
                SetCurOutAttribResult=SCI_Init_Add_New_Measure_Signal
                                      ((*pstrCurMeasureID),pCurElemOutput,
                                       pInitNode->PublicElemData.nScanTaskNo);

                if(SetCurOutAttribResult!=EP_SUCCESS)
                {
                    LOG_Dbg_Msg(" Error,Set  Suanfa  Tuyuan  Elem  Name \'%s\'  \n Output  Measure  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurMeasureID),0,0,0,0);

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:算法图元\'%s\'的输出的测量标识为\'%s\'的测量设置错误\n"
                                   ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurMeasureID));
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp  rslv err:Algorithm  element \'%s\' output measurement id \'%s\' set err\n"
                                   ,(int)(pInitNode->strSuanfaName),(int)(*pstrCurMeasureID));
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
                    LOG_Dbg_Msg("Error, Set Suanfa Tuyuan Elem Name \'%s\' \n Output AO ID is \'%s\' Value Error\n",
                                (int)(pInitNode->strSuanfaName), (int)(*pstrCurAOID), 0, 0, 0, 0);

                    if (ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:算法图元\'%s\'的输出的AO标识为\'%s\'的AO设置错误!\n",
                                   (int)(pInitNode->strSuanfaName), (int)(*pstrCurAOID));
                    }
                    else if (ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp  rslv err:Algorithm  element \'%s\' output measurement id \'%s\' set err\n",
                                   (int)(pInitNode->strSuanfaName), (int)(*pstrCurAOID));
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
        pbCurYaoxinSetFlag++;
        pbCurMeasureSetFlag++;
        pbCurAOSetFlag++;

        pucGrpOutSignalType++;

        pstrCurLuboID++;
        pstrCurFlagID++;
        pstrCurYaoceID++;
        pstrCurYaoxinID++;
        pstrCurMeasureID++;
        pstrCurAOID++;

    }

    return  EP_SUCCESS;
}









/*    根据元件名，查找用户开发的算法入口函数指针
      参数   strSuanfaName，算法元件名
             ppfRtFunc，供返回入口函数指针
      返回值，EP_STATUS
              成功  EP_SUCCESS;
              失败,其他
*/
EP_STATUS   RE_SearchUserDevInitFunc(char  *strSuanfaName,
                                     USER_INIT_FUNC_TYPE   *  ppfRtFunc)
{
    /*获得算法映射表大小   */

    long   nSearchResult;
    long   nMapCount=0;
    long   nMapCount_sys=0;
    int   nMapTableSize;
    EP_EXT_ELEM_MAP   *pMapMember;
    int  i;
    EP_EXT_ELEM_MAP   *pMapMember_sys;

    if(pSuanfaDebugEntryFunc_g==NULL)
    {
        /* 若调试入口函数的指针为空，则表示是系统首次运行逻辑图 */
        pMapMember=aextmap;
        nMapTableSize=EP_Ext_Elem_Num();
    }
    else
    {
        /*否则，则表示是download 运行*/
        /* 获得算法元件映射表的相关信息 */
        pMapMember=SuanfaElemMapArrayAddr_g;
        nMapTableSize=nSuanfaElemMapCount_g;
    }


    for(i=0; i<nMapTableSize; i++)
    {
        nSearchResult=strcmp(pMapMember->acElemName,strSuanfaName);
        if(nSearchResult==0)
        {
            /* 若找到,则累加  */
            nMapCount++;
            *ppfRtFunc=pMapMember->Init_Func;
        }
        pMapMember++;
    }


    if(nMapCount<1)
    {
        /* 若找不到同名的,则出错 */
        pMapMember_sys=aextmap_sys;

        for(i=0; i<sizeof(aextmap_sys)/sizeof(aextmap_sys[0]); i++)
        {

            nSearchResult=strcmp(pMapMember_sys->acElemName,strSuanfaName);
            if(nSearchResult==0)
            {
                /* è??òμ?,?òà??ó  */
                nMapCount_sys++;
                *ppfRtFunc=pMapMember_sys->Init_Func;
            }
            pMapMember_sys++;
        }
        if(nMapCount_sys==1)
            return  EP_SUCCESS;



        /* 若找不到同名的,则出错 */
        LOG_Dbg_Msg("Error,Can't find one  SuanfaTuyuan  initfunc by  SuanfaName \'%s\'  from  SuanfaMapTable!\n",
                    (int)strSuanfaName,0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:算法图元\'%s\'不能从算法匹配表中找到\n",
                       (int)strSuanfaName,0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:Algorithm  element \'%s\' can not find in Algorithm match table\n",
                       (int)strSuanfaName,0);
        }


        return  EP_SYS_ERR;
    }
    else  if(nMapCount>1)
    {
        /* 若找多个同名的,则出错*/
        LOG_Dbg_Msg("Error,find  morethan  one  SuanfaTuyuan  initfunc by  SuanfaName   \'%s\'   from  SuanfaMapTable!\n",(int)strSuanfaName,0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:算法图元\'%s\'在算法匹配表中被多次找到\n",
                       (int)strSuanfaName,0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:Algorithm  element \'%s\' find multi in Algorithm match table\n",
                       (int)strSuanfaName,0);
        }

        return  EP_SYS_ERR;

    }

    return  EP_SUCCESS;
}



/*   当输出为虚拟通道类型时,
     检查该输出信号类型是否为所允许的类型
     参数  pCheckedOutElem,待检查的输出的指针
     返回值  EP_STATUS
             无错误,则返回真,
             否则,返回假
*/

EP_STATUS   RE_SuanfaTuyuanVirtualChOutSignalTypeCheck
(EP_ELEM_IO  *pCheckedOutElem)
{

    if(((pCheckedOutElem->ucAttrib)!=REAL_FORM_DIANLIU_SIGNAL)
            &&((pCheckedOutElem->ucAttrib)!=COMPLEX_FORM_DIANLIU_SIGNAL)
            &&((pCheckedOutElem->ucAttrib)!=VALUE_ANGLE_FORM_DIANLIU_SIGNAL)
            &&((pCheckedOutElem->ucAttrib)!=REAL_FORM_DIANLIU_SIGNAL_KA)
            &&((pCheckedOutElem->ucAttrib)!=REAL_FORM_DIANLIU_SIGNAL_MA)
            &&((pCheckedOutElem->ucAttrib)!=COMPLEX_FORM_DIANLIU_SIGNAL_KA)
            &&((pCheckedOutElem->ucAttrib)!=VALUE_ANGLE_FORM_DIANLIU_SIGNAL_KA)
            &&((pCheckedOutElem->ucAttrib)!=REAL_FORM_DIANYA_SIGNAL)
            &&((pCheckedOutElem->ucAttrib)!=COMPLEX_FORM_DIANYA_SIGNAL)
            &&((pCheckedOutElem->ucAttrib)!=VALUE_ANGLE_FORM_DIANYA_SIGNAL)
            &&((pCheckedOutElem->ucAttrib)!=REAL_FORM_DIANYA_SIGNAL_KV)
            &&((pCheckedOutElem->ucAttrib)!=COMPLEX_FORM_DIANYA_SIGNAL_KV)
            &&((pCheckedOutElem->ucAttrib)!=VALUE_ANGLE_FORM_DIANYA_SIGNAL_KV)
            &&((pCheckedOutElem->ucAttrib)!=REAL_FORM_ZUKANG_SIGNAL)
            &&((pCheckedOutElem->ucAttrib)!=COMPLEX_FORM_ZUKANG_SIGNAL)
            &&((pCheckedOutElem->ucAttrib)!=VALUE_ANGLE_FORM_ZUKANG_SIGNAL)
            &&((pCheckedOutElem->ucAttrib)!=REAL_FORM_ZUKANG_SIGNAL_KO)
            &&((pCheckedOutElem->ucAttrib)!=COMPLEX_FORM_ZUKANG_SIGNAL_KO)
            &&((pCheckedOutElem->ucAttrib)!=VALUE_ANGLE_FORM_ZUKANG_SIGNAL_KO))
    {
        return  EP_SYS_ERR;
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

EP_STATUS   RE_SuanfaCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{
    NODE  *pNode;
    int  i;
    Suanfa_Scan_Node_Type    *pScanNode;
    Suanfa_Init_Node_Type    *pInitNode;

    pInitNode=(Suanfa_Init_Node_Type    *)pElemInitNode->pTuyuan;

    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("Suanfa  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pScanNode=(Suanfa_Scan_Node_Type    *)malloc(sizeof(Suanfa_Scan_Node_Type));
    if(pScanNode==NULL)
    {
        LOG_Dbg_Msg("suanfa  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }

    /* 设置实际扫描图元类型 */
    pNode->ulTuyuanType=RE_SUANFA_SCAN;
    pNode->pTuyuan=(void  *)pScanNode;
    pScanNode->elem.pchart=pInitNode->PublicElemData.elem.pchart;
    pScanNode->elem.ucType=pInitNode->PublicElemData.elem.ucType;
    pScanNode->elem.ucOutNum=pInitNode->PublicElemData.elem.ucOutNum;
    pScanNode->elem.unInNum=pInitNode->PublicElemData.elem.unInNum;
    for(i=0; i<MAX_OUTPUT_NUM; i++)
    {
        pScanNode->elem.aioOut[i]=pInitNode->
                                  PublicElemData.elem.aioOut[i];

    }
    pScanNode->elem.Scan_Func=pInitNode->
                              PublicElemData.elem.Scan_Func;
    pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                      PublicElemData.pfGetScanNodeOutIOFunc;

    /*  设置图元输入数组指针 */
    pScanNode->elem.ppioIn=pScanNode->apInArr;
    /* 设置断点设置标志 */
    pScanNode->bBreakPointSetFlag=pInitNode->bBreakPointSetFlag;

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

EP_ELEM_IO *  RE_SuanfaTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode)
{

    Suanfa_Scan_Node_Type    *pTuyuanScanNode;

    pTuyuanScanNode=(Suanfa_Scan_Node_Type    *)pScanNode->pTuyuan;

    if(unOutNum>=(pTuyuanScanNode->elem.ucOutNum))
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Get One suanfa  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  NULL;

    }
    return   &(pTuyuanScanNode->elem.aioOut[unOutNum]);


}



