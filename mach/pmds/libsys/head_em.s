.define	CERASE,CKILL,CSTOP,CSTART
.define .lino,.filn
.define F_DUM,EXIT
.define	begtext,begdata,begbss
.define	EARRAY,ERANGE,ESET,EIDIVZ,EHEAP,EILLINS,ECASE
.define	hol0,.reghp,.limhp,.trpim,.trppc
.define	LINO_AD,FILN_AD
.sect .text
.sect .rom
.sect .data
.sect .bss
.sect .text




CERASE	= 010
CKILL	= 030
CSTART	= 021
CSTOP	= 023
F_DUM	= 0


LINO_AD	= 0
FILN_AD	= 4

EARRAY	= 0
ERANGE	= 1
ESET	= 2
EIDIVZ	= 6
EHEAP	= 17
EILLINS	= 18
ECASE	= 20

	.sect .text
begtext:
	add.l	#2,sp !convert argc from 4-byte to 2-byte
	jsr	_m_a_i_n
	add	#010,sp
EXIT:
	move.w	d0,-(sp)
	jsr	_exit

	.sect .data
begdata:
hol0:
.lino:
	.data2	0,0	! lino
.filn:
	.data4	0	! filn
.reghp:
	.data4	endbss
.limhp:
	.data4	endbss
.trppc:
	.data4	0
.trpim:
	.data2	0


	.sect .bss
begbss:
