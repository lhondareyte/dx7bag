/*
 *	dx7getb.c - get a bank dump from a DX7
 *	AYM 2002-07-21
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


#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "common.h"
#include "version.h"


#define ch 0
const char self[] = "dx7getb";


static void usage (void);
static int emit (FILE *fp, ...);
static void dbg (size_t address, int state, const char *fmt, ...);


static int debug = 0;


int main (int argc, char *argv[])
{
  int passive = 0;
#ifdef __FreeBSD__
  const char *midi_pathname = "/dev/umidi0.0";
#else
  const char *midi_pathname = "/dev/midi";
#endif
  FILE *midi_fp = NULL;

  {
    int g;

    if (argc == 2 && strcmp (argv[1], "--help") == 0)
    {
      usage ();
      exit (0);
    }

    if (argc == 2 && strcmp (argv[1], "--version") == 0)
    {
      puts (version);
      exit (0);
    }

    while ((g = getopt (argc, argv, "dm:p")) != EOF)
    {
      if (g == 'd')
	debug = 1;
      else if (g == 'm')
	midi_pathname = optarg;
      else if (g == 'p')
	passive = 1;
      else
	exit (1);
    }
  }

  if (isatty (fileno (stdout)))
  {
    err ("not writing MIDI data to a terminal");
    exit (1);
  }

  midi_fp = fopen (midi_pathname, "r+b");
  if (midi_fp == NULL)
  {
    err ("%s: %s", midi_pathname, strerror (errno));
    exit (1);
  }
  if (setvbuf (midi_fp, NULL, _IONBF, 0) != 0)
  {
    err ("%s: %s", midi_pathname, strerror (errno));
    exit (1);
  }

  if (! passive)
  {
    /* [function] [14] => "battery check" should appear */
    emit (midi_fp, 0xf0, 0x43, 0x10 + ch, 0x08, 0x27, 0x7f, -1);
    emit (midi_fp, 0xf0, 0x43, 0x10 + ch, 0x08, 0x27, 0x00, -1);
    usleep (10000);
    emit (midi_fp, 0xf0, 0x43, 0x10 + ch, 0x08, 0x0d, 0x7f, -1);
    emit (midi_fp, 0xf0, 0x43, 0x10 + ch, 0x08, 0x0d, 0x00, -1);
    usleep (10000);

    /* [8] => "midi ch=" should appear */
    emit (midi_fp, 0xf0, 0x43, 0x10 + ch, 0x08, 0x07, 0x7f, -1);
    emit (midi_fp, 0xf0, 0x43, 0x10 + ch, 0x08, 0x07, 0x00, -1);
    usleep (10000);

    /* [8] => "sys info [un]avail" should appear */
    emit (midi_fp, 0xf0, 0x43, 0x10 + ch, 0x08, 0x07, 0x7f, -1);
    emit (midi_fp, 0xf0, 0x43, 0x10 + ch, 0x08, 0x07, 0x00, -1);
    usleep (10000);

    /* [yes] => "sys info avail" */
    emit (midi_fp, 0xf0, 0x43, 0x10 + ch, 0x08, 0x29, 0x7f, -1);
    emit (midi_fp, 0xf0, 0x43, 0x10 + ch, 0x08, 0x29, 0x00, -1);
    usleep (10000);

    /* [8] => "midi transmit ?" should appear */
    emit (midi_fp, 0xf0, 0x43, 0x10 + ch, 0x08, 0x07, 0x7f, -1);
    emit (midi_fp, 0xf0, 0x43, 0x10 + ch, 0x08, 0x07, 0x00, -1);
    usleep (10000);

    /* [yes] => get the DX7 to send a bank dump */
    emit (midi_fp, 0xf0, 0x43, 0x10 + ch, 0x08, 0x29, 0x7f, -1);
  }

  {
    typedef enum
    {
      CANY,				/* Any byte */
      CSTATUS,				/* A status byte */
      CDATA				/* A data byte */
    } trans_type_t;

    typedef enum
    {
      SILENT = 0x01,			/* If bad value, silently roll back */
      RCKSUM = 0x02,			/* No value, must match checksum */
      WCKSUM = 0x04,			/* Counts towards checksum */
      ANY    = 0x08,			/* No value, value member is a count */
    } trans_flag_t;

    typedef struct
    {
      trans_type_t  type;
      trans_flag_t  flags;
      int           value;
      const char   *errstr;
    } transition_t;

    static transition_t transitions[] =
    {
      { CANY,    SILENT,       0xf0,      NULL },
      { CDATA,   SILENT,       0x43,      NULL },
      { CDATA,   SILENT,       0x00 + ch, NULL },
      { CDATA,   SILENT,       0x09,      NULL },
      { CDATA,   0,            0x20,      "byte count is not 4096" }, 
      { CDATA,   0,            0x00,      "byte count is not 4096" },
      { CDATA,   WCKSUM | ANY, 4096,      NULL },
      { CDATA,   RCKSUM,       0,         "bad checksum" },
      {	CSTATUS, 0,            0xf7,      "EOX expected" }
    };

    int state = 0;
    unsigned char buf[4104];
    int c;
    size_t buflen = 0;
    int datacount = 0;
    int datasum = 0;
    int havebyte = 0;
    int success = 0;

#define FAILURE								\
  do									\
  {									\
    buflen = 0;								\
    state = 0;								\
    havebyte = 1;							\
  }									\
  while (0)

    dbg (buflen, state, "\n");
    for (;;)
    {
      const transition_t *t;

      if (state == 0)
      {
	buflen = 0;
	datasum = 0;
	datacount = 0;
      }

      if (! havebyte)
        c = getc (midi_fp);
      havebyte = 0;
      dbg (buflen, state, " %02X", c);

      /* Real time messages are cheerfully ignored.
         Including RESET which maybe is a bit off. */
      if (c >= 0xf8)
      {
	dbg (buflen, state, " ign");
	continue;
      }

      t = transitions + state;

      if (c == EOF)
      {
	err ("unexpected EOF");
	break;
      }

      /* Check the class. Class errors are always fatal. */
      if (t->type == CDATA && (c < 0x00 || c > 0x7f))
      {
	err ("expected a data byte");
	break;
      }
      else if (t->type == CSTATUS && (c < 0x80 || c > 0xff))
      {
	err ("expected a status byte");
	break;
      }

      /* Check the value */
      if (! (t->flags & ANY))
      {
	if (t->flags & RCKSUM)
	{
	  if (c != (- datasum & 0x7f))
	  {
	    err ("bad checksum");
	    break;
	  }
	}
	else
	{
	  if (c != t->value)
	  {
	    if (t->flags & SILENT)
	    {
	      if (state != 0)
		havebyte = 1;
	      state = 0;
	      buflen = 0;
	      dbg (buflen, state, " abort\n");
	      continue;
	    }
	    else
	    {
	      if (t->errstr != NULL)
		err ("%s", t->errstr);
	      else
		err ("expected %02Xh, got %02Xh", t->value, c);
	      break;
	    }
	  }
	}
      }

      /* Update the checksum */
      if (t->flags & WCKSUM)
	datasum += c;
       
      /* Add to the buffer */
      if (buflen >= sizeof buf)
      {
	bug ("dump buffer overflow in state %d", state);
	exit (1);
      }
      buf[buflen++] = c;

      /* Change state */
      if (t->flags & ANY)
      {
	datacount++;
	//fprintf (stderr, "\r%d", datacount);
	if (datacount >= t->value)
	{
	  state++;
	  dbg (buflen, state, " count\n");
	}
	else if (datacount % 16 == 0)
	  dbg (buflen, state, "\n");
      }
      else
      {
	state++;
	dbg (buflen, state, " OK\n", state);
      }
      if ((size_t) state >= sizeof transitions / sizeof *transitions)
      {
	success = 1;
	break;
      }
    }

    /* Write the SYSEX message to file */
    if (success)
    {
      /* Sanity checks */
      if (state != sizeof transitions / sizeof *transitions)
      {
	bug ("success in state %d", state);
	exit (1);
      }
      if (buflen != sizeof buf/ sizeof *buf)
      {
	bug ("less than %d bytes in buffer at end of message",
	    (int) (sizeof buf / sizeof *buf));
	exit (1);
      }

      if (fwrite (buf, buflen, 1, stdout) != 1)
      {
	err ("(stdout): write error");
	exit (1);
      }
      if (fflush (stdout) != 0)
      {
	err ("(stdout): %s", strerror (errno));
	exit (1);
      }
    }
  }

  /* Now that the DX7 has replied, release the button. The DX7
     gets confused when it gets the button release message
     during the dump. */
  emit (midi_fp, 0xf0, 0x43, 0x10 + ch, 0x08, 0x29, 0x00, -1);

  return 0;
}


static void usage (void)
{
  fputs ("Usage:\n"
"  dx7getb --help\n"
"  dx7getb --version\n"
"  dx7getb [-dp] [-m file]\n"
"Options:\n"
"  -d         Debug mode\n"
"  --help     Print usage to standard output and exit successfully\n"
"  -m file    Use file as MIDI I/O device (default /dev/midi)\n"
"  -p         Passive mode (wait for bank dump to arrive)\n"
"  --version  Print version number to standard output and exit successfully\n",
    stdout);
}


static int emit (FILE *fp, ...)
{
  va_list argp;
  int byte;

  va_start (argp, fp);
  while ((byte = va_arg (argp, int)) >= 0)
    fputc (byte, fp);
  va_end (argp);
  return fflush (fp);
}


static void dbg (size_t address, int state, const char *fmt, ...)
{
  va_list argp;

  if (! debug)
    return;
  fflush (stdout);
  va_start (argp, fmt);
  vfprintf (stderr, fmt, argp);
  va_end (argp);
  if (strlen (fmt) >= 1 && fmt[strlen (fmt) - 1] == '\n')
    fprintf (stderr, "a=%04lu s=%01d", (unsigned long) address, state);
}


