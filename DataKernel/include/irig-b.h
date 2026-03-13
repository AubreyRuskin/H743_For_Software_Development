#ifndef IRIG_B_H
#define IRIG_B_H



#include <vxWorks.h>
#include "edpbase.h"



typedef struct
{
    uint8_t ucSecond ;
    uint8_t ucMinute ;
    uint8_t ucHour ;
    uint16_t ucDay ;
} IRIG_BTime;


void irig_B_int_link(void);
void Timer3_ISR1(int val);
void IRIGDeCode(INT8 codeMark);
void GetTimeInfo();
BOOL  irig_B_Initialize(void);



#endif                                  /* IRIG_B_H */