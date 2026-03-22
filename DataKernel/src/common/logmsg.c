/* logmsg.c - This file contains external interface of log message module */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 29dec02, hdx Verified version 0.1.
01a, 23oct02, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains external interface of log message module.
INCLUDE: logmsg.h
*/

/* includes */

#include "edpbase.h"
#include "logmsg.h"
#include "filetool.h"
#include "swcfg.h"

#include <stdio_compat.h>
#include "string_compat.h"
#include <logLib.h>
#include <ioLib.h>
#include <semLib.h>
#include <taskLib.h>
#include <intLib.h>
#include <sys/stat.h>
#include "miscfunc.h"
#include "sys_ioctl_compat.h"
#include "unistd_compat.h"
/* defines */

#define LOG_BUF_NUM     256				/* The number of LOG. */
#define LOG_FILE_BUF_SIZE   150000L  					/* The size of the buffer for LOG file, now is 150000 Bytes, the old is 500000. */
#define LOG_MAX_LOG_NUM   1000      /*日志文件记录最大日志个数，包括2种类型  */
#define LOG_MAX_EX_LOG_NUM   100    /*日志文件记录的最大特殊日志个数  */
#define LOG_MAX_GN_LOG_NUM     (LOG_MAX_LOG_NUM-LOG_MAX_EX_LOG_NUM)     /*日志文件记录允许的普通日志个数  */
#define LOG_GEN_LOG_BASE     LOG_MAX_EX_LOG_NUM   /*普通日志在日志文件中的基址  */
#define LOG_EXTRA  0x8000         /* 表示记录特殊日志类型，和LOG_RUN并列，但不会被应用使用，只供内部使用 */
/* typedefs */

typedef struct
{
    uint16_t unLvl;		/* running information, operation information, information form arithmatic, information from processing, infotmation from BSP. */
    uint8_t aucMsg[99];													/* information */
    uint32_t  ulUsTime;						/* The us count when logging.以前BUG,3.0版本中未合并进去,ZY 2009-11-24重新修改 */
} LOG_BUF;

/* locals */

static SEM_ID semWrLog_g;				/* sem for LOG */
static LOG_BUF alogBuf_g[LOG_BUF_NUM];							/* Buffer for LOG. */
static int iBufUsed_g;						/* The number of LOG that can be used. */

static int iWrLogFd_g=-1;							/* 日志文件的句柄 */
static int iMaxLogNum_g;				/* 允许的最大日志总数 */
static int iLogWrIdx_g;											/* 普通日志当前正在写的文件中的位置序号，在特殊日志区之后 */
static int iWrIdxPos_g;									/* 文件头部的下一个日志序号INDEX提示信息在文件中的写位置,是一个固定位置 */


static BOOL bOpenLogMsg_g=FALSE;    /* 是否打印信息标志变量标志，可由LOG_OpenLog函数控制为TRUE，
                                       该变量和FAST_BOOT引脚信号以或逻辑控制是否打印 */

static int nLogWriteTaskID_g;		/* 为了监测日志记录任务的正常与否，而设置的全局变量任务ID号 */
static char *pLogFileBackBuf=NULL;
static char *pLogFileBackBufTail=NULL;		/* 缓冲区末尾 */
static int nBackBufLen_g=-1;
static BOOL bBootFq_g = FALSE;		/* 频繁重启标志 */


static  int  iMaxExLogNum_g;          /*最大特殊日志个数  */
static  int  iMaxGnLogNum_g;          /*最大普通日志个数　*/
static  int  iExLogKeyOfstInItem_g;          /*特殊日志的关键字在特殊日志信息串中的偏移*/
uint32_t   ulLogNonComleteCnt_g=0;      /*当前未完成写入消息队列的日志计数　2010-4-26 ZY  */




/* globals */

BOOL bLogWriteTaskStartFlag_g=FALSE;
EP_DATE_TIME LOG_dtLastWdRebootTime_g;			/* 从日志获得的，看门狗上次重启绝对时间 */

/* local functions */

static BOOL LOG_Is_Valid_File(
    int iFd
);

static int LOG_Creat_New(void);

static int LOG_Wr_File(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
);

static void LOG_WrLogFileTaskExecHandle(
    int  iFd
);

static int LOG_Begain_Wr_New_Info();

/*功能：写入普通日志到日志文件中
　　　　参数： iFd,　日志文件符
               plog，日志信息 */
static   void  LOG_WriteGnLogToFile(int  iFd,LOG_BUF *plog);


/*写入特殊日志到日志文件中
　　　参数：iFd,　日志文件符
            plog，日志信息   */
static   void  LOG_WriteExLogToFile(int  iFd,LOG_BUF *plog);

/*清空特殊日志区域
　参数：iFd ，日志文件描述符
　返回：>=0　表示操作成功
　　　　<0,表示操作失败*/
static  int LOG_ClrExLog(int  iFd);


/* global functions */

/***********************************************************************
* GetAdjustTimeSuccessFlag - Get the time adjustment flag.
*
* RETURNS: 无
*
* Alert: MMI_SOFT.C提供的对时成功标志, 04若初始化后还没有对时成功,则返回FALSE,否则,返回TRUE
*
*/
extern BOOL GetAdjustTimeSuccessFlag();



/*得到某特殊日志条目在日志文件中待写入日志位置
　　　参数： plog 待查询的特殊日志
      返回： int  该特殊日志待写入的日志位置
      　　　 >=0,表示找到合适的写入位置，若有同名条目名称，则返回相应位置，
      　　　　　　　　　　　　　　　　　否则若有空白条目，则返回第１个空白位置
      　　　 <0,表示无空白位置，无法写入  */
extern  int  LOG_GetExLogWrIdx(LOG_BUF *plog);
/* functions */

/***********************************************************************
* LOG_Assert - 自己封装的assert，在系统assert之前将行号和文件名记录到日志
*
* Para:
* strDef: msg to be showed in assert
* strFile: file name where assert happens
* nline: line NO. in the strFile where assert happens
* unOpFlag: 是对assert错误作出的反应，这是一个位标志，可以根据需要把它们
*       或起来（当标志为0时最基本的错误反应是记录到日志中）
*       #define ER_ALERT    0x0001     呼唤
*       #define ER_REPORT   0x0002     保存到实时/历史事件记录
*       #define ER_ALARM    0x0004     告警
*       #define ER_LOCK     0x0008     闭锁保护功能
*
*/
void LOG_Assert(char *strDef, char *strFilename, int nline, uint16_t unOpFlag)
{
    uint8_t aucLogInfo[255]="";

    sprintf(aucLogInfo, "Assertion failed: %s, at file: %s, line: %d.", strDef, strFilename, nline);

    /* ER_Set_Err(EV_HINT_INFO, unOpFlag, aucLogInfo, 0, 0); */

    taskDelay(200);
    assert(0);
}

/***********************************************************************
* LOG_Init - 日志初始化
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS LOG_Init(void)
{
    STATUS vxsts;

    iMaxLogNum_g = LOG_MAX_LOG_NUM;
    iMaxExLogNum_g=LOG_MAX_EX_LOG_NUM;
    iMaxGnLogNum_g=LOG_MAX_GN_LOG_NUM;
    iExLogKeyOfstInItem_g=29;


    /* 设置上次看门狗重启时间默认值 */
    LOG_dtLastWdRebootTime_g.unYear = 2000;
    LOG_dtLastWdRebootTime_g.ucMonth = 1;
    LOG_dtLastWdRebootTime_g.ucDate = 1;
    LOG_dtLastWdRebootTime_g.ucHour = 0;
    LOG_dtLastWdRebootTime_g.ucMinute = 0;
    LOG_dtLastWdRebootTime_g.ucSec = 0;
    LOG_dtLastWdRebootTime_g.unMSEL = 0;
    LOG_dtLastWdRebootTime_g.unMicroSec = 0;

    if (FT_Is_File(EP_SYS_LOG_FILE))
    {
        /* 该文件存在 */
        iWrLogFd_g = open(EP_SYS_LOG_FILE, O_RDWR, 0);
        assert (iWrLogFd_g >= 0);

        if (!LOG_Is_Valid_File(iWrLogFd_g))
        {
            /* 旧文件不合法 */
            LOG_Dbg_Msg("warning, old log file is invalid, it is refreshed.\n", 0, 0, 0, 0, 0, 0);
            vxsts = close(iWrLogFd_g);
            assert (vxsts == OK);

            iWrLogFd_g = -1;

            if (FT_Is_File(EP_BAK_LOG_FILE))
            {
                vxsts = remove(EP_BAK_LOG_FILE);
                assert (vxsts == OK);
            }

            vxsts = rename(EP_SYS_LOG_FILE, EP_BAK_LOG_FILE);			/* 若旧文件不合法, 将旧文件改名为EP_BAK_LOG_FILE */
            assert (vxsts == OK);
        }
    }

    pLogFileBackBuf = (char *)malloc(LOG_FILE_BUF_SIZE);			/* 申请空间防止出错 */
    if (!pLogFileBackBuf)
    {
        /* 若缓冲申请不成功 */
        LOG_Dbg_Msg("ERROR, malloc Log File Backup Buf failure for no memory when Init LOG.\n", 0, 0, 0, 0, 0, 0);

        return EP_BUF_ERR;
    }

    if (iWrLogFd_g<0)
    {
        if (LOG_Creat_New()<0)
        {
            /* 若创建新的文件失败, 则返回 */
            LOG_Dbg_Msg("ERROR, Create new Log File failure for error when Init LOG.\n", 0, 0, 0, 0, 0, 0);

            return EP_FILE_ERR;
        }
    }
    else
    {
        /* 2009-11-11 ZY,若是老文件,则清除特殊日志区域*/
        if(LOG_ClrExLog(iWrLogFd_g)<0)
        {
            LOG_Dbg_Msg("提示, 清空特殊日志区域不成功\n", 0, 0, 0, 0, 0, 0);
            return EP_FILE_ERR;

        }

    }

    if (LOG_Begain_Wr_New_Info()<0)
    {
        /* 若预处理错误,则忽略该新日志 */
        return  EP_FILE_ERR;
    }

    semWrLog_g = semCCreate(SEM_Q_PRIORITY, 0);
    assert (semWrLog_g != NULL);

    nLogWriteTaskID_g = taskSpawn("tWrLog", TSK_PRI_WR_LOG, 0, 50000, LOG_Wr_File, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    assert (nLogWriteTaskID_g != ERROR);
    bLogWriteTaskStartFlag_g = TRUE;

    return EP_SUCCESS;
}

/* 记录系统日志
 * 参数: unLvl，日志级别，可且仅可取如下值其中之一：
 *           LOG_RUN; LOG_OPRATE; LOG_ARITH; LOG_INFO; LOG_KERNEL.
 *       strMsg，日志信息，受日志最大长度限制，有可能被截尾.
 *       pdttm，日志时间，如果为NULL，系统会记录调用发生的时间.
 * 返回值: 无
 * 注意: 此函数可以在中断的上下文中调用
 *
 */
void LOG_Write(uint16_t unLvl, const uint8_t *strMsg, const EP_DATE_TIME *pdttm)
{
    int iLockKey;
    static int iBufWrPos;
    LOG_BUF *plog;
    STATUS vxsts;
    static uint32_t ulWrNum = 0;  /* 记录条数控制 */

    ulWrNum++;
    if ((!bLogWriteTaskStartFlag_g) || (ulWrNum>MAX_LOG_NUM))
    {
        /* 若还没有初始化,则空操作 */
        return;
    }


    if (iWrLogFd_g >= 0)
    {
        // iLockKey=intLock();
        // if (iBufUsed_g<LOG_BUF_NUM)
        // {
        //     ulLogNonComleteCnt_g++;
        //     iBufUsed_g++;			/* 可使用的增加1 */
        //     plog=alogBuf_g+iBufWrPos++;					/* 已经产生的日志维持一个缓冲，最大为LOG_BUF_NUM */

        //     if (iBufWrPos >= LOG_BUF_NUM)
        //         iBufWrPos=0;

        //     intUnlock(iLockKey);

        //     plog->unLvl=unLvl;
        //     strncpy(plog->aucMsg, strMsg, sizeof(plog->aucMsg)-1);
        //     plog->aucMsg[sizeof(plog->aucMsg)-1]='\0';

        //     plog->ulUsTime=TM_High_Get_usCnt();


        //     vxsts=semGive(semWrLog_g);
        //     assert(vxsts==OK);
        //     ulLogNonComleteCnt_g--;
        // }
        // else
        //     intUnlock(iLockKey);		/* 若缓冲满, 则丢掉日志 */
    }
}

/***********************************************************************
* LOG_Creat_New - 创建新的日志文件
*
* RETURNS: 返回值, <0, 表示创建失败
*
*/
static int LOG_Creat_New(void)
{
    uint8_t aucBuf[256];
    int i;
    int j;

    iWrLogFd_g=creat(EP_SYS_LOG_FILE, O_RDWR);
    if (iWrLogFd_g >= 0)
    {
        assert (strlen(aucEqName_g)<100);

        i=sprintf(aucBuf, "EDP 01 log file - DO NOT EDIT!\n"
                  "Equipment name: %s\n"
                  "Max log number: %u\n"
                  "Next log index: 0    \n", aucEqName_g, iMaxLogNum_g);

        iLogWrIdx_g=LOG_GEN_LOG_BASE;			/* 初始化第1个日志的写位置，在特殊日志区之后 */
        iWrIdxPos_g=i-6;					/* 下一个日志序号的位置 */

        memset(aucBuf+i, ' ', 255-i);
        aucBuf[255]='\f';

        i=write(iWrLogFd_g, aucBuf, 256);
        if (!(i == 256))
        {
            return -1;
        }


        /*初始化日志区  */
        memset(aucBuf, ' ', 127);
        aucBuf[127]='\n';  /*2009-11-11　ZY 改掉  */
        for (j=0; j<iMaxLogNum_g; j++)
        {
            /* 预先写入空字符串 */
            i=write(iWrLogFd_g, aucBuf, 128);
            if (!(i == 128))
            {
                return -1;
            }
        }
    }

    return  iWrLogFd_g;
}

/***********************************************************************
* LOG_Is_Valid_File - justify if the LOG file is valid
*
* RETURNS: None
*
*/
static BOOL LOG_Is_Valid_File(int iFd)
{
    uint8_t aucBuf[256];
    uint8_t aucHead[256];
    int i;
    int iCurRdIdxPos;
    int k;

    assert (iFd >= 0);

    if (FT_File_Len(iFd) != 256+128*iMaxLogNum_g)
    {
        return FALSE;
    }
    i=read(iFd, aucBuf, 256);
    assert (i == 256);

    if (aucBuf[255] != '\f')
    {
        return FALSE;
    }

    i=sprintf(aucHead, "EDP 01 log file - DO NOT EDIT!\n"
              "Equipment name: %s\n"
              "Max log number: %u\n"
              "Next log index: ", aucEqName_g, iMaxLogNum_g);

    if (memcmp(aucBuf, aucHead, i))
    {
        return FALSE;
    }

    aucBuf[255]='\0';
    iLogWrIdx_g=atoi(aucBuf+i);
    if (iLogWrIdx_g<LOG_GEN_LOG_BASE || iLogWrIdx_g >= iMaxLogNum_g)
    {
        return FALSE;
    }
    iWrIdxPos_g=i;

    /* 循环读取以前的日志信息位置，提供获得日志文件上次看门狗重启的绝对时间 */
    iCurRdIdxPos=iLogWrIdx_g-1;

    for (k=0; k<iMaxLogNum_g; k++)
    {
        /* 循环，当日志较满时，效率可能比较低下 */
        if (iCurRdIdxPos<0)
        {
            iCurRdIdxPos=iCurRdIdxPos+iMaxLogNum_g;
        }

        i=lseek(iFd, 256+iCurRdIdxPos*128, SEEK_SET);
        if (!(i == 256+iCurRdIdxPos*128))
        {
            /* 若文件无这么多日志，则认为无记录看门狗重启信息 */
            break;
        }

        i=read(iFd, aucBuf, 39);
        if (!(i == 39))
        {
            /* 若文件无这么多日志，则认为无记录看门狗重启信息 */
            break;
        }

        if ((strncmp(aucBuf+29, "reboot复位", 10) == 0)
                || (strncmp(aucBuf+29, "硬狗复位", 8) == 0)
                || (strncmp(aucBuf+29, "软狗复位", 8) == 0))
        {
            /* 若找到最接近的看门狗复位日志信息，则获得该时间 */
            LOG_dtLastWdRebootTime_g.unYear = atoi(aucBuf+5);
            LOG_dtLastWdRebootTime_g.ucMonth = atoi(aucBuf+10);
            LOG_dtLastWdRebootTime_g.ucDate = atoi(aucBuf+13);
            LOG_dtLastWdRebootTime_g.ucHour = atoi(aucBuf+16);

            LOG_dtLastWdRebootTime_g.ucMinute = 0;	/* 不需要足够的精度 */
            LOG_dtLastWdRebootTime_g.ucSec = 0;
            LOG_dtLastWdRebootTime_g.unMSEL = 0;
            LOG_dtLastWdRebootTime_g.unMicroSec = 0;

            break;
        }

        iCurRdIdxPos--;
    }

    return TRUE;
}

/***********************************************************************
* LOG_Wr_File -记录日志
*
* RETURNS: None
*
*
*/
static int LOG_Wr_File(
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
)
{
    static int iBufRdPos;
    LOG_BUF *plog;
    int iLockKey;
    STATUS vxsts;

//     while (1)
//     {
// #if defined(EDP_01_02_BUILD)		/* EDP01_CA and EDP02 must wait for the time adjustment flag, otherwise do not write LOG file. */
//         if(GetAdjustTimeSuccessFlag())
//             /* if(TRUE) */
//         {
//             /* If the time adjustment have finished, then writing LOG file. */
// #endif
//             vxsts=semTake(semWrLog_g, WAIT_FOREVER);
//             assert (vxsts == OK);

//             while(ulLogNonComleteCnt_g!=0)
//             {
//                 /*若还有新消息未完全写入消息队列，则等待100ms 2010-4-26 ZY  */
//                 taskDelay(10);
//             }

//             if (!(iLogWrIdx_g >= 0 && iBufUsed_g>0 && iBufUsed_g <= LOG_BUF_NUM))
//             {
//                 /* If the counter error, then not operate, and pop out message. iLogWrIdx_g is the current serial No., and iBufUsed_g is the number of LOG that can be used. */
//                 static uint32_t ulMsgCnt=0;

//                 if (ulMsgCnt%200 == 0)
//                 {
//                     logMsg("ERROR, Log Info Buf error.\n", 0, 0, 0, 0, 0, 0);
//                 }

//                 ulMsgCnt++;

//                 continue;
//             }

//             /* 只要获得信号量, 不管真正写到文件中成功与否, 都需进行读写位置的调整,
//                否则由于信号量已获得, 和缓冲写的位置不匹配 */
//             plog=alogBuf_g+iBufRdPos++;			/* Counter for reading. */
//             if (iBufRdPos >= LOG_BUF_NUM)
//             {
//                 iBufRdPos=0;
//             }
//             iLockKey=intLock();
//             iBufUsed_g--;		/* Reduce the number. */
//             intUnlock(iLockKey);

//             if(plog->unLvl ==LOG_EXTRA)
//             {
//                 /*特殊日志  */
//                 LOG_WriteExLogToFile(iWrLogFd_g,plog);
//             }
//             else
//             {
//                 /*普通日志*/
//                 LOG_WriteGnLogToFile(iWrLogFd_g,plog);

//             }

// #if defined(EDP_01_02_BUILD)		/* EDP01_CA and EDP02 must wait for the time adjustment flag, otherwise do not write the LOG file. */
//         }
//         else
//         {
//             /*若还没有对时,则等待30秒  */
//             taskDelay(SYS_SEC*30);
//         }
// #endif
//     }
}

/***********************************************************************
* GetLogTaskStatus -获得Log_Task的状态
*
* RETURNS:
*               TRUE, 正常
*               FALSE, 不正常
*
*/
BOOL GetLogTaskStatus()
{
    static  char  strTaskStatus[128];
    if(taskIdVerify(nLogWriteTaskID_g)==ERROR)
    {
        /*首先判定该任务是否有效  */
        return   FALSE;
    }
    taskStatusString(nLogWriteTaskID_g,strTaskStatus);/*只有当该任务有效时,读取该任务的状态字才有效,否则返回的虚假信息  */

    if(strcmp(strTaskStatus,"SUSPEND")==0
            ||strcmp(strTaskStatus,"DELAY+S")==0
            ||strcmp(strTaskStatus,"PEND+S")==0
            ||strcmp(strTaskStatus,"PEND+S+T")==0
            ||strcmp(strTaskStatus,"SUSPEND+I")==0
            ||strcmp(strTaskStatus,"DELAY+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+T+I")==0
            ||strcmp(strTaskStatus,"DEAD")==0)
    {
        return   FALSE;
    }
    else
    {
        return   TRUE;
    }
}

/***********************************************************************
* LOG_WrLogFileTaskExecHandle - 为了处理日志文件写入异常时的处理函数，删掉旧文件, 创建新文件
*
* RETURNS: OK, or ERROR
*
*/
static void LOG_WrLogFileTaskExecHandle(int iFd)
{
    int nWriteLen;
    static uint32_t ulExecCnt=0;

    if (iFd >= 0)
    {
        /* 若是有效文件, 则关闭之 */
        close(iFd);
    }

    if (FT_Is_File(EP_SYS_LOG_FILE))
    {
        /* 删掉日志文件 */
        remove(EP_SYS_LOG_FILE);
    }

    if (ulExecCnt%200 == 0)
    {
        logMsg("WARNING, Write new info to Log File failure for busy or no space.\n", 0, 0, 0, 0, 0, 0);
    }
    ulExecCnt++;

    iWrLogFd_g=-1;         /* 置文件无效标志 */

    if (nBackBufLen_g<0)
    {
        /* 若缓冲中的数据是无效, 重新建立全新文件 */
        if(LOG_Creat_New()<0)
        {
            /* 若创建新文件失败, 空操作 */
            static uint32_t ulMsgCnt=0;

            if (ulMsgCnt%200 == 0)
            {
                logMsg("WARNING,create new  Log File failure for error  or  no space!\n", 0, 0, 0, 0, 0, 0);
            }

            ulMsgCnt++;
            if (iWrLogFd_g >= 0)		/* 防止新日志文件创建成功, 但写失败的情况 */
            {
                /* 若是有效文件,则关闭之 */
                close(iWrLogFd_g);
            }

            if (FT_Is_File(EP_SYS_LOG_FILE))
            {
                /* 删掉日志文件 */
                remove(EP_SYS_LOG_FILE);
            }
            iWrLogFd_g=-1;         /* 置文件无效标志 */
            return;
        }
    }
    else
    {
        /* 否则将将旧文件缓冲中的内容恢复填写到文件中 */
        iWrLogFd_g=creat(EP_SYS_LOG_FILE, O_RDWR);
        if (iWrLogFd_g<0)
        {
            /* 若创建不成功 */
            static uint32_t ulMsgCnt=0;

            if(ulMsgCnt%200==0)
            {
                logMsg("WARNING,Resume Backup  Log File failure for error  or  no space!\n", 0, 0, 0, 0, 0, 0);
            }

            ulMsgCnt++;

            return ;
        }
        nWriteLen=write(iWrLogFd_g, pLogFileBackBuf, nBackBufLen_g);
        if (nWriteLen != nBackBufLen_g)
        {
            /* 若写入不成功 */
            static uint32_t ulMsgCnt=0;

            if (ulMsgCnt%200 == 0)
            {
                logMsg("WARNING,Resume Backup  Log File failure for error  or  no space  when write!\n", 0, 0, 0, 0, 0, 0);
            }
            ulMsgCnt++;

            if (iWrLogFd_g >= 0)		/* 防止新日志文件创建成功, 但写失败的情况 */
            {
                /* 若是有效文件, 则关闭之 */
                close(iWrLogFd_g);
            }

            if (FT_Is_File(EP_SYS_LOG_FILE))
            {
                /* 删掉日志文件 */
                remove(EP_SYS_LOG_FILE);
            }

            iWrLogFd_g=-1;         /* 置文件无效标志 */
            return;
        }
    }
}

/***********************************************************************
* LOG_Begain_Wr_New_Info - 在写新的日志到日志文件之前的预处理,保存旧的文件内容到缓冲,防止写操作失败
*
* RETURNS: OK, or ERROR
*
*/
static int LOG_Begain_Wr_New_Info()
{
    /* 若返回值<0, 则表示预操作不成功 */
    STATUS vxsts;
    struct stat statStruct;
    int nReadLen;
    int i;

    /* 每1次保存1次日志文件到缓冲 */

    // vxsts= ioctl (iWrLogFd_g, FIOFSTATGET, (int)&statStruct);
    vxsts= fstat(iWrLogFd_g, &statStruct);
    if(vxsts==ERROR)
    {
        /* 若读取文件状态异常,则处理该文件,返回失败 */
        static uint32_t ulMsgCnt=0;
        if(ulMsgCnt%200==0)
        {
            logMsg("ERROR,Get Log File status  failure!\n",
                   0, 0, 0, 0, 0, 0);
        }
        ulMsgCnt++;
        LOG_WrLogFileTaskExecHandle(iWrLogFd_g);
        return  -1;
    }
    if(statStruct.st_size>=LOG_FILE_BUF_SIZE-10)
    {
        /*若返回长度过大,则认为出错  */
        static   uint32_t   ulMsgCnt=0;
        if(ulMsgCnt%200==0)
        {
            logMsg("ERROR,Get Log File  Length  failure!\n",
                   0,0,0,0,0,0);
        }
        ulMsgCnt++;
        LOG_WrLogFileTaskExecHandle(iWrLogFd_g);
        return  -1;
    }

    i=lseek(iWrLogFd_g, 0, SEEK_SET);/*需要先定位于开始位置  */
    if(i!=0)
    {
        /*若移动文件指针出错  */
        static   uint32_t   ulMsgCnt=0;
        if(ulMsgCnt%200==0)
        {
            logMsg("ERROR,Seek  Log File  Pointer  failure!\n",
                   0,0,0,0,0,0);
        }
        ulMsgCnt++;
        LOG_WrLogFileTaskExecHandle(iWrLogFd_g);
        return  -1;
    }
    nReadLen= read(iWrLogFd_g,pLogFileBackBuf,statStruct.st_size);
    if((nReadLen!=statStruct.st_size))
    {
        /*若整个文件读取到缓冲不成功,则返回失败  */
        static   uint32_t   ulMsgCnt=0;
        if(ulMsgCnt%200==0)
        {
            /*    */
            static  uint32_t  ulReadLen_test;
            static  uint32_t  ulSize_test;/*for test  */

            ulReadLen_test=nReadLen;
            ulSize_test=statStruct.st_size;
            logMsg("ERROR,Read Log File to Backup Buffer  failure,Log file ReadLen is %d,log file Size is %d!\n",
                   ulReadLen_test,ulSize_test,0,0,0,0);
        }

        nBackBufLen_g=-1;
        ulMsgCnt++;
        LOG_WrLogFileTaskExecHandle(iWrLogFd_g);
        return  -1;
    }

    nBackBufLen_g=nReadLen;			/* 返回读到的实际长度 */
    return  1;
}

/***********************************************************************
* MylogMsg - 封装的logMsg
*
* RETURNS: OK, or ERROR
*
*/
int MylogMsg (
    char *fmt,
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6
)
{
    if(bOpenLogMsg_g||EP_IS_FAST_BOOT())
    {
        return logMsg (fmt, arg1, arg2,arg3, arg4, arg5, arg6);
    }
    else
    {
        return  0;
    }
}

/***********************************************************************
* LOG_OpenLog - 开放LOG_Dbg_Msg功能
*
* RETURNS: 无
*
*/
void LOG_OpenLog()
{
    bOpenLogMsg_g=TRUE;
}

/***********************************************************************
* LOG_CloseLog - 关闭LOG_Dbg_Msg功能
*
* RETURNS: 无
*
*/
void LOG_CloseLog()
{
    bOpenLogMsg_g=FALSE;
}




/*功能：记录特殊日志条目
　参数：strItemKey  日志条目关键字字符串
　　　　strItemContent  日志条目内容字符串
  注意：strItemKey+strItemContent长度不要超过80字节,否则会截尾
  　　　相同关键字的特殊日志可以被重复写入，覆盖掉以前的日志条目  */
void  LOG_ExtraItemWrite(const uint8_t *strItemKey, const uint8_t  *strItemContent)
{
    uint8_t aucRec[128];
    int  i;
    if((!strItemKey)||(!strItemContent))
    {
        return;
    }
    if((strlen(strItemKey)+strlen(strItemContent))>120)
    {
        /*防止越界  */
        return;
    }
    i=sprintf(aucRec, "%s=%s\n",strItemKey, strItemContent);
    if(i<0)
    {
        return;
    }

    LOG_Write(LOG_EXTRA, aucRec, NULL);

}



/*功能：写入普通日志到日志文件中
　　　　参数： iFd,　日志文件符
               plog，日志信息 */
void  LOG_WriteGnLogToFile(int  iFd,LOG_BUF *plog)
{
    EP_DATE_TIME  	dttm;
    uint8_t aucRec[256];
    int i;

    if (!( iFd>= 0))
    {
        /* 若文件无效, 则处理之, 丢掉该日志 */
        LOG_WrLogFileTaskExecHandle(iFd);
        return;
    }
    if(!plog)
    {
        return;
    }

    i=lseek(iFd, 256+iLogWrIdx_g*128, SEEK_SET);
    if (!(i == 256+iLogWrIdx_g*128))
    {
        /* 若位置不对, 则处理, 然后获得下一个日志 */
        LOG_WrLogFileTaskExecHandle(iFd);
        return ;
    }

    if(!(plog->unLvl == LOG_RUN || plog->unLvl == LOG_OPRATE ||
            plog->unLvl == LOG_ARITH || plog->unLvl == LOG_INFO ||
            plog->unLvl == LOG_KERNEL))
    {
        return ;
    }

    TM_To_Dttm(plog->ulUsTime, &dttm);

    i=sprintf(aucRec, "%04X\n%04u-%02u-%02u %02u:%02u:%02u.%03u\n%s",
              plog->unLvl, dttm.unYear, dttm.ucMonth,
              dttm.ucDate, dttm.ucHour, dttm.ucMinute,
              dttm.ucSec, dttm.unMSEL, plog->aucMsg);
    if (i>127)
    {
        /* 若事件信息大于127, 则将之截尾 */
        aucRec[127]='\0';		/* One byte. */
        i=127;
    }

    memset(aucRec+i, ' ', 128-i);
    aucRec[127]='\n';   /*2009-11-11  ZY 改掉  */

    i=write(iFd, aucRec, 128);
    if (!(i == 128))
    {
        LOG_WrLogFileTaskExecHandle(iFd);
        return;
    }
    /* 写指针 */
    pLogFileBackBufTail=pLogFileBackBuf+256+iLogWrIdx_g*128;
    memcpy(pLogFileBackBufTail, aucRec, 128);
    if (++iLogWrIdx_g >= iMaxLogNum_g)
    {
        iLogWrIdx_g=LOG_GEN_LOG_BASE;
    }

    /* judge if boot frequently. */

    if ((strncmp(aucRec+29, "reboot复位", 10) == 0)
            || (strncmp(aucRec+29, "硬狗复位", 8) == 0)
            || (strncmp(aucRec+29, "软狗复位", 8) == 0))
    {
        if ((dttm.unYear == LOG_dtLastWdRebootTime_g.unYear)
                && (dttm.ucMonth == LOG_dtLastWdRebootTime_g.ucMonth)
                && (dttm.ucDate == LOG_dtLastWdRebootTime_g.ucDate)
                && ((dttm.ucHour-LOG_dtLastWdRebootTime_g.ucHour) >= 0)
                && ((dttm.ucHour-LOG_dtLastWdRebootTime_g.ucHour) <= 2))
        {
            /* If the system was reseted frequently, the alarm node will be set. */
            bBootFq_g = TRUE;
        }
    }

    i=lseek(iFd, iWrIdxPos_g, SEEK_SET);
    if (!(i == iWrIdxPos_g))
    {
        iLogWrIdx_g--;
        if (iLogWrIdx_g<LOG_GEN_LOG_BASE)
        {
            iLogWrIdx_g=iLogWrIdx_g-LOG_GEN_LOG_BASE+iMaxLogNum_g;
        } /* 这个语句必须放到异常处理前面，因为iLogWrIdx_g在异常处理中可能被修改 */
        LOG_WrLogFileTaskExecHandle(iFd);
        return ;
    }

    i=sprintf(aucRec, "%d", iLogWrIdx_g);
    memset(aucRec+i, ' ', 4);

    i=write(iFd, aucRec, 5);
    if (!(i == 5))
    {
        iLogWrIdx_g--;
        if(iLogWrIdx_g<LOG_GEN_LOG_BASE)
        {
            iLogWrIdx_g=iLogWrIdx_g-LOG_GEN_LOG_BASE+iMaxLogNum_g;
        }
        LOG_WrLogFileTaskExecHandle(iFd);
        return ;
    }
    /* 写指针 */
    pLogFileBackBufTail=pLogFileBackBuf+iWrIdxPos_g;
    memcpy(pLogFileBackBufTail, aucRec, 5);
    /*这里调一个事务点,真正保存到磁盘上,调flush   */
    // ioctl (iFd, FIOFLUSH, 0);
    fsync(iFd);
}


/*写入特殊日志到日志文件中
　　　参数： iFd,　日志文件符
             plog，日志信息   */
void  LOG_WriteExLogToFile(int  iFd,LOG_BUF *plog)
{
    EP_DATE_TIME  	dttm;
    uint8_t aucRec[256];
    int i;
    int   iExLogWrIdx;

    if (!( iFd>= 0))
    {
        /* 若文件无效, 则处理之, 丢掉该日志 */
        LOG_WrLogFileTaskExecHandle(iFd);
        return;
    }
    if(!plog)
    {
        return;
    }
    if(plog->unLvl != LOG_EXTRA)
    {
        return ;
    }
    iExLogWrIdx=LOG_GetExLogWrIdx(plog);
    if(iExLogWrIdx<0)
    {
        /* 若无合适位置 */
        return;
    }

    i=lseek(iFd, 256+iExLogWrIdx*128, SEEK_SET);
    if (!(i == 256+iExLogWrIdx*128))
    {
        /* 若位置不对, 则处理, 然后获得下一个日志 */
        LOG_WrLogFileTaskExecHandle(iFd);
        return ;
    }

    TM_To_Dttm(plog->ulUsTime, &dttm);

    i=sprintf(aucRec, "%04X\n%04u-%02u-%02u %02u:%02u:%02u.%03u\n%s",
              plog->unLvl, dttm.unYear, dttm.ucMonth,
              dttm.ucDate, dttm.ucHour, dttm.ucMinute,
              dttm.ucSec, dttm.unMSEL, plog->aucMsg);
    if (i>127)
    {
        /* 若事件信息大于127, 则将之截尾 */
        aucRec[127]='\0';		/* One byte. */
        i=127;
    }

    memset(aucRec+i, ' ', 128-i);
    aucRec[127]='\n';   /*2009-11-11  ZY 改掉  */

    i=write(iFd, aucRec, 128);
    if (!(i == 128))
    {
        LOG_WrLogFileTaskExecHandle(iFd);
        return;
    }
    /* 写指针 */
    pLogFileBackBufTail=pLogFileBackBuf+256+iExLogWrIdx*128;
    memcpy(pLogFileBackBufTail, aucRec, 128);

    /*这里调一个事务点,真正保存到磁盘上,调flush   */
    // ioctl (iFd, FIOFLUSH, 0);
    fsync(iFd);

}



/*得到某特殊日志条目在日志文件中待写入日志位置
　　　参数： plog 待查询的特殊日志
      返回： int  该特殊日志待写入的日志位置
      　　　 >=0,表示找到合适的写入位置，若有同名条目名称，则返回相应位置，
      　　　　　　　　　　　　　　　　　否则若有空白条目，则返回第１个空白位置
      　　　 <0,表示无空白位置，无法写入  */
int  LOG_GetExLogWrIdx(LOG_BUF *plog)
{

    int  i,j;
    uint8_t  aucItemName[128];
    int  iResult;
    int  iFirstNullIdx;
    uint8_t  *puc;

    iFirstNullIdx=-1;

    if(!plog)
    {
        return  -1;
    }
    puc=strchr(plog->aucMsg,'=');
    if(!puc)
    {
        return  -1;
    }
    i=puc-plog->aucMsg;
    if(i>80)
    {
        return  -1;
    }
    /*获得日志的关键字+'='  */
    strncpy(aucItemName,plog->aucMsg,i+1);
    aucItemName[i+1]='\0';

    /*查询日志文件的内存缓冲  */
    for(i=0; i<LOG_MAX_EX_LOG_NUM; i++)
    {
        j=256+i*128;
        j=j+iExLogKeyOfstInItem_g;
        iResult=memcmp(aucItemName,(pLogFileBackBuf+j),strlen(aucItemName));
        if(!iResult)
        {
            /*若找到同名条目的特殊日志,则返回正确位置  */
            return  i;
        }
        if(iFirstNullIdx<0)
        {
            iResult=memcmp("    ",(pLogFileBackBuf+j),4);
            if(!iResult)
            {
                /*若找到第1个未用的空白条目,则保存  */
                iFirstNullIdx=i;
            }
        }
    }
    return  iFirstNullIdx;
}


/*清空特殊日志区域
　参数：iFd ，日志文件描述符
　返回：>=0　表示操作成功
　　　　<0,表示操作失败*/
int LOG_ClrExLog(int  iFd)
{

    uint8_t aucBuf[256];
    int i;
    int j;

    i=lseek(iFd, 256, SEEK_SET);
    if (!(i == 256))
    {
        /* 若位置不对, 则处理,  */
        LOG_WrLogFileTaskExecHandle(iFd);
        return -1;
    }
    /*初始化特殊日志区  */
    memset(aucBuf, ' ', 127);
    aucBuf[127]='\n';
    for (j=0; j<LOG_MAX_EX_LOG_NUM; j++)
    {
        i=write(iFd, aucBuf, 128);
        if (!(i == 128))
        {
            LOG_WrLogFileTaskExecHandle(iFd);
            return -1;
        }
    }

    return  1;
}




/***********************************************************************
* LogFileTest - LOG文件测试
*
* RETURNS: 无
*
*/
void LogFileTest()
{
    int nWriteLen;
    int iWrLogFd;

    iWrLogFd=creat("/set/log/edp.log", O_RDWR);
    assert(iWrLogFd>0);
    nWriteLen=write(iWrLogFd, pLogFileBackBuf, nBackBufLen_g);
    if(nWriteLen != nBackBufLen_g)
    {
        /* 若写入不成功 */
        static uint32_t ulMsgCnt=0;
        if(ulMsgCnt%200 == 0)
        {
            logMsg("WARNING,Resume Backup  Log File failure for error  or  no space  when write!\n",
                   0, 0, 0, 0, 0, 0);
        }

        ulMsgCnt++;
        if(iWrLogFd >= 0)			/* 防止新日志文件创建成功,但写失败的情况 */
        {
            /* 若是有效文件,则关闭之 */
            close(iWrLogFd);
        }
        iWrLogFd=-1;         /*置文件无效标志  */
        return ;
    }
    close(iWrLogFd);
}

/***********************************************************************
* LogFileCRCComp - LOG文件与缓冲区文件CRC对比
*
* RETURNS: 无
*
*/
void LogFileCRCComp(void)
{
    char *pFile;
    int i;
    int nReadLen;
    struct stat statStruct;
    STATUS vxsts;
    uint16_t unCrcFile=0;
    uint16_t unCrcBuf=0;

    pFile=(char *)malloc(LOG_FILE_BUF_SIZE);			/* 申请空间防止出错 */
    if(pFile == NULL)
    {
        assert(FALSE);
    }

    // vxsts=ioctl (iWrLogFd_g, FIOFSTATGET, (int)&statStruct);
    vxsts=fstat(iWrLogFd_g, &statStruct);
    if(vxsts == ERROR)
    {
        assert(FALSE);
    }

    if(statStruct.st_size >= LOG_FILE_BUF_SIZE-10)
    {
        assert(FALSE);
    }

    i=lseek(iWrLogFd_g, 0, SEEK_SET);			/* 需要先定位于开始位置 */
    if(i != 0)
    {
        assert(FALSE);
    }

    nReadLen=read(iWrLogFd_g, pFile, statStruct.st_size);
    if(nReadLen != statStruct.st_size)
    {
        assert(FALSE);
    }

    if(nReadLen != nBackBufLen_g)
    {
        logMsg("日志文件与日志缓冲区长度不同!\n", 0, 0, 0, 0, 0, 0);
    }
    else
    {
        logMsg("日志文件与日志缓冲区长度相同!\n", 0, 0, 0, 0, 0, 0);
    }

    EP_CCITT_CRC16(pFile, nReadLen, unCrcFile);
    EP_CCITT_CRC16(pLogFileBackBuf, nBackBufLen_g, unCrcBuf);

    if(unCrcFile != unCrcBuf)
    {
        logMsg("日志文件与日志缓冲区内容不同!\n", 0, 0, 0, 0, 0, 0);
    }
    else
    {
        logMsg("日志文件与日志缓冲区内容相同!\n", 0, 0, 0, 0, 0, 0);
    }

    free(pFile);
}

/* get the flag if boot frequently.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL LOG_GetBootFlag(void)
{
    return bBootFq_g;
}
