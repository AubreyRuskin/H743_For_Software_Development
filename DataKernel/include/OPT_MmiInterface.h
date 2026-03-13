/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                  */
/*                                                                              */
/*      OPT_MmiInterface.h                                    1.0               */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了光纵模块和MMI交互的头文件                             */
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
/*         张云       2006.4.11                创建文件1.0版本              */
/*                                                                            */
/*                                                                              */
/********************************************************************************/


#ifndef OPT_MMI_INTERFACE_H
#define OPT_MMI_INTERFACE_H

#include  "edpbase.h"

#ifdef	__cplusplus
extern "C" {
#endif


/*光纵通道状态报告，报告给MMI  */
typedef   struct
{
    uint8_t    ucOptChNum;            /*该光纵通道序号，从1开始  */
    BOOL       bChInitFlag;           /*2007-10-22 DQ: 将其意义从通道是否已初始化改为通道是否配置，TRUE表示已配置，否则表示未配置*/
    BOOL       bChComStableFlag;      /*通道通信稳定状态 ，TRUE，表示通道稳定，否则不稳定 */
    BOOL       bChSamSynFlag;         /*通道采样同步状态 ，TRUE，表示采样同步上，否则，未同步上 */

    uint32_t   ulFrComTime;              /*报文收发通信时间，单位微秒US  */
    uint32_t   ulTotalComInstableNum;    /*通道总的通信失稳次数， */
    uint32_t   ulTotalFrLostNum;         /*通道总的丢帧次数， */
    uint32_t   ulTotalFrDelayNum;        /*通道总的帧延迟次数， */
    uint32_t   ulTotalFrErrNum;          /*通道总的帧错误次数， */
    uint32_t   ulTotalChComeTimeChangeNum; /*通道总的通信时间变化次数， */
    uint32_t   ulTotalSamMissSynNum;      /*通道总的采样失步次数， */

    uint32_t   ulHdlcCrcErr;              /*通道hdlc总的CRC校验错误次数 */
    uint32_t   ulHdlcDpllErr;             /*通道hdlc总的锁相环错误次数  */
    uint32_t   ulHdlcAddrErr;             /*通道hdlc总的接收地址串扰错误次数  */
    uint32_t   ulHdlcCpmBusy;             /*通道hdlc总的因CPM忙导致的错误  */
    uint32_t   ulHdlcRcvBusy;             /*通道hdlc总的因接收忙导致的错误  */

    uint32_t   ulFrCRCErrNumPerSec;         /*通道每秒CRC帧错误次数， */
    uint32_t   ulFrLostNumPerSec;           /*通道每秒丢帧次数， */
    uint32_t   ulFrDelayNumPerSec;          /*通道每秒帧延迟次数，*/

    /*2006-6-20日添加  */
    BOOL   bLocalIsMaster;      /*本机同步主从信息，TRUE，本机为主，FALSE，本机为从  */
    BOOL   bPeerIsMaster;      /*该通道对侧的同步主从信息，TRUE，对侧为主，FALSE，对侧为从  */
    uint16_t  uiLocalRandCode;   /* 本侧随机编码 */
    uint16_t  uiPeerRandCode;    /* 该通道对侧随机编码 */

}  OPT_CH_STS_REPORT;



/*获得光纵通道的总数
  参数: 无
  返回:光纵通道个数  */
int OPT_GetOptChTotalNum();


/*获得光纵通道的状态报告
  参数: iOptChNum,光纵通道号,从0开始
        pRtOptChStsRpt, 返回光纵通道的状态报告
  返回:成功与否  */
EP_STATUS  OPT_GetOptChStsRpt(int  iOptChNum,OPT_CH_STS_REPORT  * pRtOptChStsRpt);

/*清除光纵通道状态
  参数: iOptChNum,光纵通道号,从0开始
  返回:成功与否  */
void    OPT_ClearOptChStsRpt(int  iOptChNum);


#ifdef	__cplusplus
}
#endif

#endif                                  /* OPT_MMI_INTERFACE_H */
