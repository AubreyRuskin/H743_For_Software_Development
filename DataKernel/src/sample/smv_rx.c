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
INCLUDES: smv_rx.h
*/

/* includes */

#include <vxWorks.h>
#include <memLib.h>
#include <semLib.h>
#include <taskLib.h>
#include <msgQLib.h>
// #include "goose_eth.H"

#include <stdio_compat.h>
#include "assert_compat.h"
// #include <stdLib.h>
#include "smv_rx.h"
#include "adc.h"
#include <intLib.h>

// #include "sbcm8260Siu.h"
// #include "m8260IntrCtl.h"
#include "smvcfg.h"
// //#include "GooseInterface.h"
// #include "config04.h"
#include "edp_asst.h"
#include "eth_callback.h"
#include "spt_drv.h"  /* SPT总线支持 */
#include "Smv_Go_CommStat_File.h"
#include "iecgoose.h"
#include "bsp.h"


/* defines */
#define SMV_ETH_TYPE 0x88BA
#define ETHE_MAX_LEN 1518

#define BYTE2WORD(high,low)  (uint16_t)(((uint8_t)high)<<8|((uint8_t)low))

/* 与DCU板通信ASDU长度 */
#define ASDU_LENGTH	46
#define MU_DELAY_UPDATE_PERIOD 30  /* 延时更新周期 */

#define SMV_ERR_CNT 16 /* 异常记录 */

/* typedefs */

/* globals */

SMVINFO sSmvInfo;
SMV_CC_INFO sSmvCCInfo;
SMVDATA sSmvData[SMV_9_1_CHANNUM];
SMV_92_STRUCT SmvStruct[SMV_9_1_CHANNUM];

/* 改为与中断缓冲一致 */
/*INT32 send_data[MAXQSIZESAMPDATA+1][MAXCHNELS];
uint32_t send_data_sts[MAXQSIZESAMPDATA+1][MAXCHNELS];*/
UINT16 poIec_index=0;
UINT16 synout_index=0;

UINT8 pAdrSyn[SMV_9_1_CHANNUM];
UINT8 pAdrCount[SMV_9_1_CHANNUM];
UINT8 pAdrChnValue[SMV_9_1_CHANNUM];
UINT8 nSamChnels;//该ASDU通道数
UINT8 nCCpacket=0;

INT16 SmplCntLocal=0;//对应于TIME2中的本地采样计数器，1200翻转一次
BOOL yabanVal[MAXLINKNUM];
INT32 nMUDelay[MAXCCPACK*MAXCCPACKASDU];//各合并器延时存放位置
BOOL bMUDelayUpdate[MAXCCPACK*MAXCCPACKASDU];  /* 各间隔延时更新标志 */

UINT8 pAdrpTemp; /* DCU临时地址 */

UINT32 nSetValue_MUTDelay_Transmit = 0;  /* MU延时 */
float fSetValue_MUTDelay_Local = 2.500;  /* 本侧获得的用于数字化 */
float fSetValue_MUTDelay_Ops;    /* 从通道获得的用于传统侧 */
uint32_t ulSetValue_PriRateCur;
uint32_t ulSetValue_PriRateVol;
BOOL bMUDelaySndFlag = FALSE;

/* SPT总线相关数据 */
uint32_t ulRxBufCnt = 0;
uint32_t ulProcBufCnt = 0;
uint32_t ulRxBufNum = 0;

/* CC板功能分类号名称 */
uint8_t ucFuncNameArr[][TEMP_INFO_MAX_LEN] =
{
    {"点对点母差主机(接传统子机)——入24点，出24点"},
    {"点对点母差/主变主机(接数字化子机)——入80点，出24、48、96点"},
    {"点对点线路光差——入80点，出24、48、96点"},
    {"点对点录波主机——入80点，出80点"},
    {"点对点子机——入80点，出80点"},
    {""},
    {""},
    {""},
    {"组网母差/主变/线路(非光差)单网——入80点，出24、48、96点"},
    {"组网线路(光差)单网——入80点，出24、48、96点"},
    {"组网录波单网——入80点，出80点"},
    {""},
    {"组网母差/主变/线路(非光差) AB网——入80点，出24、48、96点"},
    {"组网线路(光差)AB网——入80点，出24、48、96点"},
    {"组网录波AB网——入80点，出80点"}
};

/* 是否指针传递模式, 缺省为值传递模式 */
extern BOOL bDataTransMod;

extern T_WATT_QUEUE g_OptWattRcv;

extern SMV_TOTAL_VT_SV_TERM_CFG   SMV_TotalVtAITermCfg_g;

extern BOOL g_bStormState[MAX_CC_BOARD_NUM];		/*是否进入抑制状态*/
extern uint16_t g_bStormOffset[MAX_CC_BOARD_NUM];	/*风暴位的偏移*/
BOOL g_bIsFirstOptFrame[MAX_CC_BOARD_NUM] = {TRUE, TRUE, TRUE, TRUE,
                                             TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
                                             TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
                                             TRUE, TRUE, TRUE, TRUE
                                            };		/*是否该CC板的第一次光功率报文到来*/
extern uint16_t g_usStormOffSet;						/*计算用*/

extern BOOL bFirstTime;

extern BOOL g_bCcdCrcErr;          /* CCD文件CRC校验出错 */
extern BOOL g_bCcdFileErr;         /* CCD文件内容错误 */

/* statics */

/* global functions */

extern IEC_SMV_9_1_CFG *GetSmvChn(uint8_t* addr, uint16_t appID);

extern int SubQueueCallback_goose(UINT8 port, UINT8 *ptr, int revSubLen);
extern EP_STATUS SCI_Get_Yaban_Value(int16_t nNum, BOOL *pbRtYabanValue, uint32_t ulScnTime);

extern void HDL_Change_LineFilt();
/* 显示SPT总线寄存器数值 */
extern void regshow(void);

/* 重新注册采样中断处理函数.
 * Para:
 *     RcvType, 接收类型.
 * Return:
 *     NONE.
 */
extern void smvChgTransType(uint8_t RcvType);

/* ststic functions */

/* 获取smv配置通道与数据包、ASDU对应关系.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
static void cfgMatchAsdu(void);

/* 获取状态标
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
static void convert_stsWord_to_chnSts(UINT16 status1, UINT16 status2, UINT16 status3, UINT8 chnNum, UINT8 *chnStatus);

/* 解析光功率报文(获取网络风暴抑制标识).
 * Para:
 *     ptr, 数据指针.
 *     revSubLen, 报文长度.
 * Return:
 *     OK, or ERROR.
 */
static int smvParseOpt(UINT8 *ptr, int revSubLen);

/* 初始化DataSet字段.
 * Para:
 *     ptr, 报文指针.
 *     pTemp, 当前指针.
 *     pAsduInfo, ASDU信息指针.
 * Return:
 *     NONE.
 */
static void smvInitDataSet(UINT8 *ptr, UINT8 *pTemp, ASDU_INFO *pAsduInfo);

/* 解析线路保护DataSet字段.
 * Para:
 *     ptr, 报文指针.
 *     pAsduInfo, ASDU信息指针.
 * Return:
 *     状态变化标识.
 */
static BOOL smvParseLineDataSet(UINT8 *ptr, ASDU_INFO *pAsduInfo);

/* 完整解析DataSet字段.
 * Para:
 *     ptr, 报文指针.
 *     pAsduInfo, ASDU信息指针.
 * Return:
 *     状态变化标识.
 */
static BOOL smvParseDataSet(UINT8 *ptr, ASDU_INFO *pAsduInfo);

/* functions */

/* 内部规约与DCU板通信.
 * Para:
 *     port,端口号.
 *     ptr,数据地址.
 *     revSubLen,数据长度.
 * Return:
 *     状态.
 */
static int smvQueueCallback_type05(UINT8 port, UINT8 *ptr, uint32_t revSubLen, int para1, int para2)
{
    UINT16 i = 0, j, chn = 0;
    UINT16 nLength;
    UINT8 *pTemp = ptr;
    UINT8 *pData = NULL;
    UINT8 *pDest = NULL;
    UINT8 *pTemp2 = NULL;

    INT16 nDiff;

    UINT16 nStatus1;
    UINT16 nStatus2;
    INT16 VAL;
    UINT16 nCount;

    IEC_SMV_9_1_CFG *smvCfg = gSmvCfg.Smv_9_1Cfg;
    IEC_SMV_CHAP *chap;
    int16_t ntmptmp = 0;
    UINT8 new_data_sts_temp[MAXCHNELS];
    static BOOL bLogMsg = FALSE;
    static BOOL bFirstRead = TRUE;
    static BOOL bFirstLog = TRUE;
    static UINT32 nLogMsgCnt = 0;

    if ((revSubLen<ETHE_MINLEN) || (revSubLen>ETHE_MAXLEN))
    {
        return ERROR;
    }

    if ((port != SUB_ETHERNET_PACKET_A) && (port != SUB_ETHERNET_PACKET_B))
    {
        return ERROR;
    }

    /* Get first field after DST/SRC MACs. Normally it is Length/Type field
     * but it could be beginning of QTag Prefix (IEEE 802.1Q).
     */

    pTemp += 12;

    sSmvInfo.nTPID = U8_TO_U16(*pTemp, *(pTemp+1));
    pTemp += 2;

    /* If field0 is beginning of QTAG Prefix (IEEE 802.1Q), handle it.
     */
    if (sSmvInfo.nTPID == 0x8100)
    {
        sSmvInfo.nTCI = U8_TO_U16(*pTemp, *(pTemp+1));
        pTemp += 2;

        sSmvInfo.nEthType = U8_TO_U16(*pTemp, *(pTemp+1));
        pTemp += 2;
    }
    else
    {
        sSmvInfo.nEthType = sSmvInfo.nTPID;
    }

    /* Only SMV packet can be copied into the smvQueue.
     */
    if (sSmvInfo.nEthType == 0x88BA)
    {
        /* 通道恢复重新同步
         * 两个周波恢复正常
         */
        if (!sSmvData[0].bSmvCommOk)
        {
            sSmvData[0].ComBackCountCnt++;
            if (sSmvData[0].ComBackCountCnt >= SamplingNum_g*2)
            {
                sSmvData[0].bSmvCommOk = TRUE;
                Smv_Go_CommStat_Chg();
                sSmvData[0].ComBackCountCnt = 0;
            }
            bSysSynFlag = FALSE;
            bFirstRead = TRUE;
        }

        /* 第一次读,以后用固定偏移
         */
        if (bFirstRead)
        {
            bFirstRead = FALSE;
            sSmvInfo.nAPPID = U8_TO_U16(*pTemp, *(pTemp+1));
            pTemp += 2;

            nLength = BYTE2WORD(*pTemp, *(pTemp+1));
            pTemp += 2;

            if (revSubLen<(nLength+18))
            {
                return OK;
            }

            /* Make sure it can't be too long, so go ahead and fill in "sn_req".
             */
            pTemp += 4;

            /* Check the ASDU tag, and find the location of the first ASDU.
             */
            if ((*pTemp) != 0x80)
            {
                return OK;
            }
            pTemp += 1;

            if ((*pTemp)&0x80)
            {
                pTemp += (*pTemp&0x7F);
            }
            pTemp += 1;

            /* compute the asdu number.	*/
            sSmvInfo.nAsduNum = BYTE2WORD(*pTemp, *(pTemp+1));
            pTemp += 2;

            pTemp += 6;

            sSmvInfo.nRatedPhsCur = BYTE2WORD(*pTemp, *(pTemp+1));
            pTemp += 2;

            sSmvInfo.nRatedNeuCur = BYTE2WORD(*pTemp, *(pTemp+1));
            pTemp += 2;

            sSmvInfo.nRatedPhsVol = BYTE2WORD(*pTemp, *(pTemp+1));
            pTemp += 2;

            sSmvInfo.nRatedDlyTime = BYTE2WORD(*pTemp, *(pTemp+1));
            pTemp += 2;

            /* 周波采样率
             */
            sSmvInfo.nSmpRate = *(pTemp+30);
            sSmvInfo.nConfRevNo = *(pTemp+31);

            if (SamplingNum_g == 0)
            {
                return OK;
            }

            if (sSmvInfo.nAsduNum == 0)
            {
                return OK;
            }

            /* 周期读取数据基准指针
             */
            pAdrpTemp = pTemp-ptr;
        }

        pTemp = ptr+pAdrpTemp;
        nCount = BYTE2WORD(*(pTemp+28), *(pTemp+29));

        if ((nCount % (sSmvInfo.nSmpRate/SamplingNum_g)) == 0)
        {
            /* bSampRec必须放于此生成新点的地方,此时为有效抽取点,
             * 否则报文晚到会出错
             */
            sSmvData[0].bSampRec = TRUE;
        }

        /* 通讯未完全恢复直接返回
         */
        if (!sSmvData[0].bSmvCommOk)
        {
            return OK;
        }

        /* 1s计数判断
         */
        nDiff = nCount - sSmvData[0].gRecCount;
        if (nDiff < 0)
            nDiff += sSmvInfo.nSmpRate*50;

        if ((nDiff != 1) && (!sSmvData[0].bSampFirst))
        {
            LOG_Dbg_Msg("!!!!loss point,nDiff:%d,now:%d\n", nDiff, nCount, 0, 0, 0, 0);
            nLogMsgCnt = 0;
            if (bLogMsg)
            {
                char str[100] = "";
                bLogMsg = FALSE;
                sprintf(str, "SMV丢点,差异%d,之前点:%d,当前点:%d!\n",
                        nDiff, sSmvData[0].gRecCount, nCount);
                LOG_Write(LOG_KERNEL, str, NULL);
            }

            if (nDiff>2)  /* 允许丢1点 */
            {
                if(sSmvData[0].bSmvCommOk)
                {
                    sSmvData[0].bSmvCommOk = FALSE; /* 一点丢点就置错,防止角差 */
                    Smv_Go_CommStat_Chg();
                }
                sSmvData[0].bSampFirst = TRUE;
                sSmvData[0].bSampRec = FALSE;
                sSmvData[0].ComBackCountCnt = 0;
                return OK;
            }
        }
        else
        {
            if (!bLogMsg)
            {
                if (nLogMsgCnt<1200)
                    nLogMsgCnt++;
                else
                {
                    bLogMsg = TRUE;
                }
            }
        }

        /* 历史计数 */
        sSmvData[0].gRecCount = nCount;

        if (sSmvData[0].bSampFirst)
        {
            sSmvData[0].gRecCount = nCount;
            sSmvData[0].bSampFirst = FALSE;
        }

        /* 更新压板
         * 一个周波更新一次
         */
        if (nCount == 0)
        {
            for (chn = 0; chn<iLinkNum_g; chn++)
            {
                SCI_Get_Yaban_Value(chn, &yabanVal[chn], 0);
            }
        }

        /* 抽点,不考虑不能整除现象
         */
        if ((nCount % (sSmvInfo.nSmpRate/SamplingNum_g)) == 0)
        {
            pData = pTemp;

            /* 此处最终改为从配置文件中读取到的ASDU数,可节省空间,后面的拷贝按照
             */

            if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 8)
                ntmptmp = SmplCntLocal;  /* 此时读到的本地采样计数器为++的,需要-1 */
            if (ntmptmp<0)
                ntmptmp += MAXQSIZESAMPDATA;

            /* 以下这段是为了出错情况下的一个补偿,防止波形出尖刺
             */
            if (bSysSynFlag)
            {
                static int16_t ntmptmpLast = 0;
                int16_t ntmpdiff = 0;

                if (bFirstLog)
                {
                    bFirstLog = FALSE;
                    ntmptmpLast = ntmptmp;
                }
                else
                {
                    ntmpdiff = ntmptmp-ntmptmpLast;

                    if (ntmpdiff<0)
                        ntmpdiff += MAXQSIZESAMPDATA;
                    if (ntmpdiff != 1)
                    {
                        LOG_Dbg_Msg("=========ntmpdiff;%d,now:%d,SmplCntLocal:%d\r\n",
                                    ntmpdiff,
                                    ntmptmp,
                                    SmplCntLocal,
                                    0, 0, 0);

                        /* 以下操作为防止产生空点
                         */
                        if (ntmpdiff == 0)
                            ntmptmp = (ntmptmp+1)%MAXQSIZESAMPDATA;
                        else if (ntmpdiff == 2)
                            ntmptmp = (ntmptmp+MAXQSIZESAMPDATA-1)%MAXQSIZESAMPDATA;
                    }
                    ntmptmpLast = ntmptmp;

                    /* 为防止合并器有重发点,导致连续调节
                     */
                    if ((nCount%sSmvInfo.nSmpRate) == 0)
                        ntmptmpLast = SmplCntLocal;

                }
            }

            /* 先清零,再求和
             * 按最大物理通道数处理
             */
            for (chn = 0; chn<ucMaxAnaNumber; chn++)
            {
                send_data[ntmptmp][chn] = 0;
                send_data_sts[ntmptmp][chn] = 0;
            }

            /* 按ASDU数查询
             */
            for (i = 0; i<smvCfg->asduNum; i++)
            {
                /* 定位到ASDU
                 */
                pTemp = pData+ASDU_LENGTH*i;

                nStatus1 =  BYTE2WORD(*(pTemp+24), *(pTemp+25));
                nStatus2 =  BYTE2WORD(*(pTemp+26), *(pTemp+27));
                convert_stsWord_to_chnSts(nStatus1, nStatus2, 0, 12, new_data_sts_temp);

                /* 按通道查询
                 */
                for (j = 0; j<smvCfg->dataNum; j++)
                {
                    chap = smvCfg->smvData + j;

                    /* 获取通道值
                     */
                    pTemp2 = pTemp+chap->smvAdsuChn*2;
                    VAL = BYTE2WORD(*pTemp2, *(pTemp2+1));

                    /* 按内序排列数据 */
                    pDest = (UINT8 *)&send_data[ntmptmp][chap->smvDataChn];

                    /* 输入通道配置成255,表示对应硬件通道数据无效,值为零
                     */
                    if (chap->smvAdsuChn == 254)
                    {
                        memset(pDest, 0, 4);
                        send_data_sts[ntmptmp][chap->smvDataChn] =  AI_DAT_VLD;
                    }
                    else
                    {
                        if (i == chap->smvAdsuNo)
                        {
                            /* 配置文件里配了压板 */
                            if (chap->MuLinkUSE)
                            {
                                if (yabanVal[chap->MuLinkNo])
                                {
                                    send_data[ntmptmp][chap->smvDataChn] += VAL;
                                    send_data_sts[ntmptmp][chap->smvDataChn] |= new_data_sts_temp[chap->smvAdsuChn];
                                }
                                else
                                {
                                    send_data[ntmptmp][chap->smvDataChn] += 0;
                                    send_data_sts[ntmptmp][chap->smvDataChn] |= 0x00;
                                }
                            }
                            else  /* 没有配置压板 */
                            {
                                send_data[ntmptmp][chap->smvDataChn] += VAL;
                                send_data_sts[ntmptmp][chap->smvDataChn] |= new_data_sts_temp[chap->smvAdsuChn];
                            }

                            if (gSmvCfg.Smv_9_1Cfg[0].forceSyn)	 /* 需要强制同步 */
                            {
                                send_data_sts[ntmptmp][chap->smvDataChn] &= ~AI_DAT_SYN;
                            }
                        }
                    }
                }
            }
        }

        return OK;
    }

    return ERROR;
}

/* 解析光功率报文(获取网络风暴抑制标识).
 * Para:
 *     ptr, 报文指针.
 *     revSubLen, 报文长度.
 * Return:
 *     OK, or ERROR.
 */
static int smvParseOpt(UINT8 *ptr, int revSubLen)
{
    STATUS bSts = ERROR;
    T_WATT_DATA_ELE *pCur = NULL;  /* 临时写入点指针 */
    UINT8 *srcPtr = ptr+6; /* 源地址 */
    uint8_t ucOptStorm = 0;

    bSts = GsEnQue(&g_OptWattRcv, &pCur);
    if (bSts == OK)
    {
        memcpy((uint8_t *)pCur->ulData, ptr, revSubLen);
        pCur->ulLen = revSubLen;
        pCur->ucAddr = *(srcPtr+5); /* CC板地址 */
        if (g_bIsFirstOptFrame[pCur->ucAddr])
        {
            g_bIsFirstOptFrame[pCur->ucAddr] = FALSE;

            /* 计算得到偏移地址
             */
            ParseOptWatt((uint8_t *)pCur->ulData, pCur->ulLen, pCur->ucAddr);
            g_bStormOffset[pCur->ucAddr] = g_usStormOffSet;
            ucOptStorm = *(((uint8_t *)pCur->ulData)+g_usStormOffSet);
            g_bStormState[pCur->ucAddr] = ucOptStorm & MASK_NET_STORM;
        }
        else
        {
            ucOptStorm = *(((uint8_t *)pCur->ulData)+g_bStormOffset[pCur->ucAddr]);
            g_bStormState[pCur->ucAddr] = ucOptStorm & MASK_NET_STORM;
        }

        /* 进行消抖时间切换（线路、母差）
         */
        if ((uiAppType_g == APP_LINE) || (uiAppType_g == APP_BUS))
        {
            HDL_Change_LineFilt();
        }
    }

    return OK;
}

/* 初始化DataSet字段.
 * Para:
 *     ptr, 报文指针.
 *     pTemp, 当前指针.
 *     pAsduInfo, ASDU信息指针.
 * Return:
 *     NONE.
 */
static void smvInitDataSet(UINT8 *ptr, UINT8 *pTemp, ASDU_INFO *pAsduInfo)
{
    /* 端口状态标处理 */
    pTemp += 2;  /* 延时通道 */

    /* 本级/A网信息,ASDU按包中序号进行填写
     */
    pAsduInfo->AdrMuInfo = pTemp-ptr;
    memcpy((uint8_t *)&pAsduInfo->muInfo.ulMuInfo, pTemp, 4);
    pTemp += 4;

    /* 前级/B网端口信息 */
    pAsduInfo->AdrSlaveInfo = pTemp-ptr;
    memcpy((uint8_t *)&pAsduInfo->sInfo.ulSlaveInfo, pTemp, 4);

    /* 点对点传输 */
    if (sSmvCCInfo.bP2PorNet)
    {
        /* 是否需要读取前级CC端口状态判断 */
        if (pAsduInfo->sInfo.sInfo_st.V0)
        {
            pAsduInfo->bSlaveFlag = TRUE;
        }
        else
        {
            pAsduInfo->bSlaveFlag = FALSE;
        }

        /* 保存上次子机端口有效与否状态用于更新判断
         */
        pAsduInfo->bLstSlaveFlag = pAsduInfo->bSlaveFlag;
    }
    else  /* 组网传输 */
    {
        /* 是否读取A网状态 */
        if (pAsduInfo->muInfo.muInfo_st.V0)
        {
            pAsduInfo->bNetAFlag = TRUE;
        }
        else
        {
            pAsduInfo->bNetAFlag = FALSE;
        }

        /* 是否读取B网信息 */
        if (pAsduInfo->sInfo.sInfo_st.V0)
        {
            pAsduInfo->bNetBFlag = TRUE;
        }
        else
        {
            pAsduInfo->bNetBFlag = FALSE;
        }

        /* 保存上次有效与否状态用于更新判断
         */
        pAsduInfo->bLstNetAFlag = pAsduInfo->bNetAFlag;

        pAsduInfo->bLstNetBFlag = pAsduInfo->bNetBFlag;
    }

    pAsduInfo->bPortStsFlag = TRUE;
}

/* 完整解析DataSet字段.
 * Para:
 *     ptr, 报文指针.
 *     pAsduInfo, ASDU信息指针.
 * Return:
 *     状态变化标识.
 */
static BOOL smvParseDataSet(UINT8 *ptr, ASDU_INFO *pAsduInfo)
{
    UINT16 j;
    BOOL bChgSmvStsFile=FALSE;

    if (pAsduInfo->bPortStsFlag)
    {
        memcpy((uint8_t *)(&pAsduInfo->muInfo.ulMuInfo),
               (uint8_t *)(ptr+pAsduInfo->AdrMuInfo), 4);

        memcpy((uint8_t *)(&pAsduInfo->sInfo.ulSlaveInfo),
               (uint8_t *)(ptr+pAsduInfo->AdrSlaveInfo), 4);

        /* 点对点传输 */
        if (sSmvCCInfo.bP2PorNet)
        {
            /* 是否需要读取前级CC端口状态判断 */
            if (pAsduInfo->sInfo.sInfo_st.V0)
            {
                pAsduInfo->bSlaveFlag = TRUE;

                /* 主机端口状态有无异常 */
                if (pAsduInfo->muInfo.ulMuInfo & MU_STS_MASK)
                {
                    pAsduInfo->bMuFlag = TRUE;

                    if (pAsduInfo->muInfo.ulMuInfo&AI_CC_CONFREV_CHECK_ERR)
                    {
                        pAsduInfo->muInfo.ulMuInfo |= AI_CC_APPID_CHECK_ERR;
                    }

                    pAsduInfo->ulPortSts
                        = (pAsduInfo->muInfo.ulMuInfo & MU_STS_MASK_WITHOUT_CONFREV_CHECK) << STS_POS_SHIFT;

                }
                else
                {
                    pAsduInfo->bMuFlag = FALSE;

                    if (pAsduInfo->sInfo.ulSlaveInfo&AI_CC_CONFREV_CHECK_ERR)
                    {
                        pAsduInfo->sInfo.ulSlaveInfo |= AI_CC_APPID_CHECK_ERR;
                    }

                    pAsduInfo->ulPortSts
                        = ((pAsduInfo->sInfo.ulSlaveInfo & SLAVE_STS_MASK_WITHOUT_CONFREV_CHECK) << STS_POS_SHIFT)
                          | AI_SLAVE_FLAG;
                }
            }
            else
            {
                pAsduInfo->bSlaveFlag = FALSE;

                if (pAsduInfo->muInfo.ulMuInfo&AI_CC_CONFREV_CHECK_ERR)
                {
                    pAsduInfo->muInfo.ulMuInfo |= AI_CC_APPID_CHECK_ERR;
                }

                pAsduInfo->ulPortSts
                    = (pAsduInfo->muInfo.ulMuInfo & MU_STS_MASK_WITHOUT_CONFREV_CHECK) << STS_POS_SHIFT;
            }

            /* 子机端口有效性出现变化,从无效变为有效
             */
            if (pAsduInfo->bSlaveFlag
                    && !pAsduInfo->bLstSlaveFlag)
            {
                for (j = 0; j<iHwAiChNum_g; j++)
                {
                    if ((phwaich_g[j].ucModCh<HCHNNUM) && phwaich_g[j].pSmv)
                        phwaich_g[j].ucMstPortNum = cfgGetPortNo(phwaich_g[j].pSmv,
                                                    &phwaich_g[j].ucSlvPortNum, phwaich_g[j].arrSVID);
                }

            }

            pAsduInfo->bLstSlaveFlag
                = pAsduInfo->bSlaveFlag;
        }
        else /* 组网传输 */
        {
            /* 是否读取A网状态 */
            if (pAsduInfo->muInfo.muInfo_st.V0)
            {
                pAsduInfo->bNetAFlag = TRUE;

                if (pAsduInfo->muInfo.ulMuInfo&AI_CC_CONFREV_CHECK_ERR)
                {
                    pAsduInfo->muInfo.ulMuInfo |= AI_CC_APPID_CHECK_ERR;
                }

                pAsduInfo->ulPortSts
                    = (pAsduInfo->muInfo.ulMuInfo & NET_AB_MAST_WITHOUT_CONFREV_CHECK) << STS_POS_SHIFT;

                if (pAsduInfo->muInfo.ulMuInfo & DATA_SRC_POS)
                {
                    pAsduInfo->ulPortSts &= ~AI_SLAVE_FLAG;
                }
            }
            else
            {
                pAsduInfo->bNetAFlag = FALSE;
                pAsduInfo->ulPortSts = 0;
            }

            /* 是否读取B网信息 */
            if (pAsduInfo->sInfo.sInfo_st.V0)
            {
                pAsduInfo->bNetBFlag = TRUE;

                if (pAsduInfo->sInfo.ulSlaveInfo&AI_CC_CONFREV_CHECK_ERR)
                {
                    pAsduInfo->sInfo.ulSlaveInfo |= AI_CC_APPID_CHECK_ERR;
                }

                pAsduInfo->ulPortSts
                |= (pAsduInfo->sInfo.ulSlaveInfo & NET_AB_MAST_WITHOUT_CONFREV_CHECK) << (STS_POS_SHIFT+NET_B_POS_SHIFT);

                /* CC板处理源唯一的问题
                 */
                if (pAsduInfo->sInfo.ulSlaveInfo & DATA_SRC_POS)
                {
                    pAsduInfo->ulPortSts |= AI_SLAVE_FLAG;
                }
            }
            else
            {
                pAsduInfo->bNetBFlag = FALSE;
            }

            /* 组网光差同步标识 */
            if ((gSmvCfg.Smv_9_1Cfg[0].receiveType == 9)
                    && (!sSmvCCInfo.bP2PorNet))
            {
                /* 同步模式 */
                if (!sSmvCCInfo.bNetSynMod)
                {
                    pAsduInfo->ulPortSts |= AI_NET_SYN_MODE_FLAG;
                }

                /* 外同步信号状态 */
                if (!sSmvCCInfo.bNetOptSyn)
                {
                    pAsduInfo->ulPortSts |= AI_NET_SYN_FLAG;
                }
            }

            /* 端口有效性出现变化,从无效变为有效
             */
            if ((pAsduInfo->bNetAFlag
                    && !pAsduInfo->bLstNetAFlag)
                    ||
                    (pAsduInfo->bNetBFlag
                     && !pAsduInfo->bLstNetBFlag))
            {
                for (j = 0; j<iHwAiChNum_g; j++)
                {
                    if ((phwaich_g[j].ucModCh<HCHNNUM) && phwaich_g[j].pSmv)
                        phwaich_g[j].ucMstPortNum = cfgGetPortNo(phwaich_g[j].pSmv,
                                                    &phwaich_g[j].ucSlvPortNum, phwaich_g[j].arrSVID);
                }

            }

            /* 保存上次有效与否状态用于更新判断
             */
            pAsduInfo->bLstNetAFlag
                = pAsduInfo->bNetAFlag;

            pAsduInfo->bLstNetBFlag
                = pAsduInfo->bNetBFlag;
        }

        if((pAsduInfo->muInfoBak.ulMuInfoBak
                != pAsduInfo->muInfo.ulMuInfo)
                || (pAsduInfo->sInfoBak.ulSlaveInfoBak
                    != pAsduInfo->sInfo.ulSlaveInfo))
        {
            pAsduInfo->muInfoBak.ulMuInfoBak
                =pAsduInfo->muInfo.ulMuInfo;
            pAsduInfo->sInfoBak.ulSlaveInfoBak
                =pAsduInfo->sInfo.ulSlaveInfo;
            bChgSmvStsFile=TRUE;
        }
    }

    return bChgSmvStsFile;
}

/* 解析线路保护DataSet字段.
 * Para:
 *     ptr, 报文指针.
 *     pAsduInfo, ASDU信息指针.
 * Return:
 *     状态变化标识.
 */
static BOOL smvParseLineDataSet(UINT8 *ptr, ASDU_INFO *pAsduInfo)
{
    BOOL bChgSmvStsFile=FALSE;

    if (pAsduInfo->bPortStsFlag)
    {
        memcpy((uint8_t *)(&pAsduInfo->muInfo.ulMuInfo),
               (uint8_t *)(ptr+pAsduInfo->AdrMuInfo), 4);

        pAsduInfo->ulPortSts
            = (pAsduInfo->muInfo.ulMuInfo & MU_STS_MASK_WITHOUT_CONFREV_CHECK) << STS_POS_SHIFT;

        if ((pAsduInfo->muInfoBak.ulMuInfoBak
                != pAsduInfo->muInfo.ulMuInfo))
        {
            pAsduInfo->muInfoBak.ulMuInfoBak
                = pAsduInfo->muInfo.ulMuInfo;
            bChgSmvStsFile = TRUE;
        }
    }

    return bChgSmvStsFile;
}

/* 从CC板接收数据包.
 * Para:
 *     port,端口号.
 *     ptr,数据地址.
 *     revSubLen,数据长度.
 * Return:
 *     状态.
 */
static int smvQueueCallback_type22(UINT8 port, UINT8 *ptr, uint32_t revSubLen, int para1, int para2)
{


    UINT16 i=0,j,chn;
    UINT16 nLength;
    UINT8 *pTemp=ptr;
    UINT8 *pData=NULL;
    UINT32 *pDest = NULL;
    UINT8 nowpackno; /* 从0开始 */
    UINT8   backpoint;
    int head=0,back=0;

    UINT8 temp;
    int datalen=0;
    int16_t ntmptmp = 0;  /* 记录当前采样节拍 */
    INT32  nMUDelayData;

    INT16  nDiff;
    UINT16 nCount = 0;
    IEC_SMV_9_1_CFG *smvCfg = gSmvCfg.Smv_9_1Cfg;
    IEC_SMV_CHAP *chap = NULL;

    static BOOL bLogMsg=FALSE;
    static BOOL bFirstRead[SMV_9_1_CHANNUM] = {TRUE, TRUE, TRUE, TRUE};
    /* 第一次读取数据,需要设定匹配关系 */
    static BOOL bFirstEnter[SMV_9_1_CHANNUM] = {TRUE, TRUE, TRUE, TRUE};
    static UINT32 nLogMsgCnt=0;
    static BOOL bHasGetVer_s = FALSE;
    static uint32_t ulUpdateDelayCnt = 0;  /* 延时更新计数 */

    BOOL bChgSmvStsFile=FALSE;
    UINT8 *srcPtr = ptr+6;
    static BOOL bFstPackCom[SMV_9_1_CHANNUM] = {FALSE, FALSE, FALSE, FALSE}; /* 第一包是否到来 */

    if ( (revSubLen<ETHE_MINLEN)||(revSubLen>ETHE_MAXLEN) )
    {
        return ERROR;
    }

    /* 处理网口A/B数据和SPT接收数据 */
    if ( (port != SUB_ETHERNET_PACKET_A)
            && (port != SUB_ETHERNET_PACKET_B)
            && (port != SUB_ETHERNET_PACKET_C)) /* SPT总线接收 */
    {
        return ERROR;
    }

    if(g_bCcdCrcErr || g_bCcdFileErr)
    {
        return OK;
    }

    /* Get first field after DST/SRC MACs. Normally it is Length/Type field		*/
    /* but it could be beginning of QTag Prefix (IEEE 802.1Q).		*/
    memcpy(sSmvInfo.nDstMacAddr, pTemp, sizeof(sSmvInfo.nDstMacAddr));

    pTemp += 12;

    sSmvInfo.nTPID = U8_TO_U16(*pTemp, *(pTemp+1));
    pTemp += 2;

    /* If field0 is beginning of QTAG Prefix (IEEE 802.1Q), handle it.	*/
    if (sSmvInfo.nTPID==0x8100)   /* TPID */
    {
        sSmvInfo.nTCI = U8_TO_U16(*pTemp, *(pTemp+1));
        pTemp += 2;

        sSmvInfo.nEthType = U8_TO_U16(*pTemp, *(pTemp+1));
        pTemp += 2;
    }
    else
    {
        sSmvInfo.nEthType = sSmvInfo.nTPID;
    }

    /* 光功率报文缓冲填写, 有一定风险, 后续考虑是否处理SV口过来光功率报文 */
    if ((sSmvInfo.nEthType == 0x88B8)
            && (*srcPtr == 0x00)
            && (*(srcPtr+1) == 0x00)
            && (*(srcPtr+2) == 0x00)
            && (*(srcPtr+4) == 0x00)
       )
    {
        /* 光功率报文 */
        if (*(srcPtr+3) == 0x01)
        {
            smvParseOpt(ptr, revSubLen);
            return OK;
        }
    }

    /* Only SMV packet can be copied into the smvQueue.	*/
    if (sSmvInfo.nEthType==0x88BA)
    {
        sSmvInfo.nAPPID = U8_TO_U16(*pTemp, *(pTemp+1));
        pTemp += 2;

        smvCfg = GetSmvChn(sSmvInfo.nDstMacAddr, sSmvInfo.nAPPID);
        if(smvCfg == NULL)
        {
            return OK;
        }

        /* 通道恢复重新同步
         * 两个周波数据有效则SMV通信有效
         */
        if (!sSmvData[smvCfg->smvID].bSmvCommOk)
        {
            /* 重新处理第一包 */
            if (sSmvData[smvCfg->smvID].ComBackCountCnt == 0)
            {
                bFstPackCom[smvCfg->smvID] = FALSE;
            }

            sSmvData[smvCfg->smvID].ComBackCountCnt++;
            if (sSmvData[smvCfg->smvID].ComBackCountCnt >= SamplingNum_g*sSmvCCInfo.comBackCycleLimit)
            {
                sSmvData[smvCfg->smvID].bSmvCommOk = TRUE;
                Smv_Go_CommStat_Chg();
                sSmvData[smvCfg->smvID].ComBackCountCnt = 0;
                bFirstRead[smvCfg->smvID] = FALSE;
            }
            else
            {
                bFirstRead[smvCfg->smvID] = TRUE;
            }
            bSysSynFlag = FALSE;
        }

        nLength = U8_TO_U16(*pTemp, *(pTemp+1));
        pTemp += 2;
        if (revSubLen<(nLength+18))
        {
            return OK;
        }

        /* reserved */
        sSmvCCInfo.reserved1=U8_TO_U16(*pTemp, *(pTemp+1));
        pTemp += 2;
        sSmvCCInfo.reserved2=U8_TO_U16(*pTemp, *(pTemp+1));
        pTemp += 2;

        if ((sSmvCCInfo.reserved2 == 0) && (sSmvCCInfo.reserved1 == 0))
        {
            /* 保留字段没有使用 */
            nowpackno =0;
            SmvStruct[smvCfg->smvID].allpackno = 1;  /* 缺省总包数为1 */
            backpoint    =0;
        }
        else if (sSmvCCInfo.reserved1<10)
        {
            /* 保留字段1表示总分包数,保留字段2表示当前分包 */
            nowpackno = sSmvCCInfo.reserved2-1; /* 有分包机制时,第一个分包上送编号为1 */
            SmvStruct[smvCfg->smvID].allpackno = sSmvCCInfo.reserved1;
            backpoint    =0;
        }
        else
        {
            /* 保留字段1内表示当前分包/总分包/回退点数
             * 如:0x1203
             * 表示总报文分包数为2,当前包序号为1,插值回退3个24点间隔
             */
            nowpackno = ((sSmvCCInfo.reserved1>>12) & (0x0f))-1; /* 有分包机制时,第一个分包上送编号为1 */
            SmvStruct[smvCfg->smvID].allpackno = (sSmvCCInfo.reserved1>>8) & (0x0f);
            backpoint = (sSmvCCInfo.reserved1 & 0x000f);
            sSmvCCInfo.CCVersion = (sSmvCCInfo.reserved2>>8) & (0xff);

            if (sSmvCCInfo.backpoint != backpoint)
            {
                char str[100] = "";
                static uint32_t ulLogCnt = 0;

                ulLogCnt++;

                if (ulLogCnt<SMV_ERR_CNT)
                {
                    sprintf(str, "CC板延时整点数变化,点数%d!\n", backpoint);
                    LOG_Write(LOG_KERNEL, str, NULL);
                }
            }
            if (sSmvCCInfo.backpoint > sSmvCCInfo.backPointLimit)
            {
                char str[100] = "";
                static uint32_t ulLogCnt = 0;

                ulLogCnt++;
                if (ulLogCnt<SMV_ERR_CNT)
                {
                    sprintf(str, "CC板延时整点数超过%ld,点数%d!\n",
                            sSmvCCInfo.backPointLimit, backpoint);
                    LOG_Write(LOG_KERNEL, str, NULL);
                }
            }
            sSmvCCInfo.backpoint = backpoint;

            /* 保留字节信息
             */
            sSmvCCInfo.FuncNo = sSmvCCInfo.reserved2 & 0xf; /* 功能分类号 */
            sSmvCCInfo.bProgCfgMatchFlag = (sSmvCCInfo.reserved2 & 0x10) ? FALSE : TRUE;  /* 程序与配置匹配与否 */
            sSmvCCInfo.bCpuSyn = (sSmvCCInfo.reserved2 & 0x20) ? TRUE : FALSE;  /* 光差CPU同步与否 */
            sSmvCCInfo.bNetOptSyn = (sSmvCCInfo.reserved2 & 0x40) ? TRUE : FALSE;  /* 组网光差外同步标识 */
            sSmvCCInfo.bNetSynMod = (sSmvCCInfo.reserved2 & 0x80) ? TRUE : FALSE;  /* 同步模式, 0: 计数器同步 1: 外同步 */

            /* 点对点与组网判断
             * 初始化时CC板根据配置文件确定
             */
            sSmvCCInfo.bP2PorNet = (smvGetTransType() == P_2_P) ? TRUE : FALSE;

            /* 光差模式下接收方式切换
             * 同时为组网模式
            */
            if ((gSmvCfg.Smv_9_1Cfg[0].receiveType == 9)
                    && !sSmvCCInfo.bP2PorNet
                    && (!bFirstTime) /* 非第1点 */
                    && (sSmvData[smvCfg->smvID].bSmvCommOk == TRUE)) /* 通信正常 */
            {
                if ((sSmvCCInfo.bNetSynMod != sSmvCCInfo.bLstNetSynMod)
                        || bFirstRead[smvCfg->smvID])
                {
                    if (sSmvCCInfo.bNetSynMod)
                    {
                        smvChgTransType(9);
                    }
                    else
                    {
                        smvChgTransType(3);
                    }
                }
                sSmvCCInfo.bLstNetSynMod = sSmvCCInfo.bNetSynMod;
            }

            /* 写入CC板程序版本 */
            if (!bHasGetVer_s)
            {
                uint8_t aucVer[32] = "";

                sprintf(aucVer, "%x", (int)sSmvCCInfo.CCVersion);
                LOG_ExtraItemWrite("CC板(SMV)程序版本", aucVer);
                bHasGetVer_s = TRUE;
            }
        }

        /* 初次处理,实际上前2个周波亦如此处理 */
        if (bFirstRead[smvCfg->smvID])
        {
            for (chn = 0; chn<iLinkNum_g; chn++)
            {
                SCI_Get_Yaban_Value(chn, &yabanVal[chn], 0);
            }
            if(nowpackno==0)
            {
                /* 没有分包或第1个分包 */
                bFstPackCom[smvCfg->smvID] = TRUE;  /* 第一包到来 */
                head = 0;
                back = sSmvInfo.nMulAsduNum[0];
                if (smvCfg->asduNum<back)
                {
                    back = smvCfg->asduNum;
                }

                if (nCCpacket<1)
                {
                    nCCpacket = 1;
                }
            }
            else if (nowpackno == 1)
            {
                /* 第2个分包 */
                if (!bFstPackCom[smvCfg->smvID])
                {
                    /* 第一包没来 */
                    return OK;
                }

                head = sSmvInfo.nMulAsduNum[0];
                back = sSmvInfo.nMulAsduNum[0]+sSmvInfo.nMulAsduNum[1];

                if (smvCfg->asduNum <= head)	/* 只解第一分包 */
                {
                    return OK;
                }
                else if (smvCfg->asduNum<back)
                {
                    back = smvCfg->asduNum;
                }

                if (nCCpacket<2)
                {
                    nCCpacket = 2;
                }
            }
            else if (nowpackno == 2)
            {
                /* 第3个分包 */

                if (!bFstPackCom[smvCfg->smvID])
                {
                    /* 第一包没来 */
                    return OK;
                }

                head = sSmvInfo.nMulAsduNum[0]+sSmvInfo.nMulAsduNum[1];
                back = sSmvInfo.nMulAsduNum[0]+sSmvInfo.nMulAsduNum[1]+sSmvInfo.nMulAsduNum[2];
                if (smvCfg->asduNum <= head)	/* 只解第一分包 */
                {
                    return OK;
                }
                else if (smvCfg->asduNum<back)	/* 需要解该分包 */
                {
                    back = smvCfg->asduNum;
                }

                if (nCCpacket<3)
                {
                    nCCpacket = 3;
                }
            }
            else if (nowpackno == 3)
            {
                /* 第4个分包,最多支持4个分包 */

                if (!bFstPackCom[smvCfg->smvID])
                {
                    /* 第一包没来 */
                    return OK;
                }

                head = sSmvInfo.nMulAsduNum[0]+sSmvInfo.nMulAsduNum[1]+sSmvInfo.nMulAsduNum[2];
                back = sSmvInfo.nMulAsduNum[0]+sSmvInfo.nMulAsduNum[1]+sSmvInfo.nMulAsduNum[2]+sSmvInfo.nMulAsduNum[3];
                if (smvCfg->asduNum <= head)	/* 只解前两分包 */
                {
                    return OK;
                }
                else if (smvCfg->asduNum<back)  /* 需要解该分包 */
                {
                    back = smvCfg->asduNum;
                }
                if (nCCpacket<4)
                {
                    nCCpacket = 4;
                }
            }
            else
            {
                return OK;
            }

            /* 分包中的ASDU序号,此顺序为配置顺序,从0开始,所有包排序
             * 用于与smv.xml配置文件中通道所对应的ASDU号匹配
             */
            SmvStruct[smvCfg->smvID].packInfo[nowpackno].head = head;
            SmvStruct[smvCfg->smvID].packInfo[nowpackno].back = back;
            SmvStruct[smvCfg->smvID].packInfo[nowpackno].asduNum = back-head;  /* 根据配置查询所需ASDU数 */

            /* savPdu tag */
            if (*pTemp++ == 0x60)
            {
                temp = *pTemp++;	/* savPdu length */
                if (temp & 0x80)
                {
                    temp &= 0x7F;
                    pTemp += temp;
                }
            }
            else
            {
                return OK;
            }

            /* NoASDU tag */
            if (*pTemp++ == 0x80)
            {
                temp = *pTemp++;	/* NoASDU length */
                if (temp & 0x80)
                {
                    temp &= 0x7F;
                    pTemp += temp;
                }

                if (temp == 1)
                {
                    sSmvInfo.nMulAsduNum[nowpackno] = *pTemp++;
                }
                else if (temp == 2)
                {
                    sSmvInfo.nMulAsduNum[nowpackno] = U8_TO_U16(*pTemp, *(pTemp+1));
                    pTemp += 2;
                }
                else
                {
                    return OK;
                }
            }
            else
            {
                return OK;
            }

            /* Sequence of ASDU tag */
            if (*pTemp++ == 0xA2)
            {
                temp = *pTemp++;	/* Sequence of ASDU length */
                if (temp & 0x80)
                {
                    temp &= 0x7F;
                    pTemp += temp;
                }
            }
            else
            {
                return OK;
            }

            /* 逐一处理ASDU */
            for (i = 0; i<SmvStruct[smvCfg->smvID].packInfo[nowpackno].asduNum; i++)
            {

                /* Sequence of ASDU tag */
                if (*pTemp++ == 0x30)
                {
                    temp = *pTemp++;  /* Sequence of ASDU length */
                    if (temp & 0x80)
                    {
                        temp &= 0x7F;
                        pTemp += temp;
                    }
                }
                else
                {
                    return OK;
                }

                /* svID tag */
                if (*pTemp++ == 0x80)
                {
                    temp = *pTemp++;  /* svID length */
                    if (temp & 0x80)
                    {
                        temp &= 0x7F;
                        if (temp == 1)
                        {
                            datalen = (int) (*pTemp);
                        }
                        else if (temp == 2)
                        {
                            datalen = U8_TO_U16(*pTemp, *(pTemp + 1));
                        }
                        pTemp += temp;
                    }
                    else
                    {
                        datalen = temp;
                    }

                    /*xdf added 20131017: 限制svId长度为<=34字节*/
                    if(datalen>SV_ID_MAX_LEN)
                    {
                        return OK;
                    }

                    memcpy(SmvStruct[smvCfg->smvID].asduInfo[nowpackno][i].arrSVID, pTemp, datalen);
                    SmvStruct[smvCfg->smvID].asduInfo[nowpackno][i].arrSVID[datalen] = '\0';

                    pTemp += datalen;
                }
                else
                {
                    return OK;
                }

                /* DatSet tag,可选 */
                if (*pTemp == 0x81)
                {
                    pTemp++;

                    /* DatSet length */
                    temp = *pTemp++;
                    if (temp & 0x80)
                    {
                        temp &= 0x7F;
                        if (temp == 1)
                        {
                            datalen = (int) (*pTemp);
                        }
                        else if (temp == 2)
                        {
                            datalen = (int)U8_TO_U16(*pTemp, *(pTemp + 1));
                        }
                        else
                        {
                            return OK;
                        }
                        pTemp += temp;
                    }
                    else
                    {
                        datalen = temp;
                    }

                    /* data */

                    if (datalen == 0x0A)
                    {
                        smvInitDataSet(ptr, pTemp, &SmvStruct[smvCfg->smvID].asduInfo[nowpackno][i]);
                    }
                    else
                    {
                        SmvStruct[smvCfg->smvID].asduInfo[nowpackno][i].bPortStsFlag = FALSE;
                    }
                    pTemp += datalen;
                }

                /* SmpCnt tag */
                if (*pTemp++ == 0x82)
                {
                    temp = *pTemp++;	/* SmpCnt length */
                    if (temp & 0x80)
                    {
                        temp &= 0x7F;
                        pTemp += temp;
                    }

                    /* 记录下采样计数器位置 */
                    SmvStruct[smvCfg->smvID].pAdrCount[nowpackno][i] = pTemp-ptr;
                    nCount = U8_TO_U16(*pTemp, *(pTemp + 1));
                    pTemp += 2;
                }
                else
                {
                    return OK;
                }

                /* ConfRev tag */
                if (*pTemp++ == 0x83)
                {
                    temp = *pTemp++;  /* ConfRev length */
                    if (temp & 0x80)
                    {
                        temp &= 0x7F;
                        pTemp += temp;
                    }

                    pTemp += 4;
                }
                else
                {
                    return OK;
                }

                /* RefrTm tag 可选*/
                if (*pTemp == 0x84)
                {
                    pTemp++;
                    temp = *pTemp++; /* RefrTm length */
                    if (temp & 0x80)
                    {
                        temp &= 0x7F;

                        /* 计算值长度, 缺省设置为6 */
                        datalen = 6;

                        pTemp += temp;

                        pTemp += datalen;
                    }
                    else
                    {
                        datalen = temp;
                        pTemp += datalen;
                    }
                }

                /* SmpSynch tag */
                if (*pTemp++ == 0x85)
                {
                    temp = *pTemp++; /* SmpSynchlength */
                    if (temp & 0x80)
                    {
                        temp &= 0x7F;
                        pTemp += temp;
                    }

                    /* 记录下采样计数器位置 */
                    SmvStruct[smvCfg->smvID].pAdrSyn[nowpackno][i] = pTemp-ptr;
                    if (gSmvCfg.Smv_9_1Cfg[0].forceSyn)	 /* 需要强制同步 */
                        SmvStruct[smvCfg->smvID].asduInfo[nowpackno][i].bSyn = TRUE;
                    else
                        SmvStruct[smvCfg->smvID].asduInfo[nowpackno][i].bSyn = *pTemp;

                    pTemp += 1;
                }
                else
                {
                    return OK;
                }

                /* 为了兼容有些9_2LE无86报文
                 * 无86报文对应采样率为80点
                 * SmpRate tag
                 */
                if (*pTemp == 0x86)
                {
                    pTemp++;
                    temp = *pTemp++;  /* SmpRate length */
                    if (temp & 0x80)
                    {
                        temp &= 0x7F;
                        pTemp += temp;
                        sSmvInfo.nSmpRate = U8_TO_U16(*pTemp, *(pTemp + 1));

                        pTemp += 2;  /* 固定偏移两个字节 */
                    }
                    else
                    {
                        sSmvInfo.nSmpRate = U8_TO_U16(*pTemp, *(pTemp + 1));
                        pTemp += 2;
                    }
                }
                else
                {
                    sSmvInfo.nSmpRate = gSmvCfg.Smv_9_1Cfg[0].smprate9_2;
                }

                /* Sequence of Data tag */
                if (*pTemp++ == 0x87)
                {
                    temp = *pTemp++;  /* Sequence of Data length */
                    if (temp & 0x80)
                    {
                        temp &= 0x7F;

                        if (temp == 1)
                        {
                            /* 计算通道数,每个通道8个字节
                             * 记录下位置
                             * 以及记录下该ASDU通道数量
                             */
                            SmvStruct[smvCfg->smvID].pAdrDataLen[nowpackno][i] = pTemp-ptr;
                            nSamChnels = *pTemp/8;
                            sSmvInfo.nChnNum[nowpackno][i] = nSamChnels;
                        }
                        pTemp += temp;
                    }

                    /* 记录下位置 */
                    SmvStruct[smvCfg->smvID].pAdrChnValue[nowpackno][i] = pTemp-ptr;
                }
                else
                {
                    return OK;
                }

                pTemp += (nSamChnels)*8;
            }
        }
        else /* 2周波后处理数据 */
        {

            /* 第一次正式读取,处理通道与数据包、ASDU的对应关系
             */
            if (bFirstEnter[smvCfg->smvID])
            {
                bFirstEnter[smvCfg->smvID] = FALSE;
                cfgMatchAsdu();
            }

            /* 节拍处理 */
            if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 9)
            {
                /* 光差 */
                if (sSmvCCInfo.bNetSynMod || sSmvCCInfo.bP2PorNet)
                {
                    ntmptmp = SmplCntLocal;
                }
                else
                {
                    ntmptmp = poIec_index;
                }
            }
            else
            {
                /* 非光差 */
                ntmptmp = poIec_index;
            }

            /* 只在第1包中处理 */
            if (nowpackno == 0)
            {
                /* 采样计数器
                 * 假定所有ASDU一致
                 */
                pTemp = ptr+SmvStruct[smvCfg->smvID].pAdrCount[nowpackno][0];
                nCount = U8_TO_U16(*pTemp, *(pTemp + 1));

                /* 丢点判断　*/
                if (sSmvData[smvCfg->smvID].bSampFirst)
                {
                    /* 第一点处理 */
                    sSmvData[smvCfg->smvID].gRecCount = nCount;
                    sSmvData[smvCfg->smvID].bSampFirst = FALSE;
                }
                else
                {
                    nDiff = nCount - sSmvData[smvCfg->smvID].gRecCount;
                    if (nDiff < 0)
                        nDiff += sSmvInfo.nSmpRate * 50;
                    if (nDiff != 1)
                    {
                        nLogMsgCnt = 0;

                        if (bLogMsg)
                        {
                            char str[100] = "";

                            bLogMsg = FALSE;
                            sprintf(str, "SMV丢点,差异%d,之前点:%d,当前点:%d!\n",
                                    nDiff, sSmvData[smvCfg->smvID].gRecCount, nCount);
                            LOG_Write(LOG_KERNEL, str, NULL);
                        }

                        if (nDiff>2)  /* 允许丢1点 */
                        {
                            if(sSmvData[smvCfg->smvID].bSmvCommOk)
                            {
                                sSmvData[smvCfg->smvID].bSmvCommOk = FALSE;
                                Smv_Go_CommStat_Chg();
                            }
                            sSmvData[smvCfg->smvID].bSampFirst = TRUE;
                            sSmvData[smvCfg->smvID].bSampRec = FALSE;

                            return OK;
                        }
                    }
                    else
                    {
                        if (!bLogMsg)
                        {
                            if (nLogMsgCnt<1200)
                                nLogMsgCnt++;
                            else
                            {
                                bLogMsg = TRUE;
                            }
                        }
                    }
                }

                sSmvData[smvCfg->smvID].gRecCount = nCount;

                if (nCount == 0)
                {
                    for (chn = 0; chn<iLinkNum_g; chn++)
                    {
                        SCI_Get_Yaban_Value(chn, &yabanVal[chn], 0);
                    }

                    /* 1秒一次计数 */
                    ulUpdateDelayCnt++;
                }
            }

            /* 本包的所有ASDU关键信息
             */
            for (i = 0; i<SmvStruct[smvCfg->smvID].packInfo[nowpackno].asduNum; i++)
            {
                /* 同步标志 */
                pTemp = ptr+SmvStruct[smvCfg->smvID].pAdrSyn[nowpackno][i];
                if (gSmvCfg.Smv_9_1Cfg[0].forceSyn)	 /* 需要强制同步 */
                    SmvStruct[smvCfg->smvID].asduInfo[nowpackno][i].bSyn = TRUE;
                else
                    SmvStruct[smvCfg->smvID].asduInfo[nowpackno][i].bSyn = *pTemp;

                if (uiAppType_g == APP_LINE)
                {
                    bChgSmvStsFile |= smvParseLineDataSet(ptr, &SmvStruct[smvCfg->smvID].asduInfo[nowpackno][i]);
                }
                else
                {
                    bChgSmvStsFile |= smvParseDataSet(ptr, &SmvStruct[smvCfg->smvID].asduInfo[nowpackno][i]);
                }
            }

            if(bChgSmvStsFile)
            {
                Smv_Go_CommStat_Chg();
            }

            /* 查询所有通道 */
            for (j = 0; j<SmvStruct[smvCfg->smvID].packInfo[nowpackno].iChapNum; j++)
            {
                chap = SmvStruct[smvCfg->smvID].packInfo[nowpackno].smvChapArr[j];

                /* 获取该ASDU的数据区头部 */
                pTemp= ptr+SmvStruct[smvCfg->smvID].pAdrChnValue[nowpackno][chap->smvAsduNoInPack];
                pData = pTemp+chap->smvAdsuChn*8;	/* 值为4个字节,Q状态为4个字节 */

                pDest = &send_data[ntmptmp][chap->smvDataChn];

                /* 输入通道配置成255,表示对应硬件通道数据无效,值为零
                 * 一般用于合并单元没有接入
                 * 为了扩建预留的SMV配置
                 */
                if (chap->smvAdsuChn != 254)
                {
                    if (chap->smvChnType == 1)	/* 类型:采样值通道 */
                    {
                        /* 配置压板,且压板投入,
                         * 或配置文件中没有配置压板
                         */
                        if ((chap->MuLinkUSE && yabanVal[chap->MuLinkNo])
                                || (!chap->MuLinkUSE))
                        {
                            memcpy((UINT8 *)pDest, pData, 4);

                            send_data_sts[ntmptmp][chap->smvDataChn]
                                = SmvStruct[smvCfg->smvID].asduInfo[nowpackno][chap->smvAsduNoInPack].ulPortSts;

                            if (*(pData+7) || (*(pData+6) & 0x13))
                                send_data_sts[ntmptmp][chap->smvDataChn] |= AI_DAT_VLD;
                            if (*(pData+6) & 0x08)
                                send_data_sts[ntmptmp][chap->smvDataChn] |= AI_TEST_DAT;
                            if (!SmvStruct[smvCfg->smvID].asduInfo[nowpackno][chap->smvAsduNoInPack].bSyn)
                                send_data_sts[ntmptmp][chap->smvDataChn] |= AI_DAT_SYN;

                            /*2013-6-25 ZY 为界面虚端子状态显示添加 */
                            //aulCurSamDataStsArr_g[chap->smvDataChn]=send_data_sts[ntmptmp][chap->smvDataChn];
                        }
                        else	/* 该MU压板未投入,数据置0,状态标正常 */
                        {

                            /*2013-6-25 ZY 为界面虚端子状态显示添加 */
                            send_data_sts[ntmptmp][chap->smvDataChn]
                                = SmvStruct[smvCfg->smvID].asduInfo[nowpackno][chap->smvAsduNoInPack].ulPortSts;

                            if (*(pData+7) || (*(pData+6) & 0x13))
                                send_data_sts[ntmptmp][chap->smvDataChn] |= AI_DAT_VLD;
                            if (*(pData+6) & 0x08)
                                send_data_sts[ntmptmp][chap->smvDataChn] |= AI_TEST_DAT;
                            if (!SmvStruct[smvCfg->smvID].asduInfo[nowpackno][chap->smvAsduNoInPack].bSyn)
                                send_data_sts[ntmptmp][chap->smvDataChn] |= AI_DAT_SYN;

                            aulCurSamDataStsArr_g[chap->smvDataChn]=send_data_sts[ntmptmp][chap->smvDataChn];
                            /*2013-6-25  添加结束 */

                            *pDest = 0;
                            send_data_sts[ntmptmp][chap->smvDataChn] = AI_LINK_STS; /* 配置SV压板, 但没有投入 */
                        }
                    }
                    else if ((chap->smvChnType == 2) /* 类型:延时通道 */
                             && ((ulUpdateDelayCnt >= MU_DELAY_UPDATE_PERIOD) || (!bMUDelayUpdate[chap->MuTypeNo])))   /* 更新周期是否到以及是否第一次 */
                    {
                        pDest = (UINT32 *)&nMUDelayData;	/* 获取延时数据 */
                        memcpy((UINT8 *)pDest, pData, 4);
                        bMUDelayUpdate[chap->MuTypeNo] = TRUE;

                        /* 延时差大于10us
                         */
                        if ((abs(nMUDelayData-nMUDelay[chap->MuTypeNo])>10) /* && (!(*(pData+7) || (*(pData+6) & 0x13))) */ )
                        {
                            SLOW_MESSAGE_NODE Info;

                            nMUDelay[chap->MuTypeNo] = nMUDelayData;  /* 更新 */
                            Info.type = MUDELAYWR;							/* 写合并单元延时 */
                            Info.MuTypeNo = chap->MuTypeNo;				/* 条目号 */
                            Info.MuDelay = nMUDelay[chap->MuTypeNo];		/* 延时数值 */
                            msgQSend(SlowMessage, (char *)&Info, sizeof(SLOW_MESSAGE_NODE), NO_WAIT, MSG_PRI_NORMAL);
                        }
                    }
                }
            }

            /* 更新节拍控制, 最末一包清零 */
            if (ulUpdateDelayCnt >= MU_DELAY_UPDATE_PERIOD)
            {
                if ((nowpackno+1) == nCCpacket)
                {
                    ulUpdateDelayCnt = 0;
                }
            }
        }

        bFirstRead[smvCfg->smvID] = FALSE;

        /* 本包没有到总包数
         * 返回等下一包再写缓冲区
         */
        if ((nowpackno+1) != nCCpacket)
        {
            return OK;
        }

        /* 报文类型有效 */
        sSmvData[smvCfg->smvID].bSampRec=TRUE;

        poIec_index++;
        if (SamplingNum_g == 0)
        {
            return OK;
        }

        if (poIec_index >= MAXQSIZESAMPDATA)
        {
            poIec_index = 0;
        }

        return OK; /* 直接返回 */
    }

    return ERROR;
}

/* SMV采样初始化.
 * Para:
 *     NONE.
 * Return:
 *     TRUE, FALSE.
 */
BOOL GetSampDataInit (void)
{
    return FALSE;
    int i;
    uint8_t  innerSrc[6]= {0x01,0x0C,0xCD,0x01,0x01,0xFF};   /* MUL_SRC:后的MAC地址*/
    uint8_t optWattSrc[6] = {0x01, 0x0C, 0xCD, 0x01, 0xFF, 0xFF};  /* 光功率报文地址 */

#if defined(SUBUNIT)

    /* 禁止以太网中断,
     * 但属于传统采样
     */
    // fcc_intDisable(SUB_ETHERNET_PACKET_A);
    return FALSE;
#endif

    if(LoadSmvCfg(SMV_CFG_FILE)==FALSE)
    {
        logMsg("Error opening input file '%s'\n", (int)SMV_CFG_FILE,0,0,0,0,0);
        return FALSE;
    }

    if(mCastAddrAdd(SUB_ETHERNET_PACKET_A, innerSrc) != OK)
    {
        logMsg("设置DCPU MAC地址失败!\n", 0,0,0,0,0,0);
        return FALSE;
    }

    mCastAddrAdd(SUB_ETHERNET_PACKET_A, optWattSrc);  /* 光功率报文地址处理 */

    if((gSmvCfg.smvNum>0)&&((gSmvFT3Cfg.smvNum>0)||(gSmvADCfg.smvNum>0)))
    {
        logMsg("File '%s' config error,ETH & AD or FT3 can not config together!\n", (int)SMV_CFG_FILE,0,0,0,0,0);
        assert(0);
    }
    sSmvInfo.nMulAsduNum[0]=1;
    sSmvInfo.nMulAsduNum[1]=1;
    sSmvInfo.nMulAsduNum[2]=1;
    sSmvInfo.nMulAsduNum[3]=1;

    assert (sizeof(SmvStruct[0].asduInfo[0][0].muInfo.muInfo_st) == UINT32_BYTE_NUM);
    assert (sizeof(SmvStruct[0].asduInfo[0][0].sInfo.sInfo_st) == UINT32_BYTE_NUM);

    if(gSmvCfg.smvNum>0)
    {
        /*配置有9-1\9-2接受*/
        for(i=0; i<gSmvCfg.smvNum; i++)
        {
            /*STI板9_1接受*/
            if(mCastAddrAdd(SUB_ETHERNET_PACKET_A, gSmvCfg.Smv_9_1Cfg[i].smvSrc) != OK)
            {
                logMsg("设置MAC地址失败!\n", 0,0,0,0,0,0);
                return FALSE;
            }
        }
    }
    else
    {
        return TRUE;
    }

    if(gSmvCfg.Smv_9_1Cfg[0].receiveType==3)
    {
        sSmvCCInfo.backPointLimit = MAX_BACK_POINT_TYPE_3;
        sSmvCCInfo.comBackCycleLimit = COM_BACK_CYCLE_NUM_TYPE_3;

        // fcc_intDisable(SUB_ETHERNET_PACKET_A);

        if((gSmvCfg.Smv_9_1Cfg[0].smprate9_2==24)||(gSmvCfg.Smv_9_1Cfg[0].smprate9_2==48)||(gSmvCfg.Smv_9_1Cfg[0].smprate9_2==96))//内部多ASDU9-2接收 24点
        {
            if (reg_Int_Goose_Recv_Fun(SUB_ETHERNET_PACKET_A, smvQueueCallback_type22) == ERROR)
            {
                return FALSE;
            }
            else
            {
                printf("以太网回调函数注册成功\n");

                return TRUE;

            }

        }
        else
        {
            printf("SMV.XML配置错误,9-2只支持24点、48点和96点三种配置\n");
            assert (FALSE);

            return FALSE;
        }
    }
    else if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 8)
    {
        /* 与DCP板通信 */
        // fcc_intDisable(SUB_ETHERNET_PACKET_A);
        if (reg_Int_Goose_Recv_Fun(SUB_ETHERNET_PACKET_A, smvQueueCallback_type05) == ERROR)
        {
            return FALSE;
        }
        else
        {
            bMUDelaySndFlag = TRUE;
            return TRUE;
        }
    }

//     /*内部扩展9-2规约接受，发秒脉冲给前置板zq*/
    else if(gSmvCfg.Smv_9_1Cfg[0].receiveType==9)
    {
        sSmvCCInfo.backPointLimit = MAX_BACK_POINT_TYPE_9;
        sSmvCCInfo.comBackCycleLimit = COM_BACK_CYCLE_NUM_TYPE_9;

        // fcc_intDisable(SUB_ETHERNET_PACKET_A);
        if(reg_Int_Goose_Recv_Fun(SUB_ETHERNET_PACKET_A, smvQueueCallback_type22) == ERROR)
        {
            return FALSE;
        }
        else
        {
            printf("以太网回调函数注册成功\n");
            return TRUE;
        }

    }
    else if (gSmvCfg.Smv_9_1Cfg[0].receiveType == 15)
    {
        /* 删除SPT总线支持 */
    }
    else
    {
        assert (FALSE);
    }

    return FALSE;
}

/* 获取物理通道对应smv配置通道.
 * Para:
 *     ucModCh,内序.
 * Return:
 *     smv配置通道指针.
 */
void *cfgGetAsdu(uint8_t ucModCh)
{
    int i;
    int j;

    for (i = 0; i<gSmvCfg.smvNum; i++)
    {
        for (j = 0; j<gSmvCfg.Smv_9_1Cfg[i].dataNum; j++)
        {
            /* 内序一致 */
            if (ucModCh == gSmvCfg.Smv_9_1Cfg[i].smvData[j].smvDataChn)
            {
                return &gSmvCfg.Smv_9_1Cfg[i].smvData[j];
            }
        }
    }

    return NULL;
}

/* 获取物理通道对应网络端口号(本级CC板端口号和前级CC板端口号).
 * Para:
 *     pSmvIn,smv通道指针.
 *     pSlvPortNum,前级CC板端口号.
 *     arrSVID,svID保存数组.
 * Return:
 *     本级CC板端口号.
 */
extern uint8_t cfgGetPortNo(void *pSmvIn, uint8_t *pSlvPortNum, uint8_t *arrSVID)
{
    int i;
    int j;
    IEC_SMV_CHAP *pSmv = NULL;
    ASDU_INFO *pAsdu = NULL;
    int len;

    if (pSmvIn)
    {
        pSmv = (IEC_SMV_CHAP *)pSmvIn;

        for (i = 0; i<SMV_9_1_CHANNUM; i++)
        {
            for (j = 0; j<MAXCCPACK; j++)
            {
                /* 配置文件中按照所有的ASDU统一编号
                 * 但数据帧内部是从0开始编号
                 */
                if ((pSmv->smvAdsuNo >= SmvStruct[i].packInfo[j].head)
                        && (pSmv->smvAdsuNo < SmvStruct[i].packInfo[j].back))
                {
                    pAsdu = &SmvStruct[i].asduInfo[j][pSmv->smvAdsuNo-SmvStruct[i].packInfo[j].head];

                    /* SVID写入 */
                    if (arrSVID)
                    {
                        len = strlen(pAsdu->arrSVID);
                        if (len <= MAX_ID_LEN)
                        {
                            strncpy(arrSVID, pAsdu->arrSVID, len);
                        }
                    }

                    if (pSlvPortNum && pAsdu->sInfo.sInfo_st.V0)
                    {
                        *pSlvPortNum = pAsdu->sInfo.sInfo_st.sNetNo;
                    }

                    if (pAsdu->muInfo.muInfo_st.V0)
                    {
                        return pAsdu->muInfo.muInfo_st.muNetNo;
                    }
                }
            }
        }
    }

    if (pSlvPortNum)
    {
        *pSlvPortNum = 0xFF;
    }

    return 0xFF;
}

/* 获取smv配置通道与数据包、ASDU对应关系.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
static void cfgMatchAsdu(void)
{
    int i;
    int j;
    int k;

    for (i = 0; i<gSmvCfg.smvNum; i++)
    {
        /* 数据集个数 */
        for (j = 0; j<gSmvCfg.Smv_9_1Cfg[i].dataNum; j++)
        {
            /* 通道个数 */
            for (k = 0; k<SmvStruct[i].allpackno; k++)
            {
                /* 包数 */
                if ((gSmvCfg.Smv_9_1Cfg[i].smvData[j].smvAdsuNo >= SmvStruct[i].packInfo[k].head)
                        && (gSmvCfg.Smv_9_1Cfg[i].smvData[j].smvAdsuNo < SmvStruct[i].packInfo[k].back))
                {
                    /* ASDU编号对应 */
                    gSmvCfg.Smv_9_1Cfg[i].smvData[j].PackNo = k;
                    gSmvCfg.Smv_9_1Cfg[i].smvData[j].smvAsduNoInPack
                        = gSmvCfg.Smv_9_1Cfg[i].smvData[j].smvAdsuNo-SmvStruct[i].packInfo[k].head;
                    SmvStruct[i].packInfo[k].smvChapArr[SmvStruct[i].packInfo[k].iChapNum]
                        = &gSmvCfg.Smv_9_1Cfg[i].smvData[j];
                    SmvStruct[i].packInfo[k].iChapNum++;
                }
            }

        }
    }
}

/* 获取状态标
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void convert_stsWord_to_chnSts(UINT16 status1, UINT16 status2, UINT16 status3, UINT8 chnNum, UINT8 *chnStatus)
{
    int i;
    UINT16 u, uu;
    UINT8 *pTemp = NULL;

    memset(chnStatus, 0, MAXCHNELS);

    /* 标准9-1状态字解析 */

    /* READ WORDSTATE1 */
    pTemp = chnStatus;
    for (i = 0; i<12; i++)
    {
        if (status1 & 0x0001)
            *(pTemp+i) |= AI_PHHealth_DAT;
        if (status1 & 0x0002)
            *(pTemp+i) |= AI_TEST_DAT;
        if (status1 & 0x0008)
            *(pTemp+i) |= AI_DAT_SYN_MOD;
        if (status1 & 0x0010)
            *(pTemp+i) |= AI_DAT_SYN;
        if (status1 & 0x1000)
            *(pTemp+i) |= AI_SENSOR_TYPE;
        if (status1 & 0x2000)
            *(pTemp+i) |= AI_DAT_SCP_FLG;
    }

    u = 0x1 << 5;
    for (i = 0; i<7; i++)
    {
        if ((status1 & u) || (status1 & 0x0004))
        {
            *(pTemp+i) |= AI_DAT_VLD;  /* 通信断置异常标 */
        }
        u = u << 1;
    }

    /* READ WORDSTATE2 */
    uu = 0x1;
    for (i = 7; i<12; i++)
    {
        if ((status2 & uu) || (status1 & 0x0004))
        {
            *(pTemp+i) |= AI_DAT_VLD;  /* 通信断置异常标 */
        }
        uu = uu << 1;
    }
}

/* 向DCU发送延时信息
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void TMuDelay_Transmit(void)
{
#define MU_DELAY_FRAME_LEN 80

    UINT8 *pD = NULL;
    UINT8 *pDataSend = NULL;
    UINT8 *Stat_addr0 = NULL;
    unsigned char bTestMode = 0;
    UINT32 Sec_num;
    unsigned char ucSndBuf[MU_DELAY_FRAME_LEN];

    nSetValue_MUTDelay_Transmit = (UINT32)(fSetValue_MUTDelay_Local*1000.0);

    pD = ucSndBuf;
    assert (pD != NULL);
    pDataSend = pD;

    EP_Get_Repair_Sts(&bTestMode);
    Stat_addr0 = pD;
    *pD++ = 0x01;  /* Destination address */
    *pD++ = 0x0C;
    *pD++ = 0xCD;
    *pD++ = 0x01;
    *pD++ = 0x01;
    *pD++ = 0xFF;

    *pD++ = 0x73;  /* sac001 Source address */
    *pD++ = 0x61;
    *pD++ = 0x63;
    *pD++ = 0x00;
    *pD++ = 0x00;
    *pD++ = 0x01;

    *pD++ = 0x81;
    *pD++ = 0x00; /* TPID */

    *pD = 0;
    *pD++ |= 2 << 5;	 /* user's priority */
    *pD++ = 10;	 /* vlan 10 */

    *pD++ = 0x88;
    *pD++ = 0xBF;	/* ethernet type */

    *pD++ = 0x40;
    *pD++ = 0xFF; /* appid */

    *pD++ = 0x00;
    *pD++ = 0x2A; /* appid */

    *pD++ = 0x20;	/* 代表MU延时 */
    *pD++ = 0x04;		/* 字节数 */
    *pD++ = (nSetValue_MUTDelay_Transmit >> 24) & 0xFF; /* 合并器延时最低位 */
    *pD++ = (nSetValue_MUTDelay_Transmit >> 16) & 0xFF; /* 合并器延时次低位 */
    *pD++ = (nSetValue_MUTDelay_Transmit >> 8) & 0xFF;    /* 合并器延时次高位 */
    *pD++ = (nSetValue_MUTDelay_Transmit >> 0) & 0xFF;  /* 合并器延时最高位 */

    *pD++ = 0x21;	/* 代表保护装置测试态 */
    *pD++ = 0x01; /* 字节数 */
    *pD++ = bTestMode;	/* 保护装置实际测试态 */

    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;

    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;

    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;

    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;
    *pD++ = 0;

    /* configuration version */
    *pD++ = 0x57;

    Sec_num = pD-Stat_addr0;

    // goose_send_raw(0, pDataSend, Sec_num);
}

/* 初始化计数.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void InitCnt(void)
{
    ulRxBufCnt = 0;
    ulProcBufCnt = 0;
    ulRxBufNum = 0;
}

/* 配置总线子板位置.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
static void init_cfg_table_cc(void)
{
    char table[16];

    table[0] = 1;
    table[1] = 2;
    // sptSlotArraySet(0, &table[0], 2);
}

/* 显示端口状态 */
void showEthPort(void)
{
    int i, j;
    int iLockKey;

    iLockKey = intLock();

    for (i = 0; i<MAXCCPACK; i++)
    {
        if (SmvStruct[0].packInfo[i].asduNum)
        {
            LOG_Dbg_Msg("包序%d ASDU数%d\n", i, SmvStruct[0].packInfo[i].asduNum, 0, 0, 0, 0);
            for (j = 0; j<SmvStruct[0].packInfo[i].asduNum; j++)
            {
                LOG_Dbg_Msg("是否显示端口状态%d 是否子机%d 是否处理本级标志%d 本级状态%x 前级状态%x\n",
                            SmvStruct[0].asduInfo[i][j].bPortStsFlag,
                            SmvStruct[0].asduInfo[i][j].bSlaveFlag,
                            SmvStruct[0].asduInfo[i][j].bMuFlag,
                            SmvStruct[0].asduInfo[i][j].muInfo.ulMuInfo,
                            SmvStruct[0].asduInfo[i][j].sInfo.ulSlaveInfo, 0);
            }
        }
    }

    intUnlock(iLockKey);
}

/* 获取SMV接收模式.
 * Para:
 *     NONE.
 * Return:
 *     0: 点对点; 1: 单网; 2: 双网.
 */
int32_t smvGetTransType(void)
{
    if (sSmvCCInfo.FuncNo>0xf)
    {
        return P_2_P;
    }
    else
    {
        if (sSmvCCInfo.FuncNo & 0x0c)
        {
            return DOUBLE_NET;
        }
        else if (sSmvCCInfo.FuncNo & 0x08)
        {
            return SINGLE_NET;
        }
        else
        {
            return P_2_P;
        }
    }

    return P_2_P;
}

/* 获取程序/配置匹配关系.
 * Para:
 *     NONE.
 * Return:
 *     TRUE: 匹配; FALSE: 不匹配.
 */
BOOL smvGetMacthSts(void)
{
    return sSmvCCInfo.bProgCfgMatchFlag;
}

/* 获取CPU脉冲同步状态.
 * Para:
 *     NONE.
 * Return:
 *     TRUE: 正常; FALSE: 异常.
 */
BOOL smvGetCpuSynSts(void)
{
    return sSmvCCInfo.bCpuSyn;
}

/* 组网光差外同步状态.
 * Para:
 *     NONE.
 * Return:
 *     TRUE: 正常; FALSE: 异常.
 */
BOOL smvGetNetOptSyn(void)
{
    return sSmvCCInfo.bNetOptSyn;
}

/* 获取组网光差同步模式.
 * Para:
 *     NONE.
 * Return:
 *     FALSE: 计数器同步; TRUE: 外同步.
 */
BOOL smvGetNetSynMod(void)
{
    return sSmvCCInfo.bNetSynMod;
}


/*为扩展机箱移植到此*/
/*功能：得到CPU的SV虚端子配置信息  2013-6-5  ZY
  参数：ppRtTotalCfgAddr：供返回SV虚端子的总体配置信息全局变量地址对应的变量的指针。
                      该全局变量地址对应的变量由调用方分配，被调用方填充。
                      该全局变量自身，由被调用方分配和管理
  返回：EP_SUCCESS:操作成功
       其他，操作失败 */
EP_STATUS  SMV_Get_Vt_SV_Term_Cfg(SMV_TOTAL_VT_SV_TERM_CFG   **ppRtTotalCfgAddr)
{
    IEC_SMV_9_1_CFG  *pSmvCfg;
    SMV_VT_SV_TERM_CFG  *pTermCfg;
    IEC_SMV_CHAP   *pSmvCh;
    RD_LGC_AI_CH *paich;
    int  i;
    int  k;
    BOOL   bYabanIsFind;
    const SC_LINK_ITEM *  pLink;

    assert(ppRtTotalCfgAddr);
    *ppRtTotalCfgAddr=&(SMV_TotalVtAITermCfg_g);

    pSmvCfg=&(gSmvCfg.Smv_9_1Cfg[0]);/*CC板送过来，合并在1个SUB里面 */
    SMV_TotalVtAITermCfg_g.iTermCnt=0;
    for(i=0; i<pSmvCfg->dataNum; i++)
    {
        pTermCfg=SMV_TotalVtAITermCfg_g.aTermCfgArr
                 +SMV_TotalVtAITermCfg_g.iTermCnt;
        pSmvCh=pSmvCfg->smvData+i;
        if((pSmvCh->smvChnType==1)&&(pSmvCh->phwaich))
        {
            /*若是采样通道且关联到有效的物理AI通道，非悬空或延时通道 */
            pTermCfg->iValType=6;
            pTermCfg->pHwCh=pSmvCh->phwaich;
            pTermCfg->uiDataSetAppID=pSmvCfg->appID;/*由于CC合并多间隔信息，输出只有1个SUB配置，
	   	      	                                        CPU该信息已经没有意义 */
            pTermCfg->ucUnit=((RD_HW_AI_CH *)(pSmvCh->phwaich))->ucUnit;

            /* 虚端子是否悬空 */
            if ((pSmvCh->smvAdsuChn == 254)
                    || (pSmvCh->smvAdsuNo == 254))
            {
                pTermCfg->bIsPend = TRUE;
            }
            else
            {
                pTermCfg->bIsPend = FALSE;
            }

            if(strlen(pSmvCh->smvDes)>64)
            {
                /*若溢出 */
                strncpy(pTermCfg->aucDescStr,pSmvCh->smvDes,64);
                pTermCfg->aucDescStr[64]='\0';
            }
            else
            {
                strcpy(pTermCfg->aucDescStr,pSmvCh->smvDes);
            }

            /*获得关联压板信息 */
            if(pSmvCh->MuLinkUSE)
            {
                /*查询获得压板名称，而不是ID */
                bYabanIsFind=FALSE;
                for(k=0; k<iLinkNum_g; k++)
                {
                    pLink=SC_Get_Link_Attr(k);
                    if(!(strcmp(pSmvCh->MuYabanIDStr,pLink->aucId)))
                    {
                        /*若ID相等 */
                        if(strlen(pLink->aucName)>32)
                        {
                            /*防止溢出   */
                            strncpy(pTermCfg->aucYabanIDStr,pLink->aucName,32);
                            pTermCfg->aucYabanIDStr[32]='\0';
                        }
                        else
                        {
                            strcpy(pTermCfg->aucYabanIDStr,pLink->aucName);
                        }
                        bYabanIsFind=TRUE;
                        pTermCfg->iYabanNo=pSmvCh->MuLinkNo;
                        break;
                    }
                }
                if(!bYabanIsFind)
                {
                    pTermCfg->aucYabanIDStr[0]='\0';
                    pTermCfg->iYabanNo=-1;
                }
            }
            else
            {
                pTermCfg->aucYabanIDStr[0]='\0';
                pTermCfg->iYabanNo=-1;
            }

            /*获得关联logic AI  */
            pTermCfg->pLogAI=NULL;
            for (paich=plgcaich_g; paich<plgcaich_g+iLgcAiChNum_g-iVtAiChNum_g; paich++)
            {
                if((paich->phwai==pSmvCh->phwaich)&&(paich->ucFiltTp==0))
                {
                    pTermCfg->pLogAI=paich;
                    break;
                }
            }

            SMV_TotalVtAITermCfg_g.iTermCnt++;
        }/*if((pSmvCh->smvChnType==1)结束 */

    }/*fori=0;结束 */
    return  EP_SUCCESS;
}


/*功能：得到CPU虚拟SV虚端子状态信息  2013-6-5 ZY
  参数：pRtTotalSTS：供返回SV虚端子的总体状态信息变量指针。
                  该变量，由调用方分配，被调用方填充
  返回：EP_SUCCESS:操作成功
       其他，操作失败 */
EP_STATUS  SMV_Get_Vt_SV_Term_Sts(SMV_TOTAL_VT_SV_TERM_STS   *pRtTotalSts)
{
#define  AI_TERM_COM_ERR_MASK      0X01  /*中断位 */
#define  AI_TERM_INVALID_MASK       0X02  /*无效位 */
#define  AI_TERM_NoSyn_MASK        0X04  /*失步位 */
#define  AI_TERM_TEST_MASK         0X08  /*测试位 */
#define  AI_TERM_YB_EXIT_MASK      0X40  /*压板退出位 */
#define  AI_TERM_NO_CONN_MASK     0X80  /*虚端子未关联位 */

    RD_HW_AI_MEA   aHwAIMeaArr[iHwAiChNum_g];
    SMV_VT_SV_TERM_CFG  *pTermCfg;
    SMV_VT_SV_TERM_STS  *pTermSts;
    BOOL   bYbIsRun;
    uint32_t  ulSts;
    int   iHwChOfst;
    int   i;
    RD_HW_AI_CH * pHwCh;
    uint32_t  *pBufSts;
    uint32_t  ulComSts;

    assert(pRtTotalSts);
    /*获得硬件通道监视值 */
    RD_Mea_Hw_AI(aHwAIMeaArr,TRUE);
    pRtTotalSts->iTermCnt=SMV_TotalVtAITermCfg_g.iTermCnt;
    for(i=0; i<SMV_TotalVtAITermCfg_g.iTermCnt; i++)
    {
        pTermCfg=SMV_TotalVtAITermCfg_g.aTermCfgArr+i;
        pTermSts=pRtTotalSts->aTermStsArr+i;
        pHwCh=(RD_HW_AI_CH *)(pTermCfg->pHwCh);

        /* 悬空时赋予缺省值 */
        if (pTermCfg->bIsPend)
        {
            pTermSts->fTermVal = 0.0;
            pTermSts->ucTermQuality = AI_TERM_NO_CONN_MASK;

            continue;
        }

        /*获取采样值 */
        iHwChOfst=pHwCh-phwaich_g;
        pTermSts->fTermVal=aHwAIMeaArr[iHwChOfst].fRmsVal;

        /*获取状态标 */
        ulSts=0;

        /*接收压板状态 */
        if(pTermCfg->iYabanNo>=0)
        {
            SCI_Get_Yaban_Value(pTermCfg->iYabanNo, &bYbIsRun, 0);
            if(!bYbIsRun)
            {
                ulSts=ulSts|AI_TERM_YB_EXIT_MASK;
            }
        }

        ulComSts=0;
        /*获得实际链路状态，不管接收压板是否投入 */
        if (!sSmvData[0].bSmvCommOk)
        {
            /*若通信不正常，连续丢点 */
            ulComSts=AI_TERM_COM_ERR_MASK|AI_TERM_INVALID_MASK|AI_TERM_NoSyn_MASK;
        }
        else
        {
            /*若通信正常 */
            if(ulSts&AI_TERM_YB_EXIT_MASK)
            {
                /*压板退出，取原始状态 */
                ulComSts=aulCurSamDataStsArr_g[pHwCh->ucModCh];
                if(ulComSts&AI_NO_FRAME_ERR)
                {
                    /*外部端口无接收*/
                    ulComSts=ulComSts|AI_TERM_COM_ERR_MASK;
                }
            }
            else
            {
                /*压板有效，取采样缓冲中的状态*/
                pBufSts=RD_Get_Chn_Sts_All(pTermCfg->pLogAI, RD_AI_Cnt());
                ulComSts=(*pBufSts);
                if(ulComSts&AI_NO_FRAME_ERR)
                {
                    /*外部端口无接收*/
                    ulComSts=ulComSts|AI_TERM_COM_ERR_MASK;
                }
            }

            ulComSts=ulComSts&(0x0F);
        }

        pTermSts->ucTermQuality=(uint8_t)((ulSts|ulComSts)&(0xFF));
    }
    return  EP_SUCCESS;
}

/* 显示ASDU状态标 */
void showAsduData(void)
{
    int i = 0;
    int j = 0;
    int k = 0;
    int m = 0;

    printf("功能分类号: %ld %s\n",
           sSmvCCInfo.FuncNo, ucFuncNameArr[sSmvCCInfo.FuncNo]);
    printf("保留字节: %x %x\n", sSmvCCInfo.reserved1, sSmvCCInfo.reserved2);
    printf("程序与配置匹配: %d\n", sSmvCCInfo.bProgCfgMatchFlag);
    printf("CPU脉冲同步: %d\n", sSmvCCInfo.bCpuSyn);
    printf("外同步脉冲同步: %d 光差组网同步模式: %d\n", sSmvCCInfo.bNetOptSyn, sSmvCCInfo.bNetSynMod);
    printf("插值回退点: %ld\n", sSmvCCInfo.backpoint);

    printf("通信状态%d %d\n", sSmvData[0].bSmvCommOk, sSmvData[0].bSampRec);

    printf("数据状态!\n");
    if (bDataTransMod)
    {
        for (i = 0; i<ucMaxAnaNumber; i++)
        {
            printf("%d %x\n", i, (int)(SampDataQ.base[0].pStatus[i]));
        }
    }
    else
    {
        for (i = 0; i<ucMaxAnaNumber; i++)
        {
            printf("%d %x\n", i, (int)(SampDataQ.base[0].Status[i]));
        }
    }

    if (sSmvCCInfo.bP2PorNet)
    {
        printf("点对点传输:\n");
    }
    else
    {
        printf("组网传输\n");
    }

    /* ASDU状态标 */
    for (i = 0; i<SMV_9_1_CHANNUM; i++)
    {
        for (j = 0; j<MAXCCPACK; j++)
        {
            for (k = 0;  k<SmvStruct[i].packInfo[j].asduNum; k++)
            {
                m++;
                printf("%x  %x  %x  %d %x %x\n",
                       (int)SmvStruct[i].asduInfo[j][k].muInfo.ulMuInfo,
                       (int)SmvStruct[i].asduInfo[j][k].sInfo.ulSlaveInfo,
                       (int)SmvStruct[i].asduInfo[j][k].ulPortSts,
                       (int)nMUDelay[m],
                       (int)SmvStruct[i].asduInfo[j][k].muInfo.ulMuInfo,
                       (int)SmvStruct[i].asduInfo[j][k].sInfo.ulSlaveInfo);

            }
        }
    }

    /* 通道与包, ASDU对应关系 */
    for (i = 0; i<gSmvCfg.smvNum; i++)
    {
        /* 数据集个数 */
        for (j = 0; j<gSmvCfg.Smv_9_1Cfg[i].dataNum; j++)
        {
            /* 通道个数 */
            printf("通道%d 包号%d 包中序号%d 配置ASDU号%d 配置通道号%d 配置内序号%d 配置%x 类型%d\n",
                   j,
                   gSmvCfg.Smv_9_1Cfg[i].smvData[j].PackNo,
                   gSmvCfg.Smv_9_1Cfg[i].smvData[j].smvAsduNoInPack,
                   gSmvCfg.Smv_9_1Cfg[i].smvData[j].smvAdsuNo,
                   gSmvCfg.Smv_9_1Cfg[i].smvData[j].smvAdsuChn,
                   gSmvCfg.Smv_9_1Cfg[i].smvData[j].smvDataChn,
                   (int)&gSmvCfg.Smv_9_1Cfg[i].smvData[j],
                   gSmvCfg.Smv_9_1Cfg[i].smvData[j].smvChnType);
        }
    }

    /* 包属性 */
    for (i = 0; i<gSmvCfg.smvNum; i++)
    {
        for (j = 0; j<SmvStruct[i].allpackno; j++)
        {
            /* 包数 */
            printf("包%d通道数%d head = %d back = %d 包数%d %d\n", j,
                   SmvStruct[i].packInfo[j].iChapNum,
                   SmvStruct[i].packInfo[j].head,
                   SmvStruct[i].packInfo[j].back,
                   SmvStruct[i].packInfo[j].asduNum,
                   sSmvInfo.nMulAsduNum[j]);

            for (k = 0; k<SmvStruct[i].packInfo[j].iChapNum; k++)
            {
                printf("配置%x\n", (int)SmvStruct[i].packInfo[j].smvChapArr[k]);
            }
        }
    }
}

/* 查询通道相应的ASDU序号以及是否采样延时越限(针对9-2报文)
 * Para:
 *     pvLgcAiHnd, 逻辑标志.
 *     pSmvAdsuNo, 返回ASDU序号.
 *     pMuDelay,   返回该ASDU延时时间.
 * Return:
 *     采样延时越限标志, TRUE为越限, FALSE为正常.
 * History:
 *     2015/2/10    张全    创建
 *     2015/3/14    张全    修改:增加返回ASDU延时时间.
 */
BOOL RD_Get_DelayOverFlow (void *pvLgcAiHnd, int *pSmvAdsuNo, int *pMuDelay)
{
    RD_LGC_AI_CH *plgcai = NULL; /* 逻辑通道配置 */
    RD_HW_AI_CH *phwai = NULL;   /*物理通道*/
    IEC_SMV_CHAP *pSmv = NULL;   /*采样配置*/

    /*根据句柄依次获取相应指针,判断获取的指针是否为空*/
    assert(pvLgcAiHnd);
    plgcai = (RD_LGC_AI_CH*)pvLgcAiHnd;
    if (plgcai->phwai != NULL)
    {
        phwai = (RD_HW_AI_CH *)plgcai->phwai;
        if (phwai->pSmv != NULL)
        {
            pSmv = (IEC_SMV_CHAP *)phwai->pSmv;
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

    *pSmvAdsuNo = pSmv->smvAdsuNo;
    *pMuDelay = nMUDelay[pSmv->MuTypeNo];
    if (sSmvCCInfo.bP2PorNet)/*点对点模式下*/
    {
        /*判断ASDU本级CC接入还是前级CC接入*/
        if(SmvStruct[pSmv->smvNum].asduInfo[pSmv->PackNo][pSmv->smvAsduNoInPack].bSlaveFlag)
        {
            /*判断相应标志位是否为1*/
            if(SmvStruct[pSmv->smvNum].asduInfo[pSmv->PackNo][pSmv->smvAsduNoInPack].sInfo.sInfo_st.S3)
            {
                return TRUE;
            }
        }
        else
        {
            if(SmvStruct[pSmv->smvNum].asduInfo[pSmv->PackNo][pSmv->smvAsduNoInPack].muInfo.muInfo_st.S3)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

