/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_AllTuyuanDataDef.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块所有图元数据结构所需要的定义                 */
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
/*         张云       2002.11.30              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#ifndef RE_ALLTUYUANDATADEF_H
#define RE_ALLTUYUANDATADEF_H


#include <vxWorks.h>
#include "RE_PublicDataDef.h"
#include   "string_compat.h"
#include   "swcfg.h"
#include   "view.h"
#include   "RE_LoadLogrpFile.h"
/*****************所有图元公共结构定义*********************************/



/****图元公共初始化结构定义******/

typedef   struct   ElEM_PUBLIC_DATA_TYPE
{

    EP_ELEMENT    elem;
    BOOL   *pbCurScanDingzhiRreshFlag;/*当前扫描需要刷新定植标志的指针
                                        主要用于需获得定植的图元才有意义
                                        比如时间继电器  这在创建图元
                                        数据节点时会传给该指针*/
    long   nScanTaskNo;/*  该图元所在的逻辑图扫描任务序号 */

    /* 数组，存放每个图元输出结果的录波，标志，遥测，遥信、测量标志 */
    BOOL     abLuboFlagArr[MAX_OUTPUT_NUM];/* 录波标志 */
    BOOL     abFlagSetFlagArr[MAX_OUTPUT_NUM]; /* 标志设置标志*/
    BOOL     abYaoxinFlagArr[MAX_OUTPUT_NUM];  /* 遥测设置标志*/
    BOOL     abYaoceFlagArr[MAX_OUTPUT_NUM];   /* 遥信设置标志*/
    BOOL     abMeasureFlagArr[MAX_OUTPUT_NUM]; /* 测量量标志  */
    BOOL abAOFlagArr[MAX_OUTPUT_NUM];      /* AO symbol, if set this attribute. */

    /* 该图元扫描结构所对应的初始化函数指针 */
    /*
       参数 pElemInitNode，图元初始化数据节点指针，
            pElemScanNode，图元扫描时数据节点数据指针
           pGrpScanNodeList   图元待访问的逻辑分图扫描数据节点连表
           bPartGrpRunFlag  该图元所在的分图被投入的标志，若被投入
                            则为真，否则，为假
    */
    /*   返回值，成功与否   */

    EP_STATUS  (*  pfInitScanFunc)(NODE * pElemInitNode,
                                   NODE  *pElemScanNode,LIST  *pGrpScanNodeList
                                   ,BOOL  bPartGrpRunFlag);


    /*获得扫描节点的输出IO的指针的函数指针*/

    /*参数1为扫描节点的输出号
      参数2为扫描节点的指针
      返回值为相应输出的指针，若失败，则返回NULL；

      */

    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


}  WRAPED_PUBLIC_ELEM_TYPE;





#endif



