/*
 * msgQLib.c — VxWorks message queue compatibility layer on FreeRTOS
 *
 * Maps vxWorks msgQ API to FreeRTOS queues.
 *
 * vxWorks semantics:
 *   msgQCreate(maxMsgs, maxMsgLength, options)
 *     options: MSG_Q_FIFO   (0x00) — strict FIFO order
 *              MSG_Q_PRIORITY (0x01) — priority-based order
 *   msgQSend(msgQId, buf, nBytes, timeout, priority)
 *     priority: MSG_PRI_NORMAL (0) — send to back
 *               MSG_PRI_URGENT (1) — send to front
 *   msgQReceive(msgQId, buf, maxNBytes, timeout) → bytes received
 *
 * FreeRTOS mapping:
 *   Each queue item is maxMsgLength + sizeof(size_t) bytes.
 *   The leading size_t stores the actual message length so the
 *   receiver knows how many bytes were sent.
 *
 *   MSG_Q_FIFO:   both normal and urgent → xQueueSendToBack
 *   MSG_Q_PRIORITY:
 *       MSG_PRI_URGENT → xQueueSendToFront
 *       MSG_PRI_NORMAL → xQueueSendToBack
 *
 * Tick rates: vxWorks 100 Hz → FreeRTOS 1000 Hz (×10).
 */

#include "msgQLib.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <string.h>
#include <stdlib.h>

/* ── Internal wrapper ───────────────────────────────────────────── */

#define MSGQ_SIZE_OVERHEAD  sizeof(size_t)

typedef struct {
    QueueHandle_t handle;
    int           maxMsgLength;
    int           options;
} vx_msgq_t;

/* ── Helpers ────────────────────────────────────────────────────── */

static inline vx_msgq_t *msgq_from_id(MSG_Q_ID id)
{
    return (vx_msgq_t *)id;
}

static TickType_t vx_to_freertos_ticks(int timeout)
{
    if (timeout < 0)  return portMAX_DELAY;
    if (timeout == 0) return 0;
    return (TickType_t)((unsigned)timeout * 10u);
}

/* ── msgQCreate ──────────────────────────────────────────────────── */

MSG_Q_ID msgQCreate(int maxMsgs, int maxMsgLength, int options)
{
    vx_msgq_t *mq;
    size_t     item_size;

    if (maxMsgs <= 0 || maxMsgLength <= 0)
        return MSG_Q_ID_NULL;

    mq = (vx_msgq_t *)pvPortMalloc(sizeof(vx_msgq_t));
    if (mq == NULL)
        return MSG_Q_ID_NULL;

    item_size = (size_t)maxMsgLength + MSGQ_SIZE_OVERHEAD;

    mq->handle       = xQueueCreate((UBaseType_t)maxMsgs, (UBaseType_t)item_size);
    mq->maxMsgLength = maxMsgLength;
    mq->options      = options;

    if (mq->handle == NULL) {
        vPortFree(mq);
        return MSG_Q_ID_NULL;
    }

    return (MSG_Q_ID)mq;
}

/* ── msgQDelete ──────────────────────────────────────────────────── */

STATUS msgQDelete(MSG_Q_ID msgQId)
{
    vx_msgq_t *mq;

    if (msgQId == MSG_Q_ID_NULL)
        return ERROR;

    mq = msgq_from_id(msgQId);
    vQueueDelete(mq->handle);
    vPortFree(mq);
    return OK;
}

/* ── msgQSend ────────────────────────────────────────────────────── */

STATUS msgQSend(MSG_Q_ID msgQId, char *buffer, size_t nBytes,
                int timeout, int priority)
{
    vx_msgq_t    *mq;
    TickType_t    ticks;
    BaseType_t    rc;
    size_t        send_len;

    if (msgQId == MSG_Q_ID_NULL || buffer == NULL || nBytes == 0)
        return ERROR;

    mq = msgq_from_id(msgQId);
    if (nBytes > (size_t)mq->maxMsgLength)
        nBytes = (size_t)mq->maxMsgLength;

    ticks = vx_to_freertos_ticks(timeout);

    /*
     * Prepare queue item: [size_t msg_len][data...]
     * Work in a stack buffer capped to the queue item size.
     */
    send_len = nBytes;
    {
        size_t item_size = (size_t)mq->maxMsgLength + MSGQ_SIZE_OVERHEAD;
        uint8_t *item = (uint8_t *)pvPortMalloc(item_size);
        if (item == NULL)
            return ERROR;

        memcpy(item, &send_len, MSGQ_SIZE_OVERHEAD);
        memcpy(item + MSGQ_SIZE_OVERHEAD, buffer, nBytes);

        /* Determine send direction */
        if ((mq->options & MSG_Q_PRIORITY) && (priority == MSG_PRI_URGENT))
            rc = xQueueSendToFront(mq->handle, item, ticks);
        else
            rc = xQueueSendToBack(mq->handle, item, ticks);

        vPortFree(item);
    }

    return (rc == pdPASS) ? OK : ERROR;
}

/* ── msgQReceive ─────────────────────────────────────────────────── */

ssize_t msgQReceive(MSG_Q_ID msgQId, char *buffer, size_t maxNBytes,
                    int timeout)
{
    vx_msgq_t    *mq;
    TickType_t    ticks;
    size_t        msg_len;
    size_t        copy_len;
    size_t        item_size;
    uint8_t      *item;

    if (msgQId == MSG_Q_ID_NULL || buffer == NULL || maxNBytes == 0)
        return ERROR;

    mq  = msgq_from_id(msgQId);
    ticks = vx_to_freertos_ticks(timeout);
    item_size = (size_t)mq->maxMsgLength + MSGQ_SIZE_OVERHEAD;

    item = (uint8_t *)pvPortMalloc(item_size);
    if (item == NULL)
        return ERROR;

    if (xQueueReceive(mq->handle, item, ticks) != pdPASS) {
        vPortFree(item);
        return ERROR;
    }

    /* Extract the actual message length */
    memcpy(&msg_len, item, MSGQ_SIZE_OVERHEAD);
    copy_len = (msg_len < maxNBytes) ? msg_len : maxNBytes;
    memcpy(buffer, item + MSGQ_SIZE_OVERHEAD, copy_len);

    vPortFree(item);
    return (ssize_t)copy_len;
}
