/* EdpVer.h - subroutine library for managing the version. */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 22aug07, zy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for managing the version.
*/

#ifndef EDP_VER_H
#define EDP_VER_H

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */

//#include "vxWorks.h"
#include "edpbase.h"
#include "filetool.h"

/* defines */

#define MAX_HW_ID_LEN 256

/* typdefs */

typedef struct  /* 系统相关软件版本信息结构 */
{
    uint16_t unCfgProtocolVer;				/* 配置规约版本号 */
    uint16_t unCfgPrgVer;										/* 配置程序版本号 */
    uint16_t unHdCfgVer;				/* 硬件配置版本号 */
    uint16_t unSwCfgVer;												/* 软件配置版本号 */
    uint16_t unLogicPrgVer;						/* 逻辑图程序版本号 */
    uint16_t unLogicVer;															/* 逻辑图版本号 */
    uint8_t  aucSysVer[MAX_ID_LEN];					/* 保护配置（配置和逻辑图）版本号字符串 */
    uint16_t unRlsCRC; 							    /* 逻辑图配置的CRC发布校验码 */
    uint16_t unActualRlsCRC;                           /* 发布CRC校验码 */
    uint16_t unEdpSwCRC;							/* 平台软件校验码 */
    uint16_t unSwCfgCRC;	/* 软件配置文件CRC */
} SI_SYS_VER;

typedef enum			/* CPU硬件版本序列号列表 ，以后可以继续添加 */
{
    EDP_UNKOWN_BOARD=0,  		/* 未支持的硬件 */
    EDP01_CPU_C_A_200609_BORAD=1,
    EDP02_CPU_A_A_200703_BORAD=2,
    EDP02_CPU_A_C_200706_BORAD=3,
    EDP02_CPU_C_A_200703_BORAD=4,
    EDP02_CPU_D_A_200703_BORAD=5,		/* 增加电铁 */
    EDP02_CPU_A_B_200709_BORAD=6,		/* 增加一个开入开出 */
    EDP03_CPU_A_A_200612_BORAD=7,
    E03_CPU_B_A_200806_BORAD=8,
    E03_CPU_C_A_200805_BORAD=9,
    PSVR100_CPU_A_A_200606_BORAD=10,
    E01_CPU_D_A_200712_BORAD = 11,
    E02_CPU_A_C_200803_BORAD = 12,
    PSR660U_STI_A_A_BORAD = 13,
    E02_CPU_F_BORAD = 14,
    DEFAULT_BORAD = 15
} HW_VER_SN_TYPE;

typedef struct  /* CPU板硬件版本匹配结构 */
{
    uint8_t aucHwVerID[MAX_HW_ID_LEN+1];			/* 硬件版本标识 */
    int iHwVerSN;			    /* 对应的硬件版本序列号 见HW_VER_SN_TYPE类型中定义 */
} EP_HW_VER_MAP;

/* globals */

extern SI_SYS_VER SI_SysVer_g;			/* 系统相关软件版本信息 */
extern uint8_t aucsysBspVer[FT_VER_INFO_LEN+1];

/***********************************************************************
* VER_InitVerFunc - 初始化版本管理模块
*
* RETURNS:
*               EP_SUCCESS, 成功
*               EP_ERROR, 失败
*
*/
EP_STATUS VER_InitVerFunc();

/* 版本号写入
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void VER_WrVer(void);

/*********************************** PCB硬件版本相关信息******************************/

/***********************************************************************
* sysModel - 获得PCB板型号，BSP提供
*
* RETURNS: NULL或指向板号数组的指针
*
*/
extern char *sysModel(void);

/***********************************************************************
* VER_GetHwBoardSN - 获得硬件版本序列号
*
* RETURNS: 硬件版本序列号，见HW_VER_SN_TYPE类型中定义
*
* Alert: 若返回UNKOWN_BOARD，则表示BSP未支持该版本硬件
*
*/
int VER_GetHwBoardSN();

/*功能：获得平台软件CRC  　
  参数：无
  返回：16位CCITT的CRC
        */
uint16_t  VER_GetEdpSwCRC();

/*功能：获得保护配置软件的总CRC，包括软硬件配置和逻辑图 　
  参数：无
  返回：16位CCITT的CRC
        */
uint16_t  VER_GetCfgCRC();

/*功能：获得逻辑图配置版本号，暂时无实际意义
  参数：无
  返回：版本号
       4 BCD numbers.16位,高8位为整数位,低8位为小数位 */
uint16_t  GetLogicVer();

/*
功能：获得平台支撑组件的CRC信息
参数： puiRtPlatCRC ，返回平台组件的CRC，若该法获得，则该值无意义
返回：若获得失败，则返回FALSE
                否则返回TRUE
注意：由BSP版本号作为CRC原始输入，和平台支撑软件的CRC（详见VER_GetEdpSwCRC()具体实现）计算组合而成，
若对EDP03而言，则还需要计算mmi.out的CRC（但对mega16的程序，最好不包括，因为该程序升级之后，会自动消失掉）
*/
uint16_t VER_UN_GetPlatCRC();

/*
功能：获得功能选配组件的CRC信息
参数： 无
返回：功能选配组件的CRC
*/
uint16_t VER_UN_GetFuncOptCRC();

/*
功能：获得平台支撑组件的版本信息
参数： pucRtPlatVer ,    返回平台组件的版本号字符串，若无法获得版本号，则该串应该返回空串。
该字符串空间由调用方分配，由被调用方填充iVerStrLen,该字符串空间分配的长度。
返回：若获得失败，则返回FALSE
                否则返回TRUE
 */
BOOL VER_UN_GetPlatVer(uint8_t *pucRtPlatVer,int iVerStrLen);

/*
功能：获得保护应用组件的CRC信息
参数： puiRtRelayCRC ，返回保护应用组件的CRC，若该法获得，则该值无意义,
返回：若获得失败，则返回FALSE
                否则返回TRUE
注意：计算返回实际的CRC，由平台支撑组件的特征码，作为原始输入，然后再计算edpapp.out,
      若包括保护配置文件的话，则还需要再依次计算hwcfg.ehc,swcfg.esc,logic.egs
*/
uint16_t VER_UN_GetRelayCRC();

/*
功能：获得保护应用组件整体版本信息
参数： pucRtRelayVer ,    返回保护应用组件的版本号字符串，若无法获得版本号，则该串应该返回空串。
该字符串空间由调用方分配，由被调用方填充iVerStrLen,该字符串空间分配的长度。
返回：若获得失败，则返回FALSE
                否则返回TRUE
注意：该版本信息，由保护人员在逻辑图中中设置，若逻辑图未设置,则使用extend.c函数GetUsrSwVer()。
 */
BOOL VER_UN_GetRelayPrioVer(uint8_t *pucRtRelayVer, int iVerStrLen);

/*
功能：获得MMI的CRC信息
*/
uint16_t VER_UN_GetMmiCRC();

/*
功能：获得保护应用组件CRC是否包含保护配置文件
参数：无
返回：若计算保护应用组件CRC包含保护配置文件，则返回TRUE,否则返回FALSE
注意：该设置由保护人员在程序中设置，在extend.c中，可由保护人员定义一个宏，定义不包含保护配置文件（缺省是包含的）
该函数程序实现，放在extend.c中。
 */
BOOL VER_UN_RelayCRCIsCoverCfg();

/*功能：获得应用软件CRC  　
        在实现动态加载功能后的版本，用户界面显示的CRC，
        为了便于现场升级，当逻辑图中配置了发布CRC时， 此时返回逻辑图中配置的发布CRC 　
  参数：无
  返回：16位CCITT的CRC
        */
uint16_t  VER_GetUsrSwCRC();

/*功能：获得总CRC，包括BOOTROM，BSP，平台软件，软硬件配置和逻辑图，保护应用软件  　
  参数：无
  返回：16位CCITT的CRC
        */
uint16_t  VER_GetTotalCRC();

/*
功能：获得平台支撑组件的特征码
参数： puiRtPlatLabel , 返回平台组件的特征码
返回：若获得失败，则返回FALSE
                否则返回TRUE
 */
uint16_t VER_UN_GetPlatLabel();

/*
功能：获得保护程序的版本信息
参数： pucRtRelayVer ,    返回保护应用组件的版本号字符串，若无法获得版本号，则该串应该返回空串。
该字符串空间由调用方分配，由被调用方填充iVerStrLen,该字符串空间分配的长度。
返回：若获得失败，则返回FALSE
                否则返回TRUE
注意：该版本信息，由保护人员在程序中设置，在extend.c中。
 */
BOOL VER_UN_GetRelayVer(uint8_t *pucRtRelayVer, int iVerStrLen);

/***********************************************************************
* GetSysPlatLabel - 获得平台软件版本号
*
* RETURNS: 平台软件特征码
*
*/
uint16_t GetSysPlatLabel();

/*
功能：获得mmi的版本信息
参数： pucMmiVer ,    返回mmi组件的版本号字符串，若无法获得版本号，则该串应该返回空串。
该字符串空间由调用方分配，由被调用方填充iVerStrLen,该字符串空间分配的长度。
返回：若获得失败，则返回FALSE
                否则返回TRUE
注意：该版本信息，由mmi在程序中设置。
 */
BOOL VER_UN_GetMmiVer(uint8_t *pucRtMmiVer, int iVerStrLen);

/* 获取软件配置校验CRC.
 * Para:
 *     NONE.
 * Return:
 *     CRC.
 */
uint16_t VER_GetSwCfgCRC();

/* 外部调用获取软件配置校验CRC.
 * Para:
 *     NONE.
 * Return:
 *     CRC.
 */
uint16_t VER_ExtGetSwCfgCRC();

typedef  void  (*  EP_GET_USR_VER_FUNC_TYPE)(uint8_t *, int);

#ifdef	__cplusplus
}
#endif

#endif                                  /* EDP_VER_H */
