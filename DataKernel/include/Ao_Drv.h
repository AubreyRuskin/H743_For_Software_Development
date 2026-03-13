/* Ao_Drv.h - subroutine library for handling the analog to digital convertion */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 29jul06, dy tested.
01a, 27dec05, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling the analog to digital convertion.
*/

#ifndef AO_DRV_H
#define AO_DRV_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "edpbase.h"
#include "dspai.h"
#include "realdata.h"
#include "dsp.h"
#include "hwcfg.h"

/* defines */

#define MAX_DA_AO_NUM 50 		/* DA输出的AO配置最大数目 */
#define DAADDR1 0x80000000
#define DAADDR2 0x90000000
#define DAFPGAADDR 0xe00

#if 0
/* DA输出的AO通道配置 */
typedef struct
{
    uint8_t ucAOHdCh;                         /* DA输出的AO物理通道号，从0开始 */
    float fCoff;                        /* 系数 */
    int iAOSrcType;                          /* 1为AI来源,2为逻辑图中间结果来源,其他保留 */
    uint8_t ucSrcAIHdCh;                      /* 若是AI来源,DA输出的AO的本地机箱源AI对应的物理通道号,  从0开始 */
    void *pElemSrc;                         /* 若是中间结果来源,逻辑图中间结果IO指针 */
} DA_AO_CFG;

/* DA输出的AO配置 */
typedef struct
{
    int iDaAONum;         /* DA输出的总的AO总数 */
    int iDaAISrcAONum;    /* AI来源的AO总数 */
    int iDaMidSrcAONum;    /* 逻辑图中间结果的AO总数 */
    DA_AO_CFG aDaAoCfg_g[MAX_DA_AO_NUM];
} DA_PART_AO_CFG;
#endif

/* globals */

extern int iHwAoChNum_g;
extern RD_HW_AO_CH *phwaoch_g;			/* AO配置  6/8/2006 */
extern DA_PART_AO_CFG PartAoCfgDa_g;  						/* DA输出的AO配置 */
extern int16_t *pArray[MAX_DA_AO_NUM];  			/* 采样通道指针 */

extern  uint8_t  aucZerofloat_g[4];				/* 2006-6-14,浮点0的字符串 */
extern BOOL DAInitializeFinishedFlag;

/***********************************************************************
* DA_Initialize - 初始化DA输出
*
* RETURNS: 无
*
*/
EP_STATUS DA_Initialize(void);

/***********************************************************************
* DA_Out - DA输出
*
* RETURNS: 无
*
*/
void DA_Out(void);

/***********************************************************************
* AO_CfgInitFinish - 初始化AO来源
*
* RETURNS: 无
*
*/
EP_STATUS AO_CfgInitFinish(void);

/***********************************************************************
* DAC1Out - DAC 1 Out
*
* RETURNS: 无
*
*/
void DAC1Out(
    float fDaOutData		/* 输出电压 */
);

/***********************************************************************
* DAC2Out - DAC 2 Out
*
* RETURNS: 无
*
*/
void DAC2Out(
    float fDaOutData		/* 输出电压 */
);

#ifdef  __cplusplus
}
#endif

#endif

