/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
/* $Header$ */
/*		L A B E L   H A N D L I N G		*/

#include	"Lpars.h"
#include	"level.h"
#include	"idf.h"
#include	"label.h"
#include	"arith.h"
#include	"def.h"
#include	"type.h"

extern char options[];

define_label(idf)
	struct idf *idf;
{
	/*	The identifier idf is defined as a label. If it is new,
		it is entered into the idf list with the largest possible
		scope, i.e., on the lowest possible level.
	*/
	enter_label(idf, 1);
}

apply_label(idf)
	struct idf *idf;
{
	/*	The identifier idf is applied as a label. It may or may
		not be there, and if it is there, it may be from a
		declaration or another application.
	*/
	enter_label(idf, 0);
}

enter_label(idf, defining)
	register struct idf *idf;
{
	/*	The identifier idf is entered as a label. If it is new,
		it is entered into the idf list with the largest possible
		scope, i.e., on the lowest possible level.
		If defining, the label comes from a label statement.
	*/
	if (idf->id_def)	{
		register struct def *def = idf->id_def;
		
		if (def->df_sc == LABEL)	{
			if (defining && def->df_initialized)
				error("redeclaration of label %s",
								idf->id_text);
		}
		else	{		/* there may still be room for it */
			int deflevel = def->df_level;
			
			if (options['R'] && def->df_sc == TYPEDEF)
				warning("label %s is also a typedef",
					idf->id_text);
			
			if (deflevel == level)	/* but alas, no */
				error("%s is not a label", idf->id_text);
			else	{
				int lvl;
				
				if (options['R'] && deflevel > L_LOCAL)
					warning("label %s is not function-wide",
								idf->id_text);
				lvl = deflevel + 1;
				if (lvl < L_LOCAL)
					lvl = L_LOCAL;
				add_def(idf, LABEL, label_type, lvl);
			}
		}
	}
	else	{
		add_def(idf, LABEL, label_type, L_LOCAL);
	}
	if (idf->id_def->df_address == 0)
		idf->id_def->df_address = (arith) text_label();
	if (defining)
		idf->id_def->df_initialized = 1;
}

unstack_label(idf)
	register struct idf *idf;
{
	/*	The scope in which the label idf occurred is left.
	*/
	if (!idf->id_def->df_initialized && !is_anon_idf(idf))
		error("label %s not defined", idf->id_text);
}
