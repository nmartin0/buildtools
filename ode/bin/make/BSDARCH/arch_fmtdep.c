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
 * $Log:	arch_fmtdep.c,v $
 * Revision 2.3  93/06/17  13:24:28  mrt
 * 	Fixed fence-post error in ArchReadMember.
 * 	[93/06/17            mrt]
 * 
 * Revision 2.2  92/05/20  20:12:07  mrt
 * 	Contitionalized use of void * under __STDC__ so it would compile
 * 	with mips compiler
 * 	[92/05/17            mrt]
 * 
 */
#include    <sys/types.h>
#include    <sys/time.h>
#include    <ctype.h>
#include    <ar.h>
#include    <ranlib.h>
#include    <stdio.h>
#include    "make.h"
#include    "hash.h"

#ifndef RANLIBMAG
#define RANLIBMAG "__.SYMDEF"
#endif

struct ar_hdr *ArchStatMember();
FILE *ArchFindMember();

static struct ar_hdr size_arh;
#define AR_MAX_NAME_LEN	    (sizeof(size_arh.ar_name)-1)

ArchFixMembName(memberPtr)
char **memberPtr;
{
    int len;
    char *member;

    if (DEBUG(ARCH)) {
	printf("ArchFixMembName(%s)\n", *memberPtr);
    }
    len = strlen(*memberPtr);
    if (len <= AR_MAX_NAME_LEN)
	return;
    member = emalloc(AR_MAX_NAME_LEN+1);
    strncpy(member, *memberPtr, AR_MAX_NAME_LEN);
    member[AR_MAX_NAME_LEN] = '\0';
    *memberPtr = member;
    if (DEBUG(ARCH)) {
	printf("ArchFixMembName: truncated to %s\n", *memberPtr);
    }
}

ArchReadHdr(arch, hdrInfoPtr)
FILE *arch;
void **hdrInfoPtr;
{
    char	  magic[SARMAG];

    if ((fread (magic, SARMAG, 1, arch) != 1) ||
    	(strncmp (magic, ARMAG, SARMAG) != 0))
	return 0;
    return 1;
}

ArchReadMember(arch, memNamePtr, arhPtr, hdrInfo)
FILE *arch;
char **memNamePtr;
struct ar_hdr *arhPtr;
#ifdef	__SDTC__
void *hdrInfo;
#else	__SDTC__
char *hdrInfo;
#endif	__SDTC__
{
    char memName[AR_MAX_NAME_LEN+1];
    char *cp;

    if (fread ((char *)arhPtr, sizeof(struct ar_hdr), 1, arch) != 1)
	return 0;
    if (strncmp (arhPtr->ar_fmag, ARFMAG, sizeof(arhPtr->ar_fmag)) != 0) {
	/*
	 * The header is bogus, so the archive is bad
	 * and there's no way we can recover...
	 */
	return -1;
    }
    (void) strncpy (memName, arhPtr->ar_name, sizeof(arhPtr->ar_name));
    for (cp = &memName[AR_MAX_NAME_LEN]; *cp == ' '; cp--)
	continue;
    if ( cp != &memName[AR_MAX_NAME_LEN] ) ++cp;
    *cp = '\0';
    *memNamePtr = (char *)strdup(memName);
    return 1;
}

ArchToNextMember(arch, arhPtr, hdrInfo)
FILE *arch;
struct ar_hdr *arhPtr;
#ifdef	__STDC__
void *hdrInfo;
#else	__STDC__
char *hdrInfo;
#endif	__STDC__
{
    int size;

    arhPtr->ar_size[sizeof(arhPtr->ar_size)-1] = '\0';
    (void) sscanf (arhPtr->ar_size, "%10d", &size);
    fseek (arch, (size + 1) & ~1, 1);
}

ArchTouchTOC(gn)
GNode *gn;
{
    FILE *	    arch;	/* Stream open to archive */
    struct ar_hdr   arh;      	/* Header describing table of contents */

    arch = ArchFindMember (gn->path, RANLIBMAG, &arh, "r+");
    if (arch != (FILE *) NULL) {
	sprintf(arh.ar_date, "%-12d", now);
	(void)fwrite ((char *)&arh, sizeof (struct ar_hdr), 1, arch);
	fclose (arch);
    }
}

ArchTOCTime(gn, oodatePtr)
GNode *gn;
Boolean *oodatePtr;
{
    struct ar_hdr *arhPtr;    /* Header for __.SYMDEF */
    int 	  modTimeTOC; /* The table-of-contents's mod time */

    arhPtr = ArchStatMember (gn->path, RANLIBMAG, FALSE);

    if (arhPtr != (struct ar_hdr *)NULL) {
	(void)sscanf (arhPtr->ar_date, "%12d", &modTimeTOC);

	if (DEBUG(ARCH) || DEBUG(MAKE)) {
	    printf("%s modified %s...", RANLIBMAG, Targ_FmtTime(modTimeTOC));
	}
	*oodatePtr = (gn->mtime > modTimeTOC);
    } else {
	/*
	 * A library w/o a table of contents is out-of-date
	 */
	if (DEBUG(ARCH) || DEBUG(MAKE)) {
	    printf("No t.o.c....");
	}
	*oodatePtr = TRUE;
    }
}
