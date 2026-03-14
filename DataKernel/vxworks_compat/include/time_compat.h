#ifndef TIME_COMPAT_H
#define TIME_COMPAT_H

/* compat header for <time.h> */

/* 启用 POSIX timer 声明 (clock_gettime, clock_settime 等) */
#ifndef _POSIX_TIMERS
#define _POSIX_TIMERS 1
#endif

/* 启用 CLOCK_MONOTONIC 等单调时钟定义 */
#ifndef _POSIX_MONOTONIC_CLOCK
#define _POSIX_MONOTONIC_CLOCK 1
#endif

#include <time.h>

#endif /* TIME_COMPAT_H */
