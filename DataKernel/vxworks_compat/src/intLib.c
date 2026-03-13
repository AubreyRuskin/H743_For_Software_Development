#define _POSIX_C_SOURCE 200809L

#include "intLib.h"
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <stdio_compat.h>
#include <string.h>

// 定义最大中断向量数
#define MAX_INTERRUPTS 256

// 定义中断处理程序结构体
typedef struct {
    VOIDFUNCPTR routine;  // 中断处理函数
    int arg;              // 中断处理函数的参数
    timer_t timer_id;     // POSIX定时器ID
    int initialized;      // 是否已初始化
    int enabled;          // 是否启用
    int interval_us;      // 保存间隔时间（微秒）
} InterruptVector;

// 全局中断向量数组
static InterruptVector interruptVectors[MAX_INTERRUPTS];
static int global_interrupt_lock = 0;

// 定时器信号处理函数 - 每个向量有独立的处理函数
void timerHandler(int sig, siginfo_t *si, void *uc) {
    int vector = (int)(long)si->si_value.sival_ptr;
    
    // 检查全局中断锁和向量使能状态
    if (global_interrupt_lock || !interruptVectors[vector].enabled) {
        return;
    }
    
    // 调用对应向量的中断处理函数
    if (interruptVectors[vector].routine != NULL) {
        interruptVectors[vector].routine(interruptVectors[vector].arg);
    }
}

// 初始化指定向量的定时器 - 支持微秒精度，但不启动
STATUS initTimer(int vector, int interval_us) {
    if (vector < 0 || vector >= MAX_INTERRUPTS) {
        return ERROR;
    }
    
    if (interruptVectors[vector].initialized) {
        return OK; // 已经初始化过了
    }
    
    struct sigaction sa;
    struct sigevent sev;
    
    // 设置信号处理函数
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timerHandler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGRTMIN + vector, &sa, NULL);
    
    // 创建定时器
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN + vector;
    sev.sigev_value.sival_ptr = (void*)(long)vector;
    
    if (timer_create(CLOCK_REALTIME, &sev, &interruptVectors[vector].timer_id) == -1) {
        return ERROR;
    }
    
    // 保存间隔时间，但不启动定时器
    interruptVectors[vector].interval_us = interval_us;
    interruptVectors[vector].initialized = 1;
    interruptVectors[vector].enabled = 0; // 默认禁用
    
    return OK;
}

// 为了向后兼容，提供毫秒版本的函数
STATUS initTimerMs(int vector, int interval_ms) {
    return initTimer(vector, interval_ms * 1000);
}

// 启用指定向量的定时器
STATUS intEnable(int vector) {
    if (vector < 0 || vector >= MAX_INTERRUPTS) {
        return ERROR;
    }
    
    if (!interruptVectors[vector].initialized) {
        return ERROR; // 必须先初始化
    }
    
    if (interruptVectors[vector].enabled) {
        return OK; // 已经启用了
    }
    
    struct itimerspec its;
    
    // 设置定时器时间 - 微秒精度
    its.it_value.tv_sec = interruptVectors[vector].interval_us / 1000000;  // 秒部分
    its.it_value.tv_nsec = (interruptVectors[vector].interval_us % 1000000) * 1000;  // 纳秒部分
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    
    if (timer_settime(interruptVectors[vector].timer_id, 0, &its, NULL) == -1) {
        return ERROR;
    }
    
    interruptVectors[vector].enabled = 1;
    return OK;
}

// 禁用指定向量的定时器
STATUS intDisable(int vector) {
    if (vector < 0 || vector >= MAX_INTERRUPTS) {
        return ERROR;
    }
    
    if (!interruptVectors[vector].initialized) {
        return ERROR; // 必须先初始化
    }
    
    if (!interruptVectors[vector].enabled) {
        return OK; // 已经禁用了
    }
    
    struct itimerspec its = {0}; // 全部设为0停止定时器
    
    if (timer_settime(interruptVectors[vector].timer_id, 0, &its, NULL) == -1) {
        return ERROR;
    }
    
    interruptVectors[vector].enabled = 0;
    return OK;
}

STATUS intConnect(int vector, VOIDFUNCPTR routine, int arg1) {
    if (vector < 0 || vector >= MAX_INTERRUPTS) {
        return ERROR; // 向量号超出范围
    }

    // 注册中断处理程序
    interruptVectors[vector].routine = routine;
    interruptVectors[vector].arg = arg1;
    // interruptVectors[vector].initialized = 0;
    interruptVectors[vector].enabled = 0;
    // interruptVectors[vector].interval_us = 0;

    return OK;
}

int intLock()
{
    // 全局中断锁定
    global_interrupt_lock = 1;
    return 0;
}

int intUnlock(int k){
    // 全局中断解锁
    global_interrupt_lock = 0;
    return 0;
}