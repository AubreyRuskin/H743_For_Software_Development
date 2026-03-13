/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_LoadLogrpFile.c                                   1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中读取逻辑图文件的相关函数的实现                 */


/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                    */
/*                                                                              */
/*         张云       2002.12.11            创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/


#include <vxWorks.h>
#include   "RE_LoadLogrpFile.h"

#include  "RE_SuanfaTuyuan.h"
#include  "RE_AndTuyuan.h"
#include  "RE_OrTuyuan.h"
#include  "RE_NotTuyuan.h"
#include  "RE_TimerTuyuan.h"
#include  "RE_YabanTuyuan.h"
#include  "RE_ControlWordTuyuan.h"
#include  "RE_ConstOneTuyuan.h"
#include  "RE_ConstZeroTuyuan.h"
#include  "RE_GreaterThanTuyuan.h"
#include  "RE_LessThanTuyuan.h"
#include  "RE_EqualTuyuan.h"
#include  "RE_OuterInputTuyuan.h"
#include  "RE_OuterOutputTuyuan.h"
#include  "RE_LuboTuyuan.h"
#include  "RE_EventTuyuan.h"
#include  "RE_MultiwaySelectTuyuan.h"
#include  "RE_ImportTuyuan.h"
#include  "RE_ExportTuyuan.h"
#include  "RE_SettingTuyuan.h"
#include  "RE_RelayStartTuyuan.h"
#include  "RE_ReportStartTuyuan.h"
#include  "RE_DIModTuyuan.h"
#include  "RE_AISetTuyuan.h"
#include  "RE_DISetTuyuan.h"
#include  "RE_DOSetTuyuan.h"
#include  "RE_MaxTuyuan.h"
#include  "RE_MinTuyuan.h"
#include  "RE_AmpAngTuyuan.h"
#include  "RE_RealImageTuyuan.h"
#include  "RE_ModeWordTuyuan.h"
#include  "RE_OuterOrderTuyuan.h"
#include  "RE_SysServerTuyuan.h"

/*2011-7-27  ZY  */
#include  "RE_DOCollect.h"
#include  "RE_LampCollect.h"
#include  "RE_EventCollect.h"
#include  "RE_ExportCollect.h"
#include  "RE_FloatAICollect.h"
#include  "RE_CmplxAICollect.h"
#include  "RE_DICollect.h"
#include  "RE_ImportCollect.h"
#include  "RE_TuyuanCollect.h"



/*  读取逻辑图文件,并创建相应的图元节点,添加到相应的节点连表中去
    参数  strLogrpSeqFileName ,逻辑图文件名
          pRtLogrpAttrib,供返回相应的逻辑图属性信息
    返回值  EP_STATUS
 */
EP_STATUS   RE_ReadLogrpFileInit(char   *  strLogrpSeqFileName,
                                 LOGRP_ATTRIB_TYPE  *pRtLogrpAttrib)
{
    int  i;
    uint32_t   ulFpCurOffset=0;/* 当前文件指针偏移 */
    unsigned  long  ulTempLong;
    uint16_t   unTempShort;
    char   strZhuangzhiName[300];
    char   strRlsVer[300];
    unsigned  char  ucPartGrpCount;/* 分图个数 */
    long  nNodeListArrDims;/*创建的节点连表数组当前维数  */
    unsigned  long   ulStrNameLen;
    unsigned  long   ulPartGrpDataLen;/* 当前分图的内容长度 */

    int  nOpeResult;


    EP_STATUS  RtResult;
    BOOL    bReadFileSuccess;
    FILE  *fp;
    fp=fopen(strLogrpSeqFileName,"rb");/* 以2进制方式打开逻辑图只读文件 */
    if(fp==NULL)
    {
        /* 若打开失败,则返回错误 */
        LOG_Dbg_Msg("Open  logrp  file  failure  by  file  name!\n",0,0,0,0,0,0);
        return  EP_NOT_INIT;
    }
    /* 读取文件头符 */
    bReadFileSuccess=ReadUnsignedLongFromResloveSeqFile
                     (fp,&ulTempLong);
    if(!bReadFileSuccess)
    {
        fclose(fp);/*  关闭文件 */
        return  EP_NOT_INIT;
    }
    if(ulTempLong!=0XCC000033)
    {
        /* 若文件头符不对,则出错 */
        LOG_Dbg_Msg("Logrp  file  head  type  isn't  match!\n",0,0,0,0,0,0);
        fclose(fp);
        return  EP_NOT_INIT;

    }
    ulFpCurOffset=ulFpCurOffset+4;/* 更新文件指针偏移 */
    /* 读取配置规约版本号  */
    bReadFileSuccess=ReadUnsignedShortFromResloveSeqFile
                     (fp,&unTempShort);
    if(!bReadFileSuccess)
    {
        fclose(fp);/*  关闭文件 */
        return  EP_NOT_INIT;
    }
    ulFpCurOffset=ulFpCurOffset+2;/* 更新文件指针偏移 */
    /* 读取程序版本号  */
    bReadFileSuccess=ReadUnsignedShortFromResloveSeqFile
                     (fp,&unTempShort);
    if(!bReadFileSuccess)
    {
        fclose(fp);/*  关闭文件 */
        return  EP_NOT_INIT;
    }
    ulFpCurOffset=ulFpCurOffset+2;/* 更新文件指针偏移 */

    /*读取版本号字符串  */
    bReadFileSuccess=ReadStringOfByteLenFromResloveSeqFile
                     ( fp,strRlsVer,&ulStrNameLen);
    if(!bReadFileSuccess)
    {
        fclose(fp);/*  关闭文件 */
        return  EP_NOT_INIT;
    }
    ulFpCurOffset=ulFpCurOffset+ulStrNameLen+1;/* 更新文件指针偏移 */

    /*  读取2个字节逻辑图版本号 */
    bReadFileSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,2);
    if(!bReadFileSuccess)
    {
        fclose(fp);/*  关闭文件 */
        return  EP_NOT_INIT;
    }
    ulFpCurOffset=ulFpCurOffset+2;/* 更新文件指针偏移 */
    /* 读取逻辑图文件，即装置名  */
    bReadFileSuccess=ReadStringOfByteLenFromResloveSeqFile
                     ( fp,strZhuangzhiName,&ulStrNameLen);
    if(!bReadFileSuccess)
    {
        fclose(fp);/*  关闭文件 */
        return  EP_NOT_INIT;
    }
    ulFpCurOffset=ulFpCurOffset+ulStrNameLen+1;/* 更新文件指针偏移 */

    /*  读取4个保留字节 */
    bReadFileSuccess=ReadReserveBytesFromResloveSeqFile
                     (fp,4);
    if(!bReadFileSuccess)
    {
        fclose(fp);/*  关闭文件 */
        return  EP_NOT_INIT;
    }
    ulFpCurOffset=ulFpCurOffset+4;/* 更新文件指针偏移 */

    /* 读取独立功能分图个数  */
    bReadFileSuccess=ReadUnsignedCharFromResloveSeqFile
                     (fp,&ucPartGrpCount);
    if(!bReadFileSuccess)
    {
        fclose(fp);/*  关闭文件 */
        return  EP_NOT_INIT;
    }
    if(ucPartGrpCount>MAX_RELAY_FUNC_COUNT)
    {
        /* 若分图个数超过允许的最大数目,则出错 */
        LOG_Dbg_Msg("Part Grp  Count  is  too  much!\n",0,0,0,0,0,0);
        fclose(fp);
        return  EP_NOT_INIT;
    }
    ulFpCurOffset=ulFpCurOffset+1;/* 更新文件指针偏移 */

    /* 获取所有分图和总图的属性   */
    RtResult=RE_GetAllPartGrpAttrib
             (fp,ulFpCurOffset,(unsigned  long)ucPartGrpCount,
              pRtLogrpAttrib);
    if(RtResult!=EP_SUCCESS)
    {
        LOG_Dbg_Msg("Error,Get  All  Part Grp  Attrib  failure!\n",0,0,0,0,0,0);
        fclose(fp);
        return  RtResult;
    }


    /* 读取当前每个分图的内容  */
    nNodeListArrDims=0;/* 设定连表维数初值为0,表示还未创建 */
    for(i=0; i<ucPartGrpCount; i++)
    {
        /*  对每个分图进行循环读取*/

        /* 文件指针需重新定位,因为在读取每个分图时,
            也使用了文件指针并进行过重新定位  */
        int  nOpeResult;
        nOpeResult=fseek(fp,ulFpCurOffset,0);
        if(nOpeResult!=0)
        {
            LOG_Dbg_Msg("File  Pointer  Seek  Failure  !\n",0,0,0,0,0,0);
            fclose(fp);
            return  EP_NOT_INIT;
        }

        /* 获得该分图的内容长度 */
        bReadFileSuccess=ReadUnsignedLongFromResloveSeqFile
                         (fp,&ulPartGrpDataLen);
        if(!bReadFileSuccess)
        {
            fclose(fp);
            return  EP_NOT_INIT;
        }
        ulFpCurOffset=ulFpCurOffset+4; /* 更新文件指针偏移 */
        /* 读取该分图 */
        RtResult=RE_PartGrpReadFileInit(fp,ulFpCurOffset,
                                        nNodeListArrDims);
        if(RtResult!=EP_SUCCESS)
        {
            fclose(fp);
            return  RtResult;
        }
        /* 读取该分图后更新文件指针偏移,供读取下1个分图 */
        ulFpCurOffset=ulFpCurOffset+ulPartGrpDataLen;
        nNodeListArrDims++;

    }

    /* 文件指针重新定位  */
    nOpeResult=fseek(fp,ulFpCurOffset,0);
    if(nOpeResult!=0)
    {
        LOG_Dbg_Msg("File  Pointer  Seek  Failure  !\n",0,0,0,0,0,0);
        return  EP_SYS_ERR;
    }
    /*读取文件尾符  */

    bReadFileSuccess=ReadUnsignedLongFromResloveSeqFile
                     (fp,&ulTempLong);
    if(!bReadFileSuccess)
    {
        fclose(fp);/*  关闭文件 */
        return  EP_NOT_INIT;
    }
    if(ulTempLong!=0XC6000039)
    {
        /* 若文件尾符不对,则出错 */
        LOG_Dbg_Msg("Logrp  file  tail  type  isn't  match!\n",0,0,0,0,0,0);
        fclose(fp);
        return  EP_NOT_INIT;

    }
    ulFpCurOffset=ulFpCurOffset+4;/* 更新文件指针偏移 */


    /* 最后关闭文件 */
    fclose(fp);
    return  EP_SUCCESS;


}


/*  从逻辑图文件中获取所有分图的属性信息
        主要指每个分图的属性和总图的属性信息
    参数   fp  文件指针
           ulReadOffsetToBegain，当前分图内容相对于文件头的偏移
           ulPartGrpCount,所有分图的总数
           pRtLogrpAttrib,供返回相应的逻辑图属性信息
     返回值  EP_STATUS

 */
EP_STATUS   RE_GetAllPartGrpAttrib
(FILE  *fp,uint32_t  ulReadOffsetToBegain
 ,unsigned  long  ulPartGrpCount,
 LOGRP_ATTRIB_TYPE  *pRtLogrpAttrib)
{

    uint32_t  ulFpCurOffset ;/* 文件指针当前操作位置  */
    static  char   strCurPartGrpName[300];/* 为了能打印信息，设为静态 */
    int  i,k;
    BOOL   bReadSuccess;
    EP_STATUS   RtResult;
    unsigned  long   nStrLen;
    BOOL  bCurPartGrpStatus;
    int  nOpeResult;
    unsigned  char  ucTempChar;
    unsigned  short  uiTempShort;
    unsigned  long   ulPartGrpDataLen;/* 当前分图的内容长度 */
    unsigned  int   uiMinScanInterval;   /*2006-2-9日进行了修改  */
    unsigned  int   uiBottomScanInterval;
    unsigned  long  ulTaskCouter;  /*可能创建的最大的任务数   */
    unsigned  int   uiTaskScanInterval = 0xFFFFFFFF;
    BOOL   bCanCreateTask;

    BOOL    bReadFileSuccess;



    ulFpCurOffset =ulReadOffsetToBegain;


    /*  对每个分图,获取投退属性和扫描周期属性*/

    for(i=0; i<ulPartGrpCount; i++)
    {

        /* 文件指针需重新定位,因为在读取每个分图时,
            也使用了文件指针并进行过重新定位  */

        nOpeResult=fseek(fp,ulFpCurOffset,0);
        if(nOpeResult!=0)
        {
            LOG_Dbg_Msg("File  Pointer  Seek  Failure  !\n",0,0,0,0,0,0);
            fclose(fp);
            return  EP_NOT_INIT;
        }

        /* 获得该分图的内容长度 */
        bReadFileSuccess=ReadUnsignedLongFromResloveSeqFile
                         (fp,&ulPartGrpDataLen);
        if(!bReadFileSuccess)
        {
            fclose(fp);
            return  EP_NOT_INIT;
        }

        ulFpCurOffset=ulFpCurOffset+4; /* 更新文件指针偏移 */

        /* 获得该分图的属性信息 */

        /* 读取保护分图名 */
        bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                     (fp,strCurPartGrpName,&nStrLen);
        if(!bReadSuccess)
        {
            return  EP_SYS_ERR;
        }

        /* 访问保护投退状态 */
        RtResult=SCI_Init_Get_RelayFunc_RunExit_Set_Status
                 (strCurPartGrpName, &bCurPartGrpStatus);

        strcpy(RE_aPartGrpAttribArr[i].strCurPartGrpName, strCurPartGrpName);		/* 逻辑图名称给定 */

        if(RtResult!=EP_SUCCESS)
        {

            LOG_Dbg_Msg("Error,Get  Relay  Func   \'%s\'  RunExit Status  Failure !\n"
                        ,(int)(strCurPartGrpName),0,0,0,0,0);


            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "逻辑图解析错误:获得保护功能\'%s\'投退状态失败\n"
                           ,(int)(strCurPartGrpName),0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK,
                           "logic grp rslv err:get  relay func \'%s\' run status failure\n"
                           ,(int)(strCurPartGrpName),0);
            }
            assert(FALSE);  /* 若非以上类型,则告警   */
            return  EP_SYS_ERR;
        }

        if(bCurPartGrpStatus==FALSE)
        {
            /* 若该保护分图退出,则直接返回 */
            RE_aPartGrpAttribArr[i].bRunFlag
                =FALSE;
        }
        else
        {
            /* 若该保护分图被投入 */

            RE_aPartGrpAttribArr[i].bRunFlag
                =TRUE;
        }

        /* 读取2字节本分图扫描周期信息,2006-2-9  */

        bReadFileSuccess=ReadUnsignedShortFromResloveSeqFile
                         (fp,&uiTempShort);
        if(!bReadFileSuccess)
        {
            fclose(fp);/*  关闭文件 */
            return  EP_NOT_INIT;
        }
        RE_aPartGrpAttribArr[i].uiScanDriveInterval
            =uiTempShort;
        /* 读取1字节本分图扫描属性信息*/

        bReadFileSuccess=ReadUnsignedCharFromResloveSeqFile
                         (fp,&ucTempChar);
        if(!bReadFileSuccess)
        {
            fclose(fp);/*  关闭文件 */
            return  EP_NOT_INIT;
        }
        RE_aPartGrpAttribArr[i].ucScanAttr
            =ucTempChar;
        /* 读取该分图后更新文件指针偏移,供读取下1个分图 */
        ulFpCurOffset=ulFpCurOffset+ulPartGrpDataLen;

    }



    /*  对每个分图,获取保护任务号属性*/


    uiBottomScanInterval=0;
    ulTaskCouter=0;
    while(TRUE)
    {
        BOOL   bCanFindGreaterInterval=FALSE;
        uiMinScanInterval=0xFFFFFFFF;
        for(i=0; i<ulPartGrpCount; i++)
        {
            /*  对每个分图获得当前次小值*/
            unsigned  int    uiCurGrpInterval;
            uiCurGrpInterval=RE_aPartGrpAttribArr[i]
                             .uiScanDriveInterval;
            if((uiCurGrpInterval>uiBottomScanInterval)
                    &&(uiCurGrpInterval<uiMinScanInterval))
            {
                uiMinScanInterval=uiCurGrpInterval;
                bCanFindGreaterInterval=TRUE;
            }


        }
        if(!bCanFindGreaterInterval)
        {
            /*  若不能找到，则表示设置结束 */
            break;
        }
        else
        {

            for(k=0; k<ulPartGrpCount; k++)
            {
                /*  对每个分图设置投入时，相应的任务号*/
                if(RE_aPartGrpAttribArr[k].uiScanDriveInterval==
                        uiMinScanInterval)
                {
                    RE_aPartGrpAttribArr[k].nScanTaskNo
                        =ulTaskCouter;
                }

            }

        }
        ulTaskCouter++;
        uiBottomScanInterval=uiMinScanInterval;
    }

    /*  获得总图的属性信息     */

    pRtLogrpAttrib->nNodeListArrDims=(long)ulPartGrpCount;
    pRtLogrpAttrib->nAllowMaxTaskCount=ulTaskCouter;
    for(i=0; i<ulTaskCouter; i++)
    {
        /* 对每个任务，确定是否能被投入  */
        bCanCreateTask=FALSE;

        for(k=0; k<ulPartGrpCount; k++)
        {
            /* 查找每个分图*/
            if(RE_aPartGrpAttribArr[k].nScanTaskNo==i)
            {
                uiTaskScanInterval=
                    RE_aPartGrpAttribArr[k].uiScanDriveInterval ;
                if(RE_aPartGrpAttribArr[k].bRunFlag)
                {
                    /* 若找到1个相应级别的分图被投入，
                       则跳出循环 */
                    bCanCreateTask=TRUE;
                    break;

                }
            }

        }
        if(bCanCreateTask)
        {
            /* 若该级任务能被创建  */
            pRtLogrpAttrib->bGrpScanTaskCreateFlagArr[i]
                =TRUE;
            pRtLogrpAttrib->bGrpScanTaskScannedFlagArr[i]=FALSE; /*2007-10-26 DQ: 初始化逻辑图已扫描标志为false*/
            pRtLogrpAttrib->uiGrpScanDriveSamPeriodIntervalArr[i]
                =  uiTaskScanInterval;

            pRtLogrpAttrib->iTaskExportScanNodeCntArr[i]=0;/* 2008-1-24日张云修改 */
            pRtLogrpAttrib->ppTaskImportScanNodeArr[i]=calloc(RELAY_SCAN_TASK_ALLOW_MAX_IMPORT_SIZE, sizeof(NODE *)) ;/* 2006-11-4日张云修改 */
            if(!(pRtLogrpAttrib->ppTaskImportScanNodeArr[i]))
            {
                return  EP_NOT_INIT;
            }
            pRtLogrpAttrib->iTaskExportScanNodeCntArr[i]=0;
            pRtLogrpAttrib->ppTaskExportScanNodeArr[i]=calloc(RELAY_SCAN_TASK_ALLOW_MAX_EXPORT_SIZE, sizeof(NODE *)) ;
            if(!(pRtLogrpAttrib->ppTaskExportScanNodeArr[i]))
            {
                return  EP_NOT_INIT;
            }
            pRtLogrpAttrib->iTaskOptAOScanNodeCntArr[i]=0;
            pRtLogrpAttrib->ppTaskOptAOScanNodeArr[i]=calloc(RELAY_SCAN_TASK_ALLOW_MAX_OPTAO_SIZE, sizeof(NODE *)) ;
            if(!(pRtLogrpAttrib->ppTaskOptAOScanNodeArr[i]))
            {
                return  EP_NOT_INIT;
            }
        }
        else
        {
            /* 若该级任务不能被创建  */

            pRtLogrpAttrib->bGrpScanTaskCreateFlagArr[i]
                =FALSE;
            pRtLogrpAttrib->bGrpScanTaskScannedFlagArr[i]=FALSE; /*2008-1-24 张云DQ: 初始化逻辑图已扫描标志为false*/
            pRtLogrpAttrib->uiGrpScanDriveSamPeriodIntervalArr[i]
                =  uiTaskScanInterval;

            pRtLogrpAttrib->iTaskExportScanNodeCntArr[i]=0;/* 2008-1-24日张云修改 */
            pRtLogrpAttrib->ppTaskImportScanNodeArr[i]=NULL;
            pRtLogrpAttrib->iTaskExportScanNodeCntArr[i]=0;
            pRtLogrpAttrib->ppTaskExportScanNodeArr[i]=NULL;
            pRtLogrpAttrib->iTaskOptAOScanNodeCntArr[i]=0;
            pRtLogrpAttrib->ppTaskOptAOScanNodeArr[i]=NULL ;
        }

    }

    return  EP_SUCCESS;
}





/*  从逻辑图文件中读取保护功能分图信息
    若该保护功能分图被投入，则创建并添加图元节点到连表数组成员中
    否则，不用读取解释图元节点内容，不添加新连表到连表数组中

    参数   fp  文件指针
           ulReadOffsetToBegain，当前分图内容相对于文件头的偏移,
                                 从分图名开始
           nCurNodeListArrDim,当前操作时的图元节点连表数组序号，


 */
EP_STATUS   RE_PartGrpReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                   long     nCurNodeListArrDim)
{

    uint32_t  ulCurFPOffset;/* 文件指针当前操作位置  */
    char   strCurPartGrpName[300];
    int  i;
    BOOL   bReadSuccess;
    EP_STATUS   RtResult;
    unsigned  long   nStrLen;
    unsigned  short  unTuyuanDataLen;
    unsigned  short   nTuyuanCount;
    int  nOpeResult;

    unsigned  char  ucTempChar;

    /* 文件指针重新定位  */
    nOpeResult=fseek(fp,ulReadOffsetToBegain,0);
    if(nOpeResult!=0)
    {
        LOG_Dbg_Msg("File  Pointer  Seek  Failure  !\n",0,0,0,0,0,0);
        return  EP_SYS_ERR;
    }

    ulCurFPOffset=ulReadOffsetToBegain;

    /* 读取保护分图名 */
    bReadSuccess=ReadStringOfByteLenFromResloveSeqFile
                 (fp,strCurPartGrpName,&nStrLen);
    if(!bReadSuccess)
    {
        return  EP_SYS_ERR;
    }

    ulCurFPOffset=ulCurFPOffset+1+nStrLen;/* 更新文件指针当前位置偏移 */

    /* 读取1字节本分图扫描周期信息  */

    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&ucTempChar);
    if(!bReadSuccess)
    {
        fclose(fp);/*  关闭文件 */
        return  EP_NOT_INIT;
    }

    ulCurFPOffset=ulCurFPOffset+1;/* 更新文件指针偏移 */

    /* 读取3字节分图属性 */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile(fp,3);
    if(!bReadSuccess)
    {
        return  EP_SYS_ERR;
    }

    ulCurFPOffset=ulCurFPOffset+3;
    /*  读取4字节保留字节 */
    bReadSuccess=ReadReserveBytesFromResloveSeqFile(fp,4);
    if(!bReadSuccess)
    {
        return  EP_SYS_ERR;
    }
    ulCurFPOffset=ulCurFPOffset+4;
    /* 获得分图图元个数 */
    bReadSuccess=ReadUnsignedShortFromResloveSeqFile
                 (fp,&nTuyuanCount);
    if(!bReadSuccess)
    {
        return  EP_SYS_ERR;
    }
    ulCurFPOffset=ulCurFPOffset+2;
    /* 对每1个图元进行读取操作 */
    for(i=0; i<nTuyuanCount; i++)
    {

        /* 文件指针需重新定位,因为在读取每个图元时,
           也使用了文件指针并进行过重新定位  */
        nOpeResult=fseek(fp,ulCurFPOffset,0);
        if(nOpeResult!=0)
        {
            LOG_Dbg_Msg("File  Pointer  Seek  Failure  !\n",0,0,0,0,0,0);
            return  EP_SYS_ERR;
        }

        /* 获得该图元的图元内容长度 */
        bReadSuccess=ReadUnsignedShortFromResloveSeqFile
                     (fp,&unTuyuanDataLen);
        if(!bReadSuccess)
        {
            return  EP_SYS_ERR;
        }
        ulCurFPOffset=ulCurFPOffset+2;
        /* 读取该图元 */
        RtResult=RE_TuyuanReadFileInit(fp,ulCurFPOffset,
                                       (RE_aPartGrpAttribArr+nCurNodeListArrDim)
                                       ,nCurNodeListArrDim);
        if(RtResult!=EP_SUCCESS)
        {
            return  RtResult;
        }
        /* 读取该图元后更新文件指针偏移,供读取下1个图元 */
        ulCurFPOffset=ulCurFPOffset+unTuyuanDataLen;
    }

    return  EP_SUCCESS;

}





/*  从逻辑图文件中读取图元信息

    参数   fp  文件指针
           ulReadOffsetToBegain，当前图元内容相对于文件头的偏移,
           包括图元类型字节
           pPartGrpAttrib,该分图的属性的指针，传输分图属性
           nPartGrpNo,所在分图号
     返回值  EP_STATUS
*/
EP_STATUS   RE_TuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                  PARTGRP_ATTRIB_TYPE  *pPartGrpAttrib,long  nPartGrpNo)
{

    uint8_t   ucTuyuanType;
    BOOL   bReadSuccess;
    EP_STATUS   RtResult;
    NODE   *pElemInitNode;
    LIST   *pInitNodeList;
    long   nTaskNo;
    TUYUAN_READ_FILE_INIT_DATA   TuyuanInitData;

    nTaskNo=pPartGrpAttrib->nScanTaskNo;

    TuyuanInitData.pchart=RE_aGrpScanTaskMsg+nTaskNo;
    TuyuanInitData.pbScanRefreshDingzhiFlag=
        RE_aGrpScanDingzhiRefreshFlag+nTaskNo;
    TuyuanInitData.nScanTaskNo=nTaskNo;/* 任务 序号*/



    /* 获得当前操作的分图的节点连表 */
    pInitNodeList=RE_aPartGrpInitNodeList+nPartGrpNo;

    /* 读取图元类型  */
    bReadSuccess=ReadUnsignedCharFromResloveSeqFile
                 (fp,&ucTuyuanType);
    if(!bReadSuccess)
    {
        return  EP_SYS_ERR;
    }
    /* 设置图元读取偏移 */
    ulReadOffsetToBegain=ulReadOffsetToBegain+1;
    switch(ucTuyuanType)
    {
        /* 根据不同的图元类型,分别读取图元内容,申请节点  */
        case  RE_SUANFA:
            RtResult=RE_SuanfaTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                 &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);
            break;
        case RE_AND :
            RtResult=RE_AndTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                              &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case RE_OR :
            RtResult=RE_OrTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                             &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_NOT:
            RtResult=RE_NotTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                              &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case RE_TIMER :
            RtResult=RE_TimerTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_YABAN:
            RtResult=RE_YabanTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_CONTROLWORD:
            RtResult=RE_ControlWordTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                     &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_ZERO:
            RtResult=RE_ConstZeroTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                    &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_ONE:
            RtResult=RE_ConstOneTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                   &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_GREATERTHAN:
            RtResult=RE_GreaterThanTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                     &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_LESSTHAN:
            RtResult=RE_LessThanTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                   &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_EQUAL:
            RtResult=RE_EqualTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_OUTERINPUT:
            RtResult=RE_OuterInputTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                     &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_OUTEROUTPUT:
            RtResult=RE_OuterOutputTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                     &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_LUBO:
            RtResult=RE_LuboTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                               &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_EVENT:
            RtResult=RE_EventTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_MULTIWAYSELECT:
            RtResult=RE_MultiwaySelectTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                     &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_IMPORT:
            RtResult=RE_ImportTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                 &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;

        case  RE_EXPORT:
            RtResult=RE_ExportTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                 &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_SETTING:
            RtResult=RE_SettingTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                  &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_RELAYSTART:
            RtResult=RE_RelayStartTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                     &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;

        case  RE_REPORTSTART:
            RtResult=RE_ReportStartTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                     &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_DIMOD:
            RtResult=RE_DIModTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_AISET:
            RtResult=RE_AISetTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;

        case  RE_DISET:
            RtResult=RE_DISetTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_DOSET:
            RtResult=RE_DOSetTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_AMPANG:
            RtResult=RE_AmpAngTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                 &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;

        case  RE_REALIMAGE:
            RtResult=RE_RealImageTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                    &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_MAX:
            RtResult=RE_MaxTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                              &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_MIN:
            RtResult=RE_MinTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                              &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case  RE_MODEWORD:
            RtResult=RE_ModeWordTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                   &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case RE_OUTERORDER:
            RtResult=RE_OuterOrderTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                     &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        case RE_SYSSERVOUT:
            RtResult=RE_SysServerTuyuanReadFileInit(fp,ulReadOffsetToBegain,
                                                    &TuyuanInitData,&pElemInitNode);
            if(RtResult!=EP_SUCCESS)
            {
                return  RtResult;
            }
            /*添加申请的初始化节点到相应连表中  */
            RE_LstAdd(pInitNodeList,pElemInitNode);

            break;
        default:
            /* 若是其他图元类型,则出错  */
            LOG_Dbg_Msg("Read  Tuyuan  Type  Error!\n",0,0,0,0,0,0);
            assert(FALSE);
            return  EP_SYS_ERR;

    }

    return  EP_SUCCESS;

}





/* 创建相应的扫描图元节点,添加到相应的扫描节点连表中去
    参数
          PartGrpScanNodeListArr，扫描节点连表的数组，
                                  此时每个连表内容还为空
          PartGrpInitNodeListArr，初始化节点连表数组
          nPartGrpSum，创建的分图个数
          PartGrpAttribArr, 分图属性
    返回值  EP_STATUS

   */

EP_STATUS   RE_CreateScanNodeInit
(LIST  * PartGrpScanNodeListArr,
 LIST  * PartGrpInitNodeListArr,
 int   nPartGrpSum,
 PARTGRP_ATTRIB_TYPE *PartGrpAttribArr)
{
    int  k;
    LIST  *  pPartGrpScanNodeListArr;
    LIST   * pPartGrpInitNodeListArr;
    PARTGRP_ATTRIB_TYPE *pPartGrpAttribArr;
    long   nPartGrpCount;

    LIST  *pCurPartGrpScanNodeList;
    LIST  *pCurPartGrpInitNodeList;
    PARTGRP_ATTRIB_TYPE  *pCurPartGrpAttribArr;
    EP_STATUS  RtResult;
    NODE  *pCurInitNode;
    NODE   *pCurScanNode;
    Setting_Init_Node_Type *pSettingInitNode; /* 定值 */
    OuterInput_Init_Node_Type *pOuterInputInitNode; /* 外部输入 */
    OuterOutput_Init_Node_Type *pOuterOutputInitNode; /* 外部输出 */
    Lubo_Init_Node_Type *pLuboInitNode; /* 录波 */
    AISet_Init_Node_Type *pAISetInitNode; /* AI集 */

    pPartGrpScanNodeListArr=PartGrpScanNodeListArr;
    pPartGrpInitNodeListArr=PartGrpInitNodeListArr;
    pPartGrpAttribArr = PartGrpAttribArr;
    nPartGrpCount=(long)nPartGrpSum;
    for(k=0; k<nPartGrpCount; k++)
    {
        /* 依次获得各分图的图元的扫描节点连表 */
        pCurPartGrpScanNodeList=pPartGrpScanNodeListArr+k;
        /* 依次获得各分图的图元的初始化节点连表 */
        pCurPartGrpInitNodeList=pPartGrpInitNodeListArr+k;

        pCurPartGrpAttribArr = pPartGrpAttribArr+k;
        /* 获得当前初始化连表第1个图元 */
        pCurInitNode=RE_LstFirst(pCurPartGrpInitNodeList);
        for(;;)
        {
            /* 扫描图元*/
            if(pCurInitNode==NULL)
            {
                /* 若到了图元连表的尾部,则跳出连表操作 */
                break;
            }

            switch(pCurInitNode->ulTuyuanType)
            {
                /* 根据不同的图元类型,分别根据图元初始化节点的内容,申请扫描节点  */
                case  RE_SUANFA:
                    pCurPartGrpAttribArr->sfNum++;
                    RtResult=RE_SuanfaCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);
                    break;
                case RE_AND :
                    pCurPartGrpAttribArr->andNum++;
                    RtResult=RE_AndCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case RE_OR :
                    pCurPartGrpAttribArr->orNum++;
                    RtResult=RE_OrCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_NOT:
                    pCurPartGrpAttribArr->notNum++;
                    RtResult=RE_NotCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case RE_TIMER :
                    pCurPartGrpAttribArr->timerNum++;
                    RtResult=RE_TimerCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);
                    break;
                case  RE_YABAN:
                    pCurPartGrpAttribArr->ybNum++;
                    RtResult=RE_YabanCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_CONTROLWORD:
                    pCurPartGrpAttribArr->cwNum++;
                    RtResult=RE_ControlWordCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_ZERO:
                    pCurPartGrpAttribArr->constZeroNum++;
                    RtResult=RE_ConstZeroCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_ONE:
                    pCurPartGrpAttribArr->constOneNum++;
                    RtResult=RE_ConstOneCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_GREATERTHAN:
                    pCurPartGrpAttribArr->bNum++;
                    RtResult=RE_GreaterThanCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_LESSTHAN:
                    pCurPartGrpAttribArr->sNum++;
                    RtResult=RE_LessThanCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_EQUAL:
                    pCurPartGrpAttribArr->eNum++;
                    RtResult=RE_EqualCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_OUTERINPUT:
                    pOuterInputInitNode = (OuterInput_Init_Node_Type *)pCurInitNode->pTuyuan;
                    if (pOuterInputInitNode->ucSignalSourceType == DIGITAL_INPUT_SOURCE)
                    {
                        pCurPartGrpAttribArr->DIScanNodeNum++;
                    }
                    else if (pOuterInputInitNode->ucSignalSourceType == ANALOG_INPUT_SOURCE)
                    {
                        if (pOuterInputInitNode->ucSignalValueType == FLOAT_VALUE_TYPE)
                        {
                            pCurPartGrpAttribArr->FloatAIScanNodeNum++;
                        }
                        else if (pOuterInputInitNode->ucSignalValueType == COMPLEX_VALUE_TYPE)
                        {
                            pCurPartGrpAttribArr->CmplxAIScanNodeNum++;
                        }
                    }

                    RtResult=RE_OuterInputCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_OUTEROUTPUT:
                    pOuterOutputInitNode = (OuterOutput_Init_Node_Type *)pCurInitNode->pTuyuan;
                    if (pOuterOutputInitNode->ucSignalDestType == DIGITAL_OUTPUT_DEST)
                    {
                        pCurPartGrpAttribArr->DOScanNodeNum++;
                    }
                    else if (pOuterOutputInitNode->ucSignalDestType == HINTLAMP_OUTPUT_DEST)
                    {
                        pCurPartGrpAttribArr->LampScanNodeNum++;
                    }
                    RtResult=RE_OuterOutputCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_LUBO:
                    pLuboInitNode = (Lubo_Init_Node_Type *)pCurInitNode->pTuyuan;
                    if (pLuboInitNode->ucLuboStartStopType == LUBO_ONLY_START)
                    {
                        pCurPartGrpAttribArr->OnlyStartLuboScanNodeNum++;
                    }
                    else if (pLuboInitNode->ucLuboStartStopType == LUBO_ONLY_STOP)
                    {
                        pCurPartGrpAttribArr->OnlyStopLuboScanNodeNum++;
                    }
                    else if (pLuboInitNode->ucLuboStartStopType == LUBO_STARTSTOP)
                    {
                        pCurPartGrpAttribArr->StartStopLuboScanNodeNum++;
                    }
                    RtResult=RE_LuboCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_EVENT:
                    pCurPartGrpAttribArr->EventScanNodeNum++;
                    RtResult=RE_EventCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_MULTIWAYSELECT:
                    pCurPartGrpAttribArr->mwNum++;
                    RtResult=RE_MultiwaySelectCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_IMPORT:
                    pCurPartGrpAttribArr->externImportNum++;
                    RtResult=RE_ImportCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;

                case  RE_EXPORT:
                    pCurPartGrpAttribArr->externExportNum++;
                    RtResult=RE_ExportCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_SETTING:
                    pSettingInitNode = (Setting_Init_Node_Type *)pCurInitNode->pTuyuan;
                    if (pSettingInitNode->ucSignalSourceType == DINGZHI_INPUT_SOURCE)
                    {
                        pCurPartGrpAttribArr->SetScanNodeNum++;
                    }
                    else if (pSettingInitNode->ucSignalSourceType == ANNLOG_CONSTVALUE_SOURCE)
                    {
                        pCurPartGrpAttribArr->constSource++;
                    }

                    RtResult=RE_SettingCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_RELAYSTART:
                    pCurPartGrpAttribArr->rltNum++;
                    RtResult=RE_RelayStartCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_REPORTSTART:
                    pCurPartGrpAttribArr->sptNum++;
                    RtResult=RE_ReportStartCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_DIMOD:
                    pCurPartGrpAttribArr->DIModScanNodeNum++;
                    RtResult=RE_DIModCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;

                case  RE_AISET:
                    pAISetInitNode = (AISet_Init_Node_Type *)pCurInitNode->pTuyuan;
                    if (pAISetInitNode->ucSignalValueType == FLOAT_VALUE_TYPE)
                    {
                        pCurPartGrpAttribArr->FloatAISetScanNodeNum++;
                    }
                    else if (pAISetInitNode->ucSignalValueType == COMPLEX_VALUE_TYPE)
                    {
                        pCurPartGrpAttribArr->CmplxAISetScanNodeNum++;
                    }

                    RtResult=RE_AISetCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_DISET:
                    pCurPartGrpAttribArr->DISetScanNodeNum++;
                    RtResult=RE_DISetCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_DOSET:
                    pCurPartGrpAttribArr->DOSetScanNodeNum++;
                    RtResult=RE_DOSetCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;

                case  RE_AMPANG:
                    pCurPartGrpAttribArr->apNum++;
                    RtResult=RE_AmpAngCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_REALIMAGE:
                    pCurPartGrpAttribArr->riNum++;
                    RtResult=RE_RealImageCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_MAX:
                    pCurPartGrpAttribArr->maxNum++;
                    RtResult=RE_MaxCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;

                case  RE_MIN:
                    pCurPartGrpAttribArr->minNum++;
                    RtResult=RE_MinCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_MODEWORD:
                    pCurPartGrpAttribArr->mwdNum++;
                    RtResult=RE_ModeWordCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case  RE_OUTERORDER:
                    pCurPartGrpAttribArr->owNum++;
                    RtResult=RE_OuterOrderCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                case RE_SYSSERVOUT:
                    pCurPartGrpAttribArr->soNum++;
                    RtResult=RE_SysServerCreateScanNodeInit(
                                 &pCurScanNode,pCurInitNode);
                    if(RtResult!=EP_SUCCESS)
                    {
                        return  RtResult;
                    }
                    /*添加申请的扫描节点到相应连表中  */
                    RE_LstAdd(pCurPartGrpScanNodeList,pCurScanNode);

                    break;
                default:
                    /* 若是其他图元类型,则出错  */
                    LOG_Dbg_Msg("Read  Tuyuan  Type  Error!\n",0,0,0,0,0,0);
                    assert(FALSE);
                    return  EP_SYS_ERR;

            }


            /* 获得连表的下1个图元 */
            pCurInitNode=RE_LstNext(pCurInitNode);

        }

    }

    return   EP_SUCCESS;

}




/* 在所有初始化接点连表数组中，根据名称来查询获得同名的
    端口输出图元
    参数
          PartGrpInitNodeListArr，初始化节点连表的数组
          nPartGrpCount,初始化连表数组的大小
          strSearchName,代查询的名称
          pRtLastInitOuterOutput,返回的最后一个查找到的相应
                              外部输出图元的初始化节点
                              用于返回测试比较信号类型是否一致
          pnRtFindTuyuanNoInInitListArr，返回的该初始化节点所在的
                              连表在连表数组中的序号，以0开始
          pnRtFindTuyuanNoInInitList，返回的该初始化节点所在的
                              连表中的序号号，以0开始

    返回值  代表查找到的个数

   */

unsigned  long    RE_SearchExternExportDestOuterOutputInInitListArr
(LIST  * PartGrpInitNodeListArr,
 unsigned  long  nPartGrpCount,
 char  *  strSearchName,
 NODE  ** pRtLastInitOuterOutput,
 unsigned long  *pnRtFindTuyuanNoInInitListArr,
 unsigned long  *pnRtFindTuyuanNoInInitList)
{
    unsigned  long  ulFindCount;
    int  i,m;
    LIST  *  pCurInitNodeList;
    int  nPartGrpTuyuanCount;
    unsigned  long  nCurNoInListArr,nCurCounterInListArr;
    unsigned  long  nCurNoInList,nCurCounterInList;
    NODE   *pCurInitNode;
    int  InitNodeType;
    Export_Init_Node_Type   *pExportInitNode;
    char  *  strTuyuanName;
    NODE  *   pFindNode;


    ulFindCount=0;

    pFindNode=NULL;
    nCurNoInListArr=0;
    nCurNoInList=0;

    nCurCounterInListArr=0;/*连表计数  */

    for(i=0; i<nPartGrpCount; i++)
    {
        nCurCounterInList=0;/* 连表内的图元计数 */

        pCurInitNodeList=PartGrpInitNodeListArr+i;
        /* 获得当前连表第1个图元 */
        nPartGrpTuyuanCount=RE_LstCount(pCurInitNodeList);
        pCurInitNode=RE_LstFirst(pCurInitNodeList);
        for(m=0; m<nPartGrpTuyuanCount; m++)
        {
            /* 查找每个图元   */
            InitNodeType=pCurInitNode->ulTuyuanType;
            if(InitNodeType==RE_EXPORT)
            {
                /* 若是外部输出图元 */
                int  nCmpResult;
                pExportInitNode=(Export_Init_Node_Type   *)
                                pCurInitNode->pTuyuan;
                strTuyuanName=pExportInitNode->
                              strOutputDestID;
                nCmpResult=strcmp(strSearchName,strTuyuanName);
                if(nCmpResult==0)
                {
                    /*若名称相同，则保存这时的节点和位置信息，计数个数  */
                    pFindNode=(NODE  *)pCurInitNode;
                    nCurNoInListArr=nCurCounterInListArr;
                    nCurNoInList=nCurCounterInList;
                    ulFindCount++;
                }
            }
            /* 获得连表的下1个图元 */
            pCurInitNode=RE_LstNext(pCurInitNode);
            nCurCounterInList++;
        }

        nCurCounterInListArr++;
    }

    *pRtLastInitOuterOutput=pFindNode;
    *pnRtFindTuyuanNoInInitListArr=nCurNoInListArr;
    *pnRtFindTuyuanNoInInitList=nCurNoInList;
    return  ulFindCount;


}




/* 在所有扫描接点连表数组中，根据图元的位置查找的
    端口输出图元
    参数
          PartGrpScanNodeListArr，扫描节点连表的数组
          nPartGrpCount,扫描节点连表数组的大小
          nFindTuyuanNoInScanListArr，该扫描节点所在的
                              连表在连表数组中的序号，以0开始
          nFindTuyuanNoInScanList，该扫描节点所在的
                              连表中的序号号，以0开始

    返回值  返回找到的扫描节点，若失败，则返回NULL

   */

NODE  *    RE_SearchExternExportDestOuterOutputInScanListArr
(LIST  * PartGrpScanNodeListArr,
 unsigned  long  nPartGrpCount,
 unsigned long  nFindTuyuanNoInScanListArr,
 unsigned long  nFindTuyuanNoInScanList)
{
    NODE  *  pSearchScanNode;
    LIST  *  pCurScanNodeList;
    int  nPartGrpTuyuanCount;


    pSearchScanNode=NULL;

    if(nFindTuyuanNoInScanListArr>=nPartGrpCount)
    {
        return  NULL;
    }
    pCurScanNodeList=PartGrpScanNodeListArr+
                     nFindTuyuanNoInScanListArr;
    nPartGrpTuyuanCount=RE_LstCount(pCurScanNodeList);
    if(nFindTuyuanNoInScanList>=nPartGrpTuyuanCount)
    {
        return  NULL;

    }

    pSearchScanNode=RE_LstNth (pCurScanNodeList,(nFindTuyuanNoInScanList+1));
    if(pSearchScanNode==NULL)
    {
        return  NULL;

    }
    if(pSearchScanNode->ulTuyuanType!=
            RE_EXTERN_EXPORT_OUTEROUTPUT_SCAN)
    {
        return  NULL;

    }

    return   pSearchScanNode;

}




/*     此函数初始化匹配所有分图的所有端口引入目的地类型的
       外部输入的相应来源,
       此时所有初始化节点和扫描节点都已创建,并已进行完初步的创建
        初始化 ,但尚未开始扫描节点的输入来源地址等操作

        参数  nPartGrpScanNodeListArr,所有分图的图元扫描节点的连表
               nPartGrpInitNodeListArr,所有分图的图元初始化节点的连表
              nPartGrpSum ,分图个数,也是可对连表数组操作的维数
        返回值,EP_STATUS ,

*/
EP_STATUS  RE_InitMatchAllExternImportOuterInputTuyuan
(LIST  * PartGrpScanNodeListArr,
 LIST  * PartGrpInitNodeListArr,
 int   nPartGrpSum )
{


    int  i,m;
    LIST  *  pCurInitNodeList;
    LIST  *  pCurScanNodeList;
    int  nPartGrpTuyuanCount;
    NODE   *pCurInitNode;
    NODE    *pCurScanNode;
    int  InitNodeType;
    Import_Init_Node_Type   *pImportInitNode;
    NODE   *pMatchInitNode;
    Export_Init_Node_Type   *pMatchExportInitNode;
    char  *  strTuyuanName;
    NODE  *   pFindNode;
    ExternImportOuterInput_Scan_Node_Type    *pFindInputScanTuyuan;
    ExternExportOuterOutput_Scan_Node_Type   *pFindOutputScanTuyuan;
    EP_ELEM_IO     *pExternExportOutputIO;

    for(i=0; i<nPartGrpSum; i++)
    {
        pCurInitNodeList=PartGrpInitNodeListArr+i;
        pCurScanNodeList=PartGrpScanNodeListArr+i;
        /* 获得当前连表第1个图元 */
        nPartGrpTuyuanCount=RE_LstCount(pCurInitNodeList);
        pCurInitNode=RE_LstFirst(pCurInitNodeList);
        pCurScanNode=RE_LstFirst(pCurScanNodeList);
        for(m=0; m<nPartGrpTuyuanCount; m++)
        {
            /* 查找每个图元   */
            InitNodeType=pCurInitNode->ulTuyuanType;
            if(InitNodeType==RE_IMPORT)
            {
                /* 若是端口引入图元 */
                unsigned  long  ulMatchCount;
                unsigned  long  ulMatchListArrNo;
                unsigned  long  ulMatchListNo;

                pImportInitNode=(Import_Init_Node_Type   *)
                                pCurInitNode->pTuyuan;
                strTuyuanName=pImportInitNode->
                              strInputSourceID;
                ulMatchCount=
                    RE_SearchExternExportDestOuterOutputInInitListArr
                    (PartGrpInitNodeListArr,
                     nPartGrpSum,
                     strTuyuanName,
                     &pMatchInitNode,
                     &ulMatchListArrNo,
                     &ulMatchListNo);

                if(ulMatchCount<1)
                {

                    LOG_Dbg_Msg("Error, Import  Tuyuan by  Name \'%s\'  Can't  Match one Same  Name  Extern Export  Dest  OuterOutput  Tuyuan!\n",
                                (int)strTuyuanName,0,0,0,0,0);
                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:名称为\'%s\'的端口引入图元不能匹配到同名的端口引出图元\n",
                                   (int)strTuyuanName,0);
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp rslv err: \'%s\' import  element  can not match same name export  element\n",
                                   (int)strTuyuanName,0);
                    }
                    assert(FALSE);  /* 若非以上类型,则告警   */

                    return  EP_SYS_ERR;
                }
                else  if(ulMatchCount>1)
                {
                    LOG_Dbg_Msg("Error, Import  Tuyuan by  Name \'%s\'  Match Multi  Same  Name  Extern Export  Dest  OuterOutput  Tuyuan!\n",
                                (int)strTuyuanName,0,0,0,0,0);
                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "逻辑图解析错误:名称为\'%s\'的端口引入图元匹配了多个同名的端口引出图元\n",
                                   (int)strTuyuanName,0);
                    }
                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "logic grp rslv err: \'%s\' import  element  match multi same name export  element\n",
                                   (int)strTuyuanName,0);
                    }
                    assert(FALSE);  /* 若非以上类型,则告警   */

                    return  EP_SYS_ERR;

                }
                else
                {
                    unsigned  char  ucImportAttrib,ucExportAttrib;
                    pMatchExportInitNode=
                        (Export_Init_Node_Type   *)
                        pMatchInitNode->pTuyuan;
                    ucImportAttrib=pImportInitNode->
                                   PublicElemData.elem.aioOut[0].ucAttrib;
                    ucExportAttrib=pMatchExportInitNode->
                                   PublicElemData.elem.aioOut[0].ucAttrib;
                    if(ucImportAttrib!=ucExportAttrib)
                    {
                        /* 若是端口引入和引出属性不一致，则出错 */
                        LOG_Dbg_Msg("Error, Import  Tuyuan by  Name \'%s\' Can't  Match  Same Name  And  Signal Type  Extern Export  Dest  OuterOutput  Tuyuan!\n",
                                    (int)strTuyuanName,0,0,0,0,0);

                        if(ENG_MODE == 0)
                        {
                            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                       ER_REPORT|ER_ALARM|ER_LOCK,
                                       "逻辑图解析错误:名称为\'%s\'的端口引入图元不能匹配同名和相同信号类型的端口引出图元\n",
                                       (int)strTuyuanName,0);
                        }
                        else if(ENG_MODE == 1)
                        {
                            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                       ER_REPORT|ER_ALARM|ER_LOCK,
                                       "logic grp rslv err: \'%s\'import  element can not match  same name and same signal type export  element\n",
                                       (int)strTuyuanName,0);
                        }
                        assert(FALSE);  /* 若非以上类型,则告警   */

                        return  EP_SYS_ERR;

                    }
                    pFindNode=
                        RE_SearchExternExportDestOuterOutputInScanListArr
                        (PartGrpScanNodeListArr,
                         nPartGrpSum,
                         ulMatchListArrNo,
                         ulMatchListNo);
                    if(pFindNode==NULL)
                    {
                        /* 若是搜索相应端口引出扫描节点不成功，则也失败 */

                        LOG_Dbg_Msg("Error,Can't  Find  Export   Tuyuan  Scan  Node  by  Name \'%s\' By  List  Position!\n",
                                    (int)strTuyuanName,0,0,0,0,0);
                        if(ENG_MODE == 0)
                        {
                            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                       ER_REPORT|ER_ALARM|ER_LOCK,
                                       "逻辑图解析错误:在扫描图元链表中不能找到名称为\'%s\'的端口引出图元\n",
                                       (int)strTuyuanName,0);
                        }
                        else if(ENG_MODE == 1)
                        {
                            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                       ER_REPORT|ER_ALARM|ER_LOCK,
                                       "logic grp rslv err: can not find  \'%s\' export element in scan  element list\n",
                                       (int)strTuyuanName,0);
                        }
                        assert(FALSE);  /* 若非以上类型,则告警   */

                        return  EP_SYS_ERR;

                    }

                    if(pCurScanNode->ulTuyuanType!=
                            RE_EXTERN_IMPORT_OUTERINPUT_SCAN)
                    {
                        /* 若是查询的初始化节点对应的扫描节点类型，
                           不是端口引入扫描节点类型，则出错*/
                        LOG_Dbg_Msg("Error,Scan  Node  Type  of  Import  Tuyuan  Scan  Node  by  Name \'%s\' isn't  Expected!\n",
                                    (int)strTuyuanName,0,0,0,0,0);
                        if(ENG_MODE == 0)
                        {
                            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                       ER_REPORT|ER_ALARM|ER_LOCK,
                                       "逻辑图解析错误:图元链表中查找名称为\'%s\'的端口引入图元类型不是所期望的\n",
                                       (int)strTuyuanName,0);
                        }
                        else if(ENG_MODE == 1)
                        {
                            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                       ER_REPORT|ER_ALARM|ER_LOCK,
                                       "logic grp rslv err:  \'%s\' import  element is not expected type in diagram element list\n",
                                       (int)strTuyuanName,0);
                        }
                        assert(FALSE);  /* 若非以上类型,则告警   */

                        return  EP_SYS_ERR;

                    }
                    pFindInputScanTuyuan=(ExternImportOuterInput_Scan_Node_Type    *)
                                         pCurScanNode->pTuyuan;
                    pFindOutputScanTuyuan=(ExternExportOuterOutput_Scan_Node_Type   *)
                                          pFindNode->pTuyuan;
                    pExternExportOutputIO=
                        RE_ExportTuyuanGetOutIO
                        (0,pFindNode);
                    if(pExternExportOutputIO==NULL)
                    {
                        /* 若是查找端口引出图元IO失败 */

                        LOG_Dbg_Msg("Error,Get  Export  Tuyuan  by  Name \'%s\' Output IO Failure!\n",
                                    (int)strTuyuanName,0,0,0,0,0);
                        if(ENG_MODE == 0)
                        {
                            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                       ER_REPORT|ER_ALARM|ER_LOCK,
                                       "逻辑图解析错误:查找名称为\'%s\'的端口引出图元的输出IO失败\n",
                                       (int)strTuyuanName,0);
                        }
                        else if(ENG_MODE == 1)
                        {
                            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                       ER_REPORT|ER_ALARM|ER_LOCK,
                                       "logic grp rslv err:search  \'%s\' export  element's output io failure\n",
                                       (int)strTuyuanName,0);
                        }
                        assert(FALSE);  /* 若非以上类型,则告警   */

                        return  EP_SYS_ERR;

                    }
                    /*  成功设置该端口引入图元的IO指针*/
                    RE_ImportTuyuanSetExternImportOutputPt
                    (pExternExportOutputIO,pFindInputScanTuyuan);

                    /*2011-7-27  ZY 对Import集中后添加集中后的Export输出指针 */
                    RE_ImportTuyuanSetCollectOutputPt
                    (RE_ExportTuyuanGetCollectOut(0,pFindNode),
                     pFindInputScanTuyuan);
                }/*搜索相应端口引出图元成功处理结束 */

            }/* 若查询图元是端口引入图元类型处理结束 */

            /* 获得连表的下1个图元 */
            pCurInitNode=RE_LstNext(pCurInitNode);
            pCurScanNode=RE_LstNext(pCurScanNode);
        }/* 对分图的每个图元进行查询 */

    }/* 对所有分图进行查询 */

    return  EP_SUCCESS;
}



/* 获得扫描节点的GET输出IO的指针的函数指针
      参数   pScanNode，待查询的扫描节点
      返回值  该节点的获得输出IO的函数指针
              ，返回为NULL，表示失败
 */
GET_OUT_IO_FUNC_TYPE   RE_GetScanNodeGetOutIOFunc(NODE  *pScanNode)
{
    GET_OUT_IO_FUNC_TYPE   returnFunc;

    switch(pScanNode->ulTuyuanType)
    {

        case    RE_SUANFA_SCAN:
            /* 算法 */
            returnFunc=((Suanfa_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_2_AND_SCAN:
            /* 与门 */
            returnFunc=((And2_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_3_AND_SCAN:
            returnFunc=((And3_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_4_AND_SCAN:

            returnFunc=((And4_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_5_AND_SCAN:

            returnFunc=((And5_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case   RE_6_AND_SCAN:

            returnFunc=((And6_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;
            break;

        case   RE_7_AND_SCAN:

            returnFunc=((And7_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_8_AND_SCAN:

            returnFunc=((And8_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case   RE_MULTI_AND_SCAN:

            returnFunc=((AndMulti_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_2_OR_SCAN:
            /*或门  */

            returnFunc=((Or2_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_3_OR_SCAN:

            returnFunc=((Or3_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_4_OR_SCAN:

            returnFunc=((Or4_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_5_OR_SCAN:

            returnFunc=((Or5_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case   RE_6_OR_SCAN:

            returnFunc=((Or6_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_7_OR_SCAN:

            returnFunc=((Or7_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_8_OR_SCAN:

            returnFunc=((Or8_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case   RE_MULTI_OR_SCAN:

            returnFunc=((OrMulti_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_NOT_SCAN:
            /* 非门 */

            returnFunc=((Not_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;


        case    RE_TIMER_SCAN:
            /* 时间继电器 */

            returnFunc=((Timer_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_YABAN_SCAN:
            /* 压板 */

            returnFunc=(( Yaban_Scan_Node_Type *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case   RE_CONTROLWORD_SCAN:
            /* 控制字 */

            returnFunc=((ControlWord_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;


        case    RE_ZERO_SCAN:
            /* 恒0  */

            returnFunc=(( ConstZero_Scan_Node_Type *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case     RE_ONE_SCAN:
            /* 恒1  */

            returnFunc=((ConstOne_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_FLOAT_GREATERTHAN_SCAN:
            /* 大于比较  */

            returnFunc=(( GreaterThan_Scan_Node_Type *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_UNSIGNED_32INT_GREATERTHAN_SCAN:

            returnFunc=(( GreaterThan_Scan_Node_Type *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_SIGNED_32INT_GREATERTHAN_SCAN:

            returnFunc=((GreaterThan_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_FLOAT_LESSTHAN_SCAN:
            /* 小于比较  */

            returnFunc=((LessThan_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_UNSIGNED_32INT_LESSTHAN_SCAN:

            returnFunc=(( LessThan_Scan_Node_Type *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_SIGNED_32INT_LESSTHAN_SCAN:

            returnFunc=(( LessThan_Scan_Node_Type *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case     RE_FLOAT_EQUAL_SCAN:
            /* 等于比较 */

            returnFunc=((Equal_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case     RE_UNSIGNED_32INT_EQUAL_SCAN:

            returnFunc=((Equal_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case     RE_SIGNED_32INT_EQUAL_SCAN:

            returnFunc=(( Equal_Scan_Node_Type *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_FLOAT_AI_OUTERINPUT_SCAN:
            /* 外部输入 */

            returnFunc=((FloatAIOuterInput_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_COMPLEX_AI_OUTERINPUT_SCAN:

            returnFunc=((ComplexAIOuterInput_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_MEA_AI_OUTERINPUT_SCAN:

            returnFunc=((MeaAIOuterInput_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case     RE_DI_OUTERINPUT_SCAN:

            returnFunc=((DIOuterInput_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_PO_OUTERINPUT_SCAN:

            returnFunc=((PulseOutput_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_AI_PLUSCOF_OUTERINPUT_SCAN:

            returnFunc=((AIPlusCofInput_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_AI_OFFCOF_OUTERINPUT_SCAN:

            returnFunc=((AIOffCofInput_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_AI_PROCOF_OUTERINPUT_SCAN:

            returnFunc=((AIProCofInput_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_CL_PLUSCOF_OUTERINPUT_SCAN:

            returnFunc=((ClPlusCofInput_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_CL_OFFCOF_OUTERINPUT_SCAN:

            returnFunc=((ClOffCofInput_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case     RE_DINGZHI_OUTERINPUT_SCAN:

            returnFunc=((DingzhiOuterInput_Scan_Node_Type *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case     RE_CONSTVALUE_OUTERINPUT_SCAN:

            returnFunc=((ConstValueOuterInput_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case     RE_EXTERN_IMPORT_OUTERINPUT_SCAN:

            returnFunc=((ExternImportOuterInput_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_2WAY_MULTIWAYSELECT_SCAN:
            /* 多路选通  */

            returnFunc=((MultiwaySelect2_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_3WAY_MULTIWAYSELECT_SCAN:

            returnFunc=((MultiwaySelect3_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_MULTIWAY_MULTIWAYSELECT_SCAN:

            returnFunc=((MultiWay_MultiwaySelect_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case   RE_DO_OUTEROUTPUT_SCAN:
            /*  外部输出 */

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  DO  OuterOutput  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;

        case     RE_LAMP_OUTEROUTPUT_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because Lamp OuterOutput  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;


        case     RE_TRIP_ENABLE_OUTEROUTPUT_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  RelayStart  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;

        case     RE_EXTERN_EXPORT_OUTEROUTPUT_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  Export  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;

        case     RE_FLOAT_AI_CH_OUTEROUTPUT_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  FloatAI Channel  OuterOutput Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;

        case     RE_COMPLEX_AI_CH_OUTEROUTPUT_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  ComplexAI  Channel  OuterOutput Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;
        case     RE_OPTAO_OUTEROUTPUT_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  AO  OuterOutput Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;
        case   RE_AIPLUSADT_OUTEROUTPUT_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  AIPlusCof Adjust OuterOutput  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;

        case   RE_AIOFFADT_OUTEROUTPUT_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  AIOffCof Adjust  OuterOutput  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;
        case   RE_CLPLUSADT_OUTEROUTPUT_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  ClPlusCof Adjust  OuterOutput  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;

        case   RE_CLOFFADT_OUTEROUTPUT_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  ClOffCof Adjust  OuterOutput  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;
        case   RE_DIFILTADT_OUTEROUTPUT_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  DIFiltTime Adjust  OuterOutput  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;

        case   RE_YCOVERADT_OUTEROUTPUT_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  YcOverCof Adjust  OuterOutput  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;
        case   RE_CLOVERADT_OUTEROUTPUT_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  ClOverCof Adjust  OuterOutput  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;

        case   RE_PO_OUTEROUTPUT_SCAN:
            /*  外部输出 */

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because PO  OuterOutput  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;
        case      RE_ONLYSTART_LUBO_SCAN:
            /* 录波启停 */

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  OnlyStart  Lubo  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;

        case      RE_ONLYSTOP_LUBO_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  OnlyStop  Lubo Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;

        case     RE_STARTSTOP_LUBO_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  StartStop  Lubo  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;

        case    RE_EVENT_SCAN:
            /*  事件 */

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  Event  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;

        case    RE_REPORT_ENABLE_OUTEROUTPUT_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  ReportStart  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;
        case    RE_DI_MOD_SCAN:

            returnFunc=((DIMod_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_DI_SET_SCAN:

            returnFunc=((DISet_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_FLOAT_AI_SET_SCAN:

            returnFunc=((AISet_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_CMPLX_AI_SET_SCAN:

            returnFunc=((AISet_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_DO_SET_SCAN:

            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  DOSet  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;

        case    RE_AMPANG_SCAN:

            returnFunc=((AmpAng_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_REALIMAGE_SCAN:

            returnFunc=((RealImage_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_FLOAT_MAX_SCAN:

            returnFunc=((Max_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_UNSIGNED_32INT_MAX_SCAN:

            returnFunc=((Max_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_SIGNED_32INT_MAX_SCAN:

            returnFunc=((Max_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_FLOAT_MIN_SCAN:

            returnFunc=((Min_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_UNSIGNED_32INT_MIN_SCAN:

            returnFunc=((Min_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_SIGNED_32INT_MIN_SCAN:

            returnFunc=((Min_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_MODEWORD_SCAN:

            returnFunc=((ModeWord_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_MEADO_OUTERORDER_SCAN:

            returnFunc=((MeaDOOuterOrder_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_PLUSADJUST_OUTERORDER_SCAN:

            returnFunc=((PlusAdtOuterOrder_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_OFFADJUST_OUTERORDER_SCAN:

            returnFunc=((OffAdtOuterOrder_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_POCLEAR_OUTERORDER_SCAN:

            returnFunc=((PulseClearOuterOrder_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case RE_ZHUBIAN_OUTERORDER_SCAN:
            returnFunc=((SwitchZhuBianOuterOrder_Scan_Node_Type *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case RE_JINXIAN_OUTERORDER_SCAN:
            returnFunc=((SwitchJinXianOuterOrder_Scan_Node_Type *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_FARSTSCHG_OUTERORDER_SCAN:		/* 获得输出指针，返回数据源 */

            returnFunc=((FarChgOuterOrder_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;
        case    RE_YXJXCHG_OUTERORDER_SCAN:		/* 获得输出指针，返回数据源 */

            returnFunc=((YXJXChgOuterOrder_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;

        case    RE_JGSCHG_OUTERORDER_SCAN:		/* 获得输出指针，返回数据源 */

            returnFunc=((JGSChgOuterOrder_Scan_Node_Type  *)pScanNode->pTuyuan)
                       ->pfGetScanNodeOutIOFunc;

            break;


        case RE_SETTINGSWITCH_SYSSERV_SCAN:
            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  SettingSwitch  OuterOrder  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;
        case RE_REVERT_SYSSERV_SCAN:
            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  Revert  OuterOrder  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;
        case RE_SWITCHFAR_SYSSERV_SCAN:
            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  SwitchFar  OuterOrder  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;
        case RE_SWITCHEXAM_SYSSERV_SCAN:
            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  SwitchExam  OuterOrder  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;
        case RE_THREEU0WARN_SYSSERV_SCAN:
            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  ThreeU0Warn  OuterOrder  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

        case RE_PLUSADTOVER_SYSSERV_SCAN:
            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  PlusAdt  OuterOrder  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

        case RE_OFFADTOVER_SYSSERV_SCAN:
            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  OffAdt  OuterOrder  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

        case RE_YABANTT_SYSSERV_SCAN:
            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  YabanTT  OuterOrder  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

        case RE_SETAUTOSET_SYSSERV_SCAN:		/* 自动整定定值 */
            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  SetAutoSet  OuterOrder  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

        case RE_DBYX_SYSSERV_SCAN:		/* 遥信 */
            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because  DBYX  OuterOrder  Tuyuan    no  OutputIO !\n",0,0,0,0,0,0);
            assert(FALSE);

            break;
        default:
            returnFunc=NULL;
            LOG_Dbg_Msg("Error,Get Scan Node GetOutIOFunc  failure \n because Scan  Tuyuan  Type  isn't  Expected!\n",0,0,0,0,0,0);
            assert(FALSE);
            break;
    }
    if(returnFunc==NULL)
    {

        LOG_Dbg_Msg("Error,ReturnGetIOFunc is Null\n",0,0,0,0,0,0);

    }
    return   returnFunc;
}



/* 获得初始化节点的初始化扫描节点InitScanFunc的函数指针
      参数   pInitNode，待查询的初始化节点
      返回值  该初始化节点的InitScanFunc的函数指针
              ，返回为NULL，表示失败
*/
INIT_SCAN_FUNC_TYPE   RE_GetInitNodeInitScanFunc(NODE  *pInitNode)
{
    INIT_SCAN_FUNC_TYPE    pfRtFunc=NULL;

    switch(pInitNode->ulTuyuanType)
    {
        /* 根据不同的图元类型,分别根据图元初始化节点的内容,申请扫描节点  */
        case  RE_SUANFA:
            pfRtFunc=((Suanfa_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case RE_AND :

            pfRtFunc=(( And_Init_Node_Type *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case RE_OR :

            pfRtFunc=(( Or_Init_Node_Type *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_NOT:

            pfRtFunc=(( Not_Init_Node_Type *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case RE_TIMER :

            pfRtFunc=((Timer_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_YABAN:

            pfRtFunc=((Yaban_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_CONTROLWORD:

            pfRtFunc=((ControlWord_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_ZERO:

            pfRtFunc=(( ConstZero_Init_Node_Type *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_ONE:

            pfRtFunc=(( ConstOne_Init_Node_Type *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_GREATERTHAN:

            pfRtFunc=(( GreaterThan_Init_Node_Type *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_LESSTHAN:

            pfRtFunc=(( LessThan_Init_Node_Type *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_EQUAL:

            pfRtFunc=(( Equal_Init_Node_Type *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_OUTERINPUT:

            pfRtFunc=(( OuterInput_Init_Node_Type *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_OUTEROUTPUT:

            pfRtFunc=(( OuterOutput_Init_Node_Type *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_LUBO:

            pfRtFunc=(( Lubo_Init_Node_Type *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_EVENT:

            pfRtFunc=((Event_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_MULTIWAYSELECT:

            pfRtFunc=((MultiwaySelect_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_IMPORT:

            pfRtFunc=((Import_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_EXPORT:

            pfRtFunc=((Export_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_SETTING:

            pfRtFunc=((Setting_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_RELAYSTART:

            pfRtFunc=((RelayStart_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_REPORTSTART:

            pfRtFunc=((ReportStart_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_DIMOD:

            pfRtFunc=((DIMod_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_DISET:

            pfRtFunc=((DISet_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_DOSET:

            pfRtFunc=((DOSet_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_AISET:

            pfRtFunc=((AISet_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_AMPANG:

            pfRtFunc=((AmpAng_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_REALIMAGE:

            pfRtFunc=((RealImage_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_MAX:

            pfRtFunc=((Max_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_MIN:

            pfRtFunc=((Min_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_MODEWORD:

            pfRtFunc=((ModeWord_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case  RE_OUTERORDER:
            pfRtFunc=((OuterOrder_Init_Node_Type  *)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        case RE_SYSSERVOUT:
            pfRtFunc=((SysServer_Init_Node_Type*)pInitNode->pTuyuan)->PublicElemData.pfInitScanFunc;
            break;
        default:
            /* 若是其他图元类型,则出错  */
            LOG_Dbg_Msg(" Init  Tuyuan  Type  Error!\n",0,0,0,0,0,0);
            assert(FALSE);
            pfRtFunc=NULL;
            break;
    }
    return   pfRtFunc;

}

/* 对扫描链表节点分类.
 * Para:
 *     PartGrpScanNodeListArr, 扫描节点链表.
 *     PartGrpAttrib, 分图属性.
 *     nPartGrpSum, 分图个数.
 * Return:
 *     EP_STATUS, or EP_ERROR.
 */
EP_STATUS RE_ClassScanNodeInit(LIST *PartGrpScanNodeListArr,
                               PARTGRP_ATTRIB_TYPE *PartGrpAttrib, int nPartGrpSum)
{
    int i;
    NODE *pCurScanNode;
    LIST *pCurPartGrpScanNodeList;
    PARTGRP_ATTRIB_TYPE *pCurPartGrpAttrib;
    NODE **ppCurSetNode; /* 定值 */
    NODE **ppCurDINode;   /* 开入 */
    NODE **ppCurDIModNode;   /* 开入模件 */
    NODE **ppCurDISetNode; /* 开入集 */
    NODE **ppCurFloatAINode;  /* 实数 */
    NODE **ppCurCmplxAINode;    /* 复数 */
    NODE **ppCurFloatAISetNode; /* 实数集 */
    NODE **ppCurCmplxAISetNode; /* 复数集 */
    NODE **ppCurEventNode;  /* 事件 */
    NODE **ppCurDONode;   /* 开出 */
    NODE **ppCurDOSetNode;   /* 开出集 */
    NODE **ppCurLampNode;  /* 信号灯 */
    NODE **ppCurOnlyStartLuboNode;   /* 启动录波 */
    NODE **ppCurOnlyStopLuboNode;  /* 停止录波 */
    NODE **ppCurStartStopLuboNode;    /* 启停录波 */

    /* 对任务中的各种图元集中功能进行操作计数清零 2011-7-27  ZY*/
    for(i=0; i<MAX_CREATE_RELAYFUNC_TASK_COUNT ; i++)
    {
        RE_DOCollectClearSeqNo(i);
        RE_LampCollectClearSeqNo(i);
        RE_EventCollectClearSeqNo(i);
        RE_ExportCollectClearSeqNo(i);
        RE_FloatAICollectClearSeqNo(i);
        RE_CmplxAICollectClearSeqNo(i);
        RE_DICollectClearSeqNo(i);
        RE_ImportCollectClearSeqNo(i);
    }
    pCurPartGrpScanNodeList = PartGrpScanNodeListArr;
    pCurPartGrpAttrib = PartGrpAttrib;

    for (i = 0; i<nPartGrpSum;
            i++, pCurPartGrpScanNodeList++, pCurPartGrpAttrib++)
    {
        pCurScanNode = RE_LstFirst(pCurPartGrpScanNodeList);

        pCurPartGrpAttrib->ppSetNode = calloc(pCurPartGrpAttrib->SetScanNodeNum,
                                              sizeof(*pCurPartGrpAttrib->ppSetNode));
        if (pCurPartGrpAttrib->ppSetNode == NULL)
        {
            return EP_ERROR;
        }
        ppCurSetNode = pCurPartGrpAttrib->ppSetNode;

        pCurPartGrpAttrib->ppDINode = calloc(pCurPartGrpAttrib->DIScanNodeNum,
                                             sizeof(*pCurPartGrpAttrib->ppDINode));
        if (pCurPartGrpAttrib->ppDINode == NULL)
        {
            return EP_ERROR;
        }
        ppCurDINode = pCurPartGrpAttrib->ppDINode;

        pCurPartGrpAttrib->ppDIModNode = calloc(pCurPartGrpAttrib->DIModScanNodeNum,
                                                sizeof(*pCurPartGrpAttrib->ppDIModNode));
        if (pCurPartGrpAttrib->ppDIModNode == NULL)
        {
            return EP_ERROR;
        }
        ppCurDIModNode = pCurPartGrpAttrib->ppDIModNode;

        pCurPartGrpAttrib->ppDISetNode = calloc(pCurPartGrpAttrib->DISetScanNodeNum,
                                                sizeof(*pCurPartGrpAttrib->ppDISetNode));
        if (pCurPartGrpAttrib->ppDISetNode == NULL)
        {
            return EP_ERROR;
        }
        ppCurDISetNode = pCurPartGrpAttrib->ppDISetNode;

        pCurPartGrpAttrib->ppFloatAINode = calloc(pCurPartGrpAttrib->FloatAIScanNodeNum,
                                           sizeof(*pCurPartGrpAttrib->ppFloatAINode));
        if (pCurPartGrpAttrib->ppFloatAINode == NULL)
        {
            return EP_ERROR;
        }
        ppCurFloatAINode = pCurPartGrpAttrib->ppFloatAINode;

        pCurPartGrpAttrib->ppCmplxAINode = calloc(pCurPartGrpAttrib->CmplxAIScanNodeNum,
                                           sizeof(*pCurPartGrpAttrib->ppCmplxAINode));
        if (pCurPartGrpAttrib->ppCmplxAINode == NULL)
        {
            return EP_ERROR;
        }
        ppCurCmplxAINode = pCurPartGrpAttrib->ppCmplxAINode;

        pCurPartGrpAttrib->ppFloatAISetNode = calloc(pCurPartGrpAttrib->FloatAISetScanNodeNum,
                                              sizeof(*pCurPartGrpAttrib->ppFloatAISetNode));
        if (pCurPartGrpAttrib->ppFloatAISetNode == NULL)
        {
            return EP_ERROR;
        }
        ppCurFloatAISetNode = pCurPartGrpAttrib->ppFloatAISetNode;

        pCurPartGrpAttrib->ppCmplxAISetNode = calloc(pCurPartGrpAttrib->CmplxAISetScanNodeNum,
                                              sizeof(*pCurPartGrpAttrib->ppCmplxAISetNode));
        if (pCurPartGrpAttrib->ppCmplxAISetNode == NULL)
        {
            return EP_ERROR;
        }
        ppCurCmplxAISetNode = pCurPartGrpAttrib->ppCmplxAISetNode;

        pCurPartGrpAttrib->ppEventNode = calloc(pCurPartGrpAttrib->EventScanNodeNum,
                                                sizeof(*pCurPartGrpAttrib->ppEventNode));
        if (pCurPartGrpAttrib->ppEventNode == NULL)
        {
            return EP_ERROR;
        }
        ppCurEventNode = pCurPartGrpAttrib->ppEventNode;

        pCurPartGrpAttrib->ppDONode = calloc(pCurPartGrpAttrib->DOScanNodeNum,
                                             sizeof(*pCurPartGrpAttrib->ppDONode));
        if (pCurPartGrpAttrib->ppDONode == NULL)
        {
            return EP_ERROR;
        }
        ppCurDONode = pCurPartGrpAttrib->ppDONode;

        pCurPartGrpAttrib->ppDOSetNode = calloc(pCurPartGrpAttrib->DOSetScanNodeNum,
                                                sizeof(*pCurPartGrpAttrib->ppDOSetNode));
        if (pCurPartGrpAttrib->ppDOSetNode == NULL)
        {
            return EP_ERROR;
        }
        ppCurDOSetNode = pCurPartGrpAttrib->ppDOSetNode;

        pCurPartGrpAttrib->ppLampNode = calloc(pCurPartGrpAttrib->LampScanNodeNum,
                                               sizeof(*pCurPartGrpAttrib->ppLampNode));
        if (pCurPartGrpAttrib->ppLampNode == NULL)
        {
            return EP_ERROR;
        }
        ppCurLampNode = pCurPartGrpAttrib->ppLampNode;

        pCurPartGrpAttrib->ppOnlyStartLuboNode = calloc(pCurPartGrpAttrib->OnlyStartLuboScanNodeNum,
                sizeof(*pCurPartGrpAttrib->ppOnlyStartLuboNode));
        if (pCurPartGrpAttrib->ppOnlyStartLuboNode == NULL)
        {
            return EP_ERROR;
        }
        ppCurOnlyStartLuboNode = pCurPartGrpAttrib->ppOnlyStartLuboNode;

        pCurPartGrpAttrib->ppOnlyStopLuboNode = calloc(pCurPartGrpAttrib->OnlyStopLuboScanNodeNum,
                                                sizeof(*pCurPartGrpAttrib->ppOnlyStopLuboNode));
        if (pCurPartGrpAttrib->ppOnlyStopLuboNode == NULL)
        {
            return EP_ERROR;
        }
        ppCurOnlyStopLuboNode = pCurPartGrpAttrib->ppOnlyStopLuboNode;

        pCurPartGrpAttrib->ppStratStopLuboNode = calloc(pCurPartGrpAttrib->StartStopLuboScanNodeNum,
                sizeof(*pCurPartGrpAttrib->ppStratStopLuboNode));
        if (pCurPartGrpAttrib->ppStratStopLuboNode == NULL)
        {
            return EP_ERROR;
        }
        ppCurStartStopLuboNode = pCurPartGrpAttrib->ppStratStopLuboNode;

        for ( ; ; )
        {
            if (pCurScanNode == NULL)
            {
                /* 若到图元链表的尾部, 则跳出链表操作 */
                break;
            }

            switch(pCurScanNode->ulTuyuanType)
            {
                case RE_DINGZHI_OUTERINPUT_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurSetNode = pCurScanNode;
                    ppCurSetNode++;
                    break;

                case RE_DI_OUTERINPUT_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurDINode = pCurScanNode;
                    ppCurDINode++;
                    /*2011-7-27  ZY  */
                    RE_DICollectAddByDITuyuan(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_DI_MOD_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurDIModNode = pCurScanNode;
                    ppCurDIModNode++;
                    break;

                case RE_DI_SET_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurDISetNode = pCurScanNode;
                    ppCurDISetNode++;
                    /*2011-7-27  ZY  */
                    RE_DICollectAddByDISetTuyuan(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_FLOAT_AI_OUTERINPUT_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurFloatAINode = pCurScanNode;
                    ppCurFloatAINode++;
                    /*2011-7-27  ZY  */
                    RE_FloatAICollectAddByAITuyuan(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_COMPLEX_AI_OUTERINPUT_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurCmplxAINode = pCurScanNode;
                    ppCurCmplxAINode++;
                    /*2011-7-27  ZY  */
                    RE_CmplxAICollectAddByAITuyuan(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_FLOAT_AI_SET_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurFloatAISetNode = pCurScanNode;
                    ppCurFloatAISetNode++;
                    /*2011-7-27  ZY  */
                    RE_FloatAICollectAddByAISetTuyuan(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_CMPLX_AI_SET_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurCmplxAISetNode = pCurScanNode;
                    ppCurCmplxAISetNode++;
                    /*2011-7-27  ZY  */
                    RE_CmplxAICollectAddByAISetTuyuan(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_EVENT_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurEventNode = pCurScanNode;
                    ppCurEventNode++;
                    /*2011-7-27  ZY  */
                    RE_EventCollectAddByEventTuyuan(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_DO_OUTEROUTPUT_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurDONode = pCurScanNode;
                    ppCurDONode++;
                    /*2011-7-27  ZY  */
                    RE_DOCollectAddByDOTuyuan(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_DO_SET_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurDOSetNode = pCurScanNode;
                    ppCurDOSetNode++;
                    /*2011-7-27  ZY  */
                    RE_DOCollectAddByDOSetTuyuan(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_LAMP_OUTEROUTPUT_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurLampNode = pCurScanNode;
                    ppCurLampNode++;
                    /*2011-7-27  ZY  */
                    RE_LampCollectAddByLampTuyuan(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_ONLYSTART_LUBO_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurOnlyStartLuboNode = pCurScanNode;
                    ppCurOnlyStartLuboNode++;
                    break;

                case RE_ONLYSTOP_LUBO_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurOnlyStopLuboNode = pCurScanNode;
                    ppCurOnlyStopLuboNode++;
                    break;

                case RE_STARTSTOP_LUBO_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    *ppCurStartStopLuboNode = pCurScanNode;
                    ppCurStartStopLuboNode++;
                    break;
                /*2011-7-27  */
                case  RE_EXTERN_EXPORT_OUTEROUTPUT_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    RE_ExportCollectAddByExportTuyuan(pCurPartGrpAttrib,pCurScanNode);
                    break;
                /*2011-7-27  ZY 注意在这里才在连表中删除掉 */
                case  RE_EXTERN_IMPORT_OUTERINPUT_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    RE_ImportCollectAddByImportTuyuan(pCurPartGrpAttrib,pCurScanNode);
                    break;

                /* 删除常量/恒0/恒1图元 */
                case RE_CONSTVALUE_OUTERINPUT_SCAN:
                case RE_ZERO_SCAN:
                case RE_ONE_SCAN:
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);
                    break;
            }
            pCurScanNode = RE_LstNext(pCurScanNode);
        }
    }

    return EP_SUCCESS;
}