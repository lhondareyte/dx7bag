/*
 *	common.c - stuff shared by dx7bag programs
 *	AYM 2002-07-26
 */


/*
This file is copyright André Majorel 2002.

This program is free software; you can redistribute it and/or modify it under
the terms of version 2 of the GNU General Public License as published by the
Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307, USA.
*/


#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "common.h"


void err (const char *fmt, ...)
{
  va_list argp;

  fflush (stdout);
  fprintf (stderr, "%s: ", self);
  va_start (argp, fmt);
  vfprintf (stderr, fmt, argp);
  va_end (argp);
  fputc ('\n', stderr);
}


void bug (const char *fmt, ...)
{
  va_list argp;

  fflush (stdout);
  fprintf (stderr, "%s: internal error: ", self);
  va_start (argp, fmt);
  vfprintf (stderr, fmt, argp);
  va_end (argp);
  fputc ('\n', stderr);
  fprintf (stderr, "%s: report this bug to the maintainer\n", self);
}

