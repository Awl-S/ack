/ $Header$
.text
.globl sru~,slu~
.globl unknown~

sru~:
	neg	2(sp)
slu~:
	mov	(sp)+,r3
	cmp	r0,$02
	bgt	1f
	mov	2(sp),r1
	clr	r0
	ashc	(sp)+,r0
2:	mov	r1,-(sp)
	jmp	(r3)
1:	cmp	r0,$04
	bgt	3f
	mov	02(sp),r0
	mov	04(sp),r1
	tst	(sp)
	beq	4f
	ashc	$-1,r0
	bic	$0100000,r0
	inc	(sp)
	beq	4f
	ashc	(sp)+,r0
4:	mov	r0,(sp)
	mov	r1,02(sp)
	jmp	(r3)
3:	jmp	unknown~
