/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
/* $Header$ */

#include <system.h>

char *sbrk();

char *
sys_break(incr)
	int incr;
{
	char *sbrk();
	char *brk = sbrk(incr);

	if (brk == (char *) 0 || brk == (char *)-1)
		return ILL_BREAK;
	return brk;
}
