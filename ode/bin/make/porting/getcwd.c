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
 * $Log:	getcwd.c,v $
 * Revision 2.2  92/05/20  20:14:29  mrt
 * 	First checkin
 * 	[92/05/20  16:40:40  mrt]
 * 
 */
/*
 * Portable version for getcwd suitable for use by make, but not for others
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <stdio.h>

extern int errno;

#ifndef PATH_MAX
#define PATH_MAX MAXPATHLEN
#endif

char *
getcwd(buf, bufsize)
    char *buf;
    int bufsize;
{
    if (bufsize != PATH_MAX + 1) {
	errno = ERANGE;
	return(NULL);
    }
    if (getwd(buf) == NULL) {
	if (errno == 0)
	    errno = EACCES;	/* better than 0 ? */
	return(NULL);
    }
    return(buf);
}
