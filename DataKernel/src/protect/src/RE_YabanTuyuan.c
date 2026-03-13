/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_YabanTuyuan.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的压板元件的实现代码                        */
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

#include  "RE_YabanTuyuan.h"


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

EP_STATUS   RE_YabanTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                       TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
                                       NODE ** pRtElemInitNodePointer)
{
    NODE  *pNode;
    Yaban_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    int  i;
    unsigned  char   *   pucCurInSignalType;
    unsigned  short   *  punCurInSourceSeqNo;
    unsigned  char   *   pucInSourceOutputNo;

    BOOL  bOtherInitSuccess;

    unsigned   long   TempLong;
    unsigned  char      ucCurLuboSetFlag;
    unsigned   char     ucCurFlagSetFlag;
    unsigned   char     ucCurYaoxinSetFlag;

    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *    pbCurYaoxinSetFlag;

    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoxinID)[MAX_LOGID_STR_LEN+1];

    EP_ELEM_IO  *  pCurElemOutput;

    unsigned  long  ulIDStrLen;


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
        LOG_Dbg_Msg("Yaban  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }

    pInitNode=(Yaban_Init_Node_Type    *)malloc(sizeof(Yaban_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("Yaban  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_YABAN;
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


    /* 读取压板的逻辑标识  */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 ( fp,pInitNode->strYabanID,&ulIDStrLen);
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

            LOG_Dbg_Msg("Read   Yaban  Tuyuan  Lubo  Flag   from  Logrp  SeqFile  error!\n",0,0,0,0,0,0);
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

            LOG_Dbg_Msg("Read  Yaban  Tuyuan   FlagSet  Flag   from  Logrp  SeqFile  error!\n",0,0,0,0,0,0);
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
            LOG_Dbg_Msg("Read  Yaban  Tuyuan  Yaoxin  Flag   from  Logrp  SeqFile  error!\n",0,0,0,0,0,0);
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
    bOtherInitSuccess=RE_YabanTuyuanReadFileOtherInit
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

EP_STATUS   RE_YabanTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                   LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{


    Yaban_Init_Node_Type    *pInitNode;
    Yaban_Scan_Node_Type    *pScanNode;
    NODE  *  pCurSourceNode;
    EP_STATUS  SetCurOutAttribResult;

    EP_ELEM_IO  *  pCurElemOutput;

    BOOL  *   pbCurLuboSetFlag;
    BOOL  *	  pbCurFlagSetFlag;
    BOOL  *    pbCurYaoxinSetFlag;

    char   (*  pstrCurLuboID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurFlagID)[MAX_LOGID_STR_LEN+1];
    char   (*  pstrCurYaoxinID)[MAX_LOGID_STR_LEN+1];

    uint8_t   ucCurInSignalType;
    uint8_t  * pucCurInSignalType;
    uint16_t   * punCurInSignalSourceSeqNo;
    uint8_t   *  pucCurInSignalSourceOutNo;
    EP_ELEM_IO *  pInElem;

    pInitNode=(Yaban_Init_Node_Type    *)pElemInitNode->pTuyuan;
    pScanNode=(Yaban_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    pucCurInSignalType=pInitNode->aucSourceSignalTypeArr;
    punCurInSignalSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucCurInSignalSourceOutNo=pInitNode->aucInSourceOutputNumArr;

    /* 循环设定图元的输入源指针  */

    pCurSourceNode=RE_LstNth(pGrpScanNodeList,
                             ((*punCurInSignalSourceSeqNo)+1));/* 注意序号从1开始 */
    if(pCurSourceNode==NULL)
    {
        LOG_Dbg_Msg("error,Get  Yaban  Tuyuan  Input Source  Init  failure!\n",0,0,0,0,0,0);


        assert(FALSE);
        return  EP_SYS_ERR;

    }

    /* 获得并设置当前输入源的指针 */

    pInElem=(* RE_GetScanNodeGetOutIOFunc(pCurSourceNode))
            ((*pucCurInSignalSourceOutNo),pCurSourceNode);


    /*  确保该图元输入在来源图元的输出号没有出界 */
    if(pInElem==NULL)
    {

        LOG_Dbg_Msg("error,Get  Yaban  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);


        assert(FALSE);  /* 若非以上类型,则告警   */
        return  EP_SYS_ERR;

    }
    /*  设置该IO指针到扫描节点相应的输入  */

    SetCurOutAttribResult=RE_YabanTuyuanSetInputIO
                          (0,pInElem,pElemScanNode);
    if(SetCurOutAttribResult!=EP_SUCCESS)
    {

        LOG_Dbg_Msg("error,Set  Yaban  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);


        assert(FALSE);  /* 若非以上类型,则告警   */
        return  EP_SYS_ERR;

    }

    /*比较信号类型是否匹配  */

    ucCurInSignalType=pInElem->ucAttrib;
    if(ucCurInSignalType!=(*pucCurInSignalType))
    {
        LOG_Dbg_Msg("error,Yaban Tuyuan One  Input  Signal Type  can't  Match!\n",0,0,0,0,0,0);


        assert(FALSE);
        return  EP_SYS_ERR;
    }

    /* 循环设定图元的输出属性*/

    pCurElemOutput=RE_YabanTuyuanGetOutIO
                   (0,pElemScanNode);

    if(pCurElemOutput==NULL)
    {

        LOG_Dbg_Msg("error,Can't  Get  Yaban  Tuyuan One Output  info!\n",0,0,0,0,0,0);


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
                LOG_Dbg_Msg("Set  Yaban  Tuyuan  \'%s\'  Lubo  Value  Error!\n"
                            ,(int)(*pstrCurLuboID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:设置压板图元的录波错误,该录波标识为\'%s\'\n"
                               ,(int)(*pstrCurLuboID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:set switch  element disturbance record id \'%s\' err\n"
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
                LOG_Dbg_Msg("Set  Yaban  Tuyuan  \'%s\'  Flag  Value  Error!\n"
                            ,(int)(*pstrCurFlagID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:设置压板图元的标志错误,该标志标识为\'%s\'\n"
                               ,(int)(*pstrCurFlagID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:set switch  element flag err,flag id is \'%s\'\n"
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
                LOG_Dbg_Msg("Set  Yaban  Tuyuan  \'%s\'  Yaoxin  Value  Error!\n"
                            ,(int)(*pstrCurYaoxinID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:设置压板图元的遥信错误,该遥信标识为\'%s\'\n"
                               ,(int)(*pstrCurYaoxinID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err:set switch  element remote signal err,remote signal id is \'%s\'\n"
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
BOOL      RE_YabanTuyuanReadFileOtherInit
(Yaban_Init_Node_Type * pElemInitNodePointer)
{

    pElemInitNodePointer->PublicElemData.elem.ucType=RE_YABAN;
    /*  设置图元输入数组指针 */
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_YabanTuyuanScanInit;
    /* 设置获得IO输出的函数 */
    pElemInitNodePointer->PublicElemData.pfGetScanNodeOutIOFunc
        =RE_YabanTuyuanGetOutIO;

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

EP_STATUS   RE_YabanCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{
    NODE  *pNode;
    Yaban_Scan_Node_Type    *pScanNode;
    Yaban_Init_Node_Type    *pInitNode;
    EP_STATUS    OpeResult;

    pInitNode=(Yaban_Init_Node_Type    *)pElemInitNode->pTuyuan;

    pNode=(NODE  *)malloc(sizeof(NODE));
    if(pNode==NULL)
    {
        LOG_Dbg_Msg("Yaban  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pScanNode=(Yaban_Scan_Node_Type    *)malloc
              (sizeof(Yaban_Scan_Node_Type));
    if(pScanNode==NULL)
    {
        LOG_Dbg_Msg("Yaban  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pNode->ulTuyuanType=RE_YABAN_SCAN;
    pNode->pTuyuan=(void  *)pScanNode;
    pScanNode->ioOut=pInitNode->
                     PublicElemData.elem.aioOut[0];
    pScanNode->pChartMsg=pInitNode->
                         PublicElemData.elem.pchart;
    pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                      PublicElemData.pfGetScanNodeOutIOFunc;

    /* 定值是否更新 */
    pScanNode->pbCurScanDingzhiRreshFlag
        = pInitNode->PublicElemData.pbCurScanDingzhiRreshFlag;


    /*根据压板的逻辑标识，获得压板号，并获得压板的当前值  */

    OpeResult=SCI_Init_Get_Yaban_Info(pInitNode->strYabanID,
                                      &(pScanNode->nYabanNum));
    if(OpeResult!=EP_SUCCESS)
    {
        /*  若不成功,则失败*/

        LOG_Dbg_Msg("Get  Yaban  Tuyuan  ID  \'%s\'  Yaban  Info  failure!\n"
                    ,(int)(pInitNode->strYabanID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得压板图元标识为 \'%s\'的压板信息失败\n"
                       ,(int)(pInitNode->strYabanID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get switch  element id \'%s\' info failure\n"
                       ,(int)(pInitNode->strYabanID),0);
        }
        assert(FALSE);
        return  FALSE;
    }

    OpeResult=SCI_Get_Yaban_Value(pScanNode->nYabanNum,
                                  &(pScanNode->bYabanCurValue),
                                  TM_Get_usCnt());
    if(OpeResult!=EP_SUCCESS)
    {
        /*  若不成功,则失败*/

        LOG_Dbg_Msg(" Get switch diagram element  ID  \'%s\'  \nswitch  Current Value   failure  By  switch  Num!\n"
                    ,(int)(pInitNode->strYabanID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:根据压板号,获得压板图元标识为\'%s\'的压板当前值失败\n"
                       ,(int)(pInitNode->strYabanID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get switch  element id \'%s\' value faiure by num\n"
                       ,(int)(pInitNode->strYabanID),0);
        }


        assert(FALSE);
        return  FALSE;
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

EP_ELEM_IO *  RE_YabanTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode)
{
    Yaban_Scan_Node_Type    *pTuyuanNode;

    pTuyuanNode=(Yaban_Scan_Node_Type    *)pScanNode->pTuyuan;
    if(unOutNum>=1)
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Get One Yaban  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
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

EP_STATUS   RE_YabanTuyuanSetInputIO
(uint8_t  ucInputNum,EP_ELEM_IO *  pElemIO,
 NODE  * pScanNode)
{

    Yaban_Scan_Node_Type    *pTuyuanNode;

    pTuyuanNode=(Yaban_Scan_Node_Type    *)pScanNode->pTuyuan;
    if(ucInputNum>=1)
    {
        /* 若越界，则出错 */

        LOG_Dbg_Msg("Set One  Yaban Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
        assert(FALSE);
        return  EP_SYS_ERR;

    }
    pTuyuanNode->pInArr0=pElemIO;
    return   EP_SUCCESS;



}



/*    压板图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
void     RE_YabanTuyuanScan(NODE *pElemScanNode)
{


    Yaban_Scan_Node_Type    *pScanNode;

    pScanNode=(Yaban_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    if (*(pScanNode->pbCurScanDingzhiRreshFlag))
    {
        SCI_Get_Yaban_Value(pScanNode->nYabanNum,
                            &(pScanNode->bYabanCurValue),
                            pScanNode->pChartMsg->ulScnTime);
    }

    if((pScanNode->bYabanCurValue))
    {
        /* 若压板当前为投入,则设置输出为输入值 */

        pScanNode->ioOut.now.bVal=
            (pScanNode->pInArr0->now.bVal);
        return;
    }
    else
    {
        /*若压板为退出,则设置输出为FALSE;  */
        pScanNode->ioOut.now.bVal=
            FALSE;
        return;

    }

    return;
}
