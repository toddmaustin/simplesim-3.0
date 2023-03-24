/* exolex.l - EXO library lexor */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */


%{
/* C-style lexor. */

#include <stdio.h>
#include <stdlib.h>

#include "../host.h"
#include "../misc.h"
#include "libexo.h"

/* maximum size token (including strings) for lex */
#undef YYLMAX
#define YYLMAX	(16*1024)

/* disable interactive features, as they are non-portable */
#define YY_NEVER_INTERACTIVE	1

unsigned line = 1;		/* line of last recognized token */

int yywrap(void);
static void lex_eat_comment(void);

%}

%s BLOB_MODE
%p 3000

D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
C1			"/"
C2			"*"
P			(~|!|@|#|\$|%|\^|&|\*|-|\+|\||\/|\?)

%%

<BLOB_MODE>{H}{H}			{ return lex_byte; }

<INITIAL>{C1}{C2}			{ lex_eat_comment(); }

<INITIAL>[\-]?0[xX]{H}+			{ return lex_address; }
<INITIAL>[\-]?0{D}+			{ return lex_integer; }
<INITIAL>[\-]?{D}+			{ return lex_integer; }

<INITIAL>[\-]?{D}+{E}			{ return lex_float; }
<INITIAL>[\-]?{D}*"."{D}+({E})?		{ return lex_float; }
<INITIAL>[\-]?{D}+"."{D}*({E})?		{ return lex_float; }

<INITIAL>'(\\.|[^\\'])+'		{ return lex_char; }

<INITIAL>\"(\\.|[^\\"])*\" /*"*/	{ return lex_string; }

<INITIAL>{L}({L}|{D})*			{ return lex_token; }

<INITIAL>{P}+				{ return lex_token; }

<INITIAL>"{"				{ return '{'; }
<INITIAL>"}"				{ return '}'; }
<INITIAL>","				{ return ','; }
<INITIAL>"("				{ return '('; }
<INITIAL>")"				{ return ')'; }
<INITIAL>"["				{ return '['; }
<INITIAL>"]"				{ return ']'; }
<INITIAL>"<"				{ BEGIN(BLOB_MODE); return '<'; }
<BLOB_MODE>">"				{ BEGIN(INITIAL); return '>'; }

[ \t\v\f]				{ /* nada */; }
[\n]					{ line++; }
<<EOF>>					{ return lex_eof; }
.					{ /* bogus char */
					  fatal("bogus character in input"); }

%%

int
yywrap(void)
{
  return 1;
}

static void
lex_eat_comment(void)
{
  char c, c1;

loop:
  while ((c = input()) != '*' && c != 0)
    {
      if (c == '\n')
	line++;
    }

  if ((c1 = input()) != '/' && c1 != 0)
    {
      unput(c1);
      goto loop;
    }
}

int
yy_nextchar(void)
{
  int c;

  do {
    c = input();
  } while (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\n');
  unput(c);

  return c;
}

/* FIXME: this is a total KLUDGE (but well contained...) that I will someday
   address when I have copious amounts of free time... */
void
yy_setstream(FILE *stream)
{
  int i;
#define MAX_STREAMS	16
  static struct {
    FILE *stream;
    YY_BUFFER_STATE buffer;
  } streams[MAX_STREAMS] = { {NULL, NULL}, };
  static int num_streams = 0;
  static FILE *last_stream = NULL;

  /* same stream? */
  if (stream == last_stream)
    return;

  /* else, switch to new stream */
  for (i=0; i < num_streams; i++)
    {
      if (streams[i].stream == stream)
	{
	  yy_switch_to_buffer(streams[i].buffer);
	  return;
	}
    }

  /* hrmmm... not found, create a new buffer for this stream */
  if (num_streams == MAX_STREAMS)
    fatal("out of lex buffer streams, increase MAX_STREAMS");

  streams[num_streams].stream = stream;
  streams[num_streams].buffer = yy_create_buffer(stream, YY_BUF_SIZE);
  yy_switch_to_buffer(streams[num_streams].buffer);
  num_streams++;
}
