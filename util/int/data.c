/*
	Data access
*/

/* $Header$ */

#include	<em_abs.h>
#include	"logging.h"
#include	"nofloat.h"
#include	"global.h"
#include	"log.h"
#include	"trap.h"
#include	"warn.h"
#include	"alloc.h"
#include	"memdirect.h"
#include	"mem.h"
#include	"shadow.h"

#define	HEAPSIZE	1000L		/* initial heap size */

extern size maxheap;			/* from main.c */

#ifdef	LOGGING
char *data_sh;				/* shadowbytes */
#endif	LOGGING

PRIVATE warn_dtbits();

init_data(hb)
	ptr hb;
{
	HB = hb;			/* set Heap Base */
	HP = HB;			/* initialize Heap Pointer */
	HL = HB + HEAPSIZE;		/* initialize Heap Limit */

	data = Malloc((size)p2i(HL), "data space");
#ifdef	LOGGING
	data_sh = Malloc((size)p2i(HL), "shadowspace for data");
	dt_clear_area(i2p(0), HL);
#endif	LOGGING
}


/********************************************************
 *	EM-register division.				*
 ********************************************************
 *							*
 *	newHP(p)     - check and adjust HeapPointer.	*
 *							*
 ********************************************************/

newHP(ap)
	ptr ap;
{
	register ptr p = ap;

	if (in_gda(p)) {
		wtrap(WHPGDA, EHEAP);
	}
	if (in_stack(p)) {
		wtrap(WHPSTACK, EHEAP);
	}
	if (!is_wordaligned(p)) {
		wtrap(WHPODD, EHEAP);
	}
	if (maxheap) {
		/* more than allowed on command line */
		if (p - HB > maxheap) {
			warning(WEHEAP);
			trap(EHEAP);
		}
	}
	if (p > HL) {
		/* extend heap space */
		HL = i2p(allocfrac(p2i(p)) - 1);
		data = Realloc(data, (size)(p2i(HL) + 1), "heap space");
#ifdef	LOGGING
		data_sh = Realloc(data_sh, (size)(p2i(HL) + 1),
						"shadowspace for heap");
#endif	LOGGING
	}

#ifdef	LOGGING
	if (p > HP) {
		dt_clear_area(HP, p);
	}
#endif	LOGGING
	HP = p;
}

/************************************************************************
 *	Data store division.						*
 ************************************************************************
 *									*
 *	dt_stdp(addr, p)	- STore Data Pointer.			*
 *	dt_stn(addr, l, n)	- STore N byte integer.			*
 *	dt_stw(addr, l)		- STore wsize byte integer.		*
 *	dt_stf(addr, f, n)	- STore n byte Floating point number.	*
 *									*
 ************************************************************************/

dt_stdp(addr, ap)
	register ptr addr;
	ptr ap;
{
	register int i;
	register long p = (long) ap;

	LOG(("@g6 dt_stdp(%lu, %lu)", addr, p));
	ch_in_data(addr, psize);
	ch_wordaligned(addr);
	for (i = (int) psize; i > 0; i--, addr++) {
		ch_dt_prot(addr);
		data_loc(addr) = (char) (p);
		dt_dp(addr);
		p = p>>8;
	}
}

dt_stip(addr, ap)
	register ptr addr;
	ptr ap;
{
	register int i;
	register long p = (long) ap;

	LOG(("@g6 dt_stip(%lu, %lu)", addr, p));
	ch_in_data(addr, psize);
	ch_wordaligned(addr);
	for (i = (int) psize; i > 0; i--, addr++) {
		ch_dt_prot(addr);
		data_loc(addr) = (char) (p);
		dt_ip(addr);
		p = p>>8;
	}
}

dt_stn(addr, al, n)
	register ptr addr;
	long al;
	size n;
{
	register int i;
	register long l = al;
#ifdef LOGGING
	/* a psize zero is ambiguous */
	int sh_flags = (l == 0 && n == psize) ? (SH_INT|SH_DATAP) : SH_INT;
#endif LOGGING

	LOG(("@g6 dt_stn(%lu, %lu, %lu)", addr, l, n));
	ch_in_data(addr, n);
	ch_aligned(addr, n);
	for (i = (int) n; i > 0; i--, addr++) {
		ch_dt_prot(addr);
		data_loc(addr) = (char) l;
#ifdef LOGGING
		dt_sh(addr) = sh_flags;
#endif	LOGGING
		l = l>>8;
	}
}

dt_stw(addr, al)
	register ptr addr;
	long al;
{
	register int i;
	register long l = al;
#ifdef LOGGING
	/* a psize zero is ambiguous */
	int sh_flags = (l == 0 && wsize == psize) ? (SH_INT|SH_DATAP) : SH_INT;
#endif LOGGING

	LOG(("@g6 dt_stw(%lu, %lu)", addr, l));
	ch_in_data(addr, wsize);
	ch_wordaligned(addr);
	for (i = (int) wsize; i > 0; i--, addr++) {
		ch_dt_prot(addr);
		data_loc(addr) = (char) l;
#ifdef LOGGING
		dt_sh(addr) = sh_flags;
#endif	LOGGING
		l = l>>8;
	}
}

#ifndef	NOFLOAT
dt_stf(addr, f, n)
	register ptr addr;
	double f;
	register size n;
{
	register char *cp = (char *) &f;
	register int i;
	float fl;

	LOG(("@g6 dt_stf(%lu, %g, %lu)", addr, f, n));
	ch_in_data(addr, n);
	ch_wordaligned(addr);
	if ((int) n == 4) {
		fl = f;
		cp = (char *) &fl;
	}
	for (i = (int) n; i > 0; i--, addr++) {
		ch_dt_prot(addr);
		data_loc(addr) = *cp++;
		dt_fl(addr);
	}
}
#endif	NOFLOAT

/************************************************************************
 *	Data load division.						*
 ************************************************************************
 *									*
 *	dt_lddp(addr)      - LoaD Data Pointer from data.		*
 *	dt_ldip(addr)      - LoaD Instruction Pointer from data.	*
 *	dt_ldu(addr, n)    - LoaD n Unsigned bytes from data.		*
 *	dt_lduw(addr)      - LoaD wsize Unsigned bytes from data.	*
 *	dt_lds(addr, n)    - LoaD n Signed bytes from data.		*
 *	dt_ldsw(addr)      - LoaD wsize Signed bytes from data.		*
 *									*
 ************************************************************************/

ptr dt_lddp(addr)
	register ptr addr;
{
	register ptr p;

	LOG(("@g6 dt_lddp(%lu)", addr));

	ch_in_data(addr, psize);
	ch_wordaligned(addr);
#ifdef	LOGGING
	if (!is_dt_set(addr, psize, SH_DATAP)) {
		warning(WGDPEXP);
		warn_dtbits(addr, psize);
	}
#endif	LOGGING

	p = p_in_data(addr);
	LOG(("@g6 dt_lddp() returns %lu", p));
	return (p);
}

ptr dt_ldip(addr)
	register ptr addr;
{
	register ptr p;

	LOG(("@g6 dt_ldip(%lu)", addr));

	ch_in_data(addr, psize);
	ch_wordaligned(addr);
#ifdef	LOGGING
	if (!is_dt_set(addr, psize, SH_INSP)) {
		warning(WGIPEXP);
		warn_dtbits(addr, psize);
	}
#endif	LOGGING

	p = p_in_data(addr);
	LOG(("@g6 dt_ldip() returns %lu", p));
	return (p);
}

unsigned long dt_ldu(addr, n)
	register ptr addr;
	size n;
{
	register int i;
	register unsigned long u = 0;

	LOG(("@g6 dt_ldu(%lu, %lu)", addr, n));

	ch_in_data(addr, n);
	ch_aligned(addr, n);
#ifdef	LOGGING
	if (!is_dt_set(addr, n, SH_INT)) {
		warning(n == 1 ? WGCEXP : WGIEXP);
		warn_dtbits(addr, n);
	}
#endif	LOGGING

	addr += n-1;
	for (i = (int) n-1; i >= 0; i--, addr--) {
		u = (u<<8) | btou(data_loc(addr));
	}
	LOG(("@g6 dt_ldu() returns %lu", u));
	return (u);
}

unsigned long dt_lduw(addr)
	register ptr addr;
{
	register int i;
	register unsigned long u = 0;

	LOG(("@g6 dt_lduw(%lu)", addr));

	ch_in_data(addr, wsize);
	ch_wordaligned(addr);
#ifdef	LOGGING
	if (!is_dt_set(addr, wsize, SH_INT)) {
		warning(WGIEXP);
		warn_dtbits(addr, wsize);
	}
#endif	LOGGING

	addr += wsize-1;
	for (i = (int) wsize-1; i >= 0; i--, addr--) {
		u = (u<<8) | btou(data_loc(addr));
	}
	LOG(("@g6 dt_lduw() returns %lu", u));
	return (u);
}

long dt_lds(addr, n)
	register ptr addr;
	size n;
{
	register int i;
	register long l;

	LOG(("@g6 dt_lds(%lu, %lu)", addr, n));

	ch_in_data(addr, n);
	ch_aligned(addr, n);
#ifdef	LOGGING
	if (!is_dt_set(addr, n, SH_INT)) {
		warning(n == 1 ? WGCEXP : WGIEXP);
		warn_dtbits(addr, n);
	}
#endif	LOGGING

	addr += n-2;
	l = btos(data_loc(addr + 1));
	for (i = n - 2; i >= 0; i--, addr--) {
		l = (l<<8) | btol(data_loc(addr));
	}
	LOG(("@g6 dt_lds() returns %lu", l));
	return (l);
}

long dt_ldsw(addr)
	register ptr addr;
{
	register int i;
	register long l;

	LOG(("@g6 dt_ldsw(%lu)", addr));

	ch_in_data(addr, wsize);
	ch_wordaligned(addr);
#ifdef	LOGGING
	if (!is_dt_set(addr, wsize, SH_INT)) {
		warning(WGIEXP);
		warn_dtbits(addr, wsize);
	}
#endif	LOGGING

	addr += wsize-2;
	l = btos(data_loc(addr + 1));
	for (i = wsize - 2; i >= 0; i--, addr--) {
		l = (l<<8) | btol(data_loc(addr));
	}
	LOG(("@g6 dt_ldsw() returns %lu", l));
	return (l);
}

/************************************************************************
 *	Data move division						*
 ************************************************************************
 *									*
 *	dt_mvd(d2, d1, n) - Move n bytes in data from d1 to d2.		*
 *	dt_mvs(d, s, n)   - Move n bytes from s in stack to d in data.	*
 *									*
 *	See st_mvs() in stack.c for a description.			*
 *									*
 ************************************************************************/

dt_mvd(d2, d1, n)			/* d1 -> d2 */
	register ptr d2, d1;
	size n;
{
	register int i;

	ch_in_data(d1, n);
	ch_wordaligned(d1);
	ch_in_data(d2, n);
	ch_wordaligned(d2);

	for (i = (int) n; i > 0; i--, d1++, d2++) {
		ch_dt_prot(d2);
		data_loc(d2) = data_loc(d1);
#ifdef	LOGGING
		dt_sh(d2) = dt_sh(d1) & ~SH_PROT;
#endif	LOGGING
	}
}

dt_mvs(d, s, n)				/* s -> d */
	register ptr d, s;
	size n;
{
	register int i;

	ch_in_stack(s, n);
	ch_wordaligned(s);
	ch_in_data(d, n);
	ch_wordaligned(d);

	for (i = (int) n; i > 0; i--, d++, s++) {
		ch_dt_prot(d);
		ch_st_prot(s);
		data_loc(d) = stack_loc(s);
#ifdef	LOGGING
		dt_sh(d) = st_sh(s) & ~SH_PROT;
#endif	LOGGING
	}
}

#ifdef	LOGGING

PRIVATE warn_dtbits(addr, n)
	register ptr addr;
	register size n;
{
	register int or_bits = 0;
	register int and_bits = 0xff;

	while (n--) {
		or_bits |= dt_sh(addr);
		and_bits &= dt_sh(addr);
		addr++;
	}

	if (or_bits != and_bits) {
		/* no use trying to diagnose */
		warningcont(WWASMISC);
		return;
	}
	if (or_bits == 0)
		warningcont(WWASUND);
	if (or_bits & SH_INT)
		warningcont(WWASINT);
	if (or_bits & SH_FLOAT)
		warningcont(WWASFLOAT);
	if (or_bits & SH_DATAP)
		warningcont(WWASDATAP);
	if (or_bits & SH_INSP)
		warningcont(WWASINSP);
}

#endif	LOGGING

