/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_SysServerTuyuan.h                            1.1                  */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的系统服务输出元件的文件头                        */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      ghx                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                    */
/*                                                                              */
/*      Hongxia Gao     2006.11.13              创建文件1.0版本              */
/*                                                                              */
/********************************************************************************/

#ifndef RE_SysServerTuyuan_H
#define RE_SysServerTuyuan_H

#include <vxWorks.h>
#include   "RE_PublicDataDef.h"
#include    "RE_AllTuyuanDataDef.h"
#include    "stdio_compat.h"


/* defines */

/******************系统服务输出图元的初始化和扫描时的数据节点定义*******************/

/*系统服务输出图元初始化时，保存的系统服务输出图元数据结构节点，
 该数据节点在完成初始化任务后,为了不占用内寸,会被删除.
 这是因为有字符串，会占用许多内存，*/

typedef  struct
{
    WRAPED_PUBLIC_ELEM_TYPE    PublicElemData;/*公共图元扫描数据   */

    /* *********系统服务输出图元的私有初始化数据   *********/


    uint8_t     ucServerType;/* 系统服务类型  */

    /* 系统服务输出图元的输入信号类型数组*/
    uint8_t   aucSourceSignalTypeArr[MAX_SYSSERVER_INPUT_COUNT];

    /* 系统服务输出图元的输入源顺序号数组*/
    uint16_t   aunInSourceSeqNoArr[MAX_SYSSERVER_INPUT_COUNT];

    /*  系统服务输出图元的输入源在相应图元的输出号*/
    uint8_t    aucInSourceOutputNumArr[MAX_SYSSERVER_INPUT_COUNT];

    /*  定义输出目的的逻辑标识或名称，对DO，指示灯，
       	AI通道，跳闸启停，端口引出有意义*/
    char   strOutputDestID1[MAX_LOGID_STR_LEN+1];

    int16_t iYabanNum;			/* 压板号 */
    int32_t iCh;						/* 遥信号 */
    uint8_t ucType;		/* Type of YaoXin */
    FLT_U32_UNION *pSetDest;			/* 更新的目标定值指针，初始化数据结构 */


    /*  定义输出目的的逻辑标识或名称，对DO，指示灯，
       	AI通道，跳闸启停，端口引出有意义*/
    char   strOutputDestID2[MAX_LOGID_STR_LEN+1];
}  SysServer_Init_Node_Type;




/*保护定值区切换土缘扫描时的数据结构节点    */
typedef  struct
{

    EP_ELEM_IO *   apInArr[MAX_SYSSERVER_INPUT_COUNT];/*输入数据指针*/

    BOOL   bLastValue;

}  SettingSwitch_Scan_Node_Type;

/*复归命令图元*/
typedef  struct
{

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

    BOOL   bLastValue;

}  Revert_Scan_Node_Type;

/*切换到远方态图元*/
typedef  struct
{

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

}  SwitchFar_Scan_Node_Type;
/*切换到就地态图元*/
typedef  struct
{

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

}  SwitchLocal_Scan_Node_Type;
/*切换到运行态图元*/
typedef  struct
{

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

}  SwitchRun_Scan_Node_Type;
/*切换到检修态图元*/
typedef  struct
{

    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

}  SwitchExam_Scan_Node_Type;


/*3U0越限告警*/
typedef struct
{
    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

} ThreeU0Warn_Scan_Node_Type;

/*系统服务图元增益校准扫描时的数据结构节点    */
typedef  struct
{
    EP_ELEM_IO *   apInArr[MAX_SYSSERVER_INPUT_COUNT];/*输入数据指针*/
    BOOL bCurVaueLst;

}  PlusAdtSysServer_Scan_Node_Type;

/*系统服务图元偏置校准扫描时的数据结构节点    */
typedef  struct
{
    EP_ELEM_IO *   apInArr[MAX_SYSSERVER_INPUT_COUNT];/*输入数据指针*/
    BOOL bCurVaueLst;

}  OffAdtSysServer_Scan_Node_Type;

/* 压板投退 */
typedef struct
{
    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */
    int16_t iYabanNum;			/* 压板号 */
    BOOL bCurVaueLst;
} YabanTT_Scan_Node_Type;

/* 参数定值自动整定，建立扫描数据结构 */
typedef struct
{
    EP_ELEM_IO *apInArr[MAX_SYSSERVER_INPUT_COUNT];		/* 输入数据指针 */
    FLT_U32_UNION *pSetDest;			/* 更新的目标定值指针 */
    BOOL bLastVal;                      /*上次扫描的触发信号*/
} STAUTOST_Scan_Node_Type;

/* 遥信 */
typedef struct
{
    EP_ELEM_IO *apInArr[MAX_SYSSERVER_INPUT_COUNT];		/* 输入数据指针 */
    int32_t iCh;
    uint8_t ucType;		/* Type of YaoXin */
    BOOL bCurValLst;
} DBYX_Scan_Node_Type;

/* global functions */
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

EP_STATUS   RE_SysServerTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
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

EP_STATUS   RE_SysServerCreateScanNodeInit(
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

EP_STATUS   RE_SysServerTuyuanSetInputIO
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

EP_STATUS   RE_SysServerTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                       LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag);


/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_SysServerTuyuanReadFileOtherInit
(SysServer_Init_Node_Type * pElemInitNodePointer);

/***********************************************************************
* RE_IncSetAtSetCnt - 增加任务自动整定定值计数
*
* RETURNS: 无
*
*/
extern void RE_IncSetAtSetCnt(void);

/*    定值切换系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
extern void     RE_SettingSwitchTuyuanScan(NODE *pElemScanNode);


/*    复归系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
extern void     RE_RevertTuyuanScan(NODE *pElemScanNode);

/*    切换到远方态系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
extern void     RE_SwitchFarTuyuanScan(NODE *pElemScanNode);

/*    切换到检修态系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
extern void     RE_SwitchExamTuyuanScan(NODE *pElemScanNode);

/*    3U0越限告警系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
extern void     RE_ThreeU0WarnTuyuanScan(NODE *pElemScanNode);

/*    增益校准结束系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
extern void     RE_PlusAdtOverTuyuanScan(NODE *pElemScanNode);

/*    偏置校准结束系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
extern void     RE_OffAdtOverTuyuanScan(NODE *pElemScanNode);

/*    压板投退系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
extern void     RE_YaBanTTTuyuanScan(NODE *pElemScanNode);

/*    自动整定定值系统服务输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */
extern void RE_SetAutoSetTuyuanScan(NODE *pElemScanNode);

/***********************************************************************
* RE_DBYXTuyuanScan - 遥信系统服务图元的扫描函数
*
* RETURNS: 无
*
*/
extern void RE_DBYXTuyuanScan(
    NODE *pElemScanNode			/* 操纵的扫描数据节点指针 */
);

#endif



