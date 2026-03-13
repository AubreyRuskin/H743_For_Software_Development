/* edp_asst.h - subroutine library for handling the assistant operation */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 20may09, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling the assistant operation.
*/

#ifndef EDP_ASST_H
#define EDP_ASST_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include <vxWorks.h>  /* 基本的包含头文件 */
#include "assert_compat.h"
#include "stdlib_compat.h"
#include "msgQLib.h"
#include <taskLib.h>

/* defines */

#define INFO_MAX_LEN 16 /* 信息长度 */
#define MAX_CC_PORT_NUM 12 /* CC板最大端口数 */
#define MAX_CC_BOARD_NUM 32  /* 最大CC板个数 */
#define MAX_CC_BOARD_ID_NUM 32 /* 最大CC板板号个数 */
#define MASK_NET_STORM 0x01  /* 网络风暴位 */
#define MAX_NET_STORM_CNT 32 /* 最多网络风暴计数次数 */

/* typedefs */

typedef struct		/* 慢速任务处理消息结构 */
{
    enum
    {
        ACMDCHG,
        FREQCHG,
        ACCOFCHG,
        LANGUAGECHG,
        SETAUTOWR,			/* 定值自动写入 */
        SETAREACHG,	/* 定值切换命令 */
        MUDELAYWR,			/*合并单元延时写入*/
        MUNAMEWR,			/*合并单元名称写入*/
        CT_PT_RATE_VAL_WR, /* 一次额定值写入 */
        SETTING_RANGE_SET   /* 设置定值的最大值、最小值、默认值和步长 */
    } type;	/* message type */

    int param1;		/* arg to message */
    int param2;
    int param3;
    int param4;
    int param5;
    int MuTypeNo; /* 消息中合并单元写文件的条目序号 */
    int MuDelay;     /* 消息中合并单元写文件的延时数值 */
    char smvDes[SLOW_MSG_MAX_CHAR_NUM];  /*消息中合并单元名称描述*/

} SLOW_MESSAGE_NODE;

/* 单光口状态 */
typedef struct
{
    /* 光口光收发器收发状态 */
    union STS_INFO_UN
    {
        struct STS_INFO_ST
        {
            uint16_t Reserved1:7;
            uint16_t ComFail:1;
            uint16_t Reserved2:4;
            uint16_t BaudRate:2;
            uint16_t TxFault:1;
            uint16_t RxLOS:1;
        } StsInfo_st;

        uint16_t usStsInfo;
    } StsInfo;

    uint8_t ucSupplierInfo[INFO_MAX_LEN+1]; /* 供货商信息 */

    /* 光收发器温度 */
    uint16_t usTemp;

    /* 光收发器供电电平 */
    uint16_t usVolt;

    /* 光收发器发送电流 */
    uint16_t usCurrent;

    /* 光收发器发送光功率 */
    uint16_t usSndWatt;

    /* 光收发器接收光功率 */
    uint16_t usRcvWatt;

    /* 光收发器预警状态 */
    union WARM_INFO_UN
    {
        struct WARM_INFO_ST
        {
            uint16_t TempH:1;
            uint16_t TempL:1;
            uint16_t VccH:1;
            uint16_t VccL:1;
            uint16_t TxBiasH:1;
            uint16_t TxBiasL:1;
            uint16_t TxPowH:1;
            uint16_t TxPowL:1;
            uint16_t RxPowH:1;
            uint16_t RxPowL:1;
            uint16_t Resetved:6;
        } WarmInfo_st;

        uint16_t usWarmInfo;
    } WarmInfo;

    /* 收发器报警状态 */
    union ALARM_INFO_UN
    {
        struct ALARM_INFO_ST
        {
            uint16_t TempH:1;
            uint16_t TempL:1;
            uint16_t VccH:1;
            uint16_t VccL:1;
            uint16_t TxBiasH:1;
            uint16_t TxBiasL:1;
            uint16_t TxPowH:1;
            uint16_t TxPowL:1;
            uint16_t RxPowH:1;
            uint16_t RxPowL:1;
            uint16_t Resetved:6;
        } AlarmInfo_st;

        uint16_t usAlarmInfo;
    } AlarmInfo;

    uint32_t ulSvFlowCnt;           /*SV报文流量统计单位帧/秒*/
    uint32_t ulSvIntervalErrCnt;    /*SV报文间隔异常计数*/
} T_OPT_PORT_STS, *pT_OPT_PORT_STS;

/* CC板状态 */
typedef struct
{
    uint8_t ucCCSn;  /* CC板序号 */
    uint8_t ucProtocolVer;  /* 协议版本号 */
    uint16_t usFPGASwVer;  /* FPGA程序版本号 */
    uint16_t usNiosSwVer;  /* Nios程序版本号 */
    uint16_t usSvCfgVer;  /* SV配置版本号 */
    uint16_t usSvCfgCrc;  	  /* SV配置CRC */
    uint16_t usSvBayNum;  /* SV支持最多间隔数 */
    uint16_t usGsCfgVer;          /* GS配置版本号 */
    uint16_t usGsCfgCrc;  /* GS配置CRC */
    uint16_t usGsBayNum;      /* GS支持最多间隔数 */
    uint8_t ucPortNum;  /* CC板支持的端口数 */
    uint32_t ulOptRptCnt;  /* 光功率总报文数 */
    uint8_t ucOtherInfo;  /* 其它信息, 如是否网络风暴等 */
    BOOL bNetStormSts;  /* 是否网络风暴状态 */
    BOOL bLstNetStormSts;  /* 上次网络风暴状态 */
    uint32_t ulNetStormCnt;  /* 网络风暴计数次数 */
    uint8_t ucCCDesc[INFO_MAX_LEN+1];  /* CC描述 */
    T_OPT_PORT_STS tOptPortSts[MAX_CC_PORT_NUM];
} T_CC_STS, *pT_CC_STS;

/* globals */

extern BOOL bConnectMmiSuccessFlag;  /* if connnect. */
extern uint8_t ucCPUSeq_g;	 /* CPU位置序号，0: 第一块CPU；1: 第2块CPU*/
extern T_CC_STS arrCcPortSts[MAX_CC_BOARD_NUM];  /* CC端口状态 */
extern uint8_t g_ucCCSnToArr[MAX_CC_BOARD_ID_NUM];    /* 板号到数组关系 */
extern uint8_t g_ucCCNum;  /* 支持CC个数 */

/* global functions */

/***********************************************************************
* MonitorInit - 慢速处理任务初始化
*
* RETURNS: 无
*
*/
extern void SlowTaskProcessInit();

/***********************************************************************
* GetSysTaskStatus - 获取系统任务状态
*
* RETURNS: 无
*
*/
extern void GetSysTaskStatus(void);

/***********************************************************************
* AddTaskToList - 增加任务到监视队列
*
* RETURNS: 无
*
*/
extern void AddTaskToList(
    TASK_ID iTaskID, 		/* 添加的任务ID号 */
    BOOL bTaskBeCreated,			/* 任务时候生成标志 */
    char *pMessage,			/* 出错时显示的相关信息 */
    BOOL bRebootFlag /* 异常后是否重启 */
);

/***********************************************************************
* DeleteSelfTaskFromList - 从监视的文件列表中删除对本任务的监视结点
*
* RETURNS: 无
*
*/
extern void DeleteSelfTaskFromList();

/* show the list of monitored task.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void EP_ShowMonitorTaskList(void);

/* 获得装置设备名称.
 * Para:
 *     ppucRtDevNameAddr, 返回装置设备名称字符串基址.
 *     piRtNameLen, 返回设备名称字符串长度(注意，不包括"\0").
 * Return:
 *     NONE.
 */
extern EP_STATUS EP_GetDevName(uint8_t **ppucRtDevNameAddr, int *piRtNameLen);

/***********************************************************************
* EP_GetPwrFreq - Get the system frequency
*
* RETURNS:
*               50 or 60: Normal
*               0: Error
*
*
*/
extern u_int EP_GetPwrFreq(void);

/***********************************************************************
* EB_GetLanguageType - Get the Language Type
*
* RETURNS: None
*
*
*/
extern void EB_GetLanguageType(void);

/* 解析光功率报文.
 * Para:
 *     ptr, 数据指针.
 *     rcvSubLen, 数据长度.
 *     ucAddr, CC板地址.
 * Return:
 *     EP_SUSSESS, success.
 *     EP_ERROR, fail.
 */
extern EP_STATUS ParseOptWatt(uint8_t *srcPtr, int32_t rcvSubLen, uint8_t ucAddr);

/* 解析光功率应用报文.
 * Para:
 *     ptr, 数据指针.
 *     rcvSubLen, 数据长度.
 *     pCcSts, CC板状态.
 * Return:
 *     EP_SUSSESS, success.
 *     EP_ERROR, fail.
 */
extern EP_STATUS ParseOptApp(uint8_t *srcPtr, int32_t rcvSubLen, T_CC_STS *pCcSts);

/* 初始化主板光口信息.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUSSESS, success.
 *     EP_ERROR, fail.
 */
extern EP_STATUS EP_InitCPUWatt(void);

/* 更新主CPU光功率
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
extern int update_sfp_info(void);

#ifdef  __cplusplus
}
#endif

#endif
