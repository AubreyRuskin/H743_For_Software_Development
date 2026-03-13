/**************************************************************************
EDP_UnifiedCfgParsePrivate.h

九统一配置文件解析头文件

Copyright (c) 2015 SNAC(Guodian Nanjing Automation Co., Ltd.)
All Rights Reserved.


History:
Revision     Date          Who            Comment
-----------------------------------------------------------------
V1.00         2015.07.16    kevin         初始版本
***************************************************************************/

#ifndef EDP_UNIFIEDCFGDEPARSEPRIVATE_H
#define EDP_UNIFIEDCFGDEPARSEPRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "EDP_UnifiedCfgDefine.h"


/*装置接线方式.01:外接线方式. 02:HSB总线方式*/
#define CONNECTION_TYPE_LINE  1    /*装置接线方式，外接线方式*/
#define CONNECTION_TYPE_HSB  2    /*装置接线方式，HSB总线方式*/

/*板子类型1：普通CPU板，2：CC板，3：冗余从CPU板*/
#define BOARD_TYPE_NORMAL_CPU   1        /*普通CPU板*/
#define BOARD_TYPE_CC   2                            /*CC板*/
#define BOARD_TYPE_REDUN_CPU   3             /*冗余从CPU板*/
#define BOARD_TYPE_MEA_CPU 4            /*测控CPU*/
/*该级联端口转发的数据类型，1：gs，2：sv，3：sv+gs*/
#define PORT_TRANSFOR_DATA_TYPE_GS      1        /*转发的GS报文*/
#define PORT_TRANSFOR_DATA_TYPE_SV      2        /*转发的SV报文*/
#define PORT_TRANSFOR_DATA_TYPE_SVGS      3      /*转发的SV+GS报文*/

/*该级联口转发goose的模式：1：普通模式。2：冗余CPU模式*/
#define PORT_TRANSFOR_GS_MODE_NORMAL  1          /*普通模式*/
#define PORT_TRANSFOR_GS_MODE_REDUN  2           /*冗余CPU模式*/



/*
描述: 得到XML元素下的子元素个数
参数:
GOOSE SUB的XML指针.
pSubGooseGcb: sub的结构
返回值: 解析是否成功
 */
extern int EDP_GetXmlElementCnt(mxml_node_t *pRoot, char *elementName);

/*
描述: 解析CCD文件的私有配置文件 入口函数.
参数:     CCD文件的指针.
返回值: 解析是否成功
 */
extern int EDP_LoadPrivateFile(char *pFileName);


/*
描述:获取private.xml中的borad个数
参数:无
返回值:返回private.xml中的borad个数
*/
extern int EDP_GetPrivateBoardCnt();
/*
描述:获取private.xml中的borad 首指针
参数:无
返回值:返回private.xml中的borad 首指针
*/
extern PRIVATE_BOARD *EDP_GetPrivateBoardPtrFromIndex();

/*
描述:获取private.xml中的指定iBoardId的borad 首指针
参数:
iBoardId: 板子的BoardId
返回值:返回private.xml中的borad 首指针
*/
extern PRIVATE_BOARD *EDP_GetPrivateBoardPtrFromId(int iBoardId);

/*
描述:获取private.xml中的指定iNodeAddr的borad 首指针
参数:
iNodeAddr: 板子的NodeAddr
返回值:返回private.xml中的borad 首指针
*/
PRIVATE_BOARD *EDP_GetPrivateBoardPtrFromNodeAddr(int iNodeAddr);

/*
描述: 获取private.xml的指针
参数:无
返回值:返回private.xml的指针
*/
extern PRIVATE_CFG *EDP_GetPrivatePtr();

/*
描述:得到当前CPU板的接收端口信息
参数:
addrIndex:索引
iNodeAddr:板件nodeAddr(外绕线用1、2区分主从)
ucType: 采样还是GOOSE类型 PORT_TRANSFOR_DATA_TYPE_GS or PORT_TRANSFOR_DATA_TYPE_SV
pNetInfo:返回网络端口
返回值:是否成功
*/
extern BOOL EDP_GetCPUConnectPort(int addrIndex, int iNodeAddr, uint8_t ucType, NET_INFO *pNetInfo);

/*
描述:根据CPU的发送端口得到是哪一个CC端口的对侧的OutputId与之一致
参数:
pCpuBoardPort: CPU的发送端口
pCcResultBoardPort: 返回哪一个CC端口的对侧的OutputId与pCpuBoardPort一致
返回值:是否成功
*/
extern BOOL EDP_GetCCPortInfo(char *pCpuBoardPort, char *pCcResultBoardPort);


/*
描述:得到CC板个数,用来确定生成几个文件和转发关系时需要先知道个数
参数:无
返回值:是否成功
*/
extern uint8_t EDP_GetCCCnt();

/*
描述: 通过板件号获取板件HSB总线节点地址
参数:iBoardId: 板子的BoardId
返回值:返回节点地址
*/
extern int EDP_GetNodeAddrByBoardId(int iBoardId);

/*
描述: 通过板件号获取板件HSB总线节点地址
参数:iNodeAddr: 板子的NodeAddr
返回值:返回板件号
*/
extern int EDP_GetBoardIdByNodeAddr(int iNodeAddr);


/*
描述: 释放内存,Devcfg
参数:    NONE
返回值: 释放是否成功
 */
extern int EDP_FreeDevCfg();

#ifdef    __cplusplus
}
#endif

#endif
