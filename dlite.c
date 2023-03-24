/* dlite.c - DLite, the lite debugger, routines */

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
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "version.h"
#include "eval.h"
#include "regs.h"
#include "memory.h"
#include "sim.h"
#include "symbol.h"
#include "loader.h"
#include "options.h"
#include "stats.h"
#include "range.h"
#include "dlite.h"

/* architected state accessors, initialized by dlite_init() */
static dlite_reg_obj_t f_dlite_reg_obj = NULL;
static dlite_mem_obj_t f_dlite_mem_obj = NULL;
static dlite_mstate_obj_t f_dlite_mstate_obj = NULL;

/* set non-zero to enter DLite after next instruction */
int dlite_active = FALSE;

/* non-zero to force a check for a break */
int dlite_check = FALSE;

/* set non-zero to exit DLite command loop */
static int dlite_return = FALSE;

/* size modifier mask bit definitions */
#define MOD_BYTE	0x0001		/* b - print a byte */
#define MOD_HALF	0x0002		/* h - print a half (short) */
#define MOD_WORD	0x0004		/* w - print a word */
#define MOD_QWORD	0x0008		/* q - print a qword */
#define MOD_FLOAT	0x0010		/* F - print a float */
#define MOD_DOUBLE	0x0020		/* f - print a double */
#define MOD_CHAR	0x0040		/* c - print a character */
#define MOD_STRING	0x0080		/* s - print a string */

#define MOD_SIZES							\
  (MOD_BYTE|MOD_HALF|MOD_WORD|MOD_QWORD					\
   |MOD_FLOAT|MOD_DOUBLE|MOD_CHAR|MOD_STRING)

/* format modifier mask bit definitions */
#define MOD_DECIMAL	0x0100		/* d - print in decimal format */
#define MOD_UNSIGNED	0x0200		/* u - print in unsigned format */
#define MOD_OCTAL	0x0400		/* o - print in octal format */
#define MOD_HEX		0x0800		/* x - print in hex format */
#define MOD_BINARY	0x1000		/* 1 - print in binary format */

#define MOD_FORMATS							\
  (MOD_DECIMAL|MOD_UNSIGNED|MOD_OCTAL|MOD_HEX|MOD_BINARY)


/* DLite modifier parser, transforms /<mods> strings to modifier mask */
static char *				/* error string, NULL for no err */
modifier_parser(char *p,		/* ptr to /<mods> string */
		char **endp,		/* ptr to first byte not consumed */
		int *pmod)		/* modifier mask written to *PMOD */
{
  int modifiers = 0;

  /* default modifiers */
  *pmod = 0;

  /* is this a valid modifier? */
  if (*p == '/')
    {
      p++;
      /* parse modifiers until end-of-string or whitespace is found */
      while (*p != '\0' && *p != '\n' && *p != ' ' && *p != '\t')
	{
	  switch (*p)
	    {
	    case 'b':
	      modifiers |= MOD_BYTE;
	      break;
	    case 'h':
	      modifiers |= MOD_HALF;
	      break;
	    case 'w':
	      modifiers |= MOD_WORD;
	      break;
	    case 'q':
	      modifiers |= MOD_QWORD;
	      break;
	    case 'd':
	      modifiers |= MOD_DECIMAL;
	      break;
	    case 'u':
	      modifiers |= MOD_UNSIGNED;
	      break;
	    case 'o':
	      modifiers |= MOD_OCTAL;
	      break;
	    case 'x':
	      modifiers |= MOD_HEX;
	      break;
	    case '1':
	      modifiers |= MOD_BINARY;
	      break;
	    case 'F':
	      modifiers |= MOD_FLOAT;
	      break;
	    case 'f':
	      modifiers |= MOD_DOUBLE;
	      break;
	    case 'c':
	      modifiers |= MOD_CHAR;
	      break;
	    case 's':
	      modifiers |= MOD_STRING;
	      break;
	    default:
	      return "bad modifier (use one or more of /bhwqduox1fdcs)";
	    }
	  p++;
	}
    }

  /* no error, return end of string and modifier mask */
  *endp = p;
  *pmod = modifiers;
  return NULL;
}

/* DLite default expression evaluator */
static struct eval_state_t *dlite_evaluator = NULL;
static struct regs_t *local_regs = NULL;

/* DLite identifier evaluator, used by the expression evaluator, returns
   the value of the ident in ES->TOK_BUF, sets eval_error to value other
   than ERR_NOERR if an error is encountered */
static struct eval_value_t			/* value of identifier */
ident_evaluator(struct eval_state_t *es)	/* expression evaluator */
{
  int i;
  char *err_str;
  struct eval_value_t val;
  struct stat_stat_t *stat;
  struct sym_sym_t *sym;
  static struct eval_value_t err_value = { et_int, { 0 } };

  /* is this a builtin register definition? */
  for (i=0; md_reg_names[i].str != NULL; i++)
    {
      if (!mystricmp(es->tok_buf, md_reg_names[i].str))
	{
	  err_str =
	    f_dlite_reg_obj(local_regs, /* !is_write */FALSE,
			    md_reg_names[i].file, md_reg_names[i].reg, &val);
	  if (err_str)
	    {
	      eval_error = ERR_UNDEFVAR;
	      val = err_value;
	    }
	  return val;
	}
    }

  /* else, try to locate a program symbol */
  sym_loadsyms(ld_prog_fname, /* load locals */TRUE);
  sym = sym_bind_name(es->tok_buf, NULL, sdb_any);
  if (sym)
    {
      /* found a symbol with this name, return it's (address) value */
      val.type = et_addr;
      val.value.as_addr = sym->addr;
      return val;
    }

  /* else, try to locate a statistical value symbol */
  stat = stat_find_stat(sim_sdb, es->tok_buf);
  if (stat)
    {
      /* found it, convert stat value to an eval_value_t value */
      switch (stat->sc)
	{
	case sc_int:
	  val.type = et_int;
	  val.value.as_int = *stat->variant.for_int.var;
	  break;
	case sc_uint:
	  val.type = et_uint;
	  val.value.as_uint = *stat->variant.for_uint.var;
	  break;
#ifdef HOST_HAS_QWORD
	case sc_qword:
	  val.type = et_qword;
	  val.value.as_qword = *stat->variant.for_qword.var;
	  break;
#endif /* HOST_HAS_QWORD */
	case sc_float:
	  val.type = et_float;
	  val.value.as_float = *stat->variant.for_float.var;
	  break;
	case sc_double:
	  val.type = et_double;
	  val.value.as_double = *stat->variant.for_double.var;
	  break;
	case sc_dist:
	case sc_sdist:
	  eval_error = ERR_BADEXPR;
	  val = err_value;
	  break;
	case sc_formula:
	  {
	    /* instantiate a new evaluator to avoid recursion problems */
	    struct eval_state_t *es = eval_new(ident_evaluator, sim_sdb);
	    char *endp;

	    val = eval_expr(es, stat->variant.for_formula.formula, &endp);
	    if (eval_error != ERR_NOERR || *endp != '\0')
	      {
		/* pass through eval_error */
		val = err_value;
	      }
	    /* else, use value returned */
	    eval_delete(es);
	  }
	  break;
	default:
	  panic("bogus stat class");
	}
      return val;
    }
  /* else, not found */

  /* else, this is a bogus symbol */
  eval_error = ERR_UNDEFVAR;
  val = err_value;
  return val;
}

/* maximum number of arguments that can passed to a dlite command handler */
#define MAX_ARGS	4

/* maximum length of a dlite command string argument */
#define MAX_STR		128

/* argument array entry, argument arrays are passed to command handlers */
union arg_val_t {
  int as_modifier;
  struct eval_value_t as_value;
  int as_access;
  char as_str[MAX_STR];
};

/* a DLite command handler function pointer */
typedef char *					/* err str, NULL for no err */
(*cmd_fn_t)(int nargs,				/* number of arguments */
	    union arg_val_t args[],		/* argument vector */
	    struct regs_t *regs,		/* registers to access */
	    struct mem_t *mem);			/* memory to access */

/* DLite command descriptor, fully describes a command supported by the
   DLite debugger command handler */
struct dlite_cmd_t {
  char *cmd_str;		/* DLite command string */
  char *arg_strs[MAX_ARGS];	/* NULL-terminated cmd args (? - optional):
				     m - size/type modifiers
				     a - address expression
				     c - count expression
				     e - any expression
				     s - any string
				     t - access type {r|w|x}
				     i - breakpoint id */
  cmd_fn_t cmd_fn;		/* implementing function */
  char *help_str;		/* DLite command help string */
};

/* forward handler decls */
static char *
dlite_help(int nargs, union arg_val_t args[],
	   struct regs_t *regs, struct mem_t *mem);
static char *
dlite_version(int nargs, union arg_val_t args[],
	      struct regs_t *regs, struct mem_t *mem);
static char *
dlite_terminate(int nargs, union arg_val_t args[],
		struct regs_t *regs, struct mem_t *mem);
static char *
dlite_quit(int nargs, union arg_val_t args[],
	   struct regs_t *regs, struct mem_t *mem);
static char *
dlite_cont(int nargs, union arg_val_t args[],
	   struct regs_t *regs, struct mem_t *mem);
static char *
dlite_step(int nargs, union arg_val_t args[],
	   struct regs_t *regs, struct mem_t *mem);
static char *
dlite_print(int nargs, union arg_val_t args[],
	    struct regs_t *regs, struct mem_t *mem);
static char *
dlite_options(int nargs, union arg_val_t args[],
	      struct regs_t *regs, struct mem_t *mem);
static char *
dlite_option(int nargs, union arg_val_t args[],
	     struct regs_t *regs, struct mem_t *mem);
static char *
dlite_stats(int nargs, union arg_val_t args[],
	    struct regs_t *regs, struct mem_t *mem);
static char *
dlite_stat(int nargs, union arg_val_t args[],
	   struct regs_t *regs, struct mem_t *mem);
static char *
dlite_whatis(int nargs, union arg_val_t args[],
	     struct regs_t *regs, struct mem_t *mem);
static char *
dlite_regs(int nargs, union arg_val_t args[],
	   struct regs_t *regs, struct mem_t *mem);
static char *
dlite_iregs(int nargs, union arg_val_t args[],
	    struct regs_t *regs, struct mem_t *mem);
static char *
dlite_fpregs(int nargs, union arg_val_t args[],
	     struct regs_t *regs, struct mem_t *mem);
static char *
dlite_cregs(int nargs, union arg_val_t args[],
	    struct regs_t *regs, struct mem_t *mem);
static char *
dlite_mstate(int nargs, union arg_val_t args[],
	     struct regs_t *regs, struct mem_t *mem);
static char *
dlite_display(int nargs, union arg_val_t args[],
	      struct regs_t *regs, struct mem_t *mem);
static char *
dlite_dump(int nargs, union arg_val_t args[],
	   struct regs_t *regs, struct mem_t *mem);
static char *
dlite_dis(int nargs, union arg_val_t args[],
	  struct regs_t *regs, struct mem_t *mem);
static char *
dlite_break(int nargs, union arg_val_t args[],
	    struct regs_t *regs, struct mem_t *mem);
static char *
dlite_dbreak(int nargs, union arg_val_t args[],
	     struct regs_t *regs, struct mem_t *mem);
static char *
dlite_rbreak(int nargs, union arg_val_t args[],
	     struct regs_t *regs, struct mem_t *mem);
static char *
dlite_breaks(int nargs, union arg_val_t args[],
	     struct regs_t *regs, struct mem_t *mem);
static char *
dlite_delete(int nargs, union arg_val_t args[],
	     struct regs_t *regs, struct mem_t *mem);
static char *
dlite_clear(int nargs, union arg_val_t args[],
	    struct regs_t *regs, struct mem_t *mem);
static char *
dlite_symbols(int nargs, union arg_val_t args[],
	      struct regs_t *regs, struct mem_t *mem);
static char *
dlite_tsymbols(int nargs, union arg_val_t args[],
	       struct regs_t *regs, struct mem_t *mem);
static char *
dlite_dsymbols(int nargs, union arg_val_t args[],
	       struct regs_t *regs, struct mem_t *mem);
static char *
dlite_symbol(int nargs, union arg_val_t args[],
	     struct regs_t *regs, struct mem_t *mem);

/* DLite debugger command parser command definitions, NOTE: optional
   arguments must be trailing arguments, otherwise the command parser will
   break (modifiers are an exception to this rule) */
static struct dlite_cmd_t cmd_db[] =
{
  { "help", { "s?", NULL }, dlite_help,
    "print command reference" },
  { "version", { NULL }, dlite_version,
    "print DLite version information" },
  { "terminate", { NULL }, dlite_terminate,
    "terminate the simulation with statistics" },
  { "quit", { NULL }, dlite_quit,
    "exit the simulator" },
  { "cont", { "a?", NULL }, dlite_cont,
    "continue program execution (optionally at <addr>)" },
  { "step", { NULL }, dlite_step,
    "step program one instruction" },
#if 0 /* NYI */
  { "next", { NULL }, dlite_next,
    "step program one instruction in current procedure" },
#endif
  { "print", { "m?", "e", NULL }, dlite_print,
    "print the value of <expr> using format <modifiers>" },
  { "options", { NULL }, dlite_options,
    "print the value of all options" },
  { "option", { "s", NULL }, dlite_option,
    "print the value of an option" },
  { "stats", { NULL }, dlite_stats,
    "print the value of all statistical variables" },
  { "stat", { "s", NULL }, dlite_stat,
    "print the value of a statistical variable" },
  { "whatis", { "e", NULL }, dlite_whatis,
    "print the type of expression <expr>" },
  { "---", { NULL }, NULL, NULL },
  { "regs", { NULL }, dlite_regs,
    "print all register contents" },
  { "iregs", { NULL }, dlite_iregs,
    "print integer register contents" },
  { "fpregs", { NULL }, dlite_fpregs,
    "print floating point register contents" },
  { "cregs", { NULL }, dlite_cregs,
    "print control register contents" },
  { "mstate", { "s?", NULL }, dlite_mstate,
    "print machine specific state (simulator dependent)" },
  { "display", { "m?", "a", NULL }, dlite_display,
    "display the value at memory location <addr> using format <modifiers>" },
  { "dump", { "a?", "c?", NULL }, dlite_dump,
    "dump memory at <addr> (optionally for <cnt> words)" },
  { "dis", { "a?", "c?", NULL }, dlite_dis,
    "disassemble instructions at <addr> (for <cnt> insts)" },
  { "break", { "a", NULL }, dlite_break,
    "set breakpoint at <addr>, returns <id> of breakpoint" },
  { "dbreak", { "a", "t?", NULL }, dlite_dbreak,
    "set data breakpoint at <addr> (for (r)ead, (w)rite,\n"
    "    and/or e(x)ecute, returns <id> of breakpoint" },
  { "rbreak", { "s", "t?", NULL }, dlite_rbreak,
    "set read/write/exec breakpoint at <range> (for (r)ead, (w)rite,\n"
    "    and/or e(x)ecute, returns <id> of breakpoint" },
  { "breaks", { NULL }, dlite_breaks,
    "list active code and data breakpoints" },
  { "delete", { "i", NULL }, dlite_delete,
    "delete breakpoint <id>" },
  { "clear", { NULL }, dlite_clear,
    "clear all breakpoints (code and data)" },
  { "---", { NULL }, NULL, NULL },
  { "symbols", { NULL }, dlite_symbols,
    "print the value of all program symbols" },
  { "tsymbols", { NULL }, dlite_tsymbols,
    "print the value of all program text symbols" },
  { "dsymbols", { NULL }, dlite_dsymbols,
    "print the value of all program data symbols" },
  { "symbol", { "s", NULL }, dlite_symbol,
    "print the value of a symbol" },
  { "---", { NULL }, NULL, NULL },
  /* list terminator */
  { NULL, { NULL }, NULL, NULL }
};

/* help command trailing text */
static char *dlite_help_tail =
  "Arguments <addr>, <cnt>, <expr>, and <id> are any legal expression:\n"
  "  <expr>    <-  <factor> +|- <expr>\n"
  "  <factor>  <-  <term> *|/ <factor>\n"
  "  <term>    <-  ( <expr> )\n"
  "                | - <term>\n"
  "                | <const>\n"
  "                | <symbol>\n"
  "                | <file:loc>\n"
  "\n"
  "Command modifiers <mods> are any of the following:\n"
  "\n"
  "  b - print a byte\n"
  "  h - print a half (short)\n"
  "  w - print a word (default)\n"
#ifdef HOST_HAS_QWORD
  "  q - print a qword\n"
#endif /* HOST_HAS_QWORD */
  "  F - print a float\n"
  "  f - print a double\n"
  "  c - print a character\n"
  "  s - print a string\n"
  "  d - print in decimal format (default)\n"
  "  u - print in unsigned decimal format\n"
  "  o - print in octal format\n"
  "  x - print in hex format\n"
  "  1 - print in binary format\n";


/* execute DLite command string CMD */
static char *					/* err str, NULL if no err */
dlite_exec(char *cmd_str,			/* command string */
	   struct regs_t *regs,			/* registers to access */
	   struct mem_t *mem)			/* memory to access */
{
  int i, arg_cnt;
  struct dlite_cmd_t *cmd;
  char cmd_buf[512], *p, *q, *endp;
  union arg_val_t args[MAX_ARGS];

  p = cmd_str;
  q = cmd_buf;

  /* skip any whitespace before argument */
  while (*p == ' ' || *p == '\t' || *p == '\n')
    p++;

  /* anything left? */
  if (*p == '\0')
    {
      /* NOP, no error */
      return NULL;
    }

  /* copy out command name string */
  while (*p != '\0' && *p != '\n' && *p != ' ' && *p != '\t' && *p != '/')
    *q++ = *p++;
  *q = '\0';

  /* find matching command */
  for (cmd=cmd_db; cmd->cmd_str != NULL; cmd++)
    {
      if (!strcmp(cmd->cmd_str, cmd_buf))
	break;
    }
  if (cmd->cmd_str == NULL)
    return "unknown command";

  /* match arguments for *CMD */
  for (i=0, arg_cnt=0; i<MAX_ARGS && cmd->arg_strs[i] != NULL; i++, arg_cnt++)
    {
      int optional, access, modifiers;
      char *arg, arg_type, *err_str;
      struct eval_value_t val;

      /* skip any whitespace before argument */
      while (*p == ' ' || *p == '\t' || *p == '\n')
	p++;

      arg = cmd->arg_strs[i];
      arg_type = arg[0];
      optional = (arg[1] == '?');

      if (*p == '\0')
	{
	  if (optional)
	    {
	      /* all arguments parsed */
	      break;
	    }
	  else
	    return "missing an argument";
	}

      endp = p;
      switch (arg_type)
	{
	case 'm':
	  err_str = modifier_parser(p, &endp, &modifiers);
	  if (err_str)
	    return err_str;
	  args[arg_cnt].as_modifier = modifiers;
	  break;
	case 'a':
	  local_regs = regs;
	  val = eval_expr(dlite_evaluator, p, &endp);
	  if (eval_error)
	    return eval_err_str[eval_error];
	  args[arg_cnt].as_value = val;
	  break;
	case 'c':
	  local_regs = regs;
	  val = eval_expr(dlite_evaluator, p, &endp);
	  if (eval_error)
	    return eval_err_str[eval_error];
	  args[arg_cnt].as_value = val;
	  break;
	case 'e':
	  local_regs = regs;
	  val = eval_expr(dlite_evaluator, p, &endp);
	  if (eval_error)
	    return eval_err_str[eval_error];
	  args[arg_cnt].as_value = val;
	  break;
	case 't':
	  access = 0;
	  while (*p != '\0' && *p != '\n' && *p != ' ' && *p != '\t')
	    {
	      switch (*p)
		{
		case 'r':
		  access |= ACCESS_READ;
		  break;
		case 'w':
		  access |= ACCESS_WRITE;
		  break;
		case 'x':
		  access |= ACCESS_EXEC;
		  break;
		default:
		  return "bad access type specifier (use r|w|x)";
		}
	      p++;
	    }
	  endp = p;
	  args[arg_cnt].as_access = access;
	  break;
	case 'i':
	  local_regs = regs;
	  val = eval_expr(dlite_evaluator, p, &endp);
	  if (eval_error)
	    return eval_err_str[eval_error];
	  args[arg_cnt].as_value = val;
	  break;
	case 's':
	  q = args[arg_cnt].as_str;
	  while (*p != ' ' && *p != '\t' && *p != '\0')
	    *q++ = *p++;
	  *q = '\0';
	  endp = p;
	  break;
	default:
	  panic("bogus argument type: `%c'", arg_type);
	}
      p = endp;
    }


  /* skip any whitespace before any trailing argument */
  while (*p == ' ' || *p == '\t' || *p == '\n')
    p++;

  /* check for any orphan arguments */
  if (*p != '\0')
    return "too many arguments";

  /* if we reach here, all arguments were parsed correctly, call handler */
  return cmd->cmd_fn(arg_cnt, args, regs, mem);
}

/* print expression value VAL using modifiers MODIFIERS */
static char *					/* err str, NULL for no err */
print_val(int modifiers,			/* print modifiers */
	  struct eval_value_t val)		/* expr value to print */
{
  char *format = "", *prefix = "", radix, buf[512];

  /* fill in any default size */
  if ((modifiers & MOD_SIZES) == 0)
    {
      /* compute default size */
      switch (val.type)
	{
	case et_int:	modifiers |= MOD_WORD; break;
	case et_uint:	modifiers |= MOD_WORD; break;
	case et_addr:
#ifdef HOST_HAS_QWORD
	  if (sizeof(md_addr_t) > 4)
	    modifiers |= MOD_QWORD;
	  else
#endif /* HOST_HAS_QWORD */
	    modifiers |= MOD_WORD;
	  break;
#ifdef HOST_HAS_QWORD
	case et_qword:	modifiers |= MOD_QWORD; break;
	case et_sqword:	modifiers |= MOD_QWORD; break;
#endif /* HOST_HAS_QWORD */
	case et_float:	modifiers |= MOD_FLOAT; break;
	case et_double:	modifiers |= MOD_DOUBLE; break;
	case et_symbol:
	default:	return "bad print value";
	}
    }
  if (((modifiers & MOD_SIZES) & ((modifiers & MOD_SIZES) - 1)) != 0)
    return "multiple size specifiers";

  /* fill in any default format */
  if ((modifiers & MOD_FORMATS) == 0)
    {
      /* compute default size */
      switch (val.type)
	{
	case et_int:	modifiers |= MOD_DECIMAL; break;
	case et_uint:	modifiers |= MOD_UNSIGNED; break;
	case et_addr:	modifiers |= MOD_HEX; break;
#ifdef HOST_HAS_QWORD
	case et_qword:	modifiers |= MOD_UNSIGNED; break;
	case et_sqword:	modifiers |= MOD_DECIMAL; break;
#endif /* HOST_HAS_QWORD */
	case et_float:	/* use default format */break;
	case et_double:	/* use default format */break;
	case et_symbol:
	default:	return "bad print value";
	}
    }
  if (((modifiers & MOD_FORMATS) & ((modifiers & MOD_FORMATS) - 1)) != 0)
    return "multiple format specifiers";

  /* decode modifiers */
  if (modifiers & (MOD_BYTE|MOD_HALF|MOD_WORD|MOD_QWORD))
    {
      if (modifiers & MOD_DECIMAL)
	radix = 'd';
      else if (modifiers & MOD_UNSIGNED)
	radix = 'u';
      else if (modifiers & MOD_OCTAL)
	radix = 'o';
      else if (modifiers & MOD_HEX)
	radix = 'x';
      else if (modifiers & MOD_BINARY)
	return "binary format not yet implemented";
      else
	panic("no default integer format");

      if (modifiers & MOD_BYTE)
	{
	  if (modifiers & MOD_OCTAL)
	    {
	      prefix = "0";
	      format = "03";
	    }
	  else if (modifiers & MOD_HEX)
	    {
	      prefix = "0x";
	      format = "02";
	    }
	  else
	    {
	      prefix = "";
	      format = "";
	    }

	  sprintf(buf, "%s%%%s%c", prefix, format, radix);
	  myfprintf(stdout, buf, eval_as_uint(val));
	}
      else if (modifiers & MOD_HALF)
	{
	  if (modifiers & MOD_OCTAL)
	    {
	      prefix = "0";
	      format = "06";
	    }
	  else if (modifiers & MOD_HEX)
	    {
	      prefix = "0x";
	      format = "04";
	    }
	  else
	    {
	      prefix = "";
	      format = "";
	    }

	  sprintf(buf, "%s%%%s%c", prefix, format, radix);
	  myfprintf(stdout, buf, eval_as_uint(val));
	}
      else if (modifiers & MOD_WORD)
	{
	  if (modifiers & MOD_OCTAL)
	    {
	      prefix = "0";
	      format = "011";
	    }
	  else if (modifiers & MOD_HEX)
	    {
	      prefix = "0x";
	      format = "08";
	    }
	  else
	    {
	      prefix = "";
	      format = "";
	    }

	  sprintf(buf, "%s%%%s%c", prefix, format, radix);
	  myfprintf(stdout, buf, eval_as_uint(val));
	}
#ifdef HOST_HAS_QWORD
      else if (modifiers & MOD_QWORD)
	{
	  if (modifiers & MOD_OCTAL)
	    {
	      prefix = "0";
	      format = "022";
	    }
	  else if (modifiers & MOD_HEX)
	    {
	      prefix = "0x";
	      format = "016";
	    }
	  else
	    {
	      prefix = "";
	      format = "";
	    }

	  sprintf(buf, "%s%%%sl%c", prefix, format, radix);
	  myfprintf(stdout, buf, eval_as_qword(val));
	}
#endif /* HOST_HAS_QWORD */
    }
  else if (modifiers & MOD_FLOAT)
    fprintf(stdout, "%f", (double)eval_as_float(val));
  else if (modifiers & MOD_DOUBLE)
    fprintf(stdout, "%f", eval_as_double(val));
  else if (modifiers & MOD_CHAR)
    fprintf(stdout, "`%c'", eval_as_uint(val));
  else if (modifiers & MOD_STRING)
    return "string format not yet implemented";
  else /* no format specified, default to value type format */
    panic("no default format");

  /* no error */
  return NULL;
}

/* default memory state accessor */
char *						/* err str, NULL for no err */
dlite_mem_obj(struct mem_t *mem,		/* memory space to access */
	      int is_write,			/* access type */
	      md_addr_t addr,			/* address to access */
	      char *p,				/* input/output buffer */
	      int nbytes)			/* size of access */
{
  enum mem_cmd cmd;

  if (!is_write)
    cmd = Read;
  else
    cmd = Write;

#if 0
  char *errstr;
  errstr = mem_valid(cmd, addr, nbytes, /* !declare */FALSE);
  if (errstr)
    return errstr;
#endif

  /* else, no error, access memory */
  mem_access(mem, cmd, addr, p, nbytes);

  /* no error */
  return NULL;
}

/* default machine state accessor */
char *						/* err str, NULL for no err */
dlite_mstate_obj(FILE *stream,			/* output stream */
		 char *cmd,			/* optional command string */
		 struct regs_t *regs,		/* registers to access */
		 struct mem_t *mem)		/* memory to access */
{
  /* nada */
  fprintf(stream, "No machine state.\n");

  /* no error */
  return NULL;
}

/* scroll terminator, wait for user to press return */
static void
dlite_pause(void)
{
  char buf[512];
  fprintf(stdout, "Press <return> to continue...");
  fflush(stdout);
  fgets(buf, 512, stdin);
}

/* print help information for DLite command CMD */
static void
print_help(struct dlite_cmd_t *cmd)		/* command to describe */
{
  int i;

  /* print command name */
  fprintf(stdout, "  %s ", cmd->cmd_str);

  /* print arguments of command */
  for (i=0; i < MAX_ARGS && cmd->arg_strs[i] != NULL; i++)
    {
      int optional;
      char *arg, arg_type;

      arg = cmd->arg_strs[i];
      arg_type = arg[0];
      optional = (arg[1] == '?');

      if (optional)
	fprintf(stdout, "{");
      else
	fprintf(stdout, "<");

      switch (arg_type)
	{
	case 'm':
	  fprintf(stdout, "/modifiers");
	  break;
	case 'a':
	  fprintf(stdout, "addr");
	  break;
	case 'c':
	  fprintf(stdout, "count");
	  break;
	case 'e':
	  fprintf(stdout, "expr");
	  break;
	case 't':
	  fprintf(stdout, "r|w|x");
	  break;
	case 'i':
	  fprintf(stdout, "id");
	  break;
	case 's':
	  fprintf(stdout, "string");
	  break;
	default:
	  panic("bogus argument type: `%c'", arg_type);
	}

      if (optional)
	fprintf(stdout, "}");
      else
	fprintf(stdout, ">");

      fprintf(stdout, " ");
    }
  fprintf(stdout, "\n");

  /* print command description */
  fprintf(stdout, "    %s\n", cmd->help_str);
}

/* print help messages for all (or single) DLite debugger commands */
static char *					/* err str, NULL for no err */
dlite_help(int nargs, union arg_val_t args[],	/* command arguments */
	   struct regs_t *regs,			/* registers to access */
	   struct mem_t *mem)			/* memory to access */
{
  struct dlite_cmd_t *cmd;

  if (nargs != 0 && nargs != 1)
    return "too many arguments";

  if (nargs == 1)
    {
      /* print help for specified commands */
      for (cmd=cmd_db; cmd->cmd_str != NULL; cmd++)
	{
	  if (!strcmp(cmd->cmd_str, args[0].as_str))
	    break;
	}
      if (!cmd->cmd_str)
	return "command unknown";

      print_help(cmd);
    }
  else
    {
      /* print help for all commands */
      for (cmd=cmd_db; cmd->cmd_str != NULL; cmd++)
	{
	  /* `---' specifies a good point for a scroll pause */
	  if (!strcmp(cmd->cmd_str, "---"))
	    dlite_pause();
	  else
	    print_help(cmd);
	}

      fprintf (stdout, "\n");
      if (dlite_help_tail)
	fprintf (stdout, "%s\n", dlite_help_tail);
    }

  /* no error */
  return NULL;
}

/* print version information for simulator */
static char *				 	/* err str, NULL for no err */
dlite_version(int nargs, union arg_val_t args[],/* command arguments */
	      struct regs_t *regs,		/* registers to access */
	      struct mem_t *mem)		/* memory to access */
{
  if (nargs != 0)
    return "too many arguments";

  /* print simulator version info */
  fprintf(stdout, "The SimpleScalar/%s Tool Set, version %d.%d of %s.\n",
	  VER_TARGET, VER_MAJOR, VER_MINOR, VER_UPDATE);
  fprintf(stdout,
    "Copyright (c) 1994-1998 by Todd M. Austin.  All Rights Reserved.\n");

  /* no error */
  return NULL;
}

/* terminate simulation with statistics */
static char *					/* err str, NULL for no err */
dlite_terminate(int nargs, union arg_val_t args[],/* command arguments */
		struct regs_t *regs,		/* registers to access */
		struct mem_t *mem)		/* memory to access */
{
  if (nargs != 0)
    return "too many arguments";

  fprintf(stdout, "DLite: terminating simulation...\n");
  longjmp(sim_exit_buf, /* exitcode */1);

  /* no error */
  return NULL;
}

/* quit the simulator, omit any stats dump */
static char *					/* err str, NULL for no err */
dlite_quit(int nargs, union arg_val_t args[],	/* command arguments */
	   struct regs_t *regs,			/* registers to access */
	   struct mem_t *mem)			/* memory to access */
{
  if (nargs != 0)
    return "too many arguments";

  fprintf(stdout, "DLite: exiting simulator...\n");
  exit(1);

  /* no error */
  return NULL;
}

/* continue executing program (possibly at specified address) */
static char *					/* err str, NULL for no err */
dlite_cont(int nargs, union arg_val_t args[],	/* command arguments */
	   struct regs_t *regs,			/* registers to access */
	   struct mem_t *mem)			/* memory to access */
{
  struct eval_value_t val;

  if (!f_dlite_reg_obj || !f_dlite_mem_obj)
    panic("DLite is not configured");

  if (nargs != 0 && nargs != 1)
    return "too many arguments";

  if (nargs == 1)
    {
      /* continue from specified address, check address */
      if (!EVAL_INTEGRAL(args[0].as_value.type))
	return "address argument must be an integral type";

      /* reset PC */
      val.type = et_addr;
      val.value.as_addr = eval_as_addr(args[0].as_value);
      f_dlite_reg_obj(regs, /* is_write */TRUE, rt_PC, 0, &val);

      myfprintf(stdout, "DLite: continuing execution @ 0x%08p...\n",
		val.value.as_addr);
    }

  /* signal end of main debugger loop, and continuation of prog execution */
  dlite_active = FALSE;
  dlite_return = TRUE;

  /* no error */
  return NULL;
}

/* step program one instruction */
static char *					/* err str, NULL for no err */
dlite_step(int nargs, union arg_val_t args[],	/* command arguments */
	   struct regs_t *regs,			/* registers to access */
	   struct mem_t *mem)			/* memory to access */
{
  if (nargs != 0)
    return "too many arguments";

  /* signal on instruction step */
  dlite_active = TRUE;
  dlite_return = TRUE;

  /* no error */
  return NULL;
}

#if 0 /* NYI */
/* step program one instruction in current procedure */
static char *					/* err str, NULL for no err */
dlite_next(int nargs, union arg_val_t args[])	/* command arguments */
{
  if (nargs != 0)
    return "too many arguments";

  /* signal on instruction step */
  dlite_step_cnt = 1;
  dlite_step_into = FALSE;

  /* no error */
  return NULL;
}
#endif

/* print the value of <expr> using format <modifiers> */
static char *				      	/* err str, NULL for no err */
dlite_print(int nargs, union arg_val_t args[],	/* command arguments */
	    struct regs_t *regs,		/* registers to access */
	    struct mem_t *mem)			/* memory to access */
{
  int modifiers = 0;
  char *err_str;
  struct eval_value_t val;

  if (nargs != 1 && nargs != 2)
    return "wrong number of arguments";

  if (nargs == 2)
    {
      /* arguments include modifiers and expression value */
      modifiers = args[0].as_modifier;
      val = args[1].as_value;
    }
  else
    {
      /* arguments include only expression value */
      val = args[0].as_value;
    }

  /* print expression value */
  err_str = print_val(modifiers, val);
  if (err_str)
    return err_str;
  fprintf(stdout, "\n");

  /* no error */
  return NULL;
}

/* print the value of all command line options */
static char *					/* err str, NULL for no err */
dlite_options(int nargs, union arg_val_t args[],/* command arguments */
	      struct regs_t *regs,		/* registers to access */
	      struct mem_t *mem)		/* memory to access */
{
  if (nargs != 0)
    return "wrong number of arguments";

  /* print all options */
  opt_print_options(sim_odb, stdout, /* terse */TRUE, /* !notes */FALSE);

  /* no error */
  return NULL;
}

/* print the value of all (or single) command line options */
static char *					/* err str, NULL for no err */
dlite_option(int nargs, union arg_val_t args[],	/* command arguments */
	     struct regs_t *regs,		/* registers to access */
	     struct mem_t *mem)			/* memory to access */
{
  struct opt_opt_t *opt;

  if (nargs != 1)
    return "wrong number of arguments";

  /* print a single option, specified by argument */
  opt = opt_find_option(sim_odb, args[0].as_str);
  if (!opt)
    return "option is not defined";

  /* else, print this option's value */
  fprintf(stdout, "%-16s  ", opt->name);
  opt_print_option(opt, stdout);
  if (opt->desc)
    fprintf(stdout, " # %s", opt->desc);
  fprintf(stdout, "\n");

  /* no error */
  return NULL;
}

/* print the value of all statistical variables */
static char *					/* err str, NULL for no err */
dlite_stats(int nargs, union arg_val_t args[],	/* command arguments */
	    struct regs_t *regs,		/* registers to access */
	    struct mem_t *mem)			/* memory to access */
{
  if (nargs != 0)
    return "wrong number of arguments";

  /* print all options */
  stat_print_stats(sim_sdb, stdout);
  sim_aux_stats(stdout);

  /* no error */
  return NULL;
}

/* print the value of a statistical variable */
static char *					/* err str, NULL for no err */
dlite_stat(int nargs, union arg_val_t args[],	/* command arguments */
	   struct regs_t *regs,			/* registers to access */
	   struct mem_t *mem)			/* memory to access */
{
  struct stat_stat_t *stat;

  if (nargs != 1)
    return "wrong number of arguments";

  /* print a single option, specified by argument */
  stat = stat_find_stat(sim_sdb, args[0].as_str);
  if (!stat)
    return "statistical variable is not defined";

  /* else, print this option's value */
  stat_print_stat(sim_sdb, stat, stdout);

  /* no error */
  return NULL;
}

/* print the type of expression <expr> */
static char *					/* err str, NULL for no err */
dlite_whatis(int nargs, union arg_val_t args[],	/* command arguments */
	     struct regs_t *regs,		/* registers to access */
	     struct mem_t *mem)			/* memory to access */
{
  if (nargs != 1)
    return "wrong number of arguments";

  fprintf(stdout, "type == `%s'\n", eval_type_str[args[0].as_value.type]);

  /* no error */
  return NULL;
}

/* print integer register contents */
static char *					/* err str, NULL for no err */
dlite_iregs(int nargs, union arg_val_t args[],	/* command arguments */
	    struct regs_t *regs,		/* registers to access */
	    struct mem_t *mem)			/* memory to access */
{
  char *err_str;

  if (nargs != 0)
    return "too many arguments";

  /* print integer registers */
  myfprintf(stdout, "PC: 0x%08p   NPC: 0x%08p\n",
	    regs->regs_PC, regs->regs_NPC);
  if ((err_str = dlite_cregs(nargs, args, regs, mem)) != NULL)
    return err_str;
  md_print_iregs(regs->regs_R, stdout);

  /* no error */
  return NULL;
}

/* print floating point register contents */
static char *					/* err str, NULL for no err */
dlite_fpregs(int nargs, union arg_val_t args[],	/* command arguments */
	     struct regs_t *regs,		/* registers to access */
	     struct mem_t *mem)			/* memory to access */
{
  /* print floating point registers */
  md_print_fpregs(regs->regs_F, stdout);

  /* no error */
  return NULL;
}

/* print floating point register contents */
static char *					/* err str, NULL for no err */
dlite_cregs(int nargs, union arg_val_t args[],	/* command arguments */
	    struct regs_t *regs,		/* registers to access */
	    struct mem_t *mem)			/* memory to access */
{
  /* print floating point registers */
  md_print_cregs(regs->regs_C, stdout);

  /* no error */
  return NULL;
}

/* print all register contents */
static char *					/* err str, NULL for no err */
dlite_regs(int nargs, union arg_val_t args[],	/* command arguments */
	   struct regs_t *regs,			/* registers to access */
	   struct mem_t *mem)			/* memory to access */
{
  char *err_str;

  myfprintf(stdout, "PC: 0x%08p   NPC: 0x%08p\n",
	    regs->regs_PC, regs->regs_NPC);
  if ((err_str = dlite_cregs(nargs, args, regs, mem)) != NULL)
    return err_str;
  md_print_iregs(regs->regs_R, stdout);
  dlite_pause();
  if ((err_str = dlite_fpregs(nargs, args, regs, mem)) != NULL)
    return err_str;

  /* no error */
  return NULL;
}

/* print machine specific state (simulator dependent) */
static char *					/* err str, NULL for no err */
dlite_mstate(int nargs, union arg_val_t args[],	/* command arguments */
	     struct regs_t *regs,		/* registers to access */
	     struct mem_t *mem)			/* memory to access */
{
  char *errstr;

  if (nargs != 0 && nargs != 1)
    return "too many arguments";

  if (f_dlite_mstate_obj)
    {
      if (nargs == 0)
	{
	  errstr = f_dlite_mstate_obj(stdout, NULL, regs, mem);
	  if (errstr)
	    return errstr;
	}
      else
	{
	  errstr = f_dlite_mstate_obj(stdout, args[0].as_str, regs, mem);
	  if (errstr)
	    return errstr;
	}
    }

  /* no error */
  return NULL;
}

/* display the value at memory location <addr> using format <modifiers> */
static char *					/* err str, NULL for no err */
dlite_display(int nargs, union arg_val_t args[],/* command arguments */
	      struct regs_t *regs,		/* registers to access */
	      struct mem_t *mem)		/* memory to access */
{
  int modifiers, size;
  md_addr_t addr;
  unsigned char buf[512];
  struct eval_value_t val;
  char *errstr;

  if (nargs != 1 && nargs != 2)
    return "wrong number of arguments";

  if (nargs == 1)
    {
      /* no modifiers */
      modifiers = 0;

      /* check address */
      if (!EVAL_INTEGRAL(args[0].as_value.type))
	return "address argument must be an integral type";

      /* reset address */
      addr = eval_as_addr(args[0].as_value);
    }
  else if (nargs == 2)
    {
      modifiers = args[0].as_modifier;

      /* check address */
      if (!EVAL_INTEGRAL(args[1].as_value.type))
	return "address argument must be an integral type";

      /* reset address */
      addr = eval_as_addr(args[1].as_value);
    }

  /* determine operand size */
  if (modifiers & (MOD_BYTE|MOD_CHAR))
    size = 1;
  else if (modifiers & MOD_HALF)
    size = 2;
  else if (modifiers & (MOD_QWORD|MOD_DOUBLE))
    size = 8;
  else /* no modifiers, or MOD_WORD|MOD_FLOAT */
    size = 4;

  /* read memory */
  errstr = f_dlite_mem_obj(mem, /* !is_write */FALSE, addr, (char *)buf, size);
  if (errstr)
    return errstr;

  /* marshall a value */
  if (modifiers & (MOD_BYTE|MOD_CHAR))
    {
      /* size == 1 */
      val.type = et_int;
      val.value.as_int = (int)*(unsigned char *)buf;
    }
  else if (modifiers & MOD_HALF)
    {
      /* size == 2 */
      val.type = et_int;
      val.value.as_int = (int)*(unsigned short *)buf;
    }
  else if (modifiers & (MOD_QWORD|MOD_DOUBLE))
    {
      /* size == 8 */
      val.type = et_double;
      val.value.as_double = *(double *)buf;
    }
  else /* no modifiers, or MOD_WORD|MOD_FLOAT */
    {
      /* size == 4 */
      val.type = et_uint;
      val.value.as_uint = *(unsigned int *)buf;
    }

  /* print the value */
  errstr = print_val(modifiers, val);
  if (errstr)
    return errstr;
  fprintf(stdout, "\n");

  /* no error */
  return NULL;
}

/* `dump' command print format */
#define BYTES_PER_LINE			16 /* must be a power of two */
#define LINES_PER_SCREEN		4

/* dump the contents of memory to screen */
static char *					/* err str, NULL for no err */
dlite_dump(int nargs, union arg_val_t args[],	/* command arguments */
	   struct regs_t *regs,			/* registers to access */
	   struct mem_t *mem)			/* memory to access */
{
  int i, j;
  int count = LINES_PER_SCREEN * BYTES_PER_LINE, i_count, fmt_count, fmt_lines;
  md_addr_t fmt_addr, i_addr;
  static md_addr_t addr = 0;
  unsigned char byte;
  char buf[512];
  char *errstr;

  if (nargs < 0 || nargs > 2)
    return "too many arguments";

  if (nargs == 1)
    {
      /* check address */
      if (!EVAL_INTEGRAL(args[0].as_value.type))
	return "address argument must be an integral type";

      /* reset PC */
      addr = eval_as_addr(args[0].as_value);
    }
  else if (nargs == 2)
    {
      /* check address */
      if (!EVAL_INTEGRAL(args[0].as_value.type))
	return "address argument must be an integral type";

      /* reset addr */
      addr = eval_as_addr(args[0].as_value);

      /* check count */
      if (!EVAL_INTEGRAL(args[1].as_value.type))
	return "count argument must be an integral type";

      if (eval_as_uint(args[1].as_value) > 1024)
	return "bad count argument";

      /* reset count */
      count = eval_as_uint(args[1].as_value);
    }
  /* else, nargs == 0, use addr, count */

  /* normalize start address and count */
  fmt_addr = addr & ~(BYTES_PER_LINE - 1);
  fmt_count = (count + (BYTES_PER_LINE - 1)) & ~(BYTES_PER_LINE - 1);
  fmt_lines = fmt_count / BYTES_PER_LINE;

  if (fmt_lines < 1)
    panic("no output lines");

  /* print dump */
  if (fmt_lines == 1)
    {
      /* unformatted dump */
      i_addr = fmt_addr;
      myfprintf(stdout, "0x%08p: ", i_addr);
      for (i=0; i < count; i++)
	{
	  errstr =
	    f_dlite_mem_obj(mem, /* !is_write */FALSE,
			    i_addr, (char *)&byte, 1);
	  if (errstr)
	    return errstr;
	  fprintf(stdout, "%02x ", byte);
	  if (isprint(byte))
	    buf[i] = byte;
	  else
	    buf[i] = '.';
	  i_addr++;
	  addr++;
	}
      buf[i] = '\0';

      /* character view */
      fprintf(stdout, "[%s]\n", buf);
    }
  else /* lines > 1 */
    {
      i_count = 0;
      i_addr = fmt_addr;
      for (i=0; i < fmt_lines; i++)
	{
	  myfprintf(stdout, "0x%08p: ", i_addr);

	  /* byte view */
	  for (j=0; j < BYTES_PER_LINE; j++)
	    {
	      if (i_addr >= addr && i_count <= count)
		{
		  errstr =
		    f_dlite_mem_obj(mem, /* !is_write */FALSE,
				    i_addr, (char *)&byte, 1);
		  if (errstr)
		    return errstr;
		  fprintf(stdout, "%02x ", byte);
		  if (isprint(byte))
		    buf[j] = byte;
		  else
		    buf[j] = '.';
		  i_count++;
		  addr++;
		}
	      else
		{
		  fprintf(stdout, "   ");
		  buf[j] = ' ';
		}
	      i_addr++;
	    }
	  buf[j] = '\0';

	  /* character view */
	  fprintf(stdout, "[%s]\n", buf);
	}
    }

  /* no error */
  return NULL;
}

/* disassembler print format */
#define INSTS_PER_SCREEN		16

/* disassemble instructions at specified address */
static char *					/* err str, NULL for no err */
dlite_dis(int nargs, union arg_val_t args[],	/* command arguments */
	  struct regs_t *regs,			/* registers to access */
	  struct mem_t *mem)			/* memory to access */
{
  int i;
  int count = INSTS_PER_SCREEN;
  static md_addr_t addr = 0;
  md_inst_t inst;
  char *errstr;

  if (nargs < 0 || nargs > 2)
    return "too many arguments";

  if (nargs == 1)
    {
      /* check address */
      if (!EVAL_INTEGRAL(args[0].as_value.type))
	return "address argument must be an integral type";

      /* reset PC */
      addr = eval_as_addr(args[0].as_value);
    }
  else if (nargs == 2)
    {
      /* check address */
      if (!EVAL_INTEGRAL(args[0].as_value.type))
	return "address argument must be an integral type";

      /* reset addr */
      addr = eval_as_addr(args[0].as_value);

      /* check count */
      if (!EVAL_INTEGRAL(args[0].as_value.type))
	return "count argument must be an integral type";

      /* reset count */
      count = eval_as_uint(args[1].as_value);

      if (count < 0 || count > 1024)
	return "bad count argument";
    }
  /* else, nargs == 0, use addr, count */

  if ((addr % sizeof(md_inst_t)) != 0)
    return "instruction addresses are a multiple of eight";

  /* disassemble COUNT insts at ADDR */
  for (i=0; i<count; i++)
    {
      /* read and disassemble instruction */
      myfprintf(stdout, "    0x%08p:   ", addr);
      errstr =
	f_dlite_mem_obj(mem, /* !is_write */FALSE,
			addr, (char *)&inst, sizeof(inst));
      inst = MD_SWAPI(inst);
      if (errstr)
	return errstr;
      md_print_insn(inst, addr, stdout);
      fprintf(stdout, "\n");

      /* go to next instruction */
      addr += sizeof(md_inst_t);
    }

  /* no error */
  return NULL;
}

/* break instance descriptor, one allocated for each breakpoint set */
struct dlite_break_t {
  struct dlite_break_t *next;	/* next active breakpoint */
  int id;			/* break id */
  int class;			/* break class */
  struct range_range_t range;	/* break range */
};

/* all active break points, in a list */
static struct dlite_break_t *dlite_bps = NULL;

/* unique id of next breakpoint */
static int break_id = 1;

/* return breakpoint class as a string */
static char *					/* breakpoint class string */
bp_class_str(int class)				/* breakpoint class mask */
{
  if (class == (ACCESS_READ|ACCESS_WRITE|ACCESS_EXEC))
    return "read|write|exec";
  else if (class == (ACCESS_READ|ACCESS_WRITE))
    return "read|write";
  else if (class == (ACCESS_WRITE|ACCESS_EXEC))
    return "write|exec";
  else if (class == (ACCESS_READ|ACCESS_EXEC))
    return "read|exec";
  else if (class == ACCESS_READ)
    return "read";
  else if (class == ACCESS_WRITE)
    return "write";
  else if (class == ACCESS_EXEC)
    return "exec";
  else
    panic("bogus access class");
}

/* set a breakpoint of class CLASS at address ADDR */
static char *					/* err str, NULL for no err */
set_break(int class,				/* break class, use ACCESS_* */
	  struct range_range_t *range)		/* range breakpoint */
{
  struct dlite_break_t *bp;

  /* add breakpoint to break list */
  bp = calloc(1, sizeof(struct dlite_break_t));
  if (!bp)
    fatal("out of virtual memory");

  bp->id = break_id++;
  bp->range = *range;
  bp->class = class;

  bp->next = dlite_bps;
  dlite_bps = bp;

  fprintf(stdout, "breakpoint #%d set @ ", bp->id);
  range_print_range(&bp->range, stdout);
  fprintf(stdout, ", class: %s\n", bp_class_str(class));

  /* a breakpoint is set now, check for a breakpoint */
  dlite_check = TRUE;

  /* no error */
  return NULL;
}

/* delete breakpoint with id ID */
static char *					/* err str, NULL for no err */
delete_break(int id)				/* id of brkpnt to delete */
{
  struct dlite_break_t *bp, *prev;

  if (!dlite_bps)
    return "no breakpoints set";

  for (bp=dlite_bps,prev=NULL; bp != NULL; prev=bp,bp=bp->next)
    {
      if (bp->id == id)
	break;
    }
  if (!bp)
    return "breakpoint not found";

  if (!prev)
    {
      /* head of list, unlink */
      dlite_bps = bp->next;
    }
  else
    {
      /* middle or end of list */
      prev->next = bp->next;
    }

  fprintf(stdout, "breakpoint #%d deleted @ ",  bp->id);
  range_print_range(&bp->range, stdout);
  fprintf(stdout, ", class: %s\n", bp_class_str(bp->class));

  bp->next = NULL;
  free(bp);

  if (!dlite_bps)
    {
      /* no breakpoints set, cancel checks */
      dlite_check = FALSE;
    }
  else
    {
      /* breakpoints are set, do checks */
      dlite_check = TRUE;
    }

  /* no error */
  return NULL;
}

/* this variable clues dlite_main() into why it was called */
static int break_access = 0;

/* internal break check interface */
int						/* non-zero if brkpt hit */
__check_break(md_addr_t next_PC,		/* address of next inst */
	      int access,			/* mem access of last inst */
	      md_addr_t addr,			/* mem addr of last inst */
	      counter_t icount,			/* instruction count */
	      counter_t cycle)			/* cycle count */
{
  struct dlite_break_t *bp;

  if (dlite_active)
    {
      /* single-stepping, break always */
      break_access = /* single step */0;
      return TRUE;
    }
  /* else, check for a breakpoint */

  for (bp=dlite_bps; bp != NULL; bp=bp->next)
    {
      switch (bp->range.start.ptype)
	{
	case pt_addr:
	  if ((bp->class & ACCESS_EXEC)
	      && !range_cmp_range(&bp->range, next_PC))
	    {
	      /* hit a code breakpoint */
	      myfprintf(stdout,
			"Stopping at code breakpoint #%d @ 0x%08p...\n",
			bp->id, next_PC);
	      break_access = ACCESS_EXEC;
	      return TRUE;
	    }
	  if ((bp->class & ACCESS_READ)
	      && ((access & ACCESS_READ)
		  && !range_cmp_range(&bp->range, addr)))
	    {
	      /* hit a read breakpoint */
	      myfprintf(stdout,
			"Stopping at read breakpoint #%d @ 0x%08p...\n",
			bp->id, addr);
	      break_access = ACCESS_READ;
	      return TRUE;
	    }
	  if ((bp->class & ACCESS_WRITE)
	      && ((access & ACCESS_WRITE)
		  && !range_cmp_range(&bp->range, addr)))
	    {
	      /* hit a write breakpoint */
	      myfprintf(stdout,
			"Stopping at write breakpoint #%d @ 0x%08p...\n",
			bp->id, addr);
	      break_access = ACCESS_WRITE;
	      return TRUE;
	    }
	  break;

	case pt_inst:
	  if (!range_cmp_range(&bp->range, icount))
	    {
	      /* hit a code breakpoint */
	      fprintf(stdout,
		      "Stopping at inst count breakpoint #%d @ %.0f...\n",
		      bp->id, (double)icount);
	      break_access = ACCESS_EXEC;
	      return TRUE;
	    }
	  break;

	case pt_cycle:
	  if (!range_cmp_range(&bp->range, cycle))
	    {
	      /* hit a code breakpoint */
	      fprintf(stdout,
		      "Stopping at cycle count breakpoint #%d @ %.0f...\n",
		      bp->id, (double)cycle);
	      break_access = ACCESS_EXEC;
	      return TRUE;
	    }
	  break;

	default:
	  panic("bogus range type");
	}


    }

  /* no matching breakpoint found */
  break_access = /* no break */0;
  return FALSE;
}

/* set a text breakpoint */
static char *					/* err str, NULL for no err */
dlite_break(int nargs, union arg_val_t args[],	/* command arguments */
	    struct regs_t *regs,		/* registers to access */
	    struct mem_t *mem)			/* memory to access */
{
  md_addr_t addr;
  struct range_range_t range;

  if (nargs != 1)
    return "wrong number of arguments";

  /* check address */
  if (!EVAL_INTEGRAL(args[0].as_value.type))
    return "address argument must be an integral type";

  /* reset addr */
  addr = eval_as_addr(args[0].as_value);

  /* build the range */
  range.start.ptype = pt_addr;
  range.start.pos = addr;
  range.end.ptype = pt_addr;
#ifdef TARGET_ALPHA
  /* need some extra space here, as functional have multiple entry points
     depending on if $GP needs to be loaded or not */
  range.end.pos = addr + 9;
#else /* !TARGET_ALPHA */
  range.end.pos = addr + 1;
#endif /* TARGET_ALPHA */

  /* set a code break point */
  return set_break(ACCESS_EXEC, &range);
}

/* set a data breakpoint at specified address */
static char *					/* err str, NULL for no err */
dlite_dbreak(int nargs, union arg_val_t args[],	/* command arguments */
	     struct regs_t *regs,		/* registers to access */
	     struct mem_t *mem)			/* memory to access */
{
  int access;
  md_addr_t addr;
  struct range_range_t range;

  if (nargs != 1 && nargs != 2)
    return "wrong number of arguments";

  if (nargs == 1)
    {
      /* check address */
      if (!EVAL_INTEGRAL(args[0].as_value.type))
	return "address argument must be an integral type";

      /* reset addr */
      addr = eval_as_addr(args[0].as_value);

      /* break on read or write */
      access = ACCESS_READ|ACCESS_WRITE;
    }
  else if (nargs == 2)
    {
      /* check address */
      if (!EVAL_INTEGRAL(args[0].as_value.type))
	return "address argument must be an integral type";

      /* reset addr */
      addr = eval_as_addr(args[0].as_value);

      /* get access */
      access = args[1].as_access;
    }

  /* build the range */
  range.start.ptype = pt_addr;
  range.start.pos = addr;
  range.end.ptype = pt_addr;
  range.end.pos = addr + 1;

  /* set the breakpoint */
  return set_break(access, &range);
}

/* set a breakpoint at specified range */
static char *					/* err str, NULL for no err */
dlite_rbreak(int nargs, union arg_val_t args[],	/* command arguments */
	     struct regs_t *regs,		/* registers to access */
	     struct mem_t *mem)			/* memory to access */
{
  int access;
  char *errstr;
  struct range_range_t range;

  if (nargs != 1 && nargs != 2)
    return "wrong number of arguments";

  if (nargs == 2)
    {
      /* get access */
      access = args[1].as_access;
    }
  else
    {
      /* break on read or write or exec */
      access = ACCESS_READ|ACCESS_WRITE|ACCESS_EXEC;
    }

  /* check range */
  errstr = range_parse_range(args[0].as_str, &range);
  if (errstr)
    return errstr;

  /* sanity checks for ranges */
  if (range.start.ptype != range.end.ptype)
    return "range endpoints are not of the same type";
  else if (range.start.pos > range.end.pos)
    return "range start is after range end";

  /* set the breakpoint */
  return set_break(access, &range);
}

/* list all outstanding breakpoints */
static char *					/* err str, NULL for no err */
dlite_breaks(int nargs, union arg_val_t args[],	/* command arguments */
	     struct regs_t *regs,		/* registers to access */
	     struct mem_t *mem)			/* memory to access */
{
  struct dlite_break_t *bp;

  if (!dlite_bps)
    {
      fprintf(stdout, "No active breakpoints.\n");

      /* no error */
      return NULL;
    }

  fprintf(stdout, "Active breakpoints:\n");
  for (bp=dlite_bps; bp != NULL; bp=bp->next)
    {
      fprintf(stdout, "  breakpoint #%d @ ",  bp->id);
      range_print_range(&bp->range, stdout);
      fprintf(stdout, ", class: %s\n", bp_class_str(bp->class));
    }

  /* no error */
  return NULL;
}

/* delete specified breakpoint */
static char *					/* err str, NULL for no err */
dlite_delete(int nargs, union arg_val_t args[],	/* command arguments */
	     struct regs_t *regs,		/* registers to access */
	     struct mem_t *mem)			/* memory to access */
{
  int id;

  if (nargs != 1)
    return "wrong number of arguments";

  /* check bp id */
  if (!EVAL_INTEGRAL(args[0].as_value.type))
    return "id must be an integral type";
  
  id = eval_as_uint(args[0].as_value);
  return delete_break(id);
}

/* clear all breakpoints */
static char *					/* err str, NULL for no err */
dlite_clear(int nargs, union arg_val_t args[],	/* command arguments */
	    struct regs_t *regs,		/* registers to access */
	    struct mem_t *mem)			/* memory to access */
{
  if (!dlite_bps)
    {
      fprintf(stdout, "No active breakpoints.\n");

      /* no error */
      return NULL;
    }

  while (dlite_bps != NULL)
    {
      /* delete first breakpoint */
      delete_break(dlite_bps->id);
    }
  fprintf(stdout, "All breakpoints cleared.\n");

  /* no error */
  return NULL;
}

/* print the value of all program symbols */
static char *					/* err str, NULL for no err */
dlite_symbols(int nargs, union arg_val_t args[],/* command arguments */
	      struct regs_t *regs,		/* registers to access */
	      struct mem_t *mem)		/* memory to access */
{
  int i;

  if (nargs != 0)
    return "wrong number of arguments";

  /* load symbols, if not already loaded */
  sym_loadsyms(ld_prog_fname, /* !locals */FALSE);

  /* print all symbol values */
  for (i=0; i<sym_nsyms; i++)
    sym_dumpsym(sym_syms[i], stdout);

  /* no error */
  return NULL;
}

/* print the value of all text symbols */
static char *					/* err str, NULL for no err */
dlite_tsymbols(int nargs, union arg_val_t args[],/* command arguments */
	       struct regs_t *regs,		/* registers to access */
	       struct mem_t *mem)		/* memory to access */
{
  int i;

  if (nargs != 0)
    return "wrong number of arguments";

  /* load symbols, if not already loaded */
  sym_loadsyms(ld_prog_fname, /* !locals */FALSE);

  /* print all symbol values */
  for (i=0; i<sym_ntextsyms; i++)
    sym_dumpsym(sym_textsyms[i], stdout);

  /* no error */
  return NULL;
}

/* print the value of all text symbols */
static char *					/* err str, NULL for no err */
dlite_dsymbols(int nargs, union arg_val_t args[],/* command arguments */
	       struct regs_t *regs,		/* registers to access */
	       struct mem_t *mem)		/* memory to access */
{
  int i;

  if (nargs != 0)
    return "wrong number of arguments";

  /* load symbols, if not already loaded */
  sym_loadsyms(ld_prog_fname, /* !locals */FALSE);

  /* print all symbol values */
  for (i=0; i<sym_ndatasyms; i++)
    sym_dumpsym(sym_datasyms[i], stdout);

  /* no error */
  return NULL;
}

/* print the value of all (or single) command line options */
static char *					/* err str, NULL for no err */
dlite_symbol(int nargs, union arg_val_t args[],	/* command arguments */
	     struct regs_t *regs,		/* registers to access */
	     struct mem_t *mem)			/* memory to access */
{
  int index;
  struct sym_sym_t *sym;

  if (nargs != 1)
    return "wrong number of arguments";

  /* load symbols, if not already loaded */
  sym_loadsyms(ld_prog_fname, /* !locals */FALSE);

  /* print a single option, specified by argument */
  sym = sym_bind_name(args[0].as_str, &index, sdb_any);
  if (!sym)
    return "symbol is not defined";

  /* else, print this symbols's value */
  sym_dumpsym(sym_syms_by_name[index], stdout);

  /* no error */
  return NULL;
}

/* initialize the DLite debugger */
void
dlite_init(dlite_reg_obj_t reg_obj,		/* register state object */
	   dlite_mem_obj_t mem_obj,		/* memory state object */
	   dlite_mstate_obj_t mstate_obj)	/* machine state object */
{
  /* architected state accessors */
  f_dlite_reg_obj = reg_obj;
  f_dlite_mem_obj = mem_obj;
  f_dlite_mstate_obj = mstate_obj;

  /* instantiate the expression evaluator */
  dlite_evaluator = eval_new(ident_evaluator, NULL);
}

/* print a mini-state header */
static void
dlite_status(md_addr_t regs_PC,			/* PC of just completed inst */
	     md_addr_t next_PC,			/* PC of next inst to exec */
	     counter_t cycle,			/* current cycle */
	     int dbreak,			/* last break a data break? */
	     struct regs_t *regs,		/* registers to access */
	     struct mem_t *mem)			/* memory to access */
{
  md_inst_t inst;
  char *errstr;

  if (dbreak)
    {
      fprintf(stdout, "\n");
      fprintf(stdout, "Instruction (now finished) that caused data break:\n");
      myfprintf(stdout, "[%10n] 0x%08p:    ", cycle, regs_PC);
      errstr =
	f_dlite_mem_obj(mem, /* !is_write */FALSE,
			regs_PC, (char *)&inst, sizeof(inst));
      inst = MD_SWAPI(inst);
      if (errstr)
	fprintf(stdout, "<invalid memory>: %s", errstr);
      else
	md_print_insn(inst, regs_PC, stdout);
      fprintf(stdout, "\n");
      fprintf(stdout, "\n");
    }

  /* read and disassemble instruction */
  myfprintf(stdout, "[%10n] 0x%08p:    ", cycle, next_PC);
  errstr =
    f_dlite_mem_obj(mem, /* !is_write */FALSE,
		    next_PC, (char *)&inst, sizeof(inst));
  inst = MD_SWAPI(inst);
  if (errstr)
    fprintf(stdout, "<invalid memory>: %s", errstr);
  else
    md_print_insn(inst, next_PC, stdout);
  fprintf(stdout, "\n");
}

/* DLite command line prompt */
#define DLITE_PROMPT		"DLite! > "

/* DLite debugger main loop */
void
dlite_main(md_addr_t regs_PC,			/* addr of last inst to exec */
	   md_addr_t next_PC,			/* addr of next inst to exec */
	   counter_t cycle,			/* current procesor cycle */
	   struct regs_t *regs,			/* registers to access */
	   struct mem_t *mem)			/* memory to access */
{
  char buf[512], *err_str;
  int dbreak = (break_access & (ACCESS_READ|ACCESS_WRITE)) != 0;
  static char cmd[512] = "";

  dlite_active = TRUE;
  dlite_return = FALSE;
  dlite_status(regs_PC, next_PC, cycle, dbreak, regs, mem);

  while (dlite_active && !dlite_return)
    {
      fprintf(stdout, DLITE_PROMPT);
      fflush(stdout);
      fgets(buf, 512, stdin);

      /* chop */
      if (buf[strlen(buf)-1] == '\n')
	buf[strlen(buf)-1] = '\0';

      if (buf[0] != '\0')
	{
	  /* use this command */
	  strcpy(cmd, buf);
	}
      /* else, use last command */

      err_str = dlite_exec(cmd, regs, mem);
      if (err_str)
	fprintf(stdout, "Dlite: error: %s\n", err_str);
    }
}
