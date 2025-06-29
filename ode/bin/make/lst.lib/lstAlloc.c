/*
 * Distributed as part of the Mach Operating System
 */
/*
 * @OSF_FREE_COPYRIGHT@
 * 
 * Copyright (c) 1990, 1991
 * Open Software Foundation, Inc.
 * 
 * Permission is hereby granted to use, copy, modify and freely distribute
 * the software in this file and its documentation for any purpose without
 * fee, provided that the above copyright notice appears in all copies and
 * that both the copyright notice and this permission notice appear in
 * supporting documentation.  Further, provided that the name of Open
 * Software Foundation, Inc. ("OSF") not be used in advertising or
 * publicity pertaining to distribution of the software without prior
 * written permission from OSF.  OSF makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */
/*
 * HISTORY
 * $Log:	lstAlloc.c,v $
 * Revision 2.3  93/03/20  00:04:37  mrt
 * 	Added include of  stdlib.h for malloc.
 * 	[93/03/09            mrt]
 * 
 * Revision 2.2  92/05/20  20:12:56  mrt
 * 	First checkin
 * 
 * 	$EndLog$
 * 	[92/05/20  16:43:43  mrt]
 * 
 */
/*
 * Copyright (c) 1988, 1989, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "%W% (Berkeley) %G%";
#endif /* not lint */

/*-
 * LstAlloc.c --
 *	Allocation and Deallocation of List and ListNode structures
 */

#include	"lstInt.h"
#include	<stdlib.h>

List ListFreeList;
int nListFree;
List ListNodeFreeList;
int nListNodeFree;
int inited = 0;

#define InitList(list)					\
    (list) = (List)malloc(sizeof(*(list)));		\
    (list)->firstPtr = (list)->lastPtr = NilListNode;	\
    (list)->isOpen = (list)->isCirc = FALSE;		\
    (list)->atEnd = Unknown

#define RemoveFront(list, front)			\
    if ((front)->nextPtr != NilListNode)		\
	(front)->nextPtr->prevPtr = (front)->prevPtr;	\
    if ((front)->prevPtr != NilListNode)		\
	(front)->prevPtr->nextPtr = (front)->nextPtr;	\
    if ((list)->firstPtr == (front))			\
	(list)->firstPtr = (front)->nextPtr;		\
    if ((list)->lastPtr == (front))			\
	(list)->lastPtr = (front)->prevPtr

#define InsertFront(list, front, ln)			\
    (ln)->useCount = (ln)->flags = 0;			\
    if ((front) == NilListNode) {			\
	(ln)->prevPtr = (ln)->nextPtr = NilListNode;	\
	(list)->lastPtr = (ln);				\
    } else {						\
	(ln)->prevPtr = (front)->prevPtr;		\
	(ln)->nextPtr = (front);			\
	if ((ln)->prevPtr != NilListNode)		\
	    (ln)->prevPtr->nextPtr = (ln);		\
	(front)->prevPtr = (ln);			\
    }							\
    (list)->firstPtr = (ln)


List
PAlloc()
{
    List l, list;
    ListNode ln;
    register ListNode front;

    if (!inited) {
	InitList(ListFreeList);
	InitList(ListNodeFreeList);
	nListFree = nListNodeFree = 0;
	inited = 1;
    }
    if (nListFree == 0)
	return((List)malloc(sizeof(*l)));
    list = ListFreeList;
    front = list->firstPtr;
    l = (List) front->datum;
    RemoveFront(list, front);
    PFreeNode ((Address)front);
    nListFree--;
    return(l);
}

ListNode
PAllocNode()
{
    register ListNode front;
    register List list = (List)ListNodeFreeList;

    if (nListNodeFree == 0)
	return((ListNode)malloc(sizeof(*front)));
    front = list->firstPtr;
    RemoveFront(list, front);
    nListNodeFree--;
    return(front);
}

void
PFree(l)
    List l;
{
    register List list = ListFreeList;
    register ListNode front;
    register ListNode ln;

    front = list->firstPtr;
    ln = PAllocNode();
    ln->datum = (ClientData)l;
    InsertFront(list, front, ln);
    nListFree++;
}

void PFreeNode(ln)
    ListNode ln;
{
    register List 	list = ListNodeFreeList;
    register ListNode	front;

    front = list->firstPtr;
    ln->datum = 0;
    InsertFront(list, front, ln);
    nListNodeFree++;
}
