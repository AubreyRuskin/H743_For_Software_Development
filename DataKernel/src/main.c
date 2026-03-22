/* edpbase.c - This file contains system initilization procedures */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02a, 29jul06, dy add the interface to BSP and the rebooting function.
01a, 27sep02, hdx first created.
*/

/*
DESCRIPTION
This file contains system initilization procedures.
INCLUDES: edpbase.h
*/

/* includes */

#include "edpbase.h"

#include "realdata.h"
#include "logmsg.h"
#include "errtest.h"
#include "hwcfg.h"
#include "swcfg.h"
#include "dspai.h"
#include "filetool.h"
#include "RE_RelayEngine.h"
#include "sysinfo.h"
#include "view.h"
#include "rec.h"
#include "measure.h"
#include "auto_upload.h"
#include "man_fram.h"
#include "VoltageWatch.h"
#include "Smv_Go_CommStat_File.h"
#include <intLib.h>
#include <tickLib.h>
#include "protectmmiinterface.h"
#include "bspinterface.h"
// #include "EDP_UnifiedCfgInterface.h"

#include "HardClock.h"
#include <stdio_compat.h>
//#include <taskLib.h>
//#include <logLib.h>
//#include <intLib.h>
//#include <rebootLib.h>
//#include <drv/mem/m8260Siu.h>
//#include "UsbMmiInterface.h"
#include "configerrordisp.h"
//#include "protectmmiinterface.h"
#include "detailoperate_log.h"
//#include "tickLib.h"

//#include <linux/time.h>
//#include <linux/jiffies.h>

/* The following is used for exception processing. */
//#include "m8260IntrCtl.h"
//#include "excLib.h"
//#include "arch\ppc\esfPpc.h"
//#include "config04.h"

#include "POLE_VtBox.h"	 /* Use for same pole equipments. */
// #include "HDL_VtBox.h"		/* Intellegent operation box. */
// //#include "GooseInterface.h"
#include "EdpNetCfg.h"	   /* Net configuration. */
#include "EdpVer.h"    /* Version control. */
#include "scc_hdlc_raw.h"
#include "FileSynPro.h"		/* 文件列表同�?? */
#include "VTBOX_Interface.h"
#include "VTBOX_SamInterface.h"
#include "OPT_VtBox.h"
#include "VTBOX_Box.h"
// #include "bspinterface.h"
#include "edp_asst.h"
// #include "kernelLib.h"
#include "excErrHandle.h"

#include "FileCRC.h"
#include "Festofftime.h"
// #include "wdLib.h"
// #include "hal.h"
#include "smvcfg.h"
#include "smv_rx.h"
#include "HDL_VtBox.h"

/* defines */

/* initialization error */

#define INIT_TM_ERR     0x0001
#define INIT_FT_ERR     0x0002
#define INIT_AU_ERR     0x0004
#define INIT_RD_ERR     0x0008
#define INIT_FUNC_ERR   0x0010
#define INIT_SC_ERR     0x0020
#define INIT_SI_ERR     0x0040
#define INIT_SET_ERR    0x0080
#define INIT_VI_ERR     0x0100
#define INIT_DSP_ERR    0x0200
#define INIT_RC_ERR     0x0400
#define INIT_LGC_ERR    0x0800
#define INIT_FRAM_ERR   0x1000
#define INIT_LOG_ERR    0x2000
#define INIT_VX_ERR     0x4000

#ifdef EXCITE_BUILD
#define INIT_FPGA_ERR 0x8000
#define INIT_MEASURE_ERR 0x10000
#define INIT_AO_ERR 0x20000
#define INIT_REDUN_ERR 0x40000
#define INIT_OPTCOMM_ERR 0x80000
#endif

#define DEV_ABNORMAL_HINT_LAMP_ID "装置异常�??"

#ifndef EDP01_305A_C
#define EDP01_305A_C
#endif

/* typedefs */



/* globals */

T_WATT_QUEUE g_OptWattRcv;

extern uint32_t ulDspAccessCounter_g;   /* Counter for DSP task */
/* Monitor counter for sampling task based on photoelectron current transition. */
extern uint32_t ulExtAccessCounter_g;
extern BOOL bDspTaskStartFlag_g;
/* If the sampling task based on photoelectron current transition have been created. */
extern BOOL bExtRecvTaskStartFlag_g;
extern BOOL bRecBufTaskStartFlag_g;
extern BOOL bRecFileTaskStartFlag_g;
extern BOOL bLogWriteTaskStartFlag_g;
extern BOOL bEvtMakeRtpTaskStartFlag_g;
extern int consoleFd;
extern EP_DATE_TIME LOG_dtLastWdRebootTime_g;
extern LIST m_WatchDogTaskList_g;

int iIMMR_g;
u_int uiEdpStatus_g;

pFV pMessageBox = NULL;
pPARACHECKFV pParaCheckFun = NULL; /* 参数校验回调函数 */
char MessageStr_g[MESSAGE_MAX_LEN];
BOOL bEnableAlarm_g = FALSE;	/* Alarm setting flag for 04 board alarm. */

int ENG_MODE = 0;		/* Used for edition control. */

char *pramLowMemAdrs;		/* RAM最低地址 */
u_int uiInitErrFlag_g;

BOOL NET_PRINTER = FALSE; 	/* TRUE: printer using Ethernet; FLSE: printer using UART */
int32_t	m_lPrnSerials;	/* terminal descriptor. */
uint32_t PrnBufPos;
char *PrnBuf;
char *pPrnBuf;
uint8_t ComVer_g;	/* Type of the communication auxiliary board. */
uint8_t ComVerExt_g;	/* 扩展通�??板类�?? */

MSG_Q_ID SlowMessage;
BOOL bSciChangedFlag_g = FALSE;
EP_DATE_TIME EP_dtRebootTime_g = {0, 0, 0, 0, 0, 1, 1, 1, 2000}; /* Time of reboot. */
BASE_IDLE_STAT idleStat;	/* 空闲统�?? */

APP_TYPE appType_g = APP_TYPE_TRAD;   /* 应用类型 */
BOARD_TYPE bdType_g = BOARD_TYPE_E01;   /* �??件类�?? */
uint8_t ucCpuSpiRol_g = 0;		/* CPU在SPI通信�??主从标志, 0: 标志无效; 1: 主CPU�??2: 从CPU */
uint32_t sysInputFreq_g;   /* 系统总线频率 */
int iBootReason_g;			/* Boot resean */
uint32_t g_ulStartTm;  /* 标�?�开始时�?? */
uint32_t g_ulEndTm; /* 标�?�结束时�?? */
uint32_t g_ulComsumeTm;  /* 消耗时�?? */

/* �??描任务驱动函数注�?? */
RD_REG_FUNC_AI aregf_g[MAX_AI_FUNC_NUM+1];
BOOL bDspDrvMod = FALSE; /* 数据驱动模式, FALSE: 无DSP任务模式; TRUE: DSP任务模式 */

/* locals */

static BOOL bReSetFlag_g;
static EP_DATE_TIME EP_dtNewWdRebootTime_g = {0, 0, 0, 0, 0, 1, 1, 1, 2000};
static BOOL bInitEndFlag_g = FALSE;
static EP_DATE_TIME ulNetInitBeginUs_g = {0, 0, 0, 0, 0, 1, 1, 1, 2000};	/* 开始网络初始化时间 */
static int InitTaskID_g = -1;
static int MonitorInitTaskID_g = -1;
/* Used for saving the us counter when calling the externMain. */
static uint32_t SysUsBeginCount_g;
static void *pvDevAbnormalHintLampHdl_g = NULL;

static BOOL bHasChgRepairSts_g = FALSE;	/* If the system examination flag have been set. */
static u_int uiIOFuncPinSts_g;
// LOCAL WDOG_ID idleStatWd = NULL;			/* idle statistic watchdog ID */
int idleInterval = 0;  /* 统�?�间�?? */
int idleIntervaOrin = 0;  /* 原�?�统计间�?? */

/* global functions */

/* 看门狗�?�理相关代码 */
extern BOOL GetRecBufTaskStatus();			/* 录波缓冲任务运�?��?�测函�? */
extern BOOL GetRecFileTaskStatus();					/* 录波文件任务运�?��?�测函�? */
extern BOOL GetLogTaskStatus();						/* 日志记录任务运�?��?�测函�? */
extern BOOL GetEvtMakeRptTaskStatus();					/* 事件报告任务运�?��?�测函�? */
extern BOOL RE_Get_Relay_Task_Run_State(uint8_t *pnRelayTaskNo, uint8_t *pnTskSts);
BOOL Get61850FuncStatus(void);			/* 61850功能监�?? */

/***********************************************************************
* init - 初�?�化通�??口相关变�??
*
* RETURNS: �??
*
*/
extern void init(void);

/***********************************************************************
* Exc_SysregExcHandle - 平台异常处理函数挂接
*
* RETURNS: �??
*
*/
extern void Exc_SysregExcHandle();


/* SMV采样初�?�化.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, FALSE.
 */
extern BOOL GetSampDataInit (void);

/***********************************************************************
* SetAdjustTimeSuccessFlag - 设置系统对时标志
*
* RETURNS: �??
*
*/
extern void SetAdjustTimeSuccessFlag(
    BOOL bOkFlag
);

/***********************************************************************
* GetAdjustTimeSuccessFlag - 获取对时�??否成功标�??
*
* RETURNS: TRUE, or FALSE
*
*/
extern BOOL GetAdjustTimeSuccessFlag();

/* 保护任务消耗时间统�??
 * Para:
 *     ulIntMs, ms统�?�间�??.
 * Return:
 *     NONE.
 */
extern void  RE_StatTaskComsumeTimeStat(uint32_t ulIntMs);

/* 显示保护任务消耗时间统�??
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void RE_ShowTaskComsumeTimeStat(void);

/* ˢ��Ӧ�ò�������?����̬����ʱʹ�ã�
   ��Ҫ�Ǹ���Ӧ�õ��㷨��
   ��������
   ���أ�EP_SUCCESS  �ɹ�
         ������FALSE */
EP_STATUS   EP_App_Refresh();

/* static functions */

static int Init_Task(int arg1, int arg2, int arg3, int arg4, int arg5,
                     int arg6, int arg7, int arg8, int arg9, int arg10);
static EP_STATUS EP_Hw_Watch_Dog_Init(void);			/* 看门狗初始化函数 */
static void EP_Hw_Watch_Dog_Handle(void);						/* 看门狗�?�理函数 */
static void SetAbnormalHintLampHdl();		/* 获得装置异常指示�?? */
static BOOL GetMonitorTaskStatus();			/* 获得任务tMonitor的状�?? */

/***********************************************************************
* Monitor - 监�?�任务入口函�??
*
* RETURNS: �??
*
*/
static int Monitor(
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

/* 空闲统�??.
 * Para:
 *     arg.
 * Return:
 *     OK, or ERROR.
 */
static int IdleStatEntry(
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
* MonitorInit - 监�?�任务初始化
*
* RETURNS: �??
*
*/
static void MonitorInit();

/***********************************************************************
* GetTaskStatus - 获得相应任务的状�??,若�?�常,则返回真,否则,返回�??
*
* RETURNS: TRUE, or FALSE
*
*/
static BOOL GetTaskStatus(
    int TaskID,
    char *pTaskStatus
);

/***********************************************************************
* EP_LogBootReason - Record the boot resean into the LOG, must be called after the LOG initialization.
*
* RETURNS: None
*
*/
static EP_STATUS EP_LogBootReason(void);

/***********************************************************************
* MemWr - 内存�??
*
* RETURNS: �??
*
* 注意: 调用此函数�?�小�??, 防�?�破坏内�??
*
*/
static void MemWr(
    uint32_t MemAddr, /* 写入地址 */
    uint32_t WrData				/* 写入数据 */
);

/* 空闲统�?�函�??
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
static void idleStatStart(void);

/* functions */

/* 外部�??件初始化.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void externHwInit()
{
}

/* 0为成功，1为失�??*/
UINT16 Set_PSMR_PRO_mode(void) 
{ 
#define PSMR_PRO 0xf0011324  /*另一�??以太网则�?? 0xf0011A68 */

    UINT16 mode; 
    *(UINT16 *)PSMR_PRO|=0x0040; 
    mode=*((UINT16 *) PSMR_PRO); 
    if (mode==0x1470) 
    { 
        return 0; 
    } 
    else 
    { 
        return 1; 
    } 
}

/* Initialize the Net.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void EDP_Init_Net(void)
{
    // externBSP(); 		/* 加载符号�?? */
    init(); 		/* 通�??初�?�化 */
    // SecondECardConfig();  		/* 初�?�化�??2个FCC */

    // if (EP_IS_FAST_BOOT())
    // {
    //     /* 调试态初始化 */
    //     Init_Wdb();						/* 初�?�化wdb */
    //     Init_Telnet();		/* 初�?�化telnet */
    // }
    // Set_PSMR_PRO_mode();
}

/***********************************************************************
* externMain - 外部调用主函�??
*
* RETURNS: �??
*
*/
/* void externMain() */
int externMain()
{
    uint64_t ullTimebasePeriod_ns;
    uint32_t ulBaseH, ulBaseL;
    uint64_t ullValue1;
    uint32_t ulResult1;
    uint32_t ulResult2;
    static BOOL bInitFlag = FALSE;

    if (bInitFlag)
    {
        return 0;
    }
    bInitFlag = TRUE;

    printf("long size: %zu bytes\n", sizeof(long));

    EP_App_Refresh();
    
    /* 获取系统总线频率
     */
    //sysInputFreq_g = sysInputFreqGet();

    /* 获取�??件平台版�??及类�??
     */
    //VER_InitVerFunc();

    #if 0
    /* 最多网口数�??, 和硬件版�??有关 */
    if (VER_GetHwBoardSN() == E02_CPU_F_BORAD)
    {
        iHdlNetNum_g = MAX_GSE_NET_CNT;
    }
    else
    {
        iHdlNetNum_g = MAX_GENERAL_PORT_NUM;
    }

    /* 获取FAST_BOOT及BOOT_SEL跳线信号 */
    if (!IS_Quick_Boot_Mode())
        uiIOFuncPinSts_g |= IO_PIN_FAST_BOOT;

    if (IS_Boot_From_Net())
        uiIOFuncPinSts_g |= IO_PIN_BOOT_SEL;



    ullTimebasePeriod_ns = (uint64_t)1000000000/((uint64_t)(sysInputFreq_g/4));

    vxTimeBaseGet(&ulBaseH, &ulBaseL);
    ullValue1 = (((uint64_t)ulBaseH)<<32)+(uint64_t)ulBaseL;
    ullValue1 = (ullValue1*ullTimebasePeriod_ns/(uint64_t)1000);
    ulResult1 = (uint32_t)(ullValue1&0xFFFFFFFF);
    #endif 

    SlowMessage = msgQCreate(100,	  /* max messages that can be queued */
                             sizeof(SLOW_MESSAGE_NODE), /* max bytes in a message */
                             MSG_Q_FIFO 	  /* message queue options */
                            );

    taskSpawn("tIdleStat", 255, 0, 1024, IdleStatEntry, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    

    #if 0
    vxTimeBaseGet(&ulBaseH, &ulBaseL);
    ulResult1 = (((uint64_t)ulBaseH)<<32)+(uint64_t)ulBaseL;
    idleStat.lstCount = idleStat.curCount;
    taskDelay(2);
    vxTimeBaseGet(&ulBaseH, &ulBaseL);
    ulResult2 = (((uint64_t)ulBaseH)<<32)+(uint64_t)ulBaseL;
    idleStat.maxCount = (idleStat.curCount - idleStat.lstCount)*((sysInputFreqGet()/4)/(ulResult2-ulResult1));
    idleStat.ulWdCnt = 0;
    idleStat.ullTotalCount = 0;

    idleInterval = sysClkRateGet ();
    idleIntervaOrin = idleInterval;
    idleStatWd = wdCreate ();

    wdStart (idleStatWd, idleInterval,
             (FUNCPTR) idleStatStart, (int) 0);
    
    /* �??动初始化主任务，该任务最终退�?? */

    #endif 

    InitTaskID_g = taskSpawn("tEdpInit", TSK_PRI_SYS, VX_FP_TASK, 100000, Init_Task,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (InitTaskID_g == ERROR)
    {
        printf("Init task failure.\n");
    }

    /* externMain is entered from defaultTask; finish by exiting current thread. */
    if (taskIdSelf() != 0)
    {
        (void)taskDelete(0);
    }

    return 0;

}

#if 0
/***********************************************************************
* MonitorInit - 监�?�任务初始化
*
* RETURNS: �??
*
*/
void MonitorInit()
{
    /* 建立异常监�?�任�?? */
    MonitorInitTaskID_g = taskSpawn("tMonitor", TSK_PRI_SYS, 0, 10000, Monitor,
                                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (MonitorInitTaskID_g == ERROR)
    {
        LOG_Dbg_Msg("MonitorInit failure.\n", 0, 0, 0, 0, 0, 0);
    }

    AddTaskToList(MonitorInitTaskID_g, TRUE,
                  "看门狗�?�位:因异常监视任务异常或退�??,看门狗�?�位CPU.\n", TRUE);
}


/***********************************************************************
* Monitor - 监�?�任务入口函�??
*
* RETURNS: �??
*
*/
int Monitor(
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
    int32_t iSecCnt = 0;
    int32_t iFatalErrSec = 0;
    EP_DATE_TIME dttm;
    uint32_t ulCnt = 0;

    while (1)
    {
        taskDelay(SYS_SEC);

        ulCnt++;
        if (++iSecCnt >= 60)
        {
            if (TM_Get_Sys_Time(&dttm) == EP_SUCCESS)
            {
                /* set the time tag of operation system every minutes. */
                if(dttm.unYear<2036)
                {
                    //sts = set_date(dttm.unYear, dttm.ucMonth, dttm.ucDate,
                    //               dttm.ucHour, dttm.ucMinute, dttm.ucSec);
                    //assert (sts == OK);
                }

                iSecCnt = 0;
            }
            else
                iSecCnt = 60;  /* Check system clock status every sec. */
        }

        /* 重启且非看门狗�?�位�?? */
        if ((uiEdpStatus_g & REBOOT_DLY) && (!EP_IN_WDG_RESET()))
        {
            EP_Set_Sts_Bit(REBOOT_DLY);
            iFatalErrSec++;


            /* 定值校验错�??和存储系统错�?? */
            if (ER_Sys_Err_Sts(EV_SET_ERR) || ER_Sys_Err_Sts(EV_STORAGE_ERR))
            {
                if (iFatalErrSec>100)
                {
                    //if (!EP_IS_BOOT_SEL())
                    //    EDPreboot(REBOOT_EXCEP);
                }
            }
            else if (iFatalErrSec>20)
            {
                //if (!EP_IS_BOOT_SEL())
                //    EDPreboot(REBOOT_EXCEP);
            }
        }
        else
            iFatalErrSec=0;
    }
}
#endif 

/* 空闲统�??.
 * Para:
 *     arg.
 * Return:
 *     OK, or ERROR.
 */
static int IdleStatEntry(
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
    while (1)
    {
        idleStat.curCount++;
    }
}

#if 0
/* 空闲统�?�函�??
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
static void idleStatStart(void)
{
    idleStat.intCount = idleStat.curCount-idleStat.lstCount;
    idleStat.lstCount = idleStat.curCount;

    /* 计数�??�??,每个周期�??加一�?? */
    idleStat.ulWdCnt++;
    idleStat.ullTotalCount += idleStat.intCount;

    /* 统�?�扫描任务消�?? */
    RE_StatTaskComsumeTimeStat(idleInterval);

    wdStart (idleStatWd, idleInterval,
             (FUNCPTR) idleStatStart, (int) 0);
}
#endif

/***********************************************************************
* Init_Task - 初�?�化任务入口函数
*
* RETURNS: �??
*
*/
int Init_Task(int arg1, int arg2, int arg3, int arg4, int arg5,
              int arg6, int arg7, int arg8, int arg9, int arg10)
{
    EP_DATE_TIME dttm;
    char shutofftimestr[TEMP_INFO_MAX_LEN];

    STATUS sts= ERROR;

    int iShellTaskID;	/* 优先级修�?? */
    int iLogTaskID;
    int iWdbTaskID = -1;
    int iSpyTaskID;
    int iExcTaskID;
    uint32_t ulBeginTime;
    uint32_t ulEndTime;
    uint32_t RunTime;

    #if 0
    /* EDP02平台禁�?�告�?? */
    if (bdType_g == BOARD_TYPE_E02)
    {
        SIO_Disable_Alm();
    }
    #endif

    /* 根据跳线来决定是否有调试信息 */
    LOG_Dbg_Msg("Init task begins.\n", 0, 0, 0, 0, 0, 0);

    #if 0
    iShellTaskID = taskNameToId("tShell" );	   /* 任务优先级修�?? */
    iLogTaskID = taskNameToId("tLogTask");
    iSpyTaskID = taskNameToId("tSpyTask");
    iExcTaskID = taskNameToId("tExcTask");	/* 异常任务 */
    taskPrioritySet(iShellTaskID, 71);
    taskPrioritySet(iLogTaskID, 61);
    taskPrioritySet(iSpyTaskID, TSK_PRI_SPY);
    #endif

    #if 0
    /* 仅EDP01平台C-A版本调整 */
    if (bdType_g == BOARD_TYPE_E01)
    {
        taskPrioritySet(iExcTaskID, TSK_PRI_SEVENTH_BACKUP_RELAY_SCAN+1);
        taskOptionsSet(iExcTaskID, VX_UNBREAKABLE, 0);		/* 调整为可打断 */
    }

    kernelTimeSlice(5);		/* 设置同级任务�??�??时间片为5tick，为50�??�?? */
    #endif

    EP_Set_04CPU_Init_End_Flag(FALSE);    /* 设置04版初始化�??完成 */
    //iIMMR_g = vxImmrGet();         /* Set global IMMR value. */

    VI_Init_Sem();	/* 事件和报告有关信号量初�?�化 */
    FileSyn_Init_Sem();		/* 初�?�化文件列表相关变量 */

    /* 所有平台都�??持呼唤功�??
     */
    ER_InitAlertFunc();		/* 初�?�化呼唤功能 */

    RunTimeTag.uCounter = 0;		/* 程序执�?�点计数 */

    AddRunTimeTag("Init task begins");
    ulBeginTime = TM_Get_usCnt();	/* 记录开始运行时间点 */

    #if 0
    /* �??动看门狗任务 */
    if (EP_Hw_Watch_Dog_Init() != EP_SUCCESS)
    {
        LOG_Dbg_Msg("Hw Watch dog Init failure.\n", 0, 0, 0, 0, 0, 0);
    }
    #endif

    dttm.unYear = 2000;
    dttm.ucMonth = 1;
    dttm.ucDate = 1;
    dttm.ucHour = 0;
    dttm.ucMinute = 0;
    dttm.ucSec = 0;
    dttm.unMSEL = 0;
    dttm.unMicroSec = 0;

    #if 0
    /* 函数调用内部进�?�平台区�??
     */
    if (GetClock(&dttm) == EP_SUCCESS)		/* 获取时钟�??片存储时�??*/
    {
        LOG_Dbg_Msg("获取时钟信号正常.\n", 0, 0, 0, 0, 0, 0);

        /* 如果从时钟芯片中读出的时间�?�常,
         * 则置对时正常
         */
        SetAdjustTimeSuccessFlag(TRUE);
    }
    else
    {
        LOG_Dbg_Msg("获取时钟信号失败.\n", 0, 0, 0, 0, 0, 0);
    }
    #endif

    #if 0
    LOG_Dbg_Msg("当前时间: %d-%d-%d-%d-%d-%d\n", dttm.unYear, dttm.ucMonth,
                dttm.ucDate, dttm.ucHour, dttm.ucMinute, dttm.ucSec);
    TM_Set_Sys_Time(&dttm, FALSE);		/* 设置系统时间 */

    /* 设置操作系统时间 */
    if(dttm.unYear<2036)
    {
        sts = set_date(dttm.unYear, dttm.ucMonth, dttm.ucDate,
                       dttm.ucHour, dttm.ucMinute, dttm.ucSec);
        assert (sts == OK);
    }
    #endif

    #if 0
    if (EP_IS_BOOT_SEL())    /* 调试时显示时�?? */
    {
        if (show_date() != OK)		/* 显示vxWorks系统时间 */
        {
            LOG_Dbg_Msg("操作系统时间读出错�??.\n", 0, 0, 0, 0, 0, 0);
        }
    }
    #endif

    if (FT_Init_Cfg() != EP_SUCCESS)
    {
        /* 文件系统初�?�化 */
        uiInitErrFlag_g |= INIT_FT_ERR;		/* 报错，但不重�?? */
    }
    AddRunTimeTag("FT_Init_Cfg");


    if (LOG_Init() != EP_SUCCESS)
    {
        /* 日志初�?�化 */
        uiInitErrFlag_g |= INIT_LOG_ERR;

        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "日志模块异常\n", 0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "Log module error\n", 0, 0);
        }
    }
    AddRunTimeTag("LOG_Init");

    /*2011-6-30�??  ZY 必须放到LOG初�?�化之后, VI_InitSn(访问了INI文件)之前 */
    FT_Init_Sys_Ini_File();

    //FileCRC_Init();

    #if 0
    /* 初�?�化NAND flash */
    if (sysInputFreq_g == BUS_FREQ_50MHZ)
    {
    }
    else if (sysInputFreq_g == BUS_FREQ_66MHZ)
    {
        Only_Init_Nand();
    }
    else
    {
        Only_Init_Nand();
    }
    #endif

    /* 附加配置信息初�?�化 */
    FT_Init_Auc_Sys_Ini_File();

    if (VI_InitSn() == EP_ERROR)
    {
        LOG_Write(LOG_KERNEL, "读取初�?�报告号失败.\n", NULL);
    }
    //EB_GetLanguageType();		/* Language select */

    TM_To_Dttm(0, &EP_dtRebootTime_g); 	/* Get the time of rebooting. */

    //Exc_WrRebootInfo(REBOOT_UNKNOWN);	/* 初�?�写�??reboot相关信息 */

    /* 读取索引定值页序出�?? */
    if (!AdMdType.bValid)
    {
        if (ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "读取索引定值页序出�??.\n", NULL);
        }
        else if (ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Can not read the index setting page number.\n", NULL);
        }
    }

    /* 相关版本号写入日�??
     */
    VER_WrVer();

    TM_To_Dttm(TM_Get_usCnt(), &ulNetInitBeginUs_g);	/* 网络开始初始化之前的时间点 */


    /* 铁电有效性�?��?
     */
    CheckFset();

    #if 0
    /* 读取铁电存储时间
     * 没有对时之前, 使用该时间�?�置系统时间
     * 但事�??/日志等的写入必须先等待�?�时,
     * 对时没有成功才使�?�??�时�??
     */
    if(ReadOfftime(shutofftimestr))
    {
        if(!GetAdjustTimeSuccessFlag())
        {
            if(GetDttmfromStr(shutofftimestr,&dttm))
            {
                TM_Set_Sys_Time(&dttm, FALSE);
            }
        }
    }
    #endif

    #if 0
    if (NT_NetCfgInit() != EP_SUCCESS)
    {
        if (ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Net configuration module initialization error.\n", NULL);
        }
        else if (ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "网络配置初�?�化失败\n", NULL);
        }
    }

    SysUsBeginCount_g = TM_Get_usCnt();
    #endif

    #if 0
    if (EP_IS_FAST_BOOT())
    {
        if (ENG_MODE == 0)
            LOG_Write(LOG_KERNEL, "超级终�??开�??.\n", NULL);
        else
            LOG_Write(LOG_KERNEL, "Open terminal.\n", NULL);
    }

    if (EP_IS_BOOT_SEL())
    {
        if (ENG_MODE == 0)
            LOG_Write(LOG_KERNEL, "网络引�?�模�??.\n", NULL);
        else
            LOG_Write(LOG_KERNEL, "Net Boot Mode.\n", NULL);
    }
    #endif

    //if (!EP_IS_BOOT_SEL())		/* �??能在非调试态挂接异常�?�理 */
    //    Exc_SysregExcHandle();

    {
        /*在扫描逻辑图前，先初�?�化录波共用的临时缓冲区*/
        extern void Init_Tmp_Recbuf();
        Init_Tmp_Recbuf();
    }

    if (SC_Init_Func_Cfg(EP_LGC_CFG_FILE) != EP_SUCCESS)
    {
        /*必须在软�??件配�??前面，因为配�??用到了�?�信�?? */
        uiInitErrFlag_g |= INIT_FUNC_ERR;

        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "保护元件配置状态异常\n", 0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "Function config error\n", 0, 0);
        }
    }
    AddRunTimeTag("SC_Init_Func_Cfg");

    {
        /*读取并解析保护应用功能选配�??*/
        extern void ReadFile_FuncOptIni();
        ReadFile_FuncOptIni();
    }

    if (RD_Initialize(EP_HW_CFG_FILE) != EP_SUCCESS)
    {
        /* �??件配�?? */
        uiInitErrFlag_g |= INIT_RD_ERR;

        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "�??件配�??异常\n", 0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "Hardware config error\n", 0, 0);
        }
    }
    AddRunTimeTag("RD_Initialize");

    if (SC_Initialize(EP_SW_CFG_FILE) != EP_SUCCESS)
    {
        /* �??件配�?? */
        uiInitErrFlag_g |= INIT_SC_ERR;
        LOG_Dbg_Msg("Error, 04 board software configure component init failure.\n", 0, 0, 0, 0, 0, 0);
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "�??件配�??异常\n", 0, 0);

        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "Software config error\n", 0, 0);
        }
    }
    AddRunTimeTag("SC_Initialize");

    if(SC_RD_SetRange() != EP_SUCCESS)
    {
        LOG_Write(LOG_KERNEL, "读取定值量程文件失败。\n", NULL);
    }

    #if 0
    /* �??前只有F-A板支持FPGA配置
     */
    if (VER_GetHwBoardSN() == E02_CPU_F_BORAD)
    {
        /* 以太网模式初始化 */
        if (GetFPGASampDataInit() == TRUE)
        {
            LOG_Write(LOG_KERNEL, "数字化应�??,FPGA模式,�??持以�??网口!\n", NULL);
        }
        else  /* �??前不�??持两种模�?? */
        {
            /* SPT总线模式 */
            if (GetSampDataInit() != TRUE)
            {
                LOG_Write(LOG_KERNEL, "数字化应�??,FPGA模式,�??持SPT总线!\n", NULL);
            }
        }

        appType_g = APP_TYPE_DIG;
    }
    else
#endif

    {

        /* 缺省态为传统应用模式,
         * 如果数字化采样配�??文件smv.xml解析正常,
         * 则为数字化应用模�??
         */
        if (GetSampDataInit() != TRUE)
        {
            LOG_Write(LOG_KERNEL, "传统应用!\n", NULL);
            appType_g = APP_TYPE_TRAD;

            if(FT_Is_File(EP_EDP_DSamSts_FILE))
            {
                /* 传统应用使用传统采样，删除合并器延时保存文件 */
                remove(EP_EDP_DSamSts_FILE);
            }
        }
        else
        {
            LOG_Write(LOG_KERNEL, "数字化应�??!\n", NULL);
            appType_g = APP_TYPE_DIG;
        }
    }

    /* CPU光功率初始化,
    母差装置有两�??电源，当CC不断电，CPU�??电时会造成光功率数组中
    CPU不在�??0的位�??，造成后续出问�??*/
    //EP_InitCPUWatt();

    /* 初�?�化光功率报文队�?? */
    //GsInitQue(&g_OptWattRcv);

    /* 初�?�化GOOSE功能,延后至所有配�??完成
     */
    //RD_InitGoose();

    #if 0
    if (TM_Initialize() != EP_SUCCESS)
    {
        /* 时钟模块初�?�化 */
        uiInitErrFlag_g |= INIT_TM_ERR;
    }
    #endif

    if (appType_g == APP_TYPE_TRAD)
    {
        /* 传统应用 */

        /* �??有EDP01平台�??持扩展机�??
         */
        if (bdType_g == BOARD_TYPE_E01)
        {
            //motFccRawInit();		 		/* 初�?�化扩展机�?�接�?? */
        }
    }

    /* 提示:
     * sysinfo.c如果上面解析的配�??文件�??改了�??
     * 生成电子盘上的新的edpdi.set，edplink.set，edpfunc.set，edpset.idz文件
     * 生成新的系统信息文件syscfg.sci
     * �??改edp01.ini文件�??的相关信�??
     */

    if (SI_Chk_Cfg() != EP_SUCCESS)
    {
        uiInitErrFlag_g |= INIT_SI_ERR;
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "保护配置异常\n", 0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "Relay config error\n", 0, 0);
        }
    }
    AddRunTimeTag("SI_Chk_Cfg");

    /* 提示:
     * swcfg.c 上电后�?�查保护�??�置
     * 根据edpdi.set，edplink.set，edpfunc.set，edpset.idz�??/set/areaxx.dza文件
     * 检查并设定开入量、压板状态、保护、内部定值、运行定值区�??
     */
    if (SC_Chk_Set() != EP_SUCCESS)
    {
        uiInitErrFlag_g |= INIT_SET_ERR;
    }
    AddRunTimeTag("SC_Chk_Set");


    if (RD_Boot_Dsp() != EP_SUCCESS)
    {
        uiInitErrFlag_g |= INIT_DSP_ERR;
    }
    AddRunTimeTag("RD_Boot_Dsp");

    if (!(uiInitErrFlag_g
            & (INIT_RD_ERR)))
    {
        /* 无错则执行以下程�?? */
        if (Relay_Engine_Activate(EP_LGC_CFG_FILE) != EP_SUCCESS)
        {
            uiInitErrFlag_g |= INIT_LGC_ERR;
        }
        AddRunTimeTag("Relay_Engine_Activate");
    }
    else
    {
        LOG_Write(LOG_KERNEL, "�??件配�??错�??,逻辑处理任务�??�??�??!!\n", NULL);

        EP_Set_04CPU_Init_End_Flag(TRUE);
    }

    RunTime = tickGet();/* 用于保证光功率报文能够有效接�?? */

    /* 所有平台支持光�??,根据配置进�?��?�理
     */
    taskDelay(100);
    //InitOptBoxChn2();

    SysUsBeginCount_g = TM_Get_usCnt();

    /*
     * 由于sci文件生成较慢, 导致mmi�??动有�??能�?�到�??sci文件,
     * 因�?�这里�?�判断sci�??否需要改�??, 如果需要改变则删除sci文件, 让mmi等待,
     * �??前只针�?�edp01和edp02
     */

    if (SI_Need_Reset_SCI())
    {
        bSciChangedFlag_g = TRUE;
        LOG_Write(LOG_RUN, "系统配置信息发生改变.\n", NULL);
    }

    com_init();   		/* 初�?�化通信功能 */

    #if 0
    /* 不�?�理STI�??
     */
    if (SI_Chk_Ver_INI() != EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_VERSION,
                       "版本校验异常\n", 0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_VERSION,
                       "Version check error\n", 0, 0);
        }
    }
    #endif


    /* 不�?�理STI�??
     */
    if (SI_Init_Sys_Info(EP_SYS_INFO_FILE) != EP_SUCCESS)
    {
        /* 生成系统信息文件 */
        uiInitErrFlag_g |= INIT_SI_ERR;

        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "系统配置异常\n", 0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "System config error\n", 0, 0);

        }
    }
    AddRunTimeTag("SI_Init_Sys_Info");

    /* 同�?�文件列表初始化 */
    if (FileSynInit() != EP_SUCCESS)
    {
        uiInitErrFlag_g |= INIT_SI_ERR;
    }

    if (FT_After_Relay_Init_Cfg() != EP_SUCCESS)
    {
        /* 保护�??动之后的文件系统初�?�化 */
        uiInitErrFlag_g |= INIT_FT_ERR;/*报错,但不重启  */
    }
    AddRunTimeTag("FT_After_Relay_Init_Cfg");

    /* 提示:
     * view.c 设定事件报告的最大数�??，并且启动一�??任务，不�??地�?�录事件报告
     */

    if (VI_Initialize() != EP_SUCCESS)
    {
        uiInitErrFlag_g |= INIT_VI_ERR;

        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "事件模块异常\n", 0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "Event module error\n", 0, 0);
        }
    }
    AddRunTimeTag("VI_Initialize");

    if (RC_After_Relay_Init() != EP_SUCCESS)
    {
        uiInitErrFlag_g |= INIT_RC_ERR;
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                       "录波模块异常\n", 0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT|ER_ALARM|ER_LOCK,
                       "Fault recorder error\n", 0, 0);
        }
    }

    /* 初�?�化日志功能 */
    CPU_DetailOpr_Log_Init();

    if (uiEdpStatus_g & NO_LICENSE)
    {
        LOG_Dbg_Msg("WARNING: no valid license. Running in demo mode.\n", 0, 0, 0, 0, 0, 0);
    }

    if (!(uiInitErrFlag_g & ~INIT_AU_ERR))
        LOG_Dbg_Msg("System begins running OK.\n", 0, 0, 0, 0, 0, 0);
    else if (!(uiInitErrFlag_g & ~(INIT_AU_ERR | INIT_LGC_ERR)))
        LOG_Dbg_Msg("Ready to download user part and debug.\n", 0, 0, 0, 0, 0, 0);
    else
        LOG_Dbg_Msg("Please correct all ERRORs then reboot.\n", 0, 0, 0, 0, 0, 0);

    iWdbTaskID = taskNameToId("tWdbTask");

    AddRunTimeTag("mmi_task_main");

    Init_Smv_Go_CommStat_File_Log();

    SlowTaskProcessInit();		/* 慢速�?�理任务 */
    //MonitorInit();		/* 监�?�任�?? */

    ulEndTime = TM_Get_usCnt();
    LOG_Dbg_Msg("保护�??动时间为%dms\n", (ulEndTime-ulBeginTime)/1000, 0, 0, 0, 0, 0);

    EP_Set_04CPU_Init_End_Flag(TRUE);
    
    #if 0
    if (!EP_IS_BOOT_SEL())
    {
        GetSysTaskStatus();
    }
    #endif

    #if 0
    while((tickGet()-RunTime)<600)
    {
        /* 保证光功率报文接收函数注册时间超�??6�?? */
        taskDelay(50);
    }
    #endif

    //EDP_CheckCcStatus();


    return 0;
}

/***********************************************************************
* EP_Set_ReSet_Flag - 设置需复归标志，逻辑图中设置
*
* RETURNS: TRUE, or FALSE
*
*/
void EP_Set_ReSet_Flag()
{
    taskLock();
    bReSetFlag_g = TRUE;
    taskUnlock();
}

/***********************************************************************
* EP_Clr_ReSet_Flag - �?�??�需复归标志，慢速任务中执�?��?�归操作后恢�??
*
* RETURNS: TRUE, or FALSE
*
*/
void EP_Clr_ReSet_Flag()
{
    taskLock();
    bReSetFlag_g = FALSE;
    taskUnlock();
}

/***********************************************************************
* EP_Get_ReSet_Flag - 得到需复归标志,慢速任务根�??该标志来进�?��?�归操作
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL EP_Get_ReSet_Flag()
{
    return bReSetFlag_g;
}

/***********************************************************************
* EP_Set_Sts_Bit - Set system status(OR operation)
*
* RETURNS: �??
*
* Alert:
*        This function can be called in ISR.
*
*/
void EP_Set_Sts_Bit(
    u_int uiSts			/* new bit flag to be set */
)
{
    int iLockKey;
    BOOL bLanguageChgFlag = FALSE;
    SLOW_MESSAGE_NODE Info;

    /* 若�?�置英文版，则需要发事件和�?�录到日志中 */
    if (!(uiEdpStatus_g & SYS_ENG_MODE) && (uiSts & SYS_ENG_MODE))
    {
        if (ENG_MODE == 0)
        {
            LOG_Dbg_Msg("切换到英文版.\n", 0, 0, 0, 0, 0, 0);
            LOG_Write(LOG_RUN, "切换到英文版.\n", NULL);
        }
        else if (ENG_MODE == 1)
        {
            LOG_Dbg_Msg("switch to english mode\n", 0, 0, 0, 0, 0, 0);
            LOG_Write(LOG_RUN, "switched to english mode.\n", NULL);
        }
    }

    /* 若�?�置远方态，则需要发事件和�?�录到日志中 */
    if (!(uiEdpStatus_g & ON_FAR_STATE) && (uiSts & ON_FAR_STATE))
    {
        if (ENG_MODE == 0)
        {
            LOG_Dbg_Msg("切换到远方�?.\n", 0, 0, 0, 0, 0, 0);
            LOG_Write(LOG_RUN, "切换到远方�?.\n", NULL);
        }
        else if (ENG_MODE == 1)
        {
            LOG_Dbg_Msg("switch to remote mode\n", 0, 0, 0, 0, 0, 0);
            LOG_Write(LOG_RUN, "switched to remote mode.\n", NULL);
        }
    }

    /* 若�?�置检�??态，则需要发事件和�?�录到日志中 */
    if (!(uiEdpStatus_g & ON_EXAM_STATE) && (uiSts & ON_EXAM_STATE))
    {
        if (ENG_MODE == 0)
        {
            LOG_Dbg_Msg("切换到�?��?�??.\n", 0, 0, 0, 0, 0, 0);
            LOG_Write(LOG_RUN, "切换到�?��?�??.\n", NULL);
        }
        else if (ENG_MODE == 1)
        {
            LOG_Dbg_Msg("switch to repair state.\n", 0, 0, 0, 0, 0, 0);
            LOG_Write(LOG_RUN, "switched to repair state.\n", NULL);
        }
    }

    /* 切换�??言模式 */
    if (!(uiEdpStatus_g & SYS_ENG_MODE) && (uiSts & SYS_ENG_MODE))
    {
        uiEdpStatus_g |= SYS_ENG_MODE;
        LOG_Dbg_Msg("switched to english mode.\n", 0, 0, 0, 0, 0, 0);

        ENG_MODE = 1;
        bLanguageChgFlag = TRUE;
    }

    iLockKey = intLock();

    if (!(uiEdpStatus_g & (SYS_LOCK_DO | LOCK_DO)) &&
            (uiSts & (SYS_LOCK_DO | LOCK_DO)))
    {
        SIO_Disable_DO();		/* Disable the DO. */
        LOG_Dbg_Msg("ALERT: DO is disable.\n", 0, 0, 0, 0, 0, 0);
    }

    if (!(uiEdpStatus_g & (SYS_LOCK_EVT | LOCK_EVT)) &&
            (uiSts & (SYS_LOCK_EVT | LOCK_EVT)))
    {
        LOG_Dbg_Msg("ALERT: event message is disable.\n", 0, 0, 0, 0, 0, 0);
    }

    uiEdpStatus_g |= uiSts;

    intUnlock(iLockKey);

    if (bLanguageChgFlag && (ENG_MODE == 1))
    {
        FT_Wr_Sys_INI("[SYSTEM]", "Language","1");
        if (FT_Is_File(EP_SYS_INFO_FILE))
        {
            remove(EP_SYS_INFO_FILE);
        }
        SI_Creat_SCI(EP_SYS_INFO_FILE);

        /* EDP01平台与扩展机箱通信更新�??言类型
         */
        if (bdType_g == BOARD_TYPE_E01)
        {
            Info.type = LANGUAGECHG;
            msgQSend(SlowMessage, (char *)&Info, sizeof(SLOW_MESSAGE_NODE),
                     NO_WAIT, MSG_PRI_NORMAL);
        }
    }
}

/***********************************************************************
* EP_Clr_Sts_Bit - Clear system status(AND operation)
*
* RETURNS: �??
*
* Alert:
*        This function can be called in ISR.
*
*/
void EP_Clr_Sts_Bit(
    u_int uiSts			/* bit flag to be cleared */
)
{
    int iLockKey;
    BOOL bLanguageChgFlag = FALSE;
    SLOW_MESSAGE_NODE Info;

    /* 若�?�置英文版，则需要发事件和�?�录到日志中 */
    if ((uiEdpStatus_g & SYS_ENG_MODE) && (uiSts & SYS_ENG_MODE))
    {
        if (ENG_MODE == 0)
        {
            LOG_Dbg_Msg("切换到中文版.\n", 0, 0, 0, 0, 0, 0);
            LOG_Write(LOG_RUN, "切换到中文版.\n", NULL);
        }
        else if (ENG_MODE == 1)
        {
            LOG_Dbg_Msg("switch to chinese mode\n", 0, 0, 0, 0, 0, 0);
            LOG_Write(LOG_RUN, "switched to chinese mode.\n", NULL);
        }
    }

    /* 若�?�置就地态，则需要发事件和�?�录到日志中 */
    if ((uiEdpStatus_g & ON_FAR_STATE) && (uiSts & ON_FAR_STATE))
    {
        if (ENG_MODE == 0)
        {
            LOG_Dbg_Msg("切换到就地�?.\n", 0, 0, 0, 0, 0, 0);
            LOG_Write(LOG_RUN, "切换到就地�?.\n", NULL);
        }
        else if (ENG_MODE == 1)
        {
            LOG_Dbg_Msg("switch to local mode.\n", 0, 0, 0, 0, 0, 0);
            LOG_Write(LOG_RUN, "switched to local mode.\n", NULL);
        }
    }

    /* 若�?�置非�?��?态，则需要发事件和�?�录到日志中 */
    if ((uiEdpStatus_g & ON_EXAM_STATE) && (uiSts & ON_EXAM_STATE))
    {
        if (ENG_MODE == 0)
        {
            LOG_Dbg_Msg("切换到非检�??�??.\n", 0, 0, 0, 0, 0, 0);
            LOG_Write(LOG_RUN, "切换到非检�??�??.\n", NULL);
        }
        else if (ENG_MODE == 1)
        {
            LOG_Dbg_Msg("switch to no check state.\n", 0, 0, 0, 0, 0, 0);
            LOG_Write(LOG_RUN, "switched out of repair state.\n", NULL);

        }
    }

    /* 切换�??言模式 */
    if ((uiEdpStatus_g & SYS_ENG_MODE) && (uiSts & SYS_ENG_MODE))
    {
        uiEdpStatus_g &= ~SYS_ENG_MODE;
        LOG_Dbg_Msg("switched to chinese mode.\n", 0, 0, 0, 0, 0, 0);

        bLanguageChgFlag = TRUE;
        ENG_MODE = 0;
    }

    iLockKey = intLock();

    /* NO_LICENSE bit not allowed clear. */
    uiEdpStatus_g &= (~uiSts | NO_LICENSE);

    intUnlock(iLockKey);

    if ((uiSts & (SYS_LOCK_DO | LOCK_DO)) &&
            !(uiEdpStatus_g & (SYS_LOCK_DO | LOCK_DO)))
    {

    }

    if ((uiSts & (SYS_LOCK_EVT | LOCK_EVT)) && !
            (uiEdpStatus_g & (SYS_LOCK_EVT | LOCK_EVT)))
    {

    }

    if (bLanguageChgFlag && (ENG_MODE == 0))
    {
        FT_Wr_Sys_INI("[SYSTEM]", "Language","0");
        if (FT_Is_File(EP_SYS_INFO_FILE))
        {
            remove(EP_SYS_INFO_FILE);
        }
        SI_Creat_SCI(EP_SYS_INFO_FILE);

        /* EDP01平台与扩展机箱通信更新�??言类型
         */
        if (bdType_g == BOARD_TYPE_E01)
        {
            Info.type = LANGUAGECHG;
            msgQSend(SlowMessage, (char *)&Info, sizeof(SLOW_MESSAGE_NODE),
                     NO_WAIT, MSG_PRI_NORMAL);
        }
    }
}

/***********************************************************************
* EP_Bgn_Hw_Test - Enter hardware test mode.  Logic function will be disabled
*
* RETURNS: �??
*
*/
void EP_Bgn_Hw_Test(void)
{
    int iLockKey;

    iLockKey = intLock();

    if (uiEdpStatus_g & HW_TEST_MODE)
    {
        intUnlock(iLockKey);

        return;
    }

    uiEdpStatus_g |= HW_TEST_MODE;

    intUnlock(iLockKey);

    if (ENG_MODE == 0)
    {
        LOG_Write(LOG_OPRATE,
                  "测试模式: 进入�??件测试模�??,所有保护功能退�??.\n", NULL);
    }
    else if (ENG_MODE == 1)
    {
        LOG_Write(LOG_OPRATE,
                  "Test mode: Enter hardware test mode, all protection relay disabled.\n", NULL);
    }

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)			/* 励�?�平台也使用此函�?? */
    VI_Clear_Evt(0);			/* 先�?�归 */
#endif

#if defined(EDP_01_02_BUILD)
    /* 为了防�?�原来的DO设置还起作用，需要先将其清零
     * 不�?�理扩展机�??
     */
    RD_Clear_All_Phy_DO();
    VI_Clear_Evt();			/* 先�?�归 */
    SIO_Enable_DO();
#endif
}

/***********************************************************************
* EP_End_Hw_Test - Exit hardware test mode.  System will reboot after several seconds
*
* RETURNS: �??
*
*/
void EP_End_Hw_Test(void)
{
    if (uiEdpStatus_g & HW_TEST_MODE)
    {
#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)			/* 励�?�平台也使用此函�?? */
        VI_Clear_Evt(0);			/* 先�?�归 */
#endif

#if defined(EDP_01_02_BUILD)
        /* 为了防�?�原来的DO设置还起作用，需要先将其清零
         * 不�?�理扩展机�??
         */
        RD_Clear_All_Phy_DO();
        VI_Clear_Evt();			/* 先�?�归 */
#endif

#ifndef EXCITE_BUILD
        SIO_Disable_DO();
#endif
        if (ENG_MODE == 0)
        {
            LOG_Write(LOG_OPRATE,
                      "测试模式: 退出硬件测试模�??, 所有保护功能重新投�??.\n", NULL);
        }
        else if (ENG_MODE == 1)
        {
            LOG_Write(LOG_OPRATE,
                      "Test mode: Exit hardware test mode, all protection relay enabled again.\n", NULL);
        }

        /* 退出测试模式后重新刷新压板 */
        RE_SetLogSetChgCnt();
        EP_Clr_Sts_Bit(HW_TEST_MODE);
    }
}

/***********************************************************************
* EP_Bgn_Logic_Stop - Enter logic scanning stopping mode.  Logic function will be disabled
*
* RETURNS: �??
*
*/
void EP_Bgn_Logic_Stop(void)
{
    int iLockKey;

    iLockKey = intLock();

    if (uiEdpStatus_g & STOP_LOGIC_SACN_FLAG)
    {
        intUnlock(iLockKey);

        return;
    }

    SIO_Disable_DO();		/* Must disable the DO. */
    uiEdpStatus_g |= STOP_LOGIC_SACN_FLAG;

    intUnlock(iLockKey);
}

/***********************************************************************
* EP_End_Logic_Stop - Exit logic scanning stopping mode.
*
* RETURNS: �??
*
*/
void EP_End_Logic_Stop(void)
{
    if (uiEdpStatus_g & STOP_LOGIC_SACN_FLAG)
    {
        SIO_Disable_DO();
        EP_Clr_Sts_Bit(STOP_LOGIC_SACN_FLAG);
    }
}

/***********************************************************************
* EP_Get_Logic_Stop - Get logic scanning stopping mode.
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL EP_Get_Logic_Stop(void)
{
    if (uiEdpStatus_g & STOP_LOGIC_SACN_FLAG)
    {
        SIO_Disable_DO();
        EP_Clr_Sts_Bit(STOP_LOGIC_SACN_FLAG);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/***********************************************************************
* EP_Setlock - 设置挂锁状�?.
*
* RETURNS: �??
*
*/
void EP_Setlock(void)
{
    EP_Set_Sts_Bit(JGS_STATE);
}

/***********************************************************************
* EP_SetUnlock - 设置解锁状�?.
*
* RETURNS: �??
*
*/
void EP_SetUnlock(void)
{
    if (uiEdpStatus_g & JGS_STATE)
    {
        EP_Clr_Sts_Bit(JGS_STATE);
    }
}

/***********************************************************************
* EP_GetLockSts - 获取解挂锁状态，同时清除该状�??.
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL EP_GetLockSts(void)
{
    if (uiEdpStatus_g & JGS_STATE)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/***********************************************************************
* EP_Before_Lgc_Run - Finish last system initializaion before logic running
*
* RETURNS: �??
*
* Alert:
*       This function is only called by RelayEngine module
*/
void EP_Before_Lgc_Run(void)
{
    if (RC_Initialize() != EP_SUCCESS)
        uiInitErrFlag_g |= INIT_RC_ERR;

#ifdef EXCITE_BUILD		/* 励�?�平�?? */
    if (AO_CfgInitFinish() != EP_SUCCESS)	/* AO初�?�化 */
        uiInitErrFlag_g |= INIT_AO_ERR;

    if (Init_Redun_Finish() != EP_SUCCESS)	/* 冗余机�?�初始化 */
        uiInitErrFlag_g |= INIT_REDUN_ERR;

    if (Init_OptComm_Finish() != EP_SUCCESS)	/* 光纤通�??初�?�化 */
        uiInitErrFlag_g |= INIT_OPTCOMM_ERR;
#endif

    /* 所有平台支持光�??,配置区分
     */
    if (OPT_AOCfgInitFinish() != EP_SUCCESS)   /* 通知光纵AO初�?�化完成 */
    {
        uiInitErrFlag_g |= INIT_RD_ERR;
        LOG_Dbg_Msg("光纵初�?�化失败.\n", 0, 0, 0, 0, 0, 0);
    }

#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)
    if (POLE_AOCfgInitFinish() != EP_SUCCESS)   /* 通知同杆并架虚拟机�?�AO初�?�化完成 */
    {
        uiInitErrFlag_g |= INIT_RD_ERR;
        LOG_Dbg_Msg("同杆并架初�?�化失败.\n", 0, 0, 0, 0, 0, 0);
    }

    if (HDL_AOCfgInitFinish() != EP_SUCCESS)   /* 通知智能操作箱初始化完成 */
    {
        uiInitErrFlag_g |= INIT_RD_ERR;
        LOG_Dbg_Msg("智能操作箱初始化失败.\n", 0, 0, 0, 0, 0, 0);
    }
#endif

#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)
    if (!InitAllGoYabanInfo())		/* 通知goose压板初�?�化完成 */
    {
        uiInitErrFlag_g |= INIT_SC_ERR;
        LOG_Dbg_Msg("goose压板配置初�?�化失败.\n", 0, 0, 0, 0, 0, 0);
    }
#endif

    /* 初�?�化开出配�??压板 */
    if (!RD_InitLinkofGsDo())
    {
        uiInitErrFlag_g |= INIT_SC_ERR;
        LOG_Dbg_Msg("goose do压板配置初�?�化失败.\n", 0, 0, 0, 0, 0, 0);
    }
}

/***********************************************************************
* EP_Hw_Watch_Dog_Init - 看门狗任务初始化
*
* RETURNS: �??
*
*/
static EP_STATUS EP_Hw_Watch_Dog_Init(void)
{
    int nTaskID;

    ulDspAccessCounter_g = 1;
    // swWatchDogInit();

    nTaskID = taskSpawn("tWatchDogTask",	/* 任务�?? */
                        TSK_PRI_HW_WATCH_DOG,	/* 优先�?? */
                        0, /* �??点支�?? */
                        5000,		/* 堆栈大小，由20K改为5K */
                        (FUNCPTR)EP_Hw_Watch_Dog_Handle,  /* 入口函数 */
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (nTaskID == ERROR)
    {
        LOG_Dbg_Msg("watch dog Task Create failure.\n", 0, 0, 0, 0, 0, 0);

        assert(FALSE);

        return EP_SYS_ERR;
    }

    return EP_SUCCESS;
}

/***********************************************************************
* EP_Hw_Watch_Dog_Handle - 看门狗任务�?�理函数
*
* RETURNS: �??
*
* alert:
*       Don't report event when watch dog reset.
*/
void EP_Hw_Watch_Dog_Handle(void)
{
#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
#define SET_WDG_RESET_FLAG_DELAY_TIME 120000000   /* 设置DOLOCK标志延迟时间, 2分钟, 让日志有足�?�的时间进�?��?�录 */
#else
#define SET_WDG_RESET_FLAG_DELAY_TIME 60000000  	/* 设置DOLOCK标志延迟时间, 1分钟, 让日志有足�?�的时间进�?��?�录 */
#endif

#define RUNTIMESPERSEC 4	/* 看门狗轮询间�?? */

#ifdef EDP01_CA_OPT_BUILD
    static unsigned long ulExtDogEnterCnt = 0;	/* Exception counter */
#endif

    static unsigned long ulSpyEnterCnt = 0;		/* tSpy task counter */
    uint32_t ulWdgResetTime = 0;
    BOOL bWdgResetFlag = FALSE;
    TTaskWatchDog *ptNode ;
    char strTaskStatus[128];
    TASK_DESC TaskDesc;
    char strTaskInfo[512];
    uint8_t nRelayTaskNo;	/* 保护任务�?? */
    uint8_t nTskSts;
    int iTaskFailCount = 0;
    int iTaskFailMax = 0;
    int iSpyTaskID;

    if (!EP_IS_BOOT_SEL())
        DisablePit();  		/* 正式运�?�关闭Pit */

#if defined(EDP03_INTELBOX_BUILD)
    iTaskFailMax = 10;
#else
    if (appType_g == APP_TYPE_DIG)
    {
        /* 数字化应�?? */
        iTaskFailMax = 5;
    }
    else if (appType_g == APP_TYPE_TRAD)
    {
        /* 传统应用 */
        iTaskFailMax = 2;
    }
    else
    {
        assert (FALSE);
    }
#endif

    while (1)
    {
        if ((iSpyTaskID = taskNameToId("tSpyTask")) != ERROR)
        {
            ulSpyEnterCnt++;
            if (ulSpyEnterCnt == 1)
            {
                LOG_Dbg_Msg("Task tSpyTask Start.\n", 0, 0, 0, 0, 0, 0);
            }
            taskPrioritySet(iSpyTaskID, TSK_PRI_SPY);
        }
        else
        {
            ulSpyEnterCnt = 0;
        }

        if (!GetMonitorTaskStatus())
        {
            /*监控任务异常,  */
            if (!bWdgResetFlag)		/* 若未复位, 则异�?? */
            {
                EP_Set_Sts_Bit(SYS_LOCK_DO|WDG_RESET);	/* 为了防�?��??�??, 需要很�??�??�?? */
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "看门狗�?�位:因系统监视任务异常或退�??,看门狗�?�位CPU.\n", NULL);
                    LOG_Dbg_Msg("看门狗�?�位:因系统监视任务异常或退�??,看门狗�?�位CPU.\n",
                                0, 0, 0, 0, 0, 0);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Watchdog reset: because of task tMonitor is abnormal or dead, watchdog reset the CPU.\n", NULL);
                    LOG_Dbg_Msg("Watchdog reset: because of system monitor task is abnormal or dead, watchdog reset the CPU.\n",
                                0, 0, 0, 0, 0, 0);
                }
                bWdgResetFlag = TRUE;
                ulWdgResetTime = TM_Get_usCnt();
                TM_To_Dttm(ulWdgResetTime, &EP_dtNewWdRebootTime_g);
            }
        }

        /* 检测DSP任务 */
        if(bDspTaskStartFlag_g)
        {
            if (ulDspAccessCounter_g == 0)
            {
                if (!bWdgResetFlag)
                {
                    EP_Set_Sts_Bit(SYS_LOCK_DO|WDG_RESET);		/* 为了防�?��??�??, 需要很�??�??�?? */

                    if (ENG_MODE == 0)
                    {
                        LOG_Write(LOG_KERNEL, "看门狗�?�位:因DSP任务异常或退�??,看门狗�?�位CPU.\n", NULL);
                        LOG_Dbg_Msg("看门狗�?�位:因DSP任务异常或退�??,看门狗�?�位CPU.\n", 0, 0, 0, 0, 0, 0);
                    }
                    else if (ENG_MODE == 1)
                    {
                        LOG_Write(LOG_KERNEL,
                                  "Watchdog reset: because of task tDSPprocess is abnormal or dead, watchdog reset the CPU.\n", NULL);
                        LOG_Dbg_Msg("Watchdog reset: because of task tDSPprocess is unnormal or dead, watchdog reset the CPU.\n",
                                    0, 0, 0, 0, 0, 0);
                    }
                    bWdgResetFlag = TRUE;
                    ulWdgResetTime = TM_Get_usCnt();
                    TM_To_Dttm(ulWdgResetTime, &EP_dtNewWdRebootTime_g);
                }
            }
            else
            {
                ulDspAccessCounter_g = 0;
            }
        }

#ifdef EDP01_CA_OPT_BUILD

        /* Check if the sampling task based on photoelectron current transition is running normally. */
        if (bExtRecvTaskStartFlag_g)
        {
            /* The task have been created. */
            if (ulExtAccessCounter_g == 0)
            {
                /* unnormal if zero. */
                ulExtDogEnterCnt++;
                if ((!bWdgResetFlag) && (ulExtDogEnterCnt>8))		/* If the counter don't be refreshed in 2 second, report the alarm. */
                {
                    EP_Set_Sts_Bit(SYS_LOCK_DO|WDG_RESET);	/* 为了防�?��??�??, 需要很�??�??�?? */
                    if (ENG_MODE == 1)		/* English edition */
                    {
                        ER_Set_Err(EV_SAMPLE_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* call, report, alarm, lock */
                                   "Receive error(%02d)\n",
                                   NO_SAMPLING_DATE, 0);
                    }
                    else if (ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SAMPLE_ERR,
                                   ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 呼唤，报告，告�?�，�??�?? */
                                   "采样接收异常(%02d)\n", NO_SAMPLING_DATE, 0);
                    }
                    LOG_Write(LOG_KERNEL, "接收扩展板采样数�??任务异常!!\n",NULL);
                    bWdgResetFlag = TRUE;
                    ulWdgResetTime = TM_Get_usCnt();
                    TM_To_Dttm(ulWdgResetTime, &EP_dtNewWdRebootTime_g);
                }
            }
            else
            {
                ulExtAccessCounter_g = 0;
                ulExtDogEnterCnt = 0;		/* Clear 0 */
            }
        }

#endif

        if (!EP_IS_BOOT_SEL())
        {
            if (!RE_Get_Relay_Task_Run_State(&nRelayTaskNo, &nTskSts))
            {
                iTaskFailCount++;

                if (!bWdgResetFlag && (iTaskFailCount >= iTaskFailMax))
                {
                    EP_Set_Sts_Bit(SYS_LOCK_DO|WDG_RESET);	/* 为了防�?��??�??, 需要很�??�??�?? */

                    iTaskFailCount = 0;
                    if (ENG_MODE == 0)
                    {
                        if (nTskSts == 0x1)
                        {
                            sprintf(strTaskInfo,
                                    "看门狗�?�位: 因保护任�??%d�??20分钟内都没有创建, 看门狗�?�位CPU.\n",
                                    nRelayTaskNo);
                            LOG_Write(LOG_KERNEL, strTaskInfo, NULL);
                            LOG_Dbg_Msg("看门狗�?�位: 因保护任�??%d�??20分钟内都没有创建, 看门狗�?�位CPU.\n",
                                        nRelayTaskNo, 0, 0, 0, 0, 0);
                        }
                        else if (nTskSts == 0x2)
                        {
                            sprintf(strTaskInfo,
                                    "看门狗�?�位: 因保护任�??%d�??创建, 且采样已经驱�??, 但逻辑图没有扫�??, 看门狗�?�位CPU.\n",
                                    nRelayTaskNo);
                            LOG_Write(LOG_KERNEL, strTaskInfo, NULL);
                            LOG_Dbg_Msg("看门狗�?�位: 因保护任�??%d�??创建, 且采样已经驱�??, 但逻辑图没有扫�??, 看门狗�?�位CPU.\n",
                                        nRelayTaskNo, 0, 0, 0, 0, 0);
                        }
                        else if (nTskSts == 0x3)
                        {
                            sprintf(strTaskInfo,
                                    "看门狗�?�位: 因保护任�??%d已�??创建, �??10分钟内都没有�??驱动, 看门狗�?�位CPU.\n",
                                    nRelayTaskNo);
                            LOG_Write(LOG_KERNEL, strTaskInfo, NULL);
                            LOG_Dbg_Msg("看门狗�?�位: 因保护任�??%d已�??创建, �??10分钟内都没有�??驱动, 看门狗�?�位CPU.\n",
                                        nRelayTaskNo, 0, 0, 0, 0, 0);
                        }
                    }
                    else if (ENG_MODE == 1)
                    {
                        sprintf(strTaskInfo,
                                "Watchdog reset: because of relay protect task %d is abnormal or dead, watchdog reset the CPU.\n",
                                nRelayTaskNo);
                        LOG_Write(LOG_KERNEL, strTaskInfo, NULL);
                        LOG_Dbg_Msg("Watchdog reboot system: because of relay protect task %d is abnormal or dead,watchdog reboot the CPU.\n",nRelayTaskNo, 0, 0, 0, 0, 0);
                    }
                    bWdgResetFlag = TRUE;
                    ulWdgResetTime = TM_Get_usCnt();
                    TM_To_Dttm(ulWdgResetTime, &EP_dtNewWdRebootTime_g);
                }
            }
        }

        /* 日志记录任务检�?? */
        if (bLogWriteTaskStartFlag_g && (!GetLogTaskStatus()))
        {
            if (!bWdgResetFlag)
            {
                EP_Set_Sts_Bit(SYS_LOCK_DO|WDG_RESET);		/* 为了防�?��??�??, 需要很�??�??�?? */
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "看门狗�?�位:因日志�?�录任务异常或退�??,看门狗�?�位CPU.\n", NULL);
                    LOG_Dbg_Msg("看门狗�?�位:因日志�?�录任务异常或退�??,看门狗�?�位CPU.\n", 0, 0, 0, 0, 0, 0);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Watchdog reset: because of task tWrLog is abnormal or dead, watchdog reset the CPU.\n", NULL);
                    LOG_Dbg_Msg("Watchdog reset: because of task tWrLog is abnormal or dead, watchdog reset the CPU.\n",
                                0, 0, 0, 0, 0, 0);
                }
                bWdgResetFlag = TRUE;
                ulWdgResetTime = TM_Get_usCnt();
                TM_To_Dttm(ulWdgResetTime, &EP_dtNewWdRebootTime_g);
            }
        }

        /* 事件报告任务检�?? */
        if (bEvtMakeRtpTaskStartFlag_g && (!GetEvtMakeRptTaskStatus()))
        {
            if (!bWdgResetFlag)
            {
                EP_Set_Sts_Bit(SYS_LOCK_DO|WDG_RESET);		/* 为了防�?��??�??,需要很�??�??�?? */
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "看门狗�?�位:因事件报告任务异常或退�??,看门狗�?�位CPU.\n", NULL);
                    LOG_Dbg_Msg("看门狗�?�位:因事件报告任务异常或退�??,看门狗�?�位CPU.\n", 0, 0, 0, 0, 0, 0);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Watchdog reset: because of task tMakeRpt is abnormal or dead, watchdog reset the CPU.\n", NULL);
                    LOG_Dbg_Msg("Watchdog reset: because of task tMakeRpt is abnormal or dead, watchdog reset the CPU.\n",
                                0, 0, 0, 0, 0, 0);
                }
                bWdgResetFlag = TRUE;
                ulWdgResetTime = TM_Get_usCnt();
                TM_To_Dttm(ulWdgResetTime, &EP_dtNewWdRebootTime_g);
            }
        }

        /* 录波缓冲任务检�?? */
        if (bRecBufTaskStartFlag_g && (!GetRecBufTaskStatus()))
        {
            if (!bWdgResetFlag)
            {
                EP_Set_Sts_Bit(SYS_LOCK_DO|WDG_RESET);		/* 为了防�?��??�??,需要很�??�??�?? */
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "看门狗�?�位:因录波缓冲任务异常或退�??,看门狗�?�位CPU.\n", NULL);
                    LOG_Dbg_Msg("看门狗�?�位: 因录波缓冲任务异常或退�??,看门狗�?�位CPU.\n", 0, 0, 0, 0, 0, 0);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Watchdog reset: because of tRecBuf task is abnormal or dead, watchdog reset the CPU.\n", NULL);
                    LOG_Dbg_Msg("Watchdog reset: because of task tRecBuf is abnormal or dead, watchdog reset the CPU.\n",
                                0, 0, 0, 0, 0, 0);
                }
                bWdgResetFlag = TRUE;
                ulWdgResetTime = TM_Get_usCnt();
                TM_To_Dttm(ulWdgResetTime, &EP_dtNewWdRebootTime_g);
            }
        }

        /* 录波文件任务检�?? */
        if (bRecFileTaskStartFlag_g && (!GetRecFileTaskStatus()))
        {
            if (!bWdgResetFlag)
            {
                EP_Set_Sts_Bit(SYS_LOCK_DO|WDG_RESET);		/* 为了防�?��??�??,需要很�??�??�?? */
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "看门狗�?�位:因录波报告任务异常或退�??,看门狗�?�位CPU.\n", NULL);
                    LOG_Dbg_Msg("看门狗�?�位:因录波报告任务异常或退�??,看门狗�?�位CPU.\n", 0, 0, 0, 0, 0, 0);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Watchdog reset: because of  tRecFile task  is abnormal or dead, watchdog reset the CPU.\n", NULL);
                    LOG_Dbg_Msg("Watchdog reboot system: because of task tRecFile is abnormal or dead, watchdog reboot the CPU.\n",
                                0, 0, 0, 0, 0, 0);
                }
                bWdgResetFlag = TRUE;
                ulWdgResetTime = TM_Get_usCnt();
                TM_To_Dttm(ulWdgResetTime, &EP_dtNewWdRebootTime_g);
            }
        }

        for (ptNode=(TTaskWatchDog *)lstFirst(&m_WatchDogTaskList_g); ptNode != NULL;
                ptNode=(TTaskWatchDog *)lstNext((NODE*)ptNode))
        {
            if (ptNode->bTaskIsGood && (!GetTaskStatus(ptNode->iTaskID, strTaskStatus)))
            {
                if (ptNode->bRebootFlag)
                {
                    if (taskInfoGet(ptNode->iTaskID, &TaskDesc)==OK)
                    {
                        LOG_Write(LOG_KERNEL, ptNode->WarningMessage, NULL);
                    }
                    else
                    {
                        if (ENG_MODE == 0)
                        {
                            sprintf(strTaskInfo, "任务ID: %x 非法, 最近任务错�??状�?%s.\n",
                                    ptNode->iTaskID, ptNode->WarningMessage);
                        }
                        else if (ENG_MODE == 1)
                        {
                            sprintf(strTaskInfo, "Task ID: %x is invalid, the current error state is: %s.\n",
                                    ptNode->iTaskID, ptNode->WarningMessage);
                        }

                        LOG_Write(LOG_KERNEL, (const uint8_t *)strTaskInfo, NULL);
                    }
                    ptNode->bTaskIsGood = FALSE;
                    bWdgResetFlag = TRUE;
                    ulWdgResetTime = TM_Get_usCnt();
                    TM_To_Dttm(ulWdgResetTime, &EP_dtNewWdRebootTime_g);
                }
                else
                {
                    if (taskInfoGet(ptNode->iTaskID, &TaskDesc)==OK)
                    {
                        LOG_Write(LOG_KERNEL, ptNode->WarningMessage, NULL);
                    }
                    else
                    {
                        sprintf(strTaskInfo, "任务ID(%x), %s.\n",
                                ptNode->iTaskID, ptNode->WarningMessage);

                        LOG_Write(LOG_KERNEL, (const uint8_t *)strTaskInfo, NULL);
                    }
                    ptNode->bTaskIsGood = FALSE;
                }
            }
        }

        /* 61850功能检�??,设定区分
         */
        if (!Get61850FuncStatus())
        {
            if (!bWdgResetFlag)
            {

                EP_Set_Sts_Bit(SYS_LOCK_DO|WDG_RESET);/**/
                if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL, "watchdog reset :for 61850 func abnormal, reset cpu.\n", NULL);
                    LOG_Dbg_Msg("watchdog reboot :for 61850  func abnormal ,reboot cpu!\n",0,0,0,0,0,0);
                }
                else if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "看门狗�?�位:�??61850功能异常，看门狗复位CPU.\n", NULL);
                    LOG_Dbg_Msg("看门狗�?�位:�??61850功能异常,看门狗�?�位CPU.\n", 0, 0, 0, 0, 0, 0);
                }
                bWdgResetFlag = TRUE;
                ulWdgResetTime = TM_Get_usCnt();

            }
        }

        /* 击狗 */
        // kickSwDog();
        // kickHwDog();
        LightDevAbnormalHintLamp();  /* 点�?�置异常�?? */

        if (bWdgResetFlag
                && ((TM_Get_usCnt()-ulWdgResetTime)>SET_WDG_RESET_FLAG_DELAY_TIME))
        {
            /* 若看门狗重置, 则需要延迟一段时�??, 使得日志能�?�确记录 */
            if (!EP_IS_BOOT_SEL())
            {

                LOG_Dbg_Msg("dog reboot\n", 0, 0, 0, 0, 0, 0);
#ifndef EDP01_CA_OPT_BUILD 		/* Do not reboot in platform EDP01 */

                EDPreboot(REBOOT_EXCEP);
#endif
            }
        }

        taskDelay(SYS_SEC/RUNTIMESPERSEC);
    }
}

// #if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
// /***********************************************************************
// * isNumber_2_04CPU - �??否是04�??
// *
// * RETURNS:
// *         TRUE, �??
// *         FALSE, �??
// *
// * alert:
// *        其它平台由底层提供�?�函�??.
// */
// BOOL isNumber_2_04CPU(void)
// {
//     return FALSE;
// }
// #endif

/* 设置SPI通信主从标志
 * Para:
 *     1,主CPU;
 *     2,从CPU.
 * Return:
 *     NONE.
 */
void EP_Set02CPUPos(uint8_t ucPos)
{
    if ((ucPos == 1) || (ucPos == 2))
    {
        ucCpuSpiRol_g = ucPos;
    }
    else
    {
        ucCpuSpiRol_g = 0;
    }
}

BOOL Get61850FuncStatus(void){
    return FALSE;
}

/***********************************************************************
* EP_Set_04CPU_Init_End_Flag - 设置04板初始化完成标志
*
* RETURNS:
*         TRUE, 表示已初始化完成
*         FALSE, 表示初�?�化还未完成
*
*/
void EP_Set_04CPU_Init_End_Flag(
    BOOL bInitEndFlag			/* 初�?�化完成标志 */
)
{
    bInitEndFlag_g=bInitEndFlag;
}

/***********************************************************************
* EP_Get_04CPU_Init_End_Flag - 获得04板初始化完成标志
*
* RETURNS:
*         TRUE, 表示已初始化完成
*         FALSE, 表示初�?�化还未完成
*
*/
BOOL EP_Get_04CPU_Init_End_Flag()
{
    return bInitEndFlag_g;
}

/***********************************************************************
* SetAbnormalHintLampHdl - 获得装置异常�??句柄
*
* RETURNS: �??
*
*/
void SetAbnormalHintLampHdl()
{
    pvDevAbnormalHintLampHdl_g = RD_Get_Handle(DEV_ABNORMAL_HINT_LAMP_ID, RD_LGC_LED_HDL);

    if ((pvDevAbnormalHintLampHdl_g) == NULL)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,ER_REPORT | ER_ALARM | ER_LOCK,
                       "异常指示�??�??配置\n",
                       0, 0);
        }
        else if (ENG_MODE ==1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,ER_REPORT | ER_ALARM | ER_LOCK,
                       "Alarm Indicator not configed\n", 0, 0);
        }
    }
}

/***********************************************************************
* LightDevAbnormalHintLamp - Set the Equipment Abnormity Lamp
*
* RETURNS: �??
*
*/
void LightDevAbnormalHintLamp()
{
    static BOOL LightFlag = FALSE; /* 与应用程序配�?? */
    RD_LGC_LED_CH *plgcled;

#ifdef EDP01_CA_OPT_BUILD
    /* && (!EP_IN_WDG_RESET()) If reset happen in watchdog task,
     * the alarm wil be set quickly.
     * Change it when modifying in platform EDP01_C-A for photoelectron current transition.
     */
    if (((uiEdpStatus_g & (REBOOT_DLY | SYS_LOCK_DO | LOCK_EVT | LOCK_DO | SYS_LOCK_EVT
                           | SET_ALARM_FLAG | VERSION_NOT_MATCHED_FLAG | SCI_CHANGED_FLAG|DIGITAL_SAMPLE_ERR_FLAG))
            && (!EP_IN_HW_TEST())) || (LOG_GetBootFlag()))
#else
    /*  If reset happen in watchdog task, the alarm wil not be set quickly. */
    if (((uiEdpStatus_g & (REBOOT_DLY | SYS_LOCK_DO | LOCK_EVT | LOCK_DO | SYS_LOCK_EVT
                           | SET_ALARM_FLAG | VERSION_NOT_MATCHED_FLAG | SCI_CHANGED_FLAG|DIGITAL_SAMPLE_ERR_FLAG))
            && (!EP_IN_HW_TEST()) && (!EP_IN_WDG_RESET())) || (LOG_GetBootFlag()))
#endif
    {
        /* If the watchdog reset the system or the system is in hardware test mode,
         * do not set the alarm node.
         * LOG_Dbg_Msg("SIO_Enable_Alm Enter!\n", 0, 0, 0, 0, 0, 0);
         */

        SIO_Enable_Alm(); 		/* Set the alarm node. */
        bEnableAlarm_g = TRUE;

        if (LOG_GetBootFlag())
        {
            /* If the system was reseted frequently, the alarm node will be set. */
            static BOOL bWrFlag = TRUE;

            if (bWrFlag)
            {
                /* Only hint the first time */

                bWrFlag = FALSE;
#ifndef EDP01_CA_OPT_BUILD    /* When the reset occur frequently, alarm will be set quickly. */

                if (ENG_MODE == 1)
                {
                    /* need to report the abnormity and send to MMI to alarm and report. */
                    ER_Set_Err(EV_POWER_ON, ER_REPORT|ER_ALARM|ER_LOCK,
                               "CPU reboot frequently\n", 0, 0);
                }
                else if (ENG_MODE == 0)
                {
                    /* need to report the abnormity and send to MMI to alarm and report. */
                    ER_Set_Err(EV_POWER_ON, ER_REPORT|ER_ALARM|ER_LOCK,
                               "CPU频繁复位.\n", 0, 0);
                }
#endif
            }
        }
    }
    else
    {
        /* If in hardware test mode, the bEnableAlarm_g cann't be clear. */
        if (bEnableAlarm_g
                && (!EP_IN_HW_TEST())
                && !(uiEdpStatus_g & (REBOOT_DLY | SYS_LOCK_DO | LOCK_EVT | LOCK_DO | SYS_LOCK_EVT
                                      | SET_ALARM_FLAG | VERSION_NOT_MATCHED_FLAG | SCI_CHANGED_FLAG)))
        {
            /* If the alarm flag was set, then cancel the flag here, otherwise not. */
            SIO_Disable_Alm();
            bEnableAlarm_g = FALSE;
        }
    }

    /* 信号�??控制 */
    if (pvDevAbnormalHintLampHdl_g != NULL)
    {
        plgcled = (RD_LGC_LED_CH *) pvDevAbnormalHintLampHdl_g;
#ifdef EXCITE_BUILD
        if ((uiEdpStatus_g & (REBOOT_DLY | SYS_LOCK_DO|LOCK_EVT|LOCK_DO|SYS_LOCK_EVT))
                && (!EP_IN_HW_TEST()) && (!EP_IN_WDG_RESET()))
#else
        /* The alarm lamp is the same to the abnormility lamp in stability control equipment. */
        if((uiEdpStatus_g & (REBOOT_DLY | SYS_LOCK_DO | LOCK_EVT | LOCK_DO | SYS_LOCK_EVT
                             | SET_ALARM_FLAG | VERSION_NOT_MATCHED_FLAG | SCI_CHANGED_FLAG|DIGITAL_SAMPLE_ERR_FLAG))
                && (!EP_IN_HW_TEST()) && (!EP_IN_WDG_RESET()))
#endif
        {
            /* If the watchdog reset the system or the system is in hardware test mode, do not set the alarm node. */

            RD_Set_LED(pvDevAbnormalHintLampHdl_g, TRUE);
            LightFlag = TRUE;
        }
        else if (LOG_GetBootFlag())
        {
            /* If the system was reseted frequently, the alarm lamp will be set. */
            RD_Set_LED(pvDevAbnormalHintLampHdl_g, TRUE);
            LightFlag = TRUE;
        }
        else
        {
#ifdef EXCITE_BUILD
            if ((plgcled->bSts) && (LightFlag))	/* �??熄灭�??己点的灯 */
#endif
            {
                RD_Set_LED(pvDevAbnormalHintLampHdl_g, FALSE);
                LightFlag = FALSE;
            }
        }
    }
}

/***********************************************************************
* GetMonitorTaskStatus - 获取监�?�任务状�??
*
* RETURNS: TRUE, 正常; FALSE, 返回.
*
*/
BOOL GetMonitorTaskStatus()
{
    /* 最大监视延迟时间为20分钟, 便于初�?�化出错�??, 进�?��?�理 */
#define MAX_DELAY_SPY_TIME 1200000000UL

    static BOOL bBegainSpy = FALSE;
    static BOOL bFirstEnter = FALSE;
    static uint32_t ulFirstTime;
    static char strTaskStatus[128];

    if (!bBegainSpy && !bFirstEnter)
    {
        ulFirstTime = TM_Get_usCnt();
        bFirstEnter = TRUE;

        return TRUE;
    }

    if (!bBegainSpy && bFirstEnter)
    {
        if (TM_Get_usCnt()-ulFirstTime>MAX_DELAY_SPY_TIME)
        {
            bBegainSpy = TRUE;
        }

        return TRUE;
    }

    /* 若未创建初�?�化任务, 比�?�从网络�??�??, 则返回�?�常 */
    if (MonitorInitTaskID_g<0)
    {
        return TRUE;
    }

    /* 若初始化�??成功, 则返回�?�常 */
    if (!(EP_Get_04CPU_Init_End_Flag()))
    {
        return TRUE;
    }

    if (taskIdVerify(MonitorInitTaskID_g) == ERROR)
    {
        /* 首先判定该任务是否有�?? */
        return FALSE;
    }

    taskStatusString(MonitorInitTaskID_g, strTaskStatus);
    if (strcmp(strTaskStatus,"SUSPEND") == 0
            ||strcmp(strTaskStatus,"DELAY+S") == 0
            ||strcmp(strTaskStatus,"PEND+S") == 0
            ||strcmp(strTaskStatus,"PEND+S+T") == 0
            ||strcmp(strTaskStatus,"SUSPEND+I") == 0
            ||strcmp(strTaskStatus,"DELAY+S+I") == 0
            ||strcmp(strTaskStatus,"PEND+S+I") == 0
            ||strcmp(strTaskStatus,"PEND+S+T+I") == 0
            ||strcmp(strTaskStatus,"DEAD") == 0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/***********************************************************************
* DisablePit - 关闭Pit
*
* RETURNS: �??
*
*/
void DisablePit(void)
{
    // kickSwDog();
    // kickHwDog();
    // disablePitKickDog(); 	/* 关闭PIT打狗 */
    /* 击狗 */
    // kickSwDog();
    // kickHwDog();
}

/***********************************************************************
* MessageBoxRegister - 注册显示函数
*
* RETURNS: �??
*
*/
void MessageBoxRegister(void *fun)
{
    pMessageBox = (pFV)fun;
}

/***********************************************************************
* GetTaskStatus - 获得相应任务的状�??
*
* RETURNS: TRUE, or FALSE
*
*/
static BOOL GetTaskStatus(
    int TaskID,
    char *pTaskStatus
)
{
    char *strTaskStatus;

    if (pTaskStatus != NULL)
        strTaskStatus = pTaskStatus;
    else
        return TRUE;

    if (taskIdVerify(TaskID) == ERROR)
    {
        /*首先判定该任务是否有�??  */
        sprintf(strTaskStatus, "%s", "任务无效或不存在");

        return FALSE;
    }

    taskStatusString(TaskID, strTaskStatus);

    if (strcmp(strTaskStatus,"SUSPEND") == 0
            ||strcmp(strTaskStatus,"DELAY+S") == 0
            ||strcmp(strTaskStatus,"PEND+S") == 0
            ||strcmp(strTaskStatus,"PEND+S+T") == 0
            ||strcmp(strTaskStatus,"SUSPEND+I") == 0
            ||strcmp(strTaskStatus,"DELAY+S+I") == 0
            ||strcmp(strTaskStatus,"PEND+S+I") == 0
            ||strcmp(strTaskStatus,"PEND+S+T+I") == 0
            ||strcmp(strTaskStatus,"DEAD") == 0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/***********************************************************************
* runExternBsp - �??否执行externBSP
*
* RETURNS:
*         TRUE: 执�??
*         FALSE: 不执�??
*/
BOOL runExternBsp()
{
    return TRUE;
}

/***********************************************************************
* runExternMain - �??否执行externMain
*
* RETURNS:
*         TRUE: 执�??
*         FALSE: 不执�??
*/
// BOOL runExternMain()
// {
//     if(IS_Boot_From_Net())
//         return FALSE;		/* 如果处于调试态，则不调用externMain */
//     else
//         return TRUE;		/* 否则调用 */

// }

/***********************************************************************
* initWDB - �??否初始化WDB
*
* RETURNS:
*         TRUE: 执�??
*         FALSE: 不执�??
*/
BOOL initWDB()
{
    if (IS_Boot_From_Net())		/* 如果处于调试态，则初始化WDB */
        return TRUE;
    else
        return TRUE;	/* 否则不初始化WDB */	/* 不初始化WDB对C++代码有影�?? */
}

/***********************************************************************
* EDPreboot - 平台重启
*
* RETURNS: �??
*
*/
void EDPreboot
(
    int startType             /* how the boot ROMS will reboot */
)
{
    Exc_WrRebootInfo(startType);
#ifdef EDP03_BUILD
    IO_GJFGSLDisable();		/* 禁�?�告�?? */
#endif

    sysToMonitor(0);
}

/***********************************************************************
* HardChannelTestCheck - mmi使用临时函数
*
* RETURNS: �??
*
*/
BOOL HardChannelTestCheck(
    int commandStyle,
    int channelCode,
    int standardStyle
)
{
    return TRUE;
}

/***********************************************************************
* PowerZero - mmi使用临时函数
*
* RETURNS: �??
*
*/
BOOL PowerZero()
{
    return TRUE;
}

/***********************************************************************
* MemWr - 内存�??
*
* RETURNS: �??
*
* 注意: 调用此函数�?�小�??, 防�?�破坏内�??
*
*/
static void MemWr(
    uint32_t MemAddr, /* 写入地址 */
    uint32_t WrData				/* 写入数据 */
)
{
    *((uint32_t *)MemAddr)=WrData;
}

/* 查�?�当前是否�?�于检�??�??.
 * Para:
 *     pbRtRepairSts, 返回检�??状态，若返回值为�??0值，则为检�??态，若为0，则为运行�?.
 * Return:
 *     �??0�??: 查�?�操作成�??; 0: 查�?�操作失�??.
 *
 * 注意:
 *     因为61850的BOOL和vxowrks都定义了BOOL，有冲突, 用地址来返回BOOL变量有问题，
 *     所以用固定长度的变量来返回
 *
 */
unsigned char EP_Get_Repair_Sts(
    unsigned char *pbRtRepairSts
)
{

    if (uiEdpStatus_g & ON_EXAM_STATE)
    {
        *pbRtRepairSts=1;
    }
    else
    {
        *pbRtRepairSts=0;
    }

    if (!bHasChgRepairSts_g)
    {
        return  1;
    }
    else
    {
        return   0;
    }

}

/***********************************************************************
* EP_Set_Repair_Sts - 设置检�??状�?.
*
* RETURNS: None
*
*/
void EP_Set_Repair_Sts()
{
    if (!bHasChgRepairSts_g)
    {
        bHasChgRepairSts_g = TRUE;
    }
    if (!(uiEdpStatus_g & ON_EXAM_STATE))
    {

        if (ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "switched to repair mode.\n", NULL);
        }
        else if (ENG_MODE == 0)
        {

            LOG_Write(LOG_KERNEL, "切换到�?��?模式\n", NULL);

        }
        EP_Set_Sts_Bit(ON_EXAM_STATE);
    }
}

/***********************************************************************
* EP_Clear_Repair_Sts - 清除检�??状�?.
*
* RETURNS: None
*
*/
void EP_Clear_Repair_Sts(void)
{
    if (!bHasChgRepairSts_g)
    {
        bHasChgRepairSts_g = TRUE;
    }
    if (uiEdpStatus_g & ON_EXAM_STATE)
    {
        if (ENG_MODE == 1)              /*2007-4-19�?? 张云�??改，为了�??持英文版  */
        {
            LOG_Write(LOG_KERNEL, "clear repair mode\n", NULL);
        }
        else if (ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "清除检�??模式\n", NULL);
        }
        EP_Clr_Sts_Bit(ON_EXAM_STATE);
    }
}

#if defined(EDP_01_02_BUILD) || defined(EDP03_BUILD)
/***********************************************************************
* EP_LogBootReason - Record the boot resean into the LOG, must be called after the LOG initialization.
*
* RETURNS: None
*
*/
static EP_STATUS EP_LogBootReason(void)
{
    int iBootReason;			/* Boot resean */
    int iBspErrSts;

    // iBootReason = boot_reason();
    iBootReason = BOOT_COLDRESET;
    iBootReason_g = iBootReason;
    switch (iBootReason)
    {
        case BOOT_COLDRESET:
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "装置上电.\n", &EP_dtRebootTime_g);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "The 04 board reboot because of power on again.\n",
                          &EP_dtRebootTime_g);
            }
            break;

        case BOOT_SW:
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "�??狗�?�位.\n", &EP_dtRebootTime_g);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "The 04 Board restart because of software watchdog reset.\n",
                          &EP_dtRebootTime_g);
            }
            break;

        case BOOT_HW:
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "�??狗�?�位.\n", &EP_dtRebootTime_g);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "The 04 Board restart because of hardware watchdog reset.\n",
                          &EP_dtRebootTime_g);
            }
            break;

        case BOOT_REBOOT:
            Exc_RdRebootInfo();	/* read reboot information. */
            break;

        case BOOT_JTRS:
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "JTAG复位.\n", &EP_dtRebootTime_g);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "The 04 Board restart because of JTAG reset.\n",
                          &EP_dtRebootTime_g);
            }
            break;

        case BOOT_CSRS:
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "因CHECK STOP异常,导致04板重�??.\n", &EP_dtRebootTime_g);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "The 04 Board restart because of CHECK STOP exception.\n",
                          &EP_dtRebootTime_g);

            }
            break;

        case BOOT_BMRS:
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "因总线监�?��?�位,导致04板重�??.\n", &EP_dtRebootTime_g);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "The 04 Board restart because of bus monitor reset.\n",
                          &EP_dtRebootTime_g);
            }
            break;

        default:
            if (ENG_MODE == 0)
            {
                char strLog[50]="";
                sprintf(strLog,"因其他未知原�??%d,导致04板重�??.\n",iBootReason);
                LOG_Write(LOG_KERNEL,strLog, &EP_dtRebootTime_g);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "The 04 Board restart because of unknown resean.\n",
                          &EP_dtRebootTime_g);
            }
            break;
    }

    iBspErrSts = sysProcNumGet();

    if (iBspErrSts > 0)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT | ER_ALARM | ER_LOCK,
                       "CPU模件配置错�??(0x%x)\n", iBspErrSts, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT | ER_ALARM | ER_LOCK,
                       "CPU module config error(0x%x)\n", iBspErrSts, 0);
        }
    }

    return EP_SUCCESS;
}
#endif

/***********************************************************************
* EP_Clear_Alarm - 清除掉告警信号，复归时调�??.
*
* RETURNS: None
*
*/
void EP_Clear_Alarm(void)
{
    if(bEnableAlarm_g&&!(uiEdpStatus_g & (REBOOT_DLY | SYS_LOCK_DO|LOCK_EVT|LOCK_DO|SYS_LOCK_EVT |SET_ALARM_FLAG)))
    {
        /* 若原来�?�置了告警标�??,则撤消告警标�??,否则不用撤消 */
        SIO_Disable_Alm();
        bEnableAlarm_g = FALSE;
        if (pvDevAbnormalHintLampHdl_g != NULL)
        {
            RD_Set_LED(pvDevAbnormalHintLampHdl_g,FALSE);
        }
    }
}

/***********************************************************************
* EP_Is_Lock_DO - 获得当前�??否闭锁保护标�??
*
* RETURNS:
*               若已经�?�置保护�??锁，返回�??
　　　否则，返回假
*
*/
BOOL EP_Is_Lock_DO(void)
{
    if ((uiEdpStatus_g & (SYS_LOCK_DO | LOCK_DO))
            || bEnableAlarm_g)
    {
        /* 若已经�?�置�??锁保护标�??(告�?�亦�??锁保�??)，则返回�?? */
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/* judge the method of booting.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, excute vxworks_rom automatically for debug;
 *     FALSE, excute according autoexec.ini for release.
 */
BOOL EP_IS_BOOT_SEL(void)
{
    if (uiIOFuncPinSts_g&IO_PIN_BOOT_SEL)
        return TRUE;
    else
        return FALSE;
}

/* judge the rate of booting.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, fast; FALSE, slow.
 */
BOOL EP_IS_FAST_BOOT(void)
{
    if (uiIOFuncPinSts_g&IO_PIN_FAST_BOOT)
        return TRUE;
    else
        return FALSE;
}

/***********************************************************************
* GetFarStFlag - 获取�??否�?�于远方态标�??
*
* RETURNS: TRUE: 远方�??, or FALSE: 就地�??
*
*/
BOOL GetFarStFlag(void)
{
    if (uiEdpStatus_g & ON_FAR_STATE)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/***********************************************************************
* GetExamStFlag - 获取�??否�?�于检�??态标�??
*
* RETURNS: TRUE: 检�??�??, or FALSE: 正常运�?��?
*
*/
BOOL GetExamStFlag(void)
{
    if (uiEdpStatus_g & ON_EXAM_STATE)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/***********************************************************************
* GetDealSampExamFlag - �??否启用数字化采样值�?��?不一致判�??
*
* RETURNS: TRUE: 判别检�??状�?, or FALSE: 不判�??检�??状�?
*
*/
BOOL GetDealSampExamFlag(void)
{
    if (appType_g == APP_TYPE_DIG)
    {
        /* 数字化应�?? */
        return TRUE;
    }
    else if (appType_g == APP_TYPE_TRAD)
    {
        /* 传统应用 */
        return FALSE;
    }

    return FALSE;
}


/***********************************************************************
* EP_Is_Enable_Alarm - 获得装置当前�??否�?�置告�?�标�??
*
* RETURNS:
*      已经设置告�?�，返回�??
*　　　否则，返回假
*
*/
BOOL EP_Is_Enable_Alarm(void)
{
    if (bEnableAlarm_g)
    {
        /* 若已经�?�置告�?�，则返回真 */
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/* 显示CPU空闲状�?,由shell调用
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void showIdleStat(void)
{
    static uint8_t aucLine[INI_TAG_LEN];
    uint32_t qVal;
    uint32_t resVal;

    if (idleStat.maxCount == 0)
    {
        return;
    }

    idleStat.idlePercent
        = (idleStat.intCount*10*1000)/(idleStat.maxCount*(idleInterval*1000/idleIntervaOrin)/100);
    qVal = idleStat.idlePercent/10;
    resVal = idleStat.idlePercent%10;
    sprintf(aucLine, "%u.%u%%", (unsigned int)qVal, (unsigned int)resVal);
    LOG_Dbg_Msg("CPU CUR IDLE: %s\n", (int)aucLine, 0, 0, 0, 0, 0);
    printf("CPU CUR IDLE: %s\n", aucLine);

    idleStat.idlePercent
        = (idleStat.ullTotalCount*10*1000)/(idleStat.ulWdCnt*idleStat.maxCount*(idleInterval*1000/idleIntervaOrin)/100);
    qVal = idleStat.idlePercent/10;
    resVal = idleStat.idlePercent%10;
    sprintf(aucLine, "%u.%u%%(统�?�周�??%d)", (unsigned int)qVal, (unsigned int)resVal, (int)(idleStat.ulWdCnt));
    LOG_Dbg_Msg("CPU AVR IDLE: %s\n", (int)aucLine, 0, 0, 0, 0, 0);
    printf("CPU AVR IDLE: %s\n", aucLine);

    RE_ShowTaskComsumeTimeStat();

    /* 没有处理互斥,但仅用于调试 */
    idleStat.ulWdCnt = 0;
    idleStat.ullTotalCount = 0;

    return;
}

/* 得到CPU空闲比例,由浮点环境任务调�??
 * Para:
 *     NONE.
 * Return:
 *     idle percent.
 */
float getIdleStat(void)
{
    if (idleStat.maxCount == 0)
    {
        return 0.0;
    }

    return (float)(idleStat.intCount*100.0)/(float)idleStat.maxCount;
}

/* 调整统�?�间�??
 * Para:
 *     msInt, 间隔,ms.
 * Return:
 *     NONE.
 */
void setIdleStatInt(uint32_t msInt)
{
    uint32_t ulIntPeriod;

    ulIntPeriod = 1000/sysClkRateGet ();
    if (msInt<ulIntPeriod)
    {
        idleInterval = 1;
    }
    else
    {
        idleInterval = msInt/ulIntPeriod;
    }
}

/* 计算消耗时�??
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void CalcDif(void)
{
    int iLockKey;

    iLockKey = intLock();

    g_ulComsumeTm = g_ulEndTm-g_ulStartTm;
    printf("时间消�?%d\n", (int)g_ulComsumeTm);

    intUnlock(iLockKey);
}

#if 1
/* 1.65BSP没有提供以下函数, 添加, 如果BSP不修改则另�?�添�??.out文件 */
// int Set_Sys_Hw_Clock(UINT8 *buf)
// {
//     return ERROR;
// }

// short Get_Boot_Context()
// {
//     return -1;
// }

// int Get_Sys_Hw_Clock(UINT8 *buf)
// {
//     return ERROR;
// }

// int Get_Boot_Info()
// {
//     return -1;
// }

int ds3231Write(UINT8 data_ptr,UINT8 *buf,UINT8 length)
{
    return -1;
}

// void    Write_FPGA_Program()
// {
//     return;
// }

int ds3231Read(UINT8 data_ptr,UINT8 *buf,UINT8 length)
{
    return -1;
}
#endif

/* 调整tNetTask任务优先�?? */

void EP_AdjNetPri(void)
{
    int iNetTaskID;

    iNetTaskID = taskNameToId("tNetTask");
    taskPrioritySet(iNetTaskID, TSK_PRI_TERM_NET);
}
