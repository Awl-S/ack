/*** Generic C Identifier Processing Module	***/
/* IMPORT CheckId(char *, int), DoOption(char *), BeginOfProgram(), and
   EndOfProgram().
*/

#include <stdio.h>

extern CheckId();
extern DoOption();
extern BeginOfProgram(), EndOfProgram();

#define MAX_ID_LEN	256

char *ProgName;
int GCcopy;

main(argc, argv)
	char *argv[];
{
	char **nargv;
	int nargc = 0;
	FILE *fp;

	ProgName = *argv++;
	nargv = argv;

	BeginOfProgram();
	while (--argc > 0) {
		if ((*argv)[0] == '-') {
			DoOption(*argv++);
		}
		else {
			nargv[nargc++] = *argv++;
		}
	}

	if (nargc > 0) {
		while (nargc-- > 0) {
			if ((fp = fopen(*nargv, "r")) == NULL) {
				fprintf(stderr, "%s: cannot read file \"%s\"\n",
					ProgName, *nargv);
			}
			else {
				DoFile(fp);
			}
			nargv++;
		}
	}
	else {
		DoFile(stdin);
	}
	EndOfProgram();
	exit(0);
}

DoFile(fp)
	FILE *fp;
{
	register c;

	while ((c = getc(fp)) != EOF) {
		switch (c) {

		case '"':
		case '\'':
			if (GCcopy) putchar(c);
			SkipString(fp, c);
			break;
		
		case '/':
			if (GCcopy) putchar(c);
			if ((c = getc(fp)) == '*') {
				if (GCcopy) putchar(c);
				SkipComment(fp);
			}
			else {
				ungetc(c, fp);
			}
			break;

		default:
			if (StartId(c)) {
				DoIdent(fp, c);
			}
			else if (StartNum(c)) {
				if (GCcopy) putchar(c);
				DoNum(fp, c);
			}
			else if (GCcopy) putchar(c);
			break;
		}
	}
	fclose(fp);
}

SkipString(fp, stopc)
	FILE *fp;
{
	register c;

	while ((c = getc(fp)) != EOF) {
		if (GCcopy) putchar(c);
		if (c == stopc) {
			return;
		}

		if (c == '\\') {
			c = getc(fp);
			if (GCcopy) putchar(c);
		}
	}
}

SkipComment(fp)
	FILE *fp;
{
	register c;

	while ((c = getc(fp)) != EOF) {
		if (GCcopy) putchar(c);
		if (c == '*') {
			if ((c = getc(fp)) == '/') {
				if (GCcopy) putchar(c);
				return;
			}
			ungetc(c, fp);
		}
	}
}

DoIdent(fp, s)
	FILE *fp;
{
	char id_buf[MAX_ID_LEN];
	register cnt = 1;
	register c;

	id_buf[0] = s;

	while ((c = getc(fp)) != EOF) {
		if (InId(c)) {
			id_buf[cnt++] = c;
		}
		else {
			ungetc(c, fp);
			id_buf[cnt] = '\0';
			CheckId(id_buf, cnt);
			return;
		}
	}
}

StartId(c)
{
	switch (c) {

	case 'a': case 'b': case 'c': case 'd': case 'e':
	case 'f': case 'g': case 'h': case 'i': case 'j':
	case 'k': case 'l': case 'm': case 'n': case 'o':
	case 'p': case 'q': case 'r': case 's': case 't':
	case 'u': case 'v': case 'w': case 'x': case 'y':
	case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E':
	case 'F': case 'G': case 'H': case 'I': case 'J':
	case 'K': case 'L': case 'M': case 'N': case 'O':
	case 'P': case 'Q': case 'R': case 'S': case 'T':
	case 'U': case 'V': case 'W': case 'X': case 'Y':
	case 'Z':
	case '_':
		return 1;
	
	default:
		return 0;
	}
}

InId(c)
{
	switch (c) {

	case 'a': case 'b': case 'c': case 'd': case 'e':
	case 'f': case 'g': case 'h': case 'i': case 'j':
	case 'k': case 'l': case 'm': case 'n': case 'o':
	case 'p': case 'q': case 'r': case 's': case 't':
	case 'u': case 'v': case 'w': case 'x': case 'y':
	case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E':
	case 'F': case 'G': case 'H': case 'I': case 'J':
	case 'K': case 'L': case 'M': case 'N': case 'O':
	case 'P': case 'Q': case 'R': case 'S': case 'T':
	case 'U': case 'V': case 'W': case 'X': case 'Y':
	case 'Z':
	case '_':
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return 1;
	
	default:
		return 0;
	}
}

StartNum(c)
{
	switch(c) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return 1;
	}
	return 0;
}

#define inrange(c, l, u)	((unsigned)((c) - (l)) <= ((u) - (l)))
#define isdec(c) inrange(c, '0', '9')
#define isoct(c) inrange(c, '0', '7')
#define ishex(c) (isdec(c) || inrange(c, 'a', 'f') || inrange(c, 'A', 'F'))
#define getdec(c, fp)	do { c = getc((fp)); if (GCcopy) putchar(c);} while (isdec(c))
#define getoct(c, fp)	do { c = getc((fp)); if (GCcopy) putchar(c);} while (isoct(c))
#define gethex(c, fp)	do { c = getc((fp)); if (GCcopy) putchar(c);} while (ishex(c))

DoNum(fp, c)
	register FILE *fp;
{
	if (c != '0') {
		getdec(c, fp);
		if (c == '.')
			getdec(c, fp);
		if (c == 'e') {
			c = getc(fp);
			if (c == '+' || c == '-')
				c = getc(fp);
			if (isdec(c))
				getdec(c, fp);
		}
	}
	else {
		c = getc(fp);
		if (GCcopy) putchar(c);
		if (c == 'x' || c == 'X')
			gethex(c, fp);
		else
		if (isoct(c))
			getoct(c, fp);
	}
}
