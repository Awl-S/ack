/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 *
 */

/*
 *  L L G E N
 *
 *  An Extended LL(1) Parser Generator
 *
 *  Author : Ceriel J.H. Jacobs
 */

/*
 * tokens.g
 * Defines the tokens for the grammar of LLgen.
 * The lexical analyser and LLmessage are also included here. 
 */

{
# include "types.h"
# include "io.h"
# include "extern.h"
# include "assert.h"
# include "cclass.h"

# ifndef NORCSID
static string	rcsidc = "$Header$";
# endif

/* Here are defined : */
extern int	scanner();
extern 		LLmessage();
extern int	input();
extern		unput();
extern		skipcomment();
# ifdef LINE_DIRECTIVE
STATIC		linedirective();
# endif
STATIC string	cpy();
STATIC string	vallookup();
}
/* Classes */

%token  C_IDENT ;	/* lextoken.t_string contains the identifier read */
%token	C_NUMBER ;	/* lextoken.t_num contains the number read */
%token	C_LITERAL ;	/* lextoken.t_string contains the literal read */

/* Keywords */

%token	C_TOKEN ;
%token	C_START ;
%token	C_IF ;
%token	C_WHILE ;
%token	C_PERSISTENT ;
%token	C_FIRST ;
%token	C_LEXICAL ;
%token	C_ONERROR ;
%token	C_AVOID ;
%token	C_PREFER ;
%token	C_DEFAULT ;

%lexical scanner ;

{

/*
 * Structure for a keyword
 */

typedef struct keyword {
	string	w_word;
	int	w_value;
} t_keyw, *p_keyw;

/*
 * The list of keywords, the most often used keywords come first.
 * Linear search is used, as there are not many keywords
 */

static t_keyw resword[] = {
	{ "token",	C_TOKEN	},
	{ "avoid",	C_AVOID	},
	{ "prefer",	C_PREFER	},
	{ "persistent", C_PERSISTENT	},
	{ "default",	C_DEFAULT	},
	{ "if",		C_IF	},
	{ "while",	C_WHILE	},
	{ "first",	C_FIRST	},
	{ "start",	C_START	},
	{ "lexical",	C_LEXICAL	},
	{ "onerror",	C_ONERROR	},
	{ 0,		0	}
};

static t_token	savedtok;	/* to save lextoken in case of an insertion */
# ifdef LINE_DIRECTIVE
static  int	nostartline;	/* = 0 if at the start of a line */
# endif

scanner() {
	/*
	 * Lexical analyser, what else
	 */
	register int	ch;		/* Current char */
	register char *p = ltext;
	int		reserved = 0;	/* reserved word? */
	char		*max = &ltext[LTEXTSZ - 1];

	if (savedtok.t_tokno) {
				/* A token has been inserted.
				 * Now deliver the last lextoken again
				 */
		lextoken = savedtok;
		savedtok.t_tokno = 0;
		return lextoken.t_tokno;
	}
	for (;;) {
		ch = input();
		if (ch == EOF) return ch;
# ifdef LINE_DIRECTIVE
		if (ch == '#' && !nostartline) {
			linedirective();
			continue;
		}
# endif
		switch(c_class[ch]) {
		  case ISLIT :
			for (;;) {
				ch = input();
				if (ch == '\n' || ch == EOF) {
					error(linecount,"Missing '");
					break;
				}
				if (ch == '\'') break;
				if (ch == '\\') {
					*p++ = ch;
					ch = input();
				}
				*p++ = ch;
				if (p > max) p--;
			}
			*p = '\0';
			lextoken.t_string = ltext;
			return C_LITERAL;
		  case ISCOM :
			skipcomment(0);
			/* Fall through */
		  case ISSPA :
			continue;
		  case ISDIG : {
			register i = 0;
			do {
				i = 10 * i + (ch - '0');
				ch= input();
			} while (c_class[ch] == ISDIG);
			lextoken.t_num = i;
			unput(ch);
			return C_NUMBER; }
		  default:
			return ch;
		  case ISKEY :
			reserved = 1;
			ch = input();
			/* Fall through */
		  case ISLET :
			do {
				if (reserved && ch >= 'A' && ch <= 'Z') {
					ch += 'a' - 'A';
				}
				*p++ = ch;
				if (p > max) p--;
				ch = input();
			} while (c_class[ch] == ISDIG || c_class[ch] == ISLET);
			unput(ch);
			*p = '\0';
			if (reserved) {	/*
			 		 * Now search for the keyword
					 */
				register p_keyw w;

				w = resword;
				while (w->w_word) {
					if (! strcmp(ltext,w->w_word)) {
						/*
						 * Return token number.
						 */
						return w->w_value;
					}
					w++;
				}
				error(linecount,"Illegal reserved word");
			}
			lextoken.t_string = ltext;
			return C_IDENT;
		}
	}
}

static int	backupc;	/* for unput() */
static int	nonline;	/* = 1 if last char read was a newline */

input() {
	/*
	 * Low level input routine, used by all other input routines
	 */
	register	c;

        if (c = backupc) {
			/* Last char was "unput()". Deliver it again
			 */
		backupc = 0;
                return c;
	}
	if ((c = getc(finput)) == EOF) return c;
# ifdef LINE_DIRECTIVE
	nostartline = 1;
# endif
	if (!nonline) {
		linecount++;
# ifdef LINE_DIRECTIVE
		nostartline = 0;
# endif
		nonline = 1;
	}
	if (c == '\n') nonline = 0;
	return c;
}

unput(c) {
	/*
	 * "unread" c
	 */
	backupc = c;
}

skipcomment(flag) {
	/*
	 * Skip comment. If flag != 0, the comment is inside a fragment
	 * of C-code, so the newlines in it must be copied to enable the
	 * C-compiler to keep a correct line count
	 */
	register int	ch;
	int		saved;	/* line count on which comment starts */

	saved = linecount;
	if (input() != '*') error(linecount,"Illegal comment");
	do {
		ch = input();
		while (ch == '*') {
			if ((ch = input()) == '/') return;
		}
		if (flag && ch == '\n') putc(ch,fact);
	} while (ch != EOF);
	error(saved,"Comment does not terminate");
}

# ifdef LINE_DIRECTIVE
STATIC
linedirective() {
	/*
	 * Read a line directive
	 */
	register int	ch;
	register int	i;
	string		s_error = "Illegal line directive";
	string		store();
	register string	c;

	do {	/*
		 * Skip to next digit
		 * Do not skip newlines
		 */
		ch = input();
	} while (ch != '\n' && c_class[ch] != ISDIG);
	if (ch == '\n') {
		error(linecount,s_error);
		return;
	}
	i = 0;
	do  {
		i = i*10 + (ch - '0');
		ch = input();
	} while (c_class[ch] == ISDIG);
	while (ch != '\n' && ch != '"') ch = input();
	if (ch == '"') {
		c = ltext;
		do {
			*c++ = ch = input();
		} while (ch != '"' && ch != '\n');
		if (ch == '\n') {
			error(linecount,s_error);
			return;
		}
		*--c = '\0';
		do {
			ch = input();
		} while (ch != '\n');
		/*
		 * Remember the file name
		 */
		if (strcmp(f_input,ltext)) f_input = store(ltext);
	}
	linecount = i;
}
# endif

STATIC string
vallookup(s) {
	/*
	 * Look up the keyword that has token number s
	 */
	register p_keyw p = resword;

	while (p->w_value) {
		if (p->w_value == s) return p->w_word;
		p++;
	}
	return 0;
}

STATIC string
cpy(s,p,inserted) register string p; {
	/*
	 * Create a piece of error message for token s and put it at p.
	 * inserted = 0 if the token s was deleted (in which case we have
	 * attributes), else it was inserted
	 */
	register string t = 0;

	switch(s) {
	  case C_IDENT : 	
		if (!inserted) t = lextoken.t_string;
		else t = "identifier";
		break;
	  case C_NUMBER :
		t = "number";
		break;
	  case C_LITERAL :
		if (!inserted) {
			*p++ = '\'';
			t = lextoken.t_string;
			break;
		}
		t = "literal";
		break;
	  case EOFILE :
		t = "endoffile";
		break;
	}
	if (!t && (t = vallookup(s))) {
		*p++ = '%';
	}
	if (t) {	/*
			 * We have a string for the token. Copy it
			 */
		while (*t) *p++ = *t++;
		if (s == C_LITERAL && !inserted) {
			*p++ = '\'';
		}
		return p;
	}
	/*
	 * The token is a literal
	 */
	*p++ = '\'';
	if (s >= 040 && s <= 0176) *p++ = s;
	else {
	    *p++ = '\\';
	    switch(s) {
	      case '\b' : *p++ = 'b'; break;
	      case '\f' : *p++ = 'f'; break;
	      case '\n' : *p++ = 'n'; break;
	      case '\r' : *p++ = 'r'; break;
	      case '\t' : *p++ = 't'; break;
	      default : *p++='0'+((s&0377)>>6); *p++='0'+((s>>3)&07);
		        *p++='0'+(s&07);
	    }
	}
	*p++ = '\'';
	return p;
}

LLmessage(d) {
	/*
	 * d is either 0, in which case the current token has been deleted,
	 * or non-zero, in which case it represents a token that is inserted
	 * before the current token
	 */
	register string	s,t;
	char		buf[128];

	nerrors++;
	s = buf;
	if (d == 0) {
		s = cpy(LLsymb,s,0);
		t = " deleted";
		do *s++ = *t; while (*t++);
	} else {
		s = cpy(d,s,1);
		t = " inserted in front of ";
		do *s++ = *t++; while (*t);
		s = cpy(LLsymb,s,0);
		*s = '\0';
	}
	error(linecount, "%s", buf);
			/* Don't change this line to 
			 * error(linecount, buf).
			 * The string in "buf" might contain '%' ...
			 */
	if (d) {	/*
			 * Save the current token and make up some
			 * attributes for the inserted token
			 */
		savedtok = lextoken;
		savedtok.t_tokno = LLsymb;
		if (d == C_IDENT) lextoken.t_string = "dummy_identifier";
		else if (d == C_LITERAL) lextoken.t_string = "dummy_literal";
		else if (d == C_NUMBER) lextoken.t_num = 1;
	}
}
}
