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
 * $Log:	release_db.h,v $
 * Revision 2.2  92/05/20  20:15:43  mrt
 * 	First checkin
 * 	[92/05/20  18:07:59  mrt]
 * 
 */
/* @(#)$RCSfile: release_db.h,v $ $Revision: 2.2 $ (OSF) $Date: 92/05/20 20:15:43 $ */

#if	USE_REFERENCE_COUNTING
#define	DECL_REFCNT	u_short refcnt;
#else	/* USE_REFERENCE_COUNTING */
#define	DECL_REFCNT
#endif	/* USE_REFERENCE_COUNTING */

/* table of pointers */
struct table {
    u_short cur;
    u_short max;
    union {
	char **u_tab;
	struct string **u_stab;
	struct file_info **u_fitab;
	struct r_file **u_ftab;
	struct dir **u_dtab;
    } tab_u;
#define stab	tab_u.u_stab
#define fitab	tab_u.u_fitab
#define ftab	tab_u.u_ftab
#define dtab	tab_u.u_dtab
};

/* string header */
struct strhdr {
    struct strhdr *sh_first;
#define sh_next sh_first
    struct strhdr *sh_last;
#define sh_prev sh_last
};

/* string structure */
struct string {
    struct strhdr sh;		/* string header */
    u_short index;		/* index */
    DECL_REFCNT			/* reference count */
    u_short len;		/* length of string */
    u_char ahash;		/* add hash value */
    u_char xhash;		/* xor hash value */
    char *data;			/* contents */
};

struct loginfo {
    DECL_REFCNT			/* reference count */
    struct string *installer;	/* installer */
    struct string *message;	/* message */
    time_t itime;		/* installation time */
    time_t wtime;		/* when to warn about release status */
};

struct instance {
    struct instance *next;	/* next instance of this file */
    u_short index;		/* index for this instance */
    DECL_REFCNT			/* reference count */
    struct loginfo *loginfo;	/* log information */
    u_short flags;		/* flags */
#define	IF_COMPRESSED	1	/* file is compressed */
    time_t ctime;		/* last time "inode" changed */
    time_t mtime;		/* last time file data modified */
};

struct file_info {
    u_short index;		/* index for this file info */
    DECL_REFCNT			/* reference count */
    struct string *owner;	/* owner name */
    struct string *group;	/* group name */
    u_short mode;		/* file permission mode */
};

struct r_file {
    u_short index;		/* index for this file */
    DECL_REFCNT			/* reference count */
    struct dir *parent;		/* directory that contains this file */
    struct r_file *links;	/* other links to this file */
    struct dir *srcdir;		/* directory to build this file */
    struct string *name;	/* file name (shared) */
    struct file_info *info;	/* file info (shared) */
    struct instance *instances;	/* instances of file for distribution */
};

struct dir {
    u_short index;		/* index for this directory */
    DECL_REFCNT			/* reference count */
    struct dir *parent;		/* directory that contains this directory */
    struct string *name;	/* directory name (shared) */
    struct table ftable;	/* table of files in directory */
    struct table dtable;	/* table of subdirectories in directory */
    struct file_info *info;	/* directory info (shared) */
};
