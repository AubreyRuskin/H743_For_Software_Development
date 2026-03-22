/* platformtest.c - subroutine library for testing the hardware platform */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 22mar07, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for testing the hardware platform.
INCLUDES: platformtest.h
*/

/* includes */

#include "platformtest.h"
#include "hwcfg.h"
#include "dspai.h"
#include "realdata.h"
#include "errtest.h"
#include "dspai.h"
#include "ext_box.h"
#if defined(EDP03_BUILD) || defined(EXCITE_BUILD)
#include "io_Drv.h"
#endif
#include "datetime.h"
#include "logmsg.h"
#include "miscfunc.h"
#include "view.h"
#include "filetool.h"
#include "sysinfo.h"
#include "swcfg.h"

#include <stdio_compat.h>
#include "string_compat.h"
#include "ctype_compat.h"
#include <ioLib.h>
#include <intLib.h>
#include <taskLib.h>
#include <semLib.h>
#include "dsp.h"

#include "filetool.h"
#include "string_compat.h"
#include "ctype_compat.h"
#include <stdio_compat.h>
#include <dirent_compat.h>
#include <sys/stat.h>
#include <ioLib.h>
#include <lstLib.h>
#include <semLib.h>
#include <dosFsLib.h>
#include "protectmmiinterface.h"

#include "vxWorks.h"
/* #include "m8260Cp.h" */
#include "logLib.h"
// #include "sbcm8260Siu.h"
// #include "m8260IntrCtl.h"

#include "excLib.h"
// #include "arch\ppc\esfPpc.h"
#include "tickLib.h"
#include "edp_asst.h"
#include "bspinterface.h"

/* defines */

#define EXC_HOOK_DELAY_TICKS 6000
#define US_ADC_INT_MIN_INIT_VAL 208
#define US_ADC_INT_MAX_INIT_VAL 1302

/* typedefs */

// void ppcExcHandler
// (
//     int      task,    				/* ID of offending task */
//     int      vecNum,  						/* exception vector number */
//     ESFPPC  *pEsf    				/* pointer to exception stack frame */
// );

// void excHandler(ESFPPC *pEsf);

/* globals */

char usbcopySrcDir[128]= {"/u0/test"};			/* 当前传输的目的地址，目录 */
char usbcopySrcFileName[10][128];		/* 当前需传输的源文件 */
char norcopySrcDir[128]= {"/data"};			/* 当前传输的目的地址，目录 */
unsigned long usMinGap = US_ADC_INT_MIN_INIT_VAL;
unsigned long usMaxGap = 0;

unsigned short usIntMinGap = US_ADC_INT_MAX_INIT_VAL;
unsigned short usIntMaxGap = 0;

BOOL beginSaveUsGet=TRUE;

/* functions */

void    regExcHandle()
{
//     excConnect((VOIDFUNCPTR*) 0x700, (VOIDFUNCPTR) excHandler);		/* 程序执行异常 */
//     excConnect((VOIDFUNCPTR*) 0x800, (VOIDFUNCPTR) excHandler);				/* 浮点异常 */
}

void    testFloatErr()
{
    volatile int a;
    volatile float b = 1e50;

    a = (int)b;
}

// void excHandler(ESFPPC *pEsf)
// {
//     unsigned int pcValue;
//     int		vecNum = pEsf->vecOffset;	/* exception vector number */
//     char *pTaskName = NULL;
//     char taskNameAry[] = "Unknown task";

//     pcValue = pEsf->regSet.pc;
//     if(!INT_CONTEXT ())
//     {
//         pTaskName = taskName((int)taskIdCurrent);
//         if(NULL == pTaskName)
//         {
//             pTaskName = taskNameAry;
//         }
//     }

//     while(1)
//     {
//         /*while中的内容可以替换成自己需要的程序,比如写一个文件记录下出错的PC值
//             记录下PC值后,可以通过几种方法找到出错的函数
//             1.  在shell(可以是超级终端的tShell,或者Tornado中的windSh)中输入lkAddr PC值,
//                 列出出错地址附近的函数名称.
//                 对于非静态函数可以很快定位到出错的函数.
//                 [例] lkAddr 0x123456
//             2.  在PC机上,编译好带调试信息(编译时使用-g选项)的Vxworks后,
//                 可以通过addr2lineppc -e vxworks [地址]
//                 找到某地址所在的文件名和行号.如果出错的地址是在一个库当中,
//                 要想使用addr2lineppc找文件名和行号,要求库编译时也要带调试信息.
//         */
//         {
//             if(pTaskName)
//             {
//                 logMsg("%s error, Exception number is %d, PC is 0x%8.8x\n", (int)pTaskName, vecNum, pcValue,0,0,0);
//             }
//             else
//             {
//                 logMsg("Interrupt error, Exception number is %d, PC is 0x%8.8x\n", (int)pTaskName, vecNum, pcValue,0,0,0);
//             }
//         }
//         taskDelay(EXC_HOOK_DELAY_TICKS);
//     }
// }

/***********************************************************************
* FT_Temp_Name_New_Test - 临时函数(防止修改filetool.c)，用于测试
*
* RETURNS: 无
*
*/
void FT_Temp_Name_New_Test(
    uint8_t *pucRslt,
    const uint8_t *strFile
)
{
    uint8_t *puc;

    assert(pucRslt);
    assert(strFile && strlen(strFile)<FULL_NAME_LEN);

    strncpy(pucRslt, strFile, FULL_NAME_LEN);
    pucRslt[FULL_NAME_LEN]='\0';

    /* Let's add a '#' just after the last '/'. */
    for (puc=pucRslt+strlen(pucRslt)-1; puc>=pucRslt; puc--)
    {
        if (*puc=='/')
            break;
    }
    assert(puc>=pucRslt);

    memmove(puc+2, puc+1, strlen(puc)+1);
    puc[1]='#';
}

/* Prepair to update/create a file.  (Using temp file to keep data integrity.)
 * Parameters:
 *      strFile, name of file to be updated.
 * Return:
 *      >=0, file descriptor. For using read/write function.
 *      EP_PARM_ERR, the file does not exist.
 *      EP_FILE_ERR, create temp file failure. */
EP_STATUS FT_Bgn_Update_Test(const uint8_t *strFile)
{
    uint8_t aucTempFile[FULL_NAME_LEN+2];   /* Keep one space to insert '#'. */
    int i;

    assert(strFile && strlen(strFile)<FULL_NAME_LEN);

    FT_Temp_Name_New_Test(aucTempFile, strFile);
    /* TODO: what happens with creat when file already exists?
     * We want to open it on this suituation. */
    i=creat(aucTempFile, O_RDWR);
    if (i!=ERROR)
        return i;
    else
        return EP_FILE_ERR;
}

/* Finish updating/creating file.  Temp file is cleaned normally.
 * Parameters:
 *      strFile, name of file to be updated.
 *      iFd, file descriptor returned by FT_Bgn_Update.
 * Return:
 *      EP_SUCCESS, update OK.
 *      EP_ERROR, operating failure. */
EP_STATUS FT_End_Update_Test(const uint8_t *strFile, int iFd)
{
    uint8_t aucTempFile[FULL_NAME_LEN+2];   /* Keep one space to insert '#'. */
    STATUS vxsts;

    assert(iFd>=0);

    vxsts=close(iFd);
    assert(vxsts!=ERROR);

    FT_Temp_Name_New_Test(aucTempFile, strFile);

    if (FT_Is_File(strFile))
    {
        vxsts=remove(strFile);
        assert(vxsts==OK);
    }

    vxsts=rename(aucTempFile, strFile);
    assert(vxsts==OK);

    return EP_SUCCESS;
}

/***********************************************************************
* FileWrInit - 文件写初始化
*
* RETURNS: 无
*
*/
EP_STATUS FileWrInit(void)
{
    uint8_t aucTempFile[FULL_NAME_LEN+1];
    int i;
    int iFd;
    static uint8_t aucDftIni[]= {"PoWrInit\n"};
    EP_STATUS sts;

    sts = EP_SUCCESS;

    FT_Temp_Name_New_Test(aucTempFile, PLATFORM_TEST_FILE);
    if (!FT_Is_File(PLATFORM_TEST_FILE) && !FT_Is_File(aucTempFile))
    {

        iFd=FT_Bgn_Update_Test(PLATFORM_TEST_FILE);
        assert(iFd>=0);

        i=write(iFd, aucDftIni, sizeof(aucDftIni));
        assert(i==sizeof(aucDftIni));

        sts=FT_End_Update_Test(PLATFORM_TEST_FILE, iFd);
        assert(sts==EP_SUCCESS);
    }

    return sts;
}

/***********************************************************************
* FileWrFile - 文件写
*
* RETURNS: 无
*
*/
EP_STATUS FileWrFile(void)
{
    int i;
    int iFd;
    static uint8_t aucDftIni[20];
    EP_STATUS sts;
    RD_LGC_PO_CH *plgcpo;

    sts = EP_SUCCESS;

    /* FT_Temp_Name_New_Test(aucTempFile, PLATFORM_TEST_FILE); */

    iFd=FT_Bgn_Update_Test(PLATFORM_TEST_FILE);
    assert(iFd>=0);

    for(plgcpo = plgcpoch_g; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++)
    {
        /* i=sprintf(aucDftIni, "%16.10f", plgcpo->TotalEnergy); */
        i=sprintf(aucDftIni, "%16.10f", plgcpo->Val.TotalEnergy);
        i=write(iFd, aucDftIni, sizeof(aucDftIni));
        assert(i==sizeof(aucDftIni));
    }

    sts=FT_End_Update_Test(PLATFORM_TEST_FILE, iFd);
    assert(sts==EP_SUCCESS);

    if(sts == EP_SUCCESS)
    {
        LOG_Dbg_Msg("FileWrFile OK!\n", 0, 0, 0, 0, 0, 0);
    }

    return sts;
}

/***********************************************************************
* FileRdFile - 从文件读
*
* RETURNS: 无
*
*/
EP_STATUS FileRdFile(void)
{
    int lFile, i;
    EP_STATUS retcode= EP_SUCCESS;
    uint8_t *pucBuf;
    uint8_t *pDest;
    int RdNum;
    RD_LGC_PO_CH *plgcpo;
    int RdLength;

    RdLength = 20*iLgcPoChNum_g;
    lFile = open((char *)PLATFORM_TEST_FILE,O_RDONLY,0);
    if(lFile<=0)
    {
        retcode = EP_ERROR;
        LOG_Dbg_Msg("Open Error!\n",0,0,0,0,0,0);
        return retcode;
    }

    if ((pucBuf=malloc(RdLength))!=NULL)
    {
        if((RdNum = read(lFile,(char*)pucBuf,RdLength)) != RdLength)
        {
            /* 关闭已打开文件 */
            if (lFile >= 0)
            {
                close(lFile);
            }

            /* 内存释放 */
            if (pucBuf)
            {
                free(pucBuf);
            }

            retcode = EP_ERROR;
            LOG_Dbg_Msg("RdNum = %d Read Error!\n",RdNum,0,0,0,0,0);
            return retcode;
        }
    }

    pDest = pucBuf;
    for(plgcpo = plgcpoch_g, i=0; plgcpo<plgcpoch_g+iLgcPoChNum_g; plgcpo++, i++)
    {
        pDest = pucBuf+20*i;		/* 定位*/
        plgcpo->Val.TotalEnergy = atof(pDest);		/* 字符串转整型 */
        plgcpo->OriginVal.TotalEnergy = plgcpo->Val.TotalEnergy;		/* 原始值*/
        /* logMsg("%d=%d\n", i, (int)(plgcpo->Val.TotalEnergy*1000), 0, 0, 0, 0); */
    }

    free(pucBuf);
    close(lFile);

    return retcode;
}

/***********************************************************************
* FileWrTestEntry - 文件写测试任务入口函数
*
* RETURNS: 无
*
*/
EP_STATUS FileWrTestEntry(void)
{
    static int preTime=0;
    static int ulCnt = 0;

    while(1)
    {
        if((tickGet()-preTime)>=100)/* 1秒定时器 */
        {
            FileWrFile();	/* 写*/
            FileRdFile();		/* 读 */
            ulCnt++;
            if(ulCnt%60 == 1)
                logMsg("文件系统测试: 已写读%d次\n", ulCnt, 0, 0, 0, 0, 0);

            preTime =  tickGet();
        }
    }
    DeleteSelfTaskFromList();

    return EP_SUCCESS;
}

/***********************************************************************
* FileWrTestStart - 文件写测试任务开始
*
* RETURNS: 无
*
*/
EP_STATUS FileWrTestStart(void)
{
    int nFileWrTestTaskID;

    if(FileWrInit() == EP_SUCCESS)
    {
        FileRdFile();		/* 初始化 */
        logMsg("文件测试初始化成功!\n", 0, 0, 0, 0, 0, 0);
    }
    else
    {
        logMsg("文件测试初始化失败!\n", 0, 0, 0, 0, 0, 0);
    }

    /* DSP任务创建 */
    nFileWrTestTaskID = taskSpawn("tFileWrTest",
                                  98,
                                  VX_FP_TASK|VX_DEALLOC_STACK,
                                  10000,		/* 堆栈由200改为100，以下同*/
                                  (FUNCPTR)FileWrTestEntry,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    AddTaskToList(nFileWrTestTaskID, TRUE, "看门狗复位:因文件测试任务异常或退出,看门狗复位CPU.\n", TRUE);

    return EP_SUCCESS;
}

void FestRamTest(int Num)
{
    int i;
    float fVal;
    unsigned char *pTmpData;

    unsigned char *pSrcData;
    uint32_t ultmpfist;
    uint32_t ultmpsec;
    STATUS vxsts;	/* DY 11/2/2006 */

    for(i=0; i<Num; i++)
    {
        pTmpData=(unsigned char *)(&fVal);

        if(read_ram_data(4*i, (unsigned char *)&ultmpfist, 4) != OK)
        {
            if(ENG_MODE==0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "铁电存储器错误(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL, "铁电读出错误\n",NULL);
            }
            else if(ENG_MODE ==1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "Fest RAM  error(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL,"Fest RAM writing error!\n", NULL);
            }
            continue;
        }

        if(read_ram_data(0x1000+4*i, (unsigned char *)&ultmpsec, 4) != OK)
        {
            if(ENG_MODE==0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "铁电存储器错误(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL, "铁电读出错误\n",NULL);
            }
            else if(ENG_MODE ==1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "Fest RAM  error(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL,"Fest RAM writing error!\n", NULL);
            }
            continue;
        }

        if(ultmpfist != ultmpsec)
        {
            logMsg(" 铁电读取失败!\n", 0, 0, 0, 0, 0, 0);
            continue;
        }

        pSrcData=(unsigned char *)(&ultmpfist);

        vxsts=taskLock();
        pTmpData[0]=pSrcData[0];		/* 写到目标 */
        pTmpData[1]=pSrcData[1];
        pTmpData[2]=pSrcData[2];
        pTmpData[3]=pSrcData[3];
        vxsts=taskUnlock();

        logMsg("PO: %d=%d\n", i, (int)(fVal*1000), 0, 0, 0, 0);
    }
}

/***********************************************************************
* RamTest - 铁电测试
*
* RETURNS:
*			EP_SUCCESS, 正常
*			EP_BUF_ERR, 错误
*/
EP_STATUS RamTest(void)
{
    while(1)
    {
        PoWrFile();
        taskDelay(20);
    }
}

/***********************************************************************
* RamTest2 - 铁电测试
*
* RETURNS:
*			EP_SUCCESS, 正常
*			EP_BUF_ERR, 错误
*/
EP_STATUS RamTest2(void)
{
    int i;
#define DATALENGTH 8
    unsigned char Data[10]= {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    unsigned char TmpData[10];
    unsigned char utmpfist[10];
    unsigned char utmpsec[10];
    uint16_t LastCRC=0;
    uint16_t LastCRCLst;
    uint16_t LastCRCSec;
    uint16_t LastCRCLstRd=0;
    uint16_t LastCRCSecRd=0;

    while(1)
    {
        LastCRC=EP_CCITT_CRC16(Data, DATALENGTH, LastCRC);

        if(write_ram_data(0x0, Data, DATALENGTH) != OK)
        {
#ifdef ERSETFORRAM
            if(ENG_MODE==0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL, "铁电写入错误\n",NULL);
            }
            else if(ENG_MODE ==1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,		ER_LOCK
                           "Fest RAM  error(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL,"Fest RAM writing error!\n", NULL);
            }
#endif
            logMsg("铁电写入错误\n", 0, 0, 0, 0, 0, 0);
        }

        if(write_ram_data(0x1000+0x0, Data, DATALENGTH) != OK)
        {
#ifdef ERSETFORRAM
            if(ENG_MODE==0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL, "铁电写入错误\n",NULL);
            }
            else if(ENG_MODE ==1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "Fest RAM  error(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL,"Fest RAM writing error!\n", NULL);
            }
#endif
            logMsg("铁电写入错误\n", 0, 0, 0, 0, 0, 0);
        }

        if(write_ram_data(0x0+DATALENGTH, (unsigned char *)&LastCRC, 2) != OK)
        {
#ifdef ERSETFORRAM
            if(ENG_MODE==0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL, "铁电写入错误\n",NULL);
            }
            else if(ENG_MODE ==1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "Fest RAM  error(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL,"Fest RAM writing error!\n", NULL);
            }
#endif
            logMsg("铁电写入错误\n", 0, 0, 0, 0, 0, 0);
        }/* CRC */

        if(write_ram_data(0x1000+0x0+DATALENGTH, (unsigned char *)&LastCRC, 2) != OK)
        {
#ifdef ERSETFORRAM
            if(ENG_MODE==0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "铁电存储器错误(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL, "铁电写入错误\n",NULL);
            }
            else if(ENG_MODE ==1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "Fest RAM  error(%02d)\n",WRITING_ERR,0);
                LOG_Write(LOG_KERNEL,"Fest RAM writing error!\n", NULL);
            }
#endif
            logMsg("铁电写入错误\n", 0, 0, 0, 0, 0, 0);
        }

        if(read_ram_data(0x0, utmpfist, DATALENGTH) != OK)
        {
#ifdef ERSETFORRAM
            if(ENG_MODE==0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "铁电存储器错误(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL, "铁电读出错误\n",NULL);
            }
            else if(ENG_MODE ==1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "Fest RAM  error(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL,"Fest RAM reading error!\n", NULL);
            }
#endif
            logMsg("铁电读出错误\n", 0, 0, 0, 0, 0, 0);
        }

        if(read_ram_data(0x1000+0x0, utmpsec, DATALENGTH) != OK)
        {
#ifdef ERSETFORRAM
            if(ENG_MODE==0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "铁电存储器错误(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL|ER_NOLOGWRITE, "铁电读出错误\n",NULL);
            }
            else if(ENG_MODE ==1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "Fest RAM  error(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL|ER_NOLOGWRITE,"Fest RAM reading error!\n", NULL);
            }
#endif
            logMsg("铁电读出错误\n", 0, 0, 0, 0, 0, 0);
        }

        if(read_ram_data(0x0+DATALENGTH, (unsigned char *)&LastCRCLst, 2) != OK)
        {
#ifdef ERSETFORRAM
            if(ENG_MODE==0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "铁电存储器错误(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL, "铁电读出错误\n",NULL);
            }
            else if(ENG_MODE ==1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "Fest RAM  error(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL,"Fest RAM reading error!\n", NULL);
            }
#endif
            logMsg("铁电读出错误\n", 0, 0, 0, 0, 0, 0);
        }

        if(read_ram_data(0x1000+0x0+DATALENGTH, (unsigned char *)&LastCRCSec, 2) != OK)
        {
#ifdef ERSETFORRAM
            if(ENG_MODE==0)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "铁电存储器错误(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL, "铁电读出错误\n",NULL);
            }
            else if(ENG_MODE ==1)
            {
                ER_Set_Err(EV_STORAGE_ERR,
                           ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                           "Fest RAM  error(%02d)\n",READING_ERR,0);
                LOG_Write(LOG_KERNEL,"Fest RAM reading error!\n", NULL);
            }
#endif
            logMsg("铁电读出错误\n", 0, 0, 0, 0, 0, 0);
        }

        LastCRCLstRd=EP_CCITT_CRC16(utmpfist, DATALENGTH, LastCRCLstRd);
        LastCRCSecRd=EP_CCITT_CRC16(utmpsec, DATALENGTH, LastCRCSecRd);

        for(i=0; i<DATALENGTH; i++)
        {
            if(utmpfist[i] != utmpsec[i])
            {
                logMsg(" 铁电读取失败!\n", 0, 0, 0, 0, 0, 0);
                continue;
            }
            TmpData[i]=utmpfist[i];
        }

        taskDelay(20);
    }
}

/***********************************************************************
* FestRamWrTestStart - 铁电测试任务开始
*
* RETURNS: 无
*
*/
EP_STATUS FestRamTestStart(void)
{
    int nFestRamTestTaskID;

    /* 任务创建 */
    nFestRamTestTaskID = taskSpawn("tFestRamTest",
                                   98,
                                   VX_FP_TASK|VX_DEALLOC_STACK,
                                   10000,		/* 堆栈由200改为100，以下同*/
                                   (FUNCPTR)RamTest2,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    AddTaskToList(nFestRamTestTaskID, TRUE, "看门狗复位:因铁电测试任务异常或退出,看门狗复位CPU.\n", TRUE);

    return EP_SUCCESS;
}

EP_STATUS UsbTest1Entry(
    int FileSn		/* 文件名 */
)
{
    char srcStr[128]= {0};
    int srcFd;
    uint8_t aucBuf[10]= {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    int Num;

    strcpy(srcStr, usbcopySrcDir);
    strcat(srcStr, "/");
    sprintf(usbcopySrcFileName[FileSn], "%s%02x.ini", "data", FileSn);
    strcat(srcStr, usbcopySrcFileName[FileSn]);

    srcFd=open(srcStr, O_RDWR, 0);
    if(srcFd<0)
    {
        logMsg("Open File Error!\n", 0, 0, 0, 0, 0, 0);
        return EP_ERROR;
    }

    while(1)
    {
        Num=write(srcFd, aucBuf, 10);

        if(Num != 10)
        {
            logMsg("Write Error!\n", 0, 0, 0, 0, 0, 0);
        }

        /* aucBuf[0]++; */

        taskDelay(200);
    }
}

EP_STATUS UsbTest2Entry(
    int FileSn		/* 文件名 */
)
{
    char srcStr[128]= {0};
    FILE *srcFd;
    uint8_t aucBuf[10]= {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    int Num;

    strcpy(srcStr, norcopySrcDir);
    strcat(srcStr, "/");
    sprintf(usbcopySrcFileName[FileSn], "%s%02x.ini", "data", FileSn);
    strcat(srcStr, usbcopySrcFileName[FileSn]);

    srcFd=fopen(srcStr, "w");
    if(srcFd == NULL)
    {
        logMsg("Open File Error!\n", 0, 0, 0, 0, 0, 0);
        return EP_ERROR;
    }

    while(1)
    {
        Num=fwrite(aucBuf, 1, 10, srcFd);

        if(Num != 10)
        {
            logMsg("Write Error!\n", 0, 0, 0, 0, 0, 0);
        }

        /* aucBuf[0]++; */

        taskDelay(200);
    }
}

EP_STATUS UsbTestTaskStart(
    int FileSn			/* 文件名 */
)
{
    int nUsbTestTaskID1;
    int nUsbTestTaskID2;

    /* 任务创建 */
    nUsbTestTaskID1 = taskSpawn("tUsbTest",
                                140,
                                VX_FP_TASK|VX_DEALLOC_STACK,
                                10000,		/* 堆栈由200改为100，以下同*/
                                (FUNCPTR)UsbTest1Entry,
                                FileSn,
                                0, 0, 0, 0, 0, 0, 0, 0, 0);
    AddTaskToList(nUsbTestTaskID1, TRUE, "看门狗复位:因USB测试任务异常或退出,看门狗复位CPU.\n", TRUE);

    /* 任务创建 */
    nUsbTestTaskID2 = taskSpawn("tNorTest",
                                140,
                                VX_FP_TASK|VX_DEALLOC_STACK,
                                10000,		/* 堆栈由200改为100，以下同*/
                                (FUNCPTR)UsbTest2Entry,
                                FileSn,
                                0, 0, 0, 0, 0, 0, 0, 0, 0);
    AddTaskToList(nUsbTestTaskID2, TRUE, "看门狗复位:因USB测试任务异常或退出,看门狗复位CPU.\n", TRUE);

    return EP_SUCCESS;
}

void DataTypeTest(void)
{
    uint32_t ulCur=4;
    uint32_t ulLst=0xfffffffe;
    int32_t lRet;

    lRet=ulCur-ulLst;
    printf("%x\n", (int)lRet);
}

/* Stat the gap of A/D interrupt
 * Para: NONE
 */
void us_Gap_Save(uint16_t ulCnt)
{
    static BOOL hasCalled = FALSE;
    static unsigned long usPrev = 0;
    static unsigned long usNow = 0;
    long usGap = 0;

    static unsigned short usIntNow = 0;

    if (beginSaveUsGet)
    {
        if (hasCalled)
        {
            usNow = TM_Get_usCnt();
            usGap = (long)(usNow - usPrev);
            if (usGap > usMaxGap)
            {
                usMaxGap = usGap;
            }
            if (usGap < usMinGap)
            {
                usMinGap = usGap;
            }
            usPrev = usNow;

            usIntNow = ulCnt;
            if (usIntNow > usIntMaxGap)
            {
                usIntMaxGap = usIntNow;
            }
            if (usIntNow < usIntMinGap)
            {
                usIntMinGap = usIntNow;
            }

        }
        else
        {
            usPrev = TM_Get_usCnt();
            usNow = usPrev;

            hasCalled = TRUE;
        }
    }
}

/* Show the gap of A/D interrupt
 * Para: NONE
 */
void show_Adc_Gap_Save(void)
{
    printf("A/D Interval: Max=%d, Min=%d.\n", (int)usMaxGap, (int)usMinGap);
    printf("A/D Read Delay: Max=%d, Min=%d.\n", (usIntMaxGap*160+500)/1000, (usIntMinGap*160+500)/1000);
}

/* calculate the interval of two sampling pointing.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void us_Interval_Cal (uint32_t ulSampleRate)
{
    static uint32_t ulLstVal;
    static uint32_t ulCnt = 0;
    uint32_t ulCurVal;

    ulCnt++;
    ulCurVal = TM_Get_usCnt ();

    if (!(ulCnt%ulSampleRate))
    {
        LOG_Dbg_Msg("period is: %d, interval: %d.\n", ulSampleRate, ulCurVal-ulLstVal, 0, 0, 0, 0);
        ulLstVal = ulCurVal;
    }
}

/* test the multiplication of two integer.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void MulTest(void)
{
    uint32_t ulmuldata1 = 3;
    static uint32_t ulCnt = 1431655759;

    ulCnt++;
    if ((ulCnt*ulmuldata1) % 240)
    {
        LOG_Dbg_Msg("%x 非整除.\n", ulCnt*ulmuldata1, 0, 0, 0, 0, 0);
    }
    else
    {
        LOG_Dbg_Msg("%x 整除.\n", ulCnt*ulmuldata1, 0, 0, 0, 0, 0);
    }
}