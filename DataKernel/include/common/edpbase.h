/* edpbase.h - This file contains system initilization procedures */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02b, 20may09, dy change the code style.
02a, 29jul06, dy add the interface to BSP and the rebooting function.
01a, 27sep02, hdx first created.
*/

/*
DESCRIPTION
This file contains system initilization procedures.
DATA STRUCTURES:
	EP_STATUS: Data type of status used in EDP-01.
	int64_t: 64-bit int.
	uint64_t: 64-bit unsigned int.
	COMPLEX: Float complex type.
	FLT_U32_UNION: Union of a float and a uint32_t.
FUNCTIONS:
	REAL: Get real part of a COMPLEX.
	IMAGE: Get image part of a COMPLEX.
	HI8: Get high 8-bit byte of 16-bit word.
	LO8: Get low 8-bit byte of 16-bit word.
	HI16: Get high 16-bit word of 32-bit dword.
	LO16: Get low 16-bit word of 32-bit dword.
	HH8: Get b31-b24 byte of 32-bit dword.
	HL8: Get b31-b24 byte of 32-bit dword.
	LH8: Get b15-b8 byte of 32-bit dword.
	LL8: Get b7-b0 byte of 32-bit dword.
	U8_TO_U16: Get uint16_t combined by 2 uint8_t.
	U16_TO_U32: Get uint32_t combined by 2 uint16_t.
	U8_TO_U32: Get uint32_t combined by 4 uint8_t.
	BV8: Get uint8_t bit value.
	BV16: Get uint16_t bit value.
	BV32: Get uint32_t bit value.
*/

#ifndef EDPBASE_H
#define EDPBASE_H

#ifdef	__cplusplus
extern "C" {
#endif

struct VxMsgQ; 

typedef struct VxMsgQ* MSG_Q_ID;



/* includes */
#include "vxWorks.h"

#define EP_SYS_SW_VER "3.67d.g-38"

#define EDP_BURN 	/* 是否调用主程序 */
#define EDP_01_02_BUILD
#define EDP_DYNAMICLOAD
#define VXWORKS_ROM
#define MAINSTATION
#define ETH_QUERY_MODE
#define NO_DBL_BUF
#define APP_LINE_SUPPORT
#define EDP02_GTP_BUILD 

#define EP_INNER_PRTCL_VER 0X0396  	/* 支持的内部规约版本号是3.96, 高8位为整数位, 低8位为小数位 */

/* 整体发布用特征码，当对保护应用接口发生改变或者有重大缺陷改进式需要更新，
 * 低字节表示缺陷更正，高字节表示应用接口变化
 */

#define EP_CHARACTER_LABEL 0x0102

/* 提示:
 * EP_STATUS类型定义了一些名称作为表征模块自检状态或者函数调用的结果，其它
 * 各模块可以根据情况使用EP_STATUS数据类型
 */

#define EP_SUCCESS 0
#define EP_ERROR -1
#define EP_LOCAL_MSG -2
#define EP_TIMEOUT -3
#define EP_PARM_ERR -4
#define EP_NOT_INIT -5
#define EP_HARD_ERR -6
#define EP_COM_ERR -7
#define EP_BAD_DATA -8
#define EP_IO_ERR -9
#define EP_BUF_ERR -10
#define EP_FRAME_ERR -11
#define EP_CLOSED -12
#define EP_FILE_ERR -13
#define EP_CFG_ERR -14
#define EP_SYS_ERR -15

#define REAL(x) __real__(x)
#define IMAGE(x) __imag__(x)
#define FLT_PRECISION 0.00001

#define SYS_SEC 100
#define SYS_TICK (1000/SYS_SEC)

/* 中断分配 */

#define IRQ_SPI 2
#define IRQ_TIMER1 12
#define IRQ_TIMER2 13
#define IRQ_TIMER3 14
#define IRQ_TIMER4 15
#define IRQ_DSP_AI 19
#define IRQ_AI_SYN 20
#define IRQ_GPS 22

/* for application platfrom. */

#define MAX_ID_LEN 128
#define MAX_SUB_LGC_NUM 16
#define MAX_EVT_PARM_NUM 32
#define MAX_ALLOW_RPT_NUM 500
#define MAX_ALLOW_REC_NUM 128
#define MAX_ALLOW_LOG_NUM 1000

/* The priority of the task, more smaller of the number more higher of the priority */

#define TSK_EXT_RCV_CMD 2
#define TSK_PRI_DSP 3
#define TSK_PRI_SMV_RCV 3	/* 61850-9-1 */
#define TSK_PRI_SMV_TX  3

#define TSK_PRI_HW_WATCH_DOG 1   /* 看门狗任务 */


/* EDP01_CA平台和EDP02平台 */
#define TSK_OPT_CH_SEND 2
#define TSK_EXT_SND_DAT 4

#define TSK_OPT1_CH_RCV 4
#define TSK_OPT2_CH_RCV 6

#define TSK_PRI_TERM_NET 34

/* 逻辑图扫描任务的优先级 */

#define TSK_PRI_MASTER_RELAY_SCAN 20  /* 主保护扫描任务的优先级 */
#define TSK_PRI_BACKUP_RELAY_SCAN 21  		/* 第1个后备保护扫描任务的优先级 */
#define TSK_PRI_SECOND_BACKUP_RELAY_SCAN 22  /* 第2个后备保护扫描任务的优先级 */
#define TSK_PRI_THIRD_BACKUP_RELAY_SCAN 23  /* 第3个后备保护扫描任务的优先级 */
#define TSK_PRI_FORTH_BACKUP_RELAY_SCAN 24      /* 第4个后备保护扫描任务的优先级 */
#define TSK_PRI_FIFTH_BACKUP_RELAY_SCAN 25  /* 第5个后备保护扫描任务的优先级 */
#define TSK_PRI_SIXTH_BACKUP_RELAY_SCAN 26         /* 第6个后备保护扫描任务的优先级 */
#define TSK_PRI_SEVENTH_BACKUP_RELAY_SCAN 27  /* 第7个后备保护扫描任务的优先级 */

#define TSK_PRI_61850_RUN 37
#define TSK_PRI_61850_SERVER_PROT 119

#define TSK_PRI_SPY 30	/* spy 任务优先级 */
#define TSK_PRI_VOLT_WATCH 50  /* 电压监视任务 */

/* 提升EDP02平台优先级,无影响 */
#define TSK_PRI_SYS 100	  /* 解决EDP01直接启动时丢点问题 */

#define TSK_PRI_EVT_SND 36  /* 自动上送任务 */
#define TSK_PRI_FAST_SEVER_SVR 38  /* 与MMI快速通信任务 */

#define TSK_PRI_REC_BUF 40
#define TSK_PRI_SEVER_SVR 75
#define TSK_PRI_LISTEN_SVR 80

#define TSK_PRI_WR_LOG 110  /* 日志写文件任务 */
#define TSK_PRI_RPT 120
#define TSK_PRI_REC_FILE 130
#define TSK_PRI_OPRE_LOG 140
#define TSK_PRI_REC_SVR 131 /* 录波server任务 */

/* Intelligent operation box, edp02 platform, and edp01_c-a platform. */
#define TSK_PRI_61850_INIT 112

#define TSK_PRI_TEST 199 /* for test task */

#define TSK_PRI_MSU 200    /* for measuremnt */
#define TSK_PRI_SPI 201    		/* for SPI Communication */
#define TSK_PRI_ZERO_EXCUR 202    /* for eliminate the zero excursion */
#define TSK_PRI_DST_TEST 210
#define TSK_PRI_MAINLOOP 90	  /* for slow task. */

/* platform status */

#define LOCK_DO 0x0001	 /* Lock DO */
#define LOCK_EVT 0x0002		/* Lock event */
#define NO_LICENSE 0x0004		/* No license */
#define SYS_GPS_ERR 0x0008
#define SYS_LOCK_DO 0x0010	/* Lock DO in system level */
#define SYS_LOCK_EVT 0x0020		/* Lock event in system level */
#define SYS_ENG_MODE 0x0040
#define USE_HW_LINK 0x0080	/* Hard link */
#define USE_SW_LINK 0x0100		/* Soft link */
#define EVT_NOT_CLR 0x0200	/* The event not be cleared. */
#define HW_TEST_MODE 0x0400	  /* Hardware test mode */
#define REBOOT_DLY 0x0800	/* System reboot */
#define CLR_EVT_FLAG 0x1000		/* The event have been cleared. */
#define SET_ALARM_FLAG 0x2000     /* Set alarm */
#define ON_FAR_STATE 0x4000	 /* Be in far operation mode. */
#define WDG_RESET 0x8000       /* Watchdog will reset the system. */
#define ON_EXAM_STATE 0x10000        /* Be in examination mode. */
#define THREEUO_WARN_STATE 0x20000		/* 3U0 overflow */
#define STOP_LOGIC_SACN_FLAG 0x40000  /* Stop logic scanning. */
#define VERSION_NOT_MATCHED_FLAG 0x80000   /* version.ini not matched */
#define SCI_CHANGED_FLAG 0x100000	/* SCI file changed alarm */
#define JGS_STATE 0x200000	/* JGS state */
#define DIGITAL_SAMPLE_ERR_FLAG 0x400000				/* digital sample err flag */

#define LINK_MODE_NONE 0x0000 	/* 压板模式无效 */
#define LINK_MODE_HW 0x0001 		/* 总硬压板模式 */
#define LINK_MODE_SW 0x0002 		/* 总软压板模式 */
#define LINK_MODE_AND 0x0004  /* 总软硬相与模式 */
#define LINK_MODE_OR 0x0008 	 /* 总软硬相或模式 */
#define LINK_MODE_CUS 0x0010  /* 定制模式 */

#define EP_IN_HW_TEST() (uiEdpStatus_g & HW_TEST_MODE)	/* Be in hardware test mode */
#define EP_DO_EVT_CLR() (uiEdpStatus_g & CLR_EVT_FLAG)	  /* The system is clearing the event. */
#define EP_IN_WDG_RESET() (uiEdpStatus_g & WDG_RESET)  /* The system is in watchdog reset mode. */
#define EP_IN_LOGIC_STOP() (uiEdpStatus_g & STOP_LOGIC_SACN_FLAG)    /* The system is in logic scanning stopping mode. */

#define IO_PIN_BOOT_SEL 0x0001	/* 启动方式选择 */
#define IO_PIN_FAST_BOOT 0x0002		/* 启动速度及超级终端是否初始化选择 */

#define BASE_TIMER_T (1000000000L/(sysInputFreq_g/4))   /* SCC_CLK周期，时间单位为ns */
#define MESSAGE_MAX_LEN 64
#define LANGUAGE_TYPE_NUM 2		/* 语言种类 */

#define LOGIC_DELAY_ONE_LEVEL 1 /* 一级滞后标识 */
#define LOGIC_DELAY_TWO_LEVEL 2 /* 二级滞后标识 */

typedef int BOOL;

//typedef char int8_t;
// typedef unsigned char uint8_t;
// typedef short int16_t;
// typedef unsigned short uint16_t;
// typedef int int32_t;
// typedef unsigned int uint32_t;
// typedef long long int64_t;
// typedef unsigned long long uint64_t;
typedef int STATUS;

/* 各种宽度的整数相互转化 */

/* Get high 8-bit byte of 16-bit word. */
#define HI8(un) (uint8_t)(((un)>>8) & 0xFF)

/* Get low 8-bit byte of 16-bit word. */
#define LO8(un) (uint8_t)((un) & 0xFF)

/* Get high 16-bit word of 32-bit dword. */
#define HI16(ul) (uint16_t)(((ul)>>16) & 0xFFFF)

/* Get low 16-bit word of 32-bit dword. */
#define LO16(ul) (uint16_t)((ul) & 0xFFFF)

/* Get b31-b24 byte of 32-bit dword. */
#define HH8(ul) (uint8_t)(((ul)>>24) & 0xFF)

/* Get b23-b16 byte of 32-bit dword. */
#define HL8(ul) (uint8_t)(((ul)>>16) & 0xFF)

/* Get b15-b8 byte of 32-bit dword. */
#define LH8(ul) (uint8_t)(((ul)>>8) & 0xFF)

/* Get b7-b0 byte of 32-bit dword. */
#define LL8(ul) (uint8_t)((ul) & 0xFF)

/* Get uint16_t combined by 2 uint8_t. */
#define U8_TO_U16(ucH8, ucL8)\
	(uint16_t)((((uint8_t)(ucH8))<<8)|(uint8_t)(ucL8))

/* Get uint32_t combined by 2 uint16_t. */
#define U16_TO_U32(unH16, unL16)\
	(uint32_t)((((uint16_t)(unH16))<<16)|(uint16_t)unL16)

/* Get uint32_t combined by 4 uint8_t. */
#define U8_TO_U32(ucHH8, ucHL8, ucLH8, ucLL8)\
	(uint32_t)((((uint8_t)(ucHH8))<<24)|(((uint8_t)(ucHL8))<<16)|\
	((uint8_t)(ucLH8)<<8)|(uint8_t)(ucLL8))

/* Intel次序(Little Endian)的字节流转化为32位整数
 * 参数：pucIn，待转换的Intel次序(Little Endian)4字节输入
 * 返回值：32位整数转换结果
 * 注意：输入的4字节必须按照低字节低地址(Little Endian)的Intel次序
 *       (和EDP 01的内部通信规约相匹配)
 */
#define BYTES_TO_U32(pucIn) U8_TO_U32(((uint8_t*)(pucIn))[3],\
	((uint8_t*)(pucIn))[2], ((uint8_t*)(pucIn))[1], ((uint8_t*)(pucIn))[0])

/* Intel次序(Little Endian)的字节流转化为浮点数
 * 参数：pucIn，待转换的Intel次序(Little Endian)4字节输入
 * 返回值：浮点数转换结果
 * 注意：输入的4字节必须按照低字节低地址(Little Endian)的Intel次序
 *       (和EDP 01的内部通信规约相匹配)
 */
#define BYTES_TO_FLT(pucIn) U8_TO_FLT(((uint8_t*)(pucIn))[3],\
	((uint8_t*)(pucIn))[2], ((uint8_t*)(pucIn))[1], ((uint8_t*)(pucIn))[0])

/* 位标志 */

#define BV8(b) (assert(b<8), (uint8_t)1<<(b))
#define BV16(b) (assert(b<16), (uint16_t)1<<(b))
#define BV32(b) (assert(b<32), (uint32_t)1<<(b))

/* 内存释放 */

#ifndef NDEBUG
#define EP_free(p) do {assert(p), free(p), p = NULL;} while (0)
#else
#define EP_free(p) free(p)
#endif

/* 总线频率类型 */
#define BUS_FREQ_50MHZ 50000000L
#define BUS_FREQ_66MHZ 66000000L
#define BUS_FREQ_100MHZ 100000000L

#define TEMP_INFO_MAX_LEN 256 /* 临时字符串长度 */

/* 扫描任务最大个数 */
#define MAX_AI_FUNC_NUM 8

#define MAX_CHN_NUM_IN_BAY 30   /* 间隔最大通道数 */

#define LINE_MAX_FRM_PER_POLL 12 /* 线路保护每次查询帧数 */
#define LINE_MAX_BYTE_PER_POOL (4400) /* 线路保护每次查询字节数 */
#define LINE_QD_MAX_FRM_PER_POLL 1 /* 启动态线路保护每次查询帧数 */

/**/
#define SLOW_MSG_MAX_CHAR_NUM 128 /*慢速任务中条目描述的最大字节数 张全140707*/

/* #define ITEM_LEN_81 */ /* 初始项长度81 */

#ifdef ITEM_LEN_81
#define ITEM_LEN 81 /* 压板等配置项文件存储长度 */
#define ITEM_NAME_LEN 64 /* 压板等配置项文件存储名称长度 */
#define ITEM_VALUE_LEN 17 /* 压板等配置项文件存储值长度 */
#else
#define ITEM_LEN 15 /* 压板等配置项文件存储长度 */
#define ITEM_NAME_LEN 10 /* 压板等配置项文件存储名称长度 */
#define ITEM_VALUE_LEN 5 /* 压板等配置项文件存储值长度 */
#endif

#define TRUE 1
#define FALSE 0 

/* typedefs */

typedef int EP_STATUS;
typedef __complex__ float COMPLEX;

/* The function point for showing the error information on interface, especially when the configuration occur. */
typedef uint8_t (*pFV)(char *, char *);

/* 参数校验回调函数
 * 校验正常返回TRUE, 校验异常返回FALSE
 */
typedef BOOL (*pPARACHECKFV)(void);

/* 用于浮点数到字节流转换的联合 */
typedef union
{
    float fVal;
    uint32_t ulVal;
    int32_t lVal;
} FLT_U32_UNION;

enum BOOT_REASON
{
    BOOT_COLDRESET =0,   /* cold, 上电复位 */
    BOOT_SW,   /* 软狗复位 */
    BOOT_HW,  		/* 硬狗复位 */
    BOOT_REBOOT,  /* reboot()执行或执行CTRL+X复位 */
    BOOT_JTRS, 		/* jtag reset */
    BOOT_CSRS, 	/* CHECK STOP */
    BOOT_BMRS  		/* bus monitor reset */
};   		/* boot原因，BSP提供 */

enum REBOOT_REASON
{
    REBOOT_UNKNOWN = 0,  /* 未知原因调用reboot. */
    REBOOT_ACTIVE,    /* 平台软件主动调用. */
    REBOOT_EXCEP  /* 异常调用. */
};

/* 应用类型 */
typedef enum
{
    APP_TYPE_TRAD = 0,  /* 传统应用. */
    APP_TYPE_DIG    /* 数字化应用. */
} APP_TYPE;

/* 硬件类型 */
typedef enum
{
    BOARD_TYPE_E01 = 0,  /* EDP01平台. */
    BOARD_TYPE_E02,    /* EDP02平台. */
    BOARD_TYPE_E03,    /* EDP03平台. */
    BOARD_TYPE_EXCITE,    /* 励磁平台. */
} BOARD_TYPE;

/* 各种应用类型 */
enum App_Type
{
    APP_COM = 0x00,  /* 通用保护装置 */
    APP_LINE = 0x01,   /* 中高压线路保护装置 */
    APP_TRANS = 0x02,  /* 中高压变压器保护装置 */
    APP_BUS = 0x03, /* 中高压母差保护装置 */
    APP_PROT_MEA_MERGE = 0x04,  /* 中高压保护测控一体化装置 */
    APP_LOW_PROT = 0x05,  /* 低压保护测控装置 */
    APP_EXCITE = 0x06,	/* 励磁调节器装置 */
    APP_STAB_CONTROL = 0x07,  /* 稳定控制装置 */
    APP_MT = 0X8,  /* 铁路自动化装置 */
    APP_NO_ELEC = 0x09,    /* 非电量保护装置 */
    APP_INTEL_BOX = 0xa,      /* 智能操作箱 */
    APP_INVALID = 0xb,  /* 非法应用类型 */
};


/* 空闲统计 */
typedef struct baseIdleStat
{
    uint64_t maxCount;
    uint64_t curCount;
    uint64_t lstCount;
    uint64_t intCount;
    uint32_t idlePercent;
    uint32_t ulWdCnt;  /* 看门狗触发次数 */
    uint64_t ullTotalCount;  /* 总计数 */
} BASE_IDLE_STAT, *pBASE_IDLE_STAT;

/* 算法注册结构 */
typedef struct tag_RD_REG_FUNC_AI
{
    void (*pfUser)(void *pvParm);
    void *pvParm;
    u_int uiPts;
    u_int uiCnt;
} RD_REG_FUNC_AI;


/* 逻辑分图属性 */
typedef struct
{
    uint8_t aucName[MAX_ID_LEN+1];
    uint16_t usInterval;  /* 扫描周期, 以采样点数为单元 */
    BOOL bRun;
} SC_SUB_LGC_ITEM;

/* 定值项 */
typedef struct
{
    uint8_t ucUnit;
    uint8_t aucId[MAX_ID_LEN+1]; /* 逻辑标识 */
    uint8_t aucName[MAX_ID_LEN+1]; /* 内部名称 */
    uint8_t aucABRV[4]; /* 内部简称 */
    FLT_U32_UNION valMax; /* 最大值 */
    FLT_U32_UNION valMin;  /* 最小值 */
    FLT_U32_UNION valDft; /* 缺省值 */
    FLT_U32_UNION valStep; /* 步长 */
    FLT_U32_UNION valMaxOrg; /* 软件配置原始最大值 */
    FLT_U32_UNION valMinOrg;  /* 软件配置原始最小值 */
    FLT_U32_UNION valDftOrg; /*软件配置原始 缺省值 */
    FLT_U32_UNION valStepOrg; /* 软件配置原始步长 */
    FLT_U32_UNION valNow;    /* 当前值 */
    uint8_t *pucUnitName;
    uint8_t ucAttr; /* 内部定值属性, 0: 内部定值; 1: 索引定值 */
    uint8_t ucType; /* 定值类别 */
    BOOL bIsPrvtUse; /* 内部定值专用 */
    BOOL bStdSet; /* 0: 自定义定值 1: 国网标准定值 */
    BOOL bAutoSet;  /* 是否可自动整定 */
    SC_SUB_LGC_ITEM *psublgc;  /* 内部定值专用，内部定值所属保护 */
    uint8_t aucDftStr[MAX_ID_LEN+1];   /* 存储缺省字符串定值的内容，注意不包括'\0' */
    uint8_t aucNowStr[MAX_ID_LEN+1]; /* 存储当前整定字符串定值的内容，注意不包括'\0' */
} SC_SET_ITEM;

/* 定值页 */
typedef struct
{
    uint8_t aucName[MAX_ID_LEN+1];
    BOOL bIsPub;
    SC_SUB_LGC_ITEM *psublgc;
    int iSetNum; /* 定值页数 */
    SC_SET_ITEM *pset;
} SC_SET_PAGE;

/* 压板状态 */
typedef struct
{
    uint8_t aucId[MAX_ID_LEN+1]; /* 逻辑标识 */
    uint8_t aucName[MAX_ID_LEN+1];  /* 内部名称 */
    uint8_t aucABRV[4]; /* 内部简称 */
    BOOL bDftVal; /* 缺省值 */
    BOOL bSwVal;    /* 软压板值 */
    uint8_t aucDiSrc[MAX_ID_LEN+1];
    void *pvHdDI;
    uint8_t aucSecondDiSrc[MAX_ID_LEN+1]; /* 硬压板双开入模式下第二个开入的逻辑标识 */
    void *pvHdSecondDI;
    uint8_t HwLinkType;  /* 硬压板模式字 */
    uint8_t LinkSwitchMode;
    BOOL bNowVal;  /* 有效压板值 */

    /* 表示该压板当前所处模式，只有在压板定制模式下(ulTotalLinkMode_g=LINK_MODE_CUS)生效
     * LINK_MODE_HW|LINK_MODE_SW|LINK_MODE_AND|LINK_MODE_OR|LINK_MODE_NONE
     */
    uint8_t aucMode;
    BOOL bJgCurFlg;  /* 是否判断电流有流标识 */
    uint32_t ulArrChnNo[MAX_CHN_NUM_IN_BAY];  /* 电流有流判断时, 所关联的物理通道号数组 */
    uint32_t ulChnNum;  /* 关联的通道数总数 */
} SC_LINK_ITEM;

/* globals */

extern int ENG_MODE;    /* Used for language control. */

extern int iIMMR_g;			/* inner memory address. */
extern u_int uiEdpStatus_g;			/* platform status. */
extern pFV pMessageBox;			/* function for showing. */
extern char MessageStr_g[MESSAGE_MAX_LEN];		/* showing content. */
extern BOOL bEnableAlarm_g;	/* The alarm flag 04 board. */
extern uint8_t ComVer_g;	  /* Type of the communication auxiliary board. */
extern uint8_t ComVerExt_g;		/* 扩展通讯板类型 */
extern char *pramLowMemAdrs;  /* RAM最低地址 */
extern MSG_Q_ID SlowMessage;		/* slow message ID. */
extern BASE_IDLE_STAT idleStat;   /* CPU空闲统计 */
extern APP_TYPE appType_g;   /* 应用类型 */
extern BOARD_TYPE bdType_g;   /* 硬件类型 */
extern uint8_t ucCpuSpiRol_g;		/* CPU在SPI通信中主从标志, 0: 标志无效; 1: 主CPU；2: 从CPU */
extern uint32_t sysInputFreq_g;   /* 系统总线频率 */
extern pPARACHECKFV pParaCheckFun; /* 参数(内部定值, 保护定值, 测控定值, 压板)校验回调函数 */
extern uint8_t uiAppType_g; /* 应用类型 */

/* locals */

/* 用于保存部分IO引脚信号, 0: 跳开, 1: 短接
 * bit0: BOOT_SEL跳线状态
 * bit1: FAST_BOOT跳线状态
 */

extern u_int uiInitErrFlag_g;		/* initialization error. */

/* for printer. */

extern BOOL NET_PRINTER;	/* TRUE: printer using Ethernet; FALSE: printer using UART. */
extern int32_t m_lPrnSerials;   /* terminal descriptor. */
extern uint32_t PrnBufPos;	 /* buffer position. */
extern char *PrnBuf;    /* printer buffer. */
extern char *pPrnBuf;         /* printer buffer. */

extern BOOL bSciChangedFlag_g; 		/* if changed. */
extern BOOL bCfgChgFlag_g;  /* 配置是否更改标志 */
extern int iBootReason_g;			/* Boot resean */
extern uint32_t g_ulStartTm;  /* 标记开始时间 */
extern uint32_t g_ulEndTm; /* 标记结束时间 */
extern uint32_t g_ulComsumeTm;  /* 消耗时间 */
extern RD_REG_FUNC_AI aregf_g[MAX_AI_FUNC_NUM+1];  /* 扫描任务驱动函数注册 */
extern BOOL bDspDrvMod; /* 数据驱动模式, FALSE: 无DSP任务模式; TRUE: DSP任务模式 */

/* global functions */

/***********************************************************************
* EP_Get_Sts_Bit - 获得装置当前状态
*
* RETURNS: 系统状态字
*
*/
__inline__ static u_int EP_Get_Sts_Bit()
{
    return uiEdpStatus_g;
}

/***********************************************************************
* GetInitErrFlag - 获得初始化过程错误标志
*
* RETURNS: 无
*
*/
__inline__ static uint32_t GetInitErrFlag(void)
{
    return uiInitErrFlag_g;
}

/* get the status of BOOT_SEL.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL EP_IS_BOOT_SEL(void);

/* get the status of QUICK_BOOT.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL EP_IS_FAST_BOOT(void);

/* 设置需复归标志，逻辑图中设置.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void EP_Set_ReSet_Flag();

/* 恢复需复归标志，慢速任务中执行复归操作后恢复.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void EP_Clr_ReSet_Flag();

/* 得到需复归标志, 慢速任务根据该标志来进行复归操作.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
BOOL EP_Get_ReSet_Flag();

/* Set system status(OR operation).
 * Parameter:
 *      uiSts, new bit flag to be set.
 * Return value:
 *      None.
 * Alert:
 *      This function can be called in ISR.
 */
void EP_Set_Sts_Bit(u_int uiSts);

/* Clear system status(AND operation).
 * Parameters:
 *      uiSts, bit flag to be cleared.
 * Return value:
 *      None.
 * Alert:
 *      This function can be called in ISR.
 */
void EP_Clr_Sts_Bit(u_int uiSts);

/* Check if running as 2nd CPU.
 * Parameters:
 *      None.
 * Return valud:
 *      TRUE, is the 2nd CPU(slave).
 *      FALSE, is not the 2nd CPU(master).
 * Alert:
 *      This function is supported by BSP.
 */
BOOL isNumber_2_04CPU(void);

/* Enter hardware test mode.  Logic function will be disabled.
 * Parameters:
 *      None.
 * Return value:
 *      None.
 */
void EP_Bgn_Hw_Test(void);

/* Exit hardware test mode.  System will reboot after several seconds.
 * Parameters:
 *      None.
 * Return value:
 *      None.
 */
void EP_End_Hw_Test(void);

/* Finish last system initializaion before logic running.
 * Parameters:
 *      None.
 * Return value:
 *      None.
 * Alert:
 *      This function is only called by RelayEngine module.
 */
void EP_Before_Lgc_Run(void);

/* 获得04板初始化完成标志.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, 初始化完成.
 *     FALSE, 初始化还未完成.
 */
BOOL EP_Get_04CPU_Init_End_Flag();

/* Get float combined by 4 uint8_t.
 * Para:
 *     pucOut, 用于存放4字节结果.
 *     ulIn, 待转换的32位数.
 * Return:
 *     result.
 */
static __inline__ float U8_TO_FLT(uint8_t ucHH8, uint8_t ucHL8,
                                  uint8_t ucLH8, uint8_t ucLL8)
{
    FLT_U32_UNION fu32;

    fu32.ulVal = U8_TO_U32(ucHH8, ucHL8, ucLH8, ucLL8);

    return fu32.fVal;
}

/* 32位整数转化为Intel次序(Little Endian)的字节流
 * 参数：pucOut，用于存放4字节结果
 *       ulIn，待转换的32位数
 * 返回值：无
 * 注意：转化出来的4字节是低字节低地址(Little Endian)的Intel次序
 *       (和EDP 01的内部通信规约相匹配)
 */
static __inline__ void U32_TO_BYTES(uint8_t *pucOut, uint32_t ulIn)
{
    pucOut[0] = LL8(ulIn);
    pucOut[1] = LH8(ulIn);
    pucOut[2] = HL8(ulIn);
    pucOut[3] = HH8(ulIn);
}

/* 浮点数转化为Intel次序(Little Endian)的字节流
 * 参数：pucOut，用于存放4字节结果
 *       fIn，待转换的浮点数
 * 返回值：无
 * 注意：转化出来的4字节是低字节低地址(Little Endian)的Intel次序
 *       (和EDP 01的内部通信规约相匹配)
 */
static __inline__ void FLT_TO_BYTES(uint8_t *pucOut, float fIn)
{
    FLT_U32_UNION fu32;

    fu32.fVal = fIn;
    pucOut[0] = LL8(fu32.ulVal);
    pucOut[1] = LH8(fu32.ulVal);
    pucOut[2] = HL8(fu32.ulVal);
    pucOut[3] = HH8(fu32.ulVal);
}

/***********************************************************************
* DisablePit - 关闭Pit
*
* RETURNS: 无
*
*/
void DisablePit(void);

/***********************************************************************
* runExternBsp - 是否执行externBSP
*
* RETURNS:
*          TRUE: 执行
*          FALSE: 不执行
*/
BOOL runExternBsp();

/***********************************************************************
* runExternMain - 是否执行externMain
*
* RETURNS:
*          TRUE: 执行
*          FALSE: 不执行
*/
// BOOL runExternMain();

/***********************************************************************
* initWDB - 是否初始化WDB
*
* RETURNS:
*          TRUE: 执行
*          FALSE: 不执行
*/
BOOL initWDB();

/***********************************************************************
* EDPreboot - 平台重启
*
* RETURNS: 无
*
*/
void EDPreboot
(
    int startType             /* how the boot ROMS will reboot */
);

/* Initialize the Net.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void EDP_Init_Net(void);

/***********************************************************************
* GetSysTaskStatus - 获取系统任务状态
*
* RETURNS: 无
*
*/
void GetSysTaskStatus(void);

/* 查询当前是否处于检修态.
 * Para:
 *     pbRtRepairSts, 返回检修状态，若返回值为非0值，则为检修态，若为0，则为运行态.
 * Return:
 *     非0值: 查询操作成功; 0: 查询操作失败.
 *
 * 注意:
 *     因为61850的BOOL和vxowrks都定义了BOOL，有冲突, 用地址来返回BOOL变量有问题，
 *     所以用固定长度的变量来返回
 *
 */
unsigned char EP_Get_Repair_Sts(unsigned char *pbRtRepairSts);

/* 设置检修状态.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void EP_Set_Repair_Sts();

/* 清除检修状态.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void EP_Clear_Repair_Sts();

/***********************************************************************
* EP_Clear_Alarm - 清除掉告警信号，复归时调用.
*
* RETURNS: None
*
*/
void EP_Clear_Alarm(void);

/***********************************************************************
* EP_Is_Lock_DO - 获得当前是否闭锁保护标志
*
* RETURNS:
*        若已经设置保护闭锁，返回真
*　　　  否则，返回假
*
*/
BOOL EP_Is_Lock_DO(void);

/***********************************************************************
* EP_Is_Enable_Alarm - 获得装置当前是否设置告警标志
*
* RETURNS:
*      已经设置告警，返回真
*　　　否则，返回假
*
*/
BOOL EP_Is_Enable_Alarm(void);

/***********************************************************************
* EP_Set_04CPU_Init_End_Flag - 设置04板初始化完成标志
*
* RETURNS:
*         TRUE, 表示已初始化完成
*         FALSE, 表示初始化还未完成
*
*/
void EP_Set_04CPU_Init_End_Flag(
    BOOL bInitEndFlag			/* 初始化完成标志 */
);

/***********************************************************************
* LightDevAbnormalHintLamp - Set the Equipment Abnormity Lamp
*
* RETURNS: 无
*
*/
void LightDevAbnormalHintLamp();

/***********************************************************************
* MessageBoxRegister - 注册显示函数
*
* RETURNS: 无
*
*/
extern void MessageBoxRegister(void *fun);

/* 保护启动后功能初始化.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void EP_Init_After_Relay(void);

/* 设置逻辑图扫描任务DI更新计数.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void RE_SetLogDIUpdateCnt(void);

/* 显示CPU空闲百分比,由shell调用
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void showIdleStat(void);

/* 得到CPU空闲百分比,由浮点环境任务调用
 * Para:
 *     NONE.
 * Return:
 *     idle percent.
 */
extern float getIdleStat(void);

/* 设置定值更新计数(按任务).
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void RE_SetLogSetChgCnt(void);

/* 设置SPI通信主从标志
 * Para:
 *     1,主CPU;
 *     2,从CPU.
 * Return:
 *     NONE.
 */
extern void EP_Set02CPUPos(uint8_t ucPos);

/*  Function:   run function
    Parameter:  funName;   function name
                pPara, 返回参数.
    Return Value;   OK; ERROR
*/
extern int EP_runFun(char *funName, void *pPara)__attribute__((deprecated));

/* 调整tNetTask任务优先级 */
extern void EP_AdjNetPri(void);

#ifdef	__cplusplus
}
#endif

#endif                                  /* EDPBASE_H */
