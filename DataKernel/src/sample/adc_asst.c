/* adc_asst.c - subroutine library for handling the A/D convertion and the DSP, including simulation */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 25may09, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling the A/D convertion and the DSP, including simulation.
INCLUDES: adc_asst.h
*/

/* includes */

#include "adc_asst.h"
#include "adc.h"
#include "vxWorks.h"
#include <intLib.h>
#include <taskLib.h>

#include "msu.h"

#include "dspai.h"
#include <m8260IntrCtl.h>
// #include <drv/parallel/m8260IOPort.h>
// #include "target.h"
// #include "sbcm8260Cpm.h"
// #include "sbcm8260Siu.h"
#include "io_ctrl.h"

#include <semLib.h>
#include "realdata.h"
#include "filetool.h"	/* 文件操作 */
#include "protectmmiinterface.h"
#include "OPT_Data.h"

/* 合并版所有平台包含 */
#include "POLE_Data.h"
#include "HDL_Data.h"

// #include "config04.h"		/* 所有平台都包含，包括励磁 */

#include "EdpVer.h"		/* 版本控制信息 */
#include "view.h"
#include "VoltageWatch.h"
#include "tickLib.h"
#include "math_compat.h"
#include "edp_asst.h"
#include "bspinterface.h"
#include "intLib.h"

/* globals */

POWERCFG PowerCfg[HCHNNUM/2];						/* 功率计算配置 */
int PowerCfgNum;					/* 功率计算配置数*/

/* statics */

static int iAdcBufTestCount = 0;
static int iAdcBufChnNum = 0;
static SAMPTIMEINTERVALSTAT SampTimeIntervalStat;
static SENDTIMEINTERVALSTAT SendTimeIntervalStat;

/* static functions */

/***********************************************************************
* GetDspEnterCount - 获取DSP任务开始时时间
*
* RETURNS: 无
*
*/
static void GetDspEnterCount(void);

/* 励磁平台使用 */

/* global functions */

/***********************************************************************
* RealImageTrAltAngle - 实虚部转幅值相角
*
* RETURNS: 无
*
*/
extern void RealImageTrAltAngle(
    float fReal,		/* 实部*/
    float fImage,				/* 虚部*/
    float *pfAlt,		/* 幅度*/
    float *pfAngle				/* 相角 */
);

/* functions */

/***********************************************************************
* SetAdcSamp - Setting the ADC Sampling configuration
*
* RETURNS: 无
*
*/
void SetAdcSamp (
    int ChnNum
)
{
    iAdcBufTestCount=0;
    iAdcBufChnNum=ChnNum;
}

/***********************************************************************
* PowerCalInit - 功率计算配置
*
* RETURNS: 无
*
*/
void PowerCalInit(
    u_int uiLgcCh, 		/* 采样的逻辑通道数 */
    DSP_LGC_AI_CFG *plgccfg			/* 指向逻辑通道配置数组第0个元素的指针，数组元素有uiLgcCh个 */
)
{
    int i,j;
    DSP_LGC_AI_CFG *tempLoInfo = plgccfg; 		/* Temporary logic pointer */

    PowerCfgNum = 1;

    /* 功率通道配置 */

    PowerCfg[0].UNum = 5;		/* 物理通道，从1开始*/
    PowerCfg[0].INum = 10;
    PowerCfg[0].fCoff = 1;
    PowerCfg[0].WaveNum = 0;		/* 从0开始 */
    strcpy(PowerCfg[0].PowerName, "PA");

    for(i=0; i<PowerCfgNum; i++)
    {
        for(j=0; j<uiLgcCh; j++)
        {
            /* 找电压逻辑通道 */
            if((tempLoInfo[j].ucHdCh == PowerCfg[i].UNum)
                    &&((tempLoInfo[j].ucUnit == 0x14) || (tempLoInfo[j].ucUnit == 0x17))
                    &&(tempLoInfo[j].ucFiltNum == 0))
            {
                PowerCfg[i].UNumLog = j;
                break;
            }
        }
        assert(j<uiLgcCh);

        for(j=0; j<uiLgcCh; j++)
        {
            /* 找电流逻辑通道 */
            if((tempLoInfo[j].ucHdCh == PowerCfg[i].INum)
                    &&((tempLoInfo[j].ucUnit == 0x8) || (tempLoInfo[j].ucUnit == 0xB))
                    &&(tempLoInfo[j].ucFiltNum == 0))
            {
                PowerCfg[i].INumLog = j;
                break;
            }
        }
        assert(j<uiLgcCh);

    }
}

/***********************************************************************
* AdcDataRecAnalyse - 采样数据分析
*
* RETURNS: 无
*
*/
void AdcDataRecAnalyse(void)
{
    ADCDATAANALYSE *pHeadAdcDataAnalyse;		/* 头指针 */
    ADCDATAANALYSE *pTailAdcDataAnalyse;						/* 尾指针 */
    ADCDATAANALYSE *pWork;		/* 当前指针 */
    int i;
    int j;
    BOOL BeginFlag=TRUE;		/* 开始标志 */
    BOOL SearchSuccessFlag=FALSE;				/* 是否找到标志 */
    int32_t iMax;
    int32_t iMin;
    float fExcur=0;
    float fMax;
    float fMin;
    float fr[TESTSEARCHRANGE];
    float fi[TESTSEARCHRANGE];
    float fAlt;
    float fAng;
    float fMaxAlt = 0.0;
    float fMinAlt = 0.0;
    float fAltTotal=0.0;
    int ilShowDtNn;		/* 显示数据数 */
    FILE *fp;

    fp = fopen (EP_ADC_DATA_FILE, "w");
    if (fp == NULL)
        LOG_Dbg_Msg ("WARNING: could not open output file '%s'", (int)EP_ADC_DATA_FILE, 0, 0, 0, 0, 0);

    pHeadAdcDataAnalyse=calloc(1, sizeof(ADCDATAANALYSE));	/* 申请内存 */
    assert(pHeadAdcDataAnalyse);
    pTailAdcDataAnalyse=pHeadAdcDataAnalyse;
    pTailAdcDataAnalyse->pNext=NULL;	/* 末尾 */

    pHeadAdcDataAnalyse->Count=0;
    printf("****************采样点分析程序****************\n\n");
    printf("各物理通道当前采样值\n");
    for(i=0; i<iAdcChipNum_g; i++)
    {
        for (j = 0; j<g_iAdcChnNumPerChip; j++)
        {
            printf("%d    ----    %ld\n", i*g_iAdcChnNumPerChip+j, DspResult.pAdcData[i*g_iAdcChnNumPerChip+j]);
        }
    }
    printf("单通道分析点数为%d点，当前分析物理通道号为%d\n", TESTSEARCHRANGE, SampDataShow.uChnNum);
    printf("********采样点分布(采样值<-->点数)********\n");
    for(i=0; i<TESTSEARCHRANGE; i++)
    {
        if (fp)
            fprintf (fp, "%d %ld\n", i, SampDataShow.iSamData[i]);

        SearchSuccessFlag=FALSE;
        if(BeginFlag)
        {
            pHeadAdcDataAnalyse->Num=SampDataShow.iSamData[i];
            pHeadAdcDataAnalyse->Count++;
            BeginFlag=FALSE;
        }
        else
        {
            /* 不是第一次 */
            pWork=pHeadAdcDataAnalyse;
            do
            {
                if(SampDataShow.iSamData[i] == pWork->Num)
                {
                    /* 已经存在 */
                    pWork->Count++;
                    SearchSuccessFlag=TRUE;
                    break;
                }
                else
                {
                    pWork=pWork->pNext;		/* 指向下一个*/
                }
            }
            while(pWork);

            if(!SearchSuccessFlag)
            {
                /* 找不到，新建立接点 */
                pWork=calloc(1, sizeof(ADCDATAANALYSE));
                assert(pWork);
                pWork->Num=SampDataShow.iSamData[i];
                pWork->Count=0;
                pWork->Count++;
                pTailAdcDataAnalyse->pNext=pWork;
                pTailAdcDataAnalyse=pWork;
                pWork->pNext=NULL;		/* 末尾 */
            }
        }
    }

    if (fp)
        fclose (fp);

    pWork=pHeadAdcDataAnalyse;
    do
    {
        printf("%d	%d\n", pWork->Num, pWork->Count);
        pWork=pWork->pNext;
    }
    while(pWork);

    pWork=pHeadAdcDataAnalyse;
    iMax=pWork->Num;
    iMin=pWork->Num;
    do
    {
        if(iMax<pWork->Num)
        {
            iMax=pWork->Num;
        }
        if(iMin>pWork->Num)
        {
            iMin=pWork->Num;
        }

        fExcur+=pWork->Num*pWork->Count;
        pWork=pWork->pNext;
    }
    while(pWork);

    printf("********漂移范围(LSB)********\n");
    printf("%d\n", abs(iMax-iMin)+1);

    printf("********最大值、最小值及平均值(单位 mV)********\n");

    fMax=(float)iMax*5.0*1000.0/32768;
    fMin=(float)iMin*5.0*1000.0/32768;
    fExcur=(fExcur*5*1000.0)/(TESTSEARCHRANGE*32768);

    printf("%f	%f	%f\n", fMax, fMin, fExcur);
    printf("********周波傅立叶分析********\n");
    printf("周波%d点，显示数据点数%d\n", SamplingNum_g, ilShowDtNn=(2*SamplingNum_g <= TESTSEARCHRANGE) ? (2*SamplingNum_g) : TESTSEARCHRANGE);

    printf("点号  数字量  电压值/mV\n");
    for(i=0; i<ilShowDtNn; i++)
    {
        printf("%d  %ld  %f\n", i, SampDataShow.iSamData[i], SampDataShow.iSamData[i]*1000.0*5.0/32768.0);
    }

    for(i=0; i<SamplingNum_g/2; i++)
    {
        fr[i]=0.0;
        fi[i]=0.0;
    }

    printf("分析结果(幅值(单位mV)<-->相角(单位°)<-->频率(单位Hz)\n");
    for(i=0; i<SamplingNum_g/2; i++)
    {
        for(j=0; j<SamplingNum_g; j++)
        {
            fr[i]=fr[i]+SampDataShow.iSamData[j]*2.0/(SamplingNum_g)*cos(2*3.1415926*i*j/SamplingNum_g);
            fi[i]=fi[i]+SampDataShow.iSamData[j]*2.0/(SamplingNum_g)*sin(2*3.1415926*i*j/SamplingNum_g);
        }
        fr[i]=fr[i]*5.0*1000/(32768*sqrt(2));
        fi[i]=fi[i]*5.0*1000/(32768*sqrt(2));
        RealImageTrAltAngle(fr[i], fi[i], &fAlt, &fAng);
        printf("%d	%f	%f	%f\n", i, fAlt, fAng, 50.0*i);
    }

    for(i=SamplingNum_g; i<=TESTSEARCHRANGE-SamplingNum_g; i++)
    {
        fr[0]=0.0;
        fi[0]=0.0;
        for(j=0; j<SamplingNum_g; j++)
        {
            fr[0]=fr[0]+SampDataShow.iSamData[i+j]*2.0/(SamplingNum_g)*cos(2*3.1415926*j/SamplingNum_g);
            fi[0]=fi[0]+SampDataShow.iSamData[i+j]*2.0/(SamplingNum_g)*sin(2*3.1415926*j/SamplingNum_g);
        }
        fr[0]=fr[0]*5.0*1000/(32768*sqrt(2));
        fi[0]=fi[0]*5.0*1000/(32768*sqrt(2));
        RealImageTrAltAngle(fr[0], fi[0], &fAlt, &fAng);
        if(i == SamplingNum_g)
        {
            fMaxAlt=fAlt;
            fMinAlt=fAlt;
        }
        else
        {
            if(fMaxAlt<fAlt)
            {
                fMaxAlt=fAlt;
            }
            if(fMinAlt>fAlt)
            {
                fMinAlt=fAlt;
            }
        }
        fAltTotal+=fAlt;
    }
    fAltTotal=fAltTotal/(TESTSEARCHRANGE-SamplingNum_g-SamplingNum_g+1);
    printf("基波幅度测量\n");
    printf("最大值%f\n最小值%f\n平均值%f\n", fMaxAlt, fMinAlt, fAltTotal);

    free(pHeadAdcDataAnalyse);
}

/***********************************************************************
* AdcDataRecAnalyse - 采样数据分析
*
* RETURNS: 无
*
*/
void DataAnalyseTaskStart(void)
{
    int nTaskDataAnalyse;

    nTaskDataAnalyse=taskSpawn("tDataAnalyse",
                               TSK_PRI_DST_TEST,
                               VX_FP_TASK|VX_DEALLOC_STACK,
                               10000,
                               (FUNCPTR)AdcDataRecAnalyse,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    assert(nTaskDataAnalyse != ERROR);
}

/***********************************************************************
* SimulWaveData - 使用仿真数据
*
* RETURNS: 无
*
*/
void SimulWaveData(
    uint8_t ChnNum,		/* 物理通道号 */
    uint16_t usTimes,		/* 仿真次数 */
    int32_t iAlt						/* 幅度，数字量 */
)
{
    int i;
    int iLockKey;

    iLockKey = intLock();		/* 闭锁中断 */
    SampErrorSimul.ChnNum=ChnNum;
    SampErrorSimul.bUseSimulData=TRUE;
    SampErrorSimul.usBreakPointCount=2*SamplingNum_g;
    SampErrorSimul.usTimes=usTimes;

    for(i=0; i<2*SamplingNum_g; i++)
    {
        SampErrorSimul.iWaveData[i]=(int32_t)iAlt*sin((2*M_PI*i)/SamplingNum_g);
    }

    intUnlock(iLockKey);
}

/***********************************************************************
* SampBreakOnePoint - 突变一点
*
* RETURNS: 无
*
*/
void SampBreakOnePoint(
    uint8_t ChnNum,		/* 物理通道号 */
    uint16_t usNum
)
{
    SampErrorSimul.ChnNum=ChnNum;
    SampErrorSimul.BreakOnePointFlag=TRUE;
    SampErrorSimul.usBreakPointCount=usNum;
}

/***********************************************************************
* SampInsertZero - 插零
*
* RETURNS: 无
*
*/
void SampInsertZero(
    uint8_t ChnNum,		/* 物理通道号 */
    uint16_t ZeroPointNum			/* 零点个数 */
)
{
    SampErrorSimul.ChnNum=ChnNum;
    SampErrorSimul.InsertZeroFlag=TRUE;
    SampErrorSimul.InsertZeroCount=ZeroPointNum;
}

/***********************************************************************
* SampStop - 停止采样
*
* RETURNS: 无
*
*/
void SampStop(
    uint16_t StopPointNum			/* 停止采样点数 */
)
{
    SampErrorSimul.StopFlag=TRUE;
    SampErrorSimul.StopCount=StopPointNum;
}

/***********************************************************************
* SetHighestIntLevel - 设置最高优先级任务
*
* RETURNS: 无
*
*/
void SetHighestIntLevel(void)
{
    // *SICR(0xf0000000) |= 0x0d00;
}

/***********************************************************************
* GetSendInterval - 获取发送间隔
*
* RETURNS: 无
*
*/
void GetSendInterval(void)
{
    // static BOOL bFstFlag=TRUE;
    // static uint32_t ulCnt=0;

    // SendTimeIntervalStat.ulForCount=SendTimeIntervalStat.ulCurCount;
    // SendTimeIntervalStat.ulTimerBackCount=SendTimeIntervalStat.ulTimerForeCount;
    // if(bFstFlag)
    // {
    //     bFstFlag=FALSE;
    //     SendTimeIntervalStat.ulCurCount=TM_Get_usCnt();
    //     SendTimeIntervalStat.ulForCount=0;
    //     SendTimeIntervalStat.lDif=SendTimeIntervalStat.ulCurCount-SendTimeIntervalStat.ulForCount;
    //     SendTimeIntervalStat.lMin=SendTimeIntervalStat.lDif;
    //     SendTimeIntervalStat.lLowCout=0;
    // }
    // else
    // {
    //     SendTimeIntervalStat.ulCurCount=TM_Get_usCnt();
    //     SendTimeIntervalStat.lDif=SendTimeIntervalStat.ulCurCount-SendTimeIntervalStat.ulForCount;
    // }

    // LOG_Dbg_Msg("Time=%dus\n", TM_Get_usCnt(), 0, 0, 0, 0, 0);

    // if(SendTimeIntervalStat.lMin>SendTimeIntervalStat.lDif)
    // {
    //     SendTimeIntervalStat.lMin=SendTimeIntervalStat.lDif;
    // }

    // if(SendTimeIntervalStat.lDif<408)
    // {
    //     logMsg("ulCurCount=%d ulForCount=%d lDif=%d\n",
    //            SendTimeIntervalStat.ulCurCount,
    //            SendTimeIntervalStat.ulForCount,
    //            SendTimeIntervalStat.lDif, 0, 0, 0);
    //     SendTimeIntervalStat.ulBackErrorTimerCount=SendTimeIntervalStat.ulTimerForeCount;
    //     SendTimeIntervalStat.ulForeErrorTimerCount=*TCN2(0xF0000000);
    //     SendTimeIntervalStat.lLowCout++;
    // }

    // SendTimeIntervalStat.ulTimerForeCount=*TCN2(0xF0000000);

    // ulCnt++;
    // if(ulCnt%2400 == 1)
    // {
    //     logMsg("ulCurCount=%d ulForCount=%d SendTimeIntervalStat.lDif=%d\n",
    //            SendTimeIntervalStat.ulCurCount,
    //            SendTimeIntervalStat.ulForCount,
    //            SendTimeIntervalStat.lDif, 0, 0, 0);
    // }
}

/***********************************************************************
* GetDspEnterCount - 获取DSP任务开始时时间
*
* RETURNS: 无
*
*/
static void GetDspEnterCount(void)
{
    // SampTimeIntervalStat.ulDspEnterCount=(*TCN2(0xF0000000)*16*1000)/(sysInputFreq_g/1000000);
}

/***********************************************************************
* AdcDataRec - 单通道录波
*
* RETURNS: 无
*
*/
void AdcDataRec(
    int32_t *pAdcData
)
{
    if(SampDataShow.EnableFlag)
    {
        /* 允许录波 */
        SampDataShow.iSamData[SampDataShow.ulCnt]=pAdcData[SampDataShow.uChnNum];		/* 缓冲 */
        SampDataShow.ulCnt++;
        if(SampDataShow.ulCnt == TESTSEARCHRANGE)
        {
            SampDataShow.ulCnt=0;
            SampDataShow.EnableFlag=FALSE;
        }
    }
}

/***********************************************************************
* SetAdcDataRec - 设定单通道录波
*
* RETURNS: 无
*
*/
void SetAdcDataRec(uint8_t nChnNum)
{
    if(!SampDataShow.EnableFlag)
    {
        /* 目前不在录波 */
        SampDataShow.EnableFlag=TRUE;
        SampDataShow.uChnNum=nChnNum;
    }
}

/***********************************************************************
* CancelAdcDataRec - 取消单通道录波
*
* RETURNS: 无
*
*/
void CancelAdcDataRec(void)
{
    SampDataShow.EnableFlag=FALSE;
    SampDataShow.ulCnt=0;
}

/***********************************************************************
* ShowAdcDataRec - 显示单通道录波结果
*
* RETURNS: 无
*
*/
void ShowAdcDataRec(uint8_t nChnNum)
{
    int i;

    for(i=0; i<TESTSEARCHRANGE; i++)
    {
        printf("%d	%ld\n", i, SampDataShow.iSamData[i]);
        logMsg("%d	%ld\n", i, SampDataShow.iSamData[i], 0, 0, 0, 0);
    }
}

/***********************************************************************
* SysDriveDisable - 系统驱动禁止
*
* RETURNS: 无
*
*/
void SysDriveDisable(void)
{
    // intDisable(INUM_TIMER2);		/* Disable timer2 interrupt */
}

/***********************************************************************
* SysDriveEnable - 系统驱动启动
*
* RETURNS: 无
*
*/
void SysDriveEnable(void)
{
    // intEnable(INUM_TIMER2);		/* Enable timer2 interrupt */
}

/***********************************************************************
* SetCalibrateZero - 零点校准
*
* RETURNS: 无
*
*/
EP_STATUS SetCalibrateZero(int VISel)
{
#ifdef ZEROENABLE
    int i;

    ZeroCalibrateFlag = TRUE;

    while(DspInfo.ZeroBufFullFlag != 1)
    {
        /* 等待零点计算缓冲写满 */
        taskDelay(1);
    }
    for(i=0; i<LogicChnNumber; i++)
    {
        ZeroCalibrate(i);			/* 零漂计算 */
    }

    ZeroCalibrateFlag = FALSE;	/* 校准结束 */

    return EP_SUCCESS;
#else
    return EP_SUCCESS;
#endif
}

/***********************************************************************
* MsuAngleCalibrateEnable - 允许测量角度校准
*
* RETURNS: 无
*
*/
void MsuAngleCalibrateEnable(void)
{
    MusAngleCalibrateEnableFlag = TRUE;
}

/***********************************************************************
* MsuPowerCalibrateEnable - 允许测量功率校准
*
* RETURNS: 无
*
*/
void MsuPowerCalibrateEnable(void)
{
    MusPowerCalibrateEnableFlag = TRUE;
}
