/*
 * Distributed as part of the Mach Operating System
 */
/*
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
 * $Log:	strerror.c,v $
 * Revision 2.3  93/05/08  00:04:53  mrt
 * 	Changed string.h to strings.h
 * 	[93/05/07            mrt]
 * 
 * Revision 2.2  92/05/20  20:14:37  mrt
 * 	First checkin
 * 	[92/05/20  16:41:28  mrt]
 * 
 */
/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strerror.c	5.4 (Berkeley) 6/24/90";
#endif /* LIBC_SCCS and not lint */

#include <strings.h>

char *
strerror(errnum)
	int errnum;
{
	extern int sys_nerr;
	extern char *sys_errlist[];
	static char ebuf[40];		/* 64-bit number + slop */

	if ((unsigned int)errnum < sys_nerr)
		return(sys_errlist[errnum]);
	(void)sprintf(ebuf, "Unknown error: %d", errnum);
	return(ebuf);
}
