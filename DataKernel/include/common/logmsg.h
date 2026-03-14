/* logmsg.h - This file contains external interface of log message module */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 29dec02, hdx Verified version 0.1.
01a, 23oct02, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains external interface of log message module.
*/

#ifndef LOGMSG_H
#define LOGMSG_H

/* includes */

#include "datetime.h"
#include "errtest.h"
#include "syslog_compat.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* defines */

#define LOG_RUN     0x0001	/* 运行信息，如开关变位、自检出错等 */
#define LOG_OPRATE  0x0002  /* 操作信息，如修改定值、控制输出等 */
#define LOG_ARITH   0x0004  /* 算法模块的信息，如数据异常等 */
#define LOG_INFO    0x0008  /* 信息处理模块的信息，如通信中断，规约异常等 */
#define LOG_KERNEL  0x0010  /* 底层支持模块的信息，如非法调用、程序复位等 */

#define MAX_LOG_NUM 50000  /* 每次运行最多日志记录条数, 按MMI处理 */

#define MyAssert(test, unOpFlag) ((void) ((test) ? ((void) 0) : LOG_Assert(""#test"", __FILE__, __LINE__, unOpFlag)))

/* 输出调试信息
 * 参数: strFmt，欲显示的格式字符串, 类似printf格式串, 但不支持浮点数.
 *       iArg1-iArg6, 格式字符串中指定要输出的参数，如不足6个，最后应补0.
 * 返回值: 无
 * 注意：
 *    1、此函数可以在中断的上下文中调用, 调用LOG_OpenLog()函数后或在FAST_BOOT跳上跳线情况下输出;
 *    2、信息以后台方式进行输出，故要求格式串或者字符串参数全局有效, 否则输出可能为乱码.
 */
#define LOG_Dbg_Msg MylogMsg

/* globals */

extern BOOL bLogWriteTaskStartFlag_g;
extern EP_DATE_TIME LOG_dtLastWdRebootTime_g;			/* 从日志获得的，看门狗上次重启绝对时间 */

/* global functions */

/* initialize LOG module.
 * Para:
 *     NONE.
 * Return:
 *     EP_STATUS, EP_ERROR.
 */
extern EP_STATUS LOG_Init(void);

/* redefined logmsg.
 * Para:
 *     NONE.
 * Return:
 *     The number of bytes written to the log queue,
 *     or EOF if the routine is unable to write a message.
 */
extern int MylogMsg (char *fmt, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6);

/* enable the LOG function.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void LOG_OpenLog();

/* disable the LOG function.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void LOG_CloseLog();

/* 记录系统日志
 * 参数: unLvl，日志级别，可且仅可取如下值其中之一：
 *           LOG_RUN; LOG_OPRATE; LOG_ARITH; LOG_INFO; LOG_KERNEL.
 *       strMsg，日志信息，受日志最大长度限制，有可能被截尾.
 *       pdttm，日志时间，如果为NULL，系统会记录调用发生的时间.
 * 返回值: 无
 * 注意: 此函数可以在中断的上下文中调用
 *
 */
extern void LOG_Write(uint16_t unLvl, const uint8_t *strMsg, const EP_DATE_TIME *pdttm);

/* get the flag if boot frequently.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL LOG_GetBootFlag(void);


/*功能：记录特殊日志条目
　参数：strItemKey  日志条目关键字字符串
　　　　strItemContent  日志条目内容字符串
  注意：strItemKey+strItemContent长度不要超过80字节,否则会截尾
  　　　相同关键字的特殊日志可以被重复写入，覆盖掉以前的日志
         */
void  LOG_ExtraItemWrite(const uint8_t *strItemKey, const uint8_t  *strItemContent);
#ifdef  __cplusplus
}
#endif

#endif                                  /* LOGMSG_H */