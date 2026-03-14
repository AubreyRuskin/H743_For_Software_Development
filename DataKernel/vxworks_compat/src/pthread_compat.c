#include "pthread_compat.h"

void pthread_exit(void *retval)
{
    /* TODO: 用 FreeRTOS vTaskDelete(NULL) 实现 */
    (void)retval;
    while (1) { } /* 不应返回 */
}
