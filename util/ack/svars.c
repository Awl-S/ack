/*
 * (c) copyright 1983 by the Vrije Universiteit, Amsterdam, The Netherlands.
 *
 *          This product is part of the Amsterdam Compiler Kit.
 *
 * Permission to use, sell, duplicate or disclose this software must be
 * obtained in writing. Requests for such permissions may be sent to
 *
 *      Dr. Andrew S. Tanenbaum
 *      Wiskundig Seminarium
 *      Vrije Universiteit
 *      Postbox 7161
 *      1007 MC Amsterdam
 *      The Netherlands
 *
 */

#include "ack.h"

#ifndef NORCSID
static char rcs_id[] = "$Header$" ;
#endif

/*      The processing of string valued variables,
	this is an almost self contained module.

	Five externally visible routines:

	setsvar(name,result)
		Associate the name with the result.

		name    a string pointer
		result  a string pointer

	setpvar(name,routine)
		Associate the name with the routine.

		name    a string pointer
		routine a routine id

	   The parameters name and result are supposed to be pointing to
	   non-volatile string storage used only for this call.

	char *getvar(name)
		returns the pointer to a string associated with name,
		the pointer is produced by returning result or the
		value returned by calling the routine.

		name    a string pointer

	Other routines called

	fatal(args*)    When something goes wrong
	getcore(size)   Core allocation

*/

extern  char    *getcore();
extern          fatal();

struct vars {
	char                            *v_name;
	enum { routine, string }        v_type;

	union {
		char    *v_string;
		char    *(*v_routine)();
	}                               v_value ;
	struct vars                     *v_next ;
};

static struct vars *v_first ;

static struct vars *newvar(name) char *name; {
	register struct vars *new ;

	for ( new=v_first ; new ; new= new->v_next ) {
		if ( strcmp(name,new->v_name)==0 ) {
			throws(name) ;
			if ( new->v_type== string ) {
				throws(new->v_value.v_string) ;
			}
			return new ;
		}
	}
	new= (struct vars *)getcore( (unsigned)sizeof (struct vars));
	new->v_name= name ;
	new->v_next= v_first ;
	v_first= new ;
	return new ;
}

setsvar(name,str) char *name, *str ; {
	register struct vars *new ;

	new= newvar(name);
#ifdef DEBUG
	if ( debug>=2 ) vprint("%s=%s\n", name, str) ;
#endif
	new->v_type= string;
	new->v_value.v_string= str;
}

setpvar(name,rout) char *name, *(*rout)() ; {
	register struct vars *new ;

	new= newvar(name);
#ifdef DEBUG
	if ( debug>=2 ) vprint("%s= (*%o)()\n",name,rout) ;
#endif
	new->v_type= routine;
	new->v_value.v_routine= rout;
}

char *getvar(name) char *name ; {
	register struct vars *scan ;

	for ( scan=v_first ; scan ; scan= scan->v_next ) {
		if ( strcmp(name,scan->v_name)==0 ) {
			switch ( scan->v_type ) {
			case string:
				return scan->v_value.v_string ;
			case routine:
				return (*scan->v_value.v_routine)() ;
			}
		}
	}
	return (char *)0 ;
}
