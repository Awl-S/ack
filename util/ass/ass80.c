/*
 * (c) copyright 1983 by the Vrije Universiteit, Amsterdam, The Netherlands.
 *
 *          This product is part of the Amsterdam Compiler Kit.
 *
 * Permission to use, sell, duplicate or disclose this software must be
 * obtained in writing. Requests for such permissions may be sent to
 *
 *      Dr. Andrew S. Tanenbaum
 *      Wiskundig Seminarium
 *      Vrije Universiteit
 *      Postbox 7161
 *      1007 MC Amsterdam
 *      The Netherlands
 *
 */

#include        "ass00.h"
#include        "assex.h"
#include        "../../h/em_path.h"

#ifndef NORCSID
static char rcs_id[] = "$Header$" ;
#endif

/*
 * this file contains several library routines.
 */

zero(area,length) char *area; unsigned length ; {
	register char *p;
	register n;
	/*
	 * Clear area of length bytes.
	 */
	if ((n=length)==0)
		return;
	p = area;
	do *p++=0; while (--n);
}

/* VARARGS1 */
static void pr_error(string1,a1,a2,a3,a4) char *string1 ; {
	/*
	 * diagnostic output
	 */
	fprintf(stderr,"%s: ",progname);
	if (curfile) {
		fprintf(stderr,"file %s",curfile);
		if (archmode)
			fprintf(stderr," (%.14s)",archhdr.ar_name);
		fprintf(stderr,": ");
	}
	if ( pstate.s_curpro ) {
		fprintf(stderr,"proc %s, ",pstate.s_curpro->p_name);
	}
	fprintf(stderr,"line %d: ",line_num);
	fprintf(stderr,string1,a1,a2,a3,a4);
	fprintf(stderr,"\n");
}

/* VARARGS1 */
void error(string1,a1,a2,a3,a4) char *string1 ; {
	pr_error(string1,a1,a2,a3,a4) ;
	nerrors++ ;
}

/* VARARGS1 */
void werror(string1,a1,a2,a3,a4) char *string1 ; {
	if ( wflag ) return ;
	pr_error(string1,a1,a2,a3,a4) ;
}

fatal(s) char *s; {
	/*
	 * handle fatal errors
	 */
	error("Fatal error: %s",s);
	dump(0);
	exit(-1);
}

#ifndef CPM
FILE *frewind(f) FILE *f ; {
	/* Rewind a file open for writing and open it for reading */
	/* Assumption, file descriptor is r/w */
	register FILE *tmp ;
	rewind(f);
	tmp=fdopen(dup(fileno(f)),"r");
	fclose(f);
	return tmp ;
}
#endif

int xgetc(af) register FILE *af; {
	register int nextc;
	/*
	 * read next character; fatal if there isn't one
	 */
	nextc=fgetc(af) ;
	if ( feof(af) )
			fatal("unexpected end of file");
	return nextc ;
}

xputc(c,af) register FILE *af; {
	/* output one character and scream if it gives an error */
	fputc(c,af) ;
	if ( ferror(af) ) fatal("write error") ;
}


putblk(stream,from,amount)
	register FILE *stream; register char *from ; register int amount ; {

	for ( ; amount-- ; from++ ) {
		fputc(*from,stream) ;
		if ( ferror(stream) ) fatal("write error") ;
	}
}

int getblk(stream,from,amount)
	register FILE *stream; register char *from ; register int amount ; {

	for ( ; amount-- ; from++ ) {
		*from = fgetc(stream) ;
		if ( feof(stream) ) return 1 ;
	}
	return 0 ;
}

xput16(w,f) FILE *f; {
	/*
	 * two times xputc
	 */
	xputc(w,f);
	xputc(w>>8,f);
}

xputarb(l,w,f) int l ; cons_t w ; FILE *f ; {
	while ( l-- ) {
		xputc( int_cast w,f) ;
		w >>=8 ;
	}
}

put8(n) {
	xputc(n,tfile);
	textoff++;
}

put16(n) {
	/*
	 * note reversed order of bytes.
	 * this is done for faster interpretation.
	 */
	xputc(n>>8,tfile);
	xputc(n&0377,tfile);
	textoff += 2;
}

put32(n) cons_t n ; {
	put16( int_cast (n>>16)) ;
	put16( int_cast n) ;
}

put64(n) cons_t n ; {
	fatal("put64 called") ;
}

int xget8() {
	/*
	 * Read one byte from ifile.
	 */
	if (libeof && inpoff >= libeof)
		return EOF ;
	inpoff++;
	return fgetc(ifile) ;
}

unsigned get8() {
	register int nextc;
	/*
	 * Read one byte from ifile.
	 */
	nextc=xget8();
	if ( nextc==EOF ) {
		if (libeof)
			fatal("Tried to read past end of arentry\n");
		else
			fatal("end of file on input");
	}
	return nextc ;
}

cons_t xgetarb(l,f) int l; FILE *f ; {
	cons_t val ;
	register int shift ;

	shift=0 ; val=0 ;
	while ( l-- ) {
		val += ((cons_t)ctrunc(xgetc(f)))<<shift ;
		shift += 8 ;
	}
	return val ;
}

ext8(b) {
	/*
	 * Handle one byte of data.
	 */
	++dataoff;
	xputc(b,dfile);
}

extword(w) cons_t w ; {
	/* Assemble the word constant w.
	 * NOTE: The bytes are written low to high.
	 */
	register i ;
	for ( i=wordsize ; i-- ; ) {
		ext8( int_cast w) ;
		w >>= 8 ;
	}
}

extarb(size,value) int size ; long value ; {
	/* Assemble the 'size' constant value.
	 * The bytes are again written low to high.
	 */
	register i ;
	for ( i=size ; i-- ; ) {
		ext8( int_cast value ) ;
		value >>=8 ;
	}
}

extadr(a) cons_t a ; {
	/* Assemble the word constant a.
	 * NOTE: The bytes are written low to high.
	 */
	register i ;
	for ( i=ptrsize ; i-- ; ) {
		ext8( int_cast a) ;
		a >>= 8 ;
	}
}

xputa(a,f) cons_t a ; FILE *f ; {
	/* Assemble the pointer constant a.
	 * NOTE: The bytes are written low to high.
	 */
	register i ;
	for ( i=ptrsize ; i-- ; ) {
		xputc( int_cast a,f) ;
		a >>= 8 ;
	}
}

cons_t xgeta(f) FILE *f ; {
	/* Read the pointer constant a.
	 * NOTE: The bytes were written low to high.
	 */
	register i, shift ;
	cons_t val ;
	val = 0 ; shift=0 ;
	for ( i=ptrsize ; i-- ; ) {
		val += ((cons_t)xgetc(f))<<shift ;
		shift += 8 ;
	}
	return val ;
}

#define MAXBYTE 255

int icount(size) {
	int amount ;
	amount=(dataoff-lastoff)/size ;
	if ( amount>MAXBYTE) fatal("Descriptor overflow");
	return amount ;
}

setmode(mode) {

	if (datamode==mode) {   /* in right mode already */
		switch ( datamode ) {
		case DATA_CONST:
			if ( (dataoff-lastoff)/wordsize < MAXBYTE ) return ;
			break ;
		case DATA_BYTES:
			if ( dataoff-lastoff < MAXBYTE ) return ;
			break ;
		case DATA_IPTR:
		case DATA_DPTR:
			if ( (dataoff-lastoff)/ptrsize < MAXBYTE ) return ;
			break ;
		case DATA_ICON:
		case DATA_FCON:
		case DATA_UCON:
			break ;
		default:
			return ;
		}
		setmode(DATA_NUL) ; /* flush current descriptor */
		setmode(mode) ;
		return;
	}
	switch(datamode) {              /* terminate current mode */
	case DATA_NUL:
		break;                  /* nothing to terminate */
	case DATA_CONST:
		lastheader->r_val.rel_i=icount(wordsize) ;
		lastheader->r_typ = RELHEAD;
		datablocks++;
		break;
	case DATA_BYTES:
		lastheader->r_val.rel_i=icount(1) ;
		lastheader->r_typ = RELHEAD;
		datablocks++;
		break;
	case DATA_DPTR:
	case DATA_IPTR:
		lastheader->r_val.rel_i=icount(ptrsize) ;
		lastheader->r_typ = RELHEAD;
		datablocks++;
		break;
	default:
		datablocks++;
		break;
	}
	datamode=mode;
	switch(datamode) {
	case DATA_NUL:
		break;
	case DATA_CONST:
		ext8(HEADCONST);
		lastheader=data_reloc( chp_cast 0,dataoff,RELNULL);
		ext8(0);
		lastoff=dataoff;
		break;
	case DATA_BYTES:
		ext8(HEADBYTE);
		lastheader=data_reloc( chp_cast 0,dataoff,RELNULL);
		ext8(0);
		lastoff=dataoff;
		break;
	case DATA_IPTR:
		ext8(HEADIPTR);
		lastheader=data_reloc( chp_cast 0,dataoff,RELNULL);
		ext8(0);
		lastoff=dataoff;
		break;
	case DATA_DPTR:
		ext8(HEADDPTR);
		lastheader=data_reloc( chp_cast 0,dataoff,RELNULL);
		ext8(0);
		lastoff=dataoff;
		break;
	case DATA_ICON:
		ext8(HEADICON) ;
		ext8( int_cast consiz) ;
		break;
	case DATA_FCON:
		ext8(HEADFCON) ;
		ext8( int_cast consiz) ;
		break;
	case DATA_UCON:
		ext8(HEADUCON) ;
		ext8( int_cast consiz) ;
		break;
	case DATA_REP:
		ext8(HEADREP) ;
		break ;
	default:
		fatal("Unknown mode in setmode") ;
	}
}

#ifndef CPM
int tmpfil() {
	register char *fname, *cpname ;
	char *sfname;
	register fildes,pid;
	static char name[80] = TMP_DIR ;
	int count;
	/*
	 * This procedure returns a file-descriptor of a temporary
	 * file valid for reading and writing.
	 * After closing the tmpfil-descriptor the file is lost
	 * Calling this routine frees the program from generating uniqe names.
	 */
	sfname = fname = "tmp.00000";
	count = 10;
	pid = getpid();
	fname += 4;
	while (pid!=0) {
		*fname++ = (pid&07) + '0';
		pid >>= 3;
	}
	*fname = 0;
	for ( fname=name ; *fname ; fname++ ) ;
	cpname=sfname ;
	while ( *fname++ = *cpname++ ) ;
	do {
		fname = name;
		if ((fildes = creat(fname, 0600)) < 0)
			if ((fildes = creat(fname=sfname, 0600)) < 0)
				return(-1);
		if (close(fildes) < 0)
			;
	} while((fildes = open(fname, 2)) < 0 && count--);
	if (unlink(fname) < 0)
		;
	return(fildes);
}
#endif
