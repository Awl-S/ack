/*
 * Sources of the "PROCEDURE CALL" group instructions
 */

/* $Header$ */

#include	<em_abs.h>
#include	"logging.h"
#include	"global.h"
#include	"log.h"
#include	"mem.h"
#include	"shadow.h"
#include	"memdirect.h"
#include	"trap.h"
#include	"warn.h"
#include	"text.h"
#include	"proctab.h"
#include	"fra.h"
#include	"rsb.h"
#include	"linfil.h"

extern int running;			/* from main.c */

PRIVATE lfr(), ret();

DoCAI()				/* proc identifier on top of stack */
{
	/* CAI -: Call procedure (procedure identifier on stack) */
	register long pi = spop(psize);

	LOG(("@P6 DoCAI(%lu)", pi));
	call(arg_p(pi), RSB_CAL);
}

DoCAL(pi)
	register long pi;
{
	/* CAL p: Call procedure (with identifier p) */

	LOG(("@P6 DoCAL(%lu)", pi));
	call(arg_p(pi), RSB_CAL);
}

DoLFR(l)
	register size l;
{
	/* LFR s: Load function result */

	LOG(("@P6 DoLFR(%ld)", l));
	lfr(arg_s(l));
}

DoRET(l)
	register size l;
{
	/* RET z: Return (function result consists of top z bytes) */

	LOG(("@P6 DoRET(%ld)", l));
	ret(arg_z(l));
}

/************************************************************************
 *		Calling a new procedure.				*
 ************************************************************************/

call(new_PI, rsbcode)
	long new_PI;
	int rsbcode;
{
	/* legality of new_PI has already been checked */
	register size nloc = proctab[new_PI].pr_nloc;
	register ptr ep = proctab[new_PI].pr_ep;

	push_frame(SP);			/* remember AB */
	pushrsb(rsbcode);

	/* do the call */
	PI = new_PI;
	st_inc(nloc);
	newPC(ep);
	spoilFRA();
	LOG(("@p5 call: new_PI = %lu, nloc = %lu, ep = %lu",
				new_PI, nloc, ep));
}

/************************************************************************
 *		Loading a function result.				*
 ************************************************************************/

PRIVATE lfr(sz)
	size sz;
{
	if (sz > FRALimit) {
		wtrap(WILLLFR, EILLINS);
	}

	LOG(("@p5 lfr: size = %ld", sz));

#ifdef	LOGGING
	if (!FRA_def) {
		warning(WRFUNGAR);
	}
	if (sz != FRASize) {
		warning(FRASize < sz ? WRFUNSML : WRFUNLAR);
	}
#endif	LOGGING

	pushFRA(sz);
	spoilFRA();
}

/************************************************************************
 *		Returning from a procedure.				*
 ************************************************************************/

PRIVATE ret(sz)
	size sz;
{
	if (sz > FRALimit) {
		wtrap(WILLRET, EILLINS);
	}

	LOG(("@p5 ret: size = %ld", sz));

	/* retrieve return value from stack */
	FRA_def = DEFINED;
	FRASize = sz;
	popFRA(FRASize);

	switch (poprsb(0)) {
	case RSB_STP:
		if (sz == wsize) {
			ES_def = DEFINED;
			ES = btol(FRA[sz-1]);
					/* one byte only */
		}
		running = 0;		/* stop the machine */
		return;
	case RSB_CAL:
		/* OK */
		break;
	case RSB_RTT:
	case RSB_NRT:
		warning(WRETTRAP);
		running = 0;		/* stop the machine */
		return;
	default:
		warning(WRETBAD);
		return;
	}
}

