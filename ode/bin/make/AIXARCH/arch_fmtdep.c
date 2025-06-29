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
 * Revision 2.2  92/05/20  20:12:05  mrt
 * 	First checkin
 * 	[92/05/20  17:22:59  mrt]
 * 
 */
#include    <sys/types.h>
#include    <sys/time.h>
#include    <ctype.h>
#include    <ar.h>
#include    <stdio.h>
#include    "make.h"
#include    "hash.h"

#define AR_MAX_NAME_LEN	    255

struct hdrInfo {
    long next_member, last_member, found_last;
};

ArchFixMembName(memberPtr)
char **memberPtr;
{
    int len;
    char *member;

    len = strlen(*memberPtr);
    if (len <= AR_MAX_NAME_LEN)
	return;
    member = emalloc(AR_MAX_NAME_LEN+1);
    strncpy(member, *memberPtr, AR_MAX_NAME_LEN);
    member[AR_MAX_NAME_LEN] = '\0';
    *memberPtr = member;
}

ArchReadHdr(arch, hdrInfoPtr)
FILE *arch;
void **hdrInfoPtr;
{
    struct fl_hdr flhdr;
    struct hdrInfo *hi;

    if ((fread (&flhdr, FL_HSZ, 1, arch) != 1) ||
    	(strncmp (flhdr.fl_magic, AIAMAG, SAIAMAG) != 0))
	return 0;
    hi = (struct hdrInfo *) emalloc(sizeof(struct hdrInfo));
    hi->last_member = atol(flhdr.fl_lstmoff);
    hi->next_member = atol(flhdr.fl_fstmoff);
    hi->found_last = 0;
    *hdrInfoPtr = (void *)hi;
    return 1;
}

ArchReadMember(arch, memNamePtr, arhPtr, hdrInfo)
FILE *arch;
char **memNamePtr;
struct ar_hdr *arhPtr;
void *hdrInfo;
{
    struct hdrInfo *hi = (struct hdrInfo *) hdrInfo;
    char memName[AR_MAX_NAME_LEN+1];
    long len;

    if (!hi->next_member || hi->found_last)
	return 0;
    hi->found_last = (hi->next_member == hi->last_member);

    fseek(arch, hi->next_member, 0);
    fread(arhPtr, AR_HSZ, 1, arch);
    memName[0] = arhPtr->_ar_name.ar_name[0];
    memName[1] = arhPtr->_ar_name.ar_name[1];
    len = atol(arhPtr->ar_namlen);
    fread(&memName[2], len - 2, 1, arch);
    memName[len] = '\0';
    *memNamePtr = strdup(memName);
    return 1;
}

ArchToNextMember(arch, arhPtr, hdrInfo)
FILE *arch;
struct ar_hdr *arhPtr;
void *hdrInfo;
{
    struct hdrInfo *hi = (struct hdrInfo *) hdrInfo;

    hi->next_member = atol(arhPtr->ar_nxtmem);
}

ArchTouchTOC(gn)
GNode *gn;
{
}

ArchTOCTime(gn, oodatePtr)
GNode *gn;
Boolean *oodatePtr;
{
    *oodatePtr = FALSE;
}
