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
 * $Log:	main.c,v $
 * Revision 2.4  93/05/08  00:20:22  mrt
 * 	Use HOST_MACHINE instead of MACHINE to set the MACHINE global
 * 	variable, to avoid conflicts with this parameter and the value
 * 	set in machparam.h
 * 	[93/04/30            mrt]
 * 
 * Revision 2.3  93/01/31  22:08:33  mrt
 * 	Fixed it so that VPATH elements could be separated by spaces as
 * 	well as :. 
 * 	[92/08/21            mrt]
 * 
 * Revision 2.2  92/05/20  20:14:01  mrt
 * 	Added a Var_Set call for .MAKE since it was being used
 * 	by brk_string but never getting set.
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
char copyright[] =
"@(#) Copyright (c) 1989 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)main.c	5.24 (Berkeley) 2/2/91";
#endif /* not lint */

/*-
 * main.c --
 *	The main file for this entire program. Exit routines etc
 *	reside here.
 *
 * Utility functions defined in this file:
 *	Main_ParseArgLine	Takes a line of arguments, breaks them and
 *				treats them as if they were given when first
 *				invoked. Used by the parse module to implement
 *				the .MFLAGS target.
 *
 *	Error			Print a tagged error message. The global
 *				MAKE variable must have been defined. This
 *				takes a format string and two optional
 *				arguments for it.
 *
 *	Fatal			Print an error message and exit. Also takes
 *				a format string and two arguments.
 *
 *	Punt			Aborts all jobs and exits with a message. Also
 *				takes a format string and two arguments.
 *
 *	Finish			Finish things up by printing the number of
 *				errors which occured, as passed to it, and
 *				exiting.
 */

#include <sys/param.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <varargs.h>
#include "make.h"
#include "buf.h"
#include "pathnames.h"

extern int errno;

extern void free();

#ifndef PATH_MAX
#define PATH_MAX MAXPATHLEN
#endif

#define	MAKEFLAGS	".MAKEFLAGS"

Lst			create;		/* Targets to be made */
time_t			now;		/* Time at start of make */
GNode			*DEFAULT;	/* .DEFAULT node */
Boolean			allPrecious;	/* .PRECIOUS given on line by itself */

static Boolean		noBuiltins;	/* -r flag */
static Lst		makefiles;	/* ordered list of makefiles to read */
int			maxJobs;	/* -J argument */
static int		maxLocal;	/* -L argument */
Boolean			debug;		/* -d flag */
Boolean			noExecute;	/* -n flag */
Boolean			keepgoing;	/* -k flag */
Boolean			queryFlag;	/* -q flag */
Boolean			touchFlag;	/* -t flag */
Boolean			ignoreErrors;	/* -i flag */
Boolean			beSilent;	/* -s flag */
Boolean			oldVars;	/* variable substitution style */
Boolean			checkEnvFirst;	/* -e flag */
Boolean			noisy;		/* -N flag */
static Boolean		jobsRunning;	/* TRUE if the jobs might be running */

static Boolean		ReadMakefile();

static char *curdir;			/* if chdir'd for an architecture */

extern char *getenv();

/*-
 * MainParseArgs --
 *	Parse a given argument vector. Called from main() and from
 *	Main_ParseArgLine() when the .MAKEFLAGS target is used.
 *
 *	XXX: Deal with command line overriding .MAKEFLAGS in makefile
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Various global and local flags will be set depending on the flags
 *	given
 */
static void
MainParseArgs(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	extern char *optarg;
	register int i;
	register char *cp;
	int c;

	optind = 1;	/* since we're called more than once */
rearg:	while((c = getopt(argc, argv, "D:I:L:NPSd:ef:ij:knqrst")) != EOF) {
		switch(c) {
		case 'D':
			Var_Set(optarg, "1", VAR_GLOBAL);
			Var_Append(MAKEFLAGS, "-D", VAR_GLOBAL);
			Var_Append(MAKEFLAGS, optarg, VAR_GLOBAL);
			break;
		case 'I':
			Parse_AddIncludeDir(optarg);
			Var_Append(MAKEFLAGS, "-I", VAR_GLOBAL);
			Var_Append(MAKEFLAGS, optarg, VAR_GLOBAL);
			break;
		case 'L':
			maxLocal = atoi(optarg);
			Var_Append(MAKEFLAGS, "-L", VAR_GLOBAL);
			Var_Append(MAKEFLAGS, optarg, VAR_GLOBAL);
			break;
		case 'N':
			noisy = TRUE;
			Var_Append(MAKEFLAGS, "-N", VAR_GLOBAL);
			break;
		case 'S':
			keepgoing = FALSE;
			Var_Append(MAKEFLAGS, "-S", VAR_GLOBAL);
			break;
		case 'd': {
			char *modules = optarg;

			for (; *modules; ++modules)
				switch (*modules) {
				case 'A':
					debug = ~0;
					break;
				case 'a':
					debug |= DEBUG_ARCH;
					break;
				case 'c':
					debug |= DEBUG_COND;
					break;
				case 'd':
					debug |= DEBUG_DIR;
					break;
				case 'g':
					if (modules[1] == '1') {
						debug |= DEBUG_GRAPH1;
						++modules;
					}
					else if (modules[1] == '2') {
						debug |= DEBUG_GRAPH2;
						++modules;
					}
					break;
				case 'j':
					debug |= DEBUG_JOB;
					break;
				case 'm':
					debug |= DEBUG_MAKE;
					break;
				case 's':
					debug |= DEBUG_SUFF;
					break;
				case 't':
					debug |= DEBUG_TARG;
					break;
				case 'v':
					debug |= DEBUG_VAR;
					break;
				}
			Var_Append(MAKEFLAGS, "-d", VAR_GLOBAL);
			Var_Append(MAKEFLAGS, optarg, VAR_GLOBAL);
			break;
		}
		case 'e':
			checkEnvFirst = TRUE;
			Var_Append(MAKEFLAGS, "-e", VAR_GLOBAL);
			break;
		case 'f':
			(void)Lst_AtEnd(makefiles, (ClientData)optarg);
			break;
		case 'i':
			ignoreErrors = TRUE;
			Var_Append(MAKEFLAGS, "-i", VAR_GLOBAL);
			break;
		case 'j':
			maxJobs = atoi(optarg);
			Var_Append(MAKEFLAGS, "-j", VAR_GLOBAL);
			Var_Append(MAKEFLAGS, optarg, VAR_GLOBAL);
			break;
		case 'k':
			keepgoing = TRUE;
			Var_Append(MAKEFLAGS, "-k", VAR_GLOBAL);
			break;
		case 'n':
			noExecute = TRUE;
			Var_Append(MAKEFLAGS, "-n", VAR_GLOBAL);
			break;
		case 'q':
			queryFlag = TRUE;
			/* Kind of nonsensical, wot? */
			Var_Append(MAKEFLAGS, "-q", VAR_GLOBAL);
			break;
		case 'r':
			noBuiltins = TRUE;
			Var_Append(MAKEFLAGS, "-r", VAR_GLOBAL);
			break;
		case 's':
			beSilent = TRUE;
			Var_Append(MAKEFLAGS, "-s", VAR_GLOBAL);
			break;
		case 't':
			touchFlag = TRUE;
			Var_Append(MAKEFLAGS, "-t", VAR_GLOBAL);
			break;
		default:
		case '?':
			usage();
		}
	}

	oldVars = TRUE;

	/*
	 * See if the rest of the arguments are variable assignments and
	 * perform them if so. Else take them to be targets and stuff them
	 * on the end of the "create" list.
	 */
	for (argv += optind, argc -= optind; *argv; ++argv, --argc)
		if (Parse_IsVar(*argv))
			Parse_DoVar(*argv, VAR_CMD);
		else {
			if (!**argv)
				Punt("illegal (null) argument.");
			if (**argv == '-') {
				optind = 0;
				goto rearg;
			}
			(void)Lst_AtEnd(create, (ClientData)*argv);
		}
}

/*-
 * Main_ParseArgLine --
 *  	Used by the parse module when a .MFLAGS or .MAKEFLAGS target
 *	is encountered and by main() when reading the .MAKEFLAGS envariable.
 *	Takes a line of arguments and breaks it into its
 * 	component words and passes those words and the number of them to the
 *	MainParseArgs function.
 *	The line should have all its leading whitespace removed.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Only those that come from the various arguments.
 */
void
Main_ParseArgLine(line)
	char *line;			/* Line to fracture */
{
	char **argv;			/* Manufactured argument vector */
	int argc;			/* Number of arguments in argv */

	if (line == NULL)
		return;
	for (; *line == ' '; ++line);
	if (!*line)
		return;

	argv = brk_string(line, &argc);
	MainParseArgs(argc, argv);
}

/*-
 * main --
 *	The main function, for obvious reasons. Initializes variables
 *	and a few modules, then parses the arguments give it in the
 *	environment and on the command line. Reads the system makefile
 *	followed by either Makefile, makefile or the file given by the
 *	-f argument. Sets the .MAKEFLAGS PMake variable based on all the
 *	flags it has received by then uses either the Make or the Compat
 *	module to create the initial list of targets.
 *
 * Results:
 *	If -q was given, exits -1 if anything was out-of-date. Else it exits
 *	0.
 *
 * Side Effects:
 *	The program exits when done. Targets are created. etc. etc. etc.
 */
main(argc, argv)
	int argc;
	char **argv;
{
	Lst targs;	/* target nodes to create -- passed to Make_Init */
	Boolean outOfDate; 	/* FALSE if all targets up to date */
	char *p;

	create = Lst_Init(FALSE);
	makefiles = Lst_Init(FALSE);
	beSilent = FALSE;		/* Print commands as executed */
	ignoreErrors = FALSE;		/* Pay attention to non-zero returns */
	noExecute = FALSE;		/* Execute all commands */
	keepgoing = FALSE;		/* Stop on error */
	allPrecious = FALSE;		/* Remove targets when interrupted */
	queryFlag = FALSE;		/* This is not just a check-run */
	noBuiltins = FALSE;		/* Read the built-in rules */
	touchFlag = FALSE;		/* Actually update targets */
	debug = 0;			/* No debug verbosity, please. */
	jobsRunning = FALSE;
	noisy = FALSE;

	maxJobs = 0;			/* Set default max concurrency */
	maxLocal = 0;			/* Set default local max concurrency */
    
	/*
	 * Initialize the parsing, directory and variable modules to prepare
	 * for the reading of inclusion paths and variable settings on the
	 * command line
	 */
	Dir_Init();		/* Initialize directory structures so -I flags
				 * can be processed correctly */
	Parse_Init();		/* Need to initialize the paths of #include
				 * directories */
	Var_Init();		/* As well as the lists of variables for
				 * parsing arguments */

	/*
	 * Initialize various variables.
	 *	MAKE also gets this name, for compatibility
	 *	.MAKEFLAGS gets set to the empty string just in case.
	 *	MFLAGS also gets initialized empty, for compatibility.
	 */
	Var_Set("MAKE", argv[0], VAR_GLOBAL);
	Var_Set(".MAKE", argv[0], VAR_GLOBAL);
	Var_Set(MAKEFLAGS, "", VAR_GLOBAL);
	Var_Set("MFLAGS", "", VAR_GLOBAL);
	Var_Set("MACHINE", HOST_MACHINE, VAR_GLOBAL);
	Var_Set("NPROC", (p = getenv("NPROC")) ? p : "1", VAR_GLOBAL);

	/*
	 * First snag any flags out of the MAKE environment variable.
	 * (Note this is *not* MAKEFLAGS since /bin/make uses that and it's
	 * in a different format).
	 */
	Main_ParseArgLine(getenv("MAKEFLAGS"));
    
	MainParseArgs(argc, argv);

	/*
	 * Now make sure that maxJobs and maxLocal are positive.  If
	 * not, use the value of NPROC (or 1 if NPROC is not positive)
	 */
	{
		int nproc = atoi(Var_Value("NPROC", VAR_GLOBAL));

		if (nproc < 1)
			nproc = 1;
		if (maxJobs < 1)
			maxJobs = nproc;
		if (maxLocal < 1)
			maxLocal = maxJobs;
	}

	/*
	 * Initialize archive, target and suffix modules in preparation for
	 * parsing the makefile(s)
	 */
	Arch_Init();
	Targ_Init();
	Suff_Init();

	DEFAULT = NILGNODE;
	(void)time(&now);

	/*
	 * Set up the .TARGETS variable to contain the list of targets to be
	 * created. If none specified, make the variable empty -- the parser
	 * will fill the thing in with the default or .MAIN target.
	 */
	if (!Lst_IsEmpty(create)) {
		LstNode ln;

		for (ln = Lst_First(create); ln != NILLNODE;
		    ln = Lst_Succ(ln)) {
			char *name = (char *)Lst_Datum(ln);

			Var_Append(".TARGETS", name, VAR_GLOBAL);
		}
	} else
		Var_Set(".TARGETS", "", VAR_GLOBAL);

	/*
	 * Locate the correct working directory for make.  Adjust for any
	 * movement taken from the directory of our parent if we are a
	 * sub-make.
	 */
	MakeSetWorkingDir("Makeconf");

	/*
	 * Read in the built-in rules first, followed by the specified
	 * makefile, if it was (makefile != (char *) NULL), or the default
	 * Makefile and makefile, in that order, if it wasn't.
	 */
	 if (!noBuiltins && !ReadMakefile(_PATH_DEFSYSMK))
		Fatal("make: no system rules (%s).", _PATH_DEFSYSMK);

	if (!Lst_IsEmpty(makefiles)) {
		LstNode ln;

		ln = Lst_Find(makefiles, (ClientData)NULL, ReadMakefile);
		if (ln != NILLNODE)
			Fatal("make: cannot open %s.", (char *)Lst_Datum(ln));
	} else if (!ReadMakefile("makefile"))
		(void)ReadMakefile("Makefile");

	(void)ReadMakefile(".depend");

	Var_Append("MFLAGS", Var_Value(MAKEFLAGS, VAR_GLOBAL), VAR_GLOBAL);

	/* Install all the flags into the MAKE envariable. */
	if ((p = Var_Value(MAKEFLAGS, VAR_GLOBAL)) && *p)
		setenv("MAKEFLAGS", p, 1);

	/*
	 * For compatibility, look at the directories in the VPATH variable
	 * and add them to the search path, if the variable is defined. The
	 * variable's value is in the same format as the PATH envariable, i.e.
	 * <directory>:<directory>:<directory>...
	 */
	if (Var_Exists("VPATH", VAR_CMD)) {
		char *vpath, *path, *cp, savec;
		/*
		 * GCC stores string constants in read-only memory, but
		 * Var_Subst will want to write this thing, so store it
		 * in an array
		 */
		static char VPATH[] = "${VPATH}";

		vpath = Var_Subst(VPATH, VAR_CMD, FALSE);
		path = vpath;
		do {
			/* skip to end of directory */
			for (cp = path; *cp != ':' && *cp != ' ' && *cp != '\0'; cp++);
			/* Save terminator character so know when to stop */
			savec = *cp;
			*cp = '\0';
			/* Add directory to search path */
			Dir_AddDir(dirSearchPath, path);
			*cp = savec;
			path = cp + 1;
		} while (savec == ':' || savec == ' ');
		(void)free((Address)vpath);
	}

	/*
	 * Now that all search paths have been read for suffixes et al, it's
	 * time to add the default search path to their lists...
	 */
	Suff_DoPaths();

	/* print the initial graph, if the user requested it */
	if (DEBUG(GRAPH1))
		Targ_PrintGraph(1);

	/*
	 * Have now read the entire graph and need to make a list of targets
	 * to create. If none was given on the command line, we consult the
	 * parsing module to find the main target(s) to create.
	 */
	if (Lst_IsEmpty(create))
		targs = Parse_MainName();
	else
		targs = Targ_FindList(create, TARG_CREATE);

	/*
	 * Initialize job module before traversing the graph, now that
	 * any .BEGIN and .END targets have been read.  This is done
	 * only if the -q flag wasn't given (to prevent the .BEGIN from
	 * being executed should it exist).
	 */
	if (!queryFlag) {
		Job_Init(maxJobs, maxLocal);
		jobsRunning = TRUE;
	}

	/* Traverse the graph, checking on all the targets */
	outOfDate = Make_Run(targs);
    
	/* print the graph now it's been processed if the user requested it */
	if (DEBUG(GRAPH2))
		Targ_PrintGraph(2);

	if (queryFlag && outOfDate)
		exit(1);
	else
		exit(0);
}

/*-
 * ReadMakefile  --
 *	Open and parse the given makefile.
 *
 * Results:
 *	TRUE if ok. FALSE if couldn't open file.
 *
 * Side Effects:
 *	lots
 */
static Boolean
ReadMakefile(fname)
	char *fname;		/* makefile to read */
{
	extern Lst parseIncPath, sysIncPath;
	FILE *stream;
	char *name;

	if (!strcmp(fname, "-")) {
		Var_Set("MAKEFILE", "", VAR_GLOBAL);
		Parse_File("(stdin)", stdin);
	} else {
		if (((name = Dir_FindFile(fname, dirSearchPath)) ||
		     (name = Dir_FindFile(fname, parseIncPath)) ||
		     (name = Dir_FindFile(fname, sysIncPath))) &&
		    (stream = fopen(name, "r")))
			fname = name;
		else
			return(FALSE);
		/*
		 * set the MAKEFILE variable desired by System V fans -- the
		 * placement of the setting here means it gets set to the last
		 * makefile specified, as it is set by SysV make.
		 */
		Var_Set("MAKEFILE", fname, VAR_GLOBAL);
		Parse_File(fname, stream);
		(void)fclose(stream);
	}
	return(TRUE);
}

/*-
 * Error --
 *	Print an error message given its format.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The message is printed.
 */
/* VARARGS */
void
Error(va_alist)
	va_dcl
{
	va_list ap;
	char *fmt;

	va_start(ap);
	fmt = va_arg(ap, char *);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	(void)fflush(stderr);
}

/*-
 * Fatal --
 *	Produce a Fatal error message. If jobs are running, waits for them
 *	to finish.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	The program exits
 */
/* VARARGS */
void
Fatal(va_alist)
	va_dcl
{
	va_list ap;
	char *fmt;

	if (jobsRunning)
		Job_Wait();

	va_start(ap);
	fmt = va_arg(ap, char *);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	(void)fflush(stderr);

	if (DEBUG(GRAPH2))
		Targ_PrintGraph(2);
	exit(2);		/* Not 1 so -q can distinguish error */
}

/*
 * Punt --
 *	Major exception once jobs are being created. Kills all jobs, prints
 *	a message and exits.
 *
 * Results:
 *	None 
 *
 * Side Effects:
 *	All children are killed indiscriminately and the program Lib_Exits
 */
/* VARARGS */
void
Punt(va_alist)
	va_dcl
{
	va_list ap;
	char *fmt;

	(void)fprintf(stderr, "make: ");
	va_start(ap);
	fmt = va_arg(ap, char *);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	(void)fflush(stderr);

	DieHorribly();
}

/*-
 * DieHorribly --
 *	Exit without giving a message.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	A big one...
 */
void
DieHorribly()
{
	if (jobsRunning)
		Job_AbortAll();
	if (DEBUG(GRAPH2))
		Targ_PrintGraph(2);
	exit(2);		/* Not 1, so -q can distinguish error */
}

/*
 * Finish --
 *	Called when aborting due to errors in child shell to signal
 *	abnormal exit. 
 *
 * Results:
 *	None 
 *
 * Side Effects:
 *	The program exits
 */
void
Finish(errors)
	int errors;	/* number of errors encountered in Make_Make */
{
	Fatal("%d error%s", errors, errors == 1 ? "" : "s");
}

/*
 * emalloc --
 *	malloc, but die on error.
 */
char *
emalloc(len)
	u_int len;
{
	char *p, *malloc();

	if (!(p = malloc(len)))
		enomem();
	return(p);
}

/*
 * enomem --
 *	die when out of memory.
 */
enomem()
{
	(void)fprintf(stderr, "make: %s.\n", strerror(errno));
	exit(2);
}

/*
 * usage --
 *	exit with usage message
 */
usage()
{
	(void)fprintf(stderr,
"usage: make [-eiknqrst] [-D variable] [-d flags] [-f makefile ]\n\t\
[-I directory] [-j max_jobs] [variable=value]\n");
	exit(2);
}

static char OBJECTS[]	= "MAKEOBJDIR";
static char SOURCES[]	= "MAKESRCDIRPATH";
static char MAKEDIR[]	= "MAKEDIR";
static char MAKETOP[]	= "MAKETOP";
static char MAKESUB[]	= "MAKESUB";
static char MAKECONF[]	= "_MAKECONF";
static char MAKECWD[]	= "_MAKECWD";
static char MAKEPSD[]	= "_MAKEPSD";
static char MAKEIOD[]	= "_MAKEIOD";

MakeSetWorkingDir(cf)
	char *cf;
{
	char *owd, *psd, cwd[PATH_MAX+1];

	if (getcwd(cwd, sizeof(cwd)) == 0 || *cwd != '/')
		Fatal("getwd: %s", strerror(errno));
	if ((owd = getenv(MAKECWD)) && (psd = getenv(MAKEPSD))) {
		if (*owd != '/')
			Fatal("make: bad %s environment variable", MAKECWD);
		if (*psd == '\0')
			Fatal("make: bad %s environment variable", MAKEPSD);
		movedmake(cwd, owd, psd);
	} else
		plainmake(cwd, cf);
}

/*
 * find and read the configuration file and move
 * to the object tree
 */
plainmake(cwd, cf)
	char *cwd, *cf;
{
	register char *d, *p, *q;
	int n;
	char *pname;
	struct stat sb;

	pname = q = NULL;
	if (stat(cf, &sb) == 0) {
		pname = strdup(cf);
	} else if (cwd[1] != '\0') {
		n = 0;
		for (p = cwd; *p != '\0'; p++)
			if (*p == '/')
				n++;
		p = d = emalloc((n * 3) + strlen(cf) + 1);
		q = cwd;
		while (*q != '\0')
			if (*q++ == '/') {
				*p++ = '.';
				*p++ = '.';
				*p++ = '/';
			}
		(void) strcpy(p, cf);
		q = p;
		for (;;) {
			q -= 3;
			if (stat(q, &sb) == 0) {
				pname = strdup(q);
				*p = '\0';
				break;
			}
			if (q == d) {
				q = NULL;
				break;
			}
		}
	}
	confmove(cwd, pname, q);
}

/*
 * we may have changed directories, so we calulate possible difference
 */
movedmake(cwd, owd, psd)
	char *cwd, *owd, *psd;
{
	register char *p, *q, *r, *s;
	register char **v;
	char *ep;
	struct stat sb;
	int cnt;
	Lst newsd;
	LstNode ln;
	Buffer buf = Buf_Init(0);

	/*
	 * Locate the difference between the current working directory
	 * and the previous working directory.
	 */
	for (p = r = owd, q = s = cwd; *r && *r == *s; r++, s++)
		if (*r == '/') {
			while (*(r+1) == '/')
				r++;
			while (*(s+1) == '/')
				s++;
			p = r + 1;
			q = s + 1;
		}
	if (*r == 0 && *s == 0) {
		p = r;
		q = s;
	} else if (*r == '/' && *s == 0) {
		while (*++r && *r == '/')
			;
		p = r;
		q = s;
	} else if (*r == 0 && *s == '/') {
		p = r;
		while (*++s && *s == '/')
			;
		q = s;
	}

	ptol(psd, &newsd);
	if (Lst_Open(newsd) != SUCCESS)
		Fatal("movedmake: Lst_Open");
	while ((ln = Lst_Next(newsd)) != NILLNODE) {
		s = (char *) Lst_Datum(ln);
		if (*s != '/') {
			rdircat(q, p, buf);
			if (Buf_Size(buf) != 0) {
				r = (char *)Buf_GetAll(buf, &cnt);
				if (r[cnt-1] != '/')
					Buf_AddByte(buf, (Byte)'/');
			}
		}
		Buf_AddBytes(buf, strlen(s), s);
		if ((*p || *q) && *s && Buf_Size(buf) != 0) {
			r = (char *)Buf_GetAll(buf, &cnt);
			if (r[cnt-1] != '/')
				Buf_AddByte(buf, (Byte)'/');
		}
		rdircat(p, q, buf);
		r = strdup((char *)Buf_GetAll(buf, (int *)NULL));
		Buf_Discard(buf, Buf_Size(buf));
		Str_Flatten(r);
		Lst_Replace(ln, (ClientData)r);
	}
	Lst_Close(newsd);
	if (ep = getenv(MAKEDIR)) {
		while (*ep == '/')
			ep++;
		Buf_AddByte(buf, (Byte)'/');
		Buf_AddBytes(buf, strlen(ep), ep);
		if (*p || *q) {
			r = (char *)Buf_GetAll(buf, &cnt);
			if (r[cnt-1] != '/')
				Buf_AddByte(buf, (Byte)'/');
		}
	} else
		Buf_AddByte(buf, (Byte)'/');
	rdircat(p, q, buf);
	r = strdup((char *)Buf_GetAll(buf, (int *)NULL));
	Buf_Discard(buf, Buf_Size(buf));
	Str_Flatten(r);
	setenv(MAKEDIR, r, 1);
	if (DEBUG(VAR)) {
		printf("%s=%s\n", MAKEDIR, r);
	}
	setpathvars(r);
	(void) free(r);
	if (ep = getenv(MAKECONF)) {
		if (stat(ep, &sb) == 0)
			(void) ReadMakefile(ep);
	}
	if (!didchdir())
		Var_Delete(OBJECTS, VAR_GLOBAL);
	setenv(MAKECWD, cwd, 1);
	if (DEBUG(VAR)) {
		printf("%s=%s\n", MAKECWD, cwd);
	}
	ltop(newsd, buf);
	r = (char *)Buf_GetAll(buf, (int *)NULL);
	setenv(MAKEPSD, r, 1);
	if (DEBUG(VAR)) {
		printf("%s=%s\n", MAKEPSD, r);
	}
	Buf_Destroy(buf);
	Dir_ReInit(newsd);
	Lst_Destroy(newsd, free);
}

setpathvars(dir)
	char *dir;
{
	register char *p, *q;
	int cnt;
	Buffer buf = Buf_Init(0);

	Var_Set(MAKEDIR, dir, VAR_GLOBAL);
	while (*dir == '/')
		dir++;
	q = dir;
	while (*q) {
		if (*q == '.' && strncmp(q, "../", 3) == 0)
			Fatal("%s: bad MAKEDIR", dir);
		if (*q++ == '/') {
			while (*q == '/')
				q++;
		} else {
			Buf_AddBytes(buf, 3, "../");
			while (*q && *q != '/')
				q++;
		}
	}
	p = (char *)Buf_GetAll(buf, (int *)NULL);
	Var_Set(MAKETOP, p, VAR_GLOBAL);
	Buf_Discard(buf, Buf_Size(buf));
	Buf_AddBytes(buf, strlen(dir), dir);
	if (Buf_Size(buf) != 0) {
		p = (char *)Buf_GetAll(buf, &cnt);
		if (p[cnt-1] != '/')
			Buf_AddByte(buf, (Byte)'/');
	}
	p = (char *)Buf_GetAll(buf, &cnt);
	Var_Set(MAKESUB, p, VAR_GLOBAL);
	Buf_Destroy(buf);
}

confmove(cwd, cf, pre)
	char *cwd;
	char *cf;
	char *pre;
{
	register char *src, *obj, *r;
	register char *vp;
	char *suf;
	int cnt;
	struct stat sb;
	Lst newsd;
	LstNode ln;
	Buffer buf = Buf_Init(0);

	Buf_AddByte(buf, (Byte)'/');
	if (pre && *pre) {
		suf = cwd + strlen(cwd);
		for (r = pre;
		     *r == '.' && *(r+1) == '.' && *(r+2) == '/';
		     r += 3) {
			while (*(suf-1) == '/')
				suf--;
			if (suf == cwd || suf == cwd + 1) {
				suf = 0;
				break;
			}
			while (*(suf-1) != '/')
				suf--;
		}
		if (suf && *suf) {
			suf = strdup(suf);
			Buf_AddBytes(buf, strlen(suf), suf);
		} else
			suf = 0;
	} else {
		pre = 0;
		suf = 0;
	}
	src = (char *)Buf_GetAll(buf, (int *)NULL);
	setenv(MAKEDIR, src, 1);
	if (DEBUG(VAR)) {
		printf("%s=%s\n", MAKEDIR, src);
	}
	setpathvars(src);
	Buf_Discard(buf, Buf_Size(buf));
	if (cf) {
		if (*cf != '/') {
			Buf_AddBytes(buf, strlen(cwd), cwd);
			Buf_AddByte(buf, (Byte)'/');
		}
		Buf_AddBytes(buf, strlen(cf), cf);
		r = strdup((char *)Buf_GetAll(buf, (int *)NULL));
		Buf_Discard(buf, Buf_Size(buf));
		Str_Flatten(r);
		setenv(MAKECONF, r, 1);
		if (DEBUG(VAR)) {
			printf("%s=%s\n", MAKECONF, r);
		}
		Var_Set(MAKECONF, r, VAR_GLOBAL);
		(void) ReadMakefile(r);
		(void) free(r);
	}
	obj = NULL;
	if (vp = Var_Value(OBJECTS, VAR_GLOBAL)) {
		if (index (vp, '$') != (char *)NULL)
			vp = Var_Subst(vp, VAR_GLOBAL, FALSE);
		else
			vp = strdup(vp);
		if (DEBUG(VAR)) {
			printf("< %s=%s\n", OBJECTS, vp);
		}
		if (index(vp, ':'))
			Fatal("%s: only one component allowed", OBJECTS);
		if (*vp == '\0') {
			Var_Delete(OBJECTS, VAR_GLOBAL);
		} else {
			if (*vp != '/' && pre)
				Buf_AddBytes(buf, strlen(pre), pre);
			Buf_AddBytes(buf, strlen(vp), vp);
			r = (char *)Buf_GetAll(buf, &cnt);
			if (stat(r, &sb) < 0)
				Fatal("No such directory: %s", r);
			if (suf) {
				if (Buf_Size(buf) != 0 && r[cnt-1] != '/')
					Buf_AddByte(buf, (Byte)'/');
				Buf_AddBytes(buf, strlen(suf), suf);
			}
			obj = strdup((char *)Buf_GetAll(buf, &cnt));
			Buf_Discard(buf, Buf_Size(buf));
			Str_Flatten(obj);
			if (DEBUG(VAR)) {
				printf("> %s=%s\n", OBJECTS, obj);
			}
		}
	}
	if (vp = Var_Value(SOURCES, VAR_GLOBAL)) {
		if (index (vp, '$') != (char *)NULL)
			vp = Var_Subst(vp, VAR_GLOBAL, FALSE);
		else
			vp = strdup(vp);
		if (DEBUG(VAR)) {
			printf("< %s=%s\n", SOURCES, vp);
		}
		if (*vp == '\0')
			vp = NULL;
		else {
			ptol(vp, &newsd);
			(void) fixvar(newsd, pre, suf);
			if (DEBUG(VAR)) {
				ltop(newsd, buf);
				r = (char *)Buf_GetAll(buf, (int *)NULL);
				printf("> %s=%s\n", SOURCES, r);
				Buf_Discard(buf, Buf_Size(buf));
			}
		}
	}
	Buf_Destroy(buf);
	if (vp == NULL)
		newsd = Lst_Init(FALSE);
	Lst_AtFront(newsd, (ClientData) strdup(""));
	if (obj || Lst_First(newsd) != Lst_Last(newsd))
		reldir(cwd, obj, newsd);
	Lst_Destroy(newsd, free);
}

fixvar(l, pre, suf)
	Lst l;
	char *pre, *suf;
{
	register char *p;
	int cnt;
	Buffer buf = Buf_Init(0);
	LstNode ln;

	if (Lst_Open(l) != SUCCESS)
		Fatal("fixvar: Lst_Open");
	while ((ln = Lst_Next(l)) != NILLNODE) {
		p = (char *) Lst_Datum(ln);
		if (*p != '/' && pre)
			Buf_AddBytes(buf, strlen(pre), pre);
		Buf_AddBytes(buf, strlen(p), p);
		if (suf) {
			if (Buf_Size(buf) != 0) {
				p = (char *)Buf_GetAll(buf, &cnt);
				if (p[cnt-1] != '/')
					Buf_AddByte(buf, (Byte)'/');
			}
			Buf_AddBytes(buf, strlen(suf), suf);
		}
		p = strdup((char *)Buf_GetAll(buf, (int *)NULL));
		Buf_Discard(buf, Buf_Size(buf));
		Str_Flatten(p);
		Lst_Replace(ln, (ClientData)p);
	}
	Lst_Close(l);
}

/*
 * find names for the source directories after a
 * chdir(obj) and use them to adjust search path
 */
reldir(cwd, obj, newsd)
	char *cwd, *obj;
	Lst newsd;
{
	register char *h, *t;
	register char *p, *q, *r;
	char lastc;
	char *twd;
	int cnt;
	LstNode ln;
	Buffer buf = Buf_Init(0);

	if (Lst_Open(newsd) != SUCCESS)
		Fatal("reldir: Lst_Open");
	while ((ln = Lst_Next(newsd)) != NILLNODE) {
		p = (char *) Lst_Datum(ln);
		if (*p == '/' || obj == NULL)
			continue;
		Buf_AddBytes(buf, strlen(cwd), cwd);
		if (*obj == '/') {
			r = (char *)Buf_GetAll(buf, &cnt);
			if (r[cnt-1] != '/')
				Buf_AddByte(buf, (Byte)'/');
			Buf_AddBytes(buf, strlen(p), p);
			r = strdup((char *)Buf_GetAll(buf, (int *)NULL));
			Buf_Discard(buf, Buf_Size(buf));
			Str_Flatten(r);
			Lst_Replace(ln, (ClientData)r);
			continue;
		}
		r = (char *)Buf_GetAll(buf, &cnt);
		twd = emalloc(cnt + strlen(obj) + 2);
		bcopy(r, twd, cnt + 1);
		Buf_Discard(buf, Buf_Size(buf));
		r = twd + cnt - 1;
		lastc = '\0';
		for (h = t = obj; *h; h = t) {
			while (*t && *t != '/')
				t++;
			if (t == h + 1 && *h == '.') {
				while (*t == '/')
					t++;
			}
			if (lastc != '\0') {
				Buf_AddByte(buf, (Byte)'/');
				lastc = '/';
			}
			if (t == h + 2 && *h == '.' && *(h+1) == '.') {
				if (r == twd) {
					if (lastc != '/')
						Buf_AddByte(buf, (Byte)'/');
					break;
				}
				while (*r != '/') {
					Buf_AddByte(buf, (Byte)*r);
					lastc = *r--;
				}
				while (r != twd && *r == '/')
					--r;
			} else {
				*++r = '/';
				while (h != t)
					*++r = *h++;
				Buf_AddByte(buf, (Byte)'.');
				Buf_AddByte(buf, (Byte)'.');
				lastc = '.';
			}
			while (*t == '/')
				t++;
		}
		(void)free(twd);
		q = (char *)Buf_GetAll(buf, &cnt);
		t = q + cnt - 1;
		r = emalloc(cnt + strlen(p) + 2);
		h = r;
		while (cnt--) {
			*h++ = *t--;
		}
		Buf_Discard(buf, Buf_Size(buf));
		if (h != r && *(h-1) != '/')
			*h++ = '/';
		(void) strcpy(h, p);
		Str_Flatten(r);
		Lst_Replace(ln, (ClientData)r);
	}
	Lst_Close(newsd);
	makechdir(obj, cwd, newsd);
	Dir_ReInit(newsd);
}

makechdir(obj, cwd, newsd)
	register char *obj;
	char *cwd;
	Lst newsd;
{
	char wd[PATH_MAX+1];
	char *p;
	Buffer buf = Buf_Init(0);

	if (obj) {
		makepath(obj, (char *)Lst_Datum(Lst_First(newsd)));
		if (!beSilent)
			printf("cd %s\n", obj);
		if (chdir(obj) == -1)
			Fatal("%s: %s", obj, strerror(errno));
		Var_Set(".CURDIR", cwd, VAR_GLOBAL);
		cwd = wd;
		if (getcwd(wd, sizeof(wd)) == 0 || *cwd != '/')
			Fatal("getwd: %s", strerror(errno));
		setenv(MAKEIOD, MAKEIOD, 1);
	} else
		Var_Set(".CURDIR", ".", VAR_GLOBAL);
	setenv(MAKECWD, cwd, 1);
	if (DEBUG(VAR)) {
		printf("%s=%s\n", MAKECWD, cwd);
	}
	ltop(newsd, buf);
	p = (char *)Buf_GetAll(buf, (int *)NULL);
	setenv(MAKEPSD, p, 1);
	if (DEBUG(VAR)) {
		printf("%s=%s\n", MAKEPSD, p);
	}
	Buf_Destroy(buf);
}

didchdir()
{
	register char *p;

	return (p = getenv(MAKEIOD)) != 0 && strcmp(p, MAKEIOD) == 0;
}

makepath(dir, src)
	char *dir, *src;
{
	register char *p, *q, *r, *s;
	register char c, c1;
	int len;
	struct stat sb;

	len = strlen(src) + 1;
	if (*src != '/')
		len += strlen(dir) + 1;
	r = src;
	p = src = emalloc(len);
	if (*r != '/') {
		for (q = dir; *q; *p++ = *q++)
			;
		*p++ = '/';
	}
	for (q = r; *p = *q; p++, q++)
		;
	Str_Flatten(src);
	p = dir + strlen(dir) - 1;
	q = src + strlen(src) - 1;
	r = p;
	s = q;
	while (p > dir && q > src && *--p == *--q) {
		if (*p != '/')
			continue;
		r = p + 1;
		s = q + 1;
		while (p > dir && *(p - 1) == '/')
			--p;
		while (q > src && *(q - 1) == '/')
			--q;
	}
	for (q = dir; *q == '/'; q++)
		;
	while (*q) {
		while (*++q && *q != '/')
			;
		c = *q;
		*q = 0;
		if (q > r)
			while (*s && *s != '/')
				s++;
		if (stat(dir, &sb) == -1) {
			sb.st_mode = 0777;
			if (q > r) {
				c1 = *s;
				*s = 0;
				if (stat(src, &sb) != -1 && DEBUG(VAR))
					printf("using mode of %s\n", src);
				*s = c1;
			}
			if (!beSilent)
				printf("mkdir %s\n", dir);
			if (mkdir(dir, (int) sb.st_mode & 0777) == -1)
				Fatal("Couldn't make directory: %s", dir);
		} else if ((sb.st_mode & S_IFMT) != S_IFDIR)
			Fatal("Not a directory: %s", dir);
		if (q > r)
			while (*s == '/')
				s++;
		*q = c;
		while (*q == '/')
			q++;
	}
}

/*
 * "rev(a) | '/' | b" into buf
 */
rdircat(a, b, buf)
	char *a, *b;
	Buffer buf;
{
	register char *p;

	for (p = a; *p; ) {
		if (*p == '/' ||
		    (*p == '.' &&
		     (*(p+1) == 0 || *(p+1) == '/' ||
		      (*(p+1) == '.' && (*(p+2) == 0 || *(p+2) == '/')))))
			Fatal("bad directory: %s", a);
		while (*p && *p++ != '/')
			;
		Buf_AddByte(buf, (Byte)'.');
		Buf_AddByte(buf, (Byte)'.');
		if (*p || *b != 0)
			Buf_AddByte(buf, (Byte)'/');
	}
	if (*b != 0)
		Buf_AddBytes(buf, strlen(b), b);
}

ptol(p, lp)
	register char *p;
	register Lst *lp;
{
	char *q;
	Lst l = Lst_Init(FALSE);

	*lp = l;
	for (;;) {
		q = p;
		while (*p && *p != ':')
			p++;
		if (*p == 0)
			break;
		*p++ = 0;
		(void) Lst_AtEnd(l, (ClientData) strdup(q));
	}
	(void) Lst_AtEnd(l, (ClientData) strdup(q));
}

ltop(l, buf)
	register Lst l;
	register Buffer buf;
{
	register char *q;
	LstNode ln;

	if (Lst_Open(l) != SUCCESS)
		Fatal("ltop: Lst_Open");
	if ((ln = Lst_Next(l)) != NILLNODE)
		for (;;) {
			q = (char *) Lst_Datum(ln);
			Buf_AddBytes(buf, strlen(q), q);
			if ((ln = Lst_Next(l)) == NILLNODE)
				break;
			Buf_AddByte(buf, (Byte)':');
		}
	Lst_Close(l);
}
