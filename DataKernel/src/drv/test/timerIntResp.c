/*  2007/7/23
测试中断响应时间,中断响应时间的测试方法是打开一个定时器,
设置其为定时到后自动清零.然后在中断程序中读取计数值,就可以知道花了多长时间进中断.
*/

#include "vxWorks.h"			/* always first */

#include "taskLib.h"
#include "logLib.h"
#include "intLib.h"


/*<BSP0022  CXZ 2007/4/19*/
LOCAL void (*msClkRoutine)(int)   = NULL;
LOCAL int     msClkArg            = 0;
LOCAL BOOL    msClkRunning        = FALSE;
LOCAL BOOL    msClkIntConnected   = FALSE;
LOCAL int     msClkTicksPerSecond = 1000;

extern UINT32 sysCoreFreqGet();
extern UINT32 vxImmrGet();
#define	AUX_CLK_RATE_MIN          1    /* min auxiliary clock rate */
#define	AUX_CLK_RATE_MAX          8000 /* max auxiliary clock rate */
#define IMMR_ADDR   0xF0000000

UINT16  timeCounterVal;
UINT16  maxTimerCount = 0;
/*  Usage:
    reg_ms_Timer_int(timer_isr, 0);
    msClkEnable();
*/

/*  Function:   ms clk int*/
LOCAL void msClkInt (void)
{
    // timeCounterVal = *M8260_TCN3(0xF0000000);

    // *M8260_TER3(IMMR_ADDR) = M8260_TER_REF | M8260_TER_CAP;

    // if(timeCounterVal > maxTimerCount)
    // {
    //     maxTimerCount = timeCounterVal;
    //     logMsg("Timer counter is %d\n", timeCounterVal,0,0,0,0,0);
    // }
    // if (msClkRoutine != NULL)
    // {
    //     (*msClkRoutine) (msClkArg);
    // }
}

/*  Function:   Initalize ms timer and start the timer
*/
void msClkEnable ()
{
    // UINT32 immrVal = vxImmrGet();
    

    /*
    * Calculate the preliminary value for the Reference register. This
    * does not yet take into account whether we will divide the general
    * system clock by 16 or not. The 8 bit left shift accounts for
    * the prescaler which will always be set to 0xFF i.e. the clock
    * will further be divided by 256.
    *
    */

    UINT32  tempDiv = 0;
    // sysCoreFreqGet() / (msClkTicksPerSecond << 8);

    /*
    * Enable the auxiliary clock only if it is not already running
    * and if the required reference value will fit in the reference register
    * if the general system clock is divided by 16.
    */

    // if ((!msClkRunning) && (tempDiv < ((1 << (M8260_TRR_SIZE-1)) * 16)))
    // {
    //     /*
    //     * start and hold in reset (i.e. disable) timer2
    //     * Note that RST is active low while STP is active high
    //     */
    //     *M8260_TGCR2(immrVal) &= ~M8260_TGCR_STP3;	/* clear stop bit */
    //     *M8260_TGCR2(immrVal) &= ~M8260_TGCR_RST3;	/* clear reset bit */

    //     *M8260_TCN3(immrVal) = 0x0;	/* clear the timer counter */

    //     /*
    //     * If the preliminary value for the Reference register is small
    //     * enough, we don't need to divide the general system clock by 16,
    //     * and the preliminary value for the Reference register is the
    //     * value used
    //     */

    //     if (tempDiv < (1 << (M8260_TRR_SIZE-1)))
    //     {
    //         *M8260_TRR3(immrVal) = (UINT16) tempDiv;
    //         *M8260_TMR3(immrVal) = (
    //                                    (M8260_TMR_ICLK_IN_GEN  & M8260_TMR_ICLK_MSK) |    /* int clk */
    //                                    M8260_TMR_ORI |                                 /* int on ref */
    //                                    M8260_TMR_FRR |                                 /* free run */
    //                                    (M8260_TMR_PS_MSK & M8260_TMR_PS_MIN));		/* max prscl  */
    //     }
    //     else
    //     {
    //         *M8260_TRR3(immrVal) = (UINT16) (tempDiv / 16);
    //         *M8260_TMR3(immrVal) = (M8260_TMR_ICLK_IN_GEN_DIV16 |
    //                                 M8260_TMR_ORI |
    //                                 M8260_TMR_FRR |
    //                                 (M8260_TMR_PS_MSK & M8260_TMR_PS_MIN));
    //     }

    //     /* clear all timer events */

    //     *M8260_TER3(immrVal) = M8260_TER_REF | M8260_TER_CAP;

    //     /* enable timer interrupt */

    //     if (! msClkIntConnected)
    //     {
    //         (void) intConnect (INUM_TO_IVEC(INUM_TIMER3),
    //                            (VOIDFUNCPTR) msClkInt, (int)NULL);
    //         msClkIntConnected = TRUE;
    //     }
    //     m8260IntEnable(INUM_TIMER3);

    //     /* Enable timer by removing reset, which is active low */

    //     *M8260_TGCR2(immrVal) |= M8260_TGCR_RST3;	/* set reset bit */
    // }
}

/*  Function:   disable the ms timer
*/
void msClkDisable (void)
{
    // UINT32 immrVal = vxImmrGet();

    // if (msClkRunning)
    // {
    //     m8260IntDisable(INUM_TIMER3);
    //     *M8260_TGCR2(immrVal) |= M8260_TGCR_STP3;	/* stop timer */
    //     msClkRunning = FALSE;		/* clock is no longer running */
    // }
}

/*  Function:   get ms timer tick rate
*/
int msClkRateGet (void)
{
    return (msClkTicksPerSecond);
}

/*  Function:   set ms timer tick rate
    Parameter:  msticksPerSecond; tick per second
*/
STATUS msClkRateSet
(
    int msticksPerSecond	    /* number of clock interrupts per second */
)
{
    if ((msticksPerSecond < AUX_CLK_RATE_MIN)
            || (msticksPerSecond > AUX_CLK_RATE_MAX))
    {
        return (ERROR);
    }

    msClkTicksPerSecond = msticksPerSecond;

    if (msClkRunning)
    {
        msClkDisable();
        msClkEnable();
    }

    return (OK);
}

/*  Function:   Register ms timer interrupt
    Parameter:  isr; ISR
                arg; arg of ISR
    Return value:   OK, ERROR
*/
int reg_ms_Timer_int(void (*isr)(int), int arg)
{
    int retValue = ERROR;
    int key;

    key = intLock();
    if(NULL != isr)
    {
        msClkRoutine = isr;
        msClkArg = arg;
        retValue = OK;
    }

    intUnlock(key);
    return retValue;
}
