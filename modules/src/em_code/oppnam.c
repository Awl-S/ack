#include "em_private.h"

/* $Header$ */

CC_oppnam(opcode, pnam)
	char *pnam;
{
	/*	Instruction that has a procedure name as argument
		Argument types: p
	*/
	OP(opcode);
	PNAM(pnam);
	NL();
}
