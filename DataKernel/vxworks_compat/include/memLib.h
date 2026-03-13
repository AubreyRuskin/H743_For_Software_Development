#ifndef MEM_LIB_H
#define MEM_LIB_H

#include "vxworks_type.h"


#include <stdint.h> // For intptr_t



// --- 类型和宏定义 ---

// 使用 intptr_t 来定义 PART_ID，方便与整数（如-1）兼容
typedef intptr_t PART_ID;

#define NULL_PART ((PART_ID)NULL)

// 假设 OK 和 ERROR 宏已经定义
#ifndef OK
#define OK 0
#endif
#ifndef ERROR
#define ERROR (-1)
#endif

// 在 VxWorks 中，memSysPartId 是一个指向系统内存池的全局变量
// 我们在这里也声明一个，并在 .c 文件中定义它
extern PART_ID memSysPartId;


// --- 函数原型 ---

PART_ID memPartCreate(char *pPool, unsigned poolSize);
void* memPartAlloc(PART_ID partId, unsigned nBytes);
int memPartFree(PART_ID partId, char *pBlock);
int memPartShow(PART_ID partId, int type);
int memFindMax(void); // 这个函数查找的是系统内存池


typedef struct {
    unsigned long numBytesFree;    /* Number of Free Bytes in Partition       */
    unsigned long numBlocksFree;   /* Number of Free Blocks in Partition      */
    unsigned long maxBlockSizeFree;/* Maximum block size that is free.        */
    unsigned long numBytesAlloc;   /* Number of Allocated Bytes in Partition  */
    unsigned long numBlocksAlloc;  /* Number of Allocated Blocks in Partition */
    unsigned long maxBytesAlloc;   /* Maximum number of Allocated Bytes at any*/
                                   /* time */
} MEM_PART_STATS;

STATUS   memPartInfoGet (PART_ID partId, MEM_PART_STATS *ppartStats);



#endif