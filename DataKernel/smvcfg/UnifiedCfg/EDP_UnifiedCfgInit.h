/**************************************************************************
EDP_UnifiedCfgInit.h

九统一统一配置文件初始化操作头文件

Copyright (c) 2015 SNAC(Guodian Nanjing Automation Co., Ltd.)
All Rights Reserved.


History:
Revision     Date          Who            Comment
-----------------------------------------------------------------
V1.00         2015.07.16    kevin         初始版本
***************************************************************************/

#ifndef EDP_UNIFIEDCFGINIT_H
#define EDP_UNIFIEDCFGINIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "EDP_UnifiedCfgDefine.h"

/*
描述: 解析过程层配置文件
要放到IO板初始化的后面, RD_Initialize的后面
参数:
返回值: 解析是否成功
 */
extern int EDP_InitProcessFileCfg();


/* 升级前置CC板
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void PreUnitUpdate(void);
#ifdef    __cplusplus
}
#endif

#endif
