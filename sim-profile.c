/* sim-profile.c - sample functional simulator implementation w/ profiling */

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
#include "symbol.h"
#include "options.h"
#include "stats.h"
#include "sim.h"

/*
 * This file implements a functional simulator with profiling support.  Run
 * with the `-h' flag to see profiling options available.
 */

/* simulated registers */
static struct regs_t regs;

/* simulated memory */
static struct mem_t *mem = NULL;

/* track number of refs */
static counter_t sim_num_refs = 0;

/* maximum number of inst's to execute */
static unsigned int max_insts;

/* profiling options */
static int prof_all /* = FALSE */;
static int prof_ic /* = FALSE */;
static int prof_inst /* = FALSE */;
static int prof_bc /* = FALSE */;
static int prof_am /* = FALSE */;
static int prof_seg /* = FALSE */;
static int prof_tsyms /* = FALSE */;
static int prof_dsyms /* = FALSE */;
static int load_locals /* = FALSE */;
static int prof_taddr /* = FALSE */;

/* text-based stat profiles */
#define MAX_PCSTAT_VARS 8
static int pcstat_nelt = 0;
static char *pcstat_vars[MAX_PCSTAT_VARS];

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-profile: This simulator implements a functional simulator with\n"
"profiling support.  Run with the `-h' flag to see profiling options\n"
"available.\n"
		 );

  /* instruction limit */
  opt_reg_uint(odb, "-max:inst", "maximum number of inst's to execute",
	       &max_insts, /* default */0,
	       /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-all", "enable all profile options",
	       &prof_all, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-iclass", "enable instruction class profiling",
	       &prof_ic, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-iprof", "enable instruction profiling",
	       &prof_inst, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-brprof", "enable branch instruction profiling",
	       &prof_bc, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-amprof", "enable address mode profiling",
	       &prof_am, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-segprof", "enable load/store address segment profiling",
	       &prof_seg, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-tsymprof", "enable text symbol profiling",
	       &prof_tsyms, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-taddrprof", "enable text address profiling",
	       &prof_taddr, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-dsymprof", "enable data symbol profiling",
	       &prof_dsyms, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-internal",
	       "include compiler-internal symbols during symbol profiling",
	       &load_locals, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_string_list(odb, "-pcstat",
		      "profile stat(s) against text addr's (mult uses ok)",
		      pcstat_vars, MAX_PCSTAT_VARS, &pcstat_nelt, NULL,
		      /* !print */FALSE, /* format */NULL, /* accrue */TRUE);
}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  if (prof_all)
    {
      /* enable all options */
      prof_ic = TRUE;
      prof_inst = TRUE;
      prof_bc = TRUE;
      prof_am = TRUE;
      prof_seg = TRUE;
      prof_tsyms = TRUE;
      prof_dsyms = TRUE;
      prof_taddr = TRUE;
    }
}

/* instruction classes */
enum inst_class_t {
  ic_load,		/* load inst */
  ic_store,		/* store inst */
  ic_uncond,		/* uncond branch */
  ic_cond,		/* cond branch */
  ic_icomp,		/* all other integer computation */
  ic_fcomp,		/* all floating point computation */
  ic_trap,		/* system call */
  ic_NUM
};

/* instruction class strings */
static char *inst_class_str[ic_NUM] = {
  "load",		/* load inst */
  "store",		/* store inst */
  "uncond branch",	/* uncond branch */
  "cond branch",	/* cond branch */
  "int computation",	/* all other integer computation */
  "fp computation",	/* all floating point computation */
  "trap"		/* system call */
};

/* instruction class profile */
static struct stat_stat_t *ic_prof = NULL;

/* instruction description strings */
static char *inst_str[OP_MAX];

/* instruction profile */
static struct stat_stat_t *inst_prof = NULL;

/* branch class profile */
enum branch_class_t {
  bc_uncond_dir,	/* direct unconditional branch */
  bc_cond_dir,		/* direct conditional branch */
  bc_call_dir,		/* direct functional call */
  bc_uncond_indir,	/* indirect unconditional branch */
  bc_cond_indir,	/* indirect conditional branch */
  bc_call_indir,	/* indirect function call */
  bc_NUM
};

/* branch class description strings */
static char *branch_class_str[bc_NUM] = {
  "uncond direct",	/* direct unconditional branch */
  "cond direct",	/* direct conditional branch */
  "call direct",	/* direct functional call */
  "uncond indirect",	/* indirect unconditional branch */
  "cond indirect",	/* indirect conditional branch */
  "call indirect"	/* indirect function call */
};

/* branch profile */
static struct stat_stat_t *bc_prof = NULL;

/* addressing mode profile */
static struct stat_stat_t *am_prof = NULL;

/* address segments */
enum addr_seg_t {
  seg_data,		/* data segment */
  seg_heap,		/* heap segment */
  seg_stack,		/* stack segment */
  seg_text,		/* text segment */
  seg_NUM
};

/* address segment strings */
static char *addr_seg_str[seg_NUM] = {
  "data segment",	/* data segment */
  "heap segment",	/* heap segment */
  "stack segment",	/* stack segment */
  "text segment",	/* text segment */
};

/* address segment profile */
static struct stat_stat_t *seg_prof = NULL;

/* bind ADDR to the segment it references */
static enum addr_seg_t			/* segment referenced by ADDR */
bind_to_seg(md_addr_t addr)		/* address to bind to a segment */
{
  if (ld_data_base <= addr && addr < (ld_data_base + ld_data_size))
    return seg_data;
  else if ((ld_data_base + ld_data_size) <= addr && addr < ld_brk_point)
    return seg_heap;
  /* FIXME: ouch! */
  else if ((ld_stack_base - (16*1024*1024)) <= addr && addr < ld_stack_base)
    return seg_stack;
  else if (ld_text_base <= addr && addr < (ld_text_base + ld_text_size))
    return seg_text;
  else
    panic("cannot bind address to segment");
}

/* text symbol profile */
static struct stat_stat_t *tsym_prof = NULL;
static char **tsym_names = NULL;

/* data symbol profile */
static struct stat_stat_t *dsym_prof = NULL;
static char **dsym_names = NULL;

/* text address profile */
static struct stat_stat_t *taddr_prof = NULL;

/* text-based stat profiles */
static struct stat_stat_t *pcstat_stats[MAX_PCSTAT_VARS];
static counter_t pcstat_lastvals[MAX_PCSTAT_VARS];
static struct stat_stat_t *pcstat_sdists[MAX_PCSTAT_VARS];

/* wedge all stat values into a counter_t */
#define STATVAL(STAT)							\
  ((STAT)->sc == sc_int							\
   ? (counter_t)*((STAT)->variant.for_int.var)				\
   : ((STAT)->sc == sc_uint						\
      ? (counter_t)*((STAT)->variant.for_uint.var)			\
      : ((STAT)->sc == sc_counter					\
	 ? *((STAT)->variant.for_counter.var)				\
	 : (panic("bad stat class"), 0))))

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{
  int i;

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

  if (prof_ic)
    {
      /* instruction class profile */
      ic_prof = stat_reg_dist(sdb, "sim_inst_class_prof",
			      "instruction class profile",
			      /* initial value */0,
			      /* array size */ic_NUM,
			      /* bucket size */1,
			      /* print format */(PF_COUNT|PF_PDF),
			      /* format */NULL,
			      /* index map */inst_class_str,
			      /* print fn */NULL);
    }

  if (prof_inst)
    {
      int i;
      char buf[512];

      /* conjure up appropriate instruction description strings */
      for (i=0; i < /* skip NA */OP_MAX-1; i++)
	{
	  sprintf(buf, "%-8s %-6s", md_op2name[i+1], md_op2format[i+1]);
	  inst_str[i] = mystrdup(buf);
	}
      
      /* instruction profile */
      inst_prof = stat_reg_dist(sdb, "sim_inst_prof",
				"instruction profile",
				/* initial value */0,
				/* array size */ /* skip NA */OP_MAX-1,
				/* bucket size */1,
				/* print format */(PF_COUNT|PF_PDF),
				/* format */NULL,
				/* index map */inst_str,
				/* print fn */NULL);
    }

  if (prof_bc)
    {
      /* instruction branch profile */
      bc_prof = stat_reg_dist(sdb, "sim_branch_prof",
			      "branch instruction profile",
			      /* initial value */0,
			      /* array size */bc_NUM,
			      /* bucket size */1,
			      /* print format */(PF_COUNT|PF_PDF),
			      /* format */NULL,
			      /* index map */branch_class_str,
			      /* print fn */NULL);
    }

  if (prof_am)
    {
      /* instruction branch profile */
      am_prof = stat_reg_dist(sdb, "sim_addr_mode_prof",
			      "addressing mode profile",
			      /* initial value */0,
			      /* array size */md_amode_NUM,
			      /* bucket size */1,
			      /* print format */(PF_COUNT|PF_PDF),
			      /* format */NULL,
			      /* index map */md_amode_str,
			      /* print fn */NULL);
    }

  if (prof_seg)
    {
      /* instruction branch profile */
      seg_prof = stat_reg_dist(sdb, "sim_addr_seg_prof",
			       "load/store address segment profile",
			       /* initial value */0,
			       /* array size */seg_NUM,
			       /* bucket size */1,
			       /* print format */(PF_COUNT|PF_PDF),
			       /* format */NULL,
			       /* index map */addr_seg_str,
			       /* print fn */NULL);
    }

  if (prof_tsyms && sym_ntextsyms != 0)
    {
      int i;

      /* load program symbols */
      sym_loadsyms(ld_prog_fname, load_locals);

      /* conjure up appropriate instruction description strings */
      tsym_names = (char **)calloc(sym_ntextsyms, sizeof(char *));

      for (i=0; i < sym_ntextsyms; i++)
	tsym_names[i] = sym_textsyms[i]->name;
      
      /* text symbol profile */
      tsym_prof = stat_reg_dist(sdb, "sim_text_sym_prof",
				"text symbol profile",
				/* initial value */0,
				/* array size */sym_ntextsyms,
				/* bucket size */1,
				/* print format */(PF_COUNT|PF_PDF),
				/* format */NULL,
				/* index map */tsym_names,
				/* print fn */NULL);
    }

  if (prof_dsyms && sym_ndatasyms != 0)
    {
      int i;

      /* load program symbols */
      sym_loadsyms(ld_prog_fname, load_locals);

      /* conjure up appropriate instruction description strings */
      dsym_names = (char **)calloc(sym_ndatasyms, sizeof(char *));

      for (i=0; i < sym_ndatasyms; i++)
	dsym_names[i] = sym_datasyms[i]->name;
      
      /* data symbol profile */
      dsym_prof = stat_reg_dist(sdb, "sim_data_sym_prof",
				"data symbol profile",
				/* initial value */0,
				/* array size */sym_ndatasyms,
				/* bucket size */1,
				/* print format */(PF_COUNT|PF_PDF),
				/* format */NULL,
				/* index map */dsym_names,
				/* print fn */NULL);
    }

  if (prof_taddr)
    {
      /* text address profile (sparse profile), NOTE: a dense print format
         is used, its more difficult to read, but the profiles are *much*
	 smaller, I've assumed that the profiles are read by programs, at
	 least for your sake I hope this is the case!! */
      taddr_prof = stat_reg_sdist(sdb, "sim_text_addr_prof",
				  "text address profile",
				  /* initial value */0,
				  /* print format */(PF_COUNT|PF_PDF),
				  /* format */"0x%p %u %.2f",
				  /* print fn */NULL);
    }

  for (i=0; i<pcstat_nelt; i++)
    {
      char buf[512], buf1[512];
      struct stat_stat_t *stat;

      /* track the named statistical variable by text address */

      /* find it... */
      stat = stat_find_stat(sdb, pcstat_vars[i]);
      if (!stat)
	fatal("cannot locate any statistic named `%s'", pcstat_vars[i]);

      /* stat must be an integral type */
      if (stat->sc != sc_int && stat->sc != sc_uint && stat->sc != sc_counter)
	fatal("`-pcstat' statistical variable `%s' is not an integral type",
	      stat->name);

      /* register this stat */
      pcstat_stats[i] = stat;
      pcstat_lastvals[i] = STATVAL(stat);

      /* declare the sparce text distribution */
      sprintf(buf, "%s_by_pc", stat->name);
      sprintf(buf1, "%s (by text address)", stat->desc);
      pcstat_sdists[i] = stat_reg_sdist(sdb, buf, buf1,
					/* initial value */0,
					/* print format */(PF_COUNT|PF_PDF),
					/* format */"0x%p %u %.2f",
					/* print fn */NULL);
    }
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

/* local machine state accessor */
static char *					/* err str, NULL for no err */
profile_mstate_obj(FILE *stream,		/* output stream */
		   char *cmd,			/* optional command string */
		   struct regs_t *regs,		/* registers to access */
		   struct mem_t *mem)		/* memory to access */
{
  /* just dump intermediate stats */
  sim_print_stats(stream);

  /* no error */
  return NULL;
}

/* load program into simulated state */
void
sim_load_prog(char *fname,		/* program to load */
	      int argc, char **argv,	/* program arguments */
	      char **envp)		/* program environment */
{
  /* load program text and data, set up environment, memory, and regs */
  ld_load_prog(fname, argc, argv, envp, &regs, mem, TRUE);

  /* initialize the DLite debugger */
  dlite_init(md_reg_obj, dlite_mem_obj, profile_mstate_obj);
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
}

/* un-initialize simulator-specific state */
void
sim_uninit(void)
{
  /* nada */
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
#define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)


/* addressing mode FSM (dest of last LUI, used for decoding addr modes) */
static unsigned int fsm = 0;

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{
  int i;
  md_inst_t inst;
  register md_addr_t addr;
  register int is_write;
  enum md_opcode op;
  unsigned int flags;
  enum md_fault_type fault;

  fprintf(stderr, "sim: ** starting functional simulation **\n");

  /* set up initial default next PC */
  regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);

  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs.regs_PC, /* no access */0, /* addr */0, 0, 0))
    dlite_main(regs.regs_PC - sizeof(md_inst_t), regs.regs_PC,
	       sim_num_insn, &regs, mem);

  while (TRUE)
    {
      /* maintain $r0 semantics */
      regs.regs_R[MD_REG_ZERO] = 0;
#ifdef TARGET_ALPHA
      regs.regs_F.d[MD_REG_ZERO] = 0.0;
#endif /* TARGET_ALPHA */

      /* get the next instruction to execute */
      MD_FETCH_INST(inst, mem, regs.regs_PC);

      if (verbose)
	{
	  myfprintf(stderr, "%10n @ 0x%08p: ", sim_num_insn, regs.regs_PC);
	  md_print_insn(inst, regs.regs_PC, stderr);
	  fprintf(stderr, "\n");
	  /* fflush(stderr); */
	}

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
	  panic("attempted to execute a bogus opcode");
      }

      if (MD_OP_FLAGS(op) & F_MEM)
	{
	  sim_num_refs++;
	  if (MD_OP_FLAGS(op) & F_STORE)
	    is_write = TRUE;
	}

      /*
       * profile this instruction
       */
      flags = MD_OP_FLAGS(op);

      if (prof_ic)
	{
	  enum inst_class_t ic;

	  /* compute instruction class */
	  if (flags & F_LOAD)
	    ic = ic_load;
	  else if (flags & F_STORE)
	    ic = ic_store;
	  else if (flags & F_UNCOND)
	    ic = ic_uncond;
	  else if (flags & F_COND)
	    ic = ic_cond;      
	  else if (flags & F_ICOMP)
	    ic = ic_icomp;
	  else if (flags & F_FCOMP)
	    ic = ic_fcomp;
	  else if (flags & F_TRAP)
	    ic = ic_trap;
	  else
	    panic("instruction has no class");

	  /* update instruction class profile */
	  stat_add_sample(ic_prof, (int)ic);
	}

      if (prof_inst)
	{
	  /* update instruction profile */
	  stat_add_sample(inst_prof, (int)op - /* skip NA */1);
	}

      if (prof_bc)
	{
	  enum branch_class_t bc;

	  /* compute instruction class */
	  if (flags & F_CTRL)
	    {
	      if ((flags & (F_CALL|F_DIRJMP)) == (F_CALL|F_DIRJMP))
		bc = bc_call_dir;
	      else if ((flags & (F_CALL|F_INDIRJMP)) == (F_CALL|F_INDIRJMP))
		bc = bc_call_indir;
	      else if ((flags & (F_UNCOND|F_DIRJMP)) == (F_UNCOND|F_DIRJMP))
		bc = bc_uncond_dir;
	      else if ((flags & (F_UNCOND|F_INDIRJMP))== (F_UNCOND|F_INDIRJMP))
		bc = bc_uncond_indir;
	      else if ((flags & (F_COND|F_DIRJMP)) == (F_COND|F_DIRJMP))
		bc = bc_cond_dir;
	      else if ((flags & (F_COND|F_INDIRJMP)) == (F_COND|F_INDIRJMP))
		bc = bc_cond_indir;
	      else
		panic("branch has no class");

	      /* update instruction class profile */
	      stat_add_sample(bc_prof, (int)bc);
	    }
	}

      if (prof_am)
	{
	  enum md_amode_type am;

	  /* update addressing mode pre-probe FSM */
	  MD_AMODE_PREPROBE(op, fsm);

	  /* compute addressing mode */
	  if (flags & F_MEM)
	    {
	      /* compute addressing mode */
	      MD_AMODE_PROBE(am, op, fsm);

	      /* update the addressing mode profile */
	      stat_add_sample(am_prof, (int)am);

	      /* addressing mode pre-probe FSM, after all loads and stores */
	      MD_AMODE_POSTPROBE(fsm);
	    }
	}

      if (prof_seg)
	{
	  if (flags & F_MEM)
	    {
	      /* update instruction profile */
	      stat_add_sample(seg_prof, (int)bind_to_seg(addr));
	    }
	}

      if (prof_tsyms)
	{
	  int tindex;

	  /* attempt to bind inst address to a text segment symbol */
	  sym_bind_addr(regs.regs_PC, &tindex, /* !exact */FALSE, sdb_text);

	  if (tindex >= 0)
	    {
	      if (tindex > sym_ntextsyms)
		panic("bogus text symbol index");

	      stat_add_sample(tsym_prof, tindex);
	    }
	  /* else, could not bind to a symbol */
	}

      if (prof_dsyms)
	{
	  int dindex;

	  if (flags & F_MEM)
	    {
	      /* attempt to bind inst address to a text segment symbol */
	      sym_bind_addr(addr, &dindex, /* !exact */FALSE, sdb_data);

	      if (dindex >= 0)
		{
		  if (dindex > sym_ndatasyms)
		    panic("bogus data symbol index");

		  stat_add_sample(dsym_prof, dindex);
		}
	      /* else, could not bind to a symbol */
	    }
	}

      if (prof_taddr)
	{
	  /* add regs_PC exec event to text address profile */
	  stat_add_sample(taddr_prof, regs.regs_PC);
	}

      /* update any stats tracked by PC */
      for (i=0; i<pcstat_nelt; i++)
	{
	  counter_t newval;
	  int delta;

	  /* check if any tracked stats changed */
	  newval = STATVAL(pcstat_stats[i]);
	  delta = newval - pcstat_lastvals[i];
	  if (delta != 0)
	    {
	      stat_add_samples(pcstat_sdists[i], regs.regs_PC, delta);
	      pcstat_lastvals[i] = newval;
	    }

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
