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
/*       该文件定义了保护功能模块中的系统服务输出元件的代码实现                       */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*     ghx                                                                    */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                    */
/*                                                                              */
/*    Hongxia Gao       2006.11.13              创建文件1.0版本              */
/*                                                                              */
/********************************************************************************/

#include <vxWorks.h>

#include  "RE_SysServerTuyuan.h"
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

EP_STATUS   RE_SysServerTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
        TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
        NODE ** pRtElemInitNodePointer)
{
    NODE  *pNode;
    SysServer_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    int  i;
    unsigned  char   *   pucCurInSignalType;
    unsigned  short   *  punCurInSourceSeqNo;
    unsigned  char   *   pucInSourceOutputNo;

    BOOL  bOtherInitSuccess;
    unsigned  char   ucInputCount;
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
        LOG_Dbg_Msg("SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    pInitNode=(SysServer_Init_Node_Type    *)malloc(sizeof(SysServer_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_SYSSERVOUT;
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
    /* 读取系统服务类型类型*/
    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&(pInitNode->ucServerType));
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

#if 0
    /* 读取22保留字节  */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,22);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
#endif

#if 1
    /* 读取16保留字节  */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,16);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    /*  读取输出信号目的地逻辑标识 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 ( fp,pInitNode->strOutputDestID1,&ulIDStrLen);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    /* 读取4保留字节  */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,4);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

    /*  读取输出信号目的地逻辑标识 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 ( fp,pInitNode->strOutputDestID2,&ulIDStrLen);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }

#endif

    /* 读取图元输入个数 */
    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&ucInputCount);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    /*赋值给节点  */
    pInitNode->PublicElemData.elem.unInNum=ucInputCount;
    /*循环读取每个输入信息*/
    pucCurInSignalType=pInitNode->aucSourceSignalTypeArr;
    punCurInSourceSeqNo=pInitNode->aunInSourceSeqNoArr;
    pucInSourceOutputNo=pInitNode->aucInSourceOutputNumArr;

    for(i=0; i<ucInputCount; i++)
    {
        /*读取输入信号类型  */
        if(i==0)
        {
            bReadSuccess=ReadReserveBytesFromResloveSeqFile
                         (fp,1);
            if(!bReadSuccess)
            {
                return   EP_SYS_ERR;
            }
            *pucCurInSignalType=0x04;
        }
        else
        {
            bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                         (fp,pucCurInSignalType);
            if(!bReadSuccess)
            {
                return   EP_SYS_ERR;
            }
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

    /* 设置该扫描节点的输出个数 */
    pInitNode->PublicElemData.elem.ucOutNum=0;

    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_SysServerTuyuanReadFileOtherInit
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

EP_STATUS   RE_SysServerTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                       LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    SysServer_Init_Node_Type    *pInitNode;
    NODE  *  pCurSourceNode;
    int  i;
    uint8_t  * pucCurInSignalType;
    uint16_t   * punCurInSignalSourceSeqNo;
    uint8_t   *  pucCurInSignalSourceOutNo;
    EP_ELEM_IO *  pInElem;

    EP_STATUS  SetCurOutAttribResult;

    uint8_t   ucCurInSignalType;


    pInitNode=(SysServer_Init_Node_Type    *)pElemInitNode->pTuyuan;
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
            LOG_Dbg_Msg("error,Get  SysServer  Tuyuan  Input Source  Init  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }

        /* 获得并设置当前输入源的指针 */

        pInElem=(*  RE_GetScanNodeGetOutIOFunc(pCurSourceNode))
                ((*pucCurInSignalSourceOutNo),pCurSourceNode);


        /*  确保该图元输入在来源图元的输出号没有出界 */
        if(pInElem==NULL)
        {

            LOG_Dbg_Msg("error,Get  SysServer  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }
        /*  设置该IO指针到扫描节点相应的输入  */

        SetCurOutAttribResult=RE_SysServerTuyuanSetInputIO
                              (i,pInElem,pElemScanNode);
        if(SetCurOutAttribResult!=EP_SUCCESS)
        {
            LOG_Dbg_Msg("error,Set  SysServer  Tuyuan  input  info  failure!\n",0,0,0,0,0,0);

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;

        }

        /*比较信号类型是否匹配  */

        ucCurInSignalType=pInElem->ucAttrib;

        if(ucCurInSignalType!=(*pucCurInSignalType))
        {
            LOG_Dbg_Msg("error,SysServer Tuyuan One  Input  Signal Type  can't  Match!\n",0,0,0,0,0,0);


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
BOOL      RE_SysServerTuyuanReadFileOtherInit
(SysServer_Init_Node_Type * pElemInitNodePointer)
{

    pElemInitNodePointer->PublicElemData.elem.ucType=RE_SYSSERVOUT;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_SysServerTuyuanScanInit;

#if 1
    switch(pElemInitNodePointer->ucServerType)
    {
        case  YABAN_TT_DEST:
        {
            /* 压板退出 */
            if((SCI_Init_Get_Yaban_Info(pElemInitNodePointer->strOutputDestID1, &pElemInitNodePointer->iYabanNum)) != EP_SUCCESS)
            {
                assert(FALSE);  /* 若非以上类型,则告警   */
                return  EP_BAD_DATA;
            }
        }
        break;

        case  SETAUTOSET_DEST:
        {
            /* 自动整定定值，获取定值目的指针 */
            if(bulRelayTaskHasAutoSet_g==FALSE)
                bulRelayTaskHasAutoSet_g=TRUE;
            if((SCI_Init_Get_Set_Dest(pElemInitNodePointer->strOutputDestID1, &pElemInitNodePointer->pSetDest)) != EP_SUCCESS)
            {
                LOG_Dbg_Msg("在参数定值中找不到自动整定定值\'%s\'.\n", (int)pElemInitNodePointer->strOutputDestID1, 0, 0, 0, 0, 0);
                assert(FALSE);  /* 若非以上类型,则告警   */

                return  EP_BAD_DATA;
            }
        }
        break;

        case DBYX_DEST:
        {
            /* 设置遥信 */
            if(SCI_Init_Add_TimeSet_New_Yaoxin_Signal(pElemInitNodePointer->strOutputDestID1, &pElemInitNodePointer->iCh, &pElemInitNodePointer->ucType) != EP_SUCCESS)
            {
                LOG_Dbg_Msg("在软件配置中找不到该遥信\'%s\'!\n", (int)pElemInitNodePointer->strOutputDestID1, 0, 0, 0, 0, 0);
                assert(FALSE); 		/* 若在软件配置中找不到该遥信则报错 */
                return EP_BAD_DATA;
            }
        }
        break;

        default:
            break;
    }
#endif

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

EP_STATUS   RE_SysServerCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{

    NODE  *pNode;
    SysServer_Init_Node_Type    *pInitNode;
    pInitNode=(SysServer_Init_Node_Type    *)pElemInitNode->pTuyuan;

    /*  设置图元类型*/
    switch(pInitNode->ucServerType)
    {
        case  SETTING_SWITCH_DEST:
        {
            SettingSwitch_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("SettingSwitch SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(SettingSwitch_Scan_Node_Type   *)
                      malloc
                      (sizeof(SettingSwitch_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("SettingSwitch SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_SETTINGSWITCH_SYSSERV_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->bLastValue=FALSE;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  REVERT_DEST:
        {
            Revert_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("Revert SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(Revert_Scan_Node_Type   *)
                      malloc
                      (sizeof(Revert_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("Revert SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_REVERT_SYSSERV_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->bLastValue=FALSE;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  SWITCH_TO_FAR_DEST:
        {
            SwitchFar_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("SwitchFar SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(SwitchFar_Scan_Node_Type   *)
                      malloc
                      (sizeof(SwitchFar_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("SwitchFar SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_SWITCHFAR_SYSSERV_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  SWITCH_TO_EXAM_DEST:
        {
            SwitchExam_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("SwitchExam SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(SwitchExam_Scan_Node_Type   *)
                      malloc
                      (sizeof(SwitchExam_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("SwitchExam SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_SWITCHEXAM_SYSSERV_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  THREEU0_WARN_DEST:
        {
            ThreeU0Warn_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("ThreeU0Warn SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(ThreeU0Warn_Scan_Node_Type   *)
                      malloc
                      (sizeof(ThreeU0Warn_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("ThreeU0Warn SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_THREEU0WARN_SYSSERV_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case  PLUSADTOVER_DEST:
        {
            PlusAdtSysServer_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("PlusAdt SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(PlusAdtSysServer_Scan_Node_Type   *)
                      malloc
                      (sizeof(PlusAdtSysServer_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("PlusAdt SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_PLUSADTOVER_SYSSERV_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            pScanNode->bCurVaueLst = TRUE;
            return  EP_SUCCESS;
        }
        break;
        case  OFFADTOVER_DEST:
        {
            OffAdtSysServer_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("OffAdt SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(OffAdtSysServer_Scan_Node_Type   *)
                      malloc
                      (sizeof(OffAdtSysServer_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("OffAdt SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }

            pNode->ulTuyuanType=RE_OFFADTOVER_SYSSERV_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            pScanNode->bCurVaueLst = TRUE;
            return  EP_SUCCESS;
        }
        break;
        case  YABAN_TT_DEST:
        {
            YabanTT_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("YaBanTT SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(YabanTT_Scan_Node_Type   *)
                      malloc
                      (sizeof(YabanTT_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("YaBanTT SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode->iYabanNum=pInitNode->iYabanNum;		/* 压板号赋值 */
            pNode->ulTuyuanType=RE_YABANTT_SYSSERV_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            pScanNode->bCurVaueLst = FALSE;
            return  EP_SUCCESS;
        }
        break;

        case  SETAUTOSET_DEST:		/* 自动整定定值，建立扫描节点，完成部分初始化 */
        {
            STAUTOST_Scan_Node_Type   *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("SetAutoSet SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(STAUTOST_Scan_Node_Type   *)
                      malloc
                      (sizeof(STAUTOST_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("SetAutoSet SysServer  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode->pSetDest=pInitNode->pSetDest;		/* 参数定值指针 */
            pScanNode->bLastVal=FALSE;                     /*初始为低*/
            pNode->ulTuyuanType=RE_SETAUTOSET_SYSSERV_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;

        case  DBYX_DEST:			/* 遥信 */
        {
            DBYX_Scan_Node_Type *pScanNode;
            pNode=(NODE *)malloc(sizeof(NODE));
            if(pNode == NULL)
            {
                LOG_Dbg_Msg("DBYX SysServer Tuyuan malloc failiure!\n", 0, 0, 0, 0, 0, 0);
                return EP_BUF_ERR;
            }

            pScanNode=(DBYX_Scan_Node_Type *)
                      malloc(sizeof(DBYX_Scan_Node_Type));
            if(pScanNode == NULL)
            {
                LOG_Dbg_Msg("DBYX SysServer Tuyuan malloc failiure!\n", 0, 0, 0, 0, 0, 0);
                return EP_BUF_ERR;
            }

            pScanNode->iCh=pInitNode->iCh;				/* 遥信点号 */
            pScanNode->ucType=pInitNode->ucType;				/* Type */
            pScanNode->bCurValLst = FALSE;
            pNode->ulTuyuanType=RE_DBYX_SYSSERV_SCAN;
            pNode->pTuyuan=(void *)pScanNode;

            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE *)pNode;
            return EP_SUCCESS;
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

EP_STATUS   RE_SysServerTuyuanSetInputIO
(uint8_t  ucInputNum,EP_ELEM_IO *  pElemIO,
 NODE  * pScanNode)
{

    switch(pScanNode->ulTuyuanType)
    {
        case   RE_SETTINGSWITCH_SYSSERV_SCAN:
        {

            SettingSwitch_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(SettingSwitch_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=2)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set SettingSwitch SysServer Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->apInArr[ucInputNum]=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_REVERT_SYSSERV_SCAN:
        {

            Revert_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(Revert_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set Revert SysServer Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_SWITCHFAR_SYSSERV_SCAN:
        {

            SwitchFar_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(SwitchFar_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set SwitchFar SysServer Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_SWITCHEXAM_SYSSERV_SCAN:
        {

            SwitchExam_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(SwitchExam_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set SwitchExam SysServer Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_THREEU0WARN_SYSSERV_SCAN:
        {

            ThreeU0Warn_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(ThreeU0Warn_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set ThreeU0Warn SysServer Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_PLUSADTOVER_SYSSERV_SCAN:
        {

            PlusAdtSysServer_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(PlusAdtSysServer_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=3)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("PlusAdt SysServer Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->apInArr[ucInputNum]=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_OFFADTOVER_SYSSERV_SCAN:
        {

            OffAdtSysServer_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(OffAdtSysServer_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=3)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("OffAdt SysServer Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->apInArr[ucInputNum]=pElemIO;
            return   EP_SUCCESS;

        }
        break;
        case   RE_YABANTT_SYSSERV_SCAN:
        {

            YabanTT_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(YabanTT_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set YaBanTT SysServer Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->pInArr0=pElemIO;
            return   EP_SUCCESS;

        }
        break;

        case   RE_SETAUTOSET_SYSSERV_SCAN:		/* 给定自动整定图元的输入指针，指向输出源 */
        {

            STAUTOST_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(STAUTOST_Scan_Node_Type    *)pScanNode->pTuyuan;

            if(ucInputNum >= 2)			/* 设定该图元输入两个参数 */
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Set SetAutoSet SysServer Tuyuan  InputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  EP_SYS_ERR;

            }
            pTuyuanNode->apInArr[ucInputNum]=pElemIO;
            return   EP_SUCCESS;

        }
        break;


        case RE_DBYX_SYSSERV_SCAN:	/* 遥信 */
        {
            DBYX_Scan_Node_Type *pTuyuanNode;
            pTuyuanNode=(DBYX_Scan_Node_Type *)pScanNode->pTuyuan;

            if(ucInputNum >= 4)
            {
                /* 若越界，则出错  */
                LOG_Dbg_Msg("Set DBYX SysServer Tuyuan InputIO Beyond Bound!\n", 0, 0, 0, 0, 0, 0);
                assert(FALSE);
                return EP_SYS_ERR;
            }

            pTuyuanNode->apInArr[ucInputNum]=pElemIO;
            return EP_SUCCESS;

        }
        break;

        default  :

            LOG_Dbg_Msg("Set One SysServer  Tuyuan  InputIO  Error!\n",0,0,0,0,0,0);
            assert(FALSE);
            return  EP_SYS_ERR;
            break;

    }

    return  EP_SUCCESS;
}



/*    定值切换系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
void     RE_SettingSwitchTuyuanScan(NODE *pElemScanNode)
{
    SettingSwitch_Scan_Node_Type *pTuyuan;
    BOOL bCurValue;
    int32_t nSettingArea;

    pTuyuan=(SettingSwitch_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    bCurValue=pTuyuan->apInArr[0]->now.bVal;
    nSettingArea=pTuyuan->apInArr[1]->now.lVal;
    if((!pTuyuan->bLastValue)&&bCurValue)
    {
        /*上升沿  */
#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
        SC_Set_Next_Work_Area(nSettingArea);
#endif

#if defined(EDP_01_02_BUILD)		/* 定值区切换由CPU端处理 */
        SC_Set_Work_Area_LogicScan(nSettingArea);
#endif
    }

    pTuyuan->bLastValue=bCurValue;
}

/*    复归系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
void     RE_RevertTuyuanScan(NODE *pElemScanNode)
{
    Revert_Scan_Node_Type    *  pTuyuan;
    BOOL   bCurValue;

    pTuyuan=(Revert_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    bCurValue=pTuyuan->pInArr0->now.bVal;
    if((!pTuyuan->bLastValue)&&bCurValue)
    {
        /*上升沿  */
        EP_Set_ReSet_Flag();
    }

    pTuyuan->bLastValue=bCurValue;
}

/*    切换到远方态系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
void     RE_SwitchFarTuyuanScan(NODE *pElemScanNode)
{
    SwitchFar_Scan_Node_Type    *  pTuyuan;
    BOOL   bCurValue;

    pTuyuan=(SwitchFar_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    bCurValue=pTuyuan->pInArr0->now.bVal;
    if(bCurValue &&!(EP_Get_Sts_Bit()&ON_FAR_STATE))
    {
        /*变成高电平*/
        EP_Set_Sts_Bit(ON_FAR_STATE);
    }
    else if((!bCurValue) && (EP_Get_Sts_Bit()&ON_FAR_STATE))
    {
        /*变成低电平*/
        EP_Clr_Sts_Bit(ON_FAR_STATE);
    }
}


/*    切换到检修态系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
void     RE_SwitchExamTuyuanScan(NODE *pElemScanNode)
{
    SwitchExam_Scan_Node_Type    *  pTuyuan;
    BOOL   bCurValue;

    pTuyuan=(SwitchExam_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    bCurValue=pTuyuan->pInArr0->now.bVal;
    if(bCurValue &&!(EP_Get_Sts_Bit()&ON_EXAM_STATE))
    {
        /*变成高电平*/
        EP_Set_Sts_Bit(ON_EXAM_STATE);
    }
    else if((!bCurValue) && (EP_Get_Sts_Bit()&ON_EXAM_STATE))
    {
        /*变成低电平*/
        EP_Clr_Sts_Bit(ON_EXAM_STATE);
    }
}

/*    3U0越限告警系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
void     RE_ThreeU0WarnTuyuanScan(NODE *pElemScanNode)
{
    ThreeU0Warn_Scan_Node_Type    *  pTuyuan;
    BOOL   bCurValue;

    pTuyuan=(ThreeU0Warn_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    bCurValue=pTuyuan->pInArr0->now.bVal;
    if(bCurValue &&!(EP_Get_Sts_Bit()&THREEUO_WARN_STATE))
    {
        /*变成高电平*/
        EP_Set_Sts_Bit(THREEUO_WARN_STATE);
    }
    else if((!bCurValue) && (EP_Get_Sts_Bit()&THREEUO_WARN_STATE))
    {
        /*变成低电平*/
        EP_Clr_Sts_Bit(THREEUO_WARN_STATE);
    }
}

/*    增益校准结束系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
void     RE_PlusAdtOverTuyuanScan(NODE *pElemScanNode)
{
    PlusAdtSysServer_Scan_Node_Type    *  pTuyuan;
    BOOL   bCurValue;

    pTuyuan=(PlusAdtSysServer_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    bCurValue=pTuyuan->apInArr[0]->now.bVal;

    if(bCurValue && (!pTuyuan->bCurVaueLst))
    {
        VI_Come_Over_Plus_Adjust(&pTuyuan->apInArr[1]->now.ulVal, &pTuyuan->apInArr[2]->now.ulVal);
    }

    pTuyuan->bCurVaueLst =	bCurValue;			/* 下次使用 */
}

/*    偏置校准结束系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
void     RE_OffAdtOverTuyuanScan(NODE *pElemScanNode)
{
    OffAdtSysServer_Scan_Node_Type    *  pTuyuan;
    BOOL   bCurValue;

    pTuyuan=(OffAdtSysServer_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    bCurValue=pTuyuan->apInArr[0]->now.bVal;

    if(bCurValue && (!pTuyuan->bCurVaueLst))
    {
        VI_Come_Over_Off_Adjust(&pTuyuan->apInArr[1]->now.ulVal, &pTuyuan->apInArr[2]->now.ulVal);
    }
    pTuyuan->bCurVaueLst =	bCurValue;			/* 下次使用 */
}

/*    压板投退系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
void     RE_YaBanTTTuyuanScan(NODE *pElemScanNode)
{
    YabanTT_Scan_Node_Type    *  pTuyuan;
    BOOL   bCurValue;

    pTuyuan=(YabanTT_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    bCurValue=pTuyuan->pInArr0->now.bVal;

    if(bCurValue && (!pTuyuan->bCurVaueLst))
    {
        /* 上升沿触发，压板投退 */
        EP_Set_YBTT_Flag(pTuyuan->iYabanNum);
    }

    pTuyuan->bCurVaueLst =	bCurValue;			/* 下次使用 */
}

/*    自动整定定值系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
void RE_SetAutoSetTuyuanScan(NODE *pElemScanNode)
{
    STAUTOST_Scan_Node_Type *pTuyuan;

    BOOL bCurValue;

    pTuyuan=(STAUTOST_Scan_Node_Type *)pElemScanNode->pTuyuan;
    bCurValue = pTuyuan->apInArr[0]->now.bVal;
    if(bCurValue&&!pTuyuan->bLastVal)
    {
        pTuyuan->pSetDest->ulVal=pTuyuan->apInArr[1]->now.ulVal;
        RE_IncSetAtSetCnt();
    }
    pTuyuan->bLastVal=bCurValue;
}

/***********************************************************************
* RE_DBYXTuyuanScan - 遥信系统服务图元的扫描函数
*
* RETURNS: 无
*
*/
void RE_DBYXTuyuanScan(
    NODE *pElemScanNode			/* 操纵的扫描数据节点指针 */
)
{
    DBYX_Scan_Node_Type *pTuyuan;
    BOOL bCurVal;
    BOOL lCurVal;
    uint32_t ulInfoIn;

    pTuyuan=(DBYX_Scan_Node_Type *)pElemScanNode->pTuyuan;
    bCurVal=pTuyuan->apInArr[0]->now.bVal;
    lCurVal=pTuyuan->apInArr[1]->now.lVal;
    ulInfoIn = pTuyuan->apInArr[3]->now.ulVal;

    if((bCurVal && (!pTuyuan->bCurValLst)) || (!bCurVal && pTuyuan->bCurValLst))
    {
        /* 上升沿或下降沿，并且不是第一次 */
        /* LOG_Dbg_Msg("RE_DBYXTuyuanScan 上升沿 %d %d!\n", pTuyuan->ucType, pTuyuan->iCh, 0, 0, 0, 0); */
        if(pTuyuan->iCh >= 0)
        {
            /* 允许触发SOE */
            VI_New_TimeSet_SOE(pTuyuan->iCh,
                               lCurVal,
                               pTuyuan->apInArr[2]->now.ulVal,
                               (BOOL)(ulInfoIn&0x01),
                               (uint16_t)((ulInfoIn & 0xFFFF0000) >> 16));
        }
    }

    pTuyuan->bCurValLst=bCurVal;			/* 保存上一次 */
}
