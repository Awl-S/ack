/*
  (c) copyright 1988 by the Vrije Universiteit, Amsterdam, The Netherlands.
  See the copyright notice in the ACK home directory, in the file "Copyright".
*/

/* $Header$ */

/*
	SUBTRACT TWO FLOATS - DOUBLE Precision (SBF 8)
*/

#include	"FP_types.h"

extern	_double	adf8(), ngf8();

_double
sbf8(s2,s1)
_double	s1,s2;
{
	_double *result = &s1;	/* s1 may not be in a register! */

	if (s2.__double[0] == 0 && s2.__double[1] == 0) {
		return s1;
	}
	s2 = ngf8(s2);
	*result = adf8(s2,s1);	/* add and return result */
	return(s1);
}
