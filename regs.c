/* regs.c - architected registers state routines */

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

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "loader.h"
#include "regs.h"


/* create a register file */
struct regs_t *
regs_create(void)
{
  struct regs_t *regs;

  regs = calloc(1, sizeof(struct regs_t));
  if (!regs)
    fatal("out of virtual memory");

  return regs;
}

/* initialize architected register state */
void
regs_init(struct regs_t *regs)		/* register file to initialize */
{
  /* FIXME: assuming all entries should be zero... */
  memset(regs, 0, sizeof(*regs));

  /* regs->regs_R[MD_SP_INDEX] and regs->regs_PC initialized by loader... */
}




#if 0

/* floating point register file format */
union regs_FP_t {
    md_gpr_t l[MD_NUM_FREGS];			/* integer word view */
    md_SS_FLOAT_TYPE f[SS_NUM_REGS];		/* single-precision FP view */
    SS_DOUBLE_TYPE d[SS_NUM_REGS/2];		/* double-precision FP view */
};

/* floating point register file */
extern union md_regs_FP_t regs_F;

/* (signed) hi register, holds mult/div results */
extern SS_WORD_TYPE regs_HI;

/* (signed) lo register, holds mult/div results */
extern SS_WORD_TYPE regs_LO;

/* floating point condition codes */
extern int regs_FCC;

/* program counter */
extern SS_ADDR_TYPE regs_PC;

/* dump all architected register state values to output stream STREAM */
void
regs_dump(FILE *stream)		/* output stream */
{
  int i;

  /* stderr is the default output stream */
  if (!stream)
    stream = stderr;

  /* dump processor register state */
  fprintf(stream, "Processor state:\n");
  fprintf(stream, "    PC: 0x%08x\n", regs_PC);
  for (i=0; i<SS_NUM_REGS; i += 2)
    {
      fprintf(stream, "    R[%2d]: %12d/0x%08x",
	      i, regs_R[i], regs_R[i]);
      fprintf(stream, "  R[%2d]: %12d/0x%08x\n",
	      i+1, regs_R[i+1], regs_R[i+1]);
    }
  fprintf(stream, "    HI:      %10d/0x%08x  LO:      %10d/0x%08x\n",
	  regs_HI, regs_HI, regs_LO, regs_LO);
  for (i=0; i<SS_NUM_REGS; i += 2)
    {
      fprintf(stream, "    F[%2d]: %12d/0x%08x",
	      i, regs_F.l[i], regs_F.l[i]);
      fprintf(stream, "  F[%2d]: %12d/0x%08x\n",
	      i+1, regs_F.l[i+1], regs_F.l[i+1]);
    }
  fprintf(stream, "    FCC:                0x%08x\n", regs_FCC);
}

/* (signed) integer register file */
SS_WORD_TYPE regs_R[SS_NUM_REGS];

/* floating point register file */
union regs_FP regs_F;

/* (signed) hi register, holds mult/div results */
SS_WORD_TYPE regs_HI;
/* (signed) lo register, holds mult/div results */
SS_WORD_TYPE regs_LO;

/* floating point condition codes */
int regs_FCC;

/* program counter */
SS_ADDR_TYPE regs_PC;

#endif
