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
/*      extbox.h                                    EDP01-04-0.1                */
/*                                                                              */
/* COMPONENT                                                                    */
/*                                                                              */
/*      RD - Realtime data management(ExtBox).                                  */
/*                                                                              */
/* DESCRIPTION                                                                  */
/*                                                                              */
/*      This file contains interface to extend box data acq. system.            */
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
/*      None                                                                    */
/*                                                                              */
/* DEPENDENCIES                                                                 */
/*                                                                              */
/*      None                                                                    */
/*                                                                              */
/* HISTORY                                                                      */
/*                                                                              */
/*         NAME            DATE                    REMARKS                      */
/*                                                                              */
/*      Daoxu Hu        2003.09.10      Created first version 0.1.              */
/*                                                                              */
/********************************************************************************/

#ifndef EXT_BOX_H
#define EXT_BOX_H

#include "dspai.h"

typedef struct
{
    uint32_t *pulStsPos;
    uint32_t ulStsMsk;
} EXT_DI_HND;

extern int iExtDiNum_g;                 /* 扩展机箱DI总数 */

/* 初始化（并启动）扩展机箱
 * 参数：	uiSmplRate，采样速率
 *			uiSysFreq，系统频率
 *			uiTxPts，每次传送采样点数
 *			pvAiMod，该模块（扩展机箱负责的所有AI采集/计算通道）的句柄
 *			uiLgcCh，采样的逻辑通道数
 *          plgccfg，指向逻辑通道配置数组第0个元素的指针，数组元素有unLgcCh个
 *          uiCalcCfg，预处理通道配置数
 *          pcalccfg，指向预处理通道配置数组第0个元素的指针，数组元素uiCalcCfg个
 * 返回值：	EP_SUCCESS，正常返回
 *			EP_BUF_ERR，内存错误
 *			EP_COM_ERR，扩展机箱通信出错 */
EP_STATUS Init_Ext_Box(u_int uiSmplRate, u_int uiSysFreq, u_int uiTxPts,
                       void *pvAiMod, u_int uiLgcCh, DSP_LGC_AI_CFG *plgccfg,
                       u_int uiCalcCfg, DSP_CALC_AI_CFG *pcalccfg);

/* 初始化DI通道
 * 参数：   iModAddr，模块硬件地址
 *          uiCh，在本模件内的DO物理通道号，从0开始
 *          ulFilt，去抖动时间，单位us
 * 返回值： 用来索引DI通道的void指针，或者NULL表示调用出错 */
void *EX_Init_DI(int iModAddr, u_int uiCh, uint32_t ulFilt);

#if 0
/* 读取DI数据实时状态
 * 参数：   pvDiCh，用来索引DI数据元素的void指针，应该是SIO_Init_DI的返回值
 * 返回值： 此DI通道的当前状态，TRUE=闭合；FALSE=打开 */
BOOL EX_Get_DI(void *pvDiCh);
#endif

/********************************************************************************/
/* 子程序名: EX_Rd_Data                                                         */
/* 入口参数: 无                                                                 */
/* 出口参数: 无                                                                 */
/* 功能:     读取扩展机箱实时AI/DI数据                                          */
/********************************************************************************/
EP_STATUS EX_Rd_Data(void);

/* 读取DI数据实时状态
 * 参数：   pvDiCh，用来索引DI数据元素的void指针，应该是SIO_Init_DI的返回值
 * 返回值： 此DI通道的当前状态，TRUE=闭合；FALSE=打开 */
static __inline__ BOOL EX_Get_DI(void *pvDiCh)
{
    EXT_DI_HND *phnd;

    phnd=(EXT_DI_HND*)pvDiCh;
    /*原代码有BUG,张云改过
    return (*phnd->pulStsPos & phnd->ulStsMsk);
    */

    if(*phnd->pulStsPos & phnd->ulStsMsk)
        return  TRUE;
    else
        return  FALSE;
}
#endif
