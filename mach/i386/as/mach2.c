/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
#define RCSID2 "$Header$"

/*
 * INTEL 80386 tokens
 */

%token <y_word> R32
%token <y_word> R16
%token <y_word> R8
%token <y_word> RSEG
%token <y_word> RSYSCR
%token <y_word> RSYSDR
%token <y_word> RSYSTR
%token <y_word> PREFIX
%token <y_word> ADDOP
%token <y_word>	BITTEST
%token <y_word>	BOUND
%token <y_word> CALFOP
%token <y_word> CALLOP
%token <y_word> ENTER
%token <y_word> EXTEND
%token <y_word> EXTOP
%token <y_word> EXTOP1
%token <y_word> IMUL
%token <y_word> IMULB
%token <y_word> INCOP
%token <y_word> INT
%token <y_word> IOOP
%token <y_word> JOP
%token <y_word> JOP2
%token <y_word> LEAOP
%token <y_word> LEAOP2
%token <y_word> LSHFT
%token <y_word> MOV
%token <y_word> NOOP_1
%token <y_word> NOOP_2
%token <y_word> NOTOP
%token <y_word> PUSHOP
%token <y_word> RET
%token <y_word> ROLOP
%token <y_word> SETCC
%token <y_word> TEST
%token <y_word> XCHG
