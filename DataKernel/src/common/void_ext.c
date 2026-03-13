/* void_ext.c - This file contains all mapping tables to arithmetic parts */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 27dec02, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains all mapping tables to arithmetic parts.
*/

/* includes */
#include "logic.h"
#include  "filetool.h"

/* defines */

/* 说明:
	应用软件版本号,16位无符号整数,高8位为整数,低8位为小数,用BCD码,每4位表示0~9的一位10进制数,
  	最高4位表示整数的10位数,次高4位表示整数的个位数,次低4位表示小数的10^-1位,最低4位表示小数的10^-2位
  	比如10.99,实际应是整数位=10,小数位99,表示为16进制:0x1099
 	需要保护开发人员修改应用软件时更新,便于进行应用程序版本控制  */

#define USR_SW_VER "0.00"

/* globals */

// EP_EXT_ELEM_MAP aextmap[]= {};		/* 算法元件接口表 */

/* functions */

/***********************************************************************
* EDP_Example - Init function of the example logic part.
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS EDP_Example(
    EP_ELEMENT *pelm			/* 图元数据结构 */
);


/***********************************************************************
* EP_Ext_Elem_Num - 算法元件个数计算
*
* RETURNS: 算法元件个数
*
*/
// int EP_Ext_Elem_Num()
// {
//     return sizeof(aextmap)/sizeof(aextmap[0]);
// }

/***********************************************************************
* EP_Debug_Part - 调试函数
*
* RETURNS: 无
*
*/
// void EP_Debug_Part(void)
// {
//     volatile int i;

//     i++;
// }

/***********************************************************************
* GetUsrSwVer - 获得用户程序版本号
*
* RETURNS: 无
*
*/
// void GetUsrSwVer(uint8_t *ucUsrSysVer, int nStrlen)
// {
//     uint16_t unLen=0;
//     unLen=strlen(USR_SW_VER);
//     if(unLen>=nStrlen||unLen==0)
//     {
//         return;
//     }
//     strncpy(ucUsrSysVer,USR_SW_VER,unLen);
//     ucUsrSysVer[unLen]='\0';
//     return ;
// }
