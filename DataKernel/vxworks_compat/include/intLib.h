#ifndef INT_LIB_H
#define INT_LIB_H

#include "vxworks_type.h"
STATUS intConnect(int vector, VOIDFUNCPTR routine, int arg1);
// STATUS intDisconnect(int vector, VOIDFUNCPTR routine, int arg1);
STATUS initTimer(int vector, int interval_us);
STATUS initTimerMs(int vector, int interval_ms);
int intLock();
int intUnlock(int key);
STATUS intEnable(int vector);
STATUS intDisable(int vector);


#endif /* INT_LIB_H */