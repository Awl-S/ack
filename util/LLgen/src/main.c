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
 * main.c
 * Contains main program, and some error message routines
 */

# include "types.h"
# include "io.h"
# include "extern.h"
# include "sets.h"
# include "assert.h"

# ifndef NORCSID
static string rcsid6 = "$Header$";
# endif

/* In this file the following routines are defined: */
extern int	main();
STATIC		readgrammar();
STATIC		doparse();
extern		error();
extern		fatal();
extern		comfatal();
extern		copyfile();
extern		install();
# ifndef NDEBUG
extern		badassertion();
# endif not NDEBUG

main(argc,argv) register string	argv[]; {
	register string arg;
	string libpath();

	/* Initialize */

	assval = 0400;
	/* read options */
	
	while (argc >= 2 && (arg = argv[1], *arg == '-')) {
		while (*++arg) {
			switch(*arg) {
			  case 'j':
			  case 'J':
				jmptable_option = 1;
				continue;
			  case 'w':
			  case 'W':
				wflag = 1;
				continue;
			  case 'v':
			  case 'V':
				verbose++;
				continue;
			  case 'l':
			  case 'L':
				low_percentage = atoi(++arg);
				break;
			  case 'h':
			  case 'H':
				high_percentage = atoi(++arg);
				break;
			  case 'm':
			  case 'M':
				min_cases_for_jmptable = atoi(++arg);
				break;
# ifndef NDEBUG
			  case 'd':
			  case 'D':
				debug++;
				continue;
			  case 'r':
			  case 'R':
				if (rec_file) {
					fprintf(stderr,"duplicate -r flag\n");
					exit(1);
				}
			  	rec_file = ++arg;
				break;
			  case 'i':
			  case 'I':
				if (incl_file) {
					fprintf(stderr,"duplicate -i flag\n");
					exit(1);
				}
			  	incl_file = ++arg;
				break;
# endif not NDEBUG
			  case 'x':
			  case 'X':
				ntneeded = 1;
				ntprint = 1;
				continue;
			  default:
				fprintf(stderr,"illegal option : %c\n",*arg);
				exit(1);
			}
			break;
		}
		argv++;
		argc--;
	}
	/*
	 * Now check wether the sets should include nonterminals
	 */
	if (verbose == 2) ntneeded = 1;
	/*
	 * Initialise
	 */
# ifndef NDEBUG
	if (!rec_file) {
# endif
		rec_file = libpath("rec");
# ifndef NDEBUG
	}
	if (!incl_file) {
# endif
		incl_file = libpath("incl");
# ifndef NDEBUG
	}
# endif
	if ((fact = fopen(f_temp,"w")) == NULL) {
		fputs("Cannot create temporary\n",stderr);
		exit(1);
	}
	name_init();
	readgrammar(argc,argv);
	setinit(ntneeded);
	maxnt = &nonterms[nnonterms];
	maxt = &tokens[ntokens];
	fclose(fact);
	/*
	 * Now, the grammar is read. Do some computations
	 */
	co_reach();		/* Check for undefined and unreachable */
	if (nerrors) comfatal();
	do_compute();
	conflchecks();
	if (nerrors) comfatal();
	if (argc-- == 1) {
		fputs("No code generation for input from standard input\n",
		      stderr);
	}
	else	gencode(argc);
	UNLINK(f_temp);
	UNLINK(f_pars);
	if (verbose) {
		extern char *sbrk(), *end;

		fprintf(stderr, "number of nonterminals: %d\n", nnonterms);
		fprintf(stderr, "number of tokens: %d\n", ntokens);
		fprintf(stderr, "number of term structures: %d\n", nterms);
		fprintf(stderr, "number of alternation structures: %d\n", nalts);
		fprintf(stderr, "total memory used: %d\n", sbrk(0) - (char *) &end);
	}
	exit(0);
}

STATIC
readgrammar(argc,argv) char *argv[]; {
	/*
	 * Do just what the name suggests : read the grammar
	 */
	register p_file	p;
	p_mem		alloc();

	linecount = 0;
	f_input = "no filename";
	/*
	 * Build the file structure
	 */
	files = p = (p_file) alloc((unsigned) (argc+1) * sizeof(t_file));
	if (argc-- == 1) {
		finput = stdin;
		f_input = "standard input";
		doparse(p++);
	} else {
		while (argc--) {
			if ((finput = fopen(f_input=argv[1],"r")) == NULL) {
				fatal(0,e_noopen,f_input);
			}
			doparse(p++);
			argv++;
			fclose(finput);
		}
	}
	maxfiles = p;
	if (! lexical) lexical = "yylex";
	/*
	 * There must be a start symbol!
	 */
	if (start == 0) {
		fatal(linecount,"Missing %%start");
	}
	if (nerrors) comfatal();
}

STATIC
doparse(p) register p_file p; {

	linecount = 0;
	p->f_name = f_input;
	p->f_firsts = 0;
	pfile = p;
	sorder = 0;
	porder = 0;
	LLparse();
	p->f_list = sorder;
}

/* VARARGS1 */
error(lineno,s,t,u) string	s,t,u; {	
	/*
	 * Just an error message
	 */

	++nerrors;
	if (!lineno) lineno = 1;
	fprintf(stderr,"\"%s\", line %d : ",f_input, lineno);
	fprintf(stderr,s,t,u);
	fputs("\n",stderr);
}

/* VARARGS1 */
warning(lineno,s,t,u) string	s,t,u; {	
	/*
	 * Just a warning
	 */

	if (wflag) return;
	if (!lineno) lineno = 1;
	fprintf(stderr,"\"%s\", line %d : (Warning) ",f_input, lineno);
	fprintf(stderr,s,t,u);
	fputs("\n",stderr);
}

/* VARARGS1 */
fatal(lineno,s,t,u) string	s,t,u; {
	/*
	 * Fatal error
	 */
	error(lineno,s,t,u);
	comfatal();
}

comfatal() {
	/*
	 * Some common code for exit on errors
	 */
	if (fact != NULL) {
		fclose(fact);
		UNLINK(f_temp);
	}
	if (fpars != NULL) fclose(fpars);
	UNLINK(f_pars);
	exit(1);
}

copyfile(file) string file; {
	/*
	 * Copies a file indicated by the parameter to filedescriptor fpars.
	 */
	register int	c;
	register FILE	*f;

	if ((f = fopen(file,"r")) == NULL) {
		fatal(0,"Cannot open libraryfile, call an expert");
	}
	while ((c = getc(f)) != EOF) putc(c,fpars);
	fclose(f);
}

install(target, source) string target, source; {
	/*
	 * Copy the temporary file generated from source to target
	 * if allowed (which means that the target must be generated
	 * by LLgen from the source, or that the target is not present
	 */
	register int	c1, c2;
	register FILE	*f1, *f2;
	int		cnt;

	/*
	 * First open temporary, generated for source
	 */
	if ((f1 = fopen(f_pars,"r")) == NULL) {
		fatal(0,e_noopen,f_pars);
	}
	/*
	 * Now open target for reading
	 */
	if ((f2 = fopen(target,"r")) == NULL) {
		fclose(f1);
		RENAME(f_pars, target);
		return;
	}
	/*
	 * Compute length of LLgen identification string. The target must
	 * start with that!
	 */
	cnt = strlen(LLgenid) + strlen(source) - 2;
	/*
	 * Now compare the target with the temporary
	 */
	do {
		c1 = getc(f1);
		c2 = getc(f2);
		if (cnt >= 0) cnt--;
	} while (c1 == c2 && c1 != EOF);
	fclose(f1);
	fclose(f2);
	/*
	 * Here, if c1 != c2 the target must be recreated
	 */
	if (c1 != c2) {
		if (cnt >= 0) {
			fatal(0,"%s : not a file generated by LLgen",target);
		}
		RENAME(f_pars,target);
	}
}

#ifndef NDEBUG
badassertion(asstr,file,line) char *asstr, *file; {

	fprintf(stderr,"Assertion \"%s\" failed %s(%d)\n",asstr,file,line);
	if (fact != NULL) fclose(fact);
	if (fpars != NULL) fclose(fpars);
	abort();
}
#endif
