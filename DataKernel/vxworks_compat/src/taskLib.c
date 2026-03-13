// 告诉 C++ 编译器，我们正在实现一个可以被 C 调用的函数

#include <vxWorks.h>
#include <string.h>

#include "vxworks_type.h"


#include "taskLib.h" 


// POSIX 头文件
#include <pthread.h>
#include <sched.h>






typedef int (*VxTaskEntryFunc)(int, int, int, int, int, int, int, int, int,
                               int);


 TASK_ID taskSpawn(char *name,      /* 任务名，一个字符串 */
                             int priority,    /* 任务优先级 (0-255) */
                             int options,     /* 任务选项 (位掩码) */
                             int stackSize,   /* 任务栈大小 (字节) */
                             FUNCPTR entryPt, /* 任务入口函数指针 */
                             int arg1, /* 传递给入口函数的第1个参数 */
                             int arg2, /* 传递给入口函数的第2个参数 */
                             int arg3, /* 传递给入口函数的第3个参数 */
                             int arg4, /* 传递给入口函数的第4个参数 */
                             int arg5, /* 传递给入口函数的第5个参数 */
                             int arg6, /* 传递给入口函数的第6个参数 */
                             int arg7, /* 传递给入口函数的第7个参数 */
                             int arg8, /* 传递给入口函数的第8个参数 */
                             int arg9, /* 传递给入口函数的第9个参数 */
                             int arg10 /* 传递给入口函数的第10个参数 */
) {
 

  return NULL;
}

 TASK_ID taskNameToId(const char *name) {
 return ERROR;
}

int taskWait(TASK_ID tid, void **returnValue) {
  if (tid == ERROR)
    return -1;


  return -1;
}

STATUS taskPrioritySet(TASK_ID tid, int newPriority) {
  if (tid == ERROR)
    return -1;

  
  return -1;
}

STATUS taskOptionsSet(TASK_ID tid, int options, int mask) {
  
  if (tid == ERROR)
    return -1;

  
  return 0; // 假装成功
}

STATUS taskDelete(TASK_ID tid) {
  if (tid == ERROR)
    return -1;


  return 0;
}


int taskLock(void) {
    // 确保互斥锁只被初始化一次
   
    
    return ERROR;
}

int taskUnlock(void) {
   

    return ERROR;
}


STATUS taskDelay(int ticks) {
  
    return OK;
}


STATUS taskStatusString(TASK_ID tid,char* buf)
{
  //todo: 实现任务状态查询
  strcpy(buf,"READY");
  return OK;
}


STATUS taskIdVerify(int tid){
  //todo 
  return OK;
}

TASK_ID taskIdSelf (void)
{
  return NULL;
}

STATUS taskMsSleep(int ms)
{
  //todo
    return taskDelay((ms * VX_SYS_CLK_RATE + 999) / 1000); // 向上取整
}

const char *taskName(TASK_ID tid){
  return "task";
}

STATUS taskInfoGet(TASK_ID tid, TASK_DESC *pTaskDesc){
    return OK;
}

