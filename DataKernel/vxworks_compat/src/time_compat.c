#include "time_compat.h"

#include <errno.h>

#include "stm32h7xx_hal.h"

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    uint32_t ms;

    if (tp == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    if ((clk_id != CLOCK_REALTIME) && (clk_id != CLOCK_MONOTONIC))
    {
        errno = EINVAL;
        return -1;
    }

    /* TODO: placeholder implementation.
     * Use a real RTC source for CLOCK_REALTIME and a dedicated monotonic
     * high-resolution counter (DWT/TIM) for CLOCK_MONOTONIC.
     */
    ms = HAL_GetTick();
    tp->tv_sec = (time_t)(ms / 1000U);
    tp->tv_nsec = (long)((ms % 1000U) * 1000000UL);

    return 0;
}
