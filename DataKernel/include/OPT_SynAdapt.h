/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                  */
/*                                                                              */
/*      OPT_SynAdapt.h                                    1.0                       */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了光纵同步自适应模块的头文件                                   */
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
/*         张云       2006.6.17                创建文件1.0版本              */
/*                                                                            */
/*                                                                              */
/********************************************************************************/



#ifndef OPT_SYN_ADAPT_H
#define OPT_SYN_ADAPT_H

#ifdef	__cplusplus
extern "C" {
#endif



typedef  struct
{
    BOOL   bIsMaster;    /*采样主从标志 */
    uint16_t   uiRandCode;/*同步随机数  */

}  OPT_NODE_MASTER_INFO_TYPE;/*光纵节点主从信息  */


typedef   struct
{
    BOOL   bChLocalRecvIsStable;   /*该光纵通道的本地接收稳定与否，只对通道1，通道2有意义 */
    BOOL   bChPeerRecvIsStable;    /*该光纵通道的对侧接收稳定与否 只对通道1，通道2有意义*/
    BOOL   bChLocalRelayYabanIsRun;  /*该光纵通道对应的本地保护压板投退状态  只对通道1，通道2有意义*/
    BOOL   bChPeerRelayYabanIsRun;   /*该光纵通道对应的对侧保护压板投退状态 只对通道1，通道2有意义 */
    BOOL   bChPeerSndDataIsUncredible;    /*该光纵通道对应的对侧发送数据不可信标志，1为不可信，0为可信，
                                            只对通道1，通道2有意义   2009-2-15 ZY*/
    BOOL   bChIsSelfCircle;          /*该通道自环与否  只对通道1，通道2有意义*/
    BOOL   bChRecvThirdChIsValid;    /*该通道接收到的第3个通道有效状态  只对通道1，通道2有意义*/
    BOOL   bChIsValid;               /*该通道判定有效与否,对通道1，通道2,通道3都有意义*/
}   OPT_CH_VALID_INFO_TYPE;   /*从本侧节点的角度的光纵通道有效性信息 只对通道1，通道2，通道3都有意义 */

typedef   enum
{
    CH000_STS=0,     /*通道1，2，3无效，独立运行态  */
    CH001_STS=1,     /*通道2，3无效， 2端运行态  */
    CH010_STS=2,     /*通道1，3无效， 2端运行态  */
    CH011_STS=3,     /*通道3无效，3端非完全运行态  */
    CH100_STS=4,     /*通道1，2无效，独立运行态  */
    CH101_STS=5,     /*通道2无效，3端非完全运行态   */
    CH110_STS=6,     /*通道1无效，3端非完全运行态  */
    CH111_STS=7,     /*通道1，2，3都有效，3端完全运行态  */
}  GOBAL_CH_STS_TYPE;/*从本侧节点的角度看光纵3个通道的有效性状态，BIT0，代表通道1，BIT1，代表通道2，BIT3代表通道3，  */


extern  OPT_NODE_MASTER_INFO_TYPE    aNodeMasterInfo_g[3];/*获得的三端主从同步状态  */
extern  OPT_CH_VALID_INFO_TYPE       aChValidInfo_g[3];   /*获得的3个通道有效状态  */
extern  uint32_t   ulChValidSts_g;    /*3个通道有效性状态，第0个BIT代表通道1，第1个BIT代表通道2，第2个BIT代表通道3  */

extern BOOL   bOptIs3EndRunMode_g;                          /*光纵是否是3端运行模式，TRUE为3端运行模式，FALSE为2端运行模式  */
extern BOOL   bOptChIsRedundMode_g;                           /*光纵是否是冗余2通道模式，TRUE为冗余2通道模式，FALSE为单通道模式    2006-7-12 */



/*获得新的随机数，
  参数 无
  返回  16位无符号随机数 */
uint16_t  OPT_GetNewRadom();

/*初始化光纵同步自适应模块
   参数，无；
   返回 EP_SUCCESS,操作成功
        否则，操作失败  */
EP_STATUS   OPT_InitSynAdapt();


/*更新3端光纵通道有效性状态
   参数   无
   返回，EP_SUCCESS,操作成功
         其他，操作失败
     */
EP_STATUS   OPT_3EndUpdateChValidSts();

/*光纵3端同步主从自适应处理
   参数，无
   返回  EP_SUCCESS,成功操作
         其他，失败*/
EP_STATUS   OPT_3EndSynAdaptDeal();


/*更新2端光纵通道有效性状态
   参数   无
   返回，EP_SUCCESS,操作成功
         其他，操作失败
     */
EP_STATUS   OPT_2EndUpdateChValidSts();

/*光纵2端同步主从自适应处理
   参数，无
   返回  EP_SUCCESS,成功操作
         其他，失败*/
EP_STATUS   OPT_2EndSynAdaptDeal();


/*光纵2端单通道主从自适应处理
   参数，无
   返回  EP_SUCCESS,成功操作
         其他，失败*/
EP_STATUS   OPT_2EndSingleChDeal();


/*光纵2端冗余双通道主从自适应处理
   参数，无
   返回  EP_SUCCESS,成功操作
         其他，失败*/
EP_STATUS   OPT_2EndRedundChDeal();

/*光纵同步主从自适应处理
   参数，无
   返回  EP_SUCCESS,成功操作
         其他，失败*/
EP_STATUS   OPT_SynAdaptDeal();


/* 光纵通道1，2，3都有效，3端完全运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt111StsDeal();


/* 通道1无效，3端非完全运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt110StsDeal();


/* 通道2无效，3端非完全运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt101StsDeal();


/* 通道1，2无效，独立运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt100StsDeal();


/* 通道3无效，3端非完全运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt011StsDeal();

/* 通道1，3无效， 2端运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt010StsDeal();

/* 通道2，3无效， 2端运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt001StsDeal();

/* 通道1，2，3无效，独立运行态
   主从自适应处理
   参数  无
   返回  EP_SUCCESS,操作成功
         其他，失败*/
EP_STATUS   OPT_Adapt000StsDeal();


#ifdef	__cplusplus
}
#endif

#endif                                  /* OPT_SYN_ADAPT_H */
