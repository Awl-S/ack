extern lset	entities;	/* The pseudo-symboltable. */

extern entity_p	find_entity();	/* (valnum vn)
				 * Tries to find an entity with value number vn.
				 */

extern entity_p	en_enter();	/* (entity_p enp)
				 * Enter the entity in enp in the set of
				 * entities if it was not already there.
				 */

extern		clr_entities();	/* ()
				 * Release all space occupied by our
				 * pseudo-symboltable.
				 */
