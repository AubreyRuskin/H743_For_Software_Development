/* dspai.h - This file contains interface to AI channel in DSP system */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02b, 29jul06, dy add the attribute ucUnit in structure DSP_LGC_AI_CFG.
02a, 27dec05, dy add the structure DSP_MSU_AI_CFG for measuring.
01a, 26jul02, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains interface to AI channel in DSP system.
*/

#ifndef DSPAI_H
#define DSPAI_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "edpbase.h"
#include "logmsg.h"

#include <stdio_compat.h>
#include "string_compat.h"
#include "ctype_compat.h"


#include "sys_ioctl_compat.h"
#include "fcntl_compat.h"
#include "unistd_compat.h"
#include <semLib.h>



/* defines */

#ifdef VIRT_BOX
#define MAX_VT_BOX_COUNT 32					/* 虚拟机箱最大个数，包含所有配置的虚拟机箱，分布式母差时配得最多 */
#else
#define MAX_VT_BOX_COUNT 0
#endif

#define MAX_DIST_VTBOX_NUM 32			/* 最大分布式母差机箱个数 */

/* typedefs */

typedef struct			/* 逻辑通道配置 */
{
    uint8_t ucHdCh;              /* 物理通道号 */
    float fCoff;                        			/* 系数 */
    uint8_t ucFiltNum;                 					 /* 滤波算法号，按照规约中的定义 */
    uint8_t ucUnit;						/* 单位类型 */
} DSP_LGC_AI_CFG;

typedef struct	 /* DC logic channel. */
{
    uint8_t ucHwAddr;   /* address. */
    uint8_t ucHdCh;   /* channel number. */
    float fCoff;              /* coefficient. */
    void *pSrc;		    /* pointer to driver channel handle. */
    float *pData;                   /* pointer to box data buffer. */
    void *pCfg;
    float fRate;		/* get from IO module. */
} DSP_LGC_DC_AI_CFG;

typedef struct					/* 预处理通道配置，一个配置可以对应多个连续通道 */
{
    uint8_t ucBgnLgcCh;            /* 起始逻辑通道号 */
    uint8_t ucChNum;                    			/* 该预处理算法对应的逻辑通道号数 */
    uint8_t ucArithNum;                 	/* 预处理算法号，按照规约中的定义 */
    uint8_t ucArithParm;                						/* 预处理算法参数，按照规约中的定义，包括上传与否的定义 */
} DSP_CALC_AI_CFG;

typedef struct		/* 测量通道配置，一个配置可以对应多个连续通道 */
{
    uint8_t ucBgnLgcCh;           /* 起始逻辑通道号 */
    uint8_t ucChNum;                    						/* 该预处理算法对应的逻辑通道号数 */
    uint8_t ucArithNum;                 /* 预处理算法号，按照规约中的定义 */
    uint8_t ucArithParm;                				/* 预处理算法参数，按照规约中的定义，包括上传与否的定义 */
} DSP_MSU_AI_CFG;

typedef struct			/* 实时数据缓冲区 */
{
    u_int uiTotalCh; 			/* 总的通道数 */
    float *pfBufBgn; 					/* 起始地址 */
    float *pfBufEnd; 				/* 结束地址 */
    uint32_t ulBufLen;
    u_int uiChBytes;
    uint32_t ulBufBytes;
    float *pfCopy;
} RD_LGC_AI_DB;

typedef struct			/* 实时状态标志缓冲区 */
{
    uint32_t *pBufBgn;  /* 起始地址 */
    uint32_t *pBufEnd; 	  /* 结束地址 */
} RD_LGC_AI_STS_DB;

typedef struct		/* 测量用数据缓冲区 */
{
    u_int uiTotalCh; 		/* 总的测量通道数 */
    COMPLEX *pxBufBgn;  		/* 开始地址 */
    uint32_t ulBufLen; 		/* 缓冲区长度 */
    u_int uiChBytes; 					/* 接受字节数 */
    uint32_t ulBufBytes; 						/* 应该是接受字节总数 */
} RD_MSU_AI_DB;

typedef struct			/* 预处理数据缓冲区 */
{
    u_int uiTotalCh;
    COMPLEX *pxBufBgn;
    COMPLEX *pxBufEnd;
    uint32_t ulBufLen;
    u_int uiChBytes;
    uint32_t ulBufBytes;
    COMPLEX *pxCopy;
} RD_CALC_AI_DB;

typedef struct		/* 开入缓冲区 */
{
    u_int uiTotalCh;
    BOOL *pbBufBgn;
    BOOL *pbBufEnd;
    uint32_t ulBufLen;
    BOOL *pbWork;
} RD_DI_DB;

typedef struct		/* AI数据有效缓冲区 */
{
    u_int uiTotalCh; 		/* 所有通道 */
    BOOL *pbBufBgn; 				/* 开始 */
    BOOL *pbBufEnd; 		/* 结尾 */
    uint32_t ulBufLen; 				/* 应该是单个缓冲区的长度 */
} RD_AI_VALID_DB;

/* globals declarations */

extern DSP_LGC_AI_CFG *pdspl_cfg_g;
extern DSP_LGC_DC_AI_CFG *pdspl_dc_cfg_g;			/* DC processing configuration. */
extern DSP_CALC_AI_CFG *pdspc_cfg_g;
extern DSP_MSU_AI_CFG *pdspm_cfg_g;		/* Measurement variable */
extern DSP_LGC_AI_CFG *pdspl_cfg_ext_g;
extern DSP_CALC_AI_CFG *pdspc_cfg_ext_g;
extern DSP_LGC_AI_CFG *pdspl_cfg_redun_g;
extern DSP_CALC_AI_CFG *pdspc_cfg_redun_g;
extern DSP_LGC_AI_CFG *pdspl_cfg_Opt_g[2];
extern DSP_CALC_AI_CFG *pdspc_cfg_Opt_g[2];

extern DSP_LGC_AI_CFG *pdspl_cfg_Pole_g;
extern DSP_CALC_AI_CFG *pdspc_cfg_Pole_g;

extern DSP_LGC_AI_CFG *pdspl_cfg_Virt_g[MAX_VT_BOX_COUNT]; 	/* 虚拟机箱，原始通道 */
extern DSP_CALC_AI_CFG *pdspc_cfg_Virt_g[MAX_VT_BOX_COUNT];			/* 预处理通道 */

extern SEM_ID semHwAiMea;
extern SEM_ID semPoMea;


extern RD_LGC_AI_DB lgcaidb_g; 		/* 逻辑通道数据缓冲 */
extern RD_CALC_AI_DB calcaidb_g; 				/* 预处理通道数据缓冲 */
extern RD_MSU_AI_DB msucaidb_g;
extern RD_DI_DB didb_g;
extern RD_AI_VALID_DB aivaliddb_g;   /* 张云添加 */

/***********************************************************************
* RD_Init_AI_Db - 初始化AI数据缓冲区
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
EP_STATUS RD_Init_AI_Db(void);

/***********************************************************************
* RD_Init_DI_Db - 初始化DI数据缓冲区
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
EP_STATUS RD_Init_DI_Db(void);

/***********************************************************************
* RD_Init_DO_Db - 初始化DO数据缓冲区
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
EP_STATUS RD_Init_DO_Db(void);

/***********************************************************************
* RD_Boot_Dsp - 启动DSP计算
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*			EP_COM_ERR, DSP通信出错
*
*/
EP_STATUS RD_Boot_Dsp(void);

/***********************************************************************
* DSP_Initialize - 初始化整个DSP-AI驱动模块
*
* RETURNS:
*		EP_SUCCESS，正常返回
* 		EP_BUF_ERR，内存错误
*		EP_COM_ERR，DSP通信出错
*
*/
EP_STATUS DSP_Initialize(
    u_int uiSmplRate, 	/* 每周波采样点数 */
    uint16_t uiProRate, 		/* DSP计算使用点数 */
    u_int Sysfrequency, 			/* 系统频率 */
    u_int uiTxPts, 		/* 每次传送采样点数 */
    void *pvAiMod,				/* 该模块（DSP负责的所有AI采集/计算通道）的句柄 */
    u_int uiLgcCh, 		/* 采样的逻辑通道数 */
    DSP_LGC_AI_CFG *plgccfg,			/* 指向逻辑通道配置数组第0个元素的指针，数组元素有uiLgcCh个 */
    u_int uiCalcCh, 		/* 预处理通道配置数 */
    DSP_CALC_AI_CFG *pcalccfg,		/* 指向预处理通道配置数组第0个元素的指针，数组元素uiCalcCfg个 */
    u_int uiMsucCh, 		/* 测量通道配置数 */
    DSP_MSU_AI_CFG *pmsuccfg,		/* 指向测量通道配置数组第0个元素的指针 */
    u_int Msu_Base_Num,		/* 测量基准通道 */
    u_int uiLgcDcCh,    /* 直流通道数 */
    DSP_LGC_DC_AI_CFG *plgdccfg				/* 直流通道配置 */
);

// EP_STATUS DSP_Initialize(u_int uiSmplRate, u_int Sysfrequency, u_int uiTxPts, void *pvAiMod,
//                          u_int uiLgcCh, DSP_LGC_AI_CFG *plgccfg,
//                          u_int uiCalcCh, DSP_CALC_AI_CFG *pcalccfg);

/***********************************************************************
* Init_Redun_Finish - 初始化冗余机箱，用于励磁平台
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS Init_Redun_Finish(void);

/***********************************************************************
* Init_OptComm_Finish - 初始化光纤通讯
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS Init_OptComm_Finish(void);

/***********************************************************************
* RD_Init_AO_Db - 初始化AO库，目前只允许光纵，此时只初始化AI来源的AO
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
EP_STATUS RD_Init_AO_Db(void);

/***********************************************************************
* InitOptBoxChn2 - 初始化光纵机箱通道2
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 错误
*
*/
EP_STATUS InitOptBoxChn2(void);

/***********************************************************************
* RD_Init_AO_Coff - 初始化AI来源AO的系数
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
EP_STATUS RD_Chg_AO_Coff(void);

/* 初始化时更新计算用系数
 * Para:
 *     NONE.
 * Return:
 *     NONE.
*/
void RDUpdateDspCfg(void);

/***********************************************************************
* CmpExtAcCoff - 比较扩展机箱交流采样系数
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS CmpExtAcCoff(
    uint8_t *puc
);

/* 运行时更新扩展机箱计算用通道系数
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
EP_STATUS DspExtBoxCoeRunUpdate(void);

/* 初始化GOOSE开出通道压板.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or EP_FALSE.
 */
extern BOOL RD_InitLinkofGsDo(void);

/* 禁止光差报文接收.
 * Para:
 *     OptCh, 通道序号.
 * Return:
 *     EP_SUCCESS, or NP_ERROR.
 */
extern EP_STATUS OPT_DisableRecv(int32_t OptCh);

#ifdef	__cplusplus
}
#endif

#endif               /* DSPAI_H */
