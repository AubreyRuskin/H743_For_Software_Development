/* view.c - This file contains interface to EPView communication module */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02a, 30may07, dy change the code style.
01c, 29jul03, hdx Updated to version 1.0.
01b, 27may03 hdx Verified version 0.1.
01a, 15feb03, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains interface to EPView communication module.
INCLUDE: view.h
*/

/* includes */

#include "edpbase.h"	/* base header file. */
#include "view.h"
#include "errtest.h"
#include "logmsg.h"
#include "miscfunc.h"
#include "realdata.h"
#include "filetool.h"
#include "rec.h"
#include "sysinfo.h"
#include "string_compat.h"

#if defined(EDP_01_02_BUILD)
#include "spiio.h"
#elif defined(EDP03_BUILD) || defined(EXCITE_BUILD)
#include "io_Drv.h"
#endif

#include <stdio_compat.h>
#include "string_compat.h"
#include <taskLib.h>
#include <ioLib.h>
#include <dirent_compat.h>
#include "FileSynPro.h"
#include <tickLib.h>
#include "bspinterface.h"
#include "intLib.h"

#ifdef EDP02_PSR_BUILD
#include "comm_interface.h"
#include "EdpNetCfg.h"
#endif
/* defines */

#define ANG_VALID_COFF      0.0005      /* Angle valid module to fOvMax. */
#define MIN_REF_MOD_COFF    0.002       /* Refrence channel module to fOvMax. */
#define MAX_EVT_FOLODR_FILE_NUM (MAX_ALLOW_RPT_NUM+100)  		/* 事件文件夹中允许的最大文件个数 */
#define MAX_CREATE_RELAYFUNC_TASK_COUNT 8  		/* 该宏已经在RE_PublicDataDef.h文件中定义，为防止链表相关定义重复，没有包含该头文件，但两个定义应保持一致 */

/* 内部通讯规约中SOE主动上送的品质位定义 */
#define VI_SOE_INVALID_STS  0x08        /* 无效标志位 */
#define VI_SOE_OLD_DATA_STS  0x10       /* 老数据标志位 */
#define VI_SOE_CHANGE_STS  0x20         /* 变位标志位 */
#define VI_SOE_EVENT_STS  0x40          /* 遥信变位触发SOE事件标志位 */
#define VI_SOE_REPAIR_STS 0x80          /* 检修标志位 */

/* typedefs */
#ifndef EDP02_PSR_BUILD
typedef struct
{
    EP_ELEM_IO *pelmSrc;
    void *pvAiHnd;
    uint32_t ulScanTaskNo;
    BOOL bIsRiCplx;
    uint8_t ucArith;
    COMPLEX xVal;
    float fVal;
    float fAngVldMod;                   /* Only used when ucArith==2. */
} VI_MEA_AI_DB;

typedef struct
{
    EP_ELEM_IO *pelmSrc;
    void *pvDiHnd;
    BOOL bVal;
    BOOL bSOE;
    uint16_t usQuality; /* 遥信品质位 */
} VI_MEA_DI_DB;

typedef   struct
{
    uint16_t uiCurRptNum;
    BOOL bCurRptIsFastDelFlg;		    /* 当前事件报告是快速删除事件报告标志,报告中只要存在一个慢速事件,则就是慢速事件报告 */
    uint8_t ucFstEvtType;  				/* 第1个事件类型 */
    uint16_t uiFstEvtCode;   		/* 第1个事件的区分码 */
    EP_DATE_TIME  tmRptFaultTime;		/* 报告故障发生的时间 */
}   EVT_RPT_NAME_INFO_TYPE;			/* 事件报告名的相关信息 */

typedef struct
{
    enum
    {
        SHORT_PULSE_ACT,
        LONG_PULSE_ACT,
        MEA_DO_RETURN,
        CUSTOM_PULSE_ACT,
    } type;
    uint32_t iMeaDoPointerNum;   	/* 遥控点号 */
    uint8_t ucCmdType;			/* Type of order. */
    BOOL bVal;   		/* 来遥控信号 */
    BOOL bAppSetFlag;			/* 逻辑图设定标志 */
    uint32_t uiPulseTm;   		/* 当类型为CUSTOM_PULSE_ACT自定义脉宽时的脉宽时间 */
    uint32_t ulTqPara;
} VI_MEA_DO_DB;

typedef struct
{
    BOOL bVal;  		/* 来了电度清零信号 */
    BOOL bAdjustRunFlag;				/* 校准正在进行标志 */
    uint32_t uAdjustStartTickNum;
    uint8_t ucObjNum;  		/* 脉冲电度输出通道号0xff表示所有的通道 */
} VI_POCLEAR_INFO;

typedef struct		/* 铁电相关命令处理 */
{
    BOOL bVal;  		/* 命令是否到来 */
    BOOL bAdjustRunFlag;				/* 校准正在切换标志 */
    uint32_t uAdjustStartTickNum;
    uint8_t ucCmdType;  		/* 命令类型 */
} VI_TDSWITCH_INFO;

typedef struct		/* 外部命令图元 */
{
    BOOL bVal;  		/* 外部命令到来 */
    uint32_t ulOrderType;			/* 命令类型 */
} VI_OUTORDER_INFO;
#endif
/* locals */

static VI_MEA_AI_CFG *pmaicfg_g;
static VI_MEA_AI_DB *pmaidb_g;
static int iRefAngCh_g=-1;					/* -1=no relative angle channel; -2=need search for ref-channel. */
static float fMinRefMod_g;
static SEM_ID semMeaAi;

static VI_MEA_DI_CFG *pmdicfg_g;
static VI_MEA_DI_DB *pmdidb_g;
static VI_MEA_DI_DB **appmdidbTsk_g[MAX_SUB_LGC_NUM];

static VI_MEA_DO_CFG *pmdocfg_g;
static VI_MEA_DO_DB * pmdodb_g;
static VI_EVT_CFG *pevtcfg_g;

static VI_POCLEAR_INFO poclearinfo_g;
VI_TDSWITCH_INFO TdSwitch_g;
VI_OUTORDER_INFO FarSts_g;		/* 远方就地状态 */
VI_OUTORDER_INFO RepairSts_g;			/* 检修状态 */
VI_OUTORDER_INFO JgsSts_g;			/* 解挂锁状态 */

static EVT_RPT_NAME_INFO_TYPE CurEvtRptInfo_g;
static BOOL bCurRptHaveGetFirstEvtFlg_g;   		/* 获得当前报告的第1个事件的标志 */
static uint16_t ulCurEvtFileRptNum_g=0XFFFF;				/* 当前事件报告号 */
static BOOL bNotUseRTFdbkFlag = FALSE;
static uint16_t SlowDelSN[MAX_SLOW_DEL_EVT_FILE];
static uint8_t ucNewestPos;

/* globals */

VI_RPT_STS rptsts_g;
int iMeaAiNum_g;
int iMeaDiNum_g;
int iMeaDoNum_g;
int iEvtNum_g;
VI_RUN_INFO arinf_g[SZ_RUN_INFO_BUF];
u_int uiCurInfIdx_g;
u_int uiCurInfPos_g;
SEM_ID semNewInfCom_g;
SEM_ID semNewInfRpt_g;
#ifdef GXC01U
SEM_ID semNewInfGxc_g;  /*给GXC01U装置专用访问事件*/
#endif
SEM_ID semWrReportSN;	/* write the newest report serial number. */
VI_ADJUST_INFO adjoverinfo_g;
VI_ADJUST_INFO adjinfo_g;

extern BOOL bEvtDirExistFlag_g;		/* 事件目录创建成功标志 */
/*如果故障开始时间是在正闰秒之后, 则该标志置TRUE，
后续根据该标志，将其余事件的时间也置成该标志
为什么不根据每次事件时间来判断，是因为
如果是开始时间在正闰秒之后，则第一次转换时时间已经-1，就是实际时间
而在转换函数内的LS来临的TIME是1秒的时刻，所以正闰秒0秒的转换会出问题*/
BOOL g_bErrBgnAfterLs = FALSE;

int nEvtMakeRptTaskID_g;		/* 为了监测事件报告任务的正常与否，而设置的全局变量任务ID号 */
BOOL bEvtMakeRtpTaskStartFlag_g=FALSE;

BOOL bViewModIsInit_g=FALSE;		/* 2006-8-2  事件模块初始化初始化成功标志，用于事件报告的生成判别 */
uint32_t   ulInfNonComleteCnt_g=0;      /*当前未完成写入消息队列的运行信息计数　2010-4-26 ZY  */

/* local functions */

/***********************************************************************
* VI_Search_Ref_Ch - 搜索参考通道(用于遥测)
*
* RETURNS: 无
*
*/
static void VI_Search_Ref_Ch(void);

/***********************************************************************
* VI_Init_Rpt_SN - 获得最小的报告号
*
* RETURNS: 报告号
*
*/
static uint16_t VI_Init_Rpt_SN(void);

/***********************************************************************
* VI_Rpt_Added - 增加报告
*
* RETURNS: 无
*
*/
static void VI_Rpt_Added(void);

/***********************************************************************
* VI_Make_Rpt - 报告写入任务入口函数
*
* RETURNS: 无
*
*/
static int VI_Make_Rpt(
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
* VI_Bgn_Rpt - 开始报告写入
*
* RETURNS: 无
*
*/
static int VI_Bgn_Rpt(
    uint16_t unRptSN,
    const EP_DATE_TIME *pdttm
);

/***********************************************************************
* VI_End_Rpt - 结束报告写入
*
* RETURNS: 无
*
*/
static void VI_End_Rpt(
    int iFd,
    uint32_t ulEvtMsgLen,
    u_int uiEvtNum
);

/***********************************************************************
* VI_Wr_Evt_Rpt - 写事件报告
*
* RETURNS: 报告号
*
*/
static int VI_Wr_Evt_Rpt(
    int iFd,
    const VI_EVT_MSG *pevt,
    EP_DATE_TIME *pdttm
);

/***********************************************************************
* VI_Wr_Soe_Rpt - 写SOE报告
*
* RETURNS: 写入字节数
*
*/
static int VI_Wr_Soe_Rpt(
    int iFd,
    const VI_SOE_MSG *psoe,
    EP_DATE_TIME *pdttm
);

/***********************************************************************
* VI_MakeEvtFileTaskExecHandle - 为了处理事件报告文件异常时的处理函数，张云添加
*
* RETURNS: 无
*
*/
static void VI_MakeEvtFileTaskExecHandle(
    int iFd,		/* 文件句柄 */
    int iExecReason		/* 异常原因 */
);

/***********************************************************************
* VI_Wr_Err_Rpt - 写异常事件报告
*
* RETURNS: 报告号
*
*/
static int VI_Wr_Err_Rpt(
    int iFd,
    const VI_ERR_MSG *perr,
    EP_DATE_TIME *pdttm
);

/***********************************************************************
* VI_Wr_Err_Stat_Rpt - 写异常事件报告(带状态)
*
* RETURNS: 报告号
*
*/
static int VI_Wr_Err_Stat_Rpt(
    int iFd,
    const VI_ERR_STAT_MSG *perr,
    EP_DATE_TIME *pdttm
);

/* global functions */

/***********************************************************************
* GetAdjustTimeSuccessFlag - MMI_SOFT.C提供的对时成功标志,
*
* RETURNS:
*					TRUE: 04板初始化后对时成功
*					FALSE: 04板初始化后还没有对时成功
*
*/
extern BOOL GetAdjustTimeSuccessFlag();

/* initialize the event delete attribution array.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
static void VI_InitDelAttr(void);

/* write event file deleting attribution.
 * Para:
 *     usCurSN, current SN.
 * Return:
 *     NONE.
 */
static void VI_WrEventDelAttr(uint16_t usCurSN);

/* functions */

/***********************************************************************
* VI_Init_Sem - 初始化信号量
*
* RETURNS: 无
*
*/
void VI_Init_Sem(void)
{
    semNewInfCom_g = semCCreate(SEM_Q_FIFO, 0);
    assert (semNewInfCom_g != NULL);

    semNewInfRpt_g = semCCreate(SEM_Q_FIFO, 0);
    assert (semNewInfRpt_g != NULL);

#ifdef GXC01U
    semNewInfGxc_g = semCCreate(SEM_Q_FIFO, 0);
    assert (semNewInfGxc_g != NULL);
#endif

    semWrReportSN = semMCreate(SEM_Q_PRIORITY  | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);

    /* The events created before the event module intialization don't be processed. */
    rptsts_g.bRptSNProcessFinish = FALSE;
}

/***********************************************************************
* VI_Initialize - 事件报告初始化
*
* RETURNS: TRUE, or FALSE
*
*/
EP_STATUS VI_Initialize(void)
{
    STATUS vxsts;
#if defined(EDP_01_02_BUILD)
    int iNandFlashSize;
#endif
    uint16_t unRptSN;

    rptsts_g.iMaxRptNum = MAX_EVT_FILE_NUM;

#if defined(EDP_01_02_BUILD)
    iNandFlashSize = Ffx_Get_Nand_Size_In_MegaByte();
    LOG_Dbg_Msg("可用NandFlash大小为%d M.\n", iNandFlashSize, 0, 0, 0, 0, 0);

    /* 此数字不能太大，最好不超过500，否则文件删除操作速度太慢，对内存要求也太多 */

    if (iNandFlashSize>100)
    {
        rptsts_g.iMaxRptNum = MAX_EVT_FILE_NUM;
    }
    else if (iNandFlashSize>50)
    {
        rptsts_g.iMaxRptNum = MAX_EVT_FILE_NUM;
    }
    else if (iNandFlashSize>10)
    {
        rptsts_g.iMaxRptNum = MAX_EVT_FILE_NUM;
    }
    else
    {
        rptsts_g.iMaxRptNum = 100;
    }
#endif

    if (!bViewModIsInit_g)	/* VI_InitSn函数没有读取到最新报告号 */
    {
        unRptSN = VI_Init_Rpt_SN();	 /* 通过文件列表获得最新报告号 */
        rptsts_g.uStNewestSN = unRptSN;
        rptsts_g.unRptSN = unRptSN+1;
        rptsts_g.unRptSNReserved = rptsts_g.unRptSN;  /* Reserved SN. */
        rptsts_g.unRptSN += RPTSNRESERVED;	 /* reserved automatically. */
        bViewModIsInit_g = TRUE;
    }

    VI_InitDelAttr();

    if (!bEvtDirExistFlag_g)
    {
        /* 若事件目录不存在, 则不创建事件文件任务 */
        return EP_NOT_INIT;
    }

    nEvtMakeRptTaskID_g = taskSpawn("tMakeRpt", TSK_PRI_RPT, VX_FP_TASK|VX_DEALLOC_STACK, 30000, VI_Make_Rpt,
                                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    assert (nEvtMakeRptTaskID_g != ERROR);
    bEvtMakeRtpTaskStartFlag_g = TRUE;

    vxsts = taskLock();
    assert (vxsts == OK);

    rptsts_g.ulRptNumBeforeEventInit = uiCurInfIdx_g;  /* The current number of events. */

    vxsts = taskUnlock();
    assert (vxsts == OK);

    return EP_SUCCESS;
}

/* Initialize the serial number.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUSSESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VI_InitSn(void)
{
    uint16_t unRptSN;

    if (EP_GetNewestSN(&unRptSN) == EP_ERROR)
    {
        bViewModIsInit_g = FALSE;

        return EP_ERROR;
    }
    else
    {
        rptsts_g.uStNewestSN = unRptSN;
        rptsts_g.unRptSN = unRptSN+1;		/* the SN for new report. */
        rptsts_g.bRptSNProcessFinish = TRUE;   /* 不必处理获取最新报告号前事件 */
        bViewModIsInit_g=TRUE;

        return EP_SUCCESS;
    }
}

/***********************************************************************
* VI_Cfg_Event - 事件配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_Cfg_Event(
    uint8_t *pucCfg,
    uint32_t ulLen
)
{
    uint8_t *puc;
    VI_EVT_CFG *pevtcfg;
    VI_EVT_PARM_CFG *pparmcfg;
    int iItemCfgLen;

    puc=pucCfg;

    iEvtNum_g=U8_TO_U16(puc[1], puc[0]);
    puc+=6;

    if ((pevtcfg_g=calloc(iEvtNum_g, sizeof(*pevtcfg_g)))==NULL)
        return EP_BUF_ERR;

    for (pevtcfg=pevtcfg_g; pevtcfg<pevtcfg_g+iEvtNum_g; pevtcfg++)
    {
        iItemCfgLen=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pevtcfg->aucId, puc+1, puc[0]);
        puc+=1+puc[0];

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pevtcfg->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        pevtcfg->aucABRV[0]=*puc++;
        pevtcfg->aucABRV[1]=*puc++;
        pevtcfg->aucABRV[2]=*puc++;
        pevtcfg->aucABRV[3]=*puc++;

        pevtcfg->bKeep=*puc++?TRUE:FALSE;

        pevtcfg->bHaveSts=*puc++?TRUE:FALSE;

        pevtcfg->bNeedFastDel=((*puc)&0x01)?TRUE:FALSE ;
        pevtcfg->bNotNeedUpSend=((*puc)&0x02)?TRUE:FALSE ;
        pevtcfg->bMmiNotDisRtn=((*puc)&0x04)?TRUE:FALSE ;
        pevtcfg->bGWReport=((*puc)&0x08)?TRUE:FALSE ;
        pevtcfg->bGWXinXiReport=((*puc)&0x10)?TRUE:FALSE ;

        puc++;
        pevtcfg->ucMMIDlgPopAttr=*puc;
        puc++;

        pevtcfg->unCode=U8_TO_U16(puc[1], puc[0]);
        puc+=2;
        assert(pevtcfg->unCode>=0x40);

        pevtcfg->ucType=*puc++;

        pevtcfg->pAlertHdl=NULL;		/* 保护启动和动作事件，不注册呼唤 */

        pevtcfg->ucParmNum=*puc++;
        assert(pevtcfg->ucParmNum<=MAX_EVT_PARM_NUM);

        for (pparmcfg=pevtcfg->aparmcfg;
                pparmcfg<pevtcfg->aparmcfg+pevtcfg->ucParmNum; pparmcfg++)
        {
            iItemCfgLen-=6+puc[0];
            EP_ID_Copy(pparmcfg->aucName, puc+1, puc[0]);
            puc+=1+puc[0];

            puc+=4;

            pparmcfg->ucAttrib=*puc++;
        }

        assert(iItemCfgLen==14);
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
        return EP_SUCCESS;
}

/***********************************************************************
* VI_Cfg_Alarm - 告警配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_Cfg_Alarm(
    uint8_t *pucCfg,
    uint32_t ulLen
)
{
    uint8_t *puc;
    VI_EVT_CFG *pevtcfg;
    VI_EVT_PARM_CFG *pparmcfg;
    int iItemCfgLen;
    int iAlarmNum;

    puc=pucCfg;

    iAlarmNum=U8_TO_U16(puc[1], puc[0]);
    if (!iAlarmNum)
        return EP_SUCCESS;

    puc+=6;

    if ((pevtcfg_g=realloc(pevtcfg_g,
                           (iEvtNum_g+iAlarmNum)*sizeof(*pevtcfg_g)))==NULL)
        return EP_BUF_ERR;

    memset(pevtcfg_g+iEvtNum_g, 0, iAlarmNum*sizeof(*pevtcfg_g));

    for (pevtcfg=pevtcfg_g+iEvtNum_g;
            pevtcfg<pevtcfg_g+iEvtNum_g+iAlarmNum; pevtcfg++)
    {
        iItemCfgLen=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pevtcfg->aucId, puc+1, puc[0]);
        puc+=1+puc[0];

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pevtcfg->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        pevtcfg->aucABRV[0]=*puc++;
        pevtcfg->aucABRV[1]=*puc++;
        pevtcfg->aucABRV[2]=*puc++;
        pevtcfg->aucABRV[3]=*puc++;

        pevtcfg->bKeep=*puc++?TRUE:FALSE;

        pevtcfg->bHaveSts=*puc++?TRUE:FALSE;

        pevtcfg->bNeedFastDel=((*puc)&0x01)?TRUE:FALSE ;
        pevtcfg->bNotNeedUpSend=((*puc)&0x02)?TRUE:FALSE ;
        pevtcfg->bMmiNotDisRtn=((*puc)&0x04)?TRUE:FALSE ;
        pevtcfg->bGWReport=((*puc)&0x08)?TRUE:FALSE ;
        pevtcfg->bGWXinXiReport=((*puc)&0x10)?TRUE:FALSE ;

        puc++;
        pevtcfg->ucMMIDlgPopAttr=*puc;
        puc++;

        pevtcfg->unCode=U8_TO_U16(puc[1], puc[0]);
        puc+=2;
        assert(pevtcfg->unCode>=0x40);

        pevtcfg->ucType=*puc++;

        if((pevtcfg->ucType == 4)
                /* ||(pevtcfg->ucType == 8) */ )
        {
            /* 若是保护告警和呼唤，则注册呼唤 */
            pevtcfg->pAlertHdl=(void *)ER_RegAlertSignal();
        }

        pevtcfg->ucParmNum=*puc++;
        assert(pevtcfg->ucParmNum<=MAX_EVT_PARM_NUM);

        for (pparmcfg=pevtcfg->aparmcfg;
                pparmcfg<pevtcfg->aparmcfg+pevtcfg->ucParmNum; pparmcfg++)
        {
            iItemCfgLen-=6+puc[0];
            EP_ID_Copy(pparmcfg->aucName, puc+1, puc[0]);
            puc+=1+puc[0];

            puc+=4;

            pparmcfg->ucAttrib=*puc++;
        }

        assert(iItemCfgLen==14);
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
    {
        iEvtNum_g+=iAlarmNum;
        return EP_SUCCESS;
    }
}

/***********************************************************************
* VI_Get_Evt_Attr - Get event/alarm attribution.
*
* RETURNS:
*					Pointer to the event attribution structure.
*					NULL if iIdx is invalid(>=iEvtNum_g).
*
*/
const VI_EVT_CFG *VI_Get_Evt_Attr(
    int iIdx		/* index of the event/alarm(from 0). */
)
{
    if (iIdx<iEvtNum_g)
        return pevtcfg_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/***********************************************************************
* VI_Cfg_Mea_AI - 遥测配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_Cfg_Mea_AI(
    uint8_t *pucCfg,
    uint32_t ulLen
)
{
    uint8_t *puc;
    VI_MEA_AI_CFG *pmaicfg;
    VI_MEA_AI_DB *pmaidb;
    int iItemCfgLen;
    uint8_t aucPrtc[MAX_ID_LEN+1];
    int i;

    puc=pucCfg;

    iMeaAiNum_g=U8_TO_U16(puc[1], puc[0]);
    puc+=6;

    if ((pmaicfg_g=calloc(iMeaAiNum_g, sizeof(*pmaicfg_g)))==NULL)
        return EP_BUF_ERR;

    if ((pmaidb_g=calloc(iMeaAiNum_g, sizeof(*pmaidb_g)))==NULL)
        return EP_BUF_ERR;

    for (pmaicfg=pmaicfg_g, pmaidb=pmaidb_g;
            pmaicfg<pmaicfg_g+iMeaAiNum_g; pmaicfg++, pmaidb++)
    {
        iItemCfgLen=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pmaicfg->aucId, puc+1, puc[0]);
        puc+=1+puc[0];

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pmaicfg->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        pmaicfg->ucArith=*puc++;

        pmaicfg->ucParaSetMode=*puc++;
        if(pmaicfg->ucParaSetMode&0x01)
        {
            iItemCfgLen-=1+puc[0];
            EP_ID_Copy(pmaicfg->aucRtMaxSettingId,  puc+1, puc[0]);
            puc+=1+puc[0];
        }
        if(pmaicfg->ucParaSetMode&0x02)
        {
            iItemCfgLen-=1+puc[0];
            EP_ID_Copy(pmaicfg->aucRtMinSettingId,  puc+1, puc[0]);
            puc+=1+puc[0];
        }
        if(pmaicfg->ucParaSetMode&0x04)
        {
            iItemCfgLen-=1+puc[0];
            EP_ID_Copy(pmaicfg->aucOvMaxSettingId,  puc+1, puc[0]);
            puc+=1+puc[0];
        }
        if(pmaicfg->ucParaSetMode&0x08)
        {
            iItemCfgLen-=1+puc[0];
            EP_ID_Copy(pmaicfg->aucOvMinSettingId,  puc+1, puc[0]);
            puc+=1+puc[0];
        }
        pmaicfg->bIsPrvtUse=*puc++;
        if(pmaicfg->bIsPrvtUse)
        {
            iItemCfgLen-=1+puc[0];
            EP_ID_Copy(aucPrtc, puc+1, puc[0]);
            puc+=1+puc[0];
            for (i=0; i<iSubLgcNum_g; i++)
            {
                if (!strcmp(psublgc_g[i].aucName, aucPrtc))
                {
                    pmaicfg->psublgc=psublgc_g+i;
                    break;
                }
            }

            if (! pmaicfg->psublgc)
            {
                LOG_Dbg_Msg("ERROR: can't find protect %s for measure ai.\n",
                            (int)aucPrtc, 0, 0, 0, 0, 0);

                return EP_PARM_ERR;
            }
        }

        pmaicfg->ucHmSeq=*puc++;		/* 谐波次数 */
        pmaicfg->ucUnit=*puc++;

        /*2006-6-27日，要求遥测必须是实数信号，否则出错，张云，否则可能会浮点异常  */
        assert((IS_REAL_SIG(pmaicfg->ucUnit) || IS_UNSIGNED_INTEGER_SIG(pmaicfg->ucUnit)));

        pmaicfg->fRtMax=BYTES_TO_FLT(puc);
        puc+=4;

        pmaicfg->fRtMin=BYTES_TO_FLT(puc);
        puc+=4;

        pmaicfg->fOvMax=BYTES_TO_FLT(puc);
        puc+=4;

        pmaicfg->fOvMin=BYTES_TO_FLT(puc);
        puc+=4;

        pmaicfg->aucABRV[0]=*puc++;
        pmaicfg->aucABRV[1]=*puc++;
        pmaicfg->aucABRV[2]=*puc++;
        pmaicfg->aucABRV[3]=*puc++;

        pmaicfg->bNotNeedUpSend=((*puc)&0x01)?TRUE:FALSE ;

        puc+=1;
        pmaicfg->fChgCoff=BYTES_TO_FLT(puc);
        puc+=4;

        assert(iItemCfgLen==32);

        /* Initialize database of MEA_AI. */
        pmaidb->ucArith=pmaicfg->ucArith;

        pmaidb->pvAiHnd=RD_Mea_AI_Hnd(pmaicfg->aucId);
        if (pmaidb->pvAiHnd)
        {
            if (IS_RI_CPLX_AI(AI_HND_TO_UNIT(pmaidb->pvAiHnd)))
                pmaidb->bIsRiCplx=TRUE;

            if (!pmaidb->ucArith)
            {
                assert(IS_REAL_AI(pmaicfg->ucUnit) &&
                       pmaicfg->ucUnit==AI_HND_TO_UNIT(pmaidb->pvAiHnd));
            }
            else if (pmaidb->ucArith==2)
                iRefAngCh_g=-2;

            if((((RD_LGC_AI_CH *)pmaidb->pvAiHnd)->ucFiltTp == 1) || (((RD_LGC_AI_CH *)pmaidb->pvAiHnd)->ucFiltTp == 11))	/* 只是一次谐波区分 */
                pmaicfg->bSrcType=0;		/* 外部通道来源 */
            else
                pmaicfg->bSrcType=1;		/* 非一次谐波 */
        }
        else
        {
            pmaicfg->bSrcType=1;		/* 中间结果 */
        }
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }

    semMeaAi=semMCreate(SEM_Q_PRIORITY);
    assert(semMeaAi!=NULL);

    return EP_SUCCESS;
}

/***********************************************************************
* VI_Cfg_Mea_DI - 遥信配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_Cfg_Mea_DI(
    uint8_t *pucCfg,
    uint32_t ulLen
)
{
    uint8_t *puc;
    VI_MEA_DI_CFG *pmdicfg;
    int iItemCfgLen;
    uint8_t aucPrtc[MAX_ID_LEN+1];
    int i;
    puc=pucCfg;

    iMeaDiNum_g=U8_TO_U16(puc[1], puc[0]);
    puc+=6;

    if ((pmdicfg_g=calloc(iMeaDiNum_g, sizeof(*pmdicfg_g)))==NULL)
        return EP_BUF_ERR;

    if ((pmdidb_g=calloc(iMeaDiNum_g, sizeof(*pmdidb_g)))==NULL)
        return EP_BUF_ERR;

    for (pmdicfg=pmdicfg_g; pmdicfg<pmdicfg_g+iMeaDiNum_g; pmdicfg++)
    {
        iItemCfgLen=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pmdicfg->aucId, puc+1, puc[0]);

#if 0   /* 根据应用需求，不再使用延时功能 */
        /* 是否延迟触发 */
        pmdicfg->bDelay = FALSE;
        if (uiAppType_g == APP_LINE)
        {
            for (i = 0; i<LINE_CUST_MEA_DI_NUM; i++)
            {
                if (!strcmp(pmdicfg->aucId, ucArrMeaDiName[i]))
                {
                    pmdicfg->bDelay = TRUE;
                    break;
                }
            }
        }
#endif

        puc+=1+puc[0];

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pmdicfg->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        pmdicfg->bNotNeedUpSend=((*puc)&0x01)?TRUE:FALSE ;
        pmdicfg->bJianXiuUpSendFlag=((*puc)&0x02)?TRUE:FALSE;
        pmdicfg->bDbSts=((*puc)&0x04)?TRUE:FALSE;
        pmdicfg->bSOE=((*puc)&0x08)?FALSE:TRUE;		/* 配置为0的时候形成，取了一个反，写到sci文件里时按照规约来 */
        puc++;
        pmdicfg->bIsPrvtUse=*puc++;
        if(pmdicfg->bIsPrvtUse)
        {
            iItemCfgLen-=1+puc[0];
            EP_ID_Copy(aucPrtc, puc+1, puc[0]);
            puc+=1+puc[0];
            for (i=0; i<iSubLgcNum_g; i++)
            {
                if (!strcmp(psublgc_g[i].aucName, aucPrtc))
                {
                    pmdicfg->psublgc=psublgc_g+i;
                    break;
                }
            }

            if (! pmdicfg->psublgc)
            {
                LOG_Dbg_Msg("ERROR: can't find protect %s for measure di.\n",
                            (int)aucPrtc, 0, 0, 0, 0, 0);

                return EP_PARM_ERR;
            }
        }
        puc+=2;

        assert(iItemCfgLen==6);

        pmdidb_g[pmdicfg-pmdicfg_g].pvDiHnd=
            RD_Mea_DI_Hnd(pmdicfg->aucId, pmdicfg-pmdicfg_g, pmdicfg->bSOE);

        if(pmdidb_g[pmdicfg-pmdicfg_g].pvDiHnd)
        {
            pmdicfg->bSrcType=0;		/* 外部 */
        }
        else
        {
            pmdicfg->bSrcType=1;		/* 中间结果 */
        }
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
        return EP_SUCCESS;
}

/***********************************************************************
* VI_Cfg_Mea_DO - 遥控配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_Cfg_Mea_DO(
    uint8_t *pucCfg,
    uint32_t ulLen
)
{
    uint8_t *puc;
    VI_MEA_DO_CFG *pmdocfg;
    VI_MEA_DO_DB * pmdodb;
    VI_EVT_PARM_CFG *pparmcfg;
    int iItemCfgLen;
    uint8_t aucmeadoPointerNum[MAX_ID_LEN+1];
    int i;

    puc=pucCfg;

    iMeaDoNum_g=U8_TO_U16(puc[1], puc[0]);
    puc+=10;

    if ((pmdocfg_g=calloc(iMeaDoNum_g, sizeof(*pmdocfg_g)))==NULL)
        return EP_BUF_ERR;
    if ((pmdodb_g=calloc(iMeaDoNum_g, sizeof(*pmdodb_g)))==NULL)
        return EP_BUF_ERR;

    for (pmdocfg=pmdocfg_g,pmdodb=pmdodb_g;
            pmdocfg<pmdocfg_g+iMeaDoNum_g; pmdocfg++,pmdodb++)
    {
        iItemCfgLen=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pmdocfg->aucId, puc+1, puc[0]);
        puc+=1+puc[0];

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pmdocfg->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        pmdocfg->aucABRV[0]=*puc++;
        pmdocfg->aucABRV[1]=*puc++;
        pmdocfg->aucABRV[2]=*puc++;
        pmdocfg->aucABRV[3]=*puc++;

        pmdocfg->uiPtNum=U8_TO_U16(puc[1], puc[0]);
        puc+=2;
        puc+=27;
        pmdocfg->ucType=*puc++;
        pmdocfg->ucParmNum=*puc++;
        /*assert(pmdocfg->ucParmNum>=3 && pmdocfg->ucParmNum<=MAX_EVT_PARM_NUM);*/

        i=0;
        for (pparmcfg=pmdocfg->aparmcfg;
                pparmcfg<pmdocfg->aparmcfg+pmdocfg->ucParmNum; pparmcfg++)
        {
            iItemCfgLen-=10+puc[0];
            EP_ID_Copy(pparmcfg->aucName, puc+1, puc[0]);
            if(i==1)
                EP_ID_Copy(aucmeadoPointerNum,puc+1,puc[0]);
            puc+=1+puc[0];

            puc+=8;

            pparmcfg->ucAttrib=*puc++;
            if(i==0)
                assert(pparmcfg->ucAttrib==0x04);
            if(i==1)
                assert(pparmcfg->ucAttrib==0x61);
            if(i==2)
                assert(pparmcfg->ucAttrib==0x04);
            i++;
        }
        pmdodb->iMeaDoPointerNum=pmdocfg->uiPtNum;
        pmdodb->ucCmdType=0x00;		/* 初始命令类型 */
        assert(iItemCfgLen==37);

    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }

    /*semMeaAi=semMCreate(SEM_Q_PRIORITY);
    assert(semMeaAi!=NULL);*/

    return EP_SUCCESS;

}

/***********************事件触发(不包括遥信变位SOE事件)功能的访问函数接口定义********************************/

/***********************************************************************
* SCI_Init_Get_Event_Info - 根据逻辑图上得到事件的逻辑标识,参数信号来源等相关信息，
*			获得事件的区分码需和调试配置模块中的事件配置信息进行比较纠错.若出错,
*			则返回错误
*
* RETURNS:
*					EP_SUCCESS, 操作成功
*              	EP_BAD_DATA, 找不到同名逻辑标识的保护事件
*              	EP_NOT_INIT, 找到多于1个的同名逻辑标识的保护事件
*               	EP_PARA_ERR, 因事件信息参数和调试配置模块中的事件配置信息不一致,
*                               		  导致的错误
*               	EP_SYS_ERR, 其他原因导致的错误.
*
*/
EP_STATUS SCI_Init_Get_Event_Info(
    SCI_EVENT_INFO_TYPE  *pEventInfo,	/* 逻辑图上得到的事件信息,供初始化时和调试配置模块中的
                              												 相关事件配置信息进行比较查错.并供逻辑图扫描触发事件时,
                              												 根据事件参数信号来源指针数组来访问事件所有参数的即时值.*/
    int16_t *pnRtNum		/* 供返回该事件的区分码 */
)
{
    VI_EVT_CFG *pevtcfg;
    VI_EVT_PARM_CFG *pparm;
    int i;
    BOOL bFind;

    assert(pEventInfo && pnRtNum);
    assert(pEventInfo->strID && strlen(pEventInfo->strID)<=MAX_ID_LEN);

    bFind=FALSE;
    for (pevtcfg=pevtcfg_g; pevtcfg<pevtcfg_g+iEvtNum_g; pevtcfg++)
    {
        if (!strcmp(pEventInfo->strID, pevtcfg->aucId))
        {
            if (!bFind)
            {
                if (pEventInfo->ucParaCount!=pevtcfg->ucParmNum)
                {
                    LOG_Dbg_Msg("ERROR: parameter number mismatch for event"
                                " \"%s\".\n", (int)pEventInfo->strID, 0, 0, 0, 0, 0);

                    return EP_PARM_ERR;
                }

                for (i=0; i<pevtcfg->ucParmNum; i++)
                {
                    pparm=pevtcfg->aparmcfg+i;
                    if (pparm->ucAttrib!=
                            pEventInfo->ppParaSourceSignal[i]->ucAttrib)
                    {
                        LOG_Dbg_Msg("ERROR: parameter attrib mismatch for event"
                                    " \"%s\".\n", (int)pEventInfo->strID, 0, 0, 0, 0, 0);

                        return EP_PARM_ERR;
                    }

                    assert(pEventInfo->ppParaSourceSignal[i]);
                    assert(!pparm->pelmSrc
                           || pparm->pelmSrc==pEventInfo->ppParaSourceSignal[i]);

                    pparm->pelmSrc=pEventInfo->ppParaSourceSignal[i];
                }

                *pnRtNum=pevtcfg-pevtcfg_g;
                bFind=TRUE;
            }
            else
            {
                LOG_Dbg_Msg("ERROR: repeat logic ID \"%s\" in event.\n",
                            (int)pEventInfo->strID, 0, 0, 0, 0, 0);

                return EP_NOT_INIT;
            }
        }
    }

    if (bFind)
        return EP_SUCCESS;
    else
    {
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\" of event.\n",
                    (int)pEventInfo->strID, 0, 0, 0, 0, 0);

        return EP_BAD_DATA;
    }
}

/***********************************************************************
* SCI_Trigger_Event - 根据事件的区分码，触发事件发送
*
* RETURNS: 无
*
*/
void SCI_Trigger_Event(
    int16_t nNum,			/* 事件区分码 */
    uint32_t ulScnTime, 			/* 进行本次逻辑图扫描时的时刻（us计数器值） */
    BOOL bActFlag		/* 事件动作标志，TRUE为表示是事件动作，FALSE表示是事件返回 */
)
{
    VI_RUN_INFO *pinf;
    VI_EVT_CFG *pevtcfg;
    VI_EVT_PARM *pparm;
    VI_EVT_PARM_CFG *pparmcfg;
    int iLockKey;

    bActFlag=bActFlag?TRUE:FALSE;

    pevtcfg=pevtcfg_g+nNum;

    iLockKey=intLock();

    pevtcfg->bStsIn=bActFlag;

    if (pevtcfg->bStsNow != bActFlag && (bActFlag || !pevtcfg->bKeep))
    {
        /* 有变化，同时是上升沿或非自保持 */
        pevtcfg->bStsNow=bActFlag;
        if (bActFlag)
        {
            uiEvtTimes_g++;
            uiEdpStatus_g |= EVT_NOT_CLR;
        }
        else
        {
            uiEvtTimes_g--;
            if (!uiEvtTimes_g)
                uiEdpStatus_g &= ~EVT_NOT_CLR;
            if(pevtcfg->pAlertHdl)
            {
                /* 清除该事件对应的呼唤  2008-1-28日zhangyun  merge,不区分是否是EDP02*/
                ER_ClearAlertSignal(pevtcfg->pAlertHdl);
            }
        }
    }

    intUnlock(iLockKey);

    if (bActFlag || (!pevtcfg->bKeep && pevtcfg->bHaveSts))
    {
        /* 上升沿，或者非自保持同时有状态 */
        pinf=VI_Run_Info_Wr_P();

        if(bViewModIsInit_g)
        {
            pinf->bViewModIsInit=TRUE;
        }
        else
        {
            pinf->bViewModIsInit=FALSE;
        }

        pinf->type=NEW_EVT;

        pinf->msg.evt.pcfg=pevtcfg;
        pinf->msg.evt.ucCOT=1;

        iLockKey=intLock();

        if (rptsts_g.iFault)
        {
            pinf->msg.evt.unRptSN=rptsts_g.unRptSN;
            pinf->msg.evt.ucRecSN=rptsts_g.ucRecSN;
        }
        else
        {
            pinf->msg.evt.unRptSN=rptsts_g.unRptSN++;
            pinf->msg.evt.ucRecSN=0;
        }

        intUnlock(iLockKey);

        pinf->msg.evt.bState=bActFlag;

        pinf->msg.evt.ulTime=ulScnTime;



        for (pparm=pinf->msg.evt.aparm, pparmcfg=pevtcfg->aparmcfg;
                pparmcfg<pevtcfg->aparmcfg+pevtcfg->ucParmNum; pparm++, pparmcfg++)
        {
            pparm->xVal=pparmcfg->pelmSrc->now.xVal;
        }

        if (!pinf->msg.evt.pcfg->bNeedFastDel)
        {
            VI_WrEventDelAttr(pinf->msg.evt.unRptSN);
        }

        VI_End_Wr_Run_Info();
    }
}

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)

/***********************************************************************
* VI_Clear_Evt - Clear self-keep event signals.
*
* RETURNS: 无
*
*/
void VI_Clear_Evt(
    int nType		/* 暂时使用 */
)
{
    VI_RUN_INFO *pinf;
    VI_EVT_CFG *pevtcfg;
    VI_EVT_PARM *pparm;
    VI_EVT_PARM_CFG *pparmcfg;
    uint32_t ulTime;
    int iLockKey;

    EP_Set_Sts_Bit(CLR_EVT_FLAG);

    ulTime=TM_Get_usCnt();

    for (pevtcfg=pevtcfg_g; pevtcfg<pevtcfg_g+iEvtNum_g; pevtcfg++)
    {
        iLockKey=intLock();

        if (pevtcfg->bStsNow && pevtcfg->bKeep && !pevtcfg->bStsIn)
        {
            pevtcfg->bStsNow=FALSE;

            uiEvtTimes_g--;
            if (!uiEvtTimes_g)
                uiEdpStatus_g &= ~EVT_NOT_CLR;

            intUnlock(iLockKey);

            pinf=VI_Run_Info_Wr_P();
            if(bViewModIsInit_g)
            {
                pinf->bViewModIsInit=TRUE;
            }
            else
            {
                pinf->bViewModIsInit=FALSE;
            }

            pinf->type=NEW_EVT;

            pinf->msg.evt.pcfg=pevtcfg;
            pinf->msg.evt.ucCOT=1;
            pinf->msg.evt.unRptSN=rptsts_g.unRptSN;
            pinf->msg.evt.ucRecSN=rptsts_g.ucRecSN;
            pinf->msg.evt.bState=FALSE;
            pinf->msg.evt.ulTime=ulTime;

            for (pparm=pinf->msg.evt.aparm, pparmcfg=pevtcfg->aparmcfg;
                    pparmcfg<pevtcfg->aparmcfg+pevtcfg->ucParmNum; pparm++, pparmcfg++)
            {
                pparm->xVal=pparmcfg->pelmSrc->now.xVal;
            }
            if (!pinf->msg.evt.pcfg->bNeedFastDel)
            {
                VI_WrEventDelAttr(pinf->msg.evt.unRptSN);
            }

            VI_End_Wr_Run_Info();
        }
        else
            intUnlock(iLockKey);
    }

    if(nType == 0)
        SIO_Clr_DO_Keep();
    else if(nType == 1)
    {
#ifdef EDP03_BUILD
        SIO_Clr_DO_Keep_Logic();
#endif
    }

    ER_Clear_Alert();   /* 复归时，清除所有呼唤 */

    EP_Clr_Sts_Bit(CLR_EVT_FLAG);

    LOG_Dbg_Msg("装置复归!bEnableAlarm_g=%d HW_TEST=%d\n", bEnableAlarm_g, EP_IN_HW_TEST(), 0, 0, 0, 0);

#if (defined(EDP03_BUILD) && (defined(EDP03_LOWPROTECT_BUILD)))		/* When the alarm occur, 
	                the low protect equipment wil reboot, but the equipment for electric power stabbility and Intellegent operating box do not. */
    if(bEnableAlarm_g && (!EP_IN_HW_TEST()))
    {
        /* If the alarm exsit, and system don't be in test state, the reset will reboot the system. */
        EDPreboot(REBOOT_ACTIVE);
    }
#endif
}
#endif

#if defined(EDP_01_02_BUILD)

/***********************************************************************
* VI_Clear_Evt - Clear self-keep event signals.
*
* RETURNS: 无
*
*/
void VI_Clear_Evt(void)
{
    VI_RUN_INFO *pinf;
    VI_EVT_CFG *pevtcfg;
    VI_EVT_PARM *pparm;
    VI_EVT_PARM_CFG *pparmcfg;
    uint32_t ulTime;
    int iLockKey;

    EP_Set_Sts_Bit(CLR_EVT_FLAG);

    ulTime=TM_Get_usCnt();

    for (pevtcfg=pevtcfg_g; pevtcfg<pevtcfg_g+iEvtNum_g; pevtcfg++)
    {
        iLockKey=intLock();

        if (pevtcfg->bStsNow && pevtcfg->bKeep && !pevtcfg->bStsIn)
        {
            pevtcfg->bStsNow=FALSE;

            uiEvtTimes_g--;
            if (!uiEvtTimes_g)
                uiEdpStatus_g &= ~EVT_NOT_CLR;

            intUnlock(iLockKey);

            pinf=VI_Run_Info_Wr_P();

            if(bViewModIsInit_g)		/* 2006-8-2 张云 */
            {
                pinf->bViewModIsInit=TRUE;
            }
            else
            {
                pinf->bViewModIsInit=FALSE;
            }
            pinf->type=NEW_EVT;

            pinf->msg.evt.pcfg=pevtcfg;
            pinf->msg.evt.ucCOT=1;
            pinf->msg.evt.unRptSN=rptsts_g.unRptSN;
            pinf->msg.evt.ucRecSN=rptsts_g.ucRecSN;
            pinf->msg.evt.bState=FALSE;
            pinf->msg.evt.ulTime=ulTime;

            for (pparm=pinf->msg.evt.aparm, pparmcfg=pevtcfg->aparmcfg;
                    pparmcfg<pevtcfg->aparmcfg+pevtcfg->ucParmNum; pparm++, pparmcfg++)
            {
                pparm->xVal=pparmcfg->pelmSrc->now.xVal;
            }
            if (!pinf->msg.evt.pcfg->bNeedFastDel)
            {
                VI_WrEventDelAttr(pinf->msg.evt.unRptSN);
            }

            VI_End_Wr_Run_Info();
        }
        else
            intUnlock(iLockKey);
    }

    SIO_Clr_DO_Keep();

    ER_Clear_Alert();   /* 复归时，清除所有呼唤 */
    EP_Clear_Alarm(); /* 复归时，清除告警 */

    EP_Clr_Sts_Bit(CLR_EVT_FLAG);
}

#endif

/****************************************遥信功能的访问函数接口定义********************************/

/***********************************************************************
* SCI_Init_Add_New_Yaoxin_Signal - 逻辑图中添加1个新的中间结果遥信量到遥信量集中
*
* RETURNS:
*					EP_SUCCESS, 操作成功
*             	EP_BAD_DATA, 找不到同名逻辑标识的遥信量
*              	EP_NOT_INIT, 找到多于1个的同名逻辑标识的遥信量
*              	EP_PARA_ERR, 因遥信量数据指针参数和调试配置模块中的遥信量配置
*                                   	  信息不一致,导致的错误
*          		EP_SYS_ERR, 其他原因导致的错误.
*
*/
EP_STATUS  SCI_Init_Add_New_Yaoxin_Signal(
    uint8_t *strID,		/* 该遥信量的逻辑标识 */
    EP_ELEM_IO *pYaoxinSignal, 			/* 该遥信量的数据访问指针,用于遥信处理时,
                                         								 访问该遥信量 */
    uint32_t ulScanTaskNo		/* 该遥信量所在的逻辑图扫描任务号。
                                  						 用于遥信操作时区分不同任务的遥信量 */
)
{
    VI_MEA_DI_CFG *pmdicfg;
    VI_MEA_DI_DB *pmdidb;
    VI_MEA_DI_DB **ppmdidbTsk;
    BOOL bFind;

    assert(strID && strlen(strID)<=MAX_ID_LEN);
    assert(pYaoxinSignal && pYaoxinSignal->ucAttrib==0x04);
    assert(ulScanTaskNo<MAX_SUB_LGC_NUM);

    bFind=FALSE;
    for (pmdicfg=pmdicfg_g; pmdicfg<pmdicfg_g+iMeaDiNum_g; pmdicfg++)
    {
        if (!strcmp(strID, pmdicfg->aucId))
        {
            pmdidb=pmdidb_g+(pmdicfg-pmdicfg_g);

            assert(!pmdidb->pvDiHnd && !pmdidb->pelmSrc);

            if (!bFind)
            {
                bFind=TRUE;

                if (!appmdidbTsk_g[ulScanTaskNo])
                {
                    if ((appmdidbTsk_g[ulScanTaskNo]=
                                calloc(iMeaDiNum_g+1, sizeof(*appmdidbTsk_g[0])))==NULL)
                        return EP_SYS_ERR;
                }

                for (ppmdidbTsk=appmdidbTsk_g[ulScanTaskNo];
                        ppmdidbTsk<appmdidbTsk_g[ulScanTaskNo]+iMeaDiNum_g;
                        ppmdidbTsk++)
                {
                    if (!*ppmdidbTsk)
                    {
                        *ppmdidbTsk=pmdidb;
                        break;
                    }
                }
                assert(ppmdidbTsk<appmdidbTsk_g[ulScanTaskNo]+iMeaDiNum_g);

                pmdidb->pelmSrc=pYaoxinSignal;
                pmdidb->bSOE=pmdicfg->bSOE;		/* 是否上传SOE */
            }
            else
            {
                LOG_Dbg_Msg("ERROR: repeat logic ID \"%s\" in MEA_DI.\n",
                            (int)strID, 0, 0, 0, 0, 0);

                return EP_NOT_INIT;
            }
        }
    }

    if (bFind)
        return EP_SUCCESS;
    else
    {
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\" of MEA_DI.\n",
                    (int)strID, 0, 0, 0, 0, 0);

        return EP_BAD_DATA;
    }
}

/***********************************************************************
* SCI_Init_Add_TimeSet_New_Yaoxin_Signal - 逻辑图中添加1个新的中间结果遥信量，时间由逻辑图给出
*
* RETURNS:
*				EP_SUCCESS, 操作成功
*             	EP_BAD_DATA, 找不到同名逻辑标识的遥信量
*              	EP_NOT_INIT, 找到多于1个的同名逻辑标识的遥信量
*              	EP_PARA_ERR, 因遥信量数据指针参数和调试配置模块中的遥信量配置
*                                   	  信息不一致,导致的错误
*          		EP_SYS_ERR, 其他原因导致的错误.
*
*/
EP_STATUS SCI_Init_Add_TimeSet_New_Yaoxin_Signal(
    uint8_t *strID,		/* 该遥信量的逻辑标识 */
    int32_t *piCh,				/* 遥信号 */
    uint8_t *pucType			/* Single or double */
)
{
    VI_MEA_DI_CFG *pmdicfg;
    BOOL bFind;

    assert(strID && strlen(strID) <= MAX_ID_LEN);

    bFind=FALSE;
    for (pmdicfg=pmdicfg_g; pmdicfg<pmdicfg_g+iMeaDiNum_g; pmdicfg++)
    {
        if (!strcmp(strID, pmdicfg->aucId))
        {
            if (!bFind)
            {
                /* 配置为双点遥信 */
                if(pmdicfg->bDbSts && pmdicfg->bSOE)
                {
                    /* Double YaoXin */
                    *piCh=pmdicfg-pmdicfg_g;		/* 遥信点号 */
                    *pucType=0x01;
                }
                else if(pmdicfg->bSOE)
                {
                    /* Single YaoXin */
                    *piCh=pmdicfg-pmdicfg_g;		/* 遥信点号 */
                    *pucType=0x00;
                }
                else
                {
                    *piCh=-1;		/* 不处理 */
                }
                bFind=TRUE;
            }
            else
            {
                LOG_Dbg_Msg("ERROR: repeat logic ID \"%s\" in MEA_DI.\n",
                            (int)strID, 0, 0, 0, 0, 0);

                return EP_NOT_INIT;
            }
        }
    }

    if (bFind)
        return EP_SUCCESS;
    else
    {
        *piCh=-1;		/* Cann't find. */
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\" of MEA_DI.\n",
                    (int)strID, 0, 0, 0, 0, 0);

        return EP_BAD_DATA;
    }
}

/***********************************************************************
* SCI_Process_Cur_Logrp_Period_Yaoxin - 处理本扫描任务的本次逻辑图扫描周期的遥信操作,
*																		该函数在扫描任务的每次逻辑图扫描的最后进行调用。
*
* RETURNS: 无
* Alert:
* 		实现时，若在本次扫描时若有1个或多个遥信量发生变位，则可触发1个或多个SOE事件
*    	（在1次扫描周期内，若有多个遥信变位时，是只触发1个遥信SOE事件，还是多个，
*    	实现时斟酌）
*
*/
void SCI_Process_Cur_Logrp_Period_Yaoxin(
    uint32_t  ulScanTaskNo,		/* 进行本次遥信操作的扫描任务号 */
    uint32_t ulScnAiCnt			/* 进行本次逻辑图扫描时的AI采样计数器值 */
)
{
    VI_MEA_DI_DB **ppmdidb;
    VI_MEA_DI_DB *pmdidb;

    assert(ulScanTaskNo<MAX_SUB_LGC_NUM);

    ppmdidb=appmdidbTsk_g[ulScanTaskNo];
    if (!ppmdidb)
        return;

    while ((pmdidb=*ppmdidb++) != NULL)
    {
        assert(pmdidb->pelmSrc);

        if (pmdidb->pelmSrc->now.bVal != pmdidb->bVal)
        {
            pmdidb->bVal=pmdidb->pelmSrc->now.bVal;
            pmdidb->usQuality = 0;

            /* Create SOE record. */
            VI_New_SOE(pmdidb-pmdidb_g, pmdidb->bVal,
                       RD_AI_Cnt_To_us(ulScnAiCnt+1),
                       pmdidb->bSOE,
                       pmdidb->usQuality);		/* 传递数据窗时间 */
        }
    }
}

/***********************************************************************
* VI_New_SOE - Record new SOE.
*
* RETURNS: 无
*
*/
void VI_New_SOE(
    int iCh,		/* MEA_DI channel number. */
    BOOL bSts, 			/* new DI status. */
    uint32_t ulTime,					/* us time. */
    BOOL bSOE,					/* If create SOE. */
    uint16_t usQuality
)
{
    VI_RUN_INFO *pinf;
    int iLockKey;

    assert(iCh>=0 && iCh<iMeaDiNum_g);

    pinf=VI_Run_Info_Wr_P();

    if(bViewModIsInit_g)
    {
        pinf->bViewModIsInit=TRUE;
    }
    else
    {
        pinf->bViewModIsInit=FALSE;
    }

    pinf->type=NEW_SOE;

    pinf->msg.soe.pcfg=pmdicfg_g+iCh;
    pinf->msg.soe.ucCOT=1;

    pinf->msg.soe.ucFtype=16;

#if 0   /* 根据应用需求，不再使用延时功能 */
    if (pinf->msg.soe.pcfg->bDelay)
    {
        pinf->msg.soe.ulTime = ulTime-LINE_REC_DELAY_TIME;	/* 时间延迟 */
    }
    else
#endif
    {
        pinf->msg.soe.ulTime=ulTime;	/*均使用真实事件*/
    }

    pinf->msg.soe.unCh=iCh;

    if (bSts)
        pinf->msg.soe.ucDIQ=0x21;
    else
        pinf->msg.soe.ucDIQ=0x20;

    /* 品质位 */
    if (usQuality & 0x0010)
        pinf->msg.soe.ucDIQ |= 0x80;
    else
        pinf->msg.soe.ucDIQ &= (~0x80);

    if(!bSOE)
    {
        pinf->msg.soe.ucDIQ=pinf->msg.soe.ucDIQ|0x40;
    }

    iLockKey=intLock();

    if (rptsts_g.iFault)
    {
        pinf->msg.soe.unRptSN=rptsts_g.unRptSN;
        pinf->msg.soe.ucRecSN=rptsts_g.ucRecSN;
    }
    else
    {
        pinf->msg.soe.unRptSN=rptsts_g.unRptSN++;
        pinf->msg.soe.ucRecSN=0;
    }

    intUnlock(iLockKey);

    VI_End_Wr_Run_Info();
}

/***********************************************************************
* VI_New_TimeSet_SOE - Record new SOE.
*
* RETURNS: NONE
*
*/
void VI_New_TimeSet_SOE(
    int iCh,		/* MEA_DI channel number. */
    int32_t iSts, 			/* new status. */
    uint32_t ulTime,					/* us time. */
    BOOL bSOE,
    uint16_t usQuality
)
{
    VI_RUN_INFO *pinf;
    int iLockKey;
    VI_MEA_DI_DB *pmdidb;
    uint8_t ucDIQ;

    ucDIQ = iSts&0x03;

    /* 品质位 */
    ucDIQ |= VI_SOE_CHANGE_STS;

    if (usQuality & DI_REPAIR_STS)
        ucDIQ |= 0x80;
    else
        ucDIQ &= (~0x80);

    if (usQuality & DI_INVALID_STS)
    {
        ucDIQ |= VI_SOE_OLD_DATA_STS;
        ucDIQ |= VI_SOE_INVALID_STS;
    }
    else
    {
        ucDIQ &= (~VI_SOE_OLD_DATA_STS);
        ucDIQ &= (~VI_SOE_INVALID_STS);
    }

    if(!bSOE)
    {
        ucDIQ |= VI_SOE_EVENT_STS;
    }

    pmdidb=pmdidb_g+iCh;
    pmdidb->bVal=ucDIQ;
    pmdidb->usQuality = usQuality;

    if((uiAppType_g == APP_PROT_MEA_MERGE)&&(!bSOE))
    {
        /* 测控装置如果SOE不使能则返回不产生SOE事件 */
        return;
    }

    pinf=VI_Run_Info_Wr_P();

    if(uiAppType_g == APP_PROT_MEA_MERGE)
    {
        /* 为了获取绝对时标,在测控CPU下装置取ulTime为指向绝对时标的指针 */
        pinf->msg.soe.ullusCntFrom1970=*(uint64_t*)ulTime;
        //printf("VI SOE 变位时间%llu\n", pinf->msg.soe.ullusCntFrom1970);
    }
    else
    {
        /* 非测控CPU正常使用相对时标 */
        pinf->msg.soe.ulTime=ulTime;
        pinf->msg.soe.ullusCntFrom1970=0;
    }

    if(bViewModIsInit_g)
    {
        pinf->bViewModIsInit=TRUE;
    }
    else
    {
        pinf->bViewModIsInit=FALSE;
    }

    pinf->type=NEW_SOE;

    pinf->msg.soe.pcfg=pmdicfg_g+iCh;
    pinf->msg.soe.ucCOT=1;

    pinf->msg.soe.ucFtype=16;

    pinf->msg.soe.unCh=iCh;

    /* 品质位 */
    pinf->msg.soe.ucDIQ = ucDIQ;

    iLockKey=intLock();

    if (rptsts_g.iFault)
    {
        pinf->msg.soe.unRptSN=rptsts_g.unRptSN;
        pinf->msg.soe.ucRecSN=rptsts_g.ucRecSN;
    }
    else
    {
        pinf->msg.soe.unRptSN=rptsts_g.unRptSN++;
        pinf->msg.soe.ucRecSN=0;
    }

    intUnlock(iLockKey);

    VI_End_Wr_Run_Info();
}



/****************************************遥测功能的访问函数接口定义********************************/

/***********************************************************************
* SCI_Init_Add_New_Yaoce_Signal - 逻辑图中添加1个新的中间结果遥测量到遥测量集中
*
* RETURNS:
*					EP_SUCCESS, 操作成功
*             	EP_BAD_DATA, 找不到同名逻辑标识的遥测量
*              	EP_NOT_INIT, 找到多于1个的同名逻辑标识的遥测量
*               	EP_PARA_ERR, 因遥测量数据指针参数和调试配置模块中的遥测量配置信息
*               							 不一致,导致的错误
*               	EP_SYS_ERR, 其他原因导致的错误.
*
*/
EP_STATUS SCI_Init_Add_New_Yaoce_Signal(
    uint8_t  *strID,			/* 该遥测量的逻辑标识 */
    EP_ELEM_IO *pYaoceSignal, 			/* 该遥测量的数据访问指针,用于遥测处理时,访问该遥测量 */
    uint32_t ulScanTaskNo		/* 该遥测量所在的逻辑图扫描任务号。用于遥测操作时区分不同任务的遥测量 */
)
{
    VI_MEA_AI_CFG *pmaicfg;
    VI_MEA_AI_DB *pmaidb;
    BOOL bFind;

    assert(strID && strlen(strID)<=MAX_ID_LEN);
    assert(pYaoceSignal);               /* TODO: pYaoceSignal->ucAttrib? */
    assert(ulScanTaskNo<MAX_SUB_LGC_NUM);

    bFind=FALSE;
    for (pmaicfg=pmaicfg_g; pmaicfg<pmaicfg_g+iMeaAiNum_g; pmaicfg++)
    {
        if (!strcmp(strID, pmaicfg->aucId))
        {
            pmaidb=pmaidb_g+(pmaicfg-pmaicfg_g);

            assert(!pmaidb->pelmSrc && !pmaidb->pvAiHnd);

            pmaidb->pelmSrc=pYaoceSignal;
            pmaidb->ulScanTaskNo=ulScanTaskNo;

            if (IS_CPLX_AI(pYaoceSignal->ucAttrib))
            {
                bFind=TRUE;
                if (IS_RI_CPLX_AI(pYaoceSignal->ucAttrib))
                    pmaidb->bIsRiCplx=TRUE;

                if (pmaidb->ucArith==2)
                    iRefAngCh_g=-2;
                else if (!pmaidb->ucArith)
                {
                    LOG_Dbg_Msg("ERROR: arithmetic mismatch for MEA_AI \"%s\".\n",
                                (int)strID, 0, 0, 0, 0, 0);

                    return EP_PARM_ERR;
                }
            }
            else if (!bFind)
            {
                bFind=TRUE;

                if (pmaidb->ucArith || pmaicfg->ucUnit!=pYaoceSignal->ucAttrib)
                {
                    LOG_Dbg_Msg("ERROR: arithmetic mismatch for MEA_AI \"%s\".\n",
                                (int)strID, 0, 0, 0, 0, 0);

                    return EP_PARM_ERR;
                }
            }
            else
            {
                LOG_Dbg_Msg("ERROR: repeat logic ID \"%s\" in MEA_AI.\n",
                            (int)strID, 0, 0, 0, 0, 0);

                return EP_NOT_INIT;
            }
        }
    }

    if (bFind)
        return EP_SUCCESS;
    else
    {
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\" of MEA_AI.\n",
                    (int)strID, 0, 0, 0, 0, 0);

        return EP_BAD_DATA;
    }
}

/***********************************************************************
* SCI_Process_Cur_Logrp_Period_Yaoce - 处理本扫描任务的本次逻辑图扫描周期的遥测操作,
*			该函数在该扫描任务的每次逻辑图扫描的最后进行调用。实现时，
*			若在本次扫描时若有1个或多个遥测量发生越限，则可进行遥测量的
*       	主动上送或循环上送
*
* RETURNS: 无
*
*/
void SCI_Process_Cur_Logrp_Period_Yaoce(
    uint32_t  ulScanTaskNo,		/* 进行本次遥测操作的扫描任务号 */
    uint32_t ulScnAiCnt		/* 进行本次逻辑图扫描时的AI采样计数器值 */
)
{

}

/***********************************************************************
* VI_Get_Mea_AI_Attr - Get measurement AI attribution.
*
* RETURNS:
* 			Pointer to the MEA_AI attribution structure.
*			NULL if iIdx is invalid(>=iMeaAiNum_g).
*
*/
const VI_MEA_AI_CFG *VI_Get_Mea_AI_Attr(
    int iIdx		/* index of the MEA_AI(from 0). */
)
{
    if (iIdx<iMeaAiNum_g)
        return pmaicfg_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/***********************************************************************
* VI_Get_Mea_DI_Attr - Get measurement DI attribution.
*
* RETURNS:
* 			Pointer to the MEA_DI attribution structure.
*			NULL if iIdx is invalid(>=iMeaDiNum_g).
*
*/
const VI_MEA_DI_CFG *VI_Get_Mea_DI_Attr(
    int iIdx		/* index of the MEA_DI(from 0). */
)
{
    if (iIdx<iMeaDiNum_g)
        return pmdicfg_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/***********************************************************************
* VI_Get_Mea_DO_Attr - Get measurement DO attribution.
*
* RETURNS:
* 			Pointer to the MEA_DO attribution structure.
*			NULL if iIdx is invalid(>=iMeaDoNum_g).
*
*/
const VI_MEA_DO_CFG *VI_Get_Mea_DO_Attr(
    int iIdx		/* index of the MEA_DO(from 0). */
)
{
    if (iIdx<iMeaDoNum_g)
        return pmdocfg_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/***********************************************************************
* VI_Get_Mea_Do_Num - 获取遥控点序号
*
* RETURNS: 无
*
*/
BOOL VI_Get_Mea_Do_Num(
    uint8_t *strID,
    uint16_t *puiRtNum,
    uint8_t *pucRtParaNum
)
{
    VI_MEA_DO_CFG *pmdocfg;

    if(!iMeaDoNum_g)
        return FALSE;

    for (pmdocfg=pmdocfg_g; pmdocfg<pmdocfg_g+iMeaDoNum_g; pmdocfg++)
    {
        if (!strcmp(strID, pmdocfg->aucId))
        {
            *puiRtNum=pmdocfg-pmdocfg_g;
            *pucRtParaNum=pmdocfg->ucParmNum;
            return TRUE;
        }
    }

    return FALSE;
}

/***********************************************************************
* VI_New_MeaDo - 遥控
*
* RETURNS: 无
*
*/
BOOL VI_New_MeaDo(
    uint8_t PtNum, 			/* 点号 */
    uint8_t OptNum,			/* 遥控类型 */
    uint8_t ucCmdType,		/* Type of order，有预发和执行两种类型 */
    uint32_t OptPara,	/* 脉宽长度 */
    uint32_t usTqPara		/* 同期参数 */
)
{
    VI_MEA_DO_DB *pmdodb;
    BOOL bFindSuccess = FALSE;
    STATUS vxsts;
    uint8_t count;

    if (!iMeaDoNum_g)
        return FALSE;

    PtNum++;
    for (pmdodb = pmdodb_g; pmdodb<pmdodb_g+iMeaDoNum_g; pmdodb++)
    {
        if (pmdodb->iMeaDoPointerNum == PtNum)
        {
            bFindSuccess = TRUE;
            break;
        }
    }

    if (!bFindSuccess)
        return FALSE;

    if (bNotUseRTFdbkFlag)
    {
        /* if process remote control feedback. */
        if (ucCmdType == 0x00)
        {
            /* prep remote control. */
            return TRUE;
        }
    }

    vxsts = taskLock();
    pmdodb->ucCmdType = ucCmdType;		/* Type of order. */
    LOG_Dbg_Msg("遥控命令为%x 类型为%x 脉宽为%d 同期参数为%d\n", ucCmdType, OptNum, OptPara, usTqPara, 0, 0);
    pmdodb->bVal = TRUE;
    pmdodb->bAppSetFlag = TRUE;
    switch (OptNum)
    {
        case 0x55:
            pmdodb->type = SHORT_PULSE_ACT;
            break;

        case 0x5A:
            pmdodb->type = LONG_PULSE_ACT;
            break;

        case 0xA5:
            pmdodb->type = MEA_DO_RETURN;
            break;

        case 0xAA:
        {
            pmdodb->type = CUSTOM_PULSE_ACT;
            pmdodb->uiPulseTm = OptPara;
        }
        break;

        default:
            assert(FALSE);
            break;
    }

    pmdodb->ulTqPara = usTqPara;
    vxsts = taskUnlock();

    count = 0;
    while (pmdodb->bVal)
    {
        taskDelay(1);
        count++;

        if (count>10)
        {
            break;
        }
    }

    if (pmdodb->bVal)  	/* 逻辑图中没有响应这遥控命令*/
    {
        LOG_Dbg_Msg("逻辑图无遥控图元.\n", 0, 0, 0, 0, 0, 0);
        pmdodb->bVal = FALSE;

        return FALSE;
    }

    if (bNotUseRTFdbkFlag)
    {
        /* if process remote control feedback. */
        pmdodb->bAppSetFlag = FALSE;

        return TRUE;
    }

    count = 0;
    while (pmdodb->bAppSetFlag)
    {
        taskDelay(1);
        count++;

        if (count>100)
        {
            break;
        }
    }

    if (pmdodb->bAppSetFlag)		/* 遥控是否实际执行 */
    {
        /* 如果没有执行 */
        LOG_Dbg_Msg("逻辑图没有执行遥控.\n", 0, 0, 0, 0, 0, 0);
        pmdodb->bAppSetFlag = FALSE;

        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
* VI_Come_New_MeaDo - 遥控
*
* RETURNS: 无
*
*/
BOOL VI_Come_New_MeaDo(
    uint16_t uiNodeNum,
    int *pirtPtNum,
    BOOL *pbRtSignal,
    uint32_t *pulRtPulseTm,
    uint32_t *pulTqPara
)
{
    VI_MEA_DO_DB *pmdodb;
    uint32_t ulTmpCmdType;

    pmdodb=pmdodb_g+uiNodeNum;
    if(pmdodb->bVal)
    {
        *pirtPtNum=pmdodb->iMeaDoPointerNum;
        ulTmpCmdType=pmdodb->ucCmdType;

        *pirtPtNum=*pirtPtNum&0x0000FFFF;
        *pirtPtNum=*pirtPtNum+((ulTmpCmdType<<16)&0x00FF0000);
        if(pmdodb->type==MEA_DO_RETURN)
            *pbRtSignal=FALSE;
        else
            *pbRtSignal=TRUE;
        if(pmdodb->type==CUSTOM_PULSE_ACT)
        {
            *pulRtPulseTm=pmdodb->uiPulseTm;
        }

        *pulTqPara=pmdodb->ulTqPara;

        pmdodb->bVal=FALSE;
        return TRUE;
    }

    return FALSE;
}

/***********************************************************************
* VI_SetMeaDo - 逻辑图中执行实际遥控时调用，通知MMI调用成功
*
* RETURNS: OK, or ERROR
*
*/
BOOL VI_SetMeaDo(
    int irtPtNum		/* 遥控点号 */
)
{
    VI_MEA_DO_DB *pmdodb = NULL;
    BOOL bFindSuccess = FALSE;

    if(!iMeaDoNum_g)
        return FALSE;

    for(pmdodb=pmdodb_g; pmdodb<pmdodb_g+iMeaDoNum_g; pmdodb++)
    {
        if(pmdodb->iMeaDoPointerNum == irtPtNum)
        {
            bFindSuccess=TRUE;
            break;
        }
    }

    if(!bFindSuccess)
    {
        return FALSE;
    }

    if(pmdodb->bAppSetFlag)
    {
        pmdodb->bAppSetFlag=FALSE;
    }

    return TRUE;
}

/***********************************************************************
* VI_Rd_Mea_AI_Val - Read all measurement AIs' value.
*
* RETURNS: NONE
*
* Alert:
*		pfRslt must contains space to save iMeaAiNum_g float numbers.
*/
void VI_Rd_Mea_AI_Val(
    float *pfRslt		/* to save all MEA_AIs' current value. */
)
{
    static uint32_t ulCalcTime;
    BOOL bReCalc;
    VI_MEA_AI_DB *pmaidb;
    float fDeltaAng=0.0;
    uint32_t ulAiCnt;
    STATUS vxsts;

    if (!iMeaAiNum_g)
        return;

    vxsts=semTake(semMeaAi, WAIT_FOREVER);
    assert(vxsts==OK);

    if (TM_Get_usCnt()-ulCalcTime<500000L)
    {
        bReCalc=FALSE;
        goto saverslt;
    }

    bReCalc=TRUE;

    ulAiCnt=RD_AI_Cnt();

    vxsts=taskLock();
    assert(vxsts==OK);

    for (pmaidb=pmaidb_g; pmaidb<pmaidb_g+iMeaAiNum_g; pmaidb++)
    {
        if (pmaidb->pelmSrc)
        {
            if (pmaidb->ucArith)
                pmaidb->xVal=pmaidb->pelmSrc->now.xVal;
            else
                pmaidb->fVal=pmaidb->pelmSrc->now.fVal;
        }
        else if (pmaidb->pvAiHnd)
        {
            if (pmaidb->ucArith)
            {
                /* 张云2006-4-14日修改过 */
                COMPLEX *pxAI;
                pxAI=RD_Calc_AI_P(pmaidb->pvAiHnd, ulAiCnt);
                if(pxAI != NULL)
                {
                    pmaidb->xVal=*pxAI;
                }
                else
                {
                    pmaidb->xVal=0.0+0.0i;
                }
            }
            else
            {
                float  *pfAI;
                pfAI=RD_Lgc_AI_P(pmaidb->pvAiHnd, ulAiCnt);
                if(pfAI != NULL)
                {
                    pmaidb->fVal=*pfAI;
                }
                else
                {
                    pmaidb->fVal=0.0;
                }
            }
        }
    }

    vxsts=taskUnlock();
    assert(vxsts==OK);

    if (iRefAngCh_g != -1)                /* Need caculate relative angle. */
    {
        for (pmaidb=pmaidb_g; pmaidb<pmaidb_g+iMeaAiNum_g; pmaidb++)
        {
            if (pmaidb->ucArith==2 && pmaidb->bIsRiCplx)
            {
                pmaidb->xVal=RI_CPLX_MOD(pmaidb->xVal)+RI_CPLX_ANG(pmaidb->xVal)*1i;
            }
        }

        /* Never search refrence channel or module of ref-channel too small. */

        /*每次重新刷新基准通道,张云改过  */
        iRefAngCh_g=-2;
        if (iRefAngCh_g==-2 || REAL(pmaidb_g[iRefAngCh_g].xVal)<fMinRefMod_g)
            VI_Search_Ref_Ch();

        pmaidb=pmaidb_g+iRefAngCh_g;
        fDeltaAng=0.01-IMAGE(pmaidb->xVal);/*张云修改过的 */
    }

    ulCalcTime=TM_Get_usCnt();

saverslt:
    for (pmaidb=pmaidb_g; pmaidb<pmaidb_g+iMeaAiNum_g; pmaidb++)
    {
        switch (pmaidb->ucArith)
        {
            case 0:
                *pfRslt++=pmaidb->fVal;
                break;

            case 1:
                if (pmaidb->bIsRiCplx)
                    *pfRslt++=RI_CPLX_MOD(pmaidb->xVal);
                else
                    *pfRslt++=REAL(pmaidb->xVal);
                break;

            case 2:
                if (bReCalc)
                {
                    pmaidb->fVal=IMAGE(pmaidb->xVal)+fDeltaAng;

                    if (pmaidb->fVal>360.0)
                        pmaidb->fVal-=360.0;
                    else if (pmaidb->fVal<0.0)
                        pmaidb->fVal+=360.0;
                }

                if (REAL(pmaidb->xVal)>pmaidb->fAngVldMod)
                    *pfRslt++=pmaidb->fVal;
                else
                    *pfRslt++=0.0;
                break;

            case 3:
                if (pmaidb->bIsRiCplx)
                    *pfRslt++=RI_CPLX_ANG(pmaidb->xVal);
                else
                    *pfRslt++=IMAGE(pmaidb->xVal);
                break;

            case 4:
                *pfRslt++=REAL(pmaidb->xVal);
                break;

            case 5:
                *pfRslt++=IMAGE(pmaidb->xVal);
                break;

            default:
                assert(FALSE);
                break;
        }
    }

    vxsts=semGive(semMeaAi);
    assert(vxsts==OK);
}

/***********************************************************************
* VI_Search_Ref_Ch - 搜索参考通道(用于遥测)
*
* RETURNS: 无
*
*/
static void VI_Search_Ref_Ch(void)
{
    VI_MEA_AI_DB *pmaidb;
    VI_MEA_AI_DB *pmaidbM;
    float fRefModThsh=0.0;
    int iTempRef=-1;
    int iVldRef=-1;

    for (pmaidb=pmaidb_g; pmaidb<pmaidb_g+iMeaAiNum_g; pmaidb++)
    {
        if (pmaidb->ucArith==2 && (pmaidb->pelmSrc || pmaidb->pvAiHnd))
        {
            if (iRefAngCh_g==-2)
            {
                /* Search module MEA_AI config of the same channel. */
                for (pmaidbM=pmaidb_g; pmaidbM<pmaidb_g+iMeaAiNum_g; pmaidbM++)
                {
                    if (pmaidbM->ucArith==1 && pmaidbM->pelmSrc==pmaidb->pelmSrc &&
                            pmaidbM->pvAiHnd==pmaidb->pvAiHnd)
                    {
                        pmaidb->fAngVldMod=fabs(pmaicfg_g[pmaidbM-pmaidb_g].fOvMax)*ANG_VALID_COFF;
                        break;
                    }
                }
                assert(pmaidbM<pmaidb_g+iMeaAiNum_g);
            }

            iTempRef=pmaidb-pmaidb_g;
            fRefModThsh=pmaidb->fAngVldMod*MIN_REF_MOD_COFF/ANG_VALID_COFF;
            if (iVldRef==-1 && REAL(pmaidb->xVal)>fRefModThsh)
            {
                /* Find suitable. */
                iVldRef=iTempRef;
                fMinRefMod_g=fRefModThsh;
            }
        }
    }

    if (iVldRef!=-1)                    /* Find suitable. */
        iRefAngCh_g=iVldRef;
    else                                /* Not find suitable, use the last. */
    {
        iRefAngCh_g=iTempRef;
        fMinRefMod_g=fRefModThsh;
    }
}

/***********************************************************************
* VI_Rd_Mea_DI_Val - Read all measurement DIs' value.
*
* RETURNS: 无
*
* alert:
* 		pbRslt must contains space to save iMeaDiNum_g BOOL numbers.
*/
void VI_Rd_Mea_DI_Val(
    BOOL *pbRslt,		/* to save all MEA_DIs' current value. */
    uint16_t *pQuality
)
{
    VI_MEA_DI_DB *pmdidb;
    STATUS vxsts;

    vxsts=taskLock();
    assert(vxsts==OK);

    for (pmdidb=pmdidb_g; pmdidb<pmdidb_g+iMeaDiNum_g; pmdidb++)
    {
        if (pmdidb->pvDiHnd)
        {
            *pbRslt++=RD_Get_DI(pmdidb->pvDiHnd);
            *pQuality++ = RD_Get_DI_Quality(pmdidb->pvDiHnd);
        }
        else
        {
            *pbRslt++=pmdidb->bVal;
            *pQuality++ = pmdidb->usQuality;
        }
    }

    vxsts=taskUnlock();
    assert(vxsts==OK);
}

/***********************************************************************
* VI_Rd_Run_Info - Read running information according to the index.
*
* RETURNS: Pointer to the running information.
*                 or NULL if the index is out of buffer.
*
* Alert:
* 		The u_int index is a serial number begin from 0 and increase one
*      for every new information. If not dealing with the infomation for
*      long time, the information mapping with uiIdx may be not valid.
*      Using of count semorpthore is commended to keep uiIdx syn. between
*      core and applications.
*      The return value is a const pointer before, beccause of must modifying the SN,
*      so changing the attribute.
*/
VI_RUN_INFO *VI_Rd_Run_Info(
    u_int uiIdx			/* index of the information. */
)
{
    int iPast;
    int iLockKey;
    VI_RUN_INFO *pinf;

    iLockKey=intLock();

    iPast=uiCurInfIdx_g-uiIdx;			/* 剩余条数 */

    if (iPast<=0 || iPast>SZ_VALID_INFO_BUF)		/* 张云修改,防止访问无效数据 */
    {
        static BOOL bWrLogFlag = FALSE;

        intUnlock(iLockKey);

        if (!bWrLogFlag)
        {
            bWrLogFlag = TRUE;
            LOG_Write(LOG_KERNEL, "事件队列满,当前事件无效.\n", NULL);
        }

        return NULL;
    }

    if (uiCurInfPos_g >= iPast)			/* 现有的还没有读完 */
        pinf=arinf_g+(uiCurInfPos_g-iPast);
    else
        pinf=arinf_g+(SZ_RUN_INFO_BUF+uiCurInfPos_g-iPast);			/* 从后面开始 */

    intUnlock(iLockKey);

    return pinf;
}

/***********************************************************************
* VI_Bgn_Fault - Set fault begin flag.
*
* RETURNS: 无
*
*/
void VI_Bgn_Fault(
    uint32_t ulAiCnt	,uint16_t uForwordTime	/* AI count of this time logic scanning. */
)
{
    VI_RUN_INFO *pinf;
    STATUS vxsts;

    vxsts=taskLock();
    assert(vxsts==OK);

    if (!rptsts_g.iFault++)
    {
        /* 故障累加，动作数加1 */
        rptsts_g.ulEvtBgnCnt=ulAiCnt-uForwordTime;

        rptsts_g.ulFaultBeginTimeUs=RD_AI_Cnt_To_us(ulAiCnt);		/* 当前采样节拍转换为时间 */
        rptsts_g.ulFaultBeginTimeUs-= uForwordTime*rdinfo_g.uiSmplPeriod;

        pinf=VI_Run_Info_Wr_P();

        if(bViewModIsInit_g)
        {
            pinf->bViewModIsInit=TRUE;
        }
        else
        {
            pinf->bViewModIsInit=FALSE;
        }

        pinf->type=FAULT_STS;

        pinf->msg.fault.unRptSN=rptsts_g.unRptSN;
        pinf->msg.fault.ucRecSN=rptsts_g.ucRecSN;
        /* pinf->msg.fault.ulTime=TM_Get_usCnt(); */ /*原来的实现屏蔽,用当前实际时间  */
        pinf->msg.fault.ulTime=rptsts_g.ulFaultBeginTimeUs;		/* 数据窗时间 */
        pinf->msg.fault.bSts=TRUE;

        VI_End_Wr_Run_Info();
    }

    vxsts=taskUnlock();
    assert(vxsts==OK);
}

/***********************************************************************
* VI_End_Fault - Set fault end flag.
*
* RETURNS: 无
*
*/
void VI_End_Fault(
    uint32_t ulAiCnt		/* AI count of this time logic scanning. */
)
{
    VI_RUN_INFO *pinf;
    uint16_t unRptSN;
    uint8_t ucRecSN;

    taskLock();

    if (!--rptsts_g.iFault)
    {
        /* 故障数递减，动作数减1 ，加减成对出现 */
        RC_End_Wave(ulAiCnt);

        unRptSN=rptsts_g.unRptSN;
        ucRecSN=rptsts_g.ucRecSN;

        pinf=VI_Run_Info_Wr_P();

        if(bViewModIsInit_g)
        {
            pinf->bViewModIsInit=TRUE;
        }
        else
        {
            pinf->bViewModIsInit=FALSE;
        }

        pinf->type=FAULT_STS;

        pinf->msg.fault.unRptSN=unRptSN;
        pinf->msg.fault.ucRecSN=ucRecSN;
        /* pinf->msg.fault.ulTime=TM_Get_usCnt(); */	/* 原来实现屏蔽,用当前实际时间 */
        pinf->msg.fault.ulTime=RD_AI_Cnt_To_us(ulAiCnt); /* 使用数据窗时间 */
        pinf->msg.fault.bSts=FALSE;

        VI_End_Wr_Run_Info();
        /* 张云修改过 */
        rptsts_g.unRptSN++;			/* 这里报告号加1，导致报告号不连续 */
        rptsts_g.ucRecSN=0;

    }

    taskUnlock();
}

/***********************************************************************
* VI_Make_Rpt - The Entry of report recording task
*
* RETURNS: None
*
*/
static int VI_Make_Rpt(
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
    /* Only the event , SOE, and protect start/stop information wil be recorded in the event report, and the error hint will not be recorded. */
    STATUS vxsts;
    u_int uiInfIdx;
    VI_RUN_INFO *pinf;		/* Can be modified. */
    BOOL bInFault;
    int iFd;
    uint32_t ulEvtMsgLen;
    u_int uiEvtNum;
    EP_DATE_TIME dttm;
    EP_STATUS sts;
    /* To avoid date-time changing while recording on report, only begin us is
     * directly convented to date-time, others use delta us for caculating. */
    EP_DATE_TIME dttmBgn;
    uint32_t ulBgnTime;
    int  nWrCnt;
    int16_t unForeRptSN = 0;				/* The foregoing SN. */
    BOOL bFstRptSNFlag=TRUE;		/* The first time to receive the events created before the event initilization. */

    iFd=-1;
    bInFault=FALSE;

    /* Initialize them only to avoid compiler warning. */
    ulEvtMsgLen=0;
    uiEvtNum=0;
    ulBgnTime=0;

    while (1)
    {
        /* Waiting for the time adjustment. */

#if defined(EDP_01_02_BUILD)
        if(GetAdjustTimeSuccessFlag()) 				/* Must get the time adjustment flag, otherwise do not record the event. */
#elif defined(EDP03_BUILD) || defined(EXCITE_BUILD)
        if(TRUE)			/* adjust the time in local CPU, so record the event directly. */
#endif

        {
            /* If the time adjustment finished, the event will be recorded, otherwise not. */
            for (uiInfIdx=0; TRUE; uiInfIdx++)
            {
                vxsts=semTake(semNewInfRpt_g, WAIT_FOREVER);
                assert(vxsts==OK);

                while(ulInfNonComleteCnt_g!=0)
                {
                    /*若还有新消息未完全写入消息队列，则等待100ms 2010-4-26 ZY  */
                    taskDelay(10);
                }

                pinf=VI_Rd_Run_Info(uiInfIdx);

                if (pinf==NULL)
                {
                    static uint32_t ulCnt=0;
                    if(ulCnt%2000==0)
                    {
                        LOG_Dbg_Msg("ERROR: Event infomation buffer overflow.\n", 0, 0, 0, 0, 0, 0);
                    }
                    ulCnt++;

                    /* TODO: write message to log file. */

                    /* assert(FALSE);*/
                    continue;
                }
                
                if(pinf->bViewModIsInit && (!rptsts_g.bRptSNProcessFinish))
                {
                    /* The event module have been intializated. */
                    rptsts_g.bRptSNProcessFinish=TRUE;

                }

                if(((rptsts_g.ulRptNumBeforeEventInit-1) == uiInfIdx) && (!rptsts_g.bRptSNProcessFinish))
                {
                    rptsts_g.bRptSNProcessFinish=TRUE;
                }



                if (pinf->type==FAULT_STS)
                {
                    /* 故障态 */
                    if(!(pinf->bViewModIsInit))
                    {
                        /* 若该事件产生时，事件模块还没有初始化，则此时事件报告的报告号不对，则不记录事件报告，否则报告号会乱掉 */
                        vxsts=taskLock();
                        assert(vxsts==OK);

                        if(bFstRptSNFlag)
                        {
                            /* The first time. */
                            bFstRptSNFlag=FALSE;
                            unForeRptSN=pinf->msg.fault.unRptSN;		/* The foregoing SN. */
                            pinf->msg.fault.unRptSN=rptsts_g.unRptSNReserved++;		/* SN increased. */
                        }
                        else
                        {
                            /* Not the first time. */
                            if(pinf->msg.fault.unRptSN == unForeRptSN)
                            {
                                /* 和前一报告相同 */
                                pinf->msg.fault.unRptSN=rptsts_g.unRptSNReserved-1;		/* Not increated. */
                            }
                            else
                            {
                                unForeRptSN=pinf->msg.fault.unRptSN;
                                pinf->msg.fault.unRptSN=rptsts_g.unRptSNReserved++;			/* Use the new serial number. */
                            }
                        }

                        vxsts=taskUnlock();
                        assert(vxsts==OK);
                    }
                    if (pinf->msg.fault.bSts)
                    {
                        /* 动作 */
                        if(bInFault)
                        {
                            /* 说明消息队列有可能丢失上次故障返回消息，上次故障动作还没有处理完 */
                            if(iFd>=0)
                            {
                                /*关闭当前文件  */
                                VI_MakeEvtFileTaskExecHandle(iFd, 0);
                                iFd=-1;
                            }
                        }
                        bInFault=TRUE;

                        ulEvtMsgLen=6;		/* 事件长度 */
                        uiEvtNum=0;		/* 事件条数 */

                        ulBgnTime=pinf->msg.fault.ulTime;
                        sts=TM_To_Dttm(ulBgnTime, &dttmBgn);
                        if (sts!=EP_SUCCESS)
                            dttmBgn.ucHour |= 0x80;		/* 出错标志 */

                        iFd=VI_Bgn_Rpt(pinf->msg.fault.unRptSN, &dttmBgn);
                        if(!(iFd>=0))
                        {
                            continue;
                        }
                    }
                    else
                    {
                        /* 返回 */
                        if(!(bInFault))
                        {
                            /* 说明消息队列有可能丢失上次故障启动消息，返回的上次应该是动作 */
                            if(iFd>=0)
                            {
                                /* 关闭当前文件,结束此次处理 */
                                VI_MakeEvtFileTaskExecHandle(iFd, 1);
                                iFd=-1;
                                bInFault=FALSE;
                                continue;
                            }
                        }
                        bInFault=FALSE;

                        VI_End_Rpt(iFd, ulEvtMsgLen, uiEvtNum);		/* 如果返回丢失，则形不成有效报告 */
                    }

                    /* TODO: if need record fault begin/end event? */
                }
                else if (pinf->type==NEW_EVT)
                {
                    /* 新的事件 */
                    if(!(pinf->bViewModIsInit))
                    {
                        /* 若该事件产生时，事件模块还没有初始化，则此时事件报告的报告号不对，则不记录事件报告，否则报告号会乱掉 */
                        vxsts=taskLock();
                        assert(vxsts==OK);

                        if(bFstRptSNFlag)
                        {
                            /* The first time. */
                            bFstRptSNFlag=FALSE;
                            unForeRptSN=pinf->msg.evt.unRptSN;
                            pinf->msg.evt.unRptSN=rptsts_g.unRptSNReserved++;		/* SN increased. */
                        }
                        else
                        {
                            /* Not the first time. */
                            if(pinf->msg.evt.unRptSN == unForeRptSN)
                            {
                                /* 和前一报告相同 */
                                pinf->msg.evt.unRptSN=rptsts_g.unRptSNReserved-1;
                            }
                            else
                            {
                                unForeRptSN=pinf->msg.evt.unRptSN;	/* Update the foregong SN. */
                                pinf->msg.evt.unRptSN=rptsts_g.unRptSNReserved++;			/* Use the new serial number. */
                            }
                        }

                        vxsts=taskUnlock();
                        assert(vxsts==OK);
                    }
                    if (!bInFault)
                    {
                        /* 不是故障态，形成新的报告 */
                        sts=TM_To_Dttm(pinf->msg.evt.ulTime, &dttm);
                        if (sts!=EP_SUCCESS)
                            dttm.ucHour |= 0x80;

                        iFd=VI_Bgn_Rpt(pinf->msg.evt.unRptSN, &dttm);
                        if(!(iFd>=0))
                        {
                            continue;
                        }
                        nWrCnt=VI_Wr_Evt_Rpt(iFd, &pinf->msg.evt, &dttm);
                        if(nWrCnt<0)
                        {
                            VI_MakeEvtFileTaskExecHandle(iFd, 2);
                            iFd=-1;
                            continue;
                        }
                        ulEvtMsgLen=6+nWrCnt;
                        VI_End_Rpt(iFd, ulEvtMsgLen, 1);
                    }
                    else
                    {
                        sts=TM_To_Dttm(pinf->msg.evt.ulTime, &dttm);
                        if (sts != EP_SUCCESS)
                            dttm.ucHour |= 0x80;
#if 0
                        if (dttmBgn.ucHour & 0x80)
                        {
                            dttmBgn.ucHour &= 0x7F;		/* 防止出错，意义不大 */

                            sts=TM_Calc_Time(&dttmBgn, &dttm,
                                             pinf->msg.evt.ulTime-ulBgnTime, 0);
                            assert(sts==EP_SUCCESS);

                            dttmBgn.ucHour |= 0x80;
                            dttm.ucHour |= 0x80;
                        }
                        else
                        {
                            sts=TM_Calc_Time(&dttmBgn, &dttm,
                                             pinf->msg.evt.ulTime-ulBgnTime, 0);
                            assert(sts==EP_SUCCESS);
                        }
#endif

                        if(!(iFd>=0))
                        {
                            continue;
                        }
                        nWrCnt=VI_Wr_Evt_Rpt(iFd, &pinf->msg.evt, &dttm);
                        if(nWrCnt<0)
                        {
                            VI_MakeEvtFileTaskExecHandle(iFd, 3);
                            iFd=-1;
                            continue;
                        }
                        ulEvtMsgLen+=nWrCnt;
                        uiEvtNum++;
                    }
                }
                else if (pinf->type==NEW_SOE)
                {
                    /* SOE */
                    if(!(pinf->bViewModIsInit))
                    {
                        /* 若该事件产生时，事件模块还没有初始化，则此时事件报告的报告号不对，则不记录事件报告，否则报告号会乱掉 */
                        vxsts=taskLock();
                        assert(vxsts==OK);

                        if(bFstRptSNFlag)
                        {
                            /* The first time. */
                            bFstRptSNFlag=FALSE;
                            unForeRptSN=pinf->msg.soe.unRptSN;
                            pinf->msg.soe.unRptSN=rptsts_g.unRptSNReserved++;		/* SN increased. */
                        }
                        else
                        {
                            /* Not the first time. */
                            if(pinf->msg.soe.unRptSN == unForeRptSN)
                            {
                                /* 和前一报告相同 */
                                pinf->msg.soe.unRptSN=rptsts_g.unRptSNReserved-1;
                            }
                            else
                            {
                                unForeRptSN=pinf->msg.soe.unRptSN;
                                pinf->msg.soe.unRptSN=rptsts_g.unRptSNReserved++;			/* Use the new serial number. */
                            }
                        }

                        vxsts=taskUnlock();
                        assert(vxsts==OK);
                    }
                    if (!bInFault)
                    {
                        /* 不处于故障态 */
                        ulEvtMsgLen=6;
                        uiEvtNum=0;

                        if(pinf->msg.soe.ullusCntFrom1970 != 0)
                        {
                            US_CNT_UTC_TIME usUTCtmTmp;
                            usUTCtmTmp.ullusCntFrom1970 = pinf->msg.soe.ullusCntFrom1970;
                            Us_UTC_Time_To_Dttm(&usUTCtmTmp, &dttm);
                        }
                        else
                        {
                            sts=TM_To_Dttm(pinf->msg.soe.ulTime, &dttm);

                            if (sts!=EP_SUCCESS)
                                dttm.ucHour |= 0x80;
                        }

                        iFd=VI_Bgn_Rpt(pinf->msg.soe.unRptSN, &dttm);
                        if(!(iFd>=0))
                        {
                            continue;
                        }
                        nWrCnt=VI_Wr_Soe_Rpt(iFd, &pinf->msg.soe, &dttm);
                        if(nWrCnt<0)
                        {
                            VI_MakeEvtFileTaskExecHandle(iFd, 4);
                            iFd=-1;
                            continue;
                        }
                        ulEvtMsgLen=6+nWrCnt;

                        VI_End_Rpt(iFd, ulEvtMsgLen, 1);
                    }
                    else
                    {
                        if(pinf->msg.soe.ullusCntFrom1970 != 0)
                        {
                            US_CNT_UTC_TIME usUTCtmTmp;
                            usUTCtmTmp.ullusCntFrom1970 = pinf->msg.soe.ullusCntFrom1970;
                            Us_UTC_Time_To_Dttm(&usUTCtmTmp, &dttm);
                        }
                        else
                        {
                            sts=TM_To_Dttm(pinf->msg.soe.ulTime, &dttm);

                            if (sts!=EP_SUCCESS)
                                dttm.ucHour |= 0x80;
                        }
#if 0
                        if (dttmBgn.ucHour & 0x80)
                        {
                            dttmBgn.ucHour &= 0x7F;

                            sts=TM_Calc_Time(&dttmBgn, &dttm,
                                             pinf->msg.soe.ulTime-ulBgnTime, 0);
                            assert(sts==EP_SUCCESS);

                            dttmBgn.ucHour |= 0x80;
                            dttm.ucHour |= 0x80;
                        }
                        else
                        {
                            sts=TM_Calc_Time(&dttmBgn, &dttm,
                                             pinf->msg.soe.ulTime-ulBgnTime, 0);
                            assert(sts==EP_SUCCESS);
                        }
#endif

                        if(!(iFd>=0))
                        {
                            continue;
                        }
                        nWrCnt=VI_Wr_Soe_Rpt(iFd, &pinf->msg.soe, &dttm);
                        if(nWrCnt<0)
                        {
                            VI_MakeEvtFileTaskExecHandle(iFd, 5);
                            iFd=-1;
                            continue;
                        }
                        ulEvtMsgLen+=nWrCnt;
                        uiEvtNum++;
                    }
                }
                else if (pinf->type==ERR_OCR)
                {
                    /* 2006-11-2日张云修改，支持异常事件记录 */
                    if(!(pinf->bViewModIsInit))
                    {
                        /* 若该事件产生时，事件模块还没有初始化，则此时事件报告的报告号不对，则不记录事件报告，否则报告号会乱掉 */
                        vxsts=taskLock();
                        assert(vxsts==OK);

                        if(bFstRptSNFlag)
                        {
                            /* The first time. */
                            bFstRptSNFlag=FALSE;
                            unForeRptSN=pinf->msg.err.unRptSN;
                            pinf->msg.err.unRptSN=rptsts_g.unRptSNReserved++;		/* SN increased. */
                        }
                        else
                        {
                            /* Not the first time. */
                            if(pinf->msg.err.unRptSN == unForeRptSN)
                            {
                                /* 和前一报告相同 */
                                pinf->msg.err.unRptSN=rptsts_g.unRptSNReserved-1;
                            }
                            else
                            {
                                unForeRptSN=pinf->msg.err.unRptSN;
                                pinf->msg.err.unRptSN=rptsts_g.unRptSNReserved++;			/* Use the new serial number. */
                            }
                        }

                        vxsts=taskUnlock();
                        assert(vxsts==OK);
                    }
                    if (!bInFault)
                    {
                        sts=TM_To_Dttm(pinf->msg.err.ulTime, &dttm);
                        if (sts!=EP_SUCCESS)
                            dttm.ucHour |= 0x80;

                        iFd=VI_Bgn_Rpt(pinf->msg.err.unRptSN, &dttm);
                        if(!(iFd>=0))
                        {
                            continue;
                        }
                        nWrCnt=VI_Wr_Err_Rpt(iFd, &pinf->msg.err, &dttm);
                        if(nWrCnt<0)
                        {
                            VI_MakeEvtFileTaskExecHandle(iFd, 6);
                            iFd=-1;
                            continue;
                        }
                        ulEvtMsgLen=6+nWrCnt;
                        VI_End_Rpt(iFd, ulEvtMsgLen, 1);
                    }
                    else
                    {
                        sts=TM_To_Dttm(pinf->msg.err.ulTime, &dttm);
                        if (sts != EP_SUCCESS)
                            dttm.ucHour |= 0x80;
#if 0
                        if (dttmBgn.ucHour & 0x80)
                        {
                            dttmBgn.ucHour &= 0x7F;

                            sts=TM_Calc_Time(&dttmBgn, &dttm,
                                             pinf->msg.err.ulTime-ulBgnTime, 0);
                            assert(sts==EP_SUCCESS);

                            dttmBgn.ucHour |= 0x80;
                            dttm.ucHour |= 0x80;
                        }
                        else
                        {
                            sts=TM_Calc_Time(&dttmBgn, &dttm,
                                             pinf->msg.err.ulTime-ulBgnTime, 0);
                            assert(sts==EP_SUCCESS);
                        }
#endif

                        if(!(iFd>=0))
                        {
                            continue;
                        }
                        nWrCnt=VI_Wr_Err_Rpt(iFd, &pinf->msg.err, &dttm);
                        if(nWrCnt<0)
                        {
                            VI_MakeEvtFileTaskExecHandle(iFd, 7);
                            iFd=-1;
                            continue;
                        }
                        ulEvtMsgLen+=nWrCnt;
                        uiEvtNum++;
                    }
                }
                else if(pinf->type == ERR_OCR_STAT)
                {
                    /* 2006-11-2日张云修改，支持异常事件记录 */
                    if(!(pinf->bViewModIsInit))
                    {
                        /* 若该事件产生时，事件模块还没有初始化，则此时事件报告的报告号不对，则不记录事件报告，否则报告号会乱掉 */
                        vxsts=taskLock();
                        assert(vxsts==OK);

                        if(bFstRptSNFlag)
                        {
                            /* The first time. */
                            bFstRptSNFlag=FALSE;
                            unForeRptSN=pinf->msg.errstat.unRptSN;
                            pinf->msg.errstat.unRptSN=rptsts_g.unRptSNReserved++;		/* SN increased. */
                        }
                        else
                        {
                            /* Not the first time. */
                            if(pinf->msg.errstat.unRptSN == unForeRptSN)
                            {
                                /* 和前一报告相同 */
                                pinf->msg.errstat.unRptSN=rptsts_g.unRptSNReserved-1;
                            }
                            else
                            {
                                unForeRptSN=pinf->msg.errstat.unRptSN;
                                pinf->msg.errstat.unRptSN=rptsts_g.unRptSNReserved++;			/* Use the new serial number. */
                            }
                        }

                        vxsts=taskUnlock();
                        assert(vxsts==OK);
                    }
                    if (!bInFault)
                    {
                        sts=TM_To_Dttm(pinf->msg.errstat.ulTime, &dttm);
                        if (sts!=EP_SUCCESS)
                            dttm.ucHour |= 0x80;

                        iFd=VI_Bgn_Rpt(pinf->msg.errstat.unRptSN, &dttm);
                        if(!(iFd>=0))
                        {
                            continue;
                        }
                        nWrCnt=VI_Wr_Err_Stat_Rpt(iFd, &pinf->msg.errstat, &dttm);
                        if(nWrCnt<0)
                        {
                            VI_MakeEvtFileTaskExecHandle(iFd, 8);
                            iFd=-1;
                            continue;
                        }
                        ulEvtMsgLen=6+nWrCnt;
                        VI_End_Rpt(iFd, ulEvtMsgLen, 1);
                    }
                    else
                    {
                        sts=TM_To_Dttm(pinf->msg.errstat.ulTime, &dttm);
                        if (sts != EP_SUCCESS)
                            dttm.ucHour |= 0x80;
#if 0
                        if (dttmBgn.ucHour & 0x80)
                        {
                            dttmBgn.ucHour &= 0x7F;

                            sts=TM_Calc_Time(&dttmBgn, &dttm,
                                             pinf->msg.errstat.ulTime-ulBgnTime, 0);
                            assert(sts==EP_SUCCESS);

                            dttmBgn.ucHour |= 0x80;
                            dttm.ucHour |= 0x80;
                        }
                        else
                        {
                            sts=TM_Calc_Time(&dttmBgn, &dttm,
                                             pinf->msg.errstat.ulTime-ulBgnTime, 0);
                            assert(sts==EP_SUCCESS);
                        }
#endif

                        if(!(iFd>=0))
                        {
                            continue;
                        }
                        nWrCnt=VI_Wr_Err_Stat_Rpt(iFd, &pinf->msg.errstat, &dttm);
                        if(nWrCnt<0)
                        {
                            VI_MakeEvtFileTaskExecHandle(iFd, 9);
                            iFd=-1;
                            continue;
                        }
                        ulEvtMsgLen+=nWrCnt;
                        uiEvtNum++;
                    }
                }
            }
        }
        else
        {
            /* 若还没有对时,则等待30秒 */
            taskDelay(SYS_SEC*30);
        }
    }

}





/***********************************************************************
* VI_Bgn_Rpt - 开始报告写入
*
* RETURNS: 无
*
*/
static int VI_Bgn_Rpt(
    uint16_t unRptSN,
    const EP_DATE_TIME *pdttm
)
{
    static uint8_t aucBuf[FULL_NAME_LEN+1];
    int iFd;
    int i;
    u_int uiTemp;

    if(ulCurEvtFileRptNum_g==unRptSN)
    {
        /* 防止因为故障返回事件丢失,导致同一报告号报告被多次创建，报告号在返回事件中递增 */
        return  -1;
    }
    /*g_bErrBgnAfterLs = FALSE;*/
    bCurRptHaveGetFirstEvtFlg_g=FALSE;	/* 设置未获得第1个事件标志 */
    CurEvtRptInfo_g.bCurRptIsFastDelFlg=TRUE;		/* 设置快速删除标志初始值为真*/
    CurEvtRptInfo_g.tmRptFaultTime=*pdttm;				/* 获得报告故障发生的时间 */
    CurEvtRptInfo_g.uiCurRptNum=unRptSN;

    ulCurEvtFileRptNum_g=unRptSN;	/* 张云添加,保存当前最新文件的报告号 */

    rptsts_g.bEvtWrFileOn=TRUE;		/* 创建新的事件文件 */

    sprintf(aucBuf, EP_EVT_RPT_DIR "/edp%04x.evt", unRptSN);
    iFd=creatInDataDisk(aucBuf, O_RDWR);
    if(iFd==ERROR)
    {
        static uint32_t ulMsgCnt=0;
        if (FT_Is_File(aucBuf))
        {
            remove(aucBuf);
        }

        ulMsgCnt++;
        if(ulMsgCnt%200==1)
        {
            logMsg("WARNING,Create  New  Event Report  File  failure  for  busy  or  error,FileName  is  %s!\n",
                   (int)aucBuf,0,0,0,0,0);
            if(ENG_MODE ==1)
                LOG_Write(LOG_KERNEL, "WARNING,Create New  Event Report  File  failure  for  busy  or  error!\n", NULL);
            else if(ENG_MODE ==0)
                LOG_Write(LOG_KERNEL, "警告,创建新的事件报告文件失败!\n", NULL);
        }

        return  iFd;
    }

    memset(aucBuf, 0, sizeof(aucBuf));

    aucBuf[0]=0x77;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x88;
    aucBuf[4]=LO8(GetInnerProtocolVer());
    aucBuf[5]=HI8(GetInnerProtocolVer());
    aucBuf[8]=pdttm->ucIrigbLSFlag;     /*时间闰秒属性*/
    /*if((pdttm->ucIrigbLSFlag & IRIGB_AFTER_PLS_0SEC) == IRIGB_AFTER_PLS_0SEC)
    {
        g_bErrBgnAfterLs = TRUE;
    }*/
    aucBuf[9]=LO8(unRptSN);
    aucBuf[10]=HI8(unRptSN);

    aucBuf[11]=LO8(pdttm->unYear);
    aucBuf[12]=HI8(pdttm->unYear);
    aucBuf[13]=pdttm->ucMonth;
    aucBuf[14]=pdttm->ucDate;
    aucBuf[15]=pdttm->ucHour;
    aucBuf[16]=pdttm->ucMinute;

    uiTemp=pdttm->ucSec*1000+pdttm->unMSEL;
    aucBuf[17]=LO8(uiTemp);
    aucBuf[18]=HI8(uiTemp);

    aucBuf[19]=LO8(pdttm->unMicroSec);
    aucBuf[20]=HI8(pdttm->unMicroSec);

    aucBuf[21]=(sizeof(EP_WAVE_REC_DIR)-1)+12;

    sprintf(aucBuf+22, EP_WAVE_REC_DIR "/edp%04x.frw", unRptSN);

    /* 10 bytes of event message length/event number/reserved are 0 now. */

    i=writeInDataDisk(iFd, aucBuf, 22+(sizeof(EP_WAVE_REC_DIR)-1)+12+10);
    if(!(i==22+(sizeof(EP_WAVE_REC_DIR)-1)+12+10))
    {
        /*若写入失败  */
        VI_MakeEvtFileTaskExecHandle(iFd, 10);
        return  -1;
    }

    return iFd;
}

/***********************************************************************
* VI_End_Rpt - 结束报告写入
*
* RETURNS: 无
*
*/
static void VI_End_Rpt(
    int iFd,
    uint32_t ulEvtMsgLen,
    u_int uiEvtNum
)
{
    int i;
    uint8_t aucBuf[6];
    STATUS vxsts;
    uint8_t aucTempNameBuf[FULL_NAME_LEN+1];
    uint8_t aucNameBuf[FULL_NAME_LEN+1];
    uint8_t aucFullNameBuf[FULL_NAME_LEN+1];
    uint8_t  ucDelAttr;
    uint32_t ulseclong = 0;

    if(!(iFd >= 0))
    {
        return  ;
    }

    i=lseek(iFd, 22+(sizeof(EP_WAVE_REC_DIR)-1)+12, SEEK_SET);
    if(!(i==22+(sizeof(EP_WAVE_REC_DIR)-1)+12))
    {
        VI_MakeEvtFileTaskExecHandle(iFd, 11);
        return  ;
    }

    U32_TO_BYTES(aucBuf, ulEvtMsgLen);
    aucBuf[4]=LO8(uiEvtNum);
    aucBuf[5]=HI8(uiEvtNum);

    i=writeInDataDisk(iFd, aucBuf, 6);
    if(!(i==6))
    {
        VI_MakeEvtFileTaskExecHandle(iFd, 11);
        return  ;
    }

    aucBuf[0]=0x7D;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x82;

    lseek(iFd, 0, SEEK_END);
    i=writeInDataDisk(iFd, aucBuf, 4);
    if(!(i==4))
    {
        VI_MakeEvtFileTaskExecHandle(iFd, 11);
        return  ;
    }

    vxsts=close(iFd);
    if(!(vxsts==OK))
    {
        VI_MakeEvtFileTaskExecHandle(iFd, 11);
        return  ;
    }
    /* 重命名事件报告文件名 */
    if(CurEvtRptInfo_g.bCurRptIsFastDelFlg)
    {
        /* 删除属性 */
        ucDelAttr=1;
    }
    else
    {
        ucDelAttr=0;
    }

    if((CurEvtRptInfo_g.tmRptFaultTime.ucIrigbLSFlag & IRIGB_PLS_60SEC) == IRIGB_PLS_60SEC)
    {
        ulseclong = TM_Time_To_Long(&CurEvtRptInfo_g.tmRptFaultTime);
        ulseclong = ulseclong- 1;
        TM_Long_To_Time(&CurEvtRptInfo_g.tmRptFaultTime, ulseclong);
        if(CurEvtRptInfo_g.tmRptFaultTime.ucSec == 59)
            CurEvtRptInfo_g.tmRptFaultTime.ucSec = 60;
    }

    sprintf(aucTempNameBuf, EP_EVT_RPT_DIR "/edp%04x.evt", CurEvtRptInfo_g.uiCurRptNum);
    sprintf(aucFullNameBuf, EP_EVT_RPT_DIR "/edp%04x%02x%02x%04x%04d%02d%02d%02d%02d%02d%03d.evt",
            CurEvtRptInfo_g.uiCurRptNum,
            ucDelAttr,
            CurEvtRptInfo_g.ucFstEvtType,
            CurEvtRptInfo_g.uiFstEvtCode,
            CurEvtRptInfo_g.tmRptFaultTime.unYear,
            CurEvtRptInfo_g.tmRptFaultTime.ucMonth,
            CurEvtRptInfo_g.tmRptFaultTime.ucDate,
            CurEvtRptInfo_g.tmRptFaultTime.ucHour,
            CurEvtRptInfo_g.tmRptFaultTime.ucMinute,
            CurEvtRptInfo_g.tmRptFaultTime.ucSec,
            CurEvtRptInfo_g.tmRptFaultTime.unMSEL);

    if (FT_Is_File(aucFullNameBuf))
    {
        LOG_Dbg_Msg("删除已有文件%s.\n", (int)aucFullNameBuf, 0, 0, 0, 0, 0);
        vxsts=FS_RemoveFile(aucFullNameBuf, EVT_DIR);
        if (vxsts == ERROR)
        {
            LOG_Dbg_Msg("删除同名事件文件失败!\n", 0, 0, 0, 0, 0, 0);
        }
    }

    EP_SetNewestSN(CurEvtRptInfo_g.uiCurRptNum);

    sprintf(aucNameBuf, "edp%04x%02x%02x%04x%04d%02d%02d%02d%02d%02d%03d.evt",
            CurEvtRptInfo_g.uiCurRptNum,
            ucDelAttr,
            CurEvtRptInfo_g.ucFstEvtType,
            CurEvtRptInfo_g.uiFstEvtCode,
            CurEvtRptInfo_g.tmRptFaultTime.unYear,
            CurEvtRptInfo_g.tmRptFaultTime.ucMonth,
            CurEvtRptInfo_g.tmRptFaultTime.ucDate,
            CurEvtRptInfo_g.tmRptFaultTime.ucHour,
            CurEvtRptInfo_g.tmRptFaultTime.ucMinute,
            CurEvtRptInfo_g.tmRptFaultTime.ucSec,
            CurEvtRptInfo_g.tmRptFaultTime.unMSEL
           );

    vxsts = FS_SearchInsertFile(aucTempNameBuf, aucNameBuf, EVT_DIR, ucDelAttr);		/* 将新生成的文件插入到列表中 */

    if(!(vxsts==OK))
    {
        /* 若重命名不成功,则删除当前文件 */
        static uint32_t ulMsgCnt=0;
        if (FT_Is_File(aucTempNameBuf))
        {
            vxsts=remove(aucTempNameBuf);
        }
        if (FT_Is_File(aucFullNameBuf))
        {
            vxsts=FS_RemoveFile(aucFullNameBuf, EVT_DIR);
        }

        ulMsgCnt++;
        if(ulMsgCnt%200==1)
        {
            logMsg("WARNING,Rename Event Report File failure for error!\n", 0, 0, 0, 0, 0, 0);

            if(ENG_MODE == 1)
                LOG_Write(LOG_KERNEL, "WARNING,Rename Event Report File failed !\n", NULL);
            else if(ENG_MODE ==0)
                LOG_Write(LOG_KERNEL, "事件报告重命名失败!\n", NULL);
        }
        return  ;
    }

    rptsts_g.bEvtWrFileOn=FALSE;		/* 生成新的事件文件 */

    VI_Rpt_Added();
}

/***********************************************************************
* VI_Wr_Evt_Rpt - 写事件报告
*
* RETURNS: 报告号
*
*/
static int VI_Wr_Evt_Rpt(
    int iFd,
    const VI_EVT_MSG *pevt,
    EP_DATE_TIME *pdttm
)
{
    uint8_t aucBuf[18+9*MAX_EVT_PARM_NUM];
    u_int uiTemp;
    int i;
    const VI_EVT_PARM *pparm;
    const VI_EVT_PARM_CFG *pparmcfg;
    uint8_t *puc;

    if(!(iFd>=0 && pevt && pdttm))
    {
        return   -1;
    }

    /*SYN_SetIrigBFlag(pevt->ulTime, pdttm);*/

    /*if(g_bErrBgnAfterLs)
    {
        pdttm->ucIrigbLSFlag |= IRIGB_AFTER_PLS_0SEC;
    }*/

    uiTemp=16+9*pevt->pcfg->ucParmNum;		/* 长度 */
    aucBuf[0]=LO8(uiTemp);
    aucBuf[1]=HI8(uiTemp);

    aucBuf[2]=LO8(pdttm->unYear);
    aucBuf[3]=HI8(pdttm->unYear);
    aucBuf[4]=pdttm->ucMonth;
    aucBuf[5]=pdttm->ucDate;
    aucBuf[6]=pdttm->ucHour;
    aucBuf[7]=pdttm->ucMinute;

    uiTemp=pdttm->ucSec*1000+pdttm->unMSEL;
    aucBuf[8]=LO8(uiTemp);
    aucBuf[9]=HI8(uiTemp);

    aucBuf[10]=LO8(pdttm->unMicroSec);
    aucBuf[11]=HI8(pdttm->unMicroSec);

    aucBuf[12]=pdttm->ucIrigbLSFlag;

    aucBuf[13]=pevt->bState?0:1;

    aucBuf[14]=LO8(pevt->pcfg->unCode);			/* 事件区分码 */
    aucBuf[15]=HI8(pevt->pcfg->unCode);

    aucBuf[16]=pevt->pcfg->ucType;		/* 类型 */

    aucBuf[17]=pevt->pcfg->ucParmNum;
    if(aucBuf[17]>MAX_EVT_PARM_NUM)
    {
        /* 若是无效数据则失败 */
        return   -1;
    }

    for (puc=aucBuf+18, pparm=pevt->aparm, pparmcfg=pevt->pcfg->aparmcfg;
            pparm<pevt->aparm+aucBuf[17]; puc+=9, pparm++, pparmcfg++)
    {
        puc[0]=pparmcfg->ucAttrib;
        FLT_TO_BYTES(puc+1, REAL(pparm->xVal));		/* 实部*/
        FLT_TO_BYTES(puc+5, IMAGE(pparm->xVal));			/* 虚部 */
    }

    assert(puc-aucBuf==18+9*pevt->pcfg->ucParmNum);

    i=writeInDataDisk(iFd, aucBuf, puc-aucBuf);
    if(!(i==puc-aucBuf))
    {
        return  -1;
    }

    if(!bCurRptHaveGetFirstEvtFlg_g)
    {
        /* 获得第1个事件的信息 */

        CurEvtRptInfo_g.ucFstEvtType=pevt->pcfg->ucType;
        CurEvtRptInfo_g.uiFstEvtCode=pevt->pcfg->unCode;

        if((pevt->pcfg->ucType!=32)&&(pevt->pcfg->ucType!=33))
        {
            bCurRptHaveGetFirstEvtFlg_g=TRUE;
        }
    }
    if(!(pevt->pcfg->bNeedFastDel))
    {
        /* 若是事件为慢速删除,则设置快速删除标志为假,SOE变位都是快速删除事件 */
        CurEvtRptInfo_g.bCurRptIsFastDelFlg=FALSE;
    }

    return i;
}

/***********************************************************************
* VI_Wr_Err_Rpt - 写异常事件报告
*
* RETURNS: 报告号
*
*/
static int VI_Wr_Err_Rpt(
    int iFd,
    const VI_ERR_MSG *perr,
    EP_DATE_TIME *pdttm
)
{
    /* 装置异常告警事件记录 */
    uint8_t aucBuf[18+9*20];		/* 允许足够长字符串 */
    u_int uiTemp;
    int i;
    uint8_t *puc;
    int iErrMsgLen;
    int iErrMsgParaNum;

    if(!(iFd>=0 && perr && pdttm))
    {
        return   -1;
    }

    /*SYN_SetIrigBFlag(perr->ulTime, pdttm);*/

    iErrMsgLen=strlen(perr->aucNote);
    if(iErrMsgLen>=128)
    {
        iErrMsgLen=127;
    }
    iErrMsgParaNum=(iErrMsgLen+7)/8;


    uiTemp=16+9*iErrMsgParaNum;
    aucBuf[0]=LO8(uiTemp);
    aucBuf[1]=HI8(uiTemp);

    aucBuf[2]=LO8(pdttm->unYear);
    aucBuf[3]=HI8(pdttm->unYear);
    aucBuf[4]=pdttm->ucMonth;
    aucBuf[5]=pdttm->ucDate;
    aucBuf[6]=pdttm->ucHour;
    aucBuf[7]=pdttm->ucMinute;

    uiTemp=pdttm->ucSec*1000+pdttm->unMSEL;
    aucBuf[8]=LO8(uiTemp);
    aucBuf[9]=HI8(uiTemp);

    aucBuf[10]=LO8(pdttm->unMicroSec);
    aucBuf[11]=HI8(pdttm->unMicroSec);

    aucBuf[12]=pdttm->ucIrigbLSFlag;;

    aucBuf[13]=0;		/* 异常事件状态为动作 */

    aucBuf[14]=LO8(perr->unErrCode);			/* 事件区分码为异常代码 */
    aucBuf[15]=HI8(perr->unErrCode);

    aucBuf[16]=0x05;		/* 事件类型为装置告警 */

    aucBuf[17]=iErrMsgParaNum;
    if(aucBuf[17]>MAX_EVT_PARM_NUM)
    {
        /* 若是无效数据则失败 */
        return   -1;
    }
    /* 记录字符串 */
    puc=aucBuf+18;
    for(i=0; i<iErrMsgParaNum; i++)
    {
        *puc++=0x68;
        memcpy(puc,perr->aucNote+i*8,8);
        puc+=8;
    }

    assert(puc-aucBuf==18+9*iErrMsgParaNum);

    i=writeInDataDisk(iFd, aucBuf, puc-aucBuf);
    if(!(i==puc-aucBuf))
    {
        return  -1;
    }

    if(!bCurRptHaveGetFirstEvtFlg_g)
    {
        /* 获得第1个事件的信息 */
        bCurRptHaveGetFirstEvtFlg_g=TRUE;
        CurEvtRptInfo_g.ucFstEvtType=0x5;
        CurEvtRptInfo_g.uiFstEvtCode=perr->unErrCode;

        /* 异常事件限制触发次数, 且不返回;
         * 设置为慢速删除, 赋值一次即可
         */
        CurEvtRptInfo_g.bCurRptIsFastDelFlg = FALSE;
    }

    return i;
}

/***********************************************************************
* VI_Wr_Err_Stat_Rpt - 写异常事件报告(带状态)
*
* RETURNS: 报告号
*
*/
static int VI_Wr_Err_Stat_Rpt(
    int iFd,
    const VI_ERR_STAT_MSG *perr,
    EP_DATE_TIME *pdttm
)
{
    /* 装置异常告警事件记录 */
    uint8_t aucBuf[18+9*20];		/* 允许足够长字符串 */
    u_int uiTemp;
    int i;
    uint8_t *puc;
    int iErrMsgLen;
    int iErrMsgParaNum;

    if(!(iFd>=0 && perr && pdttm))
    {
        return   -1;
    }

    /*SYN_SetIrigBFlag(perr->ulTime, pdttm);*/

    iErrMsgLen=strlen(perr->aucNote);
    if(iErrMsgLen>=128)
    {
        iErrMsgLen=127;
    }
    iErrMsgParaNum=(iErrMsgLen+7)/8;


    uiTemp=16+9*iErrMsgParaNum;
    aucBuf[0]=LO8(uiTemp);
    aucBuf[1]=HI8(uiTemp);

    aucBuf[2]=LO8(pdttm->unYear);
    aucBuf[3]=HI8(pdttm->unYear);
    aucBuf[4]=pdttm->ucMonth;
    aucBuf[5]=pdttm->ucDate;
    aucBuf[6]=pdttm->ucHour;
    aucBuf[7]=pdttm->ucMinute;

    uiTemp=pdttm->ucSec*1000+pdttm->unMSEL;
    aucBuf[8]=LO8(uiTemp);
    aucBuf[9]=HI8(uiTemp);

    aucBuf[10]=LO8(pdttm->unMicroSec);
    aucBuf[11]=HI8(pdttm->unMicroSec);

    aucBuf[12]=pdttm->ucIrigbLSFlag;

    aucBuf[13]=perr->ulSts?0:1;		/* 异常事件状态为动作 */

    aucBuf[14]=LO8(perr->unErrCode);			/* 事件区分码为异常代码 */
    aucBuf[15]=HI8(perr->unErrCode);

    aucBuf[16]=0x05;		/* 事件类型为装置告警 */

    aucBuf[17]=iErrMsgParaNum;
    if(aucBuf[17]>MAX_EVT_PARM_NUM)
    {
        /* 若是无效数据则失败 */
        return   -1;
    }
    /* 记录字符串 */
    puc=aucBuf+18;
    for(i=0; i<iErrMsgParaNum; i++)
    {
        *puc++=0x68;
        memcpy(puc,perr->aucNote+i*8,8);
        puc+=8;
    }

    assert(puc-aucBuf==18+9*iErrMsgParaNum);

    i=writeInDataDisk(iFd, aucBuf, puc-aucBuf);
    if(!(i==puc-aucBuf))
    {
        return  -1;
    }

    if(!bCurRptHaveGetFirstEvtFlg_g)
    {
        /* 获得第1个事件的信息 */
        bCurRptHaveGetFirstEvtFlg_g=TRUE;
        CurEvtRptInfo_g.ucFstEvtType=0x5;
        CurEvtRptInfo_g.uiFstEvtCode=perr->unErrCode;

        /* 异常事件限制触发次数, 且不返回;
         * 设置为慢速删除, 赋值一次即可
         */
        CurEvtRptInfo_g.bCurRptIsFastDelFlg = FALSE;
    }

    return i;
}

/***********************************************************************
* VI_Wr_Soe_Rpt - 写SOE报告
*
* RETURNS: 写入字节数
*
*/
static int VI_Wr_Soe_Rpt(
    int iFd,
    const VI_SOE_MSG *psoe,
    EP_DATE_TIME *pdttm
)
{
    uint8_t aucBuf[18];
    u_int uiTemp;
    int i;

    if(!(iFd>=0 && psoe && pdttm))
    {
        return  -1;
    }

    /*SYN_SetIrigBFlag(psoe->ulTime, pdttm);*/

    aucBuf[0]=LO8(16);
    aucBuf[1]=HI8(16);

    aucBuf[2]=LO8(pdttm->unYear);
    aucBuf[3]=HI8(pdttm->unYear);
    aucBuf[4]=pdttm->ucMonth;
    aucBuf[5]=pdttm->ucDate;
    aucBuf[6]=pdttm->ucHour;
    aucBuf[7]=pdttm->ucMinute;

    uiTemp=pdttm->ucSec*1000+pdttm->unMSEL;
    aucBuf[8]=LO8(uiTemp);
    aucBuf[9]=HI8(uiTemp);

    aucBuf[10]=LO8(pdttm->unMicroSec);
    aucBuf[11]=HI8(pdttm->unMicroSec);

    aucBuf[12]=pdttm->ucIrigbLSFlag;

    aucBuf[13]=psoe->ucDIQ;

    aucBuf[14]=LO8(psoe->unCh);
    aucBuf[15]=HI8(psoe->unCh);

    aucBuf[16]=psoe->ucFtype;

    aucBuf[17]=0;

    i=writeInDataDisk(iFd, aucBuf, 18);
    if(!(i==18))
    {
        return  -1;
    }

    if(!bCurRptHaveGetFirstEvtFlg_g)
    {
        /* 获得第1个事件的信息 */
        bCurRptHaveGetFirstEvtFlg_g=TRUE;
        CurEvtRptInfo_g.ucFstEvtType=psoe->ucFtype;
        CurEvtRptInfo_g.uiFstEvtCode=psoe->unCh;
    }

    return i;
}

/* Get the max report SN.
 * Para:
 *     NONE.
 * Return:
 *     max report SN.
 */
static uint16_t VI_Init_Rpt_SN(void)
{
    BOOL bFind;
    uint16_t uiCurRpt;
    FILENODE *pFileNode;	/* 文件节点指针 */

    bFind=FALSE;
    uiCurRpt=0;

    semTake(semEvtFileListWR_g, WAIT_FOREVER); 		/* Waiting for semaphore. */

    pFileNode = (FILENODE *)lstFirst(pmEvtFileList_g);
    if (pFileNode)
    {
        uiCurRpt = pFileNode->uiFileSN;	/* 获得最新报告号 */
        bFind = TRUE;
    }

    semGive(semEvtFileListWR_g);

    semTake(semRecFileListWR_g, WAIT_FOREVER); 		/* Waiting for semaphore. */

    pFileNode = (FILENODE *)lstFirst(pmRecFileList_g);
    if (pFileNode)
    {
        if (!bFind)
        {
            uiCurRpt = pFileNode->uiFileSN;	/* 获得最新报告号 */
        }
        else if ((int16_t)(pFileNode->uiFileSN - uiCurRpt) >= 0)
        {
            uiCurRpt = pFileNode->uiFileSN;	/* 获得最新报告号 */
        }
    }

    semGive(semRecFileListWR_g);

    return (uint16_t)uiCurRpt;
}

/***********************************************************************
* VI_Rpt_Added - 删除事件文件
*
* RETURNS: 无
*
*/
static void VI_Rpt_Added(void)
{
    int iRptFileNum;   /* 总报告数 */
    int iSlowDelEvtRptCnt = 0;	/* 统计慢速删除报告数 */
    FILENODE *pFileNode;	/* 文件节点指针 */
    FILENODE *pDelFileNode = NULL;	/* 待删除文件节点指针 */

    semTake(semEvtFileListWR_g, WAIT_FOREVER);
    /* 获取总文件数 */
    iRptFileNum = lstCount(pmEvtFileList_g);

    /* 判断文件是否满,每次满后需要删除多个文件 */
    if (iRptFileNum>rptsts_g.iMaxRptNum)
    {
        int32_t iAllowCnt;

        iAllowCnt = rptsts_g.iMaxRptNum*3/4; 	/* 每次删除1/4最大数目 */

        /* 统计慢速删除个数 */
        for (pFileNode = (FILENODE *)lstFirst(pmEvtFileList_g); pFileNode != NULL;
                pFileNode = (FILENODE *)lstNext((NODE *)pFileNode))
        {
            if (pFileNode->ucDelAttr == 0)
            {
                iSlowDelEvtRptCnt++;
            }
        }

        /* 大于允许删除数
         * 同时慢速文件个数多于指定个数
         * 删除最早文件(可能是快速删除,或慢速删除)
         */

        for (pFileNode = (FILENODE *)lstLast(pmEvtFileList_g);
                (pFileNode != NULL) && (iRptFileNum>iAllowCnt) && (iSlowDelEvtRptCnt>MIN_SLOW_DELETE_NUM);)
        {
            pDelFileNode = pFileNode;
            pFileNode = (FILENODE *)lstPrevious((NODE *)pFileNode);

            /* 该报告号录波文件是否存在，如果不存在则删除该事件文件 */
            if (!FS_isExist(pDelFileNode->uiFileSN, REC_DIR))
            {
                if (pDelFileNode->ucDelAttr == 0)
                {
                    iSlowDelEvtRptCnt--;
                }
                FS_RemoveFile(pDelFileNode->ucFullFileName, EVT_DIR);
                iRptFileNum--;
            }
        }

        /* 删除最后快速删除文件节点 */
        for (pFileNode = (FILENODE *)lstLast(pmEvtFileList_g);
                (pFileNode != NULL) && (iRptFileNum>iAllowCnt);)
        {
            pDelFileNode = pFileNode;
            pFileNode = (FILENODE *)lstPrevious((NODE *)pFileNode);
            if (pDelFileNode->ucDelAttr == 0x01)
            {
                /* 该报告号录波文件是否存在，如果不存在则删除该事件文件 */
                if (!FS_isExist(pDelFileNode->uiFileSN, REC_DIR))
                {
                    FS_RemoveFile(pDelFileNode->ucFullFileName, EVT_DIR);
                    iRptFileNum--;
                }
            }
        }

        /* 大于允许删除数
         * 经前两步删除后仍没有满足要求
         * 则删除最早文件(可能是快速删除,或慢速删除)
         */

        for (pFileNode = (FILENODE *)lstLast(pmEvtFileList_g);
                (pFileNode != NULL) && (iRptFileNum>iAllowCnt);)
        {
            pDelFileNode = pFileNode;
            pFileNode = (FILENODE *)lstPrevious((NODE *)pFileNode);

            /* 直接删除 */
            FS_RemoveFile(pDelFileNode->ucFullFileName, EVT_DIR);
            iRptFileNum--;
        }
    }
    semGive(semEvtFileListWR_g);
}

/***********************************************************************
* VI_Open_QD - 开放启动继电器.
*
* RETURNS: 无
*
*/
void VI_Open_QD()
{
    STATUS vxsts;

    vxsts=taskLock();
    assert(vxsts==OK);

    if (!rptsts_g.iOpenQDCnt++)
    {
        /* 若首次开放启动 */
        if(!(EP_Is_Lock_DO()))
        {
            /*2008-1-28日，zhangyun  merge ,只有当没有异常，开出未闭锁时，才允许开放启动  张云*/
            SIO_Enable_DO();
        }
    }

    vxsts=taskUnlock();
    assert(vxsts==OK);
}

/***********************************************************************
* VI_Close_QD - 关闭启动继电器.
*
* RETURNS: 无
*
*/
void VI_Close_QD()
{
    STATUS vxsts;

    assert(rptsts_g.iOpenQDCnt>0);

    vxsts=taskLock();
    assert(vxsts==OK);

    if (!--rptsts_g.iOpenQDCnt)
    {
        /* 若最后一次关闭启动 */

        SIO_Disable_DO();
    }

    vxsts=taskUnlock();
    assert(vxsts==OK);

}

/***********************************************************************
* GetEvtMakeRptTaskStatus - 获取报告任务状态
*
* RETURNS: 无
*
*/
BOOL GetEvtMakeRptTaskStatus()
{
    /* 获得Evt_Task的状态,若正常,则返回真,否则,返回假 */
    static char strTaskStatus[128];

    if(taskIdVerify(nEvtMakeRptTaskID_g)==ERROR)
    {
        /* 首先判定该任务是否有效 */
        return FALSE;
    }

    taskStatusString(nEvtMakeRptTaskID_g,strTaskStatus);
    if(strcmp(strTaskStatus,"SUSPEND")==0
            ||strcmp(strTaskStatus,"DELAY+S")==0
            ||strcmp(strTaskStatus,"PEND+S")==0
            ||strcmp(strTaskStatus,"PEND+S+T")==0
            ||strcmp(strTaskStatus,"SUSPEND+I")==0
            ||strcmp(strTaskStatus,"DELAY+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+T+I")==0
            ||strcmp(strTaskStatus,"DEAD")==0)
    {
        return FALSE;
    }
    else
    {
        return   TRUE;
    }
}

/***********************************************************************
* VI_MakeEvtFileTaskExecHandle - 为了处理事件报告文件异常时的处理函数，张云添加
*
* RETURNS: 无
*
*/
static void VI_MakeEvtFileTaskExecHandle(
    int iFd,		/* 文件句柄 */
    int iExecReason
)
{
    uint8_t aucBuf[FULL_NAME_LEN+1];
    STATUS vxsts;
    static uint32_t ulCnt = 0;

    if (iFd >= 0)
    {
        static uint32_t ulMsgCnt = 0;

        close(iFd);
        sprintf(aucBuf, EP_EVT_RPT_DIR "/edp%04x.evt", ulCurEvtFileRptNum_g);		/* 删除当前文件 */
        if (FT_Is_File(aucBuf))
        {
            vxsts = remove(aucBuf);
        }

        ulMsgCnt++;
        if (ulMsgCnt%200 == 1)
        {
            LOG_Dbg_Msg("WARNING, Make Event Report File failure for busy or no space.\n", 0, 0, 0, 0, 0, 0);
            if (ENG_MODE == 1)
                LOG_Write(LOG_KERNEL, "WARNING, Make Event Report File failure for busy or no space.\n", NULL);
            else if (ENG_MODE == 0)
                LOG_Write(LOG_KERNEL, "空间太小或CPU负荷过重，无法生成事件报告文件.\n", NULL);
        }
    }

    ulCnt++;
    if (ulCnt%200 == 1)
    {
        static uint8_t aucLogInfo[256];

        sprintf(aucLogInfo, "生成事件文件失败(%d).\n", iExecReason);		/* 记录出错原因 */
        LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
    }
}

/***********************************************************************
* VI_Chg_Mea_Some_Attrs - 改变测量属性
*
* RETURNS: 无
*
*/
void  VI_Chg_Mea_Some_Attrs(void)
{
    VI_MEA_AI_CFG *pmaicfg;
    SCI_SIGNAL_VALUE_TYPE settingvalue;
    EP_STATUS stsResult;
    STATUS vxsts;
    vxsts=taskLock();		/* DY 11/2/2006 */

    for (pmaicfg=pmaicfg_g;
            pmaicfg<pmaicfg_g+iMeaAiNum_g; pmaicfg++)
    {
        if(pmaicfg->ucParaSetMode&0x01)
        {
            stsResult=SCI_Get_Inner_Setting_BySettingStrBase(pmaicfg->aucRtMaxSettingId,&settingvalue);
            if(stsResult==EP_SUCCESS)
            {
                if(IS_INT32_SET(settingvalue.ucAttrib))
                    pmaicfg->fRtMax=settingvalue.Value.lVal;
                else if(IS_UINT32_SET(settingvalue.ucAttrib))
                    pmaicfg->fRtMax=settingvalue.Value.ulVal;
                else
                    pmaicfg->fRtMax=settingvalue.Value.fVal;
            }
            else
            {
                assert(FALSE);
            }
        }
        if(pmaicfg->ucParaSetMode&0x02)
        {
            stsResult=SCI_Get_Inner_Setting_BySettingStrBase(pmaicfg->aucRtMinSettingId,&settingvalue);
            if(stsResult==EP_SUCCESS)
            {
                if(IS_INT32_SET(settingvalue.ucAttrib))
                    pmaicfg->fRtMin=settingvalue.Value.lVal;
                else if(IS_UINT32_SET(settingvalue.ucAttrib))
                    pmaicfg->fRtMin=settingvalue.Value.ulVal;
                else
                    pmaicfg->fRtMin=settingvalue.Value.fVal;
            }
            else
            {
                assert(FALSE);
            }
        }
        if(pmaicfg->ucParaSetMode&0x04)
        {
            stsResult=SCI_Get_Inner_Setting_BySettingStrBase(pmaicfg->aucOvMaxSettingId,&settingvalue);
            if(stsResult==EP_SUCCESS)
            {
                if(IS_INT32_SET(settingvalue.ucAttrib))
                    pmaicfg->fOvMax=settingvalue.Value.lVal;
                else if(IS_UINT32_SET(settingvalue.ucAttrib))
                    pmaicfg->fOvMax=settingvalue.Value.ulVal;
                else
                    pmaicfg->fOvMax=settingvalue.Value.fVal;
            }
            else
            {
                assert(FALSE);
            }
        }
        if(pmaicfg->ucParaSetMode&0x08)
        {
            stsResult=SCI_Get_Inner_Setting_BySettingStrBase(pmaicfg->aucOvMinSettingId,&settingvalue);
            if(stsResult==EP_SUCCESS)
            {
                if(IS_INT32_SET(settingvalue.ucAttrib))
                    pmaicfg->fOvMin=settingvalue.Value.lVal;
                else if(IS_UINT32_SET(settingvalue.ucAttrib))
                    pmaicfg->fOvMin=settingvalue.Value.ulVal;
                else
                    pmaicfg->fOvMin=settingvalue.Value.fVal;
            }
            else
            {
                assert(FALSE);
            }
        }
    }

    vxsts=taskUnlock();
}

/***********************************************************************
* VI_CK_Mea_Attrs - 获取遥测量属性
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_CK_Mea_Attrs(void)
{
    VI_MEA_AI_CFG *pmaicfg;
    EP_STATUS stsResult;
    BOOL bUsed;

    for (pmaicfg=pmaicfg_g;
            pmaicfg<pmaicfg_g+iMeaAiNum_g; pmaicfg++)
    {
        if(pmaicfg->ucParaSetMode&0x01)
        {
            stsResult=SC_Find_Setbase(pmaicfg->aucRtMaxSettingId,&bUsed);
            if(stsResult!=EP_SUCCESS)
            {
                logMsg("Error, No %d mea ai RtMaxsetbase %s  isn't cofigured in sy set !\n",
                       pmaicfg-pmaicfg_g,(int)pmaicfg->aucRtMaxSettingId,0,0,0,0);

                return EP_CFG_ERR;
            }
        }
        if(pmaicfg->ucParaSetMode&0x02)
        {
            stsResult=SC_Find_Setbase(pmaicfg->aucRtMinSettingId,&bUsed);
            if(stsResult!=EP_SUCCESS)
            {
                logMsg("Error, No %d mea ai RtMinsetbase %s   isn't cofigured in sy set !\n",
                       pmaicfg-pmaicfg_g,(int)pmaicfg->aucRtMinSettingId,0,0,0,0);
                return EP_CFG_ERR;
            }
        }
        if(pmaicfg->ucParaSetMode&0x04)
        {
            stsResult=SC_Find_Setbase(pmaicfg->aucOvMaxSettingId,&bUsed);
            if(stsResult!=EP_SUCCESS)
            {
                logMsg("Error, No %d mea ai OvMaxsetbase %s  isn't cofigured in sy set !\n",
                       pmaicfg-pmaicfg_g,(int)pmaicfg->aucOvMaxSettingId,0,0,0,0);

                return EP_CFG_ERR;
            }
        }
        if(pmaicfg->ucParaSetMode&0x08)
        {
            stsResult=SC_Find_Setbase(pmaicfg->aucOvMinSettingId,&bUsed);
            if(stsResult!=EP_SUCCESS)
            {
                logMsg("Error, No %d mea ai OvMinsetbase %s  isn't cofigured in sy set !\n",
                       pmaicfg-pmaicfg_g,(int)pmaicfg->aucOvMinSettingId,0,0,0,0);
                return EP_CFG_ERR;
            }
        }
    }

    return EP_SUCCESS;
}

/***********************************************************************
* VI_New_Adjust - 校准命令，供mmi调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_New_Adjust(
    uint8_t ucObjType, 			/* 校准对象类型，0: ai物理通道，1: 测量量 */
    uint8_t ucOrdType			/* 校准命令类型， 0: 增益校准，1: 偏置校准 */
)
{
    STATUS vxsts;
    uint8_t count = 0;

    vxsts = taskLock();
    adjinfo_g.bVal = TRUE;
    adjinfo_g.bAdjustRunFlag = TRUE;
    adjinfo_g.uAdjustStartTickNum = tickGet();			/* 开始校准tick数 */
    adjinfo_g.ucOrdType = ucOrdType;
    adjinfo_g.ucObjType = ucObjType;
    adjinfo_g.ucNum = 0xFF;		/* 0xFF说明是所有通道，以后可扩充为对单个通道进行校准 */
    vxsts = taskUnlock();

    while (adjinfo_g.bVal)
    {
        taskDelay(1);
        count++;

        if (count>10)
        {
            break;
        }
    }

    if (adjinfo_g.bVal)            /* 逻辑图中没有响应校准命令 */
        return FALSE;

    return TRUE;
}

/***********************************************************************
* VI_Come_New_Plus_Adjust - 新的增益校准命令，供逻辑图扫描调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_Come_New_Plus_Adjust(
    int  nScanTaskNo,
    int32_t *piRtObjType,
    int32_t *piRtNum
)
{
    int i;
    static BOOL AdjStartFlag[MAX_CREATE_RELAYFUNC_TASK_COUNT];
    BOOL bRst=FALSE;

    if(adjinfo_g.bVal && adjinfo_g.ucOrdType==0)
    {
        for(i=0; i<MAX_CREATE_RELAYFUNC_TASK_COUNT; i++)
        {
            AdjStartFlag[i]=TRUE;
        }
        logMsg("校准开始!任务号%d\n", nScanTaskNo, 0, 0, 0, 0, 0);

        adjinfo_g.bVal=FALSE;
    }

    *piRtObjType=adjinfo_g.ucObjType;
    *piRtNum=adjinfo_g.ucNum;
    bRst=AdjStartFlag[nScanTaskNo];
    AdjStartFlag[nScanTaskNo]=FALSE;

    return bRst;
}

/***********************************************************************
* VI_Come_New_Off_Adjust - 新的偏置校准命令，供逻辑图扫描调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_Come_New_Off_Adjust(
    int nScanTaskNo,
    int32_t *piRtObjType,
    int32_t *piRtNum
)
{
    int i;
    static BOOL AdjStartFlag[MAX_CREATE_RELAYFUNC_TASK_COUNT];
    BOOL bRst=FALSE;

    if(adjinfo_g.bVal &&adjinfo_g.ucOrdType==1)
    {
        /* 第一次扫描把所有标志置位 */
        for(i=0; i<MAX_CREATE_RELAYFUNC_TASK_COUNT; i++)
        {
            AdjStartFlag[i]=TRUE;
        }

        adjinfo_g.bVal=FALSE;
    }

    *piRtObjType=adjinfo_g.ucObjType;
    *piRtNum=adjinfo_g.ucNum;
    bRst=AdjStartFlag[nScanTaskNo];
    AdjStartFlag[nScanTaskNo]=FALSE;

    return bRst;
}

/***********************************************************************
* VI_Come_Over_Plus_Adjust - 增益校准结束，供逻辑图扫描调用
*
* RETURNS: 无
*
*/
void VI_Come_Over_Plus_Adjust(
    int32_t *piRtObjType, 		/* 类型 */
    int32_t *piRtNum		/* 通道号 */
)
{
    adjoverinfo_g.bVal=TRUE;
    adjoverinfo_g.ucOrdType=0x0;		/* 增益 */
    adjoverinfo_g.ucObjType=*piRtObjType;
    adjoverinfo_g.ucNum=*piRtNum;
}

/***********************************************************************
* VI_Come_Over_Off_Adjust - 偏置校准结束，供逻辑图扫描调用
*
* RETURNS: 无
*
*/
void VI_Come_Over_Off_Adjust(
    int32_t *piRtObjType, 		/* 类型 */
    int32_t *piRtNum			/* 通道号 */
)
{
    adjoverinfo_g.bVal=TRUE;
    adjoverinfo_g.ucOrdType=0x1;		/* 偏置 */
    adjoverinfo_g.ucObjType=*piRtObjType;
    adjoverinfo_g.ucNum=*piRtNum;
}

/***********************************************************************
* VI_Get_Mea_AI_Idx - 获取遥测量的通道序号
*
* RETURNS: 序号
*
*/
int VI_Get_Mea_AI_Idx(
    uint8_t *pStrID
)
{
    VI_MEA_AI_CFG *pmaicfg;
    BOOL bFind;

    assert(pStrID && strlen(pStrID) <= MAX_ID_LEN);

    bFind=FALSE;
    for (pmaicfg=pmaicfg_g; pmaicfg<pmaicfg_g+iMeaAiNum_g; pmaicfg++)
    {
        if (!strcmp(pStrID, pmaicfg->aucId))
        {
            return pmaicfg-pmaicfg_g;
        }
    }

    return -1;
}

/***********************************************************************
* VI_Set_Mea_AI_ChgCoff - 根据遥测量序号设置遥测量的越限系数
*
* RETURNS: 无
*
*/
void VI_Set_Mea_AI_ChgCoff(
    int iIdx,		/* 序号 */
    float fCoff			/* 系数 */
)
{
    VI_MEA_AI_CFG *pCfg;

    pCfg=pmaicfg_g+iIdx;
    pCfg->fChgCoff=fCoff;
}

/***********************************************************************
* VI_New_PoClear - PO清零，供mmi调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_New_PoClear(
    uint8_t ucObjNum
)
{

    int iLockKey;

    iLockKey=intLock();
    poclearinfo_g.bVal=TRUE;
    poclearinfo_g.bAdjustRunFlag=TRUE;
    poclearinfo_g.uAdjustStartTickNum=tickGet();			/* 开始校准tick数 */
    poclearinfo_g.ucObjNum=ucObjNum;
    intUnlock(iLockKey);

    Reset_PO_Val();
    taskDelay(10);		/* 延迟100ms */
    if(poclearinfo_g.bVal)            /*逻辑图中没有响应电能清零命令*/
        return FALSE;

    return TRUE;
}

/* new command for switching, called by MMI.
 * Para:
 *     ucCmdType, command type.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VI_New_TdSwitch(uint8_t ucCmdType)
{

    int iLockKey;

    iLockKey=intLock();
    TdSwitch_g.bVal=TRUE;
    TdSwitch_g.bAdjustRunFlag=TRUE;
    TdSwitch_g.uAdjustStartTickNum=tickGet();	/* 开始tick */
    TdSwitch_g.ucCmdType=ucCmdType;
    intUnlock(iLockKey);

    taskDelay(10);		/* 延迟100ms */
    if (TdSwitch_g.bVal)     /* 逻辑图中没有响应电能清零命令 */
    {
        return FALSE;
    }

    return TRUE;
}


/***********************************************************************
* VI_New_FarSts - 远方就地状态，供MMI调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_New_FarSts(
    uint32_t ulFarSts
)
{
    int iLockKey;

    iLockKey=intLock();
    FarSts_g.bVal=TRUE;
    FarSts_g.ulOrderType=ulFarSts;
    intUnlock(iLockKey);

    taskDelay(10);		/* 延迟100ms */
    if(FarSts_g.bVal)            /* 逻辑图中没有响应该命令 */
        return FALSE;

    return TRUE;
}

/***********************************************************************
* VI_New_RepairSts - 检修状态，供MMI调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_New_RepairSts(
    uint32_t ulRepairSts
)
{
    int iLockKey;

    iLockKey=intLock();
    RepairSts_g.bVal=TRUE;
    RepairSts_g.ulOrderType=ulRepairSts;
    intUnlock(iLockKey);

    taskDelay(10);		/* 延迟100ms */
    if(RepairSts_g.bVal)            /* 逻辑图中没有响应该命令 */
        return FALSE;

    return TRUE;
}

/***********************************************************************
* VI_New_JgsSts - 解挂锁状态，供MMI调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_New_JgsSts(
    uint32_t ulJgsSts
)
{
    int iLockKey;

    iLockKey=intLock();
    JgsSts_g.bVal=TRUE;
    JgsSts_g.ulOrderType=ulJgsSts;
    intUnlock(iLockKey);

    taskDelay(10);		/* 延迟100ms */
    if(JgsSts_g.bVal)            /* 逻辑图中没有响应该命令 */
        return FALSE;

    return TRUE;
}

/***********************************************************************
* VI_Come_New_PoClear - PO清零，供逻辑图扫描调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_Come_New_PoClear(
    int32_t *piRtNum
)
{
    if(poclearinfo_g.bVal )
    {
        *piRtNum=poclearinfo_g.ucObjNum;
        poclearinfo_g.bVal=FALSE;					/* adjinfo_g应该为poclearinfo_g */
        return TRUE;
    }
    return FALSE;
}

/* get command for ZhuBian switching, called by logic graph.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VI_Come_New_TdZhuBianSwitch(void)
{
    if (TdSwitch_g.bVal && (TdSwitch_g.ucCmdType == 0x11))
    {
        TdSwitch_g.bVal=FALSE;

        return TRUE;
    }
    return FALSE;
}

/* get command for JinXian switching, called by logic graph.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VI_Come_New_TdJinXianSwitch(void)
{
    if (TdSwitch_g.bVal && (TdSwitch_g.ucCmdType == 0x33))
    {
        TdSwitch_g.bVal=FALSE;

        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
* VI_Come_New_FarSts - 远方就地状态，供逻辑图扫描调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_Come_New_FarSts(
    int32_t *PulOrderType
)
{
    if(FarSts_g.bVal )
    {
        *PulOrderType=FarSts_g.ulOrderType;
        FarSts_g.bVal=FALSE;
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
* VI_Come_New_RepairSts - 检修状态，供逻辑图扫描调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_Come_New_RepairSts(
    int32_t *PulOrderType
)
{
    if(RepairSts_g.bVal )
    {
        *PulOrderType=RepairSts_g.ulOrderType;
        RepairSts_g.bVal=FALSE;
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
* VI_Come_New_JgsSts - 解挂锁状态，供逻辑图扫描调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_Come_New_JgsSts(
    int32_t *PulOrderType
)
{
    if(JgsSts_g.bVal )
    {
        *PulOrderType=JgsSts_g.ulOrderType;
        JgsSts_g.bVal=FALSE;
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
* DelEvtFile - 配置修改后删除事件文件和录波文件
*
* RETURNS: 无
*
*/
void DelEvtAndRecFile(void)
{
    DIR *pdir;
    struct dirent *pent;
    uint8_t RptFileName[FULL_NAME_LEN];
    STATUS vxsts;

    pdir=opendir(EP_EVT_RPT_DIR);
    if (pdir)
    {
        /* 对事件文件夹中的文件名进行读取 */
        while ((pent=readdir(pdir)) != NULL)
        {
            /*  */
            strcpy(RptFileName,EP_EVT_RPT_DIR);
            strcat(RptFileName,"/");
            strcat(RptFileName,pent->d_name);
            vxsts=remove(RptFileName);
            assert(vxsts == OK);
            /*taskDelay(10);	zhangyun  2008-1-28日 merge,不合理，延时去掉*/	/* 延迟100ms */
        }
        closedir(pdir);
    }

    pdir=opendir(EP_WAVE_REC_DIR);
    if (pdir)
    {
        /* 对事件文件夹中的文件名进行读取 */
        while ((pent=readdir(pdir)) != NULL)
        {
            /*  */
            strcpy(RptFileName,EP_WAVE_REC_DIR);
            strcat(RptFileName,"/");
            strcat(RptFileName,pent->d_name);
            vxsts=remove(RptFileName);
            assert(vxsts == OK);
            /*taskDelay(10);  zhangyun  2008-1-28日 merge,不合理，延时去掉*/	/* 延迟100ms */
        }
        closedir(pdir);
    }
}

/***********************************************************************
* DelPanelAucCpuFile - 通讯成功后删除面板辅助CPU程序
*
* RETURNS: 无
*
*/
STATUS DelPanelAucCpuFile(
    uint8_t *pAttrFlag
)
{
    STATUS vxsts;

    vxsts=OK;
    if (FT_Is_File(EP_PANEL_BIN_FILE))
    {
        /* 删掉bin文件 */
        *pAttrFlag |= 0x01;
        vxsts=remove(EP_PANEL_BIN_FILE);
    }

    if (FT_Is_File(EP_PANEL_FUSE_FILE))
    {
        /* 删掉fuse文件 */
        *pAttrFlag |= 0x02;
        vxsts=remove(EP_PANEL_FUSE_FILE);
    }

    return vxsts;
}

/***********************************************************************
* DelMainAucCpuFile - 通讯成功后删除主板辅助CPU程序
*
* RETURNS: 无
*
*/
STATUS DelMainAucCpuFile(
    uint8_t *pAttrFlag
)
{
    STATUS vxsts;

    vxsts=OK;
    if (FT_Is_File(EP_WATCHALARM_BIN_FILE))
    {
        /* 删掉bin文件 */
        *pAttrFlag |= 0x01;
        vxsts=remove(EP_WATCHALARM_BIN_FILE);
    }

    if (FT_Is_File(EP_WATCHALARM_FUSE_FILE))
    {
        /* 删掉fuse文件 */
        *pAttrFlag |= 0x02;
        vxsts=remove(EP_WATCHALARM_FUSE_FILE);
    }

    return vxsts;
}

/***********************************************************************
* GetRptSNProcessState - 事件模块初始化之前的事件是否处理完成
*
* RETURNS:
*				  TRUE: 已处理完
*              FALSE: 没有处理完
*
*/
BOOL GetRptSNProcessState(void)
{
    return rptsts_g.bRptSNProcessFinish;
}

/***********************************************************************
* SCI_Deal_Event_Alert - 根据事件的区分码，处理事件的呼唤属性，要求每次逻辑图都扫描处理
*
* RETURNS: 无
*
*/
void SCI_Deal_Event_Alert(
    int16_t nNum, 	/* 事件区分码 */
    BOOL bActFlag				/* 事件动作标志，TRUE为表示是事件动作，FALSE表示是事件返回 */
)
{
    VI_EVT_CFG *pevtcfg;

    pevtcfg=pevtcfg_g+nNum;

    if(bActFlag)
    {
        if(pevtcfg->pAlertHdl)
        {
            /* 设置事件对应的呼唤，为了防止复归时，清除还存在的呼唤，每次需要重新检测，且设置 */
            ER_SetAlertSignal(pevtcfg->pAlertHdl);
        }
    }
}

/***********************************************************************
* VI_Is_Fault - 返回是否处于故障态，故障态认为是一个特殊状态
*
* RETURNS:
*               TRUE: 是故障态
*               FALSE: 非故障态
*
*/
BOOL VI_Is_Fault(void)
{
    if(rptsts_g.iFault)
    {
        return TRUE;
    }
    else
    {
        return  FALSE;
    }
}

/***********************************************************************
* GetMeaCoff - 获取遥测量相关系数，MMI调用
*
* RETURNS: NONE
*
*/
void GetMeaCoff(
    VI_AI_COFF *pMeaCoff		/* 分配iMeaAiNum_g个 */
)
{
    VI_MEA_AI_CFG *pmaicfg;
    STATUS vxsts;

    assert(pMeaCoff);
    vxsts=taskLock();
    assert(vxsts==OK);

    for (pmaicfg=pmaicfg_g;
            pmaicfg<pmaicfg_g+iMeaAiNum_g; pmaicfg++, pMeaCoff++)
    {
        pMeaCoff->fRtMax=pmaicfg->fRtMax;
        pMeaCoff->fRtMin=pmaicfg->fRtMin;
        pMeaCoff->fOvMax=pmaicfg->fOvMax;
        pMeaCoff->fOvMin=pmaicfg->fOvMin;
        pMeaCoff->fChgCoff=pmaicfg->fChgCoff;
    }

    vxsts=taskUnlock();
}

/* read the newest SN.
 * Para:
 *     Newest SN.
 * Return:
 *     TRUE, or FALSE.
 */
EP_STATUS EP_GetNewestSN(uint16_t *pNewestSN)
{
    uint8_t aucBuf[31];
    int iRst;
    int32_t iNewest;

    /* 读取额外系统配置文件 */
    iRst = FT_Rd_Aux_Sys_INI("[SYSTEM]", "NewestSN", aucBuf, 30);

    if (iRst == 1)
    {
        iNewest=atoi(aucBuf);		/* The newest. */
        if((iNewest >= 0) && (iNewest <= 0xFFFF))//此处推测是原代码存在的问题，会导致第一次运行时无法通过检查
        {
            *pNewestSN = iNewest;

            return EP_SUCCESS;
        }
        else
        {
            goto reterr;
        }
    }
    else if(iRst == 0)
    {
        /* 重新生成系统文件 */
        LOG_Write(LOG_KERNEL, "因[SYSTEM] NewestSn值读取失败，创建新的附加系统INI文件.\n", NULL);
        /* 新设立文件, 不存在兼容性问题 */

        goto reterr;
    }
    else if(iRst>1)
    {
        LOG_Write(LOG_KERNEL, "找到附加系统设置文件，但最新序列号设置项多于1个!\n", NULL);

        goto reterr;
    }
    else if(iRst == EP_ERROR)
    {
        LOG_Write(LOG_KERNEL, "附加系统文件读取失败!\n", NULL);

        goto reterr;
    }


reterr:

    return EP_ERROR;
}

/* set the newest SN.
 * Para:
 *     newest SN.
 * Return:
 *     Result.
 */
EP_STATUS EP_SetNewestSN(uint16_t uNewestSN)
{
    uint8_t aucBuf[31];
    EP_STATUS Sts = EP_SUCCESS;
    static BOOL bFstEnter = TRUE;
    BOOL bWrFlag = FALSE;
    uint16_t usDif = 0;

    semTake(semWrReportSN, WAIT_FOREVER);

    if (bFstEnter)
    {
        bFstEnter = FALSE;
        bWrFlag = TRUE;
    }
    else
    {
        usDif = (uint16_t)(rptsts_g.unRptSN - uNewestSN);
        if (usDif < (uint16_t)(rptsts_g.unRptSN-rptsts_g.uStNewestSN))
        {
            bWrFlag = TRUE;
        }
        else
        {
            bWrFlag = FALSE;
        }
    }


    if (bWrFlag)
    {
        rptsts_g.uStNewestSN = uNewestSN;
        sprintf(aucBuf, "%d", uNewestSN);

        /* 读取额外系统配置文件 */
        if (FT_Wr_Aux_Sys_INI("[SYSTEM]", "NewestSN", aucBuf)<0)
        {
            Sts = EP_ERROR;
        }
    }
    else
    {
        Sts = EP_SUCCESS;
    }

    semGive(semWrReportSN);

    return Sts;
}

/* set flag not using remote control feedback.
 * Para:
 *     bType, remote control type;
 *     TRUE: include prep telecommand and feedback;
 *     FALSE: not include.
 * Return:
 *     NONE.
 */
void VI_SetRemoteControlFlag(BOOL bType)
{
    bNotUseRTFdbkFlag = bType;
}

/* initialize the event delete attribution array.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
static void VI_InitDelAttr(void)
{
    int i;

    for (i = 0; i<MAX_SLOW_DEL_EVT_FILE; i++)
    {
        SlowDelSN[i] = rptsts_g.uStNewestSN;
    }

    ucNewestPos = 0;
}

/* write event file deleting attribution.
 * Para:
 *     usCurSN, current SN.
 * Return:
 *     NONE.
 */
static void VI_WrEventDelAttr(uint16_t usCurSN)
{
    taskLock();

    if (usCurSN == SlowDelSN[ucNewestPos])
    {
        taskUnlock();

        return;
    }
    else if ((int16_t)(usCurSN-SlowDelSN[ucNewestPos])>0)
    {
        ucNewestPos = (ucNewestPos+1) & MAX_SLOW_DEL_EVT_FILE_MASK;
        SlowDelSN[ucNewestPos] = usCurSN;
    }
    else
    {
        int i;

        for (i = 0; i<MAX_SLOW_DEL_EVT_FILE; i++)
        {
            if (usCurSN == SlowDelSN[i])
            {
                break;
            }
        }

        if (i >= MAX_SLOW_DEL_EVT_FILE)
        {
            ucNewestPos = (ucNewestPos+1) & MAX_SLOW_DEL_EVT_FILE_MASK;
            SlowDelSN[ucNewestPos] = usCurSN;
        }
    }

    taskUnlock();
}

/* read event file deleting attribution.
 * Para:
 *     usCurSN, current SN.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VI_RdEventDelAttr(uint16_t usCurSN)
{
    int i;

    taskLock();
    for (i = 0; i<MAX_SLOW_DEL_EVT_FILE; i++)
    {
        if (usCurSN == SlowDelSN[i])
        {
            taskUnlock();

            return TRUE;
        }
    }
    taskUnlock();

    return FALSE;
}

/***********************************************************************
* VI_Run_Info_Wr_P - 开始运行信息写入
*
* RETURNS: 无
*
*/
VI_RUN_INFO *VI_Run_Info_Wr_P(void)
{
    VI_RUN_INFO *pinf;
    int iLockKey;

    ulInfNonComleteCnt_g++;

    iLockKey=intLock();

    pinf=arinf_g+uiCurInfPos_g;

    uiCurInfIdx_g++;			/* 总的条数 */
    uiCurInfPos_g=(uiCurInfPos_g+1)%SZ_RUN_INFO_BUF;			/* 缓冲区中的条数 */

    intUnlock(iLockKey);

    return pinf;
}

/***********************************************************************
* VI_End_Wr_Run_Info - 结束运行信息写入
*
* RETURNS: 无
*
*/
void VI_End_Wr_Run_Info(void)
{
    if (semGive(semNewInfRpt_g) != OK)
        assert(FALSE);

    if (semGive(semNewInfCom_g) != OK)
        assert(FALSE);

#ifdef GXC01U
    if (semGive(semNewInfGxc_g) != OK)
    {
        assert(FALSE);
    }
#endif

    ulInfNonComleteCnt_g--;
}

/* 显示遥信状态 */
void showMeaDi(void)
{
    int untemp_val;
    uint16_t *punshort; /* 遥信品质位 */
    uint16_t *pus;
    BOOL *pbool;
    BOOL *pb;

    pbool = malloc(5000);
    assert(pbool);

    punshort = malloc(5000);
    assert(punshort);
    pb = pbool;
    pus = punshort;
    VI_Rd_Mea_DI_Val(pbool, punshort);

    printf("遥信个数%d\n", iMeaDiNum_g);

    for (untemp_val = 0; untemp_val<iMeaDiNum_g; untemp_val++)
    {
        printf("遥信%d 状态: %d 品质: %x\n", untemp_val, *pb++, *pus++);
    }

    free(pbool);
    free(punshort);
}

#ifdef EDP02_PSR_BUILD

#define NORMAL_SETTING1 1                /* 普通保护定值模式*/
#define     INSIDE_SETTING1  2						/*内部定值模式*/
#define     FARCTRL_SETTING1 3						/*测控定值模式*/
#define SET_FILE_LENGTH1 (1024*20) 			/*定值文件最大长度*/

extern SC_SET_PAGE *psetpg_g; /*from swcfg.c*/
extern int iSetPgNum_g;/*from swcfg.c*/

/****************************demo********************/


typedef struct
{
    uint8_t type;
    void  (*func)(void* info );
} EdpCan_Auto_Send_Stru;

EdpCan_Auto_Send_Stru EdpCan_AutoSend[] =
{
    {MEA_OVER, NULL	},
    {NEW_EVT, NULL	},
    {NEW_SOE, NULL	},
    {LED_CHG, NULL	},
    {LINK_CHG, NULL	},
    {ERR_OCR, NULL	},
    {ERR_OCR_STAT, NULL	},
};

const uint8_t AutoSend_Num = sizeof(EdpCan_AutoSend)/sizeof(EdpCan_AutoSend[0]);


/*主动上送的example*/
void EdpCan_AutoSend_Task()
{
    EdpCan_Auto_Send_Stru *p=EdpCan_AutoSend;
    uint8_t *p_send_buffer;
    uint8_t *p_rcv_buffer;
    uint16_t unRType;
    uint8_t i;
    u_int autoIdx=0;
    const VI_RUN_INFO *prunInfo;
    EP_DATE_TIME dttm;
    uint8_t pucGet[4];
    uint16_t err_msg_len;
    uint8_t uctemp_val;
    int retcode;
    int sem_ok;
    sem_ok=semTake(semNewInfCom_g,WAIT_FOREVER);
    if(sem_ok==ERROR)
        /*      LOG_Dbg_Msg("Take a Event sem and semTake ERROR\n",0,0,0,0,0,0);    */
        prunInfo=VI_Rd_Run_Info(autoIdx);
    autoIdx++;
    if(prunInfo!=NULL)
    {
        for(i=0; i<AutoSend_Num; i++)
        {
            if( prunInfo->type == (p+i)->type)
            {
                (p+i)->func(prunInfo);
                /*   			retcode=msgQSend(SendEventToMmiMsgQFd,(char *)p_send_buffer, 512+2+2, NO_WAIT,MSG_PRI_NORMAL);
                   			taskDelay(1);
                   			retcode=msgQSend(SendEventToSgviewMsgQFd,(char *)p_send_buffer, 512+2+2, NO_WAIT,MSG_PRI_NORMAL);
                   	*/
            }
        }
    }
}






/**************************log相关处理函数*****************************/


/*切换定值区记日志ok*/
extern void ChangeSetAreaModifiesToLog(uint8_t preArea,uint8_t newArea, uint16_t usOpSrc);

/*软压板操作记日志*/
extern void SybChangeToLog(SC_LINK_ITEM *RybItem, BOOL bOldStas, BOOL bNewStats, uint16_t usOpSrc);

/*删除定值区记日志ok*/
extern void  DeleteSetAreaToLog(uint8_t AreaCode, uint16_t usOpSrc);

/*切换yb总选择记日志ok*/
extern void YBTotalToLog(uint16_t oldTotalYbStas,uint8_t newYbTotalStas, uint16_t usOpSrc);

/*保护功能投退记日志ok*/
extern void ProtStatsModifiesToLog(const SC_SUB_LGC_ITEM *ProItem,BOOL bOldProtStats,
                                   BOOL bNewProtStats, uint16_t usOpSrc);

/*网络设置记日志ok*/
extern void IPAdressModifiesToLog(EDP_NET_CFG_INFO OldIpStats,EDP_NET_CFG_INFO NewIpStats);

/*装置电能清零记日志ok*/
extern void PoClearAdjustToLog(int iChannel);

/*装置通道校准记日志ok*/
extern void ChannelAdjustToLog();

/*装置复归记日志ok*/
extern void DeviceResetToLog();










/*****************************事件相关******************************************/
int EdpCan_Get_Event_Cfg_Num()
{
    return iEvtNum_g;
}

/*获取事件配置的头指针*/
VI_EVT_CFG* EdpCan_Get_Event_Cfg_Point()
{
    return pevtcfg_g;
}

/*获取事件的相关数据长度*/
uint16_t EdpCan_Get_Event_Para_Num(const VI_RUN_INFO *prunInfo)
{
    return prunInfo->msg.evt.pcfg->ucParmNum;
}

/*获取当前事件的类型*/
uint16_t EdpCan_Get_Event_Type(const VI_RUN_INFO *prunInfo)
{
    return prunInfo->msg.evt.pcfg->unCode;
}

/*获取当前事件的属性*/
uint16_t EdpCan_Get_Event_ParaAttrib(const VI_RUN_INFO *prunInfo,uint16_t serial)
{
    return prunInfo->msg.evt.pcfg->aparmcfg[serial].ucAttrib;
}

/*获取事件的相关数据*/
VI_EVT_PARM* 	EdpCan_Get_Event_ParaData(const VI_RUN_INFO *prunInfo,uint16_t serial)
{
    return &(prunInfo->msg.evt.aparm[serial].xVal);
}

/*获取SOE事件的相关数据*/
VI_SOE_MSG* 	EdpCan_Get_Soe_Data(const VI_RUN_INFO *prunInfo)
{
    return &(prunInfo->msg.soe);
}

/*获取面板灯的相关数据*/
VI_LED_MSG* 	EdpCan_Get_Led_Status(const VI_RUN_INFO *prunInfo)
{
    return &(prunInfo->msg.led);
}


/*获取连接的相关数据*/
VI_LINK_MSG* 	EdpCan_Get_Link_Status(const VI_RUN_INFO *prunInfo)
{
    return &(prunInfo->msg.link);
}

/*获取ERR的相关数据*/
VI_ERR_MSG* 	EdpCan_Get_Err_Status(const VI_RUN_INFO *prunInfo)
{
    return &(prunInfo->msg.err);
}

/*获取ERR_STAT的相关数据*/
VI_ERR_STAT_MSG* 	EdpCan_Get_Err_Stat_Status(const VI_RUN_INFO *prunInfo)
{
    return &(prunInfo->msg.errstat);
}













/*****************************开出相关******************************************/



/*遥控配置结构体的头指针*/
VI_MEA_DO_CFG* EdpCan_Get_Remote_Do_Cfg()
{
    return pmdocfg_g;
}

/*遥控数据结构体的头指针*/
VI_MEA_DO_DB* EdpCan_Get_Remote_Do_DB()
{
    return pmdodb_g;
}

/*遥控点的个数*/
uint16_t EdpCan_Get_Remote_Do_Num()
{
    return iMeaDoNum_g;
}



/*获取物理开出量的总个数*/
uint16_t EdpCan_Get_Hw_Do_Num()
{
    return iLgcDoChNum_g;
}

/*获取物理开出量的结构指针*/
RD_LGC_DO_CH* EdpCan_Get_Hw_Do_Stru()
{
    return plgcdoch_g;
}









/*****************************定值相关******************************************/

/* Get high 8-bit byte of 16-bit word. */
#define HIGH8(un)     (uint8_t)(((un)>>8) & 0xFF)
/* Get low 8-bit byte of 16-bit word. */
#define LOW8(un)     (uint8_t)((un) & 0xFF)

/*用于将所有的定值区刷入到结构体中*/
extern EP_STATUS SC_End_Wr_Set(int iArea, uint8_t *Back_Filename);

/*判断当前定值区是否有效*/
uint8_t EdpCan_If_Set_Area_Efficient(int iArea)
{
    if (iArea)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/* 在strFile 字符串最后的'/'后面加上'#','$' 幅值到temp_filename */
void EdpCan_Temp_File_Name(char *temp_filename, char  *strFile,char insChar )
{
    char *puc;

    strcpy(temp_filename, strFile );
    /* Let's add a '#' or a '$' just after the last '/'. */
    for (puc=temp_filename+strlen(temp_filename)-1; puc>=temp_filename; puc--)
    {
        if (*puc=='/')
            break;
    }
    memmove(puc+2, puc+1, strlen(puc)+1);
    puc[1]=insChar;
}

EP_STATUS EdpCan_WriteValidSettingFile(int style,int zoneCode,uint8_t *ptBuf,int fileLen)
{
    int new_fileID=0;
    BOOL bValidSetFlag = FALSE ; /*定值文件是否有效标志*/
    EP_STATUS bValidSetFlag1=EP_ERROR; /*定值文件是否有效标志1*/
    char filename[30] ;
    char new_tmp_filename[30];
    char back_filename[30];
    EP_STATUS retcode= EP_ERROR;
    char *ptBufBak =calloc(SET_FILE_LENGTH1,sizeof(char));
    int bakfilelen=0;
    BOOL bOldFileIsValid = FALSE ;
    if(ptBufBak==NULL)
    {
        return retcode;
    }


    switch(style)
    {
        case NORMAL_SETTING1:
            sprintf(filename,"%s/area%02x.dza",EP_SET_AREA_DIR,zoneCode);
            break;
        case FARCTRL_SETTING1:
            strcpy(filename, EP_CK_SET_FILE);
            break;
        case INSIDE_SETTING1:
            strcpy(filename, EP_INNER_SET_FILE);
            break;
        default:
            break;
    }
    EdpCan_Temp_File_Name(new_tmp_filename,filename,'$');
    if((new_fileID=creat(new_tmp_filename,O_RDWR))<0)
        goto exit ;
    retcode = write(new_fileID,ptBuf,fileLen);
    if(retcode !=fileLen)
    {
        /*  LOG_Dbg_Msg_Mmi("When write settings file an error has occurred\n",0,0,0,0,0,0);*/
        close(new_fileID);
        remove(new_tmp_filename);
        retcode = EP_ERROR ;
        goto exit;
    }
    lseek(new_fileID,0,SEEK_SET);
    switch(style)
    {
        case NORMAL_SETTING1:
            bValidSetFlag =SC_Is_Valid_Set(new_fileID);
            break;
        case FARCTRL_SETTING1:
            bValidSetFlag1=SC_Chg_CK_Set(new_fileID);
            break;
        case INSIDE_SETTING1:
            bValidSetFlag1=SC_Chg_Inner_Set(new_fileID);
            break;
        default:
            break;
    }
    if(bValidSetFlag==TRUE || bValidSetFlag1==EP_SUCCESS)
    {
        close(new_fileID);
        if((new_fileID=open(filename,0,0))>0)
        {
            bakfilelen = read(new_fileID,ptBufBak,SET_FILE_LENGTH1-1);
            if(bakfilelen>=(SET_FILE_LENGTH1-1))
                assert(FALSE);
            switch(style)
            {
                case NORMAL_SETTING1:
                    bOldFileIsValid = SC_Is_Valid_Set(new_fileID);/*旧文件是否有效标志*/
                    break;
                default:
                    break;
            }
            close(new_fileID);
            strcpy(back_filename,"");
            EdpCan_Temp_File_Name(back_filename,filename,'#');
            if((new_fileID=open(back_filename,0,0))>0)
            {
                close(new_fileID);
                remove(back_filename);
            }
            rename(filename,back_filename);
            remove(filename);
        }
        else    /* 原来无该定值区，需新建 */
        {
            strcpy(back_filename,SET_BACK_FILE_NONE);
        }

        rename(new_tmp_filename,filename);
        switch(style)
        {
            case NORMAL_SETTING1:
                /* 更新失败返回 */
                if (SC_End_Wr_Set(zoneCode,back_filename) == EP_ERROR)
                {
                    goto exit;
                }
                RE_SetLogSetChgCnt();/* 文件操作结束后更新计数 */
                break;
            default:
                break;
        }
        remove(new_tmp_filename);
        if((new_fileID=open(back_filename,0,0))>0)
        {
            close(new_fileID);
            remove(back_filename);
        }
        if((new_fileID=open(filename,0,0))>0)
        {
            int filelentmp=0;
            filelentmp = read(new_fileID,ptBuf,SET_FILE_LENGTH1-1);
            if(filelentmp>=(SET_FILE_LENGTH1-1))
                assert(FALSE);
            close(new_fileID);
            if(filelentmp!=fileLen)
            {
                retcode=EP_ERROR;
                goto exit;
            }
        }
        /*       switch(style)
               {
                   case NORMAL_SETTING:
                       CheckSettingModifiesToLog(bOldFileIsValid,zoneCode,ptBufBak,bakfilelen,ptBuf,fileLen);
                       break;
                   case FARCTRL_SETTING:
                       CheckMeaCtrlSetModifiesToLog(ptBufBak,bakfilelen,ptBuf,fileLen);
                       break;
                   case INSIDE_SETTING:
                       CheckInsideSetModifiesToLog(ptBufBak,bakfilelen,ptBuf,fileLen);	//这个函数是MMI的？？？
                       break;
                   default:
                       break;
               }*/
        retcode = EP_SUCCESS;
    }
    else
    {
        close(new_fileID);
        remove(new_tmp_filename);
        retcode = EP_ERROR ;
    }

exit:

    if(ptBufBak!=NULL)
        free(ptBufBak);
    return retcode ;
}


/*定值结构体中的数据刷入到文件中*/
EP_STATUS EdpCan_WriteSettings(uint8_t zoneCode,BOOL bZeroForTest)
{
    SC_SET_PAGE *psetpg;
    SC_SET_ITEM *pset;
    int i=0,j=0;
    char *ptBuf =calloc(SET_FILE_LENGTH1,sizeof(char));
    int fileLen=0 ;
    EP_STATUS retcode= EP_ERROR;
    char *buf=ptBuf;

    *buf++=0x82;
    *buf++=0x00;
    *buf++=0x00;
    *buf++=0x7D;
    *buf++=LOW8(EP_INNER_PRTCL_VER);
    *buf++=HIGH8(EP_INNER_PRTCL_VER);
    /*
    *buf++=pCpuInfo_g->version04Info[MMI_CPU_PROTOCOL_VER_04_CODE*2]; 	//这里需要改
    *buf++=pCpuInfo_g->version04Info[MMI_CPU_PROTOCOL_VER_04_CODE*2+1]; 		*/
    *buf++=0x00;
    *buf++=0x00;
    *buf++=0x00;
    *buf++=iSetPgNum_g-1;
    // *buf++=pCpuInfo_g->m_unSetPages;/*页数*/
    // for(i=0;i<pCpuInfo_g->m_unSetPages;i++)
    i=0;
    for (psetpg=psetpg_g+1; psetpg<psetpg_g+iSetPgNum_g; psetpg++)
    {

        //  *buf++=(uint8_t)pCpuInfo_g->m_ptSetPage[i].m_unSN;/*序号*/
        *buf++=i;/*序号*/
        *buf++=0; /*是否激活,已废除,保留*/
        //   *buf++=LOW8(pCpuInfo_g->m_ptSetPage[i].m_unSets);/*该页定值数*/
        //  *buf++=HIGH8(pCpuInfo_g->m_ptSetPage[i].m_unSets);
        *buf++=LOW8(psetpg->iSetNum);/*该页定值数*/
        *buf++=HIGH8(psetpg->iSetNum);
        pset=psetpg->pset;
        //     for(j=0;j<pCpuInfo_g->m_ptSetPage[i].m_unSets;j++)
        for(j=0; j<psetpg->iSetNum; j++)
        {
            *buf++=0x00;
            *buf++=0x00;
            // *buf++=LOW8(pCpuInfo_g->m_ptSetPage[i].m_ptSet[j].m_unSN);/*序号*/
            // *buf++=HIGH8(pCpuInfo_g->m_ptSetPage[i].m_ptSet[j].m_unSN);
            *buf++=LOW8(j);/*序号*/
            *buf++=HIGH8(j);
            //*buf++=pCpuInfo_g->m_ptSetPage[i].m_ptSet[j].m_ucType;/*类型*/
            *buf++=(pset+j)->ucUnit;
            // if((pCpuInfo_g->m_SetPageCode_ForTest==i)&&
            //  (bZeroForTest==TRUE)) /* 励磁用动态试验参数*/
            //     memcpy(buf,pCpuInfo_g->m_ptSetPage[i].m_ptSet[j].m_aucDefaultVal,4);
            //  else/*整定值*/  /* 正常*/
            //     memcpy(buf,pCpuInfo_g->m_ptSetPage[i].m_ptSet[j].m_currentVal,4);
            memcpy(buf,&((pset+j)->valNow.ulVal),4);

            buf+=4;
        }
        i++;
    }
    *buf++=0x84;
    *buf++=0x00;
    *buf++=0x00;
    *buf++=0x7B;

    fileLen = buf - ptBuf ;
    if(fileLen>=(SET_FILE_LENGTH1-1))
        assert(FALSE);
    retcode =EdpCan_WriteValidSettingFile(NORMAL_SETTING1,zoneCode,ptBuf,fileLen);
    if(ptBuf!=NULL)
        free(ptBuf);
    return retcode ;
}

/*获取运行定值区定值页的结构指针*/
SC_SET_PAGE* EdpCan_Get_Run_Set_Page_Stru()
{
    return psetpg_g;
}
/*获取定值页的总个数*/
uint16_t EdpCan_Get_Set_Page_Num()
{
    return iSetPgNum_g;
}
/*获取当前运行区具体的定值的结构体
  page  具体的定值页
  serial 具体定值页的具体定值的偏移*/
SC_SET_ITEM* EdpCan_Get_Run_Set_Point(uint16_t page,uint16_t serial)
{
    return ( (psetpg_g+page)->pset+serial);
}

/*获取定值区中具体某页中定值的具体个数*/
uint16_t EdpCan_Get_Set_Num(uint16_t page)
{
    return ( (psetpg_g+page)->iSetNum);
}

/*
enum
{
    NORMAL_SETTING =1,
    INSIDE_SETTING,
    FARCTRL_SETTING,
};
*/






/*将通信buf中的数据刷入到文件中*/
EP_STATUS EdpCan_WriteSettings_From_Buf(uint8_t zoneCode,uint8_t* commbuf)
{
    uint8_t tbuf[4];

    uint8_t* cbuf = commbuf;
    SC_SET_PAGE *psetpg;
    SC_SET_ITEM *pset;
    int i=0,j=0;
    char *ptBuf =calloc(SET_FILE_LENGTH1,sizeof(char));
    int fileLen=0 ;
    EP_STATUS retcode= EP_ERROR;
    char *buf=ptBuf;

    *buf++=0x82;
    *buf++=0x00;
    *buf++=0x00;
    *buf++=0x7D;
    *buf++=LOW8(EP_INNER_PRTCL_VER);
    *buf++=HIGH8(EP_INNER_PRTCL_VER);
    /*
    *buf++=pCpuInfo_g->version04Info[MMI_CPU_PROTOCOL_VER_04_CODE*2]; 	//这里需要改
    *buf++=pCpuInfo_g->version04Info[MMI_CPU_PROTOCOL_VER_04_CODE*2+1]; 		*/
    *buf++=0x00;
    *buf++=0x00;
    *buf++=0x00;
    *buf++=iSetPgNum_g-1;
    // *buf++=pCpuInfo_g->m_unSetPages;/*页数*/
    // for(i=0;i<pCpuInfo_g->m_unSetPages;i++)
    i=0;
    for (psetpg=psetpg_g+1; psetpg<psetpg_g+iSetPgNum_g; psetpg++)
    {

        //  *buf++=(uint8_t)pCpuInfo_g->m_ptSetPage[i].m_unSN;/*序号*/
        *buf++=i;/*序号*/
        *buf++=0; /*是否激活,已废除,保留*/
        //   *buf++=LOW8(pCpuInfo_g->m_ptSetPage[i].m_unSets);/*该页定值数*/
        //  *buf++=HIGH8(pCpuInfo_g->m_ptSetPage[i].m_unSets);
        *buf++=LOW8(psetpg->iSetNum);/*该页定值数*/
        *buf++=HIGH8(psetpg->iSetNum);
        pset=psetpg->pset;
        //     for(j=0;j<pCpuInfo_g->m_ptSetPage[i].m_unSets;j++)
        for(j=0; j<psetpg->iSetNum; j++)
        {
            *buf++=0x00;
            *buf++=0x00;
            // *buf++=LOW8(pCpuInfo_g->m_ptSetPage[i].m_ptSet[j].m_unSN);/*序号*/
            // *buf++=HIGH8(pCpuInfo_g->m_ptSetPage[i].m_ptSet[j].m_unSN);
            *buf++=LOW8(j);/*序号*/
            *buf++=HIGH8(j);
            //*buf++=pCpuInfo_g->m_ptSetPage[i].m_ptSet[j].m_ucType;/*类型*/
            *buf++=(pset+j)->ucUnit;
            // if((pCpuInfo_g->m_SetPageCode_ForTest==i)&&
            //  (bZeroForTest==TRUE)) /* 励磁用动态试验参数*/
            //     memcpy(buf,pCpuInfo_g->m_ptSetPage[i].m_ptSet[j].m_aucDefaultVal,4);
            //  else/*整定值*/  /* 正常*/
            //     memcpy(buf,pCpuInfo_g->m_ptSetPage[i].m_ptSet[j].m_currentVal,4);
            tbuf[0]=cbuf[3];
            tbuf[1]=cbuf[2];
            tbuf[2]=cbuf[1];
            tbuf[3]=cbuf[0];
            memcpy(buf,tbuf,4);
            cbuf+=4;
            buf+=4;
        }
        i++;
    }
    *buf++=0x84;
    *buf++=0x00;
    *buf++=0x00;
    *buf++=0x7B;

    fileLen = buf - ptBuf ;
    if(fileLen>=(SET_FILE_LENGTH1-1))
        assert(FALSE);
    retcode =EdpCan_WriteValidSettingFile(NORMAL_SETTING1,zoneCode,ptBuf,fileLen);
    if(ptBuf!=NULL)
        free(ptBuf);
    return retcode ;
}

uint8_t combuf[512];

void edpcan_test()
{
    uint16_t i;
    for(i=0; i<512; i++)
    {
        combuf[i]=i/4;
    }
    EdpCan_WriteSettings_From_Buf(0,combuf);
}


/* Check if setting area file is valid.
 * Parameters:
 *      iFd, file descriptor opened previously.
 * Return value:
 *      TRUE, the setting area file is valid.
 *      FALSE, the setting area file is NOT valid.
 * Alert:
 *      Current position of the file is changed in this function. */
extern BOOL SC_Is_Valid_Set(int iFd);






/***********************************************************************
* SC_Del_Set_Area - 删除定值区
*
*		iArea:     定值区号
* RETURNS:
*       		   EP_SUCCESS, delete OK.
*               EP_PARM_ERR, iArea is the working area, can't delete.
*               EP_FILE_ERR, the area file not exists.
*
*/
extern EP_STATUS SC_Del_Set_Area(int iArea	);

/*切换运行定值区*/
extern EP_STATUS SC_Chg_Work_Area(int iArea);

/*获取当前运行定值区*/
extern int SC_Work_Set_Area(void);




/*获取当前实际运行定值区*/
extern int SC_Real_Work_Set_Area(void);


/***********************************************************************
* SC_Get_Valid_Area - Get every valid setting area number.
*	pucRslt			to save setting area number result.
* RETURNS: Number of total valid setting area.
*
*/
extern int SC_Get_Valid_Area(	uint8_t *pucRslt);







/************************网络相关*********************************************************/

/*
typedef struct
{
  	int iNetSeqNo;
  	uint8_t aucIpAddr[4];
   	uint8_t aucIpMsk[4];
   	uint8_t aucMacAddr[6];

}ONE_NET_CFG_INFO;

typedef struct
{
  	int iValidNetNum;
  	ONE_NET_CFG_INFO NetInfArr[MAX_EDP_NET_NUM];
}EDP_NET_CFG_INFO;
*/
/***********************************************************************
* NT_GetNetRunCfg - 获得网络实际运行配置
*
* RETURNS: EP_SUCCESS: 读取成功
*                 其他, 读取失败
*
*/
extern EP_STATUS NT_GetNetRunCfg(		EDP_NET_CFG_INFO *pRtNetInfo	);


/***********************************************************************
* NT_SetOneNetIpAddr - 设置某网口的IP地址，须重启后才起作用
*
*	iNetSeqNo,					网络号，从0开始
*	pIpAddrBase					IP地址串基址
* RETURNS: EP_SUCCESS: 读取成功
*                 其他, 读取失败
*
*/
extern EP_STATUS NT_SetOneNetIpAddr(		int iNetSeqNo,	uint8_t *pIpAddrBase);


/*功能,设置某网口的MAC地址，目前由IP地址自动生成，不额外配置。
  参数,  iNetSeqNo,网络号,从0开始
         pMacAddrBase,Mac地址串基址*/
extern EP_STATUS   NT_SetOneNetMacAddr(int  iNetSeqNo,uint8_t *pMacAddrBase);


/***********************************************************************
 * 设置某网口的IP子网掩码
 * 参数:
 * iNetSeqNo, 网络号, 从0开始
 * pIpMskBase, 子网掩码串基址
 */
extern EP_STATUS NT_SetOneNetIpMsk(int iNetSeqNo, uint8_t *pIpMskBase);













/*************************时间相关**********************************************/



/***********************************************************************
* Get system time(from GPS or a master station).
 * Parameters:
 *      pdttmNow, structure to save the date/time.
 * Return value:
 *      EP_SUCCESS, successful get the date/time.
 *      EP_LOCAL_MSG, system date/time not checked for long time.
 *      EP_NOT_INIT, system time was never checked from CPU reset.
 *      EP_HARD_ERR, hardware error(such as the crystal not working.
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
extern EP_STATUS TM_Get_Sys_Time(EP_DATE_TIME *pdttmNow);


/* Set the system time.
 * Parameters:
 *      pdttmSet, date time want to set(week day vlue is not care).
 *      bPecision, flag of if the time is pecision.
 * Return value:
 *      EP_SUCCESS, successful set the clock.
 *      EP_HARD_ERR, hardware error(such as the crystal not working).
 *      EP_SYS_ERR, other unexpected system error.
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
extern EP_STATUS TM_Set_Sys_Time(const EP_DATE_TIME *pdttmSet, BOOL bPecision);

extern STATUS set_date(int year, int month, int day, int hour, int minute, int second);

/***********************************************************************
* GetAdjustTimeSuccessFlag -  MMI_SOFT.C提供的对时成功标志,
*
* RETURNS:
*				   TRUE, 初始化后对时成功
*					FALSE, 初始化后对时还没有成功
*
*/
extern BOOL GetAdjustTimeSuccessFlag();









/************************压板相关***********************************************/







/*获取当前的压板序列号*/
extern SC_LINK_ITEM* SC_Get_Sw_Link(int iIdx);

/*获取当前的压板状态*/
uint8_t EdpCan_Get_Sw_Status(int iIdx)
{
    SC_LINK_ITEM * plink;
    plink = SC_Get_Sw_Link(iIdx);
    return plink->bSwVal;
}


/*修改压板状态
 	iIdx    压板序号
 	bSts		压板状态
*/
extern EP_STATUS SC_Chg_Sw_Link(int iIdx, BOOL bSts);


/*
	获取总的压板模式
	ulTotalLinkMode 					总模式
*/
extern EP_STATUS SC_Get_Link_Mode_Sts(		uint16_t *ulTotalLinkMode		);



/*
Description: change the content of the /set/set/edplinkmode.set
iIdx: represent the certain link idx, only valid when bTotalFlag==false
ulMode: the link's mode to be set.
bTotalFlag:  if false, ucMode represent certain link's mode
             if true, ucMode represent total link's mode
*/
extern EP_STATUS SC_Chg_Link_Mode_File(int iIdx, uint8_t ucMode, BOOL bTotalFlag);


/* 获取压板状态.
 * Parameters:
 *      iIdx, link index(from 0).
 *      pbRslt, to save return result.
 * Return value:
 *      EP_SUCCESS, read status OK.
 *      EP_BAD_DATA, invalid index(>=iLinkNum_g). */
extern EP_STATUS SC_Get_Link_Now_Sts(int iIdx,		BOOL *pbRslt);



/* DQ:
 * Get hard link status.获取硬压板状态
 * Parameters:
 *      iIdx, link index(from 0).
 *      pbRslt, to save return result.
 * Return value:
 *      EP_SUCCESS, read status OK.
 *      EP_BAD_DATA, invalid index(>=iLinkNum_g)
 *      EP_PARM_ERR, link is not relevent. */
extern EP_STATUS SC_Get_Link_HW_Sts(	int iIdx,	BOOL *pbRslt);

/* Get link attribution.
 * Parameters:
 *      iIdx, index of the link(from 0).
 * Return value:
 *      Pointer to the link attribution structure.
 *      NULL if iIdx is invalid(>=iLinkNum_g). */
extern const SC_LINK_ITEM *SC_Get_Link_Attr(int iIdx);

/*获取压板的总个数*/
uint16_t EdpCan_Get_Link_Num()
{
    return iLinkNum_g;
}







/*******************测量相关****************************************************/

/*请注意：这里描述的遥测和保护电量就是真正含义*/

/*获取保护电量的配置指针*/
VI_MEA_AI_CFG* EdpCan_Get_Protect_AI_Cfg_Stru()
{
    return pmaicfg_g;
}

/*获取保护电量的数值结构指针*/
VI_MEA_AI_DB* EdpCan_Get_Protect_AI_DB_Stru()
{
    return pmaidb_g;
}

extern ME_MEA_VALUE_CFG *pmeacfg_g;/*来源于measure.c*/

/*获取遥测量的配置指针*/
ME_MEA_VALUE_CFG* EdpCan_Get_Mea_AI_Cfg_Stru()
{
    return pmeacfg_g;
}

extern ME_MEA_AI_DB *pmeadb_g;/*来源于measure.c*/
/*获取遥测量的数值结构指针*/
ME_MEA_AI_DB* EdpCan_Get_Mea_AI_DB_Stru()
{
    return pmeadb_g;
}


/*获取遥测越限的遥测量的个数*/
uint16_t  EdpCan_Get_MeaOver_Data_Num(const VI_RUN_INFO *prunInfo)
{
    uint16_t mea_overflow_num = 0;
    mea_overflow_num=prunInfo->msg.mea.uiNum;
    return 	mea_overflow_num;
}

/*根据参数制定的len长度提供相应个数的结构体数组的头指针*/
uint16_t  EdpCan_Get_MeaOver_Data(const VI_RUN_INFO *prunInfo,uint16_t pos, uint16_t len,ME_MEA_AI_DB *ppmeadb)
{
    ME_MEA_AI_DATA_DB **my_ppmeadb;
    uint16_t i;
    uint16_t mea_overflow_num;
    uint16_t get_len=0;
    my_ppmeadb=prunInfo->msg.mea.ppcfg;
    mea_overflow_num=prunInfo->msg.mea.uiNum;
    if(mea_overflow_num>pos+len)
    {
        get_len = len;
    }
    else
    {
        if( (mea_overflow_num-pos)<=0)
        {
            get_len = 0;
            return get_len;
        }
        get_len = mea_overflow_num-pos;

    }
    ppmeadb = my_ppmeadb[pos];
    return get_len;
}



/*获取全部遥测量个数*/
extern int ME_Get_Msu_Num(void);

/***********************************************************************
* RD_Mea_AI - Read all measurment value
* 读取所有遥测量的数据的结构体（简化带一定描述的数值）
  调用者提供空间
* RETURNS: None
*
* Alert:
*        pmeaRslt must contains space to save iMeaValueNum_g members.
*/
extern void RD_Mea_AI(	RD_AI_MEA *pmeaRslt) ;

/*获取保护电量的总个数*/
uint16_t EdpCan_Get_Yaoce_Num()
{
    return iMeaAiNum_g;
}

/*获取遥测量的总个数*/

extern int ME_Get_Msu_Num(void);
/*
uint16_t EdpCan_Get_Celiang_Num()
{
	return iMeaValueNum;
}*/


/* Read all measurement AIs' value.
   获取遥测量的所有数据，调用者提供空间（只提供值）
 * Parameter:
 *      pfRslt, to save all Measure_AIs' current value.
 * Return value:
 *      None.
 * Alert:
 *      pfRslt must contains space to save iMeaValueNum_g float numbers. */
extern void ME_Rd_Mea_AI_Val(float *pfRslt);

/***********************************************************************
* RD_Mea_Hw_AI - Read all hardware AI measurment value
*	 to save all hardware AI measurement value 读取所有物理通道的值
* RETURNS: None
* 需要注意的是这个函数不是仅仅读取数值，还包含了相当数量的计算
* Alert:
*        phwmeaRslt must contains space to save iHwAiChNum_g members.
*/
extern void RD_Mea_Hw_AI(RD_HW_AI_MEA *phwmeaRslt, BOOL bIsCalc);








/***************************电度相关********************************************/



/***********************************************************************
* GetPoCfgNum - 获取PO配置数
*
* RETURNS: PO配置数
*
*/
extern int GetPoCfgNum(void);


/***********************************************************************
* VI_New_PoClear - PO清零，供mmi调用
*
* RETURNS: TRUE, or FALSE
*
*/
extern BOOL VI_New_PoClear(uint8_t ucObjNum);

/*装置电能清零记日志ok*/
extern void PoClearAdjustToLog(int iChannel);

/***********************************************************************
* RD_Mea_Po - Read all pulse output measurment value
*ppomeaRslt 结构体数组，保存所有的电度数据（调用者提供内存资源)
* RETURNS: None
*
* Alert:
*        ppomeaRslt must contains space to save iLgcPoChNum_g members.
*/
extern void RD_Mea_Po(	RD_PO_MEA *ppomeaRslt	) ;



/*获取电度量的结构指针*/
RD_LGC_PO_CH* EdpCan_Get_Po_Cfg_Stru()
{
    return plgcpoch_g;
}





/*******************系统状态相关**************************************************/




/************************************************************************
  功能：获得装置设备名称，2008-7-25   张云
  参数：ppucRtDevNameAddr: 返回装置设备名称字符串基址，
            调用方，将uint8_t  *类型的变量的地址传过来，
            供返回平台内部维护的设备名称全局字符串地址,
            注意，不发生字符串拷贝操作。
        piRtNameLen，返回设备名称字符串长度(注意，不包括"\0")。

*/
extern EP_STATUS  EP_GetDevName(uint8_t  **ppucRtDevNameAddr,int   *piRtNameLen);




/***********************************************************************
* VI_New_Adjust - 校准命令，供mmi调用

	ucObjType, 			 校准对象类型，0: ai物理通道，1: 测量量
*	ucOrdType			   校准命令类型， 0: 增益校准，1: 偏置校准
* RETURNS: TRUE, or FALSE
*
*/
extern BOOL VI_New_Adjust(	uint8_t ucObjType, 	uint8_t ucOrdType	);


/***********************************************************************
* EP_Clr_Sts_Bit - Clear system status(AND operation)
*
* RETURNS: 无
*
* Alert:
*        This function can be called in ISR.
*
*/
extern void EP_Clr_Sts_Bit(		u_int uiSts	);


/*设置一些系统状态*/
extern void EP_Set_Sts_Bit(	u_int uiSts		);

/***********************************************************************
* VI_New_RepairSts - 检修状态，供MMI调用，根据是否为检修状态完成一定的操作
*
* RETURNS: TRUE, or FALSE
*
*/
extern BOOL VI_New_RepairSts(	uint32_t ulRepairSts	);

/*获取解挂锁状态，1为处于解挂锁状态
uint8_t EdpCan_Get_JGS_Status()
{
	if(uiEdpStatus_g&JGS_STATE){
		return 1;
	}
	else
	{
		return 0;
	}
}
*/
/* Change protect function run/exit status.
 * Parameters:
 *      strName, protect(sub-logic) name.
 *      bSts, new status.
 * Return value:
 *      EP_SUCCESS, change run/exit status OK.
 *      EP_FILE_ERR, file operating failure. */

extern EP_STATUS SC_Chg_Prtc_Sts(const uint8_t *strName, BOOL bSts);


/*
				获取具体子逻辑图的名称和该子逻辑任务是否运行
				 Get sub-logic attribution.
 * Parameters:
 *      iIdx, index of the sub-logic(from 0).
 * Return value:
 *      Pointer to the sub-logic attribution structure.
 *      NULL if iIdx is invalid(>=iSubLgcNum_g). */
extern const SC_SUB_LGC_ITEM *SC_Get_Sub_Lgc_Attr(int iIdx);


/***********************************************************************
* GetRecWrSts - 获取录波状态
*
* RETURNS:
*                TRUE，正在录波
*                FALSE，录波结束
*
*/
extern BOOL GetRecWrSts(void);

/***********************************************************************
* VI_Is_Fault - 返回是否处于故障态，故障态认为是一个特殊状态
*
* RETURNS:
*               TRUE: 是故障态
*               FALSE: 非故障态
*
*/
extern BOOL VI_Is_Fault(void);

/***********************************************************************
* EP_Get_Repair_Sts - Inquiring about if the system is in examination state
*
* RETURNS: None
*
*
*/
extern unsigned char EP_Get_Repair_Sts(	unsigned char *pbRtRepairSts);

/***********************************************************************
* ER_IsSetAlertFlag - 获得是否设置呼唤标志
*
* RETURNS: TRUE: 已经设置呼唤
*                 FALSE: 未设置呼唤
*
*/
extern BOOL ER_IsSetAlertFlag();

/*获取具体的装置的单个状态*/
uint8_t EdpCan_Get_Ied_Single_Status(u_int mode)
{
    if(uiEdpStatus_g&mode)
    {
        return 1;
    }
    else
    {
        return 0;
    }

}

/***********************************************************************
* EP_Bgn_Hw_Test - Enter hardware test mode.  Logic function will be disabled
*
* RETURNS: 无
*
*/
extern void EP_Bgn_Hw_Test(void);


/***********************************************************************
* EP_End_Hw_Test - Exit hardware test mode.  System will reboot after several seconds
*
* RETURNS: 无
*
*/
extern void EP_End_Hw_Test(void);















/***************************DI部分处理****************************************/



/***********************************************************************
* VI_Rd_Mea_DI_Val - Read all measurement DIs' value.
*	pbRslt		 to save all MEA_DIs' current value.这是一个布尔数组
* RETURNS: 无
*
* alert:
* 		pbRslt must contains space to save iMeaDiNum_g BOOL numbers.
*/
extern void VI_Rd_Mea_DI_Val(
    BOOL *pbRslt,		/* to save all MEA_DIs' current value. */
    uint16_t *pQuality
);


/*获取遥信量的总个数*/
uint16_t EdpCan_Get_Mea_Di_Num()
{
    return iMeaDiNum_g;
}

VI_MEA_DI_CFG* EdpCan_Get_Mea_Di_Stru()
{
    return pmdicfg_g;
}



/* Read hardware DI measurement value.
 * Parameters:
 *      iIdx, index of DI(from 0).
 * Return value:
 *      Low 15 bit is now status(TRUE or FALSE) while bit15(0x8000) is flag of
 *          force DI. */
extern int RD_Mea_Hw_DI(int iIdx);


/* Change force DI status.
 * Parameters:
 *      iIdx, index of DI(from 0).
 *      iSts, new status: TRUE, FALSE or -1 means release force.
 * Return value:
 *      EP_SUCCESS, change link status OK.
 *      EP_FILE_ERR, file operating failure. */
extern EP_STATUS RD_Chg_Force_DI(int iIdx, int iSts);



/********************************end*********************************************/









/**************某种保护类别是否投入运行***************************/


/* Get sub-logic attribution.
 * Parameters:
 *      iIdx, index of the sub-logic(from 0).
 * Return value:
 *      Pointer to the sub-logic attribution structure.
 *      NULL if iIdx is invalid(>=iSubLgcNum_g). */
extern const SC_SUB_LGC_ITEM *SC_Get_Sub_Lgc_Attr(int iIdx);








/***********以下为临时的函数，最终没有******************/

/* Finish writing a setting area.
 * Parameters:
 *      iArea, setting area number.
 * Return value:
 *      EP_SUCCESS, or EP_ERROR. */
extern EP_STATUS SC_End_Wr_Set(int iArea);



#endif

/* 查看遥信量 */
void VI_Show_Mea_DI_Val(void)
{
    VI_MEA_DI_DB *pmdidb = NULL;

    for (pmdidb = pmdidb_g; pmdidb<pmdidb_g+iMeaDiNum_g; pmdidb++)
    {
        logMsg("%d %d %d\n", (int)(pmdidb-pmdidb_g),
               pmdidb->bVal, pmdidb->usQuality, 0, 0, 0);
    }
}
