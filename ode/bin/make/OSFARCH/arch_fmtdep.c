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
 * Revision 2.2  92/05/20  20:12:11  mrt
 * 	First checkin
 * 	[92/05/20  17:24:07  mrt]
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

#ifndef AR_EFMT1
#define	AR_EFMT1	"#1/"	/* extended format #1 */
#endif

struct ar_hdr *ArchStatMember();
FILE *ArchFindMember();

static struct ar_hdr size_arh;
#define AR_MAX_NAME_LEN	    (sizeof(size_arh.ar_name)-1)

ArchFixMembName(memberPtr)
char **memberPtr;
{
    if (DEBUG(ARCH)) {
	printf("ArchFixMembName(%s)\n", *memberPtr);
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
void *hdrInfo;
{
    char memName[NAME_MAX+1];
    char *cp;
    int  retval;
    int  count;

    if (fread ((char *)arhPtr, sizeof(struct ar_hdr), 1, arch) != 1)
	return 0;
    if (strncmp (arhPtr->ar_fmag, ARFMAG, sizeof(arhPtr->ar_fmag)) != 0) {
	/*
	 * The header is bogus, so the archive is bad
	 * and there's no way we can recover...
	 */
	return -1;
    }
    if (bcmp(arhPtr->ar_name, AR_EFMT1, sizeof(AR_EFMT1)-1) == 0 &&
	arhPtr->ar_name[sizeof(AR_EFMT1)-1] != ' ') {
	count = atoi(arhPtr->ar_name+sizeof(AR_EFMT1)-1);
	if (count <= 0 || count > NAME_MAX)
	    return -1;

	retval = fread(memName, count, (size_t) 1, arch);

	if (retval != count)
	    return -1;
	memName[count] = 0;

	sprintf (arhPtr->ar_size, "%hd",
		 (strtol(arhPtr->ar_size) - (long)count));

    } else {
	bcopy(arhPtr->ar_name, memName, sizeof(arhPtr->ar_name));
	memName[sizeof(arhPtr->ar_name)] = 0;
	cp = strchr(memName, '/');
	if (cp)
	    *cp = '\0';                      /* Mark end of member */
	else {
	    cp = memName + sizeof(arhPtr->ar_name) - 1;
	    while (cp > memName && *cp == ' ')
	      cp--;
	    *++cp = '\0';
	}
    }

    *memNamePtr = strdup(memName);
    return 1;
}

ArchToNextMember(arch, arhPtr, hdrInfo)
FILE *arch;
struct ar_hdr *arhPtr;
void *hdrInfo;
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
