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
# Revision 2.5  93/05/07  23:56:09  mrt
# 	Added CCTYPE = host. Sometimes it makes a difference.
# 	[93/05/07            mrt]
# 
# Revision 2.4  93/03/20  00:06:05  mrt
# 	Changed to use PORTING_FLAGS.
# 
# Revision 2.3  93/01/31  22:08:48  mrt
# 	Added defintion to release the program to bin/export.
# 	Fixed to always make in BLD mode (no rcfiles)
# 	[93/01/08            mrt]
# 
# Revision 2.2  92/05/20  20:15:09  mrt
# 	First checkin
# 	[92/05/20  18:02:26  mrt]
# 

CCTYPE			= host

PROGRAMS		= makepath
SETUP_PROGRAMS		= makepath
MANPAGES		= makepath
MANSECTION		= 1

EXPBIN_TARGETS		= export_makepath
ILIST			= makepath makepath.${MANSECTION} makepath.0
IDIR			= /bin/
makepath.${MANSECTION}_IDIR	= /man/man${MANSECTION}/
makepath.0_IDIR		= /man/cat${MANSECTION}/


#For now always be in BLD mode

BLD			=
CFLAGS			= -D_BLD ${PORTING_FLAGS}

.if !defined(BLD)
LIBS			= -lsb
.endif

# Use generated version of makepath in the export rule, since
# the rule will delete the previously exported one.

MAKEPATH		= ./makepath

.include <${RULES_MK}>
