/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*       GO_Interface.h                                    1.0                      */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护模块和61950的GOOSE模块的接口头文件                       */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                        */
/*                                                                              */
/*         张云       2007.3.28                创建文件1.0版本                 */
/*                                                                             */
/*                                                                              */
/********************************************************************************/

#ifndef GO_INTERFACE_H
#define GO_INTERFACE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include   "edpbase.h"
#include "iecgoose.h"


#define   GO_TASK_BUF_SIZE   50000    /*goose任务所需的BUF SIZE大小，用于BRGB的空间，  */
#define   AO_DATA_PUB_CHG_TH    (0.001)/*2007-6-25日 张云，AO数据PUB时的变化门槛  */
#define   MAX_ALLOW_SUB_GO_NUM    64   /*允许最大的sub goose个数，2007-7-3  */
//#define   MAX_GSE_NET_CNT    8         /*2007-7-17日 张云, 允许保护goose的最大网络个数 */

/* 保护启动之前的61850解析成功标志 */
extern  BOOL   bInit61850BfRelayIsSuccess_g;

/*保护启动之前初始化61850的功能
  参数  无
  返回 ： EP_SUCCESS,成功
          其他失败*/
EP_STATUS   GO_Init61850BfRelay();


/*根据Ai Num,查询所在数据源ACTIVE GOOSE的DA INDEX
   参数： iGoSrcType，   ACTIVE GOOSE源类型，目前只能为SAME_POLE_GO_SRC_TYPE
          iAiNum,        在该数据源active goose的Ai序号，序号从1开始，
          piRtDaIndex,   供返回的在GOOSE da 中的Index
          ppSubMapData, Map Data指针存储地址.
   返回， TRUE，匹配成功
          FALSE，匹配失败*/
BOOL   GO_QueryActiveGoDaIdxByAiNum
(uint32_t iGoSrcType,
 int      iAiNum,
 int *piRtDaIndex,
 void **ppSubMapData);

/*根据Di Num,查询所在数据源ACTIVE GOOSE的DA INDEX
   参数： iGoSrcType，   ACTIVE GOOSE源类型，目前只能为SAME_POLE_GO_SRC_TYPE和HDL_BOX_GO_SRC_TYPE
          iDiNum,        在该数据源active goose的Di序号，序号从1开始，
          piRtDaIndex,   供返回的在GOOSE da 中的Index
          ppSubMapData, Map Data指针存储地址.
   返回， TRUE，匹配成功
          FALSE，匹配失败*/
BOOL   GO_QueryActiveGoDaIdxByDiNum(uint32_t iGoSrcType,
                                    int      iDiNum,
                                    int  *   piRtDaIndex,
                                    int *piSubNum,
                                    void **ppSubMapData,
                                    int *piTDaIndex,
                                    void **ppTSubMapData,
                                    VALUETYPE *pValueType,
                                    int **ppiSubYabanIndex,
                                    int  *   piRtDaVtIndex
                                   );

BOOL GO_QueryActiveGoTimeSourceDiIdxByDoNum(uint32_t iGoSrcType,
        int iDoNum,
        int *pindex
                                           );


/* 根据ACTIVE GOOSE中的sub goose中的DA INDEX,获得相应的AI值
   参数，iDaIndx  ，在DA集中的INDEX，
         pfRtVal， 供返回的AI的VAL
         pSubMapData, Map Data指针.
   返回：TRUE，成功
         FALSE，失败 */
BOOL   GO_GetActiveGoAIValByDaIndx
(
    int  iDaIndx,
    void *pSubMapData,
    float *pfRtVal
);

/* 根据ACTIVE GOOSE中的sub goose中的DA INDEX,获得相应的DI值
   参数，iDaIndx  ，在DA集中的INDEX，
         pbRtVal， 供返回的DI的VAL
         pSubMapData, Map Data指针.
   返回：TRUE，成功
         FALSE，失败 */
BOOL   GO_GetActiveGoDIValByDaIndx
(
    int  iDaIndx,
    void *pSubMapData,
    BOOL *pbRtVal
);


/* 测控实现的记录googse状态，我需要实现一个空操作
   参数，iSubIndex，sub goose 序号
         iStsNum，STS序号
         pucIedName，此IED名称
   返回，空 */
void Set_61850_goose_status(int  iSubIndex, int  iStsNum, char  * pucIedName, unsigned char ucNetNo);

#ifdef	__cplusplus
}
#endif

#endif                                  /* GO_INTERFACE_H */
