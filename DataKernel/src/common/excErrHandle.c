/* excErrHandle.c - subroutine library for handling the system exception */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01b, 16jul07, dy realized on platform edp03.
01a, 15jul07, hcj first created.
*/

/*
DESCRIPTION
This module includes subroutine library for handling the system exception.
INCLUDES: excErrHandle.h
*/

/* includes */

#include "excErrHandle.h"
// #include "config04.h"
#include "datetime.h"
#include "taskLib.h"
#include "bspinterface.h"

/* defines */

/* globals */

extern EP_DATE_TIME EP_dtRebootTime_g;
extern FUNCPTR _func_excBaseHook;

/* statics */

static char *pExcErrBuf = NULL; 		/* 装置异常内容存贮指针 */
static char *pErrOffsetbuf = NULL; 				/* 装置异常内容指针偏移位置 */

/* global functions */

/* static functions */

/***********************************************************************
* Exc_Init_And_SaveToLog - 系统错误记入日志中
*
* RETURNS: 无
*
*/
static void Exc_Init_And_SaveToLog();

/***********************************************************************
* Exc_ProgHandler - 平台异常处理
*
* RETURNS: 无
*
*/
// static void Exc_ProgHandler(
//     int vecNumIn,
//     ESFPPC *pEsf,		/* 异常信息 */
//     REG_SET *pRegs,
//     EXC_INFO *excInfo
// );

/* functions */

/**********存贮异常信息数据格式
1B.   异常条目个数
1B.   所有异常信息的数据长度(目前暂定为1项异常)

4B.   4个标志字节(每个异常条目信息begin),
1B.   每个异常条目来源:  1为任务出错，2为中断出错
1B+lengthB.   任务名称长度(length)+任务名称,假如任务为空，则长度为零
4B.   (exception vector number)
4B.   (PC 值)
4B.   4个标志字节
1B,   每个异常CRC  (累加取反)(每个异常条目信息end)
*/

/***********************************************************************
* Exc_ProgHandler - 平台异常处理
*
* RETURNS: 无
*
*/
// static void Exc_ProgHandler(
//     int vecNumIn,
//     ESFPPC *pEsf,		/* 异常信息 */
//     REG_SET *pRegs,
//     EXC_INFO *excInfo
// )
// {
//     /* 程序执行及浮点异常处理 */
//     unsigned int pcValue = 0;
//     int i;
//     uint8_t sum=0;
//     int vecNum = pEsf->vecOffset;			/* exception vector number */
//     char *pTaskName = NULL;
//     char taskNameAry[] = "Unknown task";
//     static unsigned int pcValueLst=0xFFFFFFFF;
//     static int vecNumLst=0x7FFFFFFF;

//     pcValue = pEsf->regSet.pc;
//     if((pcValueLst == pcValue) && (vecNumLst == vecNum))
//     {
//         /* 同一异常进入一次 */
//         return;
//     }

//     pcValueLst=pcValue;
//     vecNumLst=vecNum;

//     if(!INT_CONTEXT ())
//     {
//         /* 不是中断 ，获取任务名称 */
//         pTaskName = taskName(taskIdSelf());
//         if(NULL == pTaskName)
//         {
//             pTaskName = taskNameAry;
//         }
//     }

//     if((*pExcErrBuf) >= 1)			/* 假如目前已记录1条异常信息 */
//         return;

//     (*pExcErrBuf) += 1;			/* 异常条目个数+1 */
//     pErrOffsetbuf += 1;					/* 异常条目个数 */
//     pErrOffsetbuf += 1;							/* 所有异常信息的数据长度 */

//     *pErrOffsetbuf = 0x5A;
//     *(pErrOffsetbuf+1) = 0xA5;
//     *(pErrOffsetbuf+2) = 0x5A;
//     *(pErrOffsetbuf+3) = 0xA5;

//     pErrOffsetbuf+=4; 			/* 4个保留字节 */
//     if(pTaskName)
//     {
//         /* 任务出错 */
//         *pErrOffsetbuf++=1; 				/* 来源，任务出错 */

//         *pErrOffsetbuf++=strlen(pTaskName); 				/* 任务名称长度 */
//         if(strlen(pTaskName)>0)
//         {
//             memcpy(pErrOffsetbuf,pTaskName,strlen(pTaskName));
//             pErrOffsetbuf += strlen(pTaskName);
//         }
//     }
//     else
//     {
//         *pErrOffsetbuf++=2; 	/* 来源，中断出错 */
//         *pErrOffsetbuf++=0; 				/* 任务名称长度为零 */
//     }
//     *pErrOffsetbuf++=LL8(vecNum);		/* 向量号 */
//     *pErrOffsetbuf++=LH8(vecNum);
//     *pErrOffsetbuf++=HL8(vecNum);
//     *pErrOffsetbuf++=HH8(vecNum);
//     *pErrOffsetbuf++=LL8(pcValue);			/* PC指针 */
//     *pErrOffsetbuf++=LH8(pcValue);
//     *pErrOffsetbuf++=HL8(pcValue);
//     *pErrOffsetbuf++=HH8(pcValue);

//     *pErrOffsetbuf = 0x5A;
//     *(pErrOffsetbuf+1) = 0xA5;
//     *(pErrOffsetbuf+2) = 0x5A;
//     *(pErrOffsetbuf+3) = 0xA5;

//     pErrOffsetbuf += 4;		/* 4个保留字节 */
//     for(i=2; i<(pErrOffsetbuf-pExcErrBuf); i++)
//     {
//         sum += pExcErrBuf[i];
//     }
//     *pErrOffsetbuf++=(uint8_t)~sum; 		/* 每一项异常的CRC */
//     *(pExcErrBuf+1)=(pErrOffsetbuf-pExcErrBuf-2); 		/* 一项异常的长度 */

//     EDPreboot(REBOOT_EXCEP);
// }

/***********************************************************************
* Exc_SysregExcHandle - 平台异常处理函数挂接
*
* RETURNS: 无
*
*/
void Exc_SysregExcHandle()
{
    /* 平台异常处理函数挂接 */
    /* 初始化 */
    // Exc_Init_And_SaveToLog();
    // _func_excBaseHook = (FUNCPTR) Exc_ProgHandler;
}

/***********************************************************************
* Exc_Init_And_SaveToLog - 系统错误记入日志中
*
* RETURNS: 无
*
*/
// static void Exc_Init_And_SaveToLog()
// {
//     /* 系统错误记入日志中 */
//     char excErrStr[256]= {0}; 			/* 针对于存贮异常信息内存 */
//     BOOL bInvalid = FALSE;
//     char *pTemp=NULL;
//     int i;
//     uint8_t sum=0;
//     int excLen=0; 		/* 每一个异常信息长度 */
//     unsigned int pcValue=0;

//     /* 初始化 */
//     pExcErrBuf=(char *)EXC_ERROR_HANDLE_POS;
//     pErrOffsetbuf = pExcErrBuf;
//     /* 解析异常信息并存入日志 */

//     if (((*pExcErrBuf) == 1)
//             && (*(pExcErrBuf+2) == 0x5A)
//             && (*(pExcErrBuf+3) == 0xA5)
//             && (*(pExcErrBuf+4) == 0x5A)
//             && (*(pExcErrBuf+5) == 0xA5))
//     {
//         memcpy(excErrStr, pExcErrBuf, 255);
//         excLen=*(excErrStr+1);
//         for(i=2; i<(excLen+2-1); i++)
//         {
//             /* 最后一个是校验码 */
//             sum += excErrStr[i];
//         }
//         if (((uint8_t)(~sum) == *(excErrStr+excLen+2-1))
//                 && (*(excErrStr+excLen-3) == 0x5A)
//                 && (*(excErrStr+excLen-2) == 0xA5)
//                 && (*(excErrStr+excLen-1) == 0x5A)
//                 && (*(excErrStr+excLen) == 0xA5)) 			/* 存贮的数据完整有效 */
//             bInvalid = TRUE ;
//     }

//     if (bInvalid == TRUE)
//     {
//         pTemp = excErrStr;
//     }

//     if (pTemp)
//     {
//         /* 解析异常信息 */
//         uint8_t errSource=0;
//         char logStr[256]= {0};			/* 记入日值的字符串 */
//         char excTaskName[256]= {0};				/* 异常来源任务名 */
//         int vecNum=0;			/* exception vector number */

//         pTemp+=1; 					/* 异常条目个数 */
//         pTemp+=1; 			/* 所有异常信息的数据长度 */
//         pTemp+=4;							/* 4个标志字节 */
//         errSource=*pTemp++; 					/* 异常来源 */
//         if((*pTemp) != 0)
//         {
//             int namelen = *pTemp;
//             memcpy(excTaskName,pTemp+1,namelen); /*异常来源任务名*/
//             memset(&excTaskName[namelen],0,1);
//             pTemp += (1+namelen);
//         }
//         else
//         {
//             if(ENG_MODE == 0)
//             {
//                 sprintf(excTaskName,"%s","中断");
//             }
//             else if(ENG_MODE == 1)
//             {
//                 sprintf(excTaskName,"%s","Interrupt");
//             }
//             pTemp+=1;
//         }
//         vecNum=U8_TO_U32(pTemp[3],pTemp[2],pTemp[1],pTemp[0]);
//         pTemp+=4 ;
//         pcValue=U8_TO_U32(pTemp[3],pTemp[2],pTemp[1],pTemp[0]);
//         pTemp+=4;
//         pTemp+=1; 		/* CRC */

//         if (errSource == 1)
//         {
//             if(ENG_MODE ==1)
//                 sprintf(logStr, "%s error, Exc NO. is %x, PC is 0x%8.8x\n", (char *)excTaskName, vecNum, pcValue);
//             else if(ENG_MODE ==0)
//                 sprintf(logStr, "异常复位: 任务: %s, 异常号: 0x%x, PC: 0x%8.8x\n", (char *)excTaskName, vecNum, pcValue);
//         }
//         else if (errSource == 2)
//         {
//             if (ENG_MODE == 1)
//                 sprintf(logStr, "Interrupt error, Exc NO. is %d, PC is 0x%8.8x.\n",
//                         vecNum, pcValue);
//             else if (ENG_MODE == 0)
//                 sprintf(logStr, "异常复位: 中断, 异常号: 0x%x, PC: 0x%8.8x.\n",
//                         vecNum, pcValue);
//         }
//         LOG_Write(LOG_KERNEL, (const uint8_t *)logStr, NULL);
//     }

//     memset(pExcErrBuf,0,256);				/* 清存贮异常信息的内存区 */
// }

/* 调用sysToMonitor实现CPU重启时信息记录格式
 * 4B: 记录头部(0x5A 0xA5 0x5A 0xA5)
 * 1B: 所有信息的数据长度
 * 1B: 调用sysToMonitor函数任务名称长度，如任务为空，则长度为0
 * lentgh: 调用sysToMonitor函数任务名称
 * 1B: 区分主动调用和异常被动调用标志(0x5A: 主动调用, 0xA5: 异常调用)
 * 4B: 记录尾部(0x5A 0xA5 0x5A 0xA5)
 * 1B: 求和取反校验码
 * 设计要点:
 * 1、初始化时记录异常调用, 任务名称记录为"未知任务或中断", 防止其它任务调用sysToMonitor重启.
 */

/* write the low address memory when calling sysToMonitor.
 * Para:
 *     callType, call type,
 *     REBOOT_UNKNOWN: initialize calling;
 *     REBOOT_ACTIVE: normal calling;
 *     REBOOT_EXCEP: exception calling.
 * Return:
 *     OK, ERROR.
 */
BOOL Exc_WrRebootInfo(int32_t callType)
{
    // uint8_t *pHeadPos;
    // uint8_t *pTemp;
    // uint8_t *pLenPos;
    // uint8_t *pName = NULL;
    // uint8_t len;
    // uint8_t *p;
    // uint8_t sum = 0;

    // if (callType == REBOOT_UNKNOWN)
    // {
    //     pName = "未知任务或中断";
    // }
    // else if ((callType == REBOOT_ACTIVE) || (callType == REBOOT_EXCEP))
    // {
    //     if (!INT_CONTEXT ())
    //     {
    //         /* called in task. */
    //         pName = taskName(taskIdSelf());
    //         if (NULL == pName)
    //         {
    //             pName = "未知任务";
    //         }
    //     }
    //     else
    //     {
    //         pName = "中断";
    //     }
    // }
    // else
    // {
    //     return ERROR;
    // }

    // pHeadPos = (char *)REBOOT_INFO_SAVE_POS;
    // pTemp = pHeadPos;
    // *pTemp++ = 0x5A;
    // *pTemp++ = 0xA5;
    // *pTemp++ = 0x5A;
    // *pTemp++ = 0xA5;
    // pLenPos = pTemp++;	/* save length. */

    // len = strlen(pName);
    // *pTemp++ = len;
    // if (len>0)
    // {
    //     memcpy(pTemp, pName, len);
    //     pTemp += len;
    // }

    // if ((callType == REBOOT_UNKNOWN) || (callType == REBOOT_EXCEP))
    // {
    //     *pTemp++ = 0xA5;
    // }
    // else if (callType == REBOOT_ACTIVE)
    // {
    //     *pTemp++ = 0x5A;
    // }
    // else
    // {
    //     return ERROR;
    // }

    // *pTemp++ = 0x5A;
    // *pTemp++ = 0xA5;
    // *pTemp++ = 0x5A;
    // *pTemp++ = 0xA5;
    // *pLenPos = pTemp - pHeadPos+1;
    // for (p = pHeadPos; p<pTemp; p++)
    // {
    //     sum += *p;
    // }
    // *pTemp++ = (uint8_t)~sum;

    return OK;
}

/* read the low address memory when power up.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void Exc_RdRebootInfo(void)
{
//     uint8_t *pTemp;
//     uint8_t RebootInfoStr[256] = {0};
//     uint8_t len;
//     uint8_t sum = 0;
//     uint8_t i;
//     uint8_t rebootName[256] = {0};
//     BOOL bSts = FALSE;
//     uint8_t logStr[256] = {0};
//     int16_t BootInfo = 0;

//     pTemp = (char *)REBOOT_INFO_SAVE_POS;
//     if ((pTemp[0] != 0x5A) || (pTemp[1] != 0xA5) || (pTemp[2] != 0x5A) || (pTemp[3] != 0xA5))
//     {
//         sprintf(rebootName, "%s", "未知任务或中断");
//         bSts = TRUE;

//         goto ret;
//     }
//     memcpy(RebootInfoStr, pTemp, 255);
//     pTemp = RebootInfoStr;
//     len = pTemp[4];

//     if (len>255)
//     {
//         sprintf(rebootName, "%s", "未知任务或中断");
//         bSts = TRUE;

//         goto ret;
//     }

//     for (i = 0; i<len-1; i++)
//     {
//         sum += pTemp[i];
//     }

//     if ((pTemp[len-1] != (uint8_t)~sum)
//             || (pTemp[len-5] != 0x5A) || (pTemp[len-4] != 0xA5) || (pTemp[len-3] != 0x5A) || (pTemp[len-2] != 0xA5))
//     {
//         sprintf(rebootName, "%s", "未知任务或中断");
//         bSts = TRUE;

//         goto ret;
//     }
//     pTemp += 5;
//     len = *pTemp++;
//     if (len)
//     {
//         memcpy(rebootName, pTemp, len); 	/* reboot source name. */
//         memset(&rebootName[len], 0, 1);
//         pTemp += len;
//     }
//     else
//     {
//         sprintf(rebootName, "%s", "未知任务或中断");
//     }

//     if ((*pTemp) == 0xA5)
//     {
//         bSts = TRUE;
//     }

// ret:
//     if (bSts)
//     {
//         sprintf(logStr, "reboot复位: %s.\n", (char *)rebootName);
//     }
//     else
//     {
//         sprintf(logStr, "主动复位: %s.\n", (char *)rebootName);
//     }
//     LOG_Write(LOG_KERNEL, (const uint8_t *)logStr, &EP_dtRebootTime_g);

//     /* 读取BSP传递重启状态
//      */
//     BootInfo = Get_Boot_Context();
//     if (BootInfo == 1)
//     {
//         /* 中断中发生 */
//         sprintf(rebootName, "BSP记录中断中调用reboot复位,中断号为%d\n", Get_Boot_Info());
//         LOG_Write(LOG_KERNEL, (const uint8_t *)rebootName, &EP_dtRebootTime_g);
//     }
//     else if (BootInfo == 2)
//     {
//         /* 任务中发生 */
//         sprintf(rebootName, "BSP记录任务中调用reboot复位,任务ID为%x\n", Get_Boot_Info());
//         LOG_Write(LOG_KERNEL, (const uint8_t *)rebootName, &EP_dtRebootTime_g);
//     }

    return;
}

/* includes */
#include "vxWorks.h"
#include "string_compat.h"
// #include "regs.h"
#include "stdio_compat.h"

// #include "sysSymTbl.h"
// #include "errnoLib.h"
// #include "taskArchLib.h"
#include "intLib.h"			/* intLock/intUnlock */
// #include "private/funcBindP.h"
// #include "private/taskLibP.h"
// #include "private/kernelLibP.h"
// #include "private/eventLibP.h"		/* eventTaskShow () */

#define MAX_DSP_TASKS 500  /* 最多任务个数 */

/* 记录任务状态 */
void Exc_RecTaskInfo(void)
{
    // int idList[MAX_DSP_TASKS];	/* list of active IDs */
    // int	nTasks;
    // int	ix;
    // TASK_DESC td;
    // REG_SET regSet;
    // char statusString[10];
    // char TempInfo[TEMP_INFO_MAX_LEN];

    // nTasks = taskIdListGet (idList, NELEMENTS (idList));
    // taskIdListSort (idList, nTasks);

    // for (ix = 0; ix < nTasks; ++ix)
    // {
    //     if (taskInfoGet (idList [ix], &td) == OK)
    //     {
    //         taskStatusString (td.td_id, statusString);
    //         taskRegsGet (td.td_id, &regSet);

    //         sprintf (TempInfo, "%-11.11s %8x %3d %-10.10s %8x %8x %7x %5u\n",
    //                  td.td_name,
    //                  td.td_id,
    //                  td.td_priority,
    //                  statusString,
    //                  regSet.pc,
    //                  (int)regSet.spReg,
    //                  td.td_errorStatus,
    //                  td.td_delay);
    //     }
    //     else
    //     {
    //         sprintf (TempInfo, "异常任务: %-11.11s %8x %3d %7x %5u\n",
    //                  td.td_name,
    //                  td.td_id,
    //                  td.td_priority,
    //                  td.td_errorStatus,
    //                  td.td_delay);
    //     }

    //     LOG_Write(LOG_KERNEL, (const uint8_t *)TempInfo, NULL);
    // }
}

/* 记录启动时间.
 * Para:
 *     ulSn, 序号.
 * Return:
 *     NONE.
 */
void Exc_RecStartTm(int32_t ulSn)
{
    // uint8_t *pInfoOffsetbuf = NULL;
    // uint64_t ullValue1;
    // uint32_t ulResult;
    // UINT32 ulBaseH, ulBaseL;
    // static uint32_t ulAddrCnt = 0;

    // pInfoOffsetbuf = (uint8_t *)START_INFO_SAVE_POS+8*ulAddrCnt;

    // sysInputFreq_g = sysInputFreqGet();

    // vxTimeBaseGet(&ulBaseH, &ulBaseL);
    // ullValue1 = (((uint64_t)ulBaseH) << 32)+(uint64_t)ulBaseL;

    // ullValue1 = ullValue1*1000/(uint64_t)CALC_FREQ;	 /* 转换为微秒计数 */
    // ulResult = (uint32_t)(ullValue1&0xFFFFFFFF); /* 取低32位, 即32位微秒计数器的值 */

    // *pInfoOffsetbuf++ = LL8(ulSn);		/* 运行时间 */
    // *pInfoOffsetbuf++ = LH8(ulSn);
    // *pInfoOffsetbuf++ = HL8(ulSn);
    // *pInfoOffsetbuf++ = HH8(ulSn);

    // *pInfoOffsetbuf++ = LL8(ulResult);		/* 运行时间 */
    // *pInfoOffsetbuf++ = LH8(ulResult);
    // *pInfoOffsetbuf++ = HL8(ulResult);
    // *pInfoOffsetbuf++ = HH8(ulResult);
    // ulAddrCnt++;
}