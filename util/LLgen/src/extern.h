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
 * extern.h $Header$
 * Miscellanious constants and
 * some variables that are visible in more than one file
 */

# define LTEXTSZ	51	/* Size of longest token */

/*
 * options for the identifier search routine
 */

# define ENTERING	1
# define BOTH		2

/*
 * Now for some	declarations
 */

extern char	ltext[];	/* input buffer	*/
extern int	nnonterms;	/* number of nonterminals */
extern int	ntokens;	/* number of terminals */
extern int	nterms;		/* number of terms */
extern int	nalts;		/* number of alternatives */
extern p_start	start;		/* will	contain	startsymbols */
extern int	linecount;	/* line number */
extern int	assval;		/* to create difference	between	literals
				 * and other terminals
				 */
extern p_nont	nonterms;	/* the nonterminal array */
extern p_nont	maxnt;		/* is filled up until here */
extern p_token	tokens;		/* the token array */
extern p_token	maxt;		/* is filled up until here */
extern struct order *sorder, *porder;
				/* order of nonterminals in the grammar,
				 * important because actions are copied to
				 * a temporary file in the order in which they
				 * were read
				 */
extern string	e_noopen;	/* Error message string used often */
extern int	verbose;	/* Level of verbosity */
extern string	lexical;	/* name of lexical analyser */
extern string	onerror;	/* name of user error handler */
extern int	ntneeded;	/* ntneeded = 1 if nonterminals are included
				 * in the sets.
				 */
extern int	ntprint;	/* ntprint = 1 if they must be printed too in
				 * the LL.output file (-x option)
				 */
# ifndef NDEBUG
extern int	debug;
# endif not NDEBUG
extern p_file	files,pfile;	/* pointers to file structure.
				 * "files" points to the start of the
				 * list */
extern p_file	maxfiles;
extern string	LLgenid;	/* LLgen identification string */
extern t_token	lextoken;	/* the current token */
extern int	nerrors;
extern string	rec_file, incl_file;
