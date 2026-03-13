/* irig_b.h - This file contains code to do IRIG-B time checking */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01h, 24feb2009, dy change the code style.
01g, 28sep05, ws Add year message, Verified version 1.2.
01f, 28sep05, hdx Add year message, Merged IRIG-B year decode work by xingzhi Chen, update to 1.2.
01e, 28sep05, gsm Verified version 1.1.
01d, 24sep05, hdx Add year message, update to 1.1.
01c, 30may05, hcj Verified version 1.0.
01b, 24may05, hdx Updated to version 1.0.
01a, 05arp05, hcj Create first version 0.9.
*/

/*
DESCRIPTION
This file contains code to do IRIG-B time checking.
*/

/* includes */

#ifndef IRIG_B_H
#define IRIG_B_H

#ifdef	__cplusplus
extern "C" {
#endif

/* typedefs */

typedef struct
{
    char cSecond;       /* 0-59 */
    char cMinute;           /* 0-59 */
    char cHour;   /* 0-23 */
    char cYear;       /* Last 2 bit of BCD year, 00=INVALID */
    int iDay;   /* 1-365(366 for leap year) */
} IRIG_BTime;

typedef struct
{
    int iYear;       /* 2005, 2006... */
    char cMonth;            /* 1-12 */
    char cDate;   /* 1-31 */
} IRIG_BDate;

/* register a adjusting function.
 * Para:
 *     Check_Time, callback function set by user to process/adjust time, called in Irig_B_ms_Scan.
 *     (para: ptmNow, the current time; iDelta_ms, the ms number of the current time)
 * Return:
 *     NONE.
 */
void Irig_B_Reg_Time_Func(void (*Check_Time)(IRIG_BTime *ptmNow, int iDelta_ms));

/* Scan function to be called every ms.
 * Para:
 *     iGpsDiSts, the GSP input state, 1: valid; 0: invalid.
 * Return:
 *     0, or -1.
 */
void Irig_B_ms_Scan(int iGpsDiSts);

/* Create date from IRIG-B infomation.
 * Para:
 *     pdt, the reference time.
 *     ptm, result of decode.
 * Return:
 *     NONE.
 *
 * alert: the caller must guarantee the validity of the input parameter, otherwise the result is unsure.
 *
 */
void Irig_B_Create_Date(IRIG_BDate *pdt, const IRIG_BTime *ptm);

#ifdef	__cplusplus
}
#endif

#endif                                  /* IRIG_B_H */
