.define	.diagnos
.sect .text
.sect .rom
.sect .data
.sect .bss

space	= 040
del	= 0177

	.sect .text
.diagnos:
	move.l	hol0,-(sp)
	move.l	hol0+4,d2
	beq	1f
	move.l	d2,a0
	move.l	#40,d0
3:
	move.b	(a0)+,d1
	beq	2f
	cmp.b	#del,d1
	bge	1f
	cmp.b	#space,d1
	blt	1f
	sub	#1,d0
	bgt	3b
	clr.b	(a0)
2:
	move.l	d2,-(sp)
	pea	fmt
	jsr	printf
	add	#12,sp
	jmp	printf

1:
	move.l	#unknwn,d2
	bra	2b

.sect .bss
getal:
	.space	12
char:
	.space	1
	.align  4
.sect .data
hexs:
	.ascii	"0123456789abcdef"
	.align	4
.sect .text
printf:
	movem.l	d0/d1/d2/a0/a1/a2/a3/a4/a5/a6, -(sp)
	lea	44(sp), a6	! a6 <- address of arguments
	move.l	(a6)+, a5	! a5 <- address of format
next:	move.b	(a5)+, d0
	beq	out
	cmp.b	#'%', d0
	beq	procnt
put:	move.l	d0, -(sp)
	jsr	putchar		! long argument on stack
	tst.l	(sp)+
	bra	next

procnt:	move.b	(a5)+, d0
	cmp.b	#'d', d0	! NOTE: %d means unsigned.
	beq	digit
	cmp.b	#'x', d0
	beq	hex
	cmp.b	#'s', d0
	beq	string
	cmp.b	#'%', d0	! second % has to be printed.
	beq	put
	tst.b	-(a5)		! normal char should be printed
	bra	next

string:	move.l	(a6)+, a2	! a2 <- address of string
sloop:	move.b	(a2)+, d0
	beq	next
	move.l	d0, -(sp)
	jsr	putchar		! long argument on stack
	tst.l	(sp)+
	bra	sloop

digit:	move.l	(a6)+, d1	! d1 <- integer
	move.l	#getal+12, a2	! a2 <- ptr to last part of buf
	clr.b	-(a2)		! stringterminator
1:	
	move.l	d1,-(sp)
	move.l	#10,-(sp)
	jsr	.dvu		! d1 <- qotient; d0 <- remainder
	add.l	#'0', d0
	move.b	d0, -(a2)
	tst.l	d1		! if quotient = 0 then ready
	bne	1b
	bra	sloop		! print digitstring.

hex:	move.l	(a6)+, d1	! d1 <- integer
	move.l	#getal+12, a2	! a2 <- ptr to last part of buf
	clr.b	-(a2)		! stringterminator
	move.l	#7, d2		! loop control
1:	move.l	d1, d0
	and.l	#15, d0
	move.l	#hexs,a0
	add.l	d0,a0
	move.b	(a0), -(a2) ! hex digit
	asr.l	#4, d1
	dbf	d2, 1b
	bra	sloop

out:
	movem.l	(sp)+, d0/d1/d2/a0/a1/a2/a3/a4/a5/a6
	rts


putchar:
	move.l	#1, -(sp)
	pea	11(sp)
	move.l	#1, -(sp)
	jsr	_write
	lea	12(sp), sp
	rts
.align 2
	.sect .data
fmt:	.asciz "%s, line %d: "
unknwn:	.asciz "unknown file"
.align 2
