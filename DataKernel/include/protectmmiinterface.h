/* protectmmiinterface.h - subroutine library for supplying the interface between protect and mmi */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 23mar07, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for supplying the interface between protect and mmi .
*/

#ifndef PROTECTMMIINTERFACE_H
#define PROTECTMMIINTERFACE_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "lstLib.h"
#include "vxworks_type.h"

/* typedefs */

typedef struct
{
    /* 用于任务监视节点 */
    NODE tNode;
    BOOL bTaskBeCreated;
    BOOL bTaskIsGood;
    int iTaskID;
    char WarningMessage[MAX_ID_LEN];		/* 警告信息 */
    char ucTaskName[MAX_ID_LEN];	/* 任务名称 */
    BOOL bRebootFlag; /* 是否重启 */
} TTaskWatchDog;

/* functions */

#ifdef  __cplusplus
}
#endif

#endif
