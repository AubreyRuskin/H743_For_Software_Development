#ifndef VXWORKS_TYPE_H
#define VXWORKS_TYPE_H

#include <stdint.h>
#include <stddef.h>
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
#define ERROR                (-1)

// typedef enum
//     {
//     ERROR = -1,
//     OK    = 0
//     } STATUS;


// #define  MAX_ID_LEN 138
#endif