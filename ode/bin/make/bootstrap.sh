#! /bin/sh
#
# Distributed as part of the Mach Operating System
#
# Copyright (c) 1990, 1991
# Open Software Foundation, Inc.
# 
# Permission is hereby granted to use, copy, modify and freely distribute
# the software in this file and its documentation for any purpose without
# fee, provided that the above copyright notice appears in all copies and
# that both the copyright notice and this permission notice appear in
# supporting documentation.  Further, provided that the name of Open
# Software Foundation, Inc. ("OSF") not be used in advertising or
# publicity pertaining to distribution of the software without prior
# written permission from OSF.  OSF makes no representations about the
# suitability of this software for any purpose.  It is provided "as is"
# without express or implied warranty.
#
# HISTORY
# $Log:	bootstrap.sh,v $
# Revision 2.6  93/05/08  00:19:28  mrt
# 	Changed MACHINE for i386_bnr to I386. 
# 	Added exportbase/buildtools/include to -I flags
# 	[93/05/07            mrt]
# 
# 	Changed -DMACHINE flag to main.c to -DHOST_MACHINE.
# 	[93/04/30            mrt]
# 
# Revision 2.5  93/03/20  00:39:45  mrt
# 	Added alpha_mach and parisc_mach cases.
# 	[93/03/08            mrt]
# 
# Revision 2.4  93/01/31  22:06:02  mrt
# 	Added case for luna88k
# 	[93/01/14            mrt]
# 
# Revision 2.3  92/07/08  18:05:27  mrt
# 	Added a case for the i386_bnr context.
# 	[92/07/08            mrt]
# 
# Revision 2.2  92/05/20  20:12:19  mrt
# 	First checkin
# 	[92/05/20  16:37:23  mrt]
# 
#

# Defines that Make cares about - needs Reno or later getopt.c
#
# MACHINE NO_SETENV NO_STRDUP NO_STRERROR NO_GETCWD NO_UTIME NO_WAITPID
# NO_GETOPT ARCHIVE_FORMAT TARGET_LIBS

# MACHINE is an attempt to be backwards compatible with the BSD 3.4 make
# It is not used by this version of make and should not be used by any
# updated Makefiles, They should use HOST_MACHINE, TARGET_MACHINE, 
# target_machine, TARGET_CPU,or target_cpu. This version of make does not
# automatically search any machine-specific directories.

case $context in
	"pmax_osf1")	MACHINE="mips" ;
			ARCHIVE_FORMAT=OSFARCH;;
	"alpha_osf1")	MACHINE="alpha" ;
			ARCHIVE_FORMAT=BSDARCH ;
			NO_SETENV=NO_SETENV ;;
	"mmax_osf1")	MACHINE="mmax" ;
			ARCHIVE_FORMAT=OSFARCH;;
	"at386_osf1")	MACHINE="i386";
			ARCHIVE_FORMAT=OSFARCH;;
	"pmax_ultrix")	MACHINE="mips" ;
			ARCHIVE_FORMAT=BSDARCH ;
			NO_STRDUP=NO_STRDUP ;
			NO_SETENV=NO_SETENV ;;
	"rios_aix")	MACHINE="RIOS";
			ARCHIVE_FORMAT=AIXARCH;
			NO_SETENV=NO_SETENV;
			TARGET_LIBS=-lbsd;;
	"hp300_hpux")	MACHINE="hp300";
			ARCHIVE_FORMAT=BSDARCH;
			TARGET_LIBS=-lBSD;
                        NO_FFS=NO_FFS;
			NO_SETENV=NO_SETENV;
			TARGET_FLAGS=-DNO_UTIMES;;
	"i386_bnr")	MACHINE="I386";
			ARCHIVE_FORMAT=BSDARCH;;
	*_mach)		ARCHIVE_FORMAT=BSDARCH;
			NO_STRDUP=NO_STRDUP;
			NO_GETOPT=NO_GETOPT;
			NO_GETCWD=NO_GETCWD;
			NO_WAITPID=NO_WAITPID;
			NO_UTIME=NO_UTIME;
	case $context in
	"i386_mach")	MACHINE="I386" ;;
	"pmax_mach")	MACHINE="PMAX" ;;
	"sun3_mach")	MACHINE="SUN3" ;;
	"sun4_mach")	MACHINE="SUN4" ;;
	"vax_mach")	MACHINE="VAX"  ;;
	"luna88k_mach") MACHINE="LUNA88K" ;;
	"alpha_mach")	MACHINE="ALPHA" ;
			NO_STRERROR=NO_STRERROR ;;
	"parisc_mach")  MACHINE="PARISC" 
	esac
esac


SRCPATH=${MAKETOP}${MAKESUB}
CFLAGS=" ${CENV} ${PROFILING_FLAGS} -D_BSD -I${SRCPATH} -I${SRCPATH}lst.lib -I${EXPORTBASE}/buildtools/include -I${SRCPATH}porting ${TARGET_FLAGS}"
MAIN_CFLAGS="-DHOST_MACHINE=\"$MACHINE\""

CC=${CC-cc}
PORTING_OFILES=
if [ -n "$NO_FFS" ]
then
    ${CC} -c ${CFLAGS} ${SRCPATH}porting/ffs.c
    PORTING_OFILES="${PORTING_OFILES} ffs.o"
fi
if [ -n "$NO_SETENV" ]
then
    ${CC} -c ${CFLAGS} ${SRCPATH}porting/environment.c
    PORTING_OFILES="${PORTING_OFILES} environment.o"
fi
if [ -n "$NO_STRDUP" ]
then
    ${CC} -c ${CFLAGS} ${SRCPATH}porting/strdup.c
    PORTING_OFILES="${PORTING_OFILES} strdup.o"
fi
if [ -n "$NO_STRERROR" ]
then
    ${CC} -c ${CFLAGS} ${SRCPATH}porting/strerror.c
    PORTING_OFILES="${PORTING_OFILES} strerror.o"
fi
if [ -n "$NO_GETCWD" ]
then
    ${CC} -c ${CFLAGS} ${SRCPATH}porting/getcwd.c
    PORTING_OFILES="${PORTING_OFILES} getcwd.o"
fi
if [ -n "$NO_UTIME" ]
then
    ${CC} -c ${CFLAGS} ${SRCPATH}porting/utime.c
    PORTING_OFILES="${PORTING_OFILES} utime.o"
fi
if [ -n "$NO_WAITPID" ]
then
    ${CC} -c ${CFLAGS} ${SRCPATH}porting/waitpid.c
    PORTING_OFILES="${PORTING_OFILES} waitpid.o"
fi
if [ -n "$NO_GETOPT" ]
then
    ${CC} -c ${CFLAGS} ${SRCPATH}porting/getopt.c
    PORTING_OFILES="${PORTING_OFILES} getopt.o"
fi
${CC} -c ${CFLAGS} ${SRCPATH}arch.c
${CC} -c ${CFLAGS} ${SRCPATH}${ARCHIVE_FORMAT}/arch_fmtdep.c
${CC} -c ${CFLAGS} ${SRCPATH}buf.c
${CC} -c ${CFLAGS} ${SRCPATH}compat.c
${CC} -c ${CFLAGS} ${SRCPATH}cond.c
${CC} -c ${CFLAGS} ${SRCPATH}dir.c
${CC} -c ${CFLAGS} ${SRCPATH}hash.c
${CC} -c ${CFLAGS} ${SRCPATH}job.c
${CC} -c ${CFLAGS} ${MAIN_CFLAGS} ${SRCPATH}main.c
${CC} -c ${CFLAGS} ${SRCPATH}make.c
${CC} -c ${CFLAGS} ${SRCPATH}parse.c
${CC} -c ${CFLAGS} ${SRCPATH}str.c
${CC} -c ${CFLAGS} ${SRCPATH}suff.c
${CC} -c ${CFLAGS} ${SRCPATH}targ.c
${CC} -c ${CFLAGS} ${SRCPATH}var.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstAlloc.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstAppend.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstAtEnd.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstAtFront.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstClose.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstConcat.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstDatum.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstDeQueue.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstDestroy.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstDupl.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstEnQueue.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstFind.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstFindFrom.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstFirst.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstForEach.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstForEachFrom.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstInit.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstInsert.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstIsAtEnd.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstIsEmpty.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstLast.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstMember.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstNext.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstOpen.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstRemove.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstReplace.c
${CC} -c ${CFLAGS} ${SRCPATH}lst.lib/lstSucc.c
${CC} -g ${PROFILING_FLAGS}\
    arch.o\
    arch_fmtdep.o\
    buf.o\
    compat.o\
    cond.o\
    dir.o\
    hash.o\
    job.o\
    main.o\
    make.o\
    parse.o\
    str.o\
    suff.o\
    targ.o\
    var.o\
    lstAlloc.o\
    lstAppend.o\
    lstAtEnd.o\
    lstAtFront.o\
    lstClose.o\
    lstConcat.o\
    lstDatum.o\
    lstDeQueue.o\
    lstDestroy.o\
    lstDupl.o\
    lstEnQueue.o\
    lstFind.o\
    lstFindFrom.o\
    lstFirst.o\
    lstForEach.o\
    lstForEachFrom.o\
    lstInit.o\
    lstInsert.o\
    lstIsAtEnd.o\
    lstIsEmpty.o\
    lstLast.o\
    lstMember.o\
    lstNext.o\
    lstOpen.o\
    lstRemove.o\
    lstReplace.o\
    lstSucc.o\
    ${PORTING_OFILES}\
    ${TARGET_LIBS}\
    -o odemake.X
mv odemake.X odemake
