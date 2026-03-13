/********************************************************************************/
/*                                                                              */
/*      Copyright (c) 2003 SNAC(Guodian Nanjing Automation Co., Ltd.)           */
/*      All Rights Reserved.                                                    */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/* FILE NAME                                            VERSION                 */
/*                                                                              */
/*      ext_eth.h                                   EDP01-04-0.1                */
/*                                                                              */
/* COMPONENT                                                                    */
/*                                                                              */
/*      BSP - Board support packages(COM).                                      */
/*                                                                              */
/* DESCRIPTION                                                                  */
/*                                                                              */
/*      This file contains interface to transmit raw data via extend Ether net. */
/*                                                                              */
/* AUTHOR                                                                       */
/*                                                                              */
/*      Daoxu Hu, SNAC                                                          */
/*                                                                              */
/* DATA STRUCTURES                                                              */
/*                                                                              */
/*      None                                                                    */
/*                                                                              */
/* FUNCTIONS                                                                    */
/*                                                                              */
/*      None                                                                    */
/*                                                                              */
/* DEPENDENCIES                                                                 */
/*                                                                              */
/*      None                                                                    */
/*                                                                              */
/* HISTORY                                                                      */
/*                                                                              */
/*         NAME            DATE                    REMARKS                      */
/*                                                                              */
/*      Daoxu Hu        2003.08.12      Created first version 0.1.              */
/*      Daoxu Hu        2003.xx.xx      Verified version 0.1.                   */
/*      Daoxu Hu        2003.xx.xx      Updated to version 1.0                  */
/*                                                                              */
/********************************************************************************/

#ifndef EXT_ETH_H
#define EXT_ETH_H

#include <vxWorks.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define ETH_BD_HEAD     16              /* 6: dest addr; 6: source addr; 2: type/len */
#define ETH_MAX_DAT     1498            /* Not include CRC code. */

#if 0
#define ext_send_raw    DBG_ext_send
#define ext_recv_raw    DBG_ext_recv

static int DBG_ext_send(uint8_t *pucBuf, int iLen)
{
    return -1;
}

static int DBG_ext_recv(uint8_t **ppucBuf, int iTimeout)
{
    return -1;
}
#else
/* Send raw data via extend Ether net.
 * Parameters:
 *      pucBuf, pointer to user data to be send.  This buffer must contain
 *          ETH_BD_HEAD/ETH_BD_TAIL bytes space before/after the user data area
 *          for the low level layer working with zero-copy mode.
 *      iLen, length of user data.  It should<=ETH_MAX_DAT and !=0.
 * Return value:
 *      =iLen on success.  Or -1 means error. */
int ext_send_raw(uint8_t *pucBuf, int iLen);

/* Receive raw data via extend Ether net.
 * Parameters:
 *      ppucBuf, to save pointer to the received user data.  The receive buffer
 *          should be allocated from a circular pool in the low level layer and
 *          needn't high layer do anything after use.
 *          The receive buffer is 16 bytes aligned.
 *      iTimeout, maximal waiting time in system tick unit. Timeouts of
 *          WAIT_FOREVER (-1) and NO_WAIT (0) indicate to wait indefinitely or
 *          not to wait at all.
 * Return value:
 *      Bytes of user data received.  Or 0 means timeout while -1 means error. */
int ext_recv_raw(uint8_t **ppucBuf, int iTimeout);
#endif

#ifdef	__cplusplus
}
#endif

#endif                                  /* EXT_ETH_H */
