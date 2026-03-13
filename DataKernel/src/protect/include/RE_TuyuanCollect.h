/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_TuyuanCollect.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的图元集中功能的公共文件头                  */
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

#ifndef RE_TUYUANCOLLECT_H
#define RE_TUYUANCOLLECT_H

#include <vxWorks.h>
#include   "RE_PublicDataDef.h"


/* 对图元集中功能进行初始化.
 * Para:
 *     PartGrpScanNodeListArr, 扫描节点链表.
 *     PartGrpAttrib, 分图属性数组.
 *     nPartGrpSum, 分图个数.
 * Return:
 *     EP_STATUS, or EP_ERROR.
 */
EP_STATUS RE_TyuanCollectInit(LIST *PartGrpScanNodeListArr,
                              PARTGRP_ATTRIB_TYPE *PartGrpAttrib, int nPartGrpSum);

#endif



