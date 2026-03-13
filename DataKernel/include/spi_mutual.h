/* spi_mutual.h - subroutine library for handling SPI and IO Module mutual operation */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 25may09, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling SPI and IO Module mutual operation.
*/

#ifndef SPI_MUTUAL_H
#define SPI_MUTUAL_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "edpbase.h"
#include "realdatadef.h"    /* 模件类型 */
#include "spiio.h"

/* global functions */

/***********************************************************************
* ShowIOSts - 显示IO模件状态
*
* RETURNS: 无
*
*/
extern void ShowIOSts(void);

/* stop SPI communication.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void IO_StopSpi(void);

/* start SPI communication.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void IO_StartSpi(void);

/* send common command.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
extern void IO_SndComCmd(SPI_IO_BUF *pspibuf);

/* clear common command.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
extern void IO_ClrComCmd(SPI_IO_BUF *pspibuf);

/* the second board get the overflow value.
 * Para:
 *     pspibuf, IO buffer.
 *     pSrc, Rcv spi packet buf
 * Return: NONE.
 */
void IO_Num_2_Get_OvVal(SPI_IO_BUF *pspibuf, uint8_t *pSrc);

/* send periodic command.
 * Para:
 *     pspibuf, IO buffer.
 * Return: NONE.
 */
extern void IO_SndPeriodCmd(SPI_IO_BUF *pspibuf);

/* adjusting, called by MMI.
 * Para:
 *     ucModAddr, address.
 *     ucAdjCtrl, adjusting control word.
 *     ucChnStrl, channel control word.
 * Return: EP_SUCCESS, EP_ERROR.
 */
extern EP_STATUS IO_adjust(uint8_t ucModAddr, uint8_t ucAdjCtrl, uint8_t ucChnStrl);

/***********************************************************************
* SetMBDoTest - 获取母板开出(电铁使用)
*
* RETURNS: EP_ERROR, EP_SUCCESS
*
*/
extern EP_STATUS SetMBDoTest(
    int iSetVal		/* 设定值 */
);

/***********************************************************************
* SetOptCoupleEnable - 设置光耦是否导通(电铁使用)
*
* RETURNS: 无
*
*/
extern void SetOptCoupleEnable(
    BOOL bSts		/* 设置值 */
);

/* get the IO module type description information.
 * Para:
 *     ucModType, module type.
 * Return:
 *     pointer to description information.
 */
extern uint8_t *IO_GetModDesInfo(uint8_t ucModType);

#ifdef  __cplusplus
}
#endif

#endif
