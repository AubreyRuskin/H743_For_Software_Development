/* sysinfo.h - This file contains interface to manager system infomation files */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02b, 5jul07, dy add the processing to flag.
02a, 30may07, dy change the code style.
01c, 29jul03, hdx Updated to version 1.0.
01b, 27may03 hdx Verified version 0.1.
01a, 15feb03, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains interface to manager system infomation files.
INCLUDE: sysinfo.h
*/

/* includes */

#include "sysinfo.h"
#include "swcfg.h"
#include "filetool.h"
#include "FileCRC.h"
#include "errtest.h"
#include "miscfunc.h"
#include "view.h"
#include "realdata.h"
#include "hwcfg.h"
#include "rec.h"

/* 所有平台包含 */
#include "measure.h"

#include <stdio_compat.h>
#include "string_compat.h"
#include "ctype_compat.h"
#include <ioLib.h>

#include "VTBOX_Interface.h"
#include "VTBOX_SamInterface.h"
#include "EdpVer.h"
#include "bspinterface.h"
#include "HDL_VtBox.h"
#include "smvcfg.h"
#include "logLib.h"
/* defines */

#define HW_CFG_UPDT     0x01
#define SW_CFG_UPDT     0x02
#define LGC_CFG_UPDT    0x04
#define VX_CFG_UPDT     0X08
#define SYS_CFG_UPDT     0X10
#define APP_CFG_UPDT     0X20
#define USB_CFG_UPDT     0X40
#define MMI_CFG_UPDT     0X80
#define AUTOEXEC_CFG_UPDT     0X100
#define VX_BOOTROM_UPDT     0X200
#define ACMODTYPE_UPDT      0X400
#define EXTMODVER_UPDT      0X800

#define FUNC_OPT_CFG_UPDT 0x1000 /* 功能选配文件 */

/* globals */

SI_SYS_VER  SI_SysVer_g;

#ifdef ITEM_LEN_81
const uint8_t * const pucDiFileHead_g=
    "EDP 01 DI setting file - DO NOT EDIT!"
    "                                           \n";
const uint8_t * const pucLinkFileHead_g=
    "EDP 01 link setting file - DO NOT EDIT!"
    "                                         \n";
#else

const uint8_t * const pucDiFileHead_g=
    "DI file       \n";

const uint8_t * const pucLinkFileHead_g=
    "link file     \n";
#endif

const uint8_t * const pucFuncFileHead_g=
    "EDP 01 protect function setting file - DO NOT EDIT!"
    "                             \n";
const uint8_t * const pucLinkModeFileHead_g=
    "EDP 01 link mode file - DO NOT EDIT!"
    "                                            \n";

extern SC_SET_PAGE *psetpg_g; /*from swcfg.c*/

/* locals */

static   u_int  uiUpdtFg=0;

/* globals */

/* local functions */


static void SI_Wr_Prtc_Cfg(int iFd);
static void SI_Wr_Hw_AI_Cfg(int iFd);
static void SI_Wr_DI_Cfg(int iFd);
static void SI_Wr_DO_Cfg(int iFd);
static void SI_Wr_Hw_Led_Cfg(int iFd);
static void SI_Wr_Sw_Led_Cfg(int iFd);
static void SI_Wr_Set_Cfg(int iFd);
static void SI_Wr_Link_Cfg(int iFd);
static void SI_Wr_Rec_AI_Cfg(int iFd);
static void SI_Wr_Rec_DI_Cfg(int iFd);
static void SI_Wr_Flag_Cfg(int iFd);
static void SI_Wr_Evt_Cfg(int iFd);
static void SI_Wr_Mea_AI_Cfg(int iFd);
static void SI_Wr_Mea_DI_Cfg(int iFd);
static void SI_Wr_PO_Cfg(int iFd);
static void SI_Wr_PI_Cfg(int iFd);
static void SI_Wr_Mea_DO_Cfg(int iFd);
static void SI_Wr_Msu_Cfg(int iFd);
void SI_New_DI_File(void);
void SI_New_Link_File(void);
static void SI_New_Func_File(void);
static void SI_New_Inner_Set(void);
static void SI_New_CK_Set(void);

/* 检查压板文件是否有CRC
 * Para:
 *     pbCrc, 检查结果指针.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
extern EP_STATUS SC_Judge_Sw_Link(BOOL *pbCrc);

/***********************************************************************
* SI_New_Link_MODE_File - 检查当前“定制压板模式配置文件”内容是否需要重建或者更新
*
* RETURNS: 无
*
*/
static void SI_New_Link_MODE_File();

/*DQ: 将此定义从SI_Chk_Cfg()函数中移到静态全局,SI_Chk_Cfg()中被赋值后,
在SI_Init_Sys_Info()中SI_Creat_SCI()之后判断是否需要写ini文件*/
static uint8_t aucHwNowCfgVer[FT_VER_INFO_LEN+1]="";
static uint8_t aucSwNowCfgVer[FT_VER_INFO_LEN+1]="";
static uint8_t aucLgNowCfgVer[FT_VER_INFO_LEN+1]="";
static uint8_t aucFuncOptNowCfgVer[FT_VER_INFO_LEN+1] = ""; /* 功能选配文件 */

UNITE_VER_INFO UnVerInfo_g;
UNITE_VER_INFO UnVerInfoRd_g;
uint16_t UnVerMatchedFlag_g=FALSE;

extern EP_HW_VER_MAP aHwVerMapArr[];
extern BOOL bSciChangedFlag_g;
extern SEM_ID semCkCRCIni_g;

/*功能：得到CPU的SV虚端子配置信息  2013-6-5  ZY
  参数：ppRtTotalCfgAddr：供返回SV虚端子的总体配置信息全局变量地址对应的变量的指针。
                      该全局变量地址对应的变量由调用方分配，被调用方填充。
                      该全局变量自身，由被调用方分配和管理
  返回：EP_SUCCESS:操作成功
       其他，操作失败 */
extern EP_STATUS  SMV_Get_Vt_SV_Term_Cfg(SMV_TOTAL_VT_SV_TERM_CFG   **ppRtTotalCfgAddr);

/* matching the 3 configuration file(hwcfg.ehc\swcfg.esc\logic.egs), called before SI_Init_Sys_Info.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS SI_Chk_Cfg()
{
    uint8_t aucIniCfgVer[FT_VER_INFO_LEN+1];
    int i;
    char back_filename[100];
    char new_tmp_filename[100];
    BOOL backfileisexisted = FALSE;
#if 0
    BOOL bCrc = FALSE; /* 压板是否有自校验CRC */
#endif

    if (FT_Get_Cfg_File_Ver(EP_HW_CFG_FILE, aucHwNowCfgVer) == EP_SUCCESS)
    {
        i = FT_Rd_Sys_INI("[SYSTEM]", "HwCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (i != 1 || strcmp(aucIniCfgVer, aucHwNowCfgVer))
        {
            if (i == 0)
            {
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "因[SYSTEM] HwCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL, "Because of failing ro read [SYSTEM] HwCfgVer, create new ini file.\n", NULL);
                }

                FT_New_SYS_INI_File();
            }
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "硬件配置文件更新.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "Hareware config file is updated.\n", NULL);
            }

            uiUpdtFg |= HW_CFG_UPDT;
        }
    }
    else
    {
        return EP_ERROR;
    }

    SI_SysVer_g.unSwCfgCRC = VER_GetSwCfgCRC();
    if (FT_Get_Cfg_File_Ver(EP_SW_CFG_FILE, aucSwNowCfgVer) == EP_SUCCESS)
    {
        i = FT_Rd_Sys_INI("[SYSTEM]", "SwCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (i != 1 || strcmp(aucIniCfgVer, aucSwNowCfgVer))
        {
            if (i == 0)
            {
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "因[SYSTEM] SwCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL, "Because of failing ro read [SYSTEM] SwCfgVer, create new ini file.\n", NULL);
                }

                FT_New_SYS_INI_File();
            }

            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "软件配置文件更新.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "Software config file is updated.\n", NULL);
            }

            uiUpdtFg |= SW_CFG_UPDT;
        }
    }
    else
    {
        return   EP_ERROR;
    }

    if (FT_Get_Cfg_File_Ver(EP_LGC_CFG_FILE, aucLgNowCfgVer) == EP_SUCCESS)
    {
        i = FT_Rd_Sys_INI("[SYSTEM]", "LgcCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (i != 1 || strcmp(aucIniCfgVer, aucLgNowCfgVer))
        {
            if (i == 0)
            {
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "因[SYSTEM] LgcCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL, "Because of failing ro read [SYSTEM] LgcCfgVer, create new ini file.\n", NULL);
                }

                FT_New_SYS_INI_File();
            }

            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "逻辑图配置文件更新.\n", NULL);
            }
            else if(ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "Logic diagram config file is updated.\n", NULL);
            }

            uiUpdtFg |= LGC_CFG_UPDT;
        }
    }
    else
    {
        return EP_ERROR;
    }

    /* 选配功能文件是否存在, 不存在则不进行判断 */
    if (FT_Is_File(EP_FUNCOPTION_FILE))
    {
        if (FT_Get_Cfg_File_Ver(EP_FUNCOPTION_FILE, aucFuncOptNowCfgVer) == EP_SUCCESS)
        {
            i = FT_Rd_Sys_INI("[SYSTEM]", "FuncOptVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
            if (i != 1 || strcmp(aucIniCfgVer, aucFuncOptNowCfgVer))
            {
                if (i == 0)
                {
                    if (ENG_MODE == 0)
                    {
                        LOG_Write(LOG_KERNEL, "因[SYSTEM] FuncOptVer值读取失败，创建新的系统INI文件.\n", NULL);
                    }
                    else if (ENG_MODE == 1)
                    {
                        LOG_Write(LOG_KERNEL, "Because of failing ro read [SYSTEM] FuncOptVer, create new ini file.\n", NULL);
                    }

                    FT_New_SYS_INI_File();
                }
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_OPRATE, "功能选配文件更新.\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_OPRATE, "Function option file is updated.\n", NULL);
                }

                /* 需考虑是否有必要更新SCI文件, 目前更新, 同时告警 */
                uiUpdtFg |= FUNC_OPT_CFG_UPDT;
            }
        }
        else
        {
            return EP_ERROR;
        }
    }

    if ((uiUpdtFg & HW_CFG_UPDT) || !FT_Is_File(EP_DI_STS_FILE) || (uiUpdtFg & VX_BOOTROM_UPDT))
    {
        if (!(uiUpdtFg & HW_CFG_UPDT) && !(uiUpdtFg & VX_BOOTROM_UPDT))
        {
            strcpy(new_tmp_filename,"");
            Bak_File_Name(new_tmp_filename,EP_DI_STS_FILE,".bkf");
            if(FT_Is_File(new_tmp_filename))
            {
                backfileisexisted = TRUE;
                rename(new_tmp_filename, EP_DI_STS_FILE);
            }

            if(backfileisexisted)
            {
                LOG_Write(LOG_RUN, "DI强制状态使用备份文件.\n", NULL);
                backfileisexisted = FALSE;
            }
            else
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                               "系统文件[%s]错误\n", (int)EP_DI_STS_FILE, 0);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                               "File error(missing/corrupted):system file %s missing\n", (int)EP_DI_STS_FILE, 0);
                }
                SI_New_DI_File();
            }
        }
        else
        {
            SI_New_DI_File();
        }
    }

    /* 格式已经不能兼容 */
#if 0
    /* 文件存在时判断是否有CRC
     * 如不存在则重新生成带CRC的开入强制文件
     */
    if (FT_Is_File(EP_DI_STS_FILE) && (SC_Judge_Force_Di(&bCrc) == EP_SUCCESS))
    {
        if (!bCrc)
        {
            LOG_Write(LOG_KERNEL, "开入强制文件增加CRC校验\n", NULL);
            SI_New_DI_File();
        }
    }
#endif

    if ((uiUpdtFg & SW_CFG_UPDT) || !FT_Is_File(EP_LINK_STS_FILE) || (uiUpdtFg & VX_BOOTROM_UPDT))
    {
        if (!(uiUpdtFg & SW_CFG_UPDT) && !(uiUpdtFg & VX_BOOTROM_UPDT))
        {
            strcpy(new_tmp_filename,"");
            Bak_File_Name(new_tmp_filename,EP_LINK_STS_FILE,".bkf");
            if(FT_Is_File(new_tmp_filename))
            {
                backfileisexisted = TRUE;
                rename(new_tmp_filename, EP_LINK_STS_FILE);
            }

            if(backfileisexisted)
            {
                LOG_Write(LOG_RUN, "软压板状态使用备份文件.\n", NULL);
                backfileisexisted = FALSE;
            }
            else
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM  |ER_LOCK| ER_REPORT,
                               "系统文件[%s]错误\n", (int)EP_LINK_STS_FILE, 0);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM  |ER_LOCK| ER_REPORT,
                               "system file %s missing\n", (int)EP_LINK_STS_FILE, 0);
                }
                SI_New_Link_File();
            }
        }
        else
        {
            SI_New_Link_File();
        }
    }

    /* 格式已经不能兼容 */
#if 0
    /* 文件存在时判断是否有CRC
     * 如不存在则重新生成带CRC的压板文件
     */
    if (FT_Is_File(EP_LINK_STS_FILE) && (SC_Judge_Sw_Link(&bCrc) == EP_SUCCESS))
    {
        if (!bCrc)
        {
            LOG_Write(LOG_KERNEL, "压板文件增加CRC校验\n", NULL);
            SI_New_Link_File();
        }
    }
#endif


    if ((uiUpdtFg & SW_CFG_UPDT) || !FT_Is_File(EP_LINK_MODE_FILE) || (uiUpdtFg & VX_BOOTROM_UPDT))
    {
        /* 软件配置文件更改或“定制模式压板配置文件”不存在，重新生成“定制模式压板配置文件”*/
        if (!(uiUpdtFg & SW_CFG_UPDT) && !(uiUpdtFg & VX_BOOTROM_UPDT))
        {
            if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM  |ER_LOCK| ER_REPORT,
                           "File error(missing/corrupted):system file %s missing\n", (int)EP_LINK_MODE_FILE, 0);
            }
            else if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM |ER_LOCK | ER_REPORT,
                           "系统文件[%s]错误\n", (int)EP_LINK_MODE_FILE, 0);
            }
        }

        SI_New_Link_MODE_File();
    }

    if ((uiUpdtFg & SW_CFG_UPDT) || !FT_Is_File(EP_INNER_SET_FILE) || (uiUpdtFg & VX_BOOTROM_UPDT))
    {
        if (!(uiUpdtFg & SW_CFG_UPDT) && !(uiUpdtFg & VX_BOOTROM_UPDT))
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM  |ER_LOCK| ER_REPORT,
                           "系统文件[%s]错误\n", (int)EP_INNER_SET_FILE, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM |ER_LOCK | ER_REPORT,
                           "File error(missing/corrupted):system file %s missing\n", (int)EP_INNER_SET_FILE, 0);
            }
        }

        SI_New_Inner_Set();
    }

    /* 对EDP01，开放测控定值功能 */
    if ((uiUpdtFg & SW_CFG_UPDT) || !FT_Is_File(EP_CK_SET_FILE) || (uiUpdtFg & VX_BOOTROM_UPDT))
    {
        if (!(uiUpdtFg & SW_CFG_UPDT) && !(uiUpdtFg & VX_BOOTROM_UPDT))
        {
            if (CrcInfo_g.bCkSetCrcWrFlag)
            {
                strcpy(back_filename,"");
                strcpy(new_tmp_filename,"");
                Bak_File_Name(back_filename,EP_CK_SET_FILE,".old");
                Bak_File_Name(new_tmp_filename,EP_CK_SET_FILE,".bak");
                if(FT_Is_File(new_tmp_filename))
                {
                    backfileisexisted = TRUE;
                    rename(new_tmp_filename, EP_CK_SET_FILE);
                }
                else if(FT_Is_File(back_filename))
                {
                    backfileisexisted = TRUE;
                    rename(back_filename, EP_CK_SET_FILE);
                }
            }

            if(backfileisexisted)
            {
                LOG_Write(LOG_RUN, "参数定值使用备份文件.\n", NULL);
                backfileisexisted = FALSE;
            }
            else
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM |ER_LOCK | ER_REPORT,
                               "系统文件[%s]错误\n", (int)EP_CK_SET_FILE, 0);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM |ER_LOCK | ER_REPORT,
                               "File error(missing/corrupted):system file %s missing\n", (int)EP_CK_SET_FILE, 0);
                }
                SI_New_CK_Set();
            }
        }
        else
        {
            SI_New_CK_Set();
        }
    }

    if ((uiUpdtFg & HW_CFG_UPDT))
    {
        /* 更新电量存储 */
        ResetFRAMVal();
    }

    if ((uiUpdtFg & LGC_CFG_UPDT) || !FT_Is_File(EP_FUNC_STS_FILE) || (uiUpdtFg & VX_BOOTROM_UPDT))
    {
        if (!(uiUpdtFg & LGC_CFG_UPDT) && !(uiUpdtFg & VX_BOOTROM_UPDT))
        {
            if(CrcInfo_g.bFunStsWrFlag)
            {
                strcpy(new_tmp_filename,"");
                Bak_File_Name(new_tmp_filename,EP_FUNC_STS_FILE,".bkf");
                if(FT_Is_File(new_tmp_filename))
                {
                    backfileisexisted = TRUE;
                    rename(new_tmp_filename, EP_FUNC_STS_FILE);
                }
            }

            if(backfileisexisted)
            {
                LOG_Write(LOG_RUN, "保护投退使用备份文件.\n", NULL);
                backfileisexisted = FALSE;
            }
            else
            {
                if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM |ER_LOCK | ER_REPORT,
                               "系统文件[%s]错误\n", (int)EP_FUNC_STS_FILE, 0);
                }
                else if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM |ER_LOCK | ER_REPORT,
                               "File error(missing/corrupted):system file %s missing\n", (int)EP_FUNC_STS_FILE, 0);
                }
                SI_New_Func_File();
            }
        }
        else
        {
            SI_New_Func_File();
        }
    }

    return EP_SUCCESS;

}

/* 判断是否需要重新生成sci文件.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, 需要重新生成sci文件,并且旧sci文件已经被删除
 *     FALSE, 不需重新生成.
 */
BOOL SI_Need_Reset_SCI()
{
    uint8_t aucIniCfgVer[FT_VER_INFO_LEN+1];

#ifndef VXWORKS_ROM
    uint8_t aucVxNowCfgVer[FT_VER_INFO_LEN+1];
#endif

    uint8_t aucSysNowCfgVer[FT_VER_INFO_LEN+1];
    uint8_t aucAppNowCfgVer[FT_VER_INFO_LEN+1];
    uint8_t aucAutoexecNowCfgVer[FT_VER_INFO_LEN+1];
    uint8_t aucBootromVer[FT_VER_INFO_LEN+1];
    uint16_t unBootromVer;
    uint8_t ucDelSciFlag = 0;
    STATUS vxsts;

    if (uiUpdtFg)
    {
        ucDelSciFlag = TRUE;
        goto DEL;
    }

    if (EP_GetAcMdTypeChgFlag())
    {
        if (ENG_MODE == 0)
        {
            LOG_Write(LOG_RUN, "因AC模件类型切换，需重新生成系统配置文件.\n", NULL);
        }
        else if (ENG_MODE == 1)
        {
            LOG_Write(LOG_RUN,
                      "Need to create new configuration file because of AC type changed.\n", NULL);
        }

        ucDelSciFlag = TRUE;
        uiUpdtFg |= ACMODTYPE_UPDT;		/* AC模件类型切换 */
    }

#ifdef VXWORKS_ROM
    if (((unBootromVer=GetBootromVer())>0) || ((unBootromVer=GetBspVer())>0))
#else
    if ((unBootromVer=GetBootromVer())>0)
#endif
    {
        sprintf(aucBootromVer, "%x.%x%s", HI8(unBootromVer), LO8(unBootromVer), aucsysBspVer);
        FT_Rd_Sys_INI("[SYSTEM]", "BootromVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (strcmp(aucIniCfgVer, aucBootromVer))
        {
            ucDelSciFlag = TRUE;
            uiUpdtFg |= VX_BOOTROM_UPDT;

            goto DEL;
        }
    }
#ifndef VXWORKS_ROM
    if (FT_Get_File_modify_time(EP_VX_RUN_FILE, aucVxNowCfgVer) == EP_SUCCESS)
    {
        FT_Rd_Sys_INI("[SYSTEM]", "VxCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        aucIniCfgVer[26] = '\0';
        if (strcmp(aucIniCfgVer, aucVxNowCfgVer))
        {
            ucDelSciFlag = TRUE;
            uiUpdtFg |= VX_CFG_UPDT;

            goto DEL;
        }
    }
#endif

#if defined(EDP_DYNAMICLOAD)
    if (FT_Get_File_modify_time(EP_EDP_SYS_FILE, aucSysNowCfgVer) == EP_SUCCESS)
    {
        FT_Rd_Sys_INI("[SYSTEM]", "SysCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        aucIniCfgVer[26]='\0';
        if (strcmp(aucIniCfgVer, aucSysNowCfgVer))
        {
            ucDelSciFlag = TRUE;
            uiUpdtFg |= SYS_CFG_UPDT;

            goto DEL;
        }
    }

    if (FT_Get_File_modify_time(EP_EDP_APP_FILE, aucAppNowCfgVer) == EP_SUCCESS)
    {
        FT_Rd_Sys_INI("[SYSTEM]", "AppCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        aucIniCfgVer[26] = '\0';
        if (strcmp(aucIniCfgVer, aucAppNowCfgVer))
        {
            ucDelSciFlag = TRUE;
            uiUpdtFg |= APP_CFG_UPDT;

            goto DEL;
        }
    }

    if (FT_Get_File_modify_time(EP_EDP_AUTOEXEC_FILE, aucAutoexecNowCfgVer) == EP_SUCCESS)
    {
        FT_Rd_Sys_INI("[SYSTEM]", "AutoexecCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        aucIniCfgVer[26]='\0';
        if (strcmp(aucIniCfgVer, aucAutoexecNowCfgVer))
        {
            ucDelSciFlag = TRUE;
            uiUpdtFg |= AUTOEXEC_CFG_UPDT;

            goto DEL;
        }
    }
#endif

DEL:
    if (ucDelSciFlag)
    {
        if (FT_Is_File(EP_SYS_INFO_FILE))
        {
            vxsts = remove(EP_SYS_INFO_FILE);
            assert (vxsts == OK);
        }
        return TRUE;
    }
    else if (!FT_Is_File(EP_SYS_INFO_FILE))
    {
        return TRUE;
    }
    else
        return FALSE;
}

/* 生成系统信息文件.
 * Para:
 *     strSysCfgFile, system configuration file name.
 * Return:
 *     EP_SUCCESS, EP_SUCCESS.
 *     EP_ERROR, EP_ERROR.
 */
EP_STATUS SI_Init_Sys_Info(const uint8_t *strSysCfgFile)
{
    uint8_t aucIniCfgVer[FT_VER_INFO_LEN+1];
    uint8_t aucVxNowCfgVer[FT_VER_INFO_LEN+1];
    uint8_t aucSysNowCfgVer[FT_VER_INFO_LEN+1];
    uint8_t aucAppNowCfgVer[FT_VER_INFO_LEN+1];

#if defined(EDP03_BUILD)
    uint8_t aucUsbNowCfgVer[FT_VER_INFO_LEN+1];
#endif

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
    uint8_t aucMmiNowCfgVer[FT_VER_INFO_LEN+1];
#endif

    uint8_t aucAutoexecNowCfgVer[FT_VER_INFO_LEN+1];
    uint8_t aucBootromVer[FT_VER_INFO_LEN+1];
    uint16_t unBootromVer;

    int i;
    EP_STATUS sts;
    HDL_TOTAL_VT_DI_TERM_CFG *pGsDiCfg = NULL;
    SMV_TOTAL_VT_SV_TERM_CFG *pSvCfg = NULL;

    /* 虚端子GOOSE DI配置 */
    if (HDL_Get_Vt_DI_Term_Cfg(&pGsDiCfg) != EP_SUCCESS)
    {
        return(FALSE);
    }

    /* 虚端子SV配置 */
    if (SMV_Get_Vt_SV_Term_Cfg(&pSvCfg) != EP_SUCCESS)
    {
        return(FALSE);
    }

#ifdef VXWORKS_ROM
    if (((unBootromVer = GetBootromVer())>0) || ((unBootromVer = GetBspVer())>0))
#else
    if ((unBootromVer = GetBootromVer())>0)
#endif
    {
        sprintf(aucBootromVer, "%x.%x%s", HI8(unBootromVer), LO8(unBootromVer), aucsysBspVer);
        i = FT_Rd_Sys_INI("[SYSTEM]", "BootromVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (i != 1 || strcmp(aucIniCfgVer, aucBootromVer))
        {
            if (i == 0)
            {
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "因[SYSTEM] BootromVer值读取失败，创建新的系统INI文件.\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Because of failing ro read [SYSTEM] BootromVer, create new ini file.\n", NULL);
                }

                FT_New_SYS_INI_File();
            }
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "BootRom 更新.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "BootRom is changed.\n", NULL);
            }

            uiUpdtFg |= VX_BOOTROM_UPDT;
        }
    }
    else
    {
        return EP_ERROR;
    }
#ifndef VXWORKS_ROM
    if (FT_Get_File_Ver(EP_VX_RUN_FILE, aucVxNowCfgVer) == EP_SUCCESS)
    {
        i = FT_Rd_Sys_INI("[SYSTEM]", "VxCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (i != 1 || strcmp(aucIniCfgVer, aucVxNowCfgVer))
        {
            if (i == 0)
            {
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "因[SYSTEM] VxCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
                }
                else if(ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Because of failing ro read [SYSTEM] VxCfgVer, create new ini file.\n", NULL);
                }

                FT_New_SYS_INI_File();
            }
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "文件vxworks 更新.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "vxworks is changed.\n", NULL);
            }

            uiUpdtFg |= VX_CFG_UPDT;
        }
    }
    else
    {
        return EP_ERROR;
    }
#endif

#if defined(EDP_DYNAMICLOAD)
#ifdef EDP03_BUILD /* EDP03 platform */

#if defined(EDP03_STABCONTROL_BUILD) || defined(EDP03_LOWPROTECT_BUILD)

    if (FT_Get_File_Ver(EP_EDP_SYS_FILE, aucSysNowCfgVer) == EP_SUCCESS)
    {
        i = FT_Rd_Sys_INI("[SYSTEM]", "SysCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (i != 1 || strcmp(aucIniCfgVer, aucSysNowCfgVer))
        {
            if (i == 0)
            {
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "因[SYSTEM] SysCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Because of failing ro read [SYSTEM] SysCfgVer, create new ini file.\n", NULL);
                }
                FT_New_SYS_INI_File();
            }
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "文件edpsys.out 更新.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "edpsys.out is changed.\n", NULL);
            }

            uiUpdtFg |= SYS_CFG_UPDT;
        }
    }
    else
    {
        return EP_ERROR;
    }

    if (FT_Get_File_Ver(EP_EDP_APP_FILE, aucAppNowCfgVer) == EP_SUCCESS)
    {
        i = FT_Rd_Sys_INI("[SYSTEM]", "AppCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (i != 1 || strcmp(aucIniCfgVer, aucAppNowCfgVer))
        {
            if (i == 0)
            {
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "因[SYSTEM] AppCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Because of failing ro read [SYSTEM] AppCfgVer, create new ini file.\n", NULL);
                }
                FT_New_SYS_INI_File();
            }
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "文件edpapp.out 更新.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "edpapp.out is changed.\n", NULL);
            }

            uiUpdtFg |= APP_CFG_UPDT;
        }
    }
    else
    {
        return EP_ERROR;
    }

    if (FT_Get_File_Ver(EP_EDP_USB_FILE, aucUsbNowCfgVer) == EP_SUCCESS)
    {
        i = FT_Rd_Sys_INI("[SYSTEM]", "UsbCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (i != 1 || strcmp(aucIniCfgVer, aucUsbNowCfgVer))
        {
            if (i == 0)
            {
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "因[SYSTEM] UsbCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Because of failing ro read [SYSTEM] UsbCfgVer, create new ini file.\n", NULL);
                }
                FT_New_SYS_INI_File();
            }
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "文件usb.out 更新.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "usb.out is changed.\n", NULL);
            }

            uiUpdtFg |= USB_CFG_UPDT;
        }
    }
    else
    {
        return EP_ERROR;
    }

    if (FT_Get_File_Ver(EP_EDP_MMI_FILE, aucMmiNowCfgVer) == EP_SUCCESS)
    {
        i = FT_Rd_Sys_INI("[SYSTEM]", "MmiCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (i != 1 || strcmp(aucIniCfgVer, aucMmiNowCfgVer))
        {
            if (i == 0)
            {
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "因[SYSTEM] MmiCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Because of failing ro read [SYSTEM] MmiCfgVer, create new ini file.\n", NULL);
                }
                FT_New_SYS_INI_File();
            }
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "文件mmi.out 更新.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "mmi.out is changed.\n", NULL);
            }

            uiUpdtFg |= MMI_CFG_UPDT;
        }
    }
    else
    {
        return EP_ERROR;
    }

#endif	/* end of EDP03_STABCONTROL_BUILD || EDP03_LOWPROTECT_BUILD */

#if defined(EDP03_INTELBOX_BUILD)

    if (FT_Get_File_Ver(EP_EDP_SYS_FILE, aucSysNowCfgVer) == EP_SUCCESS)
    {
        i = FT_Rd_Sys_INI("[SYSTEM]", "SysCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (i != 1 || strcmp(aucIniCfgVer, aucSysNowCfgVer))
        {
            if (i == 0)
            {
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "因[SYSTEM] SysCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Because of failing ro read [SYSTEM] SysCfgVer, create new ini file.\n", NULL);
                }
                FT_New_SYS_INI_File();
            }

            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "文件edpsys.out 更新.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "edpsys.out is changed.\n", NULL);
            }

            uiUpdtFg |= SYS_CFG_UPDT;
        }
    }
    else
    {
        return EP_ERROR;
    }

    if (FT_Get_File_Ver(EP_EDP_APP_FILE, aucAppNowCfgVer) == EP_SUCCESS)
    {
        i = FT_Rd_Sys_INI("[SYSTEM]", "AppCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (i != 1 || strcmp(aucIniCfgVer, aucAppNowCfgVer))
        {
            if (i == 0)
            {
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "因[SYSTEM] AppCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Because of failing ro read [SYSTEM] AppCfgVer, create new ini file.\n", NULL);
                }
                FT_New_SYS_INI_File();
            }
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "文件edpapp.out 更新.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "edpapp.out is changed.\n", NULL);
            }

            uiUpdtFg |= APP_CFG_UPDT;
        }
    }
    else
    {
        return EP_ERROR;
    }

#endif		/* end of EDP03_INTELBOX_BUILD */

#endif	/* end of EDP03_BUILD */

#if defined(EDP_01_02_BUILD)

    // if (FT_Get_File_Ver(EP_EDP_SYS_FILE, aucSysNowCfgVer) == EP_SUCCESS)
    // {
    //     i = FT_Rd_Sys_INI("[SYSTEM]", "SysCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
    //     if (i != 1 || strcmp(aucIniCfgVer, aucSysNowCfgVer))
    //     {
    //         if (i == 0)
    //         {
    //             if (ENG_MODE == 0)
    //             {
    //                 LOG_Write(LOG_KERNEL, "因[SYSTEM] SysCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
    //             }
    //             else if (ENG_MODE == 1)
    //             {
    //                 LOG_Write(LOG_KERNEL,
    //                           "Because of failing ro read [SYSTEM] SysCfgVer, create new ini file.\n", NULL);
    //             }
    //             FT_New_SYS_INI_File();
    //         }
    //         if (ENG_MODE == 0)
    //         {
    //             LOG_Write(LOG_OPRATE, "文件edpsys.out 更新.\n", NULL);
    //         }
    //         else if (ENG_MODE == 1)
    //         {
    //             LOG_Write(LOG_OPRATE, "edpsys.out is changed.\n", NULL);
    //         }

    //         uiUpdtFg |= SYS_CFG_UPDT;
    //     }
    // }
    // else
    // {
    //     static uint8_t aucLogInfo[256];

    //     if (ENG_MODE == 0)
    //     {
    //         sprintf(aucLogInfo, "获取文件%s版本信息失败.\n",
    //                 EP_EDP_SYS_FILE);
    //     }
    //     else if (ENG_MODE == 1)
    //     {
    //         sprintf(aucLogInfo, "Getting the version of file %s failed.\n",
    //                 EP_EDP_SYS_FILE);
    //     }

    //     LOG_Write(LOG_OPRATE, aucLogInfo, NULL);

    //     return EP_ERROR;
    // }

    // if (FT_Get_File_Ver(EP_EDP_APP_FILE, aucAppNowCfgVer) == EP_SUCCESS)
    // {
    //     i = FT_Rd_Sys_INI("[SYSTEM]", "AppCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
    //     if (i != 1 || strcmp(aucIniCfgVer, aucAppNowCfgVer))
    //     {
    //         if (i == 0)
    //         {
    //             if (ENG_MODE == 0)
    //             {
    //                 LOG_Write(LOG_KERNEL, "因[SYSTEM] AppCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
    //             }
    //             else if (ENG_MODE == 1)
    //             {
    //                 LOG_Write(LOG_KERNEL,
    //                           "Because of failing ro read [SYSTEM] AppCfgVer, create new ini file.\n", NULL);
    //             }
    //             FT_New_SYS_INI_File();
    //         }
    //         if (ENG_MODE == 0)
    //         {
    //             LOG_Write(LOG_OPRATE, "文件edpapp.out 更新.\n", NULL);
    //         }
    //         else if (ENG_MODE == 1)
    //         {
    //             LOG_Write(LOG_OPRATE, "edpapp.out is changed.\n", NULL);
    //         }

    //         uiUpdtFg |= APP_CFG_UPDT;
    //     }
    // }
    // else
    // {
    //     static uint8_t aucLogInfo[256];

    //     if (ENG_MODE == 0)
    //     {
    //         sprintf(aucLogInfo, "获取文件%s版本信息失败!\n",
    //                 EP_EDP_APP_FILE);
    //     }
    //     else if (ENG_MODE == 1)
    //     {
    //         sprintf(aucLogInfo, "Getting the version of file %s failed!\n",
    //                 EP_EDP_APP_FILE);
    //     }

    //     LOG_Write(LOG_OPRATE, aucLogInfo, NULL);

    //     return EP_ERROR;
    // }

#endif		/* end of EDP_01_02_BUILD */

#if defined(EXCITE_BUILD)

    if (FT_Get_File_Ver(EP_EDP_SYS_FILE, aucSysNowCfgVer) == EP_SUCCESS)
    {
        i = FT_Rd_Sys_INI("[SYSTEM]", "SysCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (i != 1 || strcmp(aucIniCfgVer, aucSysNowCfgVer))
        {
            if (i == 0)
            {
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "因[SYSTEM] SysCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Because of failing ro read [SYSTEM] SysCfgVer, create new ini file.\n", NULL);
                }
                FT_New_SYS_INI_File();
            }
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "文件edpsys.out 更新.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "edpsys.out is changed.\n", NULL);
            }

            uiUpdtFg |= SYS_CFG_UPDT;
        }
    }
    else
    {
        return EP_ERROR;
    }

    if (FT_Get_File_Ver(EP_EDP_APP_FILE, aucAppNowCfgVer) == EP_SUCCESS)
    {
        i = FT_Rd_Sys_INI("[SYSTEM]", "AppCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (i != 1 || strcmp(aucIniCfgVer, aucAppNowCfgVer))
        {
            if (i == 0)
            {
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "因[SYSTEM] AppCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Because of failing ro read [SYSTEM] AppCfgVer, create new ini file.\n", NULL);
                }
                FT_New_SYS_INI_File();
            }

            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "文件edpapp.out 更新.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "edpapp.out is changed.\n", NULL);
            }

            uiUpdtFg |= APP_CFG_UPDT;
        }
    }
    else
    {
        return EP_ERROR;
    }

    if (FT_Get_File_Ver(EP_EDP_MMI_FILE, aucMmiNowCfgVer) == EP_SUCCESS)
    {
        i = FT_Rd_Sys_INI("[SYSTEM]", "MmiCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
        if (i != 1 || strcmp(aucIniCfgVer, aucMmiNowCfgVer))
        {
            if (i == 0)
            {
                if (ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "因[SYSTEM] MmiCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
                }
                else if (ENG_MODE == 1)
                {
                    LOG_Write(LOG_KERNEL,
                              "Because of failing ro read [SYSTEM] MmiCfgVer, create new ini file.\n", NULL);
                }
                FT_New_SYS_INI_File();
            }
            if (ENG_MODE == 0)
            {
                LOG_Write(LOG_OPRATE, "文件mmi.out 更新.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                LOG_Write(LOG_OPRATE, "mmi.out is changed.\n", NULL);
            }

            uiUpdtFg |= MMI_CFG_UPDT;
        }
    }
    else
    {
        return EP_ERROR;
    }

#endif		/* end of EXCITE_BUILD */

    // if (FT_Get_File_Ver(EP_EDP_AUTOEXEC_FILE, aucAutoexecNowCfgVer) == EP_SUCCESS)
    // {
    //     i = FT_Rd_Sys_INI("[SYSTEM]", "AutoexecCfgVer", aucIniCfgVer, FT_VER_INFO_LEN+1);
    //     if (i != 1 || strcmp(aucIniCfgVer, aucAutoexecNowCfgVer))
    //     {
    //         if (i == 0)
    //         {
    //             if (ENG_MODE == 0)
    //             {
    //                 LOG_Write(LOG_KERNEL, "因[SYSTEM] AutoexecCfgVer值读取失败，创建新的系统INI文件.\n", NULL);
    //             }
    //             else if (ENG_MODE == 1)
    //             {
    //                 LOG_Write(LOG_KERNEL,
    //                           "Because of failing ro read [SYSTEM] AutoexecCfgVer, create new ini file.\n", NULL);
    //             }
    //             FT_New_SYS_INI_File();
    //         }
    //         if (ENG_MODE == 0)
    //         {
    //             LOG_Write(LOG_OPRATE, "文件autoexec.ini 更新.\n", NULL);
    //         }
    //         else if (ENG_MODE == 1)
    //         {
    //             LOG_Write(LOG_OPRATE, "autoexec.ini is changed.\n", NULL);
    //         }

    //         uiUpdtFg |= AUTOEXEC_CFG_UPDT;
    //     }
    // }
    // else
    // {
    //     static uint8_t aucLogInfo[256];

    //     if (ENG_MODE == 0)
    //     {
    //         sprintf(aucLogInfo, "获取文件%s版本信息失败!\n",
    //                 EP_EDP_AUTOEXEC_FILE);
    //     }
    //     else if (ENG_MODE == 1)
    //     {
    //         sprintf(aucLogInfo, "Getting the version of file %s failed!\n",
    //                 EP_EDP_AUTOEXEC_FILE);
    //     }

    //     LOG_Write(LOG_OPRATE, aucLogInfo, NULL);

    //     return EP_ERROR;
    // }

#endif /* end of EDP_DYNAMICLOAD */



    if (!uiUpdtFg && FT_Is_File(strSysCfgFile))
    {
        /*若不需要更新SCI文件　2009-11-17 ZY  */
        /* 写入版本信息到系统日志文件 */
        SI_WrVerToExLog();
    }
    else
    {
        /*若需要重新生成SCI文件  */
        if (!uiUpdtFg)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM  |ER_LOCK| ER_REPORT,
                           "系统文件[%s]错误\n", (int)strSysCfgFile, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM  |ER_LOCK| ER_REPORT,
                           "File error(missing/corrupted):system file %s missing\n", (int)strSysCfgFile, 0);
            }
        }

        /* Create new system configuration. */
        SI_Creat_SCI(strSysCfgFile);

        /* HwCfgVer,SwCfgVer,LgcCfgVer三个ini文件的版本信息从SI_Chk_Cfg中挪到SI_Creat_SCI结束后进行,
         * 避免在算CRC，或生成SCI文件过程时关机，
         * 可能出现的旧SCI文件没有正常更新，并且以后也不会更新情况
         */

        if (uiUpdtFg&HW_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "HwCfgVer", aucHwNowCfgVer);
            assert (sts != EP_ERROR);
        }

        if (uiUpdtFg&SW_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "SwCfgVer", aucSwNowCfgVer);
            assert (sts != EP_ERROR);
        }

        if (uiUpdtFg&LGC_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "LgcCfgVer", aucLgNowCfgVer);
            assert (sts != EP_ERROR);
        }

        /* 更新edp01.ini文件 */
        if (uiUpdtFg & FUNC_OPT_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "FuncOptVer", aucFuncOptNowCfgVer);
            assert (sts != EP_ERROR);
        }

        if (uiUpdtFg&VX_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "VxCfgVer", aucVxNowCfgVer);
            assert (sts != EP_ERROR);
        }

        if (uiUpdtFg&VX_BOOTROM_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "BootromVer", aucBootromVer);
            assert (sts != EP_ERROR);
        }

#if defined(EDP_DYNAMICLOAD)
#ifdef EDP03_BUILD /* EDP03 platform */

#if defined(EDP03_STABCONTROL_BUILD) || defined(EDP03_LOWPROTECT_BUILD)
        if (uiUpdtFg&SYS_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "SysCfgVer", aucSysNowCfgVer);
            assert (sts != EP_ERROR);
        }

        if (uiUpdtFg&APP_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "AppCfgVer", aucAppNowCfgVer);
            assert (sts != EP_ERROR);
        }

        if (uiUpdtFg&USB_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "UsbCfgVer", aucUsbNowCfgVer);
            assert (sts != EP_ERROR);
        }

        if (uiUpdtFg&MMI_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "MmiCfgVer", aucMmiNowCfgVer);
            assert (sts != EP_ERROR);
        }
#endif	/* end of EDP03_STABCONTROL_BUILD || EDP03_LOWPROTECT_BUILD */

#if defined(EDP03_INTELBOX_BUILD)
        if (uiUpdtFg&SYS_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "SysCfgVer", aucSysNowCfgVer);
            assert (sts != EP_ERROR);
        }

        if (uiUpdtFg&APP_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "AppCfgVer", aucAppNowCfgVer);
            assert (sts != EP_ERROR);
        }
#endif		/* end of EDP03_INTELBOX_BUILD */

#endif	/* end of EDP03_BUILD */

#if defined(EDP_01_02_BUILD)
        if (uiUpdtFg&SYS_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "SysCfgVer", aucSysNowCfgVer);
            assert (sts != EP_ERROR);
        }

        if (uiUpdtFg&APP_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "AppCfgVer", aucAppNowCfgVer);
            assert (sts != EP_ERROR);
        }
#endif		/* end of EDP03_INTELBOX_BUILD */

#if defined(EXCITE_BUILD)
        if (uiUpdtFg&SYS_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "SysCfgVer", aucSysNowCfgVer);
            assert (sts != EP_ERROR);
        }

        if (uiUpdtFg&APP_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "AppCfgVer", aucAppNowCfgVer);
            assert (sts != EP_ERROR);
        }

        if (uiUpdtFg&MMI_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "MmiCfgVer", aucMmiNowCfgVer);
            assert (sts != EP_ERROR);
        }
#endif		/* end of EDP03_INTELBOX_BUILD */

        if (uiUpdtFg&AUTOEXEC_CFG_UPDT)
        {
            sts = FT_Wr_Sys_INI("[SYSTEM]", "AutoexecCfgVer", aucAutoexecNowCfgVer);
            assert (sts != EP_ERROR);
        }
#endif
    }

    /* 如果配置或vxWorks更新，或其它模块更改，则删除录波文件和事件文件 */
    if (uiUpdtFg)
    {
#ifdef EDP03_STABCONTROL_BUILD			/* 用于稳控装置 */
        DelEvtAndRecFile();
#endif
    }

    return EP_SUCCESS;
}


/***********************************************************************
* SI_Chk_Un_Ver - 校验version.ini中统一发布相关版本与实际程序是否一致
* RETURNS:
*         EP_SUCCESS, 正常
*         EP_ERROR, 错误
*/
EP_STATUS SI_Chk_Ver_INI()
{
    uint8_t aucRdCrc[FT_VER_INFO_LEN+1] = "";
    uint8_t aucRdVer[FT_VER_INFO_LEN+1] = "";
    uint16_t unCrc;
    uint16_t unRdCrc;
    uint16_t unVer;
    uint8_t aucActualVer[FT_VER_INFO_LEN+1] = "";
    int i;
    uint8_t ucFlag = 0;
    uint8_t aucPromtInfo[128] = "";

    UnVerInfo_g.bRelayCRCIsCoverCfgFlag = TRUE;   /* 默认保护和配置合并 */
    UnVerInfoRd_g.bRelayCRCIsCoverCfgFlag = TRUE;

    /* VER_UN_GetRelayCRC()函数依赖于该信息 */
    i = FT_Rd_Version_INI("[CONFIGURE]", "CRC", aucRdCrc, FT_VER_INFO_LEN+1);
    if (strlen(aucRdCrc)>0)
    {
        UnVerInfo_g.bRelayCRCIsCoverCfgFlag = FALSE;
        UnVerInfoRd_g.bRelayCRCIsCoverCfgFlag = FALSE;
    }

    unCrc = VER_GetCfgCRC();      /* hwcfg+swcfg+logic */
    UnVerInfo_g.unCfgCrc = unCrc;

    memset(aucActualVer, 0, sizeof(aucActualVer));
    unVer = GetLogicVer(); /* 获取逻辑图配置版本 */
    sprintf(aucActualVer, "%x.%02x", HI8(unVer), LO8(unVer));
    strncpy(UnVerInfo_g.aucCfgVer, aucActualVer, strlen(aucActualVer)+1);

    unCrc = VER_UN_GetPlatCRC();
    UnVerInfo_g.unPlatCrc = unCrc;

    memset(aucActualVer, 0, sizeof(aucActualVer));
    VER_UN_GetPlatVer(aucActualVer, sizeof(aucActualVer));
    strncpy(UnVerInfo_g.aucPlatVer, aucActualVer, strlen(aucActualVer)+1);

    if (UnVerInfo_g.unRelayCrc == 0)
    {
        /* 如果已生成实际CRC，则不需要再次生成 */
        unCrc = VER_UN_GetRelayCRC();
        UnVerInfo_g.unRelayCrc = unCrc;
    }

    memset(aucActualVer, 0, sizeof(aucActualVer));
    VER_UN_GetRelayPrioVer(aucActualVer, sizeof(aucActualVer));
    strncpy(UnVerInfo_g.aucRelayVer, aucActualVer, strlen(aucActualVer)+1);

    unCrc = VER_UN_GetFuncOptCRC();
    UnVerInfo_g.unFuncOptCrc = unCrc;

    unCrc = VER_UN_GetMmiCRC();
    UnVerInfo_g.unMmiCrc = unCrc;

#if (defined(EDP03_BUILD) || defined(EXCITE_BUILD)) && !defined(EDP03_INTELBOX_BUILD)
    memset(aucActualVer, 0, sizeof(aucActualVer));
    VER_UN_GetMmiVer(aucActualVer, sizeof(aucActualVer));
    strncpy(UnVerInfo_g.aucMmiVer, aucActualVer, strlen(aucActualVer)+1);
#endif
#ifndef EDP02_PSR_BUILD
    if (!FT_Is_File(EP_UN_VERSION_FILE))
    {
        if (ENG_MODE == 1)
            strcat(aucPromtInfo, "Not found version information file, please affirm software mudule information and create it.\n");
        else if (ENG_MODE == 0)
            strcat(aucPromtInfo, "未找到版本信息文件,请在HMI确认组件信息并生成.\n");
        LOG_Write(LOG_KERNEL, aucPromtInfo, NULL);

        /* 传统子单元版本更新不告警 */
#ifndef SUBUNIT
        ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_REPORT | ER_NOLOGWRITE, aucPromtInfo, 0, 0);  /* 不闭锁保护 */
#endif
        UnVerMatchedFlag_g = FALSE;

        return EP_SUCCESS;
    }
#endif
    i = FT_Rd_Version_INI("[CONFIGURE]", "CRC", aucRdCrc, FT_VER_INFO_LEN+1);
    if (strlen(aucRdCrc) == 0)
    {
        UnVerInfo_g.bRelayCRCIsCoverCfgFlag = TRUE;
        UnVerInfoRd_g.bRelayCRCIsCoverCfgFlag = TRUE;
    }

    /* 如果保护和配置分开发布, 才检测version.ini的[CONFIGURE] */
    if (!VER_UN_RelayCRCIsCoverCfg())
    {
        unCrc = UnVerInfo_g.unCfgCrc;      /* hwcfg+swcfg+logic */
        unRdCrc = strtol(aucRdCrc, NULL, 16);
        UnVerInfoRd_g.unCfgCrc = unRdCrc;
        if (unRdCrc != unCrc)
        {
            ucFlag |= 0x04;
            strcat(aucPromtInfo, "Configure, ");

            if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                           "relay protect configuration updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
            }
            else if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                           "保护配置更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
            }
        }

        i = FT_Rd_Version_INI("[CONFIGURE]", "VERSION", aucRdVer, FT_VER_INFO_LEN+1);
        if (i != 1)
        {
            LOG_Write(LOG_KERNEL, "版本校验:读取CONFIGURE/VERSION失败!\n", NULL);

            return EP_ERROR;
        }
        else if (strcmp(aucRdVer, UnVerInfo_g.aucCfgVer))
        {
            if (!(ucFlag&0x04))
            {
                strcat(aucPromtInfo, "Configure, ");
                ucFlag |= 0x04;
                if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                               "relay protect configuration updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                }
                else if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                               "保护配置更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                }
            }
        }
        strncpy(UnVerInfoRd_g.aucCfgVer, aucRdVer, strlen(aucRdVer)+1);
    }

    unCrc = UnVerInfo_g.unPlatCrc;
    if (unCrc)
    {
        i = FT_Rd_Version_INI("[PLATFORM]", "CRC", aucRdCrc, FT_VER_INFO_LEN+1);
        unRdCrc = strtol(aucRdCrc, NULL, 16);
        if (i != 1)
        {
            LOG_Write(LOG_KERNEL, "版本校验:读取PLATFORM/CRC失败!\n", NULL);

            return EP_ERROR;
        }
        else if (unRdCrc != unCrc)
        {
            ucFlag |= 0x01;
            strcat(aucPromtInfo, "Platform, ");

            if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                           "platform software updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
            }
            else if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                           "平台程序更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
            }
        }
        UnVerInfo_g.unPlatCrc = unCrc;
        UnVerInfoRd_g.unPlatCrc = unRdCrc;
    }

    if (UnVerInfo_g.aucPlatVer)
    {
        i = FT_Rd_Version_INI("[PLATFORM]", "VERSION", aucRdVer, FT_VER_INFO_LEN+1);
        if (i != 1)
        {
            LOG_Write(LOG_KERNEL, "版本校验:读取PLATFORM/VERSION失败!\n", NULL);

            return EP_ERROR;
        }
        else if (strcmp(aucRdVer, UnVerInfo_g.aucPlatVer))
        {
            if (!(ucFlag&0x01))
            {
                ucFlag |= 0x01;
                strcat(aucPromtInfo, "Platform, ");
                if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                               "platform software updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                }
                else if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                               "平台程序更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                }
            }
        }
        strncpy(UnVerInfoRd_g.aucPlatVer, aucRdVer, strlen(aucRdVer)+1);
    }
    else
    {
        LOG_Write(LOG_KERNEL, "版本校验:无效平台版本!\n", NULL);
        return EP_ERROR;
    }

    /* EDP01平台扩展机箱程序版本比较和记录 */
    if (bdType_g == BOARD_TYPE_E01)
    {
        if (UnVerInfo_g.acExtBspVer)
        {
            i = FT_Rd_Version_INI("[EXTBOX]", "BSPVERSION", aucRdVer, FT_VER_INFO_LEN+1);
            if (i >1)
            {
                LOG_Write(LOG_KERNEL, "版本校验:读取EXTBOX/BSPVERSION失败!\n", NULL);

                return EP_ERROR;
            }
            else if (strcmp(aucRdVer, UnVerInfo_g.acExtBspVer))
            {
                uiUpdtFg |=EXTMODVER_UPDT;
                LOG_Write(LOG_RUN, "系统配置信息发生改变.\n", NULL);
                if (!(ucFlag&0x01))
                {
                    ucFlag |= 0x01;
                    strcat(aucPromtInfo, "ExtBsp, ");
                    if (ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                                   "ExtBsp software updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                    }
                    else if (ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                                   "扩展机箱BSP程序更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                    }
                }
            }
            strncpy(UnVerInfoRd_g.acExtBspVer, aucRdVer, strlen(aucRdVer)+1);
        }

        if (UnVerInfo_g.acExtEdpVer)
        {
            i = FT_Rd_Version_INI("[EXTBOX]", "PLATFORMVERSION", aucRdVer, FT_VER_INFO_LEN+1);
            if (i > 1)
            {
                LOG_Write(LOG_KERNEL, "版本校验:读取EXTBOX/PLATFORMVERSION失败!\n", NULL);

                return EP_ERROR;
            }
            else if (strcmp(aucRdVer, UnVerInfo_g.acExtEdpVer))
            {
                uiUpdtFg |=EXTMODVER_UPDT;
                LOG_Write(LOG_RUN, "系统配置信息发生改变.\n", NULL);
                if (!(ucFlag&0x01))
                {
                    ucFlag |= 0x01;
                    strcat(aucPromtInfo, "ExtPlatform, ");
                    if (ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                                   "Extplatform software updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                    }
                    else if (ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                                   "扩展机箱平台程序更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                    }
                }
            }
            strncpy(UnVerInfoRd_g.acExtEdpVer, aucRdVer, strlen(aucRdVer)+1);
        }

        if (UnVerInfo_g.unExtEdpCrc)
        {
            i = FT_Rd_Version_INI("[EXTBOX]", "CRC", aucRdCrc, FT_VER_INFO_LEN+1);
            unRdCrc = strtol(aucRdCrc, NULL, 16);
            if (i > 1)
            {
                LOG_Write(LOG_KERNEL, "版本校验:读取EXTBOX/CRC失败!\n", NULL);

                return EP_ERROR;
            }
            else if (unRdCrc != UnVerInfo_g.unExtEdpCrc)
            {
                uiUpdtFg |=EXTMODVER_UPDT;
                LOG_Write(LOG_RUN, "系统配置信息发生改变.\n", NULL);
                if (!(ucFlag&0x01))
                {
                    ucFlag |= 0x01;
                    strcat(aucPromtInfo, "ExtPlatform, ");
                    if (ENG_MODE == 1)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                                   "ExtplatformCrc software updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                    }
                    else if (ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                                   "扩展机箱平台程序CRC更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                    }
                }
            }
            UnVerInfoRd_g.unExtEdpCrc = unRdCrc;
        }

        /*if (UnVerInfo_g.acExtEdpVer)
        {
            int index;
            uint8_t aucIOName[8];
            for (index=0;index<MAX_MOD_NUM;index++)
            {
                sprintf(aucIOName,"IO%02d",index+1);
                i = FT_Rd_Version_INI("[EXTBOX]", aucIOName, aucRdVer, FT_VER_INFO_LEN+1);
                if (i > 1)
                {
                    return EP_ERROR;
                }
                else if (strcmp(aucRdVer, UnVerInfo_g.aucExtIOVer[index]))
                {
                    uiUpdtFg |=EXTMODVER_UPDT;
        	        LOG_Write(LOG_RUN, "系统配置信息发生改变.\n", NULL);
                    if (!(ucFlag&0x01))
                    {
                        ucFlag |= 0x01;
                        strcat(aucPromtInfo, "ExtPlatform IO, ");
        		if (ENG_MODE == 1)
        		{
        		    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
        		        ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
        			"Extplatform IO software updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
        	        }
        		else if (ENG_MODE == 0)
        		{
        		    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
        			ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
        			"扩展机箱IO程序更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                        }
                    }
                }
                strncpy(UnVerInfoRd_g.aucExtIOVer[index], aucRdVer, strlen(aucRdVer)+1);
            }
        }*/
    }
    unCrc = UnVerInfo_g.unRelayCrc;
    if (unCrc)
    {
        i = FT_Rd_Version_INI("[RELAY]", "CRC", aucRdCrc, FT_VER_INFO_LEN+1);
        unRdCrc = strtol(aucRdCrc, NULL, 16);
        if (i != 1)
        {
            LOG_Write(LOG_KERNEL, "版本校验:读取RELAY/CRC失败!\n", NULL);

            return EP_ERROR;
        }
        else if (unRdCrc != unCrc)
        {
            ucFlag |= 0x02;
            strcat(aucPromtInfo, "Relay, ");
            if (ENG_MODE == 1)
            {
                if (UnVerInfo_g.bRelayCRCIsCoverCfgFlag)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                               "relay protect software/configuration updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                }
                else
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                               "relay protect software updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                }
            }
            else if (ENG_MODE == 0)
            {
                if (UnVerInfo_g.bRelayCRCIsCoverCfgFlag)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                               "保护程序/保护配置更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                }
                else
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                               "保护程序更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                }
            }
        }
        UnVerInfoRd_g.unRelayCrc = unRdCrc;
    }

    /* 优先逻辑图设置发布版本，否则edpapp.out提供 */
    if (UnVerInfo_g.aucRelayVer)
    {
        i = FT_Rd_Version_INI("[RELAY]", "VERSION", aucRdVer, FT_VER_INFO_LEN+1);
        if (i != 1)
        {
            LOG_Write(LOG_KERNEL, "版本校验:读取RELAY/VERSION失败!\n", NULL);

            return EP_ERROR;
        }
        else if (strcmp(aucRdVer, UnVerInfo_g.aucRelayVer))
        {
            if (!(ucFlag&0x02))
            {
                ucFlag |= 0x02;
                strcat(aucPromtInfo, "Relay, ");
                if (ENG_MODE == 1)
                {
                    if (UnVerInfo_g.bRelayCRCIsCoverCfgFlag)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                                   "relay protect software/configuration updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                    }
                    else
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                                   "relay protect software updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                    }
                }
                else if (ENG_MODE == 0)
                {
                    if (UnVerInfo_g.bRelayCRCIsCoverCfgFlag)
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                                   "保护程序/保护配置更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                    }
                    else
                    {
                        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                                   ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                                   "保护程序更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                    }
                }
            }
        }
        strncpy(UnVerInfoRd_g.aucRelayVer, aucRdVer, strlen(aucRdVer)+1);
    }
    else
    {
        LOG_Write(LOG_KERNEL, "版本校验:无效应用版本!\n", NULL);

        return EP_ERROR;
    }

    unCrc = UnVerInfo_g.unFuncOptCrc;
    if (unCrc)
    {
        i = FT_Rd_Version_INI("[FUNC]", "CRC", aucRdCrc, FT_VER_INFO_LEN+1);
        unRdCrc = strtol(aucRdCrc, NULL, 16);
        if (i != 1)
        {
            LOG_Write(LOG_KERNEL, "版本校验:读取FUNC/CRC失败!\n", NULL);

            return EP_ERROR;
        }
        else if (unRdCrc != unCrc)
        {
            ucFlag |= 0x01;
            strcat(aucPromtInfo, "FuncOpt, ");

            if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                           "FuncOpt.ini be updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
            }
            else if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                           "功能选配文件更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
            }
        }
        UnVerInfo_g.unFuncOptCrc = unCrc;
        UnVerInfoRd_g.unFuncOptCrc = unRdCrc;
    }

    unCrc = UnVerInfo_g.unMmiCrc;
    if (unCrc)
    {
        i = FT_Rd_Version_INI("[MMI]", "CRC", aucRdCrc, FT_VER_INFO_LEN+1);
        unRdCrc = strtol(aucRdCrc, NULL, 16);
        if (i != 1)
        {
            LOG_Write(LOG_KERNEL, "版本校验:读取MMI/CRC失败!\n", NULL);

            return EP_ERROR;
        }
        else if (unRdCrc != unCrc)
        {
            ucFlag |= 0x08;
            strcat(aucPromtInfo,"MU ");

            if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                           "MUCFG software configuration updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
            }
            else if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                           ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                           "MUCFG软件配置更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
            }
        }
        UnVerInfoRd_g.unMmiCrc = unRdCrc;
    }

#if (defined(EDP03_BUILD) || defined(EXCITE_BUILD)) && !defined(EDP03_INTELBOX_BUILD)
    if (UnVerInfo_g.aucMmiVer)
    {
        i = FT_Rd_Version_INI("[MMI]", "VERSION", aucRdVer, FT_VER_INFO_LEN+1);
        if (i != 1)
        {
            LOG_Write(LOG_KERNEL, "版本校验:读取MMI/VERSION失败!\n", NULL);

            return EP_ERROR;
        }
        else if (strcmp(aucRdVer, UnVerInfo_g.aucMmiVer))
        {
            if (!(ucFlag&0x08))
            {
                ucFlag |= 0x08;
                strcat(aucPromtInfo, "HMI, ");
                if (ENG_MODE == 1)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT | ER_VERSION | ER_NOLOGWRITE,
                               "HMI software updated(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                }
                else if (ENG_MODE == 0)
                {
                    ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                               ER_REPORT|ER_VERSION | ER_NOLOGWRITE,
                               "HMI程序更新(%02d).\n", NOT_MATCH_VERSION_FILE, 0);
                }
            }
        }
        strncpy(UnVerInfoRd_g.aucMmiVer, aucRdVer, strlen(aucRdVer)+1);
    }
    else
    {
        LOG_Write(LOG_KERNEL, "版本校验:无效MMI版本!\n", NULL);

        return EP_ERROR;
    }
#endif

    if (ucFlag)
    {
        if (ENG_MODE == 1)
        {
            strcat(aucPromtInfo, "info not matched with version.ini.\n");
        }
        else if (ENG_MODE == 0)
        {
            strcat(aucPromtInfo, "与版本信息文件version.ini不匹配.\n");
        }

        LOG_Write(LOG_KERNEL, aucPromtInfo, NULL);

        UnVerMatchedFlag_g = FALSE;
    }
    else
        UnVerMatchedFlag_g = TRUE;

    return EP_SUCCESS;
}

/***********************************************************************
* GetRelayComAttr - 获取保护组件属性
* RETURNS: NONE
*/
void GetRelayComAttr(void)
{
    uint8_t aucRdCrc[FT_VER_INFO_LEN+1] = "";
    int i;

    UnVerInfo_g.bRelayCRCIsCoverCfgFlag = TRUE;   /* 默认保护和配置合并 */
    UnVerInfoRd_g.bRelayCRCIsCoverCfgFlag = TRUE;

    /* VER_UN_GetRelayCRC()函数依赖于该信息 */
    i = FT_Rd_Version_INI("[CONFIGURE]", "CRC", aucRdCrc, FT_VER_INFO_LEN+1);
    if (strlen(aucRdCrc)>0)
    {
        UnVerInfo_g.bRelayCRCIsCoverCfgFlag = FALSE;
        UnVerInfoRd_g.bRelayCRCIsCoverCfgFlag = FALSE;
    }
}

/***********************************************************************
* ShowUNVer - 显示统一版本
* RETURNS: 无
*/
void ShowUNVer()
{
    LOG_Dbg_Msg("Plat: %s %x\n", (int)UnVerInfo_g.aucPlatVer, (int)UnVerInfo_g.unPlatCrc, 0, 0, 0, 0);
    LOG_Dbg_Msg("Rd Plat: %s %x\n", (int)UnVerInfoRd_g.aucPlatVer, (int)UnVerInfoRd_g.unPlatCrc, 0, 0, 0, 0);

    LOG_Dbg_Msg("Relay: %s %x\n", (int)UnVerInfo_g.aucRelayVer, (int)UnVerInfo_g.unRelayCrc, 0, 0, 0, 0);
    LOG_Dbg_Msg("Rd Relay: %s %x\n", (int)UnVerInfoRd_g.aucRelayVer, (int)UnVerInfoRd_g.unRelayCrc, 0, 0, 0, 0);
}

/***********************************************************************
* SI_Rst_Ver_INI - 将装置实际运行组件信息更新回version.ini中
* 注意，必须在SI_Chk_Ver_INI()之后调用
* RETURNS:
*       	   EP_SUCCESS, 正常
*              EP_ERROR, 错误
*/
EP_STATUS SI_Rst_Ver_INI()
{
    uint8_t aucCrc[FT_VER_INFO_LEN+1]="";
    EP_STATUS sts;
    uint8_t aucPromtInfo[256]="";
    uint8_t ucFlag=0;
    uint8_t aucRdCrc[FT_VER_INFO_LEN+1]="";
    uint8_t aucRdVer[FT_VER_INFO_LEN+1]="";
    uint16_t unRdCrc;
    int i;

    if(bSciChangedFlag_g)
    {
        LOG_Write(LOG_OPRATE, "系统信息配置改变告警被复归.\n", NULL);
    }

    assert(UnVerInfoRd_g.bRelayCRCIsCoverCfgFlag==UnVerInfo_g.bRelayCRCIsCoverCfgFlag);

    if(UnVerMatchedFlag_g)
        return EP_SUCCESS;

    if(!VER_UN_RelayCRCIsCoverCfg())        /*如果保护和配置分开发布,才回写version.ini的[CONFIGURE]*/
    {
        if(UnVerInfoRd_g.unCfgCrc!=UnVerInfo_g.unCfgCrc)
        {
            memset(aucCrc,0,sizeof(aucCrc));
            sprintf(aucCrc,"%04X",UnVerInfo_g.unCfgCrc);
            sts=FT_Wr_Version_INI("[CONFIGURE]","CRC",aucCrc);
            if(sts!=0&&sts!=1)
                return EP_ERROR;
            ucFlag |= 0x04;
            strcat(aucPromtInfo,"CONFIGURE CRC,");
        }
        if(strcmp(UnVerInfoRd_g.aucCfgVer,UnVerInfo_g.aucCfgVer))
        {
            sts=FT_Wr_Version_INI("[CONFIGURE]","VERSION",UnVerInfo_g.aucCfgVer);
            if(sts!=0&&sts!=1)
                return EP_ERROR;
            ucFlag |= 0x04;
            strcat(aucPromtInfo,"CONFIGURE Version,");
        }
    }

    if(UnVerInfoRd_g.unRelayCrc!=UnVerInfo_g.unRelayCrc)
    {
        memset(aucCrc,0,sizeof(aucCrc));
        sprintf(aucCrc,"%04X",UnVerInfo_g.unRelayCrc);
        sts=FT_Wr_Version_INI("[RELAY]","CRC",aucCrc);
        if(sts!=0&&sts!=1)
            return EP_ERROR;
        ucFlag |= 0x02;
        strcat(aucPromtInfo,"Relay CRC,");
    }

    if(strcmp(UnVerInfoRd_g.aucRelayVer,UnVerInfo_g.aucRelayVer))
    {
        sts=FT_Wr_Version_INI("[RELAY]","VERSION",UnVerInfo_g.aucRelayVer);
        logMsg("==%d\n",sts,0,0,0,0,0);
        if(sts!=0&&sts!=1)
            return EP_ERROR;

        ucFlag |= 0x02;
        strcat(aucPromtInfo,"Relay Version,");
    }

    if(UnVerInfoRd_g.unPlatCrc!=UnVerInfo_g.unPlatCrc)
    {
        memset(aucCrc,0,sizeof(aucCrc));
        sprintf(aucCrc,"%04X",UnVerInfo_g.unPlatCrc);
        sts=FT_Wr_Version_INI("[PLATFORM]","CRC",aucCrc);
        if(sts!=0&&sts!=1)
            return EP_ERROR;
        ucFlag |= 0x01;
        strcat(aucPromtInfo,"Platform CRC,");
    }

    if(strcmp(UnVerInfoRd_g.aucPlatVer,UnVerInfo_g.aucPlatVer))
    {
        sts=FT_Wr_Version_INI("[PLATFORM]","VERSION",UnVerInfo_g.aucPlatVer);
        if(sts!=0&&sts!=1)
            return EP_ERROR;
        ucFlag |= 0x01;
        strcat(aucPromtInfo,"Platform Version,");
    }

    /* 扩展机箱程序版本复位 */
    if (bdType_g == BOARD_TYPE_E01)
    {
        if(strcmp(UnVerInfoRd_g.acExtBspVer,UnVerInfo_g.acExtBspVer))
        {
            sts=FT_Wr_Version_INI("[EXTBOX]","BSPVERSION",UnVerInfo_g.acExtBspVer);
            if(sts!=0&&sts!=1)
                return EP_ERROR;
            ucFlag |= 0x01;
            strcat(aucPromtInfo,"ExtBsp Version,");
        }

        if(strcmp(UnVerInfoRd_g.acExtEdpVer,UnVerInfo_g.acExtEdpVer))
        {
            sts=FT_Wr_Version_INI("[EXTBOX]","PLATFORMVERSION",UnVerInfo_g.acExtEdpVer);
            if(sts!=0&&sts!=1)
                return EP_ERROR;
            ucFlag |= 0x01;
            strcat(aucPromtInfo,"ExtPlatform Version,");
        }

        /*{
            int index;
            uint8_t aucIOName[8];
            for (index=0;index<MAX_MOD_NUM;index++)
            {
                sprintf(aucIOName,"IO%02d",index+1);
                if(strcmp(UnVerInfoRd_g.aucExtIOVer[index],UnVerInfo_g.aucExtIOVer[index]))
                {
                    sts=FT_Wr_Version_INI("[EXTBOX]",aucIOName,UnVerInfo_g.aucExtIOVer[index]);
                    if(sts!=0&&sts!=1)
                        return EP_ERROR;
                    ucFlag |= 0x01;
                    strcat(aucPromtInfo,"ExtPlatform IO Version,");
                }
            }
        }*/

        if(UnVerInfoRd_g.unExtEdpCrc!=UnVerInfo_g.unExtEdpCrc)
        {
            memset(aucCrc,0,sizeof(aucCrc));
            sprintf(aucCrc,"%04X",UnVerInfo_g.unExtEdpCrc);
            sts=FT_Wr_Version_INI("[EXTBOX]","CRC",aucCrc);
            if(sts!=0&&sts!=1)
                return EP_ERROR;
            ucFlag |= 0x01;
            strcat(aucPromtInfo,"ExtPlatform CRC,");
        }
    }

    if(UnVerInfoRd_g.unFuncOptCrc!=UnVerInfo_g.unFuncOptCrc)
    {
        memset(aucCrc,0,sizeof(aucCrc));
        sprintf(aucCrc,"%04X",UnVerInfo_g.unFuncOptCrc);
        sts=FT_Wr_Version_INI("[FUNC]","CRC",aucCrc);
        if(sts!=0&&sts!=1)
            return EP_ERROR;
        ucFlag |= 0x01;
        strcat(aucPromtInfo,"FuncOpt CRC,");
    }

    if(UnVerInfoRd_g.unMmiCrc!=UnVerInfo_g.unMmiCrc)
    {
        memset(aucCrc,0,sizeof(aucCrc));
        sprintf(aucCrc,"%04X",UnVerInfo_g.unMmiCrc);
        sts=FT_Wr_Version_INI("[MMI]","CRC",aucCrc);
        if(sts!=0&&sts!=1)
            return EP_ERROR;
        ucFlag |= 0x08;
        strcat(aucPromtInfo,"MUCFG CRC,");
    }

    if(strcmp(UnVerInfoRd_g.aucMmiVer,UnVerInfo_g.aucMmiVer))
    {
        sts=FT_Wr_Version_INI("[MMI]","VERSION",UnVerInfo_g.aucMmiVer);
        if(sts!=0&&sts!=1)
            return EP_ERROR;
        ucFlag |= 0x08;
        strcat(aucPromtInfo,"HMI Version,");
    }

    if(ucFlag)
    {
        if(ENG_MODE==1)
            strcat(aucPromtInfo,"synchronized to version.ini.\n");
        else if(ENG_MODE==0)
            strcat(aucPromtInfo,"同步到版本信息文件version.ini.\n");
        UnVerMatchedFlag_g=TRUE;
        LOG_Write(LOG_OPRATE, aucPromtInfo, NULL);
        EP_Clr_Sts_Bit(VERSION_NOT_MATCHED_FLAG);
    }

    taskDelay(200);		/* 延迟2秒 */

    /* 更新读取结果 */
    i=FT_Rd_Version_INI("[CONFIGURE]", "CRC", aucRdCrc, FT_VER_INFO_LEN+1);
    if(strlen(aucRdCrc)==0)
    {
        UnVerInfo_g.bRelayCRCIsCoverCfgFlag=TRUE;
        UnVerInfoRd_g.bRelayCRCIsCoverCfgFlag=TRUE;
    }

    if(!VER_UN_RelayCRCIsCoverCfg())        /*如果保护和配置分开发布,才检测version.ini的[CONFIGURE]*/
    {
        unRdCrc=strtol(aucRdCrc, NULL, 16);
        UnVerInfoRd_g.unCfgCrc=unRdCrc;

        i=FT_Rd_Version_INI("[CONFIGURE]", "VERSION", aucRdVer, FT_VER_INFO_LEN+1);
        if(i!=1)
        {
            LOG_Dbg_Msg("[CONFIGURE] VERSION读取失败.\n", 0, 0, 0, 0, 0, 0);
            return EP_ERROR;
        }
        strncpy(UnVerInfoRd_g.aucCfgVer,aucRdVer,strlen(aucRdVer)+1);
    }

    if(UnVerInfo_g.unPlatCrc)
    {
        i=FT_Rd_Version_INI("[PLATFORM]", "CRC", aucRdCrc, FT_VER_INFO_LEN+1);
        unRdCrc=strtol(aucRdCrc, NULL, 16);
        if(i!=1)
        {
            LOG_Dbg_Msg("[PLATFORM] CRC读取失败.\n", 0, 0, 0, 0, 0, 0);
            return EP_ERROR;
        }
        UnVerInfoRd_g.unPlatCrc=unRdCrc;
    }

    if(UnVerInfo_g.aucPlatVer)
    {
        i=FT_Rd_Version_INI("[PLATFORM]", "VERSION", aucRdVer, FT_VER_INFO_LEN+1);
        if(i!=1)
        {
            LOG_Dbg_Msg("[PLATFORM] VERSION读取失败.\n", 0, 0, 0, 0, 0, 0);
            return EP_ERROR;
        }
        strncpy(UnVerInfoRd_g.aucPlatVer,aucRdVer,strlen(aucRdVer)+1);
    }
    else
        return EP_ERROR;

    if(UnVerInfo_g.unRelayCrc)
    {
        i=FT_Rd_Version_INI("[RELAY]", "CRC", aucRdCrc, FT_VER_INFO_LEN+1);
        unRdCrc=strtol(aucRdCrc, NULL, 16);
        if(i!=1)
        {
            LOG_Dbg_Msg("[RELAY] CRC读取失败.\n", 0, 0, 0, 0, 0, 0);
            return EP_ERROR;
        }
        UnVerInfoRd_g.unRelayCrc=unRdCrc;
    }

    if(UnVerInfo_g.aucRelayVer)   /*优先逻辑图设置发布版本，否则edpapp.out提供*/
    {
        i=FT_Rd_Version_INI("[RELAY]", "VERSION", aucRdVer, FT_VER_INFO_LEN+1);
        if(i!=1)
        {
            LOG_Dbg_Msg("[RELAY] VERSION读取失败.\n", 0, 0, 0, 0, 0, 0);
            return EP_ERROR;
        }
        strncpy(UnVerInfoRd_g.aucRelayVer,aucRdVer,strlen(aucRdVer)+1);
    }
    else
        return EP_ERROR;


    if(UnVerInfo_g.unMmiCrc)
    {
        i=FT_Rd_Version_INI("[MMI]", "CRC", aucRdCrc, FT_VER_INFO_LEN+1);
        unRdCrc=strtol(aucRdCrc, NULL, 16);
        if(i!=1)
        {
            LOG_Dbg_Msg("[MMI] CRC读取失败.\n", 0, 0, 0, 0, 0, 0);
            return EP_ERROR;
        }
        UnVerInfoRd_g.unMmiCrc=unRdCrc;
    }

#if (defined(EDP03_BUILD) || defined(EXCITE_BUILD)) && !defined(EDP03_INTELBOX_BUILD)
    if(UnVerInfo_g.aucMmiVer)
    {
        i=FT_Rd_Version_INI("[MMI]", "VERSION", aucRdVer, FT_VER_INFO_LEN+1);
        if(i!=1)
        {
            LOG_Dbg_Msg("[MMI] VERSION读取失败.\n", 0, 0, 0, 0, 0, 0);
            return EP_ERROR;
        }
        strncpy(UnVerInfoRd_g.aucMmiVer,aucRdVer,strlen(aucRdVer)+1);
    }
    else
        return EP_ERROR;
#endif

    /* 扩展机箱程序版本复位 */
    if (bdType_g == BOARD_TYPE_E01)
    {
        if ((bdType_g == BOARD_TYPE_E01) && UnVerInfo_g.acExtBspVer)
        {
            i=FT_Rd_Version_INI("[EXTBOX]", "BSPVERSION", aucRdVer, FT_VER_INFO_LEN+1);
            if(i!=1)
            {
                LOG_Dbg_Msg("[EXTBOX] BSPVERSION读取失败.\n", 0, 0, 0, 0, 0, 0);
                return EP_ERROR;
            }
            strncpy(UnVerInfoRd_g.acExtBspVer,aucRdVer,strlen(aucRdVer)+1);
        }

        if(UnVerInfo_g.acExtEdpVer)
        {
            i=FT_Rd_Version_INI("[EXTBOX]", "PLATFORMVERSION", aucRdVer, FT_VER_INFO_LEN+1);
            if(i!=1)
            {
                LOG_Dbg_Msg("[EXTBOX] PLATFORMVERSION读取失败.\n", 0, 0, 0, 0, 0, 0);
                return EP_ERROR;
            }
            strncpy(UnVerInfoRd_g.acExtEdpVer,aucRdVer,strlen(aucRdVer)+1);
        }

        /*{
            int index;
            uint8_t aucIOName[8];
            for (index=0;index<MAX_MOD_NUM;index++)
            {
                if(UnVerInfo_g.aucExtIOVer[index])
                {
                    sprintf(aucIOName,"IO%02d",index+1);
                    i=FT_Rd_Version_INI("[EXTBOX]", aucIOName, aucRdVer, FT_VER_INFO_LEN+1);
                    if(i!=1)
                    {
                        LOG_Dbg_Msg("[EXTBOX] IO VERSION读取失败.\n", 0, 0, 0, 0, 0, 0);
                        return EP_ERROR;
                    }
                    strncpy(UnVerInfoRd_g.aucExtIOVer[index],aucRdVer,strlen(aucRdVer)+1);
                }
            }
        }*/

        if(UnVerInfo_g.unExtEdpCrc)
        {
            i=FT_Rd_Version_INI("[EXTBOX]", "CRC", aucRdCrc, FT_VER_INFO_LEN+1);
            unRdCrc=strtol(aucRdCrc, NULL, 16);
            if(i!=1)
            {
                LOG_Dbg_Msg("[EXTBOX] PLATFORMCRC读取失败.\n", 0, 0, 0, 0, 0, 0);
                return EP_ERROR;
            }
            UnVerInfoRd_g.unExtEdpCrc=unRdCrc;
        }
    }

    LOG_Dbg_Msg("读取Version.ini文件成功!\n", 0, 0, 0, 0, 0, 0);

    return EP_SUCCESS;

}

/*
2	BOOTROM的CRC校验码（若BOOTROM和BSP一体，则两者CRC校验码一致）
2	BSP的CRC校验码
2	平台软件的CRC校验码
2	保护程序的CRC校验码（指保护算法软件）
2	保护配置的CRC校验码（包括逻辑图和软硬件配置，逻辑图中设置）
2	保护应用软件的发布CRC校验码（包括保护程序，或（还包括软硬件配置和逻辑图）生成的总校验码，或者是逻辑图配置的虚拟发布CRC）
2	总的CRC校验码（包括BOOTROM,BSP,平台软件，保护程序，保护配置，MMI软件（若有的话，比如edp03,CPU板和MMI板是同一块板））
2	MMI程序的CRC校验码（若有的话，比如edp03,CPU板和MMI板是同一块板）
2	平台软件的特征码
124	保留
1	CPU硬件版本号字符串长度CPU_HW_VER_LEN
CPU_HW_VER_LEN	CPU硬件版本号CPU_HW_VER字符串，比如"EDP01CPUA-C"
1	BOOTROM版本号字符串长度BOOTROM_VER_LEN（若BOOTROM和BSP一体，则两者版本号一致）
BOOTROM_VER_LEN	BOOTROM版本号BOOTROM_VER字符串
1	BSP版本号字符串长度BSP_VER_LEN
BSP_VER_LEN	BSP版本号BSP_VER字符串

1	平台软件版本号字符串长度PLAT_VER_LEN
PLAT_VER_LEN	平台软件版本号PLAT_VER字符串
1	保护程序版本号字符串长度RELAY_VER_LEN（指保护算法软件）
RELAY_VER_LEN	保护程序版本号RELAY_VER字符串
1	保护配置版本号字符串长度RELAY_CFG_VER_LEN（包括软硬件配置和逻辑图，逻辑图中设置）
RELAY_CFG_VER_LEN	保护配置版本号RELAY_CFG_VER字符串
1	MMI版本号字符串长度MMI_VER_LEN(若CPU未包括MMI软件，则该字符串长度为0)
MMI_VER_LEN	MMI软件版本号MMI_VER字符串

1	保留字符串2长度RsvStr2Len
RsvStr2Len	保留字符串RsvStr2
1	保留字符串3长度RsvStr3Len
RsvStr3Len	保留字符串RsvStr3
1	保留字符串4长度RsvStr4Len
RsvStr4Len	保留字符串RsvStr4
1	保留字符串5长度RsvStr5Len
RsvStr5Len	保留字符串RsvStr5
1	保留字符串6长度RsvStr6Len
RsvStr6Len	保留字符串RsvStr6
1	保留字符串7长度RsvStr7Len
RsvStr7Len	保留字符串RsvStr7
1	保留字符串8长度RsvStr8Len
RsvStr8Len	保留字符串RsvStr8
1	保留字符串9长度RsvStr9Len
RsvStr9Len	保留字符串RsvStr9
1	保留字符串10长度RsvStr10Len
RsvStr10Len	保留字符串RsvStr10
*/
static void SI_Wr_Ext_Seg(int iFd)
{
    uint8_t aucBuf[6144]="";
    uint8_t aucVer[32]="";
    uint8_t aucStep[4096]=""; /* 定值步长修改内容 */
    uint32_t ulLen=0;
    uint16_t unCrc=0;
    int i,j;
    uint8_t *puc;
    uint8_t *pSetStep;
    uint8_t aucExLog[256]="";/* 2009-11-17 ZY */
    SC_SET_PAGE *psetpg;
    SC_SET_ITEM *pset;

    assert(iFd>=0);

    puc=aucBuf;
    pSetStep=aucStep;

    puc+=2;

    *puc++=0x00;
    *puc++=0x00;/*BOOTROM的CRC校验码*/
    *puc++=0x00;
    *puc++=0x00;/*BSP的CRC校验码*/
    *puc++=LO8(SI_SysVer_g.unEdpSwCRC);
    *puc++=HI8(SI_SysVer_g.unEdpSwCRC);

    /* 写入平台软件版本信息到系统日志　2009-11-17 ZY */
    memset(aucVer,0,sizeof(aucVer));
    VER_UN_GetPlatVer(aucVer,sizeof(aucVer));
    sprintf(aucExLog,"%s,%04x",aucVer,SI_SysVer_g.unEdpSwCRC);
    LOG_ExtraItemWrite("平台软件版本",aucExLog);


    unCrc=VER_GetUsrSwCRC();/*edpapp.out*/
    *puc++=LO8(unCrc);
    *puc++=HI8(unCrc);

    /* 写入保护程序版本信息到系统日志　2009-11-17 ZY */
    memset(aucVer,0,sizeof(aucVer));
    VER_UN_GetRelayVer(aucVer,sizeof(aucVer));
    sprintf(aucExLog,"%s,%04x",aucVer,unCrc);
    LOG_ExtraItemWrite("保护程序版本",aucExLog);

    unCrc=VER_GetCfgCRC();  /*hwcfg swcfg logic*/
    *puc++=LO8(unCrc);
    *puc++=HI8(unCrc);


    /* 写入保护配置版本信息到系统日志　2009-11-17 ZY */
    sprintf(aucExLog,"%x.%02x,%04x",HI8(SI_SysVer_g.unLogicVer),LO8(SI_SysVer_g.unLogicVer),unCrc);
    LOG_ExtraItemWrite("保护配置版本",aucExLog);

    if(SI_SysVer_g.unRlsCRC==0)
    {
        unCrc=VER_UN_GetRelayCRC();
        UnVerInfo_g.unRelayCrc=unCrc;
    }
    else
        unCrc=SI_SysVer_g.unRlsCRC;
    *puc++=LO8(unCrc); /*保护应用软件的发布CRC校验码（优先逻辑图配置的虚拟发布CRC,要么是保护组件的CRC或是）（供MMI显示版本简明信息的发布校验码）*/
    *puc++=HI8(unCrc);

    /* 写入应用软件发布版本信息到系统日志　2009-11-17 ZY */
    memset(aucVer,0,sizeof(aucVer));
    VER_UN_GetRelayPrioVer(aucVer,sizeof(aucVer));
    sprintf(aucExLog,"%s,%04x",aucVer,unCrc);
    LOG_ExtraItemWrite("应用软件发布版本",aucExLog);

    unCrc=VER_GetTotalCRC();
    *puc++=LO8(unCrc);
    *puc++=HI8(unCrc);
    unCrc=VER_UN_GetMmiCRC();
    *puc++=LO8(unCrc);
    *puc++=HI8(unCrc);

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
    /* 写入mmi软件版本信息到系统日志　2009-11-17 ZY */
    memset(aucVer,0,sizeof(aucVer));
    VER_UN_GetMmiVer(aucVer,sizeof(aucVer));
    sprintf(aucExLog,"%s,%04x",aucVer,unCrc);
    LOG_ExtraItemWrite("MMI软件版本",aucExLog);
#endif

    *puc++=LO8(VER_UN_GetPlatLabel());
    *puc++=HI8(VER_UN_GetPlatLabel());
    puc+=124; /*保留*/
    if(VER_GetHwBoardSN()>0)
    {
        ulLen=strlen(aHwVerMapArr[VER_GetHwBoardSN()-1].aucHwVerID);
        *puc++=ulLen;
        assert((ulLen+puc-aucBuf)<sizeof(aucBuf));
        for(i=0; i<ulLen; i++)
            *puc++=*(aHwVerMapArr[VER_GetHwBoardSN()-1].aucHwVerID+i);

        /* 写入硬件版本信息到系统日志　2009-11-17 ZY */
        LOG_ExtraItemWrite("硬件版本",aHwVerMapArr[VER_GetHwBoardSN()-1].aucHwVerID);
    }
    else
        *puc++=0;

    memset(aucVer,0,sizeof(aucVer));
    sprintf(aucVer,"%x.%x%s",HI8(GetBootromVer()),LO8(GetBootromVer()), aucsysBspVer);
    ulLen=strlen(aucVer);
    *puc++=ulLen;
    assert((ulLen+puc-aucBuf)<sizeof(aucBuf));
    for(i=0; i<ulLen; i++)
        *puc++=aucVer[i];

    memset(aucVer,0,sizeof(aucVer));
    sprintf(aucVer,"%x.%x%s",HI8(GetBspVer()),LO8(GetBspVer()), aucsysBspVer);
    ulLen=strlen(aucVer);
    *puc++=ulLen;
    assert((ulLen+puc-aucBuf)<sizeof(aucBuf));
    for(i=0; i<ulLen; i++)
        *puc++=aucVer[i];


    /* 写入BSP版本信息到系统日志　2009-11-17 ZY */
    LOG_ExtraItemWrite("BSP版本",aucVer);
    memset(aucVer,0,sizeof(aucVer));
    if(VER_UN_GetPlatVer(aucVer,sizeof(aucVer)))    /*平台版本号*/
    {
        ulLen=strlen(aucVer);
        *puc++=ulLen;
        assert((ulLen+puc-aucBuf)<sizeof(aucBuf));
        for(i=0; i<ulLen; i++)
            *puc++=aucVer[i];
    }
    else
        *puc++=0;

    memset(aucVer,0,sizeof(aucVer));
    if(VER_UN_GetRelayVer(aucVer,sizeof(aucVer)))    /*保护程序版本号*/
    {
        ulLen=strlen(aucVer);
        *puc++=ulLen;
        assert((ulLen+puc-aucBuf)<sizeof(aucBuf));
        for(i=0; i<ulLen; i++)
            *puc++=aucVer[i];
    }
    else
        *puc++=0;

    memset(aucVer,0,sizeof(aucVer));
    {
        /*逻辑图配置版本号*/
        sprintf(aucVer,"%x.%02x",HI8(SI_SysVer_g.unLogicVer),LO8(SI_SysVer_g.unLogicVer));
        ulLen=strlen(aucVer);
        *puc++=ulLen;
        assert((ulLen+puc-aucBuf)<sizeof(aucBuf));
        for(i=0; i<ulLen; i++)
            *puc++=aucVer[i];
    }

    assert((10+puc-aucBuf)<sizeof(aucBuf));

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
    memset(aucVer,0,sizeof(aucVer));
    if(VER_UN_GetMmiVer(aucVer,sizeof(aucVer)))    /*保护程序版本号*/
    {
        ulLen=strlen(aucVer);
        *puc++=ulLen;
        assert((ulLen+puc-aucBuf)<sizeof(aucBuf));
        for(i=0; i<ulLen; i++)
            *puc++=aucVer[i];
    }
    else
        *puc++=0;
#else
    *puc++=0;       /*MMI版本号暂时为0*/
#endif

    memset(aucVer,0,sizeof(aucVer));
    {
        /*内部规约版本号*/
        sprintf(aucVer,"%x.%02x",HI8(GetInnerProtocolVer()),LO8(GetInnerProtocolVer()));
        ulLen=strlen(aucVer);
        *puc++=ulLen;
        assert((ulLen+puc-aucBuf)<sizeof(aucBuf));
        for(i=0; i<ulLen; i++)
            *puc++=aucVer[i];


        /* 写入内部规约版本信息到系统日志　2009-11-17 ZY */
        LOG_ExtraItemWrite("内部规约版本",aucVer);
    }

    memset(aucVer,0,sizeof(aucVer));
    if(VER_UN_GetRelayPrioVer(aucVer,sizeof(aucVer)))    /*保护发布版本号,逻辑图设置保护发布版本号优先*/
    {
        ulLen=strlen(aucVer);
        *puc++=ulLen;
        assert((ulLen+puc-aucBuf)<sizeof(aucBuf));
        for(i=0; i<ulLen; i++)
            *puc++=aucVer[i];
    }
    else
        *puc++=0;

    memset(aucVer,0,sizeof(aucVer));
    {
        /*硬件配置版本号*/
        sprintf(aucVer,"%x.%02x",HI8(SI_SysVer_g.unHdCfgVer),LO8(SI_SysVer_g.unHdCfgVer));
        ulLen=strlen(aucVer);
        *puc++=ulLen;
        assert((ulLen+puc-aucBuf)<sizeof(aucBuf));
        for(i=0; i<ulLen; i++)
            *puc++=aucVer[i];

        /* 写入硬件配置版本信息到系统日志　2009-11-17 ZY */
        LOG_ExtraItemWrite("硬件配置版本",aucVer);
    }
    memset(aucVer,0,sizeof(aucVer));
    {
        /*软件配置版本号*/
        sprintf(aucVer,"%x.%02x",HI8(SI_SysVer_g.unSwCfgVer),LO8(SI_SysVer_g.unSwCfgVer));
        ulLen=strlen(aucVer);
        *puc++=ulLen;
        assert((ulLen+puc-aucBuf)<sizeof(aucBuf));
        for(i=0; i<ulLen; i++)
            *puc++=aucVer[i];

        /* 写入软件配置版本信息到系统日志　2009-11-17 ZY */
        LOG_ExtraItemWrite("软件配置版本",aucVer);
    }

    /* 扩展机箱程序版本传递至MMI */
    if (bdType_g != BOARD_TYPE_E01)
    {
        *puc++=0;
    }
    else
    {
        /*扩展机箱版本号等信息*/
        extern char acExtHwVer[32];    /*扩展机箱硬件版本号*/
        //extern uint8_t ucExtIoSum;    /*IO模块数量*/
        //extern uint8_t aExtSubModInfo[MAX_MOD_NUM*5];/* 最多支持MAX_MOD_NUM个模件 */
        uint16_t unExtInfoLen;
        uint8_t *pucTmp=puc; /*暂存长度字节指针*/
        puc++;

        ulLen=strlen(acExtHwVer);
        assert((ulLen+1+puc-aucBuf)<sizeof(aucBuf));
        *puc++=ulLen;
        for(i=0; i<ulLen; i++)
            *puc++=acExtHwVer[i];
        unExtInfoLen=ulLen+1;

        ulLen=strlen(UnVerInfo_g.acExtBspVer);
        assert((ulLen+1+puc-aucBuf)<sizeof(aucBuf));
        *puc++=ulLen;
        for(i=0; i<ulLen; i++)
            *puc++=UnVerInfo_g.acExtBspVer[i];
        unExtInfoLen=unExtInfoLen+ulLen+1;

        ulLen=strlen(UnVerInfo_g.acExtEdpVer);
        assert((ulLen+1+puc-aucBuf)<sizeof(aucBuf));
        *puc++=ulLen;
        for(i=0; i<ulLen; i++)
            *puc++=UnVerInfo_g.acExtEdpVer[i];
        unExtInfoLen=unExtInfoLen+ulLen+1;

        assert((2+puc-aucBuf)<sizeof(aucBuf));
        *puc++=LO8(UnVerInfo_g.unExtEdpCrc);
        *puc++=HI8(UnVerInfo_g.unExtEdpCrc);
        unExtInfoLen=unExtInfoLen+2;

        /*
        assert((1+ucExtIoSum*5+puc-aucBuf)<sizeof(aucBuf));
        *puc++=ucExtIoSum;
        memcpy(puc,aExtSubModInfo,sizeof(uint8_t)*ucExtIoSum*5);
        puc+=ucExtIoSum*5;
        unExtInfoLen=unExtInfoLen+ucExtIoSum*5+1;
        */

        *pucTmp=unExtInfoLen;
    }

    for(i=0; i<4; i++)  /*保留字符串为空*/
        *puc++=0;
    assert((puc-aucBuf)<sizeof(aucBuf));

    /* 定值配置修改步长 */
    for(i=0; i<4; i++)  /*保留字符串为空*/
    {
        *pSetStep++=0;
    }
    *pSetStep++ = iSetPgNum_g;
    for(i=1; i<iSetPgNum_g+1; i++)
    {
        if(i<iSetPgNum_g)
        {
            /* 保护定值 */
            psetpg = psetpg_g+i;

            *pSetStep++ = i;  /* 该页定值在定值集中的页号 */

            for(j=0; j<4; j++)  /*保留字符串为空*/
            {
                *pSetStep++=0;
            }

            *pSetStep++ = LO8(psetpg->iSetNum);
            *pSetStep++ = HI8(psetpg->iSetNum);

            pset = psetpg->pset;
            for(j=0; j<psetpg->iSetNum; j++)
            {
                /* 该页第j个定值在该页定值集中的序号 */
                *pSetStep++ = LO8(j);
                *pSetStep++ = HI8(j);

                /* 保留3 个字节 */
                *pSetStep++ =0;
                *pSetStep++ =0;
                *pSetStep++ =0;

                /* 该定值修改步长 */
                U32_TO_BYTES(pSetStep, pset->valStep.ulVal);
                pSetStep = pSetStep+4;

                pset++;
            }
        }
        else
        {
            /* 参数定值 */
            *pSetStep++ = i;  /* 该页定值在定值集中的页号 */

            for(j=0; j<4; j++)  /*保留字符串为空*/
            {
                *pSetStep++=0;
            }

            *pSetStep++ = LO8(iCkSetNum_g);
            *pSetStep++ = HI8(iCkSetNum_g);

            pset = pCkset_g;
            for(j=0; j<iCkSetNum_g; j++)
            {
                /* 该页第j个定值在该页定值集中的序号 */
                *pSetStep++ = LO8(j);
                *pSetStep++ = HI8(j);

                /* 保留3 个字节 */
                *pSetStep++ =0;
                *pSetStep++ =0;
                *pSetStep++ =0;

                /* 该定值修改步长 */
                U32_TO_BYTES(pSetStep, pset->valStep.ulVal);
                pSetStep = pSetStep+4;

                pset++;
            }
        }
    }

    ulLen=pSetStep-aucStep;
    assert(ulLen<=sizeof(aucStep));

    *puc++=LL8(ulLen);
    *puc++=LH8(ulLen);
    *puc++=HL8(ulLen);
    *puc++=HH8(ulLen);

    assert((ulLen+puc-aucBuf)<sizeof(aucBuf));
    for(i=0; i<ulLen; i++)
        *puc++=aucStep[i];

    aucBuf[0]=LO8((puc-aucBuf-2));
    aucBuf[1]=HI8((puc-aucBuf-2));

    i=write(iFd, aucBuf, (puc-aucBuf));
    assert(i==(puc-aucBuf));

}


/*功能，记录SCI的相关版本信息到系统日志文件中
  参数，无
  返回，无
  注意: ,只有当SCI文件不更新时,才调用 2009-11-17 ZY*/
void  SI_WrVerToExLog()
{
    /*  */

    uint8_t aucVer[32]="";
    uint16_t unCrc=0;
    uint8_t aucExLog[256]="";


    /* 写入平台软件版本信息到系统日志　 */
    memset(aucVer,0,sizeof(aucVer));
    VER_UN_GetPlatVer(aucVer,sizeof(aucVer));
    sprintf(aucExLog,"%s,%04x",aucVer,VER_GetEdpSwCRC());
    LOG_ExtraItemWrite("平台软件版本",aucExLog);

    /* 写入保护程序版本信息到系统日志　 */
    unCrc=VER_GetUsrSwCRC();/*edpapp.out*/
    memset(aucVer,0,sizeof(aucVer));
    VER_UN_GetRelayVer(aucVer,sizeof(aucVer));
    sprintf(aucExLog,"%s,%04x",aucVer,unCrc);
    LOG_ExtraItemWrite("保护程序版本",aucExLog);

    /* 写入保护配置版本信息到系统日志　*/
    unCrc=VER_GetCfgCRC();
    sprintf(aucExLog,"%x.%02x,%04x",HI8(SI_SysVer_g.unLogicVer),LO8(SI_SysVer_g.unLogicVer),unCrc);
    LOG_ExtraItemWrite("保护配置版本",aucExLog);

    /* 写入应用软件发布版本信息到系统日志　 */
    if(SI_SysVer_g.unRlsCRC==0)
    {
        unCrc=VER_UN_GetRelayCRC();
        UnVerInfo_g.unRelayCrc=unCrc;
    }
    else
        unCrc=SI_SysVer_g.unRlsCRC;
    memset(aucVer,0,sizeof(aucVer));
    VER_UN_GetRelayPrioVer(aucVer,sizeof(aucVer));
    sprintf(aucExLog,"%s,%04x",aucVer,unCrc);
    LOG_ExtraItemWrite("应用软件发布版本",aucExLog);

#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
    /* 写入mmi软件版本信息到系统日志　 */
    unCrc=VER_UN_GetMmiCRC();
    memset(aucVer,0,sizeof(aucVer));
    VER_UN_GetMmiVer(aucVer,sizeof(aucVer));
    sprintf(aucExLog,"%s,%04x",aucVer,unCrc);
    LOG_ExtraItemWrite("MMI软件版本",aucExLog);
#endif

    /* 写入硬件版本信息到系统日志　*/
    if(VER_GetHwBoardSN()>0)
    {
        LOG_ExtraItemWrite("硬件版本",aHwVerMapArr[VER_GetHwBoardSN()-1].aucHwVerID);
    }

    /* 写入BSP版本信息到系统日志　 */
    sprintf(aucVer,"%x.%x%s",HI8(GetBspVer()),LO8(GetBspVer()), aucsysBspVer);
    LOG_ExtraItemWrite("BSP版本",aucVer);

    /* 写入内部规约版本信息到系统日志　 */
    sprintf(aucVer,"%x.%02x",HI8(GetInnerProtocolVer()),LO8(GetInnerProtocolVer()));
    LOG_ExtraItemWrite("内部规约版本",aucVer);

    /* 写入硬件配置版本信息到系统日志　 */
    sprintf(aucVer,"%x.%02x",HI8(SI_SysVer_g.unHdCfgVer),LO8(SI_SysVer_g.unHdCfgVer));
    LOG_ExtraItemWrite("硬件配置版本",aucVer);

    /* 写入软件配置版本信息到系统日志　 */
    sprintf(aucVer,"%x.%02x",HI8(SI_SysVer_g.unSwCfgVer),LO8(SI_SysVer_g.unSwCfgVer));
    LOG_ExtraItemWrite("软件配置版本",aucVer);

}


/***********************************************************************
* SI_Creat_SCI - 生成系统文件
*
* RETURNS: 无
*
*/
void SI_Creat_SCI(
    const uint8_t *strSysCfgFile)
{
    int iFd;
    uint8_t aucBuf[200];
    EP_STATUS sts;
    int i;
    uint8_t   *puc;

    iFd=FT_Bgn_Update(strSysCfgFile);
    assert(iFd>=0);

    aucBuf[0]=0x66;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x99;
    aucBuf[4]=0x02; /*文件格式字, 文件尾附有扩展信息字段内容字段*/
    aucBuf[5]=0;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    /* TODO: where to get CPU name? */
    aucBuf[9]=8;
    aucBuf[10]='E';
    aucBuf[11]='D';
    aucBuf[12]='P';
    aucBuf[13]='0';
    aucBuf[14]='1';
    aucBuf[15]='-';
    aucBuf[16]='0';
    aucBuf[17]='4';

    /* 该字段信息全部放到扩展字段中表述. */
    aucBuf[18]=LO8(SI_SysVer_g.unCfgProtocolVer);
    aucBuf[19]=HI8(SI_SysVer_g.unCfgProtocolVer);
    aucBuf[20]=LO8(GetInnerProtocolVer());
    aucBuf[21]=HI8(GetInnerProtocolVer());
    aucBuf[22]=0x00;
    aucBuf[23]=0x00;
    aucBuf[24]=0x00;
    aucBuf[25]=0x00;
    aucBuf[26]=0x00;
    aucBuf[27]=0x00;
    aucBuf[28]=0x00;
    aucBuf[29]=0x00;
    aucBuf[30]=0x00;
    aucBuf[31]=0x00;
    aucBuf[32]=0x00;
    aucBuf[33]=0x00;
    aucBuf[34]=0x00;
    aucBuf[35]=0x00;
    aucBuf[36]=0x00;
    aucBuf[37]=0x00;
    aucBuf[38]=0x00;
    aucBuf[39]=0x00;

    SI_SysVer_g.unEdpSwCRC=VER_GetEdpSwCRC();
    aucBuf[40]=0x00;//LO8(SI_SysVer_g.unEdpSwCRC);
    aucBuf[41]=0x00;//HI8(SI_SysVer_g.unEdpSwCRC);

    if(SI_SysVer_g.unRlsCRC==0)
    {
        SI_SysVer_g.unActualRlsCRC=GetSysCRC();
    }
    else
    {
        SI_SysVer_g.unActualRlsCRC=SI_SysVer_g.unRlsCRC;
    }
    aucBuf[42]=LO8(SI_SysVer_g.unActualRlsCRC);
    aucBuf[43]=HI8(SI_SysVer_g.unActualRlsCRC);

    i=write(iFd, aucBuf, 44);
    assert(i==44);

    aucBuf[0]=strlen(SI_SysVer_g.aucSysVer);
    puc=aucBuf+1;
    memcpy(puc, SI_SysVer_g.aucSysVer, aucBuf[0]);
    i=write(iFd, aucBuf, aucBuf[0]+1);
    assert(i==aucBuf[0]+1);

    aucBuf[0]=0;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0;
    aucBuf[4]=0;
    aucBuf[5]=0;


    /* Get equipment function. */
    i=0x0C;
    if (iMeaAiNum_g)
        i |= 0x01;
    if (iMeaDiNum_g)
        i |= 0x02;

    aucBuf[6]=i;

    /* Total number of sub-configurations.
     * Channel of the sub-configuration may be 0, but it must contain header. */
    aucBuf[7]=18;/*添加条目后需要修改  */

    i=write(iFd, aucBuf, 8);
    assert(i==8);

    /* Protect function config. */
    SI_Wr_Prtc_Cfg(iFd);

    /* Hardware AI config. */
    SI_Wr_Hw_AI_Cfg(iFd);

    /* DI config. */
    SI_Wr_DI_Cfg(iFd);

    /* DO config. */
    SI_Wr_DO_Cfg(iFd);

    /* Setting config. */
    SI_Wr_Set_Cfg(iFd);

    /* Link config. */
    SI_Wr_Link_Cfg(iFd);

    /* Record AI config. */
    SI_Wr_Rec_AI_Cfg(iFd);

    /* Record DI config. */
    SI_Wr_Rec_DI_Cfg(iFd);

    /* Event config. */
    SI_Wr_Evt_Cfg(iFd);

    /* Record flag config. */
    SI_Wr_Flag_Cfg(iFd);

    /* Measurement AI config. */
    SI_Wr_Mea_AI_Cfg(iFd);

    /* Measurement DI config. */
    SI_Wr_Mea_DI_Cfg(iFd);

    /* PO config. */
    SI_Wr_PO_Cfg(iFd);

    /* Measurement DO config. */
    SI_Wr_Mea_DO_Cfg(iFd);

    /* Hardware LED config. */
    SI_Wr_Hw_Led_Cfg(iFd);

    /* Software LED config. */
    SI_Wr_Sw_Led_Cfg(iFd);

    /* Measure value config*/
    SI_Wr_Msu_Cfg(iFd);

    /* PI config.*/
    SI_Wr_PI_Cfg(iFd);

    aucBuf[0]=0x62;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x9D;

    i=write(iFd, aucBuf, 4);
    assert(i==4);

    SI_Wr_Ext_Seg(iFd);

    sts=FT_End_Update(strSysCfgFile, iFd);
    assert(sts==EP_SUCCESS);

    if(ENG_MODE == 0)
    {
        LOG_Write(LOG_OPRATE, "创建新的系统SCI文件.\n", NULL);
    }
    else if(ENG_MODE == 1)
    {
        LOG_Write(LOG_OPRATE, "create new  system SCI file.\n", NULL);

    }
}

/***********************************************************************
* SI_Wr_Prtc_Cfg - Protect function config.
*
* RETURNS: 无
*
*/
static void SI_Wr_Prtc_Cfg(
    int iFd
)
{
    uint8_t aucBuf[12+MAX_ID_LEN];
    uint8_t *puc;
    const SC_SUB_LGC_ITEM *psublgc;
    int i;
    int iIdx;
    uint32_t ulLen;

    ulLen=8+iSubLgcNum_g*5;

    ulLen+=strlen(aucEqName_g);
    for (iIdx=0; iIdx<iSubLgcNum_g; iIdx++)
    {
        psublgc=SC_Get_Sub_Lgc_Attr(iIdx);
        ulLen+=strlen(psublgc->aucName);
    }

    memset(aucBuf, 0, sizeof(aucBuf));

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=0;

    aucBuf[5]=strlen(aucEqName_g);
    puc=aucBuf+6;
    memcpy(puc, aucEqName_g, aucBuf[5]);
    puc+=aucBuf[5];

    if (EnviromentType.bFreq50Sys && !EnviromentType.bFreq60Sys && EnviromentType.bFiveAmpSys && !EnviromentType.bOneAmpSys)
        *puc++=0x00;
    else if (EnviromentType.bFreq50Sys && !EnviromentType.bFreq60Sys && !EnviromentType.bFiveAmpSys && EnviromentType.bOneAmpSys)
        *puc++=0x01;
    else if (!EnviromentType.bFreq50Sys && EnviromentType.bFreq60Sys && EnviromentType.bFiveAmpSys && !EnviromentType.bOneAmpSys)
        *puc++=0x02;
    else if (!EnviromentType.bFreq50Sys && EnviromentType.bFreq60Sys && !EnviromentType.bFiveAmpSys && EnviromentType.bOneAmpSys)
        *puc++=0x03;
    else if (EnviromentType.bFreq50Sys && !EnviromentType.bFreq60Sys && EnviromentType.bFiveAmpSys && EnviromentType.bOneAmpSys)
        *puc++=0x80;
    else if (!EnviromentType.bFreq50Sys && EnviromentType.bFreq60Sys && EnviromentType.bFiveAmpSys && EnviromentType.bOneAmpSys)
        *puc++=0x81;
    else if (EnviromentType.bFreq50Sys && EnviromentType.bFreq60Sys && EnviromentType.bFiveAmpSys && !EnviromentType.bOneAmpSys)
        *puc++=0xC0;
    else if (EnviromentType.bFreq50Sys && EnviromentType.bFreq60Sys && !EnviromentType.bFiveAmpSys && EnviromentType.bOneAmpSys)
        *puc++=0xC1;
    else
        *puc++=0xC2;

    *puc++=uiAppType_g;		/* 应用类型 */
    *puc++=0;
    *puc++=0;
    *puc++=0;

    *puc++=iSubLgcNum_g;

    i=write(iFd, aucBuf, puc-aucBuf);
    assert(i==puc-aucBuf);

    for (iIdx=0; iIdx<iSubLgcNum_g; iIdx++)
    {
        psublgc=SC_Get_Sub_Lgc_Attr(iIdx);

        aucBuf[0]=strlen(psublgc->aucName);

        puc=aucBuf+1;
        memcpy(puc, psublgc->aucName, aucBuf[0]);
        puc+=aucBuf[0];

        *puc++=0;
        *puc++=0;
        *puc++=0;
        *puc++=0;

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);
    }
}

/***********************************************************************
* SI_Wr_Hw_AI_Cfg - Hardware AI config.
*
* RETURNS: 无
*
*/
static void SI_Wr_Hw_AI_Cfg(
    int iFd
)
{
    uint8_t aucBuf[15+MAX_ID_LEN];
    uint32_t ulLen;
    const RD_HW_AI_CH *pch;
    int iCh;
    int i;
    uint8_t *puc;
    int  k;

    assert(iFd>=0);

    assert(iHwAiChNum_g<=255);
    ulLen=6+iHwAiChNum_g*31;
    for (iCh=0; iCh<iHwAiChNum_g; iCh++)
    {
        pch=RD_Get_Hw_AI_Attr(iCh);
        ulLen+=strlen(pch->aucId);
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=4;

    aucBuf[5]=1;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    aucBuf[9]=iHwAiChNum_g;

    i=write(iFd, aucBuf, 10);
    assert(i==10);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=0; iCh<iHwAiChNum_g; iCh++)
    {
        pch=RD_Get_Hw_AI_Attr(iCh);

        aucBuf[0]=iCh;
        i=write(iFd, aucBuf, 1);
        assert(i==1);

        for(k=0; k<16; k++)
        {
            aucBuf[k]=0;
        }
#if defined(EDP_01_02_BUILD) || defined(EDP03_BUILD)                    /*EDP02平台 */
        if((pch->paimod==(&(aimodOpt_g[0])))||(pch->paimod==(&(aimodOpt_g[1]))) || (pch->paimod==(&(aimodPole_g)))|| (pch->paimod==(&(aimodHdl_g))))
        {
            /* 添加同杆并架 DY 6/12/2007 */
            aucBuf[0]=1;
        }
        else  if((pch->paimod==(&aimodDsp_g))||(pch->paimod==(&aimodExt_g)))
        {

            aucBuf[0]=pch->ucMmiShow;
        }
        else
        {
            assert(FALSE);
        }
#endif

        i=write(iFd, aucBuf, 16);
        assert(i==16);

        aucBuf[0]=0;
        aucBuf[1]=0;
        aucBuf[2]=0;
        aucBuf[3]=0;

        aucBuf[4]=pch->ucUnit;

        FLT_TO_BYTES(aucBuf+5,pch->fOriginCoff);

        aucBuf[9]=strlen(pch->aucId);
        assert(aucBuf[9]<=MAX_ID_LEN);

        puc=aucBuf+10;
        memcpy(puc, pch->aucId, aucBuf[9]);
        puc+=aucBuf[9];

        *puc++=pch->aucABRV[0];
        *puc++=pch->aucABRV[1];
        *puc++=pch->aucABRV[2];
        *puc++=pch->aucABRV[3];

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);
    }
}

/***********************************************************************
* SI_Wr_DI_Cfg - DI config.
*
* RETURNS: 无
*
*/
static void SI_Wr_DI_Cfg(
    int iFd
)
{
    uint8_t aucBuf[10+MAX_ID_LEN];
    uint32_t ulLen;
    const RD_LGC_DI_CH *pch;
    int iCh;
    int i;
    uint8_t *puc;
    int  k;
    assert(iFd>=0);

    assert(iLgcDiChNum_g<=512);
    ulLen=6+iLgcDiChNum_g*26;
    for (iCh=0; iCh<iLgcDiChNum_g; iCh++)
    {
        pch=RD_Get_DI_Attr(iCh);
        ulLen+=strlen(pch->aucName);
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=5;

    aucBuf[5]=1;
    aucBuf[6]=0;
    aucBuf[7]=0;

    aucBuf[8]=HI8(iLgcDiChNum_g);
    aucBuf[9]=LO8(iLgcDiChNum_g);

    i=write(iFd, aucBuf, 10);
    assert(i==10);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=0; iCh<iLgcDiChNum_g; iCh++)
    {
        pch=RD_Get_DI_Attr(iCh);

        aucBuf[0]=iCh;
        i=write(iFd, aucBuf, 1);
        assert(i==1);
        for(k=0; k<16; k++)
        {
            aucBuf[k]=0;
        }

        if((pch->mod==RD_OPT1_DI)||(pch->mod==RD_OPT2_DI) || (pch->mod==RD_SAME_POLE_DI) || (pch->mod == RD_VT_BOX_DI))
        {
            /* 添加同杆并架 DY 6/12/2007 */
            aucBuf[0]=1;
        }
        else  if((pch->mod==RD_SPI_DI)||(pch->mod==RD_EXT_DI) || (pch->mod==RD_HDL_BOX_DI))
        {
            /* 添加智能操作箱 DY 6/12/2007 */

            aucBuf[0]=pch->ucMmiShow;
        }
        else
            assert(FALSE);

        aucBuf[1]=pch->ReserveAttribute;
        i=write(iFd, aucBuf, 16);
        assert(i==16);

        U32_TO_BYTES(aucBuf, pch->ulFiltTime/1000);

        aucBuf[4]=strlen(pch->aucName);
        assert(aucBuf[4]<=MAX_ID_LEN);

        puc=aucBuf+5;
        memcpy(puc, pch->aucName, aucBuf[4]);
        puc+=aucBuf[4];

        *puc++=pch->aucABRV[0];
        *puc++=pch->aucABRV[1];
        *puc++=pch->aucABRV[2];
        *puc++=pch->aucABRV[3];

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);
    }
}

/***********************************************************************
* SI_Wr_DO_Cfg - DO config.
*
* RETURNS: 无
*
*/
static void SI_Wr_DO_Cfg(
    int iFd
)
{
    uint8_t aucBuf[11+MAX_ID_LEN];
    uint32_t ulLen;
    const RD_LGC_DO_CH *pch;
    int iCh;
    int i;
    uint8_t *puc;
    int k;
    uint8_t aucTemp[16]= {0};
    uint8_t aucTemp2[16]= {0};
    int ilength=0;
    int ilength2=0;
    assert(iFd>=0);

    assert(iLgcDoChNum_g<=255);
    ulLen=6+iLgcDoChNum_g*27;
    for (iCh=0; iCh<iLgcDoChNum_g; iCh++)
    {
        pch=RD_Get_DO_Attr(iCh);
        ulLen+=strlen(pch->aucName);
    }
#ifdef EDP_01_02_BUILD
    if(ENG_MODE==0)
        ilength =sprintf(aucTemp,"启动测试");
    else if(ENG_MODE==1)
        ilength =sprintf(aucTemp,"QD test");
    ulLen+=27 +ilength;

    if(ENG_MODE==0)
        ilength2 =sprintf(aucTemp2,"CPU告警");
    else if(ENG_MODE==1)
        ilength2 =sprintf(aucTemp2,"Device Alarm");
    ulLen+=27 +ilength2;
#endif

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=6;

    aucBuf[5]=1;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;
#ifdef EDP_01_02_BUILD
    aucBuf[9]=iLgcDoChNum_g+2;
#else
    aucBuf[9]=iLgcDoChNum_g;
#endif
    i=write(iFd, aucBuf, 10);
    assert(i==10);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=0; iCh<iLgcDoChNum_g; iCh++)
    {
        pch=RD_Get_DO_Attr(iCh);

        aucBuf[0]=iCh;
        i=write(iFd, aucBuf, 1);
        assert(i==1);

        for(k=0; k<16; k++)
        {
            aucBuf[k]=0;
        }

        if((pch->mod==RD_OPT1_DO)||(pch->mod==RD_OPT2_DO) || (pch->mod==RD_SAME_POLE_DO) || (pch->mod == RD_VT_BOX_DO))
        {
            /* 添加同杆并架 DY 6/12/2007 */
            aucBuf[0]=1;
        }
        else  if((pch->mod==RD_SPI_DO) || (pch->mod==RD_HDL_BOX_DO))
        {
            /* 添加智能操作箱 DY 6/12/2007 */

            aucBuf[0]=pch->ucMmiShow;
        }
        else
        {
            assert(FALSE);
        }

        aucBuf[1]=pch->ReserveAttribute;
        i=write(iFd, aucBuf, 16);
        assert(i==16);

        aucBuf[0]=0;
        aucBuf[1]=0;
        aucBuf[2]=0;
        aucBuf[3]=0;
        aucBuf[4]=pch->bValid ? 1:0;

        aucBuf[5]=strlen(pch->aucName);
        assert(aucBuf[5]<=MAX_ID_LEN);

        puc=aucBuf+6;
        memcpy(puc, pch->aucName, aucBuf[5]);
        puc+=aucBuf[5];

        *puc++=pch->aucABRV[0];
        *puc++=pch->aucABRV[1];
        *puc++=pch->aucABRV[2];
        *puc++=pch->aucABRV[3];

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);
    }

#ifdef EDP_01_02_BUILD
    aucBuf[0]=iCh;
    i=write(iFd, aucBuf, 1);
    for(k=0; k<16; k++)
    {
        aucBuf[k]=0;
    }

    aucBuf[1]=0;
    i=write(iFd, aucBuf, 16);

    aucBuf[0]=0;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0;
    aucBuf[4]=1;

    aucBuf[5]=ilength;
    puc=aucBuf+6;
    memcpy(puc, aucTemp, aucBuf[5]);
    puc+=aucBuf[5];

    sprintf(aucTemp,"QDCS");
    *puc++=aucTemp[0];
    *puc++=aucTemp[1];
    *puc++=aucTemp[2];
    *puc++=aucTemp[3];

    write(iFd, aucBuf, puc-aucBuf);

    /* 装置告警 */
    aucBuf[0]=iCh+1;
    i=write(iFd, aucBuf, 1);
    for(k=0; k<16; k++)
    {
        aucBuf[k]=0;
    }

    aucBuf[1]=0;
    i=write(iFd, aucBuf, 16);

    aucBuf[0]=0;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0;
    aucBuf[4]=1;

    aucBuf[5]=ilength2;
    puc=aucBuf+6;
    memcpy(puc, aucTemp2, aucBuf[5]);
    puc+=aucBuf[5];

    sprintf(aucTemp2,"ZZGJ");
    *puc++=aucTemp2[0];
    *puc++=aucTemp2[1];
    *puc++=aucTemp2[2];
    *puc++=aucTemp2[3];

    write(iFd, aucBuf, puc-aucBuf);
#endif

}

/***********************************************************************
* SI_Wr_Hw_Led_Cfg - Hardware LED config.
*
* RETURNS: 无
*
*/
static void SI_Wr_Hw_Led_Cfg(
    int iFd
)
{
    uint8_t aucBuf[12+MAX_ID_LEN];
    uint32_t ulLen;
    const RD_LGC_LED_CH *pch;
    int iCh;
    int i;
    uint8_t *puc;

    assert(iFd>=0);

    assert(iHwLedChNum_g<=255);
    ulLen=7+iHwLedChNum_g*12;
    for (iCh=0; iCh<iHwLedChNum_g; iCh++)
    {
        pch=RD_Get_Hw_Led_Attr(iCh);
        assert(pch->bIsHwLED);
        ulLen+=strlen(pch->aucName);
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=24;

    aucBuf[5]=0;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    aucBuf[9]=LO8(iHwLedChNum_g);
    aucBuf[10]=HI8(iHwLedChNum_g);

    i=write(iFd, aucBuf, 11);
    assert(i==11);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=0; iCh<iHwLedChNum_g; iCh++)
    {
        pch=RD_Get_Hw_Led_Attr(iCh);

        assert(pch->unCh<=255);
        aucBuf[0]=pch->unCh;
        aucBuf[1]=0;
        aucBuf[2]=0;
        aucBuf[3]=0;
        aucBuf[4]=0;
        assert(pch->ucColor>=1 && pch->ucColor<=3);
        aucBuf[5]=pch->ucColor;
        aucBuf[6]=pch->bKeep ? 1:0;
        aucBuf[7]=pch->ucBlink;
        aucBuf[8]=pch->ReserveAttribute;
        aucBuf[9]=0;
        aucBuf[10]=0;
        aucBuf[11]=strlen(pch->aucName);
        assert(aucBuf[11]<=MAX_ID_LEN);

        puc=aucBuf+12;
        memcpy(puc, pch->aucName, aucBuf[11]);
        puc+=aucBuf[11];

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);
    }
}

/***********************************************************************
* SI_Wr_Sw_Led_Cfg - Software LED config.
*
* RETURNS: 无
*
*/
static void SI_Wr_Sw_Led_Cfg(
    int iFd
)
{
    uint8_t aucBuf[12+MAX_ID_LEN];
    uint32_t ulLen;
    const RD_LGC_LED_CH *pch;
    int iCh;
    int i;
    uint8_t *puc;

    assert(iFd>=0);

    assert(iSwLedChNum_g<=255);
    ulLen=7+iSwLedChNum_g*12;
    for (iCh=0; iCh<iSwLedChNum_g; iCh++)
    {
        pch=RD_Get_Sw_Led_Attr(iCh);
        assert(!pch->bIsHwLED);
        ulLen+=strlen(pch->aucName);
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=25;

    aucBuf[5]=0;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    aucBuf[9]=LO8(iSwLedChNum_g);
    aucBuf[10]=HI8(iSwLedChNum_g);

    i=write(iFd, aucBuf, 11);
    assert(i==11);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=0; iCh<iSwLedChNum_g; iCh++)
    {
        pch=RD_Get_Sw_Led_Attr(iCh);

        assert(pch->unCh<=255);
        aucBuf[0]=pch->unCh;
        aucBuf[1]=0;
        aucBuf[2]=0;
        aucBuf[3]=0;
        aucBuf[4]=0;
        assert(pch->ucColor>=1 && pch->ucColor<=3);
        aucBuf[5]=pch->ucColor;
        aucBuf[6]=pch->bKeep ? 1:0;
        aucBuf[7]=pch->ucBlink;
        aucBuf[8]=0;
        aucBuf[9]=0;
        aucBuf[10]=0;
        aucBuf[11]=strlen(pch->aucName);
        assert(aucBuf[11]<=MAX_ID_LEN);

        puc=aucBuf+12;
        memcpy(puc, pch->aucName, aucBuf[11]);
        puc+=aucBuf[11];

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);
    }
}

/***********************************************************************
* SI_Wr_Set_Cfg - Write setting config to system config infomation file.
*
* RETURNS: 无
*
*/
static void SI_Wr_Set_Cfg(
    int iFd
)
{
    uint8_t aucBuf[1050+MAX_ID_LEN];
    uint32_t ulLen;
    const SC_SET_PAGE *psetpg;
    SC_SET_ITEM *pset;
    int iCh;
    int i;
    uint8_t *puc;

    assert(iFd>=0);

    ulLen=6+(iSetPgNum_g-1)*9;
    for (iCh=1; iCh<iSetPgNum_g; iCh++) /* Skip page 0(internal setting). */
    {
        psetpg=SC_Get_Set_Pg_Attr(iCh);

        ulLen+=strlen(psetpg->aucName)+psetpg->iSetNum*26;
        if (!psetpg->bIsPub)
            ulLen+=1+strlen(psetpg->psublgc->aucName);

        for (pset=psetpg->pset; pset<psetpg->pset+psetpg->iSetNum; pset++)
            ulLen+=strlen(pset->aucName)+strlen(pset->pucUnitName);
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=8;

    aucBuf[5]=0;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    assert(iSetPgNum_g && iSetPgNum_g<=256);
    aucBuf[9]=iSetPgNum_g-1;

    i=write(iFd, aucBuf, 10);
    assert(i==10);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=1; iCh<iSetPgNum_g; iCh++) /* Skip page 0(internal setting). */
    {
        psetpg=SC_Get_Set_Pg_Attr(iCh);

        /* Page number from 0. */
        aucBuf[0]=iCh-1;

        aucBuf[1]=strlen(psetpg->aucName);
        puc=aucBuf+2;
        memcpy(puc, psetpg->aucName, aucBuf[1]);
        puc+=aucBuf[1];

        if (psetpg->bIsPub)
            *puc++=1;
        else
        {
            *puc++=0;
            i=strlen(psetpg->psublgc->aucName);
            assert(i<=255);
            *puc++=i;
            memcpy(puc, psetpg->psublgc->aucName, i);
            puc+=i;
        }

        *puc++=0;
        *puc++=0;
        *puc++=0;
        *puc++=0;

        *puc++=LO8(psetpg->iSetNum);
        *puc++=HI8(psetpg->iSetNum);

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);

        for (pset=psetpg->pset; pset<psetpg->pset+psetpg->iSetNum; pset++)
        {
            i=pset-psetpg->pset;
            aucBuf[0]=LO8(i);
            aucBuf[1]=HI8(i);
            if(pset->bStdSet)
                aucBuf[2]=1;
            else
                aucBuf[2]=0;

            aucBuf[3]=0;
            aucBuf[4]=0;
            aucBuf[5]=0;
            aucBuf[6]=pset->ucUnit;

            U32_TO_BYTES(aucBuf+7, pset->valMax.ulVal);
            U32_TO_BYTES(aucBuf+11, pset->valMin.ulVal);
            U32_TO_BYTES(aucBuf+15, pset->valDft.ulVal);


            i=write(iFd, aucBuf, 19);/*支持字符串，2008-7-21日 张云  */
            assert(i==19);
            if(pset->ucUnit==0x68)
            {
                i=write(iFd, pset->aucDftStr, pset->valDft.ulVal);
                assert(i==pset->valDft.ulVal);
            }

            i=strlen(pset->aucName);
            assert(i<=255);
            aucBuf[0]=i;/*支持字符串改动，2008-7-21日 张云  */
            puc=aucBuf+1;

            memcpy(puc, pset->aucName, i);
            puc+=i;

            *puc++=pset->aucABRV[0];
            *puc++=pset->aucABRV[1];
            *puc++=pset->aucABRV[2];
            *puc++=pset->aucABRV[3];

            i=strlen(pset->pucUnitName);
            assert(i<=1024);
            *puc++=LO8(i);
            *puc++=HI8(i);

            memcpy(puc, pset->pucUnitName, i);
            puc+=i;

            i=write(iFd, aucBuf, puc-aucBuf);
            assert(i==puc-aucBuf);
        }
    }
}

/***********************************************************************
* SI_Wr_Link_Cfg - Write link config to system config infomation file.
*
* RETURNS: 无
*
*/
static void SI_Wr_Link_Cfg(
    int iFd
)
{
    uint8_t aucBuf[10+2*MAX_ID_LEN];
    uint32_t ulLen;
    const SC_LINK_PAGE *plkpg;
    const SC_LINK_ITEM *plink;
    const RD_LGC_DI_CH *pdich;
    int iCh;
    int iPg;
    int i;
    uint8_t *puc;

    assert(iFd>=0);

    ulLen=6+iLkPgNum_g*9;
    for (iPg=0; iPg<iLkPgNum_g; iPg++)
    {
        plkpg=SC_Get_Link_Pg_Attr(iPg);

        ulLen+=strlen(plkpg->aucName)+plkpg->iLinkNum*12;
        if (!plkpg->bIsPub)
            ulLen+=1+strlen(plkpg->psublgc->aucName);
    }
    for (iCh=0; iCh<iLinkNum_g; iCh++)
    {
        plink=SC_Get_Link_Attr(iCh);
        ulLen+=strlen(plink->aucName);
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=9;

    aucBuf[5]=0;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    assert(iLkPgNum_g<=255);
    aucBuf[9]=iLkPgNum_g;

    i=write(iFd, aucBuf, 10);
    assert(i==10);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iPg=0; iPg<iLkPgNum_g; iPg++)
    {
        plkpg=SC_Get_Link_Pg_Attr(iPg);

        aucBuf[0]=iPg;

        aucBuf[1]=strlen(plkpg->aucName);
        puc=aucBuf+2;
        memcpy(puc, plkpg->aucName, aucBuf[1]);
        puc+=aucBuf[1];

        if (plkpg->bIsPub)
            *puc++=1;
        else
        {
            *puc++=0;
            i=strlen(plkpg->psublgc->aucName);
            assert(i<=MAX_ID_LEN && i<=255);
            *puc++=i;
            memcpy(puc, plkpg->psublgc->aucName, i);
            puc+=i;
        }

        *puc++=0;
        *puc++=0;
        *puc++=0;
        *puc++=0;

        *puc++=LO8(plkpg->iLinkNum);
        *puc++=HI8(plkpg->iLinkNum);

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);

        memset(aucBuf, 0, sizeof(aucBuf));
        for (iCh=plkpg->iLinkPos; iCh<plkpg->iLinkPos+plkpg->iLinkNum; iCh++)
        {
            plink=SC_Get_Link_Attr(iCh);

            aucBuf[0]=LO8(iCh);
            aucBuf[1]=HI8(iCh);

            if (plink->bDftVal)
                aucBuf[2]=1;
            else
                aucBuf[2]=0;

            for (i=0; i<iLgcDiChNum_g; i++)
            {
                pdich=RD_Get_DI_Attr(i);
                assert(pdich);

                if (!strcmp(pdich->aucId, plink->aucDiSrc))
                    break;
            }
            aucBuf[3]=i;

            aucBuf[4]=plink->HwLinkType;
            if(plink->HwLinkType&0x01)
            {
                for (i=0; i<iLgcDiChNum_g; i++)
                {
                    pdich=RD_Get_DI_Attr(i);
                    assert(pdich);

                    if (!strcmp(pdich->aucId, plink->aucSecondDiSrc))
                        break;
                }
                aucBuf[5]=i;
            }
            else
                aucBuf[5]=0;
            aucBuf[6]=plink->LinkSwitchMode;

            aucBuf[7]=strlen(plink->aucName);
            assert(aucBuf[7]<=MAX_ID_LEN);

            puc=aucBuf+8;
            memcpy(puc, plink->aucName, aucBuf[7]);
            puc+=aucBuf[7];

            *puc++=plink->aucABRV[0];
            *puc++=plink->aucABRV[1];
            *puc++=plink->aucABRV[2];
            *puc++=plink->aucABRV[3];

            i=write(iFd, aucBuf, puc-aucBuf);
            assert(i==puc-aucBuf);
        }
    }
}

/***********************************************************************
* SI_Wr_Rec_AI_Cfg - Record AI config.
*
* RETURNS: 无
*
*/
static void SI_Wr_Rec_AI_Cfg(
    int iFd
)
{
    uint8_t aucBuf[8+MAX_ID_LEN];
    uint32_t ulLen;
    const RC_AI_CFG *pcfg;
    int iCh;
    int i;
    uint8_t *puc;
    uint8_t   ucTemp;

    assert(iFd>=0);

    ulLen=7+iRecAiNum_g*8;
    for (iCh=0; iCh<iRecAiNum_g; iCh++)
    {
        pcfg=RC_Get_Rec_AI_Attr(iCh);
        ulLen+=strlen(pcfg->aucName);
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=12;

    aucBuf[5]=0;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    aucBuf[9]=LO8(iRecAiNum_g);
    aucBuf[10]=HI8(iRecAiNum_g);

    i=write(iFd, aucBuf, 11);
    assert(i==11);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=0; iCh<iRecAiNum_g; iCh++)
    {
        pcfg=RC_Get_Rec_AI_Attr(iCh);

        aucBuf[0]=LO8(iCh);
        aucBuf[1]=HI8(iCh);

        ucTemp=0;
        if(pcfg->bNotNeedPrint)
        {
            ucTemp=ucTemp|0x01;
        }
        if(pcfg->bNotNeedUpSend)
        {
            ucTemp=ucTemp|0x02;
        }
        if(pcfg->bSrcType)
        {
            ucTemp=ucTemp|0x04;
        }
        aucBuf[2]=ucTemp;

        aucBuf[3]=pcfg->ucPageNum;
        aucBuf[4]=0;
        aucBuf[5]=0;

        aucBuf[6]=pcfg->ucUnit;

        aucBuf[7]=strlen(pcfg->aucName);
        assert(aucBuf[7]<=MAX_ID_LEN);

        puc=aucBuf+8;
        memcpy(puc, pcfg->aucName, aucBuf[7]);
        puc+=aucBuf[7];

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);
    }
}

/***********************************************************************
* SI_Wr_Rec_DI_Cfg - Record DI config.
*
* RETURNS: 无
*
*/
static void SI_Wr_Rec_DI_Cfg(
    int iFd
)
{
    uint8_t aucBuf[7+MAX_ID_LEN];
    uint32_t ulLen;
    const RC_DI_CFG *pcfg;
    int iCh;
    int i;
    uint8_t *puc;
    uint8_t   ucTemp;

    assert(iFd>=0);

    ulLen=7+iRecDiNum_g*7;
    for (iCh=0; iCh<iRecDiNum_g; iCh++)
    {
        pcfg=RC_Get_Rec_DI_Attr(iCh);
        ulLen+=strlen(pcfg->aucName);
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=13;

    aucBuf[5]=0;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    aucBuf[9]=LO8(iRecDiNum_g);
    aucBuf[10]=HI8(iRecDiNum_g);

    i=write(iFd, aucBuf, 11);
    assert(i==11);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=0; iCh<iRecDiNum_g; iCh++)
    {
        pcfg=RC_Get_Rec_DI_Attr(iCh);

        aucBuf[0]=LO8(iCh);
        aucBuf[1]=HI8(iCh);

        ucTemp=0;
        if(pcfg->bNotNeedPrint)
        {
            ucTemp=ucTemp|0x01;
        }
        if(pcfg->bNotNeedUpSend)
        {
            ucTemp=ucTemp|0x02;
        }
        if(pcfg->bSrcType)
        {
            ucTemp=ucTemp|0x04;
        }
        if(pcfg->bNotDORec)
        {
            ucTemp=ucTemp|0x08;
        }
        aucBuf[2]=ucTemp;

        aucBuf[3]=pcfg->ucPageNum;
        aucBuf[4]=0;
        aucBuf[5]=0;

        aucBuf[6]=strlen(pcfg->aucName);
        assert(aucBuf[6]<=MAX_ID_LEN);

        puc=aucBuf+7;
        memcpy(puc, pcfg->aucName, aucBuf[6]);
        puc+=aucBuf[6];

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);
    }
}

/***********************************************************************
* SI_Wr_Flag_Cfg - Record flag config.
*
* RETURNS: 无
*
*/
static void SI_Wr_Flag_Cfg(
    int iFd
)
{
    uint8_t aucBuf[14+2*MAX_ID_LEN];
    uint32_t ulLen;
    const RC_FLAG_PAGE *pfgpg;
    const RC_FLAG_CFG *pflag;
    int iCh;
    int iPg;
    int i;
    uint8_t *puc;
    RC_MODWORD_BIT_FLAG_CFG  *pBitFlagCfg;/*2006-11-24日 张云  */
    int  k;

    assert(iFd>=0);

    ulLen=6+iFgPgNum_g*13;
    for (iPg=0; iPg<iFgPgNum_g; iPg++)
    {
        pfgpg=RC_Get_Flag_Pg_Attr(iPg);

        ulLen+=strlen(pfgpg->aucName)+pfgpg->iFlagNum*12;
        if (!pfgpg->bIsPub)
            ulLen+=1+strlen(pfgpg->psublgc->aucName);
    }
    for (iCh=0; iCh<iRecFgNum_g; iCh++)
    {
        pflag=RC_Get_Flag_Attr(iCh);
        ulLen+=strlen(pflag->aucName);
        if(pflag->bHexword)
        {
            pBitFlagCfg=pflag->pModWordBitFlagArr;
            assert(pBitFlagCfg);
            for(k=0; k<32; k++)
            {
                ulLen+=strlen(pBitFlagCfg->aucName)+5;
                pBitFlagCfg++;
            }
        }
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=15;

    aucBuf[5]=0;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    assert(iFgPgNum_g<=255);
    aucBuf[9]=iFgPgNum_g;

    i=write(iFd, aucBuf, 10);
    assert(i==10);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iPg=0; iPg<iFgPgNum_g; iPg++)
    {
        pfgpg=RC_Get_Flag_Pg_Attr(iPg);

        aucBuf[0]=iPg;

        aucBuf[1]=strlen(pfgpg->aucName);
        puc=aucBuf+2;
        memcpy(puc, pfgpg->aucName, aucBuf[1]);
        puc+=aucBuf[1];

        *puc++=pfgpg->aucABRV[0];
        *puc++=pfgpg->aucABRV[1];
        *puc++=pfgpg->aucABRV[2];
        *puc++=pfgpg->aucABRV[3];

        if (pfgpg->bIsPub)
            *puc++=1;
        else
        {
            *puc++=0;
            i=strlen(pfgpg->psublgc->aucName);
            assert(i<=MAX_ID_LEN && i<=255);
            *puc++=i;
            memcpy(puc, pfgpg->psublgc->aucName, i);
            puc+=i;
        }

        *puc++=0;
        *puc++=0;
        *puc++=0;
        *puc++=0;

        *puc++=LO8(pfgpg->iFlagNum);
        *puc++=HI8(pfgpg->iFlagNum);

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);

        memset(aucBuf, 0, sizeof(aucBuf));
        for (iCh=0; iCh<pfgpg->iFlagNum; iCh++)
        {
            pflag=RC_Get_Flag_Attr(pfgpg->iFlagPos+iCh);

            aucBuf[0]=LO8(iCh);
            aucBuf[1]=HI8(iCh);

            i=write(iFd, aucBuf, 2);
            assert(i==2);

            if(pflag->bHexword)
            {
                /*若是方式字标志， 记录位标志信息 2006-11-24日*/
                aucBuf[0]=1;
                i=write(iFd, aucBuf, 1);
                assert(i==1);

                pBitFlagCfg=pflag->pModWordBitFlagArr;
                for(k=0; k<32; k++)
                {
                    aucBuf[0]=strlen(pBitFlagCfg->aucName);
                    assert(aucBuf[0]<=MAX_ID_LEN);

                    puc=aucBuf+1;
                    memcpy(puc, pBitFlagCfg->aucName, aucBuf[0]);
                    puc+=aucBuf[0];

                    *puc++=pBitFlagCfg->aucABRV[0];
                    *puc++=pBitFlagCfg->aucABRV[1];
                    *puc++=pBitFlagCfg->aucABRV[2];
                    *puc++=pBitFlagCfg->aucABRV[3];

                    i=write(iFd, aucBuf, puc-aucBuf);
                    assert(i==puc-aucBuf);

                    pBitFlagCfg++;
                }

            }
            else
            {
                /*若是非方式字标志。*/
                aucBuf[0]=0;
                i=write(iFd, aucBuf, 1);
                assert(i==1);
            }


            aucBuf[0]=0;
            aucBuf[1]=0;
            aucBuf[2]=0;
            aucBuf[3]=pflag->ucUnit;

            aucBuf[4]=strlen(pflag->aucName);
            assert(aucBuf[4]<=MAX_ID_LEN);

            puc=aucBuf+5;
            memcpy(puc, pflag->aucName, aucBuf[4]);
            puc+=aucBuf[4];

            *puc++=pflag->aucABRV[0];
            *puc++=pflag->aucABRV[1];
            *puc++=pflag->aucABRV[2];
            *puc++=pflag->aucABRV[3];

            i=write(iFd, aucBuf, puc-aucBuf);
            assert(i==puc-aucBuf);
        }
    }
}

/***********************************************************************
* SI_Wr_Evt_Cfg - Event config.
*
* RETURNS: 无
*
*/
static void SI_Wr_Evt_Cfg(
    int iFd
)
{
    uint8_t aucBuf[13+MAX_ID_LEN];
    uint32_t ulLen;
    const VI_EVT_CFG *pevt;
    const VI_EVT_PARM_CFG *pparm;
    int iCh;
    int i;
    uint8_t *puc;
    uint8_t  ucTemp;

    assert(iFd>=0);

    ulLen=7+(iEvtNum_g+SysMaxErrNum_g)*13;
    for(iCh =0; iCh<MAX_SYS_ERR_NUM; iCh++)
    {
        if (SysErrEnableFlag_g & (1LL << iCh))
        {
            ulLen += strlen(SysErrorName[ENG_MODE][iCh]);
        }
    }
    for (iCh=0; iCh<iEvtNum_g; iCh++)
    {
        pevt=VI_Get_Evt_Attr(iCh);
        ulLen+=strlen(pevt->aucName)+pevt->ucParmNum*6;

        for (pparm=pevt->aparmcfg; pparm<pevt->aparmcfg+pevt->ucParmNum; pparm++)
            ulLen+=strlen(pparm->aucName);
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=14;

    aucBuf[5]=0;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    aucBuf[9]=LO8(iEvtNum_g+SysMaxErrNum_g);
    aucBuf[10]=HI8(iEvtNum_g+SysMaxErrNum_g);

    i=write(iFd, aucBuf, 11);
    assert(i==11);
    memset(aucBuf, 0, sizeof(aucBuf));
    for(iCh =0; iCh<MAX_SYS_ERR_NUM; iCh++)
    {
        if (SysErrEnableFlag_g & (1LL << iCh))
        {
            aucBuf[0]=LO8(iCh);
            aucBuf[1]=HI8(iCh);
            aucBuf[2]=strlen(SysErrorName[ENG_MODE][iCh]);
            assert(aucBuf[2]<=MAX_ID_LEN);
            puc=aucBuf+3;
            memcpy(puc, SysErrorName[ENG_MODE][iCh], aucBuf[2]);
            puc+=aucBuf[2];

            *puc++=0;
            *puc++=0;
            *puc++=0;
            *puc++=0;

            *puc++=0;
            *puc++=0x01;   /* 有状态 */

            *puc++=0;
            *puc++=0;
            *puc++=0x05;
            *puc++=0;

            i=write(iFd, aucBuf, puc-aucBuf);
            assert(i==puc-aucBuf);
        }
    }
    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=0; iCh<iEvtNum_g; iCh++)
    {
        pevt=VI_Get_Evt_Attr(iCh);

        aucBuf[0]=LO8(pevt->unCode);
        aucBuf[1]=HI8(pevt->unCode);

        aucBuf[2]=strlen(pevt->aucName);
        assert(aucBuf[2]<=MAX_ID_LEN);

        puc=aucBuf+3;
        memcpy(puc, pevt->aucName, aucBuf[2]);
        puc+=aucBuf[2];

        *puc++=pevt->aucABRV[0];
        *puc++=pevt->aucABRV[1];
        *puc++=pevt->aucABRV[2];
        *puc++=pevt->aucABRV[3];

        *puc++=pevt->bKeep?1:0;
        *puc++=pevt->bHaveSts?1:0;

        ucTemp=0;
        if(pevt->bNeedFastDel)
        {
            ucTemp=ucTemp|0x01;
        }
        if(pevt->bNotNeedUpSend)
        {
            ucTemp=ucTemp|0x02;
        }
        if(pevt->bMmiNotDisRtn)
        {
            ucTemp=ucTemp|0x04;
        }
        if(pevt->bGWReport)
        {
            ucTemp=ucTemp|0x08;
        }
        if(pevt->bGWXinXiReport)
        {
            ucTemp=ucTemp|0x10;
        }
        *puc=ucTemp;

        puc++;
        *puc++=pevt->ucMMIDlgPopAttr;

        *puc++=pevt->ucType;

        *puc++=pevt->ucParmNum;

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);

        for (pparm=pevt->aparmcfg; pparm<pevt->aparmcfg+pevt->ucParmNum; pparm++)
        {
            aucBuf[0]=strlen(pparm->aucName);
            assert(aucBuf[0]<=MAX_ID_LEN);

            puc=aucBuf+1;
            memcpy(puc, pparm->aucName, aucBuf[0]);
            puc+=aucBuf[0];

            *puc++=0;
            *puc++=0;
            *puc++=0;
            *puc++=0;

            *puc++=pparm->ucAttrib;

            i=write(iFd, aucBuf, puc-aucBuf);
            assert(i==puc-aucBuf);
        }
    }
}

/***********************************************************************
* SI_Wr_Mea_AI_Cfg - Measurement AI config.
*
* RETURNS: 无
*
*/
static void SI_Wr_Mea_AI_Cfg(
    int iFd
)
{
    uint8_t aucBuf[45+2*MAX_ID_LEN];
    uint32_t ulLen;
    const VI_MEA_AI_CFG *pcfg;
    int iCh;
    int i;
    uint8_t *puc;
    uint8_t   ucTemp;

    assert(iFd>=0);

    ulLen=7+iMeaAiNum_g*45;
    for (iCh=0; iCh<iMeaAiNum_g; iCh++)
    {
        pcfg=VI_Get_Mea_AI_Attr(iCh);
        ulLen+=strlen(pcfg->aucName);
        if(pcfg->bIsPrvtUse)
        {
            ulLen+=1+strlen(pcfg->psublgc->aucName);
        }
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=20;

    aucBuf[5]=1;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    aucBuf[9]=LO8(iMeaAiNum_g);
    aucBuf[10]=HI8(iMeaAiNum_g);

    i=write(iFd, aucBuf, 11);
    assert(i==11);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=0; iCh<iMeaAiNum_g; iCh++)
    {
        pcfg=VI_Get_Mea_AI_Attr(iCh);

        aucBuf[0]=LO8(iCh);
        aucBuf[1]=HI8(iCh);
        aucBuf[2]=pcfg->ucArith;
        aucBuf[3]=0;
        aucBuf[4]=0;
        aucBuf[5]=0;

        aucBuf[6]=pcfg->ucUnit;

        puc=aucBuf+7;
        *puc++=pcfg->aucABRV[0];
        *puc++=pcfg->aucABRV[1];
        *puc++=pcfg->aucABRV[2];
        *puc++=pcfg->aucABRV[3];
        *puc++=pcfg->bIsPrvtUse;
        if(pcfg->bIsPrvtUse)
        {
            ucTemp=strlen(pcfg->psublgc->aucName);
            assert(ucTemp<=MAX_ID_LEN);
            *puc++=ucTemp;

            memcpy(puc, pcfg->psublgc->aucName, ucTemp);
            puc+=ucTemp;
        }
        *puc++=pcfg->ucHmSeq;		/* 谐波次数 */
        *puc++=0;
        *puc++=0;
        *puc++=0;
        *puc++=0;
        *puc++=0;
        *puc++=0;
        *puc++=0;
        *puc++=0;
        *puc++=0;
        *puc++=0;

        FLT_TO_BYTES(puc, pcfg->fRtMax);
        FLT_TO_BYTES(puc+4, pcfg->fRtMin);
        FLT_TO_BYTES(puc+8, pcfg->fOvMax);
        FLT_TO_BYTES(puc+12, pcfg->fOvMin);
        puc+=16;

        ucTemp=0;
        if(pcfg->bNotNeedUpSend)
        {
            ucTemp=ucTemp|0x01;
        }
        if(pcfg->bSrcType)
        {
            ucTemp=ucTemp|0x02;
        }
        *puc++=ucTemp;

        FLT_TO_BYTES(puc, pcfg->fChgCoff);
        puc+=4;

        ucTemp=strlen(pcfg->aucName);
        assert(ucTemp<=MAX_ID_LEN);
        *puc++=ucTemp;

        memcpy(puc, pcfg->aucName, ucTemp);
        puc+=ucTemp;

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);
    }
}

/***********************************************************************
* SI_Wr_Mea_DI_Cfg - Measurement DI config.
*
* RETURNS: 无
*
*/
static void SI_Wr_Mea_DI_Cfg(
    int iFd
)
{
    uint8_t aucBuf[7+2*MAX_ID_LEN];
    uint32_t ulLen;
    const VI_MEA_DI_CFG *pcfg;
    int iCh;
    int i;
    uint8_t *puc;
    uint8_t  ucTemp;


    assert(iFd>=0);

    ulLen=7+iMeaDiNum_g*7;
    for (iCh=0; iCh<iMeaDiNum_g; iCh++)
    {
        pcfg=VI_Get_Mea_DI_Attr(iCh);
        ulLen+=strlen(pcfg->aucName);
        if(pcfg->bIsPrvtUse)
        {
            /* 保护专用 */
            ulLen += strlen(pcfg->psublgc->aucName)+1;
        }
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=21;

    aucBuf[5]=0;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    aucBuf[9]=LO8(iMeaDiNum_g);
    aucBuf[10]=HI8(iMeaDiNum_g);

    i=write(iFd, aucBuf, 11);
    assert(i==11);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=0; iCh<iMeaDiNum_g; iCh++)
    {
        pcfg=VI_Get_Mea_DI_Attr(iCh);

        aucBuf[0]=LO8(iCh);
        aucBuf[1]=HI8(iCh);

        ucTemp=0;
        if(pcfg->bNotNeedUpSend)
        {
            ucTemp=ucTemp|0x01;
        }
        if(pcfg->bJianXiuUpSendFlag)
        {
            /* 检修是否上送，为1则上送 */
            ucTemp=ucTemp|0x02;
        }
        if(pcfg->bSrcType)
        {
            ucTemp=ucTemp|0x04;
        }
        if(pcfg->bDbSts)
        {
            /* 双点遥信 */
            ucTemp=ucTemp|0x08;
        }
        if(!pcfg->bSOE)
        {
            /* 0: 上送SOE 1: 不上送SOE，为便于处理，进行了逻辑取反 */
            ucTemp=ucTemp|0x10;
        }
        aucBuf[2]=ucTemp;
        puc=aucBuf+3;
        *puc++=pcfg->bIsPrvtUse;
        if(pcfg->bIsPrvtUse)
        {
            ucTemp=strlen(pcfg->psublgc->aucName);
            assert(ucTemp<=MAX_ID_LEN);
            *puc++=ucTemp;

            memcpy(puc, pcfg->psublgc->aucName, ucTemp);
            puc+=ucTemp;
        }
        *puc++=0;
        *puc++=0;
        ucTemp=strlen(pcfg->aucName);
        assert(ucTemp<=MAX_ID_LEN);
        *puc++=ucTemp;

        memcpy(puc, pcfg->aucName, ucTemp);
        puc+=ucTemp;

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);
    }
}

/***********************************************************************
* SI_Wr_PO_Cfg - po config.
*
* RETURNS: 无
*
*/
static void SI_Wr_PO_Cfg(
    int iFd
)
{
    uint8_t aucBuf[35+MAX_ID_LEN];
    uint32_t ulLen;
    const RD_LGC_PO_CH *pcfg;
    uint8_t *puc;
    uint8_t ucTemp;
    int i;
    int iCh;

    assert(iFd>=0);

    ulLen=7+iLgcPoChNum_g*35;
    for (iCh=0; iCh<iLgcPoChNum_g; iCh++)
    {
        pcfg=RD_Get_PO_Attr(iCh);
        ulLen+=strlen(pcfg->aucName);
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=22;
    aucBuf[5]=0;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    aucBuf[9]=LO8(iLgcPoChNum_g);
    aucBuf[10]=HI8(iLgcPoChNum_g);

    i=write(iFd, aucBuf, 11);
    assert(i==11);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=0; iCh<iLgcPoChNum_g; iCh++)
    {
        pcfg=RD_Get_PO_Attr(iCh);

        aucBuf[0]=LO8(pcfg->unLgcSN);
        aucBuf[1]=HI8(pcfg->unLgcSN);

        aucBuf[2]=pcfg->ucType;		/* PO类型 */
        aucBuf[3]=pcfg->ucUnit;		/* 单位类型 */
        puc=aucBuf+4;
        for(i = 0; i<26; i++)
            *puc++=0;/*保留*/
        /*puc+=26; */
        *puc++=pcfg->aucABRV[0];
        *puc++=pcfg->aucABRV[1];
        *puc++=pcfg->aucABRV[2];
        *puc++=pcfg->aucABRV[3];

        ucTemp=strlen(pcfg->aucName);
        assert(ucTemp<=MAX_ID_LEN);
        *puc++=ucTemp;

        memcpy(puc, pcfg->aucName, ucTemp);
        puc+=ucTemp;

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);
    }
}

/***********************************************************************
* SI_Wr_PI_Cfg - pi config
*
* RETURNS: 无
*
*/
static void SI_Wr_PI_Cfg(
    int iFd
)
{
    uint8_t aucBuf[35+MAX_ID_LEN];
    uint32_t ulLen;
    const RD_LGC_PI_CH *pcfg;
    uint8_t *puc;
    uint8_t ucTemp;
    int i;
    int iCh;

    assert(iFd>=0);

    ulLen=7+iLgcPiChNum_g*35;
    for (iCh=0; iCh<iLgcPiChNum_g; iCh++)
    {
        pcfg=RD_Get_PI_Attr(iCh);
        ulLen+=strlen(pcfg->aucName);
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=27;
    aucBuf[5]=0;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    aucBuf[9]=LO8(iLgcPiChNum_g);
    aucBuf[10]=HI8(iLgcPiChNum_g);

    i=write(iFd, aucBuf, 11);
    assert(i==11);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=0; iCh<iLgcPiChNum_g; iCh++)
    {
        pcfg=RD_Get_PI_Attr(iCh);

        aucBuf[0]=LO8(pcfg->unLgcSN);
        aucBuf[1]=HI8(pcfg->unLgcSN);

        aucBuf[2]=pcfg->ucUnit;
        puc=aucBuf+3;
        /*puc+=27;  保留*/
        for(i=0; i<27; i++)
            *puc++=0;
        *puc++=pcfg->aucABRV[0];
        *puc++=pcfg->aucABRV[1];
        *puc++=pcfg->aucABRV[2];
        *puc++=pcfg->aucABRV[3];

        ucTemp=strlen(pcfg->aucName);
        assert(ucTemp<=MAX_ID_LEN);
        *puc++=ucTemp;

        memcpy(puc, pcfg->aucName, ucTemp);
        puc+=ucTemp;

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);
    }
}

/***********************************************************************
* SI_Wr_Mea_DO_Cfg - Measurement DO config.
*
* RETURNS: 无
*
*/
static void SI_Wr_Mea_DO_Cfg(
    int iFd
)
{
    uint8_t aucBuf[39+MAX_ID_LEN];
    uint32_t ulLen;
    const VI_MEA_DO_CFG *pcfg = NULL;
    const VI_EVT_PARM_CFG *pparm;
    int iCh;
    int i;
    uint8_t *puc;
    uint8_t  ucTemp;


    assert(iFd>=0);

    ulLen=6+iMeaDoNum_g*39;
    for (iCh=0; iCh<iMeaDoNum_g; iCh++)
    {
        pcfg = VI_Get_Mea_DO_Attr(iCh);
        ulLen+=strlen(pcfg->aucName)+pcfg->ucParmNum*10;
        for (pparm=pcfg->aparmcfg; pparm<pcfg->aparmcfg+pcfg->ucParmNum; pparm++)
            ulLen+=strlen(pparm->aucName);
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=23;

    aucBuf[5]=0;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    aucBuf[9]=LO8(iMeaDoNum_g);

    i=write(iFd, aucBuf, 10);
    assert(i==10);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=0; iCh<iMeaDoNum_g; iCh++)
    {
        pcfg = VI_Get_Mea_DO_Attr(iCh);

        aucBuf[0]=LO8(pcfg->uiPtNum);
        aucBuf[1]=HI8(pcfg->uiPtNum);
        puc=aucBuf+28;
        memset(aucBuf+2,0,26);


        *puc++ = pcfg->ucType;

        ucTemp=strlen(pcfg->aucName);
        assert(ucTemp<=MAX_ID_LEN);
        *puc++=ucTemp;
        memcpy(puc, pcfg->aucName, ucTemp);
        puc+=ucTemp;

        *puc++=pcfg->aucABRV[0];
        *puc++=pcfg->aucABRV[1];
        *puc++=pcfg->aucABRV[2];
        *puc++=pcfg->aucABRV[3];

        puc+=4;

        *puc++=pcfg->ucParmNum;

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);

        for (pparm=pcfg->aparmcfg; pparm<pcfg->aparmcfg+pcfg->ucParmNum; pparm++)
        {
            memset(aucBuf, 0, sizeof(aucBuf));
            aucBuf[0]=strlen(pparm->aucName);
            assert(aucBuf[0]<=MAX_ID_LEN);

            puc=aucBuf+1;
            memcpy(puc, pparm->aucName, aucBuf[0]);
            puc+=aucBuf[0];
            for(i=0; i<8; i++)
                *puc++=0;

            *puc++=pparm->ucAttrib;

            i=write(iFd, aucBuf, puc-aucBuf);
            assert(i==puc-aucBuf);
        }
    }
}

/***********************************************************************
* SI_Wr_Msu_Cfg - Measure value config
*
* RETURNS: 无
*
*/
static void  SI_Wr_Msu_Cfg(
    int iFd
)
{
    uint8_t aucBuf[62+MAX_ID_LEN];
    uint32_t ulLen;
    const ME_MEA_VALUE_CFG *pcfg;
    int iCh;
    int i;
    uint8_t *puc;
    uint8_t   ucTemp;

    assert(iFd>=0);

    ulLen=7+iMeaValueNum_g*62;
    for (iCh=0; iCh<iMeaValueNum_g; iCh++)
    {
        pcfg=ME_Get_Msu_Value_Attr(iCh);
        ulLen+=strlen(pcfg->aucName);
    }

    U32_TO_BYTES(aucBuf, ulLen);

    aucBuf[4]=26;

    aucBuf[5]=1;
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    aucBuf[9]=LO8(iMeaValueNum_g);
    aucBuf[10]=HI8(iMeaValueNum_g);

    i=write(iFd, aucBuf, 11);
    assert(i==11);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (iCh=0; iCh<iMeaValueNum_g; iCh++)
    {
        pcfg=ME_Get_Msu_Value_Attr(iCh);

        aucBuf[0]=LO8(iCh);
        aucBuf[1]=HI8(iCh);
        aucBuf[2]=pcfg->ucArith;
        aucBuf[3]=pcfg->ucHmSeq;		/* 谐波次数 */
        aucBuf[4]=0;
        aucBuf[5]=0;
        aucBuf[6]=0;

        aucBuf[7]=pcfg->ucUnit;
        puc=aucBuf+8;
        *puc++=pcfg->aucABRV[0];
        *puc++=pcfg->aucABRV[1];
        *puc++=pcfg->aucABRV[2];
        *puc++=pcfg->aucABRV[3];
        for(i=0; i<12; i++)
            *puc++=0;

        FLT_TO_BYTES(puc, pcfg->fRtMax);
        FLT_TO_BYTES(puc+4, pcfg->fRtMin);
        FLT_TO_BYTES(puc+8, pcfg->fOvMax);
        FLT_TO_BYTES(puc+12, pcfg->fOvMin);
        puc+=16;

        ucTemp=0;
        if(pcfg->bNotNeedUpSend)
        {
            ucTemp=ucTemp|0x01;
        }
        *puc++=ucTemp;
        for(i=0; i<20; i++)
            *puc++=0;
        /*puc+=20;*/

        ucTemp=strlen(pcfg->aucName);
        assert(ucTemp<MAX_ID_LEN);
        *puc++=ucTemp;
        memcpy(puc, pcfg->aucName, ucTemp);
        puc+=ucTemp;

        i=write(iFd, aucBuf, puc-aucBuf);
        assert(i==puc-aucBuf);
    }
}

/***********************************************************************
* SI_New_DI_File - 生成新的开入默认文件
*
* RETURNS: 无
*
*/
void SI_New_DI_File(void)
{
    uint8_t aucLine[ITEM_LEN];
    uint8_t *pucOld;
    const RD_LGC_DI_CH *pdi;
    uint8_t *puc;
    int i;
    int iIdx;
    int iFd;
    uint32_t ulOldLen;
    EP_STATUS sts;

    uint16_t ulCrc=0;
    uint8_t aucTmp[ITEM_LEN];

    if ((pucOld=FT_File_To_Mem(EP_DI_STS_FILE, &ulOldLen))!=NULL)
    {
        if (ulOldLen%ITEM_LEN || ulOldLen<ITEM_LEN || memcmp(pucOld, pucDiFileHead_g, ITEM_LEN))
        {
            /* File length or header not valid. */
            /* assert(FALSE); */

            if (FT_Is_File(EP_DI_STS_FILE))
            {
                remove(EP_DI_STS_FILE);
            }

            EP_free(pucOld);
            pucOld=NULL;
        }
    }

    iFd=FT_Bgn_Update(EP_DI_STS_FILE);
    assert(iFd>=0);

    i=write(iFd, (char*)pucDiFileHead_g, ITEM_LEN);
    assert(i==ITEM_LEN);
    ulCrc=EP_CCITT_CRC16((uint8_t *)pucDiFileHead_g,ITEM_LEN,ulCrc);

    for (iIdx=0; iIdx<iLgcDiChNum_g; iIdx++)
    {
        pdi=RD_Get_DI_Attr(iIdx);

        SI_Tag_Str_Cpy(aucLine, pdi->aucABRV, ITEM_NAME_LEN);

        if (pucOld)
        {
            for (puc=pucOld+ITEM_LEN; puc<pucOld+ulOldLen; puc+=ITEM_LEN)
            {
                if (!memcmp(puc, aucLine, ITEM_NAME_LEN) &&
                        (!memcmp(puc+ITEM_NAME_LEN, DI_CLOSE"\n", ITEM_VALUE_LEN) ||
                         !memcmp(puc+ITEM_NAME_LEN, DI_OPEN"\n", ITEM_VALUE_LEN) ||
                         !memcmp(puc+ITEM_NAME_LEN, DI_NORMAL"\n", ITEM_VALUE_LEN)))
                {
                    i=write(iFd, puc, ITEM_LEN);
                    assert(i==ITEM_LEN);
                    ulCrc=EP_CCITT_CRC16(puc,ITEM_LEN,ulCrc);

                    break;
                }
            }
            if (puc<pucOld+ulOldLen)
                continue;
        }

        /* Not found in old file. */
        memcpy(aucLine+ITEM_NAME_LEN, DI_NORMAL"\n", ITEM_VALUE_LEN);
        i=write(iFd, aucLine, ITEM_LEN);
        assert(i==ITEM_LEN);
        ulCrc=EP_CCITT_CRC16(aucLine,ITEM_LEN,ulCrc);
    }

    /* 写入CRC */
    SI_Tag_Str_Cpy(aucLine, "CRC", ITEM_NAME_LEN);
    sprintf(aucTmp, "%04x", ulCrc);
    SI_Tag_Str_Cpy(aucLine+ITEM_NAME_LEN, aucTmp, ITEM_VALUE_LEN);

    i = write(iFd, aucLine, ITEM_LEN);
    assert (i == ITEM_LEN);

    if (pucOld)
        EP_free(pucOld);

    sts=FT_End_Update(EP_DI_STS_FILE, iFd);
    assert(sts==EP_SUCCESS);

#if 0
    sts=FT_Wr_INI_CRC("[CRC]",CRC_ITEM_DIFDORCE,ulCrc);
    assert (sts != EP_ERROR);
#endif

    if(ENG_MODE==1)
        LOG_Write(LOG_OPRATE, "Successful create new DI status file .\n", NULL);
    else if(ENG_MODE==0)
        LOG_Write(LOG_OPRATE, "成功创建新的DI状态文件.\n", NULL);
}

/***********************************************************************
* SI_New_Link_File - 生成新的压板文件
*
* RETURNS: 无
*
*/
void SI_New_Link_File(void)
{
    uint8_t aucLine[ITEM_LEN];
    uint8_t *pucOld;
    const SC_LINK_ITEM *plink;
    uint8_t *puc=NULL;
    int i;
    int iIdx;
    int iFd;
    uint32_t ulOldLen;
    EP_STATUS sts;

    uint16_t ulCrc=0;
    uint8_t aucTmp[ITEM_LEN];

    if ((pucOld=FT_File_To_Mem(EP_LINK_STS_FILE, &ulOldLen))!=NULL)
    {
        if (ulOldLen%ITEM_LEN || ulOldLen<ITEM_LEN
                || memcmp(pucOld, pucLinkFileHead_g, ITEM_LEN))
        {
            /* File length or header not valid. */

            /* assert(FALSE); */
            if (FT_Is_File(EP_LINK_STS_FILE))
            {
                remove(EP_LINK_STS_FILE);
            }

            EP_free(pucOld);
            pucOld=NULL;
        }
    }

    iFd=FT_Bgn_Update(EP_LINK_STS_FILE);
    assert(iFd>=0);

    i=write(iFd, (char*)pucLinkFileHead_g, ITEM_LEN);
    assert(i==ITEM_LEN);
    ulCrc=EP_CCITT_CRC16((uint8_t *)pucLinkFileHead_g,ITEM_LEN,ulCrc);

    SI_Tag_Str_Cpy(aucLine, SW_CUR_LINK_MODE_NAME, ITEM_NAME_LEN);
    puc=pucOld+ITEM_LEN;		/* 开始地址 */
    if (pucOld && !memcmp(pucOld+ITEM_LEN, aucLine, ITEM_NAME_LEN) &&
            (!memcmp(puc+ITEM_NAME_LEN, SWLINK_NAME, ITEM_VALUE_LEN) ||
             !memcmp(puc+ITEM_NAME_LEN, HWLINK_NAME, ITEM_VALUE_LEN) ||
             !memcmp(puc+ITEM_NAME_LEN, SHWLINK_NAME, ITEM_VALUE_LEN)))
    {
        i=write(iFd, pucOld+ITEM_LEN, ITEM_LEN);
        assert(i==ITEM_LEN);
        ulCrc=EP_CCITT_CRC16(pucOld+ITEM_LEN,ITEM_LEN,ulCrc);
    }
    else
    {
        memcpy(aucLine+ITEM_NAME_LEN, SWLINK_NAME, ITEM_VALUE_LEN);
        i=write(iFd, aucLine, ITEM_LEN);
        assert(i==ITEM_LEN);
        ulCrc=EP_CCITT_CRC16(aucLine,ITEM_LEN,ulCrc);
    }

    for (iIdx=0; iIdx<iLinkNum_g; iIdx++)
    {
        plink=SC_Get_Link_Attr(iIdx);

        SI_Tag_Str_Cpy(aucLine, plink->aucABRV, ITEM_NAME_LEN);

        if (pucOld)
        {
            for (puc=pucOld+ITEM_LEN*2; puc<pucOld+ulOldLen; puc+=ITEM_LEN)
            {
                if (!memcmp(puc, aucLine, ITEM_NAME_LEN) &&
                        (!memcmp(puc+ITEM_NAME_LEN, SW_CLOSE"\n", ITEM_VALUE_LEN) ||
                         !memcmp(puc+ITEM_NAME_LEN, SW_OPEN"\n", ITEM_VALUE_LEN)))
                {
                    i=write(iFd, puc, ITEM_LEN);
                    assert(i==ITEM_LEN);
                    ulCrc=EP_CCITT_CRC16(puc,ITEM_LEN,ulCrc);

                    break;
                }
            }
            if (puc<pucOld+ulOldLen)
                continue;
        }

        /* Not found in old file. */
        if (plink->bDftVal)
            memcpy(aucLine+ITEM_NAME_LEN, SW_CLOSE"\n", ITEM_VALUE_LEN);
        else
            memcpy(aucLine+ITEM_NAME_LEN, SW_OPEN"\n", ITEM_VALUE_LEN);

        i=write(iFd, aucLine, ITEM_LEN);
        ulCrc=EP_CCITT_CRC16(aucLine,ITEM_LEN,ulCrc);
        assert(i==ITEM_LEN);
    }

    /* 写入CRC */
    SI_Tag_Str_Cpy(aucLine, "CRC", ITEM_NAME_LEN);
    sprintf(aucTmp, "%04x", ulCrc);
    SI_Tag_Str_Cpy(aucLine+ITEM_NAME_LEN, aucTmp, ITEM_VALUE_LEN);

    i = write(iFd, aucLine, ITEM_LEN);
    assert (i == ITEM_LEN);

    if (pucOld)
        EP_free(pucOld);

    sts=FT_End_Update(EP_LINK_STS_FILE, iFd);
    assert(sts==EP_SUCCESS);

#if 0
    sts=FT_Wr_INI_CRC("[CRC]",CRC_ITEM_LINKSTATS,ulCrc);
    assert (sts != EP_ERROR);
#endif

    if(ENG_MODE==1)
        LOG_Write(LOG_OPRATE, "Successful create new switch status file .\n", NULL);
    else if(ENG_MODE==0)
        LOG_Write(LOG_OPRATE, "成功创建新的压板状态文件.\n", NULL);

}

/***********************************************************************
* SI_New_Link_MODE_File - 检查当前“定制压板模式配置文件”内容是否需要重建或者更新
*
* RETURNS: 无
*
*/
static void SI_New_Link_MODE_File()
{
    uint8_t aucLine[81]="";
    uint8_t *pucOld;
    const SC_LINK_ITEM *plink;
    uint8_t *puc;
    int i;
    int iIdx;
    int iFd;
    uint32_t ulOldLen;
    EP_STATUS sts;
    uint8_t aucSgMode[81]="__TotalYabanMode__                                              ";
    uint8_t aucSgNum[81] ="__TotalYabanNum__                                               ";
    uint8_t aucTemp[17]="";

    uint16_t ulCrc=0;


    if ((pucOld=FT_File_To_Mem(EP_LINK_MODE_FILE, &ulOldLen))!=NULL)
    {
        if (ulOldLen%81 || ulOldLen<81 || memcmp(pucOld, pucLinkModeFileHead_g, 81))
        {
            /* File length or header not valid. */
            assert(FALSE);
            EP_free(pucOld);
            pucOld=NULL;
        }
    }

    iFd=FT_Bgn_Update(EP_LINK_MODE_FILE);
    assert(iFd>=0);

    i=write(iFd, (char*)pucLinkModeFileHead_g, 81);/*line 1*/
    assert(i==81);
    ulCrc=EP_CCITT_CRC16((uint8_t *)pucLinkModeFileHead_g,81,ulCrc);

    if(pucOld)
    {
        puc=pucOld+81;
        if (!memcmp(puc, aucSgMode, 64)&&
                (!memcmp(puc+64, "HARD            \n", 17)||
                 !memcmp(puc+64, "SOFT            \n", 17)||
                 !memcmp(puc+64, "AND             \n", 17)||
                 !memcmp(puc+64, "OR              \n", 17)||
                 !memcmp(puc+64, "CUSTOM          \n", 17)))
        {
            memcpy(aucSgMode+64,puc+64,17);
        }
        else
        {
            memcpy(aucSgMode+64,"HARD            \n",17);
        }
    }
    else
    {
        /* 根据平台进行设定 */
#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)    	/* EDP03平台和励磁平台 */
        SI_Tag_Str_Cpy(aucSgMode+64,"SOFT            \n",17);
#else
        SI_Tag_Str_Cpy(aucSgMode+64,"AND             \n",17);
#endif
    }
    i=write(iFd, (char*)aucSgMode, 81);/*line 2*/
    assert(i==81);
    ulCrc=EP_CCITT_CRC16(aucSgMode,81,ulCrc);

    sprintf(aucTemp,"%d",iLinkNum_g);
    SI_Tag_Str_Cpy(aucSgNum+64,aucTemp,17);
    aucSgNum[80]='\n';
    i=write(iFd, (char*)aucSgNum, 81);/*line 3*/
    assert(i==81);
    ulCrc=EP_CCITT_CRC16(aucSgNum,81,ulCrc);

    SI_Tag_Str_Cpy(aucLine," ",81); /*aucLine赋值为空行：80个空格＋'\n'*/
    aucLine[80]='\n';
    i=write(iFd, (char*)aucLine, 81);/*line 4*/
    assert(i==81);
    ulCrc=EP_CCITT_CRC16(aucLine,81,ulCrc);

    i=write(iFd, (char*)aucLine, 81);/*line 5*/
    assert(i==81);
    ulCrc=EP_CCITT_CRC16(aucLine,81,ulCrc);

    for (iIdx=0; iIdx<iLinkNum_g; iIdx++)
    {
        plink=SC_Get_Link_Attr(iIdx);

        memset(aucLine,0,sizeof(aucLine));
        SI_Tag_Str_Cpy(aucLine, plink->aucId, 64);
        if(plink->LinkSwitchMode==1)
        {
            SI_Tag_Str_Cpy(aucLine+64,"HARD            \n", 17);
            i=write(iFd,(char *)aucLine, 81);
            assert(i==81);
            ulCrc=EP_CCITT_CRC16(aucLine,81,ulCrc);
        }
        else if(plink->LinkSwitchMode==2)
        {
            SI_Tag_Str_Cpy(aucLine+64,"SOFT            \n", 17);
            i=write(iFd,(char *)aucLine,  81);
            assert(i==81);
            ulCrc=EP_CCITT_CRC16(aucLine,81,ulCrc);
        }
        else if(plink->LinkSwitchMode==3)
        {
            SI_Tag_Str_Cpy((char *)aucLine+64,"AND             \n", 17);
            i=write(iFd, (char *)aucLine, 81);
            assert(i==81);
            ulCrc=EP_CCITT_CRC16(aucLine,81,ulCrc);
        }
        else if(plink->LinkSwitchMode==4)
        {
            SI_Tag_Str_Cpy(aucLine+64,"OR              \n", 17);
            i=write(iFd, (char *)aucLine, 81);
            assert(i==81);
            ulCrc=EP_CCITT_CRC16(aucLine,81,ulCrc);
        }

        else
        {
            if (pucOld)
            {
                for (puc=pucOld+81; puc<pucOld+ulOldLen; puc+=81)
                {
                    if(!memcmp(puc, aucLine, 64))
                    {
                        if (!memcmp(puc+64, "HARD            \n", 17)||
                                !memcmp(puc+64, "SOFT            \n", 17)||
                                !memcmp(puc+64, "AND             \n", 17)||
                                !memcmp(puc+64, "OR              \n", 17))
                        {
                            /*Found in old link mode file, inherit the old mode*/
                            i=write(iFd, puc, 81);
                            assert(i==81);
                            ulCrc=EP_CCITT_CRC16(puc,81,ulCrc);
                            break;
                        }


                    }

                }
                if (puc<pucOld+ulOldLen)
                    continue;
            }


            /* Not found in old link mode file, default hard mode*/
#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)  /* EDP03平台和励磁平台 */
            memcpy(aucLine+64, "SOFT            \n", 17);
#else
            memcpy(aucLine+64, "AND             \n", 17);
#endif
            i=write(iFd, aucLine, 81);
            assert(i==81);
            ulCrc=EP_CCITT_CRC16(aucLine,81,ulCrc);
        }
    }

    if (pucOld)
        EP_free(pucOld);

    sts=FT_End_Update(EP_LINK_MODE_FILE, iFd);
    assert(sts==EP_SUCCESS);

    sts=FT_Wr_INI_CRC("[CRC]",CRC_ITEM_LINKMODE,ulCrc);
    assert (sts != EP_ERROR);

    if(ENG_MODE == 1)
    {
        LOG_Write(LOG_OPRATE, "Successful create new switch mode file.\n", NULL);
    }
    else if(ENG_MODE == 0)
    {
        LOG_Write(LOG_OPRATE, "成功创建新的压板模式文件.\n", NULL);
    }
}

/***********************************************************************
* SI_New_Func_File - 生成新的功能文件
*
* RETURNS: 无
*
*/
static void SI_New_Func_File(void)
{
    uint8_t aucLine[81];
    uint8_t *pucOld;
    uint8_t *puc;
    int i;
    int iIdx;
    int iFd;
    const SC_SUB_LGC_ITEM *psublgc;
    uint32_t ulOldLen;
    EP_STATUS sts;

    uint16_t ulCrc=0;

    if ((pucOld=FT_File_To_Mem(EP_FUNC_STS_FILE, &ulOldLen))!=NULL)
    {
        if (ulOldLen%81 || ulOldLen<3*81 || memcmp(pucOld, pucFuncFileHead_g, 81))
        {
            /* File length or header not valid. */
            assert(FALSE);
            EP_free(pucOld);
            pucOld=NULL;
        }
    }

    iFd=FT_Bgn_Update(EP_FUNC_STS_FILE);
    assert(iFd>=0);

    i=write(iFd, (char*)pucFuncFileHead_g, 81);
    assert(i==81);
    ulCrc=EP_CCITT_CRC16((uint8_t *)pucFuncFileHead_g,81,ulCrc);

    SI_Tag_Str_Cpy(aucLine, "__ReportSN__", 64);
    if (pucOld && !memcmp(pucOld+81, aucLine, 64) &&
            isxdigit(pucOld[81*2+64]) && isxdigit(pucOld[81*2+64+1]) &&
            isxdigit(pucOld[81*2+64+2]) && isxdigit(pucOld[81*2+64+3]) &&
            !memcmp(pucOld+81*2+64+4, "              \n", 13))
    {
        i=write(iFd, pucOld+81, 81);
        assert(i==81);
        ulCrc=EP_CCITT_CRC16(pucOld+81,81,ulCrc);
    }
    else
    {
        memcpy(aucLine+64, "0000            \n", 17);
        i=write(iFd, aucLine, 81);
        assert(i==81);
        ulCrc=EP_CCITT_CRC16(aucLine,81,ulCrc);
    }

    SI_Tag_Str_Cpy(aucLine, "__RunSetArea__", 64);
    if (pucOld && !memcmp(pucOld+81*2, aucLine, 64) &&
            isxdigit(pucOld[81*2+64]) && isxdigit(pucOld[81*2+64+1]) &&
            !memcmp(pucOld+81*2+64+2, "              \n", 15))
    {
        i=write(iFd, pucOld+81*2, 81);
        assert(i==81);
        ulCrc=EP_CCITT_CRC16(pucOld+81*2,81,ulCrc);
    }
    else
    {
        memcpy(aucLine+64, "01              \n", 17);
        i=write(iFd, aucLine, 81);
        assert(i==81);
        ulCrc=EP_CCITT_CRC16(aucLine,81,ulCrc);
    }

    for (iIdx=0; iIdx<iSubLgcNum_g; iIdx++)
    {
        psublgc=SC_Get_Sub_Lgc_Attr(iIdx);

        SI_Tag_Str_Cpy(aucLine, psublgc->aucName, 64);

        if (pucOld)
        {
            for (puc=pucOld+81*3; puc<pucOld+ulOldLen; puc+=81)
            {
                if (!memcmp(puc, aucLine, 64) &&
                        (!memcmp(puc+64, "RUN             \n", 17) ||
                         !memcmp(puc+64, "EXIT            \n", 17)))
                {
                    i=write(iFd, puc, 81);
                    assert(i==81);
                    ulCrc=EP_CCITT_CRC16(puc,81,ulCrc);

                    break;
                }
            }
            if (puc<pucOld+ulOldLen)
                continue;
        }

        /* Not found in old file.根据默认值设置 */
        if(psublgc_g[iIdx].bRun==TRUE)
            memcpy(aucLine+64, "RUN             \n", 17);
        else
            memcpy(aucLine+64, "EXIT            \n", 17);
        i=write(iFd, aucLine, 81);
        assert(i==81);
        ulCrc=EP_CCITT_CRC16(aucLine,81,ulCrc);
    }

    if (pucOld)
        EP_free(pucOld);

    Set_FunSts_Wr_Sts(1);

    sts=FT_End_Update(EP_FUNC_STS_FILE, iFd);
    assert(sts==EP_SUCCESS);

    sts=FT_Wr_INI_CRC("[CRC]",CRC_ITEM_FUNC,ulCrc);
    assert (sts != EP_ERROR);

    Set_FunSts_Wr_Sts(0);

    if(ENG_MODE==1)
        LOG_Write(LOG_OPRATE, "Successful create new protect status file.\n", NULL);
    else if(ENG_MODE==0)
        LOG_Write(LOG_OPRATE, "成功创建新的保护功能状态文件.\n", NULL);

}

/***********************************************************************
* SI_New_Inner_Set - 生成新的内部定值文件
*
* RETURNS: 无
*
*/
static void SI_New_Inner_Set(void)
{
    int iFd;
    uint8_t aucBuf[27+MAX_ID_LEN];
    uint8_t *puc;
    const SC_SET_PAGE *psetpg;
    SC_SET_ITEM *psetOld;
    int iOldNum = 0;
    SC_SET_ITEM *pset;
    SC_SET_ITEM *psetRd;
    int iWrLen;
    int i;
    EP_STATUS sts;
    STATUS vxsts;
    uint16_t LastCRC=0;
    BOOL bhasCRC = TRUE;  /* 缺省使用CRC校验 */
    uint8_t temp = 0; /* 临时状态 */

    if ((iFd = open(EP_INNER_SET_FILE, O_RDONLY, 0)) != ERROR)
    {
        /* 读取原有文件配置项内容,项数,是否CRC校验标志
         * 如果文件有效,是否校验标志由当前文件决定
         */
        psetOld = SC_Rd_Inner_Set(iFd, &iOldNum, &bhasCRC);

        vxsts=close(iFd);
        assert (vxsts == OK);
    }
    else
    {
        psetOld=NULL;
    }

    psetpg=SC_Get_Set_Pg_Attr(0);

    /* 读出有效时进行项数判断
     * 如不相等则忽略读出结果
     * 不直接删除文件,留待以下操作处理
     */
    if (psetOld != NULL)
    {
        if (iOldNum != psetpg->iSetNum)
        {
            SC_Free_Set_Mem(psetOld, iOldNum);
            psetOld = NULL;
        }
    }

    iFd=FT_Bgn_Update(EP_INNER_SET_FILE);
    assert(iFd>=0);

    aucBuf[0]=0x93;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x6C;
    aucBuf[4]=LO8(GetInnerProtocolVer());
    aucBuf[5]=HI8(GetInnerProtocolVer());
    if(bhasCRC)
        aucBuf[6]=1;
    else
        aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    assert(psetpg->iSetNum<=255);
    aucBuf[9]=psetpg->iSetNum;


    iWrLen=write(iFd, aucBuf, 10);
    assert(iWrLen==10);
    LastCRC = EP_CCITT_CRC16(aucBuf,10,LastCRC);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (pset=psetpg->pset; pset<psetpg->pset+psetpg->iSetNum; pset++)
    {
        temp = 0;
        aucBuf[0]=pset-psetpg->pset;
        aucBuf[1]=strlen(pset->aucName);


        iWrLen=write(iFd, aucBuf, 2);
        assert(iWrLen==2);
        LastCRC = EP_CCITT_CRC16(aucBuf,2,LastCRC);



        iWrLen=write(iFd, pset->aucName, aucBuf[1]);
        assert(iWrLen==aucBuf[1]);
        LastCRC = EP_CCITT_CRC16(pset->aucName,aucBuf[1],LastCRC);

        aucBuf[0]=pset->aucABRV[0];
        aucBuf[1]=pset->aucABRV[1];
        aucBuf[2]=pset->aucABRV[2];
        aucBuf[3]=pset->aucABRV[3];
        aucBuf[4]=pset->bIsPrvtUse;
        puc=aucBuf+5;
        if(pset->bIsPrvtUse)
        {
            /* 获取逻辑图名长度 */
            *puc = strlen(pset->psublgc->aucName);
            i=*puc++;
            memcpy(puc, pset->psublgc->aucName, i);
            puc+=i;
        }
        *puc++=pset->ucAttr;
        if(pset->bStdSet)
            temp |=0x01;
        *puc++=temp;
        *puc++ = 0;
        *puc++=pset->ucUnit;

        iWrLen=write(iFd, aucBuf, puc-aucBuf);
        assert(iWrLen==puc-aucBuf);
        LastCRC = EP_CCITT_CRC16(aucBuf,puc-aucBuf,LastCRC);

        puc=aucBuf;

        U32_TO_BYTES(puc, pset->valMax.ulVal);

        U32_TO_BYTES(puc+4, pset->valMin.ulVal);

        U32_TO_BYTES(puc+8, pset->valDft.ulVal);

        if(pset->ucUnit==0x68)/*若是字符串  张云2008-7-19日   */
        {
            strncpy(puc+12,pset->aucDftStr,pset->valDft.ulVal);
            i=12+pset->valDft.ulVal;
        }
        else
        {
            i=12;
        }

        iWrLen=write(iFd, aucBuf, i);
        assert(iWrLen==i);
        LastCRC = EP_CCITT_CRC16(aucBuf,i,LastCRC);

        puc=aucBuf;/*若是字符串  张云2008-7-19日   */
        U32_TO_BYTES(puc, pset->valNow.ulVal);

        if(pset->ucUnit==0x68)/*若是字符串  张云2008-7-19日   */
        {
            strncpy(puc+4,pset->aucNowStr,pset->valNow.ulVal);
            i=4+pset->valNow.ulVal;
        }
        else
        {
            i=4;
        }



        if (psetOld)
        {
            for (psetRd=psetOld; psetRd<psetOld+iOldNum; psetRd++)
            {
                if (psetRd->ucUnit==pset->ucUnit &&
                        psetRd->aucABRV[0]==pset->aucABRV[0] &&
                        psetRd->aucABRV[1]==pset->aucABRV[1] &&
                        psetRd->aucABRV[2]==pset->aucABRV[2] &&
                        psetRd->aucABRV[3]==pset->aucABRV[3] &&
                        !strcmp(psetRd->aucName, pset->aucName) &&
                        !strcmp(psetRd->pucUnitName, pset->pucUnitName)
                        &&(pset->ucAttr == 0))		/* 非索引定值 */
                {
                    /*2008-1-30日 张云merge修改，原来有一个BUG，   */
                    if ((IS_INT32_SET(psetRd->ucUnit) &&
                            psetRd->valNow.lVal<=pset->valMax.lVal &&
                            psetRd->valNow.lVal>=pset->valMin.lVal) ||
                            (IS_UINT32_SET(psetRd->ucUnit) &&
                             psetRd->valNow.ulVal<=pset->valMax.ulVal &&
                             psetRd->valNow.ulVal>=pset->valMin.ulVal) ||
                            (IS_FLT_SET(psetRd->ucUnit) &&
                             psetRd->valNow.fVal<=pset->valMax.fVal &&
                             psetRd->valNow.fVal>=pset->valMin.fVal))
                    {
                        /* 原有文件当前值替换设定当前值
                         * 要求原有文件缺省值与当前缺省值一致
                         */
                        if (pset->ucUnit == 0x68)
                        {
                            /* 若是字符串 */
                            if (!strcmp(psetRd->aucDftStr, pset->aucDftStr))
                            {
                                /* 字符串一致 */
                                U32_TO_BYTES(puc, psetRd->valNow.ulVal);
                                strncpy(puc+4, psetRd->aucNowStr, psetRd->valNow.ulVal);
                                i=4+psetRd->valNow.ulVal;
                            }
                        }
                        else if (psetRd->valDft.ulVal == pset->valDft.ulVal)
                        {
                            /* 非字符串 */
                            U32_TO_BYTES(puc, psetRd->valNow.ulVal);
                            i=4;
                        }
                    }
                    break;
                }
            }
        }

        iWrLen=write(iFd, aucBuf, i);
        assert(iWrLen==i);
        LastCRC = EP_CCITT_CRC16(aucBuf,i,LastCRC);

        puc=aucBuf;  /*张云2008-7-19日   */
        i=strlen(pset->pucUnitName);
        assert(i<=1024);
        *puc++=LO8(i);
        *puc++=HI8(i);


        iWrLen=write(iFd, aucBuf, 2);
        assert(iWrLen==2);
        LastCRC = EP_CCITT_CRC16(aucBuf,2,LastCRC);


        iWrLen=write(iFd, pset->pucUnitName, i);
        assert(iWrLen==i);
        LastCRC = EP_CCITT_CRC16(pset->pucUnitName,i,LastCRC);
    }

    aucBuf[0]=0x9C;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x63;


    iWrLen=write(iFd, aucBuf, 4);
    assert(iWrLen==4);
    LastCRC = EP_CCITT_CRC16(aucBuf,4,LastCRC);

    if(bhasCRC)
    {
        aucBuf[0] = LO8(LastCRC);
        aucBuf[1] = HI8(LastCRC);
        iWrLen=write(iFd, aucBuf, 2);
        LastCRC = EP_CCITT_CRC16(aucBuf,2,LastCRC);
    }

    sts=FT_End_Update(EP_INNER_SET_FILE, iFd);
    assert(sts==EP_SUCCESS);

    sts=FT_Wr_INI_CRC("[CRC]",CRC_ITEM_NBSET,LastCRC);
    assert (sts != EP_ERROR);

    if (psetOld)
        SC_Free_Set_Mem(psetOld, iOldNum);

    if(ENG_MODE==1)
        LOG_Write(LOG_OPRATE, "Successful create new INNER SETTING file.\n", NULL);
    else if(ENG_MODE==0)
        LOG_Write(LOG_OPRATE, "成功创建新的内部定值文件.\n", NULL);
}

/***********************************************************************
* SI_New_CK_Set - 生成新的测控定值文件
*
* RETURNS: 无
*
*/
static void SI_New_CK_Set(void)
{
    int iFd;
    uint8_t aucBuf[35];
    const SC_SET_ITEM *psetHd;
    SC_SET_ITEM *psetOld;
    int iOldNum;
    const SC_SET_ITEM *pset;
    SC_SET_ITEM *psetRd = NULL;
    int iWrLen;
    int i;
    EP_STATUS sts;
    STATUS vxsts;
    uint8_t temp;
    uint16_t LastCRC=0;
    BOOL bhasCRC = TRUE;  /* 缺省使用CRC校验 */

    BOOL   bOldSetNowValIsValid;  /*2008-7-21日 张云为字符串功能添加  */

    if ((iFd=open(EP_CK_SET_FILE, O_RDONLY, 0))!=ERROR)
    {
        psetOld=SC_Rd_CK_Set(iFd, &iOldNum,&bhasCRC);

        vxsts=close(iFd);
        assert(vxsts==OK);
    }
    else
    {
        psetOld=NULL;
    }
    psetHd=SC_Get_CK_Set();

    /* 读出有效时进行项数判断
     * 如不相等则忽略读出结果
     * 不直接删除文件,留待以下操作处理
     */
    if (psetOld != NULL)
    {
        if (iOldNum != iCkSetNum_g)
        {
            SC_Free_Set_Mem(psetOld, iOldNum);
            psetOld = NULL;
        }
    }

    iFd=FT_Bgn_Update(EP_CK_SET_FILE);
    assert(iFd>=0);

    aucBuf[0]=0xD1;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x1F;
    aucBuf[4]=LO8(GetInnerProtocolVer());
    aucBuf[5]=HI8(GetInnerProtocolVer());
    if(bhasCRC)
        aucBuf[6]=1;
    else
        aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    assert(iCkSetNum_g<=255);
    aucBuf[9]=iCkSetNum_g;


    iWrLen=write(iFd, aucBuf, 10);
    assert(iWrLen==10);
    LastCRC = EP_CCITT_CRC16(aucBuf,10,LastCRC);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (pset=psetHd; pset<psetHd+iCkSetNum_g; pset++)
    {
        temp=0;
        aucBuf[0]=pset-psetHd;
        aucBuf[1]=strlen(pset->aucName);


        iWrLen=write(iFd, aucBuf, 2);
        assert(iWrLen==2);
        LastCRC = EP_CCITT_CRC16(aucBuf,2,LastCRC);


        iWrLen=write(iFd, (char *)pset->aucName, aucBuf[1]);
        assert(iWrLen==aucBuf[1]);
        LastCRC = EP_CCITT_CRC16((uint8_t *)pset->aucName,aucBuf[1],LastCRC);


        memset(aucBuf,0,35);
        aucBuf[0]=pset->aucABRV[0];
        aucBuf[1]=pset->aucABRV[1];
        aucBuf[2]=pset->aucABRV[2];
        aucBuf[3]=pset->aucABRV[3];
        if(pset->bStdSet)
            temp |= 0x01;
        if(pset->bAutoSet)
            temp |= 0x02;
        aucBuf[4]=temp;

        iWrLen=write(iFd, aucBuf, 21);
        assert(iWrLen==21);

        U32_TO_BYTES(aucBuf+21, pset->valStep.ulVal);  /* 定值修改步长 */
        iWrLen=write(iFd, aucBuf+21, 14);
        assert(iWrLen==14);

        LastCRC = EP_CCITT_CRC16(aucBuf,35,LastCRC);

        aucBuf[0]=pset->ucUnit;

        U32_TO_BYTES(aucBuf+1, pset->valMax.ulVal);

        U32_TO_BYTES(aucBuf+5, pset->valMin.ulVal);

        U32_TO_BYTES(aucBuf+9, pset->valDft.ulVal);

        iWrLen=write(iFd, aucBuf, 13);/*针对字符串，进行处理  张云2008-7-19日   */
        assert(iWrLen==13);
        LastCRC = EP_CCITT_CRC16(aucBuf,13,LastCRC);

        if(pset->ucUnit==0x68)
        {

            iWrLen=write(iFd, (uint8_t *)pset->aucDftStr, pset->valDft.ulVal);
            assert(iWrLen==pset->valDft.ulVal);
            LastCRC = EP_CCITT_CRC16((uint8_t *)pset->aucDftStr, pset->valDft.ulVal,LastCRC);
        }

        /*处理当前值  */
        bOldSetNowValIsValid=FALSE;
        U32_TO_BYTES(aucBuf, pset->valNow.ulVal);


        if (psetOld)
        {
            for (psetRd=psetOld; psetRd<psetOld+iOldNum; psetRd++)
            {
                /*2008-1-30日 张云merge修改，原来有一个BUG，   */
                if (psetRd->ucUnit==pset->ucUnit &&
                        psetRd->aucABRV[0]==pset->aucABRV[0] &&
                        psetRd->aucABRV[1]==pset->aucABRV[1] &&
                        psetRd->aucABRV[2]==pset->aucABRV[2] &&
                        psetRd->aucABRV[3]==pset->aucABRV[3] &&
                        !strcmp(psetRd->aucName, pset->aucName) &&
                        !strcmp(psetRd->pucUnitName, pset->pucUnitName))
                {
                    if ((IS_INT32_SET(psetRd->ucUnit) &&
                            psetRd->valNow.lVal<=pset->valMax.lVal &&
                            psetRd->valNow.lVal>=pset->valMin.lVal) ||
                            (IS_UINT32_SET(psetRd->ucUnit) &&
                             psetRd->valNow.ulVal<=pset->valMax.ulVal &&
                             psetRd->valNow.ulVal>=pset->valMin.ulVal) ||
                            (IS_FLT_SET(psetRd->ucUnit) &&
                             psetRd->valNow.fVal<=pset->valMax.fVal &&
                             psetRd->valNow.fVal>=pset->valMin.fVal))
                    {

                        /* 原有文件当前值替换设定当前值
                         * 要求原有文件缺省值与当前缺省值一致
                         */
                        if (pset->ucUnit == 0x68)
                        {
                            /* 字符串 */
                            if (!strcmp(psetRd->aucDftStr, pset->aucDftStr))
                            {
                                /* 一致 */
                                U32_TO_BYTES(aucBuf, psetRd->valNow.ulVal);
                                bOldSetNowValIsValid=TRUE;
                            }
                        }
                        else if (psetRd->valDft.ulVal == pset->valDft.ulVal)
                        {
                            U32_TO_BYTES(aucBuf, psetRd->valNow.ulVal);
                        }
                    }
                    break;
                }
            }
        }

        iWrLen=write(iFd, aucBuf, 4);/*针对字符串，进行处理  张云2008-7-21日   */
        assert(iWrLen==4);
        LastCRC = EP_CCITT_CRC16(aucBuf,4,LastCRC);

        if(pset->ucUnit==0x68)
        {
            if(bOldSetNowValIsValid)
            {
                iWrLen=write(iFd, psetRd->aucNowStr, psetRd->valNow.ulVal);
                assert(iWrLen==psetRd->valNow.ulVal);
                LastCRC = EP_CCITT_CRC16(psetRd->aucNowStr, psetRd->valNow.ulVal,LastCRC);
            }
            else
            {
                iWrLen=write(iFd, (uint8_t *)pset->aucNowStr, pset->valNow.ulVal);
                assert(iWrLen==pset->valNow.ulVal);
                LastCRC = EP_CCITT_CRC16((uint8_t *)pset->aucNowStr, pset->valNow.ulVal,LastCRC);

            }
        }

        i=strlen(pset->pucUnitName);
        assert(i<=1024);

        aucBuf[0]=LO8(i);
        aucBuf[1]=HI8(i);

        iWrLen=write(iFd, aucBuf, 2);
        assert(iWrLen==2);
        LastCRC = EP_CCITT_CRC16(aucBuf,2,LastCRC);


        iWrLen=write(iFd, pset->pucUnitName, i);
        assert(iWrLen==i);
        LastCRC = EP_CCITT_CRC16(pset->pucUnitName, i,LastCRC);

    }

    aucBuf[0]=0x6F;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x36;


    iWrLen=write(iFd, aucBuf, 4);
    assert(iWrLen==4);
    LastCRC = EP_CCITT_CRC16(aucBuf,4,LastCRC);
    if(bhasCRC)
    {
        aucBuf[0] = LO8(LastCRC);
        aucBuf[1] = HI8(LastCRC);
        iWrLen=write(iFd, aucBuf, 2);

        LastCRC = EP_CCITT_CRC16(aucBuf,2,LastCRC);
    }

    sts=FT_End_Update(EP_CK_SET_FILE, iFd);
    assert(sts==EP_SUCCESS);


    sts=FT_Wr_INI_CRC("[CRC]",CRC_ITEM_CKSET,LastCRC);
    assert (sts != EP_ERROR);

    if (psetOld)
        SC_Free_Set_Mem(psetOld, iOldNum);

    if(ENG_MODE==1)
        LOG_Write(LOG_OPRATE, "Successful create new measure/control SETTING file.\n", NULL);
    else if(ENG_MODE==0)
        LOG_Write(LOG_OPRATE, "成功创建新的测控定值文件.\n", NULL);
}
/***********************************************************************
* SI_New_CK_Set - 生成新的测控定值文件
*
* RETURNS: 无SI_Wr_New_CK_Set
*
*/
void SI_Wr_New_CK_Set(void)
{

    int iFd;
    uint8_t aucBuf[35];
    const SC_SET_ITEM *psetHd;
    const SC_SET_ITEM *pset;
    int iWrLen;
    int i;
    EP_STATUS sts;
    uint8_t temp;
    STATUS vxsts;
    uint16_t LastCRC=0;

    psetHd=SC_Get_CK_Set();

    vxsts=semTake(semCkCRCIni_g, WAIT_FOREVER);
    assert(vxsts==OK);

    iFd=FT_Bgn_Update(EP_CK_SET_FILE);
    assert(iFd>=0);

    aucBuf[0]=0xD1;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x1F;
    aucBuf[4]=LO8(GetInnerProtocolVer());
    aucBuf[5]=HI8(GetInnerProtocolVer());
    aucBuf[6]=0;
    aucBuf[7]=0;
    aucBuf[8]=0;

    assert(iCkSetNum_g<=255);
    aucBuf[9]=iCkSetNum_g;

    iWrLen=write(iFd, aucBuf, 10);
    assert(iWrLen==10);
    LastCRC = EP_CCITT_CRC16(aucBuf,10,LastCRC);

    memset(aucBuf, 0, sizeof(aucBuf));
    for (pset=psetHd; pset<psetHd+iCkSetNum_g; pset++)
    {
        temp = 0;
        aucBuf[0]=pset-psetHd;
        aucBuf[1]=strlen(pset->aucName);

        iWrLen=write(iFd, aucBuf, 2);
        assert(iWrLen==2);
        LastCRC = EP_CCITT_CRC16(aucBuf,2,LastCRC);

        iWrLen=write(iFd, (char *)pset->aucName, aucBuf[1]);
        assert(iWrLen==aucBuf[1]);
        LastCRC = EP_CCITT_CRC16((uint8_t *)pset->aucName,aucBuf[1],LastCRC);

        memset(aucBuf,0,35);
        aucBuf[0]=pset->aucABRV[0];
        aucBuf[1]=pset->aucABRV[1];
        aucBuf[2]=pset->aucABRV[2];
        aucBuf[3]=pset->aucABRV[3];
        if(pset->bStdSet)
            temp |= 0x01;
        if(pset->bAutoSet)
            temp |= 0x02;
        aucBuf[4]=temp;
        iWrLen=write(iFd, aucBuf, 21);
        assert(iWrLen==21);

        U32_TO_BYTES(aucBuf+21, pset->valStep.ulVal);  /* 定值修改步长 */
        iWrLen=write(iFd, aucBuf+21, 14);
        assert(iWrLen==14);

        LastCRC = EP_CCITT_CRC16(aucBuf,35,LastCRC);

        aucBuf[0]=pset->ucUnit;

        U32_TO_BYTES(aucBuf+1, pset->valMax.ulVal);

        U32_TO_BYTES(aucBuf+5, pset->valMin.ulVal);

        U32_TO_BYTES(aucBuf+9, pset->valDft.ulVal);

        iWrLen=write(iFd, aucBuf, 13);/*针对字符串，进行处理  张云2008-7-19日   */
        assert(iWrLen==13);
        LastCRC = EP_CCITT_CRC16(aucBuf,13,LastCRC);
        if(pset->ucUnit==0x68)
        {
            iWrLen=write(iFd, (uint8_t *)pset->aucDftStr, pset->valDft.ulVal);
            assert(iWrLen==pset->valDft.ulVal);
            LastCRC = EP_CCITT_CRC16((uint8_t *)pset->aucDftStr,pset->valDft.ulVal,LastCRC);
        }
        U32_TO_BYTES(aucBuf, pset->valNow.ulVal);
        iWrLen=write(iFd, aucBuf, 4);/*针对字符串，进行处理  张云2008-7-21日   */
        assert(iWrLen==4);
        LastCRC = EP_CCITT_CRC16(aucBuf,4,LastCRC);
        if(pset->ucUnit==0x68)
        {
            iWrLen=write(iFd, (uint8_t *)pset->aucNowStr, pset->valNow.ulVal);
            assert(iWrLen==pset->valNow.ulVal);
            LastCRC = EP_CCITT_CRC16((uint8_t *)pset->aucNowStr, pset->valNow.ulVal,LastCRC);
        }


        i=strlen(pset->pucUnitName);
        assert(i<=1024);
        aucBuf[0]=LO8(i);
        aucBuf[1]=HI8(i);

        iWrLen=write(iFd, aucBuf, 2);
        assert(iWrLen==2);
        LastCRC = EP_CCITT_CRC16(aucBuf,2,LastCRC);

        iWrLen=write(iFd, pset->pucUnitName, i);
        assert(iWrLen==i);
        LastCRC = EP_CCITT_CRC16(pset->pucUnitName,i,LastCRC);

    }

    aucBuf[0]=0x6F;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x36;

    iWrLen=write(iFd, aucBuf, 4);
    assert(iWrLen==4);
    LastCRC = EP_CCITT_CRC16(aucBuf,4,LastCRC);

    Set_Ckset_Wr_Sts(1); /* 测控定值写入 */

    sts=FT_End_Update(EP_CK_SET_FILE, iFd);
    assert(sts==EP_SUCCESS);

    Write_Ckset_CRC();
    Set_Ckset_Wr_Sts(0); /* 测控定值写入结束 */

    vxsts=semGive(semCkCRCIni_g);
    assert(vxsts==OK);

    if(ENG_MODE==1)
        LOG_Write(LOG_OPRATE, "Successful create new measure/control SETTING file.\n", NULL);
    else if(ENG_MODE==0)
        LOG_Write(LOG_OPRATE, "成功创建参数定值文件.\n", NULL);
}
/***********************************************************************
* SI_New_AI_Gain_Set - 生成新的物理通道系数文件
*
* RETURNS: 无
*
*/
void SI_New_AI_Gain_Set(void)
{
    uint8_t aucTempFile[FULL_NAME_LEN+1];
    int i,j;
    int iFd;
    uint8_t aucBuf[6];
    EP_STATUS sts;

    uint16_t ulCrc=0;



    sts = EP_SUCCESS;
    FT_Temp_Name_New(aucTempFile, EP_AI_GAIN_FILE);

    iFd=FT_Bgn_Update(EP_AI_GAIN_FILE);
    assert(iFd>=0);

    aucBuf[0]=0xA2;		/* 文件头符 */
    aucBuf[1]=0x0;
    aucBuf[2]=0x0;
    aucBuf[3]=0x5D;

    i=write(iFd, aucBuf, 4);
    assert(i==4);
    ulCrc =EP_CCITT_CRC16(aucBuf,4,ulCrc);

    aucBuf[0]=0x01;		/* 版本号 */
    aucBuf[1]=0x10;
    i=write(iFd, aucBuf, 2);
    assert(i==2);
    ulCrc =EP_CCITT_CRC16(aucBuf,2,ulCrc);

    aucBuf[0]=0x0;		/* 保留 */
    aucBuf[1]=0x0;
    aucBuf[2]=0x0;

    ulCrc =EP_CCITT_CRC16(aucBuf,3,ulCrc);

    i=write(iFd, aucBuf, 3);
    assert(i==3);

    aucBuf[0] = iHwAiChNum_g;		/* 物理通道总数 */
    i=write(iFd, aucBuf, 1);
    assert(i==1);
    ulCrc =EP_CCITT_CRC16(aucBuf,1,ulCrc);

    LOG_Dbg_Msg("Begin to calibrate the gain coefficient!\n", 0, 0, 0, 0, 0, 0);
    for (j=0; j<iHwAiChNum_g; j++)
    {
        taskDelay(1);		/* 延时 */
        aucBuf[0] = j;			/* 编号 */
        i=write(iFd, aucBuf, 1);
        assert(i==1);
        ulCrc =EP_CCITT_CRC16(aucBuf,1,ulCrc);

        FLT_TO_BYTES(aucBuf, 0.0);		/* 偏置系数, 默认为0 */
        i=write(iFd, aucBuf, 4);
        assert(i==4);
        ulCrc =EP_CCITT_CRC16(aucBuf,4,ulCrc);

        FLT_TO_BYTES(aucBuf, 1.0);		/* 增益系数, 默认为1.0 */
        i=write(iFd, aucBuf, 4);
        assert(i== 4);
        ulCrc =EP_CCITT_CRC16(aucBuf,4,ulCrc);
    }

    aucBuf[0]=0xA8;		/* 保留 */
    aucBuf[1]=0x0;
    aucBuf[2]=0x0;
    aucBuf[3]=0x57;
    i=write(iFd, aucBuf, 4);
    assert(i==4);
    ulCrc =EP_CCITT_CRC16(aucBuf,4,ulCrc);

    sts=FT_End_Update(EP_AI_GAIN_FILE, iFd);
    assert(sts==EP_SUCCESS);

    sts=FT_Wr_INI_CRC("[CRC]",CRC_ITEM_HDCOF,ulCrc);
    assert (sts != EP_ERROR);



    if(ENG_MODE == 0)
    {
        LOG_Write(LOG_OPRATE, "创建新的物理通道系数文件.\n", NULL);
    }
    else if(ENG_MODE == 1)
    {
        LOG_Write(LOG_OPRATE, "New coefficient of phisical channel is created.\n", NULL);
    }
}

/***********************************************************************
* SI_New_CL_Gain_Set - 生成新的测量量系数文件
*
* RETURNS: 无
*
*/
void SI_New_CL_Gain_Set(void)
{
    int iFd;
    uint8_t aucBuf[10];
    EP_STATUS sts;
    int i,j;

    uint16_t ulCrc=0;

    iFd=FT_Bgn_Update(EP_CL_GAIN_FILE);
    assert(iFd>=0);

    aucBuf[0]=0xD3;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x2D;
    aucBuf[4]=LO8(GetInnerProtocolVer());
    aucBuf[5]=HI8(GetInnerProtocolVer());

    aucBuf[8]=HI8(iMeaValueNum_g);
    aucBuf[9]=LO8(iMeaValueNum_g);

    i=write(iFd, aucBuf, 10);
    assert(i==10);
    ulCrc =EP_CCITT_CRC16(aucBuf,10,ulCrc);

    j=0;
    for (j=0; j<iMeaValueNum_g; j++)
    {
        aucBuf[0]=j;		/* 编号 */
        FLT_TO_BYTES(aucBuf+1, 0.0);		/* 偏置系数, 默认为0.0 */
        FLT_TO_BYTES(aucBuf+5, 1.0);		/* 增益系数, 默认为1.0  */
        i=write(iFd, aucBuf, 9);
        assert(i==9);
        ulCrc =EP_CCITT_CRC16(aucBuf,9,ulCrc);
    }

    aucBuf[0]=0xA9;
    aucBuf[1]=0;
    aucBuf[2]=0;
    aucBuf[3]=0x58;

    i=write(iFd, aucBuf, 4);
    assert(i==4);
    ulCrc =EP_CCITT_CRC16(aucBuf,4,ulCrc);

    sts=FT_End_Update(EP_CL_GAIN_FILE, iFd);
    assert(sts==EP_SUCCESS);

    sts=FT_Wr_INI_CRC("[CRC]",CRC_ITEM_CLCOF,ulCrc);
    assert (sts != EP_ERROR);

    if(ENG_MODE == 0)
    {
        LOG_Write(LOG_OPRATE, "创建新的测量量系数文件.\n", NULL);
    }
    else if(ENG_MODE == 1)
    {
        LOG_Write(LOG_OPRATE, "New coefficient of measurement is created.\n", NULL);
    }

}

/***********************************************************************
* SI_Tag_Str_Cpy - 标签到字符串
*
* RETURNS: 无
*
*/
void SI_Tag_Str_Cpy(
    uint8_t *pucD,			/* 目标 */
    const uint8_t *pucS, 				/* 源*/
    int iLen		/* 长度 */
)
{
    int i;

    assert(pucD && pucS);

    strncpy(pucD, pucS, iLen);

    i=strlen(pucS);
    assert(i && i<=iLen);

    if (i<iLen)
        memset(pucD+i, ' ', iLen-i);
}

/***********************************************************************
* SI_Chg_Sts_File_Item - 更换文件项值
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS SI_Chg_Sts_File_Item(
    const uint8_t *pucFile,
    const uint8_t *pucTitle,
    int iTotalLine,
    int iChgLine,
    const uint8_t *pucVal
)
{
    /*2013-7-23 ZY  去掉assert */
    uint8_t *pucBuf;
    uint32_t ulLen;
    int iFd;
    int i;
    EP_STATUS sts;

    if ((pucBuf=FT_File_To_Mem(pucFile, &ulLen))==NULL)
        return EP_FILE_ERR;

    if (ulLen!=81+iTotalLine*81 || memcmp(pucBuf, pucTitle, 81) ||
            (iFd=FT_Bgn_Update(pucFile))<0)
    {
        EP_free(pucBuf);
        return EP_FILE_ERR;
    }

    memcpy(pucBuf+81+iChgLine*81+64, pucVal, 16);

    i=write(iFd, pucBuf, ulLen);
    //assert(i==ulLen);
    if(!(i==ulLen))
    {
        EP_free(pucBuf);
        FT_Abort_Update(pucFile, iFd);
        return EP_FILE_ERR;
    }

    EP_free(pucBuf);

    sts=FT_End_Update(pucFile, iFd);
    //assert(sts==EP_SUCCESS);
    if(!(sts==EP_SUCCESS))
    {
        return EP_FILE_ERR;
    }

    return EP_SUCCESS;
}


/***********************************************************************
* SI_Chg_Link_Sts_File_Item - 更换压板文件项值
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS SI_Chg_Link_Sts_File_Item(
    const uint8_t *pucFile,
    const uint8_t *pucTitle,
    int iTotalLine,
    int iChgLine,
    const uint8_t *pucVal
)
{
    /*2013-7-23 ZY  去掉assert */
    uint8_t *pucBuf;
    uint32_t ulLen;
    int iFd;
    int i;
    EP_STATUS sts;
    uint16_t usCrc = 0;
    uint8_t aucLine[ITEM_LEN];
    uint8_t aucTmp[ITEM_LEN];

    if ((pucBuf=FT_File_To_Mem(pucFile, &ulLen))==NULL)
    {
        return EP_FILE_ERR;
    }

    /* 增加CRC, iTotalLine包含压板总状态
     */
    if (ulLen!=ITEM_LEN+iTotalLine*ITEM_LEN+ITEM_LEN
            || memcmp(pucBuf, pucTitle, ITEM_LEN) ||
            (iFd=FT_Bgn_Update(pucFile))<0)
    {
        EP_free(pucBuf);
        return EP_FILE_ERR;
    }

    /* 写入状态 */
    memcpy(pucBuf+ITEM_LEN+iChgLine*ITEM_LEN+ITEM_NAME_LEN,
           pucVal, ITEM_VALUE_LEN-1);

    /* 写入CRC */
    usCrc = EP_CCITT_CRC16(pucBuf, ITEM_LEN+iTotalLine*ITEM_LEN, usCrc);

    sprintf(aucTmp, "%04x", usCrc);
    SI_Tag_Str_Cpy(aucLine, aucTmp, ITEM_VALUE_LEN);

    memcpy(pucBuf+ITEM_LEN+iTotalLine*ITEM_LEN+ITEM_NAME_LEN,
           aucLine, ITEM_VALUE_LEN);

    i=write(iFd, pucBuf, ulLen);
    //assert(i==ulLen);
    if(!(i==ulLen))
    {
        EP_free(pucBuf);
        FT_Abort_Update(pucFile, iFd);
        return EP_FILE_ERR;
    }

    EP_free(pucBuf);

    sts=FT_End_Update(pucFile, iFd);
    //assert(sts==EP_SUCCESS);
    if(!(sts==EP_SUCCESS))
    {
        return EP_FILE_ERR;
    }

    return EP_SUCCESS;
}

/***********************************************************************
* SI_Chg_Sts_File_Multi_Item - 更换多个文件项值
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS SI_Chg_Sts_File_Multi_Item(
    const uint8_t *pucFile,
    const uint8_t *pucTitle,
    int iTotalLine,
    SI_File_Item * pChgItem,
    int iChgNum
)
{
    uint8_t *pucBuf;
    uint32_t ulLen;
    int iFd;
    int i;
    EP_STATUS sts;
    SI_File_Item *pTemp;

    assert(pChgItem);
    if ((pucBuf=FT_File_To_Mem(pucFile, &ulLen))==NULL)
        return EP_FILE_ERR;

    if (ulLen!=81+iTotalLine*81 || memcmp(pucBuf, pucTitle, 81) ||
            (iFd=FT_Bgn_Update(pucFile))<0)
    {
        EP_free(pucBuf);
        return EP_FILE_ERR;
    }
    for(pTemp=pChgItem; pTemp<pChgItem+iChgNum; pTemp++)
    {
        memcpy(pucBuf+81+(pTemp->iLine)*81+64, pTemp->aucVal, 16);
    }
    i=write(iFd, pucBuf, ulLen);
    assert(i==ulLen);

    EP_free(pucBuf);

    sts=FT_End_Update(pucFile, iFd);



    assert(sts==EP_SUCCESS);

    return EP_SUCCESS;
}

/***********************************************************************
* GetInnerProtocolVer - 获得支持的内部规约版本号
*
* RETURNS: 内部规约版本号
*
*/
uint16_t GetInnerProtocolVer()
{
    return   EP_INNER_PRTCL_VER;
}

/***********************************************************************
* GetSysSwVer - 获得平台软件版本号
*
* RETURNS: 平台软件版本号
*
*/
uint8_t* GetSysSwVer()
{
    printf("平台软件版本: %s\n", EP_SYS_SW_VER);
    return    EP_SYS_SW_VER;
}

#ifndef EDP03_INTELBOX_BUILD
/***********************************************************************
* GetMmiSwVer - 获得mmi版本号，如果没有设置，与平台程序一致
*
* RETURNS: 无
*
*/
void  GetMmiSwVer(
    uint8_t *ucMmiSysVer,
    int nStrlen
)
{
    uint16_t unLen=0;
    unLen=strlen(EP_SYS_SW_VER);
    if(unLen>=nStrlen||unLen==0)
    {
        return;
    }
    strncpy(ucMmiSysVer,EP_SYS_SW_VER,unLen);
    ucMmiSysVer[unLen]='\0';
    return ;
}
#endif

/***********************************************************************
* GetSysPlatLabel - 获得平台软件版本号
*
* RETURNS: 平台软件特征码
*
*/
uint16_t GetSysPlatLabel()
{
    return EP_CHARACTER_LABEL;
}

#ifdef VXWORKS_ROM

/***********************************************************************
* GetVxworksCRC - 获得VXWORKS CRC
*
* RETURNS: VxWorks校验码
*
*/
uint16_t GetVxworksCRC()
{
    return 0;
}

/***********************************************************************
* GetSysCRC - 获得系统CRC
*
* RETURNS: 系统校验码
*
*/
uint16_t GetSysCRC()
{
    uint16_t unHwCrc;
    uint16_t unSwCrc;
    uint16_t unLogicCrc;
    uint16_t unTmpCrc;
    uint8_t aucBuf[10];
    uint16_t unCrc=0;

    unHwCrc=FT_File_CRC16(EP_HW_CFG_FILE);
    aucBuf[0]=LO8(unHwCrc);
    aucBuf[1]=HI8(unHwCrc);
    unSwCrc=FT_File_CRC16(EP_SW_CFG_FILE);
    aucBuf[2]=LO8(unSwCrc);
    aucBuf[3]=HI8(unSwCrc);
    unLogicCrc=FT_File_CRC16(EP_LGC_CFG_FILE);
    aucBuf[4]=LO8(unLogicCrc);
    aucBuf[5]=HI8(unLogicCrc);

#if defined(EDP_DYNAMICLOAD)
    unTmpCrc=FT_File_CRC16(EP_EDP_APP_FILE);
    aucBuf[6]=LO8(unTmpCrc);
    aucBuf[7]=HI8(unTmpCrc);

    return  EP_CCITT_CRC16(aucBuf, 8, unCrc);
#else
    return  EP_CCITT_CRC16(aucBuf, 6, unCrc);
#endif  /* end of EDP_DYNAMICLOAD */
}

#else

/***********************************************************************
* GetVxworksCRC - 获得VXWORKS CRC
*
* RETURNS: VxWorks校验码
*
*/
uint16_t GetVxworksCRC()
{
    return FT_File_CRC16(EP_VX_RUN_FILE);
}

/***********************************************************************
* GetSysCRC - 获得系统CRC
*
* RETURNS: 系统校验码
*
*/
uint16_t GetSysCRC(void)
{
    uint16_t unVxCrc;
    uint16_t unHwCrc;
    uint16_t unSwCrc;
    uint16_t unLogicCrc;
    uint16_t unTmpCrc;
    uint8_t aucBuf[12];
    uint16_t unCrc=0;

    unHwCrc=FT_File_CRC16(EP_HW_CFG_FILE);
    aucBuf[0]=LO8(unHwCrc);
    aucBuf[1]=HI8(unHwCrc);
    unSwCrc=FT_File_CRC16(EP_SW_CFG_FILE);
    aucBuf[2]=LO8(unSwCrc);
    aucBuf[3]=HI8(unSwCrc);
    unLogicCrc=FT_File_CRC16(EP_LGC_CFG_FILE);
    aucBuf[4]=LO8(unLogicCrc);
    aucBuf[5]=HI8(unLogicCrc);

#if defined(EDP_DYNAMICLOAD)
    unTmpCrc=FT_File_CRC16(EP_EDP_APP_FILE);
    aucBuf[6]=LO8(unTmpCrc);
    aucBuf[7]=HI8(unTmpCrc);
    return  EP_CCITT_CRC16(aucBuf, 8, unCrc);
#else		/* 只有配置文件，主文件VxWorks放到平台软件中 */
    return  EP_CCITT_CRC16(aucBuf, 6, unCrc);
#endif  /* end of EDP_DYNAMICLOAD */
}
#endif		/* end of VXWORKS_ROM */
