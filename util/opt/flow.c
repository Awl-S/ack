#include "param.h"
#include "types.h"
#include "../../h/em_flag.h"
#include "../../h/em_spec.h"
#include "../../h/em_mnem.h"
#include "alloc.h"
#include "line.h"
#include "proinf.h"
#include "optim.h"
#include "ext.h"

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
 * Author: Hans van Staveren
 */

flow() {

	findreach();	/* determine reachable labels */
	cleaninstrs();	/* throw away unreachable code */
}

findreach() {
	register num_p	*npp,np;

	reach(instrs);
	for(npp=curpro.numhash;npp< &curpro.numhash[NNUMHASH]; npp++)
		for(np= *npp; np != (num_p) 0 ; np = np->n_next)
			if (np->n_flags&NUMDATA) {
				np->n_repl->n_flags |= NUMREACH;
				np->n_repl->n_jumps++;
				if (!(np->n_flags&NUMSCAN)) {
					np->n_flags |= NUMSCAN;
					reach(np->n_line->l_next);
				}
			}
}

reach(lnp) register line_p lnp; {
	register num_p np;

	for (;lnp != (line_p) 0; lnp = lnp->l_next) {
		if(lnp->l_optyp == OPNUMLAB) {
			/*
			 * Branch instruction or label
			 */
			np = lnp->l_a.la_np;
			if ((lnp->l_instr&BMASK) != op_lab)
				np = np->n_repl;
			np->n_flags |= NUMREACH;
			if (!(np->n_flags&NUMSCAN)) {
				np->n_flags |= NUMSCAN;
				reach(np->n_line->l_next);
			}
			if ((lnp->l_instr&BMASK) == op_lab)
				return;
			else
				np->n_jumps++;
		}
		if ((em_flag[(lnp->l_instr&BMASK)-sp_fmnem]&EM_FLO)==FLO_T)
			return;
	}
}

cleaninstrs() {
	register line_p *lpp,lp,*lastbra;
	bool reachable,superfluous;
	int instr;

	lpp = &instrs; lastbra = (line_p *) 0; reachable = TRUE;
	while ((lp = *lpp) != (line_p) 0) {
		instr = lp->l_instr&BMASK;
		if (instr == op_lab) {
			if ((lp->l_a.la_np->n_flags&NUMREACH) != 0) {
				reachable = TRUE;
				if (lastbra != (line_p *) 0
				    && (*lastbra)->l_next == lp
				    && (*lastbra)->l_a.la_np->n_repl==lp->l_a.la_np) {
					oldline(*lastbra);
					OPTIM(O_BRALAB);
					lpp = lastbra;
					*lpp = lp;
					lp->l_a.la_np->n_jumps--;
				}
			}
			if ( lp->l_a.la_np->n_repl != lp->l_a.la_np ||
			     ((lp->l_a.la_np->n_flags&NUMDATA)==0 &&
			      lp->l_a.la_np->n_jumps == 0))
				superfluous = TRUE;
			else
				superfluous = FALSE;
		} else
			superfluous = FALSE;
		if ( (!reachable) || superfluous) {
			lp = lp->l_next;
			oldline(*lpp);
			OPTIM(O_UNREACH);
			*lpp = lp;
		} else {
			if ( instr <= sp_lmnem &&
			    (em_flag[instr-sp_fmnem]&EM_FLO)==FLO_T) {
				reachable = FALSE;
				if ((lp->l_instr&BMASK) == op_bra)
					lastbra = lpp;
			}
			lpp = &lp->l_next;
		}
	}
}
