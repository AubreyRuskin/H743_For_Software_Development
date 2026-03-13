/* errtest.c - subroutine library for handling interface to report system errors */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02a, 20jun09, dy add ER_Set_Err_Multi_Para and ER_Set_Err_Stat functions.
01c, 29oct02, hdx Updated to version 1.0.
01b, 29aug02, hdx Verified version 0.1.
01a, 27jul02, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This module includes subroutine library for handling interface to report system errors.
INCLUDE: errtest.h
*/

/* includes */

#include "errtest.h"
#include "logmsg.h"
#include "view.h"
#include <stdio_compat.h>
#include "GO_Interface.h"
#include "HDL_VtBox.h"
#include "GooseInterface.h"
#include "logLib.h"

/* locals */

static uint64_t ulSysErr_g;		/* 改为32位 */
static BOOL abSysErr_g[MAX_SYS_ERR_NUM];

static uint64_t GoErrNetStats[MAX_EXTERN_PORT_NUM];
static BOOL GoErrConfiStats[MAX_EXTERN_PORT_NUM*MAX_ALLOW_SUB_GO_NUM]; /* 数据异常错误 */

static BOOL bAlertFuncIsInit_g = FALSE;			/* 呼唤功能是否已经初始化 */
BOOL bAlertIsSet_g = FALSE;   	 /* 呼唤功能是否设置 2011-7-27  zy*/
static BOOL bPlatFormAlertSet_g = FALSE;        /*平台是否呼唤*/
static ALERT_REG_INFO AlertRegInfo_g;	  /* 呼唤注册信息 */
BOOL bHmiAlertSet_g = FALSE; /* HMI是否呼唤 */
BOOL g_bAlertLightOn = FALSE; /* 平台是否呼唤 */
/* globals */

u_int uiEvtTimes_g;
uint64_t SysErrEnableFlag_g = 0;
uint16_t SysMaxErrNum_g = 0;

char SysErrorName[LANGUAGE_TYPE_NUM][MAX_SYS_ERR_NUM][MESSAGE_MAX_LEN] =
{
    {
        "装置上电",
        "存储器错误",
        "运行定值区无效",
        "定值校验错误",
        "开入开出异常",
        "采样异常",
        "程序校验错误",
        "监视模块告警",
        "扩展机箱告警",
        "FPGA接口错误",
        "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "",
        "",
        "GOOSE的A网接收中断",
        "GOOSE的B网接收中断",
        "GOOSE的CONFVER配置错误",
        "GOOSE的C网接收中断",
        "GOOSE的D网接收中断",
        "GOOSE的E网接收中断",
        "GOOSE的F网接收中断",
        "GOOSE的G网接收中断",
        "GOOSE的H网接收中断",
        "GOOSE的I网接收中断",
        "GOOSE的J网接收中断",
        "GOOSE的K网接收中断"
    },
    {
        "Device powered on",
        "Storage unit error",
        "Invalid sector",
        "Setting check error",
        "Digital DI/DO error",
        "Sampling unit error",
        "Software/configuration file check error",
        "Monitoring CPU alarm",
        "Extended box alarm",
        "FPGA interface error",
        "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "",
        "",
        "Relay GOOSE A interrupted",
        "Relay GOOSE B interrupted",
        "Relay GOOSE configuration error",
        "Relay GOOSE C interrupted",
        "Relay GOOSE D interrupted",
        "Relay GOOSE E interrupted",
        "Relay GOOSE F interrupted",
        "Relay GOOSE G interrupted",
        "Relay GOOSE H interrupted",
        "Relay GOOSE I interrupted",
        "Relay GOOSE J interrupted",
        "Relay GOOSE K interrupted"
    }
};

/* HMI设置异常标识 */
BOOL g_baHmiSetErr[MMI_S_AE_HINT_INFO];

/* global functions */

/* 报告错误.
 * Para:
 *     unErrCode, 错误代码，需使用预定义的符号.
 *     unOpFlag, 对错误做出的反应.
 *     strFmt, 欲显示的格式字符串。类似printf格式串，但不支持浮点数.
 *     iArg1, 格式字符串中指定要输出的参数，如不足2个，最后应补0.
 *     iArg2, 格式字符串中指定要输出的参数.
 * Return:
 *     NONE.
 * 注意:
 *     此函数可以在中断的上下文中调用
 *     unOpFlag是对错误作出的反应，这是一个位标志，可以根据需要把它们
 *     或起来（当标志为0时最基本的错误反应是记录到日志中).
 *
 */
void ER_Set_Err(uint16_t unErrCode, uint16_t unOpFlag, const uint8_t *strFmt, int iArg1, int iArg2)
{
    // VI_RUN_INFO *pinf;
    // int iLockKey;
    // uint8_t TempInfo[256];
    // uint64_t ullShiftBit=1;

    // assert((unErrCode<MAX_SYS_ERR_NUM)||(unErrCode==MMI_S_AE_HINT_INFO));

    // /* Set system run mode. */
    // iLockKey = intLock();

    // if (unOpFlag & ER_LOCK)
    //     EP_Set_Sts_Bit(SYS_LOCK_DO);

    // if (unOpFlag & ER_VERSION)
    //     EP_Set_Sts_Bit(VERSION_NOT_MATCHED_FLAG);

    // if (unOpFlag & ER_SCI_CHANGE)
    //     EP_Set_Sts_Bit(SCI_CHANGED_FLAG);

    // if (unOpFlag & ER_DIGI_SAMPLE)
    //     EP_Set_Sts_Bit(DIGITAL_SAMPLE_ERR_FLAG);

    // if (unOpFlag & ER_ALERT)
    //     bPlatFormAlertSet_g = TRUE;


    // if (unOpFlag & ER_ALARM)
    // {
    //     EP_Set_Sts_Bit(SET_ALARM_FLAG);
    // }

    // intUnlock(iLockKey);

    // if(unErrCode!=MMI_S_AE_HINT_INFO)
    // {
    //     abSysErr_g[unErrCode] = TRUE;
    //     ulSysErr_g |= (ullShiftBit << unErrCode);		/* ZY 2010年12月30日 修改以前超过32位(GOOSE B网中断)缺陷 */

    // }

    // pinf = VI_Run_Info_Wr_P();  /* 获取报告存储空间 */

    // if (bViewModIsInit_g)	/* VI模块是否完成标志 */
    // {
    //     pinf->bViewModIsInit = TRUE;
    // }
    // else
    // {
    //     pinf->bViewModIsInit = FALSE;
    // }

    // pinf->type = ERR_OCR;

    // pinf->msg.err.unErrCode = unErrCode;
    // pinf->msg.err.unOpFlag = unOpFlag;
    // pinf->msg.err.ulTime = TM_Get_usCnt();

    // sprintf(pinf->msg.err.aucNote, strFmt, iArg1, iArg2);

    // iLockKey = intLock();

    // if (rptsts_g.iFault)
    // {
    //     /* 如果处于故障启动态，则使用现有的报告号和录波号 */
    //     pinf->msg.err.unRptSN = rptsts_g.unRptSN;
    //     pinf->msg.err.ucRecSN = rptsts_g.ucRecSN;
    // }
    // else
    // {
    //     /* 否则使用新的报告号和录波号 */
    //     pinf->msg.err.unRptSN = rptsts_g.unRptSN++;
    //     pinf->msg.err.ucRecSN = 0;
    // }

    // intUnlock(iLockKey);

    // VI_End_Wr_Run_Info();  /* 释放信号量，结束故障报告 */
    // if (unOpFlag & ER_NOLOGWRITE)
    //     return;
    // if(unErrCode==MMI_S_AE_HINT_INFO)
    // {
    //     sprintf(TempInfo, "%s:%s", (ENG_MODE==0)?"提示信息":"HINT", pinf->msg.err.aucNote);
    // }
    // else
    // {
    //     sprintf(TempInfo, "%s:%s", SysErrorName[ENG_MODE][unErrCode], pinf->msg.err.aucNote);
    // }
    // LOG_Write(LOG_KERNEL, TempInfo, NULL);		/* 日志记录 */
}

/* 报告错误(带状态，目前提供动作和返回).
 * Para:
 *     unErrCode, 错误代码，需使用预定义的符号.
 *     unOpFlag, 对错误做出的反应.
 *     strFmt, 欲显示的格式字符串。类似printf格式串，但不支持浮点数.
 *     iArg1, 格式字符串中指定要输出的参数，如不足2个，最后应补0.
 *     iArg2, 格式字符串中指定要输出的参数.
 *     ulSts, 0: 返回; 1: 动作.
 * Return:
 *     NONE.
 * 注意:
 *     此函数可以在中断的上下文中调用
 *     unOpFlag是对错误作出的反应，这是一个位标志，可以根据需要把它们
 *     或起来（当标志为0时最基本的错误反应是记录到日志中).
 *
 */
void ER_Set_Err_Stat(uint16_t unErrCode, uint16_t unOpFlag, const uint8_t *strFmt,
                     int iArg1, int iArg2, uint32_t ulSts, uint32_t ulUserInfo)
{
    // VI_RUN_INFO *pinf;
    // int iLockKey;
    // uint8_t TempInfo[256];
    // uint64_t ullShiftBit=1;
    // int32_t i = 0;

    // /* 支持提示信息 */
    // assert ((unErrCode<MAX_SYS_ERR_NUM)
    //         || (unErrCode == MMI_S_AE_MMI_ERR));

    // /* Set system run mode. */
    // iLockKey = intLock();

    // if (unOpFlag & ER_LOCK)
    //     EP_Set_Sts_Bit(SYS_LOCK_DO);

    // if (unOpFlag & ER_VERSION)
    //     EP_Set_Sts_Bit(VERSION_NOT_MATCHED_FLAG);

    // if (unOpFlag & ER_SCI_CHANGE)
    //     EP_Set_Sts_Bit(SCI_CHANGED_FLAG);
    // if (unOpFlag & ER_ALERT)
    //     bPlatFormAlertSet_g = TRUE;

    // /* 设置HMI呼唤态, 不和平台冲突
    //  */
    // if (unErrCode == MMI_S_AE_MMI_ERR)
    // {
    //     if (unOpFlag & ER_ALERT)
    //     {
    //         if (ulSts)
    //         {
    //             bHmiAlertSet_g = TRUE;
    //         }
    //         else
    //         {
    //             bHmiAlertSet_g = FALSE;
    //         }
    //     }
    // }

    // if (unOpFlag & ER_ALARM)
    // {
    //     EP_Set_Sts_Bit(SET_ALARM_FLAG);
    // }

    // if (unOpFlag & ER_DIGI_SAMPLE)
    // {
    //     if(ulSts)
    //         EP_Set_Sts_Bit(DIGITAL_SAMPLE_ERR_FLAG);
    //     else
    //         EP_Clr_Sts_Bit(DIGITAL_SAMPLE_ERR_FLAG);
    // }
    // intUnlock(iLockKey);

    // /* 提示信息不支持多点触发
    //  */
    // if (unErrCode != MMI_S_AE_MMI_ERR)
    // {
    //     if (ulSts == 1)
    //     {
    //         if((unErrCode>=EV_REL_GSE_A_NET_HALT)&&(unErrCode<=EV_REL_GSE_B_NET_HALT))
    //         {
    //             GoErrNetStats[unErrCode-EV_REL_GSE_A_NET_HALT] |= (1LL<<ulUserInfo);
    //         }
    //         else if((unErrCode>=EV_REL_GSE_C_NET_HALT)&&(unErrCode<=EV_REL_GSE_K_NET_HALT))
    //         {
    //             GoErrNetStats[unErrCode-EV_REL_GSE_C_NET_HALT+2] |= (1LL<<ulUserInfo);
    //         }
    //         abSysErr_g[unErrCode] = TRUE;
    //         ulSysErr_g |= (ullShiftBit << unErrCode);	 /* ZY 2010年12月30日 修改以前超过32位(GOOSE B网中断)缺陷 */

    //         if (unErrCode == EV_GSE_CONFI_ERR)
    //         {
    //             GoErrConfiStats[ulUserInfo] = TRUE; /* 数据异常 */
    //         }
    //     }
    //     else if (ulSts == 0)
    //     {
    //         if((unErrCode>=EV_REL_GSE_A_NET_HALT)&&(unErrCode<=EV_REL_GSE_B_NET_HALT))
    //         {
    //             GoErrNetStats[unErrCode-EV_REL_GSE_A_NET_HALT] &= ~(1LL<<ulUserInfo);
    //             if(GoErrNetStats[unErrCode-EV_REL_GSE_A_NET_HALT]==0)
    //             {
    //                 abSysErr_g[unErrCode] = FALSE;
    //                 ulSysErr_g &= ~(ullShiftBit << unErrCode);	/* ZY 2010年12月30日 修改以前超过32位(GOOSE B网中断)缺陷 */
    //             }
    //         }
    //         else if((unErrCode>=EV_REL_GSE_C_NET_HALT)&&(unErrCode<=EV_REL_GSE_K_NET_HALT))
    //         {
    //             GoErrNetStats[unErrCode-EV_REL_GSE_C_NET_HALT+2] &= ~(1LL<<ulUserInfo);
    //             if(GoErrNetStats[unErrCode-EV_REL_GSE_C_NET_HALT+2]==0)
    //             {
    //                 abSysErr_g[unErrCode] = FALSE;
    //                 ulSysErr_g &= ~(ullShiftBit << unErrCode);	/* ZY 2010年12月30日 修改以前超过32位(GOOSE B网中断)缺陷 */
    //             }
    //         }
    //         else if (unErrCode == EV_GSE_CONFI_ERR)
    //         {
    //             /* 数据异常 */
    //             BOOL bSts = FALSE;

    //             GoErrConfiStats[ulUserInfo] = FALSE;

    //             /* 所有间隔/网络是否都退出 */
    //             for (i = 0; i<iHdlSubGoNum_g*iHdlNetNum_g; i++)
    //             {
    //                 if (GoErrConfiStats[i])
    //                 {
    //                     bSts = TRUE;
    //                     break;
    //                 }
    //             }

    //             if (!bSts)
    //             {
    //                 abSysErr_g[unErrCode] = FALSE;
    //                 ulSysErr_g &= ~(ullShiftBit << unErrCode);
    //             }
    //         }
    //         else
    //         {
    //             abSysErr_g[unErrCode] = FALSE;
    //             ulSysErr_g &= ~(ullShiftBit << unErrCode);	/* ZY 2010年12月30日 修改以前超过32位(GOOSE B网中断)缺陷 */
    //         }
    //     }
    // }

    // /* 是否保存到实时/历史事件记录
    //  * 包括事件和日志
    //  */
    // if (unOpFlag & ER_REPORT)
    // {
    //     pinf = VI_Run_Info_Wr_P();    /* 获取报告存储空间 */

    //     if (bViewModIsInit_g)  /* VI模块是否完成标志 */
    //     {
    //         pinf->bViewModIsInit = TRUE;
    //     }
    //     else
    //     {
    //         pinf->bViewModIsInit = FALSE;
    //     }

    //     pinf->type = ERR_OCR_STAT;

    //     pinf->msg.errstat.unErrCode = unErrCode;
    //     pinf->msg.errstat.unOpFlag = unOpFlag;
    //     pinf->msg.errstat.ulTime = TM_Get_usCnt();
    //     pinf->msg.errstat.ulSts = ulSts;

    //     sprintf(pinf->msg.errstat.aucNote, strFmt, iArg1, iArg2);

    //     iLockKey = intLock();

    //     if (rptsts_g.iFault)
    //     {
    //         /* 如果处于故障启动态，则使用现有的报告号和录波号 */
    //         pinf->msg.errstat.unRptSN = rptsts_g.unRptSN;
    //         pinf->msg.errstat.ucRecSN = rptsts_g.ucRecSN;
    //     }
    //     else
    //     {
    //         /* 否则使用新的报告号和录波号 */
    //         pinf->msg.errstat.unRptSN = rptsts_g.unRptSN++;
    //         pinf->msg.errstat.ucRecSN = 0;
    //     }

    //     intUnlock(iLockKey);

    //     VI_End_Wr_Run_Info();	/* 释放信号量，结束故障报告 */
    //     if (unOpFlag & ER_NOLOGWRITE)
    //         return;

    //     /* 提示信息单独列出, 防止越界
    //      */
    //     if (unErrCode == MMI_S_AE_MMI_ERR)
    //     {
    //         sprintf(TempInfo, "%s:%s", (ENG_MODE == 0) ? "HMI模件异常" : "HMI Module Error",
    //                 pinf->msg.err.aucNote);
    //     }
    //     else
    //     {
    //         sprintf(TempInfo, "%s:%s", SysErrorName[ENG_MODE][unErrCode],
    //                 pinf->msg.err.aucNote);
    //     }

    //     LOG_Write(LOG_KERNEL, TempInfo, NULL);		/* 日志记录 */
    // }
}

/* report error with multi parameter.
 * Para:
 *     unErrCode, error code.
 *     unOpFlag, error level.
 *     strFmt, formatting string.
 *     iArg1, parameter 1;
 *     iArg2, parameter 2;
 *     iArg3, parameter 3;
 *     iArg4, parameter 4;
 *     iArg5, parameter 5;
 *     iArg6, parameter 6;
 * Return:
 *     NONE.
 */
void ER_Set_Err_Multi_Para(uint16_t unErrCode, uint16_t unOpFlag, const uint8_t *strFmt,
                           int iArg1, int iArg2, int iArg3, int iArg4, int iArg5, int iArg6)
{
    // VI_RUN_INFO *pinf;
    // int iLockKey;
    // uint8_t TempInfo[256];
    // uint64_t ullShiftBit=1;

    // assert (unErrCode<MAX_SYS_ERR_NUM);

    // /* Set system run mode. */

    // iLockKey = intLock();

    // if (unOpFlag & ER_LOCK)
    //     EP_Set_Sts_Bit(SYS_LOCK_DO);

    // if (unOpFlag & ER_VERSION)
    //     EP_Set_Sts_Bit(VERSION_NOT_MATCHED_FLAG);

    // if (unOpFlag & ER_SCI_CHANGE)
    //     EP_Set_Sts_Bit(SCI_CHANGED_FLAG);

    // if (unOpFlag & ER_ALARM)
    // {
    //     EP_Set_Sts_Bit(SET_ALARM_FLAG);
    // }

    // intUnlock(iLockKey);

    // abSysErr_g[unErrCode] = TRUE;
    // ulSysErr_g |= (ullShiftBit << unErrCode);		/* ZY 2010年12月30日 修改以前超过32位(设置GOOSE B网中断)缺陷 */

    // pinf=VI_Run_Info_Wr_P();		/* get the memory space for report. */

    // if (bViewModIsInit_g)				/* judge if the VIEW module have been finished. */
    // {
    //     pinf->bViewModIsInit = TRUE;
    // }
    // else
    // {
    //     pinf->bViewModIsInit = FALSE;
    // }

    // pinf->type = ERR_OCR;

    // pinf->msg.err.unErrCode = unErrCode;
    // pinf->msg.err.unOpFlag = unOpFlag;
    // pinf->msg.err.ulTime = TM_Get_usCnt();

    // sprintf(pinf->msg.err.aucNote, strFmt, iArg1, iArg2, iArg3, iArg4, iArg5, iArg6);

    // iLockKey = intLock();

    // if (rptsts_g.iFault)
    // {
    //     /* if QD. */
    //     pinf->msg.err.unRptSN = rptsts_g.unRptSN;
    //     pinf->msg.err.ucRecSN = rptsts_g.ucRecSN;
    // }
    // else
    // {
    //     pinf->msg.err.unRptSN = rptsts_g.unRptSN++;
    //     pinf->msg.err.ucRecSN = 0;
    // }

    // intUnlock(iLockKey);

    // VI_End_Wr_Run_Info();
    // if (unOpFlag & ER_NOLOGWRITE)
    //     return;

    // sprintf(TempInfo, "%s:%s", SysErrorName[ENG_MODE][unErrCode], pinf->msg.err.aucNote);
    // LOG_Write(LOG_KERNEL, TempInfo, NULL);		/* 日志记录 */
}

/*
子单元CPU告警
strFmt 欲显示的格式化字符串
iArg1 欲显示的格式字符串。类似printf格式串，但不支持浮点数, 要求长度小于200字节
iArg2 格式字符串中指定要输出的参数，如不足2个，最后应补0
*/
void ER_App_Set_Ext_Alarm(
    const uint8_t *strFmt,
    int iArg1,
    int iArg2,
    uint16_t unOpFlag)
{
    ER_Set_Err(EV_EXT_COM_ALARM, unOpFlag, strFmt,iArg1,iArg2);
}


void ER_App_Set_Digital_AD_Err(
    const uint8_t *strFmt, 			/* 欲显示的格式字符串。类似printf格式串，但不支持浮点数, 要求长度小于200字节 */
    int iArg1, 		/* 格式字符串中指定要输出的参数，如不足2个，最后应补0 */
    int iArg2,
    uint16_t unOpFlag)
{
    ER_Set_Err_Stat(EV_SAMPLE_ERR, unOpFlag, strFmt,iArg1,iArg2,1,0);
}

void ER_App_Clear_Digital_AD_Err(
    const uint8_t *strFmt, 			/* 欲显示的格式字符串。类似printf格式串，但不支持浮点数, 要求长度小于200字节 */
    int iArg1, 		/* 格式字符串中指定要输出的参数，如不足2个，最后应补0 */
    int iArg2,
    uint16_t unOpFlag)
{
    ER_Set_Err_Stat(EV_SAMPLE_ERR, unOpFlag, strFmt,iArg1,iArg2,0,0);
}

void ER_App_Set_Digital_AD_Alert(
    const uint8_t *strFmt, 			/* 欲显示的格式字符串。类似printf格式串，但不支持浮点数, 要求长度小于200字节 */
    int iArg1, 		/* 格式字符串中指定要输出的参数，如不足2个，最后应补0 */
    int iArg2,
    uint16_t unOpFlag)
{
    ER_Set_Err_Stat(EV_SAMPLE_ERR, unOpFlag, strFmt,iArg1,iArg2,1,0);
}

void ER_App_Clear_Digital_AD_Alert(
    const uint8_t *strFmt, 			/* 欲显示的格式字符串。类似printf格式串，但不支持浮点数, 要求长度小于200字节 */
    int iArg1, 		/* 格式字符串中指定要输出的参数，如不足2个，最后应补0 */
    int iArg2,
    uint16_t unOpFlag)
{
    ER_Set_Err_Stat(EV_SAMPLE_ERR, unOpFlag, strFmt,iArg1,iArg2,0,0);
}

/***********************************************************************
* ER_Sys_Err_Sts - Get current status of system error.
*
* RETURNS:
*					TRUE, the error occured.
*					FALSE, the error not occured.
*
*/
BOOL ER_Sys_Err_Sts(
    int iIdx		/* index of system error(reserved error code */
)
{
    assert(iIdx<MAX_SYS_ERR_NUM);

    return abSysErr_g[iIdx];
}

/***********************************************************************
* GetSysErrFlag - 获取全局系统错误状态
*
* RETURNS: 错误状态
*
*/
uint64_t GetSysErrFlag(void)
{
    return ulSysErr_g;
}

/***********************************************************************
* GetSysErrFlagRelayStop - 获取系统错误状态(保护退出)
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL GetSysErrFlagRelayStop()
{
    if((ulSysErr_g&(1L<<EV_STORAGE_ERR))
            || (ulSysErr_g&(1L<<EV_DIDO_ERR))
            || (ulSysErr_g&(1L<<EV_SET_ERR))
            || (ulSysErr_g&(1L<<EV_SAMPLE_ERR))
            || (ulSysErr_g&(1L<<EV_EXT_COM_ALARM))
            || (ulSysErr_g&(1L<<EV_WATCH_CPU_ALARM))
            || (ulSysErr_g&(1L<<EV_FPGA_ERR)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/***********************************************************************
* GetSysErrFlagRelayContinue - 获取系统错误状态(保护不退出)
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL GetSysErrFlagRelayContinue()
{
    if(
        (ulSysErr_g&(1LL<<EV_SECT_ERR))
        || (ulSysErr_g&(1LL<<EV_POWER_ON))
        || (ulSysErr_g&(1LL<<EV_REL_GSE_A_NET_HALT))
        || (ulSysErr_g&(1LL<<EV_REL_GSE_B_NET_HALT))
        || (ulSysErr_g&(1LL<<EV_GSE_CONFI_ERR))
        || (ulSysErr_g&(1LL<<EV_REL_GSE_C_NET_HALT))
        || (ulSysErr_g&(1LL<<EV_REL_GSE_D_NET_HALT))
        || (ulSysErr_g&(1LL<<EV_REL_GSE_E_NET_HALT))
        || (ulSysErr_g&(1LL<<EV_REL_GSE_F_NET_HALT))
        || (ulSysErr_g&(1LL<<EV_REL_GSE_G_NET_HALT))
        || (ulSysErr_g&(1LL<<EV_REL_GSE_H_NET_HALT))
        || (ulSysErr_g&(1LL<<EV_REL_GSE_I_NET_HALT))
        || (ulSysErr_g&(1LL<<EV_REL_GSE_J_NET_HALT))
        || (ulSysErr_g&(1LL<<EV_REL_GSE_K_NET_HALT)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/***********************************************************************
* GetGlobalErrorFlag - 设置逻辑图扫描获得的错误变量
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL GetGlobalErrorFlag(void)
{
    if(ulSysErr_g)
        return TRUE;
    else
        return FALSE;
}

/****************************************呼唤相关函数********************************************************/

/***********************************************************************
* ER_InitAlertFunc - 初始化呼唤功能，必须在初始化函数中被调用
*
* RETURNS: 无
*
*/
void ER_InitAlertFunc(void)
{
    /* 需要初始化相应IO */
    bAlertFuncIsInit_g = TRUE;
    AlertRegInfo_g.iRegAlertCnt = 0;
}

/***********************************************************************
* ER_RegAlertSignal - 注册某应用对应的呼唤
*
* RETURNS: 该呼唤对应的操作句柄，若为NULL，表示注册未成功
*
* 注意: 呼唤可由多个应用驱动
*              任何一个应用发出呼唤，则驱动呼唤继电器，只有所有呼唤都收回，才收回呼唤驱动继电器
*              该函数由需要发出呼唤的相关应用初始化时，调用一次。得到句柄。
*              以后设置和收回呼唤，就由该对应句柄来进行操作
*
*/
void *ER_RegAlertSignal(void)
{
    if(!bAlertFuncIsInit_g)
    {
        /* 若呼唤功能未初始化，则返回失败 */
        return NULL;
    }

    if(AlertRegInfo_g.iRegAlertCnt >= MAX_REG_ALERT_NUM)
    {
        /* 若呼唤功能个数超出限制，则返回失败 */
        logMsg("Alert Register  failure  for  too much.\n", 0, 0, 0, 0, 0, 0);
        if(ENG_MODE==1)
            LOG_Write(LOG_KERNEL, " Register  failure  for  too much Alert.\n", NULL);
        else if(ENG_MODE==0)
            LOG_Write(LOG_KERNEL, "呼唤功能超出限制，注册失败.\n", NULL);

        return NULL;
    }

    AlertRegInfo_g.iRegAlertCnt++;
    AlertRegInfo_g.hAlertArr[AlertRegInfo_g.iRegAlertCnt-1].bAlertIsOpen=FALSE;
    return (void *)(AlertRegInfo_g.hAlertArr+AlertRegInfo_g.iRegAlertCnt-1);
}

/***********************************************************************
* ER_SetAlertSignal - 设置某应用对应的呼唤
*
* RETURNS: 无
*
*/
void ER_SetAlertSignal(
    void *pvAlertHdl		/* 该呼唤对应的句柄，由ER_RegAlertSignal函数得到 */
)
{
    ((ALERT_HDL *)pvAlertHdl)->bAlertIsOpen=TRUE;
    /* 设置呼唤 */

    if(!bAlertIsSet_g)
    {
        bAlertIsSet_g=TRUE;
    }
}

/***********************************************************************
* ER_ClearAlertSignal - 收回某应用对应的呼唤
*
* RETURNS: 无
*
*/
void ER_ClearAlertSignal(
    void *pvAlertHdl		/* 该呼唤对应的句柄，由ER_RegAlertSignal函数得到 */
)
{
    int i;
    BOOL bAlertIsOpen=FALSE;

    assert(pvAlertHdl);
    ((ALERT_HDL *)pvAlertHdl)->bAlertIsOpen=FALSE;

    for(i=0; i<AlertRegInfo_g.iRegAlertCnt; i++)
    {
        if(AlertRegInfo_g.hAlertArr[i].bAlertIsOpen)
        {
            bAlertIsOpen=TRUE;
            break;
        }
    }
    if(!bAlertIsOpen)
    {
        /* 只有当所有呼唤都消除后，才清除呼唤 */
        /* 清除呼唤 */
        if(bAlertIsSet_g)
        {
            bAlertIsSet_g=FALSE;
        }
    }

    return;
}

/***********************************************************************
* ER_IsSetAlertFlag - 获得是否设置呼唤标志
*
* RETURNS: TRUE: 已经设置呼唤
*                 FALSE: 未设置呼唤
*
*/
BOOL ER_IsSetAlertFlag()
{
    return (bAlertIsSet_g || bHmiAlertSet_g || g_bAlertLightOn);
}

BOOL ER_IsPlatFormSetAlertFlag()
{
    return bPlatFormAlertSet_g;
}

/***********************************************************************
* ER_Clear_Alert - 清除掉呼唤，供复归时调用
*
* RETURNS: 无
*
*/
void ER_Clear_Alert(void)
{
    int i;

    /* 只有当所有呼唤都消除后，才清除呼唤 */
    for(i=0; i<AlertRegInfo_g.iRegAlertCnt; i++)
    {
        AlertRegInfo_g.hAlertArr[i].bAlertIsOpen=FALSE;
    }

    /* 清除呼唤 */
    if(bAlertIsSet_g)
    {
        bAlertIsSet_g=FALSE;
        logMsg("CPU Clear  Alert Flag.\n", 0, 0, 0, 0, 0, 0);
        if(ENG_MODE==1)
            LOG_Write(LOG_KERNEL, "CPU Clear  Alert Flag.\n", NULL);
        else if(ENG_MODE==0)
            LOG_Write(LOG_KERNEL,"CPU 清除呼唤标志.\n", NULL);

    }

    return;
}

/* 查看平台状态,包括总状态/告警状态/呼唤状态 */
void ER_showAlertSts(void)
{
    LOG_Dbg_Msg("平台总状态: 0x%x\n", uiEdpStatus_g, 0, 0, 0, 0, 0);
    LOG_Dbg_Msg("平台错误状态: 0x%x\n", ulSysErr_g, 0, 0, 0, 0, 0);

    LOG_Dbg_Msg("呼唤总状态: %s 保护呼唤: %s 平台呼唤: %s HMI呼唤: %s\n",
                (int)(ER_IsSetAlertFlag() ? "开放" : "关闭"),
                (int)(bAlertIsSet_g ? "开放" : "关闭"),
                (int)(bPlatFormAlertSet_g ? "开放" : "关闭"),
                (int)(bHmiAlertSet_g ? "开放" : "关闭"), 0, 0);
}