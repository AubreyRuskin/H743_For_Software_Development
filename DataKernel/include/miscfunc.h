/* misfunc.h - This file contains interface to some misc functions(CRC, reboot...) */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02a, 30may07, dy change the code style.
01c, 29jul03, hdx Updated to version 1.0.
01b, 27may03 hdx Verified version 0.1.
01a, 15feb03, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains interface to some misc functions(CRC, reboot...).
*/

#ifndef MISCFUNC_H
#define MISCFUNC_H


#include "vxworks_type.h"
#ifdef	__cplusplus
extern "C" {
#endif

/* globals */

extern const uint8_t aucBitWeight8_g[256];

/* Caculate CCITT-CRC16 of a data block.
 * Parameters:
 *      pucBuff, first address of the data block.
 *      uiLen, total bytes.
 *      unLastCRC, last CRC value.
 * Return value:
 *      Result of CCITT-CRC16. */
uint16_t EP_CCITT_CRC16(uint8_t *pucBuff, u_int uiLen, uint16_t unLastCRC);

/* Caculate CRC32.
 * Parameters:
 *      pucBuff, first address of the data block.
 *      uiLen, total bytes.
 *      ulCRC, last CRC value.
 * Return value:
 *      Result of CRC32.
 * Alert:
 *      This computes the standard preset and inverted CRC, as used
 *          by most networking standards.  Start by passing in an initial
 *          chaining value of 0, and then pass in the return value from the
 *          previous EP_CRC32() call.  The final return value is the CRC.
 *          Note that this is a little-endian CRC, which is best used with
 *          data transmitted lsbit-first, and it should, itself, be appended
 *          to data in little-endian byte and bit order to preserve the
 *          property of detecting all burst errors of length 32 bits or less. */
uint32_t EP_CRC32(char const *pucBuf, u_int uiLen, uint32_t ulCrc);

/* Caculate bit weight(count of "1" number) of an uint32_t data.
 * Parameter:
 *      ulData, to be count.
 * Return value:
 *      Number of "1" bit in ulData. */
int EP_Bit_Weight(uint32_t ulData);

/* Copy ID string.
 * Parameters:
 *      pvD, destination.  Must contains space for (MAX_ID_LEN+1) bytes.
 *      pvS, not '\0' teminated source.
 *      iLen, length of source ID.
 * Return value:
 *      Pointer to destination(pvD).
 * Alert:
 *      Only MAX_ID_LEN bytes in pvS are valid. */
void *EP_ID_Copy(void *pvD, const void *pvS, int iLen);

/* decimal data convert to character array.
 * Para:
 *     decimal, decimal data.
 *     array, character array.
 *     len, length of array.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL dectochar(int32_t decimal, uint8_t *array, int8_t len);

/* character array convert to decimal data.
 * Para:
 *     array, character array.
 *     len, length of array.
 * Return:
 *     decimal.
 */
int32_t chartodec(uint8_t *array, int8_t len);

/* trim space of a string.
 * Para:
 *     sDes, destination string.
 *     sString, source string.
 * Return:
 *     NONE.
 */
void trim_space(char *sDes, const char *sString);

/* Caculate BCH check code for CDT protocol.
 * Parameters:
 *      pucBuff, pointer to the first byte of message.
 * Return value:
 *      Check code of the 5 bytes.
 * Alert:
 *      In standard DL451-91 CDT protocol, the syn. word should be 2 bytes:
 *      0xD7 and 0x09, then LSB is shifted out first which causes EB90 pattern
 *      on the serial line.  Further more, BCH code is pruducted in the same
 *      bit order.  D1 of ucCdtVer should be 1 in this satuation.
 *      But in some old systems, they explain the protocol in another way(MSB
 *      first), so product 0xEB90 as syn. word and the other BCH code caculate
 *      way.  For compatible with these systems, we support both. */
extern uint8_t CDT_BCH_Check(uint8_t *pucBuff,uint16_t unDataLen);

#define CRC_POLYNOMIAL 0xedb88320
#define CRC_INITIAL    0xffffffff
#define MAC_ADDRLEN    6
#define BITS_PER_BYTE  8

/* 获取剩余内存大小
 */
extern BOOL GetMemPartInfo(int32_t *pNumBytesFree, int32_t *pNumBytesAlloc);

#ifdef	__cplusplus
}
#endif

#endif                                  /* MISCFUNC_H */
