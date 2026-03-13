#include "memLib.h"
#include <stdlib.h> // For malloc, free
#include <stdio_compat.h>  // For printf

// 定义一个“伪”的系统分区ID。任何非 NULL 的值都可以。
// 我们的 memPartCreate 会返回这个值，而 memSysPartId 也等于这个值。
#define DUMMY_SYS_PART_ID ((PART_ID)1)

// 定义全局的系统分区 ID
PART_ID memSysPartId = DUMMY_SYS_PART_ID;

/**
 * @brief 创建一个内存分区 (哑实现)
 * @param pPool - 被忽略
 * @param poolSize - 被忽略
 * @return 一个固定的伪句柄，表示成功。
 */
PART_ID memPartCreate(char *pPool, unsigned poolSize) {
    // 我们不使用传入的内存池，因为所有操作都将重定向到 malloc/free。
    // 只需返回一个有效的、非 NULL 的句柄即可。
    (void)pPool;    // 避免编译器关于“未使用参数”的警告
    (void)poolSize;
    return DUMMY_SYS_PART_ID;
}

/**
 * @brief 从指定分区申请内存 (哑实现)
 * @param partId - 被忽略
 * @param nBytes - 要申请的字节数
 * @return 指向已分配内存的指针，或 NULL。
 */
void* memPartAlloc(PART_ID partId, unsigned nBytes) {
    // 忽略 partId，直接调用标准 malloc
    (void)partId;
    return malloc(nBytes);
}

/**
 * @brief 释放内存回指定分区 (哑实现)
 * @param partId - 被忽略
 * @param pBlock - 要释放的内存块指针
 * @return 总是返回 OK
 */
int memPartFree(PART_ID partId, char *pBlock) {
    // 忽略 partId，直接调用标准 free
    (void)partId;
    free(pBlock);
    return OK;
}

/**
 * @brief 显示分区状态 (哑实现)
 */
int memPartShow(PART_ID partId, int type) {
    (void)partId;
    (void)type;
    printf("\nNOTE: memPartShow is a dummy implementation.\n");
    printf("All memory is managed by the system's standard heap (glibc malloc).\n\n");
    return OK;
}

/**
 * @brief 查找最大可用内存块 (哑实现)
 */
int memFindMax(void) {
    printf("WARNING: memFindMax() is not supported on this platform and returns 0.\n");
    // Linux 的 glibc 堆非常复杂，没有一个标准、简单的方法来获取这个值。
    // 返回 0 是一个安全的默认值。
    return 0;
}


/*
 * Get memory pool info
 */
STATUS   memPartInfoGet (PART_ID partId, MEM_PART_STATS *ppartStats)
{
    size_t  stByteSize;
    ULONG   ulSegmentCounter;
    size_t  stUsedByteSize;
    size_t  stFreeByteSize;
    size_t  stMaxUsedByteSize;

    // if (!ppartStats) {
    //     errno = EINVAL;
    //     return  (ERROR);
    // }

    // if (API_RegionStatus(partId, &stByteSize, &ulSegmentCounter,
    //                      &stUsedByteSize, &stFreeByteSize, &stMaxUsedByteSize)) {
    //     return  (ERROR);
    // }

    ppartStats->numBytesFree      = (unsigned long)stFreeByteSize;
    ppartStats->numBlocksFree     = (unsigned long)1;
    ppartStats->maxBlockSizeFree  = (unsigned long)stFreeByteSize;
    ppartStats->numBytesAlloc     = (unsigned long)stUsedByteSize;
    ppartStats->numBlocksAlloc    = (unsigned long)ulSegmentCounter;
    ppartStats->maxBytesAlloc     = (unsigned long)stMaxUsedByteSize;

    return  (OK);
}
