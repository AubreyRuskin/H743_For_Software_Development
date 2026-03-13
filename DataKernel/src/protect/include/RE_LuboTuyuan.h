/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_LuboTuyuan.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的录波元件的文件头                        */
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
/*         张云       2002.12.12              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#ifndef RE_LuboTuyuan_H
#define RE_LuboTuyuan_H


#include <vxWorks.h>
#include   "RE_PublicDataDef.h"
#include    "RE_AllTuyuanDataDef.h"
#include    "stdio_compat.h"




/******************录波图元的初始化和扫描时的数据节点定义**********************************************/


/*录波图元初始化时，保存的录波图元数据结构节点，
 该数据节点在完成初始化任务后,为了不占用内寸,会被删除.
 这是因为有字符串，会占用许多内存，*/

typedef  struct
{
    WRAPED_PUBLIC_ELEM_TYPE    PublicElemData;/*公共图元扫描数据   */

    /* *********录波图元的私有初始化数据   *********/


    /*录波启停操作类型  */
    uint8_t   ucLuboStartStopType;

    /* 录波启动触发类型 ,对录波启动和启停类型有效*/
    uint8_t   ucLuboStartTriggerType;

    /* 录波停止触发类型 ,对录波停止和启停类型有效*/
    uint8_t   ucLuboStopTriggerType;

    /* 录波启动信息,对录波启动和启停类型有效 */
    SCI_LUBO_START_INFO_TYPE   LuboStartInfo;

    /* 录波图元的输入信号类型数组*/
    uint8_t   aucSourceSignalTypeArr[1];

    /* 录波图元的输入源顺序号数组*/
    uint16_t   aunInSourceSeqNoArr[1];

    /*  录波图元的输入源在相应图元的输出号*/
    uint8_t    aucInSourceOutputNumArr[1];

}  Lubo_Init_Node_Type;



/*启动型录波图元扫描时的数据结构节点    */
typedef  struct
{


    EP_ELEM_IO *   pInArr0;/*或门输入数据指针0  */

    /* 录波启动触发类型 ,对录波启动和启停类型有效*/
    uint8_t   ucLuboStartTriggerType;

    /*  上次扫描输入触发值  */
    BOOL   bLastScanInputValue;

    /* 录波启动信息,对录波启动和启停类型有效 */
    SCI_LUBO_START_INFO_TYPE   LuboStartInfo;


    EP_CHART_MSG  *pChartMsg;  /*逻辑图信息*/



}  OnlyStartLubo_Scan_Node_Type;



/*停止型录波图元扫描时的数据结构节点    */
typedef  struct
{


    EP_ELEM_IO *   pInArr0;/*或门输入数据指针0  */

    /* 录波停止触发类型 ,对录波停止和启停类型有效*/
    uint8_t   ucLuboStopTriggerType;

    /*  上次扫描输入触发值  */
    BOOL   bLastScanInputValue;


    EP_CHART_MSG  *pChartMsg;  /*逻辑图信息*/


}  OnlyStopLubo_Scan_Node_Type;



/*启停型录波图元扫描时的数据结构节点    */
typedef  struct
{


    EP_ELEM_IO *   pInArr0;/*或门输入数据指针0  */

    /*  上次扫描输入触发值  */
    BOOL   bLastScanInputValue;

    /* 录波启动触发类型 ,对录波启动和启停类型有效*/
    uint8_t   ucLuboStartTriggerType;


    /* 录波停止触发类型 ,对录波停止和启停类型有效*/
    uint8_t   ucLuboStopTriggerType;



    /* 录波启动信息,对录波启动和启停类型有效 */
    SCI_LUBO_START_INFO_TYPE   LuboStartInfo;


    EP_CHART_MSG  *pChartMsg;  /*逻辑图信息*/



}  StartStopLubo_Scan_Node_Type;



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

EP_STATUS   RE_LuboTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
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

EP_STATUS   RE_LuboCreateScanNodeInit(
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

EP_STATUS   RE_LuboTuyuanSetInputIO
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

EP_STATUS   RE_LuboTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                  LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag);









/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_LuboTuyuanReadFileOtherInit
(Lubo_Init_Node_Type * pElemInitNodePointer);






/*    启动型录波图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_OnlyStartLuboTuyuanScan(NODE *pElemScanNode);



/*    停止型录波图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_OnlyStopLuboTuyuanScan(NODE *pElemScanNode);




/*    启动停止类型的录波图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_StartStopLuboTuyuanScan(NODE *pElemScanNode);

/* 扫描多个启动型录波节点.
 * Para:
 *     PartGrpAttrib, 分图属性.
 * Return:
 *     NONE.
 */
extern void RE_MultiOnlyStartLuboTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib);

/* 扫描多个停止型录波节点.
 * Para:
 *     PartGrpAttrib, 分图属性.
 * Return:
 *     NONE.
 */
extern void RE_MultiOnlyStopLuboTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib);

/* 扫描多个启动停止型录波节点.
 * Para:
 *     PartGrpAttrib, 分图属性.
 * Return:
 *     NONE.
 */
extern void RE_MultiStartStopLuboTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib);

#endif



