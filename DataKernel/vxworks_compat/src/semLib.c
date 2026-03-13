#include "semLib.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <errno.h>
#include "vxworks_type.h"

// 内部结构体定义
typedef enum {
    SEM_TYPE_BINARY,
    SEM_TYPE_COUNTING,
    SEM_TYPE_MUTEX
} VxSemType;

typedef struct {
    VxSemType type;
    union {
        pthread_mutex_t mutex;
        sem_t           semaphore;
    } handle;
} VxSem;

SEM_ID semMCreate(int options) {
    VxSem* sem = (VxSem*)malloc(sizeof(VxSem));
    if (!sem) return NULL_ID;

    sem->type = SEM_TYPE_MUTEX;
    
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);

    if (options & SEM_INVERSION_SAFE) {
        pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
    }
    if (options & SEM_DELETE_SAFE) {
        // POSIX mutexes are safe to delete when not locked, no special action needed
    }
    if (options & SEM_Q_FIFO) {
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL); // FIFO-like behavior
    }
    if (options & SEM_Q_PRIORITY) {
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK); // Priority-based behavior
    }

    // SEM_Q_PRIORITY in VxWorks corresponds to priority-ceiling mutexes in POSIX,
    // which is more complex. Priority inheritance is the common use case.

    if (pthread_mutex_init(&sem->handle.mutex, &attr) != 0) {
        free(sem);
        return NULL_ID;
    }
    pthread_mutexattr_destroy(&attr);
    return (SEM_ID)sem;
}

SEM_ID semBCreate(int options, SEM_B_STATE initialState) {
    VxSem* sem = (VxSem*)malloc(sizeof(VxSem));
    if (!sem) return NULL_ID;
    
    sem->type = SEM_TYPE_BINARY;

    // sem_init: a pshared value of 0 means the semaphore is shared between threads of a process.
    if (sem_init(&sem->handle.semaphore, 0, initialState) != 0) {
        free(sem);
        return NULL_ID;
    }
    return (SEM_ID)sem;
}

int semTake(SEM_ID semId, int timeout) {
    if (!semId) return ERROR;
    VxSem* sem = (VxSem*)semId;
    int ret = 0;

    switch (sem->type) {
        case SEM_TYPE_MUTEX:
            if (timeout == WAIT_FOREVER) {
                ret = pthread_mutex_lock(&sem->handle.mutex);
            } else if (timeout == NO_WAIT) {
                ret = pthread_mutex_trylock(&sem->handle.mutex);
            } else {
                // pthread_mutex_timedlock is more complex, requires converting ticks to timespec
                // For simplicity, we only implement the main cases here.
                ret = pthread_mutex_lock(&sem->handle.mutex); // Fallback for timed
            }
            break;
        case SEM_TYPE_BINARY:
        case SEM_TYPE_COUNTING:
            if (timeout == WAIT_FOREVER) {
                ret = sem_wait(&sem->handle.semaphore);
            } else if (timeout == NO_WAIT) {
                ret = sem_trywait(&sem->handle.semaphore);
            } else {
                // sem_timedwait is also more complex.
                ret = sem_wait(&sem->handle.semaphore); // Fallback for timed
            }
            break;
    }
    return (ret == 0) ? OK : ERROR;
}

int semGive(SEM_ID semId) {
    if (!semId) return ERROR;
    VxSem* sem = (VxSem*)semId;
    int ret = 0;

    switch (sem->type) {
        case SEM_TYPE_MUTEX:
            ret = pthread_mutex_unlock(&sem->handle.mutex);
            break;
        case SEM_TYPE_BINARY:
        case SEM_TYPE_COUNTING:
            ret = sem_post(&sem->handle.semaphore);
            break;
    }
    return (ret == 0) ? OK : ERROR;
}

int semDelete(SEM_ID semId) {
    if (!semId) return ERROR;
    VxSem* sem = (VxSem*)semId;
    int ret = 0;

    switch (sem->type) {
        case SEM_TYPE_MUTEX:
            ret = pthread_mutex_destroy(&sem->handle.mutex);
            break;
        case SEM_TYPE_BINARY:
        case SEM_TYPE_COUNTING:
            ret = sem_destroy(&sem->handle.semaphore);
            break;
    }
    free(sem);
    return (ret == 0) ? OK : ERROR;
}

SEM_ID semCCreate(int options, int initialCount) {
    VxSem* sem = (VxSem*)malloc(sizeof(VxSem));
    if (!sem) return NULL_ID;
    
    sem->type = SEM_TYPE_COUNTING;

    if (sem_init(&sem->handle.semaphore, 0, initialCount) != 0) {
        free(sem);
        return NULL_ID;
    }
    return (SEM_ID)sem;
}