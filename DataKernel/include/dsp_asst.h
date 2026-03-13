/* dsp_asst.h - test subroutine library for handling algorithms of DSP */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 29jul06, dy add digital differential analyzer.
01a, 27dec05, dy first created.
*/

/*
DESCRIPTION
This module includes test subroutine library for handling algorithms of DSP.
*/

#ifndef DSP_ASST_H
#define DSP_ASST_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "dsp.h"

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

#ifdef  __cplusplus
}
#endif

#endif
