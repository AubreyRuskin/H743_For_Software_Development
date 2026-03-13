/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_OuterOrderTuyuan.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的外部命令元件的代码文件                        */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      ghx                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                    */
/*                                                                              */
/*     Hongxia Gao    2002.12.12              创建文件1.0版本              */
/*                                                                                 */
/*                                                                              */
/********************************************************************************/

#include <vxWorks.h>

#include  "RE_OuterOrderTuyuan.h"

/*判断各类外部命令的各个输出信号类型是否符合要求
iOrderType    命令类型
iOutputNum   命令输出个数
pioOut           命令的输出指针
返回，如果不符合要求，返回FALSE，否则返回TRUE*/
BOOL RE_Ck_OuterOrderTuyuanOutIOSignalType(uint8_t iOrderType,
        uint8_t iOutputNum, EP_ELEM_IO  * pioOut);

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

EP_STATUS   RE_OuterOrderTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
        TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
        NODE ** pRtElemInitNodePointer)
{

    NODE  *pNode;
    OuterOrder_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    unsigned  long  ulStrLen;	/* DY 11/18/2006 */
    unsigned  char   ucOutputCount;
    int  i;
    BOOL  bOtherInitSuccess;
    BOOL  bCkSignalTypeSuccess;

    EP_ELEM_IO  *  pCurElemOutput;

    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoceID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoxinID)[MAX_LOGID_STR_LEN+1];
    char (* pstrCurAOID)[MAX_LOGID_STR_LEN+1];		/* AO logic symbol. */

    unsigned  char      ucCurLuboSetFlag;
    unsigned   char     ucCurFlagSetFlag;
    unsigned   char     ucCurYaoceSetFlag;
    unsigned   char     ucCurYaoxinSetFlag;
    unsigned char       ucCurMeasureSetFlag;
    unsigned char ucCurAOChSetFlag;		/* AO channel flag. */

    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *   pbCurYaoceSetFlag;
    BOOL  *    pbCurYaoxinSetFlag;
    BOOL *pbCurAOChSetFlag;	/* AO channel setting flag. */

    char   (*  pstrCurMeasureID)[MAX_LOGID_STR_LEN+1];	/* 定义临时变量 */
    BOOL  *   pbCurMeasureSetFlag;



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
        LOG_Dbg_Msg("OuterOrder  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    pInitNode=(OuterOrder_Init_Node_Type    *)malloc(sizeof(OuterOrder_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("OuterOrder Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_OUTERORDER;
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
    /*赋值输入个数给节点  */
    pInitNode->PublicElemData.elem.unInNum=0;
    /* 读取信号来源类型 */

    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&(pInitNode->ucSignalSourceType));
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    /* 获得命令逻辑标识 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 (fp,pInitNode->strInputSourceID,&ulStrLen);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    /* 读取22保留字节  */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,22);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }


    /* 读取图元输出个数 */
    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&ucOutputCount);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    /* 根据信号来源类型、命令逻辑标识初始化一些信息 */
    switch(pInitNode->ucSignalSourceType)
    {
        case  MEA_DO:  /*  遥控命令*/

            bReadSuccess=VI_Get_Mea_Do_Num(pInitNode->strInputSourceID,&(pInitNode->uiNodeNum),&(pInitNode->ucParaCount));
            if(!bReadSuccess)
            {
                LOG_Dbg_Msg("Get order of OuterOrder  Tuyuan  \'%s\'   in swcfg   Error!\n"
                            ,(int)(pInitNode->strInputSourceID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得外部命令图元\'%s\'在配置中的序号错误\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:get External order  element id \'%s\' num in configure error\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }
                return   EP_SYS_ERR;
            }
            if(ucOutputCount>MAX_OUTERORDER_OUTPUT_COUNT || ucOutputCount<3 )
            {

                LOG_Dbg_Msg("Error,Meado OuterOrder  Tuyuan Elem  Name\'%s\'  Output  Count  is  out of  range!\n"
                            ,(int)(pInitNode->strInputSourceID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图初始化错误:遥控类型外部命令图元\'%s\'的输出个数错误\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "Logic graph initialization error: control type element id %s output number error\n"
                               ,(int)(pInitNode->strInputSourceID),0);

                }
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            break;
        case  PLUS_ADJUST:/* 增益校准命令*/
        case  OFF_ADJUST:
            if(ucOutputCount!=3)
            {

                LOG_Dbg_Msg("Error,Adjust OuterOrder  Tuyuan Elem  Name\'%s\'  Output  Count  is  out of  range!\n"
                            ,(int)(pInitNode->strInputSourceID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图初始化错误:校准类型外部命令图元\'%s\'的输出个数应为3\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "Logic graph initialization error: adjust type element %s output number should be 3\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            break;
        case PULSE_OUTPUT_CLEAR:
            if(ucOutputCount!=2)
            {

                LOG_Dbg_Msg("Error,PoClear OuterOrder  Tuyuan Elem  Name\'%s\'  Output  Count  is  out of  range!\n"
                            ,(int)(pInitNode->strInputSourceID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图初始化错误:脉冲电度清零类型外部命令图元\'%s\'的输出个数应为3\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "Logic graph initialization error: pulse/kwh reset type element %s output number should be 3\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            break;

        case ZHUBIAN_CHG:
            if (ucOutputCount != 1)
            {
                LOG_Dbg_Msg("Error, TdZhuBian OuterOrder Tuyuan Elem Name \'%s\' Output Count is out of range.\n",
                            (int)(pInitNode->strInputSourceID), 0, 0, 0, 0, 0);

                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT | ER_ALARM | ER_LOCK,
                               "逻辑图初始化错误: 切换主变类型外部命令图元 \'%s\' 的输出个数应为1.\n",
                               (int)(pInitNode->strInputSourceID), 0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT | ER_ALARM | ER_LOCK,
                               "Logic graph initialization error: ZhuBianSwitch type element \'%s\' output number should be 1.\n",
                               (int)(pInitNode->strInputSourceID), 0);
                }
                assert(FALSE);

                return EP_SYS_ERR;

            }
            break;

        case JINXIAN_CHG:
            if (ucOutputCount != 1)
            {
                LOG_Dbg_Msg("Error, TdJinXian OuterOrder Tuyuan Elem Name \'%s\' Output Count is out of range.\n",
                            (int)(pInitNode->strInputSourceID), 0, 0, 0, 0, 0);

                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT | ER_ALARM | ER_LOCK,
                               "逻辑图初始化错误: 切换进线类型外部命令图元 \'%s\' 的输出个数应为1.\n",
                               (int)(pInitNode->strInputSourceID), 0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT | ER_ALARM | ER_LOCK,
                               "Logic graph initialization error: JinXianSwitch type element \'%s\' output number should be 1.\n",
                               (int)(pInitNode->strInputSourceID), 0);
                }
                assert(FALSE);

                return EP_SYS_ERR;

            }
            break;

        case FAR_STS_CHG:		/* 建立初始化接点 */
            if(ucOutputCount!=2)
            {

                LOG_Dbg_Msg("Error,FarStsChg OuterOrder  Tuyuan Elem  Name\'%s\'  Output  Count  is  out of  range!\n"
                            ,(int)(pInitNode->strInputSourceID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图初始化错误:远方就地切换类型外部命令图元\'%s\'的输出个数应为3!\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "Logic graph initialization error: FarStsChg type element %s output number should be 3.\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            break;

        case YXJX_CHG:
            if(ucOutputCount!=2)
            {

                LOG_Dbg_Msg("Error,YXJXChg OuterOrder  Tuyuan Elem  Name\'%s\'  Output  Count  is  out of  range!\n"
                            ,(int)(pInitNode->strInputSourceID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图初始化错误:运行检修切换类型外部命令图元\'%s\'的输出个数应为3!\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "Logic graph initialization error: YXJXChg type element %s output number should be 3.\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            break;

        case JGS_CHG:
            if(ucOutputCount!=2)
            {

                LOG_Dbg_Msg("Error,JGSChg OuterOrder  Tuyuan Elem  Name\'%s\'  Output  Count  is  out of  range!\n"
                            ,(int)(pInitNode->strInputSourceID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图初始化错误:解挂锁切换类型外部命令图元\'%s\'的输出个数应为3!\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "Logic graph initialization error: YXJXChg type element %s output number should be 3.\n"
                               ,(int)(pInitNode->strInputSourceID),0);
                }
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            break;


        default:
            LOG_Dbg_Msg("OuterOrder Tuyuan Input Source  Type  isn't  Expected!\n",0,0,0,0,0,0);
            return  EP_SYS_ERR;
            break;

    }


    /*赋值给节点 ， */

    pInitNode->PublicElemData.elem.ucOutNum=ucOutputCount;
    pInitNode->ucParaCount=ucOutputCount;
    /*循环读取所有输出信息   */
    pCurElemOutput=pInitNode->PublicElemData.elem.aioOut;

    pbCurLuboSetFlag=pInitNode->PublicElemData.abLuboFlagArr;
    pbCurFlagSetFlag=pInitNode->PublicElemData.abFlagSetFlagArr;
    pbCurYaoceSetFlag=pInitNode->PublicElemData.abYaoceFlagArr;
    pbCurYaoxinSetFlag=pInitNode->PublicElemData.abYaoxinFlagArr;
    pbCurAOChSetFlag=pInitNode->PublicElemData.abAOFlagArr;  /* If setting. */

    /* 定义用指向数组的指针，这里不能用双重指针，因为2维字符数组还未初始化 */

    pstrCurLuboID=pInitNode->aStrLuboIDArr;
    pstrCurFlagID=pInitNode->aStrFlagIDArr;
    pstrCurYaoceID=pInitNode->aStrYaoceIDArr;
    pstrCurYaoxinID=pInitNode->aStrYaoxinIDArr;
    pstrCurAOID=pInitNode->aStrAOChIDArr;  /* ID */


    pbCurMeasureSetFlag=pInitNode->PublicElemData.abMeasureFlagArr;
    pstrCurMeasureID=pInitNode->aStrMeasureIDArr;


    for(i=0; i<ucOutputCount; i++)
    {

        /*读取信号类型  */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&(pCurElemOutput->ucAttrib));
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        RE_InitElemIONowValue
        (pCurElemOutput);/*初始化该输出当前值，如果是遥控命令，下面还要修改遥控点号即第二个输出*/
        pCurElemOutput->pvCh=NULL;/*通道句柄为空*/
        pCurElemOutput->ucType=0xFF;/*中间结果  */

        /* 读取7个保留字节  */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,7);
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
                         (fp, (*pstrCurAOID), &ulStrLen);

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
            LOG_Dbg_Msg("Error, Read OuterOrder Tuyuan AO Flag Error!\n",
                        0, 0, 0, 0, 0, 0);

            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误: 读取外部命令图元AO设置错误!\n",
                           0, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err: read OuterOrder Tuyuan AO set err!\n",
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
                         (fp,(*pstrCurLuboID),&ulStrLen);
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

            LOG_Dbg_Msg("Error, Read  OuterOrder  Tuyuan  Elem  Name \'%s\'    Lubo  Flag  Error!\n"
                        ,(int)(pInitNode->strInputSourceID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取外部命令图元\'%s\'的录波设置错误\n"
                           ,(int)(pInitNode->strInputSourceID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err: get External order element \'%s\' Recorded wave setting error\n "
                           ,(int)(pInitNode->strInputSourceID),0);
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
                         (fp,(*pstrCurFlagID),&ulStrLen);
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
            LOG_Dbg_Msg("Error, Read  OuterOrder  Tuyuan  Elem  Name \'%s\'    FlagSet  Flag  Error!\n"
                        ,(int)(pInitNode->strInputSourceID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取外部命令图元\'%s\'的标志设置错误\n"
                           ,(int)(pInitNode->strInputSourceID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err: get External order element \'%s\' flag setting error\n"
                           ,(int)(pInitNode->strInputSourceID),0);
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
                         (fp,(*pstrCurYaoceID),&ulStrLen);
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

            LOG_Dbg_Msg("Error, Read  OuterOrder  Tuyuan  Elem  Name \'%s\'    Yaoce  Flag  Error!\n"
                        ,(int)(pInitNode->strInputSourceID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取外部命令图元\'%s\'的遥测设置错误\n"
                           ,(int)(pInitNode->strInputSourceID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err: get External order element \'%s\' telemetry setting error\n"
                           ,(int)(pInitNode->strInputSourceID),0);
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
                         (fp,(*pstrCurYaoxinID),&ulStrLen);
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
            LOG_Dbg_Msg("error,Read  OuterOrder  Tuyuan Yaoxin  Flag   from  Logrp  SeqFile  error!\n",0,0,0,0,0,0);
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
                         (fp,(*pstrCurMeasureID),&ulStrLen);
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
            LOG_Dbg_Msg("Error, Read  OuterOrder  Tuyuan  Elem  Name \'%s\'    MeasureValue  Flag  Error!\n"
                        ,(int)(pInitNode->strInputSourceID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取外部命令图元\'%s\'的测量设置错误\n"
                           ,(int)(pInitNode->strInputSourceID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err: get External order element \'%s\' measurement setting error\n"
                           ,(int)(pInitNode->strInputSourceID),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */

            return  EP_SYS_ERR;
        }
        pbCurMeasureSetFlag++;
        pstrCurMeasureID++;


        /* 读取10个保留字节  */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,10);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        /* 为了提高效率,不用数组 */
        pCurElemOutput++;

        pbCurLuboSetFlag++;
        pbCurFlagSetFlag++;
        pbCurYaoceSetFlag++;
        pbCurYaoxinSetFlag++;
        pbCurAOChSetFlag++;	/* AO symbol. */

        pstrCurLuboID++;
        pstrCurFlagID++;
        pstrCurYaoceID++;
        pstrCurYaoxinID++;
        pstrCurAOID++;       /* ID of AO. */

    }/*所有输出循环处理结束   */

    /*判断外部命令的各个输入信号类型是否符合要求 ghx2006-12-31添加*/
    bCkSignalTypeSuccess=RE_Ck_OuterOrderTuyuanOutIOSignalType(
                             pInitNode->ucSignalSourceType,pInitNode->ucParaCount,pInitNode->PublicElemData.elem.aioOut);
    if(!bCkSignalTypeSuccess)
    {
        LOG_Dbg_Msg("Error, Read  OuterOrder  Tuyuan  Elem  Name \'%s\'    Output SignalType  Error!\n"
                    ,(int)(pInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:读取外部命令图元\'%s\'的输出信号类型错误\n"
                       ,(int)(pInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err: get External order element \'%s\' output type error\n"
                       ,(int)(pInitNode->strInputSourceID),0);
        }
        return  EP_SYS_ERR;
    }
    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_OuterOrderTuyuanReadFileOtherInit
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

EP_STATUS   RE_OuterOrderTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                        LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    OuterOrder_Init_Node_Type    *pInitNode;

    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *   pbCurYaoceSetFlag;
    BOOL  *    pbCurYaoxinSetFlag;
    BOOL *pbCurAOSetFlag;		/* AO symbol. */


    EP_STATUS  SetCurOutAttribResult;

    EP_ELEM_IO  *  pCurElemOutput;


    /* 定义用指向数组的指针，这里未用双重指针, */
    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoceID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoxinID)[MAX_LOGID_STR_LEN+1];
    char (* pstrCurAOID)[MAX_LOGID_STR_LEN+1];
    BOOL  *   pbCurMeasureSetFlag;		/* 定义临时变量 */
    char   (*  pstrCurMeasureID)[MAX_LOGID_STR_LEN+1];

    int  i;

    pInitNode=(OuterOrder_Init_Node_Type    *)pElemInitNode->pTuyuan;

    pbCurLuboSetFlag=pInitNode->PublicElemData.abLuboFlagArr;
    pbCurFlagSetFlag=pInitNode->PublicElemData.abFlagSetFlagArr;
    pbCurYaoceSetFlag=pInitNode->PublicElemData.abYaoceFlagArr;
    pbCurYaoxinSetFlag=pInitNode->PublicElemData.abYaoxinFlagArr;
    pbCurAOSetFlag=pInitNode->PublicElemData.abAOFlagArr;


    pstrCurLuboID=pInitNode->aStrLuboIDArr;
    pstrCurFlagID=pInitNode->aStrFlagIDArr;
    pstrCurYaoceID=pInitNode->aStrYaoceIDArr;
    pstrCurYaoxinID=pInitNode->aStrYaoxinIDArr;
    pstrCurAOID=pInitNode->aStrAOChIDArr;

    pbCurMeasureSetFlag=pInitNode->PublicElemData.abMeasureFlagArr;
    pstrCurMeasureID=pInitNode->aStrMeasureIDArr;


    for(i=0; i<pInitNode->ucParaCount ; i++)
    {
        /* 循环设定图元的输出属性*/

        pCurElemOutput=RE_OuterOrderTuyuanGetOutIO
                       (i,pElemScanNode);
        /*   */
        if(pCurElemOutput==NULL)
        {
            LOG_Dbg_Msg("error,Can't  Get  OuterOrder Tuyuan One Output  info!\n",0,0,0,0,0,0);

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
                    LOG_Dbg_Msg(" Error,Set  OuterOrder  Tuyuan  Elem  Name \'%s\'  \n Output  Lubo  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurLuboID),0,0,0,0);

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误::外部命令图元\'%s\'\n的输出的录波标识为\'%s\'的录波设置错误\n"
                                   ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurLuboID));
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp rslv err: get External order element \'%s\' Recorded wave \'%s\' setting error\n"
                                   ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurLuboID));
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
                    LOG_Dbg_Msg(" Error,Set  OuterOrder  Tuyuan  Elem  Name \'%s\'  \n Output  Flag  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurFlagID),0,0,0,0);

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误::外部命令图元\'%s\'的输出的标志标识为\'%s\'的标志设置错误\n"
                                   ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurFlagID));
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp rslv err: get External order element \'%s\' output marking \'%s\' setting error\n"
                                   ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurFlagID));
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
                    LOG_Dbg_Msg(" Error,Set  OuterOrder  Tuyuan  Elem  Name \'%s\'  \n Output  Yaoce  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurYaoceID),0,0,0,0);
                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误::外部命令图元\'%s\'的输出的遥测标识为\'%s\'的遥测设置错误\n"
                                   ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurYaoceID));

                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp rslv err: get External order element \'%s\' output Telemetry \'%s\' setting error\n"
                                   ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurYaoceID));
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
                    LOG_Dbg_Msg(" Error,Set  OuterOrder  Tuyuan  Elem  Name \'%s\'  \n Output  Yaoxin  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurYaoxinID),0,0,0,0);

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误::外部命令图元\'%s\'的输出的遥信标识为\'%s\'的遥信设置错误\n"
                                   ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurYaoxinID));
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp rslv err: get External order element \'%s\' output remote signal \'%s\' setting error\n"
                                   ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurYaoxinID));
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
                    LOG_Dbg_Msg(" Error,Set  OuterOrder  Tuyuan  Elem  Name \'%s\'  \n Output  Measure  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurMeasureID),0,0,0,0);

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:外部命令图元\'%s\'的输出的测量标识为\'%s\'的测量设置错误\n"
                                   ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurMeasureID));

                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp rslv err: get External order element \'%s\' output measurement \'%s\' setting error\n"
                                   ,(int)(pInitNode->strInputSourceID),(int)(*pstrCurMeasureID));
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
        pbCurYaoxinSetFlag++;
        pbCurAOSetFlag++;


        pstrCurLuboID++;
        pstrCurFlagID++;
        pstrCurYaoceID++;
        pstrCurYaoxinID++;
        pstrCurAOID++;

        pbCurMeasureSetFlag++;
        pstrCurMeasureID++;

    }

    return  EP_SUCCESS;

}



/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_OuterOrderTuyuanReadFileOtherInit
(OuterOrder_Init_Node_Type * pElemInitNodePointer)		/* DY 11/18/2006 */
{
    pElemInitNodePointer->PublicElemData.elem.ucType=RE_OUTERORDER;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_OuterOrderTuyuanScanInit;

    /* 设置获得IO输出的函数 */
    pElemInitNodePointer->PublicElemData.pfGetScanNodeOutIOFunc
        =RE_OuterOrderTuyuanGetOutIO;

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

EP_STATUS   RE_OuterOrderCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{
    NODE  *pNode;
    int i;	/* DY 11/18/2006 */
    MeaDOOuterOrder_Scan_Node_Type    *pScanNode;
    OuterOrder_Init_Node_Type    *pInitNode;
    pInitNode=(OuterOrder_Init_Node_Type    *)pElemInitNode->pTuyuan;

    /*  设置图元类型*/
    switch(pInitNode->ucSignalSourceType)
    {
        case  MEA_DO:


            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("Mea DO OuterOrder  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(MeaDOOuterOrder_Scan_Node_Type    *)
                      malloc
                      (sizeof(MeaDOOuterOrder_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("Mea DO OuterOrder  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_MEADO_OUTERORDER_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            for(i=0; i<pInitNode->ucParaCount; i++)
                pScanNode->ioOutArr[i]=pInitNode->
                                       PublicElemData.elem.aioOut[i];
            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                              PublicElemData.pfGetScanNodeOutIOFunc;
            pScanNode->uiNodeNum=pInitNode->
                                 uiNodeNum;
            pScanNode->ucParaCount=pInitNode->ucParaCount;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;

            break;
        case  PLUS_ADJUST:
        {
            PlusAdtOuterOrder_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("Plus Adjust OuterOrder   Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(PlusAdtOuterOrder_Scan_Node_Type    *)
                      malloc
                      (sizeof(PlusAdtOuterOrder_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("Plus Adjust OuterOrder  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_PLUSADJUST_OUTERORDER_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            for(i=0; i<pInitNode->ucParaCount; i++)
                pScanNode->ioOutArr[i]=pInitNode->
                                       PublicElemData.elem.aioOut[i];
            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                              PublicElemData.pfGetScanNodeOutIOFunc;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  OFF_ADJUST:
        {
            OffAdtOuterOrder_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("Off Adjust OuterOrder   Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(OffAdtOuterOrder_Scan_Node_Type    *)
                      malloc
                      (sizeof(OffAdtOuterOrder_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("Off Adjust OuterOrder  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_OFFADJUST_OUTERORDER_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            for(i=0; i<pInitNode->ucParaCount; i++)
                pScanNode->ioOutArr[i]=pInitNode->
                                       PublicElemData.elem.aioOut[i];
            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                              PublicElemData.pfGetScanNodeOutIOFunc;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  PULSE_OUTPUT_CLEAR:
        {
            PulseClearOuterOrder_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("PoClear OuterOrder   Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(PulseClearOuterOrder_Scan_Node_Type    *)
                      malloc
                      (sizeof(PulseClearOuterOrder_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("PoClear OuterOrder  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_POCLEAR_OUTERORDER_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            for(i=0; i<pInitNode->ucParaCount; i++)
                pScanNode->ioOutArr[i]=pInitNode->
                                       PublicElemData.elem.aioOut[i];
            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                              PublicElemData.pfGetScanNodeOutIOFunc;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;

        case ZHUBIAN_CHG:
        {
            SwitchZhuBianOuterOrder_Scan_Node_Type *pScanNode;
            pNode=(NODE *)malloc(sizeof(NODE));

            if (pNode == NULL)
            {
                LOG_Dbg_Msg("TdZhuBianSwitch OuterOrder Tuyuan malloc failiure.\n", 0, 0, 0, 0, 0, 0);
                return EP_BUF_ERR;
            }

            pScanNode=(SwitchZhuBianOuterOrder_Scan_Node_Type *)
                      malloc(sizeof(SwitchZhuBianOuterOrder_Scan_Node_Type));

            if (pScanNode == NULL)
            {
                LOG_Dbg_Msg("TdZhuBianSwitch OuterOrder Tuyuan malloc failiure.\n", 0, 0, 0, 0, 0, 0);
                return EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_ZHUBIAN_OUTERORDER_SCAN;
            pNode->pTuyuan=(void *)pScanNode;

            for (i=0; i<pInitNode->ucParaCount; i++)
                pScanNode->ioOutArr[i]=pInitNode->PublicElemData.elem.aioOut[i];

            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->PublicElemData.pfGetScanNodeOutIOFunc;

            /* 设定返回的节点地址 */
            *pReturnElemScanNodePointer=(NODE  *)pNode;

            return EP_SUCCESS;
        }
        break;

        case JINXIAN_CHG:
        {
            SwitchJinXianOuterOrder_Scan_Node_Type *pScanNode;
            pNode=(NODE *)malloc(sizeof(NODE));

            if (pNode == NULL)
            {
                LOG_Dbg_Msg("TdJinXianSwitch OuterOrder Tuyuan malloc failiure.\n", 0, 0, 0, 0, 0, 0);
                return EP_BUF_ERR;
            }

            pScanNode=(SwitchJinXianOuterOrder_Scan_Node_Type *)
                      malloc(sizeof(SwitchJinXianOuterOrder_Scan_Node_Type));

            if (pScanNode == NULL)
            {
                LOG_Dbg_Msg("TdJinXianSwitch OuterOrder Tuyuan malloc failiure.\n", 0, 0, 0, 0, 0, 0);
                return EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_JINXIAN_OUTERORDER_SCAN;
            pNode->pTuyuan=(void *)pScanNode;

            for (i=0; i<pInitNode->ucParaCount; i++)
                pScanNode->ioOutArr[i]=pInitNode->PublicElemData.elem.aioOut[i];

            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->PublicElemData.pfGetScanNodeOutIOFunc;

            /* 设定返回的节点地址 */
            *pReturnElemScanNodePointer=(NODE  *)pNode;

            return EP_SUCCESS;
        }
        break;

        case  FAR_STS_CHG:		/* 建立扫描节点 */
        {
            FarChgOuterOrder_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("FarStsChg OuterOrder   Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(FarChgOuterOrder_Scan_Node_Type    *)
                      malloc
                      (sizeof(FarChgOuterOrder_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("FarStsChg OuterOrder  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_FARSTSCHG_OUTERORDER_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            for(i=0; i<pInitNode->ucParaCount; i++)
                pScanNode->ioOutArr[i]=pInitNode->
                                       PublicElemData.elem.aioOut[i];
            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                              PublicElemData.pfGetScanNodeOutIOFunc;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;

        case  YXJX_CHG:		/* 建立扫描节点 */
        {
            YXJXChgOuterOrder_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("YXJXChg OuterOrder   Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(YXJXChgOuterOrder_Scan_Node_Type    *)
                      malloc
                      (sizeof(YXJXChgOuterOrder_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("YXJXChg OuterOrder  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_YXJXCHG_OUTERORDER_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            for(i=0; i<pInitNode->ucParaCount; i++)
                pScanNode->ioOutArr[i]=pInitNode->
                                       PublicElemData.elem.aioOut[i];
            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                              PublicElemData.pfGetScanNodeOutIOFunc;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;

        case  JGS_CHG:		/* 建立扫描节点 */
        {
            JGSChgOuterOrder_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("JGSChg OuterOrder   Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(JGSChgOuterOrder_Scan_Node_Type    *)
                      malloc
                      (sizeof(JGSChgOuterOrder_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("JGSChg OuterOrder  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_JGSCHG_OUTERORDER_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            for(i=0; i<pInitNode->ucParaCount; i++)
                pScanNode->ioOutArr[i]=pInitNode->
                                       PublicElemData.elem.aioOut[i];
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

EP_ELEM_IO *  RE_OuterOrderTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode)
{

    switch(pScanNode->ulTuyuanType)
    {
        case   RE_MEADO_OUTERORDER_SCAN:
        {

            MeaDOOuterOrder_Scan_Node_Type    *pTuyuanScanNode;

            pTuyuanScanNode=(MeaDOOuterOrder_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(unOutNum>=MAX_OUTERORDER_OUTPUT_COUNT)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get One OuterOrder  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanScanNode->ioOutArr[unOutNum]);
        }
        break;
        case   RE_PLUSADJUST_OUTERORDER_SCAN:
        {

            PlusAdtOuterOrder_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(PlusAdtOuterOrder_Scan_Node_Type    *)pScanNode->pTuyuan;
            if(unOutNum>=3)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get One OuterOrder  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOutArr[unOutNum]);

        }
        break;
        case   RE_OFFADJUST_OUTERORDER_SCAN:
        {

            OffAdtOuterOrder_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(OffAdtOuterOrder_Scan_Node_Type    *)pScanNode->pTuyuan;
            if(unOutNum>=3)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get One OuterOrder  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOutArr[unOutNum]);

        }
        break;
        case   RE_POCLEAR_OUTERORDER_SCAN:
        {

            PulseClearOuterOrder_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(PulseClearOuterOrder_Scan_Node_Type    *)pScanNode->pTuyuan;
            if(unOutNum>=2)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get One OuterOrder  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOutArr[unOutNum]);

        }
        break;

        case RE_ZHUBIAN_OUTERORDER_SCAN:
        {
            SwitchZhuBianOuterOrder_Scan_Node_Type *pTuyuanNode;
            pTuyuanNode=(SwitchZhuBianOuterOrder_Scan_Node_Type *)pScanNode->pTuyuan;

            if (unOutNum >= 1)
            {
                /* 若越界，则出错 */
                LOG_Dbg_Msg("Get One OuterOrder Tuyuan OutputIO Beyond Bound.\n", 0, 0, 0, 0, 0, 0);

                assert (FALSE);

                return NULL;
            }

            return &(pTuyuanNode->ioOutArr[unOutNum]);
        }
        break;

        case RE_JINXIAN_OUTERORDER_SCAN:
        {
            SwitchJinXianOuterOrder_Scan_Node_Type *pTuyuanNode;

            pTuyuanNode=(SwitchJinXianOuterOrder_Scan_Node_Type *)pScanNode->pTuyuan;

            if (unOutNum >= 1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get One OuterOrder Tuyuan OutputIO Beyond Bound.\n", 0, 0, 0, 0, 0, 0);

                assert (FALSE);

                return NULL;

            }

            return &(pTuyuanNode->ioOutArr[unOutNum]);
        }
        break;

        case   RE_FARSTSCHG_OUTERORDER_SCAN:		/* 返回源地址 */
        {

            FarChgOuterOrder_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(FarChgOuterOrder_Scan_Node_Type    *)pScanNode->pTuyuan;
            if(unOutNum>=2)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get One OuterOrder  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOutArr[unOutNum]);

        }
        break;

        case   RE_YXJXCHG_OUTERORDER_SCAN:
        {

            YXJXChgOuterOrder_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(YXJXChgOuterOrder_Scan_Node_Type    *)pScanNode->pTuyuan;
            if(unOutNum>=2)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get One OuterOrder  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOutArr[unOutNum]);

        }
        break;

        case   RE_JGSCHG_OUTERORDER_SCAN:
        {

            JGSChgOuterOrder_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(JGSChgOuterOrder_Scan_Node_Type    *)pScanNode->pTuyuan;
            if(unOutNum>=2)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get One OuterOrder  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOutArr[unOutNum]);

        }
        break;

        default  :

            LOG_Dbg_Msg("Get One OuterOrder  Tuyuan  OutputIO  Error!\n",0,0,0,0,0,0);
            assert(FALSE);
            return  NULL;
            break;
    }


}


EP_STATUS RE_Ck_OuterOrderTuyuanOutIOSignalType(uint8_t iOrderType,
        uint8_t iOutputNum, EP_ELEM_IO  * pioOut)
{
    switch(iOrderType)
    {
        case  MEA_DO:  /*  遥控命令*/
        {
            assert(iOutputNum>=3);
            if(pioOut[0].ucAttrib!=0x04 || pioOut[1].ucAttrib!=0x61  || pioOut[2].ucAttrib!=0x04 )
                return FALSE;
        }
        break;
        case  PLUS_ADJUST:/* 增益校准命令*/
        case  OFF_ADJUST:
        {
            assert(iOutputNum==3);
            if(pioOut[0].ucAttrib!=0x04 || pioOut[1].ucAttrib!=0x61  || pioOut[2].ucAttrib!=0x61 )
                return FALSE;
        }
        break;
        case PULSE_OUTPUT_CLEAR:
        {
            assert(iOutputNum==2);
            if(pioOut[0].ucAttrib!=0x04 || pioOut[1].ucAttrib!=0x61  )
                return FALSE;
        }
        break;

        case ZHUBIAN_CHG:
        {
            assert (iOutputNum == 1);

            if (pioOut[0].ucAttrib != 0x04)
                return FALSE;
        }
        break;

        case JINXIAN_CHG:
        {
            assert (iOutputNum == 1);

            if (pioOut[0].ucAttrib != 0x04)
                return FALSE;
        }
        break;

        case FAR_STS_CHG:		/* 输出信号类型 */
        {
            assert(iOutputNum==2);
            if(pioOut[0].ucAttrib!=0x04 || pioOut[1].ucAttrib!=0x61  )
                return FALSE;
        }
        break;

        case YXJX_CHG:		/* 输出信号类型 */
        {
            assert(iOutputNum==2);
            if(pioOut[0].ucAttrib!=0x04 || pioOut[1].ucAttrib!=0x61  )
                return FALSE;
        }
        break;

        case JGS_CHG:		/* 输出信号类型 */
        {
            assert(iOutputNum==2);
            if(pioOut[0].ucAttrib!=0x04 || pioOut[1].ucAttrib!=0x61  )
                return FALSE;
        }
        break;

        default:
            assert(FALSE);
            break;
    }
    return TRUE;
}

/*    外部命令图元遥控命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_MeaDoOuterOrderTuyuanScan(NODE *pElemScanNode)
{
    int iPtNum;
    BOOL bAct;
    uint32_t ulPulseTm;
    uint32_t ulTqPara;

    /* 访问当前采样节拍的物理AI通道数据指针 */
    MeaDOOuterOrder_Scan_Node_Type    * pTuyuanNode;
    /*uint32_t   *pul;*/
    pTuyuanNode=(MeaDOOuterOrder_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    if(!VI_Come_New_MeaDo(pTuyuanNode->uiNodeNum,&iPtNum,&bAct, &ulPulseTm, &ulTqPara))
    {
        pTuyuanNode->ioOutArr[0].now.bVal=FALSE;
    }
    else
    {
        pTuyuanNode->ioOutArr[0].now.bVal=TRUE;
        pTuyuanNode->ioOutArr[1].now.lVal=iPtNum;
        pTuyuanNode->ioOutArr[2].now.bVal=bAct;
        if(pTuyuanNode->ucParaCount == 4)
        {
            /* 一个扩展参数 */
            pTuyuanNode->ioOutArr[3].now.ulVal=ulPulseTm;
        }
        else if(pTuyuanNode->ucParaCount >= 5)
        {
            /* 两个扩展参数 */
            pTuyuanNode->ioOutArr[3].now.ulVal=ulPulseTm;
            pTuyuanNode->ioOutArr[4].now.ulVal=ulTqPara;
        }
    }
}

/*    外部命令图元偏置校准命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_OffAdtOuterOrderTuyuanScan(
    int nScanTaskNo,
    NODE *pElemScanNode
)
{
    int32_t iObjType;
    int32_t  iNum;
    /* 访问当前采样节拍的物理AI通道数据指针 */
    OffAdtOuterOrder_Scan_Node_Type    * pTuyuanNode;
    pTuyuanNode=(OffAdtOuterOrder_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    if(!VI_Come_New_Off_Adjust(nScanTaskNo, &iObjType, &iNum))
    {
        pTuyuanNode->ioOutArr[0].now.bVal=FALSE;
    }
    else
    {
        pTuyuanNode->ioOutArr[0].now.bVal=TRUE;
        pTuyuanNode->ioOutArr[1].now.lVal=iObjType;
        pTuyuanNode->ioOutArr[2].now.lVal=iNum;
    }
}

/*    外部命令图元增益校准命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_PlusAdtOuterOrderTuyuanScan(
    int  nScanTaskNo,		/* 访问逻辑图任务号 */
    NODE *pElemScanNode
)
{
    int32_t  iObjType;
    int32_t  iNum;
    /* 访问当前采样节拍的物理AI通道数据指针 */
    PlusAdtOuterOrder_Scan_Node_Type    * pTuyuanNode;
    /*uint32_t   *pul;*/
    pTuyuanNode=(PlusAdtOuterOrder_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    if(!VI_Come_New_Plus_Adjust(nScanTaskNo, &iObjType, &iNum))
    {
        pTuyuanNode->ioOutArr[0].now.bVal=FALSE;
    }
    else
    {
        pTuyuanNode->ioOutArr[0].now.bVal=TRUE;
        pTuyuanNode->ioOutArr[1].now.lVal=iObjType;
        pTuyuanNode->ioOutArr[2].now.lVal=iNum;
    }

}

/* 增益校准命令
 * Para:
 *     pElemScanNode, 扫描节点.
 * Return:
 *     NONE.
 */
void RE_PlusAdtOuterOrderTuyuanScan2(NODE *pElemScanNode)
{
    int32_t iObjType;
    int32_t iNum;

    /* 访问当前采样节拍的物理AI通道数据指针 */
    PlusAdtOuterOrder_Scan_Node_Type *pTuyuanNode;
    int nScanTaskNo;

    nScanTaskNo = pElemScanNode->nScanTaskNo;

    pTuyuanNode = (PlusAdtOuterOrder_Scan_Node_Type *)pElemScanNode->pTuyuan;

    if (!VI_Come_New_Plus_Adjust(0, &iObjType, &iNum))
    {
        pTuyuanNode->ioOutArr[0].now.bVal = FALSE;
    }
    else
    {
        pTuyuanNode->ioOutArr[0].now.bVal = TRUE;
        pTuyuanNode->ioOutArr[1].now.lVal = iObjType;
        pTuyuanNode->ioOutArr[2].now.lVal = iNum;
    }
}

/*    外部命令图元脉冲电度清零命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_PulseClearOuterOrderTuyuanScan(NODE *pElemScanNode)
{
    int32_t  iNum;
    /* 访问当前采样节拍的物理AI通道数据指针 */
    PulseClearOuterOrder_Scan_Node_Type    * pTuyuanNode;
    pTuyuanNode=(PulseClearOuterOrder_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    if(!VI_Come_New_PoClear(&iNum))
    {
        pTuyuanNode->ioOutArr[0].now.bVal=FALSE;
    }
    else
    {
        pTuyuanNode->ioOutArr[0].now.bVal=TRUE;
        pTuyuanNode->ioOutArr[1].now.lVal=iNum;
    }
}

/* scanning function for ZhuBian switching.
 * Para:
 *     pElemScanNode, scanning pointer.
 * Return:
 *     NONE.
 */
void RE_TdZhuBianSwitchOuterOrderTuyuanScan(NODE *pElemScanNode)
{
    SwitchZhuBianOuterOrder_Scan_Node_Type *pTuyuanNode;

    pTuyuanNode=(SwitchZhuBianOuterOrder_Scan_Node_Type *)pElemScanNode->pTuyuan;

    if (VI_Come_New_TdZhuBianSwitch())
    {
        pTuyuanNode->ioOutArr[0].now.bVal=TRUE;
    }
    else
    {
        pTuyuanNode->ioOutArr[0].now.bVal=FALSE;
    }
}

/* scanning function for JinXian switching.
 * Para:
 *     pElemScanNode, scanning pointer.
 * Return:
 *     NONE.
 */
void RE_TdJinXianSwitchOuterOrderTuyuanScan(NODE *pElemScanNode)
{
    SwitchJinXianOuterOrder_Scan_Node_Type *pTuyuanNode;

    pTuyuanNode=(SwitchJinXianOuterOrder_Scan_Node_Type *)pElemScanNode->pTuyuan;

    if (VI_Come_New_TdJinXianSwitch())
    {
        pTuyuanNode->ioOutArr[0].now.bVal=TRUE;
    }
    else
    {
        pTuyuanNode->ioOutArr[0].now.bVal=FALSE;
    }
}

/*    远方就地切换命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_FarStsChgOuterOrderTuyuanScan(NODE *pElemScanNode)
{
    int32_t  iNum;
    /* 访问当前采样节拍的物理AI通道数据指针 */
    FarChgOuterOrder_Scan_Node_Type    * pTuyuanNode;
    pTuyuanNode=(FarChgOuterOrder_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    if(!VI_Come_New_FarSts(&iNum))
    {
        pTuyuanNode->ioOutArr[0].now.bVal=FALSE;
    }
    else
    {
        pTuyuanNode->ioOutArr[0].now.bVal=TRUE;
        pTuyuanNode->ioOutArr[1].now.lVal=iNum;
    }

}

/*    运行检修切换命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_YXJXChgOuterOrderTuyuanScan(NODE *pElemScanNode)
{
    int32_t  iNum;
    /* 访问当前采样节拍的物理AI通道数据指针 */
    YXJXChgOuterOrder_Scan_Node_Type    * pTuyuanNode;
    pTuyuanNode=(YXJXChgOuterOrder_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    if(!VI_Come_New_RepairSts(&iNum))
    {
        pTuyuanNode->ioOutArr[0].now.bVal=FALSE;
    }
    else
    {
        pTuyuanNode->ioOutArr[0].now.bVal=TRUE;
        pTuyuanNode->ioOutArr[1].now.lVal=iNum;
    }

}

/*    解挂锁切换命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_JGSChgOuterOrderTuyuanScan(NODE *pElemScanNode)
{
    int32_t  iNum;
    /* 访问当前采样节拍的物理AI通道数据指针 */
    JGSChgOuterOrder_Scan_Node_Type    * pTuyuanNode;
    pTuyuanNode=(JGSChgOuterOrder_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    if(!VI_Come_New_JgsSts(&iNum))
    {
        pTuyuanNode->ioOutArr[0].now.bVal=FALSE;
    }
    else
    {
        pTuyuanNode->ioOutArr[0].now.bVal=TRUE;
        pTuyuanNode->ioOutArr[1].now.lVal=iNum;
    }

}

/* 偏置校准命令
 * Para:
 *     pElemScanNode, 扫描节点.
 * Return:
 *     NONE.
 */
void RE_OffAdtOuterOrderTuyuanScan2(NODE *pElemScanNode)
{
    int32_t iObjType;
    int32_t iNum;
    int nScanTaskNo;

    /* 访问当前采样节拍的物理AI通道数据指针 */
    OffAdtOuterOrder_Scan_Node_Type *pTuyuanNode;

    nScanTaskNo = pElemScanNode->nScanTaskNo;

    pTuyuanNode = (OffAdtOuterOrder_Scan_Node_Type *)pElemScanNode->pTuyuan;

    if (!VI_Come_New_Off_Adjust(nScanTaskNo, &iObjType, &iNum))
    {
        pTuyuanNode->ioOutArr[0].now.bVal = FALSE;
    }
    else
    {
        pTuyuanNode->ioOutArr[0].now.bVal = TRUE;
        pTuyuanNode->ioOutArr[1].now.lVal = iObjType;
        pTuyuanNode->ioOutArr[2].now.lVal = iNum;
    }
}
