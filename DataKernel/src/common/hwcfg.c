/* hwcfg.c - subroutine library for parsing the hardware configuration */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 11nov06, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for parsing the hardware configuration.
INCLUDES: hwcfg.h
*/

/* includes */

#include "hwcfg.h"
#include "dspai.h"
#include "realdata.h"
#include "errtest.h"
#include "dspai.h"
#include "ext_box.h"
#include "datetime.h"
#include "logmsg.h"
#include "miscfunc.h"
#include "view.h"
#include "filetool.h"
#include "sysinfo.h"
#include "swcfg.h"

#include <stdio_compat.h>
#include "string_compat.h"
#include "ctype_compat.h"
#include <ioLib.h>
#include <intLib.h>
#include <taskLib.h>
#include <semLib.h>
#include "dsp.h"

#include "filetool.h"
#include "string_compat.h"
#include "ctype_compat.h"
#include <stdio_compat.h>
#include <dirent_compat.h>
#include <sys/stat.h>
#include <ioLib.h>
#include <lstLib.h>
#include <semLib.h>
#include <dosFsLib.h>
#include "OPT_Data.h"

// #include "Eth_callback.h"
#include "GO_Interface.h"    /* 保护模型与61850的接口 */
// #include "Gmrp.h"
#include "VTBOX_Interface.h"		/* 虚拟机箱 */
#include "VTBOX_SamInterface.h"

#if defined(EDP_01_02_BUILD)
#include "spiio.h"
#include "spi_mutual.h"
#endif

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
#include "io_Drv.h"
#endif

// #include "OPT_data.h"
#include "POLE_VtBox.h"
#include "HDL_VtBox.h"
#include "VTBOX_Data.h"
#include "AppInterface.h"
#include "OPT_VtBox.h"
#include "VTBOX_Box.h"

#if defined(EXCITE_BUILD)
#include "redun_box.h"
#endif

#include "edp_asst.h"
#include "bspinterface.h"
#include "FileCRC.h"
#include "smvcfg.h"
#include "iecgoose.h"
#include "EDP_UnifiedCfgInit.h"

/* defines */

#define POFESTRAMMEM  		/* 铁电保存宏 */
#define MAXPONUM 50		/* 最大电度量个数 */
#define UNIT_ID "Unit 1"  /* 缺省的间隔单元名称 */

/* locals */

static BOOL bPofestramInitOK_s = FALSE;		/* 因数字化添加本变量 */
static BOOL bSpiModuleUsed=FALSE;                   /* 是否有SPI模件 决定是否要启动SPI通信 */
static BOOL bMbDiUsed=FALSE;                           /* 是否启用母板SPI */
static BOOL bFstValChg_g = FALSE;  /* 一次额定值更新标志 */
static BOOL bGooseCfgFlag = FALSE;  /* GOOSE配置是否初始化成功标志 */

/* globals */

int iHwAoChNum_g;
RD_HW_AO_CH *phwaoch_g;/*AO配置 */
DA_PART_AO_CFG PartAoCfgDa_g;  /* DA输出的AO配置 */
int iLgcPiChNum_g;
RD_LGC_PI_CH *plgcpich_g;
int iLgcPoChNum_g;
RD_LGC_PO_CH *plgcpoch_g;
int iHwAiChNum_g;
int iLineNum_g = 1;  /* 单元数 */
RD_HW_AI_CH *phwaich_g;
RD_AI_MOD aimodExt_g;
int iLgcAiChNum_g;
RD_LGC_AI_CH *plgcaich_g;
int iVtAiChNum_g;
RD_VT_AI_CH *pvtaich_g;
int iLgcDoChNum_g;
RD_LGC_DO_CH *plgcdoch_g;
RD_LGC_LED_CH *plgcledch_g;
int iLgcLedChNum_g;
int iHwLedChNum_g;
int iSwLedChNum_g;
int iMsuAiChNum_g;
RD_MSU_AI_CH *pmsuaich_g;
uint16_t AIBaseCh_g;
RD_SYS_INFO rdinfo_g;
RD_AI_MOD aimodDsp_g;
RD_AI_MOD aimodRedun_g; 				/* 冗余机箱 */
RD_AI_MOD aimodPole_g;    /*为同杆并架添加  2007-3-20  */
RD_AI_MOD aimodHdl_g;
RD_AI_MOD aimodOpt_g[2];  /*为光纵添加 2006-2-8 */
RD_AI_MOD aimodVtBox_g[MAX_VT_BOX_COUNT];			/* 虚拟机箱 */
RD_PART_INFO apartinf_g[MAX_PART_NUM] __attribute__((section(".bss_dtcm")));
int iLgcDiChNum_g;
RD_LGC_DI_CH *plgcdich_g;
BOOL bStopRefreshData=FALSE;
int FestRamErrorMaxNum=0;
int FestRamWrSuccessNum=0;
int FestRamWrFailNum=0;
int FestI2CErrorMaxNum=0;
ACMOULDTYPE AdMdType;
ENVIROMENTTYPE EnviromentType;
SEM_ID semI2CWrEnableFlag; /* I2C总线访问许可 */
uint16_t uBaseUnitFstRatedVal_g;							/* 一次额定值 */
uint16_t uBaseUnitSecRatedVal_g;			/* 二次额定值 */
uint32_t g_RdBufCyc = 0;

extern uint16_t Sam_Counter; 			/* The calculation number */

/* global functions */

/* 是否使用SPI-IO模件标志 */
extern BOOL RD_GetSpiModuleFlag();

/* 是否使用母板DI */
extern BOOL RD_GetMbDIUsedFlag(void);

/* 获取GOOSE接收时间 */
extern BOOL HDL_Get_T(void *pvDiCh, uint32_t *ultime);

/* 设置母板DI使用标志 */
extern void RD_SetMbDIUsed();

/* 设置SPI-IO板使用标志 */
extern void RD_SetSpiModuleUsed();

/*  初始化同杆并架AO所有配置
     参数：
          iAOCfgNum:所有AO配置个数
     返回：成功与否*/
extern EP_STATUS   HDL_InitAOCfg(int  iAOCfgNum);

/* 更新一次额定值
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void UpdateSmvValIn(void);

extern int SubQueueCallback_goose(UINT8 port, UINT8 *ptr, int revSubLen);

/* static functions */

/***********************************************************************
* RD_Deal_Cfg_Item - 分项配置
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Deal_Cfg_Item(
    RD_HW_CFG_ITEM *pcfg			/* 配置指针 */
);

/***********************************************************************
* RD_Cfg_Resource - 装置资源配置项目
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_Resource(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
);

/***********************************************************************
* RD_Cfg_Hw_AI - 模拟输入硬件通道配置项目
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_Hw_AI(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
);

/***********************************************************************
* RD_Cfg_Lgc_AI - 模拟输入逻辑通道配置项目
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_Lgc_AI(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
);

/***********************************************************************
* RD_Cfg_DI - 开入配置项目
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_DI(
    uint8_t *pucCfg,		/* 开入 */
    uint32_t ulLen	/* 长度 */
);

/***********************************************************************
* RD_Cfg_DO - 开出配置项目
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_DO(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
);

/***********************************************************************
* RD_Cfg_Sw_LED - 屏幕指示灯配置
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_Sw_LED(
    uint8_t *pucCfg,	/* 配置指针 */
    uint32_t ulLen				/* 长度 */
);

/***********************************************************************
* RD_Cfg_Hw_LED - 面板指示灯配置
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_Hw_LED(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
);

/***********************************************************************
* RD_Cfg_Virtual_AI - 虚拟模拟输入通道配置
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_Virtual_AI(
    uint8_t *pucCfg,		/* 配置结构 */
    uint32_t ulLen		/* 长度 */
);

/***********************************************************************
* RD_Cfg_Msu_AI - 测量通道配置
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_Msu_AI(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
);

/***********************************************************************
* RD_Cfg_Hw_AI_Gain - 读取增益系数文件
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*/
static EP_STATUS RD_Cfg_Hw_AI_Gain(
    int iFd		/* 文件句柄 */
);

/***********************************************************************
* RD_Cfg_CT_Ratio - 读取CT变比系数文件
*
* RETURNS: 无
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*/
static EP_STATUS RD_Cfg_CT_Ratio(
    int iFd		/* 文件句柄 */
);

/* global functions */

/* 发送扩展机箱通道系数
 * Para:
 *     uiLgcCh, 通道数.
 *     plgccfg, 通道配置.
 * Return:
 *     result.
 */
int UpdateExtAcCoff(u_int uiLgcCh, DSP_LGC_AI_CFG *plgccfg);

/* 获取发送标志
 * Para:
 *     NONE.
 * Return:
 *     当前标志.
 */
int GetExtSndFlag(void);

/* 设置发送标志
 * Para:
 *     iVal, 设置值.
 * Return:
 *     NONE.
 */
void SetExtSndFlag(int iVal);

/* functions */

/***********************************************************************
* RD_Initialize - 初始化整个实时数据模块
*
* RETURNS:
*		  EP_SUCCESS, 正常返回
*		  EP_BUF_ERR, 内存错误
*
*/
EP_STATUS RD_Initialize(
    const uint8_t *strHwCfgFile				/* 硬件配置文件名称 */
)
{
    int iFd;
    uint8_t aucBuf[100];
    int iItemNum;
    RD_HW_CFG_ITEM cfg;
    int i;
    uint32_t ul;
    int32_t lPos;
    EP_STATUS stsRet;

    stsRet = EP_SUCCESS;

    lPos = -1;

    if ((iFd = open(strHwCfgFile, O_RDONLY, 0)) == ERROR)
    {
        stsRet = EP_CFG_ERR;

        goto ret1;
    }

    lseek(iFd, -4, SEEK_END);

    lPos = -4;

    if (read(iFd, aucBuf, 4) != 4 ||
            aucBuf[0] != 0x14 || aucBuf[1] != 0 || aucBuf[2] != 0 || aucBuf[3] != 0xEB)
        goto reterr;

    lseek(iFd, 0, SEEK_SET);

    lPos = 0;

    if (read(iFd, aucBuf, 8) != 8 ||
            aucBuf[0] != 0x11 || aucBuf[1] != 0 || aucBuf[2] != 0 || aucBuf[3] != 0xEE)
        goto reterr;
    else
    {
        uint16_t ulProtocolVer;
        lPos = 8;
        ulProtocolVer = U8_TO_U16(aucBuf[5], aucBuf[4]);
        if (SI_SysVer_g.unCfgProtocolVer != ulProtocolVer)
        {
            if (ENG_MODE == 0)
            {
                LOG_Dbg_Msg("提示:硬件配置文件和逻辑图文件支持的配置规约版本不一致.\n",
                            0, 0, 0, 0, 0, 0);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Dbg_Msg("Hint: the version of protocl between hardware configuration and logic configuration is different.\n",
                            0, 0, 0, 0, 0, 0);
            }
        }
        SI_SysVer_g.unCfgPrgVer = U8_TO_U16(aucBuf[7], aucBuf[6]);
    }

    i = read(iFd, aucBuf, 4);
    assert (i == 4);
    lPos = 12;

    iItemNum = aucBuf[0];	/* 项目配置个数 */
    SI_SysVer_g.unHdCfgVer = U8_TO_U16(aucBuf[2], aucBuf[1]); /* 硬件配置版本号 */

    cfg.ulBufLen = 0;
    cfg.pucDat = NULL;

    for (i=0; i<iItemNum; i++)
    {
        ul = read(iFd, aucBuf, 5);
        assert (ul == 5);
        lPos += ul;

        cfg.ucType = aucBuf[0];	 /* 类型 */
        ul = BYTES_TO_U32(aucBuf+1);  /* 长度 */
        if (ul>cfg.ulBufLen)
        {
            if (cfg.pucDat)	 /* 清空 */
                EP_free(cfg.pucDat);

            if ((cfg.pucDat = malloc(ul)) != NULL) 	/* 申请空间，并且不预设值 */
                cfg.ulBufLen = ul;
            else
                goto reterr;
        }

        cfg.ulLen = read(iFd, cfg.pucDat, ul);	/* 读数据 */
        assert (cfg.ulLen == ul);
        lPos += ul;

        /* 配置各个项目 */
        if (RD_Deal_Cfg_Item(&cfg) != EP_SUCCESS)
        {
            EP_free(cfg.pucDat);
            goto reterr;
        }
    }

    assert (i == iItemNum);
    if (cfg.ulBufLen)
        EP_free(cfg.pucDat);

#ifndef EDP02_PSR_BUILD
#if defined(EDP_01_02_BUILD)
    if (RD_GetSpiModuleFlag()||RD_GetMbDIUsedFlag())
#endif
        if (SIO_Initialize() != EP_SUCCESS)
        {
            /* 初始化整个SPI<-->SIO模块 */
            logMsg("===Begin init SPI IO!\n",0,0,0,0,0,0);
            return EP_IO_ERR;
        }
#endif

    // /* 为光纵添加 */
    // if (OPT_IO_Initialize(0) != EP_SUCCESS)
    // {
    //     /* 初始化光纵通道1的IO */
    //     return EP_IO_ERR;
    // }

    // /* 为光纵添加 */
    // if (OPT_IO_Initialize(1) != EP_SUCCESS)
    // {
    //     /* 初始化光纵通道2的IO */
    //     return EP_IO_ERR;
    // }

    /* 使用于智能操作箱 */
#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)

    /* 为同杆并架添加 */
    if (POLE_IO_Initialize() != EP_SUCCESS)
    {
        /* 初始化同杆并架的IO */
        return EP_IO_ERR;
    }

    /* 为智能操作箱添加 */
    if (HDL_IO_Initialize() != EP_SUCCESS)
    {
        /* 初始化智能操作箱的IO */
        return EP_IO_ERR;
    }

#endif

    if (RD_Init_AI_Db() != EP_SUCCESS)	/* 初始化AI数据缓冲区 */
        goto reterr;

    if (RD_Init_DI_Db() != EP_SUCCESS)	  /* DI缓冲区初始化 */
    {
        /* RD_Init_DI_Db函数之后的函数不初始化会导致装置重启，而从超级终端看不到重启原因 */
        if (!EP_IS_BOOT_SEL())
        {
            /* 如果是发布版，停止往下运行 */
        }

        goto reterr;
    }
    if (RD_Init_DO_Db() != EP_SUCCESS)			/* DO缓冲区初始化 */
    {
        /* RD_Init_DI_Db函数之后的函数不初始化会导致装置重启，而从超级终端看不到重启原因 */
        if (!EP_IS_BOOT_SEL())
        {
            /* 如果是发布版，停止往下运行 */
        }

        goto reterr;
    }

    if (RD_Init_AO_Db() != EP_SUCCESS)
    {
        /* AO缓冲区初始化，供光纵和励磁冗余机箱使用 */
        goto reterr;
    }

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
    if (DOInitTest()) /* DO测试 */
    {
    }
    else
    {
        if (ENG_MODE == 0)
        {
            LOG_Write(LOG_RUN, "开出自检失败.\n", NULL);
        }
        else if (ENG_MODE == 1)
        {
            LOG_Write(LOG_RUN, "DO Test Fail.\n", NULL);
        }
    }
#endif

#if (defined(EDP03_LOWPROTECT_BUILD))	/* 只有低压保护需要小电流接地 */
    if (RecBufInit() != EP_SUCCESS)
    {
        LOG_Dbg_Msg("Record Buffer Allocation Failed.\n",
                    0, 0, 0, 0, 0, 0);
        assert(FALSE);
    }
#endif

    assert (stsRet == EP_SUCCESS);
    goto ret2;

reterr:
    stsRet = EP_CFG_ERR;

ret2:
    close(iFd);

ret1:
    if (stsRet == EP_CFG_ERR)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                       "硬件配置文件无效.\n",
                       0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                       "Invalid hardware configuration file.\n",
                       0, 0);
        }
    }

    return stsRet;
}

/***********************************************************************
* InitDb - 初始化缓冲区
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*/
EP_STATUS InitDb(void)
{
    EP_STATUS stsRet;

    stsRet=EP_SUCCESS;

    if (RD_Init_AI_Db() != EP_SUCCESS)			/* 初始化AI数据缓冲区 */
        stsRet=EP_CFG_ERR;

    return stsRet;
}

/***********************************************************************
* RD_Deal_Cfg_Item - 分项配置
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Deal_Cfg_Item(
    RD_HW_CFG_ITEM *pcfg		/* 配置指针 */
)
{
    EP_STATUS stsRet;

    assert(pcfg && pcfg->ulBufLen>=pcfg->ulLen);

    switch (pcfg->ucType)
    {
        case 0:
            stsRet=RD_Cfg_Resource(pcfg->pucDat, pcfg->ulLen);
            break;

        case 1:
            stsRet=RD_Cfg_Hw_AI(pcfg->pucDat, pcfg->ulLen);
            break;

        case 2:
            stsRet=RD_Cfg_Lgc_AI(pcfg->pucDat, pcfg->ulLen);
            break;

        case 3:
            stsRet=RD_Cfg_DI(pcfg->pucDat, pcfg->ulLen);
            break;

        case 4:
            stsRet=RD_Cfg_DO(pcfg->pucDat, pcfg->ulLen);
            break;

        case 5:
            stsRet=RD_Cfg_Hw_LED(pcfg->pucDat, pcfg->ulLen);
            break;

        case 6:
            stsRet=RD_Cfg_Sw_LED(pcfg->pucDat, pcfg->ulLen);
            break;

        case 7:
            stsRet=RD_Cfg_Virtual_AI(pcfg->pucDat, pcfg->ulLen);
            break;

        case 8:
            stsRet=RD_Cfg_Msu_AI(pcfg->pucDat, pcfg->ulLen);
            break;

        case 9:
            stsRet=RD_Cfg_AO(pcfg->pucDat, pcfg->ulLen);		/* AO输出 */
            break;

        case 0xa:
            stsRet=RD_Cfg_Lgc_PI(pcfg->pucDat, pcfg->ulLen);		/* 脉冲输入 */
            break;

        case 0xb:
            stsRet=RD_Cfg_Lgc_PO(pcfg->pucDat, pcfg->ulLen);		/* 脉冲输出 */
            break;

        default:
            assert(FALSE);
            stsRet=EP_PARM_ERR;
            break;
    }

    return stsRet;
}

/***********************************************************************
* RD_Cfg_Resource - 装置资源配置项目
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_Resource(
    uint8_t *pucCfg, 		/* 系统资源配置指针 */
    uint32_t ulLen		/* 长度 */
)
{
    RD_PART_INFO *p_part;
    uint8_t *puc;
    uint8_t tempaddress[5];		/* 保存临时地址 */
    int i;
    char TempInfo[256];

    rdinfo_g.unPartNum=U8_TO_U16(pucCfg[1], pucCfg[0]);
    /* logMsg("RD_Cfg_Resource %d  OK !\n",rdinfo_g.unPartNum, 0, 0, 0, 0, 0); */

    assert(rdinfo_g.unPartNum <= MAX_PART_NUM);

    rdinfo_g.bExpand=((pucCfg[2])&0x01) ? TRUE:FALSE;		/* 扩展机箱 */

    /* 3.32规约支持 */
    rdinfo_g.b64KOptCh1=((pucCfg[2])&0x02) ? TRUE:FALSE;
    rdinfo_g.b2MOptCh1=((pucCfg[2])&0x04) ? TRUE:FALSE;
    rdinfo_g.b64KOptCh2=((pucCfg[2])&0x08) ? TRUE:FALSE;
    rdinfo_g.b2MOptCh2=((pucCfg[2])&0x10) ? TRUE:FALSE;

    /* 3.36规约支持 */
    rdinfo_g.bSamePole=((pucCfg[4])&0x01) ? TRUE:FALSE;
    rdinfo_g.bHdlBox=((pucCfg[4])&0x02) ? TRUE:FALSE;
    rdinfo_g.bAssmDev_9_1=((pucCfg[4])&0x04) ? TRUE : FALSE;
    rdinfo_g.bAssmDev_xn=((pucCfg[4])&0x08) ? TRUE : FALSE;
    rdinfo_g.bVirtBox=((pucCfg[4])&0x10) ? TRUE : FALSE;

    SysErrEnableFlag_g |= 0xFF;

    if (rdinfo_g.bExpand)
    {
        SysErrEnableFlag_g |= (1LL<<EV_EXT_COM_ALARM);		/* 扩展机箱 */
    }

#if defined(EXCITE_BUILD)
    SysErrEnableFlag_g |= (1LL<<EV_FPGA_ERR);  /* FPGA */
#endif

    if (rdinfo_g.bHdlBox)
    {
        /* 智能操作箱, GOOSE出错 */
        SysErrEnableFlag_g
        |= (1LL<<EV_REL_GSE_A_NET_HALT) | (1LL<<EV_REL_GSE_B_NET_HALT) | (1LL<<EV_GSE_CONFI_ERR)
           | (1LL<<EV_REL_GSE_C_NET_HALT) | (1LL<<EV_REL_GSE_D_NET_HALT)
           | (1LL<<EV_REL_GSE_E_NET_HALT) | (1LL<<EV_REL_GSE_F_NET_HALT)
           | (1LL<<EV_REL_GSE_G_NET_HALT) | (1LL<<EV_REL_GSE_H_NET_HALT)
           | (1LL<<EV_REL_GSE_I_NET_HALT) | (1LL<<EV_REL_GSE_J_NET_HALT)
           | (1LL<<EV_REL_GSE_K_NET_HALT);
    }

    for (i = 0; i<MAX_SYS_ERR_NUM; i++)
    {
        if (SysErrEnableFlag_g & (1LL << i))
        {
            SysMaxErrNum_g++;
        }
    }

    if(rdinfo_g.bExpand &&
            (rdinfo_g.b64KOptCh1 || rdinfo_g.b2MOptCh1 || rdinfo_g.b64KOptCh2 || rdinfo_g.b2MOptCh2))
    {
        LOG_Dbg_Msg("硬件配置错误，不能同时配置扩展机箱和光纵虚拟机箱!\n", 0, 0, 0, 0, 0, 0);
        assert(FALSE);
    } 	/* 3.36规约支持 */

    if(rdinfo_g.bExpand &&
            (rdinfo_g.bSamePole))
    {
        LOG_Dbg_Msg("硬件配置错误，不能同时配置扩展机箱和同杆并架机箱!\n", 0, 0, 0, 0, 0, 0);
        assert(FALSE);
    }

    if(rdinfo_g.b64KOptCh1 && rdinfo_g.b2MOptCh1)
    {
        LOG_Dbg_Msg("硬件配置错误，不能同时配置64K光纵虚拟机箱1和2M光纵虚拟机箱1!\n", 0, 0, 0, 0, 0, 0);
        assert(FALSE);
    }
    if(rdinfo_g.b64KOptCh2 && rdinfo_g.b2MOptCh2)
    {
        LOG_Dbg_Msg("硬件配置错误，不能同时配置64K光纵虚拟机箱2和2M光纵虚拟机箱2\n", 0, 0, 0, 0, 0, 0);
        assert(FALSE);
    }
    if(!(rdinfo_g.b64KOptCh1) && (rdinfo_g.b64KOptCh2))
    {

        LOG_Dbg_Msg("硬件配置错误，不能配置64K光纵虚拟机箱2而未配置64K光纵虚拟机箱1!\n",0,0,0,0,0,0);
        assert(FALSE);

    }
    if(!(rdinfo_g.b2MOptCh1) && (rdinfo_g.b2MOptCh2))
    {
        LOG_Dbg_Msg("硬件配置错误，不能配置2M光纵虚拟机箱2而未配置2M光纵虚拟机箱1!\n",0,0,0,0,0,0);
        assert(FALSE);

    }

    rdinfo_g.ucEnvType=pucCfg[3];

    /* Get power system freq. */
    switch (pucCfg[3])
    {
        case 0x00:
            uiPwrFreq_g=50;
            bOneAmpSys_g=FALSE;
            bFiveAmpSys_g=TRUE;
            EnviromentType.bOneAmpSys=FALSE;
            EnviromentType.bFiveAmpSys=TRUE;
            EnviromentType.bFreq50Sys=TRUE;
            EnviromentType.bFreq60Sys=FALSE;
            break;

        case 0x01:
            uiPwrFreq_g=50;
            bOneAmpSys_g=TRUE;
            bFiveAmpSys_g=FALSE;
            EnviromentType.bOneAmpSys=TRUE;
            EnviromentType.bFiveAmpSys=FALSE;
            EnviromentType.bFreq50Sys=TRUE;
            EnviromentType.bFreq60Sys=FALSE;
            break;

        case 0x02:
            uiPwrFreq_g=60;
            bOneAmpSys_g=FALSE;
            bFiveAmpSys_g=TRUE;
            EnviromentType.bOneAmpSys=FALSE;
            EnviromentType.bFiveAmpSys=TRUE;
            EnviromentType.bFreq50Sys=FALSE;
            EnviromentType.bFreq60Sys=TRUE;
            break;

        case 0x03:
            uiPwrFreq_g=60;
            bOneAmpSys_g=TRUE;
            bFiveAmpSys_g=FALSE;
            EnviromentType.bOneAmpSys=TRUE;
            EnviromentType.bFiveAmpSys=FALSE;
            EnviromentType.bFreq50Sys=FALSE;
            EnviromentType.bFreq60Sys=TRUE;
            break;

        case 0x80:
            uiPwrFreq_g=50;
            bOneAmpSys_g=TRUE;
            bFiveAmpSys_g=TRUE;
            EnviromentType.bOneAmpSys=TRUE;
            EnviromentType.bFiveAmpSys=TRUE;
            EnviromentType.bFreq50Sys=TRUE;
            EnviromentType.bFreq60Sys=FALSE;
            break;

        case 0x81:
            uiPwrFreq_g=60;
            bOneAmpSys_g=TRUE;
            bFiveAmpSys_g=TRUE;
            EnviromentType.bOneAmpSys=TRUE;
            EnviromentType.bFiveAmpSys=TRUE;
            EnviromentType.bFreq50Sys=FALSE;
            EnviromentType.bFreq60Sys=TRUE;
            break;

        case 0xC0:
            EnviromentType.bOneAmpSys=FALSE;
            EnviromentType.bFiveAmpSys=TRUE;
            EnviromentType.bFreq50Sys=TRUE;
            EnviromentType.bFreq60Sys=TRUE;
            break;

        case 0xC1:
            EnviromentType.bOneAmpSys=TRUE;
            EnviromentType.bFiveAmpSys=FALSE;
            EnviromentType.bFreq50Sys=TRUE;
            EnviromentType.bFreq60Sys=TRUE;
            break;

        case 0xC2:
            EnviromentType.bOneAmpSys=TRUE;
            EnviromentType.bFiveAmpSys=TRUE;
            EnviromentType.bFreq50Sys=TRUE;
            EnviromentType.bFreq60Sys=TRUE;
            break;

        default:
            assert(FALSE);
            break;
    }

    uiPwrFreq_g=EP_GetPwrFreq();
    if(uiPwrFreq_g == 0)
    {
        /* 默认为50Hz */
        uiPwrFreq_g=50;
    }

    assert(uiPwrFreq_g==50 || uiPwrFreq_g==60);

    uiDioRate_g=RD_SIO_RATE;

    bDoubleCPUFlag_g=(pucCfg[5]&0x01)?FALSE:TRUE;

    if(pucCfg[5]&0x02)
    {
        ucCPUPos_g=0x01;
        EP_Set02CPU();
    }
    else
    {
        ucCPUPos_g=0x00;
    }

    /*过程层统一配置解析*/
    if(EDP_InitProcessFileCfg() != 0)
    {
        logMsg("##########过程层统一配置初始化失败\n",0,0,0,0,0,0);
        LOG_Write(LOG_KERNEL, "过程层统一配置初始化失败\n", NULL);
    }
    else
    {
        logMsg("##########过程层统一配置初始化成功\n",0,0,0,0,0,0);
        LOG_Write(LOG_KERNEL, "过程层统一配置初始化成功\n", NULL);
    }

    /* 装置应用类型 */
    uiAppType_g=pucCfg[6];

    if (uiAppType_g >= APP_INVALID)
    {
        uiAppType_g = APP_COM;
    }

    sprintf(TempInfo, "装置应用类型: %s\n", aAppMapArr[uiAppType_g].aucAppID);
    LOG_Write(LOG_KERNEL, TempInfo, NULL);

    /* 根据应用设置周波点数
     * 稳控和励磁单独处理
     */
    if ((uiAppType_g == APP_STAB_CONTROL)
            || (uiAppType_g == APP_EXCITE))
    {
        g_RdBufCyc = RD_BUF_CYC_LONG;
    }
    else if (uiAppType_g == APP_PROT_MEA_MERGE)
    {
        /* 测控应用单独处理 */
        g_RdBufCyc = RD_BUF_CYC_SHORT;
    }
    else
    {
        g_RdBufCyc = RD_BUF_CYC;
    }

    puc=pucCfg+8;
    for (p_part=apartinf_g; p_part<apartinf_g+rdinfo_g.unPartNum; p_part++)
    {
        uint8_t ucTotalLength;
        uint8_t ucaucIdLength;

        /* assert(U8_TO_U16(puc[1], puc[0])==16+puc[2]); */

        ucTotalLength = U8_TO_U16(puc[1], puc[0]);
        ucaucIdLength = puc[2];

        EP_ID_Copy(p_part->aucId, puc+3, puc[2]);
        puc+=puc[2]+3;

        p_part->ucType=*puc++;
        p_part->ucPosition=*puc++;
        p_part->aucHwAddr[0]=*puc++;
        p_part->aucHwAddr[1]=*puc++;
        p_part->aucHwAddr[2]=*puc++;
        p_part->aucHwAddr[3]=*puc++;

        if (p_part->ucPosition == RD_VT_BOX)
        {
            assert (FALSE);
        }
        else
        {
            assert (ucTotalLength == 16+ucaucIdLength);
        }

        puc+=3;

        p_part->ucAoNum=*puc++;
        p_part->ucAiNum=*puc++;
        p_part->ucDiNum=*puc++;
        p_part->ucDoNum=*puc++;
        p_part->ulDIModCurVaule=0;
        p_part->servertype = 0;

        /* logMsg("p_part->ucType = %d p_part->aucHwAddr[0] = %d\n",p_part->ucType,p_part->aucHwAddr[0],0,0,0,0); */
        if (p_part->ucType == AI_MODULE)
        {
            dectochar(p_part->aucHwAddr[0]+p_part->aucHwAddr[1]*256+p_part->aucHwAddr[2]*256*256+p_part->aucHwAddr[3]*256*256*256,
                      tempaddress, 6);
            LOG_Dbg_Msg("%d %d %d %d %d %d\n",
                        tempaddress[0],
                        tempaddress[1],
                        tempaddress[2],
                        tempaddress[3],
                        tempaddress[4],
                        tempaddress[5]);

            if (tempaddress[2] == RD_AI_DC)
            {
                LOG_Dbg_Msg("直流模件.\n", 0, 0, 0, 0, 0, 0);
                p_part->aucHwAddr[0] = chartodec(tempaddress, 2);		/* 变换为实际地址 */

                p_part->ucAiModType = RD_AI_DC;
                p_part->ucType = COM_MODULE;		/* 转换为通用模件 */
                p_part->servertype |= CHN_ADJ_SERVER;
            }
            else if ((p_part->aucHwAddr[0] == CPU_04_ADDR)
                     || (p_part->aucHwAddr[0] == REDUN_CPU_04_ADDR))
            {
                LOG_Dbg_Msg("交流模件.\n", 0, 0, 0, 0, 0, 0);
                p_part->ucAiModType = RD_AI_AC;
            }

            LOG_Dbg_Msg("地址为: %d\n",
                        p_part->aucHwAddr[0], 0, 0, 0, 0, 0);

            assert((p_part->aucHwAddr[0] == CPU_04_ADDR)
                   || (p_part->aucHwAddr[0] == REDUN_CPU_04_ADDR)
                   || (p_part->aucHwAddr[0]<MAX_MOD_NUM));		/* 增加了冗余机箱配置 */

            /* p_part->unAiSmplRate=U8_TO_U16(puc[1], puc[0]); */
            p_part->unAiPts=U8_TO_U16(puc[1], puc[0]);			/* 周波采样点数 */
            p_part->unAiSmplRate=p_part->unAiPts*uiPwrFreq_g;					/* 采样频率 */
#ifndef NO_DEBUG
            LOG_Dbg_Msg("Sampling Rate: %d\n", p_part->unAiSmplRate, 0, 0, 0, 0, 0);
#endif
            // p_part->unAiSmplRate = 1200; 		/* 测试，采样点数 */
            if (!uiAiRate_g)
            {
                uiAiPts_g=p_part->unAiPts;
                uiAiRate_g=p_part->unAiSmplRate;		/* 采样率，单位为Hz */
                assert(uiAiRate_g>=600 && uiAiRate_g<=20000);

                rdinfo_g.uiSmplPeriod=
                    (1000000L+uiAiRate_g/2)/uiAiRate_g; 			/* 采样周期，用us表示 */

                rdinfo_g.uiBufPts=
                    ((uint32_t)uiAiRate_g/uiPwrFreq_g)*g_RdBufCyc; 				/* 保存采样点数 */

                rdinfo_g.uiRealBuf=
                    ((uint32_t)uiAiRate_g/uiPwrFreq_g)*(g_RdBufCyc+1); 			/* 实际缓冲区长度，用点数来表示 */
            }
            else
                assert(uiAiRate_g==p_part->unAiSmplRate);
        }

        if(((p_part->ucType == 0x12) || (p_part->ucType == 0x13)) && (p_part->ucPosition == 0))
        {
            /* 开入模件或开入开出模件，只是主机箱这么处理 */
            /* logMsg("DI p_part->aucHwAddr[0]=%d\n", p_part->aucHwAddr[0], 0, 0, 0, 0, 0); */
            if(p_part->aucHwAddr[0] == CPU_04_ADDR)
            {
                /* 主CPU上 */
                p_part->aucHwAddr[0]=DIADDRONCPU;		/* 0~15由IO子模件占用 */
                RD_SetMbDIUsed();
            }
        }
#if defined(EDP_01_02_BUILD)
        if(((p_part->ucType == DO_MODULE) || (p_part->ucType == DI_MODULE)  ||
                (p_part->ucType == DIO_MODULE) || (p_part->ucType == CKDIO_MODULE) || (p_part->ucType == COM_MODULE))
                && (p_part->ucPosition == 0))
        {
            RD_SetSpiModuleUsed();
        }
#endif
        puc+=2;
    }

#if defined(EDP_01_02_BUILD)
    EDP_Init_Net();
#endif

    if(rdinfo_g.bSamePole || rdinfo_g.bHdlBox)
    {
        /* 若是初始化了同杆并架，或智能操作箱，则需要初始化61850的GOOSE功能 2007-05-27日张云  否则不用初始化goose*/

        /* 必须在硬件配置真正初始化之前，进行goose模块初始化，张云 */
#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)
        Set_Goose_Platform_Support_Func_Code(PLATFORM_FUNC_CODE_E02_TRANSMIT);

        if (goose_cfg_start() != TRUE)
        {
            LOG_Dbg_Msg("61850配置未成功初始化!\n", 0, 0, 0, 0, 0, 0);
            if(ENG_MODE==1)
                LOG_Write(LOG_KERNEL, "61850 configuration initialized failure!\n", NULL);
            else if(ENG_MODE==0)
                LOG_Write(LOG_KERNEL, "61850配置初始化失败!\n", NULL);

            /* 置标志,引导后续初始化 */
            bGooseCfgFlag = FALSE;
        }
        else
        {
            bGooseCfgFlag = TRUE;
        }
#endif

    }

    if (puc-pucCfg != ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
        return EP_SUCCESS;
}

/* 初始化GOOSE功能
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void RD_InitGoose(void)
{
//     uint8_t optWattSrc[6] = {0x01, 0x0C, 0xCD, 0x01, 0xFF, 0xFF};  /* 光功率报文地址 */

//     /* 配置初始化失败则不执行以下操作
//      */
//     if (!bGooseCfgFlag)
//     {
//         return;
//     }

//     gmrp_initialize(gmrp_send_packet,MacAddrGet);

//     if(rdinfo_g.bSamePole || rdinfo_g.bHdlBox)
//     {
//         /* 若是初始化了同杆并架，或智能操作箱，则需要初始化61850的GOOSE功能 2007-05-27日张云  否则不用初始化goose*/

//         /* MAC地址进入HASH表, 且注册0x88b8类型报文处理函数
//          */
//         mCastAddrAdd(GENERAL_NET_B, optWattSrc);  /* 光功率报文地址处理 */
//         eth_register_dissector(GENERAL_NET_B, 0x88b8, (eth_cb_func)SubQueueCallback_goose);

//         /* 记录版本 */
//         LOG_ExtraItemWrite("GOOSE库版本", VERSION);
//         /* 必须在硬件配置真正初始化之前，进行goose模块初始化，张云 */
// #if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)
//         if(GO_Init61850BfRelay() != EP_SUCCESS)
//         {
//             LOG_Dbg_Msg("61850模块未成功初始化!\n", 0, 0, 0, 0, 0, 0);
//             if(ENG_MODE==1)
//                 LOG_Write(LOG_KERNEL, "61850 Module initialized failure!\n", NULL);
//             else if(ENG_MODE==0)
//                 LOG_Write(LOG_KERNEL, "61850模块初始化失败!\n", NULL);
//         }
// #endif

//     }
}

/***********************************************************************
* RD_Cfg_Hw_AI - 模拟输入硬件通道配置项目
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_Hw_AI(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
)
{
    RD_PART_INFO *p_part;
    RD_HW_AI_CH *pch;
    int iChCfgLen;
    int i;
    uint8_t *puc;
    uint8_t aucModId[MAX_ID_LEN+1];
    int iFd;
    STATUS vxsts;
    BOOL bNeedCfg;
    EP_STATUS sts;

    assert(!plgcaich_g);		    /* Hardware AI must be configed before logic AI. */

    iHwAiChNum_g=U8_TO_U16(pucCfg[1], pucCfg[0]);
    if ((phwaich_g=calloc(iHwAiChNum_g, sizeof(*phwaich_g)))==NULL)
        return EP_BUF_ERR;

    puc=pucCfg+6;
    for (i=0; i<iHwAiChNum_g; i++)
    {
        iChCfgLen=U8_TO_U16(puc[1], puc[0]);

        pch=phwaich_g+puc[7+puc[2]];

        /* Never used before. */
        assert(pch->aucId[0]=='\0');

        iChCfgLen-=puc[2];
        EP_ID_Copy(pch->aucId, puc+3, puc[2]);
        puc+=3+puc[2];

        pch->aucABRV[0]=*puc++;
        pch->aucABRV[1]=*puc++;
        pch->aucABRV[2]=*puc++;
        pch->aucABRV[3]=*puc++;

        puc++;

        pch->fMaxVal=BYTES_TO_FLT(puc);
        puc+=4;

        pch->fMinVal=BYTES_TO_FLT(puc);
        puc+=4;

        pch->FactorSetModWord = *puc++;		 /* 配置方式控制字 */
        if((pch->FactorSetModWord)&0x01)
        {
            iChCfgLen-=puc[0];
            iChCfgLen--;		/* 与以前规约不一致*/
            EP_ID_Copy(pch->MaxValueDingzhiTagBase, puc+1, puc[0]);			/* 最大值内部定值逻辑标志字符串 */
            puc+=1+puc[0];
        }
        if((pch->FactorSetModWord)&0x02)
        {
            iChCfgLen-=puc[0];
            iChCfgLen--;
            EP_ID_Copy(pch->MinValueDingzhiTagBase, puc+1, puc[0]);				/* 最小值内部定值逻辑标志字符串 */
            puc+=1+puc[0];
        }
        if((pch->FactorSetModWord)&0x04)
        {
            iChCfgLen-=puc[0];
            iChCfgLen--;
            EP_ID_Copy(pch->ScalefactorDingzhiTagBase, puc+1, puc[0]);					/* 最小值内部定值逻辑标志字符串 */
            puc+=1+puc[0];
        }

        pch->bSetAo = *puc++;
        if (pch->bSetAo)
        {
            iChCfgLen -= puc[0];
            iChCfgLen--;
            EP_ID_Copy(pch->aucAoLogicId, puc+1, puc[0]);					/* AO逻辑标志字符串 */
            puc += 1+puc[0];
        }

        puc += 1;		/* 保留 */
        pch->ucMmiShow=*puc++;

        puc+=1;

        iChCfgLen-=puc[0];
        EP_ID_Copy(aucModId, puc+1, puc[0]);
        puc+=1+puc[0];

        for (p_part=apartinf_g; p_part<apartinf_g+MAX_PART_NUM; p_part++)
        {
            /* logMsg("p_part->ucType = %d p_part->aucHwAddr[0] = %d\n",p_part->ucType,p_part->aucHwAddr[0],0,0,0,0); */
            if (!strcmp(p_part->aucId, aucModId))
            {
                /* logMsg("p_part->aucId = %s p_part->ucType = %d p_part->aucHwAddr[0] = %d\n",p_part->aucId, p_part->ucType,p_part->aucHwAddr[0],0,0,0); */
                assert(((p_part->ucType == AI_MODULE)
                        && ((p_part->aucHwAddr[0] == CPU_04_ADDR)
                            || (p_part->aucHwAddr[0] == REDUN_CPU_04_ADDR)))  /* for excite */
                       || ((p_part->ucType == COM_MODULE) || (p_part->aucHwAddr[0]<MAX_MOD_NUM)));
                break;
            }
        }
        assert(p_part<apartinf_g+MAX_PART_NUM);

        pch->p_part=p_part;

        if (p_part->ucType == COM_MODULE)
        {
            /* channel server type. */
            pch->servertype = p_part->servertype;
        }

        /* 数字化采样值从通信口传入,初始时无通信连接 */
        pch->pSmv = NULL;  /* 所属SMV配置通道地址 */
        pch->ucMstPortNum = 0xFF;  /* 本级CC板端口 */
        pch->ucSlvPortNum = 0xFF;     /* 前级CC板端口 */

        if (p_part->ucPosition==0)
        {
#ifdef EDP01_CA_OPT_BUILD			/* EDP01平台C-A版光CT获取数据，屏蔽本机采样 */
            assert(FALSE);
#endif
            pch->paimod=&aimodDsp_g;
        }
        else if (p_part->ucPosition==1)
            pch->paimod=&aimodExt_g;
        else if(p_part->ucPosition == 8)		/* for excite */
            pch->paimod=&aimodRedun_g;
        else if((p_part->ucPosition == RD_64K_OPT1_BOX)  		/* 为光纵修改，2006-2-8 */
                ||(p_part->ucPosition == RD_2M_OPT1_BOX))
            pch->paimod=&aimodOpt_g[0];
        else if((p_part->ucPosition == RD_64K_OPT2_BOX)
                ||(p_part->ucPosition==RD_2M_OPT2_BOX))
            pch->paimod=&aimodOpt_g[1];
        else if((p_part->ucPosition == RD_SAME_POLE_BOX))		/* 为同杆并架和操作箱修改 2007-3-20日 */
            pch->paimod=&aimodPole_g;
        else  if((p_part->ucPosition==RD_HDL_BOX))
            pch->paimod=&aimodHdl_g;
        else if(p_part->ucPosition == RD_VT_BOX)		/* 虚拟机箱 */
        {
            pch->paimod=&aimodVtBox_g[p_part->ucVtBoxPos];		/* 模拟硬件输入通道所位于的虚拟机箱位置 */
            pch->ucVtBoxPos=p_part->ucVtBoxPos;		/* 硬件模拟通道所位于的虚拟机箱位置 */
        }
        else
            assert(FALSE);

        pch->ucModCh=*puc++;

        pch->fGain=1.0;                 /* If no gain file, use this. */
        pch->fExcCoff=0.0;				/* If no gain file, use this */
        pch->fRatio=1.0;                /* If no CT ratio file, use this. */
        pch->fCoff=BYTES_TO_FLT(puc);
        pch->fOriginCoff = pch->fCoff;		/* 原始 */
        pch->fSetCoff=pch->fCoff;		/* 设定系数 */
        puc+=4;

        pch->ucUnit=*puc++;

        /* 实数式电流
         */
        if (pch->ucUnit == 0x8)
        {
            pch->uFstRatedVal = DEFAULT_CURRENT_FST_RATED_VAL;
            pch->uSecRatedVal = DEFAULT_CURRENT_SEC_RATED_VAL;
            pch->fThreshold = THD_CURRENT_1A;
        }
        else if (pch->ucUnit == 0x14)
        {
            /* 实数式电压 */
            pch->uFstRatedVal = DEFAULT_VOLTAGE_FST_RATED_VAL;
            pch->uSecRatedVal = DEFAULT_VOLTAGE_SEC_RATED_VAL;
            pch->fThreshold = THD_VOLT;
        }
        else
        {
            /*
             * 允许其它类型存在
             */
            pch->uFstRatedVal = 1000;
            pch->uSecRatedVal = 100; /* 缺省额定值 */
        }

        /* 新的实现 */
        if(puc[0]==1)
        {
            pch->bSetOptAo=TRUE;
            puc++;
            iChCfgLen=iChCfgLen-1-puc[0];                           /* 设置光纵标志，为光纵修改，2006-2-8 */
            EP_ID_Copy(pch->aucAoId, puc+1, puc[0]);
            puc+=1+puc[0];
        }
        else  if(puc[0]==0)
        {
            pch->bSetOptAo=FALSE;
            puc++;
        }
        else
        {
            assert(FALSE);
        }

        pch->iIndexSn = -1; /* 初始索引定值页序 */

        /* logMsg("puc-pucCfg = %d iChCfgLen= %d\n", puc-pucCfg,iChCfgLen,0,0,0,0); */
        /* logMsg("iChCfgLen = %d\n", iChCfgLen, 0, 0, 0, 0, 0); */
        assert(iChCfgLen==27);		/* 衰减增益补偿不配定值时为27 DY 12/19/2006 */
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }

    bNeedCfg=FALSE;
    if ((i=FT_Rd_Sys_INI("[SYSTEM]", "NeedHdCof", aucModId, 30))==1)
    {
        for (puc=aucModId; *puc; puc++)
            *puc=tolower(*puc);

        if (!strcmp(aucModId, "1") ||
                !strcmp(aucModId, "true") || !strcmp(aucModId, "yes"))
            bNeedCfg=TRUE;
    }

    if ((iFd=open(EP_AI_GAIN_FILE, O_RDONLY, 0))!=ERROR)
    {
        sts=RD_Cfg_Hw_AI_Gain(iFd);

        vxsts=close(iFd);
        assert(vxsts==OK);

        if (sts==EP_SUCCESS)
        {
            if (!bNeedCfg)
            {
                i=FT_Wr_Sys_INI("[SYSTEM]", "NeedHdCof", "1");
                assert(i!=EP_ERROR);
            }
        }
        else if(bNeedCfg)
        {
            /* 防止程序中断 */

            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                           "通道增益系数文件无效\n",
                           0, 0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                           "Hardware AI channel gain coefficient file is invalid\n",
                           0, 0);
            }
            SI_New_AI_Gain_Set();		/* 生成新的增益文件 */

            /* return EP_FILE_ERR; */
        }
        else
        {
            SI_New_AI_Gain_Set();		/* 生成新的增益文件 */
        }
    }
    else if (bNeedCfg)
    {
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                       "通道增益系数文件无效\n",
                       0, 0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                       "Unable to open hardware AI gain coefficient file\n",
                       0, 0);
        }

        SI_New_AI_Gain_Set();		/* 生成新的增益文件 */

    }

    bNeedCfg=FALSE;
    if ((i=FT_Rd_Sys_INI("[SYSTEM]", "NeedCtRatio", aucModId, 30))==1)
    {
        for (puc=aucModId; *puc; puc++)
            *puc=tolower(*puc);

        if (!strcmp(aucModId, "1") ||
                !strcmp(aucModId, "true") || !strcmp(aucModId, "yes"))
            bNeedCfg=TRUE;
    }
    else if(i==0)
    {
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "因[SYSTEM] NeedCtRatio值读取失败，创建新的系统INI文件.\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Because of failing ro read [SYSTEM] NeedCtRatio, create new ini file.\n", NULL);
        }

        FT_New_SYS_INI_File();
    }
    if ((iFd=open(EP_CT_RATIO_FILE, O_RDONLY, 0))!=ERROR)
    {
        if(RD_Get_CT_Ratio_File_Len() == FT_Get_Len(EP_CT_RATIO_FILE))
        {
            sts=RD_Cfg_CT_Ratio(iFd);
        }
        else
        {
            sts = EP_FILE_ERR;
        }

        vxsts=close(iFd);
        assert(vxsts==OK);

        if (sts==EP_SUCCESS)
        {
            if (!bNeedCfg)
            {
                i=FT_Wr_Sys_INI("[SYSTEM]", "NeedCtRatio", "1");
                assert(i!=EP_ERROR);
            }
        }
        else if(bNeedCfg)
        {
            /* 防止程序中断 */
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                           "CT变比系数文件无效\n",
                           0, 0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                           "Invalid CT ratio coefficient file\n",
                           0, 0);
            }

            RD_New_CT_Ratio();  /* 生成新的CT系数文件 */

        }
        else
        {
            RD_New_CT_Ratio();  /* 生成新的CT系数文件 */
        }
    }
    else if (bNeedCfg)
    {
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                       "CT变比系数文件无效\n",
                       0, 0);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                       "Unable to open CT ratio coefficient file\n",
                       0, 0);
        }

        RD_New_CT_Ratio();  /* 生成新的CT系数文件 */
    }

    return EP_SUCCESS;
}

/***********************************************************************
* RD_Cfg_Lgc_AI - 模拟输入逻辑通道配置项目
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_Lgc_AI(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
)
{
    RD_LGC_AI_CH *pch;
    RD_VT_AI_CH *pvtch;
    int iChCfgLen;
    uint8_t *puc;
    EP_STATUS stsRet;

    stsRet=EP_SUCCESS;

    iLgcAiChNum_g=U8_TO_U16(pucCfg[1], pucCfg[0])+iVtAiChNum_g;			/* Virtual AI belongs to logic AI. */
    if ((plgcaich_g=calloc(iLgcAiChNum_g, sizeof(*plgcaich_g)))==NULL)
        return EP_BUF_ERR;

    if (iVtAiChNum_g)
    {
        /* Copy virtual AI config to the tail logic AI. */
        assert(pvtaich_g);

        for (pvtch=pvtaich_g, pch=plgcaich_g+iLgcAiChNum_g-iVtAiChNum_g;
                pvtch<pvtaich_g+iVtAiChNum_g; pvtch++, pch++)
        {
            strcpy(pch->aucId, pvtch->aucId);

            pch->unLgcSN=pvtch->unLgcSN;

            pch->ucUnit=pvtch->ucUnit;

            pch->bRec=pvtch->bRec;
            if (pch->bRec)
                strcpy(pch->aucRecId, pvtch->aucRecId);

            pch->bFlag=pvtch->bFlag;
            if (pch->bFlag)
                strcpy(pch->aucFlagId, pvtch->aucFlagId);

            pch->bMea=pvtch->bMea;
            if (pch->bMea)
                strcpy(pch->aucMeaId, pvtch->aucMeaId);
        }

#if FREE_INIT_MEM
        EP_free(pvtaich_g);
#endif
    }

    puc=pucCfg+6;                       /* Skip AICount and reserved 4 bytes. */

    for (pch=plgcaich_g; pch<plgcaich_g+iLgcAiChNum_g-iVtAiChNum_g; pch++)
    {
        iChCfgLen=U8_TO_U16(puc[1], puc[0]);

        iChCfgLen-=puc[2];
        EP_ID_Copy(pch->aucId, puc+3, puc[2]);
        puc+=3+puc[2];

        pch->unLgcSN=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        puc+=3;                         /* Skip reserved 4 bytes. */

        if (puc[0] == 1)
        {
            pch->bSetAo = TRUE;
            puc++;
            iChCfgLen=iChCfgLen-1-puc[0];             /* 设置AO标志 */
            EP_ID_Copy(pch->aucAoId, puc+1, puc[0]);
            puc += 1+puc[0];
        }
        else if (puc[0] == 0)
        {
            pch->bSetAo = FALSE;
            puc++;
        }
        else
        {
            assert (FALSE);
        }

        assert(puc[0]<iHwAiChNum_g && phwaich_g);
        pch->phwai=phwaich_g+*puc++;

#ifndef NO_DEBUG
        /* LOG_Dbg_Msg("DSP Channel: %d %d\n",*(puc-1),pch->phwai->ucModCh,0,0,0,0); */
#endif

        pch->ucFiltTp=*puc++;

#ifdef EDP01_CA_OPT_BUILD		/* EDP01平台C-A版本，使用光CT，屏蔽本机采样 */
        assert(pch->ucFiltTp==0);
#endif

        if(((pch->phwai->paimod == &(aimodOpt_g[0])) || (pch->phwai->paimod == &(aimodOpt_g[1])))
                &&(pch->ucFiltTp != 0))
        {
            /* 光纵除了原始滤波算法，不能配置其他算法 */
            LOG_Dbg_Msg("硬件配置错误，光纵AI通道不能配置非瞬时值滤波算法\n", 0, 0, 0, 0, 0, 0);
            assert(FALSE);
        }

        if((pch->phwai->paimod == &(aimodPole_g))
                &&(pch->ucFiltTp != 0))
        {
            /* 同杆并架除了原始滤波算法，不能配置其他算法  2007-3-20日 */
            LOG_Dbg_Msg("硬件配置错误，同杆并架对侧机箱AI通道不能配置非瞬时值滤波算法\n", 0, 0, 0, 0, 0, 0);
            assert(FALSE);
        }

        if((pch->phwai->paimod == &(aimodHdl_g))
                &&(pch->ucFiltTp != 0))
        {
            /* 若是智能操作箱，目前不支持AI采集  2007-3-20日 */
            LOG_Dbg_Msg("硬件配置错误，智能操作箱的AI通道不能配置非瞬时值滤波算法\n", 0, 0, 0, 0, 0, 0);
            assert(FALSE);
        }


        pch->ucUnit=*puc++;

        pch->bRec=*puc++;
        if (pch->bRec)
        {
            //201-1-11  ZY 开放光差AI通道录波

            if(pch->phwai->paimod == &(aimodPole_g))
            {
                /*同杆并架不能配置录波  */
                LOG_Dbg_Msg("硬件配置错误，同杆并架AI通道不能配置录波!\n", 0, 0, 0, 0, 0, 0);
                assert(FALSE);
            }

            iChCfgLen-=1+puc[0];
            EP_ID_Copy(pch->aucRecId, puc+1, puc[0]);
            puc+=1+puc[0];
        }

        pch->bFlag=*puc++;
        if (pch->bFlag)
        {
            if((pch->phwai->paimod == &(aimodOpt_g[0])) || (pch->phwai->paimod == &(aimodOpt_g[1])))
            {
                /* 光纵不能配置标志 */
                LOG_Dbg_Msg("硬件配置错误，光纵AI通道不能配置标志!\n", 0, 0, 0, 0, 0, 0);
                assert(FALSE);
            }

            iChCfgLen-=1+puc[0];
            EP_ID_Copy(pch->aucFlagId, puc+1, puc[0]);
            puc+=1+puc[0];
        }

        pch->bMea=*puc++;
        if (pch->bMea)
        {

            if((pch->phwai->paimod == &(aimodOpt_g[0])) || (pch->phwai->paimod == &(aimodOpt_g[1])))
            {
                /*光纵不能配置遥测  */
                LOG_Dbg_Msg("硬件配置错误，光纵AI通道不能配置遥测!\n", 0, 0, 0, 0, 0, 0);
                assert(FALSE);
            }

            iChCfgLen-=1+puc[0];
            EP_ID_Copy(pch->aucMeaId, puc+1, puc[0]);
            puc+=1+puc[0];
        }

        assert(iChCfgLen==13);
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        stsRet=EP_PARM_ERR;
    }

    return stsRet;
}

/***********************************************************************
* RD_Cfg_DI - 开入配置项目
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_DI(
    uint8_t *pucCfg,		/* 配置指针*/
    uint32_t ulLen		/* 长度 */
)
{
    RD_LGC_DI_CH *pch;
    RD_PART_INFO *p_part;
    int iChCfgLen;
    uint8_t *puc;
    uint8_t aucModId[MAX_ID_LEN+1];

    iLgcDiChNum_g=U8_TO_U16(pucCfg[1], pucCfg[0]);
    if ((plgcdich_g=calloc(iLgcDiChNum_g, sizeof(*plgcdich_g)))==NULL)
        return EP_BUF_ERR;


    puc=pucCfg+6; 		/* Skip DICount and reserved 4 bytes. */

    for (pch=plgcdich_g; pch<plgcdich_g+iLgcDiChNum_g; pch++)
    {
        iChCfgLen=U8_TO_U16(puc[1], puc[0]);

        iChCfgLen-=puc[2];
        EP_ID_Copy(pch->aucId, puc+3, puc[2]);
        puc+=3+puc[2];

        pch->unLgcSN=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        iChCfgLen-=puc[0];
        EP_ID_Copy(pch->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        pch->aucABRV[0]=*puc++;		/* 内部简称 */
        pch->aucABRV[1]=*puc++;
        pch->aucABRV[2]=*puc++;
        pch->aucABRV[3]=*puc++;
#if 0
        if (strncmp(LINE_CUST_DI1_FLT, pch->aucId, strlen(LINE_CUST_DI1_FLT)) == 0
                || strncmp(LINE_CUST_DI2_FLT, pch->aucId, strlen(LINE_CUST_DI2_FLT)) == 0
                || strncmp(LINE_CUST_DI3_FLT, pch->aucId, strlen(LINE_CUST_DI3_FLT)) == 0
                || strncmp(LINE_CUST_DI4_FLT, pch->aucId, strlen(LINE_CUST_DI4_FLT)) == 0
                || strncmp(LINE_CUST_DI5_FLT, pch->aucId, strlen(LINE_CUST_DI5_FLT)) == 0
                || strncmp(LINE_CUST_DI6_FLT, pch->aucId, strlen(LINE_CUST_DI6_FLT)) == 0
                || strncmp(LINE_CUST_DI7_FLT, pch->aucId, strlen(LINE_CUST_DI7_FLT)) == 0
                || strncmp(LINE_CUST_DI8_FLT, pch->aucId, strlen(LINE_CUST_DI8_FLT)) == 0
                || strncmp(LINE_CUST_DI9_FLT, pch->aucId, strlen(LINE_CUST_DI9_FLT)) == 0
                || strncmp(LINE_CUST_DI10_FLT, pch->aucId, strlen(LINE_CUST_DI10_FLT)) == 0
                || strncmp(LINE_CUST_DI11_FLT, pch->aucId, strlen(LINE_CUST_DI11_FLT)) == 0
                || strncmp(LINE_CUST_DI12_FLT, pch->aucId, strlen(LINE_CUST_DI12_FLT)) == 0
                || strncmp(LINE_CUST_DI13_FLT, pch->aucId, strlen(LINE_CUST_DI13_FLT)) == 0
                || strncmp(LINE_CUST_DI14_FLT, pch->aucId, strlen(LINE_CUST_DI14_FLT)) == 0
                || strncmp(LINE_CUST_DI15_FLT, pch->aucId, strlen(LINE_CUST_DI15_FLT)) == 0
                || strncmp(LINE_CUST_DI16_FLT, pch->aucId, strlen(LINE_CUST_DI16_FLT)) == 0
                || strncmp(LINE_CUST_DI17_FLT, pch->aucId, strlen(LINE_CUST_DI17_FLT)) == 0
                || strncmp(LINE_CUST_DI18_FLT, pch->aucId, strlen(LINE_CUST_DI18_FLT)) == 0
                || strncmp(LINE_CUST_DI19_FLT, pch->aucId, strlen(LINE_CUST_DI19_FLT)) == 0
                || strncmp(LINE_CUST_DI20_FLT, pch->aucId, strlen(LINE_CUST_DI20_FLT)) == 0
                || strncmp(LINE_CUST_DI21_FLT, pch->aucId, strlen(LINE_CUST_DI21_FLT)) == 0
                || strncmp(LINE_CUST_DI22_FLT, pch->aucId, strlen(LINE_CUST_DI22_FLT)) == 0
                || strncmp(LINE_CUST_DI23_FLT, pch->aucId, strlen(LINE_CUST_DI23_FLT)) == 0
                || strncmp(LINE_CUST_DI24_FLT, pch->aucId, strlen(LINE_CUST_DI24_FLT)) == 0
                || strncmp(LINE_CUST_DI25_FLT, pch->aucId, strlen(LINE_CUST_DI25_FLT)) == 0
                || strncmp(LINE_CUST_DI26_FLT, pch->aucId, strlen(LINE_CUST_DI26_FLT)) == 0
                || strncmp(LINE_CUST_DI27_FLT, pch->aucId, strlen(LINE_CUST_DI27_FLT)) == 0
                || strncmp(LINE_CUST_DI28_FLT, pch->aucId, strlen(LINE_CUST_DI28_FLT)) == 0
                || strncmp(LINE_CUST_DI29_FLT, pch->aucId, strlen(LINE_CUST_DI29_FLT)) == 0
                || strncmp(LINE_CUST_DI30_FLT, pch->aucId, strlen(LINE_CUST_DI30_FLT)) == 0
                || strncmp(LINE_CUST_DI31_FLT, pch->aucId, strlen(LINE_CUST_DI31_FLT)) == 0
                || strncmp(LINE_CUST_DI32_FLT, pch->aucId, strlen(LINE_CUST_DI32_FLT)) == 0
                || strncmp(LINE_CUST_DI33_FLT, pch->aucId, strlen(LINE_CUST_DI33_FLT)) == 0
                || (uiAppType_g == APP_BUS))
        {
            pch->ulFiltTimeLine=BYTES_TO_U32(puc)*1000+LINE_STORM_FILTER_ADD;		/*线路风暴时的 去抖动时间，转化为us */
            pch->ulFiltTime=BYTES_TO_U32(puc)*1000;		/* 去抖动时间，转化为us */
        }
        else
        {
            pch->ulFiltTimeLine=BYTES_TO_U32(puc)*1000;		/* 去抖动时间，转化为us */
            pch->ulFiltTime=BYTES_TO_U32(puc)*1000;		/* 去抖动时间，转化为us */
        }
#endif
        pch->ulFiltTimeLine=BYTES_TO_U32(puc)*1000+LINE_STORM_FILTER_ADD;       /*线路风暴时的 去抖动时间，转化为us */
        pch->ulFiltTime=BYTES_TO_U32(puc)*1000;     /* 去抖动时间，转化为us */

        puc+=4;
        pch->ReserveAttribute = *puc++;		/* 是否保留给平台使用 */

        /* 获得DI无效时的缺省值 2010-9-6  ZY*/
        pch->bDIInvalidDftVal=*puc++;

        pch->ucMmiShow = *puc++;		/* 是否显示 */

        puc+=5;		/* 保留 */

        pch->ucDIRefreshRate=puc[0];			/* 刷新速率 */
        assert(pch->ucDIRefreshRate==DI_FAST_REFRESH_RATE
               ||pch->ucDIRefreshRate==DI_MID_REFRESH_RATE
               ||pch->ucDIRefreshRate==DI_SLOW_REFRESH_RATE);
        puc++;

        iChCfgLen-=puc[0];
        EP_ID_Copy(aucModId, puc+1, puc[0]);
        aucModId[puc[0]]='\0';
        puc+=1+puc[0];

        for (p_part=apartinf_g; p_part<apartinf_g+MAX_PART_NUM; p_part++)
        {
            if (!strcmp(p_part->aucId, aucModId))
            {
                /* 在3个模件都有DI */
                assert(p_part->ucType==0x11 || p_part->ucType==0x12 || p_part->ucType==0x13 || p_part->ucType==0x16);
                break;
            }
        }
        assert(p_part<apartinf_g+MAX_PART_NUM);

        pch->p_part=p_part;

        if (p_part->ucPosition==0)			/* 主机箱 */
            pch->mod=RD_SPI_DI;
        else if (p_part->ucPosition==1)				/* 扩展机箱 */
            pch->mod=RD_EXT_DI;
        else if(p_part->ucPosition==8)			/* for excite */
            pch->mod=RD_REDUN_DI;
        else if (p_part->ucPosition==RD_64K_OPT1_BOX||p_part->ucPosition==RD_2M_OPT1_BOX)
            pch->mod=RD_OPT1_DI;
        else if (p_part->ucPosition==RD_64K_OPT2_BOX||p_part->ucPosition==RD_2M_OPT2_BOX)
            pch->mod=RD_OPT2_DI;
        else  if(p_part->ucPosition==RD_SAME_POLE_BOX)/*2007-3-28日 张云  */
            pch->mod=RD_SAME_POLE_DI;
        else  if(p_part->ucPosition==RD_HDL_BOX)
            pch->mod=RD_HDL_BOX_DI;
        else if(p_part->ucPosition == RD_VT_BOX)
        {
            /* 模件所位于的位置 */
            pch->mod=RD_VT_BOX_DI;		/* 开入输入通道类型 */
            pch->ucVtBoxPos=p_part->ucVtBoxPos;		/* 开入通道所位于虚拟机箱的位置 */
        }
        else
            assert(FALSE);

        pch->ucModCh=*puc++;			/* 所属物理模件序号 */

        pch->bRec=*puc++;
        if (pch->bRec)
        {

            if((pch->mod==RD_OPT1_DI)||(pch->mod==RD_OPT2_DI))
            {
                /*光纵不能配置录波  */
                LOG_Dbg_Msg("硬件配置错误，光纵DI通道不能配置录波!\n",0,0,0,0,0,0);
                assert(FALSE);
            }
            if(pch->mod==RD_SAME_POLE_BOX)
            {
                /*同杆并架DI不能配置录波  2007-3-28日张云*/
                LOG_Dbg_Msg("硬件配置错误，同杆并架DI通道不能配置录波!\n",0,0,0,0,0,0);
                assert(FALSE);
            }
            if(pch->mod==RD_HDL_BOX)
            {
                /*智能操作相DI不能配置录波  2007-3-28日张云*/
                LOG_Dbg_Msg("硬件配置错误，智能操作箱DI通道不能配置录波!\n",0,0,0,0,0,0);
                assert(FALSE);
            }

            iChCfgLen-=1+puc[0];
            EP_ID_Copy(pch->aucRecId, puc+1, puc[0]);
            puc+=1+puc[0];
        }

        pch->bFlag=*puc++;
        if (pch->bFlag)
        {

            if((pch->mod==RD_OPT1_DI)||(pch->mod==RD_OPT2_DI))
            {
                /*光纵不能配置标志  */
                LOG_Dbg_Msg("硬件配置错误，光纵DI通道不能配置标志!\n",0,0,0,0,0,0);
                assert(FALSE);
            }

            if(pch->mod==RD_SAME_POLE_BOX)
            {
                /*同杆并架DI不能配置标志  2007-3-28日张云*/
                LOG_Dbg_Msg("硬件配置错误，同杆并架DI通道不能配置标志!\n",0,0,0,0,0,0);
                assert(FALSE);
            }
            if(pch->mod==RD_HDL_BOX)
            {
                /*智能操作相DI不能配置标志  2007-3-28日张云*/
                LOG_Dbg_Msg("硬件配置错误，智能操作箱DI通道不能配置标志!\n",0,0,0,0,0,0);
                assert(FALSE);
            }

            iChCfgLen-=1+puc[0];
            EP_ID_Copy(pch->aucFlagId, puc+1, puc[0]);
            puc+=1+puc[0];
        }

        pch->bMea=*puc++;
        if (pch->bMea)
        {

            if((pch->mod==RD_OPT1_DI)||(pch->mod==RD_OPT2_DI))
            {
                /*光纵不能配置遥信  */
                LOG_Dbg_Msg("硬件配置错误，光纵DI通道不能配置遥信!\n",0,0,0,0,0,0);
                assert(FALSE);
            }

            if(pch->mod==RD_SAME_POLE_BOX)
            {
                /*同杆并架DI不能配置遥信  2007-3-28日张云*/
                LOG_Dbg_Msg("硬件配置错误，同杆并架DI通道不能配置遥信!\n",0,0,0,0,0,0);
                assert(FALSE);
            }
            if(pch->mod==RD_HDL_BOX)
            {
                /*智能操作相DI不能配置遥信  2007-3-28日张云*/
                LOG_Dbg_Msg("硬件配置错误，智能操作箱DI通道不能配置遥信!\n",0,0,0,0,0,0);
                assert(FALSE);
            }


            iChCfgLen-=1+puc[0];
            EP_ID_Copy(pch->aucMeaId, puc+1, puc[0]);
            puc+=1+puc[0];
        }

        pch->iMeaCh=-1;
        assert(iChCfgLen==26);

        pch->utChgTime.ucQflag=0x60;
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
        return EP_SUCCESS;
}

/***********************************************************************
* RD_Cfg_DO - 开出配置项目
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_DO(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
)
{
    RD_LGC_DO_CH *pch;
    RD_PART_INFO *p_part;
    int iChCfgLen;
    uint8_t *puc;
    uint8_t aucModId[MAX_ID_LEN+1];

    iLgcDoChNum_g=U8_TO_U16(pucCfg[1], pucCfg[0]);
    if ((plgcdoch_g=calloc(iLgcDoChNum_g, sizeof(*plgcdoch_g)))==NULL)
        return EP_BUF_ERR;

    puc=pucCfg+6;                       /* Skip DOCount and reserved 4 bytes. */

    for (pch=plgcdoch_g; pch<plgcdoch_g+iLgcDoChNum_g; pch++)
    {
        iChCfgLen=U8_TO_U16(puc[1], puc[0]);

        iChCfgLen-=puc[2];
        EP_ID_Copy(pch->aucId, puc+3, puc[2]);
        puc+=3+puc[2];

        pch->unLgcSN=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        iChCfgLen-=puc[0];
        EP_ID_Copy(pch->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        pch->aucABRV[0]=*puc++;
        pch->aucABRV[1]=*puc++;
        pch->aucABRV[2]=*puc++;
        pch->aucABRV[3]=*puc++;

        pch->ReserveAttribute = *puc++;		/* 是否保留给平台使用 */
        pch->ucMmiShow = *puc++;		/* 是否显示 */

        puc += 2;

        iChCfgLen-=puc[0];
        EP_ID_Copy(aucModId, puc+1, puc[0]);
        puc+=1+puc[0];

        for (p_part=apartinf_g; p_part<apartinf_g+MAX_PART_NUM; p_part++)
        {
            if (!strcmp(p_part->aucId, aucModId))
            {
                /* 在2个模件里有DO */
                assert(p_part->ucType==0x11 || p_part->ucType==0x13 || p_part->ucType==0x16);
                break;
            }
        }
        assert(p_part<apartinf_g+MAX_PART_NUM);

        pch->p_part=p_part;

        if (p_part->ucPosition==0)
            pch->mod=RD_SPI_DO;
        else if(p_part->ucPosition==8)
            pch->mod=RD_REDUN_DO; 				/* for excite */
        else if (p_part->ucPosition==RD_64K_OPT1_BOX||p_part->ucPosition==RD_2M_OPT1_BOX)
            pch->mod=RD_OPT1_DO;
        else if (p_part->ucPosition==RD_64K_OPT2_BOX||p_part->ucPosition==RD_2M_OPT2_BOX)
            pch->mod=RD_OPT2_DO;
        else  if(p_part->ucPosition==RD_SAME_POLE_BOX)/*2007-3-28日 张云  */
            pch->mod=RD_SAME_POLE_DO;
        else  if(p_part->ucPosition==RD_HDL_BOX)
            pch->mod=RD_HDL_BOX_DO;
        else if(p_part->ucPosition == RD_VT_BOX)		/* 虚拟机箱 */
        {
            /* 模件所位于虚拟机箱的位置 */
            pch->mod=RD_VT_BOX_DO;		/* 开出类型 */
            pch->ucVtBoxPos=p_part->ucVtBoxPos;				/* 开出所位于虚拟机箱的位置 */
        }
        else
            assert(FALSE);

        pch->ucModCh=*puc++;

        pch->bValid=*puc++;		/* 是否有效 */

        puc+=5;		/* 保留 */

        assert(iChCfgLen==20);
        pch->iVal=0;
        pch->iTripDOCnt=0;
        pch->iSetVal=0;  /* 2011-8-18 ZY  */
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
        return EP_SUCCESS;
}

/***********************************************************************
* RD_Cfg_Hw_LED - 面板指示灯配置
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_Hw_LED(
    uint8_t *pucCfg, 		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
)
{
    RD_LGC_LED_CH *pch;
    int iChCfgLen;
    uint8_t *puc;

    iHwLedChNum_g=U8_TO_U16(pucCfg[1], pucCfg[0]);
    iLgcLedChNum_g=iHwLedChNum_g;
    if ((plgcledch_g=calloc(iLgcLedChNum_g, sizeof(*plgcledch_g)))==NULL)
        return EP_BUF_ERR;

    puc=pucCfg+6;                       /* Skip LedCount and reserved 4 bytes. */

    for (pch=plgcledch_g; pch<plgcledch_g+iLgcLedChNum_g; pch++)
    {
        pch->bIsHwLED=TRUE;

        iChCfgLen=U8_TO_U16(puc[1], puc[0]);

        iChCfgLen-=puc[2];
        EP_ID_Copy(pch->aucId, puc+3, puc[2]);
        puc+=3+puc[2];

        pch->unCh=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        iChCfgLen-=puc[0];
        EP_ID_Copy(pch->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        puc+=4;

        pch->ucColor=*puc++;
        assert(pch->ucColor>=1 && pch->ucColor<=3);

        pch->ucBlink=*puc++;

        puc+=3;

        pch->bKeep=*puc++;

        assert(iChCfgLen==14);
        pch->bSts=FALSE;
        pch->iTripLedCnt=0;
        pch->bSetVal=FALSE;  /*2011-8-18 ZY  */
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
        return EP_SUCCESS;
}

/***********************************************************************
* RD_Cfg_Sw_LED - 屏幕指示灯配置
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_Sw_LED(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
)
{
    RD_LGC_LED_CH *pch;
    int iChCfgLen;
    uint8_t *puc;

    assert(plgcledch_g);                /* Hardware LED must be configed before. */

    iSwLedChNum_g=U8_TO_U16(pucCfg[1], pucCfg[0]);

    if (!iSwLedChNum_g)
        return EP_SUCCESS;

    assert(iLgcLedChNum_g==iHwLedChNum_g);
    iLgcLedChNum_g+=iSwLedChNum_g;

    if ((plgcledch_g=realloc(plgcledch_g,
                             iLgcLedChNum_g*sizeof(*plgcledch_g)))==NULL)
        return EP_BUF_ERR;
    memset(plgcledch_g+iHwLedChNum_g, 0, iSwLedChNum_g*sizeof(*plgcledch_g));

    puc=pucCfg+6;                       /* Skip LedCount and reserved 4 bytes. */

    for (pch=plgcledch_g+iHwLedChNum_g;
            pch<plgcledch_g+iLgcLedChNum_g; pch++)
    {
        pch->bIsHwLED=FALSE;

        iChCfgLen=U8_TO_U16(puc[1], puc[0]);

        iChCfgLen-=puc[2];
        EP_ID_Copy(pch->aucId, puc+3, puc[2]);
        puc+=3+puc[2];

        pch->unCh=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        iChCfgLen-=puc[0];
        EP_ID_Copy(pch->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        puc+=4;

        pch->ucColor=*puc++;
        assert(pch->ucColor>=1 && pch->ucColor<=3);

        pch->ucBlink=*puc++;

        puc+=3;

        pch->bKeep=*puc++;

        assert(iChCfgLen==14);

        pch->bSts=FALSE;
        pch->iTripLedCnt=0;
        pch->bSetVal=FALSE;  /*2011-8-18 ZY  */
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
        return EP_SUCCESS;
}

/***********************************************************************
* RD_Cfg_Virtual_AI - 虚拟模拟输入通道配置
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_Virtual_AI(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
)
{
    RD_VT_AI_CH *pch;
    int iChCfgLen;
    uint8_t *puc;

    /* Virtaul AI must be configed before logic AI. */
    assert(!plgcaich_g);

    iVtAiChNum_g=U8_TO_U16(pucCfg[1], pucCfg[0]);
    if ((pvtaich_g=calloc(iVtAiChNum_g, sizeof(*pvtaich_g)))==NULL)
        return EP_BUF_ERR;

    puc=pucCfg+6;

    for (pch=pvtaich_g; pch<pvtaich_g+iVtAiChNum_g; pch++)
    {
        iChCfgLen=U8_TO_U16(puc[1], puc[0]);

        iChCfgLen-=puc[2];
        EP_ID_Copy(pch->aucId, puc+3, puc[2]);
        puc+=3+puc[2];

        pch->unLgcSN=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        puc+=4;

        pch->ucUnit=*puc++;

        pch->bRec=*puc++;
        if (pch->bRec)
        {
            iChCfgLen-=1+puc[0];
            EP_ID_Copy(pch->aucRecId, puc+1, puc[0]);
            puc+=1+puc[0];
        }

        pch->bFlag=*puc++;
        if (pch->bFlag)
        {
            iChCfgLen-=1+puc[0];
            EP_ID_Copy(pch->aucFlagId, puc+1, puc[0]);
            puc+=1+puc[0];
        }

        pch->bMea=*puc++;
        if (pch->bMea)
        {
            iChCfgLen-=1+puc[0];
            EP_ID_Copy(pch->aucMeaId, puc+1, puc[0]);
            puc+=1+puc[0];
        }

        puc+=4;

        assert(iChCfgLen==15);
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
        return EP_SUCCESS;
}

/***********************************************************************
* RD_Cfg_Msu_AI - 测量通道配置
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Cfg_Msu_AI(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
)
{
    RD_MSU_AI_CH *pch; 			/* 测量有关文件配置结构指针 */
    int iChCfgLen;
    uint8_t *puc;
    EP_STATUS stsRet;

    stsRet=EP_SUCCESS;

    iMsuAiChNum_g=U8_TO_U16(pucCfg[1], pucCfg[0]); 		/* 所有测量通道 */
    if ((pmsuaich_g=calloc(iMsuAiChNum_g, sizeof(*pmsuaich_g)))==NULL) 		/* 为文件配置结构分配存储空间 */
        return EP_BUF_ERR;

    AIBaseCh_g = pucCfg[2];		/* 基准测量通道号 */

#ifndef NO_DEBUG
    LOG_Dbg_Msg("AIBaseCh_g = %d\n", AIBaseCh_g, 0, 0, 0, 0, 0);
#endif

    puc=pucCfg+7;

    for (pch=pmsuaich_g; pch<pmsuaich_g+iMsuAiChNum_g; pch++)
    {
        iChCfgLen=U8_TO_U16(puc[1], puc[0]); 		/* 测量通道的数据内容 大小 */
        iChCfgLen-=puc[2];

        EP_ID_Copy(pch->aucId, puc+3, puc[2]); 			/* 逻辑标志字符串长度 */

        puc+=3+puc[2];

        pch->unLgcSN=U8_TO_U16(puc[1], puc[0]); 			/* 在测量通道集中的序号 */
        puc+=2;

        puc+=4;
        assert(puc[0]<iHwAiChNum_g && phwaich_g); 		/* 物理通道集中的序号及物理通道结构判断 */
        pch->phwai=phwaich_g+*puc++; 		/* 物理通道，在物理通道集中的序号，不是物理通道 */

#ifndef NO_DEBUG
        LOG_Dbg_Msg("MSU Channel: %d %d\n",*(puc-1),pch->phwai->ucModCh,0,0,0,0);
#endif

        pch->ucFiltTp=*puc++; 		/* 滤波算法 */
        pch->ucUnit=*puc++; 					/* 单位类型 */

        pch->bFlag=*puc++; 				/* 设置是否上传 */
        if(pch->bFlag)
        {
            iChCfgLen-=1+puc[0];
            EP_ID_Copy(pch->aucFlagId, puc+1, puc[0]); 				/* 逻辑标志字符串 */
            puc+=1+puc[0];
        }
        assert(iChCfgLen==11);
    }
    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        stsRet=EP_PARM_ERR;
    }
    return stsRet;
}

/***********************************************************************
* RD_Cfg_Lgc_PI - 装置脉冲输入量配置解析
*
* RETURNS: 无
*
*/
EP_STATUS RD_Cfg_Lgc_PI(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
)
{
    RD_LGC_PI_CH *pch;
    int iChCfgLen;
    uint8_t *puc;

    iLgcPiChNum_g=U8_TO_U16(pucCfg[1], pucCfg[0]);
    if ((plgcpich_g=calloc(iLgcPiChNum_g, sizeof(*plgcpich_g))) == NULL)
        return EP_BUF_ERR;


    puc=pucCfg+10; 				/* Skip PICount and reserved 8 bytes. */

    for (pch=plgcpich_g; pch<plgcpich_g+iLgcPiChNum_g; pch++)
    {
        iChCfgLen=U8_TO_U16(puc[1], puc[0]);  		/* 数据内容大小 */

        iChCfgLen-=puc[2];			/* 逻辑标志字符串长度 */
        EP_ID_Copy(pch->aucId, puc+3, puc[2]);  		/* 逻辑标志字符串 */
        puc+=3+puc[2];		/* 逻辑标志字符串之后 */

        iChCfgLen-=puc[0];
        EP_ID_Copy(pch->aucName, puc+1, puc[0]);		/* 内部名称字符串 */
        puc+=1+puc[0];

        pch->aucABRV[0]=*puc++;			/* 内部简称 */
        pch->aucABRV[1]=*puc++;
        pch->aucABRV[2]=*puc++;
        pch->aucABRV[3]=*puc++;


        pch->unLgcSN=U8_TO_U16(puc[1], puc[0]);  	/* 在PI通道集中的序号 */
        puc+=2;

        puc+=16;		/* 保留16个字节 */

        EP_ID_Copy(pch->RsvStr1, puc+1, puc[0]);		/* 第一个保留字符串 */
        puc+=1+puc[0]+4;
        EP_ID_Copy(pch->RsvStr2, puc+1, puc[0]);		/* 第二个保留字符串 */
        puc+=1+puc[0]+4;
        EP_ID_Copy(pch->RsvStr3, puc+1, puc[0]);		/* 第三个保留字符串 */
        puc+=1+puc[0];

        pch->ucUnit = *puc++;		/* 单位类型 */


        pch->bRec=*puc++;
        if (pch->bRec)
        {
            /* 滤波模拟量 */
            iChCfgLen-=1+puc[0];
            EP_ID_Copy(pch->aucRecId, puc+1, puc[0]);
            puc+=1+puc[0];
        }

        pch->bFlag=*puc++;
        if (pch->bFlag)
        {
            /* 标志量 */
            iChCfgLen-=1+puc[0];
            EP_ID_Copy(pch->aucFlagId, puc+1, puc[0]);
            puc+=1+puc[0];
        }

        pch->bMea=*puc++;
        if (pch->bMea)
        {
            /* 遥测量 */
            iChCfgLen-=1+puc[0];
            EP_ID_Copy(pch->aucMeaId, puc+1, puc[0]);
            puc+=1+puc[0];
        }
        assert(iChCfgLen==39);		/* 剩余长度，待定 */
    }

    if (puc-pucCfg != ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
        return EP_SUCCESS;
}

/***********************************************************************
* RD_Cfg_Lgc_PI - 装置脉冲输出量配置解析
*
* RETURNS: 无
*
*/
EP_STATUS RD_Cfg_Lgc_PO(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
)
{
    RD_LGC_PO_CH *pch;
    int iChCfgLen;
    uint8_t *puc;

    iLgcPoChNum_g=U8_TO_U16(pucCfg[1], pucCfg[0]);
    if ((plgcpoch_g=calloc(iLgcPoChNum_g, sizeof(*plgcpoch_g))) == NULL)
        return EP_BUF_ERR;


    puc=pucCfg+10; 			/* Skip PICount and reserved 8 bytes. */

    for (pch=plgcpoch_g; pch<plgcpoch_g+iLgcPoChNum_g; pch++)
    {
        iChCfgLen=U8_TO_U16(puc[1], puc[0]);  		/* 数据内容大小 */

        iChCfgLen-=puc[2];			/* 逻辑标志字符串长度 */
        EP_ID_Copy(pch->aucId, puc+3, puc[2]);  		/* 逻辑标志字符串 */
        puc+=3+puc[2];		/* 逻辑标志字符串之后 */

        iChCfgLen-=puc[0];
        EP_ID_Copy(pch->aucName, puc+1, puc[0]);		/* 内部名称字符串 */
        puc+=1+puc[0];

        pch->aucABRV[0]=*puc++;			/* 内部简称 */
        pch->aucABRV[1]=*puc++;
        pch->aucABRV[2]=*puc++;
        pch->aucABRV[3]=*puc++;


        pch->unLgcSN=U8_TO_U16(puc[1], puc[0]);  	/* 在PI通道集中的序号 */
        puc+=2;

        pch->ucUnit = *puc++;		/* 单位类型 */

        puc+=15;		/* 保留15个字节 */

        EP_ID_Copy(pch->RsvStr1, puc+1, puc[0]);		/* 第一个保留字符串 */
        puc+=1+puc[0]+4;
        EP_ID_Copy(pch->RsvStr2, puc+1, puc[0]);		/* 第二个保留字符串 */
        puc+=1+puc[0]+4;
        EP_ID_Copy(pch->RsvStr3, puc+1, puc[0]);		/* 第三个保留字符串 */
        puc+=1+puc[0];

        pch->ucType = *puc++;		/* 通道类型 */

        assert(iChCfgLen==36);		/* 剩余长度，待定 */
    }

    if (puc-pucCfg != ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
        return EP_SUCCESS;
}

/***********************************************************************
* DA_InitAOCfg - 初始化DA输出的AO所有配置
*
* RETURNS: 成功与否
*
*/
EP_STATUS DA_InitAOCfg(
    int iAOCfgNum		/* 所有AO配置个数 */
)
{
    DA_PART_AO_CFG *pPartAOCfg;
    int  i;
    DA_AO_CFG *pAOCfg;

    pPartAOCfg=&PartAoCfgDa_g;

    assert(iAOCfgNum<=MAX_DA_AO_NUM);

    pPartAOCfg->iDaAONum=iAOCfgNum;
    pPartAOCfg->iDaAISrcAONum=0;
    pPartAOCfg->iDaMidSrcAONum=0;

    pAOCfg=pPartAOCfg->aDaAoCfg_g;
    for(i=0; i<iAOCfgNum; i++)
    {
        /* 全部置为空 */
        pAOCfg->ucAOHdCh=-1;
        pAOCfg->iAOSrcType=DA_VOID_SRC;
        pAOCfg->ucSrcAIHdCh=-1;
        pAOCfg->pElemSrc=NULL;
        pAOCfg++;
    }

    return  EP_SUCCESS;
}

/***********************************************************************
* RD_GetHwAiByOptAoId - 根据光纵AO的ID，获得本机硬件AI的句柄
*
* RETURNS: 本机硬件AI的句柄，为空，说明出错未找到
*
*/
void *RD_GetHwAiByOptAoId(
    uint8_t *strOptAoId,	/* 光纵句柄 */
    uint8_t ucUnit					/* 单位 */
)
{
    RD_HW_AI_CH *pch = NULL;
    RD_HW_AI_CH *pFindCh = NULL;
    int i;
    int  iFindCnt=0;

    pch=phwaich_g;
    for (i=0; i<iHwAiChNum_g; i++)
    {
        if(pch->bSetOptAo)
        {
            if(!(strcmp(strOptAoId,pch->aucAoId))
                    &&ucUnit==pch->ucUnit)
            {
                pFindCh=pch;
                iFindCnt++;
            }
        }

        pch++;
    }
    if(iFindCnt==1)
    {
        return  pFindCh;
    }
    else  if(iFindCnt==0)
    {
        return  NULL;
    }
    else
    {
        return  NULL;
    }
}

/* Get the physical AI handle by AO ID.
 * Para:
 *     strAoId, ID.
 *     ucUnit, unit.
 * Return:
 *     NONE.
 */
void *RD_GetHwAiByAoId(uint8_t *strAoId, uint8_t ucUnit)
{
    RD_HW_AI_CH *pch = NULL;
    RD_HW_AI_CH *pFindCh = NULL;
    int i;
    int iFindCnt=0;

    pch=phwaich_g;
    for (i=0; i<iHwAiChNum_g; i++)
    {
        if(pch->bSetAo)
        {
            if (!(strcmp(strAoId, pch->aucAoLogicId)))   /* ucUnit == pch->ucUnit */
            {
                pFindCh=pch;
                iFindCnt++;
            }
        }

        pch++;
    }

    if (iFindCnt == 1)
    {
        return pFindCh;
    }
    else if (iFindCnt == 0)
    {
        return NULL;
    }
    else
    {
        return NULL;
    }
}

/* Get the logic AI handle bye AO ID.
 * Para:
 *     strAoId, ID.
 *     ucUnit, unit.
 * Return:
 *     NONE.
 */
void *RD_GetLgcAiByAoId(uint8_t *strAoId, uint8_t ucUnit)
{
    RD_LGC_AI_CH *pch = NULL;
    RD_LGC_AI_CH *pFindCh = NULL;
    int i;
    int iFindCnt=0;

    pch=plgcaich_g;
    for (i=0; i<iLgcAiChNum_g-iVtAiChNum_g; i++)
    {
        if(pch->bSetAo)
        {
            if (!(strcmp(strAoId, pch->aucAoId)))	/* ucUnit == pch->ucUnit */
            {
                pFindCh=pch;
                iFindCnt++;
            }
        }

        pch++;
    }

    if (iFindCnt == 1)
    {
        return pFindCh;
    }
    else if (iFindCnt == 0)
    {
        return NULL;
    }
    else
    {
        return NULL;
    }
}

/***********************************************************************
* RD_Cfg_AO - AO输出量配置解析
*
* RETURNS: 无
*
*/
EP_STATUS RD_Cfg_AO(
    uint8_t *pucCfg,		/* 配置指针 */
    uint32_t ulLen		/* 长度 */
)
{
    RD_PART_INFO *p_part;
    RD_HW_AO_CH *pch;
    int iChCfgLen;
    int i;
    uint8_t *puc;
    uint8_t aucModId[MAX_ID_LEN+1];
    int iIdStrLen;
    int iNameStrLen;
    BOOL bInitVtBoxAOCfg[MAX_VT_BOX_COUNT]= {FALSE};

    BOOL bInitOpt1BoxAOCfg=FALSE;
    BOOL bInitOpt2BoxAOCfg=FALSE;


    static BOOL bInitDaPartAOCfg=FALSE;

#ifdef EXCITE_BUILD
    static BOOL bInitDaPartRedunAOCfg=FALSE;
#endif
    int iAONum;

#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)			/* 智能操作箱 */
    BOOL bInitSamePoleAOCFG=FALSE;		/* 2007-3-28日 张云 */
    BOOL bInitHdlBoxAOCFG=FALSE;		/* 2007-3-28日 张云 */
#endif

    iHwAoChNum_g=U8_TO_U16(pucCfg[1], pucCfg[0]); /* AO配置总数 */
    if ((phwaoch_g=calloc(iHwAoChNum_g, sizeof(*phwaoch_g)))==NULL)
        return EP_BUF_ERR;

    for (i=0; i<MAX_VT_BOX_COUNT; i++)
    {
        bInitVtBoxAOCfg[i]=FALSE;		/* 初始值 */
    }

    puc=pucCfg+6;
    for (i=0; i<iHwAoChNum_g; i++)
    {
        iChCfgLen=U8_TO_U16(puc[1], puc[0]);

        iIdStrLen=puc[2]; 			/* AO逻辑标志字符串长度 */
        iNameStrLen=puc[3+iIdStrLen];   				/* 内部名称字符串长度 */
        iAONum=puc[8+iIdStrLen+iNameStrLen]; 							/* 物理通道集中的序号 */
        pch=phwaoch_g+iAONum;

        assert(pch->aucId[0]=='\0');				 /* Never used before. */

        iChCfgLen-=puc[2];
        EP_ID_Copy(pch->aucId, puc+3, puc[2]);
        puc+=3+puc[2];

        iChCfgLen-=puc[0];
        EP_ID_Copy(pch->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        pch->aucABRV[0]=*puc++;  /* 内部简称 */
        pch->aucABRV[1]=*puc++;
        pch->aucABRV[2]=*puc++;
        pch->aucABRV[3]=*puc++;

        puc++;

        pch->fMaxVal=BYTES_TO_FLT(puc); /* 量程最大值*/
        puc+=4;

        pch->fMinVal=BYTES_TO_FLT(puc);		/* 量程最小值*/

        puc+=4;

        pch->FactorSetModWord = *puc++;		 /* 配置方式控制字 */
        if((pch->FactorSetModWord)&0x01)
        {
            iChCfgLen-=puc[0];
            iChCfgLen--;		/* 与以前规约不一致*/
            EP_ID_Copy(pch->MaxValueDingzhiTagBase, puc+1, puc[0]);			/* 最大值内部定值逻辑标志字符串 */
            puc+=1+puc[0];
        }
        if((pch->FactorSetModWord)&0x02)
        {
            iChCfgLen-=puc[0];
            iChCfgLen--;
            EP_ID_Copy(pch->MinValueDingzhiTagBase, puc+1, puc[0]);				/* 最小值内部定值逻辑标志字符串 */
            puc+=1+puc[0];
        }
        if((pch->FactorSetModWord)&0x04)
        {
            iChCfgLen-=puc[0];
            iChCfgLen--;
            EP_ID_Copy(pch->ScalefactorDingzhiTagBase, puc+1, puc[0]);					/* 最小值内部定值逻辑标志字符串 */
            puc+=1+puc[0];
        }

        pch->ucAttr=*puc++;		/* 刷新速度 */

        puc+=1;

        if (*puc++)
        {
            /* 是否邦定AO数据源 */
            iChCfgLen -= puc[0];
            iChCfgLen--;
            EP_ID_Copy(pch->aucSrcId, puc+1, puc[0]);					/* 最小值内部定值逻辑标志字符串 */
            puc += 1+puc[0];

            pch->ucSrcType=*puc++;		/* 取值算法类型 */
            iChCfgLen--;
        }

        pch->ucOptAOOutputIntv=*puc++;		/* 光纵输出发送间隔，以采样点为单位 */

        iChCfgLen-=puc[0];
        EP_ID_Copy(aucModId, puc+1, puc[0]);  /* 所属物理模件标志字符串 */
        puc+=1+puc[0]; /* 越过标志字符串 */

        for (p_part=apartinf_g; p_part<apartinf_g+MAX_PART_NUM; p_part++)
        {
            /* 寻找所属物理模件 */
            if (!strcmp(p_part->aucId, aucModId)) 				/* aucModId为所属物理模件标志字符串 */
            {
                assert(p_part->ucType==0x15);
                break;
            }
        }
        pch->p_part = p_part;

        assert(p_part<apartinf_g+MAX_PART_NUM);

        if (p_part->ucPosition == RD_LOCAL_BOX) 					/* 模件位置，在主机箱 */
        {
            pch->paimod=&aimodDsp_g;
            if(!bInitDaPartAOCfg)
            {
                if(DA_InitAOCfg(p_part->ucAoNum)!=EP_SUCCESS) 				/* 初始化DA输出模件 */
                {
                    assert(FALSE);
                }
            }
        }
        else if (p_part->ucPosition == RD_REDUN_BOX) 			/* AO模件在冗余机箱, for excite */
        {
#ifdef EXCITE_BUILD      	/* 使用冗余机箱, DY 6/24/2007 */
            pch->paimod=&aimodRedun_g;
            if(!bInitDaPartRedunAOCfg)
            {
                if(DA_InitRedunAOCfg(p_part->ucAoNum)!=EP_SUCCESS) 				/* 初始化DA输出模件 */
                {
                    assert(FALSE);
                }
            }
#endif
        }
        else  if((p_part->ucPosition==RD_64K_OPT1_BOX)
                 ||(p_part->ucPosition==RD_2M_OPT1_BOX))
        {
            if(!bInitOpt1BoxAOCfg)
            {
                bInitOpt1BoxAOCfg=TRUE;

                if(OPT_InitAOCfg(0,p_part->ucAoNum)!=EP_SUCCESS) 						/* 初始化光纵机箱 */
                {
                    assert(FALSE);
                }
            }
            pch->paimod=&aimodOpt_g[0]; 						/* 所属机箱号 */
        }
        else  if((p_part->ucPosition==RD_64K_OPT2_BOX)
                 ||(p_part->ucPosition==RD_2M_OPT2_BOX))
        {
            if(!bInitOpt2BoxAOCfg)
            {
                bInitOpt2BoxAOCfg=TRUE;
                if(OPT_InitAOCfg(1,p_part->ucAoNum)!=EP_SUCCESS)
                {
                    assert(FALSE);
                }
            }
            pch->paimod=&aimodOpt_g[1]; 						/* 给顶所属机箱号 */
        }

#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)			/* 智能操作箱 */
        else  if(p_part->ucPosition==RD_SAME_POLE_BOX)/*2007-3-28日 张云  */
        {
            if(!bInitSamePoleAOCFG)
            {
                bInitSamePoleAOCFG=TRUE;
                if(POLE_InitAOCfg(p_part->ucAoNum)!=EP_SUCCESS)
                {
                    assert(FALSE);
                }
            }
            pch->paimod=&aimodPole_g;
        }
        else  if(p_part->ucPosition==RD_HDL_BOX)/*2007-3-28日 张云  */
        {
            if(!bInitHdlBoxAOCFG)
            {
                bInitHdlBoxAOCFG=TRUE;
                if(HDL_InitAOCfg(p_part->ucAoNum)!=EP_SUCCESS)
                {
                    assert(FALSE);
                }
            }
            pch->paimod=&aimodHdl_g;
        }
#endif

        else if (p_part->ucPosition == RD_VT_BOX)		/* 虚拟机箱 */
        {
            /* 模件位置，所有虚拟机箱有统一的类型 */
            assert(FALSE);
        }
        else
        {
            assert(FALSE);

            return EP_ERROR;
        }
        pch->ucModCh=*puc++; 				/* 在物理模件中的序号*/
        pch->fOriCoff=BYTES_TO_FLT(puc);
        pch->fCoff=pch->fOriCoff; 						/* 通道比例系数 */
        pch->fSetCoff=pch->fOriCoff;		/* 设定系数 */
        puc+=4;

        pch->ucUnit=*puc++;  			/* 物理通道数据单位 */
        /* LOG_Dbg_Msg("p_part->ucPosition = %d pch->aucId = %s pch->ucUnit = %d\n",p_part->ucPosition,(int)pch->aucId,pch->ucUnit,0,0,0); */

        pch->iAOSrcType=DA_VOID_SRC; 				/* 数据来源类型 */
        pch->pvSrc=NULL; 			/* 数据来源地点 */

#if defined(EDP_01_02_BUILD) || defined(EDP03_BUILD)		/* 如果是从AI来，则寻找到相应的模拟通道 */
        if((pch->pvSrc=(void  *)RD_GetHwAiByOptAoId(pch->aucId,pch->ucUnit))!=NULL)
        {
            /*在AI表中查找光纵AO设置  */
            pch->iAOSrcType=DA_AI_SRC;
        }
#endif

        if (pch->iAOSrcType == DA_VOID_SRC)
        {
            if ((pch->pvSrc = (void *)RD_GetHwAiByAoId(pch->aucSrcId, pch->ucUnit)) != NULL)
            {
                /* 在AI表中查找AO设置 */
                pch->iAOSrcType=DA_AI_SRC;
            }
        }

        if (pch->iAOSrcType == DA_VOID_SRC)
        {
            if ((pch->pvSrc = (void *)RD_GetLgcAiByAoId(pch->aucSrcId, pch->ucUnit)) != NULL)
            {
                pch->iAOSrcType = DA_PRE_SRC;
            }
        }

        assert(iChCfgLen==27);
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }

    if(!bInitOpt1BoxAOCfg)
    {
        bInitOpt1BoxAOCfg=TRUE;
        if(OPT_InitAOCfg(0,0)!=EP_SUCCESS)
        {
            assert(FALSE);
        }
    }

    if(!bInitOpt2BoxAOCfg)
    {
        bInitOpt2BoxAOCfg=TRUE;
        if(OPT_InitAOCfg(1,0)!=EP_SUCCESS)
        {
            assert(FALSE);
        }
    }

    return EP_SUCCESS;
}

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
)
{
    RD_HW_AO_CH *pch = NULL;
    RD_HW_AO_CH *pFindCh = NULL;
    int i;
    int iFindCnt=0;

    assert(pvElemSrc);		/* 来源不能为空 */
    pch=phwaoch_g; 		/* 配置信息 */
    for (i=0; i<iHwAoChNum_g; i++)
    {
        if(!(strcmp(strOptAoId,pch->aucId))
                && ucUnit==pch->ucUnit)
        {
            assert(pch->iAOSrcType == DA_VOID_SRC);
            pch->iAOSrcType=DA_MID_SRC;		/* 用于DA输出的中间结果 */
            pch->pvSrc=pvElemSrc; 					/* 数据源, 用在配置上 */
            pFindCh=pch;
            iFindCnt++;
        }
        pch++;
    }
    if(iFindCnt == 1)
    {
#ifdef EXCITE_BUILD
        if(pFindCh->paimod == &aimodDsp_g)	/* 主机箱*/
        {
            LOG_Dbg_Msg("strOptAoId = %s\n", (int)strOptAoId, 0, 0, 0, 0, 0);		/* DY 9/7/2006 */
            if(DA_Init_Mid_Src_AO(pFindCh->iAOSrcType, pFindCh->ucModCh, pFindCh->pvSrc, pFindCh ->fCoff) == EP_SUCCESS)
            {
                return EP_SUCCESS;
            }
            else
            {
                return EP_ERROR;
            }
        }
        else if(pFindCh->paimod == &aimodRedun_g)		/* 冗余机箱*/
        {
            LOG_Dbg_Msg("strOptAoId = %s\n",(int)strOptAoId,0,0,0,0,0);		/* DY 9/7/2006 */
            if(DA_Redun_Init_Mid_Src_AO(pFindCh->iAOSrcType, pFindCh->ucModCh, pFindCh->pvSrc, pFindCh ->fCoff) == EP_SUCCESS)
            {
                return EP_SUCCESS;
            }
            else
            {
                return EP_ERROR;
            }
        }
        else
        {
            assert(FALSE);
            return  EP_ERROR;
        }
#endif

        if(pFindCh->paimod==&(aimodOpt_g[0]))
        {
            if(OPT_Init_Mid_Src_AO(0,pFindCh->iAOSrcType, pFindCh->ucModCh,pFindCh->pvSrc)==EP_SUCCESS)
            {
                return EP_SUCCESS;
            }
            else
            {
                return EP_ERROR;
            }
        }
        else  if(pFindCh->paimod==&(aimodOpt_g[1]))
        {
            if(OPT_Init_Mid_Src_AO(1,pFindCh->iAOSrcType, pFindCh->ucModCh, pFindCh->pvSrc)==EP_SUCCESS)
            {
                return EP_SUCCESS;
            }
            else
            {
                return EP_ERROR;
            }
        }
        else if(pFindCh->paimod==&(aimodPole_g))
        {
            if(POLE_Init_Mid_Src_AO(pFindCh->iAOSrcType, pFindCh->ucModCh, pFindCh->pvSrc)==EP_SUCCESS)
            {
                return EP_SUCCESS;
            }
            else
            {
                return EP_ERROR;
            }
        }
        else if(pFindCh->paimod==&(aimodHdl_g))
        {
            if(HDL_Init_Mid_Src_AO(pFindCh->iAOSrcType, pFindCh->ucModCh, pFindCh->pvSrc)==EP_SUCCESS)
            {
                return EP_SUCCESS;
            }
            else
            {
                return EP_ERROR;
            }
        }
        else
        {
            assert(FALSE);
            return  EP_ERROR;
        }

        /* 此处之前有问题 */
        if(pFindCh->paimod==&(aimodPole_g))
        {
            if(POLE_Init_Mid_Src_AO(pFindCh->iAOSrcType, pFindCh->ucModCh, pFindCh->pvSrc)==EP_SUCCESS)
            {
                return EP_SUCCESS;
            }
            else
            {
                return EP_ERROR;
            }
        }
        else
        {
            assert(FALSE);
            return  EP_ERROR;
        }

    }

    else if(iFindCnt == 0)
    {
        return EP_ERROR;
    }

    return  EP_ERROR;
}

/* Set AO from logic graph in virtual box, call by logic graph.
 * Para:
 *     strOptAoId, ID of AO.
 *     ucUnit, unit.
 *     pvElemSrc, source pointer from logic graph middle variable
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS RD_VirtBoxAddMidSrcAo(uint8_t *strOptAoId, uint8_t ucUnit, void *pvElemSrc)
{
    RD_HW_AO_CH *pch, *pFindCh;
    int i;
    int iFindCnt=0;

    assert (pvElemSrc);
    assert (FALSE);

    pch=phwaoch_g; 		/* configuration. */
    for (i=0; i<iHwAoChNum_g; i++)
    {
        if(!(strcmp(strOptAoId, pch->aucSrcId))
                && (ucUnit == pch->ucUnit))
        {
            assert(pch->iAOSrcType == DA_VOID_SRC);

            pch->iAOSrcType=DA_MID_SRC;		/* logic graph middle variable. */
            pch->pvSrc=pvElemSrc; 					/* source pointer. */
            pFindCh=pch;

            iFindCnt++;
        }
        pch++;
    }

    if (iFindCnt >= 1)
    {
        return EP_SUCCESS;
    }
    else if (iFindCnt == 0)
    {
        return EP_ERROR;
    }
    else
    {
        return  EP_ERROR;
    }
}

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
)
{
    if (iIdx<iLgcPiChNum_g)
        return plgcpich_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

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
)
{
    if (iIdx<iLgcPoChNum_g)
        return plgcpoch_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/***********************************************************************
* GetPiCfgNum - 获取PI配置数
*
* RETURNS: PI配置数
*
*/
int GetPiCfgNum(void)
{
    return iLgcPiChNum_g;
}

/***********************************************************************
* GetPoCfgNum - 获取PO配置数
*
* RETURNS: PO配置数
*
*/
int GetPoCfgNum(void)
{
    return iLgcPoChNum_g;
}

/***********************************************************************
* GetAiSeqNum - 由AI逻辑标志字符串获得其配置序号
*
* RETURNS: AI配置序号
*
*/
int GetAiSeqNum(
    char * AiName			/* 逻辑标志字符串 */
)
{
    RD_LGC_AI_CH *pch;
    int i ;

    pch = plgcaich_g;
    for(i=0; i<iLgcAiChNum_g; i++)
    {
        if(strcmp(pch->aucId,AiName) ==0 )
            return (pch-plgcaich_g);
        pch++;
    }

    return -1;
}

/***********************************************************************
* ModifyAiScaleCoe - 修改Ai逻辑配置对应物理通道的增益系数
*
* RETURNS: 无
*
*/
void ModifyAiScaleCoe(
    int SeqNum,			/* 序号 */
    float ScaleCoe
)
{
    RD_LGC_AI_CH *pch;

    taskLock();

    pch = plgcaich_g+SeqNum;
    pch->phwai->fCoff= ScaleCoe;

    taskUnlock();
}

/***********************************************************************
* GetAiScaleCoe - 获得Ai逻辑配置对应物理通道的增益系数
*
* RETURNS: 无
*
*/
float GetAiScaleCoe(
    int SeqNum			/* 序号 */
)
{
    RD_LGC_AI_CH *pch;

    pch = plgcaich_g+SeqNum;

    return pch->phwai->fCoff;
}

/***********************************************************************
* ModifyAiScaleCoeLgc - 修改Ai逻辑配置对应物理通道的增益系数
*
* RETURNS: 无
*
*/
void ModifyAiScaleCoeLgc(
    void *pAiCfg,				/* AI逻辑通道配置 */
    float ScaleCoe
)
{
    RD_LGC_AI_CH *pch;

    pch = (RD_LGC_AI_CH *)pAiCfg;
    taskLock();

    pch->phwai->fGain= ScaleCoe;

    taskUnlock();
}

/***********************************************************************
* GetAiScaleCoeLgc - 获得Ai逻辑配置对应物理通道的增益系数
*
* RETURNS: 增益系数
*
*/
float GetAiScaleCoeLgc(
    void *pAiCfg				/* AI逻辑通道配置 */
)
{
    RD_LGC_AI_CH *pch;

    pch = (RD_LGC_AI_CH *)pAiCfg;

    return pch->phwai->fGain;
}

/***********************************************************************
* ModifyAiExcCoe - 修改Ai逻辑配置对应物理通道的偏移系数
*
* RETURNS: 无
*
*/
void ModifyAiExcCoe(
    int SeqNum,			/* 序号 */
    float fExcCoe
)
{
    RD_LGC_AI_CH *pch;

    taskLock();		/* DY 11/2/2006 */

    pch = plgcaich_g+SeqNum;
    pch->phwai->fExcCoff= fExcCoe;

    taskUnlock();
}

/***********************************************************************
* GetAiExcCoe - 获得Ai逻辑配置对应物理通道的偏移系数
*
* RETURNS: 偏移系数
*
*/
float GetAiExcCoe(
    int SeqNum			/* 序号 */
)
{
    RD_LGC_AI_CH *pch;

    pch = plgcaich_g+SeqNum;

    return pch->phwai->fExcCoff;
}

/***********************************************************************
* ModifyAiExcCoeLgc - 修改Ai逻辑配置对应物理通道的偏置系数
*
* RETURNS: 无
*
*/
void ModifyAiExcCoeLgc(
    void *pAiCfg,				/* AI逻辑通道配置 */
    float fExcCoe
)
{
    RD_LGC_AI_CH *pch;

    pch = (RD_LGC_AI_CH *)pAiCfg;
    taskLock();

    pch->phwai->fExcCoff= fExcCoe;

    taskUnlock();
}

/***********************************************************************
* GetAiExcCoeLgc - 获得Ai逻辑配置对应物理通道的偏置系数
*
* RETURNS: 偏置系数
*
*/
float GetAiExcCoeLgc(
    void *pAiCfg				/* AI逻辑通道配置 */
)
{
    RD_LGC_AI_CH *pch;

    pch = (RD_LGC_AI_CH *)pAiCfg;

    return pch->phwai->fExcCoff;
}

/***********************************************************************
* GetAiCoffLgc - 获得Ai逻辑配置对应物理通道的比例系数
*
* RETURNS: 比例系数
*
*/
float GetAiCoffLgc(
    void *pAiCfg				/* AI逻辑通道配置 */
)
{
    RD_LGC_AI_CH *pch;

    pch = (RD_LGC_AI_CH *)pAiCfg;

    return pch->phwai->fCoff*5.0/32768;		/* 通道比例系数，包括模拟转数字的比例 */
}

/***********************************************************************
* ModifyDiFiltTime - 修改DI配置通道消抖时间
*
* RETURNS: 无
*
*/
void ModifyDiFiltTime(
    void *pDiCfg,				/* DI逻辑通道配置 */
    uint32_t ulFiltTime
)
{
    RD_LGC_DI_CH *pch;

    taskLock();

    pch =(RD_LGC_DI_CH *)pDiCfg;
    pch->ulFiltTime  = ulFiltTime;

    taskUnlock();
}

/***********************************************************************
* RD_Chg_Led_Attr - 调整指示灯属性
*
* RETURNS: 无
*
*/
void RD_Chg_Led_Attr(
    void *pvLenHnd,
    BOOL bKeep,
    uint8_t ucColor,
    uint8_t ucBlink
)
{
    RD_LGC_LED_CH *pch;

    taskLock();

    pch = (RD_LGC_LED_CH *)pvLenHnd;

    taskUnlock();
}

/***********************************************************************
* RD_Chg_Coff - 通过定值修改配置系数
*
* RETURNS: 无
*
*/
EP_STATUS RD_Chg_Coff(void)
{
    RD_LGC_AI_CH *paich;
    RD_HW_AO_CH *paoch;
    RD_LGC_DI_CH *pdich;
    SCI_SIGNAL_VALUE_TYPE RtSettingValue;
    STATUS vxsts;	/* DY 11/2/2006 */
    EP_STATUS stsRet;

    stsRet = EP_SUCCESS;
    vxsts=taskLock();		/* DY 11/2/2006 */

    for(paich=plgcaich_g; paich<plgcaich_g+iLgcAiChNum_g; paich++)
    {
        /* AI修改 */
        if((paich->phwai->FactorSetModWord)&0x01)
        {
            /* 最大值*/
            stsRet = SCI_Get_Inner_Setting_BySettingStrBase(paich->phwai->MaxValueDingzhiTagBase, &RtSettingValue);
            if(stsRet == EP_SUCCESS)
            {
                paich->phwai->fMaxVal = (float)(RtSettingValue.Value.fVal);
            }
            else
            {
                assert(FALSE);
            }
        }
        if((paich->phwai->FactorSetModWord)&0x02)
        {
            /* 最小值*/
            stsRet = SCI_Get_Inner_Setting_BySettingStrBase(paich->phwai->MinValueDingzhiTagBase, &RtSettingValue);
            if(stsRet == EP_SUCCESS)
            {
                paich->phwai->fMinVal = (float)(RtSettingValue.Value.fVal);
            }
            else
            {
                assert(FALSE);
            }
        }
        if((paich->phwai->FactorSetModWord)&0x04)
        {
            /* 增益系数 */
            stsRet = SCI_Get_Coff_Inner_Setting_BySettingStrBase(paich->phwai->ScalefactorDingzhiTagBase,
                     &RtSettingValue, paich->phwai->iIndexSn); /* 增加通道索引定值页序 */

            if(stsRet == EP_SUCCESS)
            {
                /* paich->phwai->fCoff = (float)(RtSettingValue.Value.fVal); */
                paich->phwai->fSetCoff=RtSettingValue.Value.fVal; 	/* 设定系数 */
            }
            else
            {
                assert(FALSE);
            }
        }
    }

    for(paoch=phwaoch_g; paoch<phwaoch_g+iHwAoChNum_g; paoch++)
    {
        /* AO修改 */
        if((paoch->FactorSetModWord)&0x01)
        {
            /* 最大值*/
            stsRet =SCI_Get_Inner_Setting_BySettingStrBase(paoch->MaxValueDingzhiTagBase, &RtSettingValue);
            if(stsRet == EP_SUCCESS)
            {
                paoch->fMaxVal = (float)(RtSettingValue.Value.fVal);
            }
            else
            {
                assert(FALSE);
            }
        }
        if((paoch->FactorSetModWord)&0x02)
        {
            /* 最小值*/
            stsRet = SCI_Get_Inner_Setting_BySettingStrBase(paoch->MinValueDingzhiTagBase, &RtSettingValue);
            if(stsRet == EP_SUCCESS)
            {
                paoch->fMinVal = (float)(RtSettingValue.Value.fVal);
            }
            else
            {
                assert(FALSE);
            }
        }
        if((paoch->FactorSetModWord)&0x04)
        {
            /* 增益系数 */
            stsRet = SCI_Get_Inner_Setting_BySettingStrBase(paoch->ScalefactorDingzhiTagBase, &RtSettingValue);
            if(stsRet == EP_SUCCESS)
            {
                /* paoch->fCoff = (float)(RtSettingValue.Value.fVal); */
                paoch->fSetCoff=RtSettingValue.Value.fVal; 				/* 设定系数 */
            }
            else
            {
                assert(FALSE);
            }
        }
    }

    for(pdich=plgcdich_g; pdich<plgcdich_g+iLgcDiChNum_g; pdich++ )
    {
        /* DI消抖 */
        if(pdich->DebounceTimeSetMod == 1)
        {
            stsRet = SCI_Get_Inner_Setting_BySettingStrBase(pdich->DebounceTimeDingzhiTag, &RtSettingValue);
            if(stsRet == EP_SUCCESS)
            {
                pdich->ulFiltTime = RtSettingValue.Value.ulVal;
            }
            else
            {
                assert(FALSE);
            }
        }
    }

    vxsts=taskUnlock();

    return stsRet;
}

/***********************************************************************
* RD_Wr_PO - 脉冲输出量输出
*
* RETURNS: 无
*
*/
void RD_Wr_PO(
    void *pvPoHnd,				/* 用来索引PO对象的void指针，应该通过调用本模块提供的RD_Get_Handle得到 */
    uint32_t ulVal				/* 写入值 */
)
{
    RD_LGC_PO_CH *plgcpo;

    taskLock();

    plgcpo=(RD_LGC_PO_CH *)pvPoHnd;
    plgcpo->Val.ulVal = ulVal;		/* 写入 */

    taskUnlock();
}

/***********************************************************************
* RD_Rd_PO - 获取脉冲输出量
*
* RETURNS: 无
*
*/
void RD_Rd_PO(
    void *pvPoHnd,				/* 用来索引PO对象的void指针，应该通过调用本模块提供的RD_Get_Handle得到 */
    uint32_t *pulVal				/* 写入值地址 */
)
{
    RD_LGC_PO_CH *plgcpo;

    taskLock();

    plgcpo=(RD_LGC_PO_CH *)pvPoHnd;

    *pulVal = plgcpo->OriginVal.ulVal;

    taskUnlock();
}

/***********************************************************************
* RD_Cfg_Hw_AI_Gain - 读取增益系数文件
*
* RETURNS: 无
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*/
static EP_STATUS RD_Cfg_Hw_AI_Gain(
    int iFd		/* 文件句柄 */
)
{
    uint8_t aucBuf[10];
    int i;
    uint8_t aucFlag[256];

    assert(iFd>=0);

    lseek(iFd, -4, SEEK_END);

    if (read(iFd, aucBuf, 4)!=4 ||
            aucBuf[0]!=0xA8 || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x57)
        return EP_FILE_ERR;

    lseek(iFd, 0, SEEK_SET);

    if (read(iFd, aucBuf, 10)!=10 ||
            aucBuf[0]!=0xA2 || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x5D|| aucBuf[9]!=iHwAiChNum_g)
    {
        if(aucBuf[9] != iHwAiChNum_g)
        {
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                           "增益系数文件无效\n",
                           0, 0);

            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_LOCK | ER_ALARM | ER_REPORT,
                           "Hardware AI channel gain coefficient file is invalid\n",
                           0, 0);
            }
        }
        return EP_FILE_ERR;
    }

    memset(aucFlag, 0, sizeof(aucFlag));
    for (i=0; i<iHwAiChNum_g; i++)
    {
        if (read(iFd, aucBuf, 9)==9 && aucBuf[0]<iHwAiChNum_g &&
                !aucFlag[aucBuf[0]])
        {
            aucFlag[aucBuf[0]]=0xFF;

            phwaich_g[aucBuf[0]].fExcCoff = BYTES_TO_FLT(aucBuf+1);		/* 偏置系数 */
            phwaich_g[aucBuf[0]].fGain=BYTES_TO_FLT(aucBuf+5);
            phwaich_g[aucBuf[0]].fCoff*=phwaich_g[aucBuf[0]].fGain;
        }
        else
        {
            assert(FALSE);
            return EP_FILE_ERR;
        }
    }

    return EP_SUCCESS;
}

/***********************************************************************
* RD_Cfg_CT_Ratio - 读取CT变比系数文件
*
* RETURNS: 无
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*/
static EP_STATUS RD_Cfg_CT_Ratio(
    int iFd		/* 文件句柄 */
)
{
    uint8_t aucBuf[MAX_ID_LEN+10];
    int iLineNum;
    int iItem;
    int i;
    RD_HW_AI_CH *phwai;
    int k=0;
    float fTmp;

    assert(iFd>=0);

    lseek(iFd, 0, SEEK_SET);

    if (read(iFd, aucBuf, 10)!=10 ||
            aucBuf[0]!=0xC0 || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x3F)
        return EP_FILE_ERR;

    iLineNum=aucBuf[9];
    k=iLineNum;

    /* 是否带CRC校验判断 */
    if (aucBuf[6] & 0x01)
    {
        lseek(iFd, -6, SEEK_END);

        if (read(iFd, aucBuf, 6) != 6 ||
                aucBuf[0] != 0xCA || aucBuf[1] != 0 || aucBuf[2] != 0 || aucBuf[3] != 0x35)
            return EP_FILE_ERR;

        /* CRC判断 */
        if (!SC_Check_CRC(iFd))
        {
            return EP_FILE_ERR;
        }
    }
    else
    {
        /* 无CRC判断 */
        lseek(iFd, -4, SEEK_END);

        if (read(iFd, aucBuf, 4) != 4 ||
                aucBuf[0] != 0xCA || aucBuf[1] != 0 || aucBuf[2] != 0 || aucBuf[3] != 0x35)
            return EP_FILE_ERR;
    }

    /* 重新置位 */
    lseek(iFd, 10, SEEK_SET);

    /*读取单元相关信息  */
    while(k--)
    {
        i=read(iFd, aucBuf, 1);
        assert(i==1 && aucBuf[0] && aucBuf[0]<=MAX_ID_LEN);

        i=read(iFd, aucBuf+1, aucBuf[0]+4);
        assert(i==aucBuf[0]+4);

        if(k == (iLineNum-1))
        {
            /* 第一个单元为基准单元 */
            uBaseUnitFstRatedVal_g=U8_TO_U16(aucBuf[1+aucBuf[0]+1], aucBuf[1+aucBuf[0]]);
            uBaseUnitSecRatedVal_g=U8_TO_U16(aucBuf[1+aucBuf[0]+3], aucBuf[1+aucBuf[0]+2]);
        }
    }

    i=read(iFd, aucBuf, 4);
    assert(i==4);

    i=read(iFd, aucBuf, 1);
    assert(i==1);

    iItem=aucBuf[0];
    if (iItem>iHwAiChNum_g)
        return EP_FILE_ERR;

    while (iItem--)
    {
        i=read(iFd, aucBuf, 1);
        assert(i==1 && aucBuf[0] && aucBuf[0]<=MAX_ID_LEN);

        i=read(iFd, aucBuf+1, aucBuf[0]+11);
        assert(i==aucBuf[0]+11);

        i=aucBuf[aucBuf[0]+1];
        assert(i && i<=iLineNum);

        aucBuf[aucBuf[0]+1]='\0';

        for (phwai=phwaich_g; phwai<phwaich_g+iHwAiChNum_g; phwai++)
        {
            if (!strcmp(aucBuf+1, phwai->aucId))
            {
                /* Not be set before. */
                assert(phwai->fRatio==1.0);
                break;
            }
        }
        if (phwai>=phwaich_g+iHwAiChNum_g)
            return EP_FILE_ERR;
        else
        {
            phwai->uFstRatedVal=U8_TO_U32(aucBuf[1+aucBuf[0]+4], aucBuf[1+aucBuf[0]+3], aucBuf[1+aucBuf[0]+2], aucBuf[1+aucBuf[0]+1]);
            phwai->uSecRatedVal=U8_TO_U16(aucBuf[1+aucBuf[0]+6], aucBuf[1+aucBuf[0]+5]);
            fTmp = phwai->uSecRatedVal;
            fTmp = fTmp/100.0;

            phwai->fTradSecToFstCoff = fabs((float)phwai->uFstRatedVal/fTmp);

            /* phwai->fRatio=BYTES_TO_FLT(aucBuf+1+aucBuf[0]+5); */
            phwai->fRatio=1.0;		/* 使用默认值  */

            phwai->fCoff*=phwai->fRatio;/*可以为0 */
        }
    }

    return EP_SUCCESS;
}

/* 生成新的CT变比系数文件
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
int RD_Get_CT_Ratio_File_Len(void)
{
    int iLen = 0;
    RD_HW_AI_CH *phwai;

    iLen += 10; /* CT变比系数文件头 */
    iLen += strlen(UNIT_ID)+1;   /* CT变比系数第1单元名称 */
    iLen += 2;   /* 第1号单元一次额定值 */
    iLen += 2;   /* 第1号单元二次额定值 */
    iLen += 4;   /* 保留4个字节 */
    iLen += 1;   /* 物理通道总数 */
    for (phwai = phwaich_g; phwai<phwaich_g+iHwAiChNum_g; phwai++)
    {
        iLen += strlen(phwai->aucId)+1;   /* 通道名称 */
        iLen += 1;   /* 单元号 */
        iLen += 4;   /* 一次额定值 */
        iLen += 2;   /* 二次额定值 */
        iLen += 4;   /* 增益系数 */
    }
    iLen += 4;  /* 保留 */
    iLen += 2;  /* CRC */

    return iLen;
}

/* 生成新的CT变比系数文件
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void RD_New_CT_Ratio(void)
{
#define UNIT_ID "Unit 1"  /* 缺省的间隔单元名称 */
    int i;
    int iFd;
    uint8_t aucBuf[MAX_ID_LEN];
    int iLen;
    RD_HW_AI_CH *phwai;
    EP_STATUS sts;
    uint8_t *pucDat;  /* 文件内容存储地址 */
    int temp_val; /* 文件长度计算 */
    int file_lenth;
    uint16_t unCalcCrc = 0;

    sts = EP_SUCCESS;

    iFd = FT_Bgn_Update(EP_CT_RATIO_FILE);
    assert (iFd >= 0);

    /* 文件头符 */
    aucBuf[0] = 0xC0;
    aucBuf[1] = 0x00;
    aucBuf[2] = 0x00;
    aucBuf[3] = 0x3F;
    i = write(iFd, aucBuf, 4);
    assert (i == 4);

    /* 版本号 */
    aucBuf[0] = LO8(EP_INNER_PRTCL_VER);
    aucBuf[1] = HI8(EP_INNER_PRTCL_VER);
    i = write(iFd, aucBuf, 2);
    assert (i == 2);

    /* 保留 */
    aucBuf[0] = 0x01; /* 计算CRC */
    aucBuf[1] = 0x00;
    aucBuf[2] = 0x00;
    i = write(iFd, aucBuf, 3);
    assert (i == 3);

    /* 单元数
     * 目前缺省为1个单元
     */
    iLineNum_g = 1;
    aucBuf[0] = iLineNum_g;
    i = write(iFd, aucBuf, 1);
    assert (i == 1);

    /* 第1号单元名称长度 */
    iLen = strlen(UNIT_ID);
    aucBuf[0] = iLen;
    i = write(iFd, aucBuf, 1);
    assert (i == 1);

    /* 第一号单元名称 */
    memcpy(aucBuf, UNIT_ID, iLen);
    assert (iLen <= MAX_ID_LEN);
    i = write(iFd, aucBuf, iLen);
    assert (i == iLen);

    /* 第1号单元一次额定值 */
    aucBuf[0] = 0x0;
    aucBuf[1] = 0x0;
    i = write(iFd, aucBuf, 2);
    assert (i == 2);

    /* 第1号单元二次额定值 */
    aucBuf[0] = 0x0;
    aucBuf[1] = 0x0;
    i = write(iFd, aucBuf, 2);
    assert (i == 2);

    /* 保留4个字节 */
    aucBuf[0] = 0x0;
    aucBuf[1] = 0x0;
    aucBuf[2] = 0x0;
    aucBuf[3] = 0x0;
    i = write(iFd, aucBuf, 4);
    assert (i == 4);

    /* 物理通道总数 */
    aucBuf[0] = iHwAiChNum_g;
    i = write(iFd, aucBuf, 1);
    assert (i == 1);

    /* 逐一写入物理通道参数
     */
    for (phwai = phwaich_g; phwai<phwaich_g+iHwAiChNum_g; phwai++)
    {
        /* 通道名称长度 */
        iLen = strlen(phwai->aucId);
        aucBuf[0] = iLen;
        i = write(iFd, aucBuf, 1);
        assert (i == 1);

        /* 通道名称 */
        memcpy(aucBuf, phwai->aucId, iLen);
        assert (iLen <= MAX_ID_LEN);
        i = write(iFd, aucBuf, iLen);
        assert (i == iLen);

        /* 单元号
         * 缺省认为在第一单元号
         */
        aucBuf[0] = 1;
        i = write(iFd, aucBuf, 1);
        assert (i == 1);

        /* 一次额定值 */
        aucBuf[0] = LL8(phwai->uFstRatedVal);
        aucBuf[1] = LH8(phwai->uFstRatedVal);
        aucBuf[2] = HL8(phwai->uFstRatedVal);
        aucBuf[3] = HH8(phwai->uFstRatedVal);
        i = write(iFd, aucBuf, 4);
        assert (i == 4);

        /* 二次额定值 */
        aucBuf[0] = LO8(phwai->uSecRatedVal);
        aucBuf[1] = HI8(phwai->uSecRatedVal);
        i = write(iFd, aucBuf, 2);
        assert (i == 2);

        /* 增益系数,无效
         */
        i = write(iFd, (uint8_t *)&phwai->fRatio, 4);
        assert (i == 4);
    }

    /* 保留 */
    aucBuf[0] = 0xCA;
    aucBuf[1] = 0x00;
    aucBuf[2] = 0x00;
    aucBuf[3] = 0x35;
    i = write(iFd, aucBuf, 4);
    assert (i == 4);

    /* 立即写入文件 */
    // ioctl (iFd, FIOFLUSH, 0);
    fsync(iFd);
    /* 长度计算 */
    temp_val = lseek(iFd, 0, SEEK_SET);
    file_lenth = lseek(iFd, 0, SEEK_END);
    file_lenth = file_lenth-temp_val;

    /* CRC计算 */
    if ((pucDat = malloc(file_lenth)) == NULL)
    {
        if (iFd >= 0)
        {
            /* 关闭已打开文件 */
            close(iFd);
        }

        return;
    }
    lseek(iFd, 0, SEEK_SET);
    i = read(iFd, pucDat, file_lenth);	/* 读数据 */
    assert (i == file_lenth);
    unCalcCrc = EP_CCITT_CRC16(pucDat, file_lenth, unCalcCrc);
    free(pucDat);

    /* CRC */
    aucBuf[0] = LO8(unCalcCrc);
    aucBuf[1] = HI8(unCalcCrc);
    i = write(iFd, aucBuf, 2);
    assert (i == 2);

    sts = FT_End_Update(EP_CT_RATIO_FILE, iFd);
    assert (sts == EP_SUCCESS);

    LOG_Write(LOG_OPRATE, "创建新的CT/PT变比系数文件.\n", NULL);
}

/***********************************************************************
* PoWrInit - 脉冲电量保存初始化
*
* RETURNS: 无
*
*/
EP_STATUS PoWrInit(void)
{
    EP_STATUS sts;

    sts=EP_SUCCESS;

#ifndef POFESTRAMMEM
    FT_Temp_Name_New(aucTempFile, EP_PO_FILE);
    if (!FT_Is_File(EP_PO_FILE) && !FT_Is_File(aucTempFile))
    {
        iFd=FT_Bgn_Update(EP_PO_FILE);
        assert(iFd >= 0);

        sts=FT_End_Update(EP_PO_FILE, iFd);
        assert(sts == EP_SUCCESS);
    }
    else
    {

    }
#endif

    return sts;
}

/***********************************************************************
* Reset_PO_Val - 脉冲输出量清零
*
* RETURNS: 无
*
*/
void Reset_PO_Val()
{
    RD_LGC_PO_CH *plgcpo;

    taskLock();

    for(plgcpo = plgcpoch_g; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++)
    {
        /* 清零 */
        if(plgcpo->ucType == 1)
        {
            /* 仅仅是电度清零 */
            plgcpo->OriginVal.TotalEnergy = 0;
        }
    }

    taskUnlock();
}

/***********************************************************************
* ResetFRAMVal - 铁电存储器清零
*
* RETURNS: 无
*
*/
EP_STATUS ResetFRAMVal()
{
    int i;
    EP_STATUS sts;
    RD_LGC_PO_CH *plgcpo;
    unsigned char *pTmpData;
    float fVal;
    uint16_t LastCRC=0;
    FLT_U32_UNION ResultTmp;

    sts = EP_SUCCESS;

    ResultTmp.fVal=0.0;

    if(iLgcPoChNum_g == 0)
    {
        /* 没有配置测量不执行清零操作 */
        return EP_SUCCESS;
    }

#ifndef POFESTRAMMEM
    iFd=FT_Bgn_Update(EP_PO_FILE);
    assert(iFd >= 0);

    for(plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
    {
        /* CRC计算 */
        pTmpData=&(ResultTmp.ulVal);
        LastCRC=EP_CCITT_CRC16(pTmpData, 4, LastCRC);
    }

    for(plgcpo = plgcpoch_g; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++)
    {
        /* 写电量 */
        i=write(iFd, &ResultTmp.ulVal, 4);
        assert(i == 4);
    }

    i=write(iFd, &LastCRC, 2);		/* CRC校验 */
    assert(i == 2);

    i=write(iFd, &iLgcPoChNum_g, 4);		/* 个数 */
    assert(i == 4);

    sts=FT_End_Update(EP_PO_FILE, iFd);
    assert(sts == EP_SUCCESS);

    if(sts == EP_SUCCESS)
    {
        LOG_Dbg_Msg("PoWrFile OK!\n", 0, 0, 0, 0, 0, 0);
    }
#endif

#ifdef POFESTRAMMEM
    for(plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
    {
        pTmpData=(unsigned char *)(&fVal);
        LastCRC=EP_CCITT_CRC16(pTmpData, 4, LastCRC);
    }

    for(plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
    {
        pTmpData=(unsigned char *)(&fVal);
        if(write_ram_data_cycle(4*i, pTmpData, 4) != OK)
        {
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 不闭锁开出*/
                           "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 不闭锁开出*/
                           "FRAM Error(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
            }
            continue;
        }


        if(write_ram_data_cycle(0x1000+4*i, pTmpData, 4) != OK)
        {
            if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 不闭锁开出*/
                           "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
            }
            else if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 不闭锁开出*/
                           "FRAM Error(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
            }
            continue;
        }/* 双缓冲 */

    }

    if(write_ram_data_cycle(4*i, (unsigned char *)&LastCRC, 2) != OK)
    {
        /* CRC码*/
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 不闭锁开出*/
                       "铁电存储器错误(%02d)\n",WRITING_ERR,0);
            LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 不闭锁开出*/
                       "FRAM Error(%02d)\n",WRITING_ERR,0);
            LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
        }
    }


    if(write_ram_data_cycle(0x1000+4*i, (unsigned char *)&LastCRC, 2) != OK)
    {
        /* CRC码 */
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 不闭锁开出*/
                       "铁电存储器错误(%02d)\n",WRITING_ERR,0);
            LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 不闭锁开出*/
                       "FRAM Error(%02d)\n",WRITING_ERR,0);
            LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
        }
    }/* 双缓冲 */

    if(write_ram_data_cycle(4*i+2, (unsigned char *)&iLgcPoChNum_g, 4) != OK)
    {
        /* PO个数*/
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 不闭锁开出*/
                       "铁电存储器错误(%02d)\n",WRITING_ERR,0);
            LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 不闭锁开出*/
                       "FRAM Error(%02d)\n",WRITING_ERR,0);
            LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
        }
    }


    if(write_ram_data_cycle(0x1000+4*i+2, (unsigned char *)&iLgcPoChNum_g, 4) != OK)
    {
        /* PO个数 */
        if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 不闭锁开出*/
                       "铁电存储器错误(%02d)\n",WRITING_ERR,0);
            LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 不闭锁开出*/
                       "FRAM Error(%02d)\n",WRITING_ERR,0);
            LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
        }
    }/* 双缓冲 */
#endif

    return sts;
}

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
)
{
    int CycleNum=0;
    STATUS sts;

    semTake(semI2CWrEnableFlag, WAIT_FOREVER);					/* 防止重入 */
    while(CycleNum<I2CCYCLEMAXNUM)
    {
        if(CycleNum>FestRamErrorMaxNum)
        {
            FestRamErrorMaxNum=CycleNum;
        }

        if(write_ram_data(addr, pBuf, length) == OK)
        {
            sts=semGive(semI2CWrEnableFlag);
            assert(sts == OK);
            return OK;
        }

#ifdef EXCITE_BUILD
        if(TRUE)
        {
            return OK;
        }
#endif
        else
        {
            CycleNum++;
        }

        taskDelay(I2CDELAYTICKNUM);
    }

    sts=semGive(semI2CWrEnableFlag);
    assert(sts == OK);

    return ERROR;
}

/* write PO ro FRAM.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS PoWrFile(void)
{
    int i;
    EP_STATUS sts;
    RD_LGC_PO_CH *plgcpo = NULL;
    unsigned char *pTmpData = NULL;
    uint16_t LastCRC=0;
    FLT_U32_UNION ResultTmpArray[MAXPONUM];		/* 最大50个电度量 */
    STATUS vxsts;
    static BOOL bFstWrFlag=TRUE;		/* 第一次写标志 */
    static uint32_t ulErrorCnt=0;


    sts = EP_SUCCESS;

    if (iLgcPoChNum_g == 0)
    {
        /* 没有配置测量 */
        return EP_SUCCESS;
    }

#ifndef POFESTRAMMEM   		/* 写文件 */

    /* FT_Temp_Name_New(aucTempFile, EP_PO_FILE); */

    vxsts=taskLock();	/* 防止被高优先级的任务打断 */
    for (plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
    {
        ResultTmpArray[i].ulVal=plgcpo->Val.ulVal;
    }
    vxsts=taskUnlock();

    for (plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
    {
        pTmpData=&(ResultTmpArray[i].ulVal);
        LastCRC=EP_CCITT_CRC16(pTmpData, 4, LastCRC);
    }

    iFd=FT_Bgn_Update(EP_PO_FILE);
    assert(iFd >= 0);

    for (plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
    {
        if (FloatValidCheck(ResultTmpArray[i].fVal) != EP_SUCCESS)
        {
            ulErrorCnt++;
            if ((ulErrorCnt%1000) == 1)
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 不闭锁开出*/
                               "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                    LOG_Write(LOG_KERNEL, "铁电写入浮点错误!\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		/* 不闭锁开出*/
                               "FRAM Error(%02d)\n",WRITING_ERR,0);
                    LOG_Write(LOG_KERNEL, "Float point error when Fest RAM writing!\n", NULL);
                }
            }
            continue;
        }

        BufNum=write(iFd, &ResultTmpArray[i].ulVal, 4);
        assert(BufNum == 4);
    }

    BufNum=write(iFd, &LastCRC, 2);		/* CRC校验 */
    assert(BufNum == 2);

    if (bFstWrFlag)
    {
        /* 写一次 */
        /* bFstWrFlag=FALSE; */
        BufNum=write(iFd, &iLgcPoChNum_g, 4);		/* 个数 */
        assert(BufNum == 4);
    }

    sts=FT_End_Update(EP_PO_FILE, iFd);
    assert(sts == EP_SUCCESS);

    if (sts == EP_SUCCESS)
    {
        LOG_Dbg_Msg("PoWrFile OK!\n", 0, 0, 0, 0, 0, 0);
    }
#endif

#ifdef POFESTRAMMEM
    vxsts=taskLock();	/* 防止被高优先级的任务打断 */
    for (plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
    {
        ResultTmpArray[i].ulVal=plgcpo->Val.ulVal;
    }
    vxsts=taskUnlock();

    for (plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
    {
        pTmpData=(unsigned char *)&(ResultTmpArray[i].ulVal);
        LastCRC=EP_CCITT_CRC16(pTmpData, 4, LastCRC);
    }

    /* 第一存储区 */
    for (plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
    {
        pTmpData=(unsigned char *)&(ResultTmpArray[i].ulVal);
        if (FloatValidCheck(ResultTmpArray[i].fVal) != EP_SUCCESS)
        {
            ulErrorCnt++;
            if ((ulErrorCnt%1000) == 1)
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                    LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "FRAM error(%02d)\n",WRITING_ERR,0);
                    LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
                }
            }
            continue;
        }

        if (write_ram_data_cycle(4*i, pTmpData, 4) !=OK)
        {
            ulErrorCnt++;
            if ((ulErrorCnt%1000) == 1)
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                    LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "FRAM error(%02d)\n",WRITING_ERR,0);
                    LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
                }
            }
            continue;
        }
    }

    if (write_ram_data_cycle(4*i, (unsigned char *)&LastCRC, 2) != OK)
    {
        /* CRC码*/
        ulErrorCnt++;
        if((ulErrorCnt%1000) == 1)
        {
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "FRAM error(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
            }
        }
    }

    /* 第二存储区 */
    for (plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
    {
        pTmpData=(unsigned char *)&(ResultTmpArray[i].ulVal);
        if (FloatValidCheck(ResultTmpArray[i].fVal) != EP_SUCCESS)
        {
            ulErrorCnt++;
            if ((ulErrorCnt%1000) == 1)
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                    LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "FRAM error(%02d)\n",WRITING_ERR,0);
                    LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
                }
            }
            continue;
        }

        if (write_ram_data_cycle(0x1000+4*i, pTmpData, 4) != OK)
        {
            ulErrorCnt++;
            if ((ulErrorCnt%1000) == 1)
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                    LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "FRAM error(%02d)\n",WRITING_ERR,0);
                    LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);

                }
            }
            continue;
        }
    }

    if (write_ram_data_cycle(0x1000+4*i, (unsigned char *)&LastCRC, 2) != OK)
    {
        /* CRC码 */
        ulErrorCnt++;
        if ((ulErrorCnt%1000) == 1)
        {
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "FRAM error(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
            }
        }
    } /* 双缓冲 */

    if (bFstWrFlag)
    {
        /* 个数只写1次 */
        bFstWrFlag=FALSE;
        if (write_ram_data_cycle(4*i+2, (unsigned char *)&iLgcPoChNum_g, 4) != OK)
        {
            /* PO个数 */
            ulErrorCnt++;
            if ((ulErrorCnt%1000) == 1)
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                    LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "FRAM error(%02d)\n",WRITING_ERR,0);
                    LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
                }
            }
        }

        /* taskDelay(1); */ 	/* 为防止数据写入与CRC写入的一致性被中断，暂时不延迟 */

        if (write_ram_data_cycle(0x1000+4*i+2, (unsigned char *)&iLgcPoChNum_g, 4) != OK)
        {
            /* PO个数 */
            ulErrorCnt++;
            if ((ulErrorCnt%1000) == 1)
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                    LOG_Write(LOG_KERNEL, "铁电写入错误!\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "FRAM error(%02d)\n",WRITING_ERR,0);
                    LOG_Write(LOG_KERNEL, "Fest RAM writing error!\n", NULL);
                }
            }
        }  /* 双缓冲 */
    }
#endif

    return sts;
}

/* read PO from FRAM.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS PoRdFile(void)
{
    int i;
    EP_STATUS retcode = EP_SUCCESS;
    RD_LGC_PO_CH *plgcpo = NULL;
    unsigned char *pTmpData = NULL;
    unsigned char *pSrcData = NULL;
    STATUS vxsts;
    uint16_t uCRCFst;
    uint16_t uCRCSec;
    uint16_t uCRCFstCalc=0;
    uint16_t uCRCSecCalc=0;
    FLT_U32_UNION ResultTmpArrayFst[MAXPONUM];		/* 最大50个电度量 */
    FLT_U32_UNION ResultTmpArraySec[MAXPONUM];			/* 最大50个电度量 */
    int uPoNumFst;
    int uPoNumSec;

    if (iLgcPoChNum_g == 0)
    {
        return EP_SUCCESS;
    }

#ifndef POFESTRAMMEM	/* 文件中保存 */

    lFile = open((char *)EP_PO_FILE, O_RDONLY, 0);
    if (lFile <= 0)
    {
        retcode = EP_ERROR;
        LOG_Dbg_Msg("Open Error!\n", 0, 0, 0, 0, 0, 0);

        return retcode;
    }

    lseek(lFile, 4*iLgcPoChNum_g+2, SEEK_SET);
    BufNum=read(lFile, aucBuf, 4);
    if (BufNum != 4)
    {
        if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "FRAM error(%02d)\n",PO_CHANGE,0);
            LOG_Write(LOG_KERNEL,"The number of PO change because of configuration updating.\n",NULL);
        }
        else if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "铁电存储器错误(%02d)\n", PO_CHANGE, 0);
            LOG_Write(LOG_KERNEL, "因配置更新PO配置个数改变\n", NULL);
        }
        close(lFile);

        return EP_ERROR;
    }

    uPoNumFst=U8_TO_U32(aucBuf[0], aucBuf[1], aucBuf[2], aucBuf[3]);
    if (uPoNumFst != iLgcPoChNum_g)
    {
        if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "FRAM error(%02d)\n",PO_CHANGE,0);
            LOG_Write(LOG_KERNEL,"The number of PO change because of configuration updating.\n",NULL);
        }
        else if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "铁电存储器错误(%02d)\n", PO_CHANGE, 0);
            LOG_Write(LOG_KERNEL, "因配置更新PO配置个数改变\n", NULL);
        }
        close(lFile);

        return EP_ERROR;
    }

    lseek(lFile, 4*iLgcPoChNum_g, SEEK_SET);
    BufNum=read(lFile, aucBuf, 2);
    assert (BufNum == 2);
    uCRCFst=U8_TO_U16(aucBuf[0], aucBuf[1]);

    lseek(lFile, 0, SEEK_SET);

    for (plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
    {
        BufNum=read(lFile, aucBuf, 4);
        assert (BufNum == 4);
        plgcpo->Val.ulVal=U8_TO_U32(aucBuf[0], aucBuf[1], aucBuf[2], aucBuf[3]);
        plgcpo->OriginVal.TotalEnergy = plgcpo->Val.TotalEnergy;		/* 原始值 */
        pTmpData=(unsigned char *)(&plgcpo->Val.ulVal);
        uCRCFstCalc=EP_CCITT_CRC16(pTmpData, 4, uCRCFstCalc);
    }

    if (uCRCFstCalc != uCRCFst)
    {
        /* CRC校验出错 */
        if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "FRAM error(%02d)\n", CRC_CHECK_ERR, 0);
            LOG_Write(LOG_KERNEL, "Fest RAM CRC check error.\n", NULL);
        }
        else if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "铁电存储器错误(%02d)\n", CRC_CHECK_ERR, 0);
            LOG_Write(LOG_KERNEL, "铁电CRC校验错误\n",NULL);
        }
        close(lFile);

        return EP_ERROR;
    }
    close(lFile);
#endif

#ifdef POFESTRAMMEM		/* 铁电中保存 */

    if (read_ram_data(4*iLgcPoChNum_g+2, (unsigned char *)&uPoNumFst, 4) != OK)
    {
        /* 个数 */
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "铁电存储器错误(%02d)\n", READING_ERR, 0);
            LOG_Write(LOG_KERNEL, "铁电第一存储区PO个数读出错误\n",NULL);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "FRAM error(%02d)\n", READING_ERR, 0);
            LOG_Write(LOG_KERNEL, "The number of PO in the first memory is error!\n", NULL);
        }
    }

    if (read_ram_data(0x1000+4*iLgcPoChNum_g+2, (unsigned char *)&uPoNumSec, 4) != OK)
    {
        /* 个数 */
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "铁电存储器错误(%02d)\n", READING_ERR, 0);
            LOG_Write(LOG_KERNEL, "铁电第二存储区PO个数读出错误\n",NULL);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "FRAM error(%02d)\n", READING_ERR, 0);
            LOG_Write(LOG_KERNEL, "The number of PO in the secend memory is error!\n", NULL);
        }
    }

    LOG_Dbg_Msg("uPoNumFst=%x, uPoNumSec=%x, iLgcPoChNum_g=%x\n", uPoNumFst, uPoNumSec, iLgcPoChNum_g, 0, 0, 0);

    if ((uPoNumFst != uPoNumSec) || (uPoNumFst != iLgcPoChNum_g) || (uPoNumSec != iLgcPoChNum_g))
    {
        if(ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "FRAM error(%02d)\n",PO_CHANGE,0);
            LOG_Write(LOG_KERNEL,"The number of PO change because of configuration updating.\n",NULL);
        }
        else if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "铁电存储器错误(%02d)\n", PO_CHANGE, 0);
            LOG_Write(LOG_KERNEL, "因配置更新PO配置个数改变\n", NULL);
        }

        bPofestramInitOK_s=TRUE;		/* 正常写，防止下次读出后仍错误 */

        return EP_ERROR;
    }

    if (read_ram_data(4*iLgcPoChNum_g, (unsigned char *)&uCRCFst, 2) != OK)
    {
        if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "FRAM error(%02d)\n", CRC_CHECK_ERR, 0);
            LOG_Write(LOG_KERNEL,"CRC in the first memory of Fest RAM is error!\n",NULL);
        }
        else if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "铁电存储器错误(%02d)\n", CRC_CHECK_ERR, 0);
            LOG_Write(LOG_KERNEL,"铁电第一存储区CRC校验错误\n",NULL);
        }
    }

    if (read_ram_data(0x1000+4*iLgcPoChNum_g, (unsigned char *)&uCRCSec, 2) != OK)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "铁电存储器错误(%02d)\n", CRC_CHECK_ERR, 0);
            LOG_Write(LOG_KERNEL,"铁电第二存储区CRC校验错误\n",NULL);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "FRAM error(%02d)\n", CRC_CHECK_ERR, 0);
            LOG_Write(LOG_KERNEL,"CRC in the secend memory of Fest RAM is error!\n",NULL);
        }
    }

    for (plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
    {
        pTmpData=(unsigned char *)(&ResultTmpArrayFst[i].ulVal);

        if (read_ram_data(4*i, pTmpData, 4) != OK)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "铁电存储器错误(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL,"铁电读出错误\n",NULL);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "FRAM error(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL,"Fest RAM reading error!\n",NULL);
            }
            continue;
        }
        uCRCFstCalc=EP_CCITT_CRC16(pTmpData, 4, uCRCFstCalc);
        taskDelay(1);
    }

    for (plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
    {
        pTmpData=(unsigned char *)(&ResultTmpArraySec[i].ulVal);

        if (read_ram_data(0x1000+4*i, pTmpData, 4) != OK)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "铁电存储器错误(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL,"铁电读出错误\n",NULL);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "FRAM error(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL,"Fest RAM reading error!\n",NULL);
            }
            continue;
        }
        uCRCSecCalc=EP_CCITT_CRC16(pTmpData, 4, uCRCSecCalc);
        taskDelay(1);
    }

    LOG_Dbg_Msg("uCRCFst=%x, uCRCSec=%x, uCRCFstCalc=%x, uCRCSecCalc=%x\n", uCRCFst, uCRCSec, uCRCFstCalc, uCRCSecCalc, 0, 0);

    if (uCRCFst == uCRCFstCalc)
    {
        for (plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
        {
            pTmpData=(unsigned char *)(&(plgcpo->Val.ulVal));
            if (FloatValidCheck(ResultTmpArrayFst[i].fVal) != EP_SUCCESS)
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "铁电存储器错误(%02d)\n",READING_ERR,0);
                    LOG_Write(LOG_KERNEL,"铁电读出错误\n",NULL);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "FRAM error(%02d)\n",READING_ERR,0);
                    LOG_Write(LOG_KERNEL,"Fest RAM reading error!\n",NULL);
                }
                continue;
            }

            pSrcData=(unsigned char *)(&ResultTmpArrayFst[i].ulVal);

            vxsts=taskLock();
            pTmpData[0]=pSrcData[0];		/* 写到目标 */
            pTmpData[1]=pSrcData[1];
            pTmpData[2]=pSrcData[2];
            pTmpData[3]=pSrcData[3];
            vxsts=taskUnlock();

            plgcpo->OriginVal.TotalEnergy = plgcpo->Val.TotalEnergy;		/* 原始值 */
        }
        retcode=EP_SUCCESS;
        bPofestramInitOK_s=TRUE;
        return retcode;
    }
    else
    {
    }

    if (uCRCSec == uCRCSecCalc)
    {
        for (plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
        {
            pTmpData=(unsigned char *)(&(plgcpo->Val.ulVal));
            if (FloatValidCheck(ResultTmpArraySec[i].fVal) != EP_SUCCESS)
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "铁电存储器错误(%02d)\n",READING_ERR,0);
                    LOG_Write(LOG_KERNEL,"铁电读出错误\n",NULL);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "FRAM error(%02d)\n",READING_ERR,0);
                    LOG_Write(LOG_KERNEL,"Fest RAM reading error!\n",NULL);
                }
                continue;
            }

            pSrcData=(unsigned char *)(&ResultTmpArraySec[i].ulVal);

            vxsts=taskLock();
            pTmpData[0]=pSrcData[0];		/* 写到目标 */
            pTmpData[1]=pSrcData[1];
            pTmpData[2]=pSrcData[2];
            pTmpData[3]=pSrcData[3];
            vxsts=taskUnlock();

            plgcpo->OriginVal.TotalEnergy = plgcpo->Val.TotalEnergy;		/* 原始值 */
        }
        retcode=EP_SUCCESS;
        bPofestramInitOK_s=TRUE;

        return retcode;
    }
    else
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "铁电存储区错误(%02d)\n",CRC_CHECK_ERR,0);
            LOG_Write(LOG_KERNEL,"铁电CRC校验错误!\n",NULL);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "FRAM error(%02d)\n", CRC_CHECK_ERR, 0);
            LOG_Write(LOG_KERNEL,"Fest RAM CRC check error!\n",NULL);
            //"Fest RAM CRC check error.\n, the value in Fest RAM is cleared.\n",0,0);
        }
        ResetFRAMVal();
        for (plgcpo = plgcpoch_g; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++)
        {
            /* 清零 */
            plgcpo->OriginVal.TotalEnergy=0;
            plgcpo->Val.TotalEnergy=0;
        }

        retcode=EP_ERROR;
    }
#endif

    if (retcode == EP_SUCCESS)
    {
        /* 返回 */
        bPofestramInitOK_s=TRUE;
    }

    return retcode;
}

/***********************************************************************
* FT_Temp_Name_New - 临时函数(防止修改filetool.c)
*
* RETURNS: 无
*
*/
void FT_Temp_Name_New(
    uint8_t *pucRslt,
    const uint8_t *strFile
)
{
    uint8_t *puc;

    assert(pucRslt);
    assert(strFile && strlen(strFile)<FULL_NAME_LEN);

    strncpy(pucRslt, strFile, FULL_NAME_LEN);
    pucRslt[FULL_NAME_LEN]='\0';

    /* Let's add a '#' just after the last '/'. */
    for (puc=pucRslt+strlen(pucRslt)-1; puc>=pucRslt; puc--)
    {
        if (*puc=='/')
            break;
    }
    assert(puc>=pucRslt);

    memmove(puc+2, puc+1, strlen(puc)+1);
    puc[1]='#';
}

/***********************************************************************
* RD_Is_Valid_Gain - 读取增益有效性文件
*
* RETURNS: 无
*
*/
BOOL RD_Is_Valid_Gain(int iFd)
{
    uint8_t aucBuf[10];
    int i;
    uint8_t aucFlag[256];

    assert(iFd>=0);

    lseek(iFd, -4, SEEK_END);

    if (read(iFd, aucBuf, 4)!=4 ||
            aucBuf[0]!=0xA8 || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x57)
        return FALSE;

    lseek(iFd, 0, SEEK_SET);

    if (read(iFd, aucBuf, 10)!=10 ||
            aucBuf[0]!=0xA2 || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x5D|| aucBuf[9]!=iHwAiChNum_g)
        return FALSE;

    memset(aucFlag, 0, sizeof(aucFlag));
    for (i=0; i<iHwAiChNum_g; i++)
    {
        if (read(iFd, aucBuf, 9)==9 && aucBuf[0]<iHwAiChNum_g &&
                fabs(BYTES_TO_FLT(aucBuf+5))>FLT_PRECISION && !aucFlag[aucBuf[0]])
            aucFlag[aucBuf[0]]=0xFF;
        else
            return FALSE;
    }

    return TRUE;
}

/***********************************************************************
* WR_Cfg_Hw_AI_Gain - 保存物理通道增益系数，MMI调用
*
* RETURNS: 无
*
*/
EP_STATUS WR_Cfg_Hw_AI_Gain(int nType)
{
    uint8_t aucTempFile[FULL_NAME_LEN+1];
    int i;
    int iFd;
    uint8_t aucBuf[6];
    EP_STATUS sts;
    RD_HW_AI_CH *pch;

    uint16_t ulCrc=0;

    sts = EP_SUCCESS;
    FT_Temp_Name_New(aucTempFile, EP_AI_GAIN_FILE);

    iFd=FT_Bgn_Update(EP_AI_GAIN_FILE);
    assert(iFd>=0);

    aucBuf[0]=0xA2;		/* 文件头符 */
    aucBuf[1]=0x0;
    aucBuf[2]=0x0;
    aucBuf[3]=0x5D;
    i=write(iFd, aucBuf, 4);
    assert(i==4);
    ulCrc =EP_CCITT_CRC16(aucBuf,4,ulCrc);

    aucBuf[0]=0x01;		/* 版本号 */
    aucBuf[1]=0x10;
    i=write(iFd, aucBuf, 2);
    assert(i==2);
    ulCrc =EP_CCITT_CRC16(aucBuf,2,ulCrc);


    aucBuf[0]=0x0;		/* 保留 */
    aucBuf[1]=0x0;
    aucBuf[2]=0x0;
    i=write(iFd, aucBuf, 3);
    assert(i==3);
    ulCrc =EP_CCITT_CRC16(aucBuf,3,ulCrc);

    aucBuf[0] = iHwAiChNum_g;		/* 物理通道总数 */
    i=write(iFd, aucBuf, 1);
    assert(i==1);
    ulCrc =EP_CCITT_CRC16(aucBuf,1,ulCrc);

    LOG_Dbg_Msg("Begin to calibrate the gain coefficient!\n", 0, 0, 0, 0, 0, 0);
    for (pch=phwaich_g; pch<phwaich_g+iHwAiChNum_g; pch++)
    {
        taskDelay(1);		/* 延时 */
        aucBuf[0] = pch-phwaich_g;
        i=write(iFd, aucBuf, 1);
        assert(i==1);
        ulCrc =EP_CCITT_CRC16(aucBuf,1,ulCrc);

        FLT_TO_BYTES(aucBuf, pch->fExcCoff);		/* 偏置系数 */
        i=write(iFd, aucBuf, 4);
        assert(i==4);
        ulCrc =EP_CCITT_CRC16(aucBuf,4,ulCrc);

        FLT_TO_BYTES(aucBuf, pch->fGain);
        i=write(iFd, aucBuf, 4);
        assert(i==4);
        ulCrc =EP_CCITT_CRC16(aucBuf,4,ulCrc);
    }

    aucBuf[0]=0xA8;		/* 保留 */
    aucBuf[1]=0x0;
    aucBuf[2]=0x0;
    aucBuf[3]=0x57;
    i=write(iFd, aucBuf, 4);
    assert(i==4);
    ulCrc =EP_CCITT_CRC16(aucBuf,4,ulCrc);


    sts=FT_End_Update(EP_AI_GAIN_FILE, iFd);
    assert(sts==EP_SUCCESS);

    sts=FT_Wr_INI_CRC("[CRC]",CRC_ITEM_HDCOF,ulCrc);
    assert (sts != EP_ERROR);

    if(nType == 0)
    {
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_OPRATE, "创建新的物理通道增益系数文件!\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_OPRATE, "The new gain coefficient file of phisical channel is created!\n", NULL);
        }
        UpdateFPGASmvSendCoff();/*更新FPGA采样转发系数*/
        UpdateAcCoff();	/* 更新变比系数 */
    }
    else if(nType == 1)
    {
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_OPRATE, "创建新的物理通道偏置系数文件!\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_OPRATE, "The new offset coefficient file of phisical channel is created!\n", NULL);
        }
    }
    else
        assert(FALSE);

    return sts;
}

/***********************************************************************
* Reset_Cfg_Hw_AI_Gain - 保存物理通道增益系数，MMI调用
*
* RETURNS: 无
*
*/
EP_STATUS Reset_Cfg_Hw_AI_Gain(void)
{
    uint8_t aucTempFile[FULL_NAME_LEN+1];
    int i;
    int iFd;
    uint8_t aucBuf[6];
    EP_STATUS sts;
    RD_HW_AI_CH *pch;

    sts = EP_SUCCESS;
    FT_Temp_Name_New(aucTempFile, EP_AI_GAIN_FILE);

    iFd=FT_Bgn_Update(EP_AI_GAIN_FILE);
    assert(iFd>=0);

    aucBuf[0]=0xA2;		/* 文件头符 */
    aucBuf[1]=0x0;
    aucBuf[2]=0x0;
    aucBuf[3]=0x5D;
    i=write(iFd, aucBuf, 4);
    assert(i==4);

    aucBuf[0]=0x01;		/* 版本号 */
    aucBuf[1]=0x10;
    i=write(iFd, aucBuf, 2);
    assert(i==2);

    aucBuf[0]=0x0;		/* 保留 */
    aucBuf[1]=0x0;
    aucBuf[2]=0x0;
    i=write(iFd, aucBuf, 3);
    assert(i==3);

    aucBuf[0] = iHwAiChNum_g;		/* 物理通道总数 */
    i=write(iFd, aucBuf, 1);
    assert(i==1);

    LOG_Dbg_Msg("Begin to reset the gain coefficient!\n", 0, 0, 0, 0, 0, 0);
    for (pch=phwaich_g; pch<phwaich_g+iHwAiChNum_g; pch++)
    {
        taskDelay(1);		/* 延时 */
        aucBuf[0] = pch-phwaich_g;
        i=write(iFd, aucBuf, 1);
        assert(i==1);

        FLT_TO_BYTES(aucBuf, 0.0);
        i=write(iFd, aucBuf, 4);
        assert(i==4);

        FLT_TO_BYTES(aucBuf, 1.0);
        i=write(iFd, aucBuf, 4);
        if(i != 4);
    }

    aucBuf[0]=0xA8;		/* 保留 */
    aucBuf[1]=0x0;
    aucBuf[2]=0x0;
    aucBuf[3]=0x57;
    i=write(iFd, aucBuf, 4);
    assert(i==4);

    sts=FT_End_Update(EP_AI_GAIN_FILE, iFd);
    assert(sts==EP_SUCCESS);

    return sts;
}

/***********************************************************************
* RD_Ck_Coff - 系数检查
*
* RETURNS:
*			EP_SUCCESS, 正常
*			EP_BUF_ERR, 错误
*/
EP_STATUS RD_Ck_Coff(void)
{
    RD_LGC_AI_CH *paich;
    RD_HW_AO_CH *paoch;
    BOOL bUsed;
    EP_STATUS stsRet;

    stsRet = EP_SUCCESS;

    for(paich=plgcaich_g; paich<plgcaich_g+iLgcAiChNum_g; paich++)
    {
        /* AI修改 */
        if((paich->phwai->FactorSetModWord)&0x01)
        {
            /* 最大值*/
            stsRet = SC_Find_Setbase(paich->phwai->MaxValueDingzhiTagBase, &bUsed);
            if(stsRet!=EP_SUCCESS)
            {
                logMsg("Error, No %d logic ai Maxsetbase %s  isn't cofigured in sy set !\n",
                       paich-plgcaich_g,(int)paich->phwai->MaxValueDingzhiTagBase,0,0,0,0);
                return EP_CFG_ERR;
            }
        }
        if((paich->phwai->FactorSetModWord)&0x02)
        {
            /* 最小值*/
            stsRet = SC_Find_Setbase(paich->phwai->MinValueDingzhiTagBase, &bUsed);
            if(stsRet!=EP_SUCCESS)
            {
                logMsg("Error, No %d logic ai Minsetbase %s  isn't cofigured in sy set !\n",
                       paich-plgcaich_g,(int)paich->phwai->MinValueDingzhiTagBase,0,0,0,0);
                return EP_CFG_ERR;
            }
        }
        if((paich->phwai->FactorSetModWord)&0x04)
        {
            /* 增益系数 */
            stsRet = SC_Find_Setbase(paich->phwai->ScalefactorDingzhiTagBase, &bUsed);
            if(stsRet!=EP_SUCCESS)
            {
                logMsg("Error, No %d logic ai scalefactor setbase %s  isn't cofigured in sy set !\n",
                       paich-plgcaich_g,(int)paich->phwai->ScalefactorDingzhiTagBase,0,0,0,0);
                return EP_CFG_ERR;
            }
        }
    }

    for(paoch=phwaoch_g; paoch<phwaoch_g+iHwAoChNum_g; paoch++)
    {
        /* AO修改 */
        if((paoch->FactorSetModWord)&0x01)
        {
            /* 最大值*/
            stsRet =SC_Find_Setbase(paoch->MaxValueDingzhiTagBase, &bUsed);
            if(stsRet!=EP_SUCCESS)
            {
                logMsg("Error, No %d ao Maxsetbase %s  isn't cofigured in sy set !\n",
                       paoch-phwaoch_g,(int)paoch->MaxValueDingzhiTagBase,0,0,0,0);
                return EP_CFG_ERR;
            }
        }
        if((paoch->FactorSetModWord)&0x02)
        {
            /* 最小值*/
            stsRet = SC_Find_Setbase(paoch->MinValueDingzhiTagBase, &bUsed);
            if(stsRet!=EP_SUCCESS)
            {
                logMsg("Error, No %d ao Minsetbase %s  isn't cofigured in sy set !\n",
                       paoch-phwaoch_g,(int)paoch->MinValueDingzhiTagBase,0,0,0,0);
                return EP_CFG_ERR;
            }
        }
        if((paoch->FactorSetModWord)&0x04)
        {
            /* 增益系数 */
            stsRet = SC_Find_Setbase(paoch->ScalefactorDingzhiTagBase, &bUsed);
            if(stsRet!=EP_SUCCESS)
            {
                logMsg("Error, No %d ao scalefactor setbase %s  isn't cofigured in sy set !\n",
                       paoch-phwaoch_g,(int)paoch->ScalefactorDingzhiTagBase,0,0,0,0);
                return EP_CFG_ERR;
            }
        }
    }

    return EP_SUCCESS;
}
#if 0
/***********************************************************************
* RecBufInit - 小电流接地缓冲区初始化
*
* RETURNS:
*			EP_SUCCESS, 正常
*			EP_BUF_ERR, 错误
*/
EP_STATUS RecBufInit(void)
{
    RD_HW_AI_CH *pch;
    EP_STATUS stsRet;

    stsRet=EP_SUCCESS;
    AnalogBufHandle.EnableFlag=TRUE;
    AnalogBufHandle.iItemNum=TRANSCHNNUM;
    AnalogBufHandle.iCycleNum=TRANSCYCLENUM;
    AnalogBufHandle.iCycleSamNum=uiAiPts_g;
    AnalogBufHandle.iRecPointNum = TRANSCYCLENUM*uiAiPts_g;

    AnalogBufHandle.AnalogBufInfo[0].m_unSN=0;

    for (pch=phwaich_g; pch<phwaich_g+iHwAiChNum_g; pch++)
    {
        if (pch->ucModCh == 0)
        {
            /* A相电流*/
            AnalogBufHandle.AnalogBufInfo[0].m_ucType=pch->ucUnit;
            AnalogBufHandle.AnalogBufInfo[0].ucHdCh = pch->ucModCh;
            strcpy(AnalogBufHandle.AnalogBufInfo[0].m_pcName, pch->aucId);
            break;
        }
    }
    if(pch == (phwaich_g+iHwAiChNum_g))
    {
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  physical AI.\nMust configure \"%s\" for power line protect!\n",
                    (int)"A相电流", (int)"A相电流", 0, 0, 0, 0);

        /* assert(FALSE); */
        AnalogBufHandle.EnableFlag=FALSE;
        return EP_SUCCESS;
    }

    AnalogBufHandle.AnalogBufInfo[1].m_unSN=1;

    for (pch=phwaich_g; pch<phwaich_g+iHwAiChNum_g; pch++)
    {
        if (pch->ucModCh == 1)
        {
            AnalogBufHandle.AnalogBufInfo[1].m_ucType=pch->ucUnit;
            AnalogBufHandle.AnalogBufInfo[1].ucHdCh = pch->ucModCh;
            strcpy(AnalogBufHandle.AnalogBufInfo[1].m_pcName, pch->aucId);
            break;
        }
    }
    if(pch == (phwaich_g+iHwAiChNum_g))
    {
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  physical AI.\nMust configure \"%s\" for power line protect!\n",
                    (int)"B相电流", (int)"B相电流", 0, 0, 0, 0);
        /* assert(FALSE); */
        AnalogBufHandle.EnableFlag=FALSE;
        return EP_SUCCESS;
    }

    AnalogBufHandle.AnalogBufInfo[2].m_unSN=2;

    for (pch=phwaich_g; pch<phwaich_g+iHwAiChNum_g; pch++)
    {
        if (pch->ucModCh == 2)
        {
            AnalogBufHandle.AnalogBufInfo[2].m_ucType=pch->ucUnit;
            AnalogBufHandle.AnalogBufInfo[2].ucHdCh = pch->ucModCh;
            strcpy(AnalogBufHandle.AnalogBufInfo[2].m_pcName, pch->aucId);
            break;
        }
    }
    if(pch == (phwaich_g+iHwAiChNum_g))
    {
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  physical AI.\nMust configure \"%s\" for power line protect!\n",
                    (int)"C相电流", (int)"C相电流", 0, 0, 0, 0);
        /* assert(FALSE); */
        AnalogBufHandle.EnableFlag=FALSE;
        return EP_SUCCESS;
    }

    AnalogBufHandle.AnalogBufInfo[3].m_unSN=3;

    for (pch=phwaich_g; pch<phwaich_g+iHwAiChNum_g; pch++)
    {
        if (pch->ucModCh == 3)
        {
            AnalogBufHandle.AnalogBufInfo[3].m_ucType=pch->ucUnit;
            AnalogBufHandle.AnalogBufInfo[3].ucHdCh = pch->ucModCh;
            strcpy(AnalogBufHandle.AnalogBufInfo[3].m_pcName, pch->aucId);
            break;
        }
    }
    if(pch == (phwaich_g+iHwAiChNum_g))
    {
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  physical AI.\nMust configure \"%s\" for power line protect!\n",
                    (int)"零序电流", (int)"零序电流", 0, 0, 0, 0);
        /* assert(FALSE); */
        AnalogBufHandle.EnableFlag=FALSE;
        return EP_SUCCESS;
    }

    AnalogBufHandle.AnalogBufInfo[4].m_unSN=4;

    for (pch=phwaich_g; pch<phwaich_g+iHwAiChNum_g; pch++)
    {
        if (pch->ucModCh == 4)
        {
            AnalogBufHandle.AnalogBufInfo[4].m_ucType=pch->ucUnit;
            AnalogBufHandle.AnalogBufInfo[4].ucHdCh = pch->ucModCh;
            strcpy(AnalogBufHandle.AnalogBufInfo[4].m_pcName, pch->aucId);
            break;
        }
    }
    if(pch == (phwaich_g+iHwAiChNum_g))
    {
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  physical AI.\nMust configure \"%s\" for power line protect!\n",
                    (int)"A相电压", (int)"A相电压", 0, 0, 0, 0);
        /* assert(FALSE); */
        AnalogBufHandle.EnableFlag=FALSE;
        return EP_SUCCESS;
    }

    AnalogBufHandle.AnalogBufInfo[5].m_unSN=5;

    for (pch=phwaich_g; pch<phwaich_g+iHwAiChNum_g; pch++)
    {
        if (pch->ucModCh == 5)
        {
            AnalogBufHandle.AnalogBufInfo[5].m_ucType=pch->ucUnit;
            AnalogBufHandle.AnalogBufInfo[5].ucHdCh = pch->ucModCh;
            strcpy(AnalogBufHandle.AnalogBufInfo[5].m_pcName, pch->aucId);
            break;
        }
    }
    if(pch == (phwaich_g+iHwAiChNum_g))
    {
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  physical AI.\nMust configure \"%s\" for power line protect!\n",
                    (int)"B相电压", (int)"B相电压", 0, 0, 0, 0);
        /* assert(FALSE); */
        AnalogBufHandle.EnableFlag=FALSE;
        return EP_SUCCESS;
    }

    AnalogBufHandle.AnalogBufInfo[6].m_unSN=6;

    for (pch=phwaich_g; pch<phwaich_g+iHwAiChNum_g; pch++)
    {
        if (pch->ucModCh == 6)
        {
            AnalogBufHandle.AnalogBufInfo[6].m_ucType=pch->ucUnit;
            AnalogBufHandle.AnalogBufInfo[6].ucHdCh = pch->ucModCh;
            strcpy(AnalogBufHandle.AnalogBufInfo[6].m_pcName, pch->aucId);
            break;
        }
    }
    if(pch == (phwaich_g+iHwAiChNum_g))
    {
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\"  physical AI.\nMust configure \"%s\" for power line protect!\n",
                    (int)"C相电压", (int)"C相电压", 0, 0, 0, 0);
        /* assert(FALSE); */
        AnalogBufHandle.EnableFlag=FALSE;
        return EP_SUCCESS;
    }

    AnalogBufHandle.AnalogBufInfo[7].m_unSN=7;
    AnalogBufHandle.AnalogBufInfo[7].m_ucType = AnalogBufHandle.AnalogBufInfo[4].m_ucType;		/* 使用A相电压的单位 */
    strcpy(AnalogBufHandle.AnalogBufInfo[7].m_pcName, "零序电压");

    return EP_SUCCESS;
}
#endif
/***********************************************************************
* TestNestStep - 下一步测试
*
* RETURNS: 无
*
*/
void TestNestStep(void)
{
    bStopRefreshData = FALSE;
    aimodDsp_g.ulHeadClk = Sam_Counter-1;
}

/***********************************************************************
* FileToolTest - 文件测试
*
* RETURNS: 无
*
*/
void FileToolTest(void)
{
    int iFd;

    iFd=FT_Bgn_Update("/data/datatemp0");
    printf("%d\n", iFd);
}

/***********************************************************************
* FloatValidCheck - 浮点有效检查
*
* RETURNS: 无
*
*/
EP_STATUS FloatValidCheck(
    float fVal
)
{
    FLT_U32_UNION FltU32Val;
    EP_STATUS stsRet;

    stsRet=EP_SUCCESS;
    FltU32Val.fVal=fVal;
    if((FltU32Val.ulVal&0x80000000) && ((FltU32Val.ulVal&0x7F800000) == 0x7F800000) && ((FltU32Val.ulVal&0x007FFFFF) == 0x0))
    {
        logMsg("浮点异常!为正无穷大数!\n", 0, 0, 0, 0, 0, 0);
        stsRet=EP_ERROR;
    }
    else if(!(FltU32Val.ulVal&0x80000000) && ((FltU32Val.ulVal&0x7F800000) == 0x7F800000) && ((FltU32Val.ulVal&0x007FFFFF) == 0x0))
    {
        logMsg("浮点异常!为负无穷大数!\n", 0, 0, 0, 0, 0, 0);
        stsRet=EP_ERROR;
    }
    else if(((FltU32Val.ulVal&0x7F800000) == 0x7F800000) && ((FltU32Val.ulVal&0x007FFFFF) != 0x0))
    {
        logMsg("浮点异常!非数!\n", 0, 0, 0, 0, 0, 0);
        stsRet=EP_ERROR;
    }

    return stsRet;
}

/***********************************************************************
* FloatValidCheckTest - 浮点有效检查测试
*
* RETURNS: 无
*
*/
void FloatValidCheckTest(
    uint32_t ulVal
)
{
    FLT_U32_UNION FltU32Val;

    FltU32Val.ulVal=ulVal;
    FloatValidCheck(FltU32Val.fVal);
}

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
)
{
    DA_AO_CFG *pAoCfg;

    pAoCfg=&(PartAoCfgDa_g.aDaAoCfg_g[PartAoCfgDa_g.iDaAISrcAONum+PartAoCfgDa_g.iDaMidSrcAONum]);
    pAoCfg->iAOSrcType=iSrcType;
    assert(uiCh<PartAoCfgDa_g.iDaAONum);
    pAoCfg->ucAOHdCh=uiCh;
    pAoCfg->pElemSrc=pElemIOSrc;
    pAoCfg ->fCoff = fCoff;
    PartAoCfgDa_g.iDaMidSrcAONum++;
    assert((PartAoCfgDa_g.iDaAISrcAONum+PartAoCfgDa_g.iDaMidSrcAONum)<=PartAoCfgDa_g.iDaAONum);

    return  EP_SUCCESS;
}

/***********************************************************************
* GetAiCntPeriod - 获取采样周期，单位为us
*
* RETURNS: 无
*
*/
float GetAiCntPeriod(void)
{
    return 1000000.0/uiAiRate_g;
}

/* Get the AC module type.
 * Para:
 *     NONE.
 * Return:
 *     >=0, 索引定值页序.
 *     -1, error.
*/
int32_t EP_GetAcMdType(void)
{
    uint8_t aucBuf[30];
    int iRst;
    int32_t uiMdType;

    iRst=FT_Rd_Sys_INI("[SYSTEM]", "AcMdType", aucBuf, 30);

    /* 读出有效值 */
    if (iRst == 1)
    {
        uiMdType=atoi(aucBuf);

        if (uiMdType >= 0)
        {
            return uiMdType;
        }
        else
        {
            /* 读出值无效 */
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "因[SYSTEM] AcMdType值无效,创建新的系统INI文件.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_KERNEL, "Because of invalid [SYSTEM] AcMdType, create new INI file.\n", NULL);
            }

            FT_New_SYS_INI_File();

            goto reterr;
        }
    }
    else if(iRst == 0)
    {
        /* 无该值 */
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "因[SYSTEM] AcMdType值读取失败,创建新的系统INI文件.\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Because of failing ro read [SYSTEM] AcMdType, create new INI file.\n", NULL);
        }

        FT_New_SYS_INI_File();

        goto reterr;
    }
    else if(iRst>1)
    {
        /* 多于一个值 */
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "模件类型设置项多于1个.\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "AC module type item is more than 1.\n", NULL);
        }

        goto reterr;
    }
    else if(iRst == EP_ERROR)
    {
        /* 读出失败,如无INI文件 */
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "系统文件读取失败!\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Read system file error!\n", NULL);
        }

        goto reterr;
    }


reterr:

    return -1;
}

/***********************************************************************
* EP_GetAcMdTypeChgFlag - judge if the AC mould type changed
*
* RETURNS: TRUE, or FALSE
*
*
*/
BOOL EP_GetAcMdTypeChgFlag(void)
{
    uint8_t aucBuf[31];
    int iRst;
    int32_t uiResult;

    iRst=FT_Rd_Sys_INI("[SYSTEM]", "AcMdTypeChgFlag", aucBuf, 30);

    if (iRst == 1)
    {
        uiResult=atoi(aucBuf);

        if(uiResult == 1)
        {
            /* changed */
            EP_SetAcMdTypeChgFlag(0);		/* Clear flag */

            return TRUE;
        }
        else if(uiResult == 0)
        {
            return FALSE;
        }
    }
    else if(iRst == 0)
    {
        /* 重新生成系统文件 */
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "因[SYSTEM] AcMdTypeChgFlag值读取失败，创建新的系统INI文件.\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Because of failing ro read [SYSTEM] AcMdTypeChgFlag, create new ini file.\n", NULL);
        }

        FT_New_SYS_INI_File();

        goto reterr;
    }
    else if(iRst>1)
    {
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "找到系统设置文件，但模件类型设置项多于1个!\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Header was found, but AC module type item is more than 1!\n", NULL);
        }

        goto reterr;
    }
    else if(iRst == EP_ERROR)
    {
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "系统文件读取失败!\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Read system file error!\n", NULL);
        }

        goto reterr;
    }


reterr:

    return FALSE;
}

/***********************************************************************
* EP_SetAcMdType - Set the AC mould type
*
* RETURNS:
*               EP_SUCCESS: Normal
*               EP_ERROR: Error
*
*
*/
EP_STATUS EP_SetAcMdType(
    int32_t uAcMdType			/* Mould type used */
)
{
    uint8_t aucBuf[128];

    sprintf(aucBuf,"%d", (int)uAcMdType);

    if(FT_Wr_Sys_INI("[SYSTEM]", "AcMdType", aucBuf)<0)
    {
        if(ENG_MODE == 0)
        {
            logMsg("保存设置的模件类型到文件失败!\n", 0, 0, 0, 0, 0, 0);
        }
        else if(ENG_MODE == 1)
        {
            logMsg("Saving the mould type to file is error!\n", 0, 0, 0, 0, 0, 0);
        }

        return EP_ERROR;
    }

    return EP_SUCCESS;
}

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
)
{
    uint8_t aucBuf[128];

    sprintf(aucBuf,"%d", (int)uAcMdTypeChgFlag);

    if(FT_Wr_Sys_INI("[SYSTEM]", "AcMdTypeChgFlag", aucBuf)<0)
    {
        return EP_ERROR;
    }

    return EP_SUCCESS;
}

/* 切换交流模件类型
 * Para:
 *     iMdType, 模件类型,即索引定值页序.
 * Return:
 *     NONE.
*/
void EP_ChgAcMdType(int32_t iMdType)
{
    SLOW_MESSAGE_NODE Info;

    taskLock();

    /* 页序越限 */
    if (iMdType >= AdMdType.iMaxTypeNum)
    {
        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_OPRATE, "索引定值页序越限.\n", NULL);
        }
        else if (ENG_MODE == 1)
        {
            LOG_Write(LOG_OPRATE, "Index setting page number overflow.\n", NULL);
        }
        taskUnlock();

        return;
    }

    /* 当前设置类型与使用类型一致,同时类型有效 */
    if ((AdMdType.iCurrentType == iMdType) && AdMdType.bValid)
    {
        char TempInfo[256];

        if (ENG_MODE == 0)
        {
            sprintf(TempInfo, "当前定值页序为%d!!", (int)iMdType);
        }
        else if(ENG_MODE == 1)
        {
            sprintf(TempInfo, "The current AC Module type is %d!!", (int)iMdType);
        }
        LOG_Write(LOG_OPRATE, TempInfo, NULL);
        taskUnlock();

        return;
    }
    AdMdType.iCurrentType=iMdType;
    AdMdType.bChgFlag = TRUE;  /* 立即生效 */

    /* 通过向慢速任务发消息把索引定值页写入edp01.ini文件
     * 所有平台一样处理
     */
    Info.type=ACMDCHG;
    Info.param1=iMdType;
    msgQSend(SlowMessage, (char *)&Info, sizeof(SLOW_MESSAGE_NODE), NO_WAIT, MSG_PRI_NORMAL);

    taskUnlock();
}

/***********************************************************************
* App_GetAcMdType -获取交流模件类型
*
* RETURNS: 模件类型，根据索引定值所配确定，如第0页: 1A；第1页: 5A
*
*/
int32_t App_GetAcMdType(void)
{
    return AdMdType.iCurrentType;
}

/***********************************************************************
* GetBaseUnitFstRatedVal - 获取基准单元的1次额定值
*
* RETURNS: 无
*
*/
uint16_t GetBaseUnitFstRatedVal(void)
{
    return uBaseUnitFstRatedVal_g;
}

/***********************************************************************
* GetBaseUnitSecRatedVal - 获取基准单元的2次额定值
*
* RETURNS: 无
*
*/
uint16_t GetBaseUnitSecRatedVal(void)
{
    return uBaseUnitSecRatedVal_g;
}

/***********************************************************************
* GetChnRatedVal - 获取通道额定值
*
* RETURNS: 无
*
*/
void GetChnRatedVal(
    void *pvLgcAiHnd,				/* 句柄 */
    uint32_t *pFstRatedVal, 			/* 一次额定值 */
    uint16_t *pSecRatedVal							/* 二次值 */
)
{
    RD_LGC_AI_CH *plgcai; /* 逻辑通道配置*/

    assert(pvLgcAiHnd);
    plgcai=(RD_LGC_AI_CH*)pvLgcAiHnd;

    *pFstRatedVal=plgcai->phwai->uFstRatedVal;
    *pSecRatedVal=plgcai->phwai->uSecRatedVal;
}

/* 测试代码 */

/***********************************************************************
* ShowChnRatedVal - 显示通道额定值
*
* RETURNS: 无
*
*/
void ShowChnRatedVal(void)
{
    RD_LGC_AI_CH *pch;
    uint32_t FstRatedVal; 			/* 一次额定值 */
    uint16_t SecRatedVal;							/* 二次值 */

    LOG_Dbg_Msg("基准一次额定%d，基准二次额定%d\n", GetBaseUnitFstRatedVal(), GetBaseUnitSecRatedVal(), 0, 0, 0, 0);

    for (pch=plgcaich_g; pch<plgcaich_g+iLgcAiChNum_g; pch++)
    {
        GetChnRatedVal(pch, &FstRatedVal, &SecRatedVal);
        LOG_Dbg_Msg("通道%s一次额定值为%d，二次额定值为%d\n", (int)pch->aucId, (int)FstRatedVal, (int)SecRatedVal, 0, 0, 0);
    }
}

/***********************************************************************
* GetCurSysFreq - 获取当前系统频率
*
* RETURNS: 当前系统频率
*
*/
u_int GetCurSysFreq(void)
{
    return uiPwrFreq_g;
}

/***********************************************************************
* GetAICoff - 获取AI通道相关系数，MMI调用
*
* RETURNS: NONE
*
*/
void GetAICoff(
    RD_AIO_HW_COFF *pAiCoff		/* 分配iHwAiChNum_g个 */
)
{
    RD_HW_AI_CH *pch;
    STATUS vxsts;

    assert(pAiCoff);
    vxsts=taskLock();
    assert(vxsts==OK);

    for (pch=phwaich_g;
            pch<phwaich_g+iHwAiChNum_g; pch++, pAiCoff++)
    {
        pAiCoff->fMaxVal=pch->fMaxVal;
        pAiCoff->fMinVal=pch->fMinVal;
        pAiCoff->fCoff=pch->fCoff;
        pAiCoff->fExcCoff=pch->fExcCoff;
    }

    vxsts=taskUnlock();
}

/***********************************************************************
* GetAOCoff - 获取AO通道相关系数，MMI调用
*
* RETURNS: NONE
*
*/
void GetAOCoff(
    RD_AIO_HW_COFF *pAoCoff		/* 分配iHwAoChNum_g个 */
)
{
    RD_HW_AO_CH *pch;
    STATUS vxsts;

    assert(pAoCoff);
    vxsts=taskLock();
    assert(vxsts==OK);

    for (pch=phwaoch_g;
            pch<phwaoch_g+iHwAoChNum_g; pch++, pAoCoff++)
    {
        pAoCoff->fMaxVal=pch->fMaxVal;
        pAoCoff->fMinVal=pch->fMinVal;
        pAoCoff->fCoff=pch->fCoff;
    }

    vxsts=taskUnlock();
}

/***********************************************************************
* GetDiChgTime - 获取DI变位时间
*
* RETURNS: 变位时间，单位为us
*
*/
uint32_t GetDiChgTime(
    void *pSrc		/* DI句柄 */
)
{
    RD_LGC_DI_CH *plgcdi;

    plgcdi=(RD_LGC_DI_CH *)pSrc;

    if ((plgcdi->mod == RD_HDL_BOX_DI) || (plgcdi->iVal& 0x8000))
        return plgcdi->ulChgTime;
    else
        return SIO_GetDiChgTime(plgcdi->pvSrc);
}

/***********************************************************************
* GetDiChgPacketTime - 获取DI变位报文到达时间
*
* RETURNS: 变位时间，单位为us
*
*/
uint32_t GetDiChgPacketTime(
    void *pSrc		/* DI句柄 */
)
{
    RD_LGC_DI_CH *plgcdi;

    plgcdi=(RD_LGC_DI_CH *)pSrc;

    if(plgcdi->mod==RD_HDL_BOX_DI)
        return plgcdi->ulChgPackTime;
    else
        return SIO_GetDiChgTime(plgcdi->pvSrc);
}

/***********************************************************************
* 功能:通过传递指针的方式获取DI变位UTC时间
* 参数:
*       pSrc, DI句柄;
*       pDestUtcTime, 传递UTC时间的指针;
* 返回:
*       无.
*/
void GetDiChgUTCTimeByPtr(
    void *pSrc,	/* DI句柄 */
    uint64_t *pDestUtcTime
)
{
    RD_LGC_DI_CH *plgcdi;
    uint64_t ullusCntFrom1970 = 0;

    plgcdi=(RD_LGC_DI_CH *)pSrc;

    if(plgcdi->mod==RD_HDL_BOX_DI)
    {
        memcpy(pDestUtcTime, &plgcdi->utChgTime.ullusCntFrom1970, sizeof(uint64_t));
    }
    else
    {
        ullusCntFrom1970 = SIO_GetDiChgUTCTime(plgcdi->pvSrc);
        memcpy(pDestUtcTime, &ullusCntFrom1970, sizeof(uint64_t));
    }
    return;
}

/***********************************************************************
* GetDiQuality - 获取DI品质(应用调用)
*
* RETURNS: 品质因数
*
*/
uint16_t GetDiQuality(
    void *pSrc		/* DI句柄 */
)
{
    RD_LGC_DI_CH *plgcdi;

    plgcdi=(RD_LGC_DI_CH *)pSrc;

    /* 无效句柄, 返回有效（全为0） */
    if (!((plgcdi >= plgcdich_g) && (plgcdi <= (plgcdich_g+iLgcDiChNum_g-1))))
    {
        return 0;
    }

    return plgcdi->usQuality;
}

/* get the information of module.
 * Para:
 *     pPara, result.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS RD_GetModInfo(uint8_t **pPara)
{
    uint8_t *p;
    uint8_t *ptemp;
    uint16_t len;
    RD_PART_INFO *p_part;

    p = *pPara;
    p += 4;
    *p++ = (uint8_t)rdinfo_g.unPartNum;
    p += 8;

    for (p_part=apartinf_g; p_part<apartinf_g+rdinfo_g.unPartNum; p_part++)
    {
        len = 0;
        ptemp = p;		/* length of every module. */
        p += 2;
        *p = strlen(p_part->aucId);
        memcpy(p+1, p_part->aucId, *p);
        p += *p+1;
        U32_TO_BYTES(p, p_part->servertype);
        p += 4;
        p += 16;
        len = p - ptemp;
        *ptemp++ = LO8(len);
        *ptemp++ = HI8(len);
    }

    *pPara = p;

    return EP_SUCCESS;
}

/* get the information of channel.
 * Para:
 *     pname, name of module.
 *     pPara, result.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS RD_GetChnInfo(uint8_t *pname, uint8_t **pPara)
{
    uint8_t *p;
    RD_PART_INFO *p_part;
    RD_PART_INFO *p_temp_part = NULL;
    uint8_t findnum = 0;
    int16_t chnnum;
    RD_HW_AI_CH *phwai;
    RD_HW_AO_CH *phwao;
    RD_LGC_DI_CH *pdi;
    RD_LGC_DO_CH *pdo;

    p = *pPara;
    p += 4;
    *p = strlen(pname);

    memcpy(p+1, pname, *p);
    p += *p+1+8;

    for (p_part=apartinf_g; p_part<apartinf_g+rdinfo_g.unPartNum; p_part++)
    {
        if (!strcmp(pname, p_part->aucId))
        {
            p_temp_part = p_part;
            findnum++;
        }
    }

    if ((findnum == 0) || (findnum>1))
    {
        return EP_ERROR;
    }

    p_part = p_temp_part;
    chnnum = p_part->ucAiNum+p_part->ucAoNum+p_part->ucDiNum+p_part->ucDoNum;
    *p++ = LO8(chnnum);
    *p++ = HI8(chnnum);

    if (p_part->ucAiNum)
    {
        for (phwai=phwaich_g; phwai<phwaich_g+iHwAiChNum_g; phwai++)
        {
            /* Search raw sample data channel for hardware AI MEA. */
            if (phwai->p_part == p_part)
            {
                *p = strlen(phwai->aucId);
                memcpy(p+1, phwai->aucId, *p);
                p += *p+1;
                U32_TO_BYTES(p, phwai->servertype);
                p += 4;
                p += 16;
            }
        }
    }

    if (p_part->ucAoNum)
    {
        for (phwao=phwaoch_g; phwao<phwaoch_g+iHwAoChNum_g; phwao++)
        {
            /* Search hardware AO. */
            if (phwao->p_part == p_part)
            {
                *p = strlen(phwao->aucId);
                memcpy(p+1, phwao->aucId, *p);
                p += *p+1;
                U32_TO_BYTES(p, phwao->servertype);
                p += 4;
                p += 16;
            }
        }
    }

    if (p_part->ucDiNum)
    {
        for (pdi=plgcdich_g; pdi<plgcdich_g+iLgcDiChNum_g; pdi++)
        {
            /* Search hardware DI. */
            if (pdi->p_part == p_part)
            {
                *p = strlen(pdi->aucName);
                memcpy(p+1, pdi->aucName, *p);
                p += *p+1;
                U32_TO_BYTES(p, pdi->servertype);
                p += 4;
                p += 16;
            }
        }
    }

    if (p_part->ucDoNum)
    {
        for (pdo=plgcdoch_g; pdo<plgcdoch_g+iLgcDoChNum_g; pdo++)
        {
            /* Search hardware DI. */
            if (pdo->p_part == p_part)
            {
                *p = strlen(pdo->aucName);
                memcpy(p+1, pdo->aucName, *p);
                p += *p+1;
                U32_TO_BYTES(p, pdo->servertype);
                p += 4;
                p += 16;
            }
        }
    }

    *pPara = p;

    return EP_SUCCESS;
}

/* adjust the mod.
 * Para:
 *     pmodname, module name.
 *     pchnname, channel name.
 * Return:
 *     result.
 */
int32_t RD_AdjMod(uint8_t *pmodname, uint8_t *pchnname, uint8_t uccmdtype)
{
    RD_PART_INFO *p_part;
    RD_PART_INFO *p_temp_part = NULL;
    uint8_t findnum = 0;
    uint8_t ucAdjCtrl = 0;
    uint8_t ucChnStrl = 0;
    RD_HW_AI_CH *phwai;

    LOG_Dbg_Msg("校准: %s %s %x\n", (int)pmodname, (int)pchnname, uccmdtype, 0, 0, 0);

    for (p_part=apartinf_g; p_part<apartinf_g+rdinfo_g.unPartNum; p_part++)
    {
        if (!strcmp(pmodname, p_part->aucId))
        {
            p_temp_part = p_part;
            findnum++;
        }
    }

    if ((findnum == 0) || (findnum>1))
    {
        /* no module or too more. */
        return 1;
    }

    p_part = p_temp_part;

    if (!(p_part->servertype&0x00000001))
    {
        /* no service. */
        return 3;
    }

    if (uccmdtype == 0x22)
    {
        /* plus. */
        ucAdjCtrl = 0x02;
    }
    else if (uccmdtype == 0x55)
    {
        /* excursion. */
        ucAdjCtrl = 0x01;
    }

    if (strlen(pchnname) == 0)
    {
        /* all channel. */
        ucChnStrl=0xff;
    }
    else
    {
        if (p_part->ucType == COM_MODULE)
        {
            if (p_part->ucAiNum)
            {
                findnum = 0;
                for (phwai=phwaich_g; phwai<phwaich_g+iHwAiChNum_g; phwai++)
                {
                    /* Search hardware AI MEA. */
                    if ((phwai->p_part == p_part) &&
                            !strcmp(phwai->aucId, pchnname))
                    {
                        ucChnStrl = phwai->ucModCh;
                        ucChnStrl++;  /* begin from 1. */
                        findnum++;
                    }
                }

                if (findnum == 0)
                {
                    /* no channel. */
                    return 2;
                }
                else if (findnum>1)
                {
                    /* more channel. */
                    return 3;
                }
            }
            else /* no channel. */
            {
                return 2;
            }
        }
        else /* module type error. */
        {
            return 3;
        }
    }

    if (IO_adjust(p_part->aucHwAddr[0], ucAdjCtrl, ucChnStrl) == EP_SUCCESS)
    {
        return 0;
    }
    else
    {
        return 3;
    }
}

/* 读取索引定值页序.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUSSESS, success.
 *     EP_ERROR, fail.
 * Alert:
 *     在文件系统没有建立起来之前调用,
 *     对任何装置都可能报错,
 *     重启后正常
 */
EP_STATUS Hw_GetAcMdType(void)
{
    /* edp01.ini文件不存在 */
    if (FT_Is_File(EP_SYS_INI_FILE) == FALSE)
    {
        goto reterr;
    }

    AdMdType.iCurrentType = EP_GetAcMdType();
    if (AdMdType.iCurrentType <= -1)
    {
        /* 无效 */
        goto reterr;
    }
    else
    {
        AdMdType.bValid = TRUE; /* 有效 */

        return EP_SUCCESS;
    }

reterr:

    /* 防止读出出错,使用默认值 */
    AdMdType.iCurrentType = 0;
    AdMdType.bChgFlag = FALSE;  /* 不进行切换 */
    AdMdType.bValid = FALSE; /* 无效 */

    return EP_ERROR;
}

void RD_SetSpiModuleUsed()
{
    bSpiModuleUsed=TRUE;
}

BOOL RD_GetSpiModuleFlag()
{
    return bSpiModuleUsed;
}

void RD_SetMbDIUsed()
{
    bMbDiUsed=TRUE;
}

BOOL RD_GetMbDIUsedFlag(void)
{
    return bMbDiUsed;
}

US_CNT_UTC_TIME RD_GetUTDiChgTimeByDiIndex(int index)
{
    US_CNT_UTC_TIME uttime;
    uttime.ullusCntFrom1970=0;
    uttime.ucQflag=0x60;

    if((index>iLgcDiChNum_g)||(index<=0))
        return uttime;

    return plgcdich_g[index-1].utChgTime;
}

int AppGetDiState(int num)
{
    RD_LGC_DI_CH *plgcdi;
    if(num<0)
        assert(FALSE);
    if(num>iLgcDiChNum_g)
        assert(FALSE);
    plgcdi=plgcdich_g+num;
    return(plgcdi->iVal);
}

/* 设置采样通道一次额定值.
 * Para:
 *     pvLgcAiHnd, 通道句柄.
 *     uFstRatedVal, 一次额定值.
 * Return:
 *     NONE.
 */
void Set_AIFstRateVal(void *pvLgcAiHnd, uint16_t uFstRatedVal)
{
    RD_LGC_AI_CH *plgcai;
    float fTmp;
    uint32_t uFstRatedValTmp = 0;

    assert (pvLgcAiHnd);

    plgcai = (RD_LGC_AI_CH *)pvLgcAiHnd;

    if (plgcai->phwai->ucUnit == 0x14)
    {
        /* 电压,将kV换算为V */
        uFstRatedValTmp = uFstRatedVal*1000;
    }
    else
    {
        uFstRatedValTmp = uFstRatedVal;
    }

    if (plgcai->phwai->uFstRatedVal != uFstRatedValTmp)
    {
        plgcai->phwai->uFstRatedVal = uFstRatedValTmp;
        fTmp = plgcai->phwai->uSecRatedVal;
        fTmp = fTmp/100.0;

        plgcai->phwai->fTradSecToFstCoff
            = fabs((float)plgcai->phwai->uFstRatedVal/fTmp);

        bFstValChg_g = TRUE;
    }
}

/* 设置采样通道一次额定值.(电压的单位为V,电流的单位为A)
 * Para:
 *     pvLgcAiHnd, 通道句柄.
 *     uFstRatedVal, 一次额定值.(电压的单位为V,电流的单位为A)
 * Return:
 *     NONE.
 */
void Set_AIFstRateVal2(void *pvLgcAiHnd, uint32_t uFstRatedVal)
{
    RD_LGC_AI_CH *plgcai;
    float fTmp;

    assert (pvLgcAiHnd);

    plgcai = (RD_LGC_AI_CH *)pvLgcAiHnd;
    if (plgcai->phwai->uFstRatedVal != uFstRatedVal)
    {
        plgcai->phwai->uFstRatedVal = uFstRatedVal;
        fTmp = plgcai->phwai->uSecRatedVal;
        fTmp = fTmp/100.0;

        plgcai->phwai->fTradSecToFstCoff
            = (float)plgcai->phwai->uFstRatedVal/fTmp;

        bFstValChg_g = TRUE;
    }
}

/* 设置采样通道二次额定值.
 * Para:
 *     pvLgcAiHnd, 通道句柄.
 *     fSecRatedVal, 二次额定值(浮点), 存储为16位整数, 扩大100倍.
 * Return:
 *     NONE.
 */
void Set_AISecRateVal(void *pvLgcAiHnd, float fSecRatedVal)
{
    RD_LGC_AI_CH *plgcai;
    uint16_t uWrSecRatedVal;
    float fTmp;
    uint8_t aucBuf[128];

    assert (pvLgcAiHnd);
    plgcai = (RD_LGC_AI_CH *)pvLgcAiHnd;

    /* 判断1A和5A */
    if ((fSecRatedVal>(1.0-FLT_PRECISION)) && (fSecRatedVal<(1.0+FLT_PRECISION)))
    {
        plgcai->phwai->iIndexSn = 0;
    }
    else if ((fSecRatedVal>(5.0-FLT_PRECISION)) && (fSecRatedVal<(5.0+FLT_PRECISION)))
    {
        plgcai->phwai->iIndexSn = 1;
    }
    else
    {
        if(plgcai->phwai->ucUnit == 0x8)   /* 电流才判断1A/5A */
        {
            sprintf(aucBuf,"%s 的二次值设置为%.3f ,非1A/5A, 无效!\n",plgcai->aucId,fSecRatedVal);
            LOG_Write(LOG_KERNEL, aucBuf, NULL);
            return;
        }
    }

    /* 确认输入监视门槛
     */
    if (plgcai->phwai->iIndexSn == 0)
    {
        /* 实数式电流
         */
        if (plgcai->phwai->ucUnit == 0x8)
        {
            plgcai->phwai->fThreshold = THD_CURRENT_1A;
        }
        else if (plgcai->phwai->ucUnit == 0x14)
        {
            /* 实数式电压 */
            plgcai->phwai->fThreshold = THD_VOLT;
        }
        else
        {
            assert (FALSE);
        }
    }
    else
    {
        /* 实数式电流
         */
        if (plgcai->phwai->ucUnit == 0x8)
        {
            plgcai->phwai->fThreshold = THD_CURRENT_5A;
        }
        else if (plgcai->phwai->ucUnit == 0x14)
        {
            /* 实数式电压 */
            plgcai->phwai->fThreshold = THD_VOLT;
        }
        else
        {
            assert (FALSE);
        }
    }

    /* 非负 */
    if (fSecRatedVal<(0.0+FLT_PRECISION))
    {
        uWrSecRatedVal = 0;
        sprintf(aucBuf,"%s 的二次值设置为%.3f ,为0, 无效!\n",plgcai->aucId,fSecRatedVal);
        LOG_Write(LOG_KERNEL, aucBuf, NULL);

        return;
    }
    else if ((fSecRatedVal*100) >= (65535.0-FLT_PRECISION))
    {
        uWrSecRatedVal = 65535;
    }
    else
    {
        uWrSecRatedVal = (uint16_t)(fSecRatedVal*100.0);
    }

    plgcai = (RD_LGC_AI_CH *)pvLgcAiHnd;
    if (plgcai->phwai->uSecRatedVal != uWrSecRatedVal)
    {
        plgcai->phwai->uSecRatedVal = uWrSecRatedVal;

        fTmp = plgcai->phwai->uSecRatedVal;
        fTmp = fTmp/100.0;

        plgcai->phwai->fTradSecToFstCoff
            = (float)plgcai->phwai->uFstRatedVal/fTmp;

        bFstValChg_g = TRUE;
    }
}

/* 设置采样通道二次额定值和索引定值页序.
 * Para:
 *     pvLgcAiHnd, 通道句柄.
 *     fSecRatedVal, 二次额定值(浮点), 存储为16位整数, 扩大100倍.
 *     iIndexSn, 大于等于0.
 * Return:
 *     NONE.
 */
void Set_AISecRateValAndIndexSn(void *pvLgcAiHnd, float fSecRatedVal, int32_t iIndexSn)
{
    RD_LGC_AI_CH *plgcai;
    uint16_t uWrSecRatedVal;
    float fTmp;

    assert (pvLgcAiHnd);

    plgcai = (RD_LGC_AI_CH *)pvLgcAiHnd;

    /* 索引定值页序 */
    if ((plgcai->phwai->iIndexSn != iIndexSn)
            && (iIndexSn<AdMdType.iMaxTypeNum))
    {
        plgcai->phwai->iIndexSn = iIndexSn;
        bFstValChg_g = TRUE;
    }

    /* 确认输入监视门槛
     */
    if (plgcai->phwai->iIndexSn == 0)
    {
        /* 实数式电流
         */
        if (plgcai->phwai->ucUnit == 0x8)
        {
            plgcai->phwai->fThreshold = THD_CURRENT_1A;
        }
        else if (plgcai->phwai->ucUnit == 0x14)
        {
            /* 实数式电压 */
            plgcai->phwai->fThreshold = THD_VOLT;
        }
        else
        {
            assert (FALSE);
        }
    }
    else
    {
        /* 实数式电流
         */
        if (plgcai->phwai->ucUnit == 0x8)
        {
            plgcai->phwai->fThreshold = THD_CURRENT_5A;
        }
        else if (plgcai->phwai->ucUnit == 0x14)
        {
            /* 实数式电压 */
            plgcai->phwai->fThreshold = THD_VOLT;
        }
        else
        {
            assert (FALSE);
        }
    }

    /* 非负 */
    if (fSecRatedVal<(0.0+FLT_PRECISION))
    {
        uWrSecRatedVal = 0;
        LOG_Write(LOG_KERNEL, "二次值设置为0, 无效!\n", NULL);

        return;
    }
    else if ((fSecRatedVal*100) >= (65535.0-FLT_PRECISION))
    {
        uWrSecRatedVal = 65535;
    }
    else
    {
        uWrSecRatedVal = (uint16_t)(fSecRatedVal*100.0);
    }

    if (plgcai->phwai->uSecRatedVal != uWrSecRatedVal)
    {
        plgcai->phwai->uSecRatedVal = uWrSecRatedVal;

        fTmp = plgcai->phwai->uSecRatedVal;
        fTmp = fTmp/100.0;

        plgcai->phwai->fTradSecToFstCoff
            = (float)plgcai->phwai->uFstRatedVal/fTmp;

        bFstValChg_g = TRUE;
    }
}

/* 更新采样通道一次额定值.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void Update_AIFstRateVal(void)
{
    SLOW_MESSAGE_NODE Info;

    if (!bFstValChg_g)
    {
        return;
    }

    bFstValChg_g = FALSE;

    // UpdateSmvValIn();  /* 更新实际CT系数 */

    AdMdType.bChgFlag = TRUE; /* 更新增益系数 */

    /* 触发写一次额定值慢速任务
     */
    Info.type = CT_PT_RATE_VAL_WR;
    msgQSend(SlowMessage, (char *)&Info,
             sizeof(SLOW_MESSAGE_NODE),
             NO_WAIT,
             MSG_PRI_NORMAL
            );
}

/* 更新单通道消抖时间.
 * Para:
 *     pSrc, 开入通道句柄, 来源于输入.
 *     ulFltTm, 消抖时间, us.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL SetDiFltTime(void *pSrc, uint32_t ulFltTm)
{
    // RD_LGC_DI_CH *plgcdi;
    // DI_CHANNEL *pdich;
    // int iLockKey;

    // plgcdi = (RD_LGC_DI_CH *)pSrc;
    // if ((pSrc == NULL) || (plgcdi->pvSrc == NULL) || (plgcdi->mod != RD_SPI_DI))
    // {
    //     return FALSE;
    // }

    // pdich = (DI_CHANNEL *)plgcdi->pvSrc;

    // if(pdich->ulFiltTime == ulFltTm)
    // {
    //     return FALSE;
    // }

    // iLockKey = intLock();
    // pdich->ulFiltTime = ulFltTm;
    // pdich->ulFltCfg = pdich->ulFiltTime/((1000000L)/RD_SIO_RATE);
    // /* 根据测试仪正偏特性及应用需求,消抖时间延长1个中断周期 */
    // pdich->ulFltCfg++;
    // pdich->ulFltTmp = pdich->ulFltCfg;
    // intUnlock(iLockKey);

    return TRUE;
}

