/********************************************************************************/
/*                                                                              */
/*      Copyright (c) 2002 SNAC(Guodian Nanjing Automation Co., Ltd.)           */
/*      All Rights Reserved.                                                    */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/* FILE NAME                                            VERSION                 */
/*                                                                              */
/*      com_api.h                                  EDP01-04-0.1                 */
/*                                                                              */
/* COMPONENT                                                                    */
/*                                                                              */
/*      Data Struct for communication                                           */
/*                                                                              */
/* DESCRIPTION                                                                  */
/*                                                                              */
/*      This file contains data struct for store communication data				*/
/*	                            												*/
/*                                                                              */
/*                                                                              */
/* AUTHOR                                                                       */
/*                                                                              */
/*      HeLong, SNAC                                                            */
/*                                                                              */
/* DATA STRUCTURES                                                              */
/*                                                                              */
/*      None                                                                    */
/*                                                                              */
/* FUNCTIONS                                                                    */
/*                                                                              */
/*      <TODO>                                                                  */
/*                                                                              */
/* DEPENDENCIES                                                                 */
/*                                                                              */
/*      None                                                                    */
/*                                                                              */
/* HISTORY                                                                      */
/*                                                                              */
/*         NAME            DATE                    REMARKS                      */
/*                                                                              */
/*        HeLong      2002.12.03      Created first version 0.1.                */
/*                                                                              */
/********************************************************************************/
#ifndef COM_API_H
#define COM_API_H
#include "edpbase.h"
#include "datetime.h"
typedef struct
{
    uint8_t STA;						/*装置运行模式*/
    uint8_t COND;						/*装置状态*/
} EquipmentRunMode;

typedef struct
{
    uint8_t SN;							/*压板编号*/
    BOOL status;		/*压板实际状态*/
    BOOL actStatus;       /*压板需要改变成的操作状态*/
} YBstatus;

typedef struct
{
    uint8_t status;						/*开入量8位编组状态*/
    uint8_t force;                      /*开入量是否被强制8位编组状态*/
} KRstatus;

typedef struct
{
    uint8_t status;						/*开出量8位编组状态*/
    uint8_t force;                      /*开出量是否被强制8位编组状态*/
} DOstatus;

typedef struct
{
    uint8_t AreaNum;					/*有效定值区号*/
} ValidSetAreaNum;

typedef struct
{
    uint16_t 		SN;				    /*遥测量序号*/
    uint8_t  	    type;			    /*遥测量的类型*/
    uint8_t  	    YCT;			    /*遥测量的属性*/
    FLT_U32_UNION   value;			    /*遥测量的值*/
    BOOL  bUsed ;   /* 此遥测量被使用*/
} RMV;

typedef struct
{
    uint8_t SN;							/*遥信量编号*/
    uint8_t DIQ;						/*遥信量状态*/
} RSV;

typedef struct
{
    uint8_t  Type ;						/*点度/脉冲量的类型*/
    uint32_t Value; 					/*点度/脉冲量的值*/
} Pulse;

typedef struct
{
    uint8_t SN;							/*指示灯编号*/
    uint8_t DIQ;						/*设置状态*/
} PL_SET;

typedef struct
{
    BOOL     RecRunning ; /* 0为录波未启动状态，1为录波启动状态*/
    BOOL   	    RMV_Good;			    /*遥测量可用标志*/
    BOOL   	    RSV_Good;               /*遥信量可用标志*/
//	BOOL        PL_Good;                /*指示灯信息可用标志*/
    BOOL        YB_Good;                /*压板信息可用标志*/
    BOOL        Patrol_Good;            /*巡检数据可用标志*/
    BOOL 	  MEA_Good ; 		/*测量量标志*/
    BOOL 	Energys_Good ;        /*脉冲量输出标志*/

    BOOL        Lock_DO;                /*闭锁开出*/
    BOOL        Lock_EVT;               /*闭锁事件*/
    BOOL        NO_License;             /*注册码不合法*/
    uint8_t       Links_Mode;               /*压板方式*/
    BOOL        EVT_Not_CLR;            /*装置未复归*/
    uint8_t     Working_Set;            /*当前定值区OK*/
    BOOL       bDeviceNameValidFlag ;   /*上送装置设备名称与否hchj for 61850*/
    char         deviceName[128] ;              /*被保护装置设备名称*/
    RSV         RSV_Info[256];          /*遥信量信息区*/
    RMV         RMV_Info[500];          /*遥测量信息区*/
    YBstatus   YB_Info[256];           /*压板状态信息区OK*/
    float 	Mea_Info[256] ;          /*测量量信息区*/
    float 	Energys_Info[256];     /*脉冲量输出信息区(包括电量量)*/

#if  0 /*EDP03 可不使用 2006-9-22 */
    PL_SET      PL_Info[256];           /*面板指示灯信息区*/
#endif
    BOOL 	Exam_State ;	/*检修状态*/

} Common_Data_Info;

typedef struct
{
    uint16_t    Event_Code;             /*事件区分码*/
    BOOL        Event_State;            /*事件状态,TRUE--动作  FALSE--未动作*/
} Event_Info;

typedef struct
{
    FLT_U32_UNION   RmsVal;             /*有效值*/
    FLT_U32_UNION   Angle;              /*相位角*/
    FLT_U32_UNION   Mean;               /*直流偏移*/
    FLT_U32_UNION   Gain;               /*增益系数*/
    uint8_t         ucType;             /*模拟量类型*/
} Hw_Ai_Mea_Info;

typedef struct
{
    char    ProtectName[256];           /*保护名称*/
    BOOL    Status;                     /*保护功能投退状态*/
} ProtectStatus;

typedef struct
{
    char            FileName[64];       /*文件或者子目录的名称*/
    BOOL            Attrib;             /*文件或者子目录的属性,TRUE--文件 FALSE--目录*/
    EP_DATE_TIME    ModifyTime;         /*文件或者子目录的修改时间*/
    uint32_t        FileLenth;          /*该文件的长度（字节数），子目录无意义*/
} FileList_Info;

typedef struct
{
    uint8_t SN;                         /*开入量在开入量集中的序号*/
    uint8_t Status;                     /*开入量状态*/
} DI_Force_Info;

typedef struct
{
    uint16_t        Ntag;               /*报告号*/
    EP_DATE_TIME    RptTime;            /*故障报告产生时间*/
    uint16_t        EventCode;          /*该故障报告引发的第一个事件的区分码*/
    uint8_t         EventType;          /*该事件的类型*/
} Rpt_List_Info;
#endif