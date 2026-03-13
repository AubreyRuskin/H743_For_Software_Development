#include <stdlib.h>
#include "lstLib.h"

void lstInit(LIST *pList) {
    if (pList == NULL) return;
    pList->node.next = NULL;
    pList->node.previous = NULL;
    pList->count = 0;
}

void lstFree(LIST *pList) {
    if (pList == NULL) return;
    NODE *node = pList->node.next;
    while (node != NULL) {
        NODE *next = node->next;
        free(node);
        node = next;
    }
    lstInit(pList);
}

void lstConcat(LIST *pDstList, LIST *pAddList) {
    if (pDstList == NULL || pAddList == NULL || pAddList->count == 0) return;

    if (pDstList->count == 0) {
        pDstList->node.next = pAddList->node.next;
        pDstList->node.previous = pAddList->node.previous;
        pDstList->count = pAddList->count;
    } else {
        NODE *dstTail = pDstList->node.previous;
        NODE *addHead = pAddList->node.next;

        if (dstTail != NULL) {
            dstTail->next = addHead;
        }
        if (addHead != NULL) {
            addHead->previous = dstTail;
        }
        pDstList->node.previous = pAddList->node.previous;
        pDstList->count += pAddList->count;
    }
    
    lstInit(pAddList);
}

void lstAdd(LIST *pList, NODE *pNode) {
    if (pList == NULL || pNode == NULL) return;

    pNode->next = NULL;
    NODE *tail = pList->node.previous;

    if (tail == NULL) { // 链表为空
        pList->node.next = pNode;
        pNode->previous = &pList->node;
    } else {
        tail->next = pNode;
        pNode->previous = tail;
    }
    
    pList->node.previous = pNode;
    pList->count++;
}

void lstInsert(LIST *pList, NODE *pPrevNode, NODE *pNode) {
    if (pList == NULL || pNode == NULL) return;

    NODE *pNextNode;

    if (pPrevNode == NULL) { // 插入到头部
        pNextNode = pList->node.next;
        pList->node.next = pNode;
        pNode->previous = &pList->node;
    } else {
        pNextNode = pPrevNode->next;
        pPrevNode->next = pNode;
        pNode->previous = pPrevNode;
    }

    if (pNextNode != NULL) {
        pNextNode->previous = pNode;
    } else { // 插入到尾部
        pList->node.previous = pNode;
    }
    pNode->next = pNextNode;
    
    pList->count++;
}

void lstDelete(LIST *pList, NODE *pNode) {
    if (pList == NULL || pNode == NULL || pNode == &pList->node) return;

    if (pNode->previous != NULL) {
        pNode->previous->next = pNode->next;
    }
    if (pNode->next != NULL) {
        pNode->next->previous = pNode->previous;
    }

    if (pList->node.next == pNode) { // 删除的是头节点
        pList->node.next = pNode->next;
    }
    if (pList->node.previous == pNode) { // 删除的是尾节点
        pList->node.previous = pNode->previous;
    }

    pList->count--;
}

NODE *lstGet(LIST *pList) {
    if (pList == NULL) return NULL;
    NODE *first = pList->node.next;
    if (first != NULL) {
        lstDelete(pList, first);
    }
    return first;
}

NODE *lstFirst(LIST *pList) {
    if (pList == NULL) return NULL;
    return pList->node.next;
}

NODE *lstLast(LIST *pList) {
    if (pList == NULL) return NULL;
    return pList->node.previous;
}

NODE *lstNext(NODE *pNode) {
    if (pNode == NULL) return NULL;
    return pNode->next;
}

NODE *lstPrevious(NODE *pNode) {
    if (pNode == NULL || pNode->previous == NULL || pNode->previous->previous == NULL) return NULL;
    return pNode->previous;
}

NODE *lstNth(LIST *pList, int nodenum) {
    if (pList == NULL || nodenum < 1 || nodenum > pList->count) return NULL;
    NODE *node = pList->node.next;
    for (int i = 1; i < nodenum; ++i) {
        if (node == NULL) return NULL;
        node = node->next;
    }
    return node;
}

NODE *lstFind(LIST *pList, NODE *pNode) {
    if (pList == NULL || pNode == NULL) return NULL;
    NODE *node = pList->node.next;
    while (node != NULL) {
        if (node == pNode) return node;
        node = node->next;
    }
    return NULL;
}

int lstCount(LIST *pList) {
    if (pList == NULL) return 0;
    return pList->count;
}
