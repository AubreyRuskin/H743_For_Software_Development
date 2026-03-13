/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_OuterOrderTuyuan.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的外部命令的文件头                        */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      ghx                                                                   */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                    */
/*                                                                              */
/*    Hongxia Gao       2006.10.20              创建文件1.0版本              */
/*                                                                                */
/*                                                                              */
/********************************************************************************/

#ifndef RE_OuterOrderTuyuan_H
#define RE_OuterOrderTuyuan_H


#include <vxWorks.h>
#include   "RE_PublicDataDef.h"
#include    "RE_AllTuyuanDataDef.h"
#include    "stdio_compat.h"




/******************外部命令图元的初始化和扫描时的数据节点定义**********************************************/

/*外部输入图元初始化时，保存的外部输入图元数据结构节点，
 该数据节点在完成初始化任务后,为了不占用内寸,会被删除.
 这是因为有字符串，会占用许多内存，*/

typedef  struct
{
    WRAPED_PUBLIC_ELEM_TYPE    PublicElemData;/*公共图元扫描数据   */

    /* *********外部命令图元的私有初始化数据   *********/


    uint8_t     ucSignalSourceType;/* 信号来源类型  */

    /*  定义输入源的逻辑标识，对遥控命令有意义*/
    char   strInputSourceID[MAX_LOGID_STR_LEN+1];
    uint16_t uiNodeNum;   /*该外部命令在整个配置表中的序号，对遥控命令有意义*/

    uint8_t   ucParaCount;  /*参数个数，也即输出个数*/

    /*定义输出的录波逻辑标识   */
    char   aStrLuboIDArr[MAX_OUTERORDER_OUTPUT_COUNT][MAX_LOGID_STR_LEN+1];

    /*定义输出的标志逻辑标识   */
    char   aStrFlagIDArr[MAX_OUTERORDER_OUTPUT_COUNT][MAX_LOGID_STR_LEN+1];

    /*定义输出的遥测逻辑标识   */
    char   aStrYaoceIDArr[MAX_OUTERORDER_OUTPUT_COUNT][MAX_LOGID_STR_LEN+1];

    /*定义输出的遥信逻辑标识   */
    char   aStrYaoxinIDArr[MAX_OUTERORDER_OUTPUT_COUNT][MAX_LOGID_STR_LEN+1];

    /* AO logic symbol. */
    char aStrAOChIDArr[MAX_OUTERORDER_OUTPUT_COUNT][MAX_LOGID_STR_LEN+1];


    /*定义输出的测量逻辑标识*/
    char   aStrMeasureIDArr[MAX_OUTERORDER_OUTPUT_COUNT][MAX_LOGID_STR_LEN+1];


}  OuterOrder_Init_Node_Type;



/*外部命令图元遥控命令扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;

    uint8_t   ucParaCount;  /*参数个数，也即输出个数*/
    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOutArr[MAX_OUTERORDER_OUTPUT_COUNT];
    uint16_t uiNodeNum;   /*该外部命令在整个配置表中的序号*/

}  MeaDOOuterOrder_Scan_Node_Type;


/*外部命令图元增益校准命令扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOutArr[3];


}  PlusAdtOuterOrder_Scan_Node_Type;

/*外部命令图元偏置校准命令扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOutArr[3];


}  OffAdtOuterOrder_Scan_Node_Type;

/*外部命令图元脉冲电度清零命令扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOutArr[2];


}  PulseClearOuterOrder_Scan_Node_Type;

typedef struct  /* 外部命令图元切换主变命令扫描时的数据结构节点 */
{
    /*
    获得扫描节点的输出IO的指针的函数指针
    参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE pfGetScanNodeOutIOFunc;


    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO ioOutArr[1];
} SwitchZhuBianOuterOrder_Scan_Node_Type;

typedef struct  /* 外部命令图元切换进线命令扫描时的数据结构节点 */
{
    /*
    获得扫描节点的输出IO的指针的函数指针
    参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE pfGetScanNodeOutIOFunc;


    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO ioOutArr[1];
} SwitchJinXianOuterOrder_Scan_Node_Type;

/*外部命令图元远方就地切换命令扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOutArr[2];


}  FarChgOuterOrder_Scan_Node_Type;

/*外部命令图元运行检修切换命令扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOutArr[2];


}  YXJXChgOuterOrder_Scan_Node_Type;

/*外部命令图元解挂锁切换命令扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOutArr[2];


}  JGSChgOuterOrder_Scan_Node_Type;

/* global functions */

/***********************************************************************
* VI_Come_New_FarSts - 远方就地状态，供逻辑图扫描调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_Come_New_FarSts(
    int32_t *PulOrderType
);

/***********************************************************************
* VI_Come_New_RepairSts - 检修状态，供逻辑图扫描调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_Come_New_RepairSts(
    int32_t *PulOrderType
);

/***********************************************************************
* VI_Come_New_JgsSts - 解挂锁状态，供逻辑图扫描调用
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL VI_Come_New_JgsSts(
    int32_t *PulOrderType
);

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

EP_STATUS   RE_OuterOrderTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
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

EP_STATUS   RE_OuterOrderCreateScanNodeInit(
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

EP_ELEM_IO *  RE_OuterOrderTuyuanGetOutIO
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

EP_STATUS   RE_OuterOrderTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                        LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag);



/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_OuterOrderTuyuanReadFileOtherInit
(OuterOrder_Init_Node_Type * pElemInitNodePointer);




/*    外部命令图元遥控命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_MeaDoOuterOrderTuyuanScan(NODE *pElemScanNode);

/*    外部命令图元增益校准命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_PlusAdtOuterOrderTuyuanScan(
    int  nScanTaskNo,		/* 访问逻辑图任务号 */
    NODE *pElemScanNode
);

/*    外部命令图元偏置校准命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_OffAdtOuterOrderTuyuanScan(
    int nScanTaskNo,
    NODE *pElemScanNode
);

/*    外部命令图元增益校准命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_PlusAdtOuterOrderTuyuanScan(
    int  nScanTaskNo,		/* 访问逻辑图任务号 */
    NODE *pElemScanNode
);

/* 增益校准命令
 * Para:
 *     pElemScanNode, 扫描节点.
 * Return:
 *     NONE.
 */
extern void RE_PlusAdtOuterOrderTuyuanScan2(NODE *pElemScanNode);

/*    外部命令图元脉冲电度清零命令的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_PulseClearOuterOrderTuyuanScan(NODE *pElemScanNode);;

/* scanning function for ZhuBian switching.
 * Para:
 *     pElemScanNode, scanning pointer.
 * Return:
 *     NONE.
 */
extern void RE_TdZhuBianSwitchOuterOrderTuyuanScan(NODE *pElemScanNode);

/* 偏置校准命令
 * Para:
 *     pElemScanNode, 扫描节点.
 * Return:
 *     NONE.
 */
extern void RE_OffAdtOuterOrderTuyuanScan2(NODE *pElemScanNode);

#endif



