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
 * $Log:	parse.c,v $
 * Revision 2.2  92/05/20  20:14:18  mrt
 * 	First checkin
 * 	[92/05/20  16:34:55  mrt]
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
static char sccsid[] = "@(#)parse.c	5.18 (Berkeley) 2/19/91";
#endif /* not lint */

/*-
 * parse.c --
 *	Functions to parse a makefile.
 *
 *	One function, Parse_Init, must be called before any functions
 *	in this module are used. After that, the function Parse_File is the
 *	main entry point and controls most of the other functions in this
 *	module.
 *
 *	Most important structures are kept in Lsts. Directories for
 *	the #include "..." function are kept in the 'parseIncPath' Lst, while
 *	those for the #include <...> are kept in the 'sysIncPath' Lst. The
 *	targets currently being defined are kept in the 'targets' Lst.
 *
 *	The variables 'fname' and 'lineno' are used to track the name
 *	of the current file and the line number in that file so that error
 *	messages can be more meaningful.
 *
 * Interface:
 *	Parse_Init	    	    Initialization function which must be
 *	    	  	    	    called before anything else in this module
 *	    	  	    	    is used.
 *
 *	Parse_File	    	    Function used to parse a makefile. It must
 *	    	  	    	    be given the name of the file, which should
 *	    	  	    	    already have been opened, and a function
 *	    	  	    	    to call to read a character from the file.
 *
 *	Parse_IsVar	    	    Returns TRUE if the given line is a
 *	    	  	    	    variable assignment. Used by MainParseArgs
 *	    	  	    	    to determine if an argument is a target
 *	    	  	    	    or a variable assignment. Used internally
 *	    	  	    	    for pretty much the same thing...
 *
 *	Parse_Error	    	    Function called when an error occurs in
 *	    	  	    	    parsing. Used by the variable and
 *	    	  	    	    conditional modules.
 *	Parse_MainName	    	    Returns a Lst of the main target to create.
 */

#include <varargs.h>
#include <stdio.h>
#include <ctype.h>
#include "make.h"
#include "buf.h"
#include "pathnames.h"

/*
 * These values are returned by ParseEOF to tell Parse_File whether to
 * CONTINUE parsing, i.e. it had only reached the end of an include file,
 * or if it's DONE.
 */
#define	CONTINUE	1
#define	DONE		0
static int 	    ParseEOF();

static Lst     	    targets;	/* targets we're working on */
static Boolean	    inLine;	/* true if currently in a dependency
				 * line or its commands */

static char    	    *fname;	/* name of current file (for errors) */
static int          lineno;	/* line number in current file */
static FILE   	    *curFILE; 	/* current makefile */

static int	    fatals = 0;

static GNode	    *mainNode;	/* The main target to create. This is the
				 * first target on the first dependency
				 * line in the first makefile */
/*
 * Definitions for handling #include specifications
 */
typedef struct IFile {
    char           *fname;	    /* name of previous file */
    int             lineno;	    /* saved line number */
    FILE *       F;		    /* the open stream */
}              	  IFile;

static Lst      includes;  	/* stack of IFiles generated by
				 * #includes */
Lst         	parseIncPath;	/* list of directories for "..." includes */
Lst         	sysIncPath;	/* list of directories for <...> includes */

/*-
 * specType contains the SPECial TYPE of the current target. It is
 * Not if the target is unspecial. If it *is* special, however, the children
 * are linked as children of the parent but not vice versa. This variable is
 * set in ParseDoDependency
 */
typedef enum {
    Begin,  	    /* .BEGIN */
    Default,	    /* .DEFAULT */
    End,    	    /* .END, .ERROR or .EXIT */
    Ignore,	    /* .IGNORE */
    Includes,	    /* .INCLUDES */
    Interrupt,	    /* .INTERRUPT */
    Libs,	    /* .LIBS */
    MFlags,	    /* .MFLAGS or .MAKEFLAGS */
    Main,	    /* .MAIN and we don't have anything user-specified to
		     * make */
    Not,	    /* Not special */
    NotParallel,    /* .NOTPARALLEL */
    Null,   	    /* .NULL */
    Order,  	    /* .ORDER */
    Path,	    /* .PATH */
    Precious,	    /* .PRECIOUS */
    Silent,	    /* .SILENT */
    Suffixes,	    /* .SUFFIXES */
    Attribute	    /* Generic attribute */
} ParseSpecial;

ParseSpecial specType;

/*
 * Predecessor node for handling .ORDER. Initialized to NILGNODE when .ORDER
 * seen, then set to each successive source on the line.
 */
static GNode	*predecessor;

/*
 * The parseKeywords table is searched using binary search when deciding
 * if a target or source is special. The 'spec' field is the ParseSpecial
 * type of the keyword ("Not" if the keyword isn't special as a target) while
 * the 'op' field is the operator to apply to the list of targets if the
 * keyword is used as a source ("0" if the keyword isn't special as a source)
 */
static struct {
    char    	  *name;    	/* Name of keyword */
    ParseSpecial  spec;	    	/* Type when used as a target */
    int	    	  op;	    	/* Operator when used as a source */
} parseKeywords[] = {
{ ".BEGIN", 	  Begin,    	0 },
{ ".DEFAULT",	  Default,  	0 },
{ ".OPTIONAL",	  Attribute,   	OP_OPTIONAL },
{ ".END",   	  End,	    	0 },
{ ".ERROR",   	  End,		0 },
{ ".EXIT",   	  End,		0 },
{ ".EXEC",	  Attribute,   	OP_EXEC },
{ ".IGNORE",	  Ignore,   	OP_IGNORE },
{ ".INCLUDES",	  Includes, 	0 },
{ ".INTERRUPT",	  Interrupt,	0 },
{ ".INVISIBLE",	  Attribute,   	OP_INVISIBLE },
{ ".JOIN",  	  Attribute,   	OP_JOIN },
{ ".LIBS",  	  Libs,	    	0 },
{ ".MAIN",	  Main,		0 },
{ ".MAKE",  	  Attribute,   	OP_MAKE },
{ ".MAKEFLAGS",	  MFlags,   	0 },
{ ".MFLAGS",	  MFlags,   	0 },
{ ".NOTMAIN",	  Attribute,   	OP_NOTMAIN },
{ ".NOTPARALLEL", NotParallel,	0 },
{ ".NULL",  	  Null,	    	0 },
{ ".ORDER", 	  Order,    	0 },
{ ".PATH",	  Path,		0 },
{ ".PRECIOUS",	  Precious, 	OP_PRECIOUS },
{ ".RECURSIVE",	  Attribute,	OP_MAKE },
{ ".SILENT",	  Silent,   	OP_SILENT },
{ ".SUFFIXES",	  Suffixes, 	0 },
{ ".USE",   	  Attribute,   	OP_USE },
};

int attributes;

/*-
 *----------------------------------------------------------------------
 * ParseFindKeyword --
 *	Look in the table of keywords for one matching the given string.
 *
 * Results:
 *	The index of the keyword, or -1 if it isn't there.
 *
 * Side Effects:
 *	None
 *----------------------------------------------------------------------
 */
static int
ParseFindKeyword (str)
    char	    *str;		/* String to find */
{
    register int    start,
		    end,
		    cur;
    register int    diff;
    
    start = 0;
    end = (sizeof(parseKeywords)/sizeof(parseKeywords[0])) - 1;

    do {
	cur = start + ((end - start) / 2);
	diff = strcmp (str, parseKeywords[cur].name);

	if (diff == 0) {
	    return (cur);
	} else if (diff < 0) {
	    end = cur - 1;
	} else {
	    start = cur + 1;
	}
    } while (start <= end);
    return (-1);
}

/*-
 * Parse_Error  --
 *	Error message abort function for parsing. Prints out the context
 *	of the error (line number and file) as well as the message with
 *	two optional arguments.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	"fatals" is incremented if the level is PARSE_FATAL.
 */
/* VARARGS */
void
Parse_Error(type, va_alist)
	int type;		/* Error type (PARSE_WARNING, PARSE_FATAL) */
	va_dcl
{
	va_list ap;
	char *fmt;

	(void)fprintf(stderr, "\"%s\", line %d: ", fname, lineno);
	if (type == PARSE_WARNING)
		(void)fprintf(stderr, "warning: ");
	va_start(ap);
	fmt = va_arg(ap, char *);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	(void)fflush(stderr);
	if (type == PARSE_FATAL)
		fatals += 1;
}

/*-
 *---------------------------------------------------------------------
 * ParseLinkSrc  --
 *	Link the parent node to its new child. Used in a Lst_ForEach by
 *	ParseDoDependency. If the specType isn't 'Not', the parent
 *	isn't linked as a parent of the child.
 *
 * Results:
 *	Always = 0
 *
 * Side Effects:
 *	New elements are added to the parents list of cgn and the
 *	children list of cgn. the unmade field of pgn is updated
 *	to reflect the additional child.
 *---------------------------------------------------------------------
 */
static int
ParseLinkSrc (pgn, cgn)
    GNode          *pgn;	/* The parent node */
    GNode          *cgn;	/* The child node */
{
    if (Lst_Member (pgn->children, (ClientData)cgn) == NILLNODE) {
	(void)Lst_AtEnd (pgn->children, (ClientData)cgn);
	if (specType == Not) {
	    (void)Lst_AtEnd (cgn->parents, (ClientData)pgn);
	}
	pgn->unmade += 1;
    }
    return (0);
}

/*-
 *---------------------------------------------------------------------
 * ParseDoOp  --
 *	Apply the parsed operator to the given target node. Used in a
 *	Lst_ForEach call by ParseDoDependency once all targets have
 *	been found and their operator parsed. If the previous and new
 *	operators are incompatible, a major error is taken.
 *
 * Results:
 *	Always 0
 *
 * Side Effects:
 *	The type field of the node is altered to reflect any new bits in
 *	the op.
 *---------------------------------------------------------------------
 */
static int
ParseDoOp (gn, op)
    GNode          *gn;		/* The node to which the operator is to be
				 * applied */
    int             op;		/* The operator to apply */
{
    /*
     * If the dependency mask of the operator and the node don't match and
     * the node has actually had an operator applied to it before, and
     * the operator actually has some dependency information in it, complain. 
     */
    if (((op & OP_OPMASK) != (gn->type & OP_OPMASK)) &&
	!OP_NOP(gn->type) && !OP_NOP(op))
    {
	Parse_Error (PARSE_FATAL, "Inconsistent operator for %s", gn->name);
	return (1);
    }

    if ((op == OP_DOUBLEDEP) && ((gn->type & OP_OPMASK) == OP_DOUBLEDEP)) {
	/*
	 * If the node was the object of a :: operator, we need to create a
	 * new instance of it for the children and commands on this dependency
	 * line. The new instance is placed on the 'cohorts' list of the
	 * initial one (note the initial one is not on its own cohorts list)
	 * and the new instance is linked to all parents of the initial
	 * instance.
	 */
	register GNode	*cohort;
	LstNode	    	ln;
			
	cohort = Targ_NewGN(gn->name);
	/*
	 * Duplicate links to parents so graph traversal is simple. Perhaps
	 * some type bits should be duplicated?
	 *
	 * Make the cohort invisible as well to avoid duplicating it into
	 * other variables. True, parents of this target won't tend to do
	 * anything with their local variables, but better safe than
	 * sorry.
	 */
	Lst_ForEach(gn->parents, ParseLinkSrc, (ClientData)cohort);
	cohort->type = OP_DOUBLEDEP|OP_INVISIBLE;
	(void)Lst_AtEnd(gn->cohorts, (ClientData)cohort);

	/*
	 * Replace the node in the targets list with the new copy
	 */
	ln = Lst_Member(targets, (ClientData)gn);
	Lst_Replace(ln, (ClientData)cohort);
	gn = cohort;
    }
    /*
     * We don't want to nuke any previous flags (whatever they were) so we
     * just OR the new operator into the old 
     */
    gn->type |= op;

    return (0);
}

/*-
 *---------------------------------------------------------------------
 * ParseDoSrc  --
 *	Given the name of a source, figure out if it is an attribute
 *	and apply it to the targets if it is. Else decide if there is
 *	some attribute which should be applied *to* the source because
 *	of some special target and apply it if so. Otherwise, make the
 *	source be a child of the targets in the list 'targets'
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Operator bits may be added to the list of targets or to the source.
 *	The targets may have a new source added to their lists of children.
 *---------------------------------------------------------------------
 */
static void
ParseDoSrc (tOp, src)
    int		tOp;	/* operator (if any) from special targets */
    char	*src;	/* name of the source to handle */
{
    int		op;	/* operator (if any) from special source */
    GNode	*gn;

    op = 0;
    if (*src == '.' && isupper (src[1])) {
	int keywd = ParseFindKeyword(src);
	if (keywd != -1) {
	    op = parseKeywords[keywd].op;
	}
    }
    if (op != 0) {
	Lst_ForEach (targets, ParseDoOp, (ClientData)op);
    } else if (specType == Main) {
	/*
	 * If we have noted the existence of a .MAIN, it means we need
	 * to add the sources of said target to the list of things
	 * to create. The string 'src' is likely to be free, so we
	 * must make a new copy of it. Note that this will only be
	 * invoked if the user didn't specify a target on the command
	 * line. This is to allow #ifmake's to succeed, or something...
	 */
	(void) Lst_AtEnd (create, (ClientData)strdup(src));
	/*
	 * Add the name to the .TARGETS variable as well, so the user can
	 * employ that, if desired.
	 */
	Var_Append(".TARGETS", src, VAR_GLOBAL);
    } else if (specType == Order) {
	/*
	 * Create proper predecessor/successor links between the previous
	 * source and the current one.
	 */
	gn = Targ_FindNode(src, TARG_CREATE);
	if (predecessor != NILGNODE) {
	    (void)Lst_AtEnd(predecessor->successors, (ClientData)gn);
	    (void)Lst_AtEnd(gn->preds, (ClientData)predecessor);
	}
	/*
	 * The current source now becomes the predecessor for the next one.
	 */
	predecessor = gn;
    } else {
	/*
	 * If the source is not an attribute, we need to find/create
	 * a node for it. After that we can apply any operator to it
	 * from a special target or link it to its parents, as
	 * appropriate.
	 *
	 * In the case of a source that was the object of a :: operator,
	 * the attribute is applied to all of its instances (as kept in
	 * the 'cohorts' list of the node) or all the cohorts are linked
	 * to all the targets.
	 */
	gn = Targ_FindNode (src, TARG_CREATE);
	if (tOp) {
	    gn->type |= tOp;
	} else {
	    Lst_ForEach (targets, ParseLinkSrc, (ClientData)gn);
	}
	if ((gn->type & OP_OPMASK) == OP_DOUBLEDEP) {
	    register GNode  	*cohort;
	    register LstNode	ln;

	    for (ln=Lst_First(gn->cohorts); ln != NILLNODE; ln = Lst_Succ(ln)){
		cohort = (GNode *)Lst_Datum(ln);
		if (tOp) {
		    cohort->type |= tOp;
		} else {
		    Lst_ForEach(targets, ParseLinkSrc, (ClientData)cohort);
		}
	    }
	}
    }
}

/*-
 *-----------------------------------------------------------------------
 * ParseFindMain --
 *	Find a real target in the list and set it to be the main one.
 *	Called by ParseDoDependency when a main target hasn't been found
 *	yet.
 *
 * Results:
 *	0 if main not found yet, 1 if it is.
 *
 * Side Effects:
 *	mainNode is changed and Targ_SetMain is called.
 *
 *-----------------------------------------------------------------------
 */
static int
ParseFindMain(gn)
    GNode   	  *gn;	    /* Node to examine */
{
    if ((gn->type & (OP_NOTMAIN|OP_USE|OP_EXEC|OP_TRANSFORM)) == 0) {
	mainNode = gn;
	Targ_SetMain(gn);
	return (1);
    } else {
	return (0);
    }
}

/*-
 *-----------------------------------------------------------------------
 * ParseAddDir --
 *	Front-end for Dir_AddDir to make sure Lst_ForEach keeps going
 *
 * Results:
 *	=== 0
 *
 * Side Effects:
 *	See Dir_AddDir.
 *
 *-----------------------------------------------------------------------
 */
static int
ParseAddDir(path, name)
    Lst	    path;
    char    *name;
{
    Dir_AddDir(path, name);
    return(0);
}

/*-
 *-----------------------------------------------------------------------
 * ParseClearPath --
 *	Front-end for Dir_ClearPath to make sure Lst_ForEach keeps going
 *
 * Results:
 *	=== 0
 *
 * Side Effects:
 *	See Dir_ClearPath
 *
 *-----------------------------------------------------------------------
 */
static int
ParseClearPath(path)
    Lst	    path;
{
    Dir_ClearPath(path);
    return(0);
}

/*-
 *---------------------------------------------------------------------
 * ParseDoDependency  --
 *	Parse the dependency line in line.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	The nodes of the sources are linked as children to the nodes of the
 *	targets. Some nodes may be created.
 *
 *	We parse a dependency line by first extracting words from the line and
 * finding nodes in the list of all targets with that name. This is done
 * until a character is encountered which is an operator character. Currently
 * these are only ! and :. At this point the operator is parsed and the
 * pointer into the line advanced until the first source is encountered.
 * 	The parsed operator is applied to each node in the 'targets' list,
 * which is where the nodes found for the targets are kept, by means of
 * the ParseDoOp function.
 *	The sources are read in much the same way as the targets were except
 * that now they are expanded using the wildcarding scheme of the C-Shell
 * and all instances of the resulting words in the list of all targets
 * are found. Each of the resulting nodes is then linked to each of the
 * targets as one of its children.
 *	Certain targets are handled specially. These are the ones detailed
 * by the specType variable.
 *	The storing of transformation rules is also taken care of here.
 * A target is recognized as a transformation rule by calling
 * Suff_IsTransform. If it is a transformation rule, its node is gotten
 * from the suffix module via Suff_AddTransform rather than the standard
 * Targ_FindNode in the target module.
 *---------------------------------------------------------------------
 */
static void
ParseDoDependency (line)
    char           *line;	/* the line to parse */
{
    register char  *cp;		/* our current position */
    register GNode *gn;		/* a general purpose temporary node */
    register int    op;		/* the operator on the line */
    char            savec;	/* a place to save a character */
    Lst    	    paths;   	/* List of search paths to alter when parsing
				 * a list of .PATH targets */
    int	    	    tOp;    	/* operator from special target */
    Lst	    	    sources;	/* list of source names after expansion */
    Lst 	    curTargs;	/* list of target names to be found and added
				 * to the targets list */

    tOp = 0;

    specType = Not;
    paths = (Lst)NULL;

    curTargs = Lst_Init(FALSE);
    
    do {
	for (cp = line;
	     *cp && !isspace (*cp) &&
	     (*cp != '!') && (*cp != ':') && (*cp != '(');
	     cp++)
	{
	    if (*cp == '$') {
		/*
		 * Must be a dynamic source (would have been expanded
		 * otherwise), so call the Var module to parse the puppy
		 * so we can safely advance beyond it...There should be
		 * no errors in this, as they would have been discovered
		 * in the initial Var_Subst and we wouldn't be here.
		 */
		cp = Var_Skip(cp, VAR_CMD, TRUE);
		cp--;
	    }
	    continue;
	}
	if (*cp == '(') {
	    /*
	     * Archives must be handled specially to make sure the OP_ARCHV
	     * flag is set in their 'type' field, for one thing, and because
	     * things like "archive(file1.o file2.o file3.o)" are permissible.
	     * Arch_ParseArchive will set 'line' to be the first non-blank
	     * after the archive-spec. It creates/finds nodes for the members
	     * and places them on the given list, returning SUCCESS if all
	     * went well and FAILURE if there was an error in the
	     * specification. On error, line should remain untouched.
	     */
	    if (Arch_ParseArchive (&line, targets, VAR_CMD) != SUCCESS) {
		Parse_Error (PARSE_FATAL,
			     "Error in archive specification: \"%s\"", line);
		return;
	    } else {
		continue;
	    }
	}
	savec = *cp;
	
	if (!*cp) {
	    /*
	     * Ending a dependency line without an operator is a Bozo
	     * no-no 
	     */
	    Parse_Error (PARSE_FATAL, "Need an operator");
	    return;
	}
	*cp = '\0';
	/*
	 * Have a word in line. See if it's a special target and set
	 * specType to match it.
	 */
	if (*line == '.' && isupper (line[1])) {
	    /*
	     * See if the target is a special target that must have it
	     * or its sources handled specially. 
	     */
	    int keywd = ParseFindKeyword(line);
	    if (keywd != -1) {
		if (specType == Path && parseKeywords[keywd].spec != Path) {
		    Parse_Error(PARSE_FATAL, "Mismatched special targets");
		    return;
		}
		
		specType = parseKeywords[keywd].spec;
		tOp = parseKeywords[keywd].op;

		/*
		 * Certain special targets have special semantics:
		 *	.PATH		Have to set the dirSearchPath
		 *			variable too
		 *	.MAIN		Its sources are only used if
		 *			nothing has been specified to
		 *			create.
		 *	.DEFAULT    	Need to create a node to hang
		 *			commands on, but we don't want
		 *			it in the graph, nor do we want
		 *			it to be the Main Target, so we
		 *			create it, set OP_NOTMAIN and
		 *			add it to the list, setting
		 *			DEFAULT to the new node for
		 *			later use. We claim the node is
		 *	    	    	A transformation rule to make
		 *	    	    	life easier later, when we'll
		 *	    	    	use Make_HandleUse to actually
		 *	    	    	apply the .DEFAULT commands.
		 *	.BEGIN
		 *	.END
		 *	.ERROR
		 *	.EXIT
		 *	.INTERRUPT  	Are not to be considered the
		 *			main target.
		 *  	.NOTPARALLEL	Make only one target at a time.
		 *  	.ORDER	    	Must set initial predecessor to NIL
		 */
		switch (specType) {
		    case Path:
			if (paths == NULL) {
			    paths = Lst_Init(FALSE);
			}
			(void)Lst_AtEnd(paths, (ClientData)dirSearchPath);
			break;
		    case Main:
			if (!Lst_IsEmpty(create)) {
			    specType = Not;
			}
			break;
		    case Begin:
		    case End:
		    case Interrupt:
			gn = Targ_FindNode(line, TARG_CREATE);
			gn->type |= OP_NOTMAIN;
			(void)Lst_AtEnd(targets, (ClientData)gn);
			break;
		    case Default:
			gn = Targ_NewGN(".DEFAULT");
			gn->type |= (OP_NOTMAIN|OP_TRANSFORM);
			(void)Lst_AtEnd(targets, (ClientData)gn);
			DEFAULT = gn;
			break;
		    case NotParallel:
		    {
			extern int  maxJobs;
			
			maxJobs = 1;
			break;
		    }
		    case Order:
			predecessor = NILGNODE;
			break;
		}
	    } else if (strncmp (line, ".PATH", 5) == 0) {
		/*
		 * .PATH<suffix> has to be handled specially.
		 * Call on the suffix module to give us a path to
		 * modify.
		 */
		Lst 	path;
		
		specType = Path;
		path = Suff_GetPath (&line[5]);
		if (path == NILLST) {
		    Parse_Error (PARSE_FATAL,
				 "Suffix '%s' not defined (yet)",
				 &line[5]);
		    return;
		} else {
		    if (paths == (Lst)NULL) {
			paths = Lst_Init(FALSE);
		    }
		    (void)Lst_AtEnd(paths, (ClientData)path);
		}
	    }
	}
	
	/*
	 * Have word in line. Get or create its node and stick it at
	 * the end of the targets list 
	 */
	if ((specType == Not) && (*line != '\0')) {
	    if (Dir_HasWildcards(line)) {
		/*
		 * Targets are to be sought only in the current directory,
		 * so create an empty path for the thing. Note we need to
		 * use Dir_Destroy in the destruction of the path as the
		 * Dir module could have added a directory to the path...
		 */
		Lst	    emptyPath = Lst_Init(FALSE);
		
		Dir_Expand(line, emptyPath, curTargs);
		
		Lst_Destroy(emptyPath, Dir_Destroy);
	    } else {
		/*
		 * No wildcards, but we want to avoid code duplication,
		 * so create a list with the word on it.
		 */
		(void)Lst_AtEnd(curTargs, (ClientData)line);
	    }
	    
	    while(!Lst_IsEmpty(curTargs)) {
		char	*targName = (char *)Lst_DeQueue(curTargs);
		
		if (!Suff_IsTransform (targName)) {
		    gn = Targ_FindNode (targName, TARG_CREATE);
		} else {
		    gn = Suff_AddTransform (targName);
		}
		
		(void)Lst_AtEnd (targets, (ClientData)gn);
	    }
	} else if (specType == Path && *line != '.' && *line != '\0') {
	    Parse_Error(PARSE_WARNING, "Extra target (%s) ignored", line);
	}
	
	*cp = savec;
	/*
	 * If it is a special type and not .PATH, it's the only target we
	 * allow on this line...
	 */
	if (specType != Not && specType != Path) {
	    Boolean warn = FALSE;
	    
	    while ((*cp != '!') && (*cp != ':') && *cp) {
		if (*cp != ' ' && *cp != '\t') {
		    warn = TRUE;
		}
		cp++;
	    }
	    if (warn) {
		Parse_Error(PARSE_WARNING, "Extra target ignored");
	    }
	} else {
	    while (*cp && isspace (*cp)) {
		cp++;
	    }
	}
	line = cp;
    } while ((*line != '!') && (*line != ':') && *line);

    /*
     * Don't need the list of target names anymore...
     */
    Lst_Destroy(curTargs, NOFREE);

    if (!Lst_IsEmpty(targets)) {
	switch(specType) {
	    default:
		Parse_Error(PARSE_WARNING, "Special and mundane targets don't mix. Mundane ones ignored");
		break;
	    case Default:
	    case Begin:
	    case End:
	    case Interrupt:
		/*
		 * These four create nodes on which to hang commands, so
		 * targets shouldn't be empty...
		 */
	    case Not:
		/*
		 * Nothing special here -- targets can be empty if it wants.
		 */
		break;
	}
    }

    /*
     * Have now parsed all the target names. Must parse the operator next. The
     * result is left in  op .
     */
    if (*cp == '!') {
	op = OP_FORCE;
    } else if (*cp == ':') {
	if (cp[1] == ':') {
	    op = OP_DOUBLEDEP;
	    cp++;
	} else {
	    op = OP_DEPENDS;
	}
    } else {
	Parse_Error (PARSE_FATAL, "Missing dependency operator");
	return;
    }

    cp++;			/* Advance beyond operator */

    Lst_ForEach (targets, ParseDoOp, (ClientData)op);

    /*
     * Get to the first source 
     */
    while (*cp && isspace (*cp)) {
	cp++;
    }
    line = cp;

    /*
     * Several special targets take different actions if present with no
     * sources:
     *	a .SUFFIXES line with no sources clears out all old suffixes
     *	a .PRECIOUS line makes all targets precious
     *	a .IGNORE line ignores errors for all targets
     *	a .SILENT line creates silence when making all targets
     *	a .PATH removes all directories from the search path(s).
     */
    if (!*line) {
	switch (specType) {
	    case Suffixes:
		Suff_ClearSuffixes ();
		break;
	    case Precious:
		allPrecious = TRUE;
		break;
	    case Ignore:
		ignoreErrors = TRUE;
		break;
	    case Silent:
		beSilent = TRUE;
		break;
	    case Path:
		Lst_ForEach(paths, ParseClearPath, (ClientData)NULL);
		break;
	}
    } else if (specType == MFlags) {
	/*
	 * Call on functions in main.c to deal with these arguments and
	 * set the initial character to a null-character so the loop to
	 * get sources won't get anything
	 */
	Main_ParseArgLine (line);
	*line = '\0';
    } else if (specType == NotParallel) {
	*line = '\0';
    }
    
    /*
     * NOW GO FOR THE SOURCES 
     */
    if ((specType == Suffixes) || (specType == Path) ||
	(specType == Includes) || (specType == Libs) ||
	(specType == Null))
    {
	while (*line) {
	    /*
	     * If the target was one that doesn't take files as its sources
	     * but takes something like suffixes, we take each
	     * space-separated word on the line as a something and deal
	     * with it accordingly.
	     *
	     * If the target was .SUFFIXES, we take each source as a
	     * suffix and add it to the list of suffixes maintained by the
	     * Suff module.
	     *
	     * If the target was a .PATH, we add the source as a directory
	     * to search on the search path.
	     *
	     * If it was .INCLUDES, the source is taken to be the suffix of
	     * files which will be #included and whose search path should
	     * be present in the .INCLUDES variable.
	     *
	     * If it was .LIBS, the source is taken to be the suffix of
	     * files which are considered libraries and whose search path
	     * should be present in the .LIBS variable.
	     *
	     * If it was .NULL, the source is the suffix to use when a file
	     * has no valid suffix.
	     */
	    char  savec;
	    while (*cp && !isspace (*cp)) {
		cp++;
	    }
	    savec = *cp;
	    *cp = '\0';
	    switch (specType) {
		case Suffixes:
		    Suff_AddSuffix (line);
		    break;
		case Path:
		    Lst_ForEach(paths, ParseAddDir, (ClientData)line);
		    break;
		case Includes:
		    Suff_AddInclude (line);
		    break;
		case Libs:
		    Suff_AddLib (line);
		    break;
		case Null:
		    Suff_SetNull (line);
		    break;
	    }
	    *cp = savec;
	    if (savec != '\0') {
		cp++;
	    }
	    while (*cp && isspace (*cp)) {
		cp++;
	    }
	    line = cp;
	}
	if (paths) {
	    Lst_Destroy(paths, NOFREE);
	}
    } else {
	while (*line) {
	    /*
	     * The targets take real sources, so we must beware of archive
	     * specifications (i.e. things with left parentheses in them)
	     * and handle them accordingly.
	     */
	    while (*cp && !isspace (*cp)) {
		if ((*cp == '(') && (cp > line) && (cp[-1] != '$')) {
		    /*
		     * Only stop for a left parenthesis if it isn't at the
		     * start of a word (that'll be for variable changes
		     * later) and isn't preceded by a dollar sign (a dynamic
		     * source).
		     */
		    break;
		} else {
		    cp++;
		}
	    }

	    if (*cp == '(') {
		GNode	  *gn;

		sources = Lst_Init (FALSE);
		if (Arch_ParseArchive (&line, sources, VAR_CMD) != SUCCESS) {
		    Parse_Error (PARSE_FATAL,
				 "Error in source archive spec \"%s\"", line);
		    return;
		}

		while (!Lst_IsEmpty (sources)) {
		    gn = (GNode *) Lst_DeQueue (sources);
		    ParseDoSrc (tOp, gn->name);
		}
		Lst_Destroy (sources, NOFREE);
		cp = line;
	    } else {
		if (*cp) {
		    *cp = '\0';
		    cp += 1;
		}

		ParseDoSrc (tOp, line);
	    }
	    while (*cp && isspace (*cp)) {
		cp++;
	    }
	    line = cp;
	}
    }
    
    if (mainNode == NILGNODE) {
	/*
	 * If we have yet to decide on a main target to make, in the
	 * absence of any user input, we want the first target on
	 * the first dependency line that is actually a real target
	 * (i.e. isn't a .USE or .EXEC rule) to be made.
	 */
	Lst_ForEach (targets, ParseFindMain, (ClientData)0);
    }

}

/*-
 *---------------------------------------------------------------------
 * Parse_IsVar  --
 *	Return TRUE if the passed line is a variable assignment. A variable
 *	assignment consists of a single word followed by optional whitespace
 *	followed by either a += or an = operator.
 *	This function is used both by the Parse_File function and main when
 *	parsing the command-line arguments.
 *
 * Results:
 *	TRUE if it is. FALSE if it ain't
 *
 * Side Effects:
 *	none
 *---------------------------------------------------------------------
 */
Boolean
Parse_IsVar (line)
    register char  *line;	/* the line to check */
{
    register Boolean wasSpace = FALSE;	/* set TRUE if found a space */
    register Boolean haveName = FALSE;	/* Set TRUE if have a variable name */

    /*
     * Skip to variable name
     */
    while ((*line == ' ') || (*line == '\t')) {
	line++;
    }

    while (*line != '=') {
	if (*line == '\0') {
	    /*
	     * end-of-line -- can't be a variable assignment.
	     */
	    return (FALSE);
	} else if ((*line == ' ') || (*line == '\t')) {
	    /*
	     * there can be as much white space as desired so long as there is
	     * only one word before the operator 
	     */
	    wasSpace = TRUE;
	} else if (wasSpace && haveName) {
	    /*
	     * Stop when an = operator is found.
	     */
	    if ((*line == '+') || (*line == ':') || (*line == '?') || 
		(*line == '!')) {
		break;    
	    }

	    /*
	     * This is the start of another word, so not assignment.
	     */
	    return (FALSE);
	} else if (*line == '$') {
	    /*
	     * skip over the imbedded variable
	     */
	    line = Var_Skip(line, VAR_CMD, TRUE);
	    line--;
	} else {
	    haveName = TRUE; 
	    wasSpace = FALSE;
	}
	line++;
    }

    /*
     * A final check: if we stopped on a +, ?, ! or :, the next character must
     * be an = or it ain't a valid assignment 
     */
    if (((*line == '+') ||
	 (*line == '?') ||
	 (*line == ':') ||
	 (*line == '!')) &&
	(line[1] != '='))
    {
	return (FALSE);
    } else {
	return (haveName);
    }
}

/*-
 *---------------------------------------------------------------------
 * Parse_DoVar  --
 *	Take the variable assignment in the passed line and do it in the
 *	global context.
 *
 *	Note: There is a lexical ambiguity with assignment modifier characters
 *	in variable names. This routine interprets the character before the =
 *	as a modifier. Therefore, an assignment like
 *	    C++=/usr/bin/CC
 *	is interpreted as "C+ +=" instead of "C++ =".
 *
 * Results:
 *	none
 *
 * Side Effects:
 *	the variable structure of the given variable name is altered in the
 *	global context.
 *---------------------------------------------------------------------
 */
void
Parse_DoVar (line, ctxt)
    char            *line;	/* a line guaranteed to be a variable
				 * assignment. This reduces error checks */
    GNode   	    *ctxt;    	/* Context in which to do the assignment */
{
    register char   *cp;	/* pointer into line */
    enum {
	VAR_SUBST, VAR_APPEND, VAR_SHELL, VAR_NORMAL
    }	    	    type;   	/* Type of assignment */
    char            *opc;	/* ptr to operator character to 
				 * null-terminate the variable name */
    char	    *linebuf;

    /*
     * Skip to variable name
     */
    while ((*line == ' ') || (*line == '\t')) {
	line++;
    }

    /*
     * Skip to operator character, nulling out whitespace as we go
     */
    linebuf = NULL;
    for (cp = line; *cp != '='; cp++) {
	if (*cp == '$') {
	    int		length, len;
	    Boolean	freeIt;
	    char	*result;
	    char	*old_linebuf;

	    result=Var_Parse(cp, VAR_CMD, TRUE, &length, &freeIt);
	    if (result == var_Error) {
		Parse_Error(PARSE_WARNING, "Bad imbedded variable");
		return;
	    }
	    len = strlen(result);
	    old_linebuf = linebuf;
	    linebuf = emalloc(strlen(line) + (len - length) + 1);
	    strncpy(linebuf, line, (cp - line));
	    strncpy(linebuf + (cp - line), result, len);
	    strcpy(linebuf + (cp - line) + len, cp + length);
	    cp = linebuf + (cp - line) + len;
	    line = linebuf;
	    if (freeIt)
		free(result);
	    if (old_linebuf != NULL)
		free(old_linebuf);
	    continue;
	}
	if (isspace (*cp)) {
	    *cp = '\0';
	}
    }
    opc = cp-1;		/* operator is the previous character */
    *cp++ = '\0';	/* nuke the = */

    /*
     * Check operator type
     */
    switch (*opc) {
	case '+':
	    type = VAR_APPEND;
	    *opc = '\0';
	    break;

	case '?':
	    /*
	     * If the variable already has a value, we don't do anything.
	     */
	    *opc = '\0';
	    if (Var_Exists(line, ctxt)) {
		return;
	    } else {
		type = VAR_NORMAL;
	    }
	    break;

	case ':':
	    type = VAR_SUBST;
	    *opc = '\0';
	    break;

	case '!':
	    type = VAR_SHELL;
	    *opc = '\0';
	    break;

	default:
	    type = VAR_NORMAL;
	    break;
    }

    while (isspace (*cp)) {
	cp++;
    }

    if (type == VAR_APPEND) {
	Var_Append (line, cp, ctxt);
    } else if (type == VAR_SUBST) {
	/*
	 * Allow variables in the old value to be undefined, but leave their
	 * invocation alone -- this is done by forcing oldVars to be false.
	 * XXX: This can cause recursive variables, but that's not hard to do,
	 * and this allows someone to do something like
	 *
	 *  CFLAGS = $(.INCLUDES)
	 *  CFLAGS := -I.. $(CFLAGS)
	 *
	 * And not get an error.
	 */
	Boolean	  oldOldVars = oldVars;

	oldVars = FALSE;
	cp = Var_Subst(cp, ctxt, FALSE);
	oldVars = oldOldVars;

	Var_Set(line, cp, ctxt);
	free(cp);
    } else if (type == VAR_SHELL) {
	char	result[BUFSIZ];	/* Result of command */
	char	*args[4];   	/* Args for invoking the shell */
	int 	fds[2];	    	/* Pipe streams */
	int 	cpid;	    	/* Child PID */
	int 	pid;	    	/* PID from wait() */
	Boolean	freeCmd;    	/* TRUE if the command needs to be freed, i.e.
				 * if any variable expansion was performed */

	/*
	 * Set up arguments for shell
	 */
	args[0] = "sh";
	args[1] = "-c";
	if (index(cp, '$') != (char *)NULL) {
	    /*
	     * There's a dollar sign in the command, so perform variable
	     * expansion on the whole thing. The resulting string will need
	     * freeing when we're done, so set freeCmd to TRUE.
	     */
	    args[2] = Var_Subst(cp, VAR_CMD, TRUE);
	    freeCmd = TRUE;
	} else {
	    args[2] = cp;
	    freeCmd = FALSE;
	}
	args[3] = (char *)NULL;

	/*
	 * Open a pipe for fetching its output
	 */
	pipe(fds);

	/*
	 * Fork
	 */
	cpid = vfork();
	if (cpid == 0) {
	    /*
	     * Close input side of pipe
	     */
	    close(fds[0]);

	    /*
	     * Duplicate the output stream to the shell's output, then
	     * shut the extra thing down. Note we don't fetch the error
	     * stream...why not? Why?
	     */
	    dup2(fds[1], 1);
	    close(fds[1]);
	    
	    execv("/bin/sh", args);
	    _exit(1);
	} else if (cpid < 0) {
	    /*
	     * Couldn't fork -- tell the user and make the variable null
	     */
	    Parse_Error(PARSE_WARNING, "Couldn't exec \"%s\"", cp);
	    Var_Set(line, "", ctxt);
	} else {
	    int	status;
	    int cc;

	    /*
	     * No need for the writing half
	     */
	    close(fds[1]);
	    
	    /*
	     * Wait for the process to exit.
	     *
	     * XXX: If the child writes more than a pipe's worth, we will
	     * deadlock.
	     */
	    while(((pid = wait(&status)) != cpid) && (pid >= 0)) {
		;
	    }

	    /*
	     * Read all the characters the child wrote.
	     */
	    cc = read(fds[0], result, sizeof(result));

	    if (cc < 0) {
		/*
		 * Couldn't read the child's output -- tell the user and
		 * set the variable to null
		 */
		Parse_Error(PARSE_WARNING, "Couldn't read shell's output");
		cc = 0;
	    }

	    if (status) {
		/*
		 * Child returned an error -- tell the user but still use
		 * the result.
		 */
		Parse_Error(PARSE_WARNING, "\"%s\" returned non-zero", cp);
	    }
	    /*
	     * Null-terminate the result, convert newlines to spaces and
	     * install it in the variable.
	     */
	    result[cc] = '\0';
	    cp = &result[cc] - 1;

	    if (*cp == '\n') {
		/*
		 * A final newline is just stripped
		 */
		*cp-- = '\0';
	    }
	    while (cp >= result) {
		if (*cp == '\n') {
		    *cp = ' ';
		}
		cp--;
	    }
	    Var_Set(line, result, ctxt);

	    /*
	     * Close the input side of the pipe.
	     */
	    close(fds[0]);
	}
	if (freeCmd) {
	    free(args[2]);
	}
    } else {
	/*
	 * Normal assignment -- just do it.
	 */
	Var_Set (line, cp, ctxt);
    }
    if (linebuf)
	free(linebuf);
}

/*-
 * ParseAddCmd  --
 *	Lst_ForEach function to add a command line to all targets
 *
 * Results:
 *	Always 0
 *
 * Side Effects:
 *	A new element is added to the commands list of the node.
 */
static
ParseAddCmd(gn, cmd)
	GNode *gn;	/* the node to which the command is to be added */
	char *cmd;	/* the command to add */
{
	/* if target already supplied, ignore commands */
	if (!(gn->type & OP_HAS_COMMANDS))
		(void)Lst_AtEnd(gn->commands, (ClientData)cmd);
	return(0);
}

/*-
 *-----------------------------------------------------------------------
 * ParseHasCommands --
 *	Callback procedure for Parse_File when destroying the list of
 *	targets on the last dependency line. Marks a target as already
 *	having commands if it does, to keep from having shell commands
 *	on multiple dependency lines.
 *
 * Results:
 *	Always 0.
 *
 * Side Effects:
 *	OP_HAS_COMMANDS may be set for the target.
 *
 *-----------------------------------------------------------------------
 */
static int
ParseHasCommands(gn)
    GNode   	  *gn;	    /* Node to examine */
{
    if (!Lst_IsEmpty(gn->commands)) {
	gn->type |= OP_HAS_COMMANDS;
    }
    return(0);
}

/*-
 *-----------------------------------------------------------------------
 * Parse_AddIncludeDir --
 *	Add a directory to the path searched for included makefiles
 *	bracketed by double-quotes. Used by functions in main.c
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The directory is appended to the list.
 *
 *-----------------------------------------------------------------------
 */
void
Parse_AddIncludeDir (dir)
    char    	  *dir;	    /* The name of the directory to add */
{
    Dir_AddDir (parseIncPath, dir);
}

/*-
 *---------------------------------------------------------------------
 * ParseDoInclude  --
 *	Push to another file.
 *	
 *	The input is the line minus the #include. A file spec is a string
 *	enclosed in <> or "". The former is looked for only in sysIncPath.
 *	The latter in . and the directories specified by -I command line
 *	options
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	A structure is added to the includes Lst and readProc, lineno,
 *	fname and curFILE are altered for the new file
 *---------------------------------------------------------------------
 */
static void
ParseDoInclude (file)
    char          *file;	/* file specification */
{
    char          *fullname;	/* full pathname of file */
    IFile         *oldFile;	/* state associated with current file */
    Lst           path;	    	/* the path to use to find the file */
    char          endc;	    	/* the character which ends the file spec */
    char          *cp;		/* current position in file spec */
    Boolean 	  isSystem; 	/* TRUE if makefile is a system makefile */

    /*
     * Skip to delimiter character so we know where to look
     */
    while ((*file == ' ') || (*file == '\t')) {
	file++;
    }

    if ((*file != '"') && (*file != '<')) {
	Parse_Error (PARSE_FATAL,
	    ".include filename must be delimited by '\"' or '<'");
	return;
    }

    /*
     * Set the search path on which to find the include file based on the
     * characters which bracket its name. Angle-brackets imply it's
     * a system Makefile while double-quotes imply it's a user makefile
     */
    if (*file == '<') {
	isSystem = TRUE;
	endc = '>';
    } else {
	isSystem = FALSE;
	endc = '"';
    }

    /*
     * Skip to matching delimiter
     */
    for (cp = ++file; *cp && *cp != endc; cp++) {
	continue;
    }

    if (*cp != endc) {
	Parse_Error (PARSE_FATAL,
		     "Unclosed .include filename. '%c' expected", endc);
	return;
    }
    *cp = '\0';

    /*
     * Substitute for any variables in the file name before trying to
     * find the thing.
     */
    file = Var_Subst (file, VAR_CMD, FALSE);

    /*
     * Now we know the file's name and its search path, we attempt to
     * find the durn thing. A return of NULL indicates the file don't
     * exist.
     */
    if (!isSystem) {
	/*
	 * Include files contained in double-quotes are first searched for
	 * relative to the including file's location. We don't want to
	 * cd there, of course, so we just tack on the old file's
	 * leading path components and call Dir_FindFile to see if
	 * we can locate the beast.
	 */
	char	  *prefEnd;

	prefEnd = rindex (fname, '/');
	if (prefEnd != (char *)NULL) {
	    char  	*newName;
	    
	    *prefEnd = '\0';
	    newName = str_concat (fname, file, STR_ADDSLASH);
	    fullname = Dir_FindFile (newName, parseIncPath);
	    if (fullname == (char *)NULL) {
		fullname = Dir_FindFile(newName, dirSearchPath);
	    }
	    free (newName);
	    *prefEnd = '/';
	} else {
	    fullname = (char *)NULL;
	}
    } else {
	fullname = (char *)NULL;
    }

    if (fullname == (char *)NULL) {
	/*
	 * System makefile or makefile wasn't found in same directory as
	 * included makefile. Search for it first on the -I search path,
	 * then on the .PATH search path, if not found in a -I directory.
	 * XXX: Suffix specific?
	 */
	fullname = Dir_FindFile (file, parseIncPath);
	if (fullname == (char *)NULL) {
	    fullname = Dir_FindFile(file, dirSearchPath);
	}
    }

    if (fullname == (char *)NULL) {
	/*
	 * Still haven't found the makefile. Look for it on the system
	 * path as a last resort.
	 */
	fullname = Dir_FindFile(file, sysIncPath);
    }

    if (fullname == (char *) NULL) {
	*cp = endc;
	Parse_Error (PARSE_FATAL, "Could not find %s", file);
	return;
    }

    /*
     * Once we find the absolute path to the file, we get to save all the
     * state from the current file before we can start reading this
     * include file. The state is stored in an IFile structure which
     * is placed on a list with other IFile structures. The list makes
     * a very nice stack to track how we got here...
     */
    oldFile = (IFile *) emalloc (sizeof (IFile));
    oldFile->fname = fname;

    oldFile->F = curFILE;
    oldFile->lineno = lineno;

    (void) Lst_AtFront (includes, (ClientData)oldFile);

    /*
     * Once the previous state has been saved, we can get down to reading
     * the new file. We set up the name of the file to be the absolute
     * name of the include file so error messages refer to the right
     * place. Naturally enough, we start reading at line number 0.
     */
    fname = fullname;
    lineno = 0;

    curFILE = fopen (fullname, "r");
    if (curFILE == (FILE * ) NULL) {
	Parse_Error (PARSE_FATAL, "Cannot open %s", fullname);
	/*
	 * Pop to previous file
	 */
	(void) ParseEOF(0);
    }
}

/*-
 *---------------------------------------------------------------------
 * ParseEOF  --
 *	Called when EOF is reached in the current file. If we were reading
 *	an include file, the includes stack is popped and things set up
 *	to go back to reading the previous file at the previous location.
 *
 * Results:
 *	CONTINUE if there's more to do. DONE if not.
 *
 * Side Effects:
 *	The old curFILE, is closed. The includes list is shortened.
 *	lineno, curFILE, and fname are changed if CONTINUE is returned.
 *---------------------------------------------------------------------
 */
static int
ParseEOF (opened)
    int opened;
{
    IFile     *ifile;	/* the state on the top of the includes stack */

    if (Lst_IsEmpty (includes)) {
	return (DONE);
    }

    ifile = (IFile *) Lst_DeQueue (includes);
    free (fname);
    fname = ifile->fname;
    lineno = ifile->lineno;
    if (opened)
	(void) fclose (curFILE);
    curFILE = ifile->F;
    free ((Address)ifile);
    return (CONTINUE);
}

/*-
 *---------------------------------------------------------------------
 * ParseReadc  --
 *	Read a character from the current file and update the line number
 *	counter as necessary
 *
 * Results:
 *	The character that was read
 *
 * Side Effects:
 *	The lineno counter is incremented if the character is a newline
 *---------------------------------------------------------------------
 */
#ifdef notdef
static int parseReadChar;

#define ParseReadc() (((parseReadChar = getc(curFILE)) == '\n') ? \
		      (lineno++, '\n') : parseReadChar)
#else
#define ParseReadc() (getc(curFILE))
#endif /* notdef */


/*-
 *---------------------------------------------------------------------
 * ParseReadLine --
 *	Read an entire line from the input file. Called only by Parse_File.
 *	To facilitate escaped newlines and what have you, a character is
 *	buffered in 'lastc', which is '\0' when no characters have been
 *	read. When we break out of the loop, c holds the terminating
 *	character and lastc holds a character that should be added to
 *	the line (unless we don't read anything but a terminator).
 *
 * Results:
 *	A line w/o its newline
 *
 * Side Effects:
 *	Only those associated with reading a character
 *---------------------------------------------------------------------
 */
static char *
ParseReadLine ()
{
    Buffer  	  buf;	    	/* Buffer for current line */
    register int  c;	      	/* the current character */
    register int  lastc;    	/* The most-recent character */
    Boolean	  semiNL;     	/* treat semi-colons as newlines */
    Boolean	  ignDepOp;   	/* TRUE if should ignore dependency operators
				 * for the purposes of setting semiNL */
    Boolean 	  ignComment;	/* TRUE if should ignore comments (in a
				 * shell command */
    char    	  *line;    	/* Result */
    int	    	  lineLength;	/* Length of result */

    semiNL = FALSE;
    ignDepOp = FALSE;
    ignComment = FALSE;

    /*
     * Handle special-characters at the beginning of the line. Either a
     * leading tab (shell command) or pound-sign (possible conditional)
     * forces us to ignore comments and dependency operators and treat
     * semi-colons as semi-colons (by leaving semiNL FALSE). This also
     * discards completely blank lines.
     */
    while(1) {
	c = ParseReadc();

	if (c == '\t') {
	    ignComment = ignDepOp = TRUE;
	    break;
	} else if (c == '.') {
	    ignComment = TRUE;
	    break;
	} else if (c == '\n') {
	    lineno++;
	} else if (c == '#') {
		ungetc(c, curFILE); 
		break;
	} else {
	    /*
	     * Anything else breaks out without doing anything
	     */
	    break;
	}
    }
	
    if (c != EOF) {
	lastc = c;
	buf = Buf_Init(0);
	
	while (((c = ParseReadc ()) != '\n' || (lastc == '\\')) &&
	       (c != EOF))
	{
test_char:
	    switch(c) {
	    case '\n':
		/*
		 * Escaped newline: read characters until a non-space or an
		 * unescaped newline and replace them all by a single space.
		 * This is done by storing the space over the backslash and
		 * dropping through with the next nonspace. If it is a
		 * semi-colon and semiNL is TRUE, it will be recognized as a
		 * newline in the code below this...
		 */
		lineno++;
		lastc = ' ';
		while ((c = ParseReadc ()) == ' ' || c == '\t') {
		    continue;
		}
		if (c == EOF || c == '\n') {
		    lineno++;
		    goto line_read;
		} else {
		    /*
		     * Check for comments, semiNL's, etc. -- easier than
		     * ungetc(c, curFILE); continue;
		     */
		    goto test_char;
		}
		break;
	    case ';':
		/*
		 * Semi-colon: Need to see if it should be interpreted as a
		 * newline
		 */
		if (semiNL) {
		    /*
		     * To make sure the command that may be following this
		     * semi-colon begins with a tab, we push one back into the
		     * input stream. This will overwrite the semi-colon in the
		     * buffer. If there is no command following, this does no
		     * harm, since the newline remains in the buffer and the
		     * whole line is ignored.
		     */
		    ungetc('\t', curFILE);
		    goto line_read;
		} 
		break;
	    case '=':
		if (!semiNL) {
		    /*
		     * Haven't seen a dependency operator before this, so this
		     * must be a variable assignment -- don't pay attention to
		     * dependency operators after this.
		     */
		    ignDepOp = TRUE;
		} else if (lastc == ':' || lastc == '!') {
		    /*
		     * Well, we've seen a dependency operator already, but it
		     * was the previous character, so this is really just an
		     * expanded variable assignment. Revert semi-colons to
		     * being just semi-colons again and ignore any more
		     * dependency operators.
		     *
		     * XXX: Note that a line like "foo : a:=b" will blow up,
		     * but who'd write a line like that anyway?
		     */
		    ignDepOp = TRUE; semiNL = FALSE;
		}
		break;
	    case '#':
		if (!ignComment) {
			/*
			 * If the character is a hash mark, this is a comment.
			 * Skip to the end of the line.
			 */
			do {
			    c = ParseReadc();
			} while ((c != '\n') && (c != EOF));
			lineno++;
			goto line_read;
		}
		break;
	    case ':':
	    case '!':
		if (!ignDepOp) {
		    /*
		     * A semi-colon is recognized as a newline only on
		     * dependency lines. Dependency lines are lines with a
		     * colon or an exclamation point. Ergo...
		     */
		    semiNL = TRUE;
		}
		break;
	    }
	    /*
	     * Copy in the previous character and save this one in lastc.
	     */
	    Buf_AddByte (buf, (Byte)lastc);
	    lastc = c;
	    
	}
	lineno++;
    line_read:
	
	if (lastc != '\0') {
	    Buf_AddByte (buf, (Byte)lastc);
	}
	Buf_AddByte (buf, (Byte)'\0');
	line = strdup((char *)Buf_GetAll (buf, &lineLength));
	Buf_Discard (buf, Buf_Size(buf));
	
	if (line[0] == '.') {
	    /*
	     * The line might be a conditional. Ask the conditional module
	     * about it and act accordingly
	     */
	    switch (Cond_Eval (line)) {
	    case COND_SKIP:
		do {
		    /*
		     * Skip to next conditional that evaluates to COND_PARSE.
		     */
		    free (line);
		    c = ParseReadc();
		    /*
		     * Skip lines until get to one that begins with a
		     * special char.
		     */
		    while ((c != '.') && (c != EOF)) {
			while (((c != '\n') || (lastc == '\\')) &&
			       (c != EOF))
			{
			    /*
			     * Advance to next unescaped newline
			     */
			    if ((lastc = c) == '\n') {
				lineno++;
			    }
			    c = ParseReadc();
			}
			lineno++;
			
			lastc = c;
			c = ParseReadc ();
		    }
		    
		    if (c == EOF) {
			Parse_Error (PARSE_FATAL, "Unclosed conditional");
			free (line);
			Buf_Destroy (buf);
			return ((char *)NULL);
		    }
		    
		    /*
		     * Read the entire line into buf
		     */
		    do {
			Buf_AddByte (buf, (Byte)c);
			c = ParseReadc();
		    } while ((c != '\n') && (c != EOF));
		    lineno++;
		    
		    Buf_AddByte (buf, (Byte)'\0');
		    line = strdup((char *)Buf_GetAll (buf, &lineLength));
		    Buf_Discard (buf, Buf_Size(buf));
		} while (Cond_Eval(line) != COND_PARSE);
		/*FALLTHRU*/
	    case COND_PARSE:
		free (line);
		line = ParseReadLine();
		break;
	    }
	}
	
	Buf_Destroy (buf);
	return (line);
    } else {
	/*
	 * Hit end-of-file, so return a NULL line to indicate this.
	 */
	return((char *)NULL);
    }
}

/*-
 *-----------------------------------------------------------------------
 * ParseFinishLine --
 *	Handle the end of a dependency group.
 *
 * Results:
 *	Nothing.
 *
 * Side Effects:
 *	inLine set FALSE. 'targets' list destroyed.
 *
 *-----------------------------------------------------------------------
 */
static void
ParseFinishLine()
{
    extern int Suff_EndTransform();

    if (inLine) {
	Lst_ForEach(targets, Suff_EndTransform, (ClientData)NULL);
	Lst_Destroy (targets, ParseHasCommands);
	inLine = FALSE;
    }
}
		    

/*-
 *---------------------------------------------------------------------
 * Parse_File --
 *	Parse a file into its component parts, incorporating it into the
 *	current dependency graph. This is the main function and controls
 *	almost every other function in this module
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Loads. Nodes are added to the list of all targets, nodes and links
 *	are added to the dependency graph. etc. etc. etc.
 *---------------------------------------------------------------------
 */
void
Parse_File(name, stream)
    char          *name;	/* the name of the file being read */
    FILE *	  stream;   	/* Stream open to makefile to parse */
{
    register char *cp,		/* pointer into the line */
                  *line;	/* the line we're working on */

    inLine = FALSE;
    fname = name;
    curFILE = stream;
    lineno = 0;
    fatals = 0;

    do {
	while (line = ParseReadLine ()) {
	    if (*line == '.') {
		/*
		 * Lines that begin with the special character are either
		 * include or undef directives.
		 */
		for (cp = line + 1; isspace (*cp); cp++) {
		    continue;
		}
		if (strncmp (cp, "include", 7) == 0) {
		    ParseDoInclude (cp + 7);
		    goto nextLine;
		} else if (strncmp(cp, "undef", 5) == 0) {
		    char *cp2;
		    for (cp += 5; isspace(*cp); cp++) {
			continue;
		    }

		    for (cp2 = cp; !isspace(*cp2) && (*cp2 != '\0'); cp2++) {
			continue;
		    }

		    *cp2 = '\0';

		    Var_Delete(cp, VAR_GLOBAL);
		    goto nextLine;
		}
	    }
	    if (*line == '#') {
		/* If we're this far, the line must be a comment. */
		goto nextLine;
	    }
	    
	    if (*line == '\t')
	    {
		/*
		 * If a line starts with a tab, it
		 * can only hope to be a creation command.
		 */
	    shellCommand:
		for (cp = line + 1; isspace (*cp); cp++) {
		    continue;
		}
		if (*cp) {
		    if (inLine) {
			/*
			 * So long as it's not a blank line and we're actually
			 * in a dependency spec, add the command to the list of
			 * commands of all targets in the dependency spec 
			 */
			Lst_ForEach (targets, ParseAddCmd, (ClientData)cp);
			continue;
		    } else {
			Parse_Error (PARSE_FATAL,
				     "Unassociated shell command \"%.20s\"",
				     cp);
		    }
		}
	    } else if (Parse_IsVar (line)) {
		ParseFinishLine();
		Parse_DoVar (line, VAR_GLOBAL);
	    } else {
		/*
		 * We now know it's a dependency line so it needs to have all
		 * variables expanded before being parsed. Tell the variable
		 * module to complain if some variable is undefined...
		 * To make life easier on novices, if the line is indented we
		 * first make sure the line has a dependency operator in it.
		 * If it doesn't have an operator and we're in a dependency
		 * line's script, we assume it's actually a shell command
		 * and add it to the current list of targets.
		 */
		Boolean	nonSpace = FALSE;
		
		cp = line;
		if (line[0] == ' ') {
		    while ((*cp != ':') && (*cp != '!') && (*cp != '\0')) {
			if (!isspace(*cp)) {
			    nonSpace = TRUE;
			}
			cp++;
		    }
		}
		    
		if (*cp == '\0') {
		    if (inLine) {
			Parse_Error (PARSE_WARNING,
				     "Shell command needs a leading tab");
			goto shellCommand;
		    } else if (nonSpace) {
			Parse_Error (PARSE_FATAL, "Missing operator");
		    }
		} else {
		    ParseFinishLine();

		    cp = Var_Subst (line, VAR_CMD, TRUE);
		    free (line);
		    line = cp;
		    
		    /*
		     * Need a non-circular list for the target nodes 
		     */
		    targets = Lst_Init (FALSE);
		    inLine = TRUE;
		    
		    ParseDoDependency (line);
		}
	    }

	    nextLine:

	    free (line);
	}
	/*
	 * Reached EOF, but it may be just EOF of an include file... 
	 */
    } while (ParseEOF(1) == CONTINUE);

    /*
     * Make sure conditionals are clean
     */
    Cond_End();

    if (fatals) {
	fprintf (stderr, "Fatal errors encountered -- cannot continue\n");
	exit (1);
    }
}

/*-
 *---------------------------------------------------------------------
 * Parse_Init --
 *	initialize the parsing module
 *
 * Results:
 *	none
 *
 * Side Effects:
 *	the parseIncPath list is initialized...
 *---------------------------------------------------------------------
 */
Parse_Init ()
{
	char *cp, *start;
	char *syspath, *getenv();
    
    mainNode = NILGNODE;
    parseIncPath = Lst_Init (FALSE);
    sysIncPath = Lst_Init (FALSE);
    includes = Lst_Init (FALSE);

    if ((syspath = getenv("MAKESYSPATH")) == NULL)
	syspath = _PATH_DEFSYSPATH;
    syspath = strdup(syspath);
    if (syspath == NULL)
	enomem();

    /*
     * Add the directories from the syspath (more than one may be given
     * as dir1:...:dirn) to the system include path.
     */
    for (start = syspath; *start != '\0'; start = cp) {
	for (cp = start; *cp != '\0' && *cp != ':'; cp++) {
	    ;
	}
	if (*cp == '\0') {
	    Dir_AddDir(sysIncPath, start);
	} else {
	    *cp++ = '\0';
	    Dir_AddDir(sysIncPath, start);
	}
    }
    free(syspath);
}

/*-
 *-----------------------------------------------------------------------
 * Parse_MainName --
 *	Return a Lst of the main target to create for main()'s sake. If
 *	no such target exists, we Punt with an obnoxious error message.
 *
 * Results:
 *	A Lst of the single node to create.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
Lst
Parse_MainName()
{
    Lst           main;	/* result list */

    main = Lst_Init (FALSE);

    if (mainNode == NILGNODE) {
	Punt ("make: no target to make.\n");
    	/*NOTREACHED*/
    } else if (mainNode->type & OP_DOUBLEDEP) {
	Lst_Concat(main, mainNode->cohorts, LST_CONCNEW);
    }
    (void) Lst_AtEnd (main, (ClientData)mainNode);
    return (main);
}
