/* S T R E N G T H   R E D U C T I O N 
 *
 * S R _ E X P R . H
 *
 */

extern bool is_ivexpr();/* (line_p l; lset ivs,vars; line_p *lbegin; iv_p *iv;
			 *	int *out_sign)
			 * Try to recognize an expression that is a linear
			 * function of presicely one induction variable.
			 * It may only use loop constants (besides the
			 * induc. var.).
			 */
