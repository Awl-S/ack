/* C O P Y   P R O P A G A T I O N 
 *
 * A U X I L I A R Y   R O U T I N E S
 */


#include "../share/types.h"
#include "../ud/ud.h"
#include "../share/debug.h"
#include "../share/global.h"
#include "../share/alloc.h"
#include "../share/lset.h"
#include "../share/cset.h"
#include "../share/def.h"
#include "../share/locals.h"
#include "../share/aux.h"
#include "../../../h/em_mnem.h"
#include "../../../h/em_pseu.h"
#include "../../../h/em_spec.h"
#include "../ud/ud_defs.h"

repl_line(old,new,b)
	line_p old,new;
	bblock_p b;
{
	/* Replace 'old' by 'new' */

	if (PREV(old) == (line_p) 0) {
		b->b_start = new;
	} else {
		PREV(old)->l_next = new;
	}
	PREV(new) = PREV(old);
	if ((new->l_next = old->l_next) != (line_p) 0) {
		PREV(new->l_next) = new;
	}
	oldline(old);
}



bool same_var(use,def)
	line_p use,def;
{
	/* 'use' is an instruction that uses a variable
	 * for which we maintain ud-info (e.g. a LOL).
	 * See if 'def' references the same variable.
	 */

	if (TYPE(use) == OPOBJECT) {
		return TYPE(def) == OPOBJECT && OBJ(use) == OBJ(def);
	} else {
		return TYPE(def) != OPOBJECT && off_set(use) == off_set(def);
	}
}
