/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_DICollect.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的DI集中功能的文件头                  */
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

#ifndef RE_DICOLLECT_H
#define RE_DICOLLECT_H

#include <vxWorks.h>
#include  "logic.h"

/*为每个任务定义的DI集中的扫描信息  */
typedef  struct
{
    int   iDICnt;/*汇总的DI个数  */

    int    iOutputCnt_4;     /*输出个数/4  */
    int    iOutputCnt_m;      /*输出个数/4后的余数  */
    uint32_t  *  pulScnAiCnt;              /*逻辑图的pChart中的ulScnAiCnt变量的地址  */
    EP_ELEM_IO  * aioOutArr;         /* 输出量数组 动态分配*/
    int  * aDISourceChOffsetArr;     /* 通道距第0号通道的偏移量数组,动态分配  */

    int   iCurDISeqNo;/*当前DI操作序号  */

}  DICollect_Scan_Type;


/*功能：DI集中功能扫描
　参数：iTaskNo，所属任务
　返回：无  */
void RE_DICollectScan(int  iTaskNo);


/*功能：DI集中功能初始化
　参数：iTaskNo ,所属任务
　返回：无  */
void RE_DICollectInit(int  iTaskNo);

/*功能：DI集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_DICollectClearSeqNo(int  iTaskNo);

/*　功能：对任务中的DI汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
          iAddCnt,添加的个数
  　返回：无  */
void  RE_DICollectAddNew(void *PartGrpAttrib,int  iAddCnt);


/*功能：DI集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_DICollectMemInit(int  iTaskNo);

/*　功能：将DI图元的信息添加到DI集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的DI图元
  　返回：无  */
void RE_DICollectAddByDITuyuan(void *PartGrpAttrib,NODE *pCurScanNode);

/*　功能：将DISet图元的信息添加到DI集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的DISet图元
  　返回：无  */
void RE_DICollectAddByDISetTuyuan(void *PartGrpAttrib,NODE *pCurScanNode);

/*　功能：将DI的输出IO地址赋值给扫描节点中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的FloatAI图元
  　返回：无  */
void RE_DICollectInitDITuyuanOut(void *PartGrpAttrib,NODE *pCurScanNode);


/*　功能：将DISet的输出IO地址赋值给扫描节点中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的DISet图元
  　返回：无  */
void RE_DICollectInitDISetTuyuanOut(void *PartGrpAttrib,NODE *pCurScanNode);


#endif



