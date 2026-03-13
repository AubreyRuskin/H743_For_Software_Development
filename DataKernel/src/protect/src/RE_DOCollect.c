/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_DOCollect.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的DO集中功能的代码实现                       */
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
#include   "realdata.h"
#include  "RE_DOCollect.h"
#include   "string_compat.h"
#include   "RE_OuterOutputTuyuan.h"
#include   "RE_DOSetTuyuan.h"



/*扫描任务的DO图元集中信息数组  */
DOCollect_Scan_Type  aTaskDOCollectArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/*功能：DO集中功能扫描
　参数：iTaskNo，所属任务
　返回：无  */
void RE_DOCollectScan(int  iTaskNo)
{
    DOCollect_Scan_Type  *pScanInf;
    int   i;
    int  **ppInVal;
    int  * pOutVal;
    int  **ppSetVal;
    int  **ppActVal;
    uint8_t *pInAttrib;
    int  iDOCnt;


    pScanInf=aTaskDOCollectArr+iTaskNo;
    iDOCnt=pScanInf->iDOCnt;
    if(iDOCnt==0)
    {
        return;
    }
    ppInVal=pScanInf->apbInValArr;
    pOutVal=pScanInf->abOutValArr;
    ppSetVal=pScanInf->apbSetOutValArr;
    ppActVal=pScanInf->apbActOutValArr;
    pInAttrib=pScanInf->aucInAttributeArr;
    for(i=0; i<iDOCnt; i++)
    {
        if((*(*ppInVal))==(*pOutVal)
                &&(*(*ppSetVal))==(*(*ppActVal)))
        {
            /* 常见的输出未变位且设置状态和实际状态一致的情况 */
            ppInVal++;
            pOutVal++;
            ppSetVal++;
            ppActVal++;
            pInAttrib++;
        }
        else
        {
            RD_LGC_DO_CH *plgcdo;
            BOOL bIsAdvance;
            BOOL  bIsFall;

            plgcdo=*(pScanInf->apvDestHandleArr+i);

            if((*pInAttrib==SHORT_INT_SIGNAL)||(*pInAttrib==LONG_INT_SIGNAL))
            {
                taskLock();
                *(*ppSetVal)=(*(*ppInVal));
                RD_Set_DO(plgcdo,(*(*ppInVal)));
                taskUnlock();
            }
            else
            {
                if((*(*ppInVal))&0x80)
                {
                    taskLock();
                    *(*ppSetVal)=(*(*ppInVal))&0x7F;
                    RD_Set_DO(plgcdo,(*(*ppInVal))&0x7F);
                    taskUnlock();
                }
                else if(((*(*ppInVal))==DP_TRUE)||((*(*ppInVal))==DP_FALSE)
                        ||((*(*ppInVal))==DP_INVALID_11)||((*(*ppInVal))==DP_INVALID_00))
                {
                    taskLock();
                    *(*ppSetVal)=(*(*ppInVal));
                    RD_Set_DO(plgcdo,(*(*ppInVal)));
                    taskUnlock();
                }
                else
                {
                    bIsAdvance = (!(*pOutVal)) && (*(*ppInVal));
                    bIsFall = (*pOutVal) && (!(*(*ppInVal)));

                    taskLock();
                    if (bIsAdvance)
                    {
                        /* 上升沿 */
                        (plgcdo->iTripDOCnt)++;
                    }
                    else if (bIsFall)
                    {
                        /* 下降沿 */
                        --(plgcdo->iTripDOCnt);
                    }
                    if (plgcdo->iTripDOCnt>0)
                    {
                        *(*ppSetVal)=TRUE;
                        RD_Set_DO(plgcdo, TRUE);
                    }
                    else
                    {
                        *(*ppSetVal)=FALSE;
                        RD_Set_DO(plgcdo, FALSE);

                    }
                    taskUnlock();
                }
            }

            *pOutVal=*(*ppInVal);

            ppInVal++;
            pOutVal++;
            ppSetVal++;
            ppActVal++;
            pInAttrib++;
        }
    }/*for结束 */
}


/*功能：DO集中功能初始化
　参数：iTaskNo ,所属任务
　返回：无  */
void RE_DOCollectInit(int  iTaskNo)
{
    DOCollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskDOCollectArr+iTaskNo;
    pScanInf->iDOCnt=0;
}

/*功能：DO集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_DOCollectClearSeqNo(int  iTaskNo)
{
    DOCollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskDOCollectArr+iTaskNo;
    pScanInf->iCurDOSeqNo=0;
}

/*　功能：对任务中的DO汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
          iAddCnt,添加的个数
  　返回：无  */
void  RE_DOCollectAddNew(void *PartGrpAttrib,int  iAddCnt)
{
    DOCollect_Scan_Type  *pScanInf;
    PARTGRP_ATTRIB_TYPE *  pPartGrpAttr;

    pPartGrpAttr=(PARTGRP_ATTRIB_TYPE *)PartGrpAttrib;
    if(!(pPartGrpAttr->bRunFlag))
    {
        return;
    }

    pScanInf=aTaskDOCollectArr+pPartGrpAttr->nScanTaskNo;
    pScanInf->iDOCnt=pScanInf->iDOCnt+iAddCnt;
}


/*功能：DO集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_DOCollectMemInit(int  iTaskNo)
{
    DOCollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskDOCollectArr+iTaskNo;
    if(!(pScanInf->iDOCnt))
    {
        return  ;
    }
    pScanInf->apbInValArr=calloc(pScanInf->iDOCnt
                                 ,sizeof(*(pScanInf->apbInValArr)));
    pScanInf->abOutValArr=calloc(pScanInf->iDOCnt
                                 ,sizeof(*(pScanInf->abOutValArr)));
    pScanInf->apbSetOutValArr=calloc(pScanInf->iDOCnt
                                     ,sizeof(*(pScanInf->apbSetOutValArr)));
    pScanInf->apbActOutValArr=calloc(pScanInf->iDOCnt
                                     ,sizeof(*(pScanInf->apbActOutValArr)));
    pScanInf->aucInAttributeArr=calloc(pScanInf->iDOCnt
                                       ,sizeof(*(pScanInf->aucInAttributeArr)));
    pScanInf->apvDestHandleArr=calloc(pScanInf->iDOCnt
                                      ,sizeof(*(pScanInf->apvDestHandleArr)));
    pScanInf->iCurDOSeqNo=0;
}



/*　功能：将DO图元的信息添加到ＤＯ集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的ＤＯ图元
  　返回：无  */
void RE_DOCollectAddByDOTuyuan(void *PartGrpAttrib,NODE *pCurScanNode)
{
    DOCollect_Scan_Type  *pScanInf;
    DOOuterOutput_Scan_Node_Type  *pDONode;
    RD_LGC_DO_CH *plgcdo;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskDOCollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pDONode=(DOOuterOutput_Scan_Node_Type  *)pCurScanNode->pTuyuan;
    plgcdo=(RD_LGC_DO_CH *)(pDONode->pvDestHandle);

    assert(pScanInf->iCurDOSeqNo<pScanInf->iDOCnt);
    *(pScanInf->apbInValArr+pScanInf->iCurDOSeqNo)=&(pDONode->pInArr0->now.bVal);
    *(pScanInf->abOutValArr+pScanInf->iCurDOSeqNo)=FALSE;
    *(pScanInf->apbSetOutValArr+pScanInf->iCurDOSeqNo)=&(plgcdo->iSetVal);
    *(pScanInf->apbActOutValArr+pScanInf->iCurDOSeqNo)=&(plgcdo->iVal);
    *(pScanInf->aucInAttributeArr+pScanInf->iCurDOSeqNo)=pDONode->pInArr0->ucAttrib;
    *(pScanInf->apvDestHandleArr+pScanInf->iCurDOSeqNo)=plgcdo;

    pScanInf->iCurDOSeqNo++;

}


/*　功能：将DOSet图元的信息添加到ＤＯ集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的DOSet图元
  　返回：无  */
void RE_DOCollectAddByDOSetTuyuan(void *PartGrpAttrib,NODE *pCurScanNode)
{

    DOCollect_Scan_Type  *pScanInf;
    DOSet_Scan_Node_Type  *pDOSetNode;
    RD_LGC_DO_CH *plgcdo;
    int  i;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskDOCollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pDOSetNode=(DOSet_Scan_Node_Type  *)pCurScanNode->pTuyuan;

    for(i=0; i<pDOSetNode->iDOCnt; i++)
    {
        assert(pScanInf->iCurDOSeqNo<pScanInf->iDOCnt);
        plgcdo=(RD_LGC_DO_CH *)(pDOSetNode->pvDestHandleArr[i]);

        *(pScanInf->apbInValArr+pScanInf->iCurDOSeqNo)=&(pDOSetNode->pInArr0->now.bVal);
        *(pScanInf->abOutValArr+pScanInf->iCurDOSeqNo)=FALSE;
        *(pScanInf->apbSetOutValArr+pScanInf->iCurDOSeqNo)=&(plgcdo->iSetVal);
        *(pScanInf->apbActOutValArr+pScanInf->iCurDOSeqNo)=&(plgcdo->iVal);
        *(pScanInf->aucInAttributeArr+pScanInf->iCurDOSeqNo)=pDOSetNode->pInArr0->ucAttrib;
        *(pScanInf->apvDestHandleArr+pScanInf->iCurDOSeqNo)=plgcdo;

        pScanInf->iCurDOSeqNo++;
    }
}