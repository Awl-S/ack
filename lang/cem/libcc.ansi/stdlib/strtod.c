/* $Header$ */

#include	"ext_fmt.h"

void _str_ext_cvt(char *s, char **ss, struct EXTEND *e);
double _ext_dbl_cvt(struct EXTEND *e);

double
strtod(char *p, char **pp)
{
	struct EXTEND e;

	_str_ext_cvt(p, pp, &e);
	return _ext_dbl_cvt(&e);
}
