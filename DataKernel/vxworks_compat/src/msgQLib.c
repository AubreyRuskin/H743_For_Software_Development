#include "msgQLib.h"
#include <stdio_compat.h>
#include <unistd.h>
#include <semLib.h>
#include <errno.h>

int MSG_Q_ID_NAME=0; // 用于生成唯一的消息队列名称



static int g_msgq_counter =0;
static pthread_mutex_t g_msgq_counter_mutex = PTHREAD_MUTEX_INITIALIZER;






MSG_Q_ID msgQCreate(int maxMsgs, int maxMsgLength, int options)
{
    // 建议使用更清晰的变量名，避免与 mqd_t 类型混淆
    MSG_Q_ID msgQId = (MSG_Q_ID)malloc(sizeof(VxMsgQ));
    if (msgQId == NULL) {
        perror("malloc for VxMsgQ failed");
        return MSG_Q_ID_NULL;
    }
    
    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_flags = 0;
    attr.mq_maxmsg = maxMsgs;
    attr.mq_msgsize = maxMsgLength;
    // attr.mq_curmsgs = 0;

    // --- 线程安全地生成一个跨进程更唯一的名称 ---
    pthread_mutex_lock(&g_msgq_counter_mutex);
    snprintf(msgQId->name, MAX_Q_NAME_LEN, "/vx_msgq_%ld_%d", (long)getpid(), g_msgq_counter++);
    pthread_mutex_unlock(&g_msgq_counter_mutex);

    // --- 使用 O_EXCL 保证创建的原子性 ---
    int oflag = O_CREAT | O_RDWR | O_EXCL;
    
    mqd_t mqid = mq_open(msgQId->name, oflag, 0644, &attr);
    if (mqid == (mqd_t)-1) {
        perror("mq_open failed");
        // --- 修复内存泄漏 ---
        free(msgQId);
        return MSG_Q_ID_NULL;
    }

    msgQId->mqd = mqid;

    return msgQId;
}

STATUS msgQSend(MSG_Q_ID msgQId, char *buffer, size_t nBytes, int timeout, int priority) {
    if (msgQId == NULL || buffer == NULL) {
        return ERROR;
    }

    assert(nBytes <= 8192); // POSIX 消息队列的最大消息大小通常是 8192 字节

    struct timespec ts;
    if (timeout != WAIT_FOREVER) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout / 1000;
        ts.tv_nsec += (timeout % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }
    }

    int result;
    if (timeout == WAIT_FOREVER) {
        result = mq_send(msgQId->mqd, buffer, nBytes, priority);
    } else {
        result = mq_timedsend(msgQId->mqd, buffer, nBytes, priority, &ts);
    }

    if (result == -1) {
        perror("msgQSend failed");
        return ERROR;
    }

    return OK;
}

ssize_t msgQReceive(MSG_Q_ID msgQId, char *buffer, size_t maxNBytes, int timeout) {
    if (msgQId == NULL || buffer == NULL) {
        return ERROR;
    }

    // 获取队列属性，确定需要的缓冲区大小
    struct mq_attr attr;
    if (mq_getattr(msgQId->mqd, &attr) != 0) {
        perror("mq_getattr failed");
        return ERROR;
    }

    // 创建足够大的内部缓冲区
    char *internalBuffer = (char*)malloc(attr.mq_msgsize);
    if (internalBuffer == NULL) {
        perror("malloc for internal buffer failed");
        return ERROR;
    }

    struct timespec ts;
    if (timeout != WAIT_FOREVER) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout / 1000;
        ts.tv_nsec += (timeout % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }
    }

    ssize_t bytesRead;
    
    // 添加重试机制处理信号中断，使用内部大缓冲区接收
    do {
        if (timeout == WAIT_FOREVER) {
            bytesRead = mq_receive(msgQId->mqd, internalBuffer, attr.mq_msgsize, NULL);
        } else {
            bytesRead = mq_timedreceive(msgQId->mqd, internalBuffer, attr.mq_msgsize, NULL, &ts);
        }
    } while (bytesRead == -1 && errno == EINTR);

    if (bytesRead == -1) {
        // 释放内部缓冲区
        free(internalBuffer);
        
        // 调试信息：记录具体的错误原因
        if (errno == EINTR) {
            printf("msgQReceive interrupted by signal (EINTR)\n");
        } else if (errno == EAGAIN || errno == ETIMEDOUT) {
            // 超时或非阻塞调用没有消息，正常情况
        } else {
            perror("msgQReceive failed");
            printf("errno = %d\n", errno);
        }
        return ERROR;
    }

    // 成功接收消息，截断拷贝到用户缓冲区
    size_t copySize = (bytesRead < maxNBytes) ? bytesRead : maxNBytes;
    memcpy(buffer, internalBuffer, copySize);
    
    // 释放内部缓冲区
    free(internalBuffer);

    // 返回实际拷贝的字节数（可能小于实际接收的字节数）
    return copySize;
}