/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_SettingTuyuan.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的定植输入元件的代码文件                        */
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
/*         张云       2005.10.25              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/


#include <vxWorks.h>

#include  "RE_SettingTuyuan.h"


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

EP_STATUS   RE_SettingTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
        TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
        NODE ** pRtElemInitNodePointer)
{

    NODE  *pNode;
    Setting_Init_Node_Type    *pInitNode;
    BOOL   bReadSuccess;
    uint8_t   ucSignalType;	/* 信号输入类型 */
    int  i;
    BOOL  bOtherInitSuccess;

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
        LOG_Dbg_Msg("Setting  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;
    }
    pInitNode=(Setting_Init_Node_Type    *)malloc(sizeof(Setting_Init_Node_Type));
    if(pInitNode==NULL)
    {
        LOG_Dbg_Msg("Setting  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
        return  EP_BUF_ERR;

    }
    /* 将节点的信息指针给节点 */
    pNode->ulTuyuanType=RE_SETTING;
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
    /*赋值输入个数给节点  */
    pInitNode->PublicElemData.elem.unInNum=0;

    /* 读取信号类型  */

    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&ucSignalType);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    /* 读取信号来源类型 */

    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&(pInitNode->ucSignalSourceType));
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }
    /* 读取4字节保留 */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile
                 (fp,4);
    if(!bReadSuccess)
    {
        return   EP_SYS_ERR;
    }


    /*赋值输出给节点 ， */

    /* 该扫描节点的输出个数 */
    pInitNode->PublicElemData.elem.ucOutNum=1;

    /*循环读取所有输出信息   */
    pCurElemOutput=pInitNode->PublicElemData.elem.aioOut;

    for(i=0; i<1; i++)
    {

        /* 设置输出信号类型 */
        pCurElemOutput->ucAttrib=ucSignalType;

        /* 根据信号来源类型读取和初始化信息 */
        switch(pInitNode->ucSignalSourceType)
        {
            case  ANNLOG_CONSTVALUE_SOURCE :/* 若是常数来源 */
                bReadSuccess=
                    RE_SettingTuyuanGetConstValueSourceFileReadInit(
                        fp,pInitNode);
                if(!bReadSuccess)
                {
                    return   EP_SYS_ERR;
                }

                break;
            case  DINGZHI_INPUT_SOURCE:/* 若是定植来源 */

                bReadSuccess=
                    RE_SettingTuyuanGetSettingSourceFileReadInit(
                        fp,pInitNode);
                if(!bReadSuccess)
                {
                    return   EP_SYS_ERR;
                }
                break;
            default:

                LOG_Dbg_Msg("Setting Tuyuan Input Source  Type  isn't  Expected!\n",0,0,0,0,0,0);
                return  EP_SYS_ERR;
                break;

        }

        /* 为了提高效率,不用数组 */
        pCurElemOutput++;

    }/*所有输出循环处理结束   */


    /* 进行节点其他未获得初值的信息,赋初值 */
    bOtherInitSuccess=RE_SettingTuyuanReadFileOtherInit
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

EP_STATUS   RE_SettingTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                     LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag)
{

    return  EP_SUCCESS;

}









/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_SettingTuyuanReadFileOtherInit
(Setting_Init_Node_Type * pElemInitNodePointer)
{
    pElemInitNodePointer->PublicElemData.elem.ucType=RE_SETTING;
    /* 设置包裹后的初始化函数 */
    pElemInitNodePointer->PublicElemData.pfInitScanFunc=
        RE_SettingTuyuanScanInit;

    /* 设置获得IO输出的函数 */
    pElemInitNodePointer->PublicElemData.pfGetScanNodeOutIOFunc
        =RE_SettingTuyuanGetOutIO;

    return   TRUE;

}





/*　当外部输入来源为常数时,读取常数,并进行相应的初始化
    参数   fp  ,文件当前指针
           pElemInitNode,初始化节点指针
*/


BOOL   RE_SettingTuyuanGetConstValueSourceFileReadInit(
    FILE  *fp,
    Setting_Init_Node_Type *  pElemInitNode
)
{
    BOOL  bReadSuccess;
    EP_ELEM_IO  *  pCurElemOutput;
    unsigned  char   ucConstValue;
    unsigned  short    unConstValue;
    unsigned  long   ulConstValue;
    float   fConstValue;


    pCurElemOutput=pElemInitNode->PublicElemData.elem.aioOut;

    /*根据信号类型来读取相应数据,并将数据赋给输出 */
    if((pCurElemOutput->ucAttrib==XIANGBIE_SIGAL)
            ||(pCurElemOutput->ucAttrib==CONTROL_WORD_SIGNAL))
    {
        /* 若信号类型是相别或控制字 */
        bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&ucConstValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /* 读取7字节保留 */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,7);
        if(!bReadSuccess)
        {
            return   FALSE;
        }

        /*  赋给输出,并进行输出初始化*/
        pCurElemOutput->now.ulVal=(unsigned  long)ucConstValue;
        pCurElemOutput->ucType=0xFF;
        pCurElemOutput->pvCh=NULL;
        pElemInitNode->ucSignalValueType=UNSIGNED_32INT_VALUE_TYPE;

    }
    else  if(pCurElemOutput->ucAttrib==SHORT_INT_SIGNAL)
    {

        unsigned long  ulValue;
        bReadSuccess=ReadUnsignedShortFromResloveSeqFile
                     (fp,&unConstValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /* 读取6字节保留 */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,6);
        if(!bReadSuccess)
        {
            return   FALSE;
        }

        /*  赋给输出,并进行输出初始化*/
        ulValue=(unsigned  long )unConstValue;
        pCurElemOutput->now.lVal=(long)ulValue;
        pCurElemOutput->ucType=0xFF;
        pCurElemOutput->pvCh=NULL;
        pElemInitNode->ucSignalValueType=SIGNED_32INT_VALUE_TYPE;
    }
    else  if(pCurElemOutput->ucAttrib==LONG_INT_SIGNAL)
    {
        bReadSuccess=ReadUnsignedLongFromResloveSeqFile
                     (fp,&ulConstValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /* 读取4字节保留 */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /*  赋给输出,并进行输出初始化*/
        pCurElemOutput->now.lVal=(long)ulConstValue;
        pCurElemOutput->ucType=0xFF;
        pCurElemOutput->pvCh=NULL;
        pElemInitNode->ucSignalValueType=SIGNED_32INT_VALUE_TYPE;
    }
    else  if(pCurElemOutput->ucAttrib==HEX_MODE_WORD_SIGNAL)
    {
        /* 若是32位16进制方式字 */
        bReadSuccess=ReadUnsignedLongFromResloveSeqFile
                     (fp,&ulConstValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /* 读取4字节保留 */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /*  赋给输出,并进行输出初始化*/
        pCurElemOutput->now.ulVal=(unsigned  long)ulConstValue;
        pCurElemOutput->ucType=0xFF;
        pCurElemOutput->pvCh=NULL;
        pElemInitNode->ucSignalValueType=UNSIGNED_32INT_VALUE_TYPE;
    }
    else  if((pCurElemOutput->ucAttrib==COMPLEX_FORM_DIANLIU_SIGNAL)
             ||(pCurElemOutput->ucAttrib==VALUE_ANGLE_FORM_DIANLIU_SIGNAL)
             ||(pCurElemOutput->ucAttrib==COMPLEX_FORM_DIANLIU_SIGNAL_KA)
             ||(pCurElemOutput->ucAttrib==VALUE_ANGLE_FORM_DIANLIU_SIGNAL_KA)
             ||(pCurElemOutput->ucAttrib==COMPLEX_FORM_DIANYA_SIGNAL)
             ||(pCurElemOutput->ucAttrib==VALUE_ANGLE_FORM_DIANYA_SIGNAL)
             ||(pCurElemOutput->ucAttrib==COMPLEX_FORM_DIANYA_SIGNAL_KV)
             ||(pCurElemOutput->ucAttrib==VALUE_ANGLE_FORM_DIANYA_SIGNAL_KV)
             ||(pCurElemOutput->ucAttrib==COMPLEX_FORM_ZUKANG_SIGNAL)
             ||(pCurElemOutput->ucAttrib==VALUE_ANGLE_FORM_ZUKANG_SIGNAL)
             ||(pCurElemOutput->ucAttrib==COMPLEX_FORM_ZUKANG_SIGNAL_KO)
             ||(pCurElemOutput->ucAttrib==VALUE_ANGLE_FORM_ZUKANG_SIGNAL_KO))
    {
        /*读取实部  */

        float   fVirValue;
        bReadSuccess=ReadFloatFromResloveSeqFileInMotorolaType
                     (fp,&fConstValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /* 读取虚部 */
        bReadSuccess=ReadFloatFromResloveSeqFileInMotorolaType
                     (fp,&fVirValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /*  赋给输出,并进行输出初始化*/

        pCurElemOutput->now.xVal=fConstValue+fVirValue*1i;

        pCurElemOutput->ucType=0xFF;
        pCurElemOutput->pvCh=NULL;
        pElemInitNode->ucSignalValueType=COMPLEX_VALUE_TYPE;

    }
    else  if(pCurElemOutput->ucAttrib==LOGIC_SIGNAL
             ||pCurElemOutput->ucAttrib==VAR_STRING)
    {
        /* 若是逻辑信号或字符串，则出错 2008-7-19日张云 */
        assert(FALSE);
        return   FALSE;

    }
    else
    {
        /*若是其他，则作为实数方式处理  */
        bReadSuccess=ReadFloatFromResloveSeqFileInMotorolaType
                     (fp,&fConstValue);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /* 读取4字节保留 */
        bReadSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
        if(!bReadSuccess)
        {
            return   FALSE;
        }
        /*  赋给输出,并进行输出初始化*/
        pCurElemOutput->now.fVal=fConstValue;
        pCurElemOutput->ucType=0xFF;
        pCurElemOutput->pvCh=NULL;
        pElemInitNode->ucSignalValueType=FLOAT_VALUE_TYPE;
    }

    return  TRUE;
}



/*　当外部输入来源为定植时,读取逻辑标识,并进行相应的初始化
    参数   fp  ,文件当前指针
           pElemInitNode,初始化节点指针
*/
BOOL   RE_SettingTuyuanGetSettingSourceFileReadInit(
    FILE  *fp,
    Setting_Init_Node_Type *  pElemInitNode
)
{

    char   strRelayFuncName1[300];/* 供返回该定植所配置的
                    属于保护功能名属性,可以和当前所在保护功能比较,
                    进行纠错,目前该功能未实现,将来可实现 */
    BOOL  bReadSuccess;
    EP_ELEM_IO  *  pCurElemOutput;
    unsigned  long  ulStrLen;
    EP_STATUS   OpeResult;

    SCI_SIGNAL_VALUE_TYPE  SettingValue;
    BOOL   bSettingTypeIsExpected;
    unsigned  char   ucSettingValueType;

    pCurElemOutput=pElemInitNode->PublicElemData.elem.aioOut;
    /* 获得定植逻辑标识 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 (fp,pElemInitNode->strInputSourceID,&ulStrLen);
    if(!bReadSuccess)
    {
        return   FALSE;
    }

    pElemInitNode->InputSourceSettingInfo.
    strRelayFuncName=strRelayFuncName1;

    OpeResult=SCI_Init_Get_Setting_Info(
                  pElemInitNode->strInputSourceID,
                  &(pElemInitNode->InputSourceSettingInfo)
              );

    if(OpeResult!=EP_SUCCESS)
    {
        /*  若不成功,则失败*/

        LOG_Dbg_Msg("Get Settings  Tuyuan  \'%s\'   Setting  Info  Error!\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得定值输入图元\'%s\'的定值信息错误\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp  rslv err:get  setting input element id \'%s\' info err\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        assert(FALSE);  /* 若非以上类型,则告警   */
        return  FALSE;
    }

    /*根据该定植是一般定植,还是内部定植,获得定植当前值  */
    if(pElemInitNode->InputSourceSettingInfo.ucType==1)
    {
        /*若该定植是内部定植,则访问该内部定植  */

        OpeResult=SCI_Get_Inner_Setting(
                      pElemInitNode->InputSourceSettingInfo.nNumInPage,
                      &SettingValue);

        if(OpeResult!=EP_SUCCESS)
        {
            /*  若不成功,则失败*/

            LOG_Dbg_Msg("Get Settings  Tuyuan  \'%s\'  Inner  Setting  Value  Error By  Info !\n"
                        ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得定值输入图元\'%s\'的内部定值错误\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }

            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp  rslv err :get setting input element id \'%s\' internal  setting err\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  FALSE;
        }

        /*  确保返回的定植的单位同设置的单位类型一致,否则出错*/

        if(SettingValue.ucAttrib!=(pCurElemOutput->ucAttrib))
        {
            LOG_Dbg_Msg("Get Settings  Tuyuan  \'%s\'  Inner  Setting  Value  Signal  Type  isn't  Match !\n"
                        ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得定值输入图元\'%s\'的内部定值的信号类型不对\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp  rslv err: get setting input  element id \'%s\' internal  setting signal type err\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  FALSE;

        }
    }
    else if(pElemInitNode->InputSourceSettingInfo.ucType==0)
    {
        /*若该定植是外部定植,则访问该外部定植  */

        OpeResult=SCI_Get_General_Setting(
                      pElemInitNode->InputSourceSettingInfo.cPageNum,
                      pElemInitNode->InputSourceSettingInfo.nNumInPage,
                      &SettingValue);

        if(OpeResult!=EP_SUCCESS)
        {
            /*  若不成功,则失败*/

            LOG_Dbg_Msg("Get Settings  Tuyuan  \'%s\'  General  Setting  Value  Error By  Info !\n"
                        ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得定值输入图元\'%s\'的一般定值的数据错误\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get setting input  element id \'%s\' general setting data err\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }

            assert(FALSE);  /* 若非以上类型,则告警   */
            return  FALSE;
        }
        /*  确保返回的定植的单位同设置的单位类型一致,否则出错*/

        if(SettingValue.ucAttrib!=(pCurElemOutput->ucAttrib))
        {
            LOG_Dbg_Msg("Get Settings  Tuyuan  \'%s\'  General  Setting  Value  Signal  Type  isn't  Match  !\n"
                        ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得定值输入图元\'%s\'的一般定值的信号类型错误\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic  grp rslv err:get setting  element id \'%s\' general setting signal type err\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  FALSE;

        }
    }
    else
    {
        /*若该定植是测控定植,则访问该测控定植  */
        /* 2008-7-25日 对EDP01开放测控定值功能 张云 */
        OpeResult=SCI_Get_CK_Setting(
                      pElemInitNode->InputSourceSettingInfo.nNumInPage,
                      &SettingValue);


        if(OpeResult!=EP_SUCCESS)
        {
            /*  若不成功,则失败*/

            LOG_Dbg_Msg("Get Settings  Tuyuan  \'%s\'  CK  Setting  Value  Error By  Info !\n"
                        ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得定值输入图元\'%s\'的测控定值错误\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp  rslv err: get setting input  element id \'%s\' measurement setting err\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  FALSE;
        }

        /*  确保返回的定植的单位同设置的单位类型一致,否则出错*/

        if(SettingValue.ucAttrib!=(pCurElemOutput->ucAttrib))
        {
            LOG_Dbg_Msg("Get Settings  Tuyuan  \'%s\'  CK  Setting  Value  Signal  Type  isn't  Match !\n"
                        ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得定值输入图元\'%s\'的测控定值的信号类型不对\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp  rslv err: get setting input  element id \'%s\' measurement signal type err\n"
                           ,(int)(pElemInitNode->strInputSourceID),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  FALSE;

        }
    }
    /* 判定定植的类型是不是所期望的范围 */
    bSettingTypeIsExpected=RE_SettingTuyuanIsExpectedSettingType
                           (&SettingValue,&ucSettingValueType);
    if(!bSettingTypeIsExpected)
    {
        LOG_Dbg_Msg("Get Settings  Tuyuan  \'%s\'  Setting  Value  Signal  Type  isn't  Expected  !\n"
                    ,(int)(pElemInitNode->strInputSourceID),0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:获得定值输入图元\'%s\'的定值信号类型错误\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:get setting input  element \'%s\' signal type err\n"
                       ,(int)(pElemInitNode->strInputSourceID),0);
        }
        assert(FALSE);  /* 若非以上类型,则告警   */
        return  FALSE;
    }


    /*初始化该输出当前值为定植当前值*/
    RE_SettingTuyuanSetOutputBySettingValue
    (pCurElemOutput,&SettingValue);
    if(SettingValue.ucAttrib==VAR_STRING)/*2008-7-19日 支持字符串定值，张云  */
    {
        pCurElemOutput->pvCh=SettingValue.pvSrc;
    }
    else
    {
        pCurElemOutput->pvCh=NULL;/*通道句柄为空*/
    }
    pCurElemOutput->ucType=2;/*定植*/
    pElemInitNode->ucSignalValueType=ucSettingValueType;
    return  TRUE;

}


/*   用得到的定植值VALUE,检查定植类型，用于定植初始化操作
     参数
            pSettingValue，用来进行赋值的定植值
                        pcRtValueType,用于返回定值值的类型
     返回   BOOL  表示成功与否
*/

BOOL   RE_SettingTuyuanIsExpectedSettingType
(SCI_SIGNAL_VALUE_TYPE   *pSettingValue,
 unsigned  char  *pcRtValueType)
{
    if(((pSettingValue->ucAttrib)==REAL_FORM_DIANLIU_SIGNAL)
            ||((pSettingValue->ucAttrib)==REAL_FORM_DIANLIU_SIGNAL_KA)
            ||((pSettingValue->ucAttrib)==REAL_FORM_DIANLIU_SIGNAL_MA)
            ||((pSettingValue->ucAttrib)==REAL_FORM_DIANYA_SIGNAL)
            ||((pSettingValue->ucAttrib)==REAL_FORM_DIANYA_SIGNAL_KV)
            ||((pSettingValue->ucAttrib)==REAL_FORM_ZUKANG_SIGNAL)
            ||((pSettingValue->ucAttrib)==REAL_FORM_ZUKANG_SIGNAL_KO)
            ||((pSettingValue->ucAttrib)==SHIJIAN_TYPE1_SIGNAL)
            ||((pSettingValue->ucAttrib)==SHIJIAN_TYPE2_SIGNAL)
            ||((pSettingValue->ucAttrib)==SHIJIAN_TYPE3_SIGNAL)
            ||((pSettingValue->ucAttrib)==SHIJIAN_TYPE4_SIGNAL)
            ||((pSettingValue->ucAttrib)==PINLV_SIGNAL)
            ||((pSettingValue->ucAttrib)==HAUACHA_SIGNAL)
            ||((pSettingValue->ucAttrib)==DIANYA_BIANHUALV_SIGNAL)
            ||((pSettingValue->ucAttrib)==JIAODU_SIGNAL)
            ||((pSettingValue->ucAttrib)==WENDU_SIGNAL)
            ||((pSettingValue->ucAttrib)==JULI_SIGNAL)
            ||((pSettingValue->ucAttrib)==BILIXISHU_SIGNAL)
            ||((pSettingValue->ucAttrib)==CEJUXISHU_SIGNAL)
            ||((pSettingValue->ucAttrib)==BUCHANGXISHU_SIGNAL)
            ||((pSettingValue->ucAttrib)==GONGLV_TYPE1_SIGNAL)
            ||((pSettingValue->ucAttrib)==GONGLV_TYPE2_SIGNAL)
            ||((pSettingValue->ucAttrib)==GONGLV_TYPE3_SIGNAL)
            ||((pSettingValue->ucAttrib)==GONGLV_TYPE4_SIGNAL)
            ||((pSettingValue->ucAttrib)==WUGONG_GONGLV_TYPE1_SIGNAL)
            ||((pSettingValue->ucAttrib)==WUGONG_GONGLV_TYPE2_SIGNAL)
            ||((pSettingValue->ucAttrib)==WUGONG_GONGLV_TYPE3_SIGNAL)
            ||((pSettingValue->ucAttrib)==WUGONG_GONGLV_TYPE4_SIGNAL)
            ||((pSettingValue->ucAttrib)==REAL_SIGNAL)
            ||((pSettingValue->ucAttrib)==CAPACITY_SIGNAL)
            ||((pSettingValue->ucAttrib)==DIANDU_TYPE1_SIGNAL)
            ||((pSettingValue->ucAttrib)==DIANDU_TYPE2_SIGNAL)
            ||((pSettingValue->ucAttrib)==DIANDU_TYPE3_SIGNAL)
            ||((pSettingValue->ucAttrib)==DIANDU_TYPE4_SIGNAL)
            ||((pSettingValue->ucAttrib)==WUGONG_DIANDU_TYPE1_SIGNAL)
            ||((pSettingValue->ucAttrib)==WUGONG_DIANDU_TYPE2_SIGNAL)
            ||((pSettingValue->ucAttrib)==WUGONG_DIANDU_TYPE3_SIGNAL)
            ||((pSettingValue->ucAttrib)==WUGONG_DIANDU_TYPE4_SIGNAL)
            ||((pSettingValue->ucAttrib)==OHM_PER_METER))
    {
        /*若是允许的定植类型，则返回真   */
        *pcRtValueType=FLOAT_VALUE_TYPE;
        return   TRUE;

    }
    else  if(((pSettingValue->ucAttrib)==CONTROL_WORD_SIGNAL)
             ||((pSettingValue->ucAttrib)==XIANGBIE_SIGAL)
             ||((pSettingValue->ucAttrib)==HEX_MODE_WORD_SIGNAL)
             ||((pSettingValue->ucAttrib)==VAR_STRING))/*添加字符串类型，张云2008-7-19日张云  */
    {
        *pcRtValueType= UNSIGNED_32INT_VALUE_TYPE;
        return   TRUE;

    }
    else  if(((pSettingValue->ucAttrib)==SHORT_INT_SIGNAL)
             ||((pSettingValue->ucAttrib)==LONG_INT_SIGNAL))
    {
        *pcRtValueType= SIGNED_32INT_VALUE_TYPE;
        return   TRUE;

    }
    else
    {
        /* 若是不允许的定植类型，则异常  */
        assert(FALSE);
        return  FALSE;

    }

}


/*   用得到的定植值VALUE，赋给图元的输出值now
     参数   pIO，待赋值的输出的指针
            pSettingValue，用来进行赋值的定植值
     返回   无
*/

void   RE_SettingTuyuanSetOutputBySettingValue
(EP_ELEM_IO  *pIO,SCI_SIGNAL_VALUE_TYPE   *pSettingValue)
{
    if(((pSettingValue->ucAttrib)==CONTROL_WORD_SIGNAL)
            ||((pSettingValue->ucAttrib)==XIANGBIE_SIGAL)
            ||((pSettingValue->ucAttrib)==HEX_MODE_WORD_SIGNAL)
            ||((pSettingValue->ucAttrib)==VAR_STRING))
    {
        /*若是控制字和相别,或32位16进制方式字或字符串定植 2008-7-19日张云  */
        pIO->now.ulVal=pSettingValue->Value.ulVal;
    }
    else  if(((pSettingValue->ucAttrib)==SHORT_INT_SIGNAL)
             ||((pSettingValue->ucAttrib)==LONG_INT_SIGNAL))
    {
        /* 若是短整型和长整型定植  */
        pIO->now.lVal=pSettingValue->Value.lVal;
    }
    else
    {
        /* 若是其他则处理为实数  */
        pIO->now.fVal=pSettingValue->Value.fVal;  /* 若非以上类型,则为实数*/
    }
    return;

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

EP_STATUS   RE_SettingCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode)
{
    NODE  *pNode;
    Setting_Init_Node_Type    *pInitNode;
    pInitNode=(Setting_Init_Node_Type    *)pElemInitNode->pTuyuan;

    /*  设置图元类型*/
    switch(pInitNode->ucSignalSourceType)
    {
        case  DINGZHI_INPUT_SOURCE:
        {
            DingzhiOuterInput_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg(" Setting   Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(DingzhiOuterInput_Scan_Node_Type   *)
                      malloc
                      (sizeof(DingzhiOuterInput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("Setting  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_DINGZHI_OUTERINPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->ioOut=pInitNode->
                             PublicElemData.elem.aioOut[0];
            pScanNode->pfGetScanNodeOutIOFunc=pInitNode->
                                              PublicElemData.pfGetScanNodeOutIOFunc;
            pScanNode->ucSignalValueType=pInitNode->
                                         ucSignalValueType;
            pScanNode->pbCurScanDingzhiRreshFlag=pInitNode->
                                                 PublicElemData.pbCurScanDingzhiRreshFlag;
            pScanNode->InputSourceSettingInfo=
                pInitNode->InputSourceSettingInfo;
            pScanNode->InputSourceSettingInfo.strRelayFuncName
                =NULL;

            /* 设定返回的节点地址  */
            *pReturnElemScanNodePointer=(NODE  *)pNode;
            return  EP_SUCCESS;
        }
        break;
        case   ANNLOG_CONSTVALUE_SOURCE:
        {
            ConstValueOuterInput_Scan_Node_Type    *pScanNode;
            pNode=(NODE  *)malloc(sizeof(NODE));
            if(pNode==NULL)
            {
                LOG_Dbg_Msg("ConstValue Source  Setting  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pScanNode=(ConstValueOuterInput_Scan_Node_Type    *)
                      malloc
                      (sizeof(ConstValueOuterInput_Scan_Node_Type));
            if(pScanNode==NULL)
            {
                LOG_Dbg_Msg("ConstValue  Source  Setting  Tuyuan  malloc  failiure!\n",0,0,0,0,0,0);
                return  EP_BUF_ERR;
            }
            pNode->ulTuyuanType=RE_CONSTVALUE_OUTERINPUT_SCAN;
            pNode->pTuyuan=(void  *)pScanNode;
            pScanNode->ioOut=pInitNode->
                             PublicElemData.elem.aioOut[0];
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

EP_ELEM_IO *  RE_SettingTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode)
{

    switch(pScanNode->ulTuyuanType)
    {
        case   RE_DINGZHI_OUTERINPUT_SCAN:
        {

            DingzhiOuterInput_Scan_Node_Type    *pTuyuanNode;
            pTuyuanNode=(DingzhiOuterInput_Scan_Node_Type    *)pScanNode->pTuyuan;
            if(unOutNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get Setting  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOut);

        }
        break;
        case   RE_CONSTVALUE_OUTERINPUT_SCAN:
        {

            ConstValueOuterInput_Scan_Node_Type   *pTuyuanNode;
            pTuyuanNode=(ConstValueOuterInput_Scan_Node_Type    *)pScanNode->pTuyuan;
            if(unOutNum>=1)
            {
                /* 若越界，则出错 */

                LOG_Dbg_Msg("Get ConstValue  Source  Setting  Tuyuan  OutputIO  Beyond  Bound!\n",0,0,0,0,0,0);
                assert(FALSE);
                return  NULL;

            }
            return   &(pTuyuanNode->ioOut);

        }
        break;
        default  :

            LOG_Dbg_Msg("Get One Setting  Tuyuan  OutputIO  Error!\n",0,0,0,0,0,0);
            assert(FALSE);
            return  NULL;
            break;
    }


}

/*    定值来源的外部输入图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_DingzhiOuterInputTuyuanScan(NODE *pElemScanNode)
{

    if(!(*(((DingzhiOuterInput_Scan_Node_Type    *)pElemScanNode->pTuyuan)->
            pbCurScanDingzhiRreshFlag)))
    {
        /*若不用更新定值  */
        return;

    }
    else
    {
        /* 若定植刷新标志为真 ，则刷新控制字定植,否则不用更新定植*/

        SCI_SIGNAL_VALUE_TYPE  SettingValue;
        EP_STATUS   OpeResult;
        DingzhiOuterInput_Scan_Node_Type    *pScanNode;

        pScanNode=(DingzhiOuterInput_Scan_Node_Type    *)
                  pElemScanNode->pTuyuan;

        /*根据该定植是一般定植,还是公共定植,获得定植当前值  */
        if(pScanNode->InputSourceSettingInfo.ucType==1)
        {
            /*若该定植是内部定植,则访问该内部定植  */
            OpeResult=SCI_Get_Inner_Setting(
                          pScanNode->InputSourceSettingInfo.nNumInPage,
                          &SettingValue);

            /* 确保返回成功 */
            assert(OpeResult==EP_SUCCESS);
            /* 确保返回类型一致 */
            assert(SettingValue.ucAttrib==
                   pScanNode->ioOut.ucAttrib);

        }
        else if(pScanNode->InputSourceSettingInfo.ucType==0)
        {
            /*若该定植是外部定植,则访问该外部定植  */

            OpeResult=SCI_Get_General_Setting(
                          pScanNode->InputSourceSettingInfo.cPageNum,
                          pScanNode->InputSourceSettingInfo.nNumInPage,
                          &SettingValue);
            /* 确保返回成功 */
            assert(OpeResult==EP_SUCCESS);
            /* 确保返回类型一致 */
            assert(SettingValue.ucAttrib==
                   pScanNode->ioOut.ucAttrib);

        }
        else
        {
            /*若该定植是测控定植,则访问该测控定植  */
            /* 对EDP01，开放测控定值功能 张云 2008-7-15日 */
            OpeResult=SCI_Get_CK_Setting(
                          pScanNode->InputSourceSettingInfo.nNumInPage,
                          &SettingValue);

            /* 确保返回成功 */
            assert(OpeResult==EP_SUCCESS);
            /* 确保返回类型一致 */
            assert(SettingValue.ucAttrib==
                   pScanNode->ioOut.ucAttrib);

        }

        /* 定值句柄或字符串基址赋值 */
        pScanNode->ioOut.pvCh = SettingValue.pvSrc;

        /*将当前更新过的定植作为该输出当前值*/

        if(pScanNode->ucSignalValueType==FLOAT_VALUE_TYPE)
        {
            /* 若是其他则处理为实数  */
            pScanNode->ioOut.now.fVal=SettingValue.Value.fVal;  /* 若非以上类型,则为实数*/
            return;
        }
        else  if(pScanNode->ucSignalValueType==UNSIGNED_32INT_VALUE_TYPE)
        {
            /*若是控制字和相别定植，，方式字，字符串  2008-7-19日张云 */
            pScanNode->ioOut.now.ulVal=SettingValue.Value.ulVal;
            return;
        }
        else  if(pScanNode->ucSignalValueType==SIGNED_32INT_VALUE_TYPE)
        {
            /* 若是短整型和长整型定植  */
            pScanNode->ioOut.now.lVal=SettingValue.Value.lVal;
            return;
        }
        else
        {
            assert(FALSE);
            return;
        }
        return;

    }
    return;
}

/* 扫描多个定值节点.
 * Para:
 *     PartGrpAttrib, 分图属性.
 * Return:
 *     NONE.
 */
void RE_MultiDingzhiOuterInputTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib)
{
    NODE **ppInputNode;
    int i;
    SCI_SIGNAL_VALUE_TYPE SettingValue;
    EP_STATUS OpeResult;
    DingzhiOuterInput_Scan_Node_Type *pScanNode;

    ppInputNode = ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->ppSetNode;
    for (i = 0; i<((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->SetScanNodeNum; i++, ppInputNode++)
    {
        pScanNode = (DingzhiOuterInput_Scan_Node_Type *)(*ppInputNode)->pTuyuan;

        /* 根据该定植是一般定植,还是公共定植,获得定植当前值 */
        if (pScanNode->InputSourceSettingInfo.ucType == 1)
        {
            /* 若该定植是内部定植,则访问该内部定植 */
            OpeResult = SCI_Get_Inner_Setting(
                            pScanNode->InputSourceSettingInfo.nNumInPage, &SettingValue);
        }
        else if (pScanNode->InputSourceSettingInfo.ucType == 0)
        {
            /* 若该定植是外部定植,则访问该外部定植 */
            OpeResult = SCI_Get_General_Setting(
                            pScanNode->InputSourceSettingInfo.cPageNum,
                            pScanNode->InputSourceSettingInfo.nNumInPage,
                            &SettingValue);
        }
        else
        {
            /* 若该定植是测控定植,则访问该测控定植 */
            OpeResult = SCI_Get_CK_Setting(
                            pScanNode->InputSourceSettingInfo.nNumInPage,
                            &SettingValue);
        }

        /* 定值句柄或字符串基址赋值 */
        pScanNode->ioOut.pvCh = SettingValue.pvSrc;

        /* 将当前更新过的定植作为该输出当前值 */
        if (pScanNode->ucSignalValueType == FLOAT_VALUE_TYPE)
        {
            /* 若是其他则处理为实数 */
            pScanNode->ioOut.now.fVal = SettingValue.Value.fVal;  /* 若非以上类型,则为实数 */
        }
        else if (pScanNode->ucSignalValueType == UNSIGNED_32INT_VALUE_TYPE)
        {
            /* 若是控制字和相别定植,方式字,字符串 */
            pScanNode->ioOut.now.ulVal = SettingValue.Value.ulVal;
        }
        else if (pScanNode->ucSignalValueType == SIGNED_32INT_VALUE_TYPE)
        {
            /* 若是短整型和长整型定植 */
            pScanNode->ioOut.now.lVal=SettingValue.Value.lVal;
        }
    }
}
