/* VTBOX_SamInterface.h - subroutine library for virtual box interface to sampling */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 07nov08, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for virtual box interface ro sampling.
*/

#ifndef VTBOX_SAMINTERFACE_H
#define VTBOX_SAMINTERFACE_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "vxWorks.h"			/* always first */
#include "realdatadef.h"
#include "logic.h"
/* #include "dsp.h" */

/* defines */

#define VIRTBOX_AOofAI_BUF_LENGTH 200

/* typedefs */

typedef struct		/* AO channel configuration in virtual box. */
{
    uint8_t ucAOHdCh;                      /* AO physical channel number in virtual box, begin from 0 */
    int iAOSrcType;                          /* type of source, 1: coming from AI, 2: coming from logic graph variables. */
    uint8_t ucSrcAIHdCh;                 /* If coming from AI, the physical, begin from 0. */
    FLT_U32_UNION SrcAIPhyCoffUnion;         /* coefficient of AI */
    uint8_t ucSrcType;				/* data source type. */
    uint8_t ucUnit;
    uint8_t ucFiltTp;
    uint8_t ucFiltNum;                 					 /* filter number. */
    uint8_t ucArithParm; 				/* filter paramiter. */
    void *pElemSrc;                         /* IO pointer when using logic graph variables. */
} VIRT_AO_CFG;

typedef struct  	/* AO configuration in virtual box. */
{
    int iVirtBoxAONum;         /* total AO number. */
    int iVirtBoxAISrcAONum;    /* AO number from AI. */
    int iVirtBoxAISrcAONumTmp;    /* temporary variable when changing the AO coefficient from AI. */
    int32_t iVirtBoxPreSrcAONum;				/* from preprocessing. */
    int  iVirtBoxMidSrcAONum;    		/* AO number from logic graph variables. */
    VIRT_AO_CFG aVirtBoxAoCfg_g[MAX_VTBOX_AO_NUM];
} VIRT_BOX_AO_CFG;

/* AI configuration as AO in virtual box. */
typedef struct
{
    uint8_t ucAOHdCh;		/* AO SN, begin from 0. */
    uint8_t ucSrcAIHdCh;     /* AI SN on local box, begin from 0. */
    float fCoff;		 /* coefficient */
} VIRT_BOX_AI_CFG;

/* interface btween virtual box and A/D module. */
typedef struct
{
    VIRT_BOX_AI_CFG aVirtBoxAiCfg_g[MAX_VTBOX_AI_NUM]; 	/* AI configuration as AO in virtual box. */
    VIRT_BOX_AI_CFG aVirtBoxPreCfg_g[MAX_VTBOX_AI_NUM]; 	/* AI configuration as AO in virtual box. */
    uint16_t AINum; /* number of AI in AO configuration. */
    u_int uiTxPts; 				/* interval. */
    int32_t iVirtBoxAoDataByteLen; 				/* length of buffer. */
    int32_t iVirtBoxAoPreDataByteLen; 				/* length of processing buffer. */
    int16_t *pVirtBoxAIResult; 		/* base address of AI buffer as AO. */
    int16_t *pVirtBoxPreResult; 						/* float base address of AI buffer as AO. */
    int16_t *pArray[MAX_VTBOX_AI_NUM];  /* pointer of sampling channel. */
    int32_t **ppArray;
    int32_t iPos[MAX_VTBOX_AI_NUM];
    uint16_t PreNum; /* number of processing variable in AO configuration. */
    float **ppfPreArray;
    int32_t iPrePos[MAX_VTBOX_AI_NUM];
    int32_t iZeroCurPos[MAX_VTBOX_AI_NUM];
    UINT32 SamTimebuf[10*MAXSAMPPOINT*2]; /* sampling time. */
    uint32_t RegNum; 				/* reference value of timer. */
    uint32_t BackupRegNum; 						/* backup reference value of timer. */
    int32_t iAdjMode; 				/* mode of adjusting. */
    int32_t  iAdjSamCnt; /* counter of adjusting. */
} VIRT_BOX_AI_MOD;

/* globals */

extern VIRT_BOX_AI_MOD VirtBoxAIMod_g[MAX_VT_BOX_COUNT];			/* information of AO from AI. */
extern uint8_t **pucVirtBoxAoDataByteBaseSamp_g[MAX_VT_BOX_COUNT];       /* data base address of AI as AO. */
extern int16_t VirtBoxAiBuf[MAX_VT_BOX_COUNT][VIRTBOX_AOofAI_BUF_LENGTH]; /* sending buffer. */
extern int16_t *pVirtBoxAdcData[MAX_VT_BOX_COUNT]; 			/* Sampling data in a cycle. */
extern BOOL bVirtBoxAoSendFlag[MAX_VT_BOX_COUNT]; 	/* permitting flag. */
extern int iVirtBoxTotalNumofTrans[MAX_VT_BOX_COUNT];
extern int16_t tmpVirtBoxBuf[MAX_VT_BOX_COUNT][2*VIRTBOX_AOofAI_BUF_LENGTH]; /* temporary buffer. */

extern BOOL VirtBoxAdjustFlag[MAX_VT_BOX_COUNT]; 		/* adjusting flag. */
extern VIRT_BOX_AO_CFG BoxAoCfgVirt_g[MAX_VT_BOX_COUNT];  /* AO configuration of virtual box. */

/* global functions */

/* Initialize the virtual box, including AO and DO.
 * Para:
 *     ucVtBoxPos, position of virtual box.
 *     uiSmplRate, sampling point per cycle.
 *     uiSysFreq, system frequency.
 *     pvAiMod, virtual box handle.
 *     uiLgcCh, number of origin channel.
 *     plgccfg, pointer to origin channel.
 *     uiCalcCfg, number of processing channel.
 *     pcalccfg, pointer to processing channel.
 *     iVirtBoxAoCh, number of AO.
 *     pVirtBoxAoCfg, pointer to AO.
 *     iVirtBoxDiCh, number of DI.
 *     pucVirtBoxDiStsBase, pointer to DI.
 *     iVirtBoxDoCh, number of DO.
 *     pucVirtBoxDoStsBase, pointer to DO.
 *     uiLgcChLocal, number of logic channel in local box.
 *     plgccfgLocal, first element of logic channel in local box.
 *     uiCalcChLocal, number of proprocessing in local box.
 *     pcalccfgLocal, first element of proprocessing in local box.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_CFG_ERR, fail.
 */
EP_STATUS Init_Virt_Box(uint8_t ucVtBoxPos, u_int uiSmplRate, u_int uiSysFreq, void *pvAiMod,
                        u_int uiLgcCh, DSP_LGC_AI_CFG *plgccfg,
                        u_int uiCalcCfg, DSP_CALC_AI_CFG *pcalccfg, VIRT_BOX_AO_CFG *pBoxAoCfg,
                        u_int uiLgcChLocal,
                        DSP_LGC_AI_CFG *plgccfgLocal,
                        u_int uiCalcChLocal,
                        DSP_CALC_AI_CFG *pcalccfgLocal
                       );

/* Initialize the AO from AI and Preprocessing.
 * Para:
 *     ucVtBoxPos, positon of virtual box.
 *     iVirtBoxAONum, local number of AO.
 *     pVirtBoxAoCfg, configuration of AO.
 *     iAoDataByteLen, byte length of AO from AI.
 *     pucOrgAISrc, pointer of pointer to address saving the AI data.
 *     iAoDataByteLenPre, byte length of AO from processing.
 *     pucOrgPreSrc, pointer of pointer to address saving the processing data.
 *     uiLgcChLocal, number of logic channel in local box.
 *     plgccfgLocal, first element of logic channel in local box.
 *     uiCalcChLocal, number of proprocessing in local box.
 *     pcalccfgLocal, first element of proprocessing in local box.
 * Return:
 *     EP_SUSSESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS Init_VirtBox_AOofAIPre(uint8_t ucVtBoxPos, int iVirtBoxAONum, VIRT_AO_CFG *pVirtBoxAoCfg, int32_t iAoDataByteLen, uint8_t **pucOrgAISrc,
                                 int32_t iAoDataByteLenPre, uint8_t **pucOrgPreSrc, u_int uiLgcChLocal,
                                 DSP_LGC_AI_CFG *plgccfgLocal,
                                 u_int uiCalcChLocal,
                                 DSP_CALC_AI_CFG *pcalccfgLocal);

/* fill in the buffer of virtual box AO from AI, called in interrupt.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void WrVirtBoxBuf(void);

/* calling the periodly called driver of virtual box.
 * Para:
 *     ucCallType, type of calling.
 *     ulCnt, counter.
 * Return:
 *     NONE.
 */
void VirtBoxPeriodDrvCall(uint8_t ucCallType, uint32_t ulCnt);

/* showing the status of all virtual box.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void ShowAllVirtBoxSts(void);

/* clearing the status of all virtual box.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void ClearAllVirtBoxSts(void);

/* calling the initialization driver of virtual box.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void VirtBoxInitDrvCall(void);

/* update the AO coefficient from AI.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS UpdateVirtBoxAcCoff(void);

/* Get the most delayed synchronizated local AI counter of the virtual box.
 * Para:
 *     NONE.
 * Return:
 *     AI counter.
 */
uint32_t get_VTBOX_MaxDelay_AI_Cnt(void);

/* Get the origin sampling data.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void GetOriginSampData(void);

#ifdef	__cplusplus
}
#endif

#endif                                  /* VTBOX_SAMINTERFACE_H */
