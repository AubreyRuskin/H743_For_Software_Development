/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_LampCollect.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的指示灯集中功能的文件头                        */
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

#ifndef RE_LAMPCOLLECT_H
#define RE_LAMPCOLLECT_H

#include <vxWorks.h>


/*为每个任务定义的指示灯集中的扫描信息  */
typedef  struct
{

    int   iLampCnt;/*汇总的LAMP个数  */
    BOOL  **apbInValArr;/*输入,动态分配,指针数组  */
    BOOL  * abOutValArr; /*输出,动态分配 ,BOOL值数组 */
    BOOL  **apbSetOutValArr; /* Lamp设置输出,动态分配,指针数组*/
    BOOL  **apbActOutValArr; /* Lamp实际输出,动态分配,指针数组*/
    void  **apvDestHandleArr;/*动态分配  */
    int   iCurLampSeqNo;/*当前Lamp操作序号  */

}  LampCollect_Scan_Type;



/*功能：指示灯集中功能扫描
　参数：iTaskNo，所属任务号
　返回：无  */
void RE_LampCollectScan(int  iTaskNo);


/*功能：指示灯集中功能初始化
　参数：iTaskNo，所属任务
　返回：无  */
void RE_LampCollectInit(int  iTaskNo);


/*功能：指示灯集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_LampCollectClearSeqNo(int  iTaskNo);

/*　功能：对任务中的指示灯图元汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
  　返回：无  */
void  RE_LampCollectAddNew(void *PartGrpAttrib);


/*功能：指示灯集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_LampCollectMemInit(int  iTaskNo);


/*　功能：将指示灯图元的信息添加到指示灯集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的指示灯图元
  　返回：无  */
void RE_LampCollectAddByLampTuyuan(void *PartGrpAttrib,NODE *pCurScanNode);


#endif



