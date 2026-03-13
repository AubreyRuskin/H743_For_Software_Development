/**************************************************************************
EDP_UnifiedCfgFile.c

九统一配置文件生成SV+GS.XML 文件,并进行zip存储的压缩

Copyright (c) 2014 SNAC(Guodian Nanjing Automation Co., Ltd.)
All Rights Reserved.

History:
Revision     Date          Who            Comment
-----------------------------------------------------------------
V1.00         2015.03.03    kevin         初始版本
***************************************************************************/


#include "EDP_UnifiedCfgFile.h"
#include "EDP_UnifiedCfgParseConfig.h"
#include "EDP_UnifiedCfgParseCCD.h"
#include "EDP_UnifiedCfgParsePrivate.h"
#include "EDP_UnifiedCfgMain.h"
#include "iecgoose.h"
#include "smvcfg.h"
#include "edp_asst.h"
#include <sys_stat_compat.h>
#include "mxml.h"

int g_iCcGcbCnt = 0;        /*CC转发的goose控制块个数，包括CCD中的发送和接收GS总和*/

CC_GS_POOL g_tCCGsPool;     /*CC的转发结构*/
CC_SV_POOL g_tCCSvPool;     /*CC板SV配置的转发结构*/
GS_TRANS_INFO *pGsTransInfo11;

uint8_t g_ucCCCnt = 0;        /*CC板个数*/
CC_TRANSMIT_INFO g_tCCTransmitInfo;      /*CC转发结构*/

BOOL g_bCcIsUsed[MAX_CC_BOARD_ID_NUM]; /*以BoardId为序号，表示当前CC是否使用*/
FPGA_CC_CFG_INFO g_tFpgaCcCfgInfo[MAX_CC_BOARD_ID_NUM]; /* 以BoardId为序号, 表示CC板的运行配置状态*/
BOOL g_bCfgChgFlag[2];
FPGA_CFG_FILE_INFO sFpgaCCInfo[CPU_PORT_NUM][FPGA_PORT_NUM];

extern mxml_node_t *mxmlNewElement(mxml_node_t *parent, const char  *name);
extern mxml_node_t *mxmlNewXML(const char *version);
extern void mxmlElementSetAttr(mxml_node_t *node, const char  *name, const char  *value);
extern mxml_node_t *mxmlNewText(mxml_node_t *parent,  int whitespace, const char  *string);
extern int g_ucCpuNodeAddr;

/*
* 'whitespace_cb()' - Let the mxmlSaveFile() function know when to insert
*                     newlines and tabs...
*/

const char *				/* O - Whitespace string or NULL */
whitespace_cb(mxml_node_t *node,	/* I - Element node */
              int         where)	/* I - Open or close tag? */
{
    const char	*name;			/* Name of element */
    mxml_node_t	*parent;		/* Parent node */
    int		level;			/* Indentation level */
    static const char *tabs = "\t\t\t\t\t\t\t\t";
    /* Tabs for indentation */


    /*
    * We can conditionally break to a new line before or after any element.
    * These are just common HTML elements...
    */

    name = node->value.element.name;
    if (!strcmp(name, "Config") ||
            !strcmp(name, "DATAINFO") ||
            !strcmp(name, "DATAMAP")||
            !strcmp(name, "SUB_GOOSE") ||
            !strcmp(name, "PUB_GOOSE") ||
            !strcmp(name, "SUB_DATASET") ||
            !strcmp(name, "PUB_DATASET") ||
            !strcmp(name, "GCB") ||
            !strcmp(name, "NET") ||
            !strcmp(name, "Address")||
            !strcmp(name, "PORT")||
            !strcmp(name, "DST_MAC")||
            !strcmp(name, "DST_Address")||
            !strcmp(name, "DATASET")||
            !strcmp(name, "FCDA"))
    {
        if (( where == MXML_WS_AFTER_OPEN && strcmp(name, "PORT") && strcmp(name, "DST_MAC"))|| (where == MXML_WS_AFTER_CLOSE))
            return("\n");
        else if (!strcmp(name, "DATAINFO") || !strcmp(name, "DATAMAP"))
        {
            return("\t");
        }
        else if (!strcmp(name, "SUB_GOOSE") || !strcmp(name, "PUB_GOOSE") || !strcmp(name, "SUB_DATASET") || !strcmp(name, "PUB_DATASET")  )
        {
            return("\t\t");
        }
        else if (!strcmp(name, "GCB"))
        {
            return("\t\t\t");
        }
        else if (!strcmp(name, "NET") ||!strcmp(name, "DATASET"))
        {
            return("\t\t\t\t");
        }
        else if (!strcmp(name, "Address") || !strcmp(name, "FCDA"))
        {
            return("\t\t\t\t\t");
        }
        else if (!strcmp(name, "PORT") && where == MXML_WS_BEFORE_OPEN )
        {
            return("\t\t\t\t\t\t");
        }
        else if (!strcmp(name, "DST_MAC") && where == MXML_WS_BEFORE_OPEN )
        {
            return("\t\t\t\t\t\t");
        }
        else if (!strcmp(name, "DST_Address"))
        {
            return("\t\t\t\t\t");
        }
    }
    else if (!strncmp(name, "?xml", 4))
    {
        if (where == MXML_WS_AFTER_OPEN)
            return ("\n");
        else
            return (NULL);
    }
    else if (where == MXML_WS_BEFORE_OPEN ||
             ((!strcmp(name, "choice") || !strcmp(name, "option")) &&
              where == MXML_WS_BEFORE_CLOSE))
    {
        for (level = -1, parent = node->parent;
                parent;
                level ++, parent = parent->parent);

        if (level > 8)
            level = 8;
        else if (level < 0)
            level = 0;

        return (tabs + 8 - level);
    }
    else if (where == MXML_WS_AFTER_CLOSE ||
             ((!strcmp(name, "group") || !strcmp(name, "option") ||
               !strcmp(name, "choice")) &&
              where == MXML_WS_AFTER_OPEN))
        return ("\n");
    else if (where == MXML_WS_AFTER_OPEN && !node->child)
        return ("\n");

    /*
    * Return NULL for no added whitespace...
    */

    return (NULL);
}

/*
* 'whitespace_cb_sv()' - Let the mxmlSaveFile() function know when to insert
*                     newlines and tabs...
*/

const char *				/* O - Whitespace string or NULL */
whitespace_cb_sv(mxml_node_t *node,	/* I - Element node */
                 int         where)	/* I - Open or close tag? */
{
    const char	*name;			/* Name of element */
    mxml_node_t	*parent;		/* Parent node */
    int		level;			/* Indentation level */
    static const char *tabs = "\t\t\t\t\t\t\t\t";
    /* Tabs for indentation */


    /*
    * We can conditionally break to a new line before or after any element.
    * These are just common HTML elements...
    */

    name = node->value.element.name;
    if (!strcmp(name, "Config") ||
            !strcmp(name, "SMV_9_1") ||
            !strcmp(name, "DataSet") ||
            !strcmp(name, "CHN_MAP") ||
            !strcmp(name, "CFG") ||
            !strcmp(name, "SMV_P2P") ||
            !strcmp(name, "SUB_SMV_P2P") ||
            !strcmp(name, "PUB_SMV_P2P") ||
            !strcmp(name, "ASDU"))
    {
        if((where == MXML_WS_AFTER_OPEN) || (where == MXML_WS_AFTER_CLOSE))
            return("\n");
        if (!strcmp(name, "SMV_9_1") || !strcmp(name, "SMV_P2P"))
        {
            return("\t");
        }
        else if (!strcmp(name, "DataSet") || !strcmp(name, "SUB_SMV_P2P") || !strcmp(name, "PUB_SMV_P2P"))
        {
            return("\t\t");
        }
        else if (!strcmp(name, "CHN_MAP") || !strcmp(name, "ASDU"))
        {
            return("\t\t\t");
        }
    }
    else if (!strncmp(name, "?xml", 4))
    {
        if (where == MXML_WS_AFTER_OPEN)
            return ("\n");
        else
            return (NULL);
    }
    else if (where == MXML_WS_BEFORE_OPEN ||
             ((!strcmp(name, "choice") || !strcmp(name, "option")) &&
              where == MXML_WS_BEFORE_CLOSE))
    {
        for (level = -1, parent = node->parent;
                parent;
                level ++, parent = parent->parent);

        if (level > 8)
            level = 8;
        else if (level < 0)
            level = 0;

        return (tabs + 8 - level);
    }
    else if (where == MXML_WS_AFTER_CLOSE ||
             ((!strcmp(name, "group") || !strcmp(name, "option") ||
               !strcmp(name, "choice")) &&
              where == MXML_WS_AFTER_OPEN))
        return ("\n");
    else if (where == MXML_WS_AFTER_OPEN && !node->child)
        return ("\n");

    /*
    * Return NULL for no added whitespace...
    */

    return (NULL);
}

/*
* 'whitespace_cb_crc()' - Let the mxmlSaveFile() function know when to insert
*                     newlines and tabs...
*/

const char *				/* O - Whitespace string or NULL */
whitespace_cb_crc(mxml_node_t *node,	/* I - Element node */
                  int         where)	/* I - Open or close tag? */
{
    /*
    * We can conditionally break to a new line before or after any element.
    * These are just common HTML elements...
    */

    if (where == MXML_WS_BEFORE_OPEN)
        return("\n");

    /*
    * Return NULL for no added whitespace...
    */

    return (NULL);
}


BOOL ProcessCfgZip(int iCount,char (* filepath)[128],char (* filename)[128],FILE* id_output)
{
    char* buf=NULL;
    struct stat filestat;
    unsigned int crc;
    int filenamelength;
    char *p=NULL;
    int mulusize=0;
    int muluoffset=0;
    int i=0;
    unsigned int *offset=NULL;
    int fileLen = 0;
    BOOL bSuccess = FALSE;
    FILE** file=NULL;

    if(id_output == NULL)
    {
        goto ZipErr;
    }

    file = calloc(iCount, sizeof(FILE**));
    if(file == NULL)
    {
        goto ZipErr;
    }

    for(i=0; i<iCount; i++)
    {
        file[i] = fopen((char*)filepath[i],"r");
        if(file[i] == NULL)
        {
            goto ZipErr;
        }
        if (stat(filepath[i], &filestat) < 0)
        {
            goto ZipErr;
        }
        fileLen += filestat.st_size + 256;
    }

    buf = calloc(fileLen,1);
    if(buf == NULL)
    {
        goto ZipErr;
    }

    offset = calloc(iCount, sizeof(int));
    if(offset==NULL)
    {
        goto ZipErr;
    }
    offset[0]=0;

    for(i=0; i<iCount; i++)
    {
        if(i!=0)
            offset[i]=p-buf;
        p=buf;
        if (stat(filepath[i], &filestat) < 0)
        {
            goto ZipErr;
        }

        buf[0]=0x50;
        buf[1]=0x4B;
        buf[2]=0x03;
        buf[3]=0x04;

        buf[4]=0x0A;
        buf[5]=0x00;  //压缩文件pkware 版本

        buf[6]=0x00;  //全局方式位标记
        buf[7]=0x00;

        buf[8]=0x00;  //压缩方式
        buf[9]=0x00;

        buf[10]=filestat.st_mtime;
        buf[11]=filestat.st_mtime>>8; //最后修改时间  查看具体怎么顺序，应该没错
        buf[12]=filestat.st_mtime>>16;
        buf[13]=filestat.st_mtime>>24;
        crc = FT_File_CRC32(filepath[i]);
        buf[14]=crc;
        buf[15]=crc>>8;
        buf[16]=crc>>16;
        buf[17]=crc>>24;  //CRC 查看高低字节顺序 ，应该没错

        buf[18]=filestat.st_size;
        buf[19]=filestat.st_size>>8; //文件大小
        buf[20]=filestat.st_size>>16;
        buf[21]=filestat.st_size>>24;


        buf[22]=filestat.st_size;
        buf[23]=filestat.st_size>>8; //文件大小
        buf[24]=filestat.st_size>>16;
        buf[25]=filestat.st_size>>24;

        filenamelength=strlen(filename[i]);
        buf[26]=filenamelength;
        buf[27]=filenamelength>>8; //文件名长度

        buf[28]=0x00;
        buf[29]=0x00; //扩展记录长度 不知道填什么，填0

        p+=30;


        memcpy(&buf[30],filename[i],filenamelength);
        p+=filenamelength;


        fread(p,filestat.st_size,1,file[i]);
        p+=filestat.st_size;

        fwrite(buf,p-buf,1,id_output);

        muluoffset+=p-buf;

    }
    //目录区

    p=buf;
    for(i=0; i<iCount; i++)
    {
        p=buf;

        if (stat(filepath[i], &filestat) < 0)
            goto ZipErr;
        *p++=0x50;
        *p++=0x4B;
        *p++=0x01;
        *p++=0x02;

        *p++=0x3F;
        *p++=0x00;  //压缩文件pkware 版本
        *p++=0x0A;
        *p++=0x00;  //解压压缩文件pkware 版本

        *p++=0x00;  //全局方式位标记
        *p++=0x00;

        *p++=0x00;  //压缩方式
        *p++=0x00;


        *p++=filestat.st_mtime;
        *p++=filestat.st_mtime>>8; //最后修改时间  查看具体怎么顺序，应该没错
        *p++=filestat.st_mtime>>16;
        *p++=filestat.st_mtime>>24;
        crc =FT_File_CRC32(filepath[i]);
        *p++=crc;
        *p++=crc>>8;
        *p++=crc>>16;
        *p++=crc>>24;  //CRC 查看高低字节顺序 ，应该没错

        *p++=filestat.st_size;
        *p++=filestat.st_size>>8; //文件大小
        *p++=filestat.st_size>>16;
        *p++=filestat.st_size>>24;


        *p++=filestat.st_size;
        *p++=filestat.st_size>>8; //文件大小
        *p++=filestat.st_size>>16;
        *p++=filestat.st_size>>24;


        filenamelength=strlen(filename[i]);
        *p++=filenamelength;
        *p++=filenamelength>>8; //文件名长度

        *p++=0x00;
        *p++=0x00; //扩展记录长度 不知道填什么，填0

        *p++=0x00;  //文件注释长度
        *p++=0x00;

        *p++=0x00;  //磁盘开始号
        *p++=0x00;

        *p++=0x00;  //内部文件属性
        *p++=0x00;

        *p++=0x20;  //外部部文件属性
        *p++=0x00;
        *p++=0x00;
        *p++=0x00;

        *p++=offset[i];  //局部头部偏移量
        *p++=offset[i]>>8;
        *p++=offset[i]>>16;
        *p++=offset[i]>>24;


        memcpy(p,filename[i],filenamelength);
        p+=filenamelength;

        mulusize+=p-buf;
        fwrite(buf,p-buf,1,id_output);
    }


    //压缩源文件目录结束标志
    p=buf;
    *p++=0x50;
    *p++=0x4B;
    *p++=0x05;
    *p++=0x06;

    *p++=0x00;  //前磁盘编号
    *p++=0x00;

    *p++=0x00;  //目录区开始磁盘编号
    *p++=0x00;


    *p++=iCount;
    *p++=iCount>>8; //本磁盘上纪录总数

    *p++=iCount;
    *p++=iCount>>8; //目录区中纪录总数

    *p++=mulusize;
    *p++=mulusize>>8;
    *p++=mulusize>>16;
    *p++=mulusize>>24;  //目录区尺寸大小

    *p++=muluoffset;
    *p++=muluoffset>>8;
    *p++=muluoffset>>16;
    *p++=muluoffset>>24;  //目录区对第一张磁盘的偏移量


    *p++=0x00;  //ZIP 文件注释长度
    *p++=0x00;

    fwrite(buf,p-buf,1,id_output);
    bSuccess = TRUE;

ZipErr:

    if(file != NULL)
    {
        free(file);
    }
    else
    {
        for(i = 0; i < iCount; i++)
        {
            if(file[i] != NULL)
            {
                fclose(file[i]);
            }
        }
    }

    if(offset != NULL)
    {
        free(offset);
    }
    if(buf != NULL)
    {
        free(buf);
    }

    return bSuccess;
}



/*
描述: 判断是否需要转发，主要是根据cbindex的值是否包含索引
如果cbindex为空，则允许转发
参数:
index: gs或者sv的索引
pPortConnect: 转发端口信息
返回值: TRUE转发. FALSE不转发
 */
BOOL EDP_IsCcTrans(int index, BOARD_PORT_CONNECT *pPortConnect)
{
    int i = 0;

    if(pPortConnect->pGsCbIndex == NULL)
    {
        return TRUE;
    }

    for(i = 0; i < pPortConnect->iGsCbCnt; i++)
    {
        if(index == pPortConnect->iGsCbList[i])
        {
            return TRUE;
        }
    }
    return FALSE;
}

/*
功能:查找当前ASDU在主CC发送报文中的排序
参数:pCcSvSubAsduInfo,所需要发送的ASDU的结构
     pPubNo,输出序号
返回:int,成功返回0,否则非0
*/
int EDP_GetSvAsduPubNo(CC_SV_SUB_ASDU_INFO *pCcSvSubAsduInfo, int *pPubNo)
{
    PROCESS_SUB_SMV *pSubSmvCfg;
    int res = 0;

    int i;

    pSubSmvCfg = (PROCESS_SUB_SMV *)EDP_GetProcessSubSmv();

    for(i = 0; i < pSubSmvCfg->iSubSmvCnt; i++)
    {
        if((pSubSmvCfg->pSubSmvInfo[i].iSubCcBoardId == pCcSvSubAsduInfo->iSubCcBoardId)
                &&(pSubSmvCfg->pSubSmvInfo[i].iSubCcPortId== pCcSvSubAsduInfo->iSubCcPortId))
        {
            *pPubNo = i;
            break;
        }
    }

    if(i == pSubSmvCfg->iSubSmvCnt)
    {
        res = -1;
        goto exit;
    }

exit:
    return res;
}

/*
功能:巡检私有配置寻找子机CC和主机CC的板件号,默认存在一个主CC
参数:pMasterSvCcId,输出主机CC板件号
     pSlaverSvCcBoardId,输出子机CC板件号
返回:int,转发SV的CC板数目
*/
int EDP_GetSvCcBoardId(int *pMasterSvCcId, int *pSlaverSvCcBoardId)
{
    PRIVATE_BOARD *pBoard = NULL;
    BOARD_PORT_CONNECT *pPortConnect = NULL;

    int iSvCcBoardCnt = 0;
    int iBoardCnt = 0;
    int i,j;

    iBoardCnt = EDP_GetPrivateBoardCnt();
    for(i = 0; i < iBoardCnt; i++)
    {
        pBoard = EDP_GetPrivateBoardPtrFromIndex(i);
        pPortConnect = pBoard->tPort.pPortConnect;
        if(pBoard->iBoardType==2)
        {
            /*判断是否为CC板*/
            for(j = 0; j < pBoard->tPort.iConnectCnt; j++)
            {
                if((pPortConnect->iDataType == PORT_TRANSFOR_DATA_TYPE_SV)
                        || (pPortConnect->iDataType == PORT_TRANSFOR_DATA_TYPE_SVGS ))
                {
                    /* 类型为SV或者SV+GS*/
                    if(pBoard->iParentCCId == pBoard->iBoardId)
                    {
                        *pMasterSvCcId = pBoard->iBoardId;
                    }
                    else
                    {
                        *pSlaverSvCcBoardId = pBoard->iBoardId;
                        pSlaverSvCcBoardId++;
                    }
                    iSvCcBoardCnt++;
                    break;
                }
                pPortConnect++;
            }
        }
    }
    return iSvCcBoardCnt;
}

/*
描述: 获取子CC板接收信息
参数: pBoard, 子CC板板件信息结构
      pSvCcTransInfo, CC转发信息
返回值: 解析是否成功
 */
int EDP_GetSlaverCcSubInfo(PRIVATE_BOARD *pBoard,  SV_TRANS_INFO *pSvCcTransInfo)
{
    int ret = 0;
    int iSubSmvRefId[12] = {0};
    int iCcSubSmvNum = 0;
    PROCESS_SUB_SMV *pSubSmvCfg = NULL;
    SUB_SMV_INFO *pSubSmvInfo = NULL;
    CC_SV_SUB_INFO *pCcSvSubInfo = NULL;

    int i = 0;

    pSubSmvCfg = (PROCESS_SUB_SMV *)EDP_GetProcessSubSmv();
    for(i = 0; i < pSubSmvCfg->iSubSmvCnt; i++)
    {
        if(pBoard->iBoardId== pSubSmvCfg->pSubSmvInfo[i].iSubCcBoardId)
        {
            iSubSmvRefId[iCcSubSmvNum] = i;
            iCcSubSmvNum++;
        }
    }

    pSvCcTransInfo->bMasterCc = FALSE;
    pSvCcTransInfo->iMaxDelay = pBoard->tSmv.tSmvCommon.iMaxDelay;
    pSvCcTransInfo->pCcSvSubInfo = calloc(iCcSubSmvNum, sizeof(CC_SV_SUB_INFO));
    pCcSvSubInfo = pSvCcTransInfo->pCcSvSubInfo;
    for(i = 0; i < iCcSubSmvNum; i++)
    {
        pSubSmvInfo = &pSubSmvCfg->pSubSmvInfo[iSubSmvRefId[i]];
        pCcSvSubInfo[i].iPort = pSubSmvInfo->iSubCcPortId;
        memcpy(pCcSvSubInfo[i].ucMacAddr, pSubSmvInfo->tConnectAp.tConnectApSmv.pSmvAddr[0].aDstMac, MAC_BYTES);
        pCcSvSubInfo[i].iSrc = 1;
        pCcSvSubInfo[i].iAsduNum = 1;
        pCcSvSubInfo[i].iSubBoard = 0;
        pCcSvSubInfo[i].usTCI = EDP_CC_TCI_DEFAULT;
        pCcSvSubInfo[i].usAppID = pSubSmvInfo->tConnectAp.tConnectApSmv.pSmvAddr[0].usAppID;
        pCcSvSubInfo[i].iGmrp = pBoard->tSmv.tSmvCommon.iGmrp;
        pCcSvSubInfo[i].pCcSvSubAsduInfo = calloc(1, sizeof(CC_SV_SUB_ASDU_INFO));

        pCcSvSubInfo[i].pCcSvSubAsduInfo[0].iChnNum = pSubSmvInfo->tDataSet.iFcdaCnt;
        pCcSvSubInfo[i].pCcSvSubAsduInfo[0].iID = i+1;
        pCcSvSubInfo[i].pCcSvSubAsduInfo[0].iShift = 0;
        pCcSvSubInfo[i].pCcSvSubAsduInfo[0].pSvID = strdup(pSubSmvInfo->tSvCtrl.pSvID);
        pCcSvSubInfo[i].pCcSvSubAsduInfo[0].ulConfRev = pSubSmvInfo->tSvCtrl.iConfRev;
        pCcSvSubInfo[i].pCcSvSubAsduInfo[0].ulDelayChan = 0x01<<pSubSmvInfo->iDelayChnNo;
        pCcSvSubInfo[i].pCcSvSubAsduInfo[0].pSelect1 = pSubSmvInfo->ulSelect1;
        pCcSvSubInfo[i].pCcSvSubAsduInfo[0].pSelect2 = pSubSmvInfo->ulSelect2;
        pCcSvSubInfo[i].pCcSvSubAsduInfo[0].iSubCcBoardId = pSubSmvInfo->iSubCcBoardId;
        pCcSvSubInfo[i].pCcSvSubAsduInfo[0].iSubCcPortId= pSubSmvInfo->iSubCcPortId;
    }

    pSvCcTransInfo->iSubNum = iCcSubSmvNum;
    pSvCcTransInfo->iSubAsduNum = iCcSubSmvNum;

    return ret;
}

/*
描述: 获取子CC板发送信息
参数: pBoard, 子CC板板件信息结构
      pSvCcTransInfo, CC转发信息
返回值: 解析是否成功
 */
int EDP_GetSlaverCcPubInfo(PRIVATE_BOARD *pBoard,  SV_TRANS_INFO *pSvCcTransInfo)
{
    int ret = 0;
    CC_SV_SUB_INFO *pCcSvSubInfo = NULL;
    CC_SV_PUB_INFO *pCcSvPubInfo = NULL;
    BOARD_PORT_CONNECT *pPortConnect = NULL;

    int iPubSmvConnectId[EDP_CC_PORT_NUM] = {0};
    int iCcPubPortNum = 0;
    int iCcPubSmvNum = 0;
    uint32_t select1 = 0;
    uint32_t select2 = 0;
    int iLen = 0;
    const char strTemp[5] = {'I','I','I','I','I'};
    char str[7];

    int i = 0, j = 0, k = 0;

    for(i = 0; i < pBoard->tPort.iConnectCnt; i++)
    {
        if((pBoard->tPort.pPortConnect[i].iDataType == PORT_TRANSFOR_DATA_TYPE_SV)
                || (pBoard->tPort.pPortConnect[i].iDataType == PORT_TRANSFOR_DATA_TYPE_SVGS))
        {
            iPubSmvConnectId[iCcPubPortNum] = i;
            iCcPubSmvNum++;
        }
        iCcPubPortNum++;
    }

    if(pSvCcTransInfo->iSubAsduNum > 6)
    {
        iCcPubSmvNum = 2;
    }
    else
    {
        iCcPubSmvNum = 1;
    }
    if(iCcPubSmvNum > iCcPubPortNum)
    {
        ret = -1;
        CFG_LOG("私有配置中子机CC采样发送端口少于实际需求量\n",1,2,3,4,5,6);
        goto exit;
    }

    pSvCcTransInfo->pCcSvPubInfo = calloc(iCcPubSmvNum, sizeof(CC_SV_PUB_INFO));

    pCcSvPubInfo = pSvCcTransInfo->pCcSvPubInfo;
    pCcSvSubInfo = pSvCcTransInfo->pCcSvSubInfo;
    for(i = 0; i < iCcPubSmvNum; i++)
    {
        pPortConnect = &pBoard->tPort.pPortConnect[iPubSmvConnectId[i]];
        pCcSvPubInfo->iOutBoardId = pPortConnect->iOutBoardId;
        pCcSvPubInfo->iOutPortId = pPortConnect->iOutPortId;
        pCcSvPubInfo->iPort = pPortConnect->iPortId;
        if((iCcPubSmvNum == 2)&&(i == 0))
        {
            pCcSvPubInfo->iAsduNum = 6;
        }
        else
        {
            pCcSvPubInfo->iAsduNum = pSvCcTransInfo->iSubAsduNum%6;
        }
        pCcSvPubInfo->pCcSvPubAsduInfo = calloc(pCcSvPubInfo->iAsduNum, sizeof(CC_SV_PUB_ASDU_INFO));

        pCcSvPubInfo->ucMacAddrSrc[0] = 0x00;
        pCcSvPubInfo->ucMacAddrSrc[1] = 0xA0;
        pCcSvPubInfo->ucMacAddrSrc[2] = 0x1E;
        pCcSvPubInfo->ucMacAddrSrc[3] = 0x04;
        pCcSvPubInfo->ucMacAddrSrc[4] = 0x00;
        pCcSvPubInfo->ucMacAddrSrc[5] = 0x50|i;

        pCcSvPubInfo->ucMacAddrDes[0] = pPortConnect->aSvMacAddr[0];
        pCcSvPubInfo->ucMacAddrDes[1] = pPortConnect->aSvMacAddr[1];
        pCcSvPubInfo->ucMacAddrDes[2] = pPortConnect->aSvMacAddr[2];
        pCcSvPubInfo->ucMacAddrDes[3] = pPortConnect->aSvMacAddr[3];
        pCcSvPubInfo->ucMacAddrDes[4] = pPortConnect->aSvMacAddr[4];
        pCcSvPubInfo->ucMacAddrDes[5] = pPortConnect->aSvMacAddr[5];

        pCcSvPubInfo->usAppID = pPortConnect->usAppID;

        pCcSvPubInfo->ulConfRev = EDP_CC_CONFREV_DEFAULT;
        pCcSvPubInfo->usTCI = EDP_CC_TCI_DEFAULT;
        pCcSvPubInfo->usPubRate = pPortConnect->iSvPubRate;

        for(j = 0; j < pCcSvPubInfo->iAsduNum; j++)
        {
            pCcSvPubInfo->pCcSvPubAsduInfo[j].iID = pCcSvSubInfo->pCcSvSubAsduInfo->iID;

            /*主CC与子CC之间的转发必须满足SVID固定为6个字符*/
            iLen = strlen(pCcSvSubInfo->pCcSvSubAsduInfo->pSvID);
            if(iLen < 6)
            {
                memcpy(str, pCcSvSubInfo->pCcSvSubAsduInfo->pSvID, iLen);
                memcpy(str+iLen, strTemp, 6-iLen);
                str[6] = 0;
            }
            else
            {
                memcpy(str, pCcSvSubInfo->pCcSvSubAsduInfo->pSvID, 6);
                str[6] = 0;
            }
            pCcSvPubInfo->pCcSvPubAsduInfo[j].pSvID = strdup(str);
            pCcSvPubInfo->pCcSvPubAsduInfo[j].pSelect1 = pCcSvSubInfo->pCcSvSubAsduInfo->pSelect1;
            pCcSvPubInfo->pCcSvPubAsduInfo[j].pSelect2 = pCcSvSubInfo->pCcSvSubAsduInfo->pSelect2;
            for(k = 0; k < MAX_NET_NUM; k++)
            {
                select1 |= pCcSvPubInfo->pCcSvPubAsduInfo[j].pSelect1[k];
                select2 |= pCcSvPubInfo->pCcSvPubAsduInfo[j].pSelect2[k];
            }
            k = 0;
            if(select2 != 0)
            {
                pCcSvPubInfo->pCcSvPubAsduInfo[j].ulSelect1 = 0xffffffff;
                while((select2 & (0x80000000>>k)) == 0)
                {
                    k++;
                }
                pCcSvPubInfo->pCcSvPubAsduInfo[j].ulSelect2 = (0xfffffffff>>k);
            }
            else
            {
                while((select1 & (0x80000000>>k)) == 0)
                {
                    k++;
                }
                pCcSvPubInfo->pCcSvPubAsduInfo[j].ulSelect1 = (0xfffffffff>>k);
            }
            pCcSvPubInfo->pCcSvPubAsduInfo[j].iChnNum = pCcSvSubInfo->pCcSvSubAsduInfo->iChnNum;
            pCcSvPubInfo->pCcSvPubAsduInfo[j].ulDelayChan = pCcSvSubInfo->pCcSvSubAsduInfo->ulDelayChan;
            pCcSvPubInfo->pCcSvPubAsduInfo[j].ulConfRev = pCcSvSubInfo->pCcSvSubAsduInfo->ulConfRev;
            pCcSvPubInfo->pCcSvPubAsduInfo[j].iSubCcBoardId= pCcSvSubInfo->pCcSvSubAsduInfo->iSubCcBoardId;
            pCcSvPubInfo->pCcSvPubAsduInfo[j].iSubCcPortId= pCcSvSubInfo->pCcSvSubAsduInfo->iSubCcPortId;
            pCcSvSubInfo++;
        }
        pCcSvPubInfo++;
    }

    pSvCcTransInfo->iPubNum= iCcPubSmvNum;

exit:
    return ret;

}

/*
描述: 获取主CC板接收信息
参数: pBoard, 主CC板板件信息结构
      pCcSvPool, CC转发结构
返回值: 解析是否成功
 */
int EDP_GetMasterCcSubInfo(PRIVATE_BOARD *pBoard,  CC_SV_POOL *pCcSvPool)
{
    int res = 0;
    int iSubSmvRefId[12] = {0};
    int iCCSubSmvNum = 0;
    int iCcSubRefNum = 0;
    int iCcSubSlaverNum = 0;
    int iCcSubAsduNum = 0;
    PROCESS_SUB_SMV *pSubSmvCfg = NULL;
    SV_TRANS_INFO *pSlaverSvCcInfo = NULL;
    SV_TRANS_INFO *pMasterSvCcTransInfo = NULL;
    SUB_SMV_INFO *pSubSmvInfo = NULL;
    CC_SV_SUB_INFO *pMasterCcSvSubInfo = NULL;
    CC_SV_PUB_INFO *pSlaverCcSvPubInfo = NULL;

    int i, j, k;

    pSlaverSvCcInfo = pCcSvPool->pSvSlaverCcTransInfo;
    pMasterSvCcTransInfo = &pCcSvPool->tSvMasterCcTransInfo;
    for(i = 0; i < pCcSvPool->iSvBoardCnt-1; i++)
    {
        pSlaverCcSvPubInfo = pSlaverSvCcInfo->pCcSvPubInfo;
        for(j = 0; j < pSlaverSvCcInfo->iPubNum; j++)
        {
            if((pSlaverCcSvPubInfo->iOutBoardId == pBoard->iBoardId)
                    &&(pSlaverCcSvPubInfo->iAsduNum != 0))
            {
                iCcSubSlaverNum++;
            }
            pSlaverCcSvPubInfo++;
        }
        pSlaverSvCcInfo++;
    }

    pSubSmvCfg = (PROCESS_SUB_SMV *)EDP_GetProcessSubSmv();
    for(i = 0; i < pSubSmvCfg->iSubSmvCnt; i++)
    {
        if(pBoard->iBoardId == pSubSmvCfg->pSubSmvInfo[i].iSubCcBoardId)
        {
            iSubSmvRefId[iCcSubRefNum] = i;
            iCcSubRefNum++;
        }
    }

    pMasterSvCcTransInfo->bMasterCc = TRUE;
    iCCSubSmvNum = iCcSubSlaverNum+iCcSubRefNum;
    pMasterSvCcTransInfo->iSubNum = iCCSubSmvNum;
    pMasterSvCcTransInfo->iMaxDelay = pBoard->tSmv.tSmvCommon.iMaxDelay;
    pMasterSvCcTransInfo->pCcSvSubInfo = calloc(iCCSubSmvNum, sizeof(CC_SV_SUB_INFO));

    pMasterCcSvSubInfo = pMasterSvCcTransInfo->pCcSvSubInfo;
    pSlaverSvCcInfo = pCcSvPool->pSvSlaverCcTransInfo;
    for(i = 0; i < pCcSvPool->iSvBoardCnt-1; i++)
    {
        pSlaverCcSvPubInfo = pSlaverSvCcInfo->pCcSvPubInfo;
        for(j = 0; j < pSlaverSvCcInfo->iPubNum; j++)
        {
            if(pSlaverCcSvPubInfo->iAsduNum == 0)
            {
                continue;
            }

            if(pSlaverCcSvPubInfo->iOutBoardId == pBoard->iBoardId)
            {
                pMasterCcSvSubInfo->iPort = pSlaverCcSvPubInfo->iOutPortId;
                memcpy(pMasterCcSvSubInfo->ucMacAddr, pSlaverCcSvPubInfo->ucMacAddrDes, MAC_BYTES);
                pMasterCcSvSubInfo->iSrc = 1;
                pMasterCcSvSubInfo->iSubBoard = 1;
                pMasterCcSvSubInfo->iAsduNum = pSlaverCcSvPubInfo->iAsduNum;
                pMasterCcSvSubInfo->usTCI = EDP_CC_TCI_DEFAULT;
                pMasterCcSvSubInfo->usAppID = pSlaverCcSvPubInfo->usAppID;
                pMasterCcSvSubInfo->iGmrp = pBoard->tSmv.tSmvCommon.iGmrp;
                pMasterCcSvSubInfo->pCcSvSubAsduInfo = calloc(pMasterCcSvSubInfo->iAsduNum, sizeof(CC_SV_SUB_ASDU_INFO));

                for(k = 0; k < pSlaverCcSvPubInfo->iAsduNum; k++)
                {
                    pMasterCcSvSubInfo->pCcSvSubAsduInfo[k].iChnNum = pSlaverCcSvPubInfo->pCcSvPubAsduInfo[k].iChnNum;
                    pMasterCcSvSubInfo->pCcSvSubAsduInfo[k].iID = iCcSubAsduNum+1;    /*ID从1开始排序*/
                    pMasterCcSvSubInfo->pCcSvSubAsduInfo[k].iShift = 0;
                    pMasterCcSvSubInfo->pCcSvSubAsduInfo[k].pSvID = strdup(pSlaverCcSvPubInfo->pCcSvPubAsduInfo[k].pSvID);
                    pMasterCcSvSubInfo->pCcSvSubAsduInfo[k].ulConfRev = EDP_CC_CONFREV_DEFAULT;
                    pMasterCcSvSubInfo->pCcSvSubAsduInfo[k].ulDelayChan = pSlaverCcSvPubInfo->pCcSvPubAsduInfo[k].ulDelayChan;
                    pMasterCcSvSubInfo->pCcSvSubAsduInfo[k].pSelect1 = pSlaverCcSvPubInfo->pCcSvPubAsduInfo[k].pSelect1;
                    pMasterCcSvSubInfo->pCcSvSubAsduInfo[k].pSelect2 = pSlaverCcSvPubInfo->pCcSvPubAsduInfo[k].pSelect2;
                    pMasterCcSvSubInfo->pCcSvSubAsduInfo[k].iSubCcBoardId= pSlaverCcSvPubInfo->pCcSvPubAsduInfo[k].iSubCcBoardId;
                    pMasterCcSvSubInfo->pCcSvSubAsduInfo[k].iSubCcPortId= pSlaverCcSvPubInfo->pCcSvPubAsduInfo[k].iSubCcPortId;
                    iCcSubAsduNum++;
                }
            }
            pSlaverCcSvPubInfo++;
            pMasterCcSvSubInfo++;
        }
        pSlaverSvCcInfo++;
    }

    for(i = 0; i < iCcSubRefNum; i++)
    {
        pSubSmvInfo = &pSubSmvCfg->pSubSmvInfo[iSubSmvRefId[i]];
        pMasterCcSvSubInfo->iPort = pSubSmvInfo->iSubCcPortId;
        memcpy(pMasterCcSvSubInfo->ucMacAddr, pSubSmvInfo->tConnectAp.tConnectApSmv.pSmvAddr[0].aDstMac, MAC_BYTES);
        pMasterCcSvSubInfo->iSrc = 1;
        pMasterCcSvSubInfo->iSubBoard = 0;
        pMasterCcSvSubInfo->iAsduNum = 1;
        pMasterCcSvSubInfo->usTCI = EDP_CC_TCI_DEFAULT;
        pMasterCcSvSubInfo->usAppID = pSubSmvInfo->tConnectAp.tConnectApSmv.pSmvAddr[0].usAppID;
        pMasterCcSvSubInfo->iGmrp = pBoard->tSmv.tSmvCommon.iGmrp;
        pMasterCcSvSubInfo->pCcSvSubAsduInfo = calloc(1, sizeof(CC_SV_SUB_ASDU_INFO));

        pMasterCcSvSubInfo->pCcSvSubAsduInfo[0].iChnNum = pSubSmvInfo->tDataSet.iFcdaCnt;
        pMasterCcSvSubInfo->pCcSvSubAsduInfo[0].iID = iCcSubAsduNum+1;
        pMasterCcSvSubInfo->pCcSvSubAsduInfo[0].iShift = 0;
        pMasterCcSvSubInfo->pCcSvSubAsduInfo[0].pSvID = strdup(pSubSmvInfo->tSvCtrl.pSvID);
        pMasterCcSvSubInfo->pCcSvSubAsduInfo[0].ulConfRev = pSubSmvInfo->tSvCtrl.iConfRev;
        pMasterCcSvSubInfo->pCcSvSubAsduInfo[0].ulDelayChan = 0x01<<pSubSmvInfo->iDelayChnNo;
        pMasterCcSvSubInfo->pCcSvSubAsduInfo[0].pSelect1 = pSubSmvInfo->ulSelect1;
        pMasterCcSvSubInfo->pCcSvSubAsduInfo[0].pSelect2 = pSubSmvInfo->ulSelect2;
        pMasterCcSvSubInfo->pCcSvSubAsduInfo[0].iSubCcBoardId= pSubSmvInfo->iSubCcBoardId;
        pMasterCcSvSubInfo->pCcSvSubAsduInfo[0].iSubCcPortId= pSubSmvInfo->iSubCcPortId;

        pMasterCcSvSubInfo++;
        iCcSubAsduNum++;
    }

    pMasterSvCcTransInfo->iSubAsduNum = iCcSubAsduNum;

    return res;
}

/*
描述: 获取子CC板发送信息
参数: pBoard, 子CC板板件信息结构
      pSvCcTransInfo, CC转发信息
返回值: 解析是否成功
 */
int EDP_GetMasterCcPubInfo(PRIVATE_BOARD *pBoard,  SV_TRANS_INFO *pSvCcTransInfo)
{
    int ret = 0;
    CC_SV_SUB_INFO *pCcSvSubInfo = NULL;
    CC_SV_PUB_INFO *pCcSvPubInfo = NULL;
    BOARD_PORT_CONNECT *pPortConnect = NULL;
    CC_SV_SUB_ASDU_INFO *pCcSvSubAsduInfo = NULL;

    int iPubSmvConnectId[EDP_CC_PORT_NUM] = {0};
    int iCcPubPortNum = 0;
    int iCcPubSmvNum = 0;
    int iAsduCntPerSub = 0;
    int iSubCnt = 0;
    int iPubNo = 0;
    char str[7];

    int i = 0, j = 0;

    for(i = 0; i < pBoard->tPort.iConnectCnt; i++)
    {
        if((pBoard->tPort.pPortConnect[i].iDataType == PORT_TRANSFOR_DATA_TYPE_SV)
                || (pBoard->tPort.pPortConnect[i].iDataType == PORT_TRANSFOR_DATA_TYPE_SVGS))
        {
            iPubSmvConnectId[iCcPubPortNum] = i;
            iCcPubSmvNum++;
        }
        iCcPubPortNum++;
    }

    pSvCcTransInfo->pCcSvPubInfo = calloc(iCcPubSmvNum, sizeof(CC_SV_PUB_INFO));

    pCcSvPubInfo = pSvCcTransInfo->pCcSvPubInfo;
    for(i = 0; i < iCcPubSmvNum; i++)
    {
        pPortConnect = &pBoard->tPort.pPortConnect[iPubSmvConnectId[i]];
        pCcSvPubInfo->iNodeAddr = EDP_GetNodeAddrByBoardId(pPortConnect->iOutBoardId);
        pCcSvPubInfo->iOutBoardId = pPortConnect->iOutBoardId;
        pCcSvPubInfo->iOutPortId = pPortConnect->iOutPortId;
        pCcSvPubInfo->iPort = pPortConnect->iPortId;
        pCcSvPubInfo->iAsduNum = pSvCcTransInfo->iSubAsduNum;
        pCcSvPubInfo->pCcSvPubAsduInfo = calloc(pCcSvPubInfo->iAsduNum, sizeof(CC_SV_PUB_ASDU_INFO));

        pCcSvPubInfo->ucMacAddrSrc[0] = 0x00;
        pCcSvPubInfo->ucMacAddrSrc[1] = 0xA0;
        pCcSvPubInfo->ucMacAddrSrc[2] = 0x1E;
        pCcSvPubInfo->ucMacAddrSrc[3] = 0x04;
        pCcSvPubInfo->ucMacAddrSrc[4] = 0x00;
        pCcSvPubInfo->ucMacAddrSrc[5] = 0x50|i;

        memcpy(pCcSvPubInfo->ucMacAddrDes, pPortConnect->aSvMacAddr, MAC_BYTES);
        pCcSvPubInfo->usAppID = pPortConnect->usAppID;

        pCcSvPubInfo->ulConfRev = EDP_CC_CONFREV_DEFAULT;
        pCcSvPubInfo->usTCI = EDP_CC_TCI_DEFAULT;
        pCcSvPubInfo->usPubRate = pPortConnect->iSvPubRate;

        pCcSvSubInfo = pSvCcTransInfo->pCcSvSubInfo;
        iSubCnt = 0;
        iAsduCntPerSub = 0;
        for(j = 0; j < pCcSvPubInfo->iAsduNum; j++)
        {
            if(iAsduCntPerSub >= pCcSvSubInfo->iAsduNum)
            {
                iAsduCntPerSub = 0;
                pCcSvSubInfo++;
                iSubCnt++;
                if(iSubCnt >= pSvCcTransInfo->iSubNum)
                {
                    break;
                }
            }

            pCcSvSubAsduInfo = pCcSvSubInfo->pCcSvSubAsduInfo+iAsduCntPerSub;
            if(EDP_GetSvAsduPubNo(pCcSvSubAsduInfo, &iPubNo) != 0)
            {
                continue;
            }
            pCcSvPubInfo->pCcSvPubAsduInfo[iPubNo].iID = pCcSvSubAsduInfo->iID;
            memcpy(str, pCcSvSubAsduInfo->pSvID, 6);
            str[6] = 0;
            pCcSvPubInfo->pCcSvPubAsduInfo[iPubNo].pSvID = strdup(str);
            pCcSvPubInfo->pCcSvPubAsduInfo[iPubNo].ulSelect1 = pCcSvSubAsduInfo->pSelect1[pCcSvPubInfo->iNodeAddr];
            pCcSvPubInfo->pCcSvPubAsduInfo[iPubNo].ulSelect2 = pCcSvSubAsduInfo->pSelect2[pCcSvPubInfo->iNodeAddr];
            pCcSvPubInfo->pCcSvPubAsduInfo[iPubNo].iChnNum = pCcSvSubAsduInfo->iChnNum;
            pCcSvPubInfo->pCcSvPubAsduInfo[iPubNo].ulDelayChan = pCcSvSubAsduInfo->ulDelayChan;
            pCcSvPubInfo->pCcSvPubAsduInfo[iPubNo].ulConfRev = pCcSvSubAsduInfo->ulConfRev;
            iAsduCntPerSub++;
        }
        pCcSvPubInfo++;
    }

    pSvCcTransInfo->iPubNum= iCcPubSmvNum;

    return ret;

}

/*
描述: 通过板件ID获取SV CC转发信息
参数:     BoardId 板件ID
返回值: SV转发信息指针 出错返回NULL
 */
SV_TRANS_INFO* GetSvTransInfoByBoardId(int BoardId)
{
    int i;

    if(g_tCCSvPool.tSvMasterCcTransInfo.iBoardId == BoardId)
    {
        return &g_tCCSvPool.tSvMasterCcTransInfo;
    }

    for(i = 0; i < g_tCCSvPool.iSvBoardCnt-1; i++)
    {
        if(g_tCCSvPool.pSvSlaverCcTransInfo[i].iBoardId == BoardId)
        {
            return &g_tCCSvPool.pSvSlaverCcTransInfo[i];
        }
    }

    return NULL;
}

/*
描述: 解析过程层SV配置信息，内存数据结构转换
参数:     NONE
返回值: 解析是否成功
 */
int EDP_ConfigParseCCSv()
{
    int res = 0;

    /*配置结构*/
    PRIVATE_BOARD *pBoard = NULL;

    int iSvCcBoardCnt = 0;
    int iSlaverSvCcBoardId[EDP_CC_PORT_NUM] = {0};   /* 子机CC板件ID号数组 */
    int iMasterSvCcId = 0;

    int i;

    iSvCcBoardCnt = EDP_GetSvCcBoardId(&iMasterSvCcId, iSlaverSvCcBoardId);
    if(iSvCcBoardCnt == 0)
    {
        CFG_LOG("未配置接收SV报文的CC板!\n",1,2,3,4,5,6);
        goto exit;
    }

    g_tCCSvPool.iSvBoardCnt= iSvCcBoardCnt;
    if(iSvCcBoardCnt > 1)
    {
        g_tCCSvPool.pSvSlaverCcTransInfo = calloc(iSvCcBoardCnt-1, sizeof(SV_TRANS_INFO));
        for(i = 0; i < (iSvCcBoardCnt-1); i++)
        {
            /*生成自己CC板的配置信息,默认一个主机CC,主机CC排在数组的*/
            pBoard = EDP_GetPrivateBoardPtrFromId(iSlaverSvCcBoardId[i]);
            g_tCCSvPool.pSvSlaverCcTransInfo[i].iBoardId = iSlaverSvCcBoardId[i];
            g_tCCSvPool.pSvSlaverCcTransInfo[i].iMode = pBoard->tSmv.tSmvCommon.iMode;
            g_tCCSvPool.pSvSlaverCcTransInfo[i].iFuncType = pBoard->tSmv.tSmvCommon.iFuncType;
            g_tCCSvPool.pSvSlaverCcTransInfo[i].iSourceRate = pBoard->tSmv.tSmvCommon.iSourceRate;
            g_tCCSvPool.pSvSlaverCcTransInfo[i].iSynPulse = pBoard->tSmv.tSmvCommon.iSynPulse;
            g_tCCSvPool.pSvSlaverCcTransInfo[i].iSynPulseEnable = pBoard->tSmv.tSmvCommon.iSynPulseEnable;
            g_tCCSvPool.pSvSlaverCcTransInfo[i].iExtSynMode = pBoard->tSmv.tSmvCommon.iExtSynMode;
            g_tCCSvPool.pSvSlaverCcTransInfo[i].iExtSynReverse = pBoard->tSmv.tSmvCommon.iExtSynReverse;
            g_tCCSvPool.pSvSlaverCcTransInfo[i].iGmrpSendGap= pBoard->tSmv.tSmvCommon.iGmrpSendGap;

            /* 调用接口函数完成采样接收部分内容解析 */
            EDP_GetSlaverCcSubInfo(pBoard, &g_tCCSvPool.pSvSlaverCcTransInfo[i]);
            /* 根据解析的采样接收部分内容生成采样发送部分内容 */
            EDP_GetSlaverCcPubInfo(pBoard, &g_tCCSvPool.pSvSlaverCcTransInfo[i]);
        }
    }

    pBoard = EDP_GetPrivateBoardPtrFromId(iMasterSvCcId);
    g_tCCSvPool.tSvMasterCcTransInfo.iBoardId = iMasterSvCcId;
    g_tCCSvPool.tSvMasterCcTransInfo.iMode = pBoard->tSmv.tSmvCommon.iMode;
    g_tCCSvPool.tSvMasterCcTransInfo.iFuncType = pBoard->tSmv.tSmvCommon.iFuncType;
    g_tCCSvPool.tSvMasterCcTransInfo.iSourceRate = pBoard->tSmv.tSmvCommon.iSourceRate;
    g_tCCSvPool.tSvMasterCcTransInfo.iSynPulse = pBoard->tSmv.tSmvCommon.iSynPulse;
    g_tCCSvPool.tSvMasterCcTransInfo.iSynPulseEnable = pBoard->tSmv.tSmvCommon.iSynPulseEnable;
    g_tCCSvPool.tSvMasterCcTransInfo.iExtSynMode = pBoard->tSmv.tSmvCommon.iExtSynMode;
    g_tCCSvPool.tSvMasterCcTransInfo.iExtSynReverse = pBoard->tSmv.tSmvCommon.iExtSynReverse;
    g_tCCSvPool.tSvMasterCcTransInfo.iGmrpSendGap= pBoard->tSmv.tSmvCommon.iGmrpSendGap;

    EDP_GetMasterCcSubInfo(pBoard, &g_tCCSvPool);
    EDP_GetMasterCcPubInfo(pBoard, &g_tCCSvPool.tSvMasterCcTransInfo);


exit:
    return res;
}


/*
描述:将port加入到list中
参数:
type: 1是src，2是dst
pSrcPort: 本板的接收端口，形如7-A
pGsTransInfoNode:转发信息，用于填写
返回值:
TRUE : 代表全部搜索完毕，结束搜索
FALSE : 代表未全部搜索完毕，继续搜索
*/
BOOL EDP_AddPort(int type, char *pPort,GS_TRANS_INFO *pGsTransInfoNode)
{
    int i = 0;

    if(pPort == NULL || pGsTransInfoNode == NULL )
        return TRUE;

    if(type == 1)
    {
        for(i = 0; i < pGsTransInfoNode->iGcbSrcPortCnt; i++)
        {
            if(memcmp(pGsTransInfoNode->pSrcPortList[i], pPort, 3) == 0)
            {
                return TRUE;
            }
        }
        pGsTransInfoNode->pSrcPortList[pGsTransInfoNode->iGcbSrcPortCnt] = strdup(pPort);
        pGsTransInfoNode->iGcbSrcPortCnt++;
    }
    else if(type == 2)
    {
        for(i = 0; i < pGsTransInfoNode->iGcbDstPortCnt; i++)
        {
            if(strcmp(pGsTransInfoNode->pDstPortList[i], pPort) == 0)
            {
                return TRUE;
            }
        }
        pGsTransInfoNode->pDstPortList[pGsTransInfoNode->iGcbDstPortCnt] = strdup(pPort);
        pGsTransInfoNode->iGcbDstPortCnt++;
    }
    return TRUE;
}



/*
描述:将port从list中删除
参数:
type: 1是src，2是dst
pSrcPort: 本板的接收端口，形如7-A
pGsTransInfoNode:转发信息，用于填写
返回值:
TRUE : 代表全部搜索完毕，结束搜索
FALSE : 代表未全部搜索完毕，继续搜索
*/
BOOL EDP_DelPort(int type, char *pPort,GS_TRANS_INFO *pGsTransInfoNode)
{
    int i = 0;
    int j = 0;

    if(pPort == NULL || pGsTransInfoNode == NULL )
        return TRUE;

    if(type == 1)
    {
        for(i = 0; i < pGsTransInfoNode->iGcbSrcPortCnt; i++)
        {
            if(memcmp(pGsTransInfoNode->pSrcPortList[i], pPort, 3) == 0)
            {
                for(j = i; j < pGsTransInfoNode->iGcbSrcPortCnt; j++)
                {
                    free(pGsTransInfoNode->pSrcPortList[j]);
                    if(j < (pGsTransInfoNode->iGcbSrcPortCnt - 1))
                    {
                        pGsTransInfoNode->pSrcPortList[j] = strdup(pGsTransInfoNode->pSrcPortList[j+1]);
                    }
                }
                pGsTransInfoNode->iGcbSrcPortCnt--;
                return TRUE;
            }
        }
    }
    else if(type == 2)
    {
        for(i = 0; i < pGsTransInfoNode->iGcbDstPortCnt; i++)
        {
            if(memcmp(pGsTransInfoNode->pDstPortList[i], pPort, 3) == 0)
            {
                for(j = i; j < pGsTransInfoNode->iGcbDstPortCnt; j++)
                {
                    free(pGsTransInfoNode->pDstPortList[j]);
                    if(j < (pGsTransInfoNode->iGcbDstPortCnt - 1))
                    {
                        pGsTransInfoNode->pDstPortList[j] = strdup(pGsTransInfoNode->pDstPortList[j+1]);
                    }
                }
                pGsTransInfoNode->iGcbDstPortCnt--;
                return TRUE;
            }
        }
    }
    return TRUE;
}


/*
描述:循环遍历得到当前接收GSE控制块的所有接收源端口和发送目的端口，针对所有CC板和CPU的
参数:
index: gs的索引
pSrcPort: 本板的接收端口，形如7-A
pGsTransInfoNode:转发信息，用于填写
返回值:
TRUE : 代表全部搜索完毕，结束搜索
FALSE : 代表未全部搜索完毕，继续搜索
*/
BOOL EDP_CycleParseSubGsCCPort(int index, char *pSrcPort,GS_TRANS_INFO *pGsTransInfoNode)
{
    int iCcBeginBoard = 0;
    int iCcBeginPort = 0;
    PRIVATE_BOARD *pBoard = NULL;
    int i = 0;
    BOARD_PORT_CONNECT *pPortConnect = NULL;
    GS_TRANS_INFO *pGsTransInfo = NULL;
    char Tmp[6];

    if(pGsTransInfoNode == NULL || pSrcPort == NULL)
    {
        return TRUE;
    }

    pGsTransInfo = pGsTransInfoNode;

    iCcBeginBoard = EDP_GetBoardId(pSrcPort);
    iCcBeginPort = EDP_GetPortId(pSrcPort);

    /*将初始端口加入源端口list中去*/
    EDP_AddPort(1, pSrcPort, pGsTransInfo);

    pBoard = EDP_GetPrivateBoardPtrFromId(iCcBeginBoard);

    /*代表结束搜索，已经搜索到了CPU*/
    if(pBoard->iBoardType != BOARD_TYPE_CC
            && pBoard->iParentCCId == pBoard->iBoardId)
    {
        return TRUE;
    }
    for(i = 0; i < pBoard->tPort.iConnectCnt; i++)
    {
        /*代表搜索结束*/
        pPortConnect = &(pBoard->tPort.pPortConnect[i]);
        if(pPortConnect->iDataType == PORT_TRANSFOR_DATA_TYPE_GS
                || pPortConnect->iDataType == PORT_TRANSFOR_DATA_TYPE_SVGS )
        {
            /*需要转发*/
            if(EDP_IsCcTrans(index, pPortConnect))
            {
                sprintf(Tmp,"%d-%c", iCcBeginBoard, (int)(pPortConnect->iPortId+'A'));
                EDP_AddPort(2, Tmp, pGsTransInfo);
                EDP_CycleParseSubGsCCPort(index, pPortConnect->pOutPortId, pGsTransInfoNode);
            }
        }
    }

    return TRUE;
}

/*
描述:得到当前接收GSE控制块的所有接收源端口和发送目的端口，针对所有CC板和CPU的
参数:
pGsTransInfoNode:转发信息
pCcSubInfoNode: GCB结构，主要是index 和intaddr 有用处
返回值:是否成功
*/
BOOL EDP_ConfigParseGsSubCCPort(GS_TRANS_INFO *pGsTransInfoNode, GSE_SUB_INFO *pCcSubInfoNode)
{
    SUB_GCB_INFO *pSubGooseGcb = NULL;
    GS_TRANS_INFO *pGsTransInfo = NULL;

    if(pGsTransInfoNode == NULL && pCcSubInfoNode == NULL)
    {
        return FALSE;
    }

    pGsTransInfo = pGsTransInfoNode;
    pSubGooseGcb = (SUB_GCB_INFO *)pCcSubInfoNode->pSubGooseGcb;

    /*将INTADDR中的端口信息添加到GSE_SUB_INFO的NETINFO中*/
    pCcSubInfoNode->NetInfo.addr[0].portnum[0] = EDP_GetPortId(pSubGooseGcb->pIntAddrName);

    EDP_CycleParseSubGsCCPort(pCcSubInfoNode->GcbIndex, pSubGooseGcb->pIntAddrName, pGsTransInfo);

    return TRUE;
}


#if 0
/*
描述:循环遍历得到当前发送GSE控制块的所有接收源端口和发送目的端口，针对所有CC板和CPU的
参数:
pSrcPort: 代表GOOSE
pGsTransInfoNode:转发信息，用于填写
返回值:
TRUE : 代表全部搜索完毕，结束搜索
FALSE : 代表未全部搜索完毕，继续搜索
*/
BOOL EDP_CycleParsePubGsCCPort(char *pSrcPort,GS_TRANS_INFO *pGsTransInfoNode, PUB_GCB_INFO *pPubGooseGcb)
{
    int iCcBeginBoard = 0;
    int iCcBeginPort = 0;
    PRIVATE_BOARD *pBoard = NULL;
    PRIVATE_BOARD *pBoardTmp = NULL;
    int i = 0;
    int j = 0;
    BOARD_PORT_CONNECT *pPortConnect = NULL;
    GS_TRANS_INFO *pGsTransInfo = NULL;
    GSE_CONNECTAP_PHYSCONN *pConnectApPhysConn = NULL;
    int iBoardId = 0;
    BOOL bContinue = FALSE;

    if(pGsTransInfoNode == NULL || pSrcPort == NULL || pPubGooseGcb == NULL)
    {
        return TRUE;
    }

    pGsTransInfo = pGsTransInfoNode;

    iCcBeginBoard = EDP_GetBoardId(pSrcPort);
    iCcBeginPort = EDP_GetPortId(pSrcPort);

    /*将初始端口加入源端口list中去*/
    pGsTransInfo->pSrcPortList[pGsTransInfo->iGcbSrcPortCnt] = strdup(pSrcPort);
    pGsTransInfo->iGcbSrcPortCnt++;

    pBoard = EDP_GetPrivateBoardPtrFromId(iCcBeginBoard);

    /*在此要设置结束条件，发送的PHYSCOON是否与该板号的ID一致，如果一致(要判断datatyp)
    则将PHYSCOON中的所有端口加入到目标端口中，如不一致，
    则进一步搜索其他子CC板的OUTPUTID中的板号是该板子的端口，
    将OUTPUTID中的作为目的端口，子CC的PORIID作为源端口*/
    for(i = 0; i < pPubGooseGcb->tConnectAp.iPhysConnCnt; i++)
    {
        pConnectApPhysConn = &(pPubGooseGcb->tConnectAp.tConnectApPhysConn[i]);
        iBoardId = EDP_GetBoardId(pConnectApPhysConn->pPort);
        /*logMsg("###### %d  physconn cnt: %d . %s,   %d , %d \n", i,
            pPubGooseGcb->tConnectAp.iPhysConnCnt,pConnectApPhysConn->pPort,iBoardId,pBoard->iBoardId,0);
        */
        if(iBoardId == pBoard->iBoardId)
        {
            pGsTransInfo->pDstPortList[pGsTransInfo->iGcbDstPortCnt] = strdup(pConnectApPhysConn->pPort);
            pGsTransInfo->iGcbDstPortCnt++;
        }
    }

    /*如果搜索到了boradid为x的板子，但是找不到x为outportid时，
    此时搜索结束，代表这是最终发送的CC板*/
    iBoardId = pBoard->iBoardId;
    for(i = 0; i < EDP_GetPrivateBoardCnt(); i++)
    {
        pBoardTmp = EDP_GetPrivateBoardPtrFromIndex(i);
        for(j = 0; j < pBoardTmp->tPort.iConnectCnt; j++)
        {
            pPortConnect = &(pBoardTmp->tPort.pPortConnect[j]);
            if(pPortConnect->iOutBoardId == iBoardId)
            {
                EDP_CycleParsePubGsCCPort(pPortConnect->pOutPortId, pGsTransInfoNode, pPubGooseGcb);
                bContinue = TRUE;
            }
        }
    }

    return TRUE;
}


/*
描述:得到当前发送GSE控制块的所有接收源端口和发送目的端口，针对所有CC板和CPU的
参数:
ucType: 采样还是GOOSE类型 PORT_TRANSFOR_DATA_TYPE_GS or PORT_TRANSFOR_DATA_TYPE_SV
pGsTransInfoNode:转发信息
pCcSubInfoNode: GCB结构，主要是index 和intaddr 有用处
返回值:是否成功
*/
BOOL EDP_ConfigParseGsPubCCPort(int iIndex, GS_TRANS_INFO *pGsTransInfoNode, GSE_SUB_INFO *pCcSubInfoNode)
{
    int iCcBeginBoard = 0;
    int iCcBeginPort = 0;
    GS_TRANS_INFO *pGsTransInfo = NULL;
    PUB_GCB_INFO *pPubGooseGcb = NULL;
    char Tmp[8];
    char Tmp2[8];
    int i = 0;
    int j = 0;

    if(pGsTransInfoNode == NULL && pCcSubInfoNode == NULL)
    {
        return FALSE;
    }

    pGsTransInfo = pGsTransInfoNode;
    pPubGooseGcb = (PUB_GCB_INFO *)pCcSubInfoNode->pSubGooseGcb;

    iCcBeginBoard = g_ptPubSAddr[iIndex].ucCpuId;
#if 1
    for(i = 0; i < pCcSubInfoNode->NetInfo.num; i++)
    {
        for(j = 0; j < pCcSubInfoNode->NetInfo.addr[i].portcount; j++)
        {
            iCcBeginPort = pCcSubInfoNode->NetInfo.addr[i].portnum[j];
            sprintf(Tmp,"%d-%c", iCcBeginBoard, (int)(iCcBeginPort+'A'));
            /*找到对应Tmp的CC板的发送端口是哪一个，对于GOOSE发送来讲他是源端口*/
            /*此处要加判断，对返回值的TMP2进行判断，否则会溢出*/
            if(EDP_GetCCPortInfo(Tmp,Tmp2))
            {
                EDP_CycleParsePubGsCCPort(Tmp2, pGsTransInfoNode, pPubGooseGcb);
                logMsg("#######  %s  %s \n",Tmp,Tmp2,0,0,0,0);
            }
            else
            {
                logMsg("未找到 %s  PUB 对应端口\n",(int)Tmp,0,0,0,0,0);
            }
        }
    }
#endif
    return TRUE;
}
#endif


/*
描述:循环遍历得到当前接收GSE控制块的所有接收源端口和发送目的端口，针对所有CC板和CPU的
参数:
index: gs的索引
pSrcPort: 本板的接收端口，形如7-A
pGsTransInfoNode:转发信息，用于填写
返回值:
TRUE : 代表全部搜索完毕，结束搜索
FALSE : 代表未全部搜索完毕，继续搜索
*/
BOOL EDP_CycleParsePubGsCCPort(int index, char *pSrcPort,GS_TRANS_INFO *pGsTransInfoNode,PUB_GCB_INFO *pPubGooseGcb)
{
    int iCcBeginBoard = 0;
    int iCcBeginPort = 0;
    PRIVATE_BOARD *pBoard = NULL;
    PRIVATE_BOARD *pBoard2 = NULL;
    int i = 0;
    BOARD_PORT_CONNECT *pPortConnect = NULL;
    GS_TRANS_INFO *pGsTransInfo = NULL;
    char Tmp[6];

    if(pGsTransInfoNode == NULL || pSrcPort == NULL)
    {
        return TRUE;
    }

    pGsTransInfo = pGsTransInfoNode;

    iCcBeginBoard = EDP_GetBoardId(pSrcPort);
    iCcBeginPort = EDP_GetPortId(pSrcPort);

    /*将初始端口加入源端口list中去,发送端口反过来加到目的端口去*/
    EDP_AddPort(2, pSrcPort, pGsTransInfo);

    pBoard = EDP_GetPrivateBoardPtrFromId(iCcBeginBoard);

    /*代表结束搜索，已经搜索到了CPU*/
    if(pBoard->iBoardType != BOARD_TYPE_CC
            && pBoard->iParentCCId == pBoard->iBoardId)
    {
        return TRUE;
    }
    for(i = 0; i < pBoard->tPort.iConnectCnt; i++)
    {
        /*代表搜索结束*/
        pPortConnect = &(pBoard->tPort.pPortConnect[i]);
        /*pPortConnect 的OUTPORT 的ID 如果是CPU，
        且CPU的ID与PUB SADDR中的CPU ID 不一致,则该端口是另外一个CPU发送的
        不将其端口加入到源端口中*/
        pBoard2 = EDP_GetPrivateBoardPtrFromId(pPortConnect->iOutBoardId);
        if((pBoard2->iBoardType != BOARD_TYPE_CC
                && pBoard2->iNodeAddr== pPubGooseGcb->iPubGcbCpuId)
                || pBoard2->iBoardType == BOARD_TYPE_CC)
        {
            if(pPortConnect->iDataType == PORT_TRANSFOR_DATA_TYPE_GS
                    || pPortConnect->iDataType == PORT_TRANSFOR_DATA_TYPE_SVGS )
            {
                /*需要转发*/
                if(EDP_IsCcTrans(index, pPortConnect))
                {
                    sprintf(Tmp,"%d-%c", iCcBeginBoard, (int)(pPortConnect->iPortId+'A'));
                    EDP_AddPort(1, Tmp, pGsTransInfo);
                    EDP_CycleParsePubGsCCPort(index, pPortConnect->pOutPortId, pGsTransInfoNode, pPubGooseGcb);
                }
            }
        }
    }

    return TRUE;
}


/*
描述:得到当前发送GSE控制块的所有接收源端口和发送目的端口，针对所有CC板和CPU的
参数:
ucType: 采样还是GOOSE类型 PORT_TRANSFOR_DATA_TYPE_GS or PORT_TRANSFOR_DATA_TYPE_SV
pGsTransInfoNode:转发信息
pCcSubInfoNode: GCB结构，主要是index 和intaddr 有用处
返回值:是否成功
*/
BOOL EDP_ConfigParseGsPubCCPort(int iIndex, GS_TRANS_INFO *pGsTransInfoNode, GSE_SUB_INFO *pCcSubInfoNode)
{
    int iCcBeginBoard = 0;
    GS_TRANS_INFO *pGsTransInfo = NULL;
    PUB_GCB_INFO *pPubGooseGcb = NULL;
    int i = 0;
    GSE_CONNECTAP_PHYSCONN *pConnectApPhysConn = NULL;

    if(pGsTransInfoNode == NULL && pCcSubInfoNode == NULL)
    {
        return FALSE;
    }

    pGsTransInfo = pGsTransInfoNode;
    pPubGooseGcb = (PUB_GCB_INFO *)pCcSubInfoNode->pSubGooseGcb;

    iCcBeginBoard = g_ptPubSAddr[iIndex].ucCpuId;
#if 0
    for(i = 0; i < pCcSubInfoNode->NetInfo.num; i++)
    {
        for(j = 0; j < pCcSubInfoNode->NetInfo.addr[i].portcount; j++)
        {
            iCcBeginPort = pCcSubInfoNode->NetInfo.addr[i].portnum[j];
            sprintf(Tmp,"%d-%c", iCcBeginBoard, (int)(iCcBeginPort+'A'));
            /*找到对应Tmp的CC板的发送端口是哪一个，对于GOOSE发送来讲他是源端口*/
            /*此处要加判断，对返回值的TMP2进行判断，否则会溢出*/
            if(EDP_GetCCPortInfo(Tmp,Tmp2))
            {
                EDP_CycleParsePubGsCCPort(Tmp2, pGsTransInfoNode, pPubGooseGcb);
                logMsg("#######  %s  %s \n",Tmp,Tmp2,0,0,0,0);
            }
            else
            {
                logMsg("未找到 %s  PUB 对应端口\n",(int)Tmp,0,0,0,0,0);
            }
        }
    }
#endif

    for(i = 0; i < pPubGooseGcb->tConnectAp.iPhysConnCnt; i++)
    {
        pConnectApPhysConn = &(pPubGooseGcb->tConnectAp.tConnectApPhysConn[i]);
        EDP_CycleParsePubGsCCPort(iIndex, pConnectApPhysConn->pPort, pGsTransInfo, pPubGooseGcb);
    }
    return TRUE;
}

/*
描述: 解析过程层GOOSE配置信息，内存数据结构转换
参数:     NONE
返回值: 解析是否成功
 */
int EDP_ConfigParseCCGs()
{
    int res = 0;
    /*实时结构*/
    GSE_SUB_INFO *pSubInfoNode = NULL;
    GSE_PUB_INFO *pPubInfoNode = NULL;

    /*CC结构*/
    GSE_SUB_INFO *pCcSubInfoNode = NULL;
    GS_TRANS_INFO *pGsTransInfoNode = NULL;

    int i = 0;

    /*获取CC板总数*/
    g_iCcGcbCnt = g_usProcessSubGooseNumAll + g_usProcessPubGooseNumAll;
    g_tCCGsPool.iGcbCnt = g_iCcGcbCnt;

    if(g_iCcGcbCnt == 0)
    {
        CFG_LOG("------>过程层无GOOSE信息数据\n",1,2,3,4,5,6);
        res = 0;
        goto exit;
    }

    g_tCCGsPool.pGsSubInfo = (GSE_SUB_INFO *)calloc(g_iCcGcbCnt, sizeof(GSE_SUB_INFO));
    if(!g_tCCGsPool.pGsSubInfo)
    {
        CFG_LOG("---->CC板GOOSE结构申请内存失败, 个数:%d \n",g_iCcGcbCnt,0,0,0,0,0);
        res = EDP_CC_GS_ERR_CALLOC_FAIL;
        goto exit;
    }
    g_tCCGsPool.pGsTransInfo = (GS_TRANS_INFO *)calloc(g_iCcGcbCnt, sizeof(GS_TRANS_INFO));
    if(!g_tCCGsPool.pGsTransInfo)
    {
        CFG_LOG("---->CC板GOOSE结构转发关系结构申请内存失败, 个数:%d \n",g_iCcGcbCnt,0,0,0,0,0);
        res = EDP_CC_GS_ERR_CALLOC_FAIL;
        goto exit;
    }

    for(i = 0; i < g_usProcessSubGooseNumAll; i++)
    {
        /*实时结构*/
        pSubInfoNode = &(g_tSubGooseAll.pSubInfoRootNode[i]);

        /*CC结构*/
        pCcSubInfoNode = &(g_tCCGsPool.pGsSubInfo[i]);
        pGsTransInfoNode = &(g_tCCGsPool.pGsTransInfo[i]);

        pCcSubInfoNode->next = (&(g_tCCGsPool.pGsSubInfo[i + 1]));
        pGsTransInfoNode->next = &(g_tCCGsPool.pGsTransInfo[i + 1]);

        pGsTransInfoNode->iIndex = i + 1;

        pCcSubInfoNode->GcbIndex = pSubInfoNode->GcbIndex;
        pCcSubInfoNode->UserInfo.pIEDName = strdup(pSubInfoNode->UserInfo.pIEDName);
        pCcSubInfoNode->UserInfo.gcRef = strdup(pSubInfoNode->UserInfo.gcRef);
        pCcSubInfoNode->UserInfo.dataSetRef = strdup(pSubInfoNode->UserInfo.dataSetRef);
        pCcSubInfoNode->UserInfo.goID = strdup(pSubInfoNode->UserInfo.goID);
        pCcSubInfoNode->UserInfo.confRev = pSubInfoNode->UserInfo.confRev;
        pCcSubInfoNode->UserInfo.appID = pSubInfoNode->UserInfo.appID;
        pCcSubInfoNode->UserInfo.DataNum = pSubInfoNode->UserInfo.DataNum;
        pCcSubInfoNode->bIsPubGcb = FALSE;

        /*此处的NET与GSE中接收的NET不一样，此处是CC板的接收，
        所以在EDP_ConfigParseGsSubCCPort函数中将NETINFO的端口根据INTADDR改成实际端口*/
        memcpy(&(pCcSubInfoNode->NetInfo), &(pSubInfoNode->NetInfo), sizeof(NET_INFO));

        pCcSubInfoNode->pSubGooseGcb = pSubInfoNode->pSubGooseGcb;

        EDP_ConfigParseGsSubCCPort(pGsTransInfoNode, pCcSubInfoNode);
    }


    for(i = 0; i < g_usProcessPubGooseNumAll; i++)
    {
        /*实时结构*/
        pPubInfoNode = &(g_tPubGooseAll.pPubInfoRootNode[i]);

        /*CC结构*/
        pCcSubInfoNode = &(g_tCCGsPool.pGsSubInfo[i + g_usProcessSubGooseNumAll]);
        pGsTransInfoNode = &(g_tCCGsPool.pGsTransInfo[i + g_usProcessSubGooseNumAll]);
        if(i < (g_usProcessPubGooseNumAll - 1))
        {
            pCcSubInfoNode->next = (&(g_tCCGsPool.pGsSubInfo[i + g_usProcessSubGooseNumAll + 1]));
            pGsTransInfoNode->next = &(g_tCCGsPool.pGsTransInfo[i + g_usProcessSubGooseNumAll + 1]);
        }
        pGsTransInfo11 = pGsTransInfoNode;
        pGsTransInfoNode->iIndex = i + g_usProcessSubGooseNumAll + 1;

        pCcSubInfoNode->GcbIndex = pPubInfoNode->GcbIndex + g_usProcessSubGooseNumAll;
        pCcSubInfoNode->UserInfo.pIEDName = strdup(pPubInfoNode->UserInfo.pIEDName);
        pCcSubInfoNode->UserInfo.gcRef = strdup(pPubInfoNode->UserInfo.gcRef);
        pCcSubInfoNode->UserInfo.dataSetRef = strdup(pPubInfoNode->UserInfo.dataSetRef);
        pCcSubInfoNode->UserInfo.goID = strdup(pPubInfoNode->UserInfo.goID);
        pCcSubInfoNode->UserInfo.confRev = pPubInfoNode->UserInfo.confRev;
        pCcSubInfoNode->UserInfo.appID = pPubInfoNode->UserInfo.appID;
        pCcSubInfoNode->UserInfo.DataNum = pPubInfoNode->UserInfo.DataNum;
        pCcSubInfoNode->bIsPubGcb = TRUE;

        /*对于发送GCB，应该NETINFO中的端口信息没什么用*/
        memcpy(&(pCcSubInfoNode->NetInfo), &(pPubInfoNode->NetInfo), sizeof(NET_INFO));

        pCcSubInfoNode->pSubGooseGcb = pPubInfoNode->pPubGooseGcb;

        EDP_ConfigParseGsPubCCPort(i,pGsTransInfoNode,pCcSubInfoNode);
    }

exit:
    return res;
}



/*
描述: 解析过程层配置信息，内存数据结构转换，用于生成文件
参数:     NONE
返回值: 解析是否成功
 */
int EDP_ConfigParseCC()
{
    int res = 0;

    res = EDP_ConfigParseCCSv();
    if(res != 0)
    {
        CFG_LOG("------>过程层SV 配置转换数据结构失败，错误码:%08x\n",res,0,0,0,0,0);
    }

    res = EDP_ConfigParseCCGs();
    if(res != 0)
    {
        CFG_LOG("------>过程层GS 配置转换数据结构失败，错误码:%08x\n",res,0,0,0,0,0);
    }

    return res;
}


/*
描述: 判断该GSE的转发关系的源接收地址中是否有该BOARD的
参数:
iBoardId:板号
pGsTransInfo:该GCB的转发关系
pRes:用于返回该板号的接收和发送端口
返回值:该板是否接收
TRUE 接收
FALSE 不接收
 */
BOOL EDP_GetCCTransInfo(int iBoardId, GS_TRANS_INFO *pGsTransInfo, GS_TRANS_INFO*pRes)
{
    int i = 0;
    int boardidTmp = 0;

    if(pGsTransInfo == NULL || pRes == NULL)
    {
        return FALSE;
    }

    pRes->iGcbSrcPortCnt = 0;
    for(i = 0; i < pGsTransInfo->iGcbSrcPortCnt; i++)
    {
        boardidTmp = EDP_GetBoardId(pGsTransInfo->pSrcPortList[i]);
        if(boardidTmp == iBoardId)
        {
            pRes->pSrcPortList[pRes->iGcbSrcPortCnt] = strdup(pGsTransInfo->pSrcPortList[i]);
            pRes->iGcbSrcPortCnt++;
        }
    }

    if(pRes->iGcbSrcPortCnt == 0)
    {
        return FALSE;
    }

    pRes->iGcbDstPortCnt = 0;
    for(i = 0; i < pGsTransInfo->iGcbDstPortCnt; i++)
    {
        boardidTmp = EDP_GetBoardId(pGsTransInfo->pDstPortList[i]);
        if(boardidTmp == iBoardId)
        {
            pRes->pDstPortList[pRes->iGcbDstPortCnt] = strdup(pGsTransInfo->pDstPortList[i]);
            pRes->iGcbDstPortCnt++;
        }
    }

    return TRUE;
}

/*
描述: 解析过程层配置信息，生成文件
参数:
iBoardId:板号
返回值: 解析是否成功
 */
int EDP_CreateCCGsFilePerBoard(int iBoardId, CC_TRANSMIT_STR *pCcTransmitStr)
{
    int res = 0;
    int i = 0;
    int j = 0;
    FILE *fp;
    mxml_node_t *root_node = NULL;
    mxml_node_t *mxml_config = NULL;
    mxml_node_t *mxml_datainfo = NULL;
    mxml_node_t *mxml_datainfo_subgoose = NULL;
    mxml_node_t *node_gcb = NULL;
    mxml_node_t *node_net = NULL;
    mxml_node_t *node_address = NULL;
    mxml_node_t *node_address_port = NULL;
    mxml_node_t *node_address_dstmac = NULL;
    mxml_node_t *node_address_srcmac = NULL;
    mxml_node_t *node_dstaddr = NULL;
    mxml_node_t *node_dstaddr_port = NULL;
    PRIVATE_BOARD *pBoard=NULL;
    char str[128];
    int xxxxxxx = 0;


    int idstport = 0;


    GSE_SUB_INFO *pCcSubInfoNode = NULL;
    GS_TRANS_INFO *pGsTransInfo = NULL;
    GS_TRANS_INFO *pTransRes = NULL;

    fp = fopen(pCcTransmitStr->pGsFilePath, "w");
    if(fp==NULL)
    {
        res = EDP_CC_GS_ERR_FILE_CREATE_FAIL;
        goto exit;
    }

    pBoard = EDP_GetPrivateBoardPtrFromId(iBoardId);

    root_node = mxmlNewXML("1.0");

    mxml_config = mxmlNewElement(root_node, "Config");
    mxmlElementSetAttr(mxml_config, "PROJECTNAME", "SAC");
    mxmlElementSetAttr(mxml_config, "VERSION", "1.0");
    sprintf(str,"0x%02x",pBoard->iBoardId);
    mxmlElementSetAttr(mxml_config, "boardSn", str);
    mxmlElementSetAttr(mxml_config, "boardDesc", pBoard->pBoardDesc);
    sprintf(str,"%d",pBoard->iGsPacketFlow);
    mxmlElementSetAttr(mxml_config, "GsPacketFlow", str);
    sprintf(str,"%d",pBoard->iGsDataFlow);
    mxmlElementSetAttr(mxml_config, "GsDataFlow", str);

    mxml_datainfo = mxmlNewElement(mxml_config, "DATAINFO");
    mxml_datainfo_subgoose = mxmlNewElement(mxml_datainfo, "SUB_GOOSE");

    pTransRes = (GS_TRANS_INFO *)calloc(1, sizeof(GS_TRANS_INFO));
    /*
    mxmlNewElement , mxmlNewInteger , mxmlNewOpaque ,
    mxmlNewReal , mxmlNewText mxmlNewTextf mxmlNewXML .
    */
    for(i = 0; i < g_iCcGcbCnt; i++)
    {
        memset(pTransRes,0,sizeof(GS_TRANS_INFO));
        pCcSubInfoNode = &(g_tCCGsPool.pGsSubInfo[i]);
        pGsTransInfo = &(g_tCCGsPool.pGsTransInfo[i]);

        /*先判断该GSE是否被该BOARD接收，判断GSE的源端口列表中是否有该board的端口*/
        if(!EDP_GetCCTransInfo(iBoardId, pGsTransInfo, pTransRes))
        {
            continue;
        }

        if(i ==  (g_iCcGcbCnt - 1))
        {
            xxxxxxx = 1;
        }
        node_gcb = mxmlNewElement(mxml_datainfo_subgoose, "GCB");
        /*
        <GCB IEDName="IL1101" DESC="110kV线路一合智一体装置:智能终端LD:GOOSE采样"
        GcRef="IL1101RPIT/LLN0$GO$gocb5" DatasetRef="IL1101RPIT/LLN0$dsGOOSE5" GoID="IL1101RPIT/LLN0$GO$gocb5"
        ConfRev="1" AppID="010B" Priority="6" T0="5000" T1="2" YabanID="" AdmYabanID="" Gmrp="0">
            <NET Type="ALONE" Num="1">
            	<Address>
            		<PORT>0</PORT>
            		<DST_MAC>01-0C-CD-01-00-0B</DST_MAC>
            	</Address>
            	<DST_Address>
            		<PORT>2</PORT>
            	</DST_Address>
            	<DST_Address>
            		<PORT>3</PORT>
            	</DST_Address>
            	<DST_Address>
            		<PORT>4</PORT>
            	</DST_Address>
            	<DST_Address>
            		<PORT>5</PORT>
            	</DST_Address>
            </NET>
        </GCB>
        */
        mxmlElementSetAttr(node_gcb, "IEDName", pCcSubInfoNode->UserInfo.pIEDName);
        mxmlElementSetAttr(node_gcb, "GcRef", pCcSubInfoNode->UserInfo.gcRef);
        mxmlElementSetAttr(node_gcb, "DatasetRef", pCcSubInfoNode->UserInfo.dataSetRef);
        mxmlElementSetAttr(node_gcb, "GoID", pCcSubInfoNode->UserInfo.goID);
        sprintf(str,"0x%08lx",pCcSubInfoNode->UserInfo.confRev);
        mxmlElementSetAttr(node_gcb, "ConfRev", str);
        sprintf(str,"%04x",pCcSubInfoNode->UserInfo.appID);
        mxmlElementSetAttr(node_gcb, "AppID", str);

        mxmlElementSetAttr(node_gcb, "DESC", "");
        mxmlElementSetAttr(node_gcb, "YabanID", "");
        mxmlElementSetAttr(node_gcb, "AdmYabanID", "");
        mxmlElementSetAttr(node_gcb, "Gmrp", "0");
        mxmlElementSetAttr(node_gcb, "T0", (char *)"5000");
        mxmlElementSetAttr(node_gcb, "T1", (char *)"2");
        mxmlElementSetAttr(node_gcb, "Priority", (char *)"6");


        node_net = mxmlNewElement(node_gcb, "NET");
        /*<NET Type="ALONE" Num="1">*/
        mxmlElementSetAttr(node_net, "Type", "ALONE");
        /*mxmlElementSetAttr(node_net, "Num", "1");*/

        for(j = 0; j < pTransRes->iGcbSrcPortCnt; j++)
        {
            node_address = mxmlNewElement(node_net, "Address");
            node_address_port = mxmlNewElement(node_address, "PORT");
            sprintf(str,"%d", EDP_GetPortId(pTransRes->pSrcPortList[j]));
            mxmlNewText(node_address_port, 0, str);
            node_address_dstmac = mxmlNewElement(node_address, "DST_MAC");
            sprintf(str,"%02X-%02X-%02X-%02X-%02X-%02X",
                    pCcSubInfoNode->NetInfo.addr[j].dst_mac[0],pCcSubInfoNode->NetInfo.addr[j].dst_mac[1],
                    pCcSubInfoNode->NetInfo.addr[j].dst_mac[2],pCcSubInfoNode->NetInfo.addr[j].dst_mac[3],
                    pCcSubInfoNode->NetInfo.addr[j].dst_mac[4],pCcSubInfoNode->NetInfo.addr[j].dst_mac[5]);
            mxmlNewText(node_address_dstmac, 0, str);
        }

        for(j = 0; j < pTransRes->iGcbDstPortCnt; j++)
        {
            node_dstaddr = mxmlNewElement(node_net, "DST_Address");
            node_dstaddr_port = mxmlNewElement(node_dstaddr, "PORT");
            idstport = EDP_GetPortId(pTransRes->pDstPortList[j]);
            sprintf(str,"%d", idstport);
            mxmlNewText(node_dstaddr_port, 0, str);
            if(pCcSubInfoNode->bIsPubGcb)
            {
                node_address_srcmac = mxmlNewElement(node_dstaddr, "SRC_MAC");
                sprintf(str,"B8-AC-6F-4D-%02X-F7", idstport);
                mxmlNewText(node_address_srcmac, 0, str);
            }
        }
    }

    if(mxmlSaveFile(root_node, fp, whitespace_cb) != 0)
    {
        CFG_LOG("------> 生成XML文件出错BOARDID:%d \n",iBoardId,0,0,0,0,0);
        res = EDP_CC_GS_ERR_FILE_SAVE_FAIL;
        goto exit;
    }

    fclose(fp);
    mxmlDelete(root_node);
    free(pTransRes);

exit:
    return res;
}

/*
描述: 解析过程层配置信息，生成文件
参数:     NONE
返回值: 解析是否成功
 */
BOOL EDP_InitCcTransmit(CC_TRANSMIT_STR *pCcTransmitStr)
{
    int iBoardId = 0;
    int i = 0;
    int j = 0;
    PRIVATE_BOARD *pBoard = NULL;
    PRIVATE_BOARD *pBoardFather = NULL;
    BOARD_PORT_CONNECT *pPortConnect = NULL;
    BOARD_PORT_CONNECT *pPortConnectFather = NULL;
    BOOL res = FALSE;

    if(pCcTransmitStr == NULL)
    {
        goto exit;
    }

    iBoardId = pCcTransmitStr->iBoardId;
    sprintf(pCcTransmitStr->pGsFilePath,"/tffs/lgh/gs%d.xml",iBoardId);
    sprintf(pCcTransmitStr->pSvFilePath,"/tffs/lgh/sv%d.xml",iBoardId);
    sprintf(pCcTransmitStr->pZipFilePath,"/tffs/lgh/cfg%d.zip",iBoardId);

    pBoard = EDP_GetPrivateBoardPtrFromId(iBoardId);
    if(pBoard != NULL)
    {
        if(pBoard->iParentCCId == iBoardId)
        {
            pCcTransmitStr->bIsMasterCC = TRUE;
            for(i = 0; i < pBoard->tPort.iConnectCnt; i++)
            {
                pPortConnect = &(pBoard->tPort.pPortConnect[i]);
                /*配置文件只从0口接收*/
                if(pPortConnect->iPortId == 0)
                {
                    pCcTransmitStr->ucSrcPort[0] = pPortConnect->iOutPortId;
                    pCcTransmitStr->ucSrcPort[1] = 0;
                    res = TRUE;
                }

                switch(pPortConnect->iDataType)
                {
                    case PORT_TRANSFOR_DATA_TYPE_GS:
                        if((pCcTransmitStr->iDataType == PORT_TRANSFOR_DATA_TYPE_SV)
                                || (pCcTransmitStr->iDataType == PORT_TRANSFOR_DATA_TYPE_SVGS))
                        {
                            pCcTransmitStr->iDataType = PORT_TRANSFOR_DATA_TYPE_SVGS;
                        }
                        else
                        {
                            pCcTransmitStr->iDataType = PORT_TRANSFOR_DATA_TYPE_GS;
                        }
                        break;
                    case PORT_TRANSFOR_DATA_TYPE_SV:
                        if((pCcTransmitStr->iDataType == PORT_TRANSFOR_DATA_TYPE_GS)
                                || (pCcTransmitStr->iDataType == PORT_TRANSFOR_DATA_TYPE_SVGS))
                        {
                            pCcTransmitStr->iDataType = PORT_TRANSFOR_DATA_TYPE_SVGS;
                        }
                        else
                        {
                            pCcTransmitStr->iDataType = PORT_TRANSFOR_DATA_TYPE_SV;
                        }
                        break;
                    case PORT_TRANSFOR_DATA_TYPE_SVGS:
                        pCcTransmitStr->iDataType = PORT_TRANSFOR_DATA_TYPE_SVGS;
                        break;
                    default:
                        res = FALSE;
                        goto exit;
                }
            }
        }
        else
        {
            pCcTransmitStr->bIsMasterCC = FALSE;
            for(i = 0; i < pBoard->tPort.iConnectCnt; i++)
            {
                pPortConnect = &(pBoard->tPort.pPortConnect[i]);
                /*配置文件只从0口接收*/
                if(pPortConnect->iPortId == 0)
                {
                    pCcTransmitStr->ucSrcPort[1] = pPortConnect->iOutPortId;
                    pBoardFather = EDP_GetPrivateBoardPtrFromId(pPortConnect->iOutBoardId);
                    if(pBoardFather != NULL)
                    {
                        for(j = 0; j < pBoardFather->tPort.iConnectCnt; j++)
                        {
                            pPortConnectFather = &(pBoardFather->tPort.pPortConnect[j]);
                            /*配置文件只从0口接收*/
                            if(pPortConnectFather->iPortId == 0)
                            {
                                pCcTransmitStr->ucSrcPort[0] = pPortConnectFather->iOutPortId;
                                res = TRUE;
                            }
                        }

                        switch(pPortConnect->iDataType)
                        {
                            case PORT_TRANSFOR_DATA_TYPE_GS:
                                if((pCcTransmitStr->iDataType == PORT_TRANSFOR_DATA_TYPE_SV)
                                        || (pCcTransmitStr->iDataType == PORT_TRANSFOR_DATA_TYPE_SVGS))
                                {
                                    pCcTransmitStr->iDataType = PORT_TRANSFOR_DATA_TYPE_SVGS;
                                }
                                else
                                {
                                    pCcTransmitStr->iDataType = PORT_TRANSFOR_DATA_TYPE_GS;
                                }
                                break;
                            case PORT_TRANSFOR_DATA_TYPE_SV:
                                if((pCcTransmitStr->iDataType == PORT_TRANSFOR_DATA_TYPE_GS)
                                        || (pCcTransmitStr->iDataType == PORT_TRANSFOR_DATA_TYPE_SVGS))
                                {
                                    pCcTransmitStr->iDataType = PORT_TRANSFOR_DATA_TYPE_SVGS;
                                }
                                else
                                {
                                    pCcTransmitStr->iDataType = PORT_TRANSFOR_DATA_TYPE_SV;
                                }
                                break;
                            case PORT_TRANSFOR_DATA_TYPE_SVGS:
                                pCcTransmitStr->iDataType = PORT_TRANSFOR_DATA_TYPE_SVGS;
                                break;
                            default:
                                res = FALSE;
                                goto exit;
                        }
                    }
                    break;
                }
            }
        }
    }

exit:
    return res;
}

/*
描述: 解析过程层配置信息，生成文件
参数:
iBoardId:板号
返回值: 解析是否成功
 */
int EDP_CreateCCSvFilePerBoard(int iBoardId, CC_TRANSMIT_STR *pCcTransmitStr)
{
    int res = 0;
    int i = 0;
    int j = 0;
    FILE *fp;
    mxml_node_t *root_node = NULL;
    mxml_node_t *mxml_cfg = NULL;
    mxml_node_t *mxml_smvp2p = NULL;
    mxml_node_t *mxml_subsmvp2p = NULL;
    mxml_node_t *mxml_pubsmvp2p = NULL;
    mxml_node_t *mxml_asdu = NULL;
    SV_TRANS_INFO* pSvTransInfo = NULL;
    PRIVATE_BOARD *pBoard=NULL;
    char str[128];
    uint64_t select;

    pBoard = EDP_GetPrivateBoardPtrFromId(iBoardId);
    pSvTransInfo = GetSvTransInfoByBoardId(iBoardId);
    if((pSvTransInfo == NULL) || (pBoard == NULL))
    {
        res = EDP_CC_SV_ERR_FIND_BOARD_FAIL;
        goto exit;
    }

    fp = fopen(pCcTransmitStr->pSvFilePath, "w");
    if(fp==NULL)
    {
        res = EDP_CC_SV_ERR_FILE_CREATE_FAIL;
        goto exit;
    }

    root_node = mxmlNewXML("1.0");

    mxml_cfg = mxmlNewElement(root_node, "CFG");
    mxmlElementSetAttr(mxml_cfg, "ver", "1.0");
    sprintf(str,"0x%02x",pBoard->iBoardId);
    mxmlElementSetAttr(mxml_cfg, "boardSn", str);
    mxmlElementSetAttr(mxml_cfg, "boardDesc", pBoard->pBoardDesc);

    mxml_smvp2p = mxmlNewElement(mxml_cfg, "SMV_P2P");
    sprintf(str,"%d", pSvTransInfo->iMode);
    mxmlElementSetAttr(mxml_smvp2p, "mode", str);
    sprintf(str,"%d", pSvTransInfo->iFuncType);
    mxmlElementSetAttr(mxml_smvp2p, "funcType", str);
    sprintf(str,"%d", pSvTransInfo->iSubNum);
    mxmlElementSetAttr(mxml_smvp2p, "subNum", str);
    sprintf(str,"%d", pSvTransInfo->iSourceRate);
    mxmlElementSetAttr(mxml_smvp2p, "sourceRate", str);
    sprintf(str,"%d", pSvTransInfo->iMaxDelay);
    mxmlElementSetAttr(mxml_smvp2p, "maxdelay", str);
    sprintf(str,"%d", pSvTransInfo->iSynPulse);
    mxmlElementSetAttr(mxml_smvp2p, "synPulse", str);
    sprintf(str,"%d", pSvTransInfo->iSynPulseEnable);
    mxmlElementSetAttr(mxml_smvp2p, "synPulseEnable", str);
    sprintf(str,"%d", pSvTransInfo->iExtSynMode);
    mxmlElementSetAttr(mxml_smvp2p, "extSynMode", str);
    sprintf(str,"%d", pSvTransInfo->iExtSynReverse);
    mxmlElementSetAttr(mxml_smvp2p, "extSynReverse", str);
    sprintf(str,"%d", pSvTransInfo->iGmrpSendGap);
    mxmlElementSetAttr(mxml_smvp2p, "gmrpSendGap", str);

    for(i = 0; i < pSvTransInfo->iSubNum; i++)
    {
        if(pSvTransInfo->pCcSvSubInfo[i].iAsduNum == 0)
        {
            continue;
        }
        mxml_subsmvp2p = mxmlNewElement(mxml_smvp2p, "SUB_SMV_P2P");

        sprintf(str,"%d", pSvTransInfo->pCcSvSubInfo[i].iPort);
        mxmlElementSetAttr(mxml_subsmvp2p, "port", str);
        sprintf(str,"%02X-%02X-%02X-%02X-%02X-%02X",
                pSvTransInfo->pCcSvSubInfo[i].ucMacAddr[0], pSvTransInfo->pCcSvSubInfo[i].ucMacAddr[1],
                pSvTransInfo->pCcSvSubInfo[i].ucMacAddr[2], pSvTransInfo->pCcSvSubInfo[i].ucMacAddr[3],
                pSvTransInfo->pCcSvSubInfo[i].ucMacAddr[4], pSvTransInfo->pCcSvSubInfo[i].ucMacAddr[5]);
        mxmlElementSetAttr(mxml_subsmvp2p, "macAddr", str);
        sprintf(str,"%d", pSvTransInfo->pCcSvSubInfo[i].iSrc);
        mxmlElementSetAttr(mxml_subsmvp2p, "src", str);
        sprintf(str,"%d", pSvTransInfo->pCcSvSubInfo[i].iSubBoard);
        mxmlElementSetAttr(mxml_subsmvp2p, "subboard", str);
        sprintf(str,"%d", pSvTransInfo->pCcSvSubInfo[i].iAsduNum);
        mxmlElementSetAttr(mxml_subsmvp2p, "asduNum", str);
        sprintf(str,"0x%04x", pSvTransInfo->pCcSvSubInfo[i].usTCI);
        mxmlElementSetAttr(mxml_subsmvp2p, "TCI", str);
        sprintf(str,"0x%04x", pSvTransInfo->pCcSvSubInfo[i].usAppID);
        mxmlElementSetAttr(mxml_subsmvp2p, "appID", str);
        sprintf(str,"0x%08lx", pSvTransInfo->pCcSvSubInfo[i].pCcSvSubAsduInfo[0].ulConfRev);
        mxmlElementSetAttr(mxml_subsmvp2p, "ConfRev", str);
        sprintf(str,"%d", pSvTransInfo->pCcSvSubInfo[i].iGmrp);
        mxmlElementSetAttr(mxml_subsmvp2p, "Gmrp", str);
        if(pSvTransInfo->pCcSvSubInfo[i].pDesc == NULL)
        {
            mxmlElementSetAttr(mxml_subsmvp2p, "desc", "");
        }
        else
        {
            mxmlElementSetAttr(mxml_subsmvp2p, "desc", pSvTransInfo->pCcSvSubInfo[i].pDesc);
        }

        for(j = 0; j < pSvTransInfo->pCcSvSubInfo[i].iAsduNum; j++)
        {
            mxml_asdu = mxmlNewElement(mxml_subsmvp2p, "ASDU");

            sprintf(str,"%d", pSvTransInfo->pCcSvSubInfo[i].pCcSvSubAsduInfo[j].iID);
            mxmlElementSetAttr(mxml_asdu, "ID", str);
            mxmlElementSetAttr(mxml_asdu, "svID", pSvTransInfo->pCcSvSubInfo[i].pCcSvSubAsduInfo[j].pSvID);
            sprintf(str,"%d", pSvTransInfo->pCcSvSubInfo[i].pCcSvSubAsduInfo[j].iChnNum);
            mxmlElementSetAttr(mxml_asdu, "chnNum", str);
            /*
            sprintf(str,"0x%08lx", pSvTransInfo->pCcSvSubInfo[i].pCcSvSubAsduInfo[j].ulConfRev);
            mxmlElementSetAttr(mxml_asdu, "confRev", str);
            */
            sprintf(str,"%d", pSvTransInfo->pCcSvSubInfo[i].pCcSvSubAsduInfo[j].iShift);
            mxmlElementSetAttr(mxml_asdu, "shift", str);
            sprintf(str,"0x%08lx", pSvTransInfo->pCcSvSubInfo[i].pCcSvSubAsduInfo[j].ulDelayChan);
            mxmlElementSetAttr(mxml_asdu, "delayChan", str);
        }
    }

    for(i = 0; i < pSvTransInfo->iPubNum; i++)
    {
        mxml_pubsmvp2p = mxmlNewElement(mxml_smvp2p, "PUB_SMV_P2P");

        sprintf(str,"%d", pSvTransInfo->pCcSvPubInfo[i].iPort);
        mxmlElementSetAttr(mxml_pubsmvp2p, "port", str);
        sprintf(str,"%d", pSvTransInfo->pCcSvPubInfo[i].iNodeAddr);
        mxmlElementSetAttr(mxml_pubsmvp2p, "nodeAddr", str);
        sprintf(str,"%02X-%02X-%02X-%02X-%02X-%02X",
                pSvTransInfo->pCcSvPubInfo[i].ucMacAddrSrc[0], pSvTransInfo->pCcSvPubInfo[i].ucMacAddrSrc[1],
                pSvTransInfo->pCcSvPubInfo[i].ucMacAddrSrc[2], pSvTransInfo->pCcSvPubInfo[i].ucMacAddrSrc[3],
                pSvTransInfo->pCcSvPubInfo[i].ucMacAddrSrc[4], pSvTransInfo->pCcSvPubInfo[i].ucMacAddrSrc[5]);
        mxmlElementSetAttr(mxml_pubsmvp2p, "macAddrSrc", str);
        sprintf(str,"%02X-%02X-%02X-%02X-%02X-%02X",
                pSvTransInfo->pCcSvPubInfo[i].ucMacAddrDes[0], pSvTransInfo->pCcSvPubInfo[i].ucMacAddrDes[1],
                pSvTransInfo->pCcSvPubInfo[i].ucMacAddrDes[2], pSvTransInfo->pCcSvPubInfo[i].ucMacAddrDes[3],
                pSvTransInfo->pCcSvPubInfo[i].ucMacAddrDes[4], pSvTransInfo->pCcSvPubInfo[i].ucMacAddrDes[5]);
        mxmlElementSetAttr(mxml_pubsmvp2p, "macAddrDes", str);
        sprintf(str,"0x%08lx", pSvTransInfo->pCcSvPubInfo[i].ulConfRev);
        mxmlElementSetAttr(mxml_pubsmvp2p, "ConfRev", str);
        sprintf(str,"%d", pSvTransInfo->pCcSvPubInfo[i].iAsduNum);
        mxmlElementSetAttr(mxml_pubsmvp2p, "asduNum", str);
        sprintf(str,"0x%04x", pSvTransInfo->pCcSvPubInfo[i].usTCI);
        mxmlElementSetAttr(mxml_pubsmvp2p, "TCI", str);
        sprintf(str,"0x%04x", pSvTransInfo->pCcSvPubInfo[i].usAppID);
        mxmlElementSetAttr(mxml_pubsmvp2p, "appID", str);
        sprintf(str,"%d", pSvTransInfo->pCcSvPubInfo[i].usPubRate);
        mxmlElementSetAttr(mxml_pubsmvp2p, "pubRate", str);

        if(pSvTransInfo->pCcSvPubInfo[i].pDesc == NULL)
        {
            mxmlElementSetAttr(mxml_pubsmvp2p, "desc", "");
        }
        else
        {
            mxmlElementSetAttr(mxml_pubsmvp2p, "desc", pSvTransInfo->pCcSvPubInfo[i].pDesc);
        }

        for(j = 0; j < pSvTransInfo->pCcSvPubInfo[i].iAsduNum; j++)
        {
            mxml_asdu = mxmlNewElement(mxml_pubsmvp2p, "ASDU");

            sprintf(str,"%d", pSvTransInfo->pCcSvPubInfo[i].pCcSvPubAsduInfo[j].iID);
            mxmlElementSetAttr(mxml_asdu, "ID", str);
            mxmlElementSetAttr(mxml_asdu, "svID", pSvTransInfo->pCcSvPubInfo[i].pCcSvPubAsduInfo[j].pSvID);
            select = pSvTransInfo->pCcSvPubInfo[i].pCcSvPubAsduInfo[j].ulSelect2;
            select = (select<<32) | pSvTransInfo->pCcSvPubInfo[i].pCcSvPubAsduInfo[j].ulSelect1;
            sprintf(str,"0x%010llx", select);
            mxmlElementSetAttr(mxml_asdu, "select", str);
        }
    }

    if(mxmlSaveFile(root_node, fp, whitespace_cb_sv) != 0)
    {
        CFG_LOG("------> 生成XML文件出错BOARDID:%d \n",iBoardId,0,0,0,0,0);
        res = EDP_CC_GS_ERR_FILE_SAVE_FAIL;
        goto exit;
    }

    fclose(fp);
    mxmlDelete(root_node);

exit:
    return res;
}

/*
描述: 解析过程层配置信息，CC转发结构
参数:     NONE
返回值: 解析是否成功
 */
int EDP_CreateCCTransStruct()
{
    int i = 0;
    int iBoardId = 0;
    PRIVATE_BOARD *pBoard = NULL;
    CC_TRANSMIT_STR *pCcTransmitStr = NULL;
    int iTmp = 0;

    /*获取CC个数*/
    g_ucCCCnt = EDP_GetCCCnt();

    /*CC转发结构*/
    g_tCCTransmitInfo.ucCCCnt = g_ucCCCnt;
    g_tCCTransmitInfo.pCcTransmitStr = (CC_TRANSMIT_STR *)calloc(g_ucCCCnt, sizeof(CC_TRANSMIT_STR));
    if(!g_tCCTransmitInfo.pCcTransmitStr)
    {
        CFG_LOG("------>  CC转发结构分配内存失败.CC 个数:%d \n",g_ucCCCnt,0,0,0,0,0);
        return FALSE;
    }

    for(i = 0; i < EDP_GetPrivateBoardCnt(); i++)
    {
        pBoard = EDP_GetPrivateBoardPtrFromIndex(i);
        if(pBoard->iBoardType != BOARD_TYPE_CC)
            continue;
        iBoardId = pBoard->iBoardId;
        pCcTransmitStr = &(g_tCCTransmitInfo.pCcTransmitStr[iTmp]);
        pCcTransmitStr->next = &(g_tCCTransmitInfo.pCcTransmitStr[iTmp+1]);
        iTmp++;
        pCcTransmitStr->iBoardId = iBoardId;

        EDP_InitCcTransmit(pCcTransmitStr);
    }

    return 0;
}


/*
描述: 解析过程层配置信息，生成文件
参数:     NONE
返回值: 解析是否成功
 */
int EDP_CreateCCFile()
{
    int i = 0;
    int iBoardId = 0;
    //int iPortId = 0;
    PRIVATE_BOARD *pBoard = NULL;
    CC_TRANSMIT_STR *pCcTransmitStr = NULL;
    FILE* zipfd;
    char filepath[2][128];
    char filename[2][128];
    int iTmp = 0;
#if 0
    /*获取CC个数*/
    g_ucCCCnt = EDP_GetCCCnt();

    /*CC转发结构*/
    g_tCCTransmitInfo.ucCCCnt = g_ucCCCnt;
    g_tCCTransmitInfo.pCcTransmitStr = (CC_TRANSMIT_STR *)calloc(g_ucCCCnt, sizeof(CC_TRANSMIT_STR));
    if(!g_tCCTransmitInfo.pCcTransmitStr)
    {
        CFG_LOG("------>  CC转发结构分配内存失败.CC 个数:%d \n",g_ucCCCnt,0,0,0,0,0);
        return FALSE;
    }
#endif

    for(i = 0; i < EDP_GetPrivateBoardCnt(); i++)
    {
#if 0
        pBoard = EDP_GetPrivateBoardPtrFromIndex(i);
        if(pBoard->iBoardType != BOARD_TYPE_CC)
            continue;
        iBoardId = pBoard->iBoardId;
        pCcTransmitStr = &(g_tCCTransmitInfo.pCcTransmitStr[iTmp]);
        pCcTransmitStr->next = &(g_tCCTransmitInfo.pCcTransmitStr[iTmp+1]);
        iTmp++;
        pCcTransmitStr->iBoardId = iBoardId;

        EDP_InitCcTransmit(pCcTransmitStr);
#endif

        pBoard = EDP_GetPrivateBoardPtrFromIndex(i);
        if(pBoard->iBoardType != BOARD_TYPE_CC)
            continue;
        iBoardId = pBoard->iBoardId;
        pCcTransmitStr = &(g_tCCTransmitInfo.pCcTransmitStr[iTmp]);
        iTmp++;

        /*在进行ZIP压缩，压缩完在删掉源xml文件*/
        zipfd = fopen(pCcTransmitStr->pZipFilePath, "w+");

        if(pCcTransmitStr->iDataType == PORT_TRANSFOR_DATA_TYPE_SVGS)
        {
            EDP_CreateCCGsFilePerBoard(iBoardId, pCcTransmitStr);
            EDP_CreateCCSvFilePerBoard(iBoardId, pCcTransmitStr);
            strcpy((char*)filepath[0], pCcTransmitStr->pGsFilePath);
            strcpy((char*)filepath[1], pCcTransmitStr->pSvFilePath);
            strcpy((char*)filename[0], "gs.xml");
            strcpy((char*)filename[1], "sv.xml");
            ProcessCfgZip(2,filepath,filename,zipfd);
        }
        else if(pCcTransmitStr->iDataType == PORT_TRANSFOR_DATA_TYPE_GS)
        {
            EDP_CreateCCGsFilePerBoard(iBoardId, pCcTransmitStr);
            strcpy((char*)filepath[0], pCcTransmitStr->pGsFilePath);
            strcpy((char*)filename[0], "gs.xml");
            ProcessCfgZip(1,filepath,filename,zipfd);
        }
        else if(pCcTransmitStr->iDataType == PORT_TRANSFOR_DATA_TYPE_SV)
        {
            EDP_CreateCCSvFilePerBoard(iBoardId, pCcTransmitStr);
            strcpy((char*)filepath[0], pCcTransmitStr->pSvFilePath);
            strcpy((char*)filename[0], "sv.xml");
            ProcessCfgZip(1,filepath,filename,zipfd);
        }

        fclose(zipfd);
    }

    return 0;
}

char * GetStrbyDataType(VALUETYPE valueType)
{
    if(valueType == BOOL_TYPE)
        return "BOOL";
    else if(valueType == DPC_TYPE)
        return "DBPOS";
    else if(valueType == INT_TYPE)
        return "INT";
    else if(valueType == UINT_TYPE)
        return "UINT";
    else if(valueType == FLOAT_TYPE)
        return "FLOAT";
    else if(valueType == QUALITY_TYPE)
        return "QUALITY";
    else if(valueType == UTCTIME_TYPE)
        return "UTC";
    return "UNKNOW_TYPE";
}

/*
描述: 生成GSE.XML文件
GSE中的接收端口信息应该是从private中的该CPU ID的GS级联口
参数:
iBoardId:板号
返回值: 解析是否成功
 */
int EDP_CreateCPUGseFile()
{
    int res = 0;
    int i = 0;
    int j = 0;
    int k = 0;
    FILE *fp;
    mxml_node_t *root_node = NULL;
    mxml_node_t *mxml_config = NULL;
    mxml_node_t *mxml_datainfo = NULL;
    mxml_node_t *mxml_datainfo_subgoose = NULL;
    mxml_node_t *mxml_datainfo_pubgoose = NULL;
    mxml_node_t *mxml_datamap = NULL;
    mxml_node_t *mxml_datamap_subdataset = NULL;
    mxml_node_t *mxml_datamap_pubdataset = NULL;
    mxml_node_t *mxml_datamap_fcda = NULL;
    mxml_node_t *node_gcb = NULL;
    mxml_node_t *node_net = NULL;
    mxml_node_t *node_address = NULL;
    mxml_node_t *node_address_port = NULL;
    mxml_node_t *node_address_vid = NULL;
    mxml_node_t *node_address_dstmac = NULL;
    mxml_node_t *node_dataset = NULL;
    mxml_node_t *node_dataset_fcda = NULL;
    char str[128];

    GSE_SUB_INFO *pSubInfoNode = NULL;
    SUB_MAP_INFO *pSubMapNode = NULL;
    GSE_PUB_INFO *pPubInfoNode = NULL;
    PUB_MAP_INFO *pPubMapNode = NULL;
    SUB_GCB_INFO *pSubGcbCfg = NULL;
    PUB_GCB_INFO *pPubGcbCfg = NULL;
    DA_INFO *pFcda = NULL;

    if((g_usProcessSubGooseNum == 0) && (g_usProcessPubGooseNum == 0))
    {
        goto exit;
    }

    fp = fopen(CONFIG_FILE, "w");
    //fp = fopen("/tffs/lgh/gse_test.xml", "w");
    if(fp==NULL)
    {
        res = EDP_CC_GS_ERR_FILE_CREATE_FAIL;
        goto exit;
    }

    root_node = mxmlNewXML("1.0");

    mxml_config = mxmlNewElement(root_node, "Config");
    mxml_datainfo = mxmlNewElement(mxml_config, "DATAINFO");
    mxml_datamap = mxmlNewElement(mxml_config, "DATAMAP");
    mxml_datainfo_subgoose = mxmlNewElement(mxml_datainfo, "SUB_GOOSE");
    mxml_datainfo_pubgoose = mxmlNewElement(mxml_datainfo, "PUB_GOOSE");
    mxml_datamap_subdataset = mxmlNewElement(mxml_datamap, "SUB_DATASET");
    mxml_datamap_pubdataset = mxmlNewElement(mxml_datamap, "PUB_DATASET");

    /*
    mxmlNewElement , mxmlNewInteger , mxmlNewOpaque ,
    mxmlNewReal , mxmlNewText mxmlNewTextf mxmlNewXML .
    */
    for(i = 0; i < g_usProcessSubGooseNum; i++)
    {
        pSubInfoNode = &(g_tSubGoose.pSubInfoRootNode[i]);

        node_gcb = mxmlNewElement(mxml_datainfo_subgoose, "GCB");

        mxmlElementSetAttr(node_gcb, "IEDName", pSubInfoNode->UserInfo.pIEDName);
        mxmlElementSetAttr(node_gcb, "GcRef", pSubInfoNode->UserInfo.gcRef);
        mxmlElementSetAttr(node_gcb, "DatasetRef", pSubInfoNode->UserInfo.dataSetRef);
        sprintf(str,"%ld",pSubInfoNode->UserInfo.confRev);
        mxmlElementSetAttr(node_gcb, "ConfRev", str);
        sprintf(str,"%04x",pSubInfoNode->UserInfo.appID);
        mxmlElementSetAttr(node_gcb, "AppID", str);
        mxmlElementSetAttr(node_gcb, "GoID", pSubInfoNode->UserInfo.goID);

        mxmlElementSetAttr(node_gcb, "DESC", "");
        if(pSubInfoNode->UserInfo.ybLogicID == NULL)
        {
            mxmlElementSetAttr(node_gcb, "YabanID", "");
        }
        else
        {
            mxmlElementSetAttr(node_gcb, "YabanID", pSubInfoNode->UserInfo.ybLogicID);
        }
        mxmlElementSetAttr(node_gcb, "AdmYabanID", "");
        mxmlElementSetAttr(node_gcb, "Gmrp", "0");
        mxmlElementSetAttr(node_gcb, "T0", (char *)"5000");
        mxmlElementSetAttr(node_gcb, "T1", (char *)"2");
        mxmlElementSetAttr(node_gcb, "Priority", "");


        node_net = mxmlNewElement(node_gcb, "NET");
        /*<NET Type="ALONE" Num="1">*/
        mxmlElementSetAttr(node_net, "Type", "ALONE");
        sprintf(str,"%d",pSubInfoNode->NetInfo.num);
        mxmlElementSetAttr(node_net, "Num", str);

        for(j = 0; j < pSubInfoNode->NetInfo.num; j++)
        {
            node_address = mxmlNewElement(node_net, "Address");
            node_address_port = mxmlNewElement(node_address, "PORT");
            sprintf(str,"%d", pSubInfoNode->NetInfo.addr[j].portnum[0]);
            mxmlNewText(node_address_port, 0, str);
            node_address_dstmac = mxmlNewElement(node_address, "DST_MAC");
            sprintf(str,"%02X-%02X-%02X-%02X-%02X-%02X",
                    pSubInfoNode->NetInfo.addr[j].dst_mac[0],pSubInfoNode->NetInfo.addr[j].dst_mac[1],
                    pSubInfoNode->NetInfo.addr[j].dst_mac[2],pSubInfoNode->NetInfo.addr[j].dst_mac[3],
                    pSubInfoNode->NetInfo.addr[j].dst_mac[4],pSubInfoNode->NetInfo.addr[j].dst_mac[5]);
            mxmlNewText(node_address_dstmac, 0, str);
        }

        node_dataset = mxmlNewElement(node_gcb, "DATASET");
        pSubGcbCfg = (SUB_GCB_INFO *)pSubInfoNode->pSubGooseGcb;
        for(j = 0; j < pSubGcbCfg->tDataSet.iFcdaCnt; j++)
        {
            pFcda = &(pSubInfoNode->pDataMbr[j]);
            node_dataset_fcda = mxmlNewElement(node_dataset, "FCDA");
            /*<FCDA INDEX="01" ORDINAL="01" TYPE="DBPOS" LEAF="PSIU601ARPIT/Q0AXCBR1$ST$Pos$stVal" DESC="断路器A相位置"></FCDA>   */
            sprintf(str,"%d",pFcda->index);
            mxmlElementSetAttr(node_dataset_fcda, "INDEX", str);
            sprintf(str,"%d",pFcda->ordinal);
            mxmlElementSetAttr(node_dataset_fcda, "ORDINAL", str);
            mxmlElementSetAttr(node_dataset_fcda, "TYPE", GetStrbyDataType(pFcda->type));
            mxmlElementSetAttr(node_dataset_fcda, "LEAF", "");
            if(pFcda->desc == NULL)
            {
                mxmlElementSetAttr(node_dataset_fcda, "DESC", "");
            }
            else
            {
                mxmlElementSetAttr(node_dataset_fcda, "DESC", pFcda->desc);
            }

            if((pFcda->bLinked = TRUE) && (pFcda->ybLogicID != NULL))
            {
                mxmlElementSetAttr(node_dataset_fcda, "YabanID", pFcda->ybLogicID);
            }
            else
            {
                mxmlElementSetAttr(node_dataset_fcda, "YabanID", "");
            }
        }

    }

    for(i = 0; i < g_usProcessPubGooseNum; i++)
    {
        pPubInfoNode = &(g_tPubGoose.pPubInfoRootNode[i]);

        node_gcb = mxmlNewElement(mxml_datainfo_pubgoose, "GCB");

        mxmlElementSetAttr(node_gcb, "IEDName", pPubInfoNode->UserInfo.pIEDName);
        mxmlElementSetAttr(node_gcb, "GcRef", pPubInfoNode->UserInfo.gcRef);
        mxmlElementSetAttr(node_gcb, "DatasetRef", pPubInfoNode->UserInfo.dataSetRef);
        sprintf(str,"%ld",pPubInfoNode->UserInfo.confRev);
        mxmlElementSetAttr(node_gcb, "ConfRev", str);
        sprintf(str,"%04x",pPubInfoNode->UserInfo.appID);
        mxmlElementSetAttr(node_gcb, "AppID", str);
        mxmlElementSetAttr(node_gcb, "GoID", pPubInfoNode->UserInfo.goID);

        mxmlElementSetAttr(node_gcb, "DESC", "");
        mxmlElementSetAttr(node_gcb, "YabanID","");
        mxmlElementSetAttr(node_gcb, "AdmYabanID", "");
        mxmlElementSetAttr(node_gcb, "Gmrp", "0");
        sprintf(str,"%ld",pPubInfoNode->UserInfo.T0);
        mxmlElementSetAttr(node_gcb, "T0", str);
        sprintf(str,"%ld",pPubInfoNode->UserInfo.T1);
        mxmlElementSetAttr(node_gcb, "T1", str);
        sprintf(str,"%d",pPubInfoNode->UserInfo.Priority);
        mxmlElementSetAttr(node_gcb, "Priority", str);


        node_net = mxmlNewElement(node_gcb, "NET");
        /*<NET Type="ALONE" Num="1">*/
        mxmlElementSetAttr(node_net, "Type", "ALONE");
        sprintf(str,"%d",pPubInfoNode->NetInfo.num);
        mxmlElementSetAttr(node_net, "Num", str);

        for(j = 0; j < pPubInfoNode->NetInfo.num; j++)
        {
            node_address = mxmlNewElement(node_net, "Address");

            for(k = 0; k < pPubInfoNode->NetInfo.addr[j].portcount; k++)
            {
                node_address_port = mxmlNewElement(node_address, "PORT");
                sprintf(str,"%d", pPubInfoNode->NetInfo.addr[j].portnum[k]);
                mxmlNewText(node_address_port, 0, str);
            }
            node_address_vid = mxmlNewElement(node_address, "VID");
            node_address_dstmac = mxmlNewElement(node_address, "DST_MAC");
            sprintf(str,"%02X-%02X-%02X-%02X-%02X-%02X",
                    pPubInfoNode->NetInfo.addr[j].dst_mac[0],pPubInfoNode->NetInfo.addr[j].dst_mac[1],
                    pPubInfoNode->NetInfo.addr[j].dst_mac[2],pPubInfoNode->NetInfo.addr[j].dst_mac[3],
                    pPubInfoNode->NetInfo.addr[j].dst_mac[4],pPubInfoNode->NetInfo.addr[j].dst_mac[5]);
            mxmlNewText(node_address_dstmac, 0, str);
            sprintf(str,"%d", pPubInfoNode->NetInfo.addr[j].vid);
            mxmlNewText(node_address_vid, 0, str);
        }

        node_dataset = mxmlNewElement(node_gcb, "DATASET");
        pPubGcbCfg = (PUB_GCB_INFO *)pPubInfoNode->pPubGooseGcb;
        for(j = 0; j < pPubGcbCfg->tDataSet.iFcdaCnt; j++)
        {
            pFcda = &(pPubInfoNode->pDataMbr[j]);
            node_dataset_fcda = mxmlNewElement(node_dataset, "FCDA");
            /*<FCDA INDEX="01" ORDINAL="01" TYPE="DBPOS" LEAF="PSIU601ARPIT/Q0AXCBR1$ST$Pos$stVal" DESC="断路器A相位置"></FCDA>   */
            sprintf(str,"%d",pFcda->index);
            mxmlElementSetAttr(node_dataset_fcda, "INDEX", str);
            sprintf(str,"%d",pFcda->ordinal);
            mxmlElementSetAttr(node_dataset_fcda, "ORDINAL", str);
            mxmlElementSetAttr(node_dataset_fcda, "TYPE", GetStrbyDataType(pFcda->type));
            mxmlElementSetAttr(node_dataset_fcda, "LEAF", "");
            if(pFcda->desc == NULL)
            {
                mxmlElementSetAttr(node_dataset_fcda, "DESC", "");
            }
            else
            {
                mxmlElementSetAttr(node_dataset_fcda, "DESC", pFcda->desc);
            }
        }

    }

    for(i = 0; i < g_ulGooseSubMapCnt; i++)
    {
        /*<FCDA DAINDEX="0x02000701" CPUID="1" DEVTYPE="2" ORDINAL="8 "
        TYPE="BOOL" DESC="闭锁重合闸-2@|K(10007)" LEAF=""></FCDA>*/
        if(i == 0)
        {
            pSubMapNode = g_tSubGoose.pSubMapRootNode;
        }
        else
        {
            pSubMapNode = pSubMapNode->next;
        }
        mxml_datamap_fcda = mxmlNewElement(mxml_datamap_subdataset, "FCDA");
        sprintf(str,"%08X",pSubMapNode->DaIndex);
        mxmlElementSetAttr(mxml_datamap_fcda, "DAINDEX", str);
        sprintf(str,"%d",pSubMapNode->cpuid);
        mxmlElementSetAttr(mxml_datamap_fcda, "CPUID", str);
        sprintf(str,"%d",pSubMapNode->dType);
        mxmlElementSetAttr(mxml_datamap_fcda, "DEVTYPE", str);
        sprintf(str,"%d",pSubMapNode->ordinal);
        mxmlElementSetAttr(mxml_datamap_fcda, "ORDINAL", str);
        mxmlElementSetAttr(mxml_datamap_fcda, "TYPE", GetStrbyDataType(pSubMapNode->type));
        mxmlElementSetAttr(mxml_datamap_fcda, "DESC", pSubMapNode->desc);
        mxmlElementSetAttr(mxml_datamap_fcda, "LEAF", "");
        sprintf(str,"%x",pSubMapNode->AIOIndex);
        mxmlElementSetAttr(mxml_datamap_fcda, "AIOINDEX", str);
    }

    for(i = 0; i < g_ulGoosePubMapCnt; i++)
    {
        /*<FCDA DAINDEX="0x02000701" CPUID="1" DEVTYPE="2" ORDINAL="8 "
        TYPE="BOOL" DESC="闭锁重合闸-2@|K(10007)" LEAF=""></FCDA>*/
        if(i == 0)
        {
            pPubMapNode = g_tPubGoose.pPubMapRootNode;
        }
        else
        {
            pPubMapNode = pPubMapNode->next;
        }
        mxml_datamap_fcda = mxmlNewElement(mxml_datamap_pubdataset, "FCDA");
        sprintf(str,"%08X",pPubMapNode->index);
        mxmlElementSetAttr(mxml_datamap_fcda, "DAINDEX", str);
        sprintf(str,"%d",pPubMapNode->cpuid);
        mxmlElementSetAttr(mxml_datamap_fcda, "CPUID", str);
        sprintf(str,"%d",pPubMapNode->dType);
        mxmlElementSetAttr(mxml_datamap_fcda, "DEVTYPE", str);
        sprintf(str,"%d",pPubMapNode->ordinal);
        mxmlElementSetAttr(mxml_datamap_fcda, "ORDINAL", str);
        mxmlElementSetAttr(mxml_datamap_fcda, "TYPE", GetStrbyDataType(pPubMapNode->type));
        if(pPubMapNode->linkLogicID == NULL)
        {
            mxmlElementSetAttr(mxml_datamap_fcda, "YabanID", "");
        }
        else
        {
            mxmlElementSetAttr(mxml_datamap_fcda, "YabanID", pPubMapNode->linkLogicID);
        }
        mxmlElementSetAttr(mxml_datamap_fcda, "DESC", "");
        mxmlElementSetAttr(mxml_datamap_fcda, "LEAF", "");
    }

    if(mxmlSaveFile(root_node, fp, whitespace_cb) != 0)
    {
        CFG_LOG("------> 生成GSE.XML文件出错\n",0,0,0,0,0,0);
        res = EDP_CC_GS_ERR_FILE_SAVE_FAIL;
        goto exit;
    }

    fclose(fp);
    mxmlDelete(root_node);

exit:
    return res;
}


/*
描述: 生成GSE.XML文件
参数:
iBoardId:板号
返回值: 解析是否成功
 */
int EDP_CreateCPUSmvFile()
{
    int res = 0;
    int i = 0;
    int j = 0;
    FILE *fp;
    mxml_node_t *root_node = NULL;
    mxml_node_t *mxml_config = NULL;
    mxml_node_t *mxml_smv91 = NULL;
    mxml_node_t *mxml_dataset = NULL;
    mxml_node_t *mxml_chnmap = NULL;
    char str[128];

    IEC_SMV_9_1_CFG *pSubInfoNode = NULL;

    if(g_pSmvCfg->smvNum == 0)
    {
        goto exit;
    }

    if(FT_Is_File(CONFIG_SMV_FILE))
    {
        remove(CONFIG_SMV_FILE);
    }

    fp = fopen(CONFIG_SMV_FILE, "w");

    if(fp==NULL)
    {
        res = EDP_CC_GS_ERR_FILE_CREATE_FAIL;
        goto exit;
    }

    root_node = mxmlNewXML("1.0");

    mxml_config = mxmlNewElement(root_node, "CONFIG");
    mxmlElementSetAttr(mxml_config, "VERSION", "0.1");
    mxml_smv91 = mxmlNewElement(mxml_config, "SMV_9_1");
    /*
    mxmlNewElement , mxmlNewInteger , mxmlNewOpaque ,
    mxmlNewReal , mxmlNewText mxmlNewTextf mxmlNewXML .
    */


    for(i = 0; i < g_pSmvCfg->smvNum; i++)
    {
        pSubInfoNode = &g_pSmvCfg->Smv_9_1Cfg[i];
        mxml_dataset = mxmlNewElement(mxml_smv91, "DataSet");

        sprintf(str,"%d",g_ucCpuId);
        mxmlElementSetAttr(mxml_dataset, "smvNodeAddr", str);
        sprintf(str,"%u",pSubInfoNode->smvPortChn);
        mxmlElementSetAttr(mxml_dataset, "smvPortChn", str);
        sprintf(str,"%02X-%02X-%02X-%02X-%02X-%02X",
                pSubInfoNode->smvSrc[0], pSubInfoNode->smvSrc[1], pSubInfoNode->smvSrc[2],
                pSubInfoNode->smvSrc[3], pSubInfoNode->smvSrc[4], pSubInfoNode->smvSrc[5]);
        mxmlElementSetAttr(mxml_dataset, "MUL_SRC", str);
        sprintf(str,"0x%04x",pSubInfoNode->appID);
        mxmlElementSetAttr(mxml_dataset, "APP_ID", str);
        sprintf(str,"%d",pSubInfoNode->receiveType);
        mxmlElementSetAttr(mxml_dataset, "TYPE", str);
        sprintf(str,"%d",pSubInfoNode->smprate9_2);
        mxmlElementSetAttr(mxml_dataset, "smprate", str);
        sprintf(str,"%d",pSubInfoNode->forceSyn);
        mxmlElementSetAttr(mxml_dataset, "forceSyn", str);
        sprintf(str,"%d",pSubInfoNode->asduNum);
        mxmlElementSetAttr(mxml_dataset, "asduNum", str);

        for(j = 0; j < pSubInfoNode->dataNum; j++)
        {
            mxml_chnmap = mxmlNewElement(mxml_dataset, "CHN_MAP");
            /*<FCDA INDEX="01" ORDINAL="01" TYPE="DBPOS" LEAF="PSIU601ARPIT/Q0AXCBR1$ST$Pos$stVal" DESC="断路器A相位置"></FCDA>   */
            sprintf(str,"%d",pSubInfoNode->smvData[j].smvAdsuNo);
            mxmlElementSetAttr(mxml_chnmap, "smvAdsuNo", str);
            sprintf(str,"%d",pSubInfoNode->smvData[j].smvAdsuChn);
            mxmlElementSetAttr(mxml_chnmap, "smvAdsuChn", str);
            sprintf(str,"%d",pSubInfoNode->smvData[j].smvDataChn);
            mxmlElementSetAttr(mxml_chnmap, "smvDataChn", str);
            sprintf(str,"%d",pSubInfoNode->smvData[j].smvValOut);
            mxmlElementSetAttr(mxml_chnmap, "smvValOut", str);
            mxmlElementSetAttr(mxml_chnmap, "YabanID", pSubInfoNode->smvData[j].MuYabanIDStr);
            sprintf(str,"%d",pSubInfoNode->smvData[j].smvChnType);
            mxmlElementSetAttr(mxml_chnmap, "smvChnType", str);
            if(pSubInfoNode->smvData[j].smvDes == NULL)
            {
                mxmlElementSetAttr(mxml_chnmap, "smvDes", "");
            }
            else
            {
                mxmlElementSetAttr(mxml_chnmap, "smvDes", pSubInfoNode->smvData[j].smvDes);
            }
        }
    }

    if(mxmlSaveFile(root_node, fp, whitespace_cb_sv) != 0)
    {
        CFG_LOG("------> 生成GSE.XML文件出错\n",0,0,0,0,0,0);
        res = EDP_CC_GS_ERR_FILE_SAVE_FAIL;
        goto exit;
    }

    fclose(fp);
    mxmlDelete(root_node);

exit:
    return res;
}

/*
描述: 生成CPU侧配置文件
参数:     NONE
返回值: 解析是否成功
 */
int EDP_CreateCPUFile()
{
    int res = 0;

    if((res = EDP_CreateCPUSmvFile()) != 0)
    {
        CFG_LOG("---->生成SMV.XML文件失败,%08x\n",res,0,0,0,0,0);
    }
    else
    {
        CFG_LOG("---->生成SMV.XML文件成功\n",0,0,0,0,0,0);
    }

    if((res = EDP_CreateCPUGseFile()) != 0)
    {
        CFG_LOG("---->生成GSE.XML文件失败,%08x\n",res,0,0,0,0,0);
    }
    else
    {
        CFG_LOG("---->生成GSE.XML文件成功\n",0,0,0,0,0,0);
    }

    res = 0;
    return res;
}

int stripspace(char *strline,char *strcontext,int icount)
{
    int yinhao = 0;
    int shuminghao = 0;
    int i = 0;

    while(strline[i] != 10 && strline[i] != 13)
    {
        if (strline[i] == '\0' || strline[i] == '\n')
        {
            break;
        }
        if ((strline[i] == '<') || (strline[i] == '>'))
        {
            shuminghao++;
            strcontext[icount++] = strline[i];
        }
        else if (strline[i] == '"')
        {
            yinhao++;
            strcontext[icount++] = strline[i];
        }
        else if (strline[i] == ' ')
        {
            if (((shuminghao%2 == 0)&&(shuminghao!=0))||(yinhao%2 == 1) )
            {
                strcontext[icount++] = strline[i];
            }
        }
        else if(strline[i] == 9)
        {
            ;
        }
        else
        {
            strcontext[icount++] = strline[i];
        }
        i++;
    }
    return icount;
}

/*
描述: 生成用于计算CRC的文件
参数: pNode, CCD文件的XML指针.
      pFileName, 文件的路径名.
返回值: 解析是否成功
 */
int EDP_CreatCrcFile(mxml_node_t *pNode, char* pFileName)
{
    FILE *fp1 = NULL;
    FILE *fp2 = NULL;
    mxml_node_t *node1 = NULL;
    mxml_node_t *node2 = NULL;
    mxml_node_t *node3 = NULL;
    mxml_node_t *node4 = NULL;
    mxml_node_t *node5 = NULL;
    int res = 0;
    char buf1[1024];
    char buf2[1024];
    char ucBakFileName[256];
    int iCount = 0;

    sprintf(ucBakFileName, "%s.xml", pFileName);
    fp1 = fopen(ucBakFileName, "w+");
    fp2 = fopen(pFileName, "w");
    if((fp1 == NULL)||(fp2==NULL))
    {
        res = EDP_CRC_FILE_CREAT_ERROR;
        goto exit;
    }

    node1 = mxmlFindElement(pNode,pNode, "IED",NULL, NULL,MXML_DESCEND);
    mxmlElementDeleteAttr(node1, "configVersion");
    mxmlElementDeleteAttr(node1, "desc");
    mxmlElementDeleteAttr(node1, "manufacturer");
    mxmlElementDeleteAttr(node1, "type");

    node1 = mxmlFindElement(pNode,pNode, "GOOSEPUB",NULL, NULL,MXML_DESCEND);
    node2 = mxmlFindElement(node1, node1, "GOCBref",NULL, NULL,MXML_DESCEND);
    while(node2 != NULL)
    {
        node3 = mxmlFindElement(node2, node2, "DataSet",NULL, NULL,MXML_DESCEND);
        node4 = mxmlFindElement(node3, node3, "FCDA",NULL, NULL,MXML_DESCEND);
        while(node4 != NULL)
        {
            mxmlElementDeleteAttr(node4, "desc");
            node4 = mxmlFindElement(node4, node3, "FCDA",NULL, NULL,MXML_DESCEND);
        }
        node2 = mxmlFindElement(node2, node1, "GOCBref",NULL, NULL,MXML_DESCEND);
    }

    node1 = mxmlFindElement(pNode,pNode, "GOOSESUB",NULL, NULL,MXML_DESCEND);
    node2 = mxmlFindElement(node1, node1, "GOCBref",NULL, NULL,MXML_DESCEND);
    while(node2 != NULL)
    {
        node3 = mxmlFindElement(node2, node2, "DataSet",NULL, NULL,MXML_DESCEND);
        node4 = mxmlFindElement(node3, node3, "FCDA",NULL, NULL,MXML_DESCEND);
        while(node4 != NULL)
        {
            mxmlElementDeleteAttr(node4, "daName");
            mxmlElementDeleteAttr(node4, "desc");
            mxmlElementDeleteAttr(node4, "doName");
            mxmlElementDeleteAttr(node4, "fc");
            mxmlElementDeleteAttr(node4, "ldInst");
            mxmlElementDeleteAttr(node4, "lnClass");
            mxmlElementDeleteAttr(node4, "lnInst");
            mxmlElementDeleteAttr(node4, "prefix");
            node5 = mxmlFindElement(node4, node4, "intAddr",NULL, NULL,MXML_DESCEND);
            while(node5 != NULL)
            {
                mxmlElementDeleteAttr(node5, "desc");
                node5 = mxmlFindElement(node5, node4, "intAddr",NULL, NULL,MXML_DESCEND);
            }
            node4 = mxmlFindElement(node4, node3, "FCDA",NULL, NULL,MXML_DESCEND);
        }
        node2 = mxmlFindElement(node2, node1, "GOCBref",NULL, NULL,MXML_DESCEND);
    }

    node1 = mxmlFindElement(pNode,pNode, "SVPUB",NULL, NULL,MXML_DESCEND);
    node2 = mxmlFindElement(node1, node1, "SMVCBref",NULL, NULL,MXML_DESCEND);
    while(node2 != NULL)
    {
        node3 = mxmlFindElement(node2, node2, "DataSet",NULL, NULL,MXML_DESCEND);
        node4 = mxmlFindElement(node3, node3, "FCDA",NULL, NULL,MXML_DESCEND);
        while(node4 != NULL)
        {
            mxmlElementDeleteAttr(node4, "desc");
            node4 = mxmlFindElement(node4, node3, "FCDA",NULL, NULL,MXML_DESCEND);
        }
        node2 = mxmlFindElement(node2, node1, "SMVCBref",NULL, NULL,MXML_DESCEND);
    }

    node1 = mxmlFindElement(pNode,pNode, "SVSUB",NULL, NULL,MXML_DESCEND);
    node2 = mxmlFindElement(node1, node1, "SMVCBref",NULL, NULL,MXML_DESCEND);
    while(node2 != NULL)
    {
        node3 = mxmlFindElement(node2, node2, "DataSet",NULL, NULL,MXML_DESCEND);
        node4 = mxmlFindElement(node3, node3, "FCDA",NULL, NULL,MXML_DESCEND);
        while(node4 != NULL)
        {
            mxmlElementDeleteAttr(node4, "daName");
            mxmlElementDeleteAttr(node4, "desc");
            mxmlElementDeleteAttr(node4, "doName");
            mxmlElementDeleteAttr(node4, "fc");
            mxmlElementDeleteAttr(node4, "ldInst");
            mxmlElementDeleteAttr(node4, "lnClass");
            mxmlElementDeleteAttr(node4, "lnInst");
            mxmlElementDeleteAttr(node4, "prefix");
            node5 = mxmlFindElement(node4, node4, "intAddr",NULL, NULL,MXML_DESCEND);
            while(node5 != NULL)
            {
                mxmlElementDeleteAttr(node5, "desc");
                node5 = mxmlFindElement(node5, node4, "intAddr",NULL, NULL,MXML_DESCEND);
            }
            node4 = mxmlFindElement(node4, node3, "FCDA",NULL, NULL,MXML_DESCEND);
        }
        node2 = mxmlFindElement(node2, node1, "SMVCBref",NULL, NULL,MXML_DESCEND);
    }

    node1 = mxmlFindElement(pNode,pNode, "CRC",NULL, NULL,MXML_DESCEND);
    mxmlDelete(node1);

    if(mxmlSaveFile(pNode, fp1, whitespace_cb_crc) != 0)
    {
        res = EDP_CRC_BAK_FILE_SAVE_ERROR;
        goto exit;
    }

    fseek(fp1, 0, SEEK_SET);
    while(fgets(buf1, 1024, fp1) != NULL)
    {
        iCount = stripspace(buf1, buf2, 0);
        fwrite(buf2, iCount, 1, fp2);
    }

exit:

    if(fp1 != NULL)
    {
        fclose(fp1);
    }

    if(fp2 != NULL)
    {
        fclose(fp2);
    }

    if(FT_Is_File(ucBakFileName))
    {
        //   remove(ucBakFileName);
    }

    return res;
}


/*
描述: 初始化张全的通讯结构
参数:     NONE
返回值: 解析是否成功
 */
int EDP_InitCcStrToSendFile()
{
    int i = 0;
    int index1 = 0;
    int index2 = 0;
    CC_TRANSMIT_STR *pCcTransmitStr = NULL;

    for(i = 0; i < g_tCCTransmitInfo.ucCCCnt; i++)
    {
        pCcTransmitStr = &(g_tCCTransmitInfo.pCcTransmitStr[i]);
        index1 = pCcTransmitStr->ucSrcPort[0];
        index2 = pCcTransmitStr->ucSrcPort[1];
        sFpgaCCInfo[index1][index2].nPort = index2;
        memcpy(sFpgaCCInfo[index1][index2].sCfgFileName, pCcTransmitStr->pZipFilePath, 256);
        sFpgaCCInfo[index1][index2].bCfgFlag = TRUE;

        if(g_bCcIsUsed[pCcTransmitStr->iBoardId] || pCcTransmitStr->bIsMasterCC)
        {
            g_tFpgaCcCfgInfo[pCcTransmitStr->iBoardId].iDataType = pCcTransmitStr->iDataType;
            if(PORT_TRANSFOR_DATA_TYPE_SV == pCcTransmitStr->iDataType)
            {
                g_tFpgaCcCfgInfo[pCcTransmitStr->iBoardId].usFpgaCcSvCfgCrc = FT_File_CRC16(pCcTransmitStr->pSvFilePath);
            }
            else if(PORT_TRANSFOR_DATA_TYPE_GS == pCcTransmitStr->iDataType)
            {
                g_tFpgaCcCfgInfo[pCcTransmitStr->iBoardId].usFpgaCcGsCfgCrc  = FT_File_CRC16(pCcTransmitStr->pGsFilePath);
            }
            else
            {
                g_tFpgaCcCfgInfo[pCcTransmitStr->iBoardId].usFpgaCcSvCfgCrc = FT_File_CRC16(pCcTransmitStr->pSvFilePath);
                g_tFpgaCcCfgInfo[pCcTransmitStr->iBoardId].usFpgaCcGsCfgCrc  = FT_File_CRC16(pCcTransmitStr->pGsFilePath);
            }
            sFpgaCCInfo[index1][index2].bGsSendOnly = FALSE;
            sFpgaCCInfo[index1][index2].bCfgFlag = TRUE;
            g_bCfgChgFlag[index1] = TRUE;
        }
        else if(pCcTransmitStr->iDataType == PORT_TRANSFOR_DATA_TYPE_GS)
        {
            sFpgaCCInfo[index1][index2].bGsSendOnly = TRUE;
            sFpgaCCInfo[index1][index2].bCfgFlag = TRUE;
            g_bCfgChgFlag[index1] = TRUE;
        }
        else
        {
            sFpgaCCInfo[index1][index2].bCfgFlag = FALSE;
        }
    }

    return 0;
}


/*
描述: 释放Gse文件操作的内存
参数:     NONE
返回值:  释放是否成功
 */
int EDP_FreeGseFile()
{
    EP_STATUS res = EP_SUCCESS;

    /*free(g_tSubGoose.pSubInfoRootNode);
    g_tSubGoose.pSubInfoRootNode = NULL;
    free(g_tSubGoose.pSubMapRootNode);
    g_tSubGoose.pSubMapRootNode = NULL;
    free(g_tPubGoose.pPubInfoRootNode);
    g_tPubGoose.pPubInfoRootNode = NULL;
    free(g_tPubGoose.pPubMapRootNode);
    g_tPubGoose.pPubMapRootNode = NULL;*/

    return res;
}

/*
描述: 释放Gs文件操作的内存
参数:     NONE
返回值:  释放是否成功
 */
int EDP_FreeGsFile()
{
    EP_STATUS res = EP_SUCCESS;
    free(g_tCCGsPool.pGsSubInfo);
    g_tCCGsPool.pGsSubInfo = NULL;
    //free(g_tCCGsPool.pGsTransInfo->pDstPortList);
    //free(g_tCCGsPool.pGsTransInfo->pSrcPortList);
    free(g_tCCGsPool.pGsTransInfo);
    g_tCCGsPool.pGsTransInfo = NULL;

    /*free(g_tCCTransmitInfo.pCcTransmitStr);
    g_tCCTransmitInfo.pCcTransmitStr = NULL;*/
    return res;
}

/*
描述: 释放Smv文件操作的内存
参数:     NONE
返回值:  释放是否成功
 */
int EDP_FreeSmvFile()
{
    EP_STATUS res = EP_SUCCESS;

    free(g_pSmvCfg);
    g_pSmvCfg = NULL;

    return res;
}
/*
描述: 释放Sv文件操作的内存
参数:     NONE
返回值:  释放是否成功
 */
int EDP_FreeSvFile()
{
    int i,j,k;
    EP_STATUS res = EP_SUCCESS;

    for(i = 0; i < g_tCCSvPool.iSvBoardCnt-1; i++)
    {
        for(j = 0; j < g_tCCSvPool.pSvSlaverCcTransInfo[i].iPubNum; j++)
        {
            for(k = 0; k < g_tCCSvPool.pSvSlaverCcTransInfo[i].pCcSvPubInfo[j].iAsduNum; k++)
            {
                free(g_tCCSvPool.pSvSlaverCcTransInfo[i].pCcSvPubInfo[j].pCcSvPubAsduInfo[k].pSvID);
                g_tCCSvPool.pSvSlaverCcTransInfo[i].pCcSvPubInfo[j].pCcSvPubAsduInfo[k].pSvID = NULL;
            }
            free(g_tCCSvPool.pSvSlaverCcTransInfo[i].pCcSvPubInfo[j].pCcSvPubAsduInfo);
            g_tCCSvPool.pSvSlaverCcTransInfo[i].pCcSvPubInfo[j].pCcSvPubAsduInfo = NULL;
        }
        free(g_tCCSvPool.pSvSlaverCcTransInfo[i].pCcSvPubInfo);
        g_tCCSvPool.pSvSlaverCcTransInfo[i].pCcSvPubInfo = NULL;
        for(j = 0; j < g_tCCSvPool.pSvSlaverCcTransInfo[i].iSubNum; j++)
        {
            for(k = 0; k < g_tCCSvPool.pSvSlaverCcTransInfo[i].pCcSvSubInfo[j].iAsduNum; k++)
            {
                free(g_tCCSvPool.pSvSlaverCcTransInfo[i].pCcSvSubInfo[j].pCcSvSubAsduInfo[k].pSvID);
                g_tCCSvPool.pSvSlaverCcTransInfo[i].pCcSvSubInfo[j].pCcSvSubAsduInfo[k].pSvID = NULL;
            }
            free(g_tCCSvPool.pSvSlaverCcTransInfo[i].pCcSvSubInfo[j].pCcSvSubAsduInfo);
            g_tCCSvPool.pSvSlaverCcTransInfo[i].pCcSvSubInfo[j].pCcSvSubAsduInfo = NULL;
        }
        free(g_tCCSvPool.pSvSlaverCcTransInfo[i].pCcSvSubInfo);
        g_tCCSvPool.pSvSlaverCcTransInfo[i].pCcSvSubInfo = NULL;
    }
    free(g_tCCSvPool.pSvSlaverCcTransInfo);
    g_tCCSvPool.pSvSlaverCcTransInfo = NULL;

    for(i = 0; i < g_tCCSvPool.tSvMasterCcTransInfo.iPubNum; i++)
    {
        for(j = 0; j < g_tCCSvPool.tSvMasterCcTransInfo.pCcSvPubInfo[i].iAsduNum; j++)
        {
            free(g_tCCSvPool.tSvMasterCcTransInfo.pCcSvPubInfo[i].pCcSvPubAsduInfo[j].pSvID);
            g_tCCSvPool.tSvMasterCcTransInfo.pCcSvPubInfo[i].pCcSvPubAsduInfo[j].pSvID = NULL;
        }
        free(g_tCCSvPool.tSvMasterCcTransInfo.pCcSvPubInfo[i].pCcSvPubAsduInfo);
        g_tCCSvPool.tSvMasterCcTransInfo.pCcSvPubInfo[i].pCcSvPubAsduInfo = NULL;
    }
    free(g_tCCSvPool.tSvMasterCcTransInfo.pCcSvPubInfo);
    g_tCCSvPool.tSvMasterCcTransInfo.pCcSvPubInfo = NULL;

    for(i = 0; i < g_tCCSvPool.tSvMasterCcTransInfo.iSubNum; i++)
    {
        for(j = 0; j < g_tCCSvPool.tSvMasterCcTransInfo.pCcSvSubInfo[i].iAsduNum; j++)
        {
            free(g_tCCSvPool.tSvMasterCcTransInfo.pCcSvSubInfo[i].pCcSvSubAsduInfo[j].pSvID);
            g_tCCSvPool.tSvMasterCcTransInfo.pCcSvSubInfo[i].pCcSvSubAsduInfo[j].pSvID = NULL;
        }
        free(g_tCCSvPool.tSvMasterCcTransInfo.pCcSvSubInfo[i].pCcSvSubAsduInfo);
        g_tCCSvPool.tSvMasterCcTransInfo.pCcSvSubInfo[i].pCcSvSubAsduInfo = NULL;
    }
    free(g_tCCSvPool.tSvMasterCcTransInfo.pCcSvSubInfo);
    g_tCCSvPool.tSvMasterCcTransInfo.pCcSvSubInfo = NULL;

    return res;
}

/*
描述: 释放文件操作的内存
参数:     NONE
返回值:  释放是否成功
 */
int EDP_FreeFile()
{
    EP_STATUS res = EP_SUCCESS;

    if(EDP_FreeGseFile() != EP_SUCCESS)
    {
        logMsg("#######  EDP_FreeGseFile执行出错\n",0,0,0,0,0,0);
        res = EP_ERROR;
    }
    if(EDP_FreeGsFile() != EP_SUCCESS)
    {
        logMsg("#######  EDP_FreeGsFile执行出错\n",0,0,0,0,0,0);
        res = EP_ERROR;
    }

    if(EDP_FreeSmvFile() != EP_SUCCESS)
    {
        logMsg("#######  EDP_FreeSmvFile执行出错\n",0,0,0,0,0,0);
        res = EP_ERROR;
    }
    if(EDP_FreeSvFile() != EP_SUCCESS)
    {
        logMsg("#######  EDP_FreeSvFile执行出错\n",0,0,0,0,0,0);
        res = EP_ERROR;
    }

    return res;
}

/*
打印CC板转发信息
*/
void EDP_ShowTransmitInfo()
{
    int i = 0;
    int cnt = 0;
    CC_TRANSMIT_STR *pCcTransmitStr = NULL;

    cnt =  g_tCCTransmitInfo.ucCCCnt;
    for(i = 0; i < cnt; i++)
    {
        pCcTransmitStr = &(g_tCCTransmitInfo.pCcTransmitStr[i]);
        logMsg("#####  index:%d, 板号:%d, 转发类型:%d, 是否主CC:%d, 源端口0: %d, 源端口1: %d\n",
               i,
               pCcTransmitStr->iBoardId,
               pCcTransmitStr->iDataType,
               pCcTransmitStr->bIsMasterCC,
               pCcTransmitStr->ucSrcPort[0],
               pCcTransmitStr->ucSrcPort[1]);
        logMsg("\t GsFile:%s, SvFile:%s, ZipFile:%s\n",
               (int)pCcTransmitStr->pGsFilePath,
               (int)pCcTransmitStr->pSvFilePath,
               (int)pCcTransmitStr->pZipFilePath,0,0,0);
    }
}

/*
描述: 测试生成XML文件
参数:     NONE
返回值: 解析是否成功
 */
int testcc()
{
    int res = 0;
    int i = 0;
    FILE *fp;
    mxml_node_t *root_node = NULL;
    mxml_node_t *mxml_config = NULL;
    mxml_node_t *mxml_datainfo = NULL;
    mxml_node_t *mxml_datainfo_subgoose = NULL;
    mxml_node_t *mxml_datainfo_pubgoose = NULL;
    mxml_node_t *mxml_datamap = NULL;
    mxml_node_t *mxml_datamap_subdataset = NULL;
    mxml_node_t *mxml_datamap_pubdataset = NULL;
    mxml_node_t *node_gcb = NULL;
    mxml_node_t *node_net = NULL;
    mxml_node_t *node_address = NULL;
    mxml_node_t *node_address_port = NULL;
    mxml_node_t *node_address_dstmac = NULL;
    mxml_node_t *node_address_dstmac_port = NULL;

    char pFileName[128];
    char str[16];


    if(!pFileName)
    {
        res = EDP_CCD_PARSE_ERR_FILE_NOT_EXIST;
        goto exit;
    }

    memcpy(pFileName, "/tffs/gs1.xml", sizeof("/tffs/gs1.xml"));

    fp = fopen(pFileName, "w+");
    if(fp==NULL)
    {
        res = EDP_CC_GS_ERR_FILE_CREATE_FAIL;
        goto exit;
    }

    root_node = mxmlNewXML("1.0");

    mxml_config = mxmlNewElement(root_node, "Config");
    mxml_datainfo = mxmlNewElement(mxml_config, "DATAINFO");
    mxml_datamap = mxmlNewElement(mxml_config, "DATAMAP");
    mxml_datainfo_subgoose = mxmlNewElement(mxml_datainfo, "SUB_GOOSE");
    mxml_datainfo_pubgoose = mxmlNewElement(mxml_datainfo, "PUB_GOOSE");
    mxml_datamap_subdataset = mxmlNewElement(mxml_datamap, "SUB_DATASET");
    mxml_datamap_pubdataset = mxmlNewElement(mxml_datamap, "PUB_DATASET");

    /*
    mxmlNewElement , mxmlNewInteger , mxmlNewOpaque ,
    mxmlNewReal , mxmlNewText mxmlNewTextf mxmlNewXML .
    */
    for(i = 0; i < 2; i++)
    {
        node_gcb = mxmlNewElement(mxml_datainfo_subgoose, "GCB");

        mxmlElementSetAttr(node_gcb, "IEDName", "PSL641");
        mxmlElementSetAttr(node_gcb, "GcRef", "111");
        mxmlElementSetAttr(node_gcb, "DatasetRef", "222");
        mxmlElementSetAttr(node_gcb, "GoID", "333");
        sprintf(str,"%d",1);
        mxmlElementSetAttr(node_gcb, "ConfRev", str);
        sprintf(str,"%04x",0x1234);
        mxmlElementSetAttr(node_gcb, "AppID", str);

        mxmlElementSetAttr(node_gcb, "DESC", "");
        mxmlElementSetAttr(node_gcb, "YabanID", "");
        mxmlElementSetAttr(node_gcb, "AdmYabanID", "");
        mxmlElementSetAttr(node_gcb, "Gmrp", "0");
        mxmlElementSetAttr(node_gcb, "T0", (char *)"5000");
        mxmlElementSetAttr(node_gcb, "T1", (char *)"2");
        mxmlElementSetAttr(node_gcb, "Priority", (char *)"6");


        node_net = mxmlNewElement(node_gcb, "NET");
        /*<NET Type="ALONE" Num="1">*/
        mxmlElementSetAttr(node_net, "Type", "ALONE");
        mxmlElementSetAttr(node_net, "Num", "1");

        node_address = mxmlNewElement(node_net, "Address");
        node_address_port = mxmlNewElement(node_address, "PORT");
        mxmlNewText(node_address_port, 0, "1");
        node_address_dstmac = mxmlNewElement(node_address, "DST_MAC");
        mxmlNewText(node_address_dstmac, 0, "01-02-03-04-05-06");

        node_address_dstmac = mxmlNewElement(node_net, "DST_Address");
        node_address_dstmac_port = mxmlNewElement(node_address_dstmac, "PORT");
        mxmlNewText(node_address_dstmac_port, 0, "1");
    }

    if(mxmlSaveFile(root_node, fp, whitespace_cb) != 0)
    {
        CFG_LOG("------> 生成XML文件出错\n",0,0,0,0,0,0);
        res = EDP_CC_GS_ERR_FILE_SAVE_FAIL;
        goto exit;
    }

    fclose(fp);

    mxmlDelete(root_node);
exit:
    return res;
}

int EDP_GetEncoding(void *p)
{
    int encoding;
    FILE *fp;
    char ch;
    char str[51];
    int i = 0;

#define UTF_8_SMALL1  "utf-8"
#define UTF_8_BIG1  "UTF-8"
#define GB2312_8_SMALL1  "gb2312"
#define GB2312_8_BIG1  "GB2312"

    encoding   = ENCODE_UNKNOWN;/*ENCODE_UTF8;*/

    if(p == NULL)
    {
        return encoding;
    }

    fp = p;

    for(i = 0; i < 50; i++)
    {
        ch = fgetc(fp);
        str[i] = ch;
    }
    str[50] = '\0';

    if((strstr(str, UTF_8_SMALL1) != NULL) || (strstr(str, UTF_8_BIG1) != NULL))
    {
        CFG_LOG("#########   文件是UTF-8编码\n",0,0,0,0,0,0);
        encoding   = ENCODE_UTF8;
    }
    else if((strstr(str, GB2312_8_SMALL1) != NULL) || (strstr(str, GB2312_8_BIG1) != NULL))
    {
        CFG_LOG("#########   文件是GB2312编码\n",0,0,0,0,0,0);
        encoding   = ENCODE_GB2312;
    }

    fclose(fp);
    return encoding;
}

BOOL testencoding()
{
    int encoding;
    FILE *fp;

    fp = fopen(CCD_FILE, "r");

    encoding = EDP_GetEncoding(fp);
    return TRUE;
}

void ZipTest()
{
    FILE* zipfd;
    char filepath[2][128];
    char filename[2][128];

    strcpy((char*)filepath[0], "/tffs/sv.xml");
    strcpy((char*)filepath[1], "/tffs/gs.xml");
    strcpy((char*)filename[0], "sv.xml");
    strcpy((char*)filename[1], "gs.xml");
    zipfd = fopen("/tffs/sv_gs_cfg.zip", "w+");

    ProcessCfgZip(2,filepath,filename,zipfd);

    fclose(zipfd);

}

