.define	.strhp
.sect .text
.sect .rom
.sect .data
.sect .bss
EHEAP=17

	.sect .text
.strhp:
	move.l	(sp)+, a0
	move.l	(sp), d1	! new heap pointer
	cmp.l	(.limhp), d1	! compare new heap pointer with limit
	blt	1f
	add.l	#0x400, d1
	and.l	#~0x3ff, d1
	move.l	d1, (.limhp)
	move.l	d1, -(sp)
	jsr	(_brk)		! allocate 1K bytes of extra storage
	add.l	#4, sp
	bcs	2f
1:
	move.l	(sp)+, (.reghp)	! store new value
	jmp	(a0)		! return
2:
	move.l	#EHEAP, -(sp)
	jsr	(.trp)
	jmp	(a0)
.align 2
