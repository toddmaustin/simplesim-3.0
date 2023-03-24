/* alpha.h - Alpha ISA definitions */

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


#ifndef ALPHA_H
#define ALPHA_H

#include <stdio.h>

#include "host.h"
#include "misc.h"
#include "config.h"
#include "endian.h"


/*
 * This file contains various definitions needed to decode, disassemble, and
 * execute Alpha AXP instructions.
 */

/* build for Alpha AXP target */
#define TARGET_ALPHA

/* probe cross-endian execution */
#if defined(BYTES_BIG_ENDIAN)
#define MD_CROSS_ENDIAN
#endif

/* not applicable/available, usable in most definition contexts */
#define NA		0

/*
 * target-dependent type definitions
 */

/* define MD_QWORD_ADDRS if the target requires 64-bit (qword) addresses */
#define MD_QWORD_ADDRS

/* address type definition */
typedef qword_t md_addr_t;


/*
 * target-dependent memory module configuration
 */

/* physical memory page size (must be a power-of-two) */
#define MD_PAGE_SIZE		8192
#define MD_LOG_PAGE_SIZE	13


/*
 * target-dependent instruction faults
 */

enum md_fault_type {
  md_fault_none = 0,		/* no fault */
  md_fault_access,		/* storage access fault */
  md_fault_alignment,		/* storage alignment fault */
  md_fault_overflow,		/* signed arithmetic overflow fault */
  md_fault_div0,		/* division by zero fault */
  md_fault_invalid,             /* invalid arithmetic operation */ 
                                /* added to allow SQRT{S,T} in FIX exts */
  md_fault_break,		/* BREAK instruction fault */
  md_fault_unimpl,		/* unimplemented instruction fault */
  md_fault_internal		/* internal S/W fault */
};


/*
 * target-dependent register file definitions, used by regs.[hc]
 */

/* number of integer registers */
#define MD_NUM_IREGS		32

/* number of floating point registers */
#define MD_NUM_FREGS		32

/* number of control registers */
#define MD_NUM_CREGS		2

/* total number of registers, excluding PC and NPC */
#define MD_TOTAL_REGS							\
  (/*int*/32 + /*fp*/32 + /*misc*/2 + /*tmp*/1 + /*mem*/1 + /*ctrl*/1)

/* general purpose (integer) register file entry type */
typedef qword_t md_gpr_t[MD_NUM_IREGS];

/* floating point register file entry type */
typedef union {
  qword_t q[MD_NUM_FREGS];	/* integer qword view */
  dfloat_t d[MD_NUM_FREGS];	/* double-precision floating point view */
} md_fpr_t;

/* control register file contents */
typedef struct {
  qword_t fpcr;			/* floating point condition codes */
  qword_t uniq;			/* process-unique register */
} md_ctrl_t;

/* well known registers */
enum md_reg_names {
  MD_REG_V0 = 0,	/* return value reg */
  MD_REG_ERR = 7,
  MD_REG_FP = 15,	/* frame pointer */
  MD_REG_A0 = 16,	/* argument regs */
  MD_REG_A1 = 17,
  MD_REG_A2 = 18,
  MD_REG_A3 = 19,
  MD_REG_A4 = 20,
  MD_REG_A5 = 21,
  MD_REG_RA = 26,	/* return address reg */
  MD_REG_GP = 29,	/* global data section pointer */
  MD_REG_SP = 30,	/* stack pointer */
  MD_REG_ZERO = 31	/* zero register */
};


/*
 * target-dependent instruction format definition
 */

/* instruction formats */
typedef word_t md_inst_t;

/* preferred nop instruction definition */
extern md_inst_t MD_NOP_INST;

/* target swap support */
#ifdef MD_CROSS_ENDIAN

#define MD_SWAPH(X)		SWAP_HALF(X)
#define MD_SWAPW(X)		SWAP_WORD(X)
#define MD_SWAPQ(X)		SWAP_QWORD(X)
#define MD_SWAPI(X)		SWAP_WORD(X)

#else /* !MD_CROSS_ENDIAN */

#define MD_SWAPH(X)		(X)
#define MD_SWAPW(X)		(X)
#define MD_SWAPQ(X)		(X)
#define MD_SWAPD(X)		(X)
#define MD_SWAPI(X)		(X)

#endif

/* fetch an instruction */
#define MD_FETCH_INST(INST, MEM, PC)					\
  { (INST) = MEM_READ_WORD((MEM), (PC)); }

/*
 * target-dependent loader module configuration
 */

/* maximum size of argc+argv+envp environment */
#define MD_MAX_ENVIRON		16384


/*
 * machine.def specific definitions
 */

/* inst -> enum md_opcode mapping, use this macro to decode insts */
#define MD_TOP_OP(INST)		(((INST) >> 26) & 0x3f)
#define MD_SET_OPCODE(OP, INST)						\
  { OP = md_mask2op[MD_TOP_OP(INST)];					\
    while (md_opmask[OP])						\
      OP = md_mask2op[((INST >> md_opshift[OP]) & md_opmask[OP])	\
		      + md_opoffset[OP]]; }

/* largest opcode field value (currently upper 8-bit are used for pre/post-
    incr/decr operation specifiers */
#define MD_MAX_MASK		2048

/* global opcode names, these are returned by the decoder (MD_OP_ENUM()) */
enum md_opcode {
  OP_NA = 0,	/* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3) OP,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) OP,
#define CONNECT(OP)
#include "machine.def"
  OP_MAX	/* number of opcodes + NA */
};

/* internal decoder state */
extern enum md_opcode md_mask2op[];
extern unsigned int md_opoffset[];
extern unsigned int md_opmask[];
extern unsigned int md_opshift[];

/* enum md_opcode -> description string */
#define MD_OP_NAME(OP)		(md_op2name[OP])
extern char *md_op2name[];

/* enum md_opcode -> opcode operand format, used by disassembler */
#define MD_OP_FORMAT(OP)	(md_op2format[OP])
extern char *md_op2format[];

/* function unit classes, update md_fu2name if you update this definition */
enum md_fu_class {
  FUClamd_NA = 0,	/* inst does not use a functional unit */
  IntALU,		/* integer ALU */
  IntMULT,		/* integer multiplier */
  IntDIV,		/* integer divider */
  FloatADD,		/* floating point adder/subtractor */
  FloatCMP,		/* floating point comparator */
  FloatCVT,		/* floating point<->integer converter */
  FloatMULT,		/* floating point multiplier */
  FloatDIV,		/* floating point divider */
  FloatSQRT,		/* floating point square root */
  RdPort,		/* memory read port */
  WrPort,		/* memory write port */
  NUM_FU_CLASSES	/* total functional unit classes */
};

/* enum md_opcode -> enum md_fu_class, used by performance simulators */
#define MD_OP_FUCLASS(OP)	(md_op2fu[OP])
extern enum md_fu_class md_op2fu[];

/* enum md_fu_class -> description string */
#define MD_FU_NAME(FU)		(md_fu2name[FU])
extern char *md_fu2name[];

/* instruction flags */
#define F_ICOMP		0x00000001	/* integer computation */
#define F_FCOMP		0x00000002	/* FP computation */
#define F_CTRL		0x00000004	/* control inst */
#define F_UNCOND	0x00000008	/*   unconditional change */
#define F_COND		0x00000010	/*   conditional change */
#define F_MEM		0x00000020	/* memory access inst */
#define F_LOAD		0x00000040	/*   load inst */
#define F_STORE		0x00000080	/*   store inst */
#define F_DISP		0x00000100	/*   displaced (R+C) addr mode */
#define F_RR		0x00000200	/*   R+R addr mode */
#define F_DIRECT	0x00000400	/*   direct addressing mode */
#define F_TRAP		0x00000800	/* traping inst */
#define F_LONGLAT	0x00001000	/* long latency inst (for sched) */
#define F_DIRJMP	0x00002000	/* direct jump */
#define F_INDIRJMP	0x00004000	/* indirect jump */
#define F_CALL		0x00008000	/* function call */
#define F_FPCOND	0x00010000	/* FP conditional branch */
#define F_IMM		0x00020000	/* instruction has immediate operand */

/* enum md_opcode -> opcode flags, used by simulators */
#define MD_OP_FLAGS(OP)		(md_op2flags[OP])
extern unsigned int md_op2flags[];


/* integer register specifiers */
#define RA		((inst >> 21) & 0x1f)		/* reg source #1 */
#define RB		((inst >> 16) & 0x1f)		/* reg source #2 */
#define RC		(inst & 0x1f)			/* reg dest */

/* returns 8-bit unsigned immediate field value */
#define IMM		((qword_t)((inst >> 13) & 0xff))

/* returns 21-bit unsigned absolute jump target field value */
#define TARG		(inst & 0x1fffff)

/* load/store 16-bit unsigned offset field value */
#define OFS		(inst & 0xffff)

/* sign-extend operands */
#define SEXT(X)								\
  (((X) & 0x8000) ? ((sqword_t)(X) | LL(0xffffffffffff0000)) : (sqword_t)(X))

#define SEXT21(X)							\
  (((X) & 0x100000) ? ((sqword_t)(X) | LL(0xffffffffffe00000)) : (sqword_t)(X))

#define SEXT32(X)							\
  (((X) & 0x80000000) ? ((sqword_t)(X)|LL(0xffffffff00000000)) : (sqword_t)(X))

/* test for arithmetic overflow */
#define ARITH_OVFL(RESULT, OP1, OP2) ((RESULT) < (OP1) || (RESULT) < (OP2))

/* test for NaN */
#define IEEEFP_DBL_SIGN(Q)	((Q) >> 63)
#define IEEEFP_DBL_EXPONENT(Q)	(((Q) >> 52) & 0x7ff)
#define IEEEFP_DBL_FRACTION(Q)	((Q) & ULL(0xfffffffffffff))
#define IS_IEEEFP_DBL_NAN(Q)						\
  ((IEEEFP_DBL_EXPONENT(Q) == 0x7ff) && (IEEEFP_DBL_FRACTION(Q)))

/* default target PC handling */
#ifndef SET_TPC
#define SET_TPC(PC)	(void)0
#endif /* SET_TPC */


/*
 * various other helper macros/functions
 */

/* non-zero if system call is an exit() */
#define OSF_SYS_exit			1
#define MD_EXIT_SYSCALL(REGS)						\
  ((REGS)->regs_R[MD_REG_V0] == OSF_SYS_exit)

/* non-zero if system call is a write to stdout/stderr */
#define OSF_SYS_write			4
#define MD_OUTPUT_SYSCALL(REGS)						\
  ((REGS)->regs_R[MD_REG_V0] == OSF_SYS_write				\
   && ((REGS)->regs_R[MD_REG_A0] == /* stdout */1			\
       || (REGS)->regs_R[MD_REG_A0] == /* stderr */2))

/* returns stream of an output system call, translated to host */
#define MD_STREAM_FILENO(REGS)		((REGS)->regs_R[MD_REG_A0])

/* returns non-zero if instruction is a function call */
#define MD_IS_CALL(OP)			((OP) == JSR || (OP) == BSR)

/* returns non-zero if instruction is a function return */
#define MD_IS_RETURN(OP)		((OP) == RETN)

/* returns non-zero if instruction is an indirect jump */
#define MD_IS_INDIR(OP)							\
  ((OP) == JMP || (OP) == JSR || (OP) == RETN || (OP) == JSR_COROUTINE)

/* addressing mode probe, enums and strings */
enum md_amode_type {
  md_amode_imm,		/* immediate addressing mode */
  md_amode_gp,		/* global data access through global pointer */
  md_amode_sp,		/* stack access through stack pointer */
  md_amode_fp,		/* stack access through frame pointer */
  md_amode_disp,	/* (reg + const) addressing */
  md_amode_rr,		/* (reg + reg) addressing */
  md_amode_NUM
};
extern char *md_amode_str[md_amode_NUM];

/* addressing mode pre-probe FSM, must see all instructions */
#define MD_AMODE_PREPROBE(OP, FSM)		{ (FSM) = 0; }

/* compute addressing mode, only for loads/stores */
#define MD_AMODE_PROBE(AM, OP, FSM)					\
  {									\
    if (MD_OP_FLAGS(OP) & F_DISP)					\
      {									\
	if ((RB) == MD_REG_GP)						\
	  (AM) = md_amode_gp;						\
	else if ((RB) == MD_REG_SP)					\
	  (AM) = md_amode_sp;						\
	else if ((RB) == MD_REG_FP) /* && bind_to_seg(addr) == seg_stack */\
	  (AM) = md_amode_fp;						\
	else								\
	  (AM) = md_amode_disp;						\
      }									\
    else if (MD_OP_FLAGS(OP) & F_RR)					\
      (AM) = md_amode_rr;						\
    else								\
      panic("cannot decode addressing mode");				\
  }

/* addressing mode pre-probe FSM, after all loads and stores */
#define MD_AMODE_POSTPROBE(FSM)			/* nada... */


/*
 * EIO package configuration/macros
 */

/* expected EIO file format */
#define MD_EIO_FILE_FORMAT		EIO_ALPHA_FORMAT

#define MD_MISC_REGS_TO_EXO(REGS)					\
  exo_new(ec_list,							\
	  /*icnt*/exo_new(ec_integer, (exo_integer_t)sim_num_insn),	\
	  /*PC*/exo_new(ec_address, (exo_integer_t)(REGS)->regs_PC),	\
	  /*NPC*/exo_new(ec_address, (exo_integer_t)(REGS)->regs_NPC),	\
	  /*FPCR*/exo_new(ec_integer, (exo_integer_t)(REGS)->regs_C.fpcr),\
	  /*UNIQ*/exo_new(ec_integer, (exo_integer_t)(REGS)->regs_C.uniq),\
	  NULL)

#define MD_IREG_TO_EXO(REGS, IDX)					\
  exo_new(ec_address, (exo_integer_t)(REGS)->regs_R[IDX])

#define MD_FREG_TO_EXO(REGS, IDX)					\
  exo_new(ec_address, (exo_integer_t)(REGS)->regs_F.q[IDX])

#define MD_EXO_TO_MISC_REGS(EXO, ICNT, REGS)				\
  /* check EXO format for errors... */					\
  if (!exo								\
      || exo->ec != ec_list						\
      || !exo->as_list.head						\
      || exo->as_list.head->ec != ec_integer				\
      || !exo->as_list.head->next					\
      || exo->as_list.head->next->ec != ec_address			\
      || !exo->as_list.head->next->next					\
      || exo->as_list.head->next->next->ec != ec_address		\
      || !exo->as_list.head->next->next->next				\
      || exo->as_list.head->next->next->next->ec != ec_integer		\
      || !exo->as_list.head->next->next->next->next			\
      || exo->as_list.head->next->next->next->next->ec != ec_integer	\
      || exo->as_list.head->next->next->next->next->next != NULL)	\
    fatal("could not read EIO misc regs");				\
  (ICNT) = (counter_t)exo->as_list.head->as_integer.val;		\
  (REGS)->regs_PC = (md_addr_t)exo->as_list.head->next->as_integer.val;	\
  (REGS)->regs_NPC =							\
    (md_addr_t)exo->as_list.head->next->next->as_integer.val;		\
  (REGS)->regs_C.fpcr =							\
    (qword_t)exo->as_list.head->next->next->next->as_integer.val;	\
  (REGS)->regs_C.uniq =							\
    (qword_t)exo->as_list.head->next->next->next->next->as_integer.val;

#define MD_EXO_TO_IREG(EXO, REGS, IDX)					\
  ((REGS)->regs_R[IDX] = (qword_t)(EXO)->as_integer.val)

#define MD_EXO_TO_FREG(EXO, REGS, IDX)					\
  ((REGS)->regs_F.q[IDX] = (qword_t)(EXO)->as_integer.val)

#define MD_EXO_CMP_IREG(EXO, REGS, IDX)					\
  ((REGS)->regs_R[IDX] != (qword_t)(EXO)->as_integer.val)

#define MD_FIRST_IN_REG			0
#define MD_LAST_IN_REG			21

#define MD_FIRST_OUT_REG		0
#define MD_LAST_OUT_REG			21


/*
 * configure the EXO package
 */

/* EXO pointer class */
typedef qword_t exo_address_t;

/* EXO integer class, 64-bit encoding */
typedef qword_t exo_integer_t;

/* EXO floating point class, 64-bit encoding */
typedef double exo_float_t;


/*
 * configure the stats package
 */

/* counter stats */
#define stat_reg_counter		stat_reg_sqword
#define sc_counter			sc_sqword
#define for_counter			for_sqword

/* address stats */
#define stat_reg_addr			stat_reg_qword


/*
 * configure the DLite! debugger
 */

/* register bank specifier */
enum md_reg_type {
  rt_gpr,		/* general purpose register */
  rt_lpr,		/* integer-precision floating pointer register */
  rt_fpr,		/* single-precision floating pointer register */
  rt_dpr,		/* double-precision floating pointer register */
  rt_ctrl,		/* control register */
  rt_PC,		/* program counter */
  rt_NPC,		/* next program counter */
  rt_NUM
};

/* register name specifier */
struct md_reg_names_t {
  char *str;			/* register name */
  enum md_reg_type file;	/* register file */
  int reg;			/* register index */
};

/* symbolic register names, parser is case-insensitive */
extern struct md_reg_names_t md_reg_names[];

/* returns a register name string */
char *md_reg_name(enum md_reg_type rt, int reg);

/* default register accessor object */
struct eval_value_t;
struct regs_t;
char *						/* err str, NULL for no err */
md_reg_obj(struct regs_t *regs,			/* registers to access */
	   int is_write,			/* access type */
	   enum md_reg_type rt,			/* reg bank to probe */
	   int reg,				/* register number */
	   struct eval_value_t *val);		/* input, output */

/* print integer REG(S) to STREAM */
void md_print_ireg(md_gpr_t regs, int reg, FILE *stream);
void md_print_iregs(md_gpr_t regs, FILE *stream);

/* print floating point REG(S) to STREAM */
void md_print_fpreg(md_fpr_t regs, int reg, FILE *stream);
void md_print_fpregs(md_fpr_t regs, FILE *stream);

/* print control REG(S) to STREAM */
void md_print_creg(md_ctrl_t regs, int reg, FILE *stream);
void md_print_cregs(md_ctrl_t regs, FILE *stream);

/* xor checksum registers */
word_t md_xor_regs(struct regs_t *regs);


/*
 * configure sim-outorder specifics
 */

/* primitive operation used to compute addresses within pipeline */
#define MD_AGEN_OP		ADDQ

/* NOP operation when injected into the pipeline */
#define MD_NOP_OP		OP_NA

/* non-zero for a valid address, used to determine if speculative accesses
   should access the DL1 data cache */
#define MD_VALID_ADDR(ADDR)						\
  (((ADDR) >= ld_text_base && (ADDR) < (ld_text_base + ld_text_size))	\
   || ((ADDR) >= ld_data_base && (ADDR) < ld_brk_point)			\
   || ((ADDR) >= (ld_stack_base - 16*1024*1024) && (ADDR) < ld_stack_base))

/*
 * configure branch predictors
 */

/* shift used to ignore branch address least significant bits, usually
   log2(sizeof(md_inst_t)) */
#define MD_BR_SHIFT		2	/* log2(4) */


/*
 * target-dependent routines
 */

/* intialize the inst decoder, this function builds the ISA decode tables */
void md_init_decoder(void);

/* disassemble a SimpleScalar instruction */
void
md_print_insn(md_inst_t inst,		/* instruction to disassemble */
	      md_addr_t pc,		/* addr of inst, used for PC-rels */
	      FILE *stream);		/* output stream */

#endif /* ALPHA_H */







#if 0

/* instruction/address formats */
typedef qword_t MD_ADDR_TYPE;
typedef qword_t MD_PTR_TYPE;
typedef word_t MD_INST_TYPE;
#define MD_INST_SIZE		sizeof(MD_INST_TYPE)

/* virtual memory segment limits */
#define MD_TEXT_BASE		0x20000000ULL
#define MD_STACK_BASE 		(MD_TEXT_BASE - (409600+4096))

/* well known registers */
enum { REG_V0, REG_A0=16, REG_A1, REG_A2, REG_A3, REG_A4, REG_A5, REG_ERR=7,
       REG_GP=29, REG_SP, REG_ZERO, REG_RA=26 };

/* total number of register in processor 32I+32F+HI+LO+FCC+TMP+MEM+CTRL */
#define MD_TOTAL_REGS							\
  (MD_NUM_REGS+MD_NUM_REGS+/*FPCR*/1+/*UNIQ*/1+/*MEM*/1+/*CTRL*/1)

/* inst check macros, activated if NO_ICHECKS is not defined (default) */
#ifndef NO_ICHECKS

/* instruction failure notification macro, this can be defined by the
   target simulator if, for example, the simulator wants to handle the
   instruction fault in a machine specific fashion; a string describing
   the instruction fault is passed to the IFAIL() macro */
#ifndef IFAIL
#define IFAIL(S)	fatal(S)
#endif /* IFAIL */

/* check for overflow in X+Y, both signed */
#define OVER(X,Y)	(((((X) > 0) && ((Y) > 0)			\
			   && (MAXINT_VAL - (X) < (Y)))			\
			  ? IFAIL("+ overflow") : (void)0),		\
			 ((((X) < 0) && ((Y) < 0)			\
			   && (-MAXINT_VAL - (X) > (Y)))		\
			  ? IFAIL("+ underflow") : (void)0))

/* check for underflow in X-Y, both signed */
#define UNDER(X,Y)	(((((X) > 0) && ((Y) < 0)			\
			   && (MAXINT_VAL + (Y) < (X)))			\
			  ? IFAIL("- overflow") : (void)0),		\
			 ((((X) < 0) && ((Y) > 0)			\
			   && (-MAXINT_VAL + (Y) > (X)))		\
			  ? IFAIL("- underflow") : (void)0))

/* check for divide by zero error, N is denom */
#define DIV0(N)		(((N) == 0) ? IFAIL("divide by 0") : (void)0)

/* check reg specifier N for required double integer word alignment */
#define INTALIGN(N)	(((N) & 01)					\
			 ? IFAIL("bad INT register alignment") : (void)0)

/* check reg specifier N for required double FP word alignment */
#define FPALIGN(N)	(((N) & 01)					\
			 ? IFAIL("bad FP register alignment") : (void)0)

/* check target address TARG for required jump target alignment */
#define TALIGN(TARG)	(((TARG) & 0x7)					\
			 ? IFAIL("bad jump alignment") : (void)0)

#else /* NO_ICHECKS */

/* inst checks disables, change all checks to NOP expressions */
#define OVER(X,Y)	((void)0)
#define UNDER(X,Y)	((void)0)
#define DIV0(N)		((void)0)
#define INTALIGN(N)	((void)0)
#define FPALIGN(N)	((void)0)
#define TALIGN(TARG)	((void)0)

#endif /* NO_ICHECKS */

/* default division operator semantics, this operation is accessed through a
   macro because some simulators need to check for divide by zero faults
   before executing this operation */
#define IDIV(A, B)	((A) / (B))
#define IMOD(A, B)	((A) % (B))
#define FDIV(A, B)	((A) / (B))
#define FINT(A)		((int)A)

#endif
