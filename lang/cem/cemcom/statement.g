/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
/* $Header$ */
/*	STATEMENT SYNTAX PARSER	*/

{
#include	"lint.h"
#ifndef	LINT
#include	<em.h>
#else
#include	"em_lint.h"
#include	"l_lint.h"
#endif	LINT

#include	"debug.h"
#include	"botch_free.h"

#include	"arith.h"
#include	"LLlex.h"
#include	"type.h"
#include	"idf.h"
#include	"label.h"
#include	"expr.h"
#include	"code.h"
#include	"stack.h"
#include	"def.h"

extern int level;
}

/*	Each statement construction is stacked in order to trace a ???
	statement to such a construction. Example: a case statement should
	be recognized as a piece of the most enclosing switch statement.
*/

/* 9 */
statement
	{
#ifdef	LINT
		lint_statement();
#endif	LINT
	}
:
%if (AHEAD != ':')
	expression_statement
|
	label ':' statement
|
	compound_statement
|
	if_statement
|
	while_statement
|
	do_statement
|
	for_statement
|
	switch_statement
|
	case_statement
|
	default_statement
|
	BREAK
	{
		code_break();
#ifdef	LINT
		lint_break_stmt();
#endif	LINT
	}
	';'
|
	CONTINUE
	{
		code_continue();
#ifdef	LINT
		lint_continue_stmt();
#endif	LINT
	}
	';'
|
	return_statement
|
	jump
|
	';'
|
	asm_statement
;


expression_statement
	{	struct expr *expr;
	}
:
	expression(&expr)
	';'
		{
#ifdef	DEBUG
			print_expr("expression_statement", expr);
#endif	DEBUG
			code_expr(expr, RVAL, FALSE, NO_LABEL, NO_LABEL);
			free_expression(expr);
		}
;

label
	{	struct idf *idf;
	}
:
	identifier(&idf)
	{
		/*	This allows the following absurd case:

				typedef int grz;
				main()	{
					grz: printf("A labelled statement\n");
				}
		*/
#ifdef	LINT
		lint_label();
#endif	LINT
		define_label(idf);
		C_df_ilb((label)idf->id_def->df_address);
	}
;

if_statement
	{
		struct expr *expr;
		label l_true = text_label();
		label l_false = text_label();
		label l_end = text_label();
	}
:
	IF
	'('
	expression(&expr)
		{
			opnd2test(&expr, IF);
			if (is_cp_cst(expr))	{
				/*	The comparison has been optimized
					to a 0 or 1.
				*/
				if (expr->VL_VALUE == (arith)0)	{
					C_bra(l_false);
				}
				/* else fall through */
#ifdef	LINT
				start_if_part(1);
#endif	LINT
			}
			else	{
				code_expr(expr, RVAL, TRUE, l_true, l_false);
				C_df_ilb(l_true);
#ifdef	LINT
				start_if_part(0);
#endif	LINT
			}
			free_expression(expr);
		}
	')'
	statement
	[%prefer
		ELSE
			{
#ifdef	LINT
				start_else_part();
#endif	LINT
				C_bra(l_end);
				C_df_ilb(l_false);
			}
		statement
			{	C_df_ilb(l_end);
#ifdef	LINT
				end_if_else_stmt();
#endif	LINT
			}
	|
		empty
			{	C_df_ilb(l_false);
#ifdef	LINT
				end_if_stmt();
#endif	LINT
			}
	]
;

while_statement
	{
		struct expr *expr;
		label l_break = text_label();
		label l_continue = text_label();
		label l_body = text_label();
	}
:
	WHILE
		{
			stack_stmt(l_break, l_continue);
			C_df_ilb(l_continue);
		}
	'('
	expression(&expr)
		{
			opnd2test(&expr, WHILE);
			if (is_cp_cst(expr))	{
				if (expr->VL_VALUE == (arith)0)	{
					C_bra(l_break);
				}
			}
			else	{
				code_expr(expr, RVAL, TRUE, l_body, l_break);
				C_df_ilb(l_body);
			}

#ifdef	LINT
			start_while_stmt(expr);
#endif	LINT

		}
	')'
	statement
		{
			C_bra(l_continue);
			C_df_ilb(l_break);
			unstack_stmt();
			free_expression(expr);
#ifdef	LINT
			end_loop_body();
			end_loop_stmt();
#endif	LINT
		}
;

do_statement
	{	struct expr *expr;
		label l_break = text_label();
		label l_continue = text_label();
		label l_body = text_label();
	}
:
	DO
		{	C_df_ilb(l_body);
			stack_stmt(l_break, l_continue);
#ifdef	LINT
			start_do_stmt();
#endif	LINT
		}
	statement
	WHILE
	'('
		{
#ifdef	LINT
			end_loop_body();
#endif	LINT
			C_df_ilb(l_continue);
		}
	expression(&expr)
		{
			opnd2test(&expr, WHILE);
			if (is_cp_cst(expr))	{
				if (expr->VL_VALUE == (arith)1)	{
					C_bra(l_body);
				}
#ifdef	LINT
				end_do_stmt(1, expr->VL_VALUE != (arith)0);
#endif	LINT
			}
			else	{
				code_expr(expr, RVAL, TRUE, l_body, l_break);
#ifdef	LINT
				end_do_stmt(0, 0);
#endif	LINT
			}
			C_df_ilb(l_break);
		}
	')'
	';'
		{
			unstack_stmt();
			free_expression(expr);
		}
;

for_statement
	{	struct expr *e_init = 0, *e_test = 0, *e_incr = 0;
		label l_break = text_label();
		label l_continue = text_label();
		label l_body = text_label();
		label l_test = text_label();
	}
:
	FOR
		{	stack_stmt(l_break, l_continue);
		}
	'('
	[
		expression(&e_init)
		{	code_expr(e_init, RVAL, FALSE, NO_LABEL, NO_LABEL);
		}
	]?
	';'
		{	C_df_ilb(l_test);
		}
	[
		expression(&e_test)
		{
			opnd2test(&e_test, FOR);
			if (is_cp_cst(e_test))	{
				if (e_test->VL_VALUE == (arith)0)	{
					C_bra(l_break);
				}
			}
			else	{
				code_expr(e_test, RVAL, TRUE, l_body, l_break);
				C_df_ilb(l_body);
			}
		}
	]?
	';'
	expression(&e_incr)?
	')'
		{
#ifdef	LINT
			start_for_stmt(e_test);
#endif	LINT
		}
	statement
		{
#ifdef	LINT
			end_loop_body();
#endif	LINT
			C_df_ilb(l_continue);
			if (e_incr)
				code_expr(e_incr, RVAL, FALSE,
							NO_LABEL, NO_LABEL);
			C_bra(l_test);
			C_df_ilb(l_break);
			unstack_stmt();
			free_expression(e_init);
			free_expression(e_test);
			free_expression(e_incr);
#ifdef	LINT
			end_loop_stmt();
#endif	LINT
		}
;

switch_statement
	{
		struct expr *expr;
	}
:
	SWITCH
	'('
	expression(&expr)
		{
			code_startswitch(&expr);
#ifdef	LINT
			start_switch_part(is_cp_cst(expr));
#endif	LINT
		}
	')'
	statement
		{
#ifdef	LINT
			end_switch_stmt();
#endif	LINT
			code_endswitch();
			free_expression(expr);
		}
;

case_statement
	{
		struct expr *expr;
	}
:
	CASE
	constant_expression(&expr)
		{
#ifdef	LINT
			lint_case_stmt(0);
#endif	LINT
			code_case(expr);
			free_expression(expr);
		}
	':'
	statement
;

default_statement
:
	DEFAULT
		{
#ifdef	LINT
			lint_case_stmt(1);
#endif	LINT
			code_default();
		}
	':'
	statement
;

return_statement
	{	struct expr *expr = 0;
	}
:
	RETURN
	[
		expression(&expr)
		{
#ifdef	LINT
			lint_ret_conv(expr);
#endif	LINT

			do_return_expr(expr);
			free_expression(expr);
#ifdef	LINT
			lint_return_stmt(VALRETURNED);
#endif	LINT
		}
	|
		empty
		{
			do_return();
#ifdef	LINT
			lint_return_stmt(NOVALRETURNED);
#endif	LINT
		}
	]
	';'
;

jump
	{	struct idf *idf;
	}
:
	GOTO
	identifier(&idf)
	';'
		{
			apply_label(idf);
			C_bra((label)idf->id_def->df_address);
#ifdef	LINT
			lint_jump_stmt(idf);
#endif	LINT
		}
;

compound_statement:
	'{'
		{
			stack_level();
		}
	[%while ((DOT != IDENTIFIER && AHEAD != ':') ||
		 (DOT == IDENTIFIER && AHEAD == IDENTIFIER))
			/* >>> conflict on TYPE_IDENTIFIER, IDENTIFIER */
		declaration
	]*
	[%persistent
		statement
	]*
	'}'
		{
			unstack_level();
		}
;

asm_statement
	{	char *asm_bts;
		int asm_len;
	}
:
	ASM
	'('
	STRING
		{	asm_bts = dot.tk_bts;
			asm_len = dot.tk_len;
		}
	')'
	';'
		{	code_asm(asm_bts, asm_len);
		}
;

