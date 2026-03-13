#include "tickLib.h"
#include <time.h>
#include <stdio_compat.h>

uint32_t tickGet(void){
 struct timespec ts;

    // 使用 CLOCK_MONOTONIC 获取一个自启动后单调递增的时间
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        // 如果失败，打印错误并返回0，这是一个安全的回退值
        perror("clock_gettime failed");
        return 0;
    }

    // 将 timespec 结构中的秒和纳秒，统一转换为一个大的单位，例如微秒
    // 1 秒 = 1,000,000 微秒
    // 1 纳秒 = 1/1000 微秒
    uint64_t total_microseconds = ((uint64_t)ts.tv_sec * 1000000) + ((uint64_t)ts.tv_nsec / 1000);

    // 根据我们定义的时钟频率，计算出每个 tick 包含多少微秒
    // SYS_CLK_RATE = 100 Hz  =>  1 tick = 1/100 s = 10 ms = 10,000 us
    uint64_t microseconds_per_tick = 1000000 / SYS_CLK_RATE;

    // 用总微秒数除以每个 tick 的微秒数，得到的结果就是模拟的 tick 数量
    // 整数除法会自动处理掉不足一个 tick 的余数，这正好模拟了 tick 计数器的离散特性
    return total_microseconds / microseconds_per_tick;
}
