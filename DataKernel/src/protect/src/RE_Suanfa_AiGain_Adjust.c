#include "hwcfg.h"
#include "logic.h"
#include "errtest.h"
#include "realdata.h"
#include "AppInterface.h"
#include "edpbase.h"


#define MAX_AI_GAIN_CHECK_TIMES 120

typedef struct tagAiGainStruct
{
    BOOL bLastScanTriggerVal;
    BOOL bAiGainAdjStarted;
    float CoffSum;
    int CurrentCheckTime;
} AI_GAIN_STRUCT;


static void EDP_AiGain_Adj_Scan(struct tag_EP_ELEMENT *pelm);

/* 校验算法初始化.
 * Para:
 *     pelm, 图元.
 * Return:
 *     NONE.
 */
EP_STATUS EDP_AiGain_Adj(EP_ELEMENT *pelm)
{
    assert (pelm && (pelm->ucType == 0));

    if (pelm->unInNum !=3)
    {
        return EP_BAD_DATA;
    }

    assert(pelm->ppioIn[2]->pvCh!=NULL);

    pelm->apvUser[0]=(void *)malloc(sizeof(AI_GAIN_STRUCT));

    assert(pelm->apvUser[0]!=NULL);

    ((AI_GAIN_STRUCT *)pelm->apvUser[0])->bLastScanTriggerVal=FALSE;
    ((AI_GAIN_STRUCT *)pelm->apvUser[0])->bAiGainAdjStarted=FALSE;
    ((AI_GAIN_STRUCT *)pelm->apvUser[0])->CoffSum=0.0;
    ((AI_GAIN_STRUCT *)pelm->apvUser[0])->CurrentCheckTime=0;

    pelm->aioOut[0].now.bVal=FALSE;

    pelm->Scan_Func=EDP_AiGain_Adj_Scan;
    return EP_SUCCESS;
}

/* 校验算法.
 * Para:
 *     pelm, 图元.
 * Return:
 *     NONE.
 */
static void EDP_AiGain_Adj_Scan(EP_ELEMENT *pelm)
{
    AI_GAIN_STRUCT *pAiGain=NULL;
    pAiGain=(AI_GAIN_STRUCT *)pelm->apvUser[0];

    if((pelm->ppioIn[0]->now.bVal==TRUE)&&(pAiGain->bLastScanTriggerVal==FALSE))
    {
        pAiGain->bAiGainAdjStarted=TRUE;
        pAiGain->CurrentCheckTime=0;
        pAiGain->CoffSum=0.0;
    }

    pAiGain->bLastScanTriggerVal=pelm->ppioIn[0]->now.bVal;

    if(pAiGain->bAiGainAdjStarted)
    {
        if(pAiGain->CurrentCheckTime<MAX_AI_GAIN_CHECK_TIMES)
        {
            pAiGain->CoffSum+=pelm->ppioIn[1]->now.fVal/(REAL(pelm->ppioIn[2]->now.xVal)/GetAiScaleCoeLgc(pelm->ppioIn[2]->pvCh));
            pAiGain->CurrentCheckTime++;
            pelm->aioOut[0].now.bVal=FALSE;
        }
        else
        {
            ModifyAiScaleCoeLgc(pelm->ppioIn[2]->pvCh,pAiGain->CoffSum/MAX_AI_GAIN_CHECK_TIMES);
            pAiGain->CurrentCheckTime=0;
            pAiGain->bAiGainAdjStarted=FALSE;
            pAiGain->CoffSum=0.0;
            pelm->aioOut[0].now.bVal=TRUE;
        }
    }
    else
    {
        pelm->aioOut[0].now.bVal=FALSE;
    }
}