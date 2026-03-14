/* adc.c - subroutine library for handling the A/D convertion and the DSP */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02b, 21apr08, dy the creating of slow message processing task is moved.
02a, 16apr08, dy merge the edp platform.
01c, 16sep07, dy add digital sampling throuth Ethernet.
01b, 29jul06, dy add calibration methods for measuring.
01a, 27dec05, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling the A/D convertion and the DSP.
INCLUDES: adc.h
*/

/* includes */

#include "vxWorks.h"
#include <intLib.h>
#include <taskLib.h>
#include "adc.h"

#include "msu.h"

#include "dspai.h"
// #include <drv/intrCtl/m8260IntrCtl.h>
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
#include <intLib.h>

/* 合并版所有平台包含 */
#include "POLE_Data.h"
#include "HDL_Data.h"
#include "time_compat.h"
// #include "config04.h"		/* 所有平台都包含，包括励磁 */

#include "EdpVer.h"		/* 版本控制信息 */
#include "view.h"
#include "VoltageWatch.h"
#include "tickLib.h"
#include "math_compat.h"
#include "edp_asst.h"
#include "bspinterface.h"
#include "errtest.h"
// #include "goose_eth.h"
#include "smvcfg.h"
#include "smv_rx.h"
#include "eth_callback.h"
#include "Smv_Go_CommStat_File.h"
#include "OPT_SamSyn.h" /*2013-5-20 ZY */
#include "m8260IntrCtl.h"//中断向量号的头文件
#include "semLib.h"

/* defines */

#define LOCAL_DELAY_NUM 4  /* 除通信外总延迟点数 */
#define DCU_DELAY_TIME 4  /* s */


/*扩展机箱锁相同步相关定义　2011-4-14  ZY */
#define SYN_POLL_INTERVAL 1000000  /* 同步信号自检周期,us */
#define SYN_FST_POLL_INTERVAL (4*60*1000000)   /* 第一次同步信号检测等待时间,us */
#define SYN_INTERVAL 200000  /* 同步信号发送间隔,us */
#define SYN_STAT_SIGN_CNT  10  /*  *统计平滑同步信号次数*/
#define SYN_EXCUR_RANGE 300      /* 同步波动范围,us */

#define PERIOD_PER_FRAME 100  /* 最快平均报文处理周期 */
#define MAX_FRM_PER_POLL 25 /* 每次查询帧数 */
#define MAX_BYTE_PER_POOL (4400) /* 小于MAX_FRM_PER_POLL*177 */

#define SIGMA_DELTA 14009

#define FACTOR_TWO (((1 << 17)-SIGMA_DELTA) >> 2)
#define FACTOR_ONE (((1 << 16)-SIGMA_DELTA) >> 2)

SEM_ID semQueueSD;			/* A/D转换信号量 */

/* typedefs */

/*　2011-4-14  ZY */
typedef   struct
{
    uint32_t  ulSynIntvlBuf[SYN_STAT_SIGN_CNT];/*最近同步信号间隔缓冲，us  */
    int     iNewSynCnt;  /*从上次统计到现在的同步信号次数  */
}  SYN_INTVL_BUF_INF_TYPE;   /*扩展机箱的同步信号间隔缓冲信息 */

/* typedefs */

typedef struct TRRCNTPR_tag		/* Timer reference value */
{
    BOOL bAdjFlag;		/* Adjusting flag. */
    BOOL bInitAdjFlag;			/* The first adjusting. */
    float fStVal;				/* Float point setting value. */
    uint16_t usStVal;		/* Setting value. */
    uint16_t usValRes;				/* Residual digital value in a cycle. */
    uint16_t usValResFrHlf;	 /* Residual digital value in front half cycle. */
    uint16_t usValResFrDesHlf;	 /* Residual degressive digital value in front half cycle. */
    uint16_t usValResBkHlf;	  /* Residual digital value in back half cycle. */
    BOOL bResetFlag;	 /* flag for reseting the timer reference value. */
    uint16_t usValResBkDesHlf;	  /* Residual degressive digital value in back half cycle. */
    BOOL bFrHlfCyl;				/* Front flag. */
    BOOL bBkHlfCyl;				/* Back half flag. */

    uint16_t usObjTrrCnt;	/* Object setting value. */
    uint16_t usLstTrrCnt;		/* Last setting value. */
    uint16_t usAdjDifCnt;			/* Adjust digital count in a point. */
    uint16_t usAdjTimes;		/* Adjust times. */
    uint32_t ulCalCnt;		/* rounding accumulating value. */
    uint32_t ulStpCnt;				/* rounding accumulating value every sampling. */
    BOOL bFreqSetFlag;		    /* setting frequency flag. */
} TRRCNTPR, *pTRRCNTPR;

/* globals */

extern BOOL bStopRefreshData;


void *pvAiModHandle_g; 					/* Handle */
int TiIMMR_g;
uint32_t ulDspAccessCounter_g = 0; 			/* DSP任务监视访问计数器 */

uint16_t usMinInterval = 0xFFFF;  /* 扫描任务最小间隔点数 */

int nDSPTaskID_g=-1;			/* 赋初始值*/
BOOL bDspTaskStartFlag_g=FALSE; 			/* DSP计算任务启动标志 */
BOOL bMsuTaskStartFlag_g=FALSE; 				/* 测量计算任务启动标志 */
BOOL bZero_ExcurTaskStartFlag_g = FALSE;   /* 零漂计算任务启动标志 */

ADCSAMPINFO AdcSampInfo;
DSPHANDLE DspHandle;

SEM_ID NewDspData; 			/* 新数据到来信号灯 */
SEM_ID NewDspDataforZero;
SEM_ID NewDspDataforMsu;

/* 励磁平台使用 */
BOOL FPGAInitState = TRUE;

SAMDATASHOW SampDataShow;

int32_t AdcData[MAXHCHNNUM];
uint32_t ulTempData[4][6];
float Adc_Modu;

/* 励磁平台使用 */
uint32_t numtest; 			/* 测试计数 */
uint32_t ticknum;
uint32_t ulDspAccessCounter=0; 	/* DSP任务监视访问计数器 */

SAMPERRORSIMUL SampErrorSimul;
SqQueue SampDataQ;
AsCur DspAsCur;

/* 对侧为数字化采样延迟1点，比传统采样延时长0.2ms
 */
#define ADSMP_SHIFT 1
#define RPT_MATCH_DATA_LEN 16  /* 报文匹配数据最大长度 */

extern SMV_CC_INFO sSmvCCInfo;

/* 缓冲与中断缓冲一致 */
INT32 smp_data[MAXQSIZESAMPDATA+1][MAXHCHNNUM];
UINT16 input_Index=0;
UINT16 synout_index_second = 0;   /* 子单元秒计数器用 */

int iAdcChipNum_g=3;      /* CPU板上AD芯片个数 */
int g_iAdcChnNumPerChip = CHIPCHNNUM;  /* 每个ADC芯片通道数, 缺省为6 */

int EnableCalibrateFlag_g;
BOOL CalibrateEnableFlag = FALSE;		/* 校准允许 */
BOOL MusAngleCalibrateEnableFlag = FALSE;	 /* 角度校准允许 */
BOOL MusPowerCalibrateEnableFlag = FALSE;		 /* 功率校准允许 */

BOOL bExtBoxCoffUpdate = FALSE; /* 扩展机箱系数更新标志 */
uint16_t usCoffUpdateCount;  /* 系数更新时计数,与bExtBoxCoffUpdate变量配合使用 */

/*为扩展机箱启动，移植至此*/
INT32 send_data[MAXQSIZESAMPDATA+1][MAXCHNELS];
uint32_t send_data_sts[MAXQSIZESAMPDATA+1][MAXCHNELS];

/* 扩展机箱同步相关变量 */
BOOL bFstSynFlag=TRUE;  /* 首次同步控制标志 */
BOOL bSynSignalIsCome_g = FALSE;  /* 同步信号是否到达 */
BOOL bFstSynSignalHaveCome_g = FALSE;    /* 初次同步信号是否已到达 */
uint32_t tmExtInitTime_g = 0;   /* 初始运行时间,us */
BOOL bSynInterruptFlag = FALSE;     /* 同步信号中断标志 */
BOOL bSynNotComeFlag = FALSE;  /* 初始化时同步信号长时间未到达标志 */
BOOL bNotDrvDspFlag = FALSE;     /* 不驱动DSP任务标志 */
BOOL bSynInvalid = FALSE;   /* 同步信号无效标志 */

/*为锁相同步添加  */
SYN_INTVL_BUF_INF_TYPE    synIntvlBufInf_g;    /* 同步信号缓冲信息　，2011-4-14  ZY*/
uint32_t  ulTRR2Base_g;          /*对EDP01的Timer2的基准大小,用于扩展机箱锁相同步使用  */
BOOL   bFstPhsFollowSucc_g=FALSE; /* 第1次锁相成功标志 */
BOOL   bFstSynFlagAfterPhsFollow_g=FALSE;  /*锁相成功后的第1次同步信号到达标志  */

/* 存储通道所属端口 */
uint8_t Sam_Chn_Source[HCHNNUM] = {255};
BOOL arrChnUsedFlag[HCHNNUM]; /* 该通道是否在工程配置文件中配置,如果不配置则不处理 */

/* 是否指针传递模式, 缺省为值传递模式 */
BOOL bDataTransMod = FALSE;

/* SV报文统计匹配数据 */
uint8_t ucArrMatchData[RPT_MATCH_DATA_LEN] = {0x01, 0x0C, 0xCD, 0x04};

void Write_FPGA_Program_Auto(void);
void init_Smv92STD_SubSend(int smvNo,int bufNo);
void iec_smv92STD_Subsend(int smvNo);
/* locals */

#ifdef ZEROENABLE
static BOOL ZeroCalibrateFlag = FALSE;
#endif

static BOOL bSampDataShowFlag=FALSE;			/* 采样数据显示标志 */

/* 扩展机箱使用 */
BOOL bDspFirstReadAiFlag_g=FALSE;

static BOOL bFirstEnterNotCreateFlag_g=FALSE;
static BOOL bFirstEnterNotFreeSemFlag_g=FALSE;

static int nMsuTaskID_g;
static int nZero_ExcurTaskID_g;
static SAMPINFO SampInfo;
static TRRCNTPR TrrCnt;			/* Timer reference value */
static uint32_t ulDCUDelaySndCnt;  /* DCU发送计数 */
static int8_t SamCountCnt = 0;  /* 中断数据采集次数计数 */

BOOL bFirstTime = TRUE;    /* 初次取点 */
static uint32_t ulCnttt=0;  /* 同步计数 */


/* 缺省数据和状态 */
static int32_t arrDefaultVal[HCHNNUM];
static uint32_t arrDefaultSts[HCHNNUM];

extern INT16 SmplCntLocal;
extern BOOL bArrPoleFlag[MAXCHNELS];  /* 极性 */

UINT32 nSynCunts=0;
UINT8 nSmv92Data1[4][1500];//传统子单元发送数据缓冲
UINT8 nAddatData1=57;//传统子单元发送数据缓冲
UINT16 nAddatData92[4][10];//传统子单元发数据地址
UINT16 nAddatCnt92[4][10];//传统子单元发送计数器地址
UINT16 nAddatSyn92[4][10];//传统子单元发送同步位地址

extern IEC_SMV_CFG    gSmvCfg;
int16_t *tempaddr=NULL;
BOOL bEnableWrDspBuf_g = FALSE;  /* 允许填充采样数据缓冲标志 */
extern BOOL bMUDelaySndFlag;

/* GOOSE状态查询周期 */
uint32_t ulGsStsQueryPeriod;		/*daibixiang delete extern*/
uint32_t ulUpdateGsCnt; /* 周期更新GOOSE计数 */

/* 查询定值更新计数 */

uint32_t g_ulPollSetChgThreshold = 0;
uint32_t g_ulDspPollSetChgThreshold = 0;
uint32_t g_ulPollSetChgCnt = 0;

/* 适应扫描任务0驱动 */
DSPINFO *pDspInfo = &DspInfo;
ADCSAMPINFO *pAdcSampInfo = &AdcSampInfo;
DSPRESULT *pDspResult = &DspResult;
DSPHANDLE *pDspHandle = &DspHandle;

/***********************************************************************
* SysSynInit - 系统同步初始化
*
* RETURNS: 无
*
*/
extern void SysSynInit(void);

/* 获取扫描任务最小扫描周期点数.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern uint16_t SC_GetTaskMinPeriod (void);

/* 是否配置光差通道
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL RD_ifOptCfg(void);

/* static functions */

/***********************************************************************
* Timer2_Hw_Init - This function is used to initialize Timer2 hardware environment
*
* RETURNS: 无
*
*/
static void Timer2_Hw_Init (void);

/***********************************************************************
* DSPProcess - 预处理计算任务入口函数
*
* RETURNS: 无
*
*/
static void DSPProcess();

/***********************************************************************
* AdcAdjTrr - Adjust the ADC interval
*
* RETURNS: NONE
*
*/
static void AdcAdjTrr(
    uint16_t usTimes	/* 周波计数 */
);

/***********************************************************************
* Timer2_ISR - Timer2 interrupt routine
*
* RETURNS: 无
*
*/
static void Timer2_ISR (void);

static void Timer2_ISR_01 (void);
static void Timer2_ISR_02 (void);
static void Timer2_ISR_06 (void);
static void Timer2_ISR_14 (void); /* 查询FPGA数据 */

/***********************************************************************
* IO_Set_PC28 - 设置PC28功能
*
* RETURNS: 无
*
*/
static int32_t IO_Set_PC28();

/***********************************************************************
* IO_Set_PC11 - 设置PC11功能
*
* RETURNS: 无
*
*/
static int32_t IO_Set_PC11();

/***********************************************************************
* AdcDataRd - 采样数据读取
*
* RETURNS: 无
*
*/
static void AdcDataRd(
    int32_t *pData		/* 采样数据保存 */
);

/***********************************************************************
* IO_Set_PC13 - 设置PC13功能
*
* RETURNS: 无
*
*/
static int32_t IO_Set_PC13();



/***********************************************************************
* SynMasterExtInit - 同步主从机箱初始化
*
* RETURNS: 无
*
*/
static void SynMasterExtInit(void);

/***********************************************************************
* SynMasterExtOut - 同步主从机箱初始化
*
* RETURNS: 无
*
*/
static void SynMasterExtOut(void);

/***********************************************************************
* IO_Set_PC24 - 设置PC24功能
*
* RETURNS: 无
*
*/
static int32_t IO_Set_PC24();

/***********************************************************************
* IO_Set_PA9 - 设置PA9功能
*
* RETURNS: 无
*
*/
static int32_t IO_Set_PA9();

/* refresh the DC data.
 * Para:
 *     dcdata, address for filling in.
 *     dc_db_data, double buffer.
 * Return:
 *     NONE.
 */
static void RD_Refresh_DC_Data(void *pvAiMod, DSP_LGC_DC_AI_CFG *pdspl_dc_cfg_g, float *dcdata, float *dc_db_data);

/* global functions */

/* Get the origin sampling data.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void GetOriginSampData(void);

/* fill in the buffer of virtual box AO from AI, called in interrupt.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void WrVirtBoxBuf(void);

/* fill in the buffer of virtual box AO from processing variables, called in task.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void WrVirtBoxPreBuf(void);

/* 旧的BSP版本提供的函数 */

/*  Function: output AD convert signal high
    Parameter: status;  TRUE;High, FALSE; low
*/
extern void    AD_Convert_High(BOOL status);

/* 保护模块提供的检测保护任务挂起的函数，用于调试异常 */
extern BOOL RE_RelayTaskIsSuspend();

/***********************************************************************
* AdcDataRec - 单通道录波
*
* RETURNS: 无
*
*/
extern void AdcDataRec(
    int32_t *pAdcData
);


/* 向DCU发送延时信息
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void TMuDelay_Transmit(void);

/* 设置扩展机箱发送标志
 * Para:
 *     iVal, 设置值.
 * Return:
 *     NONE.
 */
extern void SetExtSndFlag(int iVal);

/* 获取定值更新状态.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL RE_GetLogSetChgSts(void);

/* functions */

STATUS InitQueueSD(
    SqQueue *Q
)
{
    int i;
    int j;

    Q->base=(SampDataCur *)malloc(MAXQSIZESAMPDATA*sizeof(SampDataCur));
    if(!Q->base)
    {
        return ERROR;
    }

    Q->front=Q->rear=0;

    pAdc_Data=(Q->base+Q->front)->Data;		/* 处理数据的初始化地址 */

    for(i=0; i<INTERVALTIMES; i++)
    {
        DspAsCur.pAdc_Data_Bak[i]=(Q->base+Q->front)->Data;
    }

    /* 状态显式赋0 */
    for (i = 0; i<MAXQSIZESAMPDATA; i++)
    {
        for (j = 0; j<MAXHCHNNUM; j++)
        {
            (Q->base+i)->Status[j] = 0;
            (Q->base+i)->Data[j] = 0;
        }
    }
    
    semQueueSD = semBCreate(SEM_Q_PRIORITY, SEM_FULL);	/* 创建信号量 */
    if (semQueueSD == NULL)
    {
        free(Q->base);
        return ERROR;
    }

    /* 测试信号量是否可以立即获取 */
    if (semTake(semQueueSD, NO_WAIT) == OK)
    {
        printf("DEBUG: 信号量创建后可以立即获取\n");
        semGive(semQueueSD);  /* 立即释放用于正常使用 */
    }
    else
    {
        printf("DEBUG: 信号量创建后无法立即获取\n");
    }

    return OK;
}

/***********************************************************************
* DeleteQueueSD - 删除队列所占用内存
*
* RETURNS: NONE
*
*/
void DeleteQueueSD(void)
{
    if (semQueueSD != NULL)
    {
        semDelete(semQueueSD);  /* 删除信号量 */
        semQueueSD = NULL;
    }
    free(SampDataQ.base);
}

/***********************************************************************
* QueueLength - 获取队列当前数据长度
*
* RETURNS: 长度
*
*/
int QueueLength(
    SqQueue Q
)
{
    return (Q.rear-Q.front+MAXQSIZESAMPDATA)%MAXQSIZESAMPDATA;
}

/***********************************************************************
* ShowQ - 显示队列中的数据
*
* RETURNS: OK, ERROR
*
*/
void ShowQ(
    uint8_t nChnNum	/* 通道号 */
)
{
    int i;

    for(i=0; i<MAXHCHNNUM; i++)
    {
        LOG_Dbg_Msg("%d=%ld\n", i, *((SampDataQ.base+SampDataQ.front)->Data+i), 0, 0, 0, 0);
    }

    for(i=0; i<MAXQSIZESAMPDATA; i++)
    {
        LOG_Dbg_Msg("%d=%ld\n", i, *((SampDataQ.base+i)->Data+nChnNum), 0, 0, 0, 0);
    }
}

/***********************************************************************
* ShowQData - 显示队列数据
*
* RETURNS: 无
*
*/
void ShowQData(void)
{
    int i;
    int32_t *p;

    for(i=0; i<MAXQSIZESAMPDATA; i++)
    {
        p=(int32_t *)(SampDataQ.base+i);
        printf("%d	%ld	%ld\n", i, *p, *(p+1));
    }
}

/***********************************************************************
* AppQInit - 应用初始化队列
*
* RETURNS: 无
*
*/
void AppQInit(void)
{
    SampDataQ.front=0;
    SampDataQ.rear=0;
}


/***********************************************************************
* maininit - DSP算法初始化以及任务创建
*
* RETURNS: 无
*
*/
void maininit();

// /***********************************************************************
// * maininit - DSP算法初始化以及任务创建
// *
// * RETURNS: 无
// *
// */
void maininit()
{
    int i;

    NewDspData = semCCreate(SEM_Q_PRIORITY, 0);		/* 信号灯创建 */
    NewDspDataforZero= semBCreate(SEM_Q_PRIORITY, 0);
    NewDspDataforMsu= semBCreate(SEM_Q_PRIORITY, 0);

    /* 硬件相关初始化已由底层完成 */

    InitQueueSD(&SampDataQ);		/* 初始化实时处理采样数据队列 */

    if (bdType_g == BOARD_TYPE_E01)
    {
        /* 扩展机箱接口初始化 */
        SynMasterExtInit();
    }

    /* 初始化秒计数,从0开始计数 */
    synout_index_second = SamplingNum_g*50-1;

    /* 传统应用才复位A/D芯片 */
    if (appType_g == APP_TYPE_TRAD)
    {
        ADReset();		/* 复位ADC */
    }

    SampErrorSimul.BreakOnePointFlag=FALSE;
    SampErrorSimul.bUseSimulData=FALSE;
    SampErrorSimul.InsertZeroFlag=FALSE;
    SampErrorSimul.InsertZeroCount=0;
    SampErrorSimul.usBreakPointCount=0;
    SampErrorSimul.StopFlag=FALSE;
    SampErrorSimul.StopCount=0;

    for(i=0; i<SamplingNum_g; i++)
    {
        SampErrorSimul.iWaveData[i]=(int32_t)(30000*sin((2*M_PI*i)/SamplingNum_g));
    }

    ds1306Delay(500000);

    AdcSampInfo.NumTest=0;	/* 计数 */
    DspInfo.flagfre=0;		/* 允许频率计算标志 */
    AdcSampInfo.flagfre=0;
    for(i=0; i<DspHandle.PreProcessNumber; i++)
    {
        /* DSP全周循环计算标志初始化 */
        DspHandle.RoundNum[i] = DspHandle.pPreAiAddr[i+1].ucBgnLgcCh;
    }
    AdcSampInfo.nLampNum=0; 			/* 显示标志 */
    DspInfo.CalNum = 0;		/* 采样与计算间隔计数 */
    CDspInit();						/* 数字信号处理有关初始化 */

    /* 如果不需要预处理,则不计算DSP系数
     */
    if (DspHandle.PreProcessNumber)
    {
        DTFCoefCal(DspInfo.ProcessingNum);		/* 傅立叶系数计算 */
        DFTDifCoefCal(DspInfo.ProcessingNum); 			/* 傅立叶差分系数计算 */
    }

    DspInfo.MsuBufFullFlag = 0;			/* 测量数据缓冲区满标志 */
    DspInfo.ZeroBufFullFlag = 0;					/* 零漂计算数据缓冲区满标志 */
    SampDataShow.EnableFlag=FALSE;			/* 不允许缓冲 */
    SampDataShow.ulCnt=0;		/* 初试为0 */
    EnableCalibrateFlag_g = 0;		/* 允许校准 */

    /* 启动FPGA运行
     * 需验证对无FPGA电路板的影响
     */
    //fpgaStart();

    /* 配置扩展机箱时需要重新启用DSP任务 */
    if (bDspDrvMod)
    {
        nDSPTaskID_g = taskSpawn("tDSPProcess",
                                 TSK_PRI_DSP,
                                 VX_FP_TASK|VX_DEALLOC_STACK,
                                 5000,		/* 堆栈由200改为5，以下同*/
                                 (FUNCPTR)DSPProcess,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

        assert(nDSPTaskID_g != ERROR);
        bDspTaskStartFlag_g=TRUE;
    }

    EP_Set_04CPU_Init_End_Flag(TRUE);			/* Set TRUE directly. */


    // if ((bdType_g == BOARD_TYPE_E02) || (bdType_g == BOARD_TYPE_E03))
    // {
        /* EDP02平台和EDP03平台定时中断初始化 */
        Timer2_Hw_Init();
    // }
}

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
    u_int uiLgcDcCh,    /* number of DC signal channel. */
    DSP_LGC_DC_AI_CFG *plgdccfg				/* DC signal channel configuration. */
)
{
    uint16_t ix = 0;
    uint16_t iy = 0;
    uint16_t iz = 0;
    uint8_t ntmp;
    EP_STATUS stsRet;
    DSP_LGC_AI_CFG *tempLoInfo = plgccfg; 		/* Temporary logic pointer */
    DSP_CALC_AI_CFG *AlgoConf; 		/* Temporary preprocessing pointer */
    DSP_CALC_AI_CFG *pCalConf = pcalccfg; 		/* Temporary preprocessing pointer */
    BOOL bFindFlag = FALSE;  /* 硬件配置通道与smv.xml配置通道对应与否 */
    float fCoff = 1.0;  /* 额定一次值/额定数字量比值 */
    RD_HW_AI_CH *phwai = NULL;
    int iAdcChnNumPerChip;

    LOG_Dbg_Msg("DSP Module Initialization Begin.%d\n", TM_Get_usCnt(), 0, 0, 0, 0, 0);

    iAdcChipNum_g=Get_AD_Chip_Count();		/* 获取ADC芯片数，EDP01-CPU.C-A按32位4片处理 */

    // /* 获取每芯片通道数, 同时考虑兼容性 */
    // if (EP_runFun("Get_Each_AD_Chip_Chn_Count", &iAdcChnNumPerChip) == OK)
    // {
    //     g_iAdcChnNumPerChip = iAdcChnNumPerChip;
    // }

    LOG_Dbg_Msg("ADC芯片数为%d!%d\n", iAdcChipNum_g, TM_Get_usCnt(), 0, 0, 0, 0);
    LOG_Dbg_Msg("每芯片通道数为%d!\n", g_iAdcChnNumPerChip, 0, 0, 0, 0, 0);

    stsRet = EP_SUCCESS;
    // TiIMMR_g=vxImmrGet(); 		/* Global */
    pvAiModHandle_g = pvAiMod; 			/* Handle */
    SamplingNum_g=uiSmplRate;

    DspHandle.pLogChnInfo = plgccfg; 		/* The global pointer to the logic channel */
    DspHandle.MsuNumber = uiMsucCh;
    DspHandle.PreProcessNumber=uiCalcCh;
    DspHandle.NumPerPoints=uiTxPts;
    DspHandle.pDcDspCal = plgdccfg;   /* configuration. */
    DspHandle.DcDspChnNum = uiLgcDcCh;			/* channel number. */

    if(uiSmplRate == 96)		/* DSP采样与发送间隔 */
    {
        DspHandle.uSendInterval=4;
        DspHandle.ucProcessInterval=2*DspHandle.uSendInterval;
    }
    else if(uiSmplRate == 72)
    {
        DspHandle.uSendInterval=3;
        DspHandle.ucProcessInterval=2*DspHandle.uSendInterval;
    }
    else if(uiSmplRate == 48)
    {
        DspHandle.uSendInterval=2;
        DspHandle.ucProcessInterval=2*DspHandle.uSendInterval;
    }
    else if(uiSmplRate == 40)
    {
        DspHandle.uSendInterval=2;
        DspHandle.ucProcessInterval=2*DspHandle.uSendInterval;
    }
    else if(uiSmplRate == 24)
    {
        DspHandle.uSendInterval=1;
        DspHandle.ucProcessInterval=2*DspHandle.uSendInterval;
    }
    else if(uiSmplRate == 20)
    {
        DspHandle.uSendInterval=1;
        DspHandle.ucProcessInterval=2*DspHandle.uSendInterval;
    }
    else if(uiSmplRate == 36)
    {
        DspHandle.uSendInterval=1;
        DspHandle.ucProcessInterval=2*DspHandle.uSendInterval;
    }
    else if(uiSmplRate == 72)
    {
        DspHandle.uSendInterval=2;
        DspHandle.ucProcessInterval=2*DspHandle.uSendInterval;
    }
    else if(uiSmplRate == 128)
    {
        /* 周波128点采样 */
        DspHandle.uSendInterval=4;
        DspHandle.ucProcessInterval=2*DspHandle.uSendInterval;
    }

    // /* 传统应用才一点扫描一次 */
    if (appType_g == APP_TYPE_TRAD)
    {

        /* 传统采样时,如果配置了光差,
         * 则使用插值模式,插值到采样周期整点上
         * 否则不插值
         */
        if (RD_ifOptCfg())
        {
            gSmvCfg.Smv_9_1Cfg[0].receiveType = 0;
        }
        else
        {
            gSmvCfg.Smv_9_1Cfg[0].receiveType = 0xff;
        }

    }

    /* 非光差时采样驱动点数判断,
     * 取扫描周期点数
     */
    if (!RD_ifOptCfg())
    {
        usMinInterval = SC_GetTaskMinPeriod();
        DspHandle.ucProcessInterval = usMinInterval;

        /* 传统应用才一点扫描一次 */
        if (appType_g == APP_TYPE_TRAD)
        {
            if ((bdType_g == BOARD_TYPE_E01) && bDspDrvMod)
            {
                /* One sampling one processing */
                DspHandle.ucProcessInterval = 1;
                usMinInterval = DspHandle.ucProcessInterval;
            }
        }
    }
    else
    {
        OPTAD_flag = TRUE;

        usMinInterval = DspHandle.ucProcessInterval;

        /* 光差模式下, 必须为整数倍 */
        assert ((MAXQSIZESAMPDATA % SamplingNum_g) == 0);
    }

    if (appType_g != APP_TYPE_TRAD && bdType_g != BOARD_TYPE_E01)
    {
        assert (usMinInterval >= DI_FAST_REFRESH_INTERVAL); /* 防止GOOSE开入消抖出错 */
    }

    ulDCUDelaySndCnt = DCU_DELAY_TIME*uiAiRate_g/usMinInterval;

    /* 每周波更新1次 */
    ulUpdateGsCnt = SamplingNum_g/DspHandle.ucProcessInterval;


    // /* 3s查询1次 */
    g_ulDspPollSetChgThreshold = uiAiRate_g/DspHandle.ucProcessInterval;
    g_ulPollSetChgThreshold = 3;

    /* E02-CPU.F-A板支持FPGA操作
     * 查询模式
     */
    if (bPlatformCfgFPGA)
    {
        gSmvCfg.Smv_9_1Cfg[0].receiveType = 14;
    }

    /* 最多间隔点数判断 */
    assert (DspHandle.ucProcessInterval <= INTERVALTIMES);


    /* 线路保护暂时特殊处理 */
    // if (uiAppType_g == APP_LINE)
    // {
    //     /* 控制GOOSE以太网口每次查询帧数 */
    //     g_FccDealCount = LINE_MAX_FRM_PER_POLL;
    //     fcc_rx_deal_count_set(GENERAL_NET_B, g_FccDealCount);
    //     fcc_rx_deal_total_len_set(GENERAL_NET_B, LINE_MAX_BYTE_PER_POOL);
    // }
    // else if (uiAppType_g == APP_PROT_MEA_MERGE)
    // {
    //     /* 保护测控一体化装置(测控端) */
    //     /* 控制GOOSE以太网口每次查询帧数 */
    //     g_FccDealCount = 2*MAX_FRM_PER_POLL;
    //     fcc_rx_deal_count_set(GENERAL_NET_B, g_FccDealCount);
    //     fcc_rx_deal_total_len_set(GENERAL_NET_B, 2*MAX_BYTE_PER_POOL);
    // }
    // else
    // {
    //     /* 控制GOOSE以太网口每次查询帧数 */
    //     g_FccDealCount = MAX_FRM_PER_POLL;
    //     fcc_rx_deal_count_set(GENERAL_NET_B, g_FccDealCount);
    //     fcc_rx_deal_total_len_set(GENERAL_NET_B, MAX_BYTE_PER_POOL);
    // }

    DspInfo.SysFrequency=Sysfrequency;
    DspInfo.fCurSysFrequency=Sysfrequency;
    DspInfo.ProcessingNum = uiProRate; 			/* 保证在48点采样的情况下，能进行24点计算 */
    DspInfo.iZeroBufPointNum=ZEROCYCLENUM*uiSmplRate;		/* 零漂处理点数*/

    LogicChnNumber = 0; 					/* Number of logic channels */

    DspInfo.FreqCalNum = 0;
    DspInfo.uOriginNum = 0;
    while(iy<uiLgcCh)
    {
        /* 逻辑通道计数和最大物理通道数
         * ucHdCh从1开始
         */
        if (ucMaxAnaNumber<tempLoInfo->ucHdCh)
        {
            ucMaxAnaNumber = tempLoInfo->ucHdCh;
        }

        if((tempLoInfo->ucFiltNum == 0) || (tempLoInfo->ucFiltNum == 2))
        {
            /* 通用瞬时值和三取一 */
            LogicChnNumber++;
            iy++;
            tempLoInfo++;
            continue;
        }
        else if(tempLoInfo->ucFiltNum == 1)
        {
            /* 频率计算 */
            DspInfo.FreqCalNum++;
            iy++;
            tempLoInfo++;
            continue;
        }
        else if(tempLoInfo->ucFiltNum == 6)
        {
            /* 采样瞬时值 */
            DspInfo.uOriginNum++;
            iy++;
            tempLoInfo++;
            continue;
        }
        else
            assert(FALSE);
    }

    /* 物理通道数判定(最后一个通道用作缺省值0处理)
     */
    assert (ucMaxAnaNumber < MAXHCHNNUM);

    assert(uiLgcCh == LogicChnNumber+DspInfo.FreqCalNum+DspInfo.uOriginNum);	/* 总数检查 */

    for(ix=0; ix<uiLgcCh; ix++)
    {
        DspHandle.LgcBuffer[ix]=plgccfg[ix];
    }

    if (appType_g == APP_TYPE_DIG)
    {
        /* 数字化应用 */
        DspInfo.RCProp[0]=1.0;		/* 根据测量 */
        DspInfo.RCProp[1]=1.0;
        DspInfo.RCProp[2]=1.0;
        DspInfo.RCProp[3]=1.0;
        DspInfo.RCProp[4]=1.0;
    }
    else if (appType_g == APP_TYPE_TRAD)
    {
        DspInfo.RCProp[0]=1.0;		/* 增益系数已经作出调整 */

        DspInfo.RCProp[1]=1.06833;
        DspInfo.RCProp[2]=1.14595;
        DspInfo.RCProp[3]=1.24745;
        DspInfo.RCProp[4]=1.36868;
    }
    else
    {
        assert (FALSE);
    }

    DspHandle.pPreAiAddr=DspHandle.PreBuffer;
    DspHandle.pPreAiAddr->ucBgnLgcCh=0;
    DspHandle.pPreAiAddr->ucChNum=LogicChnNumber;

    /* 增加了定点算法，瞬时值算法获取地方要改变 */
    DspHandle.pPreAiAddr->ucArithNum=plgccfg[DspInfo.uOriginNum].ucFiltNum;
    DspHandle.pPreAiAddr->ucArithParm=0x10;

    for(ix=0; ix<DspHandle.PreProcessNumber; ix++)
    {
        DspHandle.pPreAiAddr[ix+1]=pCalConf[ix];
        DspHandle.DspCal[ix].ucMaxLgcCh = pCalConf[ix].ucBgnLgcCh+pCalConf[ix].ucChNum;
        DspHandle.DspCal[ix].ucRoundNum=pCalConf[ix].ucBgnLgcCh;		/* 第一个计算通道 */

        DspHandle.MaxLgcChnNum[ix]=pCalConf[ix].ucChNum;

        DspHandle.DspCal[ix].ucBgnLgcCh=pCalConf[ix].ucBgnLgcCh;		/* 开始计算通道 */
        DspHandle.DspCal[ix].ucChNum=pCalConf[ix].ucChNum;						/* 通道数 */
        DspHandle.DspCal[ix].ucWaveNum=(pCalConf[ix].ucArithParm&0x07)-1;		/* 谐波次数 */
        DspHandle.DspCal[ix].ucNumPoint=((pCalConf[ix].ucArithParm)&0x30)>>4;			/* 上传点数 */
        DspHandle.DspCal[ix].RecipAttenuationCoe=DspInfo.RCProp[DspHandle.DspCal[ix].ucWaveNum];
        DspHandle.DspCal[ix].ucArithNum=pCalConf[ix].ucArithNum;			/* 算法 */

        if(pCalConf[ix].ucArithNum == 3)
        {
            if(DspHandle.DspCal[ix].ucWaveNum == 0)		/* 谐波计算系数选择 */
            {
                DspHandle.DspCal[ix].pCos=DspCoe.C1im;
                DspHandle.DspCal[ix].pSin=DspCoe.C1re;
            }
            else if(DspHandle.DspCal[ix].ucWaveNum == 1)
            {
                DspHandle.DspCal[ix].pCos=DspCoe.C2im;
                DspHandle.DspCal[ix].pSin=DspCoe.C2re;
            }
            else if(DspHandle.DspCal[ix].ucWaveNum == 2)
            {
                DspHandle.DspCal[ix].pCos=DspCoe.C3im;
                DspHandle.DspCal[ix].pSin=DspCoe.C3re;
            }
            else if(DspHandle.DspCal[ix].ucWaveNum == 3)
            {
                DspHandle.DspCal[ix].pCos=DspCoe.C4im;
                DspHandle.DspCal[ix].pSin=DspCoe.C4re;
            }
            else if(DspHandle.DspCal[ix].ucWaveNum == 4)
            {
                DspHandle.DspCal[ix].pCos=DspCoe.C5im;
                DspHandle.DspCal[ix].pSin=DspCoe.C5re;
            }
        }
        else if(pCalConf[ix].ucArithNum == 5)
        {
            if(DspHandle.DspCal[ix].ucWaveNum == 0)			/* 谐波计算系数选择 */
            {
                DspHandle.DspCal[ix].pCos=DspCoe.C1im_Dif;
                DspHandle.DspCal[ix].pSin=DspCoe.C1re_Dif;
            }
            if(DspHandle.DspCal[ix].ucWaveNum == 1)
            {
                DspHandle.DspCal[ix].pCos=DspCoe.C2im_Dif;
                DspHandle.DspCal[ix].pSin=DspCoe.C2re_Dif;
            }
            if(DspHandle.DspCal[ix].ucWaveNum == 2)
            {
                DspHandle.DspCal[ix].pCos=DspCoe.C3im_Dif;
                DspHandle.DspCal[ix].pSin=DspCoe.C3re_Dif;
            }
            if(DspHandle.DspCal[ix].ucWaveNum == 3)
            {
                DspHandle.DspCal[ix].pCos=DspCoe.C4im_Dif;
                DspHandle.DspCal[ix].pSin=DspCoe.C4re_Dif;
            }
            if(DspHandle.DspCal[ix].ucWaveNum == 4)
            {
                DspHandle.DspCal[ix].pCos=DspCoe.C5im_Dif;
                DspHandle.DspCal[ix].pSin=DspCoe.C5re_Dif;
            }
        }

        /* To the specified channel and the specified wave time */
        DspHandle.DspCal[ix].pfResultBufPos
            =DspResult.DFTRealImage+2*LogicChnNumber*DspHandle.DspCal[ix].ucWaveNum
             +2*(DspHandle.DspCal[ix].ucBgnLgcCh-DspInfo.uOriginNum);
        DspHandle.DspCal[ix].uSendInterval=DspHandle.uSendInterval;
        DspHandle.DspCal[ix].SamCountCnt=0;
    }

    for(ix=0; ix<DspHandle.PreProcessNumber; ix++)
    {
        if(DspHandle.DspCal[ix].ucArithNum == 4)
        {
            /* 幅值相角计算 */
            for(iy=DspHandle.DspCal[ix].ucBgnLgcCh; iy<DspHandle.DspCal[ix].ucBgnLgcCh+DspHandle.DspCal[ix].ucChNum; iy++)
            {
                for(iz=0; iz<DspHandle.PreProcessNumber; iz++)
                {
                    if((iy >= DspHandle.DspCal[iz].ucBgnLgcCh)
                            && (iy<DspHandle.DspCal[iz].ucBgnLgcCh+DspHandle.DspCal[iz].ucChNum))
                    {
                        if(DspHandle.DspCal[iz].ucArithNum != 4)
                            DspHandle.DspCal[ix].RealImageType[iy]=DspHandle.DspCal[iz].ucArithNum;
                    }
                }
            }
        }
    }

    AlgoConf = DspHandle.pPreAiAddr;

    if (appType_g == APP_TYPE_DIG)
    {
        /* 数字化应用 */
        AdcSampInfo.AdcModu = 1.0; 		/* Forword coeficiency */

        Adc_Modu = 1.0;

    }
    else if (appType_g == APP_TYPE_TRAD)
    {
        /* 传统应用 */
        AdcSampInfo.AdcModu = 5.0/32768.0; 		/* Forword coeficiency */

        Adc_Modu = 5.0/32768.0;
    }
    else
    {
        assert (FALSE);
    }

    /* 依据定点值、瞬时值、频率值的顺序 */
    for(ix=0; ix<DspInfo.uOriginNum; ix++)
    {
        /* 定点算法物理通道到采样值传送通道 */
        DspInfo.uOriginDataPos[ix]=plgccfg[ix].ucHdCh-1;
    }

    /* 从FPGA以太网端口采集数据的配置模式
     */
    if (bPlatformCfgFPGA)
    {
        /* FPGA模式使用e02_sgcfg.xml配置文件
         * 初始对应关系
         * 所有物理通道对应采样通道为255,表示悬空
         */
        for (ix = 0; ix<HCHNNUM; ix++)
        {
            Sam_to_ana[ix] = 255;
        }

        /* 从配置文件中获取物理通道到采样通道对应关系
         */
        cfgGetSamtoAna(Sam_to_ana,Sam_Chn_Source);

        for(ix=0,phwai=phwaich_g; ix<iHwAiChNum_g; ix++,phwai++)
        {
            if (phwai->ucModCh<HCHNNUM)
                phwai->ucChSrc = Sam_Chn_Source[phwai->ucModCh];
        }

        ucMaxAnaNumber = 0;
        for (ix = 0; ix<HCHNNUM; ix++)
        {
            /* 物理通道到采样通道 */
            DspInfo.SamtoAna[ix] = Sam_to_ana[ix];

            /* 工程文件是否配置 */
            if ((Sam_to_ana[ix] <= HCHNNUM) && (Sam_to_ana[ix] != 255))
            {
                arrChnUsedFlag[Sam_to_ana[ix]-1] = TRUE;
            }

            /* 最大采样通道个数 */
            if ((ucMaxAnaNumber < Sam_to_ana[ix]) && (Sam_to_ana[ix] != 255))
            {
                ucMaxAnaNumber = Sam_to_ana[ix];
            }
        }
    }
    else
    {
        if (bdType_g == BOARD_TYPE_E01)
        {
            /* EDP01平台 */
            if (appType_g == APP_TYPE_DIG)
            {
                /* 数字化应用 */

                for(ix=0; ix<HCHNNUM; ix++)
                {
                    /* 物理通道到采样通道 */
                    DspInfo.SamtoAna[ix] = ix+1;
                }
                for (ix=0; ix<HCHNNUM; ix++)
                {
                    Sam_to_ana[ix] = ix+1;
                }
            }
            else if (appType_g == APP_TYPE_TRAD)
            {
                /* 传统应用 */
                for(ix=0; ix<HCHNNUM; ix++)
                {
                    if(ix<13)
                    {
                        DspInfo.SamtoAna[ix] = ix+1;
                        Sam_to_ana[ix] = ix+1;
                    }
                    else if(ix<27)
                    {
                        DspInfo.SamtoAna[ix] = ix+2;
                        Sam_to_ana[ix] = ix+2;
                    }
                    else if(ix<40)
                    {
                        DspInfo.SamtoAna[ix] = ix+3;
                        Sam_to_ana[ix] = ix+3;
                    }
                    else if(ix == 40)
                    {
                        DspInfo.SamtoAna[ix] = 14;
                        Sam_to_ana[ix] = 14;
                    }
                    else if(ix == 41)
                    {
                        DspInfo.SamtoAna[ix] = 29;
                        Sam_to_ana[ix] = 29;
                    }
                }
            }

        }
        else       /* 其它平台 */
        {
            for(ix=0; ix<HCHNNUM; ix++)
            {
                /* 物理通道到采样通道 */
                DspInfo.SamtoAna[ix] = ix+1;
            }

            /* 所有平台都定义 */
            for (ix=0; ix<HCHNNUM; ix++)
            {
                Sam_to_ana[ix] = ix+1;
            }

        }
    }

    /* 传统应用均应处理 */
    if (appType_g == APP_TYPE_TRAD)
    {
        for (ix = 0; ix<HCHNNUM; ix++)
        {
            arrChnUsedFlag[ix] = TRUE;
        }
    }

    /* 从硬件配置中获取smvValIn值
     */
    for (iy = 0; iy<gSmvCfg.Smv_9_1Cfg[0].dataNum; iy++)
    {
        /* 物理通道是否在工程配置文件中处理 */
        if ((gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvDataChn<HCHNNUM)
                && (gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvDataChn != INVALID_ANA_CHN_NO)
                && (gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvAdsuChn != INVALID_ANA_CHN_NO))
        {
            arrChnUsedFlag[gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvDataChn] = TRUE;
        }

        for (ix = 0; ix<iHwAiChNum_g; ix++)
        {
            if (phwaich_g[ix].ucModCh == gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvDataChn)
            {
                if (phwaich_g[ix].ucUnit == 0x8)
                {
                    /* 相电流 */
                    gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvValIn = phwaich_g[ix].uFstRatedVal*1000;
                }
                else if (phwaich_g[ix].ucUnit == 0x14)
                {
                    /* 相电压 */
                    gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvValIn = (uint32_t)(phwaich_g[ix].uFstRatedVal*100.0/sqrt(3.0));
                }

                gSmvCfg.Smv_9_1Cfg[0].smvData[iy].phwaich = &phwaich_g[ix];

                break;
            }
        }

        /* 允许smv.xml文件中配置通道个数大于硬件配置文件
         */
        if (ix == iHwAiChNum_g)
        {
            gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvValIn = 1;
            gSmvCfg.Smv_9_1Cfg[0].smvData[iy].phwaich = NULL;
        }
    }

    /* 合流通道
     */
    for (iy = 0; iy<gSmvSLFCfg.smvNum; iy++)
    {
        /* 物理通道是否在工程配置文件中处理 */
        if ((gSmvSLFCfg.Smv_SLF_Cfg[iy].smvDataChn<HCHNNUM)
                && (gSmvSLFCfg.Smv_SLF_Cfg[iy].smvDataChn != INVALID_ANA_CHN_NO)
                && ((gSmvSLFCfg.Smv_SLF_Cfg[iy].smvDataChnR1 >= 0)
                    || (gSmvSLFCfg.Smv_SLF_Cfg[iy].smvDataChnR2 >= 0)
                    || (gSmvSLFCfg.Smv_SLF_Cfg[iy].smvDataChnR3 >= 0)))
        {
            arrChnUsedFlag[gSmvSLFCfg.Smv_SLF_Cfg[iy].smvDataChn] = TRUE;
        }

        for (ix = 0; ix<iHwAiChNum_g; ix++)
        {
            if (phwaich_g[ix].ucModCh == gSmvSLFCfg.Smv_SLF_Cfg[iy].smvDataChn)
            {
                if (phwaich_g[ix].ucUnit == 0x8)
                {
                    /* 相电流 */
                    gSmvSLFCfg.Smv_SLF_Cfg[iy].smvValIn = phwaich_g[ix].uFstRatedVal*1000;
                }
                else if (phwaich_g[ix].ucUnit == 0x14)
                {
                    /* 相电压 */
                    gSmvSLFCfg.Smv_SLF_Cfg[iy].smvValIn = (uint32_t)(phwaich_g[ix].uFstRatedVal*100.0/sqrt(3.0));
                }

                gSmvSLFCfg.Smv_SLF_Cfg[iy].phwaich = &phwaich_g[ix];

                break;
            }
        }

        /* 允许smv.xml文件中配置通道个数大于硬件配置文件
         */
        if (ix == iHwAiChNum_g)
        {
            gSmvSLFCfg.Smv_SLF_Cfg[iy].smvValIn = 1;
            gSmvSLFCfg.Smv_SLF_Cfg[iy].phwaich = NULL;
        }
    }

    for(ix=0; ix<LogicChnNumber; ix++)
    {
        /* 逻辑通道到物理通道 */
        DspInfo.AnatoLog[ix] = plgccfg[DspInfo.uOriginNum+ix].ucHdCh;
        DspInfo.LogtoAna[plgccfg[DspInfo.uOriginNum+ix].ucHdCh-1]=ix;		/* 物理通道到逻辑通道 */

        /* 查询在smv.xml文件中的通道系数配置
         * 目前只处理一个数据集
         */

        bFindFlag = FALSE;

        /* 从FPGA以太网端口采集数据的配置模式
         */
        if (bPlatformCfgFPGA)
        {
            /* FPGA程序从e02_sgcfg.xml文件查找系数 */
            bFindFlag = TRUE;
            fCoff = cfgGetHwChnCoff(plgccfg[DspInfo.uOriginNum+ix].ucHdCh);

            /* 暂时不支持一次额定值统一 */
            DspInfo.pSmvValIn[ix] = NULL;
            DspInfo.pSmvValOut[ix] = NULL;
        }
        else
        {
            for (iy = 0; iy<gSmvCfg.Smv_9_1Cfg[0].dataNum; iy++)
            {
                /* ucHdCh从1开始,smvDataChn从0开始
                 * 同时非延时通道
                 */
                if ((plgccfg[DspInfo.uOriginNum+ix].ucHdCh
                        == (gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvDataChn+1))
                        && (gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvChnType == 1))
                {
                    /* 如果一次额定值为0,则二次值为0
                     */
                    if (gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvValIn == 0)
                    {
                        fCoff = 0.0;
                    }
                    else
                    {
                        fCoff = (float)(gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvValOut)/(float)(gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvValIn);
                    }

                    bFindFlag = TRUE;
                    DspInfo.pSmvValIn[ix] = &gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvValIn;
                    DspInfo.pSmvValOut[ix] = &gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvValOut;
                    break;
                }
            }

            /* 如果在9-2配置中查询不到,则从合流通道中查询 */
            if (!bFindFlag)
            {
                for (iy = 0; iy<gSmvSLFCfg.smvNum; iy++)
                {
                    /* ucHdCh从1开始 */
                    if (plgccfg[DspInfo.uOriginNum+ix].ucHdCh
                            == gSmvSLFCfg.Smv_SLF_Cfg[iy].smvDataChn+1)
                    {
                        /* 如果一次额定值为0,则二次值为0
                         */
                        if (gSmvSLFCfg.Smv_SLF_Cfg[iy].smvValIn == 0)
                        {
                            fCoff = 0.0;
                        }
                        else
                        {
                            fCoff = (float)(gSmvSLFCfg.Smv_SLF_Cfg[iy].smvValOut)/(float)(gSmvSLFCfg.Smv_SLF_Cfg[iy].smvValIn);
                        }

                        bFindFlag = TRUE;
                        DspInfo.pSmvValIn[ix] = &gSmvSLFCfg.Smv_SLF_Cfg[iy].smvValIn;
                        DspInfo.pSmvValOut[ix] = &gSmvSLFCfg.Smv_SLF_Cfg[iy].smvValOut;
                        break;
                    }
                }
            }
        }

        /* 支持光差发送时本侧采样数据需分段处理,满足与发送到对侧数据一致
         */
        if (bFindFlag)
        {
            DspInfo.fPropConf1[ix] = fCoff;
        }
        else
        {
            DspInfo.fPropConf1[ix] = 1.0;
            DspInfo.pSmvValIn[ix] = NULL;
            DspInfo.pSmvValOut[ix] = NULL;
        }

        /* 9-1不处理smv.xml配置系数
         */
        if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 8)
        {
            DspInfo.fPropConf1[ix] = 1.0;
        }

        DspInfo.fPropConf2[ix] = plgccfg[DspInfo.uOriginNum+ix].fCoff*AdcSampInfo.AdcModu;

        /* 额定数字量减半, 用于数字化光差 */
        if (OPTAD_flag && (appType_g == APP_TYPE_DIG))
        {
            DspInfo.fPropConf1[ix] /= 2.0;
            DspInfo.fPropConf2[ix] *= 2.0;
        }

        /* 按采样通道号排列系数 */
        DspInfo.fSmvInOut[plgccfg[DspInfo.uOriginNum+ix].ucHdCh - 1] = DspInfo.fPropConf1[ix];
        DspInfo.PropConf[ix] = DspInfo.fPropConf1[ix]*DspInfo.fPropConf2[ix];

        /* 防止除0错误 */
        if (fabs(DspInfo.fPropConf1[ix])<FLT_PRECISION)
        {
            DspInfo.iPropConf[ix] = 1;
        }
        else
        {
            DspInfo.iPropConf[ix] = 1.0/DspInfo.fPropConf1[ix];
        }
    }

    /* 物理通道到ASDU的对应关系
     * 非数字化平台同样处理
     */
    for (ix = 0; ix<iHwAiChNum_g; ix++)
    {
        if (phwaich_g[ix].ucModCh<HCHNNUM)
            phwaich_g[ix].pSmv = cfgGetAsdu(phwaich_g[ix].ucModCh);
    }

    /* 逻辑通道到采样通道 */
    for (ix = 0; ix<LogicChnNumber; ix++)
    {
        ntmp = DspInfo.AnatoLog[ix]-1;
        DspInfo.SamtoLog[ix] = DspInfo.SamtoAna[ntmp]-1;
        /* 通道是否有效, 没有配置, 或者设置为255 */
        if (arrChnUsedFlag[ntmp])
        {
            DspInfo.SamtoLog[ix] = DspInfo.SamtoAna[ntmp]-1;
        }
        else
        {
            /* DspInfo.SamtoLog[ix] = INVALID_ANA_CHN_NO; */

            /* 转译为最后一个通道 */
            DspInfo.SamtoLog[ix] = HCHNNUM-1;
        }
    }


    DspInfo.XPerShun=AlgoConf[0].ucArithParm; 			/* 计算参数*/
    DspInfo.XPerShun=DspInfo.XPerShun&0x30;
    DspInfo.XPerShun=DspInfo.XPerShun>>4;
    AdcSampInfo.SampleRate=SamplingNum_g*DspInfo.XPerShun*DspInfo.SysFrequency;
    AdcSampInfo.fSampleRate = AdcSampInfo.SampleRate;

    Sample_Rate=SamplingNum_g*DspInfo.XPerShun*DspInfo.SysFrequency;

    DspHandle.DataNumofPreProcess=0;

    AlgoConf =pcalccfg ;
    for(ix=0; ix<DspHandle.PreProcessNumber; ix++)
    {
        /* 结果数据个数计算 */
        DspHandle.DataNumofPreProcess
            =DspHandle.DataNumofPreProcess+((( AlgoConf[ix].ucArithParm ) & 0x30)>>4)*AlgoConf[ix].ucChNum;
    }

    /* 所有的通道数，包括采样值、需要瞬时值处理和瞬时值+频率计算 */
    DspHandle.LogicChnNumber_8 = (DspInfo.uOriginNum+LogicChnNumber+DspInfo.FreqCalNum)/8;
    DspHandle.LogicChnNumber_m = (DspInfo.uOriginNum+LogicChnNumber+DspInfo.FreqCalNum)%8;

    /* 所有预处理上传数据总的点数 */
    DspHandle.DataNumofPreProcess_8 = DspHandle.DataNumofPreProcess/8;
    DspHandle.DataNumofPreProcess_m = DspHandle.DataNumofPreProcess%8;

    LOG_Dbg_Msg("Lgc8 is  %d,Lgcm is %d,PreAll is %d,Pre8 is %d,Prem is %d \n",
                DspHandle.LogicChnNumber_8, DspHandle.LogicChnNumber_m,
                DspHandle.DataNumofPreProcess, DspHandle.DataNumofPreProcess_8,
                DspHandle.DataNumofPreProcess_m, 0);

    for(ix=0; ix<DspHandle.MsuNumber; ix++)
    {
        /* 测量配置 */
        DspHandle.MSUBuffer[ix].ucBgnLgcCh = pmsuccfg[ix].ucBgnLgcCh;
        DspHandle.MSUBuffer[ix].ucChNum = pmsuccfg[ix].ucChNum;
        DspHandle.MSUBuffer[ix].ucArithNum = pmsuccfg[ix].ucArithNum;
        DspHandle.MSUBuffer[ix].ucArithParm = pmsuccfg[ix].ucArithParm;
    }

    for(ix=0; ix<DspHandle.MsuNumber; ix++)
    {
        DspHandle.MsuCal[ix].ucBgnLgcCh=pmsuccfg[ix].ucBgnLgcCh;
        DspHandle.MsuCal[ix].ucChNum=pmsuccfg[ix].ucChNum;
        DspHandle.MsuCal[ix].ucWaveNum=((pmsuccfg[ix].ucArithParm)&0x07)-1;
        DspHandle.MsuCal[ix].ucNumPoint=((pmsuccfg[ix].ucArithParm)&0x30)>>4;
    }

    AdcSampInfo.MsuResultNum = 0;
    for(ix=0; ix<DspHandle.MsuNumber; ix++)
    {
        /* 测量结果数据长度计算 */
        AdcSampInfo.MsuResultNum = AdcSampInfo.MsuResultNum
                                   +((DspHandle.MSUBuffer[ix].ucArithParm & 0x30)>>4)*DspHandle.MSUBuffer[ix].ucChNum;
    }


    if (PoWrInit() == EP_SUCCESS)
    {
        /* 测量参数文件初始化*/
        LOG_Dbg_Msg("电量保存文件初始化成功.\n", 0, 0, 0, 0, 0, 0);
        if(PoRdFile() != EP_SUCCESS)
        {
            LOG_Dbg_Msg("电量保存文件读取失败.\n", 0, 0, 0, 0, 0, 0);
        }/* 读配置 */
        else
        {
            LOG_Dbg_Msg("电量保存文件读取成功.\n", 0, 0, 0, 0, 0, 0);
        }
    }
    else
    {
        LOG_Dbg_Msg("电量保存文件初始化失败.\n", 0, 0, 0, 0, 0, 0);
    }


    UpdateAcCoff();		/* 更新系数 */

    /* 通信中断时缺省值处理 */
    for (ix = 0; ix<ucMaxAnaNumber; ix++)
    {
        /* 工程文件是否配置判断, 悬空时不置无效标识 */
        if (arrChnUsedFlag[ix])
        {
            arrDefaultVal[ix] = 0;
            arrDefaultSts[ix] = AI_COM_ERR | AI_DAT_VLD | AI_DAT_SYN;
        }
    }

    /* 状态标预置位 */
    for (ix = 0; ix<MAXQSIZESAMPDATA+1; ix++)
    {
        for (iy = 0; iy<MAXCHNELS; iy++)
        {
            /* 悬空时置压板退出 */
            if (!arrChnUsedFlag[iy])
            {
                send_data[ix][iy] = 0;
                send_data_sts[ix][iy] = AI_LINK_STS;
                aulCurSamDataStsArr_g[iy] = AI_LINK_STS;
            }
            else
            {
                send_data[ix][iy] = 0;
                send_data_sts[ix][iy] = 0;
            }
        }
    }

    
/* 其它初始化 */
    maininit();

    LOG_Dbg_Msg("DSP Module Initialization Over.\n", 0, 0, 0, 0, 0, 0);

    return stsRet;
}

#ifndef EDP01_CA_EXT_BUILD
/***********************************************************************
* UpdateAcCoff - 更新交流通道系数
*
* RETURNS: 无
*
*/
void UpdateAcCoff(void)
{
    int ix;
    int iy;
    float fTmp;

    taskLock();

    /* 实际计算用通道系数,变比系数乘以增益系数 */
    for (iy = 0; iy<iHwAiChNum_g; iy++)
    {
        phwaich_g[iy].fCoff = phwaich_g[iy].fSetCoff*phwaich_g[iy].fGain;
    }

    /* DSP模块初始化完成之前以下语句无效 */
    for(ix=0; ix<LogicChnNumber; ix++)
    {
        for(iy=0; iy<iHwAiChNum_g; iy++)
        {
            if(((DspHandle.LgcBuffer[DspInfo.uOriginNum+ix].ucHdCh-1) == phwaich_g[iy].ucModCh)
                    && (phwaich_g[iy].paimod == pvAiModHandle_g))
            {
                break;
            }
        }

        if(iy >= iHwAiChNum_g)
        {
            assert(FALSE);
        }

        DspHandle.LgcBuffer[DspInfo.uOriginNum+ix].fCoff=phwaich_g[iy].fCoff;

        DspInfo.fPropConf2[ix] = DspHandle.LgcBuffer[DspInfo.uOriginNum+ix].fCoff*AdcSampInfo.AdcModu;

        if (OPTAD_flag && (appType_g == APP_TYPE_DIG))
        {
            DspInfo.fPropConf2[ix] *= 2.0;
        }
        DspInfo.PropConf[ix] = DspInfo.fPropConf2[ix]*DspInfo.fSmvInOut[DspHandle.LgcBuffer[DspInfo.uOriginNum+ix].ucHdCh - 1];

        phwaich_g[iy].PropConf = DspInfo.PropConf[ix];
        /* 二次转一次 */
        if (phwaich_g[iy].ucUnit == 0x8)
        {
            /* 相电流 */

            fTmp = DspInfo.PropConf[ix]*1000;
            if (fabs(fTmp) >= FLT_PRECISION)
            {
                phwaich_g[iy].fSecToFstCoff = 1.0/fTmp;
            }
            else
            {
                phwaich_g[iy].fSecToFstCoff = 1.0;
            }
        }
        else if (phwaich_g[iy].ucUnit == 0x14)
        {
            /* 相电压 */

            fTmp = DspInfo.PropConf[ix]*100;
            if (fabs(fTmp) >= FLT_PRECISION)
            {
                phwaich_g[iy].fSecToFstCoff = 1.0/fTmp;
            }
            else
            {
                phwaich_g[iy].fSecToFstCoff = 1.0;
            }
        }
    }

    /* 更改光差AO浮点发送系数
     * 光差模块初始化完成之后才能生效
     * 而初始化时已使用最新系数
     */
    UpdateOptAcCoff();
    RD_Chg_AO_Coff();

    taskUnlock();
}

// /* 更新一次额定值
//  * Para:
//  *     NONE.
//  * Return:
//  *     NONE.
//  */
// void UpdateSmvValIn(void)
// {
//     uint16_t ix = 0;
//     uint16_t iy = 0;
//     RD_HW_AI_CH *pch;
//     float fTmp;

//     taskLock();

//     /* 采样通道 */
//     for (iy = 0; iy<gSmvCfg.Smv_9_1Cfg[0].dataNum; iy++)
//     {
//         if (gSmvCfg.Smv_9_1Cfg[0].smvData[iy].phwaich)
//         {
//             pch = (RD_HW_AI_CH *)gSmvCfg.Smv_9_1Cfg[0].smvData[iy].phwaich;

//             if (pch->ucUnit == 0x8)
//             {
//                 /* 相电流 */
//                 gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvValIn = pch->uFstRatedVal*1000;
//             }
//             else if (pch->ucUnit == 0x14)
//             {
//                 /* 线电压转换为相电压 */
//                 gSmvCfg.Smv_9_1Cfg[0].smvData[iy].smvValIn = (uint32_t)(pch->uFstRatedVal*100.0/sqrt(3.0));
//             }
//         }
//     }

//     /* 合流通道 */
//     for (iy = 0; iy<gSmvSLFCfg.smvNum; iy++)
//     {
//         if (gSmvSLFCfg.Smv_SLF_Cfg[iy].phwaich)
//         {
//             pch = (RD_HW_AI_CH *)gSmvSLFCfg.Smv_SLF_Cfg[iy].phwaich;

//             if (pch->ucUnit == 0x8)
//             {
//                 /* 相电流 */
//                 gSmvSLFCfg.Smv_SLF_Cfg[iy].smvValIn = pch->uFstRatedVal*1000;
//             }
//             else if (pch->ucUnit == 0x14)
//             {
//                 /* 线电压转换为相电压 */
//                 gSmvSLFCfg.Smv_SLF_Cfg[iy].smvValIn = (uint32_t)(pch->uFstRatedVal*100.0/sqrt(3.0));
//             }
//         }
//     }

//     /* 更新实际系数 */
//     for (ix = 0; ix<LogicChnNumber; ix++)
//     {
//         if (DspInfo.pSmvValIn[ix] && DspInfo.pSmvValOut[ix])
//         {
//             if (*DspInfo.pSmvValIn[ix] == 0)
//             {
//                 DspInfo.fPropConf1[ix] = 0.0;
//             }
//             else
//             {
//                 DspInfo.fPropConf1[ix] = (float)(*DspInfo.pSmvValOut[ix])/(float)(*DspInfo.pSmvValIn[ix]);
//             }

//             /* 额定数字量减半, 用于数字化光差 */
//             if (OPTAD_flag && (appType_g == APP_TYPE_DIG))
//             {
//                 DspInfo.fPropConf1[ix] /= 2.0;
//             }

//             /* 按采样通道号排列系数 */
//             DspInfo.fSmvInOut[DspHandle.pLogChnInfo[DspInfo.uOriginNum+ix].ucHdCh - 1] = DspInfo.fPropConf1[ix];
//             DspInfo.PropConf[ix] = DspInfo.fPropConf1[ix]*DspInfo.fPropConf2[ix];

//             /* 防止除0错误 */
//             if (fabs(DspInfo.fPropConf1[ix])<FLT_PRECISION)
//             {
//                 DspInfo.iPropConf[ix] = 1;
//             }
//             else
//             {
//                 DspInfo.iPropConf[ix] = 1.0/DspInfo.fPropConf1[ix];
//             }

//             /* 更新物理通道系数 */
//             for (iy = 0; iy<iHwAiChNum_g; iy++)
//             {
//                 if (((DspHandle.pLogChnInfo[DspInfo.uOriginNum+ix].ucHdCh-1) == phwaich_g[iy].ucModCh)
//                         && (phwaich_g[iy].paimod == pvAiModHandle_g))
//                 {
//                     break;
//                 }
//             }

//             if (iy >= iHwAiChNum_g)
//             {
//                 assert(FALSE);
//             }

//             phwaich_g[iy].PropConf = DspInfo.PropConf[ix];
//             /* 二次转一次 */
//             if (phwaich_g[iy].ucUnit == 0x8)
//             {
//                 /* 相电流 */
//                 fTmp = DspInfo.PropConf[ix]*1000;
//                 if (fabs(fTmp) >= FLT_PRECISION)
//                 {
//                     phwaich_g[iy].fSecToFstCoff = 1.0/fTmp;
//                 }
//                 else
//                 {
//                     phwaich_g[iy].fSecToFstCoff = 1.0;
//                 }
//             }
//             else if (phwaich_g[iy].ucUnit == 0x14)
//             {
//                 /* 相电压 */

//                 fTmp = DspInfo.PropConf[ix]*100;
//                 if (fabs(fTmp) >= FLT_PRECISION)
//                 {
//                     phwaich_g[iy].fSecToFstCoff = 1.0/fTmp;
//                 }
//                 else
//                 {
//                     phwaich_g[iy].fSecToFstCoff = 1.0;
//                 }
//             }
//         }
//     }

//     /* 更新光差定点数转换系数 */
//     for (ix = 0; ix<OptAIMod_g.AINum; ix++)
//     {
//         OptAIMod_g.fCoff[ix] = DspInfo.fSmvInOut[OptAIMod_g.aOptBoxAiCfg_g[ix].ucSrcAIHdCh];

//         /* 防止除0错误 */
//         if (fabs(OptAIMod_g.fCoff[ix])<FLT_PRECISION)
//         {
//             OptAIMod_g.iCoff[ix] = 1;
//         }
//         else
//         {
//             OptAIMod_g.iCoff[ix] = 1.0/OptAIMod_g.fCoff[ix];
//         }
//     }

//     taskUnlock();
// }
// #endif

// /***********************************************************************
// * ShowAcCoff - 显示变比系数
// *
// * RETURNS: 无
// *
// */
// void ShowAcCoff(void)
// {
//     int ix;

//     for(ix=0; ix<LogicChnNumber; ix++)
//     {
//         LOG_Dbg_Msg("%d=%d\n", ix, (int)(DspHandle.LgcBuffer[ix].fCoff*1000), 0, 0, 0, 0);
//     }
// }



// #if defined(EDP_01_02_BUILD) || defined(EDP03_BUILD)
// /***********************************************************************
// * Timer2_Hw_Init - This function is used to initialize Timer2 hardware environment
// *
// * RETURNS: 无
// *
// */
static void Timer2_Hw_Init (void)
{
    int16_t status = ERROR;

    // TiIMMR_g=vxImmrGet();
    // *TGCR1(TiIMMR_g) |= 0x10; /* Reset timer 2 */
    // *TGCR1(TiIMMR_g) &= (~0x20); /* Normal operation. */

    initTimer(INUM_TIMER2, 833);
    // initTimerMs(INUM_TIMER2,5000); 

    /* Set timer2 interrupt frequence to RD_SIO_RATE. */
    TrrCnt.fStVal=((float)sysInputFreq_g/16.0)/(float)AdcSampInfo.SampleRate-1.0;			/* Float point setting value */

    TrrCnt.usStVal=(uint16_t)floor(TrrCnt.fStVal);		/* Setting value */

    if((sysInputFreq_g/16)/AdcSampInfo.SampleRate == 0)
    {
        /* If need to adjust timer reference */
        TrrCnt.bAdjFlag=FALSE;
    }
    else
    {
        TrrCnt.bAdjFlag=TRUE;
    }

    /* Set the reference register according to the sample number in a cycle
     * 因为100M时，16会溢出，所以改为2分频
     */

    // *TRR2(TiIMMR_g) = TrrCnt.usStVal;
    ulTRR2Base_g=TrrCnt.usStVal;/*保存基准值  2011-4-14日 ZY  */

    /* Set timer2 clock frequence 10MHz. */
    /* sysInputFreq_g/1000000L为分频次数,根据规定减1 因为100M时，16会溢出，所以改为2分频 */
    // *TMR2(TiIMMR_g) = ((1-1)<<8) | 0x1c;
    /* 2分频 */

    /* 初始化MEGA16在采样中断中的通信功能 */
    // WT_MegaComInit(1000000L/AdcSampInfo.SampleRate);

    // IoPinOutputHigh(IO_IO_SET_IO_C_OUT, IO_PIN_HIGH);

    sSmvCCInfo.backpoint = LOCAL_DELAY_NUM;/*CC板信息中回退点数赋初值*/

    /* 处理以下三种类型 */
    assert ((gSmvCfg.Smv_9_1Cfg[0].receiveType == 0)
            || (gSmvCfg.Smv_9_1Cfg[0].receiveType == 3)
            || (gSmvCfg.Smv_9_1Cfg[0].receiveType == 8)
            || (gSmvCfg.Smv_9_1Cfg[0].receiveType == 9)
            || (gSmvCfg.Smv_9_1Cfg[0].receiveType == 14)  /* E02-CPU.F-A以太网采样 */
            || (gSmvCfg.Smv_9_1Cfg[0].receiveType == 15)   /* SPT总线模式 */
            || (gSmvCfg.Smv_9_1Cfg[0].receiveType == 0xff));


    /* initialize interrupt vect */
    if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 0)
    {
        /* 光差传统侧 */
        status = intConnect(INUM_TIMER2, Timer2_ISR_01, 0);
    }
    else if(gSmvCfg.Smv_9_1Cfg[0].receiveType==3)
    {
        /* CC板接收查询模式 */
        status=intConnect(INUM_TIMER2, Timer2_ISR_02, 0);
        bDataTransMod = TRUE;
        sSmvCCInfo.bNetSynMod = FALSE; /* 初始为计数器模式 */
    }
    else if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 8)
    {
        /* 与DCU板通信 */
        status = intConnect(INUM_TIMER2, Timer2_ISR_06, 0);
        bDataTransMod = TRUE;
    }
    else if(gSmvCfg.Smv_9_1Cfg[0].receiveType==9)
    {
        /* CC板接收光纤纵差 */
        status=intConnect((INUM_TIMER2), Timer2_ISR_06, 0);
        bDataTransMod = TRUE;
        sSmvCCInfo.bNetSynMod = TRUE; /* 初始为外同步模式 */
    }
    else if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 14)
    {
        /* 与FPGA配合 */
        status = intConnect((INUM_TIMER2), Timer2_ISR_14, 0);
        bDataTransMod = TRUE;
    }
    else if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 15)
    {
        /* SPT总线支持 */
        InitCnt(); /* 初始化变量 */
        status = intConnect((INUM_TIMER2), Timer2_ISR_02, 0);
        bDataTransMod = TRUE;
    }
    else if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 0xff)
    {
        /* 子单元处理及除光差外传统采样 */
        status=intConnect((INUM_TIMER2), Timer2_ISR, 0);
    }
    else if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 0xff)
    {
        /* 子单元处理及除光差外传统采样 */
        status=intConnect((INUM_TIMER2), Timer2_ISR, 0);
    }
    else
    {
        LOG_Dbg_Msg("配置文件'%s' 'TYPE'为非定义类型\n",(int)SMV_CFG_FILE,0,0,0,0,0);
        assert(0);
    }
    printf("gSmvCfg.Smv_9_1Cfg[0].receiveType=%d \n",gSmvCfg.Smv_9_1Cfg[0].receiveType);
    if(status == ERROR)
        LOG_Dbg_Msg("!!!!!!!!!!!!!!!!!TIMER2 CONNECT ERROR! \n", 0, 0, 0, 0, 0, 0);
    else
        LOG_Dbg_Msg("!!!!!!!!!!!!!!!!!TIMER2 CONNECT OK! \n", 0, 0, 0, 0, 0, 0);

    /* 传统采样,EDP01、EDP02和EDP03平台使用TOUT */
    if (appType_g == APP_TYPE_TRAD)
    {
        // *PPARC (TiIMMR_g) |= PC28;       /* Dedicated peripheral function */
        // *PSORC (TiIMMR_g) |= 0;       /* Option 1 */
        // *PDIRC (TiIMMR_g) |= PC28;     /* Output */
    }
    else if (appType_g == APP_TYPE_DIG)
    {
        // *PDIRC (TiIMMR_g) &= ~PC28;     /* Output */
    }
    else
    {
        assert (FALSE);
    }

    /* Residual point number in a cycle. */
    TrrCnt.usValRes=(TrrCnt.fStVal-TrrCnt.usStVal)*SamplingNum_g+0.5;
    TrrCnt.usValResFrHlf=TrrCnt.usValRes/2+TrrCnt.usValRes%2;
    TrrCnt.usValResFrDesHlf=TrrCnt.usValResFrHlf;
    TrrCnt.usValResBkHlf=TrrCnt.usValRes/2;
    TrrCnt.usValResBkDesHlf=TrrCnt.usValResBkHlf;
    if(TrrCnt.usValResFrHlf)
    {
        TrrCnt.bFrHlfCyl=TRUE;			/* The front half cycle flag. */
    }
    else
    {
        TrrCnt.bFrHlfCyl=FALSE;			/* The front half cycle flag. */
    }

    if(TrrCnt.usValResBkHlf)
    {
        TrrCnt.bBkHlfCyl=TRUE;				/* The back half cycle flag. */
    }
    else
    {
        TrrCnt.bBkHlfCyl=FALSE;				/* The back half cycle flag. */
    }

    TrrCnt.usLstTrrCnt=TrrCnt.usStVal;			/* Store for stepwise adjusting. */
    TrrCnt.bInitAdjFlag=TRUE;

    /* rounding accumulating value every step. */
    TrrCnt.ulStpCnt = (uint32_t)((TrrCnt.fStVal - TrrCnt.usStVal)*1000.0);
    TrrCnt.ulCalCnt = 0;
    TrrCnt.bFreqSetFlag = FALSE;
    TrrCnt.bAdjFlag = FALSE;  /* default is not to deal with accurate sampling. */

    intEnable(INUM_TIMER2);		/* Enable timer2 interrupt */
}

#ifndef EDP01_CA_EXT_BUILD
/***********************************************************************
* SetTimer2 -设置定时器定时时间
*
* RETURNS: 无
*
*/
void SetTimer2(
    uint32_t SamRate		/* 采样速率 */
)
{
    // TiIMMR_g=vxImmrGet();
    // TrrCnt.bAdjFlag=FALSE;

    // /* Set timer2 interrupt frequence to RD_SIO_RATE. */
    // *TRR2(TiIMMR_g) = SamRate;
}

/***********************************************************************
* AiOpt_Send -光纵缓冲数据写及发送
*
* RETURNS: 无
*
*/
void AiOpt_Send(void)
{
    // WrOptBuf();			/* 写入光纵缓冲 */

    // if(Adjust_flag)
    // {
    //     /* 定时调整 */
    //     SetTimer2(OptAIMod_g.RegNum);
    //     OptAIMod_g.iAdjSamCnt--;
    //     if(OptAIMod_g.iAdjSamCnt<0)
    //     {
    //         /* 调整结束 */
    //         Adjust_flag  = FALSE;
    //         SetTimer2(OptAIMod_g.BackupRegNum);
    //     }
    // }
}

/***********************************************************************
* SetAdSampFreq - 设定AD采样频率
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS SetAdSampFreq(
    float fPwrFreq       	/* 系统当前频率，频率范围为45->65 */
)
{
    // float fSampleRate;
    // int iLockKey;

    // /* LOG_Dbg_Msg("设定AD采样频率!\n", 0, 0, 0, 0, 0, 0); */

    // if((fPwrFreq<45.0) || (fPwrFreq>65.0))
    // {
    //     LOG_Dbg_Msg("频率范围越界!\n", 0, 0, 0, 0, 0, 0);

    //     return EP_ERROR;
    // }

    // iLockKey=intLock();		/* 闭锁中断 */

    // DspInfo.fCurSysFrequency=fPwrFreq;			/* 记录当前频率 */
    // fSampleRate=(float)SamplingNum_g*fPwrFreq;		/* 采样频率 */
    // AdcSampInfo.fSampleRate = fSampleRate;

    // uiAiRate_g=(u_int)fSampleRate;
    // AdcSampInfo.SampleRate = uiAiRate_g;
    // rdinfo_g.uiSmplPeriod=(1000000L+uiAiRate_g/2)/uiAiRate_g; 		/* 更新采样周期，用us表示 */

    // TrrCnt.fStVal=((float)sysInputFreq_g/16.0)/(float)fSampleRate-1.0;	/* Float point setting value */

    // TrrCnt.usStVal=(uint16_t)floor(TrrCnt.fStVal);		/* Setting value */
    // TrrCnt.usValRes=(TrrCnt.fStVal-TrrCnt.usStVal)*SamplingNum_g+0.5;		/* Residual point number in a cycle. */
    // TrrCnt.usValResFrHlf=TrrCnt.usValRes/2+TrrCnt.usValRes%2;
    // TrrCnt.usValResFrDesHlf=TrrCnt.usValResFrHlf;
    // TrrCnt.usValResBkHlf=TrrCnt.usValRes/2;
    // TrrCnt.usValResBkDesHlf=TrrCnt.usValResBkHlf;
    // if(TrrCnt.usValResFrHlf)
    // {
    //     TrrCnt.bFrHlfCyl=TRUE;			/* The front half cycle flag. */
    // }
    // else
    // {
    //     TrrCnt.bFrHlfCyl=FALSE;			/* The front half cycle flag. */
    // }

    // if(TrrCnt.usValResBkHlf)
    // {
    //     TrrCnt.bBkHlfCyl=TRUE;				/* The back half cycle flag. */
    // }
    // else
    // {
    //     TrrCnt.bBkHlfCyl=FALSE;				/* The back half cycle flag. */
    // }
    // TrrCnt.bAdjFlag=TRUE;
    // TrrCnt.usLstTrrCnt=TrrCnt.usStVal;			/* Store for stepwise adjusting. */

    // if (TrrCnt.bFrHlfCyl
    //         || TrrCnt.bBkHlfCyl)
    // {
    //     TrrCnt.bAdjFlag = TRUE;
    // }
    // else
    // {
    //     TrrCnt.bAdjFlag = FALSE;
    // }

    // TrrCnt.bFreqSetFlag = TRUE;

    // intUnlock(iLockKey);

    return EP_SUCCESS;
}
#endif

/***********************************************************************
* AdcAdjTrr - Adjust the ADC interval
*
* RETURNS: NONE
*
*/
static void AdcAdjTrr(
    uint16_t usTimes	/* 周波计数 */
)
{
    // if(!TrrCnt.bAdjFlag)
    // {
    //     /* Not need to adjust. */
    //     if ((usTimes == 0)
    //             && TrrCnt.bFreqSetFlag)
    //     {
    //         TrrCnt.bFreqSetFlag = FALSE;
    //         *TRR2(TiIMMR_g) = TrrCnt.usStVal;	/* 设定初始值 */
    //     }

    //     return;
    // }

    // if(TrrCnt.bFrHlfCyl)
    // {
    //     /* Front half cycle. */
    //     if(usTimes == 0)
    //     {
    //         /* The first point in front cycle. */
    //         *TRR2(TiIMMR_g)=TrrCnt.usStVal+1;
    //         TrrCnt.bResetFlag = TRUE;
    //     }
    //     else if((usTimes<SamplingNum_g/2) && TrrCnt.bResetFlag)
    //     {
    //         TrrCnt.usValResFrDesHlf--;
    //         if(TrrCnt.usValResFrDesHlf == 0)
    //         {
    //             *TRR2(TiIMMR_g)=TrrCnt.usStVal;
    //             TrrCnt.usValResFrDesHlf=TrrCnt.usValResFrHlf;
    //             TrrCnt.bResetFlag = FALSE;
    //         }
    //     }
    // }

    // if(TrrCnt.bBkHlfCyl)
    // {
    //     /* The back cycle. */
    //     if(usTimes == SamplingNum_g/2)
    //     {
    //         *TRR2(TiIMMR_g)=(uint16_t)TrrCnt.usStVal+1;
    //         TrrCnt.bResetFlag = TRUE;
    //     }
    //     else if((usTimes>SamplingNum_g/2) && TrrCnt.bResetFlag)
    //     {
    //         TrrCnt.usValResBkDesHlf--;
    //         if(TrrCnt.usValResBkDesHlf == 0)
    //         {
    //             *TRR2(TiIMMR_g)=TrrCnt.usStVal;
    //             TrrCnt.usValResBkDesHlf=TrrCnt.usValResBkHlf;
    //             TrrCnt.bResetFlag = FALSE;
    //         }
    //     }
    // }
}

/***********************************************************************
* Timer2_ISR - Timer2 interrupt routine
*
* RETURNS: 无
*
*/
static void Timer2_ISR (void)
{
    static int BusyNum = 0;
    static uint32_t ulADErrCnt = 0;
    int iLockKey;
    int32_t *p = NULL;
    SampDataCur *pDataCur = NULL;
    UINT32 th, tl;
    RD_REG_FUNC_AI *pregf = NULL;

    iLockKey = intLock();

    if (semTake(semQueueSD, WAIT_FOREVER) != OK)
    {
        return ;
    }

//     /* 秒计数 */
    synout_index_second++;
    if(synout_index_second>=SamplingNum_g*50)
    {
        synout_index_second = 0;
    }

    // if (*TER2(TiIMMR_g) & 0x02)
    //     *TER2(TiIMMR_g) = 0xFFFF; 	/* Clear timer2 interrupt status word. */
    // else
    // {
    //     *TER2(TiIMMR_g) = 0xFFFF;	 	/* Clear timer2 interrupt status word. */

// #ifndef EDP01_CA_EXT_BUILD	/* 需要执行手动中断,所以屏蔽 */
//         intUnlock(iLockKey);

//         return;
// #endif
//     }

    /* 扫描任务0驱动DSP */
    if (!bDspDrvMod)
    {
        /* 扫描任务没有创建时不解包 */
        if (!bEnableWrDspBuf_g)
        {
            /* 维持通信 */
            // fcc_ether_poll(GENERAL_NET_B);
            intUnlock(iLockKey);
            semGive(semQueueSD);  /* 释放信号量 */
            return;
        }
    }

    /* BSP挂接类型 */
    // UpdateMbDiSts();

    if (EnQueueSD(&SampDataQ, &pDataCur) == ERROR)
    {
        static uint32_t ulCnt = 0;

        ulCnt++;
        if ((ulCnt&0x3FFFF) == 1)
        {
            LOG_Write(LOG_KERNEL, "采样数据队列满\n", NULL);
        }
        intUnlock(iLockKey);
        semGive(semQueueSD);  /* 释放信号量 */
        return;
    }

    Sam_Counter_Int_g++;  /* 采样十周波计数 */
    Sam_Times_Int_g++;			/* 采样一周波计数 */
    SamNumAdjustInt();		/* 采样计数调整 */
    vxTimeBaseGet(&th, &tl);			/* 采样时刻读取 */
    OptAIMod_g.SamTimebuf[2*Sam_Counter_Int_g] = th;
    OptAIMod_g.SamTimebuf[2*Sam_Counter_Int_g+1] = tl;

    pDataCur->SampCount = Sam_Counter_Int_g;
    pDataCur->SampTimes = Sam_Times_Int_g;



    if (Sam_Counter_Int_g == 0)
    {
        /* 同步 */
        SampDataQ.ulSynTime = TM_Get_usCnt();		/* 同步时间 */
    }

    //AdcAdjTrr(Sam_Times_Int_g);		/* Adjusting timer reference */


    ds1306Delay(1000); 	/* 转换等待延时 */

    p = pDataCur->Data;
    AdcDataRd(p);

    /* 5秒钟没有同步则置同步异常标志
     */
    nSynCunts++;
    if(nSynCunts>SamplingNum_g*50*5)
    {
        nSynCunts = SamplingNum_g*50*5;
        bSysSynFlag=FALSE;
    }

    /* 扫描任务0驱动DSP */
    if (!bDspDrvMod)
    {
        /* 数据窗不满不驱动逻辑图扫描 */
        if (DspInfo.WinFull == 55)
        {
            for (pregf=aregf_g; pregf->pfUser; pregf++)
            {
                if (!--pregf->uiCnt)
                {
                    pregf->uiCnt=pregf->uiPts;
                    pregf->pfUser(pregf->pvParm);
                }
            }
        }
    }

    semGive(semQueueSD);  /* 释放信号量 */
    intUnlock(iLockKey);
}

/* 扩展机箱屏蔽以下函数 */
#ifndef EDP01_CA_EXT_BUILD
/***********************************************************************
* Timer2_ISR - Timer2 interrupt routine
*
* RETURNS: 无
*
*/
static void Timer2_ISR_01 (void)
{
//     static int BusyNum = 0;
//     static uint32_t ulADErrCnt = 0;
//     int iLockKey;
//     int32_t *p = NULL;
//     SampDataCur *pDataCur = NULL;
//     int i;

//     UINT32 th, tl;


//     static BOOL bFirstPoint = TRUE;
//     int32_t nData1;
//     int32_t nData2;
//     int32_t nData3;
//     int32_t line1;
//     int32_t line2;
//     RD_REG_FUNC_AI *pregf = NULL;

//     iLockKey = intLock();   /* 闭锁中断，提前 */

//     if (*TER2(TiIMMR_g) & 0x02)
//         *TER2(TiIMMR_g) = 0xFFFF; 	/* Clear timer2 interrupt status word. */
//     else
//     {
//         *TER2(TiIMMR_g) = 0xFFFF;	 	/* Clear timer2 interrupt status word. */

// #ifndef EDP01_CA_EXT_BUILD	/* 需要执行非手动中断，所以屏蔽 */
//         intUnlock(iLockKey);

//         return;
// #endif
//     }

//     /* 扫描任务0驱动DSP */
//     if (!bDspDrvMod)
//     {
//         /* 扫描任务没有创建时不解包 */
//         if (!bEnableWrDspBuf_g)
//         {
//             /* 维持通信 */
//             (GENERAL_NET_B);
//             intUnlock(iLockKey);
//             return;
//         }
//     }

// #if defined(EDP_01_02_BUILD)
//     /* 进行mega16下行帧发送 */
//     WT_MegaDownDealInINT();
// #endif

//     UpdateMbDiSts();

//     if (EnQueueSD(&SampDataQ, &pDataCur) == ERROR)
//     {
//         static uint32_t ulCnt = 0;

//         ulCnt++;
//         if ((ulCnt&0x3FFFF) == 1)
//         {
//             LOG_Write(LOG_KERNEL, "采样数据队列满\n", NULL);
//         }
//         intUnlock(iLockKey);

//         return;
//     }

//     Sam_Counter_Int_g++;  /* 采样十周波计数 */
//     Sam_Times_Int_g++;			/* 采样一周波计数 */
//     SamNumAdjustInt();		/* 采样计数调整 */
//     vxTimeBaseGet(&th, &tl);			/* 采样时刻读取 */
//     OptAIMod_g.SamTimebuf[2*Sam_Counter_Int_g] = th;
//     OptAIMod_g.SamTimebuf[2*Sam_Counter_Int_g+1] = tl;

//     pDataCur->SampCount = Sam_Counter_Int_g;
//     pDataCur->SampTimes = Sam_Times_Int_g;

//     if (Sam_Counter_Int_g == 0)
//     {
//         /* 同步 */
//         SampDataQ.ulSynTime = TM_Get_usCnt();		/* 同步时间 */
//     }

//     AdcAdjTrr(Sam_Times_Int_g);		/* Adjusting timer reference */

//     if (Sam_Counter_Int_g == 0)
//     {

//         if (bdType_g == BOARD_TYPE_E01)
//         {
//             /* 不处理扩展机箱 */
//             SynMasterExtOut();
//         }
//     }

//     BusyNum = 0;		/* 以下为BUSY信号判断 */
//     while (WrADBusy() && (VER_GetHwBoardSN() != E01_CPU_D_A_200712_BORAD))
//     {
//         /* 在转换时为高 */
//         ds1306Delay(10);	/* 转换等待 */
//         BusyNum++;

//         if (BusyNum >= 100)
//         {
//             SIO_Disable_DO();	/* 闭锁保护 */
//             ulADErrCnt++;
//             if ((ulADErrCnt&0x1FFFF) == 1)
//             {
//                 /* 错误报告 */
//                 if (ENG_MODE == 0)
//                 {
//                     ER_Set_Err(EV_SAMPLE_ERR,
//                                ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
//                                "错误码:%02d\n", AD_TRANSTROM_ERR, 0);
//                 }
//                 else if (ENG_MODE == 1)
//                 {
//                     ER_Set_Err(EV_SAMPLE_ERR,
//                                ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
//                                "Error code:%02d\n", AD_TRANSTROM_ERR, 0);
//                 }
//                 LOG_Write(LOG_KERNEL, "A/D转换时序错误!!\n", NULL);
//             }

//             backrear(&SampDataQ);	/* 返回一点　*/
//             intUnlock(iLockKey);

//             return;
//         }
//     };

//     ds1306Delay(1000); 	/* 转换等待延时 */

//     p = smp_data[(input_Index+ADSMP_SHIFT) % MAXQSIZESAMPDATA];

//     AdcDataRd(p);

//     /* 首点处理 */
//     if (bFirstPoint)
//     {
//         for (i = 0; i<MAXHCHNNUM; i++)
//         {
//             pDataCur->Data[i] = smp_data[input_Index][i];
//         }
//         bFirstPoint = FALSE;
//     }
//     else
//     {
//         UINT16 input_Index2;

//         /* 取前一点 */
//         if (input_Index == 0)
//         {
//             input_Index2 = MAXQSIZESAMPDATA-1;
//         }
//         else
//         {
//             input_Index2 = input_Index-1;
//         }

//         for (i = 0; i<MAXHCHNNUM; i++)
//         {
//             nData1 = smp_data[input_Index2 % MAXQSIZESAMPDATA][i];
//             nData2 = smp_data[(input_Index) % MAXQSIZESAMPDATA][i];
//             nData3 = smp_data[(input_Index+1) % MAXQSIZESAMPDATA][i];
//             line1 = ((FACTOR_TWO*(nData2-nData1)) >> 14)+nData1;
//             line2 = ((FACTOR_TWO*(nData3-nData1) >> 1) >> 14)+nData1;

//             pDataCur->Data[i] = line1+(((line2-line1)*FACTOR_ONE) >> 14);
//         }

//         input_Index++;

//         if (input_Index >= MAXQSIZESAMPDATA)
//         {
//             input_Index = 0;
//         }
//     }

//     /* 扫描任务0驱动DSP */
//     if (!bDspDrvMod)
//     {
//         /* 数据窗不满不驱动逻辑图扫描 */
//         if (DspInfo.WinFull == 55)
//         {
//             for (pregf=aregf_g; pregf->pfUser; pregf++)
//             {
//                 if (!--pregf->uiCnt)
//                 {
//                     pregf->uiCnt=pregf->uiPts;
//                     pregf->pfUser(pregf->pvParm);
//                 }
//             }
//         }

//         /* 更新光差发送缓冲区 */
//         pAdc_Data = pDataCur->Data;
//         if (OPTAD_flag && (DspInfo.WinFull == 55))
//         {
//             WrBuf();
//         }
//     }

//     SamCountCnt++;
//     if (SamCountCnt == DspHandle.ucProcessInterval)
//     {
//         /* 2次发1次*/
//         SamCountCnt = 0;

// #ifdef ETH_QUERY_MODE
//         /*Poll_Goose();*/
// #endif

//         /* 光差发送 */
//         if (OPTAD_flag && (DspInfo.WinFull == 55))			/* 光纵缓冲数据发送 */
//         {
//             AiOpt_Send();
//         }

//         semGive(NewDspData);		/* 释放信号量 */
//     }
//     else
//     {
//         /* 2次避开1次 */

//     }

// #ifdef ZEROENABLE
//     semGive(NewDspDataforZero);
// #endif

//     intUnlock(iLockKey);
}



/***********************************************************************
* Timer2_ISR - Timer2 interrupt routine
*
* RETURNS: 无
*
*/
static void Timer2_ISR_02 (void)
{
//     static int8_t nsyx = 0;
//     int i;

//     int iLockKey;

//     SampDataCur *pDataCur = NULL;

//     UINT32 th, tl;

//     INT32  ndiff=0;
//     RD_REG_FUNC_AI *pregf = NULL;
//     int lCurPacketNum = 0; /* 当前帧数 */

//     iLockKey = intLock();

//     if (*TER2(TiIMMR_g) & 0x02)
//         *TER2(TiIMMR_g) = 0xFFFF; 	/* Clear timer2 interrupt status word. */
//     else
//     {
//         *TER2(TiIMMR_g) = 0xFFFF;	 	/* Clear timer2 interrupt status word. */

// #ifndef EDP01_CA_EXT_BUILD	/* 需要执行手动中断,所以屏蔽 */
//         intUnlock(iLockKey);

//         return;
// #endif
//     }

//     /* 系统同步标志,光纤距离保护影响通道标志
//      * 但实际上通信中断时也判同步
//      */
//     bSysSynFlag = TRUE;

// #ifndef EDP02_GTP_BUILD /*发变组调相机应用中IO_IO_OUT_IO_C管脚交由应用控制*/

//     /* 传统子单元需要发脉冲给E01脉冲板
//      * 从侧数字化发送虚拟秒脉冲给前置板
//      */
//     if (gSmvCfg.Smv_9_1Cfg[0].synPulse)
//     {
//         if((++ulCnttt)>=SamplingNum_g*50)
//         {
//             ulCnttt=0;
//             IoPinOutputHigh(IO_IO_OUT_IO_C, IO_PIN_LOW);
//         }
//         else if(ulCnttt==SamplingNum_g*2)
//         {
//             IoPinOutputHigh(IO_IO_OUT_IO_C, IO_PIN_HIGH);
//         }
//     }
// #endif
//     /* 扫描任务0驱动DSP */
//     if (!bDspDrvMod)
//     {
//         /* 扫描任务没有创建时不解包 */
//         if (!bEnableWrDspBuf_g)
//         {
//             /* 维持通信 */
//             fcc_ether_poll(GENERAL_NET_B);
//             intUnlock(iLockKey);
//             return;
//         }
//     }

// #ifdef EDP_01_02_BUILD
//     /* 进行mega16下行帧发送 */
//     WT_MegaDownDealInINT();
// #endif

//     UpdateMbDiSts();

//     /* 采样率大于48时特殊处理, 如周波96点
//      * 主要用于测控, 主变是48点有动作延时大问题
//      */
//     if ((SamplingNum_g>12) && (!bDspDrvMod))
//     {
//         nsyx++;
//         /* lCurPacketNum = fcc_rx_valid_packets_count_get(SUB_ETHERNET_PACKET_A, DspHandle.ucProcessInterval); */
//         lCurPacketNum = fcc_valid_count_get_with_para(SUB_ETHERNET_PACKET_A, DspHandle.ucProcessInterval, ucArrMatchData, 4, 0);


//         /* DspHandle.ucProcessInterval为预期帧数
//          * 满足预期帧数后一并处理, 提高解包效率
//          */
//         if (lCurPacketNum != aregf_g[0].uiCnt)
//         {
//             /* 查询次数与预期帧数一致时, 如果帧数为0, 说明无数据接收
//              * 此时直接驱动, 保证无数据接收和有数据接收时逻辑图扫描次数一致
//              * 比预期帧数多查询一次, 如仍达不到要求则不再等待
//              * 保证拔线后多余帧的处理
//              */
//             if ((nsyx<aregf_g[0].uiCnt)
//                     || ((nsyx == aregf_g[0].uiCnt) && (lCurPacketNum != 0)))
//             {
//                 /* 其它避开 */
//                 intUnlock(iLockKey);

//                 return;
//             }
//         }

//         nsyx = 0;

//         if ((gSmvCfg.Smv_9_1Cfg[0].receiveType == 3)
//                 || (gSmvCfg.Smv_9_1Cfg[0].receiveType == 9))  /* 允许光差使用查询模式 */
//         {
//             /* 查询A网的以太网帧,处理所有接收帧
//              * 目前只支持一路9-1配置
//              */
//             fcc_ether_poll(SUB_ETHERNET_PACKET_A);
//         }
//         else if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 15)
//         {
//             /* 删除查询SPT总线接收 */
//         }

//         /* 半个周波无效则报通信无效 */
//         if (sSmvData[0].SamCountCnt >= SamplingNum_g/2)
//         {
//             sSmvData[0].bSampFirst = TRUE;
//             if(sSmvData[0].bSmvCommOk)
//             {
//                 sSmvData[0].bSmvCommOk = FALSE;
//                 Smv_Go_CommStat_Chg();
//             }
//         }

//         if(!sSmvData[0].bSampRec)
//         {
//             sSmvData[0].ComBackCountCnt = 0;

//             if(sSmvData[0].bSmvCommOk)
//             {
//                 sSmvData[0].SamCountCnt++;

//                 /* 置FALSE,解析数据帧时判断 */
//                 sSmvData[0].bSampRec = FALSE;

//                 intUnlock(iLockKey);

//                 return;
//             }

//             /* 仅增加当前驱动所需点数 */
//             ndiff = aregf_g[0].uiCnt+1;

//         }
//         else
//         {
//             sSmvData[0].SamCountCnt = 0;

//             if(!sSmvData[0].bSmvCommOk)
//             {
//                 /* 通道恢复连续1/2个周波 */
//                 sSmvData[0].ComBackCountCnt++;
//                 if(sSmvData[0].ComBackCountCnt > (SamplingNum_g/2))
//                 {
//                     sSmvData[0].bSmvCommOk = TRUE;
//                     Smv_Go_CommStat_Chg();
//                 }
//             }
//         }

//         /* 置FALSE,解析数据帧时判断 */
//         sSmvData[0].bSampRec = FALSE;
//     }
//     else /* 24点 */
//     {
//         if ((gSmvCfg.Smv_9_1Cfg[0].receiveType == 3)
//                 || (gSmvCfg.Smv_9_1Cfg[0].receiveType == 9)) /* 支持光差模式 */
//         {
//             /* 查询A网的以太网帧,处理所有接收帧
//              * 目前只支持一路9-1配置
//              */
//             fcc_ether_poll(SUB_ETHERNET_PACKET_A);
//         }
//         else if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 15)
//         {
//             /* 删除查询SPT总线接收 */
//         }

//         /* 12点无效则报通信无效 */
//         if(sSmvData[0].SamCountCnt >=12)
//         {
//             sSmvData[0].bSampFirst = TRUE;
//             if(sSmvData[0].bSmvCommOk)
//             {
//                 sSmvData[0].bSmvCommOk = FALSE;
//                 Smv_Go_CommStat_Chg();
//             }
//         }

//         /* 本点无效
//          * 则不处理本数据点
//          */
//         if(!sSmvData[0].bSampRec)
//         {
//             sSmvData[0].ComBackCountCnt = 0;
//             if(sSmvData[0].bSmvCommOk)
//             {
//                 sSmvData[0].SamCountCnt++;

//                 /* 置FALSE,解析数据帧时判断 */
//                 sSmvData[0].bSampRec = FALSE;

//                 intUnlock(iLockKey);

//                 return;
//             }
//         }
//         else
//         {
//             /* 本点有效
//              * 计算本次处理点数
//              */
//             ndiff = poIec_index-synout_index;
//             if(ndiff<0)
//                 ndiff+=MAXQSIZESAMPDATA;
//             if(ndiff==0)
//             {
//                 /* 无有效点 */

//                 /* 置FALSE,解析数据帧时判断 */
//                 sSmvData[0].bSampRec = FALSE;

//                 intUnlock(iLockKey);
//                 return;
//             }

//             sSmvData[0].SamCountCnt = 0;
//             if(!sSmvData[0].bSmvCommOk)
//             {
//                 /* 通道恢复连续2个周波 */
//                 sSmvData[0].ComBackCountCnt++;
//                 if(sSmvData[0].ComBackCountCnt > SamplingNum_g*2)
//                 {
//                     sSmvData[0].bSmvCommOk = TRUE;
//                     Smv_Go_CommStat_Chg();
//                 }
//             }
//         }

//         /* 置FALSE,解析数据帧时判断 */
//         sSmvData[0].bSampRec = FALSE;
//     }

//     /* 循环处理数据点 */
//     do
//     {
//         if (EnQueueSD(&SampDataQ, &pDataCur) == ERROR)
//         {
//             static uint32_t ulCnt = 0;

//             ulCnt++;
//             if ((ulCnt&0x3FFFF) == 1)
//             {
//                 LOG_Write(LOG_KERNEL, "采样数据队列满\n", NULL);
//             }
//             intUnlock(iLockKey);

//             return;
//         }

//         Sam_Counter_Int_g++;  /* 采样十周波计数 */
//         Sam_Times_Int_g++;			/* 采样一周波计数 */
//         SamNumAdjustInt();		/* 采样计数调整 */
//         vxTimeBaseGet(&th, &tl);			/* 采样时刻读取 */
//         OptAIMod_g.SamTimebuf[2*Sam_Counter_Int_g] = th;
//         OptAIMod_g.SamTimebuf[2*Sam_Counter_Int_g+1] = tl;

//         pDataCur->SampCount = Sam_Counter_Int_g;
//         pDataCur->SampTimes = Sam_Times_Int_g;

//         if (Sam_Counter_Int_g == 0)
//         {
//             /* 同步 */
//             /* if(DspInfo.WinFull == 55) */
//             {
//                 /* 窗满后发送 */
//                 SampDataQ.ulSynTime = TM_Get_usCnt();		/* 同步时间 */
//                 /* RD_Syn_AI_Clk(pvAiModHandle_g, TM_Get_usCnt(), 10*DspInfo.ProcessingNum-1); */
//             }
//         }

//         AdcAdjTrr(Sam_Times_Int_g);		/* Adjusting timer reference */

//         if (Sam_Counter_Int_g == 0)
//         {
//             if (bdType_g == BOARD_TYPE_E01)
//             {
//                 /* 不处理扩展机箱 */
//                 SynMasterExtOut();
//             }
//         }

//         /* 目前只考虑一路9-1接受
//          * 通信中断情况下为了让48点的TIME2也连续4次释放信号量
//          * 通信中断时,值为0,置所有状态标
//          */
//         if(!sSmvData[0].bSmvCommOk)
//         {
//             pDataCur->pData = arrDefaultVal;
//             pDataCur->pStatus = arrDefaultSts;

//             poIec_index=0;
//             synout_index=0;
//             bFirstTime=TRUE;
//             ndiff--;
//         }
//         else
//         {
//             /* 第一次处理,同时有有效数据点 */
//             if ((poIec_index >= 1) && bFirstTime)
//             {
//                 synout_index = 0;

//                 /* 更新硬件通道来源端口号
//                  */
//                 for (i = 0; i<iHwAiChNum_g; i++)
//                 {
//                     if ((phwaich_g[i].ucModCh<HCHNNUM) && phwaich_g[i].pSmv)
//                         phwaich_g[i].ucMstPortNum = cfgGetPortNo(phwaich_g[i].pSmv,
//                                                     &phwaich_g[i].ucSlvPortNum, phwaich_g[i].arrSVID);
//                 }
//                 bFirstTime = FALSE;
//             }

//             /* 计算差值 */
//             ndiff = poIec_index-synout_index;
//             if(ndiff<0)
//                 ndiff+=MAXQSIZESAMPDATA;

//             /* 非首点,同时数据有效 */
//             if ((!bFirstTime) && (ndiff >= 1))
//             {
//                 /* 计算合流 */
//                 for(i=0; i<gSmvSLFCfg.smvNum; i++)
//                 {
//                     int chn = 0;

//                     chn=gSmvSLFCfg.Smv_SLF_Cfg[i].smvDataChn;

//                     if (arrChnUsedFlag[chn])
//                     {
//                         int chn1, chn2, chn3;
//                         UINT32 STS = 0x00;

//                         chn1=gSmvSLFCfg.Smv_SLF_Cfg[i].smvDataChnR1;
//                         chn2=gSmvSLFCfg.Smv_SLF_Cfg[i].smvDataChnR2;
//                         chn3=gSmvSLFCfg.Smv_SLF_Cfg[i].smvDataChnR3;

//                         send_data[synout_index][chn] = 0;

//                         if (chn1 >= 0)
//                         {
//                             /* 极性判断 */
//                             if (bArrPoleFlag[chn1])
//                             {
//                                 send_data[synout_index][chn] += send_data[synout_index][chn1];
//                             }
//                             else
//                             {
//                                 send_data[synout_index][chn] -= send_data[synout_index][chn1];
//                             }
//                             STS |= send_data_sts[synout_index][chn1];
//                         }

//                         if (chn2 >= 0)
//                         {
//                             if (bArrPoleFlag[chn2])
//                             {
//                                 send_data[synout_index][chn]+=send_data[synout_index][chn2];
//                             }
//                             else
//                             {
//                                 send_data[synout_index][chn]-=send_data[synout_index][chn2];
//                             }
//                             STS |= send_data_sts[synout_index][chn2];
//                         }

//                         if (chn3 >= 0)
//                         {
//                             if (bArrPoleFlag[chn3])
//                             {
//                                 send_data[synout_index][chn]+=send_data[synout_index][chn3];
//                             }
//                             else
//                             {
//                                 send_data[synout_index][chn]-=send_data[synout_index][chn3];
//                             }
//                             STS |= send_data_sts[synout_index][chn3];
//                         }
//                         send_data_sts[synout_index][chn] = STS;
//                     }
//                 }

//                 /* 改成配置中读取个数 */

//                 pDataCur->pData = send_data[synout_index];
//                 pDataCur->pStatus = send_data_sts[synout_index];

//                 synout_index++;
//                 if(synout_index>=MAXQSIZESAMPDATA)
//                 {
//                     synout_index = 0;
//                 }
//             }
//             else
//             {
//                 /* 通信刚恢复等待缓冲区填满前
//                  * 全部数据置0
//                  * 因为涉及光纵通信恢复再建立同步
//                  * 为防止报两次通信断及返回
//                  * 此处不置错误标
//                  */

//                 pDataCur->pData = arrDefaultVal;
//                 pDataCur->pStatus = arrDefaultSts;
//             }
//         }

// #ifndef EDP01_CA_EXT_BUILD  /* 扩展机箱不支持虚拟机箱 */

// #ifdef VIRT_BOX   /* if define virtual box. */
//         GetOriginSampData();
//         WrVirtBoxBuf();
// #endif

// #endif

//         /* 扫描任务0驱动DSP */
//         if (!bDspDrvMod)
//         {
//             /* 数据窗不满不驱动逻辑图扫描 */
//             if (DspInfo.WinFull == 55)
//             {
//                 for (pregf=aregf_g; pregf->pfUser; pregf++)
//                 {
//                     if (!--pregf->uiCnt)
//                     {
//                         pregf->uiCnt=pregf->uiPts;
//                         pregf->pfUser(pregf->pvParm);
//                     }
//                 }
//             }

//             /* 更新光差发送缓冲区 */
//             pAdc_Data = pDataCur->pData;
//             if (OPTAD_flag && (DspInfo.WinFull == 55))
//             {
//                 WrBuf();
//             }
//         }

//         SamCountCnt++;
//         if (SamCountCnt == DspHandle.ucProcessInterval)
//         {
//             /* 2次发1次*/
//             SamCountCnt = 0;

// #ifdef ETH_QUERY_MODE
//             /*Poll_Goose();*/
// #endif

//             /* 光差发送 */
//             if (OPTAD_flag && (DspInfo.WinFull == 55))			/* 光纵缓冲数据发送 */
//             {
//                 AiOpt_Send();
//             }

//             semGive(NewDspData);		/* 释放信号量 */
//         }
//         else
//         {
//             /* 2次避开1次 */

//         }

// #ifdef ZEROENABLE
//         semGive(NewDspDataforZero);
// #endif

//         /* 通信中断只赋一次值
//          * 退出do^^^while循环
//          */
//         if((!sSmvData[0].bSmvCommOk)&&(SamplingNum_g<=24))
//         {
//             intUnlock(iLockKey);
//             return;
//         }
//     }
//     while(ndiff>1);

//     intUnlock(iLockKey);
}

/***********************************************************************
* Timer2_ISR - Timer2 interrupt routine
*
* RETURNS: 无
*
*/
static void Timer2_ISR_06 (void)
{
//     int iLockKey;
//     INT16 nTemp;
//     int i;

//     SampDataCur *pDataCur = NULL;

//     UINT32 th, tl;
//     RD_REG_FUNC_AI *pregf = NULL;

//     iLockKey = intLock();		/* 闭锁中断 */

//     if (*TER2(TiIMMR_g) & 0x02)
//         *TER2(TiIMMR_g) = 0xFFFF; 	/* Clear timer2 interrupt status word. */
//     else
//     {
//         *TER2(TiIMMR_g) = 0xFFFF;	 	/* Clear timer2 interrupt status word. */

// #ifndef EDP01_CA_EXT_BUILD	/* 需要执行手动中断，所以屏蔽 */
//         intUnlock(iLockKey);

//         return;
// #endif
//     }

// #ifndef EDP02_GTP_BUILD /*发变组调相机应用中IO_IO_OUT_IO_C管脚交由应用控制*/
//     /* 发送秒脉冲给前置板(CC板) */
//     if ((++ulCnttt) >= SamplingNum_g*50)
//     {
//         ulCnttt = 0;

//         IoPinOutputHigh(IO_IO_OUT_IO_C, IO_PIN_LOW);
//         bSysSynFlag = TRUE;		/* 改成等通信恢复后1秒置 */
//     }
//     else if (ulCnttt == 24)
//     {
//         IoPinOutputHigh(IO_IO_OUT_IO_C, IO_PIN_HIGH);
//     }
// #endif

//     /* 扫描任务0驱动DSP */
//     if (!bDspDrvMod)
//     {
//         /* 扫描任务没有创建时不解包 */
//         if (!bEnableWrDspBuf_g)
//         {
//             /* 维持通信 */
//             fcc_ether_poll(GENERAL_NET_B);
//             intUnlock(iLockKey);
//             return;
//         }
//     }

// #ifdef EDP_01_02_BUILD
//     /* 进行mega16下行帧发送 */
//     WT_MegaDownDealInINT();
// #endif

// #ifndef APP_LINE_SUPPORT  /* 线路保护不支持母板开入更新 */
//     UpdateMbDiSts();
// #endif

//     /* 中断执行循环计数器 */
//     SamCountCnt++;

//     /* 查询网口A
//      * 解析报文数据
//      * 连续解析所有报文
//      */
//     fcc_ether_poll(SUB_ETHERNET_PACKET_A);

//     /* 通信正常的情况下判异常,由bSampRec标志和SamCountCnt结合判断
//      * 定时中断进入时初始化bSampRec为FALSE,解析函数中报文有效则置TRUE
//      * 超过2次置重新采样,通信无效
//      */
//     if (sSmvData[0].SamCountCnt >= 2)
//     {
//         sSmvData[0].bSampFirst = TRUE;
//         if(sSmvData[0].bSmvCommOk)
//         {
//             sSmvData[0].bSmvCommOk = FALSE;
//             Smv_Go_CommStat_Chg();
//         }
//     }

//     /* 采样无效且非第一点
//      */
//     if ((!sSmvData[0].bSampRec) && (!bFirstTime))
//     {
//         sSmvData[0].ComBackCountCnt = 0;
//         if (sSmvData[0].bSmvCommOk)
//         {
//             sSmvData[0].SamCountCnt++;
//             /* 此处未RETURN,是因为用的是锁相算法,故下面对实时数据处理也未用do_while,对于小抖动,缓冲区足够的情况下不会出现问题 */
//             /* 最好考虑用do_while,来处理各种以太网抖动情况 */
//             /* intUnlock(iLockKey); */
//             /* return; */
//         }
//     }
//     else
//     {
//         sSmvData[0].SamCountCnt = 0;
//     }

//     sSmvData[0].bSampRec = FALSE;

//     if (EnQueueSD(&SampDataQ, &pDataCur) == ERROR)
//     {
// #ifndef APP_LINE_SUPPORT /* 线路保护不记录日志 */
//         static uint32_t ulCnt = 0;

//         ulCnt++;
//         if ((ulCnt&0x3FFFF) == 1)
//         {
//             LOG_Write(LOG_KERNEL, "采样数据队列满\n", NULL);
//         }
//         intUnlock(iLockKey);
// #endif

//         return;
//     }

//     Sam_Counter_Int_g++;		/* 采样十周波计数 */
//     Sam_Times_Int_g++;			/* 采样一周波计数 */
//     SamNumAdjustInt();		/* 采样计数调整 */
//     vxTimeBaseGet(&th, &tl);			/* 采样时刻读取 */
//     OptAIMod_g.SamTimebuf[2*Sam_Counter_Int_g] = th;
//     OptAIMod_g.SamTimebuf[2*Sam_Counter_Int_g+1] = tl;

//     pDataCur->SampCount = Sam_Counter_Int_g;
//     pDataCur->SampTimes = Sam_Times_Int_g;

//     if (Sam_Counter_Int_g == 0)
//     {
//         /* 同步 */
//         /* if(DspInfo.WinFull == 55) */
//         {
//             /* 窗满后发送 */
//             SampDataQ.ulSynTime = TM_Get_usCnt();		/* 同步时间 */
//             /* RD_Syn_AI_Clk(pvAiModHandle_g, TM_Get_usCnt(), 10*DspInfo.ProcessingNum-1); */
//         }
//     }

// #ifndef APP_LINE_SUPPORT  /* 线路保护不需要调整采样周期 */
//     AdcAdjTrr(Sam_Times_Int_g);		/* Adjusting timer reference */
// #endif

//     /* #ifndef APP_LINE_SUPPORT */  /* 线路保护不支持扩展机箱 */
//     if (Sam_Counter_Int_g == 0)
//     {
//         if (bdType_g == BOARD_TYPE_E01)
//         {
//             /* 不处理扩展机箱 */
//             SynMasterExtOut();
//         }
//     }
//     /* #endif */

//     /* 通信没有恢复,采样值置0,状态标置0
//      * 置下次采样为首次采样
//      */
//     if (!sSmvData[0].bSmvCommOk)
//     {
//         pDataCur->pData = arrDefaultVal;
//         pDataCur->pStatus = arrDefaultSts;

//         bFirstTime = TRUE;
//     }
//     else
//     {
//         /* 获取通道与通信端口的对应关系
//          * 通信正常后查询一次
//          */
//         if (bFirstTime)
//         {
//             for (i = 0; i<iHwAiChNum_g; i++)
//             {
//                 if ((phwaich_g[i].ucModCh<HCHNNUM) && phwaich_g[i].pSmv)
//                     phwaich_g[i].ucMstPortNum = cfgGetPortNo(phwaich_g[i].pSmv,
//                                                 &phwaich_g[i].ucSlvPortNum, phwaich_g[i].arrSVID);
//             }
//             bFirstTime = FALSE;
//         }

//         nTemp = SmplCntLocal; /* CPU软件不延时 */

//         if (nTemp<0)
//         {
//             nTemp += MAXQSIZESAMPDATA;
//         }

//         /* 处理合流 */
//         for (i = 0; i<gSmvSLFCfg.smvNum; i++)
//         {

//             int chn;

//             chn = gSmvSLFCfg.Smv_SLF_Cfg[i].smvDataChn;

//             if (arrChnUsedFlag[chn])
//             {
//                 int chn1, chn2, chn3;
//                 UINT32 STS = 0x00;

//                 chn1 = gSmvSLFCfg.Smv_SLF_Cfg[i].smvDataChnR1;
//                 chn2 = gSmvSLFCfg.Smv_SLF_Cfg[i].smvDataChnR2;
//                 chn3 = gSmvSLFCfg.Smv_SLF_Cfg[i].smvDataChnR3;
//                 send_data[nTemp][chn]=0;

//                 if (chn1 >= 0)
//                 {
//                     if (bArrPoleFlag[chn1])
//                     {
//                         send_data[nTemp][chn] += send_data[nTemp][chn1];
//                     }
//                     else
//                     {
//                         send_data[nTemp][chn] -= send_data[nTemp][chn1];
//                     }
//                     STS |= send_data_sts[nTemp][chn1];
//                 }

//                 if (chn2 >= 0)
//                 {
//                     if (bArrPoleFlag[chn2])
//                     {
//                         send_data[nTemp][chn] += send_data[nTemp][chn2];
//                     }
//                     else
//                     {
//                         send_data[nTemp][chn] -= send_data[nTemp][chn2];
//                     }
//                     STS |= send_data_sts[nTemp][chn2];
//                 }

//                 if (chn3 >= 0)
//                 {
//                     if (bArrPoleFlag[chn3])
//                     {
//                         send_data[nTemp][chn] += send_data[nTemp][chn3];
//                     }
//                     else
//                     {
//                         send_data[nTemp][chn] -= send_data[nTemp][chn3];
//                     }

//                     STS |= send_data_sts[nTemp][chn3];
//                 }
//                 send_data_sts[nTemp][chn] = STS;

//             }
//         }
//         pDataCur->pData = send_data[nTemp];
//         pDataCur->pStatus = send_data_sts[nTemp];
//     }

//     /* 采样节拍处理
//      * 似乎重复
//      */
//     SmplCntLocal = (++SmplCntLocal)%MAXQSIZESAMPDATA;


//     /* 扫描任务0驱动DSP */
//     if (!bDspDrvMod)
//     {
//         /* 数据窗不满不驱动逻辑图扫描 */
//         if (DspInfo.WinFull == 55)
//         {
//             for (pregf=aregf_g; pregf->pfUser; pregf++)
//             {
//                 if (!--pregf->uiCnt)
//                 {
//                     pregf->uiCnt=pregf->uiPts;
//                     pregf->pfUser(pregf->pvParm);
//                 }
//             }
//         }


//         /* 更新光差发送缓冲区 */
//         pAdc_Data = pDataCur->pData;
//         if (OPTAD_flag && (DspInfo.WinFull == 55))
//         {
//             WrBuf();
//         }
//     }

//     if (SamCountCnt == DspHandle.ucProcessInterval)
//     {
//         /* 2次发1次*/
//         SamCountCnt = 0;

// #ifdef ETH_QUERY_MODE
//         /*Poll_Goose();*/
// #endif

//         /* 光差发送 */
//         if (OPTAD_flag && (DspInfo.WinFull == 55))			/* 光纵缓冲数据发送 */
//         {
//             AiOpt_Send();
//         }

//         semGive(NewDspData);		/* 释放信号量 */
//     }

//     intUnlock(iLockKey);
}

/***********************************************************************
* Timer2_ISR_14 - Timer2 interrupt routine(查询FPGA)
*
* RETURNS: 无
*
*/
static void Timer2_ISR_14(void)
{
//     int iLockKey;
//     SampDataCur *pDataCur = NULL;
//     UINT32 th, tl;
//     INT32 ndiff = 0;
//     RD_REG_FUNC_AI *pregf = NULL;

//     iLockKey = intLock();

//     if (*TER2(TiIMMR_g) & 0x02)
//         *TER2(TiIMMR_g) = 0xFFFF; 	/* Clear timer2 interrupt status word. */
//     else
//     {
//         *TER2(TiIMMR_g) = 0xFFFF;	 	/* Clear timer2 interrupt status word. */

//         intUnlock(iLockKey);

//         return;
//     }

//     /* 系统同步标志,光纤距离保护影响通道标志
//      * 但实际上通信中断时也判同步
//      */
//     bSysSynFlag = TRUE;

// #ifndef EDP02_GTP_BUILD /*发变组调相机应用中IO_IO_OUT_IO_C管脚交由应用控制*/

//     /* 传统子单元需要发脉冲给E01脉冲板
//      * 从侧数字化发送虚拟秒脉冲给前置板
//      */
//     if (gSmvCfg.Smv_9_1Cfg[0].synPulse)
//     {
//         static uint32_t ulCnttt = 0;

//         if ((++ulCnttt) >= SamplingNum_g*50)
//         {
//             ulCnttt = 0;
//             IoPinOutputHigh(IO_IO_OUT_IO_C, IO_PIN_LOW);
//         }
//         else if (ulCnttt == SamplingNum_g*2)
//         {
//             IoPinOutputHigh(IO_IO_OUT_IO_C, IO_PIN_HIGH);
//         }
//     }
// #endif

//     /* 扫描任务0驱动DSP */
//     if (!bDspDrvMod)
//     {
//         /* 扫描任务没有创建时不解包 */
//         if (!bEnableWrDspBuf_g)
//         {
//             /* 维持通信 */
//             fcc_ether_poll(GENERAL_NET_B);
//             intUnlock(iLockKey);
//             return;
//         }
//     }

// #ifdef EDP_01_02_BUILD
//     /* 进行mega16下行帧发送 */
//     WT_MegaDownDealInINT();
// #endif

//     UpdateMbDiSts();

//     /* 查询FPGA数据接收 */
//     fpgaValidDataJudge();

//     /* 本点有效
//      * 计算本次处理点数
//      * 有丢失本次节拍的可能性,下次产生多个节拍
//      */
//     ndiff = poIec_index-synout_index;
//     if (ndiff<0)
//         ndiff += MAXQSIZESAMPDATA;
//     if (ndiff == 0)
//     {
//         /* 无有效点 */
//         intUnlock(iLockKey);
//         return;
//     }

//     /* 循环处理数据点 */
//     do
//     {
//         if (EnQueueSD(&SampDataQ, &pDataCur) == ERROR)
//         {
//             static uint32_t ulCnt = 0;

//             ulCnt++;
//             if ((ulCnt&0x3FFFF) == 1)
//             {
//                 LOG_Write(LOG_KERNEL, "采样数据队列满\n", NULL);
//             }
//             intUnlock(iLockKey);

//             return;
//         }

//         Sam_Counter_Int_g++;  /* 采样十周波计数 */
//         Sam_Times_Int_g++;			/* 采样一周波计数 */
//         SamNumAdjustInt();		/* 采样计数调整 */
//         vxTimeBaseGet(&th, &tl);			/* 采样时刻读取 */
//         OptAIMod_g.SamTimebuf[2*Sam_Counter_Int_g] = th;
//         OptAIMod_g.SamTimebuf[2*Sam_Counter_Int_g+1] = tl;

//         pDataCur->SampCount = Sam_Counter_Int_g;
//         pDataCur->SampTimes = Sam_Times_Int_g;

//         if (Sam_Counter_Int_g == 0)
//         {
//             /* 同步 */
//             SampDataQ.ulSynTime = TM_Get_usCnt();
//         }

//         AdcAdjTrr(Sam_Times_Int_g);		/* Adjusting timer reference */

//         if (Sam_Counter_Int_g == 0)
//         {
//             if (bdType_g == BOARD_TYPE_E01)
//             {
//                 /* 不处理扩展机箱 */
//                 SynMasterExtOut();
//             }
//         }

//         /* 第一次处理,同时有有效数据点 */
//         if ((poIec_index >= 1) && bFirstTime)
//         {
//             synout_index = 0;

//             bFirstTime = FALSE;
//         }

//         /* 计算差值 */
//         ndiff = poIec_index-synout_index;
//         if (ndiff<0)
//             ndiff += MAXQSIZESAMPDATA;

//         /* 非首点,同时数据有效 */
//         if ((!bFirstTime) && (ndiff >= 1))
//         {
//             /* 改成配置中读取个数 */

//             pDataCur->pData = send_data[synout_index];
//             pDataCur->pStatus = send_data_sts[synout_index];

//             synout_index++;
//             if (synout_index >= MAXQSIZESAMPDATA)
//             {
//                 synout_index = 0;
//             }
//         }
//         else
//         {
//             pDataCur->pData = arrDefaultVal;
//             pDataCur->pStatus = arrDefaultSts;
//         }

//         /* 扫描任务0驱动DSP */
//         if (!bDspDrvMod)
//         {
//             /* 数据窗不满不驱动逻辑图扫描 */
//             if (DspInfo.WinFull == 55)
//             {
//                 for (pregf=aregf_g; pregf->pfUser; pregf++)
//                 {
//                     if (!--pregf->uiCnt)
//                     {
//                         pregf->uiCnt=pregf->uiPts;
//                         pregf->pfUser(pregf->pvParm);
//                     }
//                 }
//             }
//         }

//         SamCountCnt++;
//         if (SamCountCnt == DspHandle.ucProcessInterval)
//         {
//             /* 多次发1次*/
//             SamCountCnt = 0;

// #ifdef ETH_QUERY_MODE
//             /*Poll_Goose();*/
// #endif

//             semGive(NewDspData);		/* 释放信号量 */
//         }
//     }
//     while(ndiff>1);

//     intUnlock(iLockKey);
}
#endif

/* 扫描任务0驱动DSP任务接口.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void DSP_Scan_Drv(void)
{
    int usSamCnt;
    SampDataCur *pDataCur = NULL;

#ifndef EDP01_CA_EXT_BUILD
    static uint32_t ulCnt = 0;
#endif

#ifdef EDP02_GTP_BUILD
    float *d_dc_buf_float = NULL;  /* 直流填写地址 */
#endif

    /* 定值更新异常判断
     * 如长期无对时信号, 则主动置脉冲
     */

    ulCnt++;
    if ((ulCnt % g_ulDspPollSetChgThreshold) == 1)
    {
        if (RE_GetLogSetChgSts())
        {
            g_ulPollSetChgCnt++;
            if (g_ulPollSetChgCnt>g_ulPollSetChgThreshold)
            {
                TM_SetSecPluseSts(TRUE);
            }
        }
    }

#ifdef ETH_QUERY_MODE
    // Poll_Goose();
#endif

    /* 非扩展机箱 */
#ifndef EDP01_CA_EXT_BUILD
#ifndef APP_LINE_SUPPORT  /* 线路保护不支持DCU */
    /* 向DCU发送通道延时
     */
    if (bMUDelaySndFlag)
    {
        static uint32_t nCnt = 0;
        if ((++nCnt % ulDCUDelaySndCnt) == 1)
        {
            TMuDelay_Transmit();
        }
    }
#endif
#endif

    for (usSamCnt = 0; usSamCnt<DspHandle.ucProcessInterval; usSamCnt++)
    {

        if (semTake(semQueueSD, WAIT_FOREVER) != OK)
        {
        return;
        }
        if (DeQueueSD(&SampDataQ, &pDataCur) == ERROR)
        {
#ifndef APP_LINE_SUPPORT  /* 线路保护不记录日志 */
            static uint32_t ulCnt = 0;

            ulCnt++;
            if ((ulCnt&0x3FFFF) == 1)
            {
                LOG_Write(LOG_KERNEL, "采样数据队列空!!\n", NULL);
            }
#endif
            semGive(semQueueSD);  /* 释放信号量 */
            continue;			/* 无数据则不处理 */
        }

        
        Sam_Counter = pDataCur->SampCount; 		/* 十周波计数，依据使用的点数进行10周波采样统计 */
        Sam_Times = pDataCur->SampTimes;		/* 一周波计数 */

        /* 采样数据传递 */
        if (bDataTransMod)
        {
            pDspResult->pAdcData = pDataCur->pData;
            pDspResult->pChnStatus = pDataCur->pStatus; /* 状态标缓存指针 */
        }
        else
        {
            pDspResult->pAdcData = pDataCur->Data;
            pDspResult->pChnStatus = pDataCur->Status; /* 状态标缓存指针 */
        }

        semGive(semQueueSD);  /* 释放信号量 */

        /* 采样数据记录接口 */
        AdcDataRec(pDspResult->pAdcData);

        Sam_Counter_g++;		/* 采样十周波计数，依据配置的采样点数进行10周波统计 */
        Sam_Times_g++;			/* 采样一周波计数 */
        SamNumAdjust();		/* 采样计数调整 */

        /* 查询是否应更新通道系数
         * 不处理扩展机箱系数更新
         */
        if (SC_Updt_Each_GivenSetting_Decided_Item())
        {
            bExtBoxCoffUpdate = TRUE;
            usCoffUpdateCount = Sam_Counter; /* 更新十周波计数 */
        }

        ulDspAccessCounter_g++;

#ifndef APP_LINE_SUPPORT   /* 线路保护删除多余代码 */
        if (bdType_g == BOARD_TYPE_E01)
        {
            bDspFirstReadAiFlag_g = TRUE;		/* 开始驱动 */
        }
#endif

        if (Sam_Counter == 0)
        {
            /* 同步 */
            RD_Syn_AI_Clk(pvAiModHandle_g, SampDataQ.ulSynTime, 10*pDspInfo->ProcessingNum-1);
        }

        /* 扩展机箱屏蔽以下函数 */
#ifndef EDP01_CA_EXT_BUILD
        /* 添加智能操作箱数据刷新, 必须在本机read_AI_Data函数之前被调用 */
        HDL_Read_AI_Data(Sam_Counter);
#endif

        /* 获取数据填写地址, 更新节拍 */
        pMain = (float *)RD_AI_Dat_P(pvAiModHandle_g,
                                     Sam_Counter,
                                     (COMPLEX **)&(pDspResult->temp_p_complex),
                                     &pDspResult->dcdata);

        /* 扩展机箱屏蔽以下函数 */
#ifndef EDP01_CA_EXT_BUILD
        pStsMain = (uint32_t *)RD_Cnvrt_AI_P_to_Sts_All_P(pMain);

        /* 双缓冲处理 */
#ifndef NO_DBL_BUF
        pDbMain = (float *)((uint8_t *)pMain-lgcaidb_g.ulBufBytes);
        pDbStsMain = RD_Cnvrt_AI_P_to_Sts_All_P(pDbMain);
#endif
#endif

        RealDataModuPretreatment(pDspResult, pDspInfo, pDspHandle);


#if 0  /* 线路保护减少代码, 但处理一个缓冲满标识; 删除APP_LINE_SUPPORT */

        if( pDspInfo->flagfre == 0 )
        {
            /* 延迟(2*RD_BUF_CYC)个周波 */
            static uint16_t s_Sam_Counter = 0;

            s_Sam_Counter++;
            if (s_Sam_Counter >= (2*RD_BUF_CYC)*pDspInfo->XPerShun*pDspInfo->ProcessingNum)
                pDspInfo->flagfre=1;
        }

#else
        if (pDspHandle->PreProcessNumber)
        {
            /* 预处理 */
            pMain = pDspResult->temp_p_complex;
            pDbMain = (float *)((uint8_t *)pMain-calcaidb_g.ulBufBytes);
            DataProcessing(pDspInfo, pDspResult, pDspHandle);   		/* 数字信号处理主体程序 */
        }
#endif

        /* Read over */
        RD_End_Ai_Wr(pvAiModHandle_g);
    }
}

/* DSPProcess - 预处理计算任务入口函数
*
* RETURNS: 无
*
*/
static void DSPProcess()
{
    while(TRUE)
    {
        semTake(NewDspData, WAIT_FOREVER);

        DSP_Scan_Drv();
    }
}
#endif

/***********************************************************************
* AdcDataRd - 采样数据读取
*
* RETURNS: 无
*
*/
static void AdcDataRd(
    int32_t *pData		/* 采样数据保存 */
)
{
// #if defined(EDP_01_02_BUILD) || defined(EDP03_BUILD)
//     int i;
// #endif
//     int j;

//     int32_t *pOrigin;

//     if (VER_GetHwBoardSN() == E01_CPU_D_A_200712_BORAD)
//     {
//         return;
//     }
//     /*
//     	assert(pData);
//     */
//     pOrigin=pData;

// #ifdef EDP03_BUILD
//     for(i=0; i<iAdcChipNum_g; i++)
//     {
//         /* 选择芯片 */
//         for(j=0; j<g_iAdcChnNumPerChip; j++)
//         {
//             /* 选择同一芯片的通道 */
//             *pData++=(int32_t)Get_AD_Value(i+1);
//         }
//     }
// #endif

//     if (bdType_g == BOARD_TYPE_E01)
//     {
//         /* EDP01平台 */
//         pData += 5;
//         for(i=0; i<6; i++)
//         {
//             ulTempData[0][i]=*(CAST(volatile unsigned long *)(0x70001000));
//             *pData = (int16_t)((ulTempData[0][i]&0xFFFF0000)>>16);
//             *(pData+6) = (int16_t)(ulTempData[0][i]&0x0000FFFF);
//             if(i<5)
//                 pData--;
//         }
//         pData += 12;

//         pData += 5;
//         for(i=0; i<6; i++)
//         {
//             ulTempData[1][i]=*(CAST(volatile unsigned long *)(0x70002000));
//             *pData = (int16_t)((ulTempData[1][i]&0xFFFF0000)>>16);
//             *(pData+6) = (int16_t)(ulTempData[1][i]&0x0000FFFF);
//             if(i<5)
//                 pData--;
//         }
//         pData += 12;

//         pData +=5;
//         for(i=0; i<6; i++)
//         {
//             ulTempData[2][i]=*(CAST(volatile unsigned long *)(0x70003000));
//             *pData = (int16_t)((ulTempData[2][i]&0xFFFF0000)>>16);
//             *(pData+6) = (int16_t)(ulTempData[2][i]&0x0000FFFF);
//             if(i<5)
//                 pData--;
//         }
//         pData +=12;

//         pData +=5;
//         for(i=0; i<6; i++)
//         {
//             // ulTempData[3][i]=*(CAST(volatile unsigned long *)(0x70004000));
//             *pData = (int16_t)((ulTempData[3][i]&0xFFFF0000)>>16);
//             if(i<5)
//                 pData--;
//         }
//     }

//     if (bdType_g == BOARD_TYPE_E02)
//     {
//         /* EDP02平台 */
//         for(i=0; i<iAdcChipNum_g; i++)
//         {
//             /* 选择芯片 */
//             for(j=0; j<g_iAdcChnNumPerChip; j++)
//             {
//                 /* 选择同一芯片的通道 */
//                 *pData++ = (int32_t)Get_AD_Value(i+1);
//             }
//         }
//     }
}

/***********************************************************************
* GetAdcData - 获取ADC转换值
*
* RETURNS: 无
*
*/
void GetAdcData(void)
{
    int i,j;

    for(i=0; i<iAdcChipNum_g; i++)
    {
        /* 选择芯片，每个版本不一致 */
        for(j=0; j<g_iAdcChnNumPerChip; j++)
        {
            /* 选择通道 */
            Adc_Data[i*g_iAdcChnNumPerChip+j] = Get_AD_Value(i+1);
            DspResult.AdcData[i*g_iAdcChnNumPerChip+j]=Adc_Data[i*g_iAdcChnNumPerChip+j];		/* 使用DspResult变量 */
        }
    }
}

/* refresh the DC data.
 * Para:
 *     dcdata, address for filling in.
 *     dc_db_data, double buffer.
 * Return:
 *     NONE.
 */
static void RD_Refresh_DC_Data(void *pvAiMod, DSP_LGC_DC_AI_CFG *pdspl_dc_cfg_g, float *dcdata, float *dc_db_data)
{
    DSP_LGC_DC_AI_CFG *pdcch;
    RD_AI_MOD *aimod;
    static uint32_t ulCnt = 0;

#ifndef NO_DBL_BUF
    float ftemp;
#endif

    ulCnt++;
    aimod = (RD_AI_MOD *)pvAiMod;

    for (pdcch = pdspl_dc_cfg_g; pdcch<pdspl_dc_cfg_g+aimod->iDcAiNum; pdcch++)
    {
#ifndef NO_DBL_BUF
        ftemp = (float)SIO_Get_AI(pdcch->pSrc)*pdcch->fRate;
        *dcdata++ = ftemp;
        *dc_db_data++ = ftemp;
#else
        *dcdata++ = (float)SIO_Get_AI(pdcch->pSrc)*pdcch->fRate;
#endif
    }
}

/***********************************************************************
* GetDSPTaskStatus - 获得DSP计算任务的状态，若正常，则返回真，否则，返回假
*
* RETURNS: 无
*
*/
BOOL GetDSPTaskStatus()
{
    static char strTaskStatus[128];

    if(taskIdVerify(nDSPTaskID_g)==ERROR)
    {
        /*首先判定该任务是否有效  */
        return FALSE;
    }

    taskStatusString(nDSPTaskID_g,strTaskStatus);
    if(strcmp(strTaskStatus,"SUSPEND")==0
            ||strcmp(strTaskStatus,"DELAY+S")==0
            ||strcmp(strTaskStatus,"PEND+S")==0
            ||strcmp(strTaskStatus,"PEND+S+T")==0
            ||strcmp(strTaskStatus,"SUSPEND+I")==0
            ||strcmp(strTaskStatus,"DELAY+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+T+I")==0
            ||strcmp(strTaskStatus,"DEAD")==0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/***********************************************************************
* GetZeroExcurTaskStatus - 获得零漂计算任务的状态，若正常，则返回真，否则，返回假
*
* RETURNS: 无
*
*/
BOOL GetZeroExcurTaskStatus()
{
    static char strTaskStatus[128];

    if(taskIdVerify(nZero_ExcurTaskID_g)==ERROR)
    {
        /*首先判定该任务是否有效  */
        return FALSE;
    }

    taskStatusString(nZero_ExcurTaskID_g,strTaskStatus);
    if(strcmp(strTaskStatus,"SUSPEND")==0
            ||strcmp(strTaskStatus,"DELAY+S")==0
            ||strcmp(strTaskStatus,"PEND+S")==0
            ||strcmp(strTaskStatus,"PEND+S+T")==0
            ||strcmp(strTaskStatus,"SUSPEND+I")==0
            ||strcmp(strTaskStatus,"DELAY+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+T+I")==0
            ||strcmp(strTaskStatus,"DEAD")==0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/***********************************************************************
* GetMsuTaskStatus - 获得测量计算任务状态，若正常，则返回真，否则，返回假
*
* RETURNS: 无
*
*/
BOOL GetMsuTaskStatus()
{
    static char strTaskStatus[128];

    if(taskIdVerify(nMsuTaskID_g)==ERROR)
    {
        /*首先判定该任务是否有效  */
        return FALSE;
    }

    taskStatusString(nMsuTaskID_g,strTaskStatus);
    if(strcmp(strTaskStatus,"SUSPEND")==0
            ||strcmp(strTaskStatus,"DELAY+S")==0
            ||strcmp(strTaskStatus,"PEND+S")==0
            ||strcmp(strTaskStatus,"PEND+S+T")==0
            ||strcmp(strTaskStatus,"SUSPEND+I")==0
            ||strcmp(strTaskStatus,"DELAY+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+T+I")==0
            ||strcmp(strTaskStatus,"DEAD")==0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/***********************************************************************
* DataProcessing - DSP调用程序
*
* RETURNS: 无
*
*/
void DataProcessing(
    DSPINFO *pDspInfo,
    DSPRESULT *pDspResult,
    DSPHANDLE *pDspHandle
)
{
    uint8_t i;

    if( pDspInfo->flagfre == 0 )
    {
        /* 延迟(2*RD_BUF_CYC)个周波 */
        static uint16_t s_Sam_Counter = 0;

        s_Sam_Counter++;
        if (s_Sam_Counter >= (2*RD_BUF_CYC)*pDspInfo->XPerShun*pDspInfo->ProcessingNum)
            pDspInfo->flagfre=1;
    }

    if(pDspInfo->flagfre==1)
    {
        /* 预处理 */
        for(i=0; i<pDspHandle->PreProcessNumber; i++)
        {
            /* 多个配置计算 */
            switch ( pDspHandle->pPreAiAddr[i+1].ucArithNum)
            {
                case 3:	/* 递归傅氏*/
                    RecursionDFT(pDspHandle->DspCal+i,
                                 pDspInfo,
                                 pDspResult
                                );
                    break;

                case 4:		/* 幅值相角计算*/
                    RecursionDFTAltAngle(pDspHandle->DspCal+i,
                                         pDspInfo,
                                         pDspResult
                                        );
                    break;

                case 5:		/* 递归差分傅氏*/
                    RecursionDFTDif(pDspHandle->DspCal+i,
                                    pDspInfo,
                                    pDspResult
                                   );
                    break;

                default:
                    break;
            }
        }
    }
}

#ifdef ZEROENABLE
/***********************************************************************
* Zero_Excur_Process - 零漂计算任务入口函数
*
* RETURNS: 无
*
*/
static void Zero_Excur_Process()
{
    uint8_t i;
    static uint32_t ulCnt_Zero=0;

    while(1)
    {
        semTake(NewDspDataforZero, WAIT_FOREVER);

        if(DspInfo.ZeroBufFullFlag == 1)
        {
            /* 数据缓冲区写满后计算 */
            ulCnt_Zero++;
            if(ulCnt_Zero%50000==1)
            {
                LOG_Dbg_Msg("Zero_Excur_Process enter. %d\n", LogicChnNumber, 0, 0, 0, 0, 0);
            }

            if(!ZeroCalibrateFlag)
            {
                /* 防止与零漂校准矛盾 */
                for(i=0; i<LogicChnNumber; i++)
                {
                    ZeroExcursionPollCal(i, &DspHandle, &DspInfo);
                }
            }
            else
            {
                taskDelay(1000);		/* 延迟10s*/
            }
            taskDelay(10);		/* 延迟100ms */
            DspInfo.ZeroBufFullFlag = 0;
        }
    }
}
#endif

/***********************************************************************
* ds1306Delay - 延时
*
* RETURNS: 无
*
*/
void ds1306Delay(
    UINT32 time_ns		/* 延时时间，单位ns */
)
{
 struct timespec start, now;
    uint64_t elapsed_ns = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);

    while (elapsed_ns < time_ns) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        elapsed_ns = 
            (now.tv_sec - start.tv_sec) * 1000000000ULL + 
            (now.tv_nsec - start.tv_nsec);
    }
}

/***********************************************************************
* IO_Set_PC28 - 设置PC28功能
*
* RETURNS: 无
*
*/
static int32_t IO_Set_PC28()
{
//     UINT32 immrVal = vxImmrGet(); 			/* 获得内部内存起始地址 */

// #ifdef VXWORKS_ROM
//     *PPARC(immrVal) &= ~PC28;    /*将相应位置0*/
// #else
//     retVal = IO_Set_Port_Fun(IO_PORT_C, PC28, IO_SET_PIN_IO);
// #endif

//     *PDIRC(immrVal) |= PC28; 			/* 设置PC28为输出 */
//     *PODRC(immrVal) &= ~PC28; 							/* 设置PC28为普通输出口(不是开漏输出) */

    return IO_OPERATE_OK;
}

/***********************************************************************
* IO_Set_PC11 - 设置PC11功能
*
* RETURNS: 无
*
*/
static int32_t IO_Set_PC11()
{
//     UINT32 immrVal = vxImmrGet(); 				/* 获得内部内存起始地址 */

// #ifdef VXWORKS_ROM
//     *PPARC(immrVal) &= ~PC11;    /*将相应位置0*/
// #else
//     retVal = IO_Set_Port_Fun(IO_PORT_C, PC11, IO_SET_PIN_IO);
// #endif

//     *PDIRC(immrVal) |= PC11; 				/* 设置PC11为输出 */
//     *PODRC(immrVal) &= ~PC11; 						/* 设置PC11为普通输出口(不是开漏输出) */

    return IO_OPERATE_OK;
}

/***********************************************************************
* IO_Set_PA9 - 设置PA9功能
*
* RETURNS: 无
*
*/
static int32_t IO_Set_PA9()
{
//     UINT32 immrVal = vxImmrGet(); 			/* 获得内部内存起始地址 */

// #ifdef VXWORKS_ROM
//     *PPARA(immrVal) &= ~PA9;    /*将相应位置0*/
// #else
//     retVal = IO_Set_Port_Fun(IO_PORT_A, PA9, IO_SET_PIN_IO);
// #endif

//     *PDIRA(immrVal) |= PA9; 			/* 设置PA9为输出 */
//     *PODRA(immrVal) &= ~PA9; 						/* 设置PA9为普通输出口(不是开漏输出) */

    return IO_OPERATE_OK;
}

/***********************************************************************
* IO_Set_PC24 - 设置PC24功能
*
* RETURNS: 无
*
*/
static int32_t IO_Set_PC24()
{
    // UINT32 immrVal = vxImmrGet(); 			/* 获得内部内存起始地址 */

// #ifdef VXWORKS_ROM
//     *PPARC(immrVal) &= ~PC24;    /*将相应位置0*/
// #else
//     retVal = IO_Set_Port_Fun(IO_PORT_C, PC24, IO_SET_PIN_IO);
// #endif

//     *PDIRC(immrVal) |= PC24; 			/* 设置PC24为输出 */
//     *PODRC(immrVal) &= ~PC24; 						/* 设置PC24为普通输出口(不是开漏输出) */

    return IO_OPERATE_OK;
}

/***********************************************************************
* IO_Begin_Set_AD - 端口初始化
*
* RETURNS: 无
*
*/
void IO_Begin_Set_AD()
{
    IO_Set_PC28();   		/* A/D Start，用于测试，因不能用定时器输出启动A/D转换，需执行代码启动 */

    IO_Set_PC11(); 		/* A/D Reset init */

#ifdef EDP02_CPU200601_BUILD
    IO_Set_PA9();  		/* 2006.01, Enable buffer */
#else

#if defined(EDP02_CPU200604_BUILD)
    IO_Set_PC24();  		/* 2006.04 */
#endif

#endif 		/* 若是CPU-A-A第3版CPU，和第4版CPU，CPU-C-A,等其他CPU，则空操作  张云改过 2007-6-9 */

    IO_Set_PC13();		/* 设置A/D busy信号输入 */
}

#ifdef EDP03_BUILD
/***********************************************************************
* IO_Begin_Set_AD - 端口初始化
*
* RETURNS: 无
*
*/
void IO_Begin_Set_AD()
{
    IO_Set_CS2(); /* A/D CS */
    IO_Set_CS3(); /* D15 */
    IO_Set_CS4(); /* D900 */
}
#endif

/***********************************************************************
* IO_Set_PC13 - 设置PC13功能
*
* RETURNS: 无
*
*/
static int32_t IO_Set_PC13()
{
    // UINT32 immrVal = vxImmrGet(); 			/* 获得内部内存起始地址 */

// #ifdef VXWORKS_ROM
//     *PPARC(immrVal) &= ~PC13;    /*将相应位置0*/
//     *PDIRC(immrVal) &= ~PC13;    /*将相应位置0*/
// #else
//     retVal = IO_Set_Port_Fun(IO_PORT_C, PC13, IO_SET_PIN_IO);
//     IO_Set_Port_Direction(IO_PORT_C, PC13, IO_DIR_IN_BI);  /*设置为输入*/
// #endif

    return IO_OPERATE_OK;
}

/***********************************************************************
* IO_AD_Convert_Instant - 发送ADC转换信号
*
* RETURNS: 无
*
*/
uint32_t IO_AD_Convert_Instant()
{
    int iLockKey;
    // UINT32 immrVal = vxImmrGet(); 			/* 获得内部内存起始地址 */

    // if ((bdType_g == BOARD_TYPE_E02) || (bdType_g == BOARD_TYPE_E03))
    // {
    //     iLockKey = intLock();		/* 闭锁中断 */
    //     *PDATC(immrVal) &= ~PC28;
    //     ds1306Delay(1000);
    //     *PDATC(immrVal) |= PC28;
    //     intUnlock(iLockKey);
    // }
    // else if (bdType_g == BOARD_TYPE_E01)
    // {
    //     IoPinOutputHigh(IO_OUT_AD_CONVERT, IO_PIN_LOW);
    //     ds1306Delay(1000);
    //     IoPinOutputHigh(IO_OUT_AD_CONVERT, IO_PIN_HIGH);
    // }

    return IO_OPERATE_OK;
}

/***********************************************************************
* DSPDataOut - 结果数据显示
*
* RETURNS: 无
*
*/
void DSPDataOut(void)
{
    int i;

    for (i=0; i<100; i++)
        printf("%d	%d\n", i, (int)(DspResult.TempDataBufNoZero[LENGTH_BUFFER+LogicChnNumber*i]*100));
    for (i=0; i<50; i++)
        printf("%d\n", (int)(DspResult.ResultBuf[i]*100));
}

/***********************************************************************
* AdcintDisable - 中断采样
*
* RETURNS: 无
*
*/
void AdcintDisable(void)
{
    // intDisable(INUM_TIMER3);
}

#ifdef MSUDATATEST
/***********************************************************************
* MsuTestOut - 读取测量数据
*
* RETURNS: 无
*
*/
void MsuTestOut(void)
{
    int i;

    AdcintDisable();
    for (i=0; i<10000; i++)
        printf("%d	%f\n", i, ftmpMsu[i]);
}

/***********************************************************************
* MsuTestOut2 - 读取测量数据，用于试验
*
* RETURNS: 无
*
*/
void MsuTestOut2(void)
{
    int i;

    AdcintDisable();		/* 禁止中断 */
    for(i=0; i<10000; i++)
        printf("%d	%f\n",i,ftmpMsu2[i]);
}
#endif

/***********************************************************************
* WrADBusy - A/D转换BUSY信号读取
*
* RETURNS: 无
*
*/
BOOL WrADBusy(void)
{
    // if(IoPinInputHigh(IO_IN_AD_BUSY))	/* AD BUSY输入 */
    //     return TRUE;
    // else
    //     return FALSE;
}

/***********************************************************************
* ADC_CountMaxMin - 计算最大最小值
*
* RETURNS: 无
*
*/
void ADC_CountMaxMin(void)
{
    int i;
    static uint32_t ulCnt = 0;

    ulCnt++;
    for(i=0; i<HCHNNUM; i++)
    {
        if(SampInfo.Max[i] <	DspResult.pAdcData[i])		/* 最大值*/
            SampInfo.Max[i] = DspResult.pAdcData[i];

        if(SampInfo.Min[i] >	DspResult.pAdcData[i])		/* 最小值*/
            SampInfo.Min[i] = DspResult.pAdcData[i];

        if((ulCnt %STATLENGTHNUM) == 1)
        {
            /* 统计10000次 */
            SampInfo.Max[i] = DspResult.pAdcData[i];
            SampInfo.Min[i] = DspResult.pAdcData[i];
        }

        SampInfo.SumVal[i] +=  DspResult.pAdcData[i];		/* 累加 */
        if((ulCnt%STATLENGTHNUM) == 0)
        {
            SampInfo.AvrVal[i] = SampInfo.SumVal[i]/STATLENGTHNUM;		/* 取平均值 */
            SampInfo.SumVal[i] =0;
        }
    }
}

/***********************************************************************
* ADC_CountMaxMinTmp - 临时计算最大最小值
*
* RETURNS: 无
*
*/
void ADC_CountMaxMinTmp(void)
{
    int i;
    static uint32_t ulCnt = 0;

    ulCnt++;
    for(i=0; i<HCHNNUM; i++)
    {
        if(SampInfo.Max[i] <	DspResult.AdcData[i])		/* 最大值*/
            SampInfo.Max[i] = DspResult.AdcData[i];

        if(SampInfo.Min[i] >	DspResult.AdcData[i])		/* 最小值*/
            SampInfo.Min[i] = DspResult.AdcData[i];

        if((ulCnt %STATLENGTHNUM) == 1)
        {
            /* 统计10000次 */
            SampInfo.Max[i] = DspResult.AdcData[i];
            SampInfo.Min[i] = DspResult.AdcData[i];
        }

        SampInfo.SumVal[i] +=  DspResult.AdcData[i];		/* 累加 */
        if((ulCnt%STATLENGTHNUM) == 0)
        {
            SampInfo.AvrVal[i] = SampInfo.SumVal[i]/STATLENGTHNUM;		/* 取平均值 */
            SampInfo.SumVal[i] =0;
        }
    }

#if defined(EDP03_BUILD)

    if(EP_IS_BOOT_SEL())
    {
        if(((ulCnt%STATLENGTHNUM) == 0) && bSampDataShowFlag)
        {
            logMsg("通道号	最大值	最小值	平均值	漂移范围\n",0, 0, 0, 0, 0, 0);
            for(i=0; i<HCHNNUM; i++)
            {
                logMsg("%d:	%d	%d	%d	%d\n", i, SampInfo.Max[i], SampInfo.Min[i],
                       SampInfo.AvrVal[i], fabs(SampInfo.Max[i] - SampInfo.Min[i]), 0);
            }
        }
    }

#endif

}

/***********************************************************************
* GetSampInfo - 获取采样信息
*
* RETURNS: 无
*
*/
void GetSampInfo(SAMPINFO *pSampInfo)
{
    int i;

    for(i=0; i<HCHNNUM; i++)
    {
        pSampInfo->Max[i] = SampInfo.Max[i];
        pSampInfo->Min[i] = SampInfo.Min[i];
        pSampInfo->AvrVal[i] = SampInfo.AvrVal[i];
    }

    pSampInfo->ChnNum = HCHNNUM;
}
#if 0
/***********************************************************************
* RecBufPointerInit -3I0录波指针初始化
*
* RETURNS: 无
*
*/
static void RecBufPointerInit(
    ANALOGBUFHANDLE *pBufHandle,
    DSPHANDLE *pDspHandle
)
{
    int i, j;

    for(i=0; i<4; i++)
    {
        /* 零序电流也是通过采样获得 */
        for(j=0; j<LogicChnNumber; j++)
        {
            if(pBufHandle->AnalogBufInfo[i].ucHdCh == (pDspHandle->LgcBuffer[j].ucHdCh-1))
            {
                pBufHandle->pfTmpBuf[i]=&(pBufHandle->fTmpBuf[j]);
                break;
            }
            assert(j<LogicChnNumber);
        }
    }

    for(i=0; i<3; i++)
    {
        for(j=0; j<LogicChnNumber; j++)
        {
            if(pBufHandle->AnalogBufInfo[4+i].ucHdCh == (pDspHandle->LgcBuffer[j].ucHdCh-1))
            {
                pBufHandle->pfTmpBuf[4+i]=&(pBufHandle->fTmpBuf[j]);
                break;
            }
            assert(j<LogicChnNumber);
        }
    }
    pBufHandle->iRecPointNumCount=0;
    pBufHandle->DataRdEnable=semBCreate(SEM_Q_PRIORITY, 0);		/* 信号灯创建 */
    pBufHandle->RecBufRdFlag = FALSE;
    pBufHandle->RecBufWrFlag = FALSE;
}

/***********************************************************************
* RdRecBuf -读取录波缓冲区
*
* RETURNS: 无
*
*/
EP_STATUS RdRecBuf(
    ANALOGBUFHANDLE **ppBufHandle
)
{
    if(AnalogBufHandle.RecBufWrFlag == TRUE)
    {
        *ppBufHandle=NULL;
        return EP_ERROR;
    }
    else
    {
        AnalogBufHandle.RecBufRdFlag = TRUE;		/* 正在读标志 */
        *ppBufHandle=&AnalogBufHandle;
        return EP_SUCCESS;
    }
}
#endif
/***********************************************************************
* GetBaseChn - 获取基准通道
*
* RETURNS: 无
*
*/
int GetBaseChn()
{
    int i;

    if(DspHandle.MsuNumber>0)
    {
        /* 测量基准通道，当配置了测量通道时，则需要配置基准通道 */
        return 0;
    }
    else
    {
        /* 第一个逻辑通道 */
        for(i=0; i<DspHandle.PreProcessNumber; i++)
        {
            if((DspHandle.DspCal[i].ucArithNum == 3) || (DspHandle.DspCal[i].ucArithNum == 5))
            {
                return DspHandle.DspCal[i].ucBgnLgcCh;
            }
        }
    }

    return -1;
}

/***********************************************************************
* ADReset - 复位A/D采样芯片
*
* RETURNS: 无
*
*/
void ADReset(void)
{
    // if (VER_GetHwBoardSN() == E01_CPU_D_A_200712_BORAD)
    // {
    //     /* 该板无A/D芯片 */
    //     return;
    // }

    // /* 全部调用底层模式处理 */
    // if(IoPinOutputHigh(IO_OUT_AD_RST, IO_PIN_HIGH) != IO_PIN_HIGH)
    // {
    //     /* 锁存 */
    //     if(ENG_MODE == 0)
    //     {
    //         ER_Set_Err(EV_SAMPLE_ERR,
    //                    ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
    //                    "错误码:%02d\n",AD_RESET_ERR,0);
    //     }
    //     else if(ENG_MODE == 1)
    //     {
    //         ER_Set_Err(EV_SAMPLE_ERR,
    //                    ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
    //                    "Error code:%02d\n", AD_RESET_ERR, 0);
    //     }
    //     LOG_Write(LOG_KERNEL,"AD复位失败(1)!!\n", NULL);
    // }

    // ds1306Delay(5000);

    // if(IoPinOutputHigh(IO_OUT_AD_RST, IO_PIN_LOW) != IO_PIN_LOW)
    // {
    //     if(ENG_MODE == 0)
    //     {
    //         ER_Set_Err(EV_SAMPLE_ERR,
    //                    ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
    //                    "错误码:%02d\n",AD_RESET_ERR,0);
    //     }
    //     else if(ENG_MODE == 1)
    //     {
    //         ER_Set_Err(EV_SAMPLE_ERR,
    //                    ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
    //                    "Error code:%02d\n", AD_RESET_ERR, 0);
    //     }
    //     LOG_Write(LOG_KERNEL,"AD复位失败(0)!!\n", NULL);

    // }

    // ds1306Delay(700000);		/* 延时0.7ms */
}

/***********************************************************************
* SetSampDataShowFlag - 设置采样数据显示标志
*
* RETURNS: 无
*
*/
void SetSampDataShowFlag(void)
{
    bSampDataShowFlag=TRUE;
}

/***********************************************************************
* ClearSampDataShowFlag - 清除采样数据显示标志
*
* RETURNS: 无
*
*/
void ClearSampDataShowFlag(void)
{
    bSampDataShowFlag=FALSE;
}

/***********************************************************************
* RE_Get_DSP_Task_Run_State - 获得采样任务运行状态，供看门狗检测使用
*
* RETURNS:
*               TRUE, 表示采样任务工作正常
*               FALSE, 表示采样任务出现异常
*
*/
BOOL RE_Get_DSP_Task_Run_State(
    unsigned char *pucInfo		/* 采样异常时，返回的信息串 */
)
{
    BOOL bTaskIsRunSuccess=TRUE;
    BOOL bTaskRunState;
    static uint32_t ulFirstEnterNotCreateTime;
    static uint32_t ulFirstEnterNotFreeSemTime;
    static char strTaskStatus[128];

    if (bNotDrvDspFlag)
    {
        /* 不驱动DSP任务,默认其正常 */
        return TRUE;
    }

    if(!bDspTaskStartFlag_g)
    {
        /* 若采样任务还未创建 */
        if(!bFirstEnterNotCreateFlag_g)
        {
            /* 设置初次进入的时间 */
            bFirstEnterNotCreateFlag_g=TRUE;
            ulFirstEnterNotCreateTime=TM_Get_usCnt();
        }
        if(TM_Get_usCnt()-ulFirstEnterNotCreateTime>300000000)
        {
            /* 若采样任务5分钟内都没有创建,则认为出错 */
            if(ENG_MODE==0)
                sprintf(pucInfo,"数据处理任务长时间没有创建!!");
            else if(ENG_MODE==1)
                sprintf(pucInfo,"Data processing task failed to be created!!");
            return   FALSE;
        }
        return  TRUE;
    }
    else
    {
        /* 若采样任务已经创建 */
        if(!bFirstEnterNotFreeSemFlag_g)
        {
            /* 设置初次进入的时间 */
            bFirstEnterNotFreeSemFlag_g=TRUE;
            ulFirstEnterNotFreeSemTime=TM_Get_usCnt();
        }
        if((bDspFirstReadAiFlag_g)
                &&(ulDspAccessCounter_g<1))
        {
            /* 若该任务被创建,且采样已经首次驱动,但未正常扫描，则认为出错 */
            if(taskIdVerify(nDSPTaskID_g)==ERROR)
            {
                /* 首先判定该任务是否异常退出 */
                if(ENG_MODE==0)
                    sprintf(pucInfo,"数据处理任务无效,异常退出!!");
                else if(ENG_MODE==1)
                    sprintf(pucInfo,"Data processing task exits for error!!");
                bTaskIsRunSuccess=FALSE;
                goto  DSP_TASK_CHECK_END;
            }
            /* 再判断该任务是否异常挂起 */

            taskStatusString(nDSPTaskID_g,strTaskStatus);
            if(strcmp(strTaskStatus,"SUSPEND")==0
                    ||strcmp(strTaskStatus,"DELAY+S")==0
                    ||strcmp(strTaskStatus,"PEND+S")==0
                    ||strcmp(strTaskStatus,"PEND+S+T")==0
                    ||strcmp(strTaskStatus,"SUSPEND+I")==0
                    ||strcmp(strTaskStatus,"DELAY+S+I")==0
                    ||strcmp(strTaskStatus,"PEND+S+I")==0
                    ||strcmp(strTaskStatus,"PEND+S+T+I")==0
                    ||strcmp(strTaskStatus,"DEAD")==0)
            {
                if(ENG_MODE==0)
                    sprintf(pucInfo,"数据处理任务异常挂起,任务状态是%s!!", strTaskStatus);
                else if(ENG_MODE==1)
                    sprintf(pucInfo,"Data processing task is suspended, now status is %s!!",strTaskStatus);
                bTaskIsRunSuccess=FALSE;
                goto  DSP_TASK_CHECK_END ;
            }

            /* 最后判断该任务是否因未知原因停止驱动 */
            if(ENG_MODE==0)
                sprintf(pucInfo,"数据处理任务运行一段时间后停止运行!!");
            else if(ENG_MODE==1)
                sprintf(pucInfo,"Data processing task stop after a period of time!!");
            bTaskIsRunSuccess=FALSE;
            goto  DSP_TASK_CHECK_END ;
        }

        if((!bDspFirstReadAiFlag_g)
                &&(TM_Get_usCnt()-ulFirstEnterNotFreeSemTime>60000000))
        {
            /* 若任务已被创建,且1分钟内都没有被驱动,则认为出错 */
            if(ENG_MODE==0)
                sprintf(pucInfo,"数据处理任务创建后长时间没运行!!");
            else if(ENG_MODE==1)
                sprintf(pucInfo,"Data processing task has not been driven for a long time after created!!");
            bTaskIsRunSuccess=FALSE;
            goto  DSP_TASK_CHECK_END ;
        }

DSP_TASK_CHECK_END:

        if((!bTaskIsRunSuccess))
        {
            /* 若采样任务运行不正常，则异常 */
            bTaskRunState=FALSE;
        }
        else
        {
            bTaskRunState=TRUE;
        }

        ulDspAccessCounter_g=0;
        return bTaskRunState;
    }
}

/***********************************************************************
* EX_ReInitDspData - 初始化DSP有关数据
*
* RETURNS: 无
*
*/
EP_STATUS EX_ReInitDspData()
{
    STATUS vxsts;

    // vxsts=taskLock();
    // assert(vxsts==OK);

    // bDspTaskStartFlag_g=FALSE;		/* 需要处理当主机箱多次初始化扩展机箱的情况 2006-7-31 */
    // ulDspAccessCounter_g=0;
    // bDspFirstReadAiFlag_g=FALSE;
    // /* iDspInterviewTaskID_g=-1; */
    // bFirstEnterNotCreateFlag_g=FALSE;
    // bFirstEnterNotFreeSemFlag_g=FALSE;

    // vxsts=taskUnlock();
    // assert(vxsts==OK);

    return EP_SUCCESS;
}

/***********************************************************************
* Disable_PC14_Int - 禁止PC14中断
*
* RETURNS: 无
*
*/
void Disable_PC14_Int()
{
    // intDisable(INUM_PC12);			/* 根据MPC8260而来，中断挂接这些底层函数是按照8260来设计的 */
}

/***********************************************************************
* Disable_Timer2_Int - 禁止Timer2中断
*
* RETURNS: 无
*
*/
void Disable_Timer2_Int()
{
    // intDisable(INUM_TIMER2);
}

#ifdef EDP01_CA_EXT_BUILD

/***********************************************************************
* Get_PC14_Value - 获取PC14值
*
* RETURNS: TRUE, or FALSE
*
*/
static BOOL Get_PC14_Value()
{
    if(*PDATC(0xF0000000) & PC14)
    {
        return TRUE;
    }

    return FALSE;
}



/***********************************************************************
* PC14_Int_ISR - PC14中断函数
*
* RETURNS: 无
*
*/
static void PC14_Int_ISR(void)
{
    // static uint32_t ulLstTime;
    // uint32_t ulCurTime;
    // int32_t Interval;
    // int iLockKey;
    // int  i;
    // uint32_t  ulIntvlSum;
    // uint32_t  ulIntvlAvrg;
    // uint32_t  ulTRR2Follow;


    // iLockKey = intLock();
    // ulCurTime=TM_Get_usCnt();
    // *SIPNR_H(0xF0000000) = 0x00080000;

    // if(bFstSynFlag)
    // {
    //     /* 第一次同步 */
    //     bFstSynSignalHaveCome_g = TRUE;

    //     /*初始化锁相缓冲  2011-4-14 ZY*/
    //     synIntvlBufInf_g.iNewSynCnt=0;

    //     bFstSynFlag=FALSE;
    //     bSynSignalIsCome_g = TRUE;
    // }
    // else
    // {
    //     Interval=ulCurTime-ulLstTime;		/* 可以处理反转情况，不需要额外处理 */

    //     if((Interval>(SYN_INTERVAL-SYN_EXCUR_RANGE))
    //             && (Interval<(SYN_INTERVAL+SYN_EXCUR_RANGE)))
    //     {
    //         /* 200ms同步一次，晶振精度为50ppm */
    //         bSynSignalIsCome_g = TRUE;
    //         /*2011-4-14  ZY  */
    //         /*保存同步间隔缓冲，便于平滑  */
    //         synIntvlBufInf_g.ulSynIntvlBuf[synIntvlBufInf_g.iNewSynCnt]=Interval;
    //         synIntvlBufInf_g.iNewSynCnt++;

    //         if(synIntvlBufInf_g.iNewSynCnt==SYN_STAT_SIGN_CNT)
    //         {
    //             ulIntvlSum=0;
    //             for(i=0; i<SYN_STAT_SIGN_CNT; i++)
    //             {
    //                 /*test  */
    //                 //logMsg("buf  Cur intvl  is  %d \n",synIntvlBufInf_g.ulSynIntvlBuf[iPos],0,0,0,0,0);
    //                 ulIntvlSum=ulIntvlSum+synIntvlBufInf_g.ulSynIntvlBuf[i];/*200毫秒间隔时,不会溢出*/

    //             }
    //             ulIntvlAvrg=ulIntvlSum/SYN_STAT_SIGN_CNT;

    //             /*锁相调整TRR的值 */
    //             ulTRR2Follow=(uint32_t)(((uint64_t)ulIntvlAvrg*(uint64_t)ulTRR2Base_g+(uint64_t)(SYN_INTERVAL/2))/(uint64_t)(SYN_INTERVAL));
    //             /* test */
    //             //logMsg("intvl avrg is %u,Follow  is  %u,base is %d cnt is %d \n",ulIntvlAvrg,ulTRR2Follow,ulTRR2Base_g,synIntvlBufInf_g.iNewSynCnt,0,0);
    //             *TRR2(TiIMMR_g)=(uint16_t)ulTRR2Follow;
    //             bFstPhsFollowSucc_g=TRUE;

    //             synIntvlBufInf_g.iNewSynCnt=0;/* 统计后清零 */
    //         }

    //         if(bFstPhsFollowSucc_g&&(!bFstSynFlagAfterPhsFollow_g))
    //         {
    //             /*锁相成功后的首次同步信号到达,设置标志,必须调用中断  */
    //             bFstSynFlagAfterPhsFollow_g=TRUE;

    //             *TCN2(0xF0000000)=0;
    //             *TER2(TiIMMR_g) = 0xFFFF; 			/* Clear timer2 interrupt status word. */
    //             AutoADConvertPulse();
    //             Timer2_ISR(); 		/* 手动执行一次,本次会导致第1次更新Sam_Counter_Int_g变量 */

    //         }
    //         else  if(bFstSynFlagAfterPhsFollow_g)
    //         {
    //             if(*TCN2(0xF0000000)>(*TRR2(0xF0000000)/2))
    //             {
    //                 /* 计数值大于参考值的一半 */
    //                 *TCN2(0xF0000000)=0;
    //                 AutoADConvertPulse();		/* 转换脉冲 */
    //                 Timer2_ISR();  		/* 手动执行一次 */
    //             }
    //             else
    //             {
    //                 *TCN2(0xF0000000)=0;
    //             }
    //         }
    //     }
    //     else
    //     {
    //         /* 无效同步点，则忽略,不告警 2011-4-14  ZY*/
    //         /* test */
    //         /*
    //         static  uint32_t  ulInvalidSignCnt_s=0;
    //         ulInvalidSignCnt_s++;
    //         if((ulInvalidSignCnt_s%10)==0)
    //         {
    //            	  logMsg("Invalid intvl  is %u, \n",Interval,0,0,0,0,0);
    //         }
    //         */

    //     }
    // }

    // ulLstTime=ulCurTime;
    // intUnlock(iLockKey);
}

/* 同步信号监视
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
static BOOL synPoll(void)
{

    // static uint32_t ulLstJudgeTime = 0;    /* 上次自检时间 */
    // uint32_t ulCurTime;  /* 当前时间 */

    // if (bEnableAlarm_g)
    // {
    //     /* 告警闭锁开出则返回 */
    //     return FALSE;
    // }

    // ulCurTime = TM_Get_usCnt();
    // if (bFstSynSignalHaveCome_g)
    // {
    //     /* 初次同步信号已到达 */
    //     if ((ulCurTime - ulLstJudgeTime) >= SYN_POLL_INTERVAL)
    //     {
    //         /* 如果同步早于本函数执行,则同步到达标志已置 */
    //         ulLstJudgeTime = ulCurTime;
    //         if (bSynSignalIsCome_g)
    //         {
    //             /* 有同步信号到达则清除该标志 */
    //             bSynSignalIsCome_g = FALSE;
    //         }
    //         else
    //         {
    //             /* 无同步信号则报错并记录 */
    //             if (!bSynInterruptFlag)
    //             {
    //                 bSynInterruptFlag = TRUE;
    //                 if (ENG_MODE == 0)
    //                 {
    //                     ER_Set_Err(EV_SAMPLE_ERR,
    //                                ER_REPORT | ER_ALARM | ER_LOCK,
    //                                "同步信号长时间中断或非法\n", 0, 0);
    //                 }
    //                 else if (ENG_MODE == 1)
    //                 {
    //                     ER_Set_Err(EV_SAMPLE_ERR,
    //                                ER_REPORT | ER_ALARM | ER_LOCK,
    //                                "Synchronization interrupted\n", 0, 0);
    //                 }
    //             }
    //             return FALSE;
    //         }
    //     }/* if ((ulCurTime - ulLstJudgeTime) >= SYN_POLL_INTERVAL)结束 */
    // }
    // else
    // {
    //     /* 初次同步还未到达 */
    //     ulLstJudgeTime = ulCurTime; /* 同步晚于本函数执行时,使用其最后一次赋值 */
    //     if ((ulCurTime - tmExtInitTime_g) >= SYN_FST_POLL_INTERVAL)
    //     {
    //         if (!bSynNotComeFlag)
    //         {
    //             bSynNotComeFlag = TRUE;
    //             if(ENG_MODE == 0)
    //             {
    //                 ER_Set_Err(EV_SAMPLE_ERR,
    //                            ER_REPORT | ER_ALARM | ER_LOCK,
    //                            "长时间无同步信号\n", 0, 0);
    //             }
    //             else if(ENG_MODE == 1)
    //             {
    //                 ER_Set_Err(EV_SAMPLE_ERR,
    //                            ER_REPORT | ER_ALARM | ER_LOCK,
    //                            "No Synchronization\n", 0, 0);
    //             }
    //         }
    //     }
    //     return FALSE;
    // }

    // /* 等待锁相成功后的第1次同步信号,若不成功,则不进行后续处理 2011-4-14  ZY*/
    // if (!bFstSynFlagAfterPhsFollow_g)
    // {
    //     return FALSE;
    // }

    return TRUE;
}
/***********************************************************************
* reg_PC14_Int - PC14中断函数挂接
*
* RETURNS: 无
*
*/
static void reg_PC14_Int()
{
    // intConnect(INUM_TO_IVEC(INUM_PC12), PC14_Int_ISR, 0);		/* 根据MPC8260而来 */
}

/***********************************************************************
* Clear_SIPNR_PC14 - PC14中断标志清除
*
* RETURNS: 无
*
*/
static void Clear_SIPNR_PC14()
{
    // *SIPNR_H(0xF0000000) = 0x00080000;
}

/***********************************************************************
* Enable_PC14_Int - 使能PC14中断
*
* RETURNS: 无
*
*/
static void Enable_PC14_Int()
{
    // *SIPNR_H(0xF0000000) = 0x00080000;
    // *SIEXR(0xF0000000)      |=  0x00080000;			/* 不能用PC14 */
    // intEnable(INUM_PC12);			/* 由MPC8260而来 */
}

/***********************************************************************
* Set_PC14_18_PIN - 设置PC14&PC18
*
* RETURNS: 无
*
*/
static void Set_PC14_18_PIN()
{
    // *PPARC(0xF0000000) &= ~(PC14|PC18);
    // *PDIRC(0xF0000000) &= ~(PC14);
    // *PDIRC(0xF0000000) &= ~(PC18);		/* 开入 */
}

/***********************************************************************
* AutoADConvertPulse - 手动AD转换信号
*
* RETURNS: 无
*
*/
static void AutoADConvertPulse(void)
{
    // IO_Set_PC28();		/* 设普通IO口输出 */

    // IO_AD_Convert_Instant();			/* A/D转换开始信息，第一次采样的参考电压为+10V，避开 */
    // ds1306Delay(1000);

    // /* 以下设功能IO口 */
    // *PPARC (TiIMMR_g) |= PC28;       /* Dedicated peripheral function */
    // *PSORC (TiIMMR_g) |= 0;       /* Option 1 */
    // *PDIRC (TiIMMR_g) |= PC28;     /* Output */
}

#endif

/***********************************************************************
* SynMasterExtInit - 同步主从机箱初始化
*
* RETURNS: 无
*
*/
static void SynMasterExtInit(void)
{
// #ifdef EDP01_CA_EXT_BUILD			/* 扩展机箱 */

//     Set_PC14_18_PIN();

//     logMsg("PC14=%x\n", Get_PC14_Value(), 0, 0, 0, 0, 0);
//     Clear_SIPNR_PC14();
//     reg_PC14_Int();
//     Enable_PC14_Int();

// #else					/* 主机箱 */

//     UINT32 immrVal = vxImmrGet(); 			/* 获得内部内存起始地址 */

// #ifdef VXWORKS_ROM
//     *PPARC(immrVal) &= ~MASTEREXTSYN;    /*将相应位置0*/
// #else
//     retVal = IO_Set_Port_Fun(IO_PORT_C, MASTEREXTSYN, IO_SET_PIN_IO);
// #endif

//     *PDIRC(immrVal) |= MASTEREXTSYN; 				/* 设置为输出 */
//     *PODRC(immrVal) &= ~MASTEREXTSYN; 				/* 设置为普通输出口(不是开漏输出) */
// #endif
}

/***********************************************************************
* SynMasterExtOut - 同步主从机箱初始化
*
* RETURNS: 无
*
*/
static void SynMasterExtOut(void)
{
    // UINT32 immrVal = vxImmrGet(); 			/* 获得内部内存起始地址 */
    // int iLockKey;

    // iLockKey = intLock();		/* 闭锁中断 */
    // *PDATC(immrVal) &= ~MASTEREXTSYN;		/* 拉低 */
    // ds1306Delay(20000);
    // *PDATC(immrVal) |= MASTEREXTSYN;						/* 拉高 */
    // intUnlock(iLockKey);
}

BOOL bPtIn1=FALSE;
BOOL bPtIn2=FALSE;
BOOL bPtIn3=FALSE;
BOOL bPtIn4=FALSE;

BOOL Get_Ft3Pt1Sts(void)
{
    if(bPtIn1)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL Get_Ft3Pt2Sts(void)
{
    if(bPtIn2)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL Get_Ft3Pt3Sts(void)
{
    if(bPtIn3)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL Get_Ft3Pt4Sts(void)
{
    if(bPtIn4)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



#ifdef EDP02_PSR_BUILD
#define FPGA_BITSTREAM_FILE_NAME_STD  "/tffs/cpu_fpga_sample_std.bin"
#define FPGA_BITSTREAM_FILE_NAME_SAC  "/tffs/cpu_fpga_sample_sac.bin"
int Config_FPGA(char *fileName){
    return -1;
}
void Write_FPGA_Program_Auto()
{
    // if(gSmvFT3Cfg.Smv_FT3_Cfg[0].smvType==1)//自定义FT3
    // {
    //     Config_FPGA(FPGA_BITSTREAM_FILE_NAME_SAC);
    //     printf("        Read file:cpu_fpga_sample_sac.bin OK\n");
    // }
    // else if (gSmvFT3Cfg.Smv_FT3_Cfg[0].smvType==2)//标准FT3
    // {
    //     Config_FPGA(FPGA_BITSTREAM_FILE_NAME_STD);
    //     printf("        Read file:cpu_fpga_sample_std.bin OK\n");
    // }
}
#endif

/* 初始化传统子单元发送缓冲区.
 * Para:
 *     smvNo, ASDU数.
 *     bufNo, Buffer块号.
 * Return:
 *     NONE.
 */
void init_Smv92STD_SubSend(int smvNo, int bufNo)
{
    // UINT8 *pSendBuf1=nSmv92Data1[bufNo];
    // UINT8 *pData2=NULL;
    // UINT8 *pData3=NULL;
    // UINT8 *pData4=NULL;
    // UINT8 *pData5=NULL;
    // UINT8 *pTemp=NULL;
    // UINT8 nData1;
    // UINT8 nData2 = 0;
    // UINT16 nData3;
    // UINT16 nData4;
    // UINT16 nData5;
    // int nsvIdLength,i,asduc;
    // UINT8 svId[20]= {0x01,0x0c,0xcd,0x04,0x00,0x01};

    // *pSendBuf1++ = 0x01;
    // *pSendBuf1++ = 0x0c;
    // *pSendBuf1++ = 0xcd;
    // *pSendBuf1++ = 0x04;
    // *pSendBuf1++ = 0x01;
    // *pSendBuf1++ = 0x00;

    // *pSendBuf1++ = 0x01;
    // *pSendBuf1++ = 0x0c;
    // *pSendBuf1++ = 0xcd;
    // *pSendBuf1++ = 0x04;
    // *pSendBuf1++ = 0x01;
    // *pSendBuf1++ = 0x00;

    // *pSendBuf1++ = 0x81;  /* TPID */
    // *pSendBuf1++ = 0x00;

    // *pSendBuf1 = 0x80;	/* TCI */
    // *pSendBuf1++ |= (0x0001 >> 8) & 0x0F;   /* VLAN */
    // *pSendBuf1++ = (0x0001 >> 0) & 0x0F;  /* VLAN */

    // *pSendBuf1++ = 0x88;	/* ETHERNET TYPE */
    // *pSendBuf1++ = 0xBA;

    // *pSendBuf1++ = (0x4000 >> 8) & 0xFF;  /* APPID c */
    // *pSendBuf1++ = (0x4000 >> 0) & 0xFF;   /* b */

    // /* 从APPID开始的字节数,可变nData5 */
    // pData5=pSendBuf1;
    // *pSendBuf1++ = 0x00;  /* LENGTH a */
    // *pSendBuf1++ = 0x00;   /* 9 */

    // *pSendBuf1++ = 0x00;	/* RESERVED 1 8 */
    // *pSendBuf1++ = 0x00;  /* 7 */

    // *pSendBuf1++ = 0x00;	/* RESERVED 2 6 */
    // *pSendBuf1++ = 0x00;   /* 5 */

    // *pSendBuf1++ = 0x60;	/* APDU TAG 4 */
    // *pSendBuf1++ = 0x82;	/* APDU TAG 3 */

    // pData4=pSendBuf1;

    // /* TLV,可变,nData4 */
    // *pSendBuf1++ = 0x00;	//APDU LENGTH  2
    // *pSendBuf1++ = 0x00;	//APDU LENGTH  1

    // *pSendBuf1++ = 0x80;	//ASDU NO TAG 7
    // *pSendBuf1++ = 0x01;	//ASDU NO LENGTH 6
    // *pSendBuf1++ = smvNo+1;	//ASDU NO 5

    // *pSendBuf1++ = 0xA2;	//ASDU SEQ TAG 4
    // *pSendBuf1++ = 0x82;	// 3

    // /* TLV,可变,nData3 */
    // pData3=pSendBuf1;
    // *pSendBuf1++ = 0x00;	//ASDU SEQ LENGTH  重新计算2
    // *pSendBuf1++ = 0x00;	//ASDU SEQ LENGTH  重新计算1

    // for (asduc=0; asduc<=smvNo; asduc++)
    // {
    //     *pSendBuf1++ = 0x30;	//ASDU TAG 3
    //     *pSendBuf1++=  0x81;  //ASDU 长度肯定超过127 22通道9-2 2

    //     /* TLV,可变,nData2 */
    //     pData2=pSendBuf1;
    //     *pSendBuf1++ = 0x00;	//ASDU LENGTH 1
    //     pTemp=pSendBuf1;

    //     *pSendBuf1++ = 0x80;	//SVID TAG
    //     nsvIdLength=6;      //svID长度
    //     svId[5]=asduc+1; /* 从1开始 */
    //     *pSendBuf1++ = nsvIdLength;	//SVID LENGTH
    //     for(i=0; i<nsvIdLength; i++)
    //     {
    //         *pSendBuf1++ = svId[i];
    //     }

    //     *pSendBuf1++ = 0x82;	//SMPCNT TAG
    //     *pSendBuf1++ = 0x02;	//SMPCNT LENGTH
    //     nAddatCnt92[bufNo][asduc]=pSendBuf1-nSmv92Data1[bufNo];
    //     *pSendBuf1++ = 0x00;	//SMPCNT
    //     *pSendBuf1++ = 0x00;

    //     *pSendBuf1++ = 0x83;	//REV TAG
    //     *pSendBuf1++ = 0x04;	//REV LENGTH
    //     *pSendBuf1++ = 0x00;	//REV
    //     *pSendBuf1++ = 0x00;
    //     *pSendBuf1++ = 0x00;
    //     *pSendBuf1++ = 0x01;

    //     *pSendBuf1++ = 0x85;	//SYN TAG
    //     *pSendBuf1++ = 0x01;	//SYN LENGTH
    //     nAddatSyn92[bufNo][asduc]=pSendBuf1-nSmv92Data1[bufNo];
    //     *pSendBuf1++ = 0x00;	//SYN

    //     *pSendBuf1++ = 0x87;	//ASDU DATA TAG

    //     /* TLV,可变 */
    //     nData1=22*8;//gSmvTXCfg.Smv_TX_Cfg[asduc].dataNum*8;
    //     *pSendBuf1++ = 0x81;
    //     *pSendBuf1++ = nData1;//ASDU DATA LENGTH
    //     nAddatData92[bufNo][asduc] = pSendBuf1-nSmv92Data1[bufNo];
    //     nData2=nData1+pSendBuf1-pTemp;
    //     *pData2=nData2;//ASDU LENGH
    //     for(i=0; i<22*8; i++)
    //     {
    //         *pSendBuf1++=0x0;
    //     }
    // }

    // nData3=(nData2+0x03)*(smvNo+1);//都带81所以多3
    // *pData3++=(nData3>>8)&0xFF;
    // *pData3++=(nData3>>0)&0xFF;

    // nData4=nData3+0x07;//都带82 81所以多7
    // *pData4++=(nData4>>8)&0xFF;
    // *pData4++=(nData4>>0)&0xFF;

    // nData5=(UINT16)nData4+0x0c;
    // *pData5++=(nData5>>8)&0xFF;
    // *pData5    =(nData5>>0)&0xFF;
}

/* 报文发送.
 * Para:
 *     smvNo, 实际ASDU数(最多允许6个).
 *     pData, 数据源.
 * Return:
 *     NONE.
 */
void smv92STD_SubsendLogic(int smvNo, UINT8 *pData)
{

    // UINT8 *pData1=NULL;
    // UINT8 i,asduc;
    // static int bufNo = 0;

    // for(asduc=0; asduc<=smvNo; asduc++)
    // {
    //     pData1 = nSmv92Data1[bufNo]+nAddatData92[bufNo][asduc];

    //     /* 填写6个数据 */
    //     for(i=0; i<6; i++)
    //     {
    //         INT16 Value=(*(pData+0)<<8)|*((pData+1));
    //         INT32 Value32=Value*1;

    //         *(pData1+0) = Value32>>24;
    //         *(pData1+1) = Value32>>16;
    //         *(pData1+2) =Value32>>8;
    //         *(pData1+3) =Value32;

    //         *(pData1+7) = 0x00;
    //         pData1 += 8;
    //         pData += 2;
    //     }

    //     pData1 = nSmv92Data1[bufNo]+nAddatSyn92[bufNo][asduc];//退回到同步字节
    //     *pData1=bSysSynFlag;

    //     pData1 = nSmv92Data1[bufNo]+nAddatCnt92[bufNo][asduc];//退回到采样计数器字节
    //     if(SamplingNum_g==24)
    //     {
    //         *pData1 = ((synout_index_second)>>8)&0xFF;
    //         *(pData1+1) = ((synout_index_second)>>0)&0xFF;
    //     }
    //     else if(SamplingNum_g==48)
    //     {
    //         *pData1 = ((synout_index_second/2)>>8)&0xFF;
    //         *(pData1+1) = ((synout_index_second/2)>>0)&0xFF;
    //     }

    // }

    // /* 发送
    //  * 48点时隔点发送1次
    //  */
    // if(SamplingNum_g==24)
    // {
    //     goose_send_raw_no_buf_cpy(0, nSmv92Data1[bufNo], nAddatData92[bufNo][smvNo]+8*22);
    // }
    // else if (SamplingNum_g==48)
    // {
    //     if(synout_index_second%2==0)
    //     {
    //         goose_send_raw_no_buf_cpy(0, nSmv92Data1[bufNo], nAddatData92[bufNo][smvNo]+8*22);
    //     }
    // }

    // /* 缓冲4个报文帧 */
    // ++bufNo;
    // bufNo = (bufNo) & 0x3;
}

/***********************************************************************
* eint1_routine - 外部中断1响应
*
* RETURNS: 无
*
*/
void eint1_routine(void)
{
#define MAX_WR_LOG_NUM 10  /* 最多记录日志次数 */

    // static BOOL bFstSyn=TRUE;
    // static uint32_t uscnt=0;
    // static uint32_t ulSynCnt = 0;  /* 同步总次数 */
    // uint32_t ulCurCnt = 0;
    // uint32_t deltaus = 0;
    // UINT32 nTCN2;
    // UINT32 nTCN4;
    // int iLockKey;

    // ulSynCnt++;
    // iLockKey = intLock();

    // /* 获取同步间隔 */
    // ulCurCnt = TM_Get_usCnt();
    // deltaus = ulCurCnt-uscnt;
    // uscnt = ulCurCnt;

    // /* 获取Timer2/Timer4当前计数 */
    // nTCN4=*TCN4(0xF0000000);
    // nTCN2=*TCN2(0xF0000000);

    // /* 同步恢复后重新开始初始处理
    //  */
    // if(!bSysSynFlag)
    // {
    //     bFstSyn = TRUE;
    // }

    // /* 清除中断标志 */
    // if(*SIPNR_H(0xF0000000) & 0x00004000)
    //     *SIPNR_H(0xF0000000) = 0x00004000;

    // /* 同步间隔判断,第一次不判 */
    // if (((deltaus<999500) || (deltaus>1000500)) && (!bFstSyn))
    // {
    //     char TempInfo[256];
    //     static BOOL bWrLogFlag = FALSE;
    //     static uint32_t ulWrLogCnt = 0;

    //     if (!bWrLogFlag)
    //     {
    //         ulWrLogCnt++;
    //         if (ulWrLogCnt>MAX_WR_LOG_NUM)
    //         {
    //             bWrLogFlag = TRUE;
    //         }
    //         sprintf(TempInfo, "同步信号无效,同步间隔%d,同步总次数%d!!", (int)deltaus, (int)ulSynCnt);
    //         LOG_Write(LOG_KERNEL, TempInfo, NULL);
    //     }

    //     intUnlock(iLockKey);

    //     return;
    // }

    // /* 同步后置0,表明有同步信号 */
    // nSynCunts = 0;

    // /* 第一个同步点置同步标志
    //  * 因为第一个同步点已经对同步时刻进行了调整
    //  */
    // bSysSynFlag = TRUE;

    // if((*TCN2(0xF0000000))>(*TRR2(0xF0000000)>>1))
    // {
    //     /* 即将发生定时中断 */
    //     *TCN2(0xF0000000) = *TRR2(0xF0000000)-(100);

    //     *TER2(TiIMMR_g) = 0xFFFF;  /* 清除中断标志,防止重复响应中断 */

    //     if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 0xff)  /* 传统子机 */
    //     {
    //         synout_index_second=SamplingNum_g*50-1;
    //     }
    // }
    // else
    // {
    //     /* 已经发生定时中断 */
    //     *TCN2(0xF0000000) = nTCN4>>4;
    //     if (*TER2(TiIMMR_g) & 0x02)  /* 发生了TM2中断,但还没有得到执行 */
    //     {
    //         synout_index_second=SamplingNum_g*50-1;
    //     }
    //     else
    //     {
    //         if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 0xff)  /* 传统子机 */
    //         {
    //             synout_index_second=0;
    //         }
    //     }
    // }

    // if (bFstSyn)
    // {
    //     bFstSyn = FALSE;
    // }

    // intUnlock(iLockKey);
}


/***********************************************************************
* eint1_enable - 外部中断1使能
*
* RETURNS: 无
*
*/
void eint1_enable(void)
{
    // *SIPNR_H(0xF0000000) = 0x00004000; 		/* Clear */
    // *SIEXR(0xF0000000) |= 0x00004000; 		/* config IRQ1 high-to-low sensitive mode */

    // intConnect( INUM_TO_IVEC( INUM_IRQ1 ), (VOIDFUNCPTR)eint1_routine, 0 );

    // intEnable(INUM_IRQ1);
}


/***********************************************************************
* Set_PC18_PIN - 设置PC18
*
* RETURNS: 无
*
*/
void Set_PC18_PIN(void)
{
    // *TGCR2(0xF0000000) &= ~(TGCR2_RST4 | TGCR2_STP4);			/* stop and reset Timer4 */

    // *PPARC(0xF0000000) |= (PC18);		/* /TGATE2 */
    // *PSORC(0xF0000000) |= (PC18);
    // *PDIRC(0xF0000000) &= ~(PC18);

    // *TGCR2(0xF0000000) &= ~TGCR2_GM2 ;		/* Gate Mode for Pin2 */
    // *TMR4(0xF0000000) = ((1-1) << 8);		/* 1分频 */

    // /* 不需要计算脉宽,不需要使能上升沿触发中断
    //  */
    // *TMR4(0xF0000000)  |= TMR_ICLK_IN_GEN ;
    // *TMR4(0xF0000000)  |= TMR_GE ;

    // *TER4(0xF0000000) = TER_REF | TER_CAP;

    // *TGCR2(0xF0000000) |= TGCR2_RST4 ;

}

/***********************************************************************
* SysSynInit - 系统同步初始化
*
* RETURNS: 无
*
*/
void SysSynInit(void)
{
    Set_PC18_PIN();
    eint1_enable();
}

/* 扩展机箱屏蔽以下函数 */
#ifndef EDP01_CA_EXT_BUILD
/* 获取采样通道延时(点数表示)
 * Para:
 *     NONE.
 * Return:
 *     点数.
 */
uint8_t adcGetDelayTime(void)
{
    if (appType_g == APP_TYPE_DIG)
    {
        /* 数字化应用,
         * 包含CC板总延时和CC板-CPU板通信传递延时两部分,
         * 后者为1个点
         */
        return (uint8_t)(sSmvCCInfo.backpoint+1);

    }
    else if (appType_g == APP_TYPE_TRAD)
    {
        /* 传统应用
         * 缺省为传统应用
         */
        return ADSMP_SHIFT;
    }

    return ADSMP_SHIFT;
}

/* 重新注册采样中断处理函数.
 * Para:
 *     RcvType, 接收类型.
 * Return:
 *     NONE.
 */
void smvChgTransType(uint8_t RcvType)
{
    // if (RcvType == 3)
    // {
    //     /* 计数器处理 */
    //     poIec_index = (SmplCntLocal+1)%MAXQSIZESAMPDATA;
    //     synout_index = poIec_index;

    //     sSmvCCInfo.backPointLimit = MAX_BACK_POINT_TYPE_3;
    //     sSmvCCInfo.comBackCycleLimit = COM_BACK_CYCLE_NUM_TYPE_3;
    //     intConnect(INUM_TO_IVEC(INUM_TIMER2), Timer2_ISR_02, 0);
    // }
    // else if (RcvType == 9)
    // {
    //     /* 计数器处理 */
    //     SmplCntLocal = (poIec_index+1)%MAXQSIZESAMPDATA;

    //     sSmvCCInfo.backPointLimit = MAX_BACK_POINT_TYPE_9;
    //     sSmvCCInfo.comBackCycleLimit = COM_BACK_CYCLE_NUM_TYPE_9;
    //     intConnect(INUM_TO_IVEC(INUM_TIMER2), Timer2_ISR_06, 0);
    // }
}
#endif

/***********************************************************************
* EnQueueSD - 获取队列当前写入位置
*
* RETURNS: OK, ERROR
*
*/
STATUS EnQueueSD(
    SqQueue *Q,
    SampDataCur **p			/* 当前写入点地址，放在队尾 */
)
{
    STATUS ret;

    /* 获取信号量，实现互斥访问 */


    if (((Q->rear+1)%MAXQSIZESAMPDATA) == Q->front)
    {
        // semGive(semQueueSD);  /* 释放信号量 */
        return ERROR;
    }

    *p = Q->base+Q->rear;		/* 当前写入点 */
    Q->rear = (Q->rear+1)%MAXQSIZESAMPDATA;				/* 下一写入点*/


    return OK;
}

/***********************************************************************
* DeQueueSD - 读取队列当前位置
*
* RETURNS: OK, ERROR
*
*/
STATUS DeQueueSD(
    SqQueue *Q,
    SampDataCur **p		/* 读取地址 */
)
{
    /* 获取信号量，实现互斥访问 */


    if(Q->front == Q->rear)
    {
        // semGive(semQueueSD);  /* 释放信号量 */
        return ERROR;
    }

    *p=Q->base+Q->front;		/* 当前读出点 */
    Q->front=(Q->front+1)%MAXQSIZESAMPDATA;				/* 下一读出点 */


    return OK;
}

/* 获取扫描入口时刻节拍
 * Para:
 *     NONE.
 * Return:
 *     uint32_t.
 */
uint32_t RD_GetScanCnt(void)
{
    return (RD_AI_Cnt()+DspHandle.ucProcessInterval);
}
