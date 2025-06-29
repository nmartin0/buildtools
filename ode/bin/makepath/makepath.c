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
 * $Log:	makepath.c,v $
 * Revision 2.4  93/05/11  19:21:01  mrt
 * 	Removed unconditional include of strings.h. There was aleady
 * 	a conditional one.
 * 	[93/05/11            mrt]
 * 
 * Revision 2.3  93/03/20  00:06:08  mrt
 * 	Added strerror code for porting.
 * 	Added include of strings.h to quiet gcc warnings.
 * 	[93/03/09            mrt]
 * 
 * Revision 2.2  92/05/20  20:15:11  mrt
 * 	First checkin
 * 	[92/05/20  18:02:41  mrt]
 * 
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile: makepath.c,v $ $Revision: 2.4 $ (OSF) $Date: 93/05/11 19:21:01 $";
#endif
/*
 *  makepath - create intermediate directories along a path
 *
 *  makepath path ...
 *
 *  Create any directories missing in path.
 */
#include <sys/param.h>

#ifdef NO_SYS_LIMITS
#include <limits.h>
#else
#include <sys/limits.h>
#endif

#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>

#ifndef _BLD
#include "ode/odedefs.h"
#endif

#if !defined(NO_STRERROR) && !defined(NO_STRDUP)
#include <strings.h>
#else /* one is defined */
#if defined(NO_STRERROR)
char * strerror();
#else
extern char * strerror();
#endif /* defined(NO_STRERROR) */
#if defined(NO_STRDUP)
char * strdup();
#else
extern char * strdup();
#endif /* defined(NO_STRDUP) */
#endif /* neither defined */

#ifndef	PATH_MAX
#define PATH_MAX	1024
#endif

#define	TRUE	1
#define	FALSE	0

static char *progname;		/* program name */

static
char *fixpath(pathname)
register char *pathname;
{
    register char *ls = NULL;
    register char *p = pathname;

    *p = *pathname;
    while (*pathname != '\0') {
	pathname++;
	while (*p == '/' && *pathname == '/')
	    pathname++;
	*++p = *pathname;
	if (*p == '/')
	    ls = p;
    }
    return(ls);
}


static
mkpath(pathname, trace)
char *pathname;
int trace;
{
    char *base;
    struct stat st;
    int ch, ididit;

    if (pathname == NULL) {
	fprintf(stderr, "%s: NULL path argument\n", progname);
	return(1);
    }

    pathname = (char *)strdup(pathname);
    if (pathname == NULL)
	fprintf(stderr, "%s: strdup failed\n", progname);
    base = fixpath(pathname);

    if (base == NULL || base == pathname) {
	fprintf(stderr, "%s: %s must have an imbedded '/' character\n",
		progname, pathname);
	return(1);
    }
    *base = '\0';
    base = pathname;
    if (*base == '/')
	base++;
    if (*base == '\0') {
	fprintf(stderr, "%s: illegal pathname %s\n", progname, pathname);
	return(1);
    }
    for (;;) {
	/* find end of this component */
	while (*base != '\0' && *base != '/')
	    base++;

	/* create path so far, if necessary */
	ch = *base;
	*base = '\0';
	if (stat(pathname, &st) < 0) {
	    if (mkdir(pathname, 0777) < 0) {
		if (errno != EEXIST) {
		    fprintf(stderr, "%s: unable to create directory %s: %s\n",
			    progname, pathname, strerror(errno));
		    return(1);
		}
		ididit = FALSE;
	    } else
		ididit = TRUE;
	    if (stat(pathname, &st) < 0) {
		fprintf(stderr, "%s: unable to stat directory %s: %s\n",
			progname, pathname, strerror(errno));
		return(1);
	    }
	    if (ididit && trace)
		fprintf(stderr, "%s: created directory\n", pathname);
	} else if ((st.st_mode&S_IFMT) != S_IFDIR) {
	    fprintf(stderr, "%s: %s is not a directory (mode %#o)\n",
		    progname, pathname, (st.st_mode&S_IFMT));
	    return(1);
	}
	if (ch == '\0')
	    break;
	*base++ = ch;
    }
    return(0);
}

main(argc, argv)
    int argc;
    char *argv[];
{
    int quiet = FALSE;
    int errors = 0;

    if (argc > 0) {
	if ((progname = (char *)rindex(argv[0], '/')) == NULL)
	    progname = argv[0];
	else
	    progname++;
    } else
	progname = "makepath";
    argc--;
    argv++;

    if (argc > 0 && strcmp(argv[0], "-version") == 0)
    {
	print_revision();
	exit(0);
    }

    if (argc > 0 && strcmp(argv[0], "-quiet") == 0) {
	quiet = TRUE;
	argc--;
	argv++;
    }

    if (argc == 0) {
	print_usage();
	exit(1);
    }

    if (strcmp(argv[0], "-") == 0) {	/* read stdin */
	char *pathname, *endp, *ptr;
	char buffer[PATH_MAX];

	while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
	    ptr = buffer;
	    while (*ptr == ' ' || *ptr == '\t')
		ptr++;
	    pathname = ptr;
	    while (*ptr && *ptr != '\n' && *ptr != ' ' && *ptr != '\t')
		ptr++;
	    endp = ptr;
	    while (*ptr == ' ' || *ptr == '\t')
		ptr++;
	    if (*ptr && *ptr != '\n') {
		fprintf(stderr, "%s: bad pathname: %s\n", progname, buffer);
		continue;
	    }
	    *endp = 0;
	    if (*pathname == 0)
		continue;
	    errors |= mkpath(pathname, !quiet);
	}
    } else {
	while (argc > 0) {
	    errors |= mkpath(argv[0], !quiet);
	    argc--;
	    argv++;
	}
    }
    exit(errors);
}


/* show the revision of this program */
print_revision()
{
    printf("%s $Revision: 2.4 $ $Date: 93/05/11 19:21:01 $\n", progname);
}


/* show invocation options */
print_usage()
{
    printf("usage: %s [ -version | -quiet ] path ...\n", progname);
}

#ifdef _BLD
#ifdef NO_STRDUP

/*
 * strdup and strerror come from BSD
 *
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

char *malloc();

char *strdup(p)
char *p;
{
	register char *q;
	register int l;

	q = malloc(l = strlen(p) + 1);
	if (q != 0)
		bcopy(p, q, l);
	return(q);
}
#endif /* NO_STRDUP */

#ifdef NO_STRERROR

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
#endif /* NO_STRERROR */

#endif  /* _BLD */
