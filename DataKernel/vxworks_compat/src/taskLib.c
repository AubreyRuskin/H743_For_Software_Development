#include <vxWorks.h>
#include <string.h>
#include "vxworks_type.h"
#include "taskLib.h"

TASK_ID taskSpawn(char *name, int priority, int options, int stackSize,
                  FUNCPTR entryPt,
                  int arg1, int arg2, int arg3, int arg4, int arg5,
                  int arg6, int arg7, int arg8, int arg9, int arg10)
{
    /* TODO: 用 FreeRTOS xTaskCreate 实现 */
    (void)name; (void)priority; (void)options; (void)stackSize;
    (void)entryPt;
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    (void)arg6; (void)arg7; (void)arg8; (void)arg9; (void)arg10;
    return 0;
}

TASK_ID taskNameToId(const char *name)
{
    (void)name;
    return 0;
}

STATUS taskPrioritySet(TASK_ID tid, int newPriority)
{
    /* TODO: 用 FreeRTOS vTaskPrioritySet 实现 */
    (void)tid; (void)newPriority;
    return OK;
}

STATUS taskOptionsSet(TASK_ID tid, int options, int mask)
{
    (void)tid; (void)options; (void)mask;
    return OK;
}

STATUS taskDelete(TASK_ID tid)
{
    /* TODO: 用 FreeRTOS vTaskDelete 实现 */
    (void)tid;
    return OK;
}

int taskLock(void)
{
    /* TODO: 用 FreeRTOS vTaskSuspendAll 实现 */
    return OK;
}

int taskUnlock(void)
{
    /* TODO: 用 FreeRTOS xTaskResumeAll 实现 */
    return OK;
}

STATUS taskDelay(int ticks)
{
    /* TODO: 用 FreeRTOS vTaskDelay 实现 */
    (void)ticks;
    return OK;
}

STATUS taskStatusString(TASK_ID tid, char *buf)
{
    (void)tid;
    strcpy(buf, "READY");
    return OK;
}

STATUS taskIdVerify(int tid)
{
    (void)tid;
    return OK;
}

TASK_ID taskIdSelf(void)
{
    /* TODO: 用 FreeRTOS xTaskGetCurrentTaskHandle 实现 */
    return 0;
}

STATUS taskMsSleep(int ms)
{
    /* TODO: 用 FreeRTOS vTaskDelay(pdMS_TO_TICKS(ms)) 实现 */
    (void)ms;
    return OK;
}

const char *taskName(TASK_ID tid)
{
    /* TODO: 用 FreeRTOS pcTaskGetName 实现 */
    (void)tid;
    return "task";
}

STATUS taskInfoGet(TASK_ID tid, TASK_DESC *pTaskDesc)
{
    (void)tid; (void)pTaskDesc;
    return OK;
}
