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
 * $Log:	lstForEachFrom.c,v $
 * Revision 2.2  92/05/20  20:13:32  mrt
 * 	First checkin
 * 	[92/05/20  17:07:56  mrt]
 * 
 */
/*
 * Copyright (c) 1988, 1989, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)lstForEachFrom.c	5.3 (Berkeley) 6/1/90";
#endif /* not lint */

/*-
 * lstForEachFrom.c --
 *	Perform a given function on all elements of a list starting from
 *	a given point.
 */

#include	"lstInt.h"

/*-
 *-----------------------------------------------------------------------
 * Lst_ForEachFrom --
 *	Apply the given function to each element of the given list. The
 *	function should return 0 if traversal should continue and non-
 *	zero if it should abort. 
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Only those created by the passed-in function.
 *
 *-----------------------------------------------------------------------
 */
/*VARARGS2*/
void
Lst_ForEachFrom (l, ln, proc, d)
    Lst	    	    	l;
    LstNode    	  	ln;
    register int	(*proc)();
    register ClientData	d;
{
    register ListNode	tln = (ListNode)ln;
    register List 	list = (List)l;
    register ListNode	next;
    register ListNode	first;
    Boolean 	    	done;
    int     	    	result;
    
    if (!LstValid (list) || LstIsEmpty (list)) {
	return;
    }
    
    do {
	/*
	 * Take care of having the current element deleted out from under
	 * us.
	 */
	
	next = tln->nextPtr;
	first = list->firstPtr;
	
	tln->useCount++;
	result = (*proc) (tln->datum, d);
	tln->useCount--;

	/*
	 * We're done with the traversal if
	 *  - nothing's been added after the current node and
	 *  - the next node to examine is the first in the queue or
	 *    doesn't exist.
	 */
	done = (next == tln->nextPtr &&
		(next == NilListNode ||
		 (next == list->firstPtr &&
		  (first != tln || (tln->flags & LN_DELETED) == 0))));
	
	next = tln->nextPtr;

	if (tln->flags & LN_DELETED) {
	    PFreeNode((char *)tln);
	}
	tln = next;
    } while (!result && !LstIsEmpty(list) && !done);
    
}

