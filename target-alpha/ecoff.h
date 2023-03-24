/* ecoff.h - SimpleScalar ECOFF definitions */

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


/* SimpleScalar ECOFF definitions */

#ifndef ECOFF_H
#define ECOFF_H

#include "alpha.h"

#define ECOFF_EB_MAGIC		0x0160
#define ECOFF_EB_OTHER		0x6001
#define ECOFF_EL_MAGIC		0x0162
#define ECOFF_EL_OTHER		0x6201
#define ECOFF_ALPHAMAGIC	0603

struct ecoff_filehdr {
  half_t f_magic;
  half_t f_nscns;
  sword_t f_timdat;
  qword_t f_symptr;
  sword_t f_nsyms;
  half_t f_opthdr;
  half_t f_flags;
};

struct ecoff_aouthdr {
  half_t magic;
  half_t vstamp;
  half_t bldrev;
  half_t padcell;
  qword_t tsize;
  qword_t dsize;
  qword_t bsize;
  qword_t entry;
  qword_t text_start;
  qword_t data_start;
  qword_t bss_start;
  sword_t gprmask;
  sword_t fprmask;
  qword_t gp_value;
};

struct ecoff_scnhdr {
  char s_name[8];
  qword_t s_paddr;
  qword_t s_vaddr;
  qword_t s_size;
  qword_t s_scnptr;
  qword_t s_relptr;
  qword_t s_lnnoptr;
  half_t s_nreloc;
  half_t s_nlnno;
  sword_t s_flags;
};

typedef struct ecoff_symhdr_t {
  half_t magic;
  half_t vstamp;

  sword_t ilineMax;		/* number of line number entries */
  sword_t idnMax;		/* max index into dense number table */
  sword_t ipdMax;		/* number of procedures */
  sword_t isymMax;		/* number of local symbols */
  sword_t ioptMax;		/* max index into optimization sym entries */
  sword_t iauxMax;		/* number of auxiliary symbol entries */
  sword_t issMax;		/* max index into local strings */
  sword_t issExtMax;		/* max index into external strings */
  sword_t ifdMax;		/* number of file descriptor entries */
  sword_t crfd;			/* number of relative file descriptor ents */
  sword_t iextMax;		/* max index into external symbols */
  qword_t cbLine;		/* number of bytes for line number entries */
  qword_t cbLineOffset;		/* offset to start of line number entries*/
  qword_t cbDnOffset;		/* offset to start dense number table */
  qword_t cbPdOffset;		/* offset to procedure descriptor table */
  qword_t cbSymOffset;		/* offset to start of local symbols */
  qword_t cbOptOffset;		/* offset to optimization symbol entries */
  qword_t cbAuxOffset;		/* offset to start of aux symbol entries */
  qword_t cbSsOffset;		/* offset to start of local strings */
  qword_t cbSsExtOffset;	/* offset to start of external strings */
  qword_t cbFdOffset;		/* offset to file descriptor table */
  qword_t cbRfdOffset;		/* offset to relative file descriptor table */
  qword_t cbExtOffset;		/* offset to start of ext symbol entries */
} ecoff_HDRR;

#define ECOFF_magicSym 0x1992

typedef struct ecoff_fdr {
  word_t adr;
  sword_t rss;
  sword_t issBase;
  sword_t cbSs;
  sword_t isymBase;
  sword_t csym;
  sword_t ilineBase;
  sword_t cline;
  sword_t ioptBase;
  sword_t copt;
  half_t ipdFirst;
  half_t cpd;
  sword_t iauxBase;
  sword_t caux;
  sword_t rfdBase;
  sword_t crfd;
  unsigned lang :5;
  unsigned fMerge :1;
  unsigned fReadin :1;
  unsigned fBigendian :1;
  unsigned reserved :24;
  sword_t cbLineOffset;
  sword_t cbLine;
} ecoff_FDR;

typedef struct ecoff_pdr {
  word_t adr;
  sword_t isym;
  sword_t iline;
  sword_t regmask;
  sword_t regoffset;
  sword_t iopt;
  sword_t fregmask;
  sword_t fregoffset;
  sword_t frameoffset;
  half_t framereg;
  half_t pcreg;
  sword_t lnLow;
  sword_t lnHigh;
  sword_t cbLineOffset;
} ecoff_PDR;

typedef struct ecoff_SYMR {
  qword_t value;
  sword_t iss;
  unsigned st :6;
  unsigned sc :5;
  unsigned reserved :1;
  unsigned index :20;
} ecoff_SYMR;

typedef struct ecoff_EXTR {
  ecoff_SYMR asym;
  sword_t ifd;
  sword_t reserved;
} ecoff_EXTR;

#define ECOFF_R_SN_TEXT		1
#define ECOFF_R_SN_RDATA	2
#define ECOFF_R_SN_DATA		3
#define ECOFF_R_SN_SDATA	4
#define ECOFF_R_SN_SBSS		5
#define ECOFF_R_SN_BSS		6

#define ECOFF_STYP_TEXT		0x0020
#define ECOFF_STYP_RDATA	0x0100
#define ECOFF_STYP_DATA		0x0040
#define ECOFF_STYP_SDATA	0x0200
#define ECOFF_STYP_SBSS		0x0400
#define ECOFF_STYP_BSS		0x0080
#define ECOFF_STYP_INIT		0x80000000	/* sect only contains the text
						   insts for the .init sec. */
#define ECOFF_STYP_FINI		0x01000000	/* insts for .fini */
#define ECOFF_STYP_RCONST	0x02200000	/* overloads with reginfo */
#define ECOFF_STYP_LITA		0x04000000	/* addr literals only        */
#define ECOFF_STYP_LIT8		0x08000000	/* lit pool for 8 byte lits */
#define ECOFF_STYP_LIT4		0x10000000	/* lit pool for 4 byte lits */
#define ECOFF_STYP_XDATA	0x02400000	/* exception sections */
#define ECOFF_STYP_PDATA	0x02800000	/* exception sections */

#define ECOFF_stNil		0
#define ECOFF_stGlobal		1
#define ECOFF_stStatic		2
#define ECOFF_stParam		3
#define ECOFF_stLocal		4
#define ECOFF_stLabel		5
#define ECOFF_stProc		6
#define ECOFF_stBlock		7
#define ECOFF_stEnd		8
#define ECOFF_stMember		9
#define ECOFF_stTypedef		10
#define ECOFF_stFile		11
#define ECOFF_stRegReloc	12
#define ECOFF_stForward		13
#define ECOFF_stStaticProc	14
#define ECOFF_stConstant	15

#endif /* ECOFF_H */
