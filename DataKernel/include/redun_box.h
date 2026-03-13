/* redun_box.h - This file includes the interface to redundant box */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 30sep06, dy  create the firt vertion.
*/

/*
DESCRIPTION
This file includes the interface to redundant box.
*/

#ifndef REDUN_BOX_H
#define REDUN_BOX_H

/* includes */

#include "dspai.h"
#include "Ao_Drv.h"
#include "vxworks_type.h"

/* defines */

#define REDUNBUFLENGTH 82		/* 缓冲区长度 */
#define LINKBUFLENGTH 8		/* 缓冲区长度 */
#define HEADLENGTH	3
#define DOLENGTH 2
#define CHECKOUTLENGTH 1

#define REDUN_COM_PRTCL_TYPE 1
#define REDUN_COM_PRTCL_VER_LOW 0x01
#define REDUN_COM_PRTCL_VER_HIGH 0x01
#define MAX_AO_MAX 16
#define MAX_AI_MAX 16
#define MAX_DO_MAX 32
#define MAX_DI_MAX 32
#define COMINTERVAL 4	/* 延时数*/
#define COMBUFLENGTH (10*REDUNBUFLENGTH)		/* 通讯缓冲区长度 */

/* typedefs */

typedef struct		/* DI配置 */
{
    uint8_t ucMod;		/* 模件号 */
    uint8_t ucHdCh;			/* 通道号 */
    uint8_t aucFilt[4];		/* 去抖时间 */
} REDUN_DI_CFG;

typedef struct		/* DI操作 */
{
    uint32_t *pulStsPos;
    uint32_t ulStsMsk;
} REDUN_DI_HND;

typedef struct		/* DO配置 */
{
    int iModAddr;
    int iChIdx;
    uint32_t ulPassWd;
} REDUN_DO_CFG;

typedef struct		/* DO操作 */
{
    uint32_t *pulStsPos;
    uint32_t ulStsMsk;
} REDUN_DO_HND;

typedef enum tag_BWTYPE      /* 报文类型*/
{
    BW_DATA = 0x01,         /* 数据报文*/
    BW_YES,     /* 确认报文 */
    BW_NO,        /* 否认报文 */
    BW_LINK,		/* 主机发连接报文*/
    BW_LINK_RETURN,		/* 从机连接回复报文 */
    BW_LINK_AFFIRM,		/* 主机确认连接报文 */
} BWTYPE;

typedef struct tag_RECEIVEBUF		/* 接收 */
{
    uint8_t bwType;	/* 报文类型*/
    uint8_t bwLength;
    uint8_t *pReceive;
    float fReceive[MAX_DA_AO_NUM];
    uint32_t Di;
    BOOL stsRe;	/* 接收正确*/
    BOOL stsSe;		/* 上次发送正确 */
} RECEIVEBUF;

typedef struct tag_SENDBUF		/* 发送*/
{
    uint8_t bwType;
    uint8_t bwLength;
    char *pSend;
    union
    {
        float fSend;
        uint32_t ulSend;
    } Val[MAX_DA_AO_NUM];
    uint32_t Do;
    BOOL stsSe;
    BOOL stsRe;
} SENDBUF;

typedef struct REDUNBOXCOMATTR_tag		/* 扩展机箱通讯有关属性 */
{
    BOOL Redun_Connected;
    BOOL MasterFlag;
    BOOL SlaveFlag;
    BOOL SlaveConnected;
    BOOL MasterMessageReceived;
    BOOL SlaveSendFlag;
    BOOL SendAdmitFlag_g;
    uint32_t ulRedunBoxComSuccessCounter;		/* 通讯成功计数 */
    uint32_t ulRedunBoxComFailCounter;					/* 通讯失败计数 */
    uint32_t ulWaitCount;
    uint32_t ulCntCall;				/* 呼唤次数 */
} REDUNBOXCOMATTR;

typedef struct COMBUF_tag
{
    char ComBuf[COMBUFLENGTH];
    char *pHeadComBuf;
    char *pEndComBuf;
    char *pFront;
    char *pRear;
    uint32_t ulByteNum;		/* 接收总的字节数 */
} COMBUF;

/* globals */

extern int uiRedunLgcCh_g;				/* 瞬时值处理数 */
extern int iRedunDiNum_g;                 /* 扩展机箱DI总数 */
extern int iRedunDoNum_g;					/* 冗余机箱DO总数 */

extern int com422Fd;
extern unsigned char sendBuf[REDUNBUFLENGTH];		/* 发送缓冲区 */
extern unsigned char receiveBuf[REDUNBUFLENGTH];	/* 接受缓冲区 */
extern uint32_t RedunDiSend;
extern DA_PART_AO_CFG PartRedunAoCfgDa_g;  /* DA输出的AO配置 */

extern RECEIVEBUF rebuf;	/* 接受结构 */
extern SENDBUF sebuf;	/* 发送结构 */
extern BOOL InitRedunBoxFinishedFlag;		/* 初始化完成标志 */
extern SEM_ID RedunBoxNewSend; 			/* 新数据到来信号灯，由Timer3提供 */
extern float RedunReceiveResult[MAX_DA_AO_NUM];
extern REDUNBOXCOMATTR RedunBoxComAttr;
extern COMBUF ComBuf;

/***********************************************************************
* Init_Redun_Box - 初始化（并启动）冗余机箱
*
* RETURNS:
*        EP_SUCCESS, 正常返回
*        EP_BUF_ERR, 内存错误
*        EP_COM_ERR, 扩展机箱通信出错
*        EP_ERROR, 出错
*
*/
EP_STATUS Init_Redun_Box(
    u_int uiSmplRate, 	/* 采样速率 */
    u_int uiSysFreq, 		/* 系统频率 */
    u_int uiTxPts,		/* 每次传送采样点数 */
    void *pvAiMod, 			/* 该模块（扩展机箱负责的所有AI采集/计算通道）的句柄 */
    u_int uiLgcCh, 	/* 采样的逻辑通道数 */
    DSP_LGC_AI_CFG *plgccfg,		/* 指向逻辑通道配置数组第0个元素的指针，数组元素有unLgcCh个 */
    u_int uiCalcCfg, 		/* 预处理通道配置数 */
    DSP_CALC_AI_CFG *pcalccfg		/* 指向预处理通道配置数组第0个元素的指针，数组元素uiCalcCfg个*/
);

/***********************************************************************
* Redun_Init_DI - 初始化DI通道
*
* RETURNS: 用来索引DI通道的void指针，或者NULL表示调用出错
*
*/
void *Redun_Init_DI(
    int iModAddr, 	/* 模块硬件地址 */
    u_int uiCh, 		/* 在本模件内的DI物理通道号，从0开始 */
    uint32_t ulFilt		/* 去抖动时间，单位us */
);

/***********************************************************************
* Redun_Init_DO - 初始化DO通道
*
* RETURNS: 用来索引DO通道的void指针，或者NULL表示调用出错
*
*/
void *Redun_Init_DO(
    int iModAddr, 		/* 模块硬件地址 */
    u_int uiCh		/* 在本模件内的DO物理通道号，从0开始 */
);

/********************************************************************************/
/* 子程序名: Redun_Rd_Data                                                         */
/* 入口参数: 无                                                                 */
/* 出口参数: 无                                                                 */
/* 功能:     读取扩展机箱实时AI/DI数据                                          */
/********************************************************************************/
EP_STATUS Redun_Rd_Data(void);

/********************************************************************************/
/* 子程序名: Redun_Wr_Data                                                         */
/* 入口参数: 无                                                                 */
/* 出口参数: 无                                                                 */
/* 功能:     向冗余机箱写实时AO/DO数据                                          */
/********************************************************************************/
EP_STATUS Redun_Wr_Data(void);

/*  初始化DA输出的AO所有配置
     参数：
                 iAOCfgNum:所有AO配置个数
     返回：成功与否*/
EP_STATUS   DA_InitRedunAOCfg(int  iAOCfgNum);


/* 初始化DA输出中的中间结果源的AO通道
 * 参数：
 *             pElemIOSrc,逻辑图中间结果输出指针
 * 返回值： EP_STATUS  成功与否
            EP_SUCCESS,成功，其他失败
*/
EP_STATUS  DA_Redun_Init_Mid_Src_AO(int  iSrcType, u_int uiCh, void  *pElemIOSrc, float fCoff);

/* 写422串口 */
int Write_Com_Port(char *psendBuf, int sendNum);

/*读422串口 */
int Read_Com_Port(char *preceiveBuf, int receiveNum);

/* DO发送 */
void Redun_DO_Send(void);

/* DI接收*/
void Redun_DI_Receive(void);

/* 控制冗余机箱DO数据实时输出
 * 参数：
 *             pvDoCh，用来索引DO数据元素的void指针，应该是Redun_Init_DO的返回值
 *             bClose，TRUE=控合；FALSE=控分
 * 返回值： 无 */
void Redun_Set_DO(void *pvDoCh, BOOL bClose);

/* 读取DI数据实时状态
 * 参数：pvDiCh，用来索引DI数据元素的void指针，应该是Redun_Init_DI的返回值
 * 返回值：此DI通道的当前状态，TRUE=闭合；FALSE=打开 */

/* 发送测试 */
void Redun_Wr_Data_Test(void);

/* 接收测试 */
void Redun_Rd_Data_Test(void);

/********************************************************************************/
/* 子函数名: Redun_Wr_Ack                                                        */
/* 入口参数: 无                                                                 */
/* 出口参数: 无                                                                 */
/* 功能: 应答抱文                                          */
/********************************************************************************/
EP_STATUS Redun_Wr_Ack(uint8_t BwType);

/* 结果缓冲区写入*/
void RedunBoxResultWrite(void);

/***********************************************************************
* GetReBufNumMaster -  获取接收缓冲区字节数
*
* RETURNS: 无
*
*/
int GetReBufNumMaster(void);

/***********************************************************************
* GetReBufNumSlave -  获取接收缓冲区字节数
*
* RETURNS: 无
*
*/
int GetReBufNumSlave(void);

/***********************************************************************
* SetRedunAoTest -  测试AO
*
* RETURNS: 无
*
*/
void SetRedunAoOutTest(void);

/***********************************************************************
* Redun_422Initialize -与冗余机箱422通讯初始化
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL Redun_422Initialize(
    int channel		/* 串口号 */
);

/***********************************************************************
* Task_Serial -串口任务
*
* RETURNS: 无
*
*/
VOID Task_Serial(
    int channel		/* 串口号 */
);

__inline__ static BOOL Redun_Get_DI(void *pvDiCh)
{
    REDUN_DI_HND *phnd;

    phnd=(REDUN_DI_HND*)pvDiCh;
    /* 原代码有BUG,张云改过 */
    /* return (*phnd->pulStsPos & phnd->ulStsMsk); */

    if(*phnd->pulStsPos & phnd->ulStsMsk)
        return  TRUE;
    else
        return  FALSE;
}
#endif
