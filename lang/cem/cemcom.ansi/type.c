/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
/* $Header$ */
/*	T Y P E   D E F I N I T I O N   M E C H A N I S M	 */

#include	"nobitfield.h"
#include	"botch_free.h"
#include	<alloc.h>
#include	"Lpars.h"
#include	"arith.h"
#include	"type.h"
#include	"idf.h"
#include	"def.h"
#include	"proto.h"
#include	"sizes.h"
#include	"align.h"
#include	"decspecs.h"

extern struct type *function_of(), *array_of();
#ifndef NOBITFIELD
extern struct type *field_of();
#endif NOBITFIELD

/*	To be created dynamically in main() from defaults or from command
	line parameters.
*/
struct type
	*schar_type, *uchar_type,
	*short_type, *ushort_type,
	*word_type, *uword_type,
	*int_type, *uint_type,
	*long_type, *ulong_type,
	*float_type, *double_type, *lngdbl_type,
	*void_type, *gen_type, *label_type,
	*string_type, *funint_type, *error_type;

struct type *pa_type;	/* Pointer-Arithmetic type	*/

struct type *
create_type(fund)
	int fund;
{
	/*	A brand new struct type is created, and its tp_fund set
		to fund.
	*/
	register struct type *ntp = new_type();

	ntp->tp_fund = fund;
	ntp->tp_size = (arith)-1;

	return ntp;
}

struct type *
promoted_type(tp)
struct type *tp;
{
	if (tp->tp_fund == CHAR || tp->tp_fund == SHORT) {
		if (tp->tp_unsigned == UNSIGNED && tp->tp_size == int_size)
			return uint_type;
		else return int_type;
	} else if (tp->tp_fund == FLOAT)
		return double_type;
	else return tp;
}

struct type *
construct_type(fund, tp, qual, count, pl)
	register struct type *tp;
	register struct proto *pl;
	arith count; /* for fund == ARRAY only */
	int qual;
{
	/*	fund must be a type constructor: FIELD, FUNCTION, POINTER or
		ARRAY. The pointer to the constructed type is returned.
	*/
	register struct type *dtp;

	switch (fund)	{
#ifndef NOBITFIELD
	case FIELD:
		dtp = field_of(tp, qual);
		break;
#endif NOBITFIELD

	case FUNCTION:
		if (tp->tp_fund == FUNCTION)	{
			error("function cannot yield function");
			return error_type;
		}
		if (tp->tp_fund == ARRAY)	{
			error("function cannot yield array");
			return error_type;
		}

		dtp = function_of(tp, pl, qual);
		break;
	case POINTER:
		if (tp->tp_fund == VOID) {
			/* A void pointer has the same characteristics as a
			   character pointer. I can't make them equal, because
			   i would like to have the type information around */
			tp = qualifier_type(gen_type, tp->tp_typequal);
		}
		dtp = pointer_to(tp, qual);
		break;
	case ARRAY:
		if (count >= 0 && tp->tp_size < 0)	{
			error("cannot construct array of unknown type");
			count = (arith)-1;
		}
		else if (tp->tp_size == 0)	/* CJ */
			strict("array elements have size 0");
		if (count >= (arith)0)
			count *= tp->tp_size;
		dtp = array_of(tp, count, qual);
		break;
	default:
		crash("bad constructor in construct_type");
		/*NOTREACHED*/
	}
	return dtp;
}

struct type *
function_of(tp, pl, qual)
	register struct type *tp;
	register struct proto *pl;
	int qual;
{
	register struct type *dtp = tp->tp_function;

	/* look for a type with the right qualifier */
#if 0
/* the code doesn't work in the following case:
	int func();
	int func(int a, int b) { return q(a); }
   because updating the type works inside the data-structures for that type
   thus, a new type is created for very function. This may change in the
   future, when declarations with empty parameter lists become obsolete.
*/
	while (dtp && (dtp->tp_typequal != qual || dtp->tp_proto != pl))
		dtp = dtp->next;
#else
	dtp = 0;
#endif

	if (!dtp)	{
		dtp = create_type(FUNCTION);
		dtp->tp_up = tp;
		dtp->tp_size = pointer_size;
		dtp->tp_align = pointer_align;
		dtp->tp_typequal = qual;
		dtp->tp_proto = pl;
		dtp->next = tp->tp_function;
		tp->tp_function = dtp;
	}
	return dtp;
}

struct type *
pointer_to(tp, qual)
	register struct type *tp;
	int qual;
{
	register struct type *dtp = tp->tp_pointer;

	/* look for a type with the right qualifier */
	while (dtp && dtp->tp_typequal != qual)
		dtp = dtp->next;

	if (!dtp)	{
		dtp = create_type(POINTER);
		dtp->tp_unsigned = 1;
		dtp->tp_up = tp;
		dtp->tp_size = pointer_size;
		dtp->tp_align = pointer_align;
		dtp->tp_typequal = qual;
		dtp->next = tp->tp_pointer;
		tp->tp_pointer = dtp;
	}
	return dtp;
}

struct type *
array_of(tp, count, qual)
	register struct type *tp;
	arith count;
	int qual;
{
	register struct type *dtp = tp->tp_array;

	/* look for a type with the right size */
	while (dtp && (dtp->tp_size != count || dtp->tp_typequal != qual))
		dtp = dtp->next;

	if (!dtp)	{
		dtp = create_type(ARRAY);
		dtp->tp_up = tp;
		dtp->tp_size = count;
		dtp->tp_align = tp->tp_align;
		dtp->tp_typequal = qual;
		dtp->next = tp->tp_array;
		tp->tp_array = dtp;
	}
	return dtp;
}

#ifndef NOBITFIELD
struct type *
field_of(tp, qual)
	register struct type *tp;
	int qual;
{
	register struct type *dtp = create_type(FIELD);

	dtp->tp_up = tp;
	dtp->tp_align = tp->tp_align;
	dtp->tp_size = tp->tp_size;
	dtp->tp_typequal = qual;
	return dtp;
}
#endif NOBITFIELD

arith
size_of_type(tp, nm)
	struct type *tp;
	char nm[];
{
	arith sz = tp->tp_size;

	if (sz < 0)	{
		error("size of %s unknown", nm);
		return (arith)1;
	}
	return sz;
}

idf2type(idf, tpp)
	struct idf *idf;
	struct type **tpp;
{
	/*	Decoding  a typedef-ed identifier: if the size is yet
		unknown we have to make copy of the type descriptor to
		prevent garbage at the initialisation of arrays with
		unknown size.
	*/
	register struct type *tp = idf->id_def->df_type;

	if (	tp->tp_size < (arith)0 && tp->tp_fund == ARRAY)	{
		*tpp = new_type();
		**tpp = *tp;
			/* this is really a structure assignment, AAGH!!! */
	}
	else	{
		*tpp = tp;
	}
}

arith
align(pos, al)
	arith pos;
	int al;
{
	return ((pos + al - 1) / al) * al;
}

struct type *
standard_type(fund, sgn, algn, sz)
	int algn; arith sz;
{
	register struct type *tp = create_type(fund);

	tp->tp_unsigned = sgn;
	tp->tp_align = algn;
	tp->tp_size = sz;

	return tp;
}
