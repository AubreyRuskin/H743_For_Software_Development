/* HardClock.h - This file contains the functions to handle the clock chip */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 22nov06, hcj Created first version 0.1.
*/

/*
DESCRIPTION
This file contains the functions to handle the clock chip.
*/

#ifndef HARDCLOCK_H

/* includes */

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
#include "config05.h"
#endif

#include "edpbase.h"

/* defines */

#define HARDCLOCK_H

#ifdef	__cplusplus
extern "C" {
#endif

/*typedefs */

typedef struct tag_CHardClock
{
    EP_STATUS (*SetClock)(EP_DATE_TIME *);		/*将当前时钟设置到时钟芯片中*/
    EP_STATUS (*GetClock)(EP_DATE_TIME *);				/* 从时钟芯片得到时钟信息*/
} CHardClock;

/* globals */

extern CHardClock cHardClock ; 		/* 兼容性增加 */

/***********************************************************************
* SetClock - 设置时钟
*
* RETURNS:
*			EP_SUCCESS, 正常
*			EP_BUF_ERR, 错误
*
*/
EP_STATUS SetClock(
    EP_DATE_TIME *time		/* 时钟指针 */
);

/***********************************************************************
* GetClock - 读取时钟
*
* RETURNS:
*			EP_SUCCESS, 正常
*			EP_BUF_ERR, 错误
*
*/
EP_STATUS GetClock(
    EP_DATE_TIME *time		/* 时钟指针 */
);

#ifdef	__cplusplus
}
#endif
#endif