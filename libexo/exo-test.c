/* exo-test.c - EXO library test driver */

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


#include <stdio.h>
#include <stdlib.h>
#include "misc.h"
#include "options.h"
#include "libexo.h"

/* options database */
static struct opt_odb_t *odb;

/* dump help information */
static int help_me;

/* make internal defs */
static int make_defs;

/* EXO file to load */
static char *load_file;

/* EXO file to save */
static char *save_file;

/* print the EXO DB to stdout */
static int print_db;

/* EXO definition data base */
static struct exo_term_t *exo_db = NULL;

static void
usage(FILE *stream, int argc, char **argv)
{
  fprintf(stream, "Usage: %s {-options}\n", argv[0]);
  opt_print_help(odb, stream);
}

void
main(int argc, char **argv)
{
  /* build the command line options database */
  odb = opt_new(/* no orphan fn */NULL);
  opt_reg_flag(odb, "-h", "print help message",
               &help_me, /* default */FALSE, /* !print */FALSE, NULL);
  opt_reg_flag(odb, "-defs", "make internal defs",
               &make_defs, /* default */FALSE, /* print */TRUE, NULL);
  opt_reg_string(odb, "-load", "load an EXO file",
		 &load_file, /* default */NULL,
		 /* print */TRUE, /* format */NULL);
  opt_reg_string(odb, "-save", "save an EXO file",
		 &save_file, /* default */NULL,
		 /* print */TRUE, /* format */NULL);
  opt_reg_flag(odb, "-print", "print the EXO DB to stdout",
	       &print_db, /* default */FALSE,
	       /* print */TRUE, /* format */NULL);

  /* process the command line options */
  opt_process_options(odb, argc, argv);

  if (help_me)
    {
      /* print help message and exit */
      usage(stderr, argc, argv);
      exit(1);
    }

  /* print options used */
  opt_print_options(odb, stderr, /* short */TRUE, /* notes */TRUE);

  if (load_file)
    {
      ZFILE *exo_stream;
      struct exo_term_t *exo;

      exo_stream = myzfopen(load_file, "r");
      if (!exo_stream)
	fatal("could not open EXO file `%s'", load_file);

      while ((exo = exo_read(exo_stream->fd)) != NULL)
	exo_db = exo_chain(exo_db, exo);

      myzfclose(exo_stream);
    }

  if (make_defs)
    {
      struct exo_term_t *list, *array, *a, *b, *c, *d, *e, *f, *g, *h, *i;
      char *data = "This is a test to see if blobs really work...";
      char *data1 = "This is a test to see if blobs really work..."
	"This is a test to see if blobs really work..."
	  "This is a test to see if blobs really work..."
	    "This is a test to see if blobs really work..."
	      "This is a test to see if blobs really work..."
		"This is a test to see if blobs really work..."
		  "This is a test to see if blobs really work..."
		    "This is a test to see if blobs really work...";
      unsigned char data2[16] =
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

      exo_db =
	exo_chain(exo_db, exo_new(ec_string, "** basic types tests **"));
      exo_db = exo_chain(exo_db, a = exo_new(ec_integer, (exo_integer_t)42));
      exo_db = exo_chain(b = exo_new(ec_float, (exo_float_t)42.0), exo_db);
      exo_db = exo_chain(exo_db, c = exo_new(ec_char, (int)'x'));
      exo_db = exo_chain(exo_db, exo_new(ec_char, (int)'\n'));
      exo_db = exo_chain(exo_db, exo_new(ec_char, (int)'\b'));
      exo_db = exo_chain(exo_db, exo_new(ec_char, (int)'\x02'));
      exo_db = exo_chain(exo_db, exo_new(ec_char, (int)'\xab'));
      exo_db = exo_chain(exo_db, exo_new(ec_string, "this is a test..."));
      exo_db =
	exo_chain(exo_db, d = exo_new(ec_string, "this is\na test...\n"));
      exo_db = exo_chain(exo_db, exo_new(ec_string, "a test... <<\\\b>>\n"));

      exo_db = exo_chain(exo_db, exo_new(ec_string, "** deep copy tests **"));
      exo_db = exo_chain(exo_db, exo_deepcopy(d));
      exo_db = exo_chain(exo_db, exo_deepcopy(c));
      exo_db = exo_chain(exo_db, exo_deepcopy(b));
      exo_db = exo_chain(exo_db, exo_deepcopy(a));

      exo_db = exo_chain(exo_db, exo_new(ec_string, "** list tests **"));
      exo_db =
	exo_chain(exo_db, exo_new(ec_list,
				  exo_deepcopy(d), exo_deepcopy(c),
				  exo_deepcopy(b), exo_deepcopy(a), NULL));
      exo_db = exo_chain(exo_db, exo_new(ec_list, NULL));
      exo_db =
	exo_chain(exo_db, exo_new(ec_list,
				  exo_new(ec_list, NULL),
				  exo_new(ec_list, NULL),
				  exo_new(ec_list, exo_deepcopy(a), NULL),
				  NULL));
      list = exo_deepcopy(a);
      list = exo_chain(list, exo_deepcopy(b));
      list = exo_chain(list, exo_deepcopy(a));
      list = exo_chain(list, exo_deepcopy(b));
      list = exo_chain(exo_deepcopy(c), list);
      exo_db = exo_chain(exo_db, e = exo_new(ec_list, list, NULL));
      exo_db = exo_chain(exo_db, exo_new(ec_list,
					 exo_deepcopy(e),
					 exo_new(ec_list, NULL),
					 exo_deepcopy(e),
					 exo_deepcopy(a), NULL));

      exo_db = exo_chain(exo_db, exo_new(ec_string, "** array tests **"));
      exo_db = exo_chain(exo_db, exo_new(ec_array, 16, NULL));
      f = array = exo_new(ec_array, 16, NULL);
      EXO_ARR(array, 2) = exo_deepcopy(e);
      EXO_ARR(array, 3) = exo_deepcopy(a);
      EXO_ARR(array, 4) = exo_deepcopy(c);
      EXO_ARR(array, 6) = exo_deepcopy(EXO_ARR(array, 2));
      EXO_ARR(array, 7) = exo_deepcopy(EXO_ARR(array, 1));
      exo_db = exo_chain(exo_db, array);
      exo_db =
	exo_chain(exo_db, exo_new(ec_array, 4,
				  exo_deepcopy(a),
				  exo_deepcopy(e),
				  exo_deepcopy(c),
				  exo_deepcopy(f),
				  NULL));

      exo_db = exo_chain(exo_db, exo_new(ec_string, "** token tests **"));
#define SYM1		1
#define SYM2		2
      exo_intern_as("sym1", SYM1);
      exo_intern_as("sym2", SYM2);
      g = exo_new(ec_token, "sym1"),
      exo_db = exo_chain(exo_db,
			 exo_new(ec_list,
				 g,
				 exo_new(ec_integer,
					 (exo_integer_t)
					 g->as_token.ent->token),
				 NULL));
      h = exo_new(ec_token, "sym2"),
      exo_db = exo_chain(exo_db,
			 exo_new(ec_list,
				 h,
				 exo_new(ec_integer,
					 (exo_integer_t)
					 h->as_token.ent->token),
				 NULL));
      i = exo_new(ec_token, "sym3"),
      exo_db = exo_chain(exo_db,
			 exo_new(ec_list,
				 i,
				 exo_new(ec_integer,
					 (exo_integer_t)
					 i->as_token.ent->token),
				 NULL));

      /* das blobs */
      exo_db = exo_chain(exo_db, exo_new(ec_blob, strlen(data), data));
      exo_db = exo_chain(exo_db, exo_new(ec_blob, strlen(data1), data1));
      exo_db = exo_chain(exo_db, exo_new(ec_blob, sizeof(data2), data2));
    }

  if (print_db)
    {
      struct exo_term_t *exo;

      /* emit header comment */
      fprintf(stdout, "\n/* EXO DB */\n\n");
      fprintf(stdout,
	      "/* EXO save file, file format version %d.%d */\n\n",
	      EXO_FMT_MAJOR, EXO_FMT_MINOR);

      /* emit all defs */
      for (exo=exo_db; exo != NULL; exo=exo->next)
	{
	  exo_print(exo, stdout);
	  fprintf(stdout, "\n\n");
	}
    }

  if (save_file)
    {
      ZFILE *exo_stream;
      struct exo_term_t *exo;

      exo_stream = myzfopen(save_file, "w");
      if (!exo_stream)
	fatal("could not open EXO file `%s'", save_file);

      /* emit header comment */
      fprintf(exo_stream->fd,
	      "/* EXO save file, file format version %d.%d */\n\n",
	      EXO_FMT_MAJOR, EXO_FMT_MINOR);

      /* emit all defs */
      for (exo=exo_db; exo != NULL; exo=exo->next)
	{
	  exo_print(exo, exo_stream->fd);
	  fprintf(exo_stream->fd, "\n\n");
	}
      myzfclose(exo_stream);
    }
}
