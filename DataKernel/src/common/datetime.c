
/* datetime.c - CM - System bottom functions(DateTime). */

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
INCLUDES: datetime.h
*/

/* includes */
#ifndef RTT_BUILD
#include <taskLib.h>
#include <tickLib.h>
#endif
#include "datetime.h"
#include "smvcfg.h"
// #include "HPC_SysTime.h"
// #include "HW_SysFreq.h"
// #include "sacDev.h"
#include "limits_compat.h"
#include "time_compat.h"
// #include "SYN_Init.h"

#define IRIGB_LOOP_TASK_DELAY_CNT 10 /**< IrigB_Loop����ѭ��ִ��ʱÿ��taskDelay�ȴ����� */
/** ��ʱ����ʱ���쳣����ʱ�ȴ�ʱ�䣨2�룩,�ȴ�ʱ���ڲ�ʹ�ö�ʱ���ߵ�ʱ�� */
#define TIME_ERR_DELAY_SEC_CNT 2000/(IRIGB_LOOP_TASK_DELAY_CNT*10)

extern BOOL bSNTP60SecFlag;/*����������Ϊ60s��*/

/* statics */

static const uint16_t aunMonthDay[]=
{0,0,31,59,90,120,151,181,212,243,273,304,334};
static const uint16_t aunLeapMonthDay[]=
{0,0,31,60,91,121,152,182,213,244,274,305,335};

/* defines */

/* typedefs */

/*�ڲ�ʱ�Ӹ��������׼ 2013-5-25 ZY */
typedef struct
{
    uint32_t ulBaseLBench;         /*Timebase�ĵ�32λ��׼ʱ�� */
    uint32_t ulBaseHBench;         /*Timebase�ĸ�32λ��׼ʱ�� */
    uint32_t ulUsCntBench;        /*�ڲ�32λ΢���������׼ʱ��  */
    uint32_t ulMsCntBench;        /*�ڲ�32λ�����������׼ʱ��  */
    EP_DATE_TIME dttmBench;       /* ����ʱ���׼ʱ�� */
    US_CNT_UTC_TIME usUTCtmBench;   /*UTC-64λ΢���������׼ʱ�� */
    SEC_CNT_UTC_TIME   secUTCtmBench;  /*MMSUTC-��+΢���������׼ʱ��*/
} EP_CLOCK_HIGH_BENCH;


typedef struct
{
    uint32_t usTime;
    uint32_t ulSysUs;
    EP_DATE_TIME Time;
    uint16_t unCh;
} SOETIME, *pSOETIME;

/* statics */

static uint64_t ullTimebasePeriod_ns_g;   /* ��Timebase�����ڣ���nsΪ��λ */

static BOOL bNewSecFlag = FALSE;    /*�µ�һ�����������ڸ߱��洫��������CPUͬʱ����*/
static BOOL s_bEnableSetNewSec = FALSE;    /*�Ƿ����������µ�һ�뵽����ʶ*/

BOOL nbHmiSetQFlag = FALSE;   /* HMI�Ƿ�ͨ���ڲ�ͨѶ���ö�ʱ״̬��־ */
static BOOL s_SetSynChgFlag = FALSE; /**<  ��ֵ��Чͬ���źű仯��־ */
static BOOL s_bTimerIsIrigb = TRUE; /**<  ��ʱ��ʽ�Ƿ�ΪB�룬TRUE��B���ʱ��FALSE����B���ʱ */

/* globals */

EP_USER_CLOCK clkSys;
static EP_CLOCK_HIGH_BENCH  clkBench; /*2013-5-25�� ZY */
BOOL unTrustSysTime=0;
int32_t epstatus;
int RegBaseAddr = 0;
RUNTIMETAG RunTimeTag;

//uint32_t ulSysUs;		/* ϵͳusʱ�� */
uint64_t ulAdjustUs;

uint32_t gpsModeTimeInterval = 0xFFFFFF ; 		/* ��ʼ����Ϊһ��ֵ����ֹ����տ�ʼʱGPS��ʱ����ʾ���� */
uint32_t ulGpsTimeIntervalforRpt=0; 		/* ��ֹ��һ�ζ�ʱ���������� */

BOOL bGetAbsTime = FALSE; /* ��ȡ����ʱ���־�Ƿ����� */
uint32_t GetAbsTimeInterval = 0xFFFFFF ;
BOOL bGetPulseSetTime = FALSE; /* �������Ƿ����� */
uint32_t GetPulseSetTimeInterval = 0xFFFFFF ;

BOOL g_bMMITimeValid = FALSE;  	/* MMI�Ķ�ʱʱ���Ƿ���Ч(�ⲿ�Ƿ��ж�ʱ), Ĭ����Ч */
BOOL bIrigBSeries = FALSE;
BOOL bIntSetTimeOK=FALSE;
BOOL bTimeResetFlag=TRUE;  /*ʱ��״̬�����ʶ*/
uint32_t irigbModeTimeInterval =0xFFFFFF;
BOOL bTaskSetTimeOK=FALSE;
int iAdjustType_g;		/* ȫ�ֶ�ʱģʽ */
BOOL bTimeSyncOK=FALSE;
BOOL bSynIntSts = FALSE; /*��ʱ�ź�״̬ TRUE�����쳣���� FALSE��������*/
BOOL bSynServSts = FALSE; /*��ʱ����״̬ TRUE�����쳣���� FALSE��������*/
BOOL bTimeLeapSts = FALSE; /* �Ƿ����� TRUE�����쳣���� FALSE��������*/

uint8_t ucPulseType_g=GPS_PULSE_TYPE_INVALID;
uint8_t ucHmiTmQflag = 0;

uint32_t ulCurSecFrom1980=0;
extern BOOL bSysSecSynFlag;

/*����������־������HMI���ձ�־�ͷ���*/
BOOL g_bLeapSecondFlagHmi = FALSE; /*�������־*/
uint8_t g_ucPorNLeapSecondHmi = 0; /*���������־,01Ϊ������02Ϊ������*/

/*CPU���Ƿ��������������־,�������ʱ���ѻָ�����
Ŀǰ����;�������
45��֮���Զ����,
45��֮ǰ��HMI�����������ʱ�϶��Ǽ���ʱ����������ʵ��*/
BOOL g_bLeapSecondCpuClearFlag = TRUE;

uint32_t g_ulLSUsBeginCnt = 0;       /*�����ж�����ʱ��CPU΢��ʱ��*/
uint32_t g_ulLSUsEndCnt = 0;       /*�������ʱ��CPU΢��ʱ��*/
EP_DATE_TIME g_tLSDataTime;        /*�������0��͸������0��. (ע��: �洢����1���59��)����ʱ�̵ľ���ʱ�䣬�����ж�2��Сʱ�õ�,���������������0��ʱ�̵�ʱ��*/
uint32_t g_ulLSDataTimeSecCnt = 0;      /*����ʱ��������*/
BOOL g_bRecoverLsTime = FALSE;      /*�Ƿ��Ѿ��ָ�����ʱ��*/

uint8_t g_ucHmiLsFlag = 0;      /*HMI 0310 ���Ĵ��ݵı�־*/
BOOL g_bIrigbLog = FALSE;           /*B���ӡ��Ϣ��־*/

extern uint32_t g_ulPollSetChgCnt; /* ��ѯ���¼��� */
extern BOOL g_bIrigBFpgaMode;  /* CPU B���ʱ��FPGA�������� */

#ifndef EDP01_CA_EXT_BUILD
/* ��ʱ״̬ */
extern BOOL g_TimeSynIntSts; /* �ӿ�״̬ */
extern BOOL g_TimeServeSts; /* ����״̬ */
extern BOOL g_TimeLeapSts; /* ����״̬ */
#endif

/*��1�θ���ʱ���׼��־ 201-5-27  ZY */
static  BOOL  bFstUpdtTimeBenchFlag_s=FALSE;
static BOOL s_bSecPulseFlag = FALSE; /* �������ʶ */
static BOOL s_bEableSetSecPulse = FALSE; /* ���������������ʶ */

/* forward declarations */

static uint8_t TimeZone_g=20;
//�ص�����ָ���������
typedef int (*RECALL_FUNCTION_PTR)(int arg1,int arg2,int arg3,int arg4,int arg5);
static int GetTimeZoneInitFunc();
static void TimeZoneInit();
RECALL_FUNCTION_PTR GetTimeZoneCallback=GetTimeZoneInitFunc;

int32_t g_iDatetimeDevice; /* Datetime �豸��� */
int32_t g_iIrigbDevice; /* IRIGB �豸��� */

uint8_t quality_time=0x0a;/*ʱ�Ӿ�ȷ��*/
extern BOOL ClockNotSynchronized;/*ʱ��δͬ����*/
extern BOOL g_bConnectHmi;  /* CPU�Ƿ���Ҫ��HMIͨѶ */
INT32 g_iSynInnerDevice;                  /*δ����  IRIGB_DEV_S�豸1,ĸ���ʱ�豸,���ڲ������ĸ���ʱ���ߴ���Ķ�ʱ�ź�*/
#define BSP_IRIGB_INNER_NAME "inner"      /*inner�豸*/
BOOL IRIGB_PLS_Flag = 0; /*�������־*/
BOOL IRIGB_NLS_Flag = 0; /*�������־*/
#define IRIGB_SYN_OKSTATUE   0x4         /*B���ʱ����OK��ʶ*/

// IRIG_BTime_N g_Innertime;   /*��ʱ���ߵĽ���ʱ��*/

/*******************************************************************************
 �������� :HW_SYN_Init_M
 ����˵�� :ĸ���ʱ��ʼ����ں���
 ����˵�� :��
 ����˵�� :NP_STATUS:���س�ʼ������Ƿ���ȷ
 �޸ļ�¼ :2015/04/20:kevin Create
*******************************************************************************/
// extern NP_STATUS HW_SYN_Init_M();

/***********************************************************************
* SetAdjustTimeSuccessFlag - ����ϵͳ��ʱ��־
*
* RETURNS: ��
*
*/
extern void SetAdjustTimeSuccessFlag(
    BOOL bOkFlag
);


/***********************************************************************
* BCD_TO_HEX - BCD to HEX
*
* RETURNS: ת�����
*
*/
extern uint8_t BCD_TO_HEX (
    uint8_t bcd		/* BCD��*/
);

/***********************************************************************
* GPS_ISR - GPS�ж�
*
* RETURNS: ��
*
*/
void GPS_ISR();


void Time_Adjust_F_A_1588_ISR (void);

/***********************************************************************
* TM_Get_SecCnt - ��õ�ǰ��������Ĵ�С
*
* RETURNS: �����ֵ
*
*/
uint32_t TM_Get_SecCnt(void);

/* IRIGB �豸�ص�����.
 * Para:
 *     pIrigbInfo: B ����Ϣ.
 * Return:
 *     void.
 */
// void TM_Irigbhook(IRIGB_INFO_S *pIrigbInfo);

/* IRIGB �豸�ص�����.(��FPGA����)
 * Para:
 *     pIrigbInfo: B ����Ϣ.
 * Return:
 *     void.
 */
// void TM_Irigbhook_Fpga(IRIGB_INFO_S *pIrigbInfo);

/***********************************************************************
* TM_Get_BaseTimerCnt - ���baseTimer��С
*
* RETURNS: 64λ������ֵ
*
*/
static uint64_t TM_Get_BaseTimerCnt(void);

/***********************************************************************
* TM_Trans_Base_Dif - ת��TIMEBase��deltaΪ΢�����delta,�����ɸ�
*
* RETURNS: �ɹ�, EP_SUCCESS
*                 ����, EP_ERRROR
*
*/
static EP_STATUS TM_Trans_Base_Dif(
    int64_t llBaseDif,		/* BASE��delta */
    int32_t *plSecDif,				/* ����SEC��delta */
    int32_t *plUsDif			/* ����Us��delta */
);

LW_SYMBOL_EXPORT uint8_t GetSysTimeQFlag()
{//直接使用linux系统提供的函数，此处的时间品质则由clock_gettime是否工作正常获取
    //todo 之后视情况添加ntp检测
    uint8_t ucQflag=0;


    struct timespec ts;
    if(clock_gettime(CLOCK_REALTIME, &ts) != 0)
    {
        // 获取系统时间失败，标记不可靠
        ucQflag |= UTC_Q_CLOCK_NOT_SYNCHRONIZED;
    }
    else
    {
        // 时间成功获取，标记可靠
        ucQflag &= ~UTC_Q_CLOCK_NOT_SYNCHRONIZED;
    }

    return ucQflag;

//     if(g_bConnectHmi)
//     {
//         if(nbHmiSetQFlag)
//         {
//             /* ���ڲ�ͨѶ������ʹ��HMI�·��Ķ�ʱ״̬��Ϊʱ��Ʒ����Դ ��ѹû�������岻�ж���ر���*/
//             if (((g_eBdLevelType == Board_LEVEL_03) || (bGetPulseSetTime&&bGetAbsTime)) && g_bMMITimeValid)
//             {
//                 ucQflag&=~(UTC_Q_CLOCK_NOT_SYNCHRONIZED);
//             }
//             else
//             {
//                 ucQflag|=UTC_Q_CLOCK_NOT_SYNCHRONIZED;
//             }
//         }
//         else
//         {
//             if(0 == IRIGBStatus(g_iIrigbDevice))
//             {
//                 ucQflag&=~(UTC_Q_CLOCK_NOT_SYNCHRONIZED);
//             }
//             else
//             {
//                 ucQflag|=UTC_Q_CLOCK_NOT_SYNCHRONIZED;
//             }
//         }

//         ucQflag |= ucHmiTmQflag;
//     }
//     else
//     {
//         /* �ڲ�ͨѶ����ǰ,ʹ��FPGA��ͬ��״̬��Ϊʱ��Ʒ����Դ,���װ���ϵ�GOOSE��һ֡ʱ��Ʒ������ */
// #if defined(SylixOS_BUILD)||defined(RTT_BUILD)
//         if(SYN_GetIrigBOKFlag())
//         {
//             ucQflag&=~(UTC_Q_CLOCK_NOT_SYNCHRONIZED);
//         }
//         else
//         {
//             ucQflag|=UTC_Q_CLOCK_NOT_SYNCHRONIZED;
//         }
// #else
//         if(0 == IRIGBStatus(g_iIrigbDevice))
//         {
//             ucQflag&=~(UTC_Q_CLOCK_NOT_SYNCHRONIZED);
//         }
//         else
//         {
//             ucQflag|=UTC_Q_CLOCK_NOT_SYNCHRONIZED;
//         }
// #endif
//         ucQflag |= quality_time;
//     }

    // return ucQflag;
}

/* ��ȡHMI���ݹ����Ķ�ʱƷ��
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
uint8_t TM_GetHmiTimeQFlag(void)
{
    return ucHmiTmQflag;
}

/***********************************************************************
* int_link - set interrupt vetor
*
* RETURNS: ��
*
*/
void int_link(void)
{
    INT32 ret = 0;

//     if(!g_bIrigBFpgaMode)
//     {
//         if(TM_InitIrigbDev()==EP_SUCCESS)
//         {
//             ret = IRIGBHook(g_iIrigbDevice,TM_Irigbhook);
//             if(ret != 0)
//             {
//                 logMsg("IRIGB �豸�ص������ҽ�ʧ��ret=%d \n",ret,0,0,0,0,0);
//             }
//         }
//     }
//     else
//     {
//         /* �����������ն˶�ʱ�ɶ�ʱ���ߴ��� */
//         if(TM_InitIrigbDev()==EP_SUCCESS)
//         {
// #if !defined(SylixOS_BUILD)&&!defined(RTT_BUILD)
//             ret = IRIGBHook(g_iIrigbDevice,TM_Irigbhook_Fpga);
//             if(ret != 0)
//             {
//                 logMsg("IRIGB �豸�ص������ҽ�ʧ��ret=%d \n",ret,0,0,0,0,0);
//             }
// #endif
//         }
//     }
}

/*������������ʱ��*/
void SYN_CpyDttm( EP_DATE_TIME *pDttmDst, EP_DATE_TIME *pDttmSrc )
{
    if(pDttmSrc == NULL ||pDttmDst == NULL)
        return;

    pDttmDst->unYear = pDttmSrc->unYear;
    pDttmDst->ucMonth = pDttmSrc->ucMonth;
    pDttmDst->ucWeekDay = pDttmSrc->ucWeekDay;
    pDttmDst->ucDate = pDttmSrc->ucDate;
    pDttmDst->ucHour = pDttmSrc->ucHour;
    pDttmDst->ucMinute = pDttmSrc->ucMinute;
    pDttmDst->ucSec = pDttmSrc->ucSec;
    pDttmDst->unMSEL = pDttmSrc->unMSEL;
    pDttmDst->unMicroSec = pDttmSrc->unMicroSec;
    pDttmDst->ucQflag = pDttmSrc->ucQflag;
    pDttmDst->ucIrigbLSFlag = pDttmSrc->ucIrigbLSFlag;
}

/*������������ʱ��֮���ʱ���ֵ��������
ֻ��Сʱ�������Сʱ���ϵĲ�ֵ*/
int SYN_GetSubSec( EP_DATE_TIME *pDttmDst, EP_DATE_TIME *pDttmSrc )
{
    uint32_t secDst = 0;
    uint32_t secSrc = 0;
    int res = 0;
    if(pDttmSrc == NULL ||pDttmDst == NULL)
        return 0;

    secDst = TM_Time_To_Long(pDttmDst);
    secSrc = TM_Time_To_Long(pDttmSrc);
    res = secDst - secSrc;
    return res;
}

/*������������ʱ��֮���ʱ���ֵ�����غ���
ֻ��Сʱ�������Сʱ���ϵĲ�ֵ*/
int SYN_GetSubMSec( EP_DATE_TIME *pDttmDst, EP_DATE_TIME *pDttmSrc )
{
    int msec = 0;
    if(pDttmSrc == NULL ||pDttmDst == NULL)
        return 0;

    msec = SYN_GetSubSec(pDttmDst, pDttmSrc) * 1000 + pDttmDst->unMSEL - pDttmSrc->unMSEL;
    return msec;
}

/*������������ʱ��֮���ʱ���ֵ������΢��
ֻ��Сʱ�������Сʱ���ϵĲ�ֵ*/
int64_t SYN_GetSubMicroSec( EP_DATE_TIME *pDttmDst, EP_DATE_TIME *pDttmSrc )
{
    int64_t microsec = 0;
    if(pDttmSrc == NULL ||pDttmDst == NULL)
        return 0;

    microsec = SYN_GetSubMSec(pDttmDst, pDttmSrc) * 1000 + pDttmDst->unMicroSec - pDttmSrc->unMicroSec;
    return microsec;
}

/*�õ�B��ı�־�����Լһ��
����	ֵ0ʱ�ĺ���	ֵ1ʱ�ĺ���
0	    ��ʾ���¼���ʱ���0�벻��������ĵ�60��	��ʾ���¼���ʱ���0����������ĵ�60��
1	    ��ʾ���¼���ʱ�䲻�Ƿ������������60�뿪ʼ�Ժ�	��ʾ���¼���ʱ���Ƿ������������60�뿪ʼ�Ժ�
2	    ��ʾ���¼���ʱ�䲻�Ƿ����ڸ����������0�뿪ʼ�Ժ�	��ʾ���¼���ʱ���Ƿ����ڸ����������0�뿪ʼ�Ժ�
Ŀ��ʱ����0��ʱ�̱Ƚ�
����ʱ�����ڱȽ��Ƿ�ǰһ����60s���Ƿ������뷢����2Сʱ���ڵ�ʱ��
*/
BOOL SYN_SetIrigBFlag(uint32_t ulMicroSec, EP_DATE_TIME *pDttmDst)
{
    uint32_t ulSec = 0;
    BOOL bClkAfterLS = FALSE;
    uint32_t secDst = 0;
    uint32_t secSrc = 0;
    if(pDttmDst == NULL)
        return FALSE;

    pDttmDst->ucIrigbLSFlag = 0;

    if(SYN_IsLsFlagClear())
    {
        return FALSE;
    }
    
    /*�����жϻ�׼ʱ��ʵ������ǰ���������
    ��׼ʱ��ʵ����������������ʱ�䲻��-1��*/
    if((SYN_GetSubSec(&(clkSys.dttmCheck), &g_tLSDataTime) > 0) && g_tLSDataTime.unYear != 0)
    {
        bClkAfterLS = TRUE;
    }

    /*���ж��Ƿ����������60s�������д������־*/
    /*����Ҫ����ʱ�������*/
    if(IRIGB_PLS_Flag)
    {
        secDst = TM_Time_To_Long(pDttmDst);
        secSrc = TM_Time_To_Long(&g_tLSDataTime);
        /*��Ϊ΢�����ᷴת������Ҫ���Ͼ���ʱ����ж�
        ���üӾ���ʱ����жϣ����ж�������֮��ı�־λ����,
        �����п����¼�����ʱ��û��g_tLSDataTime��ʱ��
        g_tLSDataTime ��1��ʱ�̵�ʱ��,��������-1
        */

        if((secDst >secSrc) && (secDst-secSrc) < IRIGB_SPECIAL_SEC && g_ulLSUsBeginCnt != 0 && g_ulLSUsEndCnt != 0)

        {
            if(!bClkAfterLS)
            {
                ulSec = secDst - 1;
                TM_Long_To_Time(pDttmDst, ulSec);
                pDttmDst->ucIrigbLSFlag |= IRIGB_PLS_TIME_ADJUST;
            }
            pDttmDst->ucIrigbLSFlag |= IRIGB_AFTER_PLS_0SEC;
        }
        else if((ulMicroSec > g_ulLSUsBeginCnt && g_ulLSUsBeginCnt != 0)
                || (ulMicroSec > g_ulLSUsBeginCnt && ulMicroSec < g_ulLSUsEndCnt && g_ulLSUsBeginCnt != 0 && g_ulLSUsEndCnt != 0))
        {
            pDttmDst->ucIrigbLSFlag |= IRIGB_PLS_60SEC;
        }
        else
        {
            pDttmDst->ucIrigbLSFlag = 0;
        }
    }
    else if(IRIGB_NLS_Flag)
    {
        secDst = TM_Time_To_Long(pDttmDst);
        secSrc = TM_Time_To_Long(&g_tLSDataTime);

        if((secDst >secSrc)  && ((secDst-secSrc) < IRIGB_SPECIAL_SEC) && g_ulLSUsBeginCnt != 0)
        {
            if(!bClkAfterLS)
            {
                ulSec = secDst + 1;
                TM_Long_To_Time(pDttmDst, ulSec);
                pDttmDst->ucIrigbLSFlag |= IRIGB_NLS_TIME_ADJUST;
            }
            pDttmDst->ucIrigbLSFlag |= IRIGB_AFTER_NLS_0SEC;
        }
        else
        {
            pDttmDst->ucIrigbLSFlag = 0;
        }
    }

    return TRUE;
}

/*��������־*/
void SYN_ClearLsFlag()
{
    SYN_LOG("!!!!!!!!!!!!!!ȡ��CPU�������־. \n", 0,0,0,0,0,0);
    g_ulLSUsBeginCnt = 0;
    g_ulLSUsEndCnt = 0;
    g_bRecoverLsTime = FALSE;
    g_tLSDataTime.unYear = 0;
    g_tLSDataTime.ucMonth = 0;
    g_tLSDataTime.ucWeekDay = 0;
    g_tLSDataTime.ucDate = 0;
    g_tLSDataTime.ucHour = 0;
    g_tLSDataTime.ucMinute = 0;
    g_tLSDataTime.ucSec = 0;
    g_tLSDataTime.unMSEL = 0;
    g_tLSDataTime.unMicroSec = 0;
    g_tLSDataTime.ucQflag = 0;
    g_tLSDataTime.ucIrigbLSFlag = 0;
    g_bLeapSecondCpuClearFlag = TRUE;
    g_bLeapSecondFlagHmi = FALSE;
    g_ucPorNLeapSecondHmi = 0;
}

/*�����־�Ƿ����*/
BOOL SYN_IsLsFlagClear()
{
    return g_bLeapSecondCpuClearFlag;
}

/*���ý������봦�������־*/
void SYN_SetLsSpecialFlag()
{
    g_bLeapSecondCpuClearFlag = FALSE;
}

/***********************************************************************
* GPS_ISR - GPS interrupt service program
*
* RETURNS: ��
*
*/
void GPS_ISR(void)
{
    EP_DATE_TIME dttm;
    EP_STATUS retcode;
    uint32_t ulSec = 0;
    static uint32_t ulLastUsCnt_s=0;
    static uint8_t ucPulseType=GPS_PULSE_TYPE_INVALID;
    uint32_t ulCurUsCnt,ulDeltaUsCnt;

    // /** NPSӲ��������������ص��������ʶ��������֤����CPUͬ�������������ڶ�ʱ */
    // if(g_eBdFrameType == Board_FRAME_NPS)
    // {
    //     if(GetTimerIsIrigb()) /**< B���ʱʱҪ�ж�ֵ��Чͬ���źű�λ״̬ */
    //     {
    //         /* ����s�����ʶ */
    //         if(s_bEableSetSecPulse)
    //         {
    //             s_bEableSetSecPulse = FALSE;
    //             TM_SetSecPluseSts(TRUE);
    //         }
            
    //         bNewSecFlag = TRUE;
    //     }
    //     else /**< ��B���ʱʱ��ȡ������ֵ��Чͬ���źţ���������������ͬ�� */
    //     {
    //         /* ����s�����ʶ */
    //         TM_SetSecPluseSts(TRUE);
    //         bNewSecFlag = TRUE;
    //     }
    //     return;
    // }

    // ulCurUsCnt=TM_Get_usCnt();

    // /* ����s�����ʶ */
    // TM_SetSecPluseSts(TRUE);

    // ulDeltaUsCnt=ulCurUsCnt-ulLastUsCnt_s;

    // ulLastUsCnt_s=ulCurUsCnt;

    // if(ulDeltaUsCnt>600000&&ulDeltaUsCnt<1400000)
    // {
    //     ucPulseType=GPS_PULSE_TYPE_SEC;
    //     ucPulseType_g=GPS_PULSE_TYPE_SEC;
    // }
    // else if(ulDeltaUsCnt>59000000&&ulDeltaUsCnt<61000000)
    // {
    //     ucPulseType=GPS_PULSE_TYPE_MIN;
    //     ucPulseType_g=GPS_PULSE_TYPE_MIN;
    // }
    // else
    // {
    //     return;
    // }

    // retcode=TM_Get_Sys_Time(&dttm);
    // if(dttm.unMSEL>500)
    // {
    //     /*SYN_LOG("1111111  ʱ�������:%d-%d-%d-%d    us: %d \n",
    //         dttm.ucHour,dttm.ucMinute,dttm.ucSec,dttm.unMSEL,TM_Get_usCnt(),0);*/
    //     ulCurSecFrom1980 = TM_Time_To_Long(&dttm) + 1;
    //     ulSec=TM_Time_To_Long(&dttm)+1;
    //     TM_Long_To_Time(&dttm,ulSec);
    // }
    // else
    // {
    //     ulCurSecFrom1980=TM_Time_To_Long(&dttm);
    // }


    // /*�����봦��*/
    // if(g_bLeapSecondFlagHmi)
    // {
    //     /*��������*/
    //     if(g_ucPorNLeapSecondHmi == IRIGB_PLS)
    //     {
    //         /*����������60*/
    //         if(dttm.ucSec == 0)
    //         {
    //             SYN_LOG("kevin GPS_ISR: ������60������.ʱ-��-��-����: %d-%d-%d-%d\n",
    //                     dttm.ucHour,dttm.ucMinute,dttm.ucSec,dttm.unMSEL,0,0);
    //             g_ulLSUsBeginCnt = TM_Get_usCnt();
    //         }
    //         /*����������������ʱ��, ����-1����,
    //         ���Ĵ�1�뿪ʼ�Ļ�׼����,�����ľͲ���Ҫ��*/
    //         else if(dttm.ucSec == 1)
    //         {
    //             SYN_LOG("kevin GPS_ISR: �������0������.ʱ-��-��-����: %d-%d-%d-%d\n",
    //                     dttm.ucHour,dttm.ucMinute,dttm.ucSec,dttm.unMSEL,0,0);
    //             g_ulLSUsEndCnt = TM_Get_usCnt();
    //             SYN_CpyDttm(&g_tLSDataTime, &dttm);     /*�˿���1s*/
    //             g_ulLSDataTimeSecCnt = TM_Time_To_Long(&dttm);
    //             /*ulCurSecFrom1980 = ulCurSecFrom1980 - 1;
    //             TM_Long_To_Time(&dttm,ulCurSecFrom1980);*/
    //         }
    //         else if(dttm.ucSec == (IRIGB_SPECIAL_SEC + 1) && !g_bRecoverLsTime && dttm.ucMinute == 0)   /*�˴���HMI��ϣ�ֻ��-1s һ��*/
    //         {
    //             ulSec=TM_Time_To_Long(&dttm) - 1;
    //             TM_Long_To_Time(&dttm,ulSec);
    //             g_bRecoverLsTime = TRUE;
    //             SYN_LOG("#### kevin GPS_ISR: ʱ��Ӧ��-1��.ʱ-��-��-����: %d-%d-%d-%d\n",
    //                     dttm.ucHour,dttm.ucMinute,dttm.ucSec,dttm.unMSEL,0,0);
    //         }
    //     }
    //     /*�и�����*/
    //     else if(g_ucPorNLeapSecondHmi == IRIGB_NLS)
    //     {
    //         /*���ø�����0����Ծ��59,���಻��Ҫ����*/
    //         if(dttm.ucSec == 59)
    //         {
    //             SYN_LOG("kevin GPS_ISR: ������0������.ʱ-��-��-����: %d-%d-%d-%d\n",
    //                     dttm.ucHour,dttm.ucMinute,dttm.ucSec,dttm.unMSEL,0,0);
    //             g_ulLSUsBeginCnt = TM_Get_usCnt();
    //             SYN_CpyDttm(&g_tLSDataTime, &dttm);     /*�˿���59s*/
    //             g_ulLSDataTimeSecCnt = TM_Time_To_Long(&dttm);
    //             /*ulCurSecFrom1980 = ulCurSecFrom1980 + 1;
    //             TM_Long_To_Time(&dttm,ulCurSecFrom1980);*/
    //         }
    //         else if(dttm.ucSec == (IRIGB_SPECIAL_SEC - 1) && !g_bRecoverLsTime && dttm.ucMinute == 0)  /*�˴���HMI��ϣ�ֻ��+1s һ��*/
    //         {
    //             ulSec=TM_Time_To_Long(&dttm) + 1;
    //             TM_Long_To_Time(&dttm,ulSec);
    //             g_bRecoverLsTime = TRUE;
    //             SYN_LOG("####  kevin GPS_ISR: ʱ��Ӧ��+1��.ʱ-��-��-����: %d-%d-%d-%d\n",
    //                     dttm.ucHour,dttm.ucMinute,dttm.ucSec,dttm.unMSEL,0,0);
    //         }
    //     }
    // }

    // if(ucPulseType==GPS_PULSE_TYPE_MIN)
    //     dttm.ucSec=0;        /*������Ҫ����*/
    // dttm.unMSEL=0;
    // dttm.unMicroSec=0;

    // /*����ǰ��55������һ�ֵ�45�룬�����»�׼�������1ms
    // �˴���Χ�д�һ��,+2, �� g_bRecoverLsTime �������ж�*/
    // ulSec=TM_Time_To_Long(&dttm);
    // if((!((g_bLeapSecondFlagHmi && dttm.ucSec > 55 && dttm.ucMinute == 59)
    //         ||(g_bLeapSecondFlagHmi && dttm.ucSec < (IRIGB_SPECIAL_SEC+2) && dttm.ucMinute == 0)))
    //         || g_bRecoverLsTime)
    // {
    //     /*SYN_LOG("GPS_ISR: ����ʱ��.  ʱ-��-��-����: %d-%d-%d-%d  g_bLeapSecondFlagHmi:%d.  g_bRecoverLsTime:%d \n",
    //         dttm.ucHour,dttm.ucMinute,dttm.ucSec,dttm.unMSEL,g_bLeapSecondFlagHmi,g_bRecoverLsTime);*/
    //     TM_Set_Sys_Time(&dttm, TRUE);
    //     /*�������봦��������Ҫ������ʱ��ָ�֮��
    //     �粻��g_bRecoverLsTime���ж����������� 59��40��֮��ͻ᲻ͣ���������������*/
    //     /*if(!SYN_IsLsFlagClear() && !(g_bLeapSecondFlagHmi && dttm.ucMinute == 59))*/
    //     if(!SYN_IsLsFlagClear() && g_bRecoverLsTime)
    //     {
    //         /*SYN_LOG("GPS_ISR: CPU����������־ \n",0,0,0,0,0,0);*/
    //         SYN_ClearLsFlag();
    //     }
    // }

    // bNewSecFlag = TRUE;

    // GetPulseSetTimeInterval=tickGet();
}

/***********************************************************************
* GetSysSecFrom1980 - ��ȡϵͳʱ���ʽΪ1980��������������������������ʱ
*
* RETURNS: uint32_t Seconds from 1980
*
*/
uint32_t GetSysSecFrom1980(void)
{
    return ulCurSecFrom1980;
}

/***********************************************************************
* TM_Initialize - ��ʼ��EDP01 EDP02 GPS��ʱ�ж�
*
* RETURNS:
*         EP_SUCCESS����������
*         EP_HARD_ERR��Ӳ������
*
*/
EP_STATUS TM_Initialize(void)
{
//     if(g_eBdLevelType == Board_LEVEL_03)
//     {
//         TimeZoneInit();
//     }

//     /* ����չ���� */
// #ifndef EDP01_CA_EXT_BUILD
//     if((EP_GetSynMode()==EXTERNAL_1588_0_ADJ)
//             ||(EP_GetSynMode()==EXTERNAL_1588_1_ADJ))
//     {
//         /* ɾ��HARD 1588��ʼ�� */
//     }
//     else if((EP_GetSynMode()==EXTERNAL_1588_2_ADJ)
//             ||(EP_GetSynMode()==EXTERNAL_1588_3_ADJ))
//     {
//         /* ɾ��FPGA 1588��ʼ�� */
//     }
// #endif

//     ullTimebasePeriod_ns_g = (uint64_t)1000000000/((uint64_t)TIMEBASE_FREQ);
//     ucHmiTmQflag = 0x0A;

//     /* ��ѹ����ʹ�� */
//     if(g_eBdLevelType != Board_LEVEL_03 )
//     {
//         /* �����ж����� */
//         int_link();
//     }

    return EP_SUCCESS;
}

/***********************************************************************
* TM_Get_usCnt - ��32λus������
*
* RETURNS: 32λus��������ǰֵ
*
* ע��: �˺����������жϵ��������е���
*              ���ּ��㾫��,��ΪҪ�ж��е��ã����Բ��ø�����
*
*/
LW_SYMBOL_EXPORT uint32_t TM_Get_usCnt(void)
{
    // return HPC_Get_usCnt();
    return 1;
}

/***********************************************************************
* TM_Get_SecCnt - ��32λsec������
*
* RETURNS: 32λsec��������ǰֵ
*
* ע��: ���ڲ�ʹ��
*
*/
uint32_t TM_Get_SecCnt(void)
{
    // return HPC_Get_SecCnt();
    return 0;
}

/***********************************************************************
* TM_Get_BaseTimerCnt - ���baseTimer��С
*
* RETURNS: 64��ʱ����ǰ��С
*
*
*/
uint64_t TM_Get_BaseTimerCnt(void)
{
    uint32_t ulBaseH,ulBaseL;
    uint64_t ullValue1;

    // HW_TimeBase_Get_Base(&ulBaseH, &ulBaseL);
    // ullValue1=(((uint64_t)ulBaseH)<<32)+(uint64_t)ulBaseL;			/* ���64��ʱ����ǰ��С */

    return ullValue1;
}

/***********************************************************************
* TM_To_usCnt - ת������ʱ�ӵ�32λus������
*
* RETURNS: 32λus��������ǰֵ
*
* ע��: �˺����������жϵ��������е���
*
*/
uint32_t TM_To_usCnt(
    const EP_DATE_TIME *pdttm		/* ��ת��������ʱ�� */
)
{
    /* TODO */

    EP_DATE_TIME pdttmNow;

    uint32_t uldttmBase,uldttmGet;
    uint32_t ulusBase;
    uint32_t ultime=0;

    TM_Get_Sys_Time(&pdttmNow);
    uldttmBase = TM_Time_To_Long(&pdttmNow);
    ulusBase = (uint32_t)( (TM_Get_BaseTimerCnt()*1000/(uint64_t)CALC_FREQ)&0xFFFFFFFF);

//        logMsg("local time: %d-%d-%d %d:%d:%d!\n",pdttmNow.unYear,pdttmNow.ucMonth,
//                                                       pdttmNow.ucDate,pdttmNow.ucHour,
//                                                        pdttmNow.ucMinute,pdttmNow.ucSec);
//        logMsg("extern time: %d-%d-%d %d:%d:%d!\n",pdttm->unYear,pdttm->ucMonth,
//                                                       pdttm->ucDate,pdttm->ucHour,
//                                                        pdttm->ucMinute,pdttm->ucSec);

    uldttmGet = TM_Time_To_Long(pdttm);

    if(uldttmGet>=uldttmBase)
    {
        if((uldttmGet-uldttmBase)>1800)
        {
            ultime=0;
        }
        else
        {
            ultime=ulusBase+(uldttmGet-uldttmBase)*1000000+ (pdttm->unMSEL-pdttmNow.unMSEL)*1000-pdttmNow.unMicroSec;
        }
    }
    else
    {
        if((uldttmBase-uldttmGet)>1800)
        {
            ultime=0;
        }
        else
        {
            ultime=ulusBase+(uldttmGet-uldttmBase)*1000000+ (pdttm->unMSEL-pdttmNow.unMSEL)*1000-pdttmNow.unMicroSec;
        }
    }

    return ultime;
}

/***********************************************************************
* TM_Trans_Base_Dif - ת��TIMEBase��deltaΪ΢�����delta,�����ɸ�
*
* RETURNS: �ɹ�, EP_SUCCESS
*                 ����, EP_ERRROR
*
*/
EP_STATUS TM_Trans_Base_Dif(
    int64_t llBaseDif,		/* BASE��delta */
    int32_t *plSecDif,				/* ����SEC��delta */
    int32_t *plUsDif			/* ����Us��delta,Ӧ��<1000000 */
)
{
    uint64_t   ullValue1,ullValue2,ullValue3;
    uint32_t   ulSecDif,ulUsDif;

    if(llBaseDif>=0)
    {
        ullValue1=llBaseDif;
    }
    else
    {
        ullValue1=-llBaseDif;
    }
    /* ��DIF�ľ���ֵ���м��� */

    ullValue2=(ullValue1/(uint64_t)(CALC_FREQ*1000));				/* ת��Ϊ���������� */
    ulSecDif=(uint32_t)(ullValue2&0xFFFFFFFF);				/* ȡ��32λ,��32λ���������ֵ */

    ullValue3=(ullValue1*1000/(uint64_t)CALC_FREQ);			/* ת��Ϊ�����΢����� */
    ullValue3-=ullValue2*1000000;  			/* ghx20060328�޸�bug */
    ulUsDif=(uint32_t)(ullValue3&0xFFFFFFFF);											/* ȡ��32λ,��32λ΢���������ֵ */

    if(llBaseDif>=0)
    {
        *plSecDif=(int32_t)ulSecDif;
        *plUsDif=(int32_t)ulUsDif;
    }
    else
    {
        *plSecDif=-((int32_t)ulSecDif);
        *plUsDif=-((int32_t)ulUsDif);
    }

    return  EP_SUCCESS;
}

/***********************************************************************
* TM_Trans_Base_Dif_us - ת��TIMEBase��deltaΪ΢��delta,�����ɸ�
*
* RETURNS: �ɹ�, EP_SUCCESS
*                 ����, EP_ERRROR
*
*/
EP_STATUS TM_Trans_Base_Dif_us(
    int64_t llBaseDif,		/* BASE��delta */
    int32_t *plUsDif			/* ����Us��delta,Ӧ��<1000000 */
)
{
    uint64_t   ullValue1,ullValue2;
    uint32_t   ulUsDif;

    if(llBaseDif>=0)
    {
        ullValue1=llBaseDif;
    }
    else
    {
        ullValue1=-llBaseDif;
    }
    /* ��DIF�ľ���ֵ���м��� */

    ullValue2=(ullValue1*1000/(uint64_t)CALC_FREQ);			/* ת��Ϊ�����΢����� */
    ulUsDif=(uint32_t)(ullValue2&0xFFFFFFFF);											/* ȡ��32λ,��32λ΢���������ֵ */

    if(llBaseDif>=0)
    {
        *plUsDif=(int32_t)ulUsDif;
    }
    else
    {
        *plUsDif=-((int32_t)ulUsDif);
    }

    return  EP_SUCCESS;
}

/***********************************************************************
* TM_Calc_Time - Caculate the date/time according refrence date/time and delta MSEL/sec.
*
* Return value:
*   	EP_SUCCESS, successful get the date/time.
*    	EP_SYS_ERR, other unexpected system error.
* Alert:
*    	Only year from 1980 to 2099 is valid.
*    	Overlap between the refrence and result is allowed.
*
*/
EP_STATUS TM_Calc_Time(
    const EP_DATE_TIME *pdttmRef, 		/* system date/time to be converted. */
    EP_DATE_TIME *pdttmRslt,		/* to save the result. */
    int32_t lMicroSec, 		/* delta MSEL/us change to us. */
    int32_t lDeltaSec		/* delta second. */
)
{
    int32_t lDeltaMSEL;                     /* �������仯�� */

    assert(pdttmRef!=NULL && pdttmRslt!=NULL);

    lMicroSec+=pdttmRef->unMicroSec;

    if (lMicroSec<0)
    {
        if(lMicroSec%1000 != 0)
        {
            /* ��δ������,����Ҫ������λ */
            pdttmRslt->unMicroSec=lMicroSec%1000+1000;
            /* C99�й涨a%b=a-(a/b)*b,��������Ϊ��,��Ҫȷ��VXWORKS�Ƿ�����ʵ��,�����޸Ĺ� */
            lDeltaMSEL=lMicroSec/1000-1;

        }
        else
        {
            pdttmRslt->unMicroSec=0;
            lDeltaMSEL=lMicroSec/1000;
        }
    }
    else
    {
        pdttmRslt->unMicroSec=lMicroSec%1000;
        lDeltaMSEL=lMicroSec/1000;
    }

    lDeltaMSEL+=pdttmRef->unMSEL;
    if (lDeltaMSEL<0)
    {
        if(lDeltaMSEL%1000!=0)
        {
            /* ��δ������,����Ҫ�����λ */
            pdttmRslt->unMSEL=lDeltaMSEL%1000+1000;
            lDeltaSec+=lDeltaMSEL/1000-1;
        }
        else
        {
            pdttmRslt->unMSEL=0;
            lDeltaSec+=lDeltaMSEL/1000;
        }
    }
    else
    {
        pdttmRslt->unMSEL=lDeltaMSEL%1000;
        lDeltaSec+=lDeltaMSEL/1000;
    }

    lDeltaSec+=TM_Time_To_Long(pdttmRef);

    TM_Long_To_Time(pdttmRslt, (uint32_t)lDeltaSec);

    pdttmRslt->ucQflag = pdttmRef->ucQflag;

    return EP_SUCCESS;
}


/* Change date/time to a long integer(Seconds from 1980/01/01/0:00'00",
 * MSEL is through away).
 * Parameters:
 *      pdttm, date/time to be converted.
 * Return value:
 *      Seconds from 1980/01/01/0:00'00".
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
uint32_t TM_Time_To_Long(const EP_DATE_TIME *pdttm)

{
    uint32_t ulRslt;

    ulRslt=(pdttm->unYear-1980)*365+(pdttm->unYear-1977)/4;

    ulRslt+=aunMonthDay[pdttm->ucMonth]+pdttm->ucDate-1;

    if (pdttm->unYear%4==0 && pdttm->ucMonth>2) ulRslt++;

    ulRslt*=24*3600L;

    ulRslt+=pdttm->ucSec+60*pdttm->ucMinute+3600L*pdttm->ucHour;

    return ulRslt;
}

/* Change date/time to a long long integer(Microseconds from 1980/01/01/0:00'00",
 * Parameters:
 *      pdttm, date/time to be converted.
 * Return value:
 *      Microseconds from 1980/01/01/0:00'00"00'''.
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
uint64_t TM_Time_To_Longlong(const EP_DATE_TIME *pdttm)
{
    uint64_t ullRslt=0;

    ullRslt=(pdttm->unYear-1980)*365+(pdttm->unYear-1977)/4;

    ullRslt+=aunMonthDay[pdttm->ucMonth]+pdttm->ucDate-1;

    if (pdttm->unYear%4==0 && pdttm->ucMonth>2) ullRslt++;

    ullRslt*=24*3600;

    ullRslt+=pdttm->ucSec+60*pdttm->ucMinute+3600*pdttm->ucHour;

    ullRslt*=1000000;

    ullRslt+=pdttm->unMSEL*1000+pdttm->unMicroSec;

    return ullRslt;
}

/* Change a long integer(Seconds from 1980/01/01/0:00'00") to date/time.
 * (MSEL is not set).
 * Parameters:
 *      pdttmRslt, result date/time.
 *      ulSec, Seconds from 1980/01/01/0:00'00".
 * Return value:
 *      None.
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
void TM_Long_To_Time(EP_DATE_TIME *pdttmRslt, uint32_t ulSec)
{
    const uint16_t *punSumDay;
    uint16_t unTemp;

    pdttmRslt->ucSec=ulSec%60;
    ulSec/=60;
    pdttmRslt->ucMinute=ulSec%60;
    ulSec/=60;
    pdttmRslt->ucHour=ulSec%24;
    ulSec/=24;
    pdttmRslt->ucWeekDay=(ulSec+1)%7+1;
    unTemp=1980+ulSec/365;
    ulSec=ulSec%365;

    if (ulSec<(unTemp-1977)/4)
    {
        unTemp--;
        ulSec+=365-(unTemp-1977)/4;
    }
    else ulSec-=(unTemp-1977)/4;

    pdttmRslt->unYear=unTemp;
    if (unTemp%4==0)
        punSumDay=aunLeapMonthDay+12;
    else
        punSumDay=aunMonthDay+12;
    for (unTemp=12; unTemp>1; unTemp--, punSumDay--)
        if (*punSumDay<=ulSec) break;
    pdttmRslt->ucMonth=(uint8_t)unTemp;
    pdttmRslt->ucDate=(uint8_t)ulSec-*punSumDay+1;

    return;
}

/* Change a long integer(Seconds from 1980/01/01/0:00'00") to date/time.
 * (MSEL is not set).
 * Parameters:
 *      pdttmRslt, result date/time.
 *      ullusCntFrom1980, Micro Seconds from 1980/01/01/0:00'00".
 * Return value:
 *      None.
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
void TM_Longlong_To_Time(EP_DATE_TIME *pdttmRslt, uint64_t ullusCntFrom1980)
{
    const uint16_t *punSumDay;
    uint16_t unTemp;

    pdttmRslt->unMicroSec=ullusCntFrom1980%1000;
    ullusCntFrom1980/=1000;
    pdttmRslt->unMSEL=ullusCntFrom1980%1000;
    ullusCntFrom1980/=1000;
    pdttmRslt->ucSec=ullusCntFrom1980%60;
    ullusCntFrom1980/=60;
    pdttmRslt->ucMinute=ullusCntFrom1980%60;
    ullusCntFrom1980/=60;
    pdttmRslt->ucHour=ullusCntFrom1980%24;
    ullusCntFrom1980/=24;
    pdttmRslt->ucWeekDay=(ullusCntFrom1980+1)%7+1;
    unTemp=1980+ullusCntFrom1980/365;
    ullusCntFrom1980=ullusCntFrom1980%365;

    if (ullusCntFrom1980<(unTemp-1977)/4)
    {
        unTemp--;
        ullusCntFrom1980+=365-(unTemp-1977)/4;
    }
    else ullusCntFrom1980-=(unTemp-1977)/4;

    pdttmRslt->unYear=unTemp;
    if (unTemp%4==0)
        punSumDay=aunLeapMonthDay+12;
    else
        punSumDay=aunMonthDay+12;
    for (unTemp=12; unTemp>1; unTemp--, punSumDay--)
        if (*punSumDay<=ullusCntFrom1980) break;
    pdttmRslt->ucMonth=(uint8_t)unTemp;
    pdttmRslt->ucDate=(uint8_t)ullusCntFrom1980-*punSumDay+1;

    return;
}

/* Change date/time to a long long integer(Milliseconds from 1980/01/01/0:00'00",
 * Parameters:
 *      pdttm, date/time to be converted.
 * Return value:
 *      Milliseconds from 1980/01/01/0:00'00"00'''.
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
int64_t TM_Time_To_Msec_from_1980(const EP_DATE_TIME *pdttm)
{
    int64_t llRslt=0;

    llRslt=(pdttm->unYear-1980)*365+(pdttm->unYear-1977)/4;

    llRslt+=aunMonthDay[pdttm->ucMonth]+pdttm->ucDate-1;

    if (pdttm->unYear%4==0 && pdttm->ucMonth>2) llRslt++;

    llRslt*=24*3600;

    llRslt+=pdttm->ucSec+60*pdttm->ucMinute+3600*pdttm->ucHour;

    llRslt*=1000;

    llRslt+=pdttm->unMSEL;

    return llRslt;
}


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
// LW_SYMBOL_EXPORT EP_STATUS TM_Get_Sys_Time(EP_DATE_TIME *pdttmNow)
// {
//     EP_USER_CLOCK clkSysCpy;

//     uint64_t   ullBaseDif;
//     EP_STATUS   result;
//     int32_t   lSecStep,lUsStep;

//     assert(pdttmNow != NULL);
    
//     clkSysCpy=clkSys;
    
//     ullBaseDif=TM_Get_BaseTimerCnt()-clkSysCpy.ullBaseTimerCnt;

//     //ulSysUs=(uint32_t)((((ullBaseDif+clkSys.ullBaseTimerCnt)*1000/(uint64_t)CALC_FREQ))&0xFFFFFFFF);						/* ȡ��32λ,��32λ΢���������ֵ */

//     result=TM_Trans_Base_Dif((int64_t)ullBaseDif,&lSecStep,&lUsStep);
//     if(result==EP_SUCCESS)
//     {
//         TM_Calc_Time(&clkSysCpy.dttmCheck, pdttmNow,lUsStep, lSecStep);
//     }
//     else
//     {
//         TM_Calc_Time(&clkSysCpy.dttmCheck, pdttmNow, 0, 0);
//     }
//     /* pdttmNow->ucQflag=0x0A; */ //����0.9ms��

//     pdttmNow->ucQflag = GetSysTimeQFlag();

//     return   EP_SUCCESS;

// }
LW_SYMBOL_EXPORT EP_STATUS TM_Get_Sys_Time(EP_DATE_TIME *pdttmNow)
{
   assert(pdttmNow != NULL); // 确保传入的指针有效

    struct timespec ts;
    struct tm tm;

    // 获取当前时间（使用 CLOCK_REALTIME）
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        perror("clock_gettime failed");
        return EP_ERROR;
    }

    // 将时间戳转换为本地时间
    if (localtime_r(&ts.tv_sec, &tm) == NULL) {
        perror("localtime_r failed");
        return EP_ERROR;
    }

    // 填充 EP_DATE_TIME 结构体
    pdttmNow->unMicroSec = ts.tv_nsec / 1000; // 纳秒转换为微秒
    pdttmNow->unMSEL = ts.tv_nsec / 1000000;  // 纳秒转换为毫秒
    pdttmNow->ucSec = tm.tm_sec;
    pdttmNow->ucMinute = tm.tm_min;
    pdttmNow->ucHour = tm.tm_hour;
    pdttmNow->ucDate = tm.tm_mday;
    pdttmNow->ucWeekDay = tm.tm_wday == 0 ? 7 : tm.tm_wday; // tm_wday: 0=周日, 1=周一...
    pdttmNow->ucMonth = tm.tm_mon + 1; // tm_mon: 0=一月, 1=二月...
    pdttmNow->unYear = tm.tm_year + 1900; // tm_year: 从 1900 开始计数
    pdttmNow->ucQflag = 0; // 时间质量标志，默认设置为 0
    pdttmNow->ucIrigbLSFlag = 0; // 闰秒标志，默认设置为 0

    return EP_SUCCESS;

}

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
// LW_SYMBOL_EXPORT EP_STATUS TM_Get_Sys_Time_And_Us_Cnt(EP_DATE_TIME *pdttmNow, uint32_t *pulSysUs)
// {
//     EP_USER_CLOCK clkSysCpy;

//     uint64_t   ullBaseDif;
//     EP_STATUS   result;
//     int32_t   lSecStep,lUsStep;
//     STATUS vxsts;

//     assert(pdttmNow != NULL);

//     vxsts = EDP_Lock();
//     assert (vxsts == OK);
    
//     clkSysCpy=clkSys;
    
//     ullBaseDif=TM_Get_BaseTimerCnt()-clkSysCpy.ullBaseTimerCnt;

//     *pulSysUs = (uint32_t)((((ullBaseDif+clkSysCpy.ullBaseTimerCnt)*1000/(uint64_t)CALC_FREQ))&0xFFFFFFFF);		

//     vxsts = EDP_Unlock();
//     assert (vxsts == OK);				/* ȡ��32λ,��32λ΢���������ֵ */

//     result=TM_Trans_Base_Dif((int64_t)ullBaseDif,&lSecStep,&lUsStep);
//     if(result==EP_SUCCESS)
//     {
//         TM_Calc_Time(&clkSysCpy.dttmCheck, pdttmNow,lUsStep, lSecStep);
//     }
//     else
//     {
//         TM_Calc_Time(&clkSysCpy.dttmCheck, pdttmNow, 0, 0);
//     }
//     /* pdttmNow->ucQflag=0x0A; */ //����0.9ms��

//     pdttmNow->ucQflag = GetSysTimeQFlag();

//     return   EP_SUCCESS;

// }


LW_SYMBOL_EXPORT EP_STATUS TM_Get_Sys_Time_And_Us_Cnt(EP_DATE_TIME *pdttmNow, uint32_t *pulSysUs)
{
    if (!pdttmNow && !pulSysUs)
        return EP_SUCCESS;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts); // 获取系统时间

    if (pdttmNow) {
        struct tm tm_now;
        localtime_r(&ts.tv_sec, &tm_now);

        pdttmNow->unYear   = tm_now.tm_year + 1900;
        pdttmNow->ucMonth  = tm_now.tm_mon + 1;
        pdttmNow->ucDate   = tm_now.tm_mday;
        pdttmNow->ucHour   = tm_now.tm_hour;
        pdttmNow->ucMinute = tm_now.tm_min;
        pdttmNow->ucSec    = tm_now.tm_sec;
        pdttmNow->ucWeekDay = tm_now.tm_wday == 0 ? 7 : tm_now.tm_wday; // 1=Monday..7=Sunday

        // 纳秒转换成 unMicroSec 和 unMSEL
        uint32_t total_us = ts.tv_nsec / 1000; // 微秒
        pdttmNow->unMicroSec = total_us / 1000; // 毫秒部分 (0-999)
        pdttmNow->unMSEL     = total_us % 1000; // 毫秒内微秒部分 (0-999)

        pdttmNow->ucQflag = 0;         // 可根据需要设置
        pdttmNow->ucIrigbLSFlag = 0;   // 闰秒标志，Linux 下默认0
    }

    if (pulSysUs) {
        *pulSysUs = ts.tv_nsec / 1000; // 32位微秒计数
    }

    return EP_SUCCESS;
}


/* Get system time(from GPS or a master station).���ش������־��ʱ��
 * Parameters:
 *      pdttmNow, structure to save the date/time.
 * Return value:
 *      EP_SUCCESS, successful get the date/time.
 *      EP_LOCAL_MSG, system date/time not checked for long time.
 *      EP_NOT_INIT, system time was never checked from CPU reset.
 *      EP_HARD_ERR, hardware error(such as the crystal not working.
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
EP_STATUS TM_Get_Sys_Time_LS(EP_DATE_TIME *pdttmNow)
{
    uint32_t ulMicroSec = 0;
    uint32_t ulSec = 0;

    // assert(pdttmNow != NULL);

    // ulMicroSec = TM_Get_usCnt();
    // TM_Get_Sys_Time(pdttmNow);

    // if(EP_GetSynMode()==MMI_SYN_SNTP_ADJ)
    // {
    //     if(bSNTP60SecFlag==TRUE && pdttmNow->ucSec == 59)
    //     {
    //         pdttmNow->ucIrigbLSFlag |= IRIGB_PLS_60SEC;
    //         pdttmNow->ucSec = 60;
    //     }
    // }
    // else
    // {
    //     SYN_SetIrigBFlag(ulMicroSec, pdttmNow);

    //     if(((pdttmNow->ucIrigbLSFlag & IRIGB_PLS_60SEC) == IRIGB_PLS_60SEC )
    //             && (pdttmNow->ucSec == 0))
    //     {
    //         ulSec = TM_Time_To_Long(pdttmNow);
    //         ulSec--;
    //         TM_Long_To_Time(pdttmNow, ulSec);
    //         pdttmNow->ucSec = 60;
    //     }
    // }
    return EP_SUCCESS;
}

EP_STATUS TM_Trans_LeapTime(EP_DATE_TIME *pLeapTtm)
{
    // uint32_t ulSec=0;
    // if(EP_GetSynMode()==MMI_SYN_SNTP_ADJ)
    // {
    //     if(pLeapTtm->ucSec==59 && (pLeapTtm->ucIrigbLSFlag&0x01))
    //     {
    //         pLeapTtm->ucSec=60;
    //     }
    // }
    // else if((pLeapTtm->ucIrigbLSFlag&0x01) && (pLeapTtm->ucSec==0))/*��ʾ���¼���ʱ���0����������ĵ�60��*/
    // {
    //     ulSec=TM_Time_To_Long(pLeapTtm)-1;
    //     TM_Long_To_Time(pLeapTtm,ulSec);
    //     pLeapTtm->ucSec=60;
    // }
    return EP_SUCCESS;
}

// EP_STATUS TM_Get_Sys_Us_UTC_Time(US_CNT_UTC_TIME *pusUTCtmNow, uint32_t *pus32Cnt)
// {
//     uint64_t   ullBaseTCnt;
//     EP_STATUS   result;
//     int32_t   lUsStep;
//     EP_USER_CLOCK clkSysCpy;

//     assert(pusUTCtmNow != NULL);

//     ullBaseTCnt=TM_Get_BaseTimerCnt();

//     if (pus32Cnt != NULL)
//     {
//         *pus32Cnt =(uint32_t)(((ullBaseTCnt*1000/(uint64_t)CALC_FREQ))&0xFFFFFFFF);						/* ȡ��32λ,��32λ΢���������ֵ */
//     }

//     clkSysCpy=clkSys;

//     result=TM_Trans_Base_Dif_us((int64_t)(ullBaseTCnt-clkSysCpy.ullBaseTimerCnt),&lUsStep);

//     if(result==EP_SUCCESS)
//     {
//         pusUTCtmNow->ullusCntFrom1970=clkSysCpy.usUTCtmCheck.ullusCntFrom1970+lUsStep;
//     }
//     else
//     {
//         pusUTCtmNow->ullusCntFrom1970=clkSysCpy.usUTCtmCheck.ullusCntFrom1970;
//     }

//     /* pusUTCtmNow->ucQflag=0x0A; */ //����0.9ms��
//     pusUTCtmNow->ucQflag = GetSysTimeQFlag();

//     return   EP_SUCCESS;
// }


EP_STATUS TM_Get_Sys_Us_UTC_Time(US_CNT_UTC_TIME *pusUTCtmNow, uint32_t *pus32Cnt)//新实现，使用linux的time库
{
    assert(pusUTCtmNow != NULL);

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
    {
        // 获取时间失败 → 返回 0 + 不可靠标志
        pusUTCtmNow->ullusCntFrom1970 = 0;
        pusUTCtmNow->ucQflag = UTC_Q_CLOCK_NOT_SYNCHRONIZED;

        if (pus32Cnt)
            *pus32Cnt = 0;

        return EP_SUCCESS;
    }

    // 微秒计数
    uint64_t us_from_1970 = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
    pusUTCtmNow->ullusCntFrom1970 = us_from_1970;

    // 返回低 32 位微秒计数
    if (pus32Cnt)
        *pus32Cnt = (uint32_t)(us_from_1970 & 0xFFFFFFFF);

    // 时间质量标志
    pusUTCtmNow->ucQflag = GetSysTimeQFlag();

    return EP_SUCCESS;
}

/* ת��32λus������������ʱ��
 * ������
 *      ulMicroSec����ת����us������ֵ
 *      pdttm�����ת�����
 * ����ֵ��
 *      ��
 * ע�⣺
 *      �˺����������жϵ��������е���
 */
LW_SYMBOL_EXPORT EP_STATUS TM_To_Dttm(uint32_t ulMicroSec, EP_DATE_TIME *pdttm)
{
    int32_t lUsStep, lSecStep;
    EP_DATE_TIME dttmNow;
    uint32_t ulSysUs;

    TM_Get_Sys_Time_And_Us_Cnt(&dttmNow, &ulSysUs);	/* ��ȡϵͳ��ǰʱ�� */
    if(ulSysUs>ulMicroSec)
    {
        if((ulSysUs-ulMicroSec)<=0x7FFFFFFF)
            lUsStep=-(int32_t)(ulSysUs-ulMicroSec);
        else
            lUsStep=(int32_t)(ulMicroSec-ulSysUs);
    }
    else
    {
        if((ulMicroSec-ulSysUs)<=0x7FFFFFFF)
            lUsStep=(int32_t)(ulMicroSec-ulSysUs);
        else
            lUsStep=-(int32_t)(ulSysUs-ulMicroSec);
    }

    lSecStep=lUsStep/1000000;		/* �� */
    lUsStep=lUsStep%1000000;				/* ΢�� */
    TM_Calc_Time(&dttmNow, pdttm, lUsStep, lSecStep);

    SYN_SetIrigBFlag(ulMicroSec,pdttm);

    return EP_SUCCESS;
}


/* Set the system time.
 * Parameters:
 *      pdttmSet, date time want to set(week day vlue is not care).
 *      bPecision, flag of if the time is pecision.
 * Return value:
 *      EP_SUCCESS, successful set the clock.
 *      EP_HARD_ERR, hardware error(such as the crystal not working).
 *      EP_SYS_ERR, other unexpected system error.
 * Alert:
 *      Only year from 1980 to 2099 is valid.
 */
EP_STATUS TM_Set_Sys_Time(const EP_DATE_TIME *pdttmSet, BOOL bPecision)
{
    uint64_t  ullVal;
    STATUS vxsts;

    assert (pdttmSet != NULL);
    
    // vxsts = EDP_Lock();
    // assert (vxsts == OK);

    // clkSys.dttmCheck.unMicroSec = pdttmSet->unMicroSec;
    // clkSys.dttmCheck.unMSEL = pdttmSet->unMSEL;
    // clkSys.dttmCheck.ucSec = pdttmSet->ucSec;
    // clkSys.dttmCheck.ucMinute = pdttmSet->ucMinute;
    // clkSys.dttmCheck.ucHour = pdttmSet->ucHour;
    // clkSys.dttmCheck.ucDate = pdttmSet->ucDate;
    // clkSys.dttmCheck.ucWeekDay = pdttmSet->ucWeekDay;
    // clkSys.dttmCheck.ucMonth = pdttmSet->ucMonth;
    // clkSys.dttmCheck.unYear = pdttmSet->unYear;
    // /* clkSys.dttmCheck.ucQflag = pdttmSet->ucQflag; */

    // clkSys.ullBaseTimerCnt = TM_Get_BaseTimerCnt();

    // TM_Time_To_Us_UTC_Time(pdttmSet,&clkSys.usUTCtmCheck);

    // /*���¹����ټ����clkBench��׼ ,2013-5-25  ZY */
    // HW_TimeBase_Get_Base(&clkBench.ulBaseHBench, &clkBench.ulBaseLBench);
    // ullVal=clkSys.ullBaseTimerCnt*1000/(uint64_t)CALC_FREQ;	/* ת��Ϊ΢����� */
    // clkBench.ulUsCntBench=(uint32_t)(ullVal&0xFFFFFFFF);		  /* ȡ��32λ,��32λ΢���������ֵ */
    // ullVal=ullVal/1000;
    // clkBench.ulMsCntBench=(uint32_t)(ullVal&0xFFFFFFFF);    /* ȡ��32λ,��32λ�����������ֵ */
    // clkBench.dttmBench=clkSys.dttmCheck;
    // clkBench.usUTCtmBench=clkSys.usUTCtmCheck;
    // clkBench.secUTCtmBench.ulSecCntFrom1970=(uint32_t)(clkBench.usUTCtmBench.ullusCntFrom1970/1000000);
    // clkBench.secUTCtmBench.ulUsRemainder=clkBench.usUTCtmBench.ullusCntFrom1970-
    //                                      ((uint64_t)(clkBench.secUTCtmBench.ulSecCntFrom1970))*((uint64_t)(1000000));
    // clkBench.secUTCtmBench.ucQflag=clkBench.usUTCtmBench.ucQflag;


    // /* if the time is pecision */
    // if (bPecision)
    // {
    //     unTrustSysTime = TRUE;
    // }
    // else
    // {
    //     unTrustSysTime = FALSE;
    // }
    
    // vxsts = EDP_Unlock();
    // assert (vxsts == OK);


    return EP_SUCCESS;
}

/* �뵱ǰʱ�ӶԱȣ��õ��Ƿ���Ҫ����ϵͳʱ�ӵı�־
 * Parameters:
 *      pdttmSet, date time want to set(week day vlue is not care).
 * Return value:
 *      TRUE,��Ҫ��������ʱ��.
 *      FALSE,������������ʱ��
 * Alert:        */
BOOL TM_Get_Sys_Check_Time_Flag(EP_DATE_TIME *pdttmSet)
{
    EP_DATE_TIME dttm;
    EP_STATUS retcode;
    uint32_t ulSec =0;
    uint32_t ulSecSet =0;
    int32_t interval = 0;
    retcode=TM_Get_Sys_Time(&dttm);
    ulSec = TM_Time_To_Long(&dttm);
    ulSecSet = TM_Time_To_Long(pdttmSet);
    interval = (ulSecSet-ulSec)*1000+pdttmSet->unMSEL-dttm.unMSEL;
    if(interval>500 || interval<-500)
        return TRUE ;
    else
        return FALSE ;
}

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
EP_STATUS TM_Set_Sys_Time_Adjust(const EP_DATE_TIME *pdttmSet, BOOL bPecision, uint64_t ulAdjustUs)
{
    int iLockKey;
    uint64_t  ullVal;

    // assert(pdttmSet!=NULL);

    // iLockKey=EP_intLock();/*Ҫע�Ᵽ��ȫ������  */

    // clkSys.dttmCheck.unMicroSec=pdttmSet->unMicroSec;
    // clkSys.dttmCheck.unMSEL=pdttmSet->unMSEL;
    // clkSys.dttmCheck.ucSec=pdttmSet->ucSec;
    // clkSys.dttmCheck.ucMinute=pdttmSet->ucMinute;
    // clkSys.dttmCheck.ucHour=pdttmSet->ucHour;
    // clkSys.dttmCheck.ucDate=pdttmSet->ucDate;
    // clkSys.dttmCheck.ucWeekDay=pdttmSet->ucWeekDay;
    // clkSys.dttmCheck.ucMonth=pdttmSet->ucMonth;
    // clkSys.dttmCheck.unYear=pdttmSet->unYear;
    // clkSys.ullBaseTimerCnt=ulAdjustUs;

    // TM_Time_To_Us_UTC_Time(pdttmSet,&clkSys.usUTCtmCheck);

    // /*���¹����ټ����clkBench��׼ ,2013-5-25  ZY */
    // clkBench.ulBaseLBench=(uint32_t)((clkSys.ullBaseTimerCnt)&(0xFFFFFFFF));
    // clkBench.ulBaseHBench=(uint32_t)(((clkSys.ullBaseTimerCnt)&((uint64_t)(0xFFFFFFFF00000000LL)))>>32);
    // ullVal=clkSys.ullBaseTimerCnt*1000/(uint64_t)CALC_FREQ;	/* ת��Ϊ΢����� */
    // clkBench.ulUsCntBench=(uint32_t)(ullVal&0xFFFFFFFF);		  /* ȡ��32λ,��32λ΢���������ֵ */
    // ullVal=ullVal/1000;
    // clkBench.ulMsCntBench=(uint32_t)(ullVal&0xFFFFFFFF);    /* ȡ��32λ,��32λ�����������ֵ */
    // clkBench.dttmBench=clkSys.dttmCheck;
    // clkBench.usUTCtmBench=clkSys.usUTCtmCheck;
    // clkBench.secUTCtmBench.ulSecCntFrom1970=(uint32_t)(clkBench.usUTCtmBench.ullusCntFrom1970/1000000);
    // clkBench.secUTCtmBench.ulUsRemainder=clkBench.usUTCtmBench.ullusCntFrom1970-
    //                                      ((uint64_t)(clkBench.secUTCtmBench.ulSecCntFrom1970))*((uint64_t)1000000);
    // clkBench.secUTCtmBench.ucQflag=clkBench.usUTCtmBench.ucQflag;

    // if(bPecision)
    // {
    //     unTrustSysTime=TRUE;
    // }
    // else
    // {
    //     unTrustSysTime=FALSE;
    // }

    // EP_intUnlock(iLockKey);

    /* LOG_Write(LOG_KERNEL, "��ʱ����!Ϊ���ʱ��ֶ�ʱ\n", NULL); */

    return EP_SUCCESS;
}

/***********************************************************************
* TM_AdjustTime - ʹ��B�����/���������
*
* RETURNS: ��
*
*/
void TM_AdjustTime(
    IRIG_BTime *IRIGTime,
    uint32_t DelayTime,
    uint32_t timeTrans,
    uint8_t GPS_Flag
)
{
    uint32_t ulsec;
    EP_DATE_TIME dttm;
    EP_DATE_TIME dttm_ref;
    EP_STATUS retcode;
    uint32_t usCnt;
    uint32_t lstep;
    static EP_DATE_TIME PreDttm= {0,0,0,0,0,0,0,0,0} ;
    static uint8_t isFirstGps=0;
    uint32_t ulSec, ulSecPre;

    gpsModeTimeInterval = tickGet();
    ulGpsTimeIntervalforRpt=tickGet();

    if(GPS_Flag == 3)
    {
        /*����IGRB  */
        if((retcode=TM_Get_Sys_Time(&dttm_ref)) != EP_ERROR)
        {
            ulsec=(IRIGTime->iDay-1)*24*3600+IRIGTime->cHour*3600+IRIGTime->cMinute*60+IRIGTime->cSecond;
            dttm_ref.ucMonth=1;
            dttm_ref.ucDate=1;
            dttm_ref.ucHour=0;
            dttm_ref.ucMinute=0;
            dttm_ref.ucSec=0;
            dttm_ref.unMSEL=0;
            dttm_ref.unMicroSec=0;
            TM_Calc_Time(&dttm_ref, &dttm, 0, ulsec);			/* �����ľ�ȷֵ */
            usCnt=TM_Get_usCnt();
            if(usCnt<timeTrans)
            {
                lstep=0xffffffff-timeTrans+1+usCnt+DelayTime;
            }
            else
            {
                lstep=usCnt-timeTrans+DelayTime;
            }
            if(lstep >= 2000000) 		/* ��ʱ������ */
                lstep=0;
            dttm.unMSEL=lstep/1000;
            dttm.unMicroSec=lstep%1000;
            if(IRIGTime->cYear>0)
            {
                dttm.unYear = IRIGTime->cYear+2000 ;  /* hchj add */
            }
            else
            {
                dttm.unYear = dttm_ref.unYear ; /* hchj add */
            }
            TM_Set_Sys_Time(&dttm, TRUE);
        }
    }
    else if(GPS_Flag==0)
    {
        /*GPS����Ӧ*/
        if((retcode=TM_Get_Sys_Time(&dttm))!=EP_ERROR)
        {
            if(isFirstGps==0)
            {
                PreDttm = dttm ;
                isFirstGps = 1;
            }
            else
            {
                ulSecPre=TM_Time_To_Long(&PreDttm);
                ulSec =TM_Time_To_Long(&dttm);
                if((ulSec-ulSecPre)>50&&(ulSec-ulSecPre)<70)
                    GPS_Flag =2 ;/*GPS�ֶ�ʱ*/
                if((ulSec-ulSecPre)>=0&&(ulSec-ulSecPre)<2)
                    GPS_Flag =1 ;/*GPS���ʱ*/
                PreDttm = dttm ;
            }
        }
    }
    else if(GPS_Flag ==1)
    {
        /*GPS ���ʱ*/
        if((retcode=TM_Get_Sys_Time(&dttm)) != EP_ERROR)
        {
            if(dttm.unMSEL>(500+DelayTime/1000))			/* hchj 2006-1-14, DY 12/18/2006 DelayTime��λΪus��ת��Ϊms */
            {
                /*ԭ��helong Ϊ500,��Ϊ600(500+DelayTime),��Ҫ����GPS�������,���100ms*/
                uint32_t ulSec;
                ulSec=TM_Time_To_Long(&dttm)+1;		/* ���� */
                TM_Long_To_Time(&dttm,ulSec);
            }
            dttm.unMSEL=0;
            dttm.unMicroSec=0;/**/
            usCnt=TM_Get_usCnt();
            ulAdjustUs=TM_Get_BaseTimerCnt();

            if(usCnt<timeTrans)
            {
                lstep=0xffffffff-timeTrans+1+usCnt+DelayTime;
            }
            else
            {
                lstep=usCnt-timeTrans+DelayTime;
            }
            dttm.unMSEL+=lstep/1000;
            dttm.unMicroSec=lstep%1000;
            TM_Set_Sys_Time_Adjust(&dttm,TRUE, ulAdjustUs);
        }
    }
    else if(GPS_Flag == 2)
    {
        /*GPS �ֶ�ʱ*/
        if((retcode=TM_Get_Sys_Time(&dttm))!=EP_ERROR)
        {
            if(dttm.ucSec>30)
            {
                uint32_t ulSec;
                ulSec=TM_Time_To_Long(&dttm)+60;
                TM_Long_To_Time(&dttm,ulSec);
            }
            dttm.unMSEL=0;
            dttm.unMicroSec=0;/**/
            dttm.ucSec =0 ;
            usCnt=TM_Get_usCnt();
            ulAdjustUs=TM_Get_BaseTimerCnt();

            if(usCnt<timeTrans)
            {
                lstep=0xffffffff-timeTrans+1+usCnt+DelayTime;
            }
            else

            {
                lstep=usCnt-timeTrans+DelayTime;
            }
            dttm.unMSEL+=lstep/1000;
            dttm.unMicroSec=lstep%1000;
            TM_Set_Sys_Time_Adjust(&dttm,TRUE,ulAdjustUs); 		/* ����ϵͳʱ�� */
        }
    }
}

/***********************************************************************
* AddRunTimeTag - ���ӳ�������ʱ���
*
* RETURNS: ��
*
// */
void AddRunTimeTag(
    u_int8_t *pProgramPoint		/* ����� */
)
{
    return ;
}
// {
//     strcpy((char *)RunTimeTag.TimeTag[RunTimeTag.uCounter].ProgramPoint, pProgramPoint);
//     RunTimeTag.TimeTag[RunTimeTag.uCounter].TimePoint=TM_Get_usCnt();		/* ��ȡus��*/
//     if(RunTimeTag.uCounter == 0)
//     {
//         RunTimeTag.TimeTag[RunTimeTag.uCounter].DifTime=0;
//     }
//     else
//     {
//         RunTimeTag.TimeTag[RunTimeTag.uCounter].DifTime=RunTimeTag.TimeTag[RunTimeTag.uCounter].TimePoint-RunTimeTag.TimeTag[RunTimeTag.uCounter-1].TimePoint;
//     }
//     RunTimeTag.TimeTag[RunTimeTag.uCounter].TotalTime=RunTimeTag.TimeTag[RunTimeTag.uCounter].TimePoint-RunTimeTag.TimeTag[0].TimePoint;

//     if(RunTimeTag.uCounter == (MAXTIMETAGNUM-1))
//     {
//         RunTimeTag.uCounter=0;
//     }
//     else
//     {
//         RunTimeTag.uCounter++;
//     }
// }

/* show the time point of excuting process.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void showTimePoint(void)
{
    int i;

    LOG_Dbg_Msg("ִ��ʱ������: %d\n",
                RunTimeTag.uCounter, 0, 0, 0, 0, 0);

    for (i=0; i<RunTimeTag.uCounter; i++)
    {
        LOG_Dbg_Msg("%s: %dus %dus.\n", (int)RunTimeTag.TimeTag[i].ProgramPoint, RunTimeTag.TimeTag[i].TotalTime,RunTimeTag.TimeTag[i].DifTime, 0, 0, 0);
    }
}

/* ��ȡϵͳʱ��pdttmBgn��pdttmEnd֮���ʱ���,��΢��Ϊ��λ����
 * Parameters:
 *      pdttmBgn, �������ʼʱ��.
 *      pdttmEnd, �������ֹʱ��.
 * Return value:
 *      EP_SUCCESS, successful set the clock.
 *      EP_SYS_ERR, error.
 * ע��:
 *      pdttmEnd �������pdttmBgn,��ʱ���ܳ���4294��(71.58��),���򷵻�ʧ��;
 *      1980 ~ 2099 �����Ч,���򷵻�ʧ��;
 */
EP_STATUS TM_Get_Delta_Sys_Time(EP_DATE_TIME *pdttmBgn, EP_DATE_TIME *pdttmEnd, uint32_t *ulDeltaUs)
{
    uint32_t ulSecBgn=0;
    uint32_t ulSecEnd=0;
    uint32_t ulDeltaSec=0;

    if(pdttmBgn->unYear<1980||pdttmBgn->unYear>2099||pdttmEnd->unYear<1980||pdttmEnd->unYear>2099)
        return EP_SYS_ERR;
    ulSecBgn=TM_Time_To_Long(pdttmBgn);
    ulSecEnd=TM_Time_To_Long(pdttmEnd);
    ulDeltaSec=ulSecEnd-ulSecBgn;

    if(ulSecEnd<ulSecBgn||ulDeltaSec>4294)
        return EP_SYS_ERR;
    *ulDeltaUs=1000000+pdttmEnd->unMSEL*1000+pdttmEnd->unMicroSec
               -pdttmBgn->unMSEL*1000-pdttmBgn->unMicroSec;
    if(*ulDeltaUs>=1000000)
    {
        *ulDeltaUs+=ulDeltaSec*1000000-1000000;
    }
    else
    {
        if(ulDeltaSec==0)
            return EP_SYS_ERR;
        *ulDeltaUs+=(ulDeltaSec-1)*1000000;
    }
    return EP_SUCCESS;

}

void Dttm_To_UTC_Time(const EP_DATE_TIME *pdttm, MMS_UTC_TIME *pUtctm)
{
    uint32_t ulSec;
    double temp = 0.0;
    uint32_t MicroSec=0;

    assert(pdttm);
    assert(pUtctm);

    ulSec=TM_Time_To_Long(pdttm);

    // if(g_eBdLevelType == Board_LEVEL_03)
    // {
    //     if(GetTimeZoneCallback!=NULL)
    //     {
    //         ulSec-=(((*GetTimeZoneCallback)(0,0,0,0,0))*3600);
    //     }
    // }
    // else
    // {
    //     ulSec-=(8*3600);
    // }

    // pUtctm->secs=ulSec+315532800;/*����1970-1980ʮ���ʱ��*/

    // MicroSec=pdttm->unMSEL*1000+pdttm->unMicroSec;
    // temp = (double)MicroSec / 1000000 * (INT32)0x00FFFFFF;

    // if(temp > LONG_MAX)
    // {
    //     pUtctm->fraction= LONG_MAX;
    // }
    // else if(temp < LONG_MIN)
    // {
    //     pUtctm->fraction= LONG_MIN;
    // }
    // else
    // {
    //     pUtctm->fraction= (INT32)(temp + 0.5);
    // }
    // if(pdttm->ucQflag!=0x00)
    // {
    //     pUtctm->qflags = pdttm->ucQflag;
    // }
    // else
    // {
    //     pUtctm->qflags = 0x0A;
    // }
}


/*******************************************************
UTC_Time_To_Dttm()
����:��UTCʱ��ת��Ϊ����ʱ���ʽ��
����:putctm UTCʱ��Դ
     pdttmRslt ����ʱ��ת�����
����ֵ:��
********************************************************/
void TM_UTC_Time_To_Dttm(const MMS_UTC_TIME *putctm, EP_DATE_TIME *pdttmRslt)
{
    const uint16_t *punSumDay;
    uint32_t ulSec;
    uint32_t ulUs;
    uint32_t ulTemp;

    assert(putctm!=NULL);
    assert(pdttmRslt!=NULL);

    ulSec=putctm->secs;
    ulUs=putctm->fraction;

    pdttmRslt->ucSec=ulSec%60;
    ulSec/=60;
    pdttmRslt->ucMinute=ulSec%60;
    ulSec/=60;
    pdttmRslt->ucHour=ulSec%24;
    ulSec/=24;
    pdttmRslt->ucWeekDay=(ulSec+1)%7+1;
    ulTemp=1970+ulSec/365;
    ulSec=ulSec%365;

    if (ulSec<(ulTemp-1969)/4)
    {
        ulTemp--;
        ulSec+=365-(ulTemp-1969)/4;
    }
    else ulSec-=(ulTemp-1969)/4;

    pdttmRslt->unYear=(uint16_t)ulTemp;
    if (ulTemp%4==0)
        punSumDay=aunLeapMonthDay+12;
    else
        punSumDay=aunMonthDay+12;
    for (ulTemp=12; ulTemp>1; ulTemp--, punSumDay--)
        if (*punSumDay<=ulSec) break;
    pdttmRslt->ucMonth=(uint8_t)ulTemp;
    pdttmRslt->ucDate=(uint8_t)ulSec-*punSumDay+1;

    pdttmRslt->unMSEL=ulUs/1000;
    pdttmRslt->unMicroSec=ulUs%1000;

    return;
}

void Us_UTC_Time_To_MMS_UTC_Time(const US_CNT_UTC_TIME *pUsUtctm, MMS_UTC_TIME *pUtctm)
{
    uint64_t temp = 0.0;
    uint32_t MicroSec=0;

    assert(pUsUtctm!=NULL);
    assert(pUtctm!=NULL);

    pUtctm->secs=pUsUtctm->ullusCntFrom1970/1000000;

    MicroSec=pUsUtctm->ullusCntFrom1970%1000000;
    temp = (uint64_t)MicroSec * 0x00FFFFFF / 1000000;

    pUtctm->fraction= (uint32_t)temp;

    if(pUsUtctm->ucQflag!=0x00)
    {
        pUtctm->qflags = pUsUtctm->ucQflag;
    }
    else
    {
        pUtctm->qflags = 0x0A;
    }
}

/* ����:��ȡת��US_CNT_UTC_TIME��EP_DATE_TIME�ṹ
 * ����:
 *      pUsUtctm, ��ת����US_CNT_UTC_TIME�ṹ��ָ��;
 *      pdttm, ת�����EP_DATE_TIME�ṹ��ָ��;
 * ����:
 *      ��.
 */
LW_SYMBOL_EXPORT void Us_UTC_Time_To_Dttm(const US_CNT_UTC_TIME *pUsUtctm, EP_DATE_TIME *pdttm)
{
    uint64_t ullusCntFrom1980;
    if(pUsUtctm->ullusCntFrom1970 > (uint64_t)(315532800-8*3600)*1000000)
    {
        ullusCntFrom1980 = pUsUtctm->ullusCntFrom1970-(uint64_t)(315532800-8*3600)*1000000;/* ����1970-1980ʮ���ʱ�� */
    }
    else
    {
        ullusCntFrom1980 = 0;
    }
    TM_Longlong_To_Time(pdttm, ullusCntFrom1980);
    return;
}

/**
* @brief ����������UTCʱ��ת��Ϊ����ʱ��
* @param [in] pUsUtcTmLS ��ת���Ĵ�������UTCʱ��
* @param [in,out] pdttm ת���������ʱ��
* @return NONE.
* @ref
* @see
* @note
* @warning
*/
// void Us_UTC_Time_To_Dttm_LS(const US_CNT_UTC_TIME_LS *pUsUtcTmLS, EP_DATE_TIME *pdttm)
// {
//     // US_CNT_UTC_TIME tUsUtcTm;

//     // if((pUsUtcTmLS == NULL) || (pdttm == NULL))
//     // {
//     //     return;
//     // }

//     // tUsUtcTm.ucQflag = pUsUtcTmLS->ucQflag;
//     // tUsUtcTm.ullusCntFrom1970 = pUsUtcTmLS->ullusCntFrom1970;
//     // Us_UTC_Time_To_Dttm(&tUsUtcTm, pdttm);
//     // pdttm->ucQflag = pUsUtcTmLS->ucQflag;
//     // pdttm->ucIrigbLSFlag = pUsUtcTmLS->ucIrigbLSFlag;
// }

void MMS_UTC_Time_To_US_UTC_TIME(const MMS_UTC_TIME *pUtctm, US_CNT_UTC_TIME *pUsUtctm)
{
    double tmTemp;

    assert(pUtctm!=NULL);
    assert(pUsUtctm!=NULL);

    //tmTemp = (double)pUtctm->fraction/(pow((double)2,(double)pUtctm->qflags))*1000*1000;
    tmTemp=(double)pUtctm->fraction*1000000/((INT32)0x00FFFFFF);
    if(tmTemp>999999)
    {
        pUsUtctm->ullusCntFrom1970=999999;
    }
    else if(tmTemp<0)
    {
        pUsUtctm->ullusCntFrom1970=0;
    }
    else
    {
        pUsUtctm->ullusCntFrom1970 = tmTemp;
    }

    pUsUtctm->ullusCntFrom1970 += ((uint64_t)(pUtctm->secs)*1000000);

    pUsUtctm->ucQflag=pUtctm->qflags;

}

uint32_t Us_UTC_Time_To_us32Cnt(US_CNT_UTC_TIME uttime, BOOL *pbRet)
{
    US_CNT_UTC_TIME uttmNow;

    uint32_t ulusBase;
    uint32_t ultime=0;
    *pbRet=TRUE;

    /*if((uttime.ucQflag&0x60)!=0)*/
    if(uttime.ullusCntFrom1970==0)
    {
        *pbRet=FALSE;
        ultime=0;
        return ultime;
    }

    TM_Get_Sys_Us_UTC_Time(&uttmNow, &ulusBase);

    if(uttime.ullusCntFrom1970>=uttmNow.ullusCntFrom1970)
    {
        if((uttime.ullusCntFrom1970-uttmNow.ullusCntFrom1970)>1800000000)
        {
            ultime=0;
            *pbRet=FALSE;
        }
        else
        {
            ultime=ulusBase+(uint32_t)(uttime.ullusCntFrom1970-uttmNow.ullusCntFrom1970);
        }
    }
    else
    {
        if((uttmNow.ullusCntFrom1970-uttime.ullusCntFrom1970)>1800000000)
        {
            ultime=0;
            *pbRet=FALSE;
        }
        else
        {
            ultime=ulusBase-(uint32_t)(uttmNow.ullusCntFrom1970-uttime.ullusCntFrom1970);
        }
    }
    return ultime;
}

LW_SYMBOL_EXPORT EP_STATUS TM_Time_To_Us_UTC_Time(const EP_DATE_TIME *pdttm, US_CNT_UTC_TIME *pusutctm)
{
    pusutctm->ullusCntFrom1970=315532800-8*3600;/*����1970-1980ʮ���ʱ��*/
    pusutctm->ullusCntFrom1970*=1000000;
    pusutctm->ullusCntFrom1970+=TM_Time_To_Longlong(pdttm);
    pusutctm->ucQflag=pdttm->ucQflag;
    return EP_SUCCESS;
}

/*�����Ķ�ʱ���ڴ���TM_Set_Sys_Time*/
/* Set the system time for STI(PMU). Before setting,
 * The routine will first compare with the system time,
 * and check if the sec-pulse signal works.
 * Parameters:
 *      pdttmSet, date time want to set(week day vlue is not care).
 *      bPecision, flag of if the time is pecision.
 * Return value:
 *      EP_SUCCESS, successful set the clock.
 *      EP_HARD_ERR, hardware error(such as the crystal not working).
 *      EP_SYS_ERR, other unexpected system error.
 * Alert:
 *      Only year from 1980 to 2099 is valid. */
EP_STATUS TM_Set_Time_Compare_With_Sys_Time(const EP_DATE_TIME *pdttmSet, BOOL bPecision)
{
    EP_DATE_TIME DttmNow;
    EP_DATE_TIME DttmToSet;
    int64_t llDeltaMsec=0;
    int64_t llTmp=0;
    EP_STATUS result;

    // TM_Get_Sys_Time(&DttmNow);

    // llDeltaMsec=TM_Time_To_Msec_from_1980(pdttmSet)-TM_Time_To_Msec_from_1980(&DttmNow);

    // if(!bSysSecSynFlag)/*������δͬ��,ֱ������ʱ��*/
    // {
    //     result=TM_Set_Sys_Time(pdttmSet,bPecision);
    //     return result;
    // }
    // else if(g_eBdFrameType == Board_FRAME_NPS) /**< NPSӲ���ڶ�ʱ��������������²�ʹ�������Ķ�ʱ */
    // {
    //     return EP_SUCCESS;
    // }

    // if((llDeltaMsec<500)&&(llDeltaMsec>-500))/*���С��500ms,�����ʱ*/
    // {
    //     result=TM_Set_Sys_Time(&DttmNow,bPecision);/*����һ�鵱ǰϵͳʱ�䣬��ֹ���ڲ����ã����ϵͳʱ�ӻ�׼δ���£�
	// 												32λ΢���������ת���޷������ȷʱ��*/
    //     return result;

    // }

    // /*��ʱ���޸ĺ��뼰����*/
    // if(llDeltaMsec<0)
    // {
    //     llTmp=llDeltaMsec%1000+1000;
    // }
    // else
    // {
    //     llTmp=llDeltaMsec%1000;
    // }

    // if(llTmp>500)
    // {
    //     llDeltaMsec=llDeltaMsec+1000-llTmp;
    // }
    // else
    // {
    //     llDeltaMsec=llDeltaMsec-llTmp;
    // }

    // TM_Calc_Time(&DttmNow,&DttmToSet,0,(int32_t)(llDeltaMsec/1000));

    // result=TM_Set_Sys_Time(&DttmToSet,bPecision);

    return result;
}

EP_STATUS TM_Time_To_Us_UTC_Time_Without_Timezone_Comp(const EP_DATE_TIME *pdttm, US_CNT_UTC_TIME *pusutctm)
{
    pusutctm->ullusCntFrom1970=315532800;/*����1970-1980ʮ���ʱ��*/
    pusutctm->ullusCntFrom1970*=1000000;
    pusutctm->ullusCntFrom1970+=TM_Time_To_Longlong(pdttm);
    pusutctm->ucQflag=pdttm->ucQflag;
    return EP_SUCCESS;
}

void TM_Time_To_MMS_UTC_Time(const EP_DATE_TIME *pdttm, MMS_UTC_TIME *pUtctm)
{
    US_CNT_UTC_TIME usutctm;

    TM_Time_To_Us_UTC_Time_Without_Timezone_Comp(pdttm,&usutctm);

    Us_UTC_Time_To_MMS_UTC_Time(&usutctm,pUtctm);
}

/**
 * @brief ���ò���ϵͳʱ��
 * @param pdttm ����ʱ��
 * @return None
 * @ref  \n
 * �޸�����         �汾��       �޸���         �޸�����     \n
 * ----------------------------------------------------------\n
 * 2020/10/19      V1.0    ��ȫ       �����ú���
 * @see
 * @note
 * @warning
 */
void TM_Set_OS_Clock(const EP_DATE_TIME *pdttm)
{
    US_CNT_UTC_TIME usutctm;
    struct timespec clocktime;

    TM_Time_To_Us_UTC_Time_Without_Timezone_Comp(pdttm,&usutctm);

    clocktime.tv_sec = usutctm.ullusCntFrom1970/1000000;
    clocktime.tv_nsec = (usutctm.ullusCntFrom1970%1000000)*1000;
    
    clock_settime(CLOCK_REALTIME, &clocktime);
}

/* ����:��ȡ��ǰʱ���1970�꿪ʼ�����΢�����
 * ����:
 *      ��.
 * ����:
 *      uint64_t, ��ǰʱ���1970�꿪ʼ�����΢�����.
 */
LW_SYMBOL_EXPORT uint64_t TM_Get_usCnt_From1970(void)
{
    EP_DATE_TIME sysTime;
    US_CNT_UTC_TIME usUtcTm;

    TM_Get_Sys_Time(&sysTime);
    TM_Time_To_Us_UTC_Time(&sysTime, &usUtcTm);

    return usUtcTm.ullusCntFrom1970;
}

/*���ܣ��õ�ϵͳ���������  2013-5-25  ZY
* ��������
* ���أ����������
* ע��: ������ж��е���
      �����ڲ�ʹ��
*/
uint32_t TM_Get_msCnt(void)
{
    uint64_t ullValue1;
    uint32_t ulResult;

    ullValue1=TM_Get_BaseTimerCnt();			/* ���64��ʱ����ǰ��С */
    ullValue1=ullValue1/(uint64_t)CALC_FREQ;			/* ת��Ϊ������� */
    ulResult=(uint32_t)(ullValue1&0xFFFFFFFF);						/* ȡ��32λ,��32λ�����������ֵ */

    return  ulResult;
}


/*******************ʱ��ģ����غ�����Чʵ��*****************************/


/*���ܣ���Ч�õ�ϵͳ΢�������  2013-5-25  ZY
* ��������
* ���أ�32΢�������
* ע��: ������ж��е���
      �����ڲ�ʹ��
*/
uint32_t TM_High_Get_usCnt(void)
{
    if(bFstUpdtTimeBenchFlag_s)
    {
        /*����1��ʱ���׼���ڸ��������,
         �ÿ����㷨�����򶨵�������� */
        uint32_t ulBenchBaseL;
        uint32_t ulBenchUs;
        uint32_t ulCurBaseH,ulCurBaseL;
        uint32_t ulBaseDiff;
        uint32_t ulUsDiff;
        uint32_t ulUsCalc;
        int iLockKey;

        // /*��ֹ���жϻ�����ȼ������� */
        // iLockKey=EP_intLock();
        // ulBenchBaseL=clkBench.ulBaseLBench;
        // ulBenchUs=clkBench.ulUsCntBench;
        // EP_intUnlock(iLockKey);

        // HW_TimeBase_Get_Base(&ulCurBaseH, &ulCurBaseL);
        // ulBaseDiff=ulCurBaseL-ulBenchBaseL;
        // ulUsDiff=ulBaseDiff*1000/CALC_FREQ;/*���ÿ���32λ�����������Ϊ���¼��100ms��160ms,���ᵼ����� */
        // ulUsCalc=ulBenchUs+ulUsDiff;

        return  ulUsCalc;
    }
    else
    {
        /* ������õ�Чʵ��*/
        return(TM_Get_usCnt());
    }
}

/*���ܣ���Ч�õ�ϵͳ���������  2013-5-25  ZY
* ��������
* ���أ�32λ���������
* ע��: ������ж��е���
      �����ڲ�ʹ��
*/
uint32_t TM_High_Get_msCnt(void)
{
    if(bFstUpdtTimeBenchFlag_s)
    {
        /*��1��ʱ���׼���ڸ��������,
         �ÿ����㷨�����򶨵��������  */
        uint32_t ulBenchBaseL;
        uint32_t ulBenchMs;
        uint32_t ulCurBaseH,ulCurBaseL;
        uint32_t ulBaseDiff;
        uint32_t ulMsDiff;
        uint32_t ulMsCalc;
        int iLockKey;

        // /*��ֹ���жϻ�����ȼ������� */
        // iLockKey=EP_intLock();
        // ulBenchBaseL=clkBench.ulBaseLBench;
        // ulBenchMs=clkBench.ulMsCntBench;
        // EP_intUnlock(iLockKey);

        // HW_TimeBase_Get_Base(&ulCurBaseH, &ulCurBaseL);
        // ulBaseDiff=ulCurBaseL-ulBenchBaseL;
        // ulMsDiff=ulBaseDiff/CALC_FREQ;
        // ulMsCalc=ulBenchMs+ulMsDiff;

        return  ulMsCalc;
    }
    else
    {
        return(TM_Get_msCnt());
    }
}


/*���ܣ���Ч�õ�ϵͳʱ��  2013-5-25  ZY
* ������pdttmNow�����ص�ǰ����ʱ������ĵ�ַ
                ���÷��������
* ���أ��ɹ����
* ע��: �жϺ������ж��ܵ���
      �����ڲ�ʹ��
*/
EP_STATUS TM_High_Get_Sys_Time(EP_DATE_TIME *pdttmNow)
{
    // if(bFstUpdtTimeBenchFlag_s)
    // {
    //     /*����1��ʱ���׼���ڸ��������,
    //      �ÿ����㷨�����򶨵��������  */
    //     EP_CLOCK_HIGH_BENCH clkBenchCpy;
    //     uint32_t ulCurBaseH,ulCurBaseL;
    //     uint32_t ulBaseDiff;
    //     uint32_t ulUsDiff;
    //     uint32_t ulSecDiff;
    //     uint32_t ulUsLeft;
    //     int iLockKey;

    //     /*��ֹ���жϻ�����ȼ������� */
    //     iLockKey=EP_intLock();
    //     clkBenchCpy=clkBench;
    //     EP_intUnlock(iLockKey);

    //     HW_TimeBase_Get_Base(&ulCurBaseH, &ulCurBaseL);
    //     ulBaseDiff=ulCurBaseL-clkBenchCpy.ulBaseLBench;
    //     ulUsDiff=ulBaseDiff*1000/CALC_FREQ;/*���ÿ���32λ�����������Ϊ���¼��100ms��160ms,���ᵼ����� */
    //     ulSecDiff=ulUsDiff/1000000;
    //     ulUsLeft=ulUsDiff-ulSecDiff*1000000;
    //     TM_Calc_Time(&clkBenchCpy.dttmBench, pdttmNow,ulUsLeft, ulSecDiff);

    //     /* pdttmNow->ucQflag=0x0A; */ //����0.9ms��
    //     pdttmNow->ucQflag = GetSysTimeQFlag();

    //     return   EP_SUCCESS;
    // }
    // else
    {
        return(TM_Get_Sys_Time(pdttmNow));
    }
}

/*���ܣ���Ч�õ�ϵͳ΢��ʱ���UTC΢��ʱ��  2013-5-25  ZY
* ������pusUTCtmNow�����ص�ǰUTC΢��ʱ������ĵ�ַ
                   ���÷��������
       pus32Cnt�����ص�ǰϵͳ΢��ʱ������ĵ�ַ
                ���÷��������
* ���أ��ɹ����
* ע��: �жϻ������е���
      �����ڲ�ʹ��
*/
EP_STATUS TM_High_Get_Sys_Us_UTC_Time(US_CNT_UTC_TIME *pusUTCtmNow, uint32_t *pus32Cnt)
{
    assert(pusUTCtmNow);

    // if(bFstUpdtTimeBenchFlag_s)
    // {
    //     /*����1��ʱ���׼���ڸ��������,
    //      �ÿ����㷨�����򶨵��������  */
    //     EP_CLOCK_HIGH_BENCH clkBenchCpy;
    //     uint32_t ulCurBaseH,ulCurBaseL;
    //     uint32_t ulBaseDiff;
    //     uint32_t ulUsDiff;
    //     int iLockKey;

    //     /*��ֹ���жϻ�����ȼ������� */
    //     iLockKey=EP_intLock();
    //     clkBenchCpy=clkBench;
    //     EP_intUnlock(iLockKey);

    //     HW_TimeBase_Get_Base(&ulCurBaseH, &ulCurBaseL);
    //     ulBaseDiff=ulCurBaseL-clkBenchCpy.ulBaseLBench;
    //     ulUsDiff=ulBaseDiff*1000/CALC_FREQ;

    //     pusUTCtmNow->ullusCntFrom1970=clkBenchCpy.usUTCtmBench.ullusCntFrom1970+ulUsDiff;
    //     /* pusUTCtmNow->ucQflag=0x0A; */ //����0.9ms��
    //     pusUTCtmNow->ucQflag = GetSysTimeQFlag();
    //     if(pus32Cnt)
    //     {
    //         *pus32Cnt=clkBenchCpy.ulUsCntBench+ulUsDiff;
    //     }
    //     return   EP_SUCCESS;
    // }
    // else
    {
        return(TM_Get_Sys_Us_UTC_Time(pusUTCtmNow,pus32Cnt));
    }
}

/*���ܣ���Ч�õ�ϵͳ΢��ʱ��ʹ�����״̬���UTCʱ��
* ������pusUTCtmNow�����ص�ǰ������״̬���UTC΢��ʱ������ĵ�ַ
                   ���÷��������
       pus32Cnt�����ص�ǰϵͳ΢��ʱ������ĵ�ַ
                ���÷��������
* ���أ��ɹ����
* ע��: �жϻ������е���
      �����ڲ�ʹ��
// */
// EP_STATUS TM_High_Get_Sys_Us_UTC_Time_LS(US_CNT_UTC_TIME_LS *pusUTCtmNow, uint32_t *pus32Cnt)
// {
//     EP_CLOCK_HIGH_BENCH clkBenchCpy;
//     uint32_t ulCurBaseH,ulCurBaseL;
//     uint32_t ulBaseDiff;
//     uint32_t ulUsDiff;
//     int iLockKey;
//     EP_STATUS tRet = EP_ERROR;
//     EP_DATE_TIME tdttmNow;
//     uint32_t ulMicroSec = 0;
//     US_CNT_UTC_TIME tUtcTmNow;

//     assert(pusUTCtmNow);

//     if(bFstUpdtTimeBenchFlag_s)
//     {
//         /*����1��ʱ���׼���ڸ��������,         �ÿ����㷨�����򶨵��������  */
//         /*��ֹ���жϻ�����ȼ������� */
//         iLockKey=EP_intLock();
//         clkBenchCpy=clkBench;
//         EP_intUnlock(iLockKey);

//         HW_TimeBase_Get_Base(&ulCurBaseH, &ulCurBaseL);
//         ulBaseDiff=ulCurBaseL-clkBenchCpy.ulBaseLBench;
//         ulUsDiff=ulBaseDiff*1000/CALC_FREQ;

//         pusUTCtmNow->ullusCntFrom1970=clkBenchCpy.usUTCtmBench.ullusCntFrom1970+ulUsDiff;
//         pusUTCtmNow->ucQflag = GetSysTimeQFlag();
//         if(pus32Cnt)
//         {
//             *pus32Cnt=clkBenchCpy.ulUsCntBench+ulUsDiff;
//         }
//         tRet = EP_SUCCESS;
//     }
//     else
//     {
//         tRet = TM_Get_Sys_Us_UTC_Time(&tUtcTmNow,pus32Cnt);
//         pusUTCtmNow->ullusCntFrom1970 = tUtcTmNow.ullusCntFrom1970;
//         pusUTCtmNow->ucQflag = tUtcTmNow.ucQflag;
//     }

//     ulMicroSec = TM_Get_usCnt();
//     TM_Get_Sys_Time(&tdttmNow);

//     if(EP_GetSynMode()==MMI_SYN_SNTP_ADJ)
//     {
//         if(bSNTP60SecFlag==TRUE && tdttmNow.ucSec == 59)
//         {
//             pusUTCtmNow->ucIrigbLSFlag |= IRIGB_PLS_60SEC;
//         }
//     }
//     else
//     {
//         SYN_SetIrigBFlag(ulMicroSec, &tdttmNow);
//         pusUTCtmNow->ucIrigbLSFlag = tdttmNow.ucIrigbLSFlag;
//     }

//     return tRet;
// }

/*���ܣ�ƽ̨�ڲ������Ը����ڲ�ϵͳʱ��  2013-5-21  ZY
* ��������
* ���أ��ɹ����
* ע��: ������ж��������ԣ�Ҫ��160ms�ڣ����Ƕ�����ʵ������ʱ������100ms����,���ڷ�ֹ32λ�������ʱ��������á�
      �����ڲ�ʹ��
*/
EP_STATUS TM_Updt_Sys_Time()
{
    EP_CLOCK_HIGH_BENCH clkBenchNew;
    int iLockKey;
    EP_DATE_TIME   CurDttmNow;
    uint64_t   ullTimeBaseCnt;
    uint64_t   ullVal;

    // /*���ǵ�����ʱû�н��ⲿӲ��ʱ���õ�ǰʱ�����¸��»�׼����ֹ2��ʱ��ϵͳƫ��Խ��Խ��2013-6-25*/

    // iLockKey=EP_intLock();
    // TM_Get_Sys_Time(&CurDttmNow);
    // HW_TimeBase_Get_Base(&clkBenchNew.ulBaseHBench, &clkBenchNew.ulBaseLBench);
    // ullTimeBaseCnt=TM_Get_BaseTimerCnt();
    // EP_intUnlock(iLockKey);

    // ullVal=ullTimeBaseCnt*1000/(uint64_t)CALC_FREQ;	/* ת��Ϊ΢����� */
    // clkBenchNew.ulUsCntBench=(uint32_t)(ullVal&0xFFFFFFFF);		  /* ȡ��32λ,��32λ΢���������ֵ */

    // ullVal=ullVal/1000;
    // clkBenchNew.ulMsCntBench=(uint32_t)(ullVal&0xFFFFFFFF);    /* ȡ��32λ,��32λ�����������ֵ */

    // clkBenchNew.dttmBench=CurDttmNow;
    // /* clkBenchNew.dttmBench.ucQflag=0x0A;*/ //����0.9ms��
    // clkBenchNew.dttmBench.ucQflag = GetSysTimeQFlag();

    // TM_Time_To_Us_UTC_Time(&CurDttmNow,&clkBenchNew.usUTCtmBench);
    // /* clkBenchNew.usUTCtmBench.ucQflag=0x0A; */ //����0.9ms��
    // clkBenchNew.usUTCtmBench.ucQflag = GetSysTimeQFlag();

    // clkBenchNew.secUTCtmBench.ulSecCntFrom1970=(uint32_t)(clkBenchNew.usUTCtmBench.ullusCntFrom1970/1000000);
    // clkBenchNew.secUTCtmBench.ulUsRemainder=clkBenchNew.usUTCtmBench.ullusCntFrom1970-
    //                                         ((uint64_t)(clkBenchNew.secUTCtmBench.ulSecCntFrom1970))*((uint64_t)(1000000));
    // /* clkBenchNew.secUTCtmBench.ucQflag=0x0A; */ //����0.9ms��
    // clkBenchNew.secUTCtmBench.ucQflag = GetSysTimeQFlag();

    // iLockKey=EP_intLock();
    // clkBench=clkBenchNew;
    // EP_intUnlock(iLockKey);

    // bFstUpdtTimeBenchFlag_s=TRUE;

    return   EP_SUCCESS;

}



/*���ܣ��ڸ��������и�Чת���ڲ�UTC΢��ʱ��ΪMMS�ı�׼UTCʱ�䣬2013-5-22 ZY
  ������ pUsUtctm����ת�����ڲ�UTC΢��ʱ�������ַ
        pUtctm��ת�����MMS��UTCʱ��
  ���أ���
  ע�⣺ֻ���ڸ������񣬲������жϺͷǸ��������е���
      �����ڲ�ʹ��*/
void TM_High_Us_UTC_Time_To_MMS_UTC_Time(const US_CNT_UTC_TIME *pUsUtctm, MMS_UTC_TIME *pUtctm)
{

#define  MY_INT_MAX  ((int32_t)(0x7FFFFFFF))
#define  MY_INT_MIN  (-(int32_t)(0x7FFFFFFF))/*ʵ��32λ��С��������С1 */

    double temp = 0.0;
    int64_t  llUsDiff;
    int32_t  lUsDiff;
    int32_t lSecDiff;
    uint32_t ulUsLeft;
    int32_t lUtcAddSec;
    uint32_t ulUtcLeftUs;
    uint32_t  ulTemp;
    US_CNT_UTC_TIME  usUtcTmCpy;
    SEC_CNT_UTC_TIME  secUtcTmCpy;
    SEC_CNT_UTC_TIME  secUtcTmNew;
    int iLockKey;

    // assert(pUsUtctm!=NULL);
    // assert(pUtctm!=NULL);

    // if(!bFstUpdtTimeBenchFlag_s)
    // {
    //     /*����û�и��»�׼����������ʵ�� */
    //     Us_UTC_Time_To_MMS_UTC_Time(pUsUtctm, pUtctm);
    //     return ;
    // }
    // /*�������ݵ������ԣ���ֹ�����жϻ��������clkSys */
    // iLockKey=EP_intLock();
    // usUtcTmCpy=clkBench.usUTCtmBench;
    // secUtcTmCpy=clkBench.secUTCtmBench;
    // EP_intUnlock(iLockKey);

    // /*�ͻ�׼�ȣ�ע��ʱ�������ɸ�  */
    // llUsDiff=((int64_t)(pUsUtctm->ullusCntFrom1970))-((int64_t)(usUtcTmCpy.ullusCntFrom1970));

    // if((llUsDiff>=MY_INT_MIN)&&(llUsDiff<=MY_INT_MAX))
    // {
    //     /*����ֵ��32λ�з�������Χ�ڣ���32λ���㣬����64λ���� */
    //     lUsDiff=(int32_t)llUsDiff;
    //     if(lUsDiff>=0)
    //     {
    //         /*���ǲ�ֵΪ�� */
    //         lSecDiff=lUsDiff/1000000;
    //         ulUsLeft=lUsDiff-lSecDiff*1000000;
    //         if ((ulUsLeft+secUtcTmCpy.ulUsRemainder) >= 1000000)
    //         {
    //             /* ���λ(����֮��С��2s) */
    //             ulUtcLeftUs=ulUsLeft+secUtcTmCpy.ulUsRemainder-1000000;
    //             lUtcAddSec=lSecDiff+1;
    //         }
    //         else
    //         {
    //             ulUtcLeftUs=ulUsLeft+secUtcTmCpy.ulUsRemainder;
    //             lUtcAddSec=lSecDiff;
    //         }
    //     }
    //     else
    //     {
    //         /*���ǲ�ֵΪ�� */
    //         ulTemp=0-lUsDiff;
    //         lSecDiff=ulTemp/1000000;
    //         ulUsLeft=ulTemp-lSecDiff*1000000;
    //         if(ulUsLeft>secUtcTmCpy.ulUsRemainder)
    //         {
    //             /*���λ */
    //             ulUtcLeftUs=secUtcTmCpy.ulUsRemainder+1000000-ulUsLeft;
    //             lUtcAddSec=-lSecDiff-1;
    //         }
    //         else
    //         {
    //             ulUtcLeftUs=secUtcTmCpy.ulUsRemainder-ulUsLeft;
    //             lUtcAddSec=-lSecDiff;
    //         }
    //     }
    //     secUtcTmNew.ulSecCntFrom1970=secUtcTmCpy.ulSecCntFrom1970+lUtcAddSec;
    //     secUtcTmNew.ulUsRemainder=ulUtcLeftUs;
    // }
    // else
    // {
    //     /*����ֵ���������64λ���� */

    //     secUtcTmNew.ulSecCntFrom1970=pUsUtctm->ullusCntFrom1970/1000000;
    //     secUtcTmNew.ulUsRemainder=pUsUtctm->ullusCntFrom1970-
    //                               secUtcTmNew.ulSecCntFrom1970*1000000;
    // }

    // /*ת����� */
    // pUtctm->secs=secUtcTmNew.ulSecCntFrom1970;

    // temp = (double)(secUtcTmNew.ulUsRemainder) / 1000000 * (INT32)0x00FFFFFF;
    // if(temp > LONG_MAX)
    // {
    //     pUtctm->fraction= LONG_MAX;
    // }
    // else if(temp < LONG_MIN)
    // {
    //     pUtctm->fraction= LONG_MIN;
    // }
    // else
    // {
    //     pUtctm->fraction= (INT32)(temp + 0.5);
    // }
    // if(pUsUtctm->ucQflag!=0x00)
    // {
    //     pUtctm->qflags = pUsUtctm->ucQflag;
    // }
    // else
    // {
    //     pUtctm->qflags = 0x0A;
    // }
}

/*���ܣ��ڸ��������и�Чת��MMS�ı�׼UTCʱ��Ϊ�ڲ�UTC΢��ʱ�䣬2013-5-22 ZY
  ������ pUtctm����ת����MMS��UTCʱ��
        pUsUtctm��ת������ڲ�UTC΢��ʱ�������ַ
  ���أ���
  ע�⣺ֻ���ڸ��������е��ã��������жϺͷǸ��������е���
      �����ڲ�ʹ��*/
void TM_High_MMS_UTC_Time_To_US_UTC_TIME(const MMS_UTC_TIME *pUtctm, US_CNT_UTC_TIME *pUsUtctm)
{
    double tmTemp;
    int32_t  lUsDiff;
    int32_t lSecDiff;
    US_CNT_UTC_TIME  usUtcTmCpy;
    SEC_CNT_UTC_TIME  secUtcTmCpy;
    SEC_CNT_UTC_TIME  secUtcTmNew;
    int iLockKey;

    assert(pUtctm!=NULL);
    assert(pUsUtctm!=NULL);


    // if(!bFstUpdtTimeBenchFlag_s)
    // {
    //     /*����û�и��»�׼����������ʵ�� */
    //     MMS_UTC_Time_To_US_UTC_TIME(pUtctm, pUsUtctm);
    //     return ;
    // }


    // iLockKey=EP_intLock();
    // usUtcTmCpy=clkBench.usUTCtmBench;
    // secUtcTmCpy=clkBench.secUTCtmBench;
    // EP_intUnlock(iLockKey);

    // secUtcTmNew.ulSecCntFrom1970=pUtctm->secs;
    // tmTemp=(double)pUtctm->fraction*1000000/((INT32)0x00FFFFFF);
    // if(tmTemp>999999)
    // {
    //     secUtcTmNew.ulUsRemainder=999999;
    // }
    // else if(tmTemp<0)
    // {
    //     secUtcTmNew.ulUsRemainder=0;
    // }
    // else
    // {
    //     secUtcTmNew.ulUsRemainder=tmTemp;
    // }
    // /*�ͻ�׼�ȣ������ɸ� */
    // if((secUtcTmNew.ulSecCntFrom1970<(secUtcTmCpy.ulSecCntFrom1970+1000))
    //         &&(secUtcTmNew.ulSecCntFrom1970>(secUtcTmCpy.ulSecCntFrom1970-1000)))
    // {
    //     /*����ֵ��������32λ����,ȷ��us��ֵ���㲻��Խ�� */
    //     if(secUtcTmNew.ulSecCntFrom1970>=secUtcTmCpy.ulSecCntFrom1970)
    //     {
    //         lSecDiff=secUtcTmNew.ulSecCntFrom1970-secUtcTmCpy.ulSecCntFrom1970;
    //     }
    //     else
    //     {
    //         lSecDiff=(int32_t)(secUtcTmCpy.ulSecCntFrom1970-secUtcTmNew.ulSecCntFrom1970);
    //         lSecDiff=0-lSecDiff;
    //     }
    //     lUsDiff=((int32_t)secUtcTmNew.ulUsRemainder)-((int32_t)secUtcTmCpy.ulUsRemainder);
    //     /*���΢���ֵ����Ϊus�� */
    //     lUsDiff=lUsDiff+lSecDiff*1000000;
    //     pUsUtctm->ullusCntFrom1970=usUtcTmCpy.ullusCntFrom1970+(int64_t)lUsDiff;
    // }
    // else
    // {
    //     /*����ֵ��������64λ���� */
    //     pUsUtctm->ullusCntFrom1970=secUtcTmNew.ulUsRemainder;
    //     pUsUtctm->ullusCntFrom1970 += ((uint64_t)(secUtcTmNew.ulSecCntFrom1970)*1000000);
    // }

    // pUsUtctm->ucQflag=pUtctm->qflags;

}

/*���ܣ���Чת���ڲ�UTC΢��ʱ��Ϊ�ڲ�32λʱ�䣬2013-5-22 ZY
  ������ uttime����ת�����ڲ�UTC΢��ʱ��
        pbRet��ת���ɹ���������ַ
  ���أ�ת������ڲ�32λ΢����������
  ע�⣺�����жϺ������е���
      �����ڲ�ʹ��*/
uint32_t TM_High_Us_UTC_Time_To_us32Cnt(US_CNT_UTC_TIME uttime, BOOL *pbRet)
{
#define  MY_INT_MAX  ((int32_t)(0x7FFFFFFF))
#define  MY_INT_MIN  (-(int32_t)(0x7FFFFFFF))/*ʵ��32λ��С��������С1 */

    int64_t  llUsDiff;
    int32_t  lUsDiff;
    uint32_t  ulTemp;
    US_CNT_UTC_TIME  usUtcTmCpy;
    uint32_t  ulNewUsCnt;
    uint32_t  ulUsCntCpy;
    int iLockKey;

    // assert(pbRet);

    // if(!bFstUpdtTimeBenchFlag_s)
    // {
    //     /*����û�и��»�׼����������ʵ�� */
    //     return(Us_UTC_Time_To_us32Cnt(uttime, pbRet)) ;
    // }

    // *pbRet=TRUE;

    // /*if((uttime.ucQflag&0x60)!=0)*/
    // if(uttime.ullusCntFrom1970==0)
    // {
    //     *pbRet=FALSE;
    //     return 0;
    // }

    // iLockKey=EP_intLock();
    // usUtcTmCpy=clkBench.usUTCtmBench;
    // ulUsCntCpy=clkBench.ulUsCntBench;
    // EP_intUnlock(iLockKey);

    // llUsDiff=((int64_t)uttime.ullusCntFrom1970)-((int64_t)usUtcTmCpy.ullusCntFrom1970);
    // if((llUsDiff<MY_INT_MAX)&&(llUsDiff>MY_INT_MIN))
    // {
    //     /*����ֵ��32λ�з�������Χ�ڣ���32λ����*/
    //     lUsDiff=(int32_t)llUsDiff;
    //     if(lUsDiff>=0)
    //     {
    //         ulTemp=(uint32_t)lUsDiff;
    //         ulNewUsCnt=ulUsCntCpy+ulTemp;
    //         return  	ulNewUsCnt;
    //     }
    //     else
    //     {
    //         ulTemp=(uint32_t)(0-lUsDiff);
    //         ulNewUsCnt=ulUsCntCpy-ulTemp;
    //         return  	ulNewUsCnt;
    //     }
    // }
    // else
    {
        /*����ֵ��������Ϊ������Ч */
        *pbRet=FALSE;
        return  0;
    }
}

/*����TIMEBASE�Ĳ�,���USʱ���  */
int32_t   OptGetUsIntvlByBaseDiff(int64_t   llBaseDiff)
{
    int64_t lUs;

    /* int64_t ��ֵ��ΧΪ -9,223,372,036,854,775,808 ~ +9,223,372,036,854,775,807��Ϊ�˼��ټ��㾫����ʧ����ֹ�����
      ��llBaseDiff �ľ���ֵС��9223372036854ʱ�������ȳ��ٳ������򣬲����ȳ��ٳ˵ļ��㷽����*/
    if((llBaseDiff > -9223372036854ll) && (llBaseDiff < 9223372036854ll))
    {
        lUs = (llBaseDiff*(int64_t)1000000)/((int64_t)OPT_TIME_BASE_FREQ);
    }
    else
    {
        lUs = (llBaseDiff/((int64_t)OPT_TIME_BASE_FREQ)) *(int64_t)1000000;
    }
    if (lUs>INT_MAX)
    {
        return INT_MAX;
    }
    else if (lUs<INT_MIN)
    {
        return INT_MIN;
    }
    else
    {
        return (int32_t)lUs;
    }
}

/*TIMEBASE�����,���TIMEBASEʱ���  ,*/
int64_t   OptGetBaseDiff(OPT_TIME_BASE  *pBaseA,OPT_TIME_BASE  *pBaseB)
{
#define MAX_INT64_VAL 0x7FFFFFFFFFFFFFFFll  /* �����з���64λ�� */
#define MIN_INT64_VAL 0x8000000000000000ll  /* ��С���з���64λ�� */

    uint64_t   ullA;
    uint64_t   ullB;
    ullA=(((uint64_t)(pBaseA->ulTimeBaseH))<<32)+(uint64_t)(pBaseA->ulTimeBaseL);
    ullB=(((uint64_t)(pBaseB->ulTimeBaseH))<<32)+(uint64_t)(pBaseB->ulTimeBaseL);

    /* ����uint64_t ת��Ϊint64_t ʱ������� */
    if(ullA > (ullB + MAX_INT64_VAL))
    {
        return (int64_t)MAX_INT64_VAL;
    }
    else if(ullB > (ullA + (uint64_t)MIN_INT64_VAL))
    {
        return (int64_t)MIN_INT64_VAL;
    }
    else
    {
        return (int64_t)(ullA-ullB);
    }
}

/*TIMEBASE�����,���USʱ���
  A�Ǳ�������B�Ǽ��� ,2013-5-27 ZY�Ż�, */

int32_t    OptGetUsIntvlByBase(OPT_TIME_BASE  *pBaseA,OPT_TIME_BASE  *pBaseB)
{
    return  OptGetUsIntvlByBaseDiff(OptGetBaseDiff(pBaseA,pBaseB));
}

#ifndef EDP01_CA_EXT_BUILD
/* ��ȡ��ʱ�ӿ�״̬.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
LW_SYMBOL_EXPORT BOOL TM_GetSynIntSts(void)
{
//     if(g_eBdLevelType == Board_LEVEL_03)
//     {
//         return bSynIntSts;
//     }
//     else if( g_bIrigBFpgaMode == TRUE)
//     {
//         return SYN_GetTimeSynState();
//     }
//     else
//     {
//         return g_TimeSynIntSts;
//     }
}

/* ��ȡ��ʱ����״̬.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
LW_SYMBOL_EXPORT BOOL TM_GetSynServSts(void)
{
    // if(g_eBdLevelType == Board_LEVEL_03)
    // {
    //     return bSynServSts;
    // }
    // else if( g_bIrigBFpgaMode == TRUE)
    // {
    //     return SYN_GetTimeSynService();
    // }
    // else
    // {
    //     return g_TimeServeSts;
    // }
}

/* ��ȡʱ���������״̬.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
LW_SYMBOL_EXPORT BOOL TM_GetSynLeapSts(void)
{
    // if(g_eBdLevelType == Board_LEVEL_03)
    // {
    //     return bTimeLeapSts;
    // }
    // else if( g_bIrigBFpgaMode == TRUE)
    // {
    //     return SYN_GetTimeSynJump();
    // }
    // else
    // {
    //     return g_TimeLeapSts;
    // }
}

#endif

/* Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL TM_GetSecPluseSts(void)
{
    return s_bSecPulseFlag;
}

/**
 * @brief ʹ������������(����ͬ����ֵѹ���޸�)
 * @param NONE.
 * @return NONE.
 * @ref
 * @see
 * @note
 * @warning
 */
void TM_EnableSetSecPluse()
{
    s_bEableSetSecPulse = TRUE;
}

/* ����������״̬
 * Para:
 *     bSecPulseFlag, ����״̬.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL TM_SetSecPluseSts(BOOL bSecPulseFlag)
{
    s_bSecPulseFlag = bSecPulseFlag;
    g_ulPollSetChgCnt = 0; /* ������ʱ�ж� */

    return TRUE;
}

/**
 * @brief ʹ�������µ�һ�뵽��(����ͬ����������)
 * @param NONE.
 * @return NONE.
 * @ref
 * @see
 * @note
 * @warning
 */
void TM_EnableSetNewSec()
{
    s_bEnableSetNewSec = TRUE;
}

/* ����:    ��ѯ�Ƿ��µ�һ������
 * ����:    pelm, �������ͼԪ�ṹ��Ϣ.
 * ����:    None. */
BOOL GetNewSecFlag()
{
    /*���ڸ߱��洫��,����CPUͬ������*/
    return bNewSecFlag;
}

/* ����:    ����µ�һ��������ʶ
 * ����:    pelm, �������ͼԪ�ṹ��Ϣ.
 * ����:    None. */
void ClearNewSecFlag()
{
    bNewSecFlag = FALSE;
}

void TimeZoneInit()
{
    int lFile;
    char buf[32];

    // TimeZone_g=20;/*Ĭ�ϱ���ʱ��*/

    // lFile = open(EP_MMISET_FILE, O_RDONLY, 0);
    // if (lFile != ERROR)	    /* �ļ���ʧ�� */
    // {
    //     if (read(lFile, buf, 24) ==24)
    //     {
    //         if(buf[20]<25)
    //         {
    //             TimeZone_g=buf[20];
    //         }
    //     }
    //     close(lFile);
    // }
}

int GetTimeZoneInitFunc()
{
    int retcode=8;

    if(TimeZone_g<25)
    {
        retcode=TimeZone_g;
        retcode-=12;
    }

    return retcode;
}

/* ��ʼ��DateTime �豸.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS TM_InitDatetimeDev()
{
    EP_STATUS res = EP_ERROR;
    // DATETIME_DEV_S *pDatetimeDev =NULL;
    // int i = 0;

    // do
    // {
    //     pDatetimeDev = (DATETIME_DEV_S *)DescriptionGetByType(SAC_DEVICE_TYPE_DATETIME, (SAC_DEV_HEADER_ID)pDatetimeDev);
    //     if(pDatetimeDev != NULL)
    //     {
    //         g_iDatetimeDevice = DeviceRequest((SAC_DEV_HEADER_ID)pDatetimeDev);

    //         if (g_iDatetimeDevice <= 0)
    //         {
    //             logMsg("Datetime �豸��ȡʧ��\n", 1, 2, 3, 4, 5, 6);
    //             res = EP_ERROR;
    //             goto exit;
    //         }
    //         else
    //         {
    //             res = EP_SUCCESS;
    //         }
    //     }
    //     else
    //     {
    //         break;
    //     }
    //     i++;
    // }
    // while(1);

    // if(i == 0)
    // {
    //     logMsg("��Datetime �豸\n", 1, 2, 3, 4, 5, 6);
    //     res = EP_ERROR;
    //     goto exit;
    // }

exit:
    return res;
}

/* DateTime �豸�ص�����.
 * Para:
 *     dateTime: The current date and time.
 *     ulCtrl: The control data.
 * Return:
 *     void.
 */
void TM_Datetimehook(UINT32 uldateTime, UINT32 ulCtrl)
{
    GPS_ISR();
}

/* ��ʼ��IRIGB �豸.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS TM_InitIrigbDev()
{
    EP_STATUS res = EP_ERROR;
    // IRIGB_DEV_S *pIrigbDev =NULL;
    // int i = 0;

    // do
    // {
    //     pIrigbDev = (IRIGB_DEV_S *)DescriptionGetByType(SAC_DEVICE_TYPE_IRIGB, (SAC_DEV_HEADER_ID)pIrigbDev);
    //     if(pIrigbDev != NULL)
    //     {
    //         g_iIrigbDevice = DeviceRequest((SAC_DEV_HEADER_ID)pIrigbDev);

    //         if (g_iIrigbDevice <= 0)
    //         {
    //             logMsg("IRIGB Dev Request Err\n", 1, 2, 3, 4, 5, 6);
    //             res = EP_ERROR;
    //             goto exit;
    //         }
    //         else
    //         {
    //             res = EP_SUCCESS;
    //         }
    //     }
    //     else
    //     {
    //         break;
    //     }
    //     i++;
    // }
    // while(1);

    // if(i == 0)
    // {
    //     logMsg("No IRIGB Device\n", 1, 2, 3, 4, 5, 6);
    //     res = EP_ERROR;
    //     goto exit;
    // }

exit:
    return res;
}

/* IRIGB �豸�ص�����.
 * Para:
 *     pIrigbInfo: B ����Ϣ.
 * Return:
 *     void.
 */
// void TM_Irigbhook(IRIGB_INFO_S *pIrigbInfo)
// {
//     GPS_ISR();
// }

/* IRIGB �豸�ص�����.(��FPGA����)
 * Para:
 *     pIrigbInfo: B ����Ϣ.
 * Return:
 *     void.
 */
// void TM_Irigbhook_Fpga(IRIGB_INFO_S *pIrigbInfo)
// {
// #define CHECKTIMEROK_TIMES 5 /* ȷ�϶�ʱ�������� */

//     uint32_t uiLocalSec;/*����ʱ�������ֵ*/
//     EP_DATE_TIME Basedttm;
//     EP_DATE_TIME Resultdttm;
//     IRIG_BTime IRIGTime ;
//     uint32_t ulSec;

//     if(pIrigbInfo->year == 0)
//     {
//         return;
//     }

//     IRIGTime.cSecond=(pIrigbInfo->sec&0x0f)+((pIrigbInfo->sec>>4)&0x0f)*10;
//     IRIGTime.cMinute=(pIrigbInfo->min&0x0f)+((pIrigbInfo->min>>4)&0x0f)*10;
//     IRIGTime.cHour=(pIrigbInfo->hour&0x0f)+((pIrigbInfo->hour>>4)&0x0f)*10;
//     IRIGTime.iDay=(pIrigbInfo->day&0x0f)
//                   +((pIrigbInfo->day>>4)&0x0f)*10
//                   +((pIrigbInfo->day>>8)&0x0f)*100;
//     IRIGTime.cYear=pIrigbInfo->year+pIrigbInfo->year_10x*10;

//     IRIGTime.LSP = pIrigbInfo->leap_sec_notice;
//     IRIGTime.LS = pIrigbInfo->leap_sec;
// #ifndef EDP01_CA_EXT_BUILD
//     quality_time = IRIG_TO_UTC_TIME(pIrigbInfo->quality);
// #endif
//     if (IRIGTime.iDay >= 0)
//     {
//         if(TM_Get_Sys_Time(&Basedttm) == EP_ERROR)
//         {
//             LOG_Dbg_Msg("IRIG-B:��ȡ�ڲ�ʱ�Ӵ���\n",0,0,0,0,0,0);
//             return;
//         }
//         uiLocalSec=TM_Time_To_Long(&Basedttm);
//         if(Basedttm.unMSEL > 900)
//         {
//             uiLocalSec++;
//         }
//         ulSec=(IRIGTime.iDay-1)*24*3600+IRIGTime.cHour*3600+IRIGTime.cMinute*60+IRIGTime.cSecond+1;//����1s,FPGA�д�ŵ�����һ��ʱ��
//         Basedttm.ucMonth=1;
//         Basedttm.ucDate=1;
//         Basedttm.ucHour=0;
//         Basedttm.ucMinute=0;
//         Basedttm.ucSec=0;
//         Basedttm.unMSEL=0;
//         Basedttm.unMicroSec=0;
//         if(IRIGTime.cYear>0)
//         {
//             Basedttm.unYear = IRIGTime.cYear+2000 ;
//         }

//         TM_Calc_Time(&Basedttm,&Resultdttm,0,ulSec);
//     }

//     bGetPulseSetTime=TRUE;

//     if(Resultdttm.unYear>2035)//�ж�ʱ��Ϸ���
//     {
//         LOG_Dbg_Msg("IRIG-B:ʱ�䲻�Ϸ�\n",0,0,0,0,0,0);
//         return;
//     }

//     TM_Set_Sys_Time(&Resultdttm, TRUE);
// }

/* ����:
 *      ����HMI���ö�ʱ״̬��־λTRUE,����ʱ��Ʒ�ʵ���Դ�л�
 * ����:
 *      ��.
 * ����:
 *      ��.
 */
void TM_SetHmiSetQFlag()
{
    if(!nbHmiSetQFlag)
    {
        nbHmiSetQFlag = TRUE;
    }

    return;
}
/*******************************************************************************
 �������� :BCD_TO_DEC
 ����˵�� :BCD��תʮ����
 ����˵�� :BCD��
 ����˵�� :ʮ������
 �޸ļ�¼ :2018/11/13:Dote Create
*******************************************************************************/
uint8_t BCD_TO_DEC(
    uint8_t bcd		/* BCD��*/
)
{
    return ((bcd & 0x0F) + ((bcd >> 4) * 10)) ;
}
/* ת��32λus������������ʱ��
 * ������
 *      ulMicroSec����ת����us������ֵ
 *      pdttm�����ת�����
 * ����ֵ��
 *      ��
 * ע�⣺
 *      B���߼��������������롣
 */
EP_STATUS TM_To_Dttm_NEW(uint32_t ulMicroSec, EP_DATE_TIME *pdttm)
{
    // int32_t lUsStep, lSecStep;
    // EP_DATE_TIME dttmNow;
    // uint32_t ulSysUs;

    // TM_Get_Sys_Time_And_Us_Cnt(&dttmNow, &ulSysUs);	/* ��ȡϵͳ��ǰʱ�� */

    // if(ulSysUs>ulMicroSec)
    // {
    //     if((ulSysUs-ulMicroSec)<=0x7FFFFFFF)
    //         lUsStep=-(int32_t)(ulSysUs-ulMicroSec);
    //     else
    //         lUsStep=(int32_t)(ulMicroSec-ulSysUs);
    // }
    // else
    // {
    //     if((ulMicroSec-ulSysUs)<=0x7FFFFFFF)
    //         lUsStep=(int32_t)(ulMicroSec-ulSysUs);
    //     else
    //         lUsStep=-(int32_t)(ulSysUs-ulMicroSec);
    // }

    // lSecStep=lUsStep/1000000;		/* �� */
    // lUsStep=lUsStep%1000000;				/* ΢�� */
    // TM_Calc_Time(&dttmNow, pdttm, lUsStep, lSecStep);

    return EP_SUCCESS;
}

/***********************************************************************
* SYN_Set_Time - ����ʱ�估���������־
*/
void SYN_Set_Time(void)
{
    // IRIGB_INFO_S GPSGTime;
    // IRIG_BTime_N Innertime;
    // FPGA_IrigBTime IRIGTimeDate;
    // static BOOL Bfirst = 1;
    // static uint32_t preTime = 0;
    // uint32_t diffus = 0;
    // EP_DATE_TIME Basedttm;
    // EP_DATE_TIME Resultdttm;
    // EP_DATE_TIME Localdttm;
    // EP_DATE_TIME Curdttm;
    // BOOL bIrigbState = FALSE;
    // static uint32_t bIrigOKcnt = 0;          /*B���ź�OK����*/
    // static uint32_t bIrigERcnt = 0;          /*B���ź�ER����*/
    // static uint32_t bIrigcnt = 0;          /*B���ź�OK����*/
    // static uint32_t bTimeErrDelayCnt = 0;    /*��ʱ�����ź��쳣���ӳٵȴ�����*/
    // uint32_t ulSec;
    // uint32_t ulstartus;
    // uint32_t ulcurrtus;
    // int32_t DeltMicroSec;

    // /*IRIGBGet��ȡB��ʱ��*/
    // IRIGBGet(g_iSynInnerDevice,&GPSGTime);
    // ulstartus = TM_Get_usCnt();
    // Innertime.cSecond = BCD_TO_DEC(GPSGTime.sec);
    // Innertime.cMinute = BCD_TO_DEC(GPSGTime.min);
    // Innertime.cHour = BCD_TO_DEC(GPSGTime.hour);
    // Innertime.iDay = ((GPSGTime.day >> 8) & 0x03) * 100 + ((GPSGTime.day >> 4) & 0x0F) * 10 + (GPSGTime.day & 0x0F);
    // Innertime.cYear = GPSGTime.year + GPSGTime.year_10x * 10;
    // Innertime.unFlags.bReg1_st.irigb_status = GPSGTime.irigb_status;
    // Innertime.unFlags.bReg1_st.sync_status = GPSGTime.sync_status;
    // Innertime.unFlags.bReg1_st.timeflag = GPSGTime.timeflag;
    // Innertime.unFlags.bReg1_st.check = GPSGTime.parity;
    // Innertime.unFlags.bReg1_st.TimeQ = GPSGTime.quality;
    // Innertime.unFlags.bReg1_st.TimeOffsetHalfHour = GPSGTime.shift_half_hour;
    // Innertime.unFlags.bReg1_st.TimeOffsetHour = GPSGTime.shift_hour;
    // Innertime.unFlags.bReg1_st.TimeOffsetSymbol = GPSGTime.shift_sign;
    // Innertime.unFlags.bReg1_st.dst = GPSGTime.dst;
    // Innertime.unFlags.bReg1_st.dsp = GPSGTime.dst_notice;
    // Innertime.unFlags.bReg1_st.ls = GPSGTime.leap_sec;
    // Innertime.unFlags.bReg1_st.lsp = GPSGTime.leap_sec_notice;
    // Innertime.unFlags.bReg1_st.YearTens = GPSGTime.year_10x;
    // Innertime.unFlags.bReg1_st.Indexbit = GPSGTime.index;
    // Innertime.unFlags.bReg1_st.YearUnits = GPSGTime.year;
    // Innertime.unFlagsE.bReg2_st.delta_ns = GPSGTime.ns & 0x3FFFFFFF;
    // Innertime.unFlagsE.bReg2_st.irig_status = (GPSGTime.ns & 0xC0000000) >> 30;
    // /*ͨ��Unionȡ��B��Ĵ������ݣ����Inner������Ч��*/
    // IRIGTimeDate.bReg0_un_N.bReg0_st.Sec = GPSGTime.sec;
    // IRIGTimeDate.bReg0_un_N.bReg0_st.Min = GPSGTime.min;
    // IRIGTimeDate.bReg0_un_N.bReg0_st.Hour = GPSGTime.hour;
    // IRIGTimeDate.bReg0_un_N.bReg0_st.Day = GPSGTime.day;
    // g_Innertime = Innertime;
    // if (0)
    // {
    //     uint32_t delta_ns = Innertime.unFlagsE.bReg2_st.delta_ns;
    //     uint32_t synstatus = GPSGTime.irigb_status * 2 + GPSGTime.sync_status;
    //     logMsg("IRIGBGet ��ȡʱ��:%d-%d-%d-%d. nS:%d . status:%d \n",
    //            Innertime.iDay, Innertime.cHour, Innertime.cMinute, Innertime.cSecond,
    //            delta_ns, synstatus);
    //     logMsg(" GPSGTime.timeflag:0x%x===reg1:0x%08x=���룺%d==Ԥ�棺%d\n",GPSGTime.timeflag, Innertime.unFlags.ulBReg1
    //            ,Innertime.unFlags.bReg1_st.ls,Innertime.unFlags.bReg1_st.lsp,5,6);
    // }
    
    // bIrigcnt++;

    // /*�����ʱ�źŶ��������·�ʱ�����䣬��ʱǰ3֡����������ϵͳʱ��*/
    // bIrigbState = GPSGTime.timeflag & IRIGB_SYN_OKSTATUE;
    // if ((bIrigbState & 0x4) == 0x4 && bIrigOKcnt < 3) /*�ָ�ǰ3֡����ʱ(������Ϊǰ��֡)*/
    // {
    //     logMsg("####�ָ�B��3���ڲ���ʱ----bIrigbState:%d---- bIrigOKcnt:%d \n", bIrigbState, bIrigOKcnt, 0, 0, 0, 0);
    //     bIrigOKcnt++ ;
    //     return;
    // }
    // else if ((bIrigbState & 0x4) != 0x4)
    // {
    //     bIrigOKcnt = 0;
    // }
    // if ((bIrigbState & 0x4) != 0x4 && bIrigERcnt < 3) /*ʧЧǰ3֡����ʱ(������Ϊǰ��֡)*/
    // {
    //     logMsg("####�Ͽ�B��3���ڲ���ʱ----bIrigbState:%d---- bIrigERcnt:%d \n", bIrigbState, bIrigERcnt, 0, 0, 0, 0);
    //     bIrigERcnt++ ;
    //     return;
    // }
    // else if ((bIrigbState & 0x4) == 0x4)
    // {
    //     bIrigERcnt = 4;
    // }

    // /*�ж�Inner �·������Ƿ���Ч*/
    // if (Innertime.unFlags.ulBReg1 == 0x00000000)
    // {
    //     bTimeErrDelayCnt = TIME_ERR_DELAY_SEC_CNT;
    //     if((bIrigcnt % 200) == 1)
    //     {
    //         LOG_Dbg_Msg("####11 SYN_IrigBHook_Inner ����0x00000000ʱ�䣬���� \n", 0, 0, 0, 0, 0, 0);
    //     }
    //     return;
    // }
    // else if (((IRIGTimeDate.bReg0_un_N.ulBReg0 & 0x4fffffff) == 0)
    //          || ((Innertime.unFlags.ulBReg1 & 0x07ffffff) == 0))
    // {
    //     /* ����FPGA�Ĵ�������,�������ж�Ϊ0ʱ,��ʾ�Ĵ���Ϊ��ʼ״̬δ������,��ʹ�üĴ���ʱ�� */
    //     bTimeErrDelayCnt = TIME_ERR_DELAY_SEC_CNT;
    //     //LOG_Dbg_Msg("FPGA��ʱ���߼Ĵ���״̬��Ч, ulBReg0=0x%x ulBReg1=0x%x \n",IRIGTimeDate.bReg0_un_N.ulBReg0,Innertime.unFlags.ulBReg1,0,0,0,0);
    //     return;
    // }
    // else if(bTimeErrDelayCnt > 0)
    // {
    //     /** �ڶ�ʱ����ʱ���쳣�ȴ����ڣ���ʹ������ʱ�� */
    //     bTimeErrDelayCnt--;
    //     return;
    // }

    // /*����1s,��ʱ����������һ��ʱ��*/
    // ulSec = (Innertime.iDay - 1) * 24 * 3600 + Innertime.cHour * 3600 + Innertime.cMinute * 60 + Innertime.cSecond + 1;
    // Basedttm.ucMonth = 1;
    // Basedttm.ucDate = 1;
    // Basedttm.ucHour = 0;
    // Basedttm.ucMinute = 0;
    // Basedttm.ucSec = 0;
    // Basedttm.unMSEL = 0;
    // Basedttm.unMicroSec = 0;
    // if ((Innertime.cYear > 0) && (Innertime.cYear != 70))
    // {
    //     Basedttm.unYear = Innertime.cYear + 2000 ;
    // }
    // else if (Innertime.cYear == 70)
    // {
    //     Basedttm.unYear = Innertime.cYear + 1900 ;
    // }
    // else
    // {
    //     Basedttm.unYear = 2000 ;
    // }

    // /*B���е�ʱ��Ʒ��Ϊ0���ʾʱ������,�����ʾ�쳣*/
    // if (Innertime.unFlags.bReg1_st.TimeQ == 0)
    // {
    //     Basedttm.ucQflag = 0;
    // }
    // else
    // {
    //     Basedttm.ucQflag = 0x60;
    // }

    // /*����õ�׼ȷʱ��*/
    // TM_Calc_Time(&Basedttm, &Resultdttm, 0, ulSec);
    // Resultdttm.unMSEL = Innertime.unFlagsE.bReg2_st.delta_ns / 1000000;
    // Resultdttm.unMicroSec = (Innertime.unFlagsE.bReg2_st.delta_ns / 1000 - ((Innertime.unFlagsE.bReg2_st.delta_ns / 1000000)
    //                          * 1000));
    // /* �����ʱ��500~600ms֮������ʱ�� */
    // if ((Innertime.unFlagsE.bReg2_st.delta_ns < IRIGB_UP_NS && Innertime.unFlagsE.bReg2_st.delta_ns > IRIGB_DOWN_NS)
    //         && g_bLeapSecondCpuClearFlag)
    // {
    //     ulcurrtus = TM_Get_usCnt();
    //     DeltMicroSec = ulcurrtus - ulstartus;
    //     TM_Calc_Time(&Resultdttm, &Localdttm, DeltMicroSec, 0);
    //     TM_Set_Sys_Time(&Localdttm, FALSE);
    //     /*logMsg("########Dote:����ϵͳʱ��--��-ʱ-��-��-����-΢��: %d-%d-%d-%d-%d-%d\n", Localdttm.ucDate, Localdttm.ucHour,
    //                Localdttm.ucMinute, Localdttm.ucSec, Localdttm.unMSEL, Localdttm.unMicroSec);*/
    //     SetAdjustTimeSuccessFlag(TRUE);
    // }

    // TM_Get_Sys_Time(&Curdttm);
    // ulCurSecFrom1980 = TM_Time_To_Long(&Curdttm);

    // /* ���봦������ */
    // if((Resultdttm.ucDate != 1) || (Resultdttm.ucMonth != 7 && Resultdttm.ucMonth != 1))
    // {
    //     return;
    // }
    // if(!((Resultdttm.ucHour == 7 && Resultdttm.ucMinute == 59 && Resultdttm.ucSec > 50)
    //         || (Resultdttm.ucHour == 8 && Resultdttm.ucMinute == 00 && Resultdttm.ucSec < 50)))
    // {
    //     return;
    // }

    // if(Innertime.unFlags.bReg1_st.lsp)
    // {
    //     SYN_SetLsSpecialFlag();
    //     if(Innertime.unFlags.bReg1_st.ls)
    //     {
    //         IRIGB_NLS_Flag = 1;
    //         IRIGB_PLS_Flag = 0;
    //         if(Innertime.cSecond == 56 && Bfirst) /* �����뿨����� */
    //         {
    //             diffus = 2000000 - (Innertime.unFlagsE.bReg2_st.delta_ns/1000);
    //             g_ulLSUsBeginCnt = TM_Get_usCnt() + diffus;
    //             TM_To_Dttm_NEW(g_ulLSUsBeginCnt,&g_tLSDataTime);
    //             Bfirst = 0;
    //             preTime = tickGet();
    //         }
    //     }
    //     else
    //     {
    //         IRIGB_PLS_Flag = 1;
    //         IRIGB_NLS_Flag = 0;
    //         //printf("===dote==$$$$==>Innertime.cSecond:%d\n",Innertime.cSecond);
    //         if(Innertime.cSecond == 57 && Bfirst) /* �����뿨����� */
    //         {
    //             diffus = 2000000 - (Innertime.unFlagsE.bReg2_st.delta_ns/1000);
    //             g_ulLSUsBeginCnt = TM_Get_usCnt() + diffus;
    //             g_ulLSUsEndCnt = g_ulLSUsBeginCnt + 1000000;
    //             TM_To_Dttm_NEW(g_ulLSUsEndCnt,&g_tLSDataTime);
    //             Bfirst = 0;
    //             preTime = tickGet();
    //         }
    //     }
    // }
    // if(!Bfirst)
    // {
    //     if((tickGet()-preTime)>=4200)/* 42s���������־ */
    //     {
    //         EP_DATE_TIME  sysTime ;
    //         if(TM_Get_Sys_Time(&sysTime)==EP_SUCCESS)
    //         {
    //             TM_Set_Sys_Time(&sysTime,FALSE);
    //             IRIGB_NLS_Flag = 0;
    //             IRIGB_PLS_Flag = 0;
    //             SYN_ClearLsFlag();
    //             Bfirst = 1;
    //         }
    //         preTime = tickGet();
    //     }
    // }
}

/***********************************************************************
* IrigB_Loop - B�������־����
*
* RETURNS: ��
*
*/

void IrigB_Loop(void)
{
    EP_DATE_TIME datetimeN;

    while(TRUE)
    {
        taskDelay(IRIGB_LOOP_TASK_DELAY_CNT);
        SYN_Set_Time();
        if(0)
        {
            TM_Get_Sys_Time_LS(&datetimeN);
            logMsg("===RTC===>ʱ�䣺%d-%d-%d-%d-%d-%d\n",datetimeN.unYear,datetimeN.ucMonth,datetimeN.ucDate,
                   datetimeN.ucHour,datetimeN.ucMinute,datetimeN.ucSec);
            logMsg("====>IRIGB_PLS_Flag:%d====>IRIGB_NLS_Flag:%d\n",IRIGB_PLS_Flag,IRIGB_NLS_Flag
                   ,3,4,5,6);
        }
    }
}

/*******************************************************************************
 �������� :HW_SYN_Init
 ����˵�� :INNER��ʱ��ʼ����ں���
 ����˵�� :��
 ����˵�� :NP_STATUS:���س�ʼ������Ƿ���ȷ
 �޸ļ�¼ :2019/12/11:kevin Create
*******************************************************************************/
// NP_STATUS HW_SYN_Init()
// {
    // NP_STATUS res = NP_SUCCESS;
    // IRIGB_DEV_S *pDatatimeDev =NULL;
    // INT32 iDataTimeDevices;
    // static BOOL bFirst = TRUE;
    // int i = 0;

    // if(bFirst)
    // {
    //     bFirst = FALSE;
    // }

    // do
    // {
    //     pDatatimeDev = (IRIGB_DEV_S *)DescriptionGetByType(SAC_DEVICE_TYPE_IRIGB, (SAC_DEV_HEADER_ID)pDatatimeDev);
    //     if(pDatatimeDev != NULL)
    //     {
    //         iDataTimeDevices = DeviceRequest((SAC_DEV_HEADER_ID)pDatatimeDev);

    //         if(memcmp(pDatatimeDev->name,BSP_IRIGB_INNER_NAME, sizeof(BSP_IRIGB_INNER_NAME)) == 0)
    //         {
    //             g_iSynInnerDevice = iDataTimeDevices;
    //             logMsg("############## pIrigbDev Name=%s  iIrigbDevices: %d  %d \n",(int)pDatatimeDev->name,iDataTimeDevices,g_iSynInnerDevice,0,0,0);
    //         }
    //     }
    //     else
    //     {
    //         break;
    //     }
    //     i++;
    // }
    // while(1);

    // if(i == 0)
    // {
    //     logMsg("############## IRIGB�豸��ʼ��ʧ��\n",0,0,0,0,0,0);
    //     res = NP_ERROR;
    //     goto exit;
    // }
    // else
    // {
    //     logMsg("############## IRIGB�豸��ʼ���ɹ�\n",0,0,0,0,0,0);
    // }


// exit:
//     return res;
// }

/*******************************************************************************
 �������� :SYN_Init  B��CPU��ʱ����
 ����˵�� :��ʱ��ʼ����ں���
 ����˵�� :��
 ����˵�� :EP_STATUS:���س�ʼ������Ƿ���ȷ
 �޸ļ�¼ :2019/12/11:Dote Create
*******************************************************************************/
EP_STATUS SYN_Init()
{
    EP_STATUS res = EP_ERROR;

    // /* ��ʼ��inner��� */
    // if(HW_SYN_Init() != NP_SUCCESS)
    // {
    //     logMsg("$$$$$$$$$$$$$$$ Dote: SYN_IrigBInnerInit,��ʱ��ʼ��ʧ��\n",0,0,0,0,0,0);
    //     res = EP_ERROR;
    //     goto exit;
    // }

    // /*�����ն˴�CPU��Ҫ��ʼ��out����ȡ�ⲿB���ʱ��Ϣ*/
    // if(uiAppType_g == APP_INTEL_BOX)
    // {
    //     HW_SYN_Init_M();
    // }

    // /*����ʱ�䴦��������,���ȼ���������-4����GOOSE�������ȼ�ǰһ��*/
    // res = taskSpawn("IrigB_Loop", TSK_PRI_MASTER_RELAY_SCAN-4, OS_THREAD_OPT_FP_SUPPORT, 5000, (FUNCPTR)IrigB_Loop,
    //                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    // if (res == ERROR)
    // {
    //     logMsg("IrigB_Loop��ʼ��ʧ�ܡ�\n", 0, 0, 0, 0, 0, 0);
    //     goto exit;
    // }


exit:
    return res;

}

/**
 * @brief ���ö�ֵ��Чͬ���źű�λ��־
 * @param [in] bChgFlag ��ֵ��Чͬ����Ϣ��λ��־
 * @return NONE.
 * @ref  \n
 * �޸�����         �汾��       �޸���         �޸�����     \n
 * ----------------------------------------------------------\n
 * 2020/06/09       V1.0       chaoyang-lin     �����ú���
 * @see
 * @note
 * @warning
 */
void SetSetSynChgFlag(BOOL bChgFlag)
{
    s_SetSynChgFlag = bChgFlag;
}

/**
 * @brief ��ȡ��ֵ��Чͬ���źű�λ��־
 * @param NONE.
 * @return ���ض�ֵ��Чͬ���źű�λ��־
 * @ref  \n
 * �޸�����         �汾��       �޸���	      �޸�����     \n
 * ----------------------------------------------------------\n
 * 2020/06/09        V1.0      chaoyang-lin     �����ú���
 * @see
 * @note
 * @warning
 */
BOOL GetSetSynChgFlag(void)
{
    return s_SetSynChgFlag;
}

/**
 * @brief ���ö�ʱ��ʽ�Ƿ���B���ʱ
 * @param [in] bTimerIsIrigb ��ʱ��ʽ�Ƿ���B���ʱ(TRUE:B�룻FALSE:��B��)
 * @return NONE.
 * @ref  \n
 * �޸�����         �汾��       �޸���         �޸�����     \n
 * ----------------------------------------------------------\n
 * 2020/06/13       V1.0       chaoyang-lin     �����ú���
 * @see
 * @note
 * @warning
 */
void SetTimerIsIrigb(BOOL bTimerIsIrigb)
{
    s_bTimerIsIrigb = bTimerIsIrigb;
}

/**
 * @brief ��ȡ��ʱ��ʽ�Ƿ���B���ʱ
 * @param NONE.
 * @return ��ʱ��ʽ�Ƿ���B���ʱ
 *  @retval TRUE B���ʱ
 *  @retval FALSE ��B���ʱ
 * @ref  \n
 * �޸�����         �汾��       �޸���	      �޸�����     \n
 * ----------------------------------------------------------\n
 * 2020/06/13        V1.0      chaoyang-lin     �����ú���
 * @see
 * @note
 * @warning
 */
BOOL GetTimerIsIrigb(void)
{
    return s_bTimerIsIrigb;
}

/**
 * @brief ��ȡIrigb�豸�����������ֵ
 * @param NONE.
 * @return Irigb�豸��nsʱ��.
 * @ref  \n
 * �޸�����         �汾��       �޸���         �޸�����     \n
 * ----------------------------------------------------------\n
 * 2020/07/15       V1.0       chaoyang-lin     �����ú���
 * @see
 * @note
 * @warning
 */
uint32_t TM_GetIrigbNs(void)
{
    // IRIGB_INFO_S GPSGTime;
    uint32_t ulIrigbNs = 0;

    // IRIGBGet(g_iSynInnerDevice,&GPSGTime);
    // ulIrigbNs = GPSGTime.ns & 0x3FFFFFFF;
    return ulIrigbNs;
}

/* �����ѭ�����ñ�֤CPU GOOSEʱ������
 * Para:
 *     bMMITimeValid, MMI��ʱʱ���Ƿ���Ч.
 * Return:
 *     None.
*/
LW_SYMBOL_EXPORT void TM_KeepTimeQ(BOOL bMMITimeValid)
{
    g_bMMITimeValid = bMMITimeValid;
    GetAbsTimeInterval = tickGet();
    return;
}



BOOL Time_Adjust_F_A_1588_Task(){
    return FALSE;
}

BOOL Check_F_A_1588_Status(uint8_t synMode){
    return FALSE;
}