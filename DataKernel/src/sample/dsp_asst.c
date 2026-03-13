/* dsptest.c - testing subroutine library for handling algorithms of DSP */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 13aug07, dy first created.
*/

/*
DESCRIPTION
This module includes testing subroutine library for handling algorithms of DSP.
INCLUDES: dsp_asst.h
*/

/* defines */

#define SYSFREQ 50			/* 电力系统频率 */
#define ORIGINCYCLEPOINT 200
#define CYCLEPOINT 24
#define CHNNUM 12
#define FIRLENGTH 45

/* includes */

#include "vxWorks.h"
#include "dsp_asst.h"
#include "math_compat.h"
#include "datetime.h"
#include "realdata.h"

/* globals */

float FIRCoe[FIRLENGTH]= {0.0006,    0.0009,    0.0013,    0.0020,    0.0030,    0.0043,    0.0061,    0.0084,   0.0111,    0.0143,    0.0179,    0.0219,    0.0261,    0.0306,    0.0350,
                          0.0394,    0.0436,    0.0474,    0.0507,    0.0534,    0.0554,    0.0566,    0.0571,    0.0566,    0.0554,    0.0534,    0.0507,    0.0474,    0.0436,    0.0394,
                          0.0350,    0.0306,    0.0261,    0.0219,    0.0179,    0.0143,    0.0111,    0.0084,    0.0061,    0.0043,    0.0030,    0.0020,    0.0013,    0.0009,    0.0006
                         };

int16_t iOriginData[ORIGINCYCLEPOINT];
int16_t iOriginData2[ORIGINCYCLEPOINT];
int16_t iProcessData[CYCLEPOINT];
float fProcessData[CYCLEPOINT];

int LFAC;		/* 插值倍数，一点插两点 */
int NBF;
int InputSampleFreq;
int MFAC;
float ZTD1[CHNNUM][FIRLENGTH];
float fBaseGain;
int N0[CHNNUM];

/* functins */

/***********************************************************************
* RealImageTrAltAngle - 实虚部转幅值相角
*
* RETURNS: 无
*
*/
void RealImageTrAltAngle(
    float fReal,		/* 实部*/
    float fImage,				/* 虚部*/
    float *pfAlt,		/* 幅度*/
    float *pfAngle				/* 相角 */
);

/***********************************************************************
* FIRDigitalFilter - FIR数字滤波器
*
* RETURNS: 无
*
*/
void FIRDigitalFilter(
    uint32_t FilterDots,
    float *pFilterPoleOne,
    float *pFilterPoleTwo,
    float *pFilterOutput
);

/***********************************************************************
* SampleFreqTrRT - 实时插值与抽取
*
* RETURNS:
*               TRUE, 数据有效
*               FALSE, 数据无效
*
*/
BOOL SampleFreqTrRT(
    int16_t fSampleInput,				/* 输入数据 */
    float *pSampleOuput,
    int iChnNum
);

/***********************************************************************
* SampleFreqTr - 插值与抽取
*
* RETURNS: 无
*
*/
BOOL SampleFreqTr(
    float *pSampeInput,
    int iNum,
    float *pSampleOuput
);

/***********************************************************************
* FreqCountDFT - 傅立叶法算频率，用于连续调用
*
* RETURNS: 无
*
*/
void FreqCountDFT(
    float fVal, 		/* 当前输入值 */
    uint8_t uCycleNum				/* 当前周波采样点数 */
)
{
    static BOOL bFstFlag=TRUE;

    if(bFstFlag)
    {
        /* 第一次执行计算系数*/
        bFstFlag=FALSE;
    }
}

/* functions */

/***********************************************************************
* FIRCreate - 生成FIR滤波器
*
* RETURNS: 无
*
*/
void FIRCreate(void)
{
    float PoleOne;
    float PoleTwo;
    int i;

    PoleOne=394;
    PoleTwo=2620;
    FIRDigitalFilter(FIRLENGTH, &PoleOne, &PoleTwo, FIRCoe);

    for(i=0; i<FIRLENGTH; i++)
    {
        printf("%f\n", FIRCoe[i]);
    }
}

/***********************************************************************
* CZData - 插值数据
*
* RETURNS: 无
*
*/
void CZData(void)
{
    int i;
    float fOriginData[ORIGINCYCLEPOINT];

    for(i=0; i<ORIGINCYCLEPOINT; i++)
    {
        /* 给定一个周波的原始数据 */
        iOriginData[i]=32768*sin(2*3.1415926*i/ORIGINCYCLEPOINT);
        fOriginData[i]=(float)iOriginData[i];
        printf("%d	%f\n", 3*i, fOriginData[i]);
        printf("%d	%f\n", 3*i+1, 0.0);
        printf("%d	%f\n", 3*i+2, 0.0);
    }

}

/***********************************************************************
* MultiRateTrans - 抽样率变换
*
* RETURNS: 无
*
*/
void MultiRateTrans(void)
{
    int i;
    float fOriginData[ORIGINCYCLEPOINT];

    for(i=0; i<ORIGINCYCLEPOINT; i++)
    {
        /* 给定一个周波的原始数据 */
        iOriginData[i]=32768*sin(2*3.1415926*i/ORIGINCYCLEPOINT);
        fOriginData[i]=(float)iOriginData[i];
        printf("%d	%f\n", i, fOriginData[i]);
    }

    SampleFreqTr(fOriginData, ORIGINCYCLEPOINT, fProcessData);

    printf("\n\n");
    for(i=0; i<CYCLEPOINT; i++)
    {
        printf("%d	%f\n", i, 32768*fProcessData[i]);
    }
}

/***********************************************************************
* SampleFreqTr - 插值与抽取
*
* RETURNS: 无
*
*/
BOOL SampleFreqTr(
    float *pSampeInput,
    int iNum,
    float *pSampleOuput
)
{
    BOOL bRetCode=FALSE;
    int LFAC=3;		/* 插值倍数，一点插两点 */
    float ZTD1[FIRLENGTH];
    int N0=0;
    int NBF;
    int MFAC;
    int InputSampleFreq;
    float TempVal;
    float TempOutput;
    int INDX,i,j;
    float PoleOne,PoleTwo;

    PoleOne=394;		/* 冲击响应不变法设计FIR，给定两个极点 */
    PoleTwo=2620;
    NBF=FIRLENGTH/LFAC;			/* 需要使用滤波器系数计算的点数 */
    InputSampleFreq = ORIGINCYCLEPOINT*SYSFREQ;								/* 输入数据采样频率 */
    MFAC = (InputSampleFreq*LFAC)/(CYCLEPOINT*SYSFREQ);		/* 抽取倍数 */

    for(i=0; i<FIRLENGTH; i++)
    {
        ZTD1[i]=0;
    }
    for( i=0; i<iNum; i++ )
    {
        TempVal = *pSampeInput++;
        for( j=0; j<NBF-1; j++ )
        {
            /* 输入数据移位 */
            INDX =NBF-j-1;
            ZTD1[INDX] = ZTD1[INDX-1];
        }

        ZTD1[0] = TempVal;

        N0 =N0+LFAC;		/* 越过插零点 */
        if(N0<MFAC)						/* 是否抽取点判断*/
            continue;
        N0=N0-MFAC;

        TempOutput =0;
        for( j =1; j<NBF+1; j++ )
        {
            /* 滤波计算 */
            INDX=j*LFAC-N0;
            TempOutput=TempOutput+FIRCoe[INDX-1]*ZTD1[j-1];
        }
        *pSampleOuput=TempOutput/InputSampleFreq;
        pSampleOuput++;

    }

    bRetCode = TRUE;
    return bRetCode;

}

/***********************************************************************
* SampleFreqTrInit - 插值与抽取初始化
*
* RETURNS: 无
*
*/
void SampleFreqTrInit(void)
{
    int i;
    int j;
    float fTmpData;
    float fOriginData[ORIGINCYCLEPOINT];
    uint32_t ulBackTime;
    uint32_t ulForeTime;
    float fCosCoe[CYCLEPOINT];
    float fSinCoe[CYCLEPOINT];
    float fReal;
    float fImage;
    float fAlt;
    float fAngle;

    TM_Initialize();
    for(i=0; i<ORIGINCYCLEPOINT; i++)
    {
        /* 给定一个周波的原始数据 */
        iOriginData[i]=32768*sin(2*3.1415926*i/ORIGINCYCLEPOINT);
        iOriginData2[i]=32768*sin(2*3.1415926*i/ORIGINCYCLEPOINT);
        fOriginData[i]=(float)iOriginData[i];
        printf("%d	%f\n", i, fOriginData[i]);
    }

    for(i=0; i<CYCLEPOINT; i++)
    {
        fCosCoe[i]=2/(CYCLEPOINT*sqrt(2))*cos(2*3.1414926*i/CYCLEPOINT);
        fSinCoe[i]=2/(CYCLEPOINT*sqrt(2))*sin(2*3.1414926*i/CYCLEPOINT);
    }

    LFAC=3;
    NBF=FIRLENGTH/LFAC;			/* 需要使用滤波器系数计算的点数 */
    InputSampleFreq = ORIGINCYCLEPOINT*SYSFREQ;								/* 输入数据采样频率 */
    MFAC = (InputSampleFreq*LFAC)/(CYCLEPOINT*SYSFREQ);		/* 抽取倍数 */
    fBaseGain=2.69548;			/* 理论上应该是3，滤波器设计的问题，不是理想的 */

    for(i=0; i<CHNNUM; i++)
    {
        for(j=0; j<FIRLENGTH; j++)
            ZTD1[i][j]=0;
    }

    j=0;
    for(i=0; i<ORIGINCYCLEPOINT; i++)
    {
        ulBackTime=TM_Get_usCnt();
        if(SampleFreqTrRT(iOriginData[i], &fTmpData, 0))
        {
            fProcessData[j++]=fTmpData;
            ulForeTime=TM_Get_usCnt();
            printf("delta Time=%d\n", (int)(ulForeTime-ulBackTime));
        }
    }

    printf("\n\n");
    for(i=0; i<CYCLEPOINT; i++)
    {
        printf("%d	%f\n", i, fProcessData[i]);
    }

    fReal=0;
    fImage=0;
    for(i=0; i<CYCLEPOINT; i++)
    {
        fReal=fReal+fSinCoe[i]*fProcessData[i];
        fImage=fImage+fCosCoe[i]*fProcessData[i];
    }

    RealImageTrAltAngle(fReal, fImage, &fAlt, &fAngle);
    printf("fReal=%f fImage=%f fAlt=%f fAngle=%f\n", fReal, fImage, fAlt, fAngle);
}

/***********************************************************************
* SampleFreqTrRT - 实时插值与抽取
*
* RETURNS:
*               TRUE, 数据有效
*               FALSE, 数据无效
*
*/
BOOL SampleFreqTrRT(
    int16_t fSampleInput,				/* 输入数据 */
    float *pSampleOuput,
    int iChnNum
)
{
    float TempVal;
    int j;
    int INDX;
    float TempOutput;

    TempVal = (float)fSampleInput;
    for( j=0; j<NBF-1; j++ )
    {
        /* 输入数据移位 */
        INDX =NBF-j-1;
        ZTD1[iChnNum][INDX] = ZTD1[iChnNum][INDX-1];
    }

    ZTD1[iChnNum][0] = TempVal;

    N0[iChnNum] =N0[iChnNum]+LFAC;		/* 越过插零点 */
    if(N0[iChnNum]<MFAC)						/* 是否抽取点判断*/
    {
        return FALSE;
    }
    N0[iChnNum]=N0[iChnNum]-MFAC;

    TempOutput =0;
    for( j =1; j<NBF+1; j++ )
    {
        /* 滤波计算 */
        INDX=j*LFAC-N0[iChnNum];
        TempOutput=TempOutput+FIRCoe[INDX-1]*ZTD1[iChnNum][j-1];
    }

    *pSampleOuput=TempOutput*fBaseGain;

    return TRUE;
}

/***********************************************************************
* FIRDigitalFilter - FIR数字滤波器
*
* RETURNS: 无
*
*/
void FIRDigitalFilter(
    uint32_t FilterDots,
    float *pFilterPoleOne,
    float *pFilterPoleTwo,
    float *pFilterOutput
)
{
    int i;
    float TrFunGain;
    float PoleOneVal=0.0;
    float PoleTwoVal=0.0;
    float ParaOne;
    float ParaTwo;
    float DT;
    float tempval;

    if((pFilterOutput == NULL) || (pFilterPoleOne == NULL) || (pFilterPoleTwo == NULL))
    {
        return;
    }

    PoleOneVal = (*pFilterPoleOne);
    PoleTwoVal = (*pFilterPoleTwo);
    tempval = (PoleOneVal*PoleOneVal+377*377)*(PoleTwoVal*PoleTwoVal+377*377) ;
    TrFunGain = sqrt(tempval) / (PoleOneVal*PoleTwoVal);
    ParaOne = (TrFunGain*PoleOneVal*PoleTwoVal) / (-PoleOneVal+PoleTwoVal);
    ParaTwo = (TrFunGain*PoleOneVal*PoleTwoVal) / (PoleOneVal-PoleTwoVal);

    for(i=0; i<FilterDots; i++)
    {
        tempval =(float) i;
        DT = tempval/(FilterDots *SYSFREQ);
        *pFilterOutput = ParaOne*exp(-DT*PoleOneVal) + ParaTwo*exp(-DT*PoleTwoVal);
        pFilterOutput++;
    }

    return;
}

/***********************************************************************
* RealImageTrAltAngle - 实虚部转幅值相角
*
* RETURNS: 无
*
*/
void RealImageTrAltAngle(
    float fReal,		/* 实部*/
    float fImage,				/* 虚部*/
    float *pfAlt,		/* 幅度*/
    float *pfAngle				/* 相角 */
)
{
    float fTempAngle;

    if(fabs(fReal)<FLT_PRECISION)
    {
        if(fReal<0)
        {
            fReal=-FLT_PRECISION;
        }
        else
        {
            fReal=FLT_PRECISION;
        }
    }

    if(fabs(fImage)<FLT_PRECISION)
    {
        if(fImage<0)
        {
            fImage=-FLT_PRECISION;
        }
        else
        {
            fImage=FLT_PRECISION;
        }
    }

    fTempAngle=atan(fabs(fImage)/fabs(fReal));

    if(fReal<0)
    {
        if(fImage>0)
        {
            fTempAngle=M_PI-fTempAngle;
        }
        else
        {
            fTempAngle=M_PI+fTempAngle;
        }
    }
    else
    {
        if(fImage<0)
            fTempAngle=2*M_PI-fTempAngle;
    }

    *pfAlt=sqrt(fReal*fReal+fImage*fImage);
    *pfAngle=fTempAngle/(2*M_PI)*360;
}

/***********************************************************************
* atan2Test - atan2 test
*
* RETURNS: 无
*
*/
void atan2Test(void)
{
    float x=sqrt(3);
    float y=1.0;

    printf("atan2 testing result is %d\n", (int)(atan2(x,y)*1000));

}

/***********************************************************************
* DFT - Direct fourier transform
*
* RETURNS: None
*
*/
int DFT(
    int dir,		/* dir=-1: For Forward Transform; dir=1: For Inverse Transform. */
    int m,					/* The dimension of x and y. */
    float *x1,		/* real part of complex array. */
    float *y1								/* image part of complex array. if real number, y1 is 0 always. */
)
{
    int i, k;
    float arg;
    float cosarg, sinarg;
    float *x2=NULL, *y2=NULL;

    x2=malloc(m*sizeof(float));

    if (x2 == NULL)
    {
        return FALSE;
    }

    y2=malloc(m*sizeof(float));

    if (y2 == NULL)
    {
        free(x2);
        return FALSE;
    }

    if (x2 == NULL || y2 == NULL)
        return(FALSE);

    for (i=0; i<m; i++)
    {
        x2[i] = 0;
        y2[i] = 0;
        arg = - dir * 2.0 * 3.141592654 * (float)i / (float)m;
        for (k=0; k<m; k++)
        {
            cosarg = cos(k * arg);
            sinarg = sin(k * arg);
            x2[i] += (x1[k] * cosarg - y1[k] * sinarg);
            y2[i] += (x1[k] * sinarg + y1[k] * cosarg);
        }
    }

    /* Copy the data back */
    if (dir == 1)
    {
        for (i=0; i<m; i++)
        {
            x1[i] = x2[i] / (float)m;
            y1[i] = y2[i] / (float)m;
        }
    }
    else
    {
        for (i=0; i<m; i++)
        {
            x1[i] = x2[i];
            y1[i] = y2[i];
        }
    }

    free(x2);
    free(y2);

    return(TRUE);
}

/***********************************************************************
* DFT2 - 离散傅立叶变换
*
* RETURNS: SUCCESS, or ERROR
*
*/
BOOL DFT2(
    int m,					/* 点数. */
    float *x1,		/* 输入值，同时存储变换结果的实部. */
    float *y1				/* 存储变换结果的虚部. */
)
{
    int i, k;
    float arg;
    float cosarg, sinarg;
    float *x2=NULL, *y2=NULL;

    x2=malloc(m*sizeof(float));		/* 可以定义全局变量 */

    if (x2 == NULL)
    {
        return FALSE;
    }

    y2=malloc(m*sizeof(float));

    if (y2 == NULL)
    {
        free(x2);
        return FALSE;
    }

    if (x2 == NULL || y2 == NULL)
        return(FALSE);

    for (i=0; i<m; i++)
    {
        /* 谐波次数 */
        x2[i] = 0.0;
        y2[i] = 0.0;
        arg = 2.0 * 3.141592654 * (float)i / (float)m;
        for (k=0; k<m; k++)
        {
            /* 点数 */
            cosarg = cos(k * arg);		/* 可以预先计算 */
            sinarg = sin(k * arg);
            x2[i] += 2.0 * x1[k] * cosarg;
            y2[i] += 2.0 * x1[k] * sinarg;
        }
    }

    /* Copy the data back */

    for (i=0; i<m; i++)
    {
        x1[i] = x2[i] / (float)m;
        y1[i] = y2[i] / (float)m;
    }

    free(x2);
    free(y2);

    return(TRUE);
}