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
/*       该头文件定义了保护功能模块中的外部通道输入元件的文件头                        */
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

#ifndef RE_OuterInputTuyuan_H
#define RE_OuterInputTuyuan_H


#include <vxWorks.h>
#include   "RE_PublicDataDef.h"
#include    "RE_AllTuyuanDataDef.h"
#include    "stdio_compat.h"




/******************外部输入图元的初始化和扫描时的数据节点定义**********************************************/

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

    /* 输入源为AI时的通道距第0号通道的偏移量 ,同时也保存测量量序号 */
    unsigned long  AISourceChOffset;

}  OuterInput_Init_Node_Type;


/*浮点AI外部输入图元扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    EP_CHART_MSG *pchart;               /* 所属分图信息 */

    /* 输入源为AI时的通道距第0号通道的偏移量  */
    unsigned long  AISourceChOffset;

    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOut;

    /* AI集中后的输出IO指针,若为空,表示未集中,2011-7-27  ZY  */
    EP_ELEM_IO   *pCollectOut;


}  FloatAIOuterInput_Scan_Node_Type;



/*复数AI外部输入图元扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    EP_CHART_MSG *pchart;               /* 所属分图信息 */

    /* 输入源为AI时的通道距第0号通道的偏移量  */
    unsigned long  AISourceChOffset;

    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOut;

    /* AI集中后的输出IO指针,若为空,表示未集中,2011-7-27  ZY  */
    EP_ELEM_IO   *pCollectOut;


}  ComplexAIOuterInput_Scan_Node_Type;

/*测量AI外部输入图元扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    EP_CHART_MSG *pchart;               /* 所属分图信息 */

    /* 输入源为测量AI时的通道距第0号通道的偏移量  */
    unsigned long  AISourceChOffset;

    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOut;


}  MeaAIOuterInput_Scan_Node_Type;

/*DI外部输入图元扫描时的数据结构节点    */
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

    EP_CHART_MSG *pchart;               /* 所属分图信息 */

    /* 输入源为DI时的通道距第0号通道的偏移量 */
    unsigned long DISourceChOffset;

    /* DI集中后的输出IO指针,若为空,表示未集中,2011-7-27  ZY  */
    EP_ELEM_IO   *pCollectOut;

}  DIOuterInput_Scan_Node_Type;

/*脉冲电度输出来源的输入图元扫描时的数据结构节点    */
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


}  PulseOutput_Scan_Node_Type;
/*ai所对应的物理通道增益系数输入图元扫描时的数据结构节点    */
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


}  AIPlusCofInput_Scan_Node_Type;

/*ai所对应的物理通道偏置系数输入图元扫描时的数据结构节点    */
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



}  AIOffCofInput_Scan_Node_Type;
/*测量量增益系数输入图元扫描时的数据结构节点    */
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

    /* 输入源为测量量序号 */
    unsigned long  clNum;
}  ClPlusCofInput_Scan_Node_Type;

/*测量量的偏置系数输入图元扫描时的数据结构节点    */
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
    /* 输入源为测量量序号 */
    unsigned long  clNum;

}  ClOffCofInput_Scan_Node_Type;

/*ai所对应的物理通道比例系数输入图元扫描时的数据结构节点    */
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



}  AIProCofInput_Scan_Node_Type;

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

EP_STATUS   RE_OuterInputTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
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

EP_STATUS   RE_OuterInputCreateScanNodeInit(
    NODE **pReturnElemScanNodePointer,
    NODE * pElemInitNode);


/*    FLOAT型的AI来源的外部输入图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


__inline__  static   void     RE_FloatAIOuterInputTuyuanScan(NODE *pElemScanNode)
{
    /* 访问当前采样节拍的物理AI通道数据指针 */
    FloatAIOuterInput_Scan_Node_Type    * pTuyuanNode;
    uint32_t   *pul;
    pTuyuanNode=(FloatAIOuterInput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    /*为了能传输除浮点数之外的其他32位数据，比如32位整数，这里用32位整数来传输
      (因为32位定点当浮点转换有可能会不合浮点格式，但浮点转定点无问题。) 2006-2-23,且对union而言，
       因为32位整数和float有相同的对齐限制，这是严格成立的 */
    pul=(uint32_t  *)(pTuyuanNode->pchart->pfBase+
                      pTuyuanNode->AISourceChOffset);
    pTuyuanNode->ioOut.now.ulVal=*pul;

}


/*    复数型的AI来源的外部输入图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


__inline__  static   void     RE_ComplexAIOuterInputTuyuanScan(NODE *pElemScanNode)
{
    /* 访问当前采样节拍的物理AI通道数据指针 */
    ComplexAIOuterInput_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(ComplexAIOuterInput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    pTuyuanNode->ioOut.now.xVal=
        *(pTuyuanNode->pchart->pxBase+
          pTuyuanNode->AISourceChOffset);

}


/*    DI来源的外部输入图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */

__inline__  static   void     RE_DIOuterInputTuyuanScan(NODE *pElemScanNode)
{
    DIOuterInput_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(DIOuterInput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    pTuyuanNode->ioOut.now.bVal=RD_His_DI
                                (pTuyuanNode->ioOut.pvCh,
                                 pTuyuanNode->pchart->ulScnAiCnt);

}

/* 多个DI来源的外部输入图元的扫描函数.
 * Para:
 *     PartGrpAttrib, 分图.
 * Return:
 *     NONE.
 */
extern void RE_MultiDIOuterInputTuyuanScan(PARTGRP_ATTRIB_TYPE *PartGrpAttrib);

/*    测量AI来源的外部输入图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
__inline__  static   void     RE_MeaAIOuterInputTuyuanScan(NODE *pElemScanNode)
{
    /* 访问当前采样节拍的物理AI通道数据指针 */
    MeaAIOuterInput_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(MeaAIOuterInput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    pTuyuanNode->ioOut.now.xVal=
        *(pTuyuanNode->pchart->pxMeaBase+
          pTuyuanNode->AISourceChOffset);

}
#endif

__inline__  static   void     RE_PoInputTuyuanScan(NODE *pElemScanNode)
{

    PulseOutput_Scan_Node_Type    *pTuyuanNode;
    uint32_t uiCurValue;
    pTuyuanNode=(PulseOutput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    RD_Rd_PO(pTuyuanNode->ioOut.pvCh,&uiCurValue);  /*有待修改*/
    pTuyuanNode->ioOut.now.ulVal=uiCurValue;

}
__inline__  static   void     RE_AiPlusCofInputTuyuanScan(NODE *pElemScanNode)
{
    AIPlusCofInput_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(AIPlusCofInput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    pTuyuanNode->ioOut.now.fVal=GetAiScaleCoeLgc(pTuyuanNode->ioOut.pvCh);

}

__inline__  static   void     RE_AiOffCofInputTuyuanScan(NODE *pElemScanNode)
{
    AIOffCofInput_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(AIOffCofInput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    pTuyuanNode->ioOut.now.fVal=GetAiExcCoeLgc(pTuyuanNode->ioOut.pvCh);

}
__inline__  static   void     RE_ClPlusCofInputTuyuanScan(NODE *pElemScanNode)
{
    float clPlusCof;
    ClPlusCofInput_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(ClPlusCofInput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    ME_Get_Msu_PlusCoff(pTuyuanNode->clNum,&clPlusCof);
    pTuyuanNode->ioOut.now.fVal=clPlusCof;

}
__inline__  static   void     RE_ClOffCofInputTuyuanScan(NODE *pElemScanNode)
{
    float clOffCof;
    ClOffCofInput_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(ClOffCofInput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    ME_Get_Msu_OffCoff(pTuyuanNode->clNum,&clOffCof);
    pTuyuanNode->ioOut.now.fVal=clOffCof;
}

__inline__  static   void     RE_AiProCofInputTuyuanScan(NODE *pElemScanNode)
{
    AIProCofInput_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(AIProCofInput_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    pTuyuanNode->ioOut.now.fVal=GetAiCoffLgc(pTuyuanNode->ioOut.pvCh);

}



