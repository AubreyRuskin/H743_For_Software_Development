/* FilesynPro.c - subroutine library for handling the file synchronization processing */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 05sep08, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling the file synchronization processing.
INCLUDES: adc.h
*/

/* includes */

#include "FileSynPro.h"
#include "edpbase.h"
#include "logmsg.h"
#include "view.h"
#include "rec.h"
#include "EdpVer.h"
#include "memLib.h"
#include "stdlib_compat.h"
#include "lstLib.h"

/* defines */

#define RES_BLOCK_NUM 16  /* 多余节点数 */
#define RES_BYTE_PER_BLOCK 16  /* 操作系统内存节点额外字节数 */
#define PART_RES_BYTE 12  /* 分区额外字节数 */

/* globals */

LIST *pmRecFileList_g;			/* Point to record wave file list. */
LIST *pmEvtFileList_g;			/* Point to event file list. */

SEM_ID semRecFileListWR_g;		/* Rec file list read and write semaphore. */
SEM_ID semEvtFileListWR_g;		/* Event file list read and write semaphore. */
PART_ID FileSynMemPartId;  /* 内存分区 */
uint8_t *pFileSynMemSpace = NULL;  /* 内存空间 */
uint32_t FileSynMemPartSize;  /* 内存大小 */

/* locals */

static LIST mRecFileList;		/* record wave file list. */
static LIST mEvtFileList;				/* event file list. */

/* local functions */

/***********************************************************************
* SearchPreviousNodeSN - Search the previous node by SN.
*
* RETURNS: FILENODE
*
*/
static NODE *SearchPreviousNodeSN (
    LIST *pList,
    FILENODE *pNode
);

/***********************************************************************
* SearchNodebyName - Search the node by name.
*
* RETURNS: TRUE, or FALSE
*
*/
static BOOL SearchNodebyName (
    LIST *pList,
    const char *pName,	/* full file name. */
    FILENODE **ppFileNode
);

/* read record file directory.
 * Para:
 *     pList, list.
 * Return:
 *     file number.
 */
static uint16_t ReadRecFileDir(LIST *pList);

/* read event file directory.
 * Para:
 *     pList, list.
 * Return:
 *     file number.
 */
static uint16_t ReadEvtFileDir(LIST *pList);

/* global functions */

/* functions */

/* Initialize the semaphore
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void FileSyn_Init_Sem(void)
{
    semRecFileListWR_g = semMCreate(SEM_Q_PRIORITY); /* Rec file list read and write semaphore. */
    assert (semRecFileListWR_g != NULL);

    semEvtFileListWR_g = semMCreate(SEM_Q_PRIORITY);  /* Rec file list read and write semaphore. */
    assert (semEvtFileListWR_g != NULL);

    pmRecFileList_g = &mRecFileList;		/* global point. */
    pmEvtFileList_g = &mEvtFileList;

    lstInit(&mRecFileList);		/* Initialize the list. */
    lstInit(&mEvtFileList);
}

/* read record file directory.
 * Para:
 *     pList, list.
 * Return:
 *     file number.
 */
static uint16_t ReadRecFileDir(LIST *pList)
{
    DIR *pdir;
    struct dirent *pent;
    STATUS vxsts;
    uint8_t TempFileName[FULL_NAME_LEN+1];
    FILENODE *pFileNode;
    struct stat Stat;
    uint16_t usCount = 0;
    int fp;
    uint8_t aucBuf[2];
    NODE *pDelNode;
    NODE *pInsertNode;

    /* Search entire directory to initialize the list. */
    pdir = opendir(EP_WAVE_REC_DIR);
    if (pdir)
    {
        while ((pent = readdir(pdir)) != NULL)
        {
            strcpy(TempFileName, EP_WAVE_REC_DIR);
            strcat(TempFileName, "/");
            strcat(TempFileName, pent->d_name);
            if (FT_Is_File(TempFileName) &&
                    pent->d_name[0] == 'e' && pent->d_name[1] == 'd' &&
                    pent->d_name[2] == 'p' && pent->d_name[7] == '.' &&
                    pent->d_name[8] == 'f' && pent->d_name[9] == 'r' &&
                    pent->d_name[10] == 'w' && pent->d_name[11] == '\0')
            {
                if (stat((uint8_t*)TempFileName, &Stat) != OK)
                {
                    /* Not exist. */
                    continue;
                }

                pFileNode = (FILENODE *)memPartAlloc(FileSynMemPartId, sizeof(FILENODE));		/* Request memory space. */
                assert(pFileNode);

                if ((fp = open(TempFileName, O_RDONLY, 0))>0)
                {
                    lseek(fp, 7, SEEK_SET);
                    if (read(fp, aucBuf, 2) != 2)
                    {
                        pFileNode->uiSwCfgFileCRC = 0x00;
                    }
                    else
                    {
                        pFileNode->uiSwCfgFileCRC = U8_TO_U16(aucBuf[1], aucBuf[0]);
                    }

                    lseek(fp, 6, SEEK_SET);
                    if (read(fp, aucBuf, 1) != 1)
                    {
                        pFileNode->ucDelAttr = 0x01;   /* fast delete. */
                    }
                    else
                    {
                        pFileNode->ucDelAttr = (aucBuf[0]&FAST_DEL_BIT) ? 0x01 : 0;
                    }

                    close(fp);
                }
                else
                {
                    pFileNode->uiSwCfgFileCRC = 0x00;
                }

                strcpy(pFileNode->ucFileName, pent->d_name);			/* File name. */
                strcpy(pFileNode->ucFullFileName, TempFileName);		/* The full file name. */

                if ((Stat.st_mode & S_IFMT) == S_IFREG)
                {
                    pFileNode->ucFileType = 0x01;			/* File. */
                }
                else
                {
                    pFileNode->ucFileType = 0x00;	/* directory. */
                }

                pFileNode->ulSize = Stat.st_size;
                pFileNode->timer = Stat.st_mtime;

                pFileNode->uiFileSN = strtoul(pent->d_name+3, NULL, 16);

                pInsertNode = SearchPreviousNodeSN(pList, pFileNode);

                pDelNode = pInsertNode->next;

                if (pDelNode
                        && (((FILENODE *)pDelNode)->uiFileSN == pFileNode->uiFileSN))
                {
                    vxsts = remove(((FILENODE *)pDelNode)->ucFullFileName);

                    if (vxsts == OK)
                    {
                        lstDelete(pList, pDelNode);
                        memPartFree(FileSynMemPartId, (char *)pDelNode);
                    }
                }

                pFileNode->bExistFlag = TRUE;
                pFileNode->ulNameLength = strlen(pent->d_name);

                if (pInsertNode == &pList->node)
                {
                    lstInsert(pList, NULL, (NODE *)pFileNode);
                }
                else
                {
                    lstInsert(pList, pInsertNode, (NODE *)pFileNode);
                }
                usCount++;
            }
            else if (!GetRecWrSts())
            {
                /* Delete the invalid files. */
                if (FT_Is_File(TempFileName))
                {
                    /* Delete file. */
                    remove(TempFileName);
                }
            }
        }
        vxsts = closedir(pdir);
        assert(vxsts == OK);
    }

    return usCount;
}

/* read event file directory.
 * Para:
 *     pList, list.
 * Return:
 *     file number.
 */
static uint16_t ReadEvtFileDir(LIST *pList)
{
    DIR *pdir;
    struct dirent *pent;
    STATUS vxsts;
    uint8_t TempFileName[FULL_NAME_LEN+1];
    FILENODE *pFileNode;
    struct stat Stat;
    uint8_t ucTempCharArr[5];
    uint16_t usCount = 0;
    NODE *pDelNode;
    NODE *pInsertNode;

    pdir = opendir(EP_EVT_RPT_DIR);
    if (pdir)
    {
        while ((pent = readdir(pdir)) != NULL)
        {
            strcpy(TempFileName, EP_EVT_RPT_DIR);
            strcat(TempFileName, "/");
            strcat(TempFileName, pent->d_name);
            if (FT_Is_File(TempFileName) &&
                    pent->d_name[0] == 'e' && pent->d_name[1] == 'd' &&
                    pent->d_name[2] == 'p' && pent->d_name[32] == '.' &&
                    pent->d_name[33] == 'e' && pent->d_name[34] == 'v' &&
                    pent->d_name[35] == 't' && pent->d_name[36] == '\0')
            {
                if (stat((uint8_t*)TempFileName, &Stat) != OK)
                {
                    /* Not exist. */
                    continue;
                }

                pFileNode = (FILENODE *)memPartAlloc(FileSynMemPartId, sizeof(FILENODE));		/* Request memory space. */
                assert(pFileNode);
                strcpy(pFileNode->ucFileName, pent->d_name);			/* File name. */
                strcpy(pFileNode->ucFullFileName, TempFileName);		/* The full file name. */

                if ((Stat.st_mode & S_IFMT) == S_IFREG)
                {
                    pFileNode->ucFileType = 0x01;			/* File. */
                }
                else
                {
                    pFileNode->ucFileType = 0x00;	/* directory. */
                }

                pFileNode->ulSize = Stat.st_size;
                pFileNode->timer = Stat.st_mtime;
                strncpy(ucTempCharArr, pent->d_name+3, 4);
                ucTempCharArr[4] = '\0';
                pFileNode->uiFileSN = strtoul(ucTempCharArr, NULL, 16);

                pInsertNode = SearchPreviousNodeSN(pList, pFileNode);

                pDelNode = pInsertNode->next;

                if (pDelNode
                        && (((FILENODE *)pDelNode)->uiFileSN == pFileNode->uiFileSN))
                {
                    vxsts = remove(((FILENODE *)pDelNode)->ucFullFileName);

                    if (vxsts == OK)
                    {
                        lstDelete(pList, pDelNode);
                        memPartFree(FileSynMemPartId, (char *)pDelNode);
                    }
                }

                pFileNode->bExistFlag = TRUE;
                pFileNode->ulNameLength = strlen(pent->d_name);

                strncpy(ucTempCharArr, pent->d_name+7, 2);
                ucTempCharArr[2] = '\0';
                pFileNode->ucDelAttr = strtoul(ucTempCharArr, NULL, 16);		/* Attribute for deleting. */

                if (pInsertNode == &pList->node)
                {
                    lstInsert(pList, NULL, (NODE *)pFileNode);
                }
                else
                {
                    lstInsert(pList, pInsertNode, (NODE *)pFileNode);
                }
                usCount++;
            }
            else if (!GetEvtWrSts())
            {
                /* Delete the invalid files. */
                if (FT_Is_File(TempFileName))
                {
                    /* Delete file. */
                    remove(TempFileName);
                }
            }
        }
        vxsts = closedir(pdir);
        assert(vxsts == OK);
    }

    return usCount;
}

/***********************************************************************
* FileSynInit - Initialize the file synchronization module.
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS FileSynInit(void)
{
    STATUS vxsts;
    uint16_t usCount = 0;

    /* 内存预分配 */
    FileSynMemPartSize = PART_RES_BYTE
                         +(sizeof(FILENODE)+RES_BYTE_PER_BLOCK)*(MAX_EVT_FILE_NUM+MAX_REC_FILE_NUM+RES_BLOCK_NUM);
    pFileSynMemSpace = (uint8_t *)malloc(FileSynMemPartSize);
    if (pFileSynMemSpace == NULL)
    {
        return EP_ERROR;
    }

    /* make this chunk a partition */
    FileSynMemPartId = memPartCreate (pFileSynMemSpace, FileSynMemPartSize);
    if (FileSynMemPartId == NULL)
    {
        return EP_ERROR;
    }

    vxsts = semTake(semRecFileListWR_g, WAIT_FOREVER); 		/* Waiting for semaphore. */
    assert (vxsts == OK);

    usCount = ReadRecFileDir(pmRecFileList_g);
    LOG_Dbg_Msg("Find %d record file in directory.\n", usCount, 0, 0, 0, 0, 0);

    vxsts = semGive(semRecFileListWR_g);
    assert(vxsts == OK);

    vxsts = semTake(semEvtFileListWR_g, WAIT_FOREVER); 		/* Waiting for semaphore. */
    assert(vxsts == OK);

    usCount = ReadEvtFileDir(pmEvtFileList_g);
    LOG_Dbg_Msg("Find %d event file in directory.\n", usCount, 0, 0, 0, 0, 0);

    vxsts = semGive(semEvtFileListWR_g);
    assert(vxsts == OK);

    return EP_SUCCESS;
}

/***********************************************************************
* GetFileType - Get the file type.
*
* RETURNS: 1, record file; 2, event file.
*
*/
uint8_t GetFileType (
    char *pName		/* file name. */
)
{
    uint8_t *pucPostfix;		/* postfix */

    assert(pName);

    pucPostfix = strrchr(pName, '.');

    if (pucPostfix == NULL)
    {
        return COMMOM_DIR;
    }
    pucPostfix++;
    if(!strcmp(pucPostfix, "frw"))
    {
        return REC_DIR;
    }
    else if(!strcmp(pucPostfix, "evt"))
    {
        return EVT_DIR;
    }
    else
    {
        return COMMOM_DIR;
    }
}

/***********************************************************************
* GetDirType - Get the directory type.
*
* RETURNS: 1, record dir; 2, event dir.
*
*/
uint8_t GetDirType (
    char *pDirName		/* Directory name. */
)
{
    assert(pDirName);

    if(!strcmp(pDirName, EP_WAVE_REC_DIR))
    {
        return REC_DIR;
    }
    else if(!strcmp(pDirName, EP_EVT_RPT_DIR))
    {
        return EVT_DIR;
    }
    else
    {
        return COMMOM_DIR;
    }
}

/***********************************************************************
* SearchPreviousNodeSN - Search the previous node by item.
*
* RETURNS: FILENODE
*
*/
static NODE *SearchPreviousNodeSN (
    LIST *pList,
    FILENODE *pNode
)
{
    FILENODE *pCurFileNode = NULL;
    uint16_t usDif = 0;
    BOOL bNotRear = FALSE;

    pCurFileNode = (FILENODE *)pList->node.next;		/* The first node. */

    if (!pCurFileNode)
    {
        return &pList->node;
    }

    if (bViewModIsInit_g)
    {
        usDif = (uint16_t)(rptsts_g.uStNewestSN - pNode->uiFileSN);
    }

    for (pCurFileNode = (FILENODE *)lstFirst(pList); pCurFileNode != NULL;
            pCurFileNode = (FILENODE *)lstNext((NODE *)pCurFileNode))
    {
        if (bViewModIsInit_g)
        {
            if (usDif <= (uint16_t)(rptsts_g.uStNewestSN-pCurFileNode->uiFileSN))
            {
                break;
            }
        }
        else
        {
            if ((int16_t)(pNode->uiFileSN-pCurFileNode->uiFileSN) >= 0)
            {
                break;
            }
        }

        if (((NODE *)pCurFileNode)->next == NULL)
        {
            break;
        }
    }

    if (bViewModIsInit_g)
    {
        if (usDif <= (uint16_t)(rptsts_g.uStNewestSN-pCurFileNode->uiFileSN))
        {
            bNotRear = TRUE;
        }
    }
    else
    {
        if ((int16_t)(pNode->uiFileSN-pCurFileNode->uiFileSN) >= 0)
        {
            bNotRear = TRUE;
        }
    }

    if (bNotRear)
    {
        if  (((NODE *)pCurFileNode)->previous)
        {
            return ((NODE *)pCurFileNode)->previous;
        }
        else
        {
            return &pList->node;
        }
    }
    else
    {
        return (NODE *)pCurFileNode;
    }
}

/***********************************************************************
* SearchNodebyName - Search the node by name.
*
* RETURNS: TRUE, or FALSE
*
*/
static BOOL SearchNodebyName (
    LIST *pList,
    const char *pName,	/* full file name. */
    FILENODE **ppFileNode
)
{
    FILENODE *pNextFileNode;

    pNextFileNode=(FILENODE *)pList->node.next;		/* The first node. */

    if(!pNextFileNode)
    {
        /* NULL */
        return FALSE;
    }

    if(!strcmp(pNextFileNode->ucFullFileName, pName))
    {
        /* The first node. */
        *ppFileNode=pNextFileNode;

        return TRUE;
    }

    while(((NODE *)pNextFileNode)->next)
    {
        /* The next is not NULL. */
        pNextFileNode = (FILENODE *)(((NODE *)pNextFileNode)->next);

        if(!strcmp(pNextFileNode->ucFullFileName, pName))
        {
            *ppFileNode=pNextFileNode;

            return TRUE;
        }
    }

    return FALSE;
}

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
    uint8_t ucFileType,
    uint8_t ucDelAttr
)
{
    uint8_t TempFileName[FULL_NAME_LEN+1];
    FILENODE *pFileNode;
    struct stat Stat;
    uint8_t ucTempCharArr[5];
    STATUS vxsts;
    STATUS bSts = ERROR;
    LIST *pList;

    if (ucFileType == REC_DIR)
    {
        semTake(semRecFileListWR_g, WAIT_FOREVER); 		/* Waiting for semaphore. */
        pList = pmRecFileList_g;
    }
    else if (ucFileType == EVT_DIR)
    {
        semTake(semEvtFileListWR_g, WAIT_FOREVER); 		/* Waiting for semaphore. */
        pList = pmEvtFileList_g;
    }
    else
    {
        return bSts;
    }

    pFileNode = (FILENODE *)memPartAlloc(FileSynMemPartId, sizeof(FILENODE));		/* Request memory space. */

    if (pFileNode == NULL)
    {
        bSts = ERROR;
        goto ret;
    }

    if (ucFileType == REC_DIR)
    {
        pFileNode->uiFileSN = strtoul(pName+3, NULL, 16);
    }
    else if (ucFileType == EVT_DIR)
    {
        NODE *pTmpNode = NULL;
        NODE *pDelNode = NULL;

        strncpy(ucTempCharArr, pName+3, 4);
        ucTempCharArr[4] = '\0';
        pFileNode->uiFileSN = strtoul(ucTempCharArr, NULL, 16);

        /* delete the same name file. */

        for (pTmpNode = lstFirst(pList); pTmpNode != NULL;)
        {
            pDelNode = pTmpNode;
            pTmpNode = lstNext(pTmpNode);
            if (((FILENODE *)pDelNode)->uiFileSN == pFileNode->uiFileSN)
            {
                vxsts = remove(((FILENODE *)pDelNode)->ucFullFileName);

                if (vxsts == OK)
                {
                    lstDelete(pList, pDelNode);
                    memPartFree(FileSynMemPartId, (char *)pDelNode);
                }
                else
                {
                    bSts = ERROR;
                    goto ret;
                }
            }
        }
    }

    /* Search entire directory. */

    if (ucFileType == REC_DIR)
    {
        strcpy(TempFileName, EP_WAVE_REC_DIR);
    }
    else
    {
        strcpy(TempFileName, EP_EVT_RPT_DIR);
    }

    strcat(TempFileName, "/");
    strcat(TempFileName, pName);

    vxsts = rename(aucTempNameBuf, TempFileName); 	/* 重命名新文件 */

    if (vxsts != OK)
    {
        bSts = ERROR;
        goto ret;
    }

    /* LOG_Dbg_Msg("Insert file %s\n", (int)pName, 0, 0, 0, 0, 0); */

    if (stat((uint8_t*)TempFileName, &Stat) != OK)
    {
        /* Not exist. */
        bSts = ERROR;
        goto ret;
    }

    if (ucFileType == REC_DIR)
    {
        pFileNode->uiSwCfgFileCRC = VER_ExtGetSwCfgCRC();
    }

    strcpy(pFileNode->ucFileName, pName);			/* File name. */
    strcpy(pFileNode->ucFullFileName, TempFileName);		/* The full file name. */

    if((Stat.st_mode & S_IFMT) == S_IFREG)
    {
        pFileNode->ucFileType=0x01;			/* File. */
    }
    else
    {
        pFileNode->ucFileType=0x00;	/* directory. */
    }

    pFileNode->ulSize=Stat.st_size;
    pFileNode->timer=Stat.st_mtime;

    pFileNode->bExistFlag=TRUE;
    pFileNode->ulNameLength=strlen(pName);
    pFileNode->ucDelAttr = ucDelAttr;		/* Attribute for deleting. */

    lstInsert(pList, NULL, (NODE *)pFileNode);
    bSts = OK;

ret:
    if (bSts != OK)
    {
        if (pFileNode)
            memPartFree(FileSynMemPartId, (char *)pFileNode);
    }

    if (ucFileType == REC_DIR)
    {
        semGive(semRecFileListWR_g);
    }
    else
    {
        semGive(semEvtFileListWR_g);
    }

    return bSts;
}

/* remove file from file system and list.
 * Para:
 *     name, name of the file to remove.
 *     ucFileType, file type, 1: record file; 2: event file.
 * Return:
 *     OK, or ERROR.
 */
STATUS FS_RemoveFile(const char *name, uint8_t ucFileType)
{
    STATUS vxsts;
    FILENODE *pFileNode = NULL;
    STATUS bSts = ERROR;
    LIST *pList;

    if (ucFileType == REC_DIR)
    {
        semTake(semRecFileListWR_g, WAIT_FOREVER); 		/* Waiting for semaphore. */
        pList = pmRecFileList_g;
    }
    else if (ucFileType == EVT_DIR)
    {
        semTake(semEvtFileListWR_g, WAIT_FOREVER); 		/* Waiting for semaphore. */
        pList = pmEvtFileList_g;
    }
    else
    {
        return bSts;
    }

    bSts = OK;

    if (SearchNodebyName(pList, name, &pFileNode))
    {
        lstDelete(pList, (NODE *)pFileNode);
        memPartFree(FileSynMemPartId, (char *)pFileNode);
        vxsts = remove(name);
    }

    /* LOG_Dbg_Msg("List count %d\n", pList->count, 0, 0, 0, 0, 0); */

    if (ucFileType == REC_DIR)
    {
        semGive(semRecFileListWR_g);
    }
    else
    {
        semGive(semEvtFileListWR_g);
    }

    return bSts;
}

/* judge if the SN existed in the list.
 * Para:
 *     uiFileSN, file SN.
 *     ucFileType, file type, 1: record file; 2: event file.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL FS_isExist(uint16_t uiFileSN, uint8_t ucFileType)
{
    FILENODE *pFileNode;
    LIST *pList = NULL;
    BOOL bSts = FALSE;

    if (ucFileType == REC_DIR)
    {
        semTake(semRecFileListWR_g, WAIT_FOREVER); 		/* Waiting for semaphore. */
        pList = pmRecFileList_g;
    }
    else if (ucFileType == EVT_DIR)
    {
        semTake(semEvtFileListWR_g, WAIT_FOREVER); 		/* Waiting for semaphore. */
        pList = pmEvtFileList_g;
    }

    for (pFileNode = (FILENODE *)lstFirst(pList); pFileNode != NULL;
            pFileNode = (FILENODE *)lstNext((NODE *)pFileNode))
    {
        if (pFileNode->uiFileSN == uiFileSN)
        {
            bSts = TRUE;
            goto ret;
        }
    }

ret:
    if (ucFileType == REC_DIR)
    {
        semGive(semRecFileListWR_g);
    }
    else if (ucFileType == EVT_DIR)
    {
        semGive(semEvtFileListWR_g);
    }

    return bSts;
}

/* Code for testing. */

/***********************************************************************
* ShowFileSynList - Show the file list.
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
void ShowFileSynList(void)
{
    FILENODE *pFileNode;
    STATUS vxsts;
    uint32_t ulFileCnt = 0;
    uint16_t usLowDelNum = 0;
    uint16_t usFastDelNum = 0;

    vxsts=semTake(semRecFileListWR_g, WAIT_FOREVER); 		/* Waiting for semaphore. */
    assert(vxsts == OK);

    printf("The future report SN is %x, the newest report SN is %x.\n", rptsts_g.unRptSN, rptsts_g.uStNewestSN);
    printf("The total record wave file number is %d\n", pmRecFileList_g->count);
    for (pFileNode=(FILENODE *)lstFirst(pmRecFileList_g); pFileNode != NULL;
            pFileNode=(FILENODE *)lstNext((NODE*)pFileNode))
    {
        printf("%d File name: %s SN: %x SwCRC: %x Del: %x\n",
               (int)ulFileCnt, pFileNode->ucFileName, pFileNode->uiFileSN,
               pFileNode->uiSwCfgFileCRC, pFileNode->ucDelAttr);	/* No directory file name. */
        ulFileCnt++;
        if (pFileNode->ucDelAttr)
        {
            usFastDelNum++;
        }
        else
        {
            usLowDelNum++;
        }
    }
    printf("usLowDelNum: %d usFastDelNum:%d.\n", usLowDelNum, usFastDelNum);

    vxsts=semGive(semRecFileListWR_g);
    assert(vxsts==OK);

    usFastDelNum = 0;
    usLowDelNum = 0;
    ulFileCnt = 0;
    vxsts=semTake(semEvtFileListWR_g, WAIT_FOREVER); 		/* Waiting for semaphore. */
    assert(vxsts == OK);
    printf("The total event file number is %d\n", (int)pmEvtFileList_g->count);
    for (pFileNode=(FILENODE *)lstFirst(pmEvtFileList_g); pFileNode != NULL;
            pFileNode=(FILENODE *)lstNext((NODE*)pFileNode))
    {
        printf("%d File name: %s SN: %x Del: %x\n",
               (int)ulFileCnt, pFileNode->ucFileName, pFileNode->uiFileSN, pFileNode->ucDelAttr);		/* No directory file name. */
        ulFileCnt++;
        if (pFileNode->ucDelAttr)
        {
            usFastDelNum++;
        }
        else
        {
            usLowDelNum++;
        }
    }
    printf("usLowDelNum: %d usFastDelNum:%d.\n", usLowDelNum, usFastDelNum);

    vxsts=semGive(semEvtFileListWR_g);
    assert(vxsts==OK);
}
