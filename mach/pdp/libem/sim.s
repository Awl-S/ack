/ $Header$
.text
.globl sim~
.globl trpim~

.float = 1

sim~:
	mov	(sp)+,r3
	mov	(sp)+,r0
	mov	r0,trpim~
.if .float
	stfps	r1
	bis	$07400,r1
	bit	$020,r0
	beq	0f
	bic	$01000,r1
0:	bit	$040,r0
	beq	0f
	bic	$02000,r1
0:	bit	$01000,r0
	beq	0f
	bic	$04000,r1
0:	bit	$02000,r0
	beq	0f
	bic	$0400,r1
0:	ldfps	r1
.endif
	jmp	(r3)
