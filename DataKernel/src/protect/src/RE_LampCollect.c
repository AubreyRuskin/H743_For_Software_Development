/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_LampCollect.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的指示灯集中功能的代码实现                   */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                    */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                */
/*                                                                              */
/*         作者           日期                    说明                          */
/*                                                                              */
/*         张云       2011.7.27              创建文件1.0版本                    */

/*                                                                              */
/********************************************************************************/

#include <vxWorks.h>
#include   "RE_PublicDataDef.h"
#include    "RE_AllTuyuanDataDef.h"
#include    "stdio_compat.h"
#include    "taskLib.h"
#include    "realdata.h"
#include    "RE_LampCollect.h"
#include    "string_compat.h"
#include    "RE_OuterOutputTuyuan.h"


/*扫描任务的指示灯图元集中信息数组  */
LampCollect_Scan_Type  aTaskLampCollectArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/*功能：指示灯集中功能扫描
　参数：iTaskNo  所属保护任务号
　返回：无  */
void RE_LampCollectScan(int  iTaskNo)
{
    LampCollect_Scan_Type  *pScanInf;
    int   i;
    BOOL  **ppInVal;
    BOOL  * pOutVal;
    BOOL  **ppSetVal;
    BOOL  **ppActVal;
    int  iLampCnt;

    pScanInf=aTaskLampCollectArr+iTaskNo;
    iLampCnt=pScanInf->iLampCnt;
    if(iLampCnt==0)
    {
        return;
    }
    ppInVal=pScanInf->apbInValArr;
    pOutVal=pScanInf->abOutValArr;
    ppSetVal=pScanInf->apbSetOutValArr;
    ppActVal=pScanInf->apbActOutValArr;

    for(i=0; i<iLampCnt; i++)
    {
        if((*(*ppInVal))==(*pOutVal)
                &&(*(*ppSetVal))==(*(*ppActVal)))
        {
            /* 常见的输出未变位且设置状态和实际状态一致的情况 */
            ppInVal++;
            pOutVal++;
            ppSetVal++;
            ppActVal++;
        }
        else
        {
            RD_LGC_LED_CH *plgcLed;
            BOOL bIsAdvance;
            BOOL  bIsFall;

            plgcLed=*(pScanInf->apvDestHandleArr+i);

            bIsAdvance = (!(*pOutVal)) && (*(*ppInVal));
            bIsFall = (*pOutVal) && (!(*(*ppInVal)));

            taskLock();
            if (bIsAdvance)
            {
                /* 上升沿 */
                (plgcLed->iTripLedCnt)++;
            }
            else if (bIsFall)
            {
                /* 下降沿 */
                --(plgcLed->iTripLedCnt);
            }
            if (plgcLed->iTripLedCnt>0)
            {
                *(*ppSetVal)=TRUE;
                RD_Set_LED(plgcLed,TRUE);
            }
            else
            {
                *(*ppSetVal)=FALSE;
                RD_Set_LED(plgcLed,FALSE);
            }
            taskUnlock();

            *pOutVal=*(*ppInVal);

            ppInVal++;
            pOutVal++;
            ppSetVal++;
            ppActVal++;
        }
    }/*for结束 */
}


/*功能：指示灯集中功能初始化
　参数：iTaskNo，所属任务
　返回：无  */
void RE_LampCollectInit(int  iTaskNo)
{
    LampCollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskLampCollectArr+iTaskNo;
    pScanInf->iLampCnt=0;
}


/*功能：指示灯集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_LampCollectClearSeqNo(int  iTaskNo)
{
    LampCollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskLampCollectArr+iTaskNo;
    pScanInf->iCurLampSeqNo=0;
}


/*　功能：对任务中的指示灯图元汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
  　返回：无  */
void  RE_LampCollectAddNew(void *PartGrpAttrib)
{
    LampCollect_Scan_Type *pScanInf;
    PARTGRP_ATTRIB_TYPE *  pPartGrpAttr;

    pPartGrpAttr=(PARTGRP_ATTRIB_TYPE *)PartGrpAttrib;

    if(!(pPartGrpAttr->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskLampCollectArr+pPartGrpAttr->nScanTaskNo;
    pScanInf->iLampCnt++;
}


/*功能：指示灯集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_LampCollectMemInit(int  iTaskNo)
{
    LampCollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskLampCollectArr+iTaskNo;
    if(!(pScanInf->iLampCnt))
    {
        return  ;
    }
    pScanInf->apbInValArr=calloc(pScanInf->iLampCnt
                                 ,sizeof(*(pScanInf->apbInValArr)));
    pScanInf->abOutValArr=calloc(pScanInf->iLampCnt
                                 ,sizeof(*(pScanInf->abOutValArr)));
    pScanInf->apbSetOutValArr=calloc(pScanInf->iLampCnt
                                     ,sizeof(*(pScanInf->apbSetOutValArr)));
    pScanInf->apbActOutValArr=calloc(pScanInf->iLampCnt
                                     ,sizeof(*(pScanInf->apbActOutValArr)));
    pScanInf->apvDestHandleArr=calloc(pScanInf->iLampCnt
                                      ,sizeof(*(pScanInf->apvDestHandleArr)));
}



/*　功能：将指示灯图元的信息添加到指示灯集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的指示灯图元
  　返回：无  */
void RE_LampCollectAddByLampTuyuan(void *PartGrpAttrib,NODE *pCurScanNode)
{
    LampCollect_Scan_Type  *pScanInf;
    LampOuterOutput_Scan_Node_Type  *pLampNode;
    RD_LGC_LED_CH *plgcLed;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskLampCollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pLampNode=(LampOuterOutput_Scan_Node_Type  *)pCurScanNode->pTuyuan;
    plgcLed=(RD_LGC_LED_CH *)(pLampNode->pvDestHandle);
    assert(pScanInf->iCurLampSeqNo<pScanInf->iLampCnt);
    *(pScanInf->apbInValArr+pScanInf->iCurLampSeqNo)=&(pLampNode->pInArr0->now.bVal);
    *(pScanInf->abOutValArr+pScanInf->iCurLampSeqNo)=FALSE;
    *(pScanInf->apbSetOutValArr+pScanInf->iCurLampSeqNo)=&(plgcLed->bSetVal);
    *(pScanInf->apbActOutValArr+pScanInf->iCurLampSeqNo)=&(plgcLed->bSts);
    *(pScanInf->apvDestHandleArr+pScanInf->iCurLampSeqNo)=plgcLed;

    pScanInf->iCurLampSeqNo++;

}

