/**************************************************************************
EDP_UnifiedCfgParseCCD.c

九统一配置文件解析

Copyright (c) 2014 SNAC(Guodian Nanjing Automation Co., Ltd.)
All Rights Reserved.

History:
Revision     Date          Who            Comment
-----------------------------------------------------------------
V1.00         2015.03.03    kevin         初始版本
***************************************************************************/

#include <stdio_compat.h>
#include "string_compat.h"
#include "EDP_UnifiedCfgParsePrivate.h"
#include "EDP_UnifiedCfgParseCCD.h"
#include "EDP_UnifiedCfgMain.h"
#include "EDP_UnifiedCfgParseConfig.h"
#include "edp_asst.h"

PRIVATE_CFG g_tPrivateCfg;      /*过程层私有配置结构*/
uint8_t g_ucLineType = 0;        /*接线方式*/
extern BOOL g_bRedunCpu;
uint8_t g_ucPortNotoBoardId[MAX_CC_PORT_NUM] = {0};/*通过主CC的级联端口获取CC板的BoardId，主CC的级联端口为0*/

/*
描述: 解析privatecfg.xml的board中的port入口函数.
参数:     privateFile, 文件指针.
返回值: 无
 */
int EDP_ParsePrivatePort(mxml_node_t *pnode, PRIVATE_BOARD *pBoardInfo)
{
    char *p;
    mxml_node_t *node1=NULL;
    mxml_node_t *root_node=NULL;
    int res = 0;
    PRIVATE_BOARD *pBoard=NULL;
    BOARD_PORT_CONNECT *pPortConnect=NULL;
    int i = 0;
    int j = 0;
    int k = 0;
    char *tokenPtr = NULL;
    char Tmp[4];

    root_node = pnode;
    pBoard = pBoardInfo;

    if(pBoard->tPort.iConnectCnt == 0)
    {
        goto exit;
    }

    node1 = mxmlFindElement(root_node, root_node, "ConnectPort",NULL, NULL,MXML_DESCEND);
    while(node1 != NULL)
    {
        pPortConnect = &(pBoard->tPort.pPortConnect[j]);
        pPortConnect->iIndex = j;

        p=(char *)mxmlElementGetAttr(node1,"id");
        if(p)
        {
            //CFG_LOG("--->ConnectPort  id: %s\n\n",(int)p,0,0,0,0,0);
            if(((int)p[0] >= (int)'a') && ((int)p[0] <= (int)'z') )
            {
                pPortConnect->iPortId = (int)(p[0] - 'a' );
            }
            else if(((int)p[0] >= (int)'A') && ((int)p[0] <= (int)'Z' ))
            {
                pPortConnect->iPortId = (int)(p[0] - 'A' );
            }
        }

        p=(char *)mxmlElementGetAttr(node1,"dataType");
        if(p)
        {
            //CFG_LOG("ConnectPort  dataType: %s\n\n",(int)p,0,0,0,0,0);
            pPortConnect->iDataType = strtol(p,NULL,10);
        }
        else
        {
            res |= EDP_PRIVATE_PARSE_ERR_FILE_XML_PORT_PARSE_FAIL;
        }

        p=(char *)mxmlElementGetAttr(node1,"outPortId");
        if(p)
        {
            //CFG_LOG("ConnectPort  outPortId: %s\n\n",(int)p,0,0,0,0,0);
            pPortConnect->pOutPortId = strdup(p);
            /*解析出单独的板号和端口*/
            pPortConnect->iOutBoardId = EDP_GetBoardId(p);
            pPortConnect->iOutPortId = EDP_GetPortId(p);
        }
        else
        {
            //res |= EDP_PRIVATE_PARSE_ERR_FILE_XML_PORT_PARSE_FAIL;
        }

        p=(char *)mxmlElementGetAttr(node1,"svMacAdr");
        if(p)
        {
            //CFG_LOG("ConnectPort  svMacAdr: %s\n\n",(int)p,0,0,0,0,0);
            tokenPtr = strtok(strdup(p),"-");
            for(k = 0; (k < 6) && (tokenPtr != NULL); k++)
            {
                pPortConnect->aSvMacAddr[k] = strtol(tokenPtr,NULL,16);
                tokenPtr = strtok(NULL,"-");
            }
        }

        p=(char *)mxmlElementGetAttr(node1,"svAppID");
        if(p)
        {
            // CFG_LOG("ConnectPort  svPubRate: %s\n\n",(int)p,0,0,0,0,0);
            pPortConnect->usAppID = strtol(p,NULL,16);
        }

        p=(char *)mxmlElementGetAttr(node1,"svPubRate");
        if(p)
        {
            // CFG_LOG("ConnectPort  svPubRate: %s\n\n",(int)p,0,0,0,0,0);
            pPortConnect->iSvPubRate = strtol(p,NULL,10);
        }

        p=(char *)mxmlElementGetAttr(node1,"svType");
        if(p)
        {
            // CFG_LOG("ConnectPort  svPubRate: %s\n\n",(int)p,0,0,0,0,0);
            pPortConnect->iSvType = strtol(p,NULL,10);
        }

        p=(char *)mxmlElementGetAttr(node1,"svForceSyn");
        if(p)
        {
            // CFG_LOG("ConnectPort  svPubRate: %s\n\n",(int)p,0,0,0,0,0);
            pPortConnect->iSvForceSyn = strtol(p,NULL,10);
        }

        p=(char *)mxmlElementGetAttr(node1,"gsParentCpuType");
        if(p)
        {
            //CFG_LOG("ConnectPort  gsParentCpuType: %s\n\n",(int)p,0,0,0,0,0);
            pPortConnect->iGsParentCpuType = strtol(p,NULL,10);
        }

        p=(char *)mxmlElementGetAttr(node1,"gsCbIndex");
        if(p)
        {
            //CFG_LOG("ConnectPort  gsCbIndex: %s\n\n",(int)p,0,0,0,0,0);
            pPortConnect->pGsCbIndex = strdup(p);
            for(i = 0; i < 256; i++)
            {
                GetEnumItem_Div(p,i,Tmp,',',sizeof(Tmp));
                if(Tmp[0] == '\0')
                    break;
                else
                {
                    pPortConnect->iGsCbList[i] = strtol(Tmp,NULL,10);
                }
            }
            pPortConnect->iGsCbCnt = i;
        }


        j++;
        node1 = mxmlFindElement(node1, root_node, "ConnectPort",NULL, NULL,MXML_DESCEND);
    }

exit:
    return res;
}

/*
描述: 解析privatecfg.xml的board中的smv入口函数.
参数:     privateFile, 文件指针.
返回值: 无
 */
int EDP_ParsePrivateSmv(mxml_node_t *pnode, PRIVATE_BOARD *pBoardInfo)
{
    char *p;
    mxml_node_t *node1=NULL;
    int res = 0;
    PRIVATE_BOARD *pBoard=NULL;

    node1 = pnode;
    pBoard = pBoardInfo;

    p=(char *)mxmlElementGetAttr(node1,"svmode");
    if(p)
    {
        pBoard->tSmv.tSmvCommon.iMode = strtol(p,NULL,10);
    }
    else
    {
        res |= EDP_CONFIG_PARSE_SUB_GOOSE_ERR3;
    }

    p=(char *)mxmlElementGetAttr(node1,"svsourceRate");
    if(p)
    {
        pBoard->tSmv.tSmvCommon.iSourceRate = strtol(p,NULL,10);
    }
    else
    {
        res |= EDP_CONFIG_PARSE_SUB_GOOSE_ERR3;
    }

    p=(char *)mxmlElementGetAttr(node1,"svextSynMode");
    if(p)
    {
        pBoard->tSmv.tSmvCommon.iExtSynMode= strtol(p,NULL,10);
    }
    else
    {
        res |= EDP_CONFIG_PARSE_SUB_GOOSE_ERR3;
    }

    p=(char *)mxmlElementGetAttr(node1,"svsynPulse");
    if(p)
    {
        pBoard->tSmv.tSmvCommon.iSynPulse= strtol(p,NULL,10);
    }
    else
    {
        /*res |= EDP_CONFIG_PARSE_SUB_GOOSE_ERR3;*/
    }

    p=(char *)mxmlElementGetAttr(node1,"svsynPulseEnable");
    if(p)
    {
        pBoard->tSmv.tSmvCommon.iSynPulseEnable= strtol(p,NULL,10);
    }
    else
    {
        /*res |= EDP_CONFIG_PARSE_SUB_GOOSE_ERR3;*/
    }

    p=(char *)mxmlElementGetAttr(node1,"svextSynReverse");
    if(p)
    {
        pBoard->tSmv.tSmvCommon.iExtSynReverse= strtol(p,NULL,10);
    }
    else
    {
        /*res |= EDP_CONFIG_PARSE_SUB_GOOSE_ERR3;*/
    }

    p=(char *)mxmlElementGetAttr(node1,"svgmrpSendGap");
    if(p)
    {
        pBoard->tSmv.tSmvCommon.iGmrpSendGap= strtol(p,NULL,10);
    }
    else
    {
        /*res |= EDP_CONFIG_PARSE_SUB_GOOSE_ERR3;*/
    }

    p=(char *)mxmlElementGetAttr(node1,"Gmrp");
    if(p)
    {
        pBoard->tSmv.tSmvCommon.iGmrp= strtol(p,NULL,10);
    }
    else
    {
        /*res |= EDP_CONFIG_PARSE_SUB_GOOSE_ERR3;*/
    }

    p=(char *)mxmlElementGetAttr(node1,"maxdelay");
    if(p)
    {
        pBoard->tSmv.tSmvCommon.iMaxDelay= strtol(p,NULL,10);
    }
    else
    {
        res |= EDP_CONFIG_PARSE_SUB_GOOSE_ERR3;
    }

    return res;
}

/*
描述: 解析privatecfg.xml的board中的goose入口函数.
参数:     privateFile, 文件指针.
返回值: 无
 */
int EDP_ParsePrivateGoose(mxml_node_t *pnode, PRIVATE_BOARD *pBoardInfo)
{
    char *p;
    mxml_node_t *root_node=NULL;
    /* int res = 0; */
    PRIVATE_BOARD *pBoard=NULL;

    root_node = pnode;
    pBoard = pBoardInfo;

    p=(char *)mxmlElementGetAttr(root_node,"gsPacketFlow");
    if(p)
    {
        pBoard->iGsPacketFlow = strtol(p,NULL,10);
    }

    p=(char *)mxmlElementGetAttr(root_node,"gsDataFlow");
    if(p)
    {
        pBoard->iGsDataFlow = strtol(p,NULL,10);
    }
    return 0;
}


/*
描述: 解析CCD文件的私有配置文件 入口函数.
参数:     CCD文件的指针.
返回值: 解析是否成功
 */
int EDP_LoadPrivateFile(char *pFileName)
{
    FILE *fp;
    char *p;
    mxml_node_t *node1=NULL;
    mxml_node_t *node2=NULL;
    mxml_node_t *node3=NULL;
    mxml_node_t *root_node=NULL;
    int res = 0;
    int iBoardCnt = 0;
    int iConnectPortCnt = 0;
    PRIVATE_BOARD *pBoard=NULL;
    int i = 0;
    int j = 0;

    if(!pFileName)
    {
        res = EDP_PRIVATE_PARSE_ERR_FILE_NOT_EXIST;
        goto exit;
    }

    fp = fopen(pFileName, "r");
    if(fp==NULL)
    {
        res = EDP_PRIVATE_PARSE_ERR_FILE_OPEN_FAIL;
        goto exit;
    }

    root_node = mxmlLoadFile(NULL,fp,MXML_NO_CALLBACK);

    fclose(fp);

    if(!root_node)
    {
        res = EDP_PRIVATE_PARSE_ERR_FILE_XML_LOAD_FAIL;
        goto exit;
    }

    node1 = mxmlFindElement(root_node,root_node, "PrivateCfg",NULL, NULL,MXML_DESCEND);

    p = (char *)mxmlElementGetAttr(node1,"lineType");
    if(p)
    {
        g_tPrivateCfg.iConnectLineType = strtol(p,NULL,10);
    }
    else
    {
        res = EDP_PRIVATE_PARSE_ERR_FILE_XML_LOAD_FAIL;
        return res;
    }

    p = (char *)mxmlElementGetAttr(node1,"devType");
    if(p)
    {
        g_tPrivateCfg.iDeviceType = strtol(p,NULL,10);
    }

    iBoardCnt = EDP_GetXmlElementCnt(node1, "Board");

    if(iBoardCnt == 0)
    {
        goto exit;
    }

    g_tPrivateCfg.iBoardCnt = iBoardCnt;
    g_tPrivateCfg.pBoard = (PRIVATE_BOARD *)calloc(iBoardCnt, sizeof(PRIVATE_BOARD));
    if(! g_tPrivateCfg.pBoard)
    {
        CFG_LOG("#### kevin: PRIVATE 文件的BOARD 分配空间失败,BOARD个数:%d. \n",iBoardCnt,0,0,0,0,0);
        res = EDP_PRIVATE_PARSE_ERR_FILE_CALLOC_FAIL;
        goto exit;
    }

    node2 = mxmlFindElement(node1, node1, "Board",NULL, NULL,MXML_DESCEND);
    while(node2 != NULL)
    {
        pBoard = &(g_tPrivateCfg.pBoard[i]);
        pBoard->next = &(g_tPrivateCfg.pBoard[i+1]);
        pBoard->iIndex = i;

        p=(char *)mxmlElementGetAttr(node2,"id");
        if(p)
        {
            pBoard->iBoardId = strtol(p,NULL,10);
        }

        p=(char *)mxmlElementGetAttr(node2,"type");
        if(p)
        {
            pBoard->iBoardType = strtol(p,NULL,10);
        }
        else
        {
            res |= EDP_PRIVATE_PARSE_ERR_FILE_XML_BOARD_PARSE_FAIL;
        }

        p=(char *)mxmlElementGetAttr(node2,"desc");
        if(p)
        {
            pBoard->pBoardDesc = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node2,"parentCCId");
        if(p)
        {
            pBoard->iParentCCId = strtol(p,NULL,10);
        }
        else
        {
            res |= EDP_PRIVATE_PARSE_ERR_FILE_XML_BOARD_PARSE_FAIL;
        }

        p=(char *)mxmlElementGetAttr(node2,"subDevId");
        if(p)
        {
            pBoard->iSubDevId = strtol(p,NULL,10);
        }

        p=(char *)mxmlElementGetAttr(node2,"nodeAddr");
        if(p)
        {
            pBoard->iNodeAddr = strtol(p,NULL,10);
        }

        /* 通过私有配置的解析内容，判断当前装置是否为冗余CPU */
        if((pBoard->iNodeAddr == g_ucCpuId) && (pBoard->iBoardType == BOARD_TYPE_REDUN_CPU))
        {
            g_bRedunCpu = TRUE;
        }

        iConnectPortCnt = EDP_GetXmlElementCnt(node2, "ConnectPort");
        pBoard->tPort.iConnectCnt = iConnectPortCnt;
        if(iConnectPortCnt > 0)
        {
            pBoard->tPort.pPortConnect = (BOARD_PORT_CONNECT *)calloc(iConnectPortCnt, sizeof(BOARD_PORT_CONNECT));
            if(!pBoard->tPort.pPortConnect)
            {
                CFG_LOG("#### kevin: PRIVATE 文件的BOARD 下的CONNECT PORT 分配空间失败,BOARD个数:%d.CONNECT PORT个数:%d \n",i+1,iConnectPortCnt,0,0,0,0);
                res = EDP_PRIVATE_PARSE_ERR_FILE_CALLOC_FAIL;
                goto exit;
            }

            res |= EDP_ParsePrivatePort(node2, pBoard);
        }

        node3 = mxmlFindElement(node2, node2, "SMV",NULL, NULL,MXML_DESCEND);
        if(node3)
        {
            res |= EDP_ParsePrivateSmv(node3, pBoard);
        }

        node3 = mxmlFindElement(node2, node2, "GOOSE",NULL, NULL,MXML_DESCEND);
        if(node3)
        {
            res |= EDP_ParsePrivateGoose(node3, pBoard);
        }

        if(pBoard->iBoardType == BOARD_TYPE_CC)
        {
            if(pBoard->iBoardId == pBoard->iParentCCId)
            {
                g_ucPortNotoBoardId[0] = pBoard->iBoardId;
            }
            else
            {
                for(j = 0; j < pBoard->tPort.iConnectCnt; j++)
                {
                    g_ucPortNotoBoardId[pBoard->tPort.pPortConnect[j].iOutPortId] = pBoard->iBoardId;
                }
            }
        }

        i++;
        node2 = mxmlFindElement(node2, node1, "Board",NULL, NULL,MXML_DESCEND);
    }


    /*获取接线方式*/
    if(g_tPrivateCfg.iConnectLineType == CONNECTION_TYPE_LINE)
    {
        g_ucLineType = CONNECTION_TYPE_LINE;
    }
    else if(g_tPrivateCfg.iConnectLineType == CONNECTION_TYPE_HSB)
    {
        g_ucLineType = CONNECTION_TYPE_HSB;
    }

exit:
    if(root_node)
    {
        mxmlDelete(root_node);
    }
    return res;
}


/*
描述:获取private.xml中的borad个数
参数:无
返回值:返回private.xml中的borad个数
*/
int EDP_GetPrivateBoardCnt()
{
    return (g_tPrivateCfg.iBoardCnt);
}

/*
描述:获取private.xml中的borad 首指针
参数:无
返回值:返回private.xml中的borad 首指针
*/
PRIVATE_BOARD *EDP_GetPrivateBoardPtrFromIndex(int iNo)
{
    if(iNo < EDP_GetPrivateBoardCnt())
    {
        return (&(g_tPrivateCfg.pBoard[iNo]));
    }
    else
    {
        return NULL;
    }
}

/*
描述:获取private.xml中的指定iBoardId的borad 首指针
参数:
iBoardId: 板子的BoardId
返回值:返回private.xml中的borad 首指针
*/
PRIVATE_BOARD *EDP_GetPrivateBoardPtrFromId(int iBoardId)
{
    int i = 0;
    PRIVATE_BOARD *pBoard = NULL;

    for(i = 0; i < EDP_GetPrivateBoardCnt(); i++)
    {
        pBoard = EDP_GetPrivateBoardPtrFromIndex(i);
        if(pBoard->iBoardId == iBoardId)
        {
            break;
        }
    }
    return pBoard;
}

/*
描述:获取private.xml中的指定iNodeAddr的borad 首指针
参数:
iNodeAddr: 板子的NodeAddr
返回值:返回private.xml中的borad 首指针
*/
PRIVATE_BOARD *EDP_GetPrivateBoardPtrFromNodeAddr(int iNodeAddr)
{
    int i = 0;
    PRIVATE_BOARD *pBoard = NULL;

    for(i = 0; i < EDP_GetPrivateBoardCnt(); i++)
    {
        pBoard = EDP_GetPrivateBoardPtrFromIndex(i);
        if(pBoard->iNodeAddr== iNodeAddr)
        {
            break;
        }
    }
    return pBoard;
}

/*
描述:获取SV主CC板转发SV的端口的指针
参数:无
返回值:主CC板Board的指针
*/
BOARD_PORT_CONNECT *EDP_GetSvMasterCcPortConnectPtr()
{
    int iBoardCnt = 0;
    int i = 0, j = 0;
    PRIVATE_BOARD *pBoard = NULL;
    BOARD_PORT_CONNECT *pPortConnect = NULL;

    iBoardCnt = EDP_GetPrivateBoardCnt();
    for(i = 0; i < iBoardCnt; i++)
    {
        pBoard = EDP_GetPrivateBoardPtrFromIndex(i);
        pPortConnect = pBoard->tPort.pPortConnect;
        if((pBoard->iBoardType==BOARD_TYPE_CC) && (pBoard->iParentCCId == pBoard->iBoardId))
        {
            /*判断是否为主CC板*/
            for(j = 0; j < pBoard->tPort.iConnectCnt; j++)
            {
                if(((pPortConnect->iDataType == PORT_TRANSFOR_DATA_TYPE_SV)
                        || (pPortConnect->iDataType == PORT_TRANSFOR_DATA_TYPE_SVGS ))
                        && (pPortConnect->iOutBoardId == EDP_GetBoardIdByNodeAddr(g_ucCpuId)))
                {
                    /* 类型为SV或者SV+GS 并且转发到的CPU为当前CPU*/
                    return pPortConnect;
                }
                pPortConnect++;
            }
        }
    }
    return NULL;
}

/*
描述: 通过板件号获取板件HSB总线节点地址
参数:iBoardId: 板子的BoardId
返回值:返回节点地址
*/
int EDP_GetNodeAddrByBoardId(int iBoardId)
{
    int i = 0;
    PRIVATE_BOARD *pBoard = NULL;

    for(i = 0; i < EDP_GetPrivateBoardCnt(); i++)
    {
        pBoard = EDP_GetPrivateBoardPtrFromIndex(i);
        if(pBoard->iBoardId == iBoardId)
        {
            break;
        }
    }
    return pBoard->iNodeAddr;
}

/*
描述: 通过板件号获取板件HSB总线节点地址
参数:iNodeAddr: 板子的NodeAddr
返回值:返回板件号
*/
int EDP_GetBoardIdByNodeAddr(int iNodeAddr)
{
    int i = 0;
    PRIVATE_BOARD *pBoard = NULL;

    for(i = 0; i < EDP_GetPrivateBoardCnt(); i++)
    {
        pBoard = EDP_GetPrivateBoardPtrFromIndex(i);
        if(pBoard->iNodeAddr == iNodeAddr)
        {
            break;
        }
    }
    return pBoard->iBoardId;
}

/*
描述: 获取private.xml的指针
参数:无
返回值:返回private.xml的指针
*/
PRIVATE_CFG *EDP_GetPrivatePtr()
{
    return &(g_tPrivateCfg);
}

/*
描述:得到CC板个数,用来确定生成几个文件和转发关系时需要先知道个数
参数:无
返回值:是否成功
*/
uint8_t EDP_GetCCCnt()
{
    int i = 0;
    PRIVATE_BOARD *pBoard = NULL;
    uint8_t ucCnt = 0;

    for(i = 0; i < EDP_GetPrivateBoardCnt(); i++)
    {
        pBoard = EDP_GetPrivateBoardPtrFromIndex(i);
        if(pBoard->iBoardType == BOARD_TYPE_CC)
        {
            ucCnt++;
        }
    }
    return ucCnt;
}

/*
描述:得到CPU板个数
参数:无
返回值:是否成功
*/
uint8_t EDP_GetCPUCnt()
{
    int i = 0;
    PRIVATE_BOARD *pBoard = NULL;
    uint8_t ucCnt = 0;

    for(i = 0; i < EDP_GetPrivateBoardCnt(); i++)
    {
        pBoard = EDP_GetPrivateBoardPtrFromIndex(i);
        if(pBoard->iBoardType != BOARD_TYPE_CC)
        {
            ucCnt++;
        }
    }
    return ucCnt;
}


/*
描述:得到当前CPU板的接收端口信息
参数:
addrIndex:索引
iNodeAddr:板件nodeAddr(外绕线用1、2区分主从)
ucType: 采样还是GOOSE类型 PORT_TRANSFOR_DATA_TYPE_GS or PORT_TRANSFOR_DATA_TYPE_SV
pNetInfo:返回网络端口
返回值:是否成功
*/
BOOL EDP_GetCPUConnectPort(int addrIndex, int iNodeAddr, uint8_t ucType, NET_INFO *pNetInfo)
{
    int i = 0;
    int j = 0;
    int boardcnt = 0;
    PRIVATE_BOARD *pBoard = NULL;
    BOARD_PORT_CONNECT *pPortConnect = NULL;

    boardcnt = EDP_GetPrivateBoardCnt();
    pBoard = EDP_GetPrivateBoardPtrFromIndex(0);

    for(i = 0; i < boardcnt; i++)
    {
        pBoard = EDP_GetPrivateBoardPtrFromIndex(i);
        if(pBoard == NULL)
            continue;

        if(pBoard->iBoardType != BOARD_TYPE_CC)
        {
            if(pBoard->iNodeAddr == iNodeAddr)
            {
                for(j = 0; j < pBoard->tPort.iConnectCnt; j++)
                {
                    pPortConnect = &(pBoard->tPort.pPortConnect[j]);
                    if(pPortConnect->iDataType == ucType || pPortConnect->iDataType == PORT_TRANSFOR_DATA_TYPE_SVGS)
                    {
                        pNetInfo->addr[addrIndex].portnum[pNetInfo->addr[addrIndex].portcount] = pPortConnect->iPortId;
                        pNetInfo->addr[addrIndex].portcount++;
                    }
                }
                break;
            }
        }
    }
    return TRUE;
}


/*
描述:根据CPU的发送端口得到是哪一个CC端口的对侧的OutputId与之一致
参数:
pCpuBoardPort: CPU的发送端口
pCcResultBoardPort: 返回哪一个CC端口的对侧的OutputId与pCpuBoardPort一致
返回值:是否成功
*/
BOOL EDP_GetCCPortInfo(char *pCpuBoardPort, char *pCcResultBoardPort)
{
    int i = 0;
    int j = 0;
    PRIVATE_BOARD *pBoard = NULL;
    BOARD_PORT_CONNECT *pPortConnect = NULL;

    if(pCcResultBoardPort == NULL || pCpuBoardPort== NULL)
        return FALSE;

    for(i = 0; i < EDP_GetPrivateBoardCnt(); i++)
    {
        pBoard = EDP_GetPrivateBoardPtrFromIndex(i);
        if(pBoard->iBoardType == BOARD_TYPE_CC)
        {
            for(j = 0; j < pBoard->tPort.iConnectCnt; j++)
            {
                pPortConnect = &(pBoard->tPort.pPortConnect[j]);
                /*logMsg("EDP_GetCCPortInfo:  %s  %s  \n",pPortConnect->pOutPortId,pCpuBoardPort,0,0,0,0);*/
                /*因为是单向配置即可，所以只比较BOARD ID即可，不比较全部的OutPortId*/
                if(memcmp(pPortConnect->pOutPortId,pCpuBoardPort,1) == 0)
                {
                    /**/
                    sprintf(pCcResultBoardPort,"%d-%c", pBoard->iBoardId, (int)(pPortConnect->iPortId + (int)'A'));
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}


/*
描述: 释放内存,Devcfg
参数:    NONE
返回值: 释放是否成功
 */
int EDP_FreeDevCfg()
{
    EP_STATUS res = EP_SUCCESS;
    int i,j;

    for(i = 0; i < g_tPrivateCfg.iBoardCnt; i++)
    {
        free(g_tPrivateCfg.pBoard[i].pBoardDesc);
        g_tPrivateCfg.pBoard[i].pBoardDesc=NULL;
        for(j = 0; j < g_tPrivateCfg.pBoard[i].tPort.iConnectCnt; j++)
        {
            free(g_tPrivateCfg.pBoard[i].tPort.pPortConnect[j].pGsCbIndex);
            g_tPrivateCfg.pBoard[i].tPort.pPortConnect[j].pGsCbIndex=NULL;
            free(g_tPrivateCfg.pBoard[i].tPort.pPortConnect[j].pOutPortId);
            g_tPrivateCfg.pBoard[i].tPort.pPortConnect[j].pOutPortId=NULL;
        }
        free(g_tPrivateCfg.pBoard[i].tPort.pPortConnect);
        g_tPrivateCfg.pBoard[i].tPort.pPortConnect=NULL;
    }
    free(g_tPrivateCfg.pBoard);
    g_tPrivateCfg.pBoard=NULL;

    return res;
}


/*
描述:调试函数-打印出private.xml 配置信息
参数:无
返回值:TRUE
*/
int EDP_ShowPrivateXml()
{
    int iTmp = 0;
    int i = 0;
    int j = 0;
    PRIVATE_BOARD *pBoardInfo = NULL;
    BOARD_PORT_CONNECT *pPortConnect = NULL;
    /*BOARD_SUBSMV *pSubSmv = NULL;
    BOARD_PUBSMV *pPubSmv = NULL;
    BOARD_SUBGOOSE *pSubGse = NULL;
    BOARD_PUBGOOSE *pPubGse = NULL;
    BOARD_SUBSMV_CHANNEL *pChannel = NULL;

    BOARD_SMV_COMMON *pSmvCommon = NULL; */


    printf("\n<-------------------------开始打印----------------------------->^_^\n");

    printf("PrivateCfg deviceType: %d , LineType:%d, Board Cnt:%d \n",g_tPrivateCfg.iDeviceType,g_tPrivateCfg.iConnectLineType,g_tPrivateCfg.iBoardCnt);

    iTmp = g_tPrivateCfg.iBoardCnt;
    for(i = 0; i < iTmp; i++)
    {
        pBoardInfo = &(g_tPrivateCfg.pBoard[i]);
        printf("Board 序号:%d , ID:%d, iBoardType:%d , iParentCCId: %d , iSubDevId: %d, iNodeAddr:%d, pBoardDesc:%s \n",
               pBoardInfo->iIndex,
               pBoardInfo->iBoardId,
               pBoardInfo->iBoardType,
               pBoardInfo->iParentCCId,
               pBoardInfo->iSubDevId,
               pBoardInfo->iNodeAddr,
               pBoardInfo->pBoardDesc);

        /*port*/
        printf("\tPort信息,个数%d \n",pBoardInfo->tPort.iConnectCnt);
        for(j = 0; j < pBoardInfo->tPort.iConnectCnt; j++)
        {
            pPortConnect = &(pBoardInfo->tPort.pPortConnect[j]);

            printf("\t\tindex:%d, iPortId:%d, iDataType:%d, pOutPortId:%s, iSvPubRate:%d, iGsParentCpuType:%d \n",
                   pPortConnect->iIndex,
                   pPortConnect->iPortId,
                   pPortConnect->iDataType,
                   pPortConnect->pOutPortId,
                   pPortConnect->iSvPubRate,
                   pPortConnect->iGsParentCpuType);
            printf("\t\taSvMacAddr: %02x-%02x-%02x-%02x-%02x-%02x\n",
                   pPortConnect->aSvMacAddr[0],
                   pPortConnect->aSvMacAddr[1],
                   pPortConnect->aSvMacAddr[2],
                   pPortConnect->aSvMacAddr[3],
                   pPortConnect->aSvMacAddr[4],
                   pPortConnect->aSvMacAddr[5]);

        }

        /*SMV*/

        /*GOOSE*/
    }
    printf("\n<-------------------------打印结束----------------------------->^_^\n");
    return TRUE;
}

