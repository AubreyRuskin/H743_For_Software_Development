#ifndef VXWORKS_SEM_COMPAT_H
#define VXWORKS_SEM_COMPAT_H

#include <vxWorks.h>


#include "vxworks_type.h"
// --- VxWorks 类型和宏模拟 ---
typedef void* SEM_ID;
#define NULL_ID ((SEM_ID)NULL)

// semBCreate initial state
typedef enum {
    SEM_EMPTY=0,
    SEM_FULL=1
} SEM_B_STATE;

// semMCreate & semCCreate options
#define SEM_Q_FIFO             0x0
#define SEM_Q_PRIORITY         0x1
#define SEM_INVERSION_SAFE     0x4
#define SEM_DELETE_SAFE        0x8

// semTake timeout options
#define WAIT_FOREVER (-1)
#define NO_WAIT      0


// typedef struct {            /* SEM_INFO */
//     UINT    numTasks;       /* OUT: number of blocked tasks */
//     SEM_TYPE    semType;    /* OUT: semaphore type */
//     int     options;        /* OUT: options with which sem was created */
//     union {
//         UINT    count;      /* OUT: semaphore count (couting sems) */
//         BOOL    full;       /* OUT: binary semaphore FULL? */
//         int     owner;      /* OUT: task ID of mutex semaphore owner */
//     } state;
// } SEM_INFO;

// --- 公共 API 函数原型 ---
SEM_ID semMCreate(int options);
SEM_ID semBCreate(int options, SEM_B_STATE initialState);
STATUS semTake(SEM_ID semId, int timeout);
STATUS semGive(SEM_ID semId);
STATUS semDelete(SEM_ID semId);


SEM_ID     semCCreate(int options, int initialCount);
SEM_ID     semRWCreate(int options, int maxReaders);

STATUS     semExchange(SEM_ID giveSemId, SEM_ID takeSemId,
                       int timeout);
STATUS     semFlush(SEM_ID semId);

STATUS     semRTake(SEM_ID semId, int timeout);
STATUS     semWTake(SEM_ID semId, int timeout);
// STATUS     semInfoGet(SEM_ID semId, SEM_INFO *pInfo);




#endif // VXWORKS_SEM_COMPAT_H