/* $Header$ */

/* Language dependant support; this one is for C */

#include <stdio.h>
#include <alloc.h>

#include "position.h"
#include "class.h"
#include "langdep.h"
#include "Lpars.h"
#include "idf.h"
#include "token.h"
#include "expr.h"
#include "tree.h"
#include "operator.h"

extern FILE *db_out, *db_in;

extern int
	get_name();

extern double
	atof();

static int
	print_string(),
	print_char(),
	get_number(),
	get_string(),
	get_token(),
	print_op(),
	unop_prio(),
	binop_prio(),
	fix_bin_to_pref();

static long
	array_elsize();

static struct langdep c = {
	0,

	"%ld",
	"0%lo",
	"0x%lX",
	"%lu",
	"0x%lX",
	"%g",

	"{",
	"}",
	"{",
	"}",
	"{",
	"}",

	print_string,
	print_char,
	array_elsize,
	unop_prio,
	binop_prio,
	get_string,
	get_name,
	get_number,
	get_token,
	print_op,
	fix_bin_to_pref
};

struct langdep *c_dep = &c;

static int
print_char(c)
  int	c;
{
  fprintf(db_out, (c >= 040 && c < 0177) ? "'%c'" : "'\\0%o'", c);
}

static int
print_string(s, len)
  char	*s;
  int	len;
{
  register char	*str = s;
  int delim = '\'';

  while (*str) {
	if (*str++ == '\'') delim = '"';
  }
  fprintf(db_out, "%c%.*s%c", delim, len, s, delim);
}

extern long	int_size;

static long
array_elsize(size)
  long	size;
{
  if (! (int_size % size)) return size;
  if (! (size % int_size)) return size;
  return ((size + int_size - 1) / int_size) * int_size;
}

static int
unop_prio(op)
  int	op;
{
  switch(op) {
  case E_NOT:
  case E_BNOT:
  case E_MIN:
  case E_DEREF:
  case E_SELECT:
  case E_PLUS:
  case E_ADDR:
	return 12;
  }
  return 1;
}

static int
binop_prio(op)
  int	op;
{
  switch(op) {
  case E_OR:
	return 2;
  case E_AND:
	return 3;
  case E_BOR:
	return 4;
  case E_BXOR:
	return 5;
  case E_BAND:
	return 6;
  case E_EQUAL:
  case E_NOTEQUAL:
	return 7;
  case E_LT:
  case E_LTEQUAL:
  case E_GT:
  case E_GTEQUAL:
	return 8;
  case E_LSFT:
  case E_RSFT:
	return 9;
  case E_MIN:
  case E_PLUS:
	return 10;
  case E_MUL:
  case E_DIV:
  case E_ZDIV:
  case E_MOD:
  case E_ZMOD:
	return 11;
  }
  return 1;
}

static int
val_in_base(c, base)
  register int c;
{
  return is_dig(c) 
	? c - '0'
	: base != 16
	  ? -1
	  : is_hex(c)
	    ? (c - 'a' + 10) & 017
	    : -1;
}

static int
get_number(c)
  register int	c;
{
  char buf[512+1];
  register int base = 10;
  register char *p = &buf[0];
  register long val = 0;
  register int val_c;

  if (c == '0') {
	/* check if next char is an 'x' or an 'X' */
	c = getc(db_in);
	if (c == 'x' || c == 'X') {
		base = 16;
		c = getc(db_in);
	}
	else	base = 8;
  }
  while (val_c = val_in_base(c, base), val_c >= 0) {
	val = val * base + val_c;
	if (p - buf < 512) *p++ = c;
	c = getc(db_in);
  }
  if (base == 16 || !((c == '.' || c == 'e' || c == 'E'))) {
	ungetc(c, db_in);
	tok.ival = val;
	return INTEGER;
  }
  if (c == '.') {
	if (p - buf < 512) *p++ = c;
	c = getc(db_in);
  }
  while (is_dig(c)) {
	if (p - buf < 512) *p++ = c;
	c = getc(db_in);
  }
  if (c == 'e' || c == 'E') {
	if (p - buf < 512) *p++ = c;
	c = getc(db_in);
	if (c == '+' || c == '-') {
		if (p - buf < 512) *p++ = c;
		c = getc(db_in);
	}
	if (! is_dig(c)) {
		error("malformed floating constant");
	}
	while (is_dig(c)) {
		if (p - buf < 512) *p++ = c;
		c = getc(db_in);
	}
  }
  ungetc(c, db_in);
  *p++ = 0;
  if (p == &buf[512+1]) {
	error("floating point constant too long");
  }
  tok.fval = atof(buf);
  return REAL;
}

static int
get_token(c)
  register int	c;
{
  switch(c) {
  case '(':
  case ')':
  case '[':
  case ']':
  case '`':
  case ':':
  case ',':
	return c;

  case '.':
	tok.ival = E_SELECT;
	return SEL_OP;
  case '+':
	tok.ival = E_PLUS;
	return PREF_OR_BIN_OP;
  case '-':
	c = getc(db_in);
	if (c == '>') {
		tok.ival = E_DERSELECT;
		return BIN_OP;
	}
	ungetc(c, db_in);
	tok.ival = E_MIN;
	return PREF_OR_BIN_OP;
  case '*':
	tok.ival = E_MUL;
	return PREF_OR_BIN_OP;
  case '/':
	tok.ival = E_ZDIV;
	return BIN_OP;
  case '%':
	tok.ival = E_ZMOD;
	return BIN_OP;
  case '&':
	c = getc(db_in);
	if (c == '&') {
		tok.ival = E_AND;
		return BIN_OP;
	}
	ungetc(c, db_in);
	tok.ival = E_BAND;
	return PREF_OR_BIN_OP;
  case '^':
	tok.ival = E_BXOR;
	return BIN_OP;
  case '|':
	c = getc(db_in);
	if (c == '|') {
		tok.ival = E_OR;
	}
	else {
		ungetc(c, db_in);
		tok.ival = E_BOR;
	}
	return BIN_OP;
  case '=':
	c = getc(db_in);
	if (c == '=') {
	}
	else {
		ungetc(c, db_in);
		warning("== assumed");
	}
	tok.ival = E_EQUAL;
	return BIN_OP;
  case '<':
	c = getc(db_in);
	if (c == '=') {
		tok.ival = E_LTEQUAL;
		return BIN_OP;
	}
	if (c == '<') {
		tok.ival = E_LSFT;
		return BIN_OP;
	}
	ungetc(c, db_in);
	tok.ival = E_LT;
	return BIN_OP;
  case '>':
	c = getc(db_in);
	if (c == '=') {
		tok.ival = E_GTEQUAL;
		return BIN_OP;
	}
	if (c == '>') {
		tok.ival = E_RSFT;
		return BIN_OP;
	}
	ungetc(c, db_in);
	tok.ival = E_GT;
	return BIN_OP;
  case '!':
	c = getc(db_in);
	if (c == '=') {
		tok.ival = E_NOTEQUAL;
		return BIN_OP;
	}
	ungetc(c, db_in);
	tok.ival = E_NOT;
	return PREF_OP;
  case '~':
	tok.ival = E_BNOT;
	return PREF_OP;
  default:
	error("illegal character 0%o", c);
	return LLlex();
  }
}

static int
quoted(ch)
  int	ch;
{
  /*	quoted() replaces an escaped character sequence by the
	character meant.
  */
  /* first char after backslash already in ch */
  if (!is_oct(ch)) {		/* a quoted char */
	switch (ch) {
	case 'n':
		ch = '\n';
		break;
	case 't':
		ch = '\t';
		break;
	case 'b':
		ch = '\b';
		break;
	case 'r':
		ch = '\r';
		break;
	case 'f':
		ch = '\f';
		break;
	}
  }
  else {				/* a quoted octal */
	register int oct = 0, cnt = 0;

	do {
		oct = oct*8 + (ch-'0');
		ch = getc(db_in);
	} while (is_oct(ch) && ++cnt < 3);
	ungetc(ch, db_in);
	ch = oct;
  }
  return ch&0377;

}

static int 
get_string(c)
  int	c;
{
  register int ch;
  char buf[512];
  register int len = 0;

  while (ch = getc(db_in), ch != c) {
	if (ch == '\n') {
		error("newline in string");
		break;
	}
	if (ch == '\\') {
		ch = getc(db_in);
		ch = quoted(ch);
	}
	buf[len++] = ch;
  }
  buf[len++] = 0;
  tok.str = Salloc(buf, (unsigned) len);
  return STRING;
}

static int
print_op(p)
  p_tree	p;
{
  switch(p->t_oper) {
  case OP_UNOP:
  	switch(p->t_whichoper) {
	case E_MIN:
		fputs("-", db_out);
		print_node(p->t_args[0], 0);
		break;
	case E_PLUS:
		fputs("+", db_out);
		print_node(p->t_args[0], 0);
		break;
	case E_NOT:
		fputs("!", db_out);
		print_node(p->t_args[0], 0);
		break;
	case E_DEREF:
		fputs("*", db_out);
		print_node(p->t_args[0], 0);
		break;
	case E_BNOT:
		fputs("~", db_out);
		print_node(p->t_args[0], 0);
		break;
	case E_ADDR:
		fputs("&", db_out);
		print_node(p->t_args[0], 0);
		break;
	}
	break;
  case OP_BINOP:
	if (p->t_whichoper == E_ARRAY) {
		print_node(p->t_args[0], 0);
		fputs("[", db_out);
		print_node(p->t_args[1], 0);
		fputs("]", db_out);
		break;
	}
	if (p->t_whichoper == E_DERSELECT) {
		print_node(p->t_args[0], 0);
		fputs("->", db_out);
		print_node(p->t_args[1], 0);
		break;
	}
	if (p->t_whichoper == E_SELECT) {
		print_node(p->t_args[0], 0);
		fputs(".", db_out);
		print_node(p->t_args[1], 0);
		break;
	}
	fputs("(", db_out);
	print_node(p->t_args[0], 0);
	switch(p->t_whichoper) {
	case E_LSFT:
		fputs("<<", db_out);
		break;
	case E_RSFT:
		fputs(">>", db_out);
		break;
	case E_AND:
		fputs("&&", db_out);
		break;
	case E_BAND:
		fputs("&", db_out);
		break;
	case E_OR:
		fputs("||", db_out);
		break;
	case E_BOR:
		fputs("|", db_out);
		break;
	case E_BXOR:
		fputs("^", db_out);
		break;
	case E_ZDIV:
		fputs("/", db_out);
		break;
	case E_ZMOD:
		fputs("%", db_out);
		break;
	case E_PLUS:
		fputs("+", db_out);
		break;
	case E_MIN:
		fputs("-", db_out);
		break;
	case E_MUL:
		fputs("*", db_out);
		break;
	case E_EQUAL:
		fputs("==", db_out);
		break;
	case E_NOTEQUAL:
		fputs("!=", db_out);
		break;
	case E_LTEQUAL:
		fputs("<=", db_out);
		break;
	case E_GTEQUAL:
		fputs(">=", db_out);
		break;
	case E_LT:
		fputs("<", db_out);
		break;
	case E_GT:
		fputs(">", db_out);
		break;
	}
	print_node(p->t_args[1], 0);
	fputs(")", db_out);
	break;
  }
}

static int
fix_bin_to_pref(p)
  p_tree	p;
{
  switch(p->t_whichoper) {
  case E_MUL:
	p->t_whichoper = E_DEREF;
	break;
  case E_BAND:
	p->t_whichoper = E_ADDR;
	break;
  }
}
