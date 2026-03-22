/* FilesynPro.h - subroutine library for handling the file synchronization processing */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 05sep08, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling the file synchronization processing.
*/

#ifndef FILESYNPRO_H
#define FILESYNPRO_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

//#include "vxWorks.h"
#include "filetool.h"
#include <dirent_compat.h>
#include <sys/stat.h>
#include "time_compat.h"
#include <lstLib.h>
#include <dirent_compat.h>
#include <semLib.h>

/* defines */

#define COMMOM_DIR 0
#define REC_DIR 1
#define EVT_DIR 2

/* typedefs */

typedef struct filenode	  /* Node of a linked file list. */
{
    NODE node;		/* common node */
    uint8_t ucFileName[FULL_NAME_LEN+1];		/* File name. */
    uint8_t ucFullFileName[FULL_NAME_LEN+1];
    uint8_t ucFileType;			/* File type. 0x00: directory, 0x01: file. */
    uint32_t ulSize;					/* Length of file. */
    time_t timer;	/* time of last modification */
    uint16_t uiFileSN;		/* Sequence number. */
    BOOL bExistFlag;			/* If exist in file. */
    uint32_t ulNameLength;			/* Length of file name. */
    uint8_t ucDelAttr;			/* Attribution for deleting. */
    uint16_t uiSwCfgFileCRC;       /* swcfg.esc CRC, used for rec file. */
} FILENODE;

/* globbals */

extern LIST *pmRecFileList_g;			/* Point to record wave file list. */
extern LIST *pmEvtFileList_g;			/* Point to event file list. */
extern SEM_ID semRecFileListWR_g;		/* Rec file list read and write semaphore. */
extern SEM_ID semEvtFileListWR_g;		/* Event file list read and write semaphore. */

/* functions */

/* Initialize the semaphore
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void FileSyn_Init_Sem(void);

/***********************************************************************
* FileSynInit - Initialize the file synchronization module.
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS FileSynInit(void);

/***********************************************************************
* ShowFileSynList - Show the file list.
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
void ShowFileSynList(void);

/***********************************************************************
* GetFileType - Get the file type.
*
* RETURNS: 1, record file; 2, event file.
*
*/
uint8_t GetFileType (
    char *pName		/* file name. */
);

/***********************************************************************
* GetDirType - Get the directory type.
*
* RETURNS: 1, record dir; 2, event dir.
*
*/
uint8_t GetDirType (
    char *pDirName		/* Directory name. */
);

/* remove file from file system and list.
 * Para:
 *     name, name of the file to remove.
 *     ucFileType, file type, 1: record file; 2: event file.
 * Return:
 *     OK, or ERROR.
 */
STATUS FS_RemoveFile(const char *name, uint8_t ucFileType);

/* insert file to file system and list.
 * Para:
 *     aucTempNameBuf, name of the temporary file.
 *     pName, name of the file to insert.
 *     ucFileType, file type, 1: record file; 2: event file.
 * Return:
 *     OK, or ERROR.
 */
STATUS FS_SearchInsertFile(
    const char *aucTempNameBuf,
    const char *pName,	/* file name. */
    uint8_t ucFileType,  /* file type. */
    uint8_t ucDelAttr       /* delete attribution. */
);

/* judge if the SN existed in the list.
 * Para:
 *     uiFileSN, file SN.
 *     ucFileType, file type, 1: record file; 2: event file.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL FS_isExist(uint16_t uiFileSN, uint8_t ucFileType);

#ifdef  __cplusplus
}
#endif

#endif
