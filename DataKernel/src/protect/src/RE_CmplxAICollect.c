/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_CmplxAICollect.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的ComplexAI集中功能的代码实现                       */
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
#include  "RE_CmplxAICollect.h"
#include   "string_compat.h"
#include   "RE_OuterInputTuyuan.h"
#include   "RE_AISetTuyuan.h"



/*扫描任务的Cmplx图元集中信息数组  */
CmplxAICollect_Scan_Type  aTaskCmplxAICollectArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/*功能：CmplxAI集中功能扫描
　参数：iTaskNo，所属任务
　返回：无  */
void RE_CmplxAICollectScan(int  iTaskNo)
{
    CmplxAICollect_Scan_Type  *pScanInf;
    EP_ELEM_IO  * pCurOut;
    int *piChOffset;
    int i;
    COMPLEX   *pxBase;
    int  iOutputCnt_4;
    int  iOutputCnt_m;

    pScanInf=aTaskCmplxAICollectArr+iTaskNo;
    iOutputCnt_4=pScanInf->iOutputCnt_4;
    iOutputCnt_m=pScanInf->iOutputCnt_m;
    if(iOutputCnt_4==0&&iOutputCnt_m==0)
    {
        return;
    }
    pxBase=*(pScanInf->ppxBase);
    pCurOut=pScanInf->aioOutArr;
    piChOffset=pScanInf->aAISourceChOffsetArr;
    pCurOut--;
    piChOffset--;
    for(i=0; i<iOutputCnt_4; i++)
    {
        (++pCurOut)->now.xVal=*(pxBase+*++piChOffset);
        (++pCurOut)->now.xVal=*(pxBase+*++piChOffset);
        (++pCurOut)->now.xVal=*(pxBase+*++piChOffset);
        (++pCurOut)->now.xVal=*(pxBase+*++piChOffset);
    }
    for(i=0; i<iOutputCnt_m; i++)
    {
        (++pCurOut)->now.xVal=*(pxBase+*++piChOffset);


    }
}


/*功能：CmplxAI集中功能初始化
　参数：iTaskNo ,所属任务
　返回：无  */
void RE_CmplxAICollectInit(int  iTaskNo)
{
    CmplxAICollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskCmplxAICollectArr+iTaskNo;
    pScanInf->iCmplxAICnt=0;
}

/*功能：CmplxAI集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_CmplxAICollectClearSeqNo(int  iTaskNo)
{
    CmplxAICollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskCmplxAICollectArr+iTaskNo;
    pScanInf->iCurCmplxAISeqNo=0;
}


/*　功能：对任务中的CmplxAI汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
          iAddCnt,添加的个数
  　返回：无  */
void  RE_CmplxAICollectAddNew(void *PartGrpAttrib,int  iAddCnt)
{
    CmplxAICollect_Scan_Type  *pScanInf;
    PARTGRP_ATTRIB_TYPE *  pPartGrpAttr;

    pPartGrpAttr=(PARTGRP_ATTRIB_TYPE *)PartGrpAttrib;
    if(!(pPartGrpAttr->bRunFlag))
    {
        return;
    }

    pScanInf=aTaskCmplxAICollectArr+pPartGrpAttr->nScanTaskNo;
    pScanInf->iCmplxAICnt=pScanInf->iCmplxAICnt+iAddCnt;
}


/*功能：CmplxAI集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_CmplxAICollectMemInit(int  iTaskNo)
{
    CmplxAICollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskCmplxAICollectArr+iTaskNo;
    if(!(pScanInf->iCmplxAICnt))
    {
        return  ;
    }
    pScanInf->aioOutArr=calloc(pScanInf->iCmplxAICnt
                               ,sizeof(*(pScanInf->aioOutArr)));
    pScanInf->aAISourceChOffsetArr=calloc(pScanInf->iCmplxAICnt
                                          ,sizeof(*(pScanInf->aAISourceChOffsetArr)));
    pScanInf->iOutputCnt_4=pScanInf->iCmplxAICnt/4;
    pScanInf->iOutputCnt_m=pScanInf->iCmplxAICnt%4;
}



/*　功能：将CmplxAI图元的信息添加到CmplxAI集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的FloatAI图元
  　返回：无  */
void RE_CmplxAICollectAddByAITuyuan(void *PartGrpAttrib,NODE *pCurScanNode)
{
    CmplxAICollect_Scan_Type  *pScanInf;
    ComplexAIOuterInput_Scan_Node_Type  *pCmplxAINode;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskCmplxAICollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pCmplxAINode=(ComplexAIOuterInput_Scan_Node_Type  *)pCurScanNode->pTuyuan;

    assert(pScanInf->iCurCmplxAISeqNo<pScanInf->iCmplxAICnt);

    pScanInf->ppxBase=&(pCmplxAINode->pchart->pxBase);
    *(pScanInf->aioOutArr+pScanInf->iCurCmplxAISeqNo)=pCmplxAINode->ioOut;
    *(pScanInf->aAISourceChOffsetArr+pScanInf->iCurCmplxAISeqNo)=pCmplxAINode->AISourceChOffset;

    pScanInf->iCurCmplxAISeqNo++;

}


/*　功能：将CmplxAISet图元的信息添加到CmplxAI集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的FloatAISet图元
  　返回：无  */
void RE_CmplxAICollectAddByAISetTuyuan(void *PartGrpAttrib,NODE *pCurScanNode)
{

    CmplxAICollect_Scan_Type  *pScanInf;
    AISet_Scan_Node_Type  *pCmplxAISetNode;
    int  i;
    int  iNodeAICnt;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskCmplxAICollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pCmplxAISetNode=(AISet_Scan_Node_Type  *)pCurScanNode->pTuyuan;
    iNodeAICnt=(pCmplxAISetNode->ucOutputCnt_8)*8
               +pCmplxAISetNode->ucOutputCnt_m;
    for(i=0; i<iNodeAICnt; i++)
    {
        assert(pScanInf->iCurCmplxAISeqNo<pScanInf->iCmplxAICnt);

        pScanInf->ppxBase=&(pCmplxAISetNode->pchart->pxBase);
        *(pScanInf->aioOutArr+pScanInf->iCurCmplxAISeqNo)=pCmplxAISetNode->ioOutArr[i];
        *(pScanInf->aAISourceChOffsetArr+pScanInf->iCurCmplxAISeqNo)=pCmplxAISetNode->AISourceChOffsetArr[i];

        pScanInf->iCurCmplxAISeqNo++;
    }
}


/*　功能：将CmplxAI的输出IO地址赋值给扫描节点中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的FloatAI图元
  　返回：无  */
void RE_CmplxAICollectInitAITuyuanOut(void *PartGrpAttrib,NODE *pCurScanNode)
{

    CmplxAICollect_Scan_Type  *pScanInf;
    ComplexAIOuterInput_Scan_Node_Type  *pCmplxAINode;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskCmplxAICollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pCmplxAINode=(ComplexAIOuterInput_Scan_Node_Type  *)pCurScanNode->pTuyuan;

    assert(pScanInf->iCurCmplxAISeqNo<pScanInf->iCmplxAICnt);

    /*将AI集中后的OUT对应指针赋值给节点中的成员变量  */
    pCmplxAINode->pCollectOut=pScanInf->aioOutArr+pScanInf->iCurCmplxAISeqNo;
    *(pScanInf->aioOutArr+pScanInf->iCurCmplxAISeqNo)=pCmplxAINode->ioOut;

    pScanInf->iCurCmplxAISeqNo++;
}

/*　功能：将CmplxAISet的输出IO地址赋值给扫描节点中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的FloatAISet图元
  　返回：无  */
void RE_CmplxAICollectInitAISetTuyuanOut(void *PartGrpAttrib,NODE *pCurScanNode)
{

    CmplxAICollect_Scan_Type  *pScanInf;
    AISet_Scan_Node_Type  *pCmplxAISetNode;
    int  i;
    int  iNodeAICnt;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskCmplxAICollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pCmplxAISetNode=(AISet_Scan_Node_Type  *)pCurScanNode->pTuyuan;
    iNodeAICnt=(pCmplxAISetNode->ucOutputCnt_8)*8
               +pCmplxAISetNode->ucOutputCnt_m;
    for(i=0; i<iNodeAICnt; i++)
    {
        assert(pScanInf->iCurCmplxAISeqNo<pScanInf->iCmplxAICnt);

        pCmplxAISetNode->apCollectOutArr[i]=pScanInf->aioOutArr+pScanInf->iCurCmplxAISeqNo;
        *(pScanInf->aioOutArr+pScanInf->iCurCmplxAISeqNo)=pCmplxAISetNode->ioOutArr[i];

        pScanInf->iCurCmplxAISeqNo++;
    }
}
