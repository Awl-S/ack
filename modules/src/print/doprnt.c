/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
/* $Header$ */

#include <system.h>
#include <varargs.h>
#include "param.h"

doprnt(fp, fmt, argp)
	File *fp;
	char *fmt;
	va_list argp;
{
	char buf[SSIZE];

	sys_write(fp, buf, _format(buf, fmt, argp));
}
