/* $Header$ */
/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
#include "whichone.h"

#ifndef TBL68020
#ifndef TBL68000
Something is very wrong here. You must specify the machine: either
TBL68000 or TBL68020, in the file whichone.h, then REMOVE tables.c
and then run "make" again
#endif
#endif

#define ex_ap(y)	fprintf(codefile,".extern %s\n",y)
#define in_ap(y)	/* nothing */

#define newilb(x)	fprintf(codefile,"%s:\n",x)
#define newdlb(x)	fprintf(codefile,"%s:\n",x)
#define	dlbdlb(x,y)	fprintf(codefile,"%s = %s\n",x,y)
#define newlbss(l,x)	fprintf(codefile,".comm %s,%ld\n",l,x);

#define	pop_fmt		"(sp)+"
#define cst_fmt		"%ld"
#define	off_fmt		"%ld"
#define ilb_fmt		"I%x_%x"
#define dlb_fmt		"_%d"
#define	hol_fmt		"hol%d"

#ifdef TBL68020
#define loc_off		"(%d,a6)"
#define arg_off		"(8+%d,a6)"
#else
#define loc_off		"%d(a6)"
#define arg_off		"8+%d(a6)"
#endif
#define hol_off		"%ld+hol%d"

#define con_cst(x)	fprintf(codefile,".data4\t%ld\n",x)
#define con_ilb(x)	fprintf(codefile,".data4\t%s\n",x)
#define con_dlb(x)	fprintf(codefile,".data4\t%s\n",x)

#define modhead		".sect .text\n.sect .rom\n.sect .data\n.sect .bss\n"

#define fmt_id(sf,st)	sprintf(st,"_%s",sf)

#define BSS_INIT	0
