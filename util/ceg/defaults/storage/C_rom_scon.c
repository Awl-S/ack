#include <em.h>
#include "mach.h"
#include "back.h"

#if EM_WSIZE == 1
#define conEM_WSIZE	con1
#define romEM_WSIZE	rom1
#define CAST_WSIZE	ONE_BYTE
#endif
#if EM_WSIZE == 2
#define conEM_WSIZE	con2
#define romEM_WSIZE	rom2
#define CAST_WSIZE	TWO_BYTES
#endif
#if EM_WSIZE == 4
#define conEM_WSIZE	con4
#define romEM_WSIZE	rom4
#define CAST_WSIZE	FOUR_BYTES
#endif

#if EM_PSIZE == 1
#define relocEM_PSIZE	reloc1
#endif
#if EM_PSIZE == 2
#define relocEM_PSIZE	reloc2
#endif
#if EM_PSIZE == 4
#define relocEM_PSIZE	reloc4
#endif

C_rom_scon( s, n)
char *s;
arith n;
{
	switchseg( SEGROM);
	dump_label();	

	for ( ; n-- > 0;)
		rom1( (ONE_BYTE) *s++);
}
