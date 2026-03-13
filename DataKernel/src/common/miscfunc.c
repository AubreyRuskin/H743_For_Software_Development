/* misfunc.c - This file contains interface to some misc functions(CRC, reboot...) */

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
INCLUDE: miscfunc.h
*/

/* includes */
#ifndef RTT_BUILD
#include <ioLib.h>
#include "memLib.h"
#endif

#include "edpbase.h"
#include "string_compat.h"
#include "logmsg.h"
#include "miscfunc.h"
#include <stdio_compat.h>
#include "math_compat.h"
#include <memLib.h>

/* local functions */

static void EP_Make_CRC32_Table(void);

/* Caculate CCITT-CRC16 of a data block.
 * Parameters:
 *      pucBuff, first address of the data block.
 *      uiLen, total bytes.
 *      unLastCRC, last CRC value.
 * Return value:
 *      Result of CCITT-CRC16. */
uint16_t EP_CCITT_CRC16(uint8_t *pucBuff, u_int uiLen, uint16_t unLastCRC)
{
    /* Table to caculate CRC-16. */
    static const uint16_t aunCCITT_16[256]=
    {
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
        0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
        0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
        0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
        0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
        0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
        0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
        0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
        0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
        0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
        0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
        0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
        0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
        0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
        0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
        0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
        0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
        0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
        0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
        0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
        0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
        0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
        0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
        0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
        0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
        0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
        0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
        0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
        0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
        0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
        0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
        0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
    };

    while (uiLen--)
        unLastCRC=(unLastCRC<<8)^aunCCITT_16[(unLastCRC>>8)^*pucBuff++];

    return (unLastCRC);
}

#define POLYNOMIAL (uint32_t)0xEDB88320
static uint32_t aulTbl32_g[256];

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
uint32_t EP_CRC32(char const *pucBuf, u_int uiLen, uint32_t ulCrc)
{
    if (!aulTbl32_g[255])
        EP_Make_CRC32_Table();

    ulCrc ^= 0xFFFFFFFF;

    while (uiLen--)
        ulCrc = (ulCrc >> 8) ^ aulTbl32_g[(ulCrc ^ *pucBuf++) & 0xFF];

    return ulCrc ^ 0xFFFFFFFF;
}

/* Init CRC32 look up table.
 * Parameters:
 *      None.
 * Return value:
 *      None.
 * Alert:
 *      This routine writes each aulTbl32_g entry exactly once,
 *          with the ccorrect final value.  Thus, it is safe to call
 *          even on a table that someone else is using concurrently. */
static void EP_Make_CRC32_Table(void)
{
    unsigned int i, j;
    uint32_t ul = 1;

    aulTbl32_g[0] = 0;
    for (i = 128; i; i >>= 1)
    {
        ul = (ul >> 1) ^ ((ul & 1) ? POLYNOMIAL : 0);

        /* ul is now aulTbl32_g[i] */
        for (j = 0; j < 256; j += 2*i)
            aulTbl32_g[i+j] = aulTbl32_g[j] ^ ul;
    }
}

const uint8_t aucBitWeight8_g[256]=
{
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

/* Caculate bit weight(count of "1" number) of an uint32_t data.
 * Parameter:
 *      ulData, to be count.
 * Return value:
 *      Number of "1" bit in ulData. */
int EP_Bit_Weight(uint32_t ulData)
{
    return aucBitWeight8_g[LL8(ulData)]+aucBitWeight8_g[LH8(ulData)]+
           aucBitWeight8_g[HL8(ulData)]+aucBitWeight8_g[HH8(ulData)];
}

/* Copy ID string.
 * Parameters:
 *      pvD, destination.  Must contains space for (MAX_ID_LEN+1) bytes.
 *      pvS, not '\0' teminated source.
 *      iLen, length of source ID.
 * Return value:
 *      Pointer to destination(pvD).
 * Alert:
 *      Only MAX_ID_LEN bytes in pvS are valid. */
void *EP_ID_Copy(void *pvD, const void *pvS, int iLen)
{
    if (iLen>MAX_ID_LEN)
    {
        /* printf("%s\n",pvS); */		/* DY 11/29/2006 */
        assert(FALSE);
        iLen=MAX_ID_LEN;
    }

    ((uint8_t*)pvD)[iLen]='\0';
    return memcpy(pvD, pvS, iLen);
}

/* decimal data convert to character array.
 * Para:
 *     decimal, decimal data.
 *     array, character array.
 *     len, length of array.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL dectochar(int32_t decimal, uint8_t *array, int8_t len)
{
    int32_t residual;
    int8_t i;

    if (!array)
    {
        assert (FALSE);

        return FALSE;
    }

    if (decimal<0)
    {
        LOG_Dbg_Msg("Can not convert negative.",
                    0, 0, 0, 0, 0, 0);

        assert (FALSE);

        return FALSE;
    }
    else if (decimal<10)
    {
        if (len<1)
        {
            assert (FALSE);

            return FALSE;
        }

        *array = decimal;
    }
    else if (decimal<100)
    {
        if (len<2)
        {
            assert (FALSE);

            return FALSE;
        }

        residual = decimal;
        for (i=0; i<2; i++)
        {
            *array++=residual%10;
            residual=residual/10;
        }
    }
    else if (decimal<1000)
    {
        if (len<3)
        {
            assert (FALSE);

            return FALSE;
        }

        residual = decimal;
        for (i=0; i<3; i++)
        {
            *array++=residual%10;
            residual=residual/10;
        }
    }
    else if (decimal<10000)
    {
        if (len<4)
        {
            assert (FALSE);

            return FALSE;
        }

        residual = decimal;
        for (i=0; i<4; i++)
        {
            *array++=residual%10;
            residual=residual/10;
        }
    }
    else
    {
        LOG_Dbg_Msg("Data too big.", 0, 0, 0, 0, 0, 0);

        assert (FALSE);

        return FALSE;
    }

    return TRUE;
}

/* character array convert to decimal data.
 * Para:
 *     array, character array.
 *     len, length of array.
 * Return:
 *     decimal.
 */
int32_t chartodec(uint8_t *array, int8_t len)
{
    int8_t i;
    int32_t radices = 1;
    int32_t decimal = 0;

    for (i=0; i<len; i++)
    {
        decimal += radices*array[i];
        radices *= 10;
    }

    return decimal;
}

/* trim space of a string.
 * Para:
 *     sDes, destination string.
 *     sString, source string.
 * Return:
 *     NONE.
 */
void trim_space(char *sDes, const char *sString)
{
    char *pTmp;

    if (sDes == NULL)
    {
        return;
    }

    if (sString == NULL)
    {
        sDes[0] = 0x00;
        return;
    }

    pTmp = sDes;
    while (*sString++ == ' ');
    sString--;
    while ((*sDes++ = *sString++) != 0x00);
    sDes--;
    if (sDes <= pTmp)
    {
        sDes[0] = 0x00;

        return;
    }

    while (*--sDes == ' ');
    *++sDes = 0x00;

    return;
}

/* crc32() Takes the array of bytes, macaddr[], representing an
   Ethernet MAC address and returns the CRC-32 result over these bytes,
   where each byte is used in bit-reversed form (Ethernet bit order).
   Index 0 of macaddr[] is the first byte of the address on the wire.
   Test case: the result of crc32 on {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}
   should be 0xad0c28f3.
 */
uint32_t String_Crc32(uint8_t *str, int len)
{
    uint32_t crc, result;
    int byte, i;
    /* CRC-32 algorithm starts by inverting first 4 bytes */
    crc = CRC_INITIAL;
    /* add each byte to running CRC accumulator */
    for (byte = 0; byte < len; ++byte)
    {
        crc ^= (uint32_t)str[byte];
        /* shift CRC right to perform but reversal on byte of address */
        for (i = 0; i < BITS_PER_BYTE; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ CRC_POLYNOMIAL;
            else
                crc >>= 1;
        }
    }
    /* finally, reverse bits of result to get CRC in normal bit order */
    for (result = 0, i = 4*BITS_PER_BYTE-1; i >= 0; crc >>= 1, --i)
        result |= (crc & 1) << i;
    return result;
}

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
uint8_t CDT_BCH_Check(uint8_t *pucBuff,uint16_t unDataLen)
{
    static const uint8_t aucBchEb90[]=
    {
        0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
        0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
        0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
        0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
        0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
        0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
        0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
        0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
        0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
        0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
        0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
        0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
        0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
        0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
        0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
        0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
        0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
        0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
        0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
        0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
        0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
        0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
        0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
        0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
        0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
        0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
        0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
        0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
        0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
        0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
        0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
        0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
    };
    static const uint8_t *pucTable;
    uint16_t unSize;
    uint8_t ucRslt;

    pucTable=aucBchEb90;

    for (ucRslt=0, unSize=unDataLen; unSize!=0; unSize--)
        ucRslt=pucTable[ucRslt^(*pucBuff++)];

    return (~ucRslt);
}

#if !defined(SylixOS_BUILD)&&!defined(RTT_BUILD)
/* ��ȡʣ���ڴ��С
 */
BOOL GetMemPartInfo(int32_t *pNumBytesFree, int32_t *pNumBytesAlloc)
{
    MEM_PART_STATS sts;
    STATUS retcode = ERROR;
    retcode = memPartInfoGet(memSysPartId, &sts);
    if (retcode != ERROR)
    {
        uint8_t ucTempInfo[MAX_ID_LEN];

        *pNumBytesFree = sts.numBytesFree;
        *pNumBytesAlloc = sts.numBytesAlloc;

        sprintf((char *)ucTempInfo, "�߼�ͼ����ʱ��Сʣ���ڴ�(0x%x)\n",
                (int)(*pNumBytesFree));
        LOG_Write(LOG_RUN, ucTempInfo, NULL);

        return TRUE;
    }
    else
    {
        *pNumBytesFree = -1;
        *pNumBytesAlloc = -1;

        return FALSE;
    }

    return FALSE;
}
#endif

/* �жϸ������Ƿ����������Ҫ��.
 * Para:
 *     fVal, ���ж�����ֵ(�����ȸ���).
 *     fStepVal, ����(�����ȸ���).
 * Return:
 *     TRUE, ���ϲ���Ҫ��򲻽��в����ж�.
 *     FALSE, �����ϲ���Ҫ��.
 */
BOOL checkFloatStep(float fVal, float fStepVal)
{
#define EXP_MASK 0x7f800000 /* ָ������ */
#define EXP_SHIFT 23 /* ָ����λλ�� */
#define EXP_OFFSET_SHIFT 151 /* ƫ��ָ����λ�� */
#define MAX_INPUT_VAL 20000.0/* �������ֵ����, Ŀǰ�������ֵ����2�� */

    int32_t i;
    FLT_U32_UNION uVal;
    int32_t lExp;
    double dbOffset;
    double dbMidVal;
    int32_t lShiftNum;
    int32_t lStepShiftVal = 0;
    uint32_t ulOffsetVal;
    uint32_t ulSetVal;
    uint32_t ulStepVal;
    uint32_t ulRes;
    static uint32_t ulCnt = 0;

    ulCnt++;
    printf("%8.8f-%8.8f-%u", fVal, fStepVal, ulCnt);

    /* ����ֵȡ����ֵ�������/��Сֵ�ж� */
    fVal = fabs(fVal);
    if (fVal>MAX_INPUT_VAL)
    {
        printf("***����ֵ̫��!\n");

        return TRUE;
    }
    if (fVal<FLT_PRECISION)
    {
        printf("***����ֵ̫С!\n");

        return TRUE;
    }

    /* ��������Сֵ����Χ�ж� */
    if (fStepVal<FLT_PRECISION)
    {
        printf("***����̫С!\n");

        return TRUE;
    }
    if (!((fStepVal<(1+FLT_PRECISION) && fStepVal>(1-FLT_PRECISION)) /* ������1 */
            || (fStepVal<(0.1+FLT_PRECISION) && fStepVal>(0.1-FLT_PRECISION)) /* ������0.1 */
            || (fStepVal<(0.01+FLT_PRECISION) && fStepVal>(0.01-FLT_PRECISION)) /* ������0.01 */
            || (fStepVal<(0.001+FLT_PRECISION) && fStepVal>(0.001-FLT_PRECISION)) /* ������0.001 */
         ))
    {
        printf("***����=%f! ����Ч����(1/0.1/0.01/0.001)\n", fStepVal);

        return TRUE;
    }

    /* ���㸡��ƫ�� */
    uVal.fVal = fVal;
    lExp = (int32_t)((uVal.ulVal & EXP_MASK) >> EXP_SHIFT);
    lExp -= EXP_OFFSET_SHIFT;
    dbOffset = pow(2.0, lExp);

    /* ����ƫ�����ת��Ϊ��������λ��, ����0ʱ����λ, ��0
     * ֵС��20000.0ʱ, ���ƫ��Ϊ0.0009765625, ��λΪ-4, ������ִ���0�����
     */
    lShiftNum = (int32_t)log10(dbOffset)-1;
    if (lShiftNum<0)
    {
        lShiftNum = -lShiftNum;
    }
    else
    {
        lShiftNum = 0;
    }

    /* ƫ��ת��Ϊ���� */
    dbMidVal = dbOffset*pow(10.0, lShiftNum);
    if (dbMidVal>(UINT_MAX-1))
    {
        printf("***ƫ���м�ֵ̫��!\n");

        return TRUE;
    }
    ulOffsetVal = (uint32_t)dbMidVal+1;

    /* ����ֵ��ͬ������ת��Ϊ����, ���ݾ���Ҫ��, ƫ���ڵ�8λ����,
     * �����λ���ᳬ��9λ, ��32λ������ʾ��Χ��
     */
    dbMidVal = fVal*pow(10.0, lShiftNum);
    if (dbMidVal>UINT_MAX)
    {
        printf("***�м�ֵ̫��!\n");

        return TRUE;
    }
    ulSetVal = (uint32_t)dbMidVal;

    /* ����ת��Ϊ���� */
    ulStepVal = 1;
    if ((fStepVal<(1.0+FLT_PRECISION)) && (fStepVal>(1.0-FLT_PRECISION)))
    {
        lStepShiftVal = 0;
    }
    else if ((fStepVal<(0.1+FLT_PRECISION)) && (fStepVal>(0.1-FLT_PRECISION)))
    {
        lStepShiftVal = 1;
    }
    else if ((fStepVal<(0.01+FLT_PRECISION)) && (fStepVal>(0.01-FLT_PRECISION)))
    {
        lStepShiftVal = 2;
    }
    else if ((fStepVal<(0.001+FLT_PRECISION)) && (fStepVal>(0.001-FLT_PRECISION)))
    {
        lStepShiftVal = 3;
    }

    lStepShiftVal = lShiftNum-lStepShiftVal;
    for (i = 0; i<lStepShiftVal; i++)
    {
        ulStepVal *= 10;
    }

    /* �������� */
    ulRes = ulSetVal%ulStepVal;
    if (ulRes>ulStepVal/2)
    {
        ulRes = ulStepVal-ulRes;
    }

    printf("||%u %u %u %u\n", ulSetVal, ulStepVal, ulOffsetVal, ulRes);

    /* ����С��ƫ��, ֱ������Ч */
    if (ulStepVal<ulOffsetVal)
    {
        return TRUE;
    }

    /* ����С��ƫ��, ������Ч */
    if (ulRes <= ulOffsetVal)
    {
        return TRUE;
    }

    return FALSE;
}

/* 4�ֽڷ���洢.
 * Para:
 *     pSrc, Դ��ַ.
 *     pDest, Ŀ�ĵ�ַ.
 * Return:
 *     NONE.
 */
void chgEndian(uint32_t *pSrc, uint32_t *pDest)
{
    uint8_t *pucSrc = NULL;
    uint8_t *pucDest = NULL;

    if ((pSrc == NULL) || (pDest == NULL))
    {
        return;
    }

    pucSrc = (uint8_t *)pSrc;
    pucDest = (uint8_t *)pDest;

    pucDest[0] = pucSrc[3];
    pucDest[1] = pucSrc[2];
    pucDest[2] = pucSrc[1];
    pucDest[3] = pucSrc[0];

    return;
}