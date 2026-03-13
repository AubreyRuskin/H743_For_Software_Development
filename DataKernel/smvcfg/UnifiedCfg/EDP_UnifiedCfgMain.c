/**************************************************************************
EDP_UnifiedCfgMain.c

九统一统一配置公共操作头文件

Copyright (c) 2015 SNAC(Guodian Nanjing Automation Co., Ltd.)
All Rights Reserved.


History:
Revision     Date          Who            Comment
-----------------------------------------------------------------
V1.00         2015.07.16    kevin         初始版本
***************************************************************************/

#include "EDP_UnifiedCfgMain.h"
#include "EDP_UnifiedCfgParseConfig.h"



/*
从字符串中得到BOARDID
参数：
            pcStr,字符串
返回值：    BOARDID
*/
int EDP_GetBoardId(char *pcStr)
{
    char Tmp[4];
    int res = 0;

    if(pcStr == NULL)
        return -1;
    /*解析出单独的板号和端口*/
    GetEnumItem_Div(pcStr,0,Tmp,'-',sizeof(Tmp));

    if(Tmp[0] == '\0')
        return -1;
    else
    {
        res = strtol(Tmp,NULL,10);
    }
    return res;
}

/*
从字符串中得到PORTID ，从1开始
参数：
            pcStr,字符串
返回值：    BOARDID
*/
int EDP_GetPortId(char *pcStr)
{
    char p[4];
    int res = 0;

    if(pcStr == NULL)
        return -1;
    /*解析出单独的板号和端口*/
    GetEnumItem_Div(pcStr,1,p,'-',sizeof(p));

    if(p[0] == '\0')
        return -1;
    else
    {
        if(((int)p[0] >= (int)'a') && ((int)p[0] <= (int)'z') )
        {
            res = (int)(p[0] - 'a' );
        }
        else if(((int)p[0] >= (int)'A') && ((int)p[0] <= (int)'Z' ))
        {
            res = (int)(p[0] - 'A' );
        }
        else
        {
            res = -1;
        }
    }
    return res;
}


