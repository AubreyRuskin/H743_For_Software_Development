/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_RelayEngineEntry.H                                    1.0            */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了了保护功能模块中的保护引擎驱动的其他调用函数           */


/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                    */
/*                                                                              */
/*         张云       2002.12.5            创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#ifndef RE_RelayEngineEntry_H
#define RE_RelayEngineEntry_H


#include <vxWorks.h>
#include   "RE_PublicDataDef.h"
#include   "RE_AllTuyuanDataDef.h"
#include  "RE_ListLib.h"
/*    采样节拍驱动关联函数，

      该函数在保护功能初始化时，注册到实时数据模块，
      该函数通过检测逻辑图扫描
      标志，确定是否释放驱动相应信号量，从而驱动逻辑图扫描
      参数：pPara, 任务号地址

      返回值，无
*/

void   RE_SamDriver(void   *pPara);




/*       逻辑图任务时的扫描任务创建入口函数

       参数
              nPartGrpScanNodeListArr,所有分图扫描节点连表数组首地址
              nPartGrpAttribArr,所有分图属性数组
              nScanTaskMsgPointer, 扫描任务相关的数据指针
              nScanTaskNo,扫描的任务号
              nAllPartGrpDims,逻辑图的分图个数
              nScanInterval,该任务扫描周期
       返回值  无
*/

void   RE_tMultiPartGrpScanTask(
    int  nPartGrpScanNodeListArr,
    int  nPartGrpAttribArr,
    int  nScanTaskMsgPointer,
    int  nScanTaskNo,
    int  nAllPartGrpDims,
    int  nScanInterval,int arg1,int hnScanTaskMsgPointer,int hnPartGrpAttribArr,int  hnPartGrpScanNodeListArr);





/*     初始化所有分图的所有扫描节点的相应数据,此时所有初始化节点和扫描节点
        都已创建,
        并已进行完初步的创建初始化 ,此函数完成扫描节点的所有的初始化操作
        此函数完成所有图元的扫描节点的输入来源地址,录波,标志,遥测,遥信等工作

        参数  nPartGrpScanNodeListArr,所有分图的图元扫描节点的连表
               nPartGrpInitNodeListArr,所有分图的图元初始化节点的连表
              nPartGrpSum ,分图个数,也是可对连表数组操作的维数
        返回值,EP_STATUS ,

*/
EP_STATUS  RE_InitAllGrpNode(LIST* nPartGrpScanNodeListArr,
                             LIST* nPartGrpInitNodeListArr,int   nPartGrpSum );






/*     保护功能模块的读取逻辑图文件后的 全局初始化
       主要用于比如信号量等全局变量的初始化
       删除逻辑图图元初始化节点等功能
       该函数在逻辑图读取,完成初始化操作之后,扫描任务驱动之前调用
       参数   pGrpAttrib 逻辑图扫描属性指针

       返回值  无

 */
void   RE_SysInitAfterReadFile(LOGRP_ATTRIB_TYPE  *pGrpAttrib);



/*     保护功能模块的逻辑图扫描任务创建后的操作。
       主要用于采样驱动函数注册,在扫描任务驱动之后调用
       参数   pGrpAttrib 逻辑图扫描属性指针

       返回值  无

 */
void   RE_OpeAfterLogrpScanTaskDrive(LOGRP_ATTRIB_TYPE  *pGrpAttrib);






/*      保护功能模块的清除系统信息
        在读取逻辑图文件之前的 全局初始化 的前面调用
        主要用于清除原来的逻辑图任务的痕迹的工作
        参数  无
        返回值  无

 */
void   RE_CLearSysInfoBeforeReadFile();





/*      保护功能模块的读取逻辑图文件之前的 全局初始化
        主要用于初始化节点连表及其他相关的初始化操作
        该函数在逻辑图文件读取之前调用
        参数  无
        返回值  无

 */
void   RE_SysInitBeforeReadFile();





/*      根据前面读取逻辑图文件,并进行完所有的初始化之后
        创建逻辑图扫描多任务,并驱动扫描任务运行
        参数  pGrpAttrib  逻辑图的扫描属性指针

        返回值  EP_STATUS;

 */
EP_STATUS   LogrpScanTaskDrive(LOGRP_ATTRIB_TYPE  *pGrpAttrib);



/*     功能：设置保护任务创建状态,
       设置为真，表示保护任务已创建
       设置为假，表示保护任务还未创建
*/

void    RE_SetRelayTaskCreateState(BOOL  bCreateState);


/*     功能：读取保护任务创建状态,
       返回为真，表示保护任务已创建
       返回为假，表示保护任务还未创建
*/

BOOL    RE_GetRelayTaskCreateState();



/*扫描某扫描任务的所有端口引入图元  2006-11-6日张云
  参数  nScanTaskNo，扫描任务号
  返回，无
*/
void  RE_TaskAllImportTuyuanScan(int  nScanTaskNo);

/*扫描某扫描任务的所有端口引出图元  2006-11-6日张云
  参数  nScanTaskNo，扫描任务号
  返回，无
*/
void  RE_TaskAllExportTuyuanScan(int  nScanTaskNo);

/*扫描某扫描任务的所有光纵AO输出图元  2006-11-11日张云
  参数  nScanTaskNo，扫描任务号
        ulTaskScanAiCnt,该任务的扫描时的AICNT
  返回，无
*/
void  RE_TaskAllOptAOTuyuanScan(int  nScanTaskNo,uint32_t  ulTaskScanAiCnt);
/* 设置所有保护任务处于复归状态，在复归命令中调用
   参数：无
   返回：无 */
void  RE_SetAllTaskFgSts();

/* 获得保护任务当前是否处于复归状态，逻辑图扫描时访问
   参数， iScanTaskNo  保护任务号
   返回，TRUE，当前该任务有复归命令
         FALSE，当前该任务无复归命令*/
BOOL   RE_GetTaskCurFgSts(int  iScanTaskNo);

/* 清除保护任务的复归状态，逻辑图扫描时访问
   参数， iScanTaskNo  保护任务号
   返回，无*/
void   RE_ClearTaskFgSts(int   iScanTaskNo);

/* 功能：快速保护任务是否已经被驱动 2006-12-3日张云
   参数：无
   返回：TRUE，快速任务已经被驱动
         FALSE，表示快速任务未被驱动 */
BOOL    RE_FastTaskIsDrived();

/***********************************************************************
* RE_StatTaskComsumeTime - 保护任务消耗时间统计计算
*
* RETURNS: 无
*
*/
void  RE_StatTaskComsumeTime(
    int iTaskNo,			/* 任务号 */
    uint32_t ulCurTaskTime				/* 此次保护任务运行消耗的时间 */
);

/***********************************************************************
* RE_IncSetAtSetCnt - 增加任务自动整定定值计数
*
* RETURNS: 无
*
*/
void RE_IncSetAtSetCnt(void);

/***********************************************************************
* RE_JdSetAtSetCnt - 判断任务自动整定定值计数是否非零
*
* RETURNS: 无
*
*/
void RE_JdSetAtSetCnt(void);

/* set the DO to substation.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void setDoDataSub(void);

/* 更新任务DI计数.
 * Para:
 *     pScanTaskMsg, scan logic task.
 *     ulGrpScanDriveInterval, interval.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL RE_UpdateLogDICnt(EP_CHART_MSG *pScanTaskMsg, uint32_t ulGrpScanDriveInterval);

/* 得到DI是否更新过标志 2011-8-5日ZY
 * 参数:pScanTaskMsg 任务信息

 * 返回:
 *     TRUE:DI近期更新过,需要重新刷新
       FALSE;DI长时间未更新,可以不刷新
 */
BOOL RE_GetDIUpdatedFlag(EP_CHART_MSG *pScanTaskMsg);

#endif



