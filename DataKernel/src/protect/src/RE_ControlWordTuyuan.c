/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_ControlWordTuyuan.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的控制字元件的实现代码                        */
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

/*                                                                              */
/********************************************************************************/


#include <vxWorks.h>

#include  "RE_ControlWordTuyuan.h"

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

EP_STATUS   RE_ControlWordTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
        TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
        NODE ** pRtElemInitNodePointer)
{
    NODE  *pNode;
    unsigned  char   *   pucCurInSignalType;
    unsigned  short   *  punCurInSourceSeqNo;
    unsigned  char   *   pucInSourceOutputNo;


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

    BOOL  bOtherInitSuccess;

    EP_ELEM_IO  *  pCurElemOutput;


    unsigned  long  ulIDStrLen;

    unsigned  char   ucInputCount;
    ControlWord_Init_Node_Type    *pInitNode;
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
        LOG_Dbg_Msg("ControlWord  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }

    pInitNode=(ControlWord_Init_Node_Type    *)malloc(sizeof(ControlWord_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("ControlWord  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_CONTROLWORD;
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
    /* 读取图元输入个数 */
    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&ucInputCount);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    if((ucInputCount>MAX_CONTROLWORD_INPUT_COUNT)||(ucInputCount<2))
    {
        LOG_Dbg_Msg("error,ControlWord  Tuyuan  Input  Count  is  out of  range!\n",0,0,0,0,0,0);
        assert(FALSE);
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

        /*设置信号类型  */
        *pucCurInSignalType=LOGIC_SIGNAL;

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

    /* 读取控制字的逻辑标识 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 ( fp,pInitNode->strControlWordID,&ulIDStrLen);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    /*赋值输出给节点 ， */

    /* 该扫描节点的输出个数 */
    pInitNode->PublicElemData.elem.ucOutNum=1;

    /*循环读取所有输出信息   */
    pCurElemOutput=pInitNode->PublicElemData.elem.aioOut;

    pbCurLuboSetFlag=pInitNode->PublicElemData.abLuboFlagArr;
    pbCurFlagSetFlag=pInitNode->PublicElemData.abFlagSetFlagArr;
    pbCurYaoxinSetFlag=pInitNode->PublicElemData.abYaoxinFlagArr;


    /* 定义用指向数组的指针，这里不能用双重指针，因为2维字符数组还未初始化 */
    pstrCurLuboID=pInitNode->aStrLuboIDArr;
    pstrCurFlagID=pInitNode->aStrFlagIDArr;
    pstrCurYaoxinID=pInitNode->aStrYaoxinIDArr;

    for(i=0; i<1; i++)
    {

        /* 设置输出信号类型 */
        pCurElemOutput->ucAttrib=LOGIC_SIGNAL;

        RE_InitElemIONowValue
        (pCurElemOutput);/*初始化该输出当前值*/
        pCurElemOutput->pvCh=NULL;/*通道句柄为空*/
        pCurElemOutput->ucType=0xFF;/*中间结果  */


        /* 读取4个保留字节  */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
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

            LOG_Dbg_Msg("Read  ControlWord   Lubo  Flag   from  Logrp  SeqFile  error!\n",0,0,0,0,0,0);
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

            LOG_Dbg_Msg("Read  ControlWord  FlagSet  Flag   from  Logrp  SeqFile  error!\n",0,0,0,0,0,0);
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
            LOG_Dbg_Msg("Read  ControlWord  Yaoxin  Flag   from  Logrp  SeqFile  error!\n",0,0,0,0,0,0);
            return  EP_SYS_ERR;
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
    bOtherInitSuccess=RE_ControlWordTuyuanReadFileOtherInit
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

           最后进行录波,标志,遥信,遥测的初始化
           当连表中的所有图元都完成了初始化操作后,
           上层程序,会释放掉初始化节点的所有内存.
*/
/****参数：pElemInitNode  , 图元的操纵的初始化数据节点指针***************************/
/*           pElemScanNode   ,图元操纵的扫描数据节点指针
           pGrpScanNodeList   图元待访问的逻辑分图扫描数据节点连表
*/
/*   返回值，EP_STATUS */

EP_STATUS   RE_ControlWordTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
        LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    ControlWord_Init_Node_Type    *pInitNode;
    ControlWord_Scan_Node_Type    *pScanNode;
    NODE  *  pCurSourceNode;
    int  i;
    uint8_t  * pucCurInSignalType;
    uint16_t   * punCurInSignalSourceSeqNo;
    uint8_t   *  pucCurInSignalSourceOutNo;
    EP_ELEM_IO *  pInElem;

    uint8_t   ucCurInSignalType;
    EP_STATUS  SetCurOutAttribResult;

    EP_ELEM_IO  *  pCurElemOutput;


    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *    pbCurYaoxinSetFlag;
    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoxinID)[MAX_LOGID_STR_LEN+1];

    pInitNode=(ControlWord_Init_Node_Type    *)pElemInitNode->pTuyuan;
    pScanNode=(ControlWord_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    pucCurInSignalType=pInitNode->aucSourceSignalTypeArr;
    punCurInSignalSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucCurInSignalSourceOutNo=pInitNode->aucInSourceOutputNumArr;
    /* 循环设定图元的输入源指针  */
    for(i=0; i<(pScanNode->unInNum); i++)
    {

        pCurSourceNode=RE_LstNth(pGrpScanNodeList,
                                 ((*punCurInSignalSourceSeqNo)+1));/* 注意序号从1开始 */
        if(pCurSourceNode==NULL)
        {
            LOG_Dbg_Msg("Get  ControlWord Tuyuan Input  Source error!\n",0,0,0,0,0,0);
            assert(FALSE);
            return  EP_SYS_ERR;

        }

        /* 获得并设置当前输入源的指针 */

        pInElem=(* RE_GetScanNodeGetOutIOFunc(pCurSourceNode))
                ((*pucCurInSignalSourceOutNo),pCurSourceNode);


        /*  确保该图元输入在来源图元的输出号没有出界 */
        if(pInElem==NULL)
        {

            LOG_Dbg_Msg("Get  ControlWord  Tuyuan  input  info  error!\n",0,0,0,0,0,0);
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }
        /*  设置该IO指针到扫描节点相应的输入  */

        SetCurOutAttribResult=RE_ControlWordTuyuanSetInputIO
                              (i,pInElem,pElemScanNode);
        if(SetCurOutAttribResult!=EP_SUCCESS)
        {

            LOG_Dbg_Msg("Set  ControlWord  Tuyuan  input  info  error!\n",0,0,0,0,0,0);
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }
        /*比较信号类型是否匹配  */

        ucCurInSignalType=pInElem->ucAttrib;

        if(ucCurInSignalType!=(*pucCurInSignalType))
        {
            LOG_Dbg_Msg("ControlWord  Tuyuan One  Input  Signal  Type  Can't  Match!\n",0,0,0,0,0,0);
            assert(FALSE);
            return  EP_SYS_ERR;
        }

        pucCurInSignalType++;
        punCurInSignalSourceSeqNo++;
        pucCurInSignalSourceOutNo++;
    }



    /* 循环设定图元的输出属性*/
    pCurElemOutput=RE_ControlWordTuyuanGetOutIO
                   (0,pElemScanNode);

    /*   */
    if(pCurElemOutput==NULL)
    {

        LOG_Dbg_Msg("ControlWord  Tuyuan  Output  info  error!\n",0,0,0,0,0,0);
        assert(FALSE);  /* 若非以上类型,则告警   */
        return  EP_SYS_ERR;

    }

    pbCurLuboSetFlag=pInitNode->PublicElemData.abLuboFlagArr;
    pbCurFlagSetFlag=pInitNode->PublicElemData.abFlagSetFlagArr;
    pbCurYaoxinSetFlag=pInitNode->PublicElemData.abYaoxinFlagArr;


    /* 定义用指向数组的指针，这里未用双重指针, */
    pstrCurLuboID=pInitNode->aStrLuboIDArr;
    pstrCurFlagID=pInitNode->aStrFlagIDArr;
    pstrCurYaoxinID=pInitNode->aStrYaoxinIDArr;

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

                LOG_Dbg_Msg("Set  ControlWord  Tuyuan  \'%s\'  Lubo  Value  Error!\n"
                            ,(int)(*pstrCurLuboID),0,0,0,0,0);
                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误,设置控制字图元录波错误,录波标识为\'%s\'\n"
                               ,(int)(*pstrCurLuboID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:set controlword element's disturbance record id \'%s\' err\n"
                               ,(int)(*pstrCurLuboID),0);
                }
                assert(FALSE);
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

                LOG_Dbg_Msg("Set  ControlWord  Tuyuan  \'%s\'  Flag  Value  Error!\n"
                            ,(int)(*pstrCurFlagID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误,设置控制字图元标志错误,标志标识为\'%s\'\n"
                               ,(int)(*pstrCurFlagID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:set controlword  element's flag id\'%s\' err\n"
                               ,(int)(*pstrCurFlagID),0);
                }
                assert(FALSE);
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

                LOG_Dbg_Msg("Set  ControlWord  Tuyuan  \'%s\'  Yaoxin  Value  Error!\n"
                            ,(int)(*pstrCurYaoxinID),0,0,0,0,0);
                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误,设置控制字图元的遥信错误,遥信标识为\'%s\'\n"
                               ,(int)(*pstrCurYaoxinID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:set controlword  element remote signal id\'%s\' err\n"
                               ,(int)(*pstrCurYaoxinID),0);
                }
                assert(FALSE);
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
BOOL      RE_ControlWordTuyuanReadFileOtherInit
(ControlWord_Init_Node_Type * pElemInitNodePointer)
{

    pElemInitNodePointer->PublicElemData.elem.ucType=RE_CONTROLWORD;
    /*  设置图元输入数组指针 */
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_ControlWordTuyuanScanInit;
    /* 设置获得IO输出的函数 */
    pElemInitNodePointer->PublicElemData.pfGetScanNodeOutIOFunc
        =RE_ControlWordTuyuanGetOutIO;

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

EP_STATUS   RE_ControlWordCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{
    NODE  *pNode;
    char   strRelayFuncName1[300];/* 供返回该控制字定植所配置的
                    属于保护功能名属性,可以和当前所在保护功能比较,
                    进行纠错,目前该功能未实现,将来可实现 */
    SCI_SIGNAL_VALUE_TYPE  SettingValue;
    ControlWord_Scan_Node_Type    *pScanNode;
    ControlWord_Init_Node_Type    *pInitNode;
    EP_STATUS    OpeResult;


    pInitNode=(ControlWord_Init_Node_Type    *)pElemInitNode->pTuyuan;

    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("ControlWord  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pScanNode=(ControlWord_Scan_Node_Type    *)malloc
              (sizeof(ControlWord_Scan_Node_Type));
    if(pScanNode==NULL)
    {
        LOG_Dbg_Msg("ControlWord  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pNode->ulTuyuanType=RE_CONTROLWORD_SCAN;
    pNode->pTuyuan=(void  *)pScanNode;
    pScanNode->ioOut=pInitNode->
                     PublicElemData.elem.aioOut[0];
    pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                      PublicElemData.pfGetScanNodeOutIOFunc;
    pScanNode->pbCurScanDingzhiRreshFlag=
        pInitNode->PublicElemData.pbCurScanDingzhiRreshFlag;
    pScanNode->unInNum=
        pInitNode->PublicElemData.elem.unInNum;


    /*根据逻辑标识获得控制字的配置信息,并获得当前值  */
    pScanNode->ControlWordInfo.
    strRelayFuncName=strRelayFuncName1;

    OpeResult=SCI_Init_Get_Setting_Info(
                  pInitNode->strControlWordID,
                  &(pScanNode->ControlWordInfo)
              );

    if(OpeResult!=EP_SUCCESS)
    {
        /*  若不成功,则失败*/

        LOG_Dbg_Msg("Get  ControlWord  Tuyuan  \'%s\' Setting  Info  failure !\n"
                    ,(int)(pInitNode->strControlWordID),0,0,0,0,0);
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得控制字图元定值标识为\'%s\'的定值信息失败\n"
                       ,(int)(pInitNode->strControlWordID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get controlword  element's setting id \'%s\' info failure\n"
                       ,(int)(pInitNode->strControlWordID),0);
        }
        assert(FALSE);
        return  FALSE;
    }
    /*根据该定植是一般定植,还是公共定植,获得定植当前值  */
    if(pScanNode->ControlWordInfo.ucType==1)
    {
        /*若该定植是内部定植,则访问该内部定植  */

        OpeResult=SCI_Get_Inner_Setting(
                      pScanNode->ControlWordInfo.nNumInPage,
                      &SettingValue);

        if(OpeResult!=EP_SUCCESS)
        {
            /*  若不成功,则失败*/

            LOG_Dbg_Msg("Get  ControlWord  Tuyuan  \'%s\'  Inner  Setting  Value   failure!\n"
                        ,(int)(pInitNode->strControlWordID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得控制字图元标识为\'%s\'的内部定值失败\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get controlword  element internal setting id \'%s\' failure\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            assert(FALSE);
            return  FALSE;
        }

        /*  确保返回的定植的单位为控制字,否则出错*/

        if(SettingValue.ucAttrib!=CONTROL_WORD_SIGNAL)
        {
            LOG_Dbg_Msg("Get  ControlWord  Tuyuan  \'%s\'  Inner  Setting  Signal  Type   error !\n"
                        ,(int)(pInitNode->strControlWordID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得控制字图元标识为\'%s\'的内部定值信号类型错误\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get controlword  element internal setting id \'%s\' signal type failure\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            assert(FALSE);
            return  FALSE;

        }

        /* 确保控制字内容不超过控制字输入数 */
        if(SettingValue.Value.ulVal>=
                (pScanNode->unInNum))
        {
            LOG_Dbg_Msg("Get  ControlWord  Tuyuan  \'%s\'  Inner  Setting  Value  Beyound  Input  Count !\n"
                        ,(int)(pInitNode->strControlWordID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得控制字图元标识为\'%s\'的内部定值超过输入个数\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get controlword  element id\'%s\' internal setting  value overrun input num\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            assert(FALSE);
            return  FALSE;
        }
        /* 获得控制字当前值  */
        pScanNode->ulControlWordCurValue=
            SettingValue.Value.ulVal;

    }
    else if(pScanNode->ControlWordInfo.ucType==0)
    {
        /*若该定植是外部定植,则访问该外部定植  */

        OpeResult=SCI_Get_General_Setting(
                      pScanNode->ControlWordInfo.cPageNum,
                      pScanNode->ControlWordInfo.nNumInPage,
                      &SettingValue);

        if(OpeResult!=EP_SUCCESS)
        {
            /*  若不成功,则失败*/

            LOG_Dbg_Msg("Get  ControlWord  diagram element  \'%s\'  General  Setting  Value   failure !\n"
                        ,(int)(pInitNode->strControlWordID),0,0,0,0,0);
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得控制字图元标识为\'%s\'一般定值失败\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get controlword  element id\'%s\' general setting value failure\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            assert(FALSE);
            return  FALSE;
        }

        /*  确保返回的定植的单位为控制字,否则出错*/

        if(SettingValue.ucAttrib!=CONTROL_WORD_SIGNAL)
        {
            LOG_Dbg_Msg("Get  ControlWord  Tuyuan  \'%s\'  General  Setting  Signal  Type   error!\n"
                        ,(int)(pInitNode->strControlWordID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得控制字图元标识为\'%s\'的一般定值信号类型错误\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get controlword  element id\'%s\'general setting signal type err\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            assert(FALSE);
            return  FALSE;

        }
        /* 确保控制字内容不超过控制字输入数 */
        if(SettingValue.Value.ulVal>=
                (pScanNode->unInNum))
        {
            LOG_Dbg_Msg("Get  ControlWord  Tuyuan  \'%s\'  General  Setting  Value  Beyound  Input  Count !\n"
                        ,(int)(pInitNode->strControlWordID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得控制字图元标识为\'%s\'的一般定值超过输入个数\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get controlword  element id \'%s\'general setting overrun input num\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            assert(FALSE);

            return  FALSE;
        }
        /* 获得控制字当前值 */
        pScanNode->ulControlWordCurValue=
            SettingValue.Value.ulVal;
    }
    else
    {
        /*若该定植是测控定植,则访问该测控定植  */
        OpeResult=SCI_Get_CK_Setting(
                      pScanNode->ControlWordInfo.nNumInPage,
                      &SettingValue);

        if(OpeResult!=EP_SUCCESS)
        {
            /*  若不成功,则失败*/

            LOG_Dbg_Msg("Get  ControlWord  Tuyuan  \'%s\'  CK  Setting  Value   failure!\n"
                        ,(int)(pInitNode->strControlWordID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得控制字图元标识为\'%s\'的测控定值失败\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get controlword  element id \'%s\'Monitoring setting fail\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            assert(FALSE);
            return  FALSE;
        }

        /*  确保返回的定植的单位为控制字,否则出错*/

        if(SettingValue.ucAttrib!=CONTROL_WORD_SIGNAL)
        {
            LOG_Dbg_Msg("Get  ControlWord  Tuyuan  \'%s\'  CK  Setting  Signal  Type   error !\n"
                        ,(int)(pInitNode->strControlWordID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得控制字图元标识为\'%s\'的测控定值信号类型错误\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get controlword  element id \'%s\'Monitoring setting signal type fail\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            assert(FALSE);
            return  FALSE;

        }

        /* 确保控制字内容不超过控制字输入数 */
        if(SettingValue.Value.ulVal>=
                (pScanNode->unInNum))
        {
            LOG_Dbg_Msg("Get  ControlWord  Tuyuan  \'%s\'  CK  Setting  Value  Beyound  Input  Count !\n"
                        ,(int)(pInitNode->strControlWordID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得控制字图元标识为\'%s\'的测控定值超过输入个数\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get controlword   element id \'%s\'Monitoring setting overrun input num\n"
                           ,(int)(pInitNode->strControlWordID),0);
            }
            assert(FALSE);
            return  FALSE;
        }
        /* 获得控制字当前值  */
        pScanNode->ulControlWordCurValue=
            SettingValue.Value.ulVal;
    }

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

EP_ELEM_IO *  RE_ControlWordTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode)
{

    ControlWord_Scan_Node_Type    *pTuyuanNode;

    pTuyuanNode=(ControlWord_Scan_Node_Type    *)pScanNode->pTuyuan;
    if(unOutNum>=1)
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Get One ControlWord  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
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

EP_STATUS   RE_ControlWordTuyuanSetInputIO
(uint8_t  ucInputNum,EP_ELEM_IO *  pElemIO,
 NODE  * pScanNode)
{

    ControlWord_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(ControlWord_Scan_Node_Type    *)pScanNode->pTuyuan;

    if(ucInputNum>=(pTuyuanNode->unInNum))
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Set One  ControlWord  Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  EP_SYS_ERR;

    }
    pTuyuanNode->apInArr[ucInputNum]=pElemIO;
    return   EP_SUCCESS;


}


/*   控制字图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_ControlWordTuyuanScan(NODE *pElemScanNode)
{
    ControlWord_Scan_Node_Type    *pScanNode;

    pScanNode=(ControlWord_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    if(!(*(pScanNode->pbCurScanDingzhiRreshFlag)))
    {
        /* 若不必刷新定值 */

        /*将控制字设定的输入的赋给输出 */
        pScanNode->ioOut.now.bVal=
            (*(pScanNode->apInArr+pScanNode->ulControlWordCurValue))
            ->now.bVal;
        return;

    }
    else
    {
        /* 若定植刷新标志为真 ，则刷新控制字定植*/

        SCI_SIGNAL_VALUE_TYPE  SettingValue;
        EP_STATUS   OpeResult;
        /*根据该定植是一般定植,还是公共定植,获得定植当前值  */
        if(pScanNode->ControlWordInfo.ucType==1)
        {
            /*若该定植是内部定植,则访问该内部定植  */
            OpeResult=SCI_Get_Inner_Setting(
                          pScanNode->ControlWordInfo.nNumInPage,
                          &SettingValue);

            /* 获得控制字当前值  */
            pScanNode->ulControlWordCurValue=
                SettingValue.Value.ulVal;

        }
        else if(pScanNode->ControlWordInfo.ucType==0)
        {
            /*若该定植是外部定植,则访问该外部定植  */

            OpeResult=SCI_Get_General_Setting(
                          pScanNode->ControlWordInfo.cPageNum,
                          pScanNode->ControlWordInfo.nNumInPage,
                          &SettingValue);

            /* 获得控制字当前值 */
            pScanNode->ulControlWordCurValue=
                SettingValue.Value.ulVal;
        }
        else
        {
            /*若该定植是测控定植,则访问该测控定植  */
            OpeResult=SCI_Get_CK_Setting(
                          pScanNode->ControlWordInfo.nNumInPage,
                          &SettingValue);

            /* 获得控制字当前值  */
            pScanNode->ulControlWordCurValue=
                SettingValue.Value.ulVal;
        }

    }

    /*将控制字设定的输入的赋给输出 */
    pScanNode->ioOut.now.bVal=
        (*(pScanNode->apInArr+pScanNode->ulControlWordCurValue))
        ->now.bVal;
    return;


}

