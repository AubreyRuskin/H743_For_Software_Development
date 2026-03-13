/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_OuterOutputTuyuan.C                            1.1                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的外部通道输出元件的代码实现                       */
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
/*                    2005.10.24              修改1.1版本                    */
/*                                                                              */
/********************************************************************************/

#include <vxWorks.h>

#include  "RE_OuterOutputTuyuan.h"
#include   "string_compat.h"

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

EP_STATUS   RE_OuterOutputTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
        TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
        NODE ** pRtElemInitNodePointer)
{
    NODE  *pNode;
    OuterOutput_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    int  i;
    unsigned  char   *   pucCurInSignalType;
    unsigned  short   *  punCurInSourceSeqNo;
    unsigned  char   *   pucInSourceOutputNo;

    unsigned  long  ulIDStrLen;
    BOOL  bOtherInitSuccess;

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
        LOG_Dbg_Msg("OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    pInitNode=(OuterOutput_Init_Node_Type    *)malloc(sizeof(OuterOutput_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_OUTEROUTPUT;
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
        /*读取输入信号类型  */
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


    /* 读取输出信号目的地类型*/

    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&(pInitNode->ucSignalDestType));
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    /* 设置该扫描节点的输出个数 */
    pInitNode->PublicElemData.elem.ucOutNum=0;
    if((pInitNode->ucSignalDestType)==HINTLAMP_OUTPUT_DEST)
    {
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,(unsigned char *)&(pInitNode->bLampAdjust));
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&(pInitNode->ucLampColor));
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&(pInitNode->ucLampBlink));
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,(unsigned char *)&(pInitNode->bLampKeep));
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
    }
    else
    {
        /* 读取4个保留字节  */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   EP_SYS_ERR;
        }
    }
    /*  读取输出信号目的地逻辑标识 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 ( fp,pInitNode->strOutputDestID,&ulIDStrLen);
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

    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_OuterOutputTuyuanReadFileOtherInit
                      (pInitNode);
    assert(bOtherInitSuccess);
    if(!bOtherInitSuccess)
    {
        return  EP_SYS_ERR;
    }

    /* 设定返回的节点地址  */
    *pRtElemInitNodePointer=pNode;

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

EP_STATUS   RE_OuterOutputTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
        LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    OuterOutput_Init_Node_Type    *pInitNode;
    NODE  *  pCurSourceNode;
    int  i;
    uint8_t  * pucCurInSignalType;
    uint16_t   * punCurInSignalSourceSeqNo;
    uint8_t   *  pucCurInSignalSourceOutNo;
    EP_ELEM_IO *  pInElem;

    EP_STATUS  SetCurOutAttribResult;

    uint8_t   ucCurInSignalType;


    pInitNode=(OuterOutput_Init_Node_Type    *)pElemInitNode->pTuyuan;
    pucCurInSignalType=pInitNode->aucSourceSignalTypeArr;
    punCurInSignalSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucCurInSignalSourceOutNo=pInitNode->aucInSourceOutputNumArr;
    /* 循环设定图元的输入源指针  */
    for(i=0; i<(pInitNode->PublicElemData.elem.unInNum); i++)
    {

        pCurSourceNode=RE_LstNth(pGrpScanNodeList,
                                 ((*punCurInSignalSourceSeqNo)+1));/* 注意序号从1开始 */

        if(pCurSourceNode==NULL)
        {
            LOG_Dbg_Msg("error,Get  OuterOutput  Tuyuan  Input Source  Init  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }

        /* 获得并设置当前输入源的指针 */

        pInElem=(*  RE_GetScanNodeGetOutIOFunc(pCurSourceNode))
                ((*pucCurInSignalSourceOutNo),pCurSourceNode);


        /*  确保该图元输入在来源图元的输出号没有出界 */
        if(pInElem==NULL)
        {

            LOG_Dbg_Msg("error,Get  OuterOutput  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }
        /*  设置该IO指针到扫描节点相应的输入  */

        SetCurOutAttribResult=RE_OuterOutputTuyuanSetInputIO
                              (i,pInElem,pElemScanNode);
        if(SetCurOutAttribResult!=EP_SUCCESS)
        {
            LOG_Dbg_Msg("error,Set  OuterOutput  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }

        /*比较信号类型是否匹配  */

        ucCurInSignalType=pInElem->ucAttrib;

        if(ucCurInSignalType!=(*pucCurInSignalType))
        {
            LOG_Dbg_Msg("error,OuterOutput Tuyuan One  Input  Signal Type  can't  Match!\n",0,0,0,0,0,0);


            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        pucCurInSignalType++;
        punCurInSignalSourceSeqNo++;
        pucCurInSignalSourceOutNo++;
    }

    return  EP_SUCCESS;
}


/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_OuterOutputTuyuanReadFileOtherInit
(OuterOutput_Init_Node_Type * pElemInitNodePointer)
{

    BOOL   bInitResult;
    pElemInitNodePointer->PublicElemData.elem.ucType=RE_OUTEROUTPUT;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_OuterOutputTuyuanScanInit;

    switch(pElemInitNodePointer->ucSignalDestType)
    {
        case DIGITAL_OUTPUT_DEST:

            pElemInitNodePointer->pvDestHandle=
                RD_Get_Handle(pElemInitNodePointer->strOutputDestID,RD_LGC_DO_HDL);

            if((pElemInitNodePointer->pvDestHandle)==NULL)
            {
                LOG_Dbg_Msg("Get DO Dest OuterOutput Tuyuan \'%s\' Handle Error!\n"
                            ,(int)(pElemInitNodePointer->strOutputDestID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得DO图元\'%s\'的句柄错误\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp  rslv err:get do element id \'%s\' handle err\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                assert(FALSE);  /* 若非以上类型,则告警   */
                return   FALSE;
            }

            break;
        case  HINTLAMP_OUTPUT_DEST:

            pElemInitNodePointer->pvDestHandle=
                RD_Get_Handle(pElemInitNodePointer->strOutputDestID,RD_LGC_LED_HDL);

            if((pElemInitNodePointer->pvDestHandle)==NULL)
            {
                LOG_Dbg_Msg("Get HintLamp  Dest  OuterOutput  Tuyuan   \'%s\'   Handle  Error  !\n"
                            ,(int)(pElemInitNodePointer->strOutputDestID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得指示灯图元\'%s\'的句柄错误\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err; get lamp element id \'%s\' handle err\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }

                assert(FALSE);  /* 若非以上类型,则告警   */
                return   FALSE;
            }
#if 1
            if(pElemInitNodePointer->bLampAdjust)
            {
                /* 是否调整判断 DY 6/5/2007 */
                RD_Chg_Led_Attr(pElemInitNodePointer->pvDestHandle,pElemInitNodePointer->bLampKeep,
                                pElemInitNodePointer->ucLampColor, pElemInitNodePointer->ucLampBlink);
            }
#endif

#if 0
            RD_Chg_Led_Attr(pElemInitNodePointer->pvDestHandle,pElemInitNodePointer->bLampKeep,
                            pElemInitNodePointer->ucLampColor, pElemInitNodePointer->ucLampBlink);
#endif
            break;
        case  AI_VIRTUAL_CH_DEST:
            bInitResult=RE_OuterOutputTuyuanAIDestReadFileOtherInit
                        (pElemInitNodePointer);
            if(!bInitResult)
            {
                assert(FALSE);
                return  FALSE;
            }

            break;

        case  ANALOG_OUTPUT_DEST:
            bInitResult=RE_OuterOutputTuyuanAODestReadFileOtherInit
                        (pElemInitNodePointer);
            if(!bInitResult)
            {
                assert(FALSE);
                return  FALSE;
            }

            break;

        case AI_PLUSADT_DEST:

            pElemInitNodePointer->pvDestHandle=
                RD_Get_Handle(pElemInitNodePointer->strOutputDestID,RD_LGC_AI_HDL);

            if((pElemInitNodePointer->pvDestHandle)==NULL)
            {
                LOG_Dbg_Msg("Get AIPlusCof Output Tuyuan \'%s\' Handle Error!\n"
                            ,(int)(pElemInitNodePointer->strOutputDestID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得ai对应物理通道的增益系数设置图元\'%s\'的句柄错误\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err; get ai physical channel gain coefficient element  \'%s\' handle err\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }

                assert(FALSE);  /* 若非以上类型,则告警   */
                return   FALSE;
            }

            break;

        case AI_OFFADT_DEST:

            pElemInitNodePointer->pvDestHandle=
                RD_Get_Handle(pElemInitNodePointer->strOutputDestID,RD_LGC_AI_HDL);

            if((pElemInitNodePointer->pvDestHandle)==NULL)
            {
                LOG_Dbg_Msg("Get AIOffCof Output Tuyuan \'%s\' Handle Error!\n"
                            ,(int)(pElemInitNodePointer->strOutputDestID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得ai对应物理通道的偏置系数设置图元\'%s\'的句柄错误\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err; get ai physical channel offset coefficient element  \'%s\' handle err\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                assert(FALSE);  /* 若非以上类型,则告警   */
                return   FALSE;
            }

            break;

        case CELIANG_PLUSADT_DEST:

            pElemInitNodePointer->AIDestChOffset=
                ME_Get_Msu_Idx(pElemInitNodePointer->strOutputDestID);

            if((pElemInitNodePointer->AIDestChOffset)==-1)
            {
                LOG_Dbg_Msg("Get ClPlusCof Output Tuyuan \'%s\' Handle Error!\n"
                            ,(int)(pElemInitNodePointer->strOutputDestID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得测量量增益系数设置图元\'%s\'的句柄错误\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err; get measurement gain coefficient element  \'%s\' handle err\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                assert(FALSE);  /* 若非以上类型,则告警   */
                return   FALSE;
            }

            break;

        case CELIANG_OFFADT_DEST:

            pElemInitNodePointer->AIDestChOffset=
                ME_Get_Msu_Idx(pElemInitNodePointer->strOutputDestID);

            if((pElemInitNodePointer->AIDestChOffset)==-1)
            {
                LOG_Dbg_Msg("Get ClOffCof Output Tuyuan \'%s\' Handle Error!\n"
                            ,(int)(pElemInitNodePointer->strOutputDestID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得测量量偏置系数设置图元\'%s\'的句柄错误\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err; get measurement offset coefficient element  \'%s\' handle err\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                assert(FALSE);  /* 若非以上类型,则告警   */
                return   FALSE;
            }

            break;

        case DI_XIAODOU_DEST:

            pElemInitNodePointer->pvDestHandle=
                RD_Get_Handle(pElemInitNodePointer->strOutputDestID,RD_LGC_DI_HDL);

            if((pElemInitNodePointer->pvDestHandle)==NULL)
            {
                LOG_Dbg_Msg("Get DI xiaodou time Output Tuyuan \'%s\' Handle Error!\n"
                            ,(int)(pElemInitNodePointer->strOutputDestID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得di 消抖时间设置图元\'%s\'的句柄错误\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err; get di elimination jitter element  \'%s\' handle err\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                assert(FALSE);  /* 若非以上类型,则告警   */
                return   FALSE;
            }

            break;

        case YAOCE_OVERADT_DEST:

            pElemInitNodePointer->AIDestChOffset=
                VI_Get_Mea_AI_Idx(pElemInitNodePointer->strOutputDestID);

            if((pElemInitNodePointer->AIDestChOffset)==-1)
            {
                LOG_Dbg_Msg("Get YcOverCof Output Tuyuan \'%s\' Handle Error!\n"
                            ,(int)(pElemInitNodePointer->strOutputDestID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得遥测量越限系数设置图元\'%s\'的句柄错误\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err; get telemetry Threshold coefficient element  \'%s\' handle err\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                assert(FALSE);  /* 若非以上类型,则告警   */
                return   FALSE;
            }

            break;

        case CELIANG_OVERADT_DEST:

            pElemInitNodePointer->AIDestChOffset=
                ME_Get_Msu_Idx(pElemInitNodePointer->strOutputDestID);

            if((pElemInitNodePointer->AIDestChOffset)==-1)
            {
                LOG_Dbg_Msg("Get ClOverCof Output Tuyuan \'%s\' Handle Error!\n"
                            ,(int)(pElemInitNodePointer->strOutputDestID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得测量量越限系数设置图元\'%s\'的句柄错误\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err; get measurement Threshold coefficient element  \'%s\' handle err\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                assert(FALSE);  /* 若非以上类型,则告警   */
                return   FALSE;
            }

            break;

        case PULSE_OUTPUT_DEST:
            pElemInitNodePointer->pvDestHandle=
                RD_Get_Handle(pElemInitNodePointer->strOutputDestID,RD_LGC_PO_HDL);
            if((pElemInitNodePointer->pvDestHandle)==NULL)
            {
                LOG_Dbg_Msg("Get PO Output Tuyuan \'%s\' Handle Error!\n"
                            ,(int)(pElemInitNodePointer->strOutputDestID),0,0,0,0,0);

                if(ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "逻辑图解析错误:获得PO图元\'%s\'的句柄错误\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                else if(ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK,
                               "logic grp rslv err; get po  element  \'%s\' handle err\n"
                               ,(int)(pElemInitNodePointer->strOutputDestID),0);
                }
                assert(FALSE);  /* 若非以上类型,则告警   */
                return   FALSE;
            }
            break;
        default:
            /* 若是其他,则出错 */
            LOG_Dbg_Msg("Error,OuterOutput  Tuyuan  Output  Dest  Type isn't  Expected!\n",0,0,0,0,0,0);
            return   FALSE;
            break;

    }

    return   TRUE;
}


/*  在读取文件后,进行AI目的地类型的相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_OuterOutputTuyuanAIDestReadFileOtherInit
(OuterOutput_Init_Node_Type * pElemInitNode)
{

    uint8_t   ucSignalType;
    void  *  pvAiHandle;

    unsigned  char  ucHandleAttrib;

    /* 获得AI句柄  */
    pvAiHandle=RD_Get_Handle(pElemInitNode->strOutputDestID,RD_LGC_AI_HDL);
    if(pvAiHandle==NULL)
    {
        LOG_Dbg_Msg("Get Virtual  AI Dest  OuterOutput  Tuyuan  \'%s\'   Handle  Error!\n"
                    ,(int)(pElemInitNode->strOutputDestID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得虚拟AI通道输出图元\'%s\'的句柄错误\n"
                       ,(int)(pElemInitNode->strOutputDestID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err: get vt ai  element id \'%s\' handle err\n"
                       ,(int)(pElemInitNode->strOutputDestID),0);
        }
        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;
    }
    ucSignalType=pElemInitNode->aucSourceSignalTypeArr[0];
    ucHandleAttrib=AI_HND_TO_UNIT(pvAiHandle);

    if(ucHandleAttrib!=ucSignalType)
    {
        LOG_Dbg_Msg(" Virtual  AI  Dest  OuterOutput  Tuyuan  \'%s\' Signal  Type  isn't  Match  with  Config  Handle!\n"
                    ,(int)(pElemInitNode->strOutputDestID),0,0,0,0,0);
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得虚拟AI通道输出图元\'%s\'的信号类型同配置不一致\n"
                       ,(int)(pElemInitNode->strOutputDestID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp  rslv err:get  vt ai  element id\'%s\' signal type is not match with config\n"
                       ,(int)(pElemInitNode->strOutputDestID),0);
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
      )
    {
        /* 判定能否访问第2个采样节拍的物理AI通道数据指针 */
        float  *pfLgcAi;
        /* 获得访问AI数据指针 */
        pfLgcAi=RD_Lgc_AI_P(pvAiHandle, (RD_AI_Cnt()-10));/* 这里要求实时数据模块实现时
                                                  返回非空,出错,则返回空 */

        if(pfLgcAi==NULL)
        {
            LOG_Dbg_Msg("Get Virtual  AI Dest  OuterOutput  Tuyuan  \'%s\' Data  Error  By  Handle!\n"
                        ,(int)(pElemInitNode->strOutputDestID),0,0,0,0,0);
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得AI虚拟通道输出图元\'%s\'的数据错误\n"
                           ,(int)(pElemInitNode->strOutputDestID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp  rslv err:get VT AI  element id \'%s\' data err\n"
                           ,(int)(pElemInitNode->strOutputDestID),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return   FALSE;
        }


        /* 赋初值给输出 */
        pElemInitNode->pvDestHandle=pvAiHandle;/* 将AI句柄赋给属性 */
        pElemInitNode->AIDestChOffset=
            RD_Lgc_AI_Ofst(pvAiHandle);
        pElemInitNode->bPhyAIChFlag=TRUE;
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
            LOG_Dbg_Msg("Get Virtual  AI  Dest  OuterOutput  Tuyuan  \'%s\'  Data  Error  By  Handle!\n"
                        ,(int)(pElemInitNode->strOutputDestID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得虚拟AI通道输出图元\'%s\'的数据错误\n"
                           ,(int)(pElemInitNode->strOutputDestID),0);
            }
            else if(ENG_MODE == 1)

            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err: get vt ai  element id\'%s\' data err\n"
                           ,(int)(pElemInitNode->strOutputDestID),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return   FALSE;
        }
        /* 赋初值给输出 */
        pElemInitNode->pvDestHandle=pvAiHandle;/* 将AI句柄赋给属性 */
        pElemInitNode->AIDestChOffset=
            RD_Calc_AI_Ofst(pvAiHandle);
        pElemInitNode->bPhyAIChFlag=FALSE;

    }
    else
    {

        LOG_Dbg_Msg("Error,Virtual  AI  Dest OuterOutput  Tuyuan   \'%s\'   Signal  Type  isn't  Expected!\n"
                    ,(int)(pElemInitNode->strOutputDestID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:虚拟AI通道输出图元\'%s\'信号类型错误\n"
                       ,(int)(pElemInitNode->strOutputDestID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic  grp  rslv  err: vt ai  element id\'%s\' signal type err\n"
                       ,(int)(pElemInitNode->strOutputDestID),0);
        }
        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;

    }


    return  TRUE;
}



/*  在读取文件后,进行AO目的地类型的相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_OuterOutputTuyuanAODestReadFileOtherInit
(OuterOutput_Init_Node_Type * pElemInitNode)
{

    uint8_t   ucSignalType;

    ucSignalType=pElemInitNode->aucSourceSignalTypeArr[0];

    /* 若信号类型是实数式电流,电压,阻抗,则是物理通道 */
    if((ucSignalType==REAL_FORM_DIANLIU_SIGNAL)
            ||(ucSignalType==REAL_FORM_DIANLIU_SIGNAL_KA)
            ||(ucSignalType==REAL_FORM_DIANLIU_SIGNAL_MA)
            ||(ucSignalType==REAL_FORM_DIANYA_SIGNAL)
            ||(ucSignalType==REAL_FORM_DIANYA_SIGNAL_KV)
            ||(ucSignalType==REAL_FORM_ZUKANG_SIGNAL)
            ||(ucSignalType==REAL_FORM_ZUKANG_SIGNAL_KO)
      )
    {

    }
    else
    {

        LOG_Dbg_Msg("Error,AO  Dest OuterOutput  Tuyuan   \'%s\'   Signal  Type  isn't  Expected!\n"
                    ,(int)(pElemInitNode->strOutputDestID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:AO通道输出图元\'%s\'信号类型错误\n"
                       ,(int)(pElemInitNode->strOutputDestID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err: AO  element id \'%s\'signal type err\n"
                       ,(int)(pElemInitNode->strOutputDestID),0);
        }
        assert(FALSE);  /* 若非以上类型,则告警   */
        return   FALSE;

    }


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

EP_STATUS   RE_OuterOutputCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{

    NODE  *pNode;
    OuterOutput_Init_Node_Type    *pInitNode;
    pInitNode=(OuterOutput_Init_Node_Type    *)pElemInitNode->pTuyuan;

    /*  设置图元类型*/
    switch(pInitNode->ucSignalDestType)
    {
        case  DIGITAL_OUTPUT_DEST:
        {
            DOOuterOutput_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("DO  Dest OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(DOOuterOutput_Scan_Node_Type   *)
                      malloc
                      (sizeof(DOOuterOutput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("DO  Dest OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_DO_OUTEROUTPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->pvDestHandle=pInitNode->
                                    pvDestHandle;
            pScanNode->bLastValue=FALSE;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  HINTLAMP_OUTPUT_DEST:
        {
            LampOuterOutput_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("Lamp  Dest OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(LampOuterOutput_Scan_Node_Type    *)
                      malloc
                      (sizeof(LampOuterOutput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("Lamp Dest  OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_LAMP_OUTEROUTPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->pvDestHandle=pInitNode->
                                    pvDestHandle;
            pScanNode->bLastValue=FALSE;

            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  AI_VIRTUAL_CH_DEST:
        {
            AIOuterOutput_Scan_Node_Type    * pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("VirtualAI  Dest OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(AIOuterOutput_Scan_Node_Type    *)
                      malloc
                      (sizeof(AIOuterOutput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("VirtualAI  Dest  OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            if(pInitNode->bPhyAIChFlag)
            {
                /*若是物理通道  */

                pNode->ulTuyuanType=
                    RE_FLOAT_AI_CH_OUTEROUTPUT_SCAN;

            }
            else
            {
                /* 若是逻辑通道  */

                pNode->ulTuyuanType=
                    RE_COMPLEX_AI_CH_OUTEROUTPUT_SCAN;

            }
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->AIDestChOffset=
                pInitNode->AIDestChOffset;
            pScanNode->pChartMsg=
                pInitNode->PublicElemData.elem.pchart;

            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  ANALOG_OUTPUT_DEST:
        {
            OptAOOuterOutput_Scan_Node_Type    * pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("AO  Dest OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(OptAOOuterOutput_Scan_Node_Type    *)
                      malloc
                      (sizeof(OptAOOuterOutput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("AO  Dest  OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            if(RD_AddMidSrcAo(pInitNode->strOutputDestID,pInitNode->aucSourceSignalTypeArr[0],&(pScanNode->ioOut))!=EP_SUCCESS)
            {
                LOG_Dbg_Msg("AO  Dest  OuterOutput  Tuyuan  Init failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_OPTAO_OUTEROUTPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->ioOut.ucType=0xFF;
            pScanNode->ioOut.ucAttrib=pInitNode->aucSourceSignalTypeArr[0];
            pScanNode->ioOut.pvCh=NULL;
            pScanNode->ioOut.now.fVal=0.0;/* 2008-1-24 zy merge*/

            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;

        case  AI_PLUSADT_DEST:
        {
            AIPlusCofOutput_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("AiPlusCof OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(AIPlusCofOutput_Scan_Node_Type   *)
                      malloc
                      (sizeof(AIPlusCofOutput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("AiPlusCof OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_AIPLUSADT_OUTEROUTPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->pvDestHandle=pInitNode->
                                    pvDestHandle;
            pScanNode->fLastValue=0.0;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;

        case  AI_OFFADT_DEST:
        {
            AIOffCofOutput_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("AiOffCof OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(AIOffCofOutput_Scan_Node_Type   *)
                      malloc
                      (sizeof(AIOffCofOutput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("AiOffCof OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_AIOFFADT_OUTEROUTPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->pvDestHandle=pInitNode->
                                    pvDestHandle;
            pScanNode->fLastValue=0.0;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;

        case  CELIANG_PLUSADT_DEST:
        {
            ClPlusCofOutput_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("ClPlusCof OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(ClPlusCofOutput_Scan_Node_Type   *)
                      malloc
                      (sizeof(ClPlusCofOutput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("ClPlusCof OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_CLPLUSADT_OUTEROUTPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->clNum=(int)pInitNode->AIDestChOffset;
            pScanNode->fLastValue=0.0;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;

        case  CELIANG_OFFADT_DEST:
        {
            ClOffCofOutput_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("ClOffCof OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(ClOffCofOutput_Scan_Node_Type   *)
                      malloc
                      (sizeof(ClOffCofOutput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("ClOffCof OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_CLOFFADT_OUTEROUTPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->clNum=(int)pInitNode->AIDestChOffset;
            pScanNode->fLastValue=0.0;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  DI_XIAODOU_DEST:
        {
            DIFiltTmOutput_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("DI xiaodou time  OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(DIFiltTmOutput_Scan_Node_Type   *)
                      malloc
                      (sizeof(DIFiltTmOutput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("DI xiaodou time  OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_DIFILTADT_OUTEROUTPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->pvDestHandle=pInitNode->
                                    pvDestHandle;
            pScanNode->fLastValue=0.0;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;

        case  YAOCE_OVERADT_DEST:
        {
            YcChgCofOutput_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("YcOverCof OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(YcChgCofOutput_Scan_Node_Type   *)
                      malloc
                      (sizeof(YcChgCofOutput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("YcOverCof OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_YCOVERADT_OUTEROUTPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->clNum=(int)pInitNode->AIDestChOffset;
            pScanNode->fLastValue=0.0;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;

        case  CELIANG_OVERADT_DEST:
        {
            ClChgCofOutput_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("ClOverCof OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(ClChgCofOutput_Scan_Node_Type   *)
                      malloc
                      (sizeof(ClChgCofOutput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("ClOverCof OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_CLOVERADT_OUTEROUTPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->clNum=(int)pInitNode->AIDestChOffset;
            pScanNode->fLastValue=0.0;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  PULSE_OUTPUT_DEST:
        {
            POOuterOutput_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("PO  Dest OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(POOuterOutput_Scan_Node_Type   *)
                      malloc
                      (sizeof(POOuterOutput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("PO  Dest OuterOutput  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_PO_OUTEROUTPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->pvDestHandle=pInitNode->
                                    pvDestHandle;
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
     功能:设置图元的扫描节点的某个输入的指针
*/
/****参数：
           ucInputNum,输入的序号
           pElemIO,输入IO指针
           pScanNode，扫描节点指针
*/
/*   返回值，返回成功与否*/

EP_STATUS   RE_OuterOutputTuyuanSetInputIO
(uint8_t  ucInputNum,EP_ELEM_IO *  pElemIO,
 NODE  * pScanNode)
{

    switch(pScanNode->ulTuyuanType)
    {
        case   RE_DO_OUTEROUTPUT_SCAN:
        {

            DOOuterOutput_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(DOOuterOutput_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set DO  Dest  OuterOutput Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_LAMP_OUTEROUTPUT_SCAN:
        {

            LampOuterOutput_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(LampOuterOutput_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set HintLamp  Dest  OuterOutput Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;
        }
        break;
        case   RE_FLOAT_AI_CH_OUTEROUTPUT_SCAN:
        {

            AIOuterOutput_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(AIOuterOutput_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set VirtualCh  Dest  OuterOutput Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;
        }
        break;
        case   RE_COMPLEX_AI_CH_OUTEROUTPUT_SCAN:
        {

            AIOuterOutput_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(AIOuterOutput_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set Virtual  AI Ch  Dest  OuterOutput Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;
        }
        break;
        case   RE_OPTAO_OUTEROUTPUT_SCAN:
        {

            OptAOOuterOutput_Scan_Node_Type  *pTuyuanNode;
            pTuyuanNode=( OptAOOuterOutput_Scan_Node_Type   *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set AO  Dest  OuterOutput Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;
        }
        break;

        case   RE_PO_OUTEROUTPUT_SCAN:
        {

            POOuterOutput_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(POOuterOutput_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set PO  Dest  OuterOutput Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;
        }
        break;

        case   RE_AIPLUSADT_OUTEROUTPUT_SCAN:
        {

            AIPlusCofOutput_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(AIPlusCofOutput_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set AiPlusCof OuterOutput Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_AIOFFADT_OUTEROUTPUT_SCAN:
        {

            AIOffCofOutput_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(AIOffCofOutput_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set AiOffCof OuterOutput Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_CLPLUSADT_OUTEROUTPUT_SCAN:
        {

            ClPlusCofOutput_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(ClPlusCofOutput_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set ClPlusCof OuterOutput Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_CLOFFADT_OUTEROUTPUT_SCAN:
        {

            ClOffCofOutput_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(ClOffCofOutput_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set ClOffCof OuterOutput Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_DIFILTADT_OUTEROUTPUT_SCAN:
        {

            DIFiltTmOutput_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(DIFiltTmOutput_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set DI xiaodou time OuterOutput Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_YCOVERADT_OUTEROUTPUT_SCAN:
        {

            YcChgCofOutput_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(YcChgCofOutput_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set YcOverCof OuterOutput Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_CLOVERADT_OUTEROUTPUT_SCAN:
        {

            ClChgCofOutput_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(ClChgCofOutput_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set ClOverCof OuterOutput Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        default  :

            LOG_Dbg_Msg("Set One OuterOutput  Tuyuan  InputIO  Error!\n",0,0,0,0,0,0);
            assert(FALSE);
            return  EP_SYS_ERR;
            break;

    }

    return  EP_SUCCESS;
}



/*    FloatAI输出目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_FloatAIOuterOutputTuyuanScan(NODE *pElemScanNode)
{
    AIOuterOutput_Scan_Node_Type  *  pScanNode;
    float  *pfAI;
    int  unNewAICount;

    pScanNode=(AIOuterOutput_Scan_Node_Type  * )pElemScanNode->pTuyuan;
    pfAI=pScanNode->pChartMsg->pfBase
         +pScanNode->AIDestChOffset;
    unNewAICount=(int)pScanNode->pChartMsg->unNewAi;

    if(unNewAICount==1)
    {
        RD_Wr_Lgc_Vt_AI(pfAI, 0, pScanNode->pInArr0->now.fVal);
        return;

    }
    else  if(unNewAICount==2)
    {
        RD_Wr_Lgc_Vt_AI(pfAI, 0, pScanNode->pInArr0->now.fVal);
        RD_Wr_Lgc_Vt_AI(pfAI, -1,pScanNode->pInArr0->now.fVal);

        return;

    }
    else  if(unNewAICount==3)
    {
        float  fValue;
        fValue=pScanNode->pInArr0->now.fVal;
        RD_Wr_Lgc_Vt_AI(pfAI, 0, fValue);
        RD_Wr_Lgc_Vt_AI(pfAI, -1, fValue);
        RD_Wr_Lgc_Vt_AI(pfAI, -2, fValue);

        return;


    }
    else  if(unNewAICount==4)
    {
        float  fValue;
        fValue=pScanNode->pInArr0->now.fVal;
        RD_Wr_Lgc_Vt_AI(pfAI, 0, fValue);
        RD_Wr_Lgc_Vt_AI(pfAI, -1, fValue);
        RD_Wr_Lgc_Vt_AI(pfAI, -2, fValue);
        RD_Wr_Lgc_Vt_AI(pfAI, -3, fValue);

        return;

    }
    else
    {
        int  i;
        float  fValue;
        fValue=pScanNode->pInArr0->now.fVal;
        for(i=1; i<=unNewAICount; i++)
        {
            RD_Wr_Lgc_Vt_AI(pfAI, i-unNewAICount, fValue);
        }
        return;

    }

}

/*    ComplexAI输出目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_ComplexAIOuterOutputTuyuanScan(NODE *pElemScanNode)
{

    AIOuterOutput_Scan_Node_Type  *  pScanNode;
    COMPLEX  *pxAI;
    int  unNewAICount;

    pScanNode=(AIOuterOutput_Scan_Node_Type  * )pElemScanNode->pTuyuan;
    pxAI=pScanNode->pChartMsg->pxBase
         +pScanNode->AIDestChOffset;
    unNewAICount=(int)pScanNode->pChartMsg->unNewAi;

    if(unNewAICount==1)
    {
        RD_Wr_Calc_Vt_AI(pxAI, 0, pScanNode->pInArr0->now.xVal);
        return;

    }
    else  if(unNewAICount==2)
    {
        RD_Wr_Calc_Vt_AI(pxAI, 0, pScanNode->pInArr0->now.xVal);
        RD_Wr_Calc_Vt_AI(pxAI, -1, pScanNode->pInArr0->now.xVal);

        return;

    }
    else  if(unNewAICount==3)
    {
        COMPLEX  xValue;
        xValue=pScanNode->pInArr0->now.xVal;
        RD_Wr_Calc_Vt_AI(pxAI, 0, xValue);
        RD_Wr_Calc_Vt_AI(pxAI, -1, xValue);
        RD_Wr_Calc_Vt_AI(pxAI, -2, xValue);

        return;

    }
    else  if(unNewAICount==4)
    {
        COMPLEX  xValue;
        xValue=pScanNode->pInArr0->now.xVal;
        RD_Wr_Calc_Vt_AI(pxAI, 0, xValue);
        RD_Wr_Calc_Vt_AI(pxAI, -1, xValue);
        RD_Wr_Calc_Vt_AI(pxAI, -2, xValue);
        RD_Wr_Calc_Vt_AI(pxAI, -3, xValue);

        return;

    }
    else
    {
        int  i;
        COMPLEX  xValue;
        xValue=pScanNode->pInArr0->now.xVal;
        for(i=1; i<=unNewAICount; i++)
        {
            RD_Wr_Calc_Vt_AI(pxAI, i-unNewAICount, xValue);
        }
        return;

    }
}

/* 扫描多个DO节点.
 * Para:
 *     PartGrpAttrib, 分图属性.
 * Return:
 *     NONE.
 */
void RE_MultiDOTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib)
{
    NODE **ppCurDONode;
    int i;
    DOOuterOutput_Scan_Node_Type *pScanNode;
    RD_LGC_DO_CH *plgcdo;
    BOOL bCurValue;
    int iCurValue;

    ppCurDONode = PartGrpAttrib->ppDONode;
    for (i = 0; i<PartGrpAttrib->DOScanNodeNum; i++, ppCurDONode++)
    {
        pScanNode = (DOOuterOutput_Scan_Node_Type *)(*ppCurDONode)->pTuyuan;
        plgcdo = (RD_LGC_DO_CH *)pScanNode->pvDestHandle;

        if((pScanNode->pInArr0->ucAttrib==SHORT_INT_SIGNAL)||(pScanNode->pInArr0->ucAttrib==LONG_INT_SIGNAL))
        {
            iCurValue=pScanNode->pInArr0->now.lVal;

            taskLock();

            RD_Set_DO(plgcdo, iCurValue);

            taskUnlock();
        }
        else
        {
            bCurValue = pScanNode->pInArr0->now.bVal;

            if(bCurValue&0x80)
            {
                taskLock();

                RD_Set_DO(plgcdo, bCurValue&0x7F);

                taskUnlock();
            }
            else if((bCurValue==DP_TRUE)||(bCurValue==DP_FALSE)
                    ||(bCurValue==DP_INVALID_11)||(bCurValue==DP_INVALID_00))
            {
                taskLock();

                RD_Set_DO(plgcdo, bCurValue);

                taskUnlock();
            }
            else
            {
                taskLock();
                if ((!pScanNode->bLastValue) && bCurValue)
                {
                    /* 上升沿 */
                    (plgcdo->iTripDOCnt)++;
                }
                else if (pScanNode->bLastValue && (!bCurValue))
                {
                    /* 下降沿 */
                    --(plgcdo->iTripDOCnt);
                }

                if (plgcdo->iTripDOCnt>0)
                {
                    RD_Set_DO(plgcdo, TRUE);
                }
                else
                {
                    RD_Set_DO(plgcdo, FALSE);
                }

                taskUnlock();
                pScanNode->bLastValue = bCurValue;
            }
        }
    }
}

/* 多个指示灯输出目的的外部输出图元的扫描函数.
 * Para:
 *     分图.
 * Return:
 *     NONE.
 */
void RE_MultiLampOuterOutputTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib)
{
    int i;
    NODE **ppCurLampNode;
    LampOuterOutput_Scan_Node_Type *pScanNode;
    RD_LGC_LED_CH *plgcLed;
    STATUS vxsts;
    BOOL bCurValue;

    ppCurLampNode = PartGrpAttrib->ppLampNode;
    for (i = 0; i<PartGrpAttrib->LampScanNodeNum; i++, ppCurLampNode++)
    {
        pScanNode = (LampOuterOutput_Scan_Node_Type *)((*ppCurLampNode)->pTuyuan);
        /*
        		if (pScanNode->pInArr0->now.bVal == pScanNode->bLastValue)
        		{
        			continue;
        		}
        */		bCurValue = pScanNode->pInArr0->now.bVal;
        plgcLed = (RD_LGC_LED_CH *)pScanNode->pvDestHandle;

        vxsts = taskLock();
        if ((!pScanNode->bLastValue) && bCurValue)
        {
            (plgcLed->iTripLedCnt)++;
        }
        else if (pScanNode->bLastValue && (!bCurValue))
        {
            --(plgcLed->iTripLedCnt);
        }

        if(plgcLed->iTripLedCnt>0)
        {
            RD_Set_LED(plgcLed, TRUE);
        }
        else
        {
            RD_Set_LED(plgcLed, FALSE);
        }
        vxsts = taskUnlock();

        pScanNode->bLastValue = bCurValue;
    }
}

/*    DO输出目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_DOOuterOutputTuyuanScan(NODE *pElemScanNode)
{
    DOOuterOutput_Scan_Node_Type    *  pDOTuyuan;
    RD_LGC_DO_CH *plgcdo;
    BOOL   bCurValue;
    int iCurValue;

    pDOTuyuan=(DOOuterOutput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    plgcdo=(RD_LGC_DO_CH*)pDOTuyuan->pvDestHandle;

    if((pDOTuyuan->pInArr0->ucAttrib==SHORT_INT_SIGNAL)||(pDOTuyuan->pInArr0->ucAttrib==LONG_INT_SIGNAL))
    {
        iCurValue=pDOTuyuan->pInArr0->now.lVal;

        taskLock();

        RD_Set_DO(plgcdo,iCurValue);

        taskUnlock();
    }
    else
    {
        bCurValue=pDOTuyuan->pInArr0->now.bVal;

        if(bCurValue&0x80)
        {
            taskLock();

            RD_Set_DO(plgcdo,bCurValue&0x7F);

            taskUnlock();
        }
        else if((bCurValue==DP_TRUE)||(bCurValue==DP_FALSE)
                ||(bCurValue==DP_INVALID_11)||(bCurValue==DP_INVALID_00))
        {
            taskLock();

            RD_Set_DO(plgcdo,bCurValue);

            taskUnlock();
        }
        else
        {
            taskLock();
            if((!pDOTuyuan->bLastValue)&&bCurValue)
            {
                /*上升沿  */
                (plgcdo->iTripDOCnt)++;
            }
            else if(pDOTuyuan->bLastValue&&(!bCurValue))
            {
                /*下降沿  */
                --(plgcdo->iTripDOCnt);
            }

            if(plgcdo->iTripDOCnt>0)
            {
                RD_Set_DO(plgcdo,TRUE);
            }
            else
            {
                RD_Set_DO(plgcdo,FALSE);
            }

            taskUnlock();
            pDOTuyuan->bLastValue=bCurValue;
        }
    }
}

/*    指示灯输出目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_LampOuterOutputTuyuanScan(NODE *pElemScanNode)
{

    LampOuterOutput_Scan_Node_Type   *  pLedTuyuan;
    RD_LGC_LED_CH *plgcLed;
    BOOL   bCurValue;

    pLedTuyuan=( LampOuterOutput_Scan_Node_Type  *)pElemScanNode->pTuyuan;
    bCurValue=pLedTuyuan->pInArr0->now.bVal;
    plgcLed=(RD_LGC_LED_CH*)pLedTuyuan->pvDestHandle;

    if((!pLedTuyuan->bLastValue)&&bCurValue)
    {
        (plgcLed->iTripLedCnt)++;
    }
    else if(pLedTuyuan->bLastValue&&(!bCurValue))
    {
        --(plgcLed->iTripLedCnt);
    }
    if(plgcLed->iTripLedCnt>0)
    {

        RD_Set_LED(plgcLed,TRUE);
    }
    else
    {
        RD_Set_LED(plgcLed,FALSE);
    }

    pLedTuyuan->bLastValue=bCurValue;

}

/*    光纵AO输出目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_OptAoOuterOutputTuyuanScan(NODE *pElemScanNode)
{
    OptAOOuterOutput_Scan_Node_Type  *  pScanNode;

    pScanNode=(OptAOOuterOutput_Scan_Node_Type  * )pElemScanNode->pTuyuan;
    pScanNode->ioOut.now=pScanNode->pInArr0->now;	/*AO输出需要访问数据，需要保证数据的完整性　*/
    return;
}

/*    ai对应的物理通道增益系数输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

void     RE_AIPlusCofOutputTuyuanScan(NODE *pElemScanNode)
{
    AIPlusCofOutput_Scan_Node_Type    *  pTuyuan;
    float   fCurValue;

    pTuyuan=(AIPlusCofOutput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    fCurValue=pTuyuan->pInArr0->now.fVal;

    if(fabs(pTuyuan->fLastValue - fCurValue)>FLT_PRECISION )
    {
        /*函数里面已经进行任务锁定了*/
        ModifyAiScaleCoeLgc(pTuyuan->pvDestHandle, fCurValue);
    }

    pTuyuan->fLastValue=fCurValue;
}

/*    ai对应的物理通道偏置系数输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

void     RE_AIOffCofOutputTuyuanScan(NODE *pElemScanNode)
{
    AIOffCofOutput_Scan_Node_Type    *  pTuyuan;
    float   fCurValue;

    pTuyuan=(AIOffCofOutput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    fCurValue=pTuyuan->pInArr0->now.fVal;

    if(fabs(pTuyuan->fLastValue - fCurValue)>FLT_PRECISION )
    {
        ModifyAiExcCoeLgc(pTuyuan->pvDestHandle, fCurValue);
    }

    pTuyuan->fLastValue=fCurValue;
}

/*    测量量增益系数输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

void     RE_ClPlusCofOutputTuyuanScan(NODE *pElemScanNode)
{
    ClPlusCofOutput_Scan_Node_Type    *  pTuyuan;
    float   fCurValue;
    STATUS vxsts;

    pTuyuan=(ClPlusCofOutput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    fCurValue=pTuyuan->pInArr0->now.fVal;

    if(fabs(pTuyuan->fLastValue - fCurValue)>FLT_PRECISION  )
    {
        vxsts=taskLock();
        assert(vxsts==OK);

        ME_Set_Msu_PlusCoff(pTuyuan->clNum,fCurValue);

        vxsts=taskUnlock();
        assert(vxsts==OK);
    }

    pTuyuan->fLastValue=fCurValue;
}

/*    测量量偏置系数输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

void     RE_ClOffCofOutputTuyuanScan(NODE *pElemScanNode)
{
    ClOffCofOutput_Scan_Node_Type    *  pTuyuan;
    float   fCurValue;

    pTuyuan=(ClOffCofOutput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    fCurValue=pTuyuan->pInArr0->now.fVal;

    if(fabs(pTuyuan->fLastValue - fCurValue)>FLT_PRECISION )
    {
        taskLock();
        ME_Set_Msu_OffCoff(pTuyuan->clNum,fCurValue);

        taskUnlock();
    }

    pTuyuan->fLastValue=fCurValue;
}

/*    di效抖时间输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

void     RE_DIFiltTmOutputTuyuanScan(NODE *pElemScanNode)
{
    DIFiltTmOutput_Scan_Node_Type    *  pTuyuan;
    float   fCurValue;

    pTuyuan=(DIFiltTmOutput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    fCurValue=pTuyuan->pInArr0->now.fVal;

    if(fabs(pTuyuan->fLastValue - fCurValue)>FLT_PRECISION )
    {

        ModifyDiFiltTime(pTuyuan->pvDestHandle, fCurValue);
    }

    pTuyuan->fLastValue=fCurValue;
}

/*    遥测量越限系数输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

void     RE_YcChgCofOutputTuyuanScan(NODE *pElemScanNode)
{
    YcChgCofOutput_Scan_Node_Type    *  pTuyuan;
    float   fCurValue;

    pTuyuan=(YcChgCofOutput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    fCurValue=pTuyuan->pInArr0->now.fVal;

    if(fabs(pTuyuan->fLastValue - fCurValue)>FLT_PRECISION  )
    {
        taskLock();

        VI_Set_Mea_AI_ChgCoff(pTuyuan->clNum,fCurValue);

        taskUnlock();
    }

    pTuyuan->fLastValue=fCurValue;
}

/*    测量量越限系数输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

void     RE_ClChgCofOutputTuyuanScan(NODE *pElemScanNode)
{
    ClChgCofOutput_Scan_Node_Type    *  pTuyuan;
    float   fCurValue;

    pTuyuan=(ClChgCofOutput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    fCurValue=pTuyuan->pInArr0->now.fVal;

    if(fabs(pTuyuan->fLastValue - fCurValue)>FLT_PRECISION)
    {
        taskLock();
        ME_Set_Msu_ChgCoff(pTuyuan->clNum,fCurValue);

        taskUnlock();
    }

    pTuyuan->fLastValue=fCurValue;
}

/*    PO输出目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_POOuterOutputTuyuanScan(NODE *pElemScanNode)
{
    POOuterOutput_Scan_Node_Type    *  pPOTuyuan;
    uint32_t   uiCurValue;

    pPOTuyuan=(POOuterOutput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    uiCurValue=pPOTuyuan->pInArr0->now.ulVal;

    RD_Wr_PO(pPOTuyuan->pvDestHandle,uiCurValue);
}
