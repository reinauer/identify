*
* Internal 68000 / Kickstart 1.3 helpers. LGPL-3.0-or-later.
* Register interfaces match the library calls they replace.
*

		INCLUDE exec/memory.i
		INCLUDE exec/execbase.i
		INCLUDE utility/tagitem.i
		SECTION text,CODE
		MACHINE 68000

		public _CompatNextTagItem,_CompatGetTagData
_CompatNextTagItem
		move.l	(a0),a1
.next		move.l	a1,d0
		beq	.end
		move.l	(a1),d0
		beq	.end
		cmp.l	#TAG_IGNORE,d0
		beq	.skip
		cmp.l	#TAG_MORE,d0
		beq	.more
		cmp.l	#TAG_SKIP,d0
		beq	.skipmany
		move.l	a1,d0
		addq.l	#8,a1
		move.l	a1,(a0)
.done		rts
.more		move.l	4(a1),a1
		bra	.next
.skipmany	move.l	4(a1),d0
		lsl.l	#3,d0
		add.l	d0,a1
.skip		addq.l	#8,a1
		bra	.next
.end		clr.l	(a0)
		moveq	#0,d0
		rts

_CompatGetTagData
		movem.l	d2-d3/a2,-(sp)
		move.l	d0,d2
		move.l	d1,d3
		move.l	a0,-(sp)
.loop		move.l	sp,a0
		bsr	_CompatNextTagItem
		tst.l	d0
		beq	.default
		move.l	d0,a2
		cmp.l	(a2),d2
		bne	.loop
		move.l	4(a2),d0
		bra	.done
.default	move.l	d3,d0
.done		addq.l	#4,sp
		movem.l	(sp)+,d2-d3/a2
		rts

		public _CompatAllocVec,_CompatFreeVec
_CompatAllocVec
		movem.l	d2/a6,-(sp)
		tst.l	d0
		beq	.done
		addq.l	#8,d0		; retain AllocMem's eight-byte alignment
		bcs	.failed
		move.l	d0,d2
		move.l	4.w,a6
		jsr	-198(a6)		; AllocMem
		tst.l	d0
		beq	.done
		move.l	d0,a0
		move.l	d2,(a0)
		addq.l	#8,d0
		bra	.done
.failed		moveq	#0,d0
.done		movem.l	(sp)+,d2/a6
		rts

_CompatFreeVec	move.l	a6,-(sp)
		move.l	a1,d0
		beq	.done
		subq.l	#8,a1
		move.l	(a1),d0
		move.l	4.w,a6
		jsr	-210(a6)		; FreeMem
.done		move.l	(sp)+,a6
		rts

		public _CompatUMult32,_CompatUDivMod32
_CompatUMult32	move.l	a0,-(sp)
		move.l	4.w,a0
		btst	#AFB_68020,(AttnFlags+1,a0)
		move.l	(sp)+,a0
		beq	.slow
		MACHINE 68020
		mulu.l	d1,d0
		rts
		MACHINE 68000
.slow		movem.l d2-d3,-(sp)
		move.l	d0,d2
		move.l	d1,d3
		swap	d2
		mulu.w	d1,d2
		swap	d3
		mulu.w	d0,d3
		mulu.w	d1,d0
		add.w	d3,d2
		swap	d2
		clr.w	d2
		add.l	d2,d0
		movem.l	(sp)+,d2-d3
		rts

_CompatUDivMod32
		move.l	a0,-(sp)
		move.l	4.w,a0
		btst	#AFB_68020,(AttnFlags+1,a0)
		move.l	(sp)+,a0
		beq	.slow
		MACHINE 68020
		divul.l	d1,d1:d0		; 32-bit dividend, quotient and remainder
		rts
		MACHINE 68000
.slow		movem.l	d2-d3,-(sp)
		tst.l	d1
		bne	.start
		divu.w	d1,d0		; preserve the divide-by-zero exception
.start		moveq	#0,d2
		moveq	#31,d3
.loop		add.l	d0,d0
		addx.l	d2,d2
		bcs	.subtract
		cmp.l	d1,d2
		blo	.next
.subtract	sub.l	d1,d2
		addq.l	#1,d0
.next		dbra	d3,.loop
		move.l	d2,d1
		movem.l	(sp)+,d2-d3
		rts

		public _CompatCountChar,_CompatPutChar
_CompatCountChar addq.l #1,(a3)
		rts
_CompatPutChar	move.b d0,(a3)+
		rts
