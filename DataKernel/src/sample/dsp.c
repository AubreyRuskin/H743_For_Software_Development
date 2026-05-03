/* dsp.c - subroutine library for handling algorithms of DSP */

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
INCLUDES: adc.h
*/

/* includes */

#include "dsp.h"
#include "math_compat.h"
#include "angle_LT.h"

/* globals */

float *pMain;
float *pDbMain = NULL; /* 双缓冲通道采样值填写地址 */
uint32_t *pStsMain = NULL;  /* 状态指针 */
uint32_t *pDbStsMain = NULL;  /* 双缓冲通道状态值指针 */
float *pPreBufMain;
uint8_t LogicChnNumber; 			/* The number of logic channels */
uint8_t ucMaxAnaNumber = 0;   /* 最大物理通道数 */
uint16_t SamplingNum_g;			/* hits per cycle */
uint16_t Sam_Counter; 			/* The calculation number */
uint16_t Sam_Times; 					/* The calculation number in a cycle */
uint16_t Sam_Counter_g;			/* The sampling number */
uint16_t Sam_Times_g; 				/* The sampling number in a cycle */
uint16_t Sam_Counter_Int_g;			/* The sampling number */ 		/* 中断中使用 */
uint16_t Sam_Times_Int_g; 				/* The sampling number in a cycle */
float *pInstant_NoZero;				/* The current pointer to the data buffer not considering zero excursion */
float *pInstant_YesZero;			/* The current pointer to the data buffer considering zero excursion */

DSPINFO DspInfo;
DSPCOE DspCoe;
DSPRESULT DspResult __attribute__((section(".bss_itcm")));
/*ANALOGBUFHANDLE AnalogBufHandle;*/

/* 合并版在所有平台都定义 */
uint8_t Sam_to_ana[HCHNNUM]; 			/* Relation between physical channel and sampling channel, physical to sampling */
uint32_t Sample_Rate;
int16_t Adc_Data[HCHNNUM]; 			/* Sampling data in a cycle */

int32_t *pAdc_Data=NULL; 			/* Sampling data in a cycle */
extern BOOL OPTAD_flag; 				/* 光纵采样允许标志 */

/* local functions */

/***********************************************************************
* DFTDifHalfCycle -差分半波计算
*
* RETURNS: 无
*
*/
static void DFTDifHalfCycle(
    DSPCALC *pDspCal,
    DSPINFO *pDspInfo,
    DSPRESULT *pDspResult
);

/***********************************************************************
* DFTRoundCycle -全周计算
*
* RETURNS: 无
*
*/
static void DFTRoundCycle(
    DSPCALC *pDspCal,
    DSPINFO *pDspInfo,
    DSPRESULT *pDspResult
);

/* global functions */

extern float GetFreqMsu(
    int ChnNum		/* 通道号,从0开始 */
);

extern int GetPointNum(
    int ChnNum		/* 通道号,从0开始 */
);

extern int GetBaseChn();

/***********************************************************************
* CDspInit - DSP计算初始化
*
* RETURNS: 无
*
*/
void CDspInit(void)
{
    uint16_t i=0;

    pInstant_NoZero=DspResult.TempDataBufNoZero; 			/* Head address of data buffer not considering zero excursion */

    pMain=DspResult.ResultBuf;
    DspResult.pMainResult = DspResult.ResultBuf;

    Sam_Counter=10*DspInfo.XPerShun*DspInfo.ProcessingNum - 1; 			/* The sampling number */
    Sam_Times=DspInfo.XPerShun*DspInfo.ProcessingNum - 1; 			/* The sampling number in a cycle */
    Sam_Counter_g=10*DspInfo.XPerShun*SamplingNum_g - 1; 			/* The sampling number */
    Sam_Times_g=DspInfo.XPerShun*SamplingNum_g - 1; 			/* The sampling number in a cycle */
    Sam_Counter_Int_g=10*DspInfo.XPerShun*SamplingNum_g - 1; 			/* The sampling number */
    Sam_Times_Int_g=DspInfo.XPerShun*SamplingNum_g - 1; 			/* The sampling number in a cycle */

    for(i=0; i<HCHNNUM; i++)
    {
        DspInfo.ZeroExcursionperChn[i]=0.0;
        DspInfo.iZeroExcurChn[i]=0;			/* 整数零漂 */
        DspInfo.ZeroDelta[i]=0.0;
        DspInfo.ZeroPassLast[i]=0;
        DspInfo.ZeroAdjustFlag[i] = 0;	/* 零漂调整标志 */
    }

    for(i=0; i<2*HCHNNUM; i++)
    {
        DspResult.DFTRealImage[i]=0;
        DspResult.DFTRealImageSingleStep[i]=0;
        DspResult.AltAngle[i]=0;
    }

    DspInfo.TotalPointinCycle = SamplingNum_g*LogicChnNumber;
    DspInfo.TwoCycle = 2*SamplingNum_g;
    DspInfo.TwoChnNum = 2*LogicChnNumber;
    DspInfo.IntervalNum = SamplingNum_g/DspInfo.ProcessingNum;

    if(SamplingNum_g == 96)
        DspInfo.DFTInterVal = 4;		/* 固定为4点计算1次 */
    else if(SamplingNum_g == 72)
        DspInfo.DFTInterVal = 3;		/* 固定为3点计算1次 */
    else if(SamplingNum_g == 48)
        DspInfo.DFTInterVal = 2;		/* 固定为2点计算1次 */
    else if(SamplingNum_g == 24)
        DspInfo.DFTInterVal = 1;		/* 固定为1点计算1次 */

    else if(SamplingNum_g == 36)
        DspInfo.DFTInterVal = 1;		/* 固定为1点计算1次 */
    else if(SamplingNum_g == 72)
        DspInfo.DFTInterVal = 2;		/* 固定为2点计算1次 */

    else if(SamplingNum_g == 128)
    {
        DspInfo.DFTInterVal = 4;		/* 固定为4点计算1次 */
    }
    else if(SamplingNum_g == 200)
    {
        DspInfo.DFTInterVal = 4;		/* 固定为4点计算1次 */
    }

    DspInfo.DFTProcessingNum = DspInfo.ProcessingNum/DspInfo.DFTInterVal;		/* 固定为24点 */
    DspInfo.ZeroBeginFlag = 0; 		/* Whether to calculate the zero excersion */
    DspInfo.DifPointNum = 3;
    DspInfo.WinFull=0;			/* Judging whether the data window is full */
    DspInfo.NeedProcessing = 0;
    DspInfo.HalfPointNuminCycle=SamplingNum_g/2*LogicChnNumber;
    DspInfo.HalfPointNuminSingleChn=SamplingNum_g/2-1;
    DspInfo.DifIntervalNum = LogicChnNumber*DspInfo.DifPointNum;
    DspInfo.fExceedAngle=M_PI/2.0-1.5*2*M_PI/DspInfo.ProcessingNum;		/* 超前角度 */
}

/***********************************************************************
* DTFCoefCal - Discrete fourier transform coefficient calculation
*
* RETURNS: 无
*
*/
void DTFCoefCal(
    uint16_t Sampling_Num			/* 傅立叶计算点数 */
)
{
    uint16_t i;
    uint16_t num;
    uint16_t ntmp;
    float ftmp;
    float* pftmp;
    float* pftmp_Sec;
    float Recip_Sampling_Num;
    float Angle_Unit;
    float Val;

    float x;
    float Y_Back_Point;
    float Y_Fore_Point;

    Recip_Sampling_Num=1/(float)Sampling_Num;
    /* (2*pi/Sampling_Num)*255/(pi/2)=4*255/Sampling_Num,为2*pi/Sampling_Num */
    Angle_Unit=4*255*Recip_Sampling_Num;

    /* The first time wave coeficient calculation */
    /* The cosine value in the first quadrant */
    num=0;
    pftmp=DspCoe.C1im;
    for(i=0; i<(Sampling_Num/4+1); i++)
    {
        ftmp=Angle_Unit*num; /* 2*i*pi/Sampling_Num */
        ntmp=(int)ftmp;
        x=(float)ntmp;
        x=ftmp-x; /* difference */

        Y_Back_Point=*(Cos_Lu+ntmp);
        ntmp=ntmp+1;
        Y_Fore_Point=*(Cos_Lu+ntmp);
        Val=x*Y_Fore_Point+(1-x)*Y_Back_Point;

        Val=2*Val;
        Val=Recip_Sampling_Num*Val;
        Val=1/sqrt(2)*Val;
        *pftmp++=Val; /* 2/(N*sqrt(2))*cos(2*i*pi/Sampling_Num) */
        num=num+1;
    }

    /* The  cosine value in the second quadrant */
    pftmp_Sec=pftmp;
    pftmp_Sec--;
    pftmp_Sec--;
    for(i=0; i<(Sampling_Num/4-1); i++)
    {
        *pftmp++=-*pftmp_Sec--;
    }

    /* The  cosine value in the third quadrant and the cosine value in the fouth quadrant */
    pftmp_Sec=DspCoe.C1im;
    for(i=0; i<Sampling_Num/2; i++)
    {
        *pftmp++=-*pftmp_Sec++;
    }

    /* The sine value in the first quadrant */
    pftmp=pftmp-(int)(Sampling_Num/4); /* Address */
    pftmp_Sec=pftmp;
    pftmp=DspCoe.C1re;
    for(i=0; i<Sampling_Num/4; i++)
    {
        *pftmp++=*pftmp_Sec++;
    }

    /* The sine value in the second quadrant */
    pftmp_Sec=DspCoe.C1im;
    for(i=0; i<Sampling_Num/4; i++)
    {
        *pftmp++=*pftmp_Sec++;
    }

    /* The  sine value in the third quadrant and the sine value in the fouth quadrant */
    pftmp_Sec=DspCoe.C1re;
    for(i=0; i<Sampling_Num/2; i++)
    {
        *pftmp++=-*pftmp_Sec++;
    }
    /* The second time wave coeficient calculation */
    /* The cosine value */
    num=0;
    pftmp_Sec=DspCoe.C1im;
    pftmp=DspCoe.C2im;
    for(i=0; i<Sampling_Num; i++)
    {
        num=2*i;
        num=num+Sampling_Num;
        do
        {
            num=num-Sampling_Num;
        }
        while(num>=Sampling_Num);
        *pftmp++=*(pftmp_Sec+num);
    }

    /* The sine value */
    num=0;
    pftmp_Sec=DspCoe.C1re;
    pftmp=DspCoe.C2re;
    for(i=0; i<Sampling_Num; i++)
    {
        num=2*i;
        num=num+Sampling_Num;
        do
        {
            num=num-Sampling_Num;
        }
        while(num>=Sampling_Num);
        *pftmp++=*(pftmp_Sec+num);
    }

    /* The third time wave coeficient calculation */
    /* The cosine value */
    num=0;
    pftmp_Sec=DspCoe.C1im;
    pftmp=DspCoe.C3im;
    for(i=0; i<Sampling_Num; i++)
    {
        num=3*i;
        num=num+Sampling_Num;
        do
        {
            num=num-Sampling_Num;
        }
        while(num>=Sampling_Num);
        *pftmp++=*(pftmp_Sec+num);
    }

    /* The sine value */
    num=0;
    pftmp_Sec=DspCoe.C1re;
    pftmp=DspCoe.C3re;
    for(i=0; i<Sampling_Num; i++)
    {
        num=3*i;
        num=num+Sampling_Num;
        do
        {
            num=num-Sampling_Num;
        }
        while(num>=Sampling_Num);
        *pftmp++=*(pftmp_Sec+num);
    }

    /* The fourth time wave coeficient calculation */
    /* The cosine value */
    num=0;
    pftmp_Sec=DspCoe.C1im;
    pftmp=DspCoe.C4im;
    for(i=0; i<Sampling_Num; i++)
    {
        num=4*i;
        num=num+Sampling_Num;
        do
        {
            num=num-Sampling_Num;
        }
        while(num>=Sampling_Num);
        *pftmp++=*(pftmp_Sec+num);
    }

    /* The sine value */
    num=0;
    pftmp_Sec=DspCoe.C1re;
    pftmp=DspCoe.C4re;
    for(i=0; i<Sampling_Num; i++)
    {
        num=4*i;
        num=num+Sampling_Num;
        do
        {
            num=num-Sampling_Num;
        }
        while(num>=Sampling_Num);
        *pftmp++=*(pftmp_Sec+num);
    }

    /* The fifth time wave coeficient calculation */
    /* The cosine value */
    num=0;
    pftmp_Sec=DspCoe.C1im;
    pftmp=DspCoe.C5im;
    for(i=0; i<Sampling_Num; i++)
    {
        num=5*i;
        num=num+Sampling_Num;
        do
        {
            num=num-Sampling_Num;
        }
        while(num>=Sampling_Num);
        *pftmp++=*(pftmp_Sec+num);
    }

    /* The sine value */
    num=0;
    pftmp_Sec=DspCoe.C1re;
    pftmp=DspCoe.C5re;
    for(i=0; i<Sampling_Num; i++)
    {
        num=5*i;
        num=num+Sampling_Num;
        do
        {
            num=num-Sampling_Num;
        }
        while(num>=Sampling_Num);
        *pftmp++=*(pftmp_Sec+num);
    }
}

/***********************************************************************
* DFTDifCoefCal - Discrete fourier transform difference coefficient calculation
*
* RETURNS: 无
*
*/
void DFTDifCoefCal(
    uint16_t Sampling_Num			/* 差分傅立叶计算点数 */
)
{
    int i;
    uint16_t ntmp;
    float ftmp;
    float x;
    float Y_Back_Point;
    float Y_Fore_Point;

    float Recip_Sampling_Num;
    float Angle_Unit;
    float Val;
    float* pftmp;
    float* pftmp_Sec;

    Recip_Sampling_Num=1/(float)Sampling_Num;
    /* (2*pi/Sampling_Num)*255/(pi/2)=4*255/Sampling_Num,为2*pi/Sampling_Num */
    Angle_Unit=4*255*Recip_Sampling_Num;
    Angle_Unit=DspInfo.DifPointNum*Angle_Unit;
    /* 差分点数*(2*pi/(2*N))*255/(pi/2)=差分点数*2*255/Sampling_Num */
    Angle_Unit=0.5*Angle_Unit;

    /* The first time difference coeficient calculation */
    ftmp=255.0-Angle_Unit; /* Sine convert to cosine */
    ntmp=(int)ftmp;
    x=(float)ntmp;
    x=ftmp-x;
    Y_Back_Point=*(Cos_Lu+ntmp);
    ntmp=ntmp+1;
    Y_Fore_Point=*(Cos_Lu+ntmp);
    Val=x*Y_Fore_Point+(1-x)*Y_Back_Point; /* The sine value */

    /* Val=1/(2*sin(2*pi*差分点数/(2*N))),不考虑255点 */
    Val=2*Val;
    Val=1/Val;
    Val=2*Val; /* Using for half wave,two times */

    /* Cosine value */
    pftmp=DspCoe.C1re_Dif;
    pftmp_Sec=DspCoe.C1re;
    for(i=0; i<Sampling_Num; i++)
    {
        *pftmp++=Val*(*pftmp_Sec++);
    }
    /* Sine value */
    pftmp=DspCoe.C1im_Dif;
    pftmp_Sec=DspCoe.C1im;
    for(i=0; i<Sampling_Num; i++)
    {
        *pftmp++=Val*(*pftmp_Sec++);
    }

    /* The second time difference coeficient calculation */
    ftmp=255.0-2*Angle_Unit;
    ntmp=(int)ftmp;
    x=(float)ntmp;
    x=ftmp-x;
    Y_Back_Point=*(Cos_Lu+ntmp);
    ntmp=ntmp+1;
    Y_Fore_Point=*(Cos_Lu+ntmp);
    Val=x*Y_Fore_Point+(1-x)*Y_Back_Point;

    /* Val=1/(2*sin(2*2*pi*差分点数/(2*N))),不考虑255点 */
    Val=2*Val;
    Val=1/Val;
    Val=2*Val; /* Using for half wave */
    pftmp=DspCoe.C2re_Dif;
    pftmp_Sec=DspCoe.C2re;
    for(i=0; i<Sampling_Num; i++)
    {
        *pftmp++=Val*(*pftmp_Sec++);
    }
    pftmp=DspCoe.C2im_Dif;
    pftmp_Sec=DspCoe.C2im;
    for(i=0; i<Sampling_Num; i++)
    {
        *pftmp++=Val*(*pftmp_Sec++);
    }

    /* The third time difference coeficient calculation */
    ftmp=255.0-3*Angle_Unit;
    ntmp=(int)ftmp;
    x=(float)ntmp;
    x=ftmp-x;
    Y_Back_Point=*(Cos_Lu+ntmp);
    ntmp=ntmp+1;
    Y_Fore_Point=*(Cos_Lu+ntmp);
    Val=x*Y_Fore_Point+(1-x)*Y_Back_Point;

    /* Val=1/(2*sin(3*2*pi*差分点数/(2*N))),不考虑255点 */
    Val=2*Val;
    Val=1/Val;
    Val=2*Val; /* Using for half wave */
    pftmp=DspCoe.C3re_Dif;
    pftmp_Sec=DspCoe.C3re;
    for(i=0; i<Sampling_Num; i++)
    {
        *pftmp++=Val*(*pftmp_Sec++);
    }
    pftmp=DspCoe.C3im_Dif;
    pftmp_Sec=DspCoe.C3im;
    for(i=0; i<Sampling_Num; i++)
    {
        *pftmp++=Val*(*pftmp_Sec++);
    }

    /* The fourth time difference coeficient calculation */
    ftmp=255.0-4*Angle_Unit;
    ntmp=(int)ftmp;
    x=(float)ntmp;
    x=ftmp-x;
    Y_Back_Point=*(Cos_Lu+ntmp);
    ntmp=ntmp+1;
    Y_Fore_Point=*(Cos_Lu+ntmp);
    Val=x*Y_Fore_Point+(1-x)*Y_Back_Point;

    /* Val=1/(2*sin(4*2*pi*差分点数/(2*N))),不考虑255点 */
    Val=2*Val;
    Val=1/Val;
    Val=2*Val; /* Using for half wave */
    pftmp=DspCoe.C4re_Dif;
    pftmp_Sec=DspCoe.C4re;
    for(i=0; i<Sampling_Num; i++)
    {
        *pftmp++=Val*(*pftmp_Sec++);
    }

    pftmp=DspCoe.C4im_Dif;
    pftmp_Sec=DspCoe.C4im;
    for(i=0; i<Sampling_Num; i++)
    {
        *pftmp++=Val*(*pftmp_Sec++);
    }

    /* The fifth time difference coeficient calculation */
    /* 因差分点数过大，暂时不提供 */
}

/* convert the data and write the DSP buffer.
 * Para:
 *     pDspResult, result variable.
 *     pDspInfo, DSP information.
 *     pDspHandle, DSP calculation handle.
 * Return:
 *     OK, ERROR.
 */
void RealDataModuPretreatment(DSPRESULT *pDspResult, DSPINFO *pDspInfo, DSPHANDLE *pDspHandle)
{
    uint16_t i;
    int16_t ntmp;
    float ftmp;

    for (i=0; i<LogicChnNumber; i++)
    {
        /* processing the logic channel. */

        /* 通道裁剪处理(最后一个通道为缺省) */
        if ((pDspInfo->SamtoLog[i] == (HCHNNUM-1)) && (pDspInfo->flagfre == 1))
        {
            /* 扩展机箱不处理状态标 */
#ifndef EDP01_CA_EXT_BUILD
            pStsMain++;
#ifndef NO_DBL_BUF
            pDbStsMain++;
#endif
#endif

            pMain++;
#ifndef NO_DBL_BUF
            pDbMain++;
#endif
            continue;
        }

        /* 如果通过光差发送采样数据,则本侧处理数据必须和发送数据一致
         */
        if (OPTAD_flag)
        {
            ftmp = pDspResult->pAdcData[pDspInfo->SamtoLog[i]]/pDspInfo->iPropConf[i];

            if (ftmp > (32767.0-FLT_PRECISION))
            {
                ntmp = 32767;
            }
            else if (ftmp < (-32768.0+FLT_PRECISION))
            {
                ntmp = -32768;
            }
            else
            {
                ntmp = (int16_t)ftmp;
            }
            ftmp = ntmp*pDspInfo->fPropConf2[i];
        }
        else
        {
            ftmp = (float)pDspResult->pAdcData[pDspInfo->SamtoLog[i]]*pDspInfo->PropConf[i];			/* reciprocal of PT and CT plus. */
        }

        /* 扩展机箱不处理状态标 */
#ifndef EDP01_CA_EXT_BUILD
        *pStsMain++ = pDspResult->pChnStatus[pDspInfo->SamtoLog[i]];
#ifndef NO_DBL_BUF
        *pDbStsMain++ = pDspResult->pChnStatus[pDspInfo->SamtoLog[i]];
#endif
#endif

        *pMain++ = ftmp;	/* main buffer storage. */

#ifndef NO_DBL_BUF
        *pDbMain++ = ftmp;	/* main buffer storage. */
#endif

        /* 使能数字信号处理计算 */
        if (pDspHandle->PreProcessNumber || pDspInfo->FreqCalNum)
        {
            /* If config the preprocessing and frequency calculation, then write the DSP buffer. */
            *pInstant_NoZero = ftmp; 	/* storage of floating data not considering zero excursion. */
            *(pInstant_NoZero+LENGTH_BUFFER) = ftmp; 			/* double buffer. */


            if (pInstant_NoZero == pDspResult->TempDataBufNoZero+LENGTH_BUFFER-1)
                pInstant_NoZero = pDspResult->TempDataBufNoZero;  	/* buffer circulation, to the beginning of the buffer. */
            else
                pInstant_NoZero++;
        }
    }
}

/***********************************************************************
* SampDataPretreatment - 采样值传送
*
* RETURNS: 无
*
*/
void SampDataPretreatment(
    DSPINFO *pDspInfo,
    DSPRESULT *pDspResult,
    int Flag
)
{
    int i;

    if(Flag == 1)
    {
        for(i=0; i<pDspInfo->uOriginNum; i++)
        {
            *pMain=(float)(pDspResult->pAdcData[pDspInfo->uOriginDataPos[i]]*1.0);
            pMain++;

#ifndef NO_DBL_BUF
            *pDbMain = (float)(pDspResult->pAdcData[pDspInfo->uOriginDataPos[i]]*1.0);
            pDbMain++;
#endif
        }
    }
    else if(Flag == 0)
    {
        for(i=0; i<pDspInfo->uOriginNum; i++)
        {
            pMain++;
#ifndef NO_DBL_BUF
            pDbMain++;
#endif
        }
    }
}

/***********************************************************************
* ZeroExcursionPollCal -零漂计算
*
* RETURNS: 无
*
*/
void ZeroExcursionPollCal(
    uint8_t Chn_Num,				/* 计算通道 */
    DSPHANDLE *pDspHandle,			/* DSP计算句柄 */
    DSPINFO *pDspInfo				/* DSP有关信息 */
)
{
    uint16_t i, j;
    int16_t ntmp;
    uint8_t First_Flag;
    uint8_t Second_Flag;		/* considering the situation sampling 0 value */
    float ftmp;
    float ftmp_inv;
    float *pftmp;
    float Max;
    float *pFirst_Pass_Zero = NULL;
    float *pSecond_Pass_Zero = NULL;
    float *pMax_Pos;
    float Fore_Point;
    float Back_Point;
    float Multi_Fore_Back;
    static uint8_t Zero_Pass_Flag;
    uint8_t End_Flag;
    float Area_Total;   			/* Area calculation using for zero excersion */
    float Zero_Excersion_Temp[ZEROCYCLENUM*MAXSAMPPOINT]; 			/* Temporary data buffer for the passing Zero */
    int num;
    static uint32_t ulCnt=0;

    if(DspInfo.ZeroBeginFlag != 55)				/* 两周波后开始计算 */
    {
        if(Sam_Counter_g<DspInfo.TwoCycle)
        {
            /* 两周波后开始计算*/
            return;
        }
        DspInfo.ZeroBeginFlag=55;
    }
    i=Chn_Num;			/* 单通道计算 */

    First_Flag=0;				/* 参数初始化 */
    Second_Flag=0;
    Max=0;
    Area_Total=0;
    End_Flag=0;
    num=0;
    ntmp=0;

    /* The source of data */
    pftmp = DspResult.fZeroExcersionBufTemp + LogicChnNumber*SamplingNum_g*ZEROCYCLENUM; 			/* 从零漂计算缓冲区中获取数据 */
    pftmp = pftmp - LogicChnNumber; 		/* To the specified channel data buffer，后退一个周波，以前是两个  */
    pftmp=pftmp+i; 	/* 计算通道 */

    for(j=0; j<3*SamplingNum_g; j++)			/* 建立临时数据缓冲区，并寻找最大值 */
    {
        Zero_Excersion_Temp[j]=*pftmp;
        pftmp=pftmp-LogicChnNumber; 		/* To the previous channel */
        ftmp=fabs(Zero_Excersion_Temp[j]);
        if(ftmp>Max) 				/* The maximal data in a period */
        {
            Max=ftmp;
        }
    }

    pMax_Pos=Zero_Excersion_Temp+j-1;			/* The end address of the temporary data buffer */
    if(Max>ZEROCALCMINALT)		/* 原值为10*/
    {
        /* 幅值限制，峰值大于60mV或60mA才进行计算 */
        Zero_Pass_Flag=0;
        pftmp=Zero_Excersion_Temp;				/* 头指针，第一个数据 */
        do
        {
            Back_Point=*pftmp;

            if(pftmp == pMax_Pos)				/* considering the tail end of the sampling period, search the next sampling data. */
            {
                pftmp=Zero_Excersion_Temp;
                End_Flag=55;  				/* Having been coming to the end of the data buffer */
            }
            else
                pftmp++;
            Fore_Point=*pftmp;
            Multi_Fore_Back=Back_Point*Fore_Point;				/* 前后两点符号判断 */
            if(Multi_Fore_Back <= 0) 				/* 符号相反，为过零点，并且考虑其中一点为零的情况 */
            {
                if(Fore_Point >= Back_Point)				/* 正过零点 */
                {
                    Zero_Pass_Flag++;
                    if(Zero_Pass_Flag == 1)					/* 第1个正过零点 */
                    {
                        pFirst_Pass_Zero=pftmp;
                        num=0;
                        if(Fore_Point>0)			/* 前一点非零 */
                        {
                            First_Flag=1;
                        }
                    }
                    else if(Zero_Pass_Flag == 2)			/* 第2个正过零点 */
                    {
                        pSecond_Pass_Zero=pftmp;
                        if(Fore_Point>0)				/* 前一点非零 */
                        {
                            Second_Flag=1;
                        }
                        if((First_Flag == 0) && (Second_Flag == 1))			/* 看成全是零的情况 */
                        {
                            Second_Flag=0;
                        }
                    }
                }
            }
            if(Zero_Pass_Flag == 2)				/* 找到两个过零点 */
            {
                if((pSecond_Pass_Zero>Zero_Excersion_Temp) && (pSecond_Pass_Zero <= pMax_Pos))
                {
                    /* 后退一个点，到一个周波中，不是第一个点 */
                    pSecond_Pass_Zero--;
                }
                else if(pSecond_Pass_Zero == Zero_Excersion_Temp)
                {
                    /* 是第一个点 */
                    pSecond_Pass_Zero = pMax_Pos;
                }

                ntmp=pSecond_Pass_Zero-pFirst_Pass_Zero;				/* 周波点数减一，即整点数*/
                if(fabs(ntmp)<3)
                {
                    /* 防止抖动，整点数最少为3点 ，需要再找一个过零点 */
                    Zero_Pass_Flag=1; 			/* Filter,considering the effection of fluctuation */
                    num=0;
                }
            }
            num++;		/* 循环计数 */
        }
        while((Zero_Pass_Flag != 2) && (End_Flag == 0));		/* 没有找到两个过零点或者数据还没有结束 */

        if(Zero_Pass_Flag == 2)				/* Not overstep the data buffer */
        {
            /* 已找到两个点 */
            if(ntmp<0)
            {
                /* 防止数据次序反转，一般不会出现 */
                ntmp=ntmp+pMax_Pos-Zero_Excersion_Temp+1;				/* considering the tail end of the sampling period */
            }
            ftmp=(float)ntmp;		/* 转为浮点 */
            Fore_Point=*pFirst_Pass_Zero--; 				/* Positive */
            Back_Point=*pFirst_Pass_Zero++;
            Back_Point=fabs(Back_Point); 				/* Negative */

            ftmp_inv = Fore_Point+Back_Point;
            if(fabs(ftmp_inv) <= FLT_PRECISION)
            {
                /* 防止除数为零 */
                ftmp_inv=FLT_PRECISION;
            }

            ftmp=ftmp+Fore_Point/ftmp_inv;		/* 计算实际点数 */

            Area_Total=0.5*Fore_Point*(1+Fore_Point/ftmp_inv);

            pFirst_Pass_Zero++;
            do
            {
                Area_Total=Area_Total+*pFirst_Pass_Zero;
                pFirst_Pass_Zero++;
            }
            while(pFirst_Pass_Zero<pSecond_Pass_Zero);

            Back_Point=*pSecond_Pass_Zero++;				/* Negative */
            Back_Point=fabs(Back_Point);
            Fore_Point=*pSecond_Pass_Zero--;			/* 应防止溢出 */
            ftmp_inv = Fore_Point+Back_Point;

            if(fabs(ftmp_inv) <= FLT_PRECISION)				/* 防止除数为零 */
            {
                ftmp_inv=FLT_PRECISION;
            }
            ftmp=ftmp+Back_Point/ftmp_inv;		/* 点数 */

            Back_Point=*pSecond_Pass_Zero++; 				/* Guaranteeing the sign of Back_Point */

            Area_Total=Area_Total+0.5*Back_Point*(1+fabs(Back_Point)/ftmp_inv);		/* 求底时应该用绝对值 */
            Area_Total=Area_Total*1/ftmp;		/* 求零漂 */
        }
        else
        {
            /* 找不到两个正过零点，直接找周波点取平均  */
            pftmp=Zero_Excersion_Temp;
            for(j=0; j<SamplingNum_g; j++)
            {
                Area_Total=Area_Total+*pftmp++;
            }
            Area_Total=Area_Total/(float)SamplingNum_g;
        }
    }
    else
    {
        /* Not considering zero-passing sampling point，峰值达不到要求 */
        pftmp=Zero_Excersion_Temp;
        for(j=0; j<SamplingNum_g; j++)
        {
            Area_Total=Area_Total+*pftmp++;
        }
        Area_Total=Area_Total/(float)SamplingNum_g;
    }

    ulCnt++;

    assert(fabs(Area_Total)<1000);

    DspInfo.DcPart[i] = Area_Total;		/* 直流分量保存*/

    if(fabs(Area_Total)>ZEROCALCMAXALT)			/*  Comfirm the correction quantity，原值为0.009 */
    {
        /* 零漂过大，有可能由外部输入导致 */
        static uint32_t ulCnt=0;
        ulCnt++;
        if((ulCnt%0x3FFF) == 1)
        {
            char TempInfo[256];
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SAMPLE_ERR,
                           ER_REPORT|ER_ALARM| ER_LOCK | ER_NOLOGWRITE,	/* ER_LOCK，不闭锁开出 */
                           "(%02d)", DRIFT_BEYOND_LIMITS, 0);
                sprintf(TempInfo,"零漂越限: 第%d物理通道零漂值为%dmV/mA!\n", pDspHandle->LgcBuffer[pDspInfo->uOriginNum+i].ucHdCh, (int)(1000*DspInfo.DcPart[i]));
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SAMPLE_ERR,
                           ER_REPORT|ER_ALARM| ER_LOCK | ER_NOLOGWRITE,	/* ER_LOCK，不闭锁开出 */
                           "(%02d)",DRIFT_BEYOND_LIMITS,0);
                sprintf(TempInfo,"drift beyond limits!The %d physical channel value for the drift is %dmV/mA!\n", pDspHandle->LgcBuffer[pDspInfo->uOriginNum+i].ucHdCh, (int)(1000*DspInfo.DcPart[i]));
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
            }
        }
        Area_Total=0;
        DspInfo.ZeroDelta[i]=0;
    }
    else if(fabs(Area_Total) <= ZEROCALCMAXALT)		/* 很小的零漂认为是ADC造成的，需滤除 */
    {
        assert(fabs(Area_Total)<1000000);
        ftmp = Area_Total-DspInfo.ZeroExcursionperChn[i];		/* 计算零漂与当前零漂之差 */
        Area_Total=ftmp;
        assert(fabs(Area_Total)<1000000);
        DspInfo.ZeroDelta[i]=Area_Total/ADJUST_STEP_NUM;		/* 零漂单步调整量 */
    }

    DspInfo.ZeroAdjustFlag[i]  = 1;		/* 调整允许 */
}

/***********************************************************************
* RecursionDFT -递归傅立叶计算
*
* RETURNS: 无
*
*/
void RecursionDFT(
    DSPCALC *pDspCal,
    DSPINFO *pDspInfo,
    DSPRESULT *pDspResult
)
{
    uint8_t i;
    float ftmp;
    float* pftmp;
    float* pDFTVal; 			/* Point to the real part and image part */
    float* pDFTCos;
    float* pDFTSin;
    float DFTReal=0;
    float DFTImage=0;
    float *pfTest;

    pfTest=pInstant_NoZero;
    pftmp=pInstant_NoZero+LENGTH_BUFFER-pDspInfo->IntervalNum*LogicChnNumber+pDspCal->ucBgnLgcCh-pDspInfo->uOriginNum; 		/* To the specified channel,beginning from 0 减去定点算法*/
    pDFTVal=pDspCal->pfResultBufPos;

    pDFTSin=pDspCal->pSin+Sam_Times;		/* To the specified sampling point,begin from 0*/
    pDFTCos=pDspCal->pCos+Sam_Times;

    for(i=0; i<pDspCal->ucChNum; i++)
    {
        if((!(Sam_Times%pDspCal->uSendInterval)) || (!pDspCal->CycleCalFlag))
        {
            /* 按24点递推 */
            ftmp=*pftmp;
            pftmp=pftmp-pDspInfo->TotalPointinCycle; 		/* To the beginning point */
            ftmp=ftmp-*pftmp;
            pftmp=pftmp+pDspInfo->TotalPointinCycle;

            pftmp++; 		/* To the next channel */
            DFTReal=pDspCal->uSendInterval*(*pDFTSin)*ftmp;
            DFTImage=pDspCal->uSendInterval*(*pDFTCos)*ftmp;

            *pDFTVal=*pDFTVal+DFTReal;			/* real part */
            pDFTVal++;
            *pDFTVal=DFTImage+*pDFTVal;				/* image part */
            pDFTVal++;

            if(pDspCal->ucNumPoint == 2)
            {
                /* 上传2点 */
                *pMain++=*(pDFTVal-2)*pDspCal->RecipAttenuationCoe;
                *pMain++=*(pDFTVal-1)*pDspCal->RecipAttenuationCoe;

#ifndef NO_DBL_BUF
                *pDbMain++ = *(pDFTVal-2)*pDspCal->RecipAttenuationCoe;	/* main buffer storage. */
                *pDbMain++ = *(pDFTVal-1)*pDspCal->RecipAttenuationCoe;
#endif
            }
            else if(pDspCal->ucNumPoint == 1)
            {
                /* 上传1点 */
                *pMain++=*(pDFTVal-2)*pDspCal->RecipAttenuationCoe;
#ifndef NO_DBL_BUF
                *pDbMain++ = *(pDFTVal-2)*pDspCal->RecipAttenuationCoe;	/* main buffer storage. */
#endif
            }

            /* 傅立叶频率测量 */
            if(((i+pDspCal->ucBgnLgcCh) == GetBaseChn())&&(pDspCal->ucWaveNum == 0)&&(pDspCal->CycleCalFlag))
            {
                /* 针对基准通道进行计算，只针对0次谐波 */
                if(Sam_Times == 0)
                {
                    /* 频率计算删除 */
                }
            }
        }
        else
        {
            /* 不计算，仅仅上传 */
            if(pDspCal->ucNumPoint == 2)
            {
                /* 上传2点 */
                *pMain++=*pDFTVal*pDspCal->RecipAttenuationCoe;
#ifndef NO_DBL_BUF
                *pDbMain++=*pDFTVal*pDspCal->RecipAttenuationCoe;
#endif
                pDFTVal++;

                *pMain++=*pDFTVal*pDspCal->RecipAttenuationCoe;
#ifndef NO_DBL_BUF
                *pDbMain++=*pDFTVal*pDspCal->RecipAttenuationCoe;
#endif
                pDFTVal++;
            }
            else if(pDspCal->ucNumPoint == 1)
            {
                /* 上传1点 */
                *pMain++= *pDFTVal*pDspCal->RecipAttenuationCoe;

#ifndef NO_DBL_BUF
                *pDbMain++ = *pDFTVal*pDspCal->RecipAttenuationCoe;
#endif
                pDFTVal++;
                pDFTVal++;
            }

            /* 傅立叶频率测量 */
            if(((i+pDspCal->ucBgnLgcCh) == GetBaseChn())&&(pDspCal->ucWaveNum == 0)&&(pDspCal->CycleCalFlag))
            {
                /* 针对基准通道进行计算，只针对0次谐波 */
                if(Sam_Times == 0)
                {
                    /* 频率计算删除 */
                }
            }
        }
    }

    if(DspInfo.WinFull == 55)
    {
        /* 窗满 */
        {
            if(!(Sam_Times%pDspCal->uSendInterval))
            {
                /* 保证全周计算点与递推计算点同步 */

                if((!pDspCal->CycleCalFlag)&&(pDspCal->ucRoundNum == pDspCal->ucMaxLgcCh-1))
                {
                    /* 所有通道计算到 */
                    pDspCal->CycleCalFlag=TRUE;
                }

                if((pDspCal->ucRoundNum >= pDspCal->ucBgnLgcCh) && (pDspCal->ucRoundNum<pDspCal->ucMaxLgcCh))
                {
                    /* 循环数可以在最大通道之外累加 */
                    DFTRoundCycle(pDspCal, pDspInfo, pDspResult);
                }

                pDspCal->ucRoundNum++;
                if(pDspCal->ucRoundNum == pDspCal->ucMaxLgcCh)
                {
                    /* 超出最大通道号 */
                    pDspCal->ucRoundNum=pDspCal->ucBgnLgcCh;
                }
            }
        }
    }
}

/***********************************************************************
* DFTRoundCycle -全周计算
*
* RETURNS: 无
*
*/
static void DFTRoundCycle(
    DSPCALC *pDspCal,
    DSPINFO *pDspInfo,
    DSPRESULT *pDspResult
)
{
    uint16_t i;
    float* pftmp;
    int16_t ntmp;
    float* pDFTVal; 			/* Point to the real part and image part */
    float* pDFTCos;
    float* pDFTSin;
    float DFTReal;
    float DFTImage;

    DFTReal=0;		/* 初始化 */
    DFTImage=0;

    pftmp=pInstant_NoZero+LENGTH_BUFFER-pDspInfo->IntervalNum*LogicChnNumber+pDspCal->ucRoundNum-pDspInfo->uOriginNum-pDspInfo->IntervalNum*Sam_Times*LogicChnNumber; 			/* The source of data */
    pDFTVal=pDspResult->DFTRealImage+2*LogicChnNumber*pDspCal->ucWaveNum+2*(pDspCal->ucRoundNum-pDspInfo->uOriginNum);			/* To the specified channel and the specified wave time */

    pDFTSin=pDspCal->pSin;
    pDFTCos=pDspCal->pCos;

    pDspInfo->SamTimes = Sam_Times/pDspInfo->DFTInterVal;

    for(i=0; i<(pDspInfo->SamTimes+1); i++)
    {
        /* The amount of sampling point needing to process is Sam_Counter+1 */
        DFTReal=DFTReal+*pDFTSin*(*pftmp); 			/* real part */
        DFTImage=DFTImage+*pDFTCos*(*pftmp); 				/* image part */
        pDFTSin += pDspInfo->DFTInterVal;
        pDFTCos += pDspInfo->DFTInterVal;
        pftmp += pDspInfo->DFTInterVal*LogicChnNumber;
    }

    pftmp=pftmp-pDspInfo->TotalPointinCycle; 		/* To the previous cycle */

    ntmp=pDspInfo->DFTProcessingNum-pDspInfo->SamTimes;
    ntmp=ntmp;
    if(ntmp>1)
    {
        for(i=0; i<(ntmp-1); i++)
        {
            DFTReal=DFTReal+*pDFTSin*(*pftmp); 		/* real part */
            DFTImage=DFTImage+*pDFTCos*(*pftmp);  				/* image part */
            pDFTSin += pDspInfo->DFTInterVal;
            pDFTCos+= pDspInfo->DFTInterVal;
            pftmp += pDspInfo->DFTInterVal*LogicChnNumber;
        }
    }

    DFTReal = DFTReal*pDspInfo->DFTInterVal;
    DFTImage = DFTImage*pDspInfo->DFTInterVal;

    *pDFTVal=DFTReal;
    pDFTVal++;
    *pDFTVal=DFTImage; 			/* The storage of result */
}

/***********************************************************************
* RecursionDFTDif -差分递归傅立叶计算
*
* RETURNS: 无
*
*/
void RecursionDFTDif(
    DSPCALC *pDspCal,
    DSPINFO *pDspInfo,
    DSPRESULT *pDspResult
)
{
    uint16_t i;
    float ftmp;
    float *pftmp;
    float *pDFTVal; 			/* Point to the fact part and virtual part */
    float *pDFTDifCos;
    float *pDFTDifSin;
    float DFTReal;
    float DFTImage;
    float fReal; /* 实部 */
    float fImage;	/* 虚部 */

    pftmp=pInstant_NoZero+LENGTH_BUFFER-pDspInfo->IntervalNum*LogicChnNumber+pDspCal->ucBgnLgcCh-pDspInfo->uOriginNum; 			/* The source of data 减去定点算法 */
    pDFTVal=pDspCal->pfResultBufPos;

    pDFTDifSin=pDspCal->pSin+Sam_Times;
    pDFTDifCos=pDspCal->pCos+Sam_Times; 			/* To the specified sampling point */

    for(i=0; i<pDspCal->ucChNum; i++)
    {
        if((!(Sam_Times%pDspCal->uSendInterval)) || (!pDspCal->CycleCalFlag))
        {
            /* 按照24点递推 */
            ftmp=*pftmp;
            pftmp=pftmp-pDspInfo->DifIntervalNum;
            ftmp=ftmp-*pftmp;

            pftmp=pftmp-pDspInfo->HalfPointNuminCycle;
            ftmp=ftmp-*pftmp;

            pftmp=pftmp+pDspInfo->DifIntervalNum;
            ftmp=ftmp+*pftmp; 								/* x4-x3-x1+x2 */

            pftmp++; 		/* To the next channel */
            DFTReal=pDspCal->uSendInterval*ftmp*(*pDFTDifSin);		/* Pay attention to the coefficient. */
            DFTImage=pDspCal->uSendInterval*ftmp*(*pDFTDifCos);

            *pDFTVal=DFTReal+*pDFTVal;
            pDFTVal++;

            *pDFTVal=DFTImage+*pDFTVal;
            pDFTVal++;

            AngleRotate(*(pDFTVal-2), *(pDFTVal-1), pDspInfo); 		/* 角度旋转 */

            if(pDspCal->ucNumPoint == 2)
            {
                /* 双传2点 */
                *pMain++=pDspInfo->fReal*pDspCal->RecipAttenuationCoe; 			/* Main buffer storage */
                *pMain++=pDspInfo->fImage*pDspCal->RecipAttenuationCoe;

#ifndef NO_DBL_BUF
                *pDbMain++ = pDspInfo->fReal*pDspCal->RecipAttenuationCoe;	/* main buffer storage. */
                *pDbMain++ = pDspInfo->fImage*pDspCal->RecipAttenuationCoe;
#endif
            }
            else if(pDspCal->ucNumPoint == 1)
            {
                /* 上传1点 */
                *pMain++=pDspInfo->fReal*pDspCal->RecipAttenuationCoe; 			/* Main buffer storage */

#ifndef NO_DBL_BUF
                *pDbMain++ = pDspInfo->fReal*pDspCal->RecipAttenuationCoe;
#endif
            }

            /* 傅立叶频率测量 */
            if(((i+pDspCal->ucBgnLgcCh) == GetBaseChn())&&(pDspCal->ucWaveNum == 0)&&(pDspCal->CycleCalFlag))
            {
                /* 针对基准通道进行计算，只针对0次谐波 */
                if(Sam_Times == 0)
                {
                    /* 频率计算删除 */
                }
            }
        }
        else
        {
            fReal = *pDFTVal++;
            fImage = *pDFTVal++;

            AngleRotate(fReal, fImage, pDspInfo); 		/* 角度旋转 */

            if(pDspCal->ucNumPoint == 2)
            {
                /* 双传2点 */
                *pMain++=pDspInfo->fReal*pDspCal->RecipAttenuationCoe; 			/* Main buffer storage */
                *pMain++=pDspInfo->fImage*pDspCal->RecipAttenuationCoe; 					/* Main buffer storage,avoid storage repeatedly */

#ifndef NO_DBL_BUF
                *pDbMain++=pDspInfo->fReal*pDspCal->RecipAttenuationCoe;			/* Main buffer storage */
                *pDbMain++=pDspInfo->fImage*pDspCal->RecipAttenuationCoe; 					/* Main buffer storage,avoid storage repeatedly */
#endif


            }
            else if(pDspCal->ucNumPoint == 1)
            {
                /* 上传1点 */
                *pMain++=pDspInfo->fReal*pDspCal->RecipAttenuationCoe; 			/* Main buffer storage */

#ifndef NO_DBL_BUF
                *pDbMain++=pDspInfo->fReal*pDspCal->RecipAttenuationCoe;			/* Main buffer storage */

#endif
            }


            /* 傅立叶频率测量 */
            if(((i+pDspCal->ucBgnLgcCh) == GetBaseChn())&&(pDspCal->ucWaveNum == 0)&&(pDspCal->CycleCalFlag))
            {
                /* 针对基准通道进行计算，只针对0次谐波 */
                if(Sam_Times == 0)
                {
                    /* 频率计算删除 */
                }
            }
        }
    }

    if(pDspInfo->WinFull == 55)
    {
        /* 窗满后计算半周 */
        {
            if(!(Sam_Times%pDspCal->uSendInterval))
            {
                /* 保证全周计算点与递推计算点同步 */

                if((!pDspCal->CycleCalFlag)&&(pDspCal->ucRoundNum == pDspCal->ucMaxLgcCh-1))
                {
                    /* 所有通道计算到 */
                    pDspCal->CycleCalFlag=TRUE;
                }

                if((pDspCal->ucRoundNum >= pDspCal->ucBgnLgcCh) && (pDspCal->ucRoundNum<pDspCal->ucMaxLgcCh))
                {
                    /* 循环数可以在最大通道之外累加 */
                    DFTDifHalfCycle(pDspCal, pDspInfo, pDspResult);
                }

                pDspCal->ucRoundNum++;
                if(pDspCal->ucRoundNum == pDspCal->ucMaxLgcCh)
                {
                    /* 超出最大通道号 */
                    pDspCal->ucRoundNum=pDspCal->ucBgnLgcCh;
                }
            }
        }
    }
}

/***********************************************************************
* DFTDifHalfCycle -差分半波计算
*
* RETURNS: 无
*
*/
static void DFTDifHalfCycle(
    DSPCALC *pDspCal,
    DSPINFO *pDspInfo,
    DSPRESULT *pDspResult
)
{
    uint16_t i;
    uint16_t ntmp;
    float ftmp;
    float *pftmp;
    float *pftmpSec;
    float *pDFTVal;
    float *pDFTDifCos;
    float *pDFTDifSin;
    float DFTReal;
    float DFTVirtual;

    DFTReal=0;			/* 注意赋初值 */
    DFTVirtual=0;

    pftmp=pInstant_NoZero+LENGTH_BUFFER-pDspInfo->IntervalNum*LogicChnNumber+pDspCal->ucRoundNum-pDspInfo->uOriginNum; 			/* The source of data */
    pDFTVal=pDspResult->DFTRealImage+2*LogicChnNumber*pDspCal->ucWaveNum+2*(pDspCal->ucRoundNum-pDspInfo->uOriginNum);			/* To the specified channel and the specified wave time */

    pDFTDifCos=pDspCal->pCos;
    pDFTDifSin=pDspCal->pSin;
    ntmp=pDspInfo->HalfPointNuminSingleChn;

    if(Sam_Times >= ntmp)
    {
        pftmp=pftmp-pDspInfo->HalfPointNuminCycle;
        pftmp=pftmp+LogicChnNumber;
        pftmpSec=pftmp;
        pftmpSec=pftmpSec-pDspInfo->DifIntervalNum; 			/* To the different point */

        pDFTDifSin=pDFTDifSin+Sam_Times;
        pDFTDifCos=pDFTDifCos+Sam_Times;
        pDFTDifSin=pDFTDifSin-ntmp;
        pDFTDifCos=pDFTDifCos-ntmp;
        for(i=0; i<ntmp+1; i++)
        {
            ftmp=*pftmp-*pftmpSec;
            DFTReal=DFTReal+ftmp*(*pDFTDifSin);
            DFTVirtual=DFTVirtual+ftmp*(*pDFTDifCos);
            pftmp=pftmp+LogicChnNumber;
            pftmpSec=pftmpSec+LogicChnNumber;
            pDFTDifSin++;
            pDFTDifCos++;
        }
    }
    else
    {
        /* Sampling point number is not enough */
        pftmp=pftmp-Sam_Times*LogicChnNumber; 			/* To the begin point */
        pftmpSec=pftmp;
        pftmpSec=pftmpSec-pDspInfo->DifIntervalNum; 			/* To the different point */
        for(i=0; i<Sam_Times+1; i++)
        {
            ftmp=*pftmp-*pftmpSec;
            DFTReal=DFTReal+ftmp*(*pDFTDifSin);
            DFTVirtual=DFTVirtual+ftmp*(*pDFTDifCos);
            pftmp=pftmp+LogicChnNumber;
            pftmpSec=pftmpSec+LogicChnNumber;
            pDFTDifSin++;
            pDFTDifCos++;
        }
        ntmp=ntmp+1; 			/* 24 */
        pDFTDifSin=pDFTDifSin+ntmp;
        pDFTDifCos=pDFTDifCos+ntmp; 			/* To back half part */
        pftmp=pftmp-pDspInfo->HalfPointNuminCycle;
        pftmpSec=pftmp;
        pftmpSec=pftmpSec-pDspInfo->DifIntervalNum; 			/* To the different point */

        ntmp=ntmp-Sam_Times-1;
        for(i=0; i<ntmp; i++)
        {
            ftmp=*pftmp-*pftmpSec;
            DFTReal=DFTReal+ftmp*(*pDFTDifSin);
            DFTVirtual=DFTVirtual+ftmp*(*pDFTDifCos);
            pftmp=pftmp+LogicChnNumber;
            pftmpSec=pftmpSec+LogicChnNumber;
            pDFTDifSin++;
            pDFTDifCos++;
        }
    }
    *pDFTVal++=DFTReal;
    *pDFTVal=DFTVirtual;
}

/***********************************************************************
* RecursionDFTAltAngle -幅值相角计算
*
* RETURNS: 无
*
*/
void RecursionDFTAltAngle(
    DSPCALC *pDspCal,
    DSPINFO *pDspInfo,
    DSPRESULT *pDspResult
)
{
    uint16_t i;
    int16_t ntmp;
    float ftmp;
    float* pDFT_Val; 		/* Point to the real part and image part */
    float* pAngle;
    float* pBreadth_Angle;
    float DFT_Fact;
    float DFT_Virtual;
    float Fabs_DFT_Fact;
    float Fabs_DFT_Virtual;
    float x;
    float Y_Back_Point;
    float Y_Fore_Point;
    float Val;

    pDFT_Val=pDspResult->DFTRealImage+2*LogicChnNumber*pDspCal->ucWaveNum+2*(pDspCal->ucBgnLgcCh-pDspInfo->uOriginNum);			/* To the specified channel and the specified wave time */
    pBreadth_Angle=pDspResult->AltAngle+2*LogicChnNumber*pDspCal->ucWaveNum+2*(pDspCal->ucBgnLgcCh-pDspInfo->uOriginNum);				/* 减去原始数据通道 DY 8/7/2007 */

    pAngle=Angle;			/* 角度查表 */

    for(i=0; i<pDspCal->ucChNum; i++)
    {
        DFT_Fact=*pDFT_Val++;
        DFT_Virtual=*pDFT_Val++;
        Fabs_DFT_Fact=fabs(DFT_Fact);
        Fabs_DFT_Virtual=fabs(DFT_Virtual);

        ftmp=DFT_Fact*DFT_Fact+DFT_Virtual*DFT_Virtual;

        if(ftmp<0.00001)		/* 防止为零出现错误 */
            ftmp=0.00001;

        ftmp=sqrt(ftmp); 		/* 幅值，其计算可以优化 */
        *pBreadth_Angle++=ftmp*pDspCal->RecipAttenuationCoe; 			/* Store the breadth amended */
        ftmp=1/ftmp;

        if(Fabs_DFT_Fact>Fabs_DFT_Virtual)
        {
            ftmp=255.0*Fabs_DFT_Fact*ftmp;
            ntmp=(int)ftmp;
            x=(float)ntmp;
            x=ftmp-x;
            Y_Back_Point=*(Angle+ntmp);
            ntmp++;
            Y_Fore_Point=*(Angle+ntmp);
            Val=x*Y_Fore_Point+(1-x)*Y_Back_Point;
            Val=90-Val;
        }
        else
        {
            ftmp=255.0*Fabs_DFT_Virtual*ftmp;
            ntmp=(int)ftmp;
            x=(float)ntmp;
            x=ftmp-x;
            Y_Back_Point=*(Angle+ntmp);
            ntmp++;
            Y_Fore_Point=*(Angle+ntmp);
            Val=x*Y_Fore_Point+(1-x)*Y_Back_Point;
        }
        if(DFT_Fact<0)
        {
            if(DFT_Virtual>0)
            {
                Val=180.0-Val;
            }
            else
            {
                Val=180.0+Val;
            }
        }
        else
        {
            if(DFT_Virtual<0)
                Val=360.0-Val;
        }
        *pBreadth_Angle++=Val;

        if(pDspCal->RealImageType[pDspCal->ucBgnLgcCh+i] == 5)
        {
            /* 半周差分 */
            AngleRotateAlt(*(pBreadth_Angle-2), *(pBreadth_Angle-1), pDspInfo);
        }

        if(pDspCal->ucNumPoint == 2)
        {
            /* Main buffer storage */
            *pMain++=*(pBreadth_Angle-2);			/* 幅值 */

#ifndef NO_DBL_BUF
            *pDbMain++=*(pBreadth_Angle-2);			/* 幅值 */
#endif

            if(pDspCal->RealImageType[pDspCal->ucBgnLgcCh+i] == 5)
            {
                *pMain++=pDspInfo->fAngle; 					/* 相角 */

#ifndef NO_DBL_BUF
                *pDbMain++=pDspInfo->fAngle;			/* 幅值 */
#endif
            }
            else
            {
                *pMain++=*(pBreadth_Angle-1); 					/* 相角 */

#ifndef NO_DBL_BUF
                *pDbMain++=*(pBreadth_Angle-1); 					/* 相角 */
#endif
            }
        }
        else if(pDspCal->ucNumPoint == 1)
        {
            *pMain++=*(pBreadth_Angle-2); 		/* 幅值 */

#ifndef NO_DBL_BUF
            *pDbMain++=*(pBreadth_Angle-2); 		/* 幅值 */
#endif
        }
    }
}

/***********************************************************************
* NumAdjust -采样计数调整
*
* RETURNS: 无
*
*/
void NumAdjust(void)
{
    if(Sam_Times==DspInfo.XPerShun*DspInfo.ProcessingNum)
    {
        Sam_Times=0;
    }
    if(Sam_Counter==10*DspInfo.XPerShun*DspInfo.ProcessingNum)
    {
        /* Having been sampling ten cycles */
        Sam_Counter=0;
    }
}

/***********************************************************************
* SamNumAdjust -采样全局计数调整
*
* RETURNS: 无
*
*/
void SamNumAdjust(void)
{
    if(Sam_Times_g==DspInfo.XPerShun*SamplingNum_g)
    {
        Sam_Times_g=0;
    }
    if(Sam_Counter_g==10*DspInfo.XPerShun*SamplingNum_g)
    {
        /* Having been sampling ten cycles */
        Sam_Counter_g=0;
    }
}

/***********************************************************************
* SamNumAdjustInt -采样全局计数调整(中断中调用)
*
* RETURNS: 无
*
*/
void SamNumAdjustInt(void)
{
    int16_t ntmp;

    if(DspInfo.WinFull != 55)
    {
        ntmp=DspInfo.XPerShun*DspInfo.ProcessingNum;
        ntmp=ntmp-Sam_Times_Int_g; 			/* Sam_Counter begin with 0 */
        if(ntmp == 0)
        {
            DspInfo.WinFull=55;
            Sam_Times_Int_g=0;			/* Sampling begin again when the window is full */
        }
    }

    if(Sam_Times_Int_g==DspInfo.XPerShun*SamplingNum_g)
    {
        Sam_Times_Int_g=0;
    }
    if(Sam_Counter_Int_g==10*DspInfo.XPerShun*SamplingNum_g)
    {
        /* Having been sampling ten cycles */
        Sam_Counter_Int_g=0;
    }
}

/***********************************************************************
* RecipAttenuationCal -二阶RC滤波增益倒数计算
*
* RETURNS: 无
*
*/
float RecipAttenuationCal(
    float Freq			/* 频率*/
)
{
    float ftmp;
    float omg;

    omg = 2*M_PI*Freq;
    omg = omg*omg;

    ftmp = 1-omg*RC2;
    ftmp = ftmp*ftmp;

    ftmp = ftmp +9*omg*RC2;

    assert(ftmp>0);
    ftmp =sqrt(ftmp);

    return ftmp;
}

/***********************************************************************
* ShowZeroExcursion -显示零漂
*
* RETURNS: 无
*
*/
void ShowZeroExcursion(void)
{
    int i;

    for(i=0; i<LogicChnNumber; i++)
    {
        logMsg("%d = %d %d\n", i, (int)(DspInfo.ZeroExcursionperChn[i]*1000), DspInfo.ZeroAdjustFlag[i], 0, 0, 0);
    }

    for(i=0; i<LogicChnNumber; i++)
    {
        logMsg("%d = %d\n", i, DspInfo.iZeroExcurChn[i], 0, 0, 0, 0);
    }
}

/***********************************************************************
* GetDcPart - 获取当前直流分量
*
* RETURNS: 无
*
*/
void GetDcPart(void)
{
    int i;

    for(i=0; i<LogicChnNumber; i++)
    {
        LOG_Dbg_Msg("%d= %d\n", i, (int)(DspInfo.DcPart[i]*1000), 0, 0, 0, 0);
    }

}

/***********************************************************************
* ZeroCalibrate -零点校准
*
* RETURNS: 无
*
*/
void ZeroCalibrate(
    uint8_t Chn_Num		/* 计算通道号 */
)
{
    uint16_t i,j;
    int16_t ntmp;
    uint8_t First_Flag;
    uint8_t Second_Flag;			/* considering the situation sampling 0 value */
    float ftmp;
    float ftmp_inv;
    float *pftmp;
    float Max;
    float *pFirst_Pass_Zero = NULL;
    float *pSecond_Pass_Zero =  NULL;
    float *pMax_Pos;
    float Fore_Point;
    float Back_Point;
    float Multi_Fore_Back;
    static uint8_t Zero_Pass_Flag;
    uint8_t End_Flag;
    float Area_Total;   				/* Area calculation using for zero excersion */
    float Zero_Excersion_Temp[200]; 				/* Temporary data buffer for the passing Zero */
    int num;
    static uint32_t ulCnt = 0;

    if(DspInfo.ZeroBeginFlag != 55)				/* 两周波后开始计算 */
    {
        if(Sam_Counter_g<DspInfo.TwoCycle)
            return;
        DspInfo.ZeroBeginFlag=55;
    }
    i=Chn_Num;			/* 单通道计算 */

    First_Flag=0;		/* 参数初始化 */
    Second_Flag=0;
    Max=0;
    Area_Total=0;
    End_Flag=0;
    num = 0;
    ntmp = 0;

    pftmp = DspResult.fZeroExcersionBufTemp + LogicChnNumber*SamplingNum_g*ZEROCYCLENUM; 			/* 从零漂计算缓冲区中获取数据 */
    pftmp = pftmp - LogicChnNumber; 		/* To the specified channel data buffer，后退一个周波，以前是两个  */
    pftmp=pftmp+i;

    for(j=0; j<3*SamplingNum_g; j++)
    {
        /* 建立临时数据缓冲区，并寻找最大值 */
        Zero_Excersion_Temp[j]=*pftmp;
        pftmp=pftmp-LogicChnNumber; 		/* To the previous channel */
        ftmp=fabs(Zero_Excersion_Temp[j]);
        if(ftmp>Max) 				/* The maximal data in a period */
            Max=ftmp;
    }

    pMax_Pos=Zero_Excersion_Temp+j-1;			/* The end address of the temporary data buffer */
    if(Max>0.06)		/* Modified by DY 7/27/2006，原值为10*/
    {
        /* 幅值限制 */
        Zero_Pass_Flag=0;
        pftmp=Zero_Excersion_Temp;				/* 头指针 */
        do
        {
            Back_Point=*pftmp;

            if(pftmp==pMax_Pos)				/* considering the tail end of the sampling period */
            {
                pftmp=Zero_Excersion_Temp;
                End_Flag=55;  				/* Having been coming to the end of the data buffer */
            }
            else
                pftmp++;
            Fore_Point=*pftmp;
            Multi_Fore_Back=Back_Point*Fore_Point;				/* 前后两点符号判断 */
            if(Multi_Fore_Back<=0) 				/* 符号相反，为过零点，并且考虑其中一点为零的情况 */
            {
                if(Fore_Point>=Back_Point)				/* 正过零点 */
                {
                    Zero_Pass_Flag++;
                    if(Zero_Pass_Flag==1)					/* 第1个正过零点 */
                    {
                        pFirst_Pass_Zero=pftmp;
                        num=0;
                        if(Fore_Point>0)			/* 前一点非零 */
                            First_Flag=1;
                    }
                    else if(Zero_Pass_Flag==2)			/* 第2个正过零点 */
                    {
                        pSecond_Pass_Zero=pftmp;
                        if(Fore_Point>0)				/* 前一点非零 */
                            Second_Flag=1;
                        if(First_Flag==0&&Second_Flag==1)			/* 看成全是零的情况 */
                            Second_Flag=0;
                    }
                }
            }
            if(Zero_Pass_Flag==2)				/* 找到两个过零点 */
            {
                if((pSecond_Pass_Zero>Zero_Excersion_Temp)&&(pSecond_Pass_Zero<=pMax_Pos))		/* 10/9/2006 */
                    pSecond_Pass_Zero--;
                else if(pSecond_Pass_Zero == Zero_Excersion_Temp)
                    pSecond_Pass_Zero = pMax_Pos;

                ntmp=pSecond_Pass_Zero-pFirst_Pass_Zero;
                if(fabs(ntmp)<3) 			/*Considering the tail end of the data buffer */
                {
                    Zero_Pass_Flag=1; 			/* Filter,considering the effection of fluctuation */
                    num=0;
                }
            }
            num++;
        }
        while((Zero_Pass_Flag!=2)&&(End_Flag==0));

        if(Zero_Pass_Flag==2)				/* Not overstep the data buffer */
        {
            if(ntmp<0)
                ntmp=ntmp+pMax_Pos-Zero_Excersion_Temp+1;				/* considering the tail end of the sampling period */
            ftmp=(float)ntmp;
            Fore_Point=*pFirst_Pass_Zero--; 				/* Positive */
            Back_Point=*pFirst_Pass_Zero++;
            Back_Point=fabs(Back_Point); 				/* Negative */

            ftmp_inv = Fore_Point+Back_Point;				/* 防止除数为零 */
            if(fabs(ftmp_inv)<=0.00001)
                ftmp_inv=0.00001;

            ftmp=ftmp+Fore_Point/ftmp_inv;
            Area_Total=0.5*Fore_Point*(1+Fore_Point/ftmp_inv);

            pFirst_Pass_Zero++;
            do
            {
                Area_Total=Area_Total+*pFirst_Pass_Zero;
                pFirst_Pass_Zero++;
            }
            while(pFirst_Pass_Zero<pSecond_Pass_Zero);

            Back_Point=*pSecond_Pass_Zero++;				/* Negative */
            Back_Point=fabs(Back_Point);
            Fore_Point=*pSecond_Pass_Zero--;
            ftmp_inv = Fore_Point+Back_Point;

            if(fabs(ftmp_inv)<=0.00001)				/* 防止除数为零 */
                ftmp_inv=0.00001;
            ftmp=ftmp+Back_Point/ftmp_inv;

            /* 测试 */
            Back_Point=*pSecond_Pass_Zero++; 				/* Guaranteeing the sign of Back_Point */
            Area_Total=Area_Total+0.5*Back_Point*(1+Back_Point/ftmp_inv);
            Area_Total=Area_Total*1/ftmp;
        }
        else
        {
            /* Added by DY 7/27/2006 */
            pftmp=Zero_Excersion_Temp;
            for(j=0; j<SamplingNum_g; j++)
            {
                Area_Total=Area_Total+*pftmp++;
            }
            Area_Total=Area_Total/(float)SamplingNum_g;
        }
    }
    else
    {
        /* Not considering zero-passing sampling point */
        pftmp=Zero_Excersion_Temp;
        for(j=0; j<SamplingNum_g; j++)
        {
            Area_Total=Area_Total+*pftmp++;
        }
        Area_Total=Area_Total/(float)SamplingNum_g;
    }

    ulCnt++;

    assert(fabs(Area_Total)<1000);

    DspInfo.DcPart[i] = Area_Total;		/* 直流分量保存*/

    assert(fabs(Area_Total)<1000000);
    ftmp = Area_Total-DspInfo.ZeroExcursionperChn[i];
    Area_Total=ftmp;
    assert(fabs(Area_Total)<1000000);
    DspInfo.ZeroDelta[i]=Area_Total/ADJUST_STEP_NUM;

    DspInfo.ZeroAdjustFlag[i]  = 1;
}

/***********************************************************************
* AngleRotate - 角度旋转
*
* RETURNS: 无
*
*/
void AngleRotate(
    float fReal,		/* 实部 */
    float fImage,			/* 虚部 */
    DSPINFO *pDspInfo
)
{
    float Alt;
    float CurrentAngle;

    if(fabs(fReal)<0.00001)
    {
        if(fReal>0)
            fReal = 0.00001;
        else
            fReal = -0.00001;
    }
    if(fabs(fImage)<0.00001)
    {
        if(fImage>0)
            fImage = 0.00001;
        else
            fImage = -0.00001;
    }

    Alt=sqrt(fReal*fReal+fImage*fImage);		/* 幅值*/

    CurrentAngle = atan(fImage/fReal);

    if(fReal<0)
    {
        CurrentAngle = CurrentAngle +M_PI;
    }
    if(fReal>0&&fImage<0)
    {
        CurrentAngle = CurrentAngle +2*M_PI;
    }

    CurrentAngle -= pDspInfo->fExceedAngle;

    pDspInfo->fReal=Alt*cos(CurrentAngle);
    pDspInfo->fImage=Alt*sin(CurrentAngle);

}

/***********************************************************************
* AngleRotateAlt - 角度旋转
*
* RETURNS: 无
*
*/
void AngleRotateAlt(
    float fAlt,		/* 实部 */
    float fAngle,			/* 虚部 */
    DSPINFO *pDspInfo
)
{
    pDspInfo->fAngle=fAngle-pDspInfo->fExceedAngle/3.1415926*180;
    if(pDspInfo->fAngle<0)
        pDspInfo->fAngle += 360;
}
