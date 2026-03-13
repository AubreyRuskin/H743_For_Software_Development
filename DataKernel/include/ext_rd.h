/********************************************************************************/
/*                                                                              */
/*      Copyright (c) 2002 SNAC(Guodian Nanjing Automation Co., Ltd.)           */
/*      All Rights Reserved.                                                    */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/* FILE NAME                                            VERSION                 */
/*                                                                              */
/*      realdata.h                                  EDP01-04-0.1                */
/*                                                                              */
/* COMPONENT                                                                    */
/*                                                                              */
/*      RD - Realtime data management.                                          */
/*                                                                              */
/* DESCRIPTION                                                                  */
/*                                                                              */
/*      This file contains interface to access realtime data.                   */
/*                                                                              */
/* AUTHOR                                                                       */
/*                                                                              */
/*      Daoxu Hu, SNAC                                                          */
/*                                                                              */
/* DATA STRUCTURES                                                              */
/*                                                                              */
/*      <TODO>                                                                  */
/*                                                                              */
/* FUNCTIONS                                                                    */
/*                                                                              */
/*      RD_INC_LGC_AI_P                 Macro to increase pointer of logic AI.  */
/*      RD_DEC_LGC_AI_P                 Macro to decrease pointer of logic AI.  */
/*      RD_INC_CALC_AI_P                Macro to increase pointer of calc. AI.  */
/*      RD_DEC_CALC_AI_P                Macro to decrease pointer of calc. AI.  */
/*                                                                              */
/* DEPENDENCIES                                                                 */
/*                                                                              */
/*      edpbase.h                       Basic head file of EDP 01 project.      */
/*                                                                              */
/* HISTORY                                                                      */
/*                                                                              */
/*         NAME            DATE                    REMARKS                      */
/*                                                                              */
/*      Daoxu Hu        2002.07.26      Created first version 0.1.              */
/*      Daoxu Hu        2003.02.27      Updated to version 0.2.                 */
/*      Daoxu Hu        2003.xx.xx      Updated to version 1.0                  */
/*                                                                              */
/********************************************************************************/

#ifndef REALDATA_H
#define REALDATA_H

#include "edpbase.h"
#include "math_compat.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define RD_BUF_CYC      10              /* 缓冲的数据周波数 */

#ifndef M_PI
#define M_PI            3.141592653589793238462643
#endif

#ifndef M_SQRT2
#define M_SQRT2         1.4142135623730950488016887
#endif

typedef struct
{
    uint8_t ucMod;
    uint8_t ucHdCh;
    uint8_t aucFilt[4];
} EXT_DI_CFG;

extern u_int uiPwrFreq_g;               /* 电力系统频率，50或者60 */
extern u_int uiAiRate_g;                /* 模拟量（AI）采样周期，次/秒 */
extern u_int uiAiPts_g;                 /* 模拟量（AI）每周波采样点数 */
extern u_int uiDioRate_g;               /* 数字量（DI/DO）刷新周期，次/秒 */

/* 初始化整个实时数据模块
 * 参数：   无
 * 返回值： 无 */
void EX_Rd_Init(void);

/* 取得模块AI逻辑通道和预处理数据指针
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulSmplClk，递增采样时钟，该时钟在同步脉冲到来的时刻清零
 *          ppxWr，用来返回指向该AI引擎的第0个预处理通道数据的指针
 * 返回值： 指向该AI引擎的第0个逻辑采样通道数据的指针
 * 注意：   要求AI引擎按照采样次序调用此函数获得指针来写实时数据，
 *          AI引擎可以对指针进行直接的写操作（如果没有预处理通道，则*ppxWr无效），
 *          内部数据按照通道优先的方式进行组织 */
float *RD_AI_Dat_P(void *pvAiMod, uint32_t ulSmplClk, COMPLEX **ppxWr);

/* 报告AI引擎采样同步信息
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulTime，采样同步脉冲到来时的系统32位us时钟
 *          ulLastClk，清零前的采样时钟值
 * 返回值： 无 */
void RD_Syn_AI_Clk(void *pvAiMod, uint32_t ulTime, uint32_t ulLastClk);

/* 报告AI引擎完成一次数据刷新
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 * 返回值： 无 */
void RD_End_Ai_Wr(void *pvAiMod);

#ifdef  __cplusplus
}
#endif

#endif                                  /* REALDATA_H */
