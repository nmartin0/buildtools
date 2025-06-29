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
# Revision 2.2  93/05/11  18:05:00  mrt
# 	Created by Michael Uhlenberg
# 	[93/04/30            mrt]
# 
# 

#
# Setup variables describing the current host environment
# The target machine is assumed to be the same as the host machine
# unless target_machine is set. The target OS is assumed to be Mach
#

#HOST_MACHINE is used to chose compiler programs if CCTYPE-host
setenv HOST_MACHINE "ALPHA"
setenv host_machine "alpha"
#target_machine is used as the name of the kernel directory that "machine"
#links to unless it it overridden by KERN_MACHINE_DIR
if ( ! $?target_machine ) then 
    setenv target_machine "alpha"
endif
# TARGET_MACHINE names subdirectories with machine specific files
setenv TARGET_MACHINE "`echo ${target_machine} |  tr a-z A-Z`"
setenv TARGET_CPU ${TARGET_MACHINE}
setenv target_cpu ${target_machine}
setenv TARGET_OS "MACH"
setenv target_os "mach"
set target_context="${target_machine}_${target_os}"

#
# Default search paths
#  These are last resort directories on the HOST_MACHINE
#  These values are correct for an OSF/1 alpha machine
#
set DEFPATH=/usr/ucb:/bin:/usr/bin:/usr/bin/X11:/usr/local/bin:/sbin:.
setenv DEFCPATH /usr/local/lib/gcc-lib/alpha-dec-osf1/2.3.3/include:/usr/include
set DEFLPATH=/usr/lib

#
# File formats - used by ODE tools makefiles
#
setenv OBJECT_FORMAT COFF
setenv ARCHIVE_FORMAT BSDARCH

setenv NOSHARED_LIBRARIES NOSHARED_LIBRARIES

#
# Porting flags - used by some ODE tools to avoid using
# features that the current system doesn't support
# Set PORTING_FLAGS to the ones your system is missing
#
# NO_DIRENT used by md.c
# NO_SETENV
# NO_STRDUP used by makepath.c
# NO_STRERROR used by makepath.c
# NO_SYS_LIMITS used by makepath.c
# NO_UTIMES used by release.c
# NO_VPRINTF used by md.c, release.c

setenv PORTING_FLAGS "-DNO_SYS_LIMITS -DNO_DIRENT"

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
#setenv NO_UTIME NO_UTIME
#setenv NO_WAITPID NO_WAITPID


#PCS overriding definitions in osf*mk
#setenv ALPHA_AS as
# instead of gcc -nostdlib
setenv ALPHA_LD ld
setenv CC_OPT_LEVEL "-O2"
# the PCS gets Michael's changes - should be OSF1
setenv ALPHA_CFLAGS "-DPCS"
