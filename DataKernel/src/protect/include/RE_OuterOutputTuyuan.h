/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_OuterOutputTuyuan.h                            1.1                  */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的外部通道输出元件的文件头                        */
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
/*                    2005.10.24              修改文件1.1版本                */
/*                                                                              */
/********************************************************************************/

#ifndef RE_OuterOutputTuyuan_H
#define RE_OuterOutputTuyuan_H

#include <vxWorks.h>
#include   "RE_PublicDataDef.h"
#include    "RE_AllTuyuanDataDef.h"
#include    "stdio_compat.h"
#include    "taskLib.h"
#include   "realdata.h"



/******************外部输出图元的初始化和扫描时的数据节点定义**********************************************/

/*外部输出图元初始化时，保存的外部输出图元数据结构节点，
 该数据节点在完成初始化任务后,为了不占用内寸,会被删除.
 这是因为有字符串，会占用许多内存，*/

typedef  struct
{
    WRAPED_PUBLIC_ELEM_TYPE    PublicElemData;/*公共图元扫描数据   */

    /* *********外部输出图元的私有初始化数据   *********/

    /*输出目的的访问句柄，对DO和指示灯，AI通道都有效  */
    void  *   pvDestHandle;

    /* 输入目的地为AI时的通道距第0号通道的偏移量  */
    unsigned long  AIDestChOffset;

    /* 输入目的地为AI时是否为物理通道的标志，若是物理通道，则为真，
       否则若为逻辑通道，则为假*/
    BOOL  bPhyAIChFlag;

    uint8_t     ucSignalDestType;/* 信号目的地类型  */

    /* 外部输出图元的输入信号类型数组*/
    uint8_t   aucSourceSignalTypeArr[1];

    /* 外部输出图元的输入源顺序号数组*/
    uint16_t   aunInSourceSeqNoArr[1];

    /*  外部输出图元的输入源在相应图元的输出号*/
    uint8_t    aucInSourceOutputNumArr[1];

    /*  定义输出目的的逻辑标识或名称，对DO，指示灯，
        AI通道，跳闸启停，端口引出有意义*/
    char   strOutputDestID[MAX_LOGID_STR_LEN+1];

    BOOL bLampAdjust;   /*输出类型为指示灯时的状态调整属性，包括下面
                                         三个需调整的属性ghx20061024*/
    BOOL bLampKeep;
    uint8_t ucLampColor;
    uint8_t ucLampBlink;

}  OuterOutput_Init_Node_Type;




/*DO外部输出图元扫描时的数据结构节点    */
typedef  struct
{
    /*输出目的的访问句柄，对DO和指示灯,AI通道都有效  */
    void  *   pvDestHandle;

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

    BOOL   bLastValue;

}  DOOuterOutput_Scan_Node_Type;


/*指示灯外部输出图元扫描时的数据结构节点    */
typedef  struct
{


    /*输出目的的访问句柄，对DO和指示灯,AI通道都有效  */
    void  *   pvDestHandle;

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

    BOOL   bLastValue;

}  LampOuterOutput_Scan_Node_Type;



/*AI通道外部输出图元扫描时的数据结构节点    */
typedef  struct
{


    EP_CHART_MSG  *pChartMsg;  /*逻辑图信息*/

    /* 输入目的地为AI时的通道距第0号通道的偏移量  */
    unsigned long  AIDestChOffset;

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

}  AIOuterOutput_Scan_Node_Type;


/*光纵AO通道外部输出图元扫描时的数据结构节点    */
typedef  struct
{

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */
    EP_ELEM_IO     ioOut;
}  OptAOOuterOutput_Scan_Node_Type;

/*脉冲输出外部输出图元扫描时的数据结构节点*/
typedef struct
{
    /*输出目的的访问句柄*/
    void  *   pvDestHandle;
    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */
} POOuterOutput_Scan_Node_Type;

/*AI对应的物理通道增益系数设置图元扫描时的数据结构节点    */
typedef  struct
{
    /*输出目的的访问句柄AI通道*/
    void  *   pvDestHandle;

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

    float   fLastValue;

}  AIPlusCofOutput_Scan_Node_Type;


/*AI对应的物理通道偏置系数设置图元扫描时的数据结构节点    */
typedef  struct
{
    /*输出目的的访问句柄AI通道*/
    void  *   pvDestHandle;

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

    float   fLastValue;

}  AIOffCofOutput_Scan_Node_Type;

/*测量量增益系数设置图元扫描时的数据结构节点    */
typedef  struct
{
    int clNum;    /*该测量量在整个测量量配置表中的序号*/

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

    float   fLastValue;

}  ClPlusCofOutput_Scan_Node_Type;

/*测量量偏置系数设置图元扫描时的数据结构节点    */
typedef  struct
{
    int clNum;    /*该测量量在整个测量量配置表中的序号*/

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

    float   fLastValue;

}  ClOffCofOutput_Scan_Node_Type;

/*di消抖时间设置图元扫描时的数据结构节点    */
typedef  struct
{
    /*输出目的的访问句柄di通道*/
    void  *   pvDestHandle;

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

    float   fLastValue;

}  DIFiltTmOutput_Scan_Node_Type;

/*遥测量越限系数设置图元扫描时的数据结构节点    */
typedef  struct
{
    int clNum;    /*该遥测量在整个遥测量配置表中的序号*/

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

    float   fLastValue;

}  YcChgCofOutput_Scan_Node_Type;

/*测量量越限系数设置图元扫描时的数据结构节点    */
typedef  struct
{
    int clNum;    /*该测量量在整个测量量配置表中的序号*/

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

    float   fLastValue;

}  ClChgCofOutput_Scan_Node_Type;



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

EP_STATUS   RE_OuterOutputTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
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

EP_STATUS   RE_OuterOutputCreateScanNodeInit(
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

EP_STATUS   RE_OuterOutputTuyuanSetInputIO
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

EP_STATUS   RE_OuterOutputTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
        LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag);


/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_OuterOutputTuyuanReadFileOtherInit
(OuterOutput_Init_Node_Type * pElemInitNodePointer);


/*  在读取文件后,进行AI目的地类型的相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_OuterOutputTuyuanAIDestReadFileOtherInit
(OuterOutput_Init_Node_Type * pElemInitNode);

/*  在读取文件后,进行AO目的地类型的相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_OuterOutputTuyuanAODestReadFileOtherInit
(OuterOutput_Init_Node_Type * pElemInitNode);

/***********************************************************************
* RD_Chg_Led_Attr - 调整指示灯属性
*
* RETURNS: 无
*
*/
extern void RD_Chg_Led_Attr(
    void *pvLenHnd,
    BOOL bKeep,
    uint8_t ucColor,
    uint8_t ucBlink
);

/* 多个指示灯输出目的的外部输出图元的扫描函数.
 * Para:
 *     分图.
 * Return:
 *     NONE.
 */
extern void RE_MultiLampOuterOutputTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib);


/*    FloatAI输出目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_FloatAIOuterOutputTuyuanScan(NODE *pElemScanNode);


/*    ComplexAI输出目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_ComplexAIOuterOutputTuyuanScan(NODE *pElemScanNode);


/* 扫描多个DO节点.
 * Para:
 *     PartGrpAttrib, 分图属性.
 * Return:
 *     NONE.
 */
extern void RE_MultiDOTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib);

/* 增益校准命令
 * Para:
 *     pElemScanNode, 扫描节点.
 * Return:
 *     NONE.
 */
extern void RE_PlusAdtOuterOrderTuyuanScan2(NODE *pElemScanNode);

/*    DO输出目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_DOOuterOutputTuyuanScan(NODE *pElemScanNode);

/*    指示灯输出目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_LampOuterOutputTuyuanScan(NODE *pElemScanNode);

/* scanning function for JinXian switching.
 * Para:
 *     pElemScanNode, scanning pointer.
 * Return:
 *     NONE.
 */
extern void RE_TdJinXianSwitchOuterOrderTuyuanScan(NODE *pElemScanNode);

/*    远方就地切换命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_FarStsChgOuterOrderTuyuanScan(NODE *pElemScanNode);

/*    运行检修切换命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_YXJXChgOuterOrderTuyuanScan(NODE *pElemScanNode);

/*    解挂锁切换命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_JGSChgOuterOrderTuyuanScan(NODE *pElemScanNode);

/*    光纵AO输出目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_OptAoOuterOutputTuyuanScan(NODE *pElemScanNode);

/*    ai对应的物理通道增益系数输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

extern void     RE_AIPlusCofOutputTuyuanScan(NODE *pElemScanNode);

/*    ai对应的物理通道偏置系数输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

extern void     RE_AIOffCofOutputTuyuanScan(NODE *pElemScanNode);

/*    测量量增益系数输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

extern void     RE_ClPlusCofOutputTuyuanScan(NODE *pElemScanNode);

/*    测量量偏置系数输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

extern void     RE_ClOffCofOutputTuyuanScan(NODE *pElemScanNode);

/*    di效抖时间输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

extern void     RE_DIFiltTmOutputTuyuanScan(NODE *pElemScanNode);

/*    遥测量越限系数输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

extern void     RE_YcChgCofOutputTuyuanScan(NODE *pElemScanNode);

/*    测量量越限系数输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

extern void     RE_ClChgCofOutputTuyuanScan(NODE *pElemScanNode);

/*    PO输出目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_POOuterOutputTuyuanScan(NODE *pElemScanNode);

#endif



