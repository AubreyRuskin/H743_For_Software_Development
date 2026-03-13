/* VTBOX_Data.h - subroutine library for virtual box data */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 07nov08, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for virtual box data.
*/

#ifndef VTBOX_DATA_H
#define VTBOX_DATA_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "vxWorks.h"			/* always first */
#include "realdatadef.h"

#if defined(EDP_01_02_BUILD)
#include "spiio.h"
#elif defined(EDP03_BUILD) || defined(EXCITE_BUILD)
#include "io_Drv.h"
#endif

#include "VTBOX_Interface.h"
#include "VTBOX_SamInterface.h"

/* defines */

/* typedefs */

typedef struct		/* status of the virtual box DB. */
{
    u_int uiTotalCh;
    VIRT_BOX_CH_STS *pBufBgn;
    VIRT_BOX_CH_STS *pBufEnd;
    uint32_t ulBufLen;
    u_int uiChBytes;
    uint32_t ulBufBytes;
} VIRT_BOX_CH_STS_DB;

typedef struct		/* information of the IO module in virtual box. */
{
    SUB_MOD_TYPE type;                  /* type. */
    uint16_t unDiChNum;                 /* DI number. */
    uint16_t unDoChNum;                 /* DO number. */
} VIRT_BOX_IO_MOD_INFO;

typedef struct			/* DI handle. */
{
    int iPos;          	/* virtual box posiont ,begin from 0. */
    uint8_t ucMod;        		/* address of the module. */
    uint8_t ucHdCh;        /* channel number of the DI in this module. */
    uint8_t aucFilt[4];
    uint8_t *pucDiStsPos;    /* The position of the DI pointer in receiving buffer, unit Byte. */
    uint8_t ucDiStsMsk;    /* corresponding mask of the DI pointer in receiving buffer. */
} VIRT_BOX_DI_HND;

typedef struct		/* DO handle. */
{
    int iPos;          /* virtual box posiont ,begin from 0. */
    uint8_t ucMod;        /* address of the module. */
    uint8_t ucHdCh;       /* channel number of the DO in this module.*/
    uint8_t *pucDoStsPos;    /* The position of the DO pointer in receiving buffer, unit Byte. */
    uint8_t ucDoStsMsk;   /* corresponding mask of the DI pointer in receiving buffer. */
} VIRT_BOX_DO_HND;

typedef struct	 /* information of the all IO module in virtual box. */
{
    VIRT_BOX_IO_MOD_INFO aVirtBoxIOModInfo[MAX_MOD_NUM];		/* array of information about the IO module in virtual box, begin from 0. */

    int iVirtBoxDiNum_g;                 /* number of DI in the whole virtual box. */
    VIRT_BOX_DI_HND ahVirtBoxDiHandle[MAX_MOD_NUM*MAX_DI_PER_MOD];  /* array of DI handle. */

    int iVirtBoxDoNum_g;                 /* number of DO in the whole virtual box. */
    VIRT_BOX_DO_HND ahVirtBoxDoHandle[MAX_MOD_NUM*MAX_DO_PER_MOD]; 	 		/* array of DO handle. */
} VIRT_BOX_IO_INFO;

/* globals */

extern VIRT_BOX_CH_STS_DB virtstsdb_g;    /* virtual box DB status. */
extern VIRT_BOX_IO_INFO aVirtBoxIoInfo_g[MAX_VT_BOX_COUNT];		/* IO module information in virtual box. */

/* functions */

/* Initialize the AO channel configuration using middle variables in the virtual box
 * Para:
 *     iPos, position of the virtual box, begin from 0.
 *     iSrcType, type of the data source, middle or AI.
 *     ucSrcValType, type of value.
 *     uiCh, channel number in the module, begin from 0.
 *     pElemIOSrc, the pointer as the middle output in the logic.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS Virt_Init_Mid_Src_AO(int iPos, int iSrcType, uint8_t ucSrcValType, u_int uiCh, void *pElemIOSrc);

/* Initialize the AO channel configuration using origin AI in the virtual box
 * Para:
 *     iPos, position of the virtual box, begin from 0.
 *     iSrcType, type of the data source.
 *     uiCh, channel number in the module.
 *     uiSrcAiCh, channel number of AO in local box.
 *     fSrcAiPhyCoff, coefficient of AI.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VirtBox_Init_AI_Src_AO(int iPos, int iSrcType, u_int uiCh, u_int uiSrcAiCh, float fSrcAiPhyCoff);

/* Initialize the AO channel configuration using processing varibles in the virtual box
 * Para:
 *     iPos, position of the virtual box, begin from 0.
 *     iSrcType, type of the data source, AI, processing, or middle.
 *     ucSrcValType, type of value.
 *     uiCh, channel number in the module, begin from 0.
 *     uiSrcAiCh, channel number of AO in local AI box, begin from 0.
 *     ucFiltTp, number of filter arithmetic.
 *     ucUnit, unit.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VirtBox_Init_Pro_Src_AO(int iPos, int iSrcType, uint8_t ucSrcValType, u_int uiCh, u_int uiSrcAiCh, uint8_t ucFiltTp, uint8_t ucUnit);

/* Initialize the IO of virtual box.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VirtBox_IO_Initialize(void);

/* Initialize the DI of virtual box.
 * Para:
 *     iPos, position of the virtual box, begin from 0.
 *     iModAddr, physical address of module.
 *     uiCh, channel number.
 *     ulFilt, value of filter.
 * Return:
 *     pointer of DI index, or NULL.
 */
void *VirtBox_Init_DI(int iPos, int iModAddr, u_int uiCh, uint32_t ulFilt);

/* Initialize the DO of virtual box.
 * Para:
 *     iPos, position of the virtual box, begin from 0.
 *     iModAddr, physical address of module.
 *     uiCh, channel number.
 * Return:
 *     pointer of DO index, or NULL.
 */
void *VirtBox_Init_DO(int iPos, int iModAddr, u_int uiCh);

/* Initialize the count of virtual box AO.
 * Para:
 *     iPos, position of the virtual box, begin from 0.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VirtBox_InitAOCfgCount(int iPos);

/* Change the coefficient of AO using local AI.
 * Para:
 *     iPos, position of the virtual box, begin from 0.
 *     fSrcAiPhyCoff, coefficient of AI.
 * Return:
 *     EP_SUCCESS, success.
 *     EP_ERROR, fail.
 */
EP_STATUS VirtBox_Chg_AI_Src_AO_Coff(int iPos, float fSrcAiPhyCoff);

/* Get the status of DI in virtual box.
 * Para:
 *     pvDiCh, handle of DI.
 * Return:
 *     current status, TRUE=close or FALSE=close.
 */
BOOL VirtBox_Get_DI(void *pvDiCh);

/* Set the status of DO in virtual box.
 * Para:
 *     pvDoCh, handle of DO.
 *     bClose, current status, TRUE=close or FALSE=close
 * Return: NONE.
 */
void VirtBox_Set_DO(void *pvDoCh, BOOL bClose);

/* If having the IO in this virtual box.
 * Para:
 *     iPos, position of the virtual box.
 * Return: TRUE, or FALSE.
 */
BOOL Virt_BoxIsCfgDIO(int iPos);

/* Get the DO buffer base address of virtual box.
 * Para:
 *     iPos, position of virtual box.
 * Return:
 *     base address of DO buffer,
 *     the driver will read the every bit status of DO buffer according to physical address of module.
 */
uint8_t *getVirtBoxDoBufBaseAddr(uint8_t iPos);

/* Set the DI buffer base address of virtual box.
 * Para:
 *     iPos, position of virtual box.
 * Return:
 *     base address of DI buffer,
 *     the driver will fill in the every bit status of DI buffer according to physical address of module.
 */
uint8_t *SetVirtBoxDiBufBaseAddr(uint8_t iPos);

/* Get the sampling clock according to the virtual box AI counter.
 * Para:
 *     pvAiMod, handle of virtual box.
 *     ulScanAiCnt, local virtual box counter.
 * Return:
 *     match sampling clock in 10 cycles counter.
 * Alert:
 *     The parameter AI counter must be smaller than the newest AI counter.
 */
uint8_t VirtBox_GetMatchSamClkByAiCnt(void *pvAiMod, uint32_t ulScanAiCnt);

#ifdef	__cplusplus
}
#endif

#endif                                  /* VTBOX_DATA_H */
