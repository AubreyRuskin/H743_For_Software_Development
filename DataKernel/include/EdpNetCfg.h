/* EdpNegCfg.h - subroutine library for managing net configuration of CPU. */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 11aug07, zy first created.
01b, 30nov07, dy migrated to merged edtion.

*/

/*
DESCRIPTION
This module includes subroutine library for managing net configuration of CPU.
*/


#ifndef  EDP_NET_CFG_H
#define  EDP_NET_CFG_H

/* includes */

#include "edpbase.h"

#ifdef	__cplusplus
}
#endif

/* defines */

#define MAX_EDP_NET_NUM 6   		/* 最大允许网口数目 */

/* typedefs */

typedef struct			/* 每个网口的网络信息 */
{
    int iNetSeqNo;    		/* 该网口的编号，从0开始 */
    uint8_t aucIpAddr[4];					/* 比如192.168.0.123, aucIpAddr[0]=192， aucIpAddr[1]=168，aucIpAddr[0]=0，aucIpAddr[0]=123 */
    uint8_t aucIpMsk[4];									/* 比如255.255.0.0, aucIpMsk[0]=255， aucIpMsk[1]=255，aucIpMsk[2]=0，aucIpMsk[3]=0 */
    uint8_t aucMacAddr[6];					/* 比如22.33.44.55.66.77, aucMacAddr[0]=22, aucMacAddr[1]=33,aucMacAddr[2]=44,
                             									 aucMacAddr[3]=55,aucMacAddr[4]=66,aucMacAddr[5]=77 */
} ONE_NET_CFG_INFO;

typedef struct  		/* 所有网口的网络信息 */
{
    int iValidNetNum;       	/* 有效网口个数 */
    ONE_NET_CFG_INFO NetInfArr[MAX_EDP_NET_NUM];
} EDP_NET_CFG_INFO;

/* functions */

/***********************************************************************
* NT_NetCfgInit - 初始化网络配置
*
* RETURNS: EP_SUCCESS: 初始化成功
*                 其他, 初始化失败
*
*/
EP_STATUS  NT_NetCfgInit();

/***********************************************************************
* NT_GetNetRunCfg - 获得网络实际运行配置
*
* RETURNS: EP_SUCCESS: 初始化成功
*                 其他, 初始化失败
*
*/
EP_STATUS NT_GetNetRunCfg(
    EDP_NET_CFG_INFO  *pRtNetInfo		/* 供返回网络实际运行配置的变量地址,该变量本身,由调用方来分配 */
);

/***********************************************************************
* NT_SetOneNetIpAddr - 设置某网口的IP地址
*
* RETURNS: EP_SUCCESS: 初始化成功
*                 其他, 初始化失败
*
*/
EP_STATUS NT_SetOneNetIpAddr(
    int iNetSeqNo,		/* 网络号,从0开始 */
    uint8_t *pIpAddrBase				/* IP地址串基址 */
);

/*功能,设置某网口的IP子网掩码，目前不能设置。
  参数,  iNetSeqNo,网络号,从0开始
         pIpMskBase,子网掩码串基址*/
EP_STATUS   NT_SetOneNetIpMsk(int  iNetSeqNo,uint8_t *pIpMskBase);

#ifdef	__cplusplus
}
#endif

#endif                                  /* EDP_NET_CFG_H */
