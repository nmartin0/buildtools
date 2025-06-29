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
# Revision 2.5  93/05/07  23:56:22  mrt
# 	Added CCTYPE = host. Sometimes it makes a difference.
# 	[93/05/07            mrt]
# 
# Revision 2.4  93/03/20  00:42:55  mrt
# 	Used ${PORTING_FLAGS} instead of -DNO_DIRENT
# 	[93/03/19            mrt]
# 
# Revision 2.3  93/01/31  22:09:48  mrt
# 	Added definition to get wh exported to export/bin
# 	Added a definition of NO_DIRENT
# 	[93/01/08            mrt]
# 
# Revision 2.2  92/05/20  20:15:45  mrt
# 	First checkin
# 	[92/05/20  18:09:10  mrt]
# 

CCTYPE			= host

PROGRAMS		= wh
SETUP_PROGRAMS		= wh
MANPAGES		= wh
MANSECTION		= 1

EXPBIN_TARGETS		= export_wh

ILIST			= wh wh.${MANSECTION} wh.0
IDIR			= /bin/

wh.${MANSECTION}_IDIR	= /man/man${MANSECTION}/
wh.0_IDIR		= /man/cat${MANSECTION}/

CFLAGS			= -D_BSD -D${${TARGET_MACHINE}_VA_ARGV} ${PORTING_FLAGS}
#.if !defined(BLD)
#LIBS			= -lsb
#.endif

.include <${RULES_MK}>

