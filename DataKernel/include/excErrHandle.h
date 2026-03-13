/* adc.h - subroutine library for handling the A/D convertion and the DSP */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 29jul06, dy add calibration methods for measuring.
01a, 27dec05, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling the A/D convertion and the DSP.
*/

#ifndef ABC_H
#define ABC_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include <stdio_compat.h>
#include "string_compat.h"
#include "ctype_compat.h"
#include <ioLib.h>
#include <intLib.h>
#include <taskLib.h>
#include <semLib.h>

#include "string_compat.h"
#include "ctype_compat.h"
#include <stdio_compat.h>
#include <dirent_compat.h>
#include <sys_stat_compat.h>
#include <ioLib.h>
#include <lstLib.h>
#include <semLib.h>
#include <dosFsLib.h>

#include "vxWorks.h"
#include "logLib.h"


// #include "m8260IntrCtl.h"

#include "excLib.h"
// #include "arch\ppc\esfPpc.h"
#include "filetool.h"

#include "edpbase.h"
#include "logmsg.h"

/* defines */

#if defined(EDP03_BUILD)
#define EXC_ERROR_HANDLE_POS (pramLowMemAdrs-0x1000) 		/* 装置热启动时未被初始化的可供应用层使用的内存块地址 */
#define REBOOT_INFO_SAVE_POS (pramLowMemAdrs-0x2000)  /* address for saving the reboot information. */
#define START_INFO_SAVE_POS (pramLowMemAdrs-0x3000)     /* 重启信息记录 */
#else
#define EXC_ERROR_HANDLE_POS (0x2F000) 		/* 装置热启动时未被初始化的可供应用层使用的内存块地址 */
#define REBOOT_INFO_SAVE_POS 0x2E000 /* address for saving the reboot information. */
#define START_INFO_SAVE_POS (0x2D000)    /* 重启信息记录 */
#endif

/* functions */

/***********************************************************************
* Exc_SysregExcHandle - 平台异常处理函数挂接
*
* RETURNS: 无
*
*/
void Exc_SysregExcHandle();

/* write the low address memory when calling sysToMonitor.
 * Para:
 *     callType, call type,
 *     REBOOT_UNKNOWN: initialize calling;
 *     REBOOT_ACTIVE: normal calling;
 *     REBOOT_EXCEP: exception calling.
 * Return:
 *     OK, ERROR.
 */
BOOL Exc_WrRebootInfo(int32_t callType);

/* read the low address memory when power up.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void Exc_RdRebootInfo(void);

/* 记录任务状态 */
extern void Exc_RecTaskInfo(void);

/* 记录启动时间.
 * Para:
 *     ulSn, 序号.
 * Return:
 *     NONE.
 */
void Exc_RecStartTm(int32_t ulSn);

#ifdef  __cplusplus
}
#endif

#endif

