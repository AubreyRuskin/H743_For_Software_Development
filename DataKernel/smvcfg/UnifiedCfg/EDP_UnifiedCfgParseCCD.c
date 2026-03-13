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
#include "EDP_UnifiedCfgParseCCD.h"
#include "EDP_UnifiedCfgParseConfig.h"
#include "EDP_UnifiedCfgFile.h"
#include "EDP_UnifiedCfgMain.h"
#include "edp_asst.h"
#include "filetool.h"

#define POLYNOMIAL_TMP (uint32_t)0xEDB88320
static uint32_t aulTbl32_g_TMP[256];

PROCESS_CFG g_tProcessBusCfg;		/*过程层配置结构*/
uint32_t g_ulCcdFileCheckCrc = 0; /* 通过计算得出的CCD文件校验码 */

BOOL g_bCfgLog = TRUE;
BOOL g_bCfgLogUtf8 = TRUE;

extern BOOL g_bCcIsUsed[MAX_CC_BOARD_ID_NUM]; /*以BoardId为序号，表示当前CC是否使用*/
extern char strFuncOptShow[32];

/*
描述: 得到XML元素下的子元素个数
参数:
GOOSE SUB的XML指针.
pSubGooseGcb: sub的结构
返回值: 解析是否成功
 */
int EDP_GetXmlElementCnt(mxml_node_t *pRoot, char *elementName)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    mxml_node_t *node1=NULL;

    if(elementName == NULL)
        return res;

    root_node = pRoot;
    node1 = mxmlFindElement(root_node, root_node, elementName,NULL, NULL,MXML_DESCEND);
    while(node1 != NULL)
    {
        res++;
        node1 = mxmlFindElement(node1, root_node, elementName,NULL, NULL,MXML_DESCEND);
    }

    return res;
}



/*
描述: 得到XML元素下的子元素个数
参数:
pRoot: intAddr的元素指针.
返回值: DAI元素的指针
 */
mxml_node_t* EDP_GetXmlDAIElement(mxml_node_t *pRoot)
{
    mxml_node_t *root_node=NULL;
    mxml_node_t *node1=NULL;
    /*mxml_node_t *node2=NULL;
    mxml_node_t *node3=NULL;*/
    mxml_node_t *nodeRes=NULL;
    char *p;

    if(pRoot == NULL)
        return NULL;

    root_node = pRoot;
    node1 = mxmlFindElement(root_node, root_node, EDP_CCD_DAI_NAME,NULL, NULL,MXML_DESCEND);
    while(node1 != NULL)
    {
        p=(char *)mxmlElementGetAttr(node1,"sAddr");
        if(p)
        {
            nodeRes = node1;
            break;
        }
        node1 = mxmlFindElement(node1, root_node, EDP_CCD_DAI_NAME,NULL, NULL,MXML_DESCEND);
    }

#if 0
    /*intaddr或者FCDA下直接是DAI*/
    root_node = pRoot;
    node1 = mxmlFindElement(root_node, root_node, EDP_CCD_DAI_NAME,NULL, NULL,MXML_DESCEND);
    if(node1 != NULL)
    {
        return node1;
    }

    /*搜索DOI */
    node1 = mxmlFindElement(root_node, root_node, "DOI",NULL, NULL,MXML_DESCEND);
    if(node1 == NULL)
    {
        return NULL;
    }
    /*搜索SDI */
    node2 = mxmlFindElement(node1, node1, "SDI",NULL, NULL,MXML_DESCEND);
    while(node2 != NULL)
    {
        node3 = mxmlFindElement(node2, node2, EDP_CCD_DAI_NAME,NULL, NULL,MXML_DESCEND);
        if(node3 == NULL)
            continue;
        p=(char *)mxmlElementGetAttr(node3,"sAddr");
        if(p)
        {
            nodeRes = node3;
            break;
        }
        node2 = mxmlFindElement(node2, node1, "SDI",NULL, NULL,MXML_DESCEND);
    }
#endif
    return nodeRes;
}


/*
描述: 解析CCD文件的GOOSE SUB的GseControl.
参数:
GOOSE SUB的XML指针.
pSubGooseGcb: sub的结构
返回值: 解析是否成功
 */
int EDP_ParseGooseSubGseControl(mxml_node_t *pNode, SUB_GCB_INFO *pSubGooseGcb)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    char *p;
    SUB_GCB_INFO *pGooseGcb = NULL;
    uint8_t strInfo[TEMP_INFO_MAX_LEN];         /* 用于生成记录日志信息 */

    pGooseGcb = pSubGooseGcb;
    root_node = pNode;

    p=(char *)mxmlElementGetAttr(root_node,"appID");
    if(p)
    {
        pGooseGcb->tGseCtrl.pAppID = strdup(p);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        res |= EDP_CONFIG_NECESSARY_PARA_LOST;
        sprintf(strInfo, "GOOSESUB中name为%s的GOCBref缺少appID信息\n", pGooseGcb->pGOCBrefName);
        CFG_LOG(strInfo,1,2,3,4,5,6);
        LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
    }

    p=(char *)mxmlElementGetAttr(root_node,"confRev");
    if(p)
    {
        pGooseGcb->tGseCtrl.iConfRev = strtol(p,NULL,10);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        res |= EDP_CONFIG_NECESSARY_PARA_LOST;
        sprintf(strInfo, "GOOSESUB中name为%s的GOCBref缺少confRev信息\n", pGooseGcb->pGOCBrefName);
        CFG_LOG(strInfo,1,2,3,4,5,6);
        LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
    }

    p=(char *)mxmlElementGetAttr(root_node,"datSet");
    if(p)
    {
        pGooseGcb->tGseCtrl.pDatSet = strdup(p);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        res |= EDP_CONFIG_NECESSARY_PARA_LOST;
        sprintf(strInfo, "GOOSESUB中name为%s的GOCBref缺少datSet信息\n", pGooseGcb->pGOCBrefName);
        CFG_LOG(strInfo,1,2,3,4,5,6);
        LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
    }

    p=(char *)mxmlElementGetAttr(root_node,"name");
    if(p)
    {
        pGooseGcb->tGseCtrl.pCtrlName = strdup(p);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
    }

    p=(char *)mxmlElementGetAttr(root_node,"type");
    if(p)
    {
        pGooseGcb->tGseCtrl.pCtrlType = strdup(p);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
    }

    return res;
}

/*
描述: 解析CCD文件的GOOSE SUB的ConnectAp.
参数:
GOOSE SUB的XML指针.
pSubGooseGcb: sub的结构
返回值: 解析是否成功
 */
int EDP_ParseGooseSubConnectAp(mxml_node_t *pNode, SUB_GCB_INFO *pSubGooseGcb)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    mxml_node_t *node1=NULL;
    mxml_node_t *node2=NULL;
    mxml_node_t *node3=NULL;
    char *p;
    GSE_AP_GSE_ADDR *pGseAddr = NULL;
    SUB_GCB_INFO *pGooseGcb = NULL;
    int i = 0;
    int k = 0;
    uint8_t ucAddrNum = 0;
    char *pAddrValue = NULL;
    char *pAddrType = NULL;
    char *tokenPtr = NULL;

    uint8_t strInfo[TEMP_INFO_MAX_LEN];         /* 用于生成记录日志信息 */
    BOOL bGetParaSuccess[4] = {0,0,0,0};        /* 用于记录获取必需属性成功情况 */
    enum {MAC_ADDR, APPID, VLAN_PRI, VLAN_ID};  /* 用于区分不同的四种必需属性 */

    pGooseGcb = pSubGooseGcb;
    root_node = pNode;

    p=(char *)mxmlElementGetAttr(root_node,"apName");
    if(p)
    {
        pGooseGcb->tConnectAp.pApName = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;*/
    }

    p=(char *)mxmlElementGetAttr(root_node,"iedName");
    if(p)
    {
        pGooseGcb->tConnectAp.pApIedName = strdup(p);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
    }


    node1 = mxmlFindElement(root_node, root_node, "GSE",NULL, NULL,MXML_DESCEND);
    if(node1 == NULL)
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        goto exit;
    }

    p=(char *)mxmlElementGetAttr(node1,"cbName");
    if(p)
    {
        pGooseGcb->tConnectAp.tConnectApGse.pCbName = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;*/
    }

    p=(char *)mxmlElementGetAttr(node1,"ldInst");
    if(p)
    {
        pGooseGcb->tConnectAp.tConnectApGse.pLdInst = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;*/
    }

    ucAddrNum = EDP_GetXmlElementCnt(node1, "Address");
    pGooseGcb->tConnectAp.tConnectApGse.iAddrCnt = ucAddrNum;

    if(ucAddrNum == 0)
    {
        CFG_LOG("未配置GOOSE  pub的Address信息,PUB GCB name: %s,%d \n",(int)pGooseGcb->pGOCBrefName,pGooseGcb->iIndex,0,0,0,0);
        res = EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        goto exit;
    }

    root_node = pNode;
    node2 = mxmlFindElement(node1, node1, "Address",NULL, NULL,MXML_DESCEND);
    while(node2 != NULL)
    {
        pGseAddr = &(pGooseGcb->tConnectAp.tConnectApGse.pGseAddr[i]);
        pGseAddr->iIndex = i;

        for(node3 = mxmlFindElement(node2, node2, "P",NULL, NULL,MXML_DESCEND);
                node3!=NULL;
                node3 = mxmlFindElement(node3, node2, "P",NULL, NULL,MXML_DESCEND))
        {

            pAddrType = (char *)mxmlElementGetAttr(node3, "type");
            pAddrValue = (char *)mxmlElementGetValue(node3);
            if((!pAddrType) || (!pAddrValue))
            {
                CFG_LOG("GOOSEPUB Address解析错误,PUB GCB name: %s,%d \n",(int)pGooseGcb->pGOCBrefName,i,0,0,0,0);
                res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
                goto exit;
            }

            if(strcmp(pAddrType, "MAC-Address") == 0)
            {
                bGetParaSuccess[MAC_ADDR]=TRUE;
                tokenPtr=strtok(strdup(pAddrValue),"-");
                for(k = 0; (k < 6) && (tokenPtr != NULL); k++)
                {
                    pGseAddr->aDstMac[k] = strtol(tokenPtr,NULL,16);
                    tokenPtr = strtok(NULL,"-");
                }

                if((pGseAddr->aDstMac[0] != 0x01) || (pGseAddr->aDstMac[1] != 0x0C)
                        || (pGseAddr->aDstMac[2] != 0xCD) || (pGseAddr->aDstMac[3] != 0x01))
                {
                    res |= EDP_CONFIG_PARA_OVERFLOW;
                    sprintf(strInfo, "GOOSESUB中name为%s的GOCBref中MAC-Address越限\n", pGooseGcb->pGOCBrefName);
                    CFG_LOG(strInfo,1,2,3,4,5,6);
                    LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
                }
            }
            else if(strcmp(pAddrType, "VLAN-ID") == 0)
            {
                bGetParaSuccess[VLAN_ID]=TRUE;
                pGseAddr->usVID = strtol(pAddrValue,NULL,16);

                if(pGseAddr->usVID > 0xFFE)
                {
                    res |= EDP_CONFIG_PARA_OVERFLOW;
                    sprintf(strInfo, "GOOSESUB中name为%s的GOCBref中VLAN-ID越限\n", pGooseGcb->pGOCBrefName);
                    CFG_LOG(strInfo,1,2,3,4,5,6);
                    LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
                }
            }
            else if(strcmp(pAddrType, "VLAN-PRIORITY") == 0)
            {
                bGetParaSuccess[VLAN_PRI]=TRUE;
                pGseAddr->usVPri= strtol(pAddrValue,NULL,10);

                if(pGseAddr->usVPri > 0x07)
                {
                    res |= EDP_CONFIG_PARA_OVERFLOW;
                    sprintf(strInfo, "GOOSESUB中name为%s的GOCBref中VLAN-PRIORITY越限\n", pGooseGcb->pGOCBrefName);
                    CFG_LOG(strInfo,1,2,3,4,5,6);
                    LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
                }
            }
            else if(strcmp(pAddrType, "APPID") == 0)
            {
                bGetParaSuccess[APPID]=TRUE;
                pGseAddr->usAppID = strtol(pAddrValue,NULL,16);

                if(pGseAddr->usAppID > 0x3FFF)
                {
                    res |= EDP_CONFIG_PARA_OVERFLOW;
                    sprintf(strInfo, "GOOSESUB中name为%s的GOCBref中APPID越限\n", pGooseGcb->pGOCBrefName);
                    CFG_LOG(strInfo,1,2,3,4,5,6);
                    LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
                }
            }
        }

        if(!bGetParaSuccess[MAC_ADDR])
        {
            res |= EDP_CONFIG_NECESSARY_PARA_LOST;
            sprintf(strInfo, "GOOSESUB中name为%s的GOCBref缺少MAC-Address信息\n", pGooseGcb->pGOCBrefName);
            CFG_LOG(strInfo,1,2,3,4,5,6);
            LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
        }
        if(!bGetParaSuccess[VLAN_ID])
        {
            res |= EDP_CONFIG_NECESSARY_PARA_LOST;
            sprintf(strInfo, "GOOSESUB中name为%s的GOCBref缺少VLAN-ID信息\n", pGooseGcb->pGOCBrefName);
            CFG_LOG(strInfo,1,2,3,4,5,6);
            LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
        }
        if(!bGetParaSuccess[VLAN_PRI])
        {
            res |= EDP_CONFIG_NECESSARY_PARA_LOST;
            sprintf(strInfo, "GOOSESUB中name为%s的GOCBref缺少VLAN-PRIORITY信息\n", pGooseGcb->pGOCBrefName);
            CFG_LOG(strInfo,1,2,3,4,5,6);
            LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
        }
        if(!bGetParaSuccess[APPID])
        {
            res |= EDP_CONFIG_NECESSARY_PARA_LOST;
            sprintf(strInfo, "GOOSESUB中name为%s的GOCBref缺少APPID信息\n", pGooseGcb->pGOCBrefName);
            CFG_LOG(strInfo,1,2,3,4,5,6);
            LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
        }
        i++;
        node2 = mxmlFindElement(node2, node1, "Address",NULL, NULL,MXML_DESCEND);
    }


    node2 = mxmlFindElement(node1, node1, "MinTime",NULL, NULL,MXML_DESCEND);
    p=(char *)mxmlElementGetValue(node2);
    if(p)
    {
        pGooseGcb->tConnectAp.tConnectApGse.tGseTime.iMinTime = strtol(p,NULL,10);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
    }

    node2 = mxmlFindElement(node1, node1, "MaxTime",NULL, NULL,MXML_DESCEND);
    p=(char *)mxmlElementGetValue(node2);
    if(p)
    {
        pGooseGcb->tConnectAp.tConnectApGse.tGseTime.iMaxTime = strtol(p,NULL,10);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
    }

exit:
    return res;
}

/*
描述: 解析CCD文件的GOOSE SUB的Dataset.
参数:
GOOSE SUB的XML指针.
pSubGooseGcb: sub的结构
返回值: 解析是否成功
 */
int EDP_ParseGooseSubDataset(mxml_node_t *pNode, SUB_GCB_INFO *pSubGooseGcb)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    mxml_node_t *node1=NULL;
    mxml_node_t *node2=NULL;
    mxml_node_t *node3=NULL;
    char *p;
    SUB_GCB_INFO *pGooseGcb = NULL;
    int i = 0;
    int j = 0;
    uint8_t ucFcdaNum = 0;
    uint8_t ucIntAddrNum = 0;
    GSE_DATESET_FCDA_SUB *pGooseFcda = NULL;
    GSE_DATESET_FCDA_INTADDR *pIntAddr = NULL;
    char Tmp[16];
    int iBoardId = 0;

    char cIntAddrNameTmp[128];                          /* 存放用于寻找Q或者T关联硬件地址的字符串 */
    GSE_DATESET_FCDA_SUB *pFindQTGooseFcda = NULL;      /* 用于寻找品质或者时标关联硬件地址的Fcda指针 */
    GSE_DATESET_FCDA_INTADDR *pFindQTIntAddr = NULL;    /* 用于寻找品质或者时标关联硬件地址的IntAddr指针 */
    int iFindQTIntAddrCnt = 0;                          /* 用于寻找品质或者时标关联硬件地址的IntAddr计数 */

    pGooseGcb = pSubGooseGcb;
    root_node = pNode;

    p=(char *)mxmlElementGetAttr(root_node,"name");
    if(p)
    {
        pGooseGcb->tDataSet.pDateSetName = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;*/
    }

    ucFcdaNum = EDP_GetXmlElementCnt(root_node, "FCDA");
    pGooseGcb->tDataSet.iFcdaCnt = ucFcdaNum;

    if(ucFcdaNum == 0)
    {
        CFG_LOG("未配置GOOSE  sub的FCDA个数为0,SUB GCB name: %s,%d \n",(int)pGooseGcb->pGOCBrefName,pGooseGcb->iIndex,0,0,0,0);
        res = EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        goto exit;
    }

    /*分配空间*/
    pGooseGcb->tDataSet.pDataSetFcda = (GSE_DATESET_FCDA_SUB *)calloc(ucFcdaNum, sizeof(GSE_DATESET_FCDA_SUB));
    if(!pGooseGcb->tDataSet.pDataSetFcda)
    {
        CFG_LOG("#### kevin: CCD GOOSE SUB FCDA分配空间失败,FCDA个数:%d,GCBindex: %d \n",ucFcdaNum,pGooseGcb->iIndex,0,0,0,0);
        res = EDP_CCD_PARSE_ERR_FILE_CALLOC_FAIL;
        goto exit;
    }

    root_node = pNode;
    node1 = mxmlFindElement(root_node, root_node, "FCDA",NULL, NULL,MXML_DESCEND);
    while(node1 != NULL)
    {
        pGooseFcda = &(pGooseGcb->tDataSet.pDataSetFcda[i]);
        if(i < pGooseGcb->tDataSet.iFcdaCnt-1)
        {
            pGooseFcda->next = &(pGooseGcb->tDataSet.pDataSetFcda[i+1]);
        }
        else
        {
            pGooseFcda->next = NULL;
        }
        pGooseFcda->iIndex = i;

        p=(char *)mxmlElementGetAttr(node1,"bType");
        if(p)
        {
            pGooseFcda->pBType = strdup(p);
        }
        else
        {
            res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        }

        p=(char *)mxmlElementGetAttr(node1,"daName");
        if(p)
        {
            pGooseFcda->pDaName = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"doName");
        if(p)
        {
            pGooseFcda->pDoName = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"fc");
        if(p)
        {
            pGooseFcda->pFC = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"ldInst");
        if(p)
        {
            pGooseFcda->pLdInst = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"lnClass");
        if(p)
        {
            pGooseFcda->pLnClass = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"lnInst");
        if(p)
        {
            pGooseFcda->pLnInst = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"prefix");
        if(p)
        {
            pGooseFcda->pPrefix = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"desc");
        if(p)
        {
            pGooseFcda->pFcdaDesc = strdup(p);
        }

        ucIntAddrNum = EDP_GetXmlElementCnt(node1, "intAddr");
        pGooseFcda->iIntAddrCnt = ucIntAddrNum;
        pGooseFcda->pIntAddr = (GSE_DATESET_FCDA_INTADDR *)calloc(ucIntAddrNum,sizeof(GSE_DATESET_FCDA_INTADDR));
        if(!pGooseFcda->pIntAddr)
        {
            CFG_LOG("#### kevin: CCD GOOSE SUB FCDA下的intAddr 分配空间失败,FCDA个数:%d,intAddr个数:%d, GCBindex: %d \n",ucFcdaNum,ucIntAddrNum,pGooseGcb->iIndex,0,0,0);
            res = EDP_CCD_PARSE_ERR_FILE_CALLOC_FAIL;
            goto exit;
        }

        for(j = 0, node2 = mxmlFindElement(node1, node1, "intAddr",NULL, NULL,MXML_DESCEND);
                node2!=NULL;
                j++, node2 = mxmlFindElement(node2, node1, "intAddr",NULL, NULL,MXML_DESCEND))
        {
            pIntAddr = &(pGooseFcda->pIntAddr[j]);

            p=(char *)mxmlElementGetAttr(node2,"desc");
            if(p)
            {
                pIntAddr->pIntaddrDesc = strdup(p);
            }

            p=(char *)mxmlElementGetAttr(node2,"name");
            if(p)
            {
                pIntAddr->pIntaddrName = strdup(p);
                if(!(memcmp(pIntAddr->pIntaddrName,EDP_CCD_INTADDR_INVALID_NAME,sizeof(EDP_CCD_INTADDR_INVALID_NAME))))
                {
                    pIntAddr->bIsIntAddrValid = FALSE;
                }
                else
                {
                    pIntAddr->bIsIntAddrValid = TRUE;
                    if(pSubGooseGcb->pIntAddrName == NULL)
                    {
                        GetEnumItem_Div(pIntAddr->pIntaddrName,0,Tmp,':',sizeof(Tmp));
                        pSubGooseGcb->pIntAddrName = strdup(Tmp);
                        iBoardId = EDP_GetBoardId(pSubGooseGcb->pIntAddrName);
                        if(iBoardId > 0)
                        {
                            g_bCcIsUsed[iBoardId] = TRUE;
                        }
                    }
                }
            }
            else
            {
                res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
                CFG_LOG("读取FCDA 中intAdd名称不允许为空,SUB GCB name: %s,%d,FCDA index:%d \n",
                        (int)pGooseGcb->pGOCBrefName,i,pGooseFcda->iIndex,0,0,0);
                goto exit;
            }

            if(pIntAddr->bIsIntAddrValid)
            {
                node3 = EDP_GetXmlDAIElement(node2);
                if(node3 == NULL)
                {
#if 0 /* 规范中没有要求必须配置DAI */
                    pIntAddr->bIsDaiValid = FALSE;
                    CFG_LOG("读取FCDA 中的intAddr 中的DAI出错,SUB GCB name: %s,%d\n",(int)pGooseGcb->pGOCBrefName,i,0,0,0,0);
                    res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
#endif
                    /* 根据保测一体的配置,时标和品质关联后不会生成DAI时,需要通过IntAddr的name,寻找相应的物理地址 */
                    if((0 == strcmp(pGooseFcda->pBType, "Timestamp")) || (0 == strcmp(pGooseFcda->pBType, "Quality")))
                    {
                        if(GetEnumItem_Div_All(pIntAddr->pIntaddrName,1,cIntAddrNameTmp,'.',128) != 0)
                        {
                            pFindQTGooseFcda = pGooseGcb->tDataSet.pDataSetFcda;
                            while(pFindQTGooseFcda != NULL)
                            {
                                for(iFindQTIntAddrCnt=0; iFindQTIntAddrCnt<pFindQTGooseFcda->iIntAddrCnt; iFindQTIntAddrCnt++)
                                {
                                    pFindQTIntAddr = &pFindQTGooseFcda->pIntAddr[iFindQTIntAddrCnt];
                                    if(strstr(pFindQTIntAddr->pIntaddrName,cIntAddrNameTmp) != NULL)
                                    {/* 找到后复制相同的SADDR值 */                                    
                                        pIntAddr->bIsDaiValid = TRUE;
                                        pIntAddr->tDai.pDaiName = strdup("TEMP");/* 解析过程中此字符串无意义仅作填充 */
                                        pIntAddr->tDai.pSAddr = strdup(pFindQTIntAddr->tDai.pSAddr);
                                        break;
                                    }
                                }
                                
                                if(iFindQTIntAddrCnt != pFindQTGooseFcda->iIntAddrCnt)
                                {
                                    break;
                                }
                                
                                pFindQTGooseFcda = pFindQTGooseFcda->next;
                            }
                        }
                    }
                }
                else
                {
                    pIntAddr->bIsDaiValid = TRUE;
                    p=(char *)mxmlElementGetAttr(node3,"name");
                    if(p)
                    {
                        pIntAddr->tDai.pDaiName = strdup(p);
                    }
                    else
                    {
                        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
                    }

                    p=(char *)mxmlElementGetAttr(node3,"sAddr");
                    if(p)
                    {
                        pIntAddr->tDai.pSAddr = strdup(p);
                    }
                    else
                    {
                        CFG_LOG("读取FCDA 中的intAddr 中的DAI saaddr 出错,SUB GCB name: %s,%d,FCDA index:%d\n",
                                (int)pGooseGcb->pGOCBrefName,i,pGooseFcda->iIndex,0,0,0);
                        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
                        goto exit;
                    }
                }
            }
        }

        i++;
        node1 = mxmlFindElement(node1, root_node, "FCDA",NULL, NULL,MXML_DESCEND);
    }
exit:
    return res;
}

/*
描述: 解析CCD文件的GOOSE SUB.
参数:     GOOSE SUB的XML指针.
返回值: 解析是否成功
 */
int EDP_ParseGooseSub(mxml_node_t *pNode)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    mxml_node_t *node1=NULL;
    mxml_node_t *node2=NULL;
    char *p;
    uint8_t ucGseNum = 0;
    PROCESS_SUB_GSE *pGoose = NULL;
    SUB_GCB_INFO *pGooseGcb = NULL;
    int i = 0;

    root_node = pNode;

    /*获取SUB GOOSE 个数*/
    ucGseNum = EDP_GetXmlElementCnt(root_node, "GOCBref");
    if(ucGseNum == 0)
    {
        goto exit;
    }

    pGoose = &(g_tProcessBusCfg.tProSubGoose);
    /*GOOSE SUB个数*/
    pGoose->iSubGsCnt = ucGseNum;

    /*分配空间*/
    pGoose->pSubGcbInfo = (SUB_GCB_INFO *)calloc(ucGseNum, sizeof(SUB_GCB_INFO));
    if(!pGoose->pSubGcbInfo)
    {
        CFG_LOG("#### kevin: CCD GOOSE SUB分配空间失败,gcb个数:%d \n",ucGseNum,0,0,0,0,0);
        res = EDP_CCD_PARSE_ERR_FILE_CALLOC_FAIL;
        goto exit;
    }

    /*解析SUB GOOSE*/
    root_node = pNode;
    node1 = mxmlFindElement(root_node, root_node, "GOCBref",NULL, NULL,MXML_DESCEND);
    while(node1 != NULL)
    {
        pGooseGcb = &(pGoose->pSubGcbInfo[i]);
        pGooseGcb->next = &(pGoose->pSubGcbInfo[i+1]);
        pGooseGcb->iIndex = i;
        p=(char *)mxmlElementGetAttr(node1,"name");
        if(p)
        {
            pGooseGcb->pGOCBrefName = strdup(p);
        }
        else
        {
            res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        }

        /*解析GSEControl*/
        node2 = mxmlFindElement(node1, node1, "GSEControl",NULL, NULL,MXML_DESCEND);
        if(node2)
        {
            res |= EDP_ParseGooseSubGseControl(node2,pGooseGcb);
        }
        else
        {
            res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        }

        /*解析ConnectedAP*/
        node2 = mxmlFindElement(node1, node1, "ConnectedAP",NULL, NULL,MXML_DESCEND);
        if(node2)
        {
            res |= EDP_ParseGooseSubConnectAp(node2,pGooseGcb);
        }
        else
        {
            res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        }

        /*解析DataSet*/
        node2 = mxmlFindElement(node1, node1, "DataSet",NULL, NULL,MXML_DESCEND);
        if(node2)
        {
            res |= EDP_ParseGooseSubDataset(node2,pGooseGcb);
        }
        else
        {
            res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        }

        i++;
        node1 = mxmlFindElement(node1, root_node, "GOCBref",NULL, NULL,MXML_DESCEND);
    }

exit:
    return res;
}

/*
描述: 解析CCD文件的GOOSE SUB的GseControl.
参数:
GOOSE SUB的XML指针.
pSubGooseGcb: sub的结构
返回值: 解析是否成功
 */
int EDP_ParseGoosePubGseControl(mxml_node_t *pNode, PUB_GCB_INFO *pPubGooseGcb)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    char *p;
    PUB_GCB_INFO *pGooseGcb = NULL;
    uint8_t strInfo[TEMP_INFO_MAX_LEN];         /* 用于生成记录日志信息 */

    pGooseGcb = pPubGooseGcb;
    root_node = pNode;

    p=(char *)mxmlElementGetAttr(root_node,"appID");
    if(p)
    {
        pGooseGcb->tGseCtrl.pAppID = strdup(p);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        res |= EDP_CONFIG_NECESSARY_PARA_LOST;
        sprintf(strInfo, "GOOSEPUB中name为%s的GOCBref缺少appID信息\n", pGooseGcb->pGOCBrefName);
        CFG_LOG(strInfo,1,2,3,4,5,6);
        LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
    }

    p=(char *)mxmlElementGetAttr(root_node,"confRev");
    if(p)
    {
        pGooseGcb->tGseCtrl.iConfRev = strtol(p,NULL,10);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        res |= EDP_CONFIG_NECESSARY_PARA_LOST;
        sprintf(strInfo, "GOOSEPUB中name为%s的GOCBref缺少confRev信息\n", pGooseGcb->pGOCBrefName);
        CFG_LOG(strInfo,1,2,3,4,5,6);
        LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
    }

    p=(char *)mxmlElementGetAttr(root_node,"datSet");
    if(p)
    {
        pGooseGcb->tGseCtrl.pDatSet = strdup(p);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        res |= EDP_CONFIG_NECESSARY_PARA_LOST;
        sprintf(strInfo, "GOOSEPUB中name为%s的GOCBref缺少datSet信息\n", pGooseGcb->pGOCBrefName);
        CFG_LOG(strInfo,1,2,3,4,5,6);
        LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
    }

    p=(char *)mxmlElementGetAttr(root_node,"name");
    if(p)
    {
        pGooseGcb->tGseCtrl.pCtrlName = strdup(p);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
    }

    p=(char *)mxmlElementGetAttr(root_node,"type");
    if(p)
    {
        pGooseGcb->tGseCtrl.pCtrlType = strdup(p);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
    }

    return res;
}

/*
描述: 解析CCD文件的GOOSE SUB的ConnectAp.
参数:
GOOSE SUB的XML指针.
pSubGooseGcb: sub的结构
返回值: 解析是否成功
 */
int EDP_ParseGoosePubConnectAp(mxml_node_t *pNode, PUB_GCB_INFO *pPubGooseGcb)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    mxml_node_t *node1=NULL;
    mxml_node_t *node2=NULL;
    mxml_node_t *node3=NULL;
    char *p;
    GSE_AP_GSE_ADDR *pGseAddr = NULL;
    PUB_GCB_INFO *pGooseGcb = NULL;
    int i = 0;
    int k = 0;
    uint8_t ucAddrNum = 0;
    uint8_t ucPhysCoonNum = 0;
    GSE_CONNECTAP_PHYSCONN *pGsePhyscoon = NULL;
    char *pAddrValue = NULL;
    char *pAddrType = NULL;
    char *tokenPtr = NULL;

    uint8_t strInfo[TEMP_INFO_MAX_LEN];         /* 用于生成记录日志信息 */
    BOOL bGetParaSuccess[4] = {0,0,0,0};                    /* 用于记录获取必需属性成功情况 */
    enum {MAC_ADDR, APPID, VLAN_PRI, VLAN_ID};  /* 用于区分不同的四种必需属性 */

    pGooseGcb = pPubGooseGcb;
    root_node = pNode;

    p=(char *)mxmlElementGetAttr(root_node,"apName");
    if(p)
    {
        pGooseGcb->tConnectAp.pApName = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;*/
    }

    p=(char *)mxmlElementGetAttr(root_node,"iedName");
    if(p)
    {
        pGooseGcb->tConnectAp.pApIedName = strdup(p);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
    }


    node1 = mxmlFindElement(root_node, root_node, "GSE",NULL, NULL,MXML_DESCEND);
    if(node1 == NULL)
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        goto exit;
    }

    p=(char *)mxmlElementGetAttr(node1,"cbName");
    if(p)
    {
        pGooseGcb->tConnectAp.tConnectApGse.pCbName = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;*/
    }

    p=(char *)mxmlElementGetAttr(node1,"ldInst");
    if(p)
    {
        pGooseGcb->tConnectAp.tConnectApGse.pLdInst = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;*/
    }

    ucAddrNum = EDP_GetXmlElementCnt(node1, "Address");
    pGooseGcb->tConnectAp.tConnectApGse.iAddrCnt = ucAddrNum;

    if(ucAddrNum == 0)
    {
        CFG_LOG("未配置GOOSE  pub的Address信息,PUB GCB name: %s,%d \n",(int)pGooseGcb->pGOCBrefName,pGooseGcb->iIndex,0,0,0,0);
        res = EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        goto exit;
    }

    root_node = pNode;
    node2 = mxmlFindElement(node1, node1, "Address",NULL, NULL,MXML_DESCEND);
    while(node2 != NULL)
    {
        pGseAddr = &(pGooseGcb->tConnectAp.tConnectApGse.pGseAddr[i]);
        pGseAddr->iIndex = i;
        memset(bGetParaSuccess, 0, 4);

        for(node3 = mxmlFindElement(node2, node2, "P",NULL, NULL,MXML_DESCEND);
                node3!=NULL;
                node3 = mxmlFindElement(node3, node2, "P",NULL, NULL,MXML_DESCEND))
        {
            pAddrType = (char *)mxmlElementGetAttr(node3, "type");
            pAddrValue = (char *)mxmlElementGetValue(node3);
            if((!pAddrType) || (!pAddrValue))
            {
                CFG_LOG("GOOSEPUB Address解析错误,PUB GCB name: %s,%d \n",(int)pGooseGcb->pGOCBrefName,i,0,0,0,0);
                res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
                goto exit;
            }

            if(strcmp(pAddrType, "MAC-Address") == 0)
            {
                bGetParaSuccess[MAC_ADDR]=TRUE;
                tokenPtr = strtok(strdup(pAddrValue),"-");
                for(k = 0; (k < 6) && (tokenPtr != NULL); k++)
                {
                    pGseAddr->aDstMac[k] = strtol(tokenPtr,NULL,16);
                    tokenPtr = strtok(NULL,"-");
                }

                if((pGseAddr->aDstMac[0] != 0x01) || (pGseAddr->aDstMac[1] != 0x0C)
                        || (pGseAddr->aDstMac[2] != 0xCD) || (pGseAddr->aDstMac[3] != 0x01))
                {
                    res |= EDP_CONFIG_PARA_OVERFLOW;
                    sprintf(strInfo, "GOOSEPUB中name为%s的GOCBref中MAC-Address越限\n", pGooseGcb->pGOCBrefName);
                    CFG_LOG(strInfo,1,2,3,4,5,6);
                    LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
                }
            }
            else if(strcmp(pAddrType, "VLAN-ID") == 0)
            {
                bGetParaSuccess[VLAN_ID]=TRUE;
                pGseAddr->usVID = strtol(pAddrValue,NULL,16);

                if(pGseAddr->usVID > 0xFFE)
                {
                    res |= EDP_CONFIG_PARA_OVERFLOW;
                    sprintf(strInfo, "GOOSEPUB中name为%s的GOCBref中VLAN-ID越限\n", pGooseGcb->pGOCBrefName);
                    CFG_LOG(strInfo,1,2,3,4,5,6);
                    LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
                }
            }
            else if(strcmp(pAddrType, "VLAN-PRIORITY") == 0)
            {
                bGetParaSuccess[VLAN_PRI]=TRUE;
                pGseAddr->usVPri= strtol(pAddrValue,NULL,10);

                if(pGseAddr->usVPri > 0x07)
                {
                    res |= EDP_CONFIG_PARA_OVERFLOW;
                    sprintf(strInfo, "GOOSEPUB中name为%s的GOCBref中VLAN-PRIORITY越限\n", pGooseGcb->pGOCBrefName);
                    CFG_LOG(strInfo,1,2,3,4,5,6);
                    LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
                }
            }
            else if(strcmp(pAddrType, "APPID") == 0)
            {
                bGetParaSuccess[APPID]=TRUE;
                pGseAddr->usAppID = strtol(pAddrValue,NULL,16);

                if(pGseAddr->usAppID > 0x3FFF)
                {
                    res |= EDP_CONFIG_PARA_OVERFLOW;
                    sprintf(strInfo, "GOOSEPUB中name为%s的GOCBref中APPID越限\n", pGooseGcb->pGOCBrefName);
                    CFG_LOG(strInfo,1,2,3,4,5,6);
                    LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
                }
            }
        }

        if(!bGetParaSuccess[MAC_ADDR])
        {
            res |= EDP_CONFIG_NECESSARY_PARA_LOST;
            sprintf(strInfo, "GOOSEPUB中name为%s的GOCBref缺少MAC-Address信息\n", pGooseGcb->pGOCBrefName);
            CFG_LOG(strInfo,1,2,3,4,5,6);
            LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
        }
        if(!bGetParaSuccess[VLAN_ID])
        {
            res |= EDP_CONFIG_NECESSARY_PARA_LOST;
            sprintf(strInfo, "GOOSEPUB中name为%s的GOCBref缺少VLAN-ID信息\n", pGooseGcb->pGOCBrefName);
            CFG_LOG(strInfo,1,2,3,4,5,6);
            LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
        }
        if(!bGetParaSuccess[VLAN_PRI])
        {
            res |= EDP_CONFIG_NECESSARY_PARA_LOST;
            sprintf(strInfo, "GOOSEPUB中name为%s的GOCBref缺少VLAN-PRIORITY信息\n", pGooseGcb->pGOCBrefName);
            CFG_LOG(strInfo,1,2,3,4,5,6);
            LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
        }
        if(!bGetParaSuccess[APPID])
        {
            res |= EDP_CONFIG_NECESSARY_PARA_LOST;
            sprintf(strInfo, "GOOSEPUB中name为%s的GOCBref缺少APPID信息\n", pGooseGcb->pGOCBrefName);
            CFG_LOG(strInfo,1,2,3,4,5,6);
            LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
        }

        i++;
        node2 = mxmlFindElement(node2, node1, "Address",NULL, NULL,MXML_DESCEND);
    }


    node2 = mxmlFindElement(node1, node1, "MinTime",NULL, NULL,MXML_DESCEND);
    p=(char *)mxmlElementGetValue(node2);
    if(p)
    {
        pGooseGcb->tConnectAp.tConnectApGse.tGseTime.iMinTime = strtol(p,NULL,10);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
    }

    node2 = mxmlFindElement(node1, node1, "MaxTime",NULL, NULL,MXML_DESCEND);
    p=(char *)mxmlElementGetValue(node2);
    if(p)
    {
        pGooseGcb->tConnectAp.tConnectApGse.tGseTime.iMaxTime = strtol(p,NULL,10);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
    }

    /*解析PhysCoon*/
    root_node = pNode;
    ucPhysCoonNum = EDP_GetXmlElementCnt(root_node, "PhysConn");
    pGooseGcb->tConnectAp.iPhysConnCnt = ucPhysCoonNum;
    if(ucPhysCoonNum == 0)
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
    }
    i = 0;
    node1 = mxmlFindElement(root_node, root_node, "PhysConn",NULL, NULL,MXML_DESCEND);
    while(node1 != NULL)
    {
        pGsePhyscoon = &(pGooseGcb->tConnectAp.tConnectApPhysConn[i]);
        pGsePhyscoon->iIndex = i;

        p=(char *)mxmlElementGetAttr(node1,"type");
        if(p)
        {
            pGsePhyscoon->pPhysType = strdup(p);
        }

        for(node2 = mxmlFindElement(node1, node1, "P",NULL, NULL,MXML_DESCEND);
                node2!=NULL;
                node2 = mxmlFindElement(node2, node1, "P",NULL, NULL,MXML_DESCEND))
        {
            pAddrType = (char *)mxmlElementGetAttr(node2, "type");
            pAddrValue = (char *)mxmlElementGetValue(node2);

            if((!pAddrType) || (!pAddrValue))
            {
                CFG_LOG("GOOSEPUB physcoon解析错误,PUB GCB name: %s,%d \n",(int)pGooseGcb->pGOCBrefName,i,0,0,0,0);
                res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
                /*goto exit;*/
            }

            if(strcmp(pAddrType, "Port") == 0)
            {
                pGsePhyscoon->pPort = strdup(pAddrValue);
            }
            else if(strcmp(pAddrType, "Plug") == 0)
            {
                pGsePhyscoon->pPlug = strdup(pAddrValue);
            }
            else if(strcmp(pAddrType, "Type") == 0)
            {
                pGsePhyscoon->pSpeedType = strdup(pAddrValue);
            }
            else if(strcmp(pAddrType, "Cable") == 0)
            {
                pGsePhyscoon->pCable = strdup(pAddrValue);
            }
        }

        i++;
        node1 = mxmlFindElement(node1, root_node, "PhysConn",NULL, NULL,MXML_DESCEND);
    }


exit:
    return res;
}

/*
描述: 解析CCD文件的GOOSE SUB的Dataset.
参数:
GOOSE SUB的XML指针.
pSubGooseGcb: sub的结构
返回值: 解析是否成功
 */
int EDP_ParseGoosePubDataset(mxml_node_t *pNode, PUB_GCB_INFO *pPubGooseGcb)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    mxml_node_t *node1=NULL;
    mxml_node_t *node2=NULL;
    char *p;
    PUB_GCB_INFO *pGooseGcb = NULL;
    int i = 0;
    uint8_t ucFcdaNum = 0;
    GSE_DATESET_FCDA_PUB *pGooseFcda = NULL;
    GSE_DATESET_FCDA_DAI *pDai = NULL;

    pGooseGcb = pPubGooseGcb;
    root_node = pNode;


    p=(char *)mxmlElementGetAttr(root_node,"name");
    if(p)
    {
        pGooseGcb->tDataSet.pDateSetName = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;*/
    }


    ucFcdaNum = EDP_GetXmlElementCnt(root_node, "FCDA");
    pGooseGcb->tDataSet.iFcdaCnt = ucFcdaNum;

    if(ucFcdaNum == 0)
    {
        CFG_LOG("未配置GOOSE  sub的FCDA个数为0,SUB GCB name: %s,%d \n",(int)pGooseGcb->pGOCBrefName,pGooseGcb->iIndex,0,0,0,0);
        res = EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        goto exit;
    }

    /*分配空间*/
    pGooseGcb->tDataSet.pDataSetFcda = (GSE_DATESET_FCDA_PUB *)calloc(ucFcdaNum, sizeof(GSE_DATESET_FCDA_PUB));
    if(!pGooseGcb->tDataSet.pDataSetFcda)
    {
        CFG_LOG("#### kevin: CCD GOOSE PUB FCDA分配空间失败,FCDA个数:%d,GCBindex: %d \n",ucFcdaNum,pGooseGcb->iIndex,0,0,0,0);
        res = EDP_CCD_PARSE_ERR_FILE_CALLOC_FAIL;
        goto exit;
    }

    root_node = pNode;
    node1 = mxmlFindElement(root_node, root_node, "FCDA",NULL, NULL,MXML_DESCEND);
    while(node1 != NULL)
    {
        pGooseFcda = &(pGooseGcb->tDataSet.pDataSetFcda[i]);
        pGooseFcda->next = &(pGooseGcb->tDataSet.pDataSetFcda[i+1]);
        pGooseFcda->iIndex = i;

        p=(char *)mxmlElementGetAttr(node1,"bType");
        if(p)
        {
            pGooseFcda->pBType = strdup(p);
        }
        else
        {
            res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        }

        p=(char *)mxmlElementGetAttr(node1,"daName");
        if(p)
        {
            pGooseFcda->pDaName = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"doName");
        if(p)
        {
            pGooseFcda->pDoName = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"fc");
        if(p)
        {
            pGooseFcda->pFC = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"ldInst");
        if(p)
        {
            pGooseFcda->pLdInst = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"lnClass");
        if(p)
        {
            pGooseFcda->pLnClass = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"lnInst");
        if(p)
        {
            pGooseFcda->pLnInst = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"prefix");
        if(p)
        {
            pGooseFcda->pPrefix = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"desc");
        if(p)
        {
            pGooseFcda->pFcdaDesc = strdup(p);
        }

        pDai = &(pGooseFcda->tDai);
        node2 = EDP_GetXmlDAIElement(node1);
        if(node2 == NULL)
        {
            pGooseFcda->bIsDaiValid = FALSE;
        }
        else
        {
            pGooseFcda->bIsDaiValid = TRUE;
            p=(char *)mxmlElementGetAttr(node2,"name");
            if(p)
            {
                pDai->pDaiName = strdup(p);
            }
            else
            {
                res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
            }

            p=(char *)mxmlElementGetAttr(node2,"sAddr");
            if(p)
            {
                pDai->pSAddr = strdup(p);
            }
            else
            {
                CFG_LOG("读取FCDA 中的DAI saaddr 出错,PUB GCB name: %s,%d,FCDA index:%d\n",
                        (int)pGooseGcb->pGOCBrefName,i,pGooseFcda->iIndex,0,0,0);
                res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
                goto exit;
            }
        }


        i++;
        node1 = mxmlFindElement(node1, root_node, "FCDA",NULL, NULL,MXML_DESCEND);
    }
exit:
    return res;
}

/*
描述: 解析CCD文件的GOOSE PUB.
参数:     GOOSE PUB的XML指针.
返回值: 解析是否成功
 */
int EDP_ParseGoosePub(mxml_node_t *pNode)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    mxml_node_t *node1=NULL;
    mxml_node_t *node2=NULL;
    char *p;
    uint8_t ucGseNum = 0;
    PROCESS_PUB_GSE *pGoose = NULL;
    PUB_GCB_INFO *pGooseGcb = NULL;
    int i = 0;

    root_node = pNode;

    /*获取PUB GOOSE 个数*/
    ucGseNum = EDP_GetXmlElementCnt(root_node, "GOCBref");
    if(ucGseNum == 0)
    {
        goto exit;
    }

    pGoose = &(g_tProcessBusCfg.tProPubGoose);
    /*GOOSE SUB个数*/
    pGoose->iPubGsCnt = ucGseNum;

    /*分配空间*/
    pGoose->pPubGcbInfo = (PUB_GCB_INFO *)calloc(ucGseNum, sizeof(PUB_GCB_INFO));
    if(!pGoose->pPubGcbInfo)
    {
        CFG_LOG("#### kevin: CCD GOOSE PUB分配空间失败,gcb个数:%d \n",ucGseNum,0,0,0,0,0);
        res = EDP_CCD_PARSE_ERR_FILE_CALLOC_FAIL;
        goto exit;
    }

    /*解析SUB GOOSE*/
    root_node = pNode;
    node1 = mxmlFindElement(root_node, root_node, "GOCBref",NULL, NULL,MXML_DESCEND);
    while(node1 != NULL)
    {
        pGooseGcb = &(pGoose->pPubGcbInfo[i]);
        pGooseGcb->next = &(pGoose->pPubGcbInfo[i+1]);
        pGooseGcb->iIndex = i;
        p=(char *)mxmlElementGetAttr(node1,"name");
        if(p)
        {
            pGooseGcb->pGOCBrefName = strdup(p);
        }
        else
        {
            res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        }

        /*解析GSEControl*/
        node2 = mxmlFindElement(node1, node1, "GSEControl",NULL, NULL,MXML_DESCEND);
        if(node2)
        {
            res |= EDP_ParseGoosePubGseControl(node2,pGooseGcb);
        }
        else
        {
            res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        }

        /*解析ConnectedAP*/
        node2 = mxmlFindElement(node1, node1, "ConnectedAP",NULL, NULL,MXML_DESCEND);
        if(node2)
        {
            res |= EDP_ParseGoosePubConnectAp(node2,pGooseGcb);
        }
        else
        {
            res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        }

        /*解析DataSet*/
        node2 = mxmlFindElement(node1, node1, "DataSet",NULL, NULL,MXML_DESCEND);
        if(node2)
        {
            res |= EDP_ParseGoosePubDataset(node2,pGooseGcb);
        }
        else
        {
            res |= EDP_CCD_PARSE_ERR_FILE_XML_GS_PARSE_FAIL;
        }

        i++;
        node1 = mxmlFindElement(node1, root_node, "GOCBref",NULL, NULL,MXML_DESCEND);
    }

exit:
    return res;
}


/*
描述: 解析CCD文件的GOOSE SUB的GseControl.
参数:
GOOSE SUB的XML指针.
pSubGooseGcb: sub的结构
返回值: 解析是否成功
 */
int EDP_ParseSmvSubSvControl(mxml_node_t *pNode, SUB_SMV_INFO *pSubSMVCb)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    char *p;
    uint8_t strInfo[TEMP_INFO_MAX_LEN];         /* 用于生成记录日志信息 */

    SUB_SMV_INFO *pSmvCb = NULL;
    mxml_node_t *node1=NULL;

    pSmvCb = pSubSMVCb;
    root_node = pNode;

    p=(char *)mxmlElementGetAttr(root_node,"confRev");
    if(p)
    {
        pSubSMVCb->tSvCtrl.iConfRev = strtol(p,NULL,10);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;
        res |= EDP_CONFIG_NECESSARY_PARA_LOST;
        sprintf(strInfo, "SVSUB中name为%s的SMVCBref缺少confRev信息\n", pSmvCb->pSVCBrefName);
        CFG_LOG(strInfo,1,2,3,4,5,6);
        LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
    }

    p=(char *)mxmlElementGetAttr(root_node,"datSet");
    if(p)
    {
        pSubSMVCb->tSvCtrl.pDatSet = strdup(p);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;
        res |= EDP_CONFIG_NECESSARY_PARA_LOST;
        sprintf(strInfo, "SVSUB中name为%s的SMVCBref缺少datSet信息\n", pSmvCb->pSVCBrefName);
        CFG_LOG(strInfo,1,2,3,4,5,6);
        LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
    }

    p=(char *)mxmlElementGetAttr(root_node,"multicast");
    if(p)
    {
        pSubSMVCb->tSvCtrl.pMultiCast = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
    }

    p=(char *)mxmlElementGetAttr(root_node,"name");
    if(p)
    {
        pSubSMVCb->tSvCtrl.pSvCtrlName = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
    }

    p=(char *)mxmlElementGetAttr(root_node,"nofASDU");
    if(p)
    {
        pSubSMVCb->tSvCtrl.iAsduCnt = strtol(p,NULL,10);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
    }

    p=(char *)mxmlElementGetAttr(root_node,"smpRate");
    if(p)
    {
        pSubSMVCb->tSvCtrl.iSmpRate = strtol(p,NULL,10);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
    }

    p=(char *)mxmlElementGetAttr(root_node,"smvID");
    if(p)
    {
        pSubSMVCb->tSvCtrl.pSvID = strdup(p);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;
        res |= EDP_CONFIG_NECESSARY_PARA_LOST;
        sprintf(strInfo, "SVSUB中name为%s的SMVCBref缺少smvID信息\n", pSmvCb->pSVCBrefName);
        LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
    }

    node1 = mxmlFindElement(root_node,root_node,"SmvOpts", NULL, NULL, MXML_DESCEND);
    if(node1 == NULL)
    {
        goto exit;
    }
    p=(char *)mxmlElementGetAttr(node1,"dataRef");
    if(p)
    {
        pSubSMVCb->tSvCtrl.tSvCtrlOpts.pDataRef = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
    }
    p=(char *)mxmlElementGetAttr(node1,"refreshTime");
    if(p)
    {
        pSubSMVCb->tSvCtrl.tSvCtrlOpts.pRefreshTime = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
    }
    p=(char *)mxmlElementGetAttr(node1,"sampleRate");
    if(p)
    {
        pSubSMVCb->tSvCtrl.tSvCtrlOpts.pSampleRate = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
    }
    p=(char *)mxmlElementGetAttr(node1,"sampleSynchronized");
    if(p)
    {
        pSubSMVCb->tSvCtrl.tSvCtrlOpts.pSampleSyn = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
    }
    p=(char *)mxmlElementGetAttr(node1,"security");
    if(p)
    {
        pSubSMVCb->tSvCtrl.tSvCtrlOpts.pSecurity = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
    }

exit:
    return res;
}

/*
描述: 解析CCD文件的GOOSE SUB的ConnectAp.
参数:
GOOSE SUB的XML指针.
pSubGooseGcb: sub的结构
返回值: 解析是否成功
 */
int EDP_ParseSmvSubConnectAp(mxml_node_t *pNode, SUB_SMV_INFO *pSubSMVCb)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    mxml_node_t *node1=NULL;
    mxml_node_t *node2=NULL;
    mxml_node_t *node3=NULL;
    char *p;
    SMV_AP_SMV_ADDR *pSmvAddr = NULL;
    SUB_SMV_INFO *pSmvCb = NULL;
    int i = 0;
    int k = 0;
    uint8_t ucAddrNum = 0;
    char *pAddrValue = NULL;
    char *pAddrType = NULL;
    char *tokenPtr = NULL;
    uint8_t strInfo[TEMP_INFO_MAX_LEN];         /* 用于生成记录日志信息 */
    BOOL bGetParaSuccess[4] = {0,0,0,0};                    /* 用于记录获取必需属性成功情况 */
    enum {MAC_ADDR, APPID, VLAN_PRI, VLAN_ID};  /* 用于区分不同的四种必需属性 */

    pSmvCb = pSubSMVCb;
    root_node = pNode;

    p=(char *)mxmlElementGetAttr(root_node,"apName");
    if(p)
    {
        pSmvCb->tConnectAp.pApName = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
    }

    p=(char *)mxmlElementGetAttr(root_node,"iedName");
    if(p)
    {
        pSmvCb->tConnectAp.pApIedName = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
    }


    node1 = mxmlFindElement(root_node, root_node, "SMV",NULL, NULL,MXML_DESCEND);
    if(node1 == NULL)
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;
        goto exit;
    }

    p=(char *)mxmlElementGetAttr(node1,"cbName");
    if(p)
    {
        pSmvCb->tConnectAp.tConnectApSmv.pCbName = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
    }

    p=(char *)mxmlElementGetAttr(node1,"ldInst");
    if(p)
    {
        pSmvCb->tConnectAp.tConnectApSmv.pLdInst = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
    }

    ucAddrNum = EDP_GetXmlElementCnt(node1, "Address");
    pSmvCb->tConnectAp.tConnectApSmv.iAddrCnt = ucAddrNum;

    if(ucAddrNum == 0)
    {
        CFG_LOG("未配置SMV  Sub的Address信息,SUB SMV name: %s,%d \n",(int)pSmvCb->pSVCBrefName,pSmvCb->iIndex,0,0,0,0);
        res = EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;
        goto exit;
    }

    root_node = pNode;
    node2 = mxmlFindElement(node1, node1, "Address",NULL, NULL,MXML_DESCEND);
    while(node2 != NULL)
    {
        pSmvAddr = &(pSmvCb->tConnectAp.tConnectApSmv.pSmvAddr[i]);
        pSmvAddr->iIndex = i;
        memset(bGetParaSuccess, 0, 4);

        for(node3 = mxmlFindElement(node2, node2, "P",NULL, NULL,MXML_DESCEND);
                node3!=NULL;
                node3 = mxmlFindElement(node3, node2, "P",NULL, NULL,MXML_DESCEND))
        {
            pAddrType = (char *)mxmlElementGetAttr(node3, "type");
            pAddrValue = (char *)mxmlElementGetValue(node3);
            if((!pAddrType) || (!pAddrValue))
            {
                CFG_LOG("SMV SUB Address解析错误,SUB SMV name: %s,%d \n",(int)pSmvCb->pSVCBrefName,i,0,0,0,0);
                res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;
                goto exit;
            }

            if(strcmp(pAddrType, "MAC-Address") == 0)
            {
                bGetParaSuccess[MAC_ADDR] = TRUE;
                tokenPtr = strtok(strdup(pAddrValue),"-");
                for(k = 0; (k < 6) && (tokenPtr != NULL); k++)
                {
                    pSmvAddr->aDstMac[k] = strtol(tokenPtr,NULL,16);
                    tokenPtr = strtok(NULL,"-");
                }

                if((pSmvAddr->aDstMac[0] != 0x01) || (pSmvAddr->aDstMac[1] != 0x0C)
                        || (pSmvAddr->aDstMac[2] != 0xCD) || (pSmvAddr->aDstMac[3] != 0x04))
                {
                    res |= EDP_CONFIG_PARA_OVERFLOW;
                    sprintf(strInfo, "SVSUB中name为%s的SMVCBref中MAC-Address越限\n", pSmvCb->pSVCBrefName);
                    CFG_LOG(strInfo,1,2,3,4,5,6);
                    LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
                }
            }
            else if(strcmp(pAddrType, "VLAN-ID") == 0)
            {
                bGetParaSuccess[VLAN_ID] = TRUE;
                pSmvAddr->usVID = strtol(pAddrValue,NULL,16);

                if(pSmvAddr->usVID > 0xFFE)
                {
                    res |= EDP_CONFIG_PARA_OVERFLOW;
                    sprintf(strInfo, "SVSUB中name为%s的SMVCBref中VLAN-ID越限\n", pSmvCb->pSVCBrefName);
                    CFG_LOG(strInfo,1,2,3,4,5,6);
                    LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
                }
            }
            else if(strcmp(pAddrType, "VLAN-PRIORITY") == 0)
            {
                bGetParaSuccess[VLAN_PRI] = TRUE;
                pSmvAddr->usVPri= strtol(pAddrValue,NULL,10);

                if(pSmvAddr->usVPri > 0x07)
                {
                    res |= EDP_CONFIG_PARA_OVERFLOW;
                    sprintf(strInfo, "SVSUB中name为%s的SMVCBref中VLAN-PRIORITY越限\n", pSmvCb->pSVCBrefName);
                    CFG_LOG(strInfo,1,2,3,4,5,6);
                    LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
                }
            }
            else if(strcmp(pAddrType, "APPID") == 0)
            {
                bGetParaSuccess[APPID] = TRUE;
                pSmvAddr->usAppID = strtol(pAddrValue,NULL,16);

                if((pSmvAddr->usAppID > 0x7FFF) || (pSmvAddr->usAppID < 0x4000))
                {
                    res |= EDP_CONFIG_PARA_OVERFLOW;
                    sprintf(strInfo, "SVSUB中name为%s的SMVCBref中APPID越限\n", pSmvCb->pSVCBrefName);
                    CFG_LOG(strInfo,1,2,3,4,5,6);
                    LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
                }
            }
        }

        if(!bGetParaSuccess[MAC_ADDR])
        {
            res |= EDP_CONFIG_NECESSARY_PARA_LOST;
            sprintf(strInfo, "SVSUB中name为%s的SMVCBref缺少MAC-Address信息\n", pSmvCb->pSVCBrefName);
            CFG_LOG(strInfo,1,2,3,4,5,6);
            LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
        }
        if(!bGetParaSuccess[VLAN_ID])
        {
            res |= EDP_CONFIG_NECESSARY_PARA_LOST;
            sprintf(strInfo, "SVSUB中name为%s的SMVCBref缺少VLAN-ID信息\n", pSmvCb->pSVCBrefName);
            CFG_LOG(strInfo,1,2,3,4,5,6);
            LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
        }
        if(!bGetParaSuccess[VLAN_PRI])
        {
            res |= EDP_CONFIG_NECESSARY_PARA_LOST;
            sprintf(strInfo, "SVSUB中name为%s的SMVCBref缺少VLAN-PRIORITY信息\n", pSmvCb->pSVCBrefName);
            CFG_LOG(strInfo,1,2,3,4,5,6);
            LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
        }
        if(!bGetParaSuccess[APPID])
        {
            res |= EDP_CONFIG_NECESSARY_PARA_LOST;
            sprintf(strInfo, "SVSUB中name为%s的SMVCBref缺少APPID信息\n", pSmvCb->pSVCBrefName);
            CFG_LOG(strInfo,1,2,3,4,5,6);
            LOG_Write(LOG_INFO, (const uint8_t *)strInfo, NULL);
        }

        i++;
        node2 = mxmlFindElement(node2, node1, "Address",NULL, NULL,MXML_DESCEND);
    }

exit:
    return res;
}

/*
描述: 解析CCD文件的GOOSE SUB的Dataset.
参数:
GOOSE SUB的XML指针.
pSubGooseGcb: sub的结构
返回值: 解析是否成功
 */
int EDP_ParseSmvSubDataset(mxml_node_t *pNode, SUB_SMV_INFO *pSubSMVCb)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    mxml_node_t *node1=NULL;
    mxml_node_t *node2=NULL;
    mxml_node_t *node3=NULL;
    char *p;
    SUB_SMV_INFO *pSmvCb = NULL;
    int i = 0;
    int j = 0;
    uint8_t ucFcdaNum = 0;
    uint8_t ucIntAddrNum = 0;
    SMV_DATESET_FCDA_SUB *pSmvFcda = NULL;
    SMV_DATESET_FCDA_INTADDR *pIntAddr = NULL;
    char Tmp[16];

    pSmvCb = pSubSMVCb;
    root_node = pNode;

    p=(char *)mxmlElementGetAttr(root_node,"name");
    if(p)
    {
        pSmvCb->tDataSet.pDateSetName = strdup(p);
    }
    else
    {
        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
    }

    ucFcdaNum = EDP_GetXmlElementCnt(root_node, "FCDA");
    pSmvCb->tDataSet.iFcdaCnt = ucFcdaNum;

    if(ucFcdaNum == 0)
    {
        CFG_LOG("未配置SMV  sub的FCDA个数为0,SUB SMV name: %s,%d \n",(int)pSmvCb->pSVCBrefName,pSmvCb->iIndex,0,0,0,0);
        res = EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;
        goto exit;
    }

    /*分配空间*/
    pSmvCb->tDataSet.pDataSetFcda = (SMV_DATESET_FCDA_SUB *)calloc(ucFcdaNum, sizeof(SMV_DATESET_FCDA_SUB));
    if(!pSmvCb->tDataSet.pDataSetFcda)
    {
        CFG_LOG("#### kevin: CCD SMV SUB FCDA分配空间失败,FCDA个数:%d,SMVindex: %d \n",ucFcdaNum,pSmvCb->iIndex,0,0,0,0);
        res = EDP_CCD_PARSE_ERR_FILE_CALLOC_FAIL;
        goto exit;
    }

    root_node = pNode;
    node1 = mxmlFindElement(root_node, root_node, "FCDA",NULL, NULL,MXML_DESCEND);
    while(node1 != NULL)
    {
        pSmvFcda = &(pSmvCb->tDataSet.pDataSetFcda[i]);
        pSmvFcda->next = &(pSmvCb->tDataSet.pDataSetFcda[i+1]);
        pSmvFcda->iIndex = i;

        p=(char *)mxmlElementGetAttr(node1,"bType");
        if(p)
        {
            pSmvFcda->pBType = strdup(p);
        }
        else
        {
            /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
        }

        p=(char *)mxmlElementGetAttr(node1,"daName");
        if(p)
        {
            pSmvFcda->pDaName = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"doName");
        if(p)
        {
            pSmvFcda->pDoName = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"fc");
        if(p)
        {
            pSmvFcda->pFC = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"ldInst");
        if(p)
        {
            pSmvFcda->pLdInst = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"lnClass");
        if(p)
        {
            pSmvFcda->pLnClass = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"lnInst");
        if(p)
        {
            pSmvFcda->pLnInst = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"prefix");
        if(p)
        {
            pSmvFcda->pPrefix = strdup(p);
        }

        p=(char *)mxmlElementGetAttr(node1,"desc");
        if(p)
        {
            pSmvFcda->pFcdaDesc = strdup(p);
        }

        ucIntAddrNum = EDP_GetXmlElementCnt(node1, "intAddr");
        pSmvFcda->iIntAddrCnt = ucIntAddrNum;
        pSmvFcda->pIntAddr = (SMV_DATESET_FCDA_INTADDR *)calloc(ucIntAddrNum,sizeof(SMV_DATESET_FCDA_INTADDR));
        if(!pSmvFcda->pIntAddr)
        {
            CFG_LOG("#### kevin: CCD SMV SUB FCDA下的intAddr 分配空间失败,FCDA个数:%d,intAddr个数:%d, SMVindex: %d \n",ucFcdaNum,ucIntAddrNum,pSmvCb->iIndex,0,0,0);
            res = EDP_CCD_PARSE_ERR_FILE_CALLOC_FAIL;
            goto exit;
        }

        for(j = 0, node2 = mxmlFindElement(node1, node1, "intAddr",NULL, NULL,MXML_DESCEND);
                node2!=NULL;
                j++, node2 = mxmlFindElement(node2, node1, "intAddr",NULL, NULL,MXML_DESCEND))
        {
            pIntAddr = &(pSmvFcda->pIntAddr[j]);

            p=(char *)mxmlElementGetAttr(node2,"desc");
            if(p)
            {
                pIntAddr->pDesc = strdup(p);
            }
            else
            {
                pIntAddr->pDesc = NULL;
            }

            p=(char *)mxmlElementGetAttr(node2,"name");
            if(p)
            {
                pIntAddr->pIntaddrName = strdup(p);
                if(!(memcmp(pIntAddr->pIntaddrName,EDP_CCD_INTADDR_INVALID_NAME,sizeof(EDP_CCD_INTADDR_INVALID_NAME))))
                {
                    pIntAddr->bIsIntAddrValid = FALSE;
                }
                else
                {
                    if(!pIntAddr->bIsIntAddrValid)
                    {
                        pSmvCb->iUsedFcdaCnt++;
                    }
                    pIntAddr->bIsIntAddrValid = TRUE;
                    pSmvCb->iUsedIntaddrCnt++;
                    if(NULL == pSmvCb->pIntAddrName)
                    {
                        GetEnumItem_Div(p,0,Tmp,':',sizeof(Tmp));
                        pSmvCb->pIntAddrName = strdup(Tmp);
                        GetEnumItem_Div(pSmvCb->pIntAddrName, 0, Tmp, '-', sizeof(Tmp));
                        pSmvCb->iSubCcBoardId = strtol(Tmp,NULL,10);
                        GetEnumItem_Div(pSmvCb->pIntAddrName, 1, Tmp, '-', sizeof(Tmp));
                        pSmvCb->iSubCcPortId = (int)(Tmp[0]-'A');
                        g_bCcIsUsed[pSmvCb->iSubCcBoardId] = TRUE;
                    }
                }
            }
            else
            {
                res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;
                CFG_LOG("读取FCDA 中intAdd名称不允许为空,SUB SMV name: %s,%d,FCDA index:%d \n",
                        (int)pSmvCb->pSVCBrefName,i,pSmvFcda->iIndex,0,0,0);
                goto exit;
            }

            if(pIntAddr->bIsIntAddrValid)
            {
                node3 = EDP_GetXmlDAIElement(node2);
                if(node3 == NULL)
                {
                    pIntAddr->bIsDaiValid = FALSE;
                    CFG_LOG("读取FCDA 中的intAddr 中的DAI出错,SUB SMV name: %s,%d\n",(int)pSmvCb->pSVCBrefName,i,0,0,0,0);
                    res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;
                }
                else
                {
                    pIntAddr->bIsDaiValid = TRUE;
                    p=(char *)mxmlElementGetAttr(node3,"name");
                    if(p)
                    {
                        pIntAddr->tDai.pDaiName = strdup(p);
                    }
                    else
                    {
                        /*res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;*/
                    }

                    p=(char *)mxmlElementGetAttr(node3,"sAddr");
                    if(p)
                    {
                        pIntAddr->tDai.pSAddr = strdup(p);
                    }
                    else
                    {
                        CFG_LOG("读取FCDA 中的intAddr 中的DAI saaddr 出错,SUB SMV name: %s,%d,FCDA index:%d\n",
                                (int)pSmvCb->pSVCBrefName,i,pSmvFcda->iIndex,0,0,0);
                        res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;
                        goto exit;
                    }
                }
            }
        }

        i++;
        node1 = mxmlFindElement(node1, root_node, "FCDA",NULL, NULL,MXML_DESCEND);
    }
exit:
    return res;
}


/*
描述: 解析CCD文件的SV SUB.
参数:     SV SUB的XML指针.
返回值: 解析是否成功
 */
int EDP_ParseSvSub(mxml_node_t *pNode)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    mxml_node_t *node1=NULL;
    mxml_node_t *node2=NULL;
    char *p;
    uint8_t ucSmvNum = 0;
    PROCESS_SUB_SMV*pSmv = NULL;
    SUB_SMV_INFO *pSMVCb = NULL;
    int i = 0;

    root_node = pNode;

    /*获取SUB SV 个数*/
    ucSmvNum = EDP_GetXmlElementCnt(root_node, "SMVCBref");
    if(ucSmvNum == 0)
    {
        goto exit;
    }

    pSmv = &(g_tProcessBusCfg.tProSubSmv);
    /*SV SUB个数*/
    pSmv->iSubSmvCnt = ucSmvNum;

    /*分配空间*/
    pSmv->pSubSmvInfo = (SUB_SMV_INFO *)calloc(ucSmvNum, sizeof(SUB_SMV_INFO));
    if(!pSmv->pSubSmvInfo)
    {
        CFG_LOG("#### kevin: CCD SMV SUB分配空间失败,smvcb个数:%d \n",ucSmvNum,0,0,0,0,0);
        res = EDP_CCD_PARSE_ERR_FILE_CALLOC_FAIL;
        goto exit;
    }

    /*解析SUB SV*/
    root_node = pNode;
    node1 = mxmlFindElement(root_node, root_node, "SMVCBref",NULL, NULL,MXML_DESCEND);
    while(node1 != NULL)
    {
        pSMVCb = &(pSmv->pSubSmvInfo[i]);
        pSMVCb->next = &(pSmv->pSubSmvInfo[i+1]);
        pSMVCb->iIndex = i;
        p=(char *)mxmlElementGetAttr(node1,"name");
        if(p)
        {
            pSMVCb->pSVCBrefName = strdup(p);
        }
        else
        {
            res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;
        }

        /*解析SV Control*/
        node2 = mxmlFindElement(node1, node1, "SampledValueControl",NULL, NULL,MXML_DESCEND);
        if(node2)
        {
            res |= EDP_ParseSmvSubSvControl(node2,pSMVCb);
        }
        else
        {
            res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;
        }

        /*解析ConnectedAP*/
        node2 = mxmlFindElement(node1, node1, "ConnectedAP",NULL, NULL,MXML_DESCEND);
        if(node2)
        {
            res |= EDP_ParseSmvSubConnectAp(node2,pSMVCb);
        }
        else
        {
            res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;
        }

        /*解析DataSet*/
        node2 = mxmlFindElement(node1, node1, "DataSet",NULL, NULL,MXML_DESCEND);
        if(node2)
        {
            res |= EDP_ParseSmvSubDataset(node2,pSMVCb);
        }
        else
        {
            res |= EDP_CCD_PARSE_ERR_FILE_XML_SMV_PARSE_FAIL;
        }

        i++;
        node1 = mxmlFindElement(node1, root_node, "SMVCBref",NULL, NULL,MXML_DESCEND);
    }

exit:
    return res;
}

/*
描述: 解析CCD文件的SV PUB.
参数:     SV PUB的XML指针.
返回值: 解析是否成功
 */
int EDP_ParseSvPub(mxml_node_t *pNode)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    /*mxml_node_t *node1=NULL;
    char *p = NULL;*/


    root_node = pNode;

    goto exit;
exit:
    return res;
}

static void EP_Make_CRC32_Table_TMP(void)
{
    unsigned int i, j;
    uint32_t ul = 1;

    aulTbl32_g_TMP[0] = 0;
    for (i = 128; i; i >>= 1)
    {
        ul = (ul >> 1) ^ ((ul & 1) ? POLYNOMIAL_TMP : 0);

        /* ul is now aulTbl32_g[i] */
        for (j = 0; j < 256; j += 2*i)
            aulTbl32_g_TMP[i+j] = aulTbl32_g_TMP[j] ^ ul;
    }
}

uint32_t EP_CRC32_TMP(unsigned char *pucBuf, int uiLen, uint32_t ulCrc)
{
    static BOOL bFirst = TRUE;
    if (bFirst)
    {
        bFirst = FALSE;
        EP_Make_CRC32_Table_TMP();
    }

    ulCrc ^= 0xFFFFFFFF;

    while (uiLen--)
        ulCrc = (ulCrc >> 8) ^ aulTbl32_g_TMP[(ulCrc ^ *pucBuf++) & 0xFF];

    return ulCrc ^ 0xFFFFFFFF;
}

/*
描述: 获取CCD的CRC值
参数: pNode, CCD文件的XML指针.
      pCrc, CCD的CRC值
返回值: 解析是否成功
 */
int EDP_GetCcdFileCrc(mxml_node_t *pNode, uint32_t *pCrc)
{
    int res;
    int iLen = 0;
    FILE* fp;
    char buf[256];
    uint32_t ulCrc = 0;

    res = EDP_CreatCrcFile(pNode, PROCESS_CRC_FILE);
    if(res == 0)
    {
        fp = fopen(PROCESS_CRC_FILE, "r");
        if(fp == NULL)
        {
            res = EDP_CRC_FILE_READ_ERROR;
            goto exit;
        }

        fseek(fp, 0, SEEK_SET);
        while((iLen = fread(buf,1,256,fp)) != 0)
        {
            ulCrc = EP_CRC32_TMP(buf, iLen, ulCrc);
        }

        *pCrc = ulCrc;

        fclose(fp);
    }

exit:
    return res;
}

/*
描述: 解析CCD文件的CRC.
参数:     CRC的XML指针.
返回值: 解析是否成功
 */
int EDP_ParseCrc(mxml_node_t *pNode)
{
    mxml_node_t *root_node=NULL;
    int res = 0;
    char *p;
    char ucCrcTmp[64];
    PROCESS_PRIVATE *pProcessCrc = NULL;


    root_node = pNode;
    pProcessCrc = &(g_tProcessBusCfg.tProPrivate);

    ucCrcTmp[0] = '0';
    ucCrcTmp[1] = 'x';
    p=(char *)mxmlElementGetAttr(root_node,"id");
    strcpy(&ucCrcTmp[2], p);
    if(p)
    {
        pProcessCrc->ulCrc = strtoul(ucCrcTmp,NULL,16);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_CRC_PARSE_FAIL;
    }
    p=(char *)mxmlElementGetAttr(root_node,"timestamp");
    if(p)
    {
        pProcessCrc->pTimeStamp = strdup(p);
    }
    else
    {
        res |= EDP_CCD_PARSE_ERR_FILE_XML_CRC_PARSE_FAIL;
    }

    return res;
}

/*
描述: 校验CCD文件中的装置信息是否与本装置相符.
参数: 无.
返回值: 返回错误码,0代表正常
 */
int EDP_CheckDevInfo()
{
    int res = 0;
    char TempInfo[256];
    char sDevType[256];
    char sDevTypeWithoutFuncOpt[FT_VER_INFO_LEN+1];

    if(strcmp(g_tProcessBusCfg.pManufacturer, "GDNZ") != 0)
    {
        sprintf(TempInfo, "CCD中生产厂家信息与实际不符,现为%s,应为GDNZ\n", g_tProcessBusCfg.pManufacturer);
        res |= EDP_CONFIG_DEV_INFO_CHECK_ERR;
        LOG_Dbg_Msg(TempInfo,1,2,3,4,5,6);
        LOG_Write(LOG_INFO, TempInfo, NULL);
    }


    if(1 == FT_Rd_Version_INI("[SYSTEM]", "DEVICETYPE", sDevTypeWithoutFuncOpt, FT_VER_INFO_LEN+1))
    {
        /* 读取出错则不置异常标识 */
        if(0 == strcmp(strFuncOptShow,""))
        {
            /*无选配文件或者为未选配*/
            if(strcmp(g_tProcessBusCfg.pDevType, sDevTypeWithoutFuncOpt) != 0)
            {
                sprintf(TempInfo, "CCD中装置类型及选配信息与实际不符,现为%s,应为%s\n"
                        , g_tProcessBusCfg.pDevType, sDevTypeWithoutFuncOpt);
                res |= EDP_CONFIG_DEV_INFO_CHECK_ERR;
                LOG_Dbg_Msg(TempInfo,1,2,3,4,5,6);
                LOG_Write(LOG_INFO, TempInfo, NULL);
            }
        }
        else
        {
            sprintf(sDevType, "%s-%s", sDevTypeWithoutFuncOpt, strFuncOptShow);
            if(strcmp(g_tProcessBusCfg.pDevType, sDevType) != 0)
            {
                sprintf(TempInfo, "CCD中装置类型及选配信息与实际不符,现为%s,应为%s\n"
                        , g_tProcessBusCfg.pDevType, sDevType);
                res |= EDP_CONFIG_DEV_INFO_CHECK_ERR;
                LOG_Dbg_Msg(TempInfo,1,2,3,4,5,6);
                LOG_Write(LOG_INFO, TempInfo, NULL);
            }
        }
    }
    else
    {
        LOG_Dbg_Msg("读取Version.ini文件中的DEVICETYPE字段失败!\n",1,2,3,4,5,6);
        LOG_Write(LOG_INFO, "读取Version.ini文件中的DEVICETYPE字段失败!\n", NULL);
    }

    return res;
}

/*
描述: 解析CCD文件的入口函数.
参数:     CCD文件的指针.
返回值: 解析是否成功
 */
int EDP_LoadCCDFile(char *pFileName)
{
    FILE *fp;
    char *p;
    mxml_node_t *node1=NULL;
    mxml_node_t *root_node=NULL;
    mxml_node_t *root_node2=NULL;
    int res = 0;

    memset(g_bCcIsUsed, 0, sizeof(BOOL)*MAX_CC_BOARD_ID_NUM);

    if(!pFileName)
    {
        res = EDP_CCD_PARSE_ERR_FILE_NOT_EXIST;
        goto exit;
    }

    fp = fopen(pFileName, "r");
    if(fp==NULL)
    {
        res = EDP_CCD_PARSE_ERR_FILE_OPEN_FAIL;
        goto exit;
    }

    root_node = mxmlLoadFile(NULL,fp,MXML_NO_CALLBACK);

    fclose(fp);

    if(!root_node)
    {
        res = EDP_CCD_PARSE_ERR_FILE_XML_LOAD_FAIL;
        goto exit;
    }

    node1 = mxmlFindElement(root_node,root_node, "IED",NULL, NULL,MXML_DESCEND);

    p = (char *)mxmlElementGetAttr(node1,"name");
    if(p)
    {
        g_tProcessBusCfg.pIEDName = strdup(p);
    }
    else
    {
        res = EDP_CCD_PARSE_ERR_FILE_XML_LOAD_FAIL;
        goto exit;
    }

    p = (char *)mxmlElementGetAttr(node1,"desc");
    if(p)
    {
        g_tProcessBusCfg.pIEDDes = strdup(p);
    }

    p = (char *)mxmlElementGetAttr(node1,"configVersion");
    if(p)
    {
        g_tProcessBusCfg.pCfgVersion = strdup(p);
    }

    p = (char *)mxmlElementGetAttr(node1,"manufacturer");
    if(p)
    {
        g_tProcessBusCfg.pManufacturer = strdup(p);
    }

    p = (char *)mxmlElementGetAttr(node1,"type");
    if(p)
    {
        g_tProcessBusCfg.pDevType = strdup(p);
    }

    /*解析GOOSESUB*/
    node1 = mxmlFindElement(root_node,root_node, "GOOSESUB",NULL, NULL,MXML_DESCEND);
    if(node1)
    {
        res |= EDP_ParseGooseSub(node1);
    }

    /*解析GOOSEPUB*/
    node1 = mxmlFindElement(root_node,root_node, "GOOSEPUB",NULL, NULL,MXML_DESCEND);
    if(node1)
    {
        res |= EDP_ParseGoosePub(node1);
    }

    /*解析SVSUB*/
    node1 = mxmlFindElement(root_node,root_node, "SVSUB",NULL, NULL,MXML_DESCEND);
    if(node1)
    {
        res |= EDP_ParseSvSub(node1);
    }

    /*解析SVPUB*/
    node1 = mxmlFindElement(root_node,root_node, "SVPUB",NULL, NULL,MXML_DESCEND);
    if(node1)
    {
        res |= EDP_ParseSvPub(node1);
    }

    /*解析CRC*/
    node1 = mxmlFindElement(root_node,root_node, "CRC",NULL, NULL,MXML_DESCEND);
    if(node1)
    {
        res |= EDP_ParseCrc(node1);
    }

    EDP_GetCcdFileCrc(root_node, &g_ulCcdFileCheckCrc);
    if(g_ulCcdFileCheckCrc != EDP_GetProcessCrc()->ulCrc)
    {
        res |= EDP_CONFIG_CRC_CHECK_ERR;
        LOG_Dbg_Msg("CCD文件CRC校验不一致,计算结果为%08lx,文件中为%08lx\n"
                    , g_ulCcdFileCheckCrc, EDP_GetProcessCrc()->ulCrc,3,4,5,6);
        LOG_Write(LOG_INFO,"CCD文件CRC校验不一致!\n",NULL);
    }

    /*装置信息校验包括生产厂家、装置型号及选配信息*/
    if((EDP_CheckDevInfo()&EDP_CONFIG_DEV_INFO_CHECK_ERR) != 0)
    {
        res |= EDP_CONFIG_DEV_INFO_CHECK_ERR;
        LOG_Dbg_Msg("CCD文件装置信息配置出错\n",1,2,3,4,5,6);
    }

exit:
    mxmlDelete(root_node2);

    if(root_node)
    {
        mxmlDelete(root_node);
    }
    return res;
}

/*
描述: 获取process.xml中的pub goose指针
参数:     无
返回值: process.xml中的pub goose指针
 */
PROCESS_PUB_GSE *EDP_GetProcessPubGoose()
{
    return &(g_tProcessBusCfg.tProPubGoose);
}

/*
描述: 获取process.xml中的sub goose指针
参数:     无
返回值: process.xml中的sub goose指针
 */
PROCESS_SUB_GSE *EDP_GetProcessSubGoose()
{
    return &(g_tProcessBusCfg.tProSubGoose);
}

/*
描述: 获取process.xml中的pub sm指针
参数:     无
返回值: process.xml中的pub sm指针
 */
PROCESS_PUB_SMV *EDP_GetProcessPubSmv()
{
    return &(g_tProcessBusCfg.tProPubSmv);
}

/*
描述: 获取process.xml中的sub sm指针
参数:     无
返回值: process.xml中的sub sm指针
 */
PROCESS_SUB_SMV *EDP_GetProcessSubSmv()
{
    return &(g_tProcessBusCfg.tProSubSmv);
}


/*
描述: 获取process.xml中的private指针
参数:     无
返回值: process.xml中的private 指针
 */
PROCESS_PRIVATE *EDP_GetProcessCrc()
{
    return &(g_tProcessBusCfg.tProPrivate);
}

/*
描述: 获取process.xml的指针
参数:     无
返回值: process.xml 指针
 */
PROCESS_CFG *EDP_GetProcessPtr()
{
    return &(g_tProcessBusCfg);
}

/*
描述:获取GSE PUB的个数
参数:     无
返回值: GSE PUB的个数
 */
int EDP_GetPubGooseCnt()
{
    return g_tProcessBusCfg.tProPubGoose.iPubGsCnt;
}

/*
描述:获取GSE SUB的个数
参数:     无
返回值: GSE SUB的个数
 */
int EDP_GetSubGooseCnt()
{
    return g_tProcessBusCfg.tProSubGoose.iSubGsCnt;
}

/*
描述:获取SMV PUB的个数
参数:     无
返回值: SMV PUB的个数
 */
int EDP_GetPubSmvCnt()
{
    return g_tProcessBusCfg.tProPubSmv.iPubSmvCnt;
}

/*
描述:获取SMV SUB的个数
参数:     无
返回值: SMV SUB的个数
 */
int EDP_GetSubSmvCnt()
{
    return g_tProcessBusCfg.tProSubSmv.iSubSmvCnt;
}

/*
描述:获取IED名称
参数:     无
返回值: IED名称
 */
char * EDP_GetIEDName()
{
    return g_tProcessBusCfg.pIEDName;
}

/*
描述: 释放内存,CCD Gse
参数:    NONE
返回值: 释放是否成功
 */
int EDP_FreeCCDGse()
{
    int i = 0;
    int j = 0;
    int k= 0;
    int fcdacnt = 0;
    GSE_DATESET_FCDA_SUB *pSubFcda = NULL;
    GSE_DATESET_FCDA_PUB *pPubFcda = NULL;
    EP_STATUS res = EP_SUCCESS;
    PROCESS_SUB_GSE *pSubGse = NULL;
    PROCESS_PUB_GSE *pPubGse = NULL;
    int subcnt = 0;
    int pubcnt = 0;
    SUB_GCB_INFO *pSubGcb = NULL;
    PUB_GCB_INFO *pPubGcb = NULL;
    pSubGse = EDP_GetProcessSubGoose();
    pPubGse = EDP_GetProcessPubGoose();
    subcnt = EDP_GetSubGooseCnt();
    pubcnt = EDP_GetPubGooseCnt();

    for(i = 0; i < subcnt; i++)
    {
        pSubGcb = &(pSubGse->pSubGcbInfo[i]);
        free(pSubGcb->pGOCBrefName);
        pSubGcb->pGOCBrefName = NULL;
        free(pSubGcb->pIntAddrName);
        pSubGcb->pIntAddrName = NULL;

        free(pSubGcb->tGseCtrl.pAppID);
        pSubGcb->tGseCtrl.pAppID = NULL;
        free(pSubGcb->tGseCtrl.pDatSet);
        pSubGcb->tGseCtrl.pDatSet = NULL;
        free(pSubGcb->tGseCtrl.pCtrlName);
        pSubGcb->tGseCtrl.pCtrlName = NULL;
        free(pSubGcb->tGseCtrl.pCtrlType);
        pSubGcb->tGseCtrl.pCtrlType = NULL;


        free(pSubGcb->tConnectAp.pApIedName);
        pSubGcb->tConnectAp.pApIedName = NULL;
        free(pSubGcb->tConnectAp.pApName);
        pSubGcb->tConnectAp.pApName = NULL;

        free(pSubGcb->tConnectAp.tConnectApGse.pCbName);
        pSubGcb->tConnectAp.tConnectApGse.pCbName = NULL;
        free(pSubGcb->tConnectAp.tConnectApGse.pLdInst);
        pSubGcb->tConnectAp.tConnectApGse.pLdInst = NULL;
        free(pSubGcb->tConnectAp.tConnectApGse.tGseTime.pMaxMultiplier);
        pSubGcb->tConnectAp.tConnectApGse.tGseTime.pMaxMultiplier = NULL;
        free(pSubGcb->tConnectAp.tConnectApGse.tGseTime.pMaxUnit);
        pSubGcb->tConnectAp.tConnectApGse.tGseTime.pMaxUnit = NULL;
        free(pSubGcb->tConnectAp.tConnectApGse.tGseTime.pMinMultiplier);
        pSubGcb->tConnectAp.tConnectApGse.tGseTime.pMinMultiplier = NULL;
        free(pSubGcb->tConnectAp.tConnectApGse.tGseTime.pMinUnit);
        pSubGcb->tConnectAp.tConnectApGse.tGseTime.pMinUnit = NULL;

        free(pSubGcb->tDataSet.pDateSetName);
        pSubGcb->tDataSet.pDateSetName = NULL;
        fcdacnt = pSubGcb->tDataSet.iFcdaCnt;
        for(j = 0; j < fcdacnt; j++)
        {
            pSubFcda = &(pSubGcb->tDataSet.pDataSetFcda[j]);
            free(pSubFcda->pBType);
            pSubFcda->pBType = NULL;
            free(pSubFcda->pDaName);
            pSubFcda->pDaName= NULL;
            free(pSubFcda->pDoName);
            pSubFcda->pDoName= NULL;
            free(pSubFcda->pFC);
            pSubFcda->pFC= NULL;
            free(pSubFcda->pLdInst);
            pSubFcda->pLdInst= NULL;
            free(pSubFcda->pLnClass);
            pSubFcda->pLnClass= NULL;
            free(pSubFcda->pLnInst);
            pSubFcda->pLnInst= NULL;
            free(pSubFcda->pPrefix);
            pSubFcda->pPrefix= NULL;
            free(pSubFcda->pFcdaDesc);
            pSubFcda->pFcdaDesc= NULL;

            for(k = 0; k < pSubFcda->iIntAddrCnt; k++)
            {
                GSE_DATESET_FCDA_INTADDR *pintAddr;
                pintAddr = &(pSubFcda->pIntAddr[k]);
                free(pintAddr->pIntaddrDesc);
                pintAddr->pIntaddrDesc = NULL;
                free(pintAddr->pIntaddrName);
                pintAddr->pIntaddrName = NULL;
                free(pintAddr->tDai.pDaiName);
                pintAddr->tDai.pDaiName = NULL;
                free(pintAddr->tDai.pSAddr);
                pintAddr->tDai.pSAddr = NULL;
                free(pintAddr->tDai.tSAddr.pBay);
                pintAddr->tDai.tSAddr.pBay = NULL;
                free(pintAddr->tDai.tSAddr.pTDI);
                pintAddr->tDai.tSAddr.pTDI = NULL;
                free(pintAddr->tDai.tSAddr.pType);
                pintAddr->tDai.tSAddr.pType = NULL;
                free(pintAddr->tDai.tSAddr.pYbId);
                pintAddr->tDai.tSAddr.pYbId = NULL;
            }

        }
        free( pSubGcb->tDataSet.pDataSetFcda);
    }
    free(pSubGse->pSubGcbInfo);
    pSubGse->pSubGcbInfo = NULL;

    for(i = 0; i < pubcnt; i++)
    {
        pPubGcb = &(pPubGse->pPubGcbInfo[i]);
        free(pPubGcb->pGOCBrefName);
        pPubGcb->pGOCBrefName = NULL;

        free(pPubGcb->tGseCtrl.pAppID);
        pPubGcb->tGseCtrl.pAppID = NULL;
        free(pPubGcb->tGseCtrl.pDatSet);
        pPubGcb->tGseCtrl.pDatSet = NULL;
        free(pPubGcb->tGseCtrl.pCtrlName);
        pPubGcb->tGseCtrl.pCtrlName = NULL;
        free(pPubGcb->tGseCtrl.pCtrlType);
        pPubGcb->tGseCtrl.pCtrlType = NULL;


        free(pPubGcb->tConnectAp.pApIedName);
        pPubGcb->tConnectAp.pApIedName = NULL;
        free(pPubGcb->tConnectAp.pApName);
        pPubGcb->tConnectAp.pApName = NULL;

        free(pPubGcb->tConnectAp.tConnectApGse.pCbName);
        pPubGcb->tConnectAp.tConnectApGse.pCbName = NULL;
        free(pPubGcb->tConnectAp.tConnectApGse.pLdInst);
        pPubGcb->tConnectAp.tConnectApGse.pLdInst = NULL;
        free(pPubGcb->tConnectAp.tConnectApGse.tGseTime.pMaxMultiplier);
        pPubGcb->tConnectAp.tConnectApGse.tGseTime.pMaxMultiplier = NULL;
        free(pPubGcb->tConnectAp.tConnectApGse.tGseTime.pMaxUnit);
        pPubGcb->tConnectAp.tConnectApGse.tGseTime.pMaxUnit = NULL;
        free(pPubGcb->tConnectAp.tConnectApGse.tGseTime.pMinMultiplier);
        pPubGcb->tConnectAp.tConnectApGse.tGseTime.pMinMultiplier = NULL;
        free(pPubGcb->tConnectAp.tConnectApGse.tGseTime.pMinUnit);
        pPubGcb->tConnectAp.tConnectApGse.tGseTime.pMinUnit = NULL;

        free(pPubGcb->tDataSet.pDateSetName);
        pPubGcb->tDataSet.pDateSetName = NULL;
        fcdacnt = pPubGcb->tDataSet.iFcdaCnt;
        for(j = 0; j < fcdacnt; j++)
        {
            pPubFcda = &(pPubGcb->tDataSet.pDataSetFcda[j]);
            free(pPubFcda->pBType);
            pPubFcda->pBType = NULL;
            free(pPubFcda->pDaName);
            pPubFcda->pDaName= NULL;
            free(pPubFcda->pDoName);
            pPubFcda->pDoName= NULL;
            free(pPubFcda->pFC);
            pPubFcda->pFC= NULL;
            free(pPubFcda->pLdInst);
            pPubFcda->pLdInst= NULL;
            free(pPubFcda->pLnClass);
            pPubFcda->pLnClass= NULL;
            free(pPubFcda->pLnInst);
            pPubFcda->pLnInst= NULL;
            free(pPubFcda->pPrefix);
            pPubFcda->pPrefix= NULL;
            free(pPubFcda->pFcdaDesc);
            pPubFcda->pFcdaDesc= NULL;
            free(pPubFcda->tDai.pDaiName);
            pPubFcda->tDai.pDaiName= NULL;
            free(pPubFcda->tDai.pSAddr);
            pPubFcda->tDai.pSAddr= NULL;
            free(pPubFcda->tDai.tSAddr.pBay);
            pPubFcda->tDai.tSAddr.pBay= NULL;
            free(pPubFcda->tDai.tSAddr.pTDI);
            pPubFcda->tDai.tSAddr.pTDI= NULL;
            free(pPubFcda->tDai.tSAddr.pType);
            pPubFcda->tDai.tSAddr.pType= NULL;
            free(pPubFcda->tDai.tSAddr.pYbId);
            pPubFcda->tDai.tSAddr.pYbId= NULL;
        }
        free(pPubGcb->tDataSet.pDataSetFcda);
    }
    free(pPubGse->pPubGcbInfo);
    pPubGse->pPubGcbInfo = NULL;

    return res;
}

/*
描述: 释放内存,CCD Smv
参数:    NONE
返回值: 释放是否成功
 */
int EDP_FreeCCDSmv()
{
    int i,j;
    PROCESS_SUB_SMV *pSubSmvCfg;
    EP_STATUS res = EP_SUCCESS;

    pSubSmvCfg = (PROCESS_SUB_SMV *)EDP_GetProcessSubSmv();
    for(i = 0; i < pSubSmvCfg->iSubSmvCnt; i++)
    {
        free(pSubSmvCfg->pSubSmvInfo[i].pIntAddrName);
        pSubSmvCfg->pSubSmvInfo[i].pIntAddrName=NULL;
        free(pSubSmvCfg->pSubSmvInfo[i].pSVCBrefName);
        pSubSmvCfg->pSubSmvInfo[i].pSVCBrefName=NULL;
        for(j = 0; j < pSubSmvCfg->pSubSmvInfo[i].tDataSet.iFcdaCnt; j++)
        {
            free(pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pBType);
            pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pBType = NULL;
            free(pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pDaName);
            pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pDaName= NULL;
            free(pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pDoName);
            pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pDoName= NULL;
            free(pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pFC);
            pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pFC= NULL;
            free(pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pFcdaDesc);
            pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pFcdaDesc= NULL;
            free(pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pIntAddr);
            pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pIntAddr= NULL;
            free(pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pLdInst);
            pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pLdInst= NULL;
            free(pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pLnClass);
            pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pLnClass= NULL;
            free(pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pLnInst);
            pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pLnInst= NULL;
            free(pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pPrefix);
            pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda[j].pPrefix= NULL;
        }
        free(pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda);
        pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda=NULL;
        free(pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDateSetName);
        pSubSmvCfg->pSubSmvInfo[i].tDataSet.pDataSetFcda=NULL;

        free(pSubSmvCfg->pSubSmvInfo[i].tConnectAp.pApIedName);
        pSubSmvCfg->pSubSmvInfo[i].tConnectAp.pApIedName=NULL;
        free(pSubSmvCfg->pSubSmvInfo[i].tConnectAp.pApName);
        pSubSmvCfg->pSubSmvInfo[i].tConnectAp.pApName=NULL;

        free(pSubSmvCfg->pSubSmvInfo[i].tConnectAp.tConnectApSmv.pCbName);
        pSubSmvCfg->pSubSmvInfo[i].tConnectAp.tConnectApSmv.pCbName=NULL;
        free(pSubSmvCfg->pSubSmvInfo[i].tConnectAp.tConnectApSmv.pLdInst);
        pSubSmvCfg->pSubSmvInfo[i].tConnectAp.tConnectApSmv.pLdInst=NULL;

        free(pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.pDatSet);
        pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.pDatSet=NULL;
        free(pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.pMultiCast);
        pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.pMultiCast=NULL;
        free(pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.pSvCtrlName);
        pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.pSvCtrlName=NULL;
        free(pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.pSvID);
        pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.pSvID=NULL;

        free(pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.tSvCtrlOpts.pDataRef);
        pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.tSvCtrlOpts.pDataRef=NULL;
        free(pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.tSvCtrlOpts.pRefreshTime);
        pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.tSvCtrlOpts.pRefreshTime=NULL;
        free(pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.tSvCtrlOpts.pSampleRate);
        pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.tSvCtrlOpts.pSampleRate=NULL;
        free(pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.tSvCtrlOpts.pSampleSyn);
        pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.tSvCtrlOpts.pSampleSyn=NULL;
        free(pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.tSvCtrlOpts.pSecurity);
        pSubSmvCfg->pSubSmvInfo[i].tSvCtrl.tSvCtrlOpts.pSecurity=NULL;
    }
    free(pSubSmvCfg->pSubSmvInfo);
    pSubSmvCfg->pSubSmvInfo = NULL;

    return res;
}

/*
描述: 释放内存,CCD
参数:    NONE
返回值: 释放是否成功
 */
int EDP_FreeCCD()
{
    EP_STATUS res = EP_SUCCESS;

    if(EDP_FreeCCDGse() != EP_SUCCESS)
    {
        logMsg("#######  EDP_FreeCCDGse执行出错\n",0,0,0,0,0,0);
        res = EP_ERROR;
    }
    if(EDP_FreeCCDSmv() != EP_SUCCESS)
    {
        logMsg("#######  EDP_FreeCCDSmv执行出错\n",0,0,0,0,0,0);
        res = EP_ERROR;
    }

    return res;
}

/*
描述:调试函数-打印出process.xml 中的sub goose配置信息
参数:     无
返回值: TRUE
 */
int EDP_ShowProcessSubGoose()
{
    int iTmp = 0;
    int iTmp2 = 0;
    int j = 0;
    int i = 0;
    PROCESS_SUB_GSE *pSubGse = NULL;
    SUB_GCB_INFO *pSubGcbInfo = NULL;
    GSE_DATESET_FCDA_SUB *pFcda = NULL;
    pSubGse = EDP_GetProcessSubGoose();

    CFG_LOG_UTF8("\n<-------------------------SUB GOOSE信息----------------------------->^_^\n",0,0,0,0,0,0);

    iTmp = pSubGse->iSubGsCnt;
    if(iTmp > 0)
    {
        pSubGcbInfo = pSubGse->pSubGcbInfo;
    }
    CFG_LOG_UTF8("sub总个数: %d \n",iTmp,0,0,0,0,0);
    while(iTmp > 0 && pSubGcbInfo != NULL)
    {
        CFG_LOG_UTF8("GCB Index %d , GOCBref Name = %s\n",pSubGcbInfo->iIndex,(int)pSubGcbInfo->pGOCBrefName,0,0,0,0);

        /***GSEControl****/
        CFG_LOG_UTF8("\tGSEControl: appID = %s confRev=%d datSet=%s name=%s type=%s\n",
                     (int)pSubGcbInfo->tGseCtrl.pAppID,pSubGcbInfo->tGseCtrl.iConfRev,(int)pSubGcbInfo->tGseCtrl.pDatSet,(int)pSubGcbInfo->tGseCtrl.pCtrlName,(int)pSubGcbInfo->tGseCtrl.pCtrlType,0);
        /***GSEControl End****/

        /***ConnectedAP****/
        CFG_LOG_UTF8("\tConnectedAP: apName=%s iedName=%s\n",
                     (int)pSubGcbInfo->tConnectAp.pApName,(int)pSubGcbInfo->tConnectAp.pApIedName,0,0,0,0);

        CFG_LOG_UTF8("\t\tGSE: cbName=%s ldInst=%s\n",
                     (int)pSubGcbInfo->tConnectAp.tConnectApGse.pCbName,(int)pSubGcbInfo->tConnectAp.tConnectApGse.pLdInst,0,0,0,0);
        iTmp2 = pSubGcbInfo->tConnectAp.tConnectApGse.iAddrCnt;
        j = 0;
        while(iTmp2 > 0)
        {
            CFG_LOG_UTF8("\t\t\tAddress Index %d: APPID=%04X VLAN-ID=%u VLAN-PRIORITY=%u ",
                         pSubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].iIndex,
                         pSubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].usAppID,
                         pSubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].usVID,
                         pSubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].usVPri,0,0);
            CFG_LOG_UTF8(" MAC-Address= %02X-%02X-%02X-%02X-%02X-%02X\n",
                         pSubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].aDstMac[0],
                         pSubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].aDstMac[1],
                         pSubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].aDstMac[2],
                         pSubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].aDstMac[3],
                         pSubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].aDstMac[4],
                         pSubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].aDstMac[5]);
            iTmp2--;
            j++;
        }
        CFG_LOG_UTF8("\t\t\tMinTime = %d , MaxTime = %d \n",
                     pSubGcbInfo->tConnectAp.tConnectApGse.tGseTime.iMinTime,
                     pSubGcbInfo->tConnectAp.tConnectApGse.tGseTime.iMaxTime,0,0,0,0);
        /***ConnectedAP End****/

        /***DataSet****/
        CFG_LOG_UTF8("\tDataSet: name=%s \n",
                     (int)pSubGcbInfo->tDataSet.pDateSetName,0,0,0,0,0);
        iTmp2 = pSubGcbInfo->tDataSet.iFcdaCnt;
        j = 0;
        if(iTmp2 > 0)
            pFcda = pSubGcbInfo->tDataSet.pDataSetFcda;
        while(iTmp2 > 0)
        {
            CFG_LOG_UTF8("\t\tFCDA Index: %d, bType=%s daName=%s doName=%s fc=%s ldInst=%s ",
                         pFcda->iIndex,
                         (int)pFcda->pBType,
                         (int)pFcda->pDaName,
                         (int)pFcda->pDoName,
                         (int)pFcda->pFC,
                         (int)pFcda->pLdInst);
            CFG_LOG_UTF8("  lnClass=%s lnInst=%s prefix=%s  \n\t\t\tintAddr name:%s\n\t\t\t\tDAI name:%s,sAddr:%s\n",
                         (int)pFcda->pLnClass,
                         (int)pFcda->pLnInst,
                         (int)pFcda->pPrefix,
                         (int)pFcda->pIntAddr[0].pIntaddrName,
                         (int)pFcda->pIntAddr[0].tDai.pDaiName,
                         (int)pFcda->pIntAddr[0].tDai.pSAddr);
            /*if(pFcda->tIntAddr.bIsIntAddrValid)
            {
                CFG_LOG_UTF8("\t\t\tintAddr name:%s\n",
                    (int)pFcda->tIntAddr.pIntaddrName,0,0,0,0,0);

                if(pFcda->tIntAddr.bIsDaiValid)
                {
                    CFG_LOG_UTF8("\t\t\t\tDAI name:%s,sAddr:%s\n",
                        (int)pFcda->tIntAddr.tDai.pDaiName,
                        (int)pFcda->tIntAddr.tDai.pSAddr,0,0,0,0);
                }
            }*/
            iTmp2--;
            j++;
            pFcda = pFcda->next;
        }
        /***DataSet End****/

        pSubGcbInfo = pSubGcbInfo->next;
        iTmp--;
        i++;
    }
    CFG_LOG("\n<-------------------------SUB GOOSE信息End----------------------------->^_^\n",0,0,0,0,0,0);

    return TRUE;
}

/*
描述:调试函数-打印出process.xml 中的pub goose配置信息
参数:     无
返回值: TRUE
 */
int EDP_ShowProcessPubGoose()
{
    int iTmp = 0;
    int iTmp2 = 0;
    int j = 0;
    int i = 0;
    PROCESS_PUB_GSE *pPubGse = NULL;
    PUB_GCB_INFO *pPubGcbInfo = NULL;
    GSE_DATESET_FCDA_PUB *pFcda = NULL;
    pPubGse = EDP_GetProcessPubGoose();

    CFG_LOG("\n<-------------------------PUB GOOSE信息----------------------------->^_^\n",0,0,0,0,0,0);
    iTmp = pPubGse->iPubGsCnt;
    if(iTmp > 0)
    {
        pPubGcbInfo = pPubGse->pPubGcbInfo;
    }
    CFG_LOG_UTF8("pub总个数: %d \n",iTmp,0,0,0,0,0);
    while(iTmp > 0 && pPubGcbInfo != NULL)
    {
        CFG_LOG_UTF8("GCB Index %d , GOCBref Name = %s\n",pPubGcbInfo->iIndex,(int)pPubGcbInfo->pGOCBrefName,0,0,0,0);

        /***GSEControl****/
        CFG_LOG_UTF8("\tGSEControl: appID = %s confRev=%d datSet=%s name=%s type=%s\n",
                     (int)pPubGcbInfo->tGseCtrl.pAppID,pPubGcbInfo->tGseCtrl.iConfRev,(int)pPubGcbInfo->tGseCtrl.pDatSet,(int)pPubGcbInfo->tGseCtrl.pCtrlName,(int)pPubGcbInfo->tGseCtrl.pCtrlType,0);
        /***GSEControl End****/

        /***ConnectedAP****/
        CFG_LOG_UTF8("\tConnectedAP: apName=%s iedName=%s\n",
                     (int)pPubGcbInfo->tConnectAp.pApName,(int)pPubGcbInfo->tConnectAp.pApIedName,0,0,0,0);

        CFG_LOG_UTF8("\t\tGSE: cbName=%s ldInst=%s\n",
                     (int)pPubGcbInfo->tConnectAp.tConnectApGse.pCbName,(int)pPubGcbInfo->tConnectAp.tConnectApGse.pLdInst,0,0,0,0);
        iTmp2 = pPubGcbInfo->tConnectAp.tConnectApGse.iAddrCnt;
        j = 0;
        while(iTmp2 > 0)
        {
            CFG_LOG_UTF8("\t\t\tAddress Index %d: APPID=%04X VLAN-ID=%u VLAN-PRIORITY=%u ",
                         pPubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].iIndex,
                         pPubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].usAppID,
                         pPubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].usVID,
                         pPubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].usVPri,0,0);
            CFG_LOG_UTF8(" MAC-Address= %02X-%02X-%02X-%02X-%02X-%02X\n",
                         pPubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].aDstMac[0],
                         pPubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].aDstMac[1],
                         pPubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].aDstMac[2],
                         pPubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].aDstMac[3],
                         pPubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].aDstMac[4],
                         pPubGcbInfo->tConnectAp.tConnectApGse.pGseAddr[j].aDstMac[5]);
            iTmp2--;
            j++;
        }
        CFG_LOG_UTF8("\t\t\tMinTime = %d , MaxTime = %d \n",
                     pPubGcbInfo->tConnectAp.tConnectApGse.tGseTime.iMinTime,
                     pPubGcbInfo->tConnectAp.tConnectApGse.tGseTime.iMaxTime,0,0,0,0);
        iTmp2 = pPubGcbInfo->tConnectAp.iPhysConnCnt;
        for(j = 0; j < iTmp2; j++)

        {
            CFG_LOG_UTF8("\t\tPhysConn Index %d, type = %s,  Port = %s, Plug = %s, Cable = %s, Type = %s\n",
                         pPubGcbInfo->tConnectAp.tConnectApPhysConn[j].iIndex,
                         (int)pPubGcbInfo->tConnectAp.tConnectApPhysConn[j].pPhysType,
                         (int)pPubGcbInfo->tConnectAp.tConnectApPhysConn[j].pPort,
                         (int)pPubGcbInfo->tConnectAp.tConnectApPhysConn[j].pPlug,
                         (int)pPubGcbInfo->tConnectAp.tConnectApPhysConn[j].pCable,
                         (int)pPubGcbInfo->tConnectAp.tConnectApPhysConn[j].pSpeedType);
        }
        /***ConnectedAP End****/

        /***DataSet****/
        CFG_LOG_UTF8("\tDataSet: name=%s \n",
                     (int)pPubGcbInfo->tDataSet.pDateSetName,0,0,0,0,0);
        iTmp2 = pPubGcbInfo->tDataSet.iFcdaCnt;
        j = 0;
        if(iTmp2 > 0)
            pFcda = pPubGcbInfo->tDataSet.pDataSetFcda;
        while(iTmp2 > 0)
        {
            CFG_LOG_UTF8("\t\t FCDA Index: %d, bType=%s daName=%s doName=%s fc=%s ldInst=%s ",
                         pFcda->iIndex,
                         (int)pFcda->pBType,
                         (int)pFcda->pDaName,
                         (int)pFcda->pDoName,
                         (int)pFcda->pFC,
                         (int)pFcda->pLdInst);
            CFG_LOG_UTF8("  lnClass=%s lnInst=%s prefix=%s\n",
                         (int)pFcda->pLnClass,
                         (int)pFcda->pLnInst,
                         (int)pFcda->pPrefix,0,0,0);
            CFG_LOG_UTF8("\t\t\tDAI name:%s,sAddr:%s\n",
                         (int)pFcda->tDai.pDaiName,
                         (int)pFcda->tDai.pSAddr,0,0,0,0);
            iTmp2--;
            j++;
            pFcda = pFcda->next;
        }
        /***DataSet End****/

        pPubGcbInfo = pPubGcbInfo->next;
        iTmp--;
        i++;
    }
    CFG_LOG("\n<-------------------------PUB GOOSE信息End----------------------------->^_^\n",0,0,0,0,0,0);

    return TRUE;
}

/*
描述:调试函数-打印出process.xml 中的sub smv配置信息
参数:     无
返回值: TRUE
 */
int EDP_ShowProcessSubSmv()
{
    int iTmp = 0;
    int iTmp2 = 0;
    int iTmp3 = 0;
    int k = 0;
    int j = 0;
    int i = 0;
    PROCESS_SUB_SMV *pSubSmv = NULL;
    SMV_DATESET_FCDA_SUB *pFcda = NULL;
    SUB_SMV_INFO *pSubSmvInfo = NULL;
    SMV_DATESET_FCDA_INTADDR *pIntAddr = NULL;

    pSubSmv = EDP_GetProcessSubSmv();

    CFG_LOG("\n<-------------------------SUB SMV信息----------------------------->^_^\n",0,0,0,0,0,0);

    iTmp = pSubSmv->iSubSmvCnt;
    if(iTmp > 0)
    {
        pSubSmvInfo = pSubSmv->pSubSmvInfo;
    }
    while(iTmp > 0 && pSubSmvInfo != NULL)
    {
        CFG_LOG_UTF8("SMV Index %d , SMVCBref Name = %s, iUsedIntaddrCnt = %d, iUsedFcdaCnt = %d\n",
                     pSubSmvInfo->iIndex,(int)pSubSmvInfo->pSVCBrefName,pSubSmvInfo->iUsedIntaddrCnt,pSubSmvInfo->iUsedFcdaCnt,0,0);
        CFG_LOG_UTF8("    IndexInHsbAddr %d , HSBAddr = %d, IsDoubleNet = %d, ulSelect1 = 0x%08x,ulSelect2 = 0x%08x\n",
                     pSubSmvInfo->iIndexInHsbAddr,(int)pSubSmvInfo->iHSBAddr,pSubSmvInfo->bIsDoubleNet,(int)pSubSmvInfo->ulSelect1,(int)pSubSmvInfo->ulSelect2,0);

        /***SampledValueControl****/
        CFG_LOG_UTF8("\tSVControl: confRev=%d datSet=%s name=%s nofASDU=%d smpRate=%d smvID=%s \n",
                     pSubSmvInfo->tSvCtrl.iConfRev,
                     (int)pSubSmvInfo->tSvCtrl.pDatSet,
                     (int)pSubSmvInfo->tSvCtrl.pSvCtrlName,
                     pSubSmvInfo->tSvCtrl.iAsduCnt,
                     pSubSmvInfo->tSvCtrl.iSmpRate,
                     (int)pSubSmvInfo->tSvCtrl.pSvID);
        CFG_LOG_UTF8("\t\tSmvOpts: dataRef=%s refreshTime=%s sampleRate=%s sampleSynchronized=%s security=%s \n",
                     (int)pSubSmvInfo->tSvCtrl.tSvCtrlOpts.pDataRef,
                     (int)pSubSmvInfo->tSvCtrl.tSvCtrlOpts.pRefreshTime,
                     (int)pSubSmvInfo->tSvCtrl.tSvCtrlOpts.pSampleRate,
                     (int)pSubSmvInfo->tSvCtrl.tSvCtrlOpts.pSampleSyn,
                     (int)pSubSmvInfo->tSvCtrl.tSvCtrlOpts.pSecurity,0);
        /***SampledValueControl End****/

        /***ConnectedAP****/
        CFG_LOG_UTF8("\tConnectedAP: apName=%s iedName=%s\n",
                     (int)pSubSmvInfo->tConnectAp.pApName,(int)pSubSmvInfo->tConnectAp.pApIedName,0,0,0,0);
        CFG_LOG_UTF8("\t\tSMV: cbName=%s ldInst=%s\n",
                     (int)pSubSmvInfo->tConnectAp.tConnectApSmv.pCbName,(int)pSubSmvInfo->tConnectAp.tConnectApSmv.pLdInst,0,0,0,0);
        iTmp2 = pSubSmvInfo->tConnectAp.tConnectApSmv.iAddrCnt;
        j = 0;
        while(iTmp2 > 0)
        {
            CFG_LOG_UTF8("\t\t\tAddress Index %d: APPID=%04X VLAN-ID=%u VLAN-PRIORITY=%u ",
                         pSubSmvInfo->tConnectAp.tConnectApSmv.pSmvAddr[j].iIndex,
                         pSubSmvInfo->tConnectAp.tConnectApSmv.pSmvAddr[j].usAppID,
                         pSubSmvInfo->tConnectAp.tConnectApSmv.pSmvAddr[j].usVID,
                         pSubSmvInfo->tConnectAp.tConnectApSmv.pSmvAddr[j].usVPri,0,0);
            CFG_LOG_UTF8(" MAC-Address= %02X-%02X-%02X-%02X-%02X-%02X\n",
                         pSubSmvInfo->tConnectAp.tConnectApSmv.pSmvAddr[j].aDstMac[0],
                         pSubSmvInfo->tConnectAp.tConnectApSmv.pSmvAddr[j].aDstMac[1],
                         pSubSmvInfo->tConnectAp.tConnectApSmv.pSmvAddr[j].aDstMac[2],
                         pSubSmvInfo->tConnectAp.tConnectApSmv.pSmvAddr[j].aDstMac[3],
                         pSubSmvInfo->tConnectAp.tConnectApSmv.pSmvAddr[j].aDstMac[4],
                         pSubSmvInfo->tConnectAp.tConnectApSmv.pSmvAddr[j].aDstMac[5]);
            iTmp2--;
            j++;
        }
        /***ConnectedAP End****/

        /***DataSet****/
        CFG_LOG_UTF8("\tDataSet: name=%s \n",
                     (int)pSubSmvInfo->tDataSet.pDateSetName,0,0,0,0,0);
        iTmp2 = pSubSmvInfo->tDataSet.iFcdaCnt;
        j = 0;
        if(iTmp2 > 0)
            pFcda = pSubSmvInfo->tDataSet.pDataSetFcda;
        while(iTmp2 > 0)
        {
            CFG_LOG_UTF8("\t\tFCDA Index: %d, bType=%s daName=%s doName=%s fc=%s ldInst=%s ",
                         pFcda->iIndex,
                         (int)pFcda->pBType,
                         (int)pFcda->pDaName,
                         (int)pFcda->pDoName,
                         (int)pFcda->pFC,
                         (int)pFcda->pLdInst);
            CFG_LOG_UTF8("  lnClass=%s lnInst=%s prefix=%s  \n",
                         (int)pFcda->pLnClass,
                         (int)pFcda->pLnInst,
                         (int)pFcda->pPrefix,0,0,0);
            iTmp3 = pFcda->iIntAddrCnt;
            k = 0;
            if(iTmp3 > 0)
                pIntAddr = pFcda->pIntAddr;
            while(iTmp3 > 0 && pIntAddr != NULL)
            {
                CFG_LOG_UTF8("\t\t\tintAddr Index %d, name:%s\n",
                             pIntAddr->iIndex,
                             (int)pIntAddr->pIntaddrName,0,0,0,0);
                CFG_LOG_UTF8("\t\t\t\tDAI name:%s,sAddr:%s\n",
                             (int)pIntAddr->tDai.pDaiName,
                             (int)pIntAddr->tDai.pSAddr,0,0,0,0);
                iTmp3--;
                k++;
                pIntAddr = pIntAddr->next;
            }
            iTmp2--;
            j++;
            pFcda = pFcda->next;
        }
        /***DataSet End****/

        pSubSmvInfo = pSubSmvInfo->next;
        iTmp--;
        i++;
    }
    CFG_LOG("\n<-------------------------SUB SMV信息End----------------------------->^_^\n",0,0,0,0,0,0);

    return TRUE;
}

/*
描述:调试函数-打印出process.xml 中的pub smv配置信息
参数:     无
返回值: TRUE
 */
int EDP_ShowProcessPubSmv()
{
    /*int i = 0;*/
    PROCESS_PUB_SMV *pPubSmv = NULL;
    pPubSmv = EDP_GetProcessPubSmv();

    CFG_LOG("\n<-------------------------PUB SMV信息----------------------------->^_^\n",0,0,0,0,0,0);

    CFG_LOG("\n<-------------------------PUB SMV信息End----------------------------->^_^\n",0,0,0,0,0,0);

    return TRUE;
}

/*
描述:调试函数-打印出process.xml 中的private配置信息
参数:     无
返回值: TRUE
 */
int EDP_ShowProcessPrivate()
{
    /*int i = 0;*/
    PROCESS_PRIVATE *pPrivate = NULL;
    pPrivate = EDP_GetProcessCrc();

    CFG_LOG("\n<-------------------------PRIVATE信息----------------------------->^_^\n",0,0,0,0,0,0);

    CFG_LOG_UTF8("CRC32: = 0x%08X timestmap: %s \n",pPrivate->ulCrc,(int)pPrivate->pTimeStamp,0,0,0,0);

    CFG_LOG("\n<-------------------------PRIVATE信息End----------------------------->^_^\n",0,0,0,0,0,0);

    return TRUE;
}

/*
描述:调试函数-打印出process.xml 配置信息
参数:     无
返回值: TRUE
 */
int EDP_ShowProcessXml()
{
    /*int i = 0;*/

    CFG_LOG("\n<-------------------------开始打印----------------------------->^_^\n",0,0,0,0,0,0);
    CFG_LOG_UTF8("IEDNAME: %s\n",(int)g_tProcessBusCfg.pIEDName,0,0,0,0,0);

    EDP_ShowProcessSubGoose();
    EDP_ShowProcessPubGoose();
    EDP_ShowProcessSubSmv();
    EDP_ShowProcessPubSmv();
    EDP_ShowProcessPrivate();

    CFG_LOG("\n<-------------------------打印结束----------------------------->^_^\n",0,0,0,0,0,0);

    return TRUE;
}

