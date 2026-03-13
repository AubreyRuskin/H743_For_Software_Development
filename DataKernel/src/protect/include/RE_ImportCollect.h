/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_ImportCollect.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的Import集中功能的文件头                  */
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

#ifndef RE_IMPORTCOLLECT_H
#define RE_IMPORTCOLLECT_H

#include <vxWorks.h>
#include  "logic.h"

/*为每个任务定义的Import集中的扫描信息  */
typedef  struct
{
    int   iImportCnt;/*汇总的Import个数  */
    void  ** apInputArr;   /*输入值地址数组,动态分配,指针数组,注意成员指向EXPORT_OUTPUT_TYPE类型  */
    EP_ELEM_IO  * aioOutArr;         /* 输出量数组IO 动态分配*/
    int   iCurImportSeqNo;/*当前Import操作序号  */
}  ImportCollect_Scan_Type;


/*功能：Import集中功能扫描
　参数：iTaskNo，所属任务
　返回：无  */
void RE_ImportCollectScan(int  iTaskNo);


/*功能：Import集中功能初始化
　参数：iTaskNo ,所属任务
　返回：无  */
void RE_ImportCollectInit(int  iTaskNo);

/*功能：Import集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_ImportCollectClearSeqNo(int  iTaskNo);

/*　功能：对任务中的Import汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
  　返回：无  */
void  RE_ImportCollectAddNew(void *PartGrpAttrib);


/*功能：Import集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_ImportCollectMemInit(int  iTaskNo);

/*　功能：将Import图元的信息添加到Import集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的Import图元
  　返回：无  */
void RE_ImportCollectAddByImportTuyuan(void *PartGrpAttrib,NODE *pCurScanNode);

/*　功能：将Import的输出IO地址赋值给扫描节点中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的Import图元
  　返回：无  */
void RE_ImportCollectInitImportTuyuanOut(void *PartGrpAttrib,NODE *pCurScanNode);




#endif



