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
 * $Log:	arch.c,v $
 * Revision 2.2  92/05/20  20:12:14  mrt
 * 	Conditionalized void * on __STDC__ so that it would compile with
 * 	mips compiler
 * 	[92/05/20            mrt]
 * 
 */
/*
 * Copyright (c) 1988, 1989, 1990 The Regents of the University of California.
 * Copyright (c) 1988, 1989 by Adam de Boor
 * Copyright (c) 1989 by Berkeley Softworks
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
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

#ifndef lint
static char sccsid[] = "@(#)arch.c	5.7 (Berkeley) 12/28/90";
#endif /* not lint */

/*-
 * arch.c --
 *	Functions to manipulate libraries, archives and their members.
 *
 *	Once again, cacheing/hashing comes into play in the manipulation
 * of archives. The first time an archive is referenced, all of its members'
 * headers are read and hashed and the archive closed again. All hashed
 * archives are kept on a list which is searched each time an archive member
 * is referenced.
 *
 * The interface to this module is:
 *	Arch_ParseArchive   	Given an archive specification, return a list
 *	    	  	    	of GNode's, one for each member in the spec.
 *	    	  	    	FAILURE is returned if the specification is
 *	    	  	    	invalid for some reason.
 *
 *	Arch_Touch	    	Alter the modification time of the archive
 *	    	  	    	member described by the given node to be
 *	    	  	    	the current time.
 *
 *	Arch_TouchLib	    	Update the modification time of the library
 *	    	  	    	described by the given node. This is special
 *	    	  	    	because it also updates the modification time
 *	    	  	    	of the library's table of contents.
 *
 *	Arch_MTime	    	Find the modification time of a member of
 *	    	  	    	an archive *in the archive*. The time is also
 *	    	  	    	placed in the member's GNode. Returns the
 *	    	  	    	modification time.
 *
 *	Arch_MemTime	    	Find the modification time of a member of
 *	    	  	    	an archive. Called when the member doesn't
 *	    	  	    	already exist. Looks in the archive for the
 *	    	  	    	modification time. Returns the modification
 *	    	  	    	time.
 *
 *	Arch_FindLib	    	Search for a library along a path. The
 *	    	  	    	library name in the GNode should be in
 *	    	  	    	-l<name> format.
 *
 *	Arch_LibOODate	    	Special function to decide if a library node
 *	    	  	    	is out-of-date.
 *
 *	Arch_Init 	    	Initialize this module.
 */

#include    <sys/types.h>
#include    <sys/time.h>
#include    <ctype.h>
#include    <ar.h>
#include    <stdio.h>
#ifdef NO_UTIMES
#include    <utime.h>
#endif
#include    "make.h"
#include    "hash.h"

static Lst	  archives;   /* Lst of archives we've already examined */

typedef struct Arch {
    char	  *name;      /* Name of archive */
    Hash_Table	  members;    /* All the members of the archive described
			       * by <name, struct ar_hdr *> key/value pairs */
} Arch;

FILE *ArchFindMember();

/*-
 *-----------------------------------------------------------------------
 * Arch_ParseArchive --
 *	Parse the archive specification in the given line and find/create
 *	the nodes for the specified archive members, placing their nodes
 *	on the given list.
 *
 * Results:
 *	SUCCESS if it was a valid specification. The linePtr is updated
 *	to point to the first non-space after the archive spec. The
 *	nodes for the members are placed on the given list.
 *
 * Side Effects:
 *	Some nodes may be created. The given list is extended.
 *
 *-----------------------------------------------------------------------
 */
ReturnStatus
Arch_ParseArchive (linePtr, nodeLst, ctxt)
    char	    **linePtr;      /* Pointer to start of specification */
    Lst	    	    nodeLst;   	    /* Lst on which to place the nodes */
    GNode   	    *ctxt;  	    /* Context in which to expand variables */
{
    register char   *cp;	    /* Pointer into line */
    GNode	    *gn;     	    /* New node */
    char	    *libName;  	    /* Library-part of specification */
    char	    *memName;  	    /* Member-part of specification */
    char	    *nameBuf;       /* temporary place for node name */
    char	    saveChar;  	    /* Ending delimiter of member-name */

    libName = *linePtr;
    
    for (cp = libName; *cp != '(' && *cp != '\0'; cp++) {
	if (*cp == '$') {
	    /*
	     * Variable spec, so call the Var module to parse the puppy
	     * so we can safely advance beyond it...
	     */
	    cp = Var_Skip(cp, ctxt, TRUE);
	    cp--;
	}
    }

    *cp++ = '\0';

    while (1) {
	/*
	 * First skip to the start of the member's name, mark that
	 * place and skip to the end of it (either white-space or
	 * a close paren).
	 */
	while (*cp != '\0' && *cp != ')' && isspace (*cp)) {
	    cp++;
	}
	memName = cp;
	while (*cp != '\0' && *cp != ')' && !isspace (*cp)) {
	    if (*cp == '$') {
		/*
		 * Variable spec, so call the Var module to parse the puppy
		 * so we can safely advance beyond it...
		 */
		cp = Var_Skip(cp, ctxt, TRUE);
	    } else {
		cp++;
	    }
	}

	/*
	 * If the specification ends without a closing parenthesis,
	 * chances are there's something wrong (like a missing backslash),
	 * so it's better to return failure than allow such things to happen
	 */
	if (*cp == '\0') {
	    printf("No closing parenthesis in archive specification\n");
	    return (FAILURE);
	}

	/*
	 * If we didn't move anywhere, we must be done
	 */
	if (cp == memName) {
	    break;
	}

	saveChar = *cp;
	*cp = '\0';

	/*
	 * XXX: This should be taken care of intelligently by
	 * SuffExpandChildren, both for the archive and the member portions.
	 */
	nameBuf = emalloc(strlen(libName)+strlen(memName)+3);
	sprintf(nameBuf, "%s(%s)", libName, memName);
	gn = Targ_FindNode (nameBuf, TARG_CREATE);
	free(nameBuf);
	if (gn == NILGNODE) {
	    return (FAILURE);
	} else {
	    /*
	     * We've found the node, but have to make sure the rest of the
	     * world knows it's an archive member, without having to
	     * constantly check for parentheses, so we type the thing with
	     * the OP_ARCHV bit before we place it on the end of the
	     * provided list.
	     */
	    gn->type |= OP_ARCHV;
	    (void) Lst_AtEnd (nodeLst, (ClientData)gn);
	}
	
	*cp = saveChar;
    }

    /*
     * We promised the pointer would be set up at the next non-space, so
     * we must advance cp there before setting *linePtr... (note that on
     * entrance to the loop, cp is guaranteed to point at a ')')
     */
    do {
	cp++;
    } while (*cp != '\0' && isspace (*cp));

    *linePtr = cp;
    return (SUCCESS);
}

/*-
 *-----------------------------------------------------------------------
 * ArchFindArchive --
 *	See if the given archive is the one we are looking for. Called
 *	From ArchStatMember and ArchFindMember via Lst_Find.
 *
 * Results:
 *	0 if it is, non-zero if it isn't.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static int
ArchFindArchive (ar, archName)
    Arch	  *ar;	      	  /* Current list element */
    char	  *archName;  	  /* Name we want */
{
    return (strcmp (archName, ar->name));
}

/*-
 *-----------------------------------------------------------------------
 * ArchStatMember --
 *	Locate a member of an archive, given the path of the archive and
 *	the path of the desired member.
 *
 * Results:
 *	A pointer to the current struct ar_hdr structure for the member. Note
 *	That no position is returned, so this is not useful for touching
 *	archive members. This is mostly because we have no assurances that
 *	The archive will remain constant after we read all the headers, so
 *	there's not much point in remembering the position...
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
struct ar_hdr *
ArchStatMember (archive, member, hash)
    char	  *archive;   /* Path to the archive */
    char	  *member;    /* Name of member. If it is a path, only the
			       * last component is used. */
    Boolean	  hash;	      /* TRUE if archive should be hashed if not
    			       * already so. */
{
    FILE *	  arch;	      /* Stream to archive */
    char	  *cp;	      /* Useful character pointer */
    LstNode	  ln;	      /* Lst member containing archive descriptor */
    Arch	  *ar;	      /* Archive descriptor */
    Hash_Entry	  *he;	      /* Entry containing member's description */
    struct ar_hdr arh;        /* archive-member header for reading archive */
    char	  *memName;   /* Current member name while hashing */
#ifdef	__STDC__
    void	  *hdrInfo;   /* machine dependent header info */
#else	__STDC__
    char	  *hdrInfo;
#endif	__STDC__
    int		  status;     /* status from reading archive member */

    /*
     * Because of space constraints and similar things, files are archived
     * using their final path components, not the entire thing, so we need
     * to point 'member' to the final component, if there is one, to make
     * the comparisons easier...
     */
    cp = rindex (member, '/');
    if (cp != (char *) NULL) {
	member = cp + 1;
    }
    ArchFixMembName(&member);

    ln = Lst_Find (archives, (ClientData) archive, ArchFindArchive);
    if (ln != NILLNODE) {
	ar = (Arch *) Lst_Datum (ln);

	he = Hash_FindEntry (&ar->members, member);

	if (he != (Hash_Entry *) NULL) {
	    return ((struct ar_hdr *) Hash_GetValue (he));
	} else {
	    return ((struct ar_hdr *) NULL);
	}
    }

    if (!hash) {
	/*
	 * Caller doesn't want the thing hashed, just use ArchFindMember
	 * to read the header for the member out and close down the stream
	 * again. Since the archive is not to be hashed, we assume there's
	 * no need to allocate extra room for the header we're returning,
	 * so just declare it static.
	 */
	 static struct ar_hdr	sarh;

	 arch = ArchFindMember(archive, member, &sarh, "r");

	 if (arch == (FILE *)NULL) {
	    return ((struct ar_hdr *)NULL);
	} else {
	    fclose(arch);
	    return (&sarh);
	}
    }

    /*
     * We don't have this archive on the list yet, so we want to find out
     * everything that's in it and cache it so we can get at it quickly.
     */
    arch = fopen (archive, "r");
    if (arch == (FILE *) NULL) {
	return ((struct ar_hdr *) NULL);
    }
    
    /*
     * We use the "magic" string to make sure this is an archive we
     * can handle...
     */
    if (!ArchReadHdr(arch, &hdrInfo)) {
	fclose (arch);
	return ((struct ar_hdr *) NULL);
    }

    ar = (Arch *)emalloc (sizeof (Arch));
    ar->name = strdup (archive);
    Hash_InitTable (&ar->members, -1);
    
    while ((status = ArchReadMember(arch, &memName, &arh, hdrInfo)) == 1) {
	he = Hash_CreateEntry (&ar->members, memName, (Boolean *)NULL);
	Hash_SetValue (he, (ClientData)emalloc (sizeof (struct ar_hdr)));
	bcopy ((Address)&arh, (Address)Hash_GetValue (he), 
	    sizeof (struct ar_hdr));
	/*
	 * We need to advance the stream's pointer to the start of the
	 * next header. Files are padded with newlines to an even-byte
	 * boundary, so we need to extract the size of the file from the
	 * 'size' field of the header and round it up during the seek.
	 */
	ArchToNextMember(arch, &arh, hdrInfo);
    }

    fclose (arch);

    if (status == -1) {
	Hash_DeleteTable (&ar->members);
	free ((Address)ar);
	return ((struct ar_hdr *) NULL);
    }

    (void) Lst_AtEnd (archives, (ClientData) ar);

    /*
     * Now that the archive has been read and cached, we can look into
     * the hash table to find the desired member's header.
     */
    he = Hash_FindEntry (&ar->members, member);

    if (he != (Hash_Entry *) NULL) {
	return ((struct ar_hdr *) Hash_GetValue (he));
    } else {
	return ((struct ar_hdr *) NULL);
    }
}

/*-
 *-----------------------------------------------------------------------
 * ArchFindMember --
 *	Locate a member of an archive, given the path of the archive and
 *	the path of the desired member. If the archive is to be modified,
 *	the mode should be "r+", if not, it should be "r".
 *
 * Results:
 *	An FILE *, opened for reading and writing, positioned at the
 *	start of the member's struct ar_hdr, or NULL if the member was
 *	nonexistent. The current struct ar_hdr for member.
 *
 * Side Effects:
 *	The passed struct ar_hdr structure is filled in.
 *
 *-----------------------------------------------------------------------
 */
FILE *
ArchFindMember (archive, member, arhPtr, mode)
    char	  *archive;   /* Path to the archive */
    char	  *member;    /* Name of member. If it is a path, only the
			       * last component is used. */
    struct ar_hdr *arhPtr;    /* Pointer to header structure to be filled in */
    char	  *mode;      /* The mode for opening the stream */
{
    FILE *	  arch;	      /* Stream to archive */
    char	  *cp;	      /* Useful character pointer */
#ifdef	__STDC__
    void	  *hdrInfo;
#else	__STDC__
    char	  *hdrInfo;
#endif	__STDC__
    int		  status;
    char	 *memName;

    arch = fopen (archive, mode);
    if (arch == (FILE *) NULL) {
	return ((FILE *) NULL);
    }
    
    /*
     * We use the "magic" string to make sure this is an archive we
     * can handle...
     */
    if (!ArchReadHdr(arch, &hdrInfo)) {
	fclose (arch);
	return ((FILE *) NULL);
    }

    /*
     * Because of space constraints and similar things, files are archived
     * using their final path components, not the entire thing, so we need
     * to point 'member' to the final component, if there is one, to make
     * the comparisons easier...
     */
    cp = rindex (member, '/');
    if (cp != (char *) NULL) {
	member = cp + 1;
    }
    ArchFixMembName(&member);
    
    while ((status = ArchReadMember(arch, &memName, arhPtr, hdrInfo)) == 1) {
	if (strcmp(member, memName) == 0) {
	    /*
	     * To make life easier, we reposition the file at the start
	     * of the header we just read before we return the stream.
	     * In a more general situation, it might be better to leave
	     * the file at the actual member, rather than its header, but
	     * not here...
	     */
	    free (memName);
	    fseek (arch, -sizeof(struct ar_hdr), 1);
	    return (arch);
	}
	/*
	 * This isn't the member we're after, so we need to advance the
	 * stream's pointer to the start of the next header. Files are
	 * padded with newlines to an even-byte boundary, so we need to
	 * extract the size of the file from the 'size' field of the
	 * header and round it up during the seek.
	 */
	free (memName);
	ArchToNextMember(arch, arhPtr, hdrInfo);
    }

    /*
     * We've looked everywhere, but the member is not to be found. Close the
     * archive and return NULL -- an error.
     */
    fclose (arch);
    return ((FILE *) NULL);
}

/*-
 *-----------------------------------------------------------------------
 * Arch_Touch --
 *	Touch a member of an archive.
 *
 * Results:
 *	The 'time' field of the member's header is updated.
 *
 * Side Effects:
 *	The modification time of the entire archive is also changed.
 *	For a library, this could necessitate the re-ranlib'ing of the
 *	whole thing.
 *
 *-----------------------------------------------------------------------
 */
void
Arch_Touch (gn)
    GNode	  *gn;	  /* Node of member to touch */
{
    FILE *	  arch;	  /* Stream open to archive, positioned properly */
    struct ar_hdr arh;	  /* Current header describing member */

    arch = ArchFindMember(Var_Value (ARCHIVE, gn),
			  Var_Value (TARGET, gn),
			  &arh, "r+");
    sprintf(arh.ar_date, "%-12d", now);

    if (arch != (FILE *) NULL) {
	(void)fwrite ((char *)&arh, sizeof (struct ar_hdr), 1, arch);
	fclose (arch);
    }
}

/*-
 *-----------------------------------------------------------------------
 * Arch_TouchLib --
 *	Given a node which represents a library, touch the thing, making
 *	sure that the table of contents also is touched.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Both the modification time of the library and of the RANLIBMAG
 *	member are set to 'now'.
 *
 *-----------------------------------------------------------------------
 */
void
Arch_TouchLib (gn)
    GNode	    *gn;      	/* The node of the library to touch */
{
#ifdef NO_UTIMES
  struct utimbuf times;
#else
  struct timeval  times[2];   /* Times for utimes() call */
#endif

  ArchTouchTOC(gn);
#ifdef NO_UTIMES
  times.actime = times.modtime = now;
  utime(gn->path, times);
#else
  times[0].tv_sec = times[1].tv_sec = now;
  times[0].tv_usec = times[1].tv_usec = 0;
  utimes(gn->path, times);
#endif
}

/*-
 *-----------------------------------------------------------------------
 * Arch_MTime --
 *	Return the modification time of a member of an archive.
 *
 * Results:
 *	The modification time (seconds).
 *
 * Side Effects:
 *	The mtime field of the given node is filled in with the value
 *	returned by the function.
 *
 *-----------------------------------------------------------------------
 */
int
Arch_MTime (gn)
    GNode	  *gn;	      /* Node describing archive member */
{
    struct ar_hdr *arhPtr;    /* Header of desired member */
    int		  modTime;    /* Modification time as an integer */

    arhPtr = ArchStatMember (Var_Value (ARCHIVE, gn),
			     Var_Value (TARGET, gn),
			     TRUE);
    if (arhPtr != (struct ar_hdr *) NULL) {
	(void)sscanf (arhPtr->ar_date, "%12d", &modTime);
    } else {
	modTime = 0;
    }

    /*
     * If we have made our associated member node, then we want to
     * update ourselves to correspond to his time so that our library
     * will consider us out-of-date.
     */
    if (gn->mtime && gn->cmtime > gn->mtime)
	modTime = gn->cmtime;

    gn->mtime = modTime;
    return (modTime);
}

/*-
 *-----------------------------------------------------------------------
 * Arch_MemMTime --
 *	Given a non-existent archive member's node, get its modification
 *	time from its archived form, if it exists.
 *
 * Results:
 *	The modification time.
 *
 * Side Effects:
 *	The mtime field is filled in.
 *
 *-----------------------------------------------------------------------
 */
int
Arch_MemMTime (gn)
    GNode   	  *gn;
{
    LstNode 	  ln;
    GNode   	  *pgn;
    char    	  *nameStart,
		  *nameEnd;

    if (Lst_Open (gn->parents) != SUCCESS) {
	gn->mtime = 0;
	return (0);
    }
    while ((ln = Lst_Next (gn->parents)) != NILLNODE) {
	pgn = (GNode *) Lst_Datum (ln);

	if (pgn->type & OP_ARCHV) {
	    /*
	     * If the parent is an archive specification and is being made
	     * and its member's name matches the name of the node we were
	     * given, record the modification time of the parent in the
	     * child. We keep searching its parents in case some other
	     * parent requires this child to exist...
	     */
	    nameStart = index (pgn->name, '(') + 1;
	    nameEnd = index (nameStart, ')');

	    if (pgn->make &&
		strncmp(nameStart, gn->name, nameEnd - nameStart) == 0) {
				     gn->mtime = Arch_MTime(pgn);
	    }
	} else if (pgn->make) {
	    /*
	     * Something which isn't a library depends on the existence of
	     * this target, so it needs to exist.
	     */
	    gn->mtime = 0;
	    break;
	}
    }

    Lst_Close (gn->parents);

    return (gn->mtime);
}

/*-
 *-----------------------------------------------------------------------
 * Arch_FindLib --
 *	Search for a library along the given search path. 
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The node's 'path' field is set to the found path (including the
 *	actual file name, not -l...). If the system can handle the -L
 *	flag when linking (or we cannot find the library), we assume that
 *	the user has placed the .LIBRARIES variable in the final linking
 *	command (or the linker will know where to find it) and set the
 *	TARGET variable for this node to be the node's name. Otherwise,
 *	we set the TARGET variable to be the full path of the library,
 *	as returned by Dir_FindFile.
 *
 *-----------------------------------------------------------------------
 */
void
Arch_FindLib (gn, path)
    GNode	    *gn;	      /* Node of library to find */
    Lst	    	    path;	      /* Search path */
{
    char	    *libName;   /* file name for archive */

    libName = (char *)emalloc (strlen (gn->name) + 6 - 2);
    sprintf(libName, "lib%s.a", &gn->name[2]);

    gn->path = Dir_FindFile (libName, path);

    free (libName);

#ifdef LIBRARIES
    Var_Set (TARGET, gn->name, gn);
#else
    Var_Set (TARGET, gn->path == (char *) NULL ? gn->name : gn->path, gn);
#endif LIBRARIES
}

/*-
 *-----------------------------------------------------------------------
 * Arch_LibOODate --
 *	Decide if a node with the OP_LIB attribute is out-of-date. Called
 *	from Make_OODate to make its life easier.
 *
 *	There are several ways for a library to be out-of-date that are
 *	not available to ordinary files. In addition, there are ways
 *	that are open to regular files that are not available to
 *	libraries. A library that is only used as a source is never
 *	considered out-of-date by itself. This does not preclude the
 *	library's modification time from making its parent be out-of-date.
 *	A library will be considered out-of-date for any of these reasons,
 *	given that it is a target on a dependency line somewhere:
 *	    Its modification time is less than that of one of its
 *	    	  sources (gn->mtime < gn->cmtime).
 *	    Its modification time is greater than the time at which the
 *	    	  make began (i.e. it's been modified in the course
 *	    	  of the make, probably by archiving).
 *	    Its modification time doesn't agree with the modification
 *	    	  time of its RANLIBMAG member (i.e. its table of contents
 *	    	  is out-of-date).
 *
 *
 * Results:
 *	TRUE if the library is out-of-date. FALSE otherwise.
 *
 * Side Effects:
 *	The library will be hashed if it hasn't been already.
 *
 *-----------------------------------------------------------------------
 */
Boolean
Arch_LibOODate (gn)
    GNode   	  *gn;  	/* The library's graph node */
{
    Boolean 	  oodate;
    
    if (OP_NOP(gn->type) && Lst_IsEmpty(gn->children)) {
	oodate = FALSE;
    } else if ((gn->mtime > now) || (gn->mtime < gn->cmtime)) {
	oodate = TRUE;
    } else {
	ArchTOCTime(gn, &oodate);
    }
    return (oodate);
}

/*-
 *-----------------------------------------------------------------------
 * Arch_Init --
 *	Initialize things for this module.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The 'archives' list is initialized.
 *
 *-----------------------------------------------------------------------
 */
void
Arch_Init ()
{
    archives = Lst_Init (FALSE);
}
