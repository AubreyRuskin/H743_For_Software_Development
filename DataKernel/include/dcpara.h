/* dcpara.h - This file contains the parameter for DC sampling */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 03apr09, dy first created.
*/

/*
DESCRIPTION
This file contains the parameter for DC sampling.
*/

/* includes */

#ifndef DCPARA_H
#define DCPARA_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "vxWorks.h"

/* globals */

/* 下面的数组是热电阻的数据 */

extern const uint16_t cu50[];
extern const uint16_t pt100ba2[];
extern const uint16_t pt100[];
extern const uint16_t cu100[];
extern const uint16_t cu53[];

/* functions */

/* get the temperature using resistance value.
 * Para:
 *     rx, resistance value, magnified 100.
 *     option, type of resistance.
 * Return:
 *     temperature, magnified 256.
 */
INT16 GetTemperature(int32_t rx, uint8_t option);

#ifdef	__cplusplus
}
#endif

#endif                                  /* DCPARA_H */
