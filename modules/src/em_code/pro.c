#include "em_private.h"

/* $Header$ */

CC_pro(pnam, l)
	char *pnam;
	arith l;
{
	/*	PRO pseudo with procedure name pnam and # of locals l
	*/
	PS(ps_pro);
	PNAM(pnam);
	COMMA();
	CST(l);
	NL();
}
