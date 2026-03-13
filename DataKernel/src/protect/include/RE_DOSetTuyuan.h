/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_DOSetTuyuan.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的DO输出集元件的文件头                        */
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
/*         张云       2005.11.9              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#ifndef RE_DOSetTuyuan_H
#define RE_DOSetTuyuan_H

#include <vxWorks.h>
#include   "RE_PublicDataDef.h"
#include    "RE_AllTuyuanDataDef.h"
#include    "stdio_compat.h"
#include    "taskLib.h"
#include   "realdata.h"



/******************DO输出图元的初始化和扫描时的数据节点定义**********************************************/

/*外部输出图元初始化时，保存的外部输出图元数据结构节点，
 该数据节点在完成初始化任务后,为了不占用内寸,会被删除.
 这是因为有字符串，会占用许多内存，*/

typedef  struct
{
    WRAPED_PUBLIC_ELEM_TYPE    PublicElemData;/*公共图元扫描数据   */

    /* *********外部输出图元的私有初始化数据   *********/

    /*输出目的的访问句柄，对DO和指示灯，AI通道都有效  */
    void  *   pvDestHandleArr[MAX_DOSET_INPUT_COUNT];

    /* 外部输出图元的输入源顺序号数组*/
    uint16_t   aunInSourceSeqNoArr[1];

    /*  外部输出图元的输入源在相应图元的输出号*/
    uint8_t    aucInSourceOutputNumArr[1];

    /*  定义输出目的的逻辑标识或名称数组*/
    char   strOutputDestIDArr[MAX_DOSET_INPUT_COUNT][MAX_LOGID_STR_LEN+1];

    int   iDOCnt;/*输出DO个数  */

}  DOSet_Init_Node_Type;




/*DO外部输出图元扫描时的数据结构节点    */
typedef  struct
{
    int   iDOCnt;/*输入个数  */
    EP_ELEM_IO *   pInArr0;
    /*输出目的的访问句柄数组 */
    void  *   pvDestHandleArr[MAX_DOSET_INPUT_COUNT];
    BOOL   bLastValueArr[MAX_DOSET_INPUT_COUNT];

}  DOSet_Scan_Node_Type;



/****图元的逻辑图文件读取初始化函数****************************/
/*
     功能:从文件中读取图元相关数据,此时已读完图元类型字节
          申请初始化数据节点
          供上层程序添加两节点到连表中
          并进行数据节点内容的部分初始化

*/
/****参数：fp,逻辑图文件指针***************************/
/*         ulReadOffsetToBegain,相对于文件起始,读取的文件偏移位置
           pTuyuanInitData,图元节点初始化数据指针
           pRtElemInitNodePointer,返回申请的图元初始化数据节点内存地址

*/
/*   返回值，EP_STATUS */

EP_STATUS   RE_DOSetTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                       TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
                                       NODE ** pRtElemInitNodePointer);


/*
     功能:根据初始化节点数据，申请扫描节点，并进行扫描节点的部分初始化
     返回扫描节点指针

*/
/****参数：
           pRtElemScanNodePointer,返回申请的图元扫描数据节点内存地址
           pElemInitNode,图元初始化数据节点指针
*/
/*   返回值，EP_STATUS */

EP_STATUS   RE_DOSetCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode);


/*
     功能:设置图元的扫描节点的某个输入的指针
*/
/****参数：
           ucInputNum,输入的序号
           pElemIO,输入IO指针
           pScanNode，扫描节点指针
*/
/*   返回值，返回成功与否*/

EP_STATUS   RE_DOSetTuyuanSetInputIO
(uint8_t  ucInputNum,EP_ELEM_IO *  pElemIO,
 NODE  * pScanNode);


/****图元的扫描初始化函数****************************/
/*   功能:进行完全初始化.
           首先进行扫描节点的输入来源指针的获得
           然后调用用户开发的算法图元初始化函数
           最后进行录波,标志,遥信,遥测的初始化
           当连表中的所有图元都完成了初始化操作后,
           上层程序,会释放掉初始化节点的所有内存.
*/
/****参数：pElemInitNode  , 图元的操纵的初始化数据节点指针***************************/
/*           pElemScanNode   ,图元操纵的扫描数据节点指针
           pGrpScanNodeList   图元待访问的逻辑分图扫描数据节点连表
*/
/*   返回值，EP_STATUS */

EP_STATUS   RE_DOSetTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                   LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag);


/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_DOSetTuyuanReadFileOtherInit
(DOSet_Init_Node_Type * pElemInitNodePointer);


/* 扫描多个DO集节点.
 * Para:
 *     PartGrpAttrib, 分图属性.
 * Return:
 *     NONE.
 */
extern void RE_MultiDOSetTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib);

/*    DO输出集目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_DOSetTuyuanScan(NODE *pElemScanNode);

#endif



