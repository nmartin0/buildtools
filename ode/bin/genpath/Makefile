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
# Revision 2.4  93/05/07  23:55:58  mrt
# 	Added CCTYPE = host. Sometimes it makes a difference.
# 	[93/05/07            mrt]
# 
# Revision 2.3  93/01/31  22:05:30  mrt
# 	Added defintion to release the program to bin/export.
# 	Fixed to always make in BLD mode (no rcfiles)
# 	[93/01/08            mrt]
# 
# Revision 2.2  92/05/20  20:11:57  mrt
# 	First checkin
# 


SETUP_PROGRAMS		= genpath
PROGRAMS		= genpath
MANPAGES		= genpath
MANSECTION		= 1

CCTYPE			= host

EXPBIN_TARGETS		= export_genpath

ILIST			= genpath genpath.${MANSECTION} genpath.0
IDIR			= /bin/
genpath.${MANSECTION}_IDIR   = /man/man${MANSECTION}/
genpath.0_IDIR		= /man/cat${MANSECTION}/

#For now always be in BLD mode
BLD			=
CFLAGS			= -D_BLD

.if !defined(BLD)
LIBS			= -lsb
.endif

.include <${RULES_MK}>
