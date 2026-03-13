/**************************************************************************
EDP_UnifiedCfgInit.c

九统一统一配置文件初始化操作头文件

Copyright (c) 2015 SNAC(Guodian Nanjing Automation Co., Ltd.)
All Rights Reserved.


History:
Revision     Date          Who            Comment
-----------------------------------------------------------------
V1.00         2015.07.16    kevin         初始版本
***************************************************************************/


#include "EDP_UnifiedCfgInit.h"
#include "EDP_UnifiedCfgParseCCD.h"
#include "EDP_UnifiedCfgParsePrivate.h"
#include "EDP_UnifiedCfgParseConfig.h"
#include "EDP_UnifiedCfgFile.h"
#include "filetool.h"
#include <taskLib.h>
#include <memLib.h>
#include <stdio_compat.h>
#include "edp_asst.h"

uint16_t g_usCcdFileCrc = 0;        /*装置的当前CCD文件的CRC*/
uint16_t g_usPrivateFileCrc = 0;        /*装置的当前DEVCFG文件的CRC*/
uint16_t g_usCcdFileCrcLast = 0;        /*装置保存的CCD文件的CRC*/
uint16_t g_usPrivateFileCrcLast = 0;        /*装置保存的DEVCFG文件的CRC*/

/*过程层统一配置文件的CRC是否改变,如果改变则全部重新解析
未改变则不需要解析CCD，只解析devcfg即可*/
BOOL g_bIsProcessFileChanged = TRUE;
/* CCD解析异常标志 */
BOOL g_bCcdCrcErr = FALSE;          /* CCD文件CRC校验出错 */
BOOL g_bCcdFileErr = FALSE;         /* CCD文件内容错误 */

extern uint32_t g_ulCcdFileCheckCrc; /* 通过计算得出的CCD文件校验码 */
extern BOOL g_bCcIsUsed[MAX_CC_BOARD_ID_NUM]; /*以BoardId为序号，表示当前CC是否使用*/
extern BOOL g_bAlertLightOn; /* 平台是否呼唤 */



 void PreUnitUpdate(void){
    return ;
 }
/*
描述: 初始化公共信息函数
参数:     NONE
返回值: 解析是否成功
 */
int EDP_InitProcessCommInfo()
{
    if(isNumber_2_04CPU() || (ucCPUSeq_g == 1))
    {
        g_ucCpuId = 2;  /*1是主CPU,2是从CPU*/
    }
    else
    {
        g_ucCpuId = 1;  /*1是主CPU,2是从CPU*/
    }
    return 0;
}

/*
描述: 保存CCD文件和DEVCFG文件的CRC
参数:     NONE
返回值:无变动返回0，有变动返回1
 */
int EDP_SaveCrc(uint16_t usCcdCrc, uint16_t usPrivateCrc,uint32_t ulCcdCheckCrc)
{
    FILE *fp;
    uint32_t ulTmp = 0;
    char ucTemp[32] = {0};

    ulTmp = U16_TO_U32(usCcdCrc, usPrivateCrc);

    fp = fopen(CRC_FILE, "w");
    if(fp==NULL)
    {
        return 1;
    }

    strcpy(ucTemp, EDP_GetProcessCrc()->pTimeStamp);

    fwrite((char *)(&ulTmp),4,1,fp);/*读取字节*/

    fwrite((char*)(&ulCcdCheckCrc),4,1,fp);

    fwrite(ucTemp,32,1,fp);

    fwrite((char*)(g_bCcIsUsed),sizeof(BOOL)*MAX_CC_BOARD_ID_NUM,1,fp);

    ulTmp = U16_TO_U32(0, FT_File_CRC16(EP_FUNCOPTION_FILE));

    fwrite((char *)(&ulTmp),4,1,fp);

    fclose(fp);

    return 0;
}

/*
描述: 检查CCD文件和DEVCFG文件CRC是否有变动
参数:     NONE
返回值:无变动返回0，有变动返回1
 */
int EDP_CheckCrc()
{
    FILE *fp = NULL;
    uint32_t ulTmp = 0;
    int res = 0;
    int iLen = 0;
    char ucTemp[32] = {0};

    if (!FT_Is_File(CCD_FILE) || !FT_Is_File(PRIVATECFG_FILE))
    {
        return 0;
    }

    g_usCcdFileCrc = FT_File_CRC16(CCD_FILE);
    g_usPrivateFileCrc = FT_File_CRC16(PRIVATECFG_FILE);

    if (!FT_Is_File(CRC_FILE))
    {
        res = 1;
        goto exit;
    }

    fp = fopen(CRC_FILE, "r");
    if(fp==NULL)
    {
        res = 1;
        goto exit;
    }
    fread((char *)(&ulTmp),4,1,fp);/*读取字节*/
    g_usCcdFileCrcLast = HI16(ulTmp);
    g_usPrivateFileCrcLast = LO16(ulTmp);

    if((g_usCcdFileCrcLast == g_usCcdFileCrc)
            && (g_usPrivateFileCrcLast == g_usPrivateFileCrc))
    {

        iLen = fread((char *)(&ulTmp),4,1,fp);
        if(iLen == 1)
        {
            g_ulCcdFileCheckCrc = ulTmp;
        }
        else
        {
            res = 1;
            goto exit;
        }

        iLen = fread(ucTemp, 32, 1, fp);
        if(iLen != 1)
        {
            res = 1;
            goto exit;
        }
        else
        {
            EDP_GetProcessCrc()->pTimeStamp = strdup(ucTemp);
        }

        iLen = fread((char *)(g_bCcIsUsed),sizeof(BOOL)*MAX_CC_BOARD_ID_NUM,1,fp);
        if(iLen != 1)
        {
            res = 1;
            goto exit;
        }

        iLen = fread((char *)(&ulTmp),4,1,fp);
        if(iLen == 1)
        {
            if(FT_File_CRC16(EP_FUNCOPTION_FILE) != LO16(ulTmp))
            {
                res = 1;
                goto exit;
            }
        }
        else
        {
            res = 1;
            goto exit;
        }
    }
    else
    {
        res = 1;
    }

exit:
    if(NULL != fp)
    {
        fclose(fp);
    }
    return res;
}


/*
描述: 释放内存,CCD, DEVCFG, FILE, 中间结构等
参数:    NONE
返回值: 释放是否成功
 */
int EDP_FreeMem()
{
    EP_STATUS res = EP_SUCCESS;
    if(EDP_FreeCCD() != EP_SUCCESS)
    {
        logMsg("#######  EDP_FreeCCD执行出错\n",0,0,0,0,0,0);
        res = EP_ERROR;
    }
    if(EDP_FreeDevCfg() != EP_SUCCESS)
    {
        logMsg("#######  EDP_FreeDevCfg执行出错\n",0,0,0,0,0,0);
        res = EP_ERROR;
    }
    if(EDP_FreeTempStruct() != EP_SUCCESS)
    {
        logMsg("#######  EDP_FreeTempStruct执行出错\n",0,0,0,0,0,0);
        res = EP_ERROR;
    }
    if(EDP_FreeFile() != EP_SUCCESS)
    {
        logMsg("#######  EDP_FreeFile执行出错\n",0,0,0,0,0,0);
        res = EP_ERROR;
    }

    return res;
}

/*增加CCD文件  CRC判断*/
/*
描述: 解析过程层配置文件
要放到IO板初始化的后面, RD_Initialize的后面
参数:
返回值: 解析是否成功
 */
int EDP_InitProcessFileCfg()
{
    int res = 0;
    /*memShow(0);*/
    EDP_InitProcessCommInfo();

    res = EDP_CheckCrc();
    if(res == 0)
    {
        CFG_LOG("------>kevin:  CRC一致，无需重新解析，直接进行后续初始化ccd: %04x. private: %04x.\n",
                g_usCcdFileCrc,g_usPrivateFileCrc,0,0,0,0);
        g_bIsProcessFileChanged = FALSE;
    }
    else
    {
        CFG_LOG("------>kevin: CRC不一致，重新解析.ccd: %04x, %04x. private: %04x, %04x.\n",
                g_usCcdFileCrc,g_usCcdFileCrcLast,g_usPrivateFileCrc,g_usPrivateFileCrcLast,0,0);
        g_bIsProcessFileChanged = TRUE;
    }

    /*如果未更新，则只解析DEVCFG得到CC板转发关系，并给CC下载程序*/
    if(!g_bIsProcessFileChanged)
    {
        if(g_ucCpuId == 1)
        {
            res = EDP_LoadPrivateFile(PRIVATECFG_FILE);
            if(res != 0)
            {
                CFG_LOG("------>私有文件解析失败，错误码:%08x\n",res,0,0,0,0,0);
                goto exit;
            }
            else
            {
                CFG_LOG("------>私有文件解析成功\n",0,0,0,0,0,0);
            }

            res = EDP_CreateCCTransStruct();
            if(res != 0)
            {
                CFG_LOG("------>过程层配置转换CC转发关系失败，错误码:%08x\n",res,0,0,0,0,0);
                goto exit;
            }
            else
            {
                CFG_LOG("------>过程层配置转换CC转发关系成功\n",0,0,0,0,0,0);
            }

            res = EDP_InitCcStrToSendFile();
            if(res != 0)
            {
                CFG_LOG("------>过程层配置转换CC发送文件的结构失败，错误码:%08x\n",res,0,0,0,0,0);
                goto exit;
            }
            else
            {
                CFG_LOG("------>过程层配置转换CC发送文件的结构成功\n",0,0,0,0,0,0);
            }

            /*升级CC程序*/
            PreUnitUpdate();
        }
        goto exit;
    }

    res = EDP_LoadCCDFile(CCD_FILE);
    if(res != 0)
    {
        CFG_LOG("------> CCD文件解析失败，错误码:%08x\n",res,0,0,0,0,0);

        if((res&EDP_CONFIG_CRC_CHECK_ERR) != 0)
        {
            g_bCcdCrcErr = TRUE;
        }
        else if(((res&EDP_CONFIG_NECESSARY_PARA_LOST) != 0) ||
                ((res&EDP_CONFIG_PARA_OVERFLOW) != 0) ||
                ((res&EDP_CONFIG_DEV_INFO_CHECK_ERR) != 0) ||
                ((res&EDP_CCD_PARSE_ERR_FILE_XML_LOAD_FAIL) != 0))
        {
            g_bCcdFileErr = TRUE;
        }

        if(g_bCcdCrcErr||g_bCcdFileErr)
        {
            g_bAlertLightOn = TRUE;
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,
                       ER_REPORT|ER_ALARM|ER_LOCK,
                       "CCD文件错误\n", 0, 0);
            goto exit;
        }
    }
    else
    {
        /*记录配置文件CRC与CCD计算出的CRC*/
        EDP_SaveCrc(g_usCcdFileCrc,g_usPrivateFileCrc,g_ulCcdFileCheckCrc);
        CFG_LOG("------> CCD文件解析成功\n",0,0,0,0,0,0);
    }

    res = EDP_LoadPrivateFile(PRIVATECFG_FILE);
    if(res != 0)
    {
        CFG_LOG("------>私有文件解析失败，错误码:%08x\n",res,0,0,0,0,0);
    }
    else
    {
        CFG_LOG("------>私有文件解析成功\n",0,0,0,0,0,0);
    }

    res = EDP_ConfigParseCPU();
    if(res != 0)
    {
        CFG_LOG("------>过程层配置转换CPU数据结构失败，错误码:%08x\n",res,0,0,0,0,0);
    }
    else
    {
        CFG_LOG("------>过程层配置转换CPU数据结构成功\n",0,0,0,0,0,0);
    }

    /*如果成功解析CCD文件则删除smv与gse文件，确保以前装置生成的文件不会影响现在的配置*/
    if(FT_Is_File(CONFIG_FILE))
    {
        remove(CONFIG_FILE);
    }

    if(FT_Is_File(CONFIG_SMV_FILE))
    {
        remove(CONFIG_SMV_FILE);
    }

    res = EDP_CreateCPUFile();
    if(res != 0)
    {
        CFG_LOG("------>过程层配置生成CPU配置文件失败，错误码:%08x\n",res,0,0,0,0,0);
        goto exit;
    }

    /*只有主CPU才需生成CC文件并发送*/
    if(g_ucCpuId == 1)
    {
        res = EDP_ConfigParseCC();
        if(res != 0)
        {
            CFG_LOG("------>过程层配置转换CC数据结构失败，错误码:%08x\n",res,0,0,0,0,0);
            goto exit;
        }
        else
        {
            CFG_LOG("------>过程层配置转换CC数据结构成功\n",0,0,0,0,0,0);
        }

        res = EDP_CreateCCTransStruct();
        if(res != 0)
        {
            CFG_LOG("------>过程层配置转换CC转发关系失败，错误码:%08x\n",res,0,0,0,0,0);
            goto exit;
        }
        else
        {
            CFG_LOG("------>过程层配置转换CC转发关系成功\n",0,0,0,0,0,0);
        }

        res = EDP_CreateCCFile();
        if(res != 0)
        {
            CFG_LOG("------>过程层配置转换CC文件失败，错误码:%08x\n",res,0,0,0,0,0);
            goto exit;
        }
        else
        {
            CFG_LOG("------>过程层配置转换CC文件成功\n",0,0,0,0,0,0);
        }

        res = EDP_InitCcStrToSendFile();
        if(res != 0)
        {
            CFG_LOG("------>过程层配置转换CC发送文件的结构失败，错误码:%08x\n",res,0,0,0,0,0);
            goto exit;
        }
        else
        {
            CFG_LOG("------>过程层配置转换CC发送文件的结构成功\n",0,0,0,0,0,0);
        }

        /*升级CC程序*/
        PreUnitUpdate();
    }

exit:

    if(res == 0)
    {
        if(EDP_FreeMem() != EP_SUCCESS)
        {
            logMsg("####### EDP_FreeMem 释放内存出错\n",0,0,0,0,0,0);
        }
    }

    /*memShow(0);*/
    return res;
}


BOOL taskccd()
{
    int res = 0;

    EDP_InitProcessCommInfo();

    res = EDP_LoadCCDFile(CCD_FILE);
    if(res != 0)
    {
        CFG_LOG("------> CCD文件解析失败，错误码:%08x\n",res,0,0,0,0,0);
    }
    return TRUE;
}
BOOL taskpri()
{
    int res = 0;

    EDP_InitProcessCommInfo();

    res = EDP_LoadPrivateFile(PRIVATECFG_FILE);
    if(res != 0)
    {
        CFG_LOG("------>私有文件解析失败，错误码:%08x\n",res,0,0,0,0,0);
    }
    return TRUE;
}

BOOL xiangccd()
{
    int taskid = 0;
    taskid = taskSpawn("tXiang1",
                       TSK_PRI_DSP,
                       VX_FP_TASK|VX_DEALLOC_STACK,
                       500000,		/* 堆栈由200改为5，以下同*/
                       (FUNCPTR)taskccd,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    return TRUE;
}
BOOL xiangpri()
{
    int taskid = 0;
    taskid = taskSpawn("tXiang3",
                       TSK_PRI_DSP,
                       VX_FP_TASK|VX_DEALLOC_STACK,
                       500000,		/* 堆栈由200改为5，以下同*/
                       (FUNCPTR)taskpri,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    return TRUE;
}

