/* rec.h - This file contains program to record realtime data */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01d, 6aug04, zy 注释修改.
01c, 4nov03, hdx Updated to version 1.0.
01b, 29jul03, hdx Verified version 0.1.
01a, 5apr03, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains program to record realtime data.
说明:
	录波的主要操作过程：
	1、首先对原始通道录波量，是每次记录在每个通道实时数据缓冲中，
			对中间结果，是每次记录在db中的pxbuf中
	2、然后隔一段时间，将所有录波通道保存在通道缓冲中的录波数据倒到
			当前录波块中。
	3、然后将已满的录波块中的录波数据写到录波文件中。

	标志的主要操作过程：
	1、对每个任务，每次处理时，若有变位，将所有通道标志量和
			任务相关中间结果标志量记录到  该任务RC_TSK_FLAG的
	       ppfgblk缓冲中，每次标志记录算一个标志块 。
*/

#ifndef REC_H
#define REC_H

/* includes */

#include "edpbase.h"
#include "logic.h"
#include "swcfg.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* defines */

#define MAX_REC_FILE_NUM 128  /* 最大录波文件个数 */

#define REC_IRIGB_PLS_60SEC_FLAG 0x10       /*故障开始时间是正闰秒的60s标志*/
#define REC_IRIGB_AFTER_PLS_0SEC 0x20       /*该时刻是否正闰秒的0秒后的时间*/
#define REC_IRIGB_AFTER_NLS_0SEC 0x40       /*该时刻是否负闰秒的0秒后的时间*/
#define REC_IRIGB_LS_TIME_ADJUST  0x80       /*该时刻时间是否进行了闰秒调整*/


/* typedefs */

typedef struct		/* 标志页配置数组 */
{
    uint8_t aucName[MAX_ID_LEN+1];
    uint8_t aucABRV[4];
    BOOL bIsPub;
    SC_SUB_LGC_ITEM *psublgc;
    int iFlagPos;
    int iFlagNum;
} RC_FLAG_PAGE;

typedef struct		/* 2006-11-24日张云定义 */
{
    /* 方式字标志的位标志 */
    uint8_t aucName[MAX_ID_LEN+1];
    uint8_t aucABRV[4];
} RC_MODWORD_BIT_FLAG_CFG;

typedef struct			/* 标志配置*/
{
    uint8_t aucId[MAX_ID_LEN+1];
    uint8_t aucName[MAX_ID_LEN+1];
    uint8_t aucABRV[4];
    uint8_t ucUnit;
    BOOL bHexword;  	/* ghx061130根据规约2.63添加读取配置，具体应用参考张云 */
    RC_MODWORD_BIT_FLAG_CFG *pModWordBitFlagArr;
} RC_FLAG_CFG;

typedef struct		/* AI录波配置 */
{
    uint8_t aucId[MAX_ID_LEN+1];
    uint8_t aucName[MAX_ID_LEN+1];
    uint8_t ucUnit;
    uint8_t ucPageNum;
    BOOL   bNotNeedPrint;
    BOOL   bNotNeedUpSend;
    BOOL bSrcType;		/* 0: 外部通道来源；1: 中间计算结果来源 */

} RC_AI_CFG;

typedef struct		/* DI录波配置 */
{
    uint8_t aucId[MAX_ID_LEN+1];
    uint8_t aucName[MAX_ID_LEN+1];
    uint8_t ucPageNum;
    BOOL   bNotNeedPrint;
    BOOL   bNotNeedUpSend;
    BOOL bNotDORec;
    BOOL bSrcType;		/* 0: 外部通道来源；1: 中间计算结果来源 */
} RC_DI_CFG;

typedef   struct		/* 录波启动信息结构定义 */
{
    uint16_t   unForwardLuboTime;        /**录波启动的前向录波持续时间长度,单位为毫秒**/
    uint8_t    ucBackwardLuboTimeType;   /**录波启动的后向录波持续时间类型,0为持续
                                           									有限长时间类型,1为持续无限长时间类型***/
    uint32_t   ulBackwardLuboTime;       /**录波启动的后向持续时间长度,单位为毫秒,
                                           								只对录波后向持续有限长时间类型有效***/
    uint16_t   unLuboFreq;                /**录波启动后记录的频率****/
} SCI_LUBO_START_INFO_TYPE;

/* globals */

extern int iFgPgNum_g;                  /* Page number of REC_FLAG. */
extern int iRecFgNum_g;
extern int iRecAiNum_g;
extern int iRecDiNum_g;

/* functions */

/***********************************************************************
* RC_Cfg_Flag - 读取软件配置文件中的标志集配置，并初始化原始通道标志访问信息，中间结果标志在逻辑图初始化
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS RC_Cfg_Flag(
    uint8_t *pucCfg, 		/* 数据指针 */
    uint32_t ulLen		/* 数据长度 */
);

/***********************************************************************
* RC_Cfg_Rec_AI - 读取软件配置文件中的录波AI配置，初始化原始通道录波AI的访问信息，中间结果录波AI在逻辑图中初始化
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS RC_Cfg_Rec_AI(
    uint8_t *pucCfg, 		/* 数据指针 */
    uint32_t ulLen		/* 数据长度 */
);

/***********************************************************************
* RC_Cfg_Rec_DI - 读取软件配置文件中的录波DI配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS RC_Cfg_Rec_DI(
    uint8_t *pucCfg,		/* 数据指针 */
    uint32_t ulLen		/* 数据长度 */
);

/***********************************************************************
* RC_Initialize - This function should be called after logic intialization. 录波模块初始化
*
* RETURNS: EP_SUCCESS, or EP_ERROR
* 注意: 在逻辑图初始化之后，逻辑图运行之前，被调用，此时有些数据结构已申请过了
*
*/
EP_STATUS RC_Initialize(void);

/***********************************************************************
* RC_After_Relay_Init - This function should be called after logic intialization. 录波模块保护启动之后初始化
*
* RETURNS: EP_SUCCESS, or EP_ERROR
* 注意: 在逻辑图初始化之后，逻辑图运行之前，被调用，此时有些数据结构已申请过了
*
*/
EP_STATUS RC_After_Relay_Init(void);

/****************************************录波记录功能的访问函数接口定义********************************/


/***********************************************************************
* SCI_Init_Add_New_Lubo_Signal - 逻辑图中添加1个新的中间结果录波量到录波量集中
*
* RETURNS:
*				   	EP_SUCCESS, 操作成功
*               	EP_BAD_DATA, 找不到同名逻辑标识的录波量
*                EP_NOT_INIT, 找到多于1个的同名逻辑标识的录波量
*              	EP_PARA_ERR, 因录波量数据指针参数和调试配置模块中的录波量配置信息
*                         				 不一致,导致的错误
*             	EP_SYS_ERR, 其他原因导致的错误.
*
*/
EP_STATUS SCI_Init_Add_New_Lubo_Signal(
    uint8_t *strID, 			/* 该录波量的逻辑标识 */
    EP_ELEM_IO *pLuboSignal,				/* 该录波量的数据访问指针,用于录波记录时,访问该录波量 */
    uint32_t ulScanTaskNo			/* 该录波量所在的逻辑图扫描任务号。
                              								 用于录波时区分不同任务的录波量 */
);

/***********************************************************************
* SCI_Set_Lubo_Start_Flag - 设置录波启动标志，用于逻辑图扫描过程中触发录波启动
*
* RETURNS: 无
*
*/
void SCI_Set_Lubo_Start_Flag(
    SCI_LUBO_START_INFO_TYPE  *pLuboStartInfo,			/* 启动录波的信息 */
    uint32_t ulScnAiCnt		/* 进行本次逻辑图扫描时的AI采样计数器值 */
);

/***********************************************************************
* SCI_Set_Lubo_Stop_Flag -  设置录波停止标志, 用于逻辑图扫描过程中触发录波停止
*
* RETURNS: 无
*
*/
void SCI_Set_Lubo_Stop_Flag(
    uint32_t ulAiCnt			/* 停止录波时的AI采样计数器值 */
);

/***********************************************************************
* RC_End_Wave -  若所有故障都返回了，停止整个故障报告，供故障停止函数VI_End_Fault调用
*
* RETURNS: 无
*
*/
void RC_End_Wave(
    uint32_t ulAiCnt
);

/***********************************************************************
* SCI_Process_Cur_Logrp_Period_Lubo - 处理本扫描任务的本次逻辑图扫描周期的录波
*
* RETURNS: 无
*
* Alert: 实现时，若上次录波尚未停止,或此次触发新的录波,则进行本任务相关的录波,否则此次不录波
*
*
*/
void SCI_Process_Cur_Logrp_Period_Lubo(
    uint32_t  ulScanTaskNo,		/* 进行本次扫描录波的任务号 */
    uint32_t ulScnAiCnt			/* 进行本次逻辑图扫描时的AI采样计数器值 */
);

/****************************************标志集记录的访问函数接口定义********************************/


/***********************************************************************
* SCI_Init_Add_New_Flag_Signal - 逻辑图中添加1个新的中间结果标志量到标志量集中
*
* RETURNS:
*               EP_SUCCESS, 操作成功
*               EP_BAD_DATA, 找不到同名逻辑标识的标志量
*               EP_NOT_INIT, 找到多于1个的同名逻辑标识的标志量
*               EP_PARA_ERR, 因标志量数据指针参数和调试配置模块中的标志量配置信息
*                               		 不一致,导致的错误
*               EP_SYS_ERR, 其他原因导致的错误.
*
*/
EP_STATUS SCI_Init_Add_New_Flag_Signal(
    uint8_t *strID,			/* 该标志量的逻辑标识 */
    EP_ELEM_IO *pFlagSignal,			/* 该标志量的数据访问指针,用于标志记录时,访问该标志量 */
    uint32_t ulScanTaskNo			/* 该标志量所在的逻辑图扫描任务号。
                              								 用于标志集记录时区分不同任务的标志量 */
);

/***********************************************************************
* SCI_Process_Cur_Logrp_Period_Flagset_Record - 处理本扫描任务的本次逻辑图扫描周期的标志集记录,该函数在每次逻辑图扫描任务的
        																			  最后进行调用。
*
* RETURNS: 无
*
* Alert:
*        实现时，若当前处于事故报告期间,且在本任务本次扫描时若有逻辑量标志发生变位，则
*        进行本任务相关的标志集记录。
*
*/
void SCI_Process_Cur_Logrp_Period_Flagset_Record(
    uint32_t ulScanTaskNo,			/* 进行本次标志记录的扫描任务号 */
    uint32_t ulScnAiCnt			/* 进行本次逻辑图扫描时的AI采样计数器值 */
);

/***********************************************************************
* RC_Get_Flag_Pg_Attr - Get REC_FLAG page attribution.
*
* RETURNS:
*				  Pointer to the flag attribution structure.
*              NULL if iIdx is invalid(>=iRecFgNum_g).
*
*/
const RC_FLAG_PAGE *RC_Get_Flag_Pg_Attr(
    int iIdx		/* flag page number(from 0). */
);

/***********************************************************************
* RC_Get_Flag_Attr - Get REC_FLAG attribution.
*
* RETURNS:
*				  Pointer to the flag attribution structure.
*              NULL if iIdx is invalid(>=iRecFgNum_g).
*
*/
const RC_FLAG_CFG *RC_Get_Flag_Attr(
    int iIdx		/* index of the flag(from 0). */
);

/***********************************************************************
* RC_Get_Rec_AI_Attr - Get record AI attribution.
*
* RETURNS:
*				  Pointer to the REC_AI attribution structure.
*              NULL if iIdx is invalid(>=iRecAiNum_g).
*
*/
const RC_AI_CFG *RC_Get_Rec_AI_Attr(
    int iIdx		/* index of the REC_AI(from 0). */
);

/***********************************************************************
* RC_Get_Rec_DI_Attr - Get record DI attribution.
*
* RETURNS:
*				  Pointer to the REC_DI attribution structure.
*              NULL if iIdx is invalid(>=iRecDiNum_g).
*
*/
const RC_DI_CFG *RC_Get_Rec_DI_Attr(
    int iIdx		/* index of the REC_DI(from 0). */
);


/***********************************************************************
* RC_Start_Lubo_Sample - 启动即时录波采样信息
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS RC_Start_Lubo_Sample(
    char *strRtLuboFileName,		/* 供返回的录波报告文件名字符串，参数字符串空间由调用方分配，本函数填充返回值。 */
    uint8_t *pucRtLuboSN		/* 供返回的录波段号，参数由调用方分配，本函数填充返回值。 */
);

/***********************************************************************
* creatInDataDisk - 在DATA盘中创建文件,是对标准CREAT函数的重载,首先进行磁盘空间检测
*
* RETURNS: OK, or ERROR
*
*/
int creatInDataDisk
(
    const char *name, 		/* name of the file to create */
    int flag  		/* O_RDONLY, O_WRONLY, or O_RDWR */
);

/***********************************************************************
* writeInDataDisk - 在DATA盘中写文件,是对标准write函数的重载,首先进行磁盘空间检测
*
* RETURNS: OK, or ERROR
*
*/
int writeInDataDisk
(
    int    fd,     /* file descriptor on which to write */
    char * buffer, /* buffer containing bytes to be written */
    size_t nbytes  /* number of bytes to write */
);

/***********************************************************************
* GetRecWrSts - 获取录波状态
*
* RETURNS:
*                TRUE，正在录波
*                FALSE，录波结束
*
*/
BOOL GetRecWrSts(void);

/***********************************************************************
*  - 获取事件形成状态
*
* RETURNS:
*                TRUE，正在形成
*                FALSE，结束
*
*/
BOOL GetEvtWrSts(void);

/* 判断是否允许分配录波块.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL RC_Jg_Allot_Blk(void);

#ifdef  __cplusplus
}
#endif

#endif                                  /* REC_H */
