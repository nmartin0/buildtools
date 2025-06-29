#
# Mach Operating System
# Copyright (c) 1993,1992 Carnegie Mellon University
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
# Revision 2.2  93/05/20  18:08:07  mrt
# 	Created
# 	[93/04/13            mrt]
# 
# 

# These are the divide routines from libc.a.
# It is built as a separate library for the kernel's bootstrap program to use



DEPENDENCIES 	=
LIBRARIES 	= libdivide.a
EXPLIB_TARGETS	= export_libdivide.a
EXPDIR		= /lib/

ILIST		= libdivide.a
IDIR		= /lib/

OFILES		=  __divl.o __divqu.o __remq.o __divlu.o __reml.o __remqu.o \
	__divq.o __remlu.o 

.include <${RULES_MK}>

