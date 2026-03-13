#ifndef LSTLIB_H
#define LSTLIB_H



// 通用双向链表节点，需嵌入用户结构体首部
typedef struct node {
    struct node *next;
    struct node *previous;
} NODE;

// 链表头，管理节点和计数
typedef struct {
    NODE node; // 头结点（哨兵）
    int count;
} LIST;

void lstInit(LIST *pList);
void lstFree(LIST *pList);
void lstConcat(LIST *pDstList, LIST *pAddList);
void lstAdd(LIST *pList, NODE *pNode);
void lstInsert(LIST *pList, NODE *pPrevNode, NODE *pNode);
void lstDelete(LIST *pList, NODE *pNode);
NODE *lstGet(LIST *pList);
NODE *lstFirst(LIST *pList);
NODE *lstLast(LIST *pList);
NODE *lstNext(NODE *pNode);
NODE *lstPrevious(NODE *pNode);
NODE *lstNth(LIST *pList, int nodenum);
NODE *lstFind(LIST *pList, NODE *pNode);
int lstCount(LIST *pList);


#endif // LSTLIB_H