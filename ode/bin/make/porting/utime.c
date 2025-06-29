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
 * $Log:	utime.c,v $
 * Revision 2.2  92/05/20  20:14:39  mrt
 * 	First checkin
 * 	[92/05/20  16:41:39  mrt]
 * 
 */

#include <utime.h>
#include <sys/time.h>

utime(name, times)
	char *name;
	struct utimbuf *times;
{
	struct timeval tv[2];

        if (times) {
		tv[0].tv_sec  = times->actime;
		tv[1].tv_sec  = times->modtime;
		tv[0].tv_usec = tv[1].tv_usec = 0;
	}
	return(utimes(name, times ? tv : (struct timeval *)0));
}
