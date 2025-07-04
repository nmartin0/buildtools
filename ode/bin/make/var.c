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
 * $Log:	var.c,v $
 * Revision 2.3  93/03/20  00:05:12  mrt
 * 	Added include of stdlib.h for realloc.
 * 	[93/03/08            mrt]
 * 
 * Revision 2.2  92/05/20  20:15:04  mrt
 * 	First checkin
 * 	[92/05/20  16:36:12  mrt]
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
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)var.c	5.7 (Berkeley) 6/1/90";
#endif /* not lint */

/*-
 * var.c --
 *	Variable-handling functions
 *
 * Interface:
 *	Var_Set	  	    Set the value of a variable in the given
 *	    	  	    context. The variable is created if it doesn't
 *	    	  	    yet exist. The value and variable name need not
 *	    	  	    be preserved.
 *
 *	Var_Append	    Append more characters to an existing variable
 *	    	  	    in the given context. The variable needn't
 *	    	  	    exist already -- it will be created if it doesn't.
 *	    	  	    A space is placed between the old value and the
 *	    	  	    new one.
 *
 *	Var_Exists	    See if a variable exists.
 *
 *	Var_Value 	    Return the value of a variable in a context or
 *	    	  	    NULL if the variable is undefined.
 *
 *	Var_Subst 	    Substitute for all variables in a string using
 *	    	  	    the given context as the top-most one. If the
 *	    	  	    third argument is non-zero, Parse_Error is
 *	    	  	    called if any variables are undefined.
 *
 *	Var_Parse 	    Parse a variable expansion from a string and
 *	    	  	    return the result and the number of characters
 *	    	  	    consumed.
 *
 *	Var_Delete	    Delete a variable in a context.
 *
 *	Var_Init  	    Initialize this module.
 *
 * Debugging:
 *	Var_Dump  	    Print out all variables defined in the given
 *	    	  	    context.
 *
 * XXX: There's a lot of duplication in these functions.
 */

#include    <sys/types.h>
#include    <sys/time.h>
#include    <sys/wait.h>
#include    <stdio.h>
#include    <ctype.h>
#include    <stdlib.h>
#include    "make.h"
#include    "hash.h"
#include    "buf.h"

#define STATIC	/* static		/* debugging support */

#ifndef FD_SETSIZE
#define FD_SETSIZE      256
#endif

/*
 * This is a harmless return value for Var_Parse that can be used by Var_Subst
 * to determine if there was an error in parsing -- easier than returning
 * a flag, as things outside this module don't give a hoot.
 */
char 	var_Error[] = "";

/*
 * Similar to var_Error, but returned when the 'err' flag for Var_Parse is
 * set false. Why not just use a constant? Well, gcc likes to condense
 * identical string instances...
 */
char	varNoError[] = "";

/*
 * Internally, variables are contained in four different contexts.
 *	1) the environment. They may not be changed. If an environment
 *	    variable is appended-to, the result is placed in the global
 *	    context.
 *	2) the global context. Variables set in the Makefile are located in
 *	    the global context. It is the penultimate context searched when
 *	    substituting.
 *	3) the command-line context. All variables set on the command line
 *	   are placed in this context. They are UNALTERABLE once placed here.
 *	4) the local context. Each target has associated with it a context
 *	   list. On this list are located the structures describing such
 *	   local variables as $(@) and $(*)
 * The four contexts are searched in the reverse order from which they are
 * listed.
 */
GNode          *VAR_GLOBAL;   /* variables from the makefile */
GNode          *VAR_CMD;      /* variables defined on the command-line */
GNode          *VAR_ENV;      /* variables defined in the environment */

Hash_Table     globalHashTable;
Hash_Table     cmdHashTable;
Hash_Table     envHashTable;

#define FIND_CMD	0x1   /* look in VAR_CMD when searching */
#define FIND_GLOBAL	0x2   /* look in VAR_GLOBAL as well */
#define FIND_ENV  	0x4   /* look in the environment also */

typedef struct Var {
    char          *name;	/* the variable's name */
    Buffer	  val;	    	/* its value */
    int	    	  flags;    	/* miscellaneous status flags */
#define VAR_IN_USE	1   	    /* Variable's value currently being used.
				     * Used to avoid recursion */
#define VAR_FROM_ENV	2   	    /* Variable comes from the environment */
#define VAR_JUNK  	4   	    /* Variable is a junk variable that
				     * should be destroyed when done with
				     * it. Used by Var_Parse for undefined,
				     * modified variables */
#define VAR_KEEP	8	    /* Variable is VAR_JUNK, but we found
				     * a use for it in some modifier and
				     * the value is therefore valid */
}  Var;

STATIC char *VarParse();

/*-
 *-----------------------------------------------------------------------
 * VarRunShellCmd  --
 *	Run the command and return its value.
 *
 * Results:
 *	none
 *
 * Side Effects:
 *	none
 *-----------------------------------------------------------------------
 */
STATIC char *
VarRunShellCmd(cmd)
    char        *cmd;		/* command to execute */
{
    char	result[BUFSIZ];	/* Result of command */
    char	*args[5];   	/* Args for invoking the shell */
    char	**av;
    int 	fds[2];	    	/* Pipe streams */
    int 	cpid;	    	/* Child PID */
    int 	pid;	    	/* PID from waitpid() */
    int		status;
    int		cc;
    Buffer	buf;
    char	*str;
    char	*cp;

    buf = Buf_Init(0);

    if (Var_HasMeta(cmd)) {
	/*
	 * Set up arguments for shell
	 */
	args[0] = "/bin/sh";
	args[1] = "sh";
	args[2] = "-c";
	args[3] = cmd;
	args[4] = (char *)NULL;
	av = args;
    } else {
	/*
	 * No meta-characters, so no need to exec a shell. Break the command
	 * into words to form an argument vector we can execute.
	 */
	int argmax, curlen;
	char *buf;
	register int argc, ch;
	register char inquote, *p, *start, *t;
	Boolean done;

	argmax = 32;
	av = (char **)emalloc(argmax * sizeof(char *));

	/* allocate room for a copy of the string */
	buf = strdup(cmd);

	/*
	 * copy the string; at the same time, parse backslashes,
	 * quotes and build the argument list.
	 */
	argc = 1;
	inquote = 0;
	done = FALSE;
	for (p = cmd, start = t = buf; !done; ++p) {
	    switch(ch = *p) {
	    case '"':
	    case '\'':
		if (inquote)
		    if (inquote == ch)
			inquote = 0;
		    else
			break;
		else
		    inquote = ch;
		continue;
	    case ' ':
	    case '\t':
		if (inquote)
		    break;
		if (!start)
		    continue;
		/* FALLTHROUGH */
	    case '\n':
	    case '\0':
		/*
		 * end of a token -- make sure there's enough av
		 * space and save off a pointer.
		 */
		*t++ = '\0';
		if (argc == argmax) {
		    argmax <<= 1;		/* ramp up fast */
		    av = (char **)realloc(av,
					    argmax * sizeof(char *));
		    if (av == 0)
			enomem();
		}
		av[argc++] = start;
		start = (char *)NULL;
		if (ch == '\n' || ch == '\0')
		    done = TRUE;
		continue;
	    case '\\':
		switch (ch = *++p) {
		case '\0':
		case '\n':
		    /* hmmm; fix it up as best we can */
		    ch = '\\';
		    --p;
		    break;
		}
		break;
	    }
	    if (!start)
		start = t;
	    *t++ = ch;
	}
	av[0] = av[1];
	av[argc] = (char *)NULL;
    }

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

	execvp(av[0], &av[1]);
	_exit(1);
    }
    if (cpid < 0) {
	/*
	 * Couldn't fork -- tell the user and make the variable null
	 */
	Error("Couldn't exec \"%s\"", cmd);
	return(strdup(""));
    }

    /*
     * No need for the writing half
     */
    close(fds[1]);

    for (;;) {
	int nfds;
	fd_set rfds;
	struct timeval tout;

	FD_ZERO(&rfds);
	FD_SET(fds[0], &rfds);
	tout.tv_sec = 0;
	tout.tv_usec = 500000;
	if ((nfds = select(FD_SETSIZE, &rfds, (int *)0, (int *)0, &tout)) < 0)
	    break;
	if (nfds == 0)
	    continue;

	/*
	 * Read all the characters the child wrote.
	 */
	cc = read(fds[0], result, sizeof(result));
	if (cc < 0) {
	    /*
	     * Couldn't read the child's output -- tell the user and
	     * set the variable to null
	     */
	    Error("Couldn't read shell's output");
	    break;
	}
	if (cc == 0)
	    break;

	Buf_AddBytes(buf, cc, result);
    }

    /*
     * Wait for the process to exit.
     */
    while ((pid = waitpid(cpid, &status, 0)) != cpid && pid >= 0)
	;

    if (status) {
	/*
	 * Child returned an error -- tell the user but still use
	 * the result.
	 */
	Error("\"%s\" returned non-zero", cmd);
    }

    /*
     * Close the input side of the pipe.
     */
    close(fds[0]);

    /*
     * Null-terminate the result, convert newlines to spaces and
     * install it in the variable.
     */
    Buf_AddByte(buf, (Byte)'\0');
    str = (char *)Buf_GetAll (buf, (int *)NULL);
    str = strdup(str);
    cp = str + strlen(str) - 1;

    if (*cp == '\n') {
	/*
	 * A final newline is just stripped
	 */
	*cp-- = '\0';
    }
    while (cp >= str) {
	if (*cp == '\n') {
	    *cp = ' ';
	}
	cp--;
    }

    if (DEBUG(VAR)) {
	printf("Var: ${var:!cmd!} returned %s\n", str);
    }
    return (str);
}

/*-
 *-----------------------------------------------------------------------
 * VarCmp  --
 *	See if the given variable matches the named one. Called from
 *	Lst_Find when searching for a variable of a given name.
 *
 * Results:
 *	0 if they match. non-zero otherwise.
 *
 * Side Effects:
 *	none
 *-----------------------------------------------------------------------
 */
STATIC int
VarCmp (v, name)
    Var            *v;		/* VAR structure to compare */
    char           *name;	/* name to look for */
{
    if (*name != *v->name)
	return (1);
    if (*v->name && v->name[1] == 0)
	return (name[1] != 0);
    return (strcmp (name, v->name));
}

STATIC Hash_Table *
VarHashTable(ctxt)
    GNode          	*ctxt;
{
    if (ctxt == VAR_CMD)
	return(&cmdHashTable);
    if (ctxt == VAR_GLOBAL)
	return(&globalHashTable);
    if (ctxt == VAR_ENV)
	return(&envHashTable);
    return((Hash_Table *)NULL);
}

STATIC Var *
VarFindEntry (ctxt, name)
    GNode          	*ctxt;	/* context in which to find it */
    char           	*name;	/* name to find */
{
    Hash_Table		*ht;
    Hash_Entry		*he;	/* hash entry */
    LstNode		var;

    if ((ht = VarHashTable(ctxt)) == NULL) {
	var = Lst_Find(ctxt->context, (ClientData)name, VarCmp);
	if (var == NILLNODE)
	    return ((Var *)NIL);
	return ((Var *)Lst_Datum (var));
    }
    he = Hash_FindEntry(ht, name);
    if (he == NULL)
	return ((Var *)NIL);
    return ((Var *)Hash_GetValue(he));
}

STATIC void
VarSet(ctxt, var)
    GNode          	*ctxt;	/* context in which to find it */
    Var			*var;
{
    Hash_Table		*ht;
    Hash_Entry *he;
    Boolean isNew;

    if ((ht = VarHashTable(ctxt)) == NULL) {
	(void) Lst_AtFront (ctxt->context, (ClientData)var);
	return;
    }
    he = Hash_CreateEntry(ht, var->name, &isNew);
    if (!isNew)
	printf("%s:%s: entry exists\n");
    Hash_SetValue(he, var);
}

void
VarUnset(ctxt, name)
    GNode		*ctxt;
    char		*name;
{
    register Var  *v;
    Hash_Table *ht;

    if ((ht = VarHashTable(ctxt)) == NULL) {
	LstNode ln;

	ln = Lst_Find(ctxt->context, (ClientData)name, VarCmp);
	if (ln == NILLNODE)
	    return;
	v = (Var *)Lst_Datum(ln);
	Lst_Remove(ctxt->context, ln);
    } else {
	Hash_Entry *he;

	he = Hash_FindEntry(ht, name);
	if (he == NULL)
	    return;
	v = (Var *) Hash_GetValue(he);
	Hash_DeleteEntry(ht, he);
    }
    Buf_Destroy(v->val);
    free(v->name);
    free((char *)v);
}

STATIC char *
VarSpecial(name)
    char *name;
{
    if (*name == '.' && isupper(name[1])) {
	switch (name[1]) {
	case 'A':
	    if (!strcmp(name, ".ALLSRC"))
		return(ALLSRC);
	    if (!strcmp(name, ".ARCHIVE"))
		return(ARCHIVE);
	    break;
	case 'I':
	    if (!strcmp(name, ".IMPSRC"))
		return(IMPSRC);
	    break;
	case 'M':
	    if (!strcmp(name, ".MEMBER"))
		return(MEMBER);
	    break;
	case 'O':
	    if (!strcmp(name, ".OODATE"))
		return(OODATE);
	    break;
	case 'P':
	    if (!strcmp(name, ".PREFIX"))
		return(PREFIX);
	    break;
	case 'T':
	    if (!strcmp(name, ".TARGET"))
		return(TARGET);
	    break;
	}
    } else if (*name && name[1] == 0)
	return(index("@?><*!%", *name));
    return(NULL);
}

/*-
 *-----------------------------------------------------------------------
 * VarFind --
 *	Find the given variable in the given context and any other contexts
 *	indicated.
 *
 * Results:
 *	A pointer to the structure describing the desired variable or
 *	NIL if the variable does not exist.
 *
 * Side Effects:
 *	None
 *-----------------------------------------------------------------------
 */
STATIC Var *
VarFind (name, ctxt, flags)
    char           	*name;	/* name to find */
    GNode          	*ctxt;	/* context in which to find it */
    int             	flags;	/* FIND_GLOBAL set means to look in the
				 * VAR_GLOBAL context as well.
				 * FIND_CMD set means to look in the VAR_CMD
				 * context also.
				 * FIND_ENV set means to look in the
				 * environment */
{
    Var		  	*v;
    char		*n, namebuf[2];

    /*
     * First look for the variable in the given context. If it's not there,
     * look for it in VAR_CMD, VAR_GLOBAL and the environment, in that order,
     * depending on the FIND_* flags in 'flags'
     */
    if (ctxt != VAR_CMD && ctxt != VAR_GLOBAL && ctxt != VAR_ENV) {
	n = VarSpecial(name);
	if (n == NULL) {
	    if (ctxt->contextExtras)
		v = VarFindEntry (ctxt, name);
	    else
		v = (Var *)NIL;
	} else {
	    namebuf[0] = *n;
	    namebuf[1] = 0;
	    name = namebuf;
	    v = VarFindEntry (ctxt, name);
	}
    } else
	v = VarFindEntry (ctxt, name);
    if (v == (Var *)NIL && (flags & FIND_CMD) && ctxt != VAR_CMD)
	v = VarFindEntry (VAR_CMD, name);
    if (!checkEnvFirst && v == (Var *)NIL && (flags & FIND_GLOBAL) &&
	ctxt != VAR_GLOBAL)
	v = VarFindEntry (VAR_GLOBAL, name);
    if (v == (Var *)NIL && (flags & FIND_ENV)) {
	v = VarFindEntry (VAR_ENV, name);
	if (v != (Var *)NIL)
	    v->flags |= VAR_FROM_ENV;
	else if (checkEnvFirst && (flags & FIND_GLOBAL) &&
		   ctxt != VAR_GLOBAL)
	    v = VarFindEntry (VAR_GLOBAL, name);
	else
	    v = (Var *)NIL;
    }
    return (v);
}

/*-
 *-----------------------------------------------------------------------
 * VarAdd  --
 *	Add a new variable of name name and value val to the given context
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	The new variable is placed at the front of the given context
 *	The name and val arguments are duplicated so they may
 *	safely be freed.
 *-----------------------------------------------------------------------
 */
STATIC
VarAdd (name, val, ctxt)
    char           *name;	/* name of variable to add */
    char           *val;	/* value to set it to */
    GNode          *ctxt;	/* context in which to set it */
{
    register Var   *v;
    int	    	  len;

    v = (Var *) emalloc (sizeof (Var));

    v->name = strdup (name);

    len = strlen(val);
    v->val = Buf_Init(len+1);
    Buf_AddBytes(v->val, len, (Byte *)val);

    v->flags = 0;

    VarSet(ctxt, v);
    if (DEBUG(VAR)) {
	printf("%s:%s = %s\n", ctxt->name, name, val);
    }
}

/*-
 *-----------------------------------------------------------------------
 * Var_Delete --
 *	Remove a variable from a context.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The Var structure is removed and freed.
 *
 *-----------------------------------------------------------------------
 */
void
Var_Delete(name, ctxt)
    char    	  *name;
    GNode	  *ctxt;
{

    if (DEBUG(VAR)) {
	printf("%s:delete %s\n", ctxt->name, name);
    }
    VarUnset(ctxt, name);
}

/*-
 *-----------------------------------------------------------------------
 * Var_Set --
 *	Set the variable name to the value val in the given context.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	If the variable doesn't yet exist, a new record is created for it.
 *	Else the old value is freed and the new one stuck in its place
 *
 * Notes:
 *	The variable is searched for only in its context before being
 *	created in that context. I.e. if the context is VAR_GLOBAL,
 *	only VAR_GLOBAL->context is searched. Likewise if it is VAR_CMD, only
 *	VAR_CMD->context is searched. This is done to avoid the literally
 *	thousands of unnecessary strcmp's that used to be done to
 *	set, say, $(@) or $(<).
 *-----------------------------------------------------------------------
 */
void
Var_Set (name, val, ctxt)
    char           *name;	/* name of variable to set */
    char           *val;	/* value to give to the variable */
    GNode          *ctxt;	/* context in which to set it */
{
    register Var   *v;

    /*
     * We only look for a variable in the given context since anything set
     * here will override anything in a lower context, so there's not much
     * point in searching them all just to save a bit of memory...
     */
    v = VarFind (name, ctxt, 0);
    if (v == (Var *) NIL) {
	VarAdd (name, val, ctxt);
    } else {
	Buf_Discard(v->val, Buf_Size(v->val));
	Buf_AddBytes(v->val, strlen(val), (Byte *)val);

	if (DEBUG(VAR)) {
	    printf("%s:%s = %s\n", ctxt->name, name, val);
	}
    }
    /*
     * Any variables given on the command line are automatically exported
     * to the environment (as per POSIX standard)
     */
    if (ctxt == VAR_CMD) {
	setenv(name, val, 1);
    }
}

/*-
 *-----------------------------------------------------------------------
 * Var_Append --
 *	The variable of the given name has the given value appended to it in
 *	the given context.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	If the variable doesn't exist, it is created. Else the strings
 *	are concatenated (with a space in between).
 *
 * Notes:
 *	Only if the variable is being sought in the global context is the
 *	environment searched.
 *	XXX: Knows its calling circumstances in that if called with ctxt
 *	an actual target, it will only search that context since only
 *	a local variable could be being appended to. This is actually
 *	a big win and must be tolerated.
 *-----------------------------------------------------------------------
 */
void
Var_Append (name, val, ctxt)
    char           *name;	/* Name of variable to modify */
    char           *val;	/* String to append to it */
    GNode          *ctxt;	/* Context in which this should occur */
{
    register Var   *v;
    register char  *cp;

    v = VarFind (name, ctxt, (ctxt == VAR_GLOBAL) ? FIND_ENV : 0);

    if (v == (Var *) NIL) {
	VarAdd (name, val, ctxt);
    } else {
	Buf_AddByte(v->val, (Byte)' ');
	Buf_AddBytes(v->val, strlen(val), (Byte *)val);

	if (DEBUG(VAR)) {
	    printf("%s:%s = %s\n", ctxt->name, name,
		   Buf_GetAll(v->val, (int *)NULL));
	}

	if (v->flags & VAR_FROM_ENV) {
	    /*
	     * If the original variable came from the environment, we
	     * have to install it in the global context (we could place
	     * it in the environment, but then we should provide a way to
	     * export other variables...)
	     */
	    v->flags &= ~VAR_FROM_ENV;
	    VarSet(ctxt, v);
	}
    }
}

/*-
 *-----------------------------------------------------------------------
 * Var_Exists --
 *	See if the given variable exists.
 *
 * Results:
 *	TRUE if it does, FALSE if it doesn't
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
Boolean
Var_Exists(name, ctxt)
    char	  *name;    	/* Variable to find */
    GNode	  *ctxt;    	/* Context in which to start search */
{
    Var	    	  *v;

    v = VarFind(name, ctxt, FIND_CMD|FIND_GLOBAL|FIND_ENV);

    return ((Boolean)(v != (Var *)NIL));
}

/*-
 *-----------------------------------------------------------------------
 * Var_Value --
 *	Return the value of the named variable in the given context
 *
 * Results:
 *	The value if the variable exists, NULL if it doesn't
 *
 * Side Effects:
 *	None
 *-----------------------------------------------------------------------
 */
char *
Var_Value (name, ctxt)
    char           *name;	/* name to find */
    GNode          *ctxt;	/* context in which to search for it */
{
    Var            *v;

    v = VarFind (name, ctxt, FIND_ENV | FIND_GLOBAL | FIND_CMD);
    if (v != (Var *) NIL) {
	return ((char *)Buf_GetAll(v->val, (int *)NULL));
    } else {
	return ((char *) NULL);
    }
}

typedef struct {
    GNode   	*ctxt;    	/* The context for the variable */
    Boolean	err;    	/* TRUE if undefined variables are an error */
    Boolean	wantData;	/* TRUE if we care about the result */
    Var	    	*valVar;	/* Variable to set to the current word */
    char    	*newVal;	/* String to evaluate */
    int	    	len;		/* Length of newVal string */
} VarIndir;

/*-
 *-----------------------------------------------------------------------
 * VarIndirect --
 *	Perform a substitution on the given word, placing the
 *	result in the passed buffer.
 *
 * Results:
 *	TRUE if a space is needed before more characters are added.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
STATIC Boolean
VarIndirect (word, addSpace, buf, indirData)
    char    	  	*word;		/* Word to modify */
    Boolean 	  	addSpace;	/* True if space should be added before
					 * other characters */
    Buffer  	  	buf;		/* Buffer for result */
    register VarIndir	*indirData;	/* Data for the indirection */
{
    register int  	wordLen;/* Length of word */
    register char 	*cp;	/* General pointer */
    register Buffer	val;	/* Value buffer for our indirect variable */
    Boolean needSpace = addSpace;

    if (buf != (Buffer)0) {
	wordLen = strlen(word);
	val = indirData->valVar->val;
	if (indirData->len == -1) {
	    Buf_AddBytes(val, wordLen, (Byte *)word);
	} else {
	    Buf_Discard(val, Buf_Size(val));
	    Buf_AddBytes(val, wordLen, (Byte *)word);
	}
    }
    for (cp = indirData->newVal; *cp != '@' && *cp != '\0'; cp++) {
	if (*cp == '\\' && (cp[1] == '@' || cp[1] == '$' || cp[1] == '\\'))
	{
	    if (buf != (Buffer)0) {
		if (needSpace) {
		    Buf_AddByte (buf, (Byte)' ');
		    needSpace = FALSE;
		}
		addSpace = TRUE;
		Buf_AddByte(buf, (Byte)cp[1]);
	    }
	    cp++;
	} else if (*cp == '$') {
	    /*
	     * If unescaped dollar sign, assume it's a variable
	     * substitution and recurse.
	     */
	    char	*cp2;
	    int		len;
	    Boolean	freeIt;

	    cp2 = VarParse (cp, indirData->ctxt, indirData->err,
			    indirData->wantData, &len, &freeIt);
	    if (buf != (Buffer)0) {
		if (needSpace) {
		    Buf_AddByte (buf, (Byte)' ');
		    needSpace = FALSE;
		}
		addSpace = TRUE;
		Buf_AddBytes(buf, strlen(cp2), (Byte *)cp2);
	    }
	    if (freeIt) {
		free(cp2);
	    }
	    cp += len - 1;
	} else if (buf != (Buffer)0) {
	    if (needSpace) {
		Buf_AddByte (buf, (Byte)' ');
		needSpace = FALSE;
	    }
	    addSpace = TRUE;
	    Buf_AddByte(buf, (Byte)*cp);
	}
    }
    /*
     * Special case for the first time we are called.  We could not
     * determine the length of the replacement string until we were
     * called since we did not have a value for the indirection
     * variable.  Now we can use the first word for that value and
     * remember how many bytes of the newVal string were consumed
     * during the expansion.  We will then use that knowledge during
     * subsequent calls.
     */
    if (indirData->len == -1)
	indirData->len = cp - indirData->newVal;
    return(addSpace);
}

/*-
 *-----------------------------------------------------------------------
 * VarHead --
 *	Remove the tail of the given word and place the result in the given
 *	buffer.
 *
 * Results:
 *	TRUE if characters were added to the buffer (a space needs to be
 *	added to the buffer before the next word).
 *
 * Side Effects:
 *	The trimmed word is added to the buffer.
 *
 *-----------------------------------------------------------------------
 */
STATIC Boolean
VarHead (word, addSpace, buf)
    char    	  *word;    	/* Word to trim */
    Boolean 	  addSpace; 	/* True if need to add a space to the buffer
				 * before sticking in the head */
    Buffer  	  buf;	    	/* Buffer in which to store it */
{
    register char *slash;

    slash = rindex (word, '/');
    if (slash != (char *)NULL) {
	if (addSpace) {
	    Buf_AddByte (buf, (Byte)' ');
	}
	*slash = '\0';
	Buf_AddBytes (buf, strlen (word), (Byte *)word);
	*slash = '/';
	return (TRUE);
    } else {
	/*
	 * If no directory part, give . (q.v. the POSIX standard)
	 */
	if (addSpace) {
	    Buf_AddBytes(buf, 2, (Byte *)" .");
	} else {
	    Buf_AddByte(buf, (Byte)'.');
	}
	return(TRUE);
    }
}

/*-
 *-----------------------------------------------------------------------
 * VarTail --
 *	Remove the head of the given word and place the result in the given
 *	buffer.
 *
 * Results:
 *	TRUE if characters were added to the buffer (a space needs to be
 *	added to the buffer before the next word).
 *
 * Side Effects:
 *	The trimmed word is added to the buffer.
 *
 *-----------------------------------------------------------------------
 */
STATIC Boolean
VarTail (word, addSpace, buf)
    char    	  *word;    	/* Word to trim */
    Boolean 	  addSpace; 	/* TRUE if need to stick a space in the
				 * buffer before adding the tail */
    Buffer  	  buf;	    	/* Buffer in which to store it */
{
    register char *slash;

    if (addSpace) {
	Buf_AddByte (buf, (Byte)' ');
    }

    slash = rindex (word, '/');
    if (slash != (char *)NULL) {
	*slash++ = '\0';
	Buf_AddBytes (buf, strlen(slash), (Byte *)slash);
	slash[-1] = '/';
    } else {
	Buf_AddBytes (buf, strlen(word), (Byte *)word);
    }
    return (TRUE);
}

/*-
 *-----------------------------------------------------------------------
 * VarSuffix --
 *	Place the suffix of the given word in the given buffer.
 *
 * Results:
 *	TRUE if characters were added to the buffer (a space needs to be
 *	added to the buffer before the next word).
 *
 * Side Effects:
 *	The suffix from the word is placed in the buffer.
 *
 *-----------------------------------------------------------------------
 */
STATIC Boolean
VarSuffix (word, addSpace, buf)
    char    	  *word;    	/* Word to trim */
    Boolean 	  addSpace; 	/* TRUE if need to add a space before placing
				 * the suffix in the buffer */
    Buffer  	  buf;	    	/* Buffer in which to store it */
{
    register char *dot;

    dot = rindex (word, '.');
    if (dot != (char *)NULL) {
	if (addSpace) {
	    Buf_AddByte (buf, (Byte)' ');
	}
	*dot++ = '\0';
	Buf_AddBytes (buf, strlen (dot), (Byte *)dot);
	dot[-1] = '.';
	return (TRUE);
    } else {
	return (addSpace);
    }
}

/*-
 *-----------------------------------------------------------------------
 * VarRoot --
 *	Remove the suffix of the given word and place the result in the
 *	buffer.
 *
 * Results:
 *	TRUE if characters were added to the buffer (a space needs to be
 *	added to the buffer before the next word).
 *
 * Side Effects:
 *	The trimmed word is added to the buffer.
 *
 *-----------------------------------------------------------------------
 */
STATIC Boolean
VarRoot (word, addSpace, buf)
    char    	  *word;    	/* Word to trim */
    Boolean 	  addSpace; 	/* TRUE if need to add a space to the buffer
				 * before placing the root in it */
    Buffer  	  buf;	    	/* Buffer in which to store it */
{
    register char *dot;

    if (addSpace) {
	Buf_AddByte (buf, (Byte)' ');
    }

    dot = rindex (word, '.');
    if (dot != (char *)NULL) {
	*dot = '\0';
	Buf_AddBytes (buf, strlen (word), (Byte *)word);
	*dot = '.';
    } else {
	Buf_AddBytes (buf, strlen(word), (Byte *)word);
    }
    return (TRUE);
}

/*-
 *-----------------------------------------------------------------------
 * VarMatch --
 *	Place the word in the buffer if it matches the given pattern.
 *	Callback function for VarModify to implement the :M modifier.
 *	
 * Results:
 *	TRUE if a space should be placed in the buffer before the next
 *	word.
 *
 * Side Effects:
 *	The word may be copied to the buffer.
 *
 *-----------------------------------------------------------------------
 */
STATIC Boolean
VarMatch (word, addSpace, buf, pattern)
    char    	  *word;    	/* Word to examine */
    Boolean 	  addSpace; 	/* TRUE if need to add a space to the
				 * buffer before adding the word, if it
				 * matches */
    Buffer  	  buf;	    	/* Buffer in which to store it */
    char    	  *pattern; 	/* Pattern the word must match */
{
    if (Str_Match(word, pattern)) {
	if (addSpace) {
	    Buf_AddByte(buf, (Byte)' ');
	}
	addSpace = TRUE;
	Buf_AddBytes(buf, strlen(word), (Byte *)word);
    }
    return(addSpace);
}

/*-
 *-----------------------------------------------------------------------
 * VarNoMatch --
 *	Place the word in the buffer if it doesn't match the given pattern.
 *	Callback function for VarModify to implement the :N modifier.
 *	
 * Results:
 *	TRUE if a space should be placed in the buffer before the next
 *	word.
 *
 * Side Effects:
 *	The word may be copied to the buffer.
 *
 *-----------------------------------------------------------------------
 */
STATIC Boolean
VarNoMatch (word, addSpace, buf, pattern)
    char    	  *word;    	/* Word to examine */
    Boolean 	  addSpace; 	/* TRUE if need to add a space to the
				 * buffer before adding the word, if it
				 * matches */
    Buffer  	  buf;	    	/* Buffer in which to store it */
    char    	  *pattern; 	/* Pattern the word must match */
{
    if (!Str_Match(word, pattern)) {
	if (addSpace) {
	    Buf_AddByte(buf, (Byte)' ');
	}
	addSpace = TRUE;
	Buf_AddBytes(buf, strlen(word), (Byte *)word);
    }
    return(addSpace);
}

typedef struct {
    char    	  *lhs;	    /* String to match */
    int	    	  leftLen;  /* Length of string */
    char    	  *rhs;	    /* Replacement string (w/ &'s removed) */
    int	    	  rightLen; /* Length of replacement */
    int	    	  flags;
#define VAR_SUB_GLOBAL	1   /* Apply substitution globally */
#define VAR_MATCH_START	2   /* Match at start of word */
#define VAR_MATCH_END	4   /* Match at end of word */
#define VAR_NO_SUB	8   /* Substitution is non-global and already done */
} VarPattern;

/*-
 *-----------------------------------------------------------------------
 * VarSubstitute --
 *	Perform a string-substitution on the given word, placing the
 *	result in the passed buffer.
 *
 * Results:
 *	TRUE if a space is needed before more characters are added.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
STATIC Boolean
VarSubstitute (word, addSpace, buf, pattern)
    char    	  	*word;	    /* Word to modify */
    Boolean 	  	addSpace;   /* True if space should be added before
				     * other characters */
    Buffer  	  	buf;	    /* Buffer for result */
    register VarPattern	*pattern;   /* Pattern for substitution */
{
    register int  	wordLen;    /* Length of word */
    register char 	*cp;	    /* General pointer */

    wordLen = strlen(word);
    if ((pattern->flags & VAR_NO_SUB) == 0) {
	/*
	 * Still substituting -- break it down into simple anchored cases
	 * and if none of them fits, perform the general substitution case.
	 */
	if ((pattern->flags & VAR_MATCH_START) &&
	    (strncmp(word, pattern->lhs, pattern->leftLen) == 0)) {
		/*
		 * Anchored at start and beginning of word matches pattern
		 */
		if ((pattern->flags & VAR_MATCH_END) &&
		    (wordLen == pattern->leftLen)) {
			/*
			 * Also anchored at end and matches to the end (word
			 * is same length as pattern) add space and rhs only
			 * if rhs is non-null.
			 */
			if (pattern->rightLen != 0) {
			    if (addSpace) {
				Buf_AddByte(buf, (Byte)' ');
			    }
			    addSpace = TRUE;
			    Buf_AddBytes(buf, pattern->rightLen,
					 (Byte *)pattern->rhs);
			}
		} else if (pattern->flags & VAR_MATCH_END) {
		    /*
		     * Doesn't match to end -- copy word wholesale
		     */
		    goto nosub;
		} else {
		    /*
		     * Matches at start but need to copy in trailing characters
		     */
		    if ((pattern->rightLen + wordLen - pattern->leftLen) != 0){
			if (addSpace) {
			    Buf_AddByte(buf, (Byte)' ');
			}
			addSpace = TRUE;
		    }
		    Buf_AddBytes(buf, pattern->rightLen, (Byte *)pattern->rhs);
		    Buf_AddBytes(buf, wordLen - pattern->leftLen,
				 (Byte *)(word + pattern->leftLen));
		}
	} else if (pattern->flags & VAR_MATCH_START) {
	    /*
	     * Had to match at start of word and didn't -- copy whole word.
	     */
	    goto nosub;
	} else if (pattern->flags & VAR_MATCH_END) {
	    /*
	     * Anchored at end, Find only place match could occur (leftLen
	     * characters from the end of the word) and see if it does. Note
	     * that because the $ will be left at the end of the lhs, we have
	     * to use strncmp.
	     */
	    cp = word + (wordLen - pattern->leftLen);
	    if ((cp >= word) &&
		(strncmp(cp, pattern->lhs, pattern->leftLen) == 0)) {
		/*
		 * Match found. If we will place characters in the buffer,
		 * add a space before hand as indicated by addSpace, then
		 * stuff in the initial, unmatched part of the word followed
		 * by the right-hand-side.
		 */
		if (((cp - word) + pattern->rightLen) != 0) {
		    if (addSpace) {
			Buf_AddByte(buf, (Byte)' ');
		    }
		    addSpace = TRUE;
		}
		Buf_AddBytes(buf, cp - word, (Byte *)word);
		Buf_AddBytes(buf, pattern->rightLen, (Byte *)pattern->rhs);
	    } else {
		/*
		 * Had to match at end and didn't. Copy entire word.
		 */
		goto nosub;
	    }
	} else {
	    /*
	     * Pattern is unanchored: search for the pattern in the word using
	     * String_FindSubstring, copying unmatched portions and the
	     * right-hand-side for each match found, handling non-global
	     * subsititutions correctly, etc. When the loop is done, any
	     * remaining part of the word (word and wordLen are adjusted
	     * accordingly through the loop) is copied straight into the
	     * buffer.
	     * addSpace is set FALSE as soon as a space is added to the
	     * buffer.
	     */
	    register Boolean done;
	    int origSize;

	    done = FALSE;
	    origSize = Buf_Size(buf);
	    while (!done) {
		cp = Str_FindSubstring(word, pattern->lhs);
		if (cp != (char *)NULL) {
		    if (addSpace && (((cp - word) + pattern->rightLen) != 0)){
			Buf_AddByte(buf, (Byte)' ');
			addSpace = FALSE;
		    }
		    Buf_AddBytes(buf, cp-word, (Byte *)word);
		    Buf_AddBytes(buf, pattern->rightLen, (Byte *)pattern->rhs);
		    wordLen -= (cp - word) + pattern->leftLen;
		    word = cp + pattern->leftLen;
		    if (wordLen == 0) {
			done = TRUE;
		    }
		    if ((pattern->flags & VAR_SUB_GLOBAL) == 0) {
			done = TRUE;
			pattern->flags |= VAR_NO_SUB;
		    }
		} else {
		    done = TRUE;
		}
	    }
	    if (wordLen != 0) {
		if (addSpace) {
		    Buf_AddByte(buf, (Byte)' ');
		}
		Buf_AddBytes(buf, wordLen, (Byte *)word);
	    }
	    /*
	     * If added characters to the buffer, need to add a space
	     * before we add any more. If we didn't add any, just return
	     * the previous value of addSpace.
	     */
	    return ((Buf_Size(buf) != origSize) || addSpace);
	}
	/*
	 * Common code for anchored substitutions: if performed a substitution
	 * and it's not supposed to be global, mark the pattern as requiring
	 * no more substitutions. addSpace was set TRUE if characters were
	 * added to the buffer.
	 */
	if ((pattern->flags & VAR_SUB_GLOBAL) == 0) {
	    pattern->flags |= VAR_NO_SUB;
	}
	return (addSpace);
    }
 nosub:
    if (addSpace) {
	Buf_AddByte(buf, (Byte)' ');
    }
    Buf_AddBytes(buf, wordLen, (Byte *)word);
    return(TRUE);
}

/*-
 *-----------------------------------------------------------------------
 * VarModify --
 *	Modify each of the words of the passed string using the given
 *	function. Used to implement all modifiers.
 *
 * Results:
 *	A string of all the words modified appropriately.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
STATIC char *
VarModify (str, modProc, datum)
    char    	  *str;	    	    /* String whose words should be trimmed */
    Boolean    	  (*modProc)();     /* Function to use to modify them */
    ClientData	  datum;    	    /* Datum to pass it */
{
    Buffer  	  buf;	    	    /* Buffer for the new string */
    register char *cp;	    	    /* Pointer to end of current word */
    char    	  endc;	    	    /* Character that ended the word */
    Boolean 	  addSpace; 	    /* TRUE if need to add a space to the
				     * buffer before adding the trimmed
				     * word */
    
    buf = Buf_Init (0);
    cp = str;
    addSpace = FALSE;
    
    while (1) {
	/*
	 * Skip to next word and place cp at its end.
	 */
	while (isspace (*str)) {
	    str++;
	}
	for (cp = str; *cp != '\0' && !isspace (*cp); cp++) {
	    /* void */ ;
	}
	if (cp == str) {
	    /*
	     * If we didn't go anywhere, we must be done!
	     */
	    Buf_AddByte (buf, '\0');
	    str = strdup((char *)Buf_GetAll (buf, (int *)NULL));
	    Buf_Destroy (buf);
	    return (str);
	}
	/*
	 * Nuke terminating character, but save it in endc b/c if str was
	 * some variable's value, it would not be good to screw it
	 * over...
	 */
	endc = *cp;
	*cp = '\0';

	addSpace = (* modProc) (str, addSpace, buf, datum);

	if (endc) {
	    *cp++ = endc;
	}
	str = cp;
    }
}

/*-
 *-----------------------------------------------------------------------
 * VarParse --
 *	Given the start of a variable invocation, extract the variable
 *	name and find its value, then modify it according to the
 *	specification.  If wantData if FALSE, we just go through the
 *	motions until we know where the variable invocation ends.
 *
 * Results:
 *	The (possibly-modified) value of the variable or var_Error if the
 *	specification is invalid. The length of the specification is
 *	placed in *lengthPtr (for invalid specifications, this is just
 *	2...?).
 *	A Boolean in *freePtr telling whether the returned string should
 *	be freed by the caller.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
STATIC
char *
VarParse (str, ctxt, err, wantData, lengthPtr, freePtr)
    char    	  *str;	    	/* The string to parse */
    GNode   	  *ctxt;    	/* The context for the variable */
    Boolean 	    err;    	/* TRUE if undefined variables are an error */
    Boolean	    wantData;	/* TRUE if we care about the result */
    int	    	    *lengthPtr;	/* OUT: The length of the specification */
    Boolean 	    *freePtr; 	/* OUT: TRUE if caller should free result */
{
    register char   *tstr;    	/* Pointer into str */
    Var	    	    *v;	    	/* Variable in invocation */
    register char   *cp;    	/* Secondary pointer into str (place marker
				 * for tstr) */
    Boolean 	    haveModifier;/* TRUE if have modifiers for the variable */
    register char   endc;    	/* Ending character when variable in parens
				 * or braces */
    char    	    *start;
    Boolean 	    dynamic;	/* TRUE if the variable is local and we're
				 * expanding it in a non-local context. This
				 * is done to support dynamic sources. The
				 * result is just the invocation, unaltered */
    char	    *namebuf;	/* Buffer used to replace str if the variable
				 * name we are processing contains another
				 * imbedded variable */
    int		    lenAdjust;	/* This is an adjustment to be made to the
				 * lengthPtr calculation since we may have
				 * expanded imbedded variable(s) in the
				 * variable we are processing */
    
    *freePtr = FALSE;
    dynamic = FALSE;
    start = str;
    namebuf = NULL;
    lenAdjust = 0;
    
    if (str[1] != '(' && str[1] != '{') {
	/*
	 * If it's not bounded by braces of some sort, life is much simpler.
	 * We just need to check for the first character and return the
	 * value if it exists.
	 */
	char	  name[2];

	if (!wantData) {
	    *lengthPtr = 2;
	    if (DEBUG(VAR)) {
		printf("VarParse: returning without data\n");
	    }
	    return (NULL);
	}
	name[0] = str[1];
	name[1] = '\0';

	v = VarFind (name, ctxt, FIND_ENV | FIND_GLOBAL | FIND_CMD);
	if (v == (Var *)NIL) {
	    *lengthPtr = 2;
	    
	    if ((ctxt == VAR_CMD) || (ctxt == VAR_GLOBAL)) {
		/*
		 * If substituting a local variable in a non-local context,
		 * assume it's for dynamic source stuff. We have to handle
		 * this specially and return the longhand for the variable
		 * with the dollar sign escaped so it makes it back to the
		 * caller. Only four of the local variables are treated
		 * specially as they are the only four that will be set
		 * when dynamic sources are expanded.
		 */
		switch (str[1]) {
		    case '@':
			return("$(.TARGET)");
		    case '%':
			return("$(.ARCHIVE)");
		    case '*':
			return("$(.PREFIX)");
		    case '!':
			return("$(.MEMBER)");
		}
	    }
	    /*
	     * Error
	     */
	    return (err ? var_Error : varNoError);
	}
	haveModifier = FALSE;
	tstr = &str[1];
	endc = str[1];
    } else {
	endc = str[1] == '(' ? ')' : '}';

	/*
	 * Skip to the end character or a colon, whichever comes first.
	 */
	tstr = str + 2;
	for (;;) {
	    while (*tstr != endc && *tstr != ':' &&
		   *tstr != '$' && *tstr != '\0')
		tstr++;
	    if (*tstr == ':') {
		haveModifier = TRUE;
		break;
	    }
	    if (*tstr == endc) {
		haveModifier = FALSE;
		break;
	    }
	    if (*tstr == '$') {
		char	    *cp2;
		int		    len, len2;
		Boolean	    freeIt;
		char	    *old_namebuf;

		cp2 = VarParse(tstr, ctxt, err, wantData, &len, &freeIt);
		if (cp2 == var_Error || cp2 == varNoError) {
		    *lengthPtr = tstr - str - lenAdjust;	/* XXX */
		    Error("Bad imbedded variable in %s", start);
		    return (var_Error);
		}
		if (!wantData) {
		    if (cp2 != NULL)
			printf("VarParse: weird #1\n");
		    tstr += len;
		    continue;
		}
		len2 = strlen(cp2);
		old_namebuf = namebuf;
		namebuf = emalloc(strlen(start) + (len2 - len) + 1);
		strncpy(namebuf, start, (tstr - start));
		strncpy(namebuf + (tstr - start), cp2, len2);
		strcpy(namebuf + (tstr - start) + len2, tstr + len);
		lenAdjust += len2 - len;
		tstr = namebuf + (tstr - start) + len2;
		start = str = namebuf;
		if (freeIt)
		    free(cp2);
		if (old_namebuf != NULL)
		    free(old_namebuf);
		continue;
	    }
	    /*
	     * If we never did find the end character, return NULL
	     * right now, setting the length to be the distance to
	     * the end of the string, since that's what make does.
	     */
	    *lengthPtr = tstr - str - lenAdjust;
	    if (namebuf != NULL)
		free(namebuf);
	    return (var_Error);
	}
	*tstr = '\0';
 	
	v = VarFind (str + 2, ctxt, FIND_ENV | FIND_GLOBAL | FIND_CMD);
	if ((v == (Var *)NIL) && (ctxt != VAR_CMD) && (ctxt != VAR_GLOBAL) &&
	    ((tstr-str) == 4) && (str[3] == 'F' || str[3] == 'D'))
	{
	    /*
	     * Check for bogus D and F forms of local variables since we're
	     * in a local context and the name is the right length.
	     */
	    switch(str[2]) {
		case '@':
		case '%':
		case '*':
		case '!':
		case '>':
		case '<':
		{
		    char    vname[2];
		    char    *val;

		    /*
		     * Well, it's local -- go look for it.
		     */
		    vname[0] = str[2];
		    vname[1] = '\0';
		    v = VarFind(vname, ctxt, 0);
		    
		    if (v != (Var *)NIL) {
			if (!wantData) {
			    *lengthPtr = tstr-start+1-lenAdjust;
			    *tstr = endc;
			    if (namebuf != NULL)
				free(namebuf);
			    return(NULL);
			}
			/*
			 * No need for nested expansion or anything, as we're
			 * the only one who sets these things and we sure don't
			 * but nested invocations in them...
			 */
			val = (char *)Buf_GetAll(v->val, (int *)NULL);
			
			if (str[3] == 'D') {
			    val = VarModify(val, VarHead, (ClientData)0);
			} else {
			    val = VarModify(val, VarTail, (ClientData)0);
			}
			/*
			 * Resulting string is dynamically allocated, so
			 * tell caller to free it.
			 */
			*freePtr = TRUE;
			*lengthPtr = tstr-start+1-lenAdjust;
			*tstr = endc;
			if (namebuf != NULL)
			    free(namebuf);
			return(val);
		    }
		    break;
		}
	    }
	}
			    
	if (v == (Var *)NIL) {
	    if ((((tstr-str) == 3) ||
		 ((((tstr-str) == 4) && (str[3] == 'F' ||
					 str[3] == 'D')))) &&
		((ctxt == VAR_CMD) || (ctxt == VAR_GLOBAL)))
	    {
		/*
		 * If substituting a local variable in a non-local context,
		 * assume it's for dynamic source stuff. We have to handle
		 * this specially and return the longhand for the variable
		 * with the dollar sign escaped so it makes it back to the
		 * caller. Only four of the local variables are treated
		 * specially as they are the only four that will be set
		 * when dynamic sources are expanded.
		 */
		switch (str[2]) {
		    case '@':
		    case '%':
		    case '*':
		    case '!':
			dynamic = TRUE;
			break;
		}
	    } else if (((tstr-str) > 4) && (str[2] == '.') &&
		       isupper(str[3]) &&
		       ((ctxt == VAR_CMD) || (ctxt == VAR_GLOBAL)))
	    {
		int	len;
		
		len = (tstr-str) - 3;
		if ((strncmp(str+2, ".TARGET", len) == 0) ||
		    (strncmp(str+2, ".ARCHIVE", len) == 0) ||
		    (strncmp(str+2, ".PREFIX", len) == 0) ||
		    (strncmp(str+2, ".MEMBER", len) == 0))
		{
		    dynamic = TRUE;
		}
	    }
	    
	    if (!haveModifier) {
		if (!wantData) {
		    *lengthPtr = tstr-start+1-lenAdjust;
		    *tstr = endc;
		    if (namebuf != NULL)
			free(namebuf);
		    return(NULL);
		}
		/*
		 * No modifiers -- have specification length so we can return
		 * now.
		 */
		*lengthPtr = tstr-start+1-lenAdjust;
		*tstr = endc;
		if (namebuf != NULL) {
		    if (dynamic) {
			*freePtr = TRUE;
			return(namebuf);
		    }
		    free(namebuf);
		    return(err ? var_Error : varNoError);
		}
		if (dynamic) {
		    str = emalloc(*lengthPtr + 1);
		    strncpy(str, start, *lengthPtr);
		    str[*lengthPtr] = '\0';
		    *freePtr = TRUE;
		    return(str);
		}
		return (err ? var_Error : varNoError);
	    }
	    if (wantData) {
		/*
		 * Still need to get to the end of the variable specification,
		 * so kludge up a Var structure for the modifications
		 */
		v = (Var *) emalloc(sizeof(Var));
		v->name = strdup(str + 2);
		v->val = Buf_Init(1);
		v->flags = VAR_JUNK;
	    }
	}
    }

    if (wantData) {
	if (v->flags & VAR_IN_USE) {
	    Fatal("Variable %s is recursive.", v->name);
	    /*NOTREACHED*/
	} else {
	    v->flags |= VAR_IN_USE;
	}
	/*
	 * Before doing any modification, we have to make sure the value
	 * has been fully expanded. If it looks like recursion might be
	 * necessary (there's a dollar sign somewhere in the variable's value)
	 * we just call Var_Subst to do any other substitutions that are
	 * necessary. Note that the value returned by Var_Subst will have
	 * been dynamically-allocated, so it will need freeing when we
	 * return.
	 */
	str = (char *)Buf_GetAll(v->val, (int *)NULL);
	if (index (str, '$') != (char *)NULL) {
	    str = Var_Subst(str, ctxt, err);
	    *freePtr = TRUE;
	}

	v->flags &= ~VAR_IN_USE;
    }
    
    /*
     * Now we need to apply any modifiers the user wants applied.
     * These are:
     *  	  :M<pattern>	words which match the given <pattern>.
     *  	  	    	<pattern> is of the standard file
     *  	  	    	wildcarding form.
     *  	  :N<pattern>	words which do not match the given <pattern>.
     *  	  :S<d><pat1><d><pat2><d>[g]
     *  	  	    	Substitute <pat2> for <pat1> in the value
     *  	  :H	    	Substitute the head of each word
     *  	  :T	    	Substitute the tail of each word
     *  	  :E	    	Substitute the extension (minus '.') of
     *  	  	    	each word
     *  	  :R	    	Substitute the root of each word
     *  	  	    	(pathname minus the suffix).
     *	    	  :lhs=rhs  	Like :S, but the rhs goes to the end of
     *	    	    	    	the invocation.
     * New modifiers:
     *		  :@<tmpvar>@<newval>@
     *				Assign a temporary local variable <tmpvar>
     *				to the current value of each word in turn
     *				and replace each word with the result of
     *				evaluating <newval>
     *		  :D<newval>	Use <newval> as value if variable defined
     *		  :U<newval>	Use <newval> as value if variable undefined
     *		  :L		Use the name of the variable as the value.
     *		  :P		Use the path of the node that has the same
     *				name as the variable as the value.  This
     *				basically includes an implied :L so that
     *				the common method of refering to the path
     *				of your dependent 'x' in a rule is to use
     *				the form '${x:P}'.
     *		  :!<cmd>!	If the value is currently non-empty and
     *				contains non-whitespace chanracters, then
     *				use the result of running cmd through the
     *				system(3) call.  The optimization of not
     *				actually running a shell if there are no
     *				shell meta characters is valid.
     */
    if (haveModifier) {
	/*
	 * Skip initial colon while putting it back.
	 */
	*tstr++ = ':';
	while (*tstr != endc) {
	    char	*newStr;    /* New value to return */
	    char	termc;	    /* Character which terminated scan */
	    
	    if (DEBUG(VAR)) {
		if (wantData)
		    printf("Applying :%c to \"%s\"\n", *tstr, str);
		else
		    printf("Skipping :%c\n", *tstr);
	    }
	    switch (*tstr) {
		case '@':
		{
		    Buffer  	    buf;    	/* Buffer for newval */
		    register char   *cp2;
		    Var		    *valVar;
		    VarIndir	    indirData;
		    LstNode	    ln;

		    cp = tstr + 1;
		    while (*cp != '@' && *cp != '\0')
			cp++;
		    if (*cp == '\0') {
			if (wantData)
			    newStr = str;
			termc = *cp;
			break;
		    }

		    if (wantData) {
			valVar = (Var *) emalloc(sizeof(Var));
			*cp = '\0';
			valVar->name = strdup(tstr+1);
			*cp++ = '@';
			valVar->val = Buf_Init(strlen(str) + 1);
			valVar->flags = 0;
			VarSet(ctxt, valVar);
			ctxt->contextExtras++;
		    } else
			cp++;

		    indirData.ctxt = ctxt;
		    indirData.err = err;
		    indirData.wantData = wantData;
		    indirData.valVar = valVar;
		    indirData.newVal = cp;
		    indirData.len = -1;

		    if (wantData)
			newStr = VarModify(str, VarIndirect,
					   (ClientData)&indirData);

		    /*
		     * Need a special call if str is empty to get newval len
		     */
		    if (indirData.len == -1)
			VarIndirect("", 0, (Buffer)0, &indirData);

		    cp += indirData.len;
		    if (*cp == '@')
			cp++;
		    termc = *cp;

		    if (wantData) {
			VarUnset(ctxt, valVar->name);
			ctxt->contextExtras--;
		    }

		    break;
		}
		case 'D':
		case 'U':
		{
		    Buffer  	    buf;    	/* Buffer for patterns */
		    int		    wantit;	/* want data in buffer */

		    if (!wantData)
			wantit = FALSE;
		    else {
			if (*tstr == 'U')
			    wantit = ((v->flags & VAR_JUNK) != 0);
			else
			    wantit = ((v->flags & VAR_JUNK) == 0);
			if ((v->flags & VAR_JUNK) != 0)
			    v->flags |= VAR_KEEP;

			if (wantit)
			    buf = Buf_Init(0);
		    }

		    /*
		     * Pass through tstr looking for 1) escaped delimiters,
		     * '$'s and backslashes (place the escaped character in
		     * uninterpreted) and 2) unescaped $'s that aren't before
		     * the delimiter (expand the variable substitution).
		     * The result is left in the Buffer buf.
		     */
		    for (cp = tstr + 1;
			 *cp != endc && *cp != ':' && *cp != '\0';
			 cp++) {
			if ((*cp == '\\') &&
			    ((cp[1] == ':') ||
			     (cp[1] == '$') ||
			     (cp[1] == endc) ||
			     (cp[1] == '\\')))
			{
			    cp++;
			    if (wantit)
				Buf_AddByte(buf, (Byte)*cp);
			} else if (*cp == '$') {
			    /*
			     * If unescaped dollar sign, assume it's a
			     * variable substitution and recurse.
			     */
			    char	    *cp2;
			    int		    len;
			    Boolean	    freeIt;

			    cp2 = VarParse (cp, ctxt, err, wantData,
					    &len, &freeIt);
			    if (wantit)
				Buf_AddBytes(buf, strlen(cp2), (Byte *)cp2);
			    if (freeIt)
				free(cp2);
			    cp += len - 1;
			} else {
			    if (wantit)
				Buf_AddByte(buf, (Byte)*cp);
			}
		    }
		    termc = *cp;
		    if (wantit) {
			Buf_AddByte(buf, (Byte)'\0');
			newStr = strdup((char *)Buf_GetAll(buf, (int *)NULL));
			Buf_Destroy(buf);
		    } else if (wantData)
			newStr = str;
		    break;
		}
		case 'L':
		{
		    if (wantData) {
			if ((v->flags & VAR_JUNK) != 0)
			    v->flags |= VAR_KEEP;
			newStr = strdup(v->name);
		    }
		    cp = tstr + 1;
		    termc = *cp;
		    break;
		}
		case 'P':
		{
		    GNode *gn;

		    if (wantData) {
			if ((v->flags & VAR_JUNK) != 0)
			    v->flags |= VAR_KEEP;
			gn = Targ_FindNode(v->name, TARG_NOCREATE);
			if (gn == NILGNODE)
			    newStr = strdup(v->name);
			else
			    newStr = strdup(gn->path);
		    }
		    cp = tstr + 1;
		    termc = *cp;
		    break;
		}
		case '!':
		{
		    Buffer  	    buf;    	/* Buffer for cmd */
		    register char   *cp2;
		    int		    len;
		    Boolean	    freeIt;
		    Var		    *valVar;
		    VarIndir	    indirData;
		    LstNode	    ln;

		    if (wantData)
			buf = Buf_Init(0);

		    /*
		     * Pass through tstr looking for 1) escaped delimiters,
		     * '$'s and backslashes (place the escaped character in
		     * uninterpreted) and 2) unescaped $'s that aren't before
		     * the delimiter (expand the variable substitution).
		     * The result is left in the Buffer buf.
		     */
		    for (cp = tstr + 1; *cp != '!' && *cp != '\0'; cp++) {
			if ((*cp == '\\') &&
			    ((cp[1] == '$') ||
			     (cp[1] == '!') ||
			     (cp[1] == '\\'))) {
			    cp++;
			    if (wantData)
				Buf_AddByte(buf, (Byte)*cp);
			} else if (*cp == '$') {
			    /*
			     * If unescaped dollar sign, assume it's a
			     * variable substitution and recurse.
			     */
			    cp2 = VarParse (cp, ctxt, err, wantData,
					    &len, &freeIt);
			    if (wantData)
				Buf_AddBytes(buf, strlen(cp2), (Byte *)cp2);
			    if (freeIt)
				free(cp2);
			    cp += len - 1;
			} else if (wantData) {
			    Buf_AddByte(buf, (Byte)*cp);
			}
		    }
		    if (*cp != '!') {
			termc = *cp;
			break;
		    }
		    cp++;
		    termc = *cp;
		    if (!wantData)
			break;
		    Buf_AddByte(buf, (Byte)'\0');
		    cp2 = (char *)Buf_GetAll(buf, (int *)NULL);
		    newStr = VarRunShellCmd(cp2);
		    Buf_Destroy(buf);

		    break;
		}
		case 'N':
		case 'M':
		{
		    char    *pattern;
		    char    *cp2;
		    Boolean copy;

		    copy = FALSE;
		    for (cp = tstr + 1;
			 *cp != '\0' && *cp != ':' && *cp != endc;
			 cp++)
		    {
			if (*cp == '\\' && (cp[1] == ':' || cp[1] == endc)){
			    copy = TRUE;
			    cp++;
			}
		    }
		    termc = *cp;
		    if (!wantData)
			break;
		    *cp = '\0';
		    if (copy) {
			/*
			 * Need to compress the \:'s out of the pattern, so
			 * allocate enough room to hold the uncompressed
			 * pattern (note that cp started at tstr+1, so
			 * cp - tstr takes the null byte into account) and
			 * compress the pattern into the space.
			 */
			pattern = emalloc(cp - tstr);
			for (cp2 = pattern, cp = tstr + 1;
			     *cp != '\0';
			     cp++, cp2++)
			{
			    if ((*cp == '\\') &&
				(cp[1] == ':' || cp[1] == endc)) {
				    cp++;
			    }
			    *cp2 = *cp;
			}
			*cp2 = '\0';
		    } else {
			pattern = &tstr[1];
		    }
		    if (*tstr == 'M') {
			newStr = VarModify(str, VarMatch, (ClientData)pattern);
		    } else {
			newStr = VarModify(str, VarNoMatch,
					   (ClientData)pattern);
		    }
		    if (copy) {
			free(pattern);
		    }
		    break;
		}
		case 'S':
		{
		    VarPattern 	    pattern;
		    register char   delim;
		    Buffer  	    buf;    	/* Buffer for patterns */
		    register char   *cp2;
		    int	    	    lefts;

		    pattern.flags = 0;
		    delim = tstr[1];
		    tstr += 2;
		    /*
		     * If pattern begins with '^', it is anchored to the
		     * start of the word -- skip over it and flag pattern.
		     */
		    if (*tstr == '^') {
			pattern.flags |= VAR_MATCH_START;
			tstr += 1;
		    }

		    if (wantData)
			buf = Buf_Init(0);
		    
		    /*
		     * Pass through the lhs looking for 1) escaped delimiters,
		     * '$'s and backslashes (place the escaped character in
		     * uninterpreted) and 2) unescaped $'s that aren't before
		     * the delimiter (expand the variable substitution).
		     * The result is left in the Buffer buf.
		     */
		    for (cp = tstr; *cp != '\0' && *cp != delim; cp++) {
			if ((*cp == '\\') &&
			    ((cp[1] == delim) ||
			     (cp[1] == '$') ||
			     (cp[1] == '\\')))
			{
			    if (wantData)
				Buf_AddByte(buf, (Byte)cp[1]);
			    cp++;
			} else if (*cp == '$') {
			    if (cp[1] != delim) {
				/*
				 * If unescaped dollar sign not before the
				 * delimiter, assume it's a variable
				 * substitution and recurse.
				 */
				char	    *cp2;
				int	    len;
				Boolean	    freeIt;
				
				cp2 = VarParse(cp, ctxt, err, wantData,
					       &len, &freeIt);
				if (wantData)
				    Buf_AddBytes(buf, strlen(cp2),
						 (Byte *)cp2);
				if (freeIt) {
				    free(cp2);
				}
				cp += len - 1;
			    } else {
				/*
				 * Unescaped $ at end of pattern => anchor
				 * pattern at end.
				 */
				pattern.flags |= VAR_MATCH_END;
			    }
			} else if (wantData) {
			    Buf_AddByte(buf, (Byte)*cp);
			}
		    }

		    if (wantData)
			Buf_AddByte(buf, (Byte)'\0');
		    
		    /*
		     * If lhs didn't end with the delimiter, complain and
		     * return NULL
		     */
		    if (*cp != delim) {
			*lengthPtr = cp - start + 1 - lenAdjust;
			if (*freePtr) {
			    free(str);
			}
			if (namebuf != NULL)
			    free(namebuf);
			if (wantData)
			    Buf_Destroy(buf);
			Error("Unclosed substitution for %s (%c missing)",
			      v->name, delim);
			return (var_Error);
		    }

		    /*
		     * Fetch pattern and destroy buffer, but preserve the data
		     * in it, since that's our lhs. Note that Buf_GetAll
		     * will return the actual number of bytes, which includes
		     * the null byte, so we have to decrement the length by
		     * one.
		     */
		    if (wantData) {
			pattern.lhs = strdup((char *)Buf_GetAll(buf,
							 &pattern.leftLen));
			pattern.leftLen--;
			Buf_Destroy(buf);
		    }

		    /*
		     * Now comes the replacement string. Three things need to
		     * be done here: 1) need to compress escaped delimiters and
		     * ampersands and 2) need to replace unescaped ampersands
		     * with the l.h.s. (since this isn't regexp, we can do
		     * it right here) and 3) expand any variable substitutions.
		     */
		    if (wantData)
			buf = Buf_Init(0);
		    
		    tstr = cp + 1;
		    for (cp = tstr; *cp != '\0' && *cp != delim; cp++) {
			if ((*cp == '\\') &&
			    ((cp[1] == delim) ||
			     (cp[1] == '&') ||
			     (cp[1] == '\\') ||
			     (cp[1] == '$')))
			{
			    if (wantData)
				Buf_AddByte(buf, (Byte)cp[1]);
			    cp++;
			} else if ((*cp == '$') && (cp[1] != delim)) {
			    char    *cp2;
			    int	    len;
			    Boolean freeIt;

			    cp2 = VarParse(cp, ctxt, err, wantData,
					   &len, &freeIt);
			    if (wantData)
				Buf_AddBytes(buf, strlen(cp2), (Byte *)cp2);
			    cp += len - 1;
			    if (freeIt) {
				free(cp2);
			    }
			} else if (*cp == '&') {
			    if (wantData)
				Buf_AddBytes(buf, pattern.leftLen,
					     (Byte *)pattern.lhs);
			} else if (wantData) {
				Buf_AddByte(buf, (Byte)*cp);
			}
		    }

		    if (wantData)
			Buf_AddByte(buf, (Byte)'\0');
		    
		    /*
		     * If didn't end in delimiter character, complain
		     */
		    if (*cp != delim) {
			*lengthPtr = cp - start + 1 - lenAdjust;
			if (*freePtr) {
			    free(str);
			}
			if (namebuf != NULL)
			    free(namebuf);
			if (wantData)
			    Buf_Destroy(buf);
			Error("Unclosed substitution for %s (%c missing)",
			      v->name, delim);
			return (var_Error);
		    }

		    if (wantData) {
			pattern.rhs = strdup((char *)Buf_GetAll(buf,
							 &pattern.rightLen));
			pattern.rightLen--;
			Buf_Destroy(buf);
		    }

		    /*
		     * Check for global substitution. If 'g' after the final
		     * delimiter, substitution is global and is marked that
		     * way.
		     */
		    cp++;
		    if (*cp == 'g') {
			pattern.flags |= VAR_SUB_GLOBAL;
			cp++;
		    }

		    termc = *cp;
		    if (!wantData)
			break;

		    newStr = VarModify(str, VarSubstitute,
				       (ClientData)&pattern);
		    /*
		     * Free the two strings.
		     */
		    free(pattern.lhs);
		    free(pattern.rhs);
		    break;
		}
		case 'T':
		case 'H':
		case 'E':
		case 'R':
		    if (tstr[1] == endc || tstr[1] == ':') {
			Boolean (*modProc)();

			cp = tstr + 1;
			termc = *cp;
			if (!wantData)
			    break;
			switch (*tstr) {
			    case 'T':
				modProc = VarTail;
				break;
			    case 'H':
				modProc = VarHead;
				break;
			    case 'E':
				modProc = VarSuffix;
				break;
			    case 'R':
				modProc = VarRoot;
				break;
			}
			newStr = VarModify (str, modProc, (ClientData)0);
			break;
		    }
		    /*FALLTHRU*/
		default: {
		    /*
		     * This can either be a bogus modifier or a System-V
		     * substitution command.
		     */
		    VarPattern      pattern;
		    Boolean         eqFound;
		    
		    pattern.flags = 0;
		    eqFound = FALSE;
		    /*
		     * First we make a pass through the string trying
		     * to verify it is a SYSV-make-style translation:
		     * it must be: <string1>=<string2>)
		     */
		    for (cp = tstr; *cp != '\0' && *cp != endc; cp++) {
			if (*cp == '=') {
			    eqFound = TRUE;
			    /* continue looking for endc */
			}
		    }
		    if (*cp == endc && eqFound) {
			
			if (!wantData) {
			    termc = endc;
			    break;
			}

			/*
			 * Now we break this sucker into the lhs and
			 * rhs. We must null terminate them of course.
			 */
			for (cp = tstr; *cp != '='; cp++) {
			    ;
			}
			pattern.lhs = tstr;
			pattern.leftLen = cp - tstr;
			*cp++ = '\0';
			
			pattern.rhs = cp;
			while (*cp != endc) {
			    cp++;
			}
			pattern.rightLen = cp - pattern.rhs;
			*cp = '\0';
			
			/*
			 * SYSV modifications happen through the whole
			 * string. Note the pattern is anchored at the end.
			 */
			pattern.flags |= VAR_SUB_GLOBAL|VAR_MATCH_END;

			newStr = VarModify(str, VarSubstitute,
					   (ClientData)&pattern);

			/*
			 * Restore the nulled characters
			 */
			pattern.lhs[pattern.leftLen] = '=';
			pattern.rhs[pattern.rightLen] = endc;
			termc = endc;
		    } else {
			Error ("Unknown modifier '%c'\n", *tstr);
			for (cp = tstr+1;
			     *cp != ':' && *cp != endc && *cp != '\0';
			     cp++) {
				 ;
			}
			termc = *cp;
			newStr = var_Error;
		    }
		}
	    }
	    if (wantData && DEBUG(VAR)) {
		printf("Result is \"%s\"\n", newStr);
	    }
	    
	    if (wantData && str != newStr) {
		if (*freePtr) {
		    free (str);
		}
		str = newStr;
		if (str != var_Error) {
		    *freePtr = TRUE;
		} else {
		    *freePtr = FALSE;
		}
	    }
	    if (termc == '\0') {
		if (wantData)
		    Error("Unclosed variable specification for %s", v->name);
		else
		    Error("Unclosed variable specification");
	    } else if (termc == ':') {
		*cp++ = termc;
	    } else {
		*cp = termc;
	    }
	    tstr = cp;
	}
	*lengthPtr = tstr - start + 1 - lenAdjust;
    } else {
	*lengthPtr = tstr - start + 1 - lenAdjust;
	*tstr = endc;
    }
    
    if (!wantData) {
	if (namebuf != NULL)
	    free(namebuf);
	return(NULL);
    }

    if (v->flags & VAR_KEEP) {
	free((Address)v->name);
	free((Address)v);
	if (dynamic) {
	    if (*freePtr)
		free(str);
	    str = emalloc(*lengthPtr + 1 + lenAdjust);
	    strncpy(str, start, *lengthPtr + lenAdjust);
	    str[*lengthPtr + lenAdjust] = '\0';
	    *freePtr = TRUE;
	}
    } else if (v->flags & VAR_JUNK) {
	/*
	 * Perform any free'ing needed and set *freePtr to FALSE so the caller
	 * doesn't try to free a static pointer.
	 */
	if (*freePtr) {
	    free(str);
	}
	*freePtr = FALSE;
	free((Address)v->name);
	free((Address)v);
	if (dynamic) {
	    str = emalloc(*lengthPtr + 1 + lenAdjust);
	    strncpy(str, start, *lengthPtr + lenAdjust);
	    str[*lengthPtr + lenAdjust] = '\0';
	    *freePtr = TRUE;
	} else {
	    str = var_Error;
	}
    }
    if (namebuf != NULL)
	free(namebuf);

    if (DEBUG(VAR)) {
	printf("VarParse: returning %s\n", str);
    }
    return (str);
}

/*-
 *-----------------------------------------------------------------------
 * Var_Parse --
 *	Given the start of a variable invocation, extract the variable
 *	name and find its value, then modify it according to the
 *	specification.
 *
 * Results:
 *	The (possibly-modified) value of the variable or var_Error if the
 *	specification is invalid. The length of the specification is
 *	placed in *lengthPtr (for invalid specifications, this is just
 *	2...?).
 *	A Boolean in *freePtr telling whether the returned string should
 *	be freed by the caller.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
char *
Var_Parse (str, ctxt, err, lengthPtr, freePtr)
    char    	  *str;	    	/* The string to parse */
    GNode   	  *ctxt;    	/* The context for the variable */
    Boolean 	    err;    	/* TRUE if undefined variables are an error */
    int	    	    *lengthPtr;	/* OUT: The length of the specification */
    Boolean 	    *freePtr; 	/* OUT: TRUE if caller should free result */
{
    return(VarParse (str, ctxt, err, TRUE, lengthPtr, freePtr));
}

/*-
 *-----------------------------------------------------------------------
 * Var_Skip --
 *	Given the start of a variable invocation, extract the variable
 *	name and find its value, then modify it according to the
 *	specification.
 *
 * Results:
 *	The (possibly-modified) value of the variable or var_Error if the
 *	specification is invalid. The length of the specification is
 *	placed in *lengthPtr (for invalid specifications, this is just
 *	2...?).
 *	A Boolean in *freePtr telling whether the returned string should
 *	be freed by the caller.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
char *
Var_Skip (str, ctxt, err)
    char    	  *str;	    	/* The string to parse */
    GNode   	  *ctxt;    	/* The context for the variable */
    Boolean 	    err;    	/* TRUE if undefined variables are an error */
{
    int	    	    length;	/* The length of the specification */
    Boolean 	    freeit; 	/* TRUE if caller should free result */
    char	   *nstr;

    nstr = VarParse (str, ctxt, err, FALSE, &length, &freeit);
    if (nstr == var_Error || nstr == varNoError)
	printf("Var_Skip: Error\n");
    else if (nstr != NULL)
	printf("Var_Skip: Data %s\n", nstr);
    str += length;
    if (freeit)
	free(nstr);
    return(str);
}

/*-
 *-----------------------------------------------------------------------
 * Var_Subst  --
 *	Substitute for all variables in the given string in the given context
 *	If undefErr is TRUE, Parse_Error will be called when an undefined
 *	variable is encountered.
 *
 * Results:
 *	The resulting string.
 *
 * Side Effects:
 *	None. The old string must be freed by the caller
 *-----------------------------------------------------------------------
 */
char *
Var_Subst (str, ctxt, undefErr)
    register char *str;	    	    /* the string in which to substitute */
    GNode         *ctxt;	    /* the context wherein to find variables */
    Boolean 	  undefErr; 	    /* TRUE if undefineds are an error */
{
    Buffer  	  buf;	    	    /* Buffer for forming things */
    char    	  *val;		    /* Value to substitute for a variable */
    int	    	  length;   	    /* Length of the variable invocation */
    Boolean 	  doFree;   	    /* Set true if val should be freed */
    static Boolean errorReported;   /* Set true if an error has already
				     * been reported to prevent a plethora
				     * of messages when recursing */

    buf = Buf_Init (0);
    errorReported = FALSE;

    while (*str) {
	if ((*str == '$') && (str[1] == '$')) {
	    /*
	     * A dollar sign may be escaped either with another dollar sign.
	     * In such a case, we skip over the escape character and store the
	     * dollar sign into the buffer directly.
	     */
	    str++;
	    Buf_AddByte(buf, (Byte)*str);
	    str++;
	} else if (*str != '$') {
	    /*
	     * Skip as many characters as possible -- either to the end of
	     * the string or to the next dollar sign (variable invocation).
	     */
	    char  *cp;

	    for (cp = str++; *str != '$' && *str != '\0'; str++) {
		;
	    }
	    Buf_AddBytes(buf, str - cp, (Byte *)cp);
	} else {
	    val = VarParse (str, ctxt, undefErr, TRUE, &length, &doFree);

	    /*
	     * When we come down here, val should either point to the
	     * value of this variable, suitably modified, or be NULL.
	     * Length should be the total length of the potential
	     * variable invocation (from $ to end character...)
	     */
	    if (val == var_Error || val == varNoError) {
		/*
		 * If performing old-time variable substitution, skip over
		 * the variable and continue with the substitution. Otherwise,
		 * store the dollar sign and advance str so we continue with
		 * the string...
		 */
		if (oldVars) {
		    str += length;
		} else if (undefErr) {
		    /*
		     * If variable is undefined, complain and skip the
		     * variable. The complaint will stop us from doing anything
		     * when the file is parsed.
		     */
		    if (!errorReported) {
			Parse_Error (PARSE_FATAL,
				     "Undefined variable \"%.*s\"",length,str);
		    }
		    str += length;
		    errorReported = TRUE;
		} else {
		    Buf_AddByte (buf, (Byte)*str);
		    str += 1;
		}
	    } else {
		/*
		 * We've now got a variable structure to store in. But first,
		 * advance the string pointer.
		 */
		str += length;
		
		/*
		 * Copy all the characters from the variable value straight
		 * into the new string.
		 */
		Buf_AddBytes (buf, strlen (val), (Byte *)val);
		if (doFree) {
		    free ((Address)val);
		}
	    }
	}
    }
	
    Buf_AddByte (buf, '\0');
    str = strdup((char *)Buf_GetAll (buf, (int *)NULL));
    Buf_Destroy (buf);
    return (str);
}

/*-
 *-----------------------------------------------------------------------
 * Var_GetTail --
 *	Return the tail from each of a list of words. Used to set the
 *	System V local variables.
 *
 * Results:
 *	The resulting string.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
char *
Var_GetTail(file)
    char    	*file;	    /* Filename to modify */
{
    return(VarModify(file, VarTail, (ClientData)0));
}

/*-
 *-----------------------------------------------------------------------
 * Var_GetHead --
 *	Find the leading components of a (list of) filename(s).
 *	XXX: VarHead does not replace foo by ., as (sun) System V make
 *	does.
 *
 * Results:
 *	The leading components.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
char *
Var_GetHead(file)
    char    	*file;	    /* Filename to manipulate */
{
    return(VarModify(file, VarHead, (ClientData)0));
}

/*
 * The following array is used to make a fast determination of which
 * characters are interpreted specially by the shell.  If a command
 * contains any of these characters, it is executed by the shell, not
 * directly by us.
 */

STATIC char 	    meta[256];

/*-
 *-----------------------------------------------------------------------
 * Var_Init --
 *	Initialize the module
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	The VAR_CMD and VAR_GLOBAL contexts are created 
 *-----------------------------------------------------------------------
 */
void
Var_Init ()
{
    char    	  *cp, *vp;
    char	 **pp;
    Hash_Entry	  *he;
    extern char  **environ;

    VAR_GLOBAL = Targ_NewGN ("Global");
    VAR_CMD = Targ_NewGN ("Command");
    VAR_ENV = Targ_NewGN ("Environment");
    Hash_InitTable(&cmdHashTable, 32);
    Hash_InitTable(&globalHashTable, 256);
    Hash_InitTable(&envHashTable, 64);
    for (pp = environ; *pp; pp++) {
	cp = strdup(*pp);
	vp = index(cp, '=');
	if (vp) {
	    *vp++ = '\0';
	    VarAdd(cp, vp, VAR_ENV);
	}
	(void) free(cp);
    }

    for (cp = "#=|^(){};&<>*?[]:$`\\\n"; *cp != '\0'; cp++) {
	meta[*cp] = 1;
    }
    /*
     * The null character serves as a sentinel in the string.
     */
    meta[0] = 1;
}

/*-
 *-----------------------------------------------------------------------
 * Var_HasMeta --
 *	Check if string contains a meta character.
 *
 * Results:
 *	0 if there are no meta characters, 1 if there are.
 *
 * Side Effects:
 *	none
 *
 *-----------------------------------------------------------------------
 */
Var_HasMeta (cmd)
    char    	  *cmd;	    	/* Command to check */
{
    register char *cp;
    Boolean quote;

    /*
     * Search for meta characters in the command. If there are no meta
     * characters, there's no need to execute a shell to execute the
     * command.
     */
    quote = FALSE;
    for (cp = cmd; *cp; cp++) {
	if (quote) {
	    quote = FALSE;
	    continue;
	}
	if (*cp == '\\') {
	    quote = TRUE;
	    continue;
	}
	if (meta[*cp])
	    return (1);
    }

    if (quote)
	return (1);
    return (0);
}

/****************** PRINT DEBUGGING INFO *****************/
STATIC
VarPrintVar (v)
    Var            *v;
{
    printf ("%-16s = %s\n", v->name, Buf_GetAll(v->val, (int *)NULL));
    return (0);
}

/*-
 *-----------------------------------------------------------------------
 * Var_Dump --
 *	print all variables in a context
 *-----------------------------------------------------------------------
 */
Var_Dump (ctxt)
    GNode          *ctxt;
{
    Hash_Table *ht;
    Hash_Entry *he;
    Hash_Search hs;

    if ((ht = VarHashTable(ctxt)) == NULL) {
	Lst_ForEach (ctxt->context, VarPrintVar);
	return;
    }
    he = Hash_EnumFirst(ht, &hs);
    for (;;) {
	if (he == NULL)
	    return;
	VarPrintVar(Hash_GetValue(he));
	he = Hash_EnumNext(&hs);
    }
}
