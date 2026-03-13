/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_ImportCollect.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的Import集中功能的代码实现                       */
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
#include  "RE_ImportCollect.h"
#include   "string_compat.h"
#include   "RE_ImportTuyuan.h"
#include  "RE_ExportCollect.h"


/*扫描任务的Import图元集中信息数组  */
ImportCollect_Scan_Type  aTaskImportCollectArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/*功能：Import集中功能扫描
　参数：iTaskNo，所属任务
　返回：无  */
void RE_ImportCollectScan(int  iTaskNo)
{
    ImportCollect_Scan_Type  *pScanInf;
    EXPORT_OUTPUT_TYPE  ** ppInput;
    EP_ELEM_IO  * pCurOut;
    int  i;
    int  iImportCnt;

    pScanInf=aTaskImportCollectArr+iTaskNo;
    iImportCnt=pScanInf->iImportCnt;
    if(iImportCnt==0)
    {
        return;
    }
    ppInput=(EXPORT_OUTPUT_TYPE  **)pScanInf->apInputArr;
    pCurOut=pScanInf->aioOutArr;

    taskLock();/*为保证任务间数据访问的完整性，需要任务保护  */
    for(i=0; i<iImportCnt; i++)
    {
        pCurOut->now=(*ppInput)->Val;
        pCurOut->pvCh=(*ppInput)->pHdl;
        ppInput++;
        pCurOut++;
    }
    taskUnlock();

}


/*功能：Import集中功能初始化
　参数：iTaskNo ,所属任务
　返回：无  */
void RE_ImportCollectInit(int  iTaskNo)
{
    ImportCollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskImportCollectArr+iTaskNo;
    pScanInf->iImportCnt=0;
}

/*功能：Import集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_ImportCollectClearSeqNo(int  iTaskNo)
{
    ImportCollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskImportCollectArr+iTaskNo;
    pScanInf->iCurImportSeqNo=0;
}


/*　功能：对任务中的Import汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
  　返回：无  */
void  RE_ImportCollectAddNew(void *PartGrpAttrib)
{
    ImportCollect_Scan_Type  *pScanInf;
    PARTGRP_ATTRIB_TYPE *  pPartGrpAttr;

    pPartGrpAttr=(PARTGRP_ATTRIB_TYPE *)PartGrpAttrib;
    if(!(pPartGrpAttr->bRunFlag))
    {
        return;
    }

    pScanInf=aTaskImportCollectArr+pPartGrpAttr->nScanTaskNo;
    pScanInf->iImportCnt++;
}


/*功能：Import集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_ImportCollectMemInit(int  iTaskNo)
{
    ImportCollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskImportCollectArr+iTaskNo;
    if(!(pScanInf->iImportCnt))
    {
        return  ;
    }
    pScanInf->apInputArr=calloc(pScanInf->iImportCnt
                                ,sizeof(*(pScanInf->apInputArr)));
    pScanInf->aioOutArr=calloc(pScanInf->iImportCnt
                               ,sizeof(*(pScanInf->aioOutArr)));
}



/*　功能：将Import图元的信息添加到Import集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的Import图元
  　返回：无  */
void RE_ImportCollectAddByImportTuyuan(void *PartGrpAttrib,NODE *pCurScanNode)
{
    ImportCollect_Scan_Type  *pScanInf;
    ExternImportOuterInput_Scan_Node_Type  *pImportNode;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskImportCollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pImportNode=(ExternImportOuterInput_Scan_Node_Type  *)pCurScanNode->pTuyuan;

    assert(pScanInf->iCurImportSeqNo<pScanInf->iImportCnt);

    *(pScanInf->aioOutArr+pScanInf->iCurImportSeqNo)=pImportNode->ioOut;
    *(pScanInf->apInputArr+pScanInf->iCurImportSeqNo)=pImportNode->pCollectInput;

    pScanInf->iCurImportSeqNo++;

}



/*　功能：将Import的输出IO地址赋值给扫描节点中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的Import图元
  　返回：无  */
void RE_ImportCollectInitImportTuyuanOut(void *PartGrpAttrib,NODE *pCurScanNode)
{

    ImportCollect_Scan_Type  *pScanInf;
    ExternImportOuterInput_Scan_Node_Type  *pImportNode;

    if(!(((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->bRunFlag))
    {
        return;
    }
    pScanInf=aTaskImportCollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pImportNode=(ExternImportOuterInput_Scan_Node_Type  *)pCurScanNode->pTuyuan;

    assert(pScanInf->iCurImportSeqNo<pScanInf->iImportCnt);

    /*将Import集中后的OUT对应指针赋值给节点中的成员变量  */
    pImportNode->pCollectOut=pScanInf->aioOutArr+pScanInf->iCurImportSeqNo;

    pScanInf->iCurImportSeqNo++;
}

