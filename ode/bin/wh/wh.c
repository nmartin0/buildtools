/*
 * Copyright (c) 1993,1992 Carnegie Mellon University
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
 *  wh -- a program for finding instances of files along paths
 *      Composed from pieces of jag's wh.c for path searching,
 *	uses expand(3) to grok wildcards,
 *	normally uses ls(1) to format output, but
 *	handles emacs(1) error messages (-X) locally.
 *
 *      Options known to wh(1):
 *        name    -- search along current path for instances of name
 *        -f name -- search etc., useful if name starts with '-'
 *        -q      -- quit search after finding first instance of file
 *        -p path -- set path to search along
 *        -C      -- set path to CPATH
 *        -E      -- set path to EPATH
 *        -L      -- set path to LPATH
 *        -M      -- set path to MPATH
 *        -P      -- set path to  PATH
 *        -R      -- recursive directory search
 *	  -X      -- list names in emacs(1) error format
 *        --      -- pass remainder of switch to ls(1)
 *	All other switches (arguments starting with '-') are passed
 *	through as formatting options to ls(1) (collisions on -fqC).
 *
 *	Exit codes:
 *	  0 - if at least 1 one file was found,
 *	  1 - if no files were found,
 *	  2 - if some error was encountered
 *
 *  HISTORY
 * $Log:	wh.c,v $
 * Revision 2.4  93/03/20  00:09:36  mrt
 * 	Added code for searchp which is a libcmucs invention.
 * 	Added include of strings.h to quiet gcc warnings.
 * 	[93/03/09            mrt]
 * 
 * Revision 2.3  93/01/31  22:10:13  mrt
 * 	Removed use of quit to make this more portable.
 * 	[93/01/12            mrt]
 * 
 * Revision 2.2  92/01/24  01:27:12  rpd
 * 	Moved to Mach distribution.
 * 	Added hack definitions of expand and runvp.
 * 	[92/01/24            rpd]
 * 
 * Revision 1.3  92/01/20  22:10:12  mja
 * 	Add copyright/disclaimer for distribution.
 * 	[92/01/20  22:08:46  mja]
 * 
 * Revision 1.2  89/02/16  14:09:37  berman
 * 	Added casts for HC on IBMRT.
 * 	[89/02/16            berman]
 * 
 * 03-Mar-86  Bob Fitzgerald (rpf) at Carnegie-Mellon University
 *	Added lsargs to fix old bug caused by addition of lsarggrow.
 *	Considered lazy-evaluating calls on ls(1) to cut down on processes,
 *	but didn't because ls sorts its input.
 *
 * 06-Feb-86  Bob Fitzgerald (rpf) at Carnegie-Mellon University
 *	Converted for 4.2 directory structure.
 *	Sent diagnostic output to stderr.
 *
 * 05-May-82  Bob Fitzgerald (rpf) at Carnegie-Mellon University
 *	Replaced home-brew runls() that invoked ls(1) with a call on the
 *	runvp(3) utility.
 *
 * 29-Apr-82  Bob Fitzgerald (rpf) at Carnegie-Mellon University
 *	Beefed up parser to recognize blocks of switches, extract those
 *	meaningful here and pass the rest to ls(1).  Circumvented two
 *	bugs in expand(3).  First, check file names returned to ensure
 *	that they really exist.  Second, grow buffer when necessary
 *	(return from expand is -1, not bufsize+1 as is documented).
 *
 * 28-Mar-82  Bob Fitzgerald (rpf) at Carnegie-Mellon University
 *	Created.
 *
 */

#ifdef notdef
static char sccsid[] = "@(#) wh.c 3.2  3-Mar-86";
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <stdio.h>
#include <strings.h>


#ifdef NO_DIRENT
#define dirent direct
#endif

extern unsigned char errno;
extern char *getenv();
extern char *malloc();

char *wh;
char ls[] = "ls";

int    lsargc;			/* number of free slots in ls arglist */
char **lsargv;			/* pointer to front of entire ls arglist */
char **lsargf;			/* pointer to first file name in ls arglist */
char **lsargp;			/* pointer to free slots in ls arglist */
char **lsargs;			/* pointer to saved spot in ls arglist */

char *pathname;
char *path;
char *givenname;
char  namebuffer[256];

int   qflag;			/* quit after first find(s) when set */
int   Rflag;			/* recursive directory search when set */
int   Xflag;			/* emacs(1) format error messages when set */

int   namehits;
int   totalhits;


/*
 *  Management of argument list to ls(1).
 *  LSARGSLACK must be at least 1 to allow room for NULL terminator.
 *  Slots in use are at the beginning (lsargp-lsargv) and end (LSARGSLACK).
 *  List is doubled in size each time it is grown.
 */
#define LSARGSLACK  2
#define LSARGINIT   126
#define lsargused   ((lsargp-lsargv)+LSARGSLACK)
#define lsargnext(cur) (cur<<1)


/*
 *  newpath -- takes the string name of a path (e.g. "PATH"),
 *      looks it up in the environment and remembers pointers
 *      to both pathname and path in global variables.
 *      Make sure storage for pathname and path remains valid,
 *	as only pointers are preserved here.
 */
newpath(fpathname)
    char *fpathname;
  {
    pathname = fpathname;
    path = getenv(fpathname);
    if (!path){
	fprintf(stderr, "%s:  path %s not found in environment\n",
		wh, pathname);
        exit(2);
    }
  }


/*
 *  lsarggrow -- increase size of arg list to expand(3) and ls(1).
 *	Copies front of old arglist into new arglist.
 *	Updates lsargc, lsargv, lsargf, lsargs, lsargp for new arg list.
 */
lsarggrow(fullname)
    char *fullname;
  {
    register char **oldp;
    register char **newp;

    lsargc = lsargnext(lsargc+lsargused);
    oldp = lsargv;
    newp = (char **)malloc(lsargc*sizeof(char *));
    if (newp <= (char **)0) {
	fprintf(stderr, "%s:  ran out of free space expanding %s\n",
		wh, fullname);
	exit(2);
    }
    lsargf = newp + (lsargf - lsargv);
    lsargs = newp + (lsargs - lsargv);
    lsargv = newp;
    while (oldp < lsargp)
        *newp++ = *oldp++;
    lsargp = newp;
    lsargc -= lsargused;
  }


/*
 *  runls -- prints the entries in lsargv
 *	either here (for -X) or by invoking ls(1)
 *	unwinds lsargv back to lsargf.
 */
runls()
  {
    if (Xflag)
      {
        char **lead;
        for (lead = lsargs; *lead; lead++)
            printf("\"%s\", line 1:\n", *lead);
      }
    else
      {
        int retcode;

        retcode = runvp(ls, lsargv);
        if (-1 == retcode) {
	    fprintf(stderr, "%s:  error %d in executing %s\n", wh, errno, ls);
	    exit(2);
	}
        else if (0 < retcode) {
	    fprintf(stderr, "%s:  %s returned status %d\n", wh, ls, retcode);
	    exit(2);
	}
      }
    lsargc += (lsargp - lsargf);
    lsargp = lsargf;
  }

/*
 *  lookup -- takes a full file name, looks it up and
 *	generates output for anything found.
 *      Records successful lookups by incrementing namehits and totalhits.
 *      Returns 0 on a hit with qflag set, otherwise returns non-zero
 */
int lookup(fullname)
    char *fullname;
  {
    lsargs = lsargp;
    if (*fullname != '\0')
	    strcat(fullname, "/");
    reclookup(fullname);
    if (lsargp == lsargs)
        return(1);

#ifndef LAZYLS
    runls();
#endif
    namehits++;
    totalhits++;
    return(!qflag);
  }

/*
 *  reclookup --
 */
reclookup(fullname)
    char *fullname;
  {
    int found;
    char **lead;

    if (Rflag)
      {
        int pathlen;
        DIR *dirp;

        pathlen = strlen(fullname);
        if (pathlen > 0)
          {
            fullname[pathlen-1] = NULL;
            if (NULL == (dirp = opendir(fullname)))
                fprintf(stderr, "couldn't open \"%s\"\n", fullname);
            strcat(fullname,"/");
          }
        else
          {
            if (NULL == (dirp = opendir(".")))
                fprintf(stderr, "couldn't open \".\"\n");
          }
        if (NULL != dirp)
          {
            struct dirent *dp;

            while (NULL != (dp = readdir(dirp)))
              {
                if ( (0 != dp->d_ino) &&
                     (('.' != dp->d_name[0])) &&
                     (0 != strcmp(dp->d_name, ".")) &&
                     (0 != strcmp(dp->d_name, "..")) )
                  {
                    struct stat statbuf;
                    strcat(fullname, dp->d_name);
                    if (-1 != stat(fullname, &statbuf))
                      {
                        if (S_IFDIR & statbuf.st_mode)
                          {
                            strcat(fullname, "/");
                            reclookup(fullname);
                          }
                      }
                    else fprintf(stderr, "%s:  can't open directory %s\n", wh, fullname);
                    fullname[pathlen] = NULL;
                  }
              }
            closedir(dirp);
          }
      }

    /*
     *  expand wildcards
     *  return non-zero for nothing found
     *  check for expansion errors (expand apparently returns -1 for
     *    too-many-names, not lsargc+1 as is documented)
     *  NULL-terminate parameter list
     */
    strcat(fullname, givenname);
    found = expand(fullname,lsargp,lsargc);
    while ( (found < 0) || (lsargc < found) )
      {
        lsarggrow(fullname);
        found = expand(fullname,lsargp,lsargc);
      }
    *(lsargp+found) = 0;

    /*
     *  scan expanded list, making sure that the files really exist
     *  (since expand doesn't bother to check while expanding wildcards
     *  in directory names in the middle of paths).
     *  compress any bogus entries (before ls(1) gets them and prints
     *  some idiot message about file not existing).
     *  Check again for no acceptable files, returning non-zero if so
     */
    lead = lsargp;
    while (*lsargp = *lead)
      {
        static struct stat buf;
        if (!stat(*lead++, &buf))
          {
            lsargp++;
            lsargc--;
          }
      }
  }


/*
 *  searchpath -- look for instances of filename on path recorded
 *      in global variable path.  Gripe if nothing found.
 *	Global givenname is used to pass filename into lookup.
 *	Global namehits is incremented by lookup when appropriate.
 */
searchpath(filename)
    char *filename;
  {
    namehits = 0;
    givenname = filename;
    searchp(path, "", namebuffer, lookup);
    if (!namehits)
        fprintf(stderr, "%s:  %s not found on %s\n", wh, filename, pathname);
  }


/*
 *  switchblock -- parse one switch block, being given a pointer to
 *	the first character after the '-'.
 */
switchblock(swp)
    char *swp;
  {
    char option;
    char *leadp  = swp;			/* next option to look at */
    char *trailp = swp;			/* next place to put ls(1) option */

    /*
     *  Scan over switches in block
     *  processing those we know (qCLMPRX) and eliminating them,
     *  compacting those we doesn't know about for later.
     */
    while (option = *leadp++)
      {
        switch (option)
          {
          case 'q': qflag++; break;
          case 'R': Rflag++; break;
          case 'X': Xflag++; break;
          case 'C': newpath("CPATH"); break;
#ifdef	CMUCS
          case 'E': newpath("EPATH"); break;
#endif
          case 'L': newpath("LPATH"); break;
          case 'M': newpath("MPATH"); break;
          case 'P': newpath( "PATH"); break;
          default : *trailp++ = option; break;
          }
      }

    /*
     *  If anything remains to be passed to ls(1),
     *  NULL terminate the switch block and back up over the '-'
     *  before appending block to the ls arg list.
     */
    if (trailp != swp)
      {
#ifdef LAZYLS
        if (lsargf != lsargp)
            runls();
#endif
        *trailp++ = 0;
        *lsargp++ = swp-1;
        lsargc--;
        lsargf = lsargp;
      }
  }


/*
 *  Main program -- parse command line
 */
main(argc, argv)
    int    argc;
    char **argv;
  {
    wh = argv[0];
    if (1 == argc) {
	print_usage();	
	exit(2);
    }


    totalhits = 0;
    newpath("PATH");
    lsargc = LSARGINIT;
    lsargp = lsargv = 0;
    lsarggrow(ls);
    *lsargp++ = ls;
    lsargc--;
    lsargf = lsargp;
    while (0 < --argc)
      {
        if ('-' == **++argv)
          {
            switch (*++*argv)
              {

              case 'p':
                if (0 >= --argc){
		    fprintf(stderr, "%s:  path name expected after -p\n", wh);
		    exit(2);
		}
                newpath(*++argv);
              break;

              case 'f':
                if (0 >= --argc) {
		    fprintf(stderr, "%s:  file name expected after -f\n", wh);
		    exit(2);
		}
                searchpath(*++argv);
              break;

              case '-':
#ifdef LAZYLS
                if (lsargf != lsargp)
                    runls();
#endif
                *lsargp++ = *argv;
                lsargc--;
                lsargf = lsargp;
              break;

              default :
                switchblock(*argv);
              break;
              }
          }
         else
            searchpath(*argv);
      }
#ifdef LAZYLS
    if (lsargf != lsargp)
        runls();
#endif
    exit(totalhits == 0);
  }


/*  searchp  --  search through pathlist for file
 *
 *  Usage:  p = searchp (path,file,fullname,func);
 *	char *p, *path, *file, *fullname;
 *	int (*func)();
 *
 *  Searchp will parse "path", a list of pathnames separated
 *  by colons, prepending each pathname to "file".  The resulting
 *  filename will be passed to "func", a function provided by the
 *  user.  This function must return zero if the search is
 *  successful (i.e. ended), and non-zero if the search must
 *  continue.  If the function returns zero (success), then
 *  searching stops, the full filename is placed into "fullname",
 *  and searchp returns 0.  If the pathnames are all unsuccessfully
 *  examined, then searchp returns -1.
 *  If "file" begins with a slash, it is assumed to be an
 *  absolute pathname and the "path" list is not used.  Note
 *  that this rule is used by Bell's cc also; whereas Bell's
 *  sh uses the rule that any filename which CONTAINS a slash
 *  is assumed to be absolute.  The execlp and execvp procedures
 *  also use this latter rule.  In my opinion, this is bogosity.
 */

int searchp (path,file,fullname,func)
char *path,*file,*fullname;
int (*func)();
{
	register char *nextpath,*nextchar,*fname,*lastchar;
	int failure;

	nextpath = ((*file == '/') ? "" : path);
	do {
		fname = fullname;
		nextchar = nextpath;
		while (*nextchar && (*nextchar != ':'))
			*fname++ = *nextchar++;
		if (nextchar != nextpath && *file) *fname++ = '/';
		lastchar = nextchar;
		nextpath = ((*nextchar) ? nextchar + 1 : nextchar);
		nextchar = file;	/* append file */
		while (*nextchar)  *fname++ = *nextchar++;
		*fname = '\0';
		failure = (*func) (fullname);
	} 
	while (failure && (*lastchar));
	return (failure ? -1 : 0);
}

/*
 *	A dummy version of expand(), so that I don't have
 *	drag in yet another CMU-CS hack.
 */

expand(spec, buffer, bufsize)
    char *spec;
    char **buffer;
    int bufsize;
{
    char *copy;

    if (bufsize < 1)
	return bufsize + 1;

    copy = malloc(strlen(spec) + 1);
    if (copy == 0)
	return -1;

    strcpy(*buffer = copy, spec);
    return 1;
}

/*
 *	A dummy version of runvp(), so that I don't have
 *	drag in yet another CMU-CS hack.
 */

runvp(program, argv)
    char *program;
    char **argv;
{
    int pid;

    if ((pid = fork()) == -1)
	return -1;

    if (pid == 0) {
	execvp(program, argv);
	return -1;
    }

    while (wait(0) != pid)
	;

    return 0;
}

/* show invocation options */
print_usage()
{
    fprintf(stderr,
#if	CMUCS
	    "usage:  %s { -qCELMPRX | -p path | -f file | file }\n",
#else
	    "usage:  %s { -qCLMPRX | -p path | -f file | file }\n",
#endif
	    wh);
}

