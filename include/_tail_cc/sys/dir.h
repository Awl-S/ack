#ifdef BSD4_2
#include "/usr/include/sys/dir.h"
#else
#define DIRBLKSIZ 512
#define MAXNAMLEN 14
#undef DIRSIZ
#define DIRSIZ(dp) \
	((sizeof(struct direct) - (MAXNAMLEN+1)) + (((dp)->d_namlen+1+3)&~3))
struct direct {
	long	d_ino;
	short	d_reclen;
	short	d_namlen;
	char	d_name[MAXNAMLEN+1];
};

struct _dirdesc {
	int	dd_fd;
	long	dd_loc;
	long	dd_size;
	char	dd_buf[DIRBLKSIZ];
};

typedef struct _dirdesc DIR;

#ifndef NULL
#define NULL 0
#endif
extern DIR	*opendir();
extern struct direct *readdir();
extern long	telldir();
extern		seekdir();
#define rewinddir(dirp) seekdir((dirp), 0L)
extern		closedir();
#endif
