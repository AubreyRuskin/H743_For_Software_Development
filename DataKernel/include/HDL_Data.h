/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                  */
/*                                                                              */
/*      HDL_Data.h                                    1.0                       */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了智能操作箱数据模块的头文件                                           */
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
/*         张云       2007.3.28                创建文件1.0版本              */
/*                                                                            */
/*                                                                              */
/********************************************************************************/


#ifndef HDL_DATA_H
#define HDL_DATA_H

#include "vxworks_type.h"

#ifdef	__cplusplus
extern "C" {
#endif


#define    HDL_PUB_REFRESH_INTVL    2  /*智能操作箱发布刷新间隔毫秒数 2007-7-7日 张云修改*/


extern BOOL g_bGooseDiNeedRefresh; /* 更新标识 */

/* 控制智能操作箱DO数据实时输出
 * 参数：   pvDoCh，用来索引DO数据元素的void指针，应该是HDL_Init_DO的返回值
 *          bClose，TRUE=控合；FALSE=控分
 * 返回值： 无 */
void HDL_Set_DO(void *pvDoCh, BOOL bClose);


/* 读取智能操作箱DI数据实时状态
 * 参数：   pvDiCh，用来索引DI数据元素的void指针，应该是HDL_Init_DI的返回值
 *          nIdx, 针对某个DI关联多个GOOSE输入情况下的索引信息
 *          usQuality, 品质因素.
 * 返回值： 此DI通道的当前状态，TRUE=闭合；FALSE=打开 */
BOOL HDL_Get_DI(void *pvDiCh, int nIdx, uint16_t *usQuality);

/* 读取智能操作箱DI数据实时状态(硬件配置句柄)
 * Para:
 *     pvDiCh, DI通道句柄.
 *     pulChgTime, 变位时采样节拍.
 *     usQuality, 品质存储地址.
 *     pbFilterSts, 消抖状态.
 * Return:
 *     此DI通道的当前状态,TRUE=闭合;FALSE=打开.
 */
extern BOOL HDL_Get_DI_Many(void *pvDiCh, uint32_t *ulChgNextCnt, uint16_t *pQuality,
                            BOOL *pbFilterSts);

/* 功能：获得智能操作箱数据源的active goose,某DO通道输出的数据  2007-3-27日 张云
   参数：
        uiDONum：		在该数据源active goose的DO序号，序号从1开始，
        pbRtData:               返回的该DO通道的数据
   返回：
        TRUE； 若有匹配数据，则操作成功
        FALSE：操作失败
  	*/

BOOL  HDL_GetActiveGoDoData(uint16_t uiDONum,
                            int *pbRtData);


/* 取得智能操作箱AI逻辑通道和预处理数据指针,必须要求和本机的AD采样刷新同时调用
 * 参数：   pvAiMod，用来索引智能操作箱AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulSmplClk，本机的递增采样时钟，
 *          ppxWr，用来返回指向该同杆AI引擎的第0个预处理通道数据的指针，这里应该返回为NULL
 * 返回值： 指向该智能操作箱AI引擎的第0个逻辑采样通道数据的指针，这里应该返回为NULL
 * 注意：   要求AI引擎按照采样次序调用此函数获得指针来写实时数据，
 *          AI引擎可以对指针进行直接的写操作（如果没有预处理通道，则*ppxWr无效），
 *          内部数据按照通道优先的方式进行组织， */
float *HDL_AI_Dat_P(void *pvAiMod, uint32_t ulSmplClk,COMPLEX **ppxWr);


/* 报告智能操作相AI引擎完成一次数据刷新 要求此时rdinfo_g.ulCurrAiCnt还没有被本机的采样来刷新
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulAiCnt,该数据刷新对应的本机AI采样时刻
 * 返回值： 无 */
void  HDL_End_Ai_Wr(void *pvAiMod);


/* 刷新智能操作相的DI数据
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 * 返回值： 无 */
void HDL_Refresh_DI(void *pvAiMod);

/*刷新新的智能操作箱数据,必须在本机当地的数据刷新RD_END_AI_WR函数完成之前被调用
  参数：ulSmplClk，本机的递增采样时钟，
  返回   无*/
void  HDL_Read_AI_Data(uint32_t ulSmplClk);



/*智能操作箱数据是否有效
    ulScanTaskNo, 扫描任务号.
    ulGrpScanDriveInterval, 扫描间隔.
  返回值：TRUE：数据有效
          FALSE：数据无效*/
BOOL   HDL_Data_Is_Valid(uint32_t ulScanTaskNo, uint32_t ulGrpScanDriveInterval);


/*查询智能操作箱数据发布时，数据是否发生变化
  参数  无
  返回,TRUE,数据发生变化
       FALSE，数据未变化  2007-6-25日 张云 */
BOOL  HDL_DataPubIsChgAndSave();

/* 功能：获得某AO通道输出的数据
   参数：
        uiAONum：		在该数据源active goose的AO序号，序号从1开始，
        pfRtData:               返回的该AO通道的数据
   返回：
        TRUE； 若有匹配数据，则操作成功
        FALSE：操作失败
  	*/

BOOL  HDL_GetActiveGoAoData(uint16_t uiAONum, float *pfRtData);

/* 获取GOOSE状态.
 * Para:
 *     iDaIndx, 序号.
 *     ultime, 时间保存地址.
 * Return:
 *     状态.
 */
BOOL GO_GetActiveGoTByDaIndx(int  iDaIndx, void *pSubMapData, US_CNT_UTC_TIME *uttime);

BOOL GO_GetTPacketBySubMapData(void *pSubMapData, US_CNT_UTC_TIME *uttime);

extern BOOL HDL_Get_T(void *pvDiCh, US_CNT_UTC_TIME *uttime, int nIdx);

/* 功能：获得智能操作箱数据源的active goose,某DO通道变位时间
   参数：
        uiDONum：		在该数据源active goose的DO序号，序号从1开始，
        pbRtData:               返回的该DO通道的数据
   返回：
        TRUE； 若有匹配数据，则操作成功
        FALSE：操作失败
  	*/

extern BOOL  HDL_GetActiveGoDoTData(uint16_t uiDONum, US_CNT_UTC_TIME *ptmTData);

BOOL **Trans_Goose_Comm_Status();

/* 消抖并返回当前确认DI状态(周期调用).
 * Para:
 *     pvDiCh, 通道句柄.
 *     pulChgTime, 变位时采样节拍.
 *     pusQuality, 品质.
 * Return:
 *     该DI通道的当前状态,TRUE=闭合;FALSE=打开.
 */
extern BOOL Hdl_Filt_And_Get_DI(void *pvDiCh, uint32_t *ulChgNextCnt, uint16_t *pusQuality);

/* 重置消抖时间.
 * Para:
 *     iDiNum, DI通道号.
 *     iSubDaIdx, 多合一序号.
 * Return:
 *     NONE.
 */
extern void HDL_Reset_Filt(int iDiNum, int iSubDaIdx);

/* 查看当前控制块中是否配置当前端口
 * Para:
 *     SubNo, 控制块序号.
 *     NetNo, 端口序号.
 * Return:
 *     TRUE, 使用了当前端口;  FALSE, 没有使用
 */
BOOL Get_Goose_Sub_Net_Cfg(int SubNo,int NetNo);

extern BOOL HDL_Get_T_Many(void *pvDiCh, US_CNT_UTC_TIME  *uttime);

/* 设置GOOSE DI是否需要刷新标识
 * Para:
 *     bStatus, 设置的GOOSE DI刷新状态
 * Return:
 *     None.
 */
extern void HDL_SetGooseDiNeedRefresh(BOOL bStatus);

/* 获取GOOSE DI是否需要刷新标识
 * Para:
 *     pStatus, 获取的GOOSE DI刷新状态返回指针
 * Return:
 *     None.
 */
void HDL_GetGooseDiNeedRefresh(BOOL *pStatus);


#ifdef	__cplusplus
}
#endif
#endif
