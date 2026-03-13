/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_DOCollect.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的DO集中功能的文件头                        */
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

#ifndef RE_DOCOLLECT_H
#define RE_DOCOLLECT_H

#include <vxWorks.h>


/*为每个任务定义的DO集中的扫描信息  */
typedef  struct
{

    int   iDOCnt;/*汇总的ＤＯ个数  */
    int  **apbInValArr;/*输入,动态分配,指针数组  */
    int  * abOutValArr; /* 输出,动态分配,int值数组  */
    int  **apbSetOutValArr; /* DO设置输出,动态分配,指针数组*/
    int  **apbActOutValArr; /* DO实际输出,动态分配,指针数组*/
    uint8_t *aucInAttributeArr;/*输入值属性数组,动态分配*/
    void  **apvDestHandleArr;/*动态分配  */
    int   iCurDOSeqNo;/*当前DO操作序号  */

}  DOCollect_Scan_Type;



/*功能：DO集中功能扫描
　参数：iTaskNo，所属任务
　返回：无  */
void RE_DOCollectScan(int  iTaskNo);


/*功能：DO集中功能初始化
　参数：iTaskNo ,所属任务
　返回：无  */
void RE_DOCollectInit(int  iTaskNo);



/*功能：DO集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_DOCollectClearSeqNo(int  iTaskNo);


/*　功能：对任务中的DO汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
          iAddCnt,添加的个数
  　返回：无  */
void  RE_DOCollectAddNew(void *PartGrpAttrib,int  iAddCnt);


/*功能：DO集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_DOCollectMemInit(int  iTaskNo);

/*　功能：将DO图元的信息添加到ＤＯ集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的ＤＯ图元
  　返回：无  */
void RE_DOCollectAddByDOTuyuan(void *PartGrpAttrib,NODE *pCurScanNode);

/*　功能：将DOSet图元的信息添加到ＤＯ集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的DOSet图元
  　返回：无  */
void RE_DOCollectAddByDOSetTuyuan(void *PartGrpAttrib,NODE *pCurScanNode);



#endif



