/*
	Trap handling
*/

/* $Header$ */

#define	wtrap(wn,tr)	(warning(wn), trap(tr))
#define	trap(tr)	do_trap(tr, __LINE__, __FILE__)

extern int signalled;			/* signal nr if trap was due to sig */

extern int must_test;			/* must trap on overfl./out of range*/
					/* TEST-bit on in EM header word 2 */

