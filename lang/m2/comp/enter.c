/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 *
 * Author: Ceriel J.H. Jacobs
 */

/* H I G H   L E V E L   S Y M B O L   E N T R Y */

/* $Header$ */

#include	"debug.h"

#include	<alloc.h>
#include	<em_arith.h>
#include	<em_label.h>
#include	<em_code.h>
#include	<assert.h>

#include	"idf.h"
#include	"LLlex.h"
#include	"def.h"
#include	"type.h"
#include	"scope.h"
#include	"node.h"
#include	"main.h"
#include	"misc.h"
#include	"f_info.h"

t_def *
Enter(name, kind, type, pnam)
	char *name;
	t_type *type;
{
	/*	Enter a definition for "name" with kind "kind" and type
		"type" in the Current Scope. If it is a standard name, also
		put its number in the definition structure.
	*/
	register t_def *df;

	df = define(str2idf(name, 0), CurrentScope, kind);
	df->df_type = type;
	if (pnam) df->df_value.df_stdname = pnam;
	return df;
}

EnterType(name, type)
	char *name;
	t_type *type;
{
	/*	Enter a type definition for "name"  and type
		"type" in the Current Scope.
	*/

	if (! Enter(name, D_TYPE, type, 0)) {
		assert(0);
	}
}

EnterEnumList(Idlist, type)
	t_node *Idlist;
	register t_type *type;
{
	/*	Put a list of enumeration literals in the symbol table.
		They all have type "type".
		Also assign numbers to them, and link them together.
		We must link them together because an enumeration type may
		be exported, in which case its literals must also be exported.
		Thus, we need an easy way to get to them.
	*/
	register t_def *df;
	register t_node *idlist = Idlist;

	type->enm_ncst = 0;
	for (; idlist; idlist = idlist->nd_left) {
		df = define(idlist->nd_IDF, CurrentScope, D_ENUM);
		df->df_type = type;
		df->enm_val = (type->enm_ncst)++;
		df->enm_next = type->enm_enums;
		type->enm_enums = df;
	}
	FreeNode(Idlist);
}

EnterFieldList(Idlist, type, scope, addr)
	t_node *Idlist;
	register t_type *type;
	t_scope *scope;
	arith *addr;
{
	/*	Put a list of fields in the symbol table.
		They all have type "type", and are put in scope "scope".
		Mark them as QUALIFIED EXPORT, because that's exactly what
		fields are, you can get to them by qualifying them.
	*/
	register t_def *df;
	register t_node *idlist = Idlist;

	for (; idlist; idlist = idlist->nd_left) {
		df = define(idlist->nd_IDF, scope, D_FIELD);
		df->df_type = type;
		df->df_flags |= D_QEXPORTED;
		df->fld_off = align(*addr, type->tp_align);
		*addr = df->fld_off + type->tp_size;
	}
	FreeNode(Idlist);
}

EnterVarList(Idlist, type, local)
	t_node *Idlist;
	t_type *type;
{
	/*	Enter a list of identifiers representing variables into the
		name list. "type" represents the type of the variables.
		"local" is set if the variables are declared local to a
		procedure.
	*/
	register t_def *df;
	register t_node *idlist = Idlist;
	register t_scopelist *sc = CurrVis;
	char buf[256];
	extern char *sprint();

	if (local) {
		/* Find the closest enclosing open scope. This
		   is the procedure that we are dealing with
		*/
		while (sc->sc_scope->sc_scopeclosed) sc = enclosing(sc);
	}

	for (; idlist; idlist = idlist->nd_right) {
		df = define(idlist->nd_IDF, CurrentScope, D_VARIABLE);
		df->df_type = type;
		if (idlist->nd_left) {
			/* An address was supplied
			*/
			register t_type *tp = idlist->nd_left->nd_type;

			df->df_flags |= D_ADDRGIVEN | D_NOREG;
			if (tp != error_type && !(tp->tp_fund & T_CARDINAL)){
				node_error(idlist->nd_left,
					   "illegal type for address");
			}
			df->var_off = idlist->nd_left->nd_INT;
		}
		else if (local) {
			/* subtract aligned size of variable to the offset,
			   as the variable list exists only local to a
			   procedure
			*/
			sc->sc_scope->sc_off =
				-WA(align(type->tp_size - sc->sc_scope->sc_off,
						type->tp_align));
			df->var_off = sc->sc_scope->sc_off;
		}
		else {
			/* Global name, possibly external
			*/
			if (sc->sc_scope->sc_definedby->df_flags & D_FOREIGN) {
				df->var_name = df->df_idf->id_text;
			}
			else {
				sprint(buf,"%s_%s", sc->sc_scope->sc_name,
					    df->df_idf->id_text);
				df->var_name = Salloc(buf,
						(unsigned)(strlen(buf)+1));
			}
			df->df_flags |= D_NOREG;

 			if (DefinitionModule) {
				df->df_flags |= D_USED | D_DEFINED;
				if (sc == Defined->mod_vis) {
					C_exa_dnam(df->var_name);
				}
			}
			else {
				C_ina_dnam(df->var_name);
			}
		}
	}
	FreeNode(Idlist);
}

EnterParamList(ppr, Idlist, type, VARp, off)
	t_param **ppr;
	t_node *Idlist;
	t_type *type;
	int VARp;
	arith *off;
{
	/*	Create (part of) a parameterlist of a procedure.
		"ids" indicates the list of identifiers, "tp" their type, and
		"VARp" indicates D_VARPAR or D_VALPAR.
	*/
	register t_param *pr;
	register t_def *df;
	register t_node *idlist = Idlist;
	t_node *dummy = 0;
	static t_param *last;

	if (! idlist) {
		/* Can only happen when a procedure type is defined */
		dummy = Idlist = idlist = dot2leaf(Name);
	}
	for ( ; idlist; idlist = idlist->nd_left) {
		pr = new_paramlist();
		pr->par_next = 0;
		if (!*ppr) *ppr = pr;
		else	last->par_next = pr;
		last = pr;
		if (!DefinitionModule && idlist != dummy) {
			df = define(idlist->nd_IDF, CurrentScope, D_VARIABLE);
			df->var_off = *off;
		}
		else	df = new_def();
		pr->par_def = df;
		df->df_type = type;
		df->df_flags |= (VARp | D_DEFINED);
		if (df->df_flags & D_VARPAR) df->df_flags |= D_USED;

		if (IsConformantArray(type)) {
			/* we need room for the base address and a descriptor
			*/
			*off += pointer_size + 3 * word_size;
		}
		else if (VARp == D_VARPAR) {
			*off += pointer_size;
		}
		else {
			*off += WA(type->tp_size);
		}
	}
	FreeNode(Idlist);
}

STATIC t_def *
DoImport(df, scope)
	register t_def *df;
	t_scope *scope;
{
	/*	Definition "df" is imported to scope "scope".
		Handle the case that it is an enumeration type or a module.
	*/
	register t_def *idef = define(df->df_idf, scope, D_IMPORT);

	idef->imp_def = df;

	while (df->df_kind & D_IMPORTED) {
		df = df->imp_def;
	}

	if (df->df_kind == D_TYPE && df->df_type->tp_fund == T_ENUMERATION) {
		/* Also import all enumeration literals
		*/
		for (df = df->df_type->enm_enums; df; df = df->enm_next) {
			register t_def *df1 = define(df->df_idf, scope, D_IMPORT);
			
			df1->imp_def = df;
			df1->df_flags |= D_USED;/* don't complain when these
						   are not used
						*/
		}
		idef->df_flags |= D_USED;	/* don't complain ... */
	}
	else if (df->df_kind == D_MODULE) {
		/* Also import all definitions that are exported from this
		   module
		*/
		if (df->mod_vis == CurrVis) {
			error("cannot import current module \"%s\"",
				df->df_idf->id_text);
			return idef;
		}
		for (df = df->mod_vis->sc_scope->sc_def;
		     df;
		     df = df->df_nextinscope) {
			if (df->df_flags & D_EXPORTED) {
				register t_def *df1 =
					define(df->df_idf, scope, D_IMPORT);
			
				df1->imp_def = df;
				df1->df_flags |= D_USED;
				/* don't complain when these are not used */
			}
		}
		idef->df_flags |= D_USED;	/* don't complain ... */
	}
	return idef;
}

STATIC t_scopelist *
ForwModule(df, nd)
	register t_def *df;
	t_node *nd;
{
	/*	An import is done from a not yet defined module "df".
		We could also end up here for not found DEFINITION MODULES.
		Create a declaration and a scope for this module.
	*/
	t_scopelist *vis;

	if (df->df_scope != GlobalScope) {
		df->df_scope = enclosing(CurrVis)->sc_scope;
		df->df_kind = D_FORWMODULE;
	}
	open_scope(CLOSEDSCOPE);
	vis = CurrVis;		/* The new scope, but watch out, it's "sc_encl"
				   field is not set right. It must indicate the
				   enclosing scope, but this must be done AFTER
				   closing this one
				*/
	close_scope(0);	
	vis->sc_encl = enclosing(CurrVis);
				/* Here ! */
	df->for_vis = vis;
	df->for_node = nd;
	return vis;
}

STATIC t_def *
ForwDef(ids, scope)
	register t_node *ids;
	t_scope *scope;
{
	/*	Enter a forward definition of "ids" in scope "scope",
		if it is not already defined.
	*/
	register t_def *df;

	if (!(df = lookup(ids->nd_IDF, scope, 0, 0))) {
		df = define(ids->nd_IDF, scope, D_FORWARD);
		df->for_node = MkLeaf(Name, &(ids->nd_token));
	}
	return df;
}

EnterExportList(Idlist, qualified)
	t_node *Idlist;
{
	/*	From the current scope, the list of identifiers "ids" is
		exported. Note this fact. If the export is not qualified, make
		all the "ids" visible in the enclosing scope by defining them
		in this scope as "imported".
	*/
	register t_node *idlist = Idlist;
	register t_def *df, *df1;

	for (;idlist; idlist = idlist->nd_left) {
		df = lookup(idlist->nd_IDF, CurrentScope, 0, 0);

		if (!df) {
			/* undefined item in export list
			*/
			node_error(idlist,
				   "identifier \"%s\" not defined",
				   idlist->nd_IDF->id_text);
			continue;
		}

		if (df->df_flags & (D_EXPORTED|D_QEXPORTED)) {
			node_error(idlist,
				"multiple occurrences of \"%s\" in export list",
				idlist->nd_IDF->id_text);
		}

		df->df_flags |= qualified;
		if (qualified == D_EXPORTED) {
			/* Export, but not qualified.
			   Find all imports of the module in which this export
			   occurs, and export the current definition to it
			*/
			df1 = CurrentScope->sc_definedby->df_idf->id_def;
			while (df1) {
				if ((df1->df_kind & D_IMPORTED) &&
				    df1->imp_def == CurrentScope->sc_definedby) {
					DoImport(df, df1->df_scope)->df_flags |=
						D_IMP_BY_EXP;
				}
				df1 = df1->df_next;
			}

			/* Also handle the definition as if the enclosing
			   scope imports it.
			*/
			df1 = lookup(idlist->nd_IDF,
				     enclosing(CurrVis)->sc_scope,
				     D_IMPORTED,
				     0);
			if (df1) {
				/* It was already defined in the enclosing
				   scope. There are two legal possibilities,
				   which are examined below.
				*/
				t_def *df2 = df;

				while (df2->df_kind & D_IMPORTED) {
					df2 = df2->imp_def;
				}
				if (df1->df_kind == D_PROCHEAD &&
				     df2->df_kind == D_PROCEDURE) {
					df1->df_kind = D_IMPORT;
					df1->df_flags |= D_IMP_BY_EXP;
					df1->imp_def = df;
					continue;
				}
				if (df1->df_kind == D_HIDDEN &&
				    df2->df_kind == D_TYPE) {
					DeclareType(idlist, df1, df2->df_type);
					df1->df_kind = D_TYPE;
					continue;
				}
			}

			DoImport(df, enclosing(CurrVis)->sc_scope)->df_flags |= 
				D_IMP_BY_EXP;
		}
	}
	FreeNode(Idlist);
}

EnterFromImportList(idlist, FromDef, FromId)
	register t_node *idlist;
	register t_def *FromDef;
	t_node *FromId;
{
	/*	Import the list Idlist from the module indicated by Fromdef.
	*/
	register t_scopelist *vis;
	register t_def *df;
	char *module_name = FromDef->df_idf->id_text;
	int forwflag = 0;

	switch(FromDef->df_kind) {
	case D_ERROR:
		/* The module from which the import was done
		   is not yet declared. I'm not sure if I must
		   accept this, but for the time being I will.
		   We also end up here if some definition module could not
		   be found.
		   ???
		*/
		vis = ForwModule(FromDef, FromId);
		forwflag = 1;
		break;
	case D_FORWMODULE:
		vis = FromDef->for_vis;
		forwflag = 1;
		break;
	case D_MODULE:
		vis = FromDef->mod_vis;
		if (vis == CurrVis) {
node_error(FromId, "cannot import from current module \"%s\"", module_name);
			return;
		}
		break;
	default:
node_error(FromId,"identifier \"%s\" does not represent a module",module_name);
		return;
	}

	for (; idlist; idlist = idlist->nd_left) {
		if (forwflag) df = ForwDef(idlist, vis->sc_scope);
		else if (! (df = lookup(idlist->nd_IDF, vis->sc_scope, 0, 0))) {
			if (! is_anon_idf(idlist->nd_IDF)) {
				node_error(idlist,
			"identifier \"%s\" not declared in module \"%s\"",
					idlist->nd_IDF->id_text,
					module_name);
			}
			df = define(idlist->nd_IDF,vis->sc_scope,D_ERROR);
		}
		else if (! (df->df_flags & (D_EXPORTED|D_QEXPORTED))) {
			node_error(idlist,
			"identifier \"%s\" not exported from module \"%s\"",
			idlist->nd_IDF->id_text,
			module_name);
			df->df_flags |= D_QEXPORTED;
		}
		if (! DoImport(df, CurrentScope)) assert(0);
	}

	if (!forwflag) FreeNode(FromId);
}

EnterImportList(idlist, local)
	register t_node *idlist;
{
	/*	Import "idlist" from the enclosing scope.
		If the import is not local, definition modules must be read
		for "idlist".
	*/
	t_scope *sc = enclosing(CurrVis)->sc_scope;
	extern t_def *GetDefinitionModule();
	struct f_info f;
	
	f = file_info;

	for (; idlist; idlist = idlist->nd_left) {
		if (! DoImport(local ?
			   ForwDef(idlist, sc) :
			   GetDefinitionModule(idlist->nd_IDF, 1), 
			 CurrentScope)) assert(0);
		file_info = f;
	}
}
