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
 * gencode.c
 * Defines the routine "gencode", which generates the parser
 * we wanted so badly.
 * This file is a mess, it should be cleaned up some time.
 */

# include "types.h"
# include "io.h"
# include "extern.h"
# include "sets.h"
# include "assert.h"
# include "cclass.h"

# ifndef NORCSID
static string rcsid3 = "$Header$";
# endif NORCSID

/*
 * Some codestrings used more than once
 */

static string	c_arrend =	"0 };\n";
static string	c_close =	"}\n";
static string	c_break =	"break;\n";
static string	c_read =	"LLread();\n";

/* Some constants used for reading from the action file */
# define ENDDECL	0400
# define IDENT		0401

static int nlabel;		/* count for the generation of labels */
static int firsts;		/* are there any? */
static int listcount;

/* In this file the following routines are defined: */
extern		gencode();
STATIC		opentemp();
STATIC		geninclude();
STATIC		genrecovery();
STATIC string	genname();
STATIC		generate();
STATIC		prset();
STATIC		macro();
STATIC		controlline();
STATIC		getparams();
STATIC		gettok();
STATIC		rulecode();
STATIC int *	dopush();
STATIC int *	mk_tokenlist();
STATIC		getaction();
STATIC		alternation();
STATIC		codeforterm();
STATIC		genswhead();
STATIC		gencases();
STATIC		genpush();
STATIC		genpop();
STATIC		genincrdecr();
STATIC		add_cases();
STATIC int	analyze_switch();
STATIC		out_list();

# define NOPOP		-20000

p_mem alloc(), ralloc();

doclose(f)
	FILE *f;
{
	if (ferror(f) != 0) {
		fatal(0,"Write error on temporary");
	}
	fclose(f);
}

STATIC int *
mk_tokenlist()
{
	register int *p = (int *)alloc(ntokens * sizeof(int)) + ntokens;
	register int i = ntokens;

	while (i--) *--p = -1;

	return p;
}

gencode(argc) {
	register p_file p = files;
	
	/* Set up for code generation */
	if ((fact = fopen(f_temp,"r")) == NULL) {
		fatal(0,e_noopen,f_temp);
	}

	/* For every source file .... */
	while (argc--) {
		/* Open temporary */
		f_input = p->f_name;
		opentemp(f_input);
		/* generate code ... */
		copyfile(incl_file);
		generate(p);
		getaction(2);
		doclose(fpars);
		/* And install */
		install(genname(p->f_name),p->f_name);
		p++;
	}
	geninclude();
	genrecovery();
}

STATIC
opentemp(str) string str; {

	if ((fpars = fopen(f_pars,"w")) == NULL) {
		fatal(0,e_noopen,f_pars);
	}
	if (!str) str = ".";
	fprintf(fpars,LLgenid,str);
}

STATIC
geninclude() {
	register p_token p;

	opentemp((string) 0);
	for (p = tokens; p < maxt; p++) {
		if (p->t_tokno >= 0400) {
			fprintf(fpars,"# define %s %d\n",
				  p->t_string,
				  p->t_tokno);
		}
	}
	doclose(fpars);
	install(HFILE, ".");
}

STATIC
genrecovery() {
	register FILE 	*f;
	register p_token t;
	register int	*q;
	register p_nont	p;
	register p_set	*psetl;
	int		*index;
	int		i;
	register p_start st;

	opentemp((string) 0);
	f = fpars;
	copyfile(incl_file);
	if (!firsts) fputs("#define LLNOFIRSTS\n", f);
	for (st = start; st; st = st->ff_next) {
		/* Make sure that every set the parser needs is in the list
		 * before generating a define of the number of them!
		 */
		p = &nonterms[st->ff_nont];
		if (g_gettype(p->n_rule) == ALTERNATION) {
			findindex(p->n_contains);
		}
	}
	i = maxptr - setptr;
	fprintf(f,
"#define LL_LEXI %s\n#define LL_SSIZE %d\n#define LL_NSETS %d\n#define LL_NTERMINALS %d\n",
		  lexical,
		  nbytes,
		  i > 0 ? i : 1,
		  ntokens);
	if (onerror) fprintf(f,"#define LL_USERHOOK %s\n", onerror);
	/* Now generate the routines that call the startsymbols */
	for (st = start; st; st = st->ff_next) {
		fputs(st->ff_name, f);
		p = &nonterms[st->ff_nont];
		fputs("() {\n\tunsigned int s[LL_NTERMINALS+LL_NSETS+2];\n\tLLnewlevel(s);\n\tLLread();\n", f);
		if (g_gettype(p->n_rule) == ALTERNATION) {
			genpush(findindex(p->n_contains));
		}
		fprintf(f, "\tL%d_%s();\n",
			st->ff_nont,
			p->n_name);
		if (getntout(p) == NOSCANDONE) {
			fputs("\tLL_NOSCANDONE(EOFILE);\n",f);
		}
		else	fputs("\tLL_SCANDONE(EOFILE);\n",f);
		fputs("\tLLoldlevel(s);\n}\n",f);
	}
	/* Now generate the sets */
	fputs("static char LLsets[] = {\n",f);
	for (psetl = setptr; psetl < maxptr; psetl++) prset(*psetl);
	fputs(c_arrend, f);
	index = (int *) alloc((unsigned) (assval * sizeof(int)));
	for (q = index; q < &index[assval];) *q++ = -1;
	for (t = tokens; t < maxt; t++) {
		index[t->t_tokno] = t - tokens;
	}
	fputs("#define LLindex (LL_index+1)\nstatic short LL_index[] = {0,0,\n",f);
	for (q = index+1; q < &index[assval]; q++) {
		fprintf(f, "%d,\n", *q);
	}
	fputs(c_arrend, f);
	free((p_mem) index);
	if (onerror) {
		fputs("static short LLtok[] = {\n", f);
		for (t = tokens; t < maxt; t++) {
			fprintf(f, t->t_tokno<0400 ? "'%s',\n" : "%s,\n",t->t_string);
		}
		fputs(c_arrend, f);
	}
	fputs("#define LL_NEWMESS\n", f);
	copyfile(rec_file);
	doclose(f);
	install(RFILE, ".");
}

STATIC
generate(f) p_file f; {
	/*
	 * Generates a parsing routine for every nonterminal
	 */
	register p_order s;
	register p_nont	p;
	int i;
	register p_first ff;
	int mustpop;

	fprintf(fpars, "#define LL_LEXI %s\n", lexical);
	listcount = 0;
	/* Generate first sets */
	for (ff = f->f_firsts; ff; ff = ff->ff_next) {
		macro(ff->ff_name,&nonterms[ff->ff_nont]);
	}
	
	/* For every nonterminal generate a function */
	for (s = f->f_list; s; s = s->o_next) {
		p = &nonterms[s->o_index];
		/* Generate functions in the order in which the nonterminals
		 * were defined in the grammar. This is important, because
		 * of synchronisation with the action file
		 */
		while (p->n_count--) getaction(1);
		if (g_gettype(p->n_rule) == EORULE &&
		    getntparams(p) == 0) {
			continue;
		}
		fprintf(fpars,"L%d_%s (\n",s->o_index,p->n_name);
		if (p->n_flags & PARAMS) {
			controlline();
			getparams();
		}
		else fputs(") {\n", fpars);
		if (p->n_flags & LOCALS) getaction(1);
		i = getntsafe(p);
		mustpop = NOPOP;
		if (g_gettype(p->n_rule) == ALTERNATION &&
		    i > SAFESCANDONE) {
			mustpop = findindex(p->n_contains);
			if (i == NOSCANDONE) {
				fputs(c_read, fpars);
				i = SCANDONE;
			}
		}
		nlabel = 1;
		rulecode(p->n_rule,
			 i,
			 getntout(p) != NOSCANDONE,
			 mustpop);
		fputs(c_close, fpars);
	}
}

STATIC
prset(p) p_set p; {
	register int k;
	register unsigned i;
	int j;

	j = nbytes;
	for (;;) {
		i = (unsigned) *p++;
		for (k = 0; k < sizeof(int); k++) {
			fprintf(fpars,"0%o,",(i & 0377));
			i >>= 8;
			if (--j == 0) {
				fputs("\n",fpars);
				return;
			}
		}
	}
	/* NOTREACHED */
}

STATIC
macro(s,n) string s; p_nont n; {
	int i;

	i = findindex(n->n_first);
	if (i < 0) {
		fprintf(fpars, "#define %s(x) ((x) == %d)\n",
			s,
			tokens[-(i+1)].t_tokno);
		return;
	}
	firsts = 1;
	fprintf(fpars,"#define %s(x) LLfirst((x), %d)\n", s, i);
}

STATIC
controlline() {
	/* Copy a compiler control line */
	register int l;
	register FILE *f1,*f2;

	f1 = fact; f2 = fpars;
	l = getc(f1);
	assert(l == '\0');
	do {
		l = getc(f1);
		if (l == EOF) fatal(0, "temp file mangled");
		putc(l,f2);
	} while ( l != '\n' ) ;
}

STATIC
getparams() {
	/* getparams is called if a nonterminal has parameters. The names
	 * of the parameters have to be found, and they should be declared
 	 */
	long off;
	register int l;
	long ftell();
	char first;

	first = ' ';
	/* save offset in file to be able to copy the declaration later */
	off = ftell(fact);
	/* First pass through declaration, find the parameter names */
	while ((l = gettok()) != ENDDECL) {
		if (l == ';' || l == ',') {
			/*
			 * The last identifier found before a ';' or a ','
			 * must be a parameter
			 */
			fprintf(fpars,"%c%s", first, ltext);
			first = ',';
		}
	}
	fputs(") ",fpars);
	/*
	 * Now copy the declarations 
	 */
	l = getc(fact);		/* patch: some implementations of fseek
				   do not work properly after "ungetc"
				*/
	fseek(fact,off,0);
	getaction(0);
	fputs(" {\n",fpars);
}

STATIC
gettok() {
	/* Read from the action file. */
	register int ch;
	register string	c;
	register FILE *f;

	f = fact;
	ch = getc(f);
	switch(ch) {
		case '\n':
			ch = getc(f);
			if (ch != EOF) {
				ungetc(ch,f);
				if (ch != '\0') return '\n';
			}
			return ENDDECL;
		case '\0':
			ungetc(ch,f);
			/* Fall through */
		case EOF :
			return ENDDECL;
		default :
			if (c_class[ch] == ISLET) {
				c = ltext;
				do {
					*c++ = ch;
					if (c-ltext >= LTEXTSZ) --c;
					ch = getc(f);
				} while (c_class[ch] == ISLET || c_class[ch] == ISDIG);
				if (ch != EOF) ungetc(ch,f);
				*c = '\0';
				return IDENT;
			}
			return ch;
	}
}

STATIC
rulecode(p,safety,mustscan,mustpop) register p_gram p; {
	/*
	 * Code for a production rule.
	 */

	register int	toplevel = 1;
	register FILE	*f;
	register int	*ppu;
	int		*pushlist;
	int		*ppushlist;

	/*
	 * First generate code to push the contains sets of this rule
	 * on a stack
	 */
	ppushlist = dopush(p,safety,1, &pushlist);
	if (mustpop != NOPOP) for (ppu = pushlist; ppu < ppushlist; ppu++) {
		if (*ppu == mustpop) {
			*ppu = mustpop = NOPOP;
			break;
		}
	}
	if (g_gettype(p) != ALTERNATION) {
		genpop(mustpop);
		mustpop = NOPOP;
	}
	for (ppu = pushlist; ppu < ppushlist; ppu++) genpush(*ppu);
	free((p_mem) pushlist);
	f = fpars;
	for (;;) {
		switch (g_gettype(p)) {
		  case EORULE :
			if (mustscan && safety == NOSCANDONE) {
				fputs(c_read,f);
			}
			return;
		  case LITERAL :
		  case TERMINAL : {
			register p_token t;
			string s;

			t = &tokens[g_getcont(p)];
			if (toplevel == 0) {
				fprintf(f,"LLtdecr(%d);\n", g_getcont(p));
			}
			if (safety == SAFE) {
				fputs("LL_SAFE(",f);
			}
			else if (safety == SAFESCANDONE) {
				fputs("LL_SSCANDONE(",f);
			}
			else if (safety == SCANDONE) {
				fputs("LL_SCANDONE(",f);
			}
			else /* if (safety == NOSCANDONE) */ {
				fputs("LL_NOSCANDONE(", f);
			}
			if (t->t_tokno < 0400) s = "'%s');\n";
			else	s = "%s);\n";
			fprintf(f,s,t->t_string);
			if (safety <= SAFESCANDONE && toplevel > 0) {
				safety = NOSCANDONE;
				toplevel = -1;
				p++;
				continue;
			}
			safety = NOSCANDONE;
			break; }
		  case NONTERM : {
			register p_nont n = &nonterms[g_getcont(p)];

			if (safety == NOSCANDONE &&
			    getntsafe(n) < NOSCANDONE) {
				safety = getntsafe(n);
				fputs(c_read, f);
			}
			if (toplevel == 0 &&
			    (g_gettype(n->n_rule) != ALTERNATION ||
			     getntsafe(n) <= SAFESCANDONE)) {
				genpop(findindex(n->n_contains));
			}
			fprintf(f,"L%d_%s(\n",g_getcont(p), n->n_name);
			if (g_getnpar(p)) {
				controlline();
				getaction(0);
			}
			fputs(");\n",f);
			safety = getntout(n);
			break; }
		  case TERM :
			safety = codeforterm(g_getterm(p),
						safety,
						toplevel);
			break;
		  case ACTION :
			getaction(1);
			p++;
			continue;
		  case ALTERNATION :
			alternation(p, safety, mustscan, mustpop, 0);
			return;
		}
		p++;
		toplevel = 0;
	}
}

STATIC
alternation(pp, safety, mustscan, mustpop, lb)
	p_gram pp;
{
	register p_gram	p = pp;
	register FILE	*f = fpars;
	register p_link	l;
	int		hulp, hulp1,hulp2;
	int		haddefault = 0;
	int		nsafe;
	p_set		set;
	p_set		setalloc();
	int		*tokenlist = mk_tokenlist();
	int		casecnt = 0;
	int		compacted;
	int		unsafe;

	assert(safety < NOSCANDONE);
	hulp = nlabel++;
	hulp1 = nlabel++;
	hulp2 = nlabel++;
	if (!lb) lb = hulp1;
	unsafe = onerror || safety > SAFESCANDONE;
	if (!unsafe) {
		genpop(mustpop);
		mustpop = NOPOP;
	}
	if (unsafe && hulp1 == lb) {
		fprintf(f,"L_%d: \n", hulp1);
	}
	while (g_gettype(p) != EORULE) {
		l = g_getlink(p);
		if (l->l_flag & COND) {
			if (!(l->l_flag & NOCONF)) {
				set = setalloc();
				setunion(set, l->l_others);
				setintersect(set, l->l_symbs);
				setminus(l->l_symbs, set);
				setminus(l->l_others, set);
				add_cases(set, tokenlist, casecnt++);
			}
		}
		if (!unsafe && (l->l_flag & DEF)) {
			haddefault = 1;
		}
		else	add_cases(l->l_symbs, tokenlist, casecnt++);
		if (l->l_flag & DEF) {
			haddefault = 1;
		}
		if ((l->l_flag & COND) && !(l->l_flag & NOCONF)) {
			p++;
			if (g_gettype(p+1) == EORULE) {
				setminus(g_getlink(p)->l_symbs, set);
				free((p_mem) set);
				continue;
			}
			free((p_mem) set);
			if (!haddefault) {
			}
			else {
				add_cases(l->l_others, tokenlist, casecnt++);
				unsafe = 0;
			}
			break;
		}
		p++;
	}

	unsafe = onerror || safety > SAFESCANDONE;
	p = pp;
	haddefault = 0;
	compacted = analyze_switch(tokenlist);
	if (compacted) {
		fputs("{", f);
		out_list(tokenlist, listcount, casecnt);
		fprintf(f, "switch(LL%d_tklist[LLcsymb]) {\n", listcount++);
	}
	else	fputs("switch(LLcsymb) {\n", f);
	casecnt = 0;

	while (g_gettype(p) != EORULE) {
		l = g_getlink(p);
		if (l->l_flag & COND) {
			if (l->l_flag & NOCONF) {
				fputs("#ifdef ___NOCONFLICT___\n", f);
			}
			else gencases(tokenlist, casecnt++, compacted);
			controlline();
			fputs("if (!",f);
			getaction(0);
			fprintf(f,") goto L_%d;\n", hulp);
			if (l->l_flag & NOCONF) {
				fputs("#endif\n", f);
			}
		}
		if (!unsafe && (l->l_flag & DEF)) {
			haddefault = 1;
			fputs("default:\n", f);
		}
		else	gencases(tokenlist, casecnt++, compacted);
		nsafe = SAFE;
		if (l->l_flag & DEF) {
			if (unsafe) {
				fprintf(f,"L_%d: ;\n", hulp2);
			}
			if (safety != SAFE) nsafe = SAFESCANDONE;
		}
		rulecode(l->l_rule, nsafe, mustscan, mustpop);
		fputs(c_break,f);
		if (unsafe && (l->l_flag & DEF)) {
			haddefault = 1;
			fprintf(f,
"default: if (LLskip()) goto L_%d;\ngoto L_%d;\n",
			lb, hulp2);
		}
		if ((l->l_flag & COND) && !(l->l_flag & NOCONF)) {
			p++;
			fprintf(f,"L_%d : ;\n",hulp);
			if (g_gettype(p+1) == EORULE) {
				continue;
			}
			if (!haddefault) {
				fputs("default:\n", f);
			}
			else {
				gencases(tokenlist, casecnt++, compacted);
				safety = SAFE;
				unsafe = 0;
			}
			if (! unsafe) {
				genpop(mustpop);
				mustpop = NOPOP;
			}
			alternation(p,safety,mustscan,mustpop,lb);
			break;
		}
		p++;
	}
	if (compacted) fputs(c_close, f);
	fputs(c_close, f);
	free((p_mem) tokenlist);
}

STATIC int *
dopush(p,safety,toplevel,pp) register p_gram p; int **pp; {
	/*
	 * The safety only matters if toplevel != 0
	 */
	unsigned int i = 100;
	register int *ip = (int *) alloc(100 * sizeof(int));

	*pp = ip;

	for (;;) {
		if (ip - *pp >= i) {
			*pp = (int *)
				ralloc((p_mem) (*pp), (i + 100) * sizeof(int));
			ip = *pp + i;
			i += 100;
		}
		switch(g_gettype(p)) {
		  case EORULE :
		  case ALTERNATION :
			return ip;
		  case TERM : {
			register p_term q;
			int rep_kind, rep_count;

			q = g_getterm(p);
			rep_kind = r_getkind(q);
			rep_count = r_getnum(q);
			if (!(toplevel > 0 && safety <= SAFESCANDONE &&
			    (rep_kind == OPT || (rep_kind == FIXED && rep_count == 0)))) {
				*ip++ = findindex(q->t_contains);
			}
			break; }
		  case LITERAL :
		  case TERMINAL :
			if (toplevel > 0 && safety <= SAFESCANDONE) {
				toplevel = -1;
				p++;
				safety = NOSCANDONE;
				continue;
			}
			if (toplevel == 0) *ip++ = -(g_getcont(p)+1);
			break;
		  case NONTERM : {
			register p_nont n;

			n = &nonterms[g_getcont(p)];
			if (toplevel == 0 ||
			    (g_gettype(n->n_rule) == ALTERNATION &&
			     getntsafe(n) > SAFESCANDONE)) {
				*ip++ = findindex(n->n_contains);
			}
			break; }
		  case ACTION :
			p++;
			continue;
		}
		toplevel = 0;
		safety = NOSCANDONE;
		p++;
	}
}

# define max(a,b) ((a) < (b) ? (b) : (a))

STATIC
getaction(flag) {
	/* Read an action from the action file.
	 * flag = 1 if it is an action,
	 * 0 when reading parameters
	 */
	register int match,ch;
	register FILE *f;
	register int newline;
	int mark = 0;

	if (flag == 1) {
		controlline();
	}
	f = fpars;
	newline = 0;
	for (;;) {
		ch = gettok();
		switch(ch) {
		  case ENDDECL:
			if (flag != 2) break;
			ch = getc(fact);
			assert(ch == '\0');
			fputs("\n",f);
			if (mark) return;
			mark = 1;
			continue;
		  case '\n':
			newline = 1;
			break;
		  case '\'' :
		  case '"' :
			if (newline) {
				newline = 0;
			}
			match = ch;
			for (;;) {
				putc(ch,f);
				ch = getc(fact);
				if (ch == match || !ch) break;
				if (ch == '\\') {
					putc(ch,f);
					ch = getc(fact);
				}
			}
			break;
		  case IDENT :
			if (newline) {
				newline = 0;
			}
			fputs(ltext,f);
			continue;
		}
		mark = 0;
		if (ch == ENDDECL) break;
		if (newline && ch != '\n') {
			newline = 0;
		}
		putc(ch,f);
	}
	if (flag) fputs("\n",f);
}

STATIC
codeforterm(q,safety,toplevel) register p_term q; {
	/*
	 * Generate code for a term
	 */
	register FILE	*f = fpars;
	register int	rep_count = r_getnum(q);
	register int	rep_kind = r_getkind(q);
	int		term_is_persistent = (q->t_flags & PERSISTENT);
	int		ispushed = NOPOP;
	int		sw = SAFE;

	if (!(toplevel > 0 && 
	      (safety == 0 || (!onerror && safety <= SAFESCANDONE)) &&
	    (rep_kind == OPT || (rep_kind == FIXED && rep_count == 0)))) {
		ispushed = findindex(q->t_contains);
	}
	if (safety == NOSCANDONE && (rep_kind != FIXED || rep_count == 0 ||
	    gettout(q) != NOSCANDONE)) {
		fputs(c_read, f);
		safety = SCANDONE;
	}
	if (rep_kind == PLUS && !term_is_persistent) {
		int temp;

		temp = findindex(q->t_first);
		if (temp != ispushed) {
			genpop(ispushed);
			ispushed = temp;
			genpush(temp);
		}
	}
	if (rep_count) {
		/* N > 0, so generate fixed forloop */
		fputs("{\nregister LL_i;\n", f);
		assert(ispushed != NOPOP);
		fprintf(f, "for (LL_i = %d; LL_i >= 0; LL_i--) {\n", rep_count - 1);
		if (rep_kind == FIXED) {
			fputs("if (!LL_i) ", f);
			genpop(ispushed);
			genpush(ispushed);
			if (safety == NOSCANDONE) {
				assert(gettout(q) == NOSCANDONE);
				fputs(c_read, f);
			}
		}
	}
	else if (rep_kind != OPT && rep_kind != FIXED) {
		/* '+' or '*', so generate infinite loop */
		fputs("for (;;) {\n",f);
	}
	else if (rep_kind == OPT &&
		 (safety == 0 || (!onerror && safety <= SAFESCANDONE))) {
		genpop(ispushed);
		ispushed = NOPOP;
	}
	if (rep_kind == STAR || rep_kind == OPT) {
		sw = genswhead(q, rep_kind, rep_count, safety, ispushed);
	}
	rulecode(q->t_rule,
		 t_safety(rep_kind,rep_count,term_is_persistent,safety),
		 gettout(q) != NOSCANDONE,
		 rep_kind == FIXED ? ispushed : NOPOP);
	if (gettout(q) == NOSCANDONE && rep_kind != FIXED) {
		fputs(c_read, f);
	}
	/* in the case of '+', the if is after the code for the rule */
	if (rep_kind == PLUS) {
		if (rep_count) {
			fputs("if (!LL_i) break;\n", f);
		}
		sw = genswhead(q, rep_kind, rep_count, safety, ispushed);
	}
	if (rep_kind != OPT && rep_kind != FIXED) fputs("continue;\n", f);
	if (rep_kind != FIXED) {
		fputs(c_close, f); /* Close switch */
		fputs(c_close, f);
		if (rep_kind != OPT) {
			genpop(ispushed);
			fputs(c_break, f);
		}
	}
	if (rep_kind != OPT && (rep_kind != FIXED || rep_count > 0)) {
		fputs(c_close, f);	/* Close for */
		if (rep_count > 0) {
			fputs(c_close, f);/* Close Register ... */
		}
	}
	return t_after(rep_kind, rep_count, gettout(q));
}

STATIC
genswhead(q, rep_kind, rep_count, safety, ispushed) register p_term q; {
	/*
	 * Generate switch statement for term q
	 */
	register FILE	*f = fpars;
	p_set		p1;
	p_set		setalloc();
	int		hulp1, hulp2;
	int		safeterm;
	int		termissafe = 0;
	int		casecnt = 0;
	int		*tokenlist = mk_tokenlist();
	int		compacted;

	if (rep_kind == PLUS) safeterm = gettout(q);
	else if (rep_kind == OPT) safeterm = safety;
	else /* if (rep_kind == STAR) */ safeterm = max(safety, gettout(q));
	hulp2 = nlabel++;
	fprintf(f, "L_%d : ", hulp2);
	if (q->t_flags & RESOLVER) {
		hulp1 = nlabel++;
		if (! (q->t_flags & NOCONF)) {
			p1 = setalloc();
			setunion(p1,q->t_first);
			setintersect(p1,q->t_follow);
			/*
			 * p1 now points to a set containing the conflicting
			 * symbols
			 */
			setminus(q->t_first, p1);
			setminus(q->t_follow, p1);
			setminus(q->t_contains, p1);
			add_cases(p1, tokenlist, casecnt++);
			free((p_mem) p1);
		}
	}
	if (safeterm == 0 || (!onerror && safeterm <= SAFESCANDONE)) {
		termissafe = 1;
	}
	else	add_cases(q->t_follow, tokenlist, casecnt++);
	if (!onerror && (q->t_flags & PERSISTENT) && safeterm != SAFE) {
		add_cases(q->t_contains, tokenlist, casecnt);
	}
	else	add_cases(q->t_first, tokenlist, casecnt);
	compacted = analyze_switch(tokenlist);
	fputs("{", f);
	if (compacted) {
		out_list(tokenlist, listcount, casecnt);
		fprintf(f, "switch(LL%d_tklist[LLcsymb]) {\n", listcount++);
	}
	else	fputs("switch(LLcsymb) {\n", f);
	casecnt = 0;
	if (q->t_flags & RESOLVER) {
		if (q->t_flags & NOCONF) {
			fputs("#ifdef ___NOCONFLICT___\n", f);
		}
		else gencases(tokenlist, casecnt++, compacted);
		controlline();
		fputs("if (", f);
		getaction(0);
		fprintf(f, ") goto L_%d;\n", hulp1);
		if (q->t_flags & NOCONF) {
			fputs("#endif ___NOCONFLICT___\n", f);
		}
	}
	if (safeterm == 0 || (!onerror && safeterm <= SAFESCANDONE)) {
		fputs("default:\n", f);
	}
	else	gencases(tokenlist, casecnt++, compacted);
	if (rep_kind == OPT) genpop(ispushed);
	fputs(c_break, f);
	if (! termissafe) {
		int i;
		static int nvar;

		assert(ispushed != NOPOP);
		if (ispushed >= 0) i = -ispushed;
		else i = tokens[-(ispushed+1)].t_tokno;
		++nvar;
		fprintf(f,"default:{int LL_%d=LLnext(%d);\n;if (!LL_%d) {\n",
			nvar, i, nvar);
		if (rep_kind == OPT) genpop(ispushed);
		fputs(c_break, f);
		fputs(c_close, f);
		fprintf(f,"else if (LL_%d & 1) goto L_%d;}\n",nvar, hulp2);
	}
	gencases(tokenlist, casecnt, compacted);
	if (q->t_flags & RESOLVER) {
		fprintf(f, "L_%d : ;\n", hulp1);
	}
	if (rep_kind == OPT) genpop(ispushed);
	if (rep_count > 0) {
		assert(ispushed != NOPOP);
		fputs(rep_kind == STAR ? "if (!LL_i) " : "if (LL_i == 1) ", f);
		genpop(ispushed);
	}
	free((p_mem) tokenlist);
	return safeterm;
}

STATIC
gencases(tokenlist, caseno, compacted)
	int 	*tokenlist;
{
	/*
	 * setp points to a bitset indicating which cases must
	 * be generated.
	 * YECH, the PCC compiler does not accept many cases without statements
	 * inbetween, so after every case label an empty statement is
	 * generated.
	 * The C-grammar used by PCC is really stupid on this point :
	 * it contains the rule
	 * 	statement : label statement
	 * which is right-recursive, and as is well known, LALR parsers don't
	 * handle these things very good.
	 * The grammar should have been written :
	 * 	labeledstatement : labels statement ;
	 *	labels : labels label | ;
	 */
	register p_token p;
	register int i;

	if (compacted) fprintf(fpars, "case %d :\n", caseno);
	for (i = 0, p = tokens; i < ntokens; i++, p++) {
		if (tokenlist[i] == caseno) {
			fprintf(fpars,
				compacted ?
				   (p->t_tokno < 0400 ? "/* case '%s' */\n" :
							"/* case %s */\n") :
				   p->t_tokno<0400 ? "case /* '%s' */ %d : ;\n"
					        : "case /*  %s  */ %d : ;\n",
				p->t_string, i);
		}
	}
}

static char namebuf[20];

STATIC string
genname(s) string s; {
	/*
	 * Generate a target file name from the
	 * source file name s.
	 */
	register string c,d;

	c = namebuf;
	while (*s) {
		if (*s == '/') {
			while (*s == '/') s++;
			if (*s) c = namebuf;
			else break;
		}
		*c++ = *s++;
	}
	for (d = c; --d > namebuf;) if (*d == '.') break;
	if (d == namebuf) d = c;
	if (d >= &namebuf[12]) {
		fatal(0,"%s : filename too long",namebuf);
	}
	*d++ = '.';
	*d++ = 'c';
	*d = '\0';
	return namebuf;
}

STATIC
genpush(d) {
	genincrdecr("incr", d);
}

STATIC
genincrdecr(s, d) string s; {
	if (d == NOPOP) return;
	if (d >= 0) {
		fprintf(fpars, "LLs%s(%d);\n", s,  d / nbytes);
		return;
	}
	fprintf(fpars, "LLt%s(%d);\n", s, -(d + 1));
}

STATIC
genpop(d) {
	genincrdecr("decr", d);
}

STATIC int
analyze_switch(tokenlist)
	int	*tokenlist;
{
	register int i;
	int ncases = 0;
	int percentage;
	int maxcase = 0, mincase = 0;

	if (! jmptable_option) return 0;
	for (i = 0; i < ntokens; i++) {
		if (tokenlist[i] >= 0) {
			ncases++;
			if (! mincase) mincase = i + 1;
			maxcase = i + 1;
		}
	}

	if (ncases < min_cases_for_jmptable) return 0;
	percentage = ncases * 100 / (maxcase - mincase);
	fprintf(fpars, "/* percentage is %d */\n", percentage);
	return percentage >= low_percentage && percentage <= high_percentage;
}

STATIC
add_cases(s, tokenlist, caseno)
	p_set	s;
	int	*tokenlist;
{
	register int i;

	for (i = 0; i < ntokens; i++) {
		if (IN(s, i)) {
			tokenlist[i] = caseno;
		}
	}
}

STATIC
out_list(tokenlist, listno, casecnt)
	int	*tokenlist;
{
	register int i;
	register FILE *f = fpars;

	fprintf(f, "static %s LL%d_tklist[] = {", 
		casecnt <= 127 ? "char" : "short",
		listno);
	
	for (i = 0; i < ntokens; i++) {
		fprintf(f, "%c%d,", i % 10 == 0 ? '\n': ' ', tokenlist[i]);
	}
	fputs(c_arrend, f);
}
