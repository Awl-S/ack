
/* C O N S T A N T   P R O P A G A T I O N */

extern line_p unique_def();	/* ( line_p use; bblock_p b; short *defnr_out;)
				 * See if there is a unique explicit definition
				 * of the variable used by 'use' that
				 * reaches 'use'.
				 */
extern bool value_known();	/* (line_p def; offset *val_out)
				 * See if the value stored by definition 'def'
				 * is known statically (i.e. is a constant).
				 */
extern fold_const();		/* (line_p l; bblock_p b; offset val)
				 * Perform the substitutions required for
				 * constant folding.
				 */
extern bool is_use();		/* (line_p l)
				 * See if 'l' is a use of a variable.
				 */
extern bool affected();		/* (line_p use,l; short  v)
				 * See if the variable referenced by 'use' may 
				 * be changed by instruction l, which is 
				 * either a cal, cai or an indirect assignment.
				 */
