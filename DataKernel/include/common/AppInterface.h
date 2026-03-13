/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*       AppInterface.h                                    1.0                      */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了应用模块对平台模块逆向调用的接口头文件                       */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                        */
/*                                                                              */
/*         张云       2007.7.16                创建文件1.0版本                 */
/*                                                                             */
/*                                                                              */
/********************************************************************************/

#ifndef APP_INTERFACE_H
#define APP_INTERFACE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "vxWorks.h"
#include "EdpVer.h"
#include "logic.h"
#include "edpbase.h"

/* globals */

/* 参数(内部定值, 保护定值, 测控定值, 压板)校验相关, 所有定值指向内存区均为只读 */

extern int iSetPgNum_g; /* 定值页数, 其中第0页为内部定值, 其它为保护定值 */
extern SC_SET_PAGE *psetpgWr_g; /* 固化定值指针 */
extern SC_SET_PAGE *psetpg_g; /* 运行定值指针 */

extern int iLinkNum_g; /* 压板个数 */
extern SC_LINK_ITEM *plinkWr_g; /* 固化压板指针 */
extern SC_LINK_ITEM *plink_g; /* 运行压板指针 */

extern int iCkSetNum_g; /* 测控定值个数 */
extern SC_SET_ITEM *pCksetWr_g; /* 固化测控定值指针, pucUnitName, 单位名称不可用 */
extern SC_SET_ITEM *pCkset_g;   /* 运行测控定值指针 */

/*****************************************光纵模块的由应用调用的相关函数****************************************************/

/*功能：设置总的光纵hdlc clock的主从方式(对通道1和通道2都适用),缺省处理是主方式
  参数，  bSetIsMaster，TRUE,为主，FALSE为从
  返回，无*/
void  OPT_SetHdlcClkMasterMode(BOOL  bSetIsMaster);


/*功能：设置单通道的光纵hdlc clock的主从方式,缺省处理是主方式,2009-4-14日　ZY
  参数， iOptChNum,光差通道编号，0代表通道1,1代表通道2,其他值无效
         bSetIsMaster，TRUE,为主，FALSE为从
  返回，无*/
void  OPT_SetSingleHdlcClkMasterMode(int  iOptChNum,BOOL  bSetIsMaster);

/*功能：设置光纵的运行模式,缺省处理是2端运行模式
  参数，  iRunMode，0为2端运行模式，1为3端运行模式，其他保留
  返回，无*/
void OPT_SetRunMode(int  iRunMode);


/*功能：设置光纵通道的冗余运行模式,缺省处理是单通道不冗余模式
  参数，  iRedundMode，0为单通道不冗余模式，1为双通道冗余模式，其他保留
  返回，  无*/
void  OPT_SetRedundMode(int  iRedundMode);


/*功能：设置光纵通道1自环模式,缺省处理是正常模式
  参数，bSetCh1IsSelfCircle，TRUE，设置为自环模式，FALSE，设置为正常模式  */
void  OPT_SetCh1SelfCircleMode(BOOL  bSetCh1IsSelfCircle);


/*功能：设置光纵通道2自环模式,缺省处理是正常模式
  参数，bSetCh2IsSelfCircle，TRUE，设置为自环模式，FALSE，设置为正常模式  */
void  OPT_SetCh2SelfCircleMode(BOOL  bSetCh2IsSelfCircle);


/*功能：通知平台，光纵通道1差动保护是否已经投入。缺省处理是差动保护已投入
  参数， bCh1DiffIsRun，TRUE，通道1差动保护已经投入，FALSE，通道1差动保护已退出
  返回，无
  注意：只有当是3端模式时，有意义 */
void  OPT_NotifyCh1DiffRunExit(BOOL  bCh1DiffIsRun);


/*功能：通知平台，光纵通道2差动保护是否已经投入。缺省处理是差动保护已投入
  参数， bCh2DiffIsRun，TRUE，通道2差动保护已经投入，FALSE，通道2差动保护已退出
  返回，无
  注意：只有当是3端模式时，有意义 */
void  OPT_NotifyCh2DiffRunExit(BOOL  bCh2DiffIsRun);


/* 功能：获得光纵虚拟机箱某时刻通道数据接收有效与否和采样同步与否标志，张云2006-7-28日改动，2006-11-14日 张云
   参数：iOptCh  光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
         ulAiCnt 采样点号
         plRtTsse  用来返回通道有效时的采样同步误差（微秒）,
　　　　　　　　　　为正，表示本机采样领先，需要减慢采样，为负，表示本机采样落后，需要加快采样
         pbRtComValid  用来返回通道此时的通信是否有效标志，为TRUE，表示此刻最近有接收，通道接收正常，
                                                           为FALSE，表示通道接收异常
   返回值：该光纵通道数据采样同步有效与否
           若此刻通道通信正常并且接收到的同步数据正常，则返回TRUE
           若此刻光纵通道异常，或此刻同步数据无效，则返回FALSE
   注意：
          保护人员可以调用
*/
BOOL       OPT_Ch_Is_Valid(
    int  iOptCh,
    uint32_t   ulAiCnt,
    int32_t  *plRtTsse,
    BOOL  *pbRtComValid
);


/* 功能：获得光纵虚拟机箱某时刻通道数据的通信稳定与否标志
         即通道通信时间是否稳定
   参数：iOptCh  光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
         ulAiCnt 采样点号
   返回:
 		TRUE，通信稳定
    FALSE，通信不稳定

  注意：
       保护人员可以调用
*/
BOOL OPT_Ch_Com_Sts(
    int iOptCh,
    uint32_t ulAiCnt
);


/* 功能：获得光纵虚拟机箱某时刻，通道的接收数据可信状态  2009-3-5 ZY
                                 和收发时间差是否变化的状态

   参数：iOptCh  光纵虚拟机箱号，0代表光纵通道1，1代表光纵通道2，其他无效
         ulAiCnt 采样点号
         piRtRcvSndChgDif,供返回前后两次通信稳定状态变化期间，通道的收发时间差变化值，单位US
            Value=本次稳定时真实收发时间差-上次稳定时真实收发时间差
            若为0，表示没有发生变化，或无法判定
            若非0，表示此次判定出来的收发时间差的变化值
   返回值：TRUE,表示对方送过来的数据内容可信，
           FALSE，表示对方送过来的数据内容不可信，
   注意：保护人员可以调用
*/
BOOL  OPT_Ch_Data_Is_Credible(
    int iOptCh,
    uint32_t ulAiCnt,
    int *  piRtRcvSndChgDif
);


/*功能：获得本机节点的光差主从同步相关信息 2007-11-28-张云
  参数， pbRtNodeIsMaster，供返回该节点的同步主从标志的BOOL变量的地址(变量由调用方分配)
           ，TRUE为主，FALSE为从
         puiRtNodeRandCode，供返回该节点的16位无符号随机通道编码变量的地址(变量由调用方分配)
  返回，TRUE，表示获得信息有效
        FALSE，表示获得信息无效(比如未配置)
  注意：*/
BOOL  OPT_GetLocalNodeInfo(BOOL  * pbRtNodeIsMaster,uint16_t * puiRtNodeRandCode);

/*功能：获得通道1对端节点的光差主从同步相关信息 2007-11-28-张云
  参数， pbRtNodeIsMaster，供返回该节点的同步主从标志的BOOL变量的地址(变量由调用方分配)
           ，TRUE为主，FALSE为从
         puiRtNodeRandCode，供返回该节点的16位无符号随机通道编码变量的地址(变量由调用方分配)
  返回，TRUE，表示获得信息有效
        FALSE，表示获得信息无效(比如未配置或通信中断)
  注意：*/
BOOL  OPT_GetCh1PeerNodeInfo(BOOL  * pbRtNodeIsMaster,uint16_t * puiRtNodeRandCode);

/*功能：获得通道2对端节点的光差主从同步相关信息 2007-11-28-张云
  参数， pbRtNodeIsMaster，供返回该节点的同步主从标志的BOOL变量的地址(变量由调用方分配)
           ，TRUE为主，FALSE为从
         puiRtNodeRandCode，供返回该节点的16位无符号随机通道编码变量的地址(变量由调用方分配)
  返回，TRUE，表示获得信息有效
        FALSE，表示获得信息无效(比如未配置或通信中断)
  注意：*/
BOOL  OPT_GetCh2PeerNodeInfo(BOOL  * pbRtNodeIsMaster,uint16_t * puiRtNodeRandCode);


/*****************************************呼唤相关函数*****************************************************/
/* 功能：注册某应用对应的呼唤
   参数，无
   返回：该呼唤对应的操作句柄。若为NULL，表示注册未成功
   注意：呼唤可由多个应用驱动。
         任何一个应用发出呼唤，则驱动呼唤继电器，只有所有呼唤都收回，才收回呼唤驱动继电器
         该函数由需要发出呼唤的相关应用初始化时，调用一次。得到句柄。
         以后设置和收回呼唤，就由该对应句柄来进行操作。 */
void  *   ER_RegAlertSignal();

/*功能：设置某应用对应的呼唤
  参数：pvAlertHdl，该呼唤对应的句柄，由ER_RegAlertSignal函数得到。
  返回，无  */
void   ER_SetAlertSignal(void  * pvAlertHdl);

/*功能：收回某应用对应的呼唤
  参数：pvAlertHdl，该呼唤对应的句柄，由ER_RegAlertSignal函数得到。
  返回，无  */
void   ER_ClearAlertSignal(void  * pvAlertHdl);



/*****************************************其他由应用调用的相关函数************************************************************/

/*功能：切换系统5A或1A的工作环境
  参数，iSet5A1AEnv，5为5A，1为1A，其他保留
  返回，无 */
void  RD_Switch5A1ASysEnv(int  iSet5A1AEnv);

/*功能：保护CPU点亮或熄灭运行灯
  参数：bLightLamp,点亮运行灯，TRUE，点亮，FALSE，熄灭
  返回：无
  注意：保护应用程序需要在快速保护任务中调用该函数。函数调用频率必须大于50HZ，否则运行灯熄灭 */
void  RD_LightRunLamp(BOOL   bLightLamp);

/***********************************************************************
* SC_Work_Set_Area - Get working setting area number.
*
* RETURNS: Working setting area number.
*
* Alert:
*        Working setting area may be not a valid setting area.  This error is reported by other way.
*
*/
int SC_Work_Set_Area(void);

/***********************************************************************
* EP_ChgSysFreq -切换系统频率
*
* RETURNS: 无
*
*/
void EP_ChgSysFreq(
    int32_t iFreqType					/* 系统频率，0: 50Hz；1: 60Hz */
);

/***********************************************************************
* EP_ChgAcMdType -切换交流模件类型
*
* RETURNS: 无
*
*/
void EP_ChgAcMdType(
    int32_t iMdType					/* 模件类型，根据索引定值所配确定，如第0页: 1A；第1页: 5A */
);

/*2008-3-07 DQ
  设置同杆并架模块在保护应用中是否使用,
  平台根据该函数设置状态确定是否提示同杆通信状态,平台程序默认该状态为不使用;
  参数  bIsUsed: TRUE 设置同杆并架模块在保护应用中使用
                 FALSE 设置同杆并架模块在保护应用中不使用
*/
void  POLE_SetUsedState(BOOL bIsUsed);

/***********************************************************************
* RE_Refresh_EdpVer_Info - 更新版本设置函数，在扫描初始化函数中调用
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS RE_Refresh_EdpVer_Info(
    EP_GET_USR_VER_FUNC_TYPE pGetUsrVerEntryFunc
);

/***********************************************************************
* RE_Refresh_EdpMmiVer_Info - 更新版本设置函数，mmi初始化时调用
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS RE_Refresh_EdpMmiVer_Info(
    EP_GET_USR_VER_FUNC_TYPE  pGetMmiVerEntryFunc
);

/***********************************************************************
* GetBaseUnitFstRatedVal - 获取基准单元的1次额定值
*
* RETURNS: 1次额定值
*
*/
uint16_t GetBaseUnitFstRatedVal(void);

/***********************************************************************
* GetBaseUnitSecRatedVal - 获取基准单元的2次额定值
*
* RETURNS: 2次额定值
*
*/
uint16_t GetBaseUnitSecRatedVal(void);

/***********************************************************************
* GetChnRatedVal - 获取通道额定值
*
* RETURNS: 无
*
*/
void GetChnRatedVal(
    void *pvLgcAiHnd,				/* 句柄 */
    uint32_t *pFstRatedVal, 			/* 一次额定值 */
    uint16_t *pSecRatedVal							/* 二次值 */
);

/***********************************************************************
* GetCurSysFreq - 获取当前系统频率
*
* RETURNS: 当前系统频率，50Hz或60Hz
*
*/
u_int GetCurSysFreq(void);

/***********************************************************************
* SetAdSampFreq - 设定AD采样频率
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
EP_STATUS SetAdSampFreq(
    float fPwrFreq       	/* 系统当前频率，频率范围为45->65 */
);

/***********************************************************************
* GetDiChgTime - 获取DI变位时间
*
* RETURNS: 变位时间，单位为us
*
*/
uint32_t GetDiChgTime(
    void *pSrc		/* DI句柄 */
);

/***********************************************************************
* 功能:通过传递指针的方式获取DI变位UTC时间
* 参数:
*       pSrc, DI句柄;
*       pDestUtcTime, 传递UTC时间的指针;
* 返回:
*       无.
*/
extern void GetDiChgUTCTimeByPtr(
    void *pSrc,	/* DI句柄 */
    uint64_t *pDestUtcTime
);

/***********************************************************************
* GetDiChgPacketTime - 获取DI变位报文到达时间
*
* RETURNS: 变位时间，单位为us
*
*/
uint32_t GetDiChgPacketTime(
    void *pSrc		/* DI句柄 */
);

/***********************************************************************
* GetFarStFlag - 获取是否处于远方态标志
*
* RETURNS: TRUE: 远方态, or FALSE: 就地态
*
*/
BOOL GetFarStFlag(void);

/***********************************************************************
* GetExamStFlag - 获取是否处于检修态标志
*
* RETURNS: TRUE: 检修态, or FALSE: 正常运行态
*
*/
BOOL GetExamStFlag(void);

/***********************************************************************
* GetDealSampExamFlag - 是否启用数字化采样值检修不一致判断
*
* RETURNS: TRUE: 判别检修状态, or FALSE: 不判别检修状态
*
*/
BOOL GetDealSampExamFlag(void);

/************************************************************************
  功能：设置装置设备名称，2008-7-25   张云
  参数：pucDevName: 装置设备名称字符串基址，
        iNameLen，设备名称字符串长度(注意，不包括"\0")。
*/
void  EP_SetDevName(uint8_t  *pucDevName,int   iNameLen);

/***********************************************************************
* VI_SetMeaDo - 逻辑图中执行实际遥控时调用，通知MMI调用成功
*
* RETURNS: 无
*
*/
void VI_SetMeaDo(
    int irtPtNum		/* 遥控点号 */
);

/***********************************************************************
* EP_Setlock - 设置挂锁状态.
*
* RETURNS: 无
*
*/
void EP_Setlock(void);

/***********************************************************************
* EP_SetUnlock - 设置解锁状态.
*
* RETURNS: 无
*
*/
void EP_SetUnlock(void);

/***********************************************************************
* EP_GetLockSts - 获取解挂锁状态，同时清除该状态.
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL EP_GetLockSts(void);

/***********************************************************************
* Set02CPU - 设定第2块CPU位置.
*
* RETURNS: NONE
*
*/
void EP_Set02CPU(void);

/* Judge if the second board.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, this board is the second.
 *     FALSE, not.
 */
extern BOOL isNumber_2_04CPU(void);

/* Initialize the inner serial port.
 * Para:
 *     NONE.
 * Return:
 *     ERROR, OK.
 */
extern int Init_Inner_Uart();

/* Set the baud rate and parity method of inner serial port.
 * Para:
 *     baud, baud rate, using MACRO DEFAULT_INNER_UART_BAUD.
 *     parity, parity method, using UART_IOCTL_PARITY_NONE,
 *             UART_IOCTL_PARITY_EVEN, or UART_IOCTL_PARITY_ODD.
 * Return:
 *     ERROR, OK.
 */
extern int Init_Inner_Uart_Baud_Parity(unsigned int baud, char parity);

/* send data through the inner serial port.
 * Para:
 *     sendData, pointer of data.
 *     len, length of data.
 * Return:
 *     ERROR, OK.
 */
extern int Inner_Uart_Send(UINT8 *sendData, UINT16 len);

/* receive data through the inner serial port.
 * Para:
 *     ppBuf, pointer of pointer to the receive buffer.
 *     pFrameCount, number of residual frame.
 *     iTimeout, ticks of waiting; if 0, no waiting.
 * Return:
 *     the number of bytes received, or -1 when no byte.
 */
extern int Inner_Uart_Recv(uint8_t **ppBuf, uint8_t *pFrameCount, int iTimeout);

/* set flag not using remote control feedback.
 * Para:
 *     bType, remote control type;
 *     TRUE: include prep telecommand and feedback;
 *     FALSE: not include.
 * Return:
 *     NONE.
 */
extern void VI_SetRemoteControlFlag(BOOL bType);

/* 设定在定时器定时中断函数中发送SPI数据帧（提高SOE分辨率）.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void SIO_SetIsrSndSPIFlag(void);


extern BOOL Get_Goose_SubNum_and_NetNum(int *SubNum,int *MaxNetNum);


extern BOOL Get_Goose_Comm_Status(int SubNo,int NetNo);

extern BOOL Get_Goose_Sub_Net_Cfg(int SubNo,int NetNo);

/* 设置SPI通信主从标志
 * Para:
 *     1,主CPU;
 *     2,从CPU.
 * Return:
 *     NONE.
 */
extern void EP_Set02CPUPos(uint8_t ucPos);

/* 更新算法元件表的相关信息
 * Para:
 *     pSuanfaElemArr,算法表首地址.
 *     nSuanfaElemMapCount,算法表个数.
 *     pSuanfaDebugEntryFunc,算法调试入口函数.
 * Return:
 *     EP_STATUS, or EP_ERROR.
 */
extern EP_STATUS RE_Refresh_SuanfaTable_Info(EP_EXT_ELEM_MAP *pSuanfaElemArr, uint32_t nSuanfaElemMapCount,
        EP_DEBUG_PART_FUNC_TYPE pSuanfaDebugEntryFunc);

/* 获取DI品质(应用调用,传入DI句柄).
 * Para:
 *     pSrc, DI句柄.
 * Return:
 *     品质因素.
 */
extern uint16_t GetDiQuality(void *pSrc);

/* 设置采样通道一次额定值.(电压的单位为kV,电流的单位为A)
 * Para:
 *     pvLgcAiHnd, 通道句柄.
 *     uFstRatedVal, 一次额定值.(电压的单位为kV,电流的单位为A)
 * Return:
 *     NONE.
 */
extern void Set_AIFstRateVal(void *pvLgcAiHnd, uint16_t uFstRatedVal);

/* 设置采样通道一次额定值.(电压的单位为V,电流的单位为A)
 * Para:
 *     pvLgcAiHnd, 通道句柄.
 *     uFstRatedVal, 一次额定值.(电压的单位为V,电流的单位为A)
 * Return:
 *     NONE.
 */
extern void Set_AIFstRateVal2(void *pvLgcAiHnd, uint32_t uFstRatedVal);

/* 更新采样通道一次额定值.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void Update_AIFstRateVal(void);

/* 由应用向平台设置对时模式
 * Para:
 *     TimeSycMode.
 * Return:
 *     NONE.
 */
extern BOOL Add_TimeSycMode(int mode);

/* 由应用向平台设置插值脉冲方式
 * Para:
 *     InterPulseMode.
 * Return:
 *     NONE.
 */
extern BOOL Add_InterPulseMode(int mode);

/* 传统采样是否需要插值(需要在脚本文件autoexec.ini文件中调用)
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void RD_ifSampInsert(void);

/* 显示CPU空闲百分比,由shell调用
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void showIdleStat(void);

/* 调整统计间隔
 * Para:
 *     msInt, 间隔,ms.
 * Return:
 *     NONE.
 */
extern void setIdleStatInt(uint32_t msInt);

/* 获取SMV接收模式.
 * Para:
 *     NONE.
 * Return:
 *     0: 点对点; 1: 单网; 2: 双网.
 */
extern int32_t smvGetTransType(void);

/* 获取程序/配置匹配关系.
 * Para:
 *     NONE.
 * Return:
 *     TRUE: 匹配; FALSE: 不匹配.
 */
extern BOOL smvGetMacthSts(void);


/*功能：获取技术支持配置的选配功能代码
参数：pRtCodeStr:供返回的选配功能代码字符串，字符串空间由调用方分
配，被调用方填充，字符串以"\0"结尾。
           iStrMaxLen：调用方分配的选配功能代码字符串空间最大长度。
返回：成功与否  ,失败原因:
            0=解析成功;
            1=功能选配文件不存在;
            2=解析选配文件失败;
            3=传入参数错误;*/
extern UINT32  EP_GetFuncOptCode(uint8_t *pRtCodeStr,int  iStrMaxLen);

/* 获取一次巡检的最大FPGA GOOSE接收报文数.
 * Para:
 *     NONE.
 * Return:
 *     巡检最大报文数.
 */
extern int Get_Max_FPGA_Goose_Receive_Poll_One_Time(void);

/* 设置一次巡检的最大FPGA GOOSE接收报文数.
 * Para:
 *     NONE.
 * Return:
 *     设置后的最大报文数.
 */
extern int Set_Max_FPGA_Goose_Receive_Poll_One_Time(int pollnum);

/* 获取DI所对应虚端子状态.
 * Para:
 *     pvDiHnd, 开入通道.
 *     pucActTermNum, 实际虚端子个数.
 *     pucTermVal, 状态填写缓冲, 由应用传入, 数组长度固定为32字节(宏定义HDL_DI_MAX_RECV_NUM).
 * Return:
 *     NONE.
 * Alert:
 *     开入通道与虚端子的对应关系在GSE.xml文件中体现,
 *     在返回值缓冲区中体现配置的先后顺序。
 */
extern BOOL HDL_GetDiVtSts(void *pvDiHnd, uint8_t *pucActTermNum, uint8_t *pucTermVal);

/* 获取DI所对应虚端子状态及品质.
 * Para:
 *     pvDiHnd, 开入通道.
 *     pucActTermNum, 实际虚端子个数.
 *     pucTermVal, 状态填写缓冲, 由应用传入, 数组长度固定为32字节(宏定义HDL_DI_MAX_RECV_NUM).
 *     pusTermSts, 品质填写缓冲
 * Return:
 *     NONE.
 * Alert:
 *     开入通道与虚端子的对应关系在GSE.xml文件中体现,
 *     在返回值缓冲区中体现配置的先后顺序。
 */
extern BOOL HDL_GetDiVtValAndSts(void *pvDiHnd, uint8_t *pucActTermNum, uint8_t *pucTermVal,
                                 uint16_t *pusTermSts);

/* 获取DI所对应虚端子状态及品质.
 * Para:
 *     pvDiHnd, 开入通道.
 *     pucActTermNum, 实际虚端子个数.
 *     pucTermVal, 状态填写缓冲, 由应用传入, 数组长度固定为32字节(宏定义HDL_DI_MAX_RECV_NUM).
 *     pusTermSts, 品质填写缓冲.
 *     piSubDaVtIdx, 虚端子序号，从0开始.
 *     pulArrTmpNextCnt, 最近一次变位节拍.
 * Return:
 *     NONE.
 * Alert:
 *     开入通道与虚端子的对应关系在GSE.xml文件中体现,
 *     在返回值缓冲区中体现配置的先后顺序。
 */
extern BOOL HDL_GetDiVt(void *pvDiHnd, uint8_t *pucActTermNum, uint8_t *pucTermVal,
                        uint16_t *pusTermSts, int *piSubDaVtIdx, uint32_t *pulArrTmpNextCnt);

/* 设置通道非选配(不影响界面显示通道数目).
 * Para:
 *     iOptChNum, 通道号, 取值为0或1.
 * Return:
 *     TRUE or FALSE.
 */
extern BOOL OPT_SetChNotUsed(int iOptChNum);

/* 设置通道未配置,可影响界面显示通道数目,保测一体使用.
 * Para:
 *     iOptChNum, 通道号, 取值为0或1.
 * Return:
 *     TRUE or FALSE.
 */
extern BOOL OPT_SetChInitFlagFlase(int iOptChNum);

/* 设置采样通道二次额定值.
 * Para:
 *     pvLgcAiHnd, 通道句柄.
 *     fSecRatedVal, 二次额定值(浮点), 存储为16位整数, 扩大100倍.
 * Return:
 *     NONE.
 */
extern void Set_AISecRateVal(void *pvLgcAiHnd, float fSecRatedVal);

/* 设置采样通道二次额定值和索引定值页序.
 * Para:
 *     pvLgcAiHnd, 通道句柄.
 *     fSecRatedVal, 二次额定值(浮点), 存储为16位整数, 扩大100倍.
 *     iIndexSn, 大于等于0.
 * Return:
 *     NONE.
 */
extern void Set_AISecRateValAndIndexSn(void *pvLgcAiHnd, float fSecRatedVal, int32_t iIndexSn);

/* 获取过程层配置ASDU数.
 * Para:
 *     NONE.
 * Return:
 *     为-1时无效.
 */
extern int32_t SMV_GetAsduNum(void);

/* 获取间隔压板状态
 * Para:
 *     SubNo, 间隔号, 从1开始.
 * Return:
 *     TRUE(投入), FALSE(退出).
 */
extern BOOL HDL_GetYabanState(int32_t SubNo);

/* 获取GOOSE GOCB块的检修状态
 * Para:
 *     iSubIndex, GOCB块的序号，从1开始.
 * Return:
 *     TRUE, 检修状态;  FALSE, 非检修状态
 */
extern BOOL GetSubTestModeDirect(int iSubIndex);

/***********************************************************************
* ER_IsSetAlertFlag - 获得是否设置呼唤标志
*
* RETURNS: TRUE: 已经设置呼唤
*                 FALSE: 未设置呼唤
*
*/
extern BOOL ER_IsSetAlertFlag();

/* 获取通信状态.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL getInnerComSts(void);

/* 设置通道选配.
 * Para:
 *     iOptChNum, 通道号, 取值为0或1.
 * Return:
 *     TRUE or FALSE.
 */
extern BOOL OPT_SetChUsed(int iOptChNum);

/* 获取间隔检修不一致状态
 * Para:
 *     SubNo, 间隔号, 从1开始, 与其它接口保持一致.
 * Return:
 *     TRUE(不一致), FALSE(一致).
 */
extern BOOL HDL_GetRepairSts(int32_t SubNo);

/* 获取主CPU风暴抑制状态.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
extern BOOL EP_GetCPU1StormState(void);

/* 设置CPU2风暴抑制状态.
 * Para:
 *     bSts, TRUE or FALSE.
 * Return:
 *     NONE.
 */
extern void EP_SetCPU2StormState(BOOL bSts);

/***********************************************************************
* SET_GetRange - 获取软件配置中定值原始的最大值、最小值和默认值
* 参数:
* pSet : 定值句柄；
* pvalMax : 定值原始最大值；
* pvalMin : 定值原始最小值；
* pvalDft : 定值原始默认值；
* pvalStep: 定值原始默认步长；
*
* RETURNS: 返回获取状态
*                 EP_SUCCESS,获取成功
*                 EP_ERROR,获取失败
*
*/
extern EP_STATUS SET_GetRange(void *pSet,FLT_U32_UNION *pvalMax,
                              FLT_U32_UNION *pvalMin,FLT_U32_UNION *pvalDft,FLT_U32_UNION *pvalStep);


/***********************************************************************
* SET_SetRange - 设置定值的最大值、最小值和默认值
* 参数:
* pSet : 定值句柄；
* valMax : 定值最大值；
* valMin : 定值最小值；
* valDft : 定值默认值；
* valStep: 定值步长；
*
* RETURNS: 返回设置状态
*                 EP_SUCCESS,设置成功
*                 EP_ERROR,设置失败
*
*/
extern EP_STATUS SET_SetRange(void *pSet, FLT_U32_UNION valMax,
                              FLT_U32_UNION valMin, FLT_U32_UNION valDft, FLT_U32_UNION valStep);

/* 设置定值量程完成，触发慢速处理任务
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void SET_SetRangeOver(void);

/* 功能:获取温度监视值
 * 参数:无
 * 返回:int,温度值
*/
extern int WT_GetTemptWatchVal();

/* 功能:
 *      接口函数,用于应用触发遥测越限事件
 * 参数:
 *      iMeaCh, 遥测通道序号(从0开始)
 *      fMeaVal, 遥测值
 *      usMeaQuality, 遥测品质
 *      ulMeaCalcTm, 遥测越限事件时标
 * 返回:
 *      无
 */
extern void ME_New_Mea_Over(int iMeaCh, float fMeaVal, uint16_t usMeaQuality, uint32_t ulMeaCalcTm);

/*  功能:
 *      设置双点开入是否取反(设置后会覆盖以前根据类型判断的逻辑,未调用则遵循之前的逻辑)
 *  参数:
 *      bDpDiValIsCounter, TRUE取反状态, FALSE非取反状态
 *  返回值:
 *     无.
 */
extern void SetDpDiValIsCounter(BOOL bDpDiValIsCounter);

/***********************************************************************
* RD_Get_AI_Quality - 取得实时AI数据品质
*
* RETURNS: 品质位.
*
*/
extern uint16_t RD_Get_AI_Quality(
    void *pvAiHnd			/* 用来索引AI数据元素的void指针(RD_LGC_AI_CH)，应该通过调用本模块提供的RD_Get_Handle得到 */
);

/* 功能:
 *  设置记录GOOSE发送结束时间标识
 * 参数:
 *  bFlag, TRUE下一次记录
 * 返回:
 *  None.
 */
extern void SetGoSndTimeRecordFlag(BOOL bFlag);

/* 功能:
 *  获取记录的GOOSE发送结束时间
 * 参数:
 *  pGoSndTime,获取记录的GOOSE发送结束时间指针
 * 返回:
 *  None.
 */
extern void GetGoSndTime(uint64_t *pGoSndTime);

#ifdef	__cplusplus
}
#endif

#endif                                  /* APP_INTERFACE_H */
