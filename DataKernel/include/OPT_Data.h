/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                  */
/*                                                                              */
/*      OPT_Data.h                                    1.0                       */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了光纵数据模块的头文件                                           */
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
/*         张云       2006.2.8                创建文件1.0版本              */
/*                                                                            */
/*                                                                              */
/********************************************************************************/


#ifndef OPT_DATA_H
#define OPT_DATA_H


#include <vxWorks.h>
#include  "realdatadef.h"
#include "dspai.h"
#if defined(EDP_01_02_BUILD)
#include "spiio.h"
#elif defined(EDP03_BUILD) || defined(EXCITE_BUILD)
#include "io_Drv.h"
#endif

#include  "OPT_SamInterface.h"
#ifdef	__cplusplus
extern "C" {
#endif


#define   OPT_MAX_DI_BUF_LEN     ((MAX_MOD_NUM*MAX_DI_PER_MOD*4+7)/8)     /*光纵通道机箱DI缓冲大小，以1字节为单位  */
#define   OPT_MAX_DO_BUF_LEN     ((MAX_MOD_NUM*MAX_DO_PER_MOD*4+7)/8)     /*光纵通道机箱DO缓冲大小，以1字节为单位  */
#define MAX_OPS_DELAY_NUM 7   /* 对侧最大延迟点数 */

/* 光纵机箱IO板信息结构 */
typedef struct
{
    SUB_MOD_TYPE type;                  /* 模件类型 */
    uint16_t unDiChNum;                 /* DI个数 */
    uint16_t unDoChNum;                 /* DO个数 */
} OPT_IO_MOD_INFO;

/* 光纵机箱DI句柄结构 */
typedef struct
{
    int  iOptCh;          /*所在光纵通道号，0为光纵通道1，1为光纵通道2  */
    uint8_t ucMod;        /*所属模件地址  */
    uint8_t ucHdCh;       /*所属模件中的通道号  */
    uint8_t aucFilt[4];
    uint8_t *pucDiStsPos;    /*DI在光纵通道DI接收缓冲以Byte为单位的位置指针  */
    uint8_t ucDiStsMsk;    /*DI在光纵通道DI接收缓冲相应byte位置的相应屏蔽位  */
} OPT_DI_HND;

/* 光纵机箱DO句柄结构 */
typedef struct
{
    int  iOptCh;          /*所在光纵通道号，0为光纵通道1，1为光纵通道2  */
    uint8_t ucMod;        /*所属模件地址  */
    uint8_t ucHdCh;       /*所属模件中的通道号  */
    uint8_t *pucDoStsPos;    /*DO在光纵通道DO发送缓冲以byte为单位的位置  */
    uint8_t ucDoStsMsk;    /*DO在光纵通道DO发送缓冲相应byte位置的相应屏蔽位  */
} OPT_DO_HND;

/*整个光纵通道虚拟机箱IO信息结构定义  */
typedef  struct
{
    OPT_IO_MOD_INFO   aOptIOModInfo[MAX_MOD_NUM];/* 光纵通道虚拟机箱IO模件信息数组,按地址0开始 */

    int iOptDiNum_g;                 /* 光纵通道虚拟机箱DI总数 */
    OPT_DI_HND   ahOptDiHandle[MAX_MOD_NUM*MAX_DI_PER_MOD];  /*光纵通道虚拟机箱所有DI句柄数组  */
    uint8_t aucOptDiSts_g[OPT_MAX_DI_BUF_LEN];/*光纵通道虚拟机箱的所有DI数据缓冲  */

    int iOptDoNum_g;                 /* 光纵通道虚拟机箱DO总数 */
    OPT_DO_HND   ahOptDoHandle[MAX_MOD_NUM*MAX_DO_PER_MOD];  /*光纵通道虚拟机箱所有DO句柄数组  */
    uint8_t aucOptDoSts_g[OPT_MAX_DO_BUF_LEN];/*光纵通道虚拟机箱的所有DO数据缓冲,  */
}  OPT_BOX_IO_INFO;



typedef struct
{
    /*张云为光纵添加,光纵通道状态db定义 */
    u_int uiTotalCh;
    OPT_CH_STS *pBufBgn;
    OPT_CH_STS *pBufEnd;
    uint32_t ulBufLen;
    u_int uiChBytes;
    uint32_t ulBufBytes;
} OPT_CH_STS_DB;



/* 逻辑图快速任务的光纵发送所需信息定义 2006-11-11日  张云*/
typedef struct
{
    BOOL  bFastLogrpTaskIsDrive;   /*快速逻辑图任务已经驱动标志  */
    uint32_t  ulFastLogrpTaskAiCnt;   /*快速逻辑图任务此时对应的采样计数器AICNT  */
} OPT_FAST_LOGRP_TASK_SEND_INFO;



extern  OPT_BOX_IO_INFO   aOptBoxIoInfo_g[2];/*光纵通道虚拟机箱IO板信息，0为光纵通道1，1为光纵通道2  */

/* 逻辑图快速任务的光纵发送所需信息变量定义 ，需要赋初值，2006-11-11日  张云 */
extern  OPT_FAST_LOGRP_TASK_SEND_INFO  OptFastLogrpTaskSendInfo_g;

extern OPT_CH_STS_DB    optstsdb_g;    /*2006-2-10  为光纵添加  */

/* 功能：获得光纵虚拟机箱当前最新有效的通道采样点AICNT号,  2006-11-12日张云修改
   参数：iOptCh  光纵通道号，0代表光纵通道1，1代表光纵通道2，其他无效
         pRtFastTaskMatchAiCnt,供返回对侧快速任务的匹配的AICNT
         ulLocalCnt,本地采样节拍
         pulOptNewestValidAiCnt, 最新有效节拍
   返回值：返回该光纵通道最新有效的AI CNT
*/
extern uint32_t OPT_AI_Cnt(int iOptCh, uint32_t *pRtFastTaskMatchAiCnt, uint32_t ulLocalCnt, uint32_t *pulOptNewestValidAiCnt);

/*功能：获得光纵虚拟机箱的每秒丢帧统计
  参数： iOptCh  光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
         pulRtCalcLostFrmNumPerSec，供返回新的每秒丢帧统计数据
  返回：为真，表示本次有新的统计数据，为FALSE，表示本次无新统计数据，为脉冲信号，只供平台使用*/
BOOL       OPT_Ch_CalcLostFrm(int  iOptCh,uint32_t  * pulRtCalcLostFrmNumPerSec);


/* 初始化光纵虚拟机箱的IO
 * 参数：   iOptChNum ,光纵通道号，1为光纵通道1，2为光纵通道2，其他无效
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS  OPT_IO_Initialize(int  iOptCh);


/* 取得光纵虚拟机箱AI逻辑通道和预处理数据指针
 * 参数：   pvAiMod，用来索引光纵AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulSmplClk，同步到本机的递增采样时钟，该时钟在同步脉冲到来的时刻清零
 *          ppxWr，用来返回指向该光纵AI引擎的第0个预处理通道数据的指针
 *          ppstsWr,用来返回指向该光纵AI引擎的光纵通道状态的指针
 *          pAiCnt,用来返回指向该光纵AI引擎的光纵通道数据此刻对应的AICNT
 *          ucLastRecvCloseSamCnt, 最接近点.
 *          ucAllRltDif,延迟点数.
 * 返回值： 指向该光纵AI引擎的第0个逻辑采样通道数据的指针
 * 注意：   要求AI引擎按照采样次序调用此函数获得指针来写实时数据，
 *          AI引擎可以对指针进行直接的写操作（如果没有预处理通道，则*ppxWr无效），
 *          内部数据按照通道优先的方式进行组织 */
float *OPT_AI_Dat_P(void *pvAiMod, uint32_t ulSmplClk, COMPLEX **ppxWr,OPT_CH_STS **ppstsWr,uint32_t *pAiCnt, int8_t ucAllRltDif, uint8_t ucLastRecvCloseSamCnt);


/* 报告光纵通道虚拟机箱AI引擎完成一次数据刷新  2006-11-12日 张云修改
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulAiCnt,该数据刷新对应的本机AI采样时刻
            ulFastTaskMatchAiCnt,对侧快速任务中间结果对应到本侧的AICNT
 * 返回值： 无 */
void OPT_End_Ai_Wr(void *pvAiMod,uint32_t   ulAiCnt,uint32_t  ulFastTaskMatchAiCnt);

/* 初始化光纵虚拟机箱DI通道
 * 参数：   iOptCh,光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
            iModAddr，模块硬件地址
 *          uiCh，在本模件内的DO物理通道号，从0开始
 *          ulFilt，去抖动时间，单位us
 * 返回值： 用来索引DI通道的void指针，或者NULL表示调用出错 */
void *OPT_Init_DI(int iOptCh,int iModAddr, u_int uiCh, uint32_t ulFilt);

/* 初始化光纵虚拟机箱DO通道
 * 参数：   iOptCh,光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
 *          iModAddr，模件硬件地址
 *          uiCh，在本模件内的DO物理通道号，从0开始
 * 返回值： 用来索引DO通道的void指针，或者NULL表示调用出错 */
void *OPT_Init_DO(int iOptCh,int iModAddr, u_int uiCh);


/* 初始化光纵虚拟机箱AI源的AO通道
 * 参数：   iOptCh,光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
 *          uiCh，在光纵虚拟机箱AO模件内的AO物理通道号，从0开始
            uiSrcAiCh,该AO所代表的本地机箱AI的物理通道号
            fSrcAiPhyCoff,该AO所对应的本机AI的物理比例系数，已经乘过增益,2006-11-26 张云
 * 返回值： EP_STATUS  成功与否
            EP_SUCCESS,成功，其他失败
*/
EP_STATUS  OPT_Init_AI_Src_AO(int iOptCh,int  iSrcType, u_int uiCh,u_int  uiSrcAiCh,float fSrcAiPhyCoff);


/* 初始化光纵虚拟机箱中间结果源的AO通道
 * 参数：   iOptCh,光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
 *          pElemIOSrc,逻辑图中间结果输出指针
 * 返回值： EP_STATUS  成功与否
            EP_SUCCESS,成功，其他失败
*/
EP_STATUS  OPT_Init_Mid_Src_AO(int iOptCh,int  iSrcType, u_int uiCh, void  *pElemIOSrc);


/* 读取光纵虚拟机箱DI数据实时状态
 * 参数     pvDiCh，用来索引DI数据元素的void指针，应该是OPT_Init_DI的返回值
 * 返回值： 此DI通道的当前状态，TRUE=闭合；FALSE=打开 */
BOOL OPT_Get_DI(void *pvDiCh);

/* 控制光纵虚拟机箱DO数据实时输出
 * 参数：   pvDoCh，用来索引DO数据元素的void指针，应该是OPT_Init_DO的返回值
 *          bClose，TRUE=控合；FALSE=控分
 * 返回值： 无 */
void OPT_Set_DO(void *pvDoCh, BOOL bClose);


/*光纵虚拟机箱是否配置了DI，DO
  参数，iOptCh,光纵通道号，0代表通道1，1代表通道2
  返回：TRUE，代表该机箱配置了DI，DO
        FALSE，代表该机箱未配置任何DI，DO*/
BOOL   OPT_BoxIsCfgDIO(int iOptCh);

/* 设置快速逻辑图任务的所需的光纵发送信息   2006-11-11日 张云
      iScanTaskNo,扫描任务号，0为快速保护任务
      ulTaskScanAiCnt，该任务此时对应的采样点
    返回：EP_SUCCESS,成功
          其他，错误
    */
EP_STATUS  OPT_SetFastLogrpTaskSendInfo(int  iScanTaskNo,uint32_t  ulTaskScanAiCnt);


/*由AICNT计数器，得到该计数器对应的SAM CLK， 2006-11-12日 张云
  参数  pvAiMod，用来索引光纵AI引擎的void指针，应该由本模块在初始
               化AI通道的时候提供给底层I/O
        ulScanAiCnt,待查询的采样计数器AICNT
  返回  该采样计数器对应的SAMCLK
注意：要求该AICNT比最新的AICNT小  */
uint8_t  OPT_GetMatchSamClkByAiCnt(void *pvAiMod,uint32_t  ulScanAiCnt);

/***********************************************************************
* OPT_Chg_AI_Src_AO_Coff - 更新OPT AO系数
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS OPT_Chg_AI_Src_AO_Coff(
    int iOptCh,		/* 光纵通道号 */
    float fSrcAiPhyCoff				/* 通道系数 */
);

/***********************************************************************
* OPT_InitAOCfgCoff - 初始化光纵AO计数
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS OPT_InitAOCfgCount(
    int iOptChNum		/* 光纵通道号，0为光纵通道1，1为光纵通道2，其他无效 */
);



/* 功能：获得光纵虚拟机箱当前通信是否正常,2010-1-11日 张云
   参数：iOptCh  光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
         pRtChValidAiCnt,供返回通信正常时,最新接收同步后的采样节拍
   返回值：该光纵通道当前通信正常与否
           最近有接收,通信正常，则返回TRUE
           否则返回FALSE
   注意：
          供录波模块调用
*/
BOOL       OPT_ChIsComNormal(
    int  iOptCh,
    uint32_t  *pRtChValidAiCnt
);

#ifdef	__cplusplus
}
#endif
#endif