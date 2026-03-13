/* EdpVer.c - subroutine library for managing the version. */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 22aug07, zy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for managing the version.
INCLUDES: EdpVer.h
*/

/* includes */

#include "vxWorks.h"
// #include "config04.h"
#include "EdpVer.h"
#include "sysinfo.h"
#include "filetool.h"
#include "errtest.h"
#include "logmsg.h"
#include "string_compat.h"
#include "miscfunc.h"
// #include "bspinterface.h"
#include <stdio_compat.h>

/* typedefs */

EP_HW_VER_MAP aHwVerMapArr[]=				/* CPU硬件版本标识匹配表 ，以后可以继续添加 */
{
    {"EDP01-CPU.C-A 2006.09", EDP01_CPU_C_A_200609_BORAD},
    {"EDP02-CPU.A-A 2007.03", EDP02_CPU_A_A_200703_BORAD},
    {"EDP02-CPU.A-C 2007.06", EDP02_CPU_A_C_200706_BORAD},
    {"EDP02-CPU.C-A 2007.03", EDP02_CPU_C_A_200703_BORAD},
    {"EDP02-CPU.D-A 2007.07", EDP02_CPU_D_A_200703_BORAD},
    {"EDP02-CPU.A-B 2007.09", EDP02_CPU_A_B_200709_BORAD},
    {"EDP03-CPU.A-A 2006.12", EDP03_CPU_A_A_200612_BORAD},
    {"E03-CPU.B-A 2008.06", E03_CPU_B_A_200806_BORAD},
    {"E03-CPU.C-A 2008.05", E03_CPU_C_A_200805_BORAD},
    {"PSVR100-CPU.A-A 2006.06", PSVR100_CPU_A_A_200606_BORAD},
    {"E01-CPU.D-A 2007.12", E01_CPU_D_A_200712_BORAD},
    {"E02-CPU.A-C 2008.03", E02_CPU_A_C_200803_BORAD},
    {"PSR660U-STI.A-A 2008.12", PSR660U_STI_A_A_BORAD},
    {"E02-CPU.F", E02_CPU_F_BORAD},
    {"", DEFAULT_BORAD}
};

/* CPU硬件版本分类
 */
EP_HW_VER_MAP aBasicHwVerMapArr[]=
{
    {"EDP01", BOARD_TYPE_E01},
    {"E01", BOARD_TYPE_E01},
    {"EDP02", BOARD_TYPE_E02},
    {"E02", BOARD_TYPE_E02},
    {"EDP03", BOARD_TYPE_E03},
    {"E03", BOARD_TYPE_E03},
    {"PSVR", BOARD_TYPE_EXCITE}
};

/* globals */

int iHwBoardVerSN_g=EDP_UNKOWN_BOARD;		/* 硬件版本序列 */
EP_GET_USR_VER_FUNC_TYPE  pGetUsrVerEntryFunc_g;
EP_GET_USR_VER_FUNC_TYPE  pGetMmiVerEntryFunc_g=NULL;


extern UNITE_VER_INFO UnVerInfo_g;


uint8_t aucsysBspVer[FT_VER_INFO_LEN+1];

/* functions */

/* 初始化版本管理模块
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS VER_InitVerFunc()
{
    char *strHwID;
    int iHwMapCnt = 0;
    int i;
    BOOL bMapSuccess = FALSE;
    uint8_t *pPos = NULL;  /* 版本字符串位置 */

    SI_SysVer_g.unCfgProtocolVer = 0;
    SI_SysVer_g.unCfgPrgVer = 0;
    SI_SysVer_g.unHdCfgVer = 0;
    SI_SysVer_g.unSwCfgVer = 0;
    SI_SysVer_g.unLogicPrgVer = 0;
    SI_SysVer_g.unLogicVer = 0;
    SI_SysVer_g.aucSysVer[0] = '\0';
    SI_SysVer_g.unRlsCRC = 0;

    // trim_space(aucsysBspVer, sysBspRev());
    aucsysBspVer[0] = '\0';

    strHwID = sysModel();
    iHwMapCnt = sizeof(aHwVerMapArr)/sizeof(aHwVerMapArr[0]);

    for (i=0; i<iHwMapCnt-1; i++)
    {
        pPos = strstr(strHwID, aHwVerMapArr[i].aucHwVerID);

        if (pPos != NULL)
        {
            /* 若找到 */
            bMapSuccess = TRUE;
            iHwBoardVerSN_g = aHwVerMapArr[i].iHwVerSN;

            break;
        }
    }

    if (!bMapSuccess)
    {
        strncpy(aHwVerMapArr[iHwMapCnt-1].aucHwVerID, strHwID, MAX_HW_ID_LEN);
        aHwVerMapArr[iHwMapCnt-1].aucHwVerID[MAX_HW_ID_LEN] = '\0'; /*  添加结尾符 */

        iHwBoardVerSN_g = aHwVerMapArr[iHwMapCnt-1].iHwVerSN;
    }

    /* 平台类型区分
     */
    iHwMapCnt = sizeof(aBasicHwVerMapArr)/sizeof(aBasicHwVerMapArr[0]);
    bMapSuccess = FALSE;
    for (i = 0; i<iHwMapCnt; i++)
    {
        pPos = strstr(strHwID, aBasicHwVerMapArr[i].aucHwVerID);

        if (pPos != NULL)
        {
            /* 若找到 */
            bMapSuccess = TRUE;
            bdType_g = aBasicHwVerMapArr[i].iHwVerSN;

            break;
        }
    }

    /* 缺省设定为EDP01平台
     */
    if (!bMapSuccess)
    {
        bdType_g = BOARD_TYPE_E01;
    }

    return EP_SUCCESS;
}

/* 版本号写入
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void VER_WrVer(void)
{
    char TempInfo[256];

    /* 序号从1开始,存在一一对应关系,可以利用
     */
    sprintf(TempInfo, "硬件版本序列号: %s\n", aHwVerMapArr[iHwBoardVerSN_g-1].aucHwVerID);
    LOG_Write(LOG_KERNEL, TempInfo, NULL);

    /* 每个平台有两个名称
     */
    sprintf(TempInfo, "硬件平台类型: %s", aBasicHwVerMapArr[2*bdType_g].aucHwVerID);
    LOG_Write(LOG_KERNEL, TempInfo, NULL);

    sprintf(TempInfo, "BSP版本号: %x.%x%s", HI8(GetBspVer()), LO8(GetBspVer()), aucsysBspVer);
    LOG_Write(LOG_KERNEL, TempInfo, NULL);

    sprintf(TempInfo, "平台软件版本号: %s", EP_SYS_SW_VER);
    LOG_Write(LOG_KERNEL, TempInfo, NULL);
}

/***********************************************************************
* VER_GetHwBoardSN - 获得硬件版本序列号
*
* RETURNS: 硬件版本序列号，见HW_VER_SN_TYPE类型中定义
*
* Alert: 若返回UNKOWN_BOARD，则表示BSP未支持该版本硬件
*
*/
int VER_GetHwBoardSN()
{
    return iHwBoardVerSN_g;
}

/*功能：获得配置规约版本号 　
  参数：无
  返回：版本号
       4 BCD numbers.16位,高8位为整数位,低8位为小数位 */
uint16_t  GetCfgProtocolVer()
{
    return  SI_SysVer_g.unCfgProtocolVer;
}

/*功能：获得软硬件配置程序版本号 　
  参数：无
  返回：版本号
       4 BCD numbers.16位,高8位为整数位,低8位为小数位 */
uint16_t  GetCfgPrgVer()
{
    return  SI_SysVer_g.unCfgPrgVer;
}

/*功能：获得硬件配置版本号，暂时无实际意义
  参数：无
  返回：版本号
       4 BCD numbers.16位,高8位为整数位,低8位为小数位 */
uint16_t  GetHdCfgVer()
{
    return  SI_SysVer_g.unHdCfgVer;
}

/*功能：获得软件配置版本，暂时无实际意义
  参数：无
  返回：版本号
       4 BCD numbers.16位,高8位为整数位,低8位为小数位 */
uint16_t  GetSwCfgVer()
{
    return  SI_SysVer_g.unSwCfgVer;
}


/*功能：获得逻辑图程序版本号，暂时无实际意义
  参数：无
  返回：版本号
       4 BCD numbers.16位,高8位为整数位,低8位为小数位 */
uint16_t  GetLogicPrgVer()
{
    return  SI_SysVer_g.unLogicPrgVer;
}


/*功能：获得逻辑图配置版本号，暂时无实际意义
  参数：无
  返回：版本号
       4 BCD numbers.16位,高8位为整数位,低8位为小数位 */
uint16_t  GetLogicVer()
{
    return  SI_SysVer_g.unLogicVer;
}

/*功能：获得平台软件CRC  　
  参数：无
  返回：16位CCITT的CRC
        */
uint16_t  VER_GetSysSwCRC()
{
    uint16_t unVxCrc;

    unVxCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    return   unVxCrc;
}

#if defined(EDP03_BUILD) && (defined(EDP03_LOWPROTECT_BUILD) || defined(EDP03_STABCONTROL_BUILD)) 		/* EDP03平台提供mmi.out和usb.out */
/*功能：获得MMI软件CRC  　
  参数：无
  返回：16位CCITT的CRC
        */
uint16_t  VER_GetMmiSwCRC()
{
    uint16_t unVxCrc;

    unVxCrc=FT_File_CRC16(EP_EDP_MMI_FILE);
    return   unVxCrc;
}

/*功能：获得USB软件CRC  　
  参数：无
  返回：16位CCITT的CRC
        */
uint16_t  VER_GetUsbSwCRC()
{
    uint16_t unVxCrc;

    unVxCrc=FT_File_CRC16(EP_EDP_USB_FILE);
    return   unVxCrc;
}
#endif

/*功能：获得应用软件CRC  　
        在实现动态加载功能后的版本，用户界面显示的CRC，
        为了便于现场升级，当逻辑图中配置了发布CRC时， 此时返回逻辑图中配置的发布CRC 　
  参数：无
  返回：16位CCITT的CRC
        */
uint16_t  VER_GetUsrSwCRC()
{

    uint16_t unVxCrc;

    unVxCrc=FT_File_CRC16(EP_EDP_APP_FILE);

    return   unVxCrc;


}

/*功能：获得　保护配置总的版本号  逻辑图中设置
  参数：无
  返回：指向保护配置总的版本号字符串的指针 */
uint8_t  *  VER_GetCfgVer()
{
    return  SI_SysVer_g.aucSysVer;
}

/*功能：获得保护配置软件的总CRC，包括软硬件配置和逻辑图 　
  参数：无
  返回：16位CCITT的CRC
        */
uint16_t  VER_GetCfgCRC()
{
    uint16_t unVxCrc;
    uint16_t unCrc;
    uint8_t aucBuf[6];

    unVxCrc=FT_File_CRC16(EP_HW_CFG_FILE);
    aucBuf[0]=LO8(unVxCrc);
    aucBuf[1]=HI8(unVxCrc);
    unVxCrc=FT_File_CRC16(EP_SW_CFG_FILE);
    aucBuf[2]=LO8(unVxCrc);
    aucBuf[3]=HI8(unVxCrc);
    unVxCrc=FT_File_CRC16(EP_LGC_CFG_FILE);
    aucBuf[4]=LO8(unVxCrc);
    aucBuf[5]=HI8(unVxCrc);
    unCrc=0;
    unVxCrc=EP_CCITT_CRC16(aucBuf, 6, unCrc);

    return   unVxCrc;
}

/* 获取软件配置校验CRC.
 * Para:
 *     NONE.
 * Return:
 *     CRC.
 */
uint16_t VER_GetSwCfgCRC()
{
    uint16_t unCrc;
    uint16_t unCrcTmpOpt;

    unCrc = FT_File_CRC16(EP_SW_CFG_FILE);

    /*如果选配文件更改，则录波文件列表不上送*/
    if(FT_Is_File(EP_FUNCOPTION_FILE))
    {
        unCrcTmpOpt = FT_File_CRC16(EP_FUNCOPTION_FILE);
        unCrc=EP_CCITT_CRC16((uint8_t *)(&unCrcTmpOpt), 2, unCrc);
    }
    return unCrc;
}

/* 外部调用获取软件配置校验CRC.
 * Para:
 *     NONE.
 * Return:
 *     CRC.
 */
uint16_t VER_ExtGetSwCfgCRC()
{
    return SI_SysVer_g.unSwCfgCRC;
}

#ifdef VXWORKS_ROM
/************************************ 系统总的版本相关信息*****************************/
/*功能：获得总CRC，包括BOOTROM，BSP，平台软件，软硬件配置和逻辑图，保护应用软件  　
  参数：无
  返回：16位CCITT的CRC
        */
uint16_t  VER_GetTotalCRC()
{
    uint16_t unHwCrc;
    uint16_t unSwCrc;
    uint16_t unLogicCrc;
    uint8_t aucBuf[18];
    uint16_t unCrc=0;
    uint16_t unFuncOptCrcLen=0;/*如果选配文件不存在，则=0;否则=2*/

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
#ifdef EDP03_BUILD

#if defined(EDP03_STABCONTROL_BUILD) || defined(EDP03_LOWPROTECT_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[6]=LO8(unLogicCrc);
    aucBuf[7]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_APP_FILE);
    aucBuf[8]=LO8(unLogicCrc);
    aucBuf[9]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_USB_FILE);
    aucBuf[10]=LO8(unLogicCrc);
    aucBuf[11]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_MMI_FILE);
    aucBuf[12]=LO8(unLogicCrc);
    aucBuf[13]=HI8(unLogicCrc);
#endif /* end of EDP03_STABCONTROL_BUILD || EDP03_LOWPROTECT_BUILD */

#if defined(EDP03_INTELBOX_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[6]=LO8(unLogicCrc);
    aucBuf[7]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_APP_FILE);
    aucBuf[8]=LO8(unLogicCrc);
    aucBuf[9]=HI8(unLogicCrc);
#endif		/* end of EDP03_INTELBOX_BUILD */

#endif		/* end of EDP03_BUILD */

#if defined(EDP_01_02_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[6]=LO8(unLogicCrc);
    aucBuf[7]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_APP_FILE);
    aucBuf[8]=LO8(unLogicCrc);
    aucBuf[9]=HI8(unLogicCrc);
    if (FT_Is_File(EP_FUNCOPTION_FILE))
    {
        unLogicCrc=FT_File_CRC16(EP_FUNCOPTION_FILE);
        aucBuf[10]=LO8(unLogicCrc);
        aucBuf[11]=HI8(unLogicCrc);
        unFuncOptCrcLen=2;
    }
#endif		/* end of EDP03_INTELBOX_BUILD */

#if defined(EXCITE_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[6]=LO8(unLogicCrc);
    aucBuf[7]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_APP_FILE);
    aucBuf[8]=LO8(unLogicCrc);
    aucBuf[9]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_MMI_FILE);
    aucBuf[10]=LO8(unLogicCrc);
    aucBuf[11]=HI8(unLogicCrc);
#endif

    unCrc=0;

#ifdef EDP03_BUILD

#if defined(EDP03_STABCONTROL_BUILD) || defined(EDP03_LOWPROTECT_BUILD)
    return  EP_CCITT_CRC16(aucBuf, 14, unCrc);
#endif /* end of EDP03_STABCONTROL_BUILD || EDP03_LOWPROTECT_BUILD */

#if defined(EDP03_INTELBOX_BUILD)
    return  EP_CCITT_CRC16(aucBuf, 10, unCrc);
#endif		/* end of EDP03_BUILD */

#endif 		/* end of EDP03_BUILD */

#if defined(EDP_01_02_BUILD)
    return  EP_CCITT_CRC16(aucBuf, 10+unFuncOptCrcLen, unCrc);/*选配文件可能不存在*/
#endif

#if defined(EXCITE_BUILD)
    return  EP_CCITT_CRC16(aucBuf, 12, unCrc);
#endif		/* end of EXCITE_BUILD */
#else
    return  EP_CCITT_CRC16(aucBuf, 6, unCrc);
#endif  /* end of EDP_DYNAMICLOAD */
}

/*功能：VxWorks_Rom方式下获得平台软件CRC  　
  返回：16位CCITT的CRC

  详细：获取平台部分提供程序crc
  动态加载情况下：
  edp03-|-EDP03_STABCONTROL_BUILD: EP_EDP_SYS_FILE,EP_EDP_USB_FILE,EP_EDP_MMI_FILE
        |-EDP03_LOWPROTECT_BUILD:  EP_EDP_SYS_FILE,EP_EDP_USB_FILE,EP_EDP_MMI_FILE
        |-EDP03_INTELBOX_BUILD:    EP_EDP_SYS_FILE

  edp02-|
  edp01-|-                         EP_EDP_SYS_FILE

  EXCITE_BUILD-|-                  EP_EDP_SYS_FILE,EP_EDP_MMI_FILE
  非动态加载
    返回crc: 0
        */
uint16_t  VER_GetEdpSwCRC()
{
    uint16_t unLogicCrc;
    uint8_t aucBuf[18];
    uint16_t unCrc=0;

#if defined(EDP_DYNAMICLOAD)

#ifdef EDP03_BUILD
#if defined(EDP03_STABCONTROL_BUILD) || defined(EDP03_LOWPROTECT_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[0]=LO8(unLogicCrc);
    aucBuf[1]=HI8(unLogicCrc);
#endif
#if defined(EDP03_INTELBOX_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[0]=LO8(unLogicCrc);
    aucBuf[1]=HI8(unLogicCrc);
#endif
#endif		/* end of EDP03_BUILD */

#if defined(EDP_01_02_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[0]=LO8(unLogicCrc);
    aucBuf[1]=HI8(unLogicCrc);
#endif

#if defined(EXCITE_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[0]=LO8(unLogicCrc);
    aucBuf[1]=HI8(unLogicCrc);
#endif

    unCrc=0;

#ifdef EDP03_BUILD
#if defined(EDP03_STABCONTROL_BUILD) || defined(EDP03_LOWPROTECT_BUILD)
    return  unLogicCrc;
#endif
#if defined(EDP03_INTELBOX_BUILD)
    return  unLogicCrc;
#endif
#endif		/* end of EDP03_BUILD */

#if defined(EDP_01_02_BUILD)
    return  unLogicCrc;
#endif

#if defined(EXCITE_BUILD)
    return  unLogicCrc;
#endif

#else
    return  EP_CCITT_CRC16(aucBuf, 0, unCrc);

#endif  /* end of EDP_DYNAMICLOAD */
}


#else

/************************************ 系统总的版本相关信息*****************************/
/*功能：获得总CRC，包括BOOTROM，BSP，平台软件，软硬件配置和逻辑图，保护应用软件  　
  参数：无
  返回：16位CCITT的CRC
        */
uint16_t  VER_GetTotalCRC()
{

    uint16_t unVxCrc;
    uint16_t unHwCrc;
    uint16_t unSwCrc;
    uint16_t unLogicCrc;
    uint8_t aucBuf[18];
    uint16_t unCrc=0;

    unVxCrc=FT_File_CRC16(EP_VX_RUN_FILE);
    aucBuf[0]=LO8(unVxCrc);
    aucBuf[1]=HI8(unVxCrc);
    unHwCrc=FT_File_CRC16(EP_HW_CFG_FILE);
    aucBuf[2]=LO8(unHwCrc);
    aucBuf[3]=HI8(unHwCrc);
    unSwCrc=FT_File_CRC16(EP_SW_CFG_FILE);
    aucBuf[4]=LO8(unSwCrc);
    aucBuf[5]=HI8(unSwCrc);
    unLogicCrc=FT_File_CRC16(EP_LGC_CFG_FILE);
    aucBuf[6]=LO8(unLogicCrc);
    aucBuf[7]=HI8(unLogicCrc);

#if defined(EDP_DYNAMICLOAD)
#ifdef EDP03_BUILD

#if defined(EDP03_STABCONTROL_BUILD) || defined(EDP03_LOWPROTECT_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[8]=LO8(unLogicCrc);
    aucBuf[9]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_APP_FILE);
    aucBuf[10]=LO8(unLogicCrc);
    aucBuf[11]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_USB_FILE);
    aucBuf[12]=LO8(unLogicCrc);
    aucBuf[13]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_MMI_FILE);
    aucBuf[14]=LO8(unLogicCrc);
    aucBuf[15]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_AUTOEXEC_FILE);
    aucBuf[16]=LO8(unLogicCrc);
    aucBuf[17]=HI8(unLogicCrc);
#endif /* end of EDP03_STABCONTROL_BUILD || EDP03_LOWPROTECT_BUILD */

#if defined(EDP03_INTELBOX_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[8]=LO8(unLogicCrc);
    aucBuf[9]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_APP_FILE);
    aucBuf[10]=LO8(unLogicCrc);
    aucBuf[11]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_AUTOEXEC_FILE);
    aucBuf[12]=LO8(unLogicCrc);
    aucBuf[13]=HI8(unLogicCrc);
#endif		/* end of EDP03_INTELBOX_BUILD */

#endif		/* end of EDP03_BUILD */

#if defined(EDP_01_02_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[8]=LO8(unLogicCrc);
    aucBuf[9]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_APP_FILE);
    aucBuf[10]=LO8(unLogicCrc);
    aucBuf[11]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_AUTOEXEC_FILE);
    aucBuf[12]=LO8(unLogicCrc);
    aucBuf[13]=HI8(unLogicCrc);
#endif		/* end of EDP03_INTELBOX_BUILD */

#if defined(EXCITE_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[8]=LO8(unLogicCrc);
    aucBuf[9]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_APP_FILE);
    aucBuf[10]=LO8(unLogicCrc);
    aucBuf[11]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_MMI_FILE);
    aucBuf[12]=LO8(unLogicCrc);
    aucBuf[13]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_AUTOEXEC_FILE);
    aucBuf[14]=LO8(unLogicCrc);
    aucBuf[15]=HI8(unLogicCrc);
#endif

    unCrc=0;

#ifdef EDP03_BUILD

#if defined(EDP03_STABCONTROL_BUILD) || defined(EDP03_LOWPROTECT_BUILD)
    return  EP_CCITT_CRC16(aucBuf, 18, unCrc);
#endif /* end of EDP03_STABCONTROL_BUILD || EDP03_LOWPROTECT_BUILD */

#if defined(EDP03_INTELBOX_BUILD)
    return  EP_CCITT_CRC16(aucBuf, 14, unCrc);
#endif		/* end of EDP03_BUILD */

#endif 		/* end of EDP03_BUILD */

#if defined(EDP_01_02_BUILD)
    return  EP_CCITT_CRC16(aucBuf, 14, unCrc);
#endif

#if defined(EXCITE_BUILD)
    return  EP_CCITT_CRC16(aucBuf, 16, unCrc);
#endif		/* end of EXCITE_BUILD */
#else
    return  EP_CCITT_CRC16(aucBuf, 8, unCrc);
#endif  /* end of EDP_DYNAMICLOAD */
}

/*功能：非vxworks_rom方式下获得平台软件CRC  　
  参数：无
  返回：16位CCITT的CRC

  详细：获取平台部分提供程序crc
  动态加载情况下：
  edp03-|-EDP03_STABCONTROL_BUILD: EP_VX_RUN_FILE,EP_EDP_SYS_FILE,EP_EDP_USB_FILE,EP_EDP_MMI_FILE
        |-EDP03_LOWPROTECT_BUILD:  EP_VX_RUN_FILE,EP_EDP_SYS_FILE,EP_EDP_USB_FILE,EP_EDP_MMI_FILE
        |-EDP03_INTELBOX_BUILD:    EP_VX_RUN_FILE,EP_EDP_SYS_FILE

  edp02-|
  edp01-|-                         EP_VX_RUN_FILE,EP_EDP_SYS_FILE

  EXCITE_BUILD-|-                  EP_VX_RUN_FILE,EP_EDP_SYS_FILE,EP_EDP_MMI_FILE

  非动态加载:                      EP_VX_RUN_FILE

        */
uint16_t  VER_GetEdpSwCRC()
{
    uint16_t unVxCrc;
    uint16_t unHwCrc;
    uint16_t unSwCrc;
    uint16_t unLogicCrc;
    uint8_t aucBuf[18];
    uint16_t unCrc=0;

    unVxCrc=FT_File_CRC16(EP_VX_RUN_FILE);
    aucBuf[0]=LO8(unVxCrc);
    aucBuf[1]=HI8(unVxCrc);

#if defined(EDP_DYNAMICLOAD)
#ifdef EDP03_BUILD

#if defined(EDP03_STABCONTROL_BUILD) || defined(EDP03_LOWPROTECT_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[2]=LO8(unLogicCrc);
    aucBuf[3]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_USB_FILE);
    aucBuf[4]=LO8(unLogicCrc);
    aucBuf[5]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_MMI_FILE);
    aucBuf[6]=LO8(unLogicCrc);
    aucBuf[7]=HI8(unLogicCrc);
#endif /* end of EDP03_STABCONTROL_BUILD || EDP03_LOWPROTECT_BUILD */

#if defined(EDP03_INTELBOX_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[2]=LO8(unLogicCrc);
    aucBuf[3]=HI8(unLogicCrc);
#endif		/* end of EDP03_INTELBOX_BUILD */

#endif		/* end of EDP03_BUILD */

#if defined(EDP_01_02_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[2]=LO8(unLogicCrc);
    aucBuf[3]=HI8(unLogicCrc);
#endif		/* end of EDP03_INTELBOX_BUILD */

#if defined(EXCITE_BUILD)
    unLogicCrc=FT_File_CRC16(EP_EDP_SYS_FILE);
    aucBuf[2]=LO8(unLogicCrc);
    aucBuf[3]=HI8(unLogicCrc);
    unLogicCrc=FT_File_CRC16(EP_EDP_MMI_FILE);
    aucBuf[4]=LO8(unLogicCrc);
    aucBuf[5]=HI8(unLogicCrc);
#endif

    unCrc=0;

#ifdef EDP03_BUILD

#if defined(EDP03_STABCONTROL_BUILD) || defined(EDP03_LOWPROTECT_BUILD)
    return  EP_CCITT_CRC16(aucBuf, 8, unCrc);
#endif /* end of EDP03_STABCONTROL_BUILD || EDP03_LOWPROTECT_BUILD */

#if defined(EDP03_INTELBOX_BUILD)
    return  EP_CCITT_CRC16(aucBuf, 4, unCrc);
#endif		/* end of EDP03_BUILD */

#endif 		/* end of EDP03_BUILD */

#if defined(EDP_01_02_BUILD)
    return  EP_CCITT_CRC16(aucBuf, 4, unCrc);
#endif

#if defined(EXCITE_BUILD)
    return  EP_CCITT_CRC16(aucBuf, 6, unCrc);
#endif		/* end of EXCITE_BUILD */
#else
    return  unVxCrc;
#endif  /* end of EDP_DYNAMICLOAD */
}
#endif


/*
功能：获得平台支撑组件的CRC信息
参数： puiRtPlatCRC ，返回平台组件的CRC，若该法获得，则该值无意义
返回：若获得失败，则返回FALSE
                否则返回TRUE
注意：由BSP版本号作为CRC原始输入，和平台支撑软件的CRC（详见VER_GetEdpSwCRC()具体实现）计算组合而成，
若对EDP03而言，则还需要计算mmi.out的CRC（但对mega16的程序，最好不包括，因为该程序升级之后，会自动消失掉）
*/
uint16_t VER_UN_GetPlatCRC()
{
    uint16_t unCrc;

    unCrc = FT_File_CRC16(EP_EDP_SYS_FILE);

    return unCrc;
}

/*
功能：获得平台支撑组件的版本信息
参数： pucRtPlatVer ,    返回平台组件的版本号字符串，若无法获得版本号，则该串应该返回空串。
该字符串空间由调用方分配，由被调用方填充iVerStrLen,该字符串空间分配的长度。
返回：若获得失败，则返回FALSE
                否则返回TRUE
 */
BOOL VER_UN_GetPlatVer(uint8_t *pucRtPlatVer,int iVerStrLen)
{
    uint8_t *ucVer;

    ucVer=GetSysSwVer();
    if(iVerStrLen>strlen(ucVer))
    {
        strncpy(pucRtPlatVer, ucVer, strlen(ucVer));
        pucRtPlatVer[strlen(ucVer)]='\0';
        return TRUE;
    }
    else
        return FALSE;
}

/*
功能：获得平台支撑组件的特征码
参数： puiRtPlatLabel , 返回平台组件的特征码
返回：若获得失败，则返回FALSE
                否则返回TRUE
 */
uint16_t VER_UN_GetPlatLabel()
{
    uint16_t unLbl=0;

    unLbl=GetSysPlatLabel();
    return unLbl;

}

/*
功能：获得保护应用组件的CRC信息
参数： puiRtRelayCRC ，返回保护应用组件的CRC，若该法获得，则该值无意义,
返回：若获得失败，则返回FALSE
                否则返回TRUE
注意：计算返回实际的CRC，由平台支撑组件的特征码，作为原始输入，然后再计算edpapp.out,
      若包括保护配置文件的话，则还需要再依次计算hwcfg.ehc,swcfg.esc,logic.egs
*/
uint16_t VER_UN_GetRelayCRC()
{
    uint16_t unCrc;
    uint8_t aucBuf[10]="";

    aucBuf[0]=LO8(VER_UN_GetPlatLabel());
    aucBuf[1]=HI8(VER_UN_GetPlatLabel());
    unCrc=FT_File_CRC16(EP_EDP_APP_FILE);
    aucBuf[2]=LO8(unCrc);
    aucBuf[3]=HI8(unCrc);
    if(VER_UN_RelayCRCIsCoverCfg())
    {
        unCrc=FT_File_CRC16(EP_HW_CFG_FILE);
        aucBuf[4]=LO8(unCrc);
        aucBuf[5]=HI8(unCrc);
        unCrc=FT_File_CRC16(EP_SW_CFG_FILE);
        aucBuf[6]=LO8(unCrc);
        aucBuf[7]=HI8(unCrc);
        unCrc=FT_File_CRC16(EP_LGC_CFG_FILE);
        aucBuf[8]=LO8(unCrc);
        aucBuf[9]=HI8(unCrc);
        return EP_CCITT_CRC16(aucBuf, 10, 0);
    }
    else
    {
        return EP_CCITT_CRC16(aucBuf, 4, 0);
    }

    return   unCrc;
}

/*
功能：获得mmi的版本信息
参数： pucMmiVer ,    返回mmi组件的版本号字符串，若无法获得版本号，则该串应该返回空串。
该字符串空间由调用方分配，由被调用方填充iVerStrLen,该字符串空间分配的长度。
返回：若获得失败，则返回FALSE
                否则返回TRUE
注意：该版本信息，由mmi在程序中设置。
 */
BOOL VER_UN_GetMmiVer(uint8_t *pucRtMmiVer, int iVerStrLen)
{
    uint8_t ucVer[32]="";
    uint8_t uclen=0;

    if(pGetMmiVerEntryFunc_g==NULL)
    {
        /* GetMmiSwVer(ucVer,sizeof(ucVer)); */
        pucRtMmiVer[uclen]='\0';

        return FALSE;
    }
    else
    {
        (*pGetMmiVerEntryFunc_g)(ucVer,sizeof(ucVer));
    }

    uclen=strlen(ucVer);
    if(iVerStrLen>uclen)
    {
        strncpy(pucRtMmiVer, ucVer, uclen);
        pucRtMmiVer[uclen]='\0';
        return TRUE;
    }
    else
        return FALSE;
}

/*
功能：获得保护程序的版本信息
参数： pucRtRelayVer ,    返回保护应用组件的版本号字符串，若无法获得版本号，则该串应该返回空串。
该字符串空间由调用方分配，由被调用方填充iVerStrLen,该字符串空间分配的长度。
返回：若获得失败，则返回FALSE
                否则返回TRUE
注意：该版本信息，由保护人员在程序中设置，在extend.c中。
 */
BOOL VER_UN_GetRelayVer(uint8_t *pucRtRelayVer, int iVerStrLen)
{
    uint8_t ucVer[32]="";
    uint8_t uclen=0;

    if(pGetUsrVerEntryFunc_g==NULL)
    {
        GetUsrSwVer(ucVer,sizeof(ucVer));
    }
    else
    {
        (*pGetUsrVerEntryFunc_g)(ucVer,sizeof(ucVer));
    }

    uclen=strlen(ucVer);
    if(iVerStrLen>uclen)
    {
        strncpy(pucRtRelayVer, ucVer, uclen);
        pucRtRelayVer[uclen]='\0';
        return TRUE;
    }
    else
        return FALSE;
}

/*
功能：获得保护应用组件整体版本信息
参数： pucRtRelayVer ,    返回保护应用组件的版本号字符串，若无法获得版本号，则该串应该返回空串。
该字符串空间由调用方分配，由被调用方填充iVerStrLen,该字符串空间分配的长度。
返回：若获得失败，则返回FALSE
                否则返回TRUE
注意：该版本信息，由保护人员在逻辑图中中设置，若逻辑图未设置,则使用extend.c函数GetUsrSwVer()。
 */
BOOL VER_UN_GetRelayPrioVer(uint8_t *pucRtRelayVer, int iVerStrLen)
{
    uint8_t ucVer[32]="";
    uint8_t uclen=0;

    uclen=strlen(SI_SysVer_g.aucSysVer);

    if(uclen>0&&uclen<sizeof(ucVer))
    {
        strncpy(ucVer,SI_SysVer_g.aucSysVer,uclen);
    }
    else
    {
        if(pGetUsrVerEntryFunc_g==NULL)
        {
            GetUsrSwVer(ucVer,sizeof(ucVer));
        }
        else
        {
            (*pGetUsrVerEntryFunc_g)(ucVer,sizeof(ucVer));
        }
    }
    uclen=strlen(ucVer);
    if(iVerStrLen>uclen)
    {
        strncpy(pucRtRelayVer, ucVer, uclen);
        pucRtRelayVer[uclen]='\0';
        return TRUE;
    }
    else
        return FALSE;
}

/*
功能：获得保护应用组件CRC是否包含保护配置文件
参数：无
返回：若计算保护应用组件CRC包含保护配置文件，则返回TRUE,否则返回FALSE
注意：该设置由保护人员在程序中设置，在extend.c中，可由保护人员定义一个宏，定义不包含保护配置文件（缺省是包含的）
该函数程序实现，放在extend.c中。
 */
BOOL VER_UN_RelayCRCIsCoverCfg()
{
    return UnVerInfo_g.bRelayCRCIsCoverCfgFlag;
}

/*
功能：获得保护配置的版本信息
参数： pucRtRelayCfgVer ,    返回保护应用组件的版本号字符串，若无法获得版本号，则该串应该返回空串。
该字符串空间由调用方分配，由被调用方填充iVerStrLen,该字符串空间分配的长度。
返回：若获得失败，则返回FALSE,否则返回TRUE
注意：如果保护和配置整体发布,则返回空字符串,否则优先返回逻辑图中设置的发布版本信息。
 */
BOOL VER_UN_GetRelayCfgVer(uint8_t *pucRtRelayCfgVer, int iVerStrLen)
{
    if(VER_UN_RelayCRCIsCoverCfg())
    {
        pucRtRelayCfgVer[0]='\0';
        return TRUE;
    }
    else
        return VER_UN_GetRelayPrioVer(pucRtRelayCfgVer,iVerStrLen);
}


/*
功能：获得功能选配组件的CRC信息
参数： 无
返回：功能选配组件的CRC
*/
uint16_t VER_UN_GetFuncOptCRC()
{
    uint16_t unCrc;

    /* 兼容无功能选配文件模式 */
    if (FT_Is_File(EP_FUNCOPTION_FILE))
    {
        unCrc = FT_File_CRC16(EP_FUNCOPTION_FILE);
    }
    else
    {
        unCrc = 0;
    }

    return unCrc;
}

/*
功能：获得MMI的CRC信息
*/
uint16_t VER_UN_GetMmiCRC()
{
    uint16_t unCrc=0;
#if defined(EDP03_STABCONTROL_BUILD) || defined(EDP03_LOWPROTECT_BUILD)
    uint8_t aucBuf[4] = "";
#endif

#if defined(EDP03_BUILD)
#if defined(EDP03_STABCONTROL_BUILD) || defined(EDP03_LOWPROTECT_BUILD)
    unCrc=FT_File_CRC16(EP_EDP_MMI_FILE);
    aucBuf[0] = LO8(unCrc);
    aucBuf[1] = HI8(unCrc);
    unCrc = FT_File_CRC16(EP_EDP_USB_FILE);
    aucBuf[2] = LO8(unCrc);
    aucBuf[3] = HI8(unCrc);
    unCrc = EP_CCITT_CRC16(aucBuf, 4, 0);
#endif
#elif defined(EXCITE_BUILD)
    unCrc=FT_File_CRC16(EP_EDP_MMI_FILE);
#else
    if(FT_Is_File(EP_ROOT"/mucfg.xml"))
    {
        unCrc=FT_File_CRC16(EP_ROOT"/mucfg.xml");
    }
#endif
    return unCrc;

}

EP_STATUS RE_Refresh_EdpVer_Info(EP_GET_USR_VER_FUNC_TYPE  pGetUsrVerEntryFunc)
{
    pGetUsrVerEntryFunc_g=pGetUsrVerEntryFunc;
    return   EP_SUCCESS;
}

EP_STATUS RE_Refresh_EdpMmiVer_Info(EP_GET_USR_VER_FUNC_TYPE  pGetMmiVerEntryFunc)
{
    pGetMmiVerEntryFunc_g=pGetMmiVerEntryFunc;
    return   EP_SUCCESS;
}