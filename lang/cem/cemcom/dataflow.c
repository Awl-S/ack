/* $Header$ */
/*	DATAFLOW ANALYSIS ON C PROGRAMS	*/

/*	Compile the C compiler with flag DATAFLOW.
	Use the compiler option --d.
*/

#include	"dataflow.h"	/* UF */

#ifdef	DATAFLOW
char *CurrentFunction = 0;
int NumberOfCalls;

DfaStartFunction(nm)
	char *nm;
{
	CurrentFunction = nm;
	NumberOfCalls = 0;
}

DfaEndFunction()
{
	if (NumberOfCalls == 0)
		print("DFA: %s: --none--\n", CurrentFunction);
}

DfaCallFunction(s)
	char *s;
{
	print("DFA: %s: %s\n", CurrentFunction, s);
	++NumberOfCalls;
}
#endif	DATAFLOW
