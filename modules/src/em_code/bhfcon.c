#include "em_private.h"

CC_bhfcon(op, n, s, sz, i)
	arith n;
	char *s;
	arith sz;
	int i;
{
	/*	BSS or HOL with size n, initial value an FCON (s, sz),
		and flag i
	*/
	PS(op);
	CST(n);
	COMMA();
	WCON(sp_fcon, s, sz);
	COMMA();
	CST((arith) i);
	NL();
}
