/* alpha.c - Alpha ISA definition routines */

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

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "eval.h"
#include "regs.h"

#if 0 /* cross-endian execution now works with EIO trace files only... */
/* FIXME: currently SimpleScalar/AXP only builds on little-endian... */
#if !defined(BYTES_LITTLE_ENDIAN) || !defined(WORDS_LITTLE_ENDIAN)
#error SimpleScalar/AXP only builds on little-endian machines...
#endif
#endif

/* FIXME: currently SimpleScalar/AXP only builds with qword support... */
#if !defined(HOST_HAS_QWORD)
#error SimpleScalar/AXP only builds on hosts with builtin qword support...
#error Try building with GNU GCC, as it supports qwords on most machines.
#endif

/* preferred nop instruction definition */
md_inst_t MD_NOP_INST = 0x43ff041f;		/* addq r31,r31,r31 */

/* opcode mask -> enum md_opcodem, used by decoder (MD_OP_ENUM()) */
enum md_opcode md_mask2op[MD_MAX_MASK+1];
unsigned int md_opoffset[OP_MAX];

/* enum md_opcode -> mask for decoding next level */
unsigned int md_opmask[OP_MAX] = {
  0, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3) 0,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK) MASK,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_opcode -> shift for decoding next level */
unsigned int md_opshift[OP_MAX] = {
  0, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3) 0,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK) SHIFT,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_opcode -> description string */
char *md_op2name[OP_MAX] = {
  NULL, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3) NAME,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) NAME,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_opcode -> opcode operand format, used by disassembler */
char *md_op2format[OP_MAX] = {
  NULL, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3) OPFORM,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) NULL,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_opcode -> enum md_fu_class, used by performance simulators */
enum md_fu_class md_op2fu[OP_MAX] = {
  FUClamd_NA, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3) RES,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) FUClamd_NA,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_fu_class -> description string */
char *md_fu2name[NUM_FU_CLASSES] = {
  NULL, /* NA */
  "fu-int-ALU",
  "fu-int-multiply",
  "fu-int-divide",
  "fu-FP-add/sub",
  "fu-FP-comparison",
  "fu-FP-conversion",
  "fu-FP-multiply",
  "fu-FP-divide",
  "fu-FP-sqrt",
  "rd-port",
  "wr-port"
};

char *md_amode_str[md_amode_NUM] =
{
  "(const)",		/* immediate addressing mode */
  "(gp + const)",	/* global data access through global pointer */
  "(sp + const)",	/* stack access through stack pointer */
  "(fp + const)",	/* stack access through frame pointer */
  "(reg + const)",	/* (reg + const) addressing */
  "(reg + reg)"		/* (reg + reg) addressing */
};

/* symbolic register names, parser is case-insensitive */
struct md_reg_names_t md_reg_names[] =
{
  /* name */	/* file */	/* reg */

  /* integer register file */
  { "$r0",	rt_gpr,		0 },
  { "$v0",	rt_gpr,		0 },
  { "$r1",	rt_gpr,		1 },
  { "$r2",	rt_gpr,		2 },
  { "$r3",	rt_gpr,		3 },
  { "$r4",	rt_gpr,		4 },
  { "$r5",	rt_gpr,		5 },
  { "$r6",	rt_gpr,		6 },
  { "$r7",	rt_gpr,		7 },
  { "$err",	rt_gpr,		7 },
  { "$r8",	rt_gpr,		8 },
  { "$r9",	rt_gpr,		9 },
  { "$r10",	rt_gpr,		10 },
  { "$r11",	rt_gpr,		11 },
  { "$r12",	rt_gpr,		12 },
  { "$r13",	rt_gpr,		13 },
  { "$r14",	rt_gpr,		14 },
  { "$r15",	rt_gpr,		15 },
  { "$fp",	rt_gpr,		15 },
  { "$r16",	rt_gpr,		16 },
  { "$a0",	rt_gpr,		16 },
  { "$r17",	rt_gpr,		17 },
  { "$a1",	rt_gpr,		17 },
  { "$r18",	rt_gpr,		18 },
  { "$a2",	rt_gpr,		18 },
  { "$r19",	rt_gpr,		19 },
  { "$a3",	rt_gpr,		19 },
  { "$r20",	rt_gpr,		20 },
  { "$a4",	rt_gpr,		20 },
  { "$r21",	rt_gpr,		21 },
  { "$a5",	rt_gpr,		21 },
  { "$r22",	rt_gpr,		22 },
  { "$r23",	rt_gpr,		23 },
  { "$r24",	rt_gpr,		24 },
  { "$r25",	rt_gpr,		25 },
  { "$r26",	rt_gpr,		26 },
  { "$ra",	rt_gpr,		26 },
  { "$r27",	rt_gpr,		27 },
  { "$r28",	rt_gpr,		28 },
  { "$r29",	rt_gpr,		29 },
  { "$gp",	rt_gpr,		29 },
  { "$r30",	rt_gpr,		30 },
  { "$sp",	rt_gpr,		30 },
  { "$r31",	rt_gpr,		31 },
  { "$zero",	rt_gpr,		31 },

  /* floating point register file - double precision */
  { "$f0",	rt_fpr,		0 },
  { "$f1",	rt_fpr,		1 },
  { "$f2",	rt_fpr,		2 },
  { "$f3",	rt_fpr,		3 },
  { "$f4",	rt_fpr,		4 },
  { "$f5",	rt_fpr,		5 },
  { "$f6",	rt_fpr,		6 },
  { "$f7",	rt_fpr,		7 },
  { "$f8",	rt_fpr,		8 },
  { "$f9",	rt_fpr,		9 },
  { "$f10",	rt_fpr,		10 },
  { "$f11",	rt_fpr,		11 },
  { "$f12",	rt_fpr,		12 },
  { "$f13",	rt_fpr,		13 },
  { "$f14",	rt_fpr,		14 },
  { "$f15",	rt_fpr,		15 },
  { "$f16",	rt_fpr,		16 },
  { "$f17",	rt_fpr,		17 },
  { "$f18",	rt_fpr,		18 },
  { "$f19",	rt_fpr,		19 },
  { "$f20",	rt_fpr,		20 },
  { "$f21",	rt_fpr,		21 },
  { "$f22",	rt_fpr,		22 },
  { "$f23",	rt_fpr,		23 },
  { "$f24",	rt_fpr,		24 },
  { "$f25",	rt_fpr,		25 },
  { "$f26",	rt_fpr,		26 },
  { "$f27",	rt_fpr,		27 },
  { "$f28",	rt_fpr,		28 },
  { "$f29",	rt_fpr,		29 },
  { "$f30",	rt_fpr,		30 },
  { "$f31",	rt_fpr,		31 },

  /* floating point register file - integer precision */
  { "$l0",	rt_lpr,		0 },
  { "$l1",	rt_lpr,		1 },
  { "$l2",	rt_lpr,		2 },
  { "$l3",	rt_lpr,		3 },
  { "$l4",	rt_lpr,		4 },
  { "$l5",	rt_lpr,		5 },
  { "$l6",	rt_lpr,		6 },
  { "$l7",	rt_lpr,		7 },
  { "$l8",	rt_lpr,		8 },
  { "$l9",	rt_lpr,		9 },
  { "$l10",	rt_lpr,		10 },
  { "$l11",	rt_lpr,		11 },
  { "$l12",	rt_lpr,		12 },
  { "$l13",	rt_lpr,		13 },
  { "$l14",	rt_lpr,		14 },
  { "$l15",	rt_lpr,		15 },
  { "$l16",	rt_lpr,		16 },
  { "$l17",	rt_lpr,		17 },
  { "$l18",	rt_lpr,		18 },
  { "$l19",	rt_lpr,		19 },
  { "$l20",	rt_lpr,		20 },
  { "$l21",	rt_lpr,		21 },
  { "$l22",	rt_lpr,		22 },
  { "$l23",	rt_lpr,		23 },
  { "$l24",	rt_lpr,		24 },
  { "$l25",	rt_lpr,		25 },
  { "$l26",	rt_lpr,		26 },
  { "$l27",	rt_lpr,		27 },
  { "$l28",	rt_lpr,		28 },
  { "$l29",	rt_lpr,		29 },
  { "$l30",	rt_lpr,		30 },
  { "$l31",	rt_lpr,		31 },

  /* miscellaneous registers */
  { "$fpcr",	rt_ctrl,	0 },
  { "$uniq",	rt_ctrl,	1 },

  /* program counters */
  { "$pc",	rt_PC,		0 },
  { "$npc",	rt_NPC,		0 },

  /* sentinel */
  { NULL,	rt_gpr,		0 }
};

/* returns a register name string */
char *
md_reg_name(enum md_reg_type rt, int reg)
{
  int i;

  for (i=0; md_reg_names[i].str != NULL; i++)
    {
      if (md_reg_names[i].file == rt && md_reg_names[i].reg == reg)
	return md_reg_names[i].str;
    }

  /* no found... */
  return NULL;
}

char *						/* err str, NULL for no err */
md_reg_obj(struct regs_t *regs,			/* registers to access */
	   int is_write,			/* access type */
	   enum md_reg_type rt,			/* reg bank to probe */
	   int reg,				/* register number */
	   struct eval_value_t *val)		/* input, output */
{
  switch (rt)
    {
    case rt_gpr:
      if (reg < 0 || reg >= MD_NUM_IREGS)
	return "register number out of range";

      if (!is_write)
	{
	  val->type = et_qword;
	  val->value.as_qword = regs->regs_R[reg];
	}
      else
	regs->regs_R[reg] = eval_as_qword(*val);
      break;

    case rt_lpr:
      if (reg < 0 || reg >= MD_NUM_FREGS)
	return "register number out of range";

      if (!is_write)
	{
	  val->type = et_qword;
	  val->value.as_qword = regs->regs_F.q[reg];
	}
      else
	regs->regs_F.q[reg] = eval_as_qword(*val);
      break;

    case rt_fpr:
      if (reg < 0 || reg >= MD_NUM_FREGS)
	return "register number out of range";

      if (!is_write)
	{
	  val->type = et_double;
	  val->value.as_double = regs->regs_F.d[reg];
	}
      else
	regs->regs_F.d[reg] = eval_as_double(*val);
      break;

    case rt_ctrl:
      switch (reg)
	{
	case /* FPCR */0:
	  if (!is_write)
	    {
	      val->type = et_qword;
	      val->value.as_qword = regs->regs_C.fpcr;
	    }
	  else
	    regs->regs_C.fpcr = eval_as_qword(*val);
	  break;

	case /* UNIQ */1:
	  if (!is_write)
	    {
	      val->type = et_qword;
	      val->value.as_qword = regs->regs_C.uniq;
	    }
	  else
	    regs->regs_C.uniq = eval_as_qword(*val);
	  break;

	default:
	  return "register number out of range";
	}
      break;

    case rt_PC:
      if (!is_write)
	{
	  val->type = et_addr;
	  val->value.as_addr = regs->regs_PC;
	}
      else
	regs->regs_PC = eval_as_addr(*val);
      break;

    case rt_NPC:
      if (!is_write)
	{
	  val->type = et_addr;
	  val->value.as_addr = regs->regs_NPC;
	}
      else
	regs->regs_NPC = eval_as_addr(*val);
      break;

    default:
      panic("bogus register bank");
    }

  /* no error */
  return NULL;
}

/* print integer REG(S) to STREAM */
void
md_print_ireg(md_gpr_t regs, int reg, FILE *stream)
{
  myfprintf(stream, "%4s: %16ld/0x%012lx",
	    md_reg_name(rt_gpr, reg), regs[reg], regs[reg]);
}

void
md_print_iregs(md_gpr_t regs, FILE *stream)
{
  int i;

  for (i=0; i < MD_NUM_IREGS; i += 2)
    {
      md_print_ireg(regs, i, stream);
      fprintf(stream, "  ");
      md_print_ireg(regs, i+1, stream);
      fprintf(stream, "\n");
    }
}

/* print floating point REGS to STREAM */
void
md_print_fpreg(md_fpr_t regs, int reg, FILE *stream)
{
  myfprintf(stream, "%4s: %16ld/0x%012lx/%f",
	    md_reg_name(rt_fpr, reg), regs.q[reg], regs.q[reg], regs.d[reg]);
}

void
md_print_fpregs(md_fpr_t regs, FILE *stream)
{
  int i;

  /* floating point registers */
  for (i=0; i < MD_NUM_FREGS; i += 2)
    {
      md_print_fpreg(regs, i, stream);
      fprintf(stream, "\n");

      md_print_fpreg(regs, i+1, stream);
      fprintf(stream, "\n");
    }
}

void
md_print_creg(md_ctrl_t regs, int reg, FILE *stream)
{
  /* index is only used to iterate over these registers, hence no enums... */
  switch (reg)
    {
    case 0:
      myfprintf(stream, "FPCR: 0x%012lx", regs.fpcr);
      break;

    case 1:
      myfprintf(stream, "UNIQ: 0x%012lx", regs.uniq);
      break;

    default:
      panic("bogus control register index");
    }
}

void
md_print_cregs(md_ctrl_t regs, FILE *stream)
{
  md_print_creg(regs, 0, stream);
  fprintf(stream, "  ");
  md_print_creg(regs, 1, stream);
  fprintf(stream, "\n");
}

/* xor checksum registers */
word_t
md_xor_regs(struct regs_t *regs)
{
  int i;
  qword_t checksum = 0;

  for (i=0; i < (MD_NUM_IREGS-1); i++)
    checksum ^= regs->regs_R[i];

  for (i=0; i < (MD_NUM_FREGS-1); i++)
    checksum ^= regs->regs_F.q[i];

  checksum ^= regs->regs_C.fpcr;
  checksum ^= regs->regs_C.uniq;
  checksum ^= regs->regs_PC;
  checksum ^= regs->regs_NPC;

  return (word_t)((checksum >> 32) ^ checksum);
}


/* enum md_opcode -> opcode flags, used by simulators */
unsigned int md_op2flags[OP_MAX] = {
  NA, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3) FLAGS,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) NA,
#define CONNECT(OP)
#include "machine.def"
};


/* intialize the inst decoder, this function builds the ISA decode tables */
void
md_init_decoder(void)
{
  unsigned long max_offset = 0;
  unsigned long offset = 0;

#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
  if ((MSK)+offset >= MD_MAX_MASK)					\
    panic("MASK_MAX is too small, index==%d", (MSK)+offset);		\
  if (md_mask2op[(MSK)+offset])						\
    fatal("doubly defined opcode, index==%d", (MSK)+offset);		\
  md_mask2op[(MSK)+offset]=(OP); max_offset=MAX(max_offset,(MSK)+offset);

#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
  if ((MSK)+offset >= MD_MAX_MASK)					\
    panic("MASK_MAX is too small, index==%d", (MSK)+offset);		\
  if (md_mask2op[(MSK)+offset])						\
    fatal("doubly defined opcode, index==%d", (MSK)+offset);		\
  md_mask2op[(MSK)+offset]=(OP); max_offset=MAX(max_offset,(MSK)+offset);

#define CONNECT(OP)							\
    offset = max_offset+1; md_opoffset[OP] = offset;

#include "machine.def"

  if (max_offset >= MD_MAX_MASK)
    panic("MASK_MAX is too small, index==%d", max_offset);
}

/* disassemble an Alpha instruction */
void
md_print_insn(md_inst_t inst,		/* instruction to disassemble */
	      md_addr_t pc,		/* addr of inst, used for PC-rels */
	      FILE *stream)		/* output stream */
{
  enum md_opcode op;

  /* use stderr as default output stream */
  if (!stream)
    stream = stderr;

  /* decode the instruction, assumes predecoded text segment */
  MD_SET_OPCODE(op, inst);

  /* disassemble the instruction */
  if (op <= OP_NA || op >= OP_MAX)
    {
      /* bogus instruction */
      fprintf(stream, "<invalid inst: 0x%08x>", inst);
    }
  else
    {
      char *s;

      /* FIXME: %-10s crashes on Suns!!! */
      fprintf(stream, "%s ", MD_OP_NAME(op));

      s = MD_OP_FORMAT(op);
      while (*s) {
	switch (*s) {
	case 'a':
	  fprintf(stream, "r%d", RA);
	  break;
	case 'b':
	  fprintf(stream, "r%d", RB);
	  break;
	case 'c':
	  fprintf(stream, "r%d", RC);
	  break;
	case 'A':
	  fprintf(stream, "f%d", RA);
	  break;
	case 'B':
	  fprintf(stream, "f%d", RB);
	  break;
	case 'C':
	  fprintf(stream, "f%d", RC);
	  break;
	case 'o':
	  fprintf(stream, "%d", (sword_t)SEXT(OFS));
	  break;
	case 'j':
	  myfprintf(stream, "0x%p", pc + (SEXT(OFS) << 2) + 4);
	  break;
	case 'J':
	  myfprintf(stream, "0x%p", pc + (SEXT21(TARG) << 2) + 4);
	  break;
	case 'i':
	  fprintf(stream, "%d", (word_t)IMM);
	  break;
	default:
	  /* anything unrecognized, e.g., '.' is just passed through */
	  fputc(*s, stream);
	}
	s++;
      }
    }
}
