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
 * $Log:	lstMember.c,v $
 * Revision 2.2  92/05/20  20:13:49  mrt
 * 	First checkin
 * 	[92/05/20  17:14:25  mrt]
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
static char sccsid[] = "@(#)lstMember.c	5.3 (Berkeley) 6/1/90";
#endif /* not lint */

/*-
 * lstMember.c --
 *	See if a given datum is on a given list.
 */

#include    "lstInt.h"

LstNode
Lst_Member (l, d)
    Lst	    	  	l;
    ClientData	  	d;
{
    List    	  	list = (List) l;
    register ListNode	lNode;

    lNode = list->firstPtr;
    if (lNode == NilListNode) {
	return NILLNODE;
    }
    
    do {
	if (lNode->datum == d) {
	    return (LstNode)lNode;
	}
	lNode = lNode->nextPtr;
    } while (lNode != NilListNode && lNode != list->firstPtr);

    return NILLNODE;
}
