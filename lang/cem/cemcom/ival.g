/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
/* $Header$ */
/* CODE FOR THE INITIALISATION OF GLOBAL VARIABLES */

{
#include	"nofloat.h"
#include	<em.h>
#include	"debug.h"
#include	<alloc.h>
#include	"nobitfield.h"
#include	"arith.h"
#include	"align.h"
#include	"label.h"
#include	"expr.h"
#include	"type.h"
#include	"struct.h"
#include	"field.h"
#include	"assert.h"
#include	"Lpars.h"
#include	"class.h"
#include	"sizes.h"
#include	"idf.h"
#include	"level.h"
#include	"def.h"
#include	"LLlex.h"
#include	"noRoption.h"
#include	"estack.h"

#define con_nullbyte()	C_con_ucon("0", (arith)1)
#define aggregate_type(tp) ((tp)->tp_fund == ARRAY || (tp)->tp_fund == STRUCT)

char *long2str();
char *strncpy();
extern char options[];
static int gen_error;
struct type **gen_tphead(), **gen_tpmiddle();
struct sdef *gen_align_to_next();
struct e_stack *p_stack;
}

/*	initial_value recursively guides the initialisation expression.
	Upto now, the initialisation of a union is not allowed!
*/
/* 7 */
initial_value(register struct type **tpp; register struct expr **expp;) :
			{ if (tpp) gen_tpcheck(tpp, 0); }
[
	assignment_expression(expp)
			{ if ((*expp)->ex_type->tp_fund == ARRAY)
				array2pointer(*expp);
			  if (tpp) {
			  	gen_simple_exp(tpp, expp);
			  	free_expression(*expp);
				*expp = 0;
			  }
			}
|
	initial_value_pack(tpp, expp)
]
;

initial_value_pack(struct type **tpp; struct expr **expp;)
	{ static int level; }
:
	'{'
			{ if (level == 0) gen_error = 0; level++; }
	initial_value_list(tpp, expp)
			{ level--;
			  if (! level) {
				while (p_stack) {
					struct e_stack *p = p_stack->next;

					free_e_stack(p_stack);
					p_stack = p;
				}
			  }
			}
	'}'
;

initial_value_list(register struct type **tpp; struct expr **expp;)
	{ struct expr *e1;
	  register struct type **tpp2 = 0;
	}
:
			{ if (tpp) tpp2 = gen_tphead(tpp, 0); }
	initial_value(tpp2, &e1)
			{ if (!tpp) init_expression(&expp, e1); }
	[%while (AHEAD != '}')		/* >>> conflict on ',' */
		','
			{ if (tpp) tpp2 = gen_tpmiddle(); }
		initial_value(tpp2, &e1)
			{ if (!tpp) init_expression(&expp, e1); }
	]*
			{ if (tpp) gen_tpend(); }
	','?				/* optional trailing comma */
;

{
gen_tpcheck(tpp, union_allowed)
	struct type **tpp;
{
	register struct type *tp;

	if (gen_error) return;
	switch((tp = *tpp)->tp_fund) {
	case ARRAY:
		if (! valid_type(tp->tp_up, "array element"))
			gen_error = 1;
		break;
	case STRUCT:
		if (! valid_type(tp, "struct"))
			gen_error = 1;
		break;
	case UNION:
		if (! union_allowed) {
			error("union initialisation not allowed");
			gen_error = 1;
			break;
		}
		if (! valid_type(tp, "union"))
			gen_error = 1;
		break;
	}
}

gen_simple_exp(tpp, expp)
	struct type **tpp;
	struct expr **expp;
{
	register struct type *tp;

	if (gen_error) return;
	tp = *tpp;
	switch(tp->tp_fund) {
	case ARRAY:
		if ((*expp)->ex_class == String && tp->tp_up->tp_fund == CHAR) {
			ch_array(tpp,*expp);
			break;
		}
		/* Fall through */
	case STRUCT:
		check_and_pad(expp, tpp);
		break;
	case ERRONEOUS:
		gen_error = 1;
		break;
	default:
		check_ival(expp, tp);
		break;
	}
}

struct type **
arr_elem(tpp, p)
	struct type **tpp;
	struct e_stack *p;
{
	register struct type *tp = *tpp;

	if (tp->tp_up->tp_fund == CHAR && AHEAD == STRING && p->elem_count == 1) {
		p->nelem = 1;
		return tpp;
	}
	if (AHEAD == '{' || ! aggregate_type(tp->tp_up))
		return &(tp->tp_up);
	return gen_tphead(&(tp->tp_up), 1);
}

struct sdef *
next_field(sd, p)
	register struct sdef *sd;
	register struct e_stack *p;
{
	if (sd->sd_sdef)
		p->bytes_upto_here += zero_bytes(sd);
	if (p->last_offset != sd->sd_offset) {
		p->bytes_upto_here +=
			size_of_type(sd->sd_type, "selector");
		p->last_offset = sd->sd_offset;
	}
	return sd->sd_sdef;
}

struct type **
gen_tphead(tpp, nest)
	struct type **tpp;
{
	register struct type *tp = *tpp;
	register struct e_stack *p;
	register struct sdef *sd;

	if (tpp && *tpp == error_type) {
		gen_error = 1;
		return 0;
	}
	if (gen_error) return tpp;
	p = new_e_stack();
	p->next = p_stack;
	p_stack = p;
	p->s_nested = nest;
	p->s_tpp = tpp;
	switch(tp->tp_fund) {
	case ARRAY:
		p->nelem = -1;
		p->elem_count = 1;
		if (tp->tp_size != (arith) -1) {
			p->nelem = (tp->tp_size / tp->tp_up->tp_size);
		}
		return arr_elem(tpp, p);
	case STRUCT:
		p->s_def = sd = tp->tp_sdef;
		p->bytes_upto_here = 0;
		p->last_offset = -1;
#ifndef NOBITFIELD
		while (sd && is_anon_idf(sd->sd_idf)) {
			put_bf(sd->sd_type, (arith) 0);
			sd = next_field(sd, p);
		}
#endif
		if (! sd) {
			/* something wrong with this struct */
			gen_error = 1;
			p_stack = p->next;
			free_e_stack(p);
			return 0;
		}
		p->s_def = sd;
		if (AHEAD != '{' && aggregate_type(sd->sd_type)) {
			return gen_tphead(&(sd->sd_type), 1);
		}
		return &(sd->sd_type);
	default:
		p->nelem = 1;
		p->elem_count = 1;
		return tpp;
	}
}

struct type **
gen_tpmiddle()
{
	register struct type *tp;
	register struct sdef *sd;
	register struct e_stack *p = p_stack;

	if (gen_error) {
		if (p) return p->s_tpp;
		return 0;
	}
again:
	tp = *(p->s_tpp);
	switch(tp->tp_fund) {
	default:
		if (p->elem_count == p->nelem && p->s_nested) {
			p = p->next;
			free_e_stack(p_stack);
			p_stack = p;
			goto again;
		}
		p->elem_count++;
		if (p->nelem >= 0 && p->elem_count > p->nelem) {
			too_many_initialisers();
			return p->s_tpp;
		}
		if (tp->tp_fund == ARRAY) {
			return arr_elem(p->s_tpp, p);
		}
		return p->s_tpp;
	case STRUCT:
		sd = gen_align_to_next(p);
		if (! sd) {
			while (p->bytes_upto_here++ < tp->tp_size)
				con_nullbyte();
			if (p->s_nested) {
				p = p->next;
				free_e_stack(p_stack);
				p_stack = p;
				goto again;
			}
			too_many_initialisers();
			return p->s_tpp;
		}
		if (AHEAD != '{' && aggregate_type(sd->sd_type)) {
			return gen_tphead(&(sd->sd_type), 1);
		}
		return &(sd->sd_type);
	}
}

struct sdef *
gen_align_to_next(p)
	register struct e_stack *p;
{
	register struct sdef *sd = p->s_def;

	if (! sd) return sd;
#ifndef NOBITFIELD
	do {
		if (is_anon_idf(sd->sd_idf)) put_bf(sd->sd_type, (arith) 0);
#endif
		sd = next_field(sd, p);
#ifndef NOBITFIELD
	} while (sd && is_anon_idf(sd->sd_idf));
#endif
	p->s_def = sd;
	return sd;
}

gen_tpend()
{
	register struct e_stack *p = p_stack;
	register struct type *tp;
	register struct sdef *sd;
	int getout = 0;

	while (!getout && p) {
	    if (!gen_error) {
		tp = *(p->s_tpp);
		switch(tp->tp_fund) {
		case ARRAY:
			if (tp->tp_size == -1) {
				*(p->s_tpp) = construct_type(ARRAY,
					tp->tp_up, p->elem_count);
			}
			else {
				while (p->nelem-- > p->elem_count) {
					pad(tp->tp_up);
				}
			}
			break;
		case STRUCT:
			sd = gen_align_to_next(p);
			while (sd) {
				pad(sd->sd_type);
				if (sd->sd_sdef)
					p->bytes_upto_here += zero_bytes(sd);
				p->bytes_upto_here +=
					size_of_type(sd->sd_type, "selector");
				sd = sd->sd_sdef;
			}
			while (p->bytes_upto_here++ < tp->tp_size)
				con_nullbyte();
			break;
		}
	    }
	    if (! p->s_nested) getout = 1;
	    p = p->next;
	    free_e_stack(p_stack);
	    p_stack = p;
	}
	gen_error = 0;
}

/*	check_and_pad() is given a simple initialisation expression
	where the type can be either a simple or an aggregate type.
	In the latter case, only the first member is initialised and
	the rest is zeroed.
*/
check_and_pad(expp, tpp)
	struct type **tpp;
	struct expr **expp;
{
	register struct type *tp = *tpp;

	if (tp->tp_fund == ARRAY) {
		check_and_pad(expp, &(tp->tp_up));	/* first member	*/
		if (tp->tp_size == (arith)-1)
			/*	no size specified upto here: just
				set it to the size of one member.
			*/
			tp = *tpp = construct_type(ARRAY, tp->tp_up, (arith)1);
		else {
			register int dim = tp->tp_size / tp->tp_up->tp_size;
			/* pad remaining members with zeroes */
			while (--dim > 0)
				pad(tp->tp_up);
		}
	}
	else
	if (tp->tp_fund == STRUCT) {
		register struct sdef *sd = tp->tp_sdef;

		check_and_pad(expp, &(sd->sd_type));
		/* next selector is aligned by adding extra zeroes */
		if (sd->sd_sdef)
			zero_bytes(sd);
		while (sd = sd->sd_sdef) { /* pad remaining selectors	*/
			pad(sd->sd_type);
			if (sd->sd_sdef)
				zero_bytes(sd);
		}
	}
	else	/* simple type	*/
		check_ival(expp, tp);
}

/*	pad() fills an element of type tp with zeroes.
	If the element is an aggregate, pad() is called recursively.
*/
pad(tpx)
	struct type *tpx;
{
	register struct type *tp = tpx;
	register arith sz = tp->tp_size;

	gen_tpcheck(&tpx, 1);
	if (gen_error) return;
	switch (tp->tp_fund) {
	case UNION:
#ifndef NOROPTION
		if (options['R']) {
			warning("initialisation of unions not allowed");
		}
#endif
		break;
#ifndef NOBITFIELD
	case FIELD:
		put_bf(tp, (arith)0);
		return;
#endif NOBITFIELD
		default:
			break;
	}

	while (sz >= word_size) {
		C_con_cst((arith) 0);
		sz -= word_size;
	}
	while (sz) {
		C_con_icon("0", (arith) 1);
		sz--;
	}
}

/*	check_ival() checks whether the initialisation of an element
	of a fundamental type is legal and, if so, performs the initialisation
	by directly generating the necessary code.
	No further comment is needed to explain the internal structure
	of this straightforward function.
*/
check_ival(expp, tp)
	register struct type *tp;
	struct expr **expp;
{
	/*	The philosophy here is that ch7cast puts an explicit
		conversion node in front of the expression if the types
		are not compatible.  In this case, the initialisation
		expression is no longer a constant.
	*/
	register struct expr *expr = *expp;
	
	switch (tp->tp_fund) {
	case CHAR:
	case SHORT:
	case INT:
	case LONG:
	case ENUM:
	case POINTER:
		ch7cast(expp, '=', tp);
		expr = *expp;
#ifdef DEBUG
		print_expr("init-expr after cast", expr);
#endif DEBUG
		if (!is_ld_cst(expr))
			illegal_init_cst(expr);
		else
		if (expr->VL_CLASS == Const)
			con_int(expr);
		else
		if (expr->VL_CLASS == Name) {
			register struct idf *idf = expr->VL_IDF;

			if (idf->id_def->df_level >= L_LOCAL)
				illegal_init_cst(expr);
			else	/* e.g., int f(); int p = f; */
			if (idf->id_def->df_type->tp_fund == FUNCTION)
				C_con_pnam(idf->id_text);
			else	/* e.g., int a; int *p = &a; */
				C_con_dnam(idf->id_text, expr->VL_VALUE);
		}
		else {
			ASSERT(expr->VL_CLASS == Label);
			C_con_dlb(expr->VL_LBL, expr->VL_VALUE);
		}
		break;
#ifndef NOFLOAT
	case FLOAT:
	case DOUBLE:
		ch7cast(expp, '=', tp);
		expr = *expp;
#ifdef DEBUG
		print_expr("init-expr after cast", expr);
#endif DEBUG
		if (expr->ex_class == Float)
			C_con_fcon(expr->FL_VALUE, expr->ex_type->tp_size);
		else
		if (expr->ex_class == Oper && expr->OP_OPER == INT2FLOAT) {
			/* float f = 1; */
			expr = expr->OP_RIGHT;
			if (is_cp_cst(expr))
				C_con_fcon(long2str((long)expr->VL_VALUE, 10),
					tp->tp_size);
			else 
				illegal_init_cst(expr);
		}
		else
			illegal_init_cst(expr);
		break;
#endif NOFLOAT

#ifndef NOBITFIELD
	case FIELD:
		ch7cast(expp, '=', tp->tp_up);
		expr = *expp;
#ifdef DEBUG
		print_expr("init-expr after cast", expr);
#endif DEBUG
		if (is_cp_cst(expr))
			put_bf(tp, expr->VL_VALUE);
		else
			illegal_init_cst(expr);
		break;
#endif NOBITFIELD

	case ERRONEOUS:
		break;
	default:
		crash("check_ival");
	}
}

/*	ch_array() initialises an array of characters when given
	a string constant.
	Alignment is taken care of.
*/
ch_array(tpp, ex)
	struct type **tpp;	/* type tp = array of characters	*/
	struct expr *ex;
{
	register struct type *tp = *tpp;
	register arith length = ex->SG_LEN;
	char *s;

	ASSERT(ex->ex_class == String);
	if (tp->tp_size == (arith)-1) {
		/* set the dimension	*/
		tp = *tpp = construct_type(ARRAY, tp->tp_up, length);
	}
	else {
		arith dim = tp->tp_size / tp->tp_up->tp_size;

		if (length > dim) {
			expr_warning(ex, "too many initialisers");
		}
		length = dim;
	}
	/* throw out the characters of the already prepared string	*/
	s = Malloc((unsigned) (length));
	clear(s, (int) (length));
	strncpy(s, ex->SG_VALUE, (int) length);
	free(ex->SG_VALUE);
	str_cst(s, (int) (length));
	free(s);
}

/*	As long as some parts of the pipeline cannot handle very long string
	constants, string constants are written out in chunks
*/
str_cst(str, len)
	register char *str;
	register int len;
{
	int chunksize = ((127 + (int) word_size) / (int) word_size) * (int) word_size;

	while (len > chunksize) {
		C_con_scon(str, (arith) chunksize);
		len -= chunksize;
		str += chunksize;
	}
	C_con_scon(str, (arith) len);
}

#ifndef NOBITFIELD
/*	put_bf() takes care of the initialisation of (bit-)field
	selectors of a struct: each time such an initialisation takes place,
	put_bf() is called instead of the normal code generating routines.
	Put_bf() stores the given integral value into "field" and
	"throws" the result of "field" out if the current selector
	is the last of this number of fields stored at the same address.
*/
put_bf(tp, val)
	struct type *tp;
	arith val;
{
	static long field = (arith)0;
	static arith offset = (arith)-1;
	register struct field *fd = tp->tp_field;
	register struct sdef *sd =  fd->fd_sdef;
	static struct expr exp;

	ASSERT(sd);
	if (offset == (arith)-1) {
		/* first bitfield in this field	*/
		offset = sd->sd_offset;
		exp.ex_type = tp->tp_up;
		exp.ex_class = Value;
		exp.VL_CLASS = Const;
	}
	if (val != 0)	/* insert the value into "field"	*/
		field |= (val & fd->fd_mask) << fd->fd_shift;
	if (sd->sd_sdef == 0 || sd->sd_sdef->sd_offset != offset) {
		/* the selector was the last stored at this address	*/
		exp.VL_VALUE = field;
		con_int(&exp);
		field = (arith)0;
		offset = (arith)-1;
	}
}
#endif NOBITFIELD

int
zero_bytes(sd)
	register struct sdef *sd;
{
	/*	fills the space between a selector of a struct
		and the next selector of that struct with zero-bytes.
	*/
	register int n = sd->sd_sdef->sd_offset - sd->sd_offset -
		size_of_type(sd->sd_type, "struct member");
	register int count = n;

	while (n-- > 0)
		con_nullbyte();
	return count;
}

int
valid_type(tp, str)
	struct type *tp;
	char *str;
{
	if (tp->tp_size < 0) {
		error("size of %s unknown", str);
		return 0;
	}
	return 1;
}

con_int(ex)
	register struct expr *ex;
{
	register struct type *tp = ex->ex_type;

	ASSERT(is_cp_cst(ex));
	if (tp->tp_unsigned)
		C_con_ucon(long2str((long)ex->VL_VALUE, -10), tp->tp_size);
	else
		C_con_icon(long2str((long)ex->VL_VALUE, 10), tp->tp_size);
}

illegal_init_cst(ex)
	struct expr *ex;
{
	expr_error(ex, "illegal initialisation constant");
	gen_error = 1;
}

too_many_initialisers()
{
	error("too many initialisers");
	gen_error = 1;
}
}
