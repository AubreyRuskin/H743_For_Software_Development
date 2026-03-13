/*******************************************************************************

* Copyright (c) 2009,南自电网数字化事业部

* All rights reserved.

文件名         : smvcfg.c

相关文件       :

功能           :

作者           : 任广宇

版本           : 0.1

--------------------------------------------------------------------------------

修改记录 :

日  期          版本            修改人          修改内容

2009/09/18      0.1             任广宇          创建

********************************************************************************/



#include <stdio_compat.h>
#include "string_compat.h"
#include "mxml.h"
#include "smvcfg.h"
#include "filetool.h"
#include "edp_asst.h"
#include "swcfg.h"
#include "realdata.h"
// #include "target.h"
#include "bspinterface.h"
#include "adc.h"
#include "FPGA_Interface.h"
#include "smv_rx.h"

#define	CFG_VERSION_MAJOR	0
#define	CFG_VERSION_MINOR	2

IEC_SMV_CFG    		gSmvCfg;
IEC_SMV_AD_POOL 	gSmvADCfg;   /*最终存放的结构体名*/
IEC_SMV_FT3_POOL 	gSmvFT3Cfg;   /*最终存放的结构体名*/
IEC_SMV_SLF_POOL 	gSmvSLFCfg;   /*最终存放的结构体名*/
IEC_SMV_TX_POOL 	gSmvTXCfg;   /*最终存放的结构体名*/

/* 因扩展机箱调试移植至此 */
BOOL bArrPoleFlag[MAXCHNELS];  /* 通道极性 */
BOOL bSysSynFlag = FALSE;	/* 同步上标志 */
BOOL bPlatformCfgFPGA = FALSE; /* 平台是否配置FPGA标志 */

/*2013-6-5日 AI虚端子配置信息全局变量定义（供HMI查询） ZY */
SMV_TOTAL_VT_SV_TERM_CFG   SMV_TotalVtAITermCfg_g;

/*当前所有采样通道的实际接收状态标数组 ZY 2013-6-5 */
uint32_t aulCurSamDataStsArr_g[MAXCHNELS];

/* 接收ASDU数 */
int32_t s_smvAsduNum = -1;

/* 通过ASDU序号获取延时通道号的数组 */
uint8_t g_ucDelayChnByAsduNo[32] = {0};

BOOL AddChannals(IEC_SMV_CHAP *data,mxml_node_t *parentnode,int *datanum,int smvNum)
{
    char *s;
    int i,j;
    mxml_node_t *node1=parentnode;
    mxml_node_t *node2;
    char t[50];
    int16_t	iYabanNum;

    if(!node1)
        return FALSE;
    i=0;
    j=0;
    for(node2 = mxmlFindElement(node1, node1, "CHN_MAP",
                                NULL, NULL,
                                MXML_DESCEND);
            node2!=NULL;
            node2 = mxmlFindElement(node2, node1, "CHN_MAP",
                                    NULL, NULL,
                                    MXML_DESCEND))
    {
        data[i].smvNum = smvNum;
        s=(char *)mxmlElementGetAttr(node2,"smvAdsuNo");
        if(s)
        {
            data[i].smvAdsuNo=strtol(s,NULL,10)-1;
        }
        s=(char *)mxmlElementGetAttr(node2,"smvAdsuChn");
        if(s)
        {
            data[i].smvAdsuChn=strtol(s,NULL,10)-1;
        }
        s=(char *)mxmlElementGetAttr(node2,"smvDataChn");
        if(s)
        {
            data[i].smvDataChn=strtol(s,NULL,10)-1;
        }
        s=(char *)mxmlElementGetAttr(node2,"smvValIn");
        if(s)
        {
            /* data[i].smvValIn=strtol(s,NULL,10); */
        }
        else
        {
            data[i].smvValIn = 1;
        }

        s=(char *)mxmlElementGetAttr(node2,"smvValOut");
        if(s)
        {
            data[i].smvValOut=strtol(s,NULL,10);
        }
        else
        {
            data[i].smvValOut = 1;
        }

        /* 通道极性判断 */
        if ((data[i].smvDataChn<MAXCHNELS)  && (data[i].smvAdsuNo != INVALID_ANA_CHN_NO) && (data[i].smvAdsuChn != INVALID_ANA_CHN_NO))
        {
            if (data[i].smvValOut >= 0)
            {
                bArrPoleFlag[data[i].smvDataChn] = TRUE;
            }
            else
            {
                bArrPoleFlag[data[i].smvDataChn] = FALSE;
            }
        }

        s=(char *)mxmlElementGetAttr(node2,"YabanID");
        if(s)
        {
            if (s[0] == '\0')
            {
                data[i].MuLinkUSE = FALSE;
                data[i].MuYabanIDStr[0]='\0';/*2013-6-7  ZY */
            }
            else
            {
                /*2013-6-7  ZY */
                if(strlen(s)>32)
                {
                    data[i].MuLinkUSE = FALSE;
                    data[i].MuYabanIDStr[0]='\0';/*2013-6-7  ZY */
                }
                else
                {
                    strcpy(data[i].MuYabanIDStr,s);/*2013-6-7  ZY */

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

                        data[i].MuLinkUSE = FALSE;
                    }
                    else
                    {
                        /* 获取压板号 */
                        data[i].MuLinkUSE = TRUE;
                        data[i].MuLinkNo = iYabanNum;

                        /* 设置压板退出时是否进行无流判断 */
                        SCI_Set_Yaban_JgCurInfo(iYabanNum, data[i].smvDataChn);
                    }/*else结束 */
                }/*else结束 */
            }/*else结束  */
        }/*if(s)结束 */
        else
        {
            data[i].MuLinkUSE=FALSE;
            data[i].MuYabanIDStr[0]='\0';/*2013-6-7  ZY */
        }

        s=(char *)mxmlElementGetAttr(node2,"MuLinkNo");
        if(s)
        {
            static BOOL bLogErr=TRUE;
            if(bLogErr)
            {
                LOG_Write(LOG_INFO, "SMV CHN_MAP配置项MuLinkNo无效,请使用YabanId配置项!\n", NULL);
                bLogErr=FALSE;
            }
        }

        s=(char *)mxmlElementGetAttr(node2,"smvChnType");/*Add smvChnType ZQ 2011-02-15*/
        if(s)
        {
            data[i].smvChnType=strtol(s,NULL,10);
        }
        else
        {
            data[i].smvChnType=1;/*1:Sample /2:Delay /Defalut:1*/
        }


        s=(char *)mxmlElementGetAttr(node2,"smvDes");
        if(s)
        {
            data[i].smvDes=strdup(s);
        }
        if(data[i].smvChnType==1)
        {
            data[i].MuTypeNo=0;
        }
        else if(data[i].smvChnType==2)
        {
            SLOW_MESSAGE_NODE Info;
            j++;
            data[i].MuTypeNo=j;
            Info.type=MUNAMEWR;		/* 写合并单元名称 */
            Info.MuTypeNo = j;		/* 条目号 */
            memcpy(Info.smvDes,data[i].smvDes,SLOW_MSG_MAX_CHAR_NUM);/* 名称 */
            Info.smvDes[SLOW_MSG_MAX_CHAR_NUM-1] = '\0';
            msgQSend(SlowMessage, (char *)&Info, sizeof(SLOW_MESSAGE_NODE), NO_WAIT, MSG_PRI_NORMAL);

            Info.type = MUDELAYWR;	/* 写合并单元延时 */
            Info.MuTypeNo = j;				/* 条目号 */
            Info.MuDelay =   0;		/*延时数值*/
            msgQSend(SlowMessage, (char *)&Info, sizeof(SLOW_MESSAGE_NODE), NO_WAIT, MSG_PRI_NORMAL);
            g_ucDelayChnByAsduNo[data[i].smvAdsuNo] = i;/* 记录该ASDU中通道序号为多少 */
        }

        i++;

    }
    *datanum=i;
    sprintf(t, "%d", j);
    FT_Wr_DSamSts_INI("[MUDELAY]","MUCnt",t);
    return TRUE;

}

BOOL AddAdChannals(IEC_SMV_AD_CHAP *data,mxml_node_t *parentnode,int *datanum)
{
    char *s;
    int i;
    mxml_node_t *node1=parentnode;
    mxml_node_t *node2;

    if(!node1)
        return FALSE;
    i=0;
    for(node2 = mxmlFindElement(node1, node1, "CHN_MAP",
                                NULL, NULL,
                                MXML_DESCEND);
            node2!=NULL;
            node2 = mxmlFindElement(node2, node1, "CHN_MAP",
                                    NULL, NULL,
                                    MXML_DESCEND))
    {
        s=(char *)mxmlElementGetAttr(node2,"smvChn");
        if(s)
        {
            data[i].smvChn=strtol(s,NULL,10)-1;
        }
        s=(char *)mxmlElementGetAttr(node2,"smvChnFactor");
        if(s)
        {
            data[i].smvChnFactor=(float)strtod(s,NULL);
        }
        s=(char *)mxmlElementGetAttr(node2,"smvChnAng");
        if(s)
        {
            data[i].smvChnAng=strtol(s,NULL,10);
        }
        s=(char *)mxmlElementGetAttr(node2,"smvChnZeroCur");
        if(s)
        {
            data[i].smvChnZeroCur=strtol(s,NULL,10);
        }
        s=(char *)mxmlElementGetAttr(node2,"smvDataChn");
        if(s)
        {
            data[i].smvDataChn=strtol(s,NULL,10)-1;
        }
        s=(char *)mxmlElementGetAttr(node2,"smvDes");
        if(s)
        {
            data[i].smvDes=strdup(s);
        }

        i++;

    }
    *datanum=i;
    return TRUE;

}

BOOL AddFT3Channals(IEC_SMV_FT3_CHAP *data,mxml_node_t *parentnode,int *datanum)
{
    char *s;
    int i;
    mxml_node_t *node1=parentnode;
    mxml_node_t *node2;
    int16_t	iYabanNum;

    if(!node1)
        return FALSE;
    i=0;
    for(node2 = mxmlFindElement(node1, node1, "CHN_MAP",
                                NULL, NULL,
                                MXML_DESCEND);
            node2!=NULL;
            node2 = mxmlFindElement(node2, node1, "CHN_MAP",
                                    NULL, NULL,
                                    MXML_DESCEND))
    {
        s=(char *)mxmlElementGetAttr(node2,"smvChn");
        if(s)
        {
            data[i].smvChn=strtol(s,NULL,10)-1;
        }
        s=(char *)mxmlElementGetAttr(node2,"smvDataChn");
        if(s)
        {
            data[i].smvDataChn=strtol(s,NULL,10)-1;
        }
        s=(char *)mxmlElementGetAttr(node2,"YabanID");
        if(s)
        {
            if (s[0] == '\0')
            {
                data[i].MuLinkUSE = FALSE;
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

                    data[i].MuLinkUSE = FALSE;
                }
                else
                {
                    /* 获取压板号 */
                    data[i].MuLinkUSE = TRUE;
                    data[i].MuLinkNo = iYabanNum;
                }
            }
        }
        else
        {
            data[i].MuLinkUSE=FALSE;
        }

        s=(char *)mxmlElementGetAttr(node2,"MuLinkNo");
        if(s)
        {
            static BOOL bLogErr=TRUE;
            if(bLogErr)
            {
                LOG_Write(LOG_INFO, "FT3 CHN_MAP配置项MuLinkNo无效,请使用YabanId配置项!\n", NULL);
                bLogErr=FALSE;
            }
        }

        s=(char *)mxmlElementGetAttr(node2,"smvDes");
        if(s)
        {
            data[i].smvDes=strdup(s);
        }
        i++;

    }
    *datanum=i;
    return TRUE;

}

BOOL AddSLFChannals(IEC_SMV_SLF_CHAP *data,mxml_node_t *parentnode,int *datanum)
{
    char *s;
    int i;
    mxml_node_t *node1=parentnode;
    mxml_node_t *node2;

    if(!node1)
        return FALSE;
    i=0;
    for(node2 = mxmlFindElement(node1, node1, "CHN_MAP",
                                NULL, NULL,
                                MXML_DESCEND);
            node2!=NULL;
            node2 = mxmlFindElement(node2, node1, "CHN_MAP",
                                    NULL, NULL,
                                    MXML_DESCEND))
    {
        s=(char *)mxmlElementGetAttr(node2,"smvDataChnR1");
        if(s)
        {
            data[i].smvDataChnR1=strtol(s,NULL,10)-1;
        }
        else
        {
            data[i].smvDataChnR1=-1;
        }
        s=(char *)mxmlElementGetAttr(node2,"smvDataChnR2");
        if(s)
        {
            data[i].smvDataChnR2=strtol(s,NULL,10)-1;
        }
        else
        {
            data[i].smvDataChnR2=-1;
        }
        s=(char *)mxmlElementGetAttr(node2,"smvDataChnR3");
        if(s)
        {
            data[i].smvDataChnR3=strtol(s,NULL,10)-1;
        }
        else
        {
            data[i].smvDataChnR3=-1;
        }
        s=(char *)mxmlElementGetAttr(node2,"smvDataChn");
        if(s)
        {
            data[i].smvDataChn=strtol(s,NULL,10)-1;
        }
        s=(char *)mxmlElementGetAttr(node2,"smvValIn");
        if(s)
        {
            /* data[i].smvValIn=strtol(s,NULL,10); */
        }
        else
        {
            data[i].smvValIn = 1;
        }
        s=(char *)mxmlElementGetAttr(node2,"smvValOut");
        if(s)
        {
            data[i].smvValOut=strtol(s,NULL,10);
        }
        else
        {
            data[i].smvValOut = 1;
        }

        /* 通道极性判断 */
        if (data[i].smvDataChn<MAXCHNELS)
        {
            if (data[i].smvValOut >= 0)
            {
                bArrPoleFlag[data[i].smvDataChn] = TRUE;
            }
            else
            {
                bArrPoleFlag[data[i].smvDataChn] = FALSE;
            }
        }

        s=(char *)mxmlElementGetAttr(node2,"smvDes");
        if(s)
        {
            data[i].smvDes=strdup(s);
        }
        i++;
    }
    *datanum=i;
    return TRUE;
}

BOOL AddTXChannals(IEC_SMV_TX_CHAP *data,mxml_node_t *parentnode,int *datanum)
{
    char *s;
    int i;
    mxml_node_t *node1=parentnode;
    mxml_node_t *node2;

    if(!node1)
        return FALSE;
    i=0;
    for(node2 = mxmlFindElement(node1, node1, "CHN_MAP",
                                NULL, NULL,
                                MXML_DESCEND);
            node2!=NULL;
            node2 = mxmlFindElement(node2, node1, "CHN_MAP",
                                    NULL, NULL,
                                    MXML_DESCEND))
    {
        s=(char *)mxmlElementGetAttr(node2,"smvDataChn");
        if(s)
        {
            data[i].smvDataChn=strtol(s,NULL,10)-1;
        }
        s=(char *)mxmlElementGetAttr(node2,"smvTxDataChn");
        if(s)
        {
            data[i].smvTxDataChn=strtol(s,NULL,10)-1;
        }
        s=(char *)mxmlElementGetAttr(node2,"smvDes");
        if(s)
        {
            data[i].smvDes=strdup(s);
        }
        i++;

    }
    *datanum=i;
    return TRUE;

}


BOOL LoadSmvCfg(char *filename)
{
    int VerNo[2];
    FILE *fp;
    mxml_node_t *node1=NULL;
    mxml_node_t *node2=NULL;
    mxml_node_t *root_node=NULL;

    char *p;
    char *q;
    int i;
    int smvNum,dataNum;

    fp = fopen(filename, "r");
    if(fp==NULL)
        return FALSE;

    root_node = mxmlLoadFile(NULL,fp,MXML_NO_CALLBACK);

    fclose(fp);
    if(!root_node)
        return FALSE;

    node1 = mxmlFindElement(root_node,root_node, "CONFIG",NULL, NULL,MXML_DESCEND);
    p=(char *)mxmlElementGetAttr(node1,"VERSION");
    if(p)
    {
        for(i=0; i<2; i++)
        {
            q=strtok(p,".");
            VerNo[i]=strtol(q,NULL,10);
            p=NULL;
        }
    }
    else
    {
        return FALSE;
    }
    if(VerNo[0]>CFG_VERSION_MAJOR || VerNo[1]>CFG_VERSION_MINOR)
    {
        printf("配置文件版本VERSION: %d.%d 解析程序版本VERSION: %d.%d\n",
               VerNo[0],VerNo[1],CFG_VERSION_MAJOR,CFG_VERSION_MINOR);
        printf("文件版本大于程序版本，请升级解析程序\n");
        return FALSE;
    }

    node1 = mxmlFindElement(root_node,root_node, "SMV_9_1",NULL, NULL,MXML_DESCEND);
    if(node1)
    {
        UINT16 MUCnt=0;
        smvNum=0;

        for(node2 = mxmlFindElement(node1, node1, "DataSet",
                                    NULL, NULL,
                                    MXML_DESCEND);
                node2!=NULL;
                node2 = mxmlFindElement(node2, node1, "DataSet",
                                        NULL, NULL,
                                        MXML_DESCEND))
        {

            p=(char *)mxmlElementGetAttr(node2,"smvPortChn");
            if(p)
            {
                gSmvCfg.Smv_9_1Cfg[smvNum].smvPortChn=strtol(p,NULL,10)-1;
            }
            p=(char *)mxmlElementGetAttr(node2,"MUL_SRC");
            if(p)
            {
                for(i=0; i<6; i++)
                {
                    q=strtok(p,"-");
                    gSmvCfg.Smv_9_1Cfg[smvNum].smvSrc[i]=strtol(q,NULL,16);
                    p=NULL;
                }
            }
            p=(char *)mxmlElementGetAttr(node2,"APP_ID");
            if(p)
            {
                gSmvCfg.Smv_9_1Cfg[smvNum].appID=strtol(p,NULL,16);
            }
            p=(char *)mxmlElementGetAttr(node2,"TYPE");
            if(p)
            {
                gSmvCfg.Smv_9_1Cfg[smvNum].receiveType=strtol(p,NULL,10);
            }

            p=(char *)mxmlElementGetAttr(node2,"smprate");
            if(p)
            {
                gSmvCfg.Smv_9_1Cfg[smvNum].smprate9_2=strtol(p,NULL,10);
            }

            p=(char *)mxmlElementGetAttr(node2,"forceSyn");
            if(p)
            {
                uint16_t FORCESYN=0;
                FORCESYN=strtol(p,NULL,10);
                if(FORCESYN==1)
                {
                    gSmvCfg.Smv_9_1Cfg[smvNum].forceSyn=TRUE;
                }
                else
                {
                    gSmvCfg.Smv_9_1Cfg[smvNum].forceSyn=FALSE;
                }

            }
            else
            {
                gSmvCfg.Smv_9_1Cfg[smvNum].forceSyn=FALSE;
            }
            p=(char *)mxmlElementGetAttr(node2,"DDA");
            if(p)
            {
                uint16_t DDA=0;
                DDA=strtol(p,NULL,10);
                gSmvCfg.Smv_9_1Cfg[smvNum].DDA=DDA;
            }
            else
            {
                gSmvCfg.Smv_9_1Cfg[smvNum].DDA=0;
            }

            p=(char *)mxmlElementGetAttr(node2,"synPulse");
            if(p)
            {
                uint16_t SYNPULSE=0;
                SYNPULSE=strtol(p,NULL,10);
                if(SYNPULSE==1)
                {
                    gSmvCfg.Smv_9_1Cfg[smvNum].synPulse=TRUE;
                }
                else
                {
                    gSmvCfg.Smv_9_1Cfg[smvNum].synPulse=FALSE;
                }

            }
            else
            {
                gSmvCfg.Smv_9_1Cfg[smvNum].synPulse=FALSE;
            }

            p=(char *)mxmlElementGetAttr(node2,"asduNum");
            if(p)
            {
                gSmvCfg.Smv_9_1Cfg[smvNum].asduNum=strtol(p,NULL,10);
                MUCnt=gSmvCfg.Smv_9_1Cfg[smvNum].asduNum;

                assert (s_smvAsduNum == -1);
                s_smvAsduNum = MUCnt;
            }

            p=(char *)mxmlElementGetAttr(node2,"OPPLINE_TYPE_01");
            if(p)
            {
                gSmvCfg.Smv_9_1Cfg[smvNum].oppLineType[0]=strtol(p,NULL,10);
            }
            p=(char *)mxmlElementGetAttr(node2,"OPPLINE_TYPE_02");
            if(p)
            {
                gSmvCfg.Smv_9_1Cfg[smvNum].oppLineType[1]=strtol(p,NULL,10);
            }
            if(smvNum==0)
            {
                FT_New_DSamSts_INI_File();//初始化创建数字采样状态INI
            }

            gSmvCfg.Smv_9_1Cfg[smvNum].smvID=smvNum;
            dataNum=0;
            AddChannals(gSmvCfg.Smv_9_1Cfg[smvNum].smvData,node2,&dataNum,smvNum);
            gSmvCfg.Smv_9_1Cfg[smvNum].dataNum=dataNum;
            smvNum++;

        }

        gSmvCfg.smvNum=smvNum;
    }
    else
    {
        gSmvCfg.smvNum = 0;
    }

    node1 = mxmlFindElement(root_node,root_node, "SMV_AD",NULL, NULL,MXML_DESCEND);
    if(node1)
    {
        smvNum=0;

        for(node2 = mxmlFindElement(node1, node1, "DataSet",
                                    NULL, NULL,
                                    MXML_DESCEND);
                node2!=NULL;
                node2 = mxmlFindElement(node2, node1, "DataSet",
                                        NULL, NULL,
                                        MXML_DESCEND))
        {
            p=(char *)mxmlElementGetAttr(node2,"smvWaitPoints");
            if(p)
            {
                gSmvADCfg.Smv_AD_Cfg[smvNum].smvWaitPoints=strtol(p,NULL,10);
            }
            p=(char *)mxmlElementGetAttr(node2,"smvZeroCurType");
            if(p)
            {
                gSmvADCfg.Smv_AD_Cfg[smvNum].smvZeroCurType=strtol(p,NULL,10);
            }
            gSmvADCfg.Smv_AD_Cfg[smvNum].smvID = smvNum;

            dataNum=0;
            AddAdChannals(gSmvADCfg.Smv_AD_Cfg[smvNum].smvData,node2,&dataNum);
            gSmvADCfg.Smv_AD_Cfg[smvNum].dataNum=dataNum;
            smvNum++;
        }
        gSmvADCfg.smvNum=smvNum;
    }
    else
    {
        gSmvADCfg.smvNum = 0;
    }

    node1 = mxmlFindElement(root_node,root_node, "SMV_FT3",NULL, NULL,MXML_DESCEND);
    if(node1)
    {
        smvNum=0;

        for(node2 = mxmlFindElement(node1, node1, "DataSet",
                                    NULL, NULL,
                                    MXML_DESCEND);
                node2!=NULL;
                node2 = mxmlFindElement(node2, node1, "DataSet",
                                        NULL, NULL,
                                        MXML_DESCEND))
        {
            p=(char *)mxmlElementGetAttr(node2,"smvPortChn");
            if(p)
            {
                gSmvFT3Cfg.Smv_FT3_Cfg[smvNum].smvPortChn=strtol(p,NULL,10)-1;
            }
            p=(char *)mxmlElementGetAttr(node2,"smvWaitPoints");
            if(p)
            {
                gSmvFT3Cfg.Smv_FT3_Cfg[smvNum].smvWaitPoints=strtol(p,NULL,10);
            }
            p=(char *)mxmlElementGetAttr(node2,"smvType");
            if(p)
            {
                gSmvFT3Cfg.Smv_FT3_Cfg[smvNum].smvType=strtol(p,NULL,10);
            }
            gSmvFT3Cfg.Smv_FT3_Cfg[smvNum].smvID = smvNum;

            dataNum=0;
            AddFT3Channals(gSmvFT3Cfg.Smv_FT3_Cfg[smvNum].smvData,node2,&dataNum);
            gSmvFT3Cfg.Smv_FT3_Cfg[smvNum].dataNum=dataNum;
            smvNum++;
        }
        gSmvFT3Cfg.smvNum=smvNum;
    }
    else
    {
        gSmvFT3Cfg.smvNum = 0;
    }

//ADD SLF
    node1 = mxmlFindElement(root_node,root_node, "SMV_SLF",NULL, NULL,MXML_DESCEND);
    if(node1)
    {
        dataNum=0;
        AddSLFChannals(gSmvSLFCfg.Smv_SLF_Cfg,node1,&dataNum);
        gSmvSLFCfg.smvNum=dataNum;
    }
    else
    {
        gSmvSLFCfg.smvNum = 0;
    }

    node1 = mxmlFindElement(root_node,root_node, "SMV_PUB",NULL, NULL,MXML_DESCEND);
    if(node1)
    {
        smvNum=0;

        for(node2 = mxmlFindElement(node1, node1, "DataSet",
                                    NULL, NULL,
                                    MXML_DESCEND);
                node2!=NULL;
                node2 = mxmlFindElement(node2, node1, "DataSet",
                                        NULL, NULL,
                                        MXML_DESCEND))
        {
            p=(char *)mxmlElementGetAttr(node2,"smvRate");
            if(p)
            {
                gSmvTXCfg.Smv_TX_Cfg[smvNum].smvRate=strtol(p,NULL,10);
            }
            p=(char *)mxmlElementGetAttr(node2,"smvTxType");
            if(p)
            {
                gSmvTXCfg.Smv_TX_Cfg[smvNum].smvTxType=strtol(p,NULL,10);
            }

            p=(char *)mxmlElementGetAttr(node2,"smvAsduNum");
            if(p)
            {
                gSmvTXCfg.Smv_TX_Cfg[smvNum].smvAsduNum=strtol(p,NULL,10);
            }

            p=(char *)mxmlElementGetAttr(node2,"RatedCurrent");
            if(p)
            {
                gSmvTXCfg.Smv_TX_Cfg[smvNum].RatedCurrent=strtol(p,NULL,10);
            }
            p=(char *)mxmlElementGetAttr(node2,"RatedZeroCurren");
            if(p)
            {
                gSmvTXCfg.Smv_TX_Cfg[smvNum].RatedZeroCurren=strtol(p,NULL,10);
            }

            p=(char *)mxmlElementGetAttr(node2,"RatedVoltage");
            if(p)
            {
                gSmvTXCfg.Smv_TX_Cfg[smvNum].RatedVoltage=strtol(p,NULL,10);
            }

            p=(char *)mxmlElementGetAttr(node2,"RatedTime");
            if(p)
            {
                gSmvTXCfg.Smv_TX_Cfg[smvNum].RatedTime=strtol(p,NULL,10);
            }

            gSmvTXCfg.Smv_TX_Cfg[smvNum].smvID = smvNum;

            dataNum=0;
            AddTXChannals(gSmvTXCfg.Smv_TX_Cfg[smvNum].smvData,node2,&dataNum);
            gSmvTXCfg.Smv_TX_Cfg[smvNum].dataNum=dataNum;
            smvNum++;
        }
        gSmvTXCfg.smvNum=smvNum;
    }
    else
    {
        gSmvTXCfg.smvNum = 0;
    }

    mxmlDelete(root_node);
    return TRUE;
}

IEC_SMV_9_1_CFG *GetSmvChn(uint8_t* addr, uint16_t appID)
{
    UINT8 i;
    IEC_SMV_9_1_CFG *config = NULL;
    for(i=0; i<gSmvCfg.smvNum; i++)
    {
        config = &(gSmvCfg.Smv_9_1Cfg[i]);
        if((memcmp(addr, config->smvSrc, 6) == 0)&&(appID == config->appID))
            break;
    }
    if(i== gSmvCfg.smvNum)
        return NULL;
    return config;
}

void showPool()
{
    int i,k;

    if(LoadSmvCfg(SMV_CFG_FILE)==FALSE)
        return ;

    for(i=0; i<gSmvCfg.smvNum; i++)
    {
        printf("<MUL_SRC=%02x-%02x-%02x-%02x-%02x-%02x   ",gSmvCfg.Smv_9_1Cfg[i].smvSrc[0],
               gSmvCfg.Smv_9_1Cfg[i].smvSrc[1],gSmvCfg.Smv_9_1Cfg[i].smvSrc[2],
               gSmvCfg.Smv_9_1Cfg[i].smvSrc[3],gSmvCfg.Smv_9_1Cfg[i].smvSrc[4],
               gSmvCfg.Smv_9_1Cfg[i].smvSrc[5]);
        printf("ID=%d  PORT=%d  APP_ID=%x  TYPE=%d   asduNum=%d  dataNum=%d smprate=%d forceSyn=%d DDA=%d synPulse=%d >\n",gSmvCfg.Smv_9_1Cfg[i].smvID,gSmvCfg.Smv_9_1Cfg[i].smvPortChn,gSmvCfg.Smv_9_1Cfg[i].appID,gSmvCfg.Smv_9_1Cfg[i].receiveType,
               gSmvCfg.Smv_9_1Cfg[i].asduNum,gSmvCfg.Smv_9_1Cfg[i].dataNum,gSmvCfg.Smv_9_1Cfg[i].smprate9_2,gSmvCfg.Smv_9_1Cfg[i].forceSyn,gSmvCfg.Smv_9_1Cfg[i].DDA,gSmvCfg.Smv_9_1Cfg[i].synPulse);
        printf("<OPPLINE_TYPE_01=%d OPPLINE_TYPE_02=%d>\n",gSmvCfg.Smv_9_1Cfg[i].oppLineType[0],gSmvCfg.Smv_9_1Cfg[i].oppLineType[1]);

        for(k=0; k<gSmvCfg.Smv_9_1Cfg[i].dataNum; k++)
        {
            printf("   smvAdsuNo=%d  smvAdsuChn=%d  smvDataChn=%d  smvValIn=%d  smvValOut=%d  MuLinkNo=%d smvChnType=%d smvDes=%s\n",gSmvCfg.Smv_9_1Cfg[i].smvData[k].smvAdsuNo,
                   gSmvCfg.Smv_9_1Cfg[i].smvData[k].smvAdsuChn,
                   gSmvCfg.Smv_9_1Cfg[i].smvData[k].smvDataChn,
                   (int)gSmvCfg.Smv_9_1Cfg[i].smvData[k].smvValIn,
                   gSmvCfg.Smv_9_1Cfg[i].smvData[k].smvValOut,
                   gSmvCfg.Smv_9_1Cfg[i].smvData[k].MuLinkNo,
                   gSmvCfg.Smv_9_1Cfg[i].smvData[k].smvChnType,
                   gSmvCfg.Smv_9_1Cfg[i].smvData[k].smvDes);
        }

    }

    for(i=0; i<gSmvADCfg.smvNum; i++)
    {
        printf("<smvWaitPoints = %d  smvZeroCurType = %d  dataNum=%d>\n",
               gSmvADCfg.Smv_AD_Cfg[i].smvWaitPoints,
               gSmvADCfg.Smv_AD_Cfg[i].smvZeroCurType,gSmvADCfg.Smv_AD_Cfg[i].dataNum);

        for(k=0; k<gSmvADCfg.Smv_AD_Cfg[i].dataNum; k++)
        {
            printf("   smvChn = %d  smvChnFactor = %f  smvChnAng = %d  smvChnZeroCur = %d smvDataChn = %d smvDes=%s\n",
                   gSmvADCfg.Smv_AD_Cfg[i].smvData[k].smvChn,
                   gSmvADCfg.Smv_AD_Cfg[i].smvData[k].smvChnFactor,
                   gSmvADCfg.Smv_AD_Cfg[i].smvData[k].smvChnAng,
                   gSmvADCfg.Smv_AD_Cfg[i].smvData[k].smvChnZeroCur,
                   gSmvADCfg.Smv_AD_Cfg[i].smvData[k].smvDataChn,
                   gSmvADCfg.Smv_AD_Cfg[i].smvData[k].smvDes);
        }
    }

    for(i=0; i<gSmvFT3Cfg.smvNum; i++)
    {
        printf("<smvPortChn = %d  smvWaitPoints = %d  smvType = %d  dataNum=%d>\n",
               gSmvFT3Cfg.Smv_FT3_Cfg[i].smvPortChn,
               gSmvFT3Cfg.Smv_FT3_Cfg[i].smvWaitPoints,
               gSmvFT3Cfg.Smv_FT3_Cfg[i].smvType,gSmvFT3Cfg.Smv_FT3_Cfg[i].dataNum);

        for(k=0; k<gSmvFT3Cfg.Smv_FT3_Cfg[i].dataNum; k++)
        {
            printf("  smvChn = %d  smvDataChn = %d smvDes=%s\n",
                   gSmvFT3Cfg.Smv_FT3_Cfg[i].smvData[k].smvChn,
                   gSmvFT3Cfg.Smv_FT3_Cfg[i].smvData[k].smvDataChn,
                   gSmvFT3Cfg.Smv_FT3_Cfg[i].smvData[k].smvDes);
        }
    }

    for(i=0; i<gSmvSLFCfg.smvNum; i++)
    {
        printf("<smvDataChnR1 = %d  smvDataChnR2 = %d  smvDataChnR3 = %d  smvDataChn=%d   smvDes=%s>\n",
               gSmvSLFCfg.Smv_SLF_Cfg[i].smvDataChnR1,
               gSmvSLFCfg.Smv_SLF_Cfg[i].smvDataChnR2,
               gSmvSLFCfg.Smv_SLF_Cfg[i].smvDataChnR3,
               gSmvSLFCfg.Smv_SLF_Cfg[i].smvDataChn,
               gSmvSLFCfg.Smv_SLF_Cfg[i].smvDes);
    }


    for(i=0; i<gSmvTXCfg.smvNum; i++)
    {
        printf("<smvRate = %d  smvTxType = %d smvAsduNum = %d RatedCurrent = %d RatedZeroCurren = %d RatedVoltage = %d RatedTime = %d  dataNum=%d>\n",
               gSmvTXCfg.Smv_TX_Cfg[i].smvRate,
               gSmvTXCfg.Smv_TX_Cfg[i].smvTxType,
               gSmvTXCfg.Smv_TX_Cfg[i].smvAsduNum,
               gSmvTXCfg.Smv_TX_Cfg[i].RatedCurrent,
               gSmvTXCfg.Smv_TX_Cfg[i].RatedZeroCurren,
               gSmvTXCfg.Smv_TX_Cfg[i].RatedVoltage,
               gSmvTXCfg.Smv_TX_Cfg[i].RatedTime,
               gSmvTXCfg.Smv_TX_Cfg[i].dataNum);

        for(k=0; k<gSmvTXCfg.Smv_TX_Cfg[i].dataNum; k++)
        {
            printf("   smvDataChn = %d  smvTxDataChn = %d smvDes=%s\n",
                   gSmvTXCfg.Smv_TX_Cfg[i].smvData[k].smvDataChn,
                   gSmvTXCfg.Smv_TX_Cfg[i].smvData[k].smvTxDataChn,
                   gSmvTXCfg.Smv_TX_Cfg[i].smvData[k].smvDes);
        }
    }
}

/* 通过ASDU序号获取间隔描述
 * Para:
 *     nAsduNo, ASDU序号,从0开始计数
 *     pDesc, 间隔描述字符串输出的目的指针
 * Return:
 *     TRUE, 成功 FALSE, 失败
 */
BOOL GetBayDesByAsduNo(int nAsduNo, char *pDesc)
{
    int iChnNum;
    int iLen;
    char *pDelayDes;

    iChnNum = g_ucDelayChnByAsduNo[nAsduNo-1];
    pDelayDes = strstr(gSmvCfg.Smv_9_1Cfg[0].smvData[iChnNum].smvDes,"额定延时");
    if(pDelayDes != NULL)
    {
        iLen = (int)(pDelayDes-gSmvCfg.Smv_9_1Cfg[0].smvData[iChnNum].smvDes);
        memcpy(pDesc, gSmvCfg.Smv_9_1Cfg[0].smvData[iChnNum].smvDes, iLen);
        pDesc[iLen] = '\0';
        return TRUE;
    }

    return FALSE;
}

/* 获取过程层配置ASDU数.
 * Para:
 *     NONE.
 * Return:
 *     为-1时无效.
 */
int32_t SMV_GetAsduNum(void)
{
    return s_smvAsduNum;
}

