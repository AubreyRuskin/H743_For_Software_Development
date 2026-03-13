/* adc_asst.h - subroutine library for handling the A/D convertion and the DSP, including simulation */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 25may09, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling the A/D convertion and the DSP, including simulation.
*/

#ifndef ADC_ASST_H
#define ADC_ASST_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "edpbase.h"
#include "dspai.h"
#include "realdata.h"
#include "dsp.h"
#include "adc.h"

/* defines */

#define SAMPLE48POINT	/* 使用周波48点进行采样 */

#ifndef SAMPLE48POINT
#define SAMPLE96POINT
#endif

/* typedefs */

typedef struct ADCDATAANALYSE_tag
{
    struct ADCDATAANALYSE_tag *pAhead;
    int16_t Num;
    int Count;
    struct ADCDATAANALYSE_tag *pNext;
} ADCDATAANALYSE;

typedef struct SENDTIMEINTERVALSTAT_tag		/* 发送间隔统计 */
{
    uint32_t ulCurCount;
    uint32_t ulForCount;
    int32_t lDif;
    int32_t lMin;
    uint16_t ulForeErrorTimerCount;
    uint16_t ulBackErrorTimerCount;
    uint16_t ulTimerForeCount;
    uint16_t ulTimerBackCount;
    int32_t lLowCout;
} SENDTIMEINTERVALSTAT, *pSENDTIMEINTERVALSTAT;

typedef struct SAMPTIMEINTERVALSTAT_tag		/* 采样间隔统计 */
{
    uint32_t ulDspEnterCount;
    uint32_t ulCurCount;
    uint32_t ulMaxCount;
    uint32_t ulMinCount;
    float fTotal;
    float fAvrCount;
} SAMPTIMEINTERVALSTAT, *pSAMPTIMEINTERVALSTAT;

typedef struct POWERCFG_tag		/* 功率配置*/
{
    int UNum;				/* 电压物理通道 */
    int UNumLog;					/* 逻辑通道 */
    int INum;					/* 电流物理通道 */
    int INumLog;			/* 逻辑通道 */
    float Power[MAXWAVE];						/* 视在功率 */
    float PowerValid[MAXWAVE];						/* 有功功率 */
    float PowerInvalid[MAXWAVE];							/* 无功功率 */
    float fCoff;						/* 增益系数 */
    uint8_t WaveNum;			/* 谐波次数*/
    float Angle;								/* 电压电流之间的夹角 */
    char PowerName[MAX_ID_LEN+1];
} POWERCFG, *pPOWERCFG;

/* globals */

extern POWERCFG PowerCfg[HCHNNUM/2];						/* 功率计算配置 */
extern int PowerCfgNum;					/* 功率计算配置数*/

/* global functions */

/***********************************************************************
* AdcDataRecAnalyse - 采样数据分析
*
* RETURNS: 无
*
*/
extern void AdcDataRecAnalyse(void);

/***********************************************************************
* AdcDataRecAnalyse - 采样数据分析
*
* RETURNS: 无
*
*/
extern void DataAnalyseTaskStart(void);

/***********************************************************************
* SimulWaveData - 使用仿真数据
*
* RETURNS: 无
*
*/
extern void SimulWaveData(
    uint8_t ChnNum,		/* 物理通道号 */
    uint16_t usUum,
    int32_t iAlt						/* 幅度，数字量 */
);

/***********************************************************************
* SampBreakOnePoint - 突变一点
*
* RETURNS: 无
*
*/
extern void SampBreakOnePoint(
    uint8_t ChnNum,		/* 物理通道号 */
    uint16_t usNum
);

/***********************************************************************
* SampInsertZero - 插零
*
* RETURNS: 无
*
*/
extern void SampInsertZero(
    uint8_t ChnNum,		/* 物理通道号 */
    uint16_t ZeroPointNum			/* 零点个数 */
);

/***********************************************************************
* SampStop - 停止采样
*
* RETURNS: 无
*
*/
extern void SampStop(
    uint16_t StopPointNum			/* 停止采样点数 */
);

/***********************************************************************
* SetHighestIntLevel - 设置最高优先级任务
*
* RETURNS: 无
*
*/
extern void SetHighestIntLevel(void);

/***********************************************************************
* GetSendInterval - 获取发送间隔
*
* RETURNS: 无
*
*/
extern void GetSendInterval(void);

/***********************************************************************
* AdcDataRec - 单通道录波
*
* RETURNS: 无
*
*/
extern void AdcDataRec(
    int32_t *pAdcData
);

/***********************************************************************
* SetAdcDataRec - 设定单通道录波
*
* RETURNS: 无
*
*/
extern void SetAdcDataRec(uint8_t nChnNum);

/***********************************************************************
* MsuPowerCalibrateEnable - 允许测量功率校准
*
* RETURNS: 无
*
*/
extern void MsuPowerCalibrateEnable(void);

/***********************************************************************
* MsuAngleCalibrateEnable - 允许测量角度校准
*
* RETURNS: 无
*
*/
extern void MsuAngleCalibrateEnable(void);

/***********************************************************************
* SetCalibrateZero - 零点校准
*
* RETURNS: 无
*
*/
extern EP_STATUS SetCalibrateZero(int VISel);

/***********************************************************************
* calibratefGain - 基波校准
*
* RETURNS: 无
*
*/
extern EP_STATUS calibratefGain(int VISel);

/***********************************************************************
* calibratefGain2 - 二次谐波校准
*
* RETURNS: 无
*
*/
extern EP_STATUS calibratefGain2(int VISel);

/***********************************************************************
* ShowInit - 显示初始化
*
* RETURNS: 无
*
*/
extern void ShowInit(void);

/***********************************************************************
* ResultShow - 结果统计显示
*
* RETURNS: 无
*
*/
extern void ResultShow(void);

/***********************************************************************
* CountMaxMin - 计算最大最小值
*
* RETURNS: 无
*
*/
extern void CountMaxMin(void);

/***********************************************************************
* SetCalibrate - 设置校准
*
* RETURNS: 无
*
*/
extern void SetCalibrate(
    int VISel,		/* 电压电流选择，0为电压；1为电流*/
    int WaveNum, 		/* 谐波次数，从1开始 */
    float GainBaseVal		/* 基准值*/
);

/***********************************************************************
* PowerCalInit - 功率计算配置
*
* RETURNS: 无
*
*/
extern void PowerCalInit(
    u_int uiLgcCh, 		/* 采样的逻辑通道数 */
    DSP_LGC_AI_CFG *plgccfg			/* 指向逻辑通道配置数组第0个元素的指针，数组元素有uiLgcCh个 */
);

/***********************************************************************
* SysDriveDisable - 系统驱动禁止
*
* RETURNS: 无
*
*/
extern void SysDriveDisable(void);

#ifdef MSUDATATEST

/***********************************************************************
* calibratefGainOut2 - 二次谐波测量结果校准
*
* RETURNS: 无
*
*/
extern void calibratefGainOut2(void);

/***********************************************************************
* calibratefGainOut - 基波测量结果校准
*
* RETURNS: 无
*
*/
extern void calibratefGainOut(void);

#endif

/***********************************************************************
* SetAdcSamp - Setting the ADC Sampling configuration
*
* RETURNS: 无
*
*/
void SetAdcSamp (
    int ChnNum
);

/***********************************************************************
* LCADResultShow - LC平台A/D测试结果显示
*
* RETURNS: 无
*
*/
extern void LCADResultShow(
    int Flag,		/* 转换开关，0为连续转换；1为多通道转换 */
    int Num,		/* 连续转换次数 */
    int ChnNum		/* 连续采样通道号 */
);

/***********************************************************************
* CancelAdcDataRec - 取消单通道录波
*
* RETURNS: 无
*
*/
extern void CancelAdcDataRec(void);

/***********************************************************************
* ShowAdcDataRec - 显示单通道录波结果
*
* RETURNS: 无
*
*/
extern void ShowAdcDataRec(uint8_t nChnNum);

/***********************************************************************
* GetFactVirtual- 获取傅立叶计算实部虚部
*
* RETURNS: 无
*
*/
extern float *GetFactVirtual(void);

/***********************************************************************
* GetBreadthTest - 获取测试用幅值
*
* RETURNS: 幅值
*
*/
extern float GetBreadthTest(void);

/***********************************************************************
* GetAngleTest - 获取测试用相位
*
* RETURNS: 相位
*
*/
extern float GetAngleTest(void);

/***********************************************************************
* GetBreadthMax - 获取测试用幅值最大值
*
* RETURNS: 幅值最大值
*
*/
extern float GetBreadthMax(void);

/***********************************************************************
* GetBreadthMin - 获取测试用幅值最小值
*
* RETURNS: 幅值最小值
*
*/
extern float GetBreadthMin(void);

/***********************************************************************
* GetBreadthMaxMin - 获取测试用幅值最大最小值
*
* RETURNS: 无
*
*/
extern void GetBreadthMaxMin(void);

/***********************************************************************
* AlterTestChn - 改变测试通道
*
* RETURNS: 无
*
*/
extern void AlterTestChn(
    int ChnNum		/* 测试通道，从0开始 */
);

/***********************************************************************
* GetSampDataTestChn - 获取测试通道连续采样值
*
* RETURNS: 地址指针
*
*/
extern float *GetSampDataTestChn(void);

/***********************************************************************
* CountSinSignal - 计算正弦信号
*
* RETURNS: 无
*
*/
extern void CountSinSignal(void);

/***********************************************************************
* DataTpyeTest - 数据类型测试
*
* RETURNS: 无
*
*/
extern void DataTpyeTest(void);

/***********************************************************************
* SysDriveEnable - 系统驱动启动
*
* RETURNS: 无
*
*/
extern void SysDriveEnable(void);

/***********************************************************************
* ResetCalibrate - 复位校准
*
* RETURNS: 无
*
*/
extern void ResetCalibrate(void);

#ifdef  __cplusplus
}
#endif

#endif
