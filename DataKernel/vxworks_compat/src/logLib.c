#include "logLib.h"
#include <stdio_compat.h>

int logMsg(
    char *fmt,
    _Vx_usr_arg_t arg1,
    _Vx_usr_arg_t arg2,
    _Vx_usr_arg_t arg3,
    _Vx_usr_arg_t arg4,
    _Vx_usr_arg_t arg5,
    _Vx_usr_arg_t arg6)
{
    /* TODO: 对接 FreeRTOS 下的日志输出 (UART/RTT 等) */
    (void)fmt;
    (void)arg1; (void)arg2; (void)arg3;
    (void)arg4; (void)arg5; (void)arg6;
    return 0;
}
