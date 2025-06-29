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
# Revision 2.6  93/06/17  14:48:13  mrt
# 	Force /usr/include and /usr/cs/include to front of search path.
# 	the sys/exec.h that the Mach Unix server exports conflicts with
# 	the standard a.out.h for coff machines.
# 	[93/06/16            mrt]
# 
# Revision 2.5  93/05/07  23:56:18  mrt
# 	Added CCTYPE = host. Sometimes it makes a difference.
# 	[93/05/07            mrt]
# 
# Revision 2.4  93/03/20  00:42:25  mrt
# 	Added ${PORTING_FLAGS} to CFLAGS
# 	[93/03/19            mrt]
# 
# Revision 2.3  93/01/31  22:09:17  mrt
# 	Added defintion to release the program to bin/export.
# 	Fixed to always make in BLD mode (no rcfiles)
# 	[93/01/08            mrt]
# 
# Revision 2.2  92/05/20  20:15:25  mrt
# 	First checkin
# 	[92/05/20  18:06:47  mrt]
# 

CCTYPE			= host

PROGRAMS		= release
SETUP_PROGRAMS		= release
MANPAGES		= release
MANSECTION		= 1

EXPBIN_TARGETS		= export_release

ILIST			= release release.${MANSECTION} release.0
IDIR			= /bin/

.if defined (host_context) && ( ${host_context} == "pmax_mach" )
INCFLAGS		= "-I/usr/cs/include -I/usr/include"
.endif


release.${MANSECTION}_IDIR	= /man/man${MANSECTION}/
release.0_IDIR		= /man/cat${MANSECTION}/

#For now always be in BLD mode
BLD			=
CFLAGS			= -D_BLD -D_BSD -D${OBJECT_FORMAT} ${PORTING_FLAGS}


.if !defined(BLD)
LIBS			= -lsb
HFILES			= release_db.h
OFILES			= release.o release_db.o
.endif


.include <${RULES_MK}>
