/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_ExportTuyuan.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的端口输出元件的文件头                        */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                         */
/*                                                                              */
/*         张云       2005.11.1              创建文件1.0版本                    */

/*                                                                              */
/********************************************************************************/

#ifndef RE_ExportTuyuan_H
#define RE_ExportTuyuan_H

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


    /* 外部输出图元的输入信号类型数组*/
    uint8_t   aucSourceSignalTypeArr[1];

    /* 外部输出图元的输入源顺序号数组*/
    uint16_t   aunInSourceSeqNoArr[1];

    /*  外部输出图元的输入源在相应图元的输出号*/
    uint8_t    aucInSourceOutputNumArr[1];

    /*  定义输出目的的逻辑标识或名称，对DO，指示灯，
        AI通道，跳闸启停，端口引出有意义*/
    char   strOutputDestID[MAX_LOGID_STR_LEN+1];



}  Export_Init_Node_Type;


/*端口引出外部输出图元扫描时的数据结构节点    */
typedef  struct
{


    EP_ELEM_IO *   pInArr0;/*输入数据指针0  */

    /* 输出量，存放本图元输出结果 */
    EP_ELEM_IO   ioOut;

    /* Export集中后的输出指针(注意不是IO),若为空,表示未集中,2011-7-27  ZY  */
    void  *pCollectOut;
}  ExternExportOuterOutput_Scan_Node_Type;





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

EP_STATUS   RE_ExportTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
                                        TUYUAN_READ_FILE_INIT_DATA   *pTuyuanInitData,
                                        NODE ** pRtElemInitNodePointer);




/*　当外部输出目的地为端口引出时,读取原始值,并进行相应的初始化
    参数   fp  ,文件当前指针
           pElemInitNode,初始化节点指针
*/
BOOL   RE_ExportTuyuanGetExternExportDestInitValueFileReadInit(
    FILE  *fp,
    Export_Init_Node_Type *  pElemInitNode
);





/*
     功能:根据初始化节点数据，申请扫描节点，并进行扫描节点的部分初始化
     返回扫描节点指针

*/
/****参数：
           pRtElemScanNodePointer,返回申请的图元扫描数据节点内存地址
           pElemInitNode,图元初始化数据节点指针
*/
/*   返回值，EP_STATUS */

EP_STATUS   RE_ExportCreateScanNodeInit(
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

EP_STATUS   RE_ExportTuyuanSetInputIO
(uint8_t  ucInputNum,EP_ELEM_IO *  pElemIO,
 NODE  * pScanNode);



/*
     功能:获得外部端口输出目的地图元的扫描节点的某个输出的指针

*/
/****参数：
           unOutNum,输出的序号
           pScanNode，扫描节点指针
*/
/*   返回值，相应序号的输出IO的指针 ，若失败，则返回NULL*/

EP_ELEM_IO *  RE_ExportTuyuanGetOutIO
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

EP_STATUS   RE_ExportTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                    LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag);





/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_ExportTuyuanReadFileOtherInit
(Export_Init_Node_Type * pElemInitNodePointer);





/*    端口引出输出目的的外部输出图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


__inline__  static   void     RE_ExternExportOuterOutputTuyuanScan(NODE *pElemScanNode)
{
    /* 由于可能被其他任务访问，需要保护临界资源  */

    ExternExportOuterOutput_Scan_Node_Type  *  pCurTuyuan;
    pCurTuyuan=(ExternExportOuterOutput_Scan_Node_Type  *)pElemScanNode->pTuyuan;
    pCurTuyuan->ioOut.now=pCurTuyuan->pInArr0->now;
    pCurTuyuan->ioOut.pvCh=pCurTuyuan->pInArr0->pvCh;


}

/*   2011-8-8  ZY
     功能:获得外部端口输出图元集中后的扫描节点的输出指针(注意不是IO)

*/
/****参数：
           unOutNum,输出的序号
           pScanNode，扫描节点指针
*/
/*   返回值，相应序号的输出的指针 ，若失败，则返回NULL*/

void *  RE_ExportTuyuanGetCollectOut
(uint16_t   unOutNum,NODE  * pScanNode);




#endif



