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
 * $Log:	waitpid.c,v $
 * Revision 2.4  93/05/08  00:06:02  mrt
 * 	Put a conditional around defintiton of pid_t as BNR2
 * 	sys/types.h also defines it.
 * 	[93/05/04            mrt]
 * 
 * Revision 2.3  93/03/20  00:40:34  mrt
 * 	Added include of stdlib.h to quiet warnings.
 * 	[93/03/09            mrt]
 * 
 * Revision 2.2  92/05/20  20:14:43  mrt
 * 	First checkin
 * 	[92/05/20  16:42:03  mrt]
 * 
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <stdlib.h>

#ifndef _PID_T_
#define _PID_T_
typedef int pid_t;
#endif

struct plist {
	struct plist *next;
	pid_t pid;
	int status;
} *plist, *plist_end;

pid_t
waitpid(pid, status, options)
	pid_t pid;
	int *status;
	int options;
{
	pid_t wpid;
	struct plist *temp, *temp2;

	if (plist) {
		if (pid == -1) {
			pid = plist->pid;
			*status = plist->status;
			temp = plist;
			plist = temp->next;
			free(temp);
			return(pid);
		}
		for (temp = plist; temp; temp2 = temp, temp = temp->next) {
			if (temp->pid != pid)
				continue;
			*status = temp->status;
			if (temp == plist)
				plist = temp->next;
			else
				temp2->next = temp->next;
			if (plist_end == temp)
				plist_end = temp2;
			free(temp);
			return(pid);
		}
	}
	for (;;) {
		wpid = wait3(status, options, 0);
		if (wpid < 0 || pid == -1 || wpid == pid)
			break;
		temp = (struct plist *) malloc(sizeof(struct plist));
		temp->pid = wpid;
		temp->status = *status;
		temp->next = 0;
		if (plist)
			plist_end->next = temp;
		else
			plist = temp;
		plist_end = temp;
	}
	return(wpid);
}
