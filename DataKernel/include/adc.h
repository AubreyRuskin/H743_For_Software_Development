/* adc.h - subroutine library for handling the A/D convertion and the DSP */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 29jul06, dy add calibration methods for measuring.
01a, 27dec05, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling the A/D convertion and the DSP.
*/

#ifndef ADC_H
#define ADC_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "edpbase.h"
#include "dspai.h"
#include "realdata.h"
#include "dsp.h"
#include <semLib.h>

/* defines */

#define TESTSEARCHRANGE 256	  /* The length of buffer for dsp testing */

#define STATLENGTHNUM 8192       /* Data number for statistic */

/* EDP02平台200706版和200703版 */
#if defined(EDP02_CPU200706_BUILD) || defined(EDP02_CPU200703_BUILD)
#define ADCCHIPNUM 3	 /* 芯片数 */
#endif

#define CHIPCHNNUM 6		 /* 每芯片通道数 */

#define MASTEREXTSYN (PC10)		 /* EDP01平台C-A版本主机箱和扩展机箱同步信号 */
#define ADCBUFLENGTH 1000			     /* The length of data buffer for sampling */
#define TESTCHNADCBUFLENGTH 100	 /* The length of data buffer for specila channel sampling */

/* typedefs */

typedef struct ADCSAMPINFO_tag	  /* 采样有关信息 */
{
    int32_t Adc_Data[HCHNNUM];	/* Sampling data in a cycle */

    int32_t *pAdcBuf;
    int32_t Max[HCHNNUM];
    int32_t Min[HCHNNUM];
    float SumVal[HCHNNUM];		/* 和值 */
    float AvrVal[HCHNNUM];		/* 平均值 */
    float fAdc_Data[HCHNNUM];

    float *pfAdcBuf;
    float fMax[HCHNNUM];
    float fMin[HCHNNUM];
    struct TESTCHNINFO		/* 测试通道信息*/
    {
        uint8_t TestChnNum;
        float *pTestChnAdcData;
        float Breadth;
        float BMax;
        float BMin;
        float Angle;
        BOOL WinFull;	 /* 窗满标志 */
        BOOL RdFlag;
    } TestChnInfo;
    uint16_t SamplingNum;	/* hits per cycle */
    uint32_t SampleRate;
    float fSampleRate;   /* 浮点精确频率 */
    uint8_t flagfre; 		/* 允许频率计算标志 */
    float AdcModu;
    uint16_t nLampNum; 	/* 用于显示 */
    uint16_t MsuResultNum;
    uint32_t NumTest; 		/* 计数 */
} ADCSAMPINFO, *pADCSAMPINFO;

typedef struct SAMDATASHOW		/* 采样数据*/
{
    BOOL EnableFlag;
    int32_t iSamData[TESTSEARCHRANGE];
    float *pfSamData;		/* 提供浮点指针 */
    uint32_t ulCnt;
    uint8_t uChnNum;		/* 录波物理通道号 */
} SAMDATASHOW, *pSAMDATASHOW;

typedef struct SAMPINFO_tag		/* 采样有关信息 */
{
    int32_t Max[HCHNNUM];
    int32_t Min[HCHNNUM];
    float AvrVal[HCHNNUM];	/* 平均值 */
    float SumVal[HCHNNUM];		/* 和值 */
    int ChnNum;
} SAMPINFO, *pSAMPINFO;

typedef struct		/* 队列元素 */
{
    int32_t Data[MAXHCHNNUM];
    int32_t *pData; /* 指向采样数据点 */
    uint16_t SampCount;
    uint16_t SampTimes;
    uint32_t Status[MAXHCHNNUM];
    uint32_t *pStatus;  /* 指向状态点 */
} SampDataCur, *pSampDataCur;

typedef struct
{
    SampDataCur *base;
    int front;
    int rear;
    uint32_t ulSynTime;		/* 同步时间 */
} SqQueue;

typedef struct
{
    int32_t *pAdc_Data_Bak[INTERVALTIMES];		/* 临时指针 */
    uint16_t Sam_Counter_Bak[INTERVALTIMES];
    uint32_t *pAdc_Data_Sts_Bak[INTERVALTIMES];
    uint16_t Sam_Times_Bak[INTERVALTIMES]; 					/* The sampling number in a cycle */
    BOOL bDataValid[INTERVALTIMES];
} AsCur, *pAsCur;

typedef struct SAMPERRORSIMUL_tag
{
    uint8_t ChnNum;
    BOOL BreakOnePointFlag;
    BOOL bUseSimulData;		/* 使用仿真数据 */
    BOOL InsertZeroFlag;
    uint16_t InsertZeroCount;
    uint16_t usBreakPointCount;
    BOOL StopFlag;
    uint16_t StopCount;
    uint32_t ulOptCount;		/* 执行计数*/
    int32_t iWaveData[2*MAXSAMPPOINT];	/* 仿真数据 */
    uint16_t usTimes;
} SAMPERRORSIMUL;

/* globals */

extern BOOL bDspTaskStartFlag_g; 		/* DSP计算任务启动标志 */
extern BOOL bMsuTaskStartFlag_g; 				/* 测量计算任务启动标志 */
extern BOOL bZero_ExcurTaskStartFlag_g;  			/* 零漂计算任务启动标志 */
extern ADCSAMPINFO AdcSampInfo;
extern DSPHANDLE DspHandle;
extern SEM_ID NewDspData; 			/* 新数据到来信号灯 */
extern SEM_ID NewDspDataforZero;
extern SEM_ID NewDspDataforMsu;
extern BOOL bDspFirstReadAiFlag_g;
extern int32_t AdcData[MAXHCHNNUM];

/* 励磁平台 */
extern BOOL FPGAInitState;

extern float Adc_Modu;

extern int iAdcChipNum_g;                    /* CPU板上AD芯片个数 */
extern int g_iAdcChnNumPerChip;  /* 每个ADC芯片通道数 */

extern SqQueue SampDataQ;
extern AsCur DspAsCur;
extern SAMPERRORSIMUL SampErrorSimul;
extern SAMDATASHOW SampDataShow;
extern int TiIMMR_g;
extern void *pvAiModHandle_g; 	/* Handle */
extern uint32_t ulDspAccessCounter_g; 			/* DSP任务监视访问计数器 */
extern int nDSPTaskID_g;			/* 赋初始值*/
extern uint32_t ulTempData[4][6];

/* 励磁平台使用 */
extern uint32_t numtest; 			/* 测试计数 */
extern uint32_t ticknum;
extern uint32_t ulDspAccessCounter; 			/* DSP任务监视访问计数器 */

extern int EnableCalibrateFlag_g;
extern BOOL CalibrateEnableFlag;		/* 校准允许 */
extern BOOL MusAngleCalibrateEnableFlag;	 /* 角度校准允许 */
extern BOOL MusPowerCalibrateEnableFlag;		 /* 功率校准允许 */

/* globals declarations */

/***********************************************************************
* ds1306Delay - 延时
*
* RETURNS: 无
*
*/
extern void ds1306Delay(
    UINT32 time		/* 延时时间，单位ns */
);

/***********************************************************************
* AdcintDisable - 中断采样
*
* RETURNS: 无
*
*/
extern void AdcintDisable(void);

/***********************************************************************
* GetDSPTaskStatus - 获得DSP计算任务的状态，若正常，则返回真，否则，返回假
*
* RETURNS: 无
*
*/
extern BOOL GetDSPTaskStatus();

/***********************************************************************
* GetZeroExcurTaskStatus - 获得零漂计算任务的状态，若正常，则返回真，否则，返回假
*
* RETURNS: 无
*
*/
extern BOOL GetZeroExcurTaskStatus();

/***********************************************************************
* GetMsuTaskStatus - 获得测量计算任务状态，若正常，则返回真，否则，返回假
*
* RETURNS: 无
*
*/
extern BOOL GetMsuTaskStatus();

/***********************************************************************
* GetSampInfo - 获取采样信息
*
* RETURNS: 无
*
*/
extern void GetSampInfo(SAMPINFO *pSampInfo);
#if 0
/***********************************************************************
* RdRecBuf -读取录波缓冲区
*
* RETURNS: 无
*
*/
extern EP_STATUS RdRecBuf(
    ANALOGBUFHANDLE **ppBufHandle
);
#endif
/***********************************************************************
* Disable_PC14_Int - 禁止PC14中断
*
* RETURNS: 无
*
*/
extern void Disable_PC14_Int();

/***********************************************************************
* Disable_Timer2_Int - 禁止Timer2中断
*
* RETURNS: 无
*
*/
extern void Disable_Timer2_Int();

/***********************************************************************
* DeleteQueueSD - 删除队列所占用内存
*
* RETURNS: NONE
*
*/
extern void DeleteQueueSD(void);

/***********************************************************************
* UpdateAcCoff - 更新交流通道系数
*
* RETURNS: 无
*
*/
extern void UpdateAcCoff(void);

/* 更新一次额定值
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void UpdateSmvValIn(void);

/***********************************************************************
* EnQueueSD - 获取队列当前写入位置
*
* RETURNS: OK, ERROR
*
*/
extern STATUS EnQueueSD(
    SqQueue *Q,
    SampDataCur **p			/* 当前写入点地址，放在队尾 */
);

/* the rear position come back.
 * Para:
 *     Q, queue.
 * Return: NONE.
 */
static void __inline__ backrear (SqQueue *Q)
{
    Q->rear = (Q->rear+MAXQSIZESAMPDATA-1)%MAXQSIZESAMPDATA;				/* 返回一点　*/
}

/***********************************************************************
* DeQueueSD - 读取队列当前位置
*
* RETURNS: OK, ERROR
*
*/
extern STATUS DeQueueSD(
    SqQueue *Q,
    SampDataCur **p		/* 读取地址 */
);

/***********************************************************************
* DataProcessing - DSP调用程序
*
* RETURNS: 无
*
*/
extern void DataProcessing(
    DSPINFO *pDspInfo,
    DSPRESULT *pDspResult,
    DSPHANDLE *pDspHandle
);

/***********************************************************************
* ADC_CountMaxMin - 计算最大最小值
*
* RETURNS: 无
*
*/
extern void ADC_CountMaxMin(void);

/***********************************************************************
* AiOpt_Send -光纵缓冲数据写及发送
*
* RETURNS: 无
*
*/
extern void AiOpt_Send(void);

/***********************************************************************
* ADC_CountMaxMinTmp - 临时计算最大最小值
*
* RETURNS: 无
*
*/
extern void ADC_CountMaxMinTmp(void);

/***********************************************************************
* IO_Begin_Set_AD - 端口初始化
*
* RETURNS: 无
*
*/
extern void IO_Begin_Set_AD();

/***********************************************************************
* ADReset - 复位A/D采样芯片
*
* RETURNS: 无
*
*/
extern void ADReset(void);

/***********************************************************************
* IO_AD_Convert_Instant - 发送ADC转换信号
*
* RETURNS: 无
*
*/
extern uint32_t IO_AD_Convert_Instant();

/***********************************************************************
* WrADBusy - A/D转换BUSY信号读取
*
* RETURNS: 无
*
*/
extern BOOL WrADBusy(void);

extern STATUS InitQueueSD(
    SqQueue *Q
);

/***********************************************************************
* SetSampDataShowFlag - 设置采样数据显示标志
*
* RETURNS: 无
*
*/
extern void SetSampDataShowFlag(void);

/***********************************************************************
* ClearSampDataShowFlag - 清除采样数据显示标志
*
* RETURNS: 无
*
*/
extern void ClearSampDataShowFlag(void);

/***********************************************************************
* GetBaseChn - 获取基准通道
*
* RETURNS: 无
*
*/
extern int GetBaseChn();

#ifdef MSUDATATEST
/***********************************************************************
* MsuTestOut - 读取测量数据
*
* RETURNS: 无
*
*/
extern void MsuTestOut(void);

/***********************************************************************
* MsuTestOut2 - 读取测量数据，用于试验
*
* RETURNS: 无
*
*/
extern void MsuTestOut2(void);

/***********************************************************************
* DSPDataOut - 结果数据显示
*
* RETURNS: 无
*
*/
extern void DSPDataOut(void);

#endif

/***********************************************************************
* GetAdcData - 获取ADC转换值（目前只有EDP02平台使用）
*
* RETURNS: 无
*
*/
extern void GetAdcData(void);

/***********************************************************************
* ShowQData - 显示队列数据
*
* RETURNS: 无
*
*/
extern void ShowQData(void);

/***********************************************************************
* AppQInit - 应用初始化队列
*
* RETURNS: 无
*
*/
extern void AppQInit(void);

/* 获取采样通道延时(点数表示)
 * Para:
 *     NONE.
 * Return:
 *     点数.
 */
extern uint8_t adcGetDelayTime(void);

/* 获取扫描入口时刻节拍
 * Para:
 *     NONE.
 * Return:
 *     uint32_t.
 */
extern uint32_t RD_GetScanCnt(void);

#ifdef  __cplusplus
}
#endif

#endif
