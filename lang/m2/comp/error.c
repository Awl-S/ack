/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 *
 * Author: Ceriel J.H. Jacobs
 */

/* E R R O R   A N D   D I A G N O S T I C   R O U T I N E S */

/* $Header$ */

/*	This file contains the (non-portable) error-message and diagnostic
	giving functions.  Be aware that they are called with a variable
	number of arguments!
*/

#include	"errout.h"
#include	"debug.h"

#include	<varargs.h>

#include	<system.h>
#include	<em_arith.h>
#include	<em_label.h>
#include	<em_code.h>

#include	"strict3rd.h"
#include	"input.h"
#include	"f_info.h"
#include	"LLlex.h"
#include	"main.h"
#include	"node.h"
#include	"warning.h"
#include	"nostrict.h"

/* error classes */
#define	ERROR		1
#define	WARNING		2
#define	LEXERROR	3
#define	LEXWARNING	4
#define	CRASH		5
#define	FATAL		6
#ifdef DEBUG
#define VDEBUG		7
#endif

int err_occurred;

extern char *symbol2str();

/*	There are three general error-message functions:
		lexerror()	lexical and pre-processor error messages
		error()		syntactic and semantic error messages
		node_error()	errors in nodes
	The difference lies in the place where the file name and line
	number come from.
	Lexical errors report from the global variables LineNumber and
	FileName, node errors get their information from the
	node, whereas other errors use the information in the token.
*/

#ifdef DEBUG
/*VARARGS1*/
debug(va_alist)
	va_dcl
{
	va_list ap;

	va_start(ap);
	{
		_error(VDEBUG, NULLNODE, ap);
	}
	va_end(ap);
}
#endif DEBUG

/*VARARGS1*/
error(va_alist)
	va_dcl
{
	va_list ap;

	va_start(ap);
	{
		_error(ERROR, NULLNODE, ap);
	}
	va_end(ap);
}

/*VARARGS2*/
node_error(va_alist)
	va_dcl
{
	va_list ap;

	va_start(ap);
	{
		t_node *node = va_arg(ap, t_node *);

		_error(ERROR, node, ap);
	}
	va_end(ap);
}

/*VARARGS2*/
warning(va_alist)
	va_dcl
{
	va_list ap;

	va_start(ap);
	{
		_error(WARNING, NULLNODE, ap);
	}
	va_end(ap);
}

/*VARARGS3*/
node_warning(va_alist)
	va_dcl
{
	va_list ap;

	va_start(ap);
	{
		t_node *nd = va_arg(ap, t_node *);
		_error(WARNING, nd, ap);
	}
	va_end(ap);
}

/*VARARGS1*/
lexerror(va_alist)
	va_dcl
{
	va_list ap;

	va_start(ap);
	{
		_error(LEXERROR, NULLNODE, ap);
	}
	va_end(ap);
}

/*VARARGS2*/
lexwarning(va_alist)
	va_dcl
{
	va_list ap;

	va_start(ap);
	{
		_error(LEXWARNING, NULLNODE, ap);
	}
	va_end(ap);
}

/*VARARGS1*/
fatal(va_alist)
	va_dcl
{
	va_list ap;

	va_start(ap);
	{
		_error(FATAL, NULLNODE, ap);
	}
	va_end(ap);
	sys_stop(S_EXIT);
}

/*VARARGS1*/
crash(va_alist)
	va_dcl
{
	va_list ap;

	va_start(ap);
	{
		_error(CRASH, NULLNODE, ap);
	}
	va_end(ap);
#ifdef DEBUG
	sys_stop(S_ABORT);
#else
	sys_stop(S_EXIT);
#endif
}

_error(class, node, ap)
	int class;
	t_node *node;
	register va_list ap;
{
	/*	_error attempts to limit the number of error messages
		for a given line to MAXERR_LINE.
	*/
	static unsigned int last_ln = 0;
	unsigned int ln = 0;
	static char * last_fn = 0;
	static int e_seen = 0;
	register char *remark = 0;
	int warn_class;
	char *fmt;
	
	/*	Since name and number are gathered from different places
		depending on the class, we first collect the relevant
		values and then decide what to print.
	*/
	/* preliminaries */
	switch (class)	{
	case ERROR:
	case LEXERROR:
	case CRASH:
	case FATAL:
		if (C_busy()) C_ms_err();
		err_occurred = 1;
		break;
	}

	/* the remark */
	switch (class)	{	
	case WARNING:
	case LEXWARNING:
		warn_class = va_arg(ap, int);
		if (! (warn_class & warning_classes)) return;
		switch(warn_class) {
#ifndef STRICT_3RD_ED
		case W_OLDFASHIONED:
			remark = "(old-fashioned use)";
			break;
#endif
#ifndef NOSTRICT
		case W_STRICT:
			remark = "(strict)";
			break;
#endif
		default:
			remark = "(warning)";
			break;
		}
		break;
	case CRASH:
		remark = "CRASH\007";
		break;
	case FATAL:
		remark = "fatal error --";
		break;
#ifdef DEBUG
	case VDEBUG:
		remark = "(debug)";
		break;
#endif DEBUG
	}
	
	/* the place */
	switch (class)	{	
	case WARNING:
	case ERROR:
		ln = node ? node->nd_lineno : dot.tk_lineno;
		break;
	case LEXWARNING:
	case LEXERROR:
	case CRASH:
	case FATAL:
#ifdef DEBUG
	case VDEBUG:
#endif DEBUG
		ln = LineNumber;
		break;
	}

	fmt  = va_arg(ap, char *);	
#ifdef DEBUG
	if (class != VDEBUG) {
#endif
	if (FileName == last_fn && ln == last_ln)	{
		/* we've seen this place before */
		e_seen++;
		if (e_seen == MAXERR_LINE) fmt = "etc ...";
		else
		if (e_seen > MAXERR_LINE)
			/* and too often, I'd say ! */
			return;
	}
	else	{
		/* brand new place */
		last_ln = ln;
		last_fn = FileName;
		e_seen = 0;
	}
#ifdef DEBUG
	}
#endif DEBUG
	
	if (FileName) fprint(ERROUT, "\"%s\", line %u: ", FileName, ln);

	if (remark) fprint(ERROUT, "%s ", remark);

	doprnt(ERROUT, fmt, ap);		/* contents of error */
	fprint(ERROUT, "\n");
}
