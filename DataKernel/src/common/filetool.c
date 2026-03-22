/* filetool.c - This file contains functions to operator files(INI, update...) */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01d,23apr10,sdm modify bug of lstGet needing free node.
01c, 29apr03, hdx Updated to version 1.0.
01b, 03mar03, hdx Verified version 0.1.
01a, 19feb03, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains functions to operator files(INI, update...).
INCLUDES: filetool.h
*/

/* includes */

#include "filetool.h"
#include "datetime.h"
#include "miscfunc.h"
#include "logmsg.h"
#include "errtest.h"
#include "sysinfo.h"
#include "hwcfg.h"
#include "FileCRC.h"

#include "string_compat.h"
#include "ctype_compat.h"
#include <stdio_compat.h>
#include <dirent_compat.h>
#include <sys/stat.h>
#include <ioLib.h>
#include <lstLib.h>
#include <semLib.h>
#include <dosFsLib.h>
#include <taskLib.h>

#include "vxworks_io_compat.h"

/* defines */

#define INI_LINE_LEN    255

#define FILE_BUF_LEN    1024

#define HW_CFG_UPDT_TMP     0x01
#define SW_CFG_UPDT_TMP     0x02
#define LGC_CFG_UPDT_TMP    0x04
#define VX_CFG_UPDT     0X08
#define SYS_CFG_UPDT     0X10
#define APP_CFG_UPDT     0X20
#define USB_CFG_UPDT     0X40
#define MMI_CFG_UPDT     0X80
#define AUTOEXEC_CFG_UPDT     0X100

/* typedefs */

typedef struct
{
    NODE node;
    uint8_t aucItem[INI_TAG_LEN+1];
    uint8_t aucVal[INI_TAG_LEN+1];
} FT_INI_ELEM;

/* locals */

static SEM_ID semSysIni_g;
static SEM_ID semAucSysIni_g; /* 附加系统配置信息信号 */

static uint8_t *apucRootDir[]=
{
    EP_ROOT "/sys",
    EP_ROOT "/lgh",  /* 所有的平台都建立该目录 */
};


static uint8_t *apucSetDir[]=
{
    EP_SET "/ini",
    EP_SET "/set",
    EP_SET "/log",
    EP_SET "/bak",
};

static uint8_t *apucChkDir[]=
{
    EP_ROOT "/sys",
    EP_SET "/61850",	/* 所有的平台都建立该目录 */
    EP_SET "/ini",
    EP_SET "/set",
    EP_SET "/bak",
    EP_DATA "/rec",
};

BOOL b01IniChanged=FALSE;

static uint8_t DSamStsIni[]=
{
    "[MUDELAY]\n"
    "MUCnt=0\n"
};

static uint8_t aucDftIni[]=
{
    "[HARDWARE]\n"
    "HwSN=\n"
    "RegCode=\n"
    "\n"
    "[SYSTEM]\n"
    "HwCfgVer=\n"
    "SwCfgVer=\n"
    "LgcCfgVer=\n"
    "FuncOptVer=\n" /* 选配功能文件 */
    "VxCfgVer=\n"
    "BootromVer=0\n"
    "SysCfgVer=0\n"
    "AppCfgVer=0\n"
    "UsbCfgVer=\n"
    "MmiCfgVer=\n"
    "AutoexecCfgVer=0\n"
    "LogNum=1000\n"
    "EvtNum=500\n"
    "RecNum=128\n"
    "SetArea=32\n"
    "NeedHdCof=0\n"
    "NeedCtRatio=0\n"
    "NeedClCof=0\n"
#ifdef EN_VER
    "Language=1\n"			/* Language select */
#else
    "Language=0\n"			/* Language select */
#endif
    "PwrFreq=50\n"					/* Frequency of power line. */
    "AcMdType=0\n"									/* AC mould type. 0: 1A；1: 5A */
    "AcMdTypeChgFlag=0\n"									/* AC mould type changed flag, 0: no changed, 1: changed */
    "NewestSN=-1\n"
    "\n"
    "[NET]\n"
#ifdef EDP02_PSR_BUILD                      /*psr660u网口倒数*/
    "Net1IP=192.168.0.123\n"
    "Net2IP=172.40.20.234\n"
    "Net3IP=172.50.20.234\n"
    "Net1IPMask=255.255.255.0\n"
    "Net2IPMask=255.255.255.0\n"
    "Net3IPMask=255.255.255.0\n"
#else
    "Net1IP=172.40.20.234\n"				/* 对保护goose应用，保存网口的IP号 */
    "Net2IP=192.168.0.123\n"
#if defined(EDP03_BUILD)
    "Net3IP=172.30.20.234\n"		/* 第三个网口 */
#endif
    "Net1IPMask=255.255.255.0\n"				/* 保存网口的子网掩码 */
    "Net2IPMask=255.255.255.0\n"
#if defined(EDP03_BUILD)
    "Net3IPMask=255.255.255.0\n"		/* 第三个网口 */
#endif
#endif
    "\n"
    "[CRC]\n"
    "DIFORCE=\n"
    "FUNC=\n"
    "FUNCWRSTS=\n"
    "LINKSTATS=\n"
    "LINKSTATWRSTS=\n"
    "LINKMODE=\n"
    "NBSET=\n"
    "CKSET=\n"
    "CKSETWRSTS=\n"
    "HDCOF=\n"
    "CLCOF=\n"
    "AREA0=\n"
    "AREAWRSTS0=\n"
    "AREA1=\n"
    "AREAWRSTS1=\n"
    "AREA2=\n"
    "AREAWRSTS2=\n"
    "AREA3=\n"
    "AREAWRSTS3=\n"
    "AREA4=\n"
    "AREAWRSTS4=\n"
    "AREA5=\n"
    "AREAWRSTS5=\n"
    "AREA6=\n"
    "AREAWRSTS6=\n"
    "AREA7=\n"
    "AREAWRSTS7=\n"
    "AREA8=\n"
    "AREAWRSTS8=\n"
    "AREA9=\n"
    "AREAWRSTS9=\n"
    "AREA10=\n"
    "AREAWRSTS10=\n"
    "AREA11=\n"
    "AREAWRSTS11=\n"
    "AREA12=\n"
    "AREAWRSTS12=\n"
    "AREA13=\n"
    "AREAWRSTS13=\n"
    "AREA14=\n"
    "AREAWRSTS14=\n"
    "AREA15=\n"
    "AREAWRSTS15=\n"
    "AREA16=\n"
    "AREAWRSTS16=\n"
    "AREA17=\n"
    "AREAWRSTS17=\n"
    "AREA18=\n"
    "AREAWRSTS18=\n"
    "AREA19=\n"
    "AREAWRSTS19=\n"
    "AREA20=\n"
    "AREAWRSTS20=\n"
    "AREA21=\n"
    "AREAWRSTS21=\n"
    "AREA22=\n"
    "AREAWRSTS22=\n"
    "AREA23=\n"
    "AREAWRSTS23=\n"
    "AREA24=\n"
    "AREAWRSTS24=\n"
    "AREA25=\n"
    "AREAWRSTS25=\n"
    "AREA26=\n"
    "AREAWRSTS26=\n"
    "AREA27=\n"
    "AREAWRSTS27=\n"
    "AREA28=\n"
    "AREAWRSTS28=\n"
    "AREA29=\n"
    "AREAWRSTS29=\n"
    "AREA30=\n"
    "AREAWRSTS30=\n"
    "AREA31=\n"
    "AREAWRSTS31=\n"


};

/* 1588对时通信 */
static uint8_t auc1588Ini[] =
{
    "# 1588PTP Version 2.0\n"
    "# Editor: Yepinyong\n"
    "# Profile identifier: 00-1B-19-00-01-00\n"
    "\n"
    "[ModeSelect]\n"
    "syncOneStep=N\n"
    "slaveOnly=N\n"
    "ptpTimescale=N\n"
    "delayMechanism=2\n"
    "domainNumber=0\n"
    "currentUtcOffset=33\n"
    "\n"
    "[PTPProfile]\n"
    "priority1=128\n"
    "priority2=128\n"
    "offsetScaledLogVariance=0\n"
    "logAnnounceInterval=2\n"
    "logSyncInterval=-1\n"
    "logMinDelayReqInterval=1\n"
    "logMinPdelayReqInterval=1\n"
    "announceReceiptTimeout=5\n"
    "\n"
    "[Latency]\n"
    "inboundLatency=0\n"
    "outboundLatency=0\n"
    "syncAdjustValue=0\n"
    "delayReqAdjustValue=0\n"
    "pDelayReqAdjustValue=0\n"
    "pDelayRespAdjustValue=0\n"
    "\n"
    "[PPSSignalOutput]\n"
    "ppsEnableFlag=Y\n"
    "ppsRiseOrFallFlag=N\n"
    "ppsStartTime=5\n"
    "ppsGpio=1\n"
    "\n"
    "[ClockOutput]\n"
    "clkOutEnableFlag=N\n"
    "phaseAlignClkoutFlag=N\n"
    "clkOutSlew=N\n"
    "clkOutDivide=0\n"
    "clkOutSource=0\n"
    "\n"
    "[NetConfig0]\n"
    "srcUdpPort=2009\n"
    "srcIpAddress=172.20.20.234\n"
    "\n"
    "[NetConfig1]\n"
    "srcUdpPort=2010\n"
    "srcIpAddress=172.30.20.234\n"
};

/* 附加的系统配置信息 */
static uint8_t aucAuxDftIni[] =
{
    "[SYSTEM]\n"
    "NewestSN=-1\n"
};

/* globals */

BOOL bEvtDirExistFlag_g=TRUE;		/* 事件目录创建成功标志 */
BOOL bMmiEvtDirExistFlag_g=TRUE;		/* 操作记录目录创建成功标志 */
BOOL bRecDirExistFlag_g=TRUE;			/* 录波目录创建成功标志 */

extern UNITE_VER_INFO UnVerInfo_g;
/* local functions */

static int FT_Rd_Line_Mem(uint8_t *pucFile, uint8_t *pucLine);
static int FT_Find_Item_Mem(uint8_t *ppucVal);
static EP_STATUS FT_Wr_Sys_INI_Item(const uint8_t *strFile, const uint8_t *strHeader,
                                    const uint8_t *strItems, const uint8_t *strVals);

static void FT_Temp_Name(uint8_t *pucRslt, const uint8_t *strFile);
static EP_STATUS FT_Rd_Line(int iFd, uint8_t *pucLine);
static uint8_t *FT_Fmt_Str(uint8_t *strOrg);
static const uint8_t *FT_Copy_INI_Tag(uint8_t *pucD, const uint8_t *pucS);
static FT_INI_ELEM *FT_Find_Item(LIST *plist,
                                 uint8_t *pucFlItem, uint8_t **ppucVal);


/* functions */

/* initialize the file system, supporting partial area.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
EP_STATUS FT_Init_Cfg(void)
{
//uint8_t aucTempFile[FULL_NAME_LEN+1];
//int iFd;
    int i;
//EP_STATUS sts;
    STATUS vxsts;
    BOOL bInitSuccess = TRUE;

    LOG_Dbg_Msg("Enter filetool Init func.\n", 0, 0, 0, 0, 0, 0);

    for (i=0; i<sizeof(apucRootDir)/sizeof(apucRootDir[0]); i++)
    {
        if (!FT_Is_Dir(apucRootDir[i]))
        {
            vxsts = mkdir(apucRootDir[i]);
            if (vxsts != OK)
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "文件目录创建失败(%2d)\n", SYS_FOLDER, 0);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "Directory creating failed(%2d)\n", SYS_FOLDER, 0);
                }
                LOG_Write(LOG_KERNEL, "SYS分区文件目录创建失败!!\n", NULL);

                assert(FALSE);

                return EP_NOT_INIT;
            }
        }
    }

    for (i=0; i<sizeof(apucSetDir)/sizeof(apucSetDir[0]); i++)
    {
        if (!FT_Is_Dir(apucSetDir[i]))
        {
            vxsts = mkdir(apucSetDir[i]);
            if(vxsts!=OK)
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "文件目录创建失败(%2d)\n", SET_FOLDER, 0);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_STORAGE_ERR,
                               ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                               "Directory creating failed(%2d)\n", SET_FOLDER, 0);
                }
                LOG_Write(LOG_KERNEL, "SET分区文件目录创建失败!!\n", NULL);
                assert(FALSE);

                return EP_NOT_INIT;
            }
        }
    }


    if (bInitSuccess)
    {
        return EP_SUCCESS;
    }
    else
    {
        return EP_NOT_INIT;
    }
}

/***********************************************************************
* FT_After_Relay_Init_Cfg - 提供保护启动之后的文件系统初始化代码
*
* RETURNS: 无
*
*/
EP_STATUS FT_After_Relay_Init_Cfg(void)
{
    uint8_t aucFileName[FULL_NAME_LEN+1];
    uint8_t *puc;
    DIR *pdir;
    struct dirent *pent;
    int i;
    STATUS vxsts;
    BOOL bInitSuccess = TRUE;
    char *p =NULL;
    char TempInfo[256];

    if (!FT_Is_Dir(EP_EVT_RPT_DIR))
    {
        vxsts = mkdir(EP_EVT_RPT_DIR);
        if (vxsts != OK)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "文件目录创建失败(%2d)\n", DATA_FOLDER, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "Directory creating failed(%2d)\n", DATA_FOLDER, 0);
            }
            LOG_Write(LOG_KERNEL, "DATA分区文件目录创建失败!!\n", NULL);
            bEvtDirExistFlag_g = FALSE;
            bInitSuccess = FALSE;
        }
    }

    if (!FT_Is_Dir(EP_MMIEVT_RPT_DIR))
    {
        vxsts = mkdir(EP_MMIEVT_RPT_DIR);
        if (vxsts != OK)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "文件目录创建失败(%2d)\n", DATA_FOLDER, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "Directory creating failed(%2d)\n", DATA_FOLDER, 0);
            }
            sprintf(TempInfo, "%s文件目录创建失败!!\n", EP_MMIEVT_RPT_DIR);
            LOG_Write(LOG_KERNEL, TempInfo, NULL);
            bMmiEvtDirExistFlag_g = FALSE;
            bInitSuccess = FALSE;
        }
    }

    if (!FT_Is_Dir(EP_WAVE_REC_DIR))
    {
        vxsts = mkdir(EP_WAVE_REC_DIR);
        if (vxsts != OK)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "文件目录创建失败(%2d)\n", DATA_FOLDER, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "Directory creating failed(%2d)\n", DATA_FOLDER, 0);
            }
            sprintf(TempInfo, "%s文件目录创建失败!!\n", EP_WAVE_REC_DIR);
            LOG_Write(LOG_KERNEL, TempInfo, NULL);
            bRecDirExistFlag_g = FALSE;
            bInitSuccess = FALSE;
        }
    }

    if (!FT_Is_Dir(EP_TMP_FILE_DIR))
    {
        vxsts = mkdir(EP_TMP_FILE_DIR);
        if (vxsts != OK)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "文件目录创建失败(%2d)\n", DATA_FOLDER, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "Directory creating failed(%2d)\n", DATA_FOLDER, 0);
            }
            sprintf(TempInfo, "%s文件目录创建失败!!\n", EP_TMP_FILE_DIR);
            LOG_Write(LOG_KERNEL, TempInfo, NULL);
            bInitSuccess = FALSE;
        }
    }

    if (!FT_Is_Dir(EP_REV_TM_DIR))
    {
        vxsts = mkdir(EP_REV_TM_DIR);
        if (vxsts != OK)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "文件目录创建失败(%2d)\n", DATA_FOLDER, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "Directory creating failed(%2d)\n", DATA_FOLDER, 0);
            }
            sprintf(TempInfo, "%s文件目录创建失败!!\n", EP_REV_TM_DIR);
            LOG_Write(LOG_KERNEL, TempInfo, NULL);
            bInitSuccess = FALSE;
        }
    }

    vxsts = FT_Del_Tree(EP_TMP_FILE_DIR);
    if (vxsts != OK)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "文件目录删除失败(%2d)\n", DATA_FOLDER, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "Directory deleting failed(%2d)\n", DATA_FOLDER, 0);
        }
        sprintf(TempInfo, "%s文件目录删除失败!!\n", EP_TMP_FILE_DIR);
        LOG_Write(LOG_KERNEL, TempInfo, NULL);
        bInitSuccess = FALSE;
    }
    vxsts = mkdir(EP_TMP_FILE_DIR);
    if (vxsts != OK)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "文件目录创建失败(%2d)\n", DATA_FOLDER, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_STORAGE_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                       "Directory creating failed(%2d)\n", DATA_FOLDER, 0);
        }
        sprintf(TempInfo, "%s文件目录创建失败!!\n", EP_TMP_FILE_DIR);
        LOG_Write(LOG_KERNEL, TempInfo, NULL);
        bInitSuccess = FALSE;
    }

    for (i=0; i<sizeof(apucChkDir)/sizeof(apucChkDir[0]); i++)
    {
        strncpy(aucFileName, apucChkDir[i], DIR_NAME_LEN);
        aucFileName[DIR_NAME_LEN] = '\0';

        pdir = opendir(aucFileName);
        if (pdir)
        {
            puc = aucFileName+strlen(aucFileName);

            *puc++='/';

            while ((pent = readdir(pdir)) != NULL)
            {
                assert (strlen(pent->d_name) <= FILE_NAME_LEN);

                p=pent->d_name+strlen(pent->d_name)-4;

                if (strcmp(p,".bak")==0)
                {
                    strncpy(puc, pent->d_name, FILE_NAME_LEN);
                    aucFileName[FULL_NAME_LEN] = '\0';

                    if (FT_Is_File(aucFileName))
                    {
                        vxsts=remove(aucFileName);
                    }
                }
                else if (strcmp(p,".old")==0)
                {
                    strncpy(puc, pent->d_name, FILE_NAME_LEN);
                    aucFileName[FULL_NAME_LEN] = '\0';

                    if (FT_Is_File(aucFileName))
                    {
                        vxsts = remove(aucFileName);
                    }
                }
            }

            vxsts = closedir(pdir);
            assert (vxsts == OK);
        }
    }

    if (bInitSuccess)
    {
        return EP_SUCCESS;
    }
    else
    {
        return EP_NOT_INIT;
    }
}

/* Check if file exists.
 * Parameters:
 *      strName, file name(full name with directory message).
 * Return value:
 *      TRUE, strName is a real file.
 *      FALSE, strName is not a file. */
BOOL FT_Is_File(const uint8_t *strName)
{
    struct stat statEnt;

    assert(strName);

    if (stat((uint8_t*)strName, &statEnt)==OK &&
            (statEnt.st_mode & S_IFMT)==S_IFREG)
        return TRUE;
    else
        return FALSE;
}

/* DQ 2007-12-29
 * change system config item to the file string pointered by strFile
 * Parameters:
 *      strFile, string of file to be changed
 *      strHeader, field name, include '[' and ']'.
 *      strItems, string of items name.
 *      strVals, string of items value to set.
 * Return value:
 *      EP_SUCCESS, OK.
 *      EP_ERROR, operating failure.
 * Alert:
 *      1. Maximal length of each name/value is INI_TAG_LEN.
*/
static EP_STATUS FT_Wr_Sys_INI_Item(const uint8_t *strFile, const uint8_t *strHeader,
                                    const uint8_t *strItems, const uint8_t *strVals)
{
    uint8_t aucWork[INI_LINE_LEN+1]="";
    uint8_t aucTemp[INI_LINE_LEN+1]="";
    uint8_t *pucFile;
    int i,j;
    int iFdRd;
    BOOL bDone;
    EP_STATUS sts=EP_ERROR;

    assert(strFile!=NULL);
    pucFile= (uint8_t *)strFile;

    assert(strHeader && strHeader[0]=='[' && strHeader[strlen(strHeader)-1]==']');

    iFdRd=open(EP_SYS_INI_FILE, O_RDONLY, 0);
    assert(iFdRd>=0);

    bDone=FALSE;
    do
    {
        i=FT_Rd_Line_Mem(pucFile, aucWork);
        pucFile+=i;

        if (!strcmp(strHeader, FT_Fmt_Str(aucWork)))
        {
            /* Begin update work. */
            do
            {
                i=FT_Rd_Line_Mem(pucFile, aucWork);
                j=FT_Find_Item_Mem(aucWork);
                assert(j<INI_LINE_LEN);
                aucWork[j]='\0';
                if (!strcmp(strItems, FT_Fmt_Str(aucWork)))
                {
                    sprintf(aucTemp, "%s=%s\n", strItems, strVals);
                    j=strlen(aucTemp)-i;
                    if(strlen(strFile)+j>=2048)
                        assert(0);
                    if(j<0)
                        memmove(pucFile+j+i-1,pucFile+i-1,strlen(pucFile));
                    else
                        memmove(pucFile+j,pucFile,strlen(pucFile));
                    strncpy(pucFile,aucTemp,strlen(aucTemp)-1);
                    pucFile+=j;
                    sts=EP_SUCCESS;
                }
                else if (i==0)
                {
                    bDone=TRUE;
                }
                pucFile+=i;
            }
            while (!bDone);
        }
    }
    while (i!=0);

    close(iFdRd);

    return sts;
}

/*DQ 2007-12-24
根据aucDftIni,创建最新格式sys.ini文件，同时将
旧EP_SYS_INI_FILE文件中的内容继承到新的EP_SYS_INI_FILE文件中
*/
void FT_New_SYS_INI_File(void)
{
    uint8_t aucBuf[FT_VER_INFO_LEN+1];
    uint8_t aucFile[2048]="";
    int iFdWr;
    EP_STATUS sts=EP_SUCCESS;
    STATUS vxsts;
    uint8_t aucTemp[32];
    int i;
    int j;


    strncpy(aucFile, aucDftIni, sizeof(aucDftIni));

    if ((i=FT_Rd_Sys_INI("[HARDWARE]", "HwSN", aucBuf, 32))==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[HARDWARE]", "HwSN", aucBuf );
    assert(sts==EP_SUCCESS);
    if ((i=FT_Rd_Sys_INI("[HARDWARE]", "RegCode", aucBuf, 32))==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[HARDWARE]", "RegCode", aucBuf );
    assert(sts==EP_SUCCESS);
    if ((i=FT_Rd_Sys_INI("[SYSTEM]", "HwCfgVer", aucBuf, FT_VER_INFO_LEN+1))==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "HwCfgVer", aucBuf );
    assert(sts==EP_SUCCESS);
    if ((i=FT_Rd_Sys_INI("[SYSTEM]", "SwCfgVer", aucBuf, FT_VER_INFO_LEN+1))==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "SwCfgVer", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "LgcCfgVer", aucBuf, FT_VER_INFO_LEN+1)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "LgcCfgVer", aucBuf );
    assert(sts==EP_SUCCESS);

    /* 选配功能文件 */
    if ((i = FT_Rd_Sys_INI("[SYSTEM]", "FuncOptVer", aucBuf, FT_VER_INFO_LEN+1)) == 1)
        sts = FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "FuncOptVer", aucBuf );
    assert(sts == EP_SUCCESS);

    if (FT_Rd_Sys_INI("[SYSTEM]", "VxCfgVer", aucBuf, FT_VER_INFO_LEN+1)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "VxCfgVer", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "BootromVer", aucBuf, FT_VER_INFO_LEN+1)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "BootromVer", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "SysCfgVer", aucBuf, FT_VER_INFO_LEN+1)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "SysCfgVer", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "AppCfgVer", aucBuf, FT_VER_INFO_LEN+1)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "AppCfgVer", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "UsbCfgVer", aucBuf, FT_VER_INFO_LEN+1)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "UsbCfgVer", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "MmiCfgVer", aucBuf, FT_VER_INFO_LEN+1)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "MmiCfgVer", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "AutoexecCfgVer", aucBuf, FT_VER_INFO_LEN+1)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "AutoexecCfgVer", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "LogNum", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "LogNum", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "EvtNum", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "EvtNum", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "RecNum", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "RecNum", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "SetArea", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "SetArea", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "NeedHdCof", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "NeedHdCof", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "NeedCtRatio", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "NeedCtRatio", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "NeedClCof", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "NeedClCof", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "Language", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "Language", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "PwrFreq", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "PwrFreq", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "AcMdType", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "AcMdType", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "AcMdTypeChgFlag", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "AcMdTypeChgFlag", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[SYSTEM]", "NewestSN", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[SYSTEM]", "NewestSN", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[NET]", "Net1IP", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[NET]", "Net1IP", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[NET]", "Net2IP", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[NET]", "Net2IP", aucBuf );
    assert(sts==EP_SUCCESS);
#if defined(EDP03_BUILD)
    if (FT_Rd_Sys_INI("[NET]", "Net3IP", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[NET]", "Net3IP", aucBuf );
    assert(sts==EP_SUCCESS);
#endif
    if (FT_Rd_Sys_INI("[NET]", "Net1IPMask", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[NET]", "Net1IPMask", aucBuf );
    assert(sts==EP_SUCCESS);
    if (FT_Rd_Sys_INI("[NET]", "Net2IPMask", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[NET]", "Net2IPMask", aucBuf );
    assert(sts==EP_SUCCESS);
#if defined(EDP03_BUILD)
    if (FT_Rd_Sys_INI("[NET]", "Net3IPMask", aucBuf, 32)==1)
        sts=FT_Wr_Sys_INI_Item(aucFile, "[NET]", "Net3IPMask", aucBuf );
    assert(sts==EP_SUCCESS);
#endif
    i=FT_Rd_Sys_INI("[CRC]","FUNC",aucBuf,32);
    if(i==1)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","FUNC",aucBuf);
    else if(i==0)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","FUNC","");
    assert(sts==EP_SUCCESS);

    i = FT_Rd_Sys_INI("[CRC]", CRC_ITEM_FUN_STS_WR_STS, aucBuf, 32);
    if (i == 1)
        sts = FT_Wr_Sys_INI_Item(aucFile, "[CRC]", CRC_ITEM_FUN_STS_WR_STS, aucBuf);
    else if (i == 0)
        sts = FT_Wr_Sys_INI_Item(aucFile, "[CRC]", CRC_ITEM_FUN_STS_WR_STS, "");

    i=FT_Rd_Sys_INI("[CRC]","DIFORCE",aucBuf,32);
    if(i==1)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","DIFORCE",aucBuf);
    else if(i==0)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","DIFORCE","");
    assert(sts==EP_SUCCESS);

    i=FT_Rd_Sys_INI("[CRC]","LINKSTATS",aucBuf,32);
    if(i==1)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","LINKSTATS",aucBuf);
    else if(i==0)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","LINKSTATS","");
    assert(sts==EP_SUCCESS);

    i = FT_Rd_Sys_INI("[CRC]", CRC_ITEM_LINKSTAT_WR_STS, aucBuf, 32);
    if (i == 1)
        sts = FT_Wr_Sys_INI_Item(aucFile, "[CRC]", CRC_ITEM_LINKSTAT_WR_STS, aucBuf);
    else if (i == 0)
        sts = FT_Wr_Sys_INI_Item(aucFile, "[CRC]", CRC_ITEM_LINKSTAT_WR_STS, "");
    assert (sts == EP_SUCCESS);

    i=FT_Rd_Sys_INI("[CRC]","LINKMODE",aucBuf,32);
    if(i==1)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","LINKMODE",aucBuf);
    else if(i==0)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","LINKMODE","");
    assert(sts==EP_SUCCESS);

    i=FT_Rd_Sys_INI("[CRC]","NBSET",aucBuf,32);
    if(i==1)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","NBSET",aucBuf);
    else if(i==0)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","NBSET","");
    assert(sts==EP_SUCCESS);

    i=FT_Rd_Sys_INI("[CRC]","CKSET",aucBuf,32);
    if(i==1)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","CKSET",aucBuf);
    else if(i==0)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","CKSET","");

    assert(sts==EP_SUCCESS);

    i = FT_Rd_Sys_INI("[CRC]", CRC_ITEM_CKSET_WR_STS, aucBuf, 32);
    if (i == 1)
        sts = FT_Wr_Sys_INI_Item(aucFile, "[CRC]", CRC_ITEM_CKSET_WR_STS, aucBuf);
    else if (i == 0)
        sts = FT_Wr_Sys_INI_Item(aucFile, "[CRC]", CRC_ITEM_CKSET_WR_STS, "");

    assert(sts==EP_SUCCESS);

    i=FT_Rd_Sys_INI("[CRC]","HDCOF",aucBuf,32);
    if(i==1)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","HDCOF",aucBuf);
    else if(i==0)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","HDCOF","");
    assert(sts==EP_SUCCESS);

    i=FT_Rd_Sys_INI("[CRC]","CLCOF",aucBuf,32);
    if(i==1)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","CLCOF",aucBuf);
    else if(i==0)
        sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]","CLCOF","");
    assert(sts==EP_SUCCESS);

    for( i=0; i<32; i++)
    {
        sprintf(aucTemp,"%s%d","AREA",i);
        j=FT_Rd_Sys_INI("[CRC]",aucTemp,aucBuf,32);
        if(j==1)
        {
            sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]",aucTemp,aucBuf);
            assert(sts==EP_SUCCESS);
        }
        else if(j==0)
        {
            sts=FT_Wr_Sys_INI_Item(aucFile,"[CRC]",aucTemp,"");
            assert(sts==EP_SUCCESS);
        }

        sprintf(aucTemp, "%s%d", CRC_ITEM_AREA_WR_STS, i);
        j = FT_Rd_Sys_INI("[CRC]", aucTemp, aucBuf, 32);
        if (j == 1)
        {
            sts = FT_Wr_Sys_INI_Item(aucFile, "[CRC]", aucTemp, aucBuf);
            assert (sts == EP_SUCCESS);
        }
        else if (j == 0)
        {
            sts = FT_Wr_Sys_INI_Item(aucFile, "[CRC]", aucTemp, "");
            assert (sts == EP_SUCCESS);
        }
    }

    iFdWr=FT_Bgn_Update(EP_SYS_INI_FILE);
    assert(iFdWr>0);

    write(iFdWr,aucFile,strlen(aucFile));

    /* 原为5s */
    vxsts = semTake(semSysIni_g, WAIT_FOREVER);
    assert(vxsts==OK);

    sts=FT_End_Update(EP_SYS_INI_FILE, iFdWr);
    assert(sts==EP_SUCCESS);

    vxsts=semGive(semSysIni_g);
    assert(vxsts==OK);

    /*对新形成的edp01.ini文件,做一个备份edp01.ini.cpy,防止坏掉时,能恢复,ZY 2011-6-30   */
    FT_Cpy_Sys_Ini_File();


    if(ENG_MODE==1)
        LOG_Write(LOG_OPRATE, "Successful create new ini file.\n", NULL);
    else if(ENG_MODE==0)
        LOG_Write(LOG_OPRATE, "成功创建新的系统INI文件.\n", NULL);
}

/* Check if dir exists.
 * Parameters:
 *      strName, directory name(full name).
 * Return value:
 *      TRUE, strName is a real file.
 *      FALSE, strName is not a file. */
BOOL FT_Is_Dir(const uint8_t *strName)
{
    struct stat statEnt;

    assert(strName);

    if (stat((uint8_t*)strName, &statEnt)==OK &&
            (statEnt.st_mode & S_IFMT)==S_IFDIR)
        return TRUE;
    else
        return FALSE;
}

/* Get File Length.
 * Parameters:
 *      strName, directory name(full name).
 * Return value:
 *      int, File Length */
int FT_Get_Len(const uint8_t *strName)
{
    struct stat statEnt;

    assert(strName);
    if (stat((uint8_t*)strName, &statEnt)==OK)
    {
        return statEnt.st_size;
    }
    else
    {
        return 0;
    }
}
/* Get file verion infomation string.
 * Parameters:
 *      strFile, file name(full name).
 *      pucRslt, save the return string.
 * Return value:
 *      EP_SUCCESS, OK.
 *      EP_ERROR, strFile is not a file. */
EP_STATUS FT_Get_File_Ver(const uint8_t *strFile, uint8_t *pucRslt)
{
    struct stat statEnt;

    assert(strFile);

    /* Check if it is a file first. */
    if (stat((uint8_t*)strFile, &statEnt)==OK &&
            (statEnt.st_mode & S_IFMT)==S_IFREG)
    {
        sprintf(pucRslt, "%08lX-%08lX-%08lX-%08lX", statEnt.st_ino,
                statEnt.st_size, statEnt.st_mtime, FT_File_CRC32(strFile));
        return EP_SUCCESS;
    }
    else
    {
        *pucRslt='\0';
        return EP_ERROR;
    }
}

/* Get configuration file verion infomation string.
 * Parameters:
 *      strFile, file name(full name).
 *      pucRslt, save the return string.
 * Return value:
 *      EP_SUCCESS, OK.
 *      EP_ERROR, strFile is not a file. */
EP_STATUS FT_Get_Cfg_File_Ver(const uint8_t *strFile, uint8_t *pucRslt)
{
    struct stat statEnt;

    assert(strFile);

    /* Check if it is a file first. */
    if (stat((uint8_t*)strFile, &statEnt)==OK &&
            (statEnt.st_mode & S_IFMT)==S_IFREG)
    {
        sprintf(pucRslt, "%08lX-%08lX", statEnt.st_size, FT_File_CRC32(strFile));

        return EP_SUCCESS;
    }
    else
    {
        *pucRslt='\0';
        return EP_ERROR;
    }
}

/* Caculate CRC32 of a file.
 * Parameters:
 *      strFile, file name(full name).
 * Return value:
 *      CRC32 result of the file. */
uint32_t FT_File_CRC32(const uint8_t *strFile)
{
    int iFd;
    uint32_t ulCrc;
    int i;
    uint8_t aucBuf[FILE_BUF_LEN];
    STATUS vxsts;
    uint32_t ulCnt=0;

    iFd=open(strFile, O_RDONLY, 0);
    if (iFd==ERROR)
    {
        LOG_Dbg_Msg("%s\n", (int)strFile, 0, 0, 0, 0, 0);

        //assert(FALSE);/*2013-7-22 ZY */
        return 0;
    }

    ulCrc=0;
    while ((i=read(iFd, aucBuf, FILE_BUF_LEN))>0)
    {
        ulCrc=EP_CRC32(aucBuf, i, ulCrc);
        ulCnt++;
        if((ulCnt%512) == 0)
        {
            /* 每读取1M延迟10ms */
            taskDelay(1);
        }
    }
    vxsts=close(iFd);
    //assert(vxsts!=ERROR);/*2013-7-22 ZY */

    return ulCrc;
}


/* Caculate CRC16 of a file.
 * Parameters:
 *      strFile, file name(full name).
 * Return value:
 *      CRC16 result of the file. */
uint16_t FT_File_CRC16(const uint8_t *strFile)
{
    int iFd;
    uint16_t unCrc;
    int i;
    uint8_t aucBuf[FILE_BUF_LEN];
    STATUS vxsts;
    uint32_t ulCnt=0;

    iFd=open(strFile, O_RDONLY, 0);
    if (iFd==ERROR)
    {
        LOG_Dbg_Msg("打开文件%s 失败\n", (int)strFile, 0, 0, 0, 0, 0);

        //assert(FALSE);/*2013-7-22 ZY */
        return 0;
    }

    unCrc=0;
    while ((i=read(iFd, aucBuf, FILE_BUF_LEN))>0)
    {
        unCrc=EP_CCITT_CRC16(aucBuf, i, unCrc);
        ulCnt++;
        if((ulCnt%512) == 0)
        {
            taskDelay(1);
        }
    }

    vxsts=close(iFd);
    //assert(vxsts!=ERROR);/*2013-7-22 ZY */

    return unCrc;
}

/* Prepair to update/create a file.  (Using temp file to keep data integrity.)
 * Parameters:
 *      strFile, name of file to be updated.
 * Return:
 *      >=0, file descriptor. For using read/write function.
 *      EP_PARM_ERR, the file does not exist.
 *      EP_FILE_ERR, create temp file failure. */
EP_STATUS FT_Bgn_Update(const uint8_t *strFile)
{
    uint8_t aucTempFile[FULL_NAME_LEN+2];   /* Keep one space to insert '#'. */
    int i;

    assert(strFile && strlen(strFile)<FULL_NAME_LEN);

    FT_Temp_Name(aucTempFile, strFile);
    /* TODO: what happens with creat when file already exists?
     * We want to open it on this suituation. */
    i=creat(aucTempFile, O_RDWR);
    if (i!=ERROR)
        return i;
    else
        return EP_FILE_ERR;
}

/* Finish updating/creating file.  Temp file is cleaned normally.
 * Parameters:
 *      strFile, name of file to be updated.
 *      iFd, file descriptor returned by FT_Bgn_Update.
 * Return:
 *      EP_SUCCESS, update OK.
 *      EP_ERROR, operating failure. */
EP_STATUS FT_End_Update(const uint8_t *strFile, int iFd)
{
    uint8_t aucTempFile[FULL_NAME_LEN+2];   /* Keep one space to insert '#'. */
    STATUS vxsts;

    //assert(iFd>=0);
    if(!(iFd>=0))
    {
        /*2013-7-22 ZY */
        return  	EP_FILE_ERR;
    }

    vxsts=close(iFd);
    //assert(vxsts!=ERROR);/*2013-7-22 ZY */

    FT_Temp_Name(aucTempFile, strFile);

    if (FT_Is_File(strFile))
    {
        vxsts=remove(strFile);
        //assert(vxsts==OK);/*2013-7-22 ZY */
    }

    vxsts=rename(aucTempFile, strFile);
    //assert(vxsts==OK);/*2013-7-22 ZY */

    return EP_SUCCESS;
}

/* Abort updating/creating file.  Temp file is cleaned.
 * Parameters:
 *      strFile, name of file to be updated.
 *      iFd, file descriptor returned by FT_Bgn_Update.
 * Return:
 *      EP_SUCCESS, abort OK.
 *      EP_ERROR, operating failure. */
EP_STATUS FT_Abort_Update(const uint8_t *strFile, int iFd)
{
    uint8_t aucTempFile[FULL_NAME_LEN+2];   /* Keep one space to insert '#'. */
    STATUS vxsts;

    //assert(iFd>=0);/*2013-7-22 ZY */
    if(!(iFd>=0))
    {
        return  	EP_FILE_ERR;
    }

    vxsts=close(iFd);
    //assert(vxsts!=ERROR);/*2013-7-22 ZY */

    FT_Temp_Name(aucTempFile, strFile);

    vxsts=remove(aucTempFile);
    //assert(vxsts==OK);/*2013-7-22 ZY */

    return EP_SUCCESS;
}

static void FT_Temp_Name(uint8_t *pucRslt, const uint8_t *strFile)
{

    strcpy(pucRslt, strFile );
    strcat(pucRslt, ".bkf");

}

/* Write system config item. (Create it when not existing)
 * Parameters:
 *      strHeader, field name, include '[' and ']'.
 *      strItems, string of items name.
 *      strVals, string of items value to set.
 * Return value:
 *      >0, number of new added items.
 *      =0, update all items success.
 *      EP_ERROR, operating failure.
 * Alert:
 *      1. Maximal length of each name/value is INI_TAG_LEN.
 *      2. This function support updating multi-items in same field one time.
 *         '\n' is used in strItems and strVals to seprate items.
 *      3. This function guarantees integrity of multi-items updating. */
EP_STATUS FT_Wr_Sys_INI(const uint8_t *strHeader,
                        const uint8_t *strItems, const uint8_t *strVals)
{
    int iNewAdd;

    iNewAdd=FT_Wr_INI(EP_SYS_INI_FILE, semSysIni_g, strHeader, strItems, strVals);
    /*对新形成的edp01.ini文件,做一个备份edp01.ini.cpy,防止坏掉时,能恢复,ZY 2011-6-30   */
    if(iNewAdd>=0)
    {
        FT_Cpy_Sys_Ini_File();
    }
    return   iNewAdd;
}

/* 写附加系统信息配置文件.
 * Para:
 *     strHeader, 头部信息.
 *     strItems, 数据项.
 *     strVals, 设定值.
 * Return:
 *     >0, number of new added items.
 *     =0, update all items success.
 *     EP_ERROR, operating failure.
 */
EP_STATUS FT_Wr_Aux_Sys_INI(const uint8_t *strHeader, const uint8_t *strItems, const uint8_t *strVals)
{
    int iNewAdd;

    iNewAdd = FT_Wr_INI(EP_AUX_SYS_CFG_FILE, semAucSysIni_g, strHeader, strItems, strVals);

    /* 对新形成的auxedp.ini文件, 做一个备份auxedp.ini.cpy, 防止坏掉时, 能恢复 */
    if (iNewAdd >= 0)
    {
        FT_Cpy_Aux_Sys_Ini_File();
    }

    return iNewAdd;
}

static const uint8_t *FT_Copy_INI_Tag(uint8_t *pucD, const uint8_t *pucS)
{
    uint8_t *puc;

    for (puc=pucD; puc<pucD+INI_TAG_LEN; )
    {
        if (*pucS=='\n' || *pucS=='\0')
            break;
        else
            *puc++=*pucS++;
    }
    *puc='\0';

    puc=FT_Fmt_Str(pucD);
    memmove(pucD, puc, strlen(puc)+1);

    //assert(*pucS=='\n' || *pucS=='\0'); /* The item length<=INI_TAG_LEN. */
    /*2013-8-16 ZY */
    if(!(*pucS=='\n' || *pucS=='\0'))
    {
        return NULL;
    }

    if (*pucS=='\n')
        pucS++;                         /* Point to the begin of next item. */

    return pucS;
}

static FT_INI_ELEM *FT_Find_Item(LIST *plist,
                                 uint8_t *pucFlItem, uint8_t **ppucVal)
{
    FT_INI_ELEM *pini;
    uint8_t *puc;

    assert(plist);

    /* Search '=' to seprate item and value. */
    for (puc=pucFlItem; *puc!='\0'; puc++)
    {
        if (*puc=='=')
            break;
    }

    pini=NULL;

    if (*puc=='=')
        *puc++='\0';

    *ppucVal=puc;                       /* Point to '\0' or next to '='. */

    puc=FT_Fmt_Str(pucFlItem);
    if (*puc=='[')
        *ppucVal=NULL;
    else if (*puc)
    {
        /* Compare item with all inputs. */
        for (pini=(FT_INI_ELEM*)lstFirst(plist); pini;
                pini=(FT_INI_ELEM*)lstNext((NODE*)pini))
        {
            if (!strcmp(pini->aucItem, puc))
                break;
        }
    }

    return pini;
}

/* Read system config item.
 * Parameters:
 *      strHeader, field name, include '[' and ']'.
 *      strItems, string of items name.
 *      strVals, space to save string of items value when return.
 *      iBufLen, bytes of strVals can be written.
 * Return value:
 *      >0, number of success read items. Maybe less than items in strItems
 *          for items not found or buffer overflow.
 *      =0, header was found, but no item matches.
 *      EP_ERROR, operating failure.
 * Alert:
 *      1. Maximal length of each name/value is INI_TAG_LEN.
 *      2. This function support updating multi-items in same field one time.
 *         '\n' is used in strItems and strVals to seprate items. */
int FT_Rd_Sys_INI(const uint8_t *strHeader,
                  const uint8_t *strItems, uint8_t *strVals, int iBufLen)
{
    (EP_SYS_INI_FILE, semSysIni_g, strHeader, strItems, strVals, iBufLen);
}

/* 读附加系统信息配置文件.
 * Para:
 *     strHeader, 头部信息.
 *     strItems, 数据项.
 *     strVals, 读取值.
 *     iBufLen, 数据长度.
 * Return:
 *     >0, number of success read items. Maybe less than items in strItems
 *         for items not found or buffer overflow.
 *     =0, header was found, but no item matches.
 *     EP_ERROR, operating failure.
 */
int FT_Rd_Aux_Sys_INI(const uint8_t *strHeader, const uint8_t *strItems, uint8_t *strVals, int iBufLen)
{
    return FT_Rd_INI(EP_AUX_SYS_CFG_FILE, semAucSysIni_g, strHeader, strItems, strVals, iBufLen);
}

/* Read system version.ini item.
 * Parameters:
 *      strHeader, field name, include '[' and ']'.
 *      strItems, string of items name.
 *      strVals, space to save string of items value when return.
 *      iBufLen, bytes of strVals can be written.
 * Return value:
 *      >0, number of success read items. Maybe less than items in strItems
 *          for items not found or buffer overflow.
 *      =0, header was found, but no item matches.
 *      EP_ERROR, operating failure.
 * Alert:
 *      1. Maximal length of each name/value is INI_TAG_LEN.
 *      2. This function support updating multi-items in same field one time.
 *         '\n' is used in strItems and strVals to seprate items.
 *      3. 没有对文件描述符加保护处理，确保该函数和FT_Wr_Version_INI()不能同时调用
 */
int FT_Rd_Version_INI(const uint8_t *strHeader,
                      const uint8_t *strItems, uint8_t *strVals, int iBufLen)
{
    return FT_Rd_INI(EP_UN_VERSION_FILE, NULL, strHeader, strItems, strVals, iBufLen);
}

/* Write system ini item. (Create it when not existing)
 * Parameters:
 *      strHeader, field name, include '[' and ']'.
 *      strItems, string of items name.
 *      strVals, string of items value to set.
 * Return value:
 *      >0, number of new added items.
 *      =0, update all items success.
 *      EP_ERROR, operating failure.
 * Alert:
 *      1. Maximal length of each name/value is INI_TAG_LEN.
 *      2. This function support updating multi-items in same field one time.
 *         '\n' is used in strItems and strVals to seprate items.
 *      3. This function guarantees integrity of multi-items updating.
 *      4. 没有对文件描述符加保护处理，确保该函数和FT_Rd_Version_INI()不能同时调用
 */
EP_STATUS FT_Wr_Version_INI(const uint8_t *strHeader,const uint8_t *strItems, const uint8_t *strVals)
{
    return FT_Wr_INI(EP_UN_VERSION_FILE, NULL, strHeader, strItems, strVals);
}



static int FT_Find_Item_Mem(uint8_t *ppucVal)
{
    uint8_t *puc;

    /* Search '=' to seprate item and value. */
    for (puc=ppucVal; *puc!='\0'; puc++)
    {
        if (*puc=='=')
            break;
    }

    if (*puc=='=')
    {
//        *puc='\0';
        return puc-ppucVal;
    }
    else
    {
        return 0;
    }

}



static EP_STATUS FT_Rd_Line(int iFd, uint8_t *pucLine)
{
    uint8_t *puc;
    int i;
    int iPos;

    iPos=lseek(iFd, 0, SEEK_CUR);

    i=read(iFd, pucLine, INI_LINE_LEN);
    //assert(i!=ERROR);/*2013-7-22 ZY */

    if (i<=0)
    {
        *pucLine='\0';
        return EP_CLOSED;
    }

    for (puc=pucLine; puc<pucLine+i; )
    {
        if (*puc++=='\n')
        {
            *puc='\0';
            /* Seek to the begin of new line for next call. */
            lseek(iFd, iPos+(puc-pucLine), SEEK_SET);
            return EP_SUCCESS;
        }
    }
    *puc='\0';

    if (i<INI_LINE_LEN)                 /* Reach file end. */
        return EP_SUCCESS;

    /* Line too long. */
    //assert(FALSE);/*2013-7-22 ZY */
    return EP_FILE_ERR;
}


static int FT_Rd_Line_Mem(uint8_t *pucFile, uint8_t *pucLine)
{
    uint8_t *puc;
    int i;

    i=strlen(pucFile);
    if(i<=0)
    {
        return 0;
    }

    for (i=0,puc=pucLine; ; i++)
    {
        if (i>=INI_LINE_LEN)
        {
            //assert(0);
            return 0;
        }
        *puc=*pucFile;
        if (*pucFile=='\n')
        {
            puc++;
            *puc='\0';
            return i+1;
        }
        if (*pucFile=='\0')
        {
            *puc='\0';
            return i;
        }
        puc++;
        pucFile++;
    }

    return 0;
}


static uint8_t *FT_Fmt_Str(uint8_t *strOrg)
{
    uint8_t *puc;

    assert(strOrg);

    /* Deal with line comment first(begins with '#'). */
    for (puc=strOrg; *puc; puc++)
    {
        if (*puc=='#')
        {
            *puc='\0';                  /* Discard the comment. */
            break;
        }
    }

    /* Discard space on the tail. */
    for (puc=strOrg+strlen(strOrg)-1; puc>=strOrg; puc--)
    {
        if (!isspace(*puc))
        {
            *(puc+1)='\0';
            break;
        }
    }

    /* Skip space on the head. */
    for (puc=strOrg; *puc; puc++)
    {
        if (!isspace(*puc))
            break;
    }

    return puc;
}

#if TSFS_DBG
int rename(const char *oldname, const char *newname)
{
    int iFdWr;
    int iFdRd;
    int i;
    uint8_t aucBuf[INI_LINE_LEN];
    STATUS vxsts;

    assert(oldname && newname);

    iFdWr=creat(newname, O_WRONLY);
    if (iFdWr==ERROR)
        return ERROR;

    iFdRd=open(oldname, O_RDONLY, 0);
    if (iFdRd==ERROR)
    {
        close(iFdWr);
        return ERROR;
    }

    while ((i=read(iFdRd, aucBuf, INI_LINE_LEN))>0)
        write(iFdWr, aucBuf, i);

    close(iFdWr);
    close(iFdRd);

    vxsts=remove(oldname);
    assert(vxsts==OK);

    return OK;
}
#endif

/* 功能: 复制文件
 * 参数: oldname, 源文件路径;
 *       newname, 目标文件路径;
 * 返回: OK, 成功; ERROR, 出错;
 */
STATUS FT_Cpy_File(const char *oldname, const char *newname)
{
    int iFdWr;
    int iFdRd;
    int i;
    uint8_t aucBuf[INI_LINE_LEN];

    assert(oldname && newname);

    iFdWr=creat(newname, O_WRONLY);
    if (iFdWr==ERROR)
        return ERROR;

    iFdRd=open(oldname, O_RDONLY, 0);
    if (iFdRd==ERROR)
    {
        close(iFdWr);
        return ERROR;
    }

    while ((i=read(iFdRd, aucBuf, INI_LINE_LEN))>0)
        write(iFdWr, aucBuf, i);

    close(iFdWr);
    close(iFdRd);

    return OK;
}

/* Delete a directory tree.
 * Parameters:
 *      strDir, name of directory to be deleted.
 * Return value:
 *      OK, success.
 *      ERROR, strDir is not a valid directory.
 * Alert:
 *      This function uses recursion to implement. If the directory tree is
 *      very deep, the function may need quite more stack space. */
STATUS FT_Del_Tree(const uint8_t *strDir)
{
    DIR *pdir;
    struct dirent *pent;
    uint8_t aucFileName[FULL_NAME_LEN+1];
    uint8_t *puc;
    STATUS vxsts;

    pdir=opendir((char*)strDir);
    if (pdir)
    {
        strncpy(aucFileName, strDir, DIR_NAME_LEN);
        aucFileName[DIR_NAME_LEN]='\0';

        puc=aucFileName+strlen(aucFileName);

        *puc++='/';                     /* Add '/' to connect dir and file names. */

        while ((pent=readdir(pdir))!=NULL)
        {
            assert(strlen(pent->d_name)<=FILE_NAME_LEN);

            strncpy(puc, pent->d_name, FILE_NAME_LEN);
            aucFileName[FULL_NAME_LEN]='\0';

            if (pent->d_name[0]!='.' || (pent->d_name[1]!='\0' &&
                                         (pent->d_name[1]!='.' || pent->d_name[2]!='\0')))
            {
                if (FT_Is_File(aucFileName))
                {
                    vxsts=remove(aucFileName);
                }
                else
                {
                    vxsts=FT_Del_Tree(aucFileName);
                }
            }
        }

        vxsts=closedir(pdir);
        //assert(vxsts==OK);/*2013-7-22 ZY */

        vxsts=rmdir((char*)strDir);

        return OK;
    }
    else
        return ERROR;
}

/* Get file length from file descriptor.
 * Parameters:
 *      iFd, file descriptor.
 * Return valud:
 *      Length of the file. */
uint32_t FT_File_Len(int iFd)
{
    /*2013-7-20  ZY，允许失败，不能assert ,并且有BUG*/
    uint32_t ulOrgPos;
    uint32_t ulLen;

    //assert(iFd>=0);
    if(!(iFd>=0))
    {
        return 0;
    }

    ulOrgPos=lseek(iFd, 0, SEEK_CUR);
    //assert(ulOrgPos!=ERROR);
    if(ulOrgPos==ERROR)
    {
        return  0;
    }

    ulLen=lseek(iFd, 0, SEEK_END);
    //assert(ulOrgPos!=ERROR);/*原来代码BUG */
    if(ulLen==ERROR)
    {
        return  0;
    }

    ulOrgPos=lseek(iFd, ulOrgPos, SEEK_SET);
    //assert(ulOrgPos!=ERROR);
    if(ulOrgPos==ERROR)
    {
        return  0;
    }
    return ulLen;
}

/* Read file to memory buffer.
 * Parameters:
 *      strFile, name of file to be read.
 *      pulLen, to save file length when return.
 * Return value:
 *      Pointer to the buffer contains all the file.  The '\0' is added to the
 *      end of buffer. NULL means can't open file or file too long.
 * Alert:
 *      The return pointer is allocated with malloc function, it should be
 *      deallocated by free after use. */
uint8_t *FT_File_To_Mem(const uint8_t *strFile, uint32_t *pulLen)
{
    /*2013-7-20  ZY，允许失败，不能assert */
    uint8_t *pucBuf;
    int iFd;
    int i;
    STATUS vxsts;

    if(!(strFile && pulLen))
    {
        return  NULL;
    }

    pucBuf=NULL;
    *pulLen=0;
    if ((iFd=open(strFile, O_RDONLY, 0))!=ERROR)
    {
        *pulLen=FT_File_Len(iFd);

        if ((pucBuf=malloc(*pulLen+1))!=NULL)
        {
            i=read(iFd, pucBuf, *pulLen);
            //assert(i==*pulLen);
            if(i==*pulLen)
            {
                pucBuf[*pulLen]='\0';
            }
            else
            {
                //注意释放资源，ZY
                EP_free(pucBuf);
                vxsts=close(iFd);
                return  NULL;
            }

        }

        vxsts=close(iFd);
        //assert(vxsts==OK);
    }

    return pucBuf;
}

/* Print text file to shell.
 * Parameters:
 *      strFile, name of file to be displayed.
 * Return value:
 *      >0, file length.
 *      ERROR, can't open file or file too long. */
STATUS FT_Print_File(const uint8_t *strFile)
{
    uint8_t *pucBuf;
    uint32_t ulLen;

    assert(strFile);

    pucBuf=FT_File_To_Mem(strFile, &ulLen);

    if (pucBuf && ulLen)
        puts(pucBuf);

    if (pucBuf)
    {
        EP_free(pucBuf);
        return ulLen;
    }
    else
        return ERROR;
}



/* Get file modify time infomation string.
 * Parameters:
 *      strFile, file name(full name).
 *      pucRslt, save the return string.
 * Return value:
 *      EP_SUCCESS, OK.
 *      EP_ERROR, strFile is not a file. */
EP_STATUS FT_Get_File_modify_time(const uint8_t *strFile, uint8_t *pucRslt)
{
    struct stat statEnt;

    assert(strFile);

    /* Check if it is a file first. */
    if (stat((uint8_t*)strFile, &statEnt)==OK &&
            (statEnt.st_mode & S_IFMT)==S_IFREG)
    {

        sprintf(pucRslt, "%08lX-%08lX-%08lX", statEnt.st_ino,
                statEnt.st_size, statEnt.st_mtime);

        return EP_SUCCESS;
    }
    else
    {
        *pucRslt='\0';
        return EP_ERROR;
    }
}

//获取两个字符中间的字符串
//参数：	pcStr,待查找的母字串指针
//			lIndex,字串序号（0开始）
//			m_pcItemStr,符合两个字符的中间字符串指针
//			char_B,查找的第一个开始字符
//                  char_E,查找的第二个结束字符
//返回值：	条目字串长度
int GetData_B(uint8_t *pcStr,uint8_t lIndex,uint8_t *m_pcItemStr,uint8_t char_B,uint8_t char_E)
{
    uint8_t ch;
    uint8_t *startptr=m_pcItemStr;
    BOOL ItemFlag=FALSE;

    while(lIndex>0)
    {
        ch=*pcStr++;
        if(ch==char_B)
            ItemFlag=TRUE;
        if(ItemFlag&&ch==char_E)
        {
            lIndex--;
            ItemFlag=FALSE;
        }
        if(ch=='\0')
        {
            *m_pcItemStr='\0';
            return 0;
        }
    }

    while(1)
    {
        ch=*pcStr++;
        if(ch=='\0'||ch==char_B)
            break;
    }
    while(1)
    {
        ch=*pcStr++;
        if(ch=='\0'||ch==char_E)
            break;
        *m_pcItemStr++=ch;
    }

    *m_pcItemStr='\0';
    return m_pcItemStr-startptr;
}


/* Write system item. (Create it when not existing)
 * Parameters:
 *     strFileName, file name.
 *     semID, semaphore ID, NULL if no semaphore.
 *     strHeader, field name, include '[' and ']'.
 *     strItems, string of items name.
 *     strVals, string of items value to set.
 * Return value:
 *     >0, number of new added items.
 *     =0, update all items success.
 *     EP_ERROR, operating failure.
 * Alert:
 *     1. Maximal length of each name/value is INI_TAG_LEN.
 *     2. This function support updating multi-items in same field one time.
 *        '\n' is used in strItems and strVals to seprate items.
 *     3. This function guarantees integrity of multi-items updating.
 */
EP_STATUS FT_Wr_INI(const uint8_t *strFileName, SEM_ID semID, const uint8_t *strHeader,
                    const uint8_t *strItems, const uint8_t *strVals)
{
    /*2013-7-22 ZY 去掉assert, 特别注意异常时资源的释放*/
    int iFd;
    int iFdWr;
    int iFdRd;
    LIST list;
    FT_INI_ELEM *pini;
    uint8_t aucLine[INI_LINE_LEN+1];
    uint8_t aucWork[INI_LINE_LEN+1];
    uint8_t *pucItem;
    uint8_t *pucVal;
    int i;
    int iNewAdd;
    BOOL bDone;
    STATUS vxsts;
    EP_STATUS sts;

    //assert (strFileName && strHeader && strHeader[0] == '[' && strHeader[strlen(strHeader)-1] == ']');
    if(!(strFileName && strHeader && strHeader[0] == '[' && strHeader[strlen(strHeader)-1] == ']'))
    {
        return  EP_ERROR;
    }

    lstInit(&list);

    for (pucItem = (uint8_t *)strItems, pucVal = (uint8_t *)strVals; *pucItem != '\0'; )
    {
        pini = malloc(sizeof(*pini));
        //assert (pini);
        if(!pini)
        {
            /*需释放链表资源 */
            goto   WR_INI_FAIL1;
        }

        /*2013-8-16 ZY */
        pucItem = (uint8_t *)FT_Copy_INI_Tag(pini->aucItem, pucItem);
        if(!pucItem)
        {
            /*需释放链表资源 */
            free(pini);
            goto   WR_INI_FAIL1;
        }
        pucVal = (uint8_t *)FT_Copy_INI_Tag(pini->aucVal, pucVal);
        if(!pucVal)
        {
            /*需释放链表资源 */
            free(pini);
            goto   WR_INI_FAIL1;
        }

        lstAdd(&list, &pini->node);
    }

    if (semID != NULL)
    {
        vxsts = semTake(semID, WAIT_FOREVER);	//以前5秒有缺陷，不能实现互斥保护。
    }

    if (!FT_Is_File(strFileName))
    {
        //创建新文件
        iFd = FT_Bgn_Update(strFileName);
        //assert (iFd >= 0);
        if(!(iFd >= 0))
        {
            /*需释放链表资源 */
            goto   WR_INI_FAIL2;
        }

        if (!strcmp(strFileName, EP_1588_PROFILE_FILE))
        {
            i = write(iFd, auc1588Ini, sizeof(auc1588Ini));
            //assert (i == sizeof(auc1588Ini));
            if(!(i == sizeof(auc1588Ini)))
            {
                /*需释放链表资源 */
                FT_Abort_Update(strFileName, iFd);
                goto   WR_INI_FAIL2;
            }
        }
        sts = FT_End_Update(strFileName, iFd);
        //assert (sts == EP_SUCCESS);
        if(!(sts == EP_SUCCESS))
        {
            /*需释放链表资源 */
            goto   WR_INI_FAIL2;
        }
    }

    iFdWr = FT_Bgn_Update(strFileName);
    //assert (iFdWr >= 0);
    if(!(iFdWr >= 0))
    {
        /*需释放链表资源 */
        goto   WR_INI_FAIL2;
    }

    iFdRd = open(strFileName, O_RDONLY, 0);
    //assert (iFdRd >= 0);
    if(!(iFdRd >= 0))
    {
        /*需释放链表资源 */
        FT_Abort_Update(strFileName, iFdWr);
        goto   WR_INI_FAIL2;
    }

    iNewAdd = lstCount(&list);
    bDone = FALSE;
    do
    {
        /*按条目搜索，包括HEADER和ITEM */
        sts = FT_Rd_Line(iFdRd, aucLine);

        /* Copy the line first. */
        i = write(iFdWr, aucLine, strlen(aucLine));
        //assert (i == strlen(aucLine));
        if(!(i == strlen(aucLine)))
        {
            /*需释放链表资源 */
            goto   WR_INI_FAIL3;
        }

        if (iNewAdd && !strcmp(strHeader, FT_Fmt_Str(aucLine)))
        {
            /*若是找到同名HEADER，并有新增加，则更新之 */
            /* Begin update work. */
            do
            {
                /*按ITEM搜索 */
                sts = FT_Rd_Line(iFdRd, aucLine);
                strcpy(aucWork, aucLine);

                pini = FT_Find_Item(&list, aucWork, &pucVal);
                if (pini)
                {
                    /*若找到同名的的ITEM，用新值更新 */
                    sprintf(aucLine, "%s=%s\n", pini->aucItem, pini->aucVal);

                    lstDelete(&list, (NODE *)pini);//结束时统一释放资源。ZY，2013-7-22
                    EP_free(pini);

                    if (!--iNewAdd)
                        bDone = TRUE;
                }
                else if (!pucVal || sts==EP_CLOSED)
                {
                    /*若本HEADER读取结束，说明剩余新增加内容都是新条目 */
                    /* Reach next header or end of file. */
                    while ((pini = (FT_INI_ELEM *)lstGet(&list)) != NULL)
                    {
                        /*写入新增加条目所有内容 */
                        i = sprintf(aucWork, "%s=%s\n", pini->aucItem, pini->aucVal);

                        EP_free(pini);//注意，必须在这里释放，因为lstGet中已经删除成员了。ZY，2013-9-12

                        i = write(iFdWr, aucWork, i);
                        //assert (i == strlen(aucWork));
                        if(!(i == strlen(aucWork)))
                        {
                            /*需释放链表资源 */
                            goto   WR_INI_FAIL3;
                        }

                    }
                    bDone = TRUE;
                }
                /*写入已有条目 */
                i = write(iFdWr, aucLine, strlen(aucLine));
                //assert(i == strlen(aucLine));
                if(!(i == strlen(aucLine)))
                {
                    /*需释放链表资源 */
                    goto   WR_INI_FAIL3;
                }
            }
            while (sts == EP_SUCCESS && !bDone);
        }/*if结束 */
    }
    while (sts == EP_SUCCESS);

    if (sts == EP_FILE_ERR)
    {
        //lstFree(&list);//结束时统一释放资源。ZY，2013-7-22
        iNewAdd = EP_ERROR;
    }
    else if (!bDone)
    {
        /*若现有文件，没找到同名的HEADER，则作为新HEADER添加 */
        i = sprintf(aucLine, "\n%s\n", strHeader);

        i = write(iFdWr, aucLine, i);
        //assert (i == strlen(aucLine));
        if(!(i == strlen(aucLine)))
        {
            /*需释放链表资源 */
            goto   WR_INI_FAIL3;
        }

        while ((pini = (FT_INI_ELEM *)lstGet(&list)) != NULL)
        {
            i = sprintf(aucLine, "%s=%s\n", pini->aucItem, pini->aucVal);

            EP_free(pini);//注意，必须在这里释放，因为lstGet中已经删除成员了。ZY，2013-9-12

            i = write(iFdWr, aucLine, i);
            //assert (i == strlen(aucLine));
            if(!(i == strlen(aucLine)))
            {
                /*需释放链表资源 */
                goto   WR_INI_FAIL3;
            }
        }
    }

    /*操作成功，后续资源处理 */
WR_INI_SUCCESS:
    vxsts = close(iFdRd);
    //assert (vxsts == OK);
    sts = FT_End_Update(strFileName, iFdWr);
    //assert (sts == EP_SUCCESS);
    lstFree(&list);/*同时释放所有节点申请内存 */
    if (semID != NULL)
    {
        vxsts = semGive(semID);
    }
    return iNewAdd;

    /*操作失败1，后续资源处理 */
WR_INI_FAIL1:
    lstFree(&list);
    return  	EP_ERROR;

    /*操作失败2，后续资源处理 */
WR_INI_FAIL2:
    lstFree(&list);
    if (semID != NULL)
    {
        vxsts = semGive(semID);
    }
    return  	EP_ERROR;

    /*操作失败3，后续资源处理 */
WR_INI_FAIL3:
    vxsts = close(iFdRd);
    FT_Abort_Update(strFileName, iFdWr);
    lstFree(&list);
    if (semID != NULL)
    {
        vxsts = semGive(semID);
    }
    return  	EP_ERROR;

}




/* Read config item.
 * Parameters:
 *     strFileName, file name.
 *     semID, semaphore ID, NULL if no semaphore.
 *     strHeader, field name, include '[' and ']'.
 *     strItems, string of items name.
 *     strVals, space to save string of items value when return.
 *     iBufLen, bytes of strVals can be written.
 * Return value:
 *     >0, number of success read items. Maybe less than items in strItems
 *      for items not found or buffer overflow.
 *     =0, header was found, but no item matches.
 *     EP_ERROR, operating failure.
 * Alert:
 *     1. Maximal length of each name/value is INI_TAG_LEN.
 *     2. This function support updating multi-items in same field one time.
 *        '\n' is used in strItems and strVals to seprate items.
 */
int FT_Rd_INI(const uint8_t *strFileName, SEM_ID semID, const uint8_t *strHeader,
              const uint8_t *strItems, uint8_t *strVals, int iBufLen)
{
    /*2013-7-22 ZY 去掉assert, 特别注意异常时资源的释放*/
    int iFd;
    LIST list;
    FT_INI_ELEM *pini;
    uint8_t aucLine[INI_LINE_LEN+1];
    uint8_t *pucItem;
    uint8_t *pucVal;
    int i;
    EP_STATUS sts;
    int iRdItems;
    STATUS vxsts;

    //assert (strFileName && strHeader && strHeader[0] == '[' && strHeader[strlen(strHeader)-1] == ']');
    if(!(strFileName && strHeader && strHeader[0] == '[' && strHeader[strlen(strHeader)-1] == ']'))
    {
        return  	EP_ERROR;
    }

    if (semID != NULL)
    {
        vxsts = semTake(semID, WAIT_FOREVER);	//以前5秒有缺陷，不能实现互斥保护。
    }

    iFd = open(strFileName, O_RDONLY, 0);
    if (iFd == ERROR)
    {
        if (semID != NULL)
        {
            vxsts = semGive(semID);
        }
        return EP_ERROR;
    }

    lstInit(&list);

    for (pucItem = (uint8_t *)strItems; *pucItem != '\0'; )
    {
        pini = malloc(sizeof(*pini));
        //assert (pini);
        if(!pini)
        {
            goto  RD_INI_FAIL1;
        }

        pucItem = (uint8_t *)FT_Copy_INI_Tag(pini->aucItem, pucItem);
        if(!pucItem)
        {
            free(pini);
            goto  RD_INI_FAIL1;
        }
        pini->aucVal[0] = '\0';

        lstAdd(&list, &pini->node);
    }

    iRdItems = -1;

    /* Search header first. */
    do
    {
        sts = FT_Rd_Line(iFd, aucLine);
        if (!strcmp(strHeader, FT_Fmt_Str(aucLine)))
        {
            sts = EP_SUCCESS;
            break;
        }
    }
    while (sts == EP_SUCCESS);

    if (sts == EP_SUCCESS)    /* Header was found. */
    {
        iRdItems = 0;

        do
        {
            sts = FT_Rd_Line(iFd, aucLine);

            pini = FT_Find_Item(&list, aucLine, &pucVal);

            if (pini)
            {
                assert(pucVal);
                pucVal = FT_Fmt_Str(pucVal);
                strncpy(pini->aucVal, pucVal, INI_TAG_LEN);
                pini->aucVal[INI_TAG_LEN] = '\0';
            }
            else if (!pucVal)
                break;
        }
        while (sts == EP_SUCCESS);
    }

    iRdItems = 0;
    pucVal = strVals;
    while ((pini = (FT_INI_ELEM *)lstGet(&list)) != NULL)
    {
        i = strlen(pini->aucVal);
        if (!i)
        {
            *pucVal++ = '\n';
            EP_free(pini);
        }
        else if (i<iBufLen)
        {
            memcpy(pucVal, pini->aucVal, i);
            pucVal += i;
            *pucVal++ = '\n';
            iBufLen -= i;
            iRdItems++;
            EP_free(pini);
        }
        else
        {
            EP_free(pini);
            break;
        }
    }
    if (pucVal>strVals)
        *(pucVal-1) = '\0';
    else
        *pucVal = '\0';

    lstFree(&list);

    vxsts = close(iFd);
    //assert (vxsts == OK);

    if (semID != NULL)
    {
        vxsts = semGive(semID);
    }

    return iRdItems;

RD_INI_FAIL1:
    lstFree(&list);
    vxsts = close(iFd);
    if (semID != NULL)
    {
        vxsts = semGive(semID);
    }
    return   EP_ERROR;

}

/*ZQ 2011-02-15
根据aucDftIni,创建最新格式DSamSts.ini文件*/
void FT_New_DSamSts_INI_File(void)
{
    /*2013-7-22 ZY 去掉assert, */

    uint8_t aucFile[1024]="";
    int iFdWr;
    EP_STATUS sts=EP_SUCCESS;



//    sprintf(aucBuf,"%d",MUCnt);
    strncpy(aucFile, DSamStsIni, sizeof(DSamStsIni));

    iFdWr=FT_Bgn_Update(EP_EDP_DSamSts_FILE);
    //assert(iFdWr>0);
    if(!(iFdWr>0))
    {
        return ;
    }

    write(iFdWr,aucFile,strlen(aucFile));
    printf("成功创建新的数字采样状态文件.\n");

    sts=FT_End_Update(EP_EDP_DSamSts_FILE, iFdWr);
    //assert(sts==EP_SUCCESS);
    if(ENG_MODE==1)
        LOG_Write(LOG_OPRATE, "Successful create new DSamSts file.\n", NULL);
    else if(ENG_MODE==0)
        LOG_Write(LOG_OPRATE, "成功创建新的数字采样状态文件.\n", NULL);
}


/* Write system ini item. (Create it when not existing)
 * Parameters:
 *      strHeader, field name, include '[' and ']'.
 *      strItems, string of items name.
 *      strVals, string of items value to set.
 * Return value:
 *      >0, number of new added items.
 *      =0, update all items success.
 *      EP_ERROR, operating failure.
 * Alert:
 *      1. Maximal length of each name/value is INI_TAG_LEN.
 *      2. This function support updating multi-items in same field one time.
 *         '\n' is used in strItems and strVals to seprate items.
 *      3. This function guarantees integrity of multi-items updating.
 *      4. 没有对文件描述符加保护处理，确保该函数和FT_Rd_Version_INI()不能同时调用
 */
void FT_Wr_DSamSts_INI(const uint8_t *strHeader,const uint8_t *strItems, const uint8_t *strVals)
{
    /*2013-7-22 ZY 去掉assert, 特别注意异常时资源的释放*/
    int iFdWr;
    int iFdRd;
    LIST list;
    FT_INI_ELEM *pini;
    uint8_t aucLine[INI_LINE_LEN+1];
    uint8_t aucWork[INI_LINE_LEN+1];
    uint8_t *pucItem;
    uint8_t *pucVal;
    int i;
    int iNewAdd;
    BOOL bDone;
    STATUS vxsts;
    EP_STATUS sts;


    //assert(strHeader && strHeader[0]=='[' && strHeader[strlen(strHeader)-1]==']');
    if(!(strHeader && strHeader[0]=='[' && strHeader[strlen(strHeader)-1]==']'))
    {
        return ;
    }

    lstInit(&list);

    for (pucItem=(uint8_t*)strItems, pucVal=(uint8_t*)strVals; *pucItem!='\0'; )
    {
        pini=malloc(sizeof(*pini));
        //assert(pini);
        if(!pini)
        {
            goto   Wr_DSamSts_INI_FAIL1;
        }

        pucItem=(uint8_t*)FT_Copy_INI_Tag(pini->aucItem, pucItem);
        if(!pucItem)
        {
            free(pini);
            goto   Wr_DSamSts_INI_FAIL1;

        }
        pucVal=(uint8_t*)FT_Copy_INI_Tag(pini->aucVal, pucVal);
        if(!pucVal)
        {
            free(pini);
            goto   Wr_DSamSts_INI_FAIL1;
        }

        lstAdd(&list, &pini->node);
    }

    iFdWr=FT_Bgn_Update(EP_EDP_DSamSts_FILE);
    //assert(iFdWr>=0);
    if(!(iFdWr>=0))
    {
        goto 	Wr_DSamSts_INI_FAIL1;
    }

    iFdRd=open(EP_EDP_DSamSts_FILE, O_RDONLY, 0);
    //assert(iFdRd>=0);
    if(!(iFdRd>=0))
    {
        goto 	Wr_DSamSts_INI_FAIL2;

    }

    iNewAdd=lstCount(&list);
    bDone=FALSE;
    do
    {
        sts=FT_Rd_Line(iFdRd, aucLine);//找到一行

        /* Copy the line first. */
        i=write(iFdWr, aucLine, strlen(aucLine));
        //assert(i==strlen(aucLine));
        if(!(i==strlen(aucLine)))
        {
            goto  	Wr_DSamSts_INI_FAIL3;
        }

        if (iNewAdd && !strcmp(strHeader, FT_Fmt_Str(aucLine)))
        {
            /* Begin update work. */
            do
            {
                sts=FT_Rd_Line(iFdRd, aucLine);
                strcpy(aucWork, aucLine);

                pini=FT_Find_Item(&list, aucWork, &pucVal);
                if (pini)
                {
                    sprintf(aucLine, "%s=%s\n", pini->aucItem, pini->aucVal);

                    lstDelete(&list, (NODE*)pini);

                    EP_free(pini);

                    if (!--iNewAdd)
                        bDone=TRUE;
                }
                else if (!pucVal || sts==EP_CLOSED)
                {
                    /* Reach next header or end of file. */
                    while ((pini=(FT_INI_ELEM*)lstGet(&list))!=NULL)
                    {
                        i=sprintf(aucWork, "%s=%s\n", pini->aucItem, pini->aucVal);

                        EP_free(pini);//注意，必须在这里释放，因为lstGet中已经删除成员了。ZY，2013-9-12

                        i=write(iFdWr, aucWork, i);
                        //assert(i==strlen(aucWork));
                        if(!(i==strlen(aucWork)))
                        {
                            goto  	Wr_DSamSts_INI_FAIL3;
                        }

                    }
                    bDone=TRUE;
                }

                i=write(iFdWr, aucLine, strlen(aucLine));
                //assert(i==strlen(aucLine));
                if(!(i==strlen(aucLine)))
                {
                    goto  	Wr_DSamSts_INI_FAIL3;
                }
            }
            while (sts==EP_SUCCESS && !bDone);
        }
    }
    while (sts==EP_SUCCESS);

    if (sts==EP_FILE_ERR)
    {
        // lstFree(&list);
        iNewAdd=EP_ERROR;
    }
    else if (!bDone)
    {
        i=sprintf(aucLine, "\n%s\n", strHeader);

        i=write(iFdWr, aucLine, i);
        //assert(i==strlen(aucLine));
        if(!(i==strlen(aucLine)))
        {
            goto 	  Wr_DSamSts_INI_FAIL3;
        }

        while ((pini=(FT_INI_ELEM*)lstGet(&list))!=NULL)
        {
            i=sprintf(aucLine, "%s=%s\n", pini->aucItem, pini->aucVal);

            EP_free(pini);//注意，必须在这里释放，因为lstGet中已经删除成员了。ZY，2013-9-12

            i=write(iFdWr, aucLine, i);
            //assert(i==strlen(aucLine));
            if(!(i==strlen(aucLine)))
            {
                goto 	  Wr_DSamSts_INI_FAIL3;
            }
        }
    }

Wr_DSamSts_INI_SUCCESS:
    lstFree(&list);
    vxsts=close(iFdRd);
    //assert(vxsts==OK);
    sts=FT_End_Update(EP_EDP_DSamSts_FILE, iFdWr);
    //assert(sts==EP_SUCCESS);
    return ;

Wr_DSamSts_INI_FAIL1:
    lstFree(&list);
    return ;

Wr_DSamSts_INI_FAIL2:
    lstFree(&list);
    FT_Abort_Update(EP_EDP_DSamSts_FILE, iFdWr);
    return ;

Wr_DSamSts_INI_FAIL3:
    lstFree(&list);
    FT_Abort_Update(EP_EDP_DSamSts_FILE, iFdWr);
    vxsts=close(iFdRd);
    return ;

}





/*功能:复制一个文件的备份文件,后缀为.cpy,供系统INI文件使用
  参数:pucRslt,返回新文件名
       strFile,源文件名
  返回:无  */
void FT_Cpy_Name(uint8_t *pucRslt, const uint8_t *strFile)
{

    strcpy(pucRslt, strFile );
    strcat(pucRslt, ".cpy");

}


/*功能:  复制edp01.ini文件到edp01.ini.cpy,防止该文件被毁坏时,能恢复
  参数:  无
  返回:  EP_SUCCESS 操作成功
         其他  操作失败   ZY  2011-6-29*/
EP_STATUS  FT_Cpy_Sys_Ini_File()
{
    /*复制 edp01.ini文件  到edp01.ini.cpy  */

    int iFd;
    int iFdRd;
    int i;
    uint8_t aucBuf[INI_LINE_LEN];
    STATUS vxsts;
    uint8_t aucCpyFile[FULL_NAME_LEN+4];   /* Keep one space to insert '#'. */
    uint8_t aucTempFile[FULL_NAME_LEN+4];

    /* 原为5s */
    vxsts = semTake(semSysIni_g, WAIT_FOREVER);
    assert(vxsts==OK);
    FT_Temp_Name(aucTempFile, EP_SYS_INI_FILE);
    /* TODO: what happens with creat when file already exists?
     * We want to open it on this suituation. */
    iFd=creat(aucTempFile, O_RDWR);
    if (iFd==ERROR)
    {
        vxsts=semGive(semSysIni_g);
        assert(vxsts==OK);

        return EP_ERROR;
    }
    iFdRd=open(EP_SYS_INI_FILE, O_RDONLY, 0);
    if (iFdRd==ERROR)
    {
        close(iFd);

        vxsts=semGive(semSysIni_g);
        assert(vxsts==OK);

        return EP_ERROR;
    }
    while ((i=read(iFdRd, aucBuf, INI_LINE_LEN))>0)
        write(iFd, aucBuf, i);
    close(iFd);
    close(iFdRd);

    FT_Cpy_Name(aucCpyFile, EP_SYS_INI_FILE);
    remove(aucCpyFile);

    rename(aucTempFile,aucCpyFile);

    vxsts=semGive(semSysIni_g);
    assert(vxsts==OK);

    return  EP_SUCCESS;

}

/* 复制auxedp.ini文件到auxedp.ini.cpy, 防止该文件被毁坏时, 能恢复.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS FT_Cpy_Aux_Sys_Ini_File(void)
{
    int iFd;
    int iFdRd;
    int i;
    uint8_t aucBuf[INI_LINE_LEN];
    uint8_t aucCpyFile[FULL_NAME_LEN+4];   /* Keep one space to insert '#'. */
    uint8_t aucTempFile[FULL_NAME_LEN+4];

    semTake(semAucSysIni_g, WAIT_FOREVER);
    FT_Temp_Name(aucTempFile, EP_AUX_SYS_CFG_FILE);

    /* TODO: what happens with creat when file already exists?
     * We want to open it on this suituation. */
    iFd = creat(aucTempFile, O_RDWR);
    if (iFd == ERROR)
    {
        semGive(semAucSysIni_g);
        return EP_ERROR;
    }

    iFdRd = open(EP_AUX_SYS_CFG_FILE, O_RDONLY, 0);
    if (iFdRd == ERROR)
    {
        close(iFd);
        semGive(semAucSysIni_g);

        return EP_ERROR;
    }
    while ((i = read(iFdRd, aucBuf, INI_LINE_LEN))>0)
        write(iFd, aucBuf, i);
    close(iFd);
    close(iFdRd);

    FT_Cpy_Name(aucCpyFile, EP_AUX_SYS_CFG_FILE);
    remove(aucCpyFile);

    rename(aucTempFile, aucCpyFile);

    semGive(semAucSysIni_g);

    return EP_SUCCESS;
}

/*功能:  初始化和读取系统INI文件
  参数:  无
  返回:  无
         ZY  2011-6-29
  注意:  因为有日志记录,需要放到日志功能初始化之后*/
void  FT_Init_Sys_Ini_File()
{
    uint8_t aucCpyFile[FULL_NAME_LEN+1];  /* 2011-6-30日 ZY */
    int iFd;
    int i;
    EP_STATUS sts;
    STATUS vxsts;

    /* 读写edp01.ini文件用, 由计数信号量改为互斥信号量 */
    semSysIni_g = semMCreate(SEM_Q_PRIORITY  | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);
    assert (semSysIni_g != NULL);

    /*2011-6-30  ZY 修改  */
    FT_Cpy_Name(aucCpyFile, EP_SYS_INI_FILE);
    if (!FT_Is_File(EP_SYS_INI_FILE) && !FT_Is_File(aucCpyFile))
    {
        b01IniChanged =TRUE;
        iFd = FT_Bgn_Update(EP_SYS_INI_FILE);
        assert (iFd >= 0);

        i = write(iFd, aucDftIni, sizeof(aucDftIni));
        assert (i == sizeof(aucDftIni));

        sts = FT_End_Update(EP_SYS_INI_FILE, iFd);
        assert (sts == EP_SUCCESS);

        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "因edp01.ini和INI备份文件不存在,创建新的系统INI文件.\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Because of INI and bakup file not exist, create new INI file.\n", NULL);
        }

    }
    else if (!FT_Is_File(EP_SYS_INI_FILE) && FT_Is_File(aucCpyFile))
    {
        /* 备份文件更名为正式文件 */
        b01IniChanged =TRUE;
        vxsts = rename(aucCpyFile, EP_SYS_INI_FILE);
        assert (vxsts == OK);

        if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "因edp01.ini不存在,将INI备份文件重命名为系统INI文件.\n", NULL);
        }
        else if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Because of INI file not exist, rename bakup file to  INI file.\n", NULL);
        }


    }

    /*备份INI文件  */
    FT_Cpy_Sys_Ini_File();

    /* 获取索引定值页序 */
    Hw_GetAcMdType();

}

/* 初始化和读取附加系统信息INI文件.
 * Para:
 *     NONE.
 * Return:
 *     OK, ERROR.
 */
void FT_Init_Auc_Sys_Ini_File(void)
{
    uint8_t aucCpyFile[FULL_NAME_LEN+1];
    int iFd;
    int i;
    EP_STATUS sts;
    STATUS vxsts;
    uint8_t aucBuf[FT_VER_INFO_LEN+1];

    semAucSysIni_g = semMCreate(SEM_Q_PRIORITY  | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);
    assert (semAucSysIni_g != NULL);

    FT_Cpy_Name(aucCpyFile, EP_AUX_SYS_CFG_FILE);
    if (!FT_Is_File(EP_AUX_SYS_CFG_FILE) && !FT_Is_File(aucCpyFile))
    {
        iFd = FT_Bgn_Update(EP_AUX_SYS_CFG_FILE);
        /*assert (iFd >= 0);*/
        if(iFd < 0)
        {
            return;
        }

        i = write(iFd, aucAuxDftIni, sizeof(aucAuxDftIni));
        /*assert (i == sizeof(aucAuxDftIni));*/
        if(i != sizeof(aucAuxDftIni))
        {
            return;
        }

        sts = FT_End_Update(EP_AUX_SYS_CFG_FILE, iFd);
        /*assert (sts == EP_SUCCESS);*/
        if(sts != EP_SUCCESS)
        {
            return;
        }

        /* 从原有文件读出, 兼容现有文件模式 */
        if (FT_Rd_Sys_INI("[SYSTEM]", "NewestSN", aucBuf, 32) == 1)
        {
            sts = FT_Wr_Aux_Sys_INI("[SYSTEM]", "NewestSN", aucBuf );
            /*assert(sts == EP_SUCCESS);*/
            if(sts != EP_SUCCESS)
            {
                LOG_Write(LOG_KERNEL, "创建auxedp.ini文件失败.\n", NULL);
                return;
            }
        }

        LOG_Write(LOG_KERNEL, "因auxedp.ini和INI备份文件不存在, 创建新的系统INI文件.\n", NULL);
    }
    else if (!FT_Is_File(EP_AUX_SYS_CFG_FILE) && FT_Is_File(aucCpyFile))
    {
        /* 备份文件更名为正式文件 */
        vxsts = rename(aucCpyFile, EP_AUX_SYS_CFG_FILE);
        /*assert (vxsts == OK);*/
        if(vxsts != OK)
        {
            LOG_Write(LOG_KERNEL, "重命名auxedp.ini失败.\n", NULL);
            return;
        }

        LOG_Write(LOG_KERNEL, "因auxedp.ini不存在, 将INI备份文件重命名为系统INI文件.\n", NULL);
    }

    /* 备份文件 */
    FT_Cpy_Aux_Sys_Ini_File();
}