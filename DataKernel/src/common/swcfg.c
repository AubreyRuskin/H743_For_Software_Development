/* swcfg.c - This file contains programs to manager software config file */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
02a, 30may07, dy change the code style.
01c, 29jul03, hdx Updated to version 1.0.
01b, 27may03 hdx Verified version 0.1.
01a, 15feb03, hdx Created first version 0.1.
*/

/*
DESCRIPTION
This file contains programs to manager software config file.
INCLUDE: swcfg.h
*/

/* includes */

#include "swcfg.h"
#include "errtest.h"
#include "datetime.h"
#include "realdata.h"
#include "miscfunc.h"
#include "view.h"
#include "filetool.h"
#include "rec.h"
#include "sysinfo.h"
#include "detailoperate_log.h"
#include "FileCRC.h"

/* 所有平台包含 */
#include "measure.h"

#include <stdio_compat.h>
#include "string_compat.h"
#include <ioLib.h>
#include "EdpVer.h"		/* 程序版本 */
#include "edp_asst.h"
#include "FileCRC.h"
#include "dsp.h"
#include "AppInterface.h"
#include "RE_AllTuyuanDataDef.h"
#include "logmsg.h"
#include "taskLib.h"
#include "logLib.h"

/* typedefs */

typedef struct
{
    int iMaxSetArea;
    int iWorkSetArea;		/* 当前设置运行定值区号*/
    int iRealWorkSetArea;			/* 当前实际运行定值区号 */
    uint8_t aucSetAreaFg[256];
#define SET_HAVE_FILE   0x01
#define SET_VALID       0x02

    BOOL bSetChg;
    uint8_t curLinkMode;   		/* 当前压板采用模式 */
    int  iNextWorkArea;      					/* 即将切换的运行定值区，逻辑图扫描定值切换系统服务输出图元设置
	                                      						 mmi读取 */
} SC_SYS_INFO;

typedef struct
{
    uint8_t ucType;
    uint32_t ulLen;
    uint8_t *pucDat;
    uint32_t ulBufLen;
} SC_CFG_ITEM;


/*2008-2-21 DQ 用于记录启动时定值是否有效标志，供定值固化时使用*/
BOOL bSetIsValid_g=FALSE;

/* 定值量程文件生成结束标识 */
static BOOL bRangeFileOverFlg_g = FALSE;

/* 量程调整计数（用于HMI和sgView更新）*/
static uint8_t bRangeChgCnt_g = 0;

/* 保护定值区正在进行校验标识,正在校验时禁止整定定值区 */
static BOOL bAreaSetChkFlg_g = FALSE;

/* locals */
/*090421 by xts 去除scinfo_g的static属性*/
SC_SYS_INFO scinfo_g;

int iSetPgNum_g;
/*090421 by xts 去除psetpg_g的static属性*/

SC_SET_PAGE *psetpgWr_g; /* 固化定值指针 */
SC_SET_PAGE *psetpg_g;


int iLinkNum_g;
int iLkPgNum_g;
uint16_t unSwCrc_g = 0;

/*uint8_t aucLkTpDiSrc_g[MAX_ID_LEN+1];*/
/*void *pvLinkTpDI_g;*/
static SC_LINK_PAGE *plkpg_g;
SC_LINK_ITEM *plink_g;
SC_LINK_ITEM *plinkWr_g;  /* 固化定值指针 */

static BOOL  *pLinkFirstAccessFlag_g;/*压板首次访问标志数组,张云添加  */

static BOOL bSettingRangeChg_g = FALSE;  /* 定值最大值、最小值更新标志 */

int iSubLgcNum_g;
uint8_t aucEqName_g[MAX_ID_LEN+1];
SC_SUB_LGC_ITEM *psublgc_g;

SC_SET_ITEM *pCoefTailSet_g;   /*决定比例系数、量程等的定值字符串尾的控制字定值指针*/
BOOL bCoefTailChg;

static int iSyseBaseNum_g; /*一页索引定值字符串基的个数*/
static SC_SY_SETBASE_ITEM  *psysetbase_g;  /*一页索引定值字符串基的数据结构*/
static LOGICYBTT LogicYbTT_g;
static uint16_t ulTotalLinkMode_g;  /* 总压板模式 */

/* 为线路保护定制的遥信量名称数组 */
char ucArrMeaDiName[LINE_CUST_MEA_DI_NUM][TEMP_INFO_MAX_LEN] =
{
    LINE_CUST_MEA_DI1,
    LINE_CUST_MEA_DI2,
    LINE_CUST_MEA_DI3,
    LINE_CUST_MEA_DI4,
    LINE_CUST_MEA_DI5,
    LINE_CUST_MEA_DI6,
    LINE_CUST_MEA_DI7,
    LINE_CUST_MEA_DI8,
    LINE_CUST_MEA_DI9,
    LINE_CUST_MEA_DI10,
    LINE_CUST_MEA_DI11,
    LINE_CUST_MEA_DI12,
    LINE_CUST_MEA_DI13,
    LINE_CUST_MEA_DI14,
    LINE_CUST_MEA_DI15,
    LINE_CUST_MEA_DI16,
    LINE_CUST_MEA_DI17,
    LINE_CUST_MEA_DI18,
    LINE_CUST_MEA_DI19,
    LINE_CUST_MEA_DI20,
    LINE_CUST_MEA_DI21
};

/* local functions */

static EP_STATUS SC_Deal_Cfg_Item(SC_CFG_ITEM *pcfg);
static EP_STATUS SC_Cfg_Setting(uint8_t *pucCfg, uint32_t ulLen);
static EP_STATUS SC_Cfg_Int_Set(uint8_t *pucCfg, uint32_t ulLen);
static EP_STATUS SC_Cfg_Link(uint8_t *pucCfg, uint32_t ulLen);
static EP_STATUS SC_Rd_Link_Mode_File(void);
static EP_STATUS SC_Rd_Func_Sts(void);
static EP_STATUS SC_Rd_Sw_Link(void);
static EP_STATUS SC_Updt_Work_Set(void);
static EP_STATUS SC_Updt_Work_Set_And_Log(int iArea);
static EP_STATUS SC_Updt_Link(void);
static void SC_Updt_Certain_Link(int iIdx);

EP_STATUS  SC_CK_SY_SET(void);			/*检查索引定值的配置及在ai，ao，遥测，测量配置的使用是否正确*/

/***********************************************************************
* UpdateAcCoff - 更新交流通道系数
*
* RETURNS: 无
*
*/
extern void UpdateAcCoff(void);

extern void SI_New_DI_File(void);
extern void SI_New_Link_File(void);

/* 设置GOOSE DI是否需要刷新标识
 * Para:
 *     bStatus, 设置的GOOSE DI刷新状态
 * Return:
 *     None.
 */
extern void HDL_SetGooseDiNeedRefresh(BOOL bStatus);

extern BOOL b01IniChanged;

/* functions */

/* 初始化软件配置模块
 * Para:
 *     strSwCfgFile, 软件配置文件名称.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS SC_Initialize(const uint8_t *strSwCfgFile)
{
    int iFd;
    uint8_t aucBuf[100];
    int iItemNum;
    SC_CFG_ITEM cfg;
    int i;
    uint32_t ul;
    int32_t lPos;
    EP_STATUS stsRet;

    stsRet = EP_SUCCESS;

    lPos = -1;

    if ((iFd = open(strSwCfgFile, O_RDONLY, 0)) == ERROR)
    {
        stsRet = EP_CFG_ERR;
        goto ret1;
    }

    lseek(iFd, -4, SEEK_END);

    lPos = -4;

    if (read(iFd, aucBuf, 4) != 4 ||
            aucBuf[0] != 0x28 || aucBuf[1] != 0 || aucBuf[2] != 0 || aucBuf[3] != 0xD7)
        goto reterr;

    lseek(iFd, 0, SEEK_SET);

    lPos = 0;

    if (read(iFd, aucBuf, 8) != 8 ||
            aucBuf[0] != 0x22 || aucBuf[1] != 0 || aucBuf[2] != 0 || aucBuf[3] != 0xDD)
        goto reterr;
    else
    {
        uint16_t ulPrgVer;
        uint16_t ulProtocolVer;

        lPos = 8;
        ulProtocolVer = U8_TO_U16(aucBuf[5], aucBuf[4]);
        if (SI_SysVer_g.unCfgProtocolVer!=ulProtocolVer)
        {
            /* 去掉告警信息 */
        }

        ulPrgVer = U8_TO_U16(aucBuf[7], aucBuf[6]);
        if (SI_SysVer_g.unCfgPrgVer!=ulPrgVer)
        {
            /* 去掉告警信息 */
        }
    }

    i = read(iFd, aucBuf, 4);
    assert (i == 4);
    lPos = 12;
    iItemNum = aucBuf[0];
    SI_SysVer_g.unSwCfgVer = U8_TO_U16(aucBuf[2], aucBuf[1]);

    cfg.ulBufLen = 0;
    cfg.pucDat = NULL;
    for (i=0; i<iItemNum; i++)
    {
        ul = read(iFd, aucBuf, 5);
        assert (ul == 5);
        lPos += ul;

        cfg.ucType = aucBuf[0];
        ul = BYTES_TO_U32(aucBuf+1);
        if (ul>cfg.ulBufLen)
        {
            if (cfg.pucDat)
                EP_free(cfg.pucDat);

            if ((cfg.pucDat = malloc(ul)) != NULL)
                cfg.ulBufLen = ul;
            else
                goto reterr;
        }

        cfg.ulLen = read(iFd, cfg.pucDat, ul);
        assert (cfg.ulLen == ul);
        lPos += ul;

        if (SC_Deal_Cfg_Item(&cfg) != EP_SUCCESS)
        {
            EP_free(cfg.pucDat);
            goto reterr;
        }
    }

    /* Initialize OK here. */
    assert (i == iItemNum);
    if (cfg.ulBufLen)
        EP_free(cfg.pucDat);

    assert (stsRet == EP_SUCCESS);
    goto ret2;

reterr:
    stsRet = EP_CFG_ERR;

ret2:
    close(iFd);

ret1:
    if (stsRet == EP_CFG_ERR)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                       "软件配置文件异常(%d)\n",
                       (int)cfg.ucType, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                       "Software configuration file error(%d)\n",
                       cfg.ucType, 0);
        }
    }

    /* 匹配AI AO 遥测 测量的逻辑标识字符串基 */
    if (SC_CK_SY_SET() != EP_SUCCESS)
    {
        stsRet = EP_CFG_ERR;

        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                       "错误码:%02d\n",
                       NOT_MATCH_INDEX_SETTING, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                       "Error code:%02d\n",
                       NOT_MATCH_INDEX_SETTING, 0);
        }
    }

    return stsRet;
}

/* 初始化读取定值量程文件
 * Para:
 *     void
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS SC_RD_SetRange(void)
{
    int iFd,i;
    int iCh;
    int isetpgNum,isetNum;
    uint8_t aucBuf[MAX_ID_LEN];
    uint16_t unSwCrcFile;
    SC_SET_PAGE *psetpg=NULL;
    SC_SET_ITEM *pset;

    /* 计算本次上电生效的软件配置的CRC */
    unSwCrc_g = FT_File_CRC16(EP_SW_CFG_FILE);

    if(FT_Is_File(EP_SET_RANGE_FILE)==FALSE)
    {
        LOG_Dbg_Msg("定值量程文件不存在.\n", 0, 0, 0, 0, 0, 0);
        return EP_ERROR; /* 定值量程文件不存在则使用软件配置默认值 */
    }

    if ((iFd = open(EP_SET_RANGE_FILE, O_RDONLY, 0)) == ERROR)
    {
        LOG_Dbg_Msg("定值量程文件打开失败.\n", 0, 0, 0, 0, 0, 0);
        return EP_ERROR;
    }

    if(SC_SetRange_Check_CRC(iFd)==FALSE)  /* 验证定值量程文件的有效性 */
    {
        LOG_Write(LOG_KERNEL, "定值量程文件CRC 检验不匹配.\n", NULL);
        close(iFd);
        return EP_ERROR;
    }

    lseek(iFd, -4, SEEK_END);
    if (read(iFd, aucBuf, 2) != 2)
    {
        close(iFd);
        LOG_Dbg_Msg("定值量程文件读取失败.\n", 0, 0, 0, 0, 0, 0);
        return EP_ERROR;
    }
    unSwCrcFile = U8_TO_U16(aucBuf[1],aucBuf[0]);
    if(unSwCrcFile != unSwCrc_g)
    {
        /* 软件配置不一致，不使用且删除量程文件 */
        close(iFd);
        remove(EP_SET_RANGE_FILE);
        LOG_Dbg_Msg("软件配置不一致unSwCrcFile=%x unSwCrc_g=%x \n", unSwCrcFile, unSwCrc_g, 0, 0, 0, 0);
        return EP_ERROR;
    }

    lseek(iFd, 0, SEEK_SET);

    i = read(iFd, aucBuf, 5);
    assert (i == 5);

    isetpgNum = aucBuf[0];  /* 定值集总页数, 包括内部定值+保护定值+测控定值 */
    if(isetpgNum != (iSetPgNum_g+1))
    {
        close(iFd);
        LOG_Dbg_Msg("定值量程文件中定值总页数不一致:记录:%d 实际:%d \n", isetpgNum, iSetPgNum_g, 0, 0, 0, 0);
        return EP_ERROR;
    }

    for (iCh=0; iCh<isetpgNum; iCh++)
    {
        if (iCh<(isetpgNum-1))
        {
            /* 内部定值+保护定值 */
            psetpg = psetpg_g+iCh;
            i = read(iFd, aucBuf, 8);
            assert (i == 8);

            isetNum = U8_TO_U16(aucBuf[3],aucBuf[2]);  /* 该页定值个数 */
            if(isetNum != psetpg->iSetNum)
            {
                close(iFd);
                LOG_Dbg_Msg("定值量程文件第%d 定值页中定值数不一致:记录:%d 实际:%d \n", iCh, isetNum, psetpg->iSetNum, 0, 0, 0);
                return EP_ERROR;
            }

            for (pset=psetpg->pset; pset<psetpg->pset+isetNum; pset++)
            {
                i = read(iFd, aucBuf, 25);
                assert (i == 25);

                pset->valMax.ulVal = BYTES_TO_U32(aucBuf+5);  /* 定值最大值 */
                pset->valMin.ulVal = BYTES_TO_U32(aucBuf+9);  /* 定值最小值 */
                pset->valDft.ulVal = BYTES_TO_U32(aucBuf+13);  /* 定值默认值 */
                pset->valStep.ulVal = BYTES_TO_U32(aucBuf+17);  /* 定值步长 */
            }
        }
        else
        {
            /* 测控定值 */
            i = read(iFd, aucBuf, 8);
            assert (i == 8);

            isetNum = U8_TO_U16(aucBuf[3],aucBuf[2]);  /* 该页定值个数 */
            if(isetNum != iCkSetNum_g)
            {
                close(iFd);
                LOG_Dbg_Msg("定值量程文件测控定值页中定值数不一致:记录:%d 实际:%d \n",isetNum, psetpg->iSetNum, 0, 0, 0, 0);
                return EP_ERROR;
            }

            for (pset=pCkset_g; pset<pCkset_g+iCkSetNum_g; pset++)
            {
                i = read(iFd, aucBuf, 25);
                assert (i == 25);

                pset->valMax.ulVal = BYTES_TO_U32(aucBuf+5);  /* 定值最大值 */
                pset->valMin.ulVal = BYTES_TO_U32(aucBuf+9);  /* 定值最小值 */
                pset->valDft.ulVal = BYTES_TO_U32(aucBuf+13);  /* 定值默认值 */
                pset->valStep.ulVal = BYTES_TO_U32(aucBuf+17);  /* 定值步长 */
            }
        }
    }

    close(iFd);
    return EP_SUCCESS;
}

/* 获取定值量程
 * Para:
 *     pPara, result.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS SC_Get_SetRange(uint8_t **pPara)
{
    uint8_t *p;
    uint8_t aucBuf[32];
    uint16_t i;
    uint16_t unSwCrcFile;
    int temp_val;
    int file_lenth;
    int iFd;
    uint8_t *pucDat;  /* 文件内容存储地址 */

    if(FT_Is_File(EP_SET_RANGE_FILE)==FALSE)
    {
        LOG_Dbg_Msg("定值量程文件不存在.\n", 0, 0, 0, 0, 0, 0);
        return EP_ERROR; /* 定值量程文件不存在则使用软件配置默认值 */
    }

    if ((iFd = open(EP_SET_RANGE_FILE, O_RDONLY, 0)) == ERROR)
    {
        LOG_Dbg_Msg("定值量程文件打开失败.\n", 0, 0, 0, 0, 0, 0);
        return EP_ERROR;
    }

    if(SC_SetRange_Check_CRC(iFd)==FALSE)  /* 验证定值量程文件的有效性 */
    {
        LOG_Write(LOG_KERNEL, "定值量程文件CRC 检验不匹配.\n", NULL);
        close(iFd);
        return EP_ERROR;
    }

    lseek(iFd, -4, SEEK_END);
    if (read(iFd, aucBuf, 2) != 2)
    {
        close(iFd);
        LOG_Dbg_Msg("定值量程文件读取失败.\n", 0, 0, 0, 0, 0, 0);
        return EP_ERROR;
    }
    unSwCrcFile = U8_TO_U16(aucBuf[1],aucBuf[0]);
    if(unSwCrcFile != unSwCrc_g)
    {
        /* 软件配置不一致，不使用且删除量程文件 */
        close(iFd);
        remove(EP_SET_RANGE_FILE);
        LOG_Dbg_Msg("软件配置不一致unSwCrcFile=%x unSwCrc_g=%x \n", unSwCrcFile, unSwCrc_g, 0, 0, 0, 0);
        return EP_ERROR;
    }

    temp_val = lseek(iFd, 0, SEEK_SET);
    file_lenth = lseek(iFd, 0, SEEK_END);
    file_lenth = file_lenth-temp_val;  /* 定值量程文件长度 */

    if(file_lenth > Max_Common_Lenth)
    {
        close(iFd);
        LOG_Dbg_Msg("定值量程文件长度为%d , 超过最大报文长度%d \n", file_lenth, Max_Common_Lenth, 0, 0, 0, 0);
        return EP_ERROR;
    }

    if ((pucDat = (uint8_t *)malloc(file_lenth)) == NULL)
    {
        if (iFd >= 0)
        {
            /* 关闭已打开文件 */
            close(iFd);
        }
        return EP_ERROR;
    }
    lseek(iFd, 0, SEEK_SET);
    if (read(iFd, pucDat, file_lenth) != file_lenth)
    {
        free(pucDat);
        close(iFd);
        LOG_Dbg_Msg("定值量程文件内容读取失败.\n", 0, 0, 0, 0, 0, 0);
        return EP_ERROR;
    }

    p = *pPara;
    for(i=0; i<file_lenth; i++)
    {
        *p++ = pucDat[i];   /* 报文内容与定值量程内容相同 */
    }

    *pPara = p;
    free(pucDat);
    close(iFd);

    return EP_SUCCESS;
}

/***********************************************************************
* SC_Deal_Cfg_Item - 软件配置初始化
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS SC_Deal_Cfg_Item(
    SC_CFG_ITEM *pcfg		/* 配置项 */
)
{
    EP_STATUS stsRet;

    assert(pcfg && pcfg->ulBufLen>=pcfg->ulLen);

    switch (pcfg->ucType)
    {
        case 0:
            stsRet=SC_Cfg_Setting(pcfg->pucDat, pcfg->ulLen);
            break;

        case 1:
            stsRet=SC_Cfg_Int_Set(pcfg->pucDat, pcfg->ulLen);
            break;

        case 2:
            stsRet=SC_Cfg_Link(pcfg->pucDat, pcfg->ulLen);
            break;

        case 3:
            stsRet=VI_Cfg_Event(pcfg->pucDat, pcfg->ulLen);
            break;

        case 4:
            stsRet=VI_Cfg_Alarm(pcfg->pucDat, pcfg->ulLen);
            break;

        case 5:
            stsRet=RC_Cfg_Flag(pcfg->pucDat, pcfg->ulLen);
            break;

        case 6:
            stsRet=RC_Cfg_Rec_AI(pcfg->pucDat, pcfg->ulLen);
            break;

        case 7:
            stsRet=RC_Cfg_Rec_DI(pcfg->pucDat, pcfg->ulLen);
            break;

        case 8:
            stsRet=VI_Cfg_Mea_AI(pcfg->pucDat, pcfg->ulLen);
            break;

        case 9:
            stsRet=VI_Cfg_Mea_DI(pcfg->pucDat, pcfg->ulLen);
            break;

        case 10:
            stsRet=ME_Cfg_Measure_Value(pcfg->pucDat,pcfg->ulLen);

            break;

        case 11:
            stsRet=VI_Cfg_Mea_DO(pcfg->pucDat,pcfg->ulLen);
            break;

        case 12:/*2008-7-25日 张云在EDP01上也开放参数定值  */

            stsRet=SC_Cfg_Ck_Set(pcfg->pucDat,pcfg->ulLen);

            break;

        default:
            assert(FALSE);
            stsRet=EP_PARM_ERR;
            break;
    }


    return stsRet;
}

/***********************************************************************
* SC_Cfg_Setting - 定值配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS SC_Cfg_Setting(
    uint8_t *pucCfg,
    uint32_t ulLen
)
{
    int iPgCfgLen;
    int iItemCfgLen;
    int i;
    SC_SET_PAGE *psetpg;
    SC_SET_PAGE *psetpgWr;
    SC_SET_ITEM *pset;
    SC_SET_ITEM *psetWr;
    uint8_t aucPrtc[MAX_ID_LEN+1];
    uint8_t *puc;

    puc=pucCfg;

    iSetPgNum_g=*puc++;

    iSetPgNum_g++;                      /* Page 0 is the internal setting. */

    if ((psetpg_g=calloc(iSetPgNum_g, sizeof(*psetpg_g)))==NULL)
        return EP_BUF_ERR;

    /* 固化定值备份, 更新时进行写入 */
    if ((psetpgWr_g = calloc(iSetPgNum_g, sizeof(*psetpgWr_g))) == NULL)
        return EP_BUF_ERR;

    puc+=4;

    for (psetpg=psetpg_g+1, psetpgWr = psetpgWr_g+1; psetpg<psetpg_g+iSetPgNum_g; psetpg++, psetpgWr++)
    {
        iPgCfgLen=U8_TO_U16(puc[1], puc[0]);

        iPgCfgLen-=puc[2];
        EP_ID_Copy(psetpg->aucName, puc+3, puc[2]);
        puc+=3+puc[2];

        psetpg->bIsPub=*puc++ ? TRUE:FALSE;

        if (!psetpg->bIsPub)
        {
            iPgCfgLen-=1+puc[0];
            EP_ID_Copy(aucPrtc, puc+1, puc[0]);
            puc+=1+puc[0];

            for (i=0; i<iSubLgcNum_g; i++)
            {
                if (!strcmp(psublgc_g[i].aucName, aucPrtc))
                {
                    psetpg->psublgc=psublgc_g+i;
                    break;
                }
            }

            if (!psetpg->psublgc)
            {
                LOG_Dbg_Msg("ERROR: can't find protect %s for setting.\n",
                            (int)aucPrtc, 0, 0, 0, 0, 0);

                return EP_PARM_ERR;
            }
        }

        psetpg->iSetNum=U8_TO_U16(puc[1], puc[0]);
        *psetpgWr = *psetpg;

        if ((psetpg->pset=calloc(psetpg->iSetNum, sizeof(*psetpg->pset)))==NULL)
            return EP_BUF_ERR;

        if ((psetpgWr->pset = calloc(psetpgWr->iSetNum, sizeof(*psetpgWr->pset))) == NULL)
            return EP_BUF_ERR;

        puc+=6;

        for (pset=psetpg->pset, psetWr = psetpgWr->pset;
                pset<psetpg->pset+psetpg->iSetNum; pset++, psetWr++)
        {
            iItemCfgLen=U8_TO_U16(puc[1], puc[0]);
            puc+=2;

            iPgCfgLen-=2+iItemCfgLen;

            pset->ucUnit=*puc++;

            iItemCfgLen-=puc[0];
            EP_ID_Copy(pset->aucId, puc+1, puc[0]);
            if(!strcmp(DESIDE_COEF_VALUE_SETTING_ID,pset->aucId) && pset->ucUnit==0)
            {
                pCoefTailSet_g=pset;
                bCoefTailChg=TRUE;
            }
            puc+=1+puc[0];

            iItemCfgLen-=puc[0];
            EP_ID_Copy(pset->aucName, puc+1, puc[0]);
            puc+=1+puc[0];

            pset->aucABRV[0]=*puc++;
            pset->aucABRV[1]=*puc++;
            pset->aucABRV[2]=*puc++;
            pset->aucABRV[3]=*puc++;
            pset->bStdSet=(*puc&0x01)?TRUE:FALSE;		/* 国网标准 */

            puc+=4;

            pset->valMax.ulVal=BYTES_TO_U32(puc);
            pset->valMaxOrg.ulVal = pset->valMax.ulVal;
            puc+=4;

            pset->valMin.ulVal=BYTES_TO_U32(puc);
            pset->valMinOrg.ulVal = pset->valMin.ulVal;
            puc+=4;

            pset->valDft.ulVal=BYTES_TO_U32(puc);
            pset->valDftOrg.ulVal = pset->valDft.ulVal;
            puc+=4;


            if(pset->ucUnit==0x68)
            {
                /*2008-7-16日，支持字符串定值，张云  */
                assert(pset->valMax.ulVal<=MAX_ID_LEN);
                assert(pset->valMin.ulVal<=MAX_ID_LEN);
                assert(pset->valDft.ulVal<=MAX_ID_LEN);

                strncpy(pset->aucDftStr,puc,pset->valDft.ulVal);
                strncpy(pset->aucNowStr,puc,pset->valDft.ulVal);
                puc+=pset->valDft.ulVal;
                iItemCfgLen-=pset->valDft.ulVal;
            }
            /* Init now value same as default.  It will be modified in
             * SC_Updt_Work_Set called by SC_Chk_Set if setting area file OK. */
            pset->valNow.ulVal=pset->valDft.ulVal;

            pset->valStep.ulVal=BYTES_TO_U32(puc);  /* 步长 */
            pset->valStepOrg.ulVal = pset->valStep.ulVal;
            puc+=4;

            i=U8_TO_U16(puc[1], puc[0]);
            assert(i<=1024);
            if ((pset->pucUnitName=malloc(i+1))==NULL)
                return EP_BUF_ERR;
            iItemCfgLen-=i;
            memcpy(pset->pucUnitName, puc+2, i);
            pset->pucUnitName[i]='\0';
            puc+=2+i;

            *psetWr = *pset;

            assert(iItemCfgLen==29);
        }

        assert(iPgCfgLen==8);
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
        return EP_SUCCESS;
}

/***********************************************************************
* SC_Cfg_Int_Set - 内部定值配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS SC_Cfg_Int_Set(
    uint8_t *pucCfg,
    uint32_t ulLen
)
{
    int iItemCfgLen;
    int i;
    SC_SET_ITEM *pset;
    uint8_t *puc;
    uint8_t aucPrtc[MAX_ID_LEN+1];
    SC_SET_ITEM *psetWr;

    /* Page 0 is the internal setting.  Setting page must be configured before. */
    assert(psetpg_g);

    puc=pucCfg;

    psetpg_g->iSetNum=U8_TO_U16(puc[1], puc[0]);
    psetpgWr_g->iSetNum = psetpg_g->iSetNum;

    if ((psetpg_g->pset=calloc(psetpg_g->iSetNum, sizeof(*psetpg_g->pset)))==NULL)
        return EP_BUF_ERR;

    if ((psetpgWr_g->pset = calloc(psetpgWr_g->iSetNum, sizeof(*psetpgWr_g->pset))) == NULL)
        return EP_BUF_ERR;

    puc += 5;

    AdMdType.iMaxTypeNum=(int32_t)(*puc++);		/* 索引定值页数 */
    if ((AdMdType.iMaxTypeNum>0) && AdMdType.bValid)
    {
        /* 有索引定值页,同时当前定值页有效 */
        if(AdMdType.iCurrentType >= AdMdType.iMaxTypeNum)
        {
            /* 如果大于索引定值页数 */
            AdMdType.iCurrentType=0;
            AdMdType.bChgFlag = FALSE; /* 不需更新 */
            AdMdType.bValid = FALSE;  /* 无效 */

            /* 无效时告警 */
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SET_ERR,
                           ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                           "错误码:%02d\n", INDEX_SETTING_PAGE_NUM_OVERFLOW_ERR, 0);
                LOG_Write(LOG_KERNEL, "索引定值页序越界.\n", NULL);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SET_ERR,
                           ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                           "Error code:%02d\n", INDEX_SETTING_PAGE_NUM_OVERFLOW_ERR, 0);
                LOG_Write(LOG_KERNEL, "Index setting page number overflow.\n", NULL);
            }
        }
        else
        {
            /* 索引定值页序有效 */
            AdMdType.bChgFlag = TRUE;  /* 更新 */
            AdMdType.bValid = TRUE;  /* 有效 */
        }
    }
    else
    {
        /* 无索引定值页 */
        AdMdType.bChgFlag = FALSE;	/* 不更新 */
        AdMdType.bValid = FALSE;  /* 无效 */
    }

    for (pset=psetpg_g->pset, psetWr = psetpgWr_g->pset; pset<psetpg_g->pset+psetpg_g->iSetNum; pset++, psetWr++)
    {
        iItemCfgLen=U8_TO_U16(puc[1], puc[0]);
        puc+=2;

        pset->ucUnit=*puc++;

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pset->aucId, puc+1, puc[0]);
        puc+=1+puc[0];
        if(!strcmp(DESIDE_COEF_VALUE_SETTING_ID,pset->aucId) && pset->ucUnit==0)
        {
            if(!bCoefTailChg)
            {
                pCoefTailSet_g=pset;
                bCoefTailChg=TRUE;
            }
            else
            {
                LOG_Dbg_Msg("ERROR: control word setting of deciding ai model type has configured before.\n",
                            0, 0, 0, 0, 0, 0);

                assert(FALSE);
            }
        }

        iItemCfgLen-=puc[0];
        EP_ID_Copy(pset->aucName, puc+1, puc[0]);
        puc+=1+puc[0];

        pset->aucABRV[0]=*puc++;
        pset->aucABRV[1]=*puc++;
        pset->aucABRV[2]=*puc++;
        pset->aucABRV[3]=*puc++;

        pset->bIsPrvtUse=*puc++ ? TRUE:FALSE;

        if (pset->bIsPrvtUse)
        {
            iItemCfgLen-=1+puc[0];
            EP_ID_Copy(aucPrtc, puc+1, puc[0]);
            puc+=1+puc[0];

            for (i=0; i<iSubLgcNum_g; i++)
            {
                if (!strcmp(psublgc_g[i].aucName, aucPrtc))
                {
                    pset->psublgc=psublgc_g+i;
                    break;
                }
            }

            if (! pset->psublgc)
            {
                LOG_Dbg_Msg("ERROR: can't find protect %s for setting.\n",
                            (int)aucPrtc, 0, 0, 0, 0, 0);

                return EP_PARM_ERR;
            }
        }
        pset->ucAttr=*puc++;		/* 内部定值属性，0: 通用内部定值1: 索引内部定值 */
        pset->bStdSet=(*puc&0x01)?TRUE:FALSE;		/* 国网标准 */
        puc+=2;

        puc+=2;                         /* TODO: is this SEQ. useful? */

        pset->valMax.ulVal=BYTES_TO_U32(puc);
        puc+=4;

        pset->valMin.ulVal=BYTES_TO_U32(puc);
        puc+=4;

        pset->valDft.ulVal=BYTES_TO_U32(puc);
        puc+=4;


        if(pset->ucUnit==0x68)
        {
            /*2008-7-16日，支持字符串定值，张云  */
            assert(pset->valMax.ulVal<=MAX_ID_LEN);
            assert(pset->valMin.ulVal<=MAX_ID_LEN);
            assert(pset->valDft.ulVal<=MAX_ID_LEN);

            strncpy(pset->aucDftStr,puc,pset->valDft.ulVal);
            strncpy(pset->aucNowStr,puc,pset->valDft.ulVal);
            puc+=pset->valDft.ulVal;
            iItemCfgLen-=pset->valDft.ulVal;
        }

        /* Init now value same as default.  It will be modified in
         * SC_Updt_Inner_Set called by SC_Chk_Set if inner setting file OK. */
        pset->valNow.ulVal=pset->valDft.ulVal;

        puc+=4;                         /* Skip the unused 4 bytes. */

        i=U8_TO_U16(puc[1], puc[0]);
        assert(i<=1024);
        if ((pset->pucUnitName=malloc(i+1))==NULL)
            return EP_BUF_ERR;
        iItemCfgLen-=i;
        memcpy(pset->pucUnitName, puc+2, i);
        pset->pucUnitName[i]='\0';
        puc+=2+i;

        *psetWr = *pset;

        assert(iItemCfgLen==31);
    }

    if (puc-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }
    else
        return EP_SUCCESS;
}

/***********************************************************************
* SC_Cfg_Link - 压板配置
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS SC_Cfg_Link(
    uint8_t *pucCfg,
    uint32_t ulLen
)
{
    uint8_t *puc;
    uint8_t *pucPgBgn;
    int i;
    SC_LINK_PAGE *plkpg;
    SC_LINK_ITEM *plink;
    SC_LINK_ITEM *plinkWr; /* 固化压板 */

    int iPgCfgLen;
    uint8_t aucPrtc[MAX_ID_LEN+1];
    int iItemCfgLen;
    BOOL   *pFirstAccessFlag;
    int iHdlength;
    puc=pucCfg;

    /*if (puc[0])
    {
        EP_ID_Copy(aucLkTpDiSrc_g, puc+1, puc[0]);

        pvLinkTpDI_g=RD_Get_Handle(aucLkTpDiSrc_g,RD_LGC_DI_HDL);

    }*/

    puc+=1+puc[0];

    iLkPgNum_g=*puc++;

    if ((plkpg_g=calloc(iLkPgNum_g, sizeof(*plkpg_g)))==NULL)
        return EP_BUF_ERR;

    pucPgBgn=puc+4;

    for (plkpg=plkpg_g; plkpg<plkpg_g+iLkPgNum_g; plkpg++)
    {
        puc=pucPgBgn;

        iPgCfgLen=U8_TO_U16(puc[1], puc[0]);
        puc+=3+puc[2];

        plkpg->bIsPub=*puc++ ? TRUE:FALSE;
        if (!plkpg->bIsPub)
            puc+=1+puc[0];

        plkpg->iLinkNum=U8_TO_U16(puc[1], puc[0]);
        plkpg->iLinkPos=iLinkNum_g;
        iLinkNum_g+=plkpg->iLinkNum;

        pucPgBgn+=2+iPgCfgLen;
    }

    if (pucPgBgn-pucCfg!=ulLen)
    {
        assert(FALSE);
        return EP_PARM_ERR;
    }

    if ((plink_g=calloc(iLinkNum_g, sizeof(*plink_g)))==NULL)
        return EP_BUF_ERR;

    /* 固化压板 */
    if ((plinkWr_g = calloc(iLinkNum_g, sizeof(*plinkWr_g))) == NULL)
        return EP_BUF_ERR;

    if ((pLinkFirstAccessFlag_g=calloc(iLinkNum_g, sizeof(*pLinkFirstAccessFlag_g)))==NULL)
        return EP_BUF_ERR;
    for(pFirstAccessFlag=pLinkFirstAccessFlag_g; pFirstAccessFlag<pLinkFirstAccessFlag_g+iLinkNum_g; pFirstAccessFlag++)
    {
        /*初始化初次访问标志  */
        *pFirstAccessFlag=FALSE;
    }

    puc=pucCfg+pucCfg[0]+6;
    plink=plink_g;
    plinkWr = plinkWr_g;  /* 固化压板 */

    for (plkpg=plkpg_g; plkpg<plkpg_g+iLkPgNum_g; plkpg++)
    {
        iPgCfgLen=U8_TO_U16(puc[1], puc[0]);

        iPgCfgLen-=puc[2];
        EP_ID_Copy(plkpg->aucName, puc+3, puc[2]);
        puc+=3+puc[2];

        puc++;                          /* Skip IsPub. */

        if (!plkpg->bIsPub)
        {
            iPgCfgLen-=1+puc[0];
            EP_ID_Copy(aucPrtc, puc+1, puc[0]);
            puc+=1+puc[0];

            for (i=0; i<iSubLgcNum_g; i++)
            {
                if (!strcmp(psublgc_g[i].aucName, aucPrtc))
                {
                    plkpg->psublgc=psublgc_g+i;
                    break;
                }
            }

            if (!plkpg->psublgc)
            {
                LOG_Dbg_Msg("ERROR: can't find protect %s for link.\n",
                            (int)aucPrtc, 0, 0, 0, 0, 0);

                return EP_PARM_ERR;
            }
        }

        puc+=6;

        for (i=0; i<plkpg->iLinkNum; i++, plink++, plinkWr++)
        {
            iItemCfgLen=U8_TO_U16(puc[1], puc[0]);
            puc+=2;

            iPgCfgLen-=2+iItemCfgLen;

            iItemCfgLen-=puc[0];
            EP_ID_Copy(plink->aucId, puc+1, puc[0]);
            puc+=1+puc[0];

            iItemCfgLen-=puc[0];
            EP_ID_Copy(plink->aucName, puc+1, puc[0]);
            puc+=1+puc[0];

            plink->aucABRV[0]=*puc++;
            plink->aucABRV[1]=*puc++;
            plink->aucABRV[2]=*puc++;
            plink->aucABRV[3]=*puc++;

            plink->HwLinkType=*puc++;
            plink->LinkSwitchMode=*puc;


            puc+=3;

            puc+=2;                         /* TODO: is this SEQ. useful? */

            plink->bDftVal=*puc++ ? TRUE:FALSE;

            iItemCfgLen-=puc[0];
            iHdlength = puc[0];
            if(iHdlength!=0)
                EP_ID_Copy(plink->aucDiSrc, puc+1, puc[0]);
            puc+=1+iHdlength;
            if(iHdlength!=0)
            {
                plink->pvHdDI=RD_Get_Handle(plink->aucDiSrc,RD_LGC_DI_HDL);
                /* TODO: check ID valid? */
                if(plink->HwLinkType&0x01)
                {
                    /*双开入模式*/
                    iItemCfgLen-=1+puc[0];
                    EP_ID_Copy(plink->aucSecondDiSrc, puc+1, puc[0]);
                    puc+=1+puc[0];
                    plink->pvHdSecondDI=RD_Get_Handle(plink->aucSecondDiSrc,RD_LGC_DI_HDL);
                }
            }
            plink->aucMode=LINK_MODE_NONE; 	/* 初始值为无效值，
                                            								 待SC_Chk_Set()->SC_Rd_Link_Mode_File()读取压板模式文件后,
                                            								 若为定制压板模式后再相应赋值 */


            assert(iItemCfgLen==14);

            *plinkWr = *plink; /* 用于固化判断 */
        }

        assert(iPgCfgLen==8);
    }

    return EP_SUCCESS;
}


/****************************保护功能分图投退设置状态的访问函数接口定义**********/

/***********************************************************************
* SCI_Init_Get_RelayFunc_RunExit_Set_Status - 访问调试配置模块管理的某保护功能分图的投退设置状态
*
* RETURNS: 返回操作状态
*					 EP_SUCCESS, 操作成功
*             	 EP_BAD_DATA, 找不到同名逻辑标识的保护功能分图
*                 EP_NOT_INIT, 找到多于1个的同名逻辑标识的保护功能分图
*                 EP_SYS_ERR, 其他原因导致的错误.
*
*/
EP_STATUS SCI_Init_Get_RelayFunc_RunExit_Set_Status(
    uint8_t *strID,			/* 该保护功能分图的分图名 */
    BOOL *pbRtSetStatus			/* 供返回该保护功能分图设置的保护投退状态TRUE为设置投入，FALSE为设置退出 */
)
{
    SC_SUB_LGC_ITEM *psublgc;
    BOOL bFind;

    assert(strID && pbRtSetStatus);

    bFind=FALSE;
    for (psublgc=psublgc_g; psublgc<psublgc_g+iSubLgcNum_g; psublgc++)
    {
        if (!strcmp(psublgc->aucName, strID))
        {
            if (!bFind)
            {
                bFind=TRUE;
                *pbRtSetStatus=psublgc->bRun;
            }
            else
            {
                assert(FALSE);
                return EP_NOT_INIT;
            }
        }
    }

    if (bFind)
        return EP_SUCCESS;
    else
    {
        assert(FALSE);
        return EP_BAD_DATA;
    }
}

/****************************定值(包括内部定值和一般定值)访问函数接口定义********/

/***********************************************************************
* SCI_Init_Get_Setting_Info - 根据定值的逻辑标识，获得定值的相关信息
*
* RETURNS: 返回操作状态
*                 EP_SUCCESS,操作成功
*                 EP_BAD_DATA,找不到同名逻辑标识的保护定值
*                 EP_NOT_INIT,找到多于1个的同名逻辑标识的保护定值
*                 EP_SYS_ERR,其他原因导致的错误.
*
*/
EP_STATUS SCI_Init_Get_Setting_Info(
    uint8_t  *strID,				/* 定值逻辑标识字符串 */
    SCI_SETTING_INFO_TYPE  *pRtSettingInfo			/* 供返回该定值的相关信息 */
)
{
    SC_SET_PAGE *psetpg;
    SC_SET_ITEM *pset;
    BOOL bFind;
    static uint8_t aucNullStr[]="";

    SC_SET_ITEM *psetHd;


    assert(strID && strlen(strID)<=MAX_ID_LEN);
    assert(pRtSettingInfo);

    bFind=FALSE;
    for (psetpg=psetpg_g; psetpg<psetpg_g+iSetPgNum_g; psetpg++)
    {
        for (pset=psetpg->pset; pset<psetpg->pset+psetpg->iSetNum; pset++)
        {
            if (!strcmp(pset->aucId, strID))
            {
                if (!bFind)
                {
                    bFind=TRUE;
                    if (psetpg==psetpg_g)
                    {
                        pRtSettingInfo->ucType=1;
                        if(pset->bIsPrvtUse)
                        {
                            assert(pset->psublgc);
                            pRtSettingInfo->strRelayFuncName=
                                pset->psublgc->aucName;
                        }
                        else
                            pRtSettingInfo->strRelayFuncName=aucNullStr;
                    }
                    else
                    {
                        pRtSettingInfo->ucType=0;

                        if (psetpg->bIsPub)
                            pRtSettingInfo->strRelayFuncName=aucNullStr;
                        else
                        {
                            assert(psetpg->psublgc);
                            pRtSettingInfo->strRelayFuncName=
                                psetpg->psublgc->aucName;
                        }
                    }

                    pRtSettingInfo->cPageNum=psetpg-psetpg_g;
                    pRtSettingInfo->nNumInPage=pset-psetpg->pset;
                    pRtSettingInfo->ucAttrib=pset->ucUnit;
                }
                else
                {
                    LOG_Dbg_Msg("ERROR: repeat logic ID \"%s\" in setting.\n",
                                (int)strID, 0, 0, 0, 0, 0);

                    return EP_NOT_INIT;
                }
            }
        }
    }

    if (bFind)
        return EP_SUCCESS;
    else
    {

        psetHd=SC_Get_CK_Set();
        for (pset=psetHd; pset<psetHd+iCkSetNum_g; pset++)
        {
            if (!strcmp(pset->aucId, strID))
            {
                if (!bFind)
                {
                    bFind=TRUE;
                    pRtSettingInfo->ucType=2;
                    pRtSettingInfo->strRelayFuncName=aucNullStr;
                    pRtSettingInfo->cPageNum=-1;
                    pRtSettingInfo->nNumInPage=pset-psetHd;
                    pRtSettingInfo->ucAttrib=pset->ucUnit;
                }
                else
                {
                    LOG_Dbg_Msg("ERROR: repeat logic ID \"%s\" in setting.\n",
                                (int)strID, 0, 0, 0, 0, 0);

                    return EP_NOT_INIT;
                }
            }
        }
        if (bFind)
            return EP_SUCCESS;
        else
        {
            LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\" of setting.\n",
                        (int)strID, 0, 0, 0, 0, 0);

            return EP_BAD_DATA;
        }


    }
}

/***********************************************************************
* SCI_Get_Inner_Setting - 根据内部定值的定值号，访问定值
*
* RETURNS: 返回操作状态
*                 EP_SUCCESS,操作成功
*                 EP_BAD_DATA,因定值号参数不对,导致的错误
*                 EP_SYS_ERR,其他原因导致的错误.
*
*/
EP_STATUS SCI_Get_Inner_Setting(
    int16_t  nNumInPage,			/* 该定值在内部定值表中的定值号 */
    SCI_SIGNAL_VALUE_TYPE  *pRtSettingValue				/* 供返回该内部定值 */
)
{
    SC_SET_ITEM *pset;

    pset=psetpg_g->pset+nNumInPage;

    pRtSettingValue->Value.ulVal=pset->valNow.ulVal;
    pRtSettingValue->ucAttrib=pset->ucUnit;

    if (pset->ucUnit == VAR_STRING)
    {
        pRtSettingValue->pvSrc=pset->aucNowStr;/* 2008-7-16日 支持字符串定值，张云 */
    }
    else
    {
        pRtSettingValue->pvSrc = (void *)pset;
    }

    return EP_SUCCESS;
}

/***********************************************************************
* SCI_Get_Inner_Setting_BySettingStrBase - 根据内部定值的字符串基，访问定值，ghx根据规约3.60增加
*
* RETURNS: 返回操作状态
*                 EP_SUCCESS,操作成功
*                 EP_BAD_DATA,因参数不对,导致的错误
*
*/
EP_STATUS SCI_Get_Inner_Setting_BySettingStrBase(
    uint8_t *strBaseID, 			/* 该定值的逻辑标识字符串基，需要再根据决定字符串尾的一般定值控制字决定最终的字符串 */
    SCI_SIGNAL_VALUE_TYPE  *pRtSettingValue				/* 供返回该内部定值 */
)
{
    SC_SET_ITEM *pset;
    uint8_t aucWholeSetting[MAX_ID_LEN+1];
    uint8_t aucStrTail[3];
    int i;

    assert(pRtSettingValue);

    if(pCoefTailSet_g)
    {
        /* 配置了模件类型控制字 */

        sprintf(aucStrTail,"%u", (int)pCoefTailSet_g->valNow.ulVal);
        strcpy(aucWholeSetting,strBaseID);
        strcat(aucWholeSetting,aucStrTail);
        for(i=0; i<psetpg_g->iSetNum; i++)
        {
            pset=psetpg_g->pset+i;
            if(!strcmp(pset->aucId,aucWholeSetting))
            {
                pRtSettingValue->Value.ulVal=pset->valNow.ulVal;
                pRtSettingValue->ucAttrib=pset->ucUnit;

                pRtSettingValue->pvSrc=pset->aucNowStr;/* 2008-7-16日 支持字符串定值，张云 */
                break;
            }
        }
        if(i==psetpg_g->iSetNum)
            return EP_BAD_DATA;
        else
            return EP_SUCCESS;
    }
    else
    {
        /* 如没有配则使用系统文件中保存的索引定值页号 */
        sprintf(aucStrTail,"%u", (int)AdMdType.iCurrentType);
        strcpy(aucWholeSetting,strBaseID);
        strcat(aucWholeSetting,aucStrTail);
        for(i=0; i<psetpg_g->iSetNum; i++)
        {
            pset=psetpg_g->pset+i;
            if(!strcmp(pset->aucId,aucWholeSetting))
            {
                pRtSettingValue->Value.ulVal=pset->valNow.ulVal;
                pRtSettingValue->ucAttrib=pset->ucUnit;

                pRtSettingValue->pvSrc=pset->aucNowStr;/* 2008-7-16日 支持字符串定值，张云 */
                break;
            }
        }
        if(i==psetpg_g->iSetNum)
            return EP_BAD_DATA;
        else
            return EP_SUCCESS;
    }
}

/* 获取通道增益系数索引定值 */
EP_STATUS SCI_Get_Coff_Inner_Setting_BySettingStrBase(
    uint8_t *strBaseID,  /* 该定值的逻辑标识字符串基，需要再根据决定字符串尾的一般定值控制字决定最终的字符串 */
    SCI_SIGNAL_VALUE_TYPE  *pRtSettingValue, /* 供返回该内部定值 */
    int32_t iIndexSn  /* 索引定值页序, 初始为-1 */
)
{
    SC_SET_ITEM *pset = NULL;
    uint8_t aucWholeSetting[MAX_ID_LEN+1];
    uint8_t aucStrTail[3];
    int i;

    if (iIndexSn<0)
    {
        /* 使用统一的切换模式 */
        sprintf(aucStrTail, "%u", (int)AdMdType.iCurrentType);
    }
    else
    {
        /* 按通道使用切换模式 */
        sprintf(aucStrTail, "%d", (int)iIndexSn);
    }

    strcpy(aucWholeSetting,strBaseID);
    strcat(aucWholeSetting,aucStrTail);
    for(i=0; i<psetpg_g->iSetNum; i++)
    {
        pset=psetpg_g->pset+i;
        if(!strcmp(pset->aucId,aucWholeSetting))
        {
            pRtSettingValue->Value.ulVal=pset->valNow.ulVal;
            pRtSettingValue->ucAttrib=pset->ucUnit;

            pRtSettingValue->pvSrc=pset->aucNowStr;/* 2008-7-16日 支持字符串定值，张云 */
            break;
        }
    }
    if(i==psetpg_g->iSetNum)
        return EP_BAD_DATA;
    else
        return EP_SUCCESS;
}

/*    根据一般定值的定值页和定值号信息，访问定值
      参数：
                cPageNum,   该定值所在的定值页号
                nNumInPage，该定值在定值页中的定值号
               pRtSettingValue,供返回该一般定值

      返回值：     返回操作状态
                   EP_SUCCESS,操作成功
                   EP_BAD_DATA,因定值页或定值号参数不对,导致的错误
                   EP_SYS_ERR,其他原因导致的错误.
*/
EP_STATUS SCI_Get_General_Setting(int8_t cPageNum, int16_t nNumInPage,
                                  SCI_SIGNAL_VALUE_TYPE  *pRtSettingValue)
{
    SC_SET_ITEM *pset;

    /* Page 1 to iSetPgNum-1 as general setting. */
    pset=psetpg_g[cPageNum].pset+nNumInPage;
    pRtSettingValue->Value.ulVal=pset->valNow.ulVal;
    pRtSettingValue->ucAttrib=pset->ucUnit;

    if(pset->ucUnit == VAR_STRING)
    {
        pRtSettingValue->pvSrc=pset->aucNowStr;/* 2008-7-16日 支持字符串定值，张云 */
    }
    else
    {
        pRtSettingValue->pvSrc = (void *)pset;
    }

    return EP_SUCCESS;
}

/***********************************************************************
* SET_GetRange - 获取软件配置中定值原始的最大值、最小值和默认值
* 参数:
* pSet : 定值句柄；
* pvalMax : 定值原始最大值；
* pvalMin : 定值原始最小值；
* pvalDft : 定值原始默认值；
* pvalStep : 定值原始步长.
*
* RETURNS: 返回获取状态
*                 EP_SUCCESS,获取成功
*                 EP_ERROR,获取失败
*
*/
EP_STATUS SET_GetRange(void *pSet,FLT_U32_UNION *pvalMax,
                       FLT_U32_UNION *pvalMin,FLT_U32_UNION *pvalDft, FLT_U32_UNION *pvalStep)
{
    SC_SET_ITEM *pRtset;

    assert(pSet);
    assert(pvalMax);
    assert(pvalMin);
    assert(pvalDft);
    assert(pvalStep);

    pRtset = (SC_SET_ITEM *)pSet;
    pvalMax->ulVal = pRtset->valMaxOrg.ulVal;
    pvalMin->ulVal = pRtset->valMinOrg.ulVal;
    pvalDft->ulVal = pRtset->valDftOrg.ulVal;
    pvalStep->ulVal = pRtset->valStepOrg.ulVal;

    return EP_SUCCESS;
}


/***********************************************************************
* SET_SetRange - 设置定值的最大值、最小值和默认值
* 参数:
* pSet : 定值句柄；
* valMax : 定值最大值；
* valMin : 定值最小值；
* valDft : 定值默认值；
* valStep: 定值步长；
*
* RETURNS: 返回设置状态
*                 EP_SUCCESS,设置成功
*                 EP_ERROR,设置失败
*
*/
EP_STATUS SET_SetRange(void *pSet, FLT_U32_UNION valMax,
                       FLT_U32_UNION valMin, FLT_U32_UNION valDft, FLT_U32_UNION valStep)
{
    SC_SET_ITEM *pRtset;

    assert(pSet);
    pRtset = (SC_SET_ITEM *)pSet;

    if(pRtset->ucUnit == VAR_STRING)   /* 字符串定值不可设置 */
    {
        return EP_ERROR;
    }

    if(IS_INT32_SET(pRtset->ucUnit))  /* 有符号整形 */
    {
        if( (pRtset->valMax.lVal != valMax.lVal)
                || (pRtset->valMin.lVal != valMin.lVal)
                || (pRtset->valDft.lVal != valDft.lVal)
                || (pRtset->valStep.lVal != valStep.lVal))
        {
            pRtset->valMax.lVal = valMax.lVal;
            pRtset->valMin.lVal = valMin.lVal;
            pRtset->valDft.lVal = valDft.lVal;
            pRtset->valStep.lVal = valStep.lVal;

            bSettingRangeChg_g = TRUE;
        }
    }
    else if(IS_UINT32_SET(pRtset->ucUnit))   /* 无符号整形 */
    {
        if((pRtset->valMax.ulVal != valMax.ulVal)
                || (pRtset->valMin.ulVal != valMin.ulVal)
                || (pRtset->valDft.ulVal != valDft.ulVal)
                || (pRtset->valStep.ulVal != valStep.ulVal))
        {
            pRtset->valMax.ulVal = valMax.ulVal;
            pRtset->valMin.ulVal = valMin.ulVal;
            pRtset->valDft.ulVal = valDft.ulVal;
            pRtset->valStep.ulVal = valStep.ulVal;

            bSettingRangeChg_g = TRUE;
        }
    }
    else  /* 浮点数 */
    {
        if((pRtset->valMax.fVal<(valMax.fVal-FLT_PRECISION))
                || ( pRtset->valMax.fVal>(valMax.fVal+FLT_PRECISION)) )
        {
            pRtset->valMax.fVal = valMax.fVal;
            bSettingRangeChg_g = TRUE;
        }
        if((pRtset->valMin.fVal<(valMin.fVal-FLT_PRECISION))
                || ( pRtset->valMin.fVal>(valMin.fVal+FLT_PRECISION)) )
        {
            pRtset->valMin.fVal = valMin.fVal;
            bSettingRangeChg_g = TRUE;
        }
        if ((pRtset->valDft.fVal<(valDft.fVal-FLT_PRECISION))
                || ( pRtset->valDft.fVal>(valDft.fVal+FLT_PRECISION)) )
        {
            pRtset->valDft.fVal = valDft.fVal;
            bSettingRangeChg_g = TRUE;
        }
        if((pRtset->valStep.fVal<(valStep.fVal-FLT_PRECISION))
                || ( pRtset->valStep.fVal>(valStep.fVal+FLT_PRECISION)) )
        {
            pRtset->valStep.fVal = valStep.fVal;
            bSettingRangeChg_g = TRUE;
        }
    }

    return EP_SUCCESS;
}

/* 设置定值量程完成，触发慢速处理任务
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SET_SetRangeOver(void)
{
    SLOW_MESSAGE_NODE Info;

    if(bSettingRangeChg_g==FALSE)
    {
        return;
    }

    bSettingRangeChg_g = FALSE;

    /* 触发更新定值最大值、最小值慢速任务 */
    Info.type = SETTING_RANGE_SET;
    msgQSend(SlowMessage, (char *)&Info,
             sizeof(SLOW_MESSAGE_NODE),
             NO_WAIT,
             MSG_PRI_NORMAL
            );
}

/* 设置定值量程文件生成结束标识
* 参数:
* bRFOverFlg: 要设置的标识(TRUE or FALSE)
* 返回值:
* 无
*/
void SC_Set_RangeFile_OverFlg(BOOL bRFOverFlg)
{
    bRangeFileOverFlg_g = bRFOverFlg;
}

/* 获取定值量程文件生成结束标识
* 参数:
* 无
* 返回值:
* 结束标识的值.
*/
BOOL SC_Get_RangeFile_OverFlg(void)
{
    return bRangeFileOverFlg_g;
}

/* 设置保护定值区正在校验标识
* 参数:
* bChkFlg: 要设置的标识(TRUE or FALSE)
* 返回值:
* 无
*/
void SC_Set_AreaSet_ChkFlg(BOOL bChkFlg)
{
    bAreaSetChkFlg_g = bChkFlg;
}

/* 获取保护定值区正在校验标识
* 参数:
* 无
* 返回值:
* 标识的值.
*/
BOOL SC_Get_AreaSet_ChkFlg(void)
{
    return bAreaSetChkFlg_g;
}

/* 设置定值量程量调整计数自增1
* 参数:
* 无
* 返回值:
* 无
*/
void SC_Set_ChgCnt_Plus(void)
{
    if(bRangeChgCnt_g>=0xFF)
    {
        bRangeChgCnt_g = 0;
    }
    bRangeChgCnt_g++;
}

/* 获取量程调整计数
* 参数:
* 无
* 返回值:
* 量程调整计数值.
*/
uint8_t SC_Get_Range_ChgCnt(void)
{
    return bRangeChgCnt_g;
}

/* 校验定值量程文件CRC */
BOOL SC_SetRange_Check_CRC(int iFd)
{
    char aucBuf[256];
    int temp_val;
    int file_lenth;
    int i = 0;
    int iRdlen;
    uint8_t *pucDat;  /* 文件内容存储地址 */
    uint16_t unCalcCrc = 0;
    uint16_t unFileCRC=0;

    temp_val = lseek(iFd, 0, SEEK_SET);
    file_lenth = lseek(iFd, 0, SEEK_END);
    file_lenth = file_lenth-temp_val-2;

    /* CRC计算 */
    if ((pucDat = malloc(file_lenth)) == NULL)
    {
        return FALSE;
    }
    lseek(iFd, 0, SEEK_SET);
    i = read(iFd, pucDat, file_lenth);	/* 读数据 */
    assert (i == file_lenth);
    unCalcCrc = EP_CCITT_CRC16(pucDat, file_lenth, unCalcCrc);
    free(pucDat);

    iRdlen = read(iFd,aucBuf,2);
    if(iRdlen != 2)
    {
        return  FALSE;
    }

    unFileCRC = U8_TO_U16(aucBuf[1],aucBuf[0]);

    if(unCalcCrc == unFileCRC)
    {
        return TRUE;
    }
    else
    {
        logMsg("computer crc is %x,file crc is %x\n",unCalcCrc,unFileCRC,0,0,0,0);
        return FALSE;
    }
}

/* 生成新的定值量程文件
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SC_New_SET_Range(void)
{
    int iFd, iCh, i;
    uint16_t usetlen,usetpglen;
    uint16_t unSwCrc;
    uint16_t unCalcCrc = 0;
    uint8_t aucBuf[MAX_ID_LEN];
    uint8_t *pucDat;  /* 文件内容存储地址 */
    uint32_t temp_val; /* 文件长度计算 */
    uint32_t file_lenth;
    EP_STATUS sts;
    const SC_SET_PAGE *psetpg=NULL;
    SC_SET_ITEM *pset;

    iFd = FT_Bgn_Update(EP_SET_RANGE_FILE);
    assert (iFd >= 0);

    assert((iSetPgNum_g >0) && ( iSetPgNum_g<=255));

    aucBuf[0] = iSetPgNum_g+1; /* 内部定值, 保护定值和测控定值 */

    /* 保留4个字节 */
    aucBuf[1] = 0x0;
    aucBuf[2] = 0x0;
    aucBuf[3] = 0x0;
    aucBuf[4] = 0x0;

    i = write(iFd, aucBuf, 5);
    assert (i == 5);

    for (iCh=0; iCh<iSetPgNum_g+1; iCh++)
    {
        if (iCh<iSetPgNum_g)
        {
            /* 内部定值+保护定值 */
            psetpg=SC_Get_Set_Pg_Attr(iCh);

            /* 该页定值数据内容大小 */
            usetpglen = 8+25*psetpg->iSetNum;
            aucBuf[0] = LO8(usetpglen);
            aucBuf[1] = HI8(usetpglen);

            /* 该页所有定值个数 */
            aucBuf[2] = LO8(psetpg->iSetNum);
            aucBuf[3] = HI8(psetpg->iSetNum);
        }
        else
        {
            /* 测控定值 */
            /* 该页定值数据内容大小 */
            usetpglen = 8+25*iCkSetNum_g;
            aucBuf[0] = LO8(usetpglen);
            aucBuf[1] = HI8(usetpglen);

            /* 该页所有定值个数 */
            aucBuf[2] = LO8(iCkSetNum_g);
            aucBuf[3] = HI8(iCkSetNum_g);
        }
        i = write(iFd, aucBuf, 4);
        assert (i == 4);

        for(i=0; i<4; i++)
        {
            aucBuf[i] = 0x0;  /* 保留4 字节 */
        }
        i = write(iFd, aucBuf, 4);
        assert (i == 4);

        if(iCh<iSetPgNum_g)
        {
            /* 内部定值+保护定值 */
            for (pset=psetpg->pset; pset<psetpg->pset+psetpg->iSetNum; pset++)
            {
                /* 该定值的数据内容大小 */
                usetlen = 25;
                aucBuf[0] = LO8(usetlen);
                aucBuf[1] = HI8(usetlen);
                /* 保留3个字节 */
                aucBuf[2] = 0x0;
                aucBuf[3] = 0x0;
                aucBuf[4] = 0x0;
                i = write(iFd, aucBuf, 5);
                assert (i == 5);

                U32_TO_BYTES(aucBuf, pset->valMax.ulVal);   /* 定值最大值 */
                U32_TO_BYTES(aucBuf+4, pset->valMin.ulVal);  /* 定值最小值 */
                U32_TO_BYTES(aucBuf+8, pset->valDft.ulVal);  /* 定值默认值 */
                U32_TO_BYTES(aucBuf+12, pset->valStep.ulVal);  /* 定值修改步长 */
                U32_TO_BYTES(aucBuf+16, (uint32_t)0);  /* 保留*/
                i = write(iFd, aucBuf, 20);
                assert (i == 20);
            }
        }
        else
        {
            /* 测控定值 */
            for (pset=pCkset_g; pset<pCkset_g+iCkSetNum_g; pset++)
            {
                /* 该定值的数据内容大小 */
                usetlen = 25;
                aucBuf[0] = LO8(usetlen);
                aucBuf[1] = HI8(usetlen);
                /* 保留3个字节 */
                aucBuf[2] = 0x0;
                aucBuf[3] = 0x0;
                aucBuf[4] = 0x0;
                i = write(iFd, aucBuf, 5);
                assert (i == 5);

                U32_TO_BYTES(aucBuf, pset->valMax.ulVal);   /* 定值最大值 */
                U32_TO_BYTES(aucBuf+4, pset->valMin.ulVal);  /* 定值最小值 */
                U32_TO_BYTES(aucBuf+8, pset->valDft.ulVal);  /* 定值默认值 */
                U32_TO_BYTES(aucBuf+12, pset->valStep.ulVal);  /* 定值修改步长 */
                U32_TO_BYTES(aucBuf+16, (uint32_t)0);  /* 保留*/
                i = write(iFd, aucBuf, 20);
                assert (i == 20);
            }
        }
    }

    /* 软件配置CRC */
    unSwCrc=unSwCrc_g;
    aucBuf[0]=LO8(unSwCrc);
    aucBuf[1]=HI8(unSwCrc);
    i = write(iFd, aucBuf, 2);
    assert (i == 2);

    /* 立即写入文件 */
    // ioctl (iFd, FIOFLUSH, 0);
    fsync(iFd);

    /* 长度计算 */
    temp_val = lseek(iFd, 0, SEEK_SET);
    file_lenth = lseek(iFd, 0, SEEK_END);
    file_lenth = file_lenth-temp_val;

    /* CRC计算 */
    if ((pucDat = malloc(file_lenth)) == NULL)
    {
        if (iFd >= 0)
        {
            /* 关闭已打开文件 */
            close(iFd);
        }

        return;
    }
    lseek(iFd, 0, SEEK_SET);
    i = read(iFd, pucDat, file_lenth);	/* 读数据 */
    assert (i == file_lenth);
    unCalcCrc = EP_CCITT_CRC16(pucDat, file_lenth, unCalcCrc);
    free(pucDat);

    /* 本文件CRC */
    aucBuf[0] = LO8(unCalcCrc);
    aucBuf[1] = HI8(unCalcCrc);
    i = write(iFd, aucBuf, 2);
    assert (i == 2);

    sts = FT_End_Update(EP_SET_RANGE_FILE, iFd);
    assert (sts == EP_SUCCESS);

    LOG_Write(LOG_OPRATE, "创建新的定值量程文件.\n", NULL);
}

/***************************压板(包括硬压板和软压板)访问函数接口定义*******************************/

/*     根据压板的逻辑标识，获得压板的相关信息
       参数：   strID  , 压板逻辑标识字符串
                pnRtNum，供返回该压板在压板集中的压板号
       返回值：    返回操作状态

                   EP_SUCCESS,操作成功
                   EP_BAD_DATA,找不到同名逻辑标识的保护压板
                   EP_NOT_INIT,找到多于1个的同名逻辑标识的保护压板
                   EP_SYS_ERR,其他原因导致的错误.
*/
EP_STATUS SCI_Init_Get_Yaban_Info(uint8_t *strID, int16_t *pnRtNum)
{
    SC_LINK_ITEM *plink;
    BOOL bFind;

    assert(strID && strlen(strID)<=MAX_ID_LEN);
    assert(pnRtNum);

    bFind=FALSE;
    for (plink=plink_g; !bFind && plink<plink_g+iLinkNum_g; plink++)
    {
        if (!strcmp(plink->aucId, strID))
        {
            if (!bFind)
            {
                bFind=TRUE;
                *pnRtNum=plink-plink_g;
            }
            else
            {
                LOG_Dbg_Msg("ERROR: repeat logic ID \"%s\" in link.\n",
                            (int)strID, 0, 0, 0, 0, 0);

                return EP_NOT_INIT;
            }
        }
    }

    if (bFind)
        return EP_SUCCESS;
    else
    {
        LOG_Dbg_Msg("ERROR: unresolved refrence to logic ID \"%s\" of link.\n",
                    (int)strID, 0, 0, 0, 0, 0);

        return EP_BAD_DATA;
    }
}

/* 设置压板有流判断标识.
 * Para:
 *     sNum, 压板序号.
 *     smvDataChn, 内序, 从0开始.
 * Return: EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS SCI_Set_Yaban_JgCurInfo(int16_t sNum, int smvDataChn)
{
    SC_LINK_ITEM *plink = NULL;
    RD_HW_AI_CH *phwai = NULL;

    if (sNum >= iLinkNum_g)
    {
        return EP_ERROR;
    }
    else
    {
        /* 找到对应硬件配置通道 */
        plink = plink_g+sNum;
        for (phwai = phwaich_g;
                phwai<phwaich_g+iHwAiChNum_g; phwai++)
        {
            if ((phwai->ucModCh == smvDataChn)
                    /* && IS_CURRENT_SIG(phwai->ucUnit) */
                    && (phwai->paimod != (&(aimodOpt_g[0])))
                    && (phwai->paimod != (&(aimodOpt_g[1]))))
            {
                if (plink->ulChnNum >= MAX_CHN_NUM_IN_BAY)
                {
                    assert(FALSE);

                    return EP_ERROR;
                }

                plink->bJgCurFlg = TRUE;
                plink->ulArrChnNo[plink->ulChnNum] = phwai-phwaich_g;
                plink->ulChnNum++;

                break;
            }
        }
    }

    return EP_SUCCESS;
}

/* 判断是否有流
 * Para:
 *     sNum, 压板序号.
 * Return:
 *     TRUE, or FALSE.
 */
BOOL SCI_Exsit_Current(int16_t sNum)
{
#define CURRENT_EXIST_SCALE 0.05 /* 有流比例 */

    SC_LINK_ITEM *plink = NULL;
    int32_t i = 0;
    RD_HW_AI_MEA arrAiMea[HCHNNUM];
    float fExistedBase = 0.0;
    int32_t iCurrentType;
    RD_HW_AI_MEA *pAiMea = NULL;

    if (sNum >= iLinkNum_g)
    {
        return FALSE;
    }
    else
    {
        plink = plink_g+sNum;
        if (!plink->bJgCurFlg)
        {
            return FALSE;
        }
        else
        {
            iCurrentType = App_GetAcMdType();
            if (iCurrentType == 0)
            {
                fExistedBase = 1.0*CURRENT_EXIST_SCALE;
            }
            else if (iCurrentType == 1)
            {
                fExistedBase = 5.0*CURRENT_EXIST_SCALE;
            }
            else
            {
                return FALSE;
            }

            /* 设置需要计算标识 */
            for (i = 0; i<plink->ulChnNum; i++)
            {
                phwaich_g[plink->ulArrChnNo[i]].bCalcMeaFlag = TRUE;
            }

            RD_Mea_Hw_AI(arrAiMea, FALSE);
            for (i = 0; i<plink->ulChnNum; i++)
            {
                pAiMea = arrAiMea+plink->ulArrChnNo[i];
                if (fabs(pAiMea->fRmsVal) >= fExistedBase)
                {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

/*     根据压板的压板号，访问压板当前值
       参数：
                nNum，该压板在压板表中的压板号
                pbRtYabanValue,供返回该压板当前值，真为压板投入，假为压板退出

                ulScnTime, 进行本次逻辑图扫描时的时刻（us计数器值）
       返回值：    返回操作状态

                   EP_SUCCESS,操作成功
                   EP_BAD_DATA,因压板号参数不对,导致的错误
                   EP_SYS_ERR,其他原因导致的错误.
        注意：  此函数逻辑图模块专用
*/

EP_STATUS SCI_Get_Yaban_Value(int16_t nNum, BOOL *pbRtYabanValue, uint32_t ulScnTime)
{
    SC_LINK_ITEM *plink;

    plink=plink_g+nNum;
    *pbRtYabanValue=plink->bNowVal;	/* 不进行单个扫描 */
    return EP_SUCCESS;
}

/***********************************************************************
* SCI_Update_Yaban_Value_Auto - Update the yaban state automatically.
*
* RETURNS: EP_SUCCESS, or EP_ERROR.
*
*/
EP_STATUS SCI_Update_Yaban_Value_Auto(
    int iTaskNo,	/* The priority of the scanning task. */
    uint32_t ulGrpScanDriveInterval,		/* Scanning Interval. */
    uint32_t ulScnAiCnt			/* 进行本次逻辑图扫描时的AI采样计数器值 */
)
{
    SC_LINK_ITEM *plink;
    VI_RUN_INFO *pinf;
    BOOL bSts = FALSE;
    uint8_t hwlinkmode;
    BOOL bFirstDi;
    BOOL bSecondDi;
    int16_t nNum;
    static uint32_t ulCnt=0;
    uint8_t ucLinkMode=0;
    BOOL bGooseDiNeedRefresh = FALSE;

    if(iTaskNo != 0)
    {
        return EP_SUCCESS;
    }

    if (ulCnt++ != 0)
    {
        if ((ulCnt*ulGrpScanDriveInterval) > (uiAiRate_g/5))
        {
            ulCnt = 0;
        }
        return EP_SUCCESS;
    }

    for(nNum=0; nNum<iLinkNum_g; nNum++)
    {
        plink=plink_g+nNum;

        if (ulTotalLinkMode_g & LINK_MODE_CUS)  /*如果总压板模式是定制模式，读取各具体压板的模式*/
            ucLinkMode=plink->aucMode;
        else    /*否则采用总压板模式*/
            ucLinkMode=ulTotalLinkMode_g;
        if(plink->LinkSwitchMode==1)
            ucLinkMode=LINK_MODE_HW;
        if(plink->LinkSwitchMode==2)
            ucLinkMode=LINK_MODE_SW;
        if(plink->LinkSwitchMode==3)
            ucLinkMode=LINK_MODE_AND;
        if(plink->LinkSwitchMode==4)
            ucLinkMode=LINK_MODE_OR;

        switch(ucLinkMode)
        {
            case LINK_MODE_HW:
                hwlinkmode=plink->HwLinkType;		/* 硬压板时的模式*/
                if( !(hwlinkmode&0x01))
                {
                    /* 置0为单开入*/
                    if(plink->pvHdDI == NULL) 		/* 当硬压板未定义，则认为该硬压板退出 */
                    {
                        bSts=FALSE;
                    }
                    else
                    {
                        bFirstDi=RD_Get_DI(plink->pvHdDI);
                        if(!(hwlinkmode&0x02))
                        {
                            /* 置0为正逻辑 */
                            bSts=bFirstDi;
                        }
                        else
                        {
                            /* 负逻辑 */
                            bSts=!bFirstDi;
                        }
                    }
                }
                else
                {
                    /* 置1为双开入，开入之间采用与逻辑 */
                    if(plink->pvHdDI == NULL) /* 当硬压板未定义，则认为该硬压板退出 */
                    {
                        bSts=FALSE;
                    }
                    else
                    {
                        bFirstDi=RD_Get_DI(plink->pvHdDI);
                        if(plink->pvHdSecondDI == NULL) /* 当硬压板未定义，则认为该硬压板退出 */
                        {
                            bSts=FALSE;
                        }
                        else
                        {
                            bSecondDi=RD_Get_DI(plink->pvHdSecondDI);
                            switch(hwlinkmode&0x06)
                            {
                                case 0:
                                    bSts=bFirstDi && bSecondDi;
                                    break;

                                case 2:
                                    bSts=(!bFirstDi) && bSecondDi;
                                    break;

                                case 4:
                                    bSts=bFirstDi && (!bSecondDi);
                                    break;

                                case 6:
                                    bSts=(!bFirstDi) && (!bSecondDi);
                                    break;

                                default:
                                    assert(FALSE);
                                    break;
                            }
                        }
                    }
                }
                break;

            case LINK_MODE_SW:
                bSts=plink->bSwVal;
                break;

            case LINK_MODE_AND:
                hwlinkmode=plink->HwLinkType;		/* 硬压板时的模式*/
                if( !(hwlinkmode&0x01))
                {
                    /* 置0为单开入*/
                    if(plink->pvHdDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                    {
                        bSts=FALSE;
                    }
                    else
                    {
                        bFirstDi=RD_Get_DI(plink->pvHdDI);
                        if(!(hwlinkmode&0x02))
                        {
                            /* 置0为正逻辑 */
                            bSts=bFirstDi;
                        }
                        else
                        {
                            /* 负逻辑 */
                            bSts=!bFirstDi;
                        }
                    }
                    bSts=(bSts&&plink->bSwVal);
                }
                else
                {
                    /* 置1为双开入 */
                    if(plink->pvHdDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                    {
                        bSts=FALSE;
                    }
                    else
                    {
                        bFirstDi=RD_Get_DI(plink->pvHdDI);
                        if(plink->pvHdSecondDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                        {
                            bSts=FALSE;
                        }
                        else
                        {
                            bSecondDi=RD_Get_DI(plink->pvHdSecondDI);
                            switch(hwlinkmode&0x06)
                            {
                                case 0:
                                    bSts=bFirstDi && bSecondDi && plink->bSwVal;
                                    break;

                                case 2:
                                    bSts=(!bFirstDi) && bSecondDi && plink->bSwVal;
                                    break;

                                case 4:
                                    bSts=bFirstDi && (!bSecondDi) && plink->bSwVal;
                                    break;

                                case 6:
                                    bSts=(!bFirstDi) && (!bSecondDi) && plink->bSwVal;
                                    break;

                                default:
                                    assert(FALSE);
                                    break;
                            }
                        }
                    }
                }
                break;

            case LINK_MODE_OR:
                hwlinkmode=plink->HwLinkType;		/* 硬压板时的模式*/
                if( !(hwlinkmode&0x01))
                {
                    /* 置0为单开入*/
                    if(plink->pvHdDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                    {
                        bSts=FALSE;
                    }
                    else
                    {
                        bFirstDi=RD_Get_DI(plink->pvHdDI);
                        if(!(hwlinkmode&0x02))
                        {
                            /* 置0为正逻辑 */
                            bSts=bFirstDi;
                        }
                        else
                        {
                            /* 负逻辑 */
                            bSts=!bFirstDi;
                        }

                    }
                    bSts=(bSts||plink->bSwVal);
                }
                else
                {
                    /* 置1为双开入 */
                    if(plink->pvHdDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                    {
                        bFirstDi=FALSE;
                    }
                    else
                    {
                        bFirstDi=RD_Get_DI(plink->pvHdDI);
                    }

                    if(plink->pvHdSecondDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                    {
                        bSecondDi=FALSE;
                    }
                    else
                    {
                        bSecondDi=RD_Get_DI(plink->pvHdSecondDI);
                    }

                    switch(hwlinkmode&0x06)
                    {
                        case 0:
                            bSts=bFirstDi || bSecondDi || plink->bSwVal;
                            break;

                        case 2:
                            bSts=(!bFirstDi) || bSecondDi || plink->bSwVal;
                            break;

                        case 4:
                            bSts=bFirstDi || (!bSecondDi) || plink->bSwVal;
                            break;

                        case 6:
                            bSts=(!bFirstDi) || (!bSecondDi) || plink->bSwVal;
                            break;

                        default:
                            assert(FALSE);
                            break;
                    }
                }
                break;

            default:
                return EP_ERROR;
                break;
        }

        if (bSts!=plink->bNowVal)
        {
            bGooseDiNeedRefresh = TRUE;
            plink->bNowVal=bSts;
            if(*(pLinkFirstAccessFlag_g+nNum))
            {
                /* 若不是首次访问的变位信息,认为是有效的,否则认为是无效 */
                pinf=VI_Run_Info_Wr_P();

                if(bViewModIsInit_g)				/* VI模块是否完成标志 */
                {
                    pinf->bViewModIsInit=TRUE;
                }
                else
                {
                    pinf->bViewModIsInit=FALSE;
                }

                pinf->type=LINK_CHG;

                pinf->msg.link.pcfg=plink;
                pinf->msg.link.ucCOT=1;
                pinf->msg.link.bSts=bSts;
                /* pinf->msg.link.ulTime=TM_Get_usCnt(); */	/* 原来的实现屏蔽,时间取实际时间,这样报告组织就不会乱掉 */
                /* pinf->msg.link.ulTime=ulScnTime; */


                pinf->msg.link.ulTime=RD_AI_Cnt_To_us(ulScnAiCnt);


                pinf->msg.link.unCh=nNum;

                VI_End_Wr_Run_Info();

                if(ENG_MODE == 0)
                {
                    static uint8_t aucLogInfo[256];

                    sprintf(aucLogInfo, "压板\"%s\"变位, 压板号是%d, 当前状态为%d. \n", plink->aucName, nNum, plink->bNowVal);

                    LOG_Write(LOG_RUN, aucLogInfo, NULL);
                }
                else if(ENG_MODE == 1)
                {
                    static uint8_t aucLogInfo[256];

                    sprintf(aucLogInfo, "switch \"%s\" changed, the current state is %d.\n", plink->aucName, plink->bNowVal);

                    LOG_Write(LOG_RUN, aucLogInfo, NULL);
                }
            }
        }

        if((!(*(pLinkFirstAccessFlag_g+nNum)))
                && (EP_Get_04CPU_Init_End_Flag()))
        {
            /* 只有在采样值首次刷新成功，这时DI有效，之后的首次访问将该标志置TRUE
            因为本函数除了在压板图元扫描时进行调用，
            在压板图元初始化时，也进行了调用，此时可能采样值首次刷新还没开始，DI值无效 */
            *(pLinkFirstAccessFlag_g+nNum)=TRUE;
        }
    }

    /* 设置刷新 */
    if(bGooseDiNeedRefresh)
    {
        HDL_SetGooseDiNeedRefresh(TRUE);
        RE_SetLogSetChgCnt();
    }

    return EP_SUCCESS;
}

/* Get link mode status.
 * Para
 *      ulTotalLinkMode: the value of the ulTotalLinkMode_g
 * Return value:
 *      link mode global var ulTotalLinkMode_g;
*/
EP_STATUS SC_Get_Link_Mode_Sts(
    uint16_t *ulTotalLinkMode		/* 总模式 */
)
{
    *ulTotalLinkMode=ulTotalLinkMode_g;
    return EP_SUCCESS;
}

/* Get link now status.
 * Parameters:
 *      iIdx, link index(from 0).
 *      pbRslt, to save return result.
 * Return value:
 *      EP_SUCCESS, read status OK.
 *      EP_BAD_DATA, invalid index(>=iLinkNum_g). */
EP_STATUS SC_Get_Link_Now_Sts(
    int iIdx,		/* 压板号 */
    BOOL *pbRslt			/* 输出值 */
)
{
    assert(iIdx<iLinkNum_g);
    assert(pbRslt);

    if (iIdx<iLinkNum_g)
    {
        *pbRslt=plink_g[iIdx].bNowVal;
        return EP_SUCCESS;
    }
    else
    {
        assert(FALSE);
        return EP_BAD_DATA;
    }
}

/* DQ:
 * Get hard link status.
 * Parameters:
 *      iIdx, link index(from 0).
 *      pbRslt, to save return result.
 * Return value:
 *      EP_SUCCESS, read status OK.
 *      EP_BAD_DATA, invalid index(>=iLinkNum_g)
 *      EP_PARM_ERR, link is not relevent. */
EP_STATUS SC_Get_Link_HW_Sts(
    int iIdx,		/* 压板号 */
    BOOL *pbRslt			/* 输出值 */
)
{
    SC_LINK_ITEM *plink;
    BOOL bSts;
    EP_STATUS ret;
    assert(iIdx<iLinkNum_g);
    assert(pbRslt);

    if (iIdx<iLinkNum_g)
    {
        plink=plink_g+iIdx;
        if(plink->pvHdDI==NULL)
            return EP_PARM_ERR;
        else
            bSts=RD_Get_DI(plink->pvHdDI);
        *pbRslt=bSts;
        ret=EP_SUCCESS;
    }
    else
    {
        assert(FALSE);
        ret=EP_BAD_DATA;
    }
    return ret;
}

/* DQ:
 * Get soft link status.
 * Parameters:
 *      iIdx, link index(from 0).
 *      pbRslt, to save return result.
 * Return value:
 *      EP_SUCCESS, read status OK.
 *      EP_BAD_DATA, invalid index(>=iLinkNum_g). */
EP_STATUS SC_Get_Link_SW_Sts(
    int iIdx,
    BOOL *pbRslt
)
{
    assert(iIdx<iLinkNum_g);
    assert(pbRslt);

    if (iIdx<iLinkNum_g)
    {
        *pbRslt=plink_g[iIdx].bSwVal;
        return EP_SUCCESS;
    }
    else
    {
        assert(FALSE);
        return EP_BAD_DATA;
    }
}

/* Get link status.
 * Parameters:
 *      iIdx, link index(from 0).
 *      pbRslt, to save return result.
 * Return value:
 *      EP_SUCCESS, read status OK.
 *      EP_BAD_DATA, invalid index(>=iLinkNum_g). */
EP_STATUS SC_Get_Link_Sts(int iIdx, BOOL *pbRslt)
{
    assert(iIdx<iLinkNum_g);
    assert(pbRslt);

    if (iIdx<iLinkNum_g)
    {
        *pbRslt=plink_g[iIdx].bNowVal;
        return EP_SUCCESS;
    }
    else
    {
        assert(FALSE);
        return EP_BAD_DATA;
    }
}

/* Get setting page attribution.
 * Parameters:
 *      iIdx, setting page(0 means internal setting).
 * Return value:
 *      Pointer to the setting page attribution structure.
 *      NULL if iIdx is invalid(>=iSetPgNum_g). */
const SC_SET_PAGE *SC_Get_Set_Pg_Attr(int iIdx)
{
    if (iIdx<iSetPgNum_g)
        return psetpg_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/* Get link page attribution.
 * Parameters:
 *      iIdx, link page number.
 * Return value:
 *      Pointer to the link page attribution structure.
 *      NULL if iIdx is invalid(>=iSetPgNum_g). */
const SC_LINK_PAGE *SC_Get_Link_Pg_Attr(int iIdx)
{
    if (iIdx<iLkPgNum_g)
        return plkpg_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/* Get link attribution.
 * Parameters:
 *      iIdx, index of the link(from 0).
 * Return value:
 *      Pointer to the link attribution structure.
 *      NULL if iIdx is invalid(>=iLinkNum_g). */
const SC_LINK_ITEM *SC_Get_Link_Attr(int iIdx)
{
    if (iIdx<iLinkNum_g)
        return plink_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}


/* Initialize protect function config.
 * Parameters:
 *      strLgcFile, logic config file name.
 * Return value:
 *      EP_SUCCESS, parse config file OK.
 *      EP_CFG_ERR, config file format error.
 *	    EP_BUF_ERR, can't malloc enough memory.
 */
EP_STATUS SC_Init_Func_Cfg(const uint8_t *strLgcFile)
{
    int iFd;
    int i;
    int iIdx;
    uint8_t aucBuf[100];
    int iStrLen;
    uint32_t uiProLen;

    if ((iFd = open(EP_LGC_CFG_FILE, O_RDONLY, 0)) == ERROR)
        return EP_CFG_ERR;

    lseek(iFd, -4, SEEK_END);

    if (read(iFd, aucBuf, 4) != 4 ||
            aucBuf[0] != 0x39 || aucBuf[1] != 0 || aucBuf[2] != 0 || aucBuf[3] != 0xC6)
    {
        close(iFd);
        return EP_CFG_ERR;
    }

    lseek(iFd, 0, SEEK_SET);

    if (read(iFd, aucBuf, 9) != 9 ||
            aucBuf[0] != 0x33 || aucBuf[1] != 0 || aucBuf[2] != 0 || aucBuf[3] != 0xCC)
    {
        close(iFd);
        return EP_CFG_ERR;
    }

    SI_SysVer_g.unCfgProtocolVer = U8_TO_U16(aucBuf[5], aucBuf[4]);
    SI_SysVer_g.unLogicPrgVer = U8_TO_U16(aucBuf[7], aucBuf[6]);
    if (aucBuf[8])
    {
        assert (aucBuf[8] <= MAX_ID_LEN);
        i = read(iFd, SI_SysVer_g.aucSysVer, aucBuf[8]);
        assert (i == aucBuf[8]);
    }

    if (read(iFd, aucBuf, 3) != 3 )
    {
        close(iFd);
        return EP_CFG_ERR;
    }
    SI_SysVer_g.unLogicVer = U8_TO_U16(aucBuf[1], aucBuf[0]);

    if (aucBuf[2])
    {
        assert (aucBuf[2] <= MAX_ID_LEN);
        i = read(iFd, aucEqName_g, aucBuf[2]);
        assert (i == aucBuf[2]);
    }

    i = read(iFd, aucBuf, 5);
    assert (i == 5);

    SI_SysVer_g.unRlsCRC = U8_TO_U16(aucBuf[1], aucBuf[0]);
    iSubLgcNum_g = aucBuf[4];

    if ((psublgc_g = calloc(iSubLgcNum_g, sizeof(*psublgc_g))) == NULL)
    {
        close(iFd);
        return EP_BUF_ERR;
    }

    for (iIdx=0; iIdx<iSubLgcNum_g; iIdx++)
    {
        i = read(iFd, aucBuf, 5);
        assert (i == 5);
        uiProLen = BYTES_TO_U32(aucBuf);
        iStrLen = aucBuf[4];
        if (iStrLen)
        {
            assert (iStrLen <= MAX_ID_LEN);
            i = read(iFd, psublgc_g[iIdx].aucName, iStrLen);
            assert (i == iStrLen);
        }
        i = read(iFd,aucBuf,3);
        assert (i == 3);

        psublgc_g[iIdx].usInterval = U8_TO_U16(aucBuf[1], aucBuf[0]);

        if (aucBuf[2]&0x02)
            psublgc_g[iIdx].bRun = FALSE;
        else
            psublgc_g[iIdx].bRun = TRUE;

        i = uiProLen-1-iStrLen-3;
        lseek(iFd, i, SEEK_CUR);
    }

    close(iFd);

    return EP_SUCCESS;
}

/* 获取扫描任务最小扫描周期点数.
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
uint16_t SC_GetTaskMinPeriod (void)
{
    int iIdx;
    uint16_t usInterval = 0xFFFF;

    for (iIdx = 0; iIdx<iSubLgcNum_g; iIdx++)
    {
        if (usInterval>psublgc_g[iIdx].usInterval)
        {
            usInterval = psublgc_g[iIdx].usInterval;
        }
    }

    return usInterval;
}

/* Get sub-logic attribution.
 * Parameters:
 *      iIdx, index of the sub-logic(from 0).
 * Return value:
 *      Pointer to the sub-logic attribution structure.
 *      NULL if iIdx is invalid(>=iSubLgcNum_g). */
const SC_SUB_LGC_ITEM *SC_Get_Sub_Lgc_Attr(int iIdx)
{
    if (iIdx<iSubLgcNum_g)
        return psublgc_g+iIdx;
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/* Check protect setting after power on.
 * Parameters:
 *      None.
 * Return value:
 *      EP_SUCCESS, everything is OK.
 *      EP_FILE_ERR, some system file error.
 *      EP_CFG_ERR, setting file not valid.
 */
EP_STATUS SC_Chk_Set(void)
{
    uint8_t aucBuf[FULL_NAME_LEN+1];
    uint8_t *puc;
    int i;
    int iFd;
    BOOL bSetOK;
    STATUS vxsts;
    EP_STATUS ChkResult;
    uint16_t unComCrc;
    uint8_t aucTemp[32];
    BOOL bError =FALSE;
    char back_filename[100];
    char new_tmp_filename[100];
    BOOL backfileisexisted = FALSE;


    ChkResult = EP_SUCCESS;
    scinfo_g.iNextWorkArea = -1;  /* 初始化时通过逻辑图整定定值初始化为-1 */
    if ((i = FT_Rd_Sys_INI("[SYSTEM]", "SetArea", aucBuf, 30)) == 1)
        scinfo_g.iMaxSetArea = atoi(aucBuf);
    else if (i == 0)
    {
        if (ENG_MODE == 0)
        {
            LOG_Write(LOG_KERNEL, "因[SYSTEM] SetArea值读取失败，创建新的系统INI文件.\n", NULL);
        }
        else if (ENG_MODE == 1)
        {
            LOG_Write(LOG_KERNEL, "Because of failing ro read [SYSTEM] SetArea, create new ini file.\n", NULL);
        }

        FT_New_SYS_INI_File();
    }
    if (!scinfo_g.iMaxSetArea || scinfo_g.iMaxSetArea>255)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM |ER_LOCK | ER_REPORT,
                       "最大定值区号无效(%d)\n",
                       scinfo_g.iMaxSetArea, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM  |ER_LOCK| ER_REPORT,
                       "Invalid maximum setting sector No(%d)\n",
                       scinfo_g.iMaxSetArea, 0);
        }
        scinfo_g.iMaxSetArea = 32;
        ChkResult = EP_FILE_ERR;
    }

    if(b01IniChanged==FALSE)/*能否放在保护启动之后?这里影响保护启动时间*/
    {
        FileCRC_Check();
    }


    if (SC_Rd_Func_Sts() != EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,  ER_ALARM |ER_LOCK | ER_REPORT,
                       "保护功能投退状态文件无效\n",
                       0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,  ER_ALARM |ER_LOCK | ER_REPORT,
                       "Protection enable/disable state file invalid\n",
                       0, 0);
        }

        if (FT_Is_File(EP_FUNC_STS_FILE))
        {
            vxsts = remove(EP_FUNC_STS_FILE);
            assert (vxsts == OK);
        }

        ChkResult = EP_FILE_ERR;
    }

    if (RD_Rd_Force_DI()!=EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM  |ER_LOCK| ER_REPORT,
                       "DI强制状态文件无效\n",
                       0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM  |ER_LOCK| ER_REPORT,
                       "Invalid DI forced state file\n",
                       0, 0);
        }

        if (FT_Is_File(EP_DI_STS_FILE))
        {
            vxsts = remove(EP_DI_STS_FILE);
            assert (vxsts == OK);

            /* 删除后生成缺省文件 */
            SI_New_DI_File();
        }

        ChkResult = EP_FILE_ERR;
    }

    if (SC_Rd_Sw_Link()!=EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,  ER_ALARM |ER_LOCK | ER_REPORT,
                       "压板状态文件无效\n",
                       0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,  ER_ALARM  |ER_LOCK| ER_REPORT,
                       "Invalid switch state file\n",
                       0, 0);
        }

        if (FT_Is_File(EP_LINK_STS_FILE))
        {
            vxsts = remove(EP_LINK_STS_FILE);
            assert (vxsts == OK);

            /* 删除后生成缺省文件 */
            SI_New_Link_File();
        }

        ChkResult = EP_FILE_ERR;

    }

    if (SC_Rd_Link_Mode_File() != EP_SUCCESS )
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,  ER_ALARM  |ER_LOCK| ER_REPORT,
                       "压板模式文件无效\n",
                       0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,  ER_ALARM  |ER_LOCK| ER_REPORT,
                       "Invalid switch mode file\n",
                       0, 0);
        }

        if (FT_Is_File(EP_LINK_MODE_FILE))
        {
            vxsts = remove(EP_LINK_MODE_FILE);
            assert (vxsts == OK);
        }

        ChkResult = EP_FILE_ERR;

    }

    /* 首次获取开入状态,
     * 用于更新硬开入压板
     */
    RD_Get_Org_DI();

    SC_Updt_Link();

    bSetOK = FALSE;
    if ((iFd = open(EP_INNER_SET_FILE, O_RDONLY, 0)) != ERROR)
    {
        if (SC_Chg_Inner_Set(iFd) == EP_SUCCESS)
            bSetOK = TRUE;
        vxsts = close(iFd);
        assert (vxsts == OK);
    }

    if (!bSetOK)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SET_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                       "内部定值无效\n", 0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SET_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                       "Internal setting invalid\n", 0, 0);
        }

        if (FT_Is_File(EP_INNER_SET_FILE))
        {
            /* 如果无效，且存在则删除 */
            remove(EP_INNER_SET_FILE);
        }

        ChkResult = EP_FILE_ERR;
    }

    /* 在EDP01上也开放参数定值 */
    bSetOK = FALSE;
    if ((iFd = open(EP_CK_SET_FILE, O_RDONLY, 0)) != ERROR)
    {
        if (SC_Chg_CK_Set(iFd) == EP_SUCCESS)
            bSetOK = TRUE;
        vxsts = close(iFd);
        assert (vxsts == OK);
    }

    if (!bSetOK)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SET_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                       "参数定值无效\n", 0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SET_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                       "Parameter setting invalid\n", 0, 0);
        }

        if (FT_Is_File(EP_CK_SET_FILE))
        {
            /* 如果无效，且存在则删除 */
            remove(EP_CK_SET_FILE);
        }

        ChkResult = EP_FILE_ERR;
    }

    strcpy(aucBuf, (EP_SET_AREA_DIR "/area00.dza"));
    /* Point to area number(00). */
    puc = aucBuf+strlen(aucBuf)-6;

    bSetOK = FALSE;
    for (i=SET_AREA_START_NO; i<scinfo_g.iMaxSetArea; i++)
    {
        puc[0] = (i/16)+'0';
        if (puc[0]>'9')
            puc[0] += ('a'-'9'-1);

        puc[1] = (i%16)+'0';
        if (puc[1]>'9')
            puc[1] += ('a'-'9'-1);

        if ((iFd = open(aucBuf, O_RDONLY, 0))==ERROR)
        {
            if (CrcInfo_g.bAreaCrcWrFlag[i])
            {
                strcpy(back_filename,"");
                strcpy(new_tmp_filename,"");
                Bak_File_Name(back_filename,aucBuf,".old");
                Bak_File_Name(new_tmp_filename,aucBuf,".bak");
                if(FT_Is_File(new_tmp_filename))
                {
                    backfileisexisted = TRUE;
                    rename(new_tmp_filename, aucBuf);
                }
                else if(FT_Is_File(back_filename))
                {
                    backfileisexisted = TRUE;
                    rename(back_filename, aucBuf);
                }

            }
        }
        else
        {
            backfileisexisted = TRUE;
            vxsts = close(iFd);
            assert (vxsts == OK);
        }

        if(backfileisexisted)
        {
            backfileisexisted = FALSE;
            if ((iFd = open(aucBuf, O_RDONLY, 0))!=ERROR)
            {
                scinfo_g.aucSetAreaFg[i] |= SET_HAVE_FILE;	/* 存在该定值区*/

                if (SC_Is_Valid_Set(iFd))
                {
                    scinfo_g.aucSetAreaFg[i] |= SET_VALID; /* 是否有效 */

                    if (i == scinfo_g.iWorkSetArea)
                        bSetOK = TRUE;
                }
                else
                {
                    static uint8_t aucLogInfo[256];

                    if (ENG_MODE == 0)
                    {
                        sprintf(aucLogInfo, "无效定值区: 定值区文件(number=%d)无效.\n", i);
                    }
                    else if (ENG_MODE == 1)
                    {
                        sprintf(aucLogInfo, "Invalid setting sector: Invalid setting sector file(No. %d).\n", i);
                    }

                    LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
                    /* ChkResult = EP_FILE_ERR; */	/* 不作为错误返回 */
                }
                vxsts = close(iFd);
                assert (vxsts == OK);
            }
        }
    }

    if (!bSetOK)
    {
        scinfo_g.iRealWorkSetArea = 0xFF;		/* 无实际有效定值区 */
        /*	if (ENG_MODE == 0)
        	{
          		ER_Set_Err(EV_SECT_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
           			"定值区%d无效\n",
           			scinfo_g.iWorkSetArea, 0);
        	}
        	else if (ENG_MODE == 1)
        	{
            	ER_Set_Err(EV_SECT_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                	"Invalid setting sector %d\n",
              		scinfo_g.iWorkSetArea, 0);
        	}
        */
        ChkResult = EP_FILE_ERR;
    }
    else
    {
        bSetIsValid_g = TRUE;
    }

    if (SC_Updt_Work_Set() != EP_SUCCESS)
    {
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SECT_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                       "运行定值区%d无效\n",
                       scinfo_g.iWorkSetArea, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SECT_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                       "Active SG %d invalid\n",
                       scinfo_g.iWorkSetArea, 0);
        }
        ChkResult = EP_FILE_ERR;
    }


    /*考虑是否需要放在保护启动之后 ? 之前影响保护启动时间 之后不合理 */

    if(b01IniChanged==TRUE)
    {
        if(FT_Is_File(EP_FUNC_STS_FILE))
        {
            unComCrc =0;
            unComCrc=FT_File_CRC16(EP_FUNC_STS_FILE);
            if(FT_Wr_INI_CRC("[CRC]",CRC_ITEM_FUNC,unComCrc)==EP_ERROR)
            {
                bError =TRUE;
                goto error;
            }
        }

#if 0

        /* 修改为文件自身校验 */
        if(FT_Is_File(EP_DI_STS_FILE))
        {
            unComCrc =0;
            unComCrc=FT_File_CRC16(EP_DI_STS_FILE);
            if(FT_Wr_INI_CRC("[CRC]",CRC_ITEM_DIFDORCE,unComCrc)==EP_ERROR)
            {
                bError =TRUE;
                goto error;
            }
        }
#endif

#if 0
        /* 修改为文件自身校验 */
        if(FT_Is_File(EP_LINK_STS_FILE))
        {
            unComCrc =0;
            unComCrc=FT_File_CRC16(EP_LINK_STS_FILE);
            if(FT_Wr_INI_CRC("[CRC]",CRC_ITEM_LINKSTATS,unComCrc)==EP_ERROR)
            {
                bError =TRUE;
                goto error;
            }
        }
#endif

        if(FT_Is_File(EP_LINK_MODE_FILE))
        {
            unComCrc =0;
            unComCrc=FT_File_CRC16(EP_LINK_MODE_FILE);
            if(FT_Wr_INI_CRC("[CRC]",CRC_ITEM_LINKMODE,unComCrc)==EP_ERROR)
            {
                bError =TRUE;
                goto error;
            }
        }
        if(FT_Is_File(EP_INNER_SET_FILE))
        {
            unComCrc =0;
            unComCrc=FT_File_CRC16(EP_INNER_SET_FILE);
            if(FT_Wr_INI_CRC("[CRC]",CRC_ITEM_NBSET,unComCrc)==EP_ERROR)
            {
                bError =TRUE;
                goto error;
            }
        }
        if(FT_Is_File(EP_CK_SET_FILE))
        {
            unComCrc =0;
            unComCrc=FT_File_CRC16(EP_CK_SET_FILE);
            if(FT_Wr_INI_CRC("[CRC]",CRC_ITEM_CKSET,unComCrc)==EP_ERROR)
            {
                bError =TRUE;
                goto error;
            }
        }
        if(FT_Is_File(EP_AI_GAIN_FILE))
        {
            unComCrc =0;
            unComCrc=FT_File_CRC16(EP_AI_GAIN_FILE);
            if(FT_Wr_INI_CRC("[CRC]",CRC_ITEM_HDCOF,unComCrc)==EP_ERROR)
            {
                bError =TRUE;
                goto error;
            }
        }
        if(FT_Is_File(EP_CL_GAIN_FILE))
        {
            unComCrc =0;
            unComCrc=FT_File_CRC16(EP_CL_GAIN_FILE);
            if(FT_Wr_INI_CRC("[CRC]",CRC_ITEM_CLCOF,unComCrc)==EP_ERROR)
            {
                bError =TRUE;
                goto error;
            }
        }
        strcpy(aucBuf, (EP_SET_AREA_DIR "/area00.dza"));
        puc = aucBuf+strlen(aucBuf)-6;
        for(i=0; i<32; i++)
        {
            puc[0] = (i/16)+'0';
            if (puc[0]>'9')
                puc[0] += ('a'-'9'-1);

            puc[1] = (i%16)+'0';
            if (puc[1]>'9')
                puc[1] += ('a'-'9'-1);
            if(FT_Is_File(aucBuf))
            {
                if(scinfo_g.aucSetAreaFg[i]&SET_VALID)
                {
                    memset(aucTemp,0,sizeof(aucTemp));
                    sprintf(aucTemp,"%s%d",CRC_ITEM_AREA,i);
                    unComCrc =0;
                    unComCrc=FT_File_CRC16(aucBuf);
                    if(FT_Wr_INI_CRC("[CRC]", aucTemp,unComCrc)==EP_ERROR)
                    {
                        bError =TRUE;
                        goto error;
                    }
                }
            }
        }
    }
error:
    if(bError ==TRUE)
    {
        ChkResult =EP_ERROR;
        if (ENG_MODE == 0)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                       "程序校验文件写入失败\n", 0, 0);
        }
        else if (ENG_MODE == 1)
        {
            ER_Set_Err(EV_SOFTWARE_CHECK_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                       "EDP01.ini write error\n", 0, 0);
        }
    }


    return ChkResult;
}

/* 更新定值量程后检查定值的有效性
 * Parameters:
 *      None.
 * Return value:
 *      None.
 */
void SC_Chk_Range_All_Valid(void)
{
    EP_STATUS ChkResult = EP_SUCCESS;
    uint8_t aucLogInfo[128];
    int iFd;
    STATUS vxsts;

    /* 检查保护定值区的有效性 */
    ChkResult = SC_Chk_Range_Set_Valid();
    LOG_Dbg_Msg("保护定值区有效性ChkResult=%d \n",ChkResult,0,0,0,0,0);

    /* 检查内部定值的有效性 */
    if ((iFd = open(EP_INNER_SET_FILE, O_RDONLY, 0)) != ERROR)
    {
        ChkResult = SC_Inner_Set_Is_Valid(iFd);

        vxsts = close(iFd);
        assert (vxsts == OK);

        if(ChkResult != EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SET_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                           "内部定值无效\n", 0, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SET_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                           "Internal setting invalid\n", 0, 0);
            }
        }
    }
    else
    {
        ChkResult = EP_FILE_ERR;
        sprintf(aucLogInfo, "打开内部定值文件失败.\n");
        LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
    }
    LOG_Dbg_Msg("内部定值文件有效性ChkResult=%d \n",ChkResult,0,0,0,0,0);

    /* 检查参数定值的有效性 */
    if ((iFd = open(EP_CK_SET_FILE, O_RDONLY, 0)) != ERROR)
    {
        ChkResult = SC_CK_Is_Valid(iFd);

        vxsts = close(iFd);
        assert (vxsts == OK);

        if (ChkResult != EP_SUCCESS)
        {
            if (ENG_MODE == 0)
            {
                ER_Set_Err(EV_SET_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                           "参数定值无效\n", 0, 0);
            }
            else if (ENG_MODE == 1)
            {
                ER_Set_Err(EV_SET_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                           "Parameter setting invalid\n", 0, 0);
            }
        }
    }
    else
    {
        ChkResult = EP_FILE_ERR;
        sprintf(aucLogInfo, "打开测控定值文件失败.\n");
        LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
    }
    LOG_Dbg_Msg("测控定值文件有效性ChkResult=%d \n",ChkResult,0,0,0,0,0);
}

/* 更新定值量程后检查保护定值区的有效性
 * Parameters:
 *      None.
 * Return value:
 *      EP_SUCCESS, everything is OK.
 *      EP_FILE_ERR, some system file error.
 *      EP_CFG_ERR, setting file not valid.
 */
EP_STATUS SC_Chk_Range_Set_Valid(void)
{
    uint8_t aucBuf[FULL_NAME_LEN+1];
    uint8_t aucLogInfo[128];
    uint8_t *puc;
    STATUS vxsts;
    EP_STATUS ChkResult = EP_SUCCESS;
    int i;
    int iFd;
    BOOL backfileisexisted = FALSE;
    uint8_t back_filename[100];
    uint8_t new_tmp_filename[100];

    SC_Set_AreaSet_ChkFlg(TRUE);   /* 设置保护定值正在校验标识为TRUE */

    strcpy(aucBuf, (EP_SET_AREA_DIR "/area00.dza"));
    /* Point to area number(00). */
    puc = aucBuf+strlen(aucBuf)-6;

    for (i=SET_AREA_START_NO; i<scinfo_g.iMaxSetArea; i++)
    {
        puc[0] = (i/16)+'0';
        if (puc[0]>'9')
            puc[0] += ('a'-'9'-1);

        puc[1] = (i%16)+'0';
        if (puc[1]>'9')
            puc[1] += ('a'-'9'-1);

        if(FT_Is_File(aucBuf)==FALSE)  /* 该定值区不存在 */
        {
            scinfo_g.aucSetAreaFg[i] &= ~(SET_HAVE_FILE | SET_VALID);
            continue;
        }

        if ((iFd = open(aucBuf, O_RDONLY, 0))==ERROR)
        {
            if (CrcInfo_g.bAreaCrcWrFlag[i])
            {
                strcpy(back_filename,"");
                strcpy(new_tmp_filename,"");
                Bak_File_Name(back_filename,aucBuf,".old");
                Bak_File_Name(new_tmp_filename,aucBuf,".bak");
                if(FT_Is_File(new_tmp_filename))
                {
                    backfileisexisted = TRUE;
                    rename(new_tmp_filename, aucBuf);
                }
                else if(FT_Is_File(back_filename))
                {
                    backfileisexisted = TRUE;
                    rename(back_filename, aucBuf);
                }
            }
        }
        else
        {
            backfileisexisted = TRUE;
            vxsts = close(iFd);
            assert (vxsts == OK);
        }

        if(backfileisexisted)
        {
            backfileisexisted = FALSE;
            if ((iFd = open(aucBuf, O_RDONLY, 0))!=ERROR)
            {
                scinfo_g.aucSetAreaFg[i] |= SET_HAVE_FILE;	/* 存在该定值区*/

                if (SC_Is_Valid_Set(iFd)==FALSE)
                {
                    scinfo_g.aucSetAreaFg[i] &= ~SET_VALID; /* 置定值无效 */

                    if (ENG_MODE == 0)
                    {
                        sprintf(aucLogInfo, "无效定值区: 定值区文件(number=%d)无效.\n", i);
                    }
                    else if (ENG_MODE == 1)
                    {
                        sprintf(aucLogInfo, "Invalid setting sector: Invalid setting sector file(No. %d).\n", i);
                    }

                    LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
                    /* ChkResult = EP_FILE_ERR; */	/* 不作为错误返回 */

                    if(i == scinfo_g.iWorkSetArea)
                    {
                        if (ENG_MODE == 0)
                        {
                            ER_Set_Err(EV_SECT_ERR, ER_ALARM | ER_LOCK | ER_REPORT,
                                       "运行定值区%d无效\n",
                                       scinfo_g.iWorkSetArea, 0);
                        }
                        else if (ENG_MODE == 1)
                        {
                            ER_Set_Err(EV_SECT_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                                       "Active SG %d invalid\n",
                                       scinfo_g.iWorkSetArea, 0);
                        }
                    }
                    ChkResult = EP_CFG_ERR;
                }
                else
                {
                    scinfo_g.aucSetAreaFg[i] |= SET_VALID;	/* 该定值区有效 */
                }

                vxsts = close(iFd);
                assert (vxsts == OK);
            }
            else
            {
                scinfo_g.aucSetAreaFg[i] &= ~(SET_HAVE_FILE | SET_VALID);

                sprintf(aucLogInfo, "打开定值区文件(number=%d)失败.\n", i);
                LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
                ChkResult = EP_FILE_ERR;
            }
        }
        else
        {
            scinfo_g.aucSetAreaFg[i] &= ~(SET_HAVE_FILE | SET_VALID);

            sprintf(aucLogInfo, "打开定值区文件(number=%d)失败.\n", i);
            LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
            ChkResult = EP_FILE_ERR;
        }
    }

    SC_Set_AreaSet_ChkFlg(FALSE);   /* 设置保护定值正在校验标识为FALSE */

    return ChkResult;
}

static EP_STATUS SC_Rd_Func_Sts(void)
{
    uint8_t *pucBuf;
    uint32_t ulLen;
    uint8_t *puc;
    uint8_t aucTemp[64];
    EP_STATUS stsRet;
    SC_SUB_LGC_ITEM *psublgc;

    scinfo_g.iRealWorkSetArea=0xFF;		/* 默认无实际运行有效定值区 */
    if ((pucBuf=FT_File_To_Mem(EP_FUNC_STS_FILE, &ulLen))==NULL)
        return EP_FILE_ERR;

    if (ulLen!=(iSubLgcNum_g+3)*81 || memcmp(pucBuf, pucFuncFileHead_g, 81))
    {
        EP_free(pucBuf);
        return EP_FILE_ERR;
    }

    stsRet=EP_SUCCESS;

    /* "__ReportSN__" line is not used now. */

    puc=pucBuf+2*81;
    if (!memcmp(puc, "__RunSetArea__"
                "                                                  ", 64))
    {
        puc+=64;
        if (puc[16]=='\n')
        {
            puc[16]='\0';
            scinfo_g.iWorkSetArea=strtol(puc, NULL, 16);
            scinfo_g.iRealWorkSetArea=scinfo_g.iWorkSetArea;
            if (scinfo_g.iWorkSetArea<SET_AREA_START_NO ||
                    scinfo_g.iWorkSetArea>scinfo_g.iMaxSetArea)
            {
                scinfo_g.iRealWorkSetArea=0xFF;
                stsRet=EP_FILE_ERR;
            }
        }
        else
            stsRet=EP_FILE_ERR;
    }
    else
        stsRet=EP_FILE_ERR;

    puc=pucBuf+3*81;
    for (psublgc=psublgc_g; psublgc<psublgc_g+iSubLgcNum_g; psublgc++)
    {
        SI_Tag_Str_Cpy(aucTemp, psublgc->aucName, 64);
        if (!memcmp(puc, aucTemp, 64))
        {
            puc+=64;
            if (!memcmp(puc, "RUN             \n", 17))
                psublgc->bRun=TRUE;
            else
            {
                if (memcmp(puc, "EXIT            \n", 17))
                    stsRet=EP_FILE_ERR;
                psublgc->bRun=FALSE;
            }
            puc+=17;
        }
        else
        {
            stsRet=EP_FILE_ERR;
            puc+=81;
        }
    }

    EP_free(pucBuf);
    return stsRet;
}

/* 检查压板文件是否有CRC
 * Para:
 *     pbCrc, 检查结果指针.
 * Return:
 *     EP_SUCCESS, or EP_ERROR.
 */
EP_STATUS SC_Judge_Sw_Link(BOOL *pbCrc)
{
    uint8_t *pucBuf = NULL;
    uint32_t ulLen;

    if (pbCrc == NULL)
    {
        return EP_FILE_ERR;
    }

    if ((pucBuf = FT_File_To_Mem(EP_LINK_STS_FILE, &ulLen)) == NULL)
    {
        return EP_FILE_ERR;
    }

    /* 增加CRC项, 考虑兼容处理 */
    if (((ulLen != ITEM_LEN*(iLinkNum_g+2)) && (ulLen != ITEM_LEN*(iLinkNum_g+3)))
            || memcmp(pucBuf, pucLinkFileHead_g, ITEM_LEN))
    {
        EP_free(pucBuf);

        return EP_FILE_ERR;
    }

    /* 根据是否有CRC项进行判断
     */
    if (ulLen == ITEM_LEN*(iLinkNum_g+2))
    {
        *pbCrc = FALSE;
    }
    else
    {
        *pbCrc = TRUE;
    }

    EP_free(pucBuf);

    return EP_SUCCESS;
}

static EP_STATUS SC_Rd_Sw_Link(void)
{
    uint8_t *pucBuf;
    uint32_t ulLen;
    uint8_t *puc;
    uint8_t aucTemp[ITEM_NAME_LEN];
    EP_STATUS stsRet;
    SC_LINK_ITEM *plink;
    uint16_t usCrc = 0;
    uint16_t usCalcCrc = 0;

    if ((pucBuf=FT_File_To_Mem(EP_LINK_STS_FILE, &ulLen))==NULL)
    {
        LOG_Write(LOG_KERNEL, "压板文件读取出错.\n", NULL);

        return EP_FILE_ERR;
    }

    /* 增加CRC项 */
    if (ulLen!=ITEM_LEN*(iLinkNum_g+3) || memcmp(pucBuf, pucLinkFileHead_g, ITEM_LEN))
    {
        EP_free(pucBuf);
        LOG_Write(LOG_KERNEL, "压板文件格式出错.\n", NULL);

        return EP_FILE_ERR;
    }

    /* CRC项自身不计算 */
    usCalcCrc = EP_CCITT_CRC16(pucBuf, ITEM_LEN*(iLinkNum_g+2), usCalcCrc);

    stsRet=EP_SUCCESS;

    puc=pucBuf+ITEM_LEN;
    if (!memcmp(puc, SW_CUR_LINK_MODE, ITEM_NAME_LEN))
    {
        puc+=ITEM_NAME_LEN;
        if (!memcmp(puc, SWLINK_NAME, ITEM_VALUE_LEN))
        {
            scinfo_g.curLinkMode=0x55;
            uiEdpStatus_g|=USE_SW_LINK;
            uiEdpStatus_g &= ~USE_HW_LINK;
        }
        else if(!memcmp(puc, HWLINK_NAME, ITEM_VALUE_LEN))
        {
            scinfo_g.curLinkMode=0x5A;
            uiEdpStatus_g|=USE_HW_LINK;
            uiEdpStatus_g &= ~USE_SW_LINK;
        }
        else
        {
            if (memcmp(puc, SHWLINK_NAME, ITEM_VALUE_LEN))
                stsRet=EP_FILE_ERR;
            scinfo_g.curLinkMode=0xA5;
            uiEdpStatus_g|=USE_SW_LINK;
            uiEdpStatus_g |=USE_HW_LINK;
        }
    }
    else
        stsRet=EP_FILE_ERR;

    puc=pucBuf+2*ITEM_LEN;

    for (plink=plink_g; plink<plink_g+iLinkNum_g; plink++)
    {
        SI_Tag_Str_Cpy(aucTemp, plink->aucABRV, ITEM_NAME_LEN);
        if (!memcmp(puc, aucTemp, ITEM_NAME_LEN))
        {
            puc+=ITEM_NAME_LEN;
            if (!memcmp(puc, SW_CLOSE"\n", ITEM_VALUE_LEN))
                plink->bSwVal=TRUE;
            else
            {
                if (memcmp(puc, SW_OPEN"\n", ITEM_VALUE_LEN))
                    stsRet=EP_FILE_ERR;

                plink->bSwVal=FALSE;
            }
            puc+=ITEM_VALUE_LEN;
        }
        else
        {
            stsRet=EP_FILE_ERR;
            puc+=ITEM_LEN;
        }
    }

    /* 读取CRC */
    memcpy(aucTemp, puc+ITEM_NAME_LEN, ITEM_VALUE_LEN);
    aucTemp[4] = '\0';

    usCrc = strtoul(aucTemp, NULL, 16);

    /* CRC是否一致判断 */
    if (usCrc != usCalcCrc)
    {
        LOG_Write(LOG_KERNEL, "压板文件CRC不一致.\n", NULL);

        stsRet = EP_FILE_ERR;
    }

    EP_free(pucBuf);
    return stsRet;
}

static EP_STATUS SC_Rd_Link_Mode_File(void)
{
    uint8_t *pucBuf;
    uint32_t ulLen;
    uint8_t *puc;
    uint8_t aucTemp[64];
    EP_STATUS stsRet;
    SC_LINK_ITEM *plink;
    uint8_t aucSgMode[81]="__TotalYabanMode__                                              ";

    if ((pucBuf=FT_File_To_Mem(EP_LINK_MODE_FILE, &ulLen))==NULL)
        return EP_FILE_ERR;

    if (ulLen!=81*(iLinkNum_g+5) || memcmp(pucBuf, pucLinkModeFileHead_g, 81))
    {
        EP_free(pucBuf);
        return EP_FILE_ERR;
    }

    stsRet=EP_SUCCESS;

    puc=pucBuf+81;
    if (!memcmp(puc, aucSgMode, 64))
    {
        puc+=64;
        ulTotalLinkMode_g=0;
        if(!memcmp(puc, "HARD            \n", 17))
            ulTotalLinkMode_g|=LINK_MODE_HW;
        else if(!memcmp(puc, "SOFT            \n", 17))
            ulTotalLinkMode_g|=LINK_MODE_SW;
        else if(!memcmp(puc, "AND             \n", 17))
            ulTotalLinkMode_g|=LINK_MODE_AND;
        else if(!memcmp(puc, "OR              \n", 17))
            ulTotalLinkMode_g|=LINK_MODE_OR;
        else if(!memcmp(puc, "CUSTOM          \n", 17))
            ulTotalLinkMode_g|=LINK_MODE_CUS;
        else
            stsRet=EP_FILE_ERR;
    }
    else
        stsRet=EP_FILE_ERR;

//    if(!(ulTotalLinkMode_g&LINK_MODE_CUS))  /*DQ:非定制压板模式，仍需再继续读取各压板模式*/
//        return EP_SUCCESS;

    puc=pucBuf+81*5;

    for (plink=plink_g; plink<plink_g+iLinkNum_g; plink++)
    {
        SI_Tag_Str_Cpy(aucTemp, plink->aucId, 64);
        if (!memcmp(puc, aucTemp, 64))
        {
            puc+=64;
            if(!memcmp(puc, "HARD            \n", 17))
                plink->aucMode=LINK_MODE_HW;
            else if(!memcmp(puc, "SOFT            \n", 17))
                plink->aucMode=LINK_MODE_SW;
            else if(!memcmp(puc, "AND             \n", 17))
                plink->aucMode=LINK_MODE_AND;
            else if(!memcmp(puc, "OR              \n", 17))
                plink->aucMode=LINK_MODE_OR;
            else
            {
                stsRet=EP_FILE_ERR;
                plink->aucMode=LINK_MODE_HW;
            }
            puc+=17;
        }
        else
        {
            stsRet=EP_FILE_ERR;
            puc+=81;
        }
    }

    EP_free(pucBuf);
    return stsRet;
}

/* Check if setting area file is valid.
 * Parameters:
 *      iFd, file descriptor opened previously.
 * Return value:
 *      TRUE, the setting area file is valid.
 *      FALSE, the setting area file is NOT valid.
 * Alert:
 *      Current position of the file is changed in this function. */
BOOL SC_Is_Valid_Set(int iFd)
{
    /*201-3-7-23 ZY 去掉assert调用，允许失败,注意资源的释放 */
    uint8_t aucBuf[10];
    uint8_t *apucVldFg[256];
    int i;
    int iIdx;
    int iSet;
    BOOL bRet;
    SC_SET_PAGE *psetpg;
    SC_SET_PAGE *psetpgWr; /* 固化 */
    SC_SET_ITEM *pset;
    SC_SET_ITEM *psetWr; /* 固化 */
    FLT_U32_UNION fu32;
    BOOL bHasCRC;
    uint8_t aucLogInfo[256];

    //assert(iFd>=0);
    if(!(iFd>=0))
    {
        return  FALSE;
    }



    lseek(iFd, 0, SEEK_SET);

    if (read(iFd, aucBuf, 10)!=10 ||
            aucBuf[0]!=0x82 || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x7D|| aucBuf[9]!=iSetPgNum_g-1)
        return FALSE;
    if(aucBuf[6]==0x01)
        bHasCRC=TRUE;
    else
        bHasCRC=FALSE;
    if(bHasCRC)
    {
        lseek(iFd, -6, SEEK_END);
        if (read(iFd, aucBuf, 4)!=4 ||
                aucBuf[0]!=0x84 || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x7B)
            return FALSE;
    }
    else
    {
        lseek(iFd, -4, SEEK_END);
        if (read(iFd, aucBuf, 4)!=4 ||
                aucBuf[0]!=0x84 || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x7B)
            return FALSE;

    }

    if(bHasCRC)
    {
        if(!SC_Check_CRC(iFd))
            return FALSE;
    }

    lseek(iFd, 10, SEEK_SET);
    memset(apucVldFg, (int)NULL, sizeof(apucVldFg));

    for (iIdx=0; iIdx<iSetPgNum_g-1; iIdx++)
    {
        i=read(iFd, aucBuf, 4);
        //assert(i==4);
        if(!(i==4))
        {
            int  k1=0;
            for (k1=0; k1<iSetPgNum_g-1; k1++)
            {
                if (apucVldFg[k1])
                    EP_free(apucVldFg[k1]);
            }

            return  FALSE;
        }

        if (aucBuf[0]>=iSetPgNum_g)
            break;

        psetpg=psetpg_g+aucBuf[0]+1;
        psetpgWr = psetpgWr_g+aucBuf[0]+1;

        /*激活标志现已不用,该检查被张云删除
        if (!aucBuf[1] && (psetpg->bIsPub || psetpg->psublgc->bRun))
            break;
        */
        if (U8_TO_U16(aucBuf[3], aucBuf[2])!=psetpg->iSetNum)
            break;

        if (apucVldFg[iIdx])
            break;
        else
        {
            apucVldFg[iIdx]=calloc(psetpg->iSetNum, sizeof(*apucVldFg[iIdx]));
            //assert(apucVldFg[iIdx]);
            if(!(apucVldFg[iIdx]))
            {
                int  k2=0;
                for (k2=0; k2<iSetPgNum_g-1; k2++)
                {
                    if (apucVldFg[k2])
                        EP_free(apucVldFg[k2]);
                }

                return  FALSE;
            }
        }

        for (iSet=0; iSet<psetpg->iSetNum; iSet++)
        {
            i=read(iFd, aucBuf, 9);
            //assert(i==9);
            if(!(i==9))
            {
                int  k3=0;
                for (k3=0; k3<iSetPgNum_g-1; k3++)
                {
                    if (apucVldFg[k3])
                        EP_free(apucVldFg[k3]);
                }
                return  FALSE;
            }

            i=U8_TO_U16(aucBuf[3], aucBuf[2]);
            if (i>=psetpg->iSetNum)
            {
                break;
            }

            pset=psetpg->pset+i;
            psetWr = psetpgWr->pset+i; /* 定值项 */
            if (pset->ucUnit!=aucBuf[4])
            {
                /* 2008-1-29日 张云 merge 添加英文信息，和修改逗号用法错误 */
                /*	if(ENG_MODE == 0)
                 {
                		     ER_Set_Err(EV_SET_ERR,  ER_ALARM |ER_LOCK| ER_REPORT,
                			 "定值项[%s]类型错误\n", (int)pset->aucId, 0);
                         }
                         else if(ENG_MODE == 1)
                         {
                		     ER_Set_Err(EV_SET_ERR,  ER_ALARM | ER_LOCK | ER_REPORT,
                			 "Item [%s] type error\n", (int)pset->aucId, 0);
                    }*/
                if (ENG_MODE == 0)
                {
                    sprintf(aucLogInfo, "定值项[%s]类型错误(%u/%u)\n", pset->aucId, pset->ucUnit, aucBuf[4]);
                }
                else if (ENG_MODE == 1)
                {
                    sprintf(aucLogInfo, "Item [%s] type error(%u/%u)\n", pset->aucId, pset->ucUnit, aucBuf[4]);
                }
                LOG_Write(LOG_KERNEL, aucLogInfo, NULL);

                break;
            }

            fu32.ulVal=BYTES_TO_U32(aucBuf+5);

            if (IS_INT32_SET(pset->ucUnit))
            {
                if (fu32.lVal>pset->valMax.lVal || fu32.lVal<pset->valMin.lVal)
                {
                    /* 2008-1-29日 张云 merge 添加英文信息，和修改逗号用法错误 */
                    /*if(ENG_MODE == 0)
                    {
                         ER_Set_Err(EV_SET_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                    	 "定值 %s 越界\n", (int)pset->aucId, 0);
                     }
                     else if(ENG_MODE == 1)
                     {
                         ER_Set_Err(EV_SET_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                    	 " %s overflow\n", (int)pset->aucId, 0);
                     }*/
                    if (ENG_MODE == 0)
                    {
                        sprintf(aucLogInfo, "定值项[%s]越界(%d/%d|%d)\n", pset->aucId, (int)fu32.lVal,
                                (int)pset->valMax.lVal, (int)pset->valMin.lVal);
                    }
                    else if (ENG_MODE == 1)
                    {
                        sprintf(aucLogInfo, "Item [%s] overflow(%d/%d|%d)\n", pset->aucId, (int)fu32.lVal,
                                (int)pset->valMax.lVal, (int)pset->valMin.lVal);
                    }
                    LOG_Write(LOG_KERNEL, aucLogInfo, NULL);
                    break;
                }
            }
            else if(IS_UINT32_SET(pset->ucUnit))
            {
                if (fu32.ulVal>pset->valMax.ulVal || fu32.ulVal<pset->valMin.ulVal)
                {
                    /* 2008-1-29日 张云 merge 添加英文信息，和修改逗号用法错误 */
                    /*if(ENG_MODE == 0)
                    {
                         ER_Set_Err(EV_SET_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                    	 "定值 %s 越界\n", (int)pset->aucId, 0);
                     }
                     else if(ENG_MODE == 1)
                     {
                         ER_Set_Err(EV_SET_ERR,  ER_ALARM |ER_LOCK| ER_REPORT,
                    	 " %s overflow\n", (int)pset->aucId, 0);
                     }*/
                    if (ENG_MODE == 0)
                    {
                        sprintf(aucLogInfo, "定值项[%s]类型越界(%u/%u|%u)\n", pset->aucId,
                                (unsigned int)fu32.ulVal, (unsigned int)pset->valMax.ulVal, (unsigned int)pset->valMin.ulVal);
                    }
                    else if (ENG_MODE == 1)
                    {
                        sprintf(aucLogInfo, "Item [%s] overflow(%u/%d|%u)\n", pset->aucId,
                                (unsigned int)fu32.ulVal, (unsigned int)pset->valMax.ulVal, (unsigned int)pset->valMin.ulVal);
                    }
                    LOG_Write(LOG_KERNEL, aucLogInfo, NULL);

                    break;
                }
            }
            else if (fu32.fVal>pset->valMax.fVal+FLT_PRECISION ||
                     fu32.fVal<pset->valMin.fVal-FLT_PRECISION)
            {
                /* 2008-1-29日 张云 merge 添加英文信息，和修改逗号用法错误 */
                /*if(ENG_MODE == 0)
                {
                     ER_Set_Err(EV_SET_ERR,  ER_ALARM|ER_LOCK | ER_REPORT,
                	 " 定值 %s 越界\n", (int)pset->aucId, 0);
                 }
                 else if(ENG_MODE == 1)
                 {
                     ER_Set_Err(EV_SET_ERR,  ER_ALARM |ER_LOCK| ER_REPORT,
                	 "S %s overflow\n", (int)pset->aucId, 0);
                 }*/
                if (ENG_MODE == 0)
                {
                    sprintf(aucLogInfo, "定值项[%s]类型越界(%f/%f|%f)\n", pset->aucId, fu32.fVal, pset->valMax.fVal,
                            pset->valMin.fVal);
                }
                else if (ENG_MODE == 1)
                {
                    sprintf(aucLogInfo, "Item [%s] overflow(%f/%f|%f)\n", pset->aucId, fu32.fVal, pset->valMax.fVal,
                            pset->valMin.fVal);
                }
                LOG_Write(LOG_KERNEL, aucLogInfo, NULL);

                break;
            }

            apucVldFg[iIdx][iSet]=TRUE;

            psetWr->valNow = fu32;
        }

        if (iSet<psetpg->iSetNum)
            break;
    }

    if (iIdx==iSetPgNum_g-1)
        bRet=TRUE;
    else
        bRet=FALSE;

    for (iIdx=0; iIdx<iSetPgNum_g-1; iIdx++)
    {
        if (!bRet)
        {
            if (apucVldFg[iIdx])
                EP_free(apucVldFg[iIdx]);
        }
        else
        {
            psetpg=psetpg_g+iIdx+1;

            if (apucVldFg[iIdx])
            {
                for (i=0; i<psetpg->iSetNum; i++)
                {
                    if (!apucVldFg[iIdx][i])
                        bRet=FALSE;
                }

                EP_free(apucVldFg[iIdx]);
            }
            else if (psetpg->bIsPub || psetpg_g->psublgc->bRun)
                bRet=FALSE;
        }
    }

    return bRet;
}

/* 复位写入定值
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SC_Reset_Set(void)
{
    int iIdx;
    int iSet;
    SC_SET_PAGE *psetpgWr = NULL; /* 固化 */
    SC_SET_PAGE *psetpg = NULL;
    SC_SET_ITEM *psetWr; /* 固化 */
    SC_SET_ITEM *pset;

    for (iIdx = 0; iIdx<iSetPgNum_g-1; iIdx++)
    {
        psetpgWr = psetpgWr_g+1+iIdx;
        psetpg = psetpg_g+1+iIdx;

        for (iSet = 0; iSet<psetpg->iSetNum; iSet++)
        {
            psetWr = psetpgWr->pset+iSet; /* 定值项 */
            pset = psetpg->pset+iSet;

            psetWr->valNow = pset->valNow;
        }
    }
}

/*check inner valid or not(file has crc in end)
 * Parameters:
 *      iFd, file descriptor of inner setting file opened previously.
 * Return value:
 *      Pointer to an array contains inner setting items.
*/
BOOL SC_Check_CRC(int fp)
{
    /*2013-7-23  ZY,去掉assert调用 */
    char aucBuf[256];
    int temp_val;
    int file_lenth;
    int i = 0;
    int j = 0;
    int m;
    int iFileCRC=0;
    int iRdlen;
    int iLastCRC;
    temp_val=lseek(fp,0,0);
    file_lenth=lseek(fp,0,2);
    file_lenth=file_lenth-temp_val-2;

    /* 读取文件计算CRC */
    i = file_lenth/256;
    j= file_lenth%256;
    lseek(fp, 0, SEEK_SET);
    for(m = 0; m<i; m++)
    {
        iRdlen = read(fp,aucBuf,256);
        //assert(iRdlen==256);
        if(!(iRdlen==256))
        {
            return FALSE;
        }

        iFileCRC=EP_CCITT_CRC16(aucBuf,256,iFileCRC);
    }
    if(j!=0)
    {
        iRdlen = read(fp,aucBuf,j);
        //assert(iRdlen==j);
        if(!(iRdlen==j))
        {
            return  FALSE;
        }
        iFileCRC=EP_CCITT_CRC16(aucBuf,j,iFileCRC);
    }
    iRdlen = read(fp,aucBuf,2);
    //assert(iRdlen==2);
    if(!(iRdlen==2))
    {
        return  FALSE;
    }
    iLastCRC = U8_TO_U16(aucBuf[1],aucBuf[0]);
    LOG_Dbg_Msg("computer crc is %x,file crc is %x\n",iFileCRC,iLastCRC,0,0,0,0);
    if(iLastCRC==iFileCRC)
        return TRUE;
    else
        return FALSE;


}
/* Read inner setting from file to memory.
 * Parameters:
 *      iFd, file descriptor of inner setting file opened previously.
 *      piNum, to save setting number when retrurn.
 * Return value:
 *		TRUE, file valid .
 *      FALSE, file invalid. */
SC_SET_ITEM *SC_Rd_Inner_Set(int iFd, u_int *piNum, BOOL *bhascrc)
{
    /*2013-7-23  ZY 清除assert,注意资源回收 */
    SC_SET_ITEM *psetRet;
    SC_SET_ITEM *pset;
    uint8_t aucBuf[27+MAX_ID_LEN];
    int iRdLen;
    int iIdx;
    int i;

    //assert(iFd>=0);
    if(!(iFd>=0))
    {
        return NULL;
    }

    psetRet=NULL;
    *piNum=0;


    lseek(iFd, 0, SEEK_SET);

    if (read(iFd, aucBuf, 10)!=10 ||
            aucBuf[0]!=0x93 || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x6C)
        goto reterr;
    if(aucBuf[6]==0x01)
        *bhascrc=TRUE;
    else
        *bhascrc=FALSE;
    *piNum=aucBuf[9];
    if(*bhascrc)
    {
        lseek(iFd, -6, SEEK_END);
        if (read(iFd, aucBuf, 4)!=4 ||
                aucBuf[0]!=0x9C || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x63)
            goto reterr;
    }
    else
    {
        lseek(iFd, -4, SEEK_END);
        if (read(iFd, aucBuf, 4)!=4 ||
                aucBuf[0]!=0x9C || aucBuf[1]!=0 || aucBuf[2]!=0 || aucBuf[3]!=0x63)
            goto reterr;

    }
    lseek(iFd, 10, SEEK_SET);
    if ((psetRet=calloc(*piNum, sizeof(*psetRet)))==NULL)
        goto reterr;

    for (iIdx=0; iIdx<*piNum; iIdx++)
    {
        iRdLen=read(iFd, aucBuf, 2);
        //assert(iRdLen==2 && aucBuf[0]<*piNum);
        if(!(iRdLen==2 && aucBuf[0]<*piNum))
        {
            goto  reterr;
        }

        pset=psetRet+aucBuf[0];
        //assert(!pset->aucName[0]);
        if(!(!pset->aucName[0]))
        {
            goto  reterr;
        }

        //assert(aucBuf[1]<MAX_ID_LEN);
        if(!(aucBuf[1]<MAX_ID_LEN))
        {
            goto  reterr;
        }
        iRdLen=read(iFd, pset->aucName, aucBuf[1]);
        //assert(iRdLen==aucBuf[1]);
        if(!(iRdLen==aucBuf[1]))
        {
            goto  reterr;
        }
        pset->aucName[iRdLen]='\0';

        iRdLen=read(iFd, aucBuf, 6);
        //assert(iRdLen==6);
        if(!(iRdLen==6))
        {
            goto  reterr;
        }

        pset->aucABRV[0]=aucBuf[0];
        pset->aucABRV[1]=aucBuf[1];
        pset->aucABRV[2]=aucBuf[2];
        pset->aucABRV[3]=aucBuf[3];
        pset->bIsPrvtUse=aucBuf[4];
        if(pset->bIsPrvtUse)
        {
            i=aucBuf[5];
            //assert( i<MAX_ID_LEN);
            if(!( i<MAX_ID_LEN))
            {
                goto  reterr;
            }
            iRdLen=read(iFd, aucBuf, i+1);
            //assert(iRdLen==i+1);
            if(!(iRdLen==i+1))
            {
                goto  reterr;
            }
            pset->ucAttr=aucBuf[i];
        }
        else
            pset->ucAttr=aucBuf[5];

        iRdLen=read(iFd, aucBuf, 15);/*2008-7-16日，支持字符串定值，张云修改  */
        //assert(iRdLen==15);
        if(!(iRdLen==15))
        {
            goto  reterr;
        }


        pset->ucUnit=aucBuf[2];

        pset->valMax.ulVal=BYTES_TO_U32(aucBuf+3);

        pset->valMin.ulVal=BYTES_TO_U32(aucBuf+7);

        pset->valDft.ulVal=BYTES_TO_U32(aucBuf+11);

        if(pset->ucUnit==0x68)
        {
            /*2008-7-16日，支持字符串定值，张云  */
            if(pset->valDft.ulVal>MAX_ID_LEN)
            {
                LOG_Dbg_Msg("错误，字符串默认定值长度超出允许范围!",0,0,0,0,0,0);
                goto reterr;
            }
            iRdLen=read(iFd, pset->aucDftStr, pset->valDft.ulVal);
            //assert(iRdLen==pset->valDft.ulVal);
            if(!(iRdLen==pset->valDft.ulVal))
            {
                goto reterr;
            }
        }

        iRdLen=read(iFd, aucBuf, 4);
        //assert(iRdLen==4);
        if(!(iRdLen==4))
        {
            goto reterr;
        }
        pset->valNow.ulVal=BYTES_TO_U32(aucBuf);
        if(pset->ucUnit==0x68)
        {
            /*2008-7-16日，支持字符串定值，张云  */
            if(pset->valNow.ulVal>MAX_ID_LEN)
            {
                LOG_Dbg_Msg("错误，字符串当前定值长度超出允许范围!",0,0,0,0,0,0);
                goto reterr;
            }
            iRdLen=read(iFd,pset->aucNowStr, pset->valNow.ulVal);
            //assert(iRdLen==pset->valNow.ulVal);
            if(!(iRdLen==pset->valNow.ulVal))
            {
                goto reterr;
            }
        }

        iRdLen=read(iFd, aucBuf, 2); /*2008-7-16日，支持字符串定值，张云修改  */
        //assert(iRdLen==2);
        if(!(iRdLen==2))
        {
            goto reterr;

        }
        i=U8_TO_U16(aucBuf[1], aucBuf[0]);

        //assert(i<=1024);
        if(!(i<=1024))
        {
            goto reterr;
        }
        if ((pset->pucUnitName=malloc(i+1))==NULL)
            goto reterr;

        iRdLen=read(iFd, pset->pucUnitName, i);
        //assert(iRdLen==i);
        if(!(iRdLen==i))
        {
            goto reterr;
        }
        pset->pucUnitName[iRdLen]='\0';
    }

    return psetRet;

reterr:
    if (psetRet)
        SC_Free_Set_Mem(psetRet, *piNum);

    return NULL;
}
/* Free dynamic memory allocted for setting data structure.
 * Parameters:
 *      pset, pointer to setting array dynamic allocated before.
 *      iNum, item number in the setting array.
 * Return value:
 *      None. */
void SC_Free_Set_Mem(SC_SET_ITEM *pset, int iNum)
{
    SC_SET_ITEM *psetWork;

    if (pset)
    {
        for (psetWork=pset; psetWork<pset+iNum; psetWork++)
        {
            if (psetWork->pucUnitName)
                EP_free(psetWork->pucUnitName);
        }

        EP_free(pset);
    }
    else
        assert(FALSE);
}

/*check  inner settings is valid.
 * Parameters:
 *      iFd, file descriptor of inner setting file opened previously.
 * Return value:
 *      EP_SUCCESS, valid.
 *      EP_BAD_DATA, invalid. */
EP_STATUS SC_Inner_Set_Is_Valid(int iFd)
{
    SC_SET_ITEM *psetRd;
    int iNum;
    SC_SET_ITEM *psetOrg;
    SC_SET_ITEM *psetRdOld;
    SC_SET_ITEM *pset;
    SC_SET_ITEM *psetWr;
    BOOL bhasCRC;

    assert(iFd>=0);

    psetRd=SC_Rd_Inner_Set(iFd, &iNum,&bhasCRC);
    psetRdOld=psetRd;
    if (!psetRd)
        return EP_BAD_DATA;

    if (iNum!=psetpg_g->iSetNum)
    {
        SC_Free_Set_Mem(psetRd, iNum);
        return EP_BAD_DATA;
    }
    if(bhasCRC)
    {
        if(!SC_Check_CRC(iFd))
        {
            SC_Free_Set_Mem(psetRd, iNum);
            return EP_BAD_DATA;
        }
    }

    /* 内部定值固化结果保存 */
    psetpgWr_g->iSetNum = iNum;

    for (psetOrg=psetpg_g->pset, pset=psetRd, psetWr = psetpgWr_g->pset;
            pset<psetRd+iNum; psetOrg++, pset++, psetWr++)
    {
        /*原来代码 BUG，张云改过，2006-8-21  */
        if (pset->ucUnit!=psetOrg->ucUnit ||
                pset->aucABRV[0]!=psetOrg->aucABRV[0] ||
                pset->aucABRV[1]!=psetOrg->aucABRV[1] ||
                pset->aucABRV[2]!=psetOrg->aucABRV[2] ||
                pset->aucABRV[3]!=psetOrg->aucABRV[3] ||
                strcmp(pset->aucName, psetOrg->aucName) ||
                strcmp(pset->pucUnitName, psetOrg->pucUnitName) ||
                (IS_INT32_SET(pset->ucUnit) &&
                 ((pset->valNow.lVal>psetOrg->valMax.lVal ||
                   pset->valNow.lVal<psetOrg->valMin.lVal))) ||
                (IS_UINT32_SET(pset->ucUnit) &&
                 ((pset->valNow.ulVal>psetOrg->valMax.ulVal ||
                   pset->valNow.ulVal<psetOrg->valMin.ulVal))) ||
                (IS_FLT_SET(pset->ucUnit) &&
                 (pset->valNow.fVal>psetOrg->valMax.fVal+FLT_PRECISION ||
                  pset->valNow.fVal<psetOrg->valMin.fVal-FLT_PRECISION)))
        {
            SC_Free_Set_Mem(psetRd, iNum);
            return EP_BAD_DATA;
        }
        else
        {
            *psetWr = *pset;
        }
    }
    SC_Free_Set_Mem(psetRd, iNum);
    return EP_SUCCESS;

}

/* 复位写入内部定值
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SC_Reset_Inner_Set(void)
{
    int i;
    SC_SET_ITEM *psetWr = NULL;
    SC_SET_ITEM *pset = NULL;

    for (i = 0, psetWr = psetpgWr_g->pset, pset = psetpg_g->pset;
            i < psetpgWr_g->iSetNum; i++, psetWr++, pset++)
    {
        psetWr->valNow = pset->valNow;
    }
}

/* Change memory inner settings. used in file download
 * Parameters:
 *      iFd, file descriptor of inner setting file opened previously.
 * Return value:
 *      EP_SUCCESS, making new inner setting work OK.
 *      EP_BAD_DATA, file format error, inner setting not changed. */
EP_STATUS SC_Chg_Mem_Inner_Set(int iFd)
{
    SC_SET_ITEM *psetRd;
    int iNum;
    SC_SET_ITEM *psetOrg;
    SC_SET_ITEM *pset;
    BOOL bhasCRC;

    assert(iFd>=0);


    psetRd=SC_Rd_Inner_Set(iFd, &iNum,&bhasCRC);
    if (!psetRd)
        return EP_BAD_DATA;

    if (iNum!=psetpg_g->iSetNum)
    {
        SC_Free_Set_Mem(psetRd, iNum);
        return EP_BAD_DATA;
    }
    InsideSetModifiesToLog(psetRd,iNum);
    for (psetOrg=psetpg_g->pset, pset=psetRd;
            pset<psetRd+iNum; psetOrg++, pset++)
    {
        /*如果用于修改比例系数等的字符串尾控制字被修改，需要重新
        修改ai、ao、遥测、测量配置和db中的这些值*/
        if(pCoefTailSet_g==psetOrg &&  psetOrg->valNow.ulVal!=pset->valNow.ulVal)
            bCoefTailChg=TRUE;
        psetOrg->valNow.ulVal=pset->valNow.ulVal;

        /*2008-7-16日 张云 字符串定值  */
        if(psetOrg->ucUnit==0x68)
        {
            if(psetOrg->valNow.ulVal>MAX_ID_LEN)
            {
                SC_Free_Set_Mem(psetRd, iNum);
                return EP_BAD_DATA;
            }
            strncpy(psetOrg->aucNowStr,pset->aucNowStr,psetOrg->valNow.ulVal);
        }

    }

    SC_Free_Set_Mem(psetRd, iNum);

    RE_SetLogSetChgCnt();

    SC_Updt_Each_GivenSetting_Decided_Item();
    return EP_SUCCESS;

}
/* Change inner settings.
 * Parameters:
 *      iFd, file descriptor of inner setting file opened previously.
 * Return value:
 *      EP_SUCCESS, making new inner setting work OK.
 *      EP_BAD_DATA, file format error, inner setting not changed. */
EP_STATUS SC_Chg_Inner_Set(int iFd)
{
    SC_SET_ITEM *psetRd;
    int iNum;
    SC_SET_ITEM *psetOrg;
    SC_SET_ITEM *pset;
    BOOL bhasCRC;

    assert(iFd>=0);


    psetRd=SC_Rd_Inner_Set(iFd, &iNum,&bhasCRC);


    if (!psetRd)
        return EP_BAD_DATA;

    if (iNum!=psetpg_g->iSetNum)
    {
        SC_Free_Set_Mem(psetRd, iNum);
        return EP_BAD_DATA;
    }

    if(bhasCRC)
    {
        if(!SC_Check_CRC(iFd))
        {
            SC_Free_Set_Mem(psetRd, iNum);
            return EP_BAD_DATA;
        }
    }

    for (psetOrg=psetpg_g->pset, pset=psetRd;
            pset<psetRd+iNum; psetOrg++, pset++)
    {
        /*原来代码 BUG，张云改过，2006-8-21  */
        if (pset->ucUnit!=psetOrg->ucUnit ||
                pset->aucABRV[0]!=psetOrg->aucABRV[0] ||
                pset->aucABRV[1]!=psetOrg->aucABRV[1] ||
                pset->aucABRV[2]!=psetOrg->aucABRV[2] ||
                pset->aucABRV[3]!=psetOrg->aucABRV[3] ||
                strcmp(pset->aucName, psetOrg->aucName) ||
                strcmp(pset->pucUnitName, psetOrg->pucUnitName) ||
                (IS_INT32_SET(pset->ucUnit) &&
                 ((pset->valNow.lVal>psetOrg->valMax.lVal ||
                   pset->valNow.lVal<psetOrg->valMin.lVal))) ||
                (IS_UINT32_SET(pset->ucUnit) &&
                 ((pset->valNow.ulVal>psetOrg->valMax.ulVal ||
                   pset->valNow.ulVal<psetOrg->valMin.ulVal))) ||
                (IS_FLT_SET(pset->ucUnit) &&
                 (pset->valNow.fVal>psetOrg->valMax.fVal+FLT_PRECISION ||
                  pset->valNow.fVal<psetOrg->valMin.fVal-FLT_PRECISION)))
        {
            SC_Free_Set_Mem(psetRd, iNum);
            return EP_BAD_DATA;
        }
    }

    for (psetOrg=psetpg_g->pset, pset=psetRd;
            pset<psetRd+iNum; psetOrg++, pset++)
    {
        /*如果用于修改比例系数等的字符串尾控制字被修改，需要重新
        修改ai、ao、遥测、测量配置和db中的这些值*/
        if(pCoefTailSet_g==psetOrg &&  psetOrg->valNow.ulVal!=pset->valNow.ulVal)
            bCoefTailChg=TRUE;
        psetOrg->valNow.ulVal=pset->valNow.ulVal;

        /*2008-7-16日 张云 字符串定值  */
        if(psetOrg->ucUnit==0x68)
        {
            if(psetOrg->valNow.ulVal>MAX_ID_LEN)
            {
                SC_Free_Set_Mem(psetRd, iNum);
                return EP_BAD_DATA;
            }
            strncpy(psetOrg->aucNowStr,pset->aucNowStr,psetOrg->valNow.ulVal);
        }

    }

    SC_Free_Set_Mem(psetRd, iNum);

    RE_SetLogSetChgCnt();

    SC_Updt_Each_GivenSetting_Decided_Item();
    return EP_SUCCESS;
}

/* Finish writing a setting area.
 * Parameters:
 *      iArea, setting area number.
 * Return value:
 *      EP_SUCCESS, or EP_ERROR. */
EP_STATUS SC_End_Wr_Set(int iArea, uint8_t *Back_Filename)
{
    /*2013-8-21  ZY 去掉assert,注意资源回收 */
    uint8_t aucFileName[FULL_NAME_LEN+1];
    int iFd,iFdBak;
    STATUS vxsts;

    /* 定值区号越界 */
    if ((iArea<SET_AREA_START_NO) || (iArea >= scinfo_g.iMaxSetArea))
    {
        return EP_ERROR;
    }

    sprintf(aucFileName, EP_SET_AREA_DIR "/area%02x.dza", iArea);

    iFd=open(aucFileName, O_RDONLY, 0);
    if (iFd == ERROR)
    {
        return EP_ERROR;
    }

    /* 定值是否有效判断 */
    if (SC_Is_Valid_Set(iFd) == FALSE)
    {
        vxsts=close(iFd);
        return EP_ERROR;
    }

    vxsts=close(iFd);

    if (vxsts == ERROR)
    {
        return EP_ERROR;
    }

    scinfo_g.aucSetAreaFg[iArea] |= (SET_HAVE_FILE | SET_VALID);

    if (iArea==scinfo_g.iWorkSetArea)
    {
        if(SC_Updt_Work_Set_And_Log(iArea)!=EP_SUCCESS)
        {
            scinfo_g.iRealWorkSetArea=0xFF;

            return EP_ERROR;
        }
    }
    else
    {
        /* 定值区无备份文件，为新建定值区 */
        if(strcmp(Back_Filename,SET_BACK_FILE_NONE)==0)
        {
            SetNewToLog(iArea);
        }
        else   /* 定值区有备份文件，修改定值区 */
        {
            iFd=open(aucFileName, O_RDONLY, 0);
            if (iFd == ERROR)
            {
                return EP_ERROR;
            }

            iFdBak=open(Back_Filename, O_RDONLY, 0);
            if (iFdBak == ERROR)
            {
                vxsts=close(iFd);
                return EP_ERROR;
            }

            /* 定值备份是否有效判断 */
            if (SC_Is_Valid_Set(iFdBak) == FALSE)
            {
                vxsts=close(iFd);
                vxsts=close(iFdBak);
                SetNewToLog(iArea);
                /* return EP_ERROR; */
            }
            else
            {
                /* 修改非运行定值区定值记录日志 */
                NOWorkSetModifiesToLog(iFd, iFdBak, iArea);

                vxsts=close(iFd);
                if (vxsts == ERROR)
                {
                    vxsts=close(iFdBak);
                    return EP_ERROR;
                }

                vxsts=close(iFdBak);
                if (vxsts == ERROR)
                {
                    return EP_ERROR;
                }
            }
        }
    }

    return EP_SUCCESS;
}

/* Change protect function run/exit status.
 * Parameters:
 *      strName, protect(sub-logic) name.
 *      bSts, new status.
 * Return value:
 *      EP_SUCCESS, change run/exit status OK.
 *      EP_FILE_ERR, file operating failure. */
EP_STATUS SC_Chg_Prtc_Sts(const uint8_t *strName, BOOL bSts)
{
    SC_SUB_LGC_ITEM *psublgc;
    EP_STATUS sts;
    uint8_t *pucVal;
    uint16_t unComCrc;

    assert(strName>=0 && strlen(strName)<=MAX_ID_LEN);

    for (psublgc=psublgc_g; psublgc<psublgc_g+iSubLgcNum_g; psublgc++)
    {
        if (!strcmp(psublgc->aucName, strName))
        {
            if (psublgc->bRun==bSts)
                return EP_SUCCESS;

            if (bSts)
                pucVal="RUN             ";
            else
                pucVal="EXIT            ";

            Set_FunSts_Wr_Sts(1);

            sts=SI_Chg_Sts_File_Item(EP_FUNC_STS_FILE, pucFuncFileHead_g,
                                     iSubLgcNum_g+2, (psublgc-psublgc_g)+2, pucVal);

            if (sts!=EP_SUCCESS)
                return EP_FILE_ERR;
            else
            {
                unComCrc =0;
                unComCrc=FT_File_CRC16(EP_FUNC_STS_FILE);
                FT_Wr_INI_CRC("[CRC]",CRC_ITEM_FUNC,unComCrc);

                Set_FunSts_Wr_Sts(0);

                psublgc->bRun=bSts?TRUE:FALSE;
                if(ENG_MODE == 0)
                {
                    LOG_Write(LOG_OPRATE, "保护功能投退.\n", NULL);
                }
                else if(ENG_MODE == 1)
                {
                    LOG_Write(LOG_OPRATE, "Protection relay enabled or disabled.\n", NULL);
                }
                return EP_SUCCESS;
            }
        }
    }

    return EP_PARM_ERR;
}

/* Change software link status.
 * Parameters:
 *      iIdx, index of link(from 0).
 *      bSts, new status.
 * Return value:
 *      EP_SUCCESS, change link status OK.
 *      EP_FILE_ERR, file operating failure. */
EP_STATUS SC_Chg_Sw_Link(int iIdx, BOOL bSts)
{
    SC_LINK_ITEM *plink;
    uint8_t *pucVal;
    STATUS sts;

    assert(iIdx>=0 && iIdx<iLinkNum_g);

    plink=plink_g+iIdx;

    if (plink->bSwVal==bSts)
        return EP_SUCCESS;

    if (bSts)
        pucVal=SW_CLOSE;
    else
        pucVal=SW_OPEN;

    /* 更加保守的处理, 即使不是文件更新时出错也能接受 */

    /* Set_Link_Wr_Sts(1); */
    sts=SI_Chg_Link_Sts_File_Item(EP_LINK_STS_FILE, pucLinkFileHead_g,
                                  iLinkNum_g+1, iIdx+1, pucVal);

    if (sts!=EP_SUCCESS)
        return EP_FILE_ERR;
    else
    {
        plink->bSwVal=bSts?TRUE:FALSE;
        SC_Updt_Certain_Link(iIdx);

        /* Write_Link_CRC(); */
        /* Set_Link_Wr_Sts(0); */

        return EP_SUCCESS;
    }
}

/* 复位压板
 * Para:
 *     NONE.
 * Return:
 *     NONE.
 */
void SC_Reset_Link(void)
{
    int chn;
    SC_LINK_ITEM *plinkWr = NULL; /* 固化压板 */
    SC_LINK_ITEM *plink = NULL;

    plink = plink_g;
    plinkWr = plinkWr_g;  /* 固化压板 */

    for (chn = 0; chn<iLinkNum_g; chn++, plink++, plinkWr++)
    {
        plinkWr->bSwVal = plink->bSwVal;
    }
}

/* 多个压板一起投退
 * Parameters:
 *      pRcvLinkBuf, 报文中的压板指针.
 * Return value:
 *      EP_SUCCESS, change link status OK.
 *      EP_ERROR, failure else.
 *      EP_FILE_ERR, file operating failure. */
EP_STATUS SC_Chg_Sw_Multi_Link(uint8_t *pRcvLinkBuf)
{
    uint16_t linkNum = 0;
    uint8_t linksn = 0;
    uint8_t linkop = 0;
    int i = 0;

    SC_LINK_ITEM *plink = NULL;
    SC_LINK_ITEM *plinkWr = NULL;
    uint8_t *pucVal;
    STATUS sts;

    uint8_t *pucBuf;
    uint32_t ulLen;
    int iFd;

    BOOL bSts = FALSE;
    BOOL bOldStats = FALSE;
    uint16_t usCrc = 0;
    uint8_t aucLine[ITEM_LEN];
    uint8_t aucTmp[ITEM_LEN];

    linkNum = U8_TO_U16(pRcvLinkBuf[24],pRcvLinkBuf[23]);

    if(linkNum > iLinkNum_g)
    {
        return EP_ERROR;
    }
    /*任意不匹配返回错*/
    for(i = 0; i < linkNum; i++)
    {
        linksn = pRcvLinkBuf[25+i*3];
        linkop = pRcvLinkBuf[26+i*3];
        if(linksn >= iLinkNum_g)
        {
            return EP_ERROR;
        }
        else if((linkop != 0x5a) && (linkop != 0xa5))
        {
            return EP_ERROR;
        }
        plinkWr = SC_Get_Wr_Sw_Link(linksn);
        plinkWr->bSwVal = (linkop == 0x5a)?TRUE:FALSE;
        if (pParaCheckFun)
        {
            if (pParaCheckFun() == FALSE)
            {
                SC_Reset_Link();

                return EP_ERROR;
            }
        }
    }

    /* Set_Link_Wr_Sts(1); */

    /*检测都通过，开始切换操作,先写文件*/
    if ((pucBuf=FT_File_To_Mem(EP_LINK_STS_FILE, &ulLen))==NULL)
        return EP_FILE_ERR;

    /* 增加CRC */
    if (ulLen!=ITEM_LEN+(iLinkNum_g+1)*ITEM_LEN+ITEM_LEN
            || memcmp(pucBuf, pucLinkFileHead_g, ITEM_LEN) ||
            (iFd=FT_Bgn_Update(EP_LINK_STS_FILE))<0)
    {
        EP_free(pucBuf);
        return EP_FILE_ERR;
    }

    /* 更加保守的处理, 即使不是文件更新时出错也能接受 */
    for(i = 0; i < linkNum; i++)
    {
        linksn = pRcvLinkBuf[25+i*3];
        linkop = pRcvLinkBuf[26+i*3];

        if(linkop == 0x5a)
        {
            bSts = TRUE;
        }
        else
        {
            bSts = FALSE;
        }

        plink=plink_g+linksn;

        bOldStats = plink->bSwVal;
        if (plink->bSwVal==bSts)
            continue;

        if (bSts)
            pucVal=SW_CLOSE;
        else
            pucVal=SW_OPEN;

        memcpy(pucBuf+ITEM_LEN+(linksn+1)*ITEM_LEN+ITEM_NAME_LEN,
               pucVal, ITEM_VALUE_LEN-1);
    }

    /* 计算并写入CRC
     */
    usCrc = EP_CCITT_CRC16(pucBuf, ITEM_LEN+(iLinkNum_g+1)*ITEM_LEN, usCrc);

    sprintf(aucTmp, "%04x", usCrc);
    SI_Tag_Str_Cpy(aucLine, aucTmp, ITEM_VALUE_LEN);

    memcpy(pucBuf+ITEM_LEN+(iLinkNum_g+1)*ITEM_LEN+ITEM_NAME_LEN,
           aucLine, ITEM_VALUE_LEN);

    i=write(iFd, pucBuf, ulLen);
    if(!(i==ulLen))
    {
        EP_free(pucBuf);
        FT_Abort_Update(EP_LINK_STS_FILE, iFd);
        return EP_FILE_ERR;
    }

    EP_free(pucBuf);

    sts=FT_End_Update(EP_LINK_STS_FILE, iFd);
    if(!(sts==EP_SUCCESS))
    {
        return EP_FILE_ERR;
    }
    /*文件写成功了在切换内存的值*/
    for(i = 0; i < linkNum; i++)
    {
        linksn = pRcvLinkBuf[25+i*3];
        linkop = pRcvLinkBuf[26+i*3];
        plink=plink_g+linksn;
        plink->bSwVal=(linkop == 0x5a)?TRUE:FALSE;
        SC_Updt_Certain_Link(linksn);
        SybChangeToLog(plink, bOldStats, bSts, pRcvLinkBuf[11]);
    }

    /* Write_Link_CRC(); */
    /* Set_Link_Wr_Sts(0); */

    return EP_SUCCESS;
}

SC_LINK_ITEM * SC_Get_Sw_Link(int iIdx)
{
    SC_LINK_ITEM *plink;

    assert(iIdx>=0 && iIdx<iLinkNum_g);

    plink=plink_g+iIdx;
    return plink;
}

/* 获取固化压板项.
 * Para:
 *     iIdx, 固化压板序号.
 * Return:
 *     压板句柄.
 */
SC_LINK_ITEM * SC_Get_Wr_Sw_Link(int iIdx)
{
    SC_LINK_ITEM *plink;

    assert(iIdx>=0 && iIdx<iLinkNum_g);

    plink=plinkWr_g+iIdx;
    return plink;
}

/*
Description: change the content of the /set/set/edplinkmode.set
iIdx: represent the certain link idx, only valid when bTotalFlag==false
ulMode: the link's mode to be set.
bTotalFlag:  if false, ucMode represent certain link's mode
             if true, ucMode represent total link's mode
*/
EP_STATUS SC_Chg_Link_Mode_File(int iIdx, uint8_t ucMode, BOOL bTotalFlag)
{
    SC_LINK_ITEM *plink;
    uint8_t *pucMode;
    STATUS sts=EP_SUCCESS;
    uint16_t unComCrc;

    switch (ucMode)
    {
        case LINK_MODE_HW:
            pucMode="HARD            ";
            break;
        case LINK_MODE_SW:
            pucMode="SOFT            ";
            break;
        case LINK_MODE_AND:
            pucMode="AND             ";
            break;
        case LINK_MODE_OR:
            pucMode="OR              ";
            break;
        case LINK_MODE_CUS:
            if(bTotalFlag)
                pucMode="CUSTOM          ";
            else
            {
                sts=EP_FILE_ERR;
                return sts;
            }
            break;
        default:
            sts=EP_FILE_ERR;
            return sts;
    }

    if(bTotalFlag)
    {
        if (ulTotalLinkMode_g&ucMode)
            return EP_SUCCESS;
        else
        {
            sts=SI_Chg_Sts_File_Item(EP_LINK_MODE_FILE, pucLinkModeFileHead_g,
                                     iLinkNum_g+4, 0, pucMode);
            if (sts!=EP_SUCCESS)
                return EP_FILE_ERR;
            else
            {
                ulTotalLinkMode_g=0;
                ulTotalLinkMode_g|=ucMode;
                SC_Updt_Link();
            }
        }
    }
    else
    {
        assert(iIdx>=0 && iIdx<iLinkNum_g);
        plink=plink_g+iIdx;
        if(plink->LinkSwitchMode==0)
        {
            if (plink->aucMode==ucMode)
                return EP_SUCCESS;
            else
            {
                sts=SI_Chg_Sts_File_Item(EP_LINK_MODE_FILE, pucLinkModeFileHead_g,
                                         iLinkNum_g+4, iIdx+4, pucMode);
                if (sts!=EP_SUCCESS)
                    return EP_FILE_ERR;
                else
                {
                    plink->aucMode=0;
                    plink->aucMode|=ucMode;
                    SC_Updt_Certain_Link(iIdx);
                }
            }
        }
    }
    if(sts ==EP_SUCCESS)
    {
        unComCrc =0;
        unComCrc=FT_File_CRC16(EP_LINK_MODE_FILE);
        FT_Wr_INI_CRC("[CRC]",CRC_ITEM_LINKMODE,unComCrc);
    }
    return sts;

}

/***********************************************************************
* SC_Chg_Work_Area - Change working setting area.
*
* RETURNS:
*               EP_SUCCESS, change work setting area OK.
*               EP_FILE_ERR, file operating failure.
*
*/
EP_STATUS SC_Chg_Work_Area(
    int iArea		/* new working setting area number. */
)
{
    /*2013-7-23  ZY 去掉assert 调用 */
    int iFd;
    int i;
    STATUS vxsts;
    uint8_t aucBuf[FULL_NAME_LEN+1];
    uint16_t unComCrc;
    char new_tmp_filename[100];

    if(!(iArea>=SET_AREA_START_NO && iArea<scinfo_g.iMaxSetArea))
    {
        LOG_Dbg_Msg("错误，定值区切换的定值区号越界!",0,0,0,0,0,0);
        return  EP_FILE_ERR;
    }

    if (iArea!=scinfo_g.iWorkSetArea)
    {
        strcpy(new_tmp_filename,"");
        Bak_File_Name(new_tmp_filename,EP_FUNC_STS_FILE,".bkf");

        if(FT_Is_File(new_tmp_filename))
        {
            remove(new_tmp_filename);
        }

        FT_Cpy_File(EP_FUNC_STS_FILE, new_tmp_filename);

        if ((iFd=open(new_tmp_filename, O_WRONLY, 0))!=ERROR)
        {
            i=lseek(iFd, 81*2+64, SEEK_SET);
            //	assert(i==81*2+64);
            if(!(i==81*2+64))
            {
                vxsts=close(iFd);
                return  EP_FILE_ERR;
            }

            aucBuf[0]=iArea/16+'0';
            if (aucBuf[0]>'9')
                aucBuf[0]+=('a'-'9'-1);

            aucBuf[1]=iArea%16+'0';
            if (aucBuf[1]>'9')
                aucBuf[1]+=('a'-'9'-1);

            memset(aucBuf+2, ' ', 14);

            Set_FunSts_Wr_Sts(1);

            i=write(iFd, aucBuf, 16);
            //assert(i==16);
            if(!(i==16))
            {
                vxsts=close(iFd);
                return  EP_FILE_ERR;
            }

            vxsts=close(iFd);
            //assert(vxsts==OK);

            remove(EP_FUNC_STS_FILE);
            rename(new_tmp_filename, EP_FUNC_STS_FILE);

            unComCrc =0;
            unComCrc=FT_File_CRC16(EP_FUNC_STS_FILE);
            if(FT_Wr_INI_CRC("[CRC]",CRC_ITEM_FUNC,unComCrc)!=EP_SUCCESS)
            {
                return EP_ERROR;
            }
            Set_FunSts_Wr_Sts(0);

            scinfo_g.iWorkSetArea=iArea;
            scinfo_g.iRealWorkSetArea=scinfo_g.iWorkSetArea;
            scinfo_g.iNextWorkArea=-1;  		/* 整定定值完毕后，也要恢复-1 ghx */
            if(SC_Updt_Work_Set()!=EP_SUCCESS)
            {
                scinfo_g.iRealWorkSetArea=0xFF;
                return EP_FILE_ERR;
            }
        }
        else
            return EP_FILE_ERR;
    }

    return EP_SUCCESS;
}

/***********************************************************************
* SC_Updt_Work_Set - 更新定值区
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
* Alert:
*        TODO: need this function reentrant?
*
*/
static EP_STATUS SC_Updt_Work_Set(void)
{
    /*2013-7-23  ZY 去掉assert */
    uint8_t aucFileName[FULL_NAME_LEN+1];
    int iFd;
    uint8_t aucBuf[10];
    int i;
    int iIdx;
    int iSet;
    SC_SET_PAGE *psetpg;
    SC_SET_ITEM *pset;
    STATUS vxsts;

    if(!(scinfo_g.iWorkSetArea>=SET_AREA_START_NO && scinfo_g.iWorkSetArea<scinfo_g.iMaxSetArea))
    {
        return EP_FILE_ERR;
    }

    sprintf(aucFileName, EP_SET_AREA_DIR "/area%02x.dza", scinfo_g.iWorkSetArea);

    iFd=open(aucFileName, O_RDONLY, 0);

    /*
    if(!(iFd!=ERROR && SC_Is_Valid_Set(iFd)))
    {
    	return EP_FILE_ERR;
    }
    */
    /*原来实现有缺陷，ZY修改,2013-7-25 */
    if(iFd==ERROR)
    {
        return EP_FILE_ERR;
    }
    if(!SC_Is_Valid_Set(iFd))
    {
        vxsts=close(iFd);
        return EP_FILE_ERR;
    }

    lseek(iFd, 10, SEEK_SET);

    for (iIdx=0; iIdx<iSetPgNum_g-1; iIdx++)
    {
        i=read(iFd, aucBuf, 4);
        //assert(i==4);
        if(!(i==4))
        {
            vxsts=close(iFd);
            return EP_FILE_ERR;
        }

        psetpg=psetpg_g+aucBuf[0]+1;

        for (iSet=0; iSet<psetpg->iSetNum; iSet++)
        {
            i=read(iFd, aucBuf, 9);
            //assert(i==9);
            if(!(i==9))
            {
                vxsts=close(iFd);
                return EP_FILE_ERR;
            }

            pset=psetpg->pset+U8_TO_U16(aucBuf[3], aucBuf[2]);
            /* 如果用于修改比例系数等的字符串尾控制字被修改，需要重新
            	 修改ai、ao、遥测、测量配置和db中的这些值 */
            if(pCoefTailSet_g==pset &&  pset->valNow.ulVal!=BYTES_TO_U32(aucBuf+5))
                bCoefTailChg=TRUE;
            pset->valNow.ulVal=BYTES_TO_U32(aucBuf+5);

            if(pset->ucUnit==0x68)
            {
                /*2008-7-16日张云修改,支持字符串定值  */

                if(pset->valNow.ulVal>MAX_ID_LEN)
                {
                    vxsts=close(iFd);
                    //assert(vxsts==OK);
                    return  EP_ERROR;
                }
                i=read(iFd, pset->aucNowStr, pset->valNow.ulVal);
                //assert(i==pset->valNow.ulVal);
                if(!(i==pset->valNow.ulVal))
                {
                    vxsts=close(iFd);
                    return  EP_ERROR;
                }
            }
        }
    }

    vxsts=close(iFd);
    //assert(vxsts==OK);

    RE_SetLogSetChgCnt();

    SC_Updt_Each_GivenSetting_Decided_Item();

    return  EP_SUCCESS;
}


/***********************************************************************
* SC_Updt_Work_Set - 更新定值区
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
* Alert:
*        TODO: need this function reentrant?
*
*/
static EP_STATUS SC_Updt_Work_Set_And_Log(int iArea)
{
    /*2013-7-23  ZY 去掉assert */
    uint8_t aucFileName[FULL_NAME_LEN+1];
    int iFd;
    uint8_t aucBuf[10];
    int i;
    int iIdx;
    int iSet;
    SC_SET_PAGE *psetpg;
    SC_SET_ITEM *pset;
    STATUS vxsts;

    if(!(scinfo_g.iWorkSetArea>=SET_AREA_START_NO && scinfo_g.iWorkSetArea<scinfo_g.iMaxSetArea))
    {
        return EP_FILE_ERR;
    }

    sprintf(aucFileName, EP_SET_AREA_DIR "/area%02x.dza", scinfo_g.iWorkSetArea);

    iFd=open(aucFileName, O_RDONLY, 0);
    /*
    if(!(iFd!=ERROR && SC_Is_Valid_Set(iFd)))
    {
    	return EP_FILE_ERR;
    }
    */
    /*原来实现有缺陷，ZY修改,2013-7-25 */
    if(iFd==ERROR)
    {
        return EP_FILE_ERR;
    }
    if(!SC_Is_Valid_Set(iFd))
    {
        vxsts=close(iFd);
        return EP_FILE_ERR;
    }

    SetModifiesToLog(iFd, iArea);
    lseek(iFd, 10, SEEK_SET);
    for (iIdx=0; iIdx<iSetPgNum_g-1; iIdx++)
    {
        i=read(iFd, aucBuf, 4);
        //assert(i==4);
        if(!(i==4))
        {
            vxsts=close(iFd);
            return EP_FILE_ERR;
        }

        psetpg=psetpg_g+aucBuf[0]+1;

        for (iSet=0; iSet<psetpg->iSetNum; iSet++)
        {
            i=read(iFd, aucBuf, 9);
            //assert(i==9);
            if(!(i==9))
            {
                vxsts=close(iFd);
                return EP_FILE_ERR;
            }

            pset=psetpg->pset+U8_TO_U16(aucBuf[3], aucBuf[2]);
            /* 如果用于修改比例系数等的字符串尾控制字被修改，需要重新
            	 修改ai、ao、遥测、测量配置和db中的这些值 */
            if(pCoefTailSet_g==pset &&  pset->valNow.ulVal!=BYTES_TO_U32(aucBuf+5))
                bCoefTailChg=TRUE;

            pset->valNow.ulVal=BYTES_TO_U32(aucBuf+5);

            if(pset->ucUnit==0x68)
            {
                /*2008-7-16日张云修改,支持字符串定值  */

                if(pset->valNow.ulVal>MAX_ID_LEN)
                {
                    vxsts=close(iFd);
                    //assert(vxsts==OK);
                    return  EP_ERROR;
                }
                i=read(iFd, pset->aucNowStr, pset->valNow.ulVal);
                //assert(i==pset->valNow.ulVal);
                if(!(i==pset->valNow.ulVal))
                {
                    vxsts=close(iFd);
                    return  EP_ERROR;
                }
            }
        }
    }


    vxsts=close(iFd);
    //assert(vxsts==OK);

    SC_Updt_Each_GivenSetting_Decided_Item();

    return  EP_SUCCESS;
}

/***********************************************************************
* SC_Updt_Link - 更新压板
*
* RETURNS: EP_SUCCESS, or EP_ERROR
*
*/
static EP_STATUS SC_Updt_Link(void)
{
    uint8_t hwlinkmode;
    SC_LINK_ITEM *plink;
    BOOL bFirstDi;
    BOOL bSecondDi;
    static  uint8_t  aucLogInfo_s[129];/*2006-11-8日 张云修改  */
    uint8_t ucLinkMode=0;
    int nNum=0;
    BOOL bSts = FALSE;
    VI_RUN_INFO *pinf;

    for(plink=plink_g,nNum=0; plink<plink_g+iLinkNum_g; plink++,nNum++)
    {
        if (ulTotalLinkMode_g & LINK_MODE_CUS)  /*如果总压板模式是定制模式，读取各具体压板的模式*/
            ucLinkMode=plink->aucMode;
        else    /*否则采用总压板模式*/
            ucLinkMode=ulTotalLinkMode_g;
        switch(plink->LinkSwitchMode)
        {
            case 1:
                ucLinkMode=LINK_MODE_HW;
                break;
            case 2:
                ucLinkMode=LINK_MODE_SW;
                break;
            case 3:
                ucLinkMode=LINK_MODE_AND;
                break;
            case 4:
                ucLinkMode=LINK_MODE_OR;
                break;
            default:
                break;
        }

        switch(ucLinkMode)
        {
            case LINK_MODE_HW:
                hwlinkmode=plink->HwLinkType;		/* 硬压板时的模式*/
                if( !(hwlinkmode&0x01))
                {
                    /* 置0为单开入*/
                    if(plink->pvHdDI == NULL) 		/* 当硬压板未定义，则认为该硬压板退出 */
                    {
                        bSts=FALSE;
                    }
                    else
                    {
                        bFirstDi=RD_Get_DI(plink->pvHdDI);
                        if(!(hwlinkmode&0x02))
                        {
                            /* 置0为正逻辑 */
                            bSts=bFirstDi;
                        }
                        else
                        {
                            /* 负逻辑 */
                            bSts=!bFirstDi;
                        }
                    }
                }
                else
                {
                    /* 置1为双开入，开入之间采用与逻辑 */
                    if(plink->pvHdDI == NULL) /* 当硬压板未定义，则认为该硬压板退出 */
                    {
                        bSts=FALSE;
                    }
                    else
                    {
                        bFirstDi=RD_Get_DI(plink->pvHdDI);
                        if(plink->pvHdSecondDI == NULL) /* 当硬压板未定义，则认为该硬压板退出 */
                        {
                            bSts=FALSE;
                        }
                        else
                        {
                            bSecondDi=RD_Get_DI(plink->pvHdSecondDI);
                            switch(hwlinkmode&0x06)
                            {
                                case 0:
                                    bSts=bFirstDi && bSecondDi;
                                    break;

                                case 2:
                                    bSts=(!bFirstDi) && bSecondDi;
                                    break;

                                case 4:
                                    bSts=bFirstDi && (!bSecondDi);
                                    break;

                                case 6:
                                    bSts=(!bFirstDi) && (!bSecondDi);
                                    break;

                                default:
                                    assert(FALSE);
                                    break;
                            }
                        }
                    }
                }
                break;

            case LINK_MODE_SW:
                bSts=plink->bSwVal;
                break;

            case LINK_MODE_AND:
                hwlinkmode=plink->HwLinkType;		/* 硬压板时的模式*/
                if( !(hwlinkmode&0x01))
                {
                    /* 置0为单开入*/
                    if(plink->pvHdDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                    {
                        bSts=FALSE;
                    }
                    else
                    {
                        bFirstDi=RD_Get_DI(plink->pvHdDI);
                        if(!(hwlinkmode&0x02))
                        {
                            /* 置0为正逻辑 */
                            bSts=bFirstDi;
                        }
                        else
                        {
                            /* 负逻辑 */
                            bSts=!bFirstDi;
                        }
                    }
                    bSts=(bSts&&plink->bSwVal);
                }
                else
                {
                    /* 置1为双开入 */
                    if(plink->pvHdDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                    {
                        bSts=FALSE;
                    }
                    else
                    {
                        bFirstDi=RD_Get_DI(plink->pvHdDI);
                        if(plink->pvHdSecondDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                        {
                            bSts=FALSE;
                        }
                        else
                        {
                            bSecondDi=RD_Get_DI(plink->pvHdSecondDI);
                            switch(hwlinkmode&0x06)
                            {
                                case 0:
                                    bSts=bFirstDi && bSecondDi && plink->bSwVal;
                                    break;

                                case 2:
                                    bSts=(!bFirstDi) && bSecondDi && plink->bSwVal;
                                    break;

                                case 4:
                                    bSts=bFirstDi && (!bSecondDi) && plink->bSwVal;
                                    break;

                                case 6:
                                    bSts=(!bFirstDi) && (!bSecondDi) && plink->bSwVal;
                                    break;

                                default:
                                    assert(FALSE);
                                    break;
                            }
                        }
                    }
                }
                break;

            case LINK_MODE_OR:
                hwlinkmode=plink->HwLinkType;		/* 硬压板时的模式*/
                if( !(hwlinkmode&0x01))
                {
                    /* 置0为单开入*/
                    if(plink->pvHdDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                    {
                        bSts=FALSE;
                    }
                    else
                    {
                        bFirstDi=RD_Get_DI(plink->pvHdDI);
                        if(!(hwlinkmode&0x02))
                        {
                            /* 置0为正逻辑 */
                            bSts=bFirstDi;
                        }
                        else
                        {
                            /* 负逻辑 */
                            bSts=!bFirstDi;
                        }
                    }
                    bSts=(bSts||plink->bSwVal);
                }
                else
                {
                    /* 置1为双开入 */
                    if(plink->pvHdDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                    {
                        bFirstDi=FALSE;
                    }
                    else
                    {
                        bFirstDi=RD_Get_DI(plink->pvHdDI);
                    }

                    if(plink->pvHdSecondDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                    {
                        bSecondDi=FALSE;
                    }
                    else
                    {
                        bSecondDi=RD_Get_DI(plink->pvHdSecondDI);
                    }

                    switch(hwlinkmode&0x06)
                    {
                        case 0:
                            bSts=bFirstDi || bSecondDi || plink->bSwVal;
                            break;

                        case 2:
                            bSts=(!bFirstDi) || bSecondDi || plink->bSwVal;
                            break;

                        case 4:
                            bSts=bFirstDi || (!bSecondDi) || plink->bSwVal;
                            break;

                        case 6:
                            bSts=(!bFirstDi) || (!bSecondDi) || plink->bSwVal;
                            break;

                        default:
                            assert(FALSE);
                            break;
                    }
                }
                break;

            default:
                return EP_ERROR;
                break;
        }
        if (bSts!=plink->bNowVal)
        {
            plink->bNowVal=bSts;
            if(*(pLinkFirstAccessFlag_g+nNum))
            {
                /* 若不是首次访问的变位信息,认为是有效的,否则认为是无效 */
                pinf=VI_Run_Info_Wr_P();

                if (bViewModIsInit_g)	/* VI模块是否完成标志 */
                {
                    pinf->bViewModIsInit = TRUE;
                }
                else
                {
                    pinf->bViewModIsInit = FALSE;
                }

                pinf->type=LINK_CHG;

                pinf->msg.link.pcfg=plink;
                pinf->msg.link.ucCOT=1;
                pinf->msg.link.bSts=bSts;
                pinf->msg.link.ulTime=TM_Get_usCnt();/* 原来的实现屏蔽,时间取实际时间,这样报告组织就不会乱掉 */
                /*pinf->msg.link.ulTime=ulScnTime;*/
                pinf->msg.link.unCh=nNum;

                VI_End_Wr_Run_Info();

                if(ENG_MODE==0)
                    sprintf(aucLogInfo_s, "压板\"%s\"变位, 压板号是%d, 变位后状态是%d. \n", plink->aucName, nNum, pinf->msg.link.bSts);/*2006-11-8日 张云修改  */
                else if(ENG_MODE==1)
                    sprintf(aucLogInfo_s, "switch \"%s\"(No. %d) changed, current state is %d.\n. \n", plink->aucName, nNum, pinf->msg.link.bSts);/*2006-11-8日 张云修改  */
                LOG_Write(LOG_RUN, aucLogInfo_s,NULL);

                /* 设置刷新 */
                RE_SetLogSetChgCnt();
                HDL_SetGooseDiNeedRefresh(TRUE);
            }
        }

    }

    return EP_SUCCESS;
}

/*
DQ:
Description: 根据压板模式状态计算某个压板有效值,当硬压板未定义,则认为该硬压板退出;
*/
static void SC_Updt_Certain_Link(int iIdx)
{
    SC_LINK_ITEM *plink;
    BOOL bSts = FALSE;
    VI_RUN_INFO *pinf;
    static  uint8_t  aucLogInfo_s[129];/*2006-11-8日 张云修改  */
    uint8_t ucLinkMode=0;
    uint8_t hwlinkmode;
    BOOL bFirstDi;
    BOOL bSecondDi;

    assert(iIdx<iLinkNum_g&&iIdx>=0);

    plink=plink_g+iIdx;

    if (ulTotalLinkMode_g & LINK_MODE_CUS)  /*如果总压板模式是定制模式，读取各具体压板的模式*/
        ucLinkMode=plink->aucMode;
    else    /*否则采用总压板模式*/
        ucLinkMode=ulTotalLinkMode_g;
    switch(plink->LinkSwitchMode)
    {
        case 1:
            ucLinkMode=LINK_MODE_HW;
            break;
        case 2:
            ucLinkMode=LINK_MODE_SW;
            break;
        case 3:
            ucLinkMode=LINK_MODE_AND;
            break;
        case 4:
            ucLinkMode=LINK_MODE_OR;
            break;
        default:
            break;
    }
    switch(ucLinkMode)
    {
        case LINK_MODE_HW:
            hwlinkmode=plink->HwLinkType;		/* 硬压板时的模式*/
            if( !(hwlinkmode&0x01))
            {
                /* 置0为单开入*/
                if(plink->pvHdDI == NULL) 		/* 当硬压板未定义，则认为该硬压板退出 */
                {
                    bSts=FALSE;
                }
                else
                {
                    bFirstDi=RD_Get_DI(plink->pvHdDI);
                    if(!(hwlinkmode&0x02))
                    {
                        /* 置0为正逻辑 */
                        bSts=bFirstDi;
                    }
                    else
                    {
                        /* 负逻辑 */
                        bSts=!bFirstDi;
                    }
                }
            }
            else
            {
                /* 置1为双开入，开入之间采用与逻辑 */
                if(plink->pvHdDI == NULL) /* 当硬压板未定义，则认为该硬压板退出 */
                {
                    bSts=FALSE;
                }
                else
                {
                    bFirstDi=RD_Get_DI(plink->pvHdDI);
                    if(plink->pvHdSecondDI == NULL) /* 当硬压板未定义，则认为该硬压板退出 */
                    {
                        bSts=FALSE;
                    }
                    else
                    {
                        bSecondDi=RD_Get_DI(plink->pvHdSecondDI);
                        switch(hwlinkmode&0x06)
                        {
                            case 0:
                                bSts=bFirstDi && bSecondDi;
                                break;

                            case 2:
                                bSts=(!bFirstDi) && bSecondDi;
                                break;

                            case 4:
                                bSts=bFirstDi && (!bSecondDi);
                                break;

                            case 6:
                                bSts=(!bFirstDi) && (!bSecondDi);
                                break;

                            default:
                                assert(FALSE);
                                break;
                        }
                    }
                }
            }
            break;

        case LINK_MODE_SW:
            bSts=plink->bSwVal;
            break;

        case LINK_MODE_AND:
            hwlinkmode=plink->HwLinkType;		/* 硬压板时的模式*/
            if( !(hwlinkmode&0x01))
            {
                /* 置0为单开入*/
                if(plink->pvHdDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                {
                    bSts=FALSE;
                }
                else
                {
                    bFirstDi=RD_Get_DI(plink->pvHdDI);
                    if(!(hwlinkmode&0x02))
                    {
                        /* 置0为正逻辑 */
                        bSts=bFirstDi;
                    }
                    else
                    {
                        /* 负逻辑 */
                        bSts=!bFirstDi;
                    }
                }
                bSts=(bSts&&plink->bSwVal);
            }
            else
            {
                /* 置1为双开入 */
                if(plink->pvHdDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                {
                    bSts=FALSE;
                }
                else
                {
                    bFirstDi=RD_Get_DI(plink->pvHdDI);
                    if(plink->pvHdSecondDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                    {
                        bSts=FALSE;
                    }
                    else
                    {
                        bSecondDi=RD_Get_DI(plink->pvHdSecondDI);
                        switch(hwlinkmode&0x06)
                        {
                            case 0:
                                bSts=bFirstDi && bSecondDi && plink->bSwVal;
                                break;

                            case 2:
                                bSts=(!bFirstDi) && bSecondDi && plink->bSwVal;
                                break;

                            case 4:
                                bSts=bFirstDi && (!bSecondDi) && plink->bSwVal;
                                break;

                            case 6:
                                bSts=(!bFirstDi) && (!bSecondDi) && plink->bSwVal;
                                break;

                            default:
                                assert(FALSE);
                                break;
                        }
                    }
                }
            }
            break;

        case LINK_MODE_OR:
            hwlinkmode=plink->HwLinkType;		/* 硬压板时的模式*/
            if( !(hwlinkmode&0x01))
            {
                /* 置0为单开入*/
                if(plink->pvHdDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                {
                    bSts=FALSE;
                }
                else
                {
                    bFirstDi=RD_Get_DI(plink->pvHdDI);
                    if(!(hwlinkmode&0x02))
                    {
                        /* 置0为正逻辑 */
                        bSts=bFirstDi;
                    }
                    else
                    {
                        /* 负逻辑 */
                        bSts=!bFirstDi;
                    }
                }
                bSts=(bSts||plink->bSwVal);
            }
            else
            {
                /* 置1为双开入 */
                if(plink->pvHdDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                {
                    bFirstDi=FALSE;
                }
                else
                {
                    bFirstDi=RD_Get_DI(plink->pvHdDI);
                }

                if(plink->pvHdSecondDI == NULL) /*当硬压板未定义，则认为该硬压板退出*/
                {
                    bSecondDi=FALSE;
                }
                else
                {
                    bSecondDi=RD_Get_DI(plink->pvHdSecondDI);
                }

                switch(hwlinkmode&0x06)
                {
                    case 0:
                        bSts=bFirstDi || bSecondDi || plink->bSwVal;
                        break;

                    case 2:
                        bSts=(!bFirstDi) || bSecondDi || plink->bSwVal;
                        break;

                    case 4:
                        bSts=bFirstDi || (!bSecondDi) || plink->bSwVal;
                        break;

                    case 6:
                        bSts=(!bFirstDi) || (!bSecondDi) || plink->bSwVal;
                        break;

                    default:
                        assert(FALSE);
                        break;
                }
            }
            break;

        default:/* 张云2008-1-29日 merge 合并修改，去掉以前有返回的BUG */
            return ;
            break;
    }
    if (bSts!=plink->bNowVal)
    {
        plink->bNowVal=bSts;
        if(*(pLinkFirstAccessFlag_g+iIdx))
        {
            /* 若不是首次访问的变位信息,认为是有效的,否则认为是无效 */
            pinf=VI_Run_Info_Wr_P();

            if (bViewModIsInit_g)	/* VI模块是否完成标志 */
            {
                pinf->bViewModIsInit = TRUE;
            }
            else
            {
                pinf->bViewModIsInit = FALSE;
            }

            pinf->type=LINK_CHG;

            pinf->msg.link.pcfg=plink;
            pinf->msg.link.ucCOT=1;
            pinf->msg.link.bSts=bSts;
            pinf->msg.link.ulTime=TM_Get_usCnt();/* 原来的实现屏蔽,时间取实际时间,这样报告组织就不会乱掉 */
            /*pinf->msg.link.ulTime=ulScnTime;*/
            pinf->msg.link.unCh=iIdx;

            VI_End_Wr_Run_Info();

            if(ENG_MODE==0)
                sprintf(aucLogInfo_s, "压板\"%s\"变位, 压板号是%d, 变位后状态是%d. \n", plink->aucName, iIdx, pinf->msg.link.bSts);		/*2006-11-8日 张云修改  */
            else if(ENG_MODE==1)
                sprintf(aucLogInfo_s, "switch \"%s\"(No. %d) changed, current state is %d.\n", plink->aucName, iIdx, pinf->msg.link.bSts);		/*2006-11-8日 张云修改  */
            LOG_Write(LOG_RUN, aucLogInfo_s,NULL);

            /* 设置刷新 */
            RE_SetLogSetChgCnt();
            HDL_SetGooseDiNeedRefresh(TRUE);
        }
    }
}

/***********************************************************************
* SC_Valid_Area_Num - Get total valid setting area number.
*
* RETURNS: Number of total valid setting area.
*
*/
int SC_Valid_Area_Num(void)
{
    int i;
    int iNum;

    iNum=0;
    for (i=0; i<scinfo_g.iMaxSetArea; i++)
    {
        if (scinfo_g.aucSetAreaFg[i] & SET_VALID)
            iNum++;
        LOG_Dbg_Msg("%d=%x\n", i, scinfo_g.aucSetAreaFg[i], 3, 4, 5, 6);
    }

    return iNum;
}

/***********************************************************************
* SC_Get_Valid_Area - Get every valid setting area number.
*
* RETURNS: Number of total valid setting area.
*
*/
int SC_Get_Valid_Area(
    uint8_t *pucRslt			/* to save setting area number result. */
)
{
    int i;
    int iNum;

    iNum=0;
    for (i=0; i<scinfo_g.iMaxSetArea; i++)
    {
        if (scinfo_g.aucSetAreaFg[i] & SET_VALID)
        {
            iNum++;
            *pucRslt++=i;
        }
    }

    return iNum;
}

/***********************************************************************
* SC_Is_Valid_Area - Check if the number of setting area is valid.
*
* RETURNS:
*               TRUE, the setting area is valid.
*               FALSE, the setting area is NOT valid.
*
*/
BOOL SC_Is_Valid_Area(
    int iArea		/* number of setting area. */
)
{
    if (iArea>=0 && iArea<scinfo_g.iMaxSetArea &&
            scinfo_g.aucSetAreaFg[iArea] & SET_VALID)
        return TRUE;
    else
        return FALSE;
}

/***********************************************************************
* SC_Del_Set_Area - Delete a setting area.
*
* RETURNS:
*       		   EP_SUCCESS, delete OK.
*               EP_PARM_ERR, iArea is the working area, can't delete.
*               EP_FILE_ERR, the area file not exists.
*
*/
EP_STATUS SC_Del_Set_Area(
    int iArea		/* number of setting area. */
)
{
    //ZY去掉assert调用
    uint8_t aucFileName[FULL_NAME_LEN+1];
    STATUS vxsts;

    assert(iArea>=0 && iArea<scinfo_g.iMaxSetArea);

    if (iArea==scinfo_g.iWorkSetArea)
        return EP_PARM_ERR;
    else
    {
        sprintf(aucFileName, EP_SET_AREA_DIR "/area%02x.dza", iArea);

        if (!FT_Is_File(aucFileName))
            return EP_FILE_ERR;
        else
        {
            vxsts=remove(aucFileName);
            //assert(vxsts==OK);

            scinfo_g.aucSetAreaFg[iArea] &= ~(SET_HAVE_FILE | SET_VALID);
        }
    }

    return EP_SUCCESS;
}

/***********************************************************************
* SC_Work_Set_Area - Get working setting area number.
*
* RETURNS: Working setting area number.
*
* Alert:
*        Working setting area may be not a valid setting area.  This error is reported by other way.
*
*/
int SC_Work_Set_Area(void)
{
    return scinfo_g.iWorkSetArea;
}

/***********************************************************************
* SC_Real_Work_Set_Area - Get real working setting area number.
*
* RETURNS: Real working setting area number.
*
* Alert:
*        Real working setting area may be not a valid setting area.  This error is reported by other way.
*
*/
int SC_Real_Work_Set_Area(void)
{
    return scinfo_g.iRealWorkSetArea;
}

/* 更新索引定值页,初始化或更新定值后调用(一般定值/内部定值/测控定值)
 * Para:
 *     NONE.
 * Return:
 *     TRUE, or FALSE.
* Alert:
 *     初始化或定值更新后,如果索引定值区号进行了切换
 *     则对应的各项AI、AO、遥测、测量等相关系数应进行更新.
*/
BOOL SC_Updt_Each_GivenSetting_Decided_Item(void)
{
    EP_STATUS stsRet=EP_SUCCESS;
    static uint8_t aucLogInfo[256];

    /* 不需要更新 */
    if(!(bCoefTailChg || AdMdType.bChgFlag))
    {
        return FALSE;
    }
    else
    {
        /* 置为TRUE则执行以下程序更新
         * 调用调整AI、AO项的函数
         */

        /* 配置了控制定值字 */
        if(bCoefTailChg)
        {
            sprintf(aucLogInfo, "切换索引定值至第%d页.\n",
                    (int)pCoefTailSet_g->valNow.ulVal);
        }
        else
        {
            /* 使用edp01.ini文件保存索引定值页序 */
            sprintf(aucLogInfo, "切换索引定值至第%d页.\n",
                    (int)AdMdType.iCurrentType);
        }

        LOG_Write(LOG_KERNEL, aucLogInfo, NULL);

        /* 修改AI通道系数/AO通道系数/DI消抖系数 */
        if(RD_Chg_Coff() != EP_SUCCESS)
            stsRet=EP_CFG_ERR;

        /* 修改遥测项 */
        VI_Chg_Mea_Some_Attrs();
        /* 修改测量项 */
        ME_Chg_Msu_Some_Attrs();

        /* 更新DSP使用系数,保证实时更新 */
        UpdateAcCoff();

        /* 保证本机已经完成处理 */
        bCoefTailChg=FALSE;
        AdMdType.bChgFlag=FALSE;

        return TRUE;
    }
}

/***********************************************************************
* SC_Set_Next_Work_Area - 设置即将切换的定值区，逻辑图中调用
*
* RETURNS: 无
*
*/
void SC_Set_Next_Work_Area(
    int iArea		/* 切换定值区 */
)
{
    taskLock();
    scinfo_g.iNextWorkArea=iArea;
    taskUnlock();
}

/***********************************************************************
* SC_Set_Work_Area_LogicScan - 逻辑图扫描中切换定值区
*
* RETURNS: 无
*
*/
void SC_Set_Work_Area_LogicScan(
    int nSettingArea		/* 待切换定值区 */
)
{
    SLOW_MESSAGE_NODE Info;

    Info.type=SETAREACHG;			/* 定值区切换 */
    Info.param1=nSettingArea;				/* 切换定值区号 */
    msgQSend(SlowMessage, (char *)&Info, sizeof(SLOW_MESSAGE_NODE), NO_WAIT, MSG_PRI_NORMAL);
}

/***********************************************************************
* SC_Next_Work_Set_Area - 即将切换的定值区，mmi调用
*
* RETURNS: 下一个定值区
*
*/
int SC_Next_Work_Set_Area(void)
{
    return scinfo_g.iNextWorkArea;
}

/***********************************************************************
* SC_CK_SY_SET - 检查索引定值的配置及在ai，ao，遥测，测量配置的使用是否正确
*
* RETURNS: EP_SUCCESS, or EP_CFG_ERR
*
*/
EP_STATUS SC_CK_SY_SET(void)
{
    int iSYSetPgNum;  		/* 索引定值页数 */
    SC_SET_ITEM *pset;
    SC_SY_SETBASE_ITEM *psyset;
    int i,iSyNo;
    uint8_t ucTemp;

    if(!pCoefTailSet_g)
        return EP_SUCCESS;
    iSYSetPgNum=pCoefTailSet_g->valMax.ulVal+1;

    /* 在内部定值中查找计算每页索引定值个数 */
    for(i=0; i<psetpg_g->iSetNum; i++)
    {
        pset=psetpg_g->pset+i;
        if(pset->ucAttr!=0)
            break;
    }
    iSyNo=i;
    assert(!((psetpg_g->iSetNum-i)%iSYSetPgNum));
    iSyseBaseNum_g=(psetpg_g->iSetNum-i)/iSYSetPgNum; 			/* 每页索引定值的个数 */

    if ((psysetbase_g=calloc(iSyseBaseNum_g, sizeof(*psysetbase_g)))==NULL)
    {
        return EP_BUF_ERR;
    }
    for(i=0,psyset=psysetbase_g; i<iSyseBaseNum_g; i++,psyset++)
    {
        pset=psetpg_g->pset+i+iSyNo;
        ucTemp=strlen(pset->aucId);
        memcpy(psyset->aucName,pset->aucId,ucTemp);
        psyset->aucName[ucTemp-1]='\0';
    }

    if(RD_Ck_Coff() != EP_SUCCESS)
        return EP_CFG_ERR;

    if(VI_CK_Mea_Attrs() != EP_SUCCESS)
        return EP_CFG_ERR;

    if(ME_CK_Mea_Attrs() != EP_SUCCESS)
        return EP_CFG_ERR;

    return EP_SUCCESS;
}

/***********************************************************************
* SC_Find_Setbase - 在索引定值中匹配每个使用的字符串基
*
* RETURNS: EP_SUCCESS, or EP_CFG_ERR
*
*/
EP_STATUS SC_Find_Setbase(
    uint8_t *strBaseID,		/* ID */
    BOOL *pRtFlagRepeated				/* 是否使用重复 */
)
{
    int i;

    *pRtFlagRepeated=FALSE;
    for (i=0; i<iSyseBaseNum_g; i++)
    {
        if (!strcmp(psysetbase_g[i].aucName, strBaseID))
        {
            if(psysetbase_g[i].bUsed)
                *pRtFlagRepeated=TRUE;

            psysetbase_g[i].bUsed=TRUE;
            return EP_SUCCESS;
        }
    }

    return EP_CFG_ERR;
}

/***********************************************************************
* EP_Set_YBTT_Flag - 设置压板投退标志，逻辑图中设置
*
* RETURNS: 无
*
*/
void EP_Set_YBTT_Flag(
    int iYbNum
)
{
    LogicYbTT_g.bYaBanTTFlag=TRUE;
    LogicYbTT_g.YBState[iYbNum]=TRUE;
}

/***********************************************************************
* EP_Clr_YBTT_Flag - 恢复压板投退标志，慢速任务中执行压板投退操作后恢复
*
* RETURNS: 无
*
*/
void EP_Clr_YBTT_Flag(
    int iYbNum
)
{
    LogicYbTT_g.bYaBanTTFlag=FALSE;
    LogicYbTT_g.YBState[iYbNum]=FALSE;
}

/***********************************************************************
* EP_Get_YBTT_Flag - 获取压板投退状态
*
* RETURNS: 指针
*
* alert:
*        得到压板投退标志,慢速任务根据该标志来进行压板投退操作
*/
LOGICYBTT *EP_Get_YBTT_Flag()
{
    return &LogicYbTT_g;
}

/***********************************************************************
* ChangeLgcRunState - 改变逻辑图运行状态
*
* RETURNS: 无
*
*/
void ChangeLgcRunState(void)
{

}

/***********************************************************************
* GetMmiNeedUpdateFlag - 获取是否需要更新相关配置标志
*
* RETURNS: TRUE, or FALSE
*
*/
BOOL GetMmiNeedUpdateFlag(void)
{
    return AdMdType.bMmiUpdateFlag;
}

/***********************************************************************
* ClearMmiNeedUpdateFlag - 清除是否需要更新相关配置标志
*
* RETURNS: NONE
*
*/
void ClearMmiNeedUpdateFlag(void)
{
    AdMdType.bMmiUpdateFlag=FALSE;
}

/*读取并解析国网六统一选配文件*/
char strFuncOpt[32];    /*解析后用于存储选配代码的字符串，因为目前
                                        张云止考虑单字符作为选配代码，所以32长度足够*/
char strFuncOptShow[32]; /* 解析后用于存储用于显示的选配代码的字符串 */
UINT32 gulReadFuncOptFlag; /*解析FuncOpt.ini是否成功的标志*/
/*0=解析成功;
    1=功能选配文件不存在;
    2=解析失败;*/
/*
    修改:1.增加存储用于显示的选配代码字符串的生成,用于CCD校验工作
           张全     2016/1/4
*/
void ReadFile_FuncOptIni()
{
    int iSum=0;
    char aucTmpStr[32]="0";
    char aucTmpStr2[32]="0";
    strcpy(strFuncOpt,"");
    strcpy(strFuncOptShow,"");

    if ((FT_Is_File(EP_FUNCOPTION_FILE)) && (FT_Rd_INI(EP_FUNCOPTION_FILE,NULL,"[FUNCLIST_O]", "FUNCSUM", aucTmpStr, 32)>0))
    {
        int i;
        char strVarName[32];
        char strVarName2[32];
        iSum=strtol(aucTmpStr,NULL,10);
        for (i=1; i<=iSum; i++)
        {
            sprintf(strVarName,"FUNCOPT_%02d",i);
            sprintf(strVarName2,"FUNCSHOW_%02d", i);
            if (FT_Rd_INI(EP_FUNCOPTION_FILE,NULL,"[FUNCLIST_O]", strVarName, aucTmpStr, 32)>0)
            {
                if (strcmp(aucTmpStr,"1")==0)
                {
                    sprintf(strVarName,"FUNCCODE_%02d",i);
                    if (FT_Rd_INI(EP_FUNCOPTION_FILE,NULL,"[FUNCLIST_O]", strVarName, aucTmpStr, 32)>0)
                    {
                        strcat(strFuncOpt,aucTmpStr);

                        if (FT_Rd_INI(EP_FUNCOPTION_FILE,NULL,"[FUNCLIST_O]", strVarName2, aucTmpStr2, 32)>0)
                        {
                            /* 只有读取出的字段数值明确为0的情况才不显示 */
                            if(strcmp(aucTmpStr2,"0") != 0)
                            {
                                strcat(strFuncOptShow, aucTmpStr);
                            }
                        }
                        else
                        {
                            strcat(strFuncOptShow, aucTmpStr);
                        }

                    }
                    else
                    {
                        printf("\n Failed to read strVarName=%s\n",strVarName);
                        gulReadFuncOptFlag=2;
                        break;
                    }
                }
            }
            else
            {
                printf("\n Failed to read strVarName=%s\n",strVarName);
                gulReadFuncOptFlag=2;
                break;
            }
        }
    }
    else
    {
        printf("\n 保护功能选配文件不存在!\n");
        gulReadFuncOptFlag=1;
    }
    if (gulReadFuncOptFlag!=0)
    {
        LOG_Write(LOG_KERNEL, "保护功能选配文件解析失败或不存在!", NULL);
    }
}

/*功能：获取技术支持配置的选配功能代码
参数：pRtCodeStr:供返回的选配功能代码字符串，字符串空间由调用方分
配，被调用方填充，字符串以"\0"结尾。
           iStrMaxLen：调用方分配的选配功能代码字符串空间最大长度。
返回：成功与否  ,失败原因:
            0=解析成功;
            1=功能选配文件不存在;
            2=解析选配文件失败;
            3=传人参数错误;*/
UINT32  EP_GetFuncOptCode(uint8_t *pRtCodeStr,int  iStrMaxLen)
{
    UINT32 ulRtn=3;
    if ((pRtCodeStr !=NULL) && (iStrMaxLen>= strlen(strFuncOpt)))
    {
        strcpy(pRtCodeStr,strFuncOpt);
        ulRtn=gulReadFuncOptFlag;
    }
    return(ulRtn);
}

void testsdm()
{
    printf("选配功能字符串:%s; 标志=%d;\n",strFuncOpt,(int)gulReadFuncOptFlag);
}

/* 获取国网三压板状态.
 * Para:
 *     NONE.
 * Return:
 *     EP_SUCEESS, or EP_ERROR/EP_FILE_ERR.
 */
EP_STATUS FT_Get_GWYB_Sts(BOOL *pbRmtSwitchYB, BOOL *pbRmtSwitchSetting)
{
    uint8_t *pucBuf = NULL;
    uint8_t *puc = NULL;
    uint32_t ulLen;
    uint8_t ucItem = 0;
    uint8_t len;
    EP_STATUS stsRet = EP_SUCCESS;

    if ((pucBuf = FT_File_To_Mem(EP_LOCAL_STS_FILE, &ulLen)) == NULL)
        return EP_FILE_ERR;

    puc = pucBuf;

    /* 文件有效性标识 */
    if (*puc++ != 0x01)
    {
        stsRet = EP_ERROR;
        goto ret;
    }

    puc += 4;

    /* 条目总个数, 不限制 */
    ucItem = *puc++;

    /* 第一条 */
    if (*puc++ != 0x01)
    {
        stsRet = EP_ERROR;
        goto ret;
    }

    /* 第一条目名称 */
    len = *puc++;
    puc += len;

    /* 压板类型 */
    if (*puc++ != 0x10)
    {
        stsRet = EP_ERROR;
        goto ret;
    }

    /* 远方投退压板软压板状态 */
    *pbRmtSwitchYB = *puc++;

    puc += 10;

    /* 第二条 */
    if (*puc++ != 0x02)
    {
        stsRet = EP_ERROR;
        goto ret;
    }

    /* 第二条目名称 */
    len = *puc++;
    puc += len;

    /* 压板类型 */
    if (*puc++ != 0x10)
    {
        stsRet = EP_ERROR;
        goto ret;
    }

    /* 远方切换定值区软压板 */
    *pbRmtSwitchSetting = *puc++;

ret:
    if (pucBuf)
    {
        free(pucBuf);
    }

    return stsRet;
}
