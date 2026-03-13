/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_ListLib.c                                   1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该源代码文件重载vxWorks的节点连表的的实现                          */
/*                                                                              */
/*      作者                                                                      */
/*                                                                              */
/*      张云                                                                  */
/*                                                                              */
/*                                                                               */
/*      开发记录                                                                     */
/*                                                                              */
/*         作者           日期                    说明                    */
/*                                                                              */
/*         张云       2003.4.5              创建文件1.0版本              */

/*                                                                              */
/********************************************************************************/

#ifndef RE_LISTLIB_C
#define RE_LISTLIB_C



#include "RE_ListLib.h"
#include  "stdlib_compat.h"




/* 连表函数实现 */

NODE *	RE_LstGet (LIST *pList)
{
    /*删除并返回第1个节点  */
    if((pList->count)>0)
    {
        NODE   *pRtNode;

        pRtNode=pList->node.next;
        (pList->count)--;
        pList->node.next=pRtNode->next;
        if(pList->node.next!=NULL)
        {
            pList->node.next->previous=&(pList->node);
        }
        return  pRtNode;

    }
    else
    {
        return  NULL;

    }

}


NODE *	RE_LstLast (LIST *pList)
{
    /*  返回最后1个节点 */
    if((pList->count)>0)
    {
        NODE   *pNextNode;

        pNextNode=pList->node.next;
        while(pNextNode->next)
        {
            pNextNode=pNextNode->next;

        }
        return   pNextNode;
    }
    else
    {

        return  NULL;

    }
}



NODE *	RE_LstNth (LIST *pList, int nodenum)
{
    /* 获得第N个节点，第1个节点的序号为1 */
    if(((pList->count)>=nodenum)
            &&(nodenum>0))
    {
        int  i;
        NODE   *pNextNode;

        pNextNode=pList->node.next;
        for(i=1; i<nodenum; i++)
        {
            pNextNode=pNextNode->next;
        }

        return   pNextNode;
    }
    else
    {

        return  NULL;

    }

}

NODE *	RE_LstPrevious (NODE *pNode)
{
    /* 获得前一个节点 */
    return   pNode->previous;

}


int 	RE_LstFind (LIST *pList, NODE *pNode)
{
    /* 查找节点, 第1个节点的序号为1，若未找到，返回ERROR*/
    int  nCounter;
    NODE   *pNextNode;

    nCounter=0;
    pNextNode=pList->node.next;
    while(pNextNode)
    {
        nCounter++;
        if(pNextNode==pNode)
        {
            return  nCounter;

        }
        pNextNode=pNextNode->next;

    }
    return   ERROR;

}


void 	RE_LstAdd (LIST *pList, NODE *pNode)
{
    /*在连表末尾添加1个节点  */

    NODE   *pNextNode;

    pNextNode=&(pList->node);
    while(pNextNode->next)
    {
        pNextNode=pNextNode->next;

    }
    (pList->count)++;
    pNextNode->next=pNode;
    pNode->previous=pNextNode;
    pNode->next=NULL;
    return;

}


void 	RE_LstDelete (LIST *pList, NODE *pNode)
{
    /*  删除和释放特定的节点，若存在的话，包括节点申请的内存*/

    NODE   *pNextNode;

    pNextNode=pList->node.next;
    while(pNextNode)
    {
        if(pNextNode==pNode)
        {
            pNextNode->previous->next=pNextNode->next;
            pNextNode->next->previous=pNextNode->previous;
            free(pNode);
            (pList->count)--;
            return  ;

        }
        pNextNode=pNextNode->next;

    }
    return  ;

}


void 	RE_LstFree (LIST *pList)
{
    /*释放连表，但连表节点申请的内存应该在调用此函数之前释放 */
    pList->node.next=NULL;
    pList->node.previous=NULL;
    pList->count=0;

}


void 	RE_LstInit (LIST *pList)
{
    /* 初始化连表 */
    pList->node.next=NULL;
    pList->node.previous=NULL;
    pList->count=0;

}


void 	RE_LstInsert (LIST *pList, NODE *pPrev, NODE *pNode)
{
    /*  在pPrev节点后面插入1个新节点*/

    NODE   *pNextNode;

    pNextNode=pList->node.next;
    while(pNextNode)
    {
        if(pNextNode==pPrev)
        {
            pNode->next=pNextNode->next;
            pNextNode->next=pNode;

            pNode->next->previous=pNode;
            pNode->previous=pNextNode;

            (pList->count)++;
            return  ;

        }
        pNextNode=pNextNode->next;

    }
    return  ;

}

/* 删除节点，不释放内存 */
void RE_LstDeleteNoFree(LIST *pList, NODE *pNode)
{
    NODE *pNextNode;

    pNextNode = pList->node.next;
    while (pNextNode)
    {
        if (pNextNode == pNode)
        {
            if (pNextNode->next != NULL)
            {
                pNextNode->previous->next = pNextNode->next;
                pNextNode->next->previous = pNextNode->previous;
            }
            else
            {
                pNextNode->previous->next = NULL;
            }
            (pList->count)--;
            return;
        }
        pNextNode=pNextNode->next;
    }
    return;
}

#endif


