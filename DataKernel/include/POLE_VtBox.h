/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                  */
/*                                                                              */
/*      POLE_VtBox.h                                    1.0                       */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了同杆并架机箱初始化模块的头文件                                   */
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



#ifndef POLE_VTBOX_H
#define POLE_VTBOX_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef EDP03_BUILD		/* 用于EDP03平台 */
#include "io_Drv.h"
#endif

#if defined(EDP_01_02_BUILD)
#include "spiio.h"			/* 2007-6-23日 张云 */
#endif

#include  "realdatadef.h"/*2007-6-23日 张云  */
#include  "dspai.h"/*2007-6-23日 张云  */
#include  "GO_Interface.h"

#define   POLE_DO_PASSWORD  0XADBC1234

#define   MAX_POLE_AO_NUM  40
#define   MAX_POLE_AI_NUM  40


/* 同杆并架机箱IO板信息结构 */
typedef struct
{
    SUB_MOD_TYPE type;                  /* 模件类型 */
    uint16_t unDiChNum;                 /* DI个数 */
    uint16_t unDoChNum;                 /* DO个数 */
} POLE_IO_MOD_INFO;

/* 同杆并架机箱DI句柄结构 */
typedef struct
{
    uint8_t ucMod;        /*所属模件地址  */
    uint8_t ucHdCh;       /*所属模件中的通道号  */
    int     iSubDaIdx   ;     /*在goose SUB DA集中的索引号  */
    void *pSubMapData;  /* SUB map数据结构,单个数据 */
    VALUETYPE vtValType;
    int iSubNum; /* SUB sequence number. */
    uint8_t aucFilt[4];
} POLE_DI_HND;

/* 同杆并架机箱DO句柄结构 */
typedef struct
{
    uint8_t ucMod;        /*所属模件地址  */
    uint8_t ucHdCh;       /*所属模件中的通道号  */
    uint32_t ulPassWd;    /* 输出密码 */
    BOOL     bCurVal;     /*输出的当前值  */
    BOOL     bLstPubVal;  /*上次PUB时的值   2007-6-25日 张云  */
} POLE_DO_HND;

/*整个同杆并架虚拟机箱IO信息结构定义  */
typedef  struct
{
    POLE_IO_MOD_INFO   aPoleIOModInfo[MAX_MOD_NUM];/* 同杆并架虚拟机箱IO模件信息数组,按地址0开始 */

    int iPoleDiNum;                 /* 同杆并架虚拟机箱DI总数 */
    POLE_DI_HND   ahPoleDiHandle[MAX_MOD_NUM*MAX_DI_PER_MOD];  /*同杆并架虚拟机箱所有DI句柄数组  */

    int iPoleDoNum;                 /* 同杆并架虚拟机箱DO总数 */
    POLE_DO_HND   *  aphPoleDoIdx[MAX_MOD_NUM*MAX_DO_PER_MOD];  /*同杆并架虚拟机箱所有DO句柄索引数组  */
    POLE_DO_HND   ahPoleDoHandle[MAX_MOD_NUM*MAX_DO_PER_MOD];  /*同杆并架虚拟机箱所有DO句柄数组  */

}  POLE_BOX_IO_INFO;


/* 同杆并架机箱AI句柄结构 */
typedef struct
{
    uint8_t ucHdCh;       /*所属模件中的通道号  */
    float   fCoff;          /* 系数 */
    uint8_t ucFiltNum;    /* 滤波算法号，按照规约中的定义 */
    int     iSubDaIdx   ;     /*在goose SUB DA集中的索引号  */
    float   fBufVal;          /*2007-10-29 DQ: 该通道缓存浮点值 */
    void *pSubMapData;  /* SUB map数据结构,单个数据 */
} POLE_AI_HND;


/*同杆并架机箱AI配置  */
typedef struct
{
    int  iPoleAINum;         /*总的AI总数  */
    POLE_AI_HND  * apPoleBoxAIIdx[MAX_POLE_AI_NUM];/*按机箱AI的配置的逻辑次序存储通道指针，便于索引 */
    POLE_AI_HND   aPoleBoxAiHdl[MAX_POLE_AI_NUM];
}   POLE_BOX_AI_CFG;


/* 同杆并架机箱AO通道配置 */
typedef struct
{
    uint8_t ucAOHdCh;                         /* AO的虚拟机箱AO物理通道号，从0开始 */
    int    iAOSrcType;                          /*2为逻辑图中间结果来源,其他保留  */
    void    *pElemSrc;                         /* 若是中间结果来源,逻辑图中间结果IO指针*/
    float     fLstPubVal;                       /*上次PUB时的值   2007-6-25日 张云  */
} POLE_AO_CFG;

/*同杆并架AO配置  */
typedef struct
{
    int  iPoleAONum;         /*总的AO总数  */
    int  iOptMidSrcAONum;    /*逻辑图中间结果的AO总数  */
    POLE_AO_CFG  * apPoleBoxAoIdx[MAX_POLE_AO_NUM];/*按机箱AO的配置的逻辑次序存储通道指针，便于索引 */
    POLE_AO_CFG   aPoleBoxAoCfg[MAX_POLE_AO_NUM];
}   POLE_BOX_AO_CFG;


extern  int iPoleDiNum_g;
extern  int iPoleDoNum_g;
extern  int iPoleAiNum_g;
extern  int iPoleAoNum_g;

extern  POLE_BOX_AO_CFG   PoleBoxAoCfg_g;  /*同杆并架机箱AO配置  */
extern  POLE_BOX_IO_INFO  PoleBoxIOInfo_g;/*同杆并架机箱IO配置  */
extern  POLE_BOX_AI_CFG   PoleBoxAICfg_g;  /*同杆并架机箱AI配置  */

extern BOOL   bPoleBoxIsInit_g;  /*同杆并架机箱被初始化标志  */

extern  int   iPoleSubGoNum_g;   /*同杆并架机箱对应的sub GOOSE 个数 2007-7-3   */
extern  int   aiPoleSubGoIdx_g[MAX_ALLOW_SUB_GO_NUM];/*同杆并架机箱对应的sub GOOSE index 无效 */

/* 初始化（并启动）同杆并架虚拟机箱
 * 参数：   uiSmplRate，采样速率
 *          uiSysFreq，系统频率
 *          pvAiMod，该模块（机箱负责的所有AI采集/计算通道）的句柄
 *          uiLgcCh，采样的逻辑通道数
 *          plgccfg，指向逻辑通道配置数组第0个元素的指针，数组元素有unLgcCh个
 *          uiCalcCfg，预处理通道配置数
 *          pcalccfg，指向预处理通道配置数组第0个元素的指针，数组元素uiCalcCfg个
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，机箱出错 */
EP_STATUS Init_Pole_Box(u_int uiSmplRate, u_int uiSysFreq,
                        void *pvAiMod, u_int uiLgcCh, DSP_LGC_AI_CFG *plgccfg,
                        u_int uiCalcCfg, DSP_CALC_AI_CFG *pcalccfg);

/*  初始化同杆并架AO所有配置
     参数：
          iAOCfgNum:所有AO配置个数
     返回：成功与否*/
EP_STATUS   POLE_InitAOCfg(int  iAOCfgNum);




/* 初始化同杆并架虚拟机箱中间结果源的AO通道
 * 参数：   iSrcType,来源类型
            uiCh，模件中的通道号
 *          pElemIOSrc,逻辑图中间结果输出指针
 * 返回值： EP_STATUS  成功与否
            EP_SUCCESS,成功，其他失败
*/
EP_STATUS  POLE_Init_Mid_Src_AO(int  iSrcType, u_int uiCh, void  *pElemIOSrc);

/*通知同杆并架AO初始化完成,在逻辑图初始化完成之后，逻辑图运行之前调用
  参数：无
  返回：成功与否
*/
EP_STATUS   POLE_AOCfgInitFinish();


/* 初始化同杆并架虚拟机箱的IO
 * 参数：
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS  POLE_IO_Initialize();


/* 初始化同杆并架虚拟机箱DI通道
 * 参数：
            iModAddr，模块硬件地址
 *          uiCh，在本模件内的DO物理通道号，从0开始
 *          ulFilt，去抖动时间，单位us
 * 返回值： 用来索引DI通道的void指针，或者NULL表示调用出错 */
void *POLE_Init_DI(int iModAddr, u_int uiCh, uint32_t ulFilt);


/* 初始化同杆并架虚拟机箱DO通道
 * 参数：
 *          iModAddr，模件硬件地址,
 *          uiCh，在本模件内的DO物理通道号，从0开始
 * 返回值： 用来索引DO通道的void指针，或者NULL表示调用出错 */
void *POLE_Init_DO(int iModAddr, u_int uiCh);



#ifdef	__cplusplus
}
#endif

#endif                                  /* POLE_VTBOX_H */
