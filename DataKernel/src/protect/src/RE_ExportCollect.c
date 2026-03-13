/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_ExportCollect.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的端口引出集中功能的代码实现                 */
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
#include    "RE_ExportCollect.h"
#include    "string_compat.h"
#include    "RE_ExportTuyuan.h"

/*扫描任务的端口引出图元集中信息数组  */
ExportCollect_Scan_Type  aTaskExportCollectArr[MAX_CREATE_RELAYFUNC_TASK_COUNT];


/*功能：端口引出集中功能扫描
　参数：iTaskNo，任务号
　返回：无  */
void RE_ExportCollectScan(int  iTaskNo)
{
    ExportCollect_Scan_Type  *pScanInf;
    int   i;
    VALUE_TYPE  **ppInVal;
    void  *** ppHdl;
    EXPORT_OUTPUT_TYPE  *  pOut;
    BOOL   *pRunFlag;
    int  iExportCnt;

    pScanInf=aTaskExportCollectArr+iTaskNo;
    iExportCnt=pScanInf->iExportCnt;
    if(iExportCnt==0)
    {
        return;
    }
    ppInVal=pScanInf->apInValArr;
    ppHdl=pScanInf->apInHdlArr;
    pRunFlag=pScanInf->aRunFlagArr;
    pOut=pScanInf->aOutArr;

    taskLock();/*为保证任务间数据访问的完整性，需要任务保护  */
    for(i=0; i<iExportCnt; i++)
    {
        if(*pRunFlag)
        {
            pOut->Val=*(*ppInVal);
            pOut->pHdl=*(*ppHdl);
        }

        ppInVal++;
        ppHdl++;
        pRunFlag++;
        pOut++;
    }/*for结束 */
    taskUnlock();
}


/*功能：端口引出集中功能初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_ExportCollectInit(int  iTaskNo)
{
    ExportCollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskExportCollectArr+iTaskNo;
    pScanInf->iExportCnt=0;
}

/*功能：端口引出集中功能清零操作计数
　参数：iTaskNo，所属分图
　返回：无  */
void RE_ExportCollectClearSeqNo(int  iTaskNo)
{
    ExportCollect_Scan_Type  *pScanInf;

    assert(iTaskNo<MAX_CREATE_RELAYFUNC_TASK_COUNT);
    pScanInf=aTaskExportCollectArr+iTaskNo;
    pScanInf->iCurExportSeqNo=0;
}


/*　功能：对任务中的端口引出图元汇总,添加新的然后计数
  　参数：PartGrpAttrib，所属分图
  　返回：无  */
void  RE_ExportCollectAddNew(void *PartGrpAttrib)
{
    ExportCollect_Scan_Type  *pScanInf;
    PARTGRP_ATTRIB_TYPE *  pPartGrpAttr;

    pPartGrpAttr=(PARTGRP_ATTRIB_TYPE *)PartGrpAttrib;

    pScanInf=aTaskExportCollectArr+pPartGrpAttr->nScanTaskNo;
    pScanInf->iExportCnt++;
}

/*功能：端口引出集中功能内存分配初始化
　参数：iTaskNo，所属分图
　返回：无  */
void RE_ExportCollectMemInit(int  iTaskNo)
{
    ExportCollect_Scan_Type  *pScanInf;

    pScanInf=aTaskExportCollectArr+iTaskNo;
    if(!(pScanInf->iExportCnt))
    {
        return  ;
    }

    pScanInf->apInValArr=calloc(pScanInf->iExportCnt
                                ,sizeof(*(pScanInf->apInValArr)));
    pScanInf->apInHdlArr=calloc(pScanInf->iExportCnt
                                ,sizeof(*(pScanInf->apInHdlArr)));
    pScanInf->aRunFlagArr=calloc(pScanInf->iExportCnt
                                 ,sizeof(*(pScanInf->aRunFlagArr)));
    pScanInf->aOutArr=calloc(pScanInf->iExportCnt
                             ,sizeof(*(pScanInf->aOutArr)));
}



/*　功能：将端口引出图元的信息添加到端口集中功能中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的指示灯图元
  　返回：无  */
void RE_ExportCollectAddByExportTuyuan(void *PartGrpAttrib,NODE *pCurScanNode)
{
    ExportCollect_Scan_Type  *pScanInf;
    ExternExportOuterOutput_Scan_Node_Type  *pExportNode;
    PARTGRP_ATTRIB_TYPE *  pPartGrpAttr;
    EXPORT_OUTPUT_TYPE  *pOut;

    pPartGrpAttr=(PARTGRP_ATTRIB_TYPE *)PartGrpAttrib;

    pScanInf=aTaskExportCollectArr+pPartGrpAttr->nScanTaskNo;
    pExportNode=(ExternExportOuterOutput_Scan_Node_Type  *)pCurScanNode->pTuyuan;
    assert(pScanInf->iCurExportSeqNo<pScanInf->iExportCnt);

    *(pScanInf->apInValArr+pScanInf->iCurExportSeqNo)=&(pExportNode->pInArr0->now);
    *(pScanInf->apInHdlArr+pScanInf->iCurExportSeqNo)=&(pExportNode->pInArr0->pvCh);
    if(pPartGrpAttr->bRunFlag)
    {
        *(pScanInf->aRunFlagArr+pScanInf->iCurExportSeqNo)=TRUE;
    }
    else
    {
        *(pScanInf->aRunFlagArr+pScanInf->iCurExportSeqNo)=FALSE;
    }

    /*注意,pOut值要在这里赋初值,因为其有可能不被运行  */
    pOut=pScanInf->aOutArr+pScanInf->iCurExportSeqNo;
    pOut->Val=pExportNode->ioOut.now;
    pOut->pHdl=pExportNode->ioOut.pvCh;

    pScanInf->iCurExportSeqNo++;
}


/*　功能：将Export的输出IO地址赋值给扫描节点中
  　参数：PartGrpAttrib，所属分图
  　参数：pCurScanNode，所添加的Export图元
  　返回：无  */
void RE_ExportCollectInitExportTuyuanOut(void *PartGrpAttrib,NODE *pCurScanNode)
{
    ExportCollect_Scan_Type  *pScanInf;
    ExternExportOuterOutput_Scan_Node_Type  *pExportNode;

    pScanInf=aTaskExportCollectArr+
             ((PARTGRP_ATTRIB_TYPE *)PartGrpAttrib)->nScanTaskNo;
    pExportNode=(ExternExportOuterOutput_Scan_Node_Type *)pCurScanNode->pTuyuan;

    assert(pScanInf->iCurExportSeqNo<pScanInf->iExportCnt);

    /*将Export集中后的OUT对应指针赋值给节点中的成员变量  */
    pExportNode->pCollectOut=pScanInf->aOutArr+pScanInf->iCurExportSeqNo;

    pScanInf->iCurExportSeqNo++;
}