/* dsp.h - subroutine library for handling algorithms of DSP */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 29jul06, dy add digital differential analyzer.
01a, 27dec05, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling algorithms of DSP.
*/

#ifndef DSP_H
#define DSP_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "edpbase.h"
#include "dspai.h"
#include "datetime.h"
#include "realdatadef.h"

/* defines */

#define NO_DEBUG    /* 调试信息显示与否开关 */

#ifdef EDP03_BUILD
#define MAXHCHNNUM 18				/* EDP03平台最大硬件通道数 */
#define HCHNNUM 15 											/* 硬件最大通道数 */
#endif

/* EDP02平台增加通道数无内存问题 */
#ifdef EDP_01_02_BUILD
#ifdef EDP02_GTP_BUILD
#define MAXHCHNNUM 10				/* EDP01平台C-A版最大硬件通道数 */
#define HCHNNUM 10 											/* 硬件最大通道数 */
#else
#define MAXHCHNNUM 96				/* EDP01平台C-A版最大硬件通道数 */
#define HCHNNUM 96 											/* 硬件最大通道数 */
#endif
#endif

#ifdef EXCITE_BUILD
#define MAXHCHNNUM 66				/* 励磁平台最大硬件通道数 */
#define HCHNNUM 66 											/* 硬件最大通道数 */
#endif

/* #define AOFLOAT */ 	     /* AO输出类型选择 */
#ifndef AOFLOAT
#define AOINTEGER
#endif

#define MAXINTERVALTIMES 4		/* 最大采样/上传间隔点 */
#define LENGTH_BUFFER (4*MAXSAMPPOINT*HCHNNUM)			/* 数据缓冲区长度 */
#define ADJUST_STEP_NUM 256 							/* 零漂调整步数*/
#define ZEROCYCLENUM 4					/* 零漂计算周波数 */
#define MSUCYCLENUM 4												/* 测量计算周波数 */
#define DSP_FILTER_R (1000)									/* 滤波电阻，低压所要求截止频率为2600Hz */
#define DSP_FILTER_C (0.000000022)									/* 滤波电容 */
#define RC2 (DSP_FILTER_R*DSP_FILTER_R*DSP_FILTER_C*DSP_FILTER_C)
#define MAXWAVE 5						/* 最大计算谐波次数 */
#define TRANSCHNNUM		8
#define TRANSCYCLENUM 3

#ifdef EXCITE_BUILD
#define PROCESSNUM 6	/* 每次处理点数 */
#define FREQBEGINCHN 24			/* 频率数个数 */
#define ANGLEBEGINCHN 42		/* 角度个数 */
#endif

#ifdef EXCITE_BUILD
#define ADNUMALL 24
#define FREQNUMALL 18
#define ANGLENUMALL 24
#endif

#if defined(ZEROCALC)		/* 由编译环境定义该宏 */
#define ZEROENABLE			/* 是否需要计算零漂 */
#define DBBUF 		/* 双临时缓冲区 */
#endif

#ifndef M_PI
#define M_PI 3.141592653589793238462643
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.4142135623730950488016887
#endif

#define INTERVALTIMES 64		/* 最大间隔点数,多点发送一次,提高效率 */

#define ZEROCALCMINALT (0.06)		/* 零漂计算时最小蜂值 */
#define ZEROCALCMAXALT (0.2)		/* 认为是零漂的最大值 */

#define INVALID_ANA_CHN_NO 254  /* 无效通道号, 悬空或没有配置 */

/* 循环队列长度, 过程层接收缓冲和中断缓冲一致 */
#define MAXQSIZESAMPDATA 239

/* typedefs */
typedef struct					/* 计算结构 */
{
    uint8_t ucBgnLgcCh;            /* 起始逻辑通道号 */
    uint8_t ucChNum;                    			/* 该预处理算法对应的逻辑通道号数 */
    uint8_t ucWaveNum;		/* 谐波次数*/
    uint8_t ucNumPoint;		/* 上传点数 */
    uint8_t ucArithNum;                 	/* 预处理算法号，按照规约中的定义 */
    float *pfSamBuf;						/* 采样点指针 */
    float *pCos;
    float *pSin;
    float *pfResultBufPos;			/* 存放结果指针*/
    uint8_t	ucRoundNum;						/* 半波计算通道 */
    uint8_t ucMaxLgcCh;            /* 起始逻辑通道号 */
    float RecipAttenuationCoe;
    int RealImageType[MAXHCHNNUM];					/* 实部虚部算法类型 */
    uint8_t uSendInterval;				/* 采样发送间隔点 */
    int8_t SamCountCnt;
    BOOL CycleCalFlag;
} DSPCALC;

typedef struct					/* 计算结构 */
{
    uint8_t ucBgnLgcCh;            /* 起始逻辑通道号 */
    uint8_t ucChNum;                    			/* 该预处理算法对应的逻辑通道号数 */
    uint8_t ucWaveNum;		/* 谐波次数*/
    uint8_t ucNumPoint;		/* 上传点数 */
    float *pfSamBuf;						/* 采样点指针 */
    float *pCos;
    float *pSin;
    float *pfResultBufPos;			/* 存放结果指针*/
    uint8_t	ucRoundNum;						/* 半波计算通道 */
    uint8_t ucMaxLgcCh;            /* 起始逻辑通道号 */
    float RecipAttenuationCoe;
} MSUCALC;

typedef struct DSPHANDLE_tag		/* DSP句柄 */
{
    uint16_t MsuNumber;
    DSP_MSU_AI_CFG MSUBuffer[HCHNNUM];
    DSP_CALC_AI_CFG PreBuffer[HCHNNUM];
    DSPCALC DspCal[HCHNNUM];
    MSUCALC MsuCal[HCHNNUM];
    DSP_LGC_AI_CFG LgcBuffer[3*HCHNNUM];
    DSP_CALC_AI_CFG *pPreAiAddr; 		/* The pointer to the preprocessing channel */
    DSP_LGC_AI_CFG *pLogChnInfo; 			/* The pointer to the logic channel */
    DSP_LGC_DC_AI_CFG *pDcDspCal;
    uint16_t PreProcessNumber; 			/* The number of channels needing to preprocess */
    uint16_t DataNumofPreProcess; 			/* The number of the data transformed after the preprocessing */
    uint8_t LogicChnNumber_8; 			/* Improve the efficiency */
    uint8_t LogicChnNumber_m;
    uint8_t DataNumofPreProcess_m;
    uint8_t DataNumofPreProcess_8; 			/* Improve the efficiency */
    u_int DcDspChnNum;
    uint8_t RoundNum[HCHNNUM];
    int MaxLgcChnNum[HCHNNUM];
    uint8_t NumPerPoints; 			/* The number of the points to transform once a time */
    uint8_t uSendInterval;				/* 采样发送间隔点 */
    uint8_t ucProcessInterval;				/* 采样处理间隔点，解决EDP01_CA扩展机箱段时连续发数据不成功的问题 */
} DSPHANDLE, *pDSPHANDLE;

typedef struct DSPINFO_tag		/* DSP计算结构 */
{
    uint16_t TotalPointinCycle;
    uint16_t TwoCycle;
    uint16_t TwoChnNum;
    uint8_t CalNum; 				/* 采样与计算间隔计数 */
    uint8_t IntervalNum;			/* 限制上传点数 */
    uint16_t DFTProcessingNum;			/* DFT processing hits per cycle */
    uint16_t DFTInterVal;			/* 限制DFT计算点数 */
    uint16_t SamTimes;
    uint16_t ProcessingNum;			/* processing hits per cycle */
    float DcPart[HCHNNUM];
    int ZeroAdjustFlag[HCHNNUM];
    float ZeroDelta[MAXHCHNNUM]; 			/* The amendment of the zero excursion */
    float ZeroExcursionperChn[MAXHCHNNUM];			/* The excursion from zero of the channels */
    int16_t iZeroExcurChn[MAXHCHNNUM];		/* 整点零漂 */
    uint16_t ZeroPassLast[MAXHCHNNUM]; 			/* The last point passing zero */
    uint8_t flagfre; 			/* 允许频率计算标志 */
    uint8_t ZeroBeginFlag;
    uint8_t DifPointNum;				/* The number of difference points */
    uint8_t NeedProcessing;
    uint8_t WinFull;
    float SysFrequency; 				/* Frequency of System */
    float fCurSysFrequency;												/* 系统当前频率 */
    uint8_t XPerShun; 		/* 瞬时处理点数 */
    uint8_t FreqCalNum; 			/* The number of channel requiring frequency calculation */
    uint8_t uOriginNum; 			/* 获取原始值通道数 */
    uint8_t uOriginDataPos[HCHNNUM];			/* The channel position needing to calculate frequency */
    uint8_t MsuBufFullFlag;					/* 测量缓冲区满标志 */
    uint8_t ZeroBufFullFlag;
    uint8_t AnatoLog[HCHNNUM];			 /* Relation between logic channel and physical channel,logic to phisical */
    uint8_t LogtoAna[HCHNNUM];			 /* Relation between pchysical channel and logic channel,phisical to logic */
    uint8_t SamtoAna[HCHNNUM];    /* Relation between physical channel and sampling channel, physical to sampling */
    uint8_t SamtoLog[HCHNNUM];  /* 逻辑通道到物理通道 */
    float PropConf[HCHNNUM]; 		/* Proportion coefficient */
    int32_t iPropConf[HCHNNUM]; /* 用于光差整数除法 */
    float fPropConf1[HCHNNUM]; 		/* 一次额定/额定数字量系数 */
    int32_t *pSmvValIn[HCHNNUM];	   /* 指向一次额定值 */
    int16_t *pSmvValOut[HCHNNUM];	   /* 指向额定数字量 */
    float fPropConf2[HCHNNUM]; 		/* 通道系数 */
    float fSmvInOut[HCHNNUM]; 		/* Proportion coefficient */
    float RCProp[MAXWAVE];			/* Filter attenuation coeficient */
    uint16_t HalfPointNuminCycle;
    uint16_t HalfPointNuminSingleChn;
    uint16_t DifIntervalNum;
    float fExceedAngle;				/* 超前角度 */
    float ForeAngle;
    float BackAngle;
    float fOldReal;
    float fOldImage;
    float fReal;
    float fImage;
    float fAngle;
    int iZeroBufPointNum;		/* 零漂缓冲区点数 */
} DSPINFO, *pDSPINFO;

typedef struct DSPCOE_tag		/* 系数结构 */
{
    float C1im[MAXSAMPPOINT];	/* 傅立叶计算实部及虚部计算系数 */
    float C1re[MAXSAMPPOINT];
    float C2im[MAXSAMPPOINT];
    float C2re[MAXSAMPPOINT];
    float C3im[MAXSAMPPOINT];
    float C3re[MAXSAMPPOINT];
    float C4im[MAXSAMPPOINT];
    float C4re[MAXSAMPPOINT];
    float C5im[MAXSAMPPOINT];
    float C5re[MAXSAMPPOINT];
    float C1im_Dif[MAXSAMPPOINT];
    float C1re_Dif[MAXSAMPPOINT];
    float C2im_Dif[MAXSAMPPOINT];
    float C2re_Dif[MAXSAMPPOINT];
    float C3im_Dif[MAXSAMPPOINT];
    float C3re_Dif[MAXSAMPPOINT];
    float C4im_Dif[MAXSAMPPOINT];
    float C4re_Dif[MAXSAMPPOINT];
    float C5im_Dif[MAXSAMPPOINT];
    float C5re_Dif[MAXSAMPPOINT];
} DSPCOE, *pDSPCOE;

typedef struct DSPRESULT_tag		/* DSP计算结果*/
{
    float *pMainResult;
    int16_t AdcData[MAXHCHNNUM];				/* Sampling data in a cycle */
    uint32_t *pChnStatus;
    int32_t *pAdcData;												/* 数据指针 */
    int16_t AdcDataTmp[MAXINTERVALTIMES][MAXHCHNNUM];
    float ResultBuf[10*MAXHCHNNUM];			/* 结果保存缓冲区 */
#ifdef EXCITE_BUILD
    float ResultArray[PROCESSNUM][MAXHCHNNUM];
#endif
    float DFTRealImage[10*MAXHCHNNUM];		/* Data buffer for fact part and virtual part in DFT calculation */
    float AltAngle[10*MAXHCHNNUM];		/* Breadth and angle */
    float DFTRealImageSingleStep[10*MAXHCHNNUM];			 /* data buffer to store fact part and virtual part in single step calculation */
    float TempDataBufNoZero[2*LENGTH_BUFFER];			/* Temporary data buffer not considering excursion */
#ifdef DBBUF
    float TempDataBufYesZero[2*LENGTH_BUFFER];			/* Temporary data buffer considering excursion */
#endif

#ifdef ZEROENABLE	/* zero excursion processing. */
    float fZeroExcersionBufTemp[ZEROCYCLENUM*MAXHCHNNUM*MAXSAMPPOINT];			/* 零漂计算用数据 */
#else
    float fZeroExcersionBufTemp[1];			/* 防止出错 */
#endif

    float *temp_p_complex;  /* 通道复数值填写地址 */
    float *dcdata; /* 直流采样值填写地址 */
} DSPRESULT, *pDSPRESULT;

/* globals */

extern uint8_t LogicChnNumber;						/* The number of logic channels */
extern uint8_t ucMaxAnaNumber;   /* 最大物理通道数 */
extern uint16_t SamplingNum_g;				/* hits per cycle */
extern float *pInstant_NoZero;						/* The current pointer to the data buffer not considering zero excursion */
extern float *pInstant_YesZero; 									/* The current pointer to the data buffer considering zero excursion */
extern uint16_t Sam_Counter; 			/* The sampling number */
extern uint16_t Sam_Times; 								/* The sampling number in a cycle */
extern float *pMain;										/* 结果缓冲区指针 */
extern float *pDbMain; /* 双缓冲通道采样值指针 */
extern uint32_t *pStsMain;  /* 状态指针,用于赋值 */
extern uint32_t *pDbStsMain;  /* 双缓冲通道状态值指针 */
extern float *pPreBufMain;

extern uint16_t Sam_Counter_g;		 /* 全局10周波计数 */
extern uint16_t Sam_Times_g; 			/* The sampling number in a cycle */
extern uint16_t Sam_Counter_Int_g;			/* The sampling number */ 		/* 中断中使用 */
extern uint16_t Sam_Times_Int_g; 				/* The sampling number in a cycle */

extern DSPINFO DspInfo;
extern DSPCOE DspCoe;
extern DSPRESULT DspResult;
/*extern ANALOGBUFHANDLE AnalogBufHandle;*/

extern uint8_t Sam_to_ana[HCHNNUM]; 			/* Relation between physical channel and sampling channel, physical to sampling */
extern uint32_t Sample_Rate;

#ifdef EXCITE_BUILD
extern int32_t Adc_Data[HCHNNUM]; 			/* Sampling data in a cycle，32bit */
extern float fpga_Data[HCHNNUM];				/* Sampling data in a cycle */
extern uint16_t Sam_Counter_Send; 			/* The calculation number */
extern float *pMainTmp;
#else
extern int16_t Adc_Data[HCHNNUM]; 			/* Sampling data in a cycle */

#endif

extern int32_t *pAdc_Data; 			/* Sampling data in a cycle */

/* globals declarations */

/***********************************************************************
* CDspInit - DSP计算初始化
*
* RETURNS: 无
*
*/
extern void CDspInit();

/***********************************************************************
* RealDataModuPretreatment - 前向通道预处理
*
* RETURNS: 无
*
*/
extern void RealDataModuPretreatment(
    DSPRESULT *pDspResult,
    DSPINFO *pDspInfo,
    DSPHANDLE *pDspHandle
);

/***********************************************************************
* RecursionDFTDif -差分递归傅立叶计算
*
* RETURNS: 无
*
*/
extern void RecursionDFTDif(
    DSPCALC *pDspCal,
    DSPINFO *pDspInfo,
    DSPRESULT *pDspResult
);

/***********************************************************************
* RecursionDFT -递归傅立叶计算
*
* RETURNS: 无
*
*/
extern void RecursionDFT(
    DSPCALC *pDspCal,
    DSPINFO *pDspInfo,
    DSPRESULT *pDspResult
);

/***********************************************************************
* RecursionDFTAltAngle -幅值相角计算
*
* RETURNS: 无
*
*/
extern void RecursionDFTAltAngle(
    DSPCALC *pDspCal,
    DSPINFO *pDspInfo,
    DSPRESULT *pDspResult
);

/***********************************************************************
* ZeroExcursionPollCal -零漂计算
*
* RETURNS: 无
*
*/
extern void ZeroExcursionPollCal(
    uint8_t Chn_Num,				/* 计算通道 */
    DSPHANDLE *pDspHandle,
    DSPINFO *pDspInfo
);

/***********************************************************************
* DFTDifCoefCal - Discrete fourier transform difference coefficient calculation
*
* RETURNS: 无
*
*/
extern void DFTDifCoefCal(
    uint16_t Sampling_Num			/* 差分傅立叶计算点数 */
);

/***********************************************************************
* DTFCoefCal - Discrete fourier transform coefficient calculation
*
* RETURNS: 无
*
*/
extern void DTFCoefCal(
    uint16_t Sampling_Num			/* 傅立叶计算点数 */
);

/***********************************************************************
* NumAdjust -采样计数调整
*
* RETURNS: 无
*
*/
extern void NumAdjust(void);

/***********************************************************************
* SamNumAdjust -采样全局计数调整
*
* RETURNS: 无
*
*/
extern void SamNumAdjust(void);

/***********************************************************************
* SamNumAdjustInt -采样全局计数调整(中断中调用)
*
* RETURNS: 无
*
*/
extern void SamNumAdjustInt(void);

/***********************************************************************
* RecipAttenuationCal -二阶RC滤波增益倒数计算
*
* RETURNS: 无
*
*/
extern float RecipAttenuationCal(
    float Freq			/* 频率*/
);

/***********************************************************************
* GetDcPart - 获取当前直流分量
*
* RETURNS: 无
*
*/
extern void GetDcPart(void);

/***********************************************************************
* ZeroCalibrate - 零点校准
*
* RETURNS: 无
*
*/
extern void ZeroCalibrate(
    uint8_t Chn_Num		/* 计算通道 */
);

/***********************************************************************
* AngleRotate - 角度旋转
*
* RETURNS: 无
*
*/
extern void AngleRotate(
    float fReal,		/* 实部 */
    float fImage,			/* 虚部 */
    DSPINFO *pDspInfo
);

/***********************************************************************
* AngleRotateAlt - 角度旋转
*
* RETURNS: 无
*
*/
extern void AngleRotateAlt(
    float fAlt,		/* 实部 */
    float fAngle,			/* 虚部 */
    DSPINFO *pDspInfo
);

/***********************************************************************
* SampDataPretreatment - 采样值传送
*
* RETURNS: 无
*
*/
extern void SampDataPretreatment(
    DSPINFO *pDspInfo,
    DSPRESULT *pDspResult	,
    int Flag
);

#ifdef  __cplusplus
}
#endif

#endif
