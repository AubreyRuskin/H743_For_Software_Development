
/* sysinfo.h - This file contains interface to manager system infomation files */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02a, 30may07, dy change the code style.
01c, 29jul03, hdx Updated to version 1.0.
01b, 27may03 hdx Verified version 0.1.
01a, 15feb03, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains interface to manager system infomation files.
*/

#ifndef SYSINFO_H
#define SYSINFO_H

/* includes */

#include "edpbase.h"
#include "EdpVer.h"
#include "realdatadef.h"    /* 模件类型 */

#ifdef  __cplusplus
extern "C" {
#endif

/* typedefs */

#if 0
typedef struct
{
    uint16_t unCfgProtocolVer;			/* 配置规约版本号 */
    uint16_t unCfgPrgVer;				/* 配置程序版本号 */
    uint16_t unHdCfgVer;								/* 硬件配置版本号 */
    uint16_t unSwCfgVer;		/* 软件配置版本号 */
    uint16_t unLogicPrgVer;					/* 逻辑图程序版本号 */
    uint16_t unLogicVer;								/* 逻辑图版本号 */
    uint8_t  aucSysVer[MAX_ID_LEN];						/* 系统版本号字符串 */
    uint16_t unRlsCRC; 			/* 发布CRC校验码 */
} SI_SYS_VER;
#endif

typedef struct
{
    int iLine;
    uint8_t aucVal[16];
} SI_File_Item;

typedef struct
{
    uint16_t unPlatCrc;
    uint8_t aucPlatVer[50];
    uint16_t unPlatLabel;
    uint16_t unRelayCrc;
    uint8_t aucRelayVer[50];
    BOOL bRelayCRCIsCoverCfgFlag;
    uint16_t unCfgCrc;
    uint8_t aucCfgVer[50];
    uint16_t unFuncOptCrc;
    uint16_t unMmiCrc;
    uint8_t aucMmiVer[50];
    uint8_t acExtHwVer[32];    /*扩展机箱硬件版本号*/
    uint8_t acExtBspVer[32];
    uint8_t acExtEdpVer[32];
    uint16_t unExtEdpCrc;
    //uint8_t aucExtIOVer[MAX_MOD_NUM][8];
} UNITE_VER_INFO;

/* globals */

void SI_Creat_SCI(const uint8_t *strSysCfgFile);

extern const uint8_t * const pucDiFileHead_g;
extern const uint8_t * const pucLinkFileHead_g;
extern const uint8_t * const pucFuncFileHead_g;
extern const uint8_t * const pucLinkModeFileHead_g;		/* 压板模式文件 */
extern  SI_SYS_VER  SI_SysVer_g;

/* functions */


void SI_Creat_SCI(const uint8_t *strSysCfgFile);

/***********************************************************************
* SI_Chk_Cfg - 检测3个配置文件  ，要在SI_Init_Sys_Info之前调用
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS   SI_Chk_Cfg();

/***********************************************************************
* SI_Init_Sys_Info - 生成系统信息文件
*
* RETURNS:
*				   EP_SUCCESS, 正常返回
*               EP_BUF_ERR, 内存错误
*
*/
EP_STATUS SI_Init_Sys_Info(
    const uint8_t *strSysCfgFile		/* 系统配置文件名称 */
);

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
);

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
);

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
);

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
);

/***********************************************************************
* GetBspVer - 获得BSP版本号
*
* RETURNS: BSP版本号
*
*/
extern uint16_t GetBspVer();

/***********************************************************************
* GetUsrSwVer - 获得用户程序版本号
*
* RETURNS: TRUE OR FALSE
*
*/
void GetUsrSwVer(uint8_t *ucUsrSysVer, int nStrlen);

/***********************************************************************
* GetBootromVer - 获得BOOTROM版本号
*
* RETURNS: BOOTROM版本号
*
*/
uint16_t GetBootromVer();

/***********************************************************************
* GetInnerProtocolVer - 获得支持的内部规约版本号
*
* RETURNS: 内部规约版本号
*
*/
uint16_t GetInnerProtocolVer();

/***********************************************************************
* GetSysSwVer - 获得平台软件版本号
*
* RETURNS: 平台软件版本号
*
*/
uint8_t* GetSysSwVer();

/***********************************************************************
* GetMmiSwVer - 获得mmi版本号，如果没有设置，与平台程序一致
*
* RETURNS: 无
*
*/
void  GetMmiSwVer(
    uint8_t *ucMmiSysVer,
    int nStrlen
);

/***********************************************************************
* GetVxworksCRC - 获得VXWORKS CRC
*
* RETURNS: VxWorks校验码
*
*/
uint16_t GetVxworksCRC();

/***********************************************************************
* GetSysCRC - 获得系统CRC
*
* RETURNS: 系统校验码
*
*/
uint16_t GetSysCRC();

/***********************************************************************
* SI_New_AI_Gain_Set - 生成新的物理通道系数文件
*
* RETURNS: 无
*
*/
void SI_New_AI_Gain_Set(void);

/***********************************************************************
* SI_New_CL_Gain_Set - 生成新的测量量系数文件
*
* RETURNS: 无
*
*/
void SI_New_CL_Gain_Set(void);

/* Get configuration file verion infomation string.
 * Parameters:
 *      strFile, file name(full name).
 *      pucRslt, save the return string.
 * Return value:
 *      EP_SUCCESS, OK.
 *      EP_ERROR, strFile is not a file. */
EP_STATUS FT_Get_Cfg_File_Ver(const uint8_t *strFile, uint8_t *pucRslt);

/***********************************************************************
* SI_Need_Reset_SCI - 判断是否需要重新生成sci文件
*
* RETURNS:
*				TRUE,  需要重新生成sci,并且就sci已经被删除
*               FALSE, 不需重新生成
*
*/
BOOL SI_Need_Reset_SCI();

/***********************************************************************
* SI_Chk_Un_Ver - 校验version.ini中统一发布相关版本与实际程序是否一致
* RETURNS:
*       	   EP_SUCCESS, 正常
*              EP_ERROR, 错误
*/
EP_STATUS SI_Chk_Ver_INI();

/***********************************************************************
* SI_New_CK_Set - 生成新的测控定值文件
*
* RETURNS: 无SI_Wr_New_CK_Set
*
*/
void SI_Wr_New_CK_Set(void);

/***********************************************************************
* SI_Rst_Ver_INI - 将装置实际运行组件信息更新回version.ini中
* 注意，必须在SI_Chk_Ver_INI()之后调用
* RETURNS:
*       	   EP_SUCCESS, 正常
*              EP_ERROR, 错误
*/
EP_STATUS SI_Rst_Ver_INI();

/***********************************************************************
* GetRelayComAttr - 获取保护组件属性
* RETURNS: NONE
*/
void GetRelayComAttr(void);



/*功能，记录SCI的相关版本信息到系统日志文件中 ,
  参数，无
  返回，无
  注意: 只有当SCI文件不更新时,才调用 2009-11-17 ZY */
void  SI_WrVerToExLog();

/* 检查开入强制文件是否有CRC
 * Para:
 *     pbCrc, 检查结果指针.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
extern EP_STATUS SC_Judge_Force_Di(BOOL *pbCrc);

#ifdef  __cplusplus
}
#endif

#endif                                  /* SYSINFO_H */
