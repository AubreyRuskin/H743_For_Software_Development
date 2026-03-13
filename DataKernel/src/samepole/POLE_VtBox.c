/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*       POLE_VtBox.c                                    1.0                      */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了同杆并架虚拟机箱初始化模块的代码文件                        */
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
#include  "GO_Interface.h"
//#include  "mutual_61850.h"
#include  "realdata.h"
#include  "POLE_Data.h"
#include  "POLE_VtBox.h"
#include  "logic.h"
#include  "errtest.h"
#include  "GooseInterface.h"


int iPoleDiNum_g=0;
int iPoleDoNum_g=0;
int iPoleAiNum_g=0;
int iPoleAoNum_g=0;

POLE_BOX_AO_CFG   PoleBoxAoCfg_g;  /*同杆并架机箱AO配置  */
POLE_BOX_IO_INFO  PoleBoxIOInfo_g;/*同杆并架机箱IO配置  */
POLE_BOX_AI_CFG   PoleBoxAICfg_g;  /*同杆并架机箱AI配置  */

BOOL   bPoleBoxIsInit_g=FALSE;  /*同杆并架机箱被初始化标志  */

int   iPoleSubGoNum_g=0;   /*同杆并架机箱对应的sub GOOSE 个数 2007-7-3   */
int   aiPoleSubGoIdx_g[MAX_ALLOW_SUB_GO_NUM];/*同杆并架机箱对应的sub GOOSE index 无效 */


/*根据Ai Num,查询所在数据源ACTIVE GOOSE的DA INDEX
   参数： iGoSrcType，   ACTIVE GOOSE源类型，目前只能为SAME_POLE_GO_SRC_TYPE
          iAiNum,        在该数据源active goose的Ai序号，序号从1开始，
          piRtDaIndex,   供返回的在GOOSE da 中的Index
          ppSubMapData, Map Data指针存储地址.
   返回， TRUE，匹配成功
          FALSE，匹配失败*/
extern BOOL   GO_QueryActiveGoDaIdxByAiNum
(uint32_t iGoSrcType,
 int      iAiNum,
 int *piRtDaIndex,
 void **ppSubMapData);

/*根据Di Num,查询所在数据源ACTIVE GOOSE的DA INDEX
   参数： iGoSrcType，   ACTIVE GOOSE源类型，目前只能为SAME_POLE_GO_SRC_TYPE和HDL_BOX_GO_SRC_TYPE
          iDiNum,        在该数据源active goose的Di序号，序号从1开始，
          piRtDaIndex,   供返回的在GOOSE da 中的Index
          ppSubMapData, Map Data指针存储地址.
   返回， TRUE，匹配成功
          FALSE，匹配失败*/
BOOL   GO_QueryActiveGoDaIdxByDiNum(uint32_t iGoSrcType,
                                    int      iDiNum,
                                    int  *   piRtDaIndex,
                                    int *piSubNum,
                                    void **ppSubMapData,
                                    int *piTDaIndex,
                                    void **ppTSubMapData,
                                    VALUETYPE *pValueType,
                                    int **ppiSubYabanIndex,
                                    int  *   piRtDaVtIndex
                                   );

/* 初始化（并启动）同杆并架虚拟机箱
 * 参数：   uiSmplRate，采样速率
 *          uiSysFreq，系统频率
 *          pvAiMod，该模块（机箱负责的所有AI采集/计算通道）的句柄
 *          uiLgcCh，采样的逻辑通道数
 *          plgccfg，指向逻辑通道配置数组第0个元素的指针，数组元素有unLgcCh个
 *          uiCalcCfg，预处理通道配置数
 *          pcalccfg，指向预处理通道配置数组第0个元素的指针，数组元素uiCalcCfg个
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，机箱出错 */
EP_STATUS Init_Pole_Box(u_int uiSmplRate, u_int uiSysFreq,
                        void *pvAiMod, u_int uiLgcCh, DSP_LGC_AI_CFG *plgccfg,
                        u_int uiCalcCfg, DSP_CALC_AI_CFG *pcalccfg)
{
    int  i;
    DSP_LGC_AI_CFG * pCurLgcCfg;
    POLE_AI_HND  *   pAiHdl;
    BOOL   bQuerySuccess;
    char TempInfo[256];

    assert(pvAiMod);
    assert(uiLgcCh<=MAX_POLE_AI_NUM);
    assert(plgccfg);
    assert(uiCalcCfg==0);

    // if(!bInit61850BfRelayIsSuccess_g)
    // {
    //     if(ENG_MODE == 1)             /*2007-4-19日 张云修改，为了支持英文版  */
    //     {
    //         ER_Set_Err(EV_GSE_CONFI_ERR, ER_REPORT | ER_ALARM | ER_LOCK,
    //                    "Parallel device GOOSE error\n", 0, 0);
    //     }
    //     else if(ENG_MODE == 0)
    //     {
    //         ER_Set_Err(EV_GSE_CONFI_ERR, ER_REPORT | ER_ALARM | ER_LOCK,
    //                    "同杆装置GOOSE异常\n", 0, 0);

    //     }
    //     assert(FALSE);
    //     return  EP_CFG_ERR;
    // }

    PoleBoxAICfg_g.iPoleAINum=0;

    pCurLgcCfg=plgccfg;
    for(i=0; i<uiLgcCh; i++)
    {
        assert(pCurLgcCfg->ucFiltNum==0);/*只允许配置原始滤波算法  */

        pAiHdl=&(PoleBoxAICfg_g.aPoleBoxAiHdl[i]);
        pAiHdl->ucHdCh=pCurLgcCfg->ucHdCh;
        pAiHdl->fCoff=pCurLgcCfg->fCoff;
        pAiHdl->ucFiltNum=pCurLgcCfg->ucFiltNum;

        /* 初始化 */
        pAiHdl->iSubDaIdx = -1;
        pAiHdl->pSubMapData = NULL;

        bQuerySuccess=GO_QueryActiveGoDaIdxByAiNum/*查询sub goose  */
                      (SAME_POLE_GO_SRC_TYPE,
                       PoleBoxAICfg_g.iPoleAINum+1,
                       &(pAiHdl->iSubDaIdx),
                       &(pAiHdl->pSubMapData));
        if(!bQuerySuccess)
        {
            /*若查询获得sub goose中的DA INDEX失败*/
            if(ENG_MODE == 1)
            {
                ER_Set_Err(EV_GSE_CONFI_ERR, ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                           "Parallel device GOOSE AI config error\n", 0, 0);
            }
            else if(ENG_MODE == 0)
            {
                ER_Set_Err(EV_GSE_CONFI_ERR, ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                           "同杆装置 GOOSE AI 配置错误\n", 0, 0);
            }
            sprintf(TempInfo,
                    "同杆装置 SUB goose 中查询AI失败,AI号:%d!!\n",
                    PoleBoxAICfg_g.iPoleAINum+1);
            LOG_Write(LOG_KERNEL, TempInfo, NULL);
        }

        PoleBoxAICfg_g.apPoleBoxAIIdx[i]=pAiHdl;

        PoleBoxAICfg_g.iPoleAINum++;
        iPoleAiNum_g++;

        pCurLgcCfg++;
    }

    return   EP_SUCCESS;
}

/*  初始化同杆并架AO所有配置
     参数：
          iAOCfgNum:所有AO配置个数
     返回：成功与否*/
EP_STATUS   POLE_InitAOCfg(int  iAOCfgNum)
{
    assert(iAOCfgNum<=MAX_POLE_AO_NUM);

    PoleBoxAoCfg_g.iPoleAONum=iAOCfgNum;
    PoleBoxAoCfg_g.iOptMidSrcAONum=0;

    return  EP_SUCCESS;
}




/* 初始化同杆并架虚拟机箱中间结果源的AO通道
 * 参数：   iSrcType,来源类型
            uiCh，模件中的通道号
 *          pElemIOSrc,逻辑图中间结果输出指针
 * 返回值： EP_STATUS  成功与否
            EP_SUCCESS,成功，其他失败
*/
EP_STATUS  POLE_Init_Mid_Src_AO(int  iSrcType, u_int uiCh, void  *pElemIOSrc)
{
    assert(uiCh<MAX_POLE_AO_NUM);
    assert(iSrcType==DA_MID_SRC);
    assert(pElemIOSrc);

    PoleBoxAoCfg_g.aPoleBoxAoCfg[uiCh].ucAOHdCh=uiCh;
    PoleBoxAoCfg_g.aPoleBoxAoCfg[uiCh].iAOSrcType=iSrcType;
    PoleBoxAoCfg_g.aPoleBoxAoCfg[uiCh].pElemSrc=pElemIOSrc;
    PoleBoxAoCfg_g.aPoleBoxAoCfg[uiCh].fLstPubVal=0.0;/* 2007-6-25日 张云  */
    PoleBoxAoCfg_g.apPoleBoxAoIdx[uiCh]
        =&(PoleBoxAoCfg_g.aPoleBoxAoCfg[uiCh]);
    PoleBoxAoCfg_g.iOptMidSrcAONum++;
    iPoleAoNum_g++;

    assert(PoleBoxAoCfg_g.iOptMidSrcAONum<=PoleBoxAoCfg_g.iPoleAONum);

    return  EP_SUCCESS;
}


/*通知同杆并架AO初始化完成,在逻辑图初始化完成之后，逻辑图运行之前调用
  参数：无
  返回：成功与否
*/
EP_STATUS   POLE_AOCfgInitFinish()
{
    if(PoleBoxAoCfg_g.iOptMidSrcAONum!=PoleBoxAoCfg_g.iPoleAONum)
    {
        if(ENG_MODE == 1)              /*2007-4-19日 张云修改，为了支持英文版  */
        {
            ER_Set_Err(EV_GSE_CONFI_ERR, ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                       "Parallel device GOOSE AO config error\n", 0, 0);
        }
        else if(ENG_MODE == 0)
        {
            ER_Set_Err(EV_GSE_CONFI_ERR, ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                       "同杆装置 GOOSE AO 配置错误\n", 0, 0);
        }
        LOG_Write(LOG_KERNEL, "同杆装置配置的AO个数和逻辑图设置的实际AO输出个数不匹配!!\n", NULL);
        assert(FALSE);
    }

    if(iPoleDiNum_g!=0
            ||iPoleDoNum_g!=0
            ||iPoleAiNum_g!=0
            ||iPoleAoNum_g!=0)
    {
        bPoleBoxIsInit_g=TRUE;/*2007-6-15日 ，只有设置，且没有任何错误，设置同杆并架机箱初始化成功标志  */
    }

    return  EP_SUCCESS;
}


/* 初始化同杆并架虚拟机箱的IO
 * 参数：
 * 返回值： EP_SUCCESS，正常返回
 *          EP_BUF_ERR，内存错误
 *          EP_COM_ERR，光纵通信出错 */
EP_STATUS  POLE_IO_Initialize()
{
    int  i;
    for(i=0; i<MAX_MOD_NUM; i++)
    {
        PoleBoxIOInfo_g.aPoleIOModInfo[i].type=IDLE_MODULE;
        PoleBoxIOInfo_g.aPoleIOModInfo[i].unDiChNum=0;
        PoleBoxIOInfo_g.aPoleIOModInfo[i].unDoChNum=0;
    }
    PoleBoxIOInfo_g.iPoleDiNum=0;
    PoleBoxIOInfo_g.iPoleDoNum=0;

    return  EP_SUCCESS;
}


/* 初始化同杆并架虚拟机箱DI通道
 * 参数：
            iModAddr，模块硬件地址
 *          uiCh，在本模件内的DO物理通道号，从0开始
 *          ulFilt，去抖动时间，单位us
 * 返回值： 用来索引DI通道的void指针，或者NULL表示调用出错 */
void *POLE_Init_DI(int iModAddr, u_int uiCh, uint32_t ulFilt)
{

    POLE_DI_HND  *pHdl;
    BOOL   bQuerySuccess;

    assert(iModAddr<MAX_MOD_NUM);
    assert(uiCh<MAX_DI_PER_MOD);

    if(PoleBoxIOInfo_g.aPoleIOModInfo[iModAddr].type==DO_MODULE)
    {
        assert(FALSE);
    }

    if(PoleBoxIOInfo_g.aPoleIOModInfo[iModAddr].type==IDLE_MODULE)
    {
        PoleBoxIOInfo_g.aPoleIOModInfo[iModAddr].type=DI_MODULE;
    }
    PoleBoxIOInfo_g.aPoleIOModInfo[iModAddr].unDiChNum++;

    pHdl=&(PoleBoxIOInfo_g.ahPoleDiHandle[iModAddr*MAX_DI_PER_MOD+uiCh]);
    pHdl->ucMod=iModAddr;
    pHdl->ucHdCh=uiCh;
    pHdl->aucFilt[0]=HH8(ulFilt);
    pHdl->aucFilt[1]=HL8(ulFilt);
    pHdl->aucFilt[2]=LH8(ulFilt);
    pHdl->aucFilt[3]=LL8(ulFilt);

    /* 初始化 */
    pHdl->iSubDaIdx = -1;
    pHdl->pSubMapData = NULL;

    bQuerySuccess=GO_QueryActiveGoDaIdxByDiNum
                  (SAME_POLE_GO_SRC_TYPE,
                   PoleBoxIOInfo_g.iPoleDiNum+1,
                   &(pHdl->iSubDaIdx),
                   &(pHdl->iSubNum),
                   &(pHdl->pSubMapData),
                   NULL,
                   NULL,
                   &(pHdl->vtValType),
                   NULL,
                   NULL);
    if(!bQuerySuccess)
    {
        /*若查询获得sub goose中的DA INDEX失败  */
        char TempInfo[256];

        if(ENG_MODE ==1)
        {
            ER_Set_Err(EV_GSE_CONFI_ERR, ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                       "Parallel device GOOSE DI config error\n", 0, 0);
        }
        else if(ENG_MODE ==0)
        {
            ER_Set_Err(EV_GSE_CONFI_ERR, ER_REPORT | ER_ALARM | ER_LOCK | ER_NOLOGWRITE,
                       "同杆装置 GOOSE DI 配置错误\n", 0, 0);
        }

        sprintf(TempInfo,
                "查询同杆装置 SUB goose 中的DI失败,DI号:%d!!\n",
                PoleBoxIOInfo_g.iPoleDiNum+1);
        LOG_Write(LOG_KERNEL, TempInfo, NULL);

        return  (void  *)pHdl;
    }

    PoleBoxIOInfo_g.iPoleDiNum++;
    iPoleDiNum_g++;

    return  (void  *)pHdl;


}

/* 初始化同杆并架虚拟机箱DO通道
 * 参数：
 *          iModAddr，模件硬件地址,
 *          uiCh，在本模件内的DO物理通道号，从0开始
 * 返回值： 用来索引DO通道的void指针，或者NULL表示调用出错 */
void *POLE_Init_DO(int iModAddr, u_int uiCh)
{
    POLE_DO_HND  *pHdl;

    assert(iModAddr<MAX_MOD_NUM);
    assert(uiCh<MAX_DO_PER_MOD);

    if(PoleBoxIOInfo_g.aPoleIOModInfo[iModAddr].type==DI_MODULE)
    {
        assert(FALSE);
    }

    if(PoleBoxIOInfo_g.aPoleIOModInfo[iModAddr].type==IDLE_MODULE)
    {
        PoleBoxIOInfo_g.aPoleIOModInfo[iModAddr].type=DO_MODULE;
    }
    PoleBoxIOInfo_g.aPoleIOModInfo[iModAddr].unDoChNum++;

    pHdl=&(PoleBoxIOInfo_g.ahPoleDoHandle[iModAddr*MAX_DO_PER_MOD+uiCh]);
    pHdl->ucMod=iModAddr;
    pHdl->ucHdCh=uiCh;
    pHdl->ulPassWd=POLE_DO_PASSWORD;
    pHdl->bCurVal=FALSE;
    pHdl->bLstPubVal=FALSE;/*   2007-6-25日 张云  */


    PoleBoxIOInfo_g.aphPoleDoIdx[PoleBoxIOInfo_g.iPoleDoNum]=pHdl;

    PoleBoxIOInfo_g.iPoleDoNum++;
    iPoleDoNum_g++;

    return  (void  *)pHdl;
}
