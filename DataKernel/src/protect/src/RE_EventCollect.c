/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_EventCollect.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的事件集中功能的代码实现                   */
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

#include    <vxWorks.h>
#include   "RE_PublicDataDef.h"
#include    "RE_AllTuyuanDataDef.h"
#include    "stdio_compat.h"
#include    "taskLib.h"
#include    "realdata.h"
#include    "RE_EventCollect.h"
#include    "string_compat.h"
#include    "RE_EventTuyuan.h"
#include    "view.h"

extern   BOOL  bAlertIsSet_g;/*呼唤标志设置变量   */

/*扫描任务的事件图元集中信息数组  */
EventCollect_Scan_Type  aTaskEventCollectArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/*功能：事件集中功能扫描
　参数：iTaskNo  任务号
　返回：无  */
void RE_EventCollectScan(int  iTaskNo)
{
    EventCollect_Scan_Type  *pScanInf;
    int   i;
    BOOL  **ppInVal;
    BOOL  *pOutVal;
    int  iEventCnt;
    BOOL  bInVal;
    BOOL bIsAdvance;
    BOOL  bIsFall;
    Event_Scan_Node_Type *pScanNode;

    pScanInf=aTaskEventCollectArr+iTaskNo;
    iEventCnt=pScanInf->iEventCnt;
    if(iEventCnt==0)
    {
        return;
    }
    ppInVal=pScanInf->apbInValArr;
    pOutVal=pScanInf->abOutValArr;

    for(i=0; i<iEventCnt; i++)
    {
        bInVal=*(*ppInVal);
        if(!bInVal||bAlertIsSet_g)
        {
            if(bInVal==(*pOutVal))
            {
                /*这是经常情形  */
                ppInVal++;
                pOutVal++;
                continue;
            }
        }
        else
        {
            SCI_Deal_Event_Alert(*(pScanInf->aiEventNumArr+i),bInVal);
            if(bInVal==(*pOutVal))
            {
                ppInVal++;
                pOutVal++;
                continue;
            }
        }
        /*若有变位  */
        bIsAdvance = (!(*pOutVal)) &&bInVal;
        bIsFall =(*pOutVal)&&(!bInVal);
        pScanNode=(Event_Scan_Node_Type *)((pScanInf->apEventNodeArr[i])->pTuyuan);

        if (bIsAdvance)
        {
            /* 若当前处于上升沿,则触发事件 */
            SCI_Trigger_Event(*(pScanInf->aiEventNumArr+i),
                              pScanNode->pChartMsg->ulScnTime, TRUE);
            /* 保存此次的输入,供下次触发使用 */
        }
        else if (bIsFall)
        {
            /* 若当前处于下降沿,则触发事件 */
            SCI_Trigger_Event(*(pScanInf->aiEventNumArr+i),
                              pScanNode->pChartMsg->ulScnTime, FALSE);
            /* 保存此次的输入,供下次触发使用 */
        }
        *pOutVal=bInVal;
        ppInVal++;
        pOutVal++;
    }/*for结束 */
}


/*功能：事件集中功能初始化
　参数：iTaskNo，所属任务号
　返回：无  */
void RE_EventCollectInit(int  iTaskNo)
{
    EventCollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskEventCollectArr+iTaskNo;
    pScanInf->iEventCnt=0;
}

/*功能：Event集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_EventCollectClearSeqNo(int  iTaskNo)
{
    EventCollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskEventCollectArr+iTaskNo;
    pScanInf->iCurEventSeqNo=0;
}

/*　功能：对任务中的事件图元汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
  　返回：无  */
void  RE_EventCollectAddNew(void *PartGrpAttrib)
{
    EventCollect_Scan_Type *pScanInf;
    PARTGRP_ATTRIB_TYPE *  pPartGrpAttr;

    pPartGrpAttr=(PARTGRP_ATTRIB_TYPE *)PartGrpAttrib;

    if(!(pPartGrpAttr->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskEventCollectArr+pPartGrpAttr->nScanTaskNo;
    pScanInf->iEventCnt++;
}


/*功能：事件集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_EventCollectMemInit(int  iTaskNo)
{
    EventCollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskEventCollectArr+iTaskNo;
    if(!(pScanInf->iEventCnt))
    {
        return  ;
    }
    pScanInf->apbInValArr=calloc(pScanInf->iEventCnt
                                 ,sizeof(*(pScanInf->apbInValArr)));
    pScanInf->abOutValArr=calloc(pScanInf->iEventCnt
                                 ,sizeof(*(pScanInf->abOutValArr)));
    pScanInf->aiEventNumArr=calloc(pScanInf->iEventCnt
                                   ,sizeof(*(pScanInf->aiEventNumArr)));
    pScanInf->apEventNodeArr=calloc(pScanInf->iEventCnt
                                    ,sizeof(*(pScanInf->apEventNodeArr)));
}



/*　功能：将事件图元的信息添加到事件集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的指示灯图元
  　返回：无  */
void RE_EventCollectAddByEventTuyuan(void *PartGrpAttrib,NODE *pCurScanNode)
{
    EventCollect_Scan_Type  *pScanInf;
    Event_Scan_Node_Type  *pEventNode;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskEventCollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pEventNode=(Event_Scan_Node_Type  *)pCurScanNode->pTuyuan;
    assert(pScanInf->iCurEventSeqNo<pScanInf->iEventCnt);
    *(pScanInf->apbInValArr+pScanInf->iCurEventSeqNo)=&(pEventNode->pInArr0->now.bVal);
    *(pScanInf->abOutValArr+pScanInf->iCurEventSeqNo)=FALSE;
    *(pScanInf->aiEventNumArr+pScanInf->iCurEventSeqNo)=pEventNode->nEventNum;
    *(pScanInf->apEventNodeArr+pScanInf->iCurEventSeqNo)=pCurScanNode;

    pScanInf->iCurEventSeqNo++;

}

