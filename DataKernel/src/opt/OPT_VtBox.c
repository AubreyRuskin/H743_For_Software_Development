/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*       OPT_VtBox.c                                    1.0                      */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了光纵虚拟机箱模块的代码文件                                     */
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
/*         张云       2006.2.8                创建文件1.0版本                 */
/*                                                                             */
/*                                                                              */
/********************************************************************************/

#include  "OPT_MmiInterface.h"
#include   "OPT_VtBox.h"
#include  "OPT_SamSyn.h"
#include  "realdata.h"
#include  "swcfg.h"
#include  "RE_PublicDataDef.h"
#include  "scc_hdlc_raw.h"
#include  "math_compat.h"
#include  "taskLib.h"
#include  "OPT_SynAdapt.h"
#include  "OPT_Com.h"
#include  "AppInterface.h"
#include  "intLib.h"
/***************************光纵机箱AO配置结构  ****************************/


/*光纵机箱AI的比例系数表  */
OPT_HWAI_COFF_CFG  aBoxAiCoffTable[2][MAX_OPT_AI_NUM];/*按物理通道序  */
OPT_BOX_AO_CFG   BoxAoCfgOpt_g[2];  /*光纵AO配置  */

int   iOptTxPts_g;                        /*光纵每次发送数据点数  */
int   iOptSysFreq_g;                      /* 系统频率 */

int   iOptChType_g;                  /*通道类型,0为64K，1为2M　*/

int   iOptSamRate_g;                      /*正常系统采样率   */
int   iNormalSamPeriod_g;                   /*正常系统采样周期  ，ns为单位 */
int iNormalSamPeriod_us_g; /* 正常采样周期, 以us为单位 */
int   iNormalSamCntPerWave_g;
int   iFrCntPerWave_g;                  /*通信时，每周波帧数目  */
int   iSynAverageFrCnt_g;                /*光纵同步求平均值帧数  */
float  fSynAverageCoff_g;                 /*光纵同步求平均值系数  */

float   fSpeedSamRate_g;/*加快采样时的采样频率  */
int     iSpeedSamPeriod_g;/*加快采样时的采样周期  ，以NS为单位*/
int iSpeedSamPeriod_us_g; /* 加快采样周期, 以us为单位 */
float   fSlowSamRate_g;  /*降低采样时的采样频率  */
int     iSlowSamPeriod_g;  /*降低采样时的采样周期 ，以NS为单位 */
int iSlowSamPeriod_us_g; /* 降低采样周期, 以us为单位 */

BOOL  abOptChIsInit_g[2]= {FALSE,FALSE}; /*通道设置标志  */
BOOL  abOptChIsInitOver_g[2]= {FALSE,FALSE}; /*通道初始化完成标志  */

int   iOptAioDataByteLen_g;             /*AIO实际数据长度　*/
int   iOptDioDataByteLen_g;	              /* DIO实际数据长度  */
int   iOptDataByteLen_g;                 /* 所有实际数据长度  */
uint32_t  ulOptFrBaudSendTime_g;           /*帧的硬件BAUD发送时间，单位US 2006-12-21日  */

int  iOptAiCh_g;                         /*光纵AI个数  */

int  iOptDiCh_g;                         /*光纵DI个数  */
uint8_t * apucOptDiStsBase_g[2];             /*光纵接收DI数据基址  */

int  iOptAoCh_g;                         /*光纵AO个数  */
int  iOptAISrcAoCh_g;                    /*AI来源的AO数目  */
uint8_t  * pucOptAoDataByteBase_g;       /*光纵AI来源的AO发送数据基址  */
EP_ELEM_IO *  apMidSrcAOPt_g[2][MAX_OPT_AO_NUM];  /*光纵AO的中间结果来源的指针数组，按AO硬件地址排列，为空，表示没有  */

int  iOptDoCh_g;                          /*光纵DO个数  */
uint8_t * apucOptDoStsBase_g[2];              /*光纵发送DO数据基址 */
SEM_ID  semOptSndPtsDat_g;

/*光纵通信任务检测  */
BOOL    bOptCh1SendTaskStartFlag_g=FALSE;
BOOL    bOptCh1RecvTaskStartFlag_g=FALSE;
BOOL    bOptCh2RecvTaskStartFlag_g=FALSE;
BOOL   GetOptCh1SendTaskStatus();
BOOL   GetOptCh1RecvTaskStatus();
BOOL   GetOptCh2RecvTaskStatus();

static  int  nOptCh1SendTaskID_g;
static  int  nOptCh1RecvTaskID_g;
static  int  nOptCh2RecvTaskID_g;

uint8_t  aucZerofloat_g[4];/*按INTEL次序的浮点，低字节在前，和MOTO次序反的  */

OPT_ALL_AI_SRC_AO_CFG  OptAllAiSrcAoCfg_g;   /*所有AI来源的AO配置变量 2006-11-27日张云 */
BOOL   abOptChAllAiPhyCoffValidFlagArr[2];/*光纵通道所有AI来源的AI比例系数有效标志，为TRUE，表示通道的所有AI比例系数都有效，否则无效 2006-11-27日张云 */

/* 光纵接收初始化 */
static  EP_STATUS  OptRecvInit(RD_AI_MOD  *pAiMod, int  OptCh);

/* 光纵发送初始化  */
static  EP_STATUS  OptSendInit(void);


/* 初始化（并启动）光纵虚拟机箱,光纵机箱2的AO，DO配置也起作用
 * 参数：   iOptChNum ,光纵通道号，0为光纵通道1，1为光纵通道2，其他无效
 *          iOptChType,光纵通道类型，0为64K，1为2M，其他无效
            uiSmplRate，实际采样每周波采样点数
 *          uiSysFreq，系统频率
 *          uiTxPts, 光纵每次发送点数
 *          pvAiMod，该模块（光纵虚拟机箱负责的所有AI采集/计算通道）的句柄
 *          uiLgcCh，光纵AI采样的逻辑通道数
 *          plgccfg，指向光纵AI逻辑通道配置数组第0个元素的指针，数组元素有unLgcCh个
 *          uiCalcCfg，光纵AI预处理通道配置数，光纵的应该无预处理通道   目前应该为0
 *          pcalccfg，指向光纵AI预处理通道配置数组第0个元素的指针，数组元素uiCalcCfg个
 *          iOptAoCh,  光纵AO的数目
 *          pOptAoCfg，指向光纵AO逻辑通道配置数组第0个元素的指针，数组元素有iOptAoCh个,
 *          iOptDiCh，光纵DI的数目
 *          puiOptDiStsBase，指向光纵DI通道的数据发送缓冲基址，每个通道占1位，
 *          iOptDoCh，光纵DO的数目
 *          puiOptDoStsBase，指向光纵DO通道的数据接收缓冲基址，每个通道占1位，
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS   Init_Opt_Box(int  iOptChNum,int  iOptChType,
                         u_int uiSmplRate, u_int uiSysFreq,u_int uiTxPts,
                         void *pvAiMod,
                         u_int uiLgcCh, DSP_LGC_AI_CFG *plgccfg,
                         u_int uiCalcCfg, DSP_CALC_AI_CFG *pcalccfg,
                         int  iOptAoCh,OPT_AO_CFG  *pOptAoCfg,
                         int  iOptDiCh,uint8_t * pucOptDiStsBase,
                         int  iOptDoCh,uint8_t  *pucOptDoStsBase)
{
    char TempInfo[256];

    assert(iOptDiCh==iOptDoCh);
    assert(uiLgcCh==iOptAoCh);

    /*进行发送和接收初始化  */
    if(iOptChNum==0)
    {
        /*初始化通道无关公共配置数据  */
        if(OPT_InitCommonData(iOptChType,
                              uiSmplRate, uiSysFreq,uiTxPts,
                              iOptAoCh,pOptAoCfg,
                              iOptDoCh)!=EP_SUCCESS)
        {
            sprintf(TempInfo, "光纵通道%d的配置失败.\n",iOptChNum+1);
            LOG_Write(LOG_KERNEL, TempInfo, NULL);

            assert(FALSE);
            return  EP_CFG_ERR;
        }

    }
    else  if(iOptChNum==1)
    {
        assert(uiLgcCh==iOptAiCh_g);
        assert(iOptDiCh==iOptDiCh_g);
    }
    else
    {
        assert(FALSE);
    }


    /*初始化通道相关配置数据  */
    if(OPT_InitChCfgData(iOptChNum,
                         pvAiMod,
                         uiLgcCh, plgccfg,
                         uiCalcCfg, pcalccfg,
                         iOptDiCh,pucOptDiStsBase,pucOptDoStsBase)!=EP_SUCCESS)
    {
        LOG_Dbg_Msg("错误，光纵通道%d的配置失败!\n",iOptChNum+1,0,0,0,0,0);


        if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
        {
            sprintf(TempInfo, "fail to configure optical fiber channel %d.\n",iOptChNum+1);
            LOG_Write(LOG_KERNEL, TempInfo, NULL);
        }
        else if(ENG_MODE == 0)
        {
            sprintf(TempInfo, "光纵通道%d的配置失败.\n", iOptChNum+1);
            LOG_Write(LOG_KERNEL, TempInfo, NULL);
        }
        assert(FALSE);
        return  EP_CFG_ERR;
    }


    /*初始化通道相关采样同步数据  */
    if(OPT_InitChComSynData(iOptChNum,pvAiMod)!=EP_SUCCESS)
    {
        LOG_Dbg_Msg("错误，光纵通道%d的配置失败!\n",iOptChNum+1,0,0,0,0,0);
        if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
        {
            sprintf(TempInfo, "fail to configure optical fiber channel %d.\n",iOptChNum+1);
            LOG_Write(LOG_KERNEL, TempInfo, NULL);
        }
        else if(ENG_MODE == 0)
        {
            sprintf(TempInfo, "光纵通道%d的配置失败.\n", iOptChNum+1);
            LOG_Write(LOG_KERNEL, TempInfo, NULL);
        }
        assert(FALSE);
        return  EP_CFG_ERR;
    }


    return  EP_SUCCESS;
}



/* 初始化光纵公共数据
 * 参数：
 *          iOptChType,光纵通道类型，0为64K，1为2M，其他无效
            uiSmplRate，实际采样每周波采样点数
 *          uiSysFreq，系统频率
 *          uiTxPts, 光纵每次发送点数
 *          iOptAoCh,  光纵AO的数目
 *          pOptAoCfg，指向光纵AO逻辑通道配置数组第0个元素的指针，数组元素有iOptAoCh个
 *          iOptDoCh，光纵DO的数目
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS   OPT_InitCommonData(int  iOptChType,
                               u_int uiSmplRate, u_int uiSysFreq,u_int uiTxPts,
                               int  iOptAoCh,OPT_AO_CFG  *pOptAoCfg,
                               int  iOptDoCh)
{

    EP_STATUS    stsResult;
    int  i;
    double   dbTemp1;
    double   dbTemp2;
    double   dbTemp3;
    int  k;
    BOOL   bFindMatchAOCh;


    iOptSysFreq_g=uiSysFreq;
    iOptTxPts_g=uiTxPts;
    iOptChType_g=iOptChType;


    iOptSamRate_g=uiSmplRate*uiSysFreq;
    iNormalSamPeriod_g=1000000000/iOptSamRate_g;/* ns */
    iNormalSamPeriod_us_g = 1000000/iOptSamRate_g;
    iNormalSamCntPerWave_g=iOptSamRate_g/iOptSysFreq_g;

    iFrCntPerWave_g=iNormalSamCntPerWave_g/iOptTxPts_g;
    iSynAverageFrCnt_g=OPT_TSSE_AVERAGE_THRESH/(iNormalSamPeriod_g*iOptTxPts_g/1000);
    fSynAverageCoff_g=1.0/(float)iSynAverageFrCnt_g;

    dbTemp1=(double)1000000.0/(double)iOptSysFreq_g-(double)OPT_SAM_ADJ_TIME_PER_PERIOD;
    dbTemp2=(double)1000000.0*(double)iOptSamRate_g/(double)iOptSysFreq_g;
    dbTemp3=dbTemp2/dbTemp1;
    fSpeedSamRate_g=(float)dbTemp3;                       /*采样加快调整时的采样频率 */
    iSpeedSamPeriod_g=(int)(1000000000.0/fSpeedSamRate_g);   /*采样加快调整时的采样周期，NS  */
    iSpeedSamPeriod_us_g = (int)(1000000.0/fSpeedSamRate_g);   /* 采样加快调整时的采样周期，us */

    dbTemp1=(double)1000000.0/(double)iOptSysFreq_g+(double)OPT_SAM_ADJ_TIME_PER_PERIOD;
    dbTemp3=dbTemp2/dbTemp1;
    fSlowSamRate_g=(float)dbTemp3;  /*采样减慢调整时的采样频率 */
    iSlowSamPeriod_g=(int)(1000000000.0/fSlowSamRate_g);   /*采样加快调整时的采样周期  NS*/
    iSlowSamPeriod_us_g = (int)(1000000.0/fSlowSamRate_g);   /* 采样加快调整时的采样周期, us */


    iOptAiCh_g=iOptAoCh;
    iOptDiCh_g=iOptDoCh;

    iOptDioDataByteLen_g=(MAX_OPT_ALLOW_DIO_NUM)/8;

    /*初始化DO个数  */
    iOptDoCh_g=iOptDoCh;

    /*初始化AO ,此时只配置了ＡＩ来源的ＡＯ */
    iOptAoCh_g=iOptAoCh;
    iOptAISrcAoCh_g=BoxAoCfgOpt_g[0].iOptAISrcAONum;
    for(i=0; i<iOptAoCh_g; i++)
    {
        if(((pOptAoCfg+i)->iAOSrcType==DA_AI_SRC)&&
                (((pOptAoCfg+i)->ucAOHdCh>=BoxAoCfgOpt_g[0].iOptAISrcAONum)
                 ||(((pOptAoCfg+i)->ucAOHdCh)>=MAX_OPT_ALLOW_AIO_NUM)))
        {
            /* 若是AI来源的AO，则要求该类型来源的AO，必须排列在AO硬地址的前面 */
            assert(FALSE);
        }
    }

    /*按AO物理通道次序初始化发送用的AI来源AO配置信息 2006-11-27日 张云 */
    OptAllAiSrcAoCfg_g.iAISrcAoNum=BoxAoCfgOpt_g[0].iOptAISrcAONum;
    OptAllAiSrcAoCfg_g.ppAISrcAoCfgArr=
        calloc(OptAllAiSrcAoCfg_g.iAISrcAoNum,sizeof(*(OptAllAiSrcAoCfg_g.ppAISrcAoCfgArr)));
    if(!(OptAllAiSrcAoCfg_g.ppAISrcAoCfgArr))
    {
        /*若内存不够  */
        assert(FALSE);
    }
    for(i=0; i<OptAllAiSrcAoCfg_g.iAISrcAoNum; i++)
    {
        /*对每个AI来源AO通道进行设置  */
        bFindMatchAOCh=FALSE;
        for(k=0; k<iOptAoCh_g; k++)
        {
            if(((pOptAoCfg+k)->iAOSrcType==DA_AI_SRC)
                    &&((pOptAoCfg+k)->ucAOHdCh==i))
            {
                OptAllAiSrcAoCfg_g.ppAISrcAoCfgArr[i]=pOptAoCfg+k;
                bFindMatchAOCh=TRUE;
                break;
            }
        }
        if(!bFindMatchAOCh)
        {
            /* 若找不到，说明BUG */
            assert(FALSE);
        }
    }


    /*求得AIO数据长度  2006-6-14日作成原始AI直接传浮点AI，2006-11-26日修改传定点*/
    iOptAioDataByteLen_g=iOptTxPts_g*iOptAISrcAoCh_g*2
                         +(iOptAoCh_g-iOptAISrcAoCh_g)*4;

    if(iOptChType==0)
    {
        //assert(FALSE);/*目前考虑1M  ZY*/
        iOptDataByteLen_g=OPT_64K_DATA_HEAD+iOptAioDataByteLen_g+iOptDioDataByteLen_g+1;/*2009-2-15日 ZY，添加1字节应用层校验和  */
        ulOptFrBaudSendTime_g=(iOptDataByteLen_g+4)*8;/*获得硬件BAUD发送时间 单位US 2011-11-28 */
        assert(iOptDataByteLen_g<=MAX_OPT_64K_DATA_LEN);
    }
    else  if(iOptChType==1)
    {
        /**/
        iOptDataByteLen_g=OPT_2M_DATA_HEAD+iOptAioDataByteLen_g+iOptDioDataByteLen_g+1;/*2009-2-15日 ZY，添加1字节应用层校验和  */
        ulOptFrBaudSendTime_g=(iOptDataByteLen_g+4)*8/2;/*获得硬件BAUD发送时间 单位US  2006-12-21*/
        assert(iOptDataByteLen_g<=MAX_OPT_2M_DATA_LEN);
    }
    else
    {
        assert(FALSE);
    }

    /*给采样模块初始化AI来源的AO  */ /*  2006-6-14日作成原始AI直接传浮点AI 2006-11-26日修改传定点*/
    stsResult=Init_OptBox_AO(iOptTxPts_g,iOptAoCh,pOptAoCfg, /*只初始化1次  */
                             iOptTxPts_g*iOptAISrcAoCh_g*2,&pucOptAoDataByteBase_g);
    if(stsResult!=EP_SUCCESS)
    {
        return  EP_CFG_ERR;
    }
    return   EP_SUCCESS;
}


/* 初始化光纵通道配置数据
 * 参数：   iOptChNum ,光纵通道号，0为光纵通道1，1为光纵通道2，其他无效
 *          pvAiMod，该模块（光纵虚拟机箱负责的所有AI采集/计算通道）的句柄
 *          uiLgcCh，光纵AI采样的逻辑通道数
 *          plgccfg，指向光纵AI逻辑通道配置数组第0个元素的指针，数组元素有unLgcCh个
 *          uiCalcCfg，光纵AI预处理通道配置数，光纵的应该无预处理通道   目前应该为0
 *          pcalccfg，指向光纵AI预处理通道配置数组第0个元素的指针，数组元素uiCalcCfg个
 *          iOptDiCh，光纵DI的数目
 *          puiOptDiStsBase，指向光纵DI通道的数据接收缓冲基址，每个通道占1位，
 *          puiOptDoStsBase，指向光纵Do通道的数据发送缓冲基址，每个通道占1位，
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS   OPT_InitChCfgData(int  iOptChNum,
                              void *pvAiMod,
                              u_int uiLgcCh, DSP_LGC_AI_CFG *plgccfg,
                              u_int uiCalcCfg, DSP_CALC_AI_CFG *pcalccfg,
                              int  iOptDiCh,uint8_t * pucOptDiStsBase,uint8_t * pucOptDoStsBase)
{
    int  i;
    if((iOptChNum==0)&&(pvAiMod==&aimodOpt_g[0]))
    {
        abOptChIsInit_g[0]=TRUE;
        assert(uiLgcCh<=MAX_OPT_ALLOW_AIO_NUM);/*目前只允许配置个AI通道  */
        assert(iOptDiCh<=MAX_OPT_ALLOW_DIO_NUM);/*目前只允许配置个DI通道  */
    }
    else  if((iOptChNum==1)&&(pvAiMod==&aimodOpt_g[1]))
    {
        abOptChIsInit_g[1]=TRUE;
        assert(uiLgcCh<=MAX_OPT_ALLOW_AIO_NUM);/*目前只允许配置个AI通道  */
        assert(iOptDiCh<=MAX_OPT_ALLOW_DIO_NUM);/*目前只允许配置个DI通道  */
    }
    else
    {
        assert(FALSE);
    }

    /*初始化AI  */
    for(i=0; i<MAX_OPT_AI_NUM; i++)
    {
        aBoxAiCoffTable[iOptChNum][i].ucLgcAI=-1;
        aBoxAiCoffTable[iOptChNum][i].fCoff=1.0;
    }
    for(i=0; i<uiLgcCh; i++)
    {
        assert((((plgccfg+i)->ucHdCh)-1)<MAX_OPT_ALLOW_AIO_NUM);/*目前只允许配置个AI通道  */
        aBoxAiCoffTable[iOptChNum][((plgccfg+i)->ucHdCh)-1].ucLgcAI=i;
        aBoxAiCoffTable[iOptChNum][((plgccfg+i)->ucHdCh)-1].fPhyCoff=(plgccfg+i)->fCoff;/*2006-11-26日张云修改，光纵通上后，由对侧传来  */

        /* 传统采样时需处理模拟/数字转换系数 */
        if (appType_g == APP_TYPE_TRAD)
        {
            aBoxAiCoffTable[iOptChNum][((plgccfg+i)->ucHdCh)-1].fCoff=
                (aBoxAiCoffTable[iOptChNum][((plgccfg+i)->ucHdCh)-1].fPhyCoff)
                *5.0/32768.0;/*plgccfg->ucHdCh从1开始  */
        }
        else if (appType_g == APP_TYPE_DIG)
        {
            aBoxAiCoffTable[iOptChNum][((plgccfg+i)->ucHdCh)-1].fCoff =
                (aBoxAiCoffTable[iOptChNum][((plgccfg+i)->ucHdCh)-1].fPhyCoff);
        }
        else
        {
            assert (FALSE);
        }

        aBoxAiCoffTable[iOptChNum][((plgccfg+i)->ucHdCh)-1].bPhyCoffIsValid=FALSE;/* 2006-11-27日张云 */
    }
    abOptChAllAiPhyCoffValidFlagArr[iOptChNum]=FALSE;/* 2006-11-27日张云 */

    /*初始化DI数据  */
    apucOptDiStsBase_g[iOptChNum]=pucOptDiStsBase;

    /*初始化DO数据  */
    apucOptDoStsBase_g[iOptChNum]=pucOptDoStsBase;

    return   EP_SUCCESS;
}



/*  初始化光纵AO所有配置
     参数：iOptChNum ,光纵通道号，0为光纵通道1，1为光纵通道2，其他无效
          iAOCfgNum:所有AO配置个数
     返回：成功与否*/
EP_STATUS   OPT_InitAOCfg(int  iOptChNum,int  iAOCfgNum)
{
    OPT_BOX_AO_CFG   *pBoxAOCfg = NULL;
    int  i;
    OPT_AO_CFG    *pAOCfg;

    if(iOptChNum==0)
    {
        pBoxAOCfg=BoxAoCfgOpt_g;
    }
    else  if(iOptChNum==1)
    {

        pBoxAOCfg=BoxAoCfgOpt_g+1;
    }
    else
    {
        assert(FALSE);
    }
    assert(iAOCfgNum<=MAX_OPT_AO_NUM);
    pBoxAOCfg->iOptAONum=iAOCfgNum;
    pBoxAOCfg->iOptAISrcAONum=0;
    pBoxAOCfg->iOptAISrcAONumTmp=0;
    pBoxAOCfg->iOptMidSrcAONum=0;

    pAOCfg=pBoxAOCfg->aOptBoxAoCfg_g;
    for(i=0; i<MAX_OPT_AO_NUM; i++) /*2006-7-29日修改  */
    {
        pAOCfg->ucAOHdCh=0xff;
        pAOCfg->iAOSrcType=DA_VOID_SRC;
        pAOCfg->ucSrcAIHdCh=0xff;
        pAOCfg->pElemSrc=NULL;
        pAOCfg++;
    }
    return  EP_SUCCESS;
}




/*通知光纵AO初始化完成,在逻辑图初始化完成之后，逻辑图运行之前调用
  参数：无
  返回：成功与否
*/
EP_STATUS   OPT_AOCfgInitFinish()
{
    int  i;
    OPT_AO_CFG  *pAOCfg;
    EP_STATUS   stsResult;
    float   fVirtualValue=0.0;


    if (isNumber_2_04CPU())
    {
        /* Do not initialize the second board. */
        abOptChIsInit_g[0] = FALSE;
        abOptChIsInit_g[1] = FALSE;
    }

    FLT_TO_BYTES(aucZerofloat_g, fVirtualValue);
    if((!(abOptChIsInit_g[0]))&&(!(abOptChIsInit_g[1])))
    {
        /*若没有初始化光纵，则直接返回  2006-6-14日，使得当未配置光纵时，也能初始化*/
        return   EP_SUCCESS;
    }
    /*此时中间结果和ＡＩ来源的ＡＯ都配置完成了  */

    pAOCfg=BoxAoCfgOpt_g[0].aOptBoxAoCfg_g;
    for(i=0; i<MAX_OPT_AO_NUM; i++)
    {
        if(pAOCfg->iAOSrcType==DA_MID_SRC)
        {
            assert(pAOCfg->ucAOHdCh<BoxAoCfgOpt_g[0].iOptAONum);
            apMidSrcAOPt_g[0][pAOCfg->ucAOHdCh]=pAOCfg->pElemSrc;
        }
        else
        {
            if(pAOCfg->ucAOHdCh<MAX_OPT_AO_NUM)
            {
                /*张云2006-7-29日修改  */
                apMidSrcAOPt_g[0][pAOCfg->ucAOHdCh]=NULL;
            }
        }
        pAOCfg++;
    }
    pAOCfg=BoxAoCfgOpt_g[1].aOptBoxAoCfg_g;
    for(i=0; i<MAX_OPT_AO_NUM; i++)
    {
        if(pAOCfg->iAOSrcType==DA_MID_SRC)
        {
            assert(pAOCfg->ucAOHdCh<BoxAoCfgOpt_g[1].iOptAONum);
            apMidSrcAOPt_g[1][pAOCfg->ucAOHdCh]=pAOCfg->pElemSrc;
        }
        else
        {
            if(pAOCfg->ucAOHdCh<MAX_OPT_AO_NUM)
            {
                /*张云2006-7-29日修改  */
                apMidSrcAOPt_g[1][pAOCfg->ucAOHdCh]=NULL;
            }
        }
        pAOCfg++;
    }

    /*初始化光纵同步自适应功能  */
    stsResult=OPT_InitSynAdapt();
    if(stsResult!=EP_SUCCESS)
    {
        assert(FALSE);
    }


    /*设置光纵通道,必须一起初始化 */
    if(abOptChIsInit_g[0])
    {
        /*若通道1初始化了，  */
        aOptChStsRpt_g[0].bChInitFlag=TRUE;/*2007-10-22 DQ，设置通道0已配置标志  */
        aOptChStsRpt_g[1].bChInitFlag = TRUE; /* 通道0配置成功, 则无论如何通道1均显示通信值 */

        OPT_SetAllPhyCoffInValid(0);/*2006-12-5日张云，设置比例系数无效标志  */

        aOptChInitTimeBase_g[0]=OptGetBaseTimerCnt();
        /*应陈新之BSP初始化要求,需要先初始化网口速率,再初始化HDLC  2011-11-29 ZY  */

        if(iOptChType_g==0)
        {
            /*若是64K通道  */
            if(hdlc_clk_select(CHAN_UP, CLK_64KHZ)!=0)
            {
                LOG_Dbg_Msg("错误，光纵通道%d的HDLC64K速率设置失败!\n",1,0,0,0,0,0);
                if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
                {
                    LOG_Write(LOG_KERNEL, "Error, fail to configure HDLC 64K rate in optical fiber channel 1.\n", NULL);
                }
                else if(ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "错误,光纵通道1的HDLC64K速率设置失败.\n", NULL);
                }
                return  EP_CFG_ERR;
            }
        }
        else   if(iOptChType_g==1)
        {
            /*若是2M通道  */
            if(hdlc_clk_select(CHAN_UP, CLK_2MHZ)!=0)
            {
                LOG_Dbg_Msg("错误，光纵通道%d的HDLC2M速率设置失败!\n",1,0,0,0,0,0);

                if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
                {
                    LOG_Write(LOG_KERNEL, "Error, fail to configure HDLC 2M rate in optical fiber channel 1.\n", NULL);
                }
                else if(ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "错误,光纵通道1的HDLC2M速率设置失败.\n", NULL);
                }
                return  EP_CFG_ERR;
            }
        }
        else
        {
            assert(FALSE);
        }

        // if(m8260SccHdlcInit(4)!=0)
        // {
        //     LOG_Dbg_Msg("错误，光纵通道%d的HDLC初始化失败!\n",1,0,0,0,0,0);
        //     if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
        //     {
        //         LOG_Write(LOG_KERNEL, "fail to initialize HDLC in optical fiber channel 1.\n", NULL);
        //     }
        //     else if(ENG_MODE == 0)
        //     {
        //         LOG_Write(LOG_KERNEL, "错误,光纵通道1的HDLC初始化失败.\n", NULL);
        //     }
        //     return  EP_CFG_ERR;
        // }


        if(hdlc_clk_master_set(CHAN_UP, bOptHdlcClkIsMaster_g[0])!=0)
        {
            /* 2009-4-14  ZY */
            LOG_Dbg_Msg("错误，光纵通道%d的HDLC时钟主从设置失败!\n",1,0,0,0,0,0);

            if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
            {
                LOG_Write(LOG_KERNEL, "fail to configure HDLC clock master/slave in optical fiber channel 1.\n", NULL);
            }
            else if(ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "错误,光纵通道1的HDLC时钟主从设置失败.\n", NULL);
            }
            return  EP_CFG_ERR;
        }

    }

    /* 初始化光纵接收任务 */
    if(abOptChIsInit_g[0])
    {
        /*若通道1初始化了，  */

        stsResult=OptRecvInit(&(aimodOpt_g[0]),0);
        if(stsResult!=EP_SUCCESS)
        {
            return  EP_CFG_ERR;
        }
    }

    /*启动发送任务  */
    stsResult=OptSendInit();/*只初始化1次  */
    if(stsResult!=EP_SUCCESS)
    {
        return  EP_CFG_ERR;
    }

    /*通知采样模块可以进行发送  */

    OPT_NotifyStartSendAO();

    LOG_Write(LOG_KERNEL, "光纵通道1初始化成功.\n", NULL);

    return   EP_SUCCESS;
}

/***********************************************************************
* OPTCh2_AOCfgInitFinish - 光纵机箱通道2完成初始化完成
*
* RETURNS:
*			EP_SUCCESS, 正常返回
*			EP_BUF_ERR, 错误
*
*/
EP_STATUS OPTCh2_AOCfgInitFinish(void)
{
    EP_STATUS   stsResult;

    if(abOptChIsInit_g[1])
    {
        aOptChStsRpt_g[1].bChInitFlag=TRUE;/*2007-10-22 DQ，设置通道1已配置标志  */
        OPT_SetAllPhyCoffInValid(1);/*2006-12-5日张云，设置比例系数无效标志  */

        aOptChInitTimeBase_g[1]=OptGetBaseTimerCnt();
        /*应陈新之BSP初始化要求,需要先初始化网口速率,再初始化HDLC  2011-11-29 ZY  */
        if(iOptChType_g==0)
        {
            /*若是64K通道  */
            if(hdlc_clk_select(CHAN_DOWN, CLK_64KHZ)!=0)
            {
                LOG_Dbg_Msg("错误，光纵通道%d的HDLC64K速率设置失败!\n",2,0,0,0,0,0);
                if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
                {
                    LOG_Write(LOG_KERNEL, "Error, fail to configure HDLC 64K rate in optical fiber channel 2.\n", NULL);
                }
                else if(ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "错误，光纵通道2的HDLC64K速率设置失败.\n", NULL);
                }
                return  EP_CFG_ERR;
            }
        }
        else  if(iOptChType_g==1)
        {
            /*若是2M通道  */
            if(hdlc_clk_select(CHAN_DOWN, CLK_2MHZ)!=0)
            {
                LOG_Dbg_Msg("错误，光纵通道%d的HDLC2M速率设置失败!\n",2,0,0,0,0,0);

                if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
                {
                    LOG_Write(LOG_KERNEL, "Error, fail to configure HDLC 2M rate in optical fiber channel 2.\n", NULL);
                }
                else if(ENG_MODE == 0)
                {
                    LOG_Write(LOG_KERNEL, "错误，光纵通道2的HDLC2M速率设置失败.\n", NULL);
                }
                return  EP_CFG_ERR;
            }
        }
        else
        {
            assert(FALSE);
        }
        // if(m8260SccHdlcInit(3)!=0)
        // {
        //     LOG_Dbg_Msg("错误，光纵通道%d的HDLC初始化失败!\n",2,0,0,0,0,0);

        //     if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
        //     {
        //         LOG_Write(LOG_KERNEL, "fail to initialize HDLC in optical fiber channel 2.\n", NULL);
        //     }
        //     else if(ENG_MODE == 0)
        //     {
        //         LOG_Write(LOG_KERNEL, "错误,光纵通道2的HDLC初始化失败.\n", NULL);
        //     }
        //     return  EP_CFG_ERR;
        // }

        if(hdlc_clk_master_set(CHAN_DOWN, bOptHdlcClkIsMaster_g[1])!=0)
        {
            /* 2009-4-14  ZY */
            LOG_Dbg_Msg("错误，光纵通道%d的HDLC时钟主从设置失败!\n",2,0,0,0,0,0);
            if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
            {
                LOG_Write(LOG_KERNEL, "fail to configure HDLC clock master/slave in optical fiber channel 2.\n", NULL);
            }
            else if(ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "错误，光纵通道2的HDLC时钟主从设置失败.\n", NULL);
            }
            return  EP_CFG_ERR;
        }

    }

    if(abOptChIsInit_g[1])
    {
        stsResult=OptRecvInit(&(aimodOpt_g[1]),1);
        if(stsResult!=EP_SUCCESS)
        {
            return  EP_CFG_ERR;
        }

    }

    LOG_Write(LOG_KERNEL, "光纵通道2初始化成功.\n", NULL);

    return EP_SUCCESS;
}

/*光纵接收初始化  */
static EP_STATUS  OptRecvInit(RD_AI_MOD  *pAiMod, int  OptCh)
{
    if(OptCh==0)
    {
        /*无浮点操作，改成中断中处理，张云  2007-10-30日   */

        if(reg_Hdlc_Recv_Fun(CHAN_UP, OptRecvCmd)!=OK)
        {
            if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
            {
                LOG_Write(LOG_KERNEL,"fail to register HDLC receive function of optical fiber channel 1!\n", NULL);

            }
            else if(ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL,"错误，光纵通道1的HDLC接收函数注册失败!\n", NULL);
            }


        }


    }
    else  if(OptCh==1)
    {
        /*无浮点操作，改成中断中处理，张云  2007-10-30日   */

        if(reg_Hdlc_Recv_Fun(CHAN_DOWN, OptRecvCmd)!=OK)
        {

            if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
            {
                LOG_Write(LOG_KERNEL, "fail to register HDLC receive function of optical fiber channel 2.\n", NULL);

            }
            else if(ENG_MODE == 0)
            {
                LOG_Write(LOG_KERNEL, "错误,光纵通道2的HDLC接收函数注册失败.\n", NULL);
            }
        }
    }
    else
    {
        assert(FALSE);

    }
    return  EP_SUCCESS;
}

/*光纵发送初始化  */
static EP_STATUS  OptSendInit(void)
{
    /*
    STATUS vxsts;
        semOptSndPtsDat_g=semCCreate(SEM_Q_PRIORITY, 0);
        assert(semOptSndPtsDat_g!=NULL);

        nOptCh1SendTaskID_g=taskSpawn("tOptSndDat", TSK_OPT_CH_SEND, 0, 100000, OptSendCmd,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

        assert(nOptCh1SendTaskID_g!=ERROR);

        bOptCh1SendTaskStartFlag_g=TRUE;
    */
    return  EP_SUCCESS;
}


BOOL   GetOptCh1SendTaskStatus()
{
    /*获得OptCh1Send_Task的状态,若正常,则返回真,否则,返回假  */
    static  char  strTaskStatus[128];
    if(taskIdVerify(nOptCh1SendTaskID_g)==ERROR)
    {
        /*首先判定该任务是否有效  */
        return   FALSE;
    }
    taskStatusString(nOptCh1SendTaskID_g,strTaskStatus);

    if(strcmp(strTaskStatus,"SUSPEND")==0
            ||strcmp(strTaskStatus,"DELAY+S")==0
            ||strcmp(strTaskStatus,"PEND+S")==0
            ||strcmp(strTaskStatus,"PEND+S+T")==0
            ||strcmp(strTaskStatus,"SUSPEND+I")==0
            ||strcmp(strTaskStatus,"DELAY+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+T+I")==0
            ||strcmp(strTaskStatus,"DEAD")==0)
    {
        return   FALSE;
    }
    else
    {
        return   TRUE;
    }
}


BOOL   GetOptCh1RecvTaskStatus()
{
    /*获得OptCh1Recv_Task的状态,若正常,则返回真,否则,返回假  */
    static  char  strTaskStatus[128];
    if(taskIdVerify(nOptCh1RecvTaskID_g)==ERROR)
    {
        /*首先判定该任务是否有效  */
        return   FALSE;
    }
    taskStatusString(nOptCh1RecvTaskID_g,strTaskStatus);

    if(strcmp(strTaskStatus,"SUSPEND")==0
            ||strcmp(strTaskStatus,"DELAY+S")==0
            ||strcmp(strTaskStatus,"PEND+S")==0
            ||strcmp(strTaskStatus,"PEND+S+T")==0
            ||strcmp(strTaskStatus,"SUSPEND+I")==0
            ||strcmp(strTaskStatus,"DELAY+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+T+I")==0
            ||strcmp(strTaskStatus,"DEAD")==0)
    {
        return   FALSE;
    }
    else
    {
        return   TRUE;
    }
}


BOOL   GetOptCh2RecvTaskStatus()
{
    /*获得OptCh2Recv_Task的状态,若正常,则返回真,否则,返回假  */
    static  char  strTaskStatus[128];
    if(taskIdVerify(nOptCh2RecvTaskID_g)==ERROR)
    {
        /*首先判定该任务是否有效  */
        return   FALSE;
    }
    taskStatusString(nOptCh2RecvTaskID_g,strTaskStatus);

    if(strcmp(strTaskStatus,"SUSPEND")==0
            ||strcmp(strTaskStatus,"DELAY+S")==0
            ||strcmp(strTaskStatus,"PEND+S")==0
            ||strcmp(strTaskStatus,"PEND+S+T")==0
            ||strcmp(strTaskStatus,"SUSPEND+I")==0
            ||strcmp(strTaskStatus,"DELAY+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+I")==0
            ||strcmp(strTaskStatus,"PEND+S+T+I")==0
            ||strcmp(strTaskStatus,"DEAD")==0)
    {
        return   FALSE;
    }
    else
    {
        return   TRUE;
    }
}

/*光纵通道重启
  参数：光纵通道号
  返回：成功与否
*/
BOOL   ResetOptCh(int  iOptChNum)
{
    char TempInfo[256];
    assert(iOptChNum==0||iOptChNum==1);

    /*初始化光纵HDLC通道  */
    if(iOptChNum==0)
    {
        static   uint32_t  ulEnterCnt_s=0;
        ulEnterCnt_s++;

        aOptChInitTimeBase_g[iOptChNum]=OptGetBaseTimerCnt();
        /*应陈新之BSP初始化要求,需要先初始化网口速率,再初始化HDLC  2011-11-29 ZY  */

        if(iOptChType_g==0)
        {
            /*若是64K通道  */
            if(hdlc_clk_select(CHAN_UP, CLK_64KHZ)!=0)
            {
                if((ulEnterCnt_s&0xFFFFF)==1)
                {
                    if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
                    {
                        sprintf(TempInfo, "Error, fail to configure HDLC 64k rate in optical fiber channel %d.\n",iOptChNum+1);
                        LOG_Write(LOG_KERNEL,TempInfo, NULL);
                    }
                    else if(ENG_MODE == 0)
                    {
                        sprintf(TempInfo, "错误,光纵通道%d的HDLC64K速率设置失败.\n",iOptChNum+1);
                        LOG_Write(LOG_KERNEL,TempInfo, NULL);
                    }
                }
                return  FALSE;
            }
        }
        else   if(iOptChType_g==1)
        {
            /*若是2M通道  */
            if(hdlc_clk_select(CHAN_UP, CLK_2MHZ)!=0)
            {
                if((ulEnterCnt_s&0xFFFFF)==1)
                {
                    if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
                    {
                        sprintf(TempInfo, "Error, fail to configure HDLC 2M rate in optical fiber channel %d.\n",iOptChNum+1);
                        LOG_Write(LOG_KERNEL,TempInfo, NULL);
                    }
                    else if(ENG_MODE == 0)
                    {
                        sprintf(TempInfo, "错误,光纵通道%d的HDLC2M速率设置失败.\n",iOptChNum+1);
                        LOG_Write(LOG_KERNEL,TempInfo, NULL);
                    }
                }
                return  FALSE;
            }
        }
        else
        {
            assert(FALSE);
        }

        if(hdlc_channel_reset(4)!=0)
        {
            if((ulEnterCnt_s&0xFFFFF)==1)
            {

                if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
                {
                    sprintf(TempInfo, "Error, HDLC reset failed in optical fiber channel %d.\n",iOptChNum+1);
                    LOG_Write(LOG_KERNEL,TempInfo, NULL);

                }
                else if(ENG_MODE == 0)
                {
                    sprintf(TempInfo, "错误,光纵通道%d的HDLC Reset失败.\n",iOptChNum+1);
                    LOG_Write(LOG_KERNEL,TempInfo, NULL);
                }
            }
            return  FALSE;
        }

        if(hdlc_clk_master_set(CHAN_UP, bOptHdlcClkIsMaster_g[0])!=0)
        {
            /* 2009-4-14  ZY */
            if((ulEnterCnt_s&0xFFFFF)==1)
            {
                if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
                {
                    sprintf(TempInfo, "Error, fail to configure HDLC clock master/slave in optical fiber channel %d.\n",iOptChNum+1);
                    LOG_Write(LOG_KERNEL,TempInfo, NULL);
                }
                else if(ENG_MODE == 0)
                {
                    sprintf(TempInfo, "错误,光纵通道%d的HDLC时钟主从设置失败.\n",iOptChNum+1);
                    LOG_Write(LOG_KERNEL, TempInfo, NULL);
                }
            }
            return  FALSE;

        }
    }
    else  if(iOptChNum==1)
    {
        static   uint32_t  ulEnterCnt_s=0;
        ulEnterCnt_s++;

        aOptChInitTimeBase_g[iOptChNum]=OptGetBaseTimerCnt();
        /*应陈新之BSP初始化要求,需要先初始化网口速率,再初始化HDLC  2011-11-29 ZY  */

        if(iOptChType_g==0)
        {
            /*若是64K通道  */
            if(hdlc_clk_select(CHAN_DOWN, CLK_64KHZ)!=0)
            {
                if((ulEnterCnt_s&0xFFFFF)==1)
                {

                    if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
                    {
                        sprintf(TempInfo, "Error, fail to configure HDLC 64K rate in optical fiber channel %d.\n",iOptChNum+1);
                        LOG_Write(LOG_KERNEL,TempInfo, NULL);
                    }
                    else if(ENG_MODE == 0)
                    {
                        sprintf(TempInfo, "错误,光纵通道%d的HDLC64K速率设置失败.\n",iOptChNum+1);
                        LOG_Write(LOG_KERNEL, TempInfo, NULL);
                    }
                }
                return  FALSE;
            }
        }
        else  if(iOptChType_g==1)
        {
            /*若是2M通道  */
            if(hdlc_clk_select(CHAN_DOWN, CLK_2MHZ)!=0)
            {
                if((ulEnterCnt_s&0xFFFFF)==1)
                {
                    if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
                    {
                        sprintf(TempInfo, "Error, fail to configure HDLC 2M rate in optical fiber channel %d.\n",iOptChNum+1);
                        LOG_Write(LOG_KERNEL,TempInfo, NULL);
                    }
                    else if(ENG_MODE == 0)
                    {
                        sprintf(TempInfo, "错误,光纵通道%d的HDLC2M速率设置失败.\n",iOptChNum+1);
                        LOG_Write(LOG_KERNEL,TempInfo, NULL);
                    }
                }
                return  FALSE;
            }
        }
        else
        {
            assert(FALSE);
        }

        if(hdlc_channel_reset(3)!=0)
        {
            if((ulEnterCnt_s&0xFFFFF)==1)
            {
                if(ENG_MODE == 1)             /*2007-4-19日 张云修改，为了支持英文版  */
                {
                    sprintf(TempInfo, "Error, HDLC restart failed in optical fiber channel %d.\n",iOptChNum+1);
                    LOG_Write(LOG_KERNEL,TempInfo, NULL);
                }
                else if(ENG_MODE == 0)
                {
                    sprintf(TempInfo, "错误,光纵通道%d的HDLC重启失败.\n",iOptChNum+1);
                    LOG_Write(LOG_KERNEL, TempInfo, NULL);
                }
            }
            return  FALSE;
        }

        if(hdlc_clk_master_set(CHAN_DOWN, bOptHdlcClkIsMaster_g[1])!=0)
        {
            /* 2009-4-14  ZY */
            if((ulEnterCnt_s&0xFFFFF)==1)
            {
                if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
                {
                    sprintf(TempInfo, "Error, fail to configure HDLC clock master/slave in optical fiber channel %d.\n",iOptChNum+1);
                    LOG_Write(LOG_KERNEL,TempInfo, NULL);
                }
                else if(ENG_MODE == 0)
                {
                    sprintf(TempInfo, "错误,光纵通道%d的HDLC时钟主从设置失败.\n",iOptChNum+1);
                    LOG_Write(LOG_KERNEL,TempInfo, NULL);
                }
            }
            return  FALSE;
        }
    }
    else
    {
        assert(FALSE);
    }

    return   TRUE;

}





/*  获得所有光纵AI比例系数是否有效标志  2006-12-2日 张云添加
    参数  iOptCh,通道号
    返回，TRUE，表示所有比例系数都有效，可以使用
          FALSE， 表示不是所有比例系数都有效，不可以使用
 */
BOOL  OPT_GetAllPhyCoffValidFlag(int  iOptCh)
{
    int   i;
    BOOL  bFindPhyCoffInvalid;

    if(abOptChAllAiPhyCoffValidFlagArr[iOptCh])
    {
        return   TRUE;
    }
    else
    {
        bFindPhyCoffInvalid=FALSE;
        for(i=0; i<iOptAISrcAoCh_g; i++)
        {
            if(!(aBoxAiCoffTable[iOptCh][i].bPhyCoffIsValid))
            {
                /*若某通道比例系数无效  */
                bFindPhyCoffInvalid=TRUE;
                break;
            }
        }
        if(bFindPhyCoffInvalid)
        {
            abOptChAllAiPhyCoffValidFlagArr[iOptCh]=FALSE;
            return   FALSE;
        }
        else
        {
            abOptChAllAiPhyCoffValidFlagArr[iOptCh]=TRUE;
            return   TRUE;
        }
    }
}

/*获得控制字设置定值
  参数：pSettingInfo，定值信息指针
         pRtIsMaster，返回控制字状态
  返回，操作成功与否  */
EP_STATUS  OPT_GetCtrlWordSettingValue(SCI_SETTING_INFO_TYPE   *pSettingInfo,BOOL  *pRtIsMaster)
{
    EP_STATUS  OpeResult;
    SCI_SIGNAL_VALUE_TYPE   SettingValue;
    /*根据该定植是一般定植,还是公共定植,获得定植当前值  */
    if((pSettingInfo->ucType)==1)
    {
        /*若该定植是内部定植,则访问该内部定植  */
        OpeResult=SCI_Get_Inner_Setting(
                      pSettingInfo->nNumInPage,
                      &SettingValue);
        /* 确保返回成功 */
        assert(OpeResult==EP_SUCCESS);
        /* 确保返回类型一致 */
        assert(SettingValue.ucAttrib==CONTROL_WORD_SIGNAL);
    }
    else if((pSettingInfo->ucType)==0)
    {
        /*若该定植是外部定植,则访问该外部定植  */

        OpeResult=SCI_Get_General_Setting(
                      pSettingInfo->cPageNum,
                      pSettingInfo->nNumInPage,
                      &SettingValue);
        /* 确保返回成功 */
        assert(OpeResult==EP_SUCCESS);
        /* 确保返回类型一致 */
        assert(SettingValue.ucAttrib==CONTROL_WORD_SIGNAL);

    }
    else
    {
        OpeResult=SCI_Get_CK_Setting(
                      pSettingInfo->nNumInPage,
                      &SettingValue);
        /* 确保返回成功 */
        assert(OpeResult==EP_SUCCESS);
        /* 确保返回类型一致 */
        assert(SettingValue.ucAttrib==CONTROL_WORD_SIGNAL);

    }
    /*将当前更新过的定植作为该输出当前值*/

    if(SettingValue.Value.ulVal==0)
    {
        *pRtIsMaster=FALSE;
    }
    else  if(SettingValue.Value.ulVal==1)
    {
        *pRtIsMaster=TRUE;
    }
    else
    {
        assert(FALSE);
    }
    return  EP_SUCCESS;

}

/*  设置所有光纵AI比例系数无效标志  2006-12-2日 张云添加
    参数  iOptCh,通道号
    返回，无
 */
void  OPT_SetAllPhyCoffInValid(int  iOptCh)
{
    int   i;
    int   iLockKey;

    /*需要数据保护一下,要用INTLOCK，不要用taskLock,因为在中断中调用taskLock会死 */
    iLockKey=intLock();
    abOptChAllAiPhyCoffValidFlagArr[iOptCh]=FALSE;
    for(i=0; i<iOptAISrcAoCh_g; i++)
    {
        aBoxAiCoffTable[iOptCh][i].bPhyCoffIsValid=FALSE;
    }
    intUnlock(iLockKey);

}

/* 禁止光差报文接收.
 * Para:
 *     OptCh, 通道序号.
 * Return:
 *     EP_SUCCESS, or NP_ERROR.
 */
EP_STATUS OPT_DisableRecv(int32_t OptCh)
{
    if (OptCh == 0)
    {
        if (reg_Hdlc_Recv_Fun(CHAN_UP, NULL) != OK)
        {
            LOG_Write(LOG_KERNEL, "错误, 光纵通道1的HDLC接收函数注册失败!\n", NULL);
        }
    }
    else if (OptCh == 1)
    {
        if (reg_Hdlc_Recv_Fun(CHAN_DOWN, NULL) != OK)
        {
            LOG_Write(LOG_KERNEL, "错误, 光纵通道2的HDLC接收函数注册失败!\n", NULL);
        }
    }
    else
    {
        assert(FALSE);

    }
    return  EP_SUCCESS;
}