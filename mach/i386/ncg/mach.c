/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 *
 */

#ifndef NORCSID
static char rcs_m[]= "$Header$" ;
static char rcs_mh[]= ID_MH ;
#endif

/*
 * machine dependent back end routines for the Intel 80386
 */

con_part(sz,w) register sz; word w; {

	while (part_size % sz)
		part_size++;
	if (part_size == TEM_WSIZE)
		part_flush();
	if (sz == 1 || sz == 2) {
		w &= (sz == 1 ? 0xFF : 0xFFFF);
		w <<= 8 * part_size;
		part_word |= w;
	} else {
		assert(sz == 4);
		part_word = w;
	}
	part_size += sz;
}

con_mult(sz) word sz; {
	long l;

	if (sz != 4)
		fatal("bad icon/ucon size");
	l = atol(str);
	fprintf(codefile,"\t.data4 %ld\n", l);
}

#define CODE_GENERATOR  
#define IEEEFLOAT  
#define FL_MSL_AT_LOW_ADDRESS	0
#define FL_MSW_AT_LOW_ADDRESS	0
#define FL_MSB_AT_LOW_ADDRESS	0
#include <con_float>

/*

string holstr(n) word n; {

	sprintf(str,hol_off,n,holno);
	return(mystrcpy(str));
}
*/

prolog(nlocals) full nlocals; {

	fputs("push ebp\nmov ebp,esp\n", codefile);
	switch(nlocals) {
	case 8:
		fputs("push eax\n", codefile);
		/* fall through */
	case 4:
		fputs("push eax\n", codefile);
		/* fall through */
	case 0:
		break;
	default:
		fprintf(codefile, "\tsub\tesp,%ld\n",nlocals);
		break;
	}
}

#ifdef REGVARS
long si_off;
long di_off;
int firstreg;

regscore(off, size, typ, score, totyp)
	long off;
{
	if (size != 4) return -1;
	score -= 1;
	score += score;
	if (typ == reg_pointer || typ == reg_loop) score += (score >> 2);
	score -= 2;	/* cost of saving */
	if (off >= 0) score -= 3;
	return score;
}

i_regsave()
{
	si_off = -1;
	di_off = -1;
	firstreg = 0;
}

f_regsave()
{
}

regsave(regstr, off, size)
	char *regstr;
	long off;
{
	if (strcmp(regstr, "esi") == 0) {
		if (! firstreg) firstreg = -1;
		si_off = off;
		fputs("push esi\n", codefile);
		if (off >= 0)
			fprintf(codefile, "mov esi,%ld(ebp)\n", off);
	}
	else {
		if (! firstreg) firstreg = 1;
		di_off = off;
		fputs("push edi\n", codefile);
		if (off >= 0)
			fprintf(codefile, "mov edi,%ld(ebp)\n", off);
	}
}

regreturn()
{
	if (firstreg == 1) {
		if (si_off != -1) fputs("pop esi\n", codefile);
		fputs("pop edi\n", codefile);
	}
	else if (firstreg == -1) {
		if (di_off != -1) fputs("pop edi\n", codefile);
		fputs("pop esi\n", codefile);
	}
	fputs("leave\nret\n", codefile);
}
#endif REGVARS

mes(type) word type ; {
	int argt ;

	switch ( (int)type ) {
	case ms_ext :
		for (;;) {
			switch ( argt=getarg(
			    ptyp(sp_cend)|ptyp(sp_pnam)|sym_ptyp) ) {
			case sp_cend :
				return ;
			default:
				strarg(argt) ;
				fprintf(codefile, ".define %s\n",argstr) ;
				break ;
			}
		}
	default :
		while ( getarg(any_ptyp) != sp_cend ) ;
		break ;
	}
}

char    *segname[] = {
	".sect .text",        /* SEGTXT */
	".sect .data",        /* SEGCON */
	".sect .rom",        /* SEGROM */
	".sect .bss"          /* SEGBSS */
};
