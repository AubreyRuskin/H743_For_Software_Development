#ifndef VXWORKS_TYPE_H
#define VXWORKS_TYPE_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
typedef uint8_t UINT8;
typedef int8_t INT8;
typedef uint16_t UINT16;
typedef int16_t INT16;
typedef uint32_t UINT32;
typedef int32_t INT32;
typedef uint64_t UINT64;
typedef int64_t INT64;
typedef uint32_t UINT;
typedef uint8_t UCHAR;
typedef void VOID;
typedef int BOOL;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define FAST    register

typedef unsigned long ULONG;
typedef int    (*FUNCPTR)();   /* pointer to function returning void */
typedef int     (*FUNCPTR_INT)();/* pointer to function returning int */
typedef void    (*VOIDFUNCPTR)(); /* pointer to function returning void */

#define IMPORT extern
#define LOCAL static



typedef int  STATUS;
// 模拟 VxWorks 宏定义
#define OK                   0
/*
 * ERROR 宏与 STM32 HAL 的 ErrorStatus 枚举冲突 (stm32h7xx.h)
 * 确保 stm32h7xx.h 的枚举已被解析后再定义此宏
 */
#if defined(STM32H743xx) || defined(STM32H7xx)
#include "stm32h7xx.h"
#endif
#ifndef ERROR
#define ERROR                (-1)
#endif

// typedef enum
//     {
//     ERROR = -1,
//     OK    = 0
//     } STATUS;


// #define  MAX_ID_LEN 138
#endif