/* VTBOX_Interface.h - subroutine library for virtual box interface */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 07nov08, dy modify macros and realize the functions.
01a, 20oct08, zy first created
*/

/*
DESCRIPTION
This module includes subroutine library for virtual box interface.
*/

#ifndef VTBOX_INTERFACE_H
#define VTBOX_INTERFACE_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "vxWorks.h"			/* always first */
#include "realdatadef.h"
#include "logic.h"

/* defines */

#define MAX_VTBOX_DEV_ID_LEN 129

#define VTBOX_MAX_DI_BUF_LEN ((MAX_MOD_NUM*MAX_DI_PER_MOD*4+7)/8)     /* size of DI buffer, unit Byte. */
#define VTBOX_MAX_DO_BUF_LEN ((MAX_MOD_NUM*MAX_DO_PER_MOD*4+7)/8)     /* size of DO buffer, unit Byte. */

#define MAXPERIODFUNCNUM 6

#define VIRTBOXFRAMELENGTH 200

typedef enum		/* type of period scanning for virtual box. */
{
    VTBOX_FUNC_BY_SAMP_HEAD_INTVL = 0,  /* scan in interrupt function, in the front of the main body. */
    VTBOX_FUNC_BY_SAMP_TAIL_INTVL = 1,       /* scan in interrupt function, in the end of the main body. */
    VTBOX_FUNC_BY_PRE_PRC_HEAD_INTVL = 2,       /* scan in DSP task function, in the front of the main body. */
    VTBOX_FUNC_BY_PRE_PRC_TAIL_INTVL = 3,    /* scan in DSP task, in the end of the main body.*/
    VTBOX_FUNC_BY_FST_LGRP_HEAD_INTVL = 4,     /* scan in the fast logic scanning task, in the front of the main body. */
    VTBOX_FUNC_BY_FST_LGRP_TAIL_INTVL = 5, /* scan in the fast logic scanning task, in the end of the main body. */
} VTBOX_PERIOD_FUNC_INTVL_TYPE;

#define MAX_VIRT_BOX_ALLOW_AIO_NUM 30    /* the max AIO number. */
#define MAX_VIRT_BOX_ALLOW_DIO_NUM 30    /* the max DIO number. */

#define MAX_VIRT_BOX_DI_MOD_NUM 3      /* The max DI module number. */
#define MAX_VIRT_DI_MOD_BASE_ADDR 0      /* base address of DI module. */

#define MAX_VIRT_BOX_DO_MOD_NUM 3      /* The max DO module number. */
#define VIRT_BOX_DO_MOD_BASE_ADDR 3      		/* base address of DO module. */

/* typedefs */

/* initialization of virtual box. */
typedef EP_STATUS (* P_INIT_DRV_FUN)(void);

/* handle of function to show status on MMI. */
typedef BOOL (* P_SHOW_STS_FUNC)(
    uint8_t ucAddr, 		/* Address of this type of virtual box. */
    uint8_t **pucVtBoxShowStsStr,
    int32_t *iMaxStsStrLen
);

/* handle of function to clear status on MMI. */
typedef BOOL (* P_CLR_STS_FUNC)(
    uint8_t ucAddr		/* Address of this type of virtual box. */
);

/* period scan. */
typedef int (* P_PERIOD_USER_FUNC)(void);

typedef struct
{
    P_PERIOD_USER_FUNC pPeriodUserFunc;
    VTBOX_PERIOD_FUNC_INTVL_TYPE iPeriodIntvlType;
    int iPeriodIntvl;
} VIRT_BOX_PERIOD_FUNC;

typedef struct		/* initialization driver struct of device. */
{
    uint8_t ucDevDescPrefix[MAX_ID_LEN+1];		/* device describle. */
    P_INIT_DRV_FUN p_Init_Drv_Fun;		/* initialization driver. */
    P_SHOW_STS_FUNC p_Show_Sts_Fun;
    P_CLR_STS_FUNC p_Clr_Sts_Fun;
    VIRT_BOX_PERIOD_FUNC PeriodScan[MAXPERIODFUNCNUM];
    BOOL bInuse;		/* If used. */
} VIRT_BOX_DRIVER;

typedef struct /* configuration information of virtual box, as the interface between virtual box module and platform. */
{
    uint8_t aucVtBoxDevID[MAX_VTBOX_DEV_ID_LEN];		/* virtual box device descriptor in hardware configuration,
                                                           same virtual box have same device descriptor. */
    int iVtBoxSeqNo;		/* serial number, begin from 1 in configuration, begin from 0 in programm, the number is particular. */
    uint8_t iVtBoxAddrInSameKind;		/* address. */

    int iAiNum;      /* AI number, the first is origin value, then the preprocessing, the third is the value from logic graph.
	                    organize the information on sending terminal, must realize the configuration information between the master and the slave. */
    int iOriginLogicNum;		/* origin value. */
    int iPreProNum;		/* preprocessing number. */

    /* 该虚拟机箱配置的所有逻辑AI依次关联的AI物理通道内序数组(按XXPZ的次序), 数组有效维数为iAiNum
     * 注意，物理内序编号空间从0开始，
     * 每个逻辑AI通道有且仅关联一个物理AI通道.
     */

    uint8_t ucAiHwChArr[MAX_VTBOX_AI_NUM];

    /* 该虚拟机箱配置的所有逻辑AI依次关联的AI物理通道的物理比例系数数组
     * (是指XXPZ中配置的系数，按XXPZ的次序), 数组有效维数为iAiNum
     */

    float fAiHwCoffArr[MAX_VTBOX_AI_NUM];

    uint8_t LogicChnNumber_8;
    uint8_t LogicChnNumber_m;
    uint8_t DataNumofPreProcess_8;
    uint8_t DataNumofPreProcess_m;

    /* DI configuration. */
    int iDiNum;       /* number of DI. */
    uint8_t aucDiSts[VTBOX_MAX_DI_BUF_LEN];      /* DI buffer. */

    int iAoNum;    /* AO number, the first is origin value, then the preprocessing ,the third is value from logic graph. */

    /* AO from AI. */
    int iOrgAISrcAoNum;                    /* number of AO from AI. */
    uint8_t *pucOrgAISrcAoDataByteNextBase;       /* base address of AO from origin AI. */
    uint8_t ucOrgAISrcAoDataSN[MAX_VTBOX_AO_NUM];
    float afOrgAISrcAoCoff[MAX_VTBOX_AO_NUM];  /* coefficient of AO from AI. */

    /* AO from preprocessing. */
    int iPreAISrcAoNum;      /* AO from preprocessing AI. */
    uint8_t *pucPreAISrcAoDataByteNextBase;       /* base address of AO from preprocessing AI. */
    uint8_t pucPreAISrcAoDataSN[MAX_VTBOX_AO_NUM];
    float afPreAISrcAoCoff[MAX_VTBOX_AO_NUM];  			/* coefficient of AO from preprocessing AI. */

    /* AO from logic graph varibles. */
    int iMidSrcAoNum;     /* AO number from logic graph variables. */
    uint8_t pucMidSrcAoDataSN[MAX_VTBOX_AO_NUM];
    EP_ELEM_IO *apMidSrcAOPt[MAX_VTBOX_AO_NUM];   /* array of pointer to logic graph middle variables. */

    /* DO configuration. */
    int iDoNum;     /* number of DO. */
    uint8_t aucDoSts[VTBOX_MAX_DO_BUF_LEN];   /* DO buffer. */

    RD_AI_MOD *pAiMod;
    VIRT_BOX_DRIVER *pBoxDrv;
} VTBOX_INFO;

typedef struct		/* data frame. */
{
    uint8_t ucRevData[VIRTBOXFRAMELENGTH];
    uint8_t ucSndData[VIRTBOXFRAMELENGTH];
    uint16_t usDataLen;
    uint32_t ulTimeus;
    uint8_t ucSts;
    int16_t usSamCount;
    float fAi[MAX_VTBOX_AI_NUM];
    float fCoff[MAX_VTBOX_AI_NUM];
    int16_t usAiNum;
} VIRTBOXFRAME;

/* globals */

extern VIRT_BOX_DRIVER VirtBoxInitDriver[MAX_VT_BOX_COUNT];
extern uint32_t ulDriverNums;		/* number of drivers. */
extern VTBOX_INFO apVtBoxCfgArr_g[MAX_VT_BOX_COUNT]; /* virtual box configuration. */
extern int iVtBoxCfgNum_g;        /* number of virtual box. */
extern u_int uiAiPts_g;       /* AI sampling point number in a cycle. */

/* functions */

/* register initialization driver.
 * Para:
 *     pDriverFd, devices descriptor.
 *     pDriver, driver function.
 *     p_Show_Sts_Fun, Show status of virtual box on MMI.
 *     p_Clr_Sts_Fun, clear status.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VTBOX_Driver_Reg(uint8_t *pDriverFd, P_INIT_DRV_FUN pDriver, P_SHOW_STS_FUNC p_Show_Sts_Fun, P_CLR_STS_FUNC p_Clr_Sts_Fun);

/* register periodly called function for driver.
 * Para:
 *     pucVtBoxDevID, device describe.
 *     pfUser, function called periodly.
 *     iPeriodIntvlType, calling type.
 *     iPeriodIntvl, calling interval.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VTBOX_Reg_Period_Func(uint8_t *pucVtBoxDevID, P_PERIOD_USER_FUNC pfUser, VTBOX_PERIOD_FUNC_INTVL_TYPE iPeriodIntvlType, int iPeriodIntvl);

/* Get the synchronizated local AI counter for specialized virtual box.
 * Para:
 *     iPos, sequence number of virtual box.
 * Return:
 *     AI counter.
 */
uint32_t VTBOX_AI_Cnt(int iPos);

/* Get the pointer to realtime AI channel and preprocessing AI channel.
 * Para:
 *     pvAiMod, handle of virtual box.
 *     ulSmplClk, sampling clock.
 *     ppfRtWr, pointer for AI channel.
 *     ppxRtWr, pointer for preprocessing channel.
 *     ppstsWr, statuc of vitrual box.
 *     ppbRtAiValidWr, if valid.
 *     pulRtAiCnt, AI counter.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VTBOX_AI_Dat_P(void *pvAiMod, uint32_t ulSmplClk, float **ppfRtWr, COMPLEX **ppxRtWr, VIRT_BOX_CH_STS **ppstsWr, BOOL **ppbRtAiValidWr, uint32_t *pulRtAiCnt);

/* report the updating of virtual box.
 * Para:
 *     pvAiMod, handle of the virtual box.
 *     ulAiCnt, AI counter.
 * Return: NONE.
 */
void VTBox_End_Ai_Wr(void *pvAiMod, uint32_t ulAiCnt);

/* If the data is valid on the virtual box.
 * Para:
 *     iPos, position of virtual box.
 *     ulAiCnt, the newest valid AI counter of virtual box.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VTBOX_Get_AI_Data_Valid(int iPos, uint32_t ulAiCnt);

/* Get the current AI counter in 10 cycles on local box.
 * Para:
 *     NONE.
 * Return:
 *     counter.
 */
uint32_t VTBOX_GetLocalSamClk(void);

/* Get the interrupt time corresponding to the current AI counter in 10 cycles on local box.
 * Para:
 *     ulSamCnt, sampling counter.
 *     pulRtTimeBaseH, high word.
 *     pulRtTimeBaseL, low word.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VTBOX_GetSamTimeBase(uint32_t ulSamCnt, uint32_t *pulRtTimeBaseH, uint32_t *pulRtTimeBaseL);

/* adjust the local sampling.
 * Para:
 *     ucVtBoxPos, positon of virtual box.
 *     iAdjMode, adjusting mode, 0: accelerate, 1: decelerate, 2: normal.
 *     iAdjSamPeriod, period after adjusting.
 *     iAdjSamCnt, times of adjusting.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL VTBOX_AdjustSamMode(uint8_t ucVtBoxPos, int iAdjMode, int iAdjSamPeriod, int iAdjSamCnt);

/* Get the adjusting mode of local sampling.
 * Para:
 *     ucVtBoxPos, positon of virtual box.
 *     piRtLeftAdjCnt, left adjusting counter.
 * Return:
 *     sampling adjusting mode, 0: accelerate, 1: decelerate, 2: normal.
 */
int VTBOX_GetAdjSamMode(uint8_t ucVtBoxPos, int *piRtLeftAdjCnt);

#ifdef	__cplusplus
}
#endif

#endif                                  /* VTBOX_INTERFACE_H */
