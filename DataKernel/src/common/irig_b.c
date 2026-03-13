/* irig_b.c - This file contains code to do IRIG-B time checking */

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
INCLUDES: irig_b.h
*/

/* includes */

#include "irig_b.h"
#include "logmsg.h"

/* statics */

static void (*pfCheckTime_g)(IRIG_BTime *tmNow, int iDelta_ms);  /* pointer to adjusting function set by application */

/* static functions */

/* decode the irig B.
 * Para:
 *     ptmSave, the current time.
 *     iMark, serial number of code.
 *     iCodeVal, value of code
 * Return:
 *     0, or -1.
 */
static int Irig_B_Decode(IRIG_BTime *ptmSave, int iMark, int iCodeVal);

/* increase 1 second.
 * Para:
 *     ptm, the current time.
 * Return:
 *     NONE.
 */
static void Time_Inc_Sec(IRIG_BTime *ptm);

/* functions */

/* register a adjusting function.
 * Para:
 *     Check_Time, callback function set by user to process/adjust time, called in Irig_B_ms_Scan.
 *     (para: ptmNow, the current time; iDelta_ms, the ms number of the current time)
 * Return:
 *     NONE.
 */
void Irig_B_Reg_Time_Func(void (*Check_Time)(IRIG_BTime *ptmNow, int iDelta_ms))
{
    pfCheckTime_g=Check_Time;
}

/* Scan function to be called every ms.
 * Para:
 *     iGpsDiSts, the GSP input state, 1: valid; 0: invalid.
 * Return:
 *     0, or -1.
 */
void Irig_B_ms_Scan(int iGpsDiSts)
{
    static int iPreSts_s;   /* 前一轮询点采集到的通道数据 */
    static int iWidth_s;          /* 有效脉冲的宽度 */
    static int iPreCode_s=0;   /* 上一个脉冲码 */
    static int iCode_s;  /* 当前脉冲码 */
    static int iMark_s=0;     /* 码序号 */
    static IRIG_BTime tmNow_s;     /* 用于本次解码的时间 */
    static IRIG_BTime tmNext_s;  /* 推算的下一次时间 */
    static int ulCnt=0;

    ulCnt++;
    if (ulCnt%(2400*60) == 1)
    {
        LOG_Dbg_Msg("Irig_B_ms_Scan Enter %d\n", iGpsDiSts, 0, 0, 0, 0, 0);
    }

    if (!iGpsDiSts)		/* iGpsDiSts==0. */
    {
        if (iPreSts_s)    /* 解码时刻 */
        {
            /* 前一时刻为高电平，高电平包含时间  */
            switch(iWidth_s)
            {
                /* 根据脉宽 */
                case 0:
                    break;

                case 1:
                case 2:		/* 二进制0 */
                case 3:
                    iCode_s=0;
                    iMark_s++;
                    break;

                case 4:
                case 5:		/* 二进制1 */
                case 6:
                    iCode_s=1;
                    iMark_s++;
                    break;

                case 7:
                case 8:		/* 位置标志或秒脉冲起始标志 */
                case 9:
                    iCode_s=-1;
                    iMark_s++;

                    /* 捕捉到准时参考点? 两个8ms出现 */
                    if (iPreCode_s==-1)
                    {
                        /* 本次解码无错（iDay<0为出错标志）? */
                        if (tmNow_s.iDay>=0)
                        {
                            /* 和上此推算出的时间匹配？*/
                            if (tmNext_s.cSecond==tmNow_s.cSecond &&
                                    tmNext_s.cMinute==tmNow_s.cMinute &&
                                    tmNext_s.cHour==tmNow_s.cHour &&
                                    tmNext_s.iDay==tmNow_s.iDay)
                            {
                                /* 实际的当前时间 */
                                Time_Inc_Sec(&tmNow_s);

                                if (pfCheckTime_g)
                                    pfCheckTime_g(&tmNow_s, iWidth_s);

                                tmNext_s=tmNow_s;
                            }
                            else
                            {
                                /* 推算出下次的正确时间 */
                                Time_Inc_Sec(&tmNow_s);
                                tmNext_s=tmNow_s;
                            }
                        }

                        tmNow_s.cYear=0;
                        tmNow_s.iDay=0;
                        tmNow_s.cHour=0;
                        tmNow_s.cMinute=0;
                        tmNow_s.cSecond=0;

                        iCode_s=0;
                        iPreCode_s=0;
                        iWidth_s=0;
                        iMark_s=0;
                    }
                    break;

                default:
                    break;
            }

            iPreCode_s=iCode_s;

            if (iMark_s>0 && iMark_s<59 && iMark_s%10!=9)
            {
                /* 利用iDay<0为出错标志 */
                if (tmNow_s.iDay>=0)
                {
                    if (Irig_B_Decode(&tmNow_s, iMark_s, iCode_s))
                        tmNow_s.iDay=-1;
                }
            }
        }
    }
    else    /* iGpsDiSts==1. */
    {
        if (iPreSts_s)
            iWidth_s++;
        else
            iWidth_s=1;
    }

    iPreSts_s=iGpsDiSts;
}

/* decode the irig B.
 * Para:
 *     ptmSave, save the current time.
 *     iMark, serial number of code, begin from the two 8 ms pulse code.
 *     iCodeVal, value of code, 2ms: 0; 5ms: 1.
 * Return:
 *     0, or -1.
 */
static int Irig_B_Decode(IRIG_BTime *ptmSave, int iMark, int iCodeVal)
{
    if (iCodeVal==0)
        return 0;
    else if (iCodeVal!=1)
        return -1;

    if (iMark >0 && iMark <5)           /* Units of seconds 1-4. */
        ptmSave->cSecond +=  1<<(iMark-0-1);
    else if (iMark >5 && iMark <9)      /* Tens of seconds 6-8. */
        ptmSave->cSecond +=  10*(1<<(iMark-5-1));
    else if (iMark >9 && iMark <14)     /* Units of minutes 10-13. */
        ptmSave->cMinute += 1<<(iMark-9-1);
    else if (iMark >14 && iMark <18)    /* Tens of minutes 15-17. */
        ptmSave->cMinute += 10*(1<<(iMark-14-1));
    else if (iMark >19 && iMark <24)    /* Units of hours 20-23. */
        ptmSave->cHour += 1<<(iMark-19-1);
    else if (iMark >24 && iMark <27)    /* Tens of hours 25-26. */
        ptmSave->cHour += 10*(1<<(iMark-24-1));
    else if (iMark >29 && iMark <34)    /* Units of days 30-33. */
        ptmSave->iDay += 1<<(iMark-29-1);
    else if (iMark >34 && iMark <39)    /* Tens of days 35-38. */
        ptmSave->iDay += 10*(1<<(iMark-34-1));
    else if (iMark >39 && iMark <42)    /* Hundreds of days 40-41. */
        ptmSave->iDay += 100*(1<<(iMark-39-1));
    else if ((iMark >49) && (iMark<54)) /* Units of years 50-53. */
    {
        ptmSave->cYear += 1<<(iMark-49-1);
    }
    else if ((iMark >54) && (iMark<59)) /* Tens of years 55-58. */
    {
        ptmSave->cYear += 10 * (1<<(iMark-54-1));
    }
    else     /* 索引标志位置却是5ms脉冲 */
        return -1;

    return 0;
}

/* increase one second to time sturcture.
 * Para:
 *     ptm, the processed time.
 * Return:
 *     NONE.
 */
static void Time_Inc_Sec(IRIG_BTime *ptm)
{
    if (++ptm->cSecond>=60)
    {
        ptm->cSecond -= 60;
        if (++ptm->cMinute>=60)
        {
            ptm->cMinute -= 60;
            if (++ptm->cHour>=24)
            {
                ptm->cHour -= 24;
                ptm->iDay++;
            }
        }
    }
}

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
void Irig_B_Create_Date(IRIG_BDate *pdt, const IRIG_BTime *ptm)
{
    /* Tabel data assumes month from 0 to 11. */
    static const int aiMonthDay[]=
    {0,31,59,90,120,151,181,212,243,273,304,334,365};
    static const int aiLeapMonthDay[]=
    {0,31,60,91,121,152,182,213,244,274,305,335,366};
    int iDay;
    int cMonth;

    if (ptm->cYear)
    {
        /* IRIG-B year message is valid. */
        pdt->iYear=2000+ptm->cYear;
    }
    else
    {
        /* Use year message passed by pdt->iYear and think of year boundary. */
        if (pdt->cMonth==12 && ptm->iDay<5)
            pdt->iYear++;
        else if (pdt->cMonth==1 && ptm->iDay>360)
            pdt->iYear--;
    }

    iDay=ptm->iDay-1;

    /* 366/29=12, so aiMonthDay[12] & aiLeapMonthDay[12] must be valid.
     * cMonth should equal to actual month or great 1. */
    cMonth=iDay/29;
    if (pdt->iYear & 0x03)              /* "& 0x03" equal to "%4". */
    {
        /* Not a leap year. */
        if (iDay<aiMonthDay[cMonth])
            cMonth--;

        pdt->cDate=(char)(iDay-aiMonthDay[cMonth]+1);
    }
    else
    {
        /* Leap year. */
        if (iDay<aiLeapMonthDay[cMonth])
            cMonth--;

        pdt->cDate=(char)(iDay-aiLeapMonthDay[cMonth]+1);
    }

    pdt->cMonth=(char)(cMonth+1);
}
