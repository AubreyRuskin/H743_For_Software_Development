/* measure.h - This file contains interface to measuring */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 27feb07, dy modifying the transfering method to station.
01a, 8nov05, ghx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains interface to measuring.
*/

#ifndef MEASURE_H
#define MEASURE_H

/* includes */

#include "edpbase.h"
#include "logic.h"
#include "swcfg.h"

#ifdef	__cplusplus
extern "C" {
#endif

/* typedefs */

typedef struct
{
    uint8_t aucId[MAX_ID_LEN+1];    /* 测量量逻辑标识 */
    uint8_t aucName[MAX_ID_LEN+1];  		/* 内部名称 */
    uint8_t ucUnit;                 				/* 单位类型 */
    uint8_t ucArith;               /* 取值算法 */
    uint8_t aucABRV[4];         			/* 内部简称 */
    BOOL   bNotNeedUpSend; 					/* 综自上送 */
    float fRtMax;
    float fRtMin;
    float fOvMax;
    float fOvMin;
    float fChgCoff;                 	/* 越限系数值 */
    uint8_t aucRtMaxSettingId[MAX_ID_LEN+1];  	/* 定值设置方式，各个定值逻辑标识字符串基 */
    uint8_t aucRtMinSettingId[MAX_ID_LEN+1];
    uint8_t aucOvMaxSettingId[MAX_ID_LEN+1];
    uint8_t aucOvMinSettingId[MAX_ID_LEN+1];
    uint8_t ucParaSetMode;      		/* 配置参数设置方式 */
    float fPlusCoff; 			/* 增益系数 */
    float fOffCoff;   					/* 偏置系数 */
    uint8_t ucHmSeq;				/* 谐波次数 */
} ME_MEA_VALUE_CFG;

typedef struct
{
    EP_ELEM_IO *pelmSrc;
    void *pvAiHnd;
    uint32_t ulScanTaskNo;
    BOOL bIsRiCplx;    		/* 是否是复数式信号类型 */
    uint8_t ucArith;     				/* 算法类型 */
    COMPLEX xVal;
    float fVal;
    float fLstVal;         		/* 上次值ghx20061026添加，用于判断越限上送 */
    uint8_t ucUnit;      				/* 单位类型，以下三个都是由于上送需求添加 */
    uint16_t uiCode;   		/* 序号 */
    uint8_t ucAttr;     						/* 测量量的属性 */
    uint16_t usQuality; /* 品质 */
    uint16_t usLstQuality; /* 前一个品质状态 */
} ME_MEA_AI_DB;

typedef struct
{
    float fVal;
    uint8_t ucUnit;      				/* 单位类型，以下三个都是由于上送需求添加 */
    uint16_t uiCode;   		/* 序号 */
    uint8_t ucAttr;     						/* 测量量的属性 */
    uint16_t usQuality; /* 品质 */
} ME_MEA_AI_DATA_DB;

/* globals */

extern int iMeaValueNum_g;
extern int iCkSetNum_g;
extern SC_SET_ITEM * pCkset_g;

/* global functions */

EP_STATUS ME_Cfg_Measure_Value(uint8_t *pucCfg, uint32_t ulLen);		/* 解析测量量配置 */
EP_STATUS ME_Initialize(void);

int ME_Get_Msu_Num(void);

/****************************************测量功能的访问函数接口定义********************************/


/*     逻辑图中添加1个新的中间结果测量量到测量量集中

       参数：   strID  , 该测量量的逻辑标识
                pMeasureSignal,  该测量量的数据访问指针,用于测量上送时,访问该测量量
                ulScanTaskNo, 该测量量所在的逻辑图扫描任务号。
                              用于测量时区分不同任务的测量量

       返回值：    返回操作状态

                   EP_SUCCESS,操作成功
                   EP_BAD_DATA,找不到同名逻辑标识的测量量
                   EP_NOT_INIT,找到多于1个的同名逻辑标识的测量量
                   EP_PARA_ERR,因测量量数据指针参数和调试配置模块中的测量量配置信息
                               不一致,导致的错误
                   EP_SYS_ERR,其他原因导致的错误.
*/
EP_STATUS SCI_Init_Add_New_Measure_Signal(uint8_t *strID, EP_ELEM_IO *pMeasureSignal,
        uint32_t ulScanTaskNo);

/*     处理本扫描任务的本次逻辑图扫描周期的测量,该函数在本任务的每次逻辑图扫描的最后进行调用。

       参数：     ulScanTaskNo ,进行本次扫描测量的任务号

      返回值：     无
*/
void SCI_Process_Cur_Logrp_Period_Measure(
    uint32_t ulScanTaskNo,
    uint32_t ulGrpScanDriveInterval,		/* Scanning Interval. */
    uint32_t ulScnAiCnt		/* 进行本次逻辑图扫描时的AI采样计数器值 */
);

/* Read all measurement AIs' value.
 * Parameters:
 *      pfRslt, to save all Measure_AIs' current value.
 * Return value:
 *      None.
 * Alert:
 *      pfRslt must contains space to save iMeaValueNum_g float numbers. */
void ME_Rd_Mea_AI_Val(float *pfRslt);

/***********************************************************************
* RD_Mea_AI - Read all measurment value
*
* RETURNS: None
*
* Alert:
*        pmeaRslt must contains space to save iMeaValueNum_g members.
*/
void RD_Mea_AI(
    RD_AI_MEA *pmeaRslt		/* to save all measurement value */
) ;

/* Get measurement DI attribution.
 * Parameters:
 *      iIdx, index of the Measure value(from 0).
 * Return value:
 *      Pointer to the Measure value attribution structure.
 *      NULL if iIdx is invalid(>=iMeaVauleNum_g). */
const ME_MEA_VALUE_CFG *ME_Get_Msu_Value_Attr(int iIdx);

/*根据测量量的逻辑标示返回测量量序号,如果返回-1，表示没有找着*/
int ME_Get_Msu_Idx(uint8_t *pStrID);

/*根据测量量序号返回测量量的增益系数*/
void ME_Get_Msu_PlusCoff(int iIdx, float *pfRt);

/*根据测量量序号设置测量量的增益系数*/
void ME_Set_Msu_PlusCoff(int iIdx, float fCoff);

/*根据测量量序号返回测量量的增益系数*/
void ME_Get_Msu_OffCoff(int iIdx, float *pfRt);

/*根据测量量序号设置测量量的增益系数*/
void ME_Set_Msu_OffCoff(int iIdx, float fCoff);

/*根据测量量序号设置测量量的越限系数*/
void ME_Set_Msu_ChgCoff(int iIdx, float fCoff);

/*创建测量量增益系数文件*/
BOOL ME_Create_CoffFile(int nType);

/*根据决定内部定植给定的最大值等的控制字定植的修改，修改最大值等*/
void ME_Chg_Msu_Some_Attrs(void);

/*检查测量量的最大值等的定值字符串基是否正确*/
EP_STATUS ME_CK_Mea_Attrs(void);

/*解析测控定值页*/
EP_STATUS SC_Cfg_Ck_Set(uint8_t *pucCfg, uint32_t ulLen);

/* Read CK setting from file to memory.
 * Parameters:
 *      iFd, file descriptor of CK setting file opened previously.
 *      piNum, to save setting number when retrurn.
 * Return value:
 *      Pointer to an array contains CK setting items.
 * Alert:
 *      The return pointer should be freeed by SC_Free_Set_Mem after using. */
SC_SET_ITEM *SC_Rd_CK_Set(int iFd, u_int *piNum,BOOL *bhascrc);

/* Change CK settings.
 * Parameters:
 *      iFd, file descriptor of inner setting file opened previously.
 * Return value:
 *      EP_SUCCESS, making new CK setting work OK.
 *      EP_BAD_DATA, file format error, CK setting not changed. */
EP_STATUS SC_Chg_CK_Set(int iFd);
/* change memory CK set .
 * Parameters:
 *      iFd, file descriptor of inner setting file opened previously.
 * Return value:
 *      EP_SUCCESS, valid.
 *      EP_BAD_DATA, invalid */

EP_STATUS SC_Chg_Mem_CK_Set(int iFd);
/* Check CK is valid.
 * Parameters:
 *      iFd, file descriptor of inner setting file opened previously.
 * Return value:
 *      EP_SUCCESS, valid.
 *      EP_BAD_DATA, invalid */
EP_STATUS SC_CK_Is_Valid(int iFd);

/*Get ck settings pointer*/
SC_SET_ITEM *SC_Get_CK_Set(void);

/*    根据测控定值的定值号，访问定值
      参数：
               nNumInPage，该定值在测控定值表中的定值号
               pRtSettingValue,供返回该测控定值

      返回值：     返回操作状态

                   EP_SUCCESS,操作成功
                   EP_BAD_DATA,因定值号参数不对,导致的错误
                   EP_SYS_ERR,其他原因导致的错误.
*/
EP_STATUS SCI_Get_CK_Setting(int16_t nNumInPage,
                             SCI_SIGNAL_VALUE_TYPE  *pRtSettingValue);

/***********************************************************************
* GetMsuCoff - 获取测量量相关系数，MMI调用
*
* RETURNS: NONE
*
*/
void GetMsuCoff(
    VI_AI_COFF *pMsuCoff		/* 分配iMeaValueNum_g个 */
);

/***********************************************************************
* SCI_Init_Get_Set_Dest - 获取相关定值更新地址
*
* RETURNS: NONE
*
*/
EP_STATUS SCI_Init_Get_Set_Dest(
    char *pDestSetName,		/* 定值名 */
    FLT_U32_UNION **ppDataSrc				/* 定值数据指针 */
);

/* 获取遥测量计算时刻的时标
 * Para:
 *     NONE.
 * Return:
 *     计算时间.
 */
extern uint32_t ME_GetCalcTm(void);

/* 复位写入测控定值
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void SC_Reset_CK_Set(void);

#ifdef	__cplusplus
}
#endif

#endif                        /* MEASURE_H */
