/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 *
 * Author: Ceriel J.H. Jacobs
 */

/* D E C L A R A T I O N S */

/* $Header$ */

{
#include	"debug.h"

#include	<em_arith.h>
#include	<em_label.h>
#include	<alloc.h>
#include	<assert.h>

#include	"strict3rd.h"
#include	"idf.h"
#include	"LLlex.h"
#include	"def.h"
#include	"type.h"
#include	"scope.h"
#include	"node.h"
#include	"misc.h"
#include	"main.h"
#include	"chk_expr.h"
#include	"warning.h"

int		proclevel = 0;		/* nesting level of procedures */
int		return_occurred;	/* set if a return occurs in a block */

extern t_node	*EmptyStatement;

#define needs_static_link()	(proclevel > 1)
}

/* inline in declaration: need space
 * ProcedureDeclaration
 * {
 * 	t_def *df;
 * } :
 * 			{	++proclevel; }
 * 	ProcedureHeading(&df, D_PROCEDURE)
 * 	';' block(&(df->prc_body))
 * 	IDENT
 * 			{	EndProc(df, dot.TOK_IDF);
 * 				--proclevel;
 * 			}
 * ;
*/

ProcedureHeading(t_def **pdf; int type;)
{
	t_type	*tp = 0;
	arith	parmaddr = needs_static_link() ? pointer_size : 0;
	t_param	*pr = 0;
} :
	PROCEDURE IDENT
			{ *pdf = DeclProc(type, dot.TOK_IDF); }
	[
		'('
		[
			FPSection(&pr, &parmaddr)
			[
				';' FPSection(&pr, &parmaddr)
			]*
		]?
		')'
		[	':' qualtype(&tp)
		]?
	]?
			{ CheckWithDef(*pdf, proc_type(tp, pr, parmaddr));
			  if (tp && IsConstructed(tp)) {
warning(W_STRICT, "procedure \"%s\" has a constructed result type",
	(*pdf)->df_idf->id_text);
			  }
			}
;

block(t_node **pnd;) :
	[	%persistent
		declaration
	]*
			{ return_occurred = 0; }
	[	%default
		BEGIN
		StatementSequence(pnd)
	|
			{ *pnd = EmptyStatement; }
	]
	END
;

declaration
{
	t_def *df;
} :
	CONST [ ConstantDeclaration ';' ]*
|
	TYPE [ TypeDeclaration ';' ]*
|
	VAR [ VariableDeclaration ';' ]*
|
			{	++proclevel; }
	ProcedureHeading(&df, D_PROCEDURE)
	';'
	block(&(df->prc_body))
	IDENT
			{	EndProc(df, dot.TOK_IDF);
				--proclevel;
			}
	';'
|
	ModuleDeclaration ';'
;

/* inline in procedureheading: need space
 * FormalParameters(t_param **ppr; arith *parmaddr; t_type **ptp;):
 * 	'('
 * 	[
 * 		FPSection(ppr, parmaddr)
 * 		[
 * 			';' FPSection(ppr, parmaddr)
 * 		]*
 * 	]?
 * 	')'
 * 	[	':' qualtype(ptp)
 * 	]?
 * ;
*/

FPSection(t_param **ppr; arith *parmaddr;)
{
	t_node	*FPList;
	t_type	*tp;
	int	VARp;
} :
	var(&VARp) IdentList(&FPList) ':' FormalType(&tp)
			{ EnterParamList(ppr, FPList, tp, VARp, parmaddr); }
;

FormalType(t_type **ptp;)
{
	extern arith ArrayElSize();
} :
	ARRAY OF qualtype(ptp)
		{ /* index type of conformant array is "CARDINAL".
		     Recognize a conformant array by size 0.
		  */
		  register t_type *tp = construct_type(T_ARRAY, card_type);

		  tp->arr_elem = *ptp;
		  *ptp = tp;
		  tp->arr_elsize = ArrayElSize(tp->arr_elem);
		  tp->tp_align = tp->arr_elem->tp_align;
		}
|
	 qualtype(ptp)
;

TypeDeclaration
{
	t_def *df;
	t_type *tp;
	register t_node *nd;
}:
	IDENT		{ df = define(dot.TOK_IDF, CurrentScope, D_TYPE);
			  nd = dot2leaf(Name);
			}
	'=' type(&tp)
			{ DeclareType(nd, df, tp);
			  FreeNode(nd);
			}
;

type(register t_type **ptp;):
	%default SimpleType(ptp)
|
	ArrayType(ptp)
|
	RecordType(ptp)
|
	SetType(ptp)
|
	PointerType(ptp)
|
	ProcedureType(ptp)
;

SimpleType(register t_type **ptp;)
{
	t_type *tp;
} :
	qualtype(ptp)
	[
		/* nothing */
	|
		SubrangeType(&tp)
		/* The subrange type is given a base type by the
		   qualident (this is new modula-2).
		*/
			{ chk_basesubrange(tp, *ptp); *ptp = tp; }
	]
|
	enumeration(ptp)
|
	SubrangeType(ptp)
;

enumeration(t_type **ptp;)
{
	t_node *EnumList;
} :
	'(' IdentList(&EnumList) ')'
		{ *ptp = enum_type(EnumList); }
;

IdentList(t_node **p;)
{
	register t_node *q;
} :
	IDENT		{ *p = q = dot2leaf(Value); }
	[ %persistent
		',' IDENT
			{ q->nd_left = dot2leaf(Value);
			  q = q->nd_left;
			}
	]*
			{ q->nd_left = 0; }
;

SubrangeType(t_type **ptp;)
{
	t_node *nd1, *nd2;
}:
	/*
	   This is not exactly the rule in the new report, but see
	   the rule for "SimpleType".
	*/
	'[' ConstExpression(&nd1)
	UPTO ConstExpression(&nd2)
	']'
			{ *ptp = subr_type(nd1, nd2);
			  FreeNode(nd1);
			  FreeNode(nd2);
			}
;

ArrayType(t_type **ptp;)
{
	t_type *tp;
	register t_type *tp2;
} :
	ARRAY SimpleType(&tp)
			{ *ptp = tp2 = construct_type(T_ARRAY, tp); }
	[
		',' SimpleType(&tp)
			{ tp2->arr_elem = construct_type(T_ARRAY, tp);
			  tp2 = tp2->arr_elem;
			}
	]* OF type(&tp)
			{ tp2->arr_elem = tp;
			  ArraySizes(*ptp);
			}
;

RecordType(t_type **ptp;)
{
	register t_scope *scope;
	arith size = 0;
	int xalign = struct_align;
}
:
	RECORD
		{ scope = open_and_close_scope(OPENSCOPE); }
	FieldListSequence(scope, &size, &xalign)
		{ if (size == 0) {
			warning(W_ORDINARY, "empty record declaration");
			size = 1;
		  }
		  *ptp = standard_type(T_RECORD, xalign, size);
		  (*ptp)->rec_scope = scope;
		}
	END
;

FieldListSequence(t_scope *scope; arith *cnt; int *palign;):
	FieldList(scope, cnt, palign)
	[
		';' FieldList(scope, cnt, palign)
	]*
;

FieldList(t_scope *scope; arith *cnt; int *palign;)
{
	t_node *FldList;
	t_type *tp;
	t_node *nd;
	register t_def *df;
	arith tcnt, max;
} :
[
	IdentList(&FldList) ':' type(&tp)
			{
			  *palign = lcm(*palign, tp->tp_align);
			  EnterFieldList(FldList, tp, scope, cnt);
			}
|
	CASE
	/* Also accept old fashioned Modula-2 syntax, but give a warning.
	   Sorry for the complicated code.
	*/
	[ qualident(&nd)
	  [ ':' qualtype(&tp)
			/* This is correct, in both kinds of Modula-2, if
		   	   the first qualident is a single identifier.
			*/
			{ if (nd->nd_class != Name) {
				error("illegal variant tag");
		  	  }
			  else {
			  	df = define(nd->nd_IDF, scope, D_FIELD);
			  	*palign = lcm(*palign, tp->tp_align);
			  	if (!(tp->tp_fund & T_DISCRETE)) {
					error("illegal type in variant");
			  	}
			  	df->df_type = tp;
			  	df->fld_off = align(*cnt, tp->tp_align);
			  	*cnt = df->fld_off + tp->tp_size;
			  	df->df_flags |= D_QEXPORTED;
			  }
			  FreeNode(nd);
			}
	  |		/* Old fashioned! the first qualident now represents
			   the type
			*/
			{
#ifndef STRICT_3RD_ED
			  if (! options['3']) warning(W_OLDFASHIONED,
			      "old fashioned Modula-2 syntax; ':' missing");
			  else
#endif
			  error("':' missing");
			  tp = qualified_type(nd);
			}
	  ]
	| ':' qualtype(&tp)
	  /* Aha, third edition. Well done! */
	]
			{ tcnt = *cnt; }
	OF variant(scope, &tcnt, tp, palign)
			{ max = tcnt; tcnt = *cnt; }
	[
	  '|' variant(scope, &tcnt, tp, palign)
			{ if (tcnt > max) max = tcnt; tcnt = *cnt; }
	]*
	[ ELSE FieldListSequence(scope, &tcnt, palign)
			{ if (tcnt > max) max = tcnt; }
	]?
	END
			{ *cnt = max; }
]?
;

variant(t_scope *scope; arith *cnt; t_type *tp; int *palign;)
{
	t_node *nd;
} :
	[
		CaseLabelList(&tp, &nd)
			{ /* Ignore the cases for the time being.
			     Maybe a checking version will be supplied
			     later ???
			  */
			  FreeNode(nd);
			}
		':' FieldListSequence(scope, cnt, palign)
	]?
			/* Changed rule in new modula-2 */
;

CaseLabelList(t_type **ptp; t_node **pnd;):
	CaseLabels(ptp, pnd)
	[	
			{ *pnd = dot2node(Link, *pnd, NULLNODE); }
		',' CaseLabels(ptp, &((*pnd)->nd_right))
			{ pnd = &((*pnd)->nd_right); }
	]*
;

CaseLabels(t_type **ptp; register t_node **pnd;)
{
	register t_node *nd;
}:
	ConstExpression(pnd)
			{ 
			  if (*ptp != 0) {
				ChkCompat(pnd, *ptp, "case label");
			  }
			  nd = *pnd;
			}
	[
		UPTO	{ *pnd = nd = dot2node(Link,nd,NULLNODE);
			  nd->nd_type = nd->nd_left->nd_type;
			}
		ConstExpression(&(*pnd)->nd_right)
			{ if (!ChkCompat(&((*pnd)->nd_right), nd->nd_type,
					 "case label")) {
			  	nd->nd_type = error_type;
			  }
			  else if (! chk_bounds(nd->nd_left->nd_INT,
						nd->nd_right->nd_INT,
						BaseType(nd->nd_type)->tp_fund)) {
			    node_error(nd,
			   "lower bound exceeds upper bound in case label range");
			  }

			}
	]?
			{
			  *ptp = nd->nd_type;
			}
;

SetType(t_type **ptp;) :
	SET OF SimpleType(ptp)
			{ *ptp = set_type(*ptp); }
;

/*	In a pointer type definition, the type pointed at does not
	have to be declared yet, so be careful about identifying
	type-identifiers
*/
PointerType(register t_type **ptp;) :
			{ *ptp = construct_type(T_POINTER, NULLTYPE); }
	POINTER TO
	[ %if	(type_or_forward(ptp))
	  type(&((*ptp)->tp_next)) 
	|
	  IDENT
	]
;

qualtype(t_type **ptp;)
{
	t_node *nd;
} :
	qualident(&nd)
		{ *ptp = qualified_type(nd); }
;

ProcedureType(t_type **ptp;) :
	PROCEDURE 
	[
	  	FormalTypeList(ptp)
	|
			{ *ptp = proc_type((t_type *) 0, 
					   (t_param *) 0,
					   (arith) 0);
			}
	]
;

FormalTypeList(t_type **ptp;)
{
	t_param *pr = 0;
	arith parmaddr = 0;
} :
	'('
	[
		VarFormalType(&pr, &parmaddr)
		[
			',' VarFormalType(&pr, &parmaddr)
		]*
	]?
	')'
	[ ':' qualtype(ptp)
	|		{ *ptp = 0; }
	]
			{ *ptp = proc_type(*ptp, pr, parmaddr); }
;

VarFormalType(t_param **ppr; arith *parmaddr;)
{
	t_type *tp;
	int isvar;
} :
	var(&isvar)
	FormalType(&tp)
			{ EnterParamList(ppr,NULLNODE,tp,isvar,parmaddr); }
;

var(int *VARp;) :
	[
		VAR		{ *VARp = D_VARPAR; }
	|
		/* empty */	{ *VARp = D_VALPAR; }
	]
;

ConstantDeclaration
{
	t_idf *id;
	t_node *nd;
	register t_def *df;
}:
	IDENT		{ id = dot.TOK_IDF; }
	'=' ConstExpression(&nd)
			{ df = define(id,CurrentScope,D_CONST);
			  df->con_const = nd->nd_token;
			  df->df_type = nd->nd_type;
			  FreeNode(nd);
			}
;

VariableDeclaration
{
	t_node *VarList;
	register t_node *nd;
	t_type *tp;
} :
	IdentAddr(&VarList)
			{ nd = VarList; }
	[ %persistent
		',' IdentAddr(&(nd->nd_right))
			{ nd = nd->nd_right; }
	]*
	':' type(&tp)
			{ EnterVarList(VarList, tp, proclevel > 0); }
;

IdentAddr(t_node **pnd;) 
{
	register t_node *nd;
} :
	IDENT		{ nd = dot2leaf(Name); }
	[	'['
		ConstExpression(&(nd->nd_left))
		']'
	]?
			{ *pnd = nd; }
;
