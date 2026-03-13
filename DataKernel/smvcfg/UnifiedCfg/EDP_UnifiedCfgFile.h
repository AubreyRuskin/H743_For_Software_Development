/**************************************************************************
EDP_UnifiedCfgFile.h

九统一统一配置文件生成SV+GS.XML 文件,并进行zip存储的压缩头文件

Copyright (c) 2015 SNAC(Guodian Nanjing Automation Co., Ltd.)
All Rights Reserved.


History:
Revision     Date          Who            Comment
-----------------------------------------------------------------
V1.00         2015.07.16    kevin         初始版本
***************************************************************************/

#ifndef EDP_UNIFIEDCFGFILE_H
#define EDP_UNIFIEDCFGFILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "EDP_UnifiedCfgDefine.h"


#define EDP_CC_GS_ERR_CALLOC_FAIL 0x00000001     /*CC_GS_ERR分配内存出错*/
#define EDP_CC_GS_ERR_FILE_CREATE_FAIL  0x00000002     /*创建XML文件出错*/
#define EDP_CC_GS_ERR_FILE_SAVE_FAIL  0x00000004     /*保存XML文件出错*/

#define EDP_CC_SV_ERR_FIND_BOARD_FAIL 0x00000001         /*当前CC板找不到相应的SV配置*/
#define EDP_CC_SV_ERR_FILE_CREATE_FAIL  0x00000002     /*创建XML文件出错*/
#define EDP_CC_GS_ERR_FILE_SAVE_FAIL  0x00000004     /*保存XML文件出错*/

#define FPGA_PORT_NUM       12
#define CPU_PORT_NUM        2

#define EDP_CC_PORT_NUM 12
#define EDP_CC_APPID_DEFAULT 0x4050
#define EDP_CC_CONFREV_DEFAULT 0x00000001
#define EDP_CC_TCI_DEFAULT 0x200A

#define CONFIG_SMV_FILE		"/tffs/smv.xml"

/*GS控制块的转发信息，所有CC板转发的源端口、目的端口均在*/
typedef struct	cc_sub_cb_trans
{
    int iIndex;
    int iGcbSrcPortCnt;
    char *pSrcPortList[EDP_MAX_CC_PORT_NUM];       /*所有源端口信息*/
    int iGcbDstPortCnt;
    char *pDstPortList[EDP_MAX_CC_PORT_NUM];       /*所有目的端口信息*/
    struct	cc_sub_cb_trans	*next;
} GS_TRANS_INFO;

typedef struct	cc_sub_pool
{
    int iGcbCnt;
    GSE_SUB_INFO *pGsSubInfo;
    GS_TRANS_INFO *pGsTransInfo;        /*与pGsSubInfo 一一对应,代表转发信息*/
} CC_GS_POOL;

typedef struct cc_sv_sub_asdu_info
{
    /*CC板SV配置接收报文配置中的ASDU的信息*/
    int iID;
    char* pSvID;
    int iChnNum;
    uint32_t ulConfRev;
    int iShift;
    uint32_t ulDelayChan;
    /*非需要导出配置部分*/
    uint32_t *pSelect1;
    uint32_t *pSelect2;
    int iSubCcBoardId;                  /* 通过pIntAddrName转换的接收CC板板件ID */
    int iSubCcPortId;                   /* 通过pIntAddrName转换的接收CC板端口ID */
} CC_SV_SUB_ASDU_INFO;

typedef struct cc_sv_pub_asdu_info
{
    /*CC板SV配置发送报文配置中的ASDU的信息*/
    int iID;
    char* pSvID;
    uint32_t ulSelect1;
    uint32_t ulSelect2;
    uint32_t *pSelect1;
    uint32_t *pSelect2;
    /*非需要导出部分*/
    int iChnNum;
    uint32_t ulDelayChan;
    uint32_t ulConfRev;
    int iSubCcBoardId;                  /* 通过pIntAddrName转换的接收CC板板件ID */
    int iSubCcPortId;                   /* 通过pIntAddrName转换的接收CC板端口ID */
} CC_SV_PUB_ASDU_INFO;

typedef struct cc_sv_sub_info
{
    /*CC板SV配置接收报文配置的信息*/
    int iAsduNum;
    int iPort;
    char ucMacAddr[MAC_BYTES];
    int iSrc;
    uint16_t usTCI;
    uint16_t usAppID;
    int iGmrp;
    int iSubBoard;
    char* pDesc;
    CC_SV_SUB_ASDU_INFO *pCcSvSubAsduInfo;
} CC_SV_SUB_INFO;

typedef struct cc_sv_pub_info
{
    /*CC板SV配置发送报文配置的信息*/
    int iAsduNum;
    int iPort;
    int iNodeAddr;
    char ucMacAddrSrc[MAC_BYTES];
    char ucMacAddrDes[MAC_BYTES];
    uint32_t ulConfRev;
    uint16_t usTCI;
    uint16_t usAppID;
    uint16_t usPubRate;
    char* pDesc;
    CC_SV_PUB_ASDU_INFO *pCcSvPubAsduInfo;
    /*非需要导出信息*/
    int iOutBoardId;
    int iOutPortId;
} CC_SV_PUB_INFO;

typedef struct cc_sv_trans
{
    /*CC板SV配置的信息*/
    /*需要导出配置信息*/
    int iSubNum;
    int iMode;
    int iFuncType;
    int iSourceRate;
    int iSynPulse;
    int iSynPulseEnable;
    int iExtSynMode;
    int iExtSynReverse;
    int iGmrpSendGap;
    int iMaxDelay;
    CC_SV_SUB_INFO *pCcSvSubInfo;
    CC_SV_PUB_INFO *pCcSvPubInfo;
    /*非需要导出信息*/
    BOOL bMasterCc;
    int iBoardId;
    int iPubNum;
    int iSubAsduNum;
} SV_TRANS_INFO;

typedef struct cc_sv_pool
{
    int iSvBoardCnt;                        /*CC板板件数目*/
    SV_TRANS_INFO tSvMasterCcTransInfo;      /*主CC转发信息*/
    SV_TRANS_INFO *pSvSlaverCcTransInfo;    /*子CC转发信息*/
} CC_SV_POOL;

typedef struct
{
    int nPort;
    char sCfgFileName[256];
    int nAppType;
    int nFileType;
    int nAddr;
    BOOL bCfgFlag;
    BOOL bRequestFlag;
    BOOL bCfgFinishFlag;
    BOOL bRebootFlag;
    BOOL bGsSendOnly; /* 为仅发送GOOSE的CC板 */
} FPGA_CFG_FILE_INFO;

typedef struct
{
    int iDataType;
    uint16_t usFpgaCcSvCfgCrc; /*用于判断CC SV配置是否正常运行*/
    uint16_t usFpgaCcGsCfgCrc; /*用于判断CC GS配置是否正常运行*/
} FPGA_CC_CFG_INFO;

extern BOOL g_bCfgChgFlag[2];
extern FPGA_CFG_FILE_INFO sFpgaCCInfo[CPU_PORT_NUM][FPGA_PORT_NUM];
extern CC_SV_POOL g_tCCSvPool;     /*CC板SV配置的转发结构*/

/*
描述: 解析过程层配置信息，内存数据结构转换，用于生成文件
参数:     NONE
返回值: 解析是否成功
 */
extern int EDP_ConfigParseCC();


/*
描述: 解析过程层配置信息，生成文件
参数:     NONE
返回值: 解析是否成功
 */
extern int EDP_CreateCCFile();


/*
描述: 初始化张全的通讯结构
参数:     NONE
返回值: 解析是否成功
 */
extern int EDP_InitCcStrToSendFile();

/*
描述: 生成用于计算CRC的文件
参数: pNode, CCD文件的XML指针.
      pFileName, 文件的路径名.
返回值: 解析是否成功
 */
extern int EDP_CreatCrcFile(mxml_node_t *pNode, char* pFileName);

/*
描述: 释放文件操作的内存
参数:     NONE
返回值:  释放是否成功
 */
extern int EDP_FreeFile();

/*
描述: 解析过程层配置信息，CC转发结构
参数:     NONE
返回值: 解析是否成功
 */
extern int EDP_CreateCCTransStruct();
#ifdef    __cplusplus
}
#endif

#endif
