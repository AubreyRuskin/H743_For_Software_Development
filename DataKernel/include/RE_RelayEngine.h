/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_RelayEngine.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了供系统相应模块调用的保护功能模块的调用接口函数           */
/*       该调用接口函数由保护功能模块实现                                       */
/*       在整个系统进行完必要的初始化后,由相应模块调用该接口函数,               */
/*         驱动保护引擎功能运行                                                   */
/*                                                                               */
/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*      作者           日期                    说明                    */
/*                                                                              */
/*      张云       2002.12.2             创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#ifndef RE_RelayEngine_H
#define RE_RelayEngine_H


//#include <vxWorks.h>
#include  "errtest.h"
#include  "logic.h"



/*******************定义EP_Debug_Part的函数类型*****************************/



/****保护功能模块的保护引擎的驱动函数,该函数供系统其他相应模块调用***************/
/*
     功能:从逻辑图顺序化文件中解析该文件,驱动保护引擎功能

    参数：     strLogrpSeqFileName,逻辑图顺序化文件的文件名


    返回值，    EP_STATUS

                EP_SUCCESS,驱动保护引擎成功
                EP_SYS_ERR,其他错误导致的失败.
*/
EP_STATUS   Relay_Engine_Activate(uint8_t   *  strLogrpSeqFileName);



/************************更新算法元件表的函数******************************/
/*
           功能：更新算法元件表的相关信息

           参数：  pSuanfaElemArr,算法表首地址
                   nSuanfaElemMapCount，算法表个数
                   pSuanfaDebugEntryFunc，算法调试入口函数
           返回值   EP_STATUS ；成功与否
*/
EP_STATUS   RE_Refresh_SuanfaTable_Info(EP_EXT_ELEM_MAP   *  pSuanfaElemArr,
                                        uint32_t  nSuanfaElemMapCount,
                                        EP_DEBUG_PART_FUNC_TYPE    pSuanfaDebugEntryFunc);



/******************根据逻辑图扫描任务号获得扫描节拍的函数******************************/
/*
           功能：根据逻辑图任务号，获得该任务的AI扫描节拍

           参数：   ulScanTaskNo，扫描任务号

           返回值   该任务号的逻辑图的AI扫描节拍
*/
int  RE_Lgc_Scan_Interval(uint32_t ulScanTaskNo);



/******************获得保护任务工作状态，供看门狗检测使用************************/
/*      功能，获得保护任务运行状态，
        参数，无
        返回：TRUE，表示保护任务工作正常
              FALSE，表示保护任务出现异常
*/
BOOL     RE_Get_Relay_Task_Run_State(
    uint8_t *pnRelayTaskNo,
    uint8_t *pnTskSts
);

typedef   struct
{
    /*逻辑图任务每周期消耗的时间 2006-9-22 张云*/
    int   iTaskNo;/*该任务号  */
    uint32_t  ulCurTimePerPeriod;/*当前最新周期运行时间　微秒*/
    uint32_t  ulMinTimePerPeriod;/*最小每周期运行时间　微秒*/
    uint32_t  ulMaxTimePerPeriod;/*最大每周期运行时间　微秒*/
    uint32_t  ulAverageTimePerPeriod;/*平均每周期运行时间　微秒*/

}  RE_TASK_COMSUME_RESOURCE_TYPE;


/* 获得运行的保护任务个数  2006-9-22日 供MMI调用，获得保护任务资源消耗功能使用
   参数，piRtRunTaskNoArr,调用者传来的供返回运行保护任务号的数组基址，数组要足够大，比如8或16，由调用者提供该数组
   返回，返回运行的保护任务个数
 */
int    RE_GetRunTaskCnt(int  *piRtRunTaskNoArr);


/* 获得某运行的保护任务资源消耗2006-9-22日 供MMI调用，获得保护任务资源消耗功能使用
   参数，iRunTaskNo，待访问的保护任务号
         pRtTaskResource，供返回该任务的资源消耗，由调用者提供
   返回，无 */
void   RE_GetRunTaskConsumeResource(int  iRunTaskNo,RE_TASK_COMSUME_RESOURCE_TYPE  *pRtTaskResource);


/*      功能，获得任务状态是否挂起，
        参数，iTaskId，任务ID
        返回：TRUE，表示保护任务挂起
              FALSE，表示保护任务未挂起
*/
BOOL     RE_TaskIsSuspended(int  iTaskId);
#endif



