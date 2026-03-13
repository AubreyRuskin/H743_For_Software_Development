/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                  */
/*                                                                              */
/*      OPT_SamSyn.h                                    1.0                       */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了光纵采样同步模块的头文件                                   */
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


#ifndef OPT_SAMSYN_H
#define OPT_SAMSYN_H

#include  "swcfg.h"
#include  "OPT_MmiInterface.h"
#include  "OPT_SamInterface.h"
#include  "realdatadef.h"
#include  "semLib.h"
#include "datetime.h"
#include "bspinterface.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define   OPT_INSTABLE_THRSH_FOR_FR_LOST    6   /* 光纵失稳的平滑周期内帧丢失数目门槛 2007-1-6日 张云 */
#define   OPT_INSTABLE_THRSH_FOR_FR_DELAY   6    /*光纵失稳的平滑周期内帧长时间延迟的帧数目的门槛  2009-2-18日 张云 */
#define   OPT_FR_STS_BUF_NUM          64      /*帧状态数组大小 必须是2的指数幂 */

#define  OPT_SAM_ADJ_TIME_PER_PERIOD     15 /*光纵采样频率调整每周波调整时间,单位US  */
#define  OPT_TSSE_AVERAGE_THRESH    40000    /*光纵计算TSSE的求平均值时间门槛,单位US,2个周波  */
#define  OPT_INST_TSSE_TO_HALF_TS_DIFF_THRESH   100  /*光纵通道TSSE与半个采样周期的差值门槛，单位US，*/

#define  OPT_INST_DELAY_STABLE_FLUCT_THRESH   150  /*光纵通道通信延时瞬时波动门槛，单位US，2009-2-9日张云，现在有采样点同步功能后，门槛需要比较小，*/
#define  OPT_INST_DELAY_STABLE_ABNORMAL_THRESH   30000   /*光纵通道通信延时波动失真门槛，单位US，2009-2-18日张云，门槛需要比较大，表示此延时失真
                                                            2009-9-24日ZY 因华东测试,当帧突变过大,会状态保持,现在由10000放大到30000*/

#define  OPT_DELAY_STABLE_FLUCT_THRESH    100   /*光纵通道平均通信延时稳定的波动门槛,单位US  ，若超过该门槛，则认为暂态失稳 2006-11-14日张云*/
#define  OPT_DELAY_INSTABLE_PERSIST_THRESH  50000   /*光纵通道平均通信延时失稳持续时间的判定门槛,50毫秒，需要比平滑周期40MS长，单位US   2009-2-20日张云 */
#define  OPT_DELAY_STABLE_PERSIST_THRESH    40000    /*光纵通道平均通信延时稳定持续时间的判定门槛,40毫秒(加上平滑周期的40毫秒，实际相当于80毫秒)，单位US                                                        
                                                        注意不要太大，否则对判定路由不一致精度有影响  2009-2-17日  张云*/

#define  OPT_SAM_EXACT_SYN_THRESH   20   /*光纵采样精确同步门槛,单位US，用于调整采样  */
#define  OPT_SAM_DIFF_SYN_THRESH   100   /*光纵采样差动保护同步门槛,单位US  */
#define  OPT_SAM_RELAY_SYN_THRESH   300   /*光纵采样失步门槛,单位US  2006-7-29日张云修改过
                                                2009-9-24日  考虑到T接线和复用路由的延时波动情形,由150US放大到300US */


#define  OPT_RCV_SND_DIF_CHG_THRESH   700   /*光纵通道收发时间差变化的判定门槛,单位US  
                                               防止误判，需要采样调整的最大误差的2倍大，并留足够裕度，2009-2-13日张云*/

#define  OPT_RCV_SND_DIF_COM_STABLE_INTVL_THRESH   250000   /*允许进行收发时间差变化判定的两次光纵通道稳定状态的间隔时间门槛,单位US  
                                                                 需要>>(通信稳定状态判定门槛+40+光纤通道路由切换时间)，并留足够裕度
                                                                 防止误判，2009-2-17日张云*/


#define   OPT_TIME_BASE_PERIOD   (1000000000/OPT_TIME_BASE_FREQ)     /*TIMEBASE的周期，以ns为单位  */
#define   OPT_TIME_BASE_US       (1000000/OPT_TIME_BASE_FREQ)        /*1US对应的BASE数,有舍入误差,对精度要求不高场合适用  */

#define   CPU_POINT_SYN_ALLOW_TIME    1000000        /*采样点同步允许维持的时间，单位微秒  2009-1-5日，
                                                       取CPU复位和晶振允许偏移的最小时间*/

#define CPU_RCV_INST_HALT 30  /* 瞬时接收中断判断间隔时间, ms */

#define   OPT_DEAL_FR_BUF_NUM    6   /*保存的待处理的帧的缓冲个数  2007-1-7日 因为BSP接收缓冲才8，必须小于*/

#define   OPT_UNKNOWN_COM_TIME   190  /* 未计算的通道通信时间，与实现方式有关，包括采样中断时间，采样任务时间，
                                       发送任务时间（不包括硬件BAUD发送时间），接收任务时间等
                                        随着实现方式，需要变更 需要通过自环来进行测试，2006-12-21*/


#define DEFAULT_TIME_WHEN_INST_HALT 1000 /* 短时中断后缺省上边时间 */
#define MIN_SIM_TICK 3 /* 最小模拟节拍 */
#define MAX_SIM_TICK 30  /* 最大模拟节拍 */

/*光纵最新接收数据，供发送时使用，2007-11-28日 张云   */
typedef struct
{
    /*最近接收数据 */
    OPT_TIME_BASE   tLastRecvTime;/*光纵最近接收时的TIMEBASE时间 */
    uint8_t         ucLastRecvPeerSamCnt;/*光纵最近接收到的对端采样计数,最后一点 */
    uint32_t ulLstRcvMsCnt; /* 光纵最近接收时的ms数 */
}   OPT_CH_LST_RCV_INFO;


/*光纵接收同步数据  */
typedef struct
{
    /*本帧接收数据 */
    OPT_TIME_BASE   tLastRecvTime;/*光纵最近接收时的TIMEBASE时间 */
    uint8_t         ucLastRecvControl;/*光纵最近接收到的control code */
    uint8_t         ucLastRecvPeerSamCnt;/*光纵最近接收到的对端采样计数,最后一点 */
    uint8_t         ucPeerFastTaskClkDiff;/*光纵最近接收到的对端快速任务的采样点差 2006-11-12日 张云  */
    uint8_t         ucLastRecvPeerRecvSamCnt;/* 光纵最近接收到的对端的最近接收到的采样计数， */
    uint16_t        uiLastRecvPeerRecvtoSendIntvl;/* 光纵最近接收到的对端从接收到发送采样的时间间隔，US为单位 */
    OPT_TIME_BASE   tLastSendTime;/*光纵最近发送时的TIMEBASE时间 */

    /*本帧同步计算的数据  */
    uint32_t        ulLastChInstTime;         /* 通道本次通信时间，US */
    int             iLastRecvtoCloseCntIntvl;  /*本次接收和最接近采样的时间间隔，US为单位  */
    int             iLastSamSynDiffInstTime;  /*通道采样同步本次误差，US，>0,表示自己领先，需要慢采样，<0,表示对方领先，需要快采样，和文档是反的  */
    uint8_t         ucLastRecvCloseSamCnt;       /* 最近接收到的帧最接近的采样计数 */
    uint8_t         ucLastRecvSynLocalSamCnt;    /*光纵最近接收到的同步到本机的采样计数  */
    uint8_t         ucLastSynSamCntIntvl;        /*本次同步到本机的采样计数，和上次同步到本机的采样计数的间隔  */
    uint32_t        ulLastRecvSynLocalAiCnt;     /*光纵最近接收到的同步到本机的Ai计数  */
    uint32_t        ulLastClosetoSynSamCntDiff;     /*光纵最近接收到的最接近和同步的采样计数之差，  */

    int8_t ucLocalDelay;  /* 本地采样通道延迟点数 */
    uint8_t ucDelay;   /* 对侧采样通道延迟点数 */
    int8_t ucRltDif;   /* 本对侧采样通道相对延迟点数 */
    int8_t ucAllRltDif;   /* 本对侧总相对延迟点数（采样通道和通信） */
    uint8_t ucDelaySynLocalSamCnt;  /* 光纵最近接收到的同步到本机的采样计数(考虑采样通道延迟) */

    uint8_t ucTemp;  /* 整点延迟 */
    uint8_t ucSimTick; /* 模拟节拍 */
}   OPT_COM_SYN_DATA;

/*光纵发送公共数据  */
typedef struct
{
    uint8_t    ucLocalSamCnt;    /*最近的采样计数*/
}   OPT_SEND_COMMON_CMD;


/*光纵通道发送数据  */
typedef struct
{
    uint8_t    ucSendControl;    /*光纵最近接收到的control code */
}   OPT_CH_SEND_CMD;


/* 采样调整模式 */
typedef  enum
{
    OPT_SAM_SPEED=0,   /* 加快采样 */
    OPT_SAM_SLOW=1,    /* 减慢采样 */
    OPT_SAM_NORMAL=2,  /* 正常采样 */
}  OPT_ADJ_SAM_MODE;


/*光纵通道通信状态  2009-1-19日 ZY*/
typedef  enum
{
    OPT_CH_COM_STABLE=0,       /*光纵通道通信稳定，只有持续暂态稳定，才稳定  */
    OPT_CH_TEMP_STABLE=1,      /*光纵通道通信暂态稳定  */
    OPT_CH_TEMP_INSTABLE=2,      /*光纵通道通信暂态不稳定  */
    OPT_CH_COM_INSTABLE=10,     /*光纵通道通信失稳，只有持续暂态失稳，才失稳  */
}  OPT_CH_COM_STS;



/*光纵通道采样同步状态  2009-1-19日 ZY*/
typedef  enum
{
    OPT_CH_SAM_RELAY_SYN=0,  /*光纵通道采样保护计算同步 */
    OPT_CH_SAM_POINT_SYN=1,  /*光纵通道采样点同步 */
    OPT_CH_SAM_MISS_SYN=10,   /*光纵通道采样失步 */
}  OPT_CH_SAM_SYN_STS;


/*光纵通道通信状态数据  */
typedef  struct
{
    OPT_CH_COM_STS          iComSts;    /* 通道通信状态 */
    OPT_CH_SAM_SYN_STS      iSamSynSts;     /* 通道采样同步状态 */
    int                     iRcvSndDiffChgTime;  /* 前后两次通信稳定状态变化期间，通道的收发时间差变化值，单位US
                                                     Value=本次稳定时真实收发时间差-上次稳定时真实收发时间差
                                                     若为0，表示没有发生变化，或无法判定
                                                     若非0，表示此次判定出来的收发时间差的变化值  2009-2-13 ZY*/
    uint32_t                ulChAvrgTime;   /* 通道通信平均时间，US */
    int                     iSamSynDiffAvrgTime;  /*通道采样同步平均误差，US，>0,表示自己领先，<0,表示对方领先  */
    OPT_TIME_BASE           TempInstableStartTime;  /*开始暂态失稳的时间，TimeBase  */
    OPT_TIME_BASE           TempStableStartTime;    /*开始暂态稳定的时间，Timebase  */

    int                     iLostFrmPerWave;            /*最近求平均的通信时间内丢帧数目，凡是乱序数据都处理为丢帧  */
    int                     iDelayFrmPerWave;           /*最近求平均的通信时间内帧延迟数目  */
    uint32_t                ulChTotalTime;              /*通道求平均的通信时间总和,为了防止计算误差  */
    int                     iSamSynDiffTotalTime;       /*通道采样同步误差求平均的总和  */
} OPT_CH_STS_DATA;


/* 帧瞬时状态 */
typedef struct
{
    int   iFrLost;                       /* 该帧是否丢失，为1, 表示帧丢失，为0，表示未丢失*/
    int   iFrDelay;                      /*该帧是否长时间延迟，为1，表示延迟，为0，表示未丢失 */

    uint32_t        ulCurChInstTime;         /* 通道本次通信时间，US，
                                              若帧丢失，则用平均值代替，若帧长时间延迟，也用平均值代替 */
    int             iCurSamSynDiffInstTime;  /* 通道采样同步本次误差，
                                              US，>0,表示自己领先，需要慢采样，<0,表示对方领先，需要快采样，和文档是反的
                                               ，则用平均值代替，若帧长时间延迟，也用平均值代替  */
    uint32_t        ulCurChAvrgTime;          /*通道此时的平均通信时间  */
} OPT_ONE_FR_STS;


/* 光纵机箱AI物理通道比例系数配置 */
typedef struct
{
    OPT_ONE_FR_STS	FrStsBuf[OPT_FR_STS_BUF_NUM];/*帧瞬时状态缓冲  */
    uint32_t  uiCurFrPos;     /*帧当前帧在BUF中的位置   */
} OPT_FR_STS;



/*光纵通道接收数据有效状态  */
typedef struct
{
    BOOL   bFirstRecvFlag_s;
    uint32_t  iInValidRecvCnt_s;
    BOOL   bDataIsValid_s;
    BOOL   bLastSynFlag_s;
    BOOL   bFirstSamSynFlag_s;   /*首次同步标志2006-11-15日　张云 */
} OPT_CH_RECV_DATA_VALID_STS;


/*光纵通道最近一次通信稳定相关信息 2009-2-9日 张云 判路由不一致时使用*/
typedef struct
{
    OPT_TIME_BASE  LastRcvPeerBaseTime;   /*最近一次通信稳定时的接收对侧采样点时的Timebase时刻  */
    uint32_t       ulLastComTime;         /*最近一次通信稳定时的通信时间，US为单位  */
    uint8_t        ucRecvPeerSamCnt;      /*最近一次通信稳定时的接收的对方的采样计数器  */

} OPT_CH_LAST_COM_STABLE_INFO;

/*光纵通道最近一次通信稳定且采样同步的相关信息 2009-1-4日 张云 判采样点同步时用*/
typedef struct
{
    OPT_TIME_BASE  LastRcvPeerBaseTime;   /*最近一次通信稳定且采样计算同步时的接收对侧采样点时的Timebase时刻，  */
    uint32_t       ulLastComTime;         /*最近一次通信稳定且采样计算同步时的通信时间，US为单位  */

    uint8_t   ucRecvPeerSamCnt;             /*最近一次通信稳定采样计算同步时的接收的对方的采样计数器  */
    uint8_t   ucSamSynLocalSamCnt;          /*最近一次通信稳定采样计算同步时的同步到本侧的采样计数器  */
    uint32_t  ulTsse;                       /*最近一次通信稳定采样计算同步时的采样同步误差绝对值，US，  */

} OPT_CH_LAST_COM_STABLE_SAM_SYN_INFO;




/*光纵通道推算采样点间隔的结构 2009-1-4日 张云 */
typedef struct
{
    OPT_TIME_BASE  OldPointBaseTime;   /* 前一个采样点的base时间  */
    uint8_t  ucOldPointCnt;              /* 前一个采样点的采样节拍 */

    OPT_TIME_BASE  NewPointBaseTime;   /*后一个采样点的base时间  */
    uint8_t  ucNewPointCnt;              /* 后一个采样点的采样节拍 */

    int      iCalcSamPointIntvl;              /*计算出来的后一个采样和前一个采样点的采样节拍差，
	                                         若<0,则表示计算值无效，否则计算值有效 */

} OPT_CALC_SAM_INTVL_INFO;


/*通道本次通信接收异常统计信息  */
typedef struct
{
    BOOL  bShortLostFrm;   /*是否个别丢帧  */
    BOOL  bLongLostFrm;     /*是否多次丢帧  */
    BOOL  bShortDelayFrm;  /*是否个别帧延迟  */
    BOOL  bLongDelayFrm;   /*是否多次帧延迟  */
    BOOL  bSendTimeChange; /*本次是否发生平均延时改变  */
    BOOL  bInstLostFrm;    /*本次是否丢帧  */
    BOOL  bInstDelayFrm;    /*本次是否帧延迟  */
    OPT_ONE_FR_STS   *pCurFrSts;  /*帧缓冲中当前帧的指针  */
    OPT_ONE_FR_STS   *pPastFrSts; /*帧缓冲中前推帧的指针  */
}   OPT_CH_RCV_ABNORMAL_STAT;




extern BOOL   bOptHdlcClkIsMaster_g[2];        /*若设置HDLC时钟同步是主，则为真(当是直通时)，否则为假（当是接复接设备） 2009-4-14日 ZY */


extern OPT_SEND_COMMON_CMD   OptSendCommonCmd;         /*光纵发送时的公共数据  */


extern   OPT_CH_STS_REPORT  aOptChStsRpt_g[2];  /*光纵通道状态报告数组，给MMI  */
extern   OPT_TIME_BASE   aOptChInitTimeBase_g[2];/* 光纵通道初始化TIMEBASE时间数组 */


extern     OPT_CH_STS_DATA       OptChStsData_g[2];         /*光纵通道的状态数据  */
extern     OPT_FR_STS            OptFrSts_g[2];             /*光纵帧接收状态  */
extern     OPT_COM_SYN_DATA     aOptComSynData_g[2];      /*光纵通信接收同步数据 */
extern     OPT_CH_RECV_DATA_VALID_STS   aOptChDataValidSts[2];     /*光纵通道接收数据有效状态  */
extern     OPT_TIME_BASE   aulOptChLastRecvValidDataTime_g[2];     /*光纵通道上次接收有效数据时间  2013-5-20日 张云 */
extern     OPT_CH_LAST_COM_STABLE_SAM_SYN_INFO   aLastComStableSamSynInfo_g[2];   /*光纵通道最近的通信稳定且采样同步时的相关信息。2006-11-15  */
extern     OPT_CH_LAST_COM_STABLE_INFO   aLastComStableInfo_g[2];   /*光纵通道最近的通信稳定时的相关信息。2009-2-9  ZY*/

extern     OPT_CH_SEND_CMD       OptChSendCmd[2];              /*光纵发送时的通道数据  */

extern     OPT_CH_LST_RCV_INFO      OptChLstRcvInfo_g[2];         /*光纵通道最近接收的信息，供发送时访问  */

// extern void vxTimeBaseGet (UINT32 * pTbu, UINT32 * pTbl);

/* 初始化光纵通道通信同步数据
 * 参数：   iOptChNum ,光纵通道号，0为光纵通道1，1为光纵通道2，其他无效
 *          pvAiMod，该模块（光纵虚拟机箱负责的所有AI采集/计算通道）的句柄
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS   OPT_InitChComSynData(int  iOptChNum,
                                 void *pvAiMod);


/*计算本帧的本次的即时采样同步误差
  参数：pComSynData，接收到的数据
        pChStsData，通道状态数据
        ucRcvSamCnt ,该帧接收时对应本机的采样计数 ，2007-11-28日 张云修改
        bPeerRcvIsStable, 对侧接收是否稳定.
  返回，成功与否    */
BOOL    OPT_CalcChInstSynDiffTime(OPT_COM_SYN_DATA  *pComSynData
                                  ,OPT_CH_STS_DATA  *pChStsData
                                  ,uint8_t   ucRcvSamCnt,
                                  BOOL bPeerRcvIsStable);

/*更新通道帧状态缓冲数据
  参数：pComSynData，接收到的数据
        pFrSts,   帧状态
        iLostFrNum,丢失的帧数目
        iOptChNum,通道号，0为1号通道，1为2号通道
        bInstDataIsValid,瞬时数据有效与否标志
  返回      */
void    OPT_RefreshFrSts(OPT_COM_SYN_DATA  *pComSynData
                         ,OPT_CH_STS_DATA  *pChStsData
                         ,OPT_FR_STS* pFrSts
                         ,int  iLostFrNum
                         ,int  iOptChNum
                         ,BOOL   bInstDataIsValid);

/*更新通道状态数据
  参数：pComSynData，接收到的数据
        pChStsData,通道状态
        pChStsRpt,状态报告
        pFrSts,   帧接收状态
        iLostFrNum,丢失的帧数目


  返回  是否需要写数据到缓冲，真，表示需要写缓冲，否则不需要写缓冲    */
BOOL    OPT_RefreshChSts(OPT_COM_SYN_DATA  *pComSynData
                         ,OPT_CH_STS_DATA  *pChStsData
                         ,OPT_CH_STS_REPORT  *pChStsRpt
                         ,OPT_FR_STS* pFrSts
                         ,int  iOptChNum);



/*光纵帧数据汇总处理  2009-1-19日 ZY
  参数： pChStsData,    通道状态
         pChStsData,    通道状态
         pChStsRpt，    通道状态报告
         pFrSts,        帧接收状态
         pRtRcvStat,    供返回本帧接收异常统计
         iOptCh,通道号
 */
void   OPT_GatherFrSts(OPT_COM_SYN_DATA  *pComSynData
                       ,OPT_CH_STS_DATA  *pChStsData
                       ,OPT_CH_STS_REPORT  *pChStsRpt
                       ,OPT_FR_STS* pFrSts
                       ,OPT_CH_RCV_ABNORMAL_STAT  *pRtRcvStat
                       ,int  iOptCh);



/*光纵通信状态机处理  2006-11-15日
  参数： pChStsData,    通道状态
         pChStsData,    通道状态
         pChStsRpt，    通道状态报告
         pRcvStat,      供本帧接收异常统计
         iOptCh,通道号
 */
void   OPT_ComStsDeal(OPT_COM_SYN_DATA  *pComSynData
                      ,OPT_CH_STS_DATA  *pChStsData
                      ,OPT_CH_STS_REPORT  *pChStsRpt
                      ,OPT_CH_RCV_ABNORMAL_STAT  *pRcvStat
                      ,int  iOptCh);



/*光纵采样同步状态判定  2009-1-19日  ZY
  参数： pComSynData，通道瞬时状态
         pChStsData,    通道状态
         pChStsRpt，   通道状态报告
         pFrSts,        帧接收状态
         pRcvStat,      供本帧接收异常统计
         iOptCh,通道号
 */
void   OPT_SamSynDeal(OPT_COM_SYN_DATA  *pComSynData
                      ,OPT_CH_STS_DATA  *pChStsData
                      ,OPT_CH_STS_REPORT  *pChStsRpt
                      ,OPT_FR_STS* pFrSts
                      ,OPT_CH_RCV_ABNORMAL_STAT  *pRcvStat
                      ,int  iOptCh);


/*光纵采样同步调整  2009-1-19日  ZY
  参数： pComSynData，通道瞬时状态
         pChStsData,    通道状态
         pChStsRpt，   通道状态报告
         iOptChNum,通道号

  返回  是否需要写数据到缓冲，真，表示需要写缓冲，否则不需要写缓冲    */
BOOL   OPT_SamSynAdjust(OPT_COM_SYN_DATA  *pComSynData
                        ,OPT_CH_STS_DATA  *pChStsData
                        ,OPT_CH_STS_REPORT  *pChStsRpt
                        ,int  iOptChNum);


/*光纵收发路由差的变化判定  2009-2-9日  ZY
  参数： pComSynData，通道瞬时状态
         pChStsData,    通道状态
         iOptCh,通道号
 */
void   OPT_RouteDifChgDeal(OPT_COM_SYN_DATA  *pComSynData
                           ,OPT_CH_STS_DATA  *pChStsData
                           ,int  iOptCh);


/* 某光差通道当前是否和对侧采样同步 2009-3-5日  ZY
 * 参数：   iOptCh ,光纵通道号，0为光纵通道1，1为光纵通道2，其他无效
 *
 * 返回值：TRUE，当前该通道和对侧采样同步
           FALSE，当前该通道和对侧采样失步
   注意：  供RD_AI_Dat_P函数中调用 */
BOOL  OPT_ChCurIsSamSyn(int  iOptCh);




/*Timebase相关函数  */

/*获得baseTimer大小  */
static __inline__  OPT_TIME_BASE    OptGetBaseTimerCnt(void)
{
    OPT_TIME_BASE  CurTimeBase;
    vxTimeBaseGet((UINT32 *)&CurTimeBase.ulTimeBaseH, (UINT32 *)&CurTimeBase.ulTimeBaseL);
    return  CurTimeBase ;
}

/*daibixiang modify for 母差*/
/*TIMEBASE的相减,获得US时间差(准确的) 可正可负
  A是被减数，B是减数 ,2013-5-20 ZY, */
extern int32_t    OptGetUsIntvlByBase(OPT_TIME_BASE  *pBaseA,OPT_TIME_BASE  *pBaseB);

/*根据TIMEBASE的差,获得US时间差  */
extern int32_t   OptGetUsIntvlByBaseDiff(int64_t   llBaseDiff);

/*TIMEBASE的相减,获得TIMEBASE时间差  ,*/
extern int64_t   OptGetBaseDiff(OPT_TIME_BASE  *pBaseA,OPT_TIME_BASE  *pBaseB);


/*获得baseTimer的低32位大小  */
static __inline__  uint32_t   OptGetBaseTimerLowCnt(void)
{
    uint32_t  ulTimeBaseH,ulTimeBaseL;
    vxTimeBaseGet((UINT32 *)&ulTimeBaseH, (UINT32 *)&ulTimeBaseL);
    return  ulTimeBaseL;
}

/*TIMEBASE低32位的相减,获得US时间差(短时准确（2分多钟），长时间不准确) ，结果始终为正数
  A是被减数，B是减数 ,2013-5-20 ZY, */
static __inline__  uint32_t    OptGetUsIntvlByBaseTimerLow(uint32_t  ulBaseLowA,uint32_t  ulBaseLowB)
{
    return (ulBaseLowA-ulBaseLowB)/(OPT_TIME_BASE_FREQ/1000000); /* 一个计数为40ns, 转换为us时除以25 */
}


/*将TIMEBASE合并为64位整数*/
static __inline__  uint64_t   OptBaseUnion(uint32_t  ulBaseH,uint32_t  ulBaseL)
{
    return  (((uint64_t)(ulBaseH))<<32)+(uint64_t)(ulBaseL);
}


/*根据TIMEBASE的差,获得US时间差  */
int32_t   OptGetUsIntvlByBaseDiff(int64_t   llBaseDiff);

/*TIMEBASE的相减,获得TIMEBASE时间差  ,*/
int64_t   OptGetBaseDiff(OPT_TIME_BASE  *pBaseA,OPT_TIME_BASE  *pBaseB);


/*在TIMEBASE上加减DELTA的US时间，然后得到新的BASE时间  */
void    OptTimebaseAddUsTime(OPT_TIME_BASE  *pBaseSrc,OPT_TIME_BASE  *pBaseDst,int32_t   iUsDelta);

/*TIMEBASE的相减,获得秒时间差  ,只能是正*/
uint32_t    OptGetSecIntvlByBase(OPT_TIME_BASE  *pBaseA,OPT_TIME_BASE  *pBaseB);

/* 功能：根据pCalcInfo相关信息，计算采样节拍差
     返回信息在pCalcInfo->iCalcSamPointIntvl
   参数：pCalcInfo
   返回：无*/
void    OptCalcSamIntvl(OPT_CALC_SAM_INTVL_INFO  *pCalcInfo);

/* 清除报告信息.
 * Para:
 *     iOptChNum, 通道号, 取值为0或1.
 * Return:
 *     TRUE or FALSE.
 */
extern BOOL OPT_ClrRpt(int32_t iOptChNum);

#ifdef	__cplusplus
}
#endif

#endif                                  /* DSPAI_H */
