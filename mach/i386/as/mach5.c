/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
#define RCSID5 "$Header$"

/*
 * INTEL 80386 special routines
 */

ea_1(param) {
	if (is_expr(reg_1)) {
		serror("bad operand");
		return;
	}
	if (is_reg(reg_1)) {
		emit1(0300 | param | (reg_1&07));
		return;
	}
	if (rm_1 == 04) {
		/* sib field use here */
		emit1(mod_1 << 6 | param | 04);
		emit1(sib_1 | reg_1);
		if ((mod_1 == 0 && reg_1 == 5) || mod_1 == 2) {
#ifdef RELOCATION
			RELOMOVE(relonami, rel_1);
			newrelo(exp_1.typ, RELO4);
#endif
			emit4((long)(exp_1.val));
		}
		else if (mod_1 == 1) emit1((int)(exp_1.val));
		return;
	}
	emit1(mod_1<<6 | param | (reg_1&07));
	if ((mod_1 == 0 && reg_1 == 5) || mod_1 == 2) {
#ifdef RELOCATION
		RELOMOVE(relonami, rel_1);
		newrelo(exp_1.typ, RELO4);
#endif
		emit4((long)(exp_1.val));
	}
	else if (mod_1 == 1) {
		emit1((int)(exp_1.val));
	}
}

ea_2(param) {

	op_1 = op_2;
	RELOMOVE(rel_1, rel_2);
	ea_1(param);
}

int
checkscale(val)
	valu_t val;
{
	int v = val;

	if (v != val) v = 0;
	switch(v) {
	case 1:
		return 0;
	case 2:
		return 1 << 6;
	case 4:
		return 2 << 6;
	case 8:
		return 3 << 6;
	default:
		serror("bad scale");
		return 0;
	}
	/*NOTREACHED*/
}

reverse() {
	struct operand op;
#ifdef RELOCATION
	int r = rel_1;

	rel_1 = rel_2; rel_2 = r;
#endif
	op = op_1; op_1 = op_2; op_2 = op;
}

badsyntax() {

	serror("bad operands");
}

regsize(sz)
	int sz;
{
	register int bit;

	bit = (sz&1) ? 0 : IS_R8;
	if ((is_reg(reg_1) && (reg_1 & IS_R8) != bit) ||
	    (is_reg(reg_2) && (reg_2 & IS_R8) != bit)) 
		serror("register error");
}

indexed() {
	mod_2 = 0;
	if (sib_2 == -1)
		serror("register error");
	if (rm_2 == 0 && reg_2 == 4) {
		/* base register sp, no index register; use
		   indexed mode without index register
		*/
		rm_2 = 04;
		sib_2 = 044;
	}
	if (reg_2 == 015) {
		reg_2 = 05;
		return;
	}
	if (exp_2.typ != S_ABS || fitb(exp_2.val) == 0)
		mod_2 = 02;
	else if (exp_2.val != 0 || reg_2 == 5)
		mod_2 = 01;
}

ebranch(opc,exp)
	register int opc;
	expr_t exp;
{
	/*	Conditional branching; Full displacements are available
		on the 80386, so the welknown trick with the reverse branch
		over a jump is not needed here.
		The only complication here is with the address size, which
		can be set with a prefix. In this case, the user gets what
		he asked for.
	*/
	register int sm;
	register long dist;
	int saving = address_long ? 4 : 2;

	if (opc == 0353) saving--;
	dist = exp.val - (DOTVAL + 2);
	if (pass == PASS_2 && dist > 0 && !(exp.typ & S_DOT))
		dist -= DOTGAIN;
	sm = dist > 0 ? fitb(dist-saving) : fitb(dist);
	if ((exp.typ & ~S_DOT) != DOTTYP)
		sm = 0;
	if ((sm = small(sm,saving)) == 0) {
		if (opc == 0353) {
			emit1(0xe9);
		}
		else {
			emit1(0xF);
			emit1(opc | 0x80);
		}
		dist -= saving;
		exp.val = dist;
		adsize_exp(exp, RELPC);
	}
	else {
		if (opc == 0353) {
			emit1(opc);
		}
		else {
			emit1(opc | 0x70);
		}
		emit1((int)dist);
	}
}

branch(opc,exp)
	register int opc;
	expr_t exp;
{
	/*	LOOP, JCXZ, etc. branch instructions.
		Here, the offset just must fit in a byte.
	*/
	register long dist;

	dist = exp.val - (DOTVAL + 2);
	if (pass == PASS_2 && dist > 0 && !(exp.typ & S_DOT))
		dist -= DOTGAIN;
	fit((exp.typ & ~S_DOT) == DOTTYP && fitb(dist));
	emit1(opc);
	emit1((int)dist);
}

pushop(opc)
	register int opc;
{

	regsize(1);
	if (is_segreg(reg_1)) {
		/* segment register */
		if ((reg_1 & 07) <= 3)
			emit1(6 | opc | (reg_1&7)<<3);
		else {
			emit1(0xF);
			emit1(0200 | opc | ((reg_1&7)<<3));
		}
	} else if (is_reg(reg_1)) {
		/* normal register */
		emit1(0120 | opc<<3 | (reg_1&7));
	} else if (opc == 0) {
		if (is_expr(reg_1)) {
			if (exp_1.typ == S_ABS && fitb(exp_1.val)) {
				emit1(0152);
				emit1((int)(exp_1.val));
			}
			else {
				emit1(0150);
				RELOMOVE(relonami, rel_1);
				opsize_exp(exp_1, 1);
			}
		}
		else {
			emit1(0377); ea_1(6<<3);
		}
	} else {
		emit1(0217); ea_1(0<<3);
	}
}

opsize_exp(exp, nobyte)
	expr_t exp;
{
	if (! nobyte) {
#ifdef RELOCATION
		newrelo(exp.typ, RELO1);
#endif
		emit1((int)(exp.val));
	}
	else if (operand_long) {
#ifdef RELOCATION
		newrelo(exp.typ, RELO4);
#endif
		emit4((long)(exp.val));
	}
	else {
#ifdef RELOCATION
		newrelo(exp.typ, RELO2);
#endif
		emit2((int)(exp.val));
	}
}

adsize_exp(exp, relpc)
	expr_t exp;
{
	if (address_long) {
#ifdef RELOCATION
		newrelo(exp.typ, RELO4 | relpc);
#endif
		emit4((long)(exp.val));
	}
	else {
		if (! fitw(exp.val)) {
			warning("offset does not fit in 2 bytes; remove prefix");
		}
#ifdef RELOCATION
		newrelo(exp.typ, RELO2 | relpc);
#endif
		emit2((int)(exp.val));
	}
}

addop(opc) 
	register int opc;
{

	regsize(opc);
	if (is_reg(reg_2)) {
		/*	Add register to register or memory */
		emit1(opc); ea_1((reg_2&7)<<3);
	} else if (is_acc(reg_1) && is_expr(reg_2)) {
		/*	Add immediate to accumulator */
		emit1(opc | 4);
		RELOMOVE(relonami, rel_2);
		opsize_exp(exp_2, (opc&1));
	} else if (is_expr(reg_2)) {
		/*	Add immediate to register or memory */
		if ((opc&1) == 0) {
			emit1(0200);
		} else if (exp_2.typ != S_ABS || fitb(exp_2.val) == 0) {
			emit1(0201);
		} else {
			emit1(0203); opc &= ~1;
		}
		ea_1(opc & 070);
		RELOMOVE(relonami, rel_2);
		opsize_exp(exp_2, (opc&1));
	} else if (is_reg(reg_1)) {
		/*	Add register or memory to register */
		emit1(opc | 2);
		ea_2((reg_1&7)<<3);
	} else
		badsyntax();
}

rolop(opc)
	register int opc;
{
	register int oreg;

	oreg = reg_2;
	reg_2 = reg_1;
	regsize(opc);
	if (oreg == (IS_R8 | 1)) {
		/* cl register */
		emit1(0322 | (opc&1)); ea_1(opc&070);
	} else if (is_expr(oreg) && exp_2.typ == S_ABS && exp_2.val == 1) {
		/* shift by 1 */
		emit1(0320 | (opc&1)); ea_1(opc&070);
	} else if (is_expr(oreg)) {
		/* shift by byte count */
		emit1(0300 | (opc & 1)); 
		ea_1(opc & 070);
#ifdef RELOCATION
		RELOMOVE(relonami, rel_2);
		newrelo(exp_2.typ, RELO1);
#endif
		emit1((int)(exp_2.val));
	}
	else
		badsyntax();
}

incop(opc)
	register int opc;
{

	regsize(opc);
	if ((opc&1) && is_reg(reg_1)) {
		/* word register */
		emit1(0100 | (opc&010) | (reg_1&7));
	} else {
		emit1(0376 | (opc&1));
		ea_1(opc & 010);
	}
}

callop(opc) 
	register int opc;
{

	regsize(1);
	if (is_expr(reg_1)) {
		if (opc == (040+(0351<<8))) {
			RELOMOVE(relonami, rel_1);
			ebranch(0353,exp_1);
		} else {
			exp_1.val -= (DOTVAL+3 + (address_long ? 2 : 0));
			emit1(opc>>8);
			RELOMOVE(relonami, rel_1);
			adsize_exp(exp_1, RELPC);
		}
	} else {
		emit1(0377); ea_1(opc&070);
	}
}

xchg(opc)
	register int opc;
{

	regsize(opc);
	if (! is_reg(reg_1) || is_acc(reg_2)) {
		reverse();
	}
	if (opc == 1 && is_acc(reg_1) && is_reg(reg_2)) {
		emit1(0220 | (reg_2&7));
	} else if (is_reg(reg_1)) {
		emit1(0206 | opc); ea_2((reg_1&7)<<3);
	} else
		badsyntax();
}

test(opc)
	register int opc;
{

	regsize(opc);
	if (is_reg(reg_2) || is_expr(reg_1))
		reverse();
	if (is_expr(reg_2)) {
		if (is_acc(reg_1)) {
			emit1(0250 | opc);
			RELOMOVE(relonami, rel_2);
			opsize_exp(exp_2, (opc&1));
		}
		else {
			emit1(0366 | opc);
			ea_1(0<<3);
			RELOMOVE(relonami, rel_2);
			opsize_exp(exp_2, (opc&1));
		}
	} else if (is_reg(reg_1)) {
		emit1(0204 | opc); ea_2((reg_1&7)<<3);
	} else
		badsyntax();
}

mov(opc)
	register int opc;
{

	regsize(opc);
	if (is_segreg(reg_1)) {
		/* to segment register */
		emit1(0216); ea_2((reg_1&3)<<3);
	} else if (is_segreg(reg_2)) {
		/* from segment register */
		emit1(0214); ea_1((reg_2&3)<<3);
	} else if (is_expr(reg_2)) {
		/* from immediate */
		if (is_reg(reg_1)) {
			/* to register */
			emit1(0260 | opc<<3 | (reg_1&7)); 
		} else {
			/* to memory */
			emit1(0306 | opc); ea_1(0<<3); 
		}
		RELOMOVE(relonami, rel_2);
		opsize_exp(exp_2, (opc&1));
	} else if (rm_1 == 05 && is_acc(reg_2)) {
		/* from accumulator to memory  (displacement) */
		emit1(0242 | opc);
		RELOMOVE(relonami, rel_1);
		adsize_exp(exp_1, 0);
	} else if (rm_2 == 05 && is_acc(reg_1)) {
		/* from memory (displacement) to accumulator */
		emit1(0240 | opc);
		RELOMOVE(relonami, rel_2);
		adsize_exp(exp_2, 0);
	} else if (is_reg(reg_2)) {
		/* from register to memory or register */
		emit1(0210 | opc); ea_1((reg_2&7)<<3);
	} else if (is_reg(reg_1)) {
		/* from memory or register to register */
		emit1(0212 | opc); ea_2((reg_1&7)<<3);
	} else
		badsyntax();
}

extshft(opc, reg)
	int opc;
{
	int oreg2 = reg_2;

	reg_2 = reg_1;
	regsize(1);

	emit1(0xF);
	if (oreg2 == (IS_R8 | 1)) {
		/* cl register */
		emit1(opc|1);
		ea_1(reg << 3);
	}
	else if (is_expr(oreg2)) {
		emit1(opc);
		ea_1(reg << 3);
#ifdef RELOCATION
		RELOMOVE(relonami, rel_2);
		newrelo(exp_2.typ, RELO1);
#endif
		emit1((int)(exp_2.val));
	}
	else	badsyntax();
}

bittestop(opc)
	int opc;
{
	regsize(1);
	emit1(0xF);
	if (is_expr(reg_2)) {
		emit1(0272);
		ea_1(opc << 3);
#ifdef RELOCATION
		RELOMOVE(relonami, rel_2);
		newrelo(exp_2.typ, RELO1);
#endif
		emit1((int)(exp_2.val));
	}
	else if (is_reg(reg_2)) {
		emit1(0203 | opc);
		ea_1((reg_2&7)<<3);
	}
	else	badsyntax();
}

imul(reg)
	int reg;
{
	/*	This instruction is more elaborate on the 80386. Its most
		general form is:
			imul reg, reg_or_mem, immediate.
		This is the form processed here.
	*/
	regsize(1);
	if (is_expr(reg_1)) {
		/* To also handle
			imul reg, immediate, reg_or_mem
		*/
		reverse();
	}
	if (is_expr(reg_2)) {
		/* The immediate form; two cases: */
		if (exp_2.typ == S_ABS && fitb(exp_2.val)) {
			/* case 1: 1 byte encoding of immediate */
			emit1(0153);
			ea_1(reg << 3);
			emit1((int)(exp_2.val));
		}
		else {
			/* case 2: WORD or DWORD encoding of immediate */
			emit1(0151);
			ea_1(reg << 3);
			RELOMOVE(relonami, rel_2);
			opsize_exp(exp_2, 1);
		}
	}
	else if (is_reg(reg_1) && ((reg_1&7) == reg)) {
		/* the "reg" field and the "reg_or_mem" field are the same,
		   and the 3rd operand is not an immediate ...
		*/
		if (reg == 0) {
			/* how lucky we are, the target is the ax register */
			emit1(0367);
			ea_2(050);
		}
		else {
			/* another register ... */
			emit1(0xF);
			emit1(0257);
			ea_2(reg << 3);
		}
	}
	else badsyntax();
}
