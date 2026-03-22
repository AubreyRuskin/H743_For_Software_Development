/*-------------------------------------------*/
/*     本文件的目的在于测试各个函数的功能    */
/*-------------------------------------------*/

#include        "vxWorks.h"
#include	    "semLib.h"
#include        "msgQLib.h"

#include <dirent_compat.h>
#include <sys/stat.h>
#include <ioLib.h>
#include <intLib.h>

// #include "DS1306.H"

// #include	"mo_defs.h"					/*predefine for Motorola CPU*/

// #include    "ha_defs.h"

// #include "sy_defs.h"

// DWORD   SYC_Counter_Tester = 0;     /*1ms counter*/
// DWORD   SYC_Counter10_Tester = 0;   /*10ms counter*/
#define	SY_10ms_HISR_TIME  10
int SYC_HISR_10ms_Tester;

SEM_ID CAC_SEMA_Receive;

#undef TEST_1306
#ifdef TEST_1306
enum
{
    CL_NORMAL=0,
    CL_ERR,
};

/* Define the data type.  */
typedef struct CL_TIME_STRUCT
{
    WORD cl_year;	    /*1997-2050*/
    BYTE cl_month;	/*1-12*/
    BYTE cl_day;	    /*1-28,29,30,31*/
    BYTE cl_hour;	    /*0-23*/
    BYTE cl_minute;	/*0-59*/
    WORD cl_ms;		/*0-59999*/
} CL_TIME;
#endif

/*------1ms int tester----------*/
void SYC_LISR_1ms_Function_Tester()
{
    STATUS status1msInt;

    while(1)
    {
        // status1msInt = HAC_Get_1ms_INT_Status();

        // if(OK != status1msInt)
        // {
        //     continue;
        // }

        // SYC_Counter_Tester++;		/*系统计时器加一*/
        // {
        //     static WORD cnt=SY_10ms_HISR_TIME;
        //     cnt--;
        //     if(cnt==0)
        //     {
        //         cnt=SY_10ms_HISR_TIME;
        //         taskResume(SYC_HISR_10ms_Tester);
        //     }
        // }


#ifdef  TESTER_CAN
        /*CAN petrol*/
        if(CAS_CAN_Check())
        {
            semGive(CAC_SEMA_Receive);
        }
#endif

    }
}

/*Function: Test 1ms interrupt and SYC_Counter*/
void    SYC_1ms_Tester()
{
    int task_1ms_int;
//     static hasCreate = FALSE;

//     if(!hasCreate)
//     {
// #ifdef  TESTER_CAN
//         CAS_Initialize();
//         CAC_SEMA_Receive = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
// #endif


//         HAC_Init_1ms_INT();
//         task_1ms_int = taskSpawn
//                        (
//                            "t1msInt",					/*Task Name*/
//                            49,	    /*Task Priority*/
//                            0,								/*Task Option*/
//                            0x400,		/*Task Stack Size*/
//                            (FUNCPTR)SYC_LISR_1ms_Function_Tester,	/*Task Function*/
//                            0,0,0,0,0,0,0,0,0,0				/*Parameter*/
//                        );
//         hasCreate = TRUE;
//     }
}
/*-------end of 1ms tester------*/

/*-------begin of 10ms tester----*/
/*************************************************************************/
/*                                                                       */
/* FUNCTION: SYC_HISR_10ms_Function	                                     */
/*                                                                       */
/* DESCRIPTION: 系统10ms中断服务程序                                     */
/*                                                                       */
/*************************************************************************/
void SYC_HISR_10ms_Function_Tester()
{
    BOOL run;

    while(1)
    {
        // taskSuspend(0);     /*suspend self*/

        // SYC_Counter10_Tester++;
    }
}

void    SYC_10ms_Tester()
{
    // static int hasCreate = FALSE;
    // if(!hasCreate)
    // {
    //     SYC_HISR_10ms_Tester = taskSpawn
    //                            (
    //                                "t10msInt",					/*Task Name*/
    //                                115,	    /*Task Priority*/
    //                                0,								/*Task Option*/
    //                                0x400,		/*Task Stack Size*/
    //                                (FUNCPTR)SYC_HISR_10ms_Function_Tester,	/*Task Function*/
    //                                0,0,0,0,0,0,0,0,0,0				/*Parameter*/
    //                            );
    //     hasCreate = TRUE;
    // }
}

void    SYC_Begin_10ms_Tester()
{
    SYC_10ms_Tester();
    SYC_1ms_Tester();
    /*HAC_1ms_INT_Enable();*/
}
/*---------end of 10ms tester------------*/

/*------test ds1306----------------------*/
#ifdef TEST_1306
BYTE FUC_BCD_To_Hex(BYTE bcd)
{
    return ((bcd>>1) & 0x78) + ((bcd>>3) & 0x1e) + (bcd & 0x0f);
}

BYTE FUC_Hex_To_BCD(BYTE hex)
{
    return ((hex/10)<<4) + (hex%10);
}

int CLC_Initialize(void)
{
    ds1306Init();
    return CL_NORMAL;
}

int CLC_Read(BYTE ds1306_add,void * data_ptr,BYTE data_number)
{
    ds1306Read(ds1306_add, (BYTE *)data_ptr, data_number);

    return CL_NORMAL;
}

int CLC_Write(BYTE ds1306_add,void * data_ptr,BYTE data_number)
{
    ds1306Write(ds1306_add, (BYTE*)data_ptr, data_number);

    return CL_NORMAL;
}

int CLC_Read_Time(CL_TIME *time_ptr)
{
    BYTE temp[7];
    CL_TIME	temp_time;
    WORD year;
    int old_level;
    int cnt=0;
    while(cnt<3)
    {
        BYTE second;
        CLC_Read(0,temp,7);
        CLC_Read(0,&second,1);
        if(second==temp[0])
            break;							/*没有进位*/
    }

    temp_time.cl_ms		= FUC_BCD_To_Hex(temp[0] & 0x7f)*1000;
    temp_time.cl_minute	= FUC_BCD_To_Hex(temp[1] & 0x7f);
    if(temp[2] & 0x40)						/*12hours制*/
    {
        temp_time.cl_hour	= FUC_BCD_To_Hex(temp[2] & 0x1f);
        if(temp[2] & 0x20)					/*PM*/
            temp_time.cl_hour+=12;
    }
    else									/*24hours制*/
        temp_time.cl_hour	= FUC_BCD_To_Hex(temp[2] & 0x3f);

    temp_time.cl_day	= FUC_BCD_To_Hex(temp[4] & 0x3f);
    temp_time.cl_month	= FUC_BCD_To_Hex(temp[5] & 0x1f);
    year				= (WORD)FUC_BCD_To_Hex(temp[6]);

    if(year>90)
        year+=1900;
    else
        year+=2000;
    temp_time.cl_year=year;

    old_level = intLock();
    *time_ptr=temp_time;						/*real modify system time*/
    intUnlock(old_level);

    if(cnt<3)
        return CL_NORMAL;
    else
        return CL_ERR;
}

int CLC_Write_Time(CL_TIME *time_ptr)
{
    BYTE temp[7];
    int cnt;
    BYTE b;
    WORD w;

    w=time_ptr->cl_ms/1000;
    if(w>59)
        b=59;
    else
        b=(BYTE)w;
    temp[0]	= FUC_Hex_To_BCD(b);

    b=time_ptr->cl_minute;
    if(b>59)
        b=59;
    temp[1]	= FUC_Hex_To_BCD(b);

    b=time_ptr->cl_hour;
    if(b>23)
        b=23;
    temp[2]	= FUC_Hex_To_BCD(b);


    b=time_ptr->cl_day;
    if(b>31)
        b=0;
    if(b==0)
        b=31;
    temp[4]	= FUC_Hex_To_BCD(b);

    b=time_ptr->cl_month;
    if(b>12)
        b=0;
    if(b==0)
        b=12;
    temp[5]	= FUC_Hex_To_BCD(b);

    b=time_ptr->cl_year % 100;
    temp[6]	= FUC_Hex_To_BCD(b);

    cnt=0;
    while(cnt<3)
    {
        BYTE second;
        CLC_Write(0,temp,3);
        CLC_Write(4,&temp[4],3);
        CLC_Read(0,&second,1);
        if(second==temp[0])
            break;							/*没有进位*/
    }
    if(cnt<3)
        return CL_NORMAL;
    else
        return CL_ERR;
}

CL_TIME localTime;

void    Tester_Read_Time()
{
    CLC_Read_Time(&localTime);
}

void    Tester_Write_Time()
{
    CL_TIME fixTime;

    fixTime.cl_day = 21;
    fixTime.cl_hour = 16;
    fixTime.cl_minute = 8;
    fixTime.cl_month = 11;
    fixTime.cl_ms = 1234;
    fixTime.cl_year = 2005;

    CLC_Write_Time(&fixTime);
}

#endif

/*----end of test ds1306-----------------*/
