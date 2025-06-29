#
# Distributed as part of the Mach Operating System
#
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
# Revision 2.5  93/05/07  23:56:13  mrt
# 	Added CCTYPE = host. Sometimes it makes a difference.
# 	[93/05/07            mrt]
# 
# Revision 2.4  93/03/20  00:41:42  mrt
# 	Changed to use PORTING_FLAGS
# 
# Revision 2.3  93/01/31  22:08:55  mrt
# 	Added definition to release program to export/bin
# 	For now always build with -D_BLD and no libsb.a
# 	[93/01/08            mrt]
# 
# Revision 2.2  92/05/20  20:15:17  mrt
# 	First checkin
# 	[92/05/20  18:04:17  mrt]
# 

CCTYPE			= host

PROGRAMS		= md
SETUP_PROGRAMS		= md
MANPAGES		= md
MANSECTION		= 1

EXPBIN_TARGETS		= export_md

ILIST			= md md.${MANSECTION} md.0
IDIR			= /bin/
md.${MANSECTION}_IDIR	= /man/man${MANSECTION}/
md.0_IDIR		= /man/cat${MANSECTION}/


# Fixed to always be in BLD mode
BLD			=
CFLAGS			= -D_BSD ${PORTING_FLAGS} -D_BLD

.if !defined(BLD)
LIBS			= -lsb
.endif

.include <${RULES_MK}>
