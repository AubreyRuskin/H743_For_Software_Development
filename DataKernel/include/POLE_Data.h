/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                  */
/*                                                                              */
/*      POLE_Data.h                                    1.0                       */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了同杆并架数据模块的头文件                                           */
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


#ifndef POLE_DATA_H
#define POLE_DATA_H


#ifdef	__cplusplus
extern "C" {
#endif

#define    POLE_PUB_REFRESH_INTVL    5  /*同杆并架发布刷新间隔毫秒数 */


/* 控制同杆并架虚拟机箱DO数据实时输出
 * 参数：   pvDoCh，用来索引DO数据元素的void指针，应该是POLE_Init_DO的返回值
 *          bClose，TRUE=控合；FALSE=控分
 * 返回值： 无 */
void POLE_Set_DO(void *pvDoCh, BOOL bClose);


/* 读取同杆并架虚拟机箱DI数据实时状态
 * 参数：   pvDiCh，用来索引DI数据元素的void指针，应该是POLE_Init_DI的返回值
 * 返回值： 此DI通道的当前状态，TRUE=闭合；FALSE=打开 */
BOOL POLE_Get_DI(void *pvDiCh);


/* 读取同杆并架虚拟机箱AI数据实时状态
 * 参数：   pvAiCh，用来索引AI数据元素的void指针
 * 返回值： 此DI通道的当前状态，TRUE=闭合；FALSE=打开 */
float POLE_Get_AI(void *pvAiCh);



/* 功能：获得同杆并架数据源的active goose,某AO通道输出的数据  2007-3-27日 张云
   参数：
        uiAONum：		在该数据源active goose的AO序号，序号从1开始，
        pfRtData:               返回的该AO通道的数据
   返回：
        TRUE； 若有匹配数据，则操作成功
        FALSE：操作失败
  	*/

BOOL  POLE_GetActiveGoAoData(uint16_t uiAONum,
                             float *pfRtData);



/* 功能：获得同杆并架数据源的active goose,某DO通道输出的数据  2007-3-27日 张云
   参数：
        uiDONum：		在该数据源active goose的DO序号，序号从1开始，
        pbRtData:               返回的该DO通道的数据
   返回：
        TRUE； 若有匹配数据，则操作成功
        FALSE：操作失败
  	*/

BOOL  POLE_GetActiveGoDoData(uint16_t uiDONum,
                             BOOL *pbRtData);



/* 取得同杆并架机箱AI逻辑通道和预处理数据指针,必须要求和本机的AD采样刷新同时调用
 * 参数：   pvAiMod，用来索引同杆并架机箱AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulSmplClk，本机的递增采样时钟，
 *          ppxWr，用来返回指向该同杆AI引擎的第0个预处理通道数据的指针
 * 返回值： 指向该同杆AI引擎的第0个逻辑采样通道数据的指针
 * 注意：   要求AI引擎按照采样次序调用此函数获得指针来写实时数据，
 *          AI引擎可以对指针进行直接的写操作（如果没有预处理通道，则*ppxWr无效），
 *          内部数据按照通道优先的方式进行组织 */
float *POLE_AI_Dat_P(void *pvAiMod, uint32_t ulSmplClk,COMPLEX **ppxWr);



/* 报告同杆并架虚拟机箱AI引擎完成一次数据刷新 要求此时rdinfo_g.ulCurrAiCnt还没有被本机的采样来刷新
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulAiCnt,该数据刷新对应的本机AI采样时刻
 * 返回值： 无 */
void  POLE_End_Ai_Wr(void *pvAiMod);


/* 刷新同杆并架虚拟机箱的DI数据
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 * 返回值： 无 */
void POLE_Refresh_DI(void *pvAiMod);

/*刷新新的同杆并架数据,必须在本机当地的数据刷新RD_END_AI_WR函数完成之前被调用
  参数：ulSmplClk，本机的递增采样时钟，
  返回   无*/
void  POLE_Read_AI_Data(uint32_t ulSmplClk);


/*同杆并架机箱数据是否有效
  参数：无
  返回值：TRUE：数据有效
          FALSE：数据无效*/
BOOL   POLE_Data_Is_Valid();



/*查询同杆并架数据发布时，数据是否发生变化
  参数  无
  返回,TRUE,数据发生变化
       FALSE，数据未变化  2007-6-25日 张云 */
BOOL  POLE_DataPubIsChgAndSave();




#ifdef	__cplusplus
}
#endif
#endif