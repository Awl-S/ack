#include "em_private.h"

CC_crilb(op, l)
	label l;
{
	/*	CON or ROM with argument ILB(l)
	*/
	PS(op);
	ILB(l);
	CEND();
	NL();
}
