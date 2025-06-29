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
# Revision 2.2  93/08/02  13:40:33  mrt
# 	Copied the pmax_mach file and modified it.
# 	[93/07/07            mrt]
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


# get gcc 2.3.3 from the mach3.release area and set GCC_EXEC_PREFIX
# to where you installed it. DEFCPATH needs to be set to get the GCC version
# of stdarg.h rather than the Ultrix one.

setenv GCC_INSTALL_DIR /usr/mrt/mach3/release/pmax_ultrix/lib/gcc-lib/
setenv GCC_EXEC_PREFIX ${GCC_INSTALL_DIR}

#
# Default search path 
#
set DEFPATH=/usr/ucb:/bin:/usr/bin
setenv DEFCPATH ${GCC_INSTALL_DIR}pmax-mach/2.3.3/include:/usr/include
set DEFLPATH=/usr/lib

#
# File formats
#
setenv OBJECT_FORMAT COFF
setenv ARCHIVE_FORMAT BSDARCH

# compiler options -- ultrix 4.2 cc is not _STDC_
# it works for the buildtools, but not the kernel
#setenv PMAX_CC cc
#setenv PMAX_ASCPP "cc -E"
#setenv PMAX_CPP /usr/lib/cpp
#setenv NOT_USING_GCC2
#setenv DEF_CCFLAGS  "-DMACH -DCMU -Dconst="
# overide setting of -MD in osf.mach3.mk until local compiler supports it
#setenv DEF_MIGFLAGS "-DMACH -DCMU"
#setenv DEF_CCFLAGS  "-DMACH -DCMU -Dconst="

setenv NOSHARED_LIBRARIES NOSHARED_LIBRARIES

#
# Porting flags - used by some ODE tools to avoid using
# features that the current system doesn't support
# Set PORTING_FLAGS to the ones your system is missing
#
# NO_DIRENT used by md.c
# NO_SETENV
# NO_STRDUP used by makepath.c
# NO_SYS_LIMITS used by makepath.c
# NO_UTIMES used by release.c (if it is defined, 
#	includes <utime.h> and uses struct utimbuf)
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
setenv NO_STRDUP NO_STRDUP
#setenv NO_STRERROR NO_STRERROR
setenv NO_UTIME NO_UTIME
setenv NO_WAITPID NO_WAITPID

