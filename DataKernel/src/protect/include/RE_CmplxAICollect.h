/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_CmplxAICollect.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的ComplexAI集中功能的文件头                  */
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

#ifndef RE_CMPLXAICOLLECT_H
#define RE_CMPLXAICOLLECT_H

#include <vxWorks.h>
#include  "logic.h"

/*为每个任务定义的CmplxAI集中的扫描信息  */
typedef  struct
{
    int   iCmplxAICnt;/*汇总的CmplxAI个数  */

    int    iOutputCnt_4;     /*输出个数/4  */
    int    iOutputCnt_m;      /*输出个数/4后的余数  */
    COMPLEX **  ppxBase;              /*逻辑图的pChart中的pxBase变量的地址  */
    EP_ELEM_IO  * aioOutArr;         /* 输出量数组 动态分配*/
    int  * aAISourceChOffsetArr;     /* 通道距第0号通道的偏移量数组,动态分配  */

    int   iCurCmplxAISeqNo;/*当前CmplxAI操作序号  */

}  CmplxAICollect_Scan_Type;


/*功能：CmplxAI集中功能扫描
　参数：iTaskNo，所属任务
　返回：无  */
void RE_CmplxAICollectScan(int  iTaskNo);


/*功能：CmplxAI集中功能初始化
　参数：iTaskNo ,所属任务
　返回：无  */
void RE_CmplxAICollectInit(int  iTaskNo);

/*功能：CmplxAI集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_CmplxAICollectClearSeqNo(int  iTaskNo);

/*　功能：对任务中的CmplxAI汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
          iAddCnt,添加的个数
  　返回：无  */
void  RE_CmplxAICollectAddNew(void *PartGrpAttrib,int  iAddCnt);



/*功能：CmplxAI集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_CmplxAICollectMemInit(int  iTaskNo);

/*　功能：将CmplxAI图元的信息添加到CmplxAI集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的FloatAI图元
  　返回：无  */
void RE_CmplxAICollectAddByAITuyuan(void *PartGrpAttrib,NODE *pCurScanNode);

/*　功能：将CmplxAISet图元的信息添加到CmplxAI集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的FloatAISet图元
  　返回：无  */
void RE_CmplxAICollectAddByAISetTuyuan(void *PartGrpAttrib,NODE *pCurScanNode);

/*　功能：将CmplxAI的输出IO地址赋值给扫描节点中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的FloatAI图元
  　返回：无  */
void RE_CmplxAICollectInitAITuyuanOut(void *PartGrpAttrib,NODE *pCurScanNode);

/*　功能：将CmplxAISet的输出IO地址赋值给扫描节点中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的FloatAISet图元
  　返回：无  */
void RE_CmplxAICollectInitAISetTuyuanOut(void *PartGrpAttrib,NODE *pCurScanNode);


#endif



