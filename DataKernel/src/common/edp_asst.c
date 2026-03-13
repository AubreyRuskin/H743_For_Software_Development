/* edp_asst.c - subroutine library for handling the assistant operation */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 20may09, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling the assistant operation.
INCLUDES: edp_asst.h
*/

/* includes */

#include "edpbase.h"
#include "logmsg.h"
#include "errtest.h"
#include "view.h"
#include "sysinfo.h"
#include "filetool.h"

#include <stdio_compat.h>
#include <taskLib.h>
#include <logLib.h>
#include <intLib.h>
// #include <rebootLib.h>
// #include <drv/mem/m8260Siu.h>
#include "UsbMmiInterface.h"
#include "configerrordisp.h"
#include "detailoperate_log.h"
#include "tickLib.h"
#include "excLib.h"
// #include "arch\ppc\esfPpc.h"
// #include "config04.h"
#include "bspinterface.h"
#include "edp_asst.h"
#include "protectmmiinterface.h"
#include "smvcfg.h"
#include "HardClock.h"
// #include "Gmrp.h"
#include "FileCRC.h"
#include "Festofftime.h"
#include "Smv_Go_CommStat_File.h"
#include "symLib.h"
#include "iecgoose.h"
#include "math_compat.h"
#include "EDP_UnifiedCfgInterface.h"
#include "taskLib.h"

/* defines */
#define CC_SFP_PTL_10_VER 0x10
#define CC_SFP_PTL_11_VER 0x11
#define ONE_HOUR_CNT (6*60*24) /* 1天计数 */

/* locals */

static TASK_ID nMainLoopTaskID_g;		/* 慢速主循环任务 */
static BOOL bMainLoopSlowTaskStartFlag_g = FALSE; /* 慢速主循环任务启动标志 */
static uint8_t aucDevName_g[MAX_ID_LEN+1] = "EDP_RELAY";  /* 保护设备名称，可设置 */

/* globals */

extern BOOL bGetAbsTime;
extern uint32_t GetAbsTimeInterval;
extern BOOL bGetPulseSetTime;
extern uint32_t GetPulseSetTimeInterval;
extern uint8_t ucPulseType_g;
uint32_t g_ulGsFrmCnt;  /* GOOSE报文处理总帧数 */

BOOL bConnectMmiSuccessFlag = FALSE ;
LIST m_WatchDogTaskList_g;
uint8_t ucCPUSeq_g = 0;		/* CPU位置序号，0: 第一块CPU；1: 第2块CPU */
T_CC_STS arrCcPortSts[MAX_CC_BOARD_NUM];  /* CC端口状态 */
uint8_t g_ucCCSnToArr[MAX_CC_BOARD_ID_NUM];
BOOL g_bWriteLogFlag[MAX_CC_BOARD_NUM];		/*表示该板子的版本号是否已写日志*/
uint8_t g_ucCCNum;  /* 支持CC个数 */
BOOL g_bStormState[MAX_CC_BOARD_NUM] = {FALSE, FALSE, FALSE, FALSE,
                                        FALSE, FALSE, FALSE, FALSE,FALSE, FALSE, FALSE, FALSE,FALSE, FALSE, FALSE, FALSE
                                       };		/*是否进入抑制状态*/
uint16_t g_bStormOffset[MAX_CC_BOARD_NUM];	/*风暴位的偏移*/
uint16_t g_usStormOffSet;	/*计算用*/
uint8_t g_ucBoardIdtoCcStsNo[MAX_CC_BOARD_ID_NUM] = {0};/* 通过BoardId板件号获取CC状态结构序号(光功率结构) */

/* 全局CPU风暴抑制状态 */
BOOL g_bCPU1StormState = FALSE;
BOOL g_bCPU2StormState = FALSE;

/* global functions */

BOOL isNumber_2_04CPU(void)
{
    return FALSE;
}


#if defined(EDP_01_02_BUILD)
extern BOOL GetConnectMmiSuccessFlag();
#endif

extern void HDL_Change_Filt(BOOL bStorm);

/***********************************************************************
* GetAdjustTimeSuccessFlag - MMI_SOFT.C提供的对时成功标志,
*
* RETURNS:
*					TRUE: 04板初始化后对时成功
*					FALSE: 04板初始化后还没有对时成功
*
*/
extern BOOL GetAdjustTimeSuccessFlag();

/***********************************************************************
* SetAdjustTimeSuccessFlag - 设置系统对时标志
*
* RETURNS: 无
*
*/
extern void SetAdjustTimeSuccessFlag(
    BOOL bOkFlag
);

/***********************************************************************
* SetExtboxLanguageType - 设置扩展机箱语言类型
*
* RETURNS: 无
*
*/
extern void SetExtboxLanguageType(void);

/* static functions */

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
static BOOL GetConnectMmiSuccessFlag(void);
#endif

/***********************************************************************
* MainLoop_Process -慢速主循环任务入口
*
* RETURNS: 无
*
*/
static void MainLoop_Process(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
);

/***********************************************************************
* EP_Set02CPUSts - 设定第2块CPU标志
*
* RETURNS: NONE
*
*/
static void EP_Set02CPUSts(
    uint8_t ucSeq		/* CPU位置序号，0: 第1块CPU；1: 第2块CPU(测控CPU) */
);

/***********************************************************************
* EP_SetPwrFreq - Set the system frequency
*
* RETURNS:
*         EP_SUCCESS: Normal
*         EP_ERROR: Error
*
*/
static EP_STATUS EP_SetPwrFreq(
    int32_t uFreqType			/* 0: 50Hz 1: 60Hz */
);

/***********************************************************************
* GetTaskName - 得到相应序号的任务的名称
*
* RETURNS: 名称
*
*/
static char *GetTaskName(
    int32_t index
);

/* functions */

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
/***********************************************************************
* GetConnectMmiSuccessFlag -获取MMI是否初始化完成标志
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL GetConnectMmiSuccessFlag(void)
{
    return  bConnectMmiSuccessFlag;
}
#endif

/***********************************************************************
* MonitorInit - 慢速处理任务初始化
*
* RETURNS: 无
*
*/
void SlowTaskProcessInit()
{
    /* 慢速主循环任务创建，所有平台都建立该任务，以处理逻辑图发出的慢速事务 */
    nMainLoopTaskID_g = taskSpawn("tMainLoop",
                                  TSK_PRI_MAINLOOP,
                                  VX_FP_TASK|VX_DEALLOC_STACK,
                                  10000,
                                  (FUNCPTR)MainLoop_Process,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    assert (nMainLoopTaskID_g != ERROR);
    bMainLoopSlowTaskStartFlag_g = TRUE;

    AddTaskToList(nMainLoopTaskID_g, TRUE,
                  "看门狗复位:因慢速循环任务异常或退出,看门狗复位CPU.\n", TRUE);
}

/***********************************************************************
* MainLoop_Process -慢速主循环任务入口
*
* RETURNS: 无
*
*/
static void MainLoop_Process(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
)
{
#define INTERVALTICKNUM 2

    UINT8 numRecv;
    SLOW_MESSAGE_NODE Info;
    BOOL bResultFlag = FALSE;
    int iCount;
    int iCycleNum = 0;
    static uint32_t preTime = 0;

    uint32_t ulBeginTime = 0;
    uint8_t aucBuf[64];

    static uint32_t preOffTime = 0;
    uint32_t nowTime = 0;
    STATUS bSts = ERROR;
    T_WATT_DATA_ELE *pCur = NULL;  /* 临时写入点指针 */

    EP_DATE_TIME dttm;
    EP_STATUS retcode;

    ulBeginTime = tickGet();

    /* 第一次获取主CPU板光功率 */
    update_sfp_info();

    while (1)
    {
        numRecv = msgQReceive(SlowMessage, (char *)&Info, sizeof(SLOW_MESSAGE_NODE), NO_WAIT);

        if (numRecv == sizeof(SLOW_MESSAGE_NODE))
        {
            LOG_Dbg_Msg("SlowMessage Message coming %d.\n", Info.type, 0, 0, 0, 0, 0);

            iCount = 0;
            if (Info.type == ACMDCHG)
            {
                while (!bResultFlag)    /* 设置模件类型 */
                {
                    iCount++;
                    if (EP_SetAcMdType(Info.param1) == EP_SUCCESS)
                    {
                        if (ENG_MODE == 0)
                        {
                            if (Info.param1 == 0)
                            {
                                LOG_Write(LOG_KERNEL, "切换至1A交流模件.\n", NULL);
                            }
                            else if (Info.param1 == 1)
                            {
                                LOG_Write(LOG_KERNEL, "切换至5A交流模件.\n", NULL);
                            }
                        }
                        else if (ENG_MODE == 1)
                        {
                            if (Info.param1 == 0)
                            {
                                LOG_Write(LOG_KERNEL, "Change to 1A AC module.\n", NULL);
                            }
                            else if (Info.param1 == 1)
                            {
                                LOG_Write(LOG_KERNEL, "Change to 5A AC module.\n", NULL);
                            }
                        }
                        bResultFlag = TRUE;
                    }

                    if (iCount>5)
                    {
                        break;
                    }
                    taskDelay(10);
                }

                if (!bResultFlag)
                {
                    /* 写入没有成功 */
                    if (ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SET_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                                   "错误码:%02d\n", INDEX_SETTING_PAGE_NUM_WRITE_ERR, 0);
                        LOG_Write(LOG_KERNEL, "索引定值页序写入edp01.ini文件出错!!", NULL);
                    }
                    else if (ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SET_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                                   "Error code:%02d\n", INDEX_SETTING_PAGE_NUM_WRITE_ERR, 0);
                        LOG_Write(LOG_KERNEL, "Index setting page number writing to edp01.ini error!!", NULL);
                    }
                }
                else
                {
                    bResultFlag = FALSE;
                    iCount = 0;
                    while (!bResultFlag)   /* 设置模件类型改变标志 */
                    {
                        iCount++;
                        if (EP_SetAcMdTypeChgFlag(1) == EP_SUCCESS)
                        {
                            bResultFlag = TRUE;
                            AdMdType.bValid = TRUE;  /* 写入有效,下次写同样页序时不用操作 */
                        }

                        if (iCount>5)
                        {
                            if (!bResultFlag)
                            {
                                /* 写入没有成功 */
                                if (ENG_MODE == 0)
                                {
                                    ER_Set_Err(EV_SET_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                                               "错误码:%02d\n", INDEX_SETTING_PAGE_NUM_CHG_FALG_WRITE_ERR, 0);
                                    LOG_Write(LOG_KERNEL, "索引定值页序更改标志写入edp01.ini文件出错!!", NULL);
                                }
                                else if (ENG_MODE == 1)
                                {
                                    ER_Set_Err(EV_SET_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                                               "Error code:%02d\n", INDEX_SETTING_PAGE_NUM_CHG_FALG_WRITE_ERR, 0);
                                    LOG_Write(LOG_KERNEL, "Changing flag of index setting page number writing to edp01.ini error!!", NULL);
                                }
                            }
                            break;
                        }
                        taskDelay(10);
                    }
                }
            }
            else if (Info.type == FREQCHG)
            {
                while (!bResultFlag)	/* 设置系统频率 */
                {
                    iCount++;
                    if (EP_SetPwrFreq(Info.param1) == EP_SUCCESS)
                    {
                        bResultFlag = TRUE;
                    }

                    if (iCount>5)
                    {
                        break;
                    }
                    taskDelay(10);
                }

                if (bResultFlag)
                {
                    if (ENG_MODE == 0)
                    {
                        if (Info.param1 == 0)
                        {
                            LOG_Write(LOG_KERNEL, "因系统切换至50Hz运行环境，装置重启.\n", NULL);
                        }
                        else if (Info.param1 == 1)
                        {
                            LOG_Write(LOG_KERNEL, "因系统切换至60Hz运行环境，装置重启.\n", NULL);
                        }
                    }
                    else if (ENG_MODE == 1)
                    {
                        if (Info.param1 == 0)
                        {
                            LOG_Write(LOG_KERNEL, "Because of switching to 50Hz system,the equipment restarting,Please waiting.\n", NULL);
                        }
                        else if (Info.param1 == 1)
                        {
                            LOG_Write(LOG_KERNEL, "Because of switching to 60Hz system,the equipment restarting,Please waiting.\n", NULL);
                        }
                    }

                    while (iCycleNum<12000 && (!GetConnectMmiSuccessFlag()))
                    {
                        /* 延迟120秒或对时成功, 则退出循环 */
                        iCycleNum++;
                        taskDelay(1);
                    }

                    iCycleNum = 0;

                    taskDelay(500);	/* 等待界面起来 */

                    EDPreboot(REBOOT_ACTIVE);
                }
            }
            else if (Info.type == LANGUAGECHG)
            {
                LOG_Dbg_Msg("更改扩展机箱语言类型.\n", 0, 0, 0, 0, 0, 0);

                /* EDP01平台更改扩展机箱语言类型,继续支持
                 */
                if (bdType_g == BOARD_TYPE_E01)
                {
                    SetExtboxLanguageType();
                }

            }
            else if (Info.type == SETAUTOWR)
            {
                LOG_Dbg_Msg("自动整定定值.\n", 0, 0, 0, 0, 0, 0);
                SI_Wr_New_CK_Set();

                /* 防止重复整定 */
                /* RE_SetLogSetChgCnt(); */
            }
            else if (Info.type == SETAREACHG)
            {
                /* 切换定值区 */
                if (SC_Is_Valid_Area(Info.param1) == FALSE)
                {
                    /* 检验正确性 */
                    if (ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SECT_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                                   "%02d#定值区无效\n", Info.param1, 0);
                    }
                    else if (ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SECT_ERR,  ER_REPORT|ER_ALARM|ER_LOCK,
                                   "SG %02d\n", Info.param1, 0);
                    }
                }

                sprintf(aucBuf,EP_SET_AREA_DIR "/area%02x.dza",Info.param1);
                if(Check_Areaset_CRC(aucBuf))
                {
                    if (SC_Chg_Work_Area(Info.param1) != EP_SUCCESS)
                    {
                        /* 更换定值区 */
                        if (ENG_MODE == 0)
                        {
                            ER_Set_Err(EV_SECT_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                                       "%02d#定值区切换失败\n", Info.param1, 0);
                        }
                        else if (ENG_MODE == 1)
                        {
                            ER_Set_Err(EV_SECT_ERR,  ER_REPORT|ER_ALARM|ER_LOCK,
                                       "SG %02d switch error\n", Info.param1, 0);
                        }
                    }
                }
            }
            else if(Info.type == MUDELAYWR)/*ZQ 2011-02-22 增加写合并单元延时到文件*/
            {
                char t[50];
                char item[50];
                sprintf(t, "%dus", Info.MuDelay);
                sprintf(item, "Delay_%d", Info.MuTypeNo);
                FT_Wr_DSamSts_INI("[MUDELAY]",item,t);
            }
            else if(Info.type == MUNAMEWR)/*ZQ 2011-02-22 增加写合并单元名称到文件*/
            {
                char item[50];
                sprintf(item, "Name_%d", Info.MuTypeNo);
                FT_Wr_DSamSts_INI("[MUDELAY]",item,Info.smvDes);
            }
            else if (Info.type == CT_PT_RATE_VAL_WR)  /* 一次额定值 */
            {
                LOG_Dbg_Msg("一次额定值写入!\n", 0, 0, 0, 0, 0, 0);
                RD_New_CT_Ratio();  /* 生成新的CT系数 */
            }
            else if (Info.type == SETTING_RANGE_SET)  /* 设置定值的最大值、最小值、默认值 */
            {
                /* 生成新的定值量程文件 */
                SC_New_SET_Range();

                /* 设置模件修改标识，用于重启时重新生成sci文件 */
                if(EP_SetAcMdTypeChgFlag(1) != EP_SUCCESS)
                {
                    LOG_Write(LOG_RUN, "设置模件修改标识失败.\n", NULL);
                }

                if(ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "设置定值的量程完成.\n", NULL);
                }
                else if(ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL, "Set the  range of setting.\n", NULL);
                }

                SC_Set_RangeFile_OverFlg(TRUE);  /* 设置定值文件生成结束标识 */
                SC_Set_ChgCnt_Plus();   /* 设置量程调整计数自增 */
                SC_Chk_Range_All_Valid();  /* 检查定值的有效性 */
            }

            bResultFlag = FALSE;
        }

#if defined(EDP_01_02_BUILD)
        if (adjinfo_g.bAdjustRunFlag)
        {
            if ((tickGet()-adjinfo_g.uAdjustStartTickNum)>1000)
            {
                /* 10秒钟得不到响应 */
                adjinfo_g.bAdjustRunFlag = FALSE;
                if (ENG_MODE == 0)
                    ER_Set_Err(MMI_S_AE_HINT_INFO,
                               ER_REPORT,
                               "调校命令执行失败\n", 0, 0);
                else if (ENG_MODE == 1)
                    ER_Set_Err(MMI_S_AE_HINT_INFO,
                               ER_REPORT,
                               "Calibration excute error\n", 0, 0);

            }
        }

        if ((adjoverinfo_g.bVal) && (adjoverinfo_g.ucObjType == 1))
        {
            /* 测量校准 */
            adjinfo_g.bAdjustRunFlag = FALSE;		/* 发出的校准命令得到响应 */
            adjoverinfo_g.bVal = FALSE;
            if (ME_Create_CoffFile(adjoverinfo_g.ucOrdType) == EP_SUCCESS)
            {
            }
            else
            {
                if (ENG_MODE == 0)
                    ER_Set_Err(MMI_S_AE_HINT_INFO,
                               ER_REPORT,
                               "测量调校命令执行失败\n", 0, 0);
                else if (ENG_MODE == 1)
                    ER_Set_Err(MMI_S_AE_HINT_INFO,
                               ER_REPORT,
                               "Measurment calibration excute error\n", 0, 0);
            }
        }
        else if ((adjoverinfo_g.bVal) && (adjoverinfo_g.ucObjType == 0))
        {
            /* AI通道校准, 保护校准 */
            adjinfo_g.bAdjustRunFlag = FALSE;		/* 发出的校准命令得到响应 */
            adjoverinfo_g.bVal = FALSE;

            if (WR_Cfg_Hw_AI_Gain(adjoverinfo_g.ucOrdType) == EP_SUCCESS)
            {
            }
            else
            {
                if (ENG_MODE == 0)
                    ER_Set_Err(MMI_S_AE_HINT_INFO,
                               ER_REPORT,
                               "保护调校命令执行失败\n", 0, 0);
                else if (ENG_MODE == 1)
                    ER_Set_Err(MMI_S_AE_HINT_INFO,
                               ER_REPORT,
                               "Relay calibration excute error\n", 0, 0);

            }
        }
#endif

        if (bdType_g == BOARD_TYPE_E02)
        {
            /* EDP02平台写铁电 */
            if ((tickGet()-preTime) >= 20)	/* 200ms timer */
            {
                /* store the PO. */
                PoWrFile();
                preTime = tickGet();
            }
        }

#if defined(EDP_01_02_BUILD)
#ifdef SUBUNIT
        if (!GetAdjustTimeSuccessFlag())		/* 设置对时标志 */
            SetAdjustTimeSuccessFlag(TRUE);
#else
        if (!GetAdjustTimeSuccessFlag())		/* 设置对时标志 */
        {
            if (tickGet() - ulBeginTime >= 180*(100/INTERVALTICKNUM))
            {
                /* 3分钟内对时不成功则使用默认时间 */
                SetAdjustTimeSuccessFlag(TRUE);
            }
        }
#endif
#endif
        {
            static uint32_t preTime=0;
            if((sXmlCfg.smvCfg.synMode==EXTERNAL_1588_0_ADJ)
                    ||(sXmlCfg.smvCfg.synMode==EXTERNAL_1588_1_ADJ)
                    ||(sXmlCfg.smvCfg.synMode==EXTERNAL_1588_2_ADJ)
                    ||(sXmlCfg.smvCfg.synMode==EXTERNAL_1588_3_ADJ))
            {
                if((tickGet()-preTime)>=4500)//45秒
                {
                    if(!Time_Adjust_F_A_1588_Task())
                    {
                        if(Change_TimeSycAndInterPulseMode())
                        {
                            Time_Adjust_F_A_1588_Task();/*切换成功，则再次对时*/
                        }
                    }
                    preTime=tickGet();
                }
            }
        }


        /* 对时与同步判断 */
        {
            BOOL NeedtoSycnfrom3231 = FALSE;  /* 是否需要时钟芯片对时 */

            switch(sXmlCfg.smvCfg.synMode)
            {
                case EXTERNAL_HMI_ADJ:
                case INTERNAL_DEFAULT:

                    /* 如此处理存在隐患
                     * 无对时时出现短时对时成功的现象
                     * 以下同
                     */
                    if((tickGet()-GetAbsTimeInterval)<400000u)/*66.67分*/
                    {
                        bGetAbsTime=TRUE;
                        SetAdjustTimeSuccessFlag(TRUE);
                    }
                    else
                    {
                        bGetAbsTime=FALSE;
                        NeedtoSycnfrom3231=TRUE; /* 需要读取时钟芯片 */
                    }

                    if(ucPulseType_g==GPS_PULSE_TYPE_SEC)
                    {
                        if((tickGet()-GetPulseSetTimeInterval)<2000u)/*20秒*/
                        {
                            bGetPulseSetTime=TRUE;
                        }
                        else
                        {
                            bGetPulseSetTime=FALSE;
                        }
                    }
                    else if(ucPulseType_g==GPS_PULSE_TYPE_MIN)
                    {
                        if((tickGet()-GetPulseSetTimeInterval)<14000u)/*3分*/
                        {
                            bGetPulseSetTime=TRUE;
                        }
                        else
                        {
                            bGetPulseSetTime=FALSE;
                        }
                    }
                    else
                    {
                        bGetPulseSetTime=FALSE;
                    }
                    break;

                case EXTERNAL_SEC_ADJ:
                case EXTERNAL_SEC_FROM_OPP_ADJ:
                case EXTERNAL_SEC_FROM_TDC_ADJ:
                    if(ucPulseType_g==GPS_PULSE_TYPE_MIN)
                    {
                        if((tickGet()-GetPulseSetTimeInterval)<14000u)/*140秒*/
                        {
                            bGetPulseSetTime=TRUE;
                            NeedtoSycnfrom3231=TRUE;  /* 需要时钟芯片 */
                        }
                        else
                        {
                            bGetPulseSetTime=FALSE;
                            bGetAbsTime=FALSE;
                            NeedtoSycnfrom3231=TRUE;
                        }
                    }
                    else if(ucPulseType_g==GPS_PULSE_TYPE_SEC)
                    {
                        if((tickGet()-GetPulseSetTimeInterval)<2000u)/*20秒*/
                        {
                            bGetPulseSetTime=TRUE;
                            NeedtoSycnfrom3231=TRUE;
                        }
                        else
                        {
                            bGetPulseSetTime=FALSE;
                            bGetAbsTime=FALSE;
                            NeedtoSycnfrom3231=TRUE;
                        }
                    }
                    else
                    {
                        bGetPulseSetTime=FALSE;
                        bGetAbsTime=FALSE;
                        NeedtoSycnfrom3231=TRUE;
                    }
                    break;
                case EXTERNAL_OPT_B_ADJ:
                case EXTERNAL_B_FROM_OPP_ADJ:
                case EXTERNAL_B_FROM_TDC_ADJ:
                    if((tickGet()-GetPulseSetTimeInterval)<500u)/*5秒*/
                    {
                        bGetPulseSetTime=TRUE;
                        bGetAbsTime=TRUE;
                        SetAdjustTimeSuccessFlag(TRUE);
                    }
                    else
                    {
                        bGetPulseSetTime=FALSE;
                        bGetAbsTime=FALSE;
                        NeedtoSycnfrom3231=TRUE;
                    }
                    break;
                case EXTERNAL_1588_0_ADJ:
                case EXTERNAL_1588_1_ADJ:
                case EXTERNAL_1588_2_ADJ:
                case EXTERNAL_1588_3_ADJ:
                    if((tickGet()-GetAbsTimeInterval)<18000u)/*3分*/
                    {
                        bGetAbsTime=TRUE;
                        SetAdjustTimeSuccessFlag(TRUE);
                    }
                    else
                    {
                        bGetAbsTime=FALSE;
                        NeedtoSycnfrom3231=TRUE; /* 需要读取时钟芯片 */
                    }
                    if((tickGet()-GetPulseSetTimeInterval)<500u)/*5秒*/
                    {
                        bGetPulseSetTime=TRUE;
                    }
                    else
                    {
                        bGetPulseSetTime=FALSE;
                    }
                    break;
                default:
                    bGetPulseSetTime=FALSE;
                    bGetAbsTime=FALSE;
                    NeedtoSycnfrom3231=TRUE;
                    break;
            }

            /* 时钟芯片操作
             * 保证以下函数在其它电路板上操作无其它影响
             */
            {
                /* 应增加写入条件
                 */
                static uint32_t preTime=0;
                if((tickGet()-preTime)>=6000)/*60秒定时器*/
                {
                    EP_DATE_TIME  sysTime ;
                    TM_Get_Sys_Time(&sysTime);
                    SetClock(&sysTime); /*设置时钟芯片*/
                    preTime=tickGet();
                }

                if((tickGet()-preTime)>=6000*60L)/*1小时定时器*/
                {
                    EP_DATE_TIME  sysTime ;
                    if(NeedtoSycnfrom3231&& GetClock(&sysTime)==EP_SUCCESS)
                    {
                        TM_Set_Sys_Time(&sysTime,TRUE);
                        bGetAbsTime=TRUE;
                        SetAdjustTimeSuccessFlag(TRUE);
                    }
                    preTime = tickGet();
                }
            }
        }

#if 0
        /*检查Goose接收状态*/
        {
            static uint32_t preTime=0;
            uint32_t nowTime=0;
            uint32_t TimeElapsed;
            static BOOL bFirst=TRUE;

            if(bFirst)
            {
                bFirst=FALSE;
                preTime=TM_Get_usCnt();
                nowTime=preTime;
                TimeElapsed=0;
            }
            else
            {
                nowTime=TM_Get_usCnt();
                TimeElapsed=nowTime-preTime;
                preTime=nowTime;
                if(TimeElapsed<=60000000)
                    CheckGoRx(TimeElapsed);
            }
        }
#endif

        {
            static uint32_t preTime=0;

            if((tickGet()-preTime)>=200)
            {
                // gmrp_join_timer();
                preTime=tickGet();
            }
        }

        {
            static uint32_t preTime=0;
            static uint32_t RefreshErrTimes=0;

            if(RefreshErrTimes>=ONE_HOUR_CNT)
            {
                Smv_Go_CommStat_Chg();
                RefreshErrTimes=0;
            }

            /*if (appType_g == APP_TYPE_DIG)*/
            {
                /* 为了满足传统采样数字跳闸，不再判断装置类型 */
                if((tickGet()-preTime)>=1000)
                {
                    if(Refresh_Smv_Go_CommStat_File())
                    {
                        RefreshErrTimes=0;
                    }
                    else
                    {
                        RefreshErrTimes++;
                    }
                    preTime=tickGet();
                }
            }
        }

        if (GetFest())
        {
            nowTime = tickGet();
            if ((nowTime-preOffTime) >= 500)
            {
                WriteOfftime(TM_Get_usCnt());

                /* 获取主CPU板光功率 */
                update_sfp_info();

                preOffTime = tickGet();
            }
        }

        /* 解析光功率参数 */
        bSts = GsDeQue(&g_OptWattRcv, &pCur);
        if (bSts == OK)
        {
            while (bSts == OK)
            {
                ParseOptWatt((uint8_t *)pCur->ulData, pCur->ulLen, pCur->ucAddr);
                bSts = GsDeQue(&g_OptWattRcv, &pCur);
            }

            EDP_CheckCcStatus();
        }

        /*闰秒标志的清除
        存在闰秒标志在非预期的时间,则清除闰秒标志*/
        if(g_bLeapSecondFlagHmi)
        {
            retcode=TM_Get_Sys_Time(&dttm);
            if(!(dttm.ucMinute == 0 && dttm.ucSec < (IRIGB_SPECIAL_SEC+2))
                    && (dttm.ucMinute != 59))
            {
                SYN_LOG("慢速循环: CPU将清除闰秒标志 .时-分-秒-毫秒: %d-%d-%d-%d\n",
                        dttm.ucHour,dttm.ucMinute,dttm.ucSec,dttm.unMSEL,0,0);
                SYN_ClearLsFlag();
            }
        }


        taskDelay(INTERVALTICKNUM);
    }
}

/***********************************************************************
* GetTaskName - 得到相应序号的任务的名称
*
* RETURNS: 名称
*
*/
static char *GetTaskName(
    int32_t index
)
{

#ifdef EDP03_BUILD		/* EDP03平台 */
    switch(index)
    {
        case 0:
            return "tNetTask";

        case 1:
            return "tTelnetd";

        case 2:
            return "tUglInput";

        case 3:
            return "tShell";

        case 4:
            return "tWdbTask";

        case 5:
            return "FXTCMP0";

        case 6:
            return "FXTCMP1";

        case 7:
            return "FXTCMP2";

        case 8:
            return "wwm";

        case 9:
            return "tmmi";

        case 10:
            return "tFtpdTask";

        case 11:
            return "tLogTask";

        case 12:
            return "tBspComListen";

        default:
            assert(FALSE);
            break;
    }
#endif

    /* EDP01平台C-A版本和EDP02平台 */
#if defined(EDP_01_02_BUILD)
    switch(index)
    {
        case 0:
            return "tNetTask";

        case 1:
            return "FXTCMP0";

        case 2:
            return "FXTCMP1";

        /*case 3:
        	return "FXTCMP2";*/

        case 3:
            return "tLogTask";

        default:
            assert(FALSE);
            break;
    }
#endif

    return NULL;
}

/***********************************************************************
* GetSysTaskStatus - 获取系统任务状态
*
* RETURNS: 无
*
*/
void GetSysTaskStatus(void)
{
    TTaskWatchDog *ptNode ;
    int TaskID;
    int i;

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
    for (i=0; i<13; i++)
    {
        char *p=NULL;

        p=GetTaskName(i);
        TaskID= taskNameToId(p);
        if(taskIdVerify(TaskID)==ERROR)
        {
            /* 首先判定该任务是否有效 */
            LOG_Dbg_Msg("Ttask %d don't exist, id is %d.\n", i, TaskID, 0, 0, 0, 0);
            continue;
        }
        ptNode = (TTaskWatchDog *)malloc(sizeof(TTaskWatchDog));		/* 申请空间 */
        assert(ptNode);
        ptNode->iTaskID = TaskID;
        ptNode->bTaskBeCreated = TRUE;
        ptNode->bTaskIsGood =TRUE;
        ptNode->bRebootFlag = TRUE;
        sprintf(ptNode->WarningMessage,
                "看门狗复位:因系统任务%s异常或退出,看门狗复位CPU.\n", p);

        sprintf(ptNode->ucTaskName, "%s", p);			/* 任务名称 */
        lstAdd(&m_WatchDogTaskList_g, (NODE *)ptNode);
    }
#endif

#if defined(EDP_01_02_BUILD)
    for (i=0; i<4; i++)
    {
        char *p = NULL;

        p = GetTaskName(i);

        TaskID = taskNameToId(p);
        if (taskIdVerify(TaskID) == ERROR)
        {
            /* 首先判定该任务是否有效 */
            LOG_Dbg_Msg("Task %d don't exist, ID is %d.\n", i, TaskID, 0, 0, 0, 0);
            continue;
        }
        ptNode = (TTaskWatchDog *)malloc(sizeof(TTaskWatchDog));		/* 申请空间 */
        assert (ptNode);
        ptNode->iTaskID = TaskID;
        ptNode->bTaskBeCreated = TRUE;
        ptNode->bTaskIsGood = TRUE;
        ptNode->bRebootFlag = TRUE;
        sprintf(ptNode->WarningMessage,
                "看门狗复位:因系统任务%s异常或退出,看门狗复位CPU.\n", p);
        sprintf(ptNode->ucTaskName, "%s", p);			/* 任务名称 */

        lstAdd(&m_WatchDogTaskList_g, (NODE *)ptNode);
    }
#endif
}

/***********************************************************************
* AddTaskToList - 增加任务到监视队列
*
* RETURNS: 无
*
*/
void AddTaskToList(
    TASK_ID iTaskID, 		/* 添加的任务ID号 */
    BOOL bTaskBeCreated,			/* 任务时候生成标志 */
    char *pMessage,			/* 出错时显示的相关信息 */
    BOOL bRebootFlag /* 异常后是否重启 */
)
{
    TTaskWatchDog *ptNode ;

    if (taskIdVerify(iTaskID) == ERROR)
    {
        /* 首先判定该任务是否有效 */
        LOG_Dbg_Msg("Task %d don't exist.\n", iTaskID, 0, 0, 0, 0, 0);

        return;
    }

    ptNode = (TTaskWatchDog *)malloc(sizeof(TTaskWatchDog));		/* 申请空间 */
    if (ptNode == NULL)
    {
        return;
    }
    /* assert(ptNode); */
    ptNode->iTaskID = iTaskID;
    ptNode->bTaskBeCreated = bTaskBeCreated;
    ptNode->bTaskIsGood = TRUE;
    ptNode->bRebootFlag = bRebootFlag;
    strcpy(ptNode->WarningMessage, pMessage);		/* 拷贝告警信息 */
    sprintf(ptNode->ucTaskName, "%s", taskName(iTaskID));		/* 任务名称 */
    taskLock();
    lstAdd(&m_WatchDogTaskList_g, (NODE *)ptNode);
    taskUnlock();
}


/***********************************************************************
* DeleteSelfTaskFromList - 从监视的文件列表中删除对本任务的监视结点
*
* RETURNS: 无
*
*/
void DeleteSelfTaskFromList()
{
    int i;
    STATUS vxsts;
    TTaskWatchDog *ptNode;
    int iTaskID;
    int iTaskCount;

    // kickSwDog();		/* 打软狗 */
    // kickHwDog();  				/* 打软狗 */

    vxsts = taskLock();
    assert(vxsts == OK);

    iTaskID = taskIdSelf();
    iTaskCount = lstCount(&m_WatchDogTaskList_g);

    for (i=1; i<=iTaskCount; i++)
    {
        ptNode = (TTaskWatchDog *)lstNth(&m_WatchDogTaskList_g, i);
        if ((ptNode != NULL)&&(ptNode->iTaskID == iTaskID))
        {
            lstDelete(&m_WatchDogTaskList_g, (NODE *)ptNode);
            free(ptNode);

            break;
        }
    }

    vxsts = taskUnlock();
    assert (vxsts == OK);
}

/* show the list of monitored task.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void EP_ShowMonitorTaskList(void)
{
    TTaskWatchDog *ptNode;

    LOG_Dbg_Msg("看门狗监视任务: \n", 0, 0, 0, 0, 0, 0);

    for (ptNode=(TTaskWatchDog *)lstFirst(&m_WatchDogTaskList_g); ptNode != NULL;
            ptNode=(TTaskWatchDog *)lstNext((NODE*)ptNode))
    {
        LOG_Dbg_Msg("%s\n", (int)ptNode->ucTaskName, 0, 0, 0, 0, 0);
    }
}

/***********************************************************************
* EP_Set02CPUSts - 设定第2块CPU标志
*
* RETURNS: NONE
*
*/
static void EP_Set02CPUSts(
    uint8_t ucSeq		/* CPU位置序号，0: 第1块CPU；1: 第2块CPU(测控CPU) */
)
{
    ucCPUSeq_g=ucSeq;
}

/***********************************************************************
* Set02CPU - 设定第2块CPU位置.
*
* RETURNS: NONE
*
*/
void EP_Set02CPU(void)
{
    LOG_Dbg_Msg("设置第2块CPU位置.\n", 0, 0, 0, 0, 0, 0);
    EP_Set02CPUSts(1);
}

/************************************************************************
  功能：设置装置设备名称，2008-7-25   张云
  参数：pucDevName: 装置设备名称字符串基址，
        iNameLen，设备名称字符串长度(注意，不包括"\0")。
  返回，无
*/
void  EP_SetDevName(uint8_t  *pucDevName,int   iNameLen)
{
    assert(pucDevName);
    if(iNameLen>=MAX_ID_LEN)
    {
        return  ;
    }
    strncpy(aucDevName_g,pucDevName,iNameLen);
    aucDevName_g[iNameLen]='\0';


    return;
}


/************************************************************************
  功能：获得装置设备名称，2008-7-25   张云
  参数：ppucRtDevNameAddr: 返回装置设备名称字符串基址，
            调用方，将uint8_t  *类型的变量的地址传过来，
            供返回平台内部维护的设备名称全局字符串地址,
            注意，不发生字符串拷贝操作。
        piRtNameLen，返回设备名称字符串长度(注意，不包括"\0")。

*/
EP_STATUS  EP_GetDevName(uint8_t  **ppucRtDevNameAddr,int   *piRtNameLen)
{
    assert(ppucRtDevNameAddr);
    assert(piRtNameLen);

    *ppucRtDevNameAddr=aucDevName_g;
    *piRtNameLen=strlen(aucDevName_g);

    return  EP_SUCCESS;
}

/***********************************************************************
* EP_ChgSysFreq -切换系统频率
*
* RETURNS: 无
*
*/
void EP_ChgSysFreq(
    int32_t iFreqType					/* 系统频率，0: 50Hz；1: 60Hz */
)
{
    SLOW_MESSAGE_NODE Info;

    assert(iFreqType<2);

    if(uiPwrFreq_g == 50)
    {
        /* 设定值与当前值一致 */
        if(iFreqType == 0)
        {
            if(ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "当前已运行在50Hz环境.\n", NULL);
            }
            else if(ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "The equipment have been running under 50Hz system.\n", NULL);
            }
            return;
        }
    }
    else if(uiPwrFreq_g == 60)
    {
        if(iFreqType == 1)
        {
            if(ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "当前已运行在60Hz环境.\n", NULL);
            }
            else if(ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "The equipment have been running under 60Hz system.\n", NULL);
            }
            return;
        }
    }

    Info.type=FREQCHG;
    Info.param1=iFreqType;

    msgQSend(SlowMessage, (char *)&Info, sizeof(SLOW_MESSAGE_NODE), NO_WAIT, MSG_PRI_NORMAL);
}

/***********************************************************************
* EP_SetPwrFreq - Set the system frequency
*
* RETURNS:
*               EP_SUCCESS: Normal
*               EP_ERROR: Error
*
*
*/
EP_STATUS EP_SetPwrFreq(
    int32_t uFreqType			/* 0: 50Hz 1: 60Hz */
)
{
    if(uFreqType == 0)
    {
        /* 50Hz */
        if(FT_Wr_Sys_INI("[SYSTEM]", "PwrFreq", "50")<0)
        {
            if(ENG_MODE == 0)
            {
                LOG_Dbg_Msg("保存设置的系统频率到文件失败.\n", 0, 0, 0, 0, 0, 0);
            }
            else if(ENG_MODE == 1)
            {
                LOG_Dbg_Msg("Saving the system frequency to file is error.\n", 0, 0, 0, 0, 0, 0);
            }

            return EP_ERROR;
        }
    }
    else if(uFreqType == 1)
    {
        /* 60Hz */
        if(FT_Wr_Sys_INI("[SYSTEM]", "PwrFreq", "60")<0)
        {
            if(ENG_MODE == 0)
            {
                LOG_Dbg_Msg("保存设置的系统频率到文件失败.\n", 0, 0, 0, 0, 0, 0);
            }
            else if(ENG_MODE == 1)
            {
                LOG_Dbg_Msg("Saving the system frequency to file is error.\n", 0, 0, 0, 0, 0, 0);
            }

            return EP_ERROR;
        }
    }
    else
    {
        if(ENG_MODE == 0)
        {
            LOG_Dbg_Msg("保存设置的系统频率到文件失败,不支持该系统频率\n", 0, 0, 0, 0, 0, 0);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Dbg_Msg("Saving the system frequency to file is error, Don't support this system frequency.\n",
                        0, 0, 0, 0, 0, 0);
        }

        return EP_ERROR;
    }

    return EP_SUCCESS;
}

/***********************************************************************
* EP_GetPwrFreq - Get the system frequency
*
* RETURNS:
*               50 or 60: Normal
*               0: Error
*
*
*/
u_int EP_GetPwrFreq(void)
{
    uint8_t aucBuf[31];
    int iRst;
    u_int uiFreq;

    iRst=FT_Rd_Sys_INI("[SYSTEM]", "PwrFreq", aucBuf, 30);

    if (iRst == 1)
    {
        uiFreq=atoi(aucBuf);
        if(uiFreq == 50)
        {
            return uiFreq;
        }
        else if(uiFreq == 60)
        {
            return uiFreq;
        }
        else
        {
            uint8_t aucPrompt_g[129];

            if(ENG_MODE == 0)
            {
                sprintf(aucPrompt_g, "本装置不支持频率%d.\n", uiFreq);
            }
            else if(ENG_MODE == 1)
            {
                sprintf(aucPrompt_g, "The equipment don't support the frequency %d.\n", uiFreq);
            }

            LOG_Write(LOG_KERNEL, aucPrompt_g, NULL);

            goto reterr;
        }
    }
    else if(iRst == 0)
    {
        /* 重新生成系统文件 */
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "因[SYSTEM] PwrFreq值读取失败，创建新的系统INI文件.\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL,
                      "Because of failing ro read [SYSTEM] PwrFreq, create new ini file.\n", NULL);
        }

        FT_New_SYS_INI_File();

        goto reterr;
    }
    else if(iRst>1)
    {
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "找到系统设置文件，但系统频率设置项多于1个.\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL,
                      "Header was found, but frequency setting item is more than 1.\n", NULL);
        }

        goto reterr;
    }
    else if(iRst == EP_ERROR)
    {
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "系统文件读取失败.\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Read system file error.\n", NULL);
        }

        goto reterr;
    }


reterr:

    return 0;
}

/***********************************************************************
* EB_GetLanguageType - Get the Language Type
*
* RETURNS: None
*
*
*/
void EB_GetLanguageType(void)
{
    uint8_t aucBuf[31];
    int iRst;
    SLOW_MESSAGE_NODE Info;


    iRst = FT_Rd_Sys_INI("[SYSTEM]", "Language", aucBuf, 30);

    if (iRst == 1)
    {
        ENG_MODE = atoi(aucBuf);

        if (ENG_MODE == 1)
        {
            /* 设置英文状态 */

            /* 单独处理,防止重复生成sci文件 */
            uiEdpStatus_g |= SYS_ENG_MODE;

            if (bdType_g == BOARD_TYPE_E01)
            {
                Info.type = LANGUAGECHG;
                msgQSend(SlowMessage, (char *)&Info, sizeof(SLOW_MESSAGE_NODE),
                         NO_WAIT, MSG_PRI_NORMAL);
            }
        }
        if (ENG_MODE == 0)
        {
            /* LOG_Write(LOG_KERNEL, "本装置使用中文版本\n", NULL);	 */
        }
        else if (ENG_MODE == 1)
        {
            /* LOG_Write(LOG_KERNEL, "This equipment use English edition!\n", NULL); */
        }
        else
        {
            LOG_Write(LOG_KERNEL, "本装置使用非法语言版本，建议重新选择语言种类.This equipment use invalid language edition!Advice to select the language type again.\n", NULL);
        }
    }
    else if (iRst == 0)
    {
        if (ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "因[SYSTEM] Language值读取失败，创建新的系统INI文件.\n", NULL);
        }
        else if (ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Because of failing ro read [SYSTEM] Language, create new ini file.\n", NULL);
        }

        FT_New_SYS_INI_File();
    }
    else if (iRst>1)
    {
        if (ENG_MODE == 0)
            LOG_Write(LOG_KERNEL, "找到系统设置文件，但版本设置项多于1个.\n", NULL);
        else if (ENG_MODE ==1)
            LOG_Write(LOG_KERNEL, "system file was found, but language selecting item is more than 1.\n", NULL);

    }
    else if (iRst == EP_ERROR)
    {
        if (ENG_MODE == 0)
            LOG_Write(LOG_KERNEL, "系统文件读取失败.\n", NULL);
        else if (ENG_MODE ==1)
            LOG_Write(LOG_KERNEL, "Read system file error.\n", NULL);
    }
}

// extern SYMTAB_ID sysSymTbl; /* system symbol table id */

// /*  Function:   run function
//     Parameter:  funName;   function name
//                 pPara, 返回参数.
//     Return Value;   OK; ERROR
// */
// extern int EP_runFun(char *funName, void *pPara)
// {
//     SYM_TYPE *pType = NULL;
//     int status = ERROR;
//     FUNCPTR funTorun = NULL;
//     int *pData = NULL;

//     status = symFindByName(sysSymTbl, funName,
//                            (char **)&funTorun, pType);

//     if (status == ERROR)
//     {
//         LOG_Dbg_Msg("Cann't find functions %s!\n", (int)funName, 0, 0, 0, 0, 0);
//         return ERROR;
//     }

//     if (NULL != funTorun)
//     {
//         LOG_Dbg_Msg("Excute functions %s!\n", (int)funName, 0, 0, 0, 0, 0);

//         pData = (int *)pPara;
//         *pData = funTorun();
//     }

//     return OK;
// }

/* 解析光功率报文.
 * Para:
 *     ptr, 数据指针.
 *     rcvSubLen, 数据长度.
 *     ucAddr, CC板地址.
 * Return:
 *     EP_SUSSESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS ParseOptWatt(uint8_t *srcPtr, int32_t rcvSubLen, uint8_t ucAddr)
{
    uint32_t pdu_lenth;
    uint16_t ethVlanID;
    int32_t asn_len;
    int32_t iLenth;
    int32_t iTmpLenth;
    int32_t i;
    uint8_t *ptr = srcPtr;

    if (srcPtr == NULL)
    {
        return EP_ERROR;
    }

    g_usStormOffSet = 0;
    ptr += 6;  /* Destination MAC */
    ptr += 6;    /* Source MAC */

    /* Priority Tagged, Ethernet Type */
    ethVlanID = ((*ptr)<<8) | (*(ptr+1));
    if (ethVlanID == ETYPE_VLAN_TYPE_ID)
        ptr += 6;
    else
        ptr += 2;

    /* APPID, Length, Reserved */
    ptr += 8;

    /* APDU数据 */
    if (*ptr != 0x61)
    {
        return -1;
    }

    ptr++;

    /* 长度处理, 最多处理4字节 */
    if (((*ptr) == 0x81)
            || ((*ptr) == 0x82)
            || ((*ptr) == 0x83)
            || ((*ptr) == 0x84)
       )
    {
        asn_len = (*ptr)&0x1F;
        ptr++;
        pdu_lenth = 0;

        for (i = 0; i<asn_len; i++)
        {
            pdu_lenth |= (*(ptr+i)) << ((asn_len-i-1) << 3);
        }
        ptr += asn_len;
    }
    else
    {
        pdu_lenth = (*ptr);
        ptr++;
    }

    /* GoCBRef */
    ptr++;
    iLenth = (int)(*ptr);
    ptr++;

    /* 字节拷贝 */
    if (iLenth>(MAX_STRING_LEN-1))
    {
        iTmpLenth = MAX_STRING_LEN-1;
    }
    else
    {
        iTmpLenth = iLenth;
    }

    ptr += iLenth;

    /* 允许生存时间 */
    ptr++;
    iLenth = (int)(*ptr);
    ptr++;

    ptr += iLenth;

    /* DataSet名字 */
    ptr++;
    iLenth = (int)(*ptr);
    ptr++;

    /* 字节拷贝 */
    if (iLenth>(MAX_STRING_LEN-1))
    {
        iTmpLenth = MAX_STRING_LEN-1;
    }
    else
    {
        iTmpLenth = iLenth;
    }

    ptr += iLenth;

    /* Go字符串 */
    ptr++;
    iLenth = (int)(*ptr);
    ptr++;

    /* 字节拷贝 */
    if (iLenth>(MAX_STRING_LEN-1))
    {
        iTmpLenth = MAX_STRING_LEN-1;
    }
    else
    {
        iTmpLenth = iLenth;
    }

    ptr += iLenth;

    /* StNum加1时间 */
    ptr++;
    iLenth = (int)(*ptr);
    ptr++;

    ptr += iLenth;

    /* StNum */
    ptr++;
    iLenth = (int)(*ptr);
    ptr++;
    ptr += iLenth;

    /* SqNum */
    ptr++;
    iLenth = (int)(*ptr);
    ptr++;
    ptr += iLenth;

    /* 测试标识 */
    ptr++;
    iLenth = (int)(*ptr);
    ptr++;
    ptr += iLenth;

    /* 配置版本号 */
    ptr++;
    iLenth = (int)(*ptr);
    ptr++;
    ptr += iLenth;

    /* 未配置好标识 */
    ptr++;
    iLenth = (int)(*ptr);
    ptr++;
    ptr += iLenth;

    /* 编码单元个数 */
    ptr++;
    iLenth = (int)(*ptr);
    ptr++;

    ptr += iLenth;

    /* 应用数据标识 */
    if ((*ptr) != 0xab)
    {
        LOG_Dbg_Msg("报文错误\n", 1, 2, 3, 4, 5, 6);
        return EP_ERROR;
    }
    ptr++;

    /* 长度处理, 最多处理4字节 */
    if (((*ptr) == 0x81)
            || ((*ptr) == 0x82)
            || ((*ptr) == 0x83)
            || ((*ptr) == 0x84)
       )
    {
        asn_len = (*ptr)&0x1F;
        ptr++;
        pdu_lenth = 0;

        for (i = 0; i<asn_len; i++)
        {
            pdu_lenth |= (*(ptr+i)) << ((asn_len-i-1) << 3);
        }
        ptr += asn_len;
    }
    else
    {
        pdu_lenth = (*ptr);
        ptr++;
    }

    /* 地址限制 */
    if (ucAddr >= MAX_CC_BOARD_ID_NUM)
    {
        ucAddr = MAX_CC_BOARD_ID_NUM-1;
    }

    /* 查询数组 */
    for (i = 0; i<g_ucCCNum; i++)
    {
        if (arrCcPortSts[i].ucCCSn == ucAddr)
        {
            break;
        }
    }

    if (i >= g_ucCCNum)
    {
        arrCcPortSts[g_ucCCNum].ucCCSn = ucAddr;
        g_ucBoardIdtoCcStsNo[ucAddr-1] = i;
        g_bWriteLogFlag[g_ucCCNum] = FALSE;
        g_ucCCSnToArr[ucAddr] = g_ucCCNum;
        g_ucCCNum++;
    }

    arrCcPortSts[g_ucCCSnToArr[ucAddr]].ulOptRptCnt++;
    g_usStormOffSet += ptr - srcPtr;
    ParseOptApp(ptr, pdu_lenth, &arrCcPortSts[g_ucCCSnToArr[ucAddr]]);

    if(g_bWriteLogFlag[i] == FALSE)
    {
        uint8_t aucVer[TEMP_INFO_MAX_LEN] = "";
        sprintf(aucVer, "%x-%x-%x",
                (int)arrCcPortSts[i].ucProtocolVer, (int)arrCcPortSts[i].usFPGASwVer,
                (int)arrCcPortSts[i].usNiosSwVer);
        LOG_ExtraItemWrite(arrCcPortSts[i].ucCCDesc, aucVer);
        g_bWriteLogFlag[i] = TRUE;
    }

    return EP_SUCCESS;
}

/* 解析光功率应用报文.
 * Para:
 *     ptr, 数据指针.
 *     rcvSubLen, 数据长度.
 *     pCcSts, CC板状态.
 * Return:
 *     EP_SUSSESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS ParseOptApp(uint8_t *srcPtr, int32_t rcvSubLen, T_CC_STS *pCcSts)
{
    uint8_t *ptr = srcPtr;
    T_CC_STS *pTmpCcSts = pCcSts;
    int32_t i;
    uint8_t strInfo[TEMP_INFO_MAX_LEN];

    if ((srcPtr == NULL) || (pCcSts == NULL))
    {
        return EP_ERROR;
    }

    /* CC描述 */
    memcpy(pTmpCcSts->ucCCDesc, ptr, INFO_MAX_LEN);
    pTmpCcSts->ucCCDesc[INFO_MAX_LEN] = '\0';
    pTmpCcSts->ucCCDesc[strlen(pTmpCcSts->ucCCDesc)] = '\0';
    ptr += INFO_MAX_LEN;

    /* 如果没有配置, 则设置缺省名称 */
    if (strlen(pTmpCcSts->ucCCDesc) == 0)
    {
        sprintf(pTmpCcSts->ucCCDesc, "CC%d", pTmpCcSts->ucCCSn);
    }

    /* 规约版本号 */
    pTmpCcSts->ucProtocolVer = *ptr++;

    if (!((pTmpCcSts->ucProtocolVer == CC_SFP_PTL_10_VER)
            || (pTmpCcSts->ucProtocolVer == CC_SFP_PTL_11_VER)))
    {
        return EP_ERROR;
    }

    /* 保留字节 */
    ptr++;

    pTmpCcSts->usFPGASwVer = U8_TO_U16(*ptr, *(ptr+1));
    ptr += 2;

    pTmpCcSts->usNiosSwVer = U8_TO_U16(*ptr, *(ptr+1));
    ptr += 2;

    pTmpCcSts->usSvCfgVer = U8_TO_U16(*ptr, *(ptr+1));
    ptr += 2;

    pTmpCcSts->usSvCfgCrc = U8_TO_U16(*ptr, *(ptr+1));
    ptr += 2;

    pTmpCcSts->usSvBayNum = U8_TO_U16(*ptr, *(ptr+1));
    ptr += 2;

    pTmpCcSts->usGsCfgVer = U8_TO_U16(*ptr, *(ptr+1));
    ptr += 2;

    pTmpCcSts->usGsCfgCrc = U8_TO_U16(*ptr, *(ptr+1));
    ptr += 2;

    pTmpCcSts->usGsBayNum = U8_TO_U16(*ptr, *(ptr+1));
    ptr += 2;

    pTmpCcSts->ucPortNum = *ptr++;
    if (pTmpCcSts->ucPortNum>MAX_CC_PORT_NUM)
    {
        pTmpCcSts->ucPortNum = MAX_CC_PORT_NUM;
    }

    /* 报文序列, 以及其它信息 */
    pTmpCcSts->ucOtherInfo = *ptr;
    g_usStormOffSet += ptr - srcPtr;
    pTmpCcSts->bNetStormSts = pTmpCcSts->ucOtherInfo & MASK_NET_STORM;
    ptr++;

    if (pTmpCcSts->bNetStormSts
            && (!pTmpCcSts->bLstNetStormSts)
            && (pTmpCcSts->ulNetStormCnt < MAX_NET_STORM_CNT))
    {
        sprintf(strInfo, "%s处于网络收发风暴状态 %ld!\n", pTmpCcSts->ucCCDesc,TM_High_Get_usCnt());
        LOG_Write(LOG_KERNEL, (const uint8_t *)strInfo, NULL);
        pTmpCcSts->ulNetStormCnt++;
    }
    else if ((!pTmpCcSts->bNetStormSts)
             && pTmpCcSts->bLstNetStormSts
             && (pTmpCcSts->ulNetStormCnt < MAX_NET_STORM_CNT))
    {
        sprintf(strInfo, "%s网络收发风暴消除! %ld \n", pTmpCcSts->ucCCDesc,TM_High_Get_usCnt());
        LOG_Write(LOG_KERNEL, (const uint8_t *)strInfo, NULL);
    }

    pTmpCcSts->bLstNetStormSts = pTmpCcSts->bNetStormSts;

    for (i = 0; i<pTmpCcSts->ucPortNum; i++)
    {
        pTmpCcSts->tOptPortSts[i].StsInfo.usStsInfo = U8_TO_U16(*ptr, *(ptr+1));
        ptr += 2;

        /* 为供应商信息预留 */
        ptr += 4;

        pTmpCcSts->tOptPortSts[i].usTemp = U8_TO_U16(*ptr, *(ptr+1));
        ptr += 2;

        pTmpCcSts->tOptPortSts[i].usVolt = U8_TO_U16(*ptr, *(ptr+1));
        ptr += 2;

        pTmpCcSts->tOptPortSts[i].usCurrent = U8_TO_U16(*ptr, *(ptr+1));
        ptr += 2;

        pTmpCcSts->tOptPortSts[i].usSndWatt = U8_TO_U16(*ptr, *(ptr+1));
        ptr += 2;

        pTmpCcSts->tOptPortSts[i].usRcvWatt = U8_TO_U16(*ptr, *(ptr+1));
        ptr += 2;

        pTmpCcSts->tOptPortSts[i].WarmInfo.usWarmInfo = U8_TO_U16(*ptr, *(ptr+1));
        ptr += 2;

        pTmpCcSts->tOptPortSts[i].AlarmInfo.usAlarmInfo = U8_TO_U16(*ptr, *(ptr+1));
        ptr += 2;

        if (pTmpCcSts->ucProtocolVer == CC_SFP_PTL_11_VER)
        {
            pTmpCcSts->tOptPortSts[i].ulSvFlowCnt = U8_TO_U32(*ptr, *(ptr+1), *(ptr+2), *(ptr+3));
            ptr += 4;

            pTmpCcSts->tOptPortSts[i].ulSvIntervalErrCnt = U8_TO_U32(*ptr, *(ptr+1), *(ptr+2), *(ptr+3));
            ptr += 4;
        }
    }

    return EP_SUCCESS;
}

/* 初始化主板光口信息.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUSSESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS EP_InitCPUWatt(void)
{
    arrCcPortSts[g_ucCCNum].ucCCSn = 0;

    if (appType_g == APP_TYPE_DIG)
    {
        /* 区分EDP01/EDP02 */
        if (bdType_g == BOARD_TYPE_E01)
        {
            arrCcPortSts[g_ucCCNum].ucPortNum = 2;  /* 最多2个端口 */
        }
        else
        {
            arrCcPortSts[g_ucCCNum].ucPortNum = 4;  /* 最多4个端口 */
        }
    }
    else if (uiAppType_g == APP_LINE)
    {
        arrCcPortSts[g_ucCCNum].ucPortNum = 4;  /* 传统应用无 */
    }
    else
    {
        if(FT_Is_File(CONFIG_FILE))
        {
            arrCcPortSts[g_ucCCNum].ucPortNum = 2;  /* 有GSE配置文件则认为有2个光口 */
        }
        else
        {
            arrCcPortSts[g_ucCCNum].ucPortNum = 0;  /* 传统应用无 */
        }
    }

    memset(g_ucCCSnToArr, 0, MAX_CC_BOARD_ID_NUM);
    g_ucCCSnToArr[0] = g_ucCCNum;

    if (uiAppType_g == APP_PROT_MEA_MERGE)
    {
        /* 保护测控一体化 */
        /* 为从CPU或第2块CPU */
        if (isNumber_2_04CPU() || (ucCPUSeq_g == 1))
        {
            if (ENG_MODE == 0)
            {
                sprintf(arrCcPortSts[g_ucCCNum].ucCCDesc, "测控CPU板");
            }
            else
            {
                sprintf(arrCcPortSts[g_ucCCNum].ucCCDesc, "Mea. Device CPU Board");
            }
        }
        else
        {
            if (ENG_MODE == 0)
            {
                sprintf(arrCcPortSts[g_ucCCNum].ucCCDesc, "保护CPU板");
            }
            else
            {
                sprintf(arrCcPortSts[g_ucCCNum].ucCCDesc, "Prot. Device CPU Board");
            }
        }
    }
    else
    {
        /* 为从CPU或第2块CPU */
        if (isNumber_2_04CPU() || (ucCPUSeq_g == 1))
        {
            if (ENG_MODE == 0)
            {
                sprintf(arrCcPortSts[g_ucCCNum].ucCCDesc, "从CPU板");
            }
            else
            {
                sprintf(arrCcPortSts[g_ucCCNum].ucCCDesc, "Slave CPU Board");
            }
        }
        else
        {
            if (ENG_MODE == 0)
            {
                sprintf(arrCcPortSts[g_ucCCNum].ucCCDesc, "主CPU板");
            }
            else
            {
                sprintf(arrCcPortSts[g_ucCCNum].ucCCDesc, "Master CPU Board");
            }
        }
    }

    g_ucCCNum++;

    return EP_SUCCESS;
}


/* 更新主CPU光功率
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
int	update_sfp_info(void)
{
    //todo
    int ret_val = 0;
    // unsigned char buf[16];
    // signed char temp_val;
    // unsigned short vcc_ad_val;
    // unsigned short tx_pwr_ad_val;
    // unsigned short rx_pwr_ad_val;
    // int i;

    // arrCcPortSts[g_ucCCSnToArr[0]].ulOptRptCnt++;

    // /* 访问所有光口 */
    // for (i = 0; i<CPU_SFP_NUM; i++)
    // {
    //     ret_val = set_i2c_mux_val(i);

    //     if(0 == ret_val)
    //     {
    //         ret_val = get_sfp_status_val(SFP_TEMP_ADRS, 10, buf);
    //         if(0 == ret_val)
    //         {
    //             temp_val = (signed char)buf[0];
    //             vcc_ad_val = (buf[SFP_VCC_TO_TMP_OFFSET]<<8)
    //                          | buf[SFP_VCC_TO_TMP_OFFSET+1];
    //             tx_pwr_ad_val = (buf[SFP_TX_P_TO_TMP_OFFSET]<<8)
    //                             | buf[SFP_TX_P_TO_TMP_OFFSET+1];
    //             rx_pwr_ad_val = (buf[SFP_RX_P_TO_TMP_OFFSET]<<8)
    //                             | buf[SFP_RX_P_TO_TMP_OFFSET+1];

    //             arrCcPortSts[0].tOptPortSts[i].usTemp = temp_val;
    //             arrCcPortSts[0].tOptPortSts[i].usVolt = vcc_ad_val;
    //             arrCcPortSts[0].tOptPortSts[i].usSndWatt = tx_pwr_ad_val;
    //             arrCcPortSts[0].tOptPortSts[i].usRcvWatt = rx_pwr_ad_val;
    //         }

    //         ret_val = get_sfp_status_val(SFP_ALARM_AND_WARM_ADRS, 6, buf);
    //         if (0 == ret_val)
    //         {
    //             arrCcPortSts[0].tOptPortSts[i].WarmInfo.usWarmInfo = (buf[0] << 8)
    //                     | buf[1];
    //             arrCcPortSts[0].tOptPortSts[i].AlarmInfo.usAlarmInfo = (buf[4] << 8)
    //                     | buf[5];
    //         }
    //     }

    //     if (0 != ret_val)
    //     {
    //         arrCcPortSts[0].tOptPortSts[i].usTemp = 0;
    //         arrCcPortSts[0].tOptPortSts[i].usVolt = 0;
    //         arrCcPortSts[0].tOptPortSts[i].usSndWatt = 0;
    //         arrCcPortSts[0].tOptPortSts[i].usRcvWatt = 0;
    //     }
    // }

    return ret_val;
}

/* 显示光功率信息.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
int	show_all_sfp_info(void)
{
    int32_t i;
    int32_t j;
    uint16_t usSndWatt;  /* 光收发器发送光功率 */
    uint16_t usRcvWatt;    /* 光收发器接收光功率 */

    printf("光功率相关信息显示, 模件数:%d\n", g_ucCCNum);

    for (i = 0; i<g_ucCCNum; i++)
    {
        printf("序号:%d, 名称:%s, 端口数:%d, 协议版本号:0x%x, FPGA程序版本号:0x%x, Nios程序版本号:0x%x, 光功率报文数:%d\n",
               arrCcPortSts[i].ucCCSn,
               arrCcPortSts[i].ucCCDesc,
               arrCcPortSts[i].ucPortNum,
               arrCcPortSts[i].ucProtocolVer,
               arrCcPortSts[i].usFPGASwVer,
               arrCcPortSts[i].usNiosSwVer,
               (int)arrCcPortSts[i].ulOptRptCnt);

        printf("SV配置版本号:0x%x, SV配置CRC:0x%x, SV支持最多间隔数:%d, GS配置版本号:0x%x, GS配置CRC:0x%x, GS支持最多间隔数:%d, 其它信息:%x\n",
               arrCcPortSts[i].usSvCfgVer,
               arrCcPortSts[i].usSvCfgCrc,
               arrCcPortSts[i].usSvBayNum,
               arrCcPortSts[i].usGsCfgVer,
               arrCcPortSts[i].usGsCfgCrc,
               arrCcPortSts[i].usGsBayNum,
               arrCcPortSts[i].bLstNetStormSts);

        for (j = 0; j<arrCcPortSts[i].ucPortNum; j++)
        {
            usSndWatt = arrCcPortSts[i].tOptPortSts[j].usSndWatt;
            usRcvWatt = arrCcPortSts[i].tOptPortSts[j].usRcvWatt;

            if (usSndWatt == 0)
            {
                usSndWatt = 1;
            }

            if (usRcvWatt == 0)
            {
                usRcvWatt = 1;
            }

            printf("温度:%d, 供电电平:%d, 发送电流:%d, 发送光功率:%fdBm, 接收光功率:%fdBm\n",
                   arrCcPortSts[i].tOptPortSts[j].usTemp/TEMP_SCALE,
                   arrCcPortSts[i].tOptPortSts[j].usVolt,
                   arrCcPortSts[i].tOptPortSts[j].usCurrent,
                   -40.0+10.0*log10(1.0*usSndWatt),
                   -40.0+10.0*log10(1.0*usRcvWatt));

            printf("收发状态:0x%x, 预警状态:0x%x, 告警状态:0x%x\n",
                   arrCcPortSts[i].tOptPortSts[j].StsInfo.usStsInfo,
                   arrCcPortSts[i].tOptPortSts[j].WarmInfo.usWarmInfo,
                   arrCcPortSts[i].tOptPortSts[j].AlarmInfo.usAlarmInfo);
        }
    }

    return EP_SUCCESS;
}

/* 显示处理帧数 */
void DisplayFrmNum(void)
{
    GSE_SUB_INFO *p = GetSubInfoRootNode();
    uint32_t ulCurFrmNum = 0;
    static uint32_t ulLstFrmNum = 0;
    uint32_t ulCurTick = 0;
    static uint32_t ulLstTick = 0;
    uint8_t arrInfo[TEMP_INFO_MAX_LEN];
    static uint32_t ulLstProcFrmNum = 0;

    if (!p)
    {
        logMsg("Sub Pool is Empty!\n", 0, 0, 0, 0, 0, 0);
    }

    taskLock();
    ulCurTick = tickGet();
    ulCurFrmNum = 0;
    while (p)
    {
        ulCurFrmNum += p->ulRcvRrmCnt;
        logMsg("间隔%x %d\n", p->UserInfo.appID, p->ulRcvRrmCnt, 0, 0, 0, 0);
        p = p->next;
    }

    sprintf(arrInfo, "网络流量%u/%u, %u帧/s, %u\n",
            (unsigned int)(ulCurFrmNum-ulLstFrmNum),
            (unsigned int)(ulCurTick-ulLstTick),
            (unsigned int)(((ulCurFrmNum-ulLstFrmNum)*100)/(ulCurTick-ulLstTick)),
            (unsigned int)(ulLstProcFrmNum-g_ulGsFrmCnt));

    LOG_Write(LOG_KERNEL, arrInfo, NULL);
    logMsg("网络流量%u/%u, %u帧/s, %u\n",
           (unsigned int)(ulCurFrmNum-ulLstFrmNum),
           (unsigned int)(ulCurTick-ulLstTick),
           (unsigned int)(((ulCurFrmNum-ulLstFrmNum)*100)/(ulCurTick-ulLstTick)),
           (unsigned int)(g_ulGsFrmCnt-ulLstProcFrmNum), 0, 0);

    ulLstFrmNum = ulCurFrmNum;
    ulLstTick = ulCurTick;
    ulLstProcFrmNum = g_ulGsFrmCnt;
    taskUnlock();
}
/*得到网络风暴状态*/
BOOL EP_GetStormState()
{
    int i;
    for(i = 0; i < g_ucCCNum; i++)
    {
        if( g_bStormState[arrCcPortSts[i].ucCCSn] )
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* 获取CPU1风暴抑制状态.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL EP_GetCPU1StormState(void)
{
    return g_bCPU1StormState;
}

/* 设置CPU2风暴抑制状态.
 * Para:
 *     bSts, TRUE or FALSE.
 * Return:
 *     NONE.
 */
void EP_SetCPU2StormState(BOOL bSts)
{
    g_bCPU2StormState = bSts;
}