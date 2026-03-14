#include "semLib.h"
#include <stdlib_compat.h>
#include "vxworks_type.h"

SEM_ID semMCreate(int options)
{
    /* TODO: 用 FreeRTOS xSemaphoreCreateMutex 实现 */
    (void)options;
    return NULL_ID;
}

SEM_ID semBCreate(int options, SEM_B_STATE initialState)
{
    /* TODO: 用 FreeRTOS xSemaphoreCreateBinary 实现 */
    (void)options; (void)initialState;
    return NULL_ID;
}

SEM_ID semCCreate(int options, int initialCount)
{
    /* TODO: 用 FreeRTOS xSemaphoreCreateCounting 实现 */
    (void)options; (void)initialCount;
    return NULL_ID;
}

int semTake(SEM_ID semId, int timeout)
{
    /* TODO: 用 FreeRTOS xSemaphoreTake 实现 */
    (void)semId; (void)timeout;
    return ERROR;
}

int semGive(SEM_ID semId)
{
    /* TODO: 用 FreeRTOS xSemaphoreGive 实现 */
    (void)semId;
    return ERROR;
}

int semDelete(SEM_ID semId)
{
    /* TODO: 用 FreeRTOS vSemaphoreDelete 实现 */
    (void)semId;
    return ERROR;
}

SEM_ID semRWCreate(int options, int maxReaders)
{
    /* TODO: 实现读写锁 */
    (void)options; (void)maxReaders;
    return NULL_ID;
}

STATUS semExchange(SEM_ID giveSemId, SEM_ID takeSemId, int timeout)
{
    /* TODO: 实现信号量交换 */
    (void)giveSemId; (void)takeSemId; (void)timeout;
    return ERROR;
}

STATUS semFlush(SEM_ID semId)
{
    /* TODO: 实现信号量 flush */
    (void)semId;
    return ERROR;
}

STATUS semRTake(SEM_ID semId, int timeout)
{
    /* TODO: 实现读锁 */
    (void)semId; (void)timeout;
    return ERROR;
}

STATUS semWTake(SEM_ID semId, int timeout)
{
    /* TODO: 实现写锁 */
    (void)semId; (void)timeout;
    return ERROR;
}
