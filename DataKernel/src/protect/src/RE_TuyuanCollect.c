/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_TuyuanCollect.c                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该文件定义了保护功能模块中的图元集中功能的公共代码实现                       */
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
#include   "string_compat.h"
#include   "RE_OuterOutputTuyuan.h"
#include   "RE_DOSetTuyuan.h"
#include   "RE_EventTuyuan.h"
#include   "RE_ExportTuyuan.h"
#include  "RE_OuterInputTuyuan.h"
#include  "RE_ImportTuyuan.h"
#include  "RE_AISetTuyuan.h"
#include  "RE_DISetTuyuan.h"

#include  "RE_DOCollect.h"
#include  "RE_LampCollect.h"
#include  "RE_EventCollect.h"
#include  "RE_ExportCollect.h"
#include  "RE_FloatAICollect.h"
#include  "RE_CmplxAICollect.h"
#include  "RE_DICollect.h"
#include  "RE_ImportCollect.h"
#include  "RE_TuyuanCollect.h"


/* 对图元集中功能进行初始化.
 * Para:
 *     PartGrpScanNodeListArr, 扫描节点链表.
 *     PartGrpAttrib, 分图属性数组.
 *     nPartGrpSum, 分图个数.
 * Return:
 *     EP_STATUS, or EP_ERROR.
 */
EP_STATUS RE_TyuanCollectInit(LIST *PartGrpScanNodeListArr,
                              PARTGRP_ATTRIB_TYPE *PartGrpAttrib, int nPartGrpSum)
{
    int i;
    NODE *pCurScanNode;
    LIST *pCurPartGrpScanNodeList;
    PARTGRP_ATTRIB_TYPE *pCurPartGrpAttrib;
    int   iOutCnt;
    AISet_Scan_Node_Type  *pAISetNode;
    DISet_Scan_Node_Type  *pDISetNode;

    /* 对任务中的各种图元集中功能进行初始化 2011-7-27  ZY*/
    for(i=0; i<MAX_CREATE_RELAYFUNC_TASK_COUNT ; i++)
    {
        RE_DOCollectInit(i);
        RE_LampCollectInit(i);
        RE_EventCollectInit(i);
        RE_ExportCollectInit(i);
        RE_FloatAICollectInit(i);
        RE_CmplxAICollectInit(i);
        RE_DICollectInit(i);
        RE_ImportCollectInit(i);
    }

    /*统计各种图元集中个数  */
    pCurPartGrpScanNodeList = PartGrpScanNodeListArr;
    pCurPartGrpAttrib = PartGrpAttrib;

    for (i = 0; i<nPartGrpSum;
            i++, pCurPartGrpScanNodeList++, pCurPartGrpAttrib++)
    {
        pCurScanNode = RE_LstFirst(pCurPartGrpScanNodeList);

        for ( ; ; )
        {
            if (pCurScanNode == NULL)
            {
                /* 若到图元链表的尾部, 则跳出链表操作 */
                break;
            }
            switch(pCurScanNode->ulTuyuanType)
            {
                case RE_DI_OUTERINPUT_SCAN:
                    RE_DICollectAddNew(pCurPartGrpAttrib,1);
                    break;

                case RE_DI_SET_SCAN:
                    pDISetNode=(DISet_Scan_Node_Type   *)pCurScanNode->pTuyuan;
                    iOutCnt=(pDISetNode->ucOutputCnt_8)*8
                            +pDISetNode->ucOutputCnt_m;
                    RE_DICollectAddNew(pCurPartGrpAttrib,iOutCnt);
                    break;

                case RE_FLOAT_AI_OUTERINPUT_SCAN:
                    RE_FloatAICollectAddNew(pCurPartGrpAttrib,1);
                    break;

                case RE_COMPLEX_AI_OUTERINPUT_SCAN:
                    RE_CmplxAICollectAddNew(pCurPartGrpAttrib,1);
                    break;

                case RE_FLOAT_AI_SET_SCAN:
                    pAISetNode=(AISet_Scan_Node_Type   *)pCurScanNode->pTuyuan;
                    iOutCnt=(pAISetNode->ucOutputCnt_8)*8
                            +pAISetNode->ucOutputCnt_m;
                    RE_FloatAICollectAddNew(pCurPartGrpAttrib,iOutCnt);
                    break;

                case RE_CMPLX_AI_SET_SCAN:
                    pAISetNode=(AISet_Scan_Node_Type   *)pCurScanNode->pTuyuan;
                    iOutCnt=(pAISetNode->ucOutputCnt_8)*8
                            +pAISetNode->ucOutputCnt_m;
                    RE_CmplxAICollectAddNew(pCurPartGrpAttrib,iOutCnt);
                    break;

                case  RE_EXTERN_IMPORT_OUTERINPUT_SCAN:
                    RE_ImportCollectAddNew(pCurPartGrpAttrib);
                    break;

                case RE_EVENT_SCAN:
                    RE_EventCollectAddNew(pCurPartGrpAttrib);
                    break;

                case RE_DO_OUTEROUTPUT_SCAN:
                    RE_DOCollectAddNew(pCurPartGrpAttrib,1);
                    break;

                case RE_DO_SET_SCAN:
                    RE_DOCollectAddNew(pCurPartGrpAttrib
                                       ,((DOSet_Scan_Node_Type *)pCurScanNode->pTuyuan)->iDOCnt);
                    break;

                case RE_LAMP_OUTEROUTPUT_SCAN:
                    RE_LampCollectAddNew(pCurPartGrpAttrib);/* 2011-7-27  ZY */
                    break;

                case  RE_EXTERN_EXPORT_OUTEROUTPUT_SCAN:
                    RE_ExportCollectAddNew(pCurPartGrpAttrib);
                    break;

            }
            pCurScanNode = RE_LstNext(pCurScanNode);
        }
    }

    /* 对任务中的各种图元集中功能进行申请内存初始化 2011-7-27  ZY*/
    for(i=0; i<MAX_CREATE_RELAYFUNC_TASK_COUNT ; i++)
    {
        RE_DOCollectMemInit(i);
        RE_LampCollectMemInit(i);
        RE_EventCollectMemInit(i);
        RE_ExportCollectMemInit(i);
        RE_FloatAICollectMemInit(i);
        RE_CmplxAICollectMemInit(i);
        RE_DICollectMemInit(i);
        RE_ImportCollectMemInit(i);
    }


    /* 对任务中的各种图元集中功能进行操作计数清零 2011-7-27  ZY*/
    for(i=0; i<MAX_CREATE_RELAYFUNC_TASK_COUNT ; i++)
    {
        RE_DOCollectClearSeqNo(i);
        RE_LampCollectClearSeqNo(i);
        RE_EventCollectClearSeqNo(i);
        RE_ExportCollectClearSeqNo(i);
        RE_FloatAICollectClearSeqNo(i);
        RE_CmplxAICollectClearSeqNo(i);
        RE_DICollectClearSeqNo(i);
        RE_ImportCollectClearSeqNo(i);
    }

    /* 更新输入图元中被集中后的输入图元的IO指针索引 */
    pCurPartGrpScanNodeList = PartGrpScanNodeListArr;
    pCurPartGrpAttrib = PartGrpAttrib;

    for (i = 0; i<nPartGrpSum;
            i++, pCurPartGrpScanNodeList++, pCurPartGrpAttrib++)
    {
        pCurScanNode = RE_LstFirst(pCurPartGrpScanNodeList);

        for ( ; ; )
        {
            if (pCurScanNode == NULL)
            {
                /* 若到图元链表的尾部, 则跳出链表操作 */
                break;
            }

            switch(pCurScanNode->ulTuyuanType)
            {
                case RE_EXTERN_EXPORT_OUTEROUTPUT_SCAN:
                    RE_ExportCollectInitExportTuyuanOut(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_DI_OUTERINPUT_SCAN:
                    RE_DICollectInitDITuyuanOut(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_DI_SET_SCAN:
                    RE_DICollectInitDISetTuyuanOut(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_FLOAT_AI_OUTERINPUT_SCAN:
                    RE_FloatAICollectInitAITuyuanOut(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_COMPLEX_AI_OUTERINPUT_SCAN:
                    RE_CmplxAICollectInitAITuyuanOut(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_FLOAT_AI_SET_SCAN:
                    RE_FloatAICollectInitAISetTuyuanOut(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_CMPLX_AI_SET_SCAN:
                    RE_CmplxAICollectInitAISetTuyuanOut(pCurPartGrpAttrib,pCurScanNode);
                    break;

                case RE_EXTERN_IMPORT_OUTERINPUT_SCAN:
                    RE_ImportCollectInitImportTuyuanOut(pCurPartGrpAttrib,pCurScanNode);
                    break;
            }
            pCurScanNode = RE_LstNext(pCurScanNode);
        }
    }

    return EP_SUCCESS;
}