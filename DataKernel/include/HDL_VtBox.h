/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                  */
/*                                                                              */
/*      HDL_VtBox.h                                    1.0                       */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了智能操作箱初始化模块的头文件                                   */
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



#ifndef HDL_VTBOX_H
#define HDL_VTBOX_H

#ifdef	__cplusplus
extern "C" {
#endif


#define   HDL_DO_PASSWORD  0XDCBA4321

#define   MAX_HDL_AO_NUM  40
#define   MAX_HDL_AI_NUM  200
#define   MAX_DI_VT_TERM_NUM   512  /*2013-7-6 ZY */


#ifdef EDP03_INTELBOX_BUILD		/* 用于EDP03平台 */
#include "io_Drv.h"
#endif

#if defined(EDP_01_02_BUILD)
#include "spiio.h"			/* 2007-6-23日 张云 */
#endif

#include  "realdatadef.h"/*2007-6-23日 张云  */
#include  "dspai.h"/*2007-6-23日 张云  */
#include  "GO_Interface.h"

/* 智能操作机箱IO板信息结构 */
typedef struct
{
    SUB_MOD_TYPE type;                  /* 模件类型 */
    uint16_t unDiChNum;                 /* DI个数 */
    uint16_t unDoChNum;                 /* DO个数 */
} HDL_IO_MOD_INFO;

/* 智能操作机箱DI句柄结构 */
typedef struct
{
    uint8_t ucMod;        /*所属模件地址  */
    uint8_t ucHdCh;       /*所属模件中的通道号  */
    int     iSubDaIdx[HDL_DI_MAX_RECV_NUM];     /*在goose SUB DA集中的索引号,超过一个表示该开入包含多个信号输入  */
    int iSubDaVtIdx[HDL_DI_MAX_RECV_NUM];     /*datamap中的虚端子序号,超过一个表示该开入包含多个信号输入  */
    int iSubNum[HDL_DI_MAX_RECV_NUM];							/* SUB sequence number. */
    void *ppSubMapData[HDL_DI_MAX_RECV_NUM];  /* SUB map数据结构,支持多个数据 */
    VALUETYPE vtValType[HDL_DI_MAX_RECV_NUM];

    uint8_t aucFilt[4];
    BOOL   bInvalidDftVal;   /*通信无效时的缺省值,0:默认为分 1：默认为合  2：通信断或检修不一致前的值bValueBfInvalid  */
    BOOL   bInvalidFlag;     /*通信断或检修不一致标志*/
    BOOL bValueBfInvalid;  /* 保存通信断、检修不一致以及压板退出前的值 */
    uint32_t ulFltCfg;  /* 分快/中/慢按采样节拍折算的消抖次数 */
    uint32_t ulFltCfgLine;  /* 分快/中/慢按采样节拍折算的消抖次数,线路在风暴时用 */
    uint32_t ulFltCfgNormal;  /* 分快/中/慢按采样节拍折算的消抖次数,线路在风暴时用 */
    BOOL bSts;  /* 确认状态 */
    BOOL bLstSts;  /* 前状态 */
    uint32_t ulFltCnt;  /* 消抖计数 */
    uint32_t ulTmpNextCnt;  /* 临时保存的变位时采样节拍 */
    BOOL bChgFlag;  /* 变位标志 */

    int     iSubTStampDaIdx[HDL_DI_MAX_RECV_NUM];
    void *ppSubMapTStamp[HDL_DI_MAX_RECV_NUM];
    US_CNT_UTC_TIME utChgTime;
    US_CNT_UTC_TIME utPacketTime;
    uint32_t ulArrFltCnt[HDL_DI_MAX_RECV_NUM];  /* 消抖计数 */
    BOOL bArrLstSts[HDL_DI_MAX_RECV_NUM];  /* 前状态 */
    BOOL bArrChgFlag[HDL_DI_MAX_RECV_NUM];  /* 变位标志 */
    BOOL bArrSts[HDL_DI_MAX_RECV_NUM];  /* 确认状态 */
    uint16_t usArrQuality[HDL_DI_MAX_RECV_NUM]; /* 品质 */
    uint32_t ulArrTmpNextCnt[HDL_DI_MAX_RECV_NUM];  /* 临时保存的变位时采样节拍 */
    int *ppiSubYabanIndex[HDL_DI_MAX_RECV_NUM];
} HDL_DI_HND;

/* 智能操作机箱DO句柄结构 */
typedef struct
{
    uint8_t ucMod;        /*所属模件地址  */
    uint8_t ucHdCh;       /*所属模件中的通道号  */
    uint32_t ulPassWd;    /* 输出密码 */
    int     iCurVal;     /*输出的当前值  */
    int     iLstPubVal;  /*上次PUB时的值   2007-6-25日 张云  */
    int iTimeSrcDiIndex[MAX_TIME_SOURCE_DI_NUM];
    US_CNT_UTC_TIME utChgTime;
    int iHdlDoNum; /* GOOSE DO总序号 */
    int16_t linkNum; /* 对应的压板序号 */
    VALUETYPE vtValType;  /* 数据类型 */
} HDL_DO_HND;

/*整个智能操作机箱IO信息结构定义  */
typedef  struct
{
    HDL_IO_MOD_INFO   aHdlIOModInfo[MAX_MOD_NUM];/* 智能操作机箱IO模件信息数组,按地址0开始 */

    int iHdlDiNum;                 /* 智能操作机箱DI总数 */
    HDL_DI_HND   ahHdlDiHandle[MAX_MOD_NUM*MAX_DI_PER_MOD];  /*智能操作机箱所有DI句柄数组  */
    HDL_DI_HND *pahHdlDiHandle[MAX_MOD_NUM*MAX_DI_PER_MOD]; /* 智能操作机箱所有DI句柄索引数组 */

    int iHdlDoNum;                 /* 智能操作机箱DO总数 */
    HDL_DO_HND   *  aphHdlDoIdx[MAX_MOD_NUM*MAX_DO_PER_MOD];  /*智能操作机箱所有DO句柄索引数组  */
    HDL_DO_HND   ahHdlDoHandle[MAX_MOD_NUM*MAX_DO_PER_MOD];  /*智能操作机箱所有DO句柄数组  */

}  HDL_BOX_IO_INFO;


/* 智能操作机箱AI句柄结构 */
typedef struct
{
    uint8_t ucHdCh;       /*所属模件中的通道号  */
    float   fCoff;          /* 系数 */
    uint8_t ucFiltNum;    /* 滤波算法号，按照规约中的定义 */
    int     iSubDaIdx   ;     /*在goose SUB DA集中的索引号  */
    float   fBufVal;          /*2007-10-29 DQ: 该通道缓存浮点值 */
    void *pSubMapData;  /* SUB map数据结构,单个数据 */
    uint16_t usQuality;  /* 品质因素 */
} HDL_AI_HND;


/*智能操作机箱AI配置  */
typedef struct
{
    int  iHdlAINum;         /*总的AI总数  */
    HDL_AI_HND  * apHdlBoxAIIdx[MAX_HDL_AI_NUM];/*按机箱AI的配置的逻辑次序存储通道指针，便于索引 */
    HDL_AI_HND   aHdlBoxAiHdl[MAX_HDL_AI_NUM];
}   HDL_BOX_AI_CFG;


/* 智能操作机箱AO通道配置 */
typedef struct
{
    uint8_t ucAOHdCh;                         /* AO的虚拟机箱AO物理通道号，从0开始 */
    int    iAOSrcType;                          /*2为逻辑图中间结果来源,其他保留  */
    void    *pElemSrc;                         /* 若是中间结果来源,逻辑图中间结果IO指针*/
    float     fLstPubVal;                       /*上次PUB时的值   2009-4-8日 张云  */
} HDL_AO_CFG;

/*智能操作AO配置  */
typedef struct
{
    int  iHDLAONum;         /*总的AO总数  */
    int  iOptMidSrcAONum;    /*逻辑图中间结果的AO总数  */
    HDL_AO_CFG  * apHdlBoxAoIdx[MAX_HDL_AO_NUM];/*按机箱AO的配置的逻辑次序存储通道指针，便于索引 */
    HDL_AO_CFG   aHdlBoxAoCfg[MAX_HDL_AO_NUM];
}   HDL_BOX_AO_CFG;


/*2013-6-4 ZY 为HMI虚端子状态监视添加*/
/*CPU的GOOSE开入虚端子供界面监视的单端子配置信息结构  */
typedef struct
{
    int  iValType;             /*虚端子单位类型，和内部规约一致，1是单点，2是双点 */
    uint16_t  uiDataSetAppID;   /*关联源端数据集的APPID ，以'\0'结尾*/
    uint8_t   aucDescStr[65];   /*描述字符串，以'\0'结尾 */
    uint8_t   aucYabanIDStr[33];  /*压板ID，以'\0'结尾 */
    void   * pHdl;             /*该虚端子关联的句柄 */
    int    iSubGoIdxSeqNo;     /*关联的SUB对应aiHdlSubGoIdx_g数组中的序号，从0开始 */
    int    iSubNum;           /*该虚端子关联的SUB号 2013-7-6*/
    void   *pSubMapData;       /*该虚端子关联的SUB map数据结构指针2013-7-6*/
}   HDL_VT_DI_TERM_CFG;

/*2013-6-4 ZY 为HMI虚端子状态监视添加*/
/*CPU的GOOSE开入虚端子供界面监视的总体配置信息结构  */
typedef struct
{
    int  iTermCnt;
    HDL_VT_DI_TERM_CFG   aTermCfgArr[MAX_DI_VT_TERM_NUM];/*由于可能虚端子1对多，需要比较多  */
}   HDL_TOTAL_VT_DI_TERM_CFG;


/*2013-6-4 ZY 为HMI虚端子状态监视添加*/
/*CPU的GOOSE开入虚端子供界面监视的单端子状态信息结构  */
typedef struct
{
    uint8_t   ucTermVal;    /*开入虚端子值,分单点（0，1），双点（0，1，2，3） */
    uint8_t   ucTermQuality; /*开入虚端子品质 */
}   HDL_VT_DI_TERM_STS;

/*2013-6-4 ZY 为HMI虚端子状态监视添加*/
/*CPU的GOOSE开入虚端子供界面监视的总体状态信息结构  */
typedef struct
{
    int  iTermCnt;
    HDL_VT_DI_TERM_STS   aTermStsArr[MAX_DI_VT_TERM_NUM];
}   HDL_TOTAL_VT_DI_TERM_STS;


extern  int iHdlDiNum_g;
extern  int iHdlDoNum_g;
extern  int iHDLAiNum_g;
extern  int iHDLAoNum_g;

extern  HDL_BOX_AO_CFG    HdlBoxAoCfg_g;  /*智能操作机箱AO配置  */
extern  HDL_BOX_IO_INFO   HdlBoxIOInfo_g; /*智能操作机箱IO配置  */
extern  HDL_BOX_AI_CFG    HdlBoxAICfg_g;  /*智能操作机箱AI配置  */

extern  BOOL   bHdlBoxIsInit_g;  /*智能操作机箱被初始化标志  */

extern int iHdlSubGoNum_g;   /*智能操作箱对应的sub GOOSE 个数 2007-7-3   (有效的goose个数)*/
extern int iHdlNetNum_g;  /* 最多网口数量 */

extern int iHdlCfgSubGoNum_g;	/*所有goose配置文件配置的sub个数，用于goose告警*/
extern int aiHdlSubGoIdx_g[MAX_ALLOW_SUB_GO_NUM];/*智能操作箱对应的sub GOOSE index 数组 2007-7-3 */
extern int iHdlSubGoStat[MAX_ALLOW_SUB_GO_NUM];				/* Global variable used for sub state. */

/* GOOSE SUB压板状态 */
extern BOOL bSubGoYabanStat[MAX_ALLOW_SUB_GO_NUM];

/* GOOSE SUB所对应压板序号 */
extern int16_t iSubGoYabanSn[MAX_ALLOW_SUB_GO_NUM];

/* 全部为慢速处理DI */
extern BOOL bAllSnowFlag;

/* 初始化（并启动）智能操作箱,目前只支持DI，DO
 * 参数：   uiSmplRate，采样速率
 *          uiSysFreq，系统频率
 *          pvAiMod，该模块（机箱负责的所有AI采集/计算通道）的句柄
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，机箱出错 */
EP_STATUS Init_Hdl_Box(u_int uiSmplRate, u_int uiSysFreq,
                       void *pvAiMod, u_int uiLgcCh, DSP_LGC_AI_CFG *plgccfg,
                       u_int uiCalcCfg, DSP_CALC_AI_CFG *pcalccfg);


/*通知智能操作箱初始化完成,在逻辑图初始化完成之后，逻辑图运行之前调用
  参数：无
  返回：成功与否
*/
EP_STATUS   HDL_AOCfgInitFinish();

/* 初始化智能操作箱的IO
 * 参数：
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS  HDL_IO_Initialize();


/* 初始化智能操作箱DI通道
 * Para:
 *     iModAddr, 模块硬件地址.
 *     uiCh, 在本模件内的DI物理通道号,从0开始.
 *     ulFilt, 消抖时间,单位us.
 *     bInvalidDftVal, 通信无效时缺省值.
 *     ucDIRefreshRate, 调用速度(快/中/慢).
 *     bpPended, 是否悬空.
 * Return:
 *     用来索引DI通道的void指针,或者NULL表示调用出错.
 */
void *HDL_Init_DI(int iModAddr, u_int uiCh, uint32_t ulFilt, BOOL bInvalidDftVal, uint8_t ucDIRefreshRate, BOOL *bpPended);

/* 初始化智能操作相DO通道
 * 参数：
 *          iModAddr，模件硬件地址,
 *          uiCh，在本模件内的DO物理通道号，从0开始
 * 返回值： 用来索引DO通道的void指针，或者NULL表示调用出错 */
void *HDL_Init_DO(int iModAddr, u_int uiCh);

/* 初始化中间结果源的AO通道
 * 参数：   iSrcType,来源类型
            uiCh，模件中的通道号
 *          pElemIOSrc,逻辑图中间结果输出指针
 * 返回值： EP_STATUS  成功与否
            EP_SUCCESS,成功，其他失败
*/
EP_STATUS  HDL_Init_Mid_Src_AO(int  iSrcType, u_int uiCh, void  *pElemIOSrc);

/*  初始化同杆并架AO所有配置
     参数：
          iAOCfgNum:所有AO配置个数
     返回：成功与否*/
extern EP_STATUS   HDL_InitAOCfg(int  iAOCfgNum);

/*功能：得到CPU虚拟开入虚端子配置信息  2013-6-5  ZY
  参数：ppRtTotalCfgAddr：供返回DI虚端子的总体配置信息全局变量地址对应的变量的指针。
                      该全局变量地址对应的变量由调用方分配，被调用方填充。
                      该全局变量自身，由被调用方分配和管理
  返回：EP_SUCCESS:操作成功
       其他，操作失败 */
EP_STATUS  HDL_Get_Vt_DI_Term_Cfg(HDL_TOTAL_VT_DI_TERM_CFG   **ppRtTotalCfgAddr);


/*功能：获得某虚端子配置信息，2013-7-6 ZY
  参数：pTermCfg，虚端子配置信息变量指针
                调用方分配，被调用方填充
       pHdl，该虚端子关联的通道句柄
       iSubNum，该虚端子关联的SUB号
       pSubMapData，该虚端子关联的SUBMAP信息指针
  返回：成功与否
   */
BOOL  HDL_Get_One_Vt_DI_Term_Cfg(HDL_VT_DI_TERM_CFG  *pTermCfg,
                                 void  *pHdl,
                                 int  iSubNum,
                                 void *pSubMapData);


/*功能：得到CPU虚拟开入虚端子状态信息  2013-6-5 ZY
  参数：pRtTotalSTS：供返回DI虚端子的总体状态信息变量指针。
                  该变量，由调用方分配，被调用方填充
  返回：EP_SUCCESS:操作成功
       其他，操作失败 */
EP_STATUS  HDL_Get_Vt_DI_Term_Sts(HDL_TOTAL_VT_DI_TERM_STS   *pRtTotalSts);



/* 根据通道句柄，获得相应的虚端子原始值,2013-6-6 ZY
   参数，pTermCfg,虚端子配置信息指针
       pucRtVal， 供返回的虚端子的原始值变量的指针。
                  该变量，由调用方分配，被调用方填充
   返回：TRUE，成功
         FALSE，失败 */
BOOL   HDL_GetGoVtTermOrgVal(HDL_VT_DI_TERM_CFG  *pTermCfg,uint8_t *pucRtOrgVal);

/* 根据通道句柄,获得相应的虚端子状态  2013-6-6  ZY
   参数， pTermCfg,虚端子配置信息指针
         pucRtSts，返回虚端子状态变量的指针。
                  该变量，由调用方分配，被调用方填充
         iSubGoIdxSeqNo,关联的SUB对应aiHdlSubGoIdx_g数组中的序号，从0开始
   返回：TRUE，成功
         FALSE，失败 */
BOOL   HDL_GetGoVtTermSts(HDL_VT_DI_TERM_CFG  *pTermCfg,uint8_t *pucRtSts,int  iSubGoIdxSeqNo);

/* 开出关联压板.
 * Para:
 *     pvDoCh, 开出驱动级句柄.
 * Return:
 *     压板序号(从0开始), or -1.
 */
extern int16_t HDL_CfgLinkofDo(void *pvDoCh);

#ifdef	__cplusplus
}
#endif

#endif                                  /* HDL_VTBOX_H */
