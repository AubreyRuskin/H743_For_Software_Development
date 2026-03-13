/* swcfg.h - This file contains programs to manager software config file */

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
This file contains programs to manager software config file.
*/

#ifndef SWCFG_H
#define SWCFG_H

/* includes */

#include "edpbase.h"

#ifdef	__cplusplus
extern "C" {
#endif

/* defines */

/*****************************************公共宏定义*****************************************/
#define DESIDE_COEF_VALUE_SETTING_ID "交流模件类型"
#define MAXLINKNUM	256	 /* 最大压板数 */

#define Max_Common_Lenth 5000  /* MMI_MasterCPU中使用的最大报文长度 */

/******************************************数据结构定义******************************/

#define IS_INT32_SET(u)  ((u)==0x60 || (u)==0x61)
#define IS_UINT32_SET(u)  ((u)==0x00 || (u)==0x40 || (u)==0x68|| (u)==0x6B)
#define IS_FLT_SET(u)  ((!IS_INT32_SET(u))&&(!IS_UINT32_SET(u)))

#define SET_AREA_START_NO   0				/* 定值区号从0开始 */
#define MAX_SET_AREA_NUM 32  /* 最大定值区数目 */

#define SET_BACK_FILE_NONE  "NONE"    /* 该定值区没有定值修改前的备份文件 */

/* 为线路保护定制的遥信量 */

#define LINE_REC_DELAY_TIME 15000  /* 滞后录波时长, 单位: us */

#define LINE_CUST_MEA_DI_NUM 21  /* 遥信 */

#define LINE_CUST_MEA_DI1 "其它保护动作"
#define LINE_CUST_MEA_DI2 "其它保护动作-1"
#define LINE_CUST_MEA_DI3 "其它保护动作-2"
#define LINE_CUST_MEA_DI4 "其它保护动作-3"
#define LINE_CUST_MEA_DI5 "其它保护动作-4"
#define LINE_CUST_MEA_DI6 "其它保护动作-5"
#define LINE_CUST_MEA_DI7 "其它保护动作-6"
#define LINE_CUST_MEA_DI8 "远传1"
#define LINE_CUST_MEA_DI9 "远传1-1"
#define LINE_CUST_MEA_DI10 "远传1-2"
#define LINE_CUST_MEA_DI11 "远传1-3"
#define LINE_CUST_MEA_DI12 "远传1-4"
#define LINE_CUST_MEA_DI13 "远传1-5"
#define LINE_CUST_MEA_DI14 "远传1-6"
#define LINE_CUST_MEA_DI15 "远传2"
#define LINE_CUST_MEA_DI16 "远传2-1"
#define LINE_CUST_MEA_DI17 "远传2-2"
#define LINE_CUST_MEA_DI18 "远传2-3"
#define LINE_CUST_MEA_DI19 "远传2-4"
#define LINE_CUST_MEA_DI20 "远传2-5"
#define LINE_CUST_MEA_DI21 "远传2-6"

#ifdef ITEM_LEN_81
#define SW_CUR_LINK_MODE "__CurLinkMode__                                                 " /* 64字节 */
#define SW_CUR_LINK_MODE_NAME "__CurLinkMode__" /* 15字节 */
#define SWLINK_NAME "SWLINK          \n" /* 17字节 */
#define HWLINK_NAME "HWLINK          \n" /* 17字节 */
#define SHWLINK_NAME "SHWLINK         \n" /* 17字节 */
#define SW_CLOSE "CLOSE           " /* 17字节 */
#define SW_OPEN "OPEN            " /* 17字节 */
#else
#define SW_CUR_LINK_MODE "CurMode   " /* 10字节 */
#define SW_CUR_LINK_MODE_NAME "CurMode" /* 7字节 */
#define SWLINK_NAME "SW  \n" /* 5字节 */
#define HWLINK_NAME "HW  \n" /* 5字节 */
#define SHWLINK_NAME "SHW \n" /* 5字节 */
#define SW_CLOSE "1   " /* 5字节 */
#define SW_OPEN "0   " /* 5字节 */
#endif

/* typedefs */

typedef struct
{
    uint8_t aucName[MAX_ID_LEN+1];
    BOOL bIsPub;
    SC_SUB_LGC_ITEM *psublgc;
    int iLinkPos;
    int iLinkNum;
} SC_LINK_PAGE;

typedef struct
{
    int iLinkIdx;
    BOOL bSts;
} SC_LINK_STS;  			/* 压板状态信息 */

typedef struct			/** 信号值结构定义 **/
{
    union
    {
        COMPLEX  xVal;
        float fVal;
        uint32_t ulVal;
        int32_t lVal;
        BOOL bVal;
    } Value;                            		/* 信号数据值 */
    uint8_t ucAttrib;                   /* 信号单位类型属性 */

    void  * pvSrc;  /* 定值的数据源指针值， 若为字符串定值，用于返回
                       字符串定值的当前值的基址;否则，用于返回定值的句柄*/
} SCI_SIGNAL_VALUE_TYPE;

typedef struct					/***   定值信息结构定义***/
{
    BOOL bIsInnerField;        		/* 该定值是否是内部定值 */
    uint8_t  ucType;                 						/* 该定值分类0一般定值 1内部定值2 测控定值 */
    uint8_t *strRelayFuncName;          	/* 该定值所属的保护功能名, 对一般定值和内部定值
                                           						 有效. 若该字符串内容为空, 则表示该
                                           						 定值属于一般定值域的公共定值区 */
    int8_t cPageNum;                 /* 所属的定值页号（只对一般定值有效） */
    int16_t nNumInPage;                 		/* 该定值在定值页中的定值号
                                                                对内部定值而言，即内部定值号 */
    uint8_t ucAttrib;           /* 该定值的单位类型属性 */
} SCI_SETTING_INFO_TYPE;

typedef struct				/* 索引定值字符串基的信息结构定义 */
{
    uint8_t aucName[MAX_ID_LEN+1];
    BOOL bUsed;
} SC_SY_SETBASE_ITEM;

typedef struct tag_LOGICYBTT				/* 逻辑图中压板投退结构 */
{
    BOOL bYaBanTTFlag;			/* 是否有压板投退 */
    BOOL YBState[MAXLINKNUM];				/* 与压板号有关的退出状态，为TRUE则需要退出 */
} LOGICYBTT, *pLOGICYBTT;

typedef struct			/* 模拟量系数 */
{
    float fRtMax;
    float fRtMin;
    float fOvMax;
    float fOvMin;
    float fChgCoff;
} VI_AI_COFF;

/* globals */

extern int iSetPgNum_g;                 /* 配置的定值总页数（0页固定为内部定值） */
/* extern uint8_t aucLkTpDiSrc_g[]; */       /* 决定软/硬压板方式的开入ID */
/* extern void *pvLinkTpDI_g; */            /* 决定软/硬压板方式的开入句柄 */
extern int iLkPgNum_g;                  /* 配置的压板总页数 */
extern int iLinkNum_g;                  /* 配置的压板总个数 */
extern int iSubLgcNum_g;                /* 逻辑分图个数 */
extern SC_SUB_LGC_ITEM *psublgc_g;
extern uint8_t aucEqName_g[MAX_ID_LEN+1];			/* 逻辑总图名称（装置名称） */
extern uint16_t unLgcVer_g;

extern SC_SET_ITEM *pCoefTailSet_g;   	/* 决定比例系数、量程等的定值字符串尾的控制字定值指针 */
extern BOOL bCoefTailChg;  							/* 0没有配置 1 控制字定值在公共定值 2 在内部定值 3 在测控定值 */

/* 为线路保护定制的遥信量名称数组 */
extern char ucArrMeaDiName[LINE_CUST_MEA_DI_NUM][TEMP_INFO_MAX_LEN];

/* global functions */

/* 初始化软件配置模块
 * 参数：	strSwCfgFile，软件配置文件名称
 * 返回值：	EP_SUCCESS，正常返回
 *          EP_CFG_ERR，配置文件错误
 *			EP_BUF_ERR，内存错误 */
EP_STATUS SC_Initialize(const uint8_t *strSwCfgFile);

/****************************保护功能分图投退设置状态的访问函数接口定义**********/

/*      访问调试配置模块管理的某保护功能分图的投退设置状态

        参数：           strID  , 该保护功能分图的分图名
                         pbRtSetStatus,   供返回该保护功能分图设置的保护投退状态
                          TRUE为设置投入，FALSE为设置退出
        返回值：          返回操作状态
                          EP_SUCCESS,操作成功
                          EP_BAD_DATA,找不到同名逻辑标识的保护功能分图
                          EP_NOT_INIT,找到多于1个的同名逻辑标识的保护功能分图
                          EP_SYS_ERR,其他原因导致的错误.
*/
EP_STATUS SCI_Init_Get_RelayFunc_RunExit_Set_Status(uint8_t  *strID,
        BOOL   *pbRtSetStatus );

/****************************定值(包括内部定值和一般定值)访问函数接口定义********/

/*     根据定值的逻辑标识，获得定值的相关信息
       参数：   strID  , 定值逻辑标识字符串
               pRtSettingInfo  , 供返回该定值的相关信息

       返回值：    返回操作状态

                   EP_SUCCESS,操作成功
                   EP_BAD_DATA,找不到同名逻辑标识的保护定值
                   EP_NOT_INIT,找到多于1个的同名逻辑标识的保护定值
                   EP_SYS_ERR,其他原因导致的错误.
*/
EP_STATUS SCI_Init_Get_Setting_Info(uint8_t *strID,
                                    SCI_SETTING_INFO_TYPE  *pRtSettingInfo);

/*    根据内部定值的定值号，访问定值
      参数：
               nNumInPage，该定值在内部定值表中的定值号
               pRtSettingValue,供返回该内部定值

      返回值：     返回操作状态

                   EP_SUCCESS,操作成功
                   EP_BAD_DATA,因定值号参数不对,导致的错误
                   EP_SYS_ERR,其他原因导致的错误.
*/
EP_STATUS SCI_Get_Inner_Setting(int16_t nNumInPage,
                                SCI_SIGNAL_VALUE_TYPE  *pRtSettingValue);
/*    根据内部定值的字符串基，访问定值，规约3.60增加
      参数：
               strBaseID，该定值的逻辑标识字符串基，
               需要再根据决定字符串尾的一般定值控制字决定最终的字符串
               pRtSettingValue,供返回该内部定值

      返回值：     返回操作状态

                   EP_SUCCESS,操作成功
                   EP_BAD_DATA,因定值号参数不对,导致的错误
*/
EP_STATUS SCI_Get_Inner_Setting_BySettingStrBase(uint8_t *strBaseID,
        SCI_SIGNAL_VALUE_TYPE  *pRtSettingValue);

/* 获取通道增益系数索引定值 */
extern EP_STATUS SCI_Get_Coff_Inner_Setting_BySettingStrBase(
    uint8_t *strBaseID, /* 该定值的逻辑标识字符串基，需要再根据决定字符串尾的一般定值控制字决定最终的字符串 */
    SCI_SIGNAL_VALUE_TYPE  *pRtSettingValue, /* 供返回该内部定值 */
    int32_t iIndexSn  /* 索引定值页序, 初始为-1 */
);

/*    根据一般定值的定值页和定值号信息，访问定值
      参数：
                cPageNum,   该定值所在的定值页号
                nNumInPage，该定值在定值页中的定值号
               pRtSettingValue,供返回该一般定值

      返回值：     返回操作状态
                   EP_SUCCESS,操作成功
                   EP_BAD_DATA,因定值页或定值号参数不对,导致的错误
                   EP_SYS_ERR,其他原因导致的错误.
*/
EP_STATUS SCI_Get_General_Setting(int8_t cPageNum, int16_t nNumInPage,
                                  SCI_SIGNAL_VALUE_TYPE  *pRtSettingValue);

/***************************压板(包括硬压板和软压板)访问函数接口定义*******************************/

/*     根据压板的逻辑标识，获得压板的相关信息
       参数：   strID  , 压板逻辑标识字符串
                pnRtNum，供返回该压板在压板集中的压板号
       返回值：    返回操作状态

                   EP_SUCCESS,操作成功
                   EP_BAD_DATA,找不到同名逻辑标识的保护压板
                   EP_NOT_INIT,找到多于1个的同名逻辑标识的保护压板
                   EP_SYS_ERR,其他原因导致的错误.
*/
EP_STATUS SCI_Init_Get_Yaban_Info(uint8_t *strID, int16_t *pnRtNum);

/* 设置压板有流判断标识.
 * Para:
 *     sNum, 压板序号.
 *     smvDataChn, 通道内序, 从0开始.
 * Return: EP_SUCCESS, or EP_ERROR.
 */
extern EP_STATUS SCI_Set_Yaban_JgCurInfo(int16_t sNum, int smvDataChn);

/*     根据压板的压板号，访问压板当前值
       参数：
                nNum，该压板在压板表中的压板号
                pbRtYabanValue,供返回该压板当前值，真为压板投入，假为压板退出

                ulScnTime, 进行本次逻辑图扫描时的时刻（us计数器值）
       返回值：    返回操作状态

                   EP_SUCCESS,操作成功
                   EP_BAD_DATA,因压板号参数不对,导致的错误
                   EP_SYS_ERR,其他原因导致的错误.
*/
EP_STATUS SCI_Get_Yaban_Value(int16_t nNum, BOOL *pbRtYabanValue, uint32_t ulScnTime);

/* Get link status.
 * Parameters:
 *      iIdx, link index(from 0).
 *      pbRslt, to save return result.
 * Return value:
 *      EP_SUCCESS, read status OK.
 *      EP_BAD_DATA, invalid index(>=iLinkNum_g). */
EP_STATUS SC_Get_Link_Sts(int iIdx, BOOL *pbRslt);

/* Get setting page attribution.
 * Parameters:
 *      iIdx, setting page(0 means internal setting).
 * Return value:
 *      Pointer to the setting page attribution structure.
 *      NULL if iIdx is invalid(>=iSetPgNum_g). */
const SC_SET_PAGE *SC_Get_Set_Pg_Attr(int iIdx);

/* Get link page attribution.
 * Parameters:
 *      iIdx, link page number.
 * Return value:
 *      Pointer to the link page attribution structure.
 *      NULL if iIdx is invalid(>=iSetPgNum_g). */
const SC_LINK_PAGE *SC_Get_Link_Pg_Attr(int iIdx);

/* Get link attribution.
 * Parameters:
 *      iIdx, index of the link(from 0).
 * Return value:
 *      Pointer to the link attribution structure.
 *      NULL if iIdx is invalid(>=iLinkNum_g). */
const SC_LINK_ITEM *SC_Get_Link_Attr(int iIdx);

/* Initialize protect function config.
 * Parameters:
 *      strLgcFile, logic config file name.
 * Return value:
 *      EP_SUCCESS, parse config file OK.
 *      EP_CFG_ERR, config file format error.
 *	    EP_BUF_ERR, can't malloc enough memory. */
EP_STATUS SC_Init_Func_Cfg(const uint8_t *strLgcFile);

/* Get sub-logic attribution.
 * Parameters:
 *      iIdx, index of the sub-logic(from 0).
 * Return value:
 *      Pointer to the sub-logic attribution structure.
 *      NULL if iIdx is invalid(>=iSubLgcNum_g). */
const SC_SUB_LGC_ITEM *SC_Get_Sub_Lgc_Attr(int iIdx);

/* Check protect setting after power on.
 * Parameters:
 *      None.
 * Return value:
 *      EP_SUCCESS, everything is OK.
 *      EP_FILE_ERR, some system file error.
 *      EP_CFG_ERR, setting file not valid. */
EP_STATUS SC_Chk_Set(void);

/* Check if setting area file is valid.
 * Parameters:
 *      iFd, file descriptor opened previously.
 * Return value:
 *      TRUE, the setting area file is valid.
 *      FALSE, the setting area file is NOT valid.
 * Alert:
 *      Current position of the file is changed in this function. */
BOOL SC_Is_Valid_Set(int iFd);

/* Finish writing a setting area.
 * Parameters:
 *      iArea, setting area number.
 *      Back_Filename, setting backup file.
 * Return value:
 *      EP_SUCCESS, or EP_ERROR.
 * Alert:
 *      The caller should promise the setting area file is valid.
 *      This is the last step of updating setting area file. */
EP_STATUS SC_End_Wr_Set(int iArea, uint8_t *Back_Filename);

/*check inner valid or not(file has crc in end)
 * Parameters:
 *      iFd, file descriptor of inner setting file opened previously.
 * Return value:
 *      Pointer to an array contains inner setting items.
*/
BOOL SC_Check_CRC(int fp);
/* Read inner setting from file to memory.
 * Parameters:
 *      iFd, file descriptor of inner setting file opened previously.
 *      piNum, to save setting number when retrurn.
 * Return value:
 *      Pointer to an array contains inner setting items.
 * Alert:
 *      The return pointer should be freeed by SC_Free_Set_Mem after using. */
SC_SET_ITEM *SC_Rd_Inner_Set(int iFd, u_int *piNum,BOOL *bhasCRC);

/* Free dynamic memory allocted for setting data structure.
 * Parameters:
 *      pset, pointer to setting array dynamic allocated before.
 *      iNum, item number in the setting array.
 * Return value:
 *      None. */
void SC_Free_Set_Mem(SC_SET_ITEM *pset, int iNum);

/* Change inner settings.
 * Parameters:
 *      iFd, file descriptor of inner setting file opened previously.
 * Return value:
 *      EP_SUCCESS, making new inner setting work OK.
 *      EP_BAD_DATA, file format error, inner setting not changed. */
EP_STATUS SC_Chg_Inner_Set(int iFd);

/* Change protect function run/exit status.
 * Parameters:
 *      strName, protect(sub-logic) name.
 *      bSts, new status.
 * Return value:
 *      EP_SUCCESS, change run/exit status OK.
 *      EP_FILE_ERR, file operating failure. */
EP_STATUS SC_Chg_Prtc_Sts(const uint8_t *strName, BOOL bSts);

/* Change software link status.
 * Parameters:
 *      iIdx, index of link(from 0).
 *      bSts, new status.
 * Return value:
 *      EP_SUCCESS, change link status OK.
 *      EP_FILE_ERR, file operating failure. */
EP_STATUS SC_Chg_Sw_Link(int iIdx, BOOL bSts);


EP_STATUS SC_Chg_Sw_Multi_Link(uint8_t *pRcvLinkBuf);

SC_LINK_ITEM * SC_Get_Sw_Link(int iIdx);

/* 获取固化压板项.
 * Para:
 *     iIdx, 固化压板序号.
 * Return:
 *     压板句柄.
 */
extern SC_LINK_ITEM *SC_Get_Wr_Sw_Link(int iIdx);

/* Change working setting area.
 * Parameters:
 *      iArea, new working setting area number.
 * Return value:
 *      EP_SUCCESS, change work setting area OK.
 *      EP_FILE_ERR, file operating failure. */
EP_STATUS SC_Chg_Work_Area(int iArea);

/* Delete a setting area.
 * Parameters:
 *      iArea, number of setting area.
 * Return value:
 *      EP_SUCCESS, delete OK.
 *      EP_PARM_ERR, iArea is the working area, can't delete.
 *      EP_FILE_ERR, the area file not exists. */
EP_STATUS SC_Del_Set_Area(int iArea);

/* Get total valid setting area number.
 * Parameters:
 *      None.
 * Return value:
 *      Number of total valid setting area. */
int SC_Valid_Area_Num(void);

/* Get every valid setting area number.
 * Parameters:
 *      pucRslt, to save setting area number result.
 * Return value:
 *      Number of total valid setting area. */
int SC_Get_Valid_Area(uint8_t *pucRslt);

/* Check if the number of setting area is valid.
 * Parameters:
 *      iArea, number of setting area.
 * Return value:
 *      TRUE, the setting area is valid.
 *      FALSE, the setting area is NOT valid. */
BOOL SC_Is_Valid_Area(int iArea);

/* Get working setting area number.
 * Parameters:
 *      None.
 * Return value:
 *      Working setting area number.
 * Alert:
 *      Working setting area may be not a valid setting area.  This error is
 *      reported by other way. */
int SC_Work_Set_Area(void);

/***********************************************************************
* SC_Real_Work_Set_Area - Get real working setting area number.
*
* RETURNS: Real working setting area number.
*
* Alert:
*        Real working setting area may be not a valid setting area.  This error is reported by other way.
*
*/
int SC_Real_Work_Set_Area(void);

/*设置即将切换的定值区，逻辑图中调用*/
void  SC_Set_Next_Work_Area(int iArea);
/*即将切换的定值区，mmi调用*/
int SC_Next_Work_Set_Area(void);

/*每次更改一般定值后调用，如果决定现场需求的控制字定值修改了，
那么更新对应的各项AI、AO、遥测、测量的各个系数*/
BOOL SC_Updt_Each_GivenSetting_Decided_Item(void);

/*
在索引定值中匹配每个使用的字符串基, 如果没有配置,返回EP_CFG_ERR,否则返回EP_SUCCESS
pRtFlagRepeated 用于返回是否使用重复。目前可以使用重复
*/
EP_STATUS SC_Find_Setbase(uint8_t * strBaseID, BOOL *pRtFlagRepeated);

/*
设置压板投退标志，逻辑图中设置
*/
void EP_Set_YBTT_Flag(int iYbNum);

/*
恢复压板投退标志，慢速任务中执行压板投退操作后恢复*/
void EP_Clr_YBTT_Flag(int iYbNum);

/*得到压板投退标志,慢速任务根据该标志来进行压板投退操作*/
LOGICYBTT *EP_Get_YBTT_Flag();

/***********************************************************************
* SCI_Update_Yaban_Value_Auto - Update the yaban state automatically.
*
* RETURNS: EP_SUCCESS, or EP_ERROR.
*
*/
EP_STATUS SCI_Update_Yaban_Value_Auto(
    int iTaskNo,	/* The priority of the scanning task. */
    uint32_t ulGrpScanDriveInterval,		/* Scanning Interval. */
    uint32_t ulScnAiCnt			/* 进行本次逻辑图扫描时的AI采样计数器值 */
);

/***********************************************************************
* GetMmiNeedUpdateFlag - 获取是否需要更新相关配置标志
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL GetMmiNeedUpdateFlag(void);

/***********************************************************************
* ClearMmiNeedUpdateFlag - 清除是否需要更新相关配置标志
*
* RETURNS: NONE
*
*/
void ClearMmiNeedUpdateFlag(void);

/***********************************************************************
* SC_Set_Work_Area_LogicScan - 逻辑图扫描中切换定值区
*
* RETURNS: 无
*
*/
void SC_Set_Work_Area_LogicScan(
    int nSettingArea		/* 待切换定值区 */
);

/* Get link mode status.
 * Para
 *      ulTotalLinkMode: the value of the ulTotalLinkMode_g
 * Return value:
 *      link mode global var ulTotalLinkMode_g;
*/
EP_STATUS SC_Get_Link_Mode_Sts(
    uint16_t *ulTotalLinkMode		/* 总模式 */
);

/* Get link now status.
 * Parameters:
 *      iIdx, link index(from 0).
 *      pbRslt, to save return result.
 * Return value:
 *      EP_SUCCESS, read status OK.
 *      EP_BAD_DATA, invalid index(>=iLinkNum_g). */
EP_STATUS SC_Get_Link_Now_Sts(
    int iIdx,		/* 压板号 */
    BOOL *pbRslt			/* 输出值 */
);

/* DQ:
 * Get soft link status.
 * Parameters:
 *      iIdx, link index(from 0).
 *      pbRslt, to save return result.
 * Return value:
 *      EP_SUCCESS, read status OK.
 *      EP_BAD_DATA, invalid index(>=iLinkNum_g). */
EP_STATUS SC_Get_Link_SW_Sts(
    int iIdx,
    BOOL *pbRslt
);

/*
Description: change the content of the /set/set/edplinkmode.set
iIdx: represent the certain link idx, only valid when bTotalFlag==false
ulMode: the link's mode to be set.
bTotalFlag:  if false, ucMode represent certain link's mode
             if true, ucMode represent total link's mode
*/
EP_STATUS SC_Chg_Link_Mode_File(int iIdx, uint8_t ucMode, BOOL bTotalFlag);

/* DQ:
 * Get hard link status.
 * Parameters:
 *      iIdx, link index(from 0).
 *      pbRslt, to save return result.
 * Return value:
 *      EP_SUCCESS, read status OK.
 *      EP_BAD_DATA, invalid index(>=iLinkNum_g)
 *      EP_PARM_ERR, link is not relevent. */
EP_STATUS SC_Get_Link_HW_Sts(
    int iIdx,		/* 压板号 */
    BOOL *pbRslt			/* 输出值 */
);

/*check  inner settings is valid.
 * Parameters:
 *      iFd, file descriptor of inner setting file opened previously.
 * Return value:
 *      EP_SUCCESS, valid.
 *      EP_BAD_DATA, invalid. */
EP_STATUS SC_Inner_Set_Is_Valid(int iFd);

/* Change memory inner settings. used in file download
 * Parameters:
 *      iFd, file descriptor of inner setting file opened previously.
 * Return value:
 *      EP_SUCCESS, making new inner setting work OK.
 *      EP_BAD_DATA, file format error, inner setting not changed. */
EP_STATUS SC_Chg_Mem_Inner_Set(int iFd);

/* 判断是否有流
 * Para:
 *     sNum, 压板序号.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL SCI_Exsit_Current(int16_t sNum);

/* 获取国网三压板状态.
 * Para:
 *     pbRmtSwitchYB, 远方投退压板软压板.
 *     pbRmtSwitchSetting, 远方投退压板软压板.
 * Return:
 *     EP_SUCEESS, or EP_ERROR/EP_FILE_ERR.
 */
extern EP_STATUS FT_Get_GWYB_Sts(BOOL *pbRmtSwitchYB, BOOL *pbRmtSwitchSetting);

/* 复位写入内部定值
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void SC_Reset_Inner_Set(void);

/* 复位写入保护定值
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void SC_Reset_Set(void);

/* 复位压板
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void SC_Reset_Link(void);

/***********************************************************************
* Bak_File_Name - 获取临时文件名
*
* RETURNS: 无
*
*/
extern void Bak_File_Name(
    char *temp_filename,
    char *strFile,
    char *insChar
);

/* 初始化读取定值量程文件
 * Para:
 *     void
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
extern EP_STATUS SC_RD_SetRange(void);

/* 校验定值量程文件CRC */
extern BOOL SC_SetRange_Check_CRC(int iFd);

/* 生成新的定值量程文件
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void SC_New_SET_Range(void);

/* 设置定值量程文件生成结束标识
* 参数:
* bRFOverFlg: 要设置的标识(TRUE or FALSE)
* 返回值:
* 无
*/
extern void SC_Set_RangeFile_OverFlg(BOOL bRFOverFlg);

/* 获取定值量程文件生成结束标识
* 参数:
* 无
* 返回值:
* 结束标识的值.
*/
extern BOOL SC_Get_RangeFile_OverFlg(void);

/* 获取定值量程
 * Para:
 *     pPara, result.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
extern EP_STATUS SC_Get_SetRange(uint8_t **pPara);

/* 设置定值量程量调整计数自增1
* 参数:
* 无
* 返回值:
* 无
*/
extern void SC_Set_ChgCnt_Plus(void);

/* 获取量程调整计数
* 参数:
* 无
* 返回值:
* 量程调整计数值.
*/
extern uint8_t SC_Get_Range_ChgCnt(void);

/* 更新定值量程后检查定值的有效性
 * Parameters:
 *      None.
 * Return value:
 *      None.
 */
extern void SC_Chk_Range_All_Valid(void);

/* 更新定值量程后检查保护定值区的有效性
 * Parameters:
 *      None.
 * Return value:
 *      EP_SUCCESS, everything is OK.
 *      EP_FILE_ERR, some system file error.
 *      EP_CFG_ERR, setting file not valid.
 */
extern EP_STATUS SC_Chk_Range_Set_Valid(void);

/* 设置保护定值区正在校验标识
* 参数:
* bChkFlg: 要设置的标识(TRUE or FALSE)
* 返回值:
* 无
*/
extern void SC_Set_AreaSet_ChkFlg(BOOL bChkFlg);

/* 获取保护定值区正在校验标识
* 参数:
* 无
* 返回值:
* 标识的值(TRUE or FALSE).
*/
extern BOOL SC_Get_AreaSet_ChkFlg(void);

#ifdef	__cplusplus
}
#endif

#endif                                  /* SWCFG_H */
