/************************************************************************/
/*                                                                      */
/*      Copyright (c) 2006 SNAC(Guodian Nanjing Automation Co., Ltd.)   */
/*      All Rights Reserved.                                            */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* FILE NAME                                            VERSION         */
/*                                                                      */
/*      riscTimer.h                               EDPx-05-0.1           */
/*                                                                      */
/* COMPONENT                                                            */
/*                                                                      */
/*      MPC82xx RISC Timer Control head file                            */
/*                                                                      */
/* DESCRIPTION                                                          */
/*                                                                      */
/*    This file contains program to control the RISC Timer of MPC82xx   */
/*                                                                      */
/*                                                                      */
/* AUTHOR                                                               */
/*                                                                      */
/*      Chen, Xinzhi, SNAC                                              */
/*                                                                      */
/* DATA STRUCTURES                                                      */
/*                                                                      */
/*      None                                                            */
/*                                                                      */
/* FUNCTIONS                                                            */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/* DEPENDENCIES                                                         */
/*                                                                      */
/*      None                                                            */
/*                                                                      */
/* HISTORY                                                              */
/*                                                                      */
/*         NAME            DATE                    REMARKS              */
/*                                                                      */
/*      Chen, Xinzhi      2006/1/25                 1.00                */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/*    Usage Sample:                                                     */
/*           RiscTimerInit();                                           */
/*           regRiscTimer(0, ISR, 100000);                              */
/*           rtmEnable(0)                                               */
/*           startRiscTimer(0)                                          */
/*                                                                      */
/*        for one shot timer, we can us startRiscTimer to restart it    */
/*                                                                      */
/*    Notes:  1. The RISC Timer index is from 0 to 15                   */
/*            2. Enable_RiscTimer and Disable_RiscTimer affect          */
/*               all timer                                              */
/*            3. RISC Timer use the Dual-port RAM from 0x1F80 to        */
/*               0x1FAF                                                 */
/*            4. Default timer option is one shot                       */
/*            5. The timer isr should be be declared as                 */
/*               void foo(int arg);                                     */
/*                                                                      */
/************************************************************************/
#ifndef RISC_TIMER_H
#define RISC_TIMER_H

typedef enum ENUM_RISC_TIMER_OPTION_ENUM
{
    RTM_OPT_ONESHOT,
    RTM_OPT_REPEAT,
} RISC_TIMER_OPTION_ENUM;

/*  Function:   Initialize RISC Timer
*/
void    RiscTimerInit();

/*  Function:   Enable Timer of RISC Controller and enable RISC Timer interrupt
*/
void    Enable_RiscTimer();

/*  Function:   Disable Timer of RISC Controller and disable RISC Timer interrupt
*/
void    Disable_RiscTimer();

/*  Function:   Resister RISC Timer time and isr
    Parameter:  timerIndex; risc timer index; form 0 to MAX_RISC_TIMER_COUNT
                isr;    isr of timer
                arg;    parameter pass to isr
                microsecond;    time    (us)
    Retrun Value:   OK; ERROR
*/
STATUS  regRiscTimer(unsigned char timerIndex, void (*isr)(int), int arg, unsigned int microsecond);

/*  Function:   start risc timer
    Parameter:  timerIndex; Index of rise timer, from 0 to 15
    return value:   OK, ERROR
*/
STATUS  startRiscTimer(unsigned char timerIndex);

/*  Function:   Set the option of RISC Timer
    Parameter:  timerIndex;  Risc timer index; from 0 to MAX_RISC_TIMER_COUNT
                option;      RTM_OPT_ONESHOT;    one shot mode,
                             RTM_OPT_REPEAT;     repeat mode
*/
void    setRiscTimerOpt(unsigned char timerIndex, RISC_TIMER_OPTION_ENUM option);

/*  Function:   Enable Risc timer
    Parameter:  timerIndex; risc timer index; form 0 to MAX_RISC_TIMER_COUNT
    return value:   OK, ERROR
*/
STATUS  rtmEnable(unsigned char timerIndex);

/*  Function:   Disable Risc timer
    Parameter:  timerIndex; risc timer index; form 0 to MAX_RISC_TIMER_COUNT
    return value:   OK, ERROR
*/
STATUS  rtmDisable(unsigned char timerIndex);

#endif
