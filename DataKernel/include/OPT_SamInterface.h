/* OPT_SamInterface.h - subroutine library for interface between OPT module and sampling module  */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 27mar06, dy realize the interface to sampling module.
01a, 8feb06, zy first created.

*/

/*
DESCRIPTION
This module includes subroutine library for interface between OPT module and sampling module.
*/

#ifndef OPT_SAM_INTERFACE_H
#define OPT_SAM_INTERFACE_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "edpbase.h"
#include "OPT_SamSyn.h"
#include "dsp.h"

/* defines */

#define MAX_OPT_AI_NUM 2   /* 光纵AI的最大数目 */
#define MAX_OPT_AO_NUM 2   /* 光纵AO的最大数目 */
#define OPT_BUF_LENGTH 100

/* typedefs */

typedef struct  /* 光纵机箱AO通道中的AI配置 */
{
    uint8_t ucAOHdCh;        /* 光纵AO的虚拟机箱AO物理通道号，从0开始 */
    uint8_t ucSrcAIHdCh;          /* 若是AI来源,光纵AO的本地机箱源AI对应的物理通道号, 从0开始*/
    float fCoff;	/* 通道系数*/
} OPT_AI_CFG;

typedef struct    /* 光纵模块<-> A/D模块接口 */
{
    OPT_AI_CFG aOptBoxAiCfg_g[MAX_OPT_AI_NUM]; /* AO中AI配置 */
    uint16_t AINum; 					/* AO配置的AI通道数 */
    u_int uiTxPts; 		/* 发送间隔点数 */
    int iOptAoDataByteLen; 			/* 接受缓冲区长度 */
    int16_t *pOptAIResult; 		/* AI配置的AO通道的数据接收缓冲基址指针 */
    float *pfOptAIResult; 					/* AI配置的AO通道的数据接收缓冲基址指针，浮点型 */
    int16_t *pArray[MAX_OPT_AI_NUM];  /* 采样通道指针 */
    int32_t **ppArray;
    int iPos[MAX_OPT_AI_NUM];
    float fCoff[MAX_OPT_AI_NUM];   /* 一次值转数字量 */
    int32_t iCoff[MAX_OPT_AI_NUM];   /* 一次值转数字量,整数除法 */
    int iZeroCurPos[MAX_OPT_AI_NUM];
    UINT32 SamTimebuf[10*MAXSAMPPOINT*2]; 		/* 采样时间存储 */
    uint32_t RegNum; 				/* 定时器定时数据*/
    uint32_t BackupRegNum; 					/* 备份定时器定时数据 */
    int iAdjMode; 				/* 调整模式 */
    int iAdjSamCnt; 	/* 调整计数 */
} OPT_AI_MOD;

typedef struct  		/* 光纵机箱AO通道配置 */
{
    uint8_t ucAOHdCh;     	/* 光纵AO的虚拟机箱AO物理通道号，从0开始 */
    int iAOSrcType;               /* 1为AI来源,2为逻辑图中间结果来源,其他保留  */
    uint8_t ucSrcAIHdCh;       /* 若是AI来源,光纵AO的本地机箱源AI对应的物理通道号,  从0开始 */
    FLT_U32_UNION SrcAIPhyCoffUnion;     /* 若是AI来源,光纵AO的本地机箱源AI对应的物理通道比例系数  2006-11-26日,作成联合，便于传输时访问 */
    void *pElemSrc;      /* 若是中间结果来源,逻辑图中间结果IO指针 */
} OPT_AO_CFG;

typedef struct		/* 光纵机箱的AO配置 */
{
    int iOptAONum;         /*总的光纵AO总数  */
    int iOptAISrcAONum;    /* AI来源的AO总数 */
    int iOptAISrcAONumTmp;    /* 更改AI来源的AO系数时临时使用的计数变量 */
    int iOptMidSrcAONum;    /*逻辑图中间结果的AO总数  */
    OPT_AO_CFG aOptBoxAoCfg_g[MAX_OPT_AO_NUM];
}   OPT_BOX_AO_CFG;

/* globals */

extern OPT_AI_MOD OptAIMod_g;
extern BOOL OPTAD_flag; 				/* 光纵采样允许标志 */
extern BOOL Adjust_flag; 		/* 调整允许标志 */
extern BOOL Send_flag; 						/* 启动发送标志 */
extern int16_t OptAiBuf[OPT_BUF_LENGTH]; 			/* OPT发送缓冲区 */
extern float fOptAiBuf[OPT_BUF_LENGTH]; 		/* OPT发送缓冲区，浮点型 */
extern int16_t tmpOptBuf[2*OPT_BUF_LENGTH]; 				/* 采样整型临时缓冲区 */
extern float ftmpOptBuf[2*OPT_BUF_LENGTH];  /* 采样整型临时缓冲区, 浮点型  */
extern int16_t *ptmpOptBuf;
extern float *pftmpOptBuf;		/* 浮点型 */
extern int TotalNumofTrans;

/* functions */

/* functions */

/* Initailize AO in virtual box, realized by sampling module, called by OPT module.
 * Para:
 *     uiTxPts, points number every time by OPT.
 *     iOptAoCh, number of AO in this virtual box.
 *     pOptAoCfg, pointer to the AO configuration.
 *     iOptAoDataByteLen, the length of buffer for AO from AI.
 *     ppucRtOptAoDataByteBase, the base address of buffer for AO from AI.
 * Return:
 *     EP_SUCCESS, EP_BUF_ERR, EP_COM_ERR.
 */
EP_STATUS Init_OptBox_AO(u_int uiTxPts, int iOptAoCh, OPT_AO_CFG *pOptAoCfg, int iOptAoDataByteLen, uint8_t **ppucRtOptAoDataByteBase);

/* writing the data buffer of OPT AI channel using interger data, called in  ISR.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void WrOptBuf(void);

/* get the sampling counter of sampling module.
 * Para:
 *     NONE.
 * Return:
 *     the current counter.
 */
uint8_t OPT_GetSamClk();

/* get the sampling time based on the sampling counter.
 * Para:
 *     ucSamCnt, sampling counter.
 *     pulRtTimeBaseH, high 32 bits of time.
 *     pulRtTimeBaseL, low 32 bits of time.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS OPT_GetSamTimeBase(uint8_t ucSamCnt, uint32_t *pulRtTimeBaseH, uint32_t *pulRtTimeBaseL);

/* adjust the sampling mode.
 * Para:
 *     iAdjMode, sampling mode, 0: quick mode; 1: slow mode; 2: normal mode; other reserved.
 *     iAdjSamPeriod, period after adjusting, unit is ns.
 *     iAdjSamCnt, times of adjusting.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 *
 * alert: the times will be subtracted by 1 after every adjusting; the adjusting stop when the times equal to 0.
 *        permit to adjust again before the times equal to 0; using the new mode after adjusting again.
 */
EP_STATUS OPT_AdjustSamMode(int iAdjMode, int iAdjSamPeriod, int iAdjSamCnt);

/* get the sampling mode.
 * Para:
 *     piRtLeftAdjCnt, the left adjusting times.
 * Return:
 *     adjusting mode, 0: quick mode; 1: slow mode; 2: normal mode; other reserved.
 */
int OPT_GetAdjSamMode(int *piRtLeftAdjCnt);

/* OPT module inform the sampling mode to send AO.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void OPT_NotifyStartSendAO();

/* send the OPT AO, can be called after function OPT_NotifyStartSendAO be called.
 * Para:
 *     ucSamCnt, the current sampling counter.
 * Return:
 *     EP_SUCCESS, EP_BUF_ERR, or EP_COM_ERR.
 */
EP_STATUS OPT_Send(uint8_t ucSamCnt);

/* writing cycled buffer using interger data.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void WrBuf(void);

/* update the coefficient of OPT AI channel .
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void UpdateOptAcCoff(void);

#ifdef	__cplusplus
}
#endif

#endif                                  /* OPT_SAM_INTERFACE_H */
