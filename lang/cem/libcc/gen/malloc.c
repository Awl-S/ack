/* $Header$ */

#define CLICK_SIZE	4096
#if EM_WSIZE == EM_PSIZE
typedef unsigned int vir_bytes;
#else
typedef long vir_bytes;
#endif
extern bcopy();

#define ALIGN(x, a)	(((x) + (a - 1)) & ~(a - 1))
#define BUSY		1
#define NEXT(p)		(* (char **) (p))

#ifdef pdp
#define BUGFIX	64	/* cannot set break in top 64 bytes */
#else
#define BUGFIX	0
#endif

extern char *sbrk(), *brk();
static char *bottom, *top;

static grow(len)
unsigned len;
{
  register char *p;
  register int click = CLICK_SIZE;

  while (click >= 4) {
  	p = (char *) ALIGN((vir_bytes) top + sizeof(char *) + len, click)
							+ BUGFIX;
	if (p > top && brk(p - BUGFIX) >= 0) break;
	click >>= 1;
  }
  if (click < 4) return(0);
  top = p - (BUGFIX + sizeof(char *));
  for (p = bottom; NEXT(p) != 0; p = (char *) (* (vir_bytes *) p & ~BUSY))
	;
  NEXT(p) = top;
  NEXT(top) = 0;
  return(1);
}

char *malloc(size)
unsigned size;
{
  register char *p, *next, *new;
  register unsigned len = ALIGN(size, sizeof(char *)) + sizeof(char *);

  if ((p = bottom) == 0) {
	top = bottom = p = sbrk(sizeof(char *));
	NEXT(p) = 0;
  }
  while ((next = NEXT(p)) != 0)
	if ((vir_bytes) next & BUSY)			/* already in use */
		p = (char *) ((vir_bytes) next & ~BUSY);
	else {
		while ((new = NEXT(next)) != 0 && !((vir_bytes) new & BUSY))
			next = new;
		if (next - p >= len) {			/* fits */
			if ((new = p + len) < next)	/* too big */
				NEXT(new) = next;
			NEXT(p) = (char *) ((vir_bytes) new | BUSY);
			return(p + sizeof(char *));
		}
		p = next;
	}
  return grow(len) ? malloc(size) : 0;
}

char *realloc(old, size)
char *old;
unsigned size;
{
  register char *p = old - sizeof(char *), *next, *new;
  register unsigned len = ALIGN(size, sizeof(char *)) + sizeof(char *), n;

  next = (char *) (* (vir_bytes *) p & ~BUSY);
  n = next - old;					/* old size */
  while ((new = NEXT(next)) != 0 && !((vir_bytes) new & BUSY))
	next = new;
  if (next - p >= len) {				/* does it still fit */
	if ((new = p + len) < next) {			/* even too big */
		NEXT(new) = next;
		NEXT(p) = (char *) ((vir_bytes) new | BUSY);
	}
	else
		NEXT(p) = (char *) ((vir_bytes) next | BUSY);
	return(old);
  }
  if ((new = malloc(size)) == 0)			/* it didn't fit */
	return(0);
  bcopy(old, new, n);					/* n < size */
  * (vir_bytes *) p &= ~BUSY;
  return(new);
}

free(p)
char *p;
{
  * (vir_bytes *) (p - sizeof(char *)) &= ~BUSY;
}
