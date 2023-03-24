/* dlite.h - DLite, the lite debugger, interfaces */

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


/*
 * This module implements DLite, the lite debugger.  DLite is a very light
 * weight semi-symbolic debbugger that can interface to any simulator with
 * only a few function calls.  See sim-safe.c for an example of how to
 * interface DLite to a simulator.
 *
 * The following commands are supported by DLite: 
 *

 *
 * help			 - print command reference
 * version		 - print DLite version information
 * terminate		 - terminate the simulation with statistics
 * quit			 - exit the simulator
 * cont {<addr>}	 - continue program execution (optionally at <addr>)
 * step			 - step program one instruction
 * next			 - step program one instruction in current procedure
 * print <expr>		 - print the value of <expr>
 * regs			 - print register contents
 * mstate		 - print machine specific state (simulator dependent)
 * display/<mods> <addr> - display the value at <addr> using format <modifiers>
 * dump {<addr>} {<cnt>} - dump memory at <addr> (optionally for <cnt> words)
 * dis <addr> {<cnt>}	 - disassemble instructions at <addr> (for <cnt> insts)
 * break <addr>		 - set breakpoint at <addr>, returns <id> of breakpoint
 * dbreak <addr> {r|w|x} - set data breakpoint at <addr> (for (r)ead, (w)rite,
 *			   and/or e(x)ecute, returns <id> of breakpoint
 * breaks		 - list active code and data breakpoints
 * delete <id>		 - delete breakpoint <id>
 * clear		 - clear all breakpoints (code and data)
 *
 * ** command args <addr>, <cnt>, <expr>, and <id> are any legal expression:
 *
 * <expr>		<- <factor> +|- <expr>
 * <factor>		<- <term> *|/ <factor>
 * <term>		<- ( <expr> )
 *			   | - <term>
 *			   | <const>
 *			   | <symbol>
 *			   | <file:loc>
 *
 * ** command modifiers <mods> are any of the following:
 *
 * b - print a byte
 * h - print a half (short)
 * w - print a word
 * q - print a qword
 * t - print in decimal format
 * u - print in unsigned decimal format
 * o - print in octal format
 * x - print in hex format
 * 1 - print in binary format
 * f - print a float
 * d - print a double
 * c - print a character
 * s - print a string
 */

#ifndef DLITE_H
#define DLITE_H

#include <stdio.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "eval.h"

/* DLite register access function, the debugger uses this function to access
   simulator register state */
typedef char *					/* error str, NULL if none */
(*dlite_reg_obj_t)(struct regs_t *regs,		/* registers to access */
		   int is_write,		/* access type */
		   enum md_reg_type rt,		/* reg bank to access */
		   int reg,			/* register number */
		   struct eval_value_t *val);	/* input, output */

/* DLite memory access function, the debugger uses this function to access
   simulator memory state */
typedef char *					/* error str, NULL if none */
(*dlite_mem_obj_t)(struct mem_t *mem,		/* memory space to access */
		   int is_write,		/* access type */
		   md_addr_t addr,		/* address to access */
		   char *p,			/* input/output buffer */
		   int nbytes);			/* size of access */

/* DLite memory access function, the debugger uses this function to display
   the state of machine-specific state */
typedef char *					/* error str, NULL if none */
(*dlite_mstate_obj_t)(FILE *stream,		/* output stream */
		      char *cmd,		/* optional command string */
		      struct regs_t *regs,	/* registers to access */
		      struct mem_t *mem);	/* memory space to access */

/* initialize the DLite debugger */
void
dlite_init(dlite_reg_obj_t reg_obj,		/* register state object */
	   dlite_mem_obj_t mem_obj,		/* memory state object */
	   dlite_mstate_obj_t mstate_obj);	/* machine state object */

/*
 * default architected/machine state accessors
 */

/* default architected memory state accessor */
char *						/* err str, NULL for no err */
dlite_mem_obj(struct mem_t *mem,		/* memory space to access */
	      int is_write,			/* access type */
	      md_addr_t addr,			/* address to access */
	      char *p,				/* input, output */
	      int nbytes);			/* size of access */

/* default architected machine-specific state accessor */
char *						/* err str, NULL for no err */
dlite_mstate_obj(FILE *stream,			/* output stream */
		 char *cmd,			/* optional command string */
		 struct regs_t *regs,		/* registers to access */
		 struct mem_t *mem);		/* memory space to access */

/* state access masks */
#define ACCESS_READ	0x01			/* read access allowed */
#define ACCESS_WRITE	0x02			/* write access allowed */
#define ACCESS_EXEC	0x04			/* execute access allowed */

/* non-zero iff one breakpoint is set, for fast break check */
extern md_addr_t dlite_fastbreak /* = 0 */;

/* set non-zero to enter DLite after next instruction */
extern int dlite_active /* = FALSE */;

/* non-zero to force a check for a break */
extern int dlite_check /* = FALSE */;

/* internal break check interface */
int						/* non-zero if brkpt hit */
__check_break(md_addr_t next_PC,		/* address of next inst */
	      int access,			/* mem access of last inst */
	      md_addr_t addr,			/* mem addr of last inst */
	      counter_t icount,			/* instruction count */
	      counter_t cycle);			/* cycle count */

/* check for a break condition */
#define dlite_check_break(NPC, ACCESS, ADDR, ICNT, CYCLE)		\
  ((dlite_check || dlite_active)					\
   ? __check_break((NPC), (ACCESS), (ADDR), (ICNT), (CYCLE))		\
   : FALSE)

/* DLite debugger main loop */
void
dlite_main(md_addr_t regs_PC,			/* addr of last inst to exec */
	   md_addr_t next_PC,			/* addr of next inst to exec */
	   counter_t cycle,			/* current processor cycle */
	   struct regs_t *regs,			/* registers to access */
	   struct mem_t *mem);			/* memory to access */

#endif /* DLITE_H */
