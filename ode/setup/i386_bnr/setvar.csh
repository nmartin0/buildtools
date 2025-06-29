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
# Revision 2.4  93/06/17  16:44:53  mrt
# 	Changed ar,ranlib and ldx to be found on PATH.
# 	Set i386_LDLIBS to null as our version of arx doesn't
# 	like the __.SYMDEFS of gcc 1.39.
# 	Defined OWNER and GROUP to overide default values of cs
# 	[93/06/14            mrt]
# 
# Revision 2.3  93/06/01  22:20:57  mrt
# 	Updated setting of target_machine et al. Changed DEFPATH
# 	Defined NOT_USING_GCC2. Added -DMACH and -DCMU to DEF_CCFLAGS
# 	and DEF_MIGFLAGS.
# 	[93/05/07            mrt]
# 
# Revision 2.2  92/07/08  18:06:08  mrt
# 	Set ASCPP to /usr/bin/cpp and set DEF_MIGFLAGS to overide -MD
# 	[92/07/03            mrt]
# 
# 	$EndLog$
# 

#
# Setup variables describing the current environment
#

setenv HOST_MACHINE "I386"
setenv host_machine "i386"
# target_machine may be set on command line if we are cross-building
if ( ! $?target_machine ) then
    setenv target_machine "i386"
endif
# TARGET_MACHINE names subdirectories with machine specific files
setenv TARGET_MACHINE "`echo ${target_machine} |  tr a-z A-Z`"
setenv TARGET_CPU ${TARGET_MACHINE}
#target_cpu is used as the name of the kernel directory that "machine"
#links to unless it it overridden by KERN_MACHINE_DIR
setenv target_cpu ${target_machine}
setenv TARGET_OS "MACH"
setenv target_os "mach"
set target_context="${target_machine}_${target_os}"

#
# OWNER and GROUP are used only during install step
#
setenv OWNER bin
setenv GROUP bin

#
# Default search path 
#
setenv DEFPATH /usr/bin:/usr/sbin:/bin:/sbin
setenv DEFCPATH /usr/include
setenv DEFLPATH /usr/lib

#
# File formats
#
setenv OBJECT_FORMAT A_OUT
setenv ARCHIVE_FORMAT BSDARCH

#
#  Compiler choices
#
setenv I386_CC cc
setenv I386_ASCPP "cc -E"
setenv I386_CPP /usr/bin/cpp
# overide setting of -MD in osf.mach3.mk until local compiler supports it
setenv DEF_CCFLAGS  "-DMACH -DCMU"
setenv DEF_MIGFLAGS "-DMACH -DCMU"
setenv NOT_USING_GCC2
setenv NROFF echo
setenv I386_AR arx
setenv I386_RANLIB ranlibx
setenv I386_LD ldx
setenv I386_LDLIBS 

setenv NOSHARED_LIBRARIES NOSHARED_LIBRARIES

#
# Porting flags - used by some ODE tools to avoid using
# features that the current system doesn't support
# Set PORTING_FLAGS to the ones your system is missing
#
# NO_DIRENT used by md.c
# NO_SETENV
# NO_STRDUP used by makepath.c
# NO_SYSLIMITS used by makepath.c
# NO_UTIMES used by release.c
# NO_VPRINTF used by md.c, release.c

setenv PORTING_FLAGS "-DNO_SYS_LIMITS -DNO_DIRENT -DBNR"

#
# Define these if your system  does not have these entries
# in libc.a. Make needs them and will get them from its 
# porting directory if the NO_ environment variable is set
#

#setenv NO_FSS NOFSS
#setenv NO_GETCWD NO_GETCWD
#setenv NO_GETOPT NO_GETOPT
#setenv NO_SETENV NO_SETENV
#setenv NO_STRDUP NO_STRDUP
#setenv NO_STRERROR NO_STRERROR
#setenv NO_WAITPID NO_WAITPID

