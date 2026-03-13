/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_DICollect.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的DI集中功能的代码实现                       */
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
#include  "RE_DICollect.h"
#include   "string_compat.h"
#include   "RE_OuterInputTuyuan.h"
#include   "RE_DISetTuyuan.h"



/*扫描任务的DI图元集中信息数组  */
DICollect_Scan_Type  aTaskDICollectArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/*功能：DI集中功能扫描
　参数：iTaskNo，所属任务
　返回：无  */
void RE_DICollectScan(int  iTaskNo)
{
    DICollect_Scan_Type  *pScanInf;
    EP_ELEM_IO  * pCurOut;
    int *piChOffset;
    int  i;
    int  iOutputCnt_4;
    int  iOutputCnt_m;
    BOOL *pDIBase;

    pScanInf=aTaskDICollectArr+iTaskNo;
    iOutputCnt_4=pScanInf->iOutputCnt_4;
    iOutputCnt_m=pScanInf->iOutputCnt_m;
    if(iOutputCnt_4==0&&iOutputCnt_m==0)
    {
        return;
    }
    pDIBase = RD_Base_His_DI_P(*(pScanInf->pulScnAiCnt));
    pCurOut=pScanInf->aioOutArr;
    piChOffset=pScanInf->aDISourceChOffsetArr;
    pCurOut--;
    piChOffset--;
    for(i=0; i<iOutputCnt_4; i++)
    {
        (++pCurOut)->now.bVal = *(pDIBase+*(++piChOffset));
        (++pCurOut)->now.bVal = *(pDIBase+*(++piChOffset));
        (++pCurOut)->now.bVal = *(pDIBase+*(++piChOffset));
        (++pCurOut)->now.bVal = *(pDIBase+*(++piChOffset));

    }
    for(i=0; i<iOutputCnt_m; i++)
    {
        (++pCurOut)->now.bVal = *(pDIBase+*(++piChOffset));
    }
}


/*功能：DI集中功能初始化
　参数：iTaskNo ,所属任务
　返回：无  */
void RE_DICollectInit(int  iTaskNo)
{
    DICollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskDICollectArr+iTaskNo;
    pScanInf->iDICnt=0;
}

/*功能：DI集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_DICollectClearSeqNo(int  iTaskNo)
{
    DICollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskDICollectArr+iTaskNo;
    pScanInf->iCurDISeqNo=0;
}


/*　功能：对任务中的DI汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
          iAddCnt,添加的个数
  　返回：无  */
void  RE_DICollectAddNew(void *PartGrpAttrib,int  iAddCnt)
{
    DICollect_Scan_Type  *pScanInf;
    PARTGRP_ATTRIB_TYPE *  pPartGrpAttr;

    pPartGrpAttr=(PARTGRP_ATTRIB_TYPE *)PartGrpAttrib;
    if(!(pPartGrpAttr->bRunFlag))
    {
        return;
    }

    pScanInf=aTaskDICollectArr+pPartGrpAttr->nScanTaskNo;
    pScanInf->iDICnt=pScanInf->iDICnt+iAddCnt;
}


/*功能：DI集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_DICollectMemInit(int  iTaskNo)
{
    DICollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskDICollectArr+iTaskNo;
    if(!(pScanInf->iDICnt))
    {
        return  ;
    }
    pScanInf->aioOutArr=calloc(pScanInf->iDICnt
                               ,sizeof(*(pScanInf->aioOutArr)));
    pScanInf->aDISourceChOffsetArr=calloc(pScanInf->iDICnt
                                          ,sizeof(*(pScanInf->aDISourceChOffsetArr)));
    pScanInf->iOutputCnt_4=pScanInf->iDICnt/4;
    pScanInf->iOutputCnt_m=pScanInf->iDICnt%4;
}



/*　功能：将DI图元的信息添加到DI集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的DI图元
  　返回：无  */
void RE_DICollectAddByDITuyuan(void *PartGrpAttrib,NODE *pCurScanNode)
{
    DICollect_Scan_Type  *pScanInf;
    DIOuterInput_Scan_Node_Type  *pDINode;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskDICollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pDINode=(DIOuterInput_Scan_Node_Type  *)pCurScanNode->pTuyuan;

    assert(pScanInf->iCurDISeqNo<pScanInf->iDICnt);

    pScanInf->pulScnAiCnt=&(pDINode->pchart->ulScnAiCnt);
    *(pScanInf->aioOutArr+pScanInf->iCurDISeqNo)=pDINode->ioOut;
    *(pScanInf->aDISourceChOffsetArr+pScanInf->iCurDISeqNo)=pDINode->DISourceChOffset;

    pScanInf->iCurDISeqNo++;

}


/*　功能：将DISet图元的信息添加到DI集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的DISet图元
  　返回：无  */
void RE_DICollectAddByDISetTuyuan(void *PartGrpAttrib,NODE *pCurScanNode)
{

    DICollect_Scan_Type  *pScanInf;
    DISet_Scan_Node_Type  *pDISetNode;
    int  i;
    int  iNodeDICnt;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskDICollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pDISetNode=(DISet_Scan_Node_Type  *)pCurScanNode->pTuyuan;
    iNodeDICnt=(pDISetNode->ucOutputCnt_8)*8
               +pDISetNode->ucOutputCnt_m;
    for(i=0; i<iNodeDICnt; i++)
    {
        assert(pScanInf->iCurDISeqNo<pScanInf->iDICnt);

        pScanInf->pulScnAiCnt=&(pDISetNode->pchart->ulScnAiCnt);
        *(pScanInf->aioOutArr+pScanInf->iCurDISeqNo)=pDISetNode->ioOutArr[i];
        *(pScanInf->aDISourceChOffsetArr+pScanInf->iCurDISeqNo)=pDISetNode->DISourceChOffsetArr[i];

        pScanInf->iCurDISeqNo++;
    }
}


/*　功能：将DI的输出IO地址赋值给扫描节点中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的DI图元
  　返回：无  */
void RE_DICollectInitDITuyuanOut(void *PartGrpAttrib,NODE *pCurScanNode)
{

    DICollect_Scan_Type  *pScanInf;
    DIOuterInput_Scan_Node_Type  *pDINode;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskDICollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pDINode=(DIOuterInput_Scan_Node_Type  *)pCurScanNode->pTuyuan;

    assert(pScanInf->iCurDISeqNo<pScanInf->iDICnt);

    /*将DI集中后的OUT对应指针赋值给节点中的成员变量  */
    pDINode->pCollectOut=pScanInf->aioOutArr+pScanInf->iCurDISeqNo;
    *(pScanInf->aioOutArr+pScanInf->iCurDISeqNo)=pDINode->ioOut;

    pScanInf->iCurDISeqNo++;
}


/*　功能：将DISet的输出IO地址赋值给扫描节点中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的DISet图元
  　返回：无  */
void RE_DICollectInitDISetTuyuanOut(void *PartGrpAttrib,NODE *pCurScanNode)
{

    DICollect_Scan_Type  *pScanInf;
    DISet_Scan_Node_Type  *pDISetNode;
    int  i;
    int  iNodeDICnt;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskDICollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pDISetNode=(DISet_Scan_Node_Type  *)pCurScanNode->pTuyuan;
    iNodeDICnt=(pDISetNode->ucOutputCnt_8)*8
               +pDISetNode->ucOutputCnt_m;
    for(i=0; i<iNodeDICnt; i++)
    {
        assert(pScanInf->iCurDISeqNo<pScanInf->iDICnt);

        pDISetNode->apCollectOutArr[i]=pScanInf->aioOutArr+pScanInf->iCurDISeqNo;
        *(pScanInf->aioOutArr+pScanInf->iCurDISeqNo)=pDISetNode->ioOutArr[i];

        pScanInf->iCurDISeqNo++;
    }
}
