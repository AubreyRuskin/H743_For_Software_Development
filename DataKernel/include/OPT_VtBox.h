/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                  */
/*                                                                              */
/*      OPT_VtBox.h                                    1.0                       */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了光纵虚拟机箱模块的头文件                                   */
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



#ifndef OPT_VTBOX_H
#define OPT_VTBOX_H

#include  "OPT_SamInterface.h"
#include  "logic.h"
#include  "semLib.h"
#ifdef	__cplusplus
extern "C" {
#endif

#define  MAX_OPT_ALLOW_AIO_NUM   30    /*光纵AIO实际允许的最大数目  2006-11-8日张云改过，应EDP02需求扩大 */
#define  MAX_OPT_ALLOW_DIO_NUM   0    /*光纵DIO实际允许的最大数目,目前不允许配置  2006-6-14*/

#define   MAX_OPT_DI_MOD_NUM    3      /*光纵DI最大的摸件个数 */
#define   OPT_DI_MOD_BASE_ADDR  0      /*光纵DI摸件的允许的基址  */

#define   MAX_OPT_DO_MOD_NUM    3      /*光纵DO最大的摸件个数 */
#define   OPT_DO_MOD_BASE_ADDR  3      /*光纵DO摸件的允许的基址  */

#define   OPT_64K_SEND_POINT    2      /* 光纵1M通道每次发送点数2011-12-21日 ZY */
#define   OPT_2M_SEND_POINT     2      /* 光纵2M通道每次发送点数 */
#define   MAX_OPT_SND_BUF_LEN     400    /*光纵通道发送缓冲最大允许大小  */
#define   OPT_SND_BUF_HEAD_RSV_LEN  32   /*光纵通道发送缓冲头部保留大小  */

#define   HDLC_FRAM_HEAD      1          /*HDLC帧头部长度  */
#define   HDLC_FRAM_END       2          /*HDLC帧尾部长度  */

#define   OPT_64K_DATA_HEAD    13     /*光纵1M通道数据头部长度 2011-11-28  ZY */
#define   MAX_OPT_64K_DATA_LEN     150    /*光纵1M通道最大传输内容长度 2011-11-28  ZY */

#define   OPT_2M_DATA_HEAD    13      /*光纵2M通道数据头部长度  2006-11-12日 张云修改，添加了一字节 2006-11-26日 张云修改，添加5字节，*/
#define   MAX_OPT_2M_DATA_LEN     300     /*光纵2M通道最大传输内容长度  2006-11-8日张云改过 ，应EDP02需求扩大*/


/* 光纵机箱AI物理通道比例系数配置 2006-11-27日 张云修改*/
typedef struct
{
    /*2006-11-26日 张云修改  */
    BOOL     bPhyCoffIsValid;                /*该通道的物理比例系数是否是有效的，只有当对方串过来时，才有效，若通信长时间中断，则无效  */
    float    fPhyCoff;                      /*该通道的物理比例系数，是对方传过来的，注意不是AD最终相乘得到的系数  */
    float    fCoff;                         /*该物理通道的实际系数，是AD最终相乘得到的系数*/
    uint8_t  ucLgcAI;                       /* 光纵的虚拟机箱的物理AI对应的配置逻辑AI通道号，从0开始 */
    FLT_U32_UNION uPhyCoff;				/* 本次接收值 */
    FLT_U32_UNION uPhyCoffBak;				/* 保存上一次接收值 */
} OPT_HWAI_COFF_CFG;

/*光纵机箱物理AI来源的按物理通道序的AO信息结构，供发送时使用  2006-11-27日  张云   */
typedef  struct
{
    int  iAISrcAoNum;/*AI来源的AO通道个数  */
    OPT_AO_CFG  ** ppAISrcAoCfgArr;/* AI来源的AO通道配置信息数组，按AO物理通道次序排序，便于发送访问 */
}  OPT_ALL_AI_SRC_AO_CFG;/* 所有AI来源的AO配置结构定义 */



extern  OPT_HWAI_COFF_CFG  aBoxAiCoffTable[2][MAX_OPT_AI_NUM];/*按物理通道序  */
extern  OPT_BOX_AO_CFG   BoxAoCfgOpt_g[2];  /*光纵AO配置  */

extern  int   iOptTxPts_g;                        /*光纵每次发送数据点数  */
extern  int   iOptSysFreq_g;                      /* 系统频率 */

extern  int   iOptSamRate_g;                      /*正常系统采样率   */
extern  int   iNormalSamPeriod_g;                   /*正常系统采样周期  ns */
extern int iNormalSamPeriod_us_g; /* 正常系统采样周期, 以us为单位 */
extern  int   iNormalSamCntPerWave_g;
extern  int   iFrCntPerWave_g;                  /*通信时，每周波帧数目  */
extern  int   iSynAverageFrCnt_g;                /*光纵同步求平均值帧数  */
extern  float  fSynAverageCoff_g;                 /*光纵同步求平均值系数  */

extern  float   fSpeedSamRate_g;/*加快采样时的采样频率  */
extern  int     iSpeedSamPeriod_g;/*加快采样时的采样周期，以NS为单位  */
extern int iSpeedSamPeriod_us_g; /* 加快采样时采样周期, 以us为单位 */
extern  float   fSlowSamRate_g;  /*降低采样时的采样频率  */
extern  int     iSlowSamPeriod_g;  /*降低采样时的采样周期 ，以NS为单位  */
extern int iSlowSamPeriod_us_g; /* 降低采样时的采样周期, 以us为单位 */

extern  BOOL  abOptChIsInit_g[2];  /*通道设置标志  */
extern  BOOL  abOptChIsInitOver_g[2];  /*通道初始化完成标志  */

extern  int   iOptChType_g;                  /*通道类型,0为64K，1为2M　*/

extern  int   iOptAioDataByteLen_g;/*AIO实际数据长度　*/
extern  int   iOptDioDataByteLen_g;	              /* DIO实际数据长度  */
extern  int   iOptDataByteLen_g;                 /* 所有实际数据长度  */
extern  uint32_t  ulOptFrBaudSendTime_g;           /*帧的硬件BAUD发送时间，单位US 2006-12-21日 */

extern  int  iOptAiCh_g;                         /*光纵AI个数  */

extern  int  iOptDiCh_g;                         /*光纵DI个数  */
extern  uint8_t * apucOptDiStsBase_g[2];             /*光纵接收DI数据基址  */

extern  int  iOptAoCh_g;                         /*光纵AO个数  */
extern  int  iOptAISrcAoCh_g;                    /*AI来源的AO数目  */
extern  uint8_t  * pucOptAoDataByteBase_g;       /*光纵AI来源的AO发送数据基址  */
extern  EP_ELEM_IO *  apMidSrcAOPt_g[2][MAX_OPT_AO_NUM];  /*光纵AO的中间结果来源的指针数组，按AO硬件地址排列，为空，表示没有  */

extern  int  iOptDoCh_g;                          /*光纵DO个数  */
extern  uint8_t * apucOptDoStsBase_g[2];              /*光纵发送DO数据基址 */
extern  SEM_ID  semOptSndPtsDat_g;


extern  uint8_t  aucZerofloat_g[4];/*2006-6-14,浮点0的字符串  */

extern  OPT_ALL_AI_SRC_AO_CFG  OptAllAiSrcAoCfg_g;   /*所有AI来源的AO配置变量 2006-11-27日张云 */

extern  BOOL   abOptChAllAiPhyCoffValidFlagArr[2];/*光纵通道所有AI来源的AI比例系数有效标志，为TRUE，表示通道的所有AI比例系数都有效，否则无效 2006-11-27日张云 */

/* 初始化（并启动）光纵虚拟机箱
 * 参数：   iOptChNum ,光纵通道号，0为光纵通道1，1为光纵通道2，其他无效
 *          iOptChType,光纵通道类型，0为64K，1为2M，其他无效
            uiSmplRate，实际采样每周波采样点数
 *          uiSysFreq，系统频率
 *          uiTxPts, 光纵每次发送点数
 *          pvAiMod，该模块（光纵虚拟机箱负责的所有AI采集/计算通道）的句柄
 *          uiLgcCh，光纵AI采样的逻辑通道数
 *          plgccfg，指向光纵AI逻辑通道配置数组第0个元素的指针，数组元素有unLgcCh个
 *          uiCalcCfg，光纵AI预处理通道配置数，光纵的应该无预处理通道   目前应该为0
 *          pcalccfg，指向光纵AI预处理通道配置数组第0个元素的指针，数组元素uiCalcCfg个
 *          iOptAoCh,  光纵AO的数目
 *          pOptAoCfg，指向光纵AO逻辑通道配置数组第0个元素的指针，数组元素有iOptAoCh个
 *          iOptDiCh，光纵DI的数目
 *          puiOptDiStsBase，指向光纵DI通道的数据发送缓冲基址，每个通道占1位，
 *          iOptDoCh，光纵DO的数目
 *          puiOptDoStsBase，指向光纵DO通道的数据接收缓冲基址，每个通道占1位，
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS   Init_Opt_Box(int  iOptChNum,int  iOptChType,
                         u_int uiSmplRate, u_int uiSysFreq,u_int uiTxPts,
                         void *pvAiMod,
                         u_int uiLgcCh, DSP_LGC_AI_CFG *plgccfg,
                         u_int uiCalcCfg, DSP_CALC_AI_CFG *pcalccfg,
                         int  iOptAoCh,OPT_AO_CFG  *pOptAoCfg,
                         int  iOptDiCh,uint8_t * pucOptDiStsBase,
                         int  iOptDoCh,uint8_t  *pucOptDoStsBase);


/* 初始化光纵公共数据
 * 参数：
 *          iOptChType,光纵通道类型，0为64K，1为2M，其他无效
            uiSmplRate，实际采样每周波采样点数
 *          uiSysFreq，系统频率
 *          uiTxPts, 光纵每次发送点数
 *          iOptAoCh,  光纵AO的数目
 *          pOptAoCfg，指向光纵AO逻辑通道配置数组第0个元素的指针，数组元素有iOptAoCh个
 *          iOptDoCh，光纵DO的数目
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS   OPT_InitCommonData(int  iOptChType,
                               u_int uiSmplRate, u_int uiSysFreq,u_int uiTxPts,
                               int  iOptAoCh,OPT_AO_CFG  *pOptAoCfg,
                               int  iOptDoCh);


/* 初始化光纵通道配置数据
 * 参数：   iOptChNum ,光纵通道号，0为光纵通道1，1为光纵通道2，其他无效
 *          pvAiMod，该模块（光纵虚拟机箱负责的所有AI采集/计算通道）的句柄
 *          uiLgcCh，光纵AI采样的逻辑通道数
 *          plgccfg，指向光纵AI逻辑通道配置数组第0个元素的指针，数组元素有unLgcCh个
 *          uiCalcCfg，光纵AI预处理通道配置数，光纵的应该无预处理通道   目前应该为0
 *          pcalccfg，指向光纵AI预处理通道配置数组第0个元素的指针，数组元素uiCalcCfg个
 *          iOptDiCh，光纵DI的数目
 *          puiOptDiStsBase，指向光纵DI通道的数据缓冲接收基址，每个通道占1位，
 *          puiOptDoStsBase，指向光纵Do通道的数据发送缓冲基址，每个通道占1位，
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS   OPT_InitChCfgData(int  iOptChNum,
                              void *pvAiMod,
                              u_int uiLgcCh, DSP_LGC_AI_CFG *plgccfg,
                              u_int uiCalcCfg, DSP_CALC_AI_CFG *pcalccfg,
                              int  iOptDiCh,uint8_t * pucOptDiStsBase,uint8_t * pucOptDoStsBase);



/* 初始化光纵通道通信同步数据
 * 参数：   iOptChNum ,光纵通道号，0为光纵通道1，1为光纵通道2，其他无效
 *          pvAiMod，该模块（光纵虚拟机箱负责的所有AI采集/计算通道）的句柄
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS   OPT_InitChComSynData(int  iOptChNum,
                                 void *pvAiMod);



/*  初始化光纵AO所有配置
     参数：iOptChNum ,光纵通道号，0为光纵通道1，1为光纵通道2，其他无效
          iAOCfgNum:所有AO配置个数
     返回：成功与否*/
EP_STATUS   OPT_InitAOCfg(int  iOptChNum,int  iAOCfgNum);


/*通知光纵AO初始化完成
  参数：无
  返回：成功与否
*/
EP_STATUS   OPT_AOCfgInitFinish();


/*光纵通道重启
  参数：光纵通道号
  返回：成功与否
*/
BOOL   ResetOptCh(int  iOptChNum);


/*获得控制字设置定值
  参数：pSettingInfo，定值信息指针
         pRtIsMaster，返回控制字状态
  返回，操作成功与否  */
EP_STATUS  OPT_GetCtrlWordSettingValue(SCI_SETTING_INFO_TYPE   *pSettingInfo,BOOL  *pRtIsMaster);


/*  获得所有光纵AI比例系数是否有效标志  2006-12-2日 张云添加
    参数  iOptCh,通道号
    返回，TRUE，表示所有比例系数都有效，可以使用
          FALSE， 表示不是所有比例系数都有效，不可以使用
 */
BOOL  OPT_GetAllPhyCoffValidFlag(int  iOptCh);


/*  设置所有光纵AI比例系数无效标志  2006-12-2日 张云添加
    参数  iOptCh,通道号
    返回，无
 */
void  OPT_SetAllPhyCoffInValid(int  iOptCh);


/***********************************************************************
* OPTCh2_AOCfgInitFinish - 光纵机箱通道2完成初始化完成
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 错误
*
*/
EP_STATUS OPTCh2_AOCfgInitFinish(void);

#ifdef	__cplusplus
}
#endif

#endif                                  /* DSPAI_H */
