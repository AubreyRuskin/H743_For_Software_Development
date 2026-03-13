/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_MinTuyuan.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的最小值元件的代码实现                       */
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
/*         张云       2005.11.10              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#include <vxWorks.h>

#include   "RE_MinTuyuan.h"

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

EP_STATUS   RE_MinTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                     TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
                                     NODE ** pRtElemInitNodePointer)
{
    NODE  *pNode;
    Min_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    int  i;

    unsigned  char      ucCurLuboSetFlag;
    unsigned   char     ucCurFlagSetFlag;
    unsigned   char     ucCurYaoceSetFlag;
    unsigned char       ucCurMeasureSetFlag;

    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoceID)[MAX_LOGID_STR_LEN+1];

    uint8_t   ucSelectCount;

    BOOL  bOtherInitSuccess;

    unsigned   long   TempLong;
    unsigned  short   *  punCurInSourceSeqNo;
    unsigned  char   *   pucInSourceOutputNo;

    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *    pbCurYaoceSetFlag;
    char   (*  pstrCurMeasureID)[MAX_LOGID_STR_LEN+1];	/* 临时变量 */
    BOOL  *   pbCurMeasureSetFlag;
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
        LOG_Dbg_Msg("Min  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pInitNode=(Min_Init_Node_Type    *)malloc
              (sizeof(Min_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("Min  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_MIN;
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

    /* 读取信号类型 */
    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&(pInitNode->ucSelectSignalType));
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    if(IS_REAL_SIG(pInitNode->ucSelectSignalType))
    {
        pInitNode->ucSignalDataType=FLOAT_VALUE_TYPE;
    }
    else if(IS_SIGNED_INTEGER_SIG(pInitNode->ucSelectSignalType))
    {
        pInitNode->ucSignalDataType=SIGNED_32INT_VALUE_TYPE;
    }
    else if(IS_UNSIGNED_INTEGER_SIG(pInitNode->ucSelectSignalType))
    {
        pInitNode->ucSignalDataType=UNSIGNED_32INT_VALUE_TYPE;
    }
    else
    {
        LOG_Dbg_Msg("Error, Min Tuyuan Input Signal Type  is not  expected!\n"
                    ,0,0,0,0,0,0);
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:读取最小值图元的输入信号类型不是预期类型\n"
                       ,0,0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:read min  element input signal type is not expected\n"
                       ,0,0);
        }
        assert(FALSE);
        return   EP_SYS_ERR;
    }
    /* 读取4保留字节  */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,4);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    /* 读取信号路数 */
    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&(pInitNode->ucSelectCount));
    /*确保路数未出界  */
    ucSelectCount=pInitNode->ucSelectCount;

    if((ucSelectCount<2)||(ucSelectCount>MAX_MAX_INPUT_COUNT))
    {
        LOG_Dbg_Msg("error,Min  Tuyuan  Input  Count  is  out of  range!\n",0,0,0,0,0,0);
        assert(FALSE);  /* 若非以上类型,则告警   */
        return  EP_SYS_ERR;
    }
    /* 读取4保留字节  */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,4);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    /*输入个数赋值给节点, */
    pInitNode->PublicElemData.elem.unInNum=
        pInitNode->ucSelectCount;
    /*循环读取每个输入信息*/
    punCurInSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucInSourceOutputNo=pInitNode->aucInSourceOutputNumArr;

    for(i=0; i<pInitNode->PublicElemData.elem.unInNum; i++)
    {
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
        punCurInSourceSeqNo++;
        pucInSourceOutputNo++;
    }/*所有输入循环处理结束  */

    /*赋值输出给节点 ， */

    /* 该扫描节点的输出个数 */
    pInitNode->PublicElemData.elem.ucOutNum=1;

    /*循环读取所有输出信息   */
    pCurElemOutput=pInitNode->PublicElemData.elem.aioOut;

    pbCurLuboSetFlag=pInitNode->PublicElemData.abLuboFlagArr;
    pbCurFlagSetFlag=pInitNode->PublicElemData.abFlagSetFlagArr;
    pbCurYaoceSetFlag=pInitNode->PublicElemData.abYaoceFlagArr;
    /* 定义用指向数组的指针，这里不能用双重指针，因为2维字符数组还未初始化 */
    pstrCurLuboID=pInitNode->aStrLuboIDArr;
    pstrCurFlagID=pInitNode->aStrFlagIDArr;
    pstrCurYaoceID=pInitNode->aStrYaoceIDArr;
    pbCurMeasureSetFlag=pInitNode->PublicElemData.abMeasureFlagArr;
    pstrCurMeasureID=pInitNode->aStrMeasureIDArr;

    for(i=0; i<1; i++)
    {

        /* 设置输出信号类型 */
        pCurElemOutput->ucAttrib=pInitNode->ucSelectSignalType;

        RE_InitElemIONowValue
        (pCurElemOutput);/*初始化该输出当前值*/
        pCurElemOutput->pvCh=NULL;/*通道句柄为空*/
        pCurElemOutput->ucType=0xFF;/*中间结果  */

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

            LOG_Dbg_Msg("Read  Min Tuyuan  Lubo  Flag   from  Logrp  SeqFile  error!\n",0,0,0,0,0,0);
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
            LOG_Dbg_Msg("Read Min Tuyuan  FlagSet  Flag   from  Logrp  SeqFile  error!\n",0,0,0,0,0,0);
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
            LOG_Dbg_Msg("Read  Min  Tuyuan  Yaoce  Flag   from  Logrp  SeqFile  error!\n",0,0,0,0,0,0);
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
            LOG_Dbg_Msg("Read  Min  Tuyuan  MeasureValue  Flag   from  Logrp  SeqFile  error!\n",0,0,0,0,0,0);
            return  EP_SYS_ERR;
        }
        pbCurMeasureSetFlag++;
        pstrCurMeasureID++;


        /* 为了提高效率,不用数组 */
        pCurElemOutput++;

        pbCurLuboSetFlag++;
        pbCurFlagSetFlag++;
        pbCurYaoceSetFlag++;
        pstrCurLuboID++;
        pstrCurFlagID++;
        pstrCurYaoceID++;
    }/*所有输出循环处理结束   */

    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_MinTuyuanReadFileOtherInit
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

EP_STATUS   RE_MinTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                 LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    Min_Init_Node_Type    *pInitNode;
    NODE  *  pCurSourceNode;

    uint8_t   ucGrpCurInSignalType;
    uint16_t   * punCurInSignalSourceSeqNo;
    uint8_t   *  pucCurInSignalSourceOutNo;
    EP_ELEM_IO *  pInElem;
    int  i;
    EP_STATUS  SetCurOutAttribResult;

    EP_ELEM_IO  *  pCurElemOutput;

    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoceID)[MAX_LOGID_STR_LEN+1];
    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *   pbCurYaoceSetFlag;
    BOOL  *   pbCurMeasureSetFlag;	/* 临时变量 */
    char   (*  pstrCurMeasureID)[MAX_LOGID_STR_LEN+1];
    uint8_t   ucCurInSignalType;

    pInitNode=(Min_Init_Node_Type    *)pElemInitNode->pTuyuan;
    ucGrpCurInSignalType=pInitNode->ucSelectSignalType;
    punCurInSignalSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucCurInSignalSourceOutNo=pInitNode->aucInSourceOutputNumArr;
    /* 循环设定图元的输入源指针  */
    for(i=0; i<(pInitNode->PublicElemData.elem.unInNum); i++)
    {

        pCurSourceNode=RE_LstNth(pGrpScanNodeList,
                                 ((*punCurInSignalSourceSeqNo)+1));/* 注意序号从1开始 */

        if(pCurSourceNode==NULL)
        {
            LOG_Dbg_Msg("error,Get  Min  Tuyuan  Input Source  Init  failure!\n",0,0,0,0,0,0);
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }
        /* 获得并设置当前输入源的指针 */

        pInElem=(*  RE_GetScanNodeGetOutIOFunc(pCurSourceNode))
                ((*pucCurInSignalSourceOutNo),pCurSourceNode);

        /*  确保该图元输入在来源图元的输出号没有出界 */
        if(pInElem==NULL)
        {

            LOG_Dbg_Msg("error,Get  Min  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }
        /*  设置该IO指针到扫描节点相应的输入  */

        SetCurOutAttribResult=RE_MinTuyuanSetInputIO
                              (i,pInElem,pElemScanNode);
        if(SetCurOutAttribResult!=EP_SUCCESS)
        {

            LOG_Dbg_Msg("error,Set  Min  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }
        /*比较信号类型是否匹配  */

        ucCurInSignalType=pInElem->ucAttrib;

        if(ucCurInSignalType!=ucGrpCurInSignalType)
        {
            LOG_Dbg_Msg("error,Min Tuyuan One  Input  Signal Type  can't  Match!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        punCurInSignalSourceSeqNo++;
        pucCurInSignalSourceOutNo++;
    }


    /* 循环设定图元的输出属性*/
    pCurElemOutput=RE_MinTuyuanGetOutIO
                   (0,pElemScanNode);

    /*   */
    if(pCurElemOutput==NULL)
    {
        LOG_Dbg_Msg("error,Can't  Get   Min  Tuyuan One Output  info!\n",0,0,0,0,0,0);
        assert(FALSE);  /* 若非以上类型,则告警   */
        return  EP_SYS_ERR;

    }
    pbCurLuboSetFlag=pInitNode->PublicElemData.abLuboFlagArr;
    pbCurFlagSetFlag=pInitNode->PublicElemData.abFlagSetFlagArr;
    pbCurYaoceSetFlag=pInitNode->PublicElemData.abYaoceFlagArr;
    /* 定义用指向数组的指针，这里未用双重指针, */
    pstrCurLuboID=pInitNode->aStrLuboIDArr;
    pstrCurFlagID=pInitNode->aStrFlagIDArr;
    pstrCurYaoceID=pInitNode->aStrYaoceIDArr;
    pbCurMeasureSetFlag=pInitNode->PublicElemData.abMeasureFlagArr;
    pstrCurMeasureID=pInitNode->aStrMeasureIDArr;

    {
        extern VALUE_TYPE gTmpRecbuf[MAX_REC_TEMP_BUFSIZE];
        pCurElemOutput->recbuf=gTmpRecbuf;
    }

    if(bPartGrpRunFlag)
    {
        /*若该图元所在的分图被投入，则设置录波，标志，遥测
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
                LOG_Dbg_Msg("Set  Min  Tuyuan  \'%s\'  Lubo  Value  Error!\n"
                            ,(int)(*pstrCurLuboID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:设置最小值图元的录波失败,录波标识为\'%s\'\n"
                               ,(int)(*pstrCurLuboID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:set min  element disturbance record id \'%s\' err\n"
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
                LOG_Dbg_Msg("Set  Min  Tuyuan  \'%s\'  Flag  Value  Error!\n"
                            ,(int)(*pstrCurFlagID),0,0,0,0,0);
                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:设置最小值图元的录波失败,录波标识为\'%s\'\n"
                               ,(int)(*pstrCurFlagID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:set min element disturbance record id \'%s\' err\n"
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
                LOG_Dbg_Msg("Set  Min  Tuyuan  \'%s\'  yaoce  Value  Error!\n"
                            ,(int)(*pstrCurYaoceID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:设置最小值图元的遥测失败,遥测标识为\'%s\'\n"
                               ,(int)(*pstrCurYaoceID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:set min element remote measurement id \'%s\' err\n"
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
                LOG_Dbg_Msg("Set  Min  Tuyuan  \'%s\'  measure  Value  Error!\n"
                            ,(int)(*pstrCurMeasureID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:设置最小值图元的测量失败,测量标识为\'%s\'\n"
                               ,(int)(*pstrCurMeasureID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "Set  Min   element  measurement id \'%s\'  Error\n"
                               ,(int)(*pstrCurMeasureID),0);
                }
                assert(FALSE);  /* 若非以上类型,则告警   */
                return   SetCurOutAttribResult;
            }
        }

    }
    return  EP_SUCCESS;
}




/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_MinTuyuanReadFileOtherInit
(Min_Init_Node_Type * pElemInitNodePointer)
{
    pElemInitNodePointer->PublicElemData.elem.ucType=RE_MIN;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_MinTuyuanScanInit;
    /* 设置获得IO输出的函数 */
    pElemInitNodePointer->PublicElemData.pfGetScanNodeOutIOFunc
        =RE_MinTuyuanGetOutIO;
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

EP_STATUS   RE_MinCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{
    NODE  *pNode;
    Min_Init_Node_Type    *pInitNode;
    Min_Scan_Node_Type    *pScanNode;

    pInitNode=(Min_Init_Node_Type    *)pElemInitNode->pTuyuan;
    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("Min Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pScanNode=(Min_Scan_Node_Type    *)malloc
              (sizeof(Min_Scan_Node_Type));
    if(pScanNode==NULL)
    {
        LOG_Dbg_Msg("Min  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    /*  设置图元类型*/
    switch(pInitNode->ucSignalDataType)
    {
        /* 设置扫描图元类型 */
        case  FLOAT_VALUE_TYPE :
        {
            pNode->ulTuyuanType=RE_FLOAT_MIN_SCAN;
        }
        break;
        case   SIGNED_32INT_VALUE_TYPE:
        {

            pNode->ulTuyuanType=RE_SIGNED_32INT_MIN_SCAN;
        }
        break;
        case  UNSIGNED_32INT_VALUE_TYPE:
        {
            pNode->ulTuyuanType=RE_UNSIGNED_32INT_MIN_SCAN;
        }
        break;
        default:
        {
            assert(FALSE);
            return   EP_SYS_ERR;
        }
    }

    pNode->pTuyuan=(void  *)pScanNode;
    pScanNode->ioOut=pInitNode->
                     PublicElemData.elem.aioOut[0];
    pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                      PublicElemData.pfGetScanNodeOutIOFunc;
    pScanNode->ucSelectCount=pInitNode->ucSelectCount;
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

EP_ELEM_IO *  RE_MinTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode)
{
    Min_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(Min_Scan_Node_Type    *)pScanNode->pTuyuan;
    if(unOutNum>=1)
    {
        /* 若越界，则出错 */
        LOG_Dbg_Msg("Get Min  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  NULL;
    }
    return   &(pTuyuanNode->ioOut);
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

EP_STATUS   RE_MinTuyuanSetInputIO
(uint8_t  ucInputNum,EP_ELEM_IO *  pElemIO,
 NODE  * pScanNode)
{

    Min_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(Min_Scan_Node_Type    *)pScanNode->pTuyuan;
    if(ucInputNum>=(pTuyuanNode->ucSelectCount))
    {
        LOG_Dbg_Msg("Set Min  Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  EP_SYS_ERR;

    }
    *(pTuyuanNode->apInArr+ucInputNum)=pElemIO;
    return   EP_SUCCESS;

}



/*    浮点型输入的图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_FloatMinTuyuanScan(int  iInterval,NODE *pElemScanNode)
{

    EP_ELEM_IO **  ppInputElemPointerArr;/* 输入指针的指针*/
    uint16_t   unSelectDims;/* 路数 */
    int  i;
    Min_Scan_Node_Type    *pScanNode;
    float   fMinValue;

    pScanNode=(Min_Scan_Node_Type    *)
              pElemScanNode->pTuyuan;
    unSelectDims=pScanNode->ucSelectCount;
    ppInputElemPointerArr=pScanNode->apInArr;
    fMinValue=(*ppInputElemPointerArr)->now.fVal;
    ++ppInputElemPointerArr;
    for(i=1; i<unSelectDims; i++)
    {
        /* 对每1选通路,进行操作 */

        if((*ppInputElemPointerArr)->now.fVal<fMinValue)
        {
            /*  */
            fMinValue=(*ppInputElemPointerArr)->now.fVal;

        }
        ++ppInputElemPointerArr;
    }
    pScanNode->ioOut.now.fVal=fMinValue;
    //if(iInterval<=MAX_REC_RESULT_NUM)
    {
        while(--iInterval)
        {
            pScanNode->ioOut.recbuf[iInterval-1]=pScanNode->ioOut.now;
        }
    }
    return;

}

/* 浮点最小图元
 * Para:
 *     pElemScanNode, 扫描节点.
 * Return:
 *     NONE.
 */
void     RE_FloatMinTuyuanScan2(NODE *pElemScanNode)
{

    EP_ELEM_IO **  ppInputElemPointerArr;/* 输入指针的指针*/
    uint16_t   unSelectDims;/* 路数 */
    int  i;
    Min_Scan_Node_Type    *pScanNode;
    float   fMinValue;
    int  iInterval;

    iInterval = pElemScanNode->nScanInterval;

    pScanNode=(Min_Scan_Node_Type    *)
              pElemScanNode->pTuyuan;
    unSelectDims=pScanNode->ucSelectCount;
    ppInputElemPointerArr=pScanNode->apInArr;
    fMinValue=(*ppInputElemPointerArr)->now.fVal;
    ++ppInputElemPointerArr;
    for(i=1; i<unSelectDims; i++)
    {
        /* 对每1选通路,进行操作 */

        if((*ppInputElemPointerArr)->now.fVal<fMinValue)
        {
            /*  */
            fMinValue=(*ppInputElemPointerArr)->now.fVal;

        }
        ++ppInputElemPointerArr;
    }
    pScanNode->ioOut.now.fVal=fMinValue;
    //if(iInterval<=MAX_REC_RESULT_NUM)
    {
        while(--iInterval)
        {
            pScanNode->ioOut.recbuf[iInterval-1]=pScanNode->ioOut.now;
        }
    }
    return;

}

/*    无符号32位整数型输入的图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_Unsigned32IntMinTuyuanScan(int  iInterval,NODE *pElemScanNode)
{

    EP_ELEM_IO **  ppInputElemPointerArr;/* 输入指针的指针*/
    uint16_t   unSelectDims;/* 路数 */
    int  i;
    Min_Scan_Node_Type    *pScanNode;
    uint32_t    ulMinValue;

    pScanNode=(Min_Scan_Node_Type    *)
              pElemScanNode->pTuyuan;
    unSelectDims=pScanNode->ucSelectCount;
    ppInputElemPointerArr=pScanNode->apInArr;
    ulMinValue=(*ppInputElemPointerArr)->now.ulVal;
    ++ppInputElemPointerArr;
    for(i=1; i<unSelectDims; i++)
    {
        /* 对每1选通路,进行操作 */

        if((*ppInputElemPointerArr)->now.ulVal<ulMinValue)
        {
            /*  */
            ulMinValue=(*ppInputElemPointerArr)->now.ulVal;
        }
        ++ppInputElemPointerArr;
    }
    pScanNode->ioOut.now.ulVal=ulMinValue;
    //if(iInterval<=MAX_REC_RESULT_NUM)
    {
        while(--iInterval)
        {
            pScanNode->ioOut.recbuf[iInterval-1]=pScanNode->ioOut.now;
        }
    }
    return;

}

/* 无符号32位最小图元
 * Para:
 *     pElemScanNode, 扫描节点.
 * Return:
 *     NONE.
 */
void     RE_Unsigned32IntMinTuyuanScan2(NODE *pElemScanNode)
{

    EP_ELEM_IO **  ppInputElemPointerArr;/* 输入指针的指针*/
    uint16_t   unSelectDims;/* 路数 */
    int  i;
    Min_Scan_Node_Type    *pScanNode;
    uint32_t    ulMinValue;
    int  iInterval;

    iInterval = pElemScanNode->nScanInterval;

    pScanNode=(Min_Scan_Node_Type    *)
              pElemScanNode->pTuyuan;
    unSelectDims=pScanNode->ucSelectCount;
    ppInputElemPointerArr=pScanNode->apInArr;
    ulMinValue=(*ppInputElemPointerArr)->now.ulVal;
    ++ppInputElemPointerArr;
    for(i=1; i<unSelectDims; i++)
    {
        /* 对每1选通路,进行操作 */

        if((*ppInputElemPointerArr)->now.ulVal<ulMinValue)
        {
            /*  */
            ulMinValue=(*ppInputElemPointerArr)->now.ulVal;
        }
        ++ppInputElemPointerArr;
    }
    pScanNode->ioOut.now.ulVal=ulMinValue;
    //if(iInterval<=MAX_REC_RESULT_NUM)
    {
        while(--iInterval)
        {
            pScanNode->ioOut.recbuf[iInterval-1]=pScanNode->ioOut.now;
        }
    }
    return;

}





/*    有符号32位整数型输入的图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_Signed32IntMinTuyuanScan(int  iInterval,NODE *pElemScanNode)
{
    EP_ELEM_IO **  ppInputElemPointerArr;/* 输入指针的指针*/
    uint16_t   unSelectDims;/* 路数 */
    int  i;
    Min_Scan_Node_Type    *pScanNode;
    int32_t    lMinValue;

    pScanNode=(Min_Scan_Node_Type    *)
              pElemScanNode->pTuyuan;
    unSelectDims=pScanNode->ucSelectCount;
    ppInputElemPointerArr=pScanNode->apInArr;
    lMinValue=(*ppInputElemPointerArr)->now.lVal;
    ++ppInputElemPointerArr;
    for(i=1; i<unSelectDims; i++)
    {
        /* 对每1选通路,进行操作 */

        if((*ppInputElemPointerArr)->now.lVal<lMinValue)
        {
            /*  */
            lMinValue=(*ppInputElemPointerArr)->now.lVal;
        }
        ++ppInputElemPointerArr;
    }
    pScanNode->ioOut.now.lVal=lMinValue;

    //if(iInterval<=MAX_REC_RESULT_NUM)
    {
        while(--iInterval)
        {
            pScanNode->ioOut.recbuf[iInterval-1]=pScanNode->ioOut.now;
        }
    }
    return;
}

/* 有符号32位最小图元
 * Para:
 *     pElemScanNode, 扫描节点.
 * Return:
 *     NONE.
 */
void     RE_Signed32IntMinTuyuanScan2(NODE *pElemScanNode)
{
    EP_ELEM_IO **  ppInputElemPointerArr;/* 输入指针的指针*/
    uint16_t   unSelectDims;/* 路数 */
    int  i;
    Min_Scan_Node_Type    *pScanNode;
    int32_t    lMinValue;
    int  iInterval;

    iInterval = pElemScanNode->nScanInterval;

    pScanNode=(Min_Scan_Node_Type    *)
              pElemScanNode->pTuyuan;
    unSelectDims=pScanNode->ucSelectCount;
    ppInputElemPointerArr=pScanNode->apInArr;
    lMinValue=(*ppInputElemPointerArr)->now.lVal;
    ++ppInputElemPointerArr;
    for(i=1; i<unSelectDims; i++)
    {
        /* 对每1选通路,进行操作 */

        if((*ppInputElemPointerArr)->now.lVal<lMinValue)
        {
            /*  */
            lMinValue=(*ppInputElemPointerArr)->now.lVal;
        }
        ++ppInputElemPointerArr;
    }
    pScanNode->ioOut.now.lVal=lMinValue;

    //if(iInterval<=MAX_REC_RESULT_NUM)
    {
        while(--iInterval)
        {
            pScanNode->ioOut.recbuf[iInterval-1]=pScanNode->ioOut.now;
        }
    }
    return;
}