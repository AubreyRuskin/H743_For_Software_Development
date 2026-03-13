/**************************************************************************
EDP_UnifiedCfgInterface.c

九统一统一配置对外操作接口头文件

Copyright (c) 2015 SNAC(Guodian Nanjing Automation Co., Ltd.)
All Rights Reserved.


History:
Revision     Date          Who            Comment
-----------------------------------------------------------------
V1.00         2015.07.16    kevin         初始版本
***************************************************************************/

#include "EDP_UnifiedCfgInterface.h"
#include "errtest.h"
#include "edp_asst.h"

extern FPGA_CC_CFG_INFO g_tFpgaCcCfgInfo[MAX_CC_BOARD_ID_NUM]; /* 以BoardId为序号, 表示CC板的运行配置状态*/
extern BOOL g_bCcIsUsed[MAX_CC_BOARD_ID_NUM]; /*以BoardId为序号，表示当前CC是否使用*/
extern T_CC_STS arrCcPortSts[MAX_CC_BOARD_NUM];  /* CC端口状态 */
extern uint8_t g_ucCCSnToArr[MAX_CC_BOARD_ID_NUM];
extern BOOL g_bAlertLightOn; /* 平台是否呼唤 */

/*
描述:获取CCD文件的CRC.
暂时直接将文件中的CRC传给HMI，后续进行计算，并校验
给出实际的CRC，如不一致要闭所保护。
*/
char* EDP_GetCcdCreatTime()
{
    PROCESS_PRIVATE *pProcessPrivate = NULL;
    pProcessPrivate = EDP_GetProcessCrc();
    return pProcessPrivate->pTimeStamp;
}

/*
功能:检查CC板的运行状态
参数:无
返回:无
*/
void EDP_CheckCcStatus()
{
    int i= 0;
    char TempInfo[256];
    BOOL bStatus = TRUE;

    static BOOL bChecked[MAX_CC_BOARD_ID_NUM];

    for(i = 0; i < MAX_CC_BOARD_ID_NUM; i++)
    {
        if((g_bCcIsUsed[i]) && (g_ucCCSnToArr[i] != 0) && (!bChecked[i]))
        {
            bChecked[i] = TRUE;

            if(PORT_TRANSFOR_DATA_TYPE_SV == g_tFpgaCcCfgInfo[i].iDataType)
            {
                if(g_tFpgaCcCfgInfo[i].usFpgaCcSvCfgCrc != arrCcPortSts[g_ucCCSnToArr[i]].usSvCfgCrc)
                {
                    sprintf(TempInfo, "BoardId为%d的CC板SV配置(CRC:0x%04x)与CPU解析的配置(CRC:0x%04x)不一致\n"
                            , i, arrCcPortSts[g_ucCCSnToArr[i]].usSvCfgCrc, g_tFpgaCcCfgInfo[i].usFpgaCcSvCfgCrc);
                    LOG_Dbg_Msg(TempInfo,0,0,0,0,0,0);
                    LOG_Write(LOG_RUN, TempInfo, NULL);
                    bStatus = FALSE;
                }
            }
            else if(PORT_TRANSFOR_DATA_TYPE_GS == g_tFpgaCcCfgInfo[i].iDataType)
            {

                if(g_tFpgaCcCfgInfo[i].usFpgaCcGsCfgCrc != arrCcPortSts[g_ucCCSnToArr[i]].usGsCfgCrc)
                {
                    sprintf(TempInfo, "BoardId为%d的CC板GS配置(CRC:0x%04x)与CPU解析的配置(CRC:0x%04x)不一致\n"
                            , i, arrCcPortSts[g_ucCCSnToArr[i]].usGsCfgCrc, g_tFpgaCcCfgInfo[i].usFpgaCcGsCfgCrc);
                    LOG_Dbg_Msg(TempInfo,0,0,0,0,0,0);
                    LOG_Write(LOG_RUN, TempInfo, NULL);
                    bStatus = FALSE;
                }
            }
            else
            {
                if(g_tFpgaCcCfgInfo[i].usFpgaCcSvCfgCrc != arrCcPortSts[g_ucCCSnToArr[i]].usSvCfgCrc)
                {
                    sprintf(TempInfo, "BoardId为%d的CC板SV配置(CRC:0x%04x)与CPU解析的配置(CRC:0x%04x)不一致\n"
                            , i, arrCcPortSts[g_ucCCSnToArr[i]].usSvCfgCrc, g_tFpgaCcCfgInfo[i].usFpgaCcSvCfgCrc);
                    LOG_Dbg_Msg(TempInfo,0,0,0,0,0,0);
                    LOG_Write(LOG_RUN, TempInfo, NULL);
                    bStatus = FALSE;
                }

                if(g_tFpgaCcCfgInfo[i].usFpgaCcGsCfgCrc != arrCcPortSts[g_ucCCSnToArr[i]].usGsCfgCrc)
                {
                    sprintf(TempInfo, "BoardId为%d的CC板GS配置(CRC:0x%04x)与CPU解析的配置(CRC:0x%04x)不一致\n"
                            , i, arrCcPortSts[g_ucCCSnToArr[i]].usGsCfgCrc, g_tFpgaCcCfgInfo[i].usFpgaCcGsCfgCrc);
                    LOG_Dbg_Msg(TempInfo,0,0,0,0,0,0);
                    LOG_Write(LOG_RUN, TempInfo, NULL);
                    bStatus = FALSE;
                }
            }
        }
    }

    if(!bStatus)
    {
        /* 只要在装置上电时报出这个错误 */
        g_bAlertLightOn = TRUE;
        ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                   ER_REPORT|ER_ALARM|ER_LOCK,
                   "配置CC板失败，请重启装置", 0, 0);
    }

    return;
}
