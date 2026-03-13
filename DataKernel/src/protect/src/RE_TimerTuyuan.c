/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_TimerTuyuan.c                                   1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的时间继电器元件的实现代码                        */
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

#include  "RE_TimerTuyuan.h"


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

EP_STATUS   RE_TimerTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                       TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
                                       NODE ** pRtElemInitNodePointer)
{
    NODE  *pNode;
    EP_ELEM_IO  *  pCurElemOutput;
    Timer_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    int  i;
    unsigned  long  ulStrLen;
    unsigned  char   *   pucCurInSignalType;
    unsigned  short   *  punCurInSourceSeqNo;
    unsigned  char   *   pucInSourceOutputNo;

    unsigned   long   TempLong;
    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoxinID)[MAX_LOGID_STR_LEN+1];

    BOOL  bOtherInitSuccess;

    unsigned  char      ucCurLuboSetFlag;
    unsigned   char     ucCurFlagSetFlag;
    unsigned   char     ucCurYaoxinSetFlag;

    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *    pbCurYaoxinSetFlag;
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
        LOG_Dbg_Msg("Timer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }

    pInitNode=(Timer_Init_Node_Type    *)malloc(sizeof(Timer_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("Timer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_TIMER;
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

    /*读取启动延时信息   */

    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&(pInitNode->ucStartSourceType));

    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    if((pInitNode->ucStartSourceType)==DINGZHI_SOURCE)
    {
        /* 若是定植,则读取定植名 */
        bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                     (fp,pInitNode->strStartSettingID,&ulStrLen);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }


    }
    else if((pInitNode->ucStartSourceType)==CONSTVALUE_SOURCE)
    {
        /* 若是常数,则读取常数值 */
        bReadSuccess=ReadUnsignedLongFromResloveSeqFile(fp,(long unsigned int *)&(pInitNode->ulStartSourceValue));
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        pInitNode->ulStartSourceValueAiCnt=(uint32_t)(((uint64_t)pInitNode->ulStartSourceValue*1000+(uint32_t)GetAiCntPeriod()/2)/(uint32_t)GetAiCntPeriod());
    }
    else
    {
        /* 若是其他来源,则出错 */

        LOG_Dbg_Msg("Timer Tuyuan  Data  Read  Error  From  Logrp  File!\n",0,0,0,0,0,0);
        return  EP_SYS_ERR;
    }


    /* 读取返回延时信息 */


    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&(pInitNode->ucReturnSourceType));
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    if((pInitNode->ucReturnSourceType)==DINGZHI_SOURCE)
    {
        /* 若是定植,则读取定植名 */
        bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                     (fp,pInitNode->strReturnsSettingID,&ulStrLen);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }


    }
    else if((pInitNode->ucReturnSourceType)==CONSTVALUE_SOURCE)
    {
        /* 若是常数,则读取常数值 */
        bReadSuccess=ReadUnsignedLongFromResloveSeqFile(fp,(long unsigned int *)&(pInitNode->ulReturnSourceValue));
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        pInitNode->ulReturnSourceValueAiCnt=(uint32_t)(((uint64_t)pInitNode->ulReturnSourceValue*1000+(uint32_t)GetAiCntPeriod()/2)/(uint32_t)GetAiCntPeriod());
    }
    else
    {
        /* 若是其他来源,则出错 */

        LOG_Dbg_Msg("Timer Tuyuan  Data  Read  Error  From  Logrp  File!\n",0,0,0,0,0,0);
        return  EP_SYS_ERR;
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

            LOG_Dbg_Msg("Read  Timer  Lubo  Flag   from  Logrp  SeqFile  error!\n",0,0,0,0,0,0);
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

            LOG_Dbg_Msg("Read  Timer   FlagSet  Flag   from  Logrp  SeqFile  error!\n",0,0,0,0,0,0);
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
            LOG_Dbg_Msg("Read Timer   Yaoxin  Flag   from  Logrp  SeqFile  error!\n",0,0,0,0,0,0);
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
    bOtherInitSuccess=RE_TimerTuyuanReadFileOtherInit
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

EP_STATUS   RE_TimerTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                   LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    Timer_Init_Node_Type    *pInitNode;
    Timer_Scan_Node_Type    *pScanNode;
    NODE  *  pCurSourceNode;
    uint8_t  * pucCurInSignalType;
    uint16_t   * punCurInSignalSourceSeqNo;
    uint8_t   *  pucCurInSignalSourceOutNo;
    EP_ELEM_IO *  pInElem;

    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoxinID)[MAX_LOGID_STR_LEN+1];


    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *    pbCurYaoxinSetFlag;

    EP_STATUS  SetCurOutAttribResult;

    EP_ELEM_IO  *  pCurElemOutput;

    uint8_t   ucCurInSignalType;


    pInitNode=(Timer_Init_Node_Type    *)pElemInitNode->pTuyuan;
    pScanNode=(Timer_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    pucCurInSignalType=pInitNode->aucSourceSignalTypeArr;
    punCurInSignalSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucCurInSignalSourceOutNo=pInitNode->aucInSourceOutputNumArr;


    pCurSourceNode=RE_LstNth(pGrpScanNodeList,
                             ((*punCurInSignalSourceSeqNo)+1));/* 注意序号从1开始 */
    if(pCurSourceNode==NULL)
    {
        LOG_Dbg_Msg("error,Get  Timer  Tuyuan  Input Source  Init  failure!\n",0,0,0,0,0,0);

        assert(FALSE);  /* 若非以上类型,则告警   */
        return  EP_SYS_ERR;

    }

    /* 获得并设置当前输入源的指针 */

    pInElem=(* RE_GetScanNodeGetOutIOFunc(pCurSourceNode))
            ((*pucCurInSignalSourceOutNo),pCurSourceNode);


    /*  确保该图元输入在来源图元的输出号没有出界 */
    if(pInElem==NULL)
    {

        LOG_Dbg_Msg("error,Get  Timer  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);

        assert(FALSE);  /* 若非以上类型,则告警   */
        return  EP_SYS_ERR;

    }
    /*  设置该IO指针到扫描节点相应的输入  */

    SetCurOutAttribResult=RE_TimerTuyuanSetInputIO
                          (0,pInElem,pElemScanNode);
    if(SetCurOutAttribResult!=EP_SUCCESS)
    {

        LOG_Dbg_Msg("error,Set  Timer  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);

        assert(FALSE);  /* 若非以上类型,则告警   */
        return  EP_SYS_ERR;

    }


    /*比较信号类型是否匹配  */

    ucCurInSignalType=pInElem->ucAttrib;

    if(ucCurInSignalType!=(*pucCurInSignalType))
    {
        LOG_Dbg_Msg("error,Timer Tuyuan One  Input  Signal Type  can't  Match!\n",0,0,0,0,0,0);


        assert(FALSE);  /* 若非以上类型,则告警   */
        return  EP_SYS_ERR;
    }


    /* 循环设定图元的输出属性*/

    pCurElemOutput=RE_TimerTuyuanGetOutIO
                   (0,pElemScanNode);

    if(pCurElemOutput==NULL)
    {

        LOG_Dbg_Msg("error,Can't  Get  Timer  Tuyuan One Output  info!\n",0,0,0,0,0,0);


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
                LOG_Dbg_Msg("Set  Timer  Tuyuan  \'%s\'  Lubo  Value  Error!\n"
                            ,(int)(*pstrCurLuboID),0,0,0,0,0);


                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:设置时间继电器图元的录波错误,其录波标识为 \'%s\'\n"
                               ,(int)(*pstrCurLuboID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logig grp rslv err:set timer  element disturbance record id \'%s\' err\n"
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
                LOG_Dbg_Msg("Set  Timer  Tuyuan  \'%s\'  Flag  Value  Error!\n"
                            ,(int)(*pstrCurFlagID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:设置时间继电器图元的标志错误,其标志标识为\'%s\'\n"
                               ,(int)(*pstrCurFlagID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:set timer  element flag id \'%s\' err\n"
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
                LOG_Dbg_Msg("Set  Timer  Tuyuan  \'%s\'  Yaoxin  Value  Error!\n"
                            ,(int)(*pstrCurYaoxinID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:设置时间继电器图元的遥信错误,其遥信标识为 \'%s\'\n"
                               ,(int)(*pstrCurYaoxinID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:set timer  element remote signal  id \'%s\' err\n"
                               ,(int)(*pstrCurYaoxinID),0);
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
BOOL      RE_TimerTuyuanReadFileOtherInit
(Timer_Init_Node_Type * pElemInitNodePointer)
{

    pElemInitNodePointer->PublicElemData.elem.ucType=RE_TIMER;
    /*  设置图元输入数组指针 */
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_TimerTuyuanScanInit;
    /* 设置获得IO输出的函数 */
    pElemInitNodePointer->PublicElemData.pfGetScanNodeOutIOFunc
        =RE_TimerTuyuanGetOutIO;

    return   TRUE;

}



/*   在图元扫描执行时，更新延时来源数据，
     主要是对启动和返回延时是定植时，进行定植刷新
      参数      pElemScanNodePointer,时间图元的扫描节点
      返回值，无

*/
void      RE_TimerTuyuanScanRefreshDelaySource(
    Timer_Scan_Node_Type    *pElemScanNodePointer)
{
    SCI_SIGNAL_VALUE_TYPE  SettingValue;
    EP_STATUS   OpeResult;

    Timer_Scan_Node_Type    *  pScanNode;
    pScanNode=pElemScanNodePointer;

    if((*(pScanNode->pbCurScanDingzhiRreshFlag)))
    {
        /* 若定植刷新标志为真 ，则刷新延时定植,否则不用更新定植*/

        if(pScanNode->ucStartSourceType==
                DINGZHI_SOURCE)
        {
            /* 若启动延时来源是定植 */

            /*根据该定植是一般定植,还是公共定植,获得定植当前值  */
            if(pScanNode->StartSettingSourceInfo.ucType==1)
            {
                /*若该定植是内部定植,则访问该内部定植  */
                OpeResult=SCI_Get_Inner_Setting(
                              pScanNode->StartSettingSourceInfo.nNumInPage,
                              &SettingValue);

                /* 确保返回成功 */
                assert(OpeResult==EP_SUCCESS);
                /* 确保返回类型一致 */
                assert((SettingValue.ucAttrib==SHIJIAN_TYPE1_SIGNAL)
                       ||(SettingValue.ucAttrib==SHIJIAN_TYPE2_SIGNAL));

            }
            else if(pScanNode->StartSettingSourceInfo.ucType==0)
            {
                /*若该定植是外部定植,则访问该外部定植  */

                OpeResult=SCI_Get_General_Setting(
                              pScanNode->StartSettingSourceInfo.cPageNum,
                              pScanNode->StartSettingSourceInfo.nNumInPage,
                              &SettingValue);
                /* 确保返回成功 */
                assert(OpeResult==EP_SUCCESS);
                /* 确保返回类型一致 */
                assert((SettingValue.ucAttrib==SHIJIAN_TYPE1_SIGNAL)
                       ||(SettingValue.ucAttrib==SHIJIAN_TYPE2_SIGNAL));

            }
            else
            {
                /*若该定植是测控定植,则访问该测控定植  */

                OpeResult=SCI_Get_CK_Setting(
                              pScanNode->StartSettingSourceInfo.nNumInPage,
                              &SettingValue);

                /* 确保返回成功 */
                assert(OpeResult==EP_SUCCESS);
                /* 确保返回类型一致 */
                assert((SettingValue.ucAttrib==SHIJIAN_TYPE1_SIGNAL)
                       ||(SettingValue.ucAttrib==SHIJIAN_TYPE2_SIGNAL));

            }

            /*将当前更新过的定植作为启动延时当前值*/

            /*将动作延时的单位类型转换为微秒,
               为了提高扫描时的效率  */
            if(SettingValue.ucAttrib==SHIJIAN_TYPE1_SIGNAL)
            {
                /* 若是秒 */
                float   fStartValue;
                fStartValue=SettingValue.Value.fVal;
                fStartValue=fStartValue*1000000.0f;
                if(fStartValue<(0.0f))
                {

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "定值校验错误:获得时间继电器图元的定值来源的动作延时越界\n"
                                   ,0,0);
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "Setting check error: action time of time relay element is out of limited\n"
                                   ,0,0);
                    }
                    fStartValue=0.0;
                }
                pScanNode->ulStartSourceValueAiCnt=(uint32_t)((fStartValue+GetAiCntPeriod()/2)/GetAiCntPeriod());


            }
            else
            {
                float   fStartValue;
                fStartValue=SettingValue.Value.fVal;
                fStartValue=fStartValue*1000.0f;
                if(fStartValue<(0.0f))
                {

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "定值校验错误:获得时间继电器图元的定值来源的动作延时越界\n"
                                   ,0,0);
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "Setting check error: action time of time relay element is out of limited\n"
                                   ,0,0);
                    }
                    fStartValue=0.0;
                }
                pScanNode->ulStartSourceValueAiCnt=(uint32_t)((fStartValue+GetAiCntPeriod()/2)/GetAiCntPeriod());


            }
        }

        if(pScanNode->ucReturnSourceType==
                DINGZHI_SOURCE)
        {
            /* 若返回延时来源是定植 */

            /*根据该定植是一般定植,还是公共定植,获得定植当前值  */
            if(pScanNode->ReturnSettingSourceInfo.ucType==1)
            {
                /*若该定植是内部定植,则访问该内部定植  */
                OpeResult=SCI_Get_Inner_Setting(
                              pScanNode->ReturnSettingSourceInfo.nNumInPage,
                              &SettingValue);

                /* 确保返回成功 */
                assert(OpeResult==EP_SUCCESS);
                /* 确保返回类型一致 */
                assert((SettingValue.ucAttrib==SHIJIAN_TYPE1_SIGNAL)
                       ||(SettingValue.ucAttrib==SHIJIAN_TYPE2_SIGNAL));


            }
            else if(pScanNode->ReturnSettingSourceInfo.ucType==0)
            {
                /*若该定植是外部定植,则访问该外部定植  */

                OpeResult=SCI_Get_General_Setting(
                              pScanNode->ReturnSettingSourceInfo.cPageNum,
                              pScanNode->ReturnSettingSourceInfo.nNumInPage,
                              &SettingValue);
                /* 确保返回成功 */
                assert(OpeResult==EP_SUCCESS);
                /* 确保返回类型一致 */
                assert((SettingValue.ucAttrib==SHIJIAN_TYPE1_SIGNAL)
                       ||(SettingValue.ucAttrib==SHIJIAN_TYPE2_SIGNAL));

            }
            else
            {
                /*若该定植是测控定植,则访问该测控定植  */
                OpeResult=SCI_Get_CK_Setting(
                              pScanNode->ReturnSettingSourceInfo.nNumInPage,
                              &SettingValue);

                /* 确保返回成功 */
                assert(OpeResult==EP_SUCCESS);
                /* 确保返回类型一致 */
                assert((SettingValue.ucAttrib==SHIJIAN_TYPE1_SIGNAL)
                       ||(SettingValue.ucAttrib==SHIJIAN_TYPE2_SIGNAL));

            }
            if(SettingValue.ucAttrib==SHIJIAN_TYPE1_SIGNAL)
            {
                /* 若是秒 */
                float   fReturnValue;
                fReturnValue=SettingValue.Value.fVal;
                fReturnValue=fReturnValue*1000000.0f;
                if(fReturnValue<(0.0f-FLT_PRECISION))
                {

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "定值校验错误:获得时间继电器图元的定值来源的返回延时越界\n"
                                   ,0,0);
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "Setting check error: return time of time relay element is out of limited\n"
                                   ,0,0);
                    }
                    fReturnValue=0.0;
                }
                pScanNode->ulReturnSourceValueAiCnt=(uint32_t)((fReturnValue+GetAiCntPeriod()/2)/GetAiCntPeriod());


            }
            else
            {
                float   fReturnValue;
                fReturnValue=SettingValue.Value.fVal;
                fReturnValue=fReturnValue*1000.0f;
                if(fReturnValue<(0.0f-FLT_PRECISION))
                {

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "定值校验错误:获得时间继电器图元的定值来源的返回延时越界\n"
                                   ,0,0);
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "Setting check error: return time of time relay element is out of limited\n"
                                   ,0,0);
                    }
                    fReturnValue=0.0;
                }
                pScanNode->ulReturnSourceValueAiCnt=(uint32_t)((fReturnValue+GetAiCntPeriod()/2)/GetAiCntPeriod());


            }
        }
    }
    return  ;
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

EP_STATUS   RE_TimerCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{
    NODE  *pNode;
    Timer_Scan_Node_Type    *pScanNode;
    Timer_Init_Node_Type    *pInitNode;
    EP_STATUS    OpeResult;

    pInitNode=(Timer_Init_Node_Type    *)pElemInitNode->pTuyuan;

    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("Timer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pScanNode=(Timer_Scan_Node_Type    *)malloc
              (sizeof(Timer_Scan_Node_Type));
    if(pScanNode==NULL)
    {
        LOG_Dbg_Msg("Timer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pNode->ulTuyuanType=RE_TIMER_SCAN;
    pNode->pTuyuan=(void  *)pScanNode;
    pScanNode->ioOut=pInitNode->
                     PublicElemData.elem.aioOut[0];
    pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                      PublicElemData.pfGetScanNodeOutIOFunc;
    pScanNode->pbCurScanDingzhiRreshFlag=
        pInitNode->PublicElemData.pbCurScanDingzhiRreshFlag;
    pScanNode->pchart=
        pInitNode->PublicElemData.elem.pchart;
    pScanNode->ucStartSourceType=
        pInitNode->ucStartSourceType;
    pScanNode->ucReturnSourceType=
        pInitNode->ucReturnSourceType;
    pScanNode->ulStartSourceValueAiCnt=
        pInitNode->ulStartSourceValueAiCnt;
    pScanNode->ulReturnSourceValueAiCnt=
        pInitNode->ulReturnSourceValueAiCnt;

    /*设置时间继电器的初值  */
    pScanNode->bStartDelayFlag=FALSE;/* 启动延时标志 */
    pScanNode->bBackDelayFlag=FALSE;/* 返回延时标志 */
    pScanNode->ulStartDelayAiCnt=0;/* 启动延时的计时器  */
    pScanNode->ulBackDelayAiCnt=0;/*  返回延时的计时器*/

    /* 若启动延时来源是定植,则获得定植的配置信息和当前的定植内容 */
    if((pScanNode->ucStartSourceType)==DINGZHI_SOURCE)
    {
        SCI_SIGNAL_VALUE_TYPE  SettingValue;

        char   strRelayFuncName1[300];/* 供返回该定植所配置的
                    属于保护功能名属性,可以和当前所在保护功能比较,
                    进行纠错,目前该功能未实现,将来可实现 */
        pScanNode->StartSettingSourceInfo.
        strRelayFuncName=strRelayFuncName1;

        OpeResult=SCI_Init_Get_Setting_Info(
                      pInitNode->strStartSettingID,
                      &(pScanNode->StartSettingSourceInfo)
                  );

        if(OpeResult!=EP_SUCCESS)
        {
            /*  若不成功,则失败*/

            LOG_Dbg_Msg("Get  Timer  Tuyuan   Start  Delay  \'%s\'    Setting  Info  failure!\n"
                        ,(int)(pInitNode->strStartSettingID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得时间继电器图元的动作延时的定值标识为\'%s\' 的定值来源信息失败\n"
                           ,(int)(pInitNode->strStartSettingID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get timer  element open delay setting id\'%s\'  info failure\n"
                           ,(int)(pInitNode->strStartSettingID),0);
            }


            assert(FALSE);  /* 若非以上类型,则告警   */
            return  FALSE;
        }
        /*根据该定植是一般定植,还是公共定植,获得定植当前值  */
        if(pScanNode->StartSettingSourceInfo.ucType==1)
        {
            /*若该定植是内部定植,则访问该内部定植  */

            OpeResult=SCI_Get_Inner_Setting(
                          pScanNode->StartSettingSourceInfo.nNumInPage,
                          &SettingValue);

            if(OpeResult!=EP_SUCCESS)
            {
                /*  若不成功,则失败*/

                LOG_Dbg_Msg("Get  Timer  Tuyuan   Start  Delay  \'%s\'  Inner  Setting  Value  By  Setting  Info  failure!\n"
                            ,(int)(pInitNode->strStartSettingID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得时间继电器图元的动作延时的定值标识为\'%s\' 的内部定值失败\n"
                               ,(int)(pInitNode->strStartSettingID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:get timer  element open delay setting id \'%s\' inner setting failure\n"
                               ,(int)(pInitNode->strStartSettingID),0);
                }

                assert(FALSE);  /* 若非以上类型,则告警   */
                return  FALSE;
            }

        }
        else if(pScanNode->StartSettingSourceInfo.ucType==0)
        {
            /*若该定植是外部定植,则访问该外部定植  */

            OpeResult=SCI_Get_General_Setting(
                          pScanNode->StartSettingSourceInfo.cPageNum,
                          pScanNode->StartSettingSourceInfo.nNumInPage,
                          &SettingValue);

            if(OpeResult!=EP_SUCCESS)
            {
                /*  若不成功,则失败*/

                LOG_Dbg_Msg("Get  Timer  Tuyuan   Start  Delay  \'%s\'   General  Setting  Value  By  Setting  Info  failure !\n"
                            ,(int)(pInitNode->strStartSettingID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得时间继电器图元动作延时的定值标识为\'%s\' 的通用定值失败\n"
                               ,(int)(pInitNode->strStartSettingID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:get timer  element open delay setting id \'%s\' general setting failure\n"
                               ,(int)(pInitNode->strStartSettingID),0);
                }

                assert(FALSE);  /* 若非以上类型,则告警   */
                return  FALSE;
            }

        }
        else
        {
            /*若该定植是测控定植,则访问该测控定植  */
            OpeResult=SCI_Get_CK_Setting(
                          pScanNode->StartSettingSourceInfo.nNumInPage,
                          &SettingValue);

            if(OpeResult!=EP_SUCCESS)
            {
                /*  若不成功,则失败*/

                LOG_Dbg_Msg("Get  Timer  Tuyuan   Start  Delay  \'%s\'  CK  Setting  Value  By  Setting  Info  failure!\n"
                            ,(int)(pInitNode->strStartSettingID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得时间继电器图元的动作延时的定值标识为\'%s\' 的测控定值失败\n"
                               ,(int)(pInitNode->strStartSettingID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "Get  Timer  Tuyuan   Start  Delay  \'%s\'  CK  Setting  Value  By  Setting  Info  failure!\n"
                               ,(int)(pInitNode->strStartSettingID),0);
                }

                assert(FALSE);  /* 若非以上类型,则告警   */
                return  FALSE;
            }

        }
        /*  确保返回的定植的单位为秒或毫秒,否则出错*/
        if((SettingValue.ucAttrib!=SHIJIAN_TYPE1_SIGNAL)
                &&(SettingValue.ucAttrib!=SHIJIAN_TYPE2_SIGNAL))
        {
            LOG_Dbg_Msg("Error,Timer  Tuyuan   Start  Delay  \'%s\'  Setting  Signal  Type  isn't  s or ms !\n"
                        ,(int)(pInitNode->strStartSettingID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:时间继电器图元动作延时的定值标识为\'%s\'的定值类型不是秒或毫秒\n"
                           ,(int)(pInitNode->strStartSettingID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:timer  element open delay setting id \'%s\' set type is not s,ms\n"
                           ,(int)(pInitNode->strStartSettingID),0);
            }
            assert(FALSE);

            return  FALSE;

        }
        /*将当前更新过的定植作为启动延时当前值*/

        /*将动作延时的单位类型转换为微秒,
           为了提高扫描时的效率  */
        if(SettingValue.ucAttrib==SHIJIAN_TYPE1_SIGNAL)
        {
            /* 若是秒 */
            float   fStartValue;
            fStartValue=SettingValue.Value.fVal;
            fStartValue=fStartValue*1000000.0f;
            if(fStartValue<(0.0f))
            {

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "定值校验错误:获得时间继电器图元的定值来源的动作延时越界\n"
                               ,0,0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "Setting check error: action time of time relay element is out of limited\n"
                               ,0,0);
                }
                fStartValue=0.0;
            }
            pScanNode->ulStartSourceValueAiCnt=(uint32_t)((fStartValue+GetAiCntPeriod()/2)/GetAiCntPeriod());

        }
        else
        {
            float   fStartValue;
            fStartValue=SettingValue.Value.fVal;
            fStartValue=fStartValue*1000.0f;
            if(fStartValue<(0.0f))
            {

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "定值校验错误:获得时间继电器图元的定值来源的动作延时越界\n"
                               ,0,0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "Setting check error: action time of time relay element is out of limited\n"
                               ,0,0);
                }
                fStartValue=0.0;
            }

            pScanNode->ulStartSourceValueAiCnt=(uint32_t)((fStartValue+GetAiCntPeriod()/2)/GetAiCntPeriod());


        }


    }


    /* 若返回延时来源是定植,则获得定植的配置信息和当前的定植内容 */
    if((pScanNode->ucReturnSourceType)==DINGZHI_SOURCE)
    {

        SCI_SIGNAL_VALUE_TYPE  ReturnSettingValue;

        char   strRelayFuncName2[300];/* 供返回该定植所配置的
                    属于保护功能名属性,可以和当前所在保护功能比较,
                    进行纠错,目前该功能未实现,将来可实现 */
        pScanNode->StartSettingSourceInfo.
        strRelayFuncName=strRelayFuncName2;

        OpeResult=SCI_Init_Get_Setting_Info(
                      pInitNode->strReturnsSettingID,
                      &(pScanNode->ReturnSettingSourceInfo)
                  );

        if(OpeResult!=EP_SUCCESS)
        {
            /*  若不成功,则失败*/

            LOG_Dbg_Msg("Get  Timer  Tuyuan   Back  Delay  \'%s\'    Setting  Info  failure!\n"
                        ,(int)(pInitNode->strReturnsSettingID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得时间继电器图元的返回延时的定值标识为\'%s\'的定值信息失败\n"
                           ,(int)(pInitNode->strReturnsSettingID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get timer element back delay setting id \'%s\' info err\n"
                           ,(int)(pInitNode->strReturnsSettingID),0);
            }


            assert(FALSE);  /* 若非以上类型,则告警   */
            return  FALSE;
        }
        /*根据该定植是一般定植,还是公共定植,获得定植当前值  */
        if(pScanNode->ReturnSettingSourceInfo.ucType==1)
        {
            /*若该定植是内部定植,则访问该内部定植  */

            OpeResult=SCI_Get_Inner_Setting(
                          pScanNode->ReturnSettingSourceInfo.nNumInPage,
                          &ReturnSettingValue);

            if(OpeResult!=EP_SUCCESS)
            {
                /*  若不成功,则失败*/

                LOG_Dbg_Msg("Get  Timer  Tuyuan   Back  Delay  \'%s\'   Inner  Setting  Value  failure  By  Setting  Info !\n"
                            ,(int)(pInitNode->strReturnsSettingID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误,获得时间继电器图元的返回延时定值标识为\'%s\'内部定值失败\n"
                               ,(int)(pInitNode->strReturnsSettingID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp  rslv err:get timer  element  back delay setting id \'%s\' inner setting failure\n"
                               ,(int)(pInitNode->strReturnsSettingID),0);
                }

                assert(FALSE);  /* 若非以上类型,则告警   */
                return  FALSE;
            }

        }
        else if(pScanNode->ReturnSettingSourceInfo.ucType==0)
        {
            /*若该定植是外部定植,则访问该外部定植  */

            OpeResult=SCI_Get_General_Setting(
                          pScanNode->ReturnSettingSourceInfo.cPageNum,
                          pScanNode->ReturnSettingSourceInfo.nNumInPage,
                          &ReturnSettingValue);

            if(OpeResult!=EP_SUCCESS)
            {
                /*  若不成功,则失败*/

                LOG_Dbg_Msg("Get  Timer  Tuyuan   Back  Delay  \'%s\'   General  Setting  Value  failure  By  Setting  Info !\n"
                            ,(int)(pInitNode->strReturnsSettingID),0,0,0,0,0);


                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得时间继电器图元的返回延时的定值标识为\'%s\'的一般定值失败\n"
                               ,(int)(pInitNode->strReturnsSettingID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp  rslv err:get timer  element  back delay setting id \'%s\'general setting failure\n"
                               ,(int)(pInitNode->strReturnsSettingID),0);
                }
                assert(FALSE);  /* 若非以上类型,则告警   */
                return  FALSE;
            }
        }
        else
        {
            /*若该定植是测控定植,则访问该测控定植  */
            OpeResult=SCI_Get_CK_Setting(
                          pScanNode->ReturnSettingSourceInfo.nNumInPage,
                          &ReturnSettingValue);

            if(OpeResult!=EP_SUCCESS)
            {
                /*  若不成功,则失败*/

                LOG_Dbg_Msg("Get  Timer  Tuyuan   Back  Delay  \'%s\'   Inner  Setting  Value  failure  By  Setting  Info !\n"
                            ,(int)(pInitNode->strReturnsSettingID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误,获得时间继电器图元的返回延时定值标识为\'%s\'测控定值失败\n"
                               ,(int)(pInitNode->strReturnsSettingID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "Get  Timer  Tuyuan   Back  Delay  \'%s\'   Inner  Setting  Value  failure  By  Setting  Info !\n"
                               ,(int)(pInitNode->strReturnsSettingID),0);
                }

                assert(FALSE);  /* 若非以上类型,则告警   */
                return  FALSE;
            }

        }
        /*  确保返回的定植的单位为秒或毫秒,否则出错*/
        if((ReturnSettingValue.ucAttrib!=SHIJIAN_TYPE1_SIGNAL)
                &&(ReturnSettingValue.ucAttrib!=SHIJIAN_TYPE2_SIGNAL))
        {
            LOG_Dbg_Msg("Error,Timer  Tuyuan   Back  Delay \'%s\'  Setting  Signal Type  isn't s or ms  !\n"
                        ,(int)(pInitNode->strReturnsSettingID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误,时间继电器返回延时的定值标识为\'%s\'的定值类型不是s或ms\n"
                           ,(int)(pInitNode->strReturnsSettingID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp  rslv err:timer  element back delay setting id \'%s\' setting type is not s,ms\n"
                           ,(int)(pInitNode->strReturnsSettingID),0);
            }
            assert(FALSE);

            return  FALSE;

        }
        if(ReturnSettingValue.ucAttrib==SHIJIAN_TYPE1_SIGNAL)
        {
            /* 若是秒 */
            float   fReturnValue;
            fReturnValue=ReturnSettingValue.Value.fVal;
            fReturnValue=fReturnValue*1000000.0f;
            if(fReturnValue<(0.0f-FLT_PRECISION))
            {
                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "定值校验错误:获得时间继电器图元的定值来源的返回延时越界\n"
                               ,0,0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "Setting check error: return time of time relay element is out of limited\n"
                               ,0,0);
                }
                fReturnValue=0.0;
            }
            pScanNode->ulReturnSourceValueAiCnt=(uint32_t)((fReturnValue+GetAiCntPeriod()/2)/GetAiCntPeriod());


        }
        else
        {
            float   fReturnValue;
            fReturnValue=ReturnSettingValue.Value.fVal;
            fReturnValue=fReturnValue*1000.0f;
            if(fReturnValue<(0.0f-FLT_PRECISION))
            {

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "定值校验错误:获得时间继电器图元的定值来源的返回延时越界\n"
                               ,0,0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "Setting check error: return time of time relay element is out of limited\n"
                               ,0,0);
                }
                fReturnValue=0.0;
            }
            pScanNode->ulReturnSourceValueAiCnt=(uint32_t)((fReturnValue+GetAiCntPeriod()/2)/GetAiCntPeriod());


        }

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

EP_ELEM_IO *  RE_TimerTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode)
{
    Timer_Scan_Node_Type    *pTuyuanNode;

    pTuyuanNode=(Timer_Scan_Node_Type    *)pScanNode->pTuyuan;
    if(unOutNum>=1)
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Get One Timer  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
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

EP_STATUS   RE_TimerTuyuanSetInputIO
(uint8_t  ucInputNum,EP_ELEM_IO *  pElemIO,
 NODE  * pScanNode)
{

    Timer_Scan_Node_Type    *pTuyuanNode;

    pTuyuanNode=(Timer_Scan_Node_Type    *)pScanNode->pTuyuan;
    if(ucInputNum>=1)
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Set One  Timer Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  EP_SYS_ERR;

    }
    pTuyuanNode->pInArr0=pElemIO;
    return   EP_SUCCESS;



}


/*    时间图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_TimerTuyuanScan(NODE *pElemScanNode)
{

    Timer_Scan_Node_Type    *pScanNode;
    pScanNode=(Timer_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    if((*(pScanNode->pbCurScanDingzhiRreshFlag))
            &&((pScanNode->ucStartSourceType==DINGZHI_SOURCE)
               ||(pScanNode->ucReturnSourceType==DINGZHI_SOURCE)))
    {
        /* 若有定植延时来源，且需要更新，则更新延时来源数据， */
        RE_TimerTuyuanScanRefreshDelaySource(pScanNode);
    }

    if(pScanNode->pInArr0->now.bVal)
    {
        /* 若输入为真 */
        if(pScanNode->ioOut.now.bVal)
        {
            /*若上次输出为真*/
            if(pScanNode->bBackDelayFlag)
            {
                /*  若返回延时标志为真，则表明前面的0信号输入是毛刺
                    ，则清除返回延时启动标志*/
                pScanNode->bBackDelayFlag=FALSE;
                pScanNode->ioOut.now.bVal=
                    TRUE;
                return;
            }
            else
            {
                pScanNode->ioOut.now.bVal=
                    TRUE;
                return;
            }
            return;
        }
        else
        {
            /*若上次输出为假  */
            if(pScanNode->bStartDelayFlag)
            {
                /* 若动作延时标志为真 */
                pScanNode->ulStartDelayAiCnt=pScanNode->ulStartDelayAiCnt+pScanNode->pchart->unNewAi;

            }
            else
            {
                /* 若动作延时标志为假,则启动动作延时 ,计时器开始计时*/
                pScanNode->bStartDelayFlag=TRUE;
                pScanNode->ulStartDelayCounter=0;
                pScanNode->ulStartDelayAiCnt=0;

            }

            if(pScanNode->ulStartDelayAiCnt>=
                    pScanNode->ulStartSourceValueAiCnt)

            {
                /* 若动作延时超时,则返回真,清空动作延时启动标志 */
                pScanNode->ioOut.now.bVal=TRUE;
                pScanNode->bStartDelayFlag=FALSE;
            }
            else
            {
                /* 表示动作延时尚未超时,则输出为假 */
                pScanNode->ioOut.now.bVal=
                    FALSE;
            }
            return;
        }
        return;
    }
    else
    {
        /* 若输入为假 */

        if(pScanNode->ioOut.now.bVal)
        {
            /*若上次输出为真*/
            if(pScanNode->bBackDelayFlag)
            {
                /*若返回延时已启动,  */
                pScanNode->ulBackDelayAiCnt=pScanNode->ulBackDelayAiCnt+pScanNode->pchart->unNewAi;
                /* 获得返回延时此次扫描时的大小,单位为us */
            }
            else
            {
                /* 否则返回延时未启动,则启动 */
                pScanNode->bBackDelayFlag=TRUE;
                pScanNode->ulBackDelayCounter=0;
                pScanNode->ulBackDelayAiCnt=0;
            }
            if(pScanNode->ulBackDelayAiCnt>=
                    pScanNode->ulReturnSourceValueAiCnt)
            {
                /* 若返回延时超时,则返回假,清空返回延时启动标志 */
                pScanNode->ioOut.now.bVal=
                    FALSE;
                pScanNode->bBackDelayFlag=FALSE;
            }
            else
            {
                /* 表示返回延时尚未超时,则输出为真 */
                pScanNode->ioOut.now.bVal=
                    TRUE;
            }
            return;
        }
        else
        {
            /*若上次输出为假  */

            if(pScanNode->bStartDelayFlag)
            {
                /*  若启动延时标志为真，则表明前面的1信号输入是毛刺
                    ，则清除动作延时启动标志*/
                pScanNode->bStartDelayFlag=FALSE;
                pScanNode->ioOut.now.bVal=
                    FALSE;
                return;
            }
            else
            {
                pScanNode->ioOut.now.bVal=
                    FALSE;
                return;
            }
            return;
        }
        return;
    }

    return;

}

