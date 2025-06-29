#!/bin/sh
#
# Mach Operating System
# Copyright (c) 1993, 1992 Carnegie Mellon University
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
# $Log:	setup.sh,v $
# Revision 2.2  93/02/06  17:33:45  mrt
# 	Added some comments
# 	[93/01/10            mrt]
# 
# Revision 2.2  92/05/20  20:16:32  mrt
# 	First checkin
# 	[92/05/20            mrt]

#
# Setup variables describing the current environment
#

# TARGET_MACHINE names subdirectories with machine specific files
TARGET_MACHINE="PMAX"
#target_machine is used as the name of the kernel directory that "machine"
#links to unless it it overridden by KERN_MACHINE_DIR
target_machine="pmax"
TARGET_CPU="MIPS"
target_cpu="mips"
TARGET_OS="MACH"
target_os="mach"
KERN_MACHINE_DIR="mips"

export MACHINE TARGET_MACHINE target_machine TARGET_CPU target_cpu TARGET_OS target_os KERN_MACHINE_DIR

#
# Default search paths
#
DEFPATH=/usr/cs/bin:/usr/ucb:/bin:/usr/bin
DEFCPATH=/usr/cs/include:/usr/include
DEFLPATH=/usr/cs/lib:/usr/lib

#
# File formats
#
OBJECT_FORMAT=COFF
ARCHIVE_FORMAT=BSDARCH

export ARCHIVE_FORMAT OBJECT_FORMAT

#
#  Compiler choices
#
CC=gcc
ASCPP="cc -ES"
ASFLAGS=-nocpp

export CC ASCPP ASFLAGS

NOSHARED_LIBRARIES=NOSHARED_LIBRARIES
export NOSHARED_LIBRARIES

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

PORTING_FLAGS="-DNO_STRDUP -DNO_SYS_LIMITS -DNO_DIRENT"

#
# Define these if your system  does not have these entries
# in libc.a. Make needs them and will get them from its 
# porting directory if the NO_ environment variable is set
#

#NO_FSS=NO_FSS
#export NO_FSS
NO_GETCWD=NO_GETCWD
export NO_GETCWD
# The getopt that the CMU-CS libc.a is too old
NO_GETOPT=NO_GETOPT
export NO_GETOPT
#NO_SETENV=NO_SETENV
#export NO_SETENV
#NO_STRDUP=NO_STRDUP
#export NO_STRDUP
#NO_STRERROR=NO_STRERROR
#export NO_STRERROR
NO_WAITPID=NO_WAITPID
export NO_WAITPID
# CMU-CS has old one that doesn't use the utime structure
NO_UTIME=NO_UTIME
export NO_UTIME
