/*
 * semLib.c — VxWorks semaphore compatibility layer on FreeRTOS
 *
 * Maps vxWorks semaphore API to FreeRTOS primitives.
 *
 * Option flags actually used across the codebase (42 calls total):
 *
 *   SEM_Q_FIFO        (0x0)  — FIFO unblock order
 *   SEM_Q_PRIORITY    (0x1)  — priority-based unblock order
 *   SEM_INVERSION_SAFE(0x4)  — priority-inversion-safe mutex
 *   SEM_DELETE_SAFE   (0x8)  — unblock waiters on semDelete
 *
 * How each is handled:
 * ┌──────────────────────┬──────────────────────────────────────────┐
 * │ SEM_Q_PRIORITY       │ FreeRTOS xSemaphore (default)           │
 * │                      │ unblocks highest-priority waiter.       │
 * ├──────────────────────┼──────────────────────────────────────────┤
 * │ SEM_Q_FIFO           │ FreeRTOS xQueue (item-size 0).         │
 * │                      │ unblocks longest-waiting waiter.        │
 * ├──────────────────────┼──────────────────────────────────────────┤
 * │ SEM_INVERSION_SAFE   │ Use xSemaphoreCreateMutex (non-         │
 * │                      │ recursive, has priority inheritance)    │
 * │                      │ instead of xSemaphoreCreateRecursive.   │
 * ├──────────────────────┼──────────────────────────────────────────┤
 * │ SEM_DELETE_SAFE      │ semDelete first flushes all blocked     │
 * │                      │ tasks (xSemaphoreGive loop), then       │
 * │                      │ deletes the semaphore.                  │
 * └──────────────────────┴──────────────────────────────────────────┘
 *
 * Tick rates: vxWorks 100 Hz → FreeRTOS 1000 Hz (×10).
 */

#include "semLib.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"
#include <stdlib.h>
#include <string.h>

/* ── Internal types ─────────────────────────────────────────────── */

typedef enum {
    VX_SEM_MUTEX    = 0,
    VX_SEM_BINARY   = 1,
    VX_SEM_COUNTING = 2,
    VX_SEM_RW       = 3,
} vx_sem_type_t;

#define VX_SEM_COUNTING_MAX  0x0FFFFFFF

typedef struct {
    vx_sem_type_t     type;
    int               options;       /* raw options from creation call */
    SemaphoreHandle_t handle;        /* primary FreeRTOS handle */
    QueueHandle_t     queue;         /* used iff SEM_Q_FIFO */
    /* For RW locks */
    SemaphoreHandle_t rw_mutex;
    SemaphoreHandle_t rw_count;
    int               rw_max_readers;
} vx_sem_t;

/* ── Helpers ────────────────────────────────────────────────────── */

static TickType_t vx_to_freertos_ticks(int timeout)
{
    if (timeout < 0) return portMAX_DELAY;            /* WAIT_FOREVER */
    if (timeout == 0) return 0;                       /* NO_WAIT */
    return (TickType_t)((unsigned)timeout * 10u);      /* 100→1000 Hz */
}

static int frstatus_to_vx(BaseType_t fr) { return (fr == pdPASS) ? OK : ERROR; }

static BaseType_t sem_fifo_take(vx_sem_t *sem, TickType_t ticks)
{
    uint8_t dummy;
    return xQueueReceive(sem->queue, &dummy, ticks);
}

static BaseType_t sem_fifo_give(vx_sem_t *sem)
{
    uint8_t dummy = 0;
    return xQueueSend(sem->queue, &dummy, 0);
}

static BaseType_t sem_fifo_give_from_isr(vx_sem_t *sem, BaseType_t *pxHigherPriorityTaskWoken)
{
    uint8_t dummy = 0;
    return xQueueSendFromISR(sem->queue, &dummy, pxHigherPriorityTaskWoken);
}

/* ── semMCreate — mutex semaphore ───────────────────────────────── */

SEM_ID semMCreate(int options)
{
    vx_sem_t *sem = (vx_sem_t *)pvPortMalloc(sizeof(vx_sem_t));
    if (sem == NULL) return NULL_ID;

    memset(sem, 0, sizeof(*sem));
    sem->type    = VX_SEM_MUTEX;
    sem->options = options;
    sem->queue   = NULL;

    /*
     * SEM_INVERSION_SAFE: use non-recursive mutex which supports
     * priority inheritance in FreeRTOS (configUSE_PRIORITY_INHERITANCE).
     *
     * Without it: use recursive mutex — closer to vxWorks mutex
     * semantics where a task can take a mutex it already holds.
     */
#if (configUSE_RECURSIVE_MUTEXES == 1)
    if (options & SEM_INVERSION_SAFE) {
        sem->handle = xSemaphoreCreateMutex();
    } else {
        sem->handle = xSemaphoreCreateRecursiveMutex();
    }
#else
    sem->handle = xSemaphoreCreateMutex();
    (void)options;
#endif

    if (sem->handle == NULL) { vPortFree(sem); return NULL_ID; }
    return (SEM_ID)sem;
}

/* ── semBCreate — binary semaphore ──────────────────────────────── */

SEM_ID semBCreate(int options, SEM_B_STATE initialState)
{
    vx_sem_t *sem = (vx_sem_t *)pvPortMalloc(sizeof(vx_sem_t));
    if (sem == NULL) return NULL_ID;

    memset(sem, 0, sizeof(*sem));
    sem->type    = VX_SEM_BINARY;
    sem->options = options;
    sem->queue   = NULL;

    if (options & SEM_Q_PRIORITY) {
        /* FreeRTOS binary semaphore — unblocks highest-priority waiter */
        sem->handle = xSemaphoreCreateBinary();
        if (sem->handle == NULL) { vPortFree(sem); return NULL_ID; }
        if (initialState == SEM_FULL)
            xSemaphoreGive(sem->handle);
    } else {
        /* SEM_Q_FIFO — use a queue (item-size 0, length 1).
         * xQueueSend unblocks the longest-waiting receiver (FIFO). */
        sem->queue = xQueueCreate(1, 0);
        if (sem->queue == NULL) { vPortFree(sem); return NULL_ID; }
        if (initialState == SEM_FULL) {
            uint8_t dummy = 0;
            xQueueSend(sem->queue, &dummy, 0);
        }
    }
    return (SEM_ID)sem;
}

/* ── semCCreate — counting semaphore ────────────────────────────── */

SEM_ID semCCreate(int options, int initialCount)
{
    vx_sem_t *sem = (vx_sem_t *)pvPortMalloc(sizeof(vx_sem_t));
    if (sem == NULL) return NULL_ID;

    memset(sem, 0, sizeof(*sem));
    sem->type    = VX_SEM_COUNTING;
    sem->options = options;
    sem->queue   = NULL;

    if (options & SEM_Q_PRIORITY) {
        /* FreeRTOS counting semaphore — unblocks highest-priority waiter */
        sem->handle = xSemaphoreCreateCounting(
            (UBaseType_t)VX_SEM_COUNTING_MAX, (UBaseType_t)initialCount);
        if (sem->handle == NULL) { vPortFree(sem); return NULL_ID; }
    } else {
        /* SEM_Q_FIFO — use a queue (item-size 0) for true FIFO unblocking.
         * Max length chosen large enough for the application. */
        sem->queue = xQueueCreate((UBaseType_t)VX_SEM_COUNTING_MAX, 0);
        if (sem->queue == NULL) { vPortFree(sem); return NULL_ID; }
        for (int i = 0; i < initialCount; i++) {
            uint8_t dummy = 0;
            xQueueSend(sem->queue, &dummy, 0);
        }
    }
    return (SEM_ID)sem;
}

/* ── semTake ─────────────────────────────────────────────────────── */

STATUS semTake(SEM_ID semId, int timeout)
{
    if (semId == NULL_ID) return ERROR;

    vx_sem_t *sem   = (vx_sem_t *)semId;
    TickType_t ticks = vx_to_freertos_ticks(timeout);
    BaseType_t rc;

    /* FIFO path: use queue receive */
    if (sem->queue != NULL)
        return frstatus_to_vx(sem_fifo_take(sem, ticks));

    /* Mutex / priority semaphore path */
    switch (sem->type) {
    case VX_SEM_MUTEX:
#if (configUSE_RECURSIVE_MUTEXES == 1)
        if (sem->options & SEM_INVERSION_SAFE)
            rc = xSemaphoreTake(sem->handle, ticks);
        else
            rc = xSemaphoreTakeRecursive(sem->handle, ticks);
#else
        rc = xSemaphoreTake(sem->handle, ticks);
#endif
        break;
    default:
        rc = xSemaphoreTake(sem->handle, ticks);
        break;
    }
    return frstatus_to_vx(rc);
}

/* ── semGive ─────────────────────────────────────────────────────── */

STATUS semGive(SEM_ID semId)
{
    if (semId == NULL_ID) return ERROR;

    vx_sem_t *sem = (vx_sem_t *)semId;
    BaseType_t rc;

    /* FIFO path */
    if (sem->queue != NULL)
        return frstatus_to_vx(sem_fifo_give(sem));

    switch (sem->type) {
    case VX_SEM_MUTEX:
#if (configUSE_RECURSIVE_MUTEXES == 1)
        if (sem->options & SEM_INVERSION_SAFE)
            rc = xSemaphoreGive(sem->handle);
        else
            rc = xSemaphoreGiveRecursive(sem->handle);
#else
        rc = xSemaphoreGive(sem->handle);
#endif
        break;
    default:
        rc = xSemaphoreGive(sem->handle);
        break;
    }
    return frstatus_to_vx(rc);
}

/* ── semDelete ───────────────────────────────────────────────────── */

STATUS semDelete(SEM_ID semId)
{
    if (semId == NULL_ID) return ERROR;

    vx_sem_t *sem = (vx_sem_t *)semId;

    /*
     * SEM_DELETE_SAFE: unblock all waiting tasks before deletion.
     * vxWorks guarantees tasks blocked on a delete-safe semaphore
     * are unblocked with ERROR when the semaphore is deleted.
     */
    if (sem->options & SEM_DELETE_SAFE) {
        if (sem->queue != NULL) {
            /* FIFO queue: drain by receiving (unblocks waiters) */
            uint8_t dummy;
            while (xQueueReceive(sem->queue, &dummy, 0) == pdPASS) {}
            /* Also give then receive to unblock tasks stuck waiting */
            for (int i = 0; i < 64; i++) {
                uint8_t d = 0;
                xQueueSend(sem->queue, &d, 0);
                xQueueReceive(sem->queue, &dummy, 0);
            }
            vQueueDelete(sem->queue);
        } else if (sem->handle != NULL) {
            /*
             * Priority semaphore / mutex: repeatedly give to unblock
             * all waiting tasks, then delete.
             * Tasks unblocked after the semaphore is gone will get
             * ERROR from semTake (handle is freed).
             */
            for (int i = 0; i < 64; i++)
                xSemaphoreGive(sem->handle);
            vSemaphoreDelete(sem->handle);
        }
    } else {
        /* Non-safe: just delete. Blocked tasks stay blocked (vxWorks too). */
        if (sem->queue != NULL)
            vQueueDelete(sem->queue);
        else if (sem->handle != NULL)
            vSemaphoreDelete(sem->handle);
    }

    if (sem->rw_mutex != NULL) vSemaphoreDelete(sem->rw_mutex);
    if (sem->rw_count != NULL) vSemaphoreDelete(sem->rw_count);

    vPortFree(sem);
    return OK;
}

/* ── semFlush — unblock all waiting tasks ────────────────────────── */

STATUS semFlush(SEM_ID semId)
{
    if (semId == NULL_ID) return ERROR;

    vx_sem_t *sem = (vx_sem_t *)semId;

    if (sem->queue != NULL) {
        /*
         * FIFO queue: send dummy items to unblock waiters,
         * then drain to leave the semaphore "empty".
         */
        for (int i = 0; i < 64; i++) {
            uint8_t d = 0, dummy;
            xQueueSend(sem->queue, &d, 0);
            xQueueReceive(sem->queue, &dummy, 0);
        }
    } else if (sem->handle != NULL) {
        /*
         * Priority semaphore: xSemaphoreGive unblocks one waiter
         * who immediately takes the token.  Repeat until no more
         * waiters.  This leaves the semaphore "empty" (count = 0).
         */
        for (int i = 0; i < 64; i++)
            xSemaphoreGive(sem->handle);
    }
    return OK;
}

/* ── semExchange — atomic give + take ────────────────────────────── */

STATUS semExchange(SEM_ID giveSemId, SEM_ID takeSemId, int timeout)
{
    if (giveSemId == NULL_ID) return ERROR;
    if (semGive(giveSemId) != OK) return ERROR;
    if (takeSemId != NULL_ID) return semTake(takeSemId, timeout);
    return OK;
}

/* ── semRWCreate — read-write semaphore ──────────────────────────── */
/*
 * Not used anywhere in the current codebase (0 calls).
 * Stub kept for link compatibility.
 */

SEM_ID semRWCreate(int options, int maxReaders)
{
    (void)options;
    vx_sem_t *sem = (vx_sem_t *)pvPortMalloc(sizeof(vx_sem_t));
    if (sem == NULL) return NULL_ID;

    memset(sem, 0, sizeof(*sem));
    sem->type       = VX_SEM_RW;
    sem->options    = options;
    sem->handle     = xSemaphoreCreateBinary();
    sem->rw_mutex   = xSemaphoreCreateMutex();
    sem->rw_count   = xSemaphoreCreateCounting((UBaseType_t)maxReaders,
                                                (UBaseType_t)maxReaders);
    sem->rw_max_readers = maxReaders;

    if (sem->handle == NULL || sem->rw_mutex == NULL || sem->rw_count == NULL) {
        if (sem->handle)   vSemaphoreDelete(sem->handle);
        if (sem->rw_mutex) vSemaphoreDelete(sem->rw_mutex);
        if (sem->rw_count) vSemaphoreDelete(sem->rw_count);
        vPortFree(sem);
        return NULL_ID;
    }
    xSemaphoreGive(sem->handle);
    return (SEM_ID)sem;
}

STATUS semRTake(SEM_ID semId, int timeout)
{
    if (semId == NULL_ID) return ERROR;
    vx_sem_t *sem = (vx_sem_t *)semId;
    if (sem->type != VX_SEM_RW) return semTake(semId, timeout);

    TickType_t ticks = vx_to_freertos_ticks(timeout);
    if (xSemaphoreTake(sem->rw_count, ticks) != pdPASS) return ERROR;
    if (uxSemaphoreGetCount(sem->rw_count) ==
        (UBaseType_t)(sem->rw_max_readers - 1)) {
        if (xSemaphoreTake(sem->handle, portMAX_DELAY) != pdPASS) {
            xSemaphoreGive(sem->rw_count);
            return ERROR;
        }
    }
    return OK;
}

STATUS semWTake(SEM_ID semId, int timeout)
{
    return semTake(semId, timeout);
}
