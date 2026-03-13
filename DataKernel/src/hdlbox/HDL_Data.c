/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*       HDL_Data.c                                    1.0                      */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了智能操作箱数据模块的代码文件                                     */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                        */
/*                                                                              */
/*         张云       2007.3.28                创建文件1.0版本                 */
/*                                                                             */
/*                                                                              */
/********************************************************************************/

#include   "vxWorks.h"
#include  "GooseInterface.h"
// //#include  "mutual_61850.h"
#include  "realdata.h"
#include  "HDL_Data.h"
#include  "HDL_VtBox.h"
#include  "logic.h"
#include  "errtest.h"
#include  "logmsg.h"
#include "view.h"
#include "GO_Interface.h"
#include "Smv_Go_CommStat_File.h"
#include "intLib.h"

/* globals */
extern uint16_t usMinInterval;  /* 扫描任务最小间隔点数 */

extern BOOL g_bCPU1StormState;
extern BOOL g_bCPU2StormState;

static uint8_t aucPrompt_g[129];
static char *sGcRef_g;

#define SUB_GOOSE_GRP_MAX_NUM MAX_ALLOW_SUB_GO_NUM
#define HDL_AO_DATA_PUB_CHG_TH 0.01
#define HDL_AO_DATA_PUB_CHG_TH_INTELBOX 0.002
uint8_t   aucHdlAllSubGooseStat[SUB_GOOSE_GRP_MAX_NUM][MAX_GSE_NET_CNT];
uint8_t aucHdlLinkCtlAllSubGooseStat[SUB_GOOSE_GRP_MAX_NUM][MAX_GSE_NET_CNT];  /* 压板投入时更新 */

BOOL g_bSubRepairDiffer[SUB_GOOSE_GRP_MAX_NUM];  /*goose检修不一致，不一致时按控制块告警,FALSE代表一致,TRUE代表不一致*/
/*用于向应用传递GOOSE通信状态*/
BOOL HDL_GOOSE_SUB_COMM_STATUS[SUB_GOOSE_GRP_MAX_NUM][MAX_GSE_NET_CNT]= {{TRUE}};
BOOL HDL_GOOSE_SUB_NET_CFG[SUB_GOOSE_GRP_MAX_NUM][MAX_GSE_NET_CNT];

/*得到网络风暴状态*/
extern BOOL EP_GetStormState();

/*2013-6-5日 虚端子配置信息全局变量定义（供HMI查询） ZY */
HDL_TOTAL_VT_DI_TERM_CFG   HDL_TotalVtDITermCfg_g;

/*用于虚端子状态显示的GOOSE真实通信状态,不经压板处理，2013-6-26  ZY*/
BOOL HDL_GOOSE_SUB_REAL_COMM_STATUS[SUB_GOOSE_GRP_MAX_NUM][MAX_GSE_NET_CNT]= {{TRUE}};

BOOL g_bGooseDiNeedRefresh; /* 更新标识 */

/* global functions */

/* 获取扫描入口时刻节拍
 * Para:
 *     NONE.
 * Return:
 *     uint32_t.
 */
extern uint32_t RD_GetScanCnt(void);

/* static functions */

/* 写入智能操作相DI的开入板状态，处理变位等信息
 * 参数：   plgcdi，  DI句柄
 *          iNowVal,  此时DI值
 *          usQuality, 品质因素.
 * 返回值： 无 */
/* */
static void HDL_Modify_DI(RD_LGC_DI_CH *plgcdi, int iNowVal, uint16_t usQuality);

/* 获取GOOSE DI缺省值.
 * Para:
 *     pvDiCh, DI通道句柄.
 *     nIdx, GOOSE DI序号.
 *     bInUse, 是否配置成功.
 *     bCurVal, 当前值.
 * Return:
 *     DI状态.
 */
static BOOL HDL_Get_DI_Default_Val(void *pvDiCh, int nIdx, BOOL bInUse, BOOL bCurVal);

/*
双网模式：双网GOOSE收发，两网通信情况独立播报；
单网模式：（1）A或B网模式，只在A或B网一个网口上收发GOOSE，且只播报工作网口上的通信情况；
             （2）冗余模式，AB网都可收发GOOSE，平时只接一个网口。且只报一个网口通信断, 兰溪变模式
*/

BOOL GO_GetActiveGoTByDaIndx(int  iDaIndx, void *pSubMapData, US_CNT_UTC_TIME *uttime)
{
    return FALSE;
}


BOOL GO_GetTPacketBySubMapData(void *pSubMapData, US_CNT_UTC_TIME *uttime){
    return FALSE;
}

uint16_t Get_ErrNo_from_NetNo(int NetNo)
{
    static uint16_t GooseErrNoArry[MAX_GSE_NET_CNT]= {EV_REL_GSE_A_NET_HALT,
                                                      EV_REL_GSE_B_NET_HALT,
                                                      EV_REL_GSE_C_NET_HALT,
                                                      EV_REL_GSE_D_NET_HALT,
                                                      EV_REL_GSE_E_NET_HALT,
                                                      EV_REL_GSE_F_NET_HALT,
                                                      EV_REL_GSE_G_NET_HALT,
                                                      EV_REL_GSE_H_NET_HALT,
                                                      EV_REL_GSE_I_NET_HALT,
                                                      EV_REL_GSE_J_NET_HALT,
                                                      EV_REL_GSE_K_NET_HALT
                                                     };
    if(NetNo>=iHdlNetNum_g)
    {
        LOG_Write(LOG_KERNEL,"Goose网络号超出限度\n",NULL);
        assert(0);
    }

    return GooseErrNoArry[NetNo];
}

/* global functions */

/***********************************************************************
* RD_Modify_DI - 写入开入板状态，处理变位等信息
*
* RETURNS: 无
*
*/
extern void RD_Modify_DI(
    RD_LGC_DI_CH *plgcdi,		/* DI配置 */
    int  iNowVal		/* 配置值 */
);


/* 更新GOOSE数据(DI/AI).
 * Para:
 *     pSubMapData, SUB数据集元素.
 *     Da_Val, 数据值.
 *     nStat, 状态.
 * Return:
 *     TRUE, or FALSE.
 * Alert:
 *     为提高效率,不对传入的指针进行判断,
 *     由传入方保证.
 */
BOOL ReadSubDaValueBySubMapData(void *pSubMapData, GOOSE_DA_VALUE *Da_Val, int *nStat);

/* functions */

/* 控制智能操作箱DO数据实时输出
 * 参数：   pvDoCh，用来索引DO数据元素的void指针，应该是HDL_Init_DO的返回值
 *          bClose，TRUE=控合；FALSE=控分
 * 返回值： 无 */
void HDL_Set_DO(void *pvDoCh, int iVal)
{

    HDL_DO_HND  *pHdl;

    assert(pvDoCh);
    pHdl=(HDL_DO_HND  *)pvDoCh;
    if(pHdl->ulPassWd!=HDL_DO_PASSWORD)
    {
        if(ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL,"Digital input/output error: IOB(Intelligent Operating Box) is forced with invalid DO operation,  reset CPU!\n",NULL);
        }
        else if(ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL,"开入开出异常:对智能操作箱准备非法进行开出, 复位CPU!\n",NULL);
        }
        EP_Set_Sts_Bit(REBOOT_DLY|SYS_LOCK_DO);
        return;
    }

    pHdl->iCurVal=iVal;
    return ;
}

/* 获取GOOSE DI缺省值.
 * Para:
 *     pvDiCh, DI通道句柄.
 *     nIdx, GOOSE DI序号.
 *     bInUse, 是否配置成功.
 *     bCurVal, 当前值.
 * Return:
 *     DI状态.
 */
static BOOL HDL_Get_DI_Default_Val(void *pvDiCh, int nIdx, BOOL bInUse, BOOL bCurVal)
{
    HDL_DI_HND *pHdl;

    assert (pvDiCh);
    pHdl = (HDL_DI_HND *)pvDiCh;

    if(pHdl->vtValType[nIdx]==BOOL_TYPE)
    {
        if(bInUse)
        {
            if(pHdl->bInvalidDftVal==1)
            {
                return TRUE;
            }
            else if(pHdl->bInvalidDftVal==2)
            {
                return pHdl->bValueBfInvalid;
            }
            else if(pHdl->bInvalidDftVal==3)
            {
                return bCurVal;
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
    else if(pHdl->vtValType[nIdx]==DPC_TYPE)
    {
        if(bInUse)
        {
            if(pHdl->bInvalidDftVal==1)
            {
                return DP_INVALID_00;
            }
            else if(pHdl->bInvalidDftVal==2)
            {
                return pHdl->bValueBfInvalid;
            }
            else if(pHdl->bInvalidDftVal==3)
            {
                return bCurVal;
            }
            else
            {
                return DP_INVALID_00;
            }
        }
        else
        {
            return DP_FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

/* 获取GOOSE DI初始值.
 * Para:
 *     pvDiCh, DI通道句柄.
 *     nIdx, GOOSE DI序号.
 *     bInUse, 是否配置成功.
 * Return:
 *     DI状态.
 */
static BOOL HDL_Get_DI_Original_Val(void *pvDiCh, int nIdx, BOOL bInUse)
{
    HDL_DI_HND *pHdl;

    assert (pvDiCh);
    pHdl = (HDL_DI_HND *)pvDiCh;

    if(pHdl->vtValType[nIdx]==BOOL_TYPE)
    {
        if(bInUse)
        {
            if(pHdl->bInvalidDftVal==1)
            {
                return TRUE;
            }
            else if(pHdl->bInvalidDftVal==2)
            {
                return FALSE;
            }
            else if(pHdl->bInvalidDftVal==3)
            {
                return FALSE;
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
    else if(pHdl->vtValType[nIdx]==DPC_TYPE)
    {
        if(bInUse)
        {
            if(pHdl->bInvalidDftVal==1)
            {
                return DP_TRUE;
            }
            else if(pHdl->bInvalidDftVal==2)
            {
                return DP_FALSE;
            }
            else if(pHdl->bInvalidDftVal==3)
            {
                return DP_FALSE;
            }
            else
            {
                return DP_FALSE;
            }
        }
        else
        {
            return DP_FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

/* 读取智能操作箱DI数据实时状态
 * Para:
 *     pvDiCh, DI通道句柄.
 *     nIdx, GOOSE端口序号.
 *     usQuality, 品质存储地址.
 * Return:
 *     此DI通道的当前状态,TRUE=闭合;FALSE=打开.
 */
BOOL HDL_Get_DI(void *pvDiCh, int nIdx, uint16_t *usQuality)
{
    HDL_DI_HND *pHdl;
    BOOL bDIVal;
    BOOL bGetSuccess;
    BOOL bSubRepairSts;  /* GOOSE SUB检修态 */
    BOOL bYabanSts = FALSE;
    pHdl = (HDL_DI_HND *)pvDiCh;

    *usQuality = 0;

    /* GOOSE端子有效性判断 */
    if ((pHdl->iSubDaIdx[nIdx] > 0)&&(pHdl->ppSubMapData[nIdx]!=NULL))
    {
        /* 端子有效 */

        /*
         * 压板判断,压板退出则直接返回FALSE
         */
        if(bSubGoYabanStat[pHdl->iSubNum[nIdx]])
        {
            bYabanSts=TRUE;
        }

        if(pHdl->ppiSubYabanIndex[nIdx] != NULL)
        {
            if(*pHdl->ppiSubYabanIndex[nIdx] != -1)
            {
                SCI_Get_Yaban_Value(*pHdl->ppiSubYabanIndex[nIdx],&bYabanSts,0);
            }
        }

        if (bYabanSts)
        {
            bGetSuccess = GO_GetActiveGoDIValByDaIndx(pHdl->iSubDaIdx[nIdx],
                          pHdl->ppSubMapData[nIdx], &bDIVal);		/* GOOSE状态读取及通信有效性判断 */

            if (bGetSuccess && (iHdlSubGoStat[pHdl->iSubNum[nIdx]]==1))
            {
                /* 读取正常且通信正常 */

                bSubRepairSts = GetSubTestModeByMap((SUB_MAP_INFO *)(pHdl->ppSubMapData[nIdx]));

                /* 检修状态 */
                if (bSubRepairSts)
                {
                    *usQuality |= DI_REPAIR_STS;
                }

                /* 保护测控一体化装置 */
                if (uiAppType_g == APP_PROT_MEA_MERGE)
                {
                    pHdl->bValueBfInvalid = bDIVal;

                    return bDIVal;
                }
                else
                {
                    if ((bSubRepairSts && (uiEdpStatus_g & ON_EXAM_STATE))
                            || ((!bSubRepairSts) && (!(uiEdpStatus_g & ON_EXAM_STATE))))
                    {
                        /* 无效数值时, 使用前一接收状态 */
                        if ((bDIVal == DP_INVALID_00) || (bDIVal == DP_INVALID_11))
                        {
                            *usQuality |= DI_INVALID_STS;  /* 无效 */

                            /* 传递给应用读取
                             */
                            if (bDIVal == DP_INVALID_00)
                            {
                                *usQuality |= DI_MID_00_STS;
                            }
                            else
                            {
                                *usQuality |= DI_INVALID_11_STS;
                            }

                            return pHdl->bValueBfInvalid;
                        }
                        else
                        {
                            /* 本装置检修态与GOOSE SUB检修态一致
                             * 接收压板投入
                             	 * 同时非无效值
                             * 记录并返回实际值
                             */
                            pHdl->bValueBfInvalid = bDIVal;

                            return bDIVal;
                        }
                    }
                    else  /* 其它情况返回缺省值 */
                    {
                        *usQuality |= DI_INVALID_STS;  /* 无效 */
                        *usQuality |= DI_REPAIR_DIF_STS;

                        return HDL_Get_DI_Default_Val(pHdl,nIdx,TRUE,bDIVal);
                    }
                }
            }
            else
            {
                *usQuality |= DI_INVALID_STS;  /* 无效 */
                *usQuality |= DI_COM_STS;
                if(iHdlSubGoStat[pHdl->iSubNum[nIdx]]==-1)/*初始状态*/
                {
                    return HDL_Get_DI_Original_Val(pHdl,nIdx,TRUE);
                }
                else
                {
                    /* 读取GOOSE状态失败或GOOSE SUB异常,
                     * 任何一个为FALSE即执行,包含所有其它情况
                     */
                    /* 通信断时,即使配置为传回实际值,
                     * 也作为传回失效前状态处理
                     */
                    return HDL_Get_DI_Default_Val(pHdl,nIdx,TRUE,pHdl->bValueBfInvalid);
                }
            }
        }
        else	/* 压板没有投入 不可用 */
        {
            *usQuality |= DI_INVALID_STS;  /* 无效 */
            *usQuality |= DI_LINK_STS;  /* 压板退出 */

            return HDL_Get_DI_Default_Val(pHdl,nIdx,FALSE,pHdl->bValueBfInvalid);
        }
    }
    else if (pHdl->iSubDaIdx[nIdx] == -2)
    {
        /* 端子悬空状态 不可用 */

        *usQuality |= DI_INVALID_STS; /* 无效 */
        *usQuality |= DI_SUSPEND_STS; /* 悬空配置 */
        *usQuality |= DI_CFG_INVALID; /* 配置无效 */

        return HDL_Get_DI_Default_Val(pHdl,nIdx,FALSE,pHdl->bValueBfInvalid);
    }
    else /* 不可用 */
    {
        *usQuality |= DI_INVALID_STS;  /* 无效 */
        *usQuality |= DI_CFG_INVALID; /* 配置无效 */

        return HDL_Get_DI_Default_Val(pHdl,nIdx,FALSE,pHdl->bValueBfInvalid);
    }
}

/* 读取智能操作箱DI数据实时状态(硬件配置句柄)
 * Para:
 *     pvDiCh, DI通道句柄.
 *     pulChgTime, 变位时采样节拍.
 *     usQuality, 品质存储地址.
 *     pbFilterSts, 消抖状态.
 * Return:
 *     此DI通道的当前状态,TRUE=闭合;FALSE=打开.
 */
BOOL HDL_Get_DI_Many(void *pvDiCh, uint32_t *ulChgNextCnt, uint16_t *pQuality,
                     BOOL *pbFilterSts)
{
    HDL_DI_HND  *pHdl;
    BOOL   bDIVal=FALSE;
    uint16_t usQualityForSingle = 0;
    int i;
    BOOL bValidRd = FALSE;  /* 读取到有效品质位 */
    BOOL bNow;
    pHdl=(HDL_DI_HND  *)pvDiCh;

    *pQuality = 0;

    for(i=0; i<HDL_DI_MAX_RECV_NUM; i++)
    {
        if((pHdl->iSubDaIdx[i]<=0)&&(pHdl->iSubDaIdx[i]!=-2))
        {
            /* 悬空时可继续读取 */
            if (!bValidRd)
            {
                *pQuality |= DI_INVALID_STS;
            }
            break;
        }
        usQualityForSingle = 0;
        bNow = HDL_Get_DI(pvDiCh, i, &usQualityForSingle);

        if (bNow != pHdl->bArrLstSts[i])
        {
            /* 重置变化通道计数器 */
            pHdl->ulArrFltCnt[i] = pHdl->ulFltCfg;
            pHdl->bArrChgFlag[i] = TRUE;
            pHdl->ulArrTmpNextCnt[i] = RD_GetScanCnt();
        }

        if (pHdl->bArrChgFlag[i])
        {
            *pbFilterSts = TRUE;
            if (!pHdl->ulArrFltCnt[i])
            {
                pHdl->bArrSts[i] = bNow;
                pHdl->bArrChgFlag[i] = FALSE;

                /* 以下操作只有开入通道和虚端子一对一时才有意义
                 * 因为任何一次变位都会导致以下赋值, 此时时标可能不正确
                 */
                HDL_Get_T_Many(pHdl, &pHdl->utChgTime);
                if (ulChgNextCnt != NULL)
                {
                    if (uiAppType_g == APP_PROT_MEA_MERGE)
                    {
                        *ulChgNextCnt = pHdl->ulArrTmpNextCnt[i];
                    }
                    else
                    {
                        *ulChgNextCnt = RD_GetScanCnt();
                    }
                }
            }
            else
                pHdl->ulArrFltCnt[i]--;
        }

        pHdl->bArrLstSts[i] = bNow;
        pHdl->usArrQuality[i] = usQualityForSingle;

        bDIVal |= pHdl->bArrSts[i];
        *pQuality |= pHdl->usArrQuality[i];
        bValidRd = TRUE;
    }
    return   bDIVal;
}

/* 读取同杆并架虚拟机箱AI数据实时状态
 * 参数：   pvAiCh，用来索引AI数据元素的void指针
 * 返回值： 此DI通道的当前状态，TRUE=闭合；FALSE=打开 */
float HDL_Get_AI(void *pvAiCh)
{

    HDL_AI_HND  *pHdl;
    SUB_MAP_INFO *pSubMapData;
    float   fAIVal;
    BOOL   bGetSuccess;
    BOOL bSubRepairSts;  /* GOOSE SUB检修态 */

    assert(pvAiCh);
    pHdl=(HDL_AI_HND  *)pvAiCh;
    pSubMapData=(SUB_MAP_INFO *)(pHdl->pSubMapData);
    pHdl->usQuality = 0;

    if((pHdl->iSubDaIdx<=0)||(pHdl->pSubMapData==NULL))
    {
        /*若句柄无效  */
        pHdl->usQuality |= 0x4000; /* 无效, 使用GOOSE数据的q属性 */
        if(pHdl->iSubDaIdx==-2)  /*接收模拟量端子悬空,返回0*/
            return 0.0;

        return  0.0;
    }
    bGetSuccess=GO_GetActiveGoAIValByDaIndx
                (pHdl->iSubDaIdx, pHdl->pSubMapData, &fAIVal);
    if(bGetSuccess && (iHdlSubGoStat[pSubMapData->pInfoNode->GcbIndex]==1))
    {
        bSubRepairSts = GetSubTestModeByMap(pSubMapData);
        /* 检修状态 */
        if (bSubRepairSts)
        {
            pHdl->usQuality |= 0x0010;
        }
        return   fAIVal;
    }
    else
    {
        pHdl->usQuality |= 0x4000; /* 无效 */
        return  0.0;
    }
}

/* 获取GOOSE DI变位时间
 * Para:
 *     pvDiCh, DI通道句柄.
 *     ultime, 变位时间.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL HDL_Get_T(void *pvDiCh, US_CNT_UTC_TIME *uttime, int nIdx)
{
    HDL_DI_HND *pHdl;

    assert (pvDiCh!=NULL);
    assert (uttime!=NULL);
    pHdl = (HDL_DI_HND *)pvDiCh;

    if ((pHdl->iSubTStampDaIdx[nIdx] > 0)&&(pHdl->ppSubMapTStamp[nIdx]!=NULL))
    {
        if (bSubGoYabanStat[pHdl->iSubNum[nIdx]]&&(iHdlSubGoStat[pHdl->iSubNum[nIdx]]==1))
        {
            return GO_GetActiveGoTByDaIndx(pHdl->iSubTStampDaIdx[nIdx], pHdl->ppSubMapTStamp[nIdx],uttime);
        }
        else
        {
            uttime->ullusCntFrom1970=0;
            uttime->ucQflag=0x60;
            return FALSE;
        }
    }
    else
    {
        uttime->ullusCntFrom1970=0;
        uttime->ucQflag=0x60;
        return FALSE;
    }
}

BOOL HDL_Get_T_Many(void *pvDiCh, US_CNT_UTC_TIME  *uttime)
{
    HDL_DI_HND  *pHdl;
    int i;
    US_CNT_UTC_TIME utTimeTmp;

    assert (pvDiCh!=NULL);
    assert (uttime!=NULL);

    uttime->ullusCntFrom1970=0;
    uttime->ucQflag=0x60;
    pHdl=(HDL_DI_HND  *)pvDiCh;

    for(i=0; i<HDL_DI_MAX_RECV_NUM; i++)
    {
        if((pHdl->iSubTStampDaIdx[i]<=0)&&(pHdl->iSubTStampDaIdx[i]!=-2))
            break;
        if(HDL_Get_T(pvDiCh, &utTimeTmp, i))
        {
            /*if((utTimeTmp.ucQflag&0x60)==0)*/
            {
                if(utTimeTmp.ullusCntFrom1970>uttime->ullusCntFrom1970)
                    *uttime=utTimeTmp;
            }
        }
    }

    if((uttime->ucQflag&0x60)==0)
        return TRUE;
    else
        return FALSE;
}

/* 获取GOOSE DI变位时间
 * Para:
 *     pvDiCh, DI通道句柄.
 *     ultime, 变位时间.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL HDL_Get_PacketT(void *pvDiCh, US_CNT_UTC_TIME *uttime, int nIdx)
{
    HDL_DI_HND *pHdl;

    assert (pvDiCh!=NULL);
    assert (uttime!=NULL);
    pHdl = (HDL_DI_HND *)pvDiCh;

    if ((pHdl->iSubDaIdx[nIdx] > 0)&&(pHdl->ppSubMapData[nIdx]!=NULL))
    {
        if (bSubGoYabanStat[pHdl->iSubNum[nIdx]]
                &&(iHdlSubGoStat[pHdl->iSubNum[nIdx]]==1))
        {
            return GO_GetTPacketBySubMapData(pHdl->ppSubMapData[nIdx],uttime);
        }
        else
        {
            uttime->ullusCntFrom1970=0;
            uttime->ucQflag=0x60;
            return FALSE;
        }
    }
    else
    {
        uttime->ullusCntFrom1970=0;
        uttime->ucQflag=0x60;
        return FALSE;
    }
}

BOOL HDL_Get_PacketT_Many(void *pvDiCh, US_CNT_UTC_TIME  *uttime)
{
    HDL_DI_HND  *pHdl;
    int i;
    US_CNT_UTC_TIME utTimeTmp;

    assert (pvDiCh!=NULL);
    assert (uttime!=NULL);

    uttime->ullusCntFrom1970=0;
    uttime->ucQflag=0x60;
    pHdl=(HDL_DI_HND  *)pvDiCh;

    for(i=0; i<HDL_DI_MAX_RECV_NUM; i++)
    {
        if(pHdl->iSubDaIdx[i]<=0)
            break;

        if(HDL_Get_PacketT(pvDiCh, &utTimeTmp, i))
        {
            /*if((utTimeTmp.ucQflag&0x60)==0)*/
            {
                if(utTimeTmp.ullusCntFrom1970>uttime->ullusCntFrom1970)
                    *uttime=utTimeTmp;
            }
        }
    }

    if((uttime->ucQflag&0x60)==0)
        return TRUE;
    else
        return FALSE;
}

/* 功能：获得智能操作箱数据源的active goose,某DO通道输出的数据  2007-3-27日 张云
   参数：
        uiDONum：		在该数据源active goose的DO序号，序号从1开始，
        pbRtData:               返回的该DO通道的数据
   返回：
        TRUE； 若有匹配数据，则操作成功
        FALSE：操作失败
  	*/

BOOL  HDL_GetActiveGoDoData(uint16_t uiDONum,
                            int *pbRtData)
{

    int   iCurVal;

    /* 取经过压板“与”操作之后的结果 */
    iCurVal = HdlBoxIOInfo_g.aphHdlDoIdx[uiDONum-1]->iLstPubVal;
    *pbRtData=iCurVal;

    return   TRUE;

}

/* 功能：获得智能操作箱数据源的active goose,某DO通道变位时间
   参数：
        uiDONum：		在该数据源active goose的DO序号，序号从1开始，
        pbRtData:               返回的该DO通道的数据
   返回：
        TRUE； 若有匹配数据，则操作成功
        FALSE：操作失败
  	*/

BOOL  HDL_GetActiveGoDoTData(uint16_t uiDONum, US_CNT_UTC_TIME *ptmTData)
{
    *ptmTData=HdlBoxIOInfo_g.aphHdlDoIdx[uiDONum-1]->utChgTime;

    return   TRUE;
}


/* 取得智能操作箱AI逻辑通道和预处理数据指针,必须要求和本机的AD采样刷新同时调用
 * 参数：   pvAiMod，用来索引智能操作箱AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulSmplClk，本机的递增采样时钟，
 *          ppxWr，用来返回指向该同杆AI引擎的第0个预处理通道数据的指针，这里应该返回为NULL
 * 返回值： 指向该智能操作箱AI引擎的第0个逻辑采样通道数据的指针，这里应该返回为NULL
 * 注意：   要求AI引擎按照采样次序调用此函数获得指针来写实时数据，
 *          AI引擎可以对指针进行直接的写操作（如果没有预处理通道，则*ppxWr无效），
 *          内部数据按照通道优先的方式进行组织， */
float *HDL_AI_Dat_P(void *pvAiMod, uint32_t ulSmplClk,COMPLEX **ppxWr)
{

    RD_AI_MOD *paimod;
    int i;
    uint32_t  ulMissedClk;
    static  BOOL   bFirstEnter_s=TRUE;

    paimod=(RD_AI_MOD*)pvAiMod;


    if (!bFirstEnter_s)
    {
        /* 若是非首次进去 */

        if((ulSmplClk==paimod->ulHeadClk+1) ||ulSmplClk==0)
        {
            /*若未丢点  */
            paimod->pfWork=(float*)((uint8_t*)paimod->pfWork+lgcaidb_g.uiChBytes);
            if (paimod->pfWork>=lgcaidb_g.pfBufEnd)
            {
                paimod->pfWork=paimod->pfDbBgn;
                paimod->pxWork=paimod->pxDbBgn;
                paimod->pbAiValidDbWork=paimod->pbAiValidDbBgn;
            }
            else
            {
                paimod->pxWork=(COMPLEX*)((uint8_t*)paimod->pxWork+calcaidb_g.uiChBytes);
                paimod->pbAiValidDbWork=paimod->pbAiValidDbWork+aivaliddb_g.uiTotalCh;
            }

            paimod->ulHeadClk=ulSmplClk;
            paimod->ulNextCnt++;
        }
        else  if((ulSmplClk==(paimod->ulHeadClk+2)%RD_SAM_SYN_CLK)
                 ||(ulSmplClk==(paimod->ulHeadClk+3)%RD_SAM_SYN_CLK))
        {
            /*若丢1点或2点  */
            if(ulSmplClk==(paimod->ulHeadClk+2)%RD_SAM_SYN_CLK)
            {
                ulMissedClk=1;
            }
            else
            {
                ulMissedClk=2;
            }
            if (paimod==&aimodHdl_g)
            {
                static uint32_t  ulErrCnt=0;
                ulErrCnt++;
                if(ulErrCnt%20000==1)
                {
                    /*每20000次报一次，第1次必须报   */

                    LOG_Write(LOG_KERNEL, "提示: HDL AI数据丢失 .\n", NULL);
                }
            }
            else
            {
                assert(FALSE);
            }
            for(i=0; i<=ulMissedClk; i++)
            {
                /*返回正确的写位置 */
                paimod->pfWork=(float*)((uint8_t*)paimod->pfWork+lgcaidb_g.uiChBytes);
                if (paimod->pfWork>=lgcaidb_g.pfBufEnd)
                {
                    paimod->pfWork=paimod->pfDbBgn;
                    paimod->pxWork=paimod->pxDbBgn;
                    paimod->pbAiValidDbWork=paimod->pbAiValidDbBgn;
                }
                else
                {
                    paimod->pxWork=(COMPLEX*)((uint8_t*)paimod->pxWork+calcaidb_g.uiChBytes);
                    paimod->pbAiValidDbWork=paimod->pbAiValidDbWork+aivaliddb_g.uiTotalCh;
                }

                paimod->ulNextCnt++;
            }

            paimod->ulHeadClk=ulSmplClk;
        }
        else
        {
            /*若丢多点,则设置错误  */

            if (paimod==&aimodHdl_g)
            {
                static uint32_t  ulErrCnt=0;
                ulErrCnt++;
                if(ulErrCnt%20000==1)
                {
                    /*每20000次报一次，第1次必须报，但不闭锁  */
                    char TempInfo[256];
                    if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
                    {
                        ER_Set_Err(EV_SAMPLE_ERR, ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                                   "hdl box data  clock missed\n"
                                   ,0,0);
                    }
                    else if(ENG_MODE == 0)
                    {
                        ER_Set_Err(EV_SAMPLE_ERR, ER_REPORT|ER_ALARM|ER_LOCK|ER_NOLOGWRITE,
                                   "hdl机箱数据连续丢点\n"
                                   ,0,0);
                    }

                    sprintf(TempInfo,"同杆并架数据异常: 同杆并架机箱数据连续丢点.前次采样点是  %d,本次采样点是  %d \n"
                            , (int)paimod->ulHeadClk, (int)ulSmplClk);
                    LOG_Write(LOG_KERNEL, TempInfo, NULL);
                }
            }
            paimod->ulHeadClk=ulSmplClk;
        }

    }
    else
    {
        /*必须要求和本机的AD采样刷新同时调用和刷新*/
        uint32_t  ulCurDspModNextCnt;
        int  iCurDspModDbOfst;

        bFirstEnter_s=FALSE;

        /*此时根据DSP MOD的信息，来获得同杆并架机箱初始化时在DB中的相应位置，
          注意此时调用时，DSP MOD还没有进行此轮刷新  */
        RD_Get_DSP_MOD_Info(&ulCurDspModNextCnt,&iCurDspModDbOfst);

        /*前一点的MOD信息  */
        paimod->pfWork=paimod->pfDbBgn+iCurDspModDbOfst;
        paimod->pxWork=paimod->pxDbBgn+iCurDspModDbOfst;
        paimod->ulNextCnt=ulCurDspModNextCnt;
        paimod->pbAiValidDbWork=paimod->pbAiValidDbBgn+iCurDspModDbOfst;

        paimod->pfWork=(float*)((uint8_t*)paimod->pfWork+lgcaidb_g.uiChBytes);
        if (paimod->pfWork>=lgcaidb_g.pfBufEnd)
        {
            paimod->pfWork=paimod->pfDbBgn;
            paimod->pxWork=paimod->pxDbBgn;
            paimod->pbAiValidDbWork=paimod->pbAiValidDbBgn;
        }
        else
        {
            paimod->pxWork=(COMPLEX*)((uint8_t*)paimod->pxWork+calcaidb_g.uiChBytes);
            paimod->pbAiValidDbWork=paimod->pbAiValidDbWork+aivaliddb_g.uiTotalCh;
        }

        paimod->ulHeadClk=ulSmplClk;
        paimod->ulNextCnt++;
    }

    *ppxWr=paimod->pxWork;
    return paimod->pfWork;
}



/* 报告智能操作相AI引擎完成一次数据刷新 要求此时rdinfo_g.ulCurrAiCnt还没有被本机的采样来刷新
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 *          ulAiCnt,该数据刷新对应的本机AI采样时刻
 * 返回值： 无 */
void  HDL_End_Ai_Wr(void *pvAiMod)
{
    uint32_t ulAiCnt;
    int iNewPts;

    /*2007-10-29 DQ: 当同杆未配di或带扩展机箱时，不进行di刷新*/
    if((!iHdlDiNum_g) || bDspDrvMod)
        return ;

    ulAiCnt=rdinfo_g.ulCurrAiCnt;
    iNewPts=aimodHdl_g.ulNextCnt-1-ulAiCnt;/*要求此时rdinfo_g.ulCurrAiCnt还没有被本机的采样来刷新  */

    while (iNewPts--)
    {
        HDL_Refresh_DI(pvAiMod);
    }

    return   ;


}


/* 刷新智能操作相的DI数据
 * 参数：   pvAiMod，用来索引AI引擎的void指针，应该由本模块在初始
 *              化AI通道的时候提供给底层I/O
 * 返回值： 无 */
void HDL_Refresh_DI(void *pvAiMod)
{

    BOOL  bCurAiIsFast=FALSE;
    BOOL  bCurAiIsMid=FALSE;
    BOOL  bCurAiIsSlow=FALSE;
    BOOL  bCurAiISFasterThanFast=FALSE;
    static   uint32_t   ulRefreshCnt=0;

    /*2007-4-13日 张云 ，目前该版本实现只支持如下设置  */
    //assert(DI_SLOW_REFRESH_INTERVAL==12);
    //assert(DI_MID_REFRESH_INTERVAL==6);
    //assert(DI_FAST_REFRESH_INTERVAL==2);

    ulRefreshCnt++;

    if((ulRefreshCnt%DI_SLOW_REFRESH_INTERVAL)==1)
    {
        bCurAiIsSlow=TRUE;
    }

    /* 全部为慢速则不进行如下处理 */
    if (!bAllSnowFlag)
    {
        if((ulRefreshCnt%DI_MID_REFRESH_INTERVAL)==1)
        {
            bCurAiIsMid=TRUE;
        }
        if((ulRefreshCnt%DI_FAST_REFRESH_INTERVAL)==1
                ||DI_FAST_REFRESH_INTERVAL==1)
        {
            bCurAiIsFast=TRUE;
        }

        if ((usMinInterval<DI_FAST_REFRESH_INTERVAL) && (usMinInterval >= 1)
                && (uiAppType_g != APP_BUS)) /* 防止扩展机箱1点扫描时频繁更新 */
        {
            bCurAiISFasterThanFast=TRUE;
        }
    }

    if(bCurAiIsFast||bCurAiIsMid||bCurAiIsSlow||bCurAiISFasterThanFast)
    {
        /* 若需要刷新 */
        BOOL  *pBaseWork;
        BOOL  *pbFirst;
        BOOL *pbSecond;
        BOOL *pbThird;
        BOOL *pbForth;
        BOOL *pbFifth;
        BOOL *pbSixth;
        BOOL *pbSeventh;
        BOOL *pbEighth;
        BOOL *pbNinth;
        BOOL *pbTenth;
        BOOL *pbEleventh;
        BOOL *pbTwelvth;
        RD_LGC_DI_CH *plgcdi;
        int  i;
        int iNow;
        uint16_t usQuality = 0; /* 品质位 */
        BOOL btempChgFlag;
        BOOL bFilterSts = FALSE;

        HDL_GetGooseDiNeedRefresh(&btempChgFlag);

        if(bCurAiIsSlow)
        {
            /*若是慢速，则获得相应的多次存储的基址  */
            pBaseWork=didb_g.pbWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFirst=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSecond=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbThird=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbForth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFifth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSixth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSeventh=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbEighth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbNinth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbTenth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbEleventh=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbTwelvth=pBaseWork;

        }
        else  if(bCurAiIsMid)
        {
            pBaseWork=didb_g.pbWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFirst=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSecond=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbThird=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbForth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFifth=pBaseWork;
            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSixth=pBaseWork;

        }
        else if(bCurAiIsFast)
        {
            pBaseWork=didb_g.pbWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFirst=pBaseWork;

            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSecond=pBaseWork; 					/* 2点 */
        }
        else if(bCurAiISFasterThanFast)
        {
            pBaseWork=didb_g.pbWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFirst=pBaseWork;
        }
        else
        {
            pBaseWork=didb_g.pbWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbFirst=pBaseWork;

            pBaseWork=pBaseWork+didb_g.uiTotalCh;
            if (pBaseWork>=didb_g.pbBufEnd)
                pBaseWork=didb_g.pbBufBgn;
            pbSecond=pBaseWork;
        }

        for (plgcdi=plgcdich_g,i=0; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++,i++)
        {
            /*对每个ＤＩ通道判定，是否需要刷新  */

            /* 仅处理操作箱DI, 同时悬空或裁剪后不处理 */
            if ((plgcdi->mod != RD_HDL_BOX_DI)
                    || (plgcdi->bPended && (uiAppType_g != APP_BUS)))
            {
                continue;
            }

            usQuality = 0;
            if(plgcdi->ucDIRefreshRate==DI_FAST_REFRESH_RATE
                    &&bCurAiISFasterThanFast)
            {
                /*按逻辑图扫描节拍刷新 */
                if ((plgcdi->iForceSts==-1))
                {
                    /*若是相应智能操作箱的DI，且不是强制，则更新缓冲，若是强制，则在RD_Refresh_DI中进行了更新  */

                    if (btempChgFlag)
                    {
                        iNow = HDL_Get_DI_Many(plgcdi->pvSrc, &plgcdi->ulChgNextCnt,
                                               &usQuality, &bFilterSts);
                        /* iNow = Hdl_Filt_And_Get_DI(plgcdi->pvSrc, &plgcdi->ulChgNextCnt, &usQuality); */
                        plgcdi->iLstVal = iNow;
                        plgcdi->usLstQuality = usQuality;
                    }
                    else
                    {
                        iNow = plgcdi->iLstVal;
                        usQuality = plgcdi->usLstQuality;
                    }

                    *(pbFirst+i)=iNow;
                    HDL_Modify_DI(plgcdi,iNow, usQuality);
                }
                else
                {
                    if (btempChgFlag)
                    {
                        iNow = HDL_Get_DI_Many(plgcdi->pvSrc, &plgcdi->ulChgNextCnt,
                                               &usQuality, &bFilterSts);
                        plgcdi->iLstVal = iNow;
                        plgcdi->usLstQuality = usQuality;
                        plgcdi->usQuality = usQuality;
                    }

                    iNow = plgcdi->iForceSts; /* 控制结果 */
                    *(pbFirst+i) = iNow;
                    iNow |= 0x8000;
                    RD_Modify_DI(plgcdi, iNow);			/* Every kind of DI wil be refresh independently, DY 9/15/2007 */
                }

            }
            else if(plgcdi->ucDIRefreshRate==DI_FAST_REFRESH_RATE
                    &&bCurAiIsFast)
            {
                /*若是快速通道，且此次是快速刷新节拍，则快速刷新３点  */
                if ((plgcdi->iForceSts==-1))
                {
                    /*若是相应智能操作箱的DI，且不是强制，则更新缓冲，若是强制，则在RD_Refresh_DI中进行了更新  */

                    if (btempChgFlag)
                    {
                        iNow = HDL_Get_DI_Many(plgcdi->pvSrc, &plgcdi->ulChgNextCnt,
                                               &usQuality, &bFilterSts);
                        /* iNow = Hdl_Filt_And_Get_DI(plgcdi->pvSrc, &plgcdi->ulChgNextCnt, &usQuality); */
                        plgcdi->iLstVal = iNow;
                        plgcdi->usLstQuality = usQuality;
                    }
                    else
                    {
                        iNow = plgcdi->iLstVal;
                        usQuality = plgcdi->usLstQuality;
                    }

                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    /*
                    *(pbThird+i)=iNow;
                    */
                    HDL_Modify_DI(plgcdi, iNow, usQuality);
                }
                else
                {

                    if (btempChgFlag)
                    {
                        iNow = HDL_Get_DI_Many(plgcdi->pvSrc, &plgcdi->ulChgNextCnt,
                                               &usQuality, &bFilterSts);
                        plgcdi->iLstVal = iNow;
                        plgcdi->usLstQuality = usQuality;
                        plgcdi->usQuality = usQuality;
                    }

                    iNow=plgcdi->iForceSts; /* 控制结果 */
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    iNow |= 0x8000;
                    RD_Modify_DI(plgcdi,iNow);			/* Every kind of DI wil be refresh independently, DY 9/15/2007 */
                }

            }
            else  if(bCurAiIsMid
                     &&plgcdi->ucDIRefreshRate==DI_MID_REFRESH_RATE)
            {
                /*若是中速通道，且此次是中速刷新节拍，则中速刷新６点  */

                if ((plgcdi->iForceSts==-1))
                {
                    if (btempChgFlag)
                    {
                        iNow = HDL_Get_DI_Many(plgcdi->pvSrc, &plgcdi->ulChgNextCnt,
                                               &usQuality, &bFilterSts);
                        /* iNow = Hdl_Filt_And_Get_DI(plgcdi->pvSrc, &plgcdi->ulChgNextCnt, &usQuality); */
                        plgcdi->iLstVal = iNow;
                        plgcdi->usLstQuality = usQuality;
                    }
                    else
                    {
                        iNow = plgcdi->iLstVal;
                        usQuality = plgcdi->usLstQuality;
                    }

                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    HDL_Modify_DI(plgcdi,iNow, usQuality);
                }
                else
                {

                    if (btempChgFlag)
                    {
                        iNow = HDL_Get_DI_Many(plgcdi->pvSrc, &plgcdi->ulChgNextCnt,
                                               &usQuality, &bFilterSts);
                        plgcdi->iLstVal = iNow;
                        plgcdi->usLstQuality = usQuality;
                        plgcdi->usQuality = usQuality;
                    }

                    iNow=plgcdi->iForceSts;
                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    iNow |= 0x8000;
                    RD_Modify_DI(plgcdi,iNow);
                }
            }
            else if(bCurAiIsSlow
                    &&plgcdi->ucDIRefreshRate==DI_SLOW_REFRESH_RATE)
            {
                /*若是慢速通道，且此次是慢速刷新节拍，则慢速刷新１２点，若是其他，则对该通道空操作  */
                if ((plgcdi->iForceSts==-1))
                {
                    if (btempChgFlag)
                    {
                        iNow = HDL_Get_DI_Many(plgcdi->pvSrc, &plgcdi->ulChgNextCnt,
                                               &usQuality, &bFilterSts);
                        /* iNow = Hdl_Filt_And_Get_DI(plgcdi->pvSrc, &plgcdi->ulChgNextCnt, &usQuality); */
                        plgcdi->iLstVal = iNow;
                        plgcdi->usLstQuality = usQuality;
                    }
                    else
                    {
                        iNow = plgcdi->iLstVal;
                        usQuality = plgcdi->usLstQuality;
                    }

                    *(pbFirst+i)=iNow;
                    *(pbSecond+i)=iNow;
                    *(pbThird+i)=iNow;
                    *(pbForth+i)=iNow;
                    *(pbFifth+i)=iNow;
                    *(pbSixth+i)=iNow;
                    *(pbSeventh+i)=iNow;
                    *(pbEighth+i)=iNow;
                    *(pbNinth+i)=iNow;
                    *(pbTenth+i)=iNow;
                    *(pbEleventh+i)=iNow;
                    *(pbTwelvth+i)=iNow;
                    HDL_Modify_DI(plgcdi,iNow, usQuality);
                }
                else
                {

                    if (btempChgFlag)
                    {
                        iNow = HDL_Get_DI_Many(plgcdi->pvSrc, &plgcdi->ulChgNextCnt,
                                               &usQuality, &bFilterSts);
                        plgcdi->iLstVal = iNow;
                        plgcdi->usLstQuality = usQuality;
                        plgcdi->usQuality = usQuality;
                    }

                    iNow = plgcdi->iForceSts;
                    *(pbFirst+i) = iNow;
                    *(pbSecond+i) = iNow;
                    *(pbThird+i) = iNow;
                    *(pbForth+i) = iNow;
                    *(pbFifth+i) = iNow;
                    *(pbSixth+i) = iNow;
                    *(pbSeventh+i) = iNow;
                    *(pbEighth+i) = iNow;
                    *(pbNinth+i) = iNow;
                    *(pbTenth+i) = iNow;
                    *(pbEleventh+i) = iNow;
                    *(pbTwelvth+i) = iNow;

                    iNow |= 0x8000;
                    RD_Modify_DI(plgcdi,iNow);
                }
            }/*else if(bCurAiIsSlow
        &&plgcdi->ucDIRefreshRate==DI_SLOW_REFRESH_RATE)结束  */
        }/*for循环结束  */

        /* 清除更新标识, 仅更新一次
         */
        if (btempChgFlag && (!bFilterSts) && bCurAiIsSlow)
        {
            HDL_SetGooseDiNeedRefresh(FALSE);
        }
    }/*if结束 */

    return;
}



/* 写入智能操作相DI的开入板状态，处理变位等信息
 * 参数：   plgcdi，  DI句柄
 *          iNowVal,  此时DI值
 * 返回值： 无 */
/* */
static void HDL_Modify_DI(RD_LGC_DI_CH *plgcdi, int iNowVal, uint16_t usQuality)
{
    /*设置开入摸件的32位开入值中的相关位  */
    if(!(iNowVal & 0x7FFF))
    {
        /*若为0  */
        plgcdi->p_part->ulDIModCurVaule &= (~(ulDwordBitArr_g[plgcdi->ucModCh]));
    }
    else
    {
        /*若为1  */
        plgcdi->p_part->ulDIModCurVaule |= (ulDwordBitArr_g[plgcdi->ucModCh]);
    }

    plgcdi->usQuality = usQuality;

    if (plgcdi->iVal!=iNowVal)
    {
        RE_SetLogDIUpdateCnt();
//            if ((plgcdi->iMeaCh!=-1/*||(plgcdi->iMeaCh==-1&&plgcdi->mod==RD_HDL_BOX_DI)*/)
//            && (plgcdi->iVal & 0x7FFF)!=(iNowVal & 0x7FFF))
        if ((plgcdi->iVal & 0x7FFF)!=(iNowVal & 0x7FFF))
        {
            /* 母差独立处理 */
            if (uiAppType_g != APP_BUS)
            {
                /* 获取变位时间, 非强制 */
                HDL_Get_T(plgcdi->pvSrc, &(((HDL_DI_HND *)(plgcdi->pvSrc))->utChgTime), 0);
                HDL_Get_PacketT(plgcdi->pvSrc, &(((HDL_DI_HND *)(plgcdi->pvSrc))->utPacketTime), 0);
            }
            else
            {
                HDL_Get_T_Many(plgcdi->pvSrc, &(((HDL_DI_HND *)(plgcdi->pvSrc))->utChgTime));
                HDL_Get_PacketT_Many(plgcdi->pvSrc, &(((HDL_DI_HND *)(plgcdi->pvSrc))->utPacketTime));
            }

            /* Report SOE. */
            if ((plgcdi->iVal | iNowVal) & 0x8000)
            {
                /*2014-5-23  ZY */
                TM_High_Get_Sys_Us_UTC_Time(&plgcdi->utChgTime,&plgcdi->ulChgTime);
                if(plgcdi->iMeaCh!=-1)
                    VI_New_SOE(plgcdi->iMeaCh, (iNowVal & 0x7FFF), plgcdi->ulChgTime, plgcdi->bSOE, usQuality);
            }
            else
            {
                plgcdi->utChgTime =((HDL_DI_HND *)(plgcdi->pvSrc))->utChgTime;
               /*if((((HDL_DI_HND *)(plgcdi->pvSrc))->utChgTime.ucQflag&0x60)!=0)*/
               if(((HDL_DI_HND *)(plgcdi->pvSrc))->utChgTime.ullusCntFrom1970==0)
               {
                   /* plgcdi->ulChgTime = RD_AI_Cnt_To_us(plgcdi->ulChgNextCnt-1); */
                   plgcdi->ulChgTime = 0;
               }
               else
               {
                   BOOL bTransOK=TRUE;/*2013-5-23  ZY  */
                   plgcdi->ulChgTime = TM_High_Us_UTC_Time_To_us32Cnt(((HDL_DI_HND *)(plgcdi->pvSrc))->utChgTime, &bTransOK);
                   if(!bTransOK)
                       plgcdi->ulChgTime = RD_AI_Cnt_To_us(plgcdi->ulChgNextCnt-1);
               }

               plgcdi->utChgPackTime = ((HDL_DI_HND *)(plgcdi->pvSrc))->utPacketTime;

               if(((HDL_DI_HND *)(plgcdi->pvSrc))->utPacketTime.ullusCntFrom1970==0)
               {
                   plgcdi->ulChgPackTime = plgcdi->ulChgTime;
                   /*logMsg("HDL Get packet time error\n",1,2,3,4,5,6);*/
               }
               else
               {
                   BOOL bTransOK=TRUE;/*2013-5-23  ZY  */
                   plgcdi->ulChgPackTime = TM_High_Us_UTC_Time_To_us32Cnt(((HDL_DI_HND *)(plgcdi->pvSrc))->utPacketTime, &bTransOK);
                   if(!bTransOK)
                   {
                       plgcdi->ulChgPackTime = plgcdi->ulChgTime;
                       /*logMsg("HDL Trans packet time error\n",1,2,3,4,5,6);*/
                   }

               }

               if (plgcdi->ulChgTime == 0)
               {
                   /* 如果GOOSE开入未关联时标,则使用GOOSE报文到达时标 */
                   plgcdi->ulChgTime = plgcdi->ulChgPackTime;
                   memcpy(&plgcdi->utChgTime, &plgcdi->utChgPackTime, sizeof(plgcdi->utChgTime));
                   if(plgcdi->utChgTime.ullusCntFrom1970 == 0)
                   {
                       TM_High_Get_Sys_Us_UTC_Time(&plgcdi->utChgTime,&plgcdi->ulChgTime);
                   }
               }

                if(plgcdi->iMeaCh!=-1)
                    VI_New_SOE(plgcdi->iMeaCh, iNowVal, plgcdi->ulChgTime, plgcdi->bSOE, usQuality);
            }
        }
        plgcdi->iVal=iNowVal;
    }
}



/*刷新新的智能操作箱数据,必须在本机当地的数据刷新RD_END_AI_WR函数完成之前被调用
  参数：ulSmplClk，本机的递增采样时钟，
  返回   无*/
void  HDL_Read_AI_Data(uint32_t ulSmplClk)
{
    /*注意，没有复数式数据,要求每点数据都更新  */
    int  i;
    float *hdx_float;

#ifndef NO_DBL_BUF
    float *d_buf_float;
#endif

    COMPLEX * temp_p_complex;
    //float  fTempVal;
    void  *  pvMod;
    static uint32_t ulReadCnt=0;


    if(!bHdlBoxIsInit_g)
    {
        /*若智能操作箱没有被初始化好，则空操作  */
        return;
    }

    pvMod=&aimodHdl_g;
    hdx_float = (float*)HDL_AI_Dat_P(pvMod,ulSmplClk, (COMPLEX**)&temp_p_complex); 	/* 2007-6-19日张云添加修改 */

    ++ulReadCnt;

#if 1	/* 针对测控CPU的效率问题，进行优化，GOOSE AI不在加入采样缓存 */
    if(ulReadCnt%100 == 1)
    {
        for(i=0; i<HdlBoxAICfg_g.iHdlAINum; i++)
        {
            HdlBoxAICfg_g.apHdlBoxAIIdx[i]->fBufVal=HDL_Get_AI(HdlBoxAICfg_g.apHdlBoxAIIdx[i]);
        }
    }
#else
#ifndef NO_DBL_BUF 					/*若是双缓冲  */

    d_buf_float=(float*)((uint8_t*)hdx_float-lgcaidb_g.ulBufBytes);
    for(i=0; i<HdlBoxAICfg_g.iHdlAINum; i++)
    {
        if(ulReadCnt&0x01)    /*2007-10-30 DQ: 改为每两次进行一次HDL_Get_AI，另一次则读上次的缓存值*/
        {
            fTempVal=HDL_Get_AI(HdlBoxAICfg_g.apHdlBoxAIIdx[i]);
            *hdx_float++=fTempVal;
            *d_buf_float++=fTempVal;
            HdlBoxAICfg_g.apHdlBoxAIIdx[i]->fBufVal=fTempVal;
        }
        else
        {
            fTempVal=HdlBoxAICfg_g.apHdlBoxAIIdx[i]->fBufVal;
            *hdx_float++=fTempVal;
            *d_buf_float++=fTempVal;
        }
    }

#else                                                  /*若是单缓冲  */
    for(i=0; i<HdlBoxAICfg_g.iHdlAINum; i++)
    {
        if(ulReadCnt&0x01)
        {
            fTempVal=HDL_Get_AI(HdlBoxAICfg_g.apHdlBoxAIIdx[i]);
            *hdx_float++=fTempVal;
            HdlBoxAICfg_g.apHdlBoxAIIdx[i]->fBufVal=fTempVal;
        }
        else
        {
            fTempVal=HdlBoxAICfg_g.apHdlBoxAIIdx[i]->fBufVal;
            *hdx_float++=fTempVal;
        }
    }
#endif
#endif
//	wvEvent(2,1,1);
    HDL_End_Ai_Wr(pvMod);
//	wvEvent(3,1,1);
}

/* 智能操作箱数据是否有效
 * Para:
 *     ulScanTaskNo, 扫描任务号.
 *     ulGrpScanDriveInterval, 扫描间隔.
 * Return:
 *     TRUE, or FALSE.
 * Alert:
 *     只有当所有SUB组数据都有效时,才返回数据有效
 *     而对某组SUB数据而言,只要该组SUB的任何一个网络的数据有效,则该组SUB数据有效
 *
 */
BOOL HDL_Data_Is_Valid(uint32_t ulScanTaskNo, uint32_t ulGrpScanDriveInterval)
{
    int j;
    uint32_t k;
    static BOOL bDataIsValid; /* 所有任务共享 */
    static BOOL bLastDataIsValid = 0x10;
    BOOL bCurSubDataIsValid;
    uint8_t aucAllNetSubStat[MAX_GSE_NET_CNT];
    uint8_t aucAllNetSubStatOrigin[MAX_GSE_NET_CNT];
    int iValidNetCnt;
    uint16_t unErrCode = 0;
    BOOL bGooseDiNeedRefresh = FALSE;

    static BOOL bIsFstEnter_s = TRUE;

    static ALERT_HDL *pHdlAlert;
    static uint32_t uiCount;
    static uint32_t uiStsChgCnt=0;
    static BOOL bIsFstPoll = TRUE;


    if (ulScanTaskNo != 0)
    {
        return bDataIsValid;
    }

    /* 滞后更新通信状态,
     * 防止检修状态还没有置时更新了GOOSE DI
     */
    if (bIsFstPoll)
    {
        static uint32_t ulCount;

        ulCount++;

        if ((ulCount*ulGrpScanDriveInterval) > (uiAiRate_g/10))
        {
            bIsFstPoll = FALSE;
        }

        return bDataIsValid;
    }

    if(GetCommStsChangeFlag())
    {
        uiStsChgCnt++;

        ClearCommStsChangeFlag();
    }
    else
    {
        uiStsChgCnt=0;
    }

    if ((uiCount++!=0)
            &&((uiStsChgCnt>5)||(uiStsChgCnt==0)))
    {

        if ((uiCount*ulGrpScanDriveInterval) > (uiAiRate_g/100))
        {
            uiCount = 0;
        }

        return bDataIsValid;
    }

    /* 若SUB个数为0, 则数据亦无效
     * 为防止应用调用出错, 返回TRUE
     */
    if (iHdlSubGoNum_g == 0)
    {
        bDataIsValid = TRUE;

        return bDataIsValid;
    }

    /*
     * 若智能操作箱没有被初始化好,则数据无效
     *
     */
    if (!bHdlBoxIsInit_g)
    {
        return FALSE;
    }

    /* 初始执行
     * 初始化相关变量
     */
    if (bIsFstEnter_s)
    {
        bIsFstEnter_s = FALSE;

        /* 兰溪变要求GOOSE中断点呼唤灯
         * 没有定义变量pHdlAlert
         * 本程序不能兼容兰溪变工程(EDP02平台)
         */
        pHdlAlert = (ALERT_HDL *)ER_RegAlertSignal();

        iHdlCfgSubGoNum_g=Get_Goose_Sub_Num();

        /* SUB数到网络数 */
        for (k = 0; (k<iHdlCfgSubGoNum_g) && (k<SUB_GOOSE_GRP_MAX_NUM); k++)
        {
            g_bSubRepairDiffer[k] = FALSE;
            for (j = 0; j<iHdlNetNum_g; j++)
            {
                aucHdlAllSubGooseStat[k][j] = 0;
                aucHdlLinkCtlAllSubGooseStat[k][j] = 0;  /* 压板投入时更新 */

                /* GOOSE状态初始化,由应用获取 */
                HDL_GOOSE_SUB_COMM_STATUS[k][j] = TRUE;
            }
        }
    }

    bDataIsValid = TRUE;

    /* 对每组SUB GOOSE进行数据有效状态查询
     */
    for (k = 0; (k<iHdlSubGoNum_g) && (k<SUB_GOOSE_GRP_MAX_NUM); k++)
    {
        iValidNetCnt = 0;

        /* iValidNetCnt仅用于读取无效时的报错
         */
        if (ReadSubStat(aiHdlSubGoIdx_g[k], &iValidNetCnt, aucAllNetSubStat, aucAllNetSubStatOrigin))
        {
            bCurSubDataIsValid = FALSE;

            /* 针对每一个网络进行查询 */
            for (j = 0; j<iHdlNetNum_g; j++)
            {

                /*维护带压板控制的SUB状态  2013-6-26 ZY*/
                switch(aucAllNetSubStat[j])
                {
                    case SUB_IED_OK:

                        if ((aucHdlLinkCtlAllSubGooseStat[k][j] != SUB_IED_OK)
                                && bSubGoYabanStat[aiHdlSubGoIdx_g[k]])
                        {
                            /* 恢复,压板投入 */

                            /* GcRef有效 */
                            assert (sGcRef_g = QuerySubGcRefByIdx(aiHdlSubGoIdx_g[k]));

                            unErrCode = Get_ErrNo_from_NetNo(j);

                            if(aucHdlLinkCtlAllSubGooseStat[k][j]==SUB_IED_CONFREV_ERR)
                            {
                                if (ENG_MODE == 0)
                                {
                                    sprintf(aucPrompt_g, "GOOSE(%s)接收数据版本或类型错误返回 \'%d\' \'%d\'!\n",
                                            sGcRef_g, aiHdlSubGoIdx_g[k], j);
                                    ER_Set_Err_Stat(EV_GSE_CONFI_ERR, ER_REPORT, aucPrompt_g, 0, 0, 0, k*iHdlNetNum_g+j);
                                }
                            }
                            else
                            {
                                if (ENG_MODE == 0)
                                {
                                    sprintf(aucPrompt_g, "GOOSE(%s)通信恢复 \'%d\' \'%d\'!\n",
                                            sGcRef_g, aiHdlSubGoIdx_g[k], j);
                                    ER_Set_Err_Stat(unErrCode, ER_ALERT | ER_REPORT, aucPrompt_g, 0, 0, 0, k);
                                }
                            }

                            aucHdlLinkCtlAllSubGooseStat[k][j] = SUB_IED_OK;
                        }
                        bCurSubDataIsValid = TRUE;
                        break;
                    case SUB_IED_OVERTIME:
                    case SUB_IED_COM_ERR:
                        if ((aucHdlLinkCtlAllSubGooseStat[k][j] != SUB_IED_COM_ERR)
                                && bSubGoYabanStat[aiHdlSubGoIdx_g[k]])
                        {
                            /* 压板投入 */

                            assert (sGcRef_g = QuerySubGcRefByIdx(aiHdlSubGoIdx_g[k]));
                            unErrCode = Get_ErrNo_from_NetNo(j);
                            if (ENG_MODE == 0)
                            {
                                sprintf(aucPrompt_g, "GOOSE(%s)通信中断 \'%d\' \'%d\'!\n",
                                        sGcRef_g, aiHdlSubGoIdx_g[k], j);
                                ER_Set_Err_Stat(unErrCode, ER_ALERT | ER_REPORT, aucPrompt_g, 0, 0, 1, k);
                            }

                            aucHdlLinkCtlAllSubGooseStat[k][j] = SUB_IED_COM_ERR;
                        }
                        break;

                    case SUB_IED_FAULT:
                        if ((aucHdlLinkCtlAllSubGooseStat[k][j] != SUB_IED_FAULT)
                                && bSubGoYabanStat[aiHdlSubGoIdx_g[k]])
                        {
                            /* 压板投入 */

                            assert (sGcRef_g = QuerySubGcRefByIdx(aiHdlSubGoIdx_g[k]));
                            /*unErrCode = Get_ErrNo_from_NetNo(j);*/
                            if (ENG_MODE == 0)
                            {
                                sprintf(aucPrompt_g, "GOOSE(%s)初始化异常 \'%d\' \'%d\'!\n",
                                        sGcRef_g, aiHdlSubGoIdx_g[k], j);
                                ER_Set_Err_Stat(EV_GSE_CONFI_ERR, ER_ALERT | ER_REPORT, aucPrompt_g, 0, 0, 1, k*iHdlNetNum_g+j);
                            }

                            aucHdlLinkCtlAllSubGooseStat[k][j] = SUB_IED_FAULT;
                        }
                        break;

                    case SUB_IED_IN_REPAIR:
                        if ((aucHdlLinkCtlAllSubGooseStat[k][j] != SUB_IED_IN_REPAIR)
                                && bSubGoYabanStat[aiHdlSubGoIdx_g[k]])
                        {
                            /* 压板投入 */

                            assert (sGcRef_g = QuerySubGcRefByIdx(aiHdlSubGoIdx_g[k]));
                            unErrCode = Get_ErrNo_from_NetNo(j);

                            if (ENG_MODE == 0)
                            {
                                sprintf(aucPrompt_g, "GOOSE(%s)检修态 \'%d\' \'%d\'!\n",
                                        sGcRef_g, aiHdlSubGoIdx_g[k], j);
                                ER_Set_Err_Stat(EV_GSE_CONFI_ERR, ER_REPORT, aucPrompt_g, 0, 0, 1, k*iHdlNetNum_g+j);
                            }

                            aucHdlLinkCtlAllSubGooseStat[k][j] = SUB_IED_IN_REPAIR;
                        }
                        break;

                    case SUB_IED_CONFREV_ERR:
                        if ((aucHdlLinkCtlAllSubGooseStat[k][j] != SUB_IED_CONFREV_ERR)
                                && bSubGoYabanStat[aiHdlSubGoIdx_g[k]])
                        {
                            /* 压板投入 */

                            assert (sGcRef_g = QuerySubGcRefByIdx(aiHdlSubGoIdx_g[k]));

                            unErrCode = Get_ErrNo_from_NetNo(j);
                            if ((aucHdlLinkCtlAllSubGooseStat[k][j] == SUB_IED_OVERTIME)
                                    || (aucHdlLinkCtlAllSubGooseStat[k][j] == SUB_IED_COM_ERR))
                            {
                                if (ENG_MODE == 0)
                                {
                                    sprintf(aucPrompt_g, "GOOSE(%s)通信恢复 \'%d\' \'%d\'!\n",
                                            sGcRef_g, aiHdlSubGoIdx_g[k], j);
                                    ER_Set_Err_Stat(unErrCode, ER_ALERT | ER_REPORT, aucPrompt_g, 0, 0, 0, k);
                                }
                            }

                            if (ENG_MODE == 0)
                            {
                                sprintf(aucPrompt_g, "GOOSE(%s)接收数据版本或类型错误 \'%d\' \'%d\'!\n",
                                        sGcRef_g, aiHdlSubGoIdx_g[k], j);
                                ER_Set_Err_Stat(EV_GSE_CONFI_ERR, ER_REPORT, aucPrompt_g, 0, 0, 1, k*iHdlNetNum_g+j);
                            }

                            aucHdlLinkCtlAllSubGooseStat[k][j] = SUB_IED_CONFREV_ERR;
                        }
                        break;

#if 0
                    case SUB_IED_YABAN_EXIT:
                        if ((aucHdlLinkCtlAllSubGooseStat[k][j] != SUB_IED_YABAN_EXIT)
                                && bSubGoYabanStat[aiHdlSubGoIdx_g[k]])
                        {
                            /* 压板投入 */

                            assert (sGcRef_g = QuerySubGcRefByIdx(aiHdlSubGoIdx_g[k]));
                            unErrCode = Get_ErrNo_from_NetNo(j);

                            if (ENG_MODE == 0)
                            {
                                sprintf(aucPrompt_g, "GOOSE(%s)接收压板退出 \'%d\' \'%d\'!\n",
                                        sGcRef_g, aiHdlSubGoIdx_g[k], j);
                                LOG_Write(LOG_KERNEL, aucPrompt_g,NULL );
                            }

                            aucHdlLinkCtlAllSubGooseStat[k][j] = SUB_IED_YABAN_EXIT;
                        }
                        bCurSubDataIsValid = TRUE;
                        break;
#endif

                    default:

                        break;

                } /*switch结束 */

                if(aucAllNetSubStatOrigin[j] != aucHdlAllSubGooseStat[k][j])
                {
                    /* 使用不带压板控制的SUB原始通讯状态 */
                    bGooseDiNeedRefresh = TRUE;
                }

                /* 如果压板退出则返回 */
                if (!bSubGoYabanStat[aiHdlSubGoIdx_g[k]])
                {
                    bCurSubDataIsValid = TRUE;
                    if (aucHdlLinkCtlAllSubGooseStat[k][j] == SUB_IED_COM_ERR)
                    {
                        assert (sGcRef_g = QuerySubGcRefByIdx(aiHdlSubGoIdx_g[k]));

                        unErrCode = Get_ErrNo_from_NetNo(j);

                        sprintf(aucPrompt_g, "GOOSE(%s)通信恢复 \'%d\' \'%d\'!\n",
                                sGcRef_g, aiHdlSubGoIdx_g[k], j);

                        ER_Set_Err_Stat(unErrCode, ER_ALERT | ER_REPORT, aucPrompt_g, 0, 0, 0, k);

                        aucHdlLinkCtlAllSubGooseStat[k][j] = SUB_IED_OK;
                        bGooseDiNeedRefresh = TRUE;

                    }
                }

                /*维护不带压板控制的SUB原始通信状态（不包含压板退出状态）  2013-6-27 ZY*/
                switch(aucAllNetSubStatOrigin[j])
                {
                    case SUB_IED_OK:
                        aucHdlAllSubGooseStat[k][j] = SUB_IED_OK;
                        break;

                    case SUB_IED_OVERTIME:
                    case SUB_IED_COM_ERR:
                        aucHdlAllSubGooseStat[k][j] = SUB_IED_COM_ERR;
                        break;

                    case SUB_IED_FAULT:
                        aucHdlAllSubGooseStat[k][j] = SUB_IED_FAULT;
                        break;

                    case SUB_IED_IN_REPAIR:
                        aucHdlAllSubGooseStat[k][j] = SUB_IED_IN_REPAIR;
                        break;

                    case SUB_IED_CONFREV_ERR:
                        aucHdlAllSubGooseStat[k][j] = SUB_IED_CONFREV_ERR;
                        break;

                    default:
                        aucHdlAllSubGooseStat[k][j] = aucAllNetSubStatOrigin[j];
                        break;

                } /*switch结束 */


            }/*for(j=0结束 */

            /*检修不一致判断,主变不需要*/
            if(appType_g != APP_TRANS)
            {
                if(GetSubTestMode(aiHdlSubGoIdx_g[k]) != ((uiEdpStatus_g & ON_EXAM_STATE)?TRUE:FALSE))
                {
                    if (g_bSubRepairDiffer[k] == FALSE)
                    {
                        assert (sGcRef_g = QuerySubGcRefByIdx(aiHdlSubGoIdx_g[k]));
                        for (j = 0; j<iHdlNetNum_g; j++)
                        {
                            /* 遍历所有端口 */
                            if (Get_Goose_Sub_Net_Cfg(aiHdlSubGoIdx_g[k]-1,j))
                            {
                                /* 通讯状态不是初始值 可以认为存在 */
                                g_bSubRepairDiffer[k] = TRUE;
                                bGooseDiNeedRefresh = TRUE;
#if 0
                                if (ENG_MODE == 0)
                                {
                                    sprintf(aucPrompt_g, "GOOSE(%s)检修不一致 \'%d\' \'%d\'!\n",
                                            sGcRef_g, aiHdlSubGoIdx_g[k], j);
                                    ER_Set_Err_Stat(EV_GSE_CONFI_ERR, ER_REPORT, aucPrompt_g, 0, 0, 1, k);
                                }
#endif
                            }
                        }
                    }
                }
                else if(GetSubTestMode(aiHdlSubGoIdx_g[k]) == ((uiEdpStatus_g & ON_EXAM_STATE)?TRUE:FALSE) && g_bSubRepairDiffer[k] == TRUE)
                {
                    g_bSubRepairDiffer[k] = FALSE;
                    bGooseDiNeedRefresh = TRUE;
                    assert (sGcRef_g = QuerySubGcRefByIdx(aiHdlSubGoIdx_g[k]));
                    for (j = 0; j<iHdlNetNum_g; j++)
                    {
                        /* 遍历所有端口 */
                        if (Get_Goose_Sub_Net_Cfg(aiHdlSubGoIdx_g[k]-1,j))
                        {
                            /* 通讯状态不是初始值 可以认为存在 */
#if 0
                            if (ENG_MODE == 0)
                            {
                                sprintf(aucPrompt_g, "GOOSE(%s)检修不一致返回 \'%d\' \'%d\'!\n",
                                        sGcRef_g, aiHdlSubGoIdx_g[k], j);
                                ER_Set_Err_Stat(EV_GSE_CONFI_ERR, ER_REPORT, aucPrompt_g, 0, 0, 0, k);
                            }
#endif
                        }
                    }
                }
            }

            /* 按SUB序号填写SUB状态,便于GOOSE DI句柄查询 */
            if(bCurSubDataIsValid)
            {
                iHdlSubGoStat[aiHdlSubGoIdx_g[k]]=1;
            }
            else
            {
                if(iHdlSubGoStat[aiHdlSubGoIdx_g[k]]!=-1)
                {
                    iHdlSubGoStat[aiHdlSubGoIdx_g[k]]=0;
                }
            }

            if (!bCurSubDataIsValid)
            {
                /* 非检修态 */
                if (GetSubTestMode(aiHdlSubGoIdx_g[k]) != TRUE)
                    bDataIsValid = FALSE;
            }
        }/*if (ReadSubStat结束 */
        else  /* 若读取状态失败,则数据无效 */
        {
            for (j = 0; j<iValidNetCnt; j++)
            {
                unErrCode = Get_ErrNo_from_NetNo(j);

                /* 暂不处理压板退出时的返回 */
                if (bSubGoYabanStat[aiHdlSubGoIdx_g[k]])
                {
                    if (ENG_MODE == 0)
                    {
                        sprintf(aucPrompt_g, "GOOSE(%s)数据无效 \'%d\' \'%d\'!\n",
                                sGcRef_g, aiHdlSubGoIdx_g[k], j);
                        ER_Set_Err_Stat(unErrCode, ER_ALERT | ER_REPORT, aucPrompt_g, 0, 0, 1, k);
                    }
                }
            }
            bDataIsValid = FALSE;

            /* The state of the current sub. */
            iHdlSubGoStat[aiHdlSubGoIdx_g[k]] = 0;
        }
    }/*for(k=0结束  */

    Trans_Goose_Comm_Status();

    /* 发出呼唤信号 */
    if (bDataIsValid != bLastDataIsValid)
    {
        if (bDataIsValid)
        {
            ER_ClearAlertSignal(pHdlAlert);
            LOG_Write(LOG_INFO, "GOOSE异常呼唤返回\n", NULL);
        }
    }
    if (!bDataIsValid)
    {
        ER_SetAlertSignal(pHdlAlert);

        if(bLastDataIsValid)
        {
            LOG_Write(LOG_INFO, "GOOSE异常呼唤动作\n", NULL);
        }
    }

    if(bGooseDiNeedRefresh)
    {
        HDL_SetGooseDiNeedRefresh(TRUE);
    }

    bLastDataIsValid = bDataIsValid;

    return bDataIsValid;
}



/*查询智能操作箱数据发布时，数据是否发生变化,同时保存此次PUB值
  参数  无
  返回,TRUE,数据发生变化
       FALSE，数据未变化  2007-6-25日 张云 */
BOOL  HDL_DataPubIsChgAndSave()
{
    int  i,j;
    BOOL  bHdlDataIsChg=FALSE;
    static   BOOL  bHdlIsFstPub=TRUE;
    float fAoCurVal;
    float fAoLstPubVal;
    US_CNT_UTC_TIME uttmNow;
    US_CNT_UTC_TIME uttmFromDi;
    US_CNT_UTC_TIME uttmTmp;
    int iCurVal;
    BOOL bRtYabanValue;

    uttmFromDi.ucQflag=0x60;
    uttmFromDi.ullusCntFrom1970=0;

    if(bHdlIsFstPub)
    {
        /*若是首次发布  */
        bHdlIsFstPub=FALSE;
        bHdlDataIsChg=TRUE;
        TM_High_Get_Sys_Us_UTC_Time(&uttmNow, NULL);/*2013-5-23 ZY  */

        for(i=0; i<HdlBoxIOInfo_g.iHdlDoNum; i++)
        {
            for(j=0; j<MAX_TIME_SOURCE_DI_NUM; j++)
            {
                if(HdlBoxIOInfo_g.aphHdlDoIdx[i]->iTimeSrcDiIndex[j]>0)
                {
                    uttmTmp = RD_GetUTDiChgTimeByDiIndex(HdlBoxIOInfo_g.aphHdlDoIdx[i]->iTimeSrcDiIndex[j]);
                    if(uttmTmp.ullusCntFrom1970>uttmFromDi.ullusCntFrom1970)
                    {
                        uttmFromDi=uttmTmp;
                    }
                }
                else
                    break;
            }

            /*if((uttmFromDi.ucQflag&0x60)==0)*/
            if(uttmFromDi.ullusCntFrom1970>0)/*时标无效也取开入变位时间*/
                HdlBoxIOInfo_g.aphHdlDoIdx[i]->utChgTime=uttmFromDi;
            else
                HdlBoxIOInfo_g.aphHdlDoIdx[i]->utChgTime=uttmNow;
        }
    }

    for(i=0; i<HdlBoxIOInfo_g.iHdlDoNum; i++)
    {
        /*查询PUB时，DO数据是否发生变化  */
        if (HdlBoxIOInfo_g.aphHdlDoIdx[i]->linkNum == -1)
        {
            /* 不关联压板 */
            iCurVal = HdlBoxIOInfo_g.aphHdlDoIdx[i]->iCurVal;
        }
        else
        {
            /* 关联压板 */
            SCI_Get_Yaban_Value(HdlBoxIOInfo_g.aphHdlDoIdx[i]->linkNum, &bRtYabanValue, 0);

            if (bRtYabanValue)
            {
                /* 压板投入取实际值 */
                iCurVal = HdlBoxIOInfo_g.aphHdlDoIdx[i]->iCurVal;
            }
            else
            {
                /* 压板退出根据类型判断 */
                if (HdlBoxIOInfo_g.aphHdlDoIdx[i]->vtValType == BOOL_TYPE)
                {
                    iCurVal = FALSE;
                }
                else
                {
                    iCurVal = DP_FALSE; /* 双点信息 */
                }
            }
        }

        if (iCurVal != HdlBoxIOInfo_g.aphHdlDoIdx[i]->iLstPubVal)
        {
            uttmFromDi.ucQflag=0x60;
            uttmFromDi.ullusCntFrom1970=0;
            /* 获取本次变位绝对时间 */
            if (!bHdlDataIsChg)
            {
                bHdlDataIsChg=TRUE;
                TM_High_Get_Sys_Us_UTC_Time(&uttmNow, NULL);/*2013-5-23 ZY  */
            }

            for(j=0; j<MAX_TIME_SOURCE_DI_NUM; j++)
            {
                if(HdlBoxIOInfo_g.aphHdlDoIdx[i]->iTimeSrcDiIndex[j]>0)
                {
                    uttmTmp = RD_GetUTDiChgTimeByDiIndex(HdlBoxIOInfo_g.aphHdlDoIdx[i]->iTimeSrcDiIndex[j]);
                    if(uttmTmp.ullusCntFrom1970>uttmFromDi.ullusCntFrom1970)
                    {
                        uttmFromDi=uttmTmp;
                    }
                }
                else
                    break;
            }

            //LOG_Dbg_Msg("j=%d, Time from DI Q=%X, Us=%lu\n",j,uttmFromDi.ucQflag,uttmFromDi.ullusCntFrom1970,0,0,0);

            /*if((uttmFromDi.ucQflag&0x60)==0)*/
            if(uttmFromDi.ullusCntFrom1970>0)/*时标无效也取开入变位时间*/
                HdlBoxIOInfo_g.aphHdlDoIdx[i]->utChgTime=uttmFromDi;
            else
                HdlBoxIOInfo_g.aphHdlDoIdx[i]->utChgTime=uttmNow;

            HdlBoxIOInfo_g.aphHdlDoIdx[i]->iLstPubVal = iCurVal;/*保存此次PUB值  */
        }
    }

    for(i=0; i<HdlBoxAoCfg_g.iHDLAONum; i++)
    {
        /*查询PUB时，AO数据是否发生变化  */
        assert(HdlBoxAoCfg_g.apHdlBoxAoIdx[i]->pElemSrc);
        fAoCurVal=((EP_ELEM_IO *)HdlBoxAoCfg_g.apHdlBoxAoIdx[i]->pElemSrc)->now.fVal;
        fAoLstPubVal=HdlBoxAoCfg_g.apHdlBoxAoIdx[i]->fLstPubVal;
        if(uiAppType_g == APP_INTEL_BOX)
        {
            if(fabs(fAoCurVal-fAoLstPubVal)>HDL_AO_DATA_PUB_CHG_TH_INTELBOX)/*若超过允许的门槛  */
            {
                bHdlDataIsChg=TRUE;
                HdlBoxAoCfg_g.apHdlBoxAoIdx[i]->fLstPubVal=fAoCurVal;
            }
        }
        else
        {
            if(fabs(fAoCurVal-fAoLstPubVal)>HDL_AO_DATA_PUB_CHG_TH)/*若超过允许的门槛  */
            {
                bHdlDataIsChg=TRUE;
                HdlBoxAoCfg_g.apHdlBoxAoIdx[i]->fLstPubVal=fAoCurVal;
            }
        }
    }

    return  bHdlDataIsChg;
}

/* 功能：获得某AO通道输出的数据
   参数：
        uiAONum：		在该数据源active goose的AO序号，序号从1开始，
        pfRtData:               返回的该AO通道的数据
   返回：
        TRUE； 若有匹配数据，则操作成功
        FALSE：操作失败
  	*/

BOOL  HDL_GetActiveGoAoData(uint16_t uiAONum,
                            float *pfRtData)
{
    EP_ELEM_IO *  pElemIO;

    if(uiAONum>HdlBoxAoCfg_g.iHDLAONum||uiAONum<1)
    {
        static   uint32_t  ulTestCnt_s=0;
        ulTestCnt_s++;
        if((ulTestCnt_s&0x3ffff)==1)
        {
            if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
            {
                ER_Set_Err(EV_GSE_CONFI_ERR,
                           ER_REPORT|ER_ALARM,
                           "Goose config err:active  goose get same HDL box AO val，AO num out of range,AO num is %d!\n",
                           uiAONum, 0);

            }
            else if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_GSE_CONFI_ERR,
                           ER_REPORT|ER_ALARM,
                           "Goose 配置错误:active  goose获得智能操作箱机箱AO值时，AO序号越界,AO序号是%d!\n",
                           uiAONum, 0);
            }
        }

        *pfRtData=0.0;
        return   FALSE;
    }
    pElemIO=(EP_ELEM_IO *)HdlBoxAoCfg_g.apHdlBoxAoIdx[uiAONum-1]->pElemSrc;
    assert(pElemIO);
    *pfRtData=pElemIO->now.fVal;

    return   TRUE;
}

BOOL **Trans_Goose_Comm_Status()
{
    int i,j;
    static BOOL bFirstTime=TRUE;
    BOOL bChgGoStsFile=FALSE;

    if(bFirstTime)
    {
        /*printf("First enter Trans_Goose_Comm_Status subnum=%d\n",iHdlSubGoNum_g);*/
        for (i = 0; i<iHdlSubGoNum_g; i++)
        {
            for(j=0; j<iHdlNetNum_g; j++)
            {
                /*printf("goose stat:%u sub%d,net%d\n",aucHdlAllSubGooseStat[i][j],i,j);*/
                if(aucHdlAllSubGooseStat[i][j]==0)
                {
                    HDL_GOOSE_SUB_NET_CFG[aiHdlSubGoIdx_g[i]-1][j]=FALSE;
                }
                else
                {
                    HDL_GOOSE_SUB_NET_CFG[aiHdlSubGoIdx_g[i]-1][j]=TRUE;
                }
            }
        }
        bFirstTime=FALSE;
    }

    for (i = 0; i<iHdlSubGoNum_g; i++)
    {
        for(j=0; j<iHdlNetNum_g; j++)
        {
            /*经过压板处理的通信状态处理 */
            if((aucHdlLinkCtlAllSubGooseStat[i][j]==SUB_IED_OVERTIME)
                    ||(aucHdlLinkCtlAllSubGooseStat[i][j]==SUB_IED_FAULT)
                    ||(aucHdlLinkCtlAllSubGooseStat[i][j]==SUB_IED_COM_ERR)
                    ||(aucHdlLinkCtlAllSubGooseStat[i][j]==SUB_IED_CONFREV_ERR))
            {
                if(HDL_GOOSE_SUB_COMM_STATUS[aiHdlSubGoIdx_g[i]-1][j]==TRUE)
                {
                    HDL_GOOSE_SUB_COMM_STATUS[aiHdlSubGoIdx_g[i]-1][j]=FALSE;
                    bChgGoStsFile=TRUE;
                }
            }
            else
            {
                if(HDL_GOOSE_SUB_COMM_STATUS[aiHdlSubGoIdx_g[i]-1][j]==FALSE)
                {
                    HDL_GOOSE_SUB_COMM_STATUS[aiHdlSubGoIdx_g[i]-1][j]=TRUE;
                    bChgGoStsFile=TRUE;
                }
            }



            /*不经过压板处理的实际通信状态处理，供虚端子状态显示，2013-6-26  ZY */
            if((aucHdlAllSubGooseStat[i][j]==SUB_IED_OVERTIME)
                    ||(aucHdlAllSubGooseStat[i][j]==SUB_IED_FAULT)
                    ||(aucHdlAllSubGooseStat[i][j]==SUB_IED_COM_ERR)
                    ||(aucHdlAllSubGooseStat[i][j]==SUB_IED_CONFREV_ERR))
            {
                if(HDL_GOOSE_SUB_REAL_COMM_STATUS[aiHdlSubGoIdx_g[i]-1][j]==TRUE)
                {
                    HDL_GOOSE_SUB_REAL_COMM_STATUS[aiHdlSubGoIdx_g[i]-1][j]=FALSE;
                }
            }
            else
            {
                if(HDL_GOOSE_SUB_REAL_COMM_STATUS[aiHdlSubGoIdx_g[i]-1][j]==FALSE)
                {
                    HDL_GOOSE_SUB_REAL_COMM_STATUS[aiHdlSubGoIdx_g[i]-1][j]=TRUE;
                }
            }
        }/*for(j=0结束 */
    }/*for(i=0结束 */

    if(bChgGoStsFile)
    {
        Smv_Go_CommStat_Chg();/*触发状态文件更新*/
    }

    return (BOOL **)&HDL_GOOSE_SUB_COMM_STATUS[0][0];
}


BOOL Get_Goose_SubNum_and_NetNum(int *SubNum,int *MaxNetNum)
{
    *SubNum=iHdlCfgSubGoNum_g;
    *MaxNetNum=iHdlNetNum_g;
    return TRUE;
}

BOOL Get_Goose_Comm_Status(int SubNo,int NetNo)
{
    assert((SubNo<iHdlCfgSubGoNum_g)&&(NetNo<iHdlNetNum_g));
    return HDL_GOOSE_SUB_COMM_STATUS[SubNo][NetNo];
}

BOOL Get_Goose_Sub_Net_Cfg(int SubNo,int NetNo)
{
    assert((SubNo<iHdlCfgSubGoNum_g)&&(NetNo<iHdlNetNum_g));
    return HDL_GOOSE_SUB_NET_CFG[SubNo][NetNo];
}


/*功能：获得不经压板处理的GOOSE的SUB通信状态，2013-6-26  ZY*/
BOOL Get_Goose_Real_Comm_Status(int SubNo,int NetNo)
{
    assert((SubNo<iHdlCfgSubGoNum_g)&&(NetNo<iHdlNetNum_g));
    return HDL_GOOSE_SUB_REAL_COMM_STATUS[SubNo][NetNo];
}


/*功能：得到CPU虚拟开入虚端子配置信息  2013-6-5  ZY
  参数：ppRtTotalCfgAddr：供返回DI虚端子的总体配置信息全局变量地址对应的变量的指针。
                      该全局变量地址对应的变量由调用方分配，被调用方填充。
                      该全局变量自身，由被调用方分配和管理
  返回：EP_SUCCESS:操作成功
       其他，操作失败 */
EP_STATUS  HDL_Get_Vt_DI_Term_Cfg(HDL_TOTAL_VT_DI_TERM_CFG   **ppRtTotalCfgAddr)
{
    RD_LGC_DI_CH *plgcdi;
    HDL_DI_HND  *pHdl;
    HDL_VT_DI_TERM_CFG  *pTermCfg;
    int  i;
    static BOOL bSetDiVtCfg = TRUE;

    assert(ppRtTotalCfgAddr);

    *ppRtTotalCfgAddr=&HDL_TotalVtDITermCfg_g;
    HDL_TotalVtDITermCfg_g.iTermCnt=0;
    for (plgcdi=plgcdich_g; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++)
    {
        if (plgcdi->mod==RD_HDL_BOX_DI)
        {
            pHdl=(HDL_DI_HND  *)(plgcdi->pvSrc);
            if(pHdl)
            {
                for(i=0; i<HDL_DI_MAX_RECV_NUM; i++)
                {
                    pTermCfg=HDL_TotalVtDITermCfg_g.aTermCfgArr
                             +HDL_TotalVtDITermCfg_g.iTermCnt;
                    if(HDL_TotalVtDITermCfg_g.iTermCnt>=MAX_DI_VT_TERM_NUM)
                    {
                        break;
                    }
                    if((pHdl->iSubDaIdx[i]>0)&&(pHdl->ppSubMapData[i]))
                    {
                        /*只有虚端子关联的，才计入，若有虚端子关联，考虑开入关联多个虚端子 */

                        if(HDL_Get_One_Vt_DI_Term_Cfg(pTermCfg,
                                                      pHdl,pHdl->iSubNum[i],pHdl->ppSubMapData[i]))
                        {
                            HDL_TotalVtDITermCfg_g.iTermCnt++;

                            if (bSetDiVtCfg)
                            {
                                /* 硬开入和虚端子关联 */
                                plgcdi->pVtPortCfg[plgcdi->ucVtPortNum] = (void *)pTermCfg;
                                plgcdi->ucVtPortNum++;
                            }
                        }

                    }/* 	if((pHdl->iSubDaIdx[0]>=0)&&(pHdl->ppSubMapData[0]))结束*/
                    else
                    {
                        break;
                    }
                }/*for(i=0)结束 */
            }/*if(pHdl)结束 */

        }/*if (plgcdi->mod==RD_HDL_BOX_DI)结束 */

    }/*for(plgcdi=plgcdich_g结束 */

    bSetDiVtCfg = FALSE;

    return  EP_SUCCESS;
}

/*功能：获得某虚端子配置信息，2013-7-6 ZY
  参数：pTermCfg，虚端子配置信息变量指针
                调用方分配，被调用方填充
       pHdl，该虚端子关联的通道句柄
       iSubNum，该虚端子关联的SUB号
       pSubMapData，该虚端子关联的SUBMAP信息指针
  返回：成功与否
   */
BOOL  HDL_Get_One_Vt_DI_Term_Cfg(HDL_VT_DI_TERM_CFG  *pTermCfg,
                                 void  *pHdl,
                                 int  iSubNum,
                                 void *pSubMapData)
{
    SUB_MAP_INFO  *pSubMapInf;
    GSE_SUB_INFO  *pSubInf;
    SUB_USER_INFO  *pSubUsrInf;
    int  i;
    int  k;
    BOOL   bYabanIsFind;
    const SC_LINK_ITEM *  pLink;

    pTermCfg->pHdl=pHdl;
    pTermCfg->iSubNum=iSubNum;
    pTermCfg->pSubMapData=pSubMapData;

    pSubMapInf=(SUB_MAP_INFO  *)pSubMapData;
    if(pSubMapInf->type==BOOL_TYPE)
    {
        pTermCfg->iValType=1;
    }
    else  if(pSubMapInf->type==DPC_TYPE)
    {
        pTermCfg->iValType=2;
    }
    else
    {
        return  FALSE;
    }
    pSubInf=(GSE_SUB_INFO  *)(pSubMapInf->pInfoNode);
    pSubUsrInf=(SUB_USER_INFO  *)(&(pSubInf->UserInfo));
    pTermCfg->uiDataSetAppID=pSubUsrInf->appID;

    /*查询获得压板名称，而不是ID */
    bYabanIsFind=FALSE;
    for(k=0; k<iLinkNum_g; k++)
    {
        pLink=SC_Get_Link_Attr(k);
        if(!(strcmp(pSubUsrInf->ybLogicID,pLink->aucId)))
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
            break;
        }
    }
    if(!bYabanIsFind)
    {
        pTermCfg->aucYabanIDStr[0]='\0';
    }

    /*虚端子描述 */
    if(strlen(pSubMapInf->desc)>64)
    {
        strncpy(pTermCfg->aucDescStr,pSubMapInf->desc,64);
        pTermCfg->aucDescStr[64]='\0';
    }
    else
    {
        strcpy(pTermCfg->aucDescStr,pSubMapInf->desc);
    }

    for(i=0; i<iHdlSubGoNum_g; i++)
    {
        if(iSubNum==aiHdlSubGoIdx_g[i])
        {
            pTermCfg->iSubGoIdxSeqNo=i;
            break;
        }
    }
    if(i>=iHdlSubGoNum_g)
    {
        pTermCfg->iSubGoIdxSeqNo=0;
        return  FALSE;
    }

    return  TRUE;

}


/*功能：得到CPU开入虚端子状态信息  2013-6-5 ZY
  参数：pRtTotalSTS：供返回DI虚端子的总体状态信息变量指针。
                  该变量，由调用方分配，被调用方填充
  返回：EP_SUCCESS:操作成功
       其他，操作失败 */
EP_STATUS  HDL_Get_Vt_DI_Term_Sts(HDL_TOTAL_VT_DI_TERM_STS   *pRtTotalSts)
{
    int  i;
    HDL_VT_DI_TERM_STS  *pTermSts;
    HDL_VT_DI_TERM_CFG  *pTermCfg;
    BOOL  bGetValSuccess;

    assert(pRtTotalSts);
    pRtTotalSts->iTermCnt=HDL_TotalVtDITermCfg_g.iTermCnt;
    for(i=0; i<pRtTotalSts->iTermCnt; i++)
    {
        pTermCfg=HDL_TotalVtDITermCfg_g.aTermCfgArr+i;
        pTermSts=pRtTotalSts->aTermStsArr+i;

        bGetValSuccess=HDL_GetGoVtTermOrgVal
                       (pTermCfg,&(pTermSts->ucTermVal));
        if(!bGetValSuccess)
        {
            /*若访问异常 */
            if(pTermCfg->iValType==1)
            {
                /*单点,设为开 */
                pTermSts->ucTermVal=0;
            }
            else  if(pTermCfg->iValType==2)
            {
                /*双点，设为无效 */
                pTermSts->ucTermVal=3;
            }
        }

        HDL_GetGoVtTermSts(pTermCfg
                           ,&(pTermSts->ucTermQuality)
                           ,pTermCfg->iSubGoIdxSeqNo);

    }

    return  EP_SUCCESS;
}


/* 根据通道句柄，获得相应的虚端子原始值,2013-6-6 ZY
   参数，pTermCfg,虚端子配置信息指针
       pucRtVal， 供返回的虚端子的原始值变量的指针。
                  该变量，由调用方分配，被调用方填充
   返回：TRUE，成功
         FALSE，失败 */
BOOL   HDL_GetGoVtTermOrgVal(HDL_VT_DI_TERM_CFG  *pTermCfg,uint8_t *pucRtOrgVal)
{

    BOOL  bRdSuccess;
    GOOSE_DA_VALUE  RdDaVal;
    int  iDaStat;
    BOOL  bSuccess;

    assert(pTermCfg);
    assert(pucRtOrgVal);

    bSuccess=TRUE;

    bRdSuccess = ReadSubDaValueBySubMapData(pTermCfg->pSubMapData, &RdDaVal, &iDaStat);

    if(bRdSuccess)
    {
        if(RdDaVal.nDaType==DI_TYPE_VALUE&&RdDaVal.da_val.di_val.flag==SINGLE_TYPE_DI)
        {
            /*获得单点DI信息  */
            if((RdDaVal.da_val.di_val.cls==1)
                    ||(RdDaVal.da_val.di_val.cls==0))
            {
                *pucRtOrgVal=RdDaVal.da_val.di_val.cls;
            }
            else
            {
                /* 修改逻辑为非0即1 */
                *pucRtOrgVal=1;
            }
        }
        else  if(RdDaVal.nDaType==DI_TYPE_VALUE&&RdDaVal.da_val.di_val.flag==DUAL_TYPE_DI)
        {
            /*获得双点DI信息  */
            if((RdDaVal.da_val.di_val.cls==0)
                    ||(RdDaVal.da_val.di_val.cls==1)
                    ||(RdDaVal.da_val.di_val.cls==2)
                    ||(RdDaVal.da_val.di_val.cls==3))
            {
                *pucRtOrgVal=RdDaVal.da_val.di_val.cls;

            }
            else
            {
                *pucRtOrgVal=3;/*无效态 */
            }
        }
        else
        {
            /*若返回类型不对  */
            bSuccess=FALSE;
        }
    }
    else
    {
        /*若读取失败 */
        bSuccess=FALSE;
    }

    return  bSuccess;
}


/* 根据通道句柄,获得相应的虚端子状态  2013-6-6  ZY
   参数， pTermCfg,虚端子配置信息指针
         pucRtSts，返回虚端子状态变量的指针。
                  该变量，由调用方分配，被调用方填充
         iSubGoIdxSeqNo,关联的SUB对应aiHdlSubGoIdx_g数组中的序号，从0开始
   返回：TRUE，成功
         FALSE，失败 */
BOOL   HDL_GetGoVtTermSts(HDL_VT_DI_TERM_CFG  *pTermCfg,uint8_t *pucRtSts,int  iSubGoIdxSeqNo)
{
#define  DI_TERM_COM_ERR_MASK     0X01  /*中断位 */
#define  DI_TERM_INVALID_MASK       0X02  /*无效位 */
#define  DI_TERM_TEST_MASK         0X08  /*测试位 */
#define  DI_TERM_YB_EXIT_MASK      0X40  /*压板退出位 */
#define  DI_TERM_NO_CONN_MASK     0X80  /*虚端子未关联位 */

    BOOL   bTermIsComErr;
    BOOL   bTermIsInvalid;
    BOOL   bTermIsTest;
    BOOL   bTermIsYbExit;
    uint8_t  ucSts;
    int  iSubCfgCnt;
    int  iNetCfgCnt;
    int  i;
    int  iSubCfgNo;

    assert(pTermCfg);
    assert(pucRtSts);

    ucSts=0;

    /*判定链路中断位*/
    bTermIsComErr=TRUE;
    Get_Goose_SubNum_and_NetNum(&iSubCfgCnt,&iNetCfgCnt);
    for(i=0; i<iNetCfgCnt; i++)
    {
        /*判定DI对应SUB的所有的网口都中断，才认为中断 */
        iSubCfgNo=pTermCfg->iSubNum-1;
        if(Get_Goose_Sub_Net_Cfg(iSubCfgNo,i))
        {
            if(Get_Goose_Real_Comm_Status(iSubCfgNo,i))
            {
                bTermIsComErr=FALSE;
                break;
            }
        }
    }
    if(bTermIsComErr)
    {
        /*只有该SUB关联的网口都中断时，才认为中断 */
        ucSts=ucSts|DI_TERM_COM_ERR_MASK;
    }

    /*判定检修位 */
    if(!bTermIsComErr)
    {
        /*若通信是好的，则判定检修  */
        bTermIsTest= GetSubTestModeByMap((SUB_MAP_INFO *)(pTermCfg->pSubMapData));
        if(bTermIsTest)
        {
            ucSts=ucSts|DI_TERM_TEST_MASK;
        }
    }
    else
    {
        bTermIsTest=FALSE;
    }
    /*判定压板退出位 */
    if(bSubGoYabanStat[pTermCfg->iSubNum])
    {
        /*压板投入 */
        bTermIsYbExit=FALSE;
    }
    else
    {
        ucSts=ucSts|DI_TERM_YB_EXIT_MASK;
        bTermIsYbExit=TRUE;
    }

    /*判定数据有效位，此时需要处理通信中断信息，和忽略压板投退信息 */
    if(!bTermIsComErr)
    {
        /*若通信正常 */
        if(iHdlSubGoStat[pTermCfg->iSubNum]==1)
        {
            bTermIsInvalid=FALSE;
        }
        else
        {
            ucSts=ucSts|DI_TERM_INVALID_MASK;
            bTermIsInvalid=TRUE;
        }
    }
    else
    {
        /*若通信中断 */
        ucSts=ucSts|DI_TERM_INVALID_MASK;
        bTermIsInvalid=TRUE;
    }

    *pucRtSts=ucSts;
    return  TRUE;
}

/* 获取DI所对应虚端子状态.
 * Para:
 *     pvDiHnd, 开入通道.
 *     pucActTermNum, 实际虚端子个数.
 *     pucTermVal, 状态填写缓冲, 由应用传入, 数组长度固定为32字节(宏定义HDL_DI_MAX_RECV_NUM).
 * Return:
 *     NONE.
 * Alert:
 *     开入通道与虚端子的对应关系在GSE.xml文件中体现,
 *     在返回值缓冲区中体现配置的先后顺序。
 */
BOOL HDL_GetDiVtSts(void *pvDiHnd, uint8_t *pucActTermNum, uint8_t *pucTermVal)
{
    RD_LGC_DI_CH *plgcdi = NULL;
    int i;
    HDL_DI_HND *pHdl = NULL;

    if ((pvDiHnd == NULL) || (pucActTermNum == NULL) || (pucTermVal == NULL))
    {
        return FALSE;
    }

    plgcdi = (RD_LGC_DI_CH *)pvDiHnd;
    pHdl = (HDL_DI_HND *)plgcdi->pvSrc;

    *pucActTermNum = plgcdi->ucVtPortNum;

    for (i = 0; i<plgcdi->ucVtPortNum; i++)
    {
        *(pucTermVal+i) = (uint8_t)pHdl->bArrSts[i];
    }

    return TRUE;
}

/* 获取DI所对应虚端子状态及品质.
 * Para:
 *     pvDiHnd, 开入通道.
 *     pucActTermNum, 实际虚端子个数.
 *     pucTermVal, 状态填写缓冲, 由应用传入, 数组长度固定为32字节(宏定义HDL_DI_MAX_RECV_NUM).
 *     pusTermSts, 品质填写缓冲.
 * Return:
 *     NONE.
 * Alert:
 *     开入通道与虚端子的对应关系在GSE.xml文件中体现,
 *     在返回值缓冲区中体现配置的先后顺序。
 */
BOOL HDL_GetDiVtValAndSts(void *pvDiHnd, uint8_t *pucActTermNum, uint8_t *pucTermVal,
                          uint16_t *pusTermSts)
{
    RD_LGC_DI_CH *plgcdi = NULL;
    int i;
    HDL_DI_HND *pHdl = NULL;

    if ((pvDiHnd == NULL) || (pucActTermNum == NULL) || (pucTermVal == NULL))
    {
        return FALSE;
    }

    plgcdi = (RD_LGC_DI_CH *)pvDiHnd;
    pHdl = (HDL_DI_HND *)plgcdi->pvSrc;

    *pucActTermNum = plgcdi->ucVtPortNum;

    for (i = 0; i<plgcdi->ucVtPortNum; i++)
    {
        *(pucTermVal+i) = (uint8_t)pHdl->bArrSts[i];
        *(pusTermSts+i) = pHdl->usArrQuality[i];
    }

    return TRUE;
}

/* 获取DI所对应虚端子状态及品质.
 * Para:
 *     pvDiHnd, 开入通道.
 *     pucActTermNum, 实际虚端子个数.
 *     pucTermVal, 状态填写缓冲, 由应用传入, 数组长度固定为32字节(宏定义HDL_DI_MAX_RECV_NUM).
 *     pusTermSts, 品质填写缓冲.
 *     piSubDaVtIdx, 虚端子序号，从0开始.
 *     pulArrTmpNextCnt, 最近一次变位节拍.
 * Return:
 *     NONE.
 * Alert:
 *     开入通道与虚端子的对应关系在GSE.xml文件中体现,
 *     在返回值缓冲区中体现配置的先后顺序。
 */
BOOL HDL_GetDiVt(void *pvDiHnd, uint8_t *pucActTermNum, uint8_t *pucTermVal,
                 uint16_t *pusTermSts, int *piSubDaVtIdx, uint32_t *pulArrTmpNextCnt)
{
    RD_LGC_DI_CH *plgcdi = NULL;
    int i;
    HDL_DI_HND *pHdl = NULL;

    if ((pvDiHnd == NULL) || (pucActTermNum == NULL) || (pucTermVal == NULL)
            || (piSubDaVtIdx == NULL) || (pulArrTmpNextCnt == NULL))
    {
        return FALSE;
    }

    plgcdi = (RD_LGC_DI_CH *)pvDiHnd;
    pHdl = (HDL_DI_HND *)plgcdi->pvSrc;

    *pucActTermNum = plgcdi->ucVtPortNum;

    for (i = 0; i<plgcdi->ucVtPortNum; i++)
    {
        *(pucTermVal+i) = (uint8_t)pHdl->bArrSts[i];
        *(pusTermSts+i) = pHdl->usArrQuality[i];
        *(piSubDaVtIdx+i) = pHdl->iSubDaVtIdx[i];
        *(pulArrTmpNextCnt+i) = pHdl->ulArrTmpNextCnt[i];
    }

    return TRUE;
}

/* 消抖并返回当前确认DI状态(周期调用).
 * Para:
 *     pvDiCh, 通道句柄.
 *     pulChgTime, 变位时采样节拍.
 *     pusQuality, 品质.
 * Return:
 *     该DI通道的当前状态,TRUE=闭合;FALSE=打开.
 */
BOOL Hdl_Filt_And_Get_DI(void *pvDiCh, uint32_t *ulChgNextCnt, uint16_t *pusQuality)
{
    HDL_DI_HND *pHdl;
    int iNow;
    BOOL bFilterSts;

    if (!pvDiCh)
    {
        return FALSE;
    }

    pHdl = (HDL_DI_HND *)pvDiCh;
    iNow = HDL_Get_DI_Many(pHdl, NULL, pusQuality, &bFilterSts);

    if (iNow != pHdl->bLstSts)
    {
        /* 重置变化通道计数器 */
        pHdl->ulFltCnt = pHdl->ulFltCfg;
        pHdl->bChgFlag = TRUE;
        pHdl->ulTmpNextCnt = RD_GetScanCnt();
    }

    if (pHdl->bChgFlag)
    {
        if (!pHdl->ulFltCnt)
        {
            HDL_Get_T_Many(pHdl, &pHdl->utChgTime);
            HDL_Get_PacketT_Many(pHdl, &pHdl->utPacketTime);
            pHdl->bSts = iNow;
            pHdl->bChgFlag = FALSE;
            *ulChgNextCnt = pHdl->ulTmpNextCnt;
        }
        else
            pHdl->ulFltCnt--;
    }
    pHdl->bLstSts = iNow;

    return pHdl->bSts;
}

/* 重置消抖时间.
 * Para:
 *     iDiNum, DI通道号.
 * Return:
 *     NONE.
 */
void HDL_Reset_Filt(int iDiNum, int iSubDaIdx)
{
    HDL_DI_HND *pHdl;

    pHdl = HdlBoxIOInfo_g.pahHdlDiHandle[iDiNum];
    pHdl->ulArrFltCnt[iSubDaIdx] = pHdl->ulFltCfg;
    pHdl->bArrChgFlag[iSubDaIdx] = TRUE;
    /* pHdl->ulArrTmpNextCnt[iSubDaIdx] = aimodHdl_g.ulNextCnt; */
    /* RE_SetLogDIUpdateCnt(); */  /* 置开入变位, 需更新 */
}

/*设置线路风暴时的消抖时间
 * Para:
 *    	NONE
 * Return:
 *     NONE.
 */
void HDL_Set_LineFilt()
{
    RD_LGC_DI_CH *plgcdi;
    HDL_DI_HND  *pHdl;

    for (plgcdi=plgcdich_g; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++)
    {
        if (plgcdi->mod==RD_HDL_BOX_DI)
        {
            pHdl=&(HdlBoxIOInfo_g.ahHdlDiHandle[plgcdi->p_part->aucHwAddr[0]*MAX_DI_PER_MOD+plgcdi->ucModCh]);
            if(uiAppType_g == APP_LINE || uiAppType_g == APP_BUS)
            {
                pHdl->aucFilt[0]=HH8(plgcdi->ulFiltTimeLine);
                pHdl->aucFilt[1]=HL8(plgcdi->ulFiltTimeLine);
                pHdl->aucFilt[2]=LH8(plgcdi->ulFiltTimeLine);
                pHdl->aucFilt[3]=LL8(plgcdi->ulFiltTimeLine);

                if (plgcdi->ucDIRefreshRate == DI_FAST_REFRESH_RATE)
                {
                    pHdl->ulFltCfgLine= plgcdi->ulFiltTimeLine/(1000000L/uiAiRate_g);
                    pHdl->ulFltCfgNormal = plgcdi->ulFiltTime/(1000000L/uiAiRate_g);

                }
                else if (plgcdi->ucDIRefreshRate == DI_MID_REFRESH_RATE)
                {
                    pHdl->ulFltCfgLine = (plgcdi->ulFiltTimeLine/(1000000L/uiAiRate_g))/DI_MID_REFRESH_INTERVAL;
                    pHdl->ulFltCfgNormal = (plgcdi->ulFiltTime/(1000000L/uiAiRate_g))/DI_MID_REFRESH_INTERVAL;
                }
                else if (plgcdi->ucDIRefreshRate == DI_SLOW_REFRESH_RATE)
                {
                    pHdl->ulFltCfgLine = (plgcdi->ulFiltTimeLine/(1000000L/uiAiRate_g))/DI_SLOW_REFRESH_INTERVAL;
                    pHdl->ulFltCfgNormal = (plgcdi->ulFiltTime/(1000000L/uiAiRate_g))/DI_SLOW_REFRESH_INTERVAL;
                }
            }
        }
    }
}

/*更改线路风暴时的消抖时间，其他应用不用*/
void HDL_Change_LineFilt()
{
    RD_LGC_DI_CH *plgcdi;
    HDL_DI_HND  *pHdl;
    int i;
    for (plgcdi=plgcdich_g; plgcdi<plgcdich_g+iLgcDiChNum_g; plgcdi++)
    {
        if (plgcdi->mod==RD_HDL_BOX_DI)
        {
            pHdl=&(HdlBoxIOInfo_g.ahHdlDiHandle[plgcdi->p_part->aucHwAddr[0]*MAX_DI_PER_MOD+plgcdi->ucModCh]);
            if(EP_GetStormState() && (uiAppType_g == APP_LINE  || uiAppType_g == APP_BUS))
            {
                pHdl->ulFltCfg = pHdl->ulFltCfgLine;
                /*与邹磊讨论:只在进入风暴的时候重置，退出时不影响*/
                for(i=0; i<HDL_DI_MAX_RECV_NUM; i++)
                {
                    if((pHdl->iSubDaIdx[i]<=0)&&(pHdl->iSubDaIdx[i]!=-2))
                    {
                        break;
                    }
                    pHdl->ulArrFltCnt[i] = pHdl->ulFltCfg;
                }
            }
            else
            {
                pHdl->ulFltCfg = pHdl->ulFltCfgNormal;
            }

        }
    }
}

/* 获取间隔压板状态
 * Para:
 *     SubNo, 间隔号, 从1开始.
 * Return:
 *     TRUE(投入), FALSE(退出).
 */
BOOL HDL_GetYabanState(int32_t SubNo)
{
    assert(SubNo <= iHdlCfgSubGoNum_g);

    return bSubGoYabanStat[SubNo];
}

/* 获取间隔检修不一致状态
 * Para:
 *     SubNo, 间隔号, 从1开始, 与其它接口保持一致.
 * Return:
 *     TRUE(不一致), FALSE(一致).
 */
BOOL HDL_GetRepairSts(int32_t SubNo)
{
    assert ((SubNo>0) && (SubNo <= iHdlCfgSubGoNum_g));

    return g_bSubRepairDiffer[SubNo-1];
}

/* 设置GOOSE DI是否需要刷新标识
 * Para:
 *     bStatus, 设置的GOOSE DI刷新状态
 * Return:
 *     NONE.
 */
void HDL_SetGooseDiNeedRefresh(BOOL bStatus)
{
    int iLock;

    iLock = intLock();
    g_bGooseDiNeedRefresh = bStatus;
    intUnlock(iLock);

    return;
}

/* 获取GOOSE DI是否需要刷新标识
 * Para:
 *     pStatus, 获取的GOOSE DI刷新状态返回指针
 * Return:
 *     NONE.
 */
void HDL_GetGooseDiNeedRefresh(BOOL *pStatus)
{
    int iLock;

    iLock = intLock();
    *pStatus = g_bGooseDiNeedRefresh;
    intUnlock(iLock);

    return;
}

BOOL ReadSubDaValueBySubMapData(void *pSubMapData, GOOSE_DA_VALUE *Da_Val, int *nStat)
{

    return FALSE;
}