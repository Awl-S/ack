/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
/* $Header$ */

#ifndef FILE
#include <stdio.h>
#endif
extern	unsigned linecount;
extern	int	prodepth;
extern	bool	Lflag;
extern	bool	nflag;
extern	byte	em_flag[];
extern	line_p	instrs,pseudos;
extern	FILE	*outfile;
extern	char	template[];
extern	offset	wordsize;
extern	offset	pointersize;
extern	char	*progname;
