/* errtest.h - subroutine library for handling interface to report system errors */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01c, 29oct02, hdx Updated to version 1.0.
01b, 29aug02, hdx Verified version 0.1.
01a, 27jul02, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This module includes subroutine library for handling interface to report system errors.
*/

#ifndef ERRTEST_H
#define ERRTEST_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "edpbase.h"

/* defines */

/* 系统错误代码 */
#ifndef EDP02_PSR_BUILD
#define EV_POWER_ON 0           /* 装置上电 */
#define EV_STORAGE_ERR 1           /* 存储器错误 */
#define EV_SECT_ERR 2           /* 无效定值区 */
#define EV_SET_ERR 3           /* 定值校验错误 */
#define EV_DIDO_ERR 4           /* 开入开出异常 */
#define EV_SAMPLE_ERR 5           /* 采样系统异常 */
#define EV_SOFTWARE_CHECK_ERR 6          /* 程序校验错误 */
#define EV_WATCH_CPU_ALARM 7          /* 监视CPU告警 */
#define EV_EXT_COM_ALARM 8          /* 扩展机箱告警 */
#define EV_FPGA_ERR 9			 /*FPGA接口错误*/
#define EV_REL_GSE_A_NET_HALT 31          /* 保护goose的A网接收中断 */
#define EV_REL_GSE_B_NET_HALT 32          /* 保护goose的B网接收中断 */
#define EV_GSE_CONFI_ERR 33          /* 保护GOOSE配置错误 */
#define EV_REL_GSE_C_NET_HALT 34          /* 保护goose的C网接收中断 */
#define EV_REL_GSE_D_NET_HALT 35          /* 保护goose的D网接收中断 */
#define EV_REL_GSE_E_NET_HALT 36          /* 保护goose的E网接收中断 */
#define EV_REL_GSE_F_NET_HALT 37          /* 保护goose的F网接收中断 */
#define EV_REL_GSE_G_NET_HALT 38          /* 保护goose的G网接收中断 */
#define EV_REL_GSE_H_NET_HALT 39          /* 保护goose的H网接收中断 */
#define EV_REL_GSE_I_NET_HALT 40          /* 保护goose的I网接收中断 */
#define EV_REL_GSE_J_NET_HALT 41          /* 保护goose的J网接收中断 */
#define EV_REL_GSE_K_NET_HALT 42          /* 保护goose的K网接收中断 */
#else
#define EV_POWER_ON          0           /* 装置上电 */
#define EV_STORAGE_ERR       1           /* 存储器错误 */
#define EV_DOC_ERR           2           /* 文件系统错误 */
//#define EV_FLASH_ERR        3           /* 闪存错误 */
//#define EV_EEP_ERR          4           /* EEPROM错误 */
#define EV_SECT_ERR          3           /* 无效定值区 */
#define EV_SET_ERR           4           /* 定值校验错误 */
#define EV_DIDO_ERR          5           /* 开入开出异常 */
//#define EV_DO_ERR           8           /* 开出异常 */
#define EV_SAMPLE_ERR        6           /* 采样系统异常 */
#define EV_AD_POW_ERR        7          /* 内部电源异常*/
#define EV_FIBER_ERR         8           /*光差通信接口异常*/
#define EV_IO_ERR            9           /*IO模件通信异常*/
#define EV_EXT_COM_ERR       10          /* 扩展机箱通信中断 */
#define EV_RELAY_CONFI_ERR   11          /*保护配置错误*/
#define EV_SOFTWARE_CHECK_ERR  EV_RELAY_CONFI_ERR
#define EV_NET_CONFI_ERR     12          /*网络配置错误*/
#define EV_WATCH_CPU_ALARM   13          /*监视CPU告警*/
#define EV_EXT_COM_ALARM     14          /* 扩展机箱告警 */
#define EV_VERSION_CHECK_ERR 15          /*版本校验错误*/
#define EV_WATCH_CPU_COM_ERR 16          /*监视CPU通信异常*/
#define EV_FPGA_ERR			 17			 /*FPGA接口错误*/
#define EV_REL_GSE_A_NET_HALT    18          /* 保护goose的A网接收中断 */
#define EV_REL_GSE_B_NET_HALT    19          /* 保护goose的B网接收中断 */
#define EV_GSE_CONFI_ERR     20          /* 保护GOOSE配置错误 */
#define EV_HINT_INFO        24          /* 提示信息 */
#define EV_REL_GSE_CONFREV_ERR     33          /* 保护goose的接收到的CONFVER配置错误 */
#endif

#define MAX_SYS_ERR_NUM     64         /* 系统事件总个数 */

#define MMI_S_AE_HINT_INFO (254)
#define MAX_S_HMI_ERR_NUM    3        /* HMI事件总个数 */
#define MMI_S_AE_BASE_ERR  208

#define MMI_S_AE_INNER_COMM_ERR (MMI_S_AE_BASE_ERR) /*系统配置文件读取失败*/
#define MMI_S_AE_TIME_SYNCHRO_ERR  (MMI_S_AE_BASE_ERR+1)   /*对时信号异常错误*/
#define MMI_S_AE_MMI_ERR   (MMI_S_AE_BASE_ERR+2) /*HMI模件异常*/



#define ER_ALERT    0x0001              /* 呼唤 */
#define ER_REPORT   0x0002              /* 保存到实时/历史事件记录 */
#define ER_ALARM    0x0004              /* 告警 */
#define ER_LOCK     0x0008              /* 闭锁保护功能 */
#define ER_VERSION  0x0010              /* version.ini不匹配告警 */
#define ER_SCI_CHANGE 0x0020            /* sci配置信息改变告警 */
#define ER_NOLOGWRITE   0x0040            /*记录简明信息日志标记*/
#define ER_DIGI_SAMPLE  0x0080              /* 数字采样消失 */

/***********************呼唤相关定义*******************************/
#define MAX_REG_ALERT_NUM 256 /* 允许注册的最大呼唤个数 */



enum STORAGE_OPR_TYPE_ERR
{
    READING_ERR,/*存储区读出错*/
    WRITING_ERR,/*存储区写出错*/
    CRC_CHECK_ERR,/*存储区校验出错*/
    PO_CHANGE,/*因配置更新PO配置个数改变*/
    FILE_INIT_ERR,/*文件系统初始化异常*/
    SYS_FOLDER,/*SYS目录创建失败*/
    SET_FOLDER,/*SET目录创建失败*/
    DATA_FOLDER,/*DATA目录创建失败*/
    OPR_FOLDER,/*OPR目录创建失败*/
    OPR1_FOLDER,/*OPR1目录创建失败*/
    OPR2_FOLDER,/*OPR2目录创建失败*/
    OPR3_FOLDER,/*OPR3目录创建失败*/
    OPR4_FOLDER,/*OPR4目录创建失败*/
    OPR5_FOLDER/*OPR5目录创建失败*/
};

/* 定值校验错误错误码 */
enum SETTING_ERR_CODE
{
    INDEX_SETTING_PAGE_NUM_READ_ERR,  /* 索引定值页序读出错 */
    INDEX_SETTING_PAGE_NUM_OVERFLOW_ERR,  /* 索引定值页序越界 */
    INDEX_SETTING_PAGE_NUM_WRITE_ERR,   /* 索引定值页序写入出错 */
    INDEX_SETTING_PAGE_NUM_CHG_FALG_WRITE_ERR,   /* 索引定值页序改变标志写入出错 */
};

enum DIO_ERR
{

    DO_PROBLOM,/*开出实效*/
    DO_BREAKDOWN,/*开出击穿*/
    DO_HIGH_ERR,/*拉高检测异常*/
    DO_LOW_ERR,/*拉低检测异常*/
    DO_CHECK_ERR,	/*自检异常*/
    DO_QIDONG_ERR,/*启动失效*/
    IO_INIT_ERR,/*IO板初始化失败*/
    IO_RESET_ERR,/*IO模件复位*/
    AO_INIT_ERR,/*AO初始化异常*/
    IO_MOD_DI_INIT_ERR,/*不能初始化本机箱内的DI*/
    IO_MOD_AI_INIT_ERR,/*不能初始化本机箱内的AI*/
    EXT_IO_DI_INIT_ERR,/*不能初始化扩展机箱内的DI*/
    FIBER_IO_INIT_ERR,/*光纵通道虚拟机箱IO初始化失败*/
    FIBER_IO_DI_INIT_ERR,/*不能初始化光纵通道1虚拟机箱内的DI*/
    PTL_IO_INIT_ERR,/*同杆并架虚拟机箱IO初始化失败*/
    PTL_DI_INIT_ERR,/*不能初始化同杆并架机箱内的DI*/
    IOB_IO_INIT_ERR,/*智能操作箱IO初始化失败*/
    IOB_DI_INIT_ERR,/*不能初始化智能操作箱内的DI*/
    XINI_DI_INIT_ERR,/*不能初始化虚拟机箱内的DI*/
    IO_MOD_DO_INIT_ERR,/*不能初始化本机箱内名为的DO*/
    FIBER_IO_DO_INIT_ERR,/*不能初始化光纵通道虚拟机箱内的DO*/
    REDUN_DO_INIT_ERR,/*不能初始化冗余机箱的DO*/
    PTL_DO_INIT_ERR,/*不能初始化同杆并架虚拟机箱内的DO*/
    IOB_DO_INIT_ERR,/*不能初始化智能操作箱内的DO*/
    XUNI_DO_INIT_ERR,/*不能初始化虚拟机箱内的DO*/
    SUB_MOD_INIT_ERR,	/*不能初始化地址为%d的IO模件*/
    SPI_CHECK_ERR/*检测到IO模件SPI校验和错误*/
};

enum SAMPLING_ERR
{
    AD_REDUNDANT_INIT_ERR,/*初始化冗余机箱异常*/
    AD_PTL_INIT_ERR,/*初始化同杆并架机箱失败*/
    AD_IOB_INIT_ERR,/*初始化同杆并架机箱失败*/
    AD_XUNI_INTI_ERR,/*初始化虚拟机箱异常*/
    EXT_SYN_SAMPLE_ERR,/*初始化扩展机箱失败*/
    AD_DSP_INIT_ERR,/*初始化DSP失败*/
    INVALID_SYNCHRONIZATION,/*无效同步点*/
    HDL_LOST_DATE,/*智能操作箱连续丢点*/
    XUNI_LOST_DATA,/*虚拟机箱连续丢点*/
    NO_SAMPLING_DATE,/*光采样接收任务异常，无采样数据*/
    SAMPLING_LIST_FULL,/*采样数据队列满*/
    AD_TRANSTROM_ERR,/*A/D转换错误，闭锁保护*/
    SAMPLING_LIST_EMPTY,/*采样数据队列空*/
    AD_RESET_ERR,/*AD复位失败*/
    DRIFT_BEYOND_LIMITS,/*零漂越限*/
    MAIN_SLAVE_LOST_STEP,/*主从机箱采样失步*/
    MAIN_BOX_LOST_DATE,/*主机箱本地DSP采样连续丢点*/
    EXT_BOX_LOST_DATE,/*扩展机箱本地DSP采样连续丢点*/
    HEBINGQI_ERR,/*合并器告警*/
    HEBINGQI_TEST_STATE,/*合并器处于测试态*/
    HEBINGQI_WAKE_STATE,/*合并器处于唤醒期间，数据无效*/
    HEBINGQI_CANT_CHAZHI,/*数据不能使用插值算法同步*/
    HEBINGQI_SYN_ERR,/*数据同步丢失或无效*/
    HEBINGQI_CHANNEL_ERR,/*数据通道无效*/
    OPT_BOX_INIT_ERR,   /*初始化光差通道失败*/
    FPGA_SAMP_DATA_LOST, /* FPGA数据丢点 */
};

enum SOFTWARE_CHECK_ERR
{
    NOT_MATCH_VERSION_FILE, /* 现有版本和version.ini文件中记录不一致 */
    NOT_MATCH_INDEX_SETTING,   /* 索引定值不匹配 */
};

enum WATCH_CPU_COM_ALARM
{
    WATCH_CPU_INIT_ERR,/*电压监视模块 初始化失败*/
    OPEN_MEGA_ERR,/*打开电压监视MEGA16通信口失败*/
    MEGA_COM_ERR,/*向MEGA16监视小CPU发送信息失败*/
    VOL_INIT_ERR,
    GET_VOL_INFO_ERR,
    CPU_START_LOCK,/*电压监视小CPU，处于启动闭锁状态*/
    CPU_LOCK_RESUME,/*电压监视小CPU，启动闭锁状态恢复*/
    SET_RETRUE_STATS_DIFF,/*检测到启动设置状态和启动反馈状态不一致*/
    UP_FRAME_CHECK_ERR,/*上行帧长时间校验和错误*/
    UP_FRAME_RESUME,/*上行帧长时间错误或中断的异常恢复*/
    NO_UP_FRAME,/*上行帧长时间中断*/
    UP_FRAME_LENGTN_ERR,/*上行帧长度长时间异常*/
    DOWM_FRAME_INTERUPT,/*下行帧长时间通信中断*/
    NOT_SUPOORT_HW,/*监视小CPU软件未支持当前硬件版本*/
    UNKNOWN_ERR,/*监视小CPU发现未明异常*/
    GET_TEMPERATURE_ERR,/**/
    MAIN_CPU_RESET/*监视小CPU检测到主CPU连续复位*/

};

/* 扩展机箱告警错误码 */
enum EV_EXT_COM_ALARM_CODE
{
    INDEX_SETTING_PAGE_NUM_CHG_ERR,  /* 索引定值页序切换后通道系数下发扩展机箱出错 */
};

enum FPGA_ERR
{
    FPGA_INIT_ERR,/*FPGA初始化出错*/
    FPGA_LOCK_DO,/*FPGA给出A/D转换错误信息*/
    FPGA_INTER_TOO_LONG,/*FPGA中断间隔过长，不闭锁DO输出*/
    FPGA_INTER_TOO_LONG_LOCK_DO/*FPGA中断间隔过长，闭锁DO输出*/
};

/* typedefs */

typedef struct
{
    BOOL bAlertIsOpen;				/* 呼唤是否已经开放，TRUE: 为开放呼唤，FALSE: 为关闭呼唤 */
} ALERT_HDL;  				/* 呼唤操作句柄 */

typedef struct
{
    int iRegAlertCnt;				/* 已经注册的呼唤个数 */
    ALERT_HDL hAlertArr[MAX_REG_ALERT_NUM];					/* 呼唤句柄个数 */
} ALERT_REG_INFO; 				/* 呼唤注册相关信息 */

/* globals */

extern u_int uiEvtTimes_g;
extern char SysErrorName[LANGUAGE_TYPE_NUM][MAX_SYS_ERR_NUM][MESSAGE_MAX_LEN];
extern uint64_t SysErrEnableFlag_g;
extern uint16_t SysMaxErrNum_g;
extern BOOL g_baHmiSetErr[MMI_S_AE_HINT_INFO];

/* locals */

/* global functions */

/***********************************************************************
* ER_Set_Err - 报告错误
*
* RETURNS: 无
*
* 注意: 此函数可以在中断的上下文中调用
* 				  unOpFlag是对错误作出的反应，这是一个位标志，可以根据需要把它们
*               或起来（当标志为0时最基本的错误反应是记录到日志中）
*              #define ER_ALERT    0x0001     呼唤
*              #define ER_REPORT   0x0002     保存到实时/历史事件记录
*              #define ER_ALARM    0x0004              告警
*              #define ER_LOCK     0x0008      闭锁保护功能
*/
void ER_Set_Err(
    uint16_t unErrCode, 		/* 错误代码，需使用预定义的符号 */
    uint16_t unOpFlag,	/* 对错误做出的反应 */
    const uint8_t *strFmt, 			/* 欲显示的格式字符串。类似printf格式串，但不支持浮点数 */
    int iArg1, 		/* 格式字符串中指定要输出的参数，如不足2个，最后应补0 */
    int iArg2
);

/***********************************************************************
* ER_Set_Err_Stat - 报告错误(带状态，目前提供动作和返回)
*
* RETURNS: 无
*
* 注意: 此函数可以在中断的上下文中调用
* 				  unOpFlag是对错误作出的反应，这是一个位标志，可以根据需要把它们
*               或起来（当标志为0时最基本的错误反应是记录到日志中）
*              #define ER_ALERT    0x0001     呼唤
*              #define ER_REPORT   0x0002     保存到实时/历史事件记录
*              #define ER_ALARM    0x0004              告警
*              #define ER_LOCK     0x0008      闭锁保护功能
*/
void ER_Set_Err_Stat(
    uint16_t unErrCode, 		/* 错误代码，需使用预定义的符号 */
    uint16_t unOpFlag,	/* 对错误做出的反应 */
    const uint8_t *strFmt, 			/* 欲显示的格式字符串。类似printf格式串，但不支持浮点数 */
    int iArg1, 		/* 格式字符串中指定要输出的参数，如不足2个，最后应补0 */
    int iArg2,
    uint32_t ulSts,
    uint32_t ulUserInfo
);

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
                           int iArg1, int iArg2, int iArg3, int iArg4, int iArg5, int iArg6);

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
);

/***********************************************************************
* GetSysErrFlag - 获取全局系统错误状态
*
* RETURNS: 错误状态
*
*/
uint64_t GetSysErrFlag(void);

/***********************************************************************
* GetSysErrFlagRelayStop - 获取系统错误状态(保护退出)
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL GetSysErrFlagRelayStop();

/***********************************************************************
* GetSysErrFlagRelayContinue - 获取系统错误状态(保护不退出)
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL GetSysErrFlagRelayContinue();

/***********************************************************************
* GetGlobalErrorFlag - 设置逻辑图扫描获得的错误变量
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL GetGlobalErrorFlag(void);

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
void *ER_RegAlertSignal(void);

/***********************************************************************
* ER_ClearAlertSignal - 收回某应用对应的呼唤
*
* RETURNS: 无
*
*/
void ER_ClearAlertSignal(
    void *pvAlertHdl		/* 该呼唤对应的句柄，由ER_RegAlertSignal函数得到 */
);

/***********************************************************************
* ER_Clear_Alert - 清除掉呼唤，供复归时调用
*
* RETURNS: 无
*
*/
void ER_Clear_Alert(void);

/***********************************************************************
* ER_SetAlertSignal - 设置某应用对应的呼唤
*
* RETURNS: 无
*
*/
void ER_SetAlertSignal(
    void *pvAlertHdl		/* 该呼唤对应的句柄，由ER_RegAlertSignal函数得到 */
);

/***********************************************************************
* ER_InitAlertFunc - 初始化呼唤功能，必须在初始化函数中被调用
*
* RETURNS: 无
*
*/
void ER_InitAlertFunc(void);

/***********************************************************************
* ER_IsSetAlertFlag - 获得是否设置呼唤标志
*
* RETURNS: TRUE: 已经设置呼唤
*                 FALSE: 未设置呼唤
*
*/
BOOL ER_IsSetAlertFlag();

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
    uint16_t unOpFlag);

/*
数字化采样异常告警函数
strFmt 欲显示的格式化字符串
iArg1 欲显示的格式字符串。类似printf格式串，但不支持浮点数, 要求长度小于200字节
iArg2 格式字符串中指定要输出的参数，如不足2个，最后应补0
*/
void ER_App_Set_Digital_AD_Err(
    const uint8_t *strFmt, 			/* 欲显示的格式字符串。类似printf格式串，但不支持浮点数, 要求长度小于200字节 */
    int iArg1, 		/* 格式字符串中指定要输出的参数，如不足2个，最后应补0 */
    int iArg2,
    uint16_t unOpFlag);

/*
数字化采样异常告警复归函数
strFmt 欲显示的格式化字符串
iArg1 欲显示的格式字符串。类似printf格式串，但不支持浮点数, 要求长度小于200字节
iArg2 格式字符串中指定要输出的参数，如不足2个，最后应补0
*/
void ER_App_Clear_Digital_AD_Err(
    const uint8_t *strFmt, 			/* 欲显示的格式字符串。类似printf格式串，但不支持浮点数, 要求长度小于200字节 */
    int iArg1, 		/* 格式字符串中指定要输出的参数，如不足2个，最后应补0 */
    int iArg2,
    uint16_t unOpFlag);

/*
数字化采样异常呼唤函数
strFmt 欲显示的格式化字符串
iArg1 欲显示的格式字符串。类似printf格式串，但不支持浮点数, 要求长度小于200字节
iArg2 格式字符串中指定要输出的参数，如不足2个，最后应补0
*/
void ER_App_Set_Digital_AD_Alert(
    const uint8_t *strFmt, 			/* 欲显示的格式字符串。类似printf格式串，但不支持浮点数, 要求长度小于200字节 */
    int iArg1, 		/* 格式字符串中指定要输出的参数，如不足2个，最后应补0 */
    int iArg2,
    uint16_t unOpFlag);

/*
数字化采样异常呼唤复归函数
strFmt 欲显示的格式化字符串
iArg1 欲显示的格式字符串。类似printf格式串，但不支持浮点数, 要求长度小于200字节
iArg2 格式字符串中指定要输出的参数，如不足2个，最后应补0
*/
void ER_App_Clear_Digital_AD_Alert(
    const uint8_t *strFmt, 			/* 欲显示的格式字符串。类似printf格式串，但不支持浮点数, 要求长度小于200字节 */
    int iArg1, 		/* 格式字符串中指定要输出的参数，如不足2个，最后应补0 */
    int iArg2,
    uint16_t unOpFlag);

#ifdef	__cplusplus
}
#endif

#endif                                  /* ERRTEST_H */
