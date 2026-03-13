/* dspai.c - This file contains interface to AI channel in DSP system */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 22nov06, dy Created first version 0.1.
*/

/*
DESCRIPTION
This file contains interface to AI channel in DSP system.
INCLUDE: dspai.h
*/

/* includes */

#include "dspai.h"
#include "hwcfg.h"
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
#include "dsp.h"
#include "realdata.h"
#include "OPT_Data.h"
#include "OPT_VtBox.h"
#include "OPT_SynAdapt.h"

#include "redun_box.h"		/* 冗余机箱 */

/* 合并版所有平台包含 */
#include "POLE_Data.h"
#include "POLE_VtBox.h"
#include "HDL_Data.h"
#include "HDL_VtBox.h"

#include "VTBOX_Data.h"		/* 虚拟机箱 */
#include "VTBOX_Box.h"

// #ifdef EXCITE_BUILD
// #include "OptComm.h"
// #endif

/* defines */

#define RD_MAX_AI_RATE  5120            			/* 最大AI刷新周期，次/秒 */

#ifdef EDP03_BUILD
#define RD_DSP_AI_CH    14              /* EDP03平台*/
#endif

/* 内存满足要求 */
#ifdef EDP_01_02_BUILD
#define RD_DSP_AI_CH    180              /* EDP01平台C-A版本 */
#endif

#ifdef EXCITE_BUILD
#define RD_DSP_AI_CH    66              /* 励磁平台 */
#endif

#define DSP_LGC_AI_CH (2*RD_DSP_AI_CH)
#define DSP_CALC_AI_CH (8*RD_DSP_AI_CH)
#define DSP_MSU_AI_CH (8*RD_DSP_AI_CH)

/* globals */

DSP_LGC_AI_CFG *pdspl_cfg_g;
DSP_LGC_DC_AI_CFG *pdspl_dc_cfg_g;			/* DC signal processing varialbe. */
DSP_CALC_AI_CFG *pdspc_cfg_g;
DSP_MSU_AI_CFG *pdspm_cfg_g;		/* Measurement variable */
DSP_LGC_AI_CFG *pdspl_cfg_ext_g;
DSP_CALC_AI_CFG *pdspc_cfg_ext_g;
DSP_LGC_AI_CFG *pdspl_cfg_redun_g;
DSP_CALC_AI_CFG *pdspc_cfg_redun_g;

DSP_LGC_AI_CFG *pdspl_cfg_Opt_g[2];		/* 用于光纵 */
DSP_CALC_AI_CFG *pdspc_cfg_Opt_g[2];

DSP_LGC_AI_CFG *pdspl_cfg_Pole_g; /* 用于同杆并架 */
DSP_CALC_AI_CFG *pdspc_cfg_Pole_g;			/* 用于智能操作箱 */

DSP_LGC_AI_CFG *pdspl_cfg_Hdl_g; /* 用于智能操作箱 */
DSP_CALC_AI_CFG *pdspc_cfg_Hdl_g;

DSP_LGC_AI_CFG *pdspl_cfg_Virt_g[MAX_VT_BOX_COUNT]; 	/* 用于虚拟机箱 */
DSP_CALC_AI_CFG *pdspc_cfg_Virt_g[MAX_VT_BOX_COUNT];

RD_LGC_AI_DB lgcaidb_g; 		/* 逻辑通道数据缓冲 */
RD_LGC_AI_STS_DB lgcaistsdb_g;
RD_CALC_AI_DB calcaidb_g; 				/* 预处理通道数据缓冲 */
RD_MSU_AI_DB msucaidb_g;
RD_DI_DB didb_g;
RD_AI_VALID_DB aivaliddb_g;   /* 张云添加 */

SEM_ID semHwAiMea;
SEM_ID semPoMea;

uint16_t uiProRate_g;
uint32_t Sysfrequency_g;
uint32_t uiTxPts;


static BOOL SampInsert_s = TRUE;  /* 传统采样光差是否需要插值,缺省为需要 */

/* global functions */
extern void HDL_Set_LineFilt();

/* 设置A口为1MHz模式 */
extern void set_hdlc_a_1m(void);

/* 设置B口为1MHz模式 */
extern void set_hdlc_b_1m(void);

/* forward declarations */

/***********************************************************************
* RD_Init_Dsp_Cfg - DSP计算与配置对应
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Init_Dsp_Cfg(void);

/***********************************************************************
* RD_Init_Dsp_Msu_Cfg - 为测量计算结构及结果存储区分配存储空间，并逐项添加测量计算结构
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Init_Dsp_Msu_Cfg(void);

/***********************************************************************
* RD_Loc_Hw_Ai - 为物理通道找结果存储地址
*
* RETURNS: 无
*
*
*/
static void RD_Loc_Hw_Ai(
    RD_HW_AI_CH *phwai,		/* AI物理通道 */
    RD_AI_MOD *paimod, 					/* 机箱 */
    DSP_LGC_AI_CFG *pdspl_cfg				/* DSP配置 */
);

/***********************************************************************
* RD_Add_Ai_Cfg - 配置与DSP计算结果连接
*
* RETURNS: 无
*
*
*/
static void RD_Add_Ai_Cfg(
    RD_AI_MOD *paimod,		/* 机箱 */
    RD_LGC_AI_CH *pch,				/* DSP计算通道 */
    DSP_LGC_AI_CFG *plcfgSave, 		/* 瞬间值计算配置 */
    DSP_CALC_AI_CFG *pccfgSave					/* 预处理计算配置 */
);

/***********************************************************************
* RD_Loc_Lgc_Ai - 瞬时值遍历
*
* RETURNS: 无
*
*
*/
static void RD_Loc_Lgc_Ai(
    RD_LGC_AI_CH *paich,			/* AI逻辑通道 */
    RD_AI_MOD *paimod, 		/* 机箱 */
    DSP_LGC_AI_CFG *pdspl_cfg		/* DSP瞬时值处理配置 */
);

/***********************************************************************
* RD_Loc_Lgc_Ai - 预处理值遍历
*
* RETURNS: 无
*
*
*/
static void RD_Loc_Calc_Ai(
    RD_LGC_AI_CH *paich, 				/* AI逻辑通道 */
    RD_AI_MOD *paimod,		/* 机箱 */
    DSP_CALC_AI_CFG *pdspc_cfg, 				/* 预处理值配置 */
    DSP_LGC_AI_CFG *pdspl_cfg		/* 瞬时值配置 */
);

/***********************************************************************
* RD_Loc_Msuc_Ai - 测量结果与配置缓冲区对应
*
* RETURNS: 无
*
*
*/
static void RD_Loc_Msuc_Ai(
    RD_MSU_AI_CH *paichmsu,		/* DSP计算测量配置*/
    RD_AI_MOD *paimod,		/* 机箱 */
    DSP_MSU_AI_CFG *pdspm_cfg, 				/* 测量配置 */
    DSP_LGC_AI_CFG *pdspl_cfg		/* 瞬时值配置 */
);

/***********************************************************************
* RD_Sort_Dsp_Cfg - DSP配置排序
*
* RETURNS: 无
*
*
*/
static void RD_Sort_Dsp_Cfg(
    RD_AI_MOD *paimod, 		/* 机箱 */
    DSP_LGC_AI_CFG *pdspl_cfg,					/* DSP瞬时值配置 */
    DSP_CALC_AI_CFG *pdspc_cfg		/* DSP预处理配置 */
);

/***********************************************************************
* RD_Cmp_Calc_Cfg - 排序函数
*
* RETURNS: 无
*
*
*/
static int RD_Cmp_Calc_Cfg(
    const void *pvSrc,		/* 源 */
    const void *pvDst			/* 目标 */
);

/***********************************************************************
* RD_Compack_Calc_Cfg - 配置压缩
*
* RETURNS: DSP配置
*
*/
static int RD_Compack_Calc_Cfg(
    DSP_CALC_AI_CFG *pdspc_cfg,		/* DSP预处理配置 */
    int iCalcCfg		/* 个数 */
);

/***********************************************************************
* RD_Sort_Dsp_Msu_Cfg - 配置排序
*
* RETURNS: DSP配置
*
*/
static void RD_Sort_Dsp_Msu_Cfg(
    RD_AI_MOD *paimod, 		/* 模拟量计算结果读取有关模式结构 */
    DSP_LGC_AI_CFG *pdspl_cfg, 		/* 瞬时值处理结构 */
    DSP_MSU_AI_CFG *pdspm_cfg				/* 测量计算配置结构 */
);

/***********************************************************************
* RD_Cmp_Msu_Cfg - 测量配置排序
*
* RETURNS: 比较结果
*
*/
static int RD_Cmp_Msu_Cfg(
    const void *pvSrc,			/* 源 */
    const void *pvDst				/* 目标 */
);

/***********************************************************************
* RD_Compress_Msuc_Cfg - 测量通道配置合并
*
* RETURNS: 压缩后项数
*
*/
static int RD_Compress_Msuc_Cfg(
    DSP_MSU_AI_CFG *pdspm_cfg, 		/* 测量通道配置指针 */
    int iMsucCfg			/* 原始项数 */
);

/***********************************************************************
* RD_Add_Ai_Msu_Cfg - 解析配置文件
*
* RETURNS: 无
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*/
static void RD_Add_Ai_Msu_Cfg(
    RD_AI_MOD *paimod, 		/* 模拟量计算结果读取有关模式结构 */
    RD_MSU_AI_CH *pch, 				/* 测量有关文件配置结构 */
    DSP_LGC_AI_CFG *plcfgSave,			/* 瞬时值处理结构 */
    DSP_MSU_AI_CFG *pmcfgSave		/* 测量计算配置结构 */
);

/***********************************************************************
* DspCoeUpdate - 系数更新
*
* RETURNS: OK, or ERROR
*
*/
static EP_STATUS DspCoeUpdate(
    RD_HW_AI_CH *phwai,		/* AI物理通道 */
    RD_AI_MOD *paimod, 					/* 机箱 */
    DSP_LGC_AI_CFG *pdspl_cfg				/* DSP配置 */
);

/***********************************************************************
* RDChgAiCfg - 更改DSP配置内容
*
* RETURNS: 无
*
*
*/
static void RDChgAiCfg(
    RD_AI_MOD *paimod,		/* 机箱 */
    RD_HW_AI_CH *pch,				/* 物理通道 */
    DSP_LGC_AI_CFG *plcfgSave 				/* 瞬间值计算配置 */
);

/* match the DC result buffer with the configuration.
 * Para:
 *     paichdc, configuration channel.
 *     paimod, box information.
 *     pdspl_dc_cfg, proceesing channel.
 * Return:
 *     NONE.
 */
static void RD_Loc_Dc_Ai(RD_LGC_AI_CH *paichdc, RD_AI_MOD *paimod, DSP_LGC_DC_AI_CFG *pdspl_dc_cfg);

/* add dc Ai DSP channel.
 * Para:
 *     paimod, module.
 *     pch, configuration channel.
 *     plcfgSave, DSP channel.
 * Return:
 *     NONE.
 */
static void RD_Add_Dc_Ai_Cfg(RD_AI_MOD *paimod, RD_LGC_AI_CH *pch, DSP_LGC_DC_AI_CFG *plcfgSave);

/* set the result position in buffer for physical channel.
 * Para:
 *     phwai, physical channel.
 *     paimod, box data.
 *     pdspl_dc_cfg, DC processing structure.
 * Return:
 *     NONE.
 */
static void RD_Loc_Hw_Dc_Ai(RD_HW_AI_CH *phwai, RD_AI_MOD *paimod, DSP_LGC_DC_AI_CFG *pdspl_dc_cfg);

/* functions */

/***********************************************************************
* RD_Init_AI_Db - 初始化AI数据缓冲区
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
EP_STATUS RD_Init_AI_Db(void)
{
    int i;
    RD_LGC_AI_CH *paich;
    RD_MSU_AI_CH *paichmsu;
    RD_HW_AI_CH *phwai;
    float *pfVtAI;
    COMPLEX *pxVtAI;
    int iVtLgcAiCh;		/* 虚拟逻辑通道 */
    int iSmplCh = 0;			/* 虚拟通道个数和 */
    int iVirtBoxTotalSmplCh = 0;
    float *pfVirtBoxDbBgn;			/* 虚拟机箱通道原始通道开始地址 */
    COMPLEX *pxVirtBoxDbBgn;  /* 虚拟机箱通道原始通道开始*/
    DSP_LGC_DC_AI_CFG *pdcch;
    uint8_t TempInfo[256];
    EP_STATUS stsRet = EP_SUCCESS;

    rdinfo_g.ulDspBoxCount=0;		/* 数据写入缓冲区计数 */
    rdinfo_g.ulExtBoxCount=0;
    if (RD_Init_Dsp_Cfg() != EP_SUCCESS)		/* DSP计算缓冲区配置 */
        return EP_CFG_ERR;
    if(RD_Init_Dsp_Msu_Cfg() != EP_SUCCESS)				/* 测量计算缓冲区配置 */
        return EP_CFG_ERR;

    for (i=0; i<MAX_VT_BOX_COUNT; i++)
    {
        /* 计算虚拟机箱通道个数 */
        iVirtBoxTotalSmplCh += aimodVtBox_g[i].iLgcNum;
    }

    for (phwai=phwaich_g; phwai<phwaich_g+iHwAiChNum_g; phwai++)		  /* Search raw sample data channel for hardware AI MEA. */
    {
        if ((phwai->paimod == &aimodDsp_g) && (phwai->p_part->ucAiModType == RD_AI_AC))				/* 主机箱，交流模件 */
            RD_Loc_Hw_Ai(phwai, &aimodDsp_g, pdspl_cfg_g);
        else if (phwai->paimod == &aimodExt_g)				/* 外部机箱 */
        {
            RD_Loc_Hw_Ai(phwai, &aimodExt_g, pdspl_cfg_ext_g);
            if (phwai->iSmplCh!=-1)     		/* Valid channel, should after DSP. */
                phwai->iSmplCh+=aimodDsp_g.iLgcNum;
        }
        else if(phwai->paimod == &aimodRedun_g)		/* 励磁装置 */
        {
            RD_Loc_Hw_Ai(phwai, &aimodRedun_g, pdspl_cfg_redun_g);
            if (phwai->iSmplCh!=-1)     /* Valid channel, should after DSP. */
                phwai->iSmplCh+=aimodDsp_g.iLgcNum+aimodExt_g.iLgcNum; 			/* 结果位置通道 */
        }
        else if(phwai->paimod == &aimodOpt_g[0])    	/* 为光纵修改 */
        {
            RD_Loc_Hw_Ai(phwai, &aimodOpt_g[0], pdspl_cfg_Opt_g[0]);
            if (phwai->iSmplCh != -1)     /* Valid channel, should after Ext. */
                phwai->iSmplCh += aimodDsp_g.iLgcNum+aimodExt_g.iLgcNum+aimodRedun_g.iLgcNum;
        }
        else if(phwai->paimod == &aimodOpt_g[1])
        {
            RD_Loc_Hw_Ai(phwai, &aimodOpt_g[1], pdspl_cfg_Opt_g[1]);
            if (phwai->iSmplCh != -1)     /* Valid channel, should after Opt1. */
                phwai->iSmplCh += aimodDsp_g.iLgcNum+aimodExt_g.iLgcNum+aimodRedun_g.iLgcNum+aimodOpt_g[0].iLgcNum;
        }
        else if(phwai->paimod == &aimodPole_g) /*2007-3-28日 张云  */
        {
            RD_Loc_Hw_Ai(phwai, &aimodPole_g, pdspl_cfg_Pole_g);
            if (phwai->iSmplCh != -1)     /* Valid channel, should after Opt1. */
                phwai->iSmplCh += aimodDsp_g.iLgcNum+aimodExt_g.iLgcNum
                                  +aimodRedun_g.iLgcNum+aimodOpt_g[0].iLgcNum+aimodOpt_g[1].iLgcNum;
        }
        else if(phwai->paimod == &aimodHdl_g) /*2007-3-28日 张云  */
        {
            RD_Loc_Hw_Ai(phwai, &aimodHdl_g, pdspl_cfg_Hdl_g);
            if (phwai->iSmplCh != -1)     /* Valid channel, should after Opt1. */
                phwai->iSmplCh += aimodDsp_g.iLgcNum+aimodExt_g.iLgcNum
                                  +aimodRedun_g.iLgcNum+aimodOpt_g[0].iLgcNum+aimodOpt_g[1].iLgcNum;
        }
        else if ((phwai->paimod >= &(aimodVtBox_g[0])) && (phwai->paimod < &(aimodVtBox_g[MAX_VT_BOX_COUNT])))
        {
            /* 虚拟机箱 */
            RD_Loc_Hw_Ai(phwai, &aimodVtBox_g[phwai->ucVtBoxPos], pdspl_cfg_Virt_g[phwai->ucVtBoxPos]);

            if (phwai->iSmplCh != -1)     /* Valid channel, should after pole. */
            {
                int iSmplCh=0;
                int i;

                for(i=0; i<phwai->ucVtBoxPos; i++)
                {
                    /* 前面机箱通道个数 */
                    iSmplCh += aimodVtBox_g[i].iLgcNum;		/* 前面的虚拟机箱的总和 */
                }

                phwai->iSmplCh += aimodDsp_g.iLgcNum+aimodExt_g.iLgcNum
                                  +aimodRedun_g.iLgcNum+aimodOpt_g[0].iLgcNum+aimodOpt_g[1].iLgcNum
                                  +aimodPole_g.iLgcNum+iSmplCh;	/* 当前通道所在位置 */
            }
        }
        else if ((phwai->paimod == &aimodDsp_g)
                 && (phwai->p_part->ucAiModType == RD_AI_DC))				/* main box，DC signal module. */
        {
            RD_Loc_Hw_Dc_Ai (phwai, &aimodDsp_g, pdspl_dc_cfg_g);

            if (phwai->iSmplCh != -1)     /* Valid channel, should after Opt1. */
                phwai->iSmplCh += aimodDsp_g.iLgcNum+aimodExt_g.iLgcNum
                                  +aimodRedun_g.iLgcNum+aimodOpt_g[0].iLgcNum+aimodOpt_g[1].iLgcNum
                                  +aimodPole_g.iLgcNum+iVirtBoxTotalSmplCh;	/* 当前通道所在位置 */
        }
        else
        {
            assert(FALSE);
        }
    }

    for (pdcch=pdspl_dc_cfg_g; pdcch<pdspl_dc_cfg_g+aimodDsp_g.iDcAiNum; pdcch++)
    {
        /* DC signal processing. */

        if (!SIO_GetIOExsitSts(pdcch->ucHwAddr))
        {
            pdcch->pSrc = NULL;

            if (!EP_IS_BOOT_SEL())		/* 用于EDP01和EDP02平台调试，防止RD_Init_DI_Db函数之后的函数不初始化会导致装置重启 */
            {
                /* 如果是发布版，则返回错误 */
                stsRet = EP_CFG_ERR;
            }

            continue;
        }

        pdcch->pSrc=SIO_Init_AI (pdcch->ucHwAddr, pdcch->ucHdCh, &pdcch->fRate);

        if (!pdcch->pSrc)
        {
            if (ENG_MODE == 0)
            {
                sprintf(TempInfo, "模件地址:%d,通道名称:%s",
                        pdcch->ucHwAddr+1, (char *)(((RD_LGC_AI_CH *)pdcch->pCfg)->aucId));
                ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                           "%s(%02d)\n",
                           (int)TempInfo, IO_MOD_AI_INIT_ERR);
            }
            else if(ENG_MODE == 1)
            {
                sprintf(TempInfo, "Module address:%d,channel:%s",
                        pdcch->ucHwAddr+1, (char *)(((RD_LGC_AI_CH *)pdcch->pCfg)->aucId));
                ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                           "%s(%02d)\n",
                           (int)TempInfo, IO_MOD_AI_INIT_ERR);

            }
            sprintf(TempInfo, "输入通道初始化异常,模件地址:%d,通道名称:%s!!\n",
                    pdcch->ucHwAddr+1, (char *)(((RD_LGC_AI_CH *)pdcch->pCfg)->aucId));
            LOG_Write(LOG_KERNEL, TempInfo, NULL);

        }
    }

    semHwAiMea=semMCreate(SEM_Q_PRIORITY);					/* 物理通道可以被读取 */
    assert(semHwAiMea!=NULL);

    semPoMea=semMCreate(SEM_Q_PRIORITY);					/* PO通道可以被读取 */
    assert(semPoMea!=NULL);

    /* 增加了虚拟实数AI通道 */
    iVtLgcAiCh=0;
    for (paich=plgcaich_g+iLgcAiChNum_g-1;
            paich >= plgcaich_g+iLgcAiChNum_g-iVtAiChNum_g; paich--)
    {
        assert(!paich->phwai);          /* Virtual AI. */
        if (IS_REAL_AI(paich->ucUnit))
            iVtLgcAiCh++;
    }

    for(i=0; i<MAX_VT_BOX_COUNT; i++)
    {
        iSmplCh += aimodVtBox_g[i].iLgcNum;		/* 所有虚拟机箱的总和 */
    }

    lgcaidb_g.uiTotalCh=aimodDsp_g.iLgcNum+aimodExt_g.iLgcNum+aimodRedun_g.iLgcNum
                        +aimodOpt_g[0].iLgcNum+aimodOpt_g[1].iLgcNum+aimodPole_g.iLgcNum+aimodHdl_g.iLgcNum+iVtLgcAiCh+iSmplCh; 		/* 总的逻辑通道数 */

    lgcaidb_g.uiTotalCh += aimodDsp_g.iDcAiNum;		/* add DC channel. */

    i=0;
    for (paich=plgcaich_g; paich<plgcaich_g+iLgcAiChNum_g; paich++)				/* 从逻辑通道配置中得到预处理数 */
    {
        if (IS_CPLX_AI(paich->ucUnit))							/* 计算结果为复数 */
            i++;
    }
    calcaidb_g.uiTotalCh=i;				/* 总的预处理配置数 */

    lgcaidb_g.ulBufLen=((uint32_t)uiAiRate_g/uiPwrFreq_g)*				/* 预定周波的点数，包括所有的通道 */
                       lgcaidb_g.uiTotalCh*(g_RdBufCyc+1);
    lgcaidb_g.uiChBytes=lgcaidb_g.uiTotalCh*sizeof(float)/sizeof(uint8_t);				/* 每一采样点的字节数，包括所有的采样通道 */
    lgcaidb_g.ulBufBytes=lgcaidb_g.ulBufLen*sizeof(float)/sizeof(uint8_t);				/* 缓冲区字节数 */

#ifndef NO_DEBUG
    LOG_Dbg_Msg("lgcaidb_g.uiTotalCh = %d   lgcaidb_g.uiChBytes = %d lgcaidb_g.ulBufBytes = %d\n",lgcaidb_g.uiTotalCh,lgcaidb_g.uiChBytes,lgcaidb_g.ulBufBytes,0,0,0);
#endif

#ifndef NO_DBL_BUF				/* 分配存储空间 */
    if ((lgcaidb_g.pfBufBgn=calloc(2*lgcaidb_g.ulBufLen, sizeof(float)))==NULL)
        return EP_BUF_ERR;
    lgcaidb_g.pfBufBgn+=lgcaidb_g.ulBufLen;
    lgcaidb_g.pfCopy=lgcaidb_g.pfBufBgn;
#else
    if ((lgcaidb_g.pfBufBgn=calloc(lgcaidb_g.ulBufLen, sizeof(float)))==NULL)
        return EP_BUF_ERR;
#endif

    lgcaidb_g.pfBufEnd=lgcaidb_g.pfBufBgn+lgcaidb_g.ulBufLen;

#ifndef NO_DBL_BUF				/* 分配状态标志存储空间 */
    if ((lgcaistsdb_g.pBufBgn = calloc(2*lgcaidb_g.ulBufLen, sizeof(uint32_t))) == NULL)
        return EP_BUF_ERR;
    lgcaistsdb_g.pBufBgn+=lgcaidb_g.ulBufLen;
#else
    if ((lgcaistsdb_g.pBufBgn = calloc(lgcaidb_g.ulBufLen, sizeof(uint32_t))) == NULL)
        return EP_BUF_ERR;
#endif
    lgcaistsdb_g.pBufEnd=lgcaistsdb_g.pBufBgn+lgcaidb_g.ulBufLen;

    calcaidb_g.ulBufLen=((uint32_t)uiAiRate_g/uiPwrFreq_g)*						/* 与逻辑通道配置一样处理 */
                        calcaidb_g.uiTotalCh*(g_RdBufCyc+1);

    calcaidb_g.uiChBytes=calcaidb_g.uiTotalCh*sizeof(COMPLEX)/sizeof(uint8_t);
    calcaidb_g.ulBufBytes=calcaidb_g.ulBufLen*sizeof(COMPLEX)/sizeof(uint8_t);

#ifndef NO_DEBUG
    LOG_Dbg_Msg("calcaidb_g.uiTotalCh = %d calcaidb_g.uiChBytes = %d calcaidb_g.ulBufBytes = %d\n",calcaidb_g.uiTotalCh,calcaidb_g.uiChBytes,calcaidb_g.ulBufBytes,0,0,0);
#endif

#ifndef NO_DBL_BUF
    if ((calcaidb_g.pxBufBgn=calloc(2*calcaidb_g.ulBufLen, sizeof(COMPLEX)))==NULL)
        return EP_BUF_ERR;
    calcaidb_g.pxBufBgn+=calcaidb_g.ulBufLen;
    calcaidb_g.pxCopy=calcaidb_g.pxBufBgn;
#else
    if ((calcaidb_g.pxBufBgn=calloc(calcaidb_g.ulBufLen, sizeof(COMPLEX)))==NULL)
        return EP_BUF_ERR;
#endif

    calcaidb_g.pxBufEnd=calcaidb_g.pxBufBgn+calcaidb_g.ulBufLen;

    aimodDsp_g.pfDbBgn=lgcaidb_g.pfBufBgn;				/* 读取结构指针初始化 */
    aimodDsp_g.pxDbBgn=calcaidb_g.pxBufBgn;

    /* Extend AI channels are after local DSP AI channels. */
    aimodExt_g.pfDbBgn=lgcaidb_g.pfBufBgn+aimodDsp_g.iLgcNum;
    aimodExt_g.pxDbBgn=calcaidb_g.pxBufBgn+aimodDsp_g.iTxCalcNum;

    aimodRedun_g.pfDbBgn=lgcaidb_g.pfBufBgn+aimodDsp_g.iLgcNum+aimodExt_g.iLgcNum; /* for excite */
    aimodRedun_g.pxDbBgn=calcaidb_g.pxBufBgn+aimodDsp_g.iTxCalcNum+aimodExt_g.iCalcNum;

    /* OPT1 AI channels are after extend AI channels. 2006-2-10*/
    aimodOpt_g[0].pfDbBgn=lgcaidb_g.pfBufBgn+aimodDsp_g.iLgcNum+aimodExt_g.iLgcNum+aimodRedun_g.iLgcNum;
    aimodOpt_g[0].pxDbBgn=calcaidb_g.pxBufBgn+aimodDsp_g.iTxCalcNum+aimodExt_g.iTxCalcNum+aimodRedun_g.iTxCalcNum;

    /* OPT2 AI channels are after Opt1 AI channels. 2006-2-10*/
    aimodOpt_g[1].pfDbBgn=lgcaidb_g.pfBufBgn+aimodDsp_g.iLgcNum+aimodExt_g.iLgcNum+aimodRedun_g.iLgcNum+aimodOpt_g[0].iLgcNum;
    aimodOpt_g[1].pxDbBgn=calcaidb_g.pxBufBgn+aimodDsp_g.iTxCalcNum+aimodExt_g.iTxCalcNum+aimodRedun_g.iTxCalcNum+aimodOpt_g[0].iTxCalcNum;

    /* POLE AI channels are after Opt2 AI channels. 2007-3-28,张云*/
    aimodPole_g.pfDbBgn=lgcaidb_g.pfBufBgn+aimodDsp_g.iLgcNum+aimodExt_g.iLgcNum+aimodRedun_g.iLgcNum
                        +aimodOpt_g[0].iLgcNum+aimodOpt_g[1].iLgcNum;
    aimodPole_g.pxDbBgn=calcaidb_g.pxBufBgn+aimodDsp_g.iTxCalcNum+aimodExt_g.iTxCalcNum+aimodRedun_g.iTxCalcNum
                        +aimodOpt_g[0].iTxCalcNum+aimodOpt_g[1].iTxCalcNum;

    /* 智能操作箱目前无AI. 2007-3-28,张云*/
    aimodHdl_g.pfDbBgn=lgcaidb_g.pfBufBgn+aimodDsp_g.iLgcNum+aimodExt_g.iLgcNum+aimodRedun_g.iLgcNum
                       +aimodOpt_g[0].iLgcNum+aimodOpt_g[1].iLgcNum+aimodPole_g.iLgcNum;
    aimodHdl_g.pxDbBgn=calcaidb_g.pxBufBgn+aimodDsp_g.iTxCalcNum+aimodExt_g.iTxCalcNum+aimodRedun_g.iTxCalcNum
                       +aimodOpt_g[0].iTxCalcNum+aimodOpt_g[1].iTxCalcNum+aimodPole_g.iTxCalcNum;

    /* Virtual box */

    pfVirtBoxDbBgn = lgcaidb_g.pfBufBgn+aimodDsp_g.iLgcNum+aimodExt_g.iLgcNum+aimodRedun_g.iLgcNum
                     +aimodOpt_g[0].iLgcNum+aimodOpt_g[1].iLgcNum+aimodPole_g.iLgcNum+aimodHdl_g.iLgcNum;	/* 虚拟机箱原始通道开始地址 */

    pxVirtBoxDbBgn = calcaidb_g.pxBufBgn+aimodDsp_g.iTxCalcNum+aimodExt_g.iTxCalcNum+aimodRedun_g.iTxCalcNum
                     +aimodOpt_g[0].iTxCalcNum+aimodOpt_g[1].iTxCalcNum+aimodPole_g.iTxCalcNum+aimodHdl_g.iTxCalcNum;		/* 预处理开始地址 */


    aimodDsp_g.pfDcDbBgn = pfVirtBoxDbBgn;



    /* Locate the work pointer to "-1" position. */
    aimodDsp_g.pfWork=aimodDsp_g.pfDbBgn+lgcaidb_g.ulBufLen-lgcaidb_g.uiTotalCh;
    aimodDsp_g.pxWork=aimodDsp_g.pxDbBgn+calcaidb_g.ulBufLen-calcaidb_g.uiTotalCh;

    aimodExt_g.pfWork=aimodExt_g.pfDbBgn+lgcaidb_g.ulBufLen-lgcaidb_g.uiTotalCh;
    aimodExt_g.pxWork=aimodExt_g.pxDbBgn+calcaidb_g.ulBufLen-calcaidb_g.uiTotalCh;

    aimodRedun_g.pfWork=aimodRedun_g.pfDbBgn+lgcaidb_g.ulBufLen-lgcaidb_g.uiTotalCh; /* for excite */
    aimodRedun_g.pxWork=aimodRedun_g.pxDbBgn+calcaidb_g.ulBufLen-calcaidb_g.uiTotalCh;


    /*2006-2-10  */
    aimodOpt_g[0].pfWork=aimodOpt_g[0].pfDbBgn+lgcaidb_g.ulBufLen-lgcaidb_g.uiTotalCh;
    aimodOpt_g[0].pxWork=aimodOpt_g[0].pxDbBgn+calcaidb_g.ulBufLen-calcaidb_g.uiTotalCh;
    /*2006-2-10  */
    aimodOpt_g[1].pfWork=aimodOpt_g[1].pfDbBgn+lgcaidb_g.ulBufLen-lgcaidb_g.uiTotalCh;
    aimodOpt_g[1].pxWork=aimodOpt_g[1].pxDbBgn+calcaidb_g.ulBufLen-calcaidb_g.uiTotalCh;
    /*2007-3-28 张云  */
    aimodPole_g.pfWork=aimodPole_g.pfDbBgn+lgcaidb_g.ulBufLen-lgcaidb_g.uiTotalCh;
    aimodPole_g.pxWork=aimodPole_g.pxDbBgn+calcaidb_g.ulBufLen-calcaidb_g.uiTotalCh;

    for (i=0; i<MAX_VT_BOX_COUNT; i++)
    {
        /* 当前使用位置 */
        aimodVtBox_g[i].pfWork = aimodVtBox_g[i].pfDbBgn+lgcaidb_g.ulBufLen-lgcaidb_g.uiTotalCh;		/* 最后一个点 */
        aimodVtBox_g[i].pxWork = aimodVtBox_g[i].pxDbBgn+calcaidb_g.ulBufLen-calcaidb_g.uiTotalCh;	/* 最后一个点 */
    }

    aimodDsp_g.pfDcWork = aimodDsp_g.pfDcDbBgn + lgcaidb_g.ulBufLen-lgcaidb_g.uiTotalCh;		/* cycle address. */

    /* 智能操作箱目前无AI. 2007-3-28,张云*/
    aimodHdl_g.pfWork=aimodHdl_g.pfDbBgn+lgcaidb_g.ulBufLen-lgcaidb_g.uiTotalCh;
    aimodHdl_g.pxWork=aimodHdl_g.pxDbBgn+calcaidb_g.ulBufLen-calcaidb_g.uiTotalCh;


    for (paich=plgcaich_g; paich<plgcaich_g+iLgcAiChNum_g-iVtAiChNum_g; paich++)
    {

        assert(paich->phwai);           /* Not virtual AI. */
        if (IS_REAL_AI(paich->ucUnit))
        {
            assert(paich->ucFiltTp==8 || paich->ucFiltTp==0 ||
                   paich->ucFiltTp==0x0A || (paich->ucFiltTp == 0x18));		/* 增加定点算法 */

            if ((paich->phwai->paimod==&aimodDsp_g) && (paich->phwai->p_part->ucAiModType == RD_AI_AC))	/* 交流模件 */
                RD_Loc_Lgc_Ai(paich, &aimodDsp_g, pdspl_cfg_g);
            else if (paich->phwai->paimod==&aimodExt_g)
                RD_Loc_Lgc_Ai(paich, &aimodExt_g, pdspl_cfg_ext_g);
            else if (paich->phwai->paimod==&aimodRedun_g)
                RD_Loc_Lgc_Ai(paich, &aimodRedun_g, pdspl_cfg_redun_g);
            else if (paich->phwai->paimod == &aimodOpt_g[0])   /*2006-2-10  */
                RD_Loc_Lgc_Ai(paich, &aimodOpt_g[0], pdspl_cfg_Opt_g[0]);
            else if (paich->phwai->paimod == &aimodOpt_g[1])   /*2006-2-10  */
                RD_Loc_Lgc_Ai(paich, &aimodOpt_g[1], pdspl_cfg_Opt_g[1]);
            else if (paich->phwai->paimod == &aimodPole_g)   /*2007-3-28  */
                RD_Loc_Lgc_Ai(paich, &aimodPole_g, pdspl_cfg_Pole_g);
            else if (paich->phwai->paimod == &aimodHdl_g)   /*2007-3-28  */
                RD_Loc_Lgc_Ai(paich, &aimodHdl_g, pdspl_cfg_Hdl_g);
            else if ((paich->phwai->paimod >= &(aimodVtBox_g[0])) && (paich->phwai->paimod < &(aimodVtBox_g[MAX_VT_BOX_COUNT])))
            {
                RD_Loc_Lgc_Ai(paich, &aimodVtBox_g[paich->phwai->ucVtBoxPos], pdspl_cfg_Virt_g[paich->phwai->ucVtBoxPos]);
            }
            else if ((paich->phwai->paimod == &aimodDsp_g)
                     && (paich->phwai->p_part->ucAiModType == RD_AI_DC))  /* DC configuration. */
            {
                RD_Loc_Dc_Ai(paich, &aimodDsp_g, pdspl_dc_cfg_g);
            }
            else
            {
                assert(FALSE);
            }
        }
        else
        {
            assert(IS_CPLX_AI(paich->ucUnit));

            assert((paich->ucFiltTp & 0x0F)>=1 && (paich->ucFiltTp & 0x0F)<=7);

            if ((paich->phwai->paimod == &aimodDsp_g) && (paich->phwai->p_part->ucAiModType == RD_AI_AC))	/* 交流模件 */
                RD_Loc_Calc_Ai(paich, &aimodDsp_g, pdspc_cfg_g, pdspl_cfg_g);
            else if (paich->phwai->paimod==&aimodExt_g)
                RD_Loc_Calc_Ai(paich, &aimodExt_g, pdspc_cfg_ext_g, pdspl_cfg_ext_g);
            else if (paich->phwai->paimod==&aimodRedun_g)
                RD_Loc_Calc_Ai(paich, &aimodRedun_g, pdspc_cfg_redun_g, pdspl_cfg_redun_g);
            else if (paich->phwai->paimod == &aimodOpt_g[0])    /*2006-2-10  */
                RD_Loc_Calc_Ai(paich, &aimodOpt_g[0], pdspc_cfg_Opt_g[0], pdspl_cfg_Opt_g[0]);
            else if (paich->phwai->paimod == &aimodOpt_g[1])   /*2006-2-10  */
                RD_Loc_Calc_Ai(paich, &aimodOpt_g[1], pdspc_cfg_Opt_g[1], pdspl_cfg_Opt_g[1]);
            else if (paich->phwai->paimod == &aimodPole_g)   /*2007-3-28  */
                RD_Loc_Calc_Ai(paich, &aimodPole_g, pdspc_cfg_Pole_g, pdspl_cfg_Pole_g);
            else if (paich->phwai->paimod == &aimodHdl_g)   /*2007-3-28  */
                RD_Loc_Calc_Ai(paich, &aimodHdl_g, pdspc_cfg_Hdl_g, pdspl_cfg_Hdl_g);
            else if ((paich->phwai->paimod >= &(aimodVtBox_g[0])) && (paich->phwai->paimod < &(aimodVtBox_g[MAX_VT_BOX_COUNT])))
            {
                RD_Loc_Calc_Ai(paich, &aimodVtBox_g[paich->phwai->ucVtBoxPos], pdspc_cfg_Virt_g[paich->phwai->ucVtBoxPos],
                               pdspl_cfg_Virt_g[paich->phwai->ucVtBoxPos]);
            }
            else if ((paich->phwai->paimod == &aimodDsp_g)
                     && (paich->phwai->p_part->ucAiModType == RD_AI_DC))
            {
                /* DC configuration. */
                LOG_Dbg_Msg("直流模件配置.\n", 0, 0, 0, 0, 0, 0);
            }
            else
            {
                assert(FALSE);
            }
        }
    }

    for(paichmsu = pmsuaich_g; paichmsu<pmsuaich_g+iMsuAiChNum_g; paichmsu++)
    {
        /* 测量配置 */
        assert(paichmsu->phwai);
        assert(paichmsu->ucFiltTp < 7);

        RD_Loc_Msuc_Ai(paichmsu, &aimodDsp_g, pdspm_cfg_g, pdspl_cfg_g);
    }

    pfVtAI=lgcaidb_g.pfBufBgn+lgcaidb_g.uiTotalCh-1;
    pxVtAI=calcaidb_g.pxBufBgn+calcaidb_g.uiTotalCh-1;

    for (paich=plgcaich_g+iLgcAiChNum_g-1;
            paich>=plgcaich_g+iLgcAiChNum_g-iVtAiChNum_g; paich--)
    {
        /* 配置虚拟通道 */
        assert(!paich->phwai);          /* Virtual AI. */
        if (IS_REAL_AI(paich->ucUnit))
            paich->pdat.pfLgcAI=pfVtAI--;
        else
        {
            assert(IS_CPLX_AI(paich->ucUnit));
            paich->pdat.pxCalcAI=pxVtAI--;
        }
    }

    /*初始化AI有效性DB和MOD信息,张云添加  */
    aivaliddb_g.uiTotalCh=6+MAX_VT_BOX_COUNT;							/*主和扩展机箱每各采样点各分配一个标志，冗余机箱, 光纵1, 光纵2, 同杆并架各分配一个标志   */
    aivaliddb_g.ulBufLen=((uint32_t)uiAiRate_g/uiPwrFreq_g)*aivaliddb_g.uiTotalCh*
                         (g_RdBufCyc+1);
    if ((aivaliddb_g.pbBufBgn=calloc(aivaliddb_g.ulBufLen, sizeof(BOOL)))==NULL)
        return EP_BUF_ERR;
    aivaliddb_g.pbBufEnd=aivaliddb_g.pbBufBgn+aivaliddb_g.ulBufLen;

    aimodDsp_g.pbAiValidDbBgn=aivaliddb_g.pbBufBgn;			/* 建立数据有效标志指针 */
    aimodExt_g.pbAiValidDbBgn=aivaliddb_g.pbBufBgn+1;
    aimodRedun_g.pbAiValidDbBgn=aivaliddb_g.pbBufBgn+2;		/* for excite */

    aimodOpt_g[0].pbAiValidDbBgn=aivaliddb_g.pbBufBgn+3;
    aimodOpt_g[1].pbAiValidDbBgn=aivaliddb_g.pbBufBgn+4;
    aimodPole_g.pbAiValidDbBgn=aivaliddb_g.pbBufBgn+5;
    aimodHdl_g.pbAiValidDbBgn=NULL;

    for (i=0; i<MAX_VT_BOX_COUNT; i++)
    {
        /* 数据有效性,只关注采样点,不关注通道 */
        aimodVtBox_g[i].pbAiValidDbBgn=aivaliddb_g.pbBufBgn+6+i;		/* 机箱数据有效性 */
    }

    aimodHdl_g.pbAiValidDbBgn=NULL;


    aimodDsp_g.pbAiValidDbWork=aimodDsp_g.pbAiValidDbBgn+aivaliddb_g.ulBufLen-aivaliddb_g.uiTotalCh;
    aimodExt_g.pbAiValidDbWork=aimodExt_g.pbAiValidDbBgn+aivaliddb_g.ulBufLen-aivaliddb_g.uiTotalCh;
    aimodRedun_g.pbAiValidDbWork=aimodRedun_g.pbAiValidDbBgn+aivaliddb_g.ulBufLen-aivaliddb_g.uiTotalCh; /* for excite */

    aimodOpt_g[0].pbAiValidDbWork=aimodOpt_g[0].pbAiValidDbBgn+aivaliddb_g.ulBufLen-aivaliddb_g.uiTotalCh;
    aimodOpt_g[1].pbAiValidDbWork=aimodOpt_g[1].pbAiValidDbBgn+aivaliddb_g.ulBufLen-aivaliddb_g.uiTotalCh;

    aimodOpt_g[0].pbAiValidDbEnd = aimodOpt_g[0].pbAiValidDbBgn+aivaliddb_g.ulBufLen;
    aimodOpt_g[1].pbAiValidDbEnd = aimodOpt_g[1].pbAiValidDbBgn+aivaliddb_g.ulBufLen;

    aimodPole_g.pbAiValidDbWork=aimodPole_g.pbAiValidDbBgn+aivaliddb_g.ulBufLen-aivaliddb_g.uiTotalCh;
    aimodHdl_g.pbAiValidDbWork=NULL;

    for (i=0; i<MAX_VT_BOX_COUNT; i++)
    {
        /* 只关注机箱,每个机箱一个位置,不关注通道 */
        aimodVtBox_g[i].pbAiValidDbWork=aimodVtBox_g[i].pbAiValidDbBgn+aivaliddb_g.ulBufLen-aivaliddb_g.uiTotalCh;
    }

    aimodHdl_g.pbAiValidDbWork=NULL;

    /*初始化光纵通道状态DB和MOD信息，2007-3-28  */

#if defined(EDP_01_02_BUILD) || defined(EDP03_BUILD)
    optstsdb_g.uiTotalCh=2;/*主和扩展机箱不分配，光纵1，光纵2每各采样点各分配一个标志  2006-2-10*/
    optstsdb_g.ulBufLen=((uint32_t)uiAiRate_g/uiPwrFreq_g)*optstsdb_g.uiTotalCh*
                        (g_RdBufCyc+1);
    optstsdb_g.uiChBytes=optstsdb_g.uiTotalCh*sizeof(OPT_CH_STS)/sizeof(uint8_t);
    optstsdb_g.ulBufBytes=optstsdb_g.ulBufLen*sizeof(OPT_CH_STS)/sizeof(uint8_t);
    if ((optstsdb_g.pBufBgn=calloc(optstsdb_g.ulBufLen, sizeof(OPT_CH_STS)))==NULL)
        return EP_BUF_ERR;
    optstsdb_g.pBufEnd=optstsdb_g.pBufBgn+optstsdb_g.ulBufLen;
#endif

    aimodDsp_g.pOptChStsDbBgn=NULL;
    aimodExt_g.pOptChStsDbBgn=NULL;
#if defined(EDP_01_02_BUILD) || defined(EDP03_BUILD)
    aimodOpt_g[0].pOptChStsDbBgn=optstsdb_g.pBufBgn;
    aimodOpt_g[1].pOptChStsDbBgn=optstsdb_g.pBufBgn+1;

    aimodOpt_g[0].pOptChStsDbEnd = aimodOpt_g[0].pOptChStsDbBgn+optstsdb_g.ulBufLen;
    aimodOpt_g[1].pOptChStsDbEnd = aimodOpt_g[1].pOptChStsDbBgn+optstsdb_g.ulBufLen;
#endif

    aimodPole_g.pOptChStsDbBgn=NULL;
    aimodHdl_g.pOptChStsDbBgn=NULL;

    aimodDsp_g.pOptChStsDbWork=NULL;
    aimodExt_g.pOptChStsDbWork=NULL;
#if defined(EDP_01_02_BUILD) || defined(EDP03_BUILD)
    aimodOpt_g[0].pOptChStsDbWork=aimodOpt_g[0].pOptChStsDbBgn+optstsdb_g.ulBufLen-optstsdb_g.uiTotalCh;
    aimodOpt_g[1].pOptChStsDbWork=aimodOpt_g[1].pOptChStsDbBgn+optstsdb_g.ulBufLen-optstsdb_g.uiTotalCh;
#endif

    aimodPole_g.pOptChStsDbWork=NULL;
    aimodHdl_g.pOptChStsDbWork=NULL;

    return stsRet;
}

/***********************************************************************
* RD_Init_DI_Db - 初始化DI数据缓冲区
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
EP_STATUS RD_Init_DI_Db(void)
{
    RD_LGC_DI_CH *plgcdi;
    BOOL *pb;
    EP_STATUS stsRet;
    char TempInfo[256];

    stsRet=EP_SUCCESS;
    // return stsRet;// 先屏蔽掉数字量输入初始化，待调试完毕再打开

    rdinfo_g.ulCurrAiCnt=-1;

    didb_g.uiTotalCh=iLgcDiChNum_g;
    didb_g.ulBufLen=((uint32_t)uiAiRate_g/uiPwrFreq_g)*
                    didb_g.uiTotalCh*(g_RdBufCyc+1);
    if ((didb_g.pbBufBgn=calloc(didb_g.ulBufLen, sizeof(BOOL)))==NULL)
        return EP_BUF_ERR;

    didb_g.pbBufEnd=didb_g.pbBufBgn+didb_g.ulBufLen;

    didb_g.pbWork=didb_g.pbBufBgn+didb_g.ulBufLen-didb_g.uiTotalCh;  		/* Locate the work pointer to "-1" position. */

    pb=didb_g.pbBufBgn;
    for (plgcdi=plgcdich_g; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++)
    {
        plgcdi->ChOffset = pb - didb_g.pbBufBgn;
        if (plgcdi->mod==RD_SPI_DI)
        {
            /* 主机箱 */
            plgcdi->pbDI=pb++;
            plgcdi->iForceSts=-1;

            if (!SIO_GetIOExsitSts(plgcdi->p_part->aucHwAddr[0]))
            {
                plgcdi->pvSrc = NULL;
                if (!EP_IS_BOOT_SEL())		/* 用于EDP01和EDP02平台调试，防止RD_Init_DI_Db函数之后的函数不初始化会导致装置重启 */
                {
                    /* 如果是发布版，则返回错误 */
                    stsRet = EP_CFG_ERR;
                }

                continue;
            }

            plgcdi->pvSrc=SIO_Init_DI(plgcdi->p_part->aucHwAddr[0],					  /* 模件硬件地址，物理通道号，去抖动时间，返回DI通道结构 */
                                      plgcdi->ucModCh,
                                      plgcdi->ulFiltTime,
                                      plgcdi->bDIInvalidDftVal);
            if (!plgcdi->pvSrc)
            {
                if(ENG_MODE == 0)
                {
                    sprintf(TempInfo,"模件地址:%d,通道名称:%s",
                            plgcdi->p_part->aucHwAddr[0]+1, (char *)plgcdi->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, IO_MOD_DI_INIT_ERR);
                }
                else if(ENG_MODE == 1)
                {
                    sprintf(TempInfo,"Module address:%d,channel:%s",
                            plgcdi->p_part->aucHwAddr[0]+1,
                            (char *)plgcdi->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, IO_MOD_DI_INIT_ERR);
                }
                sprintf(TempInfo,"输入通道初始化异常,模件地址:%d,通道名称:%s!!\n",
                        plgcdi->p_part->aucHwAddr[0]+1,
                        (char *)plgcdi->aucId);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

                if(!EP_IS_BOOT_SEL())		/* 用于EDP01和EDP02平台调试，防止RD_Init_DI_Db函数之后的函数不初始化会导致装置重启 */
                {
                    /* 如果是发布版，则返回错误 */
                    stsRet=EP_CFG_ERR;
                }
            }
        }
        else if (plgcdi->mod==RD_EXT_DI) 					/* 扩展机箱去掉 */
        {
            plgcdi->pbDI=pb++;
            plgcdi->iForceSts=-1;
            plgcdi->pvSrc=EX_Init_DI(plgcdi->p_part->aucHwAddr[0],
                                     plgcdi->ucModCh, plgcdi->ulFiltTime);
            if (!plgcdi->pvSrc)
            {
                if(ENG_MODE == 0)
                {
                    sprintf(TempInfo,"模件地址:%d,通道名称:%s",
                            plgcdi->p_part->aucHwAddr[0]+1,(char *)plgcdi->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, EXT_IO_DI_INIT_ERR);
                }
                else if(ENG_MODE == 1)
                {
                    sprintf(TempInfo,"Module address:%d,channel:%s",
                            plgcdi->p_part->aucHwAddr[0]+1,(char *)plgcdi->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, EXT_IO_DI_INIT_ERR);
                }
                sprintf(TempInfo,"输入通道初始化异常,模件地址:%d,通道名称:%s!!\n",
                        plgcdi->p_part->aucHwAddr[0]+1,(char *)plgcdi->aucId);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

                stsRet=EP_CFG_ERR;
            }
        }

        else if (plgcdi->mod == RD_OPT1_DI)
        {
            plgcdi->pbDI=pb++;
            plgcdi->iForceSts=-1;
            plgcdi->pvSrc=OPT_Init_DI(0,plgcdi->p_part->aucHwAddr[0],
                                      plgcdi->ucModCh, plgcdi->ulFiltTime);
            if (!plgcdi->pvSrc)
            {
                if(ENG_MODE == 0)
                {
                    sprintf(TempInfo,"模件地址:%d,通道名称:%s",
                            plgcdi->p_part->aucHwAddr[0]+1, (char *)plgcdi->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, FIBER_IO_DI_INIT_ERR);
                }
                else if(ENG_MODE == 1)
                {
                    sprintf(TempInfo,"Module address:%d,channel:%s",
                            plgcdi->p_part->aucHwAddr[0]+1, (char *)plgcdi->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, FIBER_IO_DI_INIT_ERR);
                }
                sprintf(TempInfo,"输入通道初始化异常,模件地址:%d,通道名称:%s!!\n",
                        plgcdi->p_part->aucHwAddr[0]+1, (char *)plgcdi->aucId);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);
                stsRet=EP_CFG_ERR;
            }
        }
        else if (plgcdi->mod==RD_OPT2_DI)
        {
            plgcdi->pbDI=pb++;
            plgcdi->iForceSts=-1;
            plgcdi->pvSrc=OPT_Init_DI(1,plgcdi->p_part->aucHwAddr[0],
                                      plgcdi->ucModCh, plgcdi->ulFiltTime);
            if (!plgcdi->pvSrc)
            {
                if(ENG_MODE == 0)
                {
                    sprintf(TempInfo,"模件地址:%d,通道名称:%s",
                            plgcdi->p_part->aucHwAddr[0]+1, (char *)plgcdi->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, FIBER_IO_DI_INIT_ERR);
                }
                else if(ENG_MODE == 1)
                {
                    sprintf(TempInfo,"Module address:%d,channel:%s",
                            plgcdi->p_part->aucHwAddr[0]+1, (char *)plgcdi->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(02d)\n",
                               (int)TempInfo, FIBER_IO_DI_INIT_ERR);
                }
                sprintf(TempInfo,"输入通道初始化异常,模件地址:%d,通道名称:%s!!\n",
                        plgcdi->p_part->aucHwAddr[0]+1, (char *)plgcdi->aucId);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

                stsRet=EP_CFG_ERR;
            }
        }

#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)
        else if (plgcdi->mod==RD_SAME_POLE_DI)/*2007-3-28日 张云　*/
        {
            plgcdi->pbDI=pb++;
            plgcdi->iForceSts=-1;
            plgcdi->pvSrc=POLE_Init_DI(plgcdi->p_part->aucHwAddr[0],
                                       plgcdi->ucModCh, plgcdi->ulFiltTime);
            if (!plgcdi->pvSrc)
            {

                if(ENG_MODE == 0)
                {
                    sprintf(TempInfo,"模件地址:%d,通道名称:%s",
                            plgcdi->p_part->aucHwAddr[0]+1, (char *)plgcdi->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, PTL_DI_INIT_ERR);
                }
                else if(ENG_MODE == 1)
                {
                    sprintf(TempInfo,"Module address:%d,channel:%s",
                            plgcdi->p_part->aucHwAddr[0]+1, (char *)plgcdi->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(02d)\n",
                               (int)TempInfo, PTL_DI_INIT_ERR);
                }
                sprintf(TempInfo,"输入通道初始化异常,模件地址:%d,通道名称:%s!!\n",
                        plgcdi->p_part->aucHwAddr[0]+1, (char *)plgcdi->aucId);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

                stsRet=EP_CFG_ERR;
            }
        }
        else if (plgcdi->mod==RD_HDL_BOX_DI)/*2007-3-28日 张云　*/
        {
            plgcdi->pbDI=pb++;
            plgcdi->iForceSts=-1;
            plgcdi->pvSrc=HDL_Init_DI(plgcdi->p_part->aucHwAddr[0],
                                      plgcdi->ucModCh, plgcdi->ulFiltTime, plgcdi->bDIInvalidDftVal,
                                      plgcdi->ucDIRefreshRate, &plgcdi->bPended);

            if (!plgcdi->pvSrc)
            {
                if(ENG_MODE == 0)
                {
                    sprintf(TempInfo,"模件地址:%d,通道名称:%s",
                            plgcdi->p_part->aucHwAddr[0]+1, (char *)plgcdi->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, IOB_DI_INIT_ERR);
                }
                else if(ENG_MODE == 1)
                {
                    sprintf(TempInfo,"Module address:%d,channel:%s",
                            plgcdi->p_part->aucHwAddr[0]+1, (char *)plgcdi->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, IOB_DI_INIT_ERR);
                }
                sprintf(TempInfo,"输入通道初始化异常,模件地址:%d,通道名称:%s!!\n",
                        plgcdi->p_part->aucHwAddr[0]+1, (char *)plgcdi->aucId);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

                stsRet=EP_CFG_ERR;
            }
        }
#endif
        else if (plgcdi->mod == RD_VT_BOX_DI)
        {
            assert (FALSE);
        }
        else
            assert(FALSE);
    }
    if(uiAppType_g == APP_LINE || uiAppType_g == APP_BUS)
        HDL_Set_LineFilt();

    return stsRet;
}

/***********************************************************************
* RD_Init_DO_Db - 初始化DO数据缓冲区
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
EP_STATUS RD_Init_DO_Db(void)
{
    RD_LGC_DO_CH *plgcdo;
    EP_STATUS stsRet;
    char TempInfo[256];

    stsRet=EP_SUCCESS;
    return stsRet; // 先屏蔽掉数字量输出初始化，待调试完毕再打开
    for (plgcdo=plgcdoch_g; plgcdo<plgcdoch_g+iLgcDoChNum_g; plgcdo++)
    {
        if (plgcdo->mod==RD_SPI_DO)
        {
            if (!SIO_GetIOExsitSts(plgcdo->p_part->aucHwAddr[0]))
            {
                plgcdo->pvSrc = NULL;
                if (!EP_IS_BOOT_SEL())		/* 用于EDP01和EDP02平台调试 */
                    stsRet = EP_CFG_ERR;

                continue;
            }

            plgcdo->pvSrc=SIO_Init_DO(plgcdo->p_part->aucHwAddr[0], plgcdo->ucModCh);		/* 模件地址，物理通道号，返回DO通道结构 */
            if (!plgcdo->pvSrc)
            {
                if(ENG_MODE == 0)
                {
                    sprintf(TempInfo,"模件地址:%d,通道名称:%s",
                            plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, IO_MOD_DO_INIT_ERR);
                }
                else if(ENG_MODE == 1)
                {
                    sprintf(TempInfo,"Module address:%d,channel:%s",
                            plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, IO_MOD_DO_INIT_ERR);
                }

                sprintf(TempInfo,"输出通道初始化异常,模件地址:%d,通道名称:%s!!\n",
                        plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

                if(!EP_IS_BOOT_SEL())		/* 用于EDP01和EDP02平台调试 */
                    stsRet=EP_CFG_ERR;
            }
            plgcdo->iForceSts=-1;
        }
#if defined(EXCITE_BUILD)
        else if (plgcdo->mod == RD_REDUN_DO)
        {
            /* 模件地址，物理通道号，返回DO通道结构 */
            plgcdo->pvSrc=Redun_Init_DO(plgcdo->p_part->aucHwAddr[0], plgcdo->ucModCh);
            if (!plgcdo->pvSrc)
            {

                if(ENG_MODE == 0)
                {
                    sprintf(TempInfo,"模件地址:%d,通道名称:%s",
                            plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, REDUN_DO_INIT_ERR);
                }
                else if(ENG_MODE == 1)
                {
                    sprintf(TempInfo,"Module address:%d,channel:%s",
                            plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, REDUN_DO_INIT_ERR);
                }
                sprintf(TempInfo,"输出通道初始化异常,模件地址:%d,通道名称:%s!!\n",
                        plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

                stsRet=EP_CFG_ERR;
            }
            plgcdo->iForceSts=-1;
        }
#endif

        else if (plgcdo->mod==RD_OPT1_DO)
        {
            plgcdo->pvSrc=OPT_Init_DO(0,plgcdo->p_part->aucHwAddr[0], plgcdo->ucModCh);
            if (!plgcdo->pvSrc)
            {
                if(ENG_MODE == 0)
                {
                    sprintf(TempInfo,"模件地址:%d,通道名称:%s",
                            plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, FIBER_IO_DO_INIT_ERR);
                }
                else if(ENG_MODE == 1)
                {
                    sprintf(TempInfo,"Module address:%d,channel:%s",
                            plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, FIBER_IO_DO_INIT_ERR);
                }
                sprintf(TempInfo,"输出通道初始化异常,模件地址:%d,通道名称:%s!!\n",
                        plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

                stsRet=EP_CFG_ERR;
            }
            plgcdo->iForceSts=-1;
        }
        else if (plgcdo->mod==RD_OPT2_DO)
        {
            plgcdo->pvSrc=OPT_Init_DO(1,plgcdo->p_part->aucHwAddr[0], plgcdo->ucModCh);
            if (!plgcdo->pvSrc)
            {
                if(ENG_MODE == 0)
                {
                    sprintf(TempInfo,"模件地址:%d,通道名称:%s",
                            plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, FIBER_IO_DO_INIT_ERR);
                }
                else if(ENG_MODE == 1)
                {
                    sprintf(TempInfo,"Module address:%d,channel:%s",
                            plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, FIBER_IO_DO_INIT_ERR);
                }
                sprintf(TempInfo,"输出通道初始化异常,模件地址:%d,通道名称:%s!!\n",
                        plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

                stsRet=EP_CFG_ERR;
            }
            plgcdo->iForceSts=-1;
        }

#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)
        else if (plgcdo->mod==RD_SAME_POLE_DO)/*2007-3-29日 张云　*/
        {
            plgcdo->pvSrc=POLE_Init_DO(plgcdo->p_part->aucHwAddr[0], plgcdo->ucModCh);
            if (!plgcdo->pvSrc)
            {
                if(ENG_MODE == 0)
                {
                    sprintf(TempInfo,"模件地址:%d,通道名称:%s",
                            plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, PTL_DO_INIT_ERR);
                }
                else if(ENG_MODE == 1)
                {
                    sprintf(TempInfo,"Module address:%d,channel:%s",
                            plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, PTL_DO_INIT_ERR);
                }
                sprintf(TempInfo,"输出通道初始化异常,模件地址:%d,通道名称:%s!!\n",
                        plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

                stsRet=EP_CFG_ERR;
            }
            plgcdo->iForceSts=-1;
        }
        else if (plgcdo->mod==RD_HDL_BOX_DO)/*2007-3-29日 张云　*/
        {
            plgcdo->pvSrc=HDL_Init_DO(plgcdo->p_part->aucHwAddr[0], plgcdo->ucModCh);
            if (!plgcdo->pvSrc)
            {
                if(ENG_MODE == 0)
                {
                    sprintf(TempInfo,"模件地址:%d,通道名称:%s",
                            plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, IOB_DO_INIT_ERR);
                }
                else if(ENG_MODE == 1)
                {
                    sprintf(TempInfo,"Module address:%d,channel:%s",
                            plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                    ER_Set_Err(EV_DIDO_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                               "%s(%02d)\n",
                               (int)TempInfo, IOB_DO_INIT_ERR);
                }
                sprintf(TempInfo,"输出通道初始化异常,模件地址:%d,通道名称:%s!!\n",
                        plgcdo->p_part->aucHwAddr[0]+1, (char *)plgcdo->aucId);
                LOG_Write(LOG_KERNEL, TempInfo, NULL);

                stsRet=EP_CFG_ERR;
            }
            plgcdo->iForceSts=-1;
        }
#endif
        else if (plgcdo->mod == RD_VT_BOX_DO)
        {
            assert (FALSE);
        }

        else
            assert(FALSE);
    }

    return stsRet;
}

/* 初始化GOOSE开出通道压板.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or EP_FALSE.
 */
BOOL RD_InitLinkofGsDo(void)
{
    RD_LGC_DO_CH *plgcdo;

    for (plgcdo = plgcdoch_g; plgcdo<plgcdoch_g+iLgcDoChNum_g; plgcdo++)
    {
        if (plgcdo->mod == RD_HDL_BOX_DO)
        {
            plgcdo->linkNum = HDL_CfgLinkofDo(plgcdo->pvSrc);
        }
        else
        {
            plgcdo->linkNum = -1;
        }
    }

    return TRUE;
}

/***********************************************************************
* RD_Init_AO_Db - 初始化AO库，目前只允许光纵，此时只初始化AI来源的AO
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
EP_STATUS RD_Init_AO_Db(void)
{
    RD_HW_AO_CH *pch;
    int  i;

    pch=phwaoch_g;
    for (i=0; i<iHwAoChNum_g; i++)
    {
        if(pch->paimod==&(aimodOpt_g[0]))
        {
            if(pch->iAOSrcType==DA_AI_SRC)
            {
                if(OPT_Init_AI_Src_AO(0, pch->iAOSrcType,pch->ucModCh,((RD_HW_AI_CH *)(pch->pvSrc))->ucModCh,((RD_HW_AI_CH *)(pch->pvSrc))->fCoff)!=EP_SUCCESS)/* 2006-11-26日张云修改 */
                {
                    assert(FALSE);
                    return   EP_CFG_ERR;
                }
            }
        }
        else  if(pch->paimod==&(aimodOpt_g[1]))
        {
            if(pch->iAOSrcType==DA_AI_SRC)
            {
                if(OPT_Init_AI_Src_AO(1, pch->iAOSrcType, pch->ucModCh,((RD_HW_AI_CH *)(pch->pvSrc))->ucModCh,((RD_HW_AI_CH *)(pch->pvSrc))->fCoff)!=EP_SUCCESS)/* 2006-11-26日张云修改 */
                {
                    assert(FALSE);
                    return   EP_CFG_ERR;
                }
            }
        }
        else  if(pch->paimod==&(aimodPole_g))/*2007-5-16日  */
        {

        }
        else  if(pch->paimod==&(aimodHdl_g))/*2007-5-16日  */
        {

        }
        else if ((pch->paimod >= &(aimodVtBox_g[0])) && (pch->paimod < &(aimodVtBox_g[MAX_VT_BOX_COUNT])))
        {
            if(pch->iAOSrcType==DA_AI_SRC)
            {
                /* 初始化AI来源的AO */
                if(VirtBox_Init_AI_Src_AO(pch->ucVtBoxPos, pch->iAOSrcType, pch->ucModCh, ((RD_HW_AI_CH *)(pch->pvSrc))->ucModCh, ((RD_HW_AI_CH *)(pch->pvSrc))->fCoff) != EP_SUCCESS)
                {
                    assert(FALSE);
                    return EP_CFG_ERR;
                }
            }
            else if (pch->iAOSrcType == DA_PRE_SRC)
            {
                if (VirtBox_Init_Pro_Src_AO(pch->ucVtBoxPos, pch->iAOSrcType, pch->ucSrcType, pch->ucModCh,
                                            ((RD_HW_AI_CH *)(((RD_LGC_AI_CH *)(pch->pvSrc))->phwai))->ucModCh, ((RD_LGC_AI_CH *)(pch->pvSrc))->ucFiltTp, ((RD_LGC_AI_CH *)(pch->pvSrc))->ucUnit) != EP_SUCCESS)
                {
                    assert (FALSE);

                    return EP_CFG_ERR;

                }
            }
        }
        else
        {
            assert(FALSE);
            return   EP_CFG_ERR;
        }

        pch++;
    }
    return   EP_SUCCESS;
}

/***********************************************************************
* RD_Chg_AO_Coff - 初始化AI来源AO的系数
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
EP_STATUS RD_Chg_AO_Coff(void)
{
    RD_HW_AO_CH *pch;
    int i;

    pch=phwaoch_g;

    for(i=0; i<2; i++)
    {
#if defined(EDP_01_02_BUILD) || defined(EDP03_BUILD)
        OPT_InitAOCfgCount(i);
#endif
    }

    for (i=0; i<MAX_VT_BOX_COUNT; i++)
    {
        VirtBox_InitAOCfgCount(i);
    }

    for (i=0; i<iHwAoChNum_g; i++)
    {
        if(pch->paimod == &(aimodOpt_g[0]))
        {
            if(pch->iAOSrcType == DA_AI_SRC)
            {
                if(OPT_Chg_AI_Src_AO_Coff(0, ((RD_HW_AI_CH *)(pch->pvSrc))->fCoff) != EP_SUCCESS)
                {
                    assert(FALSE);
                    return EP_CFG_ERR;
                }
            }
        }
        else if(pch->paimod == &(aimodOpt_g[1]))
        {
            if(pch->iAOSrcType == DA_AI_SRC)
            {
                if(OPT_Chg_AI_Src_AO_Coff(1, ((RD_HW_AI_CH *)(pch->pvSrc))->fCoff) != EP_SUCCESS)
                {
                    assert(FALSE);
                    return EP_CFG_ERR;
                }
            }
        }
        else if(pch->paimod == &(aimodPole_g))
        {
        }
        else if(pch->paimod == &(aimodHdl_g))
        {
        }
        else if ((pch->paimod >= &(aimodVtBox_g[0])) && (pch->paimod < &(aimodVtBox_g[MAX_VT_BOX_COUNT])))
        {
            if(pch->iAOSrcType == DA_AI_SRC)
            {
                /* 初始化AI来源AO的系数 */
                if(VirtBox_Chg_AI_Src_AO_Coff(pch->ucVtBoxPos, ((RD_HW_AI_CH *)(pch->pvSrc))->fCoff) != EP_SUCCESS)
                {
                    assert(FALSE);

                    return EP_CFG_ERR;
                }
            }
        }
        else
        {
            assert(FALSE);
            return EP_CFG_ERR;
        }

        pch++;
    }

    return EP_SUCCESS;
}

/***********************************************************************
* RD_Init_Dsp_Cfg - DSP计算与配置对应
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Init_Dsp_Cfg(void)
{
    RD_LGC_AI_CH *pch;
    EP_STATUS stsRet;
    int i;

    stsRet=EP_SUCCESS;

    if ((pdspl_cfg_g=calloc(DSP_LGC_AI_CH, sizeof(*pdspl_cfg_g)))==NULL)			/* 以下是DSP计算项目空间分配 */
        return EP_BUF_ERR;
    if ((pdspc_cfg_g=calloc(DSP_CALC_AI_CH, sizeof(*pdspc_cfg_g)))==NULL)
        return EP_BUF_ERR;

    if ((pdspl_cfg_ext_g=calloc(DSP_LGC_AI_CH, sizeof(*pdspl_cfg_ext_g)))==NULL)
        return EP_BUF_ERR;
    if ((pdspc_cfg_ext_g=calloc(DSP_CALC_AI_CH, sizeof(*pdspc_cfg_ext_g)))==NULL)
        return EP_BUF_ERR;

    /* for excite */
    if ((pdspl_cfg_redun_g=calloc(DSP_LGC_AI_CH, sizeof(*pdspl_cfg_redun_g)))==NULL)
        return EP_BUF_ERR;
    if ((pdspc_cfg_redun_g=calloc(DSP_CALC_AI_CH, sizeof(*pdspc_cfg_redun_g)))==NULL)
        return EP_BUF_ERR;

    /* 为光纵添加 */
    if ((pdspl_cfg_Opt_g[0]=calloc(DSP_LGC_AI_CH, sizeof(*pdspl_cfg_Opt_g[0])))==NULL)
        return EP_BUF_ERR;
    if ((pdspc_cfg_Opt_g[0]=calloc(DSP_CALC_AI_CH, sizeof(*pdspc_cfg_Opt_g[0])))==NULL)
        return EP_BUF_ERR;

    if ((pdspl_cfg_Opt_g[1]=calloc(DSP_LGC_AI_CH, sizeof(*pdspl_cfg_Opt_g[1])))==NULL)
        return EP_BUF_ERR;
    if ((pdspc_cfg_Opt_g[1]=calloc(DSP_CALC_AI_CH, sizeof(*pdspc_cfg_Opt_g[1])))==NULL)
        return EP_BUF_ERR;

    /*为同杆并架添加 */
    if ((pdspl_cfg_Pole_g=calloc(DSP_LGC_AI_CH, sizeof(*pdspl_cfg_Pole_g)))==NULL)
        return EP_BUF_ERR;
    if ((pdspc_cfg_Pole_g=calloc(DSP_CALC_AI_CH, sizeof(*pdspc_cfg_Pole_g)))==NULL)
        return EP_BUF_ERR;
    /*为智能操作箱添加 */
    if ((pdspl_cfg_Hdl_g=calloc(DSP_LGC_AI_CH, sizeof(*pdspl_cfg_Hdl_g)))==NULL)
        return EP_BUF_ERR;
    if ((pdspc_cfg_Hdl_g=calloc(DSP_CALC_AI_CH, sizeof(*pdspc_cfg_Hdl_g)))==NULL)
        return EP_BUF_ERR;
    for (i=0; i<MAX_VT_BOX_COUNT; i++)
    {
        /* 分配存储空间 */
        if ((pdspl_cfg_Virt_g[i] = calloc(DSP_LGC_AI_CH, sizeof(*pdspl_cfg_Virt_g[i]))) == NULL)
            return EP_BUF_ERR;
        if ((pdspc_cfg_Virt_g[i] = calloc(DSP_CALC_AI_CH, sizeof(*pdspc_cfg_Virt_g[i]))) == NULL)
            return EP_BUF_ERR;
    }

    if ((pdspl_dc_cfg_g=calloc(DSP_MSU_AI_CH, sizeof(*pdspl_dc_cfg_g))) == NULL)
    {
        /* DC processing structure. */
        return EP_BUF_ERR;
    }

    for (pch=plgcaich_g; pch<plgcaich_g+iLgcAiChNum_g; pch++)
    {
        /* 把配置放到DSP计算结构中 */
        if (pch->phwai)
        {
            /* 物理模件不为空 */
            if ((pch->phwai->paimod == &aimodDsp_g) && (pch->phwai->p_part->ucAiModType == RD_AI_AC)) 		/* 主机箱，aimodDsp_g为数据缓冲区；只包含交流通道 */
                RD_Add_Ai_Cfg(&aimodDsp_g, pch, pdspl_cfg_g, pdspc_cfg_g);
            else if (pch->phwai->paimod==&aimodExt_g) 		/* 外部机箱 */
                RD_Add_Ai_Cfg(&aimodExt_g, pch, pdspl_cfg_ext_g, pdspc_cfg_ext_g);
            /* 冗余机箱 */
            else if (pch->phwai->paimod==&aimodRedun_g) /* for excite */
                RD_Add_Ai_Cfg(&aimodRedun_g, pch, pdspl_cfg_redun_g, pdspc_cfg_redun_g);
            else if (pch->phwai->paimod==&aimodOpt_g[0])		/* 为光综添加 */
                RD_Add_Ai_Cfg(&aimodOpt_g[0], pch, pdspl_cfg_Opt_g[0], pdspc_cfg_Opt_g[0]);
            else if (pch->phwai->paimod==&aimodOpt_g[1])
                RD_Add_Ai_Cfg(&aimodOpt_g[1], pch, pdspl_cfg_Opt_g[1], pdspc_cfg_Opt_g[1]);
            else if (pch->phwai->paimod==&aimodPole_g) /*为同杆并架添加 */
                RD_Add_Ai_Cfg(&aimodPole_g, pch, pdspl_cfg_Pole_g, pdspc_cfg_Pole_g);
            else if (pch->phwai->paimod==&aimodHdl_g) /*为智能操作箱添加 */
                RD_Add_Ai_Cfg(&aimodHdl_g, pch, pdspl_cfg_Hdl_g, pdspc_cfg_Hdl_g);
            else if ((pch->phwai->paimod >= &(aimodVtBox_g[0])) && (pch->phwai->paimod < &(aimodVtBox_g[MAX_VT_BOX_COUNT])))
            {
                RD_Add_Ai_Cfg(&aimodVtBox_g[pch->phwai->ucVtBoxPos], pch,
                              pdspl_cfg_Virt_g[pch->phwai->ucVtBoxPos], pdspc_cfg_Virt_g[pch->phwai->ucVtBoxPos]);
            }
            else if ((pch->phwai->paimod == &aimodDsp_g)
                     && (pch->phwai->p_part->ucAiModType == RD_AI_DC))
            {
                /* DC channel in main box. */
                RD_Add_Dc_Ai_Cfg (&aimodDsp_g, pch, pdspl_dc_cfg_g);
            }
            else
            {
                assert(FALSE);
            }
        }
    }

    if (aimodDsp_g.iLgcNum)		/* 排序 */
        RD_Sort_Dsp_Cfg(&aimodDsp_g, pdspl_cfg_g, pdspc_cfg_g);

    if (aimodExt_g.iLgcNum)
        RD_Sort_Dsp_Cfg(&aimodExt_g, pdspl_cfg_ext_g, pdspc_cfg_ext_g);

    if (aimodRedun_g.iLgcNum)  /* for excite */
        RD_Sort_Dsp_Cfg(&aimodRedun_g, pdspl_cfg_redun_g, pdspc_cfg_redun_g);

    if (aimodOpt_g[0].iLgcNum)		/* 为光纵添加 */
        RD_Sort_Dsp_Cfg(&aimodOpt_g[0], pdspl_cfg_Opt_g[0], pdspc_cfg_Opt_g[0]);

    if (aimodOpt_g[1].iLgcNum)
        RD_Sort_Dsp_Cfg(&aimodOpt_g[1], pdspl_cfg_Opt_g[1], pdspc_cfg_Opt_g[1]);

    if (aimodPole_g.iLgcNum) /*为同杆并架添加   */
        RD_Sort_Dsp_Cfg(&aimodPole_g, pdspl_cfg_Pole_g, pdspc_cfg_Pole_g);
    if (aimodHdl_g.iLgcNum) /*为智能操作箱添加   */
        RD_Sort_Dsp_Cfg(&aimodHdl_g, pdspl_cfg_Hdl_g, pdspc_cfg_Hdl_g);

    for (i=0; i<MAX_VT_BOX_COUNT; i++)
    {
        if (aimodVtBox_g[i].iLgcNum) /* 虚拟机箱 */
            RD_Sort_Dsp_Cfg(&aimodVtBox_g[i], pdspl_cfg_Virt_g[i], pdspc_cfg_Virt_g[i]);
    }

    return stsRet;
}

/* 初始化时更新计算用通道系数
 * Para:
 *     NONE.
 * Return:
 *     NONE.
*/
void RDUpdateDspCfg(void)
{

    /* 主机箱 */
    DspCoeUpdate(phwaich_g, &aimodDsp_g, pdspl_cfg_g);
    /* 扩展机箱 */
    DspCoeUpdate(phwaich_g, &aimodExt_g, pdspl_cfg_ext_g);
    /* 励磁冗余机箱 */
    DspCoeUpdate(phwaich_g, &aimodRedun_g, pdspl_cfg_redun_g);
    /* 光纵通道0机箱 */
    DspCoeUpdate(phwaich_g, &aimodOpt_g[0], pdspl_cfg_Opt_g[0]);
    /* 光纵通道1机箱 */
    DspCoeUpdate(phwaich_g, &aimodOpt_g[1], pdspl_cfg_Opt_g[1]);
    /* 同杆并架机箱 */
    DspCoeUpdate(phwaich_g, &aimodPole_g, pdspl_cfg_Pole_g);
}

/***********************************************************************
* RD_Init_Dsp_Msu_Cfg - 为测量计算结构及结果存储区分配存储空间，并逐项添加测量计算结构
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*
*/
static EP_STATUS RD_Init_Dsp_Msu_Cfg(void)
{
    EP_STATUS stsRet;
    RD_MSU_AI_CH *pch; 			/* 测量有关文件配置结构 */

    stsRet=EP_SUCCESS;
    if ((pdspm_cfg_g=calloc(DSP_MSU_AI_CH, sizeof(*pdspm_cfg_g)))==NULL) 		/*测量计算结构内存分配 */
        return EP_BUF_ERR;

    msucaidb_g.uiTotalCh = DSP_MSU_AI_CH;			/* 测量计算结果存储空间赋值 */
    /* msucaidb_g.ulBufLen = ((uint32_t)uiAiRate_g/uiPwrFreq_g)*msucaidb_g.uiTotalCh; */
    msucaidb_g.ulBufLen = (uint32_t)msucaidb_g.uiTotalCh;
    msucaidb_g.uiChBytes=msucaidb_g.uiTotalCh*sizeof(COMPLEX)/sizeof(uint8_t);
    msucaidb_g.ulBufBytes=msucaidb_g.ulBufLen*sizeof(COMPLEX)/sizeof(uint8_t);

    if ((msucaidb_g.pxBufBgn=calloc(2*msucaidb_g.ulBufLen, sizeof(COMPLEX))) == NULL)		/* 测量结果存储空间分配 */
        return EP_BUF_ERR;

    aimodDsp_g.pxmsuDbBgn = msucaidb_g.pxBufBgn;		/* 起始地址 */

    for(pch = pmsuaich_g; pch<pmsuaich_g+iMsuAiChNum_g; pch++)
    {
        /* 测量有关文件配置结构解析 */
        RD_Add_Ai_Msu_Cfg(&aimodDsp_g, pch, pdspl_cfg_g, pdspm_cfg_g);
    }

#ifndef NO_DEBUG
    for(i=0; i<aimodDsp_g.iMsucNum; i++)
        LOG_Dbg_Msg("MSU Channel 3: %d\n",pdspm_cfg_g[i].ucBgnLgcCh,0,0,0,0,0);
#endif

    if(aimodDsp_g.iMsucNum)			/* 排序 */
        RD_Sort_Dsp_Msu_Cfg(&aimodDsp_g, pdspl_cfg_g, pdspm_cfg_g);

#ifndef NO_DEBUG
    for(i=0; i<aimodDsp_g.iMsucNum; i++)
        LOG_Dbg_Msg("MSU Channel 3: %d\n",pdspm_cfg_g[i].ucBgnLgcCh,0,0,0,0,0);
#endif

    return stsRet;

}

/***********************************************************************
* RD_Loc_Hw_Ai - 为物理通道找结果存储地址
*
* RETURNS: 无
*
*
*/
static void RD_Loc_Hw_Ai(
    RD_HW_AI_CH *phwai,		/* AI物理通道 */
    RD_AI_MOD *paimod, 					/* 机箱 */
    DSP_LGC_AI_CFG *pdspl_cfg				/* DSP配置 */
)
{
    DSP_LGC_AI_CFG *plcfg;

    for (plcfg=pdspl_cfg; plcfg<pdspl_cfg+paimod->iLgcNum; plcfg++)
    {
        /* 遍历逻辑通道*/
        if (plcfg->ucHdCh==phwai->ucModCh &&
                (plcfg->ucFiltNum==0 || plcfg->ucFiltNum==2))
        {
            phwai->iSmplCh=plcfg-pdspl_cfg;			/* 对应的采样位置 */
            break;
        }
    }
    if (plcfg>=pdspl_cfg+paimod->iLgcNum)
    {
        LOG_Dbg_Msg("WARNING: unused hardware AI channel \"%s\".\n",
                    (int)phwai->aucId, 0, 0, 0, 0, 0);

        phwai->iSmplCh=-1;			/* 表示没有配置此通道 */
    }
}

/* set the result position in buffer for physical channel.
 * Para:
 *     phwai, physical channel.
 *     paimod, box data.
 *     pdspl_dc_cfg, DC processing structure.
 * Return:
 *     NONE.
 */
static void RD_Loc_Hw_Dc_Ai(RD_HW_AI_CH *phwai, RD_AI_MOD *paimod, DSP_LGC_DC_AI_CFG *pdspl_dc_cfg)
{
    DSP_LGC_DC_AI_CFG *pldccfg;

    for (pldccfg=pdspl_dc_cfg_g; pldccfg<pdspl_dc_cfg_g+paimod->iDcAiNum; pldccfg++)
    {
        /* search the configuration. */
        if ((pldccfg->ucHdCh == phwai->ucModCh) &&
                (pldccfg->ucHwAddr == phwai->p_part->aucHwAddr[0]))
        {
            phwai->iSmplCh=pldccfg-pdspl_dc_cfg_g;			/* the position. */

            break;
        }
    }

    if (pldccfg >= pdspl_dc_cfg_g+paimod->iDcAiNum)
    {
        LOG_Dbg_Msg("WARNING: unused hardware AI channel \"%s\".\n",
                    (int)phwai->aucId, 0, 0, 0, 0, 0);

        phwai->iSmplCh=-1;			/* 表示没有使用此通道 */
    }
}

/***********************************************************************
* RD_Add_Ai_Cfg - 配置与DSP计算结果连接
*
* RETURNS: 无
*
*
*/
static void RD_Add_Ai_Cfg(
    RD_AI_MOD *paimod,		/* 机箱 */
    RD_LGC_AI_CH *pch,				/* DSP计算通道 */
    DSP_LGC_AI_CFG *plcfgSave, 		/* 瞬间值计算配置 */
    DSP_CALC_AI_CFG *pccfgSave					/* 预处理计算配置 */
)
{
    int i;
    DSP_CALC_AI_CFG *pcalccfg;

    switch (pch->ucFiltTp)
    {
        case 0:
        case 8:
        case 0x0A:
            assert(IS_REAL_AI(pch->ucUnit));

            if (pch->ucFiltTp==0x0A)
            {
                for (i=0; i<paimod->iLgcNum; i++)
                {
                    /* 寻找瞬时值计算通道 */
                    if ((plcfgSave[i].ucHdCh==pch->phwai->ucModCh) && (plcfgSave[i].ucFiltNum != 6))
                    {
                        /* 不能为采样值算法 XS DY 3/9/2007 */
                        assert(plcfgSave[i].ucFiltNum!=1);
                        break;
                    }
                }
                if (i>=paimod->iLgcNum)
                {
                    /* 找不到则增加一个瞬时值计算通道 */
                    /* Config Freq. channel before LGC channel. */
                    plcfgSave[i].ucHdCh=pch->phwai->ucModCh;
                    plcfgSave[i].ucFiltNum=0xFF;
                    plcfgSave[i].fCoff=pch->phwai->fCoff;

                    paimod->iLgcNum++;
                    assert(paimod->iLgcNum<=DSP_LGC_AI_CH);
                }

                /* i is position to save new channel. */
                i=paimod->iLgcNum;
            }
            else
            {
                for (i=0; i<paimod->iLgcNum; i++)
                {
                    if ((plcfgSave[i].ucHdCh==pch->phwai->ucModCh) && ((plcfgSave[i].ucFiltNum != 6)))
                    {
                        /* 不能为采样值算法 XS DY 3/9/2007 */
                        assert(plcfgSave[i].ucFiltNum==0xFF);
                        break;
                    }
                }
            }

            plcfgSave[i].ucHdCh=pch->phwai->ucModCh;
            plcfgSave[i].fCoff=pch->phwai->fCoff;

            if (pch->ucFiltTp==0)
                plcfgSave[i].ucFiltNum=0;
            else if (pch->ucFiltTp==0x0A)
                plcfgSave[i].ucFiltNum=1;
            else
                plcfgSave[i].ucFiltNum=2;

            if (i>=paimod->iLgcNum)
            {
                paimod->iLgcNum++;
                assert(paimod->iLgcNum<=DSP_LGC_AI_CH);
            }
            break;

        case 0x18:
            i=	paimod->iLgcNum;		/* 新的计算配置 */
            plcfgSave[i].ucHdCh=pch->phwai->ucModCh;
            plcfgSave[i].fCoff=pch->phwai->fCoff;

            plcfgSave[i].ucFiltNum = 6;		/* 算法号为6 */
            paimod->iLgcNum++;
            assert(paimod->iLgcNum <= DSP_LGC_AI_CH);

            break;

        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 0x11:
        /* case 0x12: */
        case 0x13:
        /* case 0x14: */
        case 0x15:
            paimod->iTxCalcNum++; /* 可能是扩展机箱的预处理通道数量*/
            if (IS_RI_CPLX_AI(pch->ucUnit))
            {
                for (i=0; i<paimod->iLgcNum; i++)
                {
                    if (plcfgSave[i].ucHdCh==pch->phwai->ucModCh &&
                            (plcfgSave[i].ucFiltNum==0 ||
                             plcfgSave[i].ucFiltNum==2))
                        break;
                }
                if (i>=paimod->iLgcNum)
                {
                    /* Config CALC channel before LGC channel. */
                    plcfgSave[i].ucHdCh=pch->phwai->ucModCh;
                    plcfgSave[i].ucFiltNum=0xFF;
                    plcfgSave[i].fCoff=pch->phwai->fCoff;

                    paimod->iLgcNum++;
                    assert(paimod->iLgcNum<=DSP_LGC_AI_CH);
                }
                else
                {
                    for (pcalccfg=pccfgSave;
                            pcalccfg<pccfgSave+paimod->iCalcNum; pcalccfg++)
                    {
                        if (!(pcalccfg->ucArithParm & 0x30) &&
                                plcfgSave[pcalccfg->ucBgnLgcCh].ucHdCh==
                                pch->phwai->ucModCh && (pch->ucFiltTp & 0x07)==
                                (pcalccfg->ucArithParm & 0x07))
                        {
                            /* R/I channel configured before.
                             * Enable Tx it is enough. */
                            pcalccfg->ucArithParm |= 0x20;

                            return;         /* Skip code to increase iCalcNum. */
                        }
                    }
                }

                pcalccfg=pccfgSave+paimod->iCalcNum++;
                assert(paimod->iCalcNum<=DSP_CALC_AI_CH);

                pcalccfg->ucBgnLgcCh=i;
                pcalccfg->ucChNum=1;

                if (pch->ucFiltTp<0x10)
                    pcalccfg->ucArithNum=3;
                else
                    pcalccfg->ucArithNum=5;

                pcalccfg->ucArithParm=0x20 | (pch->ucFiltTp & 0x07);
            }
            else if (IS_MA_CPLX_AI(pch->ucUnit))
            {
                for (pcalccfg=pccfgSave;
                        pcalccfg<pccfgSave+paimod->iCalcNum; pcalccfg++)
                {
                    if (plcfgSave[pcalccfg->ucBgnLgcCh].ucHdCh==
                            pch->phwai->ucModCh &&
                            (pcalccfg->ucArithNum==3 || pcalccfg->ucArithNum==5) &&
                            (pch->ucFiltTp & 0x07)==(pcalccfg->ucArithParm & 0x07))
                        break;
                }
                if (pcalccfg>=pccfgSave+paimod->iCalcNum)
                {
                    /* Config M/A channel before R/I channel. */
                    for (i=0; i<paimod->iLgcNum; i++)
                    {
                        if (plcfgSave[i].ucHdCh==pch->phwai->ucModCh &&
                                (plcfgSave[i].ucFiltNum==0 ||
                                 plcfgSave[i].ucFiltNum==2))
                        {
                            assert(plcfgSave[i].fCoff==pch->phwai->fCoff);
                            break;
                        }
                    }
                    if (i>=paimod->iLgcNum)
                    {
                        /* Config CALC channel before LGC channel. */
                        plcfgSave[i].ucHdCh=pch->phwai->ucModCh;
                        plcfgSave[i].ucFiltNum=0xFF;
                        plcfgSave[i].fCoff=pch->phwai->fCoff;

                        paimod->iLgcNum++;
                        assert(paimod->iLgcNum<=DSP_LGC_AI_CH);
                    }

                    pcalccfg=pccfgSave+paimod->iCalcNum++;
                    assert(paimod->iCalcNum<=DSP_CALC_AI_CH);

                    pcalccfg->ucBgnLgcCh=i;
                    pcalccfg->ucChNum=1;

                    if (pch->ucFiltTp<0x10)
                        pcalccfg->ucArithNum=3;
                    else
                        pcalccfg->ucArithNum=5;

                    pcalccfg->ucArithParm=pch->ucFiltTp & 0x07;
                }

                /* pcalccfg points to R/I channel. */
                i=pcalccfg->ucBgnLgcCh;

                /* Add M/A channel to the end of pccfgSave. */
                pcalccfg=pccfgSave+paimod->iCalcNum++;
                assert(paimod->iCalcNum<=DSP_CALC_AI_CH);

                pcalccfg->ucBgnLgcCh=i;
                pcalccfg->ucChNum=1;

                pcalccfg->ucArithNum=4;
                pcalccfg->ucArithParm=0x20 | (pch->ucFiltTp & 0x07);
            }
            else
                assert(FALSE);

            break;

        default:
            assert(FALSE);
            break;
    }
}

/* add dc Ai DSP channel.
 * Para:
 *     paimod, module.
 *     pch, configuration channel.
 *     plcfgSave, DSP channel.
 * Return:
 *     NONE.
 */
static void RD_Add_Dc_Ai_Cfg(RD_AI_MOD *paimod, RD_LGC_AI_CH *pch, DSP_LGC_DC_AI_CFG *plcfgSave)
{
    int i;

    i = paimod->iDcAiNum;  /* 新的计算配置 */
    plcfgSave[i].ucHwAddr = pch->phwai->p_part->aucHwAddr[0];
    plcfgSave[i].ucHdCh=pch->phwai->ucModCh;
    plcfgSave[i].fCoff=pch->phwai->fCoff;
    plcfgSave[i].pCfg=pch;
    paimod->iDcAiNum++;
}

/***********************************************************************
* RDChgAiCfg - 更改DSP配置内容
*
* RETURNS: 无
*
*
*/
static void RDChgAiCfg(
    RD_AI_MOD *paimod,		/* 机箱 */
    RD_HW_AI_CH *pch,				/* 物理通道 */
    DSP_LGC_AI_CFG *plcfgSave 				/* 瞬间值计算配置 */
)
{
    int i;

    for (i=0; i<paimod->iLgcNum; i++)
    {
        /* 在该模件所有的瞬时值通道中寻找，包括频率计算 */
        if (((plcfgSave[i].ucHdCh-1) == pch->ucModCh))
        {
            /* 通道号已经加一，所有的预处理通道 */
            plcfgSave[i].fCoff=pch->fCoff;
        }
    }
}

/***********************************************************************
* RD_Loc_Lgc_Ai - 瞬时值遍历
*
* RETURNS: 无
*
*
*/
static void RD_Loc_Lgc_Ai(
    RD_LGC_AI_CH *paich,			/* AI逻辑通道 */
    RD_AI_MOD *paimod, 		/* 机箱 */
    DSP_LGC_AI_CFG *pdspl_cfg		/* DSP瞬时值处理配置 */
)
{
    DSP_LGC_AI_CFG *plcfg;


    for (plcfg=pdspl_cfg;
            plcfg<pdspl_cfg+paimod->iLgcNum; plcfg++)
    {
        /* 遍历逻辑通道 */
        if (paich->phwai->ucModCh==plcfg->ucHdCh && 				/* 物理通道相同 */
                ((paich->ucFiltTp==0 && plcfg->ucFiltNum==0) ||
                 (paich->ucFiltTp==8 && plcfg->ucFiltNum==2) ||
                 (paich->ucFiltTp==0x0A && plcfg->ucFiltNum==1) ||
                 (paich->ucFiltTp ==0x18 && plcfg->ucFiltNum == 6))) 				/* 算法相同 */
        {
            /* 增加定点算法 */
            assert(paich->phwai->fCoff==plcfg->fCoff); 				/* 系数相同 */

            paich->pdat.pfLgcAI=
                paimod->pfDbBgn+(plcfg-pdspl_cfg); 							/* 逻辑通道找到对应的结果存储区 */
            break;
        }
    }
    assert(plcfg<pdspl_cfg+paimod->iLgcNum);
}

/***********************************************************************
* RD_Loc_Lgc_Ai - 预处理值遍历
*
* RETURNS: 无
*
*
*/
static void RD_Loc_Calc_Ai(
    RD_LGC_AI_CH *paich, 				/* AI逻辑通道 */
    RD_AI_MOD *paimod,		/* 机箱 */
    DSP_CALC_AI_CFG *pdspc_cfg, 				/* 预处理值配置 */
    DSP_LGC_AI_CFG *pdspl_cfg		/* 瞬时值配置 */
)
{
    DSP_CALC_AI_CFG *pccfg;
    int iTxCalc;                            /* Count transmited CalcAI channel. */

    /* 遍历预处理结构 */
    iTxCalc=0;
    for (pccfg=pdspc_cfg;
            pccfg<pdspc_cfg+paimod->iCalcNum; pccfg++)
    {
        /* 物理通道对应 */
        if (paich->phwai->ucModCh==
                pdspl_cfg[pccfg->ucBgnLgcCh].ucHdCh  &&
                (paich->ucFiltTp & 0x07)==(pccfg->ucArithParm & 0x07) &&
                ((IS_RI_CPLX_AI(paich->ucUnit) &&
                  ((pccfg->ucArithNum==3 && paich->ucFiltTp<0x10) ||
                   (pccfg->ucArithNum==5 && paich->ucFiltTp>0x10))) ||
                 (IS_MA_CPLX_AI(paich->ucUnit) &&
                  pccfg->ucArithNum==4)))
        {
            assert(paich->phwai->fCoff==
                   pdspl_cfg[pccfg->ucBgnLgcCh].fCoff);

            assert((pccfg->ucArithParm & 0x30)==0x20);

            /* Transmited CalcAI channel number to determine
             * position in data buffer. */
            paich->pdat.pxCalcAI=paimod->pxDbBgn+iTxCalc;
            break;
        }
        else if (pccfg->ucArithParm & 0x30)
        {
            assert((pccfg->ucArithParm & 0x30)==0x20);
            /* Transmited CalcAI channel number increased. */
            iTxCalc++;
        }
    }
    assert(pccfg<pdspc_cfg+paimod->iCalcNum);
}

/***********************************************************************
* RD_Loc_Msuc_Ai - 测量结果与配置缓冲区对应
*
* RETURNS: 无
*
*
*/
static void RD_Loc_Msuc_Ai(
    RD_MSU_AI_CH *paichmsu,		/* DSP计算测量配置*/
    RD_AI_MOD *paimod,		/* 机箱 */
    DSP_MSU_AI_CFG *pdspm_cfg, 				/* 测量配置 */
    DSP_LGC_AI_CFG *pdspl_cfg		/* 瞬时值配置 */
)
{
    DSP_MSU_AI_CFG *pmsucfg;
    int iTxMsuc;

    iTxMsuc = 0;
    for(pmsucfg = pdspm_cfg;
            pmsucfg<pdspm_cfg+paimod->iMsucNum; pmsucfg++)
    {
        if (paichmsu->phwai->ucModCh==
                pdspl_cfg[pmsucfg->ucBgnLgcCh].ucHdCh&&(paichmsu->ucFiltTp & 0x07)==(pmsucfg->ucArithParm & 0x07) )
        {
            assert(paichmsu->phwai->fCoff==
                   pdspl_cfg[pmsucfg->ucBgnLgcCh].fCoff);
#ifndef NO_DEBUG
            LOG_Dbg_Msg("Phsical Channel 2: %d %d\n",paichmsu->phwai->ucModCh,iTxMsuc,0,0,0,0);
#endif
            paichmsu->pxMsucAI=paimod->pxmsuDbBgn+iTxMsuc;
            break;
        }
        else
        {
            iTxMsuc++;
        }
    }
    assert(pmsucfg<pdspm_cfg+paimod->iMsucNum);
}

/* match the DC result buffer with the configuration.
 * Para:
 *     paichdc, configuration channel.
 *     paimod, box information.
 *     pdspl_dc_cfg, proceesing channel.
 * Return:
 *     NONE.
 */
static void RD_Loc_Dc_Ai(RD_LGC_AI_CH *paichdc, RD_AI_MOD *paimod, DSP_LGC_DC_AI_CFG *pdspl_dc_cfg)
{
    DSP_LGC_DC_AI_CFG *plcfg;

    for (plcfg=pdspl_dc_cfg; plcfg<pdspl_dc_cfg+paimod->iDcAiNum; plcfg++)
    {
        /* search all the logic channel. */
        if ((paichdc->phwai->ucModCh == plcfg->ucHdCh)
                && (paichdc->phwai->p_part->aucHwAddr[0] == plcfg->ucHwAddr))
        {
            paichdc->pdat.pfLgcAI=paimod->pfDcDbBgn+(plcfg-pdspl_dc_cfg);
            plcfg->pData=paichdc->pdat.pfLgcAI;

            break;
        }
    }

    assert(plcfg<pdspl_dc_cfg+paimod->iDcAiNum);
}

uint8_t RD_Sort_Dsp_CfgnTmp;

/***********************************************************************
* RD_Sort_Dsp_Cfg - DSP配置排序
*
* RETURNS: 无
*
*
*/
static void RD_Sort_Dsp_Cfg(
    RD_AI_MOD *paimod, 		/* 机箱 */
    DSP_LGC_AI_CFG *pdspl_cfg,					/* DSP瞬时值配置 */
    DSP_CALC_AI_CFG *pdspc_cfg		/* DSP预处理配置 */
)
{
    DSP_CALC_AI_CFG *pccfg;
    DSP_LGC_AI_CFG lcfgTmp;
    int i;
    int j;
    uint32_t ul;
    uint32_t aulArithBit[DSP_LGC_AI_CH];

    assert(paimod->iLgcNum && paimod->iLgcNum <= DSP_LGC_AI_CH);
    assert(pdspl_cfg && pdspc_cfg);

    /* Check if some channel refrenced by CALC but not configed. */
    for (i=0; i<paimod->iLgcNum; i++)
    {
        if (pdspl_cfg[i].ucFiltNum==0xFF)
        {
            /* Change to default arith(without pre-deal). */
            pdspl_cfg[i].ucFiltNum=0;
        }
    }

    memset(aulArithBit, 0, sizeof(aulArithBit));

    /* 排序 */
    for (pccfg=pdspc_cfg; pccfg<pdspc_cfg+paimod->iCalcNum; pccfg++)
    {
        /* 条件判断 */
        assert(pccfg->ucChNum == 1);
        assert(pdspl_cfg[pccfg->ucBgnLgcCh].ucFiltNum==0 ||
               pdspl_cfg[pccfg->ucBgnLgcCh].ucFiltNum==2);

        if (pccfg->ucArithNum == 3)
        {
            assert(pccfg->ucArithParm & 0x07);

            /* Can't use both arith 3 and arith 5. */
            assert(!(aulArithBit[pccfg->ucBgnLgcCh] &
                     BV32(33-3*(pccfg->ucArithParm & 0x07))));

            RD_Sort_Dsp_CfgnTmp = (pccfg->ucArithParm)&0x07;
            aulArithBit[pccfg->ucBgnLgcCh] |=
                BV32(34-3*(pccfg->ucArithParm & 0x07));
        }
        else if (pccfg->ucArithNum == 5)
        {
            assert(pccfg->ucArithParm & 0x07);

            /* Can't use both arith 3 and arith 5. */
            assert(!(aulArithBit[pccfg->ucBgnLgcCh] &
                     BV32(34-3*(pccfg->ucArithParm & 0x07))));

            aulArithBit[pccfg->ucBgnLgcCh] |=
                BV32(33-3*(pccfg->ucArithParm & 0x07));
        }
        else if (pccfg->ucArithNum == 4)
        {
            assert((pccfg->ucArithParm & 0x20) && (pccfg->ucArithParm & 0x07));

            /* 谐波次数越大，越在后面 */
            aulArithBit[pccfg->ucBgnLgcCh] |=
                BV32(32-3*(pccfg->ucArithParm & 0x07));
        }
        else
            assert(FALSE);
    }

    for (i=0; i<paimod->iLgcNum-1; i++)
    {
        for (j=i+1; j<paimod->iLgcNum; j++)
        {
            /* 频率计算逻辑通道放在后面 */
            if ((pdspl_cfg[i].ucFiltNum == 1 && pdspl_cfg[j].ucFiltNum != 1)
                    || (pdspl_cfg[i].ucFiltNum !=1 && pdspl_cfg[j].ucFiltNum !=1 &&
                        aulArithBit[j]<aulArithBit[i]))		/* 半波计算放在前面 */
            {
                lcfgTmp=pdspl_cfg[i];
                pdspl_cfg[i]=pdspl_cfg[j];
                pdspl_cfg[j]=lcfgTmp;

                if (pdspl_cfg[i].ucFiltNum !=1 || pdspl_cfg[j].ucFiltNum != 1)
                {
                    /* 如果全是瞬时值计算，预处理起始值进行处理 */
                    ul=aulArithBit[i];
                    aulArithBit[i]=aulArithBit[j];
                    aulArithBit[j]=ul;

                    for (pccfg=pdspc_cfg;
                            pccfg<pdspc_cfg+paimod->iCalcNum; pccfg++)
                    {
                        if (pccfg->ucBgnLgcCh == i)
                            pccfg->ucBgnLgcCh=j;
                        else if (pccfg->ucBgnLgcCh == j)
                            pccfg->ucBgnLgcCh=i;
                    }
                }
            }
        }
    }

    qsort(pdspc_cfg, paimod->iCalcNum,
          sizeof(*pdspc_cfg), RD_Cmp_Calc_Cfg);
}

/***********************************************************************
* RD_Cmp_Calc_Cfg - 排序函数
*
* RETURNS: 无
*
*
*/
static int RD_Cmp_Calc_Cfg(
    const void *pvSrc,		/* 源 */
    const void *pvDst			/* 目标 */
)
{
    DSP_CALC_AI_CFG *pcfgSrc;
    DSP_CALC_AI_CFG *pcfgDst;

    pcfgSrc=(DSP_CALC_AI_CFG*)pvSrc;
    pcfgDst=(DSP_CALC_AI_CFG*)pvDst;

    if (pcfgSrc->ucArithNum>pcfgDst->ucArithNum)
        return 1;
    else if (pcfgSrc->ucArithNum<pcfgDst->ucArithNum)
        return -1;
    else
    {
        /* Same ucArithNum, compare ucArithParm. */
        if ((pcfgSrc->ucArithParm & 0x07)>
                (pcfgDst->ucArithParm & 0x07))
            return 1;
        else if ((pcfgSrc->ucArithParm & 0x07)<
                 (pcfgDst->ucArithParm & 0x07))
            return -1;
        else
        {
            /* Same ucArithNum && ucArithParm, sort with ucBgnLgcCh. */
            if (pcfgSrc->ucBgnLgcCh>pcfgDst->ucBgnLgcCh)
                return 1;
            else if (pcfgSrc->ucBgnLgcCh<pcfgDst->ucBgnLgcCh)
                return -1;
            else
                return 0;
        }
    }
}

/* 启动DSP计算
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS RD_Boot_Dsp(void)
{
    DSP_LGC_AI_CFG *plcfg;
    int iCalcCfg;
    int iTmpCalcCfg;
    int iMsucCfg;
    int iTxPts;
    EP_STATUS stsRet;
    int iOptChType = 0;
    int iOptSendPts = OPT_64K_SEND_POINT;


    iTxPts = uiAiRate_g/6000; /* 采样速率最高不超过10KHz */
    if (!iTxPts)
        iTxPts = 1;

    /* 初始化机箱前更新系数 */
    RDUpdateDspCfg();

    if (aimodExt_g.iCalcNum)
        iCalcCfg = RD_Compack_Calc_Cfg(pdspc_cfg_ext_g, aimodExt_g.iCalcNum);
    else
        iCalcCfg = 0;

    /* 配置决定 */
    if (aimodExt_g.iLgcNum)
    {
        /* Translate hardware channel to suit DSP. */
        for (plcfg=pdspl_cfg_ext_g; plcfg<pdspl_cfg_ext_g+aimodExt_g.iLgcNum; )
            plcfg++->ucHdCh++;
    }

    if (aimodExt_g.iLgcNum || iExtDiNum_g)
    {
        bDspDrvMod = TRUE; /* 配置扩展机箱时特殊处理 */

        stsRet = Init_Ext_Box(
                     uiAiPts_g,
                     uiPwrFreq_g,
                     iTxPts,
                     &aimodExt_g,
                     aimodExt_g.iLgcNum,
                     pdspl_cfg_ext_g,
                     iCalcCfg,
                     pdspc_cfg_ext_g
                 );

        if (stsRet != EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                           "扩展机箱异常(%02d)\n", EXT_SYN_SAMPLE_ERR, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                           "Extended box error(%02d)\n",
                           EXT_SYN_SAMPLE_ERR, 0);
            }
            LOG_Write(LOG_KERNEL, "扩展机箱初始化失败!!\n", NULL);

        }
    }

    AddRunTimeTag("Init_Ext_Box");  /* 时间标签 */

    /* for excite */
    if (aimodRedun_g.iCalcNum)
        iCalcCfg = RD_Compack_Calc_Cfg(pdspc_cfg_redun_g, aimodRedun_g.iCalcNum);
    else
        iCalcCfg = 0;

    if (aimodRedun_g.iLgcNum)
    {
        /* Translate hardware channel to suit DSP. */
        for (plcfg=pdspl_cfg_redun_g; plcfg<pdspl_cfg_redun_g+aimodRedun_g.iLgcNum; )
            plcfg++->ucHdCh++;
    }

    if (aimodDsp_g.iCalcNum)
        iCalcCfg = RD_Compack_Calc_Cfg(pdspc_cfg_g, aimodDsp_g.iCalcNum);
    else
        iCalcCfg = 0;

    iTmpCalcCfg = iCalcCfg;

    if (aimodDsp_g.iMsucNum)
        iMsucCfg = RD_Compress_Msuc_Cfg(pdspm_cfg_g, aimodDsp_g.iMsucNum);
    else
        iMsucCfg = 0;
#ifndef EDP01_CA_OPT_BUILD		/* EDP01平台C-A版本，使用光CT，屏蔽本机采样 */
  if(uiAppType_g != APP_STAB_CONTROL)	
    assert(aimodDsp_g.iLgcNum);
#endif

    /* Translate hardware channel to suit DSP. */
    for (plcfg=pdspl_cfg_g; plcfg<pdspl_cfg_g+aimodDsp_g.iLgcNum; plcfg++)
        plcfg->ucHdCh++;

    GetUnitType();		/* 获取单位类型 */

#ifndef EDP01_CA_OPT_BUILD 	/* EDP01平台C-A版本，使用光CT，屏蔽本机采样 */

// EP_STATUS DSP_Initialize(
//     u_int uiSmplRate, 	/* 每周波采样点数 */
//     uint16_t uiProRate, 		/* DSP计算使用点数 */
//     u_int Sysfrequency, 			/* 系统频率 */
//     u_int uiTxPts, 		/* 每次传送采样点数 */
//     void *pvAiMod,				/* 该模块（DSP负责的所有AI采集/计算通道）的句柄 */
//     u_int uiLgcCh, 		/* 采样的逻辑通道数 */
//     DSP_LGC_AI_CFG *plgccfg,			/* 指向逻辑通道配置数组第0个元素的指针，数组元素有uiLgcCh个 */
//     u_int uiCalcCh, 		/* 预处理通道配置数 */
//     DSP_CALC_AI_CFG *pcalccfg,		/* 指向预处理通道配置数组第0个元素的指针，数组元素uiCalcCfg个 */
//     u_int uiMsucCh, 		/* 测量通道配置数 */
//     DSP_MSU_AI_CFG *pmsuccfg,		/* 指向测量通道配置数组第0个元素的指针 */
//     u_int Msu_Base_Num,		/* 测量基准通道 */
//     u_int uiLgcDcCh,    /* number of DC signal channel. */
//     DSP_LGC_DC_AI_CFG *plgdccfg				/* DC signal channel configuration. */
// )

    stsRet = DSP_Initialize((u_int)uiAiPts_g,
                            (uint16_t)uiAiPts_g,
                            (u_int)Sysfrequency_g,
                            (u_int)uiTxPts,
                            (void *)&aimodDsp_g,
                            (u_int)aimodDsp_g.iLgcNum,
                            (DSP_LGC_AI_CFG *)pdspl_cfg_g,
                            (u_int)iCalcCfg,
                            (DSP_CALC_AI_CFG *)pdspc_cfg_g,
                            (u_int)iMsucCfg,
                            (DSP_MSU_AI_CFG *)pdspm_cfg_g,
                            (u_int)AIBaseCh_g,
                            (u_int)aimodDsp_g.iDcAiNum,
                            (DSP_LGC_DC_AI_CFG *)pdspl_dc_cfg_g
                           );


#if FREE_INIT_MEM
    if (pdspl_cfg_g)
        EP_free(pdspl_cfg_g);
    if (pdspc_cfg_g)
        EP_free(pdspc_cfg_g);

    if (pdspl_cfg_ext_g)
        EP_free(pdspl_cfg_ext_g);
    if (pdspc_cfg_ext_g)
        EP_free(pdspc_cfg_ext_g);
#endif

    if (stsRet != EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                       "错误码:%02d\n", AD_DSP_INIT_ERR, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                       "Error code:%02d\n", AD_DSP_INIT_ERR, 0);
        }
        LOG_Write(LOG_KERNEL, "数据处理模块初始化失败!!\n", NULL);
    }
#endif

    /* 光纵机箱1，要求光纵机箱初始化放到本机初始化后面 */

    if (aimodOpt_g[0].iCalcNum)
        iCalcCfg = RD_Compack_Calc_Cfg(pdspc_cfg_Opt_g[0], aimodOpt_g[0].iCalcNum);
    else
        iCalcCfg = 0;

    if (aimodOpt_g[0].iLgcNum)
    {
        /* Translate hardware channel to suit DSP. */
        for (plcfg=pdspl_cfg_Opt_g[0]; plcfg<pdspl_cfg_Opt_g[0]+aimodOpt_g[0].iLgcNum; )
            plcfg++->ucHdCh++;
    }

    if (aimodOpt_g[0].iLgcNum)
    {
        /* 采样频率大于2.4kHz时，光纵初始化导致装置重启 */

        if (uiAiPts_g != 24)
        {
            /* 当使用光纵时，周波采样点数固定为24点 */
            if (ENG_MODE == 0)
            {
                LOG_Dbg_Msg("当使用光纤纵差时，周波采样点数必须为24点.\n", 0, 0, 0, 0, 0, 0);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Dbg_Msg("When using the Opt communication, the sampling point number in a cycle must be 24.\n",
                            0, 0, 0, 0, 0, 0);
            }

            assert (FALSE);
        }

        if (rdinfo_g.b64KOptCh1)
        {
            iOptChType = 0;
            iOptSendPts = OPT_64K_SEND_POINT;
            // set_hdlc_a_1m(); /* 设置为1M模式 */
        }
        else if (rdinfo_g.b2MOptCh1)
        {
            iOptChType = 1;
            iOptSendPts = OPT_2M_SEND_POINT;
        }
        else
        {
            assert (FALSE);
        }


        stsRet = Init_Opt_Box(
                     0,
                     iOptChType,
                     uiAiPts_g,
                     uiPwrFreq_g,
                     iOptSendPts,
                     &aimodOpt_g[0],
                     aimodOpt_g[0].iLgcNum,
                     pdspl_cfg_Opt_g[0],
                     iCalcCfg,
                     pdspc_cfg_Opt_g[0],
                     BoxAoCfgOpt_g[0].iOptAONum,
                     BoxAoCfgOpt_g[0].aOptBoxAoCfg_g,
                     MAX_OPT_ALLOW_DIO_NUM,
                     aOptBoxIoInfo_g[0].aucOptDiSts_g,
                     MAX_OPT_ALLOW_DIO_NUM,
                     aOptBoxIoInfo_g[0].aucOptDoSts_g
                 );


        if (stsRet != EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                           "光纵通道1异常(%02d)\n", OPT_BOX_INIT_ERR, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                           "OPT channel 1 Error(%02d)\n", OPT_BOX_INIT_ERR, 0);
            }
            LOG_Write(LOG_KERNEL, "光纵通道1初始化失败!!\n", NULL);
        }

        abOptChIsInitOver_g[0] = TRUE;
    }


    /* 光纵机箱2 */

    if (aimodOpt_g[1].iCalcNum)
        iCalcCfg = RD_Compack_Calc_Cfg(pdspc_cfg_Opt_g[1], aimodOpt_g[1].iCalcNum);
    else
        iCalcCfg = 0;

    if (aimodOpt_g[1].iLgcNum)
    {
        /* Translate hardware channel to suit DSP. */
        for (plcfg=pdspl_cfg_Opt_g[1]; plcfg<pdspl_cfg_Opt_g[1]+aimodOpt_g[1].iLgcNum; )
            plcfg++->ucHdCh++;
    }

    if (aimodOpt_g[1].iLgcNum)
    {

        if (uiAiPts_g != 24)
        {
            /* 当使用光纵时，周波采样点数固定为24点 */
            if (ENG_MODE == 0)
            {
                LOG_Dbg_Msg("当使用光纤纵差时，周波采样点数必须为24点.\n",
                            0, 0, 0, 0, 0, 0);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Dbg_Msg("When using the Opt communication, the sampling point number in a cycle must be 24.\n",
                            0, 0, 0, 0, 0, 0);
            }

            assert (FALSE);
        }

        if (rdinfo_g.b64KOptCh2)
        {
            iOptChType = 0;
            iOptSendPts = OPT_64K_SEND_POINT;
            // set_hdlc_b_1m();
        }
        else if (rdinfo_g.b2MOptCh2)
        {
            iOptChType = 1;
            iOptSendPts = OPT_2M_SEND_POINT;
        }
        else
        {
            assert (FALSE);
        }

        stsRet = Init_Opt_Box(
                     1,
                     iOptChType,
                     uiAiPts_g,
                     uiPwrFreq_g,
                     iOptSendPts,
                     &aimodOpt_g[1],
                     aimodOpt_g[1].iLgcNum,
                     pdspl_cfg_Opt_g[1],
                     iCalcCfg,
                     pdspc_cfg_Opt_g[1],
                     BoxAoCfgOpt_g[1].iOptAONum,
                     BoxAoCfgOpt_g[1].aOptBoxAoCfg_g,
                     MAX_OPT_ALLOW_DIO_NUM,
                     aOptBoxIoInfo_g[1].aucOptDiSts_g,
                     MAX_OPT_ALLOW_DIO_NUM,
                     aOptBoxIoInfo_g[1].aucOptDoSts_g
                 );

        if (stsRet != EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                           "光纵通道2异常(%02d)\n", OPT_BOX_INIT_ERR, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                           "OPT channel 2 Error(%02d)\n", OPT_BOX_INIT_ERR, 0);
            }
            LOG_Write(LOG_KERNEL, "光纵通道2初始化失败!!\n", NULL);
        }
    }

#if defined(EDP03_INTELBOX_BUILD) || defined(EDP_01_02_BUILD)

    /* 同杆并架机箱 */

    if (aimodPole_g.iCalcNum)
        iCalcCfg = RD_Compack_Calc_Cfg(pdspc_cfg_Pole_g, aimodPole_g.iCalcNum);
    else
        iCalcCfg = 0;

    if (aimodPole_g.iLgcNum)
    {
        /* Translate hardware channel to suit DSP. */
        for (plcfg=pdspl_cfg_Pole_g; plcfg<pdspl_cfg_Pole_g+aimodPole_g.iLgcNum; )
            plcfg++->ucHdCh++;
    }

    if (aimodHdl_g.iLgcNum)
    {
        /* Translate hardware channel to suit DSP. */
        for (plcfg=pdspl_cfg_Hdl_g; plcfg<pdspl_cfg_Hdl_g+aimodHdl_g.iLgcNum; )
            plcfg++->ucHdCh++;
    }

    if (aimodPole_g.iLgcNum ||iPoleDiNum_g || iPoleDoNum_g)
    {

        stsRet = Init_Pole_Box(
                     uiAiPts_g,
                     uiPwrFreq_g,
                     &aimodPole_g,
                     aimodPole_g.iLgcNum,
                     pdspl_cfg_Pole_g,
                     iCalcCfg,
                     pdspc_cfg_Pole_g
                 );

        if (stsRet != EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                           "同杆装置异常(%02d)\n", AD_PTL_INIT_ERR, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                           "Parallel device error(%02d)\n", AD_PTL_INIT_ERR, 0);
            }
            LOG_Write(LOG_KERNEL, "同杆装置初始化失败!!\n", NULL);
        }
    }

    /* 智能操作箱 */

    if ( aimodHdl_g.iLgcNum || iHdlDiNum_g || iHdlDoNum_g)
    {

        stsRet=Init_Hdl_Box(uiAiPts_g, uiPwrFreq_g, &aimodHdl_g,
                            aimodHdl_g.iLgcNum, pdspl_cfg_Hdl_g, iCalcCfg, pdspc_cfg_Hdl_g);

        if (stsRet != EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT|ER_NOLOGWRITE,
                           "智能操作箱异常(%02d)\n", AD_IOB_INIT_ERR, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_LOCK | ER_ALARM | ER_REPORT | ER_NOLOGWRITE,
                           "IOB(Intelligent Operating Box) error(%02d)\n", AD_IOB_INIT_ERR, 0);
            }
            LOG_Write(LOG_KERNEL,"智能操作箱初始化失败!!\n", NULL);
        }
    }
#endif

    return stsRet;
}

/***********************************************************************
* CmpExtAcCoff - 比较扩展机箱交流采样系数
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS CmpExtAcCoff(
    uint8_t *puc
)
{
    int i;
    int j;
    DSP_LGC_AI_CFG *plcfg;

    plcfg=pdspl_cfg_ext_g;

    for (i=0; i<aimodExt_g.iLgcNum; i++, plcfg++)
    {
        if(*puc++ != plcfg->ucHdCh-1)
        {
            return EP_ERROR;
        }

        for(j=0; j<4; j++)
        {
            if(*puc++ != *((uint8_t*)&plcfg->fCoff+j))
            {
                return EP_ERROR;
            }
        }

        puc++;
    }

    return EP_SUCCESS;
}

/***********************************************************************
* RD_Compack_Calc_Cfg - 配置压缩
*
* RETURNS: DSP配置
*
*/
static int RD_Compack_Calc_Cfg(
    DSP_CALC_AI_CFG *pdspc_cfg,		/* DSP预处理配置 */
    int iCalcCfg		/* 个数 */
)
{
    DSP_CALC_AI_CFG *pccfg;

    for (pccfg=pdspc_cfg; pccfg<pdspc_cfg+iCalcCfg-1; )
    {
        if (pccfg[1].ucBgnLgcCh==pccfg->ucBgnLgcCh+pccfg->ucChNum &&
                pccfg[1].ucArithNum==pccfg->ucArithNum &&
                pccfg[1].ucArithParm==pccfg->ucArithParm)
        {
            pccfg->ucChNum+=pccfg[1].ucChNum;
            if (iCalcCfg-(pccfg-pdspc_cfg)>2)
            {
                memmove(pccfg+1, pccfg+2,
                        sizeof(*pccfg)*(iCalcCfg-(pccfg-pdspc_cfg)-2));
            }
            iCalcCfg--;
        }
        else
            pccfg++;
    }

    return iCalcCfg;
}

/***********************************************************************
* RD_Sort_Dsp_Msu_Cfg - 配置排序
*
* RETURNS: DSP配置
*
*/
static void RD_Sort_Dsp_Msu_Cfg(
    RD_AI_MOD *paimod, 		/* 模拟量计算结果读取有关模式结构 */
    DSP_LGC_AI_CFG *pdspl_cfg, 		/* 瞬时值处理结构 */
    DSP_MSU_AI_CFG *pdspm_cfg				/* 测量计算配置结构 */
)
{
    DSP_MSU_AI_CFG *pmcfg;
    int i;
    uint32_t aulArithBit[DSP_LGC_AI_CH];

    assert(paimod->iLgcNum && paimod->iLgcNum <= DSP_LGC_AI_CH);
    assert(paimod->iMsucNum&& paimod->iMsucNum <= DSP_MSU_AI_CH);
    /* 结构都不为空 */
    assert(pdspl_cfg && pdspm_cfg);

    /* Check if some channels referenced by MSU but not configured. */
    for (i=0; i<paimod->iLgcNum; i++)
    {
        if (pdspl_cfg[i].ucFiltNum == 0xFF)
        {
            /* Change to default arith(without pre-deal). */
            pdspl_cfg[i].ucFiltNum=0;
        }
    }

    /* 清零 */
    memset(aulArithBit, 0, sizeof(aulArithBit));

    for (pmcfg=pdspm_cfg; pmcfg<pdspm_cfg+paimod->iMsucNum; pmcfg++)
    {
        assert(pmcfg->ucChNum == 1);
        assert(pdspl_cfg[pmcfg->ucBgnLgcCh].ucFiltNum == 0 ||pdspl_cfg[pmcfg->ucBgnLgcCh].ucFiltNum == 2);

        if (pmcfg->ucArithNum == 7)
        {
            assert((pmcfg->ucArithParm & 0x20) && (pmcfg->ucArithParm & 0x07));
            aulArithBit[pmcfg->ucBgnLgcCh] |=BV32(31-3*(pmcfg->ucArithParm & 0x07));
        }
        else
            assert(FALSE);
    }

    qsort(pdspm_cfg, paimod->iMsucNum, sizeof(*pdspm_cfg), RD_Cmp_Msu_Cfg);
}

/***********************************************************************
* RD_Cmp_Msu_Cfg - 测量配置排序
*
* RETURNS: 比较结果
*
*/
static int RD_Cmp_Msu_Cfg(
    const void *pvSrc,			/* 源 */
    const void *pvDst				/* 目标 */
)
{
    DSP_MSU_AI_CFG *pcfgSrc;
    DSP_MSU_AI_CFG *pcfgDst;

    pcfgSrc=(DSP_MSU_AI_CFG*)pvSrc;
    pcfgDst=(DSP_MSU_AI_CFG*)pvDst;

    if (pcfgSrc->ucArithNum>pcfgDst->ucArithNum)					/* 按算法次序排序*/
        return 1;
    else if (pcfgSrc->ucArithNum<pcfgDst->ucArithNum)
        return -1;
    else
    {
        /* Same ucArithNum, compare ucArithParm. */
        if ((pcfgSrc->ucArithParm & 0x07)>(pcfgDst->ucArithParm & 0x07))				/* 按系数排序 */
            return 1;
        else if ((pcfgSrc->ucArithParm & 0x07)<(pcfgDst->ucArithParm & 0x07))
            return -1;
        else
        {
            /* Same ucArithNum && ucArithParm, sort with ucBgnLgcCh. */
            if (pcfgSrc->ucBgnLgcCh>pcfgDst->ucBgnLgcCh)							/* 按起始通道排序 */
                return 1;
            else if (pcfgSrc->ucBgnLgcCh<pcfgDst->ucBgnLgcCh)
                return -1;
            else
                return 0;
        }
    }
}

/***********************************************************************
* RD_Compress_Msuc_Cfg - 测量通道配置合并
*
* RETURNS: 压缩后项数
*
*/
static int RD_Compress_Msuc_Cfg(
    DSP_MSU_AI_CFG *pdspm_cfg, 		/* 测量通道配置指针 */
    int iMsucCfg			/* 原始项数 */
)
{
    DSP_MSU_AI_CFG *pmcfg;

    for (pmcfg=pdspm_cfg; pmcfg<pdspm_cfg+iMsucCfg-1; )
    {
        if (pmcfg[1].ucBgnLgcCh==pmcfg->ucBgnLgcCh+pmcfg->ucChNum &&
                pmcfg[1].ucArithNum==pmcfg->ucArithNum &&
                pmcfg[1].ucArithParm==pmcfg->ucArithParm)
        {
            pmcfg->ucChNum+=pmcfg[1].ucChNum;
            if (iMsucCfg-(pmcfg-pdspm_cfg)>2)
            {
                memmove(pmcfg+1, pmcfg+2,
                        sizeof(*pmcfg)*(iMsucCfg-(pmcfg-pdspm_cfg)-2));
            }
            iMsucCfg--;
        }
        else
            pmcfg++;
    }
    return iMsucCfg;
}

/***********************************************************************
* RD_Add_Ai_Msu_Cfg - 解析配置文件
*
* RETURNS: 无
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 内存错误
*/
static void RD_Add_Ai_Msu_Cfg(
    RD_AI_MOD *paimod, 		/* 模拟量计算结果读取有关模式结构 */
    RD_MSU_AI_CH *pch, 				/* 测量有关文件配置结构 */
    DSP_LGC_AI_CFG *plcfgSave,			/* 瞬时值处理结构 */
    DSP_MSU_AI_CFG *pmcfgSave		/* 测量计算配置结构 */
)
{
    static uint8_t CountFlag=0;
    static uint8_t Count=0;
    int i;
    DSP_MSU_AI_CFG *pcalmcfg; /* 测量计算配置结构 */

    if (IS_MA_CPLX_AI(pch->ucUnit)) /* 幅度相角计算 */
    {
        /* Config M/A channel before R/I channel. */
        for (i=0; i<paimod->iLgcNum; i++)
        {
            /* 物理通道号与算法检测 */
            if (plcfgSave[i].ucHdCh == pch->phwai->ucModCh && (plcfgSave[i].ucFiltNum == 0 ||
                    plcfgSave[i].ucFiltNum == 2))		/* 寻找相应的瞬时值算法 */
            {
                assert(plcfgSave[i].fCoff == pch->phwai->fCoff); /* 系数 */
                break;
            }
        }
        if (i >= paimod->iLgcNum)  /* 建立瞬时值计算算法 */
        {
            /* Config MSU channel before LGC channel. */
            plcfgSave[i].ucHdCh=pch->phwai->ucModCh;
            plcfgSave[i].ucFiltNum=0xFF; 				/* 瞬时值算法还没有确定下来 */
            plcfgSave[i].fCoff=pch->phwai->fCoff;

            paimod->iLgcNum++;
            assert(paimod->iLgcNum <= DSP_LGC_AI_CH);
        }

        pcalmcfg=pmcfgSave+paimod->iMsucNum++;
        assert(paimod->iCalcNum <= DSP_MSU_AI_CH);

        if(CountFlag == 0)
        {
            if(Count == AIBaseCh_g)
            {
                /* 使用第AIBaseCh_g个配置作为基准通道 */
                CountFlag = 1;
                AIBaseCh_g = i;		/* 转换为瞬时值通道 */
            }
            Count++;
        }

#ifndef NO_DEBUG
        LOG_Dbg_Msg("%d\n",AIBaseCh_g,0,0,0,0,0);
#endif

        pcalmcfg->ucBgnLgcCh=i; /* 开始通道 */
        pcalmcfg->ucChNum=1; /* 通道个数 */

        if (pch->ucFiltTp<0x07) /* 算法分配 */
            pcalmcfg->ucArithNum=7;
        pcalmcfg->ucArithParm=0x20 | (pch->ucFiltTp & 0x07); /* 滤波预处理算法的参数 */

        /* 添加频率计算 */
        for (i=0; i<paimod->iLgcNum; i++)
        {
            /* 物理通道号与算法检测 */
            if (plcfgSave[i].ucHdCh == pch->phwai->ucModCh && (plcfgSave[i].ucFiltNum == 1 )) /* 算法对应 */
            {
                assert(plcfgSave[i].fCoff == pch->phwai->fCoff); /* 系数 */
                break;
            }
        }
        if (i >= paimod->iLgcNum)  /* 建立频率计算算法 */
        {
            /* Config MSU channel before LGC channel. */
            plcfgSave[i].ucHdCh=pch->phwai->ucModCh;
            plcfgSave[i].ucFiltNum=1;
            plcfgSave[i].fCoff=pch->phwai->fCoff;

            paimod->iLgcNum++;
            assert(paimod->iLgcNum <= DSP_LGC_AI_CH);
        }
    }
    else
        assert(FALSE);
}

/***********************************************************************
* DspCoeUpdate - 系数更新
*
* RETURNS: OK, or ERROR
*
*/
static EP_STATUS DspCoeUpdate(
    RD_HW_AI_CH *phwai,		/* AI物理通道 */
    RD_AI_MOD *paimod, 					/* 机箱 */
    DSP_LGC_AI_CFG *pdspl_cfg				/* DSP配置 */
)
{
    DSP_LGC_AI_CFG *pch;
    RD_HW_AI_CH *phch;
    EP_STATUS stsRet;

    stsRet=EP_SUCCESS;
    for (pch=pdspl_cfg; pch<pdspl_cfg+paimod->iLgcNum; pch++)
    {
        for(phch=phwai; phch<phwai+iHwAiChNum_g; phch++)
        {
            if((pch->ucHdCh == phch->ucModCh) && (phch->paimod == paimod))
            {
                pch->fCoff=phch->fCoff;
                break;
            }
        }
        stsRet=EP_ERROR;
        assert(phch<phwai+iHwAiChNum_g);
    }

    return stsRet;
}

/* 运行时更新扩展机箱计算用通道系数
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
EP_STATUS DspExtBoxCoeRunUpdate(void)
{
    DSP_LGC_AI_CFG *pch;
    RD_HW_AI_CH *phch;
    EP_STATUS stsRet;

    stsRet = EP_SUCCESS;
    for (pch = pdspl_cfg_ext_g; pch<pdspl_cfg_ext_g+aimodExt_g.iLgcNum; pch++)
    {
        /* 遍历逻辑通道 */
        for (phch = phwaich_g; phch<phwaich_g+iHwAiChNum_g; phch++)
        {
            /* 遍历物理通道 */
            if (((pch->ucHdCh - 1) == phch->ucModCh) && (phch->paimod == &aimodExt_g))
            {
                /* 通道系数赋值 */
                pch->fCoff = phch->fCoff;
                break;
            }
        }
        assert (phch<phwaich_g+iHwAiChNum_g);
        stsRet = EP_ERROR;
    }

    return stsRet;
}

#ifdef EXCITE_BUILD
/***********************************************************************
* Init_Redun_Finish - 初始化冗余机箱，用于励磁平台
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS Init_Redun_Finish(void)
{
    DSP_LGC_AI_CFG *plcfg;
    EP_STATUS stsRet;
    int iTxPts;
    int iCalcCfg;

    stsRet = EP_SUCCESS;
    iTxPts=uiAiRate_g/2000;		/* 采样频率 */
    if (!iTxPts)
        iTxPts=1;

    if (aimodRedun_g.iCalcNum)
        iCalcCfg=RD_Compack_Calc_Cfg(pdspc_cfg_redun_g, aimodRedun_g.iCalcNum);
    else
        iCalcCfg=0;

    if (aimodRedun_g.iLgcNum)
    {
        /* Translate hardware channel to suit DSP. */
        for (plcfg=pdspl_cfg_redun_g; plcfg<pdspl_cfg_redun_g+aimodRedun_g.iLgcNum; )
            plcfg++->ucHdCh++;
    }

    if (aimodRedun_g.iLgcNum || iRedunDiNum_g)
    {
        /* 初始化冗余机箱*/
        stsRet=Init_Redun_Box(uiAiPts_g, uiPwrFreq_g, iTxPts, &aimodRedun_g,
                              aimodRedun_g.iLgcNum, pdspl_cfg_redun_g, iCalcCfg, pdspc_cfg_redun_g);

        if (stsRet!=EP_SUCCESS)
        {
            if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_ALARM |ER_LOCK| ER_REPORT,
                           "冗余机箱异常(%02d)\n", AD_REDUNDANT_INIT_ERR, 0);
            }
            else if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_SAMPLE_ERR, ER_ALARM |ER_LOCK| ER_REPORT,
                           "Redundant Box error(%02d)\n", AD_REDUNDANT_INIT_ERR, 0);
            }
        }
    }

    return stsRet;
}

/***********************************************************************
* Init_OptComm_Finish - 初始化光纤通讯
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS Init_OptComm_Finish(void)
{
    DSP_LGC_AI_CFG *plcfg;
    EP_STATUS stsRet;
    int iTxPts;
    int iCalcCfg;

    stsRet = EP_SUCCESS;
    iTxPts=uiAiRate_g/2000;		/* 采样频率 */
    if (!iTxPts)
        iTxPts=1;

    if (aimodRedun_g.iCalcNum)
        iCalcCfg=RD_Compack_Calc_Cfg(pdspc_cfg_redun_g, aimodRedun_g.iCalcNum);
    else
        iCalcCfg=0;

    if (aimodRedun_g.iLgcNum)
    {
        /* Translate hardware channel to suit DSP. */
        for (plcfg=pdspl_cfg_redun_g; plcfg<pdspl_cfg_redun_g+aimodRedun_g.iLgcNum; )
            plcfg++->ucHdCh++;
    }

    if (aimodRedun_g.iLgcNum || iRedunDiNum_g)
    {
        /* 初始化光纤通讯*/
        stsRet=Init_Opt_Comm(uiAiPts_g, uiPwrFreq_g, iTxPts, &aimodRedun_g,
                             aimodRedun_g.iLgcNum, pdspl_cfg_redun_g, iCalcCfg, pdspc_cfg_redun_g);

        if (stsRet!=EP_SUCCESS)
        {
            if(ENG_MODE==0)
            {
                LOG_Write(LOG_KERNEL, "初始化光纤通讯失败.\n",NULL);
            }
            else if(ENG_MODE==1)
            {
                LOG_Write(LOG_KERNEL, "Failed to initialize optical fiber communication.\n",NULL);
            }
        }
    }

    return stsRet;
}
#endif

/***********************************************************************
* InitOptBoxChn2 - 初始化光纵机箱通道2
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 错误
*
*/
EP_STATUS InitOptBoxChn2(void)
{
    if ((aimodOpt_g[1].iLgcNum) && (bOptIs3EndRunMode_g || bOptChIsRedundMode_g)
            && (!abOptChIsInitOver_g[1]))
    {
        OPTCh2_AOCfgInitFinish();

        abOptChIsInitOver_g[1]=TRUE;
    }

    return EP_SUCCESS;
}

void DBCheck(void)
{
    int i;
    int iPos = 0;
    int iNeg = 0;

    LOG_Dbg_Msg("ulBufLen = %d\n", didb_g.ulBufLen, 0, 0, 0, 0, 0);

    for (i=0; i<didb_g.ulBufLen; i++)
    {
        if (i%iLgcDiChNum_g == 0)
        {
            LOG_Dbg_Msg("\n\n", 0, 0, 0, 0, 0, 0);
        }
        LOG_Dbg_Msg("%d = %x\n", i, didb_g.pbBufBgn[i], 0, 0, 0, 0);
        taskDelay(5);
    }

    LOG_Dbg_Msg("iPos = %d iNeg = %d\n", iPos, iNeg, 0, 0, 0, 0);
}

/* 是否配置光差通道
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL RD_ifOptCfg(void)
{
    int i;

    /* 应用程序设定为不插值 */
    if (!SampInsert_s)
    {
        return FALSE;
    }

    for (i = 0; i<2; i++)
    {
        if (aimodOpt_g[i].iLgcNum)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/* 传统采样是否需要插值(需要在脚本文件autoexec.ini文件中调用)
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void RD_ifSampInsert(void)
{
    SampInsert_s = FALSE;
}

/* 获取是否有GOOSE DO配置标识
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL RD_ifGooseDoCfg(void)
{
    if (iHdlDoNum_g)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    return FALSE;
}
