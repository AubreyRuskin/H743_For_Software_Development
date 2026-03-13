#ifndef LOG_LIB_H
#define LOG_LIB_H

typedef int _Vx_usr_arg_t;

int logMsg (
    char    *fmt,         /* 格式化字符串，类似 printf */
    _Vx_usr_arg_t arg1,   /* 第1个参数 */
    _Vx_usr_arg_t arg2,   /* 第2个参数 */
    _Vx_usr_arg_t arg3,   /* 第3个参数 */
    _Vx_usr_arg_t arg4,   /* 第4个参数 */
    _Vx_usr_arg_t arg5,   /* 第5个参数 */
    _Vx_usr_arg_t arg6    /* 第6个参数 */
);

#endif /* LOG_LIB_H */