/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_RelayEngineEntry.c                                    1.0           */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该源文件实现了保护功能模块中的保护引擎驱动的接口函数和其他函数      */
/*       该接口函数由保护功能模块实现                                        */
/*       在整个系统进行完必要的初始化后,由相应模块调用该接口函数,            */
/*       驱动保护引擎功能运行                                                */

/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                    */
/*                                                                              */
/*         张云       2002.12.2             创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/


#include <vxWorks.h>
#include <logLib.h>

#include   "math_compat.h"

#include  "RE_RelayEngine.h"
#include  "RE_RelayEngineEntry.h"
#include   "RE_PublicDataDef.h"
#include   "datetime.h"
#include   "realdata.h"
#include   "RE_LoadLogrpFile.h"
#include   "string_compat.h"

#include   "taskLib.h"
#include   "intLib.h"

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


#include "OPT_Data.h"
#include "OPT_SamSyn.h"
#include "OPT_Com.h"
#include "smv_rx.h"

/* 合并版所有平台包含 */
#include "POLE_Data.h"   /*2007-4-6日 张云  */
#include "POLE_VtBox.h"
#include "HDL_Data.h"
#include "HDL_VtBox.h"
//#include  "mutual_61850.h"
//#include "GooseInterface.h"
#include "AppInterface.h"
#include "edp_asst.h"
// #include "config04.h"
/*2011-7-27  ZY  */
#include  "RE_DOCollect.h"
#include  "RE_LampCollect.h"
#include  "RE_EventCollect.h"
#include  "RE_ExportCollect.h"
#include  "RE_FloatAICollect.h"
#include  "RE_CmplxAICollect.h"
#include  "RE_DICollect.h"
#include  "RE_ImportCollect.h"
#include  "RE_TuyuanCollect.h" /*2011-7-27  ZY  */


#include "eth_callback.h"
#include "iecgoose.h"
#include "OPT_SamSyn.h"
#include "miscfunc.h"

/* defines */

#define ARR_SACN_MODE /* 数组模式 */

/* 驱动滞后点数 */
static uint32_t s_ulGrpScan0DriveInterval;

BOOL   bFastTaskIsDrived_g=FALSE;/*2006-12-3日 张云  */

extern BOOL bEnableWrDspBuf_g;  /* 填充采样数据缓冲标志 */

extern uint32_t g_ulPollSetChgCnt; /* 定值更新延迟计数 */

/* 扫描任务0驱动DSP任务接口.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void DSP_Scan_Drv(void);

/****保护引擎的驱动函数,该函数供系统相应模块调用****************************/

/*
     功能:从逻辑图顺序化文件中解析该文件,驱动保护引擎功能
*/

/****参数：strLogrpSeqFileName,逻辑图顺序化文件的文件名***************************/


/*   返回值，EP_STATUS */

EP_STATUS Relay_Engine_Activate(uint8_t *strLogrpSeqFileName)
{
    EP_STATUS OpeResult;
    int32_t ulNumBytesFree;
    int32_t ulNumBytesAlloc;


    LOG_Dbg_Msg("Init Logic Grp Begein.\n", 0, 0, 0, 0, 0, 0);
    /* 依序调用如下函数 */

    RE_CLearSysInfoBeforeReadFile();	/* 清除原来创建的逻辑图的痕迹 */

    /* 最初初始化, 初始化节点连表数组 */
    RE_SysInitBeforeReadFile();

    /* 解析逻辑图文件, 添加初始化节点 */
    OpeResult = RE_ReadLogrpFileInit(strLogrpSeqFileName,
                                     &LogrpAttrib_g);
    if (OpeResult != EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:逻辑图初始化失败\n",
                       0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic diagram rslv err:logic diagram init failure\n",
                       0, 0);
        }

        return EP_NOT_INIT;
    }

    /* 根据初始化节点，创建扫描节点初始化 */
    OpeResult = RE_CreateScanNodeInit(
                    RE_aPartGrpScanNodeList,
                    RE_aPartGrpInitNodeList,
                    LogrpAttrib_g.nNodeListArrDims,
                    RE_aPartGrpAttribArr
                );

    if (OpeResult != EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:逻辑图初始化失败\n",
                       0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic diagram rslv err:logic diagram init failure\n",
                       0, 0);
        }

        return EP_NOT_INIT;
    }

    /* 图元集中功能初始化,注意该函数调用位置 2011-7-27  ZY */
    OpeResult = RE_TyuanCollectInit((LIST *)RE_aPartGrpScanNodeList,
                                    (PARTGRP_ATTRIB_TYPE *)RE_aPartGrpAttribArr, LogrpAttrib_g.nNodeListArrDims);
    if (OpeResult != EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT | ER_ALARM | ER_LOCK,
                       "逻辑图解析错误:逻辑图图元集中功能初始化失败\n",
                       0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT | ER_ALARM | ER_LOCK,
                       "logic diagram rslv err:logic diagram init failure\n",
                       0, 0);
        }
    }
    /* 对端口引入和端口引出进行匹配初始化，设置端口引入的来源IO指针 */
    OpeResult = RE_InitMatchAllExternImportOuterInputTuyuan(
                    RE_aPartGrpScanNodeList,
                    RE_aPartGrpInitNodeList,
                    LogrpAttrib_g.nNodeListArrDims
                );

    if (OpeResult != EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:逻辑图初始化失败\n",
                       0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic diagram rslv err:logic diagram init failure\n",
                       0, 0);
        }

        return EP_NOT_INIT;
    }

    /* 完成扫描节点连表的剩余信息的初始化, 比如信号输入来源指针 */
    OpeResult = RE_InitAllGrpNode(
                    RE_aPartGrpScanNodeList,
                    RE_aPartGrpInitNodeList,
                    (int)(LogrpAttrib_g.nNodeListArrDims)
                );

    if (OpeResult != EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:逻辑图初始化失败\n",
                       0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic diagram rslv err:logic diagram init failure\n",
                       0, 0);
        }
        return EP_NOT_INIT;
    }

    /* 剩余内存记录
     */
    GetMemPartInfo(&ulNumBytesFree, &ulNumBytesAlloc);

    /* 逻辑图解析完成后的其他初始化, 释放无用节点 */
    RE_SysInitAfterReadFile(&LogrpAttrib_g);

    /* 设置逻辑图初始化完成标志 */
    EP_Before_Lgc_Run() ;

#ifdef ETH_QUERY_MODE
//      if(fcc_intDisable(0)!=OK)
//	    assert(0);

    // if(fcc_intDisable(1)!=OK)
    //     assert(0);
#endif

    /* 整理扫描列表 */
    OpeResult = RE_ClassScanNodeInit((LIST *)RE_aPartGrpScanNodeList,
                                     (PARTGRP_ATTRIB_TYPE *)RE_aPartGrpAttribArr, LogrpAttrib_g.nNodeListArrDims);
    if (OpeResult != EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT | ER_ALARM | ER_LOCK,
                       "逻辑图解析错误:逻辑图初始化失败\n",
                       0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT | ER_ALARM | ER_LOCK,
                       "logic diagram rslv err:logic diagram init failure\n",
                       0, 0);
        }

        return EP_NOT_INIT;
    }

    /* 初始化任务DI更新计数 */
    RE_SetLogDIUpdateCnt();

    /* 逻辑图扫描任务的创建和驱动 */
    OpeResult = LogrpScanTaskDrive(&LogrpAttrib_g);
    if (OpeResult != EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:逻辑图初始化失败\n",
                       0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic diagram rslv err:logic diagram init failure\n",
                       0, 0);
        }

        return EP_NOT_INIT;
    }

    RE_OpeAfterLogrpScanTaskDrive(&LogrpAttrib_g);

    /* 设置状态为保护任务已经创建 */
    taskDelay(SYS_SEC/50);
    RE_SetRelayTaskCreateState(TRUE);
    bEnableWrDspBuf_g = TRUE;  /* 允许数据缓冲填写 */
    InitPubGoYabanInfo();
    LOG_Dbg_Msg("Init Logic Grp End.\n", 0, 0, 0, 0, 0, 0);

    return EP_SUCCESS;
}




/*    采样节拍驱动关联函数由实时数据模块任务调用，

      该函数在保护功能的逻辑图任务启动后时，注册到实时数据模块，释放驱动相应信号量，
        从而驱动逻辑图扫描
      参数：pPara, 任务号地址

      返回值，无
*/

void   RE_SamDriver(void   *pPara)
{
    unsigned  long  * pulTaskNo;
    unsigned  long  ulTaskNo;



    pulTaskNo=(unsigned  long  *)pPara;
    ulTaskNo=*pulTaskNo;
    if((ulTaskNo>=MAX_CREATE_RELAYFUNC_TASK_COUNT)
            ||(ulTaskNo<0))
    {
        /* 若是出错，则告警 ，退出*/
        logMsg("Error,LogicScan  Task  Drive  Serious  Error !\n",
               0,0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       " 逻辑图扫描错误:逻辑图驱动错误\n",
                       0, 0);
        }

        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       " logic grp scan err:logic diagram drive err\n",
                       0, 0);
        }
        assert(FALSE);
        return;
    }

    if(RE_abLogrpScaningFlag_g[ulTaskNo])
    {

        if(!(RE_RelayTaskDriveSemFirstFreeFlagArr_g[ulTaskNo]))
        {
            /* 若是首次释放信号量 */
            RE_RelayTaskDriveSemFirstFreeAICounterArr_g[ulTaskNo]
                =RD_AI_Cnt();
            RE_RelayTaskDriveSemFirstFreeFlagArr_g[ulTaskNo]=TRUE;

        }
        semGive(RE_aInvokeLogrpNextScanSem_g[ulTaskNo]);

    }

}




/*      根据前面读取逻辑图文件,并进行完所有的初始化之后
        创建逻辑图扫描单任务或多任务,并驱动扫描任务运行
        参数    pGrpAttrib,逻辑图属性

        返回值  EP_STATUS;

 */
EP_STATUS   LogrpScanTaskDrive(LOGRP_ATTRIB_TYPE  *pGrpAttrib)
{
    int  nTaskID;
    int  i;
    int    nCurTaskPri;/* 当前任务的优先级 */
    int   nPartGrpCount;
    int   nTaskScanInterval;
    EP_CHART_MSG  *pChartMsg;
    /*  任务名号数组*/
    char   astrTaskSeqNo[10][6]=
    {"0","1","2","3","4","5","6","7","8","9"};
    char  (* pstrTaskSeqNo)[6];

    nPartGrpCount=pGrpAttrib->nNodeListArrDims;

    /* 为分图分配数组 */
    for (i = 0; i<nPartGrpCount; i++)
    {
        if ((RE_arrScanUnit[i] = calloc(RE_LstCount(RE_aPartGrpScanNodeList+i), sizeof(SCAN_UNIT)))
                == NULL)
        {
            return EP_SYS_ERR;
        }
    }

    pChartMsg=RE_aGrpScanTaskMsg;
    pstrTaskSeqNo=astrTaskSeqNo;

    /* 若允许创建的分图扫描任务数太多,则出错 */

    if(pGrpAttrib->nAllowMaxTaskCount
            >MAX_CREATE_RELAYFUNC_TASK_COUNT)
    {
        LOG_Dbg_Msg("Allow  Spawn  Relay  Scan  Task  Count  too  much!\n",0,0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图解析错误:创建的逻辑图任务太多了\n",
                       0,0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp rslv err:create too much relay task\n",
                       0,0);
        }
        assert(FALSE);  /* 若非以上类型,则告警   */
        return  EP_SYS_ERR;

    }

    /* 创建每个被投入的保护分图扫描任务 */
    for(i=0; i<pGrpAttrib->nAllowMaxTaskCount; i++)
    {
        /* 设置本任务名 */
        char   strTaskName[64]="tLogicScan";
        strcat(strTaskName,(*pstrTaskSeqNo));
        nTaskScanInterval=(int)((unsigned  long)(pGrpAttrib->
                                uiGrpScanDriveSamPeriodIntervalArr[i]));

        /* 设置任务优先级 */
        switch(i)
        {
            case  0:
                nCurTaskPri=TSK_PRI_MASTER_RELAY_SCAN;

                break;
            case  1:

                nCurTaskPri=TSK_PRI_BACKUP_RELAY_SCAN;

                break;
            case  2:

                nCurTaskPri=TSK_PRI_SECOND_BACKUP_RELAY_SCAN;

                break;
            case  3:

                nCurTaskPri=TSK_PRI_THIRD_BACKUP_RELAY_SCAN;

                break;
            case  4:

                nCurTaskPri=TSK_PRI_FORTH_BACKUP_RELAY_SCAN;

                break;
            case  5:

                nCurTaskPri=TSK_PRI_FIFTH_BACKUP_RELAY_SCAN;

                break;
            case  6:

                nCurTaskPri=TSK_PRI_SIXTH_BACKUP_RELAY_SCAN;

                break;
            case  7:

                nCurTaskPri=TSK_PRI_SEVENTH_BACKUP_RELAY_SCAN;

                break;
            default:

                return  EP_SYS_ERR;
                assert(FALSE);
                break;

        }

        if(pGrpAttrib->bGrpScanTaskCreateFlagArr[i])
        {
            /*若该任务，能被创建，则创建  */
            nTaskID=taskSpawn(
                        strTaskName,/*任务名  */
                        nCurTaskPri,/* 优先级 */
                        VX_FP_TASK,/* 浮点支持 */
                        RELAY_SCAN_TASK_STACK_SIZE,/* 堆栈大小 */
                        (FUNCPTR)RE_tMultiPartGrpScanTask,/* 入口函数 */
                        (int)RE_aPartGrpScanNodeList,/*扫描节点连表数组  */
                        (int)RE_aPartGrpAttribArr,/* 所有分图属性数组 */
                        (int)pChartMsg,/* 任务相关信息结构指针参数  */
                        (int)i,/* 扫描任务号参数 */
                        nPartGrpCount,/*逻辑图分图个数*/
                        nTaskScanInterval,/* 任务扫描周期  */
                        0,(int)(((uint64_t)pChartMsg)>>32),(int)(((uint64_t)RE_aPartGrpAttribArr)>>32),(int)(((uint64_t)RE_aPartGrpScanNodeList)>>32));/*其他参数未用  */
            if(nTaskID==ERROR)
            {
                LOG_Dbg_Msg("Spawn  Relay  Scan  Task  failure!\n",0,0,0,0,0,0);
                return  EP_SYS_ERR;
            }
        }
        /* 准备创建下1个任务 */
        pChartMsg++;
        pstrTaskSeqNo++;

    }


    return  EP_SUCCESS;
}



/*       逻辑图任务时的扫描任务创建入口函数

       参数
              nPartGrpScanNodeListArr,所有分图扫描节点连表数组首地址
              nPartGrpAttribArr,所有分图属性数组
              nScanTaskMsgPointer, 扫描任务相关的数据指针
              nScanTaskNo,扫描的任务号
              nAllPartGrpDims,逻辑图的分图个数
              nScanInterval,该任务扫描周期

       返回值  无
*/

void   RE_tMultiPartGrpScanTask(
    int  nPartGrpScanNodeListArr,
    int  nPartGrpAttribArr,
    int  nScanTaskMsgPointer,
    int  nScanTaskNo,
    int  nAllPartGrpDims,
    int  nScanInterval,
    int arg1,
    int hnScanTaskMsgPointer,
    int hnPartGrpAttribArr,
    int  hnPartGrpScanNodeListArr
    )
{
    LIST  *  pPartGrpScanNodeListArr;
    PARTGRP_ATTRIB_TYPE   *pPartGrpAttribArr;
    long  nPartGrpCount;
    EP_CHART_MSG  *  pScanTaskMsg;
    LIST  *pCurPartGrpScanNodeList;
    PARTGRP_ATTRIB_TYPE   *pCurPartGrpAttrib;

    BOOL *  pbTaskCurScanRefreshSettingFlag;
    /*配置模块中定植域被刷新的时刻*/
    uint32_t  ulSettingFieldModifiedTime=0;
    BOOL   bIsFirstScan;
    SEM_ID  *psemCurPartGrp;

    BOOL  *  pbCurPartGrpScanningFlag;

#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)
    BOOL bAllPartGrpScannedFlag=FALSE;  /*2008-1-25 张云DQ merge: 所有任务都已扫描标志*/
    int idx=0;
#endif

    NODE  *  pCurScanNode;
    NODE **ppInputNode;
    uint32_t   ulPreUsCnt=0;
    uint32_t   ulPreAICnt=0;
    uint32_t   ulCurTaskScanAICnt=0;
    uint32_t   ulDspAICnt;
    uint32_t   ulDspAheadAINum;
    int  k;
    long   nGrpTuyuanCount;
    long  m;
    uint32_t   ulGrpScanDriveInterval;
    int  iCurTaskOptAOCnt;   /*2006-11-11日 张云修改  */
    NODE ** ppCurTaskOptAONodeArr;  /*2006-11-11日 张云修改  */
    uint8_t aucLogInfo[256];
    SCAN_UNIT *pScanFunc = NULL;

    uint64_t tmp64;
    tmp64 = ((uint64_t)hnPartGrpAttribArr << 32) | (uint32_t)nPartGrpAttribArr;
    pPartGrpAttribArr = (PARTGRP_ATTRIB_TYPE *)tmp64;

    tmp64 = ((uint64_t)hnPartGrpScanNodeListArr << 32) | (uint32_t)nPartGrpScanNodeListArr;
    pPartGrpScanNodeListArr=(LIST  * )tmp64;

    tmp64 = ((uint64_t)hnScanTaskMsgPointer << 32) | (uint32_t)nScanTaskMsgPointer;
    pScanTaskMsg=(EP_CHART_MSG  * )tmp64;    


    // pPartGrpAttribArr=(PARTGRP_ATTRIB_TYPE   *)nPartGrpAttribArr;



    // pScanTaskMsg=(EP_CHART_MSG  * )nScanTaskMsgPointer;
    nPartGrpCount=(long)nAllPartGrpDims;
    ulGrpScanDriveInterval=(unsigned  long)nScanInterval;

    pbTaskCurScanRefreshSettingFlag=RE_aGrpScanDingzhiRefreshFlag+nScanTaskNo;
    psemCurPartGrp=RE_aInvokeLogrpNextScanSem_g+nScanTaskNo ;
    pbCurPartGrpScanningFlag=RE_abLogrpScaningFlag_g+ nScanTaskNo;



    /* 设置该任务的扫描图元对应的入口标号地址   */

    for(k=0; k<nPartGrpCount; k++)
    {
        /* 依次获得该任务被投入的分图的图元连表 */
        pCurPartGrpScanNodeList=pPartGrpScanNodeListArr+k;
        pCurPartGrpAttrib=pPartGrpAttribArr+k;
        if(!(pCurPartGrpAttrib->bRunFlag))
        {
            /* 若该分图未被投入，则重新循环 */
            continue;
        }
        if((pCurPartGrpAttrib->bRunFlag)
                &&(pCurPartGrpAttrib->nScanTaskNo!=nScanTaskNo))
        {
            /*若该分图被投入，但分图任务号不是所创建的任务号，则重新循环  */
            continue;
        }

        pCurPartGrpAttrib->bTaskOutFstFlag=TRUE;		/* 第一次退出 */

        /* 按分图建立数组 */
        pScanFunc = RE_arrScanUnit[k];

        /* 获得当前连表第1个图元 */
        nGrpTuyuanCount=RE_LstCount(pCurPartGrpScanNodeList);

        pCurScanNode=RE_LstFirst(pCurPartGrpScanNodeList);
        for(m=0; m<nGrpTuyuanCount; m++)
        {
            /* 设置扫描图元入口标号*/

            pCurScanNode->nScanTaskNo = nScanTaskNo;
            pCurScanNode->nScanInterval = nScanInterval;
            switch(pCurScanNode->ulTuyuanType)
            {

                case    RE_SUANFA_SCAN:
                    /* 算法 */
#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_SuanfaTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_SUANFA_SCAN_LBL;
#endif
                    break;

                case    RE_2_AND_SCAN:
                    /* 与门 */
#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_2AndTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_2_AND_SCAN_LBL;
#endif
                    break;

                case    RE_3_AND_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_3AndTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_3_AND_SCAN_LBL;
#endif
                    break;

                case    RE_4_AND_SCAN:


#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_4AndTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_4_AND_SCAN_LBL;
#endif
                    break;

                case    RE_5_AND_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_5AndTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_5_AND_SCAN_LBL;
#endif
                    break;

                case   RE_6_AND_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_6AndTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_6_AND_SCAN_LBL;
#endif
                    break;

                case   RE_7_AND_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_7AndTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_7_AND_SCAN_LBL;
#endif
                    break;

                case    RE_8_AND_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_8AndTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_8_AND_SCAN_LBL;
#endif
                    break;

                case   RE_MULTI_AND_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_MultiAndTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_MULTI_AND_SCAN_LBL;
#endif
                    break;

                case    RE_2_OR_SCAN:
                    /*或门  */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_2OrTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_2_OR_SCAN_LBL;
#endif
                    break;

                case    RE_3_OR_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_3OrTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_3_OR_SCAN_LBL;
#endif
                    break;

                case    RE_4_OR_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_4OrTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_4_OR_SCAN_LBL;
#endif
                    break;

                case    RE_5_OR_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_5OrTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_5_OR_SCAN_LBL;
#endif
                    break;

                case   RE_6_OR_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_6OrTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_6_OR_SCAN_LBL;
#endif
                    break;

                case    RE_7_OR_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_7OrTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_7_OR_SCAN_LBL;
#endif
                    break;

                case    RE_8_OR_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_8OrTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_8_OR_SCAN_LBL;
#endif
                    break;

                case   RE_MULTI_OR_SCAN:



#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_MultiOrTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_MULTI_OR_SCAN_LBL;
#endif
                    break;

                case    RE_NOT_SCAN:
                    /* 非门 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_NotTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_NOT_SCAN_LBL;
#endif
                    break;


                case    RE_TIMER_SCAN:
                    /* 时间继电器 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_TimerTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_TIMER_SCAN_LBL;
#endif
                    break;

                case    RE_YABAN_SCAN:
                    /* 压板 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_YabanTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_YABAN_SCAN_LBL;
#endif
                    break;

                case   RE_CONTROLWORD_SCAN:
                    /* 控制字 */



#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_ControlWordTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_CONTROLWORD_SCAN_LBL;
#endif
                    break;

                case    RE_FLOAT_GREATERTHAN_SCAN:
                    /* 大于比较  */



#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_FloatGreaterThanTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_FLOAT_GREATERTHAN_SCAN_LBL;
#endif
                    break;

                case    RE_UNSIGNED_32INT_GREATERTHAN_SCAN:



#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_Unsigned32IntGreaterThanTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_UNSIGNED_32INT_GREATERTHAN_SCAN_LBL;
#endif
                    break;

                case    RE_SIGNED_32INT_GREATERTHAN_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_Signed32IntGreaterThanTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_SIGNED_32INT_GREATERTHAN_SCAN_LBL;
#endif
                    break;

                case    RE_FLOAT_LESSTHAN_SCAN:
                    /* 小于比较  */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_FloatLessThanTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_FLOAT_LESSTHAN_SCAN_LBL;
#endif
                    break;
                case    RE_UNSIGNED_32INT_LESSTHAN_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_Unsigned32IntLessThanTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_UNSIGNED_32INT_LESSTHAN_SCAN_LBL;
#endif
                    break;

                case    RE_SIGNED_32INT_LESSTHAN_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_Signed32IntLessThanTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_SIGNED_32INT_LESSTHAN_SCAN_LBL;
#endif
                    break;

                case     RE_FLOAT_EQUAL_SCAN:
                    /* 等于比较 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_FloatEqualTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_FLOAT_EQUAL_SCAN_LBL;
#endif
                    break;
                case     RE_UNSIGNED_32INT_EQUAL_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_Unsigned32IntEqualTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_UNSIGNED_32INT_EQUAL_SCAN_LBL;
#endif
                    break;

                case     RE_SIGNED_32INT_EQUAL_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_Signed32IntEqualTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_SIGNED_32INT_EQUAL_SCAN_LBL;
#endif
                    break;

                case     RE_MEA_AI_OUTERINPUT_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_MeaAIOuterInputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_MEA_AI_OUTERINPUT_SCAN_LBL;
#endif
                    break;

                case    RE_PO_OUTERINPUT_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_PoInputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_PO_OUTERINPUT_SCAN_LBL;
#endif
                    break;
                case    RE_AI_PLUSCOF_OUTERINPUT_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_AiPlusCofInputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_AIPLUSCOF_OUTERINPUT_SCAN_LBL;
#endif
                    break;

                case    RE_AI_OFFCOF_OUTERINPUT_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_AiOffCofInputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_AIOFFCOF_OUTERINPUT_SCAN_LBL;
#endif
                    break;

                case    RE_CL_PLUSCOF_OUTERINPUT_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_ClPlusCofInputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_CLPLUSCOF_OUTERINPUT_SCAN_LBL;
#endif
                    break;

                case    RE_CL_OFFCOF_OUTERINPUT_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_ClOffCofInputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_CLOFFCOF_OUTERINPUT_SCAN_LBL;
#endif
                    break;
                case    RE_AI_PROCOF_OUTERINPUT_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_AiProCofInputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_AIPROCOF_OUTERINPUT_SCAN_LBL;
#endif
                    break;

                case     RE_EXTERN_IMPORT_OUTERINPUT_SCAN:
                    /*2011-8-8  ZY  */
                    assert(FALSE);
                    break;

                case     RE_TRIP_ENABLE_OUTEROUTPUT_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_TripEnableOuterOutputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_TRIP_ENABLE_OUTEROUTPUT_SCAN_LBL;
#endif
                    break;

                case     RE_EXTERN_EXPORT_OUTEROUTPUT_SCAN:
                    /*2011-8-8  ZY  */
                    assert(FALSE);

                    break;

                case     RE_FLOAT_AI_CH_OUTEROUTPUT_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_FloatAIOuterOutputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_FLOAT_AI_CH_OUTEROUTPUT_SCAN_LBL;
#endif
                    break;

                case     RE_COMPLEX_AI_CH_OUTEROUTPUT_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_ComplexAIOuterOutputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_COMPLEX_AI_CH_OUTEROUTPUT_SCAN_LBL;
#endif
                    break;

                case   RE_OPTAO_OUTEROUTPUT_SCAN  :

                    LogrpAttrib_g.iTaskOptAOScanNodeCntArr[nScanTaskNo]++;/*2006-11-11日 张云修改,保存每个任务被使用的光纵AO对外输出节点  */
                    assert(LogrpAttrib_g.iTaskOptAOScanNodeCntArr[nScanTaskNo]<RELAY_SCAN_TASK_ALLOW_MAX_OPTAO_SIZE);
                    iCurTaskOptAOCnt=LogrpAttrib_g.iTaskOptAOScanNodeCntArr[nScanTaskNo];
                    ppCurTaskOptAONodeArr=LogrpAttrib_g.ppTaskOptAOScanNodeArr[nScanTaskNo];
                    ppCurTaskOptAONodeArr[iCurTaskOptAOCnt-1]=pCurScanNode;
                    pCurPartGrpAttrib->optAoNum++;  /* 光差输出端口个数 */
                    RE_LstDeleteNoFree(pCurPartGrpScanNodeList, pCurScanNode);   /* 从列表中删除,不释放所占用内存 */

                    break;


                case   RE_AIPLUSADT_OUTEROUTPUT_SCAN:
                    /*  外部输出 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_AIPlusCofOutputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_AIPLUSADT_OUTEROUTPUT_SCAN_LBL;
#endif
                    break;

                case   RE_AIOFFADT_OUTEROUTPUT_SCAN:
                    /*  外部输出 */
#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_AIOffCofOutputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_AIOFFADT_OUTEROUTPUT_SCAN_LBL;
#endif

                    break;
                case   RE_CLPLUSADT_OUTEROUTPUT_SCAN:
                    /*  外部输出 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_ClPlusCofOutputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_CLPLUSADT_OUTEROUTPUT_SCAN_LBL;
#endif
                    break;

                case   RE_CLOFFADT_OUTEROUTPUT_SCAN:
                    /*  外部输出 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_ClOffCofOutputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_CLOFFADT_OUTEROUTPUT_SCAN_LBL;
#endif
                    break;

                case   RE_DIFILTADT_OUTEROUTPUT_SCAN:
                    /*  外部输出 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_DIFiltTmOutputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_DIFILTADT_OUTEROUTPUT_SCAN_LBL;
#endif
                    break;

                case   RE_YCOVERADT_OUTEROUTPUT_SCAN:
                    /*  外部输出 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_YcChgCofOutputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_YCOVERADT_OUTEROUTPUT_SCAN_LBL;
#endif
                    break;

                case   RE_CLOVERADT_OUTEROUTPUT_SCAN:
                    /*  外部输出 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_ClChgCofOutputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_CLOVERADT_OUTEROUTPUT_SCAN_LBL;
#endif
                    break;

                case   RE_PO_OUTEROUTPUT_SCAN:
                    /*  外部输出 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_POOuterOutputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_PO_OUTEROUTPUT_SCAN_LBL;
#endif
                    break;

                case    RE_2WAY_MULTIWAYSELECT_SCAN:
                    /* 多路选通  */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_2WayMultiwaySelectTuyuanScan2;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_2WAY_MULTIWAYSELECT_SCAN_LBL;
#endif
                    break;

                case    RE_3WAY_MULTIWAYSELECT_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_3WayMultiwaySelectTuyuanScan2;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_3WAY_MULTIWAYSELECT_SCAN_LBL;
#endif
                    break;

                case    RE_MULTIWAY_MULTIWAYSELECT_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_MultiWay_MultiwaySelectTuyuanScan2;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_MULTIWAY_MULTIWAYSELECT_SCAN_LBL;
#endif
                    break;
                case    RE_REPORT_ENABLE_OUTEROUTPUT_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_ReportEnableOuterOutputTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_REPORT_ENABLE_OUTEROUTPUT_SCAN_LBL;
#endif
                    break;

                case    RE_AMPANG_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_AmpAngTuyuanScan2;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_AMPANG_SCAN_LBL;
#endif
                    break;
                case    RE_REALIMAGE_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_RealImageTuyuanScan2;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_REALIMAGE_SCAN_LBL;
#endif
                    break;

                case    RE_FLOAT_MAX_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_FloatMaxTuyuanScan2;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_FLOAT_MAX_SCAN_LBL;
#endif
                    break;

                case   RE_UNSIGNED_32INT_MAX_SCAN :

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_Unsigned32IntMaxTuyuanScan2;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_UNSIGNED_32INT_MAX_SCAN_LBL;
#endif
                    break;
                case   RE_SIGNED_32INT_MAX_SCAN :

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_Signed32IntMaxTuyuanScan2;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_SIGNED_32INT_MAX_SCAN_LBL;
#endif
                    break;

                case    RE_FLOAT_MIN_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_FloatMinTuyuanScan2;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_FLOAT_MIN_SCAN_LBL;
#endif
                    break;

                case    RE_UNSIGNED_32INT_MIN_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_Unsigned32IntMinTuyuanScan2;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_UNSIGNED_32INT_MIN_SCAN_LBL;
#endif
                    break;

                case    RE_SIGNED_32INT_MIN_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_Signed32IntMinTuyuanScan2;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_SIGNED_32INT_MIN_SCAN_LBL;
#endif
                    break;

                case    RE_MODEWORD_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_ModeWordTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_MODEWORD_SCAN_LBL;
#endif
                    break;

                case    RE_MEADO_OUTERORDER_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_MeaDoOuterOrderTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_MEADO_OUTERORDER_SCAN_LBL;
#endif
                    break;

                case    RE_PLUSADJUST_OUTERORDER_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_PlusAdtOuterOrderTuyuanScan2;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_PLUSADT_OUTERORDER_SCAN_LBL;
#endif
                    break;
                case    RE_OFFADJUST_OUTERORDER_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_OffAdtOuterOrderTuyuanScan2;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_OFFADT_OUTERORDER_SCAN_LBL;
#endif
                    break;
                case    RE_POCLEAR_OUTERORDER_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_PulseClearOuterOrderTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_POCLEAR_OUTERORDER_SCAN_LBL;
#endif
                    break;

                case    RE_ZHUBIAN_OUTERORDER_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_TdZhuBianSwitchOuterOrderTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_TDZHUBIANSWITCH_OUTERORDER_SCAN_LBL;
#endif
                    break;

                case    RE_JINXIAN_OUTERORDER_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_TdJinXianSwitchOuterOrderTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_TDJINXIANSWITCH_OUTERORDER_SCAN_LBL;
#endif
                    break;

                case    RE_FARSTSCHG_OUTERORDER_SCAN:		/* 获得扫描地址 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_FarStsChgOuterOrderTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_FARSTSCHG_OUTERORDER_SCAN_LBL;		/* 扫描地址 */
#endif
                    break;

                case    RE_YXJXCHG_OUTERORDER_SCAN:		/* 获得扫描地址 */
#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_YXJXChgOuterOrderTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_YXJXCHG_OUTERORDER_SCAN_LBL;		/* 扫描地址 */
#endif
                    break;

                case    RE_JGSCHG_OUTERORDER_SCAN:		/* 获得扫描地址 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_JGSChgOuterOrderTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_JGSCHG_OUTERORDER_SCAN_LBL;		/* 扫描地址 */
#endif
                    break;

                case   RE_SETTINGSWITCH_SYSSERV_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_SettingSwitchTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_SETTINGSWITCH_SYSSERV_SCAN_LBL;
#endif
                    break;
                case   RE_REVERT_SYSSERV_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_RevertTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_REVERT_SYSSERV_SCAN_LBL;
#endif
                    break;
                case   RE_SWITCHFAR_SYSSERV_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_SwitchFarTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_SWITCHFAR_SYSSERV_SCAN_LBL;
#endif
                    break;
                case   RE_SWITCHEXAM_SYSSERV_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_SwitchExamTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_SWITCHEXAM_SYSSERV_SCAN_LBL;
#endif
                    break;
                case   RE_THREEU0WARN_SYSSERV_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_ThreeU0WarnTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_THREEU0WARN_SYSSERV_SCAN_LBL;
#endif
                    break;
                case   RE_PLUSADTOVER_SYSSERV_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_PlusAdtOverTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_PLUSADTOVER_SYSSERV_SCAN_LBL;
#endif
                    break;
                case   RE_OFFADTOVER_SYSSERV_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_OffAdtOverTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_OFFADTOVER_SYSSERV_SCAN_LBL;
#endif
                    break;
                case   RE_YABANTT_SYSSERV_SCAN:

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_YaBanTTTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_YABANTT_SYSSERV_SCAN_LBL;
#endif
                    break;
                case   RE_SETAUTOSET_SYSSERV_SCAN:		/* 自动整定定值 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_SetAutoSetTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_STAUTOST_SYSSERV_SCAN_LBL;
#endif
                    break;
                case   RE_DBYX_SYSSERV_SCAN:	/* 遥信 */

#ifdef ARR_SACN_MODE
                    pScanFunc->pScanFunc = RE_DBYXTuyuanScan;
                    pScanFunc->pCurScanNode = pCurScanNode;
                    pScanFunc++;
#else
                    pCurScanNode->pvScanTuyuanLabel
                        =&&RE_DBYX_SYSSERV_SCAN_LBL;
#endif
                    break;
                default:

                    LOG_Dbg_Msg("Error,Tuyuan Scan Type  isn't  Expected!\n",0,0,0,0,0,0);
                    assert(FALSE);
                    break;
            }/*  switch结束*/

            /* 获得连表的下1个图元 */
            pCurScanNode=RE_LstNext(pCurScanNode);

        }/* 分图内的图元处理结束  */

    }/*  所有分图处理结束 */

    /* 设置标号入口地址结束  */


    /*  设置定值域被在线修改标志初值为真,
    使得首次扫描引用定植时,需刷新定植 */
    /* 获得该任务所对应的定植刷新标志指针,每次扫描
      都对该标志进行设置,供图元判定是否需刷新定植  */
    *pbTaskCurScanRefreshSettingFlag=TRUE;
    pScanTaskMsg->bSetChg=TRUE;
    pScanTaskMsg->bScanIntFlag=FALSE;		/* 上次扫描中断标志，默认为FALSE，不中断 */
    bIsFirstScan=TRUE;
    *pbCurPartGrpScanningFlag=TRUE;/*设置当前正处于扫描态，
	                                    则扫描驱动函数可以释放信号量了   */

    /* 驱动滞后点数 */
    if (nScanTaskNo == 0)
    {
        s_ulGrpScan0DriveInterval = ulGrpScanDriveInterval;
    }

    /* pScanTaskMsg->pulSysErrSts=ER_Sys_All_Err_Sts(); */ /*ghx20060830将当前系统错误状态信息取出，供保护人员算法调用*/
    /*进行无穷扫描循环   */
    /* pCurPartGrpAttrib->bTaskOutFstFlag=TRUE; */		/* 第一次退出 */

    for(;;)
    {
        static uint32_t uiOptCnt_s=0;

        semTake((*psemCurPartGrp),WAIT_FOREVER);/*获取扫描驱动信号量  */

        /* 不处理STI板
         */
        if(nScanTaskNo==0)
        {
            /*若快速任务驱动，则首先处理光纵数据接收 2006-12-3日张云  */

            /* 是否由扫描任务驱动DSP */
            if (!bDspDrvMod)
            {
                DSP_Scan_Drv();
            }

            bFastTaskIsDrived_g=TRUE;/*2010-5-4日 张云为提高双通道接收效率改进  */
            if(uiAppType_g == APP_LINE) /* 母差保护版本不调用光纵相关函数 */
            {
                OPT_DealChNewRecvFrm(0);
                OPT_DealChNewRecvFrm(1);
            }

        }

        /*bb=TM_Get_usCnt();*/
        /* 任务扫描计数器刷新，供看门狗检测用 */
        RE_ulGrpScanTaskScanCounterArr_g[nScanTaskNo]++;

        if(bLogrpIsRestarted_g)
        {
            /* 若有新的逻辑图任务待重启，则自杀*/
            return;
        }

        /* pScanTaskMsg->ErrorGlobalFlag = GetGlobalErrorFlag(); */
        pScanTaskMsg->ErrorGlobalFlag = GetGlobalErrorFlag();
        /* 修改,去掉以前的条件,使下面状态异常返回时能实时更新 2010-12-30 ZY */
        pScanTaskMsg->ErrorFlag = GetSysErrFlag();		/* DY 9/1/2006 */
        pScanTaskMsg->bErrorRelayStop=GetSysErrFlagRelayStop();
        pScanTaskMsg->bErrorRelayContinue=GetSysErrFlagRelayContinue();


        pScanTaskMsg->EdpState = EP_Get_Sts_Bit();		/* 平台当前状态，DY 9/11/2006 */
        pScanTaskMsg->InitErrFlag = GetInitErrFlag();		/* 获得初始化过程错误代码，DY 9/11/2006 */

        if(bIsFirstScan)
        {
            ulSettingFieldModifiedTime=TM_Get_usCnt();

            /* 防止数据滞后 */
            if (!bDspDrvMod)
            {
                ulCurTaskScanAICnt=
                    RE_RelayTaskDriveSemFirstFreeAICounterArr_g[nScanTaskNo]+s_ulGrpScan0DriveInterval;
            }
            else
            {
                ulCurTaskScanAICnt=
                    RE_RelayTaskDriveSemFirstFreeAICounterArr_g[nScanTaskNo];

            }
            ulPreAICnt=ulCurTaskScanAICnt-ulGrpScanDriveInterval;
            ulPreUsCnt=RD_AI_Cnt_To_us(ulPreAICnt);
            bIsFirstScan=FALSE;
        }
        else
        {
            ulCurTaskScanAICnt=ulCurTaskScanAICnt+ulGrpScanDriveInterval;
        }


        ulDspAICnt=RD_AI_Cnt();
        ulDspAheadAINum=ulDspAICnt-ulCurTaskScanAICnt;

        if(ulDspAheadAINum>MAX_LOGIC_SCAN_DELAY_SAMPLE_COUNT)		/* 采样点增加，延长滞后点数 */
        {
            /* 告警，退出  */
            sprintf(aucLogInfo, "逻辑图扫描任务%d在%d个AI周期内不能扫描完成,滞后%d个AI周期!!\n",
                    (int)nScanTaskNo, (int)ulGrpScanDriveInterval, (int)ulDspAheadAINum);

            LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
            EP_Set_Sts_Bit(REBOOT_DLY);

            return;/*终止该任务*/
        }

        /* 设置本扫描任务本次扫描的相关信息,供图元扫描时进行访问 */

        pScanTaskMsg->bExistFatalErrFlag=EP_Is_Lock_DO();/*2006-11-2日 张云 严重错误标志  */
        pScanTaskMsg->ulUserSetAiCnt=ulCurTaskScanAICnt;/* 2006-11-2日 张云 用户设置的数据窗AICNT */
        /* 2006-12-21日 张云，添加获得保护任务复归命令 */
        if(RE_GetTaskCurFgSts(nScanTaskNo))
        {
            pScanTaskMsg->bRecvNewFgCmdFlag=TRUE;
            RE_ClearTaskFgSts(nScanTaskNo);/* 获得后，立即清除 */
        }
        else
        {
            pScanTaskMsg->bRecvNewFgCmdFlag=FALSE;
        }

        pScanTaskMsg->ulScnAiCnt=ulCurTaskScanAICnt;
        pScanTaskMsg->unNewAi=pScanTaskMsg->ulScnAiCnt-ulPreAICnt;
        ulPreAICnt=pScanTaskMsg->ulScnAiCnt;
        pScanTaskMsg->unDelayAi=ulDspAheadAINum;/*2006-9-21日，张云  */

        pScanTaskMsg->ulScnTime=RD_AI_Cnt_To_us(ulCurTaskScanAICnt);
        pScanTaskMsg->ulScnInterval=pScanTaskMsg->ulScnTime-ulPreUsCnt;
        ulPreUsCnt=pScanTaskMsg->ulScnTime;

        /*逻辑图实数式通道的0号通道的指针*/
        pScanTaskMsg->pfBase=RD_Base_Lgc_AI_P(ulCurTaskScanAICnt);
        assert(pScanTaskMsg->pfBase);
        /* 逻辑图复数或幅角式通道的0号通道的指针*/
        pScanTaskMsg->pxBase=RD_Base_Calc_AI_P(ulCurTaskScanAICnt);
        assert(pScanTaskMsg->pxBase);

        uiOptCnt_s++;

        /* 不处理STI板，根据配置处理
         */
        /*测量通道的0号通道的指针*/
        pScanTaskMsg->pxMeaBase=RD_Base_Msuc_AI_P();
        /* 设置光纵通道有效性 */			/* 2006-2-16 */
        if(uiAppType_g == APP_LINE)
        {
            static uint32_t uiOptTestCnt_s=0;
            uiOptTestCnt_s++;
            pScanTaskMsg->ulOptCh1ValidAiCnt=OPT_AI_Cnt(0,&(pScanTaskMsg->ulOptCh1MidResutValidAiCnt), pScanTaskMsg->ulScnAiCnt,
                                             &(pScanTaskMsg->ulOptCh1NewestValidAiCnt));
            pScanTaskMsg->ulOptCh2ValidAiCnt=OPT_AI_Cnt(1,&(pScanTaskMsg->ulOptCh2MidResutValidAiCnt), pScanTaskMsg->ulScnAiCnt,
                                             &(pScanTaskMsg->ulOptCh2NewestValidAiCnt));
            pScanTaskMsg->bOptCh1Valid=OPT_Ch_Is_Valid(0,
                                       pScanTaskMsg->ulOptCh1ValidAiCnt,&(pScanTaskMsg->lOptCh1Tsse),&(pScanTaskMsg->bOptCh1ComValid));
            pScanTaskMsg->bOptCh2Valid=OPT_Ch_Is_Valid(1,
                                       pScanTaskMsg->ulOptCh2ValidAiCnt,&(pScanTaskMsg->lOptCh2Tsse),&(pScanTaskMsg->bOptCh2ComValid));

#ifndef APP_LINE_SUPPORT  /* 线路保护在本侧采样异常时不影响本对侧同步 */
            if (appType_g == APP_TYPE_DIG)
            {
                /* 数字化应用 */
                pScanTaskMsg->bOptCh1Valid=pScanTaskMsg->bOptCh1Valid && (!sSmvData[0].bSmvCommOk || bSysSynFlag);
                pScanTaskMsg->bOptCh2Valid=pScanTaskMsg->bOptCh2Valid && (!sSmvData[0].bSmvCommOk || bSysSynFlag);
            }
#endif
            pScanTaskMsg->bOptCh1ComStable=OPT_Ch_Com_Sts(0, pScanTaskMsg->ulOptCh1ValidAiCnt);
            pScanTaskMsg->bOptCh2ComStable=OPT_Ch_Com_Sts(1, pScanTaskMsg->ulOptCh2ValidAiCnt);

            if(pScanTaskMsg->bOptCh1ComStable && !pScanTaskMsg->bOptCh1ComValid)
            {
                pScanTaskMsg->bOptCh1ComStable=FALSE;
            }

            if(pScanTaskMsg->bOptCh2ComStable && !pScanTaskMsg->bOptCh2ComValid)
            {
                pScanTaskMsg->bOptCh2ComStable=FALSE;
            }

            /* 2009-3-5日  ZY  */
            pScanTaskMsg->bOptCh1DataIsCredible=OPT_Ch_Data_Is_Credible(0,pScanTaskMsg->ulOptCh1ValidAiCnt,
                                                &pScanTaskMsg->iOptCh1RcvSndDiffChgTime);
            pScanTaskMsg->bOptCh2DataIsCredible=OPT_Ch_Data_Is_Credible(1,pScanTaskMsg->ulOptCh2ValidAiCnt,
                                                &pScanTaskMsg->iOptCh2RcvSndDiffChgTime);


            if(nScanTaskNo==0)
            {
                /* 若是快速保护，则统计丢帧值 2006-12-11日 张云 */
                pScanTaskMsg->bOptCh1NewCalcLostFrmFlag=OPT_Ch_CalcLostFrm(0,&(pScanTaskMsg->ulOptCh1CalcLostFrmNumPerSec));/*2006-12-5日 张云  */
                pScanTaskMsg->bOptCh2NewCalcLostFrmFlag=OPT_Ch_CalcLostFrm(1,&(pScanTaskMsg->ulOptCh2CalcLostFrmNumPerSec));
            }
            else
            {
                /* 若是慢速保护，则不统计 */
                pScanTaskMsg->bOptCh1NewCalcLostFrmFlag=FALSE;
                pScanTaskMsg->ulOptCh1CalcLostFrmNumPerSec=0;
                pScanTaskMsg->bOptCh2NewCalcLostFrmFlag=FALSE;
                pScanTaskMsg->ulOptCh2CalcLostFrmNumPerSec=0;
            }
            /* 获得光纵通道的通道通信时间  2006-12-21日 张云 */
            pScanTaskMsg->ulOptCh1ComTime=OPT_GetChComTime(0);
            pScanTaskMsg->ulOptCh2ComTime=OPT_GetChComTime(1);

            pScanTaskMsg->ulOptCh1ShowTime=OPT_GetChRealComTime(0);
            pScanTaskMsg->ulOptCh2ShowTime=OPT_GetChRealComTime(1);
        }

#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)
        /* 设定同杆并架数据相关内容 2007-4-6日 张云 */
        /* pScanTaskMsg->SamePoleInfo.bPoleDataIsValid=POLE_Data_Is_Valid(); */

        /* 设定智能操作箱数据相关内容 2007-4-6日 张云 */
        pScanTaskMsg->HdlBoxInfo.bHdlDataIsValid = HDL_Data_Is_Valid(nScanTaskNo, ulGrpScanDriveInterval);
#endif
        /* 该任务此次扫描定值同步刷新操作
         * 计数值大于0说明有定值变位操作
         * 闭锁任务,保证计数的一致性
         */

        taskLock();
        /* 有定值更新计数, 同时有秒脉冲到达
         * 如果没有到达, 保持原有计数
         */
        if (pScanTaskMsg->ulSetChgCnt && TM_GetSecPluseSts())
        {
            *pbTaskCurScanRefreshSettingFlag=TRUE;
            pScanTaskMsg->bSetChg=TRUE;
            pScanTaskMsg->ulSetChgCnt--;
        }
        taskUnlock();

        if (!(EP_IN_HW_TEST()))
        {
            /* 若检测到当前处于非硬件测试状态,则逻辑图扫描,否则,逻辑图不扫描 */

            if (RE_bRelayTaskEnterHwTestModeFlagArr_g[nScanTaskNo])
            {
                /* 若检测到上次处于硬件测试状态，给出退出提示信息 */

                pScanTaskMsg->bHardTestExitFlag = TRUE;
            }
            else
            {

                pScanTaskMsg->bHardTestExitFlag=FALSE;
            }

            pScanTaskMsg->bHardTestEnterFlag=FALSE;/*2006-11-2日张云  */
            RE_bRelayTaskEnterHwTestModeFlagArr_g[nScanTaskNo]=FALSE;

            /*2011-8-3  ZY 对每个任务所有的FloatAI,ComplexAI,DI,IMPORT集中处理 */
            RE_ImportCollectScan(nScanTaskNo);
            /* RE_FloatAICollectScan(nScanTaskNo); */  /* 由于应用不通过图元访问, 可不扫描AI输入图元 */
            RE_CmplxAICollectScan(nScanTaskNo);
            /* 所有开入,不包括开入板 */
            RE_UpdateLogDICnt(pScanTaskMsg, ulGrpScanDriveInterval);
            if (RE_GetDIUpdatedFlag(pScanTaskMsg))
            {
                RE_DICollectScan(nScanTaskNo);
            }
            /* 对所有该任务被投入的分图图元进行扫描  */
            for(k=0; k<nPartGrpCount; k++)
            {

                /* 依次获得各分图的图元连表 */
                pCurPartGrpScanNodeList=pPartGrpScanNodeListArr+k;
                pCurPartGrpAttrib=pPartGrpAttribArr+k;

#if defined(EDP03_BUILD) && !defined(EDP03_INTELBOX_BUILD)		/* 智能操作箱在有系统错误时暂时不退出保护 */
                if(pScanTaskMsg->bErrorRelayStop && (!(pCurPartGrpAttrib->ucScanAttr&0x04)) && (pCurPartGrpAttrib->bTaskOutFstFlag))
                {
                    /* 为1时投入，取反 */
                    /* logMsg("保护退出!\n", 0, 0, 0, 0, 0, 0); */
                    pCurPartGrpAttrib->bRunFlag=FALSE;			/* 任务不再扫描 */
                    pCurPartGrpAttrib->bTaskOutFstFlag=FALSE;				/* 不再判断 */

                    if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "因出现严重错误，扫描任务%d独立保护分图%s扫描退出!\n",
                                   nScanTaskNo, (int)pCurPartGrpAttrib->strCurPartGrpName);
                    }

                    else if(ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK,
                                   "Because of serious error, the scanning task %d of independent relay protect logic diagram %s stop!\n",
                                   nScanTaskNo, (int)pCurPartGrpAttrib->strCurPartGrpName);
                    }
                }
#endif

                if(!(pCurPartGrpAttrib->bRunFlag))
                {
                    /* 若该分图未被投入，则重新循环 */
                    continue;
                }
                if((pCurPartGrpAttrib->bRunFlag)
                        &&(pCurPartGrpAttrib->nScanTaskNo!=nScanTaskNo))
                {
                    /*若该分图被投入，但分图任务号不是所创建的任务号，则重新循环  */
                    continue;
                }

                /* 定值图元 */
                if (*pbTaskCurScanRefreshSettingFlag)
                {
                    RE_MultiDingzhiOuterInputTuyuanScan(pCurPartGrpAttrib);
                }

                /* 只剩开入板图元 ,以前调用有问题 2011-7-27 ZY*/
                if (RE_GetDIUpdatedFlag(pScanTaskMsg))
                {
                    /* 开入模件 */
                    ppInputNode = ((PARTGRP_ATTRIB_TYPE *)pCurPartGrpAttrib)->ppDIModNode;
                    for (m = 0; m<((PARTGRP_ATTRIB_TYPE *)pCurPartGrpAttrib)->DIModScanNodeNum; m++)
                    {
                        RE_DIModTuyuanScan(*ppInputNode);
                        ppInputNode++;
                    }

                }

                /* 去掉以前对FloatAI,ComplexAI,DI的集中处理　2011-8-4日 ZY */


                /* 获得当前连表第1个图元 */
                nGrpTuyuanCount=RE_LstCount(pCurPartGrpScanNodeList);

                pCurScanNode=RE_LstFirst(pCurPartGrpScanNodeList);

                /* 链表模式 */
#ifndef ARR_SACN_MODE
                for(m=0; m<nGrpTuyuanCount; m++)
                {
                    /* 扫描图元*/

                    /* 若是GNU编译器 */
                    goto   *(pCurScanNode->pvScanTuyuanLabel);

RE_MEA_AI_OUTERINPUT_SCAN_LBL:
                    RE_MeaAIOuterInputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_PO_OUTERINPUT_SCAN_LBL:
                    RE_PoInputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_AIPLUSCOF_OUTERINPUT_SCAN_LBL:

                    RE_AiPlusCofInputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_AIOFFCOF_OUTERINPUT_SCAN_LBL:

                    RE_AiOffCofInputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_CLPLUSCOF_OUTERINPUT_SCAN_LBL:

                    RE_ClPlusCofInputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_CLOFFCOF_OUTERINPUT_SCAN_LBL:

                    RE_ClOffCofInputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_AIPROCOF_OUTERINPUT_SCAN_LBL:

                    RE_AiProCofInputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_SUANFA_SCAN_LBL:
                    /* 算法 */
                    RE_SuanfaTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_AMPANG_SCAN_LBL:

                    RE_AmpAngTuyuanScan
                    (nScanInterval,pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_REALIMAGE_SCAN_LBL:

                    RE_RealImageTuyuanScan
                    (nScanInterval,pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_FLOAT_GREATERTHAN_SCAN_LBL:
                    /* 大于比较  */
                    RE_FloatGreaterThanTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_FLOAT_LESSTHAN_SCAN_LBL:
                    /* 小于比较  */
                    RE_FloatLessThanTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_FLOAT_MAX_SCAN_LBL:

                    RE_FloatMaxTuyuanScan
                    (nScanInterval,pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_FLOAT_MIN_SCAN_LBL:

                    RE_FloatMinTuyuanScan
                    (nScanInterval,pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_2_AND_SCAN_LBL:
                    /* 与门 */
                    RE_2AndTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_3_AND_SCAN_LBL:

                    RE_3AndTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_4_AND_SCAN_LBL:

                    RE_4AndTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_5_AND_SCAN_LBL:

                    RE_5AndTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_6_AND_SCAN_LBL:

                    RE_6AndTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_2_OR_SCAN_LBL:
                    /*或门  */
                    RE_2OrTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_3_OR_SCAN_LBL:

                    RE_3OrTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_4_OR_SCAN_LBL:

                    RE_4OrTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_5_OR_SCAN_LBL:

                    RE_5OrTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_6_OR_SCAN_LBL:

                    RE_6OrTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_NOT_SCAN_LBL:
                    /* 非门 */
                    RE_NotTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_TIMER_SCAN_LBL:
                    /* 时间继电器 */
                    RE_TimerTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_YABAN_SCAN_LBL:
                    /* 压板 */
                    RE_YabanTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_CONTROLWORD_SCAN_LBL:
                    /* 控制字 */
                    RE_ControlWordTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_MODEWORD_SCAN_LBL:

                    RE_ModeWordTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_3WAY_MULTIWAYSELECT_SCAN_LBL:

                    RE_3WayMultiwaySelectTuyuanScan
                    (nScanInterval,pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_TRIP_ENABLE_OUTEROUTPUT_SCAN_LBL:

                    RE_TripEnableOuterOutputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_REPORT_ENABLE_OUTEROUTPUT_SCAN_LBL:

                    RE_ReportEnableOuterOutputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_FLOAT_AI_CH_OUTEROUTPUT_SCAN_LBL:

                    RE_FloatAIOuterOutputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_COMPLEX_AI_CH_OUTEROUTPUT_SCAN_LBL:

                    RE_ComplexAIOuterOutputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_PO_OUTEROUTPUT_SCAN_LBL:
                    /*  外部输出 */
                    RE_POOuterOutputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_7_AND_SCAN_LBL:

                    RE_7AndTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_8_AND_SCAN_LBL:

                    RE_8AndTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_MULTI_AND_SCAN_LBL:

                    RE_MultiAndTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_7_OR_SCAN_LBL:

                    RE_7OrTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_8_OR_SCAN_LBL:

                    RE_8OrTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_MULTI_OR_SCAN_LBL:

                    RE_MultiOrTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_FLOAT_EQUAL_SCAN_LBL:
                    /* 等于比较 */
                    RE_FloatEqualTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_UNSIGNED_32INT_EQUAL_SCAN_LBL:

                    RE_Unsigned32IntEqualTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_SIGNED_32INT_EQUAL_SCAN_LBL:

                    RE_Signed32IntEqualTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_UNSIGNED_32INT_GREATERTHAN_SCAN_LBL:

                    RE_Unsigned32IntGreaterThanTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_SIGNED_32INT_GREATERTHAN_SCAN_LBL:

                    RE_Signed32IntGreaterThanTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_UNSIGNED_32INT_LESSTHAN_SCAN_LBL:

                    RE_Unsigned32IntLessThanTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_SIGNED_32INT_LESSTHAN_SCAN_LBL:

                    RE_Signed32IntLessThanTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_UNSIGNED_32INT_MAX_SCAN_LBL:

                    RE_Unsigned32IntMaxTuyuanScan
                    (nScanInterval,pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_SIGNED_32INT_MAX_SCAN_LBL:

                    RE_Signed32IntMaxTuyuanScan
                    (nScanInterval,pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_UNSIGNED_32INT_MIN_SCAN_LBL:

                    RE_Unsigned32IntMinTuyuanScan
                    (nScanInterval,pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_SIGNED_32INT_MIN_SCAN_LBL:

                    RE_Signed32IntMinTuyuanScan
                    (nScanInterval,pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_2WAY_MULTIWAYSELECT_SCAN_LBL:
                    /* 多路选通  */
                    RE_2WayMultiwaySelectTuyuanScan
                    (nScanInterval,pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_MULTIWAY_MULTIWAYSELECT_SCAN_LBL:

                    RE_MultiWay_MultiwaySelectTuyuanScan
                    (nScanInterval,pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_MEADO_OUTERORDER_SCAN_LBL:
                    RE_MeaDoOuterOrderTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_PLUSADT_OUTERORDER_SCAN_LBL:
                    RE_PlusAdtOuterOrderTuyuanScan
                    (nScanTaskNo, pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_OFFADT_OUTERORDER_SCAN_LBL:
                    RE_OffAdtOuterOrderTuyuanScan
                    (nScanTaskNo, pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_POCLEAR_OUTERORDER_SCAN_LBL:
                    RE_PulseClearOuterOrderTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_TDZHUBIANSWITCH_OUTERORDER_SCAN_LBL:
                    RE_TdZhuBianSwitchOuterOrderTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_TDJINXIANSWITCH_OUTERORDER_SCAN_LBL:
                    RE_TdJinXianSwitchOuterOrderTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_FARSTSCHG_OUTERORDER_SCAN_LBL:		/* 扫描地址 */
                    RE_FarStsChgOuterOrderTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_YXJXCHG_OUTERORDER_SCAN_LBL:		/* 扫描地址 */
                    RE_YXJXChgOuterOrderTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_JGSCHG_OUTERORDER_SCAN_LBL:		/* 扫描地址 */
                    RE_JGSChgOuterOrderTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_AIPLUSADT_OUTEROUTPUT_SCAN_LBL:
                    /*  外部输出 */
                    RE_AIPlusCofOutputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_AIOFFADT_OUTEROUTPUT_SCAN_LBL:
                    /*  外部输出 */
                    RE_AIOffCofOutputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_CLPLUSADT_OUTEROUTPUT_SCAN_LBL:
                    /*  外部输出 */
                    RE_ClPlusCofOutputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_CLOFFADT_OUTEROUTPUT_SCAN_LBL:
                    /*  外部输出 */
                    RE_ClOffCofOutputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_DIFILTADT_OUTEROUTPUT_SCAN_LBL:
                    /*  外部输出 */
                    RE_DIFiltTmOutputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_YCOVERADT_OUTEROUTPUT_SCAN_LBL:
                    /*  外部输出 */
                    RE_YcChgCofOutputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_CLOVERADT_OUTEROUTPUT_SCAN_LBL:
                    /*  外部输出 */
                    RE_ClChgCofOutputTuyuanScan
                    (pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;

RE_SETTINGSWITCH_SYSSERV_SCAN_LBL:
                    /*定值切换系统服务输出图元*/
                    RE_SettingSwitchTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_REVERT_SYSSERV_SCAN_LBL:
                    /*复归系统服务输出图元*/
                    RE_RevertTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_SWITCHFAR_SYSSERV_SCAN_LBL:
                    /*切换到远方态系统服务输出图元*/
                    RE_SwitchFarTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_SWITCHEXAM_SYSSERV_SCAN_LBL:
                    /*切换到检修态系统服务输出图元*/
                    RE_SwitchExamTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_THREEU0WARN_SYSSERV_SCAN_LBL:
                    /*切换到检修态系统服务输出图元*/
                    RE_ThreeU0WarnTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_PLUSADTOVER_SYSSERV_SCAN_LBL:
                    /*切换到增益校准结束系统服务输出图元*/
                    RE_PlusAdtOverTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_OFFADTOVER_SYSSERV_SCAN_LBL:
                    /*切换到偏置校准结束系统服务输出图元*/
                    RE_OffAdtOverTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_YABANTT_SYSSERV_SCAN_LBL:
                    /*切换到压板投退系统服务输出图元*/
                    RE_YaBanTTTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_STAUTOST_SYSSERV_SCAN_LBL:
                    /* 定值自动整定系统服务输出图元 */
                    RE_SetAutoSetTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
RE_DBYX_SYSSERV_SCAN_LBL:
                    /*切换到遥信系统服务输出图元*/
                    RE_DBYXTuyuanScan(pCurScanNode);
                    /* 获得连表的下1个图元 */
                    pCurScanNode=RE_LstNext(pCurScanNode);
                    continue;
                }
#else /* 数组模式 */
                pScanFunc = RE_arrScanUnit[k];
                for (m = 0; m<nGrpTuyuanCount; m++)
                {
                    pScanFunc->pScanFunc(pScanFunc->pCurScanNode);
                    pScanFunc++;
                }
#endif

                /* 去掉以前对事件，DO，DOSET，指示灯的集中处理  2011-7-27 */

                /* 启动型录波 */
                RE_MultiOnlyStartLuboTuyuanScan(pCurPartGrpAttrib);

                /* 停止型录波 */
                RE_MultiOnlyStopLuboTuyuanScan(pCurPartGrpAttrib);

                /* 启停型录波 */
                RE_MultiStartStopLuboTuyuanScan(pCurPartGrpAttrib);

            }
            /*集中处理该任务的所有DO,指示灯,事件,端口引出图元，2011-7-27日 */
            RE_ExportCollectScan(nScanTaskNo);
            RE_DOCollectScan(nScanTaskNo);
            RE_LampCollectScan(nScanTaskNo);
            RE_EventCollectScan(nScanTaskNo);

            /*扫描该任务的所有光纵AO输出图元，2008-1-24日 张云merge*/
            RE_TaskAllOptAOTuyuanScan(nScanTaskNo,pScanTaskMsg->ulScnAiCnt);

        }
        else
        {
            /* 若是硬件测试态,则逻辑图不扫描 */

            if (!(RE_bRelayTaskEnterHwTestModeFlagArr_g[nScanTaskNo]))
            {
                /* 若检测到上次处于非硬件测试状态，给出首次进入提示信息 */

                SlakeUnkeepedLed();
                pScanTaskMsg->bHardTestEnterFlag = TRUE;

                /* 重新投入时更新定值和压板
                 */
                *pbTaskCurScanRefreshSettingFlag = TRUE;
                pScanTaskMsg->bSetChg = TRUE;
            }
            else
            {
                pScanTaskMsg->bHardTestEnterFlag=FALSE;
            }

            pScanTaskMsg->bHardTestExitFlag=FALSE;
            RE_bRelayTaskEnterHwTestModeFlagArr_g[nScanTaskNo]=TRUE;
        }

#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)
        /*2006-11-7日 张云修改，若是快速保护，则主动进行SPI的IO发送，要在录波之前，这样最快
                ，同时进行ACTIVE GOOSE的PUB，2007-4-9日 张云
          2007-7-10日张云修改，给61850模块提供定时器功能*/
        if(nScanTaskNo==0)
        {
            static   uint32_t    ulGoosePubCnt_s=0;
            static   BOOL   ulLstPubIsSuccess_s=FALSE;
            /*int   iGoPubScanIntvl;*/
            uint32_t   ulGoPubSrcSet;
            int   iPubNewGoRslt;
            BOOL   bDataIsChg;
            static MMS_UTC_TIME UTCTimeStamp;
            /*uint32_t TimeElapsed;*/
            static BOOL bFirst=TRUE;
            static uint32_t ulRefreshYabanCnt = 0;

            /* 检查Goose接收状态,
             * 放至逻辑图最后处理, 防止TTL为0时直接告警不更新数据
             */
            if(bFirst)
            {
                ulGsStsQueryPeriod = rdinfo_g.uiSmplPeriod*ulGrpScanDriveInterval;
                bFirst=FALSE;
            }
            else
            {
                /* 采用固定周期模式 */
                // CheckGoRx(ulGsStsQueryPeriod);
            }

            ulGoosePubCnt_s++;
#ifdef EDP_01_02_BUILD
            if (bNormalSndFlag)
                SPI_NEW_COM(nScanInterval);
#endif
            ulGoPubSrcSet=0;
            bDataIsChg=FALSE;
#if 0
            /*PUB同杆并架GOOSE  2007-4-9日张云 */
            if(bPoleBoxIsInit_g)
            {
                /*若同杆并架机箱被初始化  */
                ulGoPubSrcSet=ulGoPubSrcSet|SAME_POLE_GO_SRC_TYPE;
                iGoPubScanIntvl=(POLE_PUB_REFRESH_INTVL*uiAiRate_g)/(pScanTaskMsg->unNewAi*1000);
                if(iGoPubScanIntvl<1)
                {
                    /*防止除0  */
                    iGoPubScanIntvl=1;
                }
                if((ulGoosePubCnt_s%iGoPubScanIntvl)==0)
                {
                    /*若到了相应节拍， */
                    if(POLE_DataPubIsChgAndSave())
                    {
                        /*且同杆数据发生变化，则设置数据变化标志 2007-6-25日 张云 必须和上面分开调用*/
                        bDataIsChg=TRUE;
                    }
                }
            }
#endif
            /*PUB智能操作箱GOOSE  2007-4-9日张云 */
            if(bHdlBoxIsInit_g)
            {
                ulGoPubSrcSet=ulGoPubSrcSet|HDL_BOX_GO_SRC_TYPE;
                if(HDL_DataPubIsChgAndSave())
                {
                    /*且智能操作箱数据发生变化，则设置数据变化标志 2007-6-25日 张云 必须和上面分开调用*/
                    bDataIsChg=TRUE;
                }
            }
            if(!ulLstPubIsSuccess_s)
            {
                /*若上次没有发送成功，则设置数据变化标志，让goose 重新发送，2007-7-10日 张云  */
                bDataIsChg=TRUE;
            }

            /*2008-1-24 张云DQ: 所有保护任务都已经运行标志*/
            if(!bAllPartGrpScannedFlag)
            {
                for(idx=0; idx<LogrpAttrib_g.nAllowMaxTaskCount; idx++)
                {
                    if(LogrpAttrib_g.bGrpScanTaskScannedFlagArr[idx]==FALSE)
                    {
                        bAllPartGrpScannedFlag=FALSE;
                        break;
                    }
                    if(idx==(LogrpAttrib_g.nAllowMaxTaskCount-1))
                        bAllPartGrpScannedFlag=TRUE;
                }
            }

            if(bDataIsChg)
            {
                /*2013-5-23日  ZY */
                US_CNT_UTC_TIME  usNowUtcTime;
                uint32_t  ulNowUsCnt;
                TM_High_Get_Sys_Us_UTC_Time(&usNowUtcTime, &ulNowUsCnt);
                TM_High_Us_UTC_Time_To_MMS_UTC_Time(&usNowUtcTime, &UTCTimeStamp);
            }

            /*给61850提供定时器功能，每次扫描都发送　*/
            if(ulGoPubSrcSet&&bAllPartGrpScannedFlag&&bDataIsChg)
            {
                /*2007-8-3日 张云修改，只有配置成功后，才调用  */
                //iPubNewGoRslt=notify_61850_prot_data_update(ulGoPubSrcSet,&UTCTimeStamp,bDataIsChg,TM_Get_usCnt());
                // iPubNewGoRslt=Drive_Breakout_Goose_Pub(ulGoPubSrcSet,&UTCTimeStamp);
                iPubNewGoRslt=0;
                ulLstPubIsSuccess_s=TRUE;
            }
            /*刷新保护goose的所有压板状态  2007-7-10日 张云  */
            if(ulRefreshYabanCnt > 300)
            {
                ulRefreshYabanCnt = 0;
                // RefreshAllGoYabanSts();
            }
            else
            {
                ulRefreshYabanCnt++;
            }

        }/*if(nScanTaskNo==0)结束  */
#endif


        if(LogrpAttrib_g.bGrpScanTaskScannedFlagArr[nScanTaskNo]==FALSE)
            LogrpAttrib_g.bGrpScanTaskScannedFlagArr[nScanTaskNo]=TRUE; /*2008-1-24 张云DQ: 置该逻辑图已扫描标志*/


        /*bb=TM_Get_usCnt()-bb;
        aa++;
        	if(aa%5000==0)
        	{
        		logMsg("scan time interval=%d us\n",bb, 0, 0, 0,0,0);
        	}*/
        /* 本次扫描末尾处理录波  */
        SCI_Process_Cur_Logrp_Period_Lubo(nScanTaskNo,
                                          pScanTaskMsg->ulScnAiCnt);

        /* 本次扫描末尾处理标志集   */
        SCI_Process_Cur_Logrp_Period_Flagset_Record(nScanTaskNo,
                pScanTaskMsg->ulScnAiCnt);

        /* 本次扫描末尾处理遥信  */
        SCI_Process_Cur_Logrp_Period_Yaoxin(nScanTaskNo,
                                            pScanTaskMsg->ulScnAiCnt);

        /* 本次扫描末尾处理遥测  */
        SCI_Process_Cur_Logrp_Period_Yaoce(nScanTaskNo,
                                           pScanTaskMsg->ulScnAiCnt);
        /* 本次扫描末尾处理测量  */
        SCI_Process_Cur_Logrp_Period_Measure(nScanTaskNo, ulGrpScanDriveInterval, pScanTaskMsg->ulScnAiCnt);

        if (!(EP_IN_HW_TEST()))
        {
            /*设置该任务定植此次扫描被刷新标志为假  */
            *pbTaskCurScanRefreshSettingFlag=FALSE;
            pScanTaskMsg->bSetChg=FALSE;
        }

        SCI_Update_Yaban_Value_Auto(nScanTaskNo, ulGrpScanDriveInterval, pScanTaskMsg->ulScnAiCnt);

        if (nScanTaskNo == 0)
        {
#ifdef VIRT_BOX  /* if define virtual box. */
            setDoDataSub();
#endif

            /*2013-6-1  ZY 隔100毫秒左右（注意，要求比较精确，时间间隔<=160ms），更新一下内部系统基准时间*/
            // {//本系统统一使用posix提供的时间函数,所以注释掉这部分
            //     static  uint32_t  ulUpdtBaseLow_s=0;
            //     uint32_t  ulCurBaseLow;

            //     ulCurBaseLow=OptGetBaseTimerLowCnt();
            //     /*要求用精确时间比较，防止逻辑图抖动 */
            //     if (OptGetUsIntvlByBaseTimerLow(ulCurBaseLow,ulUpdtBaseLow_s)>100000)
            //     {
            //         TM_Updt_Sys_Time();
            //         ulUpdtBaseLow_s=ulCurBaseLow;
            //     }
            // }
        }

#ifndef APP_LINE_SUPPORT /* 不支持连续调试 */
        if(EP_IS_BOOT_SEL())
        {
            if(pScanTaskMsg->bDebugBeginFlag)
                TestNestStep();		/* 允许下一步调试DY 3/6/2007 */
        }
#endif

        /*统计当前任务的此次消耗的时间  */

        /* #ifndef APP_LINE_SUPPORT */ /* 不支持自动整定定值 */
        RE_JdSetAtSetCnt();
        /* #endif */
    }


}




/*     初始化所有分图的所有扫描节点的相应数据,此时所有初始化节点和扫描节点
        都已创建,
        并已进行完初步的创建初始化 ,此函数完成扫描节点的所有的初始化操作
        此函数完成所有图元的扫描节点的输入来源地址,录波,标志,遥测,遥信等工作
        参数  nPartGrpScanNodeListArr,所有分图的图元扫描节点的连表
               nPartGrpInitNodeListArr,所有分图的图元初始化节点的连表
              nPartGrpSum ,分图个数,也是可对连表数组操作的维数
        返回值,EP_STATUS ,
*/
EP_STATUS  RE_InitAllGrpNode(LIST * nPartGrpScanNodeListArr,
                             LIST *   nPartGrpInitNodeListArr,
                             int   nPartGrpSum )
{
    int  k;
    LIST  *  pPartGrpScanNodeListArr;
    LIST   * pPartGrpInitNodeListArr;
    long   nPartGrpCount;

    LIST  *pCurPartGrpScanNodeList;
    LIST  *pCurPartGrpInitNodeList;

    NODE  *  pCurScanNode,*pCurInitNode;

    pPartGrpScanNodeListArr=(LIST  * )nPartGrpScanNodeListArr;
    pPartGrpInitNodeListArr=(LIST   *)nPartGrpInitNodeListArr;
    nPartGrpCount=(long)nPartGrpSum;
    for(k=0; k<nPartGrpCount; k++)
    {
        /* 依次获得各分图的图元的扫描节点连表 */
        pCurPartGrpScanNodeList=pPartGrpScanNodeListArr+k;
        /* 依次获得各分图的图元的初始化节点连表 */
        pCurPartGrpInitNodeList=pPartGrpInitNodeListArr+k;

        /* 获得当前连表第1个图元 */
        pCurScanNode=RE_LstFirst(pCurPartGrpScanNodeList);
        pCurInitNode=RE_LstFirst(pCurPartGrpInitNodeList);
        for(;;)
        {
            /* 扫描图元*/
            EP_STATUS   TuyuanInitResult;
            if(pCurScanNode==NULL)
            {
                /* 若到了图元连表的尾部,则跳出连表操作 */
                break;
            }
            TuyuanInitResult=
                (*  (RE_GetInitNodeInitScanFunc(pCurInitNode)))
                (pCurInitNode,pCurScanNode,pCurPartGrpScanNodeList
                 ,RE_aPartGrpAttribArr[k].bRunFlag);

            assert(TuyuanInitResult==EP_SUCCESS);
            if(TuyuanInitResult!=EP_SUCCESS)
            {
                /* 若图元初始化不成功,则返回错误  */
                return  EP_NOT_INIT;
            }

            /* 获得连表的下1个图元 */
            pCurScanNode=RE_LstNext(pCurScanNode);
            pCurInitNode=RE_LstNext(pCurInitNode);

        }

    }
    return   EP_SUCCESS;

}




/*      保护功能模块的清除系统信息
        在读取逻辑图文件之前的 全局初始化 的前面调用
        主要用于清除原来的逻辑图任务的痕迹的工作
        参数  无
        返回值  无

 */
void   RE_CLearSysInfoBeforeReadFile()
{
    int  i;
    /* 若不是通过RESTART启动，则设置该注册个数标志为0，表示尚未注册过  */
    if(pSuanfaDebugEntryFunc_g==NULL)
    {
        ulLastRegisteSamFuncCount_g=0;
        bLogrpIsRestarted_g=FALSE;/* 表示有新逻辑图任务不是通过restart启动的  */

    }
    else
    {
        bLogrpIsRestarted_g=TRUE;/* 表示有新逻辑图任务是通过restart启动的  */

    }

    taskDelay(SYS_SEC/10);/* 使原来的逻辑图任务自杀 */

    bLogrpIsRestarted_g=FALSE;


    /*  需清除原来逻辑图的采样注册函数 */
    for(i=0; i<ulLastRegisteSamFuncCount_g; i++)
    {
        RD_Del_Smpl_Func(LastRegisterSamFuncInfoArr_g[i].pfUser,
                         LastRegisterSamFuncInfoArr_g[i].pvParm,
                         LastRegisterSamFuncInfoArr_g[i].uiPts);

    }

    ulLastRegisteSamFuncCount_g=0;
    RE_SetRelayTaskCreateState(FALSE);

}





/* 保护功能模块的读取逻辑图文件之前的 全局初始化
   主要用于初始化节点连表及其他相关的初始化操作
   该函数在逻辑图文件读取之前调用
   参数  无
   返回值  无

 */
void   RE_SysInitBeforeReadFile()
{
    LIST  *pCurInitNodeList;
    LIST  *pCurScanNodeList;
    PARTGRP_ATTRIB_TYPE   *pPartGrpAttrib;

    int  i;
    for(i=0; i<MAX_RELAY_FUNC_COUNT ; i++)
    {
        /* 对连表数组进行循环 ,初始化连表为空*/
        pCurInitNodeList=RE_aPartGrpInitNodeList+i;
        pCurScanNodeList=RE_aPartGrpScanNodeList+i;
        pPartGrpAttrib=RE_aPartGrpAttribArr+i;
        RE_LstInit(pCurInitNodeList);
        RE_LstInit(pCurScanNodeList);
        pPartGrpAttrib->bRunFlag=FALSE;
        pPartGrpAttrib->uiScanDriveInterval=255;
        pPartGrpAttrib->ucScanAttr=0x00;
        pPartGrpAttrib->nScanTaskNo=0xFFFF;

        /* 初始化输入输出图元数组指针及个数 */
        pPartGrpAttrib->ppSetNode = NULL;
        pPartGrpAttrib->SetScanNodeNum = 0;

        pPartGrpAttrib->ppDINode = NULL;
        pPartGrpAttrib->DIScanNodeNum = 0;

        pPartGrpAttrib->ppDIModNode = NULL;
        pPartGrpAttrib->DIModScanNodeNum = 0;

        pPartGrpAttrib->ppDISetNode = NULL;
        pPartGrpAttrib->DISetScanNodeNum = 0;

        pPartGrpAttrib->ppFloatAINode = NULL;
        pPartGrpAttrib->FloatAIScanNodeNum = 0;

        pPartGrpAttrib->ppCmplxAINode = NULL;
        pPartGrpAttrib->CmplxAIScanNodeNum = 0;

        pPartGrpAttrib->ppFloatAISetNode = NULL;
        pPartGrpAttrib->FloatAISetScanNodeNum = 0;

        pPartGrpAttrib->ppCmplxAISetNode = NULL;
        pPartGrpAttrib->CmplxAISetScanNodeNum = 0;

        pPartGrpAttrib->ppEventNode = NULL;
        pPartGrpAttrib->EventScanNodeNum = 0;

        pPartGrpAttrib->ppDONode = NULL;
        pPartGrpAttrib->DOScanNodeNum = 0;

        pPartGrpAttrib->ppDOSetNode = NULL;
        pPartGrpAttrib->DOSetScanNodeNum = 0;

        pPartGrpAttrib->ppLampNode = NULL;
        pPartGrpAttrib->LampScanNodeNum = 0;

        pPartGrpAttrib->ppOnlyStartLuboNode = NULL;
        pPartGrpAttrib->OnlyStartLuboScanNodeNum = 0;

        pPartGrpAttrib->ppOnlyStopLuboNode = NULL;
        pPartGrpAttrib->OnlyStopLuboScanNodeNum = 0;

        pPartGrpAttrib->ppStratStopLuboNode = NULL;
        pPartGrpAttrib->StartStopLuboScanNodeNum = 0;

        pPartGrpAttrib->externImportNum = 0;
        pPartGrpAttrib->externExportNum = 0;

        pPartGrpAttrib->constSource = 0;
        pPartGrpAttrib->constZeroNum = 0;
        pPartGrpAttrib->constOneNum = 0;
        pPartGrpAttrib->optAoNum = 0;
    }
}



/* 保护功能模块的全局变量初始化
   主要用于比如信号量的初始化
   删除逻辑图图元初始化节点等功能
   该函数在逻辑图读取完成,并完成了初始化操作之后,扫描任务驱动之前调用
   参数  无
   返回值  无

 */
void   RE_SysInitAfterReadFile(LOGRP_ATTRIB_TYPE  *pGrpAttrib)
{

    SEM_ID   *pInvokeScanSemID;
    SEM_ID   *pAccessFlagSemID;
    BOOL  *pRefreshSettingFlag;
    BOOL   *  pScanningFlag;
    BOOL   * pRelaySemFreeFlag;
    int   i,k;
    int  nPartGrpCount;
    NODE   *pInitNode;
    unsigned  long   *pTaskNo;
    unsigned  long   *pTaskScanCounter;
    LIST  *pPartGrpInitNodeList;
    LIST  *pPartGrpScanNodeList;
    BOOL   * pbHwTestModeFlag;
    LOGRP_COMSUME_TIME_TYPE   *pCurTaskTime;
    TASK_OUT_CMD_STS_TYPE  *pCurTaskCmdSts;

    pPartGrpInitNodeList=RE_aPartGrpInitNodeList;
    pPartGrpScanNodeList=RE_aPartGrpScanNodeList;
    pInvokeScanSemID=RE_aInvokeLogrpNextScanSem_g;
    pAccessFlagSemID=RE_aAccessLogrpScaningFlagSem_g;
    pRefreshSettingFlag=RE_aGrpScanDingzhiRefreshFlag;
    pTaskNo=RE_aulLogrpScanTaskNo_g;
    pTaskScanCounter=RE_ulGrpScanTaskScanCounterArr_g;
    pScanningFlag=RE_abLogrpScaningFlag_g;
    pRelaySemFreeFlag=RE_RelayTaskDriveSemFirstFreeFlagArr_g;
    pbHwTestModeFlag=RE_bRelayTaskEnterHwTestModeFlagArr_g;
    pCurTaskTime=RE_aTaskPeriodTimeArr;/*2006-9-21日进行了修改，进行任务消耗时间的统计  */
    pCurTaskCmdSts=RE_aTaskOutCmdStsArr_g;/*2006-12-21日进行了修改，进行复归命令的状态维护  */

    /* 真正的本次RESTART全局变量初始化  */
    for(i=0; i<MAX_CREATE_RELAYFUNC_TASK_COUNT; i++)
    {
        /* 对于任务相关的全局变量初始化  */

        /* 初始化扫描驱动信号量为空 ,用来同步*/
        *pInvokeScanSemID=semCCreate(SEM_Q_FIFO,0);
        /* 初始化正在扫描标志访问信号量为满,用来互斥 */
        *pAccessFlagSemID=semBCreate(SEM_Q_FIFO,SEM_FULL);
        /* 初始化逻辑图扫描刷新定植标志为真,使得初次扫描时就更新定植 */
        *pRefreshSettingFlag=TRUE;
        /* 初始化逻辑图正在扫描标志为假,表示未进行扫描  */
        *pScanningFlag=FALSE;
        /* 初始化逻辑图首次释放信号标志为空，表示还没有释放过 */
        *pRelaySemFreeFlag=FALSE;
        /* 初始化任务号全局变量 */
        *pTaskNo=i;
        /* 初始化任务扫描计数器 */
        *pTaskScanCounter=0;
        /*初始化任务硬件测试进入状态标志  */
        *pbHwTestModeFlag=FALSE;
        /*  设置保护任务时间统计数据初值  2006-9-21  张云*/
        pCurTaskTime->iTaskNo=i;
        pCurTaskTime->bTaskIsRun=FALSE;/* 该任务是否已经被初次运行 */
        pCurTaskTime->ulCurTimePerPeriod=0;/*当前最新周期运行时间　*/
        pCurTaskTime->ulMinTimePerPeriod=0;/*最小每周期运行时间　*/
        pCurTaskTime->ulMaxTimePerPeriod=0;/*最大每周期运行时间　*/
        pCurTaskTime->ulAverageTimePerPeriod=0;/*平均每周期运行时间　*/

        pCurTaskTime->bBufIsFull=FALSE;/*缓冲已满标志  */
        pCurTaskTime->ullTotalTimeAllPeriod=0;/*总的运行时间  */
        pCurTaskTime->ulCurSavePos=0;/*当前存储位置  */
        pCurTaskCmdSts->bCurHasFgCmd=FALSE;/* 2006-12-21日 张云 */

        pInvokeScanSemID++;
        pAccessFlagSemID++;
        pRefreshSettingFlag++;
        pScanningFlag++;
        pTaskNo++;
        pRelaySemFreeFlag++;
        pTaskScanCounter++;
        pbHwTestModeFlag++;
        pCurTaskTime++;
        pCurTaskCmdSts++;
    }

    /*  删除所有逻辑分图的初始化扫描节点连表申请的内存 */
    nPartGrpCount=pGrpAttrib->nNodeListArrDims;
    for(k=0; k<nPartGrpCount; k++)
    {
        /* 对每个分图进行删除操作 */
        do
        {
            /* 删除节点 */
            pInitNode=RE_LstGet(pPartGrpInitNodeList);
            if(pInitNode!=NULL)
            {
                /*  释放节点的内存空间*/
                free(pInitNode->pTuyuan);
                free(pInitNode);
            }
        }
        while(pInitNode!=NULL); /* 直到连表为空 */

        RE_LstFree(pPartGrpInitNodeList);

        pPartGrpInitNodeList++;
    }

    /*2008-1-24日，张云merge修改，支持端口数据完整性后，不能去掉*/
#if  0
    /*删除所有分图中端口引入来源类型的外部输入图元  */
    for(k=0; k<nPartGrpCount; k++)
    {
        /* 对每个分图进行操作 */
        BOOL   bFindExternImport;
        int    nTuyuanCount;
        while(TRUE)
        {
            bFindExternImport=FALSE;
            nTuyuanCount=RE_LstCount(pPartGrpScanNodeList);
            pCurScanNode=RE_LstFirst(pPartGrpScanNodeList);
            for(m=0; m<nTuyuanCount; m++)
            {
                /* 对分图进行查找*/
                if(pCurScanNode->ulTuyuanType
                        ==RE_EXTERN_IMPORT_OUTERINPUT_SCAN)
                {
                    /* 若是外部引入图元，则删除 */
                    free(pCurScanNode->pTuyuan);
                    RE_LstDelete(pPartGrpScanNodeList, pCurScanNode);
                    bFindExternImport=TRUE;
                    break;
                }
                pCurScanNode=RE_LstNext(pCurScanNode);
            }
            if(!bFindExternImport)
            {
                break;

            }

        }
        pPartGrpScanNodeList++;
    }
#endif



}



/*     保护功能模块的逻辑图扫描任务创建后的操作。
       主要用于采样驱动函数注册,在扫描任务驱动之后调用
       参数   pGrpAttrib 逻辑图扫描属性指针

       返回值  无

 */
void   RE_OpeAfterLogrpScanTaskDrive(LOGRP_ATTRIB_TYPE  *pGrpAttrib)
{

    unsigned  long  *pLogrpScanTaskNo;
    unsigned  short  unGrpScanSamBeat;
    int  i;

    ulLastRegisteSamFuncCount_g=0;/*设置注册个数*/

    /* 注册保护采样节拍驱动函数 */
    for(i=0; i<pGrpAttrib->nAllowMaxTaskCount; i++)
    {
        pLogrpScanTaskNo=RE_aulLogrpScanTaskNo_g+i;

        if(pGrpAttrib->bGrpScanTaskCreateFlagArr[i])
        {
            /* 若该级任务能被创建，则注册相应驱动函数  */

            unGrpScanSamBeat=(unsigned  short)
                             (pGrpAttrib->uiGrpScanDriveSamPeriodIntervalArr[i]);
            RD_Reg_Smpl_Func(RE_SamDriver,(void  *)pLogrpScanTaskNo
                             ,unGrpScanSamBeat);

            /* 保存此次注册的函数信息  */
            LastRegisterSamFuncInfoArr_g[ulLastRegisteSamFuncCount_g]
            .pfUser=RE_SamDriver;
            LastRegisterSamFuncInfoArr_g[ulLastRegisteSamFuncCount_g]
            .pvParm=(void  *)pLogrpScanTaskNo;
            LastRegisterSamFuncInfoArr_g[ulLastRegisteSamFuncCount_g]
            .uiPts=unGrpScanSamBeat;

            ulLastRegisteSamFuncCount_g++;


        }

    }

    return;

}


/************************更新算法元件表的函数******************************/
/*
           功能：更新算法元件表的相关信息

           参数：  pSuanfaElemArr,算法表首地址
                   nSuanfaElemMapCount，算法表个数
                   pSuanfaDebugEntryFunc，算法调试入口函数
           返回值   EP_STATUS ；成功与否
*/


EP_STATUS   RE_Refresh_SuanfaTable_Info(EP_EXT_ELEM_MAP   *  pSuanfaElemArr,
                                        uint32_t  nSuanfaElemMapCount,
                                        EP_DEBUG_PART_FUNC_TYPE    pSuanfaDebugEntryFunc)
{
    SuanfaElemMapArrayAddr_g=pSuanfaElemArr;
    nSuanfaElemMapCount_g=nSuanfaElemMapCount;
    pSuanfaDebugEntryFunc_g=pSuanfaDebugEntryFunc;
    return   EP_SUCCESS;
}



/******************根据逻辑图扫描任务号获得扫描节拍的函数******************************/
/*
           功能：根据逻辑图任务号，获得该任务的AI扫描节拍

           参数：   ulScanTaskNo，扫描任务号

           返回值   该任务号的逻辑图的AI扫描节拍
*/



int  RE_Lgc_Scan_Interval(uint32_t ulScanTaskNo)
{
    int  nTaskInterval;
    if(ulScanTaskNo<LogrpAttrib_g.nAllowMaxTaskCount)
    {
        nTaskInterval=(int)
                      ((unsigned  long)
                       (LogrpAttrib_g.
                        uiGrpScanDriveSamPeriodIntervalArr[ulScanTaskNo]));
        return  nTaskInterval ;

    }
    else
    {
        LOG_Dbg_Msg("Error,Get Grp  Scan  Task'  Scan  Interval  failue  for  TaskNo isn't  Expected! \n",
                    0,0,0,0,0,0);

        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "逻辑图扫描错误:获得逻辑图扫描任务的扫描间隔错误\n",
                       0, 0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "logic grp scan err:get logic diagram scan task  interval  err\n",
                       0, 0);
        }
        assert(FALSE);
        return  0;


    }

}



/*     功能：设置保护任务创建状态,
       设置为真，表示保护任务已创建
       设置为假，表示保护任务还未创建
*/

void    RE_SetRelayTaskCreateState(BOOL  bCreateState)
{
    bAllRelayTaskCreateSuccess_g=bCreateState;
}

/*     功能：读取保护任务创建状态,
       返回为真，表示保护任务已创建
       返回为假，表示保护任务还未创建
*/

BOOL    RE_GetRelayTaskCreateState()
{
    return     bAllRelayTaskCreateSuccess_g;
}


/*      功能，获得保护任务运行状态，供看门狗检测使用
        参数，无
        返回：TRUE，表示保护任务工作正常
              FALSE，表示保护任务出现异常
*/
BOOL     RE_Get_Relay_Task_Run_State(
    uint8_t *pnRelayTaskNo,
    uint8_t *pnTskSts
)
{

    BOOL  bRelayTaskIsSuccessCreated;
    BOOL  bDeviceIsHwTested;
    BOOL  bTaskIsRunSuccess=TRUE;
    BOOL  *pbTaskRunFlag;
    BOOL  bTaskRunState;
    int  i;
    static   uint32_t  ulFirstEnterNotCreateTime;
    static   BOOL   bFirstEnterNotCreateFlag=FALSE;
    static   uint32_t  ulFirstEnterNotFreeSemTime;
    static   BOOL   bFirstEnterNotFreeSemFlag=FALSE;

    *pnTskSts=0;
    bRelayTaskIsSuccessCreated=RE_GetRelayTaskCreateState();
    if(!bRelayTaskIsSuccessCreated)
    {
        /* 若保护任务还未创建 */
        if(!bFirstEnterNotCreateFlag)
        {
            /*设置初次进入的时间  */
            bFirstEnterNotCreateFlag=TRUE;
            ulFirstEnterNotCreateTime=TM_Get_usCnt();
        }
        if(TM_Get_usCnt()-ulFirstEnterNotCreateTime>1200000000)
        {
            /*若保护任务20分钟内都没有创建,则认为出错  */
            *pnTskSts=0x1;
            *pnRelayTaskNo=0;

            return   FALSE;
        }
        return  TRUE;
    }
    else
    {
        if(!bFirstEnterNotFreeSemFlag)
        {
            /*设置初次进入的时间  */
            bFirstEnterNotFreeSemFlag=TRUE;
            ulFirstEnterNotFreeSemTime=TM_Get_usCnt();
        }

        pbTaskRunFlag=LogrpAttrib_g.bGrpScanTaskCreateFlagArr;
        for(i=0; i<MAX_CREATE_RELAYFUNC_TASK_COUNT; i++)
        {
            if((*pbTaskRunFlag)
                    &&(RE_RelayTaskDriveSemFirstFreeFlagArr_g[i])
                    &&(RE_ulGrpScanTaskScanCounterArr_g[i]<1))
            {
                /*若该任务被创建,且采样已经驱动,且逻辑图没有扫描,则认为出错  */
                *pnTskSts=0x2;
                bTaskIsRunSuccess=FALSE;
                *pnRelayTaskNo=i;
                break;
            }
            if((*pbTaskRunFlag)
                    &&(!(RE_RelayTaskDriveSemFirstFreeFlagArr_g[i]))
                    &&(TM_Get_usCnt()-ulFirstEnterNotFreeSemTime>600000000))
            {
                /*若任务已被创建,且10分钟内都没有被驱动,则认为出错  */
                *pnTskSts=0x3;
                bTaskIsRunSuccess=FALSE;
                *pnRelayTaskNo=i;
                break;
            }

            pbTaskRunFlag++;
        }
        bDeviceIsHwTested=EP_IN_HW_TEST();
        if((!bDeviceIsHwTested)
                &&(!bTaskIsRunSuccess))
        {
            /* 若保护不处于硬件测试态，并且保护任务运行不正常，则异常 */
            bTaskRunState=FALSE;
        }
        else
        {
            *pnTskSts=0;
            bTaskRunState=TRUE;
        }
        /* 清除扫描计数器 */
        for(i=0; i<MAX_CREATE_RELAYFUNC_TASK_COUNT; i++)
        {
            RE_ulGrpScanTaskScanCounterArr_g[i]=0;
        }
        return   bTaskRunState;
    }
}



/*      功能，获得保护任务挂起状态，供调试版本的保护任务被断点设置使用,2006-8-17日张云
        参数，无
        返回：TRUE，表示保护任务被挂起
              FALSE，表示保护任务正常未挂起
*/
BOOL     RE_RelayTaskIsSuspend()
{
    static  char   strTaskName[64];
    /*  任务名号数组*/
    static  char   astrTaskSeqNo[10][6]=
    {"0","1","2","3","4","5","6","7","8","9"};
    char  (* pstrTaskSeqNo)[6];
    int  iRelayTaskId;
    int  i;

    pstrTaskSeqNo=astrTaskSeqNo;

    /* 创建每个被投入的保护分图扫描任务 */
    for(i=0; i<6; i++)
    {
        /* 设置本任务名 */
        strcpy(strTaskName,"tLogicScan");
        strcat(strTaskName,(*pstrTaskSeqNo));
        iRelayTaskId=taskNameToId(strTaskName);
        if(iRelayTaskId!=ERROR)
        {
            /*若该保护任务存在，则检测该任务挂起与否  */
            if(RE_TaskIsSuspended(iRelayTaskId))
            {
                RE_aGrpScanTaskMsg[i].bDebugBeginFlag = TRUE;		/* DY 3/6/2007 */
                return  TRUE;
            }
            else
            {
                RE_aGrpScanTaskMsg[i].bDebugBeginFlag = FALSE;
            }
        }
        pstrTaskSeqNo++;
    }
    return   FALSE;
}

/*      功能，获得任务状态是否挂起
        参数，iTaskId，任务ID
        返回：TRUE，表示保护任务挂起
              FALSE，表示保护任务未挂起
*/
BOOL     RE_TaskIsSuspended(int  iTaskId)
{

    static  char  strTaskStatus[128];
    strTaskStatus[0]='\0';
    taskStatusString(iTaskId,strTaskStatus);
    if(strcmp(strTaskStatus,"SUSPEND")==0
            ||strcmp(strTaskStatus,"DELAY+S")==0
            ||strcmp(strTaskStatus,"PEND+S")==0
            ||strcmp(strTaskStatus,"PEND+S+T")==0
            ||strcmp(strTaskStatus,"SUSPEND+I")==0
            ||strcmp(strTaskStatus,"DELAY+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+T+I")==0)
    {
        return   TRUE;
    }
    else
    {
        return  FALSE;
    }
}

/***********************************************************************
* RE_StatTaskComsumeTime - 保护任务消耗时间统计计算
*
* RETURNS: 无
*
*/
void  RE_StatTaskComsumeTime(
    int iTaskNo,			/* 任务号 */
    uint32_t ulCurTaskTime				/* 此次保护任务运行消耗的时间 */
)
{

    LOGRP_COMSUME_TIME_TYPE *pTaskTime;

    pTaskTime=RE_aTaskPeriodTimeArr+iTaskNo;

#if 0
    if(!(pTaskTime->bTaskIsRun))
    {
        /* 若首次调用 */
        pTaskTime->bTaskIsRun=TRUE;
        pTaskTime->ulMinTimePerPeriod=ulCurTaskTime;
        pTaskTime->ulMaxTimePerPeriod=ulCurTaskTime;
    }
    if(pTaskTime->ulCurSavePos==(CALC_TASK_TIME_PER_PERIOD_COUNT-1))
    {
        if(!(pTaskTime->bBufIsFull))
        {
            pTaskTime->bBufIsFull=TRUE;
        }
    }
    /* 求最大值，最小值 */
    pTaskTime->ulCurTimePerPeriod=ulCurTaskTime;
    if(ulCurTaskTime<pTaskTime->ulMinTimePerPeriod)
    {
        pTaskTime->ulMinTimePerPeriod=ulCurTaskTime;
    }
    if(ulCurTaskTime>pTaskTime->ulMaxTimePerPeriod)
    {
        pTaskTime->ulMaxTimePerPeriod=ulCurTaskTime;
    }

    /* 求缓冲中的平均值  */
    pTaskTime->aulTimeBuf[pTaskTime->ulCurSavePos]=ulCurTaskTime;
    /* 下一个位置，即缓冲中最老的位置 */
    pTaskTime->ulCurSavePos++;
    pTaskTime->ulCurSavePos=(pTaskTime->ulCurSavePos)&(CALC_TASK_TIME_PER_PERIOD_COUNT-1);
    if(pTaskTime->bBufIsFull)
    {
        /* 若缓冲已满，则求平均值 */
        ulOldTime=pTaskTime->aulTimeBuf[pTaskTime->ulCurSavePos];
        pTaskTime->ullTotalTimeAllPeriod=pTaskTime->ullTotalTimeAllPeriod+(uint64_t)ulCurTaskTime-(uint64_t)ulOldTime;
        pTaskTime->ulAverageTimePerPeriod=(uint32_t)(pTaskTime->ullTotalTimeAllPeriod/(CALC_TASK_TIME_PER_PERIOD_COUNT-1));
    }
    else
    {
        pTaskTime->ullTotalTimeAllPeriod=pTaskTime->ullTotalTimeAllPeriod+(uint64_t)ulCurTaskTime;
        pTaskTime->ulAverageTimePerPeriod=(pTaskTime->ulMaxTimePerPeriod+pTaskTime->ulMinTimePerPeriod)/2;		/* 最大最小求平均DY 7/18/2007 */
    }
#endif

    if(!(pTaskTime->bTaskIsRun))
    {
        /* 若首次调用 */
        pTaskTime->bTaskIsRun=TRUE;
        pTaskTime->ullTotalTimeStat = 0;
    }

    /* 统计任务周期消耗 */
    taskLock();
    pTaskTime->ullTotalTimeStat += ulCurTaskTime;
    taskUnlock();
}

/* 保护任务消耗时间统计
 * Para:
 *     ulIntMs, ms统计间隔.
 * Return:
 *     NONE.
 */
void  RE_StatTaskComsumeTimeStat(uint32_t ulIntMs)
{
    int taskCount = 0;
    int taskArray[50] = {0};
    int i;
    LOGRP_COMSUME_TIME_TYPE *pTaskTime = NULL;
    int iLockKey;

    iLockKey = intLock();	/* 闭锁中断 */

    taskCount = RE_GetRunTaskCnt(&taskArray[0]);

    for (i = 0; i<taskCount; i++)
    {
        pTaskTime=RE_aTaskPeriodTimeArr+taskArray[i];
        pTaskTime->idlePercent = pTaskTime->ullTotalTimeStat/(ulIntMs*10);
        pTaskTime->ullTotalTimeStat = 0;
    }

    intUnlock(iLockKey);
}

/* 显示保护任务消耗时间统计
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void RE_ShowTaskComsumeTimeStat(void)
{
    int taskCount = 0;
    int taskArray[50] = {0};
    int i;
    LOGRP_COMSUME_TIME_TYPE *pTaskTime = NULL;
    static uint8_t aucLine[MAX_CREATE_RELAYFUNC_TASK_COUNT][INI_TAG_LEN];
    uint32_t qVal;
    uint32_t resVal;

    taskLock();
    taskCount = RE_GetRunTaskCnt(&taskArray[0]);

    for (i = 0; i<taskCount; i++)
    {
        pTaskTime=RE_aTaskPeriodTimeArr+taskArray[i];
        qVal = pTaskTime->idlePercent/10;
        resVal = pTaskTime->idlePercent%10;
        sprintf(aucLine[i], "%u.%u%%", (unsigned int)qVal, (unsigned int)resVal);
        LOG_Dbg_Msg("扫描任务%d: %s\n", taskArray[i], (int)aucLine[i], 0, 0, 0, 0);
        printf("扫描任务%d: %s\n", taskArray[i], aucLine[i]);

    }
    taskUnlock();
}

/***********************************************************************
* RE_GetRunTaskCnt - 获得运行的保护任务个数  2006-9-22日 供MMI调用，获得保护任务资源消耗功能使用
*
* RETURNS: 返回运行的保护任务个数
*
*/
int RE_GetRunTaskCnt(
    int *piRtRunTaskNoArr		/* 调用者传来的供返回运行保护任务号的数组基址，数组要足够大，比如8或16，由调用者提供该数组 */
)
{
    int i;
    int iRunTaskCnt=0;


    for(i=0; i<MAX_CREATE_RELAYFUNC_TASK_COUNT; i++)
    {
        if(RE_aTaskPeriodTimeArr[i].bTaskIsRun)
        {
            iRunTaskCnt++;
            *piRtRunTaskNoArr=RE_aTaskPeriodTimeArr[i].iTaskNo;
            piRtRunTaskNoArr++;
        }
    }
    return iRunTaskCnt;
}

/***********************************************************************
* RE_GetRunTaskConsumeResource - 获得某运行的保护任务资源消耗2006-9-22日 供MMI调用，获得保护任务资源消耗功能使用
*
* RETURNS: 无
*
*/
void RE_GetRunTaskConsumeResource(
    int iRunTaskNo,		/* 待访问的保护任务号 */
    RE_TASK_COMSUME_RESOURCE_TYPE *pRtTaskResource				/* 供返回该任务的资源消耗，由调用者提供 */
)
{
    assert(iRunTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pRtTaskResource->iTaskNo=RE_aTaskPeriodTimeArr[iRunTaskNo].iTaskNo;
    pRtTaskResource->ulCurTimePerPeriod=RE_aTaskPeriodTimeArr[iRunTaskNo].ulCurTimePerPeriod;
    pRtTaskResource->ulMinTimePerPeriod=RE_aTaskPeriodTimeArr[iRunTaskNo].ulMinTimePerPeriod;
    pRtTaskResource->ulMaxTimePerPeriod=RE_aTaskPeriodTimeArr[iRunTaskNo].ulMaxTimePerPeriod;
    pRtTaskResource->ulAverageTimePerPeriod=RE_aTaskPeriodTimeArr[iRunTaskNo].ulAverageTimePerPeriod;
}


void  RE_TaskAllImportTuyuanScan(int  nScanTaskNo)
{
    int  iCurTaskImportCnt;   /*2006-11-6日 张云修改  */
    NODE ** ppCurTaskImportNodeArr;  /*2006-11-6日 张云修改  */
    int  i;

    taskLock();/*为保证任务间数据访问的完整性，需要任务保护  */

    iCurTaskImportCnt=LogrpAttrib_g.iTaskImortScanNodeCntArr[nScanTaskNo];
    ppCurTaskImportNodeArr=LogrpAttrib_g.ppTaskImportScanNodeArr[nScanTaskNo];
    for(i=0; i<iCurTaskImportCnt; i++)
    {
        RE_ExternImportOuterInputTuyuanScan(ppCurTaskImportNodeArr[i]);
    }

    taskUnlock();

    return  ;
}

/*扫描某扫描任务的所有端口引出图元  2006-11-6日张云
  参数  nScanTaskNo，扫描任务号
  返回，无
*/
void  RE_TaskAllExportTuyuanScan(int  nScanTaskNo)
{
    int  iCurTaskExportCnt;   /*2006-11-6日 张云修改  */
    NODE ** ppCurTaskExportNodeArr;  /*2006-11-6日 张云修改  */
    int  i;


    taskLock();/*为保证任务间数据访问的完整性，需要任务保护  */

    iCurTaskExportCnt=LogrpAttrib_g.iTaskExportScanNodeCntArr[nScanTaskNo];
    ppCurTaskExportNodeArr=LogrpAttrib_g.ppTaskExportScanNodeArr[nScanTaskNo];
    for(i=0; i<iCurTaskExportCnt; i++)
    {
        RE_ExternExportOuterOutputTuyuanScan(ppCurTaskExportNodeArr[i]);
    }

    taskUnlock();

    return  ;

}

/*扫描某扫描任务的所有光纵AO输出图元  2006-11-11日张云
  参数  nScanTaskNo，扫描任务号
        ulTaskScanAiCnt,该任务的扫描时的AICNT
  返回，无
*/
void  RE_TaskAllOptAOTuyuanScan(int  nScanTaskNo,uint32_t  ulTaskScanAiCnt)
{
    int  iCurTaskOptAOCnt;   /*2006-11-11日 张云修改  */
    NODE ** ppCurTaskOptAONodeArr;  /*2006-11-11日 张云修改  */
    int  i;


    taskLock();/*为保证光纵访问时，数据访问的完整性，需要任务保护  */

    iCurTaskOptAOCnt=LogrpAttrib_g.iTaskOptAOScanNodeCntArr[nScanTaskNo];
    ppCurTaskOptAONodeArr=LogrpAttrib_g.ppTaskOptAOScanNodeArr[nScanTaskNo];
    for(i=0; i<iCurTaskOptAOCnt; i++)
    {
        RE_OptAoOuterOutputTuyuanScan(ppCurTaskOptAONodeArr[i]);
    }

    if(nScanTaskNo==0)
    {
        OPT_SetFastLogrpTaskSendInfo(nScanTaskNo,ulTaskScanAiCnt);
    }

    taskUnlock();

    return  ;

}



/***********************************************************************
* RE_FastTaskIsDrived - 快速保护任务是否已经被驱动 2006-12-3日张云
*
* RETURNS:
*				   TRUE，快速任务已经被驱动
*					FALSE，表示快速任务未被驱动
*
*/
BOOL RE_FastTaskIsDrived()
{
    return bFastTaskIsDrived_g;
}


/***********************************************************************
* RE_SetAllTaskFgSts - 设置所有保护任务处于复归状态，在复归命令中调用
*
* RETURNS: 无
*
*/
void RE_SetAllTaskFgSts()
{
    TASK_OUT_CMD_STS_TYPE *pCurTaskCmdSts;
    int i;

    pCurTaskCmdSts=RE_aTaskOutCmdStsArr_g;		/* 2006-12-21日进行了修改，进行复归命令的状态维护 */

    taskLock();
    for(i=0; i<MAX_CREATE_RELAYFUNC_TASK_COUNT; i++)
    {
        pCurTaskCmdSts->bCurHasFgCmd=TRUE;		/* 2006-12-21日 张云 */
        pCurTaskCmdSts++;
    }
    taskUnlock();
}

/***********************************************************************
* RE_GetTaskCurFgSts - 获得保护任务当前是否处于复归状态，逻辑图扫描时访问
*
* RETURNS:
*				   TRUE，当前该任务有复归命令
*					FALSE，当前该任务无复归命令
*
*/
BOOL RE_GetTaskCurFgSts(
    int iScanTaskNo		/* 保护任务号 */
)
{
    assert(iScanTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    return RE_aTaskOutCmdStsArr_g[iScanTaskNo].bCurHasFgCmd;
}

/***********************************************************************
* RE_ClearTaskFgSts - 清除保护任务的复归状态，逻辑图扫描时访问
*
* RETURNS: 无
*
*/
void RE_ClearTaskFgSts(
    int iScanTaskNo			/* 保护任务号 */
)
{
    assert(iScanTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    RE_aTaskOutCmdStsArr_g[iScanTaskNo].bCurHasFgCmd=FALSE;
}

/***********************************************************************
* RE_IncSetAtSetCnt - 增加任务自动整定定值计数
*
* RETURNS: 无
*
*/
void RE_IncSetAtSetCnt(void)
{
    taskLock();
    RE_ulRelayTaskSetAtSetCnt_g++;		/* 递增 */
    taskUnlock();
}

/***********************************************************************
* RE_JdSetAtSetCnt - 判断任务自动整定定值计数是否非零
*
* RETURNS: 无
*
*/
void RE_JdSetAtSetCnt(void)
{
    SLOW_MESSAGE_NODE Info;

    taskLock();
    if(RE_ulRelayTaskSetAtSetCnt_g>0)
    {
        RE_ulRelayTaskSetAtSetCnt_g=0;		/* 每次扫描判断一次 */
        Info.type=SETAUTOWR;
        msgQSend(SlowMessage, (char *)&Info, sizeof(SLOW_MESSAGE_NODE), NO_WAIT, MSG_PRI_NORMAL);
    }
    taskUnlock();
}

/* 设置任务DI更新计数.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void RE_SetLogDIUpdateCnt(void)
{
    int i;

    taskLock();
    for (i = 0; i<LogrpAttrib_g.nAllowMaxTaskCount; i++)
    {
        /* 与滞后判断配合,增加一个扫描间隔 */
        RE_aGrpScanTaskMsg[i].ulDiUpdateCnt = MAX_LOGIC_SCAN_DELAY_SAMPLE_COUNT
                                              +LogrpAttrib_g.uiGrpScanDriveSamPeriodIntervalArr[i];
    }
    taskUnlock();
}

/* 获取任务滞后状态.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL RE_GetLogDelaySts(void)
{
    int i;
    uint32_t ulDspAICnt;
    static uint32_t ulDelaySts = 0;
    uint32_t ulDif = 0;

    ulDspAICnt = RD_AI_Cnt();

    for (i = 1; i<LogrpAttrib_g.nAllowMaxTaskCount; i++)
    {
        /* 扫描任务0不判断 */
        ulDif = ulDspAICnt-RE_aGrpScanTaskMsg[i].ulUserSetAiCnt;

        if ((ulDif >= DELAY_LOGIC_SCAN_DELAY_SAMPLE_COUNT)
                && LogrpAttrib_g.bGrpScanTaskScannedFlagArr[i])
        {
            /* 滞后3个周波且该任务已扫描, 后一个条件除初始化外后续都满足 */
            ulDelaySts = LOGIC_DELAY_TWO_LEVEL;
            break;
        }
        else if ((ulDif >= ALARM_LOGIC_SCAN_DELAY_SAMPLE_COUNT)
                 && LogrpAttrib_g.bGrpScanTaskScannedFlagArr[i])
        {
            /* 滞后2个周波且该任务已扫描, 后一个条件除初始化外后续都满足 */
            ulDelaySts = LOGIC_DELAY_ONE_LEVEL;
            break;
        }
        else
        {
            ulDelaySts = 0;
        }
    }

    return ulDelaySts;
}

/* 设置定值更新计数(按任务).
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void RE_SetLogSetChgCnt(void)
{
    int i;

    /* 设置同步脉冲标识为等待
     * 允许多次设置
     */
    TM_SetSecPluseSts(FALSE);

    taskLock();
    for (i = 0; i<LogrpAttrib_g.nAllowMaxTaskCount; i++)
    {
        RE_aGrpScanTaskMsg[i].ulSetChgCnt++;
    }
    taskUnlock();
}

/* 获取定值更新状态.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL RE_GetLogSetChgSts(void)
{
    int i;

    for (i = 0; i<LogrpAttrib_g.nAllowMaxTaskCount; i++)
    {
        if (RE_aGrpScanTaskMsg[i].ulSetChgCnt)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/* 更新任务DI计数.
 * Para:
 *     pScanTaskMsg, scan logic task.
 *     ulGrpScanDriveInterval, interval.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL RE_UpdateLogDICnt(EP_CHART_MSG *pScanTaskMsg, uint32_t ulGrpScanDriveInterval)
{
    BOOL bRet = FALSE;

    taskLock();
    if (pScanTaskMsg->ulDiUpdateCnt)
    {
        bRet = TRUE;
        /* 防止扫描间隔不能整除周波采样点数 */
        if (pScanTaskMsg->ulDiUpdateCnt >= ulGrpScanDriveInterval)
        {
            pScanTaskMsg->ulDiUpdateCnt -= ulGrpScanDriveInterval;
        }
        else
        {
            pScanTaskMsg->ulDiUpdateCnt = 0;
        }
    }
    taskUnlock();

    return bRet;
}
/* 得到DI是否更新过标志 2011-8-5日ZY
 * 参数:pScanTaskMsg 任务信息

 * 返回:
 *     TRUE:DI近期更新过,需要重新刷新
       FALSE;DI长时间未更新,可以不刷新
 */
BOOL RE_GetDIUpdatedFlag(EP_CHART_MSG *pScanTaskMsg)
{
    if (pScanTaskMsg->ulDiUpdateCnt)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
