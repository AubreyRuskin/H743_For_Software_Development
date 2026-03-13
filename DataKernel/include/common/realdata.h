/* realdata.h - This file contains programs to manager realtime data */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02a, 11nov06, dy change the style of program and separate to three file: realdata.h, hwcfg.h, dspai.h.
01c, 26jul03, hdx Updated to version 1.0.
01b, 27feb03, hdx Updated to version 0.2.
01a, 26jul02, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains programs to manager realtime data.
*/

#ifndef REALDATA_H
#define REALDATA_H

#ifdef  __cplusplus
extern "C" {
#endif

/* includes */

#include "edpbase.h"
#include "math_compat.h"
#include "logmsg.h"
/* #include "dsp.h" */		/* 防止逻辑图引用 */
#include "dspai.h"
#include "hwcfg.h"
/* #include "realdatalogicinterface.h" */

#if defined(EDP_01_02_BUILD)   /*2011-11-16  ZY,以前逻辑有漏洞  */
#if defined(EDP02_GTP_BUILD)
#define RD_BUF_CYC 120     /* 缓冲的数据周波数 */
#else
#define RD_BUF_CYC 40     /* 缓冲的数据周波数 */
#endif
#else
#define RD_BUF_CYC 20     /* 缓冲的数据周波数 */
#endif

#define RD_BUF_CYC_LONG 600     /* 长录波, 600个周波 */
#define RD_BUF_CYC_SHORT 10     /* 短录波, 10个周波 */

#define RD_SAM_SYN_CLK    (10*uiAiPts_g)           /* 采样同步节拍数，中压是256，高压是480 2006-2-12*/

#define RD_SIO_RATE 2000     /* SPI-IO刷新周期，次/秒 ,可用于测控 */

#ifndef M_PI
#define M_PI 3.141592653589793238462643
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.4142135623730950488016887
#endif

#define DIADDRONCPU (MAX_MOD_NUM-1)		/* CPU模件上的开入模件地址 */
#define DIADDRONCPUNUM 10				/* 个数 */
#define DOADDRONCPUNUM 10				/* 个数 */
#define DIADDROMCPUMASK 0x000003FF		/* 截取10位 */

#define DP_TRUE  0x22
#define DP_FALSE 0x11
#define DP_INVALID_11 0x33
#define DP_INVALID_00 0x55

#define AI_CC_CONFREV_CHECK_ERR 0x4000 /*配置版本不一致错误*/
#define AI_CC_APPID_CHECK_ERR 0x20

#define AI_COM_ERR	0x01 /* 通信中断 */
#define AI_DAT_VLD 	0x02
#define AI_DAT_SYN	0x04
#define AI_TEST_DAT	0x08

/* 与端口关联标志 */
#define AI_PHY_ERR 0x10  /* PHY接收错误 0：正常 1：异常 */
#define AI_ASDUNUM_ERR 0x20 /*ASDU数目错误 0：正常 1：异常*/
#define AI_CHANNUM_ERR 0x40 /* 采样通道数目错误 0：正常 1：异常 */
#define AI_CRC_ERR 0x80 /* 报文非整字节错误或报文长度错误或报文CRC校验错误 0：正常 1：异常 */
#define AI_FORMAT_ERR 0x100  /* SV报文格式错误 0：正常 1：异常 */
#define AI_APPID_CHECK_ERR 0x200 /* APPID或confRev校验错误 0：正常 1：异常 */
#define AI_SVID_CHECK_ERR 0x400 /* svID校验错误 0：正常 1：异常 */
#define AI_P2P_INTERVAL_ERR 0x800  /* 点对点, 报文间隔时间超出范围(点对点250us±30us) 0：正常 1：异常 */
#define AI_NO_FRAME_ERR 0x1000  /* 未收到有效报文(点对点方式500us内没有收到有效报文，组网方式2ms内没有收到有效报文) 0：正常 1：异常 */
#define AI_P2P_DELAY_ERR 0x2000  /* 点对点, 报文额定延时变动(仅用于点对点方式) 0：正常 1：异常 */
#define AI_INSERT_ERR 0x4000  /* 点对点, 时间插值异常(查不到正确的插值区间或插值区间的前后插值点数据无效) 0：正常 1：异常 */
#define AI_SLAVE_CFG_ERR 0x8000  /* 点对点(前级), 子机程序与配置不匹配 0：正常 1：异常 */
#define AI_NET_SYN_ERR 0x10000  /* 组网, 组网方式下计数器同步异常(查不到对应计数器的报文缓冲区或报文缓冲区数据无效) 0：正常 1：异常 */

/* B网状态标 */
#define AI_NET_B_PHY_ERR 0x20000  /* B网PHY接收错误 0：正常 1：异常 */
#define AI_NET_B_BYTE_ERR 0x40000 /* B网报文非整字节错误 0：正常 1：异常 */
#define AI_NET_B_LEN_ERR 0x80000 /* B网报文长度错误 0：正常 1：异常 */
#define AI_NET_B_CRC_ERR 0x100000 /* B网报文CRC校验错误 0：正常 1：异常 */
#define AI_NET_B_FORMAT_ERR 0x200000  /* B网SV报文格式错误 0：正常 1：异常 */
#define AI_NET_B_APPID_CHECK_ERR 0x400000 /* B网APPID校验错误 0：正常 1：异常 */
#define AI_NET_B_SVID_CHECK_ERR 0x800000 /* B网svID校验错误 0：正常 1：异常 */

#define AI_LINK_STS 0x1000000  /* 通道关联压板状态 1: 退出 0: 投入 */
#define AI_NET_B_NO_FRAME_ERR 0x2000000  /* 未收到有效报文(点对点方式500us内没有收到有效报文，组网方式2ms内没有收到有效报文) 0：正常 1：异常 */

#define AI_SYS_SYN_FLAG 0x4000000	/* 秒脉冲同步异常 */
#define AI_SMPCNT_ZERO_FLAG 0x8000000	/* 采样0点标志 */

#define AI_NET_SYN_MODE_FLAG 0x10000000  /* 组网光差同步模式 0: 计数器同步; 1: 外同步 */
#define AI_NET_B_SYN_ERR 0x20000000  /* 组网, 组网方式下计数器同步异常(查不到对应计数器的报文缓冲区或报文缓冲区数据无效) 0：正常 1：异常 */
#define AI_NET_SYN_FLAG 0x40000000  /* 组网光差外同步标识 0: 同步; 1: 未同步 */
#define AI_SLAVE_FLAG 0x80000000  /* 前级标志,或A/B网标识,0: 本级,A网 1: 前级,B网 */

/* E02-CPU.F-A采样错误标 */
#define AI_AD1_ERR 0x10  /* AD1错误 */
#define AI_AD2_ERR 0x20      /* AD2错误 */
#define AI_AD3_ERR 0x40   /* AD3错误 */
#define AI_AD4_ERR 0x80   /* AD4错误 */
#define AI_BUF_OVERFLOW 0x100   /* 溢出 */
#define AI_INT_RANGE_OUT 0x200  /* 帧间隔越界 */
#define AI_FPGA_CRC_ERR 0x400   /* CRC校验出错 */
#define AI_DECODE_ERR 0x800  /* 解码错误 */
#define AI_LEN_ERROR 0x1000  /* Frame Length Error */
#define AI_OCTET_ERROR 0x2000   /* Frame Non-Octet Error */
#define AI_PHY_ERROR 0x4000   /* PHY Receive Error */

/* 解析DCU报文状态标 */
#define AI_PHHealth_DAT	0x10
#define AI_DAT_SYN_MOD 0x20
#define AI_SENSOR_TYPE 0x40
#define AI_DAT_SCP_FLG 0x80

#define APP_TAG_LEN 64

/* 开关量品质 */

#define DI_LINK_STS 0x0001  /* 通道关联压板状态 1: 退出 0: 投入 */
#define DI_CFG_INVALID 0x0002  /* 配置错误 1: 错误 0: 正常 */
#define DI_SUSPEND_STS 0x0004 /* 悬空 1: 悬空 0: 连接 */
#define DI_MID_00_STS 0x0008 /* 中间态 1: 中间态 0: 非中间态 */
#define DI_INVALID_11_STS 0x0020 /* 无效态 1: 无效态 0: 非无效态 */
#define DI_COM_STS 0x0040 /* 接收异常 */
#define DI_REPAIR_DIF_STS 0x0080 /* 检修不一致 */
#define DI_REPAIR_STS 0x0010 /* 检修 */
#define DI_FORCE_STS 0x0100 /* 强制状态 */

/* 数据无效, 总状态, 包括: 中间态, 无效态, 检修不一致, 接收异常, 压板退出
 * 悬空, 配置错误
 */
#define DI_INVALID_STS 0x4000

#ifdef ITEM_LEN_81
#define DI_NORMAL "N               " /* 16字节 */
#define DI_CLOSE "1               " /* 16字节 */
#define DI_OPEN "0               " /* 16字节 */
#else
#define DI_NORMAL "N   " /* 4字节 */
#define DI_CLOSE "1   " /* 4字节 */
#define DI_OPEN "0   " /* 4字节 */
#endif

/* typedefs */

enum Handle_Type			/* 各种句柄，根据需要可进行调整 */
{
    RD_LGC_AI_HDL=0X0,
    RD_LGC_DI_HDL=0X1,
    RD_LGC_DO_HDL=0X2,
    RD_LGC_LED_HDL=0X3,
    RD_LGC_DI_MOD_HDL=0X4,
    RD_MSU_AI_HDL = 0X5,
    RD_LGC_AO_HDL= 0X06,	/* 增加模拟量输出 */
    RD_LGC_PI_HDL = 0X7,
    RD_LGC_PO_HDL = 0X8
};

enum   Box_Type			/* 各种机箱类型，以后可以适当调整， 为光纵修改，6/8/2006 */
{
    RD_LOCAL_BOX=0X0,
    RD_EXTEND_BOX=0X1,
    RD_64K_OPT1_BOX=0X2,
    RD_2M_OPT1_BOX=0X3,
    RD_64K_OPT2_BOX=0X4,
    RD_2M_OPT2_BOX=0X05,
    RD_OTHER_CPU_BOX=0X06,				/* 双CPU虚拟机箱 */
    RD_REDUN_BOX=0X08,
    RD_SAME_POLE_BOX=0X9, 	/* 同杆并架 */
    RD_HDL_BOX=0XA,    			/* 智能操作箱 */
    RD_9_1_ASSM_BOX=0XB,			/* 61850-9-1合并器 */
    RD_XN_ASSM_BOX=0XC,
    RD_VT_BOX=0XD,		/* 虚拟机箱 */
};

typedef enum		/* AO来源 */
{
    DA_VOID_SRC=0,			/* 没有来源 */
    DA_AI_SRC =1,    							/* DA的AI来源 */
    DA_PRE_SRC = 2,				/* from processing. */
    DA_MID_SRC = 3,		/* DA中间结果来源 */
} AO_SRC_TYPE;						/*AO的数据来源类型  */

enum DI_Refresh_Interval 		/* DI刷新间隔采样点数 */
{
    DI_FAST_REFRESH_INTERVAL=2,		/* 由3改为1, 满足实时刷新的要求 DY 6/11/2007 */
    DI_MID_REFRESH_INTERVAL=6,
    DI_SLOW_REFRESH_INTERVAL=12,
};

/* 应用匹配结构 */
typedef struct
{
    int iAppSN;		/* 对应的应用类型序列号,见App_Type类型中定义 */
    uint8_t aucAppID[APP_TAG_LEN+1];  /* 应用类型名称 */
} EP_APP_MAP;

/* defines */

#define AI_HND_TO_UNIT(pv)  (((RD_LGC_AI_CH*)(pv))->ucUnit)
#define MSU_HND_TO_UNIT(pv)  (((RD_MSU_AI_CH*)(pv))->ucUnit)
#define GET_AI_CT_RATIO(pv) (((RD_LGC_AI_CH*)(pv))->phwai->fRatio)
#define PO_HND_TO_UNIT(pv)  (((RD_LGC_PO_CH*)(pv))->ucUnit)

#define IS_REAL_AI(u)       ((u)==0x08 || (u)==0x0B || (u)==0x14 || \
                            (u)==0x17 || (u)==0x1A || (u)==0x1D || (u)==0x28 || (u)==0x10)
#define IS_RI_CPLX_AI(u)    ((u)==0x09 || (u)==0x0C || (u)==0x15 || \
                            (u)==0x18 || (u)==0x1B || (u)==0x1E || (u)==0x11 )
#define IS_MA_CPLX_AI(u)    ((u)==0x0A || (u)==0x0D || (u)==0x16 || \
                            (u)==0x19 || (u)==0x1C || (u)==0x1F || (u)==0x12 )
#define IS_CPLX_AI(u)       (IS_RI_CPLX_AI((u)) || IS_MA_CPLX_AI((u)))

#define IS_BOOL_SIG(u)      ((u)==0x04)
#define IS_SIGNED_INTEGER_SIG(u)       ((u)==0x60 || (u)==0x61)
#define IS_UNSIGNED_INTEGER_SIG(u)      ((u)==0x00 || (u)==0x40 || (u)==0x6B)
#define IS_INTEGER_SIG(u)   (IS_SIGNED_INTEGER_SIG(u) || IS_UNSIGNED_INTEGER_SIG(u))
#define IS_REAL_SIG(u)      (!(IS_CPLX_AI(u) || IS_BOOL_SIG(u) || IS_INTEGER_SIG(u)))

/* 是否电流通道 */
#define IS_CURRENT_SIG(u) ((u) == 0x08 || (u) == 0x0B || (u) == 0x10)

/* 得到复数的模（精确值）
 * 参数：	采用实/虚部表示的复数
 * 返回值：	复数的模 */
#define RI_CPLX_MOD(x)      sqrt(REAL(x)*REAL(x)+IMAGE(x)*IMAGE(x))

/* 得到复数的幅角（精确值）
 * 参数：	采用实/虚部表示的复数
 * 返回值：	角度表示的幅角，0-360 */
#define RI_CPLX_ANG(x)      (atan2(IMAGE(x), REAL(x))*(180.0/M_PI)+180.0)

/* 得到复数的实部（精确值）
 * 参数：	采用幅值/相角(度)表示的复数
 * 返回值：	实部 */
#define RI_CPLX_REAL(x,y)      (x*cos(2*M_PI*x/360.0)

/* 得到复数的虚部（精确值）
 * 参数：	采用幅值/相角(度)表示的复数
 * 返回值：	虚部 */
#define RI_CPLX_IMAGE(x,y)      (x*sin(2*M_PI*x/360.0)

/* locals */

/* globals */

extern u_int uiAiPts_g;                 /* 模拟量（AI）每周波采样点数 */
extern u_int uiPwrFreq_g;               /* 电力系统频率，50或者60 */
extern u_int uiAiRate_g;                /* 模拟量（AI）采样周期，次/秒 */
extern u_int uiDioRate_g;               /* 数字量（DI/DO）刷新周期，次/秒 */
extern BOOL bOneAmpSys_g;               /* 适合1A额定电流系统 */
extern BOOL bFiveAmpSys_g;              /* 适合5A额定电流系统 */
extern BOOL bDoubleCPUFlag_g;				/* 是否支持双CPU */
extern uint8_t ucCPUPos_g;				/* CPU插槽位置 */
extern uint8_t uiAppType_g;					/* 应用类型 */

extern int iHwAiChNum_g;                /* 配置的物理AI总数 */
extern int iLgcAiChNum_g;               /* 逻辑AI通道总数（包括虚拟AI） */
extern int iLgcDiChNum_g;               /* 配置的DI总数 */
extern int iLgcDoChNum_g;               /* 配置的DO总数 */
extern int iHwLedChNum_g;               /* 配置的面板LED总数 */
extern int iSwLedChNum_g;               /* 配置的显示屏LED总数 */

extern RD_SYS_INFO rdinfo_g;
extern RD_LGC_AI_DB lgcaidb_g; /* 逻辑通道计算结果存放缓冲区 */
extern RD_CALC_AI_DB calcaidb_g; /* 预处理通道计算结果存放缓冲区 */
extern RD_AI_MOD aimodDsp_g;

extern RD_MSU_AI_DB msucaidb_g;

extern RD_DI_DB didb_g;

extern RD_LGC_AI_CH *plgcaich_g;

extern int iLgcDiChNum_g;
extern RD_LGC_DI_CH *plgcdich_g;
extern RD_LGC_AI_STS_DB lgcaistsdb_g;

extern uint32_t ulDwordBitArr_g[32];

/* 应用标识匹配表 */
extern EP_APP_MAP aAppMapArr[];

extern float fSetValue_MUTDelay_Local;  /* 本侧获得的用于数字化 */
extern float fSetValue_MUTDelay_Ops;    /* 从通道获得的用于传统侧 */
extern uint32_t ulSetValue_PriRateCur;
extern uint32_t ulSetValue_PriRateVol;

extern uint32_t g_RdBufCyc;  /* 采样缓冲周波数 */

extern BOOL g_bAppAlarm;  /* 应用告警 */

/***********************************************************************
* RD_Get_Handle - 取得实时数据I/O通道的索引
*
* RETURNS: 用来索引实时数据I/O通道的void指针，或者NULL表示调用出错
*
*/
void *RD_Get_Handle(
    uint8_t *strLgcId,			/* 逻辑标识字符串（名称） */
    int  iHdlType			/* 句柄类型 */
);

/* 注册采样节拍关联的函数
 * 参数：   pfUser，用户指定的关联函数
 *          pvParm，代用户传递的参数
 *          uiPts，周期性调用关联函数所间隔的采样节拍数
 * 返回值： 无 */
void RD_Reg_Smpl_Func(void (*pfUser)(void *pvParm), void *pvParm, u_int uiPts);

/* 撤销早先注册的采样节拍关联的函数
 * 参数：   pfUser，用户指定的关联函数
 *          pvParm，代用户传递的参数
 *          uiPts，周期性调用关联函数所间隔的采样节拍数
 * 返回值： 无 */
void RD_Del_Smpl_Func(void (*pfUser)(void *pvParm), void *pvParm, u_int uiPts);

/* 控制LED状态
 * 参数：   pvLedHnd，用来索引LED对象的void指针，应该通过调用本模块提
 *              供的RD_Get_Handle得到
 *          bOn，LED灯的状态，TRUE=点亮；FALSE=熄灭
 * 返回值： 无 */
void RD_Set_LED(void *pvLedHnd, BOOL bOn);

/* 控制DO数据实时输出
 * 参数：   pvDoHnd，用来索引DO对象的void指针，应该通过调用本模块提
 *              供的RD_Get_Handle得到
 *          iVal，DO输出值
 * 返回值： 无 */
void RD_Set_DO(void *pvDoDat, int iVal);

#if 0
/* 取得实时DI数据
 * 参数：   pvDiHnd，用来索引DI数据元素的void指针，应该通过调用本模块提
 *              供的RD_Get_Handle得到
 * 返回值： TRUE，开入量闭合
 *          FALSE，开入量打开 */
BOOL RD_Get_DI(void *pvDiHnd);

/* 取得历史DI数据
 * 参数：   pvDiHnd，用来索引DI数据元素的void指针，应该通过调用本模块提
 *              供的RD_Get_Handle得到
 *          ulAiCnt，AI采样节拍计数器值
 * 返回值： TRUE，开入量闭合
 *          FALSE，开入量打开 */
BOOL RD_His_DI(void *pvDiHnd, uint32_t ulAiCnt);
#endif

/***********************************************************************
* Turn_Run_Led_High - 驱动运行灯LED，但不点亮，应该按高于50HZ的频率，变换驱动，否则，LED认为CPU未运行，不点亮,1.3BSP no
*
* RETURNS: 无
*
* Alert: 以前是BSP提供，合并版本后，由平台提供
*
*/
void Turn_Run_Led_High(
    BOOL ledStatus				/* TRUE; on    FALSE; off */
);

/***********************************************************************
* Turn_Flash_Led_High - 设置LED点亮与否，前提是Turn_Run_Led_High函数频繁变位;,1.3BSP no
*
* RETURNS: 无
*
* Alert: 以前是BSP提供，合并版本后，由平台提供
*/
void Turn_Flash_Led_High(
    BOOL ledStatus				/* TRUE; on    FALSE; off */
);

/***********************************************************************
* RD_Lgc_AI_Ofst - 取得AI逻辑通道数据指针的偏移量
*
* RETURNS:  用户逻辑通道数据指针相对0通道指针的偏移量
*
*/
static __inline__ int RD_Lgc_AI_Ofst(
    void *pvLgcAiHnd		/* 用来索引AI逻辑通道的void指针，应该通过调用RD_Get_Handle得到 */
)
{
    RD_LGC_AI_CH *plgcai;

    plgcai=(RD_LGC_AI_CH*)pvLgcAiHnd;

    return plgcai->pdat.pfLgcAI-lgcaidb_g.pfBufBgn; /* pfLgcAI表示在第一个存储点上存储位置 */
}

/***********************************************************************
* RD_Base_Msuc_AI_P - 取得MSU测量0通道数据指针
*
* RETURNS:  指向该MSU测量0通道数据（复数）的指针，可以把它作为MSU通道数据指针的
*                  基准，通过偏移运算得到用户预处理通道数据的指针
*
*/
static __inline__ COMPLEX *RD_Base_Msuc_AI_P()
{
    return msucaidb_g.pxBufBgn;
}

/***********************************************************************
* RD_Calc_AI_Ofst - 取得AI预处理通道数据指针的偏移量
*
* RETURNS:  用户预处理通道数据指针相对0通道指针的偏移量
*
*/
static __inline__ int RD_Calc_AI_Ofst(
    void *pvCalcAiHnd			/* 用来索引AI预处理通道的void指针，应该通过调用RD_Get_Handle得到 */
)
{
    RD_LGC_AI_CH *plgcai;

    plgcai=(RD_LGC_AI_CH*)pvCalcAiHnd;

    return plgcai->pdat.pxCalcAI-calcaidb_g.pxBufBgn; /* pxCalcAI表示在第一个保存点上存储位置 */
}

/***********************************************************************
* RD_Msuc_AI_Ofst - 取得MSU测量通道数据指针的偏移量
*
* RETURNS:  用户测量通道数据指针相对0通道指针的偏移量
*
*/
static __inline__ int RD_Msuc_AI_Ofst(
    void *pvMsucAiHnd			/* 用来索引MSU测量通道的void指针，应该通过调用RD_Get_Handle得到 */
)
{
    RD_MSU_AI_CH *pmsu;

    pmsu=(RD_MSU_AI_CH*)pvMsucAiHnd;

    return pmsu->pxMsucAI-msucaidb_g.pxBufBgn;
}

/***********************************************************************
* RD_Msuc_R - 取得MSU测量通道数据指针
*
* RETURNS:  用户测量通道数据指针
*
*/
static __inline__ COMPLEX *RD_Msuc_R(
    void *pvMsucAiHnd			/* 用来索引MSU测量通道的void指针，应该通过调用RD_Get_Handle得到 */
)
{
    RD_MSU_AI_CH *pmsu;

    pmsu=(RD_MSU_AI_CH*)pvMsucAiHnd;

    return pmsu->pxMsucAI;
}

/***********************************************************************
* RD_Lgc_AI - 取得实时实数AI数据
*
* RETURNS:  相应时刻相应通道的AI值
*
*/
extern float RD_Lgc_AI(
    float *pfLgcAi, 		/* 指向AI逻辑通道数据的指针，应该通过调用本模块提供的RD_Lgc_AI_P或者通过基准指针+偏移运算得到 */
    int iDelta			/* 距离该逻辑通道索引的采样间隔，只能<=0表示从前的点 */
);

/***********************************************************************
* RD_Msuc_AI - 取得复数测量计算数据
*
* RETURNS:  相应时刻相应测量通道的计算值（复数）
*
*/
static __inline__ COMPLEX *RD_Msuc_AI(
    COMPLEX *pxMsucAi, 			/* 指向MSU测量通道数据的指针，应该通过调用本模块提供的RD_Msuc_AI_P或通过基准指针+偏移运算得到 */
    int iDelta			/* 距离该逻辑通道索引的采样间隔，只能<=0表示从前的点 */
)
{
    if(pxMsucAi>=msucaidb_g.pxBufBgn)
    {
        pxMsucAi += iDelta;
    }

    return pxMsucAi;
}

/***********************************************************************
* RD_Adj_Calc_AI_P - 调整复数实时AI数据指针
*
* RETURNS: 指向相应时刻相应预处理通道的AI值（复数）的指针
* 注意: 该函数用来实现RD_Calc_Ai，绕过了inline返回值为COMPLEX的编译器bug，一般用户不应当使用。
*
*/
extern COMPLEX *RD_Adj_Calc_AI_P(
    COMPLEX *pxCalcAi, 			/* 指向AI预处理通道数据的指针，应该通过调用本模块提供的RD_Calc_AI_P或通过基准指针+偏移运算得到 */
    int iDelta			/* 距离该逻辑通道索引的采样间隔，只能<=0表示从前的点 */
);

/* 取得复数实时AI数据
 * 参数：   pxCalcAi，指向AI预处理通道数据的指针，应该通过调用本模块提
 *              供的RD_Calc_AI_P或通过基准指针+偏移运算得到
 *          iDelta，距离该逻辑通道索引的采样间隔，只能<=0表示从前的点
 * 返回值： 相应时刻相应预处理通道的AI值（复数） */
#define RD_Calc_AI(pxCalcAi, iDelta)   (*(RD_Adj_Calc_AI_P)(pxCalcAi, iDelta))

/***********************************************************************
* RD_Wr_Lgc_Vt_AI - 写实数虚拟通道AI数据
*
* RETURNS: 无
*
*/
extern void RD_Wr_Lgc_Vt_AI(
    float *pfLgcVtAi, 		/* 指向AI虚拟逻辑通道数据的指针，应该通过调用本模块提 */
    int iDelta, 		/* 距离该逻辑通道索引的采样间隔，负数表示从前的点，正数表示将来的点 */
    float fVal		/* 欲写的值 */
);

/***********************************************************************
* RD_Wr_Calc_Vt_AI - 写复数虚拟通道AI数据
*
* RETURNS: 无
*
*/
extern void RD_Wr_Calc_Vt_AI(
    COMPLEX *pxCalcVtAi,		/* 指向AI虚拟预处理通道数据的指针，应该通过调用本模块提供的RD_Calc_AI_P或者通过基准指针+偏移运算得到 */
    int iDelta, 		/* 距离该逻辑通道索引的采样间隔，负数表示从前的点，正数表示 */
    COMPLEX xVal				/* 欲写的值 */
);

/***********************************************************************
* RD_Get_DI - 取得实时DI数据
*
* RETURNS: TRUE，开入量闭合
*                 FALSE，开入量打开
*
*/
static __inline__ BOOL RD_Get_DI(
    void *pvDiHnd			/* 用来索引DI数据元素的void指针，应该通过调用本模块提供的RD_Get_Handle得到 */
)
{
    RD_LGC_DI_CH *plgcdi;

    plgcdi=(RD_LGC_DI_CH*)pvDiHnd;

    return ( plgcdi->iVal& 0x7FFF);
}

/***********************************************************************
* RD_Get_DI_Quality - 取得实时DI数据品质
*
* RETURNS: 品质位.
*
*/
static __inline__ uint16_t RD_Get_DI_Quality(
    void *pvDiHnd			/* 用来索引DI数据元素的void指针，应该通过调用本模块提供的RD_Get_Handle得到 */
)
{
    RD_LGC_DI_CH *plgcdi;

    assert(pvDiHnd);

    plgcdi=(RD_LGC_DI_CH*)pvDiHnd;

    return plgcdi->usQuality;
}

/* 取得DI通道数据指针的偏移量.
 * Para:
 *     NONE.
 * Return:
 *     偏移量.
 */
static __inline__ uint32_t RD_DI_Ofst(void *pvDiHnd)
{
    return ((RD_LGC_DI_CH *)pvDiHnd)->ChOffset;
}

/***********************************************************************
* RD_Get_DIMod - 取得实时DI模件的32位开入状态
*
* RETURNS: 该模件32位所有开入的当前状态
*
*/
static __inline__  uint32_t  RD_Get_DIMod(
    void *pvDiModHnd			/* 用来索引DIMod模件的void指针，应该通过调用本模块提供的RD_Get_Handle得到 */
)
{

    return (((RD_PART_INFO  *)pvDiModHnd)->ulDIModCurVaule);
}

/* 取得当前AI采样节拍计数器的值
 * 参数：   无
 * 返回值： AI采样节拍计数器值，该32位计数器自由运行，用来表示AI
 *              数据的采样时刻 */
static __inline__ uint32_t RD_AI_Cnt(void)
{
    return rdinfo_g.ulCurrAiCnt;
}

/* AI逻辑采样通道数据指针后移，指向晚uiPts个采样点的原通道数据
 * 参数：	pf，用来索引AI逻辑通道的指针（最初由RD_Lgc_AI_P返回）
 * 返回值：	无
 * 注意：	它们采用宏来实现，结果直接修改在pf中  */
#define RD_ADD_LGC_AI_P(pf, uiPts)  do\
{\
    (pf)=(float*)((uint8_t*)(pf)+(u_int)(uiPts)*lgcaidb_g.uiChBytes);\
    if ((pf)>=lgcaidb_g.pfBufEnd)\
        pf=(float*)((uint8_t*)pf-lgcaidb_g.ulBufBytes);\
} while (0)

/* AI逻辑采样通道数据指针前移，指向早uiPts个采样点的原通道数据
 * 参数：	pf，用来索引AI逻辑通道的指针（最初由RD_Lgc_AI_P返回）
 * 返回值：	无
 * 注意：	它们采用宏来实现，结果直接修改在pf中  */
#define RD_SUB_LGC_AI_P(pf, uiPts)  do\
{\
    (pf)=(float*)((uint8_t*)(pf)-(u_int)(uiPts)*lgcaidb_g.uiChBytes);\
    if ((pf)<lgcaidb_g.pfBufBgn)\
        pf=(float*)((uint8_t*)pf+lgcaidb_g.ulBufBytes);\
} while (0)

/* AI预处理通道数据指针后移，指向晚uiPts个采样点的原通道数据
 * 参数：	px，用来索引AI预处理通道的指针（最初由RD_Calc_AI_P返回）
 * 返回值：	无
 * 注意：	它们采用宏来实现，结果直接修改在px中 */
#define RD_ADD_CALC_AI_P(px, uiPts)  do\
{\
    (px)=(COMPLEX*)((uint8_t*)(px)+(u_int)(uiPts)*calcaidb_g.uiChBytes);\
    if ((px)>=calcaidb_g.pxBufEnd)\
        px=(COMPLEX*)((uint8_t*)px-calcaidb_g.ulBufBytes);\
} while (0)

/* AI预处理通道数据指针前移，指向早uiPts个采样点的原通道数据
 * 参数：	px，用来索引AI预处理通道的指针（最初由RD_Calc_AI_P返回）
 * 返回值：	无
 * 注意：	它们采用宏来实现，结果直接修改在px中 */
#define RD_SUB_CALC_AI_P(px, uiPts)  do\
{\
    (px)=(COMPLEX*)((uint8_t*)(px)-(u_int)(uiPts)*calcaidb_g.uiChBytes);\
    if ((px)<calcaidb_g.pxBufBgn)\
        px=(COMPLEX*)((uint8_t*)px+calcaidb_g.ulBufBytes);\
} while (0)

/* AI逻辑采样通道状态指针(8位)后移,指向晚uiPts个采样点的原通道状态
 * 参数: pu,用来索引AI逻辑通道的指针（最初由RD_Get_Chn_Sts返回）
 * 返回值: 无
 * 注意: 它们采用宏来实现,结果直接修改在pu中 */
#define RD_ADD_LGC_AI_STS_P(pu, uiPts)  do\
{\
    assert ((pu) >= (uint8_t *)lgcaistsdb_g.pBufBgn && (pu)<(uint8_t *)lgcaistsdb_g.pBufEnd);\
	(pu) = ((uint8_t *)(pu)+(u_int)(uiPts)*4*lgcaidb_g.uiTotalCh);\
    if ((pu) >= (uint8_t *)lgcaistsdb_g.pBufEnd)\
		pu = ((uint8_t *)pu-4*lgcaidb_g.ulBufLen);\
} while (0)

#define RD_INC_LGC_AI_STS_P(pu) RD_ADD_LGC_AI_STS_P(pu, 1)

/* AI逻辑采样通道状态指针(8位)前移,指向早uiPts个采样点的原通道数据
 * 参数：pu,用来索引AI逻辑通道状态的指针（最初由RD_Get_Chn_Sts返回）
 * 返回值: 无
 * 注意: 它们采用宏来实现,结果直接修改在pf中 */
#define RD_SUB_LGC_AI_STS_P(pu, uiPts)  do\
{\
    assert ((pu) >= (uint8_t *)lgcaistsdb_g.pBufBgn && (pu)<(uint8_t *)lgcaistsdb_g.pBufEnd);\
    (pu) = ((uint8_t *)(pu)-(u_int)(uiPts)*4*lgcaidb_g.uiTotalCh);\
    if ((pu)<(uint8_t *)lgcaistsdb_g.pBufBgn)\
        pu = ((uint8_t *)pu+4*lgcaidb_g.ulBufLen);\
} while (0)

/* AI逻辑采样通道状态指针(32位)后移,指向晚uiPts个采样点的原通道状态
 * 参数: pu,用来索引AI逻辑通道的指针（最初由RD_Get_Chn_Sts_All返回）
 * 返回值: 无
 * 注意: 它们采用宏来实现,结果直接修改在pu中 */
#define RD_ADD_LGC_AI_STS_ALL_P(pu, uiPts)  do\
{\
    assert ((pu) >= lgcaistsdb_g.pBufBgn && (pu)<lgcaistsdb_g.pBufEnd);\
	(pu) = ((uint32_t *)(pu)+(u_int)(uiPts)*lgcaidb_g.uiTotalCh);\
    if ((pu) >= lgcaistsdb_g.pBufEnd)\
		pu = ((uint32_t *)pu-lgcaidb_g.ulBufLen);\
} while (0)

#define RD_INC_LGC_AI_STS_ALL_P(pu) RD_ADD_LGC_AI_STS_ALL_P(pu, 1)

/* AI逻辑采样通道状态指针(32位)前移,指向早uiPts个采样点的原通道数据
 * 参数：pu,用来索引AI逻辑通道状态的指针（最初由RD_Get_Chn_Sts_All返回）
 * 返回值: 无
 * 注意: 它们采用宏来实现,结果直接修改在pf中 */
#define RD_SUB_LGC_AI_STS_ALL_P(pu, uiPts)  do\
{\
    assert ((pu) >= lgcaistsdb_g.pBufBgn && (pu)<lgcaistsdb_g.pBufEnd);\
    (pu) = ((uint32_t *)(pu)-(u_int)(uiPts)*lgcaidb_g.uiTotalCh);\
    if ((pu)<lgcaistsdb_g.pBufBgn)\
        pu = ((uint32_t *)pu+lgcaidb_g.ulBufLen);\
} while (0)

/* AI逻辑采样通道数据指针后移，指向晚1个采样点的原通道数据
 * 参数：	pf，用来索引AI逻辑通道的指针（最初由RD_Lgc_AI_P返回）
 * 返回值：	无
 * 注意：	它们采用宏来实现，结果直接修改在pf中  */
#define RD_INC_LGC_AI_P(pf)     RD_ADD_LGC_AI_P(pf, 1)

/* AI逻辑采样通道数据指针前移，指向早1个采样点的原通道数据
 * 参数：	pf，用来索引AI逻辑通道的指针（最初由RD_Lgc_AI_P返回）
 * 返回值：	无
 * 注意：	它们采用宏来实现，结果直接修改在pf中  */
#define RD_DEC_LGC_AI_P(pf)     RD_SUB_LGC_AI_P(pf, 1)

/* AI预处理通道数据指针后移，指向晚1个采样点的原通道数据
 * 参数：	px，用来索引AI预处理通道的指针（最初由RD_Calc_AI_P返回）
 * 返回值：	无
 * 注意：	它们采用宏来实现，结果直接修改在px中 */
#define RD_INC_CALC_AI_P(px)    RD_ADD_CALC_AI_P(px, 1)

/* AI预处理通道数据指针前移，指向早1个采样点的原通道数据
 * 参数：	px，用来索引AI预处理通道的指针（最初由RD_Calc_AI_P返回）
 * 返回值：	无
 * 注意：	它们采用宏来实现，结果直接修改在px中 */
#define RD_DEC_CALC_AI_P(px)    RD_SUB_CALC_AI_P(px, 1)

/* 转换AI采样节拍计数器的值为系统us时钟
 * 参数：   AI采样节拍计数器值
 * 返回值： 该采样时刻对应的系统32位us时钟 */
uint32_t RD_AI_Cnt_To_us(uint32_t ulCnt);

/* 取得模块AI逻辑通道和预处理数据指针
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulSmplClk，递增采样时钟，该时钟在同步脉冲到来的时刻清零
 *          ppxWr，用来返回指向该AI引擎的第0个预处理通道数据的指针
 * 返回值： 指向该AI引擎的第0个逻辑采样通道数据的指针
 * 注意：   要求AI引擎按照采样次序调用此函数获得指针来写实时数据，
 *          AI引擎可以对指针进行直接的写操作（如果没有预处理通道，则*ppxWr无效），
 *          内部数据按照通道优先的方式进行组织 */
float *RD_AI_Dat_P(void *pvAiMod, uint32_t ulSmplClk, COMPLEX **ppxWr, float **dcdata);

/* 报告AI引擎采样同步信息
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulTime，采样同步脉冲到来时的系统32位us时钟
 *          ulLastClk，清零前的采样时钟值
 * 返回值： 无 */
void RD_Syn_AI_Clk(void *pvAiMod, uint32_t ulTime, uint32_t ulLastClk);

/* 获得原始采样时钟值，该值可用来确定预处理通道傅氏变换所采用的系数
 * 参数：   AI采样节拍计数器值
 * 返回值： 该采样时刻对应的原始采样时钟值 */
uint32_t RD_Get_Smpl_Clk(uint32_t ulCnt);

/* 报告AI引擎完成一次数据刷新
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 * 返回值： 无 */
void RD_End_Ai_Wr(void *pvAiMod);

/* Get hardware AI channel attribution.
 * Parameters:
 *      iIdx, index of the hardware AI(from 0).
 * Return value:
 *      Pointer to the hardware AI attribution structure.
 *      NULL if iIdx is invalid(>=iHwAiChNum_g). */
const RD_HW_AI_CH *RD_Get_Hw_AI_Attr(int iIdx);

/* Get DI channel attribution.
 * Parameters:
 *      iIdx, index of the DI(from 0).
 * Return value:
 *      Pointer to the DI attribution structure.
 *      NULL if iIdx is invalid(>=iLgcDiChNum_g). */
const RD_LGC_DI_CH *RD_Get_DI_Attr(int iIdx);

/* Get DO channel attribution.
 * Parameters:
 *      iIdx, index of the DO(from 0).
 * Return value:
 *      Pointer to the DO attribution structure.
 *      NULL if iIdx is invalid(>=iLgcDoChNum_g). */
const RD_LGC_DO_CH *RD_Get_DO_Attr(int iIdx);

/* Get hardware LED channel attribution.
 * Parameters:
 *      iIdx, index of the hardware LED(from 0).
 * Return value:
 *      Pointer to the LED attribution structure.
 *      NULL if iIdx is invalid(>=iHwLedChNum_g). */
const RD_LGC_LED_CH *RD_Get_Hw_Led_Attr(int iIdx);

/* Get software LED channel attribution.
 * Parameters:
 *      iIdx, index of the software LED(from 0).
 * Return value:
 *      Pointer to the LED attribution structure.
 *      NULL if iIdx is invalid(>=iSwLedChNum_g). */
const RD_LGC_LED_CH *RD_Get_Sw_Led_Attr(int iIdx);

/* Read all hardware LEDs' value.
 * Parameters:
 *      pbRslt, to save all hardware LEDs' current value.
 * Return value:
 *      None.
 * Alert:
 *      pbRslt must contains space to save iHwLedChNum_g BOOL numbers. */
void RD_Rd_Hw_Led_Val(BOOL *pbRslt);

/* Read all software LEDs' value.
 * Parameters:
 *      pbRslt, to save all software LEDs' current value.
 * Return value:
 *      None.
 * Alert:
 *      pbRslt must contains space to save iSwLedChNum_g BOOL numbers. */
void RD_Rd_Sw_Led_Val(BOOL *pbRslt);

/* 检查测量AI对应的实时AI数据索引
 * 参数：   strMeaAiId，遥测逻辑标识字符串
 * 返回值： 用来索引实时AI数据I/O通道的void指针，或者NULL表示找不到 */
void *RD_Mea_AI_Hnd(uint8_t *strMeaAiId);

/* 检查测量DI对应的实时DI数据索引
 * 参数：   strMeaDiId，遥信逻辑标识字符串
 *          iMeaCh，遥信通道号
 * 返回值： 用来索引实时DI数据I/O通道的void指针，或者NULL表示找不到 */
void *RD_Mea_DI_Hnd(uint8_t *strMeaDiId, int iMeaCh, BOOL bSOE);

/* 检查录波AI对应的实时AI数据索引
 * 参数：   strRecAiId，录波AI逻辑标识字符串
 * 返回值： 用来索引实时AI数据I/O通道的void指针，或者NULL表示找不到 */
void *RD_Rec_AI_Hnd(uint8_t *strRecAiId);

/* 检查录波DI对应的实时DI数据索引
 * 参数：   strRecDiId，录波DI逻辑标识字符串
 * 返回值： 用来索引实时DI数据I/O通道的void指针，或者NULL表示找不到 */
void *RD_Rec_DI_Hnd(uint8_t *strRecDiId);

/* 检查标志量AI对应的实时AI数据索引
 * 参数：   strFlagAiId，标志量AI逻辑标识字符串
 * 返回值： 用来索引实时AI数据I/O通道的void指针，或者NULL表示找不到 */
void *RD_Flag_AI_Hnd(uint8_t *strFlagAiId);

/* 检查标志量DI对应的实时DI数据索引
 * 参数：   strFlagDiId，标志量DI逻辑标识字符串
 * 返回值： 用来索引实时DI数据I/O通道的void指针，或者NULL表示找不到 */
void *RD_Flag_DI_Hnd(uint8_t *strFlagDiId);

/* Read all hardware AI measurment value.
 * Parameters:
 *      phwmeaRslt, to save all hardware AI measurement value.
 *      bIsCalc, 是否全部计算标识, TRUE, 全部计算; FALSE, 部分计算.
 * Return value:
 *      None.
 * Alert:
 *      phwmeaRslt must contains space to save iHwAiChNum_g members. */
void RD_Mea_Hw_AI(RD_HW_AI_MEA *phwmeaRslt, BOOL bIsCalc);

/***********************************************************************
* RD_Mea_Po - Read all pulse output measurment value
*
* RETURNS: None
*
* Alert:
*        ppomeaRslt must contains space to save iLgcPoChNum_g members.
*/
void RD_Mea_Po(
    RD_PO_MEA *ppomeaRslt		/* to save all pulse output measurement value */
) ;

/* Read force DI information from setting file.
 * Parameters:
 *      None.
 * Return value:
 *      EP_SUCCESS, read file OK.
 *      EP_FILE_ERR, file format error. */
EP_STATUS RD_Rd_Force_DI(void);

/* Change force DI status.
 * Parameters:
 *      iIdx, index of DI(from 0).
 *      iSts, new status: TRUE, FALSE or -1 means release force.
 * Return value:
 *      EP_SUCCESS, change link status OK.
 *      EP_FILE_ERR, file operating failure. */
EP_STATUS RD_Chg_Force_DI(int iIdx, int iSts);

/* Read hardware DI measurement value.
 * Parameters:
 *      iIdx, index of DI(from 0).
 * Return value:
 *      Low 15 bit is now status(TRUE or FALSE) while bit15(0x8000) is flag of
 *          force DI. */
int RD_Mea_Hw_DI(int iIdx);

/* 强制DO输出（用于遥控/开出传动等）
 * 参数：   iIdx, 开出量索引，从0开始
 *          iSts, 预设置的状态: TRUE, FALSE or -1 解除强制.
 * 返回值： 无 */
void RD_Force_DO(int iIdx, int iSts);

/* Read hardware DO status.
 * Parameters:
 *      iIdx, index of DO(from 0).
 * Return value:
 *      Low 15 bit is now status(TRUE or FALSE) while bit15(0x8000) is flag of
 *          force DO. */
int RD_Mea_Hw_DO(int iIdx);

/* 取得采样数据有效状态,
 * 参数：   ulCnt，AI采样节拍计数器值，该32位计数器自由运行，用来表示AI
 *              数据的采样时刻
 * 返回值： 该采样点的实时数据有效性,TRUE为有效，FALSE为无效 */
BOOL   RD_Get_Data_Valid(uint32_t ulCnt);

float *RD_AI_Msu_Dat_P(void);
void *RD_Msu_AI_Hnd(uint8_t *strMsuLgAiId);
void Start(void);

/***********************************************************************
* GetUnitType - 获得单位类型
*
* RETURNS: 无
*
*/
void GetUnitType(void);

/***********************************************************************
* GetDICode -得到开入量的序号根据开入量的名称
*
* RETURNS: 序号
*
*/
int  GetDICode(char * DiName);

/***********************************************************************
* GetDOCode -得到开入量的序号根据开入量的名称
*
* RETURNS: 序号
*
*/
int  GetDOCode(char * DoName);

/***********************************************************************
* GetLedCode -得到信号灯的序号根据信号灯的名称
*
* RETURNS: 序号
*
*/
int  GetLedCode(char * LedName);

/***********************************************************************
* VI_DI_Change -DI状态改变记录
*
* RETURNS: NONE
*
*/
void VI_DI_Change(
    RD_LGC_DI_CH *pDi, 		/* RD_LGC_DI_CH channel */
    BOOL bSts, 		/* new DI status */
    uint32_t ulTime			/* us time */
);

/***********************************************************************
* GetPoVal - 获取PO值
*
* RETURNS: 无
*
*/
void GetPoVal(RD_PO_MEA *phwmeaRslt);

/***********************************************************************
* GetPofVal - 获取PO值
*
* RETURNS: 无
*
*/
void GetPofVal(float *fRslt);

/* 测试 */
void BufTest(void);

void AddrCheckLog(uint32_t CheckAddr);

void AddrCheckCalc(uint32_t CheckAddr);

/***********************************************************************
* RD_Clear_All_Phy_DO - 清除所有物理DO通道的状态,用于中压复归时首先清除通道状态
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
void RD_Clear_All_Phy_DO();

/***********************************************************************
* SlakeUnkeepedLed - 熄灭非自保持信号灯
*
* RETURNS: 无
*
*/
void  SlakeUnkeepedLed(void);

/***********************************************************************
* RD_Get_DSP_MOD_Info - 获得DSP MOD的当前的信息，供其他机箱的MOD来使用
*
* RETURNS:
*              EP_SUCCESS, 成功
*              EP_ERROR, 失败
*
*/
EP_STATUS RD_Get_DSP_MOD_Info(
    uint32_t *pulRtNextCnt,			/* 供返回的此时的MOD信息中的nextcnt */
    int *piRtDbOfst						/* 供返回的此时的MOD在DB中的偏移 */
);

/***********************************************************************
* RD_LightRunLamp - 保护CPU点亮或熄灭运行灯
*
* RETURNS: None
*
* Alert: 保护应用程序需要在快速保护任务中调用该函数。函数调用频率必须大于50HZ，否则运行灯熄灭
*
*/
void RD_LightRunLamp(
    BOOL bLightLamp			/* 点亮运行灯，TRUE: 点亮，FALSE: 熄灭 */
);
/***********************************************************************
* RD_LightAlarmLamp - 保护CPU点亮或熄灭告警灯
*
* RETURNS: None
*
* Alert: 保护应用程序调用该函数。
*函数调用只需变位调用,和平台产生的告警是或逻辑
*
*/
void RD_LightAlarmLamp(
    BOOL bLightLamp			/* 点亮告警灯，TRUE: 点亮，FALSE: 熄灭 */
);

/* 根据采样通道指针获取状态标指针(8位)
 * Para:
 *     pf, 采样通道指针.
 * Return:
 *     状态指针(8位).
*/
extern uint8_t *RD_Cnvrt_AI_P_to_Sts_P(float *pf);

/* 根据句柄和采样节拍获取状态标指针(8位)
 * Para:
 *     pvLgcAiHnd, 句柄.
 *     ulSampleCnt, 节拍.
 * Return:
 *     状态指针(8位).
*/
extern uint8_t *RD_Get_Chn_Sts (void *pvLgcAiHnd, uint32_t ulSampleCnt);

/* 获得当前所有采样源最新有效的采样节拍,
   注意:目前只处理本地采样（包括本地，扩展，数字化（智能操作箱，9-1，数据集中器））和光差采样，
        其他的暂时不考虑（比如励磁，同杆，虚拟机箱）
   参数:无
   返回:返回所有采样源的有效采样节拍
*/
extern uint32_t   RD_GetAllAIValidCnt();

/* 查询通道网络端口来源(根据通道逻辑标志查询,通道正常后,即AI_COM_ERR清0时才能读取到有效对应关系)
 * Para:
 *     pvLgcAiHnd, 逻辑标志.
 *     pSlvPortNum, 前级CC板端口号,返回值为0xFF时无效
 * Return:
 *     本级CC板端口号,返回值为0xFF时无效
 */
extern uint8_t RD_Get_Chn_Src (void *pvLgcAiHnd, uint8_t *pSlvPortNum);

/* 根据采样通道指针获取状态标指针(32位)
 * Para:
 *     pf, 采样通道指针.
 * Return:
 *     状态指针(32位).
 */
extern uint32_t *RD_Cnvrt_AI_P_to_Sts_All_P(float *pf);

/* 根据句柄和采样节拍获取状态标指针(32位)
 * Para:
 *     pvLgcAiHnd, 句柄.
 *     ulSampleCnt, 节拍.
 * Return:
 *     状态指针(32位).
 */
extern uint32_t *RD_Get_Chn_Sts_All (void *pvLgcAiHnd, uint32_t ulSampleCnt);

/* 查询通道svID(根据通道逻辑标志,通道正常后,即AI_COM_ERR清0时才能读取到有效对应关系)
 * Para:
 *     pvLgcAiHnd, 逻辑标志.
 * Return:
 *     svID存储地址
 */
extern uint8_t *RD_Get_svID_Src (void *pvLgcAiHnd);

/***********************************************************************
* RD_Modify_DI - 写入开入板状态，处理变位等信息
*
* RETURNS: 无
*
*/
extern void RD_Modify_DI(
    RD_LGC_DI_CH *plgcdi,		/* DI配置 */
    int  iNowVal		/* 配置值 */
);

/***********************************************************************
* SynSamAdjust - 同步节拍调整
*
* RETURNS: 调整后节拍
*
*/
extern uint8_t SynSamAdjust(
    uint8_t uInputSam,		/* 当前节拍 */
    int iDelta		/* 需调整节拍 */
);

/* 获取GOOSE DO关联压板状态
 * Para:
 *     iIdx, 序号.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL RD_Get_DO_Link(int iIdx);

/***********************************************************************
* RD_Lgc_AI_P - 取得AI逻辑通道数据指针
*
* RETURNS:  指向该AI逻辑采样通道数据的指针，可以对它进行直接的读操作，
*                  也可以通过RD_Lgc_AI调用来获得历史数据
*
*/
extern float *RD_Lgc_AI_P(
    void *pvLgcAiHnd, 		/* 用来索引AI逻辑通道的void指针，应该通过调用RD_Get_Handle得到 */
    uint32_t ulCnt		/* AI采样节拍计数器值，该32位计数器自由运行，用来表示AI数据的采样时刻 */
);

/***********************************************************************
* RD_Lgc_AI_P_MC - 取得AI逻辑通道数据指针(测控使用提高效率)
*
* RETURNS:  指向该AI逻辑采样通道数据的指针，可以对它进行直接的读操作，
*                  也可以通过RD_Lgc_AI调用来获得历史数据
*
*/
extern float *RD_Lgc_AI_P_MC(
    void *pvLgcAiHnd, 		/* 用来索引AI逻辑通道的void指针，应该通过调用RD_Get_Handle得到 */
    uint32_t ulCnt		/* AI采样节拍计数器值，该32位计数器自由运行，用来表示AI数据的采样时刻 */
);

/***********************************************************************
* RD_Calc_AI_P - 取得AI预处理通道数据指针
*
* RETURNS:  指向该AI预处理通道数据（复数）的指针，可以对它进行直接的读操作，
*                  也可以通过RD_Calc_AI调用来获得历史数据
*
*/
extern COMPLEX *RD_Calc_AI_P(
    void *pvCalcAiHnd, 	/* 用来索引AI预处理通道的void指针，应该通过调用RD_Get_Handle得到 */
    uint32_t ulCnt		/* AI采样节拍计数器值，该32位计数器自由运行，用来表示AI数据的采样时刻 */
);

/***********************************************************************
* RD_Base_Lgc_AI_P - 取得AI逻辑0通道数据指针
*
* RETURNS:  指向该AI逻辑采样通道数据的指针，可以把它作为AI通道数据指针的
*                  基准，通过偏移运算得到用户逻辑通道数据的指针
*
*/
extern float *RD_Base_Lgc_AI_P(
    uint32_t ulCnt				/* AI采样节拍计数器值，该32位计数器自由运行，用来表示AI 数据的采样时刻 */
);

/***********************************************************************
* RD_Base_Calc_AI_P - 取得AI预处理0通道数据指针
*
* RETURNS:  指向该AI预处理0通道数据（复数）的指针，可以把它作为AI通道数据指针的
*                  基准，通过偏移运算得到用户预处理通道数据的指针
*
*/
extern COMPLEX *RD_Base_Calc_AI_P(
    uint32_t ulCnt		/* AI采样节拍计数器值，该32位计数器自由运行，用来表示AI数据的采样时刻 */
);

/***********************************************************************
* RD_His_DI - 取得历史DI数据
*
* RETURNS: TRUE，开入量闭合
*                 FALSE，开入量打开
*
*/
extern BOOL RD_His_DI(
    void *pvDiHnd, 			/* 用来索引DI数据元素的void指针，应该通过调用本模块提供的RD_Get_Handle得到 */
    uint32_t ulAiCnt						/* AI采样节拍计数器值 */
);

/* 取得历史DI数据缓冲的某采样节拍的基址
 * 参数：   pvDiHnd，用来索引DI数据元素的void指针，应该通过调用本模块提
 *              供的RD_Get_Handle得到
 *          ulAiCnt，AI采样节拍计数器值
 * 返回值： TRUE，开入量闭合
 *          FALSE，开入量打开 */
extern BOOL * RD_Base_His_DI_P( uint32_t ulAiCnt);

/* 查询通道相应的ASDU序号以及是否采样延时越限(针对9-2报文)
 * Para:
 *     pvLgcAiHnd, 逻辑标志.
 *     pSmvAdsuNo, 返回ASDU序号.
 *     pMuDelay,   返回该ASDU延时时间.
 * Return:
 *     采样延时越限标志, TRUE为越限, FALSE为正常.
 */
extern BOOL RD_Get_DelayOverFlow (void *pvLgcAiHnd, int *pSmvAdsuNo, int *pMuDelay);

/* 多个开入一起强制.
 * Parameters:
 *     pRcvDiBuf, 报文指针.
 * Return value:
 *     EP_SUCCESS, force DI OK.
 *     EP_ERROR, failure else.
 *     EP_FILE_ERR, file operating failure.
 */
extern EP_STATUS RD_Chg_Force_Multi_Di(uint8_t *pRcvDiBuf);

/***********************************************************************
* RD_Get_Org_DI - 获得DI通道某个时间点的值
*
* RETURNS: 无
*
*/
extern void RD_Get_Org_DI(void);

/***********************************************************************
* RD_Get_Src_DI - 取得原始实时DI数据,若为本地开入,来源为SPI通讯的结构
*
* RETURNS: TRUE，开入量闭合
*                 FALSE，开入量打开
*
*/
extern BOOL RD_Get_Src_DI(void *pSrc);

#ifdef  __cplusplus
}
#endif

#endif                                  /* REALDATA_H */
