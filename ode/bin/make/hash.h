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
 * $Log:	hash.h,v $
 * Revision 2.2  92/05/20  20:12:41  mrt
 * 	First checkin
 * 	[92/05/20  16:30:08  mrt]
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
 *
 *	@(#)hash.h	5.4 (Berkeley) 12/28/90
 */

/* hash.h --
 *
 * 	This file contains definitions used by the hash module,
 * 	which maintains hash tables.
 */

#ifndef	_HASH
#define	_HASH

/* 
 * The following defines one entry in the hash table.
 */

typedef struct Hash_Entry {
    struct Hash_Entry *next;		/* Used to link together all the
    					 * entries associated with the same
					 * bucket. */
    ClientData	      clientData;	/* Arbitrary piece of data associated
    					 * with key. */
    unsigned	      namehash;		/* hash value of key */
    char	      name[1];		/* key string */
} Hash_Entry;

typedef struct Hash_Table {
    struct Hash_Entry **bucketPtr;/* Pointers to Hash_Entry, one
    				 * for each bucket in the table. */
    int 	size;		/* Actual size of array. */
    int 	numEntries;	/* Number of entries in the table. */
    int 	mask;		/* Used to select bits for hashing. */
} Hash_Table;

/* 
 * The following structure is used by the searching routines
 * to record where we are in the search.
 */

typedef struct Hash_Search {
    Hash_Table  *tablePtr;	/* Table being searched. */
    int 	nextIndex;	/* Next bucket to check (after current). */
    Hash_Entry 	*hashEntryPtr;	/* Next entry to check in current bucket. */
} Hash_Search;

/*
 * Macros.
 */

/*
 * ClientData Hash_GetValue(h) 
 *     Hash_Entry *h; 
 */

#define Hash_GetValue(h) ((h)->clientData)

/* 
 * Hash_SetValue(h, val); 
 *     Hash_Entry *h; 
 *     char *val; 
 */

#define Hash_SetValue(h, val) ((h)->clientData = (ClientData) (val))

/* 
 * Hash_Size(n) returns the number of words in an object of n bytes 
 */

#define	Hash_Size(n)	(((n) + sizeof (int) - 1) / sizeof (int))

/*
 * The following procedure declarations and macros
 * are the only things that should be needed outside
 * the implementation code.
 */

extern Hash_Entry *	Hash_CreateEntry();
extern void		Hash_DeleteTable();
extern void		Hash_DeleteEntry();
extern void		Hash_DeleteTable();
extern Hash_Entry *	Hash_EnumFirst();
extern Hash_Entry *	Hash_EnumNext();
extern Hash_Entry *	Hash_FindEntry();
extern void		Hash_InitTable();
extern void		Hash_PrintStats();

#endif _HASH
