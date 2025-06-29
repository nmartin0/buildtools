/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
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
 * $Log:	md.c,v $
 * Revision 2.6  93/05/07  23:57:00  mrt
 * 	Renamed max name in struct table to maxargc to avoid conflict
 * 	with BNR2 param.h file.
 * 	[93/05/04            mrt]
 * 
 * Revision 2.5  93/03/20  00:06:12  mrt
 * 	Added include of strings.h to quiet gcc warnings.
 * 	[93/03/09            mrt]
 * 
 * Revision 2.4  93/01/31  22:09:05  mrt
 * 	Fixed to accept more than one target name in a .d file.
 * 	[92/12/18            mrt]
 * 
 * Revision 2.3  92/07/08  18:05:34  mrt
 * 	Added Bob Baron's version of ffilecopy that works under BNR
 * 	[92/06/24            mrt]
 * 
 * Revision 2.2  92/05/20  20:15:19  mrt
 * 	Added a check for the <file>.o: that CMU's cpp and gcc-cpp
 * 	puts on the start of each line.
 * 	Added code to check for duplicate entries. This is currently
 * 	done in the most inefficient manner possible.
 * 	[92/05/08            mrt]
 * 
 * 	Changed to use doprnt only if NO_VFPRINTF is defined.
 * 	[92/04/20            mrt]
 * 
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile: md.c,v $ $Revision: 2.6 $ (OSF) $Date: 93/05/07 23:57:00 $";
#endif

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <strings.h>
#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <stdio.h>

extern int errno;
#ifdef NO_DIRENT
#define dirent direct
#endif
#ifdef _BLD
char _argbreak;
#else
extern char _argbreak;
#endif

extern char *index();
extern char *rindex();
#if __STDC__
char *concat(char *buf, int buflen, ...);
#else
extern char *concat();
#endif
extern char *salloc();
extern char *calloc();
extern char *realloc();
extern char *nxtarg();
extern char *malloc();
extern char *errmsg();

#if __STDC__
static diag(char *fmt, ...);
static ewarn(char *fmt, ...);
static efatal(char *fmt, ...);
static warn(char *fmt, ...);
static fatal(char *fmt, ...);
#endif

#ifndef S_ISDIR
#define	S_ISDIR(m)	(((m)&S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define	S_ISREG(m)	(((m)&S_IFMT) == S_IFREG)
#endif

#define FALSE	0
#define TRUE	1

struct table {
    int maxargc;		/* max argc before realloc of argv */
    int argc;			/* current argument count */
    char **argv;		/* arguments */
};

struct target {
    char *name;
    struct table curdeps;
    struct table stddeps;
};

char *progname;

struct table curincdirs;
struct table stdincdirs;

struct table keepincdirs;

struct table dependfiles;

struct table targets;

int depfile_bufsize;
char *depfile_buffer;

char *outfile = "depend.mk";
int debug = FALSE;
int rmflag = FALSE;
int stdflag = FALSE;
int verbose = FALSE;

main(argc, argv)
int argc;
char **argv;
{

    /*
     * parse command line
     */
    parse_command_line(argc, argv);

    /*
     * process dependency files
     */
    read_dependencies();

    /*
     * update output file
     */
    update_database();

    /*
     * remove dependency files
     */
    remove_dependencies();

    exit(0);
}

resize_table(tp)
struct table *tp;
{
    if (tp->maxargc == 0) {
	tp->maxargc = 4;
	tp->argv = (char **) malloc((unsigned) tp->maxargc * sizeof(char *));
    } else {
	tp->maxargc <<= 1;
	tp->argv = (char **) realloc((char *) tp->argv,
				     (unsigned) tp->maxargc * sizeof(char *));
    }
    if (tp->argv == NULL)
	fatal("table argv malloc failed");
}

/*
 * canonicalize path - similar to abspath
 */
canonicalize(base, relpath, outbuf, outbuf_size)
char *base;
char *relpath;
char *outbuf;
int outbuf_size;
{
    char *from;
    char *to;
    char *slash;
    char *peek;

    /*
     * concatenate parts of path into buffer
     */
    if (concat(outbuf, outbuf_size, base, "/", relpath, "/", NULL) == NULL) {
	fprintf(stderr, "%s: path length exceeds buffer size\n");
	return(-1);
    }

    /*
     * remember position of first slash
     */
    slash = index(outbuf, '/');
    from = to = slash + 1;

    /*
     * canonicalize the path
     */
    while (*from != '\0') {
	if ((*to++ = *from++) != '/')
	    continue;
	peek = to-2;
	if (*peek == '/') {
	    /*
	     * found "//", back up one
	     */
	    to--;
	    continue;
	}
	if (*peek != '.')
	    continue;
	peek--;
	if (*peek == '/') {
	    /*
	     * found "/./", back up two
	     */
	    to -= 2;
	    continue;
	}
	if (*peek != '.')
	    continue;
	peek--;
	if (*peek != '/')
	    continue;
	/*
	 * found "/../", try to remove preceding token
	 */
	if (peek == slash) {
	    /*
	     * hit the "first" slash, update to not remove any more tokens
	     */
	    slash = to-1;
	    continue;
	}
	/*
	 * backup one token 
	 */
	while (*--peek != '/')
	    ;
	to = peek+1;
    }
    *to-- = '\0';
    if (to > outbuf && *to == '/')
	*to = '\0';
}


parse_command_line(argc, argv)
int argc;
char **argv;
{
    struct table *tp;
    struct stat st;
    DIR *dirp;
    struct dirent *dp;
    char buf[MAXPATHLEN];
    char *p;
    int l;

    if (argc > 0) {
	if ((progname = rindex(argv[0], '/')) != NULL)
	    progname++;
	else
	    progname = argv[0];
	argc--; argv++;
    } else
	progname = "md";

    bzero((char *)&curincdirs, sizeof(curincdirs));
    bzero((char *)&stdincdirs, sizeof(stdincdirs));
    tp = &curincdirs;

    while (argc > 0) {
	if (argv[0][0] != '-')
	    break;
	switch (argv[0][1]) {
	case 'I':
	    if (strcmp(argv[0], "-I-") == 0) {
		tp = &stdincdirs;
		break;
	    }
	    if (tp->argc == tp->maxargc)
		resize_table(tp);
	    p = (argv[0]) + 2;
	    canonicalize("", p, buf, sizeof(buf));
	    if (*p == '/')
		p = buf;
	    else
		p = buf + 1;
	    p = salloc(p);
	    if (p == NULL)
		efatal("-I salloc failed");
	    tp->argv[tp->argc++] = p;
	    break;
	case 'K':
	    if (keepincdirs.argc == keepincdirs.maxargc)
		resize_table(&keepincdirs);
	    p = (argv[0]) + 2;
	    canonicalize("", p, buf, sizeof(buf));
	    if (*p == '/')
		p = buf;
	    else
		p = buf + 1;
	    p = salloc(p);
	    if (p == NULL)
		efatal("-K salloc failed");
	    keepincdirs.argv[keepincdirs.argc++] = p;
	    break;
	case 'd':
	    if (strcmp(argv[0], "-debug") == 0)
		debug = TRUE;
	    else {
		warn("unknown switch: %s", argv[0]);
		print_usage();
	    }
	    break;
	case 'f':
	    if (strcmp(argv[0], "-file") == 0) {
		if (argc == 1)
		    fatal("missing argument to %s", argv[0]);
		argc--; argv++;
		outfile = argv[0];
	    } else {
		warn("unknown switch: %s", argv[0]);
		print_usage();
	    }
	    break;
	case 'r':
	    if (strcmp(argv[0], "-rm") == 0)
		rmflag = TRUE;
	    else {
		warn("unknown switch: %s", argv[0]);
		print_usage();
	    }
	    break;
	case 's':
	    if (strcmp(argv[0], "-std") == 0)
		stdflag = TRUE;
	    else {
		warn("unknown switch: %s", argv[0]);
		print_usage();
	    }
	    break;
	case 'v':
	    if (strcmp(argv[0], "-verbose") == 0)
		verbose = TRUE;
	    else if (strcmp(argv[0], "-version") == 0) {
		print_revision();
		exit(0);
	    } else {
		warn("unknown switch: %s", argv[0]);
		print_usage();
	    }
	    break;
	default:
	    warn("unknown switch: %s", argv[0]);
	    print_usage();
	}
	argc--; argv++;
    }

    if (argc == 0)
	print_usage();

    if (tp == &curincdirs && tp->argc > 0) {
	bcopy((char *)&curincdirs, &stdincdirs, sizeof(curincdirs));
	bzero((char *)&curincdirs, sizeof(curincdirs));
    }

    bzero((char *)&dependfiles, sizeof(dependfiles));
    tp = &dependfiles;

    while (argc > 0) {
	if (stat(argv[0], &st) < 0)
	    efatal("stat %s", argv[0]);
	if (S_ISREG(st.st_mode)) {
	    if (tp->argc == tp->maxargc)
		resize_table(tp);
	    tp->argv[tp->argc++] = argv[0];
	    argc--; argv++;
	    continue;
	}
	if (!S_ISDIR(st.st_mode))
	    fatal("%s: Not a directory or regular file", argv[0]);
	if ((dirp = opendir(argv[0])) == NULL)
	    efatal("opendir %s", argv[0]);
/* error */ 
	while ((dp = readdir(dirp)) != NULL) {
	    p = dp->d_name;
	    l = strlen(p);
	    if (l < 2 || p[l-2] != '.' || p[l-1] != 'd')
		continue;
	    if (concat(buf, sizeof(buf), argv[0], "/", p, NULL) == NULL)
		fatal("no room in buffer for %s/%s", argv[0], p);
	    p = salloc(buf);
	    if (p == NULL)
		efatal("salloc dirent name");
	    if (tp->argc == tp->maxargc)
		resize_table(tp);
	    tp->argv[tp->argc++] = p;
	}
	(void) closedir(dirp);
	argc--; argv++;
    }

    /*
     * print tables
     */
    if (verbose) {
	print_table(&curincdirs);
	print_table(&stdincdirs);
	print_table(&dependfiles);
    }
}

print_table(tp)
struct table *tp;
{
    int i;

    printf("table %x maxargc %d argc %d\n", tp, tp->maxargc, tp->argc);
    for (i = 0; i < tp->argc; i++)
	printf("  argv[%d] = %s\n", i, tp->argv[i]);
}

shorten_name(depbuf)
char *depbuf;
{
    char buf[MAXPATHLEN];
    char *ptr;
    int l1, l2, lmax;
    int maxcur, maxstd;
    int i;
    char *dep;

    canonicalize("", depbuf, buf, sizeof(buf));
    if (*depbuf == '/')
	dep = buf;
    else
	dep = buf + 1;
    l1 = strlen(dep);
    lmax = -1;
    maxcur = 0;
    for (i = 0; i < curincdirs.argc; i++) {
	ptr = curincdirs.argv[i];
	l2 = strlen(ptr);
	if (l1 < l2 || l2 <= lmax)
	    continue;
	if (l2 == 0) {
	    if (*dep == '/')
		continue;
	} else {
	    if (l1 > l2 && dep[l2] != '/')
		continue;
	    if (bcmp(ptr, dep, l2) != 0)
		continue;
	}
	lmax = l2;
	maxcur = 1;
    }
    maxstd = 0;
    for (i = 0; i < stdincdirs.argc; i++) {
	ptr = stdincdirs.argv[i];
	l2 = strlen(ptr);
	if (l1 < l2 || l2 <= lmax)
	    continue;
	if (l2 == 0) {
	    if (*dep == '/')
		continue;
	} else {
	    if (l1 > l2 && dep[l2] != '/')
		continue;
	    if (bcmp(ptr, dep, l2) != 0)
		continue;
	}
	lmax = l2;
	maxstd = 1;
    }
    if (lmax == -1)
	return(-1);
    for (i = 0; i < keepincdirs.argc; i++) {
	ptr = keepincdirs.argv[i];
	l2 = strlen(ptr);
	if (l2 == lmax && bcmp(ptr, dep, l2) == 0) {
	    (void) strcpy(depbuf, dep);
	    return(maxstd ? 1 : 0);
	}
    }
    ptr = dep + lmax;
    while (*ptr == '/')
	ptr++;
    (void) strcpy(depbuf, ptr);
    return(maxstd ? 1 : 0);
}

read_target_dependencies(file)
char *file;
{
    char buf[MAXPATHLEN];
    int fd;
    char *ptr, *dep;
    struct stat st;
    struct target *targ;
    struct table *tp;
    int i, found;
    int cnt;
    int status;

    targ = (struct target *) calloc(1, sizeof(struct target));
    if (targ == NULL)
	efatal("target calloc failed");
    if ((fd = open(file, O_RDONLY, 0)) < 0)
	efatal("open %s", file);
    if (fstat(fd, &st) < 0)
	efatal("fstat %s", file);
    if (st.st_size >= depfile_bufsize) {
	depfile_bufsize += 1024;
	if (st.st_size >= depfile_bufsize)
	    depfile_bufsize = st.st_size + 1;
	depfile_buffer = realloc(depfile_buffer, depfile_bufsize);
	if (depfile_buffer == NULL)
	    efatal("depfile_buffer realloc failed");
    }
    ptr = depfile_buffer;
    while (st.st_size > 0) {
	if ((cnt = read(fd, ptr, st.st_size)) < 0)
	    efatal("read %s", file);
	if (cnt == 0)
	    fatal("premature EOF");
	st.st_size -= cnt;
	ptr += cnt;
    }
    *ptr = '\0';
    if (close(fd) < 0)
	efatal("close %s", file);
    ptr = depfile_buffer;
    targ->name = nxtarg(&ptr, " \t:");
    if (*targ->name == '\0') {
	diag("%s: no dependencies", file);
	canonicalize("", file, buf, sizeof(buf));
	dep = rindex(buf, '/');
	targ->name = salloc(dep + 1);
	if (targ->name == NULL)
	    efatal("target name salloc failed");
	dep = targ->name + strlen(targ->name);
	while (dep > targ->name && *(dep-1) != '.')
	    dep--;
	if (*dep == '\0') {
	    diag("%s: unable to generate filename");
	    return;
	}
	*dep = 'o';
    } else {
	targ->name = salloc(targ->name);
	if (targ->name == NULL)
	    efatal("target name salloc failed");
	if (_argbreak != ':') {
	    while (*ptr == ' ' || *ptr == '\t')
		ptr++;
	    if (*ptr != ':') {
		diag("%s: missing colon in dependency file", file);
		return;
	    }
	    ptr++;
	}
    }
    while (*ptr != '\0') {
	dep = nxtarg(&ptr, " :\t\\\n");
	if (*dep == '\0')
	    continue;
	/* skip the additional file.o: lines that CMU cpp puts on each line */
	if ((_argbreak == ' ') && (*ptr == ':'))  {
		ptr++; 
		_argbreak = ':';
	}
	if (_argbreak == ':' ) 
	{
	    if ( strcmp(dep,targ->name) ) {
		/* new target in this file */
		if (targets.argc == targets.maxargc)
		resize_table(&targets);
    		targets.argv[targets.argc++] = (char *)targ;
		targ = (struct target *) calloc(1, sizeof(struct target));
		if (targ == NULL)
		    efatal("target calloc failed");
		targ->name = salloc(dep);
		if (targ->name == NULL)
	    	    efatal("target name salloc failed");
	    }
	    continue;
	}
	status = shorten_name(dep);
	if (status < 0) {
	    warn("%s: unable to shorten '%s' - skipped", file, dep);
	    continue;
	}
	dep = salloc(dep);
	if (dep == NULL)
	    efatal("dependent salloc failed");
	tp = (status == 0 ? &targ->curdeps : &targ->stddeps);
	/* now check to see if we already have seen this dependency */
	found = 0;
	for (i=0; i < tp->argc; i++) {
	    if ( strcmp(dep,tp->argv[i]) == 0 ) {
		found = 1;
		break;
	    }
	}
	if (!found) {
	    if (tp->argc == tp->maxargc)
		resize_table(tp);
	    tp->argv[tp->argc++] = dep;
	}
    }
    if (targets.argc == targets.maxargc)
	resize_table(&targets);
    targets.argv[targets.argc++] = (char *)targ;
}

read_dependencies()
{
    char *file;
    int i;

    depfile_bufsize = (8*1024)+1;
    depfile_buffer = malloc(depfile_bufsize);
    if (depfile_buffer == NULL)
	efatal("depfile_buffer realloc failed");
    for (i = 0; i < dependfiles.argc; i++) {
	file = dependfiles.argv[i];
	read_target_dependencies(file);
    }
}

cmp_targets(t1, t2)
struct target **t1, **t2;
{
    return(strcmp((*t1)->name, (*t2)->name));
}

compare_targets(t1, t2)
char *t1, *t2;
{
    char *p;
    int cmp;

    p = t1 + strlen(t1) - 1;
    if (*p-- != '\n')
	return(-1);
    if (*p != ':')
	return(-1);
    *p = '\0';
    cmp = strcmp(t1, t2);
    *p = ':';
    return(cmp);
}

update_database()
{
    char buf[MAXPATHLEN];
    char oldbuf[MAXPATHLEN];
    char buffer[BUFSIZ];
    FILE *inf, *outf;
    int i, j;
    char *p1, *p2;
    struct target *targ;
    int need_header;
    int eof;
    int cmp;

    if (targets.argc == 0)
	return;
    qsort(targets.argv, targets.argc, sizeof(char *), cmp_targets);
if (debug) {
for (i = 0; i < targets.argc; i++) {
targ = (struct target *) targets.argv[i];
printf("curdeps[%d]\n", i);
print_table(&targ->curdeps);
printf("curdeps[%d]\n", i);
print_table(&targ->stddeps);
}
}
    (void) concat(buf, sizeof(buf), outfile, ".tmp", NULL);
    (void) unlink(buf);
    if ((outf = fopen(buf, "w")) == NULL) {
	ewarn("unable to open %s for write", buf);
	(void) unlink(buf);
	exit(1);
    }
    if ((inf = fopen(outfile, "r")) == NULL) {
	if (errno != ENOENT) {
	    ewarn("unable to open %s for read", outfile);
	    (void) fclose(outf);
	    (void) unlink(buf);
	    exit(1);
	}
	for (i = 0; i < targets.argc; i++)
	    output_target(outf, (struct target *) targets.argv[i], TRUE);
	if (ferror(outf) || fclose(outf) == EOF) {
	    ewarn("error writing %s", buf);
	    (void) fclose(outf);
	    (void) unlink(buf);
	    exit(1);
	}
	if (rename(buf, outfile) < 0)
	    efatal("rename %s to %s", buf, outfile);
	return;
    }
    i = 0;
    targ = (struct target *) targets.argv[i];
    while ((fgets(buffer, sizeof(buffer), inf)) != NULL) {
	if (bcmp("# dependents of ", buffer, 16) != 0) {
	    (void) fputs(buffer, outf);
	    continue;
	}
	cmp = compare_targets(buffer + 16, targ->name);
	if (cmp < 0) {
	    (void) fputs(buffer, outf);
	    continue;
	}
	if (cmp == 0) {
	    (void) fputs(buffer, outf);
	    eof = TRUE;
	    while ((fgets(buffer, sizeof(buffer), inf)) != NULL)
		if (bcmp("# dependents of ", buffer, 16) == 0) {
		    eof = FALSE;
		    break;
		}
	} else {
	    eof = FALSE;
	    (void) fputs("# dependents of ", outf);
	    (void) fputs(targ->name, outf);
	    (void) fputs(":\n", outf);
	}
	need_header = FALSE;
	for (;;) {
	    output_target(outf, targ, need_header);
	    if (++i == targets.argc) {
		if (!eof) {
		    (void) fputs("#\n", outf);
		    (void) fputs(buffer, outf);
		    ffilecopy(inf, outf);
		}
		break;
	    }
	    need_header = TRUE;
	    targ = (struct target *) targets.argv[i];
	    if (eof)
		continue;
	    cmp = compare_targets(buffer + 16, targ->name);
	    if (cmp > 0)
		continue;
	    if (cmp < 0) {
		(void) fputs("#\n", outf);
		(void) fputs(buffer, outf);
		break;
	    }
	    (void) fputs("#\n", outf);
	    (void) fputs(buffer, outf);
	    eof = TRUE;
	    while ((fgets(buffer, sizeof(buffer), inf)) != NULL)
		if (bcmp("# dependents of ", buffer, 16) == 0) {
		    eof = FALSE;
		    break;
		}
	    need_header = FALSE;
	}
	if (i == targets.argc)
	    break;
    }
    while (i < targets.argc)
	output_target(outf, (struct target *) targets.argv[i++], TRUE);
    if (ferror(inf) || fclose(inf) == EOF) {
	ewarn("error reading %s", outfile);
	(void) fclose(inf);
	(void) fclose(outf);
	(void) unlink(buf);
	exit(1);
    }
    if (ferror(outf) || fclose(outf) == EOF) {
	ewarn("error writing %s", buf);
	(void) fclose(outf);
	(void) unlink(buf);
	exit(1);
    }
    (void) concat(oldbuf, sizeof(oldbuf), outfile, ".old", NULL);
    (void) unlink(oldbuf);
    if (link(outfile, oldbuf) < 0) {
	ewarn("link %s to %s", outfile, oldbuf);
	(void) unlink(buf);
	exit(1);
    }
    if (rename(buf, outfile) < 0) {
	ewarn("rename %s to %s", buf, outfile);
	exit(1);
    }
}

output_target(outf, targ, need_header)
FILE *outf;
struct target *targ;
int need_header;
{
    int i;

    if (need_header) {
	(void) fputs("#\n", outf);
	(void) fputs("# dependents of ", outf);
	(void) fputs(targ->name, outf);
	(void) fputs(":\n", outf);
    }
    for (i = 0; i < targ->curdeps.argc; i++) {
	(void) fputs(targ->name, outf);
	(void) fputs(": ", outf);
	(void) fputs(targ->curdeps.argv[i], outf);
	(void) putc('\n', outf);
    }
    if (!stdflag)
	return;
    for (i = 0; i < targ->stddeps.argc; i++) {
	(void) fputs("# ", outf);
	(void) fputs(targ->name, outf);
	(void) fputs(": <", outf);
	(void) fputs(targ->stddeps.argv[i], outf);
	(void) fputs(">\n", outf);
    }
}

remove_dependencies()
{
    char *file;
    int i;

    for (i = 0; i < dependfiles.argc; i++) {
	file = dependfiles.argv[i];
	if (rmflag && unlink(file) < 0)
	    ewarn("unlink %s", file);
    }
}

/*
 * error routines
 */

/*VARARGS1*/
#if __STDC__
diag(char *fmt, ...)
#else
diag(fmt, va_alist)
char *fmt;
va_dcl
#endif
{
    va_list vargs;

    (void) fflush(stdout);
#if __STDC__
    va_start(vargs, fmt);
#else
    va_start(vargs);
#endif
#ifdef _BLD
#ifdef NO_VFPRINTF
    (void) _doprnt(fmt, vargs, stderr);
#else
    (void) vfprintf(stderr, fmt, vargs);
#endif
#else
    (void) vfprintf(stderr, fmt, vargs);
#endif
    va_end(vargs);
    (void) putc('\n', stderr);
}

/*VARARGS1*/
#if __STDC__
ewarn(char *fmt, ...)
#else
ewarn(fmt, va_alist)
char *fmt;
va_dcl
#endif
{
    va_list vargs;
    int serrno = errno;

    (void) fflush(stdout);
    (void) fputs(progname, stderr);
    (void) putc(':', stderr);
    (void) putc(' ', stderr);
#if __STDC__
    va_start(vargs, fmt);
#else
    va_start(vargs);
#endif
#ifdef _BLD
#ifdef NO_VFPRINTF
    (void) _doprnt(fmt, vargs, stderr);
#else
    (void) vfprintf(stderr, fmt, vargs);
#endif
#else
    (void) vfprintf(stderr, fmt, vargs);
#endif
    va_end(vargs);
    (void) putc(':', stderr);
    (void) putc(' ', stderr);
    (void) fputs(errmsg(serrno), stderr);
    (void) putc('\n', stderr);
}

/*VARARGS1*/
#if __STDC__
efatal(char *fmt, ...)
#else
efatal(fmt, va_alist)
char *fmt;
va_dcl
#endif
{
    va_list vargs;
    int serrno = errno;

    (void) fflush(stdout);
    (void) fputs(progname, stderr);
    (void) putc(':', stderr);
    (void) putc(' ', stderr);
#if __STDC__
    va_start(vargs, fmt);
#else
    va_start(vargs);
#endif
#ifdef _BLD
#ifdef NO_VFPRINTF
    (void) _doprnt(fmt, vargs, stderr);
#else
    (void) vfprintf(stderr, fmt, vargs);
#endif
#else
    (void) vfprintf(stderr, fmt, vargs);
#endif
    va_end(vargs);
    (void) putc(':', stderr);
    (void) putc(' ', stderr);
    (void) fputs(errmsg(serrno), stderr);
    (void) putc('\n', stderr);
    exit(1);
}

/*VARARGS1*/
#if __STDC__
warn(char *fmt, ...)
#else
warn(fmt, va_alist)
char *fmt;
va_dcl
#endif
{
    va_list vargs;

    (void) fflush(stdout);
    (void) fputs(progname, stderr);
    (void) putc(':', stderr);
    (void) putc(' ', stderr);
#if __STDC__
    va_start(vargs, fmt);
#else
    va_start(vargs);
#endif
#ifdef _BLD
#ifdef NO_VFPRINTF
    (void) _doprnt(fmt, vargs, stderr);
#else
    (void) vfprintf(stderr, fmt, vargs);
#endif
#else
    (void) vfprintf(stderr, fmt, vargs);
#endif
    va_end(vargs);
    (void) putc('\n', stderr);
}

/*VARARGS1*/
#if __STDC__
fatal(char *fmt, ...)
#else
fatal(fmt, va_alist)
char *fmt;
va_dcl
#endif
{
    va_list vargs;

    (void) fflush(stdout);
    (void) fputs(progname, stderr);
    (void) putc(':', stderr);
    (void) putc(' ', stderr);
#if __STDC__
    va_start(vargs, fmt);
#else
    va_start(vargs);
#endif
#ifdef _BLD
#ifdef NO_VFPRINTF
    (void) _doprnt(fmt, vargs, stderr);
#else
    (void) vfprintf(stderr, fmt, vargs);
#endif
#else
    (void) vfprintf(stderr, fmt, vargs);
#endif
    va_end(vargs);
    (void) putc('\n', stderr);
    exit(1);
}


print_usage()
{
    fprintf(stderr, "usage: %s [ options ] <dependency-files>\n", progname);
    fprintf(stderr, "options include:\n");
    fprintf(stderr, "    -I<include-directory>\n");
    fprintf(stderr, "    -K<include-directory>\n");
    fprintf(stderr, "    -file <output-file>\n");
    fprintf(stderr, "    -debug\n");
    fprintf(stderr, "    -rm\n");
    fprintf(stderr, "    -std\n");
    fprintf(stderr, "    -verbose\n");
    fprintf(stderr, "    -version\n");
    exit(1);
}


/* show the revision of this program */
print_revision()
{
    printf("%s $Revision: 2.6 $ $Date: 93/05/07 23:57:00 $\n", progname);
}


#ifdef _BLD
static unsigned char tab[256] = {
	0
};

char *skipto (string,charset)
unsigned char *string, *charset;
{
	register unsigned char *setp,*strp;

	tab[0] = 1;		/* Stop on a null, too. */
	for (setp=charset;  *setp;  setp++) tab[*setp]=1;
	for (strp=string;  tab[*strp]==0;  strp++)  ;
	for (setp=charset;  *setp;  setp++) tab[*setp]=0;
	return ((char *)strp);
}

char *nxtarg (q,brk)
char **q,*brk;
{
	register char *front,*back;
	front = *q;			/* start of string */
	/* leading blanks and tabs */
	while (*front && (*front == ' ' || *front == '\t')) front++;
	/* find break character at end */
	if (brk == 0)  brk = " ";
	back = skipto (front,brk);
	_argbreak = *back;
	*q = (*back ? back+1 : back);	/* next arg start loc */
	/* elim trailing blanks and tabs */
	back -= 1;
	while ((back >= front) && (*back == ' ' || *back == '\t')) back--;
	back++;
	if (*back)  *back = '\0';
	return (front);
}

#define BUFFERSIZE 10240

int filecopy (here,there)
int here,there;
{
	register int kount;
	char buffer[BUFFERSIZE];
	kount = 0;
	while (kount == 0 && (kount=read(here,buffer,BUFFERSIZE)) > 0)
		kount -= write (there,buffer,kount);
	return (kount ? -1 : 0);
}

#ifdef BNR

int ffilecopy (here,there)
FILE *here, *there;
{
	register int c;

	if (fflush (there) == EOF)		/* flush pending output */
		return (EOF);

	for (c = getc(here); !feof(here); c = getc(here))
		putc(c, there);
	
	return (0);
}
#else /*  BNR */

int ffilecopy (here,there)
FILE *here, *there;
{
	register int i, herefile, therefile;

	herefile = fileno(here);
	therefile = fileno(there);

	if (fflush (there) == EOF)		/* flush pending output */
		return (EOF);

	if ((here->_cnt) > 0) {			/* flush buffered input */
		i = write (therefile, here->_ptr, here->_cnt);
		if (i != here->_cnt)  return (EOF);
		here->_ptr = here->_base;
		here->_cnt = 0;
	}

	i = filecopy (herefile, therefile);	/* fast file copy */
	if (i < 0)  return (EOF);

	(here->_flag) |= _IOEOF;		/* indicate EOF */

	return (0);
}
#endif /* BNR */

char *salloc(p)
char *p;
{
	register char *q;
	register int l;

	q = malloc(l = strlen(p) + 1);
	if (q != 0)
		bcopy(p, q, l);
	return(q);
}

extern int sys_nerr;
extern char *sys_errlist[];

static char *itoa(p,n)
char *p;
unsigned n;
{
    if (n >= 10)
	p =itoa(p,n/10);
    *p++ = (n%10)+'0';
    return(p);
}

char *errmsg(cod)
int cod;
{
	static char unkmsg[] = "Unknown error ";
	static char unk[sizeof(unkmsg)+11];		/* trust us */

	if (cod < 0) cod = errno;

	if((cod >= 0) && (cod < sys_nerr))
	    return(sys_errlist[cod]);

	strcpy(unk,unkmsg);
	*itoa(&unk[sizeof(unkmsg)-1],cod) = '\0';

	return(unk);
}

char *
vconcat(buf, buflen, ap)
char *buf;
int buflen;
va_list ap;
{
    register char *arg, *ptr, *ep;

    if (buf == NULL)
	return(NULL);
    if (buflen <= 0)
	return(NULL);
    ptr = buf;
    *ptr = '\0';
    ep = buf + buflen;
    while (ptr != NULL && (arg = va_arg(ap, char *)) != NULL)
	while (*ptr = *arg++)
	    if (++ptr == ep) {
		ptr = NULL;
		break;
	    }
    return(ptr);
}

/*VARARGS2*/
char *
#if __STDC__
concat(char *buf, int buflen, ...)
#else
concat(va_alist)
va_dcl
#endif
{
#if !__STDC__
    char *buf;
    int buflen;
#endif
    va_list ap;
    char *ptr;

#if __STDC__
    va_start(ap, buflen);
#else
    va_start(ap);
    buf = va_arg(ap, char *);
    buflen = va_arg(ap, int);
#endif
    ptr = vconcat(buf, buflen, ap);
    va_end(ap);
    return(ptr);
}
#endif /* _BLD */
