/**************************************************************************
EDP_UnifiedCfgParseCCD.h

九统一配置文件解析头文件

Copyright (c) 2015 SNAC(Guodian Nanjing Automation Co., Ltd.)
All Rights Reserved.


History:
Revision     Date          Who            Comment
-----------------------------------------------------------------
V1.00         2015.07.16    kevin         初始版本
***************************************************************************/

#ifndef EDP_UNIFIEDCFGDEPARSECCD_H
#define EDP_UNIFIEDCFGDEPARSECCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "EDP_UnifiedCfgDefine.h"


#define EDP_CCD_PARSE_ERR_FILE_NOT_EXIST 0x00000001     /*CCD文件不存在*/
#define EDP_CCD_PARSE_ERR_FILE_OPEN_FAIL 0x00000002     /*CCD文件打开失败*/
#define EDP_CCD_PARSE_ERR_FILE_XML_LOAD_FAIL 0x00000004     /*CCD文件XML加载错误*/
#define EDP_CCD_PARSE_ERR_FILE_CALLOC_FAIL 0x00000008     /*CCD 分配内存出错*/
#define EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL 0x00000010     /*CCD文件GOOSE字段XML解析错误*/
#define EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL 0x00000020     /*CCD文件SMV字段XML解析错误*/
#define EDP_CCD_PARSE_ERR_FILE_XML_CRC_PARSE_FAIL 0x00000040     /*CCD文件CRC字段XML解析错误*/
#define EDP_CCD_PARSE_ERR_FILE_CRC_NOT_EXIST 0x00000080     /*CCD 文件的CRC字段不存在*/


#define EDP_PRIVATE_PARSE_ERR_FILE_NOT_EXIST 0x00000100     /*PRIVATE文件不存在*/
#define EDP_PRIVATE_PARSE_ERR_FILE_OPEN_FAIL 0x00000200     /*PRIVATE文件打开失败*/
#define EDP_PRIVATE_PARSE_ERR_FILE_XML_LOAD_FAIL 0x00000400     /*PRIVATE文件XML加载错误*/
#define EDP_PRIVATE_PARSE_ERR_FILE_CALLOC_FAIL 0x00000800    /*PRIVATE 分配内存出错*/
#define EDP_PRIVATE_PARSE_ERR_FILE_XML_BOARD_PARSE_FAIL 0x00001000     /*PRIVATE文件BOARD 字段XML解析错误*/
#define EDP_PRIVATE_PARSE_ERR_FILE_XML_PORT_PARSE_FAIL 0x00002000     /*PRIVATE文件CONNECTPORT 字段XML解析错误*/
#define EDP_PRIVATE_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL 0x00004000     /*PRIVATE 文件SMV 字段XML解析错误*/
#define EDP_PRIVATE_PARSE_ERR_FILE_XML_GOOSE_PARSE_FAIL 0x00008000     /*PRIVATE 文件GOOSE 字段XML解析错误*/


#define EDP_CONFIG_PARSE_SUB_GOOSE_ERR 0x00010000     /*转换配置的SUB GOOSE失败*/
#define EDP_CONFIG_PARSE_PUB_GOOSE_ERR 0x00020000     /*转换配置的PUB GOOSE失败*/
#define EDP_CONFIG_PARSE_SUB_GOOSE_ERR1 0x00040000     /*PRIVATE文件XML加载错误*/
#define EDP_CONFIG_PARSE_SUB_GOOSE_ERR2 0x00080000    /*PRIVATE 分配内存出错*/
#define EDP_CONFIG_PARSE_SUB_SMV_ERR 0x00100000     /*转换配置的SUB SMV失败*/
#define EDP_CONFIG_PARSE_PUB_SMV_ERR 0x00200000     /*转换配置的PUB SMV失败*/
#define EDP_CONFIG_PARSE_SUB_GOOSE_ERR3 0x00400000     /*PRIVATE 文件SMV 字段XML解析错误*/
#define EDP_CONFIG_PARSE_SUB_GOOSE_ERR4 0x00800000     /*PRIVATE 文件GOOSE 字段XML解析错误*/

#define EDP_CONFIG_CRC_CHECK_ERR 0x01000000    /* CCD文件CRC校验出错 */
#define EDP_CONFIG_NECESSARY_PARA_LOST 0x02000000 /* CCD文件中必需参数丢失 */
#define EDP_CONFIG_PARA_OVERFLOW 0x04000000       /* CCD文件中参数值越限 */
#define EDP_CONFIG_DEV_INFO_CHECK_ERR 0x08000000 /* CCD文件中装置信息出错 */
#define EDP_CRC_FILE_CREAT_ERROR 0x00000001  /* CCD文件相应的CRC校验文件创建 */
#define EDP_CRC_BAK_FILE_SAVE_ERROR 0x00000002  /* CCD文件相应的CRC校验文件生成失败 */
#define EDP_CRC_FILE_READ_ERROR 0x00000003  /* 读取CRC校验文件失败 */

#define EDP_CCD_INTADDR_INVALID_NAME "NULL"     /*无效的intaddr名称*/
#define EDP_CCD_DAI_NAME "DAI"      /*DAI元素名称*/

/*
描述: 解析CCD文件的入口函数.
参数:     CCD文件的指针.
返回值: 解析是否成功
 */
extern int EDP_LoadCCDFile(char *pFileName);


/*
描述: 获取process.xml中的pub goose指针
参数:     无
返回值: process.xml中的pub goose指针
 */
extern PROCESS_PUB_GSE *EDP_GetProcessPubGoose();

/*
描述: 获取process.xml中的sub goose指针
参数:     无
返回值: process.xml中的sub goose指针
 */
extern PROCESS_SUB_GSE *EDP_GetProcessSubGoose();

/*
描述: 获取process.xml中的pub sm指针
参数:     无
返回值: process.xml中的pub sm指针
 */
extern PROCESS_PUB_SMV *EDP_GetProcessPubSmv();

/*
描述: 获取process.xml中的sub sm指针
参数:     无
返回值: process.xml中的sub sm指针
 */
extern PROCESS_SUB_SMV *EDP_GetProcessSubSmv();


/*
描述: 获取process.xml中的private指针
参数:     无
返回值: process.xml中的private 指针
 */
extern PROCESS_PRIVATE *EDP_GetProcessCrc();

/*
描述: 获取process.xml的指针
参数:     无
返回值: process.xml 指针
 */
extern PROCESS_CFG *EDP_GetProcessPtr();

/*
描述:获取GSE PUB的个数
参数:     无
返回值: GSE PUB的个数
 */
extern int EDP_GetPubGooseCnt();

/*
描述:获取GSE SUB的个数
参数:     无
返回值: GSE SUB的个数
 */
extern int EDP_GetSubGooseCnt();

/*
描述:获取SMV PUB的个数
参数:     无
返回值: SMV PUB的个数
 */
extern int EDP_GetPubSmvCnt();

/*
描述:获取SMV SUB的个数
参数:     无
返回值: SMV SUB的个数
 */
extern int EDP_GetSubSmvCnt();


/*
描述:获取IED名称
参数:     无
返回值: IED名称
 */
extern char * EDP_GetIEDName();

/*
描述: 释放内存,CCD
参数:    NONE
返回值: 释放是否成功
 */
extern int EDP_FreeCCD();
#ifdef    __cplusplus
}
#endif

#endif
