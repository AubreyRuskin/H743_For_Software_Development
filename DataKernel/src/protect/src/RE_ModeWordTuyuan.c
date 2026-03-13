/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_ModeWordTuyuan.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该源代码文件定义了保护功能模块中的方式字元件的实现                       */
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
/*         张云       2005.11.11              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#include   <vxWorks.h>
#include   "RE_ModeWordTuyuan.h"


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

EP_STATUS   RE_ModeWordTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
        TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
        NODE ** pRtElemInitNodePointer)
{

    NODE  *pNode;
    BOOL  bOtherInitSuccess;
    unsigned   long   TempLong;
    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoxinID)[MAX_LOGID_STR_LEN+1];

    unsigned  char      ucCurLuboSetFlag;
    unsigned   char     ucCurFlagSetFlag;
    unsigned   char     ucCurYaoxinSetFlag;

    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *    pbCurYaoxinSetFlag;

    EP_ELEM_IO  *  pCurElemOutput;

    unsigned  short   *  punCurInSourceSeqNo;
    unsigned  char   *   pucInSourceOutputNo;
    ModeWord_Init_Node_Type    *pInitNode;
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
        LOG_Dbg_Msg("ModeWord  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }

    pInitNode=(ModeWord_Init_Node_Type    *)malloc(sizeof(ModeWord_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("ModeWord  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_MODEWORD;
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
        punCurInSourceSeqNo++;
        pucInSourceOutputNo++;
    }/*所有输入循环处理结束  */


    /* 该扫描节点的输出个数有可能还会被用户开发的算法初始化函数覆盖 */
    pInitNode->PublicElemData.elem.ucOutNum=32;
    /* 在初始化节点中保存逻辑图上的输出个数,以便和用户开发的算法
       设定的输出匹配 */
    /*循环读取所有输出信息   */

    pCurElemOutput=pInitNode->PublicElemData.elem.aioOut;
    pbCurLuboSetFlag=pInitNode->PublicElemData.abLuboFlagArr;
    pbCurFlagSetFlag=pInitNode->PublicElemData.abFlagSetFlagArr;
    pbCurYaoxinSetFlag=pInitNode->PublicElemData.abYaoxinFlagArr;
    /* 定义用指向数组的指针，这里不能用双重指针，因为2维字符数组还未初始化 */

    pstrCurLuboID=pInitNode->aStrLuboIDArr;
    pstrCurFlagID=pInitNode->aStrFlagIDArr;
    pstrCurYaoxinID=pInitNode->aStrYaoxinIDArr;
    for(i=0; i<32; i++)
    {
        pCurElemOutput->ucAttrib=LOGIC_SIGNAL;
        pCurElemOutput->now.bVal=FALSE;
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
            LOG_Dbg_Msg("Error, Read  ModeWord  Tuyuan Lubo  Flag  Error!\n"
                        ,0,0,0,0,0,0);
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取方式字图元的录波设置错误\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err: read modeword  element disturbance record err\n"
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
            LOG_Dbg_Msg("Error, Read  ModeWord  Tuyuan FlagSet  Flag  Error!\n"
                        ,0,0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取方式字图元的标志设置错误\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:read modword  element flag err\n"
                           ,0,0);
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
            LOG_Dbg_Msg("Error, Read  ModeWord   element remote signal  Flag  Error!\n"
                        ,0,0,0,0,0,0);
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:读取方式字图元的遥信设置错误\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:read modword  element remote signal err\n"
                           ,0,0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        /* 读取4个保留字节  */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }

        /* 为了提高效率,不用数组 */
        pCurElemOutput++;

        pbCurLuboSetFlag++;
        pbCurFlagSetFlag++;
        pbCurYaoxinSetFlag++;
        pstrCurLuboID++;
        pstrCurFlagID++;
        pstrCurYaoxinID++;
    }/*所有输出循环处理结束   */

    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_ModeWordTuyuanReadFileOtherInit
                      (pInitNode);
    if(!bOtherInitSuccess)
    {
        return  EP_SYS_ERR;
    }

    /* 设定返回的节点地址  */
    *pRtElemInitNodePointer=(NODE  *)pNode;

    return   EP_SUCCESS;
}






/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/

BOOL      RE_ModeWordTuyuanReadFileOtherInit
(ModeWord_Init_Node_Type * pElemInitNodePointer)
{
    pElemInitNodePointer->PublicElemData.elem.ucType=RE_MODEWORD;
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_ModeWordTuyuanScanInit;
    /* 设置获取扫描节点输出的指针的函数 */
    pElemInitNodePointer->PublicElemData.pfGetScanNodeOutIOFunc=
        RE_ModeWordTuyuanGetOutIO;
    return   TRUE;
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

EP_STATUS   RE_ModeWordTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                      LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    ModeWord_Init_Node_Type    *pInitNode;
    ModeWord_Scan_Node_Type    *pScanNode;
    NODE  *  pCurSourceNode;

    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *    pbCurYaoxinSetFlag;

    EP_STATUS  SetCurOutAttribResult;

    EP_ELEM_IO  *  pCurElemOutput;

    uint16_t   * punCurInSignalSourceSeqNo;
    uint8_t   *  pucCurInSignalSourceOutNo;

    /* 定义用指向数组的指针，这里未用双重指针, */
    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoxinID)[MAX_LOGID_STR_LEN+1];

    int  i;
    uint8_t   ucCurInSignalType;

    pInitNode=(ModeWord_Init_Node_Type    *)pElemInitNode->pTuyuan;
    pScanNode=(ModeWord_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    punCurInSignalSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucCurInSignalSourceOutNo=pInitNode->aucInSourceOutputNumArr;

    /* 循环设定图元的输入源指针  */
    for(i=0; i<1; i++)
    {

        pCurSourceNode=RE_LstNth(pGrpScanNodeList,
                                 ((*punCurInSignalSourceSeqNo)+1));/* 注意序号从1开始 */

        if(pCurSourceNode==NULL)
        {
            LOG_Dbg_Msg("Error,Get  ModeWord  Tuyuan  Input Source  Init  failure!\n"
                        ,0,0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得方式字图元输入来源错误\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get modword  element input source err\n"
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
            LOG_Dbg_Msg("Error,Get  ModeWord  Tuyuan  input  info  failure!\n"
                        ,0,0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得方式字图元输入来源失败\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get modword  element input source failure\n"
                           ,0,0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }

        /*比较信号类型是否匹配  */

        ucCurInSignalType=pScanNode->pInArr0->ucAttrib;
        if(ucCurInSignalType!=HEX_MODE_WORD_SIGNAL)
        {
            LOG_Dbg_Msg("Error,ModeWord  Tuyuan  One  Input  Signal Type  can't  Match!\n"
                        ,0,0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:方式字图元的一个输入的信号类型错误\n"
                           ,0,0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:one of modeword  element's  input signal type err\n"
                           ,0,0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }
        punCurInSignalSourceSeqNo++;
        pucCurInSignalSourceOutNo++;
    }
    pCurElemOutput=pScanNode->ioOutArr;
    pbCurLuboSetFlag=pInitNode->PublicElemData.abLuboFlagArr;
    pbCurFlagSetFlag=pInitNode->PublicElemData.abFlagSetFlagArr;
    pbCurYaoxinSetFlag=pInitNode->PublicElemData.abYaoxinFlagArr;

    pstrCurLuboID=pInitNode->aStrLuboIDArr;
    pstrCurFlagID=pInitNode->aStrFlagIDArr;
    pstrCurYaoxinID=pInitNode->aStrYaoxinIDArr;

    for(i=0; i<32; i++)
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
                    LOG_Dbg_Msg(" Error,Set  ModeWord  Tuyuan  Output  Lubo  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(*pstrCurLuboID),0,0,0,0,0);

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:方式字图元的输出的录波标识为\'%s\'的录波设置错误\n"
                                   ,(int)(*pstrCurLuboID),0);
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp rslv err:modeword  element output disturbance record id\'%s\'  set err\n"
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
                    LOG_Dbg_Msg(" Error,Set  WordMode  Tuyuan Output  Flag  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(*pstrCurFlagID),0,0,0,0,0);

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:方式字图元的输出的标志标识为\'%s\'的标志设置错误\n"
                                   ,(int)(*pstrCurFlagID),0);
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp rslv err:modeword  element output flag id \'%s\' set err\n"
                                   ,(int)(*pstrCurFlagID),0);
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
                    LOG_Dbg_Msg(" Error,Set  ModeWord   element  Output  remote signal  ID is  \'%s\'  Value  Error!\n"
                                ,(int)(*pstrCurYaoxinID),0,0,0,0,0);

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:方式字图元的输出的遥信标识为\'%s\'的遥信设置错误!\n"
                                   ,(int)(*pstrCurYaoxinID),0);
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp rslv err:modeword  element output remote signal id \'%s\' set err\n"
                                   ,(int)(*pstrCurYaoxinID),0);
                    }
                    assert(FALSE);  /* 若非以上类型,则告警   */
                    return   SetCurOutAttribResult;
                }
            }

        }
        /* 为了提高效率,不用数组 */
        pCurElemOutput++;

        pbCurLuboSetFlag++;
        pbCurFlagSetFlag++;
        pbCurYaoxinSetFlag++;

        pstrCurLuboID++;
        pstrCurFlagID++;
        pstrCurYaoxinID++;
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

EP_STATUS   RE_ModeWordCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{
    NODE  *pNode;
    int  i;
    ModeWord_Scan_Node_Type    *pScanNode;
    ModeWord_Init_Node_Type    *pInitNode;

    pInitNode=(ModeWord_Init_Node_Type    *)pElemInitNode->pTuyuan;

    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("Suanfa  ModeWord  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pScanNode=(ModeWord_Scan_Node_Type    *)malloc(sizeof(ModeWord_Scan_Node_Type));
    if(pScanNode==NULL)
    {
        LOG_Dbg_Msg("ModeWord  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }

    /* 设置实际扫描图元类型 */
    pNode->ulTuyuanType=RE_MODEWORD_SCAN;
    pNode->pTuyuan=(void  *)pScanNode;
    pScanNode->pchart=pInitNode->PublicElemData.elem.pchart;
    for(i=0; i<32; i++)
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

EP_ELEM_IO *  RE_ModeWordTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode)
{

    ModeWord_Scan_Node_Type    *pTuyuanScanNode;

    pTuyuanScanNode=(ModeWord_Scan_Node_Type    *)pScanNode->pTuyuan;

    if(unOutNum>=32)
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Get One ModeWord  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  NULL;

    }
    return   &(pTuyuanScanNode->ioOutArr[unOutNum]);

}


/*    图元包裹化后的扫描函数**/
/*    功能:首先调用用户开发的算法扫描函数
           然后处理其他工作.
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***********************/
/*    返回值，无   */


void     RE_ModeWordTuyuanScan(NODE  *pElemScanNode)
{
#define CYCLE_NUM 8 /* 循环次数 */
#define PROC_NUM 4 /* 单次处理次数 */

    ModeWord_Scan_Node_Type  *  pTuyuanNode;
    EP_ELEM_IO  * pCurOut;
    uint32_t   ulInputValue;
    int32_t i;

    pTuyuanNode=(ModeWord_Scan_Node_Type   *)pElemScanNode->pTuyuan;
    ulInputValue=pTuyuanNode->pInArr0->now.ulVal;
    pCurOut=pTuyuanNode->ioOutArr;

    /* 逐位处理 */
    for (i = 0; i<CYCLE_NUM; i++)
    {
        (pCurOut++)->now.bVal = (ulInputValue&0x00000001) >> 0;
        (pCurOut++)->now.bVal = (ulInputValue&0x00000002) >> 1;
        (pCurOut++)->now.bVal = (ulInputValue&0x00000004) >> 2;
        (pCurOut++)->now.bVal = (ulInputValue&0x00000008) >> 3;
        ulInputValue >>= PROC_NUM;
    }
}
