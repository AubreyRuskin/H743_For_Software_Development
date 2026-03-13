/* hwcfg.h - subroutine library for parsing the hardware configuration */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 11nov06, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for parsing the hardware configuration.
*/

#ifndef HWCFG_H
#define HWCFG_H

#include "semLib.h"
#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "realdatadef.h"
#include "edpbase.h"
#include "math_compat.h"
#include "logmsg.h"
/* #include "dsp.h" */
#include "dspai.h"
#include "swcfg.h"

/* defines */

#define MAX_DA_AO_NUM 50 		/* DA输出的AO配置最大数目 */
#define AI_ADDR 201			/* CPU地址 */
#define I2CCYCLEMAXNUM (120*20)		/* I2C循环写操作最大次数，累计2分钟 */
#define I2CDELAYTICKNUM 3      /* 单次写不成功等待时间 */
#define CHN_ADJ_SERVER 0x01	/* channel ajdusting server. */
#define DEFAULT_CURRENT_FST_RATED_VAL 6250 /* 缺省一次额定电流,A */
#define DEFAULT_CURRENT_SEC_RATED_VAL 500 /* 缺省二次额定电流,A(存储时扩大100倍) */
#define DEFAULT_VOLTAGE_FST_RATED_VAL 500000 /* 缺省一次额定电压,V */
#define DEFAULT_VOLTAGE_SEC_RATED_VAL 5774 /* 缺省二次额定电压,V(存储时扩大100倍) */

/* 输入监视基准和显示门槛 */
#define THD_CURRENT_1A 0.02 /* 1A门槛 */
#define THD_CURRENT_5A 0.1 /* 5A门槛 */
#define THD_VOLT 0.2 /* 电压门槛 */

/* 为线路保护定制的开入 */
#define LINE_CUST_DI1_FLT "闭锁重合闸"
#define LINE_CUST_DI2_FLT "低气压闭锁重合"
#define LINE_CUST_DI3_FLT "G_边断路器位置A相"
#define LINE_CUST_DI4_FLT "G_边断路器位置B相"
#define LINE_CUST_DI5_FLT "G_边断路器位置C相"
#define LINE_CUST_DI6_FLT "G_中断路器位置A相"
#define LINE_CUST_DI7_FLT "G_中断路器位置B相"
#define LINE_CUST_DI8_FLT "G_中断路器位置C相"

#define LINE_CUST_DI9_FLT "G_断路器位置A相"
#define LINE_CUST_DI10_FLT "G_断路器位置B相"
#define LINE_CUST_DI11_FLT "G_断路器位置C相"
#define LINE_CUST_DI12_FLT "G_跳令A1"
#define LINE_CUST_DI13_FLT "G_跳令B1"
#define LINE_CUST_DI14_FLT "G_跳令C1"
#define LINE_CUST_DI15_FLT "G_跳令A2"
#define LINE_CUST_DI16_FLT "G_跳令B2"
#define LINE_CUST_DI17_FLT "G_跳令C2"
#define LINE_CUST_DI18_FLT "G_跳令Q1"
#define LINE_CUST_DI19_FLT "G_跳令Q2"
#define LINE_CUST_DI20_FLT "G_跳令Q3"
#define LINE_CUST_DI21_FLT "G_跳令Q4"
#define LINE_CUST_DI22_FLT "G_跳令Q5"
#define LINE_CUST_DI23_FLT "G_跳令Q6"
#define LINE_CUST_DI24_FLT "G_跳令Q7"
#define LINE_CUST_DI25_FLT "G_跳令Q8"
#define LINE_CUST_DI26_FLT "G_跳令Q9"
#define LINE_CUST_DI27_FLT "G_跳令Q10"
#define LINE_CUST_DI28_FLT "G_闭锁重合闸1"
#define LINE_CUST_DI29_FLT "G_闭锁重合闸2"
#define LINE_CUST_DI30_FLT "G_闭锁重合闸3"
#define LINE_CUST_DI31_FLT "G_闭锁重合闸4"
#define LINE_CUST_DI32_FLT "G_闭锁重合闸5"
#define LINE_CUST_DI33_FLT "G_压力低禁止重合闸"
#define LINE_STORM_FILTER_ADD 15000

/* typedefs */

typedef enum		/* 模件枚举 */
{
    RD_RESERVED,
    RD_DSP_LGC_AI,
    RD_DSP_CALC_AI,
    RD_SPI_DI,
    RD_SPI_DO,
    RD_EXT_DI,
    RD_REDUN_DI,		/* 冗余机箱*/
    RD_REDUN_DO,
    RD_OPT1_DI,			/* 光纵 */
    RD_OPT2_DI,
    RD_OPT1_DO,
    RD_OPT2_DO,
    RD_SAME_POLE_DI,		/* 同杆并架 */
    RD_SAME_POLE_DO,
    RD_HDL_BOX_DI,		/* 智能操作箱 */
    RD_HDL_BOX_DO,
    RD_VT_BOX_DI,			/* 虚拟机箱，统一处理 */
    RD_VT_BOX_DO,
} RD_SRC_MOD;

typedef enum		/* 模拟量模件类型枚举 */
{
    RD_AI_AC = 2,		/* 交流 */
    RD_AI_DC,		/* 直流 */
} RD_AI_SRC_MOD;

typedef struct		/* 装置资源配置结构 */
{
    uint16_t unPartNum;
    BOOL bExpand;
    uint8_t ucEnvType;
    u_int uiBufPts; 			/* 总的点数 */
    u_int uiRealBuf;
    u_int uiSmplPeriod;
    uint32_t ulCurrAiCnt; 		/* 当前AI采样计数 */
    uint32_t ulBgnCnt;
    uint32_t ulSynCnt;
    uint32_t ulSynTime;
    uint32_t ulSynClk;
    BOOL b64KOptCh1;     /* 有64K光纵通道1 */
    BOOL b2MOptCh1;          /* 有2M光纵通道1 */
    BOOL b64KOptCh2;      /* 有64K光纵通道2 */
    BOOL b2MOptCh2;             /* 有2M光纵通道2 */

    BOOL bSamePole;		/* 有同杆并架机箱 */
    BOOL bHdlBox;  				/* 有智能操作箱 */
    BOOL bAssmDev_9_1;		/* 61850-9-1合并器 */
    BOOL bAssmDev_xn;			/* 新宁集中器 */

    BOOL bVirtBox;		/* 虚拟机箱，只说明有这一类型 */

    uint32_t ulDspBoxCount;		/* 数据写入缓冲计数 */
    uint32_t ulDspBoxFstTime;
    uint32_t ulDspBoxClkFst;
    uint32_t ulExtBoxCount;
    uint32_t ulExtBoxFstTime;
    uint32_t ulExtBoxClkFst;

    uint32_t ulDspBoxNextCnt;
    uint32_t ulExtBoxNextCnt;
} RD_SYS_INFO;

typedef struct		/* 虚拟逻辑通道 */
{
    uint8_t aucId[MAX_ID_LEN+1];
    uint16_t unLgcSN;
    uint8_t ucUnit;
    BOOL bRec;
    uint8_t aucRecId[MAX_ID_LEN+1];
    BOOL bFlag;
    uint8_t aucFlagId[MAX_ID_LEN+1];
    BOOL bMea;
    uint8_t aucMeaId[MAX_ID_LEN+1];
} RD_VT_AI_CH;

enum DI_Refresh_Rate 		/* DI刷新速度 */
{
    DI_FAST_REFRESH_RATE=0,    	/* 快速刷新 */
    DI_MID_REFRESH_RATE=1,     					/* 中速刷新 */
    DI_SLOW_REFRESH_RATE=2,   /* 慢速刷新 */
};

typedef struct		/* 开入配置通道 */
{
    uint8_t aucId[MAX_ID_LEN+1];
    uint16_t unLgcSN;
    uint8_t aucName[MAX_ID_LEN+1];
    uint8_t aucABRV[4];
    BOOL bRec;
    uint8_t aucRecId[MAX_ID_LEN+1];
    BOOL bFlag;
    uint8_t aucFlagId[MAX_ID_LEN+1];
    BOOL bMea;
    uint8_t aucMeaId[MAX_ID_LEN+1];
    BOOL *pbDI;                     /* -1=no measurent channel related. */

    uint8_t ucDIRefreshRate;         /* 刷新速度,0为快速,1为中速,2为慢速,其他保留 */
    int iForceSts;                      /* -1=not force, TRUE/FALSE=force. */
    RD_SRC_MOD mod;
    void *pvSrc;
    uint32_t ulChgTime;
    US_CNT_UTC_TIME utChgTime;
    uint32_t ulChgPackTime;
    US_CNT_UTC_TIME utChgPackTime;
    uint32_t ulChgNextCnt;  /* 变位时采样节拍 */
    uint32_t ulChgTimeAfterFilt;
    uint8_t ucModCh;
    RD_PART_INFO *p_part;
    int iVal; 			    /* Low 15 bit is now status(TRUE or FALSE), bit15(0x8000) is flag of force. */
    int iMeaCh;
    uint32_t ulFiltTime;
    uint32_t ulFiltTimeLine;
    uint8_t ReserveAttribute;				/* 是否保留给平台使用标志，0: 保护使用; 1: 平台使用 */
    uint8_t DebounceTimeSetMod;					/* 去抖动时间设置方式标志 */
    uint8_t DebounceTimeDingzhiTag[MAX_ID_LEN+1];
    BOOL bDIInvalidDftVal;/* DI无效时的默认值，当前只对智能操作箱起作用 */
    BOOL bSOE;					/* 1: 形成 0: 不形成 */
    uint8_t ucMmiShow;				/* 是否在人机界面上显示该通道值*/
    uint8_t ucVtBoxPos;		/* 虚拟机箱位置编号，从0开始 */
    uint32_t servertype;	/* 服务类型 */
    uint32_t ChOffset;   /* 存储地址偏移 */
    uint16_t usQuality;  /* 品质因素 */
    BOOL bPended;  /* 悬空标识, 包括配置为-1和没有配置的情况, 目前同样处理 */
    void *pVtPortCfg[HDL_DI_MAX_RECV_NUM];  /* 虚端子索引配置 */
    uint8_t ucVtPortNum;  /* 关联虚端子个数 */

    /* 历史状态和品质 */
    int iLstVal;
    uint16_t usLstQuality;
} RD_LGC_DI_CH;

typedef struct			/* 开出通道 */
{
    uint8_t aucId[MAX_ID_LEN+1];
    uint16_t unLgcSN;
    uint8_t aucName[MAX_ID_LEN+1];
    uint8_t aucABRV[4];
    RD_PART_INFO *p_part;
    RD_SRC_MOD mod;
    uint8_t ucModCh;
    BOOL bValid;
    BOOL bRec;
    uint8_t aucRecId[MAX_ID_LEN+1];
    BOOL bFlag;
    uint8_t aucFlagId[MAX_ID_LEN+1];
    BOOL bMea;
    uint8_t aucMeaId[MAX_ID_LEN+1];
    BOOL bCtrl;
    uint8_t ucCtrlAttr;

    void *pvSrc;
    int iForceSts;                 /* -1=not force, TRUE/FALSE=force. */
    int iSetVal;                    /* 根据DO跳闸计数,设置是否跳闸 2011-8-18 ZY  */
    int iVal;
    int iTripDOCnt;                  /* DO跳闸计数 */
    uint8_t ReserveAttribute;
    uint8_t ucMmiShow;				/* 是否在人机界面上显示该通道值*/
    uint8_t ucVtBoxPos;		/* 虚拟机箱位置编号，从0开始 */
    uint32_t servertype;	/* 服务类型 */
    int16_t linkNum; /* 对应的压板序号 */
} RD_LGC_DO_CH;

typedef struct		/* 指示灯配置 */
{
    uint8_t aucId[MAX_ID_LEN+1];
    uint16_t unCh;
    uint8_t aucName[MAX_ID_LEN+1];
    BOOL bIsHwLED;
    uint8_t ucColor;
    uint8_t ucBlink;
    BOOL bKeep;
    BOOL bSetVal;                    /* 根据DO跳闸计数,设置是否跳闸 2011-8-18 ZY  */
    BOOL bSts;
    int iTripLedCnt;           /* 指示灯点亮计数 */
    uint8_t ReserveAttribute;	  /* 是否保留给平台使用标志 */
} RD_LGC_LED_CH;

typedef struct			/* 读取硬件通道结构 */
{
    float fRmsVal;
    float fAngle;
    float fMean;
    float fGain;
    uint8_t ucType;
} RD_HW_AI_MEA;

typedef struct			/* 读取PO通道结构 */
{
    float fVal;
    uint8_t ucType;
} RD_PO_MEA;

typedef struct			/* 读取测量量通道结构 */
{
    float fVal;
    uint8_t ucUnit;
    uint8_t ucAttr;     						/* 测量量的属性 */
    uint16_t usQuality; /* 品质 */
} RD_AI_MEA;

typedef struct			/* 测量通道 */
{
    uint8_t aucId[MAX_ID_LEN+1]; 		/* MAX_ID_LEN=138 */
    uint16_t unLgcSN;
    RD_HW_AI_CH *phwai; 		/* 硬件通道配置 */
    uint8_t ucFiltTp; 			/* 滤波算法号 */
    uint8_t ucUnit;
    BOOL bFlag;
    uint8_t aucFlagId[MAX_ID_LEN+1];
    COMPLEX *pxMsucAI;
} RD_MSU_AI_CH;

typedef struct			/* 硬件配置项目 */
{
    uint8_t ucType;
    uint32_t ulLen;
    uint8_t *pucDat;
    uint32_t ulBufLen;
} RD_HW_CFG_ITEM;

typedef struct		/* 装置脉冲输入量 */
{
    uint8_t aucId[MAX_ID_LEN+1];		/* 逻辑标志字符串 */
    uint8_t aucName[MAX_ID_LEN+1];			/* 内部名称字符串 */
    uint8_t aucABRV[4];		/* 内部简称 */
    uint16_t unLgcSN;
    uint8_t ucUnit;		/* 单位类型 */
    BOOL bRec;
    uint8_t aucRecId[MAX_ID_LEN+1];
    BOOL bFlag;
    uint8_t aucFlagId[MAX_ID_LEN+1];
    BOOL bMea;
    uint8_t aucMeaId[MAX_ID_LEN+1];
    uint8_t RsvStr1[MAX_ID_LEN+1];
    uint8_t RsvStr2[MAX_ID_LEN+1];
    uint8_t RsvStr3[MAX_ID_LEN+1];
    RD_PART_INFO *p_part;
    RD_SRC_MOD mod;
    BOOL bValid;
} RD_LGC_PI_CH;

typedef struct		/* 装置脉冲输出量 */
{
    uint8_t aucId[MAX_ID_LEN+1];		/* 逻辑标志字符串 */
    uint8_t aucName[MAX_ID_LEN+1];			/* 内部名称字符串 */
    uint8_t aucABRV[4];		/* 内部简称 */
    uint16_t unLgcSN;
    uint8_t ucUnit;		/* 单位类型 */
    uint8_t ucType;		/* 类型: 0为脉冲量, 1为电度量, 2为统计量 */
    uint8_t RsvStr1[MAX_ID_LEN+1];
    uint8_t RsvStr2[MAX_ID_LEN+1];
    uint8_t RsvStr3[MAX_ID_LEN+1];
    RD_PART_INFO *p_part;
    RD_SRC_MOD mod;
    BOOL bValid;
    float CurrentEnergy;		/* 当前写进电度 */

    union
    {
        /* 结果联合 */
        float TotalEnergy;							/* 写入文件电度 */
        uint32_t ulVal;		/* 定点 */
    } Val;

    union
    {
        /* 结果联合 */
        float TotalEnergy;							/* 写入文件电度 */
        uint32_t ulVal;		/* 定点 */
    } OriginVal;
} RD_LGC_PO_CH;

typedef struct			/* AO输出*/
{
    uint8_t aucId[MAX_ID_LEN+1];				/* AO逻辑标识 */
    uint8_t aucName[MAX_ID_LEN+1];				/* AO内部名称 */
    uint8_t aucABRV[4];  				/* 内部简称 */
    float fMaxVal;
    float fMinVal;
    RD_AI_MOD *paimod;		/* 所属机箱号 */
    RD_PART_INFO *p_part;		/* 所属模件 */
    uint8_t ucModCh;						/* 在所属摸件中的通道号 */
    int  iAOSrcType; 					/* AO的数据来源类型 */
    void *pvSrc;					/* 若是光纵AI来源，该光纵AO所对应的硬件AI物理通道，若是光纵中间结果来源，该AO对应的数据源指针,
    											可以使用同杆并架来源 */
    float fOriCoff;		/* 原始配置 */
    float fSetCoff;			/* 设定系数，源自硬件配置或软件配置 */
    float fCoff;
    uint8_t ucUnit;
    uint8_t FactorSetModWord;				/* 配置参数设置方式字 */
    uint8_t MaxValueDingzhiTagBaseLen;					/* 逻辑标志字符串长度 */
    uint8_t MaxValueDingzhiTagBase[MAX_ID_LEN+1];					/* 逻辑标志字符串 */
    uint8_t MinValueDingzhiTagBase[MAX_ID_LEN+1];
    uint8_t ScalefactorDingzhiTagBase[MAX_ID_LEN+1];
    uint8_t ucOptAOOutputIntv;		/* 光纵发送间隔 */
    uint8_t ucAttr;		/* 属性字，包含刷新速度 */
    uint8_t aucSrcId[MAX_ID_LEN+1];				/* AO数据源 */
    uint8_t ucSrcType;				/* 数据源类型, real or image. */
    uint8_t ucVtBoxPos;		/* 虚拟机箱位置编号，从0开始 */
    uint32_t servertype;	/* 服务类型 */
} RD_HW_AO_CH;     			/*AO配置信息，2006-2-9  */

typedef struct			/* 硬件通道系数 */
{
    float fMaxVal;			/* 最大 */
    float fMinVal;	/* 最小 */
    float fCoff;			/* 增益 */
    float fExcCoff;		/* 零点偏移值 */
} RD_AIO_HW_COFF;

typedef struct				/* DA输出的AO通道配置 */
{
    uint8_t ucAOHdCh;      	/* DA输出的AO物理通道号，从0开始 */
    float fCoff;                        		/* 系数 */
    int iAOSrcType;                /*1为AI来源,2为逻辑图中间结果来源,其他保留  */
    uint8_t ucSrcAIHdCh;                      /* 若是AI来源,DA输出的AO的本地机箱源AI对应的物理通道号,  从0开始*/
    void *pElemSrc;                    /* 若是中间结果来源,逻辑图中间结果IO指针*/
} DA_AO_CFG;

typedef struct			/* DA输出的AO配置 */
{
    int  iDaAONum;    			/* DA输出的总的AO总数 */
    int  iDaAISrcAONum;    						/* AI来源的AO总数 */
    int  iDaMidSrcAONum;    		/* 逻辑图中间结果的AO总数 */
    DA_AO_CFG aDaAoCfg_g[MAX_DA_AO_NUM];
} DA_PART_AO_CFG;

/* 交流模件类型切换 */
typedef struct tag_ACMOULDTYPE
{
    int32_t iMaxTypeNum;					/* 最多类型种类 */
    int32_t iCurrentType;													/* 当前使用类型 */
    BOOL bChgFlag;											/* 更新标志 */
    BOOL bMmiUpdateFlag;		/* If MMI need to update the configuration. */
    BOOL bValid;  /* 索引定值页序是否有效 */
} ACMOULDTYPE;

typedef struct tag_ENVIROMENTTYPE
{
    BOOL bFreq50Sys;               /* 适合系统50Hz */
    BOOL bFreq60Sys;      /* 适合系统频率60Hz */
    BOOL bOneAmpSys;               /* 适合1A额定电流系统 */
    BOOL bFiveAmpSys;      /* 适合5A额定电流系统 */
} ENVIROMENTTYPE;

/* globals */

extern int iHwAoChNum_g;
extern RD_HW_AO_CH *phwaoch_g;/*AO配置 */
extern DA_PART_AO_CFG PartAoCfgDa_g;  /* DA输出的AO配置 */
extern int iLgcPiChNum_g;
extern RD_LGC_PI_CH *plgcpich_g;
extern int iLgcPoChNum_g;
extern RD_LGC_PO_CH *plgcpoch_g;
extern int iHwAiChNum_g;
extern int iLineNum_g;  /* 单元数 */
extern RD_HW_AI_CH *phwaich_g;
extern RD_AI_MOD aimodExt_g;
extern int iLgcAiChNum_g;
extern RD_LGC_AI_CH *plgcaich_g;
extern int iVtAiChNum_g;
extern RD_VT_AI_CH *pvtaich_g;
extern int iLgcDoChNum_g;
extern RD_LGC_DO_CH *plgcdoch_g;
extern RD_LGC_LED_CH *plgcledch_g;
extern int iLgcLedChNum_g;
extern int iHwLedChNum_g;
extern int iSwLedChNum_g;
extern int iMsuAiChNum_g;
extern RD_MSU_AI_CH *pmsuaich_g;
extern uint16_t AIBaseCh_g;
extern RD_SYS_INFO rdinfo_g;
extern RD_AI_MOD aimodDsp_g;
extern RD_AI_MOD aimodRedun_g; 				/* 冗余机箱 */
extern RD_AI_MOD aimodPole_g;    /*为同杆并架添加  2007-3-20  */
extern RD_AI_MOD aimodHdl_g;			/* 智能操作箱 */
extern RD_AI_MOD aimodOpt_g[2];  /*为光纵添加 2006-2-8 */
extern RD_AI_MOD aimodVtBox_g[MAX_VT_BOX_COUNT];			/* 虚拟机箱 */
extern RD_PART_INFO apartinf_g[MAX_PART_NUM];
extern int iLgcDiChNum_g;
extern RD_LGC_DI_CH *plgcdich_g;
extern BOOL bStopRefreshData;
extern SEM_ID semI2CWrEnableFlag;
extern int FestI2CErrorMaxNum;
extern ACMOULDTYPE AdMdType;
extern ENVIROMENTTYPE EnviromentType;
extern uint16_t uBaseUnitFstRatedVal_g;							/* 一次额定值 */
extern uint16_t uBaseUnitSecRatedVal_g;			/* 二次额定值 */

/* globals declarations */

/***********************************************************************
* RD_Initialize - 初始化整个实时数据模块
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
EP_STATUS RD_Initialize(
    const uint8_t *strHwCfgFile		/* 硬件配置文件名称 */
);

/***********************************************************************
* RD_Cfg_Lgc_PI - 装置脉冲输入量配置解析
*
* RETURNS: 无
*
*/
EP_STATUS RD_Cfg_Lgc_PI(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
);

/***********************************************************************
* RD_Cfg_Lgc_PI - 装置脉冲输出量配置解析
*
* RETURNS: 无
*
*/
EP_STATUS RD_Cfg_Lgc_PO(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
);

/***********************************************************************
* RD_Cfg_AO - AO输出量配置解析
*
* RETURNS: 无
*
*/
EP_STATUS RD_Cfg_AO(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
);

/***********************************************************************
* DA_InitAOCfg - 初始化DA输出的AO所有配置
*
* RETURNS: 成功与否
*
*/
EP_STATUS DA_InitAOCfg(
    int iAOCfgNum		/* 所有AO配置个数 */
);

/***********************************************************************
* RD_Cfg_Lgc_PI - 装置脉冲输入量配置解析
*
* RETURNS: 无
*
*/
EP_STATUS RD_Cfg_Lgc_PI(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
);

/***********************************************************************
* RD_Cfg_Lgc_PI - 装置脉冲输出量配置解析
*
* RETURNS: 无
*
*/
EP_STATUS RD_Cfg_Lgc_PO(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
);

/***********************************************************************
* RD_Get_PI_Attr - Get PI channel attribution.
*
* RETURNS:
*		Pointer to the PI attribution structure.
*		NULL if iIdx is invalid(>=iLgcPiNum_g).
*
*/
const RD_LGC_PI_CH *RD_Get_PI_Attr(
    int iIdx		/* index of the PI(from 0) */
);

/***********************************************************************
* RD_Get_PO_Attr - Get PO channel attribution.
*
* RETURNS:
*		Pointer to the PO attribution structure.
*		NULL if iIdx is invalid(>=iLgcPiNum_g).
*
*/
const RD_LGC_PO_CH *RD_Get_PO_Attr(
    int iIdx		/* index of the PO(from 0) */
);

/***********************************************************************
* GetPiCfgNum - 获取PI配置数
*
* RETURNS: PI配置数
*
*/
int GetPiCfgNum(void);

/***********************************************************************
* GetPoCfgNum - 获取PO配置数
*
* RETURNS: PO配置数
*
*/
int GetPoCfgNum(void);

/***********************************************************************
* GetAiSeqNum - 由AI逻辑标志字符串获得其配置序号
*
* RETURNS: AI配置序号
*
*/
int GetAiSeqNum(
    char * AiName			/* 逻辑标志字符串 */
);

/***********************************************************************
* ModifyDiFiltTime - 修改DI配置通道消抖时间
*
* RETURNS: 无
*
*/
void ModifyDiFiltTime(
    void *pDiCfg,				/* DI逻辑通道配置 */
    uint32_t ulFiltTime
);

/***********************************************************************
* ModifyAiExcCoe - 修改Ai逻辑配置对应物理通道的偏移系数
*
* RETURNS: 无
*
*/
void ModifyAiExcCoe(
    int SeqNum,			/* 逻辑标志字符串 */
    float fExcCoe
);

/***********************************************************************
* GetAiScaleCoe - 获得Ai逻辑配置对应物理通道的增益系数
*
* RETURNS: 无
*
*/
float GetAiScaleCoe(
    int SeqNum			/* 序号 */
);

/***********************************************************************
* ModifyAiScaleCoe - 修改Ai逻辑配置对应物理通道的增益系数
*
* RETURNS: 无
*
*/
void ModifyAiScaleCoe(
    int SeqNum,			/* 逻辑标志字符串 */
    float ScaleCoe
);

/***********************************************************************
* GetAiExcCoe - 获得Ai逻辑配置对应物理通道的偏移系数
*
* RETURNS: 偏移系数
*
*/
float GetAiExcCoe(
    int SeqNum			/* 序号 */
);

/***********************************************************************
* RD_Chg_Coff - 通过定值修改配置系数
*
* RETURNS: 无
*
*/
EP_STATUS RD_Chg_Coff(void);

/***********************************************************************
* RD_Wr_PO - 脉冲输出量输出
*
* RETURNS: 无
*
*/
void RD_Wr_PO(
    void *pvPoHnd,				/* 用来索引PO对象的void指针，应该通过调用本模块提供的RD_Get_Handle得到 */
    uint32_t ulVal				/* 写入值 */
);

/***********************************************************************
* RD_Rd_PO - 获取脉冲输出量
*
* RETURNS: 无
*
*/
void RD_Rd_PO(
    void *pvPoHnd,				/* 用来索引PO对象的void指针，应该通过调用本模块提供的RD_Get_Handle得到 */
    uint32_t *pulVal				/* 写入值地址 */
);

/***********************************************************************
* Reset_PO_Val - 脉冲输出量清零
*
* RETURNS: 无
*
*/
void Reset_PO_Val();

/***********************************************************************
* PoWrInit - 脉冲电量保存初始化
*
* RETURNS: 无
*
*/
EP_STATUS PoWrInit(void);

/***********************************************************************
* PoWrFile - 脉冲电量保存入文件
*
* RETURNS: 无
*
*/
EP_STATUS PoWrFile(void);

/***********************************************************************
* PoRdFile - 从文件读脉冲电量
*
* RETURNS: 无
*
*/
EP_STATUS PoRdFile(void);

/***********************************************************************
* FT_Temp_Name_New - 临时函数(防止修改filetool.c)
*
* RETURNS: 无
*
*/
void FT_Temp_Name_New(
    uint8_t *pucRslt,
    const uint8_t *strFile
);

/***********************************************************************
* ModifyAiScaleCoeLgc - 修改Ai逻辑配置对应物理通道的增益系数
*
* RETURNS: 无
*
*/
void ModifyAiScaleCoeLgc(
    void *pAiCfg,				/* AI逻辑通道配置 */
    float ScaleCoe
);

/***********************************************************************
* GetAiScaleCoeLgc - 获得Ai逻辑配置对应物理通道的增益系数
*
* RETURNS: 增益系数
*
*/
float GetAiScaleCoeLgc(
    void *pAiCfg				/* AI逻辑通道配置 */
);

/***********************************************************************
* ModifyAiExcCoeLgc - 修改Ai逻辑配置对应物理通道的偏置系数
*
* RETURNS: 无
*
*/
void ModifyAiExcCoeLgc(
    void *pAiCfg,				/* AI逻辑通道配置 */
    float fExcCoe
);

/***********************************************************************
* GetAiExcCoeLgc - 获得Ai逻辑配置对应物理通道的偏置系数
*
* RETURNS: 偏置系数
*
*/
float GetAiExcCoeLgc(
    void *pAiCfg				/* AI逻辑通道配置 */
);

/***********************************************************************
* GetAiCoffLgc - 获得Ai逻辑配置对应物理通道的比例系数
*
* RETURNS: 比例系数
*
*/
float GetAiCoffLgc(
    void *pAiCfg				/* AI逻辑通道配置 */
);

/***********************************************************************
* RD_Is_Valid_Gain - 读取增益有效性文件
*
* RETURNS: 无
*
*/
BOOL RD_Is_Valid_Gain(int iFd);

/***********************************************************************
* WR_Cfg_Hw_AI_Gain - 保存物理通道增益系数
*
* RETURNS: 无
*
*/
EP_STATUS WR_Cfg_Hw_AI_Gain(int nType);

/***********************************************************************
* Reset_Cfg_Hw_AI_Gain - 保存物理通道增益系数，MMI调用
*
* RETURNS: 无
*
*/
EP_STATUS Reset_Cfg_Hw_AI_Gain(void);

/***********************************************************************
* InitDb - 初始化缓冲区
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*/
EP_STATUS InitDb(void);

/***********************************************************************
* RD_Ck_Coff - 系数检查
*
* RETURNS:
*			EP_SUCCESS, 正常
*			EP_BUF_ERR, 错误
*/
EP_STATUS RD_Ck_Coff(void);

/***********************************************************************
* RecBufInit - 小电流接地缓冲区初始化
*
* RETURNS:
*			EP_SUCCESS, 正常
*			EP_BUF_ERR, 错误
*/
EP_STATUS RecBufInit(void);

/***********************************************************************
* TestNestStep - 下一步测试
*
* RETURNS: 无
*
*/
void TestNestStep(void);

/***********************************************************************
* FloatValidCheck - 浮点有效检查
*
* RETURNS: 无
*
*/
EP_STATUS FloatValidCheck(
    float fVal
);

/***********************************************************************
* write_ram_data_cycle - 多次写铁电，保证正确
*
* RETURNS: ERROR, or OK
*
*/
int write_ram_data_cycle(
    unsigned short addr, 			/* address of RAM to read data */
    unsigned char *pBuf, 	/* Pointer to buffer */
    unsigned short length			/* length of data to read */
);

/***********************************************************************
* ResetFRAMVal - 铁电存储器清零
*
* RETURNS: 无
*
*/
EP_STATUS ResetFRAMVal();

/***********************************************************************
* RD_AddMidSrcAo - 设置逻辑图中间结果信号源的AO  ,在逻辑图中调用
*
* RETURNS: EP_STATUS
*
*/
EP_STATUS  RD_AddMidSrcAo(
    uint8_t *strOptAoId,				/* AO的ID号 */
    uint8_t  ucUnit,				/* 信号单位类型 */
    void *pvElemSrc		/* 逻辑图中间结果来源指针 */
);

/* Set AO from logic graph in virtual box, call by logic graph.
 * Para:
 *     strOptAoId, ID of AO.
 *     ucUnit, unit.
 *     pvElemSrc, source pointer from logic graph middle variable
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS RD_VirtBoxAddMidSrcAo(uint8_t *strOptAoId, uint8_t ucUnit, void *pvElemSrc);

/***********************************************************************
* FloatValidCheckTest - 浮点有效检查测试
*
* RETURNS: 无
*
*/
EP_STATUS DA_Init_Mid_Src_AO(
    int iSrcType, 		/* 通道类型 */
    u_int uiCh, 		/* 在模件内的通道号 */
    void *pElemIOSrc, 			/* 逻辑图中间结果输出指针 */
    float fCoff		/* 模件通道系数 */
);

/***********************************************************************
* GetAiCntPeriod - 获取采样周期，单位为us
*
* RETURNS: 无
*
*/
float GetAiCntPeriod(void);

/***********************************************************************
* EP_GetAcMdType - Get the AC mould type
*
* RETURNS: Mould type sequence
*
*
*/
int32_t EP_GetAcMdType(void);

EP_STATUS EP_SetAcMdType(
    int32_t uAcMdType			/* Mould type used */
);

/***********************************************************************
* EP_GetAcMdType -获取交流模件类型
*
* RETURNS: 模件类型，根据索引定值所配确定，如第0页: 1A；第1页: 5A
*
*/
int32_t App_GetAcMdType(void);

/***********************************************************************
* GetAICoff - 获取AI通道相关系数，MMI调用
*
* RETURNS: NONE
*
*/
void GetAICoff(
    RD_AIO_HW_COFF *pAiCoff		/* 分配iHwAiChNum_g个 */
);

/***********************************************************************
* GetAOCoff - 获取AO通道相关系数，MMI调用
*
* RETURNS: NONE
*
*/
void GetAOCoff(
    RD_AIO_HW_COFF *pAoCoff		/* 分配iHwAoChNum_g个 */
);

/***********************************************************************
* EP_GetAcMdTypeChgFlag - judge if the AC mould type changed
*
* RETURNS: TRUE, or FALSE
*
*
*/
BOOL EP_GetAcMdTypeChgFlag(void);

/***********************************************************************
* EP_SetAcMdTypeChgFlag - Set the AC mould type changed flag
*
* RETURNS:
*               EP_SUCCESS: Normal
*               EP_ERROR: Error
*
*
*/
EP_STATUS EP_SetAcMdTypeChgFlag(
    int32_t uAcMdTypeChgFlag			/* Mould type changed flag */
);

/***********************************************************************
* UpdateAcCoff - 更新交流通道系数
*
* RETURNS: 无
*
*/
extern void UpdateAcCoff(void);

/* get the information of module.
 * Para:
 *     pPara, result.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
extern EP_STATUS RD_GetModInfo(uint8_t **pPara);

/* get the information of channel.
 * Para:
 *     pname, name of module.
 *     pPara, result.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
extern EP_STATUS RD_GetChnInfo(uint8_t *pname, uint8_t **pPara);

/* adjust the mod.
 * Para:
 *     pmodname, module name.
 *     pchnname, channel name.
 * Return:
 *     result.
 */
int32_t RD_AdjMod(uint8_t *pmodname, uint8_t *pchnname, uint8_t uccmdtype);

US_CNT_UTC_TIME RD_GetUTDiChgTimeByDiIndex(int index);

int AppGetDiState(int num);

/* 生成新的CT变比文件
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void RD_New_CT_Ratio(void);

/* 读取索引定值页序.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern EP_STATUS Hw_GetAcMdType(void);

/* 初始化GOOSE功能
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void RD_InitGoose(void);

/* 更新单通道消抖时间.
 * Para:
 *     pSrc, 开入通道句柄, 来源于输入.
 *     ulFltTm, 消抖时间, us.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL SetDiFltTime(void *pSrc, uint32_t ulFltTm);

/* 生成新的CT变比系数文件
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern int RD_Get_CT_Ratio_File_Len(void);

#ifdef  __cplusplus
}
#endif

#endif
