/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_OrTuyuan.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的或门元件的文件头                        */
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

#ifndef RE_OrTuyuan_H
#define RE_OrTuyuan_H


#include <vxWorks.h>
#include   "RE_PublicDataDef.h"
#include    "RE_AllTuyuanDataDef.h"
#include    "stdio_compat.h"



/******************或门图元的初始化和扫描时的数据节点定义**********************************************/




/*或门图元初始化时，保存的或门图元数据结构节点，
 该数据节点在完成初始化任务后,为了不占用内寸,会被删除.
 这是因为有字符串，会占用许多内存，*/

typedef  struct
{
    WRAPED_PUBLIC_ELEM_TYPE    PublicElemData;/*公共图元扫描数据   */

    /* *********或门图元的私有初始化数据   *********/

    /* 或门图元的输入信号类型数组*/
    uint8_t   aucSourceSignalTypeArr[MAX_OR_INPUT_COUNT];

    /* 或门图元的输入源顺序号数组*/
    uint16_t   aunInSourceSeqNoArr[MAX_OR_INPUT_COUNT];

    /*  或门图元的输入源在相应图元的输出号*/
    uint8_t    aucInSourceOutputNumArr[MAX_OR_INPUT_COUNT];


    /*定义输出的录波逻辑标识   */
    char   aStrLuboIDArr[1][MAX_LOGID_STR_LEN+1];

    /*定义输出的标志逻辑标识   */
    char   aStrFlagIDArr[1][MAX_LOGID_STR_LEN+1];

    /*定义输出的遥信逻辑标识   */
    char   aStrYaoxinIDArr[1][MAX_LOGID_STR_LEN+1];


}  Or_Init_Node_Type;


/*2输入或门图元扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    EP_ELEM_IO *   pInArr0;/*或门输入数据指针0  */

    EP_ELEM_IO *   pInArr1;/*或门输入数据指针1  */

    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOut;


}  Or2_Scan_Node_Type;




/*3输入或门图元扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    EP_ELEM_IO *   pInArr0;/*或门输入数据指针0  */

    EP_ELEM_IO *   pInArr1;/*或门输入数据指针1  */

    EP_ELEM_IO *   pInArr2;/*或门输入数据指针2  */

    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOut;


}  Or3_Scan_Node_Type;



/*4输入或门图元扫描时的数据结构节点    */
typedef  struct
{

    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    EP_ELEM_IO *   pInArr0;/*或门输入数据指针0  */

    EP_ELEM_IO *   pInArr1;/*或门输入数据指针1  */

    EP_ELEM_IO *   pInArr2;/*或门输入数据指针2  */

    EP_ELEM_IO *   pInArr3;/*或门输入数据指针3  */

    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOut;


}  Or4_Scan_Node_Type;



/*5输入或门图元扫描时的数据结构节点    */
typedef  struct
{

    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    EP_ELEM_IO *   pInArr0;/*或门输入数据指针0  */

    EP_ELEM_IO *   pInArr1;/*或门输入数据指针1  */

    EP_ELEM_IO *   pInArr2;/*或门输入数据指针2  */

    EP_ELEM_IO *   pInArr3;/*或门输入数据指针3  */

    EP_ELEM_IO *   pInArr4;/*或门输入数据指针4  */

    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOut;


}  Or5_Scan_Node_Type;




/*6输入或门图元扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    EP_ELEM_IO *   pInArr0;/*或门输入数据指针0  */

    EP_ELEM_IO *   pInArr1;/*或门输入数据指针1  */

    EP_ELEM_IO *   pInArr2;/*或门输入数据指针2  */

    EP_ELEM_IO *   pInArr3;/*或门输入数据指针3  */

    EP_ELEM_IO *   pInArr4;/*或门输入数据指针4  */

    EP_ELEM_IO *   pInArr5;/*或门输入数据指针5  */

    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOut;


}  Or6_Scan_Node_Type;




/*7输入或门图元扫描时的数据结构节点    */
typedef  struct
{

    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    EP_ELEM_IO *   pInArr0;/*或门输入数据指针0  */

    EP_ELEM_IO *   pInArr1;/*或门输入数据指针1  */

    EP_ELEM_IO *   pInArr2;/*或门输入数据指针2  */

    EP_ELEM_IO *   pInArr3;/*或门输入数据指针3  */

    EP_ELEM_IO *   pInArr4;/*或门输入数据指针4  */

    EP_ELEM_IO *   pInArr5;/*或门输入数据指针5  */

    EP_ELEM_IO *   pInArr6;/*或门输入数据指针6  */

    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOut;


}  Or7_Scan_Node_Type;




/*8输入或门图元扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    EP_ELEM_IO *   pInArr0;/*或门输入数据指针0  */

    EP_ELEM_IO *   pInArr1;/*或门输入数据指针1  */

    EP_ELEM_IO *   pInArr2;/*或门输入数据指针2  */

    EP_ELEM_IO *   pInArr3;/*或门输入数据指针3  */

    EP_ELEM_IO *   pInArr4;/*或门输入数据指针4  */

    EP_ELEM_IO *   pInArr5;/*或门输入数据指针5  */

    EP_ELEM_IO *   pInArr6;/*或门输入数据指针6  */

    EP_ELEM_IO *   pInArr7;/*或门输入数据指针7  */

    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOut;


}  Or8_Scan_Node_Type;





/*多输入或门图元扫描时的数据结构节点    */
typedef  struct
{
    /*获得扫描节点的输出IO的指针的函数指针*/
    /*参数1为扫描节点的输出号
    参数2为扫描节点的指针
    返回值为相应输出的指针，若失败，则返回NULL；
    */
    GET_OUT_IO_FUNC_TYPE   pfGetScanNodeOutIOFunc;


    uint16_t unInNum;                   /* 总的输入个数 */

    EP_ELEM_IO *   apInArr[MAX_OR_INPUT_COUNT];/*与门输入数据指针数组   */

    /* 输出量数组，存放本图元输出结果 */
    EP_ELEM_IO   ioOut;


}  OrMulti_Scan_Node_Type;









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

EP_STATUS   RE_OrTuyuanReadFileInit(FILE  *fp,uint32_t  ulReadOffsetToBegain,
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

EP_STATUS   RE_OrCreateScanNodeInit(
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

EP_ELEM_IO *  RE_OrTuyuanGetOutIO
(uint16_t   unOutNum,NODE  * pScanNode);




/*
     功能:设置图元的扫描节点的某个输入的指针
*/
/****参数：
           ucInputNum,输入的序号
           pElemIO,输入IO指针
           pScanNode，扫描节点指针
*/
/*   返回值，返回成功与否*/

EP_STATUS   RE_OrTuyuanSetInputIO
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

EP_STATUS   RE_OrTuyuanScanInit(NODE * pElemInitNode,NODE  *pElemScanNode,
                                LIST  *pGrpScanNodeList,BOOL  bPartGrpRunFlag);




/*  在读取文件后,进行图元相关未能从文件中获得的其他信息的初始化
      参数  pElemInitNodePointer,初始化节点指针

      返回值,成功,返回真,失败返回假
*/
BOOL      RE_OrTuyuanReadFileOtherInit
(Or_Init_Node_Type * pElemInitNodePointer);




/*    2输入或门图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


__inline__  static   void     RE_2OrTuyuanScan(NODE *pElemScanNode)
{
    Or2_Scan_Node_Type    *  pTuyuanNode;
    pTuyuanNode=(Or2_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    pTuyuanNode->ioOut.now.bVal=
        (pTuyuanNode->pInArr0->now.bVal)
        ||(pTuyuanNode->pInArr1->now.bVal);


}



/*    3输入或门图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


__inline__  static   void     RE_3OrTuyuanScan(NODE *pElemScanNode)
{
    Or3_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(Or3_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    pTuyuanNode->ioOut.now.bVal=
        (pTuyuanNode->pInArr0->now.bVal)
        ||(pTuyuanNode->pInArr1->now.bVal)
        ||(pTuyuanNode->pInArr2->now.bVal);


}



/*    4输入或门图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


__inline__  static   void     RE_4OrTuyuanScan(NODE *pElemScanNode)
{
    Or4_Scan_Node_Type    *pTuyuanNode;
    pTuyuanNode=(Or4_Scan_Node_Type    *)pElemScanNode->pTuyuan;
    pTuyuanNode->ioOut.now.bVal=
        (pTuyuanNode->pInArr0->now.bVal)
        ||(pTuyuanNode->pInArr1->now.bVal)
        ||(pTuyuanNode->pInArr2->now.bVal)
        ||(pTuyuanNode->pInArr3->now.bVal);


}


/*    5输入或门图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


__inline__  static   void     RE_5OrTuyuanScan(NODE *pElemScanNode)
{

    Or5_Scan_Node_Type    *  pTuyuanNode;

    pTuyuanNode=(Or5_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    pTuyuanNode->ioOut.now.bVal=
        (pTuyuanNode->pInArr0->now.bVal)
        ||(pTuyuanNode->pInArr1->now.bVal)
        ||(pTuyuanNode->pInArr2->now.bVal)
        ||(pTuyuanNode->pInArr3->now.bVal)
        ||(pTuyuanNode->pInArr4->now.bVal);

}


/*    6输入或门图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


__inline__  static   void     RE_6OrTuyuanScan(NODE *pElemScanNode)
{

    Or6_Scan_Node_Type    *  pTuyuanNode;

    pTuyuanNode=(Or6_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    pTuyuanNode->ioOut.now.bVal=
        (pTuyuanNode->pInArr0->now.bVal)
        ||(pTuyuanNode->pInArr1->now.bVal)
        ||(pTuyuanNode->pInArr2->now.bVal)
        ||(pTuyuanNode->pInArr3->now.bVal)
        ||(pTuyuanNode->pInArr4->now.bVal)
        ||(pTuyuanNode->pInArr5->now.bVal);


}


/*    7输入或门图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


__inline__  static   void     RE_7OrTuyuanScan(NODE *pElemScanNode)
{

    Or7_Scan_Node_Type    *  pTuyuanNode;

    pTuyuanNode=(Or7_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    pTuyuanNode->ioOut.now.bVal=
        (pTuyuanNode->pInArr0->now.bVal)
        ||(pTuyuanNode->pInArr1->now.bVal)
        ||(pTuyuanNode->pInArr2->now.bVal)
        ||(pTuyuanNode->pInArr3->now.bVal)
        ||(pTuyuanNode->pInArr4->now.bVal)
        ||(pTuyuanNode->pInArr5->now.bVal)
        ||(pTuyuanNode->pInArr6->now.bVal);

}



/*    8输入或门图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


__inline__  static   void     RE_8OrTuyuanScan(NODE *pElemScanNode)
{

    Or8_Scan_Node_Type    *  pTuyuanNode;

    pTuyuanNode=(Or8_Scan_Node_Type    *)pElemScanNode->pTuyuan;

    pTuyuanNode->ioOut.now.bVal=
        (pTuyuanNode->pInArr0->now.bVal)
        ||(pTuyuanNode->pInArr1->now.bVal)
        ||(pTuyuanNode->pInArr2->now.bVal)
        ||(pTuyuanNode->pInArr3->now.bVal)
        ||(pTuyuanNode->pInArr4->now.bVal)
        ||(pTuyuanNode->pInArr5->now.bVal)
        ||(pTuyuanNode->pInArr6->now.bVal)
        ||(pTuyuanNode->pInArr7->now.bVal);

}



/*    多于8个输入的多输入或门图元的扫描函数**/
/*    功能:扫描处理
*/
/*    参数， pElemScanNode图元的操纵的扫描数据节点指针***************************/
/*    返回值，无   */


extern void     RE_MultiOrTuyuanScan(NODE *pElemScanNode);

#endif



