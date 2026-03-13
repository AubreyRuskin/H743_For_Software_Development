/* VTBOX_Box.h - subroutine library for virtual box */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 07nov08, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for virtual box.
*/

#ifndef VTBOX_BOX_H
#define VTBOX_BOX_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "vxWorks.h"			/* always first */
#include "VTBOX_Interface.h"
#include "VTBOX_SamInterface.h"
#include "realdata.h"
#include "logic.h"

/* defines */

/* globals */

extern BOOL abVirtBoxChIsInitOver_g[MAX_VT_BOX_COUNT];  		/* initialization finishing flag. */
extern EP_ELEM_IO *apVirtBoxMidSrcAOPt_g[MAX_VT_BOX_COUNT][MAX_VTBOX_AO_NUM];  /* AO from logic middle variable. */

/* functions */

/* Initialize the virtual box AO configuration
 * Para:
 *     ucVtBoxPos, position of the virtual box, begin from 0.
 *     ucAoNum, number of channel.
 */
EP_STATUS VtBox_InitAOCfg(uint8_t ucVtBoxPos, uint8_t ucAoNum);

/* Finish the AO configuration from logic graph variables.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, error.
 */
EP_STATUS VirtBox_AOCfgInitFinish(void);

#ifdef	__cplusplus
}
#endif

#endif                                  /* VTBOX_BOX_H */
