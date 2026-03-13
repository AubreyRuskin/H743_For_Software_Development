/* HardClock.c - This file contains the functions to handle the clock chip */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 22nov06, hcj Created first version 0.1.
*/

/*
DESCRIPTION
This file contains the functions to handle the clock chip.
INCLUDE: Hardlock.h
*/

/* includes */
#include <stdio_compat.h>
#include <datetime.h>
#include "HardClock.h"
#include "ds3231.h"
#include "bspinterface.h"

/* globals */

CHardClock cHardClock ; /* 兼容性增加 */

/***********************************************************************
* BCD_TO_HEX - BCD to HEX
*
* RETURNS: 转换结果
*
*/
uint8_t BCD_TO_HEX (
    uint8_t bcd		/* BCD码*/
)
{
    return ((bcd&0x0F)+((bcd>>4)*10)) ;
}

/***********************************************************************
* HEX_TO_BCD - HEX to BCD
*
* RETURNS: 转换结果
*
*/
uint8_t HEX_TO_BCD (
    uint8_t hex		/* HEX码 */
)
{
    return ((hex%10) +((hex/10)<<4)) ;
}

/***********************************************************************
* SetClock - 设置时钟
*
* RETURNS:
*			EP_SUCCESS, 正常
*			EP_BUF_ERR, 错误
*
*/
EP_STATUS SetClock(
    EP_DATE_TIME *time
)
{
    uint8_t buf[7];

    buf[0]=HEX_TO_BCD(time->ucSec);
    buf[1]=HEX_TO_BCD(time->ucMinute);
    buf[2]=HEX_TO_BCD(time->ucHour);	/*24 hours mode */
    buf[3]=time->ucWeekDay;
    buf[4]=HEX_TO_BCD(time->ucDate);
    buf[5]=HEX_TO_BCD(time->ucMonth);
    buf[6]=HEX_TO_BCD(time->unYear-2000);

    if ((bdType_g == BOARD_TYPE_E03) || (bdType_g == BOARD_TYPE_EXCITE))
    {
        /* EDP03和励磁 */
        return ds3231Write(0,buf,7)==7?EP_SUCCESS:EP_ERROR;
    }
    else if (bdType_g == BOARD_TYPE_E02)
    {
        /* EDP02　*/
        return (Set_Sys_Hw_Clock(buf)==OK)?EP_SUCCESS:EP_ERROR;
    }

    return EP_SUCCESS;
}

/***********************************************************************
* GetClock - 读取时钟
*
* RETURNS:
*		  EP_SUCCESS, 正常
*		  EP_BUF_ERR, 错误
*
*/
EP_STATUS GetClock(
    EP_DATE_TIME *time		/* 时钟指针 */
)
{
    uint8_t buf[7];
    uint8_t i;

    for (i=0; i<3; i++)
    {
        if ((bdType_g == BOARD_TYPE_E03) || (bdType_g == BOARD_TYPE_EXCITE))
        {
            /* EDP03和励磁 */
            if (ds3231Read(0, buf, 7) == 7)
            {
                time->unMicroSec = 0;
                time->unMSEL = 500;
                time->ucSec = BCD_TO_HEX(buf[0]);
                time->ucMinute = BCD_TO_HEX(buf[1]);
                time->ucHour = BCD_TO_HEX(buf[2]);
                time->ucWeekDay = BCD_TO_HEX(buf[3]);
                time->ucDate = BCD_TO_HEX(buf[4]);
                time->ucMonth = BCD_TO_HEX(buf[5]);
                time->unYear = BCD_TO_HEX(buf[6])+2000;

                return EP_SUCCESS;
            }
        }
        else if (bdType_g == BOARD_TYPE_E02)
        {
            /* EDP02平台 */
            if (Get_Sys_Hw_Clock(buf) == 7)
            {
                time->unMicroSec = 0;
                time->unMSEL = 500;
                time->ucSec = BCD_TO_HEX(buf[0]);
                time->ucMinute = BCD_TO_HEX(buf[1]);
                time->ucHour = BCD_TO_HEX(buf[2]);
                time->ucWeekDay = BCD_TO_HEX(buf[3]);
                time->ucDate = BCD_TO_HEX(buf[4]);
                time->ucMonth = BCD_TO_HEX(buf[5]);
                time->unYear = BCD_TO_HEX(buf[6])+2000;

                return EP_SUCCESS;
            }
        }
    }

    /* 提供默认时间 */
    time->unMicroSec = 0;
    time->unMSEL = 0;
    time->ucSec = 0;
    time->ucMinute = 0;
    time->ucHour = 0;
    time->ucWeekDay = 1;
    time->ucDate = 1;
    time->ucMonth = 1;
    time->unYear = 2000;

    return EP_ERROR;
}

void ShowHardClock()
{
    EP_DATE_TIME CurTime;
    if(GetClock(&CurTime)==EP_SUCCESS)
    {
        printf("Current Time: %u-%u-%u %u:%u.%u\n",CurTime.unYear,CurTime.ucMonth,CurTime.ucDate,CurTime.ucHour,CurTime.ucMinute,CurTime.ucSec);
    }
    else
    {
        printf("Get Time Error!\n");
    }
}
