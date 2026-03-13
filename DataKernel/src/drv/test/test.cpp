#include "vxWorks.h"
#include "taskLib.h"
#include "stdio_compat.h"
#include "logmsg.h"

class my_class
{
public:
    my_class();
    int Task(void);
    // static int TaskEntry(my_class *myClass);
};

extern "C" int TaskEntry(my_class *pClassInstance)
{
    // 这个函数接收一个指向类实例的指针，并调用其成员函数
    if (pClassInstance) {
        return pClassInstance->Task();
    }
    return ERROR;
}

my_class::my_class()
{
    taskSpawn("Task", 90, 0, 10000, (FUNCPTR)TaskEntry, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int my_class::Task(void)
{
    while(1)
    {
        taskDelay(100);
        logMsg("Hi!\n", 0, 0, 0, 0, 0, 0);
    }
}



// int TaskEntry(my_class *myClass)
// {
//     return myClass->Task();
// }

void CppMain(void)
{
    my_class Obj;
}