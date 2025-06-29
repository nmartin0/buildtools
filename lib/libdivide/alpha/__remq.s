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
 * $Log:	__remq.s,v $
 * Revision 2.2  93/05/20  18:07:03  mrt
 * 	Created
 * 	[93/04/13            af]
 * 
*/

#include <mach/alpha/asm.h>

LEAF(__remq,2)
	lda	sp, -48(sp)
	stq	t10, 40(sp)
	stq	t11, 32(sp)
	stq	t4, 24(sp)
	stq	t3, 16(sp)
	stq	t2, 8(sp)
	stq	t1, 0(sp)
	beq	t11, gtrap
	addq	t11, 0x1, t1
	bis	t10, t10, t2
	cmovne	t1, 0, t2
	subq/v	zero, t2, t2
	xor	t10, t11, t3
	cmplt	t10, 0, t1
	bic	t3, 0x1, t3
	bis	t1, t3, t3
	subq	zero, t10, t1
	cmovlt	t10, t1, t10
	subq	zero, t11, t1
	cmovlt	t11, t1, t11
	bis	zero, zero, t2
	addq	zero, 0x10, t4
loop:
	cmplt	t10, 0, t1
	addq	t10, t10, t10
	addq	t2, t2, t2
	bis	t2, t1, t2
	cmpule	t11, t2, t1
	addq	t10, t1, t10
	subq	t2, t11, t1
	cmovlbs	t10, t1, t2
	cmplt	t10, 0, t1
	addq	t10, t10, t10
	addq	t2, t2, t2
	bis	t2, t1, t2
	cmpule	t11, t2, t1
	addq	t10, t1, t10
	subq	t2, t11, t1
	cmovlbs	t10, t1, t2
	cmplt	t10, 0, t1
	addq	t10, t10, t10
	addq	t2, t2, t2
	bis	t2, t1, t2
	cmpule	t11, t2, t1
	addq	t10, t1, t10
	subq	t2, t11, t1
	cmovlbs	t10, t1, t2
	cmplt	t10, 0, t1
	addq	t10, t10, t10
	addq	t2, t2, t2
	bis	t2, t1, t2
	cmpule	t11, t2, t1
	addq	t10, t1, t10
	subq	t2, t11, t1
	cmovlbs	t10, t1, t2
	subq	t4, 0x1, t4
	bne	t4, loop
	subq	zero, t2, t12
	cmovlbc	t3, t2, t12
	ldq	t1, 0(sp)
	ldq	t2, 8(sp)
	ldq	t3, 16(sp)
	ldq	t4, 24(sp)
	ldq	t11, 32(sp)
	ldq	t10, 40(sp)
	lda	sp, 48(sp)
	ret	zero, (t9), 1
gtrap:
	addq	zero, 0x7, a0
	call_pal	0x81
	END(__remq)
