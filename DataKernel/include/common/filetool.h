/* filetool.h - This file contains functions to operator files(INI, update...) */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01c, 29apr03, hdx Updated to version 1.0.
01b, 03mar03, hdx Verified version 0.1.
01a, 19feb03, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains functions to operator files(INI, update...).
*/

#ifndef FILETOOL_H
#define FILETOOL_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

#include "edpbase.h"
#include <semLib.h>
// #include <semaphore.h>
// #include <sys/ipc.h>
// #include <sys/sem.h>

/* defines */

#define TSFS_DBG    0		/* 分区功能 */

#if TSFS_DBG
#define EP_ROOT             "/root/tgtsvr"
#else
#define EP_ROOT             "/root/tffs"
#define EP_SET              "/root/set"
#define EP_DATA             "/root/data"
#endif

#define EP_SYS_CFG_DIR      EP_ROOT "/sys"
#define EP_SET_INI_DIR      EP_SET "/ini"
#define EP_SET_AREA_DIR     EP_SET "/set"
#define EP_SYS_LOG_DIR      EP_SET "/log"
#define EP_BAK_PC_DIR       EP_SET "/bak"
#define EP_TMP_FILE_DIR     EP_DATA "/tmp"
#define EP_EVT_RPT_DIR      EP_DATA "/evt"
#define EP_WAVE_REC_DIR     EP_DATA "/rec"
#define EP_REV_TM_DIR       EP_DATA "/revtime"
#define EP_MMIEVT_RPT_DIR      EP_DATA "/mmievt"

#define EP_VX_RUN_FILE      EP_ROOT "/vxworks"

#ifdef EDP03_BUILD /* EDP03 platform */

#if defined(EDP03_STABCONTROL_BUILD) || defined(EDP03_LOWPROTECT_BUILD)
#define EP_EDP_SYS_FILE      EP_ROOT "/edpsys.out"
#define EP_EDP_APP_FILE      EP_ROOT "/edpapp.out"
#define EP_EDP_USB_FILE      EP_ROOT "/usb.out"
#define EP_EDP_MMI_FILE      EP_ROOT "/mmi.out"
#endif	/* end of EDP03_STABCONTROL_BUILD || EDP03_LOWPROTECT_BUILD */

#if defined(EDP03_INTELBOX_BUILD)
#define EP_EDP_SYS_FILE      EP_ROOT "/edpsys.out"
#define EP_EDP_APP_FILE      EP_ROOT "/edpapp.out"
#endif		/* end of EDP03_INTELBOX_BUILD */

#endif	/* end of EDP03_BUILD */

#if defined(EDP_01_02_BUILD)
#define EP_EDP_SYS_FILE      EP_ROOT "/edpsys.out"
#define EP_EDP_APP_FILE      EP_ROOT "/edpapp.out"
#endif		/* end of EDP03_INTELBOX_BUILD */

#if defined(EXCITE_BUILD)
#define EP_EDP_SYS_FILE      EP_ROOT "/edpsys.out"
#define EP_EDP_APP_FILE      EP_ROOT "/edpapp.out"
#define EP_EDP_MMI_FILE      EP_ROOT "/mmi.out"
#endif		/* end of EDP03_INTELBOX_BUILD */

#define EP_UN_VERSION_FILE      EP_ROOT "/Version.ini"
#define EP_EDP_AUTOEXEC_FILE      EP_ROOT "/autoexec.ini"
#define EP_1588_PROFILE_FILE EP_ROOT "/1588Profile.ini"
#define EP_EDP_DSamSts_FILE	EP_ROOT "/DSam_Sts.ini"   /* 合并器延时保存文件 */

#define EP_HW_CFG_FILE      EP_SYS_CFG_DIR "/hwcfg.ehc"
#define EP_SW_CFG_FILE      EP_SYS_CFG_DIR "/swcfg.esc"
#define EP_LGC_CFG_FILE     EP_SYS_CFG_DIR "/logic.egs"

#define EP_BIN_CFG_FILE     EP_SET_INI_DIR "/config.bin"
#define EP_SYS_INI_FILE     EP_SET_INI_DIR "/edp01.ini"
#define EP_SYS_INFO_FILE    EP_SET_INI_DIR "/syscfg.sci"

#define EP_SYS_LOG_FILE     EP_SYS_LOG_DIR "/edpcore.log"
#define EP_BAK_LOG_FILE     EP_SYS_LOG_DIR "/edpcor0.log"

#define EP_INNER_SET_FILE   EP_SET_AREA_DIR "/edpset.idz"
#define EP_CK_SET_FILE    EP_SET_AREA_DIR "/edpckset.cdz"
#define EP_LINK_STS_FILE    EP_SET_AREA_DIR "/edplink.set"
#define EP_DI_STS_FILE      EP_SET_AREA_DIR "/edpdi.set"
#define EP_FUNC_STS_FILE    EP_SET_AREA_DIR "/edpfunc.set"
#define EP_AI_GAIN_FILE     EP_SET_AREA_DIR "/hdcof.gaf"
#define EP_CT_RATIO_FILE    EP_SET_AREA_DIR "/ctratio.ctr"
#define EP_CL_GAIN_FILE    EP_SET_AREA_DIR "/clcof.gaf"
#define EP_SET_RANGE_FILE    EP_SET_AREA_DIR "/range.set"   /* 定值量程文件 */

#define EP_COMTRADE_CFG_FILE    EP_TMP_FILE_DIR "/cfg.cfg"		/* ghx 为仿真读取 comtrade录波文件添加 */
#define EP_COMTRADE_DATA_FILE  EP_TMP_FILE_DIR "/data.dat"
#define EP_ADC_DATA_FILE  EP_TMP_FILE_DIR "/ad.txt"
#define EP_MSU_COE_FILE  EP_SET_AREA_DIR "/msucoe.ini"		/* 测量系数存储文件 DY 11/18/2006 */
#define EP_PO_FILE  EP_SET_AREA_DIR "/podata.ini"		/* 脉冲电量存储文件 DY 11/18/2006 */
#define PLATFORM_TEST_FILE EP_SET_AREA_DIR "/flatformtest.ini"		/* 脉冲电量存储文件 DY 11/18/2006 */

#define EP_61850_CFG_DIR EP_ROOT "/lgh"  /* 61850有关配置文件目录 */

#define EP_PANEL_BIN_FILE EP_ROOT "/Panel.bin"
#define EP_PANEL_FUSE_FILE EP_ROOT "/PanelFuse.txt"
#define EP_WATCHALARM_BIN_FILE EP_ROOT "/WatchAlarm.bin"
#define EP_WATCHALARM_FUSE_FILE EP_ROOT "/WatchAlarmFuse.txt"
#define EP_STRINGNAME_FILE EP_SET_INI_DIR "/shname.shn"
#define EP_STRINGNAME_BAK_FILE EP_SET_INI_DIR "/shnamebak.shn"
#define EP_MMISET_FILE EP_SET_INI_DIR "/mmiset.cfg"
#define EP_LINK_MODE_FILE EP_SET_AREA_DIR "/edplinkmode.set"  		/* 定制压板模式配置文件 */
#define E02_SG_CONFIG_FILE "/tffs/e02_sgcfg.xml"
#define EP_AUX_SYS_CFG_FILE EP_DATA "/auxedp.ini"    /* 附加系统配置信息 */
#define EP_SYS_PARA_FILE EP_DATA "/syspara.ini"    /* 系统参数信息 */

#define EP_FUNCOPTION_FILE   EP_SYS_CFG_DIR "/FuncOpt.ini" /*国网六统一选配文件*/
#define EP_LOCAL_STS_FILE EP_BAK_PC_DIR "/localornot.set"  /* 远方就地状态文件 */

#define SMV_GO_COMM_STAT_FILE "/tffs/SG_CommStat.txt" /* 过程层状态文件 */

/*
#define TSFS_DBG    0

#if TSFS_DBG
#define EP_ROOT             "/tgtsvr"
#else
#define EP_ROOT             "/tffs"
#endif

#define EP_TMP_FILE_DIR     EP_ROOT "/tmp"
#define EP_SET_AREA_DIR     EP_ROOT "/set"
#define EP_EVT_RPT_DIR      EP_ROOT "/evt"
#define EP_SYS_CFG_DIR      EP_ROOT "/sys"
#define EP_WAVE_REC_DIR     EP_ROOT "/rec"
#define EP_SYS_LOG_DIR      EP_ROOT "/log"
#define EP_BAK_PC_DIR       EP_ROOT "/bak"
#define EP_REV_TM_DIR       EP_ROOT "/revtime"

#define EP_VX_RUN_FILE      EP_ROOT "/vxworks"

#define EP_BIN_CFG_FILE     EP_SYS_CFG_DIR "/config.bin"
#define EP_SYS_INI_FILE     EP_SYS_CFG_DIR "/edp01.ini"

#define EP_HW_CFG_FILE      EP_SYS_CFG_DIR "/hwcfg.ehc"
#define EP_SW_CFG_FILE      EP_SYS_CFG_DIR "/swcfg.esc"
#define EP_LGC_CFG_FILE     EP_SYS_CFG_DIR "/logic.egs"

#define EP_SYS_INFO_FILE    EP_SYS_CFG_DIR "/syscfg.sci"

#define EP_SYS_LOG_FILE     EP_SYS_LOG_DIR "/edpcore.log"
#define EP_BAK_LOG_FILE     EP_SYS_LOG_DIR "/edpcor0.log"

#define EP_INNER_SET_FILE   EP_SET_AREA_DIR "/edpset.idz"
#define EP_LINK_STS_FILE    EP_SET_AREA_DIR "/edplink.set"
#define EP_DI_STS_FILE      EP_SET_AREA_DIR "/edpdi.set"
#define EP_FUNC_STS_FILE    EP_SET_AREA_DIR "/edpfunc.set"
#define EP_AI_GAIN_FILE     EP_SET_AREA_DIR "/hdcof.gaf"
#define EP_CT_RATIO_FILE    EP_SET_AREA_DIR "/ctratio.ctr"
*/

#define INI_TAG_LEN         64
#define FT_VER_INFO_LEN     36

#define DIR_NAME_LEN        128
#define FILE_NAME_LEN       64
#define FULL_NAME_LEN       (DIR_NAME_LEN+FILE_NAME_LEN+1)

/* global functions */

/* Initilize the config file module.
 * (It has the function to clean system temp files.)
 * Parameters:
 *      None
 * Return:
 *      EP_SUCCESS,
 *      EP_ERROR. */
EP_STATUS FT_Init_Cfg(void);

/*提供保护启动之后的文件系统初始化代码*/
EP_STATUS FT_After_Relay_Init_Cfg(void);

/* Prepair to update a file.  (Using temp file to keep data integrity.)
 * Parameters:
 *      strFile, name of file to be updated.
 * Return:
 *      >=0, file descriptor. For using read/write function.
 *      EP_PARA_ERR, the file does not exist.
 *      EP_FILE_ERR, create temp file failure. */
EP_STATUS FT_Bgn_Update(const uint8_t *strFile);

/* Finish updating file.  Temp file is cleaned normally.
 * Parameters:
 *      strFile, name of file to be updated.
 *      iFd, file descriptor returned by FT_Bgn_Update.
 * Return:
 *      EP_SUCCESS, update OK.
 *      EP_ERROR, operating failure. */
EP_STATUS FT_End_Update(const uint8_t *strFile, int iFd);

/* Abort updating/creating file.  Temp file is cleaned.
 * Parameters:
 *      strFile, name of file to be updated.
 *      iFd, file descriptor returned by FT_Bgn_Update.
 * Return:
 *      EP_SUCCESS, abort OK.
 *      EP_ERROR, operating failure. */
EP_STATUS FT_Abort_Update(const uint8_t *strFile, int iFd);

/* Write system config item. (Create it when not existing)
 * Parameters:
 *      strField, field name.
 *      strItems, string of items name.
 *      strVals, string of items value to set.
 * Return value:
 *      EP_SUCCESS, item/field was added success.
 *      EP_LOCAL_MSG, modify the existing item.
 *      EP_ERROR, operating failure.
 * Alert:
 *      1. Maximal length of each name/value is MAX_ID_LEN.
 *      2. This function support updating multi-items in same field one time.
 *         '\n' is used in strItems and strVals to seprate items.
 *      3. This function guarantees integrity of multi-items updating. */
EP_STATUS FT_Wr_Sys_INI(const uint8_t *strField,
                        const uint8_t *strItems, const uint8_t *strVals);

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
                  const uint8_t *strItems, uint8_t *strVals, int iBufLen);

/* Check if file exists.
 * Parameters:
 *      strName, file name(full name with directory message).
 * Return value:
 *      TRUE, strName is a real file.
 *      FALSE, strName is not a file. */
BOOL FT_Is_File(const uint8_t *strName);

/* Check if dir exists.
 * Parameters:
 *      strName, directory name(full name).
 * Return value:
 *      TRUE, strName is a real file.
 *      FALSE, strName is not a file. */
BOOL FT_Is_Dir(const uint8_t *strName);

/* Get File Length.
 * Parameters:
 *      strName, directory name(full name).
 * Return value:
 *      int, File Length */
int FT_Get_Len(const uint8_t *strName);

/* Delete a directory tree.
 * Parameters:
 *      strDir, name of directory to be deleted.
 * Return value:
 *      OK, success.
 *      ERROR, strDir is not a valid directory.
 * Alert:
 *      This function uses recursion to implement. If the directory tree is
 *      very deep, the function may need quite more stack space. */
STATUS FT_Del_Tree(const uint8_t *strDir);

/* Get file verion infomation string.
 * Parameters:
 *      strFile, file name(full name).
 *      pucRslt, save the return string.
 * Return value:
 *      EP_SUCCESS, OK.
 *      EP_ERROR, strFile is not a file.
 * Alert:
 *      The pucRslt must at least contains FT_VER_INFO_LEN+1 bytes space. */
EP_STATUS FT_Get_File_Ver(const uint8_t *strFile, uint8_t *pucRslt);

/* 由于改成vxworksrom方式,
 * 文件系统不再存在vxworks文件,此处直接返回全零的版本.
 * Parameters:
 *      pucRslt, save the return string.
 * Return value:
 *      EP_SUCCESS, OK.
 */
EP_STATUS FT_Get_Vxworks_File_Ver(uint8_t *pucRslt);

/* Caculate CRC32 of a file.
 * Parameters:
 *      strFile, file name(full name).
 * Return value:
 *      CRC32 result of the file. */
uint32_t FT_File_CRC32(const uint8_t *strFile);

/* Caculate CCITT-CRC16 of a file.
 * Parameters:
 *      strFile, file name(full name).
 * Return value:
 *      CCITT-CRC16 result of the file. */
uint16_t FT_File_CRC16(const uint8_t *strFile);

/* Get file length from file descriptor.
 * Parameters:
 *      iFd, file descriptor.
 * Return valud:
 *      Length of the file. */
uint32_t FT_File_Len(int iFd);

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
uint8_t *FT_File_To_Mem(const uint8_t *strFile, uint32_t *pulLen);

/* Print text file to shell.
 * Parameters:
 *      strFile, name of file to be displayed.
 * Return value:
 *      >0, file length.
 *      ERROR, can't open file or file too long. */
STATUS FT_Print_File(const uint8_t *strFile);


/* Get file modify time infomation string.
 * Parameters:
 *      strFile, file name(full name).
 *      pucRslt, save the return string.
 * Return value:
 *      EP_SUCCESS, OK.
 *      EP_ERROR, strFile is not a file. */
EP_STATUS FT_Get_File_modify_time(const uint8_t *strFile, uint8_t *pucRslt);

int GetData_B(uint8_t *pcStr,uint8_t lIndex,uint8_t *m_pcItemStr,uint8_t char_B,uint8_t char_E);

/*ZQ 2011-02-15
根据aucDftIni,创建最新格式DSamSts.ini文件*/
void FT_New_DSamSts_INI_File(void);
void FT_Wr_DSamSts_INI(const uint8_t *strHeader,const uint8_t *strItems, const uint8_t *strVals);

/*DQ 2007-12-24
根据aucDftIni,创建最新格式sys.ini文件，同时将
旧EP_SYS_INI_FILE文件中的内容继承到新的EP_SYS_INI_FILE文件中
*/
void FT_New_SYS_INI_File(void);

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
EP_STATUS FT_Wr_Version_INI(const uint8_t *strHeader,const uint8_t *strItems, const uint8_t *strVals);

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
                      const uint8_t *strItems, uint8_t *strVals, int iBufLen);

/* Write config item. (Create it when not existing)
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
                           const uint8_t *strItems, const uint8_t *strVals);

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
                     const uint8_t *strItems, uint8_t *strVals, int iBufLen);


int Update_FT_Rd_INI(const char *strFileName,const uint8_t *strHeader,
                     const uint8_t *strItems, uint8_t *strVals, int iBufLen);

EP_STATUS Update_FT_Wr_INI(const char *strFileName,const uint8_t *strHeader,
                           const uint8_t *strItems, const uint8_t *strVals);


/*功能:复制一个文件的备份文件,后缀为.cpy,供系统INI文件使用
  参数:pucRslt,返回新文件名
       strFile,源文件名  ZY  2011-6-29
  返回:无  */
void FT_Cpy_Name(uint8_t *pucRslt, const uint8_t *strFile);

/*功能:  复制edp01.ini文件到edp01.ini.cpy,防止该文件被毁坏时,能恢复
  参数:  无
  返回:  EP_SUCCESS 操作成功
         其他  操作失败  ZY  2011-6-29*/
EP_STATUS  FT_Cpy_Sys_Ini_File();


/*功能:  初始化和读取系统INI文件
  参数:  无
  返回:  无
         ZY  2011-6-29
  注意:  因为有日志记录,需要放到日志功能初始化之后*/
void  FT_Init_Sys_Ini_File();

/* 复制auxedp.ini文件到auxedp.ini.cpy, 防止该文件被毁坏时, 能恢复.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS FT_Cpy_Aux_Sys_Ini_File(void);

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
int FT_Rd_Aux_Sys_INI(const uint8_t *strHeader, const uint8_t *strItems, uint8_t *strVals, int iBufLen);

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
EP_STATUS FT_Wr_Aux_Sys_INI(const uint8_t *strHeader, const uint8_t *strItems, const uint8_t *strVals);

/* 初始化和读取附加系统信息INI文件.
 * Para:
 *     NONE.
 * Return:
 *     OK, ERROR.
 */
void FT_Init_Auc_Sys_Ini_File(void);

/* 功能: 复制文件
 * 参数: oldname, 源文件路径;
 *       newname, 目标文件路径;
 * 返回: OK, 成功; ERROR, 出错;
 */
STATUS FT_Cpy_File(const char *oldname, const char *newname);

#ifdef	__cplusplus
}
#endif

#endif                                  /* FILETOOL_H */
