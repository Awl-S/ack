/*
 * (c) copyright 1990 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
#define RCSID4 "$Header$"

/*
* VAX-11 machine dependent yacc syntax rules
*/

/* _b, _w, and _l are ordinary READ/MODIFY/WRITE operands, the letter indicates
	the size,
   A means effective ADDRESS (must be memory),
   B means branch displacement,
   V means effective address or register;
   Here, no difference is made between Modify and Write.
*/

operation
	:
				{ op_ind = 0; }
		oper
				{ if ((unsigned) $2 < 0x100) {
					emit1((int)$2);
				  }
				  else {
					emit1((int)$2&0xff);
					emit1((int)$2>>8);
				  }
				  operands(op_ind);
				}
	|	OP1_Bx expr	{ branch($1, $2); }
	|	OP1_Be expr	{ op_ind = 0; ext_branch($1, $2); }
	|	OP2_l_Be 	{ op_ind = 0; opnd[0].size = 4; }
		opnd ',' expr
				{ ext_branch($1, $5); }
	|	OP3_l_V_Be	{ op_ind = 0; opnd[0].size = 4;
				  opnd[1].size = -1;
				}
		opnd ',' opnd ',' expr
				{ ext_branch($1, $7); }
	;

OP1_O
	:	OP1_b		{ opnd[0].size = 1; $$ = $1; }
	|	OP1_w		{ opnd[0].size = 2; $$ = $1; }
	|	OP1_l		{ opnd[0].size = 4; $$ = $1; }
	|	OP1_u
	;

OP1_B
	:	OP1_Bb		{ opnd[0].size = 1; $$ = $1; }
	|	OP1_Bw		{ opnd[0].size = 2; $$ = $1; }
	|	OP1_Bl		{ opnd[0].size = 4; $$ = $1; }
	;

OP2_O_O
	:	OP2_b_b		{ opnd[0].size = 1; opnd[1].size = 1; $$ = $1; }
	|	OP2_b_l		{ opnd[0].size = 1; opnd[1].size = 4; $$ = $1; }
	|	OP2_b_u		{ opnd[0].size = 1; opnd[1].size = 0; $$ = $1; }
	|	OP2_b_w		{ opnd[0].size = 1; opnd[1].size = 2; $$ = $1; }
	|	OP2_l_b		{ opnd[0].size = 4; opnd[1].size = 1; $$ = $1; }
	|	OP2_l_l		{ opnd[0].size = 4; opnd[1].size = 4; $$ = $1; }
	|	OP2_l_u		{ opnd[0].size = 4; opnd[1].size = 0; $$ = $1; }
	|	OP2_l_w		{ opnd[0].size = 4; opnd[1].size = 2; $$ = $1; }
	|	OP2_u_b		{ opnd[0].size = 0; opnd[1].size = 1; $$ = $1; }
	|	OP2_u_l		{ opnd[0].size = 0; opnd[1].size = 4; $$ = $1; }
	|	OP2_u_u		{ opnd[0].size = 0; opnd[1].size = 0; $$ = $1; }
	|	OP2_u_w		{ opnd[0].size = 0; opnd[1].size = 2; $$ = $1; }
	|	OP2_w_b		{ opnd[0].size = 2; opnd[1].size = 1; $$ = $1; }
	|	OP2_w_l		{ opnd[0].size = 2; opnd[1].size = 4; $$ = $1; }
	|	OP2_w_u		{ opnd[0].size = 2; opnd[1].size = 0; $$ = $1; }
	|	OP2_w_w		{ opnd[0].size = 2; opnd[1].size = 2; $$ = $1; }
	;

OP2_A_O
	:	OP2_A_l		{ opnd[1].size = 4; $$ = $1; }
	;

OP2_O_B
	:	OP2_l_Bb	{ opnd[0].size = 4; opnd[1].size = 1; $$ = $1; }
	;

OP2_O_A
	:	OP2_l_A		{ opnd[0].size = 4; $$ = $1; }
	;

OP3_O_O_O
	:	OP3_b_b_b	{ opnd[0].size = 1; opnd[1].size = 1;
				  opnd[2].size = 1; $$ = $1;
				}
	|	OP3_b_l_l	{ opnd[0].size = 1; opnd[1].size = 4;
				  opnd[2].size = 4; $$ = $1;
				}
	|	OP3_b_u_u	{ opnd[0].size = 1; opnd[1].size = 0;
				  opnd[2].size = 0; $$ = $1;
				}
	|	OP3_l_l_l	{ opnd[0].size = 4; opnd[1].size = 4;
				  opnd[2].size = 4; $$ = $1;
				}
	|	OP3_u_u_u	{ opnd[0].size = 0; opnd[1].size = 0;
				  opnd[2].size = 0; $$ = $1;
				}
	|	OP3_w_w_w	{ opnd[0].size = 2; opnd[1].size = 2;
				  opnd[2].size = 2; $$ = $1;
				}
	;

OP3_O_O_B
	:	OP3_l_l_Bb	{ opnd[0].size = 4; opnd[1].size = 4;
				  opnd[2].size = 1; $$ = $1;
				}
	|	OP3_l_V_Bb	{ opnd[0].size = 4; opnd[1].size = -1;
				  opnd[2].size = 1; $$ = $1;
				}
	;

OP3_O_O_A
	:	OP3_b_w_A	{ opnd[0].size = 1; opnd[1].size = 2; $$ = $1; }
	|	OP3_l_w_A	{ opnd[0].size = 4; opnd[1].size = 2; $$ = $1; }
	|	OP3_u_w_A	{ opnd[0].size = 0; opnd[1].size = 2; $$ = $1; }
	;

OP3_O_A_A
	:	OP3_w_A_A	{ opnd[0].size = 2; $$ = $1; }
	;

OP3_O_A_O
	:	OP3_w_A_l	{ opnd[0].size = 2; opnd[2].size = 4; $$ = $1; }
	;

OP4_O_O_O_O
	:	OP4_l_b_V_l	{ opnd[0].size = 4; opnd[1].size = 1;
				  opnd[2].size = -1; opnd[3].size = 4;
				  $$ = $1;
				}
	|	OP4_l_l_b_V	{ opnd[0].size = 4; opnd[1].size = 4;
				  opnd[2].size = 1; opnd[3].size = -1;
				  $$ = $1;
				}
	|	OP4_l_l_l_u	{ opnd[0].size = 4; opnd[1].size = 4;
				  opnd[2].size = 4; opnd[3].size = 0;
				  $$ = $1;
				}
	|	OP4_l_u_l_l	{ opnd[0].size = 4; opnd[1].size = 0;
				  opnd[2].size = 4; opnd[3].size = 4;
				  $$ = $1;
				}
	;	

OP4_O_O_O_B
	:	OP4_b_b_b_Bw	{ opnd[0].size = 1; opnd[1].size = 1;
				  opnd[2].size = 1; opnd[3].size = 2;
				  $$ = $1;
				}
	|	OP4_l_l_l_Bw	{ opnd[0].size = 4; opnd[1].size = 4;
				  opnd[2].size = 4; opnd[3].size = 2;
				  $$ = $1;
				}
	|	OP4_u_u_u_Bw	{ opnd[0].size = 0; opnd[1].size = 0;
				  opnd[2].size = 0; opnd[3].size = 2;
				  $$ = $1;
				}
	|	OP4_w_w_w_Bw	{ opnd[0].size = 2; opnd[1].size = 2;
				  opnd[2].size = 2; opnd[3].size = 2;
				  $$ = $1;
				}
	;

OP4_O_A_O_A
	:	OP4_w_A_w_A	{ opnd[0].size = 2; opnd[2].size = 2; $$ = $1; }
	;

OP4_O_A_A_O
	:	OP4_w_A_A_b	{ opnd[0].size = 2; opnd[3].size = 1; $$ = $1; }
	;

OP4_O_A_A_A
	:	OP4_w_A_A_A	{ opnd[0].size = 2; $$ = $1; }
	;

OP4_A_O_O_A
	:	OP4_A_l_w_A	{ opnd[1].size = 4; opnd[2].size = 2; $$ = $1; }
	;

OP5_O_A_A_O_A
	:	OP5_w_A_A_w_A	{ opnd[0].size = 2; opnd[3].size = 2; $$ = $1; }
	;

OP5_O_A_O_O_A
	:	OP5_w_A_b_w_A	{ opnd[0].size = 2; opnd[3].size = 2;
				  opnd[2].size = 1; $$ = $1;
				}
	;

OP5_O_O_O_O_O
	:	OP5_u_b_u_l_u	{ opnd[0].size = 0; opnd[1].size = 1;
				  opnd[2].size = 0; opnd[3].size = 4;
				  opnd[4].size = 0; $$ = $1;
				}
	|	OP5_u_w_u_l_u	{ opnd[0].size = 0; opnd[1].size = 2;
				  opnd[2].size = 0; opnd[3].size = 4;
				  opnd[4].size = 0; $$ = $1;
				}
	;

OP6_O_O_O_O_O_O
	:	OP6_l_l_l_l_l_l	{ opnd[0].size = 4; opnd[1].size = 4;
				  opnd[2].size = 4; opnd[3].size = 4;
				  opnd[4].size = 4; opnd[5].size = 4;
				  $$ = $1;
				}
	;

OP6_O_A_O_A_O_A
	:	OP6_w_A_b_A_w_A	{ opnd[0].size = 2; opnd[2].size = 1;
				  opnd[4].size = 2; $$ = $1;
				}
	|	OP6_w_A_w_A_w_A	{ opnd[0].size = 2; opnd[2].size = 2;
				  opnd[4].size = 2; $$ = $1;
				}
	;

OP6_O_O_A_O_O_A
	:	OP6_b_w_A_b_w_A	{ opnd[0].size = 1; opnd[1].size = 2;
				  opnd[3].size = 1; opnd[4].size = 2;
				  $$ = $1;
				}
	;

CASE_O_O_O
	:	CASE_b_b_b	{ opnd[0].size = 1; opnd[1].size = 1;
				  opnd[2].size = 1; $$ = $1;
				}
	|	CASE_w_w_w	{ opnd[0].size = 2; opnd[1].size = 2;
				  opnd[2].size = 2; $$ = $1;
				}
	|	CASE_l_l_l	{ opnd[0].size = 4; opnd[1].size = 4;
				  opnd[2].size = 4; $$ = $1;
				}
	;

oper
	:	OP0
	|	OP1_O opnd	{ $$ = $1; }
	|	OP1_A ea	{ $$ = $1; }
	|	OP1_B expr	{ $$ = $1;
				  opnd[0].exp = $2;
				  RELOMOVE(opnd[0].relo, relonami);
				  opnd[0].mode = DISPL;
				  op_ind = 1;
				}
	|	OP2_O_O opnd ',' opnd	
				{ $$ = $1; }
	|	OP2_A_O ea ',' opnd
				{ $$ = $1; }
	|	OP2_O_B opnd ',' expr
				{ $$ = $1;
				  opnd[op_ind].exp = $4;
				  RELOMOVE(opnd[op_ind].relo, relonami);
				  opnd[op_ind].mode = DISPL;
				  op_ind++;
				}
	|	OP2_A_A ea ',' ea
				{ $$ = $1; }
	|	OP2_O_A opnd ',' ea
				{ $$ = $1; }
	|	OP3_O_O_O opnd ',' opnd ',' opnd
				{ $$ = $1; }
	|	OP3_O_O_B opnd ',' opnd ',' expr
				{ $$ = $1;
				  opnd[op_ind].exp = $6;
				  RELOMOVE(opnd[op_ind].relo, relonami);
				  opnd[op_ind].mode = DISPL;
				  op_ind++;
				}
	|	OP3_O_O_A opnd ',' opnd ',' ea
				{ $$ = $1; }
	|	OP3_O_A_A opnd ',' ea ',' ea
				{ $$ = $1; }
	|	OP3_O_A_O opnd ',' ea ',' opnd
				{ $$ = $1; }
	|	OP4_O_O_O_O opnd ',' opnd ',' opnd ',' opnd
				{ $$ = $1; }
	|	OP4_O_O_O_B opnd ',' opnd ',' opnd ',' expr
				{ $$ = $1;
				  opnd[op_ind].exp = $8;
				  RELOMOVE(opnd[op_ind].relo, relonami);
				  opnd[op_ind].mode = DISPL;
				  op_ind++;
				}
	|	OP4_O_A_O_A opnd ',' ea ',' opnd ',' ea
				{ $$ = $1; }
	|	OP4_O_A_A_O opnd ',' ea ',' ea ',' opnd
				{ $$ = $1; }
	|	OP4_A_O_O_A ea ',' opnd ',' opnd ',' ea
				{ $$ = $1; }
	|	OP4_O_A_A_A opnd ',' ea ',' ea ',' ea
				{ $$ = $1; }
	|	OP5_O_A_A_O_A opnd ',' ea ',' ea ',' opnd ',' ea
				{ $$ = $1; }
	|	OP5_O_A_O_O_A opnd ',' ea ',' opnd ',' opnd ',' ea
				{ $$ = $1; }
	|	OP5_O_O_O_O_O opnd ',' opnd ',' opnd ',' opnd ',' opnd
				{ $$ = $1; }
	|	OP6_O_O_O_O_O_O opnd ',' opnd ',' opnd ',' opnd ',' opnd ',' opnd
				{ $$ = $1; }
	|	OP6_O_A_O_A_O_A opnd ',' ea ',' opnd ',' ea ',' opnd ',' ea
				{ $$ = $1; }
	|	OP6_O_O_A_O_O_A opnd ',' opnd ',' ea ',' opnd ',' opnd ',' ea
				{ $$ = $1; }
	|	CASE_O_O_O opnd ',' opnd ',' opnd
				{ $$ = $1; }
	;

opnd
	:	ea
	|	immediate
	|	REG		{ opnd[op_ind].mode = REG_MODE;
				  opnd[op_ind].reg = $1;
				  opnd[op_ind].index_reg = -1;
				  op_ind++;
				}
	;

ea
	:	eax		{ opnd[op_ind].index_reg = -1;
				  op_ind++;
				}
	|	eax '[' REG ']'	{ opnd[op_ind].index_reg = $3;
				  op_ind++;
				}
	|	immediate '[' REG ']'
				{ opnd[op_ind-1].index_reg = $3;
				}
	;
eax
	:	expr		{ opnd[op_ind].exp = $1;
				  opnd[op_ind].mode = ABS;
				  RELOMOVE(opnd[op_ind].relo, relonami);
				}
	|	'*' expr	{ opnd[op_ind].exp = $2;
				  opnd[op_ind].mode = ABS_DEF;
				  RELOMOVE(opnd[op_ind].relo, relonami);
				}
	|	'(' REG ')'	{ opnd[op_ind].mode = REGDEF_MODE;
				  opnd[op_ind].reg = $2;
				}
	|	'(' REG ')' '+'	{ opnd[op_ind].mode = AI_MODE;
				  opnd[op_ind].reg = $2;
				}
	|	'*' '(' REG ')' '+'
				{ opnd[op_ind].mode = AI_DEF_MODE;
				  opnd[op_ind].reg = $3;
				}
	|	'-' '(' REG ')'	{ opnd[op_ind].mode = AD_MODE;
				  opnd[op_ind].reg = $3;
				}
	|	expr '(' REG ')'
				{ opnd[op_ind].exp = $1;
				  opnd[op_ind].mode = DISPLL_MODE;
				  opnd[op_ind].reg = $3;
				  RELOMOVE(opnd[op_ind].relo, relonami);
				}
	|	'*' expr '(' REG ')'
				{ opnd[op_ind].exp = $2;
				  opnd[op_ind].mode = DISPLL_DEF_MODE;
				  opnd[op_ind].reg = $4;
				  RELOMOVE(opnd[op_ind].relo, relonami);
				}
	;

immediate
	:	'$' expr	{ opnd[op_ind].mode = IMM;
				  opnd[op_ind].exp = $2;
				  opnd[op_ind].index_reg = -1;
				  RELOMOVE(opnd[op_ind].relo, relonami);
				  op_ind++;
				}
	;
