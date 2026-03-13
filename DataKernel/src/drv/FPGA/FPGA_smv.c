/* FPGA_smv.c - subroutine library for sampling from FPGA */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 23dec11, dy first created.
*/

/*
DESCRIPTION
This module includes subroutine library for sampling from FPGA.
*/

/* includes */

#include <stdio_compat.h>
#include "string_compat.h"
#include "mxml.h"
#include "smvcfg.h"
#include "filetool.h"
#include "edp_asst.h"
#include "swcfg.h"
#include "smvcfg.h"
#include "smv_rx.h"
#include "realdata.h"
// #include "target.h"
#include "bspinterface.h"
#include "adc.h"
#include "FPGA_Interface.h"
#include "Smv_Go_CommStat_File.h"
#include "intLib.h"

/* defines */

#define CHG_MAX_REC_NUM 10  /* 变化记录最多次数 */

/* globals */

XML_CFG_INFO sXmlCfg;
FPGA_CFG_INFO sFpgaCfg;
SMV_DATA smvData;
FPGA_SIMUL sFpgaSimul; /* 仿真数据 */
TIME_SYC_MODE_STRUCT TimeSycModeCfg = {0};
INTER_PULSE_MODE_STRUCT InterPulseModeCfg = {0};
BOOL bNeedtoStartFPGA = FALSE;
extern BOOL bPlatformCfgFPGA; /* 平台是否配置FPGA标志 */

uint32_t FPGASmvCommStat[2+FT3_PORT_NUM+SMV_RCV_PORT_NUM];/*AD+AD复采+FT3+92*/

extern UINT16 poIec_index; /* 查询计数 */

/* static functions */

/* FPGA数据有效性状态获取.
 * Para:
 *     pSts, 状态存储地址，输入保证地址有效性.
 * Return:
 *     NONE.
 */
static void fpgaGetSts(uint32_t *pSts);

/* 解析配置文件.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, FALSE.
 */
BOOL cfgFileParse(void)
{
    FILE *fp;
    mxml_node_t *rootnode;
    BOOL ret = FALSE;

    fp = fopen(E02_SG_CONFIG_FILE, "r");
    if (fp == NULL)
    {
        LOG_Dbg_Msg("配置文件'%s'不存在!\n", (int)E02_SG_CONFIG_FILE, 0, 0, 0, 0, 0);

        return FALSE;
    }
    rootnode = mxmlLoadFile(NULL, fp, NULL);
    fclose(fp);

    if (!rootnode)
    {
        return FALSE;
    }

    ret |= cfgParseSmvSubInfo(rootnode);
    ret |= cfgParseSmvPubInfo(rootnode);

    mxmlDelete(rootnode);

    return ret;
}


/* 解析SUB配置文件.
 * Para:
 *     xml句柄.
 * Return:
 *     TRUE, FALSE.
 */
BOOL cfgParseSmvSubInfo(mxml_node_t *rootnode)
{
    char *p = NULL;
    char *q = NULL;
    char *s = NULL;
    int i, j, k;
    mxml_node_t	*node1 = NULL;
    mxml_node_t	*node2 = NULL;
    mxml_node_t	*node3 = NULL;
    mxml_node_t	*node4 = NULL;
    mxml_node_t	*node5 = NULL;
    int16_t	iYabanNum;

    node1 = mxmlFindElement(rootnode, rootnode, "SMV", NULL, NULL, MXML_DESCEND);
    if (!node1)
        return FALSE;
    p = (char *)mxmlElementGetAttr(node1, "type");
    if (p)
    {
        if (strcmp(p, "640U") == 0)
        {
            sXmlCfg.smvCfg.muMode = E03_MODE_640U;
        }
        else if (strcmp(p, "智能终端") == 0)
        {
            sXmlCfg.smvCfg.muMode = E03_MODE_INTEL_TERM;
        }
        else if (strcmp(p, "600U") == 0)
        {
            sXmlCfg.smvCfg.muMode = MODE_600U;
        }
        else
        {
            LOG_Dbg_Msg("未知配置类型 %s\n", (int)p, 2, 3, 4, 5, 6);
            return FALSE;
        }
    }

    /* 暂不使用 */
    p = (char *)mxmlElementGetAttr(node1, "fpgaMode");
    if (p)
    {
        sXmlCfg.smvCfg.fpgaMode = strtoul(p, NULL, 10);
    }

    /* 对时方式
     */
    p = (char *)mxmlElementGetAttr(node1, "synMode");
    if (p)
    {
        sXmlCfg.smvCfg.synMode = strtoul(p, NULL, 10);
    }

    /* 秒脉冲电平处理 */
    p = (char *)mxmlElementGetAttr(node1, "synRev");
    if (p)
    {
        if (strcmp(p, "取反") == 0)
            sXmlCfg.smvCfg.synRev = SYN_REV_REVERSE;
        else if (strcmp(p, "正常") == 0)
            sXmlCfg.smvCfg.synRev = SYN_REV_NORMAL;
        else
        {
            LOG_Dbg_Msg("未知脉冲方式 %s\n", (int)p, 2, 3, 4, 5, 6);
            return FALSE;
        }
    }

    /* 秒脉冲边沿处理 */
    p = (char *)mxmlElementGetAttr(node1, "synEdge");
    if (p)
    {
        if (strcmp(p, "上升沿") == 0)
            sXmlCfg.smvCfg.synEdge = SYN_EDGE_RISE;
        else if (strcmp(p, "下降沿") == 0)
            sXmlCfg.smvCfg.synEdge = SYN_EDGE_DROP;
        else
        {
            LOG_Dbg_Msg("未知触发方式 %s\n", (int)p, 2, 3, 4, 5, 6);
            return FALSE;
        }
    }

    /* 守时 */
    p = (char *)mxmlElementGetAttr(node1, "synDeal");
    if (p)
    {
        if (strcmp(p, "不守时") == 0)
            sXmlCfg.smvCfg.synDeal = SYN_DEAL_NOR;
        else if (strcmp(p, "守时") == 0)
            sXmlCfg.smvCfg.synDeal = SYN_DEAL_FOL;
        else
        {
            LOG_Dbg_Msg("未知synDeal %s\n",(int)p,2,3,4,5,6);
            return FALSE;
        }
    }

    /* 插值脉冲类型 */
    p = (char *)mxmlElementGetAttr(node1, "intelPulse");
    if (p)
    {
        sXmlCfg.smvCfg.intelPulse = strtoul(p, NULL, 10);
    }

    /* 秒脉冲电平处理 */
    p = (char *)mxmlElementGetAttr(node1, "intelRev");
    if (p)
    {
        if (strcmp(p, "取反") == 0)
            sXmlCfg.smvCfg.intelRev = SYN_REV_REVERSE;
        else if (strcmp(p, "正常") == 0)
            sXmlCfg.smvCfg.intelRev = SYN_REV_NORMAL;
        else
        {
            LOG_Dbg_Msg("未知脉冲方式 %s\n", (int)p, 2, 3, 4, 5, 6);
            return FALSE;
        }
    }

    /* 强制同步 */
    p = (char *)mxmlElementGetAttr(node1, "SYN");
    if (p)
    {
        sXmlCfg.smvCfg.syn = strtoul(p, NULL, 10);
    }

    /* 强制测试 */
    p = (char *)mxmlElementGetAttr(node1, "TEST");
    if (p)
    {
        sXmlCfg.smvCfg.test = strtoul(p, NULL, 10);
    }

    /* 目前支持交流采样/FT3
     */
    sXmlCfg.smvCfg.subNum = 0;

    i = 0;

    for (node2 = mxmlFindElement(node1, node1, "SUB_SMV",
                                 NULL, NULL,MXML_DESCEND);
            node2 != NULL;
            node2 = mxmlFindElement(node2, node1, "SUB_SMV",
                                    NULL, NULL, MXML_DESCEND))
    {
        p = (char *)mxmlElementGetAttr(node2, "type");

        if ((strcmp(p, "IEC_92LE") == 0)
                || (strcmp(p, "IEC_92") == 0))
        {
            sXmlCfg.smvCfg.smvSub[i].type = DATA_SRC_SMV;

            p = (char *)mxmlElementGetAttr(node2, "PHASE");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].phase = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node2, "DLYCHN");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].dTChn = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node2, "DLYVLD");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].dTValid = strtoul(p, NULL, 10);
            }

            for (node4 = mxmlFindElement(node2, node2, "Address",
                                         NULL, NULL, MXML_DESCEND);
                    node4 != NULL;
                    node4 = mxmlFindElement(node4, node2, "Address",
                                            NULL, NULL, MXML_DESCEND))
            {
                for (node5 = mxmlFindElement(node4, node4, "P",
                                             NULL, NULL, MXML_DESCEND);
                        node5 != NULL;
                        node5 = mxmlFindElement(node5, node4, "P",
                                                NULL, NULL, MXML_DESCEND))
                {
                    p = (char *)mxmlElementGetAttr(node5, "type");
                    q = (char *)mxmlElementGetValue(node5);
                    if (!p || !q)
                    {
                        return FALSE;
                    }

                    if (strcmp(p, "PORT") == 0)
                    {
                        /* 9-2网络端口号加2,这样保证CPU板端口号引用连续,
                         * CPU板网口编号:0/1;FPGA网口编号:2/3/4/5/6/7
                         * FT3附板：0/1(接收);2/3/4(发送)
                         * 内部处理上A/D、FT3以及9-2采样排列连续
                         */
                        sXmlCfg.smvCfg.smvSub[i].port = strtoul(q, NULL, 10)+2;

                    }
                    else if (strcmp(p, "MAC-Address") == 0)
                    {
                        for (k = 0; k<6; k++)
                        {
                            s = strtok(q, "-");
                            sXmlCfg.smvCfg.smvSub[i].macAdr[k] = strtol(s, NULL, 16);
                            q = NULL;
                        }
                    }
                    else if(strcmp(p, "APPID") == 0)
                    {
                        sXmlCfg.smvCfg.smvSub[i].appID = strtoul(q,NULL,16);

                    }
                }
            }
        }
        else if ((strcmp(p, "标准FT3") == 0)
                 || (strcmp(p, "国网FT3") == 0)
                )
        {
            if (strcmp(p, "标准FT3") == 0)
            {
                sXmlCfg.smvCfg.smvSub[i].type = DATA_SRC_STDFT3;
            }
            else if (strcmp(p, "国网FT3") == 0)
            {
                sXmlCfg.smvCfg.smvSub[i].type = DATA_SRC_GRDFT3;
            }
            else
            {
                LOG_Dbg_Msg("未定义SUB类型\n", 1, 2, 3, 4, 5, 6);
                return FALSE;
            }

            p = (char *)mxmlElementGetAttr(node2, "port");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].port = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node2, "PHASE");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].phase = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node2, "DLYVLD");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].dTValid = strtoul(p, NULL, 10);
            }
        }
        else if (strcmp(p, "交流采样") == 0)
        {
            sXmlCfg.smvCfg.smvSub[i].type = DATA_SRC_ADC;

            p = (char *)mxmlElementGetAttr(node2, "port");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].port = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node2, "PHASE");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].phase = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node2, "RCDLY");
            if (p && (p[0] != '\0'))
            {
                sXmlCfg.smvCfg.smvSub[i].rcDly = strtoul(p, NULL, 10);
                sXmlCfg.smvCfg.smvSub[i].bRdDlyCfg = TRUE;
            }
            else
            {
                sXmlCfg.smvCfg.smvSub[i].bRdDlyCfg = FALSE;
            }
        }
        else if (strcmp(p, "交流复采") == 0)
        {
            sXmlCfg.smvCfg.smvSub[i].type = DATA_SRC_ADC_REPEAT;

            p = (char *)mxmlElementGetAttr(node2, "port");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].port = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node2, "PHASE");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].phase = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node2, "RCDLY");
            if (p && (p[0] != '\0'))
            {
                sXmlCfg.smvCfg.smvSub[i].rcDly = strtoul(p, NULL, 10);
                sXmlCfg.smvCfg.smvSub[i].bRdDlyCfg = TRUE;
            }
            else
            {
                sXmlCfg.smvCfg.smvSub[i].bRdDlyCfg = FALSE;
            }
        }
        else if(strcmp(p, "矢量和") == 0)
        {
            sXmlCfg.smvCfg.smvSub[i].type = DATA_SRC_SUM;
        }
        else
        {
            //assert (FALSE);
        }

        /* 读取通道配置
         */
        sXmlCfg.smvCfg.smvSub[i].chnNum = 0;
        i = sXmlCfg.smvCfg.subNum;
        j = 0;
        for (node3 = mxmlFindElement(node2, node2, "FCDA",
                                     NULL, NULL, MXML_DESCEND);
                node3 != NULL;
                node3 = mxmlFindElement(node3, node2, "FCDA",
                                        NULL, NULL, MXML_DESCEND))
        {
            p = (char *)mxmlElementGetAttr(node3, "INPUTNO");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].inputno = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node3, "INPUTN1");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].inputno1 = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node3, "INPUTN2");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].inputno2 = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node3, "INPUTN3");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].inputno3 = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node3, "HWCFGNO");

            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node3, "CHANEL");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node3, "TYPE");
            if (p)
            {
                if (strcmp(p, "保护电流") == 0)
                    sXmlCfg.smvCfg.smvSub[i].chnSub[j].type = DATA_TYPE_PRO;
                else if (strcmp(p, "测量电流") == 0)
                    sXmlCfg.smvCfg.smvSub[i].chnSub[j].type = DATA_TYPE_MEA;
                else if (strcmp(p, "电压") == 0)
                    sXmlCfg.smvCfg.smvSub[i].chnSub[j].type = DATA_TYPE_VOL;
                else
                {
                    LOG_Dbg_Msg("未定义数据类型 %s\n", (int)p, 2, 3, 4, 5, 6);
                    return FALSE;
                }
            }

            p = (char *)mxmlElementGetAttr(node3, "RATED");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].rated = strtoul(p, NULL, 10);
            }

            p = (char *)mxmlElementGetAttr(node3, "smvValIn");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].smvValIn = strtol(p, NULL, 10);
            }

            if(sXmlCfg.smvCfg.smvSub[i].chnSub[j].smvValIn==0)
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].smvValIn=1;
            }

            p = (char *)mxmlElementGetAttr(node3, "smvValOut");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].smvValOut = strtol(p, NULL, 10);
            }

            if(sXmlCfg.smvCfg.smvSub[i].chnSub[j].smvValOut==0)
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].smvValOut=1;
            }

            s = (char *)mxmlElementGetAttr(node3, "YabanID");
            if (s)
            {
                /*
                 * 配置属性字但属性字内容为空,则为'\0'
                 */
                if (s[0] == '\0')
                {
                    sXmlCfg.smvCfg.smvSub[i].chnSub[j].MuLinkUSE = FALSE;
                }
                else
                {
                    if (SCI_Init_Get_Yaban_Info(s, &iYabanNum) != EP_SUCCESS)
                    {
                        /* 若Yaban ID没有配置,则报错,告警 */
                        if (ENG_MODE == 1)
                        {
                            ER_Set_Err(EV_GSE_CONFI_ERR,
                                       ER_REPORT | ER_ALARM | ER_LOCK,
                                       "SMV link config error\n", 0, 0);
                        }
                        else if (ENG_MODE == 0)
                        {
                            ER_Set_Err(EV_GSE_CONFI_ERR,
                                       ER_REPORT | ER_ALARM | ER_LOCK,
                                       "SMV压板配置错误\n", 0, 0);
                        }

                        sXmlCfg.smvCfg.smvSub[i].chnSub[j].MuLinkUSE = FALSE;
                    }
                    else
                    {
                        /* 获取压板号 */
                        sXmlCfg.smvCfg.smvSub[i].chnSub[j].MuLinkUSE = TRUE;
                        sXmlCfg.smvCfg.smvSub[i].chnSub[j].iYabanNum = iYabanNum;
                    }
                }
            }
            else
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].MuLinkUSE = FALSE;
            }

            p = (char *)mxmlElementGetAttr(node3, "COEFF");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].coeff = strtod(p, NULL);
            }

            p = (char *)mxmlElementGetAttr(node3, "DEAL");
            if (p)
            {
                if (strcmp(p, "积分") == 0)
                    sXmlCfg.smvCfg.smvSub[i].chnSub[j].deal = DATA_DSP_INT;
                else if (strcmp(p, "滤零漂") == 0)
                    sXmlCfg.smvCfg.smvSub[i].chnSub[j].deal = DATA_DSP_ZERO;
                else if (strcmp(p, "原始数据") == 0)
                    sXmlCfg.smvCfg.smvSub[i].chnSub[j].deal = DATA_DSP_RAW;
                else
                {
                    LOG_Dbg_Msg("未定义算法 %s\n", (int)p, 2, 3, 4, 5, 6);
                    return FALSE;
                }
            }

            /* 单通道相位
             * 不配置则用端口移相配置
             */
            p = (char *)mxmlElementGetAttr(node3, "PHASE");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].phase = strtoul(p, NULL, 10);
            }
            else
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].phase = sXmlCfg.smvCfg.smvSub[i].phase;
            }

            p = (char *)mxmlElementGetAttr(node3, "DESC");
            if (p)
            {
                sXmlCfg.smvCfg.smvSub[i].chnSub[j].desc = strdup(p);
            }
            sXmlCfg.smvCfg.smvSub[i].chnNum++;
            j++;
        }
        i++;
        sXmlCfg.smvCfg.subNum++;
    }

    return TRUE;
}


/* 解析PUB配置文件.
 * Para:
 *     xml句柄.
 * Return:
 *     TRUE, FALSE.
 */
BOOL cfgParseSmvPubInfo(mxml_node_t *rootnode)
{
    char *p = NULL;
    char *q = NULL;
    char *s = NULL;
    int i, k, j;
    mxml_node_t	*node1 = NULL;
    mxml_node_t	*node2 = NULL;
    mxml_node_t	*node3 = NULL;
    mxml_node_t	*node4 = NULL;
    mxml_node_t	*node5 = NULL;

    node1 = mxmlFindElement(rootnode, rootnode, "SMV", NULL, NULL, MXML_DESCEND);
    if (!node1)
        return FALSE;

    sXmlCfg.smvCfg.pubNum = 0;
    sXmlCfg.smvCfg.b9_2SndFlag = FALSE;
    sXmlCfg.smvCfg.bFT3SndFlag = FALSE;
    i = 0;

    for (node2 = mxmlFindElement(node1, node1, "PUB_SMV",
                                 NULL, NULL,MXML_DESCEND);
            node2!=NULL;
            node2 = mxmlFindElement(node2, node1, "PUB_SMV",
                                    NULL, NULL,MXML_DESCEND))
    {
        /* 采样率,暂不使用 */
        p = (char *)mxmlElementGetAttr(node2, "smpRate");
        if (p)
        {
            sXmlCfg.smvCfg.smvPub[i].smpRate = strtoul(p, NULL, 10);
        }

        p = (char *)mxmlElementGetAttr(node2, "noASDU");
        if (p)
        {
            sXmlCfg.smvCfg.smvPub[i].noASDU = strtoul(p, NULL, 10);
        }

        p = (char *)mxmlElementGetAttr(node2, "LNName");
        if (p)
        {
            sXmlCfg.smvCfg.smvPub[i].LNName = strtoul(p, NULL, 16);
        }

        p = (char *)mxmlElementGetAttr(node2, "DSName");
        if (p)
        {
            sXmlCfg.smvCfg.smvPub[i].DSName = strtoul(p, NULL, 16);
        }

        p = (char *)mxmlElementGetAttr(node2, "LDName");
        if (p)
        {
            sXmlCfg.smvCfg.smvPub[i].LDName = strtoul(p, NULL, 16);
        }

        p = (char *)mxmlElementGetAttr(node2, "confRev");
        if (p)
        {
            sXmlCfg.smvCfg.smvPub[i].confRev = strtoul(p, NULL, 16);
        }

        p = (char *)mxmlElementGetAttr(node2, "type");

        if ((strcmp(p, "IEC_92LE") == 0) || (strcmp(p, "IEC_92") == 0) || (strcmp(p, "IEC_91") == 0))
        {
            if (strcmp(p, "IEC_92") == 0)
                sXmlCfg.smvCfg.smvPub[i].type = SMV_SEND_SMV92;
            else
            {
                //assert (FALSE);
            }

            /* 只允许1个配置 */
            if (sXmlCfg.smvCfg.b9_2SndFlag)
            {
                //assert (FALSE);
            }

            sXmlCfg.smvCfg.b9_2SndFlag = TRUE;
            sXmlCfg.smvCfg.uc9_2Sn = i;

            node4 = mxmlFindElement(node2, node2, "RatedVal", NULL, NULL, MXML_DESCEND);
            if (node4)
            {
                p = (char *)mxmlElementGetAttr(node4,"rtdPhsCur");
                if (p)
                {
                    sXmlCfg.smvCfg.smvPub[i].rtdPhsCur = strtoul(p, NULL, 10);
                }
                p = (char *)mxmlElementGetAttr(node4, "rtdNeucur");
                if (p)
                {
                    sXmlCfg.smvCfg.smvPub[i].rtdNeucur = strtoul(p, NULL, 10);
                }
                p = (char *)mxmlElementGetAttr(node4, "rtdPhsVol");
                if (p)
                {
                    sXmlCfg.smvCfg.smvPub[i].rtdPhsVol = strtoul(p, NULL, 10);
                }
                p = (char *)mxmlElementGetAttr(node4, "rtdDlyTime");
                if (p)
                {
                    sXmlCfg.smvCfg.smvPub[i].rtdDlyTime = strtoul(p, NULL, 10);
                }
            }

            for (node4 = mxmlFindElement(node2, node2, "Address",
                                         NULL, NULL, MXML_DESCEND);
                    node4 != NULL;
                    node4 = mxmlFindElement(node4, node2, "Address",
                                            NULL, NULL, MXML_DESCEND))
            {
                for (node5 = mxmlFindElement(node4, node4, "P",
                                             NULL, NULL, MXML_DESCEND);
                        node5 != NULL;
                        node5 = mxmlFindElement(node5, node4, "P",
                                                NULL, NULL, MXML_DESCEND))
                {
                    p = (char *)mxmlElementGetAttr(node5, "type");
                    q = (char *)mxmlElementGetValue(node5);
                    if (!p || !q)
                    {
                        LOG_Dbg_Msg("数据源类型未定义\n", 1, 2, 3, 4, 5, 6);
                        return FALSE;
                    }

                    if (strcmp(p, "PORT") == 0)
                    {
                        k = strtoul(q, NULL, 10);
                        sXmlCfg.smvCfg.smvPub[i].pubAddress.port |= (0x0001 << (k-2));
                    }
                    else if (strcmp(p, "MAC-Dst") == 0)
                    {
                        for (k = 0; k<6; k++)
                        {
                            s = strtok(q, "-");
                            sXmlCfg.smvCfg.smvPub[i].pubAddress.dstMac[k] = strtol(s, NULL, 16);
                            q = NULL;
                        }

                    }
                    else if (strcmp(p, "MAC-Src") == 0)
                    {
                        for (k = 0; k<6; k++)
                        {
                            s = strtok(q, "-");
                            sXmlCfg.smvCfg.smvPub[i].pubAddress.srcMac[k] = strtol(s, NULL, 16);
                            q = NULL;
                        }

                    }
                    else if (strcmp(p, "VLAN-PRIORITY") == 0)
                    {
                        sXmlCfg.smvCfg.smvPub[i].pubAddress.priority = strtoul(q, NULL, 10);
                    }
                    else if (strcmp(p, "VLAN-ID") == 0)
                    {
                        sXmlCfg.smvCfg.smvPub[i].pubAddress.vlanID = strtoul(q, NULL, 16);
                    }
                    else if (strcmp(p, "APPID") == 0)
                    {
                        sXmlCfg.smvCfg.smvPub[i].pubAddress.appID = strtoul(q, NULL, 16);
                    }
                    else if (strcmp(p, "SVID") == 0)
                    {
                        strcpy(sXmlCfg.smvCfg.smvPub[i].pubAddress.svID, q);
                    }
                    else if(strcmp(p, "Security") == 0)
                    {
                        strcpy(sXmlCfg.smvCfg.smvPub[i].pubAddress.security, q);
                    }
                }
            }
        }
        else if ((strcmp(p, "自定义FT3") == 0)
                 || (strcmp(p, "标准FT3") == 0)
                 || (strcmp(p, "国网FT3") == 0))
        {
            if (strcmp(p, "标准FT3") == 0)
                sXmlCfg.smvCfg.smvPub[i].type = SMV_SEND_STDFT3;
            else if (strcmp(p, "国网FT3") == 0)
                sXmlCfg.smvCfg.smvPub[i].type = SMV_SEND_GRDFT3;

            /* 只允许1个配置 */
            if (sXmlCfg.smvCfg.bFT3SndFlag)
            {
                //assert (FALSE);
            }

            sXmlCfg.smvCfg.bFT3SndFlag = TRUE;
            sXmlCfg.smvCfg.ucFt3Sn = i;

            node4 = mxmlFindElement(node2, node2, "RatedVal", NULL, NULL, MXML_DESCEND);
            if (node4)
            {
                p = (char *)mxmlElementGetAttr(node4, "rtdPhsCur");
                if (p)
                {
                    sXmlCfg.smvCfg.smvPub[i].rtdPhsCur = strtoul(p, NULL, 10);
                }
                p = (char *)mxmlElementGetAttr(node4, "rtdNeucur");
                if (p)
                {
                    sXmlCfg.smvCfg.smvPub[i].rtdNeucur = strtoul(p, NULL, 10);
                }
                p = (char *)mxmlElementGetAttr(node4, "rtdPhsVol");
                if (p)
                {
                    sXmlCfg.smvCfg.smvPub[i].rtdPhsVol = strtoul(p, NULL, 10);
                }
                p = (char *)mxmlElementGetAttr(node4, "rtdDlyTime");
                if (p)
                {
                    sXmlCfg.smvCfg.smvPub[i].rtdDlyTime = strtoul(p, NULL, 10);
                }
            }

            sXmlCfg.smvCfg.smvPub[i].pubAddress.port = 0;
            for (node4 = mxmlFindElement(node2, node2, "Address",
                                         NULL, NULL, MXML_DESCEND);
                    node4 != NULL;
                    node4 = mxmlFindElement(node4, node2, "Address",
                                            NULL, NULL, MXML_DESCEND))
            {
                for (node5 = mxmlFindElement(node4, node4, "P",
                                             NULL, NULL, MXML_DESCEND);
                        node5 != NULL;
                        node5 = mxmlFindElement(node5, node4, "P",
                                                NULL, NULL, MXML_DESCEND))
                {
                    p = (char *)mxmlElementGetAttr(node5, "type");
                    q = (char *)mxmlElementGetValue(node5);
                    if (!p || !q)
                    {
                        LOG_Dbg_Msg("数据源类型未定义\n", 1, 2, 3, 4, 5, 6);
                        return FALSE;
                    }
                    if (strcmp(p, "PORT") == 0)
                    {
                        k = strtoul(q, NULL, 10);
                        sXmlCfg.smvCfg.smvPub[i].pubAddress.port |= (0x0001<<(k-1));
                    }
                }
            }
        }
        else
        {
            LOG_Dbg_Msg("Pub数据类型未定义\n", 0, 0, 0, 0, 0, 0);
            //assert(0);
        }

        if ((node3 = mxmlFindElement(node2, node2, "DataSet", NULL, NULL, MXML_DESCEND)) != NULL)
        {
            p = (char *)mxmlElementGetAttr(node3, "name");
            if (p)
            {
                strcpy(sXmlCfg.smvCfg.smvPub[i].pubDataset.name,p);
            }

            sXmlCfg.smvCfg.smvPub[i].pubDataset.chnNum = 0;
            j = 0;

            for (node4 = mxmlFindElement(node3, node3, "FCDA",
                                         NULL, NULL, MXML_DESCEND);
                    node4 != NULL;
                    node4 = mxmlFindElement(node4, node3, "FCDA",
                                            NULL, NULL, MXML_DESCEND))
            {

                p = (char *)mxmlElementGetAttr(node4, "INPUTNO");
                if (p)
                {
                    sXmlCfg.smvCfg.smvPub[i].pubDataset.chnPub[j].inputno = strtoul(p, NULL, 10);
                }

                p = (char *)mxmlElementGetAttr(node4, "OUTPUTNO");
                if (p)
                {
                    sXmlCfg.smvCfg.smvPub[i].pubDataset.chnPub[j].outputno = strtoul(p, NULL, 10);
                }

                p = (char *)mxmlElementGetAttr(node4, "smvChnType");
                if ((p) && (p[0] != '\0'))
                {
                    sXmlCfg.smvCfg.smvPub[i].pubDataset.chnPub[j].delayFlag = strtoul(p, NULL, 10);
                }
                else
                {
                    sXmlCfg.smvCfg.smvPub[i].pubDataset.chnPub[j].delayFlag = 1;
                }

                sXmlCfg.smvCfg.smvPub[i].pubDataset.chnNum++;
                j++;
            }
        }
        sXmlCfg.smvCfg.pubNum++;
        i++;
    }

    return TRUE;
}

/* 判断是否显示插值脉冲有效与否.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL cfgJudgeShowPulseSts(void)
{
    if (appType_g == APP_TYPE_DIG)
    {
        /* 数字采样 */
        /* 是否需要判同步 */
        if ((sXmlCfg.smvCfg.intelPulse == INTERNAL_CPU_CRYS)
                || (sXmlCfg.smvCfg.intelPulse == EXTERNAL_SEC_PULSE)
                || (sXmlCfg.smvCfg.intelPulse == EXTERNAL_OPT_B_PULSE)
                || (sXmlCfg.smvCfg.intelPulse == EXTERNAL_1588_0_PULSE)
                || (sXmlCfg.smvCfg.intelPulse == EXTERNAL_1588_1_PULSE)
                || (sXmlCfg.smvCfg.intelPulse == EXTERNAL_1588_2_PULSE)
                || (sXmlCfg.smvCfg.intelPulse == EXTERNAL_1588_3_PULSE)
                || (sXmlCfg.smvCfg.intelPulse == EXTERNAL_SEC_FROM_OPP_PULSE)
                || (sXmlCfg.smvCfg.intelPulse == EXTERNAL_B_FROM_OPP_PULSE)
                || (sXmlCfg.smvCfg.intelPulse ==EXTERNAL_SEC_FROM_TDC_PULSE)
                || (sXmlCfg.smvCfg.intelPulse ==EXTERNAL_B_FROM_TDC_PULSE))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else if (appType_g == APP_TYPE_TRAD)
    {
        /* 传统采样 */
        return FALSE;
    }

    return FALSE;
}

/* FPGA初始化寄存器(部分).
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void fpgaCommonRegAddrInit()
{
    /* 通用寄存器 */
    sFpgaCfg.pctrlReg0Addr = FPGA_MEM_ADRS+GENERAL_REG_BASE+GENERAL_CTRL_REG_0;
    sFpgaCfg.pstsReg0Addr = FPGA_MEM_ADRS+GENERAL_REG_BASE+GENERAL_STATUS_REG_0;
    sFpgaCfg.pstsReg1Addr = FPGA_MEM_ADRS+GENERAL_REG_BASE+GENERAL_STATUS_REG_1;
    sFpgaCfg.pstsSecPulseRegAddr = FPGA_MEM_ADRS+GENERAL_REG_BASE+GENERAL_SEC_SYN_STATUS_REG_1;
    sFpgaCfg.pBReg0Addr = FPGA_MEM_ADRS+GENERAL_REG_BASE+GENERAL_B_REG_0;
    sFpgaCfg.pBReg1Addr = FPGA_MEM_ADRS+GENERAL_REG_BASE+GENERAL_B_REG_1;
    sFpgaCfg.pnsTimeRegAddr = FPGA_MEM_ADRS+GENERAL_REG_BASE+GENERAL_TIME_NS_REG;
}

uint16_t fpgaGetProtocolVer()
{
    sFpgaCfg.stsReg0_un.ulStsReg0 = *sFpgaCfg.pstsReg0Addr;

    return (uint16_t)sFpgaCfg.stsReg0_un.stsReg0_st.proVer;
}

uint32_t fpgaGetFPGAnsTime()
{
    sFpgaCfg.nsTimeReg_un.ulnsTimeReg = *sFpgaCfg.pnsTimeRegAddr;

    return sFpgaCfg.nsTimeReg_un.nsTimeReg_st.nscount;
}

/* 获取FPGA初始化是否完成标志.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL fpgaGetInitFinishFlag()
{
    sFpgaCfg.ctrlReg0_un.ulCtrlReg0 = *sFpgaCfg.pctrlReg0Addr;
    if(sFpgaCfg.ctrlReg0_un.ctrlReg0_st.InitOverFlag)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/* 获取对时模式.
 * Para:
 *     NONE.
 * Return:
 *     对时模式.
 */
UINT8 fpgaGetTimeSyncMode()
{
    UINT8 ucSyncMode = EXTERNAL_HMI_ADJ;
    sFpgaCfg.ctrlReg0_un.ulCtrlReg0 = *sFpgaCfg.pctrlReg0Addr;
    ucSyncMode = sFpgaCfg.ctrlReg0_un.ctrlReg0_st.AdjPulse;
    return ucSyncMode;
}

/* 设置对时模式.
 * Para:
 *     对时模式
 * Return:
 *     NONE
 */
void fpgaSetTimeSyncMode(UINT8 Mode)
{
    int iLockKey;

    iLockKey = intLock();

    if(Mode<TIME_ADJ_MODE_NUM)
    {
        sFpgaCfg.ctrlReg0_un.ulCtrlReg0 = *sFpgaCfg.pctrlReg0Addr;
        sFpgaCfg.ctrlReg0_un.ctrlReg0_st.AdjPulse = Mode;
        sXmlCfg.smvCfg.synMode = Mode;
    }
    *sFpgaCfg.pctrlReg0Addr=sFpgaCfg.ctrlReg0_un.ulCtrlReg0;

    intUnlock(iLockKey);
}

/* 获取插值同步脉冲类型.
 * Para:
 *     NONE.
 * Return:
 *     插值同步脉冲类型.
 */
UINT8 fpgaGetInterPulseMode()
{
    UINT8 ucInterPulseMode = INTERNAL_FPGA_CRYS;
    sFpgaCfg.ctrlReg0_un.ulCtrlReg0 = *sFpgaCfg.pctrlReg0Addr;
    ucInterPulseMode = sFpgaCfg.ctrlReg0_un.ctrlReg0_st.IntelPulse;
    return ucInterPulseMode;
}

/* 设置插值同步脉冲类型.
 * Para:
 *     插值同步脉冲类型
 * Return:
 *     NONE
 */
void fpgaSetInterPulseMode(UINT8 Mode)
{
    int iLockKey;

    iLockKey = intLock();

    if(Mode<PULSE_MODE_MAX_NUM)
    {
        sFpgaCfg.ctrlReg0_un.ulCtrlReg0 = *sFpgaCfg.pctrlReg0Addr;
        sFpgaCfg.ctrlReg0_un.ctrlReg0_st.IntelPulse = Mode;
        sXmlCfg.smvCfg.intelPulse = Mode;
    }
    *sFpgaCfg.pctrlReg0Addr=sFpgaCfg.ctrlReg0_un.ulCtrlReg0;

    intUnlock(iLockKey);
}

/* FPGA SMV发送信息生成.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL fpgaSmvSendInfoCreate(void)
{
    int i = 0;
    int j = 0;
    int k = 0;
    int m = 0;
    int n = 0;
    int p = 0;
    int flag=0;
    BOOL bfound=FALSE;

    /*是否FT3发送*/
    if (sXmlCfg.smvCfg.bFT3SndFlag)
    {
        sFpgaCfg.ft3SndPara.bUsed = TRUE;

        /* 发送类型 */
        sFpgaCfg.ft3SndPara.type = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.ucFt3Sn].type;
        sFpgaCfg.ctrlReg0_un.ctrlReg0_st.FT3SndType = sFpgaCfg.ft3SndPara.type;

        /* 置发送标志,为0时发送 */
        sFpgaCfg.ctrlReg0_un.ctrlReg0_st.FT3SndFlag = 0;

        sFpgaCfg.ft3SndPara.chnNum = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.ucFt3Sn].pubDataset.chnNum;

        /* 只处理1个FT3发送通道 */
        for (i = 0; i<sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.ucFt3Sn].pubDataset.chnNum; i++)
        {
            for(j = 0; j<sXmlCfg.smvCfg.subNum ; j++)
            {
                for(k = 0; k<sXmlCfg.smvCfg.smvSub[j].chnNum ; k++)
                {
                    if(sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.ucFt3Sn].pubDataset.chnPub[i].inputno==sXmlCfg.smvCfg.smvSub[j].chnSub[k].inputno)
                    {
                        if ((sXmlCfg.smvCfg.smvSub[j].type == DATA_SRC_STDFT3)
                                || (sXmlCfg.smvCfg.smvSub[j].type == DATA_SRC_GRDFT3)
                                || (sXmlCfg.smvCfg.smvSub[j].type == DATA_SRC_ADC))
                        {
                            sFpgaCfg.ft3SndPara.chnSndCfg[i].chnMergCfg[0].cfgReg_st.valid=1;

                            sFpgaCfg.ft3SndPara.chnSndCfg[i].chnMergCfg[0].cfgReg_st.srcSel=sXmlCfg.smvCfg.smvSub[j].port;

                            sFpgaCfg.ft3SndPara.chnSndCfg[i].chnMergCfg[0].cfgReg_st.srcChnNo=sXmlCfg.smvCfg.smvSub[j].chnSub[k].chanel-1;

                            bfound=FALSE;

                            for(m=0; m<iHwAiChNum_g; m++)
                            {
                                if(sXmlCfg.smvCfg.smvSub[j].chnSub[k].hwcfgno==(phwaich_g[m].ucModCh+1))
                                {
                                    sFpgaCfg.ft3SndPara.chnSndCfg[i].SubNoComefrom[0]=j;
                                    sFpgaCfg.ft3SndPara.chnSndCfg[i].ChnNoComefrom[0]=k;
                                    sFpgaCfg.ft3SndPara.chnSndCfg[i].HwCfgIndexComefrom[0]=m;
                                    bfound=TRUE;
                                }
                            }

                            if(!bfound)
                            {
                                sFpgaCfg.ft3SndPara.chnSndCfg[i].SubNoComefrom[0]=-1;
                                sFpgaCfg.ft3SndPara.chnSndCfg[i].ChnNoComefrom[0]=-1;
                                sFpgaCfg.ft3SndPara.chnSndCfg[i].HwCfgIndexComefrom[0]=-1;
                            }
                        }

                        else if(sXmlCfg.smvCfg.smvSub[j].type == DATA_SRC_SUM)/*合流处理*/
                        {
                            for(m = 0; m<sXmlCfg.smvCfg.subNum ; m++)
                            {
                                if ((sXmlCfg.smvCfg.smvSub[m].type == DATA_SRC_STDFT3)
                                        || (sXmlCfg.smvCfg.smvSub[m].type == DATA_SRC_GRDFT3)
                                        || (sXmlCfg.smvCfg.smvSub[m].type == DATA_SRC_ADC)
                                        || (sXmlCfg.smvCfg.smvSub[m].type == DATA_SRC_ADC_REPEAT))/*合流不能选中合流通道*/
                                {
                                    for(n = 0; n<sXmlCfg.smvCfg.smvSub[m].chnNum ; n++)
                                    {
                                        flag=0;
                                        if(sXmlCfg.smvCfg.smvSub[j].chnSub[k].inputno1==sXmlCfg.smvCfg.smvSub[m].chnSub[n].inputno)
                                        {
                                            flag=1;
                                        }/*通道1*/
                                        else if(sXmlCfg.smvCfg.smvSub[j].chnSub[k].inputno2==sXmlCfg.smvCfg.smvSub[m].chnSub[n].inputno)
                                        {
                                            flag=2;
                                        }/*通道2*/
                                        else if(sXmlCfg.smvCfg.smvSub[j].chnSub[k].inputno3==sXmlCfg.smvCfg.smvSub[m].chnSub[n].inputno)
                                        {
                                            flag=3;
                                        }/*通道3*/
                                        else
                                        {
                                            continue;
                                        }

                                        if(flag!=0)
                                        {
                                            sFpgaCfg.ft3SndPara.chnSndCfg[i].chnMergCfg[flag-1].cfgReg_st.valid=1;

                                            sFpgaCfg.ft3SndPara.chnSndCfg[i].chnMergCfg[flag-1].cfgReg_st.srcSel=sXmlCfg.smvCfg.smvSub[m].port;

                                            sFpgaCfg.ft3SndPara.chnSndCfg[i].chnMergCfg[flag-1].cfgReg_st.srcChnNo=sXmlCfg.smvCfg.smvSub[m].chnSub[n].chanel-1;

                                            bfound=FALSE;

                                            for(p=0; p<iHwAiChNum_g; p++)
                                            {
                                                if(sXmlCfg.smvCfg.smvSub[m].chnSub[n].hwcfgno==(phwaich_g[p].ucModCh+1))
                                                {
                                                    sFpgaCfg.ft3SndPara.chnSndCfg[i].SubNoComefrom[flag-1]=m;
                                                    sFpgaCfg.ft3SndPara.chnSndCfg[i].ChnNoComefrom[flag-1]=n;
                                                    sFpgaCfg.ft3SndPara.chnSndCfg[i].HwCfgIndexComefrom[flag-1]=p;
                                                    bfound=TRUE;
                                                }
                                            }

                                            if(!bfound)
                                            {
                                                sFpgaCfg.ft3SndPara.chnSndCfg[i].SubNoComefrom[flag-1]=-1;
                                                sFpgaCfg.ft3SndPara.chnSndCfg[i].ChnNoComefrom[flag-1]=-1;
                                                sFpgaCfg.ft3SndPara.chnSndCfg[i].HwCfgIndexComefrom[flag-1]=-1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            assert(0);
                        }
                    }
                }
            }

            sFpgaCfg.ft3SndPara.outputChnNo[i]
                = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.ucFt3Sn].pubDataset.chnPub[i].outputno;
        }

        /* FT3报文头变量 */
        sFpgaCfg.ft3SndPara.LNName = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.ucFt3Sn].LNName;
        sFpgaCfg.ft3SndPara.DSName = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.ucFt3Sn].DSName;
        sFpgaCfg.ft3SndPara.LDName = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.ucFt3Sn].LDName;
        sFpgaCfg.ft3SndPara.rtdPhsCur = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.ucFt3Sn].rtdPhsCur;
        sFpgaCfg.ft3SndPara.rtdNeucur = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.ucFt3Sn].rtdNeucur;
        sFpgaCfg.ft3SndPara.rtdPhsVol = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.ucFt3Sn].rtdPhsVol;
        sFpgaCfg.ft3SndPara.rtdDlyTime = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.ucFt3Sn].rtdDlyTime;

        /* 地址赋值 */
        sFpgaCfg.ft3SndPara.pCfgRegAddr = FPGA_MEM_ADRS+FT3_T_0_REG_BASE+SV_CFG_ADDR;
        sFpgaCfg.ft3SndPara.pFrameRegAddr = FPGA_MEM_ADRS+FT3_T_0_REG_BASE+SV_DATA_BUF;
    }

    /*是否发送PUB */
    if (sXmlCfg.smvCfg.b9_2SndFlag)
    {
        sFpgaCfg.svSndPara.bUsed = TRUE;

        /* 基准端口号固定, 从配置中来
         */
        sFpgaCfg.svSndPara.ucPortNo = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubAddress.port;

        /* 置发送标志,为0时发送 */
        sFpgaCfg.ctrlReg0_un.ctrlReg0_st.SmvSndFlag = 0;

        sFpgaCfg.svSndPara.chnNum = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubDataset.chnNum;

        /* 只处理1个9-2发送通道 */
        for (i = 0; i<sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubDataset.chnNum; i++)
        {
            for(j = 0; j<sXmlCfg.smvCfg.subNum ; j++)
            {
                for(k = 0; k<sXmlCfg.smvCfg.smvSub[j].chnNum ; k++)
                {
                    if(sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubDataset.chnPub[i].inputno==sXmlCfg.smvCfg.smvSub[j].chnSub[k].inputno)
                    {
                        if ((sXmlCfg.smvCfg.smvSub[j].type == DATA_SRC_STDFT3)
                                || (sXmlCfg.smvCfg.smvSub[j].type == DATA_SRC_GRDFT3)
                                || (sXmlCfg.smvCfg.smvSub[j].type == DATA_SRC_ADC)
                                || (sXmlCfg.smvCfg.smvSub[j].type == DATA_SRC_ADC_REPEAT)
                                || (sXmlCfg.smvCfg.smvSub[j].type == DATA_SRC_SMV))
                        {
                            /* 延时通道还是采样通道判断
                             */
                            if (sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubDataset.chnPub[i].delayFlag == 2)
                            {
                                sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[0].cfgReg_st.delayFlag = 1;
                            }

                            sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[0].cfgReg_st.valid=1;

                            /* 是否是测量电流 */
                            if (sXmlCfg.smvCfg.smvSub[j].chnSub[k].type == DATA_TYPE_MEA)
                            {
                                sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[0].cfgReg_st.MeaFlag = 1;
                            }

                            sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[0].cfgReg_st.srcSel=sXmlCfg.smvCfg.smvSub[j].port;

                            sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[0].cfgReg_st.srcChnNo=sXmlCfg.smvCfg.smvSub[j].chnSub[k].chanel-1;

                            bfound=FALSE;

                            for(m=0; m<iHwAiChNum_g; m++)
                            {
                                if(sXmlCfg.smvCfg.smvSub[j].chnSub[k].hwcfgno==(phwaich_g[m].ucModCh+1))
                                {
                                    sFpgaCfg.svSndPara.chnSndCfg[i].SubNoComefrom[0]=j;
                                    sFpgaCfg.svSndPara.chnSndCfg[i].ChnNoComefrom[0]=k;
                                    sFpgaCfg.svSndPara.chnSndCfg[i].HwCfgIndexComefrom[0]=m;
                                    bfound=TRUE;
                                }
                            }

                            if(!bfound)
                            {
                                sFpgaCfg.svSndPara.chnSndCfg[i].SubNoComefrom[0]=-1;
                                sFpgaCfg.svSndPara.chnSndCfg[i].ChnNoComefrom[0]=-1;
                                sFpgaCfg.svSndPara.chnSndCfg[i].HwCfgIndexComefrom[0]=-1;
                            }
                        }

                        else if(sXmlCfg.smvCfg.smvSub[j].type == DATA_SRC_SUM)/*合流处理*/
                        {
                            for(m = 0; m<sXmlCfg.smvCfg.subNum ; m++)
                            {
                                if ((sXmlCfg.smvCfg.smvSub[m].type == DATA_SRC_STDFT3)
                                        || (sXmlCfg.smvCfg.smvSub[m].type == DATA_SRC_GRDFT3)
                                        || (sXmlCfg.smvCfg.smvSub[m].type == DATA_SRC_ADC)
                                        || (sXmlCfg.smvCfg.smvSub[m].type == DATA_SRC_ADC_REPEAT)
                                        || (sXmlCfg.smvCfg.smvSub[m].type == DATA_SRC_SMV))/*合流不能选中合流通道*/
                                {
                                    for(n = 0; n<sXmlCfg.smvCfg.smvSub[m].chnNum ; n++)
                                    {
                                        flag=0;
                                        if(sXmlCfg.smvCfg.smvSub[j].chnSub[k].inputno1==sXmlCfg.smvCfg.smvSub[m].chnSub[n].inputno)
                                        {
                                            flag=1;
                                        }/*通道1*/
                                        else if(sXmlCfg.smvCfg.smvSub[j].chnSub[k].inputno2==sXmlCfg.smvCfg.smvSub[m].chnSub[n].inputno)
                                        {
                                            flag=2;
                                        }/*通道2*/
                                        else if(sXmlCfg.smvCfg.smvSub[j].chnSub[k].inputno3==sXmlCfg.smvCfg.smvSub[m].chnSub[n].inputno)
                                        {
                                            flag=3;
                                        }/*通道3*/
                                        else
                                        {
                                            continue;
                                        }

                                        if(flag!=0)
                                        {
                                            /* 延时通道还是采样通道判断
                                             */
                                            if (sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubDataset.chnPub[i].delayFlag == 2)
                                            {
                                                sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[flag-1].cfgReg_st.delayFlag = 1;
                                            }

                                            sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[flag-1].cfgReg_st.valid=1;

                                            /* 是否是测量电流 */
                                            if (sXmlCfg.smvCfg.smvSub[m].chnSub[n].type == DATA_TYPE_MEA)
                                            {
                                                sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[flag-1].cfgReg_st.MeaFlag = 1;
                                            }

                                            sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[flag-1].cfgReg_st.srcSel=sXmlCfg.smvCfg.smvSub[m].port;

                                            sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[flag-1].cfgReg_st.srcChnNo=sXmlCfg.smvCfg.smvSub[m].chnSub[n].chanel-1;

                                            bfound=FALSE;

                                            for(p=0; p<iHwAiChNum_g; p++)
                                            {
                                                if(sXmlCfg.smvCfg.smvSub[m].chnSub[n].hwcfgno==(phwaich_g[p].ucModCh+1))
                                                {
                                                    sFpgaCfg.svSndPara.chnSndCfg[i].SubNoComefrom[flag-1]=m;
                                                    sFpgaCfg.svSndPara.chnSndCfg[i].ChnNoComefrom[flag-1]=n;
                                                    sFpgaCfg.svSndPara.chnSndCfg[i].HwCfgIndexComefrom[flag-1]=p;
                                                    bfound=TRUE;
                                                }
                                            }

                                            if(!bfound)
                                            {
                                                sFpgaCfg.svSndPara.chnSndCfg[i].SubNoComefrom[flag-1]=-1;
                                                sFpgaCfg.svSndPara.chnSndCfg[i].ChnNoComefrom[flag-1]=-1;
                                                sFpgaCfg.svSndPara.chnSndCfg[i].HwCfgIndexComefrom[flag-1]=-1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            assert(0);
                        }
                    }
                }
            }

            sFpgaCfg.svSndPara.outputChnNo[i]
                = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubDataset.chnPub[i].outputno;
        }

        /* MAC地址 */
        for (i = 0; i<MAC_ADDR_LEN; i++)
        {
            sFpgaCfg.svSndPara.ucDestMacAddr[i]
                = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubAddress.dstMac[i];
            sFpgaCfg.svSndPara.ucSrcMacAddr[i]
                = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubAddress.srcMac[i];
        }

        /*TCI*/
        sFpgaCfg.svSndPara.TCI = (sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubAddress.priority << 13)
                                 | sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubAddress.vlanID;

        /*APPID*/
        sFpgaCfg.svSndPara.APPID = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubAddress.appID;

        /* LENGTH */
        sFpgaCfg.svSndPara.seqData_len = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubDataset.chnNum;  /* 延时通道需要配置 */
        sFpgaCfg.svSndPara.noASDU = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].noASDU;
        sFpgaCfg.svSndPara.seqData_len = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubDataset.chnNum<<3;  /* 数据集长度 */
        sFpgaCfg.svSndPara.strLen = strlen(sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubAddress.svID);
        sFpgaCfg.svSndPara.AllLength
            = (uint16_t)(APDU_HEAD_LEN+((ASDU_HEAD_LEN+sFpgaCfg.svSndPara.seqData_len+sFpgaCfg.svSndPara.strLen)*sFpgaCfg.svSndPara.noASDU));
        sFpgaCfg.svSndPara.APDU_len
            = (uint8_t)(APDU_2_HEAD_LEN+((ASDU_HEAD_LEN+sFpgaCfg.svSndPara.seqData_len+sFpgaCfg.svSndPara.strLen)*sFpgaCfg.svSndPara.noASDU));
        sFpgaCfg.svSndPara.seqASDU_len
            = (uint16_t)((ASDU_HEAD_LEN+sFpgaCfg.svSndPara.seqData_len+sFpgaCfg.svSndPara.strLen)*sFpgaCfg.svSndPara.noASDU);
        sFpgaCfg.svSndPara.ASDU_len = ASDU_2_HEAD_LEN+sFpgaCfg.svSndPara.seqData_len+sFpgaCfg.svSndPara.strLen;
        sFpgaCfg.svSndPara.confRev = 0x01;

        /* 地址赋值 */
        sFpgaCfg.svSndPara.pCfgRegAddr = FPGA_MEM_ADRS+SV_REG_BASE+SV_CFG_ADDR;
        sFpgaCfg.svSndPara.pLenRegAddr = FPGA_MEM_ADRS+SV_REG_BASE+SV_LEN_ADDR;
        sFpgaCfg.svSndPara.pportEnbRegAddr = FPGA_MEM_ADRS+SV_REG_BASE+SV_ENABLE_ADDR;
        sFpgaCfg.svSndPara.pFrameRegAddr = FPGA_MEM_ADRS+SV_REG_BASE+SV_DATA_BUF;

        /* 端口使能 */
        sFpgaCfg.svSndPara.portEnb.portEnbReg_st.port2 = !((sFpgaCfg.svSndPara.ucPortNo >> 0) & 0x01);
        sFpgaCfg.svSndPara.portEnb.portEnbReg_st.port3 = !((sFpgaCfg.svSndPara.ucPortNo >> 1) & 0x01);
        sFpgaCfg.svSndPara.portEnb.portEnbReg_st.port4 = !((sFpgaCfg.svSndPara.ucPortNo >> 2) & 0x01);
        sFpgaCfg.svSndPara.portEnb.portEnbReg_st.port5 = !((sFpgaCfg.svSndPara.ucPortNo >> 3) & 0x01);
        sFpgaCfg.svSndPara.portEnb.portEnbReg_st.port6 = !((sFpgaCfg.svSndPara.ucPortNo >> 4) & 0x01);
        sFpgaCfg.svSndPara.portEnb.portEnbReg_st.port7 = !((sFpgaCfg.svSndPara.ucPortNo >> 5) & 0x01);
    }

    return TRUE;
}

/* 刷新SMV发送系数
 * Para:
 *     NONE.
 * Return:
 *     NONE
 */
void UpdateFPGASmvSendCoff(void)
{
    int i,j;
    int SubNo=-1;
    int ChnNo=-1;
    int HwCfgIndex=-1;
    CHN_SUB_INFO *SubChnInfo=NULL;
    float fCoff = 0.0;

    if(sFpgaCfg.ft3SndPara.bUsed)
    {
        for (i = 0; i<sFpgaCfg.ft3SndPara.chnNum; i++)
        {
            for (j = 0; j<CHN_MERG_NUM; j++)
            {
                if(sFpgaCfg.ft3SndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.valid)
                {
                    SubNo=sFpgaCfg.ft3SndPara.chnSndCfg[i].SubNoComefrom[j];
                    ChnNo=sFpgaCfg.ft3SndPara.chnSndCfg[i].ChnNoComefrom[j];
                    HwCfgIndex=sFpgaCfg.ft3SndPara.chnSndCfg[i].HwCfgIndexComefrom[j];

                    if((SubNo<0)||(ChnNo<0)||(HwCfgIndex<0))
                    {
                        continue;
                    }

                    SubChnInfo=&sXmlCfg.smvCfg.smvSub[SubNo].chnSub[ChnNo];

                    switch(sFpgaCfg.ft3SndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.srcSel)
                    {
                        case AD_PORT_NO:
                        case AD_REPEAT_PORT_NO:

                            if (SubChnInfo->smvValOut == 0)
                            {
                                fCoff=0.0+FLT_PRECISION;
                            }
                            else
                            {
                                /* 扩大1024倍 */
                                if (SubChnInfo->type == DATA_TYPE_PRO)
                                {
                                    fCoff=((SMV_DIGIT_SCP << 10)*SubChnInfo->coeff)*phwaich_g[HwCfgIndex].fGain
                                          /((float)SubChnInfo->smvValOut*SMV_MAX_DIGIT/(float)AD_MAX_INPUT);
                                }
                                else if (SubChnInfo->type == DATA_TYPE_MEA)
                                {
                                    fCoff=((SMV_DIGIT_SCM << 10)*SubChnInfo->coeff)*phwaich_g[HwCfgIndex].fGain
                                          /((float)SubChnInfo->smvValOut*SMV_MAX_DIGIT/(float)AD_MAX_INPUT);
                                }
                                else if (SubChnInfo->type == DATA_TYPE_VOL)
                                {
                                    fCoff=((SMV_DIGIT_SV << 10)*SubChnInfo->coeff)*phwaich_g[HwCfgIndex].fGain
                                          /((float)SubChnInfo->smvValOut*SMV_MAX_DIGIT/(float)AD_MAX_INPUT);
                                }
                            }

                            printf("FT3 send %d-%d gain=%f coff=%f\n",i,j,phwaich_g[HwCfgIndex].fGain,fCoff);

                            sFpgaCfg.ft3SndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.coff = (uint32_t)fCoff;

                            break;

                        case FT3_PORT_NO_1:
                        case FT3_PORT_NO_2:

                            fCoff= (1 << 10)*(float)SubChnInfo->coeff*phwaich_g[HwCfgIndex].fGain;

                            printf("FT3 send %d-%d gain=%f coff=%f\n",i,j,phwaich_g[HwCfgIndex].fGain,fCoff);

                            sFpgaCfg.ft3SndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.coff = (uint32_t)fCoff;

                            break;

                        case SMV_PORT_NO_1:
                            break;/*不支持*/
                        case SMV_PORT_NO_2:
                            break;/*不支持*/
                        default:
                            break;
                    }
                }

                *(sFpgaCfg.ft3SndPara.pCfgRegAddr+(sFpgaCfg.ft3SndPara.outputChnNo[i]-1)*CHN_MERG_NUM+j)
                    = sFpgaCfg.ft3SndPara.chnSndCfg[i].chnMergCfg[j].ulCfgReg;
            }
        }
    }

    if(sFpgaCfg.svSndPara.bUsed)
    {
        for (i = 0; i<sFpgaCfg.svSndPara.chnNum; i++)
        {
            for (j = 0; j<CHN_MERG_NUM; j++)
            {
                if(sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.valid)
                {
                    SubNo=sFpgaCfg.svSndPara.chnSndCfg[i].SubNoComefrom[j];
                    ChnNo=sFpgaCfg.svSndPara.chnSndCfg[i].ChnNoComefrom[j];
                    HwCfgIndex=sFpgaCfg.svSndPara.chnSndCfg[i].HwCfgIndexComefrom[j];

                    if((SubNo<0)||(ChnNo<0)||(HwCfgIndex<0))
                    {
                        continue;
                    }

                    SubChnInfo=&sXmlCfg.smvCfg.smvSub[SubNo].chnSub[ChnNo];

                    switch(sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.srcSel)
                    {
                        case AD_PORT_NO:
                        case AD_REPEAT_PORT_NO:

                            if (SubChnInfo->smvValOut == 0)
                            {
                                fCoff=0.0+FLT_PRECISION;
                            }
                            else
                            {
                                fCoff=SubChnInfo->smvValIn*SubChnInfo->coeff*phwaich_g[HwCfgIndex].fGain
                                      /((float)(SubChnInfo->smvValOut)*SMV_MAX_DIGIT/AD_MAX_INPUT);
                            }

                            printf("9-2 send %d-%d gain=%f coff=%f\n",i,j,phwaich_g[HwCfgIndex].fGain,fCoff);

                            if (sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.MeaFlag == 1)
                            {
                                sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.coff
                                    = (uint32_t)(fCoff*(0x1 << 5));
                            }
                            else
                            {
                                sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.coff = (uint32_t)fCoff;
                            }

                            break;

                        case FT3_PORT_NO_1:
                        case FT3_PORT_NO_2:

                            if (SubChnInfo->smvValOut == 0)
                            {
                                fCoff=0.0+FLT_PRECISION;
                            }
                            else
                            {
                                fCoff=SubChnInfo->smvValIn*SubChnInfo->coeff*phwaich_g[HwCfgIndex].fGain
                                      /(float)(SubChnInfo->smvValOut);
                            }

                            printf("9-2 send %d-%d gain=%f coff=%f\n",i,j,phwaich_g[HwCfgIndex].fGain,fCoff);

                            if (sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.MeaFlag == 1)
                            {
                                sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.coff
                                    = (uint32_t)(fCoff*(0x1 << 5));
                            }
                            else
                            {
                                sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.coff = (uint32_t)fCoff;
                            }

                            break;

                        case SMV_PORT_NO_1:
                        case SMV_PORT_NO_2:

                            fCoff=SubChnInfo->coeff*phwaich_g[HwCfgIndex].fGain;

                            printf("9-2 send %d-%d gain=%f coff=%f\n",i,j,phwaich_g[HwCfgIndex].fGain,fCoff);

                            if (sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.MeaFlag == 1)
                            {
                                sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.coff
                                    = (uint32_t)(fCoff*(0x1 << 5));
                            }
                            else
                            {
                                sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[j].cfgReg_st.coff = (uint32_t)fCoff;
                            }

                            break;
                        default:
                            break;
                    }
                }

                *(sFpgaCfg.svSndPara.pCfgRegAddr+(sFpgaCfg.svSndPara.outputChnNo[i]-1)*CHN_MERG_NUM+j)
                    = sFpgaCfg.svSndPara.chnSndCfg[i].chnMergCfg[j].ulCfgReg;
            }
        }
    }
}


/* FPGA初始化信息生成.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL fpgaInfoCreate(void)
{
    UINT8 i = 0;
    UINT8 j = 0;

    /*
     * 必须配置1路SUB, 即使不配置A/D采样, FPGA总是执行A/D采样操作
     */
    assert ((sXmlCfg.smvCfg.subNum <= MAX_SUB_NUM));
    assert (sXmlCfg.smvCfg.pubNum <= MAX_PUB_NUM);

    /* 数据结构判断 */
    assert (sizeof(sFpgaCfg.ctrlReg0_un.ctrlReg0_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.stsReg0_un.stsReg0_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.stsReg1_un.stsReg1_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.adPara.ctrlReg_un.ctrlReg_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.adPara.stsReg_un.stsReg_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.adPara.attrReg_un[0].attrReg_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.adPara.bdReg_un[0].bdReg_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.adParaRepeat.ctrlReg_un.ctrlReg_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.adParaRepeat.stsReg_un.stsReg_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.adParaRepeat.attrReg_un[0].attrReg_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.adParaRepeat.bdReg_un[0].bdReg_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.ft3Para[0].ctrlReg_un.ctrlReg_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.ft3Para[0].stsReg_un.stsReg_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.ft3Para[0].bdReg_un[0].bdReg_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.ft3Para[0].reg) == 4*UINT32_BYTE_NUM);
    assert (sizeof(struct AD_REPEAT_DATA_REG) == (2*(2+FPGA_AD_REPEAT_SAM_CHN_NUM)));
    assert (sizeof(struct AD_DATA_REG) == (2*(2+FPGA_AD_SAM_CHN_NUM)));
    assert (sizeof(struct FT3_DATA_REG) == (2*(2+FPGA_FT3_SAM_CHN_NUM)));
    assert (sizeof(struct CHN_FT3_CFG_ST) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.svRcvPara[0].ctrlReg_un.ctrlReg_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.svRcvPara[0].macReg0_un.macReg0_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.svRcvPara[0].macReg1_un.macReg1_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.svRcvPara[0].stsReg0_un.stsReg0_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.svRcvPara[0].stsReg1_un.stsReg1_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.svRcvPara[0].stsReg2_un.stsReg2_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.svRcvPara[0].bdReg_un[0].bdReg_st) == UINT32_BYTE_NUM);
    assert (sizeof(struct SV_RCV_DATA_REG) == (2*2+4*FPGA_SV_RCV_CHN_NUM));
    assert (sizeof(struct CHN_SV_CFG_ST) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.svSndPara.chnLen.lenReg_st) == UINT32_BYTE_NUM);
    assert (sizeof(sFpgaCfg.svSndPara.portEnb.portEnbReg_st) == UINT32_BYTE_NUM);

    /* 通用寄存器 */
    sFpgaCfg.pctrlReg0Addr = FPGA_MEM_ADRS+GENERAL_REG_BASE+GENERAL_CTRL_REG_0;
    sFpgaCfg.pstsReg0Addr = FPGA_MEM_ADRS+GENERAL_REG_BASE+GENERAL_STATUS_REG_0;
    sFpgaCfg.pstsReg1Addr = FPGA_MEM_ADRS+GENERAL_REG_BASE+GENERAL_STATUS_REG_1;
    sFpgaCfg.pstsSecPulseRegAddr = FPGA_MEM_ADRS+GENERAL_REG_BASE+GENERAL_SEC_SYN_STATUS_REG_1;
    sFpgaCfg.pBReg0Addr = FPGA_MEM_ADRS+GENERAL_REG_BASE+GENERAL_B_REG_0;
    sFpgaCfg.pBReg1Addr = FPGA_MEM_ADRS+GENERAL_REG_BASE+GENERAL_B_REG_1;
    sFpgaCfg.pnsTimeRegAddr = FPGA_MEM_ADRS+GENERAL_REG_BASE+GENERAL_TIME_NS_REG;

    sFpgaCfg.ctrlReg0_un.ulCtrlReg0 = 0;  /* 全局控制寄存器初始化为0 */
    sFpgaCfg.ctrlReg0_un.ctrlReg0_st.InitOverFlag = 0;  /* FPGA配置正在处理中 */
    sFpgaCfg.ctrlReg0_un.ctrlReg0_st.IntelPulse = sXmlCfg.smvCfg.intelPulse;  /* 插值脉冲类型 */
    sFpgaCfg.ctrlReg0_un.ctrlReg0_st.PulseRev = sXmlCfg.smvCfg.intelRev;  /* 插值信号反转 */

    sFpgaCfg.ctrlReg0_un.ctrlReg0_st.AdjPulse = sXmlCfg.smvCfg.synMode;  /* 对时脉冲类型 */

    sFpgaCfg.ctrlReg0_un.ctrlReg0_st.AdjRev = sXmlCfg.smvCfg.synRev;  /* 对时信号反转 */
    sFpgaCfg.ctrlReg0_un.ctrlReg0_st.SndSynFlag = sXmlCfg.smvCfg.syn;  /* 是否强制同步 */
    sFpgaCfg.ctrlReg0_un.ctrlReg0_st.SndTestFlag = sXmlCfg.smvCfg.test;    /* 是否强制测试 */

    /* 是否需要判同步 */
    if (cfgJudgeShowPulseSts())
    {
        sFpgaCfg.bExternalSynFlag = TRUE;
    }
    else
    {
        sFpgaCfg.bExternalSynFlag = FALSE;
    }

    /* 是否配置FT3 */
    sFpgaCfg.bFt3CfgFlag = FALSE;

    /* 有效标志 */
    smvData.ucAllSrcValidFlag = 0;
    smvData.ucCurSrcValidFlag = 0;
    smvData.ucRdCurSrcValidFlag = 0;

    /* 查询SUB配置 */
    for (i = 0; i<sXmlCfg.smvCfg.subNum; i++)
    {
        if (sXmlCfg.smvCfg.smvSub[i].type == DATA_SRC_ADC)
        {
            /* A/D采样 */

            sFpgaCfg.adPara.bUsed = TRUE;

            /* A/D采样端口号固定为0 */
            sFpgaCfg.adPara.ucPortNo = sXmlCfg.smvCfg.smvSub[i].port;
            assert (sFpgaCfg.adPara.ucPortNo == AD_PORT_NO);

            /* 按通道配置调相角 */
            for (j = 0; j<sXmlCfg.smvCfg.smvSub[i].chnNum; j++)
            {
                sFpgaCfg.adPara.attrReg_un[sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel-1].attrReg_st.phaseShift
                    = sXmlCfg.smvCfg.smvSub[i].chnSub[j].phase;
            }

            /* 如果配置则使用配置参数,否则使用默认参数 */
            if (sXmlCfg.smvCfg.smvSub[i].bRdDlyCfg)
            {
                /* 滤波延时设置,所有通道一致 */
                sFpgaCfg.adPara.ctrlReg_un.ctrlReg_st.rcDelay
                    = sXmlCfg.smvCfg.smvSub[i].rcDly;

                sFpgaCfg.adPara.bRdDlyCfg = TRUE;
            }
            else
            {
                sFpgaCfg.adPara.bRdDlyCfg = FALSE;
            }

            /* 错误标志,用于清除 */
            sFpgaCfg.adPara.stsReg_un.stsReg_st.overflowFlag = 1;

            /* 地址赋值 */
            sFpgaCfg.adPara.pctrlRegAddr = FPGA_MEM_ADRS+AD_REG_BASE+AD_CTRL_REG_0;
            sFpgaCfg.adPara.pstsRegAddr = FPGA_MEM_ADRS+AD_REG_BASE+AD_STATUS_REG_0;
            sFpgaCfg.adPara.pAttrRegAddr = FPGA_MEM_ADRS+AD_REG_BASE+AD_ATTR_REG;

            /* 有效标志 */
            smvData.ucAllSrcValidFlag |= 0x01<<AD_PORT_NO;

            /* 最大通道内序 */
            for (j = 0; j<sXmlCfg.smvCfg.smvSub[i].chnNum; j++)
            {
                if ((sFpgaCfg.adPara.ucMaxChnNum<sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel)
                        && (sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel != 255))
                {
                    sFpgaCfg.adPara.ucMaxChnNum = sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel;
                }
            }

            /* 压板号清零 */
            for (j = 0; j<sFpgaCfg.adPara.ucMaxChnNum; j++)
            {
                sFpgaCfg.adPara.iYabanNum[j] = -1;
                sFpgaCfg.adPara.bRtYabanValue[j] = TRUE;
            }

            /* 设置压板号,按采样通道排列 */
            for (j = 0; j<sXmlCfg.smvCfg.smvSub[i].chnNum; j++)
            {
                if (sXmlCfg.smvCfg.smvSub[i].chnSub[j].MuLinkUSE
                        && (sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel <= FPGA_AD_SAM_CHN_NUM))
                {

                    sFpgaCfg.adPara.iYabanNum[sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel-1]
                        = sXmlCfg.smvCfg.smvSub[i].chnSub[j].iYabanNum;
                }
                else
                {
                    sFpgaCfg.adPara.iYabanNum[sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel-1] = -1;
                }
            }

            /* 通道属性 */
            for (j = 0; j<sFpgaCfg.adPara.ucMaxChnNum; j++)
            {
                *(sFpgaCfg.adPara.pAttrRegAddr+j)
                    = sFpgaCfg.adPara.attrReg_un[j].ulAttrReg;
            }
        }
        else if (sXmlCfg.smvCfg.smvSub[i].type == DATA_SRC_ADC_REPEAT)
        {
            /* 复采通道 */
            sFpgaCfg.adParaRepeat.bUsed = TRUE;

            /* A/D复采端口号固定为1 */
            sFpgaCfg.adParaRepeat.ucPortNo = sXmlCfg.smvCfg.smvSub[i].port;
            assert (sFpgaCfg.adParaRepeat.ucPortNo == AD_REPEAT_PORT_NO);

            /* 按通道配置调相角 */
            for (j = 0; j<sXmlCfg.smvCfg.smvSub[i].chnNum; j++)
            {
                sFpgaCfg.adParaRepeat.attrReg_un[sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel-1].attrReg_st.phaseShift
                    = sXmlCfg.smvCfg.smvSub[i].chnSub[j].phase;
            }

            /* 如果配置则使用配置参数,否则使用默认参数 */
            if (sXmlCfg.smvCfg.smvSub[i].bRdDlyCfg)
            {
                /* 滤波延时设置,所有通道一致 */
                sFpgaCfg.adParaRepeat.ctrlReg_un.ctrlReg_st.rcDelay
                    = sXmlCfg.smvCfg.smvSub[i].rcDly;

                sFpgaCfg.adParaRepeat.bRdDlyCfg = TRUE;
            }
            else
            {
                sFpgaCfg.adParaRepeat.bRdDlyCfg = FALSE;
            }

            /* 错误标志,用于清除 */
            sFpgaCfg.adParaRepeat.stsReg_un.stsReg_st.overflowFlag = 1;

            /* 地址赋值 */
            sFpgaCfg.adParaRepeat.pctrlRegAddr = FPGA_MEM_ADRS+AD_DUP_REG_BASE+AD_CTRL_REG_0;
            sFpgaCfg.adParaRepeat.pstsRegAddr = FPGA_MEM_ADRS+AD_DUP_REG_BASE+AD_STATUS_REG_0;
            sFpgaCfg.adParaRepeat.pAttrRegAddr = FPGA_MEM_ADRS+AD_DUP_REG_BASE+AD_ATTR_REG;

            /* 有效标志 */
            smvData.ucAllSrcValidFlag |= 0x01<<AD_REPEAT_PORT_NO;

            /* 存储地址偏移 */
            sFpgaCfg.adParaRepeat.ucDataPosOff = FPGA_AD_SAM_CHN_NUM;

            /* 最大通道内序 */
            for (j = 0; j<sXmlCfg.smvCfg.smvSub[i].chnNum; j++)
            {
                if ((sFpgaCfg.adParaRepeat.ucMaxChnNum<sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel)
                        && (sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel != 255))
                {
                    sFpgaCfg.adParaRepeat.ucMaxChnNum = sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel;
                }
            }

            /* 压板号清零 */
            for (j = 0; j<sFpgaCfg.adParaRepeat.ucMaxChnNum; j++)
            {
                sFpgaCfg.adParaRepeat.iYabanNum[j] = -1;
                sFpgaCfg.adParaRepeat.bRtYabanValue[j] = TRUE;
            }

            /* 设置压板号,按采样通道排列 */
            for (j = 0; j<sXmlCfg.smvCfg.smvSub[i].chnNum; j++)
            {
                if (sXmlCfg.smvCfg.smvSub[i].chnSub[j].MuLinkUSE
                        && (sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel <= FPGA_AD_REPEAT_SAM_CHN_NUM))
                {

                    sFpgaCfg.adParaRepeat.iYabanNum[sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel-1]
                        = sXmlCfg.smvCfg.smvSub[i].chnSub[j].iYabanNum;
                }
                else
                {
                    sFpgaCfg.adParaRepeat.iYabanNum[sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel-1] = -1;
                }
            }

            /* 通道属性 */
            for (j = 0; j<sFpgaCfg.adParaRepeat.ucMaxChnNum; j++)
            {
                *(sFpgaCfg.adParaRepeat.pAttrRegAddr+j)
                    = sFpgaCfg.adParaRepeat.attrReg_un[j].ulAttrReg;
            }
        }
        else if ((sXmlCfg.smvCfg.smvSub[i].type == DATA_SRC_STDFT3)
                 || (sXmlCfg.smvCfg.smvSub[i].type == DATA_SRC_GRDFT3))
        {
            /* FT3配置 */

            static uint8_t ucPortCnt = 0;  /* 端口计数 */

            sFpgaCfg.ft3Para[ucPortCnt].bUsed = TRUE;

            sFpgaCfg.ft3Para[ucPortCnt].ucPortNo = sXmlCfg.smvCfg.smvSub[i].port;
            assert ((sFpgaCfg.ft3Para[ucPortCnt].ucPortNo == FT3_PORT_NO_1)
                    || (sFpgaCfg.ft3Para[ucPortCnt].ucPortNo == FT3_PORT_NO_2));

            /* 两路FT3配置应一致,不一致则报错
             */
            if (sFpgaCfg.bFt3CfgFlag)
            {
                if (sXmlCfg.smvCfg.smvSub[i].type != sFpgaCfg.ctrlReg0_un.ctrlReg0_st.FT3Type)
                {
                    assert(FALSE);
                }
            }
            sFpgaCfg.ctrlReg0_un.ctrlReg0_st.FT3Type = sXmlCfg.smvCfg.smvSub[i].type;
            sFpgaCfg.bFt3CfgFlag = TRUE;

            /* 调相角,所有通道一致 */
            sFpgaCfg.ft3Para[ucPortCnt].ctrlReg_un.ctrlReg_st.phaseShift
                = sXmlCfg.smvCfg.smvSub[i].phase;

            /* 错误标志,用于清除 */
            sFpgaCfg.ft3Para[ucPortCnt].stsReg_un.stsReg_st.overflowFlag = 1;
            sFpgaCfg.ft3Para[ucPortCnt].stsReg_un.stsReg_st.crcErr = 1;
            sFpgaCfg.ft3Para[ucPortCnt].stsReg_un.stsReg_st.rangeOut = 1;
            sFpgaCfg.ft3Para[ucPortCnt].stsReg_un.stsReg_st.linkFail = 1;

            /* 延迟时间通道是否有效 */
            sFpgaCfg.ft3Para[ucPortCnt].ctrlReg_un.ctrlReg_st.dTimeValid
                = sXmlCfg.smvCfg.smvSub[i].dTValid ? 1 : 0;

            if (sFpgaCfg.ft3Para[ucPortCnt].ucPortNo == FT3_PORT_NO_1)
            {
                /* 地址赋值 */
                sFpgaCfg.ft3Para[ucPortCnt].pctrlRegAddr = FPGA_MEM_ADRS+FT3_0_REG_BASE+FT3_CTRL_REG;
                sFpgaCfg.ft3Para[ucPortCnt].pstsRegAddr = FPGA_MEM_ADRS+FT3_0_REG_BASE+FT3_STATUS_REG;
                sFpgaCfg.ft3Para[ucPortCnt].reg.pReg3 = (struct REG3 *)(FPGA_MEM_ADRS+FT3_0_REG_BASE+FT3_STATUS_3_REG);
                sFpgaCfg.ft3Para[ucPortCnt].reg.pReg4 = (struct REG4 *)(FPGA_MEM_ADRS+FT3_0_REG_BASE+FT3_STATUS_4_REG);
            }
            else if (sFpgaCfg.ft3Para[ucPortCnt].ucPortNo == FT3_PORT_NO_2)
            {
                /* 地址赋值 */
                sFpgaCfg.ft3Para[ucPortCnt].pctrlRegAddr = FPGA_MEM_ADRS+FT3_1_REG_BASE+FT3_CTRL_REG;
                sFpgaCfg.ft3Para[ucPortCnt].pstsRegAddr = FPGA_MEM_ADRS+FT3_1_REG_BASE+FT3_STATUS_REG;
                sFpgaCfg.ft3Para[ucPortCnt].reg.pReg3 = (struct REG3 *)(FPGA_MEM_ADRS+FT3_1_REG_BASE+FT3_STATUS_3_REG);
                sFpgaCfg.ft3Para[ucPortCnt].reg.pReg4 = (struct REG4 *)(FPGA_MEM_ADRS+FT3_1_REG_BASE+FT3_STATUS_4_REG);
            }


            smvData.ucAllSrcValidFlag |= 0x01<<sFpgaCfg.ft3Para[ucPortCnt].ucPortNo;

            /* 存储地址偏移 */
            sFpgaCfg.ft3Para[ucPortCnt].ucDataPosOff
                = FPGA_AD_SAM_CHN_NUM+FPGA_AD_REPEAT_SAM_CHN_NUM+FPGA_FT3_SAM_CHN_NUM*(sFpgaCfg.ft3Para[ucPortCnt].ucPortNo-1);

            /* 最大通道内序 */
            for (j = 0; j<sXmlCfg.smvCfg.smvSub[i].chnNum; j++)
            {
                if ((sFpgaCfg.ft3Para[ucPortCnt].ucMaxChnNum<sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel)
                        && (sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel != 255))
                {
                    sFpgaCfg.ft3Para[ucPortCnt].ucMaxChnNum = sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel;
                }
            }

            /* 压板号清零 */
            for (j = 0; j<sFpgaCfg.ft3Para[ucPortCnt].ucMaxChnNum; j++)
            {
                sFpgaCfg.ft3Para[ucPortCnt].iYabanNum[j] = -1;
                sFpgaCfg.ft3Para[ucPortCnt].bRtYabanValue[j] = TRUE;
            }

            /* 设置压板号,按采样通道排列 */
            for (j = 0; j<sXmlCfg.smvCfg.smvSub[i].chnNum; j++)
            {
                if (sXmlCfg.smvCfg.smvSub[i].chnSub[j].MuLinkUSE
                        && (sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel <= FPGA_FT3_SAM_CHN_NUM))
                {

                    sFpgaCfg.ft3Para[ucPortCnt].iYabanNum[sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel-1]
                        = sXmlCfg.smvCfg.smvSub[i].chnSub[j].iYabanNum;
                }
                else
                {
                    sFpgaCfg.ft3Para[ucPortCnt].iYabanNum[sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel-1] = -1;
                }
            }

            ucPortCnt++;
        }
        else if (sXmlCfg.smvCfg.smvSub[i].type == DATA_SRC_SMV)
        {
            /* 9-2接收 */
            static uint8_t ucPortCnt = 0;  /* 端口计数 */

            sFpgaCfg.svRcvPara[ucPortCnt].bUsed = TRUE;

            sFpgaCfg.svRcvPara[ucPortCnt].ucPortNo = sXmlCfg.smvCfg.smvSub[i].port;
            assert ((sFpgaCfg.svRcvPara[ucPortCnt].ucPortNo == SMV_PORT_NO_1)
                    || (sFpgaCfg.svRcvPara[ucPortCnt].ucPortNo == SMV_PORT_NO_2)
                   );

            /* 调相角,所有通道一致 */
            sFpgaCfg.svRcvPara[ucPortCnt].ctrlReg_un.ctrlReg_st.phaseShift
                = sXmlCfg.smvCfg.smvSub[i].phase;

            /* 延迟时间通道 */
            sFpgaCfg.svRcvPara[ucPortCnt].ctrlReg_un.ctrlReg_st.dTChn
                = sXmlCfg.smvCfg.smvSub[i].dTChn-1;

            /* 延迟时间通道是否有效 */
            sFpgaCfg.svRcvPara[ucPortCnt].ctrlReg_un.ctrlReg_st.dTimeValid
                = sXmlCfg.smvCfg.smvSub[i].dTValid ? 1 : 0;

            /* MAC地址 */
            sFpgaCfg.svRcvPara[ucPortCnt].macReg0_un.macReg0_st.byte5
                = sXmlCfg.smvCfg.smvSub[i].macAdr[5];
            sFpgaCfg.svRcvPara[ucPortCnt].macReg0_un.macReg0_st.byte4
                = sXmlCfg.smvCfg.smvSub[i].macAdr[4];
            sFpgaCfg.svRcvPara[ucPortCnt].macReg0_un.macReg0_st.byte3
                = sXmlCfg.smvCfg.smvSub[i].macAdr[3];
            sFpgaCfg.svRcvPara[ucPortCnt].macReg0_un.macReg0_st.byte2
                = sXmlCfg.smvCfg.smvSub[i].macAdr[2];
            sFpgaCfg.svRcvPara[ucPortCnt].macReg1_un.macReg1_st.byte1
                = sXmlCfg.smvCfg.smvSub[i].macAdr[1];
            sFpgaCfg.svRcvPara[ucPortCnt].macReg1_un.macReg1_st.byte0
                = sXmlCfg.smvCfg.smvSub[i].macAdr[0];

            /* 错误标志,状态寄存器0,用于清除 */
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg0_un.stsReg0_st.overflowFlag = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg0_un.stsReg0_st.linkFail = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg0_un.stsReg0_st.rangeOut = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg0_un.stsReg0_st.decodeErr = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg0_un.stsReg0_st.crcErr = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg0_un.stsReg0_st.lenErr = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg0_un.stsReg0_st.noErr = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg0_un.stsReg0_st.rcvErr = 1;

            /* 状态寄存器1 */
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.syn = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.test = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn0Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn1Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn2Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn3Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn4Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn5Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn6Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn7Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn8Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn9Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn10Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn11Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn12Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn13Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn14Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn15Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn16Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn17Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn18Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn19Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn20Valid = 1;
            sFpgaCfg.svRcvPara[ucPortCnt].stsReg1_un.stsReg1_st.chn21Valid = 1;

            if (sFpgaCfg.svRcvPara[ucPortCnt].ucPortNo == SMV_PORT_NO_1)
            {
                /* 地址赋值 */
                sFpgaCfg.svRcvPara[ucPortCnt].pctrlRegAddr = FPGA_MEM_ADRS+SV_R_0_REG_BASE+SMV_CTRL_REG;
                sFpgaCfg.svRcvPara[ucPortCnt].pmacReg0Addr = FPGA_MEM_ADRS+SV_R_0_REG_BASE+SMV_MAC0_REG;
                sFpgaCfg.svRcvPara[ucPortCnt].pmacReg1Addr = FPGA_MEM_ADRS+SV_R_0_REG_BASE+SMV_MAC1_REG;
                sFpgaCfg.svRcvPara[ucPortCnt].psvRecvSts0RegAddr = FPGA_MEM_ADRS+SV_R_0_REG_BASE+SMV_STATUS0_REG;
                sFpgaCfg.svRcvPara[ucPortCnt].psvRecvSts1RegAddr = FPGA_MEM_ADRS+SV_R_0_REG_BASE+SMV_STATUS1_REG;
                sFpgaCfg.svRcvPara[ucPortCnt].psvRecvSts2RegAddr = FPGA_MEM_ADRS+SV_R_0_REG_BASE+SMV_STATUS2_REG;
            }
            else if (sFpgaCfg.svRcvPara[ucPortCnt].ucPortNo == SMV_PORT_NO_2)
            {
                /* 地址赋值 */
                sFpgaCfg.svRcvPara[ucPortCnt].pctrlRegAddr = FPGA_MEM_ADRS+SV_R_1_REG_BASE+SMV_CTRL_REG;
                sFpgaCfg.svRcvPara[ucPortCnt].pmacReg0Addr = FPGA_MEM_ADRS+SV_R_1_REG_BASE+SMV_MAC0_REG;
                sFpgaCfg.svRcvPara[ucPortCnt].pmacReg1Addr = FPGA_MEM_ADRS+SV_R_1_REG_BASE+SMV_MAC1_REG;
                sFpgaCfg.svRcvPara[ucPortCnt].psvRecvSts0RegAddr = FPGA_MEM_ADRS+SV_R_1_REG_BASE+SMV_STATUS0_REG;
                sFpgaCfg.svRcvPara[ucPortCnt].psvRecvSts1RegAddr = FPGA_MEM_ADRS+SV_R_1_REG_BASE+SMV_STATUS1_REG;
                sFpgaCfg.svRcvPara[ucPortCnt].psvRecvSts2RegAddr = FPGA_MEM_ADRS+SV_R_1_REG_BASE+SMV_STATUS2_REG;
            }

            /* FT3端口号和以太网端口号有重叠
             * 作一偏移处理
             */
            smvData.ucAllSrcValidFlag |= 0x01<<(sFpgaCfg.svRcvPara[ucPortCnt].ucPortNo);

            /* 存储地址偏移 */
            sFpgaCfg.svRcvPara[ucPortCnt].ucDataPosOff
                = FPGA_AD_SAM_CHN_NUM+FPGA_AD_REPEAT_SAM_CHN_NUM+FPGA_FT3_SAM_CHN_NUM*FT3_PORT_NUM
                  +FPGA_SV_RCV_CHN_NUM*(sFpgaCfg.svRcvPara[ucPortCnt].ucPortNo-SMV_PORT_NO_1);

            /* 最大通道内序 */
            for (j = 0; j<sXmlCfg.smvCfg.smvSub[i].chnNum; j++)
            {
                if ((sFpgaCfg.svRcvPara[ucPortCnt].ucMaxChnNum<sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel)
                        && (sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel != 255))
                {
                    sFpgaCfg.svRcvPara[ucPortCnt].ucMaxChnNum = sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel;
                }
            }

            /* 压板号清零 */
            for (j = 0; j<sFpgaCfg.svRcvPara[ucPortCnt].ucMaxChnNum; j++)
            {
                sFpgaCfg.svRcvPara[ucPortCnt].iYabanNum[j] = -1;
                sFpgaCfg.svRcvPara[ucPortCnt].bRtYabanValue[j] = TRUE;
            }

            /* 设置压板号,按采样通道排列 */
            for (j = 0; j<sXmlCfg.smvCfg.smvSub[i].chnNum; j++)
            {
                if (sXmlCfg.smvCfg.smvSub[i].chnSub[j].MuLinkUSE
                        && (sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel <= FPGA_SV_RCV_CHN_NUM))
                {

                    sFpgaCfg.svRcvPara[ucPortCnt].iYabanNum[sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel-1]
                        = sXmlCfg.smvCfg.smvSub[i].chnSub[j].iYabanNum;
                }
                else
                {
                    sFpgaCfg.svRcvPara[ucPortCnt].iYabanNum[sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel-1] = -1;
                }
            }

            ucPortCnt++;
        }
        else if(sXmlCfg.smvCfg.smvSub[i].type == DATA_SRC_SUM)
        {
            /*暂无操作*/
        }
        else
        {
            assert (FALSE);
        }
    }

    fpgaSmvSendInfoCreate();/*FT3、9-2发送配置*/


    /* 数据源计数周期 */
    smvData.usSrcCntCyle = SMP_RATE_PER_CYCLE*uiPwrFreq_g;

    /* 查询缓冲计数 */
    smvData.ucBufCnt = 0;

    return TRUE;
}


/* 初始化FPGA.
 * Para:
 *     NONE.
 * Return:
 *     OK, ERROR.
 */
void fpgaInit(void)
{
    int i = 0;
    int j = 0;
    int m = 0;
    int k;
    uint8_t *pTemp = NULL;
    uint8_t ucTmp = 0; /* 计算偏移 */
    uint8_t ulCount = 0;
    uint8_t ulCountAll = 0;
    uint8_t *pASDUOff = NULL;  /* ASDU偏移地址 */
    uint8_t *pAllLenOff = NULL;  /* 总长度偏移 */
    uint8_t *pAPDULenOff = NULL;  /* APDU偏移 */
    uint8_t *pSepAPDULenOff = NULL;  /* seqAPDU偏移 */
    uint16_t usTmpVal = 0;
    uint16_t usTmpValT = 0;
    int iWordNum = 0;

    /* 通用寄存器设置 */
    *sFpgaCfg.pctrlReg0Addr = sFpgaCfg.ctrlReg0_un.ulCtrlReg0;
    sFpgaCfg.stsReg0_un.ulStsReg0 = *sFpgaCfg.pstsReg0Addr;
    sFpgaCfg.stsReg1_un.ulStsReg1 = *sFpgaCfg.pstsReg1Addr;

    /* A/D采样相关FPGA寄存器设置 */
    if (sFpgaCfg.adPara.bUsed)
    {
        /* 保存延时 */
        if (sFpgaCfg.adPara.bRdDlyCfg)
        {
            usTmpValT = sFpgaCfg.adPara.ctrlReg_un.ctrlReg_st.rcDelay;
        }

        sFpgaCfg.adPara.ctrlReg_un.ulCtrlReg = *sFpgaCfg.adPara.pctrlRegAddr;

        /* 写入延时 */
        if (sFpgaCfg.adPara.bRdDlyCfg)
        {
            sFpgaCfg.adPara.ctrlReg_un.ctrlReg_st.rcDelay = usTmpValT;
        }

        *sFpgaCfg.adPara.pctrlRegAddr = sFpgaCfg.adPara.ctrlReg_un.ulCtrlReg;

        /* 最大BUF数为0则使用缺省值,用于调试 */
        if (sFpgaCfg.adPara.ctrlReg_un.ctrlReg_st.maxDataBufNum == 0)
        {
            sFpgaCfg.adPara.ctrlReg_un.ctrlReg_st.maxDataBufNum = MAX_BD_NUM;
        }
        sFpgaCfg.maxDataBufNum = sFpgaCfg.adPara.ctrlReg_un.ctrlReg_st.maxDataBufNum;

        /* BD寄存器赋值 */
        for (i = 0; i<sFpgaCfg.adPara.ctrlReg_un.ctrlReg_st.maxDataBufNum; i++)
        {
            sFpgaCfg.adPara.pbdRegAddr[i] = FPGA_MEM_ADRS+AD_REG_BASE+AD_BD_ADDR+i;
            sFpgaCfg.adPara.bdReg_un[i].ulBdReg = *sFpgaCfg.adPara.pbdRegAddr[i];
            sFpgaCfg.adPara.pulDataReg[i] = (volatile struct AD_DATA_REG *)(FPGA_MEM_ADRS+AD_REG_BASE+AD_DATA_BUF+AD_BUF_LEN*i);
        }

        /* 错误标清除 */
        sFpgaCfg.adPara.stsReg_un.stsReg_st.overflowFlag = 1;
        sFpgaCfg.adPara.stsReg_un.stsReg_st.ad1Error = 1;
        sFpgaCfg.adPara.stsReg_un.stsReg_st.ad2Error = 1;
        sFpgaCfg.adPara.stsReg_un.stsReg_st.ad3Error = 1;
        sFpgaCfg.adPara.stsReg_un.stsReg_st.ad4Error = 1;
    }

    /* A/D复采相关FPGA寄存器设置 */
    if (sFpgaCfg.adParaRepeat.bUsed)
    {
        /* 保存延时 */
        if (sFpgaCfg.adParaRepeat.bRdDlyCfg)
        {
            usTmpValT = sFpgaCfg.adParaRepeat.ctrlReg_un.ctrlReg_st.rcDelay;
        }

        sFpgaCfg.adParaRepeat.ctrlReg_un.ulCtrlReg = *sFpgaCfg.adParaRepeat.pctrlRegAddr;

        /* 写入延时 */
        if (sFpgaCfg.adParaRepeat.bRdDlyCfg)
        {
            sFpgaCfg.adParaRepeat.ctrlReg_un.ctrlReg_st.rcDelay = usTmpValT;
        }

        *sFpgaCfg.adParaRepeat.pctrlRegAddr = sFpgaCfg.adParaRepeat.ctrlReg_un.ulCtrlReg;

        /* 最大BUF数为0则使用缺省值,用于调试 */
        if (sFpgaCfg.adParaRepeat.ctrlReg_un.ctrlReg_st.maxDataBufNum == 0)
        {
            sFpgaCfg.adParaRepeat.ctrlReg_un.ctrlReg_st.maxDataBufNum = MAX_BD_NUM;
        }

        /* BD寄存器赋值 */
        for (i = 0; i<sFpgaCfg.adParaRepeat.ctrlReg_un.ctrlReg_st.maxDataBufNum; i++)
        {
            sFpgaCfg.adParaRepeat.pbdRegAddr[i] = FPGA_MEM_ADRS+AD_DUP_REG_BASE+AD_BD_ADDR+i;
            sFpgaCfg.adParaRepeat.bdReg_un[i].ulBdReg = *sFpgaCfg.adParaRepeat.pbdRegAddr[i];
            sFpgaCfg.adParaRepeat.pulDataReg[i] = (volatile struct AD_REPEAT_DATA_REG *)(FPGA_MEM_ADRS+AD_DUP_REG_BASE+AD_DATA_BUF+AD_BUF_LEN*i);
        }

        /* 错误标清除 */
        sFpgaCfg.adParaRepeat.stsReg_un.stsReg_st.overflowFlag = 1;
        sFpgaCfg.adParaRepeat.stsReg_un.stsReg_st.linkError = 1;
        sFpgaCfg.adParaRepeat.stsReg_un.stsReg_st.intervalError = 1;
        sFpgaCfg.adParaRepeat.stsReg_un.stsReg_st.CRCError = 1;
        sFpgaCfg.adParaRepeat.stsReg_un.stsReg_st.ad1Error = 1;
        sFpgaCfg.adParaRepeat.stsReg_un.stsReg_st.ad2Error = 1;
    }

    /* FT3配置 */
    for (i = 0; i<FT3_PORT_NUM; i++)
    {
        if (sFpgaCfg.ft3Para[i].bUsed)
        {
            usTmpVal = sFpgaCfg.ft3Para[i].ctrlReg_un.ctrlReg_st.phaseShift;
            usTmpValT = sFpgaCfg.ft3Para[i].ctrlReg_un.ctrlReg_st.dTimeValid;
            sFpgaCfg.ft3Para[i].ctrlReg_un.ulCtrlReg = *sFpgaCfg.ft3Para[i].pctrlRegAddr;
            sFpgaCfg.ft3Para[i].ctrlReg_un.ctrlReg_st.phaseShift = usTmpVal;
            sFpgaCfg.ft3Para[i].ctrlReg_un.ctrlReg_st.dTimeValid = usTmpValT;
            *sFpgaCfg.ft3Para[i].pctrlRegAddr = sFpgaCfg.ft3Para[i].ctrlReg_un.ulCtrlReg;

            sFpgaCfg.ft3Para[i].ctrlReg_un.ulCtrlReg = *sFpgaCfg.ft3Para[i].pctrlRegAddr;

            /* 最大BUF数为0则使用缺省值,用于调试 */
            if (sFpgaCfg.ft3Para[i].ctrlReg_un.ctrlReg_st.maxDataBufNum == 0)
            {
                sFpgaCfg.ft3Para[i].ctrlReg_un.ctrlReg_st.maxDataBufNum = MAX_BD_NUM;
            }

            /* 最大缓冲块大小必须设定为一致 */
            assert (sFpgaCfg.maxDataBufNum == sFpgaCfg.ft3Para[i].ctrlReg_un.ctrlReg_st.maxDataBufNum);

            *sFpgaCfg.ft3Para[i].pctrlRegAddr = sFpgaCfg.ft3Para[i].ctrlReg_un.ulCtrlReg;
            *sFpgaCfg.ft3Para[i].pstsRegAddr = sFpgaCfg.ft3Para[i].stsReg_un.ulStsReg;

            /* BD寄存器赋值 */
            for (j = 0; j<sFpgaCfg.ft3Para[i].ctrlReg_un.ctrlReg_st.maxDataBufNum; j++)
            {
                if (sFpgaCfg.ft3Para[i].ucPortNo == FT3_PORT_NO_1)
                {
                    sFpgaCfg.ft3Para[i].pbdRegAddr[j] = FPGA_MEM_ADRS+FT3_0_REG_BASE+FT3_BD_ADDR+j;
                    sFpgaCfg.ft3Para[i].bdReg_un[j].ulBdReg = *sFpgaCfg.ft3Para[i].pbdRegAddr[j];
                    sFpgaCfg.ft3Para[i].pulDataReg[j]
                        = (volatile struct FT3_DATA_REG *)(FPGA_MEM_ADRS+FT3_0_REG_BASE+FT3_DATA_BUF+FT3_BUF_LEN*j);
                }
                else if (sFpgaCfg.ft3Para[i].ucPortNo == FT3_PORT_NO_2)
                {
                    sFpgaCfg.ft3Para[i].pbdRegAddr[j] = FPGA_MEM_ADRS+FT3_1_REG_BASE+FT3_BD_ADDR+j;
                    sFpgaCfg.ft3Para[i].bdReg_un[j].ulBdReg = *sFpgaCfg.ft3Para[i].pbdRegAddr[j];
                    sFpgaCfg.ft3Para[i].pulDataReg[j]
                        = (volatile struct FT3_DATA_REG *)(FPGA_MEM_ADRS+FT3_1_REG_BASE+FT3_DATA_BUF+FT3_BUF_LEN*j);
                }
                else
                {
                    assert (FALSE);
                }
            }

            /* 错误标清除 */
            sFpgaCfg.ft3Para[i].stsReg_un.stsReg_st.overflowFlag = 1;
            sFpgaCfg.ft3Para[i].stsReg_un.stsReg_st.linkFail = 1;
            sFpgaCfg.ft3Para[i].stsReg_un.stsReg_st.rangeOut = 1;
            sFpgaCfg.ft3Para[i].stsReg_un.stsReg_st.crcErr = 1;
        }
    }

    /* 9-2配置 */
    for (i = 0; i<SMV_RCV_PORT_NUM; i++)
    {
        if (sFpgaCfg.svRcvPara[i].bUsed)
        {

            usTmpVal = sFpgaCfg.svRcvPara[i].ctrlReg_un.ctrlReg_st.phaseShift;
            usTmpValT = sFpgaCfg.svRcvPara[i].ctrlReg_un.ctrlReg_st.dTimeValid;
            sFpgaCfg.svRcvPara[i].ctrlReg_un.ulCtrlReg = *sFpgaCfg.svRcvPara[i].pctrlRegAddr;
            sFpgaCfg.svRcvPara[i].ctrlReg_un.ctrlReg_st.phaseShift = usTmpVal;
            sFpgaCfg.svRcvPara[i].ctrlReg_un.ctrlReg_st.dTimeValid = usTmpValT;
            *sFpgaCfg.svRcvPara[i].pctrlRegAddr = sFpgaCfg.svRcvPara[i].ctrlReg_un.ulCtrlReg;

            sFpgaCfg.svRcvPara[i].ctrlReg_un.ulCtrlReg = *sFpgaCfg.svRcvPara[i].pctrlRegAddr;

            /* 最大BUF数为0则使用缺省值,用于调试 */
            if (sFpgaCfg.svRcvPara[i].ctrlReg_un.ctrlReg_st.maxDataBufNum == 0)
            {
                sFpgaCfg.svRcvPara[i].ctrlReg_un.ctrlReg_st.maxDataBufNum = MAX_BD_NUM;
            }

            /* 最大缓冲块大小必须设定为一致 */
            assert (sFpgaCfg.maxDataBufNum == sFpgaCfg.svRcvPara[i].ctrlReg_un.ctrlReg_st.maxDataBufNum);

            /* 寄存器设置 */
            *sFpgaCfg.svRcvPara[i].pctrlRegAddr = sFpgaCfg.svRcvPara[i].ctrlReg_un.ulCtrlReg;
            *sFpgaCfg.svRcvPara[i].psvRecvSts0RegAddr = sFpgaCfg.svRcvPara[i].stsReg0_un.ulStsReg0;
            *sFpgaCfg.svRcvPara[i].psvRecvSts1RegAddr = sFpgaCfg.svRcvPara[i].stsReg1_un.ulStsReg1;
            *sFpgaCfg.svRcvPara[i].psvRecvSts2RegAddr = sFpgaCfg.svRcvPara[i].stsReg2_un.ulStsReg2;
            *sFpgaCfg.svRcvPara[i].pmacReg0Addr = sFpgaCfg.svRcvPara[i].macReg0_un.ulMacReg0;
            *sFpgaCfg.svRcvPara[i].pmacReg1Addr = sFpgaCfg.svRcvPara[i].macReg1_un.ulMacReg1;

            /* BD寄存器赋值 */
            for (j = 0; j<sFpgaCfg.svRcvPara[i].ctrlReg_un.ctrlReg_st.maxDataBufNum; j++)
            {
                if (sFpgaCfg.svRcvPara[i].ucPortNo == SMV_PORT_NO_1)
                {
                    sFpgaCfg.svRcvPara[i].pbdRegAddr[j] = FPGA_MEM_ADRS+SV_R_0_REG_BASE+SMV_BD_ADDR+j;
                    sFpgaCfg.svRcvPara[i].bdReg_un[j].ulBdReg = *sFpgaCfg.svRcvPara[i].pbdRegAddr[j];
                    sFpgaCfg.svRcvPara[i].pulDataReg[j] =
                        (volatile struct SV_RCV_DATA_REG *)(FPGA_MEM_ADRS+SV_R_0_REG_BASE+SMV_DATA_BUF+SMV_BUF_LEN*j);
                }
                else if (sFpgaCfg.svRcvPara[i].ucPortNo == SMV_PORT_NO_2)
                {
                    sFpgaCfg.svRcvPara[i].pbdRegAddr[j] = FPGA_MEM_ADRS+SV_R_1_REG_BASE+SMV_BD_ADDR+j;
                    sFpgaCfg.svRcvPara[i].bdReg_un[j].ulBdReg = *sFpgaCfg.svRcvPara[i].pbdRegAddr[j];
                    sFpgaCfg.svRcvPara[i].pulDataReg[j] =
                        (volatile struct SV_RCV_DATA_REG *)(FPGA_MEM_ADRS+SV_R_1_REG_BASE+SMV_DATA_BUF+SMV_BUF_LEN*j);
                }
                else
                {
                    assert (FALSE);
                }
            }

            /* 错误标志,状态寄存器0,用于清除 */
            sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.overflowFlag = 1;
            sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.linkFail = 1;
            sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.rangeOut = 1;
            sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.decodeErr = 1;
            sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.crcErr = 1;
            sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.lenErr = 1;
            sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.noErr = 1;
            sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.rcvErr = 1;

            /* 状态寄存器1 */
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.syn = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.test = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn0Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn1Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn2Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn3Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn4Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn5Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn6Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn7Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn8Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn9Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn10Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn11Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn12Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn13Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn14Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn15Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn16Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn17Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn18Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn19Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn20Valid = 1;
            sFpgaCfg.svRcvPara[i].stsReg1_un.stsReg1_st.chn21Valid = 1;
        }
    }

    UpdateFPGASmvSendCoff();

    /* SV配置 */
    if (sFpgaCfg.svSndPara.bUsed)
    {
        /* 填写标注9-2数据帧框架 */
        pTemp = (uint8_t *)(sFpgaCfg.svSndPara.ucEthFrame);

        *pTemp++ = sFpgaCfg.svSndPara.ucDestMacAddr[0];
        *pTemp++ = sFpgaCfg.svSndPara.ucDestMacAddr[1];
        *pTemp++ = sFpgaCfg.svSndPara.ucDestMacAddr[2];
        *pTemp++ = sFpgaCfg.svSndPara.ucDestMacAddr[3];
        *pTemp++ = sFpgaCfg.svSndPara.ucDestMacAddr[4];
        *pTemp++ = sFpgaCfg.svSndPara.ucDestMacAddr[5];

        *pTemp++ = sFpgaCfg.svSndPara.ucSrcMacAddr[0];
        *pTemp++ = sFpgaCfg.svSndPara.ucSrcMacAddr[1];
        *pTemp++ = sFpgaCfg.svSndPara.ucSrcMacAddr[2];
        *pTemp++ = sFpgaCfg.svSndPara.ucSrcMacAddr[3];
        *pTemp++ = sFpgaCfg.svSndPara.ucSrcMacAddr[4];
        *pTemp++ = sFpgaCfg.svSndPara.ucSrcMacAddr[5];

        /* TPID */
        *pTemp++ = 0x81;
        *pTemp++ = 0x00;

        /* TCI */
        *pTemp++ = (sFpgaCfg.svSndPara.TCI >> 8) & 0xFF;
        *pTemp++ = (sFpgaCfg.svSndPara.TCI >> 0) & 0xFF;

        /* ETHERNET TYPE */
        *pTemp++ = 0x88;
        *pTemp++ = 0xBA;

        /* APPID */
        *pTemp++ = (sFpgaCfg.svSndPara.APPID >> 8) & 0xFF;
        *pTemp++ = (sFpgaCfg.svSndPara.APPID >> 0) & 0xFF;

        pAllLenOff = pTemp; /* 总长度偏移 */

        /* 总长度 */
        *pTemp++ = HI8(sFpgaCfg.svSndPara.AllLength);
        *pTemp++ = LO8(sFpgaCfg.svSndPara.AllLength);

        /* RESERVED 1 */
        *pTemp++ = 0x00;
        *pTemp++ = 0x00;

        /* RESERVED 2 */
        *pTemp++ = 0x00;
        *pTemp++ = 0x00;

        /* APDU TAG, APDU LENGTH, 2 bytes */
        *pTemp++ = 0x60;
        *pTemp++ = 0x81;

        pAPDULenOff = pTemp; /* APDU偏移 */
        *pTemp++ = sFpgaCfg.svSndPara.APDU_len;

        *pTemp++ = 0x80; /* ASDU NO TAG */
        *pTemp++ = 0x01;  /* 长度 */
        *pTemp++ = sFpgaCfg.svSndPara.noASDU;	/* ASDU NO */

        *pTemp++ = 0xA2;   /* ASDU SEQ TAG, 2 bytes */
        *pTemp++ = 0x81;

        pSepAPDULenOff = pTemp; /* seq ASDU偏移 */
        *pTemp++ = sFpgaCfg.svSndPara.seqASDU_len;

        /* 最大ASDU限制 */
        assert (sFpgaCfg.svSndPara.noASDU <= MAX_ASDU_NUM);
        for (m = 0; m<sFpgaCfg.svSndPara.noASDU; m++)
        {
            /* ASDU TAG */
            *pTemp++ = 0x30;
            *pTemp++ = 0x81;
            pASDUOff = pTemp; /* ASDU偏移 */
            *pTemp++ = sFpgaCfg.svSndPara.ASDU_len;	/* ASDU LENGTH */

            *pTemp++ = 0x80;	/* SVID TAG,固定为3个字节 */
            *pTemp++ = sFpgaCfg.svSndPara.strLen;	/* SVID LENGTH */

            for (k = 0; k<sFpgaCfg.svSndPara.strLen; k++)	/* SVID STRING */
            {
                *pTemp++ = sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubAddress.svID[k];
            }

            /* 保证SMPCNT TAGz在4字节位置上 */
            ucTmp = (pTemp-((uint8_t *)(sFpgaCfg.svSndPara.ucEthFrame)))%4;
            if (ucTmp == 1)
            {
                *pTemp++ = 0x81;	/* dataset TAG */
                *pTemp++ = 0x01;	/* dataset LENGTH */
                *pTemp++ =0x00;
                ulCount = 3;
            }
            else if (ucTmp == 2)
            {
                *pTemp++ = 0x81;	/* dataset TAG */
                *pTemp++ = 0x00;	/* dataset LENGTH */
                ulCount = 2;
            }
            else if (ucTmp == 3)
            {
                *pTemp++ = 0x81;	/* dataset TAG */
                *pTemp++ = 0x03;	/* dataset LENGTH */
                *pTemp++ = 0x00;
                *pTemp++ = 0x00;
                *pTemp++ = 0x00;
                ulCount = 5;
            }

            *pASDUOff = sFpgaCfg.svSndPara.ASDU_len+ulCount;

            /* 累加填充字节 */
            ulCountAll += ulCount;

            sFpgaCfg.svSndPara.chnLen.lenReg_st.cntOff
                = (pTemp-((uint8_t *)(sFpgaCfg.svSndPara.ucEthFrame)))/4+SV_DATA_BUF;

            *pTemp++ = 0x82;	/* SMPCNT TAG */
            *pTemp++ = 0x02;	/* SMPCNT LENGTH */
            *pTemp++ = 0x00;	/* SMPCNT */
            *pTemp++ = 0x00;

            *pTemp++ = 0x83;	/* REV TAG */
            *pTemp++ = 0x04;	/* REV LENGTH */
            *pTemp++ = HH8(sFpgaCfg.svSndPara.confRev);	/* REV */
            *pTemp++ = HL8(sFpgaCfg.svSndPara.confRev);
            *pTemp++ = LH8(sFpgaCfg.svSndPara.confRev);
            *pTemp++ = LL8(sFpgaCfg.svSndPara.confRev);

            *pTemp++ = 0x85;	/* SYN TAG */
            *pTemp++ = 0x01;	/* SYN LENGTH */
            *pTemp++ = 0x00;	/* SYN */

            /* 处理周波采样率 */
            *pTemp++ = 0x86;	/* smpRate tag */
            *pTemp++ = 0x02;	/* smpRate len */
            *pTemp++ = 0x00;	/* smpRate value */
            *pTemp++ = 0x50;

            *pTemp++ = 0x87;	/* ASDU DATA TAG */
            *pTemp++ = 0x81;	/* ASDU DATA TAG */
            *pTemp++ = sFpgaCfg.svSndPara.seqData_len;	/* ASDU DATA LENGTH */

            for (k = 0; k<sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubDataset.chnNum; k++)
            {
                if (sXmlCfg.smvCfg.smvPub[sXmlCfg.smvCfg.uc9_2Sn].pubDataset.chnPub[k].delayFlag == 2)   /* mu delay */
                {
                    *pTemp++ = 0x00;
                    *pTemp++ = 0x00;
                    *pTemp++ = LH8(0x00);
                    *pTemp++ = LL8(0x00);
                    pTemp += 3;
                    *pTemp++ = 0x00;
                }
                else   /* 其它采样通道 */
                {
                    pTemp += 7;
                    *pTemp++ = 0x01;
                }
            }
        }

        sFpgaCfg.svSndPara.AllLength += ulCountAll;
        sFpgaCfg.svSndPara.APDU_len += ulCountAll;
        sFpgaCfg.svSndPara.seqASDU_len += ulCountAll;

        /* 总长度 */
        *pAllLenOff++ = HI8(sFpgaCfg.svSndPara.AllLength);
        *pAllLenOff = LO8(sFpgaCfg.svSndPara.AllLength);
        *pAPDULenOff = sFpgaCfg.svSndPara.APDU_len;
        *pSepAPDULenOff = sFpgaCfg.svSndPara.seqASDU_len;

        /* 9-2帧长度 */
        sFpgaCfg.svSndPara.chnLen.lenReg_st.len = sFpgaCfg.svSndPara.AllLength+ETH_HEAD_LEN;
        *sFpgaCfg.svSndPara.pLenRegAddr = sFpgaCfg.svSndPara.chnLen.ulLenReg;

        /* 填写FPGA */
        iWordNum = (sFpgaCfg.svSndPara.chnLen.lenReg_st.len+3)/4;  /* 4字节数 */
        for (i = 0; i<iWordNum; i++)
        {
            sFpgaCfg.svSndPara.pFrameRegAddr[i] = sFpgaCfg.svSndPara.ucEthFrame[i];
        }

        /* 端口使能 */
        *sFpgaCfg.svSndPara.pportEnbRegAddr = sFpgaCfg.svSndPara.portEnb.ulPortEnbReg;
    }

    /* FT3配置 */
    if (sFpgaCfg.ft3SndPara.bUsed)
    {
        /* 填写标准FT3数据帧框架 */
        pTemp = (uint8_t *)(sFpgaCfg.ft3SndPara.ucFt3Frame);

        if (sFpgaCfg.ft3SndPara.type == SMV_SEND_STDFT3)
        {
            sFpgaCfg.ft3SndPara.len = 0x002C;

            /* Frame Length */
            *pTemp++ = HI8(sFpgaCfg.ft3SndPara.len);
            *pTemp++ = LO8(sFpgaCfg.ft3SndPara.len);

            /* LNName */
            *pTemp++ = 0x02;

            /* DataSetName */
            *pTemp++ = sFpgaCfg.ft3SndPara.DSName;

            /* LDName */
            *pTemp++ = HI8(sFpgaCfg.ft3SndPara.LDName);
            *pTemp++ = LO8(sFpgaCfg.ft3SndPara.LDName);

            /* phsA.Artg */
            *pTemp++ = HI8(sFpgaCfg.ft3SndPara.rtdPhsCur);
            *pTemp++ = LO8(sFpgaCfg.ft3SndPara.rtdPhsCur);

            /* Neut.Artg */
            *pTemp++ = HI8(sFpgaCfg.ft3SndPara.rtdNeucur);
            *pTemp++ = LO8(sFpgaCfg.ft3SndPara.rtdNeucur);

            /* PhsA.Vrtg */
            *pTemp++ = HI8(sFpgaCfg.ft3SndPara.rtdPhsVol);
            *pTemp++ = LO8(sFpgaCfg.ft3SndPara.rtdPhsVol);

            /* tdr */
            *pTemp++ = HI8(sFpgaCfg.ft3SndPara.rtdDlyTime);
            *pTemp++ = LO8(sFpgaCfg.ft3SndPara.rtdDlyTime);

            /* Reserved */
            *pTemp++ = 0x00;
            *pTemp++ = 0x00;

            /* 12通道 */
            for (i = 0; i<FT3_STD_MAX_CHN_NUM; i++)
            {
                *pTemp++ = 0x00;
                *pTemp++ = 0x00;
            }

            /* Status Word #1 */
            *pTemp++ = 0x00;
            *pTemp++ = 0x00;

            /* Status Word #2 */
            *pTemp++ = 0x00;
            *pTemp++ = 0x00;

            /* SampCnt */
            *pTemp++ = 0x00;
            *pTemp++ = 0x00;

            /* reserved */
            *pTemp++ = 0x00;
            *pTemp++ = 0x00;

        }
        else if (sFpgaCfg.ft3SndPara.type == SMV_SEND_GRDFT3)
        {
            /* 国网FT3 */
            sFpgaCfg.ft3SndPara.len = 0x003E;

            /* Frame Length */
            *pTemp++ = HI8(sFpgaCfg.ft3SndPara.len);
            *pTemp++ = LO8(sFpgaCfg.ft3SndPara.len);

            /* LNName */
            *pTemp++ = 0x02;

            /* DataSetName */
            *pTemp++ = sFpgaCfg.ft3SndPara.DSName;

            /* LDName */
            *pTemp++ = HI8(sFpgaCfg.ft3SndPara.LDName);
            *pTemp++ = LO8(sFpgaCfg.ft3SndPara.LDName);

            /* phsA.Artg */
            *pTemp++ = HI8(sFpgaCfg.ft3SndPara.rtdPhsCur);
            *pTemp++ = LO8(sFpgaCfg.ft3SndPara.rtdPhsCur);

            /* Neut.Artg */
            *pTemp++ = HI8(sFpgaCfg.ft3SndPara.rtdNeucur);
            *pTemp++ = LO8(sFpgaCfg.ft3SndPara.rtdNeucur);

            /* PhsA.Vrtg */
            *pTemp++ = HI8(sFpgaCfg.ft3SndPara.rtdPhsVol);
            *pTemp++ = LO8(sFpgaCfg.ft3SndPara.rtdPhsVol);

            /* tdr */
            *pTemp++ = HI8(sFpgaCfg.ft3SndPara.rtdDlyTime);
            *pTemp++ = LO8(sFpgaCfg.ft3SndPara.rtdDlyTime);

            /* SampCnt */
            *pTemp++ = 0x00;
            *pTemp++ = 0x00;

            /* 22通道 */
            for (i = 0; i<FT3_GRD_MAX_CHN_NUM; i++)
            {
                *pTemp++ = 0x00;
                *pTemp++ = 0x00;
            }

            /* Status Word #1 */
            *pTemp++ = 0x00;
            *pTemp++ = 0x00;

            /* Status Word #2 */
            *pTemp++ = 0x00;
            *pTemp++ = 0x00;
        }
        else
        {
            assert (FALSE);
        }

        /* 填写FPGA */
        iWordNum = (sFpgaCfg.ft3SndPara.len+3)/4;  /* 4字节数 */
        for (i = 0; i<iWordNum; i++)
        {
            sFpgaCfg.ft3SndPara.pFrameRegAddr[i] = sFpgaCfg.ft3SndPara.ucFt3Frame[i];
        }
    }
}

/* 同步信号状态变化判断.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void fpgaSynChgSts(void)
{
    static char ucTmpStr[TEMP_INFO_MAX_LEN];

    /* 本地FPGA晶振计数产生,不考虑同步信号状态变化 */
    if (sXmlCfg.smvCfg.intelPulse != INTERNAL_FPGA_CRYS)
    {
        /* 读取同步标志 */
        sFpgaCfg.stsSecPulseReg_un.ulSecPulseStsReg = *sFpgaCfg.pstsSecPulseRegAddr;

        /* 同步信号有效 */
        if (sFpgaCfg.stsSecPulseReg_un.stsSecPulseReg_st.HasPulse
                && sFpgaCfg.stsSecPulseReg_un.stsSecPulseReg_st.PosPulse
                && sFpgaCfg.stsSecPulseReg_un.stsSecPulseReg_st.Period
           )
        {
            /* 还没有置同步正常标志,或者处于守时恢复状态 */
            if ((!sFpgaCfg.stsSecPulseReg_un.stsSecPulseReg_st.Syn)
                    || (sFpgaCfg.stsSecPulseReg_un.stsSecPulseReg_st.Syn
                        && sFpgaCfg.stsSecPulseReg_un.stsSecPulseReg_st.Pnct))
            {
                smvData.bSynChgFlag = TRUE;
            }
            else
            {
                smvData.bSynChgFlag = FALSE;
            }
        }
        else
        {
            smvData.bSynChgFlag = FALSE;
        }
    }
    else
    {
        smvData.bSynChgFlag = FALSE;
    }

    /* 通用状态寄存器读取 */
    sFpgaCfg.stsReg1_un.ulStsReg1 = *sFpgaCfg.pstsReg1Addr;

    /* 回推点变化记录 */
    if (smvData.ucBackPoint != sFpgaCfg.stsReg1_un.stsReg1_st.backPoint)
    {
        static uint32_t ulChgCnt = 0;

        if (ulChgCnt<CHG_MAX_REC_NUM)
        {
            sprintf(ucTmpStr, "回推点变化,当前回推点数%d,上一回推点数%d,插值回推时间%dus\n",
                    sFpgaCfg.stsReg1_un.stsReg1_st.backPoint, smvData.ucBackPoint,
                    sFpgaCfg.stsReg1_un.stsReg1_st.delaytime);
            LOG_Write(LOG_KERNEL, ucTmpStr, NULL);
        }
        ulChgCnt++;

        smvData.ucBackPoint = sFpgaCfg.stsReg1_un.stsReg1_st.backPoint;

        /* 如果对时状态没有变化,则看回推点有没有变化 */
        smvData.bSynChgFlag = TRUE;
    }
}

/* FPGA数据有效性状态获取.
 * Para:
 *     pSts, 状态存储地址，输入保证地址有效性.
 * Return:
 *     NONE.
 */
static void fpgaGetSts(uint32_t *pSts)
{
    int i;
    int j;
    BOOL bChgSmvStsFile=FALSE;
    static char ucTmpStr[TEMP_INFO_MAX_LEN];

    /* 读取同步标志 */
    sFpgaCfg.stsSecPulseReg_un.ulSecPulseStsReg = *sFpgaCfg.pstsSecPulseRegAddr;

    /* A/D通道状态标读取 */
    sFpgaCfg.adPara.bdReg_un[smvData.ucBufCnt].ulBdReg = *sFpgaCfg.adPara.pbdRegAddr[smvData.ucBufCnt];

    /* A/D采样错误标
     */
    sFpgaCfg.adPara.stsReg_un.ulStsReg = *sFpgaCfg.adPara.pstsRegAddr;

    /* 通道数据有效性,首次赋值需清0 */
    for (i = 0; i<sFpgaCfg.adPara.ucMaxChnNum; i++)
    {
        if (!((sFpgaCfg.adPara.bdReg_un[smvData.ucBufCnt].bdReg_st.q >> i) & 0x01))
        {
            pSts[i] = AI_DAT_VLD;
        }
        else
        {
            pSts[i] = 0;
        }

        if ((!sFpgaCfg.stsSecPulseReg_un.stsSecPulseReg_st.Syn)
                && sFpgaCfg.bExternalSynFlag)
        {
            pSts[i] |= AI_DAT_SYN;
        }

        if (sFpgaCfg.adPara.stsReg_un.stsReg_st.overflowFlag)
        {
            pSts[i] |= AI_BUF_OVERFLOW;
        }

        if (sFpgaCfg.adPara.stsReg_un.stsReg_st.ad1Error)
        {
            pSts[i] |= AI_AD1_ERR;
        }

        if (sFpgaCfg.adPara.stsReg_un.stsReg_st.ad2Error)
        {
            pSts[i] |= AI_AD2_ERR;
        }

        if (sFpgaCfg.adPara.stsReg_un.stsReg_st.ad3Error)
        {
            pSts[i] |= AI_AD3_ERR;
        }
        if (sFpgaCfg.adPara.stsReg_un.stsReg_st.ad4Error)
        {
            pSts[i] |= AI_AD4_ERR;
        }
    }

    if(sFpgaCfg.adPara.ucMaxChnNum>0)
    {
        if(FPGASmvCommStat[0]
                !=pSts[0])
        {
            FPGASmvCommStat[0]=pSts[0];
            bChgSmvStsFile=TRUE;
        }
    }

    /* 有异常标志则清0 */
    if (sFpgaCfg.adPara.stsReg_un.ulStsReg)
    {
        *sFpgaCfg.adPara.pstsRegAddr = sFpgaCfg.adPara.stsReg_un.ulStsReg;
    }

    /* A/D复采通道状态标读取 */
    sFpgaCfg.adParaRepeat.bdReg_un[smvData.ucBufCnt].ulBdReg = *sFpgaCfg.adParaRepeat.pbdRegAddr[smvData.ucBufCnt];

    /* A/D复采采样错误标
     */
    sFpgaCfg.adParaRepeat.stsReg_un.ulStsReg = *sFpgaCfg.adParaRepeat.pstsRegAddr;

    /* 通道数据有效性,首次赋值需清0 */
    for (i = 0; i<sFpgaCfg.adParaRepeat.ucMaxChnNum; i++)
    {
        if (!((sFpgaCfg.adParaRepeat.bdReg_un[smvData.ucBufCnt].bdReg_st.q >> i) & 0x01))
        {
            pSts[sFpgaCfg.adParaRepeat.ucDataPosOff+i] = AI_DAT_VLD;
        }
        else
        {
            pSts[sFpgaCfg.adParaRepeat.ucDataPosOff+i] = 0;
        }

        if ((!sFpgaCfg.stsSecPulseReg_un.stsSecPulseReg_st.Syn)
                && sFpgaCfg.bExternalSynFlag)
        {
            pSts[sFpgaCfg.adParaRepeat.ucDataPosOff+i] |= AI_DAT_SYN;
        }

        if (sFpgaCfg.adParaRepeat.stsReg_un.stsReg_st.overflowFlag)
        {
            pSts[sFpgaCfg.adParaRepeat.ucDataPosOff+i] |= AI_BUF_OVERFLOW;
        }

        if (sFpgaCfg.adParaRepeat.stsReg_un.stsReg_st.linkError)
        {
            pSts[sFpgaCfg.adParaRepeat.ucDataPosOff+i] |= AI_COM_ERR;
        }

        if (sFpgaCfg.adParaRepeat.stsReg_un.stsReg_st.intervalError)
        {
            pSts[sFpgaCfg.adParaRepeat.ucDataPosOff+i] |= AI_INT_RANGE_OUT;
        }

        if (sFpgaCfg.adParaRepeat.stsReg_un.stsReg_st.CRCError)
        {
            pSts[sFpgaCfg.adParaRepeat.ucDataPosOff+i] |= AI_FPGA_CRC_ERR;
        }
        if (sFpgaCfg.adParaRepeat.stsReg_un.stsReg_st.ad1Error)
        {
            pSts[sFpgaCfg.adParaRepeat.ucDataPosOff+i] |= AI_AD1_ERR;
        }
        if (sFpgaCfg.adParaRepeat.stsReg_un.stsReg_st.ad2Error)
        {
            pSts[sFpgaCfg.adParaRepeat.ucDataPosOff+i] |= AI_AD2_ERR;
        }
    }

    if(sFpgaCfg.adParaRepeat.ucMaxChnNum>0)
    {
        if(FPGASmvCommStat[1]
                !=pSts[sFpgaCfg.adParaRepeat.ucDataPosOff])
        {
            FPGASmvCommStat[1]
                =pSts[sFpgaCfg.adParaRepeat.ucDataPosOff];
            bChgSmvStsFile=TRUE;
        }
    }

    /* 有异常标志则清0 */
    if (sFpgaCfg.adParaRepeat.stsReg_un.ulStsReg)
    {
        *sFpgaCfg.adParaRepeat.pstsRegAddr = sFpgaCfg.adParaRepeat.stsReg_un.ulStsReg;
    }

    /* FT3接收标志
     */
    for (i = 0; i<FT3_PORT_NUM; i++)
    {
        if (sFpgaCfg.ft3Para[i].bUsed)
        {
            /* FT3状态标读取 */
            sFpgaCfg.ft3Para[i].bdReg_un[smvData.ucBufCnt].ulBdReg = *sFpgaCfg.ft3Para[i].pbdRegAddr[smvData.ucBufCnt];

            /* FT3错误标
             */
            sFpgaCfg.ft3Para[i].stsReg_un.ulStsReg = *sFpgaCfg.ft3Para[i].pstsRegAddr;

            /* 数据有效性,首次赋值需清0 */
            for (j = 0; j<sFpgaCfg.ft3Para[i].ucMaxChnNum; j++)
            {
                if (!((sFpgaCfg.ft3Para[i].bdReg_un[smvData.ucBufCnt].bdReg_st.q >> j) & 0x01))
                {
                    pSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j] = AI_DAT_VLD;
                }
                else
                {
                    pSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j] = 0;
                }

                /* 状态字有无效标志 */
                if (j<FT3_CHN_OFF)
                {
                    if ((sFpgaCfg.ft3Para[i].reg.pReg4->Status1 >> (FT3_VALID_OFF+j)) & 0x01)
                    {
                        static BOOL bErrFlag = FALSE;

                        pSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j] |= AI_DAT_VLD;
                        if (!bErrFlag)
                        {
                            bErrFlag = TRUE;
                            sprintf(ucTmpStr, "接收FT3数据帧无效,端口%d,通道%d\n",
                                    sFpgaCfg.ft3Para[i].ucPortNo, j);
                            LOG_Write(LOG_RUN, ucTmpStr, NULL);
                        }
                    }
                }
                else
                {
                    if ((sFpgaCfg.ft3Para[i].reg.pReg4->Status2 >> (j-FT3_CHN_OFF)) & 0x01)
                    {
                        static BOOL bErrFlag = FALSE;

                        pSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j] |= AI_DAT_VLD;
                        if (!bErrFlag)
                        {
                            bErrFlag = TRUE;
                            sprintf(ucTmpStr, "接收FT3数据帧无效,端口%d,通道%d\n",
                                    sFpgaCfg.ft3Para[i].ucPortNo, j);
                            LOG_Write(LOG_RUN, ucTmpStr, NULL);
                        }
                    }
                }

                /* 是否需要外同步 */
                if (sFpgaCfg.bExternalSynFlag)
                {
                    if (!sFpgaCfg.stsSecPulseReg_un.stsSecPulseReg_st.Syn)
                    {
                        pSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j] |= AI_DAT_SYN;
                    }

                    if ((sFpgaCfg.ft3Para[i].reg.pReg4->Status1 >> FT3_SYN_OFF) & 0x01)
                    {
                        static BOOL bErrFlag = FALSE;

                        pSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j] |= AI_DAT_SYN;
                        if (!bErrFlag)
                        {
                            bErrFlag = TRUE;
                            sprintf(ucTmpStr, "接收FT3数据帧未同步,端口%d,通道%d\n",
                                    sFpgaCfg.ft3Para[i].ucPortNo, j);
                            LOG_Write(LOG_RUN, ucTmpStr, NULL);
                        }
                    }
                }

                if (sFpgaCfg.ft3Para[i].stsReg_un.stsReg_st.overflowFlag)
                {
                    pSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j] |= AI_BUF_OVERFLOW;
                }

                if (sFpgaCfg.ft3Para[i].stsReg_un.stsReg_st.linkFail)
                {
                    pSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j] |= AI_COM_ERR;
                }

                if (sFpgaCfg.ft3Para[i].stsReg_un.stsReg_st.rangeOut)
                {
                    pSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j] |= AI_INT_RANGE_OUT;
                }

                if (sFpgaCfg.ft3Para[i].stsReg_un.stsReg_st.crcErr)
                {
                    pSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j] |= AI_FPGA_CRC_ERR;
                }

                /* 检修位 */
                if ((sFpgaCfg.ft3Para[i].reg.pReg4->Status1 >> FT3_REPAIR_OFF) & 0x01)
                {
                    pSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j] |= AI_TEST_DAT;
                }
            }

            if(sFpgaCfg.ft3Para[i].ucMaxChnNum>0)
            {
                if(FPGASmvCommStat[2+i]
                        !=pSts[sFpgaCfg.ft3Para[i].ucDataPosOff])
                {
                    FPGASmvCommStat[2+i]
                        =pSts[sFpgaCfg.ft3Para[i].ucDataPosOff];
                    bChgSmvStsFile=TRUE;
                }
            }

            /* 有异常标志则清0 */
            if (sFpgaCfg.ft3Para[i].stsReg_un.ulStsReg)
            {
                *sFpgaCfg.ft3Para[i].pstsRegAddr = sFpgaCfg.ft3Para[i].stsReg_un.ulStsReg;
            }

            /* 没有处理数据帧中的状态字 */
        }
    }

    /* 9-2状态标读取
     */
    for (i = 0; i<SMV_RCV_PORT_NUM; i++)
    {
        if (sFpgaCfg.svRcvPara[i].bUsed)
        {
            /* 状态标读取 */
            sFpgaCfg.svRcvPara[i].bdReg_un[smvData.ucBufCnt].ulBdReg = *sFpgaCfg.svRcvPara[i].pbdRegAddr[smvData.ucBufCnt];

            /* 9-2错误标 */
            sFpgaCfg.svRcvPara[i].stsReg0_un.ulStsReg0 = *sFpgaCfg.svRcvPara[i].psvRecvSts0RegAddr;

            /* 接收数据自带状态 */
            sFpgaCfg.svRcvPara[i].stsReg1_un.ulStsReg1 = *sFpgaCfg.svRcvPara[i].psvRecvSts1RegAddr;

            /* 数据有效性,首次赋值需清0 */
            for (j = 0; j<sFpgaCfg.svRcvPara[i].ucMaxChnNum; j++)
            {
                if (!((sFpgaCfg.svRcvPara[i].bdReg_un[smvData.ucBufCnt].bdReg_st.q >> j) & 0x01))
                {
                    pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j] = AI_DAT_VLD;
                }
                else
                {
                    pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j] = 0;
                }

                /* 为与本地有无效区分,记录日志 */
                if (!((sFpgaCfg.svRcvPara[i].stsReg1_un.ulStsReg1 >> (SMV_VALID_OFF+j)) & 0x01))
                {
                    static BOOL bErrFlag = FALSE;

                    pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j] |= AI_DAT_VLD;
                    if (!bErrFlag)
                    {
                        bErrFlag = TRUE;
                        sprintf(ucTmpStr, "接收9-2数据帧无效,端口%d,通道%d\n",
                                sFpgaCfg.svRcvPara[i].ucPortNo, j);
                        LOG_Write(LOG_RUN, ucTmpStr, NULL);
                    }
                }

                /* 检修 */
                if ((sFpgaCfg.svRcvPara[i].stsReg1_un.ulStsReg1 >> SMV_TEST_OFF) & 0x01)
                {
                    pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j] |= AI_TEST_DAT;
                }

                /* 是否需要外同步 */
                if (sFpgaCfg.bExternalSynFlag)
                {
                    if (!sFpgaCfg.stsSecPulseReg_un.stsSecPulseReg_st.Syn)
                    {
                        pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j] |= AI_DAT_SYN;
                    }

                    /* 为与本地同步信号区分,记录日志 */
                    if (!((sFpgaCfg.svRcvPara[i].stsReg1_un.ulStsReg1 >> SMV_SYN_OFF) & 0x01))
                    {
                        static BOOL bErrFlag = FALSE;

                        pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j] |= AI_DAT_SYN;
                        if (!bErrFlag)
                        {
                            bErrFlag = TRUE;
                            sprintf(ucTmpStr, "接收9-2数据帧失步,端口%d,通道%d\n",
                                    sFpgaCfg.svRcvPara[i].ucPortNo, j);
                            LOG_Write(LOG_RUN, ucTmpStr, NULL);
                        }
                    }
                }

                if (sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.overflowFlag)
                {
                    pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j] |= AI_BUF_OVERFLOW;
                }

                if (sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.linkFail)
                {
                    pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j] |= AI_COM_ERR;
                }

                if (sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.rangeOut)
                {
                    pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j] |= AI_INT_RANGE_OUT;
                }

                if (sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.decodeErr)
                {
                    pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j] |= AI_DECODE_ERR;
                }

                if (sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.crcErr)
                {
                    pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j] |= AI_CRC_ERR;
                }

                if (sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.lenErr)
                {
                    static BOOL bErrFlag = FALSE;

                    if (!bErrFlag)
                    {
                        bErrFlag = TRUE;
                        LOG_Write(LOG_RUN, "Frame Length Error\n", NULL);
                    }
                    pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j] |= AI_LEN_ERROR;
                }

                if (sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.noErr)
                {
                    static BOOL bErrFlag = FALSE;

                    if (!bErrFlag)
                    {
                        bErrFlag = TRUE;
                        LOG_Write(LOG_RUN, "Frame Non-Octet Error\n", NULL);
                    }
                    pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j] |= AI_OCTET_ERROR;
                }

                if (sFpgaCfg.svRcvPara[i].stsReg0_un.stsReg0_st.rcvErr)
                {
                    static BOOL bErrFlag = FALSE;

                    if (!bErrFlag)
                    {
                        bErrFlag = TRUE;
                        LOG_Write(LOG_RUN, "PHY Receive Error\n", NULL);
                    }
                    pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j] |= AI_PHY_ERROR;
                }
            }

            if(sFpgaCfg.svRcvPara[i].ucMaxChnNum>0)
            {
                if(FPGASmvCommStat[2+FT3_PORT_NUM+i]
                        !=pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff])
                {
                    FPGASmvCommStat[2+FT3_PORT_NUM+i]
                        =pSts[sFpgaCfg.svRcvPara[i].ucDataPosOff];
                    bChgSmvStsFile=TRUE;
                }
            }

            /* 有异常标志则清0 */
            if (sFpgaCfg.svRcvPara[i].stsReg0_un.ulStsReg0)
            {
                *sFpgaCfg.svRcvPara[i].psvRecvSts0RegAddr = sFpgaCfg.svRcvPara[i].stsReg0_un.ulStsReg0;
            }
        }
    }

    if(bChgSmvStsFile)
    {
        Smv_Go_CommStat_Chg();/*触发状态文件更新*/
    }
}

/* FPGA数据有效性判断.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
 * Alert:
 *     返回TRUE表明本次查询并插值抽取到有效数据,
 *     返回FALSE表明无有效插值抽取数据,
 *     支持多次查询
 */
BOOL fpgaValidDataJudge(void)
{
    int i = 0;
    int j;
    BOOL bPoints24TXReady = FALSE;  /* 插值标志 */
    BOOL bPoints24TXRead = FALSE;  /* 预存标志 */
    static BOOL bFirst = TRUE;  /* 首次获取到有效采样值 */
    static BOOL bFirstExe = TRUE;   /* 初次执行,用于清除出错标志,如缓冲溢出等 */
    int16_t nDiff = 0;  /* 差值计算 */

    sFpgaSimul.ulRdCnt++;

    /* 初次清除错误标志 */
    if (bFirstExe)
    {
        bFirstExe = FALSE;
        *sFpgaCfg.adPara.pstsRegAddr = sFpgaCfg.adPara.stsReg_un.ulStsReg;

        /* A/D复采 */
        if (sFpgaCfg.adParaRepeat.bUsed)
        {
            *sFpgaCfg.adParaRepeat.pstsRegAddr = sFpgaCfg.adParaRepeat.stsReg_un.ulStsReg;
        }

        for (i = 0; i<FT3_PORT_NUM; i++)
        {
            if (sFpgaCfg.ft3Para[i].bUsed)
            {
                *sFpgaCfg.ft3Para[i].pstsRegAddr = sFpgaCfg.ft3Para[i].stsReg_un.ulStsReg;
            }
        }

        for (i = 0; i<SMV_RCV_PORT_NUM; i++)
        {
            if (sFpgaCfg.svRcvPara[i].bUsed)
            {
                *sFpgaCfg.svRcvPara[i].psvRecvSts0RegAddr = sFpgaCfg.svRcvPara[i].stsReg0_un.ulStsReg0;
            }
        }

        /* 首次状态标清零 */
        for (i = 0; i<MAXCHNELS; i++)
        {
            smvData.nLstSamDataSts[i] = 0;
        }
    }

    /* 获取同步状态标志 */
    fpgaSynChgSts();

    /*
     * 查询所有有效数据
     */
    while (1)
    {
        bPoints24TXReady = FALSE;

        /* 每次查询前清除有效标志 */
        smvData.ucCurSrcValidFlag = 0;

        /* A/D采样任何时候都在执行
         * 从0开始查询
         */
        sFpgaCfg.adPara.bdReg_un[smvData.ucBufCnt].ulBdReg = *sFpgaCfg.adPara.pbdRegAddr[smvData.ucBufCnt];
        if (sFpgaCfg.adPara.bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty)
        {
            /* 有效数据 */
            smvData.ucCurSrcValidFlag |= 0x01 << AD_PORT_NO;
        }
        else
        {
            sFpgaSimul.ulDrawOutInvalidCnt++;

            goto ret;
        }

        /* A/D复采 */
        if (sFpgaCfg.adParaRepeat.bUsed)
        {
            sFpgaCfg.adParaRepeat.bdReg_un[smvData.ucBufCnt].ulBdReg = *sFpgaCfg.adParaRepeat.pbdRegAddr[smvData.ucBufCnt];
            if (sFpgaCfg.adParaRepeat.bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty)
            {
                /* 有效数据 */
                smvData.ucCurSrcValidFlag |= 0x01 << AD_REPEAT_PORT_NO;
            }
            else
            {
                sFpgaSimul.ulDrawOutInvalidCnt++;

                goto ret;
            }
        }

        /* FT3数据有效标志 */
        for (i = 0; i<FT3_PORT_NUM; i++)
        {
            if (sFpgaCfg.ft3Para[i].bUsed)
            {
                sFpgaCfg.ft3Para[i].bdReg_un[smvData.ucBufCnt].ulBdReg = *sFpgaCfg.ft3Para[i].pbdRegAddr[smvData.ucBufCnt];
                if (sFpgaCfg.ft3Para[i].bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty)
                {
                    /* 有效数据 */
                    smvData.ucCurSrcValidFlag |= 0x01 << sFpgaCfg.ft3Para[i].ucPortNo;
                }
                else
                {
                    sFpgaSimul.ulDrawOutInvalidCnt++;

                    goto ret;
                }
            }
        }

        /* 9-2数据有效标志 */
        for (i = 0; i<SMV_RCV_PORT_NUM; i++)
        {
            if (sFpgaCfg.svRcvPara[i].bUsed)
            {
                sFpgaCfg.svRcvPara[i].bdReg_un[smvData.ucBufCnt].ulBdReg = *sFpgaCfg.svRcvPara[i].pbdRegAddr[smvData.ucBufCnt];
                if (sFpgaCfg.svRcvPara[i].bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty)
                {
                    /* 有效数据 */
                    smvData.ucCurSrcValidFlag |= 0x01<<(sFpgaCfg.svRcvPara[i].ucPortNo);
                }
                else
                {
                    sFpgaSimul.ulDrawOutInvalidCnt++;

                    goto ret;
                }
            }
        }

        /* 该点缓冲区数据有效 */
        if (smvData.ucCurSrcValidFlag == smvData.ucAllSrcValidFlag)
        {
            /* 周波80有效点计数 */
            sFpgaSimul.ul_80_ValidCnt++;

            /* A/D采样任何时候都在执行,同时FPGA在芯片正常的情况下,节拍连续
             * 但为了防止缓冲溢出或者FPGA芯片异常,平台程序仍然对节拍连续性进行判断,不连续即报错
             * 但仅获取A/D采样节拍,其它节拍与A/D采样一致
             * 获取采样节拍,生成接收数据和生成数据周波计数器
             */
            smvData.usSamCnt = sFpgaCfg.adPara.pulDataReg[smvData.ucBufCnt]->ulSamCnt;
            smvData.usSamCycleCnt = smvData.usSamCnt % SMP_RATE_PER_CYCLE;
            smvData.usDrawCycleCnt = SamplingNum_g*smvData.usSamCycleCnt / SMP_RATE_PER_CYCLE;

            /* 周波80点丢点判断
             * 条件: 丢点, 非第一次
             * 只用于统计用
             */
            if ((((smvData.usLstSecCnt+1) % smvData.usSrcCntCyle) != smvData.usSamCnt)
                    && (!bFirst))
            {
                smvData.usLostCnt++;
                nDiff = ((smvData.usSamCnt-1+smvData.usSrcCntCyle)-smvData.usLstSecCnt)%smvData.usSrcCntCyle;
                if (smvData.ulLostMaxInt<nDiff)
                {
                    smvData.ulLostMaxInt = nDiff;
                }
            }

            /* 同步抽取点,没有处理丢点
             */
            if (smvData.usSamCnt == smvData.ulEndPointNum)
            {
                /* 同步点 */
                bPoints24TXReady = TRUE;
                smvData.nDif_Pol = smvData.nArr_Dif_Pol[smvData.usSamCycleCnt];

            }
            else
            {
                /* 抽取 */
                if (smvData.nArr_Pol_Proc_Flag[smvData.usSamCycleCnt] & POINT_DRAW)
                {
                    bPoints24TXReady = TRUE;
                    smvData.nDif_Pol = smvData.nArr_Dif_Pol[smvData.usSamCycleCnt];
                }

                /* 保存,用于插值 */
                if (smvData.nArr_Pol_Proc_Flag[smvData.usSamCycleCnt] & POINT_SAVE)
                {
                    bPoints24TXRead = TRUE;
                }
            }

            /* 抽取时判断前点 */
            if (bPoints24TXReady && (smvData.nDif_Pol != 0))
            {
                if (smvData.usLstCycleCnt != smvData.usForeCnt[smvData.usSamCycleCnt])
                {
                    bPoints24TXReady = FALSE;
                }
            }

            /* 前点计数保存 */
            smvData.usLstSecCnt = smvData.usSamCnt;

            /* 非抽取点时,如果下一点插值,则保存历史值,用于插值处理
             */
            if (!bPoints24TXReady)
            {
                /* 保存该点 */
                if (bPoints24TXRead)
                {
                    /* 获取状态才有效 */
                    smvData.usLstCycleCnt = smvData.usSamCycleCnt;

                    /* 读取本点状态标 */
                    fpgaGetSts(smvData.nLstSamDataSts);

                    /* A/D采样任何时候都在执行
                     * 拷贝数据至缓冲区,用于插值,按32位保存
                     */
                    for (i = 0; i<sFpgaCfg.adPara.ucMaxChnNum; i++)
                    {
                        smvData.nLstSamData[i] = (int32_t)(sFpgaCfg.adPara.pulDataReg[smvData.ucBufCnt]->ulSamData[i]);
                    }

                    /* A/D复采 */
                    if (sFpgaCfg.adParaRepeat.bUsed)
                    {
                        /* 数据拷贝,用于插值, 按32位保存 */
                        for (j = 0; j<sFpgaCfg.adParaRepeat.ucMaxChnNum; j++)
                        {
                            smvData.nLstSamData[sFpgaCfg.adParaRepeat.ucDataPosOff+j]
                                = (int32_t)(sFpgaCfg.adParaRepeat.pulDataReg[smvData.ucBufCnt]->ulSamData[j]);
                        }
                    }

                    /* FT3接收判断 */
                    for (i = 0; i<FT3_PORT_NUM; i++)
                    {
                        if (sFpgaCfg.ft3Para[i].bUsed)
                        {
                            /* 数据拷贝,用于插值, 按32位保存 */
                            for (j = 0; j<sFpgaCfg.ft3Para[i].ucMaxChnNum; j++)
                            {
                                smvData.nLstSamData[sFpgaCfg.ft3Para[i].ucDataPosOff+j]
                                    = (int32_t)(sFpgaCfg.ft3Para[i].pulDataReg[smvData.ucBufCnt]->ulSamData[j]);
                            }
                        }
                    }

                    /* 9-2接收判断 */
                    for (i = 0; i<SMV_RCV_PORT_NUM; i++)
                    {
                        if (sFpgaCfg.svRcvPara[i].bUsed)
                        {
                            /* 数据拷贝,用于插值,按32位保存 */
                            for (j = 0; j<sFpgaCfg.svRcvPara[i].ucMaxChnNum; j++)
                            {
                                smvData.nLstSamData[sFpgaCfg.svRcvPara[i].ucDataPosOff+j]
                                    = (int32_t)(sFpgaCfg.svRcvPara[i].pulDataReg[smvData.ucBufCnt]->ulSamData[j]);
                            }
                        }
                    }
                }

                /* 针对FPGA操作时,写1清0 */
#ifdef SMV_DEBUG
                sFpgaCfg.adPara.bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 0;
#else
                sFpgaCfg.adPara.bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 1;
#endif

                *sFpgaCfg.adPara.pbdRegAddr[smvData.ucBufCnt] = sFpgaCfg.adPara.bdReg_un[smvData.ucBufCnt].ulBdReg;


                /* A/D复采 */
                if (sFpgaCfg.adParaRepeat.bUsed)
                {
                    /* 针对FPGA操作时,写1清0 */
#ifdef SMV_DEBUG
                    sFpgaCfg.adParaRepeat.bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 0;
#else
                    sFpgaCfg.adParaRepeat.bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 1;
#endif

                    *sFpgaCfg.adParaRepeat.pbdRegAddr[smvData.ucBufCnt] = sFpgaCfg.adParaRepeat.bdReg_un[smvData.ucBufCnt].ulBdReg;
                }

                /* FT3接收判断 */
                for (i = 0; i<FT3_PORT_NUM; i++)
                {
                    if (sFpgaCfg.ft3Para[i].bUsed)
                    {
                        /* 针对FPGA操作时,写1清0 */
#ifdef SMV_DEBUG
                        sFpgaCfg.ft3Para[i].bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 0;
#else
                        sFpgaCfg.ft3Para[i].bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 1;
#endif

                        *sFpgaCfg.ft3Para[i].pbdRegAddr[smvData.ucBufCnt]
                            = sFpgaCfg.ft3Para[i].bdReg_un[smvData.ucBufCnt].ulBdReg;
                    }
                }

                /* 9-2接收判断 */
                for (i = 0; i<SMV_RCV_PORT_NUM; i++)
                {
                    if (sFpgaCfg.svRcvPara[i].bUsed)
                    {
                        /* 针对FPGA操作时,写1清0 */
#ifdef SMV_DEBUG
                        sFpgaCfg.svRcvPara[i].bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 0;
#else
                        sFpgaCfg.svRcvPara[i].bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 1;
#endif

                        *sFpgaCfg.svRcvPara[i].pbdRegAddr[smvData.ucBufCnt]
                            = sFpgaCfg.svRcvPara[i].bdReg_un[smvData.ucBufCnt].ulBdReg;
                    }
                }

                /* 缓冲块计数判断 */
                smvData.ucBufCnt++;
                if (smvData.ucBufCnt == sFpgaCfg.maxDataBufNum)
                {
                    smvData.ucBufCnt = 0;
                }
            }
            else
            {
                sFpgaSimul.ulValidDrawPoint++;

                fpgaDataDraw();

                poIec_index++;
                if (poIec_index >= MAXQSIZESAMPDATA)
                {
                    /* 增加缓冲与中断缓冲一致 */
                    poIec_index = 0;
                }
            }


            /* 首次标志清零 */
            if (bFirst)
            {
                bFirst = FALSE;
            }
        }
        else
        {
            goto ret;
        }
    }

ret:

    return bPoints24TXReady;
}

/* FPGA数据读取.
 * Para:
 *     NONE
 * Return:
 *     NONE.
 */
void fpgaDataDraw(void)
{
    int i;
    int j;
    int16_t *pSamData = NULL;
    int32_t *p9_2SamData = NULL;
    int32_t *pCurData = NULL;
    int32_t *pCurSts = NULL;
    static char ucTmpStr[TEMP_INFO_MAX_LEN];

    pCurData = (int32_t *)send_data[poIec_index];
    pCurSts = (int32_t *)send_data_sts[poIec_index];
    pSamData = (int16_t *)(sFpgaCfg.adPara.pulDataReg[smvData.ucBufCnt]->ulSamData);

    /* 读取当前点状态标 */
    fpgaGetSts(smvData.nCurSamDataSts);

    /* 转化为16位插值处理并用组合状态标上送
     */
    for (i = 0; i<sFpgaCfg.adPara.ucMaxChnNum; i++)
    {
        if (sFpgaCfg.adPara.iYabanNum[i] >= 0)
        {
            SCI_Get_Yaban_Value(sFpgaCfg.adPara.iYabanNum[i], &sFpgaCfg.adPara.bRtYabanValue[i], 0);
        }

        if (sFpgaCfg.adPara.bRtYabanValue[i])
        {
            *pCurData++ = (int32_t) ((int16_t)((smvData.nDif_Pol*((int16_t)((int16_t)smvData.nLstSamData[i]-(*pSamData)))) >> 16)
                                     + (int16_t)(*pSamData));
            pSamData++;

            if (smvData.nDif_Pol)
            {
                /* 当前点与上一点或操作 */
                *pCurSts++ = smvData.nLstSamDataSts[i] | smvData.nCurSamDataSts[i];
            }
            else
            {
                *pCurSts++ = smvData.nCurSamDataSts[i];
            }
        }
        else
        {
            *pCurData++ = 0;
            pSamData++;
            *pCurSts++ = 0;
        }
        smvData.nLstSamDataSts[i] = smvData.nCurSamDataSts[i];
    }

    /*
     * 拷贝数据至缓冲区,用于插值,按字节处理,转化为32位保存
     */
    for (i = 0; i<sFpgaCfg.adPara.ucMaxChnNum; i++)
    {
        smvData.nLstSamData[i] = (int32_t)sFpgaCfg.adPara.pulDataReg[smvData.ucBufCnt]->ulSamData[i];
    }

    /* 针对FPGA操作时,写1清0 */
#ifdef SMV_DEBUG
    sFpgaCfg.adPara.bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 0;
#else
    sFpgaCfg.adPara.bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 1;
#endif

    *sFpgaCfg.adPara.pbdRegAddr[smvData.ucBufCnt] = sFpgaCfg.adPara.bdReg_un[smvData.ucBufCnt].ulBdReg;

    /* A/D复采 */
    if (sFpgaCfg.adParaRepeat.bUsed)
    {
        pCurData = (int32_t *)send_data[poIec_index]+sFpgaCfg.adParaRepeat.ucDataPosOff;
        pCurSts = (int32_t *)send_data_sts[poIec_index]+sFpgaCfg.adParaRepeat.ucDataPosOff;
        pSamData = (int16_t *)(sFpgaCfg.adParaRepeat.pulDataReg[smvData.ucBufCnt]->ulSamData);

        /* 首点一并处理
         * 转化为16位插值处理并用最新点状态标上送
         */
        for (j = 0; j<sFpgaCfg.adParaRepeat.ucMaxChnNum; j++)
        {
            if (sFpgaCfg.adParaRepeat.iYabanNum[j] >= 0)
            {
                SCI_Get_Yaban_Value(sFpgaCfg.adParaRepeat.iYabanNum[j], &sFpgaCfg.adParaRepeat.bRtYabanValue[j], 0);
            }

            if (sFpgaCfg.adParaRepeat.bRtYabanValue[j])
            {
                *pCurData++ = (int32_t) ((int16_t)((smvData.nDif_Pol*((int16_t)((int16_t)smvData.nLstSamData[sFpgaCfg.adParaRepeat.ucDataPosOff+j]-(*pSamData)))) >> 16)
                                         + (int16_t)(*pSamData));
                pSamData++;

                if (smvData.nDif_Pol)
                {
                    /* 当前点与上一点或操作 */
                    *pCurSts++ = smvData.nLstSamDataSts[sFpgaCfg.adParaRepeat.ucDataPosOff+j]
                                 | smvData.nCurSamDataSts[sFpgaCfg.adParaRepeat.ucDataPosOff+j];
                }
                else
                {
                    *pCurSts++ = smvData.nCurSamDataSts[sFpgaCfg.adParaRepeat.ucDataPosOff+j];
                }
            }
            else
            {
                *pCurData++ = 0;
                pSamData++;
                *pCurSts++ = 0;
            }

            smvData.nLstSamDataSts[sFpgaCfg.adParaRepeat.ucDataPosOff+j]
                = smvData.nCurSamDataSts[sFpgaCfg.adParaRepeat.ucDataPosOff+j];
        }

        /* 数据拷贝,用于插值,转化为32位保存 */
        for (j = 0; j<sFpgaCfg.adParaRepeat.ucMaxChnNum; j++)
        {
            smvData.nLstSamData[sFpgaCfg.adParaRepeat.ucDataPosOff+j]
                = (int32_t)sFpgaCfg.adParaRepeat.pulDataReg[smvData.ucBufCnt]->ulSamData[j];
        }

        /* 针对FPGA操作时,写1清0 */
#ifdef SMV_DEBUG
        sFpgaCfg.adParaRepeat.bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 0;
#else
        sFpgaCfg.adParaRepeat.bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 1;
#endif

        *sFpgaCfg.adParaRepeat.pbdRegAddr[smvData.ucBufCnt] = sFpgaCfg.adParaRepeat.bdReg_un[smvData.ucBufCnt].ulBdReg;
    }

    /* FT3接收判断 */
    for (i = 0; i<FT3_PORT_NUM; i++)
    {
        if (sFpgaCfg.ft3Para[i].bUsed)
        {
            pCurData = (int32_t *)(send_data[poIec_index]+sFpgaCfg.ft3Para[i].ucDataPosOff);
            pCurSts = send_data_sts[poIec_index]+sFpgaCfg.ft3Para[i].ucDataPosOff;
            pSamData = (int16_t *)(sFpgaCfg.ft3Para[i].pulDataReg[smvData.ucBufCnt]->ulSamData);

            /* 首点一并处理
             * 转化为16位插值处理并用最新点状态标上送
             */
            for (j = 0; j<sFpgaCfg.ft3Para[i].ucMaxChnNum; j++)
            {
                if (sFpgaCfg.ft3Para[i].iYabanNum[j] >= 0)
                {
                    SCI_Get_Yaban_Value(sFpgaCfg.ft3Para[i].iYabanNum[j], &sFpgaCfg.ft3Para[i].bRtYabanValue[j], 0);
                }

                if (sFpgaCfg.ft3Para[i].bRtYabanValue[j])
                {
                    *pCurData++ = (int32_t) ((int16_t)((smvData.nDif_Pol*((int16_t)((int16_t)smvData.nLstSamData[sFpgaCfg.ft3Para[i].ucDataPosOff+j]-(*pSamData)))) >> 16)
                                             + (int16_t)(*pSamData));
                    pSamData++;

                    if (smvData.nDif_Pol)
                    {
                        /* 当前点与上一点或操作 */
                        *pCurSts++ = smvData.nLstSamDataSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j]
                                     | smvData.nCurSamDataSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j];
                    }
                    else
                    {
                        *pCurSts++ = smvData.nCurSamDataSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j];
                    }
                }
                else
                {
                    *pCurData++ = 0;
                    pSamData++;
                    *pCurSts++ = 0;
                }

                smvData.nLstSamDataSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j]
                    = smvData.nCurSamDataSts[sFpgaCfg.ft3Para[i].ucDataPosOff+j];
            }

            /* 数据拷贝,用于插值,转化为32位保存 */
            for (j = 0; j<sFpgaCfg.ft3Para[i].ucMaxChnNum; j++)
            {
                smvData.nLstSamData[sFpgaCfg.ft3Para[i].ucDataPosOff+j]
                    = (int32_t)sFpgaCfg.ft3Para[i].pulDataReg[smvData.ucBufCnt]->ulSamData[j];
            }

            /* 针对FPGA操作时,写1清0 */
#ifdef SMV_DEBUG
            sFpgaCfg.ft3Para[i].bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 0;
#else
            sFpgaCfg.ft3Para[i].bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 1;
#endif

            *sFpgaCfg.ft3Para[i].pbdRegAddr[smvData.ucBufCnt]
                = sFpgaCfg.ft3Para[i].bdReg_un[smvData.ucBufCnt].ulBdReg;

            /* 延时变化记录 */
            if (smvData.Ft3Tdr[i] != sFpgaCfg.ft3Para[i].reg.pReg3->Tdr)
            {
                static uint32_t ulChgCnt = 0;

                smvData.Ft3Tdr[i] = sFpgaCfg.ft3Para[i].reg.pReg3->Tdr;
                if (ulChgCnt<CHG_MAX_REC_NUM)
                {
                    sprintf(ucTmpStr, "端口%d延时变化,延迟时间%dus\n",
                            sFpgaCfg.ft3Para[i].ucPortNo, smvData.Ft3Tdr[i]);
                    LOG_Write(LOG_KERNEL, ucTmpStr, NULL);
                }
                ulChgCnt++;
            }
        }
    }

    /* 9-2接收判断 */
    for (i = 0; i<SMV_RCV_PORT_NUM; i++)
    {
        if (sFpgaCfg.svRcvPara[i].bUsed)
        {
            pCurData = (int32_t *)send_data[poIec_index]+sFpgaCfg.svRcvPara[i].ucDataPosOff;
            pCurSts = send_data_sts[poIec_index]+sFpgaCfg.svRcvPara[i].ucDataPosOff;
            p9_2SamData = (int32_t *)sFpgaCfg.svRcvPara[i].pulDataReg[smvData.ucBufCnt]->ulSamData;

            /* 首点一并处理
             * 插值处理并用最新点状态标上送
             */
            for (j = 0; j<sFpgaCfg.svRcvPara[i].ucMaxChnNum; j++)
            {
                if (sFpgaCfg.svRcvPara[i].iYabanNum[j] >= 0)
                {
                    SCI_Get_Yaban_Value(sFpgaCfg.svRcvPara[i].iYabanNum[j], &sFpgaCfg.svRcvPara[i].bRtYabanValue[j], 0);
                }

                if (sFpgaCfg.svRcvPara[i].bRtYabanValue[j])
                {
                    *pCurData++ = (int32_t) ((((uint64_t)smvData.nDif_Pol*(uint64_t)(smvData.nLstSamData[sFpgaCfg.svRcvPara[i].ucDataPosOff+j]-(*p9_2SamData))) >> 16)
                                             + (*p9_2SamData));
                    p9_2SamData++;

                    if (smvData.nDif_Pol)
                    {
                        /* 当前点与上一点或操作 */
                        *pCurSts++ = smvData.nLstSamDataSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j]
                                     | smvData.nCurSamDataSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j];
                    }
                    else
                    {
                        *pCurSts++ = smvData.nCurSamDataSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j];
                    }
                }
                else
                {
                    *pCurData++ = 0;
                    p9_2SamData++;
                    *pCurSts++ = 0;
                }

                smvData.nLstSamDataSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j]
                    = smvData.nCurSamDataSts[sFpgaCfg.svRcvPara[i].ucDataPosOff+j];
            }

            /* 数据拷贝,用于插值,转化为32位保存 */
            for (j = 0; j<sFpgaCfg.svRcvPara[i].ucMaxChnNum; j++)
            {
                smvData.nLstSamData[sFpgaCfg.svRcvPara[i].ucDataPosOff+j]
                    = sFpgaCfg.svRcvPara[i].pulDataReg[smvData.ucBufCnt]->ulSamData[j];
            }

            /* 针对FPGA操作时,写1清0 */
#ifdef SMV_DEBUG
            sFpgaCfg.svRcvPara[i].bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 0;
#else
            sFpgaCfg.svRcvPara[i].bdReg_un[smvData.ucBufCnt].bdReg_st.notEmpty = 1;
#endif

            *sFpgaCfg.svRcvPara[i].pbdRegAddr[smvData.ucBufCnt]
                = sFpgaCfg.svRcvPara[i].bdReg_un[smvData.ucBufCnt].ulBdReg;

            /* 延迟寄存器读取 */
            sFpgaCfg.svRcvPara[i].stsReg2_un.ulStsReg2 = *sFpgaCfg.svRcvPara[i].psvRecvSts2RegAddr;

            /* 延时变化记录 */
            if (smvData.SmvTdr[i] != sFpgaCfg.svRcvPara[i].stsReg2_un.stsReg2_st.Tdr)
            {
                static uint32_t ulChgCnt = 0;

                if (ulChgCnt<CHG_MAX_REC_NUM)
                {
                    sprintf(ucTmpStr, "端口%d延时变化,延迟时间%dus\n",
                            sFpgaCfg.svRcvPara[i].ucPortNo, smvData.SmvTdr[i]);
                    LOG_Write(LOG_KERNEL, ucTmpStr, NULL);
                }
                ulChgCnt++;

                smvData.SmvTdr[i] = sFpgaCfg.svRcvPara[i].stsReg2_un.stsReg2_st.Tdr;
            }
        }
    }

    /* 缓冲块计数判断 */
    smvData.ucBufCnt++;
    if (smvData.ucBufCnt == sFpgaCfg.maxDataBufNum)
    {
        smvData.ucBufCnt = 0;
    }
}

/* 复位FPGA.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void fpgaReset(void)
{
    IoPinOutputHigh(PD21, IO_PIN_HIGH);
    ds1306Delay(5000);
    IoPinOutputHigh(PD21, IO_PIN_LOW);
}

/* 软复位FPGA.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void fpgaSoftReset()
{
    int iLockKey;
    iLockKey = intLock();
    sFpgaCfg.ctrlReg0_un.ctrlReg0_st.FpgaSoftReset = 1;
    *sFpgaCfg.pctrlReg0Addr = sFpgaCfg.ctrlReg0_un.ulCtrlReg0;
    intUnlock(iLockKey);
}

/* 通过通讯获取采样值初始化.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, FALSE.
	*/
BOOL GetFPGASampDataInit(void)
{
    BOOL cfgOk = FALSE;

    cfgOk = cfgFileParse();	/* 配置文件解析 */
    if (cfgOk)
    {
        Write_FPGA_Program();/* 下载FPGA程序 */

        taskDelay(5);

        fpgaReset();//硬件复位FPGA

        taskDelay(5);

        fpgaSoftReset();//软件复位FPGA

        taskDelay(5);

        fpgaInfoCreate();

        /* 计算插值系数 */
        fpgaCalcInsertPos(SMP_RATE_PER_CYCLE, SamplingNum_g, 0);

        fpgaInit();

        cfgOk = TRUE;

        bNeedtoStartFPGA=TRUE;
        bPlatformCfgFPGA = TRUE;
    }
    else
    {
        fpgaCommonRegAddrInit();/* 初始化通用寄存器指针值 */

        if(fpgaGetInitFinishFlag())/* FPGA是否已初始化 */
        {
            if(TimeSycModeCfg.SycModeNum>0)
            {
                sXmlCfg.smvCfg.synMode = Get_DefaultTimeSycMode();
                fpgaSetTimeSyncMode(sXmlCfg.smvCfg.synMode);
            }
            else
            {
                sXmlCfg.smvCfg.synMode = fpgaGetTimeSyncMode();/* 获取对时方式 */
            }

            if(InterPulseModeCfg.PulseModeNum>0)
            {
                sXmlCfg.smvCfg.intelPulse = Get_DefaultInterPulseMode();
                fpgaSetInterPulseMode(sXmlCfg.smvCfg.intelPulse);
            }
            else
            {
                sXmlCfg.smvCfg.intelPulse = fpgaGetInterPulseMode();/* 获取脉冲插值方式 */
            }

            bNeedtoStartFPGA=TRUE;

            cfgOk = TRUE;
        }
        else
        {
            sXmlCfg.smvCfg.synMode = INTERNAL_DEFAULT;/* 缺省对时 */
            sXmlCfg.smvCfg.intelPulse = INTERNAL_FPGA_CRYS;/* 缺省 */
        }
        cfgOk = FALSE;
    }

    return cfgOk;
}

/* 启动FPGA运行.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void fpgaStart(void)
{
    int iLockKey;
    /* 置初始化完成标志 */
    if(bNeedtoStartFPGA)
    {
        iLockKey=intLock();
        sFpgaCfg.ctrlReg0_un.ulCtrlReg0 = *sFpgaCfg.pctrlReg0Addr;
        sFpgaCfg.ctrlReg0_un.ctrlReg0_st.InitOverFlag = 1;
        *sFpgaCfg.pctrlReg0Addr = sFpgaCfg.ctrlReg0_un.ulCtrlReg0;
        intUnlock(iLockKey);

        /* 置初始化完成标志 */
        sFpgaCfg.bFpgaInitFinishFlag = TRUE;
    }
}

BOOL Add_TimeSycMode(int mode)
{
    if(mode<TIME_ADJ_MODE_NUM)
    {
        if(TimeSycModeCfg.SycModeNum>0)
        {
            /*当前仅支持硬1588或FPGA1588其中任一种,不可混用*/
            if((TimeSycModeCfg.SycModeArray[TimeSycModeCfg.SycModeNum-1]==EXTERNAL_1588_0_ADJ)
                    ||(TimeSycModeCfg.SycModeArray[TimeSycModeCfg.SycModeNum-1]==EXTERNAL_1588_1_ADJ))
            {
                if((mode!=EXTERNAL_1588_0_ADJ)&&(mode!=EXTERNAL_1588_1_ADJ))
                {
                    return FALSE;
                }
            }
            else if((TimeSycModeCfg.SycModeArray[TimeSycModeCfg.SycModeNum-1]==EXTERNAL_1588_2_ADJ)
                    ||(TimeSycModeCfg.SycModeArray[TimeSycModeCfg.SycModeNum-1]==EXTERNAL_1588_3_ADJ))
            {
                if((mode!=EXTERNAL_1588_2_ADJ)&&(mode!=EXTERNAL_1588_3_ADJ))
                {
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }
        }
        if(TimeSycModeCfg.SycModeNum<MAX_TIME_SYC_MODE_NUM)
        {
            TimeSycModeCfg.SycModeArray[TimeSycModeCfg.SycModeNum]=mode;
            TimeSycModeCfg.SycModeNum++;
            return TRUE;
        }
    }
    return FALSE;
}

UINT8 Get_DefaultTimeSycMode()
{
    if(TimeSycModeCfg.SycModeNum>0)
    {
        return TimeSycModeCfg.SycModeArray[0];
    }
    else
    {
        return INTERNAL_DEFAULT;
    }
}

UINT8 Get_TimeSycModeAvailable()
{
    /*仅支持1588对时模式之间的切换*/
    UINT8 i;
    for(i=0; i<TimeSycModeCfg.SycModeNum; i++)
    {
        if(Check_F_A_1588_Status(TimeSycModeCfg.SycModeArray[i]))
        {
            return TimeSycModeCfg.SycModeArray[i];
        }
    }

    return INTERNAL_DEFAULT;
}

BOOL Change_TimeSycAndInterPulseMode()
{
    UINT8 ValidMode;
    if(TimeSycModeCfg.SycModeNum>1)
    {
        ValidMode=Get_TimeSycModeAvailable();
        if(ValidMode>INTERNAL_DEFAULT)
        {
            if(ValidMode!=fpgaGetTimeSyncMode())
            {
                fpgaSetTimeSyncMode(ValidMode);
                fpgaSetInterPulseMode(ValidMode);/*插值脉冲同步方式也一起切换，对时与同步方式一致*/
                LOG_Dbg_Msg("切换对时模式、插值脉冲方式为%u\n",ValidMode,2,3,4,5,6);
            }
            return TRUE;
        }
    }
    return FALSE;
}

BOOL Add_InterPulseMode(int mode)
{
    if(mode<PULSE_MODE_MAX_NUM)
    {
        if(InterPulseModeCfg.PulseModeNum>0)
        {
            /*当前仅支持硬1588或FPGA1588其中任一种,不可混用*/
            if((InterPulseModeCfg.PulseModeArray[InterPulseModeCfg.PulseModeNum-1]==EXTERNAL_1588_0_PULSE)
                    ||(InterPulseModeCfg.PulseModeArray[InterPulseModeCfg.PulseModeNum-1]==EXTERNAL_1588_1_PULSE))
            {
                if((mode!=EXTERNAL_1588_0_PULSE)&&(mode!=EXTERNAL_1588_1_PULSE))
                {
                    return FALSE;
                }
            }
            else if((InterPulseModeCfg.PulseModeArray[InterPulseModeCfg.PulseModeNum-1]==EXTERNAL_1588_2_PULSE)
                    ||(InterPulseModeCfg.PulseModeArray[InterPulseModeCfg.PulseModeNum-1]==EXTERNAL_1588_3_PULSE))
            {
                if((mode!=EXTERNAL_1588_2_PULSE)&&(mode!=EXTERNAL_1588_3_PULSE))
                {
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }
        }
        if(InterPulseModeCfg.PulseModeNum<MAX_INTER_PULSE_MODE_NUM)
        {
            InterPulseModeCfg.PulseModeArray[InterPulseModeCfg.PulseModeNum]=mode;
            InterPulseModeCfg.PulseModeNum++;
            return TRUE;
        }
    }
    return FALSE;
}

UINT8 Get_DefaultInterPulseMode()
{
    if(InterPulseModeCfg.PulseModeNum>0)
    {
        return InterPulseModeCfg.PulseModeArray[0];
    }
    else
    {
        return INTERNAL_DEFAULT;
    }
}

/* 获取物理通道到采样通道对应关系.
 * Para:
 *     pSamtoAna,物理通道到采样通道对应关系.
 *     pSamSource, 通道对应端口号.
 * Return:
 *     NONE.
 */
void cfgGetSamtoAna(uint8_t *pSamtoAna, uint8_t *pSamSource)
{
    uint8_t i;
    int j;

    assert(pSamtoAna);

    for (i = 0; i<sXmlCfg.smvCfg.subNum; i++)
    {
        for (j = 0; j<sXmlCfg.smvCfg.smvSub[i].chnNum; j++)
        {
            /* 初始指向通道1
             * 配置文件可悬空配置
             * 悬空时缺省指向通道1
             */
            pSamtoAna[sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno-1] = 1;
            pSamSource[sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno-1]=255;
            if (sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel == 255)
            {
                pSamtoAna[sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno-1] = 1;
                pSamSource[sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno-1]=255;
                continue;
            }

            if (sXmlCfg.smvCfg.smvSub[i].port == AD_PORT_NO)
            {
                pSamtoAna[sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno-1]
                    = sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel;
                pSamSource[sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno-1]=AD_PORT_NO;
            }
            else if (sXmlCfg.smvCfg.smvSub[i].port == FT3_PORT_NO_1)
            {
                pSamtoAna[sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno-1]
                    = sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel+FPGA_AD_SAM_CHN_NUM;
                pSamSource[sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno-1]=FT3_PORT_NO_1;
            }
            else if (sXmlCfg.smvCfg.smvSub[i].port == FT3_PORT_NO_2)
            {
                pSamtoAna[sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno-1]
                    = sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel
                      +FPGA_AD_SAM_CHN_NUM+FPGA_FT3_SAM_CHN_NUM;
                pSamSource[sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno-1]=FT3_PORT_NO_2;
            }
            else if (sXmlCfg.smvCfg.smvSub[i].port == SMV_PORT_NO_1)
            {
                pSamtoAna[sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno-1]
                    = sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel
                      +FPGA_AD_SAM_CHN_NUM+3*FPGA_FT3_SAM_CHN_NUM;
                pSamSource[sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno-1]=SMV_PORT_NO_1;
            }
            else if (sXmlCfg.smvCfg.smvSub[i].port == SMV_PORT_NO_2)
            {
                pSamtoAna[sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno-1]
                    = sXmlCfg.smvCfg.smvSub[i].chnSub[j].chanel
                      +FPGA_AD_SAM_CHN_NUM+3*FPGA_FT3_SAM_CHN_NUM+FPGA_SV_RCV_CHN_NUM;
                pSamSource[sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno-1]=SMV_PORT_NO_2;
            }
        }
    }
}

/* 获取物理通道一次/数字量系数比.
 * Para:
 *     ucHdCh,物理通道号,从1开始.
 * Return:
 *     系数.
 */
float cfgGetHwChnCoff(uint8_t ucHdCh)
{
    uint8_t i;
    int j;

    for (i = 0; i<sXmlCfg.smvCfg.subNum; i++)
    {
        for (j = 0; j<sXmlCfg.smvCfg.smvSub[i].chnNum; j++)
        {
            if (ucHdCh == sXmlCfg.smvCfg.smvSub[i].chnSub[j].hwcfgno)
            {
                if (sXmlCfg.smvCfg.smvSub[i].type == DATA_SRC_ADC)
                {
                    if (sXmlCfg.smvCfg.smvSub[i].chnSub[j].smvValOut == 0)
                    {
                        return 0.0;
                    }
                    else
                    {
                        if (sXmlCfg.smvCfg.smvSub[i].chnSub[j].type == DATA_TYPE_PRO)
                        {
                            return (SMV_DIGIT_SCP*(float)sXmlCfg.smvCfg.smvSub[i].chnSub[j].coeff)
                                   /((float)sXmlCfg.smvCfg.smvSub[i].chnSub[j].smvValOut*SMV_MAX_DIGIT/(float)AD_MAX_INPUT);
                        }
                        else if (sXmlCfg.smvCfg.smvSub[i].chnSub[j].type == DATA_TYPE_MEA)
                        {
                            return (SMV_DIGIT_SCM*(float)sXmlCfg.smvCfg.smvSub[i].chnSub[j].coeff)
                                   /((float)sXmlCfg.smvCfg.smvSub[i].chnSub[j].smvValOut*SMV_MAX_DIGIT/(float)AD_MAX_INPUT);
                        }
                        else if (sXmlCfg.smvCfg.smvSub[i].chnSub[j].type == DATA_TYPE_VOL)
                        {
                            return (SMV_DIGIT_SV*(float)sXmlCfg.smvCfg.smvSub[i].chnSub[j].coeff)
                                   /((float)sXmlCfg.smvCfg.smvSub[i].chnSub[j].smvValOut*SMV_MAX_DIGIT/(float)AD_MAX_INPUT);
                        }
                    }
                }
                else if ((sXmlCfg.smvCfg.smvSub[i].type == DATA_SRC_STDFT3)
                         || (sXmlCfg.smvCfg.smvSub[i].type == DATA_SRC_GRDFT3))
                {
                    return sXmlCfg.smvCfg.smvSub[i].chnSub[j].coeff;
                }
                else if (sXmlCfg.smvCfg.smvSub[i].type == DATA_SRC_SMV)
                {
                    if (sXmlCfg.smvCfg.smvSub[i].chnSub[j].smvValIn == 0)
                    {
                        return 0.0;
                    }
                    else
                    {
                        return ((float)(sXmlCfg.smvCfg.smvSub[i].chnSub[j].smvValOut)*sXmlCfg.smvCfg.smvSub[i].chnSub[j].coeff)
                               /(float)(sXmlCfg.smvCfg.smvSub[i].chnSub[j].smvValIn);
                    }
                }
                else
                {
                    assert (FALSE);
                }
            }
        }
    }

    return 1.0;
}

/* 计算插值位置
 * Para:
 *     ulOrgPointNum, 原始数据周波点数.
 *     ulInsertPointNum, 插值周波点数.
 *     ulDelayNum, 数据延迟点数.
 * Return:
 *     NONE.
 */
void fpgaCalcInsertPos(uint32_t ulOrgPointNum, uint32_t ulInsertPointNum, uint32_t ulDelayNum)
{
#define POS_PRECISION 5  /* 0.0000763 */

    int i;
    int j;
    int k;
    uint32_t ulDes;  /* 余数 */
    uint32_t nOldPoCount = 0xFFFFFFFF;

    /* 用于插值 */
    int32_t nFw_Pol = 0;
    int32_t nIw_Pol = 0;
    uint32_t nw_Pol = 0;
    uint32_t nTemp_Pol = 0;
    int32_t nFw_Draw_Pol = 0;

    uint32_t usDrawCycleCnt;  /* 抽取点 */

    /* 清零 */
    for (i = 0; i<ulOrgPointNum; i++)
    {
        smvData.nArr_Dif_Pol[i] = 0;
        smvData.nArr_Pol_Proc_Flag[i] = 0;
        smvData.usForeCnt[i] = 0;
    }

    /* 计算延迟整点数 */
    assert (ulDelayNum <= ulOrgPointNum);
    smvData.ulInsertDelay = (ulDelayNum*ulInsertPointNum)/ulOrgPointNum;
    ulDes = (ulDelayNum*ulInsertPointNum)%ulOrgPointNum; /* 余数 */
    if (ulDes != 0)
    {
        smvData.ulInsertDelay++;  /* 凑成整数点延迟 */
    }

    smvData.ulStartPointNum = (smvData.ulInsertDelay*ulOrgPointNum)/ulInsertPointNum-ulDelayNum; /* 在原始数据周波点数中的位置 */
    ulDes = (smvData.ulInsertDelay*ulOrgPointNum)%ulInsertPointNum;  /* 余数 */
    if (ulDes != 0)
    {
        smvData.ulEndPointNum = smvData.ulStartPointNum+1;
    }
    else
    {
        smvData.ulEndPointNum = smvData.ulStartPointNum;
    }

    nTemp_Pol = ulOrgPointNum*smvData.ulInsertDelay-ulDelayNum*ulInsertPointNum;
    nFw_Pol = (int32_t)((nTemp_Pol << 16)/ulInsertPointNum);  /* 在接收点中的位置,包含分点 */
    nw_Pol = nTemp_Pol/ulInsertPointNum;
    nIw_Pol = (int32_t)(nw_Pol << 16);  /* 整点位置 */
    smvData.nFw_Pol = nFw_Pol;  /* 初始点位置标定 */

    /* 分点位置 */
    if (ulDelayNum == 0)
    {
        smvData.nStartDif_Pol = 0;
    }
    else
    {
        smvData.nStartDif_Pol = nIw_Pol + (1 << 16) - nFw_Pol; /* 抽取点和当前点差 */
    }

    /* 抽取第一点 */
    smvData.nArr_Dif_Pol[smvData.ulStartPointNum] = 0;
    smvData.nArr_Pol_Proc_Flag[smvData.ulStartPointNum] = POINT_SAVE;
    smvData.nArr_Dif_Pol[smvData.ulEndPointNum] = smvData.nStartDif_Pol;
    smvData.nArr_Pol_Proc_Flag[smvData.ulEndPointNum] = POINT_DRAW;
    if (smvData.nStartDif_Pol != 0)
    {
        smvData.usForeCnt[smvData.ulEndPointNum] = smvData.ulStartPointNum;
    }
    else
    {
        smvData.usForeCnt[smvData.ulEndPointNum] = 0;
    }

    /* 计算各点属性 */

    nOldPoCount = 0; /* 第一点编号 */
    for (k = smvData.ulEndPointNum+1; k<ulOrgPointNum+smvData.ulStartPointNum; k++)
    {
        if (k >= ulOrgPointNum)
        {
            i = k-ulOrgPointNum;
        }
        else
        {
            i = k;
        }

        nFw_Pol = i << 16;
        if (nFw_Pol < smvData.nFw_Pol)
        {
            nFw_Pol =  nFw_Pol + (ulOrgPointNum << 16);
        }
        nFw_Pol = nFw_Pol-smvData.nFw_Pol;

        usDrawCycleCnt = (nFw_Pol*ulInsertPointNum)/(ulOrgPointNum << 16);
        nFw_Pol = nFw_Pol+smvData.nFw_Pol;

        if (usDrawCycleCnt != nOldPoCount)
        {
            j = i - 1;
            if (j<0)
            {
                j = j+ulOrgPointNum;
            }

            nTemp_Pol = ulOrgPointNum*usDrawCycleCnt;
            nFw_Draw_Pol = (int32_t)((nTemp_Pol << 16)/ulInsertPointNum);  /* 在接收点中的位置,包含分点 */
            nFw_Draw_Pol = nFw_Draw_Pol+smvData.nFw_Pol;  /* 增加偏移 */

            smvData.nArr_Dif_Pol[i] = ((nFw_Pol-nFw_Draw_Pol)>POS_PRECISION) ? nFw_Pol-nFw_Draw_Pol : 0;
            smvData.nArr_Pol_Proc_Flag[i] = POINT_DRAW;
            smvData.usForeCnt[i] = j;

            if (smvData.nArr_Dif_Pol[i] != 0)
            {
                smvData.nArr_Dif_Pol[j] = 0;
                smvData.nArr_Pol_Proc_Flag[j] = POINT_SAVE;
            }
            else
            {
                smvData.nArr_Dif_Pol[j] = 0;
                smvData.nArr_Pol_Proc_Flag[j] = 0;
            }
            nOldPoCount = usDrawCycleCnt;
        }
    }
}

