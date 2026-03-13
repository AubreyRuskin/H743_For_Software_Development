/********************************************************************************/
/*                                                                              */
/*                  国电南自研发中心版权所有 （2002）                           */
/*                                                                              */
/********************************************************************************/

/********************************************************************************/
/*                                                                              */
/*       文件名                                           版本                 */
/*                                                                              */
/*      RE_ListLib.h                                    1.0                   */
/*                                                                              */
/*                                                                               */
/*     文件描述                                                                  */
/*                                                                              */
/*       该头文件定义了保护功能模块中的节点连表的文件头                    */
/*                                                                         */
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

#ifndef RE_LISTLIB_H
#define RE_LISTLIB_H



#include "vxWorks.h"

/* type definitions */

typedef struct node		/* Node of a linked list. */
{
    /* 这样排列，节点扫描时，cache命中率最高 */
    struct node *previous;	/* Points at the previous node in the list */
    unsigned  long   ulTuyuanType;  /* 实际图元类型，对初始化节点和扫描节点都有意义，
                  比如对扫描图元而言，与门根据不同的输入，实际就分许多扫描图元类型 */

    int nScanTaskNo; /* 节点所在任务号 */
    int nScanInterval;  /* 所在任务扫描间隔 */

    void   *  pvScanTuyuanLabel;  /*实际扫描图元的局部标号地址
                  该信息在扫描之前才填，只对扫描节点才有意义  */
    void   *pTuyuan;   /*  该图元的内容的指针   */
    struct node *next;		/* Points at the next node in the list */
} NODE;


/* HIDDEN */

typedef struct			/* Header for a linked list. */
{
    NODE node;			/* Header list node */
    int count;			/* Number of nodes in list */
} LIST;

/* END_HIDDEN */

/* 连表函数声明 */
NODE *	RE_LstGet (LIST *pList);
NODE *	RE_LstLast (LIST *pList);
NODE *	RE_LstNth (LIST *pList, int nodenum);
NODE *	RE_LstPrevious (NODE *pNode);
int 	RE_LstFind (LIST *pList, NODE *pNode);
void 	RE_LstAdd (LIST *pList, NODE *pNode);
void 	RE_LstDelete (LIST *pList, NODE *pNode);
void 	RE_LstFree (LIST *pList);
void 	RE_LstInit (LIST *pList);
void 	RE_LstInsert (LIST *pList, NODE *pPrev, NODE *pNode);

/* 从链表删除节点，不释放内存 */
extern void RE_LstDeleteNoFree(LIST *pList, NODE *pNode);

/* 在逻辑图扫描中使用的连表函数采用内联方式实现 */

__inline__   static  int 	RE_LstCount (LIST *pList)
{
    /* 获得节点总数  */
    return  pList->count;

}


__inline__   static  NODE *	RE_LstNext (NODE *pNode)
{
    /* 获得下一个节点 */
    return   pNode->next;

}


__inline__   static   NODE *	RE_LstFirst (LIST *pList)
{
    /* 返回第1个节点 */
    return   pList->node.next;

}

#endif

