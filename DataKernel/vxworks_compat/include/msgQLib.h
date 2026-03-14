#ifndef MSG_QLIB_H
#define MSG_QLIB_H

// #include <mqueue.h>
#include <vxworks_type.h>


#define MAX_Q_NAME_LEN 32


#include "time_compat.h"
#include "sys_types_compat.h"


typedef struct VxMsgQ {
    // mqd_t  mqd;                  // 用于发送/接收的描述符
    char   name[MAX_Q_NAME_LEN]; // 用于将来删除的队列名
} VxMsgQ;
typedef VxMsgQ* MSG_Q_ID;

extern int MSG_Q_ID_NAME; // 用于生成唯一的消息队列名称

#define MSG_Q_ID_NULL ((MSG_Q_ID)NULL)

// static int g_msgq_counter ;




// 消息队列选项 VxWorks 支持两种消息队列模式：先进先出（FIFO）和优先级（PRIORITY）需要在消息队列创建的同时指定
// posix则是默认优先级模式，如果相同优先级才是先进先出
#define MSG_Q_FIFO           0x00
#define MSG_Q_PRIORITY       0x01

// 消息优先级
#define MSG_PRI_NORMAL       0
#define MSG_PRI_URGENT       1 // 对应 POSIX 中较高的优先级

// --- 公共 API 函数原型 ---

MSG_Q_ID msgQCreate(int maxMsgs, int maxMsgLength, int options);

STATUS msgQDelete(MSG_Q_ID msgQId);

STATUS msgQSend(MSG_Q_ID msgQId, char *buffer, size_t nBytes, int timeout, int priority);

ssize_t msgQReceive(MSG_Q_ID msgQId, char *buffer, size_t maxNBytes, int timeout);


#endif