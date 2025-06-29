#
# Mach Operating System
# Copyright (c) 1993-1989 Carnegie Mellon University
# All Rights Reserved.
# 
# Permission to use, copy, modify and distribute this software and its
# documentation is hereby granted, provided that both the copyright
# notice and this permission notice appear in all copies of the
# software, derivative works or modified versions, and any portions
# thereof, and that both notices appear in supporting documentation.
# 
# CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
# CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
# ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
# 
# Carnegie Mellon requests users of this software to return to
# 
#  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
#  School of Computer Science
#  Carnegie Mellon University
#  Pittsburgh PA 15213-3890
# 
# any improvements or extensions that they make and grant Carnegie Mellon
# the rights to redistribute these changes.
#
#
# HISTORY
# $Log:	Makefile,v $
# Revision 2.3  93/06/01  22:11:54  mrt
# 	Drop the buildtools from IDIR.
# 	[93/06/01            mrt]
# 
# Revision 2.2  93/05/08  00:29:20  mrt
# 	BNR2 file for reference
# 
#
#	Makefile to export the include files that are
#	needed by the buildtools to compile. Whenever possible
#	you should use the .h files that are native to your system.
#	These files are here, in case you do not have some them or
#	need a reference file that the buildtools can compile against


IDIR		= /include/sys/
INCLUDES	= cdefs.h dir.h errno.h exec.h fcntl.h file.h \
		  param.h resource.h \
		  signal.h stat.h syslimits.h time.h \
		  types.h unistd.h varargs.h wait.h


.include <${RULES_MK}>
