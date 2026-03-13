/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_ReportStartTuyuan.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的报告启停元件的文件头                        */
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
/*         张云       2005.10.25              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#ifndef RE_ReportStartTuyuan_H
#define RE_ReportStartTuyuan_H

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

    /* 是否定义事件供保护启动时触发 */
    BOOL   bDefEventToTrigger;

    /* 外部输出图元的输入信号类型数组*/
    uint8_t   aucSourceSignalTypeArr[1];

    /* 外部输出图元的输入源顺序号数组*/
    uint16_t   aunInSourceSeqNoArr[1];

    /*  外部输出图元的输入源在相应图元的输出号*/
    uint8_t    aucInSourceOutputNumArr[1];

    /*  定义输出目的的逻辑标识或名称，对DO，指示灯，
        AI通道，跳闸启停，端口引出有意义*/
    char   strOutputDestID[MAX_LOGID_STR_LEN+1];
    /*  报告启动前推采样点数*/
    uint16_t iForwordTime;

    /*图元保护启停触发的事件的参数输入数据指针数组
      事件触发信号不在该数组里面，实际这里参数个数大小为0*/
    EP_ELEM_IO *   apInParaArr[2];

    /*  定义跳闸启停时触发的事件的逻辑标识，
    只对跳闸启停有意义,允许为空，表示不触发事件*/
    char   strTripTriggerEventID[MAX_LOGID_STR_LEN+1];

}  ReportStart_Init_Node_Type;


/*报告允许外部输出图元扫描时的数据结构节点    */
typedef  struct
{
    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

    /*  上次扫描输入的触发值  */
    BOOL   bLastScanInputValue;

    EP_CHART_MSG  *pChartMsg;  /*逻辑图信息*/

    /* 是否定义事件供保护启动时触发 */
    BOOL   bDefEventToTrigger;

    /*由图元初始化时得到的保护启动触发事件的序号,
    ，只有当bExistEventToTrigger为TRUE时，才允许触发事件  */
    int16_t   nEventNum;

    /*  报告启动前推时间*/
    uint16_t iForwordTime;

}  ReportEnableOuterOutput_Scan_Node_Type;



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

EP_STATUS   RE_ReportStartTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
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

EP_STATUS   RE_ReportStartCreateScanNodeInit(
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

EP_STATUS   RE_ReportStartTuyuanSetInputIO
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

EP_STATUS   RE_ReportStartTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
        LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag);


/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_ReportStartTuyuanReadFileOtherInit
(ReportStart_Init_Node_Type * pElemInitNodePointer);

/***********************************************************************
* RE_ReportEnableOuterOutputTuyuanScan - 扫描处理, 报告允许输出目的的外部输出图元的扫描函数
*
* RETURNS: 无
*
*/
void RE_ReportEnableOuterOutputTuyuanScan(
    NODE *pElemScanNode		/* 图元的操纵的扫描数据节点指针 */
);



#endif



