/ $Header$
.text
.globl rmu~
.globl rmu2~,rmu4~,unknown~

rmu~:
	mov	(sp)+,r3
	cmp	r0,$04
	bgt	1f
	beq	2f
	cmp	r0,$02
	bne	1f
	jsr	pc,rmu2~
	mov	r1,-(sp)
	jmp	(r3)
2:	jsr	pc,rmu4~
	mov	r1,-(sp)
	mov	r0,-(sp)
	jmp	(r3)
1:	jmp	unknown~
