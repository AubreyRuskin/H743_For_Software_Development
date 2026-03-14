#include "msgQLib.h"
#include <stdio_compat.h>
#include <stdlib_compat.h>
#include <string_compat.h>
#include "vxworks_type.h"

int MSG_Q_ID_NAME = 0;

MSG_Q_ID msgQCreate(int maxMsgs, int maxMsgLength, int options)
{
    /* TODO: 用 FreeRTOS xQueueCreate 实现 */
    (void)maxMsgs; (void)maxMsgLength; (void)options;
    return MSG_Q_ID_NULL;
}

STATUS msgQDelete(MSG_Q_ID msgQId)
{
    /* TODO: 用 FreeRTOS vQueueDelete 实现 */
    (void)msgQId;
    return ERROR;
}

STATUS msgQSend(MSG_Q_ID msgQId, char *buffer, size_t nBytes, int timeout, int priority)
{
    /* TODO: 用 FreeRTOS xQueueSend 实现 */
    (void)msgQId; (void)buffer; (void)nBytes; (void)timeout; (void)priority;
    return ERROR;
}

ssize_t msgQReceive(MSG_Q_ID msgQId, char *buffer, size_t maxNBytes, int timeout)
{
    /* TODO: 用 FreeRTOS xQueueReceive 实现 */
    (void)msgQId; (void)buffer; (void)maxNBytes; (void)timeout;
    return ERROR;
}
