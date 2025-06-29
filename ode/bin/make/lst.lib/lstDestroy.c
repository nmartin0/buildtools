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
 * $Log:	lstDestroy.c,v $
 * Revision 2.2  92/05/20  20:13:18  mrt
 * 	First checkin
 * 	[92/05/20  16:57:39  mrt]
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
static char sccsid[] = "@(#)lstDestroy.c	5.3 (Berkeley) 6/1/90";
#endif /* not lint */

/*-
 * LstDestroy.c --
 *	Nuke a list and all its resources
 */

#include	"lstInt.h"

/*-
 *-----------------------------------------------------------------------
 * Lst_Destroy --
 *	Destroy a list and free all its resources. If the freeProc is
 *	given, it is called with the datum from each node in turn before
 *	the node is freed.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The given list is freed in its entirety.
 *
 *-----------------------------------------------------------------------
 */
void
Lst_Destroy (l, freeProc)
    Lst	    	  	l;
    register void	(*freeProc)();
{
    register ListNode	ln;
    register ListNode	tln = NilListNode;
    register List 	list = (List)l;
    
    if (l == NILLST || ! l) {
	/*
	 * Note the check for l == (Lst)0 to catch uninitialized static Lst's.
	 * Gross, but useful.
	 */
	return;
    }
    
    if (freeProc) {
	for (ln = list->firstPtr;
	     ln != NilListNode && tln != list->firstPtr;
	     ln = tln) {
		 tln = ln->nextPtr;
		 (*freeProc) (ln->datum);
		 PFreeNode ((Address)ln);
	}
    } else {
	for (ln = list->firstPtr;
	     ln != NilListNode && tln != list->firstPtr;
	     ln = tln) {
		 tln = ln->nextPtr;
		 PFreeNode ((Address)ln);
	}
    }
    
    PFree ((Address)l);
}
