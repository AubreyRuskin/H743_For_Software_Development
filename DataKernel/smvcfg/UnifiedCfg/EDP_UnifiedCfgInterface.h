/**************************************************************************
EDP_UnifiedCfgInterface.h

九统一统一配置对外操作接口头文件

Copyright (c) 2015 SNAC(Guodian Nanjing Automation Co., Ltd.)
All Rights Reserved.


History:
Revision     Date          Who            Comment
-----------------------------------------------------------------
V1.00         2015.07.16    kevin         初始版本
***************************************************************************/

#ifndef EDP_UNIFIEDCFGINTERFACE_H
#define EDP_UNIFIEDCFGINTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "EDP_UnifiedCfgDefine.h"
#include "EDP_UnifiedCfgParseCCD.h"
#include "EDP_UnifiedCfgParseConfig.h"
#include "EDP_UnifiedCfgParsePrivate.h"
#include "EDP_UnifiedCfgFile.h"

/*
功能:检查CC板的运行状态
参数:无
返回:无
*/
extern void EDP_CheckCcStatus();

#ifdef    __cplusplus
}
#endif

#endif
