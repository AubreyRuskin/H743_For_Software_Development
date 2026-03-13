/* VoltageWatch.h - This file contains the driver program for voltage monitor */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01c, 27jun08, dy migrate to EDP03 and Lici platform.
01b, 27feb08, zy version 1.1 created.
01a, 19apr06, zy first version 1.0 created.
*/

/*
DESCRIPTION
This file contains the driver program for voltage monitor.
*/

/* includes */

#ifndef WT_VOLTAGEWATCH_H
#define WT_VOLTAGEWATCH_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "edpbase.h"

/* defines */

#define WT_NEW_COM_INTVL (SYS_SEC/20) 	/* 每次通信读写等待时间 */


/* typedefs */

typedef enum
{
    WT_MMI_WATCH_QD_STS_UNKOWN=0,	/* 启动未知状态，默认状态 */
    WT_MMI_WATCH_QD_OPEN_STS=1, 		/* 启动开放状态 */
    WT_MMI_WATCH_QD_CLOSE_STS=2, 				/* 启动收回状态 */
    WT_MMI_WATCH_QD_LOCK_STS=3,		/* 启动闭锁状态 */

}  WT_MMI_WATCH_QD_STS_TYPE;   /* MMI监视的启动状态类型 */


typedef struct		/* 电压监视信息结构 */
{
    BOOL bVoltWatchInitFlag;		/* 电压监测功能模块初始化标志，为TRUE，表示初始化，否则表示未初始化 */
    uint8_t ucVoltWatchCh;					/* 电压监视总路数 */
    uint16_t uiSoftVer;   	/* MEGA16软件版本号，高字节代表整数位，低字节代表小数位 */

    float f10VoltWatchVal;    		/* 正10伏电压监视值 */
    float fNeg10VoltWatchVal; 	/* 负10伏电压监视值 */
    float f5VoltWatchVal;     			/* 正5伏电压监视值 */
    float fAddRefWatchVal;    		/* AD参考电压监视值,现在版本没用 */

    float f24VoltWatchVal;    		/* 正24伏电压监视值 */
    float f3Dot3VoltWatchVal; 		/* 正3.3伏电压监视值 */
    float f1Dot5VoltWatchVal; 		/* 正1.5伏电压监视值 */
    float f12VoltWatchVal;    		/* 正12伏电压监视值 */
    float fNeg12VoltWatchVal; 		/* 负12伏电压监视值 */
    float f2Dot5VoltWatchVal; 		/* 正2.5伏电压监视值 */
    float fHdlc1VoltWatchVal; 		/* Hdlc1电压监视值 */
    float fHdlc2VoltWatchVal; 		/* Hdlc2电压监视值 */
    float fFpga2Dot5VoltWatchVal; 	/* fpga使用的正2.5伏电压监视值 */

    float fTemptWatchVal;      /* 温度监视值 */
    WT_MMI_WATCH_QD_STS_TYPE ucQDSts;				/* 启动状态,0为QD状态未知，1为QD驱动，2为QD收回，3为启动闭锁，按MMI和CPU的内部规约定义 */

}  WT_SYSINFO_TYPE;

typedef struct   /* 某电压异常信息结构 */
{
    float  fOvrflwVal;     /* 上溢出值 */
    uint32_t  ulOvrflwCnt; /* 上溢出计数 */
    float  fUdrflwVal;     /* 下溢出值 */
    uint32_t  ulUdrflwCnt; /* 下溢出计数 */
}  WT_VLT_EXC_INF_TYPE;


typedef struct  /* 电压监视异常信息结构 */
{
    BOOL bVoltWatchInitFlag;	/* 电压监测功能模块初始化标志，为TRUE，表示初始化，否则表示未初始化  */

    WT_VLT_EXC_INF_TYPE inf10VoltExc;       /* 正10伏溢出异常信息 */
    WT_VLT_EXC_INF_TYPE infNeg10VoltExc;    /* 负10伏溢出异常信息 */
    WT_VLT_EXC_INF_TYPE inf5VoltExc;        /* 正5伏溢出异常信息 */
    WT_VLT_EXC_INF_TYPE inf24VoltExc;       /* 正24伏溢出异常信息 */
    WT_VLT_EXC_INF_TYPE inf3Dot3VoltExc;    /* 正3.3伏溢出异常信息 */
    WT_VLT_EXC_INF_TYPE inf1Dot5VoltExc;    /* 正1.5伏溢出异常信息 */
    WT_VLT_EXC_INF_TYPE inf12VoltExc;       /* 正12伏溢出异常信息 */
    WT_VLT_EXC_INF_TYPE infNeg12VoltExc;    /* 负12伏溢出异常信息 */
    WT_VLT_EXC_INF_TYPE inf2Dot5VoltExc;    /* 正2.5伏溢出异常信息 */
    WT_VLT_EXC_INF_TYPE infHdlc1VoltExc;    /* Hdlc1电压溢出异常信息 */
    WT_VLT_EXC_INF_TYPE infHdlc2VoltExc;    /* Hdlc2电压溢出异常信息 */
    WT_VLT_EXC_INF_TYPE infFpga2Dot5VoltExc;    /* fpga使用的正2.5伏溢出异常信息 */

    uint32_t  ulCpuRebootCnt;        /* 主CPU连续复位异常计数 */
    uint32_t  ulDownHaltCnt;         /* 下行帧中断异常计数 */
    uint32_t  ulDownChkErrCnt;       /* 下行帧连续校验和错误异常计数 */
    uint32_t  ulDownFmtErrCnt;       /* 下行帧格式错误异常计数 */
    uint32_t  ulUnSupportBoardErrCnt;  /* 硬件版本未支持错误异常计数 */
    uint32_t  ulUnKownErrCnt;          /* 未明异常计数 */

}  WT_EXC_INFO_TYPE;

typedef enum  /* 启动测试返回结果 */
{
    WT_QD_TST_NORMAL=0, 	/* 启动测试结果正常，默认状态 */
    WT_QD_TST_ABNORMAL=1,			/* 启动测试结果异常 */
    WT_QD_TST_UNKOWN=2,			/* 启动测试结果未知(比如测试前，处于启动开放态，则不能进行测试) */
}  WT_QD_TST_RSLT_TYPE;

/* functions */

/* get the state report of voltage monitoring, called for MMI.
 * Para:
 *     pRtVoltWatchStsRpt, pointer to information struct.
 * Return:
 *     EP_SUCCESS, EP_ERROR.
 */
EP_STATUS WT_VoltWatchStsRpt(WT_SYSINFO_TYPE *pRtVoltWatchStsRpt);

/* get the state report of voltage monitoring exception, called for MMI.
 * Para:
 *     pRtVoltWatchStsRpt, pointer to information struct.
 * Return:
 *     EP_SUCCESS, EP_ERROR.
 */
EP_STATUS WT_VoltExcStsRpt(WT_EXC_INFO_TYPE *pRtVoltExcStsRpt);

/* initialize the voltage monitoring module.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, EP_ERROR.
 */
EP_STATUS WT_VoltWatchInit();

/* enable the QD signal on monitor controller.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void WT_MegaOpenQD(void);

/* disable the QD signal on monitor controller.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void WT_MegaCloseQD(void);


/* tesing of QD signal on monitor controller, called for MMI.
 * Para:
 *     NONE.
 * Return:
 *     WT_QD_TST_NORMAL, WT_QD_TST_ABNORMAL, or WT_QD_TST_UNKOWN.
 *
 * alert: ms level delay ,can not be called in realtime task.
 */
WT_QD_TST_RSLT_TYPE WT_MegaQDTst(void);

/* clear the exception information on monitor controller.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void WT_MegaClrErrRec(void);

/* initialize the data for monitor controller communication with the Timer initialization.
 * Para:
 *     iTimerINTIntvl, interval of Timer, unit is us.
 * Return:
 *     NONE.
 */
void WT_MegaComInit(int iTimerINTIntvl);

/* deal with the down frame, called in Timer ISR.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void WT_MegaDownDealInINT(void);

/* get the QD lock signal on mother board.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL WT_QD_Is_Lock();

/***********************************************************************
* GetMega16Info - 获取mega16上送信息
*
* RETURNS: length of receiving data.
*
*/
int GetMega16Info(
    uint8_t *pRev
);

/* get the actual current QD status.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL SIO_Act_QD_Sts (void);

/* get the actual current LOCK status.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL SIO_Act_Lock_Sts (void);

/* get the QD type description information.
 * Para:
 *     ucQdType, QD type.
 * Return:
 *     pointer to description information.
 */
extern uint8_t *IO_GetQdDesInfo(uint8_t ucQdType);

#ifdef	__cplusplus
}
#endif

#endif      /* VOLTAGEWATCH_H */
