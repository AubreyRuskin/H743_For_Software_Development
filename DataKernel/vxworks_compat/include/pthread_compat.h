#ifndef PTHREAD_COMPAT_H
#define PTHREAD_COMPAT_H

/*
 * pthread compat layer for FreeRTOS (arm-none-eabi)
 * arm-none-eabi 没有 <pthread.h>, 此处提供使用到的 stub 声明
 * 实际实现需要在 vxworks_compat/src/ 中对接 FreeRTOS API
 */

#include <stddef.h>

/* pthread_exit stub: 在 FreeRTOS 中应调用 vTaskDelete(NULL) */
void pthread_exit(void *retval);

#endif /* PTHREAD_COMPAT_H */
