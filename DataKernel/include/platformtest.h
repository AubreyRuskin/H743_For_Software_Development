/* platformtest.h - subroutine library for testing the hardware platform */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 22mar07, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for testing the hardware platform.
*/

#ifndef PLATFORMTEST_H
#define PLATFORMTEST_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "edpbase.h"
#include "math_compat.h"
#include "logmsg.h"
#include "dsp.h"
#include "swcfg.h"

/***********************************************************************
* FT_Temp_Name_New_Test - 临时函数(防止修改filetool.c)，用于测试
*
* RETURNS: 无
*
*/
void FT_Temp_Name_New_Test(
    uint8_t *pucRslt,
    const uint8_t *strFile
);

/* Prepair to update/create a file.  (Using temp file to keep data integrity.)
 * Parameters:
 *      strFile, name of file to be updated.
 * Return:
 *      >=0, file descriptor. For using read/write function.
 *      EP_PARM_ERR, the file does not exist.
 *      EP_FILE_ERR, create temp file failure. */
EP_STATUS FT_Bgn_Update_Test(const uint8_t *strFile);

/* Finish updating/creating file.  Temp file is cleaned normally.
 * Parameters:
 *      strFile, name of file to be updated.
 *      iFd, file descriptor returned by FT_Bgn_Update.
 * Return:
 *      EP_SUCCESS, update OK.
 *      EP_ERROR, operating failure. */
EP_STATUS FT_End_Update_Test(const uint8_t *strFile, int iFd);

/***********************************************************************
* FileWrInit - 文件写初始化
*
* RETURNS: 无
*
*/
EP_STATUS FileWrInit(void);

/***********************************************************************
* FileWrFile - 文件写
*
* RETURNS: 无
*
*/
EP_STATUS FileWrFile(void);

/***********************************************************************
* FileRdFile - 从文件读
*
* RETURNS: 无
*
*/
EP_STATUS FileRdFile(void);

/* calculate the interval of two sampling pointing.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void us_Interval_Cal (uint32_t ulSampleRate);

#ifdef  __cplusplus
}
#endif

#endif
