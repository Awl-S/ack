/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
/* $Header$ */
/*	S E M A N T I C   A N A L Y S I S -- C H A P T E R  3.3		*/

#include	"lint.h"
#include	"debug.h"
#include	"nobitfield.h"
#include	"idf.h"
#include	<flt_arith.h>
#include	"arith.h"
#include	"proto.h"
#include	"type.h"
#include	"struct.h"
#include	"label.h"
#include	"expr.h"
#include	"def.h"
#include	"Lpars.h"
#include	"assert.h"
#include	"file_info.h"

extern char options[];
extern char *symbol2str();
extern struct type *qualifier_type();

/*	Most expression-handling routines have a pointer to a
	(struct type *) as first parameter. The object under the pointer
	gets updated in the process.
*/

ch3sel(expp, oper, idf)
	struct expr **expp;
	struct idf *idf;
{
	/*	The selector idf is applied to *expp; oper may be '.' or
		ARROW.
	*/
	register struct expr *exp;
	register struct type *tp;
	register struct sdef *sd;

	any2opnd(expp, oper);
	exp = *expp;
	tp = exp->ex_type;
	if (oper == ARROW)	{
		if (tp->tp_fund == POINTER &&
		    ( tp->tp_up->tp_fund == STRUCT ||
		      tp->tp_up->tp_fund == UNION))	/* normal case */
			tp = tp->tp_up;
		else {	/* constructions like "12->selector" and
				"char c; c->selector"
			*/
			switch (tp->tp_fund)	{
			case POINTER:
				break;
			case INT:
			case LONG:
				/* An error is given in idf2sdef() */
				ch3cast(expp, CAST, pa_type);
				sd = idf2sdef(idf, tp);
				tp = sd->sd_stype;
				break;
			default:
				expr_error(exp, "-> applied to %s",
					symbol2str(tp->tp_fund));
			case ERRONEOUS:
				exp->ex_type = error_type;
				return;
			}
		}
	} else {		/* oper == '.' */
		/* nothing */
	}
	exp = *expp;
	switch (tp->tp_fund)	{
	case POINTER:	/* for int *p;	p->next = ...	*/
	case STRUCT:
	case UNION:
		break;
	case INT:
	case LONG:
		/* warning will be given by idf2sdef() */
		break;
	default:
		if (!is_anon_idf(idf))
			expr_error(exp, "selector %s applied to %s",
				idf->id_text, symbol2str(tp->tp_fund));
	case ERRONEOUS:
		exp->ex_type = error_type;
		return;
	}
	sd = idf2sdef(idf, tp);
	if (oper == '.')	{
		/*	there are 3 cases in which the selection can be
			performed compile-time: 
			I:	n.sel (n either an identifier or a constant)
			II:	(e.s1).s2 (transformed into (e.(s1+s2)))
			III:	(e->s1).s2 (transformed into (e->(s1+s2)))
				The code performing these conversions is
				extremely obscure.
		*/
		if (exp->ex_class == Value)	{
			/*	It is an object we know the address of; so
				we can calculate the address of the
				selected member 
			*/
			exp->VL_VALUE += sd->sd_offset;
			exp->ex_type = sd->sd_type;
			exp->ex_lvalue = exp->ex_type->tp_fund != ARRAY;
			if (exp->ex_type == error_type) {
				exp->ex_flags |= EX_ERROR;
			}
		}
		else
		if (exp->ex_class == Oper)	{
			struct oper *op = &(exp->ex_object.ex_oper);
			
			if (op->op_oper == '.' || op->op_oper == ARROW)	{
				ASSERT(is_cp_cst(op->op_right));
				op->op_right->VL_VALUE += sd->sd_offset;
				exp->ex_type = sd->sd_type;
				exp->ex_lvalue = exp->ex_type->tp_fund != ARRAY;
				if (exp->ex_type == error_type) {
					exp->ex_flags |= EX_ERROR;
				}
			}
			else {
				exp = new_oper(sd->sd_type, exp, '.',
						intexpr(sd->sd_offset, INT));
				exp->ex_lvalue = sd->sd_type->tp_fund != ARRAY;
				if (!exp->OP_LEFT->ex_lvalue)
					exp->ex_flags |= EX_ILVALUE;
			}
		}
	}
	else { /* oper == ARROW */
		exp = new_oper(sd->sd_type,
			exp, oper, intexpr(sd->sd_offset, INT));
		exp->ex_lvalue = (sd->sd_type->tp_fund != ARRAY);
		exp->ex_flags &= ~EX_ILVALUE;
	}
	if (sd->sd_type->tp_typequal & TQ_CONST)
		exp->ex_flags |= EX_READONLY;
	if (sd->sd_type->tp_typequal & TQ_VOLATILE)
		exp->ex_flags |= EX_VOLATILE;
	if (oper == '.' && exp->ex_flags & EX_READONLY)  {
		exp->ex_type = qualifier_type(exp->ex_type, TQ_CONST);
	}
	*expp = exp;
}

ch3incr(expp, oper)
	struct expr **expp;
{
	/*	The monadic prefix/postfix incr/decr operator oper is
		applied to *expp.
	*/
	ch3asgn(expp, oper, intexpr((arith)1, INT));
}

ch3cast(expp, oper, tp)
	register struct expr **expp;
	register struct type *tp;
{
	/*	The expression *expp is cast to type tp; the cast is
		caused by the operator oper.  If the cast has
		to be passed on to run time, its left operand will be an
		expression of class Type.
	*/
	register struct type *oldtp;

	if (oper == RETURN && tp->tp_fund == VOID) {
		strict("return <expression> in function returning void");
		(*expp)->ex_type = void_type;
		return;
	}
	if ((*expp)->ex_type->tp_fund == FUNCTION)
		function2pointer(*expp);
	if ((*expp)->ex_type->tp_fund == ARRAY)
		array2pointer(*expp);
	if ((*expp)->ex_class == String)
		string2pointer(*expp);
	oldtp = (*expp)->ex_type;

#ifndef NOBITFIELD
	if (oldtp->tp_fund == FIELD)	{
		field2arith(expp);
		ch3cast(expp, oper, tp);
	}
	else
	if (tp->tp_fund == FIELD) {
		ch3cast(expp, oper, tp->tp_up);
	}
	else
#endif NOBITFIELD
	if (equal_type(tp, oldtp, oper != CAST)) {
		/* life is easy */
		(*expp)->ex_type = tp;	/* so qualifiers are allright */
	}
	else
	if (tp->tp_fund == VOID) {
		/* easy again */
		(*expp)->ex_type = void_type;
	}
	else
	if (is_arith_type(oldtp) && is_arith_type(tp))	{
		int oldi = is_integral_type(oldtp);
		int i = is_integral_type(tp);

		if (oldi && i)	{
#ifdef	LINT
			if (oper == CAST)
				(*expp)->ex_type = tp;
			else
				int2int(expp, tp);
#else	LINT
			int2int(expp, tp);
#endif	LINT
		}
		else
		if (oldi && !i)	{
#ifdef	LINT
			if (oper == CAST)
				(*expp)->ex_type = tp;
			else
				int2float(expp, tp);
#else	LINT
			int2float(expp, tp);
#endif	LINT
		}
		else
		if (!oldi && i) {
#ifdef	LINT
			if (oper == CAST)
				(*expp)->ex_type = tp;
			else
				float2int(expp, tp);
#else	LINT
			float2int(expp, tp);
#endif	LINT
		}
		else {
			/* !oldi && !i */
#ifdef	LINT
			if (oper == CAST)
				(*expp)->ex_type = tp;
			else
				float2float(expp, tp);
#else	LINT
			float2float(expp, tp);
#endif	LINT
		}
	}
	else
	if (oldtp->tp_fund == POINTER && tp->tp_fund == POINTER)	{
		if (oper == CASTAB)
			expr_warning(*expp, "incompatible pointers");
		else
		if (oper != CAST)
			expr_warning(*expp, "incompatible pointers in %s",
							symbol2str(oper));
#ifdef	LINT
		if (oper != CAST)
			lint_ptr_conv(oldtp->tp_up->tp_fund, tp->tp_up->tp_fund);
#endif	LINT
		(*expp)->ex_type = tp;	/* free conversion */
	}
	else
	if (oldtp->tp_fund == POINTER && is_integral_type(tp))	{
		/* from pointer to integral */
		if (oper != CAST)
			expr_warning(*expp,
				"illegal conversion of pointer to %s",
				symbol2str(tp->tp_fund));
		if (oldtp->tp_size > tp->tp_size)
			expr_warning(*expp,
				"conversion of pointer to %s loses accuracy",
				symbol2str(tp->tp_fund));
		if (oldtp->tp_size != tp->tp_size)
			int2int(expp, tp);
		else
			(*expp)->ex_type = tp;
	}
	else
	if (tp->tp_fund == POINTER && is_integral_type(oldtp))	{
		/* from integral to pointer */
		switch (oper)	{
		case CAST:
			break;
		case CASTAB:
		case EQUAL:
		case NOTEQUAL:
		case '=':
		case RETURN:
			if (is_cp_cst(*expp) && (*expp)->VL_VALUE == (arith)0)
				break;
		default:
			expr_warning(*expp,
				"illegal conversion of %s to pointer",
				symbol2str(oldtp->tp_fund));
			break;
		}
		if (oldtp->tp_size > tp->tp_size)
			expr_warning(*expp,
				"conversion of %s to pointer loses accuracy",
				symbol2str(oldtp->tp_fund));
		if (oldtp->tp_size != tp->tp_size)
			int2int(expp, tp);
		else
			(*expp)->ex_type = tp;
	}
	else
	if (oldtp->tp_fund == ERRONEOUS) {
		/* we just won't look */
		(*expp)->ex_type = tp;	/* brute force */
	}
	else
	if (oldtp->tp_size == tp->tp_size && oper == CAST)	{
		expr_warning(*expp, "dubious conversion based on equal size");
		(*expp)->ex_type = tp;		/* brute force */
	}
	else	{
		if (oldtp->tp_fund != ERRONEOUS && tp->tp_fund != ERRONEOUS)
			expr_error(*expp, "cannot convert %s to %s",
				symbol2str(oldtp->tp_fund),
				symbol2str(tp->tp_fund)
			);
		(*expp)->ex_type = tp;		/* brute force */
	}
	if (oper == CAST) {
		(*expp)->ex_flags |= EX_ILVALUE;
	}
}

/*	Determine whether two types are equal.
*/
equal_type(tp, otp, check_qual)
	register struct type *tp, *otp;
	int check_qual;
{
	if (tp == otp)
		return 1;
	if (!tp
	    || !otp
	    || (tp->tp_fund != otp->tp_fund)
	    || (tp->tp_unsigned != otp->tp_unsigned)
	    || (tp->tp_align != otp->tp_align))
		return 0;
	if (tp->tp_fund != ARRAY /* && tp->tp_fund != STRUCT */ ) {	/* UNION ??? */
		if (tp->tp_size != otp->tp_size)
			return 0;
	}

	switch (tp->tp_fund) {

	case FUNCTION:
		/*	If both types have parameter type lists, the type of
			each parameter in the composite parameter type list
			is the composite type of the corresponding paramaters.
		*/
		if (tp->tp_proto && otp->tp_proto) {
			if (!equal_proto(tp->tp_proto, otp->tp_proto))
				return 0;
		} else if (tp->tp_proto || otp->tp_proto) {
			if (!legal_mixture(tp, otp))
				return 0;
		}
		return equal_type(tp->tp_up, otp->tp_up, 0);

	case ARRAY:
		/*	If one type is an array of known size, the composite
			type is an array of that size
		*/
		if (tp->tp_size != otp->tp_size &&
		     (tp->tp_size != -1 && otp->tp_size != -1))
			return 0;
		return equal_type(tp->tp_up, otp->tp_up, check_qual);

	case POINTER:
		if (equal_type(tp->tp_up, otp->tp_up, check_qual)) {
		    if (check_qual) {
			if (otp->tp_up->tp_typequal & TQ_CONST) {
			    if (!(tp->tp_up->tp_typequal & TQ_CONST)) {
				strict("illegal use of pointer to const object");
			    }
			}
			if (otp->tp_up->tp_typequal & TQ_VOLATILE) {
			    if (!(tp->tp_up->tp_typequal & TQ_VOLATILE)) {
				strict("illegal use of pointer to volatile object");
			    }
			}
		    }
		    return 1;
		}
		else return 0;

	case FIELD:
		return equal_type(tp->tp_up, otp->tp_up, check_qual);

	case STRUCT:
	case UNION:
	case ENUM:
		return tp->tp_idf == otp->tp_idf && tp->tp_sdef == otp->tp_sdef;

	default:
		return 1;
	}
}

check_pseudoproto(pl, opl)
	register struct proto *pl, *opl;
{
	int retval = 1;

	if (pl->pl_flag & PL_ELLIPSIS) {
		error("illegal ellipsis terminator");
		return 2;
	}
	if (opl->pl_flag & PL_VOID) {
		if (!(pl->pl_flag & PL_VOID))
			error("function is defined without parameters");
		pl->pl_flag |= PL_ERRGIVEN;
		opl->pl_flag |= PL_ERRGIVEN;
		return 2;
	}
	while (pl && opl) {
	    if (!equal_type(pl->pl_type, opl->pl_type, 0)) {
		if (!(pl->pl_flag & PL_ERRGIVEN)
		    && !(opl->pl_flag & PL_ERRGIVEN))
			error("incorrect type for parameter %s of definition",
				opl->pl_idf->id_text);
		pl->pl_flag |= PL_ERRGIVEN;
		opl->pl_flag |= PL_ERRGIVEN;
		retval = 2;
	    }
	    pl = pl->next;
	    opl = opl->next;
	}
	if (pl || opl) {
		error("incorrect number of parameters");
		retval = 2;
	}
	return retval;
}

legal_mixture(tp, otp)
	struct type *tp, *otp;
{
	register struct proto *pl = tp->tp_proto, *opl = otp->tp_proto;
	int retval = 1;
	struct proto *prot;
	int fund;

	ASSERT( (pl != 0) ^ (opl != 0));
	if (pl)  {
		prot = pl;
	} else  {
		prot = opl;
	}
	if (!opl && otp->tp_pseudoproto) {
		return check_pseudoproto(tp->tp_proto, otp->tp_pseudoproto);
	}

	if (prot->pl_flag & PL_ELLIPSIS) {
		if (!(prot->pl_flag & PL_ERRGIVEN)) {
			if (pl)
				error("illegal ellipsis terminator");
			else	error("ellipsis terminator in previous (prototype) declaration");
		}
		prot->pl_flag |= PL_ERRGIVEN;
		prot = prot->next;
		return 2;
	}
	while (prot) {
				/* if (!(prot->pl_flag & PL_ELLIPSIS)) {} */
		fund = prot->pl_type->tp_fund;
		if (fund == CHAR || fund == SHORT || fund == FLOAT) {
			if (!(prot->pl_flag & PL_ERRGIVEN))
			    error("illegal %s parameter in %sdeclaration",
				symbol2str(fund), (opl ? "previous (prototype) " : "" ));
			prot->pl_flag |= PL_ERRGIVEN;
			retval = 2;
		}
		prot = prot->next;
	}
	return retval;
}

equal_proto(pl, opl)
	register struct proto *pl, *opl;
{
	if (pl == opl)
		return 1;

	/*	If only one type is a function type with a parameter type list
		(a function prototype), the composite type is a function
		prototype with parameter type list.
	*/
	while ( pl && opl) {

	    if ((pl->pl_flag & ~PL_ERRGIVEN) != (opl->pl_flag & ~PL_ERRGIVEN))
		return 0;

	    if (!equal_type(pl->pl_type, opl->pl_type, 0))
		return 0;

	    pl = pl->next;
	    opl = opl->next;
	}
	return !(pl || opl);
}

/* check if a type has a const declared member */
recurconst(tp)
struct type *tp;
{
	register struct sdef *sdf;

	ASSERT(tp);
	if (!tp) return 0;
	if (tp->tp_typequal & TQ_CONST) return 1;
	sdf = tp->tp_sdef;
	while (sdf) {
		if (recurconst(sdf->sd_type))
			return 1;
		sdf = sdf->sd_sdef;
	}
	return 0;
}

ch3asgn(expp, oper, expr)
	struct expr **expp;
	struct expr *expr;
{
	/*	The assignment operators.
		"f op= e" should be interpreted as
		"f = (typeof f)((typeof (f op e))f op (typeof (f op e))e)"
		and not as "f = f op (typeof f)e".
		Consider, for example, (i == 10) i *= 0.9; (i == 9), where
		typeof i == int.
		The resulting expression tree becomes:
				op=
				/ \
			       /   \
			      f     (typeof (f op e))e
		EVAL should however take care of evaluating (typeof (f op e))f
	*/
	register struct expr *exp = *expp;
	int fund = exp->ex_type->tp_fund;
	int vol = 0;
	struct type *tp;

	/* We expect an lvalue */
	if (!exp->ex_lvalue) {
		expr_error(exp, "no lvalue in operand of %s", symbol2str(oper));
	} else if (exp->ex_flags & EX_ILVALUE)	{
		strict("incorrect lvalue in operand of %s", symbol2str(oper));
	} else if (exp->ex_flags & EX_READONLY) {
		expr_error(exp, "operand of %s is read-only", symbol2str(oper));
	} else if (fund == STRUCT || fund == UNION) {
		if (recurconst(exp->ex_type))
			expr_error(expr,"operand of %s contains a const-qualified member",
					    symbol2str(oper));
	}

	/*	Preserve volatile markers across the tree.
		This is questionable, depending on the way the optimizer
		wants this information.
	*/
	vol = (exp->ex_flags & EX_VOLATILE) || (expr->ex_flags & EX_VOLATILE);

	if (oper == '=') {
		ch3cast(&expr, oper, exp->ex_type);
		tp = expr->ex_type;
	}
	else {	/* turn e into e' where typeof(e') = typeof (f op e) */
		struct expr *extmp = intexpr((arith)0, INT);

		/* this is really $#@&*%$# ! */
		/* if you correct this, please correct lint_new_oper() too */
		extmp->ex_lvalue = 1;
		extmp->ex_type = exp->ex_type;
		ch3bin(&extmp, oper, expr);
		/* Note that ch3bin creates a tree of the expression
			((typeof (f op e))f op (typeof (f op e))e),
		   where f ~ extmp and e ~ expr.
		   We want to use (typeof (f op e))e.
		   Ch3bin does not create a tree if both operands
		   were illegal or constants!
		*/
		tp = extmp->ex_type;	/* perform the arithmetic in type tp */
		if (extmp->ex_class == Oper) {
			expr = extmp->OP_RIGHT;
			extmp->OP_RIGHT = NILEXPR;
			free_expression(extmp);
		}
		else
			expr = extmp;
	}
#ifndef NOBITFIELD
	if (fund == FIELD)
		exp = new_oper(exp->ex_type->tp_up, exp, oper, expr);
	else
		exp = new_oper(exp->ex_type, exp, oper, expr);
#else NOBITFIELD
	exp = new_oper(exp->ex_type, exp, oper, expr);
#endif NOBITFIELD
	exp->OP_TYPE = tp;	/* for EVAL() */
	exp->ex_flags |= vol ? (EX_SIDEEFFECTS|EX_VOLATILE) : EX_SIDEEFFECTS;
	*expp = exp;
}

/*	Some interesting (?) questions answered.
*/
int
is_integral_type(tp)
	register struct type *tp;
{
	switch (tp->tp_fund)	{
	case GENERIC:
	case CHAR:
	case SHORT:
	case INT:
	case LONG:
	case ENUM:
		return 1;
#ifndef NOBITFIELD
	case FIELD:
		return is_integral_type(tp->tp_up);
#endif NOBITFIELD
	default:
		return 0;
	}
}

int
is_arith_type(tp)
	register struct type *tp;
{
	switch (tp->tp_fund)	{
	case GENERIC:
	case CHAR:
	case SHORT:
	case INT:
	case LONG:
	case ENUM:
	case FLOAT:
	case DOUBLE:
	case LNGDBL:
		return 1;
#ifndef NOBITFIELD
	case FIELD:
		return is_arith_type(tp->tp_up);
#endif NOBITFIELD
	default:
		return 0;
	}
}
