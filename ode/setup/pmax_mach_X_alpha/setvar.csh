#!/bin/csh
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
# Copyright (c) 1991
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
#
# HISTORY
# $Log:	setvar.csh,v $
# Revision 2.5  93/05/20  20:52:20  mrt
# 	Corrected typo.
# 	[93/05/20            mrt]
# 
# Revision 2.4  93/05/11  18:04:44  mrt
# 	Added definition of SIZE as asize
# 	Set DEF_MIGFLAGS to work with acpp
# 	Set ALPHA_ASFLAGS to -nocpp
# 	Added ALPHA_LDLIBS 
# 	Added -Dalpha to DEF_MIGFLAGS. At least machid.defs needs this.
# 	Defined NOT_USING_GCC2
# 	[93/03/22            mrt]
# 
# Revision 2.3  93/03/20  00:47:59  mrt
# 	Made PORTING_FLAGS and environment variable so it could be
# 	used by Makefiles.
# 	[93/03/19            mrt]
# 
# Revision 2.2  93/02/06  17:33:58  mrt
# 	Added setenv for HOST_MACHINE
# 	[92/12/11            mrt]
# 
# 	Fixed assignment of KERN_MACHINE_DIR
# 	[92/08/17            mrt]
# 
# Revision 2.2  92/05/20  20:16:34  mrt
# 	First checkin
# 	[92/05/20            mrt]
# 

#
# Setup variables describing the current environment
# The target machine is assumed to be the same as the host machine
# unless target_machine is set. The target OS is assumed to be Mach
#

#HOST_MACHINE is used to chose compiler programs if CCTYPE-host
setenv HOST_MACHINE "PMAX"
setenv host_machine "pmax"
# target_machine may be set on command line if we are cross-building
if ( ! $?target_machine ) then 
    setenv target_machine "pmax"
endif
# TARGET_MACHINE names subdirectories with machine specific files
setenv TARGET_MACHINE "`echo ${target_machine} |  tr a-z A-Z`"
#target_cpu is used as the name of the kernel directory that "machine"
#links to unless it it overridden by KERN_MACHINE_DIR
if ( $TARGET_MACHINE == "PMAX" ) then
    setenv TARGET_CPU "MIPS"
    setenv target_cpu "mips"
else
   setenv TARGET_CPU ${TARGET_MACHINE}
   setenv target_cpu ${target_machine}
endif
setenv TARGET_OS "MACH"
setenv target_os "mach"
set target_context="${target_machine}_${target_os}"

#
# Default search path 
#
set DEFPATH=/usr1/alpha/usr/bin:/usr/cs/bin:/usr/ucb:/bin:/usr/bin
setenv DEFCPATH /usr/cs/include:/usr/include
set DEFLPATH=/usr/cs/lib:/usr/lib

#
# Cross compilation environment
#
#
setenv PMAX_HOST_CC gcc
setenv DEF_CCFLAGS "-DMACH -DCMU -DCMUCS -DCROSS_COMPILE_32_TO_64_BITS -D__STDC__"
setenv DEF_MIGFLAGS "-DMACH -DCMU -DCMUCS -DCROSS_COMPILE_32_TO_64_BITS -D__STDC__ -Dalpha"
setenv ALPHA_RANLIB "aar ts"
setenv ALPHA_AR aar
setenv ALPHA_LD ald
setenv ALPHA_LDLIBS -ldivide
setenv ALPHA_AS aas
setenv ALPHA_ASFLAGS -nocpp
setenv ALPHA_CC acc
setenv ALPHA_CCP "acc -E"
setenv NOT_USING_GCC2 NOT_USING_GCC2
setenv XSTRIP /bin/true
setenv SIZE asize

# File formats
#
setenv OBJECT_FORMAT COFF
setenv ARCHIVE_FORMAT BSDARCH

setenv NOSHARED_LIBRARIES NOSHARED_LIBRARIES
setenv CROSS_64_CFLAGS  "-DCROSS_COMPILE_32_TO_64_BITS -g3"
setenv COMP_HOST_ROOT /usr1/alpha
#
# Porting flags - used by some ODE tools to avoid using
# features that the current system doesn't support
# Set PORTING_FLAGS to the ones your system is missing
#
# NO_DIRENT used by md.c
# NO_SETENV
# NO_STRDUP used by makepath.c
# NO_SYS_LIMITS used by makepath.c
# NO_UTIMES used by release.c
# NO_VPRINTF used by md.c, release.c

setenv PORTING_FLAGS "-DNO_STRDUP -DNO_SYS_LIMITS -DNO_DIRENT"

#
# Define these if your system  does not have these entries
# in libc.a. Make needs them and will get them from its 

# porting directory if the NO_ environment variable is set
#

#setenv NO_FSS NOFSS
setenv NO_GETCWD NO_GETCWD
# The getopt that the CMU-CS libc.a is too old
setenv NO_GETOPT NO_GETOPT
#setenv NO_SETENV NO_SETENV
#setenv NO_STRDUP NO_STRDUP
#setenv NO_STRERROR NO_STRERROR
# CMU-CS has old one that doesn't use the utime structure
setenv NO_UTIME NO_UTIME
setenv NO_WAITPID NO_WAITPID

