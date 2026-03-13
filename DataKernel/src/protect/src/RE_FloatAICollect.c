/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_FloatAICollect.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的FloatAI集中功能的代码实现                       */
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
#include  "RE_FloatAICollect.h"
#include   "string_compat.h"
#include   "RE_OuterInputTuyuan.h"
#include   "RE_AISetTuyuan.h"



/*扫描任务的FloatAI图元集中信息数组  */
FloatAICollect_Scan_Type  aTaskFloatAICollectArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/*功能：FloatAI集中功能扫描
　参数：iTaskNo，所属任务
　返回：无  */
void RE_FloatAICollectScan(int  iTaskNo)
{
    FloatAICollect_Scan_Type  *pScanInf;
    EP_ELEM_IO  * pCurOut;
    int *piChOffset;
    int i;
    float   *pfBase;
    int  iOutputCnt_4;
    int  iOutputCnt_m;

    pScanInf=aTaskFloatAICollectArr+iTaskNo;
    iOutputCnt_4=pScanInf->iOutputCnt_4;
    iOutputCnt_m=pScanInf->iOutputCnt_m;
    if(iOutputCnt_4==0&&iOutputCnt_m==0)
    {
        return;
    }
    pfBase=*(pScanInf->ppfBase);
    pCurOut=pScanInf->aioOutArr;
    piChOffset=pScanInf->aAISourceChOffsetArr;
    pCurOut--;
    piChOffset--;
    for(i=0; i<iOutputCnt_4; i++)
    {
        (++pCurOut)->now.fVal=*(pfBase+*++piChOffset);
        (++pCurOut)->now.fVal=*(pfBase+*++piChOffset);
        (++pCurOut)->now.fVal=*(pfBase+*++piChOffset);
        (++pCurOut)->now.fVal=*(pfBase+*++piChOffset);
    }
    for(i=0; i<iOutputCnt_m; i++)
    {
        (++pCurOut)->now.fVal=*(pfBase+*++piChOffset);

    }
}


/*功能：FloatAI集中功能初始化
　参数：iTaskNo ,所属任务
　返回：无  */
void RE_FloatAICollectInit(int  iTaskNo)
{
    FloatAICollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskFloatAICollectArr+iTaskNo;
    pScanInf->iFloatAICnt=0;
}

/*功能：FloatAI集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_FloatAICollectClearSeqNo(int  iTaskNo)
{
    FloatAICollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskFloatAICollectArr+iTaskNo;
    pScanInf->iCurFloatAISeqNo=0;
}


/*　功能：对任务中的FloatAI汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
          iAddCnt,添加的个数
  　返回：无  */
void  RE_FloatAICollectAddNew(void *PartGrpAttrib,int  iAddCnt)
{
    FloatAICollect_Scan_Type  *pScanInf;
    PARTGRP_ATTRIB_TYPE *  pPartGrpAttr;

    pPartGrpAttr=(PARTGRP_ATTRIB_TYPE *)PartGrpAttrib;
    if(!(pPartGrpAttr->bRunFlag))
    {
        return;
    }

    pScanInf=aTaskFloatAICollectArr+pPartGrpAttr->nScanTaskNo;
    pScanInf->iFloatAICnt=pScanInf->iFloatAICnt+iAddCnt;
}


/*功能：FloatAI集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_FloatAICollectMemInit(int  iTaskNo)
{
    FloatAICollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskFloatAICollectArr+iTaskNo;
    if(!(pScanInf->iFloatAICnt))
    {
        return  ;
    }
    pScanInf->aioOutArr=calloc(pScanInf->iFloatAICnt
                               ,sizeof(*(pScanInf->aioOutArr)));
    pScanInf->aAISourceChOffsetArr=calloc(pScanInf->iFloatAICnt
                                          ,sizeof(*(pScanInf->aAISourceChOffsetArr)));
    pScanInf->iOutputCnt_4=pScanInf->iFloatAICnt/4;
    pScanInf->iOutputCnt_m=pScanInf->iFloatAICnt%4;
}



/*　功能：将FloatAI图元的信息添加到FloatAI集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的FloatAI图元
  　返回：无  */
void RE_FloatAICollectAddByAITuyuan(void *PartGrpAttrib,NODE *pCurScanNode)
{
    FloatAICollect_Scan_Type  *pScanInf;
    FloatAIOuterInput_Scan_Node_Type  *pFloatAINode;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskFloatAICollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pFloatAINode=(FloatAIOuterInput_Scan_Node_Type  *)pCurScanNode->pTuyuan;

    assert(pScanInf->iCurFloatAISeqNo<pScanInf->iFloatAICnt);

    pScanInf->ppfBase=&(pFloatAINode->pchart->pfBase);
    *(pScanInf->aioOutArr+pScanInf->iCurFloatAISeqNo)=pFloatAINode->ioOut;
    *(pScanInf->aAISourceChOffsetArr+pScanInf->iCurFloatAISeqNo)=pFloatAINode->AISourceChOffset;

    pScanInf->iCurFloatAISeqNo++;

}


/*　功能：将FloatAISet图元的信息添加到FloatAI集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的FloatAISet图元
  　返回：无  */
void RE_FloatAICollectAddByAISetTuyuan(void *PartGrpAttrib,NODE *pCurScanNode)
{

    FloatAICollect_Scan_Type  *pScanInf;
    AISet_Scan_Node_Type  *pFloatAISetNode;
    int  i;
    int  iNodeAICnt;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskFloatAICollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pFloatAISetNode=(AISet_Scan_Node_Type  *)pCurScanNode->pTuyuan;
    iNodeAICnt=(pFloatAISetNode->ucOutputCnt_8)*8
               +pFloatAISetNode->ucOutputCnt_m;
    for(i=0; i<iNodeAICnt; i++)
    {
        assert(pScanInf->iCurFloatAISeqNo<pScanInf->iFloatAICnt);

        pScanInf->ppfBase=&(pFloatAISetNode->pchart->pfBase);
        *(pScanInf->aioOutArr+pScanInf->iCurFloatAISeqNo)=pFloatAISetNode->ioOutArr[i];
        *(pScanInf->aAISourceChOffsetArr+pScanInf->iCurFloatAISeqNo)=pFloatAISetNode->AISourceChOffsetArr[i];

        pScanInf->iCurFloatAISeqNo++;
    }
}


/*　功能：将FloatAI的输出IO地址赋值给扫描节点中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的FloatAI图元
  　返回：无  */
void RE_FloatAICollectInitAITuyuanOut(void *PartGrpAttrib,NODE *pCurScanNode)
{

    FloatAICollect_Scan_Type  *pScanInf;
    FloatAIOuterInput_Scan_Node_Type  *pFloatAINode;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskFloatAICollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pFloatAINode=(FloatAIOuterInput_Scan_Node_Type  *)pCurScanNode->pTuyuan;

    assert(pScanInf->iCurFloatAISeqNo<pScanInf->iFloatAICnt);

    /*将AI集中后的OUT对应指针赋值给节点中的成员变量  */
    pFloatAINode->pCollectOut=pScanInf->aioOutArr+pScanInf->iCurFloatAISeqNo;
    *(pScanInf->aioOutArr+pScanInf->iCurFloatAISeqNo)=pFloatAINode->ioOut;
    pScanInf->iCurFloatAISeqNo++;
}

/*　功能：将FloatAISet的输出IO地址赋值给扫描节点中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的FloatAISet图元
  　返回：无  */
void RE_FloatAICollectInitAISetTuyuanOut(void *PartGrpAttrib,NODE *pCurScanNode)
{

    FloatAICollect_Scan_Type  *pScanInf;
    AISet_Scan_Node_Type  *pFloatAISetNode;
    int  i;
    int  iNodeAICnt;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskFloatAICollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pFloatAISetNode=(AISet_Scan_Node_Type  *)pCurScanNode->pTuyuan;
    iNodeAICnt=(pFloatAISetNode->ucOutputCnt_8)*8
               +pFloatAISetNode->ucOutputCnt_m;
    for(i=0; i<iNodeAICnt; i++)
    {
        assert(pScanInf->iCurFloatAISeqNo<pScanInf->iFloatAICnt);

        pFloatAISetNode->apCollectOutArr[i]=pScanInf->aioOutArr+pScanInf->iCurFloatAISeqNo;
        *(pScanInf->aioOutArr+pScanInf->iCurFloatAISeqNo)=pFloatAISetNode->ioOutArr[i];
        pScanInf->iCurFloatAISeqNo++;
    }
}
