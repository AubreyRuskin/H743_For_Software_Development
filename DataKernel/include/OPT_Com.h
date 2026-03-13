/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                  */
/*                                                                              */
/*      OPT_Com.h                                    1.0                       */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了光纵通信收发模块的头文件                                   */
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
/*         张云       2006.12.3                创建文件1.0版本              */
/*                                                                            */
/*                                                                              */
/********************************************************************************/


#ifndef OPT_COM_H
#define OPT_COM_H

#include  "swcfg.h"
#include  "OPT_MmiInterface.h"
#include  "OPT_SamInterface.h"
#include  "realdatadef.h"
#include  "semLib.h"
#include  "OPT_VtBox.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define DELAY_MASK 0xE0   /* 延迟点数掩码 */
#define DELAY_POS 5  /* 延迟点数在通信控制字节中的位置 */

typedef   enum
{
    OPT_CH_MASTER_MSK=0X01,/*光纵主从控制BIT，1为主，0为从  */
    OPT_CH_RECV_STABLE=0X02,/*光纵该通道接收稳定标志，1为稳定，0为不稳定  */
    OPT_CH_RELAY_YABAN=0X04,/*光纵该通道差动压板投退标志，1为投入，0为退出  */
    OPT_OTHER_CH_VALID=0X08,/*光纵另外一个通道有效标志，1为有效，0为无效  */
    OPT_CH_DATA_UNCREDIBLE=0X10,/*光纵通道对侧发送过来数据不可信标志，
                                    1为不可信，0为可信，2009-2-15 ZY  */
    /*其他位暂且保留  */
}  OPT_COM_CTRLWORD_MSK;/*光纵通信控制字位分配  */


/*光纵通道每秒丢帧统计相关信息 2006-11-16日 张云 */
typedef struct
{
    OPT_TIME_BASE  ulLastStatLostFrmUsCnt;       /*最近一次丢帧统计的时刻BASE，   2013-5-20 ZY*/
    uint32_t  ulLastStatLostFrmNum;         /*最近一次丢帧统计个数，每次统计之后，需要重新清零  */
    BOOL      bNewComIsLongHaltInStat; /* 是否长时间通信中断标志(超过1s无数据接收) */
    BOOL bRcvInstHalt; /* 瞬时接收中断 */

} OPT_CH_LOST_FRM_STAT_INFO;


typedef  struct
{
    uint32_t  ulNeedDealCnt;  /* 待处理的帧个数 */
    uint32_t  ulNewRecvPos;   /* 新接收的帧位置 */
    uint32_t  ulCurDealPos;   /* 待处理的帧位置 */
    uint8_t  * apucFrmBuf[OPT_DEAL_FR_BUF_NUM];  /*帧指针缓冲数组  */

    /* 2007-11-28日 张云新添加 */

    UINT32 ulRcvBaseHBuf[OPT_DEAL_FR_BUF_NUM];    /*接收该帧时的64位TimeBase高4字节 缓冲数组 */
    UINT32  ulRcvBaseLBuf[OPT_DEAL_FR_BUF_NUM];   /*接收该帧时的64位TimeBase低4字节缓冲数组  */
    uint8_t  ucRcvLocalSamCntBuf[OPT_DEAL_FR_BUF_NUM]; /*接收该帧时的本机SamCnt 缓冲数组 */
    int      iRcvByteLenBuf[OPT_DEAL_FR_BUF_NUM];     /*接收该帧的帧长缓冲数组 */

}  OPT_CH_NEED_DEAL_FRM_INFO;/*通道需要逻辑图待处理的帧信息  2007-11-28日 张云添加*/





extern   OPT_CH_LOST_FRM_STAT_INFO   aLostFrmStatInfo_g[2]; /*光纵通道丢帧统计数据信息  */
extern   OPT_CH_NEED_DEAL_FRM_INFO   aOptChNeedDealFrmInfo[2];/*通道需要处理的帧信息  2006-11-27日 张云  */
extern     uint8_t  aucSndDataBuf_g[2][MAX_OPT_SND_BUF_LEN];    /*光纵发送通道数据缓冲  */

extern   uint32_t   aulLastQueryTotalFrLostNum_s[2];
extern   uint32_t   aulLastQueryTotalFrDelayNum_s[2];
extern   uint32_t   aulLastQueryHdlcCrcErr_s[2];


/*光纵接收函数  2007-10-30日 张云修改
   参数  ucChNum，HDLC通道编号，CHAN_UP or CHAN_DOWN
         pucRcvBuf， 接收缓冲基址，实际是在BD中
         iRcvByteLen，接收数据长度
         ulRcvBaseH， 接收数据时的64位TIMEBASE的高32位
         ulRcvBaseL，接收数据时的64位TIMEBASE的低32位
   返回，OK 或 ERROR
         目前这里要求都返回OK*/
int  OptRecvCmd(UINT8 ucChNum,
                uint8_t *pucRcvBuf,
                int  iRcvByteLen,
                UINT32 ulRcvBaseH,
                UINT32  ulRcvBaseL);

/*光纵单帧内容处理函数  2007-11-28日 张云添加
   参数  iOptCh，  光差通道号，0或者1，代表通道1或者通道2
         pucRcvBuf， 接收缓冲基址，实际是在BD中
         iRcvByteLen，接收数据长度
         ulRcvBaseH， 接收数据时的64位TIMEBASE的高32位
         ulRcvBaseL，接收数据时的64位TIMEBASE的低32位
         ulRcvLocalSamCnt,接收数据时的本机此时的采样计数
   返回，OK 或 ERROR
          */
int  Opt_DealOneFrm(int  iOptCh,
                    uint8_t *pucRcvBuf,
                    int  iRcvByteLen,
                    UINT32 ulRcvBaseH,
                    UINT32  ulRcvBaseL,
                    uint8_t  ulRcvLocalSamCnt);



/*处理光纵单帧的帧头内容处理函数  2009-1-20日 张云添加
   参数  iOptCh，  光差通道号，0或者1，代表通道1或者通道2
         pucRcvBuf， 接收缓冲基址，实际是在BD中
         ulRcvLocalSamCnt,接收数据时的本机此时的采样计数
         pChSynData，接收到的数据
         pChStsData,通道状态
         pChStsRpt,状态报告
         pChDataValidSts,数据有效状态
         pucRtLostFrmNum，返回的丢帧记数
         LastRecvTimeBase,上一帧接收的时刻
   返回，OK 或 ERROR
          */
int  Opt_DealOneFrmHead(int  iOptCh,
                        uint8_t *pucRcvBuf,
                        uint8_t  ulRcvLocalSamCnt,
                        OPT_COM_SYN_DATA  *pChSynData,
                        OPT_CH_STS_DATA  *pChStsData,
                        OPT_CH_STS_REPORT  *pChStsRpt,
                        OPT_CH_RECV_DATA_VALID_STS  *pChDataValidSts,
                        uint8_t  * pucRtLostFrmNum,
                        OPT_TIME_BASE  LastRecvTimeBase);


/*处理光纵单帧的帧异常处理函数  2009-1-20日 张云添加
   参数  iOptCh，  光差通道号，0或者1，代表通道1或者通道2
         iRcvResult,接收帧错误
   返回，OK 或 ERROR
          */
int  Opt_DealOneFrmErr(int  iOptCh,int  iRcvResult);


/* 保存光纵通道新接收到的帧的数据到缓冲，  2007-11-28日 张云
   参数   iOptCh,0或1，通道号
          pucRecv，接收的帧相关数据地址
          ucCurSynSamCnt 同步的采样点号
          iSamSynDiffTime  同步点的同步差，US
          ucLastRecvPeerSamCnt 接收的对方采样点号
   返回   EP_SUCCESS,操作成功
          其他，操作失败*/
EP_STATUS   OPT_ChStoreNewRecvFrm(int  iOptCh
                                  ,uint8_t  * pucRecv
                                  ,uint8_t  ucCurSynSamCnt
                                  ,int      iSamSynDiffTime
                                  ,uint8_t  ucLastRecvPeerSamCnt);


/*光纵发送任务函数，  */
EP_STATUS OptSendCmd();



/*光纵发送帧头处理函数，2008-1-20日修改 ,ZY
  参数，iOptInitChNum,配置的通道个数，1
  返回， */
EP_STATUS  OptSendCmdFrmHead(int  iOptInitChNum);


/*光纵发送帧异常函数，2008-1-20日修改 ,ZY
  参数，iOptCh,光纵通道号
        iSendResult,发送异常代码
  返回， */
EP_STATUS  OptSendCmdFrmErr(int  iOptCh,int iSendResult);

/*计算数据的插零时间
  参数: pucData,数据基址
        iDataLen,数据长度
        iOptType,通道类型
  返回:返回插零时间 */
int  OPT_FrZeroTime(uint8_t  *pucData,int  iDataLen,int  iOptType);


/*获得光纵通道的状态报告
  参数: iOptChNum, 光纵通道号
        pRtOptChStsRpt, 返回光纵通道的状态报告
  返回:成功与否  */
EP_STATUS  OPT_GetOptChStsRpt(int  iOptChNum,OPT_CH_STS_REPORT  * pRtOptChStsRpt);


/*获得光纵通道的上次有效接收时间    2013-5-20日 张云
  参数: iOptChNum, 光纵通道号
  返回: 上次有效接收时间BASE */
OPT_TIME_BASE  OPT_GetOptChLastRecvValidDataTime(int  iOptChNum);


/* 处理新接收到的帧，供逻辑图任务调用  2006-11-27日 张云
   参数   iOptCh
   返回   EP_SUCCESS,操作成功
          其他，操作失败*/
EP_STATUS   OPT_DealChNewRecvFrm(int  iOptCh);


/*获得某通道的通信时间，供逻辑图用
  参数： iOptChNum，通道号
  返回： 该通道的通信时间 单位US 2006-12-21 张云 */
uint32_t   OPT_GetChComTime(int  iOptChNum);


/*获得某通道的实际通信时间，供显示用，不做其他用途
  参数： iOptChNum，通道号
  返回： 该通道的通信时间 单位US 2006-12-21 张云 */
uint32_t   OPT_GetChRealComTime(int  iOptChNum);



/*功能： 判定光差通道的当前发送数据是否可信
  参数： iOptChNum，通道号
  返回： 当前发送数据可信与否
         TRUE，不可信
         FALSE，可信
  2009-2-15 张云 */
BOOL   bOptSndDataIsUncredible(int  iOptChNum);

/* 获取光差通道延迟点数
 * Para:
 *     iOptChNum, 通道号.
 * Return:
 *     延迟点数.
 */
extern uint8_t OPT_GetOptChDelay(int iOptChNum);

#ifdef	__cplusplus
}
#endif

#endif                                  /* DSPAI_H */
