#include "em_private.h"

/* $Header$ */

CC_opdnam(opcode, dnam, offset)
	char *dnam;
	arith offset;
{
	/*	Instruction that has a datalabel + offset as argument
		Argument types: g
	*/
	OP(opcode);
	NOFF(dnam, offset);
	NL();
}
