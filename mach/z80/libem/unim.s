.define unimpld, e.mon, e.rck, .trp.z, .unimpld

.unimpld:
unimpld:		! used in dispatch table to
			! catch unimplemented instructions
	ld hl,EILLINS
9:	push hl
	call .trp.z
	jp 20

e.mon:
	ld hl,EMON
	jr 9b
e.rck:
	push af
	ld a,(ignmask)
	bit 1,a
	jr nz,8f
	ld hl,ERANGE
	jr 9b
8:
	pop af
	ret


