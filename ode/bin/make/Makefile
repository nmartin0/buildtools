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
#
# HISTORY
# $Log:	Makefile,v $
# Revision 2.5  93/05/07  23:56:04  mrt
# 	Changed the -DMACHINE flag to main to be -DHOST_MACHINE
# 	Added CCTYPE = host. Sometimes it makes a difference.
# 	[93/04/30            mrt]
# 
# Revision 2.4  93/03/20  00:04:44  mrt
# 	Corrected spelling of NO_STRERROR
# 	[93/03/18            mrt]
# 
# Revision 2.3  93/01/31  22:05:52  mrt
# 	Added definition to get make released to export/bin
# 	[93/01/08            mrt]
# 
# Revision 2.2  92/05/20  20:12:09  mrt
# 	First checkin
# 	[92/05/20  16:28:41  mrt]
# 
#

CCTYPE			= host

VPATH=	${ARCHIVE_FORMAT}:lst.lib:porting

DEPENDENCIES		=
SAVE_D			=

PROGRAMS		= odemake
SETUP_PROGRAMS		= odemake
MANPAGES		= odemake
MANSECTION		= 1
EXPBIN_TARGETS		= export_odemake

ILIST			= odemake odemake.${MANSECTION} odemake.0
IDIR			= /bin/
odemake.${MANSECTION}_IDIR	= /man/man${MANSECTION}/
odemake.0_IDIR		= /man/cat${MANSECTION}/

main.o_CFLAGS		= -DHOST_MACHINE=\"${HOST_MACHINE}\"
INCFLAGS		= -Ilst.lib
.if defined(NO_UTIME)
INCFLAGS		+= -Iporting
.endif
NEED_FOR_PORTING        = ${NO_FFS:Dffs.o:U} \
			  ${NO_GETCWD:Dgetcwd.o:U} \
			  ${NO_GETOPT:Dgetopt.o:U} \
                          ${NO_SETENV:Denvironment.o:U} \
			  ${NO_STRDUP:Dstrdup.o:U} \
			  ${NO_STRERROR:Dstrerror.o:U} \
			  ${NO_WAITPID:Dwaitpid.o:U} \
			  ${NO_UTIME:Dutime.o:U}
			  
HFILES=	make.h
OFILES=	arch.o arch_fmtdep.o buf.o compat.o cond.o dir.o hash.o job.o \
	main.o make.o parse.o str.o suff.o targ.o var.o \
	lstAlloc.o lstAppend.o lstAtEnd.o lstAtFront.o lstClose.o lstConcat.o \
	lstDatum.o lstDeQueue.o lstDestroy.o lstDupl.o lstEnQueue.o \
	lstFind.o lstFindFrom.o lstFirst.o lstForEach.o lstForEachFrom.o \
	lstInit.o lstInsert.o lstIsAtEnd.o lstIsEmpty.o lstLast.o \
	lstMember.o lstNext.o lstOpen.o lstRemove.o lstReplace.o lstSucc.o \
	${NEED_FOR_PORTING}

.include <${RULES_MK}>

