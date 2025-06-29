/*
 * Distributed as part of the Mach Operating System
 */
/*
 * @OSF_FREE_COPYRIGHT@
 * 
 * Copyright (c) 1990, 1991
 * Open Software Foundation, Inc.
 * 
 * Permission is hereby granted to use, copy, modify and freely distribute
 * the software in this file and its documentation for any purpose without
 * fee, provided that the above copyright notice appears in all copies and
 * that both the copyright notice and this permission notice appear in
 * supporting documentation.  Further, provided that the name of Open
 * Software Foundation, Inc. ("OSF") not be used in advertising or
 * publicity pertaining to distribution of the software without prior
 * written permission from OSF.  OSF makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */
/*
 * HISTORY
 * $Log:	buf.h,v $
 * Revision 2.2  92/05/20  20:12:24  mrt
 * 	First checkin
 * 	[92/05/20  16:29:31  mrt]
 * 
 */
/*
 * Copyright (c) 1988, 1989, 1990 The Regents of the University of California.
 * Copyright (c) 1988, 1989 by Adam de Boor
 * Copyright (c) 1989 by Berkeley Softworks
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)buf.h	5.4 (Berkeley) 12/28/90
 */

/*-
 * buf.h --
 *	Header for users of the buf library.
 */

#ifndef _BUF_H
#define _BUF_H

#include    "sprite.h"

typedef unsigned char Byte;

typedef struct Buffer {
    int	    size; 	/* Current size of the buffer */
    int     left;	/* Space left (== size - (inPtr - buffer)) */
    Byte    *buffer;	/* The buffer itself */
    Byte    *inPtr;	/* Place to write to */
    Byte    *outPtr;	/* Place to read from */
} *Buffer;

Buffer	    	  Buf_Init();	    /* Initialize a buffer */
void	    	  Buf_Destroy();    /* Destroy a buffer */
void	    	  Buf_AddBytes();   /* Add a range of bytes to a buffer */
int	    	  Buf_GetByte();    /* Get a byte from a buffer */
int	    	  Buf_GetBytes();   /* Get multiple bytes */
void		  Buf_UngetByte();  /* Push a byte back into the buffer */
void		  Buf_UngetBytes(); /* Push many bytes back into the buf */
Byte	    	  *Buf_GetAll();    /* Get them all */
void	    	  Buf_Discard();    /* Throw away some of the bytes */
int	    	  Buf_Size();	    /* See how many are there */

/* Buf_AddByte adds a single byte to a buffer. */
#define	Buf_AddByte(bp, byte) { \
    if (--(bp)->left <= 0) \
	Buf_OvAddByte(bp, byte); \
    else { \
	*(bp)->inPtr++ = (byte); \
	*(bp)->inPtr = 0; \
    } \
}

void	Buf_OvAddByte();		/* adds a byte when buffer overflows */

#define BUF_ERROR 256

#endif _BUF_H
