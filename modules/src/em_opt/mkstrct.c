#ifndef NORCSID
static char rcsid3[] = "$Header$";
#endif

#include "nopt.h"

EM_mkop(p,opcode)
	register p_instr p;
	int opcode;
{
	p->em_type = EM_MNEM;
	p->em_opcode = opcode;
	p->em_argtype = 0;
}

EM_mknarg(p,opcode)
	register p_instr p;
	int opcode;
{
	p->em_type = EM_MNEM;
	p->em_opcode = opcode;
	p->em_argtype = 0;
	p->em_cst = 0;
}

EM_mkilb(p,opcode,lab)
	register p_instr p;
	int opcode;
	label lab;
{
	p->em_type = EM_MNEM;
	p->em_argtype = ilb_ptyp;
	p->em_opcode = opcode;
	p->em_ilb = lab;
}

EM_mknof(p,opcode,lab,off)
	register p_instr p;
	int opcode;
	label lab;
	arith off;
{
	p->em_type = EM_MNEM;
	p->em_argtype = nof_ptyp;
	p->em_opcode = opcode;
	p->em_dlb = lab;
	p->em_off = off;
}

EM_mksof(p,opcode,name,off)
	register p_instr p;
	int opcode;
	char *name;
	arith off;
{
	p->em_type = EM_MNEM;
	p->em_argtype = sof_ptyp;
	p->em_opcode = opcode;
        p->em_dnam = OO_freestr(name);
	p->em_off = off;
}

EM_mkcst(p,opcode,cst)
	register p_instr p;
	int opcode;
	arith cst;
{
	p->em_type = EM_MNEM;
	p->em_argtype = cst_ptyp;
	p->em_opcode = opcode;
	p->em_cst = cst;
}

EM_mkpro(p,opcode,pnam)
	register p_instr p;
	int opcode;
	char *pnam;
{
	p->em_type = EM_MNEM;
	p->em_argtype = pro_ptyp;
	p->em_opcode = opcode;
	p->em_pnam = OO_freestr(pnam);
}

EM_mkdefilb(p,opcode,deflb)
	register p_instr p;
	int opcode;
	label deflb;
{
	p->em_type = EM_DEFILB;
	p->em_opcode = opcode;
	p->em_argtype = 0;
	p->em_ilb = deflb;
}

