/* io_ctrl.h - I/O Control program implementation file */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 14feb06, cxz first created.
*/

/*
DESCRIPTION
I/O Control program implementation file.
*/

/* includes */

#ifndef IO_CTRL_H
#define IO_CTRL_H
#include <vxworks_type.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* typedefs */

typedef enum IO_PIN_FUN_ENUM      /* 设置引脚的方向 */
{
    IO_SET_PIN_IO = 0,         /* 设置引脚为I/O */
    IO_SET_PIN_PERIPHERAL,     /* 设置引脚为外设 */
    IO_SET_PIN_FUN_END,        /* Pin功能设置结束 */
} IO_PIN_FUN;

typedef enum IO_DIRECTION_ENUM    /* 设置IO引脚的方向 */
{
    IO_DIR_IN_BI = 0,       /* 设置引脚为输入或双向 */
    IO_DIR_OUT,      /* 设置引脚为输出 */
    IO_DIR_END,  /* IO方向设置结束 */
} IO_DIRECTION;

typedef enum IO_OUT_MODE_ENUM  /* 设置为输出的模式 */
{
    IO_OUT_MODE_NORMAL = 0,   /* 设置引脚输出为正常输出 */
    IO_OUT_MODE_OPEN_DRAIN,      /* 设置引脚输出为开漏输出 */
    IO_OUT_MODE_END,    /* 输出模式设置结束 */
} IO_OUT_MODE;

typedef enum IO_PORT_NUM_ENUM
{
    IO_PORT_A = 0,          /* Port A */
    IO_PORT_B,              /* Port B */
    IO_PORT_C,              /* Port C */
    IO_PORT_D,              /* Port D */
    IO_PORT_END,            /* 端口结束 */
} IO_PORT_NUM;

typedef enum IO_OUTPUT_VAL_ENUM   /* I/O口输出的值 */
{
    IO_OUTPUT_ZERO = 0,     /* 输出0 */
    IO_OUTPUT_ONE,          /* 输出1 */
    IO_OUTPUT_END,          /* 输出值结束 */
} IO_OUTPUT_VAL;

typedef enum IO_OPERATE_RETURN_CODE_ENUM
{
    IO_OPERATE_OK = 0,      /* I/O操作成功 */
    IO_SET_PIN_FUN_ERROR,   /* 设置引脚功能失败 */
    IO_SET_PIN_DIR_ERROR,   /* 设置引脚输入输出方向错误 */
    IO_OUT_MODE_ERROR,      /* 输出模式设置失败 */
    IO_OUTPUT_ERROR,        /* 输出错误 */
} IO_OPERATE_RETURN_CODE;

/* functions */

/* set the parall port function.
 * Para:
 *     port, port type, IO_PORT_NUM.
 *     pin, pinout, can be multi pinout.
 *     fun, function.
 * Return:
 *     IO_SET_PIN_FUN_ERROR, IO_OPERATE_OK.
 */
int32_t IO_Set_Port_Fun(uint32_t port, uint32_t pin, IO_PIN_FUN fun);

/* set the parall port direction.
 * Para:
 *     port, port type, IO_PORT_NUM.
 *     pin, pinout, can be multi pinout.
 *     fun, function.
 * Return:
 *     IO_SET_PIN_FUN_ERROR, IO_OPERATE_OK.
 */
int32_t IO_Set_Port_Direction(uint32_t port, uint32_t pin, IO_DIRECTION fun);

/* set the parall port output mode.
 * Para:
 *     port, port type, IO_PORT_NUM.
 *     pin, pinout, can be multi pinout.
 *     fun, mode.
 * Return:
 *     IO_OUT_MODE_ERROR, IO_OPERATE_OK.
 */
int32_t IO_Set_Port_Out_Mode(uint32_t port, uint32_t pin, IO_OUT_MODE fun);

/* set the parall port output data.
 * Para:
 *     port, port type, IO_PORT_NUM.
 *     pin, pinout, can be multi pinout.
 *     fun, mode.
 * Return:
 *     value of port before setting, error if 0xffffffff.
 */
uint32_t IO_Set_Port_Output(uint32_t port, uint32_t pin, IO_OUTPUT_VAL fun);

/* read the data of parall port.
 * Para:
 *     port, port type, IO_PORT_NUM.
 * Return:
 *     value of this port, error if 0xffffffff.
 */
uint32_t IO_Get_Port_Val(IO_PORT_NUM port);

#ifdef  __cplusplus
}
#endif

#endif