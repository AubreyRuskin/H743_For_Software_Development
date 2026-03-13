/* msu.c - subroutine library for handling the algorithms of signal measurement */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 29jul06, dy decrease the memory used.
01a, 27dec05, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling the algorithms of signal measurement.
*/

#ifndef MSU_H
#define MSU_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "edpbase.h"
#include "dspai.h"
#include "realdata.h"
#include "measure.h"
#include "adc.h"

/* defines */

#define AINUM 14							/* 测量通道数 */
#define MAXWAVENUM 5 								/* 测量最高谐波次数 */
#define MSU_AVER_BUF_NUM MAXSAMPPOINT	/* 平均计算缓冲区长度 */
#define MAXSAMPCYCLE (int)((1.12*MAXSAMPPOINT))								/* 一周波最多采样点 */
#define GainNUM 100    				/* 校准点数 */
#define LOWFREQ 45
#define HIGHFREQ 65
#define SIMPSIMINTGRL    			/* 辛卜森积分 */

/* typedefs */

typedef struct MSUINFO_tag			/* 测量有关信息 */
{
    uint8_t Msu_Base_Num;
    float RealData[AINUM];				/* 计算的一个个数据 */
    int Pos;							/* 频率定位 */
    int PosBack;			/* 频率定位备份 */
    float fCosCoe[MAXWAVENUM][MAXSAMPCYCLE];				/* 测量用系数*/
    float fSinCoe[MAXWAVENUM][MAXSAMPCYCLE];
    float freqreal[AINUM];							/* 瞬时计算频率*/
    float freqavr[AINUM];					/* 平滑后频率 */
    float freq;						/* 当前频率*/
    float SumFreq[AINUM]; 							/* 频率平滑滤波值*/
    float altitude[AINUM][MAXWAVENUM][2];
    float RealImage[AINUM][MAXWAVENUM][2];
    float Realaltitude[AINUM][MAXWAVENUM][2];
    float Direcaltitude[AINUM][MAXWAVENUM][2];
    int ThreeGroupNum[AINUM];
    float Power[AINUM][3];					/* 功率*/
    float fGainMsuAngle[MAXWAVENUM][AINUM];
    float fGainMsuAngleBase[MAXWAVENUM][AINUM];
    BOOL fGainMsuAngleModifyEnalbeFlag;
    float fGainMsuPower[AINUM];
    float fGainMsuPowerBase[AINUM];
    BOOL fGainMsuPowerModifyEnalbeFlag;
    int PowerThreeGroupNum[AINUM];
    float PowerValid[AINUM];
    uint8_t MSUWinFull[AINUM];			/* 满窗标志 */
    uint8_t tmpMSUWinFull[AINUM];
    int CoeOrder[AINUM];			/* 系数序列 */
    int RoundNum[AINUM];				/* 周波点数 */
    float NumDif[AINUM]; 				/* 分点数 */
    uint16_t MSUSamCounter[AINUM]; 			/* 十周波计数 */
    uint16_t MSUSamTimes[AINUM]; 			/* The sampling counting */
    float fGainMsu[MAXWAVENUM][AINUM];
    float fGainMsuTotal[MAXWAVENUM][AINUM];
    float GainBase[MAXWAVENUM];
    float GainBaseVolt[MAXWAVENUM];
    int GainWaveFlag[MAXWAVENUM];
    float SysFrequency; 				/* Frequency of System */
    float MSUFreqPerChn[AINUM]; 			/* Frequency of the signal Coming from Analog Channel */
} MSUINFO, *pMSUINFO;

typedef struct TMPMSUINFO_tag		/* 临时信息结构*/
{
    int MSUtmpCoeOrder[AINUM];						/* 测量用临时参数 */
    int tmpRoundNum[AINUM];
    float tmpNumDif[AINUM];
    uint16_t tmpMsuSamCounter[AINUM];
    uint16_t tmpMsuSamTimes[AINUM]; 				/* The sampling counting */
} TMPMSUINFO, *pTMPMSUINFO;

typedef struct MSUCOE_tag		/* 系数结构 */
{
    float RecipAttenuationCoe[2600];
} MSUCOE, *pMSUCOE;

typedef struct POWERRESULT_tag		/* 功率计算结构 */
{
    int PowerCfgNum;
    char PowerName[AINUM][MAX_ID_LEN+1];
    float Power[AINUM][3];
} POWERRESULT, *pPOWERRESULT;

typedef struct MSURESULT_tag		/* 测量结果 */
{
    float MSUResult[10*MAXHCHNNUM];				/* 测量结果保存 */
    float MSUDFTRealImage[2*MAXWAVENUM*AINUM];					/* 测量计算中间结果保存 */
    float MSUAltAngle[2*MAXWAVENUM*AINUM];           		/* 测量结果幅角存储 */
    float SumofAlt[MAXWAVENUM*AINUM];				/* 幅角求和 */
    float SumofAngle[MAXWAVENUM*AINUM];  					/* 相角求和 */
} MSURESULT, *pMSURESULT;

/* globals */

extern float *pMusR;
extern float *pMsu;				/* 以上为测量用参数 */

#ifdef MSUDATATEST
extern float ftmpMsu[10000]; 			/* 用于测量测试 */
extern float *pftmpMsu;
extern float ftmpMsu2[10000]; 					/* 用于测量测试 */
extern float *pftmpMsu2;
#endif

extern MSUINFO MsuInfo_g;				/* 测量信息 */
extern POWERRESULT PowerResult;			/* 功率计算结果 */
extern MSURESULT MsuResult;
extern TMPMSUINFO TmpMsuInfo;

/* globals declarations */

/***********************************************************************
* MsuInit - 测量有关参数初始化
*
* RETURNS: 无
*
*/
extern void MsuInit(void);

/***********************************************************************
* MsuDataPretreatment - 前向预处理
*
* RETURNS: 无
*
*/
extern void MsuDataPretreatment(
    MSUINFO *pMsuInfo,
    DSP_MSU_AI_CFG *pMsu
);

/***********************************************************************
* MsuFastAltAngle - 快速幅值计算
*
* RETURNS: 无
*
*/
extern void MsuFastAltAngle(
    uint8_t Begin_Channel_Num, 		/* 开始通道 */
    uint8_t Channel_Nums, 		/* 通道数 */
    uint8_t Wave_Num		/* 谐波次数 */
);

/***********************************************************************
* BrAverage - 幅值相角平均运算
*
* RETURNS: 无
*
*/
extern void BrAverage(
    MSUCALC *pMsuCalc,
    uint8_t Num			/* 测量配置号 */
);

/***********************************************************************
* MsuStart - 测量计算缓冲区指针定位
*
* RETURNS: 无
*
*/
extern void MsuStart(void);

/***********************************************************************
* DataOrient - 指针给定
*
* RETURNS: 无
*
*/
extern void DataOrient(void);

/***********************************************************************
* RealMsuRecursionDFT - 测量用递归傅立叶算法
*
* RETURNS: 无
*
*/
extern void RealMsuRecursionDFT(
    uint8_t Begin_Channel_Num,			/* 开始通道 */
    uint8_t Channel_Nums, 		/* 总通道数 */
    uint8_t Wave_Num		/* 谐波次数 */
);

/***********************************************************************
* RealMsuFastAltAngle - 快速幅值计算
*
* RETURNS: 无
*
*/
extern void RealMsuFastAltAngle(
    uint8_t Begin_Channel_Num,		/* 开始通道 */
    uint8_t Channel_Nums, 		/* 总的通道数 */
    uint8_t Wave_Num		/* 谐波次数 */
);

/***********************************************************************
* FreqAverage - 频率平滑运算
*
* RETURNS: 无
*
*/
extern void FreqAverage(
    uint8_t Begin_Channel_Num,			/* 开始通道 */
    uint8_t Channel_Nums			/* 通道数 */
);

/***********************************************************************
* SingleCoeCal - 根据频率计算测量用系数
*
* RETURNS: 无
*
*/
extern void SingleCoeCal();

/***********************************************************************
* CountPowerDFT - 计算两通道之间的功率(使用傅立叶变换 )
*
* RETURNS: 无
*
*/
extern void CountPowerDFT(void);

/***********************************************************************
* CountPower - 计算两通道之间的功率(使用采样值，直接对电压和电流进行积分)
*
* RETURNS: 无
*
*/
extern void CountPower(void);

/***********************************************************************
* GetPower - 得到两通道之间的功率
*
* RETURNS: 功率结构
*
*/
extern POWERRESULT *GetPower(void);

/***********************************************************************
* MsuCoeCalInit - 测量参数文件初始化
*
* RETURNS: 无
*
*/
extern EP_STATUS MsuCoeCalInit(void);

/***********************************************************************
* MsuCoeCal - 保存测量参数文件
*
* RETURNS: 无
*
*/
extern EP_STATUS MsuCoeCal(void);

/***********************************************************************
* RdMsuCoe - 读取测量参数文件
*
* RETURNS: 无
*
*/
extern EP_STATUS RdMsuCoe(void);

/***********************************************************************
* ResetMsuCoe - 复位测量参数文件
*
* RETURNS: 无
*
*/
extern EP_STATUS ResetMsuCoe(void);

/***********************************************************************
* FreqPosFix - 频率位置确定
*
* RETURNS: 无
*
*/
extern void FreqPosFix(
    MSUINFO *pMsuInfo
);

#ifdef  __cplusplus
}
#endif

#endif
