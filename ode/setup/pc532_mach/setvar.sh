#!/bin/sh
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
# $Log:	setvar.sh,v $
# Revision 2.2  93/05/07  23:33:37  mrt
# 	Created by Ian Dall
# 	[93/04/29            mrt]
# 

#
# Setup variables describing the current host environment
# The target machine is assumed to be the same as the host machine
# unless target_machine is set. The target OS is assumed to be Mach
#

ANSI_CC=gcc; export ANSI_CC
TRADITIONAL_CC="gcc -traditional"; export TRADITIONAL_CC
PC532_OPT_LEVEL=-O2; export PC532_OPT_LEVEL
NROFF=:; export NROFF
XSTRIP="strip -x"; export XSTRIP

HOST_MACHINE="PC532"; export HOST_MACHINE
host_machine="pc532"; export host_machine
# target_machine may be set on command line if we are cross-building
target_machine=${target_machine:-pc532}; export target_machine

# TARGET_MACHINE names subdirectories with machine specific files
TARGET_MACHINE="`echo ${target_machine} |  tr a-z A-Z`"; export TARGET_MACHINE
#TARGET_CPU=${TARGET_MACHINE}; export TARGET_CPU
TARGET_CPU=NS532; export TARGET_CPU
#target_cpu is used as the name of the kernel directory that "machine"
#links to unless it it overridden by KERN_MACHINE_DIR
#target_cpu=${target_machine}; export target_cpu
target_cpu=ns532; export target_cpu
TARGET_OS="MACH"; export TARGET_OS
target_os="mach"; export target_os
target_context="${target_machine}_${target_os}"

#
# Default search paths
#  These are last resort directories on the HOST_MACHINE
#  These values are correct for a CMUCS i386 machine.
#
DEFPATH=/usr/local/bin:/usr/gnu/bin:/usr/ucb:/bin:/usr/bin; export DEFPATH
DEFCPATH=/usr/gnu/lib/gcc-lib/pc532-mach/2.3.2/include:/usr/include; export DEFCPATH
DEFLPATH=/usr/lib:/lib; export DEFLPATH

#
# File formats - used by ODE tools makefiles
#
OBJECT_FORMAT=A_OUT; export OBJECT_FORMAT
ARCHIVE_FORMAT=BSDARCH; export ARCHIVE_FORMAT

NOSHARED_LIBRARIES=NOSHARED_LIBRARIES; export NSHARED_LIBRARIES

#
# Porting flags - used by some ODE tools to avoid using
# features that the current system doesn't support
# Set PORTING_FLAGS to the ones your system is missing
#
# NO_DIRENT used by md.c
# NO_SETENV
# NO_STRDUP used by makepath.c
# NO_SYS_LIMITS used by makepath.c
# NO_STRERROR used by makepath.c
# NO_UTIMES used by release.c
# NO_VPRINTF used by md.c, release.c

PORTING_FLAGS="-DNO_STRDUP -DNO_SYS_LIMITS -DNO_DIRENT"; export PORTING_FLAGS

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
#NO_GETOPT=NO_GETOPT
#export NO_GETOPT
#NO_SETENV=NO_SETENV
#export NO_SETENV
# The external i386-Tahoe release is missing this
NO_STRDUP=NO_STRDUP
export NO_STRDUP
#NO_STRERROR=NO_STRERROR
#export NO_STRERROR
NO_WAITPID=NO_WAITPID
export NO_WAITPID
# CMU-CS has old one that doesn't use the utime structure
NO_UTIME=NO_UTIME
export NO_UTIME

