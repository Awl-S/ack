#include "string.h"

/* $Header $ */

int _asc(str)
String *str;
{
	if(str==0 || str->strval==0)
		error(3);
	return( *str->strval);
}
