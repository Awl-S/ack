/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
#define RCSID0 "$Header$"

/*
 * INTEL 80386 options
 */
#define	THREE_PASS	/* branch and offset optimization */
#define	LISTING		/* enable listing facilities */
#define RELOCATION	/* generate relocation info */
#define DEBUG 1

#undef valu_t
#define valu_t long
