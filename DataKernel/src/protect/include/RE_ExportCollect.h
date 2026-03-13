/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_ExportCollect.h                                 1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的端口引出集中功能的文件头                        */
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

#ifndef RE_EXPORTCOLLECT_H
#define RE_EXPORTCOLLECT_H

#include <vxWorks.h>
#include  "logic.h"

/* 端口引出图元的输出值类型 */
typedef  struct
{
    VALUE_TYPE  Val;
    void  *pHdl;
} EXPORT_OUTPUT_TYPE;

/*为每个逻辑图扫描任务定义的事件集中的扫描信息  */
typedef  struct
{
    int   iExportCnt;/*任务中的汇总的Export个数  */
    VALUE_TYPE  ** apInValArr;/*输入值地址数组,动态分配,指针数组  */
    void   ***  apInHdlArr;   /*输入句柄地址数组,动态分配,指针数组,成员是句柄的地址  */
    BOOL   *aRunFlagArr;   /*图元所在逻辑图是否运行标志数组,动态分配  */

    EXPORT_OUTPUT_TYPE  *aOutArr; /*输出值数组,动态分配  */
    int   iCurExportSeqNo;/*当前事件操作序号  */

}  ExportCollect_Scan_Type;



/*功能：任务中端口引出集中功能扫描
　参数：iTaskNo，任务序号
　返回：无  */
void RE_ExportCollectScan(int  iTaskNo);


/*功能：端口引出集中功能初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_ExportCollectInit(int  iTaskNo);


/*功能：端口引出集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_ExportCollectClearSeqNo(int  iTaskNo);

/*　功能：对任务中的端口引出图元汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
  　返回：无  */
void  RE_ExportCollectAddNew(void *PartGrpAttrib);


/*功能：端口引出集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_ExportCollectMemInit(int  iTaskNo);

/*　功能：将端口引出图元的信息添加到端口引出集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的端口引出图元
  　返回：无  */
void RE_ExportCollectAddByExportTuyuan(void *PartGrpAttrib,NODE *pCurScanNode);


/*　功能：将Export的输出IO地址赋值给扫描节点中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的FloatAI图元
  　返回：无  */
void RE_ExportCollectInitExportTuyuanOut(void *PartGrpAttrib,NODE *pCurScanNode);

#endif



