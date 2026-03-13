#include <ioLib.h>
#include <stdio_compat.h>
#include <semLib.h>
#include "Smv_Go_CommStat_File.h"
// #include "filetool.h"
#include "smvcfg.h"
#include "smv_rx.h"
// #include "Hdl_Data.h"
#include "rec.h"
#include "AppInterface.h"
// #include "Realdata.h"
#include "iecgoose.h"
#include "smvcfg.h"
#include "edp_asst.h"

extern uint8_t g_ucPortNotoBoardId[MAX_CC_PORT_NUM];/*通过主CC的级联端口获取CC板的BoardId，主CC的级联端口为0*/
extern uint8_t g_ucBoardIdtoCcStsNo[MAX_CC_BOARD_ID_NUM];/* 通过BoardId板件号获取CC状态结构序号(光功率结构) */

/*得到网络风暴状态*/
extern BOOL EP_GetStormState();

SEM_ID semSGCommStatChg;

void Init_Smv_Go_CommStat_File_Log()
{
    semSGCommStatChg = semBCreate(SEM_Q_FIFO,SEM_FULL);
}

void Smv_Go_CommStat_Chg()
{
    semGive(semSGCommStatChg);
}

EP_STATUS Insert_FPGA_Smv_CommStat_Info(char *pContent,int *pLenth)
{
    int i;
    char strtmp[256];
    int strlenth=0;

    if((pContent==NULL)||(pLenth==NULL))
    {
        return EP_ERROR;
    }

    strcpy(pContent,"      采样通信状态\n\n");
    strlenth=strlen(pContent);

    if (sFpgaCfg.adPara.bUsed)
    {
        if(FPGASmvCommStat[0]&(~((uint32_t)(AI_DAT_SYN|AI_DAT_VLD|AI_TEST_DAT))))
        {
            sprintf(strtmp,"本地AD采样状态:异常(0x%08lX)\n",FPGASmvCommStat[0]);
        }
        else
        {
            sprintf(strtmp,"本地AD采样状态:正常(0x%08lX)\n",FPGASmvCommStat[0]);
        }
        strlenth+=strlen(strtmp);
        if(strlenth+1>*pLenth)
        {
            *pLenth=0;
            return EP_ERROR;
        }
        strcat(pContent,strtmp);
    }

    if (sFpgaCfg.adParaRepeat.bUsed)
    {
        if(FPGASmvCommStat[1]&(~((uint32_t)(AI_DAT_SYN|AI_DAT_VLD|AI_TEST_DAT))))
        {
            sprintf(strtmp,"本地AD复采状态:异常(0x%08lX)\n",FPGASmvCommStat[1]);
        }
        else
        {
            sprintf(strtmp,"本地AD复采状态:正常(0x%08lX)\n",FPGASmvCommStat[1]);
        }
        strlenth+=strlen(strtmp);
        if(strlenth+1>*pLenth)
        {
            *pLenth=0;
            return EP_ERROR;
        }
        strcat(pContent,strtmp);
    }

    for (i = 0; i<FT3_PORT_NUM; i++)
    {
        if (sFpgaCfg.ft3Para[i].bUsed)
        {
            if(FPGASmvCommStat[2+i]&(~((uint32_t)(AI_DAT_SYN|AI_DAT_VLD|AI_TEST_DAT))))
            {
                sprintf(strtmp,"FT3采样%d口状态:异常(0x%08lX)\n",i,FPGASmvCommStat[2+i]);
            }
            else
            {
                sprintf(strtmp,"FT3采样%d口状态:正常(0x%08lX)\n",i,FPGASmvCommStat[2+i]);
            }
            strlenth+=strlen(strtmp);
            if(strlenth+1>*pLenth)
            {
                return EP_ERROR;
            }
            strcat(pContent,strtmp);
        }
    }

    for (i = 0; i<SMV_RCV_PORT_NUM; i++)
    {
        if (sFpgaCfg.svRcvPara[i].bUsed)
        {
            if(FPGASmvCommStat[2+FT3_PORT_NUM+i]&(~((uint32_t)(AI_DAT_SYN|AI_DAT_VLD|AI_TEST_DAT))))
            {
                sprintf(strtmp,"IEC61850-9-2采样%d口状态:异常(0x%08lX)\n",i,FPGASmvCommStat[2+FT3_PORT_NUM+i]);
            }
            else
            {
                sprintf(strtmp,"IEC61850-9-2采样%d口状态:正常(0x%08lX)\n",i,FPGASmvCommStat[2+FT3_PORT_NUM+i]);
            }
            strlenth+=strlen(strtmp);
            if(strlenth+1>*pLenth)
            {
                *pLenth=0;
                return EP_ERROR;
            }
            strcat(pContent,strtmp);
        }
    }

    strcat(pContent,"\n");
    *pLenth=strlenth+1;
    return EP_SUCCESS;
}


EP_STATUS Insert_FrontEnd_Smv_CommStat_Info(char *pContent,int *pLenth)
{
    char strtmp[256];
    int strlenth=0;
    int i;

    if((pContent==NULL)||(pLenth==NULL))
    {
        return EP_ERROR;
    }

    strcpy(pContent,"      采样通信状态\n\n");
    strlenth=strlen(pContent);

    for(i=0; i<gSmvCfg.smvNum; i++)
    {
        switch(gSmvCfg.Smv_9_1Cfg[i].receiveType)
        {
            case 0:
            case 3:
            case 9:
            case 14:
                break;
            case 8:
                if(sSmvData[i].bSmvCommOk)
                {
                    sprintf(strtmp,"CPU-DCU采样通信状态(SMV%02u):正常\n",gSmvCfg.Smv_9_1Cfg[i].smvID);
                }
                else
                {
                    sprintf(strtmp,"CPU-DCU采样通信状态(SMV%02u):异常\n",gSmvCfg.Smv_9_1Cfg[i].smvID);
                }
                strlenth+=strlen(strtmp);
                if(strlenth+1>*pLenth)
                {
                    *pLenth=0;
                    return EP_ERROR;
                }
                strcat(pContent,strtmp);
                break;
            case 15:
                if(sSmvData[i].bSmvCommOk)
                {
                    sprintf(strtmp,"内部总线采样通信状态(SMV%02u):正常\n",gSmvCfg.Smv_9_1Cfg[i].smvID);
                }
                else
                {
                    sprintf(strtmp,"内部总线采样通信状态(SMV%02u):异常\n",gSmvCfg.Smv_9_1Cfg[i].smvID);
                }
                strlenth+=strlen(strtmp);
                if(strlenth+1>*pLenth)
                {
                    *pLenth=0;
                    return EP_ERROR;
                }
                strcat(pContent,strtmp);
                break;
            default:
                break;
        }
    }

    strcat(pContent,"\n");
    *pLenth=strlenth+1;
    return EP_SUCCESS;
}

EP_STATUS Insert_CC_Smv_CommStat_Info(char *pContent,int *pLenth)
{
    char strtmp[256];
    int strlenth=0;
    int i,j,k;
    int asducnt=0;
    uint8_t TempInfo[256];
    uint8_t ucBayDesc[64];
    int iMasterCcPort = 0;
    int iSlaverCcPort = 0;
    int iCcStsNo = 0;

    if((pContent==NULL)||(pLenth==NULL))
    {
        return EP_ERROR;
    }

    strcpy(pContent,"      采样通信状态\n\n");
    strlenth=strlen(pContent);

    for(i=0; i<gSmvCfg.smvNum; i++)
    {
        if(sSmvData[i].bSmvCommOk)
        {
            sprintf(strtmp,"CPU-CC采样通信状态(SMV%02u):正常\n",gSmvCfg.Smv_9_1Cfg[i].smvID);
        }
        else
        {
            sprintf(strtmp,"CPU-CC采样通信状态(SMV%02u):异常\n",gSmvCfg.Smv_9_1Cfg[i].smvID);
        }
        strlenth+=strlen(strtmp);
        if(strlenth+1>*pLenth)
        {
            *pLenth=0;
            return EP_ERROR;
        }
        strcat(pContent,strtmp);

        if(sSmvData[i].bSmvCommOk)
        {
            for(j=0; j<SmvStruct[i].allpackno; j++)
            {
                for(k=0; k<SmvStruct[i].packInfo[j].asduNum; k++)
                {
                    asducnt++;
                    if(SmvStruct[i].asduInfo[j][k].bPortStsFlag)
                    {
                        if(sSmvCCInfo.bP2PorNet)
                        {

                            /* 数据源确定 */
                            if (SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.V0
                                    && SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.V0)
                            {
                                if (SmvStruct[i].asduInfo[j][k].sInfoBak.ulSlaveInfoBak&SLAVE_STS_MASK_WITHOUT_CONFREV_CHECK)
                                {
                                    sprintf(TempInfo, "无效");
                                }
                                else
                                {
                                    sprintf(TempInfo, "前级");
                                }
                            }
                            else if (SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.V0)
                            {
                                if (SmvStruct[i].asduInfo[j][k].muInfoBak.ulMuInfoBak&MU_STS_MASK_WITHOUT_CONFREV_CHECK)
                                {
                                    sprintf(TempInfo, "无效");
                                }
                                else
                                {
                                    sprintf(TempInfo, "本级");
                                }
                            }
                            else
                            {
                                sprintf(TempInfo, "无效");
                            }

                            if(SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.V0)
                            {
                                if(GetBayDesByAsduNo(asducnt,ucBayDesc))
                                {
                                    sprintf(strtmp, "ASDU%02d  %s 数据来源:%s\n",
                                            asducnt, ucBayDesc, TempInfo);
                                }
                                else
                                {
                                    sprintf(strtmp, "ASDU%02d  %-6s 数据来源:%s\n",
                                            asducnt, SmvStruct[i].asduInfo[j][k].arrSVID, TempInfo);
                                }

                                strlenth+=strlen(strtmp);
                                if(strlenth+1>*pLenth)
                                {
                                    *pLenth=0;
                                    return EP_ERROR;
                                }
                                strcat(pContent,strtmp);

                                iMasterCcPort = SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.muNetNo;
                                if(SmvStruct[i].asduInfo[j][k].muInfoBak.ulMuInfoBak&MU_STS_MASK_WITHOUT_CONFREV_CHECK)
                                {
                                    sprintf(strtmp,"  本级:网口%02u 状态:异常(0x%04X)\n",
                                            SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.muNetNo,(uint16_t)(SmvStruct[i].asduInfo[j][k].muInfoBak.ulMuInfoBak&0xFFFF));
                                }
                                else
                                {
                                    sprintf(strtmp,"  本级:网口%02u 状态:正常(0x%04X)\n",
                                            SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.muNetNo,(uint16_t)(SmvStruct[i].asduInfo[j][k].muInfoBak.ulMuInfoBak&0xFFFF));
                                }
                                strlenth+=strlen(strtmp);
                                if(strlenth+1>*pLenth)
                                {
                                    *pLenth=0;
                                    return EP_ERROR;
                                }
                                strcat(pContent,strtmp);

                                if(SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.V0)
                                {
                                    iSlaverCcPort = SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.sNetNo;
                                    if(SmvStruct[i].asduInfo[j][k].sInfoBak.ulSlaveInfoBak&SLAVE_STS_MASK_WITHOUT_CONFREV_CHECK)
                                    {
                                        sprintf(strtmp,"  前级:网口%02u 状态:异常(0x%04X)\n",
                                                SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.sNetNo,(uint16_t)(SmvStruct[i].asduInfo[j][k].sInfoBak.ulSlaveInfoBak&0xFFFF));
                                    }
                                    else
                                    {
                                        sprintf(strtmp,"  前级:网口%02u 状态:正常(0x%04X)\n",
                                                SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.sNetNo,(uint16_t)(SmvStruct[i].asduInfo[j][k].sInfoBak.ulSlaveInfoBak&0xFFFF));
                                    }
                                    strlenth+=strlen(strtmp);
                                    if(strlenth+1>*pLenth)
                                    {
                                        *pLenth=0;
                                        return EP_ERROR;
                                    }
                                    strcat(pContent,strtmp);

                                    if(!isNumber_2_04CPU())
                                    {
                                        sprintf(strtmp,"  通讯端口:%d-%c\n", g_ucPortNotoBoardId[iMasterCcPort]
                                                , 'A'+SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.sNetNo);
                                        strlenth+=strlen(strtmp);
                                        if(strlenth+1>*pLenth)
                                        {
                                            *pLenth=0;
                                            return EP_ERROR;
                                        }
                                        strcat(pContent,strtmp);

                                        iCcStsNo = g_ucBoardIdtoCcStsNo[g_ucPortNotoBoardId[iMasterCcPort]-1];
                                        sprintf(strtmp,"  流量统计:%lu帧/秒\n  帧间隔异常:%lu次 \n",
                                                arrCcPortSts[iCcStsNo].tOptPortSts[iSlaverCcPort].ulSvFlowCnt,
                                                arrCcPortSts[iCcStsNo].tOptPortSts[iSlaverCcPort].ulSvIntervalErrCnt);
                                        strlenth+=strlen(strtmp);
                                        if(strlenth+1>*pLenth)
                                        {
                                            *pLenth=0;
                                            return EP_ERROR;
                                        }
                                        strcat(pContent,strtmp);
                                    }
                                }
                                else
                                {
                                    if(!isNumber_2_04CPU())
                                    {
                                        sprintf(strtmp,"  通讯端口:%d-%c\n", g_ucPortNotoBoardId[0]
                                                , 'A'+SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.muNetNo);
                                        strlenth+=strlen(strtmp);
                                        if(strlenth+1>*pLenth)
                                        {
                                            *pLenth=0;
                                            return EP_ERROR;
                                        }
                                        strcat(pContent,strtmp);

                                        iCcStsNo = g_ucBoardIdtoCcStsNo[g_ucPortNotoBoardId[0]-1];
                                        sprintf(strtmp,"  流量统计:%lu帧/秒\n  帧间隔异常:%lu次 \n",
                                                arrCcPortSts[iCcStsNo].tOptPortSts[iMasterCcPort].ulSvFlowCnt,
                                                arrCcPortSts[iCcStsNo].tOptPortSts[iMasterCcPort].ulSvIntervalErrCnt);
                                        strlenth+=strlen(strtmp);
                                        if(strlenth+1>*pLenth)
                                        {
                                            *pLenth=0;
                                            return EP_ERROR;
                                        }
                                        strcat(pContent,strtmp);
                                    }
                                }
                            }
                        }
                        else
                        {
                            /* 数据源确定 */
                            if (SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.V0 && SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.V0)
                            {
                                if (SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.s13)
                                {
                                    sprintf(TempInfo, "A网");
                                }
                                else
                                {
                                    if (SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.s13)
                                    {
                                        sprintf(TempInfo, "B网");
                                    }
                                    else
                                    {
                                        sprintf(TempInfo, "无效");
                                    }
                                }
                            }
                            else if (SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.V0)
                            {
                                if (SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.s13)
                                {
                                    sprintf(TempInfo, "A网");
                                }
                                else
                                {
                                    sprintf(TempInfo, "无效");
                                }
                            }
                            else if (SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.V0)
                            {
                                if (SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.s13)
                                {
                                    sprintf(TempInfo, "B网");
                                }
                                else
                                {
                                    sprintf(TempInfo, "无效");
                                }
                            }
                            else
                            {
                                sprintf(TempInfo, "无效");
                            }

                            if(SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.V0)
                            {
                                if(GetBayDesByAsduNo(asducnt,ucBayDesc))
                                {
                                    sprintf(strtmp, "ASDU%02d  %s 数据来源:%s\n",
                                            asducnt, ucBayDesc, TempInfo);
                                }
                                else
                                {
                                    sprintf(strtmp, "ASDU%02d  %-6s 数据来源:%s\n",
                                            asducnt, SmvStruct[i].asduInfo[j][k].arrSVID, TempInfo);
                                }

                                strlenth+=strlen(strtmp);
                                if(strlenth+1>*pLenth)
                                {
                                    *pLenth=0;
                                    return EP_ERROR;
                                }
                                strcat(pContent,strtmp);

                                if(SmvStruct[i].asduInfo[j][k].muInfoBak.ulMuInfoBak&NET_AB_MAST_WITHOUT_CONFREV_CHECK)
                                {
                                    sprintf(strtmp,"  A网 网口%02u 状态:异常(0x%04X)\n",
                                            SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.muNetNo,(uint16_t)(SmvStruct[i].asduInfo[j][k].muInfoBak.ulMuInfoBak&0xFFFF));
                                }
                                else
                                {
                                    sprintf(strtmp,"  A网 网口%02u 状态:正常(0x%04X)\n",
                                            SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.muNetNo,(uint16_t)(SmvStruct[i].asduInfo[j][k].muInfoBak.ulMuInfoBak&0xFFFF));
                                }
                                strlenth+=strlen(strtmp);
                                if(strlenth+1>*pLenth)
                                {
                                    *pLenth=0;
                                    return EP_ERROR;
                                }
                                strcat(pContent,strtmp);
                            }
                            if(SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.V0)
                            {
                                if(SmvStruct[i].asduInfo[j][k].muInfoBak.muInfo_st_bak.V0)
                                {
                                    if(SmvStruct[i].asduInfo[j][k].sInfoBak.ulSlaveInfoBak&NET_AB_MAST_WITHOUT_CONFREV_CHECK)
                                    {
                                        sprintf(strtmp,"  B网 网口%02u 状态:异常(0x%04X)\n",
                                                SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.sNetNo,(uint16_t)(SmvStruct[i].asduInfo[j][k].sInfoBak.ulSlaveInfoBak&0xFFFF));
                                    }
                                    else
                                    {
                                        sprintf(strtmp,"  B网 网口%02u 状态:正常(0x%04X)\n",
                                                SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.sNetNo,(uint16_t)(SmvStruct[i].asduInfo[j][k].sInfoBak.ulSlaveInfoBak&0xFFFF));
                                    }
                                }
                                else
                                {
                                    if(GetBayDesByAsduNo(asducnt,ucBayDesc))
                                    {
                                        sprintf(strtmp, "ASDU%02d  %s 数据来源:%s\n",
                                                asducnt, ucBayDesc, TempInfo);
                                    }
                                    else
                                    {
                                        sprintf(strtmp, "ASDU%02d  %-6s 数据来源:%s\n",
                                                asducnt, SmvStruct[i].asduInfo[j][k].arrSVID, TempInfo);
                                    }

                                    strlenth+=strlen(strtmp);
                                    if(strlenth+1>*pLenth)
                                    {
                                        *pLenth=0;
                                        return EP_ERROR;
                                    }
                                    strcat(pContent,strtmp);

                                    if(SmvStruct[i].asduInfo[j][k].sInfoBak.ulSlaveInfoBak&NET_AB_MAST_WITHOUT_CONFREV_CHECK)
                                    {
                                        sprintf(strtmp,"  B网 网口%02u 状态:异常(0x%04X)\n",
                                                SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.sNetNo,(uint16_t)(SmvStruct[i].asduInfo[j][k].sInfoBak.ulSlaveInfoBak&0xFFFF));
                                    }
                                    else
                                    {
                                        sprintf(strtmp,"  B网 网口%02u 状态:正常(0x%04X)\n",
                                                SmvStruct[i].asduInfo[j][k].sInfoBak.sInfo_st_bak.sNetNo,(uint16_t)(SmvStruct[i].asduInfo[j][k].sInfoBak.ulSlaveInfoBak&0xFFFF));
                                    }
                                }
                                strlenth+=strlen(strtmp);
                                if(strlenth+1>*pLenth)
                                {
                                    *pLenth=0;
                                    return EP_ERROR;
                                }
                                strcat(pContent,strtmp);
                            }
                        }
                    }
                }
            }
        }
    }

    strcat(pContent,"\n");
    *pLenth=strlenth+1;
    return EP_SUCCESS;
}


EP_STATUS Insert_Goose_CommStat_Info(char *pContent,int *pLenth)
{
    int i,j;
    char strtmp[TEMP_INFO_MAX_LEN];
    int strlenth=0;
    int subnum=0;
    int netnum=0;
    int netcfgnum=0;
    GSE_SUB_INFO *p = NULL;
    GSE_PUB_INFO *pPub = GetPubInfoRootNode();

    if((pContent==NULL)||(pLenth==NULL))
    {
        return EP_ERROR;
    }

    strcpy(pContent,"      Goose通信状态\n\n");
    strlenth=strlen(pContent);

    Get_Goose_SubNum_and_NetNum(&subnum,&netnum);

    if(subnum == 0)
    {
        /* 如果配置中没有GOOSE接收块，则返回错误 */
        return EP_ERROR;
    }

    sprintf(strtmp, "风暴抑制状态:%s\n", EP_GetStormState()? "是": "否");
    strlenth+=strlen(strtmp);
    if(strlenth+2>*pLenth)
    {
        *pLenth=0;
        return EP_ERROR;
    }
    strcat(pContent,strtmp);

    for(i=0; i<subnum; i++)
    {
        netcfgnum=0;
        p = QuerySubByIdx(i+1);
        sprintf(strtmp, "SUB%02d  %s\n  接收帧数:%u\n  更新帧数:%u\n  配置错误帧数:%u\n", i+1,
                QuerySubIEDNameByIdx(i+1), (unsigned int)(p->ulRcvCnt),
                (unsigned int)(p->ulRcvRrmCnt),
                (unsigned int)(p->ulRcvCfgErrCnt));

        strlenth += strlen(strtmp);
        if(strlenth+2>*pLenth)
        {
            *pLenth=0;
            return EP_ERROR;
        }
        strcat(pContent,strtmp);

        for(j=0; j<netnum; j++)
        {
            if(Get_Goose_Sub_Net_Cfg(i,j))
            {
                if(Get_Goose_Comm_Status(i,j))
                {
                    sprintf(strtmp,"    网络%02d: %s\n",j,"正常");
                }
                else
                {
                    sprintf(strtmp,"    网络%02d: %s\n",j,"异常");
                }

                strlenth+=strlen(strtmp);
                if(strlenth+2>*pLenth)
                {
                    *pLenth=0;
                    return EP_ERROR;
                }
                strcat(pContent,strtmp);

                netcfgnum++;
            }
        }
        /*
        strcat(pContent,"\n");
        strlenth++;
        */
    }

    /* 发送PUB */
    strcat(pContent,"\n");
    strlenth++;
    while (pPub)
    {
        sprintf(strtmp, "PUB%02d  %s 发送帧数: %u\n", pPub->GcbIndex,
                pPub->UserInfo.pIEDName, (unsigned int)(pPub->ulSndCnt));

        strlenth+=strlen(strtmp);
        if(strlenth+2>*pLenth)
        {
            *pLenth=0;

            return EP_ERROR;
        }
        strcat(pContent,strtmp);

        netcfgnum++;

        /*
        strcat(pContent,"\n");
        strlenth++;
        */

        pPub = pPub->next;
    }

    if(netcfgnum==0)
    {
        sprintf(strtmp,"    无效配置");
        strlenth+=strlen(strtmp);
        if(strlenth+2>*pLenth)
        {
            *pLenth=0;
            return EP_ERROR;
        }
        strcat(pContent,strtmp);
    }

    strcat(pContent,"\n");
    *pLenth=strlenth+1;
    return EP_SUCCESS;
}

EP_STATUS Create_Smv_Go_CommStat_File()
{
    int iFd;
    char *pContent=NULL;
    int ContentLen=0;
    int MaxLen=24000;
    BOOL bSaveFile = FALSE;/* 根据能否读取到采样GOOSE的通信状态，判断是否保存通信状态文件 */

    pContent=(char *)malloc(sizeof(char)*MaxLen);
    if(pContent==NULL)
    {
        return EP_ERROR;
    }

    iFd=creatInDataDisk(SMV_GO_COMM_STAT_FILE".bak", O_RDWR);

    if(iFd==ERROR)
    {
        if(FT_Is_File(SMV_GO_COMM_STAT_FILE))
        {
            remove(SMV_GO_COMM_STAT_FILE);
        }
        if(pContent!=NULL)
            free(pContent);
        return EP_ERROR;
    }

    /*SMV通信状态*/
    ContentLen=MaxLen;
    switch(gSmvCfg.Smv_9_1Cfg[0].receiveType)
    {
        case 0:
            break;
        case 3:
        case 9:
            if(Insert_CC_Smv_CommStat_Info(pContent,&ContentLen)==EP_SUCCESS)
            {
                if(write(iFd,pContent,ContentLen)!=ContentLen)
                {
                    if(pContent!=NULL)
                        free(pContent);
                    close(iFd);
                    return EP_ERROR;
                }
                else
                {
                    bSaveFile = TRUE;
                }
            }
            break;
        case 8:
        case 15:
            if(Insert_FrontEnd_Smv_CommStat_Info(pContent,&ContentLen)==EP_SUCCESS)
            {
                if(write(iFd,pContent,ContentLen)!=ContentLen)
                {
                    if(pContent!=NULL)
                        free(pContent);
                    close(iFd);
                    return EP_ERROR;
                }
                else
                {
                    bSaveFile = TRUE;
                }
            }
            break;
        case 14:
            if(Insert_FPGA_Smv_CommStat_Info(pContent,&ContentLen)==EP_SUCCESS)
            {
                if(write(iFd,pContent,ContentLen)!=ContentLen)
                {
                    if(pContent!=NULL)
                        free(pContent);
                    close(iFd);
                    return EP_ERROR;
                }
                else
                {
                    bSaveFile = TRUE;
                }
            }
            break;
        default:
            break;
    }

    /*GOOSE通信状态*/
    ContentLen=MaxLen;
    if(Insert_Goose_CommStat_Info(pContent,&ContentLen)==EP_SUCCESS)
    {
        if(write(iFd,pContent,ContentLen)!=ContentLen)
        {
            if(pContent!=NULL)
                free(pContent);
            close(iFd);
            return EP_ERROR;
        }
        else
        {
            bSaveFile = TRUE;
        }
    }

    close(iFd);
    if(pContent!=NULL)
        free(pContent);

    if(bSaveFile)
    {
        /* 如果读取到GOOSE或SV的通讯状态则生成相应文件，否则删除所有文件 */
        if(FT_Is_File(SMV_GO_COMM_STAT_FILE))
        {
            remove(SMV_GO_COMM_STAT_FILE);
        }

        if(rename(SMV_GO_COMM_STAT_FILE".bak",SMV_GO_COMM_STAT_FILE)==ERROR)
        {
            return EP_ERROR;
        }
    }
    else
    {
        if(FT_Is_File(SMV_GO_COMM_STAT_FILE))
        {
            remove(SMV_GO_COMM_STAT_FILE);
        }

        if(FT_Is_File(SMV_GO_COMM_STAT_FILE".bak"))
        {
            remove(SMV_GO_COMM_STAT_FILE".bak");
        }
    }

    return EP_SUCCESS;
}

BOOL Refresh_Smv_Go_CommStat_File()
{
    if(semTake(semSGCommStatChg,NO_WAIT)==OK)
    {
        if(Create_Smv_Go_CommStat_File()==EP_SUCCESS)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

