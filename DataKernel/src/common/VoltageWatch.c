/* VoltageWatch.c - This file contains the driver program for voltage monitor */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01c, 27jun08, dy migrate to EDP03 and Lici platform.
01b, 27feb08, zy version 1.1 created.
01a, 19apr06, zy first version 1.0 created.
*/

/*
DESCRIPTION
This file contains the driver program for voltage monitor.
INCLUDES: VoltageWatch.h
*/

/* includes */
#include "VoltageWatch.h"
#include "spiio.h"
// #include "hsbCfg.h"
// #include "sacDev.h"
// #include "VX_SysStatus.h"
EP_STATUS WT_VoltExcStsRpt(WT_EXC_INFO_TYPE *pRtVoltExcStsRpt){
    return EP_SUCCESS;
}

EP_STATUS WT_VoltWatchStsRpt(WT_SYSINFO_TYPE *pRtVoltWatchStsRpt)
{
    return EP_SUCCESS;
}
void WT_MegaClrErrRec(void){
    return;
}
WT_QD_TST_RSLT_TYPE WT_MegaQDTst(void){
    return WT_QD_TST_NORMAL;
}