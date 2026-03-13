/* spiio.h - This file contains the driver program for SPI and IO Module */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 24feb09, dy change the code style.
01a, 26jul02, yqf first created.
*/

/*
DESCRIPTION
This file contains the driver program for SPI and IO Module.
*/

/* includes */

#ifndef SPIIO_H
#define SPIIO_H

#include "semLib.h"
#ifdef	__cplusplus
extern "C" {
#endif


/* includes */

#include "edpbase.h"
#include "realdatadef.h"    /* 模件类型 */
//#include "semLib.h"

#include "semaphore_compat.h"
#include "sys_ipc_compat.h"
#include "sys_sem_compat.h"

#include "datetime.h"

/* defines */

#define MAXCMDNUM 20

/* 帧属性字字节数定义 */
#define SPI_BYTES(n) (((n) & 0x3)<<6)

/* IO driver defines */

#define IO_INIT_RETRY 5
#define DO_PASSWORD 0xA5EB90D7   /* password for DO module. */
#define DO_CLR_CH 31		/* 复位通道 */
#define DO_COM_CLR_CH 63   /* 新的规约把第64位作为复位通道，下一步修改 */

#define IO_TYPE_REG 0x02
#define IO_DESN_REG 0x03
#define IO_EDT_REG_LO 0x04
#define IO_EDT_REG_HI 0x05
#define IO_CPU_TYPE_SET 0x06		/* control register for channel reading. */
#define IO_CHN_NUM 0x07   /* channel number. */
#define IO_UF_CTRL_REG 0x08
#define IO_UF_ATTR_REG 0x09
#define IO_STATUS_REG 0x11
#define DO_INVALID_STATUS_REG 0x12

#define DO_CH_NUM_REG 0x10
#define DO_COM_STATUS_REG 0x10   /* fault information. */
#define DO_COM_OUTPUT_REG 0x16        /* output. */
#define DO_OUTPUT_REG 0x18

#define DI_CH_NUM_REG 0x10
#define DI_INPUT_REG 0x18

#define DO_SYS_EVENT 0x0E	 /* 开出板系统事件 */
#define DIO_INPUT_REG 0x0F	   		/* 开入开出模件的输入寄存器 */
#define DI_INPUT_GROUP1_BASE_REG 0x1A		/* 通用板第一组开入基寄存器地址 */
#define DI_INPUT_GROUP2_BASE_REG 0x1E   /* 通用板第二组开入基寄存器地址 */
#define CKDOFEEDBAKC1 0x1C	    /* 测控开出返回寄存器地址 */
#define CKDOFEEDBAKC2 0x1D
#define CKDOFEEDBAKC3 0x1E

#define IO_ADJ_CTRL 0x26			/* adjusting control word. */
#define IO_CHN_ADJ_CTRL 0x27	/* adjusting channel control word. */
#define IO_CHN_RD_CTRL 0x28				/* reading control word. */
#define IO_AI_CHN_SN 0x29	/* channel number. */

#define COMMANDSNDMAXDEALYCNT (2.8*RD_SIO_RATE)		/* 等待命令结束，SPI通讯最大计数 */
#define CMD_EXCUTE_ACK_NUM 10   /* 确认命令执行次数 */

#define GROUP_NUM_COM_DI 2		/* 通用板开入组数 */
#define SPI_FRAME_LEN_PER_MOD 6		/* 单个模件帧长度 */

/* DO error status. */
#define DO_INVALID 0x01
#define DO_BREAK 0x02
#define DO_POWER_DOWN 0x04		/* not use. */
#define DO_RESET 0x08
#define DO_QD_INVALID 0x10
#define DO_SPI_COM_ERROR 0x20		/* down frame error, not used. */

/* typedefs */

typedef struct	/* SPI-IO submodule hardware information */
{
    EP_STATUS stsMod;    /* module mode word */
    SUB_MOD_TYPE type;         /* module type */
    uint8_t ucHardAddr;    /* module hardware address */
    uint8_t ucDesignSN;       /* module designed serial number */
    uint16_t unVer;    /* module version number */
    uint16_t unDiChNum;        /* module digit input number */
    uint16_t unDoChNum;    /* module digit output number */
    uint16_t unAiChNum;          /* module analog input number */
    uint16_t unAoChNum;      /* module analog output number */
} SUB_MOD_INFO;

typedef enum	/* 发送到IO板命令类型 */
{
    IO_GET_FAULT_CMD = 0x01,  /* 0: 获取故障信息命令 */
    IO_GET_DI_ONDO_CMD = 0x02,			/* 1: 获取开出板上的开入命令 */
    IO_GET_DI_CKDO_CMD = 0x04, 		/* 2: 获取测控板上的开入 */
    IO_COM_ADJ_CMD = 0x08, 	/* 3: 通道校准命令 */
    IO_SET_DO_CMD = 0x10, 		/* 4: 设置DO模件属性 */
    IO_SET_CKDIO_CMD = 0x20, 	/* 5: 设置CKDO模件属性 */
    IO_SET_DI_CMD = 0x40, 	/* 6: 设置DI模件属性 */
}  IO_CMD_TYPE;

typedef struct
{
    uint64_t ulLastVal;  /* 最多64个开入 */
    uint64_t ulFltFg;
} DI_MOD_PARM;

typedef struct	/* Output module parameter struct. */
{
    uint32_t ulSts;   /* 开出状态 */
} DO_MOD_PARM;

typedef struct  /* AI input module parameter struct. */
{
    int16_t *pPos;		/* source address. */
    BOOL overflag;
    BOOL errflag;
} AI_CHANNEL;

typedef struct
{
    BOOL bSts;
    uint32_t ulChgTime;
    uint32_t ulFiltTime;	/* 消抖时间，us */
    uint32_t ulFltCfg;
    uint32_t ulFltCnt;
    uint32_t ulFltTmp; /* 为处理首次读取开入状态添加 */
    uint32_t ulTmpTime;
    US_CNT_UTC_TIME utTmpTime;
    US_CNT_UTC_TIME utChgTime;
#ifdef EDP02_PSR_BUILD
    uint32_t ulChgTimeAfterFilt;		/* 变位，滤波后的时间 */
#endif
    void *pIoBuf;  /* 指向所属I/O模件 */
    uint32_t ulInvalidDftVal;  /* 缺省状态 */
} DI_CHANNEL;

typedef struct
{
    int iModAddr;
    int iChIdx;
    uint32_t ulPassWd;
} DO_CHANNEL;

typedef struct			/* 校准信息 */
{
    uint8_t ucAdjCtrl;
    uint8_t ucChnStrl;
    uint32_t uccmdType;
} COM_ADJ_ATTR;

typedef struct tag_SPI_IO_BUF		/* SPI <-> IO 信息交换缓冲结构 */
{
    SEM_ID semMod;       /* 各模件SPI数据区访问互斥信号灯 */
    uint8_t ucModType;          /* 模件类型 */
    uint8_t aucDownFrame[6];  /* 下行帧数据 */
    uint8_t aucUpFrame[6];        /* 上行帧数据 */
    BOOL bComErr;        /* 通信状态 */
    uint8_t ucModSts;          /* 模件状态 */
    uint8_t ucModAddr;    /* 模件地址 */
    uint8_t ucDesignSN;         /* Module designed serial number. */
    uint16_t unVer;    /* Module version number. */
    uint16_t unDiChNum;      /* Module digit input number. */
    uint16_t unDoChNum;          /* Module digit output number. */
    uint16_t unAiChNum;   /* Module analog input number. */
    uint16_t unAoChNum;        /* Module analog output number. */
    BOOL bUsed;		/* 是否被使用 */
    BOOL bMatched;
    BOOL bIORebootFlag;		/* 是否重启标志 */
    uint32_t ulCheckSumErrCnt;		/* 校验出错记录，达到门槛后置0 */
    uint32_t ulExcCnt;			/* 单位时间内异常计数 */
    uint32_t ulTotalExcCnt;         /* 模件总异常数 */
    uint32_t ulRebootCnt;	/* 重启计数 */
    BOOL bRebootFlag;
    BOOL bErrOvFlag;       /* 单位时间内CRC校验出错标志 */
    uint32_t ulErrOccurCnt;          /* 单位时间内校验出错次数 */
    uint32_t ulCheckSumErrStat;				/* 出错统计用 */
    BOOL bDIDOExecFlag;		/* DIDO检测异常标志 */
    uint32_t ulLastDIDOExecTime;	/* 上次检测DIDO异常发生的us数时间 */

    BOOL bCycleCommandSndFlag;		/* 循环命令发送，每次只能发送一个命令 */

    BOOL bInvalid;		/* 失效 */
    BOOL bBreakdown;		/* 击穿 */
    BOOL bReset;	/* 重启 */
    BOOL bSpiComError;			/* SPI通讯错 */
    BOOL bQDInvalid;		/* 启动失效 */
    uint8_t aucUpErrFrame[6];        /* 上行帧数据 */

    void (*pfDealRecv)(struct tag_SPI_IO_BUF*);		/* 上行帧处理程序入口地址 */

    struct	/* 支持DIO模件，DI部分 */
    {
        DI_MOD_PARM dimod;
        DO_MOD_PARM domod;
    } modinfo;

    struct	/* 支持DIO模件，DO部分 */
    {
        AI_CHANNEL aich[MAX_VTBOX_AI_NUM];  /* AI handle. */
        DI_CHANNEL dich[MAX_DI_PER_MOD];
        DO_CHANNEL doch[MAX_DO_PER_MOD];
    } chinfo;

    int16_t aAiBuf[MAX_VTBOX_AI_NUM];	 /* AI buffer. */
    uint16_t aOvValBuf[MAX_VTBOX_AI_NUM];		/* AI over flow value. */
    uint8_t ucDiGroupNum;	/* DI group counter. */
    uint8_t ucAiChnCnt;				/* AI channel counter. */

    BOOL bGetDiFlag;	/* flag for reading DI. */
    BOOL bGetAiFlag;			/* flag for reading AI. */

    COM_ADJ_ATTR adjattr;				/* adjusting attribution. */
    uint32_t ulCmdSndCnt[MAXCMDNUM];
    uint32_t ulIOConfirmCnt[MAXCMDNUM];   /*IO板确认次数数组，与MAX_IOCONFIRM_CNT比较*/
    BOOL bCmdExecuteResult[MAXCMDNUM];
    uint32_t ulCmdSts;		/* command status. */
    BOOL bChgBaseReg;
    BOOL bSetDoFlag;
    uint32_t ulSetDoCnt;
    BOOL bSetCmdFlag;    /* 上次发送命令标志 */
    int32_t iMbDiSrcType;	/* 母板开入来源类型 */
    BOOL bCRCCheckMod;  /* 校验模式 */
    BOOL bCheckModAffirm;  /* 校验模式是否确定 */
    uint32_t ulSumCheckCnt;  /* 求和取反校验计数 */
    uint32_t ulCRCCheckCnt;     /* CRC校验计数 */
    uint32_t ulOverThresholdCnt;   /* 连续越门槛计数 */
    BOOL bDefaultFlag;   /* 使用缺省值标志 */
    uint32_t ulSwitchDefaultCnt;   /* 切换次数 */
    uint32_t ulSwitchDefaultTm;  /* 最近切换时间 */
    US_CNT_UTC_TIME utSwitchDefaultChgTime;  /* 缺省UTC时间 */
    BOOL bErrFrmLogFlag;  /* 异常报文日志记录标识 */
    uint32_t ulSPIDIRcvTimes;  /* 开入首次处理逻辑计数处理 */
    BOOL bIOCheckMod;  /* IO模式设置 */
    uint32_t ulUpChkErr;  /* 下行帧校验出错 */
} SPI_IO_BUF;

typedef struct tag_SPI_COM_INFO		/* SPI通讯相关信息 */
{
    BOOL bIOInitFinish;		/* IO模件初始化完成标志 */
    uint32_t ulIsrCnt;		/* 中断次数 */
    uint32_t ulFrmCnt;				/* 接收帧次数 */
    uint32_t ulEnterISRCnt;		/* SPI中断次数 */
    uint32_t ulSPIRxErrCheckFreq;  /* 检查周期 */
    uint32_t ulSPIRxErrAlmLevel;       /* 告警门槛 */
    uint32_t ulSPIRxErrLogLevel;  /* 日志门槛 */
    uint32_t ulSPIRxErrRetLevel;      /* 恢复门槛 */
    uint32_t ulMaxErrCnt;   /* 异常接收消抖次数 */
    uint32_t ulCommandDelayCnt;  /* 校准最大等待时间 */
    uint32_t ulSPIDelayCnt;  /* Ticks to wait for SPI communication. */
    uint32_t ulOverThreshold;  /* 连续出错门槛 */
} SPI_COM_INFO;


/* globals */

extern uint32_t ulSpiComDelay;			/* SPI上传延时，us */
#ifndef EDP02_PSR_BUILD
extern SPI_IO_BUF aspibuf_g[MAX_MOD_NUM];		/* SPI<->IO 模件数据交换缓冲 */
#endif
extern SPI_COM_INFO spiinfo;
extern BOOL bNormalSndFlag;  /* 任务中发送数据帧标志 */

/* functions */

/* intialize the whole SPI-IO driver module.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, EP_BUF_ERR, or EP_COM_ERR.
 */
EP_STATUS SIO_Initialize(void);

/* get the status of IO module.
 * Para:
 *     iAddr, address of module.
 *     pmodinfo, status infomation.
 * Return:
 *     NONE.
 */
void SIO_Mod_Info(int iAddr, SUB_MOD_INFO *pmodinfo);

/* initialize the AI channel.
 * Para:
 *     iModAddr, address of module.
 *     uiCh, channel number in this module, begin from 0.
 *     pfRate, rate coefficient.
 * Return:
 *     pointer to this AI channel, NULL if error occur.
 */
void *SIO_Init_AI(int iModAddr, u_int uiCh, float *pfRate);

/* initialize the AO channel.
 * Para:
 *     iModAddr, address of module.
 *     uiCh, channel number in this module, begin from 0.
 * Return:
 *     pointer to this AI channel, NULL if error occur.
 */
void *SIO_Init_AO(int iModAddr, u_int uiCh);

/* initialize the DO channel.
 * Para:
 *     iModAddr, address of module.
 *     uiCh, channel number in this module, begin from 0.
 * Return:
 *     pointer to this DO channel, NULL if error occur.
 */
void *SIO_Init_DO(int iModAddr, u_int uiCh);

/* initialize the DI channel.
 * Para:
 *     iModAddr, address of module.
 *     uiCh, channel number in this module, begin from 0.
 *     ulFilt, filting time, unit is us.
 *     bInvalidDftVal, default value type.
 * Return:
 *     pointer to this DI channel, NULL if error occur.
 */
void *SIO_Init_DI(int iModAddr, u_int uiCh, uint32_t ulFilt, BOOL  bInvalidDftVal);

/* read the realtime AI status.
 * Para:
 *     pvAiCh, pointer to this AI channel.
 * Return:
 *     result.
 */
int16_t SIO_Get_AI(void *pvAiCh);

/* read the realtime DI status.
 * Para:
 *     pvDoCh, pointer to this DI channel.
 *     pulChgTime, the time chage to this status(us).
 *     putChgtm, the time chage to this status(US_CNT_UTC_TIME).
 * Return:
 *     TRUE=close; FALSE=open.
 */
BOOL SIO_Get_DI(void *pvDiCh, uint32_t *pulChgTime, US_CNT_UTC_TIME *putChgtm);

/* set the realtime DO output.
 * Para:
 *     pvDoCh, pointer to this DO channel.
 *     bClose, state, TRUE=close; FALSE=open.
 * Return:
 *     NONE.
 */
void SIO_Set_DO(void *pvDoCh, BOOL bClose);

/* enable the start-up DO.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SIO_Enable_DO(void);

/* disable the start-up DO.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SIO_Disable_DO(void);

/* alarm out.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SIO_Enable_Alm(void);

/* stop alarm out.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SIO_Disable_Alm(void);

/* revert the self-keep DO(signal board).
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SIO_Clr_DO_Keep(void);

/* record the DO buffer.
 * Para:
 *     pucRecBuf, buffer address.
 *     BufSize, size of buffer.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS SIO_Rec_Do_Buf(uint8_t *pucRecBuf, int BufSize);

/* SPI send data actively, called by the fask scanning task.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SPI_NEW_COM(int nScanInterval);

/***********************************************************************
* UpdateMbDiSts - 更新母板开入状态
*
* RETURNS: 无
*
*/
void UpdateMbDiSts(void);

/* 获取开入变位时间.
 * Para:
 *     pSrc, 句柄.
 * Return:
 *     变位时间，单位为us.
 */
uint32_t SIO_GetDiChgTime(void *pSrc);

/* 获取开入变位UTC时间.
 * Para:
 *     pSrc, 句柄.
 * Return:
 *     uint64_t, 从1970记录的微妙计数.
 */
uint64_t SIO_GetDiChgUTCTime(void *pSrc);

/* get the start-up signal on mother board.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, start-up; FALSE, no start-up.
 */
extern BOOL SIO_Is_Open_QD();

/* Judge if the IO module exist.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL SIO_GetIOExsitSts(int iModAddr);

/* write the down frame.
 * Para:
 *     iModAddr, address of the module.
 *     pucData, address of data.
 * Return:
 *     NONE.
 */
void SPI_Write(int iModAddr, uint8_t *pucData);

/* SPI data receiving.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void SPI_Recv(void);

#ifdef	__cplusplus
}
#endif

#endif                                  /* SPIIO_H */
