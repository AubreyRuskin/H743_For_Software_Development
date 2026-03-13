/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_OuterInputTuyuan.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的定植输入元件的文件头                        */
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
/*         张云       2005.10.24              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#ifndef RE_SettingTuyuan_H
#define RE_SettingTuyuan_H


#include <vxWorks.h>
#include   "RE_PublicDataDef.h"
#include    "RE_AllTuyuanDataDef.h"
#include    "stdio_compat.h"




/******************定植输入图元的初始化和扫描时的数据节点定义**********************************************/

/*外部输入图元初始化时，保存的外部输入图元数据结构节点，
 该数据节点在完成初始化任务后,为了不占用内寸,会被删除.
 这是因为有字符串，会占用许多内存，*/

typedef  struct
{
    WRAPED_PUBLIC_ELEM_TYPE    PublicElemData;/*公共图元扫描数据   */

    /* *********外部输入图元的私有初始化数据   *********/

    uint8_t     ucSignalSourceType;/* 信号来源类型  */

    uint8_t     ucSignalValueType;/*  信号值的类型，定义0为实数，1为复数，2为32位有符号整数，
                                      3为32位无符号整数，4为BOOL量  */

    /*  定义输入源的逻辑标识，对AI，DI，定植,端口引入有意义*/
    char   strInputSourceID[MAX_LOGID_STR_LEN+1];
    /*输入来源为定植时的定植信息  */
    SCI_SETTING_INFO_TYPE   InputSourceSettingInfo;

}  Setting_Init_Node_Type;



/*常数输入图元扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOut;

}  ConstValueOuterInput_Scan_Node_Type;



/*定值输入图元扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    /*输入来源为定植时的定植信息  */

    BOOL   *pbCurScanDingzhiRreshFlag;/*当前扫描需要刷新定植标志的指针
                                        主要用于需获得定植的图元才有意义
                                        比如时间继电器  这在创建图元
                                        数据节点时会传给该指针*/

    SCI_SETTING_INFO_TYPE   InputSourceSettingInfo;

    uint8_t     ucSignalValueType;/*  信号值的类型，定义0为实数，1为复数，2为32位有符号整数，
                                      3为32位无符号整数，4为BOOL量  */

    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOut;

}  DingzhiOuterInput_Scan_Node_Type;


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

EP_STATUS   RE_SettingTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
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

EP_STATUS   RE_SettingCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode);


/*
     功能:获得图元的扫描节点的某个输出的指针

*/
/****参数：
           unOutNum,输出的序号
           pScanNode，扫描节点指针
*/
/*   返回值，相应序号的输出IO的指针 ，若失败，则返回NULL*/

EP_ELEM_IO *  RE_SettingTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode);


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

EP_STATUS   RE_SettingTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                     LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag);


/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_SettingTuyuanReadFileOtherInit
(Setting_Init_Node_Type * pElemInitNodePointer);


/*　当外部输入来源为常数时,读取常数,并进行相应的初始化
    参数   fp  ,文件当前指针
           pElemInitNode,初始化节点指针
*/
BOOL   RE_SettingTuyuanGetConstValueSourceFileReadInit(
    FILE  *fp,
    Setting_Init_Node_Type *  pElemInitNode
);


/*　当外部输入来源为定植时,读取逻辑标识,并进行相应的初始化
    参数   fp  ,文件当前指针
           pElemInitNode,初始化节点指针
*/
BOOL   RE_SettingTuyuanGetSettingSourceFileReadInit(
    FILE  *fp,
    Setting_Init_Node_Type *  pElemInitNode
);


/*   用得到的定植值VALUE,检查定植类型，用于定植初始化操作
     参数
            pSettingValue，用来进行赋值的定植值
            pcRtValueType,用于返回定值值的类型
     返回   BOOL值，表示成功与否
*/

BOOL   RE_SettingTuyuanIsExpectedSettingType
(SCI_SIGNAL_VALUE_TYPE   *pSettingValue,
 unsigned  char  *pcRtValueType);



/*   用得到的定植值VALUE，赋给图元的输出值now
     参数   pIO，待赋值的输出的指针
            pSettingValue，用来进行赋值的定植值
     返回   无
*/

void   RE_SettingTuyuanSetOutputBySettingValue
(EP_ELEM_IO  *pIO,SCI_SIGNAL_VALUE_TYPE   *pSettingValue);



/*    常数来源的外部输入图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


__inline__  static   void     RE_ConstValueOuterInputTuyuanScan(NODE *pElemScanNode)
{

}


/*    定值来源的外部输入图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


void     RE_DingzhiOuterInputTuyuanScan(NODE *pElemScanNode);

/* 扫描多个定值节点.
 * Para:
 *     PartGrpAttrib, 分图属性.
 * Return:
 *     NONE.
 */
extern void RE_MultiDingzhiOuterInputTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib);

#endif



