/*
 * Mach Operating System
 * Copyright (c) 1993,1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 *  
 *
 * HISTORY
 * $Log:	__divl.s,v $
 * Revision 2.2  93/05/20  18:05:36  mrt
 * 	Created
 * 	[93/04/13            af]
 * 
*/

#include <mach/alpha/asm.h>

LEAF(__divl,2)
	lda	sp, -40(sp)
	stq	t10, 32(sp)
	stq	t11, 24(sp)
	stq	t3, 16(sp)
	stq	t2, 8(sp)
	stq	t1, 0(sp)
	beq	t11, gtrap
	addl	t10, 0, t10
	addl	t11, 0, t11
	addl	t11, 0x1, t1
	bis	t10, t10, t2
	cmovne	t1, 0, t2
	subl/v	zero, t2, t2
	xor	t10, t11, t3
	cmplt	t10, 0, t1
	bic	t3, 0x1, t3
	bis	t1, t3, t3
	subl	zero, t10, t1
	cmovlt	t10, t1, t10
	subl	zero, t11, t1
	cmovlt	t11, t1, t11
	addq	zero, 0x8, t1
	sll	t11, 0x20, t11
	zap	t10, 0xf0, t10
loop:
	addq	t10, t10, t10
	cmpule	t11, t10, t2
	addq	t10, t2, t10
	subq	t10, t11, t2
	cmovlbs	t10, t2, t10
	addq	t10, t10, t10
	cmpule	t11, t10, t2
	addq	t10, t2, t10
	subq	t10, t11, t2
	cmovlbs	t10, t2, t10
	addq	t10, t10, t10
	cmpule	t11, t10, t2
	addq	t10, t2, t10
	subq	t10, t11, t2
	cmovlbs	t10, t2, t10
	addq	t10, t10, t10
	cmpule	t11, t10, t2
	addq	t10, t2, t10
	subq	t10, t11, t2
	cmovlbs	t10, t2, t10
	subq	t1, 0x1, t1
	bne	t1, loop
	addl	t10, 0, t12
	subl	zero, t12, t1
	cmovlt	t3, t1, t12
	ldq	t1, 0(sp)
	ldq	t2, 8(sp)
	ldq	t3, 16(sp)
	ldq	t11, 24(sp)
	ldq	t10, 32(sp)
	lda	sp, 40(sp)
	ret	zero, (t9), 1
gtrap:
	addq	zero, 0x7, a0
	call_pal	0x81
	END(__divl)
