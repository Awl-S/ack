        # $Header$
#include "em_abs.h"

.globl  .dvu4

.dvu4:
	movl    (sp)+,r3
	movl    (sp)+,r2
	blss    L1
	movl    (sp)+,r0
	clrl    r1
	ediv    r2,r0,r0,r1
	jmp     (r3)
L1:
	cmpl    (sp)+,r2
	bgequ   L2
	clrl	r0
	jmp     (r3)
L2:
	movl	$1,r0
	jmp     (r3)
