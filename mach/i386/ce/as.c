#include "arg_type.h"
#include "as.h"

#define last( s)	( s + strlen( s) - 1)
#define LEFT	'('
#define RIGHT	')'
#define DOLLAR	'$'

block_assemble( instr, nr, first, Last)
char **instr;
int nr, first, Last;
{
	int i;

/*
	if ( first) {
		if( strncmp( instr[0], "pop", 3) == 0) {
			*instr[0] = 'P';
			*( instr[0]+1) = 'O';
			*( instr[0]+2) = 'P';
		}
		else
			@clean_push_buf();
	}
	if ( Last && strncmp( instr[nr-1], "push", 4) == 0) {
			*instr[nr-1] = 'P';
			*( instr[nr-1]+1) = 'U';
			*( instr[nr-1]+2) = 'S';
			*( instr[nr-1]+3) = 'H';
	}
*/
	for( i=0; i<nr; i++)
		assemble( instr[i]);
}


process_label( l)
char *l;
{
}


process_mnemonic( m)
char *m;
{
}


process_operand( str, op)
char *str;
struct t_operand *op;

/*	expr		->	IS_DATA en IS_LABEL
 *	reg		->	IS_REG en IS_ACCU
 *	(expr)		->	IS_ADDR
 *	expr(reg)	->	IS_MEM
 */
{
	char *ptr, *index();

	op->type = UNKNOWN;
	if ( *last( str) == RIGHT) {
		ptr = index( str, LEFT);
		*last( str) = '\0';
		*ptr = '\0';
		if ( is_reg( ptr+1, op)) {
			op->type = IS_MEM;
			op->expr = ( *str == '\0' ? "0" : str);
		}
		else {
			set_label( ptr+1, op);
			op->type = IS_ADDR;
		}
	}
	else
		if ( is_reg( str, op))
			op->type = IS_REG;
		else {
			if ( contains_label( str))
				set_label( str, op);
			else {
				op->type = IS_DATA;
				op->expr = str;
			}
		}
}

static struct regnam {
	char *regstr;
	int regval;
} regnam[] = {
	{ "eax", 0 },
	{ "ebx", 3 },
	{ "ecx", 1 },
	{ "edx", 2 },
	{ "esp", 4 },
	{ "ebp", 5 },
	{ "esi", 6 },
	{ "edi", 7 },
	{ "al", 0 },
	{ "bl", 3 },
	{ "cl", 1 },
	{ "dl", 2 },
	{ "ah", 4 },
	{ "bh", 7 },
	{ "ch", 5 },
	{ "dh", 6 },
	{ 0, 0}
}
;

int is_reg( str, op)
char *str;
struct t_operand *op;
{
	register struct regnam *p = regnam;

	while (p->regstr) {
		if (! strcmp(p->regstr, str)) {
			op->reg = p->regval;
			return TRUE;
		}
		p++;
	}
	return FALSE;
}

#include <ctype.h>
#define	 isletter( c)	( isalpha( c) || c == '_')

int contains_label( str)
char *str;
{
	while( !isletter( *str) && *str != '\0')
		if ( *str == '$')
			if ( arg_type( str) == STRING)
				return( TRUE);
			else
				str += 2;
		else
			str++;

	return( isletter( *str));
}

set_label( str, op)
char *str;
struct t_operand *op;
{
	char *ptr, *index(), *sprint();
	static char buf[256];

	ptr = index( str, '+');

	if ( ptr == 0)
		op->off = "0";
	else {
		*ptr = '\0';
		op->off = ptr + 1;
	}

	if ( isdigit( *str) && ( *(str+1) == 'b' || *(str+1) == 'f') &&
	     *(str+2) == '\0') {
		*(str+1) = '\0';	/* remove b or f! */
		op->lab = str;
		op->type = IS_ILB;
	}
	else {
		op->type = IS_LABEL;
		if ( index( str, DOLLAR) != 0)
			op->lab = str;
		else 
			op->lab = sprint( buf, "\"%s\"", str);
	}
}


/******************************************************************************/



mod_RM( reg, op)
int reg;
struct t_operand *op;
{
	if ( REG( op))
		R233( 0x3, reg, op->reg);
	else if ( ADDR( op)) {
		R233( 0x0, reg, 0x5);
		@reloc4( %$(op->lab), %$(op->off), ABSOLUTE);
	}
	else if ( strcmp( op->expr, "0") == 0)
		switch( op->reg) {
		  case AX:
		  case BX:
		  case CX:
		  case DX:
		  case DI:
		  case SI:  R233( 0x0, reg, op->reg);
			    break;

		  case BP : R233( 0x1, reg, 0x6);	/* Exception! */
			    @text1( 0);
			    break;

		  default : fprint( STDERR, "Wrong index register %d\n",
				    op->reg);
		}
	else {
	    if (isdigit(op->expr[0])) {
		long l, atol();

		l = atol(op->expr);
		if ( l <= 127 && l >= -128) {
			switch( op->reg) {
			  case AX:
			  case BX:
			  case CX:
			  case DX:
			  case DI:
			  case BP:
		  	  case SI : R233( 0x1, reg, op->reg);
				    break;
	
			  default : fprint( STDERR, "Wrong index register %d\n",
					    op->reg);
			}
			@text1( %$(op->expr));
		} else {
			switch( op->reg) {
			  case AX:
			  case BX:
			  case CX:
			  case DX:
			  case DI:
			  case BP:
		  	  case SI : R233( 0x2, reg, op->reg);
				    break;
	
			  default : fprint( STDERR, "Wrong index register %d\n",
					    op->reg);
			}
			@text4( %$(op->expr));
		}
	    } else {
		@if ( fit_byte( %$(op->expr)))
			switch( op->reg) {
			  case AX:
			  case BX:
			  case CX:
			  case DX:
			  case DI:
			  case BP:
		  	  case SI : R233( 0x1, reg, op->reg);
				    break;
	
			  default : fprint( STDERR, "Wrong index register %d\n",
					    op->reg);
			}
			@text1( %$(op->expr));
		@else
			switch( op->reg) {
			  case AX:
			  case BX:
			  case CX:
			  case DX:
			  case DI:
			  case BP:
		  	  case SI : R233( 0x2, reg, op->reg);
				    break;
	
			  default : fprint( STDERR, "Wrong index register %d\n",
					    op->reg);
			}
			@text4( %$(op->expr));
		@fi
	    }
	}
}

mv_RG_EADDR( dst, src)
struct t_operand *dst, *src;
{
	if ( REG(src) && dst->reg == src->reg)
		; /* Nothing!! result of push/pop optimization */
	else {
		@text1( 0x8b);
		mod_RM( dst->reg, src);
	}
}


R233( a, b, c)
int a,b,c;
{
	@text1( %d( (a << 6) | ( b << 3) | c));
}


R53( a, b)
int a,b;
{
	@text1( %d( (a << 3) | b));
}

small_const(opc, src)
	struct t_operand *src;
{
	if (isdigit(src->expr[0])) {
		long l, atol();

		l = atol(src->expr);
		if (l >= -128 && l <= 127) {
			@text1(%d(opc|02));
			@text1(%$(src->expr));
		}
		else {
			@text1(%d(opc));
			@text4(%$(src->expr));
		}
	}
	else {
		@if (fit_byte(%$(src->expr)))
			@text1(%d(opc|02));
			@text1(%$(src->expr));
		@else
			@text1(%d(opc));
			@text1(%$(src->expr));
		@fi
	}
}

small_RMconst(opc, reg, dst, src)
	struct t_operand *dst, *src;
{
	if (isdigit(src->expr[0])) {
		long l, atol();

		l = atol(src->expr);
		if (l >= -128 && l <= 127) {
			@text1(%d(opc|02));
			mod_RM(reg, dst);
			@text1(%$(src->expr));
		}
		else {
			@text1(%d(opc));
			mod_RM(reg, dst);
			@text4(%$(src->expr));
		}
	}
	else {
		@if (fit_byte(%$(src->expr)))
			@text1(%d(opc|02));
			mod_RM(reg, dst);
			@text1(%$(src->expr));
		@else
			@text1(%d(opc));
			mod_RM(reg, dst);
			@text1(%$(src->expr));
		@fi
	}
}
