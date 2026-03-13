/**************************************************************************
EDP_UnifiedCfgDefine.h

九统一统一配置文件定义公共头文件

Copyright (c) 2015 SNAC(Guodian Nanjing Automation Co., Ltd.)
All Rights Reserved.


History:
Revision     Date          Who            Comment
-----------------------------------------------------------------
V1.00         2015.07.16    kevin         初始版本
***************************************************************************/

#ifndef EDP_UNIFIEDCFGDEFINE_H
#define EDP_UNIFIEDCFGDEFINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "smvcfg.h"
#include "filetool.h"
#include "mxml.h"
#include "logLib.h"



#define CFG_STRING_LEN 256 			/*字符串数组长度*/
#define MAC_BYTES 6					/*MAC地址字节数*/
#define MAX_NET_NUM 12				/*最大网络数，单网双网等*/
#define EDP_MAX_CC_PORT_NUM 128			/*CC板最大端口数*/

#define PORT_STRING_LEN 8                  /*内部private 中PORT的src填写的最大长度:12-255*/

/*文件路径*/
#define CCD_FILE EP_61850_CFG_DIR"/configured.ccd"      /*CCD文件*/
#define PRIVATECFG_FILE EP_61850_CFG_DIR"/devcfg.xml"      /*私有配置文件*/
#define PROCESS_CRC_FILE EP_61850_CFG_DIR"/process.crc"      /*过程层配置文件CRC存储*/
#define CRC_FILE EP_61850_CFG_DIR"/crc.txt"      /*过程层配置文件CRC存储*/

/*GOOSE SUB最大接收逻辑网络数目*/
#define GO_MAX_SUB_LOGIC_NET_NUM 2
#define GO_MAX_NET_PORT_NUM_PER_NODE 16


/*接线方式*/
#define LINE_TYPE_OUT_LINE 0x01     /*外接线方式*/
#define LINE_TYPE_HSB 0x02     /*外接线方式*/


extern BOOL g_bCfgLog;
#define CFG_LOG(f, a1, a2, a3, a4, a5, a6)  if (g_bCfgLog) logMsg(f, a1, a2, a3, a4, a5, a6)
#define CFG_LOG_UTF8(f, a1, a2, a3, a4, a5, a6)  if (g_bCfgLogUtf8) logMsg( f, a1, a2, a3, a4, a5, a6)





/*
短地址的结构
短地址格式是：TYPE.CPUID.INDEX.TDI:N1.BAY:N2.YB:N3.-
TYPE字段:
Sub_Rsv","Pub_Rsv","Sub_Mea","Pub_Mea","Sub_SV","Pub_SV"
Rsv:开入量
Mea:模拟量
*/
typedef struct str_saddr
{
    char *pType;        /*类型,字符串*/
    uint8_t ucCpuId;        /*CPUID*/
    uint32_t usCfgIndex;     /*装置逻辑序号,短地址中从1开始*/
    char *pTDI;                 /*MU使用*/
    char *pBay;                   /*站域的opmode*/
    char *pYbId;                 /*压板ID*/
    BOOL bIsNegative;       /*采样值是否负号*/
} EDP_SADDR_STR;



/*--------------------goose 相关分隔符------------------------------*/
/*<GSEControl>节点信息*/
typedef struct gse_control
{
    char *pAppID;
    char *pDatSet;
    char *pCtrlName;
    char *pCtrlType;
    int iConfRev;
} GSE_CONTROL;

/*<ConnectedAP>下的<GSE>下的<MinTime>和<MaxTime>节点信息*/
typedef struct gse_connectedap_gse_time
{
    char *pMinMultiplier;					/*MinTime multiplier*/
    char *pMaxMultiplier;					/*MaxTime multiplier*/
    char *pMinUnit;						/*MinTime Unit*/
    char *pMaxUnit;						/*MaxTime Unit*/
    int iMinTime;						/*MinTime*/
    int iMaxTime;						/*MaxTime*/
} GSE_AP_GSE_TIME;

/*<ConnectedAP>下的<GSE>下的<Address>节点信息*/
typedef struct gse_connectedap_gse_address
{
    int iIndex;
    char aDstMac[MAC_BYTES];			/*目的MAC地址*/
    uint16_t usVID;						/*VLAN-ID*/
    uint16_t usVPri;						/*VLAN-PRIORITY*/
    uint16_t usAppID;					/*APPID*/
} GSE_AP_GSE_ADDR;

/*<ConnectedAP>下的<GSE>节点信息*/
typedef struct gse_connectedap_gse
{
    char *pCbName;
    char *pLdInst;
    int iAddrCnt;
    GSE_AP_GSE_ADDR pGseAddr[MAX_NET_NUM];
    GSE_AP_GSE_TIME tGseTime;
} GSE_CONNECTAP_GSE;

/*<ConnectedAP>下的<PhysConn>节点信息*/
typedef struct gse_connectedap_physconn
{
    int iIndex;
    char *pPhysType;					/*PhysConn type*/
    char *pPort;						/*<P type="Port">2-A</P>*/
    char *pPlug;						/*<P type="Plug">RJ45</P>*/
    char *pCable;						/*<P type="Cable">TW</P>*/
    char *pSpeedType;					/*<P type="Type">100BaseT</P>*/
} GSE_CONNECTAP_PHYSCONN;

/*pub的<ConnectedAP>节点信息*/
typedef struct gse_pub_connectedap
{
    char *pApName;
    char *pApIedName;
    GSE_CONNECTAP_GSE tConnectApGse;
    int iPhysConnCnt;
    GSE_CONNECTAP_PHYSCONN tConnectApPhysConn[EDP_MAX_CC_PORT_NUM];
} GSE_PUB_CONNECTAP;

/*sub的<ConnectedAP>节点信息*/
typedef struct gse_sub_connectedap
{
    char *pApName;
    char *pApIedName;
    GSE_CONNECTAP_GSE tConnectApGse;
} GSE_SUB_CONNECTAP;

/*<DataSet>下的<FCDA>下的或者<intAddr>下的<DAI>节点信息*/
typedef struct gse_dateset_fcda_dai
{
    char *pDaiName;
    char *pSAddr;
    EDP_SADDR_STR tSAddr;
} GSE_DATESET_FCDA_DAI;

/*GOOSE SUB下的<DataSet>下的<FCDA>下的<intAddr>节点信息*/
typedef struct gse_dateset_fcda_intaddr
{
    int iIndex;
    char *pIntaddrName;
    char *pIntaddrDesc;
    BOOL bIsIntAddrValid;			/*FALSE无效，TRUE有效*/
    BOOL bIsDaiValid;				/*FALSE无效，TRUE有效*/
    GSE_DATESET_FCDA_DAI tDai;
} GSE_DATESET_FCDA_INTADDR;

/*pub 的<DataSet>下的<FCDA>节点信息*/
typedef struct gse_dateset_fcda_pub
{
    int iIndex;
    char *pBType;
    char *pDaName;
    char *pDoName;
    char *pFC;
    char *pLdInst;
    char *pLnClass;
    char *pLnInst;
    char *pPrefix;
    char *pFcdaDesc;
    BOOL bIsDaiValid;				/*FALSE无效，TRUE有效*/
    GSE_DATESET_FCDA_DAI tDai;
    struct gse_dateset_fcda_pub *next;
} GSE_DATESET_FCDA_PUB;

/*sub 的<DataSet>下的<FCDA>节点信息*/
typedef struct gse_dateset_fcda_sub
{
    int iIndex;
    char *pBType;
    char *pDaName;
    char *pDoName;
    char *pFC;
    char *pLdInst;
    char *pLnClass;
    char *pLnInst;
    char *pPrefix;
    char *pFcdaDesc;
    int iIntAddrCnt;
    GSE_DATESET_FCDA_INTADDR *pIntAddr;
    struct gse_dateset_fcda_sub *next;
} GSE_DATESET_FCDA_SUB;

/*<DataSet>pub节点信息*/
typedef struct gse_pubdateset
{
    char *pDateSetName;
    int iFcdaCnt;
    GSE_DATESET_FCDA_PUB *pDataSetFcda;
} GSE_PUB_DATESET;

/*<DataSet>sub节点信息*/
typedef struct gse_subdateset
{
    char *pDateSetName;
    int iFcdaCnt;
    GSE_DATESET_FCDA_SUB *pDataSetFcda;
} GSE_SUB_DATESET;
/*--------------------goose 相关分隔符End------------------------------*/

/*--------------------smv相关分隔符------------------------------*/
/*<SampledValueControl>下的<SmvOpts>节点信息*/
typedef struct smv_control_opts
{
    char *pDataRef;
    char *pRefreshTime;
    char *pSampleRate;
    char *pSampleSyn;
    char *pSecurity;
} SMV_CONTROL_OPTS;

/*<SampledValueControl>节点信息*/
typedef struct smv_control
{
    int iConfRev;
    char *pDatSet;
    char *pMultiCast;
    char *pSvCtrlName;
    int iAsduCnt;
    int iSmpRate;
    char *pSvID;
    SMV_CONTROL_OPTS tSvCtrlOpts;
} SMV_CONTROL;

/*<ConnectedAP>下的<SMV>下的<Address>节点信息*/
typedef struct smv_connectedap_smv_address
{
    int iIndex;
    char aDstMac[MAC_BYTES];			/*目的MAC地址*/
    uint16_t usVID;						/*VLAN-ID*/
    uint16_t usVPri;						/*VLAN-PRIORITY*/
    uint16_t usAppID;					/*APPID*/
} SMV_AP_SMV_ADDR;

/*<ConnectedAP>下的<smv>节点信息*/
typedef struct smv_connectedap_smv
{
    char *pCbName;
    char *pLdInst;
    int iAddrCnt;
    SMV_AP_SMV_ADDR pSmvAddr[MAX_NET_NUM];
} SMV_CONNECTAP_SMV;

/*<ConnectedAP>下的<PhysConn>节点信息*/
typedef struct smv_connectedap_physconn
{
    int iIndex;
    char *pPhysType;					/*PhysConn type*/
    char *pPort;						/*<P type="Port">2-A</P>*/
    char *pPlug;						/*<P type="Plug">RJ45</P>*/
    char *pCable;						/*<P type="Cable">TW</P>*/
    char *pSpeedType;					/*<P type="Type">100BaseT</P>*/
} SMV_CONNECTAP_PHYSCONN;

/*smv pub的<ConnectedAP>节点信息*/
typedef struct smv_pub_connectedap
{
    char *pApName;
    char *pApIedName;
    SMV_CONNECTAP_SMV tConnectApSmv;
    int iPhysConnCnt;
    SMV_CONNECTAP_PHYSCONN tConnectApPhysConn[EDP_MAX_CC_PORT_NUM];
} SMV_PUB_CONNECTAP;

/*sub的<ConnectedAP>节点信息*/
typedef struct smv_sub_connectedap
{
    char *pApName;
    char *pApIedName;
    SMV_CONNECTAP_SMV tConnectApSmv;
} SMV_SUB_CONNECTAP;

/*<DataSet>下的<FCDA>下的或者<intAddr>下的<DAI>节点信息*/
typedef struct smv_dateset_fcda_dai
{
    char *pDaiName;
    char *pSAddr;
} SMV_DATESET_FCDA_DAI;

/*<DataSet>下的<FCDA>下的<intAddr>节点信息*/
typedef struct smv_dateset_fcda_intaddr
{
    int iIndex;
    BOOL bIsIntAddrValid;			/*FALSE无效，TRUE有效*/
    BOOL bIsDaiValid;				/*FALSE无效，TRUE有效*/
    char *pIntaddrName;
    char *pDesc;
    SMV_DATESET_FCDA_DAI tDai;
    struct smv_dateset_fcda_intaddr *next;
} SMV_DATESET_FCDA_INTADDR;

/*pub 的<DataSet>下的<FCDA>节点信息*/
typedef struct smv_dateset_fcda_pub
{
    int iIndex;
    char *pBType;
    char *pDaName;
    char *pDoName;
    char *pFC;
    char *pLdInst;
    char *pLnClass;
    char *pLnInst;
    char *pPrefix;
    SMV_DATESET_FCDA_DAI tDai;
    struct smv_dateset_fcda_pub *next;
} SMV_DATESET_FCDA_PUB;

/*sub 的<DataSet>下的<FCDA>节点信息*/
typedef struct smv_dateset_fcda_sub
{
    int iIndex;
    char *pBType;
    char *pDaName;
    char *pDoName;
    char *pFC;
    char *pLdInst;
    char *pLnClass;
    char *pLnInst;
    char *pPrefix;
    char *pFcdaDesc;
    int iIntAddrCnt;
    SMV_DATESET_FCDA_INTADDR *pIntAddr;
    struct smv_dateset_fcda_sub *next;
} SMV_DATESET_FCDA_SUB;

/*<DataSet>pub节点信息*/
typedef struct smv_pubdateset
{
    char *pDateSetName;
    int iFcdaCnt;
    SMV_DATESET_FCDA_PUB *pDataSetFcda;
} SMV_PUB_DATESET;

/*<DataSet>sub节点信息*/
typedef struct smv_subdateset
{
    char *pDateSetName;
    int iFcdaCnt;
    SMV_DATESET_FCDA_SUB *pDataSetFcda;
} SMV_SUB_DATESET;
/*--------------------smv相关分隔符End------------------------------*/

/*--------------------分隔符------------------------------*/
/*PUB GCB块的结构*/
typedef struct process_pub_gcb_str
{
    int iIndex;
    /*int iGcbIndex;*/
    char *pGOCBrefName;
    int iPubGcbCpuId;         /*代表该发送控制块中的SADDR中的CPUID*/
    GSE_CONTROL tGseCtrl;						/*<GSEControl>节点信息*/
    GSE_PUB_CONNECTAP tConnectAp;			/*<ConnectedAP>节点信息*/
    GSE_PUB_DATESET tDataSet;					/*<DataSet>节点信息*/
    struct process_pub_gcb_str *next;
} PUB_GCB_INFO;

/*SUB GCB块的结构*/
typedef struct process_sub_gcb_str
{
    int iIndex;
    char *pGOCBrefName;
    char *pIntAddrName;                 /*本控制块的接收信息. 以冒号进行区分*/
    GSE_CONTROL tGseCtrl;						/*<GSEControl>节点信息*/
    GSE_SUB_CONNECTAP tConnectAp;			/*<ConnectedAP>节点信息*/
    GSE_SUB_DATESET tDataSet;				/*<DataSet>节点信息*/
    struct process_sub_gcb_str *next;
} SUB_GCB_INFO;

/*PUB SMV间隔的结构*/
typedef struct process_pub_sv_str
{
    int iIndex;
    char *pSVCBrefName;
    SMV_CONTROL tSvCtrl;
    SMV_PUB_CONNECTAP tConnectAp;
    SMV_PUB_DATESET tDataSet;
    struct process_pub_sv_str *next;
} PUB_SMV_INFO;

/*SUB SMV 间隔的结构*/
typedef struct process_sub_sv_str
{
    int iIndex;							/*总的索引*/
    int iIndexInHsbAddr;				/*在该地址中的索引*/
    int iHSBAddr;						/*该SMV所在的总线地址，由dataset中的intaddr中的name解析而来*/
    BOOL bIsDoubleNet;				/*该SMV是否双网*/
    int iPort[MAX_NET_NUM];				/*该SMV所在的总线地址的接收端口号，由dataset中的intaddr中的name解析而来*/
    char cAddrPort[MAX_NET_NUM][PORT_STRING_LEN];	/*保存类似1-1的地址和端口信息，主要用于和PRIVATE中的PORT信息进行判断用*/
    uint32_t ulSelect1[MAX_NET_NUM];					/*根据FCDA中是否存在sAddr 来得到配置时的0-31 select*/
    uint32_t ulSelect2[MAX_NET_NUM];					/*根据FCDA中是否存在sAddr 来得到配置时的32-48 select*/
    int iUsedIntaddrCnt;				/*该ASDU被选中的intaddr个数，用于导出SMV通道信息中的asduchn时用,如果1个FCDA中有两个intaddr，则算两个。先不考虑1个intaddr中的双网*/
    int iUsedFcdaCnt;					/*该ASDU被选中的通道个数，代表被选中的FCDA个数*/
    char *pSVCBrefName;
    char *pIntAddrName;                 /*本控制块的接收信息. 以冒号进行区分*/
    int iSubCcBoardId;                  /* 通过pIntAddrName转换的接收CC板板件ID */
    int iSubCcPortId;                   /* 通过pIntAddrName转换的接收CC板端口ID */
    int iDelayChnNo;                    /* 延时通道序号 */
    SMV_CONTROL tSvCtrl;
    SMV_SUB_CONNECTAP tConnectAp;
    SMV_SUB_DATESET tDataSet;
    struct process_sub_sv_str *next;
} SUB_SMV_INFO;

/*--------------------分隔符------------------------------*/
/*process.xml中的pub goose 结构*/
typedef struct process_pub_goose_str
{
    int iPubGsCnt;
    PUB_GCB_INFO *pPubGcbInfo;
} PROCESS_PUB_GSE;

/*process.xml中的sub goose 结构*/
typedef struct process_sub_goose_str
{
    int iSubGsCnt;
    SUB_GCB_INFO *pSubGcbInfo;
} PROCESS_SUB_GSE;

/*process.xml中的pub smv 结构*/
typedef struct process_pub_smv_str
{
    int iPubSmvCnt;
    PUB_SMV_INFO *pPubSmvInfo;
} PROCESS_PUB_SMV;

/*process.xml中的sub smv 结构*/
typedef struct process_sub_smv_str
{
    int iSubSmvCnt;
    SUB_SMV_INFO *pSubSmvInfo;
} PROCESS_SUB_SMV;

/*process.xml中的private 结构*/
typedef struct process_private_str
{
    uint32_t ulCrc;
    char *pTimeStamp;
} PROCESS_PRIVATE;

/*--------------------process.xml的总结构------------------------------*/

/*process.xml的结构*/
typedef struct process_str
{
    char *pIEDName; 	 				/* IED名称 */
    char *pIEDDes; 	 					/* IED描述 */
    char *pCfgVersion; 	 				/* 版本号 */
    char *pManufacturer; 	 			/* 制造厂 */
    char *pDevType; 	 				/* 装置型号 */
    PROCESS_PUB_GSE tProPubGoose;
    PROCESS_SUB_GSE tProSubGoose;
    PROCESS_PUB_SMV tProPubSmv;
    PROCESS_SUB_SMV tProSubSmv;
    PROCESS_PRIVATE tProPrivate;
} PROCESS_CFG;

/*--------------------------------分隔符------------------------------------------*/
/*<Board>下的<ConnectPort>节点结构*/
typedef struct private_board_port_connect_str
{
    int iIndex;     /*内部索引,从0开始*/
    int iPortId;        /*本板端口号,属于级联口从A开始,从0开始*/
    int iDataType;      /*转发的数据类型*/
    char *pOutPortId;         /*对端的板号-端口号*/
    int iOutBoardId;        /*对端的板号,由pOutPortId解析出来*/
    int iOutPortId;             /*对端的端口号,由pOutPortId解析出来,转换为从0开始*/
    char aSvMacAddr[MAC_BYTES];      /*sv数据的mac地址*/
    uint16_t usAppID;               /*sv数据的APPID*/
    int iSvPubRate;                         /*sv数据的发送速率*/
    int iSvType;                    /*smv.xml的Type项*/
    int iSvForceSyn;                /*smv.xml的forceSyn项,是否强制同步*/
    int iGsParentCpuType;               /*该级联口转发goose的模式*/

    char *pGsCbIndex;               /*GS 转发控制块字符串*/
    int iGsCbCnt;                       /*GS转发控制块个数*/
    int iGsCbList[256];                       /*GS转发控制块列表*/
} BOARD_PORT_CONNECT;

/*<Board>下的<ConnectPort>节点结构*/
typedef struct private_board_port_str
{
    int iConnectCnt;
    BOARD_PORT_CONNECT *pPortConnect;
} BOARD_PORT;

/*--------------------------------分隔符------------------------------------------*/
/*<Board>下的<smv>下的<SubSmv>下的<Channel>节点结构*/
typedef struct private_board_sub_smv_channel_str
{
    int iIndex;
    int iInputNo;                        /*编号    十进制*/
    char *pHwRef;                        /*输入索引*/
    int iChannelNo;                    /*输入序号    十进制*/
    char *pChannelType;                /*数据类型*/
    float fCoeff;                        /*修正系数    十进制*/
    int iPhase;                            /*相位补偿（分）    十进制*/
    char *pAdjust;                        /*温度补偿*/
    char *pDeal;                        /*处理方式*/
    char *pChannelDesc;                /*面板显示描述*/

    int iChannelNo2;                    /*通道2    十进制*/
    int iBusNo;                            /*母线号    十进制*/
    struct private_board_sub_smv_channel_str *next;
} BOARD_SUBSMV_CHANNEL;

/*<Board>下的<smv>下的<PubSmv>下的<Channel>节点结构*/
typedef struct private_board_pub_smv_channel_str
{
    int iIndex;
    struct private_board_pub_smv_channel_str *next;
} BOARD_PUBSMV_CHANNEL;

/*<Board>下的<smv>下的<SubSmv>节点结构*/
typedef struct private_board_sub_smv_str
{
    int iIndex;
    int iSvSubIndex;                   /*用于该SUBSMV对应于pprocess中的哪一个接收块，用于双cpu,24点和96点区分用*/
    char *pSubSmvType;                /*SMV类型*/
    int iSmpRate;                        /*周波采样点数    十进制*/
    int iDstRate;                        /*目标周波点数*/
    int iDstAddr;                        /*目的转发地址    十进制*/
    /*int iAsduNo;*/                        /*ASDU个数*/
    int iPhase;                            /*相位调节    十进制*/
    int iRcdly;                            /*RC滤波延迟时间    十进制*/
    int iPort;                            /*输入端口号    十进制*/
    int iMode;                            /*接收类型    十进制数*/
    int iDlyChn;                        /*9-2延时通道    十进制*/
    int iDlyVlanID;                        /*延时是否有效标识    十进制*/
    int iDlyTime;                        /*延时时间设定    十进制*/
    /*int iSynPulse;*/                        /*同步信号发送与否    十进制*/
    /*int iForceSyn;*/                        /*强制同步信号*/
    /*int iForceTest;    */                    /*强制测试信号*/

    int iChannelCnt;
    BOARD_SUBSMV_CHANNEL *pChannel;
    struct private_board_sub_smv_str *next;
} BOARD_SUBSMV;

/*<Board>下的<smv>下的<PubSmv>节点结构*/
typedef struct private_board_pub_smv_str
{
    int iIndex;

    char *pPubSmvType;                /*SMV类型*/
    int iSmpRate;                        /*周波采样点数    十进制*/
    int iPort;                            /*输入端口号    十进制*/
    int iForceSyn;                        /*强制同步    十进制*/
    char *pSrcMac;                        /*源MAC地址    字符串*/
    char *pDstMac;                        /*目的MAC地址    字符串*/

    int iRtdPhsCur;                        /*额定相电流十进制*/
    int iRtdNeuCur;                        /*额定零序电流十进制*/
    int iRtdPhsVol;                        /*额定电压十进制*/
    int iRtdDlyTime;                    /*额定延时十进制*/

    int iChannelCnt;
    BOARD_PUBSMV_CHANNEL *pChannel;
    struct private_board_pub_smv_str *next;
} BOARD_PUBSMV;

/*<Board>下的<smv>节点的属性结构*/
typedef struct private_board_smv_common_str
{
    /*SV.XML使用的*/
    int iMode;                            /* 装置运行模式 */
    int iFuncType;                        /*功能类型，代表进出各多少点*/
    int iSourceRate;                        /*采样点数*/
    int iSynPulse;                        /* 强制同步 */
    int iSynPulseEnable;
    int iExtSynMode;                      /**/
    int iExtSynReverse;                   /**/
    int iGmrpSendGap;                     /**/
    int iGmrp;                            /*GMRP功能是否使用*/
    int iMaxDelay;                        /* 最大延时时间 */
    /*SMV.XML使用的*/
    int iSynMode;                        /*对时模式    十进制*/
    int iSynOutAddr;                    /*该地址的对时信号是否输出到总线*/
    char *pSynSrc;                        /*同步源    字符串*/
    int iSynRev;                        /*对时脉冲 是否取反    十进制*/
    int iSynEdge;                        /*脉冲边沿对时    十进制*/
    int iSynDeal;                        /*处理方式    十进制*/
    int iIntelPulse;                        /*同步模式    十进制*/
    int iIntelOutAddr;                    /*该地址的同步信号是否输出到总线*/
    int iForceSyn;                        /*强制同步    十进制*/
    int iForceTest;                        /*强制测试    十进制*/
    int iRtdDlyTime;                    /*延时时间设定    十进制*/
    int iPuncMOD;                        /*守时进入模式    十进制*/
    int iPuncTime;                        /*守时时间    十进制*/
} BOARD_SMV_COMMON;

/*<Board>下的<smv>节点结构*/
typedef struct private_board_smv_str
{
    int iSubSmvCnt;
    int iPubSmvCnt;
    BOARD_SUBSMV *pSubSmv;
    BOARD_PUBSMV *pPubSmv;
    BOARD_SMV_COMMON tSmvCommon;
} BOARD_SMV;

/*--------------------------------分隔符------------------------------------------*/
/*<Board>下的<goose>下的<SubGoose>下的<Channel>节点结构*/
typedef struct private_board_sub_goose_channel_str
{
    int iIndex;
    struct private_board_sub_goose_channel_str *next;
} BOARD_SUBGOOSE_CHANNEL;

/*<Board>下的<goose>下的<PubGoose>下的<Channel>节点结构*/
typedef struct private_board_pub_goose_channel_str
{
    int iIndex;
    struct private_board_pub_goose_channel_str *next;
} BOARD_PUBGOOSE_CHANNEL;

/*<Board>下的<goose>下的<SubGoose>节点结构*/
typedef struct private_board_sub_goose_str
{
    int iChannelCnt;
    BOARD_SUBGOOSE_CHANNEL *pChannel;
    struct private_board_sub_goose_str *next;
} BOARD_SUBGOOSE;

/*<Board>下的<goose>下的<PubGoose>节点结构*/
typedef struct private_board_pub_goose_str
{
    int iChannelCnt;
    BOARD_PUBGOOSE_CHANNEL *pChannel;
    struct private_board_pub_goose_str *next;
} BOARD_PUBGOOSE;

/*<Board>下的<goose>节点结构*/
typedef struct private_board_goose_str
{
    int iSubGseCnt;
    int iPubGseCnt;
    BOARD_SUBGOOSE *pSubGse;
    BOARD_PUBGOOSE *pPubGse;
} BOARD_GOOSE;
/*
id	UINT16	[0,1]	板号
type	UINT16	1	板类型
desc	VSTRING	1	板描述
parentCCId	UINT16	1	父板板号
subDevId	UINT16	[0,1]	子机号
nodeAddr	UINT16	[0,1]	内部地址
*/
/*<Board>节点结构*/
typedef struct private_board_str
{
    int iIndex;                /*板子序号,内部编排，从0开始*/
    int iBoardId;               /*板号,该板号与模型中的PhysConn的板号一致，是对外体现的板号，与装置实际内部地址无关，该板号在某一装置中唯一，从1开始定义*/
    int iBoardType;         /*板类型，1：普通CPU板，2：CC板，3：冗余从CPU板。*/
    char *pBoardDesc;       /*板描述*/
    int iParentCCId;         /*父板板号,父CC板板号，表明该板与其他CC板的关系，如果该板是子板CC，则该字段表明该子板级联的父板板号，如果该板是主板CC或者CPU板，则该字段与本板板号一致*/
    int iSubDevId;              /*子机号,外接线模式下，该字段无效,HSB模式下有效*/
    int iNodeAddr;        /*总线地址,外接线模式下,该字段为CPU号,1为主,2为从*/
    int iGsPacketFlow;              /*GOOSE风暴过滤 报文数目门槛设置*/
    int iGsDataFlow;                /*GOOSE风暴过滤 报文数据流量门槛设置*/
    BOARD_PORT tPort;
    BOARD_SMV tSmv;
    BOARD_GOOSE tGoose;
    struct private_board_str *next;
} PRIVATE_BOARD;

/*private.xml的结构*/
typedef struct private_str
{
    int iDeviceType;                /*装置类型*/
    int iConnectLineType;       /*连接方式类型*/
    int iBoardCnt;                    /*板子数目*/
    PRIVATE_BOARD *pBoard;
} PRIVATE_CFG;

/*<-------------------------转发结构--------------------------->*/
/*单cc转发结构*/
typedef struct cc_board_transfor_info
{
    int iBoardId;                   /*板号,该板号与模型中的PhysConn的板号一致，是对外体现的板号，与装置实际内部地址无关，该板号在某一装置中唯一，从1开始定义*/
    int iDataType;                /*板件的转发类型*/
    char pGsFilePath[256];        /*生成的Gs 文件路径*/
    char pSvFilePath[256];        /*生成的Sv 文件路径*/
    char pZipFilePath[256];        /*生成的Sv 文件路径*/

    BOOL bIsMasterCC;           /*是否主CC，主CC直接是从0口接收CPU信息,子CC总主CC地址的端口进行接收*/
    /*该CC板接收的配置信息从对侧哪个端口过来。
        前提是最多只有两层级联关系
        且CC板只从 0口接收配置文件
        分两种情况，
        如果是主CC,则ucGsSrcPort[0]表示是从主CPU的那个口接收文件.
        如果是子CC,则ucGsSrcPort[0]表示是从主CPU的那个口接收文件.
        ucGsSrcPort[1]表示是从主CC的那个口转发的该文件*/
    uint8_t ucSrcPort[2];

    struct cc_board_transfor_info *next;
} CC_TRANSMIT_STR;

/*单cc转发结构*/
typedef struct all_cc_board_transfor_info
{
    uint8_t ucCCCnt;
    CC_TRANSMIT_STR *pCcTransmitStr;
} CC_TRANSMIT_INFO;


extern PROCESS_CFG g_tProcessBusCfg;		/*过程层配置结构*/
extern PRIVATE_CFG g_tPrivateCfg;           /*过程层私有配置结构*/

extern uint8_t g_ucLineType;        /*接线方式*/


#ifdef    __cplusplus
}
#endif

#endif
