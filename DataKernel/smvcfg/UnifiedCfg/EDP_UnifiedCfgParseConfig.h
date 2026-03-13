/**************************************************************************
EDP_UnifiedCfgParseConfig.h

九统一配置信息解析头文件

Copyright (c) 2015 SNAC(Guodian Nanjing Automation Co., Ltd.)
All Rights Reserved.


History:
Revision     Date          Who            Comment
-----------------------------------------------------------------
V1.00         2015.07.16    kevin         初始版本
***************************************************************************/

#ifndef EDP_UNIFIEDCFGDEPARSECONFIG_H
#define EDP_UNIFIEDCFGDEPARSECONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "EDP_UnifiedCfgDefine.h"

#define GDeviceNum 4    /*过程层GOOSE ld实例名称*/
#define VDeviceNum 3    /*过程层SV ld实例名称*/

/*短地址里面的TYPE */
#define GOOSE_SUB_RSV "Sub_Rsv"     /*GOOSE开入量接收*/
#define GOOSE_SUB_MEA "Sub_Mea"     /*GOOSE模拟量接收*/
#define GOOSE_PUB_RSV "Pub_Rsv"          /*GOOSE开入量发送*/
#define GOOSE_PUB_MEA "Pub_Mea"          /*GOOSE模拟量发送*/
#define SV_SUB "SV_SUB"                         /*采样接收*/
#define SV_PUB "Pub_SV"                         /*采样发送*/


/*CPU板号，对于外接线而言1表示主CPU;2表示从CPU*/
extern uint8_t g_ucCpuBoardId;

/*过程层中的接收GOOSE个数,去掉了站控层的GOOSE接收数目*/
extern uint16_t g_usProcessSubGooseNum;
/*过程层中的发送GOOSE个数,去掉了站控层的GOOSE发送数目*/
extern uint16_t g_usProcessPubGooseNum;
/*过程层中的接收SV个数,去掉了站控层的SV接收数目*/
extern uint16_t g_usProcessSubSvNum;
/*过程层中的发送SV个数,去掉了站控层的SV发送数目*/
extern uint16_t g_usProcessPubSvNum;

/*本板CPU的CPUID ,外接线只有1,2，根据SPI的主从来区分*/
extern uint8_t g_ucCpuId;

/*本CPU板DATAMAP中的需关联的开入开出个数*/
extern uint32_t g_ulGooseSubMapCnt;
extern uint32_t g_ulGoosePubMapCnt;


/*接收和发送的GOOSE结构，用于生成GSE文件*/
extern GSE_SUB_POOL g_tSubGoose;
extern GSE_PUB_POOL g_tPubGoose;

/*接收的SMV结构，用于生成SMV文件*/
extern IEC_SMV_CFG *g_pSmvCfg;

/*过程层中本插件的接收GOOSE个数,去掉了站控层的GOOSE接收数目*/
extern uint16_t g_usProcessSubGooseNumAll;
/*过程层中本插件的发送GOOSE个数,去掉了站控层的GOOSE发送数目*/
extern uint16_t g_usProcessPubGooseNumAll;
/*过程层中本插件的接收SV个数,去掉了站控层的SV接收数目*/
extern uint16_t g_usProcessSubSvNumAll;
/*过程层中本插件的发送SV个数,去掉了站控层的SV发送数目*/
extern uint16_t g_usProcessPubSvNumAll ;
/*接收和发送的GOOSE结构，用于生成GS文件*/
extern GSE_SUB_POOL g_tSubGooseAll;
extern GSE_PUB_POOL g_tPubGooseAll;
extern EDP_SADDR_STR *g_ptSubSAddr;    /*所有SUB GCB的SADDR*/
extern EDP_SADDR_STR *g_ptPubSAddr;    /*所有PUB GCB的SADDR*/

/*
描述: 解析过程层配置信息，内存数据结构转换
参数:     NONE
返回值: 解析是否成功
 */
extern int EDP_ConfigParseCPU();


/*
描述: 生成CPU侧配置文件
参数:     NONE
返回值: 解析是否成功
 */
extern int EDP_CreateCPUFile();

/*
获取枚举项条目字串
参数：    pcStr,枚举项字串
        lIndex,字串序号（0开始）
        pcItemStr,条目字串存储指针
        pKey,用于分割的字符
        usItemStrMaxLen,条目字符串最大长度
返回值：    条目字串长度
修改:  1.张全 20170111 增加参数传递pcItemStr指向数组的长度，防止操作中指针越界
*/
extern int GetEnumItem_Div(char *pcStr,int32_t lIndex,char *pcItemStr, char Key, uint16_t usItemStrMaxLen);

/*
获取枚举项条目字串,获取分割字符前面所有的字符串
参数：    pcStr,枚举项字串
        lIndex,字串序号（0开始）
        pcItemStr,条目字串存储指针
        pKey,用于分割的字符
        usItemStrMaxLen,条目字符串最大长度
返回值：    条目字串长度
修改:  1.张全 20170111 增加参数传递pcItemStr指向数组的长度，防止操作中指针越界
*/
extern int GetEnumItem_Div_All(char *pcStr,int32_t lIndex,char *pcItemStr, char Key, uint16_t usItemStrMaxLen);

/*
描述:获取SV主CC板Board的指针
参数:无
返回值:主CC板Board的指针
*/
extern BOARD_PORT_CONNECT *EDP_GetSvMasterCcPortConnectPtr();

/*
描述: 释放内存,CCD
参数:    NONE
返回值: 释放是否成功
 */
extern int EDP_FreeTempStruct();
#ifdef    __cplusplus
}
#endif

#endif

