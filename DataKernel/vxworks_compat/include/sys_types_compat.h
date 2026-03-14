#ifndef SYS_TYPES_COMPAT_H
#define SYS_TYPES_COMPAT_H

/* compat header for <sys/types.h> */
/* 直接复用 arm-none-eabi 工具链的定义，避免类型冲突 */
#include <sys/types.h>

#endif /* SYS_TYPES_COMPAT_H */
