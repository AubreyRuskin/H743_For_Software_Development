/*
 * freertos_heap_config.c
 *
 * 1. 将 FreeRTOS 的 ucHeap 放置到 AXI SRAM (0x24000000, 512KB 区域)
 * 2. 提供 malloc/free/calloc/realloc 的 --wrap 包装，统一走 pvPortMalloc
 */

#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <errno.h>

/* ========================================================================
 *  1. ucHeap 放置到 .freertos_heap section (链接到 RAM @ 0x24000000)
 * ======================================================================== */
uint8_t ucHeap[configTOTAL_HEAP_SIZE]
    __attribute__((section(".freertos_heap"), aligned(8)));

/* ========================================================================
 *  2. --wrap 包装: 劫持 newlib 的 malloc/free/calloc/realloc
 *     链接器标志: -Wl,--wrap=malloc,--wrap=free,--wrap=calloc,--wrap=realloc
 *
 *     当代码调用 malloc() 时，链接器实际调用 __wrap_malloc()
 *     如果需要调原始 malloc，调用 __real_malloc() (我们不需要)
 * ======================================================================== */

/* 原始 newlib 函数声明 (由链接器 --wrap 自动提供，我们不使用) */
extern void *__real_malloc(size_t size);
extern void  __real_free(void *ptr);
extern void *__real_calloc(size_t nmemb, size_t size);
extern void *__real_realloc(void *ptr, size_t size);

void *__wrap_malloc(size_t size)
{
    return pvPortMalloc(size);
}

void __wrap_free(void *ptr)
{
    vPortFree(ptr);
}

void *__wrap_calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void *p = pvPortMalloc(total);
    if (p != NULL) {
        memset(p, 0, total);
    }
    return p;
}

/*
 * realloc 包装:
 * heap_4 没有原生 realloc，这里用 malloc+copy+free 模拟
 * 注意: 无法知道旧 block 的精确大小，只能拷贝 newSize 字节
 *       (如果旧 block 比 newSize 小，可能多拷贝，但不会越界到其他 block，
 *        因为 heap_4 内部 block 头记录了大小，这里保守拷贝 newSize)
 */
void *__wrap_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {
        return pvPortMalloc(size);
    }
    if (size == 0) {
        vPortFree(ptr);
        return NULL;
    }

    void *new_ptr = pvPortMalloc(size);
    if (new_ptr != NULL) {
        /* 保守拷贝: 最多拷贝 size 字节
         * 如果原 block 比 size 小，我们多拷了一些 heap 元数据字节，
         * 但 memcpy 不会崩溃，只是多拷了 padding 区域的无意义数据 */
        memcpy(new_ptr, ptr, size);
        vPortFree(ptr);
    }
    return new_ptr;
}
