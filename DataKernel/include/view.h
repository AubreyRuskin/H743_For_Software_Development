
/* view.h - This file contains interface to EPView communication module */

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
*/

#ifndef VIEW_H
#define VIEW_H

#include "semLib.h"
#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

//#include <intLib.h>
//#include <semLib.h>

#include "semaphore_compat.h"
#include "sys_ipc_compat.h"
#include "sys_sem_compat.h"

#include "logic.h"
#include "auto_upload.h"
#include "swcfg.h"
#include "measure.h"
#include "datetime.h"

/* defines */

#define SZ_RUN_INFO_BUF 3072
/* #define SZ_VALID_INFO_BUF 512 */  /* 有效数据访问大小,必须比SZ_RUN_INFO_BUF小很多 */
#define SZ_VALID_INFO_BUF (SZ_RUN_INFO_BUF/2)			/* DY 增大7/23/2007 */

#define SZ_ERR_STR_LEN 256  /* 缓冲要大一点 */

#define RPTSNRESERVED 100		/* Reserved 100 serial number. */
#define MIN_SLOW_DELETE_NUM 32   /* 最少慢速删除文件份数 */
#define MAX_SLOW_DEL_EVT_FILE 16      /* 慢速删除属性缓冲区长度 */
#define MAX_SLOW_DEL_EVT_FILE_MASK 0xF  /* mask for buffer length. */
#define FAST_DEL_BIT 0x08  /* delete attribute bit. */
#define MAX_EVT_FILE_NUM 500  /* 最多事件条数 */

/* typedefs */

typedef struct			/* 遥测AI */
{
    uint8_t aucId[MAX_ID_LEN+1];
    uint8_t aucName[MAX_ID_LEN+1];
    uint8_t aucABRV[4];         	/* 内部简称 */
    uint8_t ucUnit;
    uint8_t ucArith;
    uint8_t ucParaSetMode;      		/* 配置参数设置方式 */
    BOOL   bNotNeedUpSend;
    BOOL bSrcType;		/* 0: 外部通道来源；1: 中间计算结果来源 */
    uint8_t ucHmSeq;				/* 谐波次数 */
    float fRtMax;
    float fRtMin;
    float fOvMax;
    float fOvMin;
    float fChgCoff;
    uint8_t aucRtMaxSettingId[MAX_ID_LEN+1];  	/* 定值设置方式，各个定值逻辑标识字符串基*/
    uint8_t aucRtMinSettingId[MAX_ID_LEN+1];
    uint8_t aucOvMaxSettingId[MAX_ID_LEN+1];
    uint8_t aucOvMinSettingId[MAX_ID_LEN+1];
    BOOL bIsPrvtUse;
    SC_SUB_LGC_ITEM *psublgc;
} VI_MEA_AI_CFG;

typedef struct			/* 遥测DI */
{
    uint8_t aucId[MAX_ID_LEN+1];
    uint8_t aucName[MAX_ID_LEN+1];
    BOOL   bNotNeedUpSend;
    BOOL   bJianXiuUpSendFlag;		/* 为TRUE上送 */
    BOOL   bDbSts;				/* 是否是双点遥信 0: 单点 1: 双点 */
    BOOL   bSOE;					/* 0: 形成 1: 不形成 */
    BOOL bSrcType;		/* 0: 外部通道来源；1: 中间计算结果来源 */
    BOOL bIsPrvtUse;
    SC_SUB_LGC_ITEM *psublgc;
    BOOL bDelay; /* 是否延迟标志 */
} VI_MEA_DI_CFG;

typedef struct
{
    uint8_t ucAttrib;
    uint8_t aucName[MAX_ID_LEN+1];
    EP_ELEM_IO *pelmSrc;
} VI_EVT_PARM_CFG;

typedef struct
{
    uint8_t aucId[MAX_ID_LEN+1];
    uint8_t aucName[MAX_ID_LEN+1];
    uint8_t aucABRV[4];         /* 内部简称 */
    uint16_t uiPtNum;          /* 序号，即遥控点号 */
    uint8_t ucType;
    uint8_t ucParmNum;
    VI_EVT_PARM_CFG aparmcfg[MAX_EVT_PARM_NUM];
} VI_MEA_DO_CFG;

typedef struct
{
    uint8_t aucId[MAX_ID_LEN+1];
    uint8_t aucName[MAX_ID_LEN+1];
    uint8_t aucABRV[4];
    BOOL bKeep;
    BOOL bHaveSts;
    BOOL bNeedFastDel;			/* 需要快速删除标志,0为慢速删除,1为快速删除 */
    BOOL bNotNeedUpSend;				/* 不需要综自上送标志,0为需要上送,1为不需上送 */
    BOOL   bMmiNotDisRtn;    /*MMI上是否要显示返回事件标志,0显示,1不显示*/
    BOOL  bGWReport;
    BOOL bGWXinXiReport;
    uint8_t ucMMIDlgPopAttr;					/* MMI事件窗口弹出消除属性 */
    void *pAlertHdl;       	/* 该事件对应的呼唤句柄，只有保护异常和呼唤类型，才有该句柄，保护事件类型，该句柄为空 */
    uint16_t unCode;
    BOOL bStsNow;
    BOOL bStsIn;
    uint8_t ucType;
    uint8_t ucParmNum;
    VI_EVT_PARM_CFG aparmcfg[MAX_EVT_PARM_NUM];
} VI_EVT_CFG;

typedef union
{
    COMPLEX xVal;
    float fVal;
    uint32_t ulVal;
    int32_t lVal;
    BOOL bVal;
} VI_EVT_PARM;

typedef struct
{
    const VI_EVT_CFG *pcfg;
    uint16_t unRptSN;
    uint8_t ucRecSN;
    uint8_t ucCOT;
    BOOL bState;
    uint32_t ulTime;
    EP_DATE_TIME dttm;		/* 绝对时间 */
    BOOL bAbsTimeCreateFlag;		/* 绝对时间形成与否标志 */
    VI_EVT_PARM aparm[MAX_EVT_PARM_NUM];
} VI_EVT_MSG;

typedef struct
{
    const VI_MEA_DI_CFG *pcfg;
    uint16_t unRptSN;
    uint8_t ucRecSN;
    uint8_t ucCOT;
    uint8_t ucFtype;
    uint32_t ulTime;
    uint64_t ullusCntFrom1970;
    EP_DATE_TIME dttm;		/* 绝对时间 */
    BOOL bAbsTimeCreateFlag;		/* 绝对时间形成与否标志 */
    uint16_t unCh;
    uint8_t ucDIQ;
} VI_SOE_MSG;

typedef struct
{
    const RD_LGC_LED_CH *pcfg;
    BOOL bSts;
    uint16_t unIdx;
} VI_LED_MSG;

typedef struct
{
    const RD_LGC_DI_CH *pcfg;
    BOOL bSts;
    uint32_t ulTime;
    EP_DATE_TIME dttm;		/* 绝对时间 */
    BOOL bAbsTimeCreateFlag;		/* 绝对时间形成与否标志 */
    uint16_t unIdx;
    uint16_t unCh;
    uint8_t ucDIQ;
    uint16_t unRptSN;
    uint8_t ucRecSN;
    uint8_t ucCOT;
    uint8_t ucFtype;
} VI_DI_MSG;

typedef struct
{
    const SC_LINK_ITEM *pcfg;
    uint8_t ucCOT;
    BOOL bSts;
    uint32_t ulTime;
    EP_DATE_TIME dttm;		/* 绝对时间 */
    BOOL bAbsTimeCreateFlag;		/* 绝对时间形成与否标志 */
    uint16_t unCh;
} VI_LINK_MSG;

typedef struct
{
    uint16_t unErrCode;
    uint16_t unOpFlag;
    uint32_t ulTime;
    EP_DATE_TIME dttm;		/* 绝对时间 */
    BOOL bAbsTimeCreateFlag;		/* 绝对时间形成与否标志 */
    uint8_t aucNote[SZ_ERR_STR_LEN+1];
    uint16_t unRptSN;				/* 2006-11-2日，张云添加 */
    uint8_t ucRecSN;
} VI_ERR_MSG;

typedef struct		/* 带参数 */
{
    uint16_t unErrCode;
    uint16_t unOpFlag;
    uint32_t ulTime;
    EP_DATE_TIME dttm;		/* 绝对时间 */
    BOOL bAbsTimeCreateFlag;		/* 绝对时间形成与否标志 */
    uint8_t aucNote[SZ_ERR_STR_LEN+1];
    uint16_t unRptSN;				/* 2006-11-2日，张云添加 */
    uint8_t ucRecSN;
    uint32_t ulSts;		/* state, 0 or 1 now. */
} VI_ERR_STAT_MSG;

typedef struct
{
    uint16_t unRptSN;
    uint8_t ucRecSN;
    BOOL bSts;
    uint32_t ulTime;
    EP_DATE_TIME dttm;		/* 绝对时间 */
    BOOL bAbsTimeCreateFlag;		/* 绝对时间形成与否标志 */
} VI_FAULT_MSG;

typedef struct
{
    ME_MEA_AI_DATA_DB **ppcfg;  	/* 根据测量量个数申请了空间,实际只有uiNum个有效值, */
    uint8_t ucCOT;  								/*上送原因*/
    uint8_t ucMode; 		/* 发送方式 */
    uint16_t uiNum;  					/* 发送个数 */
    uint32_t ulTime;
} VI_MEA_MSG;

typedef struct
{
    enum
    {
        NEW_EVT,
        NEW_SOE,
        DI_CHG,
        LED_CHG,
        LINK_CHG,
        ERR_OCR,
        ERR_OCR_STAT,
        FAULT_STS,
        MEA_OVER
    } type;

    BOOL   bViewModIsInit;		/* 2006-8-2日张云修改， */

    union
    {
        VI_EVT_MSG evt;
        VI_SOE_MSG soe;
        VI_LED_MSG led;
        VI_DI_MSG di;
        VI_LINK_MSG link;
        VI_ERR_MSG err;
        VI_ERR_STAT_MSG errstat;
        VI_FAULT_MSG fault;
        VI_MEA_MSG mea;
    } msg;
} VI_RUN_INFO;

/** 事件信息结构定义 **/
typedef struct
{
    uint8_t *strID;              	/* 事件的逻辑标识字符串 */
    uint8_t ucParaCount;           /* 事件的参数个数 */
    EP_ELEM_IO **ppParaSourceSignal ;  		/* 事件的所有参数的信号来源指针数组
                                            									 各数组成员为相应序号参数的信号来源的
                                            									 访问指针 */
}  SCI_EVENT_INFO_TYPE;

typedef struct
{
    int iMaxRptNum;
    int iMaxRecNum;
    uint16_t unRptSN;
    uint16_t unRptSNReserved;		/* Reserved serial number used for the events created before the event modual intialization. */
    uint8_t ucRecSN;		/* recorded wave segment number. */
    int iFault;
    int iOpenQDCnt;			/* 开放启动继电器计数 */
    BOOL bRecOn;
    BOOL bRecWrFileOn;
    BOOL bRecInRpt;
    uint32_t ulEvtBgnCnt;               /* AI count of fault begin event. */
    uint32_t ulFaultBeginTimeUs;						/* 故障开始的us数 */
    BOOL bRptSNProcessFinish;				/* SN modification finished. */
    u_int ulRptNumBeforeEventInit;			/* The number of event before event intialization. */
    BOOL bEvtWrFileOn;		/* 事件文件形成状态 */
    uint16_t uStNewestSN;    /* 当前文件夹中最新的报告号 */
    BOOL bAllocBlk;  /* 是否允许分配缓冲块 */
} VI_RPT_STS;

typedef struct
{
    BOOL bVal;   		/* 来了校准信号 */
    BOOL bAdjustRunFlag;				/* 校准正在进行标志 */
    uint32_t uAdjustStartTickNum;
    int32_t ucObjType;   /* 校准对象类型 0 :ai物理通道，1:测量量 */
    int32_t ucOrdType;    /* 校准命令类型0:增益校准，1:偏置校准 */
    int32_t ucNum;   					/* 校准对象序号 */
} VI_ADJUST_INFO;

/* globals */

extern VI_ADJUST_INFO adjoverinfo_g;
extern VI_ADJUST_INFO adjinfo_g;

extern int iEvtNum_g;

extern int iMeaAiNum_g;
extern int iMeaDiNum_g;
extern int iMeaDoNum_g;

extern VI_RPT_STS rptsts_g;

extern VI_RUN_INFO arinf_g[SZ_RUN_INFO_BUF];
extern u_int uiCurInfIdx_g;
extern u_int uiCurInfPos_g;
extern SEM_ID semNewInfCom_g;
extern SEM_ID semNewInfRpt_g;

#ifdef GXC01U
extern SEM_ID semNewInfGxc_g;
#endif

extern  BOOL  bViewModIsInit_g;			/* 2006-8-2日，张云，事件模块初始化标志 */
extern  uint32_t  ulInfNonComleteCnt_g;
/* global functions */

/***********************************************************************
* VI_Init_Sem - 初始化信号量
*
* RETURNS: 无
*
*/
void VI_Init_Sem(void);

/***********************************************************************
* VI_Initialize - 事件报告初始化
*
* RETURNS: TRUE, or FALSE
*
*/
EP_STATUS VI_Initialize(void);

/***********************************************************************
* VI_Cfg_Event - 事件配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_Cfg_Event(
    uint8_t *pucCfg,
    uint32_t ulLen
);

/***********************************************************************
* VI_Cfg_Alarm - 告警配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_Cfg_Alarm(
    uint8_t *pucCfg,
    uint32_t ulLen
);

/***********************************************************************
* VI_Cfg_Flag - 标志配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_Cfg_Flag(
    uint8_t *pucCfg,
    uint32_t ulLen
);

/***********************************************************************
* VI_Cfg_Rec_AI - AI录波配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_Cfg_Rec_AI(
    uint8_t *pucCfg,
    uint32_t ulLen
);

/***********************************************************************
* VI_Cfg_Rec_DI - DI录波配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_Cfg_Rec_DI(
    uint8_t *pucCfg,
    uint32_t ulLen
);

/***********************************************************************
* VI_Cfg_Mea_AI - 遥测配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_Cfg_Mea_AI(
    uint8_t *pucCfg,
    uint32_t ulLen
);

/***********************************************************************
* VI_Cfg_Mea_DI - 遥测配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_Cfg_Mea_DI(
    uint8_t *pucCfg,
    uint32_t ulLen
);

/***********************************************************************
* VI_Cfg_Mea_DO - 遥控配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_Cfg_Mea_DO(
    uint8_t *pucCfg,
    uint32_t ulLen
);

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
);


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
);

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
);

/***********************************************************************
* SCI_Init_Add_TimeSet_New_Yaoxin_Signal - 逻辑图中添加1个新的中间结果遥信量，时间由逻辑图给出
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
EP_STATUS  SCI_Init_Add_TimeSet_New_Yaoxin_Signal(
    uint8_t *strID,		/* 该遥信量的逻辑标识 */
    int32_t *piCh,				/* 遥信号 */
    uint8_t *pucType			/* Single or double */
);

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
);

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
);

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
);

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
);

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
);

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
);

/***********************************************************************
* VI_Get_Mea_DO_Attr - Get measurement DO attribution.
*
* RETURNS:
* 			Pointer to the MEA_DO attribution structure.
*			NULL if iIdx is invalid(>=iMeaDoNum_g).
*
*/
const VI_MEA_DO_CFG *VI_Get_Mea_DO_Attr(
    int iIdx			/* index of the MEA_DO(from 0). */
);

/***********************************************************************
* VI_Get_Mea_Do_Num - 获取遥控点序号
*
* RETURNS: 无
*
*/
BOOL VI_Get_Mea_Do_Num(
    uint8_t *strID, 		/* 逻辑名称 */
    uint16_t *puiRtNum, 			/* 序号 */
    uint8_t *pucRtParaNum	/* 参数 */
);

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
);

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
);

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
    uint16_t usQuality  /* quality */
);

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
    uint16_t usQuality  /* 品质 */
);

/***********************************************************************
* VI_Rd_Mea_AI_Val - Read all measurement AIs' value.
*
* RETURNS: 无
*
* Alert:
*		pfRslt must contains space to save iMeaAiNum_g float numbers.
*/
void VI_Rd_Mea_AI_Val(
    float *pfRslt		/* to save all MEA_AIs' current value. */
);

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
    uint16_t *pQuality   /* 品质 */
);

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
extern VI_RUN_INFO *VI_Rd_Run_Info(
    u_int uiIdx			/* index of the information. */
);

/***********************************************************************
* VI_End_Wr_Run_Info - 结束运行信息写入
*
* RETURNS: 无
*
*/
extern void VI_End_Wr_Run_Info(void);

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
/***********************************************************************
* VI_Clear_Evt - Clear self-keep event signals.
*
* RETURNS: 无
*
*/
void VI_Clear_Evt(
    int nType		/* 暂时使用 */
);
#endif

#if defined(EDP_01_02_BUILD)
/***********************************************************************
* VI_Clear_Evt - Clear self-keep event signals.
*
* RETURNS: 无
*
*/
void VI_Clear_Evt(void);
#endif

/***********************************************************************
* VI_Bgn_Fault - Set fault begin flag.
*
* RETURNS: 无
*
*/
void VI_Bgn_Fault(
    uint32_t ulAiCnt,uint16_t uForwordTime		/* AI count of this time logic scanning. */
);

/***********************************************************************
* VI_End_Fault - Set fault end flag.
*
* RETURNS: 无
*
*/
void VI_End_Fault(
    uint32_t ulAiCnt		/* AI count of this time logic scanning. */
);

/***********************************************************************
* VI_Open_QD - 开放启动继电器.
*
* RETURNS: 无
*
*/
void VI_Open_QD();

/***********************************************************************
* VI_Close_QD - 关闭启动继电器.
*
* RETURNS: 无
*
*/
void VI_Close_QD();

/***********************************************************************
* VI_Chg_Mea_Some_Attrs - 改变测量属性
*
* RETURNS: 无
*
*/
void  VI_Chg_Mea_Some_Attrs(void);

/***********************************************************************
* VI_CK_Mea_Attrs - 获取遥测量属性
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS VI_CK_Mea_Attrs(void);

/***********************************************************************
* VI_New_Adjust - 校准命令(0x0830)调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_New_Adjust(
    uint8_t ucObjType, 		/* 校准对象类型0,ai物理通道 1 测量量 */
    uint8_t ucOrdType				/* 校准命令类型0,增益校准,1 偏置校准 */
);

/***********************************************************************
* VI_Come_New_Plus_Adjust - 新的增益校准命令
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_Come_New_Plus_Adjust(
    int  nScanTaskNo,
    int32_t *piRtObjType,
    int32_t *piRtNum
);

/***********************************************************************
* VI_Come_New_Off_Adjust - 新的偏置校准命令
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_Come_New_Off_Adjust(
    int nScanTaskNo,
    int32_t *piRtObjType,
    int32_t *piRtNum
);

/***********************************************************************
* VI_Get_Mea_AI_Idx - 获取遥测量的通道序号
*
* RETURNS: 序号
*
*/
int VI_Get_Mea_AI_Idx(
    uint8_t *pStrID
);

/***********************************************************************
* VI_Set_Mea_AI_ChgCoff - 根据遥测量序号设置遥测量的越限系数
*
* RETURNS: 无
*
*/
void VI_Set_Mea_AI_ChgCoff(
    int iIdx,		/* 序号 */
    float fCoff			/* 系数 */
);

/***********************************************************************
* VI_New_PoClear - PO清零
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_New_PoClear(
    uint8_t ucObjNum
);

/***********************************************************************
* VI_Come_New_PoClear - PO清零
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_Come_New_PoClear(
    int32_t *piRtNum
);

/* get command for ZhuBian switching, called by logic graph.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VI_Come_New_TdZhuBianSwitch(void);

/* get command for JinXian switching, called by logic graph.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VI_Come_New_TdJinXianSwitch(void);

/***********************************************************************
* VI_Come_Over_Plus_Adjust - 增益校准
*
* RETURNS: 无
*
*/
void VI_Come_Over_Plus_Adjust(
    int32_t *piRtObjType, 		/* 类型 */
    int32_t *piRtNum		/* 通道号 */
);

/***********************************************************************
* VI_Come_Over_Off_Adjust - 偏置校准
*
* RETURNS: 无
*
*/
void VI_Come_Over_Off_Adjust(
    int32_t *piRtObjType, 		/* 类型 */
    int32_t *piRtNum			/* 通道号 */
);

/***********************************************************************
* DelEvtFile - 配置修改后删除事件文件和录波文件
*
* RETURNS: 无
*
*/
void DelEvtAndRecFile(void);

/***********************************************************************
* DelAucCpuFile - 通讯成功后删除辅助CPU程序
*
* RETURNS: 无
*
*/
STATUS DelPanelAucCpuFile(
    uint8_t *pAttrFlag
);

/***********************************************************************
* DelMainAucCpuFile - 通讯成功后删除主板辅助CPU程序
*
* RETURNS: 无
*
*/
STATUS DelMainAucCpuFile(
    uint8_t *pAttrFlag
);

/***********************************************************************
* GetRptSNProcessState - 事件模块初始化之前的事件是否处理完成
*
* RETURNS:
*				  TRUE: 已处理完
*              FALSE: 没有处理完
*
*/
BOOL GetRptSNProcessState(void);

/***********************************************************************
* VI_Is_Fault - 返回是否处于故障态
*
* RETURNS:
*               TRUE: 是故障态
*               FALSE: 非故障态
*
*/
BOOL VI_Is_Fault(void);

/***********************************************************************
* SCI_Deal_Event_Alert - 根据事件的区分码，处理事件的呼唤属性，要求每次逻辑图都扫描处理
*
* RETURNS: 无
*
*/
void SCI_Deal_Event_Alert(
    int16_t nNum, 	/* 事件区分码 */
    BOOL bActFlag				/* 事件动作标志，TRUE为表示是事件动作，FALSE表示是事件返回 */
);

/***********************************************************************
* GetMeaCoff - 获取遥测量相关系数，MMI调用
*
* RETURNS: NONE
*
*/
void GetMeaCoff(
    VI_AI_COFF *pMeaCoff		/* 分配iMeaAiNum_g个 */
);

/* read the newest SN.
 * Para:
 *     Newest SN.
 * Return:
 *     TRUE, or FALSE.
 */
EP_STATUS EP_GetNewestSN(uint16_t *pNewestSN);

/* set the newest SN.
 * Para:
 *     newest SN.
 * Return:
 *     Result.
 */
EP_STATUS EP_SetNewestSN(uint16_t uNewestSN);

/***********************************************************************
* VI_New_FarSts - 远方就地状态，供MMI调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_New_FarSts(
    uint32_t ulFarSts
);

/* new command for switching, called by MMI.
 * Para:
 *     ucCmdType, command type.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VI_New_TdSwitch(uint8_t ucCmdType);

/***********************************************************************
* VI_New_RepairSts - 检修状态，供MMI调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_New_RepairSts(
    uint32_t ulRepairSts
);

/* Initialize the serial number.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUSSESS, success.
 *     EP_ERROR, fail.
 */
extern EP_STATUS VI_InitSn(void);

/* read event file deleting attribution.
 * Para:
 *     usCurSN, current SN.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL VI_RdEventDelAttr(uint16_t usCurSN);

/***********************************************************************
* VI_Run_Info_Wr_P - 开始运行信息写入
*
* RETURNS: 无
*
*/
extern VI_RUN_INFO *VI_Run_Info_Wr_P(void);

#ifdef	__cplusplus
}
#endif

#endif                                  /* VIEW_H */
