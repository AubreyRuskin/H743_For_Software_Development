/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_LoadLogrpFile.H                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中读取逻辑图文件的相关函数                 */


/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                    */
/*                                                                              */
/*         张云       2002.12.11            创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#ifndef RE_LoadLogrpFile_H
#define RE_LoadLogrpFile_H


#include <vxWorks.h>
#include   "RE_PublicDataDef.h"
#include   "RE_AllTuyuanDataDef.h"




/*  读取逻辑图文件,并创建相应的初始化图元节点,添加到相应的
    初始化节点连表中去
    参数  strLogrpSeqFileName ,逻辑图文件名
          pRtLogrpAttrib,供返回相应的逻辑图属性信息
    返回值  EP_STATUS
 */
EP_STATUS   RE_ReadLogrpFileInit(char   *  strLogrpSeqFileName,
                                 LOGRP_ATTRIB_TYPE  *pRtLogrpAttrib);



/* 创建相应的扫描图元节点,添加到相应的扫描节点连表中去
    参数
          PartGrpScanNodeListArr，扫描节点连表的数组，
                                  此时每个连表内容还为空
          PartGrpInitNodeListArr，初始化节点连表的数组
          nPartGrpSum，创建的分图个数
          PartGrpAttribArr, 分图属性
    返回值  EP_STATUS

   */

EP_STATUS   RE_CreateScanNodeInit
(LIST  * PartGrpScanNodeListArr,
 LIST  * PartGrpInitNodeListArr,
 int   nPartGrpSum,
 PARTGRP_ATTRIB_TYPE *PartGrpAttribArr);





/*  从逻辑图文件中获取所有分图的属性信息
    主要指每个分图的属性和总图的属性信息
    参数   fp  文件指针
           ulReadOffsetToBegain，当前分图内容相对于文件头的偏移
           ulPartGrpCount,所有分图的总数
          pRtLogrpAttrib,供返回相应的逻辑图属性信息

     返回值  EP_STATUS

 */
EP_STATUS   RE_GetAllPartGrpAttrib
(FILE  *fp,uint32_t  ulReadOffsetToBegain
 ,unsigned  long  ulPartGrpCount
 ,LOGRP_ATTRIB_TYPE  *pRtLogrpAttrib);







/*  从逻辑图文件中读取当前保护功能分图初始化信息
    创建并添加图元节点到连表数组成员中


    参数   fp  文件指针
           ulReadOffsetToBegain，当前分图内容相对于文件头的偏移
           nCurNodeListArrDim,当前操作时的图元节点连表数组成员序号，
     返回值  EP_STATUS

 */
EP_STATUS   RE_PartGrpReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                   long    nCurNodeListArrDim);




/*  从逻辑图文件中读取图元信息

    参数   fp  文件指针
           ulReadOffsetToBegain，当前图元内容相对于文件头的偏移,
           包括图元类型字节
           pPartGrpAttrib,该分图的属性的指针，传输分图属性
           nPartGrpNo,所在分图号
     返回值  EP_STATUS
*/
EP_STATUS   RE_TuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                  PARTGRP_ATTRIB_TYPE  *pPartGrpAttrib,long  nPartGrpNo);





/* 在所有初始化接点连表数组中，根据名称来查询获得同名的
    端口输出图元
    参数
          PartGrpInitNodeListArr，初始化节点连表的数组
          nPartGrpCount,初始化连表数组的大小
          strSearchName,代查询的名称
          pRtLastInitOuterOutput,返回的最后一个查找到的相应
                              外部输出图元的初始化节点的指针的指针
                              用于返回测试比较信号类型是否一致
          pnRtFindTuyuanNoInInitListArr，返回的该初始化节点所在的
                              连表在连表数组中的序号，以0开始
          pnRtFindTuyuanNoInInitList，返回的该初始化节点所在的
                              连表中的序号号，以0开始

    返回值  代表查找到的个数

   */

unsigned  long    RE_SearchExternExportDestOuterOutputInInitListArr
(LIST  * PartGrpInitNodeListArr,
 unsigned  long  nPartGrpCount,
 char  *  strSearchName,
 NODE  ** pRtLastInitOuterOutput,
 unsigned long  *pnRtFindTuyuanNoInInitListArr,
 unsigned long  *pnRtFindTuyuanNoInInitList);




/* 在所有扫描接点连表数组中，根据图元的位置查找的
    端口输出图元
    参数
          PartGrpScanNodeListArr，扫描节点连表的数组
          nPartGrpCount,扫描节点连表数组的大小
          nFindTuyuanNoInScanListArr，该扫描节点所在的
                              连表在连表数组中的序号，以0开始
          nFindTuyuanNoInScanList，该扫描节点所在的
                              连表中的序号号，以0开始

    返回值  返回找到的扫描节点，若失败，则返回NULL

   */

NODE  *    RE_SearchExternExportDestOuterOutputInScanListArr
(LIST  * PartGrpScanNodeListArr,
 unsigned  long  nPartGrpCount,
 unsigned long  nFindTuyuanNoInScanListArr,
 unsigned long  nFindTuyuanNoInScanList);






/*     此函数初始化匹配所有分图的所有端口引入的相应来源,
       此时所有初始化节点和扫描节点都已创建,并已进行完初步的创建
        初始化 ,但尚未开始扫描节点的输入来源地址等操作

        参数  nPartGrpScanNodeListArr,所有分图的图元扫描节点的连表
               nPartGrpInitNodeListArr,所有分图的图元初始化节点的连表
              nPartGrpSum ,分图个数,也是可对连表数组操作的维数
        返回值,EP_STATUS ,

*/
EP_STATUS  RE_InitMatchAllExternImportOuterInputTuyuan
(LIST  *   PartGrpScanNodeListArr,
 LIST  *   PartGrpInitNodeListArr,
 int   nPartGrpSum );




/* 获得扫描节点的GET输出IO的函数指针
      参数   pScanNode，待查询的扫描节点
      返回值  该节点的获得输出IO的函数指针
              ，返回为NULL，表示失败
*/
GET_OUT_IO_FUNC_TYPE   RE_GetScanNodeGetOutIOFunc(NODE  *pScanNode);


/* 获得初始化节点的初始化扫描节点InitScanFunc的函数指针
      参数   pInitNode，待查询的初始化节点
      返回值  该初始化节点的InitScanFunc的函数指针
              ，返回为NULL，表示失败
*/
INIT_SCAN_FUNC_TYPE   RE_GetInitNodeInitScanFunc(NODE  *pInitNode);

/* 对扫描链表节点分类.
 * Para:
 *     PartGrpScanNodeListArr, 扫描节点链表.
 *     PartGrpAttrib, 分图属性.
 *     nPartGrpSum, 分图个数.
 * Return:
 *     EP_STATUS, or EP_ERROR.
 */
extern EP_STATUS RE_ClassScanNodeInit(LIST *PartGrpScanNodeListArr,
                                      PARTGRP_ATTRIB_TYPE *PartGrpAttrib, int nPartGrpSum);

#endif



