#include <vxWorks.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "vxworks_type.h"
#include "taskLib.h"

#define VX_TASK_REGISTRY_MAX 32

typedef struct
{
    TaskHandle_t handle;
    FUNCPTR entry;
    int options;
    int stack_size_bytes;
    char name[VX_TASK_NAME_LENGTH + 1];
} vx_task_record_t;

typedef struct
{
    FUNCPTR entry;
    int options;
    int stack_size_bytes;
    char name[VX_TASK_NAME_LENGTH + 1];
    int args[10];
} vx_task_spawn_ctx_t;

static vx_task_record_t g_vx_task_registry[VX_TASK_REGISTRY_MAX];

static TaskHandle_t vx_tid_to_handle(TASK_ID tid)
{
    return (TaskHandle_t)(uintptr_t)tid;
}

static BaseType_t vx_registry_register(TaskHandle_t handle,
                                       const char *name,
                                       FUNCPTR entry,
                                       int options,
                                       int stack_size_bytes)
{
    int i;
    BaseType_t inserted = pdFALSE;

    taskENTER_CRITICAL();
    for (i = 0; i < VX_TASK_REGISTRY_MAX; ++i)
    {
        if (g_vx_task_registry[i].handle == NULL)
        {
            g_vx_task_registry[i].handle = handle;
            g_vx_task_registry[i].entry = entry;
            g_vx_task_registry[i].options = options;
            g_vx_task_registry[i].stack_size_bytes = stack_size_bytes;
            if (name != NULL)
            {
                strncpy(g_vx_task_registry[i].name, name, VX_TASK_NAME_LENGTH);
                g_vx_task_registry[i].name[VX_TASK_NAME_LENGTH] = '\0';
            }
            else
            {
                strcpy(g_vx_task_registry[i].name, "vxTask");
            }
            inserted = pdTRUE;
            break;
        }
    }
    taskEXIT_CRITICAL();

    return inserted;
}

static vx_task_record_t *vx_registry_find_by_handle(TaskHandle_t handle)
{
    int i;

    for (i = 0; i < VX_TASK_REGISTRY_MAX; ++i)
    {
        if (g_vx_task_registry[i].handle == handle)
        {
            return &g_vx_task_registry[i];
        }
    }

    return NULL;
}

static vx_task_record_t *vx_registry_find_by_name(const char *name)
{
    int i;

    if (name == NULL)
    {
        return NULL;
    }

    for (i = 0; i < VX_TASK_REGISTRY_MAX; ++i)
    {
        if ((g_vx_task_registry[i].handle != NULL) &&
            (strncmp(g_vx_task_registry[i].name, name, VX_TASK_NAME_LENGTH) == 0))
        {
            return &g_vx_task_registry[i];
        }
    }

    return NULL;
}

static void vx_registry_unregister(TaskHandle_t handle)
{
    vx_task_record_t *record;

    taskENTER_CRITICAL();
    record = vx_registry_find_by_handle(handle);
    if (record != NULL)
    {
        memset(record, 0, sizeof(*record));
    }
    taskEXIT_CRITICAL();
}

static UBaseType_t vx_to_freertos_priority(int vx_priority)
{
    int clamped = vx_priority;

    if (clamped < 0)
    {
        clamped = 0;
    }
    else if (clamped > 255)
    {
        clamped = 255;
    }

    return (UBaseType_t)((configMAX_PRIORITIES - 1) -
                         ((clamped * (configMAX_PRIORITIES - 1)) / 255));
}

static void vx_task_auto_exit(vx_task_spawn_ctx_t *ctx, TaskHandle_t self)
{
    vx_registry_unregister(self);
    vPortFree(ctx);
    vTaskDelete(NULL);
}

static void vx_task_spawn_trampoline(void *param)
{
    vx_task_spawn_ctx_t *ctx = (vx_task_spawn_ctx_t *)param;
    TaskHandle_t self;

    if (ctx == NULL)
    {
        vTaskDelete(NULL);
        return;
    }

    self = xTaskGetCurrentTaskHandle();

    if (ctx->entry != NULL)
    {
        ctx->entry(ctx->args[0], ctx->args[1], ctx->args[2], ctx->args[3], ctx->args[4],
                   ctx->args[5], ctx->args[6], ctx->args[7], ctx->args[8], ctx->args[9]);
    }

    /* VxWorks task entry returns here, then we force FreeRTOS task self-delete. */
    vx_task_auto_exit(ctx, self);
}

TASK_ID taskSpawn(char *name, int priority, int options, int stackSize,
                  FUNCPTR entryPt,
                  int arg1, int arg2, int arg3, int arg4, int arg5,
                  int arg6, int arg7, int arg8, int arg9, int arg10)
{
    TaskHandle_t handle = NULL;
    BaseType_t rc;
    BaseType_t reg_rc;
    UBaseType_t task_prio;
    configSTACK_DEPTH_TYPE stack_depth;
    vx_task_spawn_ctx_t *ctx;

    if (entryPt == NULL)
    {
        return (TASK_ID)0;
    }

    ctx = (vx_task_spawn_ctx_t *)pvPortMalloc(sizeof(vx_task_spawn_ctx_t));
    if (ctx == NULL)
    {
        return (TASK_ID)0;
    }

    ctx->entry = entryPt;
    ctx->options = options;
    ctx->stack_size_bytes = stackSize;
    if (name != NULL)
    {
        strncpy(ctx->name, name, VX_TASK_NAME_LENGTH);
        ctx->name[VX_TASK_NAME_LENGTH] = '\0';
    }
    else
    {
        strcpy(ctx->name, "vxTask");
    }
    ctx->args[0] = arg1;
    ctx->args[1] = arg2;
    ctx->args[2] = arg3;
    ctx->args[3] = arg4;
    ctx->args[4] = arg5;
    ctx->args[5] = arg6;
    ctx->args[6] = arg7;
    ctx->args[7] = arg8;
    ctx->args[8] = arg9;
    ctx->args[9] = arg10;

    if (stackSize <= 0)
    {
        stack_depth = configMINIMAL_STACK_SIZE;
    }
    else
    {
        stack_depth = (configSTACK_DEPTH_TYPE)((stackSize + (int)sizeof(StackType_t) - 1) /
                                               (int)sizeof(StackType_t));
    }

    task_prio = vx_to_freertos_priority(priority);
    vTaskSuspendAll();
    rc = xTaskCreate(vx_task_spawn_trampoline,
                     ctx->name,
                     stack_depth,
                     ctx,
                     task_prio,
                     &handle);
    if (rc == pdPASS)
    {
        reg_rc = vx_registry_register(handle, ctx->name, ctx->entry, ctx->options, ctx->stack_size_bytes);
    }
    else
    {
        reg_rc = pdFALSE;
    }
    (void)xTaskResumeAll();

    if (rc != pdPASS)
    {
        vPortFree(ctx);
        return (TASK_ID)0;
    }

    if (reg_rc != pdTRUE)
    {
        vTaskDelete(handle);
        vPortFree(ctx);
        return (TASK_ID)0;
    }

    return (TASK_ID)(uintptr_t)handle;
}

TASK_ID taskNameToId(const char *name)
{
    TaskHandle_t handle;
    vx_task_record_t *record;

    taskENTER_CRITICAL();
    record = vx_registry_find_by_name(name);
    handle = (record != NULL) ? record->handle : NULL;
    taskEXIT_CRITICAL();

    if (handle == NULL)
    {
        return (TASK_ID)ERROR;
    }

    return (TASK_ID)(uintptr_t)handle;
}

STATUS taskPrioritySet(TASK_ID tid, int newPriority)
{
    TaskHandle_t handle = vx_tid_to_handle(tid);

    if (handle == NULL)
    {
        return ERROR;
    }

    vTaskPrioritySet(handle, vx_to_freertos_priority(newPriority));
    return OK;
}

STATUS taskOptionsSet(TASK_ID tid, int options, int mask)
{
    TaskHandle_t handle = vx_tid_to_handle(tid);
    vx_task_record_t *record;

    if (handle == NULL)
    {
        return ERROR;
    }

    taskENTER_CRITICAL();
    record = vx_registry_find_by_handle(handle);
    if (record != NULL)
    {
        record->options = (record->options & (~mask)) | (options & mask);
        taskEXIT_CRITICAL();
        return OK;
    }
    taskEXIT_CRITICAL();

    return ERROR;
}

STATUS taskDelete(TASK_ID tid)
{
    TaskHandle_t handle;

    if (tid == 0)
    {
        handle = xTaskGetCurrentTaskHandle();
    }
    else
    {
        handle = vx_tid_to_handle(tid);
    }

    if (handle == NULL)
    {
        return ERROR;
    }

    if (vx_registry_find_by_handle(handle) == NULL)
    {
        return ERROR;
    }

    vx_registry_unregister(handle);
    vTaskDelete(handle);
    return OK;
}

int taskLock(void)
{
    vTaskSuspendAll();
    return OK;
}

int taskUnlock(void)
{
    (void)xTaskResumeAll();
    return OK;
}

STATUS taskDelay(int ticks)
{
    if (ticks < 0)
    {
        return ERROR;
    }

    vTaskDelay((TickType_t)ticks);
    return OK;
}

STATUS taskStatusString(TASK_ID tid, char *buf)
{
    TaskHandle_t handle = vx_tid_to_handle(tid);
    vx_task_record_t *record;
    eTaskState state;

    if (buf == NULL)
    {
        return ERROR;
    }

    if (handle == NULL)
    {
        handle = xTaskGetCurrentTaskHandle();
    }

    taskENTER_CRITICAL();
    record = vx_registry_find_by_handle(handle);
    taskEXIT_CRITICAL();
    if (record == NULL)
    {
        return ERROR;
    }

    state = eTaskGetState(handle);
    switch (state)
    {
    case eRunning:
    case eReady:
        strcpy(buf, "READY");
        break;
    case eBlocked:
        strcpy(buf, "PEND");
        break;
    case eSuspended:
        strcpy(buf, "SUSPEND");
        break;
    case eDeleted:
        strcpy(buf, "DEAD");
        break;
    default:
        strcpy(buf, "UNKNOWN");
        break;
    }

    return OK;
}

STATUS taskIdVerify(int tid)
{
    TaskHandle_t handle = (TaskHandle_t)(uintptr_t)(uint32_t)tid;
    vx_task_record_t *record;

    if ((tid == ERROR) || (handle == NULL))
    {
        return ERROR;
    }

    taskENTER_CRITICAL();
    record = vx_registry_find_by_handle(handle);
    taskEXIT_CRITICAL();

    return (record != NULL) ? OK : ERROR;
}

TASK_ID taskIdSelf(void)
{
    return (TASK_ID)(uintptr_t)xTaskGetCurrentTaskHandle();
}

STATUS taskMsSleep(int ms)
{
    if (ms < 0)
    {
        return ERROR;
    }

    if (ms == 0)
    {
        taskYIELD();
    }
    else
    {
        vTaskDelay(pdMS_TO_TICKS(ms));
    }

    return OK;
}

const char *taskName(TASK_ID tid)
{
    TaskHandle_t handle = vx_tid_to_handle(tid);
    vx_task_record_t *record;

    if (handle == NULL)
    {
        handle = xTaskGetCurrentTaskHandle();
    }

    taskENTER_CRITICAL();
    record = vx_registry_find_by_handle(handle);
    taskEXIT_CRITICAL();
    if (record == NULL)
    {
        return NULL;
    }

    return pcTaskGetName(handle);
}

STATUS taskInfoGet(TASK_ID tid, TASK_DESC *pTaskDesc)
{
    TaskHandle_t handle = vx_tid_to_handle(tid);
    vx_task_record_t *record;
    eTaskState state;
    UBaseType_t prio;
    UBaseType_t hwm;

    if (pTaskDesc == NULL)
    {
        return ERROR;
    }

    if (handle == NULL)
    {
        handle = xTaskGetCurrentTaskHandle();
    }

    taskENTER_CRITICAL();
    record = vx_registry_find_by_handle(handle);
    taskEXIT_CRITICAL();
    if (record == NULL)
    {
        return ERROR;
    }

    state = eTaskGetState(handle);
    prio = uxTaskPriorityGet(handle);
    hwm = uxTaskGetStackHighWaterMark(handle);

    memset(pTaskDesc, 0, sizeof(*pTaskDesc));
    pTaskDesc->td_id = (int)(uintptr_t)handle;
    pTaskDesc->td_priority = (int)prio;
    pTaskDesc->td_sp = NULL;
    pTaskDesc->td_pStackBase = NULL;
    pTaskDesc->td_pStackEnd = NULL;
    pTaskDesc->td_stackSize = record->stack_size_bytes;
    pTaskDesc->td_stackMargin = (int)(hwm * sizeof(StackType_t));
    pTaskDesc->td_delay = 0;

    switch (state)
    {
    case eRunning:
    case eReady:
        pTaskDesc->td_status = VX_READY;
        break;
    case eBlocked:
        pTaskDesc->td_status = VX_PEND;
        break;
    case eSuspended:
        pTaskDesc->td_status = VX_SUSPEND;
        break;
    case eDeleted:
        pTaskDesc->td_status = VX_DEAD;
        break;
    default:
        pTaskDesc->td_status = VX_DELAY;
        break;
    }

    pTaskDesc->td_entry = record->entry;
    pTaskDesc->td_options = record->options;
    strncpy(pTaskDesc->td_name, record->name, VX_TASK_NAME_LENGTH);
    pTaskDesc->td_name[VX_TASK_NAME_LENGTH] = '\0';

    return OK;
}
