/* man_fram.h - This file contains functions to read/write buffered FRAM */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 8nov02, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains functions to read/write buffered FRAM.
*/

#ifndef MAN_FRAM_H
#define MAN_FRAM_H

/* includes */

#include "edpbase.h"

#ifdef	__cplusplus
extern "C" {
#endif

/* 8k bytes 0бл0x1fff.
 * But the last 1K bytes are reserved for EDP 01 system usage. */
#define FRAM_SIZE   0x2000
#define FR_SYS_BGN  0x1C00

/* Initialize buffered FRAM mangerment system.
 * Parameters:
 *      None.
 * Return value:
 *      EP_SUCCESS, initialize OK.
 *      EP_HARD_ERR, FRAM hardware error. */
EP_STATUS FR_Initialize(void);

/* Write some bytes to FRAM.
 * Parameters:
 *      uiAddr, dst address in FRAM.
 *      pucDat, src data buffer.
 *      uiSize, total length of data.
 * Return value:
 *      EP_SUCCESS, initialize OK.
 *      EP_TIMEOUT, CPU too busy, not time to refresh FRAM.
 *      EP_HARD_ERR, FRAM hardware error. */
EP_STATUS FR_Write(u_int uiAddr, const uint8_t *pucDat, u_int uiSize);

/* Read some bytes to FRAM.
 * Parameters:
 *      uiAddr, src address in FRAM.
 *      pucDat, dst data buffer.
 *      uiSize, total length of data.
 * Return value:
 *      EP_SUCCESS, initialize OK.
 *      EP_HARD_ERR, FRAM hardware error. */
EP_STATUS FR_Read(u_int uiAddr, uint8_t *pucRslt, u_int uiSize);

#ifdef	__cplusplus
}
#endif

#endif                                  /* MAN_FRAM_H */
