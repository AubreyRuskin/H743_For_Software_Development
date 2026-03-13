#ifndef TASK_LIB_H
#define TASK_LIB_H
#include <vxWorks.h>
#include "vxworks_type.h"


// typedef int    TASK_ID;
typedef uint64_t TASK_ID;


TASK_ID taskSpawn
    (
    char    *name,         /* 任务名，一个字符串 */
    int     priority,      /* 任务优先级 (0-255) */
    int     options,       /* 任务选项 (位掩码) */
    int     stackSize,     /* 任务栈大小 (字节) */
    FUNCPTR entryPt,       /* 任务入口函数指针 */
    int     arg1,          /* 传递给入口函数的第1个参数 */
    int     arg2,          /* 传递给入口函数的第2个参数 */
    int     arg3,          /* 传递给入口函数的第3个参数 */
    int     arg4,          /* 传递给入口函数的第4个参数 */
    int     arg5,          /* 传递给入口函数的第5个参数 */
    int     arg6,          /* 传递给入口函数的第6个参数 */
    int     arg7,          /* 传递给入口函数的第7个参数 */
    int     arg8,          /* 传递给入口函数的第8个参数 */
    int     arg9,          /* 传递给入口函数的第9个参数 */
    int     arg10          /* 传递给入口函数的第10个参数 */
    );

typedef enum {
    VX_READY,      // 就绪
    VX_PEND,       // 阻塞/挂起
    VX_DELAY,      // 延时
    VX_SUSPEND,    // 被挂起 (我们的实现可以与 PEND 合并)
    VX_DEAD        // 已死亡/无效
} VX_TASK_STATUS;

typedef struct events_desc {
    UINT32 wanted;      /* 0x00: events wanted          */
    UINT32 received;        /* 0x04: all events received        */
    UINT8  options;     /* 0x08: user options           */
} EVENTS_DESC;

typedef pid_t   RTP_ID;

#define LW_CFG_OBJECT_NAME_SIZE                 32   



#define VX_TASK_NAME_LENGTH LW_CFG_OBJECT_NAME_SIZE  

typedef struct {                        /* TASK_DESC - information structure */
    int         td_id;                  /* task id */
    int         td_priority;            /* task priority */
    int         td_status;              /* task status */
    int         td_options;             /* task option bits (see below) */
    FUNCPTR     td_entry;               /* original entry point of task */
    char *      td_sp;                  /* saved stack pointer */
    char *      td_pStackBase;          /* the bottom of the stack */
    char *      td_pStackEnd;           /* the actual end of the stack */
    int         td_stackSize;           /* size of stack in bytes */
    int         td_stackCurrent;        /* current stack usage in bytes */
    int         td_stackHigh;           /* maximum stack usage in bytes */
    int         td_stackMargin;         /* current stack margin in bytes */
    int         td_errorStatus;         /* most recent task error status */
    int         td_delay;               /* delay/timeout ticks */
    EVENTS_DESC td_events;              /* VxWorks events information */
    char        td_name[VX_TASK_NAME_LENGTH + 1];   /* name of task */
    RTP_ID      td_rtpId;               /* RTP owning the task */
    int         td_cpuIndex;            /* cpu index running on (if any) */
} TASK_DESC;
// 函数原型

    //VX_FP_TASK VX_DEALLOC_STACK VX_UNBREAKABLE

#define VX_FP_TASK         0x0001  /* 任务使用浮点运算 */
#define VX_DEALLOC_STACK   0x0002  /* 任务退出时释放其栈 */
#define VX_UNBREAKABLE    0x0004  /* 任务不可被打断 */

#define VX_SYS_CLK_RATE 100

TASK_ID taskNameToId(const char *name);

STATUS taskPrioritySet(TASK_ID,int);

STATUS taskOptionsSet(TASK_ID, int, int);

STATUS taskDelete(TASK_ID);

STATUS taskLock(void);

STATUS taskUnlock(void);

STATUS taskStatusString(TASK_ID tid,char* buf);

TASK_ID taskIdSelf (void);

STATUS taskIdVerify(int tid);

STATUS taskMsSleep(int ms);

const char *taskName(TASK_ID tid);

STATUS taskInfoGet(TASK_ID tid, TASK_DESC *pTaskDesc);


#define VX_SYS_CLK_RATE 100

STATUS taskDelay(int ticks);

#endif /* TASK_LIB_H */