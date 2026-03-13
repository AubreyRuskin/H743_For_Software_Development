/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_EventCollect.h                                 1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的事件集中功能的文件头                        */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                    */
/*                                                                              */
/*         张云       2011.7.27              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#ifndef RE_EVENTCOLLECT_H
#define RE_EVENTCOLLECT_H

#include <vxWorks.h>
#include   "RE_PublicDataDef.h"


/*为每个任务定义的事件集中的扫描信息  */
typedef  struct
{

    int   iEventCnt;/*汇总的Event个数  */
    BOOL  **apbInValArr;/*动态分配,指针数组  */
    BOOL  *abOutValArr; /*动态分配,输出值数组  */
    int   *aiEventNumArr;/*动态分配,事件序号数组  */
    NODE  **apEventNodeArr;/*动态分配,事件扫描节点指针数组  */
    int   iCurEventSeqNo;/*当前事件操作序号  */

}  EventCollect_Scan_Type;



/*功能：事件集中功能扫描
　参数：iTaskNo，所属任务号
　返回：无  */
void RE_EventCollectScan(int  iTaskNo);


/*功能：事件集中功能初始化
　参数：iTaskNo，所属任务号
　返回：无  */
void RE_EventCollectInit(int  iTaskNo);


/*功能：事件集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_EventCollectClearSeqNo(int  iTaskNo);

/*　功能：对任务中的事件图元汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
  　返回：无  */
void  RE_EventCollectAddNew(void *PartGrpAttrib);


/*功能：事件集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_EventCollectMemInit(int  iTaskNo);

/*　功能：将事件图元的信息添加到事件集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的指示灯图元
  　返回：无  */
void RE_EventCollectAddByEventTuyuan(void *PartGrpAttrib,NODE *pCurScanNode);


#endif



