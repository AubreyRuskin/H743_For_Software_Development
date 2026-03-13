/* datetime.h - CM - System bottom functions(DateTime). */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 1aug05, zhangyun modified  for  LowPlatform.
01a, 19dec02, helong first created.
*/

/*
DESCRIPTION
This file contains system date/time functions.
*/

#ifndef DATETIME_H
#define DATETIME_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */
#include "limits_compat.h"

#include "edpbase.h"
#include "iecgoose.h"
/* #include "irig-b.h" */
#include "irig_b.h"
#include <logLib.h>

/* defines */

#define MAXTIMETAGNUM 20

#define GPS_PULSE_TYPE_SEC     0x01
#define GPS_PULSE_TYPE_MIN     0x02
#define GPS_PULSE_TYPE_INVALID 0x04

/*当有闰秒时，给CPU下列两个标志的一个,两个标志都为0表示不含B码*/
#define IRIGB_PLS 0x01 /*正闰秒标志*/
#define IRIGB_NLS 0x02 /*负闰秒标志*/

#define IRIGB_PLS_60SEC 0x01                 /*该时刻是否正闰秒的60秒*/
#define IRIGB_AFTER_PLS_0SEC 0x02      /*该时刻是否正闰秒的0秒后的时间*/
#define IRIGB_AFTER_NLS_0SEC 0x04       /*该时刻是否负闰秒的0秒后的时间*/
#define IRIGB_NLS_TIME_ADJUST 0x40       /*该时刻时间是否进行了负闰秒的+1 -1的时间操作*/
#define IRIGB_PLS_TIME_ADJUST 0x80       /*该时刻时间是否进行了正闰秒的+1 -1的时间操作*/

extern BOOL g_bIrigbLog;
#define SYN_LOG(f, a1, a2, a3, a4, a5, a6)  if (g_bIrigbLog) logMsg(f, a1, a2, a3, a4, a5, a6)

#define IRIGB_SPECIAL_SEC 45  /*闰秒55s-下一分的45s内是特殊处理的时间段*/

#define   OPT_TIME_BASE_FREQ     (sysInputFreq_g >> 2)                        /*TIMEBASE的频率  */

#define TIMEBASE_FREQ (sysInputFreq_g/4)         /* TIMEBASE的频率 */
#define NSULLTIMEBASEPERIOD (uint64_t)1000000000/((uint64_t)TIMEBASE_FREQ)		/* 不用扩号扩起来 */
#define CALC_FREQ (TIMEBASE_FREQ/1000)  		/* 为了保证足够的计算精度和不溢出，使用该宏 */

extern BOOL g_bLeapSecondFlagHmi; /*有闰秒标志*/
extern uint8_t g_ucPorNLeapSecondHmi; /*正负闰秒标志,01为正闰秒02为负闰秒*/
extern uint32_t g_ulLSUsBeginCnt;       /*闰秒中断来临时的CPU微秒时间*/
extern uint32_t g_ulLSUsEndCnt;       /*闰秒结束时的CPU微秒时间*/
extern EP_DATE_TIME g_tLSDataTime;        /*闰秒时刻的绝对时间，用于判断2个小时用的,不管正负闰秒均从0秒时刻的时间*/
extern uint8_t g_ucHmiLsFlag;      /*HMI 0310 报文传递的标志*/
extern BOOL g_bLeapSecondCpuClearFlag; /*CPU侧是否清除了相关闰秒标志*/
extern uint32_t g_ulLSDataTimeSecCnt;      /*绝对时间的秒计数*/


/* UTC时钟品质定义 */
typedef enum
{
    UTC_Q_TIME_ACCURACY = 0x1F, /* 秒的小数部分的时间精度(最多24位有效位数) */
    UTC_Q_CLOCK_NOT_SYNCHRONIZED = 0x20, /* 时钟未同步 */
    UTC_Q_CLOCK_FAILURE = 0x40, /* 时钟故障 */
    UTC_Q_LEAP_SECONDS_KNOWN = 0x80, /* 闰秒已知 */
} UTC_Q;

/* typedefs */

enum		/* ARIG_B or GPS */
{
    ARIG_B,
    GPS,
};
#ifndef EP_DATE_TIME_STRUCT
#define EP_DATE_TIME_STRUCT
typedef struct		/* 平台时间 */
{
    uint16_t unMicroSec;                /* 0-999 */
    uint16_t unMSEL;                    /* 0-999 */
    uint8_t ucSec;                      /* 0-59 */
    uint8_t ucMinute;                   /* 0-59 */
    uint8_t ucHour;                     /* 0-23 */
    uint8_t ucDate;                     /* 1-31 */
    uint8_t ucWeekDay;                  /* 1～7，other value means don't care */
    uint8_t ucMonth;                    /* 1-12 */
    uint16_t unYear;                    /* 2000～…… */
    uint8_t ucQflag;
    uint8_t ucIrigbLSFlag;          /*闰秒的特殊处理标志*/
} EP_DATE_TIME;
#endif

typedef struct
{
    uint64_t ullusCntFrom1970;
    uint8_t ucQflag;
} US_CNT_UTC_TIME;

/*2013-5-22 ZY */
typedef struct
{
    uint32_t ulSecCntFrom1970;/* 秒数*/
    uint32_t ulUsRemainder; /*整秒外剩余的微秒数,0~99999 */
    uint8_t ucQflag;
} SEC_CNT_UTC_TIME;

typedef struct RUNTIMETAG_tag		/* 程序初始化时间点 */
{
    uint8_t uCounter;

    struct
    {
        uint8_t ProgramPoint[64];
        uint32_t TimePoint;
        int32_t DifTime;
        int32_t TotalTime;
    } TimeTag[MAXTIMETAGNUM];
} RUNTIMETAG;

/* timebase时间 */
typedef struct
{
    uint32_t  ulTimeBaseH;
    uint32_t  ulTimeBaseL;
}   OPT_TIME_BASE;

/* globals */

extern RUNTIMETAG RunTimeTag;
extern uint8_t ucHmiTmQflag; /* HMI设置品质 */

/* global functions */

/***********************************************************************
* TM_Initialize - 初始化整个DATETIME模块
*
* RETURNS: EP_SUCCESS，正常返回
*                 EP_HARD_ERR，硬件出错
*
*/
EP_STATUS TM_Initialize(void);

/* 功能:获取转换US_CNT_UTC_TIME至EP_DATE_TIME结构
 * 参数:
 *      pUsUtctm, 待转换的US_CNT_UTC_TIME结构的指针;
 *      pdttm, 转换后的EP_DATE_TIME结构的指针;
 * 返回:
 *      无.
 */
void Us_UTC_Time_To_Dttm(const US_CNT_UTC_TIME *pUsUtctm, EP_DATE_TIME *pdttm);

/***********************************************************************
* TM_Get_usCnt - 读32位us计数器
*
* RETURNS: 32位us计数器当前值
*
* 注意: 此函数可以在中断的上下文中调用
*              保持计算精度,因为要中断中调用，所以不用浮点数
*
*/
uint32_t TM_Get_usCnt(void);

/* 功能:获取当前时间从1970年开始计算的微妙计数
 * 参数:
 *      无.
 * 返回:
 *      uint64_t, 当前时间从1970年开始计算的微妙计数.
 */
uint64_t TM_Get_usCnt_From1970(void);

/***********************************************************************
* TM_To_usCnt - 转换日期时钟到32位us计数器
*
* RETURNS: 32位us计数器当前值
*
* 注意: 此函数可以在中断的上下文中调用
*
*/
uint32_t TM_To_usCnt(
    const EP_DATE_TIME *pdttm		/* 待转换的日期时间 */
);

/* 转换32位us计数器到日期时钟
 * 参数：
 *      ulMicroSec，待转换的us计数器值
 *      pdttm，存放转换结果
 * 返回值：
 *      无
 * 注意：
 *      此函数可以在中断的上下文中调用 */
EP_STATUS TM_To_Dttm(
    uint32_t ulMicroSec,
    EP_DATE_TIME *pdttm
);

/* Get system time(from GPS or a master station).
 * Parameters:
 *      pdttmNow, structure to save the date/time.
 * Return value:
 *      EP_SUCCESS, successful get the date/time.
 *      EP_LOCAL_MSG, system date/time not checked for long time.
 *      EP_NOT_INIT, system time was never checked from CPU reset.
 *      EP_HARD_ERR, hardware error(such as the crystal not working.
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
EP_STATUS TM_Get_Sys_Time(EP_DATE_TIME *pdttmNow);

EP_STATUS TM_Get_Sys_Us_UTC_Time(US_CNT_UTC_TIME *pusUTCtmNow, uint32_t *pus32Cnt);

/* Set the system time.
 * Parameters:
 *      pdttmSet, date time want to set(week day vlue is not care).
 *      bPecision, flag of if the time is pecision.
 * Return value:
 *      EP_SUCCESS, successful set the clock.
 *      EP_PARM_ERR, input parameter error.
 *      EP_HARD_ERR, hardware error(such as the crystal not working).
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
EP_STATUS TM_Set_Sys_Time(const EP_DATE_TIME *pdttmSet, BOOL bPecision);

/* 与当前时钟对比，得到是否需要设置系统时钟的标志
 * Parameters:
 *      pdttmSet, date time want to set(week day vlue is not care).
 * Return value:
 *      TRUE,需要重新设置时钟.
 *      FALSE,无需重新设置时钟
 * Alert:        */
BOOL TM_Get_Sys_Check_Time_Flag(EP_DATE_TIME *pdttmSet);

/* Set the system time.
 * Parameters:
 *      pdttmSet, date time want to set(week day vlue is not care).
 *      bPecision, flag of if the time is pecision.
 * Return value:
 *      EP_SUCCESS, successful set the clock.
 *      EP_HARD_ERR, hardware error(such as the crystal not working).
 *      EP_SYS_ERR, other unexpected system error.
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
EP_STATUS TM_Set_Sys_Time_Adjust(const EP_DATE_TIME *pdttmSet, BOOL bPecision, uint64_t ulAdjustUs);

/* Caculate the date/time according refrence date/time and delta us(MSEL included)/sec.
 * Parameters:
 *      pdttmRef, data/time recfrence.
 *      pdttmRslt, to save the result.
 *      lMicroSec, delta MSEL/us changed to us.
 *      lDeltaSec, delta second.
 * Return value:
 *      EP_SUCCESS, successful get the date/time.
 *      EP_PARM_ERR, input parameter error.
 * Alert:
 *      Only year from 1980 to 2099 is valid.
 *      Overlap between the refrence and result is allowed. */
EP_STATUS TM_Calc_Time(const EP_DATE_TIME *pdttmRef,
                       EP_DATE_TIME *pdttmRslt, int32_t lMicroSec, int32_t lDeltaSec);

/* Change date/time to a long integer(Seconds from 1980/01/01/0:00'00",
 * MSEL is through away).
 * Parameters:
 *      pdttm, date/time to be converted.
 * Return value:
 *      Seconds from 1980/01/01/0:00'00".
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
uint32_t TM_Time_To_Long(const EP_DATE_TIME *pdttm);

/* Change a long integer(Seconds from 1980/01/01/0:00'00") to date/time.
 * (MSEL is not set).
 * Parameters:
 *      pdttmRslt, result date/time.
 *      ulSec, Seconds from 1980/01/01/0:00'00".
 * Return value:
 *      None.
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
void TM_Long_To_Time(EP_DATE_TIME *pdttmRslt, uint32_t ulSec);

/* Irig_b 、GPS Adjust Time*/

/***********************************************************************
* TM_AdjustTime - 使用B码或秒/分脉冲调整
*
* RETURNS: 无
*
*/
void TM_AdjustTime(
    IRIG_BTime *IRIGTime,
    uint32_t DelayTime,
    uint32_t timeTrans,
    uint8_t GPS_Flag
);

typedef struct
{
    uint64_t ullBaseTimerCnt;           /* BaseTimer Conter on clock check,�����޸� */
    EP_DATE_TIME dttmCheck;             /* date_time on clock check */
    US_CNT_UTC_TIME usUTCtmCheck;
} EP_USER_CLOCK;

/***********************************************************************
* AddRunTimeTag - 增加程序运行时间点
*
* RETURNS: 无
*
*/
void AddRunTimeTag(
    uint8_t *pProgramPoint		/* 程序点 */
);

BOOL Time_Adjust_F_A_1588_Task();

BOOL Check_F_A_1588_Status(uint8_t synMode);

void Dttm_To_UTC_Time(const EP_DATE_TIME *pdttm, MMS_UTC_TIME *pUtctm);

void UTC_Time_To_Dttm(const MMS_UTC_TIME *pUtctm, EP_DATE_TIME *pdttm);

void MMS_UTC_Time_To_US_UTC_TIME(const MMS_UTC_TIME *pUtctm, US_CNT_UTC_TIME *pUsUtctm);

uint32_t Us_UTC_Time_To_us32Cnt(US_CNT_UTC_TIME uttime, BOOL *pbRet);

EP_STATUS TM_Time_To_Us_UTC_Time(const EP_DATE_TIME *pdttm, US_CNT_UTC_TIME *pusutctm);

void Us_UTC_Time_To_MMS_UTC_Time(const US_CNT_UTC_TIME *pUsUtctm, MMS_UTC_TIME *pUtctm);


/*功能：得到系统毫秒计数器  2013-5-25  ZY
* 参数：无
* 返回：毫秒计数器
* 注意: 任务或中断中调用
      仅供内部使用
*/
uint32_t TM_Get_msCnt(void);

/*功能：高效得到系统微秒计数器  2013-5-25  ZY
* 参数：无
* 返回：32微秒计数器
* 注意: 任务或中断中调用
      仅供内部使用
*/
uint32_t TM_High_Get_usCnt(void);


/*功能：高效得到系统毫秒计数器  2013-5-25  ZY
* 参数：无
* 返回：32位毫秒计数器
* 注意: 任务或中断中调用
      仅供内部使用
*/
uint32_t TM_High_Get_msCnt(void);


/*功能：高效得到系统时间  2013-5-25  ZY
* 参数：pdttmNow：返回当前绝对时间变量的地址
                调用方分配变量
* 返回：成功与否
* 注意: 中断和任务中都能调用
      仅供内部使用
*/
EP_STATUS TM_High_Get_Sys_Time(EP_DATE_TIME *pdttmNow);


/*功能：高效得到系统微秒时间和UTC微秒时间  2013-5-25  ZY
* 参数：pusUTCtmNow：返回当前UTC微秒时间变量的地址
                   调用方分配变量
       pus32Cnt：返回当前系统微秒时间变量的地址
                调用方分配变量
* 返回：成功与否
* 注意: 中断或任务中调用
      仅供内部使用
*/
EP_STATUS TM_High_Get_Sys_Us_UTC_Time(US_CNT_UTC_TIME *pusUTCtmNow, uint32_t *pus32Cnt);

/*功能：平台内部周期性更新内部系统时间  2013-5-21  ZY
* 参数：无
* 返回：成功与否
* 注意: 任务中周期性（640ms内，实际设置时控制在100ms左右）调用。
      仅供内部使用
*/
EP_STATUS TM_Updt_Sys_Time();

/*功能：在浮点任务中高效转换内部UTC微秒时间为MMS的标准UTC时间，2013-5-22 ZY
  参数： pUsUtctm，待转换的内部UTC微秒时间变量地址
        pUtctm，转换后的MMS的UTC时间
  返回：无
  注意：只能在浮点任务，不能在中断和非浮点任务中调用
      仅供内部使用*/
void TM_High_Us_UTC_Time_To_MMS_UTC_Time(const US_CNT_UTC_TIME *pUsUtctm, MMS_UTC_TIME *pUtctm);


/*功能：在浮点任务中高效转换MMS的标准UTC时间为内部UTC微秒时间，2013-5-22 ZY
  参数： pUtctm，待转换的MMS的UTC时间
        pUsUtctm，转换后的内部UTC微秒时间变量地址
  返回：无
  注意：只能在浮点任务中调用，不能在中断和非浮点任务中调用
      仅供内部使用*/
void TM_High_MMS_UTC_Time_To_US_UTC_TIME(const MMS_UTC_TIME *pUtctm, US_CNT_UTC_TIME *pUsUtctm);


/*功能：高效转换内部UTC微秒时间为内部32位时间，2013-5-22 ZY
  参数： uttime，待转换的内部UTC微秒时间
        pbRet，转换成功与否变量地址
  返回：转换后的内部32位微秒计数器结果
  注意：可在中断和任务中调用
      仅供内部使用*/
uint32_t TM_High_Us_UTC_Time_To_us32Cnt(US_CNT_UTC_TIME uttime, BOOL *pbRet);

/*TIMEBASE的相减,获得US时间差(准确的) 可正可负
  A是被减数，B是减数 ,2013-5-20 ZY, */
extern int32_t OptGetUsIntvlByBase(OPT_TIME_BASE  *pBaseA,OPT_TIME_BASE  *pBaseB);	/*daibixiang add 因扩展机箱调试移植至此*/

/*根据TIMEBASE的差,获得US时间差  */
extern int32_t   OptGetUsIntvlByBaseDiff(int64_t   llBaseDiff);				/*daibixiang add 因扩展机箱调试移植至此*/

/*TIMEBASE的相减,获得TIMEBASE时间差  ,*/
extern int64_t   OptGetBaseDiff(OPT_TIME_BASE  *pBaseA,OPT_TIME_BASE  *pBaseB);	/*daibixiang add 因扩展机箱调试移植至此*/

/* 获取时间品质 */
extern uint8_t GetSysTimeQFlag();

/* 获取HMI传递过来的对时品质
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
extern uint8_t TM_GetHmiTimeQFlag(void);


/*计算两个绝对时间之间的时间差值，返回秒
只算小时差，不考虑小时往上的差值*/
extern int SYN_GetSubSec( EP_DATE_TIME *pDttmDst, EP_DATE_TIME *pDttmSrc );


/*拷贝绝对日期时间*/
extern void SYN_CpyDttm( EP_DATE_TIME *pDttmDst, EP_DATE_TIME *pDttmSrc );

/*计算两个绝对时间之间的时间差值，返回微秒
只算小时差，不考虑小时往上的差值*/
extern int64_t SYN_GetSubMicroSec( EP_DATE_TIME *pDttmDst, EP_DATE_TIME *pDttmSrc );


/*清除闰秒标志*/
extern void SYN_ClearLsFlag();

/*闰秒标志是否清除*/
extern BOOL SYN_IsLsFlagClear();

/*设置进入闰秒处理特殊标志*/
extern void SYN_SetLsSpecialFlag();

/*得到B码的标志，与规约一致
比特	值0时的含义	值1时的含义
0	    表示该事件的时间的0秒不是正闰秒的第60秒	表示该事件的时间的0秒是正闰秒的第60秒
1	    表示该事件的时间不是发生在正闰秒的60秒开始以后	表示该事件的时间是发生在正闰秒的60秒开始以后
2	    表示该事件的时间不是发生在负闰秒产生的0秒开始以后	表示该事件的时间是发生在负闰秒产生的0秒开始以后
目标时刻与0秒时刻比较
绝对时间用于比较是否前一秒是60s，是否是闰秒发生的2小时以内的时间
*/
extern BOOL SYN_SetIrigBFlag(uint32_t ulMicroSec, EP_DATE_TIME *pDttmDst);

/* 获取秒脉冲状态
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL TM_GetSecPluseSts(void);

/* 设置秒脉冲状态
 * Para:
 *     bSecPulseFlag, 脉冲状态.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL TM_SetSecPluseSts(BOOL bSecPulseFlag);

 #define LW_SYMBOL_EXPORT __attribute__((visibility("default")))

#ifdef	__cplusplus
}
#endif

#endif                                  /* DATETIME_H */
