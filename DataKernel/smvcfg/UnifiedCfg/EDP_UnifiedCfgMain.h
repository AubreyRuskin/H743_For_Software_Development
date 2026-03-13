/**************************************************************************
EDP_UnifiedCfgMain.h

九统一统一配置公共操作头文件

Copyright (c) 2015 SNAC(Guodian Nanjing Automation Co., Ltd.)
All Rights Reserved.


History:
Revision     Date          Who            Comment
-----------------------------------------------------------------
V1.00         2015.07.16    kevin         初始版本
***************************************************************************/

#ifndef EDP_UNIFIEDCFGMAIN_H
#define EDP_UNIFIEDCFGMAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "EDP_UnifiedCfgDefine.h"




/*
从字符串中得到BOARDID
参数：
            pcStr,字符串
返回值：    BOARDID
*/
extern int EDP_GetBoardId(char *pcStr);

/*
从字符串中得到PORTID ，从1开始
参数：
            pcStr,字符串
返回值：    BOARDID
*/
extern int EDP_GetPortId(char *pcStr);
#ifdef    __cplusplus
}
#endif

#endif