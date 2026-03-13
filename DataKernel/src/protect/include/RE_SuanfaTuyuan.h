/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_SuanfaTuyuan.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的算法元件的文件头                        */
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
/*         张云       2002.11.28              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#ifndef RE_SuanfaTuyuan_H
#define RE_SuanfaTuyuan_H


#include <vxWorks.h>
#include   "RE_PublicDataDef.h"
#include    "RE_AllTuyuanDataDef.h"
#include    "stdio_compat.h"





/******************算法图元的初始化和扫描时的数据节点定义**********************************************/


/*算法图元初始化时，保存的算法图元数据结构节点，
 该数据节点在完成初始化任务后,为了不占用内寸,会被删除.
 这是因为有字符串，会占用许多内存，*/

typedef  struct
{
    WRAPED_PUBLIC_ELEM_TYPE    PublicElemData;/*公共图元初始化数据   */

    /* *********算法图元的私有初始化数据   *********/

    BOOL   bBreakPointSetFlag;/* 调试态时的断点设置标志,真为设置断点,假为不设置断点 */
    /* 设置输出为虚拟通道的标志数组 */
    USER_INIT_FUNC_TYPE  pfUserInit;/*用户开发的算法图元入口函数指针*/

    /* 算法图元的元件名  */

    char  strSuanfaName[MAX_LOGID_STR_LEN+1];

    /* 算法图元的输入信号类型数组*/
    uint8_t   aucSourceSignalTypeArr[MAX_SUANFA_INPUT_COUNT];

    /* 算法图元的输入源顺序号数组*/
    uint16_t   aunInSourceSeqNoArr[MAX_SUANFA_INPUT_COUNT];

    /*  算法图元的输入源在相应图元的输出号*/
    uint8_t    aucInSourceOutputNumArr[MAX_SUANFA_INPUT_COUNT];


    /* 该算法图元在逻辑图上绘制的输出个数
       用于初始化时和用户内定的输出个数比较，
       若个数不匹配，则出错  */
    uint16_t   unGrpTuyuanOutCount;


    /* 逻辑图上绘制的输出信号类型数组
       用于初始化时和用户内定的输出信号类型比较,
       若类型不匹配,则出错 */
    uint8_t   aucGrpOutSignalTypeArr[MAX_OUTPUT_NUM];

    /*定义输出的录波逻辑标识   */
    char   aStrLuboIDArr[MAX_OUTPUT_NUM][MAX_LOGID_STR_LEN+1];

    /*定义输出的标志逻辑标识   */
    char   aStrFlagIDArr[MAX_OUTPUT_NUM][MAX_LOGID_STR_LEN+1];

    /*定义输出的遥测逻辑标识   */
    char   aStrYaoceIDArr[MAX_OUTPUT_NUM][MAX_LOGID_STR_LEN+1];

    /*定义输出的遥信逻辑标识   */
    char   aStrYaoxinIDArr[MAX_OUTPUT_NUM][MAX_LOGID_STR_LEN+1];

    /*定义输出的虚拟通道逻辑标识   */
    char   aStrVirtualChIDArr[MAX_OUTPUT_NUM][MAX_LOGID_STR_LEN+1];

    /*定义输出的测量逻辑标识*/
    char   aStrMeasureIDArr[MAX_OUTPUT_NUM][MAX_LOGID_STR_LEN+1];

    /* AO logic symbol. */
    char aStrAOChIDArr[MAX_OUTPUT_NUM][MAX_LOGID_STR_LEN+1];

    /*虚拟通道标志数组   */
    BOOL     abVirtualChFlagArr[MAX_OUTPUT_NUM];

}  Suanfa_Init_Node_Type;



/*算法图元扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;

    /*图元数据节点*/
    EP_ELEMENT    elem;

    /* ****算法图元私有扫描数据*****/

    EP_ELEM_IO *   apInArr[MAX_SUANFA_INPUT_COUNT];/*算法输入数据指针数组*/
    BOOL   bBreakPointSetFlag;/* 调试态时的断点设置标志,真为设置断点,假为不设置断点 */
    /* 设置输出为虚拟通道的标志数组 */

}  Suanfa_Scan_Node_Type;






/*调试入口函数声明，该函数在Extend.c中实现  */
void EP_Debug_Part(void);





/****算法图元的逻辑图文件读取初始化函数****************************/
/*
     功能:从文件中读取图元相关数据,此时已读完图元类型字节
          申请初始化数据节点,
          供上层程序添加两节点到连表中
          并进行数据节点内容的部分初始化

*/
/****参数：fp,逻辑图文件指针***************************/
/*         ulReadOffsetToBegain,相对于文件起始,读取的文件偏移位置
           pTuyuanInitData,图元节点初始化数据指针
           pRtElemInitNodePointer,返回申请的图元初始化数据节点内存地址
*/
/*   返回值，EP_STATUS */

EP_STATUS   RE_SuanfaTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
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

EP_STATUS   RE_SuanfaCreateScanNodeInit(
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

EP_ELEM_IO *  RE_SuanfaTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode);







/****算法图元的扫描初始化函数****************************/
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

EP_STATUS   RE_SuanfaTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                    LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag);








/*  在读取文件后,进行算法图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_SuanfaTuyuanReadFileOtherInit
(Suanfa_Init_Node_Type * pElemInitNodePointer);





/*    根据元件名，查找用户开发的算法入口函数指针
      参数   strSuanfaName，算法元件名
             ppfRtFunc，供返回入口函数指针
      返回值，EP_STATUS
              成功  EP_SUCCESS;
              失败,其他
*/
EP_STATUS   RE_SearchUserDevInitFunc(char  *strSuanfaName,
                                     USER_INIT_FUNC_TYPE   *  ppfRtFunc);


/*   当输出为虚拟通道类型时,
     检查该输出信号类型是否为所允许的类型
     参数  pCheckedOutElem,待检查的输出的指针
     返回值  EP_STATUS
             无错误,则返回真,
             否则,返回假
*/

EP_STATUS   RE_SuanfaTuyuanVirtualChOutSignalTypeCheck
(EP_ELEM_IO  *pCheckedOutElem);




/*    算法图元包裹化后的扫描函数**/
/*    功能:首先调用用户开发的算法扫描函数
           然后处理其他工作.
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***********************/
/*    返回值，无   */


__inline__  static   void     RE_SuanfaTuyuanScan(NODE  *pElemScanNode)
{
    Suanfa_Scan_Node_Type  *  pTuyuanNode;

    pTuyuanNode=(Suanfa_Scan_Node_Type   *)pElemScanNode->pTuyuan;

    /* 执行用户定义的扫描函数  */
    (* pTuyuanNode->elem.Scan_Func)(&(pTuyuanNode->elem));

}



#endif



