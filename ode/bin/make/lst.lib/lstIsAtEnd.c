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
 * $Log:	lstIsAtEnd.c,v $
 * Revision 2.2  92/05/20  20:13:44  mrt
 * 	First checkin
 * 	[92/05/20  17:11:10  mrt]
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
static char sccsid[] = "@(#)lstIsAtEnd.c	5.3 (Berkeley) 6/1/90";
#endif /* not lint */

/*-
 * LstIsAtEnd.c --
 *	Tell if the current node is at the end of the list.
 *	The sequential functions access the list in a slightly different way.
 *	CurPtr points to their idea of the current node in the list and they
 *	access the list based on it. Because the list is circular, Lst_Next
 *	and Lst_Prev will go around the list forever. Lst_IsAtEnd must be
 *	used to determine when to stop.
 */

#include	"lstInt.h"

/*-
 *-----------------------------------------------------------------------
 * Lst_IsAtEnd --
 *	Return true if have reached the end of the given list.
 *
 * Results:
 *	TRUE if at the end of the list (this includes the list not being
 *	open or being invalid) or FALSE if not. We return TRUE if the list
 *	is invalid or unopend so as to cause the caller to exit its loop
 *	asap, the assumption being that the loop is of the form
 *	    while (!Lst_IsAtEnd (l)) {
 *	    	  ...
 *	    }
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
Boolean
Lst_IsAtEnd (l)
    Lst	    l;
{
    register List list = (List) l;

    return (!LstValid (l) || !list->isOpen ||
	    (list->atEnd == Head) || (list->atEnd == Tail));
}

