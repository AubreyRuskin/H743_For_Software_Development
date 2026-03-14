#include "intLib.h"
#include <string.h>

#define MAX_INTERRUPTS 256

typedef struct {
    VOIDFUNCPTR routine;
    int arg;
    int enabled;
    int interval_us;
} InterruptVector;

static InterruptVector interruptVectors[MAX_INTERRUPTS];
static int global_interrupt_lock = 0;

STATUS initTimer(int vector, int interval_us)
{
    /* TODO: 用 FreeRTOS 软件定时器或硬件定时器实现 */
    if (vector < 0 || vector >= MAX_INTERRUPTS)
        return ERROR;
    interruptVectors[vector].interval_us = interval_us;
    return OK;
}

STATUS intEnable(int vector)
{
    /* TODO: 启动对应定时器 */
    if (vector < 0 || vector >= MAX_INTERRUPTS)
        return ERROR;
    interruptVectors[vector].enabled = 1;
    return OK;
}

STATUS intDisable(int vector)
{
    if (vector < 0 || vector >= MAX_INTERRUPTS)
        return ERROR;
    interruptVectors[vector].enabled = 0;
    return OK;
}

STATUS intConnect(int vector, VOIDFUNCPTR routine, int arg1)
{
    if (vector < 0 || vector >= MAX_INTERRUPTS)
        return ERROR;
    interruptVectors[vector].routine = routine;
    interruptVectors[vector].arg = arg1;
    interruptVectors[vector].enabled = 0;
    return OK;
}

int intLock(void)
{
    /* TODO: 用 FreeRTOS taskENTER_CRITICAL / portDISABLE_INTERRUPTS 实现 */
    global_interrupt_lock = 1;
    return 0;
}

int intUnlock(int k)
{
    /* TODO: 用 FreeRTOS taskEXIT_CRITICAL / portENABLE_INTERRUPTS 实现 */
    (void)k;
    global_interrupt_lock = 0;
    return 0;
}
