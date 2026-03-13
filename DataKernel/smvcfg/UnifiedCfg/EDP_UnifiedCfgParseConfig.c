/**************************************************************************
EDP_UnifiedCfgParseConfig.c

九统一配置信息解析
将CCD PRIVATE生成GSE.XML  SMV.XML在内存中的结构

Copyright (c) 2014 SNAC(Guodian Nanjing Automation Co., Ltd.)
All Rights Reserved.

History:
Revision     Date          Who            Comment
-----------------------------------------------------------------
V1.00         2015.03.03    kevin         初始版本
***************************************************************************/


#include "EDP_UnifiedCfgParseConfig.h"
#include "EDP_UnifiedCfgParseCCD.h"
#include "EDP_UnifiedCfgParsePrivate.h"
#include "EDP_UnifiedCfgMain.h"
#include "iecgoose.h"
#include "smvcfg.h"


#define SADDR_POINT_CNT 7     /*SADDR字段中的.的个数*/
#define GOOSE_TYPE 1     /*根据ldinst 名称来判断是否是GOOSE的过程层块类型*/
#define SMV_TYPE 2      /*根据ldinst 名称来判断是否是SMV的过程层块类型*/

#define SMV_VOL_VAL_OUT 11585           /*电压的SmvValOut值*/
#define SMV_PROTECT_AMP_VAL_OUT 463     /*保护电流的SmvValOut值*/
#define SMV_MEASURE_AMP_VAL_OUT 11585   /*测量电流的SmvValOut值*/

char* g_sGDevice[] = {"PI","GOLD","RPIT","MU"};     //过程层GOOSE LDINST名字不增加PIGO 是因为判的时候判是否包含PI,判GOOSE时判PI且不等于PISV
char* g_sVDevice[] = {"SVLD","PISV","MUSV"};        //过程层SV LDINST名字

/*接收和发送的GOOSE结构，用于生成GSE文件*/
GSE_SUB_POOL g_tSubGoose;
GSE_PUB_POOL g_tPubGoose;

/*接收SMV的信息，用于生成GSE文件*/
IEC_SMV_CFG *g_pSmvCfg;

/*CPU板号，对于外接线而言1表示主CPU;2表示从CPU*/
uint8_t g_ucCpuBoardId = 0;

/*过程层中本插件的接收GOOSE个数,去掉了站控层的GOOSE接收数目*/
uint16_t g_usProcessSubGooseNum = 0;
/*过程层中本插件的发送GOOSE个数,去掉了站控层的GOOSE发送数目*/
uint16_t g_usProcessPubGooseNum = 0;
/*过程层中本插件的接收SV个数,去掉了站控层的SV接收数目*/
uint16_t g_usProcessSubSvNum = 0;
/*过程层中本插件的发送SV个数,去掉了站控层的SV发送数目*/
uint16_t g_usProcessPubSvNum = 0;

/*过程层中本插件的接收GOOSE个数,去掉了站控层的GOOSE接收数目*/
uint16_t g_usProcessSubGooseNumAll = 0;
/*过程层中本插件的发送GOOSE个数,去掉了站控层的GOOSE发送数目*/
uint16_t g_usProcessPubGooseNumAll = 0;
/*过程层中本插件的接收SV个数,去掉了站控层的SV接收数目*/
uint16_t g_usProcessSubSvNumAll = 0;
/*过程层中本插件的发送SV个数,去掉了站控层的SV发送数目*/
uint16_t g_usProcessPubSvNumAll = 0;
/*接收和发送的GOOSE结构，用于生成GS文件*/
GSE_SUB_POOL g_tSubGooseAll;
GSE_PUB_POOL g_tPubGooseAll;
EDP_SADDR_STR *g_ptSubSAddr;    /*所有SUB GCB的SADDR*/
EDP_SADDR_STR *g_ptPubSAddr;    /*所有PUB GCB的SADDR*/

/*本板CPU的CPUID ,外接线只有1,2，根据SPI的主从来区分*/
uint8_t g_ucCpuId = 0;
BOOL g_bRedunCpu = FALSE;

/*本CPU板DATAMAP中的需关联的开入开出个数*/
uint32_t g_ulGooseSubMapCnt = 0;
uint32_t g_ulGoosePubMapCnt = 0;


extern SUB_MAP_INFO *NewSubMapInfoNode();

/*
获取枚举项条目字串
参数：    pcStr,枚举项字串
        lIndex,字串序号（0开始）
        pcItemStr,条目字串存储指针
        pKey,用于分割的字符
        usItemStrMaxLen,条目字符串最大长度
返回值：    条目字串长度
修改:  1.张全 20170111 增加参数传递pcItemStr指向数组的长度，防止操作中指针越界
*/
int GetEnumItem_Div(char *pcStr,int32_t lIndex,char *pcItemStr, char Key, uint16_t usItemStrMaxLen)
{
    char ch;
    char *startptr=pcItemStr;
    uint16_t usItemStrRestLen = usItemStrMaxLen;/* 条目字符串剩余的长度 */

    if((pcStr==NULL) || (pcItemStr==NULL))
    {
        return 0;
    }

    while(lIndex>0)
    {
        ch=*pcStr++;
        if(ch==Key)
            lIndex--;
        else if(ch=='\0')
        {
            *pcItemStr='\0';
            return 0;
        }
    }

    while(1)
    {
        ch=*pcStr++;
        if((ch==Key)||(ch=='\0'))
        {
            break;
        }
        if(usItemStrRestLen == 1)
        {
            *pcItemStr='\0';
            return 0;
        }
        *pcItemStr++=ch;
        usItemStrRestLen--;
    }

    *pcItemStr='\0';
    return pcItemStr-startptr;
}

/*
获取枚举项条目字串,获取分割字符前面所有的字符串
参数：    pcStr,枚举项字串
        lIndex,字串序号（0开始）
        pcItemStr,条目字串存储指针
        pKey,用于分割的字符
        usItemStrMaxLen,条目字符串最大长度
返回值：    条目字串长度
修改:  1.张全 20170111 增加参数传递pcItemStr指向数组的长度，防止操作中指针越界
*/
int GetEnumItem_Div_All(char *pcStr,int32_t lIndex,char *pcItemStr, char Key, uint16_t usItemStrMaxLen)
{
    char ch;
    char *startptr=pcItemStr;
    uint16_t usItemStrRestLen = usItemStrMaxLen;/* 条目字符串剩余的长度 */

    if((pcStr==NULL) || (pcItemStr==NULL))
    {
        return 0;
    }

    while(1)
    {
        ch=*pcStr++;
        if(ch==Key)
        {
            if(lIndex>0)
            {
                lIndex--;
            }
            else
            {
                break;
            }
        }
        if(ch=='\0')
        {
            break;
        }
        if(usItemStrRestLen == 1)
        {
            *pcItemStr='\0';
            return 0;
        }
        *pcItemStr++=ch;
        usItemStrRestLen--;
    }

    *pcItemStr='\0';
    return pcItemStr-startptr;
}

/*******************************************************
GetDataValTypebyStr()
描述:根据字符串描述获取数据类型
参数:str 类型字符串
返回值:类型定义
********************************************************/
static VALUETYPE GetDataValTypebyStr(char *str)
{
    if(strcmp(str,"BOOLEAN")==0)
        return BOOL_TYPE;
    else if(strcmp(str,"Dbpos")==0)
        return DPC_TYPE;
    else if(strcmp(str,"INT32")==0)
        return INT_TYPE;
    else if(strcmp(str,"INT32U")==0)
        return UINT_TYPE;
    else if(strcmp(str,"FLOAT32")==0)
        return FLOAT_TYPE;
    else if(strcmp(str,"Quality")==0)
        return QUALITY_TYPE;
    else if(strcmp(str,"Timestamp")==0)
        return UTCTIME_TYPE;
    else
        return UNKNOW_TYPE;
}

/*
描述:解析SADDR
短地址格式是：TYPE.CPUID.INDEX.TDI:N1.BAY:N2.YB:N3.-
参数:
pSAddrString: SADDR的字符串
ptSAddr: 返回的结构
返回值: 解析是否成功
 */
BOOL EDP_ConfigParseSAddr(char *pSAddrString, EDP_SADDR_STR *ptSAddr)
{
    BOOL res = TRUE;
    char strtmp[32];
    char strtmp2[32];
    char strtmp3[128];
    int i = 0;

    if(pSAddrString == NULL || ptSAddr == NULL)
    {
        return FALSE;
    }

    ptSAddr->pTDI = NULL;
    ptSAddr->pBay = NULL;
    ptSAddr->pYbId = NULL;

    for(i = 0; i < SADDR_POINT_CNT; i++)
    {
        if(!(GetEnumItem_Div(pSAddrString,i,strtmp,'.',sizeof(strtmp)) > 0))
        {
            break;
        }
        if(i < 3)
        {
            switch(i)
            {
                case 0:
                    /*TYPE*/
                    ptSAddr->pType = strdup(strtmp);
                    break;
                case 1:
                    /*CPUID*/
                    ptSAddr->ucCpuId = strtol(strtmp,NULL,10);
                    break;
                case 2:
                    /*INDEX*/
                    ptSAddr->usCfgIndex = strtol(strtmp,NULL,16);
                    break;
            }
        }
        else
        {
            if(strtmp[0] == '-')
            {
                ptSAddr->bIsNegative = TRUE;
            }
            else
            {
                if(GetEnumItem_Div(strtmp,0,strtmp2,':',sizeof(strtmp2)) > 0)
                {
                    if(GetEnumItem_Div(strtmp,1,strtmp3,':',sizeof(strtmp3)) > 0)
                    {
                        if(strcmp(strtmp2,"TDI") == 0)
                        {
                            ptSAddr->pTDI = strdup(strtmp3);
                        }
                        else if(strcmp(strtmp2,"BAY") == 0)
                        {
                            ptSAddr->pBay = strdup(strtmp3);
                        }
                        else if(strcmp(strtmp2,"YB") == 0)
                        {
                            ptSAddr->pYbId = strdup(strtmp3);
                        }
                    }
                }
            }
        }
    }
    //logMsg("############ %s.   %s  %d  %d   %s \n",pSAddrString,ptSAddr->pType, ptSAddr->ucCpuId, ptSAddr->usCfgIndex, ptSAddr->pYbId,0);
    return res;
}

#if 0
/*
描述:是否过程层控制块
根据LDINST 判断是否是过程层的接收SV或者GOOSE
根据<GOCBref name="IL2201API/LLN0$GO$gocb1">的name中把IEDNAME去掉
首先根据"IL2201API/LLN0$GO$gocb1"得到 IL2201API，
然后 将 IL2201A 去掉 得到了PI，与列表中的比较，
包含则为过程层的(PI01也算)，否则为站控层的
参数:
pIEDName: IED名称
pCBrefName: pCBrefName控制块名称
ucType:判断GOOSE还是SV
返回值: 是否属于过程层的控制块
TRUE:是过程层控制块
FALSE:不是过程层控制块
 */
BOOL EDP_IsProcessCb(char *pIEDName, char *pCBrefName, uint8_t ucType)
{
    BOOL res = FALSE;
    BOOL bFind = FALSE;
    char strtmp[128];
    char ldInst[32];
    char *pTmp;
    int i = 0;
    int j = 0;

    if(pIEDName == NULL || pCBrefName == NULL)
    {
        return FALSE;
    }

    /*先得到IEDNAME_LDINST的字符串, 存在ldInst中 */
    if(GetEnumItem_Div(pCBrefName,0,strtmp,'/',sizeof(strtmp)) > 0)
    {
        while(pIEDName[i] != '\0' && strtmp[i] != '\0')
        {
            if(pIEDName[i] != strtmp[i])
                return FALSE;
            else
                i++;
        }
        pTmp = &(strtmp[i]);
        i = 0;
        while( pTmp[i] != '\0')
        {
            ldInst[i] = pTmp[i];
            i++;
        }
        ldInst[i] = '\0';
    }

    /*是否是过程层控制块*/
    if(ucType == GOOSE_TYPE)
    {
        for(j = 0; j < GDeviceNum; j++)
        {
            pTmp = g_sGDevice[j];
            i = 0;
            while((*pTmp) != '\0')
            {
                if( (*pTmp) != ldInst[i])
                    break;
                pTmp++;
                i++;
            }
            if((*pTmp) != '\0')
            {
                bFind = FALSE;
            }
            else
            {
                bFind = TRUE;
                /*如果是PI的话要不等于PISV才是GOOSE控制块*/
                if(j == 0)
                {
                    if(ldInst[0] == 'P' && ldInst[1] == 'I' && ldInst[2] == 'S' && ldInst[3] == 'V')
                    {
                        bFind = FALSE;
                    }
                }
                break;
            }

        }
    }
    else if(ucType == SMV_TYPE)
    {
        for(j = 0; j < VDeviceNum; j++)
        {
            pTmp = g_sVDevice[j];
            while((*pTmp) != '\0')
            {
                if( (*pTmp) != ldInst[i])
                    break;
                pTmp++;
            }
            if((*pTmp) != '\0')
            {
                bFind = FALSE;
            }
            else
            {
                bFind = TRUE;
                break;
            }
        }
    }

    res = bFind;
    return res;
}
#else
/* 功能:
 *      通过本GOCB的板件号判断是否为过程层GOCB(仅适用于平台与HMI不共CPU的装置)
 * 参数:
 *      iBoradId, 本GOCB通讯的板件号
 * 返回:
 *      TRUE, 该GOCB为过程层GOCB;FALSE, 该GOCB不是过程层GOCB
 */
BOOL EDP_IsProcessCb(int iBoardId)
{
    int iBoardNo = 0;
    for(iBoardNo = 0; iBoardNo < g_tPrivateCfg.iBoardCnt; iBoardNo++)
    {
        if(g_tPrivateCfg.pBoard[iBoardNo].iBoardId == iBoardId)
        {
            return TRUE;
        }
    }
    return FALSE;
}
#endif
/*
描述:初始化GCB信息
参数:
iIndex:索引 ,从1开始
pSubDaRd: GSE的FCDA指针
pFcdaCfg:CCD的FCDA信息
返回值: 初始化是否成功
 */
BOOL EDP_InitSubGcbInfo(int iIndex, GSE_SUB_INFO *pSubInfoNode, SUB_GCB_INFO *pSubGcbCfg)
{
    if(pSubInfoNode == NULL || pSubGcbCfg == NULL)
    {
        return FALSE;
    }

    pSubInfoNode->GcbIndex = iIndex;
    pSubInfoNode->ulRcvRrmCnt = 0;
    pSubInfoNode->ulRcvCnt = 0;
    pSubInfoNode->ulRcvCfgErrCnt = 0;

    /*解析控制块与数据集信息*/
    pSubInfoNode->UserInfo.pIEDName = strdup(pSubGcbCfg->tConnectAp.pApIedName);
    pSubInfoNode->UserInfo.gcRef = strdup(pSubGcbCfg->pGOCBrefName);
    /*pSubInfoNode->UserInfo.dataSetRef=strdup(pSubGcbCfg->tDataSet.pDateSetName);*/
    pSubInfoNode->UserInfo.dataSetRef = malloc(strlen(pSubGcbCfg->tConnectAp.pApIedName)
                                        + strlen(pSubGcbCfg->tConnectAp.tConnectApGse.pLdInst)
                                        + strlen(pSubGcbCfg->tDataSet.pDateSetName)+10);
    sprintf(pSubInfoNode->UserInfo.dataSetRef, "%s%s/LLN0$%s",
            pSubGcbCfg->tConnectAp.pApIedName,
            pSubGcbCfg->tConnectAp.tConnectApGse.pLdInst,
            pSubGcbCfg->tDataSet.pDateSetName);  /*datasetRef字符串拼接*/
    pSubInfoNode->UserInfo.goID = strdup(pSubGcbCfg->tGseCtrl.pAppID);
    pSubInfoNode->UserInfo.confRev = pSubGcbCfg->tGseCtrl.iConfRev;
    pSubInfoNode->UserInfo.appID = pSubGcbCfg->tConnectAp.tConnectApGse.pGseAddr[0].usAppID;
    pSubInfoNode->UserInfo.DataNum = pSubGcbCfg->tDataSet.iFcdaCnt;
    pSubInfoNode->pSubGooseGcb = (void *)pSubGcbCfg;
    /*pmacstr = pSubGcbCfg->tConnectAp.tConnectApGse.pGseAddr[0].aDstMac;*/

    return TRUE;
}


/*
描述:初始化GCB的网络信息
参数:
pSubDaRd: GSE的FCDA指针
pFcdaCfg:CCD的FCDA信息
返回值: 初始化是否成功
 */
BOOL EDP_InitSubGcbNetInfo(GSE_SUB_INFO *pSubInfoNode, SUB_GCB_INFO *pSubGcbCfg, EDP_SADDR_STR *ptSAddr)
{
    int i = 0;
    int j = 0;
    GSE_AP_GSE_ADDR *pGseAddr = NULL;

    /*解析网络信息*/
    for(i = 0; i < pSubGcbCfg->tConnectAp.tConnectApGse.iAddrCnt; i++)
    {
        pGseAddr = &(pSubGcbCfg->tConnectAp.tConnectApGse.pGseAddr[i]);
        pSubInfoNode->NetInfo.num++;

        /*MAC地址*/
        for(j = 0; j < 6; j++)
        {
            pSubInfoNode->NetInfo.addr[i].dst_mac[j] = pGseAddr->aDstMac[j];
        }

        /*接收端口信息,需要从private中的该CPU板子获取*/
        EDP_GetCPUConnectPort(i, ptSAddr->ucCpuId, PORT_TRANSFOR_DATA_TYPE_GS, &(pSubInfoNode->NetInfo));
        pSubInfoNode->NetInfo.addr[i].portcount++;
        pSubInfoNode->NetInfo.addr[i].portnum[0]=0x01;        /*此处没错, 按位赋1*/
    }

    if(pSubInfoNode->NetInfo.num == 1)
    {
        pSubInfoNode->NetInfo.type = ALONE;
    }
    else if(pSubInfoNode->NetInfo.num > 1)
    {
        pSubInfoNode->NetInfo.type = REDUNDANT;
    }

    return TRUE;
}

/*
描述:初始化GCB信息DATASET中的FCDA的结构，一条一条初始化
参数:
iIndex:索引 ,从0开始,处理时+1操作
pSubDaRd: GSE的FCDA指针
pFcdaCfg:CCD的FCDA信息
返回值: 初始化是否成功
 */
BOOL EDP_InitSubGcbFcdaInfo(int iIndex, DA_INFO *pSubDaRd, GSE_DATESET_FCDA_SUB *pFcdaCfg)
{
    if(pSubDaRd == NULL || pFcdaCfg == NULL)
    {
        return FALSE;
    }

    pSubDaRd->index = iIndex + 1;
    pSubDaRd->ordinal = 0;
    pSubDaRd->type = GetDataValTypebyStr(pFcdaCfg->pBType);
    pSubDaRd->desc = strdup(pFcdaCfg->pIntAddr[0].pIntaddrDesc);

    pSubDaRd->value=NewDaValueDouble();
    pSubDaRd->packettime=NewDaValueDouble();
    return TRUE;
}

/*
描述:初始化datamap的结构，一条一条初始化
参数:
iIndex:索引 ,从0开始,处理时+1操作,因为ordinal为0表示悬空
ptSAddr:SAddr指针
pSubMapNode: 待初始化的指针
pFcdaCfg:对应接收通道FCDA结构指针
usOldCfgIndex:CCD中的硬件序号
返回值: 初始化是否成功
 */
BOOL EDP_InitSubMapInfo(int iIndex, EDP_SADDR_STR *ptSAddr, SUB_MAP_INFO *pSubMapNode, GSE_DATESET_FCDA_SUB *pFcdaCfg, uint32_t usOldCfgIndex)
{
    uint32_t ulValue = 0;
    if(ptSAddr == NULL || pSubMapNode == NULL)
    {
        return FALSE;
    }
    //logMsg("EDP_InitSubMapInfo  index:%d, mapinfo ptr:%08x,  pValueType:%s, Saddr type:%s \n",iIndex, (int)pSubMapNode,pValueType,ptSAddr->pType,0,0);
    pSubMapNode->index = iIndex + 1;        /*go_cfg中就是这么+1 的*/
    pSubMapNode->cpuid = 1;
    pSubMapNode->ordinal = iIndex + 1;
    pSubMapNode->dType = 2;

    /*数据类型*/
    pSubMapNode->type = GetDataValTypebyStr(pFcdaCfg->pBType);

    /*DAINDEX*/
    if(strcmp(pFcdaCfg->pBType, "Quality") == 0)
    {
        ulValue = 0x00000002L;
    }
    else if(strcmp(pFcdaCfg->pBType, "Timestamp") == 0)
    {
        ulValue = 0x00000003L;
    }
    else
    {
        ulValue = 0x00000001L;
    }

    pSubMapNode->AIOIndex = usOldCfgIndex/10000;

    if(ptSAddr->pType[0] == 'S' && ptSAddr->pType[1] == 'u' && ptSAddr->pType[2] == 'b' &&
            ptSAddr->pType[3] == '_' && ptSAddr->pType[4] == 'R' && ptSAddr->pType[5] == 's' &&
            ptSAddr->pType[6] == 'v')
    {
        pSubMapNode->DaIndex = 0x02000000L |(ptSAddr->usCfgIndex<<8) |ulValue;
    }
    else if(ptSAddr->pType[0] == 'S' && ptSAddr->pType[1] == 'u' && ptSAddr->pType[2] == 'b' &&
            ptSAddr->pType[3] == '_' && ptSAddr->pType[4] == 'M' && ptSAddr->pType[5] == 'e' &&
            ptSAddr->pType[6] == 'a')
    {
        pSubMapNode->DaIndex = 0x01000000L |(ptSAddr->usCfgIndex<<8) |ulValue;
    }
    pSubMapNode->desc = pFcdaCfg->pIntAddr[0].pIntaddrDesc;

    return TRUE;
}


/*
描述:初始化GCB信息
参数:
iIndex:索引 ,从0开始,处理时+1操作
pSubDaRd: GSE的FCDA指针
pFcdaCfg:CCD的FCDA信息
返回值: 初始化是否成功
 */
BOOL EDP_InitPubGcbInfo(int iIndex, GSE_PUB_INFO *pPubInfoNode, PUB_GCB_INFO *pPubGcbCfg)
{
    if(pPubInfoNode == NULL || pPubGcbCfg == NULL)
    {
        return FALSE;
    }

    pPubInfoNode->GcbIndex = iIndex;
    pPubInfoNode->ulSndCnt = 0;
    /*解析控制块与数据集信息*/
    pPubInfoNode->UserInfo.pIEDName = strdup(pPubGcbCfg->tConnectAp.pApIedName);
    pPubInfoNode->UserInfo.gcRef = strdup(pPubGcbCfg->pGOCBrefName);
    /*pSubInfoNode->UserInfo.dataSetRef=strdup(pSubGcbCfg->tDataSet.pDateSetName);*/
    pPubInfoNode->UserInfo.dataSetRef = malloc(strlen(pPubGcbCfg->tConnectAp.pApIedName)
                                        + strlen(pPubGcbCfg->tConnectAp.tConnectApGse.pLdInst)
                                        + strlen(pPubGcbCfg->tDataSet.pDateSetName)+10);
    sprintf(pPubInfoNode->UserInfo.dataSetRef, "%s%s/LLN0$%s",
            pPubGcbCfg->tConnectAp.pApIedName,
            pPubGcbCfg->tConnectAp.tConnectApGse.pLdInst,
            pPubGcbCfg->tDataSet.pDateSetName);  /*datasetRef字符串拼接*/
    pPubInfoNode->UserInfo.goID = strdup(pPubGcbCfg->tGseCtrl.pAppID);
    pPubInfoNode->UserInfo.confRev = pPubGcbCfg->tGseCtrl.iConfRev;
    pPubInfoNode->UserInfo.appID = pPubGcbCfg->tConnectAp.tConnectApGse.pGseAddr[0].usAppID;
    pPubInfoNode->UserInfo.T0=pPubGcbCfg->tConnectAp.tConnectApGse.tGseTime.iMaxTime;
    pPubInfoNode->UserInfo.T1=pPubGcbCfg->tConnectAp.tConnectApGse.tGseTime.iMinTime;;
    pPubInfoNode->UserInfo.Priority=pPubGcbCfg->tConnectAp.tConnectApGse.pGseAddr[0].usVPri;
    //pPubInfoNode->UserInfo.vid=pPubGcbCfg->tConnectAp.tConnectApGse.pGseAddr[0].usVID;

    pPubInfoNode->UserInfo.DataNum = pPubGcbCfg->tDataSet.iFcdaCnt;

    pPubInfoNode->pPubGooseGcb = (void *)pPubGcbCfg;
    /*解析网络信息*/
    /*pmacstr = pSubGcbCfg->tConnectAp.tConnectApGse.pGseAddr[0].aDstMac;*/

    return TRUE;
}


/*
描述:初始化GCB的网络信息
参数:
iIndex:索引 ,从0开始,处理时+1操作
pSubDaRd: GSE的FCDA指针
pFcdaCfg:CCD的FCDA信息
ptSAddr:解析的短地址信息
返回值: 初始化是否成功
 */
BOOL EDP_InitPubGcbNetInfo(GSE_PUB_INFO *pPubInfoNode, PUB_GCB_INFO *pPubGcbCfg, EDP_SADDR_STR *ptSAddr)
{
    int i = 0;
    int j = 0;
    GSE_AP_GSE_ADDR *pGseAddr = NULL;

    /*解析网络信息*/
    for(i = 0; i < pPubGcbCfg->tConnectAp.tConnectApGse.iAddrCnt; i++)
    {
        pGseAddr = &(pPubGcbCfg->tConnectAp.tConnectApGse.pGseAddr[i]);
        pPubInfoNode->NetInfo.num++;

        /*MAC地址*/
        for(j = 0; j < 6; j++)
        {
            pPubInfoNode->NetInfo.addr[i].dst_mac[j] = pGseAddr->aDstMac[j];
        }

        /*VID*/
        pPubInfoNode->NetInfo.addr[i].vid = pGseAddr->usVID;

        /*发送端口信息也是由PRIVATE中的该板子的级联口获取*/
        EDP_GetCPUConnectPort(i, ptSAddr->ucCpuId, PORT_TRANSFOR_DATA_TYPE_GS, &(pPubInfoNode->NetInfo));
        /*for(j = 0; j < pPubGcbCfg->tConnectAp.iPhysConnCnt; j++)
        {
            pPubInfoNode->NetInfo.addr[i].portcount++;
            pPubInfoNode->NetInfo.addr[i].portnum[j] = EDP_GetPortId(pPubGcbCfg->tConnectAp.tConnectApPhysConn[j].pPort);
        }*/
    }

    if(pPubInfoNode->NetInfo.num == 1)
    {
        pPubInfoNode->NetInfo.type = ALONE;
    }
    else if(pPubInfoNode->NetInfo.num > 1)
    {
        pPubInfoNode->NetInfo.type = REDUNDANT;
    }

    return TRUE;

}


/*
描述:初始化GCB信息DATASET中的FCDA的结构，一条一条初始化
参数:
iIndex:索引 ,从0开始,处理时+1操作
pSubDaRd: GSE的FCDA指针
pFcdaCfg:CCD的FCDA信息
返回值: 初始化是否成功
 */
BOOL EDP_InitPubGcbFcdaInfo(int iIndex, DA_INFO *pPubDaRd, GSE_DATESET_FCDA_PUB *pFcdaCfg)
{
    if(pPubDaRd == NULL || pFcdaCfg == NULL)
    {
        return FALSE;
    }

    pPubDaRd->index = iIndex + 1;
    pPubDaRd->ordinal = 0;
    pPubDaRd->type = GetDataValTypebyStr(pFcdaCfg->pBType);
    pPubDaRd->desc = strdup(pFcdaCfg->pFcdaDesc);

    pPubDaRd->value=NewDaValue();
    pPubDaRd->packettime=NewDaValue();
    return TRUE;
}

/*
描述:初始化datamap的结构，一条一条初始化
参数:
iIndex:索引 ,从0开始,处理时+1操作,因为ordinal为0表示悬空
ptSAddr:SAddr指针
pSubMapNode: 待初始化的指针
pValueType:值类型
返回值: 初始化是否成功
 */
BOOL EDP_InitPubMapInfo(int iIndex, EDP_SADDR_STR *ptSAddr, PUB_MAP_INFO *pPubMapNode, char *pValueType)
{
    uint32_t ulValue = 0;
    if(ptSAddr == NULL || pPubMapNode == NULL)
    {
        return FALSE;
    }
    //logMsg("EDP_InitPubMapInfo  index:%d, mapinfo ptr:%08x,  pValueType:%s, Saddr type:%s \n",iIndex, (int)pPubMapNode,pValueType,ptSAddr->pType,0,0);
    //pPubMapNode->index = iIndex + 1;        /*go_cfg中就是这么+1 的*/
    pPubMapNode->cpuid = 1;
    pPubMapNode->ordinal = iIndex + 1;
    pPubMapNode->dType = 2;
    pPubMapNode->linkLogicID = strdup(ptSAddr->pYbId);

    /*数据类型*/
    pPubMapNode->type = GetDataValTypebyStr(pValueType);

    /*DAINDEX*/
    if(strcmp(pValueType, "Quality") == 0)
    {
        ulValue = 0x00000002L;
    }
    else if(strcmp(pValueType, "Timestamp") == 0)
    {
        ulValue = 0x00000003L;
    }
    else
    {
        ulValue = 0x00000001L;
    }
    if(ptSAddr->pType[0] == 'P' && ptSAddr->pType[1] == 'u' && ptSAddr->pType[2] == 'b' &&
            ptSAddr->pType[3] == '_' && ptSAddr->pType[4] == 'R' && ptSAddr->pType[5] == 's' &&
            ptSAddr->pType[6] == 'v')
    {
        pPubMapNode->index = 0x02000000L |(ptSAddr->usCfgIndex<<8) |ulValue;
    }
    else if(ptSAddr->pType[0] == 'P' && ptSAddr->pType[1] == 'u' && ptSAddr->pType[2] == 'b' &&
            ptSAddr->pType[3] == '_' && ptSAddr->pType[4] == 'M' && ptSAddr->pType[5] == 'e' &&
            ptSAddr->pType[6] == 'a')
    {
        pPubMapNode->index = 0x01000000L |(ptSAddr->usCfgIndex<<8) |ulValue;
    }

    return TRUE;
}



/*
描述:将datamap和sub goose结构关联
参数:
pSubMapNode:datamap指针
pSubDaRd:sub gcb 中的FCDA指针
pSubInfoNode: sub gcb指针
ptSAddr: 短地址
返回值: 初始化是否成功
 */
BOOL EDP_LinkSubToMapData(SUB_MAP_INFO *pSubMapNode, DA_INFO *pSubDaRd, GSE_SUB_INFO *pSubInfoNode, EDP_SADDR_STR *ptSAddr)
{
    pSubMapNode->pa = pSubDaRd;
    if(pSubDaRd->type == UTCTIME_TYPE)
    {
        pSubMapNode->value = pSubDaRd->packettime;
    }
    else
    {
        pSubMapNode->value = pSubDaRd->value;
    }
    pSubMapNode->pInfoNode = pSubInfoNode;

    pSubDaRd->bvaluemapped = TRUE;
    pSubDaRd->pointid = ptSAddr->usCfgIndex - 1;
    return TRUE;
}

/*
描述: 解析过程层SV配置信息，内存数据结构转换
参数:     NONE
返回值: 解析是否成功
 */
int EDP_ConfigParseSmvSub()
{
    int res = 0;
    /*配置结构*/
    PROCESS_SUB_SMV *pSubSmvCfg = NULL;
    SUB_SMV_INFO *pSubSmvInfo = NULL;
    SMV_DATESET_FCDA_SUB *pFcdaCfg = NULL;
    SMV_DATESET_FCDA_INTADDR *pIntAddr = NULL;
    EDP_SADDR_STR tSAddr;
    PRIVATE_BOARD *pBoard;

    /*实时结构*/
    IEC_SMV_9_1_CFG *pSubInfoNode = NULL;
    BOARD_PORT_CONNECT *pPortConnect = NULL;

    int iSmvChnNo = 0; /*通道序号*/
    int iAdsuChnNo;
    BOOL bMyChn = FALSE;

    int i = 0, j = 0, k = 0;

    g_pSmvCfg = calloc(1, sizeof(IEC_SMV_CFG));

    pSubSmvCfg = (PROCESS_SUB_SMV *)EDP_GetProcessSubSmv();

    if(pSubSmvCfg->iSubSmvCnt == 0)
    {
        CFG_LOG("------>过程层无SMV SUB配置信息数据\n",1,2,3,4,5,6);
        res = 0;
        goto exit;
    }

#ifdef EDP02_PSR_BUILD  /*针对测控STI板，多个光口接收单ASDU的采样报文*/

#else
    pPortConnect = EDP_GetSvMasterCcPortConnectPtr();
    pBoard = EDP_GetPrivateBoardPtrFromNodeAddr(g_ucCpuId);
    if(pPortConnect == NULL)
    {
        CFG_LOG("------>过程层SMV SUB 找不到SV采样主CC板\n"
                ,pSubSmvInfo->iIndex,pFcdaCfg->iIndex,3,4,5,6);
        res |= EDP_CONFIG_PARSE_SUB_SMV_ERR;
        goto exit;
    }

    g_pSmvCfg->smvNum = 1;
    pSubInfoNode = &g_pSmvCfg->Smv_9_1Cfg[0];

    pSubInfoNode->smvPortChn = pPortConnect->iOutPortId;
    memcpy(pSubInfoNode->smvSrc, pPortConnect->aSvMacAddr, MAC_BYTES);
    pSubInfoNode->appID = pPortConnect->usAppID;
    pSubInfoNode->smprate9_2 = pPortConnect->iSvPubRate;
    pSubInfoNode->receiveType = pPortConnect->iSvType;
    pSubInfoNode->forceSyn = pPortConnect->iSvForceSyn;

    pSubInfoNode->asduNum = pSubSmvCfg->iSubSmvCnt;
    pSubInfoNode->dataNum = 0;

    for(i = 0; i < pSubSmvCfg->iSubSmvCnt; i++)
    {
        pSubSmvInfo = &(pSubSmvCfg->pSubSmvInfo[i]);
        iAdsuChnNo = 1;/* 从1开始计数，用于ASDU内部通道序号排序 */
        for(j = 0; j < pSubSmvInfo->tDataSet.iFcdaCnt; j++)
        {
            bMyChn = FALSE;
            pFcdaCfg = &(pSubSmvInfo->tDataSet.pDataSetFcda[j]);
            if(!pFcdaCfg->pIntAddr->bIsIntAddrValid)
            {
                pFcdaCfg = pFcdaCfg->next;
                if(pFcdaCfg == NULL)
                {
                    CFG_LOG("------>过程层SMV SUB SMVCBref %d 有效通道数不匹配\n"
                            ,pSubSmvInfo->iIndex,2,3,4,5,6);
                    res |= EDP_CONFIG_PARSE_SUB_SMV_ERR;
                    goto exit;
                }
                continue;
            }

            for(k = 0; k < pFcdaCfg->iIntAddrCnt; k++)
            {
                pIntAddr = &(pFcdaCfg->pIntAddr[k]);

                if(pIntAddr->tDai.pSAddr == NULL)
                {
                    continue;
                }
                memset(&tSAddr,0,sizeof(tSAddr));
                if(EDP_ConfigParseSAddr(pIntAddr->tDai.pSAddr, &tSAddr))
                {
                    if(pFcdaCfg->iIndex >= 32)
                    {
                        pSubSmvInfo->ulSelect2[tSAddr.ucCpuId] |= 0x01<<(pFcdaCfg->iIndex-32);
                    }
                    else
                    {
                        pSubSmvInfo->ulSelect1[tSAddr.ucCpuId] |= 0x01<<pFcdaCfg->iIndex;
                    }

                    if(tSAddr.ucCpuId != g_ucCpuId)
                    {
                        /*找到本CPU对应的saddr配置*/
                        pIntAddr = pIntAddr->next;
                        continue;
                    }
                    bMyChn = TRUE;
                    pSubInfoNode->smvData[iSmvChnNo].smvAdsuNo = pSubSmvInfo->iIndex+1;
                    pSubInfoNode->smvData[iSmvChnNo].smvAdsuChn = iAdsuChnNo;
                    pSubInfoNode->smvData[iSmvChnNo].smvDataChn = tSAddr.usCfgIndex;
                    if(!strcmp(pFcdaCfg->pLnClass, "TVTR"))
                    {
                        /*当前通道为电压通道*/
                        pSubInfoNode->smvData[iSmvChnNo].smvChnType = 1;
                        if(tSAddr.bIsNegative)
                        {
                            pSubInfoNode->smvData[iSmvChnNo].smvValOut = -SMV_VOL_VAL_OUT;
                        }
                        else
                        {
                            pSubInfoNode->smvData[iSmvChnNo].smvValOut = SMV_VOL_VAL_OUT;
                        }
                    }
                    else if(!strcmp(pFcdaCfg->pLnClass, "TCTR"))
                    {
                        /*当前通道为电流通道*/
                        pSubInfoNode->smvData[iSmvChnNo].smvChnType = 1;
                        if(pBoard->iBoardType != BOARD_TYPE_MEA_CPU)
                        {
                            if(tSAddr.bIsNegative)
                            {
                                pSubInfoNode->smvData[iSmvChnNo].smvValOut = -SMV_PROTECT_AMP_VAL_OUT;
                            }
                            else
                            {
                                pSubInfoNode->smvData[iSmvChnNo].smvValOut = SMV_PROTECT_AMP_VAL_OUT;
                            }
                        }
                        else
                        {
                            if(tSAddr.bIsNegative)
                            {
                                pSubInfoNode->smvData[iSmvChnNo].smvValOut = -SMV_MEASURE_AMP_VAL_OUT;
                            }
                            else
                            {
                                pSubInfoNode->smvData[iSmvChnNo].smvValOut = SMV_MEASURE_AMP_VAL_OUT;
                            }
                        }
                    }
                    else if(tSAddr.usCfgIndex > 400)
                    {
                        /* 模型配置采样延时通道短地址约定 */
                        pSubSmvInfo->iDelayChnNo = j;
                        pSubInfoNode->smvData[iSmvChnNo].smvChnType = 2;
                        pSubInfoNode->smvData[iSmvChnNo].smvValOut = 0;
                        pSubInfoNode->smvData[iSmvChnNo].smvDataChn = 254;
                    }
                    else
                    {
                        CFG_LOG("------>过程层SMV SUB SMVCBref %d Fcda %d 未知的通道类型\n"
                                ,pSubSmvInfo->iIndex,pFcdaCfg->iIndex,3,4,5,6);
                        res |= EDP_CONFIG_PARSE_SUB_SMV_ERR;
                        goto exit;
                    }

                    if(tSAddr.pYbId != NULL)
                    {
                        strcpy(pSubInfoNode->smvData[iSmvChnNo].MuYabanIDStr, tSAddr.pYbId);
                        pSubInfoNode->smvData[iSmvChnNo].MuLinkUSE = TRUE;
                    }
                    else
                    {
                        pSubInfoNode->smvData[iSmvChnNo].MuLinkUSE = FALSE;
                    }
                    pSubInfoNode->smvData[iSmvChnNo].smvDes = strdup(pIntAddr->pDesc);
                    iSmvChnNo++;
                }
                /*
                else
                {
                    CFG_LOG("------>过程层SMV SUB SMVCBref %d Fcda %d sAddr未配置\n"
                        ,pSubSmvInfo->iIndex,pFcdaCfg->iIndex,3,4,5,6);
                    res |= EDP_CONFIG_PARSE_SUB_SMV_ERR;
                    goto exit;
                }
                */
            }
            if(bMyChn)
            {
                iAdsuChnNo++; /*每个FCDA对应一个外部通道*/
            }
        }
    }
    pSubInfoNode->dataNum = iSmvChnNo;
#endif
exit:
    return res;
}


/*
描述: 解析过程层SV配置信息，内存数据结构转换
参数:     NONE
返回值: 解析是否成功
 */
int EDP_ConfigParseSmvPub()
{
    int res = 0;

    return res;
}

/*
描述: 解析过程层SV配置信息，内存数据结构转换
参数:     NONE
返回值: 解析是否成功
 */
int EDP_ConfigParseSmv()
{
    int res = 0;

    res = EDP_ConfigParseSmvSub();
    if(res != 0)
    {
        CFG_LOG("------>过程层SMV Sub 配置转换数据结构失败，错误码:%08x\n",res,0,0,0,0,0);
    }

    res = EDP_ConfigParseSmvPub();
    if(res != 0)
    {
        CFG_LOG("------>过程层SMV Pub  配置转换数据结构失败，错误码:%08x\n",res,0,0,0,0,0);
    }

    return res;
}

/*
描述: 解析过程层GOOSE配置信息，内存数据结构转换
参数:     NONE
返回值: 解析是否成功
 */
int EDP_ConfigParseGooseSub()
{
    int res = 0;
    /*配置结构*/
    PROCESS_SUB_GSE *pSubGooseCfg = NULL;
    SUB_GCB_INFO *pSubGcbCfg = NULL;
    GSE_DATESET_FCDA_SUB *pFcdaCfg = NULL;
    GSE_DATESET_FCDA_INTADDR *pIntAddr = NULL;
    EDP_SADDR_STR *ptSAddr = NULL;

    /*实时结构*/
    GSE_SUB_INFO *pSubInfoNode = NULL;
    SUB_MAP_INFO *pSubMapNode = NULL;
    DA_INFO *pSubDaRd = NULL;
    int iSubMapCnt = 0;
    int iFcdaCnt = 0;
    BOOL bNetInfoConfiged = FALSE;      /*GCB网络信息是否被配置了*/

    /*实时结构All*/
    GSE_SUB_INFO *pSubInfoNode2 = NULL;
    SUB_MAP_INFO *pSubMapNode2 = NULL;
    DA_INFO *pSubDaRd2 = NULL;
    int iSubMapCnt2 = 0;
    BOOL bNetInfoConfiged2 = FALSE;      /*GCB网络信息是否被配置了*/

    /*用于填写正确的ORDINAL，解决GOOSE接收一对多的问题*/
    int ordinal = 0;
    int ordinal2 = 0;

    uint32_t usCfgIndex = 0;

    BOOL bIsGcbNeedRecv = FALSE;   /*表示本GOOSE控制块是否是本板接收的*/
    int iBoardId = 0;       /* 当前GOCB的通讯板号 */

    int i = 0;
    int j = 0;
    int k = 0;

    pSubGooseCfg = (PROCESS_SUB_GSE *)EDP_GetProcessSubGoose();

    if(pSubGooseCfg->iSubGsCnt == 0)
    {
        CFG_LOG("------>过程层无GOOSE SUB配置信息数据\n",1,2,3,4,5,6);
        res = 0;
        goto exit;
    }

    g_tSubGoose.pSubInfoRootNode = (GSE_SUB_INFO *)calloc(pSubGooseCfg->iSubGsCnt,sizeof(GSE_SUB_INFO));
    g_tSubGooseAll.pSubInfoRootNode = (GSE_SUB_INFO *)calloc(pSubGooseCfg->iSubGsCnt,sizeof(GSE_SUB_INFO));
    g_ptSubSAddr = (EDP_SADDR_STR *)calloc(pSubGooseCfg->iSubGsCnt,sizeof(EDP_SADDR_STR));
    if(g_tSubGoose.pSubInfoRootNode == NULL || g_tSubGooseAll.pSubInfoRootNode == NULL || g_ptSubSAddr == NULL)
    {
        CFG_LOG("------>过程层GOOSE SUB配置信息数据分配GSE_SUB_INFO内存失败,SUB 个数:%d \n",
                pSubGooseCfg->iSubGsCnt,2,3,4,5,6);
        res = EDP_CONFIG_PARSE_SUB_GOOSE_ERR;
        goto exit;
    }

    for(i = 0; i < pSubGooseCfg->iSubGsCnt; i++)
    {
        bIsGcbNeedRecv = FALSE;     /*初始化均认为不接受*/
        bNetInfoConfiged = FALSE;
        bNetInfoConfiged2 = FALSE;
        /*配置结构*/
        pSubGcbCfg = &(pSubGooseCfg->pSubGcbInfo[i]);
        if(i < (pSubGooseCfg->iSubGsCnt - 1))
        {
            pSubGcbCfg->next = (&(pSubGooseCfg->pSubGcbInfo[i+1]));
        }
        /*实时结构*/
        pSubInfoNode = &(g_tSubGoose.pSubInfoRootNode[g_usProcessSubGooseNum]);   /*此处不用i,用g_usProcessSubGooseNum*/
        if(i < (pSubGooseCfg->iSubGsCnt - 1))
        {
            pSubInfoNode->next = (&(g_tSubGoose.pSubInfoRootNode[g_usProcessSubGooseNum+1]));
        }
        pSubInfoNode2 = &(g_tSubGooseAll.pSubInfoRootNode[g_usProcessSubGooseNumAll]);   /*此处不用i,用g_usProcessSubGooseNumAll*/
        if(i < (pSubGooseCfg->iSubGsCnt - 1))
        {
            pSubInfoNode2->next = (&(g_tSubGooseAll.pSubInfoRootNode[g_usProcessSubGooseNumAll+1]));
        }

        iBoardId = EDP_GetBoardId(pSubGcbCfg->pIntAddrName);
        /*判断是否站控层GOOSE，如果是站控层的则CONTINUE*/
        if(EDP_IsProcessCb(iBoardId))
        {
            g_usProcessSubGooseNum++;
            g_usProcessSubGooseNumAll++;
        }
        else
        {
            continue;
        }

        EDP_InitSubGcbInfo(g_usProcessSubGooseNum, pSubInfoNode, pSubGcbCfg);
        EDP_InitSubGcbInfo(g_usProcessSubGooseNumAll, pSubInfoNode2, pSubGcbCfg);

        /*解析FCDA*/
        iFcdaCnt = pSubGcbCfg->tDataSet.iFcdaCnt;
        pSubInfoNode->pDataMbr = (DA_INFO *)calloc(iFcdaCnt,sizeof(DA_INFO));
        pSubInfoNode2->pDataMbr = (DA_INFO *)calloc(iFcdaCnt,sizeof(DA_INFO));
        if(pSubInfoNode->pDataMbr == NULL || pSubInfoNode2->pDataMbr == NULL)
        {
            CFG_LOG("------>过程层GOOSE SUB配置信息数据分配DA_INFO内存失败,FCDA 个数:%d \n",
                    iFcdaCnt,2,3,4,5,6);
            res = EDP_CONFIG_PARSE_SUB_GOOSE_ERR;
            goto exit;
        }

        for(j = 0; j < iFcdaCnt; j++)
        {
            pFcdaCfg = &(pSubGcbCfg->tDataSet.pDataSetFcda[j]);
            if(j < (iFcdaCnt - 1))
            {
                pFcdaCfg->next = (&(pSubGcbCfg->tDataSet.pDataSetFcda[j+1]));
            }

            pSubDaRd = &(pSubInfoNode->pDataMbr[j]);
            pSubDaRd2 = &(pSubInfoNode2->pDataMbr[j]);
            if(j < (iFcdaCnt - 1))
            {
                pSubDaRd->next = (&(pSubInfoNode->pDataMbr[j+1]));
                pSubDaRd2->next = (&(pSubInfoNode2->pDataMbr[j+1]));
            }

            /*初始化GCB SUB GOOSE DATA下的FCDA信息*/
            EDP_InitSubGcbFcdaInfo(j, pSubDaRd, pFcdaCfg);
            EDP_InitSubGcbFcdaInfo(j, pSubDaRd2, pFcdaCfg);

            for(k = 0; k < pFcdaCfg->iIntAddrCnt; k++)
            {
                pIntAddr = &(pFcdaCfg->pIntAddr[k]);
                ptSAddr = &(pIntAddr->tDai.tSAddr);
                if(pIntAddr->bIsIntAddrValid && pIntAddr->bIsDaiValid)
                {
                    if(EDP_ConfigParseSAddr(pIntAddr->tDai.pSAddr, ptSAddr))
                    {
                        if(k == 0)
                        {
                            ordinal2 = iSubMapCnt2;
                        }

                        /* 针对多合一GOOSE开入进行处理 */
                        usCfgIndex = ptSAddr->usCfgIndex;
                        ptSAddr->usCfgIndex -= (ptSAddr->usCfgIndex/10000)*10000;

                        /*不管是不是本插件的信息都将信息保存到所有控制块的结构中去*/
                        /*此处要增加datamap的结构与之关联*/
                        if(iSubMapCnt2 == 0)
                        {
                            pSubMapNode2 = NewSubMapInfoNode();
                            g_tSubGooseAll.pSubMapRootNode = pSubMapNode2;
                        }
                        else
                        {
                            pSubMapNode2->next = NewSubMapInfoNode();
                            pSubMapNode2 = pSubMapNode2->next;
                        }
                        /*初始化该datamap的指针*/
                        EDP_InitSubMapInfo(ordinal2, ptSAddr, pSubMapNode2, pFcdaCfg, usCfgIndex);

                        /*根据intaddt和saddr确认其他GCB的信息,网络信息等*/
                        pSubDaRd2->ordinal = ordinal2 + 1;         /*将ordinal与DATAMAP中的对应,从1开始*/
                        if(!bNetInfoConfiged2)
                        {
                            EDP_InitSubGcbNetInfo(pSubInfoNode2, pSubGcbCfg, ptSAddr);
                            memcpy(&(g_ptSubSAddr[i]),  ptSAddr, sizeof(EDP_SADDR_STR));
                            pSubInfoNode2->UserInfo.ybLogicID = strdup(ptSAddr->pYbId);
                            bNetInfoConfiged2 = TRUE;
                        }

                        if(ptSAddr->pYbId != NULL)
                        {
                            pSubDaRd->ybLogicID = ptSAddr->pYbId;
                            pSubDaRd->bLinked = TRUE;
                            pSubDaRd2->ybLogicID = ptSAddr->pYbId;
                            pSubDaRd2->bLinked = TRUE;
                        }
                        else
                        {
                            pSubDaRd->ybLogicID = NULL;
                            pSubDaRd->bLinked = FALSE;
                            pSubDaRd2->ybLogicID = NULL;
                            pSubDaRd2->bLinked = FALSE;
                        }
                        iSubMapCnt2++;

                        /*本插件的GSE信息*/
                        if((g_ucCpuId == ptSAddr->ucCpuId)
                                || (g_bRedunCpu&&(ptSAddr->ucCpuId == 1)))
                        {
                            if(k == 0)
                            {
                                ordinal = iSubMapCnt;
                            }
                            /*此处要增加datamap的结构与之关联*/
                            if(iSubMapCnt == 0)
                            {
                                pSubMapNode = NewSubMapInfoNode();
                                g_tSubGoose.pSubMapRootNode = pSubMapNode;
                            }
                            else
                            {
                                pSubMapNode->next = NewSubMapInfoNode();
                                pSubMapNode = pSubMapNode->next;
                            }
                            /*初始化该datamap的指针*/
                            EDP_InitSubMapInfo(ordinal, ptSAddr, pSubMapNode, pFcdaCfg, usCfgIndex);

                            /*根据intaddt和saddr确认其他GCB的信息,网络信息等*/
                            pSubInfoNode->UserInfo.ybLogicID = strdup(ptSAddr->pYbId);
                            pSubDaRd->ordinal = ordinal + 1;         /*将ordinal与DATAMAP中的对应,从1开始*/
                            if(!bNetInfoConfiged)
                            {
                                EDP_InitSubGcbNetInfo(pSubInfoNode, pSubGcbCfg, ptSAddr);
                                bNetInfoConfiged = TRUE;
                            }

                            /*sub info 与 data map的对应关系,不需再次对应，还是由原有的EDP程序来关联*/
                            //EDP_LinkSubToMapData(pSubMapNode, pSubDaRd, pSubInfoNode, ptSAddr);

                            iSubMapCnt++;
                            bIsGcbNeedRecv = TRUE;
                        }
                        else    /*该FCDA本板未接收*/
                        {
                        }
                    }
                }
            }
        }

        /*为TRUE, 则本GCB是本板需要接收的*/
        if(bIsGcbNeedRecv)
        {
        }
        else        /*该GCB本板不接收,则GCB个数-1*/
        {
            g_usProcessSubGooseNum--;
        }

    }

    /*本板关联的GOOSE DI数目*/
    g_ulGooseSubMapCnt = iSubMapCnt;

exit:
    return res;
}

/*
描述: 解析过程层GOOSE配置信息，内存数据结构转换
参数:     NONE
返回值: 解析是否成功
 */
int EDP_ConfigParseGoosePub()
{
    int res = 0;
    /*配置结构*/
    PROCESS_PUB_GSE *pPubGooseCfg = NULL;
    PUB_GCB_INFO *pPubGcbCfg = NULL;
    GSE_DATESET_FCDA_PUB *pFcdaCfg = NULL;
    GSE_DATESET_FCDA_DAI *pDai = NULL;
    EDP_SADDR_STR *ptSAddr = NULL;

    /*实时结构*/
    GSE_PUB_INFO *pPubInfoNode = NULL;
    PUB_MAP_INFO *pPubMapNode = NULL;
    DA_INFO *pPubDaRd = NULL;
    int iPubMapCnt = 0;
    int iFcdaCnt = 0;
    BOOL bNetInfoConfiged = FALSE;      /*GCB网络信息是否被配置了*/

    /*实时结构*/
    GSE_PUB_INFO *pPubInfoNode2 = NULL;
    PUB_MAP_INFO *pPubMapNode2 = NULL;
    DA_INFO *pPubDaRd2 = NULL;
    int iPubMapCnt2 = 0;
    BOOL bNetInfoConfiged2 = FALSE;      /*GCB网络信息是否被配置了*/


    BOOL bIsGcbNeedTrans = FALSE;   /*表示本GOOSE控制块是否是本板发送的*/
    int iPhyConnNo = 0;     /* GOCB内通讯端口信息计数 */
    int iBoardId = 0;       /* 当前GOCB的通讯板号 */

    int i = 0;
    int j = 0;

    pPubGooseCfg = (PROCESS_PUB_GSE *)EDP_GetProcessPubGoose();

    if(pPubGooseCfg->iPubGsCnt == 0)
    {
        CFG_LOG("------>过程层无GOOSE PUB配置信息数据\n",1,2,3,4,5,6);
        res = 0;
        goto exit;
    }

    g_tPubGoose.pPubInfoRootNode = (GSE_PUB_INFO *)calloc(pPubGooseCfg->iPubGsCnt,sizeof(GSE_PUB_INFO));
    g_tPubGooseAll.pPubInfoRootNode = (GSE_PUB_INFO *)calloc(pPubGooseCfg->iPubGsCnt,sizeof(GSE_PUB_INFO));
    g_ptPubSAddr = (EDP_SADDR_STR *)calloc(pPubGooseCfg->iPubGsCnt,sizeof(EDP_SADDR_STR));
    if(g_tPubGoose.pPubInfoRootNode == NULL || g_tPubGooseAll.pPubInfoRootNode == NULL || g_ptPubSAddr == NULL)
    {
        CFG_LOG("------>过程层GOOSE PUB配置信息数据分配GSE_PUB_INFO内存失败,PUB 个数:%d \n",
                pPubGooseCfg->iPubGsCnt,2,3,4,5,6);
        res = EDP_CONFIG_PARSE_PUB_GOOSE_ERR;
        goto exit;
    }

    for(i = 0; i < pPubGooseCfg->iPubGsCnt; i++)
    {
        bIsGcbNeedTrans = FALSE;     /*初始化均认为不接受*/
        bNetInfoConfiged = FALSE;
        bNetInfoConfiged2 = FALSE;
        /*配置结构*/
        pPubGcbCfg = &(pPubGooseCfg->pPubGcbInfo[i]);
        if(i < (pPubGooseCfg->iPubGsCnt - 1))
        {
            pPubGcbCfg->next = (&(pPubGooseCfg->pPubGcbInfo[i+1]));
        }
        /*实时结构*/
        pPubInfoNode = &(g_tPubGoose.pPubInfoRootNode[g_usProcessPubGooseNum]);   /*此处不用i,用g_usProcessPubGooseNum*/
        pPubInfoNode2 = &(g_tPubGooseAll.pPubInfoRootNode[g_usProcessPubGooseNumAll]);   /*此处不用i,用g_usProcessPubGooseNumAll*/
        if(i < (pPubGooseCfg->iPubGsCnt - 1))
        {
            pPubInfoNode->next = (&(g_tPubGoose.pPubInfoRootNode[g_usProcessPubGooseNum+1]));
            pPubInfoNode2->next = (&(g_tPubGooseAll.pPubInfoRootNode[g_usProcessPubGooseNumAll+1]));
        }

        /*判断是否站控层GOOSE，如果是站控层的则CONTINUE*/
        for(iPhyConnNo = 0; iPhyConnNo < pPubGcbCfg->tConnectAp.iPhysConnCnt; iPhyConnNo++)
        {
            iBoardId = EDP_GetBoardId(pPubGcbCfg->tConnectAp.tConnectApPhysConn[iPhyConnNo].pPort);
            if(EDP_IsProcessCb(iBoardId))
            {
                break;
            }
        }
        if(iPhyConnNo != pPubGcbCfg->tConnectAp.iPhysConnCnt)
        {
            g_usProcessPubGooseNum++;
            g_usProcessPubGooseNumAll++;
        }
        else
        {
            continue;
        }

        EDP_InitPubGcbInfo(g_usProcessPubGooseNum, pPubInfoNode, pPubGcbCfg);
        EDP_InitPubGcbInfo(g_usProcessPubGooseNumAll, pPubInfoNode2, pPubGcbCfg);

        /*解析FCDA*/
        iFcdaCnt = pPubGcbCfg->tDataSet.iFcdaCnt;
        pPubInfoNode->pDataMbr = (DA_INFO *)calloc(iFcdaCnt,sizeof(DA_INFO));
        pPubInfoNode2->pDataMbr = (DA_INFO *)calloc(iFcdaCnt,sizeof(DA_INFO));
        if(pPubInfoNode->pDataMbr == NULL || pPubInfoNode2->pDataMbr == NULL)
        {
            CFG_LOG("------>过程层GOOSE PUB配置信息数据分配DA_INFO内存失败,FCDA 个数:%d \n",
                    iFcdaCnt,2,3,4,5,6);
            res = EDP_CONFIG_PARSE_PUB_GOOSE_ERR;
            goto exit;
        }

        for(j = 0; j < iFcdaCnt; j++)
        {
            pFcdaCfg = &(pPubGcbCfg->tDataSet.pDataSetFcda[j]);
            if(j < (iFcdaCnt - 1))
            {
                pFcdaCfg->next = (&(pPubGcbCfg->tDataSet.pDataSetFcda[j+1]));
            }

            pPubDaRd = &(pPubInfoNode->pDataMbr[j]);
            pPubDaRd2 = &(pPubInfoNode2->pDataMbr[j]);
            if(j < (iFcdaCnt - 1))
            {
                pPubDaRd->next = (&(pPubInfoNode->pDataMbr[j+1]));
                pPubDaRd2->next = (&(pPubInfoNode2->pDataMbr[j+1]));
            }

            /*初始化GCB PUB GOOSE DATA下的FCDA信息*/
            EDP_InitPubGcbFcdaInfo(j, pPubDaRd, pFcdaCfg);
            EDP_InitPubGcbFcdaInfo(j, pPubDaRd2, pFcdaCfg);

            pDai = &(pFcdaCfg->tDai);
            ptSAddr = &(pDai->tSAddr);
            if(pFcdaCfg->bIsDaiValid)
            {
                if(EDP_ConfigParseSAddr(pDai->pSAddr, ptSAddr))
                {

                    /*不管是不是本插件的信息都将信息保存到所有控制块的结构中去*/
                    /*此处要增加datamap的结构与之关联*/
                    if(iPubMapCnt2 == 0)
                    {
                        pPubMapNode2 = NewPubMapInfoNode();
                        g_tPubGooseAll.pPubMapRootNode = pPubMapNode2;
                    }
                    else
                    {
                        pPubMapNode2->next = NewPubMapInfoNode();
                        pPubMapNode2 = pPubMapNode2->next;
                    }
                    /*初始化该datamap的指针*/
                    EDP_InitPubMapInfo(iPubMapCnt2, ptSAddr, pPubMapNode2, pFcdaCfg->pBType);

                    /*根据intaddt和saddr确认其他GCB的信息*/
                    pPubDaRd2->ordinal = iPubMapCnt2 + 1;         /*将ordinal与DATAMAP中的对应,从1开始*/

                    if(!bNetInfoConfiged2)
                    {
                        EDP_InitPubGcbNetInfo(pPubInfoNode2, pPubGcbCfg, ptSAddr);
                        memcpy(&(g_ptPubSAddr[i]),  ptSAddr, sizeof(EDP_SADDR_STR));
                        pPubGcbCfg->iPubGcbCpuId = ptSAddr->ucCpuId;
                        bNetInfoConfiged2 = TRUE;
                    }
                    iPubMapCnt2++;

                    if((g_ucCpuId == ptSAddr->ucCpuId)
                            || (g_bRedunCpu&&(ptSAddr->ucCpuId == 1)))
                    {
                        /*此处要增加datamap的结构与之关联*/
                        if(iPubMapCnt == 0)
                        {
                            pPubMapNode = NewPubMapInfoNode();
                            g_tPubGoose.pPubMapRootNode = pPubMapNode;
                        }
                        else
                        {
                            pPubMapNode->next = NewPubMapInfoNode();
                            pPubMapNode = pPubMapNode->next;
                        }
                        /*初始化该datamap的指针*/
                        EDP_InitPubMapInfo(iPubMapCnt, ptSAddr, pPubMapNode, pFcdaCfg->pBType);

                        /*根据intaddt和saddr确认其他GCB的信息*/
                        pPubDaRd->ordinal = iPubMapCnt + 1;         /*将ordinal与DATAMAP中的对应,从1开始*/

                        if(!bNetInfoConfiged)
                        {
                            EDP_InitPubGcbNetInfo(pPubInfoNode, pPubGcbCfg, ptSAddr);
                            bNetInfoConfiged = TRUE;
                        }
                        /*Pub info 与 data map的对应关系,不需再次对应，还是由原有的EDP程序来关联*/
                        //EDP_LinkPubToMapData(pPubMapNode, pPubDaRd, pPubInfoNode, ptSAddr);

                        iPubMapCnt++;
                        bIsGcbNeedTrans = TRUE;
                    }
                    else    /*该FCDA本板未接收*/
                    {
                    }
                }
            }
        }

        /*为TRUE, 则本GCB是本板需要接收的*/
        if(bIsGcbNeedTrans)
        {
        }
        else        /*该GCB本板不接收,则GCB个数-1*/
        {
            g_usProcessPubGooseNum--;
        }

    }

    /*本板关联的GOOSE DI数目*/
    g_ulGoosePubMapCnt = iPubMapCnt;

exit:
    return res;
}

/*
描述: 解析过程层GOOSE配置信息，内存数据结构转换
参数:     NONE
返回值: 解析是否成功
 */
int EDP_ConfigParseGoose()
{
    int res = 0;

    res = EDP_ConfigParseGooseSub();
    if(res != 0)
    {
        CFG_LOG("------>过程层GOOSE Sub 配置转换数据结构失败，错误码:%08x\n",res,0,0,0,0,0);
    }

    res = EDP_ConfigParseGoosePub();
    if(res != 0)
    {
        CFG_LOG("------>过程层GOOSE Pub  配置转换数据结构失败，错误码:%08x\n",res,0,0,0,0,0);
    }

    return res;
}



/*
描述: 解析过程层配置信息，内存数据结构转换
参数:     NONE
返回值: 解析是否成功
 */
int EDP_ConfigParseCPU()
{
    int res = 0;

    res = EDP_ConfigParseSmv();
    if(res != 0)
    {
        CFG_LOG("------>过程层SMV 配置转换数据结构失败，错误码:%08x\n",res,0,0,0,0,0);
    }

    res = EDP_ConfigParseGoose();
    if(res != 0)
    {
        CFG_LOG("------>过程层GOOSE 配置转换数据结构失败，错误码:%08x\n",res,0,0,0,0,0);
    }

    return res;
}

/*
描述: 释放内存,中间结果Gse
参数:    NONE
返回值: 释放是否成功
 */
int EDP_FreeTempSubGseStruct()
{
    int i = 0;
    int cnt = 0;
    EP_STATUS res = EP_SUCCESS;
    GSE_SUB_INFO *pSubInfoNode = NULL;
    GSE_SUB_INFO *pSubInfoNode2 = NULL;
    SUB_MAP_INFO	*pSubMapNode = NULL;
    SUB_MAP_INFO	*pSubMapNode2 = NULL;

    cnt = EDP_GetSubGooseCnt();

    for(i = 0; i < cnt; i++)
    {
        pSubInfoNode = &(g_tSubGoose.pSubInfoRootNode[i]);
        pSubInfoNode2 = &(g_tSubGooseAll.pSubInfoRootNode[i]);
        /*logMsg( "11111  0x%x, 0x%x \n",(int)pSubInfoNode,(int)pSubInfoNode2,0,0,0,0);*/
        free(pSubInfoNode->pDataMbr);
        free(pSubInfoNode2->pDataMbr);
        pSubInfoNode->pDataMbr = NULL;
        pSubInfoNode2->pDataMbr = NULL;
    }
    free(g_tSubGoose.pSubInfoRootNode);
    g_tSubGoose.pSubInfoRootNode = NULL;
    free(g_tSubGooseAll.pSubInfoRootNode);
    g_tSubGooseAll.pSubInfoRootNode = NULL;

    pSubMapNode = g_tSubGoose.pSubMapRootNode;
    while(pSubMapNode != NULL)
    {
        /*logMsg("Sub1: 0x%x,\n", (int)pSubMapNode,0,0,0,0,0);*/
        pSubMapNode2 = pSubMapNode->next;
        free(pSubMapNode);
        pSubMapNode = pSubMapNode2;
    }

    pSubMapNode = g_tSubGooseAll.pSubMapRootNode;
    while(pSubMapNode != NULL)
    {
        /*logMsg("Sub2: 0x%x,\n", (int)pSubMapNode,0,0,0,0,0);*/
        pSubMapNode2 = pSubMapNode->next;
        free(pSubMapNode);
        pSubMapNode = pSubMapNode2;
    }

    return res;
}
/*
描述: 释放内存,中间结果Gse
参数:    NONE
返回值: 释放是否成功
 */
int EDP_FreeTempPubGseStruct()
{
    int i = 0;
    int cnt = 0;
    EP_STATUS res = EP_SUCCESS;
    GSE_PUB_INFO	*pPubInfoRootNode = NULL;
    GSE_PUB_INFO	*pPubInfoRootNode2 = NULL;
    PUB_MAP_INFO	*pPubMapNode = NULL;
    PUB_MAP_INFO	*pPubMapNode2 = NULL;

    cnt = EDP_GetPubGooseCnt();

    for(i = 0; i < cnt; i++)
    {
        pPubInfoRootNode = &(g_tPubGoose.pPubInfoRootNode[i]);
        pPubInfoRootNode2 = &(g_tPubGooseAll.pPubInfoRootNode[i]);
        free(pPubInfoRootNode->pDataMbr);
        free(pPubInfoRootNode2->pDataMbr);
        pPubInfoRootNode->pDataMbr = NULL;
        pPubInfoRootNode2->pDataMbr = NULL;
    }

    free(g_tPubGoose.pPubInfoRootNode);
    g_tPubGoose.pPubInfoRootNode = NULL;
    free(g_tPubGooseAll.pPubInfoRootNode);
    g_tPubGooseAll.pPubInfoRootNode = NULL;

    pPubMapNode = g_tPubGoose.pPubMapRootNode;
    while(pPubMapNode != NULL)
    {
        /*logMsg("Pub1: 0x%x,\n", (int)pPubMapNode,0,0,0,0,0);*/
        pPubMapNode2 = pPubMapNode->next;
        free(pPubMapNode);
        pPubMapNode = pPubMapNode2;
    }

    pPubMapNode = g_tPubGooseAll.pPubMapRootNode;
    while(pPubMapNode != NULL)
    {
        /*logMsg("Pub2: 0x%x,\n", (int)pPubMapNode,0,0,0,0,0);*/
        pPubMapNode2 = pPubMapNode->next;
        free(pPubMapNode);
        pPubMapNode = pPubMapNode2;
    }

    return res;
}

/*
描述: 释放内存,中间结果Gse
参数:    NONE
返回值: 释放是否成功
 */
int EDP_FreeTempGseStruct()
{
    int res = 0;

    if(EDP_FreeTempSubGseStruct() != EP_SUCCESS)
    {
        logMsg("#######  EDP_FreeTempSubGseStruct执行出错\n",0,0,0,0,0,0);
        res = EP_ERROR;
    }

    if(EDP_FreeTempPubGseStruct() != EP_SUCCESS)
    {
        logMsg("#######  EDP_FreeTempPubGseStruct执行出错\n",0,0,0,0,0,0);
        res = EP_ERROR;
    }

    free(g_ptSubSAddr);
    free(g_ptPubSAddr);

    return res;
}
#if 0
/*
描述: 释放内存,中间结果Gse
参数:    NONE
返回值: 释放是否成功
 */
int EDP_FreeTempPubGseStruct()
{
    int i = 0;
    int cnt = 0;
    EP_STATUS res = EP_SUCCESS;
    GSE_SUB_INFO *pSubInfoNode = NULL;
    GSE_SUB_INFO *pSubInfoNode2 = NULL;
    GSE_PUB_INFO	*pPubInfoRootNode = NULL;
    GSE_PUB_INFO	*pPubInfoRootNode2 = NULL;
    SUB_MAP_INFO	*pSubMapNode = NULL;
    SUB_MAP_INFO	*pSubMapNode2 = NULL;
    PUB_MAP_INFO	*pPubMapNode = NULL;
    PUB_MAP_INFO	*pPubMapNode2 = NULL;

    cnt = EDP_GetSubGooseCnt();

    for(i = 0; i < cnt; i++)
    {
        pSubInfoNode = &(g_tSubGoose.pSubInfoRootNode[i]);
        pSubInfoNode2 = &(g_tSubGooseAll.pSubInfoRootNode[i]);
        free(pSubInfoNode->pDataMbr);
        free(pSubInfoNode2->pDataMbr);
        pSubInfoNode->pDataMbr = NULL;
        pSubInfoNode2->pDataMbr = NULL;
    }
    free(g_tSubGoose.pSubInfoRootNode);
    g_tSubGoose.pSubInfoRootNode = NULL;
    free(g_tSubGooseAll.pSubInfoRootNode);
    g_tSubGooseAll.pSubInfoRootNode = NULL;

    pSubMapNode = g_tSubGoose.pSubMapRootNode->next;
    free(g_tSubGoose.pSubMapRootNode);
    g_tSubGoose.pSubMapRootNode = NULL;
    while(pSubMapNode != NULL)
    {
        logMsg("Sub1: 0x%x,\n", (int)pSubMapNode,0,0,0,0,0);
        pSubMapNode2 = pSubMapNode->next;
        free(pSubMapNode);
        pSubMapNode = pSubMapNode2;
    }

    pSubMapNode = g_tSubGooseAll.pSubMapRootNode->next;
    free(g_tSubGooseAll.pSubMapRootNode);
    g_tSubGooseAll.pSubMapRootNode = NULL;
    while(pSubMapNode != NULL)
    {
        logMsg("Sub2: 0x%x,\n", (int)pSubMapNode,0,0,0,0,0);
        pSubMapNode2 = pSubMapNode->next;
        free(pSubMapNode);
        pSubMapNode = pSubMapNode2;
    }

    cnt = EDP_GetPubGooseCnt();

    for(i = 0; i < cnt; i++)
    {
        pPubInfoRootNode = &(g_tPubGoose.pPubInfoRootNode[i]);
        pPubInfoRootNode2 = &(g_tPubGooseAll.pPubInfoRootNode[i]);
        free(pPubInfoRootNode->pDataMbr);
        free(pPubInfoRootNode2->pDataMbr);
        pPubInfoRootNode->pDataMbr = NULL;
        pPubInfoRootNode2->pDataMbr = NULL;
    }

    free(g_tPubGoose.pPubInfoRootNode);
    g_tPubGoose.pPubInfoRootNode = NULL;
    free(g_tPubGooseAll.pPubInfoRootNode);
    g_tPubGooseAll.pPubInfoRootNode = NULL;

    pPubMapNode = g_tPubGoose.pPubMapRootNode->next;
    free(g_tPubGoose.pPubMapRootNode);
    g_tPubGoose.pPubMapRootNode = NULL;
    while(pPubMapNode != NULL)
    {
        logMsg("Pub1: 0x%x,\n", (int)pPubMapNode,0,0,0,0,0);
        pPubMapNode2 = pPubMapNode->next;
        free(pPubMapNode);
        pPubMapNode = pPubMapNode2;
    }

    pPubMapNode = g_tPubGooseAll.pPubMapRootNode->next;
    free(g_tPubGooseAll.pPubMapRootNode);
    g_tPubGooseAll.pPubMapRootNode = NULL;
    while(pPubMapNode != NULL)
    {
        logMsg("Pub2: 0x%x,\n", (int)pPubMapNode,0,0,0,0,0);
        pPubMapNode2 = pPubMapNode->next;
        free(pPubMapNode);
        pPubMapNode = pPubMapNode2;
    }

    /*无需释放,里面的指针地址是指向原先释放的地方*/
    /*for(i = 0; i < EDP_GetProcessSubGoose()->iSubGsCnt; i++)
    {
        pSubSaddr = &(g_ptSubSAddr[i]);
        free(pSubSaddr->pBay);
        pSubSaddr->pBay = NULL;
        free(pSubSaddr->pTDI);
        pSubSaddr->pTDI = NULL;
        free(pSubSaddr->pType);
        pSubSaddr->pType = NULL;
        free(pSubSaddr->pYbId);
        pSubSaddr->pYbId = NULL;
    }*/
    free(g_ptSubSAddr);

    /*无需释放,里面的指针地址是指向原先释放的地方*/
    /*for(i = 0; i < EDP_GetProcessPubGoose()->iPubGsCnt; i++)
    {
        pPubSaddr = &(g_ptPubSAddr[i]);
        free(pPubSaddr->pBay);
        pPubSaddr->pBay = NULL;
        free(pPubSaddr->pTDI);
        pPubSaddr->pTDI = NULL;
        free(pPubSaddr->pType);
        pPubSaddr->pType = NULL;
        free(pPubSaddr->pYbId);
        pPubSaddr->pYbId = NULL;
    }*/
    free(g_ptPubSAddr);

    return res;
}
#endif
/*
描述: 释放内存,中间结果 Smv
参数:    NONE
返回值: 释放是否成功
 */
int EDP_FreeTempSmvStruct()
{
    EP_STATUS res = EP_SUCCESS;
    return res;
}

/*
描述: 释放内存,CCD
参数:    NONE
返回值: 释放是否成功
 */
int EDP_FreeTempStruct()
{
    EP_STATUS res = EP_SUCCESS;

    if(EDP_FreeTempGseStruct() != EP_SUCCESS)
    {
        logMsg("#######  EDP_FreeTempGseStruct执行出错\n",0,0,0,0,0,0);
        res = EP_ERROR;
    }
    if(EDP_FreeTempSmvStruct() != EP_SUCCESS)
    {
        logMsg("#######  EDP_FreeTempSmvStruct执行出错\n",0,0,0,0,0,0);
        res = EP_ERROR;
    }

    return res;
}


BOOL EDP_ShowSubGse()
{
    int i = 0;

    for(i = 0; i < g_usProcessSubGooseNum; i++)
    {

    }
    return TRUE;
}


BOOL EDP_ShowPubGse()
{
    int i = 0;

    for(i = 0; i < g_usProcessPubGooseNum; i++)
    {

    }
    return TRUE;
}

