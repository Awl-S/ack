#define RCSID1 "$Header$"

#
/*
 * PDP 11 Machine dependent C declarations
 */

/* Addressing modes */
#define RMODE		00
#define AIMODE		020
#define ADMODE		040
#define INDMODE		060
#define RDEFMODE	010
#define AIDEFMODE	030
#define ADDEFMODE	050
#define INDDEFMODE	070

#define OPLDF 0172400
#define OPSTF 0174000
#define OPSOB 077000
#define OPBRA 0400

#define low3(z)	(((short) z) & 07)
#define low6(z)	(((short) z) & 077)
#define low7(z) (((short) z) & 0177)

#define fit3(z)	(low3(z) == (z))
#define fit6(z)	(low6(z) == (z))
#define fit8(z)	(lowb(z) == (z))

extern expr_t exp_1, exp_2;
extern int rel_1, rel_2;
extern int im1flag INIT(0);
extern int im2flag INIT(0);
extern valu_t adjust();
