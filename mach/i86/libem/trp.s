.define .trp

        ! $Header$
		! ax is trap number
.trp:
	xor     bx,bx
	xchg    bx,(.trppc)
	test    bx,bx
	jz      2f
	push    ax
	call    bx
	pop     ax
	ret
2:
	call    .stop
