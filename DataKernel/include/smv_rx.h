/* smv_rx.c - subroutine library for receiving the iec-smv-9-2 packet */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02a, 16apr11, dy add new error status.
01c, 16sep10, zq optimize receiving.
01b, 29jul09, syx add receiving type.
01a, 31aug07, lgh first created.
*/

/*
DESCRIPTION
This module includes subroutine library for receiving the iec-smv-9-2 packet.
*/

#ifndef SMV_RX_H
#define SMV_RX_H

#include <vxWorks.h>

#ifdef	__cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/* includes */

#include "smvcfg.h"

/* defines */

#define MAX_BUFFERS 100 /* the max Buffers in one ASPU.`*/
#define SUB_ETHERNET_PACKET_A (0)
#define SUB_ETHERNET_PACKET_B (1)
#define SUB_ETHERNET_PACKET_C (2) /* SPT总线来源 */

#define MAXPOINTS 200

#define MAX_ASDU 60

#define ETHE_MINLEN 26
#define ETHE_MAXLEN 1522

/* CC板最大包数 */
#define MAXCCPACK 4

/* 单包中最大ASDU数 */
#define MAXCCPACKASDU 32

/* ASDU中最大通道数 */
#define MAX_CHN_NUM_IN_ASDU 30

/* MAC地址长度 */
#define MAC_LEN 6

/* 功能分类号 */
#define P2P_BUS_MASTER 0  /* 点对点母差主机(接传统子机)——入24点，出24点 */
#define P2P_BUS_TRANS_MASTER 1  /* 点对点母差/主变主机(接数字化子机)——入80点，出24、48、96点 */
#define P2P_LINE 2  /* 点对点线路光差——入80点，出24、48、96点 */
#define P2P_REC 3 /* 点对点录波主机——入80点，出80点 */
#define P2P_SLAVE 4 /* 点对点子机——入80点，出80点 */
#define NET_SINGLE_TRANS_LINE_BUS 8  /* 组网母差/主变/线路(非光差)单网——入80点，出24、48、96点 */
#define NET_SINGLE_LINE 9  /* 组网线路(光差)单网——入80点，出24、48、96点 */
#define NET_SINGLE_REC 10  /* 组网录波单网——入80点，出80点 */
#define NET_DOUBLE_TRANS_LINE_BUS 12  /* 组网母差/主变/线路(非光差) AB网——入80点，出24、48、96点 */
#define NET_DOUBLE_LINE 13  /* 组网线路(光差)AB网——入80点，出24、48、96点 */
#define NET_DOUBLE_REC 14  /* 组网录波AB网——入80点，出80点 */

/* uint32类型字节数,用于校验位段 */
#define UINT32_BYTE_NUM 4

#define MU_STS_MASK 0x000047FF  /* 本级状态标掩码 */
#define MU_STS_MASK_WITHOUT_CONFREV_CHECK 0x000007FF
#define SLAVE_STS_MASK 0x00004FFF    /* 前级状态标掩码 */
#define SLAVE_STS_MASK_WITHOUT_CONFREV_CHECK 0x00000FFF
#define NET_AB_MAST 0x0000517F  /* A/B网掩码,包含点对点传输的状态标,共13位 */
#define NET_AB_MAST_WITHOUT_CONFREV_CHECK 0x0000117F
#define DATA_SRC_POS 0x2000  /* 数据源位置,A/B网,表示数据来源 */
#define STS_POS_SHIFT 4   /* 状态偏移 */
#define NET_B_POS_SHIFT 13  /* B网状态偏移 */

/* 回退点最大点数 */
#define MAX_BACK_POINT_TYPE_3 17  /* 用于非光差模式, 周波96点 */
#define MAX_BACK_POINT_TYPE_9 5        /* 用于光差模式, 周波24点 */

/* 返回正常周波数 */
#define COM_BACK_CYCLE_NUM_TYPE_3 5  /* 非光差 */
#define COM_BACK_CYCLE_NUM_TYPE_9 2    /* 光差 */

/* 版本号 */
#define CC_3_0 0x30
#define CC_3_2 0x32
#define CC_2_1 0x21
#define CC_2_0 0x20

/* 通信方式 */
#define P_2_P 0  /* 点对点 */
#define SINGLE_NET 1  /* 单网 */
#define DOUBLE_NET 2     /* 双网 */

#define SV_ID_MAX_LEN 34  /* svID最大长度 */

/* typedefs */

/********************************************************************************/
/* 					This part is for the smv data inform info.					*/
/********************************************************************************/
typedef struct
{
    unsigned short *dataBuffer;
    unsigned short smpCount;	/* The first smpCount of the dataBuffer	*/
    /* This value is in the dataBuffer.	*/
    int asduNum;				/* The max asduNum is in the macro	*/
    /* MAX_ASDU_NUMBER.					*/
} SMV_MSG_INFO;

/* SMV采样帧信息,保存最新帧信息 */
typedef struct tag_SMVINFO
{
    UINT8 nDstMacAddr[MAC_LEN];
    UINT8 nSrcMacAddr[MAC_LEN];
    UINT16 nTPID;
    UINT16 nTCI;
    UINT16 nEthType;
    UINT16 nAPPID;
    UINT16 nAsduNum;
    UINT16 nMulAsduNum[MAXCCPACK];
    UINT16 nRatedPhsCur;
    UINT16 nRatedNeuCur;
    UINT16 nRatedPhsVol;
    UINT16 nRatedDlyTime;
    UINT8 nSmpRate;
    UINT8 nConfRevNo;
    UINT16 nChnNum[MAXCCPACK][MAXCCPACKASDU]; /* 单ASDU中通道个数 */
} SMVINFO;

typedef struct tag_SMVDATA
{
    UINT16 nSmpCount[MAX_BUFFERS];
    BOOL   bValid[MAX_BUFFERS];
    UINT16 nSendIndex;
    UINT16 nRecIndex;
    BOOL   bSmvCommOk;
    BOOL   bSampFirst;
    BOOL   bSampRec;
    UINT16 ComBackCountCnt;
    UINT16 SamCountCnt;
    UINT16 gRecCount;
    UINT16 gSendCount;
    uint32_t ulValidCnt;  /* 无有效点计数 */
    uint32_t ulSwitchCnt;  /* 有效到无效计数 */
} SMVDATA;

typedef struct
{
    UINT16 reserved1; /* 保留字节 */
    UINT16 reserved2;
    UINT32 backpoint;  /* 回退点数 */
    UINT32 CCVersion;     /* CC版本号 */
    uint32_t FuncNo;  /* 功能号 */
    BOOL bProgCfgMatchFlag;  /* 程序和配置匹配与否标识 */
    BOOL bCpuSyn;  /* CPU同步标识 */
    BOOL bNetOptSyn;  /* 组网光差外同步标志 */
    BOOL bLstNetOptSyn;  /* 组网光差外同步标志历史值 */
    BOOL bNetSynMod;  /* 组网光差同步模式, 0: 计数器同步; 1: 外同步 */
    BOOL bLstNetSynMod;  /* 组网光差同步模式历史值 */
    BOOL bP2PorNet;  /* 接收类型,TRUE: 点对点; FALSE: 组网 */
    uint32_t backPointLimit;
    uint32_t comBackCycleLimit;  /* 返回周波数 */
} SMV_CC_INFO;

/* 帧信息 */
typedef struct tag_PACK_INFO
{
    uint8_t allpackno; /* 总的分包数 */
    uint8_t nowpackno;   /* 当前分包序号 */
    uint8_t backpoint;  /* 回退点数,用于线路光差 */
    uint8_t CCVersion;   /* CC版本 */
    BOOL bMatchFlag;  /* 程序与配置匹配与否标识 */
    uint8_t ucFuncNo;  /* 功能分类号 */

    int head;  /* 第一个ASDU序号 */
    int back;    /* 最后一个ASDU序号 */

    int asduNum;  /* ASDU数 */

    IEC_SMV_CHAP *smvChapArr[MAXCCPACKASDU*MAX_CHN_NUM_IN_ASDU];
    int iChapNum;  /* 通道数 */
} PACK_INFO;

/* ASDU状态 */
typedef struct tag_ASDU_INFO
{
    uint16_t usDelayTime;  /* 本级处理后的额定延时,用于前级上送额定延时 */

    /* 连接合并单元信息字段 */
    union MU_INFO_UN
    {
        struct MU_INFO_ST
        {
            uint32_t V0:1;
            uint32_t reserved1:3;
            uint32_t muNetNo:4;
            uint32_t reserved2:8;
            uint32_t s13:3; /* ASDU是否取自该网 */
            uint32_t S12:1;
            uint32_t reserved4:1;
            uint32_t S10:1;
            uint32_t S9:1;
            uint32_t S8:1;
            uint32_t S7:1;
            uint32_t S6:1;
            uint32_t S5:1;
            uint32_t S4:1;
            uint32_t S3:1;
            uint32_t S2:1;
            uint32_t S1:1;
            uint32_t S0:1;
        } muInfo_st;

        uint32_t ulMuInfo;
    } muInfo;

    union MU_INFO_UN_BAK
    {
        struct MU_INFO_ST_BAK
        {
            uint32_t V0:1;
            uint32_t reserved1:3;
            uint32_t muNetNo:4;
            uint32_t reserved2:8;
            uint32_t s13:3; /* ASDU是否取自该网 */
            uint32_t S12:1;
            uint32_t reserved4:1;
            uint32_t S10:1;
            uint32_t S9:1;
            uint32_t S8:1;
            uint32_t S7:1;
            uint32_t S6:1;
            uint32_t S5:1;
            uint32_t S4:1;
            uint32_t S3:1;
            uint32_t S2:1;
            uint32_t S1:1;
            uint32_t S0:1;
        } muInfo_st_bak;

        uint32_t ulMuInfoBak;
    } muInfoBak;

    uint16_t AdrMuInfo;  /* 帧中偏移地址 */
    BOOL bMuFlag;  /* 主机是否异常 */

    /* 连接前级信息字段 */
    union SLAVE_INFO_UN
    {
        struct SLAVE_INFO_ST
        {
            uint32_t V0:1;
            uint32_t reserved1:3;
            uint32_t sNetNo:4;
            uint32_t reserved2:8;
            uint32_t reserved3:2;
            uint32_t s13:1;     /* ASDU是否取自该网 */
            uint32_t s12:1; /* 计数器同步异常 */
            uint32_t S11:1;
            uint32_t S10:1;
            uint32_t S9:1;
            uint32_t S8:1;
            uint32_t S7:1;
            uint32_t S6:1;
            uint32_t S5:1;
            uint32_t S4:1;
            uint32_t S3:1;
            uint32_t S2:1;
            uint32_t S1:1;
            uint32_t S0:1;
        } sInfo_st;

        uint32_t ulSlaveInfo;
    } sInfo;

    union SLAVE_INFO_UN_BAK
    {
        struct SLAVE_INFO_ST_BAK
        {
            uint32_t V0:1;
            uint32_t reserved1:3;
            uint32_t sNetNo:4;
            uint32_t reserved2:8;
            uint32_t reserved3:2;
            uint32_t s13:1;     /* ASDU是否取自该网 */
            uint32_t s12:1; /* 计数器同步异常 */
            uint32_t S11:1;
            uint32_t S10:1;
            uint32_t S9:1;
            uint32_t S8:1;
            uint32_t S7:1;
            uint32_t S6:1;
            uint32_t S5:1;
            uint32_t S4:1;
            uint32_t S3:1;
            uint32_t S2:1;
            uint32_t S1:1;
            uint32_t S0:1;
        } sInfo_st_bak;

        uint32_t ulSlaveInfoBak;
    } sInfoBak;

    uint16_t AdrSlaveInfo;  /* 帧中偏移地址 */
    BOOL bSlaveFlag;  /* 是否需要读取前级状态 */
    BOOL bLstSlaveFlag;  /* 上次是否需要读取前级状态 */
    BOOL bPortStsFlag;  /* 是否有端口信息 */

    BOOL bSyn;  /* ASDU同步标 */

    BOOL bNetAFlag;  /* 是否读取A网信息 */
    BOOL bNetBFlag;    /* 是否读取B网信息 */

    BOOL bLstNetAFlag;  /* 上次是否需要读取A网信息 */
    BOOL bLstNetBFlag;  /* 上次是否需要读取B网信息 */

    uint8_t arrSVID[SV_ID_MAX_LEN+1];  /* SVID */

    uint32_t ulPortSts;  /* 端口状态, 填写到所有通道状态中 */
} ASDU_INFO;

/* 9-2数据帧状态信息 */
typedef struct
{
    /* 数据帧中偏移地址信息 */
    uint16_t pAdrAppid[MAXCCPACK];
    uint16_t pAdrReserved1[MAXCCPACK];
    uint16_t pAdrReserved2[MAXCCPACK];
    uint16_t pAdrNoAsdus[MAXCCPACK];
    uint16_t pAdrSVID[MAXCCPACK][MAXCCPACKASDU];
    uint16_t pAdrCount[MAXCCPACK][MAXCCPACKASDU];
    uint16_t pAdrConfigRev[MAXCCPACK][MAXCCPACKASDU];
    uint16_t pAdrSyn[MAXCCPACK][MAXCCPACKASDU];
    uint16_t pAdrDataLen[MAXCCPACK][MAXCCPACKASDU];
    uint16_t pAdrChnValue[MAXCCPACK][MAXCCPACKASDU];

    /* 帧信息 */
    PACK_INFO packInfo[MAXCCPACK];

    /* ASDU信息 */
    ASDU_INFO asduInfo[MAXCCPACK][MAXCCPACKASDU];

    /* 总包数 */
    UINT8 allpackno;
} SMV_92_STRUCT;

/* globals */

extern SMVDATA sSmvData[SMV_9_1_CHANNUM];
extern SMV_92_STRUCT SmvStruct[SMV_9_1_CHANNUM];
extern SMV_CC_INFO sSmvCCInfo;

/* 改为与中断缓冲一致 */
extern INT32 send_data[MAXQSIZESAMPDATA+1][MAXCHNELS];
extern uint32_t send_data_sts[MAXQSIZESAMPDATA+1][MAXCHNELS];
extern UINT16 poIec_index;
extern UINT16 synout_index;

/* 功能分类号名称 */
extern uint8_t ucFuncNameArr[][TEMP_INFO_MAX_LEN];

/* functions */

/* 获取物理通道对应网络端口号(本级CC板端口号和前级CC板端口号).
 * Para:
 *     pSmvIn,smv通道指针.
 *     pSlvPortNum,前级CC板端口号.
 *     arrSVID,svID保存数组.
 * Return:
 *     本级CC板端口号.
 */
extern uint8_t cfgGetPortNo(void *pSmvIn, uint8_t *pSlvPortNum, uint8_t *arrSVID);

/* 获取物理通道对应smv配置通道.
 * Para:
 *     ucModCh,内序.
 * Return:
 *     smv配置通道指针.
 */
extern void *cfgGetAsdu(uint8_t ucModCh);

/* SMV采样初始化.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, FALSE.
 */
extern BOOL GetSampDataInit (void);

/* 初始化计数.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
extern void InitCnt(void);

/* 获取SMV接收模式.
 * Para:
 *     NONE.
 * Return:
 *     0: 点对点; 1: 单网; 2: 双网.
 */
extern int32_t smvGetTransType(void);


/*功能：得到CPU的SV虚端子配置信息  2013-6-5  ZY
  参数：ppRtTotalCfgAddr：供返回SV虚端子的总体配置信息全局变量地址对应的变量的指针。
                      该全局变量地址对应的变量由调用方分配，被调用方填充。
                      该全局变量自身，由被调用方分配和管理
  返回：EP_SUCCESS:操作成功
       其他，操作失败 */
EP_STATUS  SMV_Get_Vt_SV_Term_Cfg(SMV_TOTAL_VT_SV_TERM_CFG   **ppRtTotalCfgAddr);


/*功能：得到CPU虚拟SV虚端子状态信息  2013-6-5 ZY
  参数：pRtTotalSTS：供返回SV虚端子的总体状态信息变量指针。
                  该变量，由调用方分配，被调用方填充
  返回：EP_SUCCESS:操作成功
       其他，操作失败 */
EP_STATUS  SMV_Get_Vt_SV_Term_Sts(SMV_TOTAL_VT_SV_TERM_STS   *pRtTotalSts);

#ifdef	__cplusplus
#if __cplusplus
}
#endif
#endif

#endif

