/* sim-eio.c - external I/O trace generator */

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
#include <math.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "eio.h"
#include "range.h"
#include "sim.h"

/*
 * This file implements a functional simulator.  This functional simulator is
 * the simplest, most user-friendly simulator in the simplescalar tool set.
 * Unlike sim-fast, this functional simulator checks for all instruction
 * errors, and the implementation is crafted for clarity rather than speed.
 */

/* simulated registers */
static struct regs_t regs;

/* simulated memory */
static struct mem_t *mem = NULL;

/* track number of refs */
static counter_t sim_num_refs = 0;

/* maximum number of inst's to execute */
static unsigned int max_insts;

/* number of insts skipped before timing starts */
static int fastfwd_count;

/* non-zero when fastforward'ing */
static int fastfwding = FALSE;

/* external I/O filename */
static char *trace_fname;
static FILE *trace_fd = NULL;

/* checkpoint filename and file descriptor */
static enum { no_chkpt, one_shot_chkpt, periodic_chkpt } chkpt_kind = no_chkpt;
static char *chkpt_fname;
static FILE *chkpt_fd = NULL;
static struct range_range_t chkpt_range;

/* periodic checkpoint args */
static counter_t per_chkpt_interval;
static counter_t next_chkpt_cycle;
static unsigned int chkpt_num;

/* checkpoint output filename and range */
static int chkpt_nelt = 0;
static char *chkpt_opts[2];

/* periodic checkpoint output filename and range */
static int per_chkpt_nelt = 0;
static char *per_chkpt_opts[2];


/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-eio: This simulator implements simulator support for generating\n"
"external event traces (EIO traces) and checkpoint files.  External\n"
"event traces capture one execution of a program, and allow it to be\n"
"packaged into a single file for later re-execution.  EIO trace executions\n"
"are 100% reproducible between subsequent executions (on the same platform.\n"
"This simulator also provides functionality to generate checkpoints at\n"
"arbitrary points within an external event trace (EIO) execution.  The\n"
"checkpoint file (along with the EIO trace) can be used to start any\n"
"SimpleScalar simulator in the middle of a program execution.\n"
		 );

  /* instruction limit */
  opt_reg_uint(odb, "-max:inst", "maximum number of inst's to execute",
	       &max_insts, /* default */0,
	       /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-fastfwd", "number of insts skipped before tracing starts",
	      &fastfwd_count, /* default */0,
	      /* print */TRUE, /* format */NULL);

  opt_reg_string(odb, "-trace", "EIO trace file output file name",
		 &trace_fname, /* default */NULL,
		 /* print */TRUE, NULL);

  opt_reg_string_list(odb, "-perdump",
		      "periodic checkpoint every n instructions: "
		      "<base fname> <interval>",
		      per_chkpt_opts, /* sz */2, &per_chkpt_nelt,
		      /* default */NULL,
		      /* !print */FALSE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_string_list(odb, "-dump",
		      "specify checkpoint file and trigger: <fname> <range>",
		      chkpt_opts, /* sz */2, &chkpt_nelt, /* default */NULL,
		      /* !print */FALSE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_note(odb,
"  Checkpoint range triggers are formatted as follows:\n"
"\n"
"    {{@|#}<start>}:{{@|#|+}<end>}\n"
"\n"
"  Both ends of the range are optional, if neither are specified, the range\n"
"  triggers immediately.  Ranges that start with a `@' designate an address\n"
"  range to trigger on, those that start with an `#' designate a cycle count\n"
"  trigger.  All other ranges represent an instruction count range.  The\n"
"  second argument, if specified with a `+', indicates a value relative\n"
"  to the first argument, e.g., 1000:+100 == 1000:1100.\n"
"\n"
"    Examples:   -ptrace FOO.trc #0:#1000\n"
"                -ptrace BAR.trc @2000:\n"
"                -ptrace BLAH.trc :1500\n"
"                -ptrace UXXE.trc :\n"
	       );
}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  if (fastfwd_count < 0 || fastfwd_count >= 2147483647)
    fatal("bad fast forward count: %d", fastfwd_count);
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{
  stat_reg_counter(sdb, "sim_num_insn",
		   "total number of instructions executed",
		   &sim_num_insn, sim_num_insn, NULL);
  stat_reg_counter(sdb, "sim_num_refs",
		   "total number of loads and stores executed",
		   &sim_num_refs, 0, NULL);
  stat_reg_int(sdb, "sim_elapsed_time",
	       "total simulation time in seconds",
	       &sim_elapsed_time, 0, NULL);
  stat_reg_formula(sdb, "sim_inst_rate",
		   "simulation speed (in insts/sec)",
		   "sim_num_insn / sim_elapsed_time", NULL);
  ld_reg_stats(sdb);
  mem_reg_stats(mem, sdb);
}

/* initialize the simulator */
void
sim_init(void)
{
  sim_num_refs = 0;

  /* allocate and initialize register file */
  regs_init(&regs);

  /* allocate and initialize memory space */
  mem = mem_create("mem");
  mem_init(mem);
}

/* load program into simulated state */
void
sim_load_prog(char *fname,		/* program to load */
	      int argc, char **argv,	/* program arguments */
	      char **envp)		/* program environment */
{
  /* load program text and data, set up environment, memory, and regs */
  ld_load_prog(fname, argc, argv, envp, &regs, mem, TRUE);

  if (chkpt_nelt == 2)
    {
      char *errstr;

      /* generate a checkpoint */
      if (!sim_eio_fd)
	fatal("checkpoints can only be generated while running an EIO trace");

      /* can't do regular & periodic chkpts at the same time */
      if (per_chkpt_nelt != 0)
	fatal("can't do both regular and periodic checkpoints");

#if 0 /* this should work fine... */
      if (trace_fname != NULL)
	fatal("checkpoints cannot be generated with generating an EIO trace");
#endif

      /* parse the range */
      errstr = range_parse_range(chkpt_opts[1], &chkpt_range);
      if (errstr)
	fatal("cannot parse pipetrace range, use: {<start>}:{<end>}");

      /* create the checkpoint file */
      chkpt_fname = chkpt_opts[0];
      chkpt_fd = eio_create(chkpt_fname);

      /* indicate checkpointing is now active... */
      chkpt_kind = one_shot_chkpt;
    }

  if (per_chkpt_nelt == 2)
    {
      chkpt_fname = per_chkpt_opts[0];
      if (strchr(chkpt_fname, '%') == NULL)
	fatal("periodic checkpoint filename must be printf-style format");

      if (sscanf(per_chkpt_opts[1], "%Ld", &per_chkpt_interval) != 1)
	fatal("can't parse periodic checkpoint interval '%s'",
	      per_chkpt_opts[1]);

      /* indicate checkpointing is now active... */
      chkpt_kind = periodic_chkpt;
      chkpt_num = 1;
      next_chkpt_cycle = per_chkpt_interval;
    }

  if (trace_fname != NULL)
    {
      fprintf(stderr, "sim: tracing execution to EIO file `%s'...\n",
	      trace_fname);

      /* create an EIO trace file */
      trace_fd = eio_create(trace_fname);
    }

  /* initialize the DLite debugger */
  dlite_init(md_reg_obj, dlite_mem_obj, dlite_mstate_obj);
}


/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)		/* output stream */
{
  /* nothing currently */
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)		/* output stream */
{
  /* nada */
}

/* un-initialize simulator-specific state */
void
sim_uninit(void)
{
  if (trace_fd != NULL)
    eio_close(trace_fd);
}


/*
 * configure the execution engine
 */

/*
 * precise architected register accessors
 */

/* next program counter */
#define SET_NPC(EXPR)		(regs.regs_NPC = (EXPR))

/* current program counter */
#define CPC			(regs.regs_PC)

/* general purpose registers */
#define GPR(N)			(regs.regs_R[N])
#define SET_GPR(N,EXPR)		(regs.regs_R[N] = (EXPR))

#if defined(TARGET_PISA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_L(N)		(regs.regs_F.l[(N)])
#define SET_FPR_L(N,EXPR)	(regs.regs_F.l[(N)] = (EXPR))
#define FPR_F(N)		(regs.regs_F.f[(N)])
#define SET_FPR_F(N,EXPR)	(regs.regs_F.f[(N)] = (EXPR))
#define FPR_D(N)		(regs.regs_F.d[(N) >> 1])
#define SET_FPR_D(N,EXPR)	(regs.regs_F.d[(N) >> 1] = (EXPR))

/* miscellaneous register accessors */
#define SET_HI(EXPR)		(regs.regs_C.hi = (EXPR))
#define HI			(regs.regs_C.hi)
#define SET_LO(EXPR)		(regs.regs_C.lo = (EXPR))
#define LO			(regs.regs_C.lo)
#define FCC			(regs.regs_C.fcc)
#define SET_FCC(EXPR)		(regs.regs_C.fcc = (EXPR))

#elif defined(TARGET_ALPHA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_Q(N)		(regs.regs_F.q[N])
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = (EXPR))
#define FPR(N)			(regs.regs_F.d[N])
#define SET_FPR(N,EXPR)		(regs.regs_F.d[N] = (EXPR))

/* miscellaneous register accessors */
#define FPCR			(regs.regs_C.fpcr)
#define SET_FPCR(EXPR)		(regs.regs_C.fpcr = (EXPR))
#define UNIQ			(regs.regs_C.uniq)
#define SET_UNIQ(EXPR)		(regs.regs_C.uniq = (EXPR))

#else
#error No ISA target defined...
#endif

/* precise architected memory state accessor macros */
#define READ_BYTE(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_BYTE(mem, addr))
#define READ_HALF(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_HALF(mem, addr))
#define READ_WORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_WORD(mem, addr))
#ifdef HOST_HAS_QWORD
#define READ_QWORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_QWORD(mem, addr))
#endif /* HOST_HAS_QWORD */

#define WRITE_BYTE(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_BYTE(mem, addr, (SRC)))
#define WRITE_HALF(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_HALF(mem, addr, (SRC)))
#define WRITE_WORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_WORD(mem, addr, (SRC)))
#ifdef HOST_HAS_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_QWORD(mem, addr, (SRC)))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#define SYSCALL(INST)							\
  ((trace_fd != NULL && !fastfwding)					\
   ? eio_write_trace(trace_fd, sim_num_insn,				\
		     &regs, mem_access, mem, INST)			\
   : sys_syscall(&regs, mem_access, mem, INST, TRUE))

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{
  md_inst_t inst;
  register md_addr_t addr;
  enum md_opcode op;
  register bool_t is_write;
  enum md_fault_type fault;

  /* set up initial default next PC */
  regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);

  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs.regs_PC, /* !access */0, /* addr */0, 0, 0))
    dlite_main(regs.regs_PC - sizeof(md_inst_t), regs.regs_PC,
	       sim_num_insn, &regs, mem);

  /* fast forward simulator loop, performs functional simulation for
     FASTFWD_COUNT insts, then turns on performance (timing) simulation */
  if (fastfwd_count > 0)
    {
      int icount;

      fprintf(stderr, "sim: ** fast forwarding %d insts **\n", fastfwd_count);

      fastfwding = TRUE;
      for (icount=0; icount < fastfwd_count; icount++)
	{
	  /* maintain $r0 semantics */
	  regs.regs_R[MD_REG_ZERO] = 0;
#ifdef TARGET_ALPHA
	  regs.regs_F.d[MD_REG_ZERO] = 0.0;
#endif /* TARGET_ALPHA */

	  /* get the next instruction to execute */
	  MD_FETCH_INST(inst, mem, regs.regs_PC);

	  /* set default reference address */
	  addr = 0; is_write = FALSE;

	  /* set default fault - none */
	  fault = md_fault_none;

	  /* decode the instruction */
	  MD_SET_OPCODE(op, inst);

	  /* execute the instruction */
	  switch (op)
	    {
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	    case OP:							\
	      SYMCAT(OP,_IMPL);						\
	      break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
	    case OP:							\
	      panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#undef DECLARE_FAULT
#define DECLARE_FAULT(FAULT)						\
	      { fault = (FAULT); break; }
#include "machine.def"
	    default:
	      panic("attempted to execute a bogus opcode");
	    }

	  if (fault != md_fault_none)
	    fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC);

	  /* update memory access stats */
	  if (MD_OP_FLAGS(op) & F_MEM)
	    {
	      if (MD_OP_FLAGS(op) & F_STORE)
		is_write = TRUE;
	    }

	  /* check for DLite debugger entry condition */
	  if (dlite_check_break(regs.regs_NPC,
				is_write ? ACCESS_WRITE : ACCESS_READ,
				addr, sim_num_insn, sim_num_insn))
	    dlite_main(regs.regs_PC, regs.regs_NPC, sim_num_insn, &regs, mem);

	  /* go to the next instruction */
	  regs.regs_PC = regs.regs_NPC;
	  regs.regs_NPC += sizeof(md_inst_t);
	}
    }
  fastfwding = FALSE;

  if (trace_fd != NULL)
    {
      fprintf(stderr, "sim: writing EIO file initial checkpoint...\n");
      if (eio_write_chkpt(&regs, mem, trace_fd) != -1)
	panic("checkpoint code is broken");
    }

  fprintf(stderr, "sim: ** starting functional simulation **\n");

  /* set up initial default next PC */
  regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);

  while (TRUE)
    {
      /* maintain $r0 semantics */
      regs.regs_R[MD_REG_ZERO] = 0;
#ifdef TARGET_ALPHA
      regs.regs_F.d[MD_REG_ZERO] = 0.0;
#endif /* TARGET_ALPHA */

      /* check if checkpoint should be generated here... */
      if (chkpt_kind == one_shot_chkpt
	  && !range_cmp_range1(&chkpt_range, regs.regs_NPC,
			       sim_num_insn, sim_num_insn))
	{
	  myfprintf(stderr, "sim: writing checkpoint file `%s' @ inst %n...\n",
		  chkpt_fname, sim_num_insn);

	  /* write the checkpoint file */
	  eio_write_chkpt(&regs, mem, chkpt_fd);

	  /* close the checkpoint file */
	  eio_close(chkpt_fd);

	  /* exit jumps to the target set in main() */
	  longjmp(sim_exit_buf, /* exitcode + fudge */0+1);
	}
      else if (chkpt_kind == periodic_chkpt
	       && sim_num_insn == next_chkpt_cycle)
	{
	  char this_chkpt_fname[256];

	  /* 'chkpt_fname' should be a printf format string */
	  sprintf(this_chkpt_fname, chkpt_fname, chkpt_num);
	  chkpt_fd = eio_create(this_chkpt_fname);

	  myfprintf(stderr, "sim: writing checkpoint file `%s' @ inst %n...\n",
		  this_chkpt_fname, sim_num_insn);

	  /* write the checkpoint file */
	  eio_write_chkpt(&regs, mem, chkpt_fd);

	  /* close the checkpoint file */
	  eio_close(chkpt_fd);

	  chkpt_num++;
	  next_chkpt_cycle += per_chkpt_interval;
	}

      /* get the next instruction to execute */
      MD_FETCH_INST(inst, mem, regs.regs_PC);

      /* keep an instruction count */
      sim_num_insn++;

      /* set default reference address and access mode */
      addr = 0; is_write = FALSE;

      /* set default fault - none */
      fault = md_fault_none;

      /* decode the instruction */
      MD_SET_OPCODE(op, inst);

      /* execute the instruction */
      switch (op)
	{
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	case OP:							\
          SYMCAT(OP,_IMPL);						\
          break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
        case OP:							\
          panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#define DECLARE_FAULT(FAULT)						\
	  { fault = (FAULT); break; }
#include "machine.def"
	default:
	  panic("bogus opcode");
      }

      if (fault != md_fault_none)
	fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC);

      if (MD_OP_FLAGS(op) & F_MEM)
	{
	  sim_num_refs++;
	  if (MD_OP_FLAGS(op) & F_STORE)
	    is_write = TRUE;
	}

      /* check for DLite debugger entry condition */
      if (dlite_check_break(regs.regs_NPC,
			    is_write ? ACCESS_WRITE : ACCESS_READ,
			    addr, sim_num_insn, sim_num_insn))
	dlite_main(regs.regs_PC, regs.regs_NPC, sim_num_insn, &regs, mem);

      /* go to the next instruction */
      regs.regs_PC = regs.regs_NPC;
      regs.regs_NPC += sizeof(md_inst_t);

      /* finish early? */
      if (max_insts && sim_num_insn >= max_insts)
	return;
    }
}
