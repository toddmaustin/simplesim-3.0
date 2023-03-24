/* loader.c - program loader routines */

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
#include "endian.h"
#include "regs.h"
#include "memory.h"
#include "sim.h"
#include "eio.h"
#include "loader.h"

#ifdef BFD_LOADER
#include <bfd.h>
#else /* !BFD_LOADER */
#include "target-pisa/ecoff.h"
#endif /* BFD_LOADER */

/* amount of tail padding added to all loaded text segments */
#define TEXT_TAIL_PADDING 128

/* program text (code) segment base */
md_addr_t ld_text_base = 0;

/* program text (code) size in bytes */
unsigned int ld_text_size = 0;

/* program initialized data segment base */
md_addr_t ld_data_base = 0;

/* program initialized ".data" and uninitialized ".bss" size in bytes */
unsigned int ld_data_size = 0;

/* top of the data segment */
md_addr_t ld_brk_point = 0;

/* program stack segment base (highest address in stack) */
md_addr_t ld_stack_base = MD_STACK_BASE;

/* program initial stack size */
unsigned int ld_stack_size = 0;

/* lowest address accessed on the stack */
md_addr_t ld_stack_min = (md_addr_t)-1;

/* program file name */
char *ld_prog_fname = NULL;

/* program entry point (initial PC) */
md_addr_t ld_prog_entry = 0;

/* program environment base address address */
md_addr_t ld_environ_base = 0;

/* target executable endian-ness, non-zero if big endian */
int ld_target_big_endian;

/* register simulator-specific statistics */
void
ld_reg_stats(struct stat_sdb_t *sdb)	/* stats data base */
{
  stat_reg_addr(sdb, "ld_text_base",
		"program text (code) segment base",
		&ld_text_base, ld_text_base, "  0x%08p");
  stat_reg_uint(sdb, "ld_text_size",
		"program text (code) size in bytes",
		&ld_text_size, ld_text_size, NULL);
  stat_reg_addr(sdb, "ld_data_base",
		"program initialized data segment base",
		&ld_data_base, ld_data_base, "  0x%08p");
  stat_reg_uint(sdb, "ld_data_size",
		"program init'ed `.data' and uninit'ed `.bss' size in bytes",
		&ld_data_size, ld_data_size, NULL);
  stat_reg_addr(sdb, "ld_stack_base",
		"program stack segment base (highest address in stack)",
		&ld_stack_base, ld_stack_base, "  0x%08p");
  stat_reg_uint(sdb, "ld_stack_size",
		"program initial stack size",
		&ld_stack_size, ld_stack_size, NULL);
#if 0 /* FIXME: broken... */
  stat_reg_addr(sdb, "ld_stack_min",
		"program stack segment lowest address",
		&ld_stack_min, ld_stack_min, "  0x%08p");
#endif
  stat_reg_addr(sdb, "ld_prog_entry",
		"program entry point (initial PC)",
		&ld_prog_entry, ld_prog_entry, "  0x%08p");
  stat_reg_addr(sdb, "ld_environ_base",
		"program environment base address address",
		&ld_environ_base, ld_environ_base, "  0x%08p");
  stat_reg_int(sdb, "ld_target_big_endian",
	       "target executable endian-ness, non-zero if big endian",
	       &ld_target_big_endian, ld_target_big_endian, NULL);
}


/* load program text and initialized data into simulated virtual memory
   space and initialize program segment range variables */
void
ld_load_prog(char *fname,		/* program to load */
	     int argc, char **argv,	/* simulated program cmd line args */
	     char **envp,		/* simulated program environment */
	     struct regs_t *regs,	/* registers to initialize for load */
	     struct mem_t *mem,		/* memory space to load prog into */
	     int zero_bss_segs)		/* zero uninit data segment? */
{
  int i;
  word_t temp;
  md_addr_t sp, data_break = 0, null_ptr = 0, argv_addr, envp_addr;

  if (eio_valid(fname))
    {
      if (argc != 1)
	{
	  fprintf(stderr, "error: EIO file has arguments\n");
	  exit(1);
	}

      fprintf(stderr, "sim: loading EIO file: %s\n", fname);

      sim_eio_fname = mystrdup(fname);

      /* open the EIO file stream */
      sim_eio_fd = eio_open(fname);

      /* load initial state checkpoint */
      if (eio_read_chkpt(regs, mem, sim_eio_fd) != -1)
	fatal("bad initial checkpoint in EIO file");

      /* load checkpoint? */
      if (sim_chkpt_fname != NULL)
	{
	  counter_t restore_icnt;

	  FILE *chkpt_fd;

	  fprintf(stderr, "sim: loading checkpoint file: %s\n",
		  sim_chkpt_fname);

	  if (!eio_valid(sim_chkpt_fname))
	    fatal("file `%s' does not appear to be a checkpoint file",
		  sim_chkpt_fname);

	  /* open the checkpoint file */
	  chkpt_fd = eio_open(sim_chkpt_fname);

	  /* load the state image */
	  restore_icnt = eio_read_chkpt(regs, mem, chkpt_fd);

	  /* fast forward the baseline EIO trace to checkpoint location */
	  myfprintf(stderr, "sim: fast forwarding to instruction %n\n",
		    restore_icnt);
	  eio_fast_forward(sim_eio_fd, restore_icnt);
	}

      /* computed state... */
      ld_environ_base = regs->regs_R[MD_REG_SP];
      ld_prog_entry = regs->regs_PC;

      /* fini... */
      return;
    }
#ifdef MD_CROSS_ENDIAN
  else
    {
      fatal("SimpleScalar/PISA only supports binary execution on\n"
	    "       same-endian hosts, use EIO files on cross-endian hosts");
    }
#endif /* MD_CROSS_ENDIAN */


  if (sim_chkpt_fname != NULL)
    fatal("checkpoints only supported while EIO tracing");

#ifdef BFD_LOADER

  {
    bfd *abfd;
    asection *sect;

    /* set up a local stack pointer, this is where the argv and envp
       data is written into program memory */
    ld_stack_base = MD_STACK_BASE;
    sp = ROUND_DOWN(MD_STACK_BASE - MD_MAX_ENVIRON, sizeof(dfloat_t));
    ld_stack_size = ld_stack_base - sp;

    /* initial stack pointer value */
    ld_environ_base = sp;

    /* load the program into memory, try both endians */
    if (!(abfd = bfd_openr(argv[0], "ss-coff-big")))
      if (!(abfd = bfd_openr(argv[0], "ss-coff-little")))
	fatal("cannot open executable `%s'", argv[0]);

    /* this call is mainly for its side effect of reading in the sections.
       we follow the traditional behavior of `strings' in that we don't
       complain if we don't recognize a file to be an object file.  */
    if (!bfd_check_format(abfd, bfd_object))
      {
	bfd_close(abfd);
	fatal("cannot open executable `%s'", argv[0]);
      }

    /* record profile file name */
    ld_prog_fname = argv[0];

    /* record endian of target */
    ld_target_big_endian = abfd->xvec->byteorder_big_p;

    debug("processing %d sections in `%s'...",
	  bfd_count_sections(abfd), argv[0]);

    /* read all sections in file */
    for (sect=abfd->sections; sect; sect=sect->next)
      {
	char *p;

	debug("processing section `%s', %d bytes @ 0x%08x...",
	      bfd_section_name(abfd, sect), bfd_section_size(abfd, sect),
	      bfd_section_vma(abfd, sect));

	/* read the section data, if allocated and loadable and non-NULL */
	if ((bfd_get_section_flags(abfd, sect) & SEC_ALLOC)
	    && (bfd_get_section_flags(abfd, sect) & SEC_LOAD)
	    && bfd_section_vma(abfd, sect)
	    && bfd_section_size(abfd, sect))
	  {
	    /* allocate a section buffer */
	    p = calloc(bfd_section_size(abfd, sect), sizeof(char));
	    if (!p)
	      fatal("cannot allocate %d bytes for section `%s'",
		    bfd_section_size(abfd, sect),
		    bfd_section_name(abfd, sect));

	    if (!bfd_get_section_contents(abfd, sect, p, (file_ptr)0,
					  bfd_section_size(abfd, sect)))
	      fatal("could not read entire `%s' section from executable",
		    bfd_section_name(abfd, sect));

	    /* copy program section it into simulator target memory */
	    mem_bcopy(mem_access, mem, Write, bfd_section_vma(abfd, sect),
		      p, bfd_section_size(abfd, sect));

	    /* release the section buffer */
	    free(p);
	  }
	/* zero out the section if it is loadable but not allocated in exec */
	else if (zero_bss_segs
		 && (bfd_get_section_flags(abfd, sect) & SEC_LOAD)
		 && bfd_section_vma(abfd, sect)
		 && bfd_section_size(abfd, sect))
	  {
	    /* zero out the section region */
	    mem_bzero(mem_access, mem,
		      bfd_section_vma(abfd, sect),
		      bfd_section_size(abfd, sect));
	  }
	else
	  {
	    /* else do nothing with this section, it's probably debug data */
	    debug("ignoring section `%s' during load...",
		  bfd_section_name(abfd, sect));
	  }

	/* expected text section */
	if (!strcmp(bfd_section_name(abfd, sect), ".text"))
	  {
	    /* .text section processing */
	    ld_text_size =
	      ((bfd_section_vma(abfd, sect) + bfd_section_size(abfd, sect))
	       - MD_TEXT_BASE)
		+ /* for speculative fetches/decodes */TEXT_TAIL_PADDING;

	    /* create tail padding and copy into simulator target memory */
	    mem_bzero(mem_access, mem,
		      bfd_section_vma(abfd, sect)
		      + bfd_section_size(abfd, sect),
		      TEXT_TAIL_PADDING);
	  }
	/* expected data sections */
	else if (!strcmp(bfd_section_name(abfd, sect), ".rdata")
		 || !strcmp(bfd_section_name(abfd, sect), ".data")
		 || !strcmp(bfd_section_name(abfd, sect), ".sdata")
		 || !strcmp(bfd_section_name(abfd, sect), ".bss")
		 || !strcmp(bfd_section_name(abfd, sect), ".sbss"))
	  {
	    /* data section processing */
	    if (bfd_section_vma(abfd, sect) + bfd_section_size(abfd, sect) >
		data_break)
	      data_break = (bfd_section_vma(abfd, sect) +
			    bfd_section_size(abfd, sect));
	  }
	else
	  {
	    /* what is this section??? */
	    fatal("encountered unknown section `%s', %d bytes @ 0x%08x",
		  bfd_section_name(abfd, sect), bfd_section_size(abfd, sect),
		  bfd_section_vma(abfd, sect));
	  }
      }

    /* compute data segment size from data break point */
    ld_text_base = MD_TEXT_BASE;
    ld_data_base = MD_DATA_BASE;
    ld_prog_entry = bfd_get_start_address(abfd);
    ld_data_size = data_break - ld_data_base;

    /* done with the executable, close it */
    if (!bfd_close(abfd))
      fatal("could not close executable `%s'", argv[0]);
  }

#else /* !BFD_LOADER, i.e., standalone loader */

  {
    FILE *fobj;
    long floc;
    struct ecoff_filehdr fhdr;
    struct ecoff_aouthdr ahdr;
    struct ecoff_scnhdr shdr;

    /* set up a local stack pointer, this is where the argv and envp
       data is written into program memory */
    ld_stack_base = MD_STACK_BASE;
    sp = ROUND_DOWN(MD_STACK_BASE - MD_MAX_ENVIRON, sizeof(dfloat_t));
    ld_stack_size = ld_stack_base - sp;

    /* initial stack pointer value */
    ld_environ_base = sp;

    /* record profile file name */
    ld_prog_fname = argv[0];

    /* load the program into memory, try both endians */
#if defined(__CYGWIN32__) || defined(_MSC_VER)
    fobj = fopen(argv[0], "rb");
#else
    fobj = fopen(argv[0], "r");
#endif
    if (!fobj)
      fatal("cannot open executable `%s'", argv[0]);

    if (fread(&fhdr, sizeof(struct ecoff_filehdr), 1, fobj) < 1)
      fatal("cannot read header from executable `%s'", argv[0]);

    /* record endian of target */
    if (fhdr.f_magic == ECOFF_EB_MAGIC)
      ld_target_big_endian = TRUE;
    else if (fhdr.f_magic == ECOFF_EL_MAGIC)
      ld_target_big_endian = FALSE;
    else if (fhdr.f_magic == ECOFF_EB_OTHER || fhdr.f_magic == ECOFF_EL_OTHER)
      fatal("PISA binary `%s' has wrong endian format", argv[0]);
    else if (fhdr.f_magic == ECOFF_ALPHAMAGIC)
      fatal("PISA simulator cannot run Alpha binary `%s'", argv[0]);
    else
      fatal("bad magic number in executable `%s' (not an executable?)",
	    argv[0]);

    if (fread(&ahdr, sizeof(struct ecoff_aouthdr), 1, fobj) < 1)
      fatal("cannot read AOUT header from executable `%s'", argv[0]);

    data_break = MD_DATA_BASE + ahdr.dsize + ahdr.bsize;

#if 0
    Data_start = ahdr.data_start;
    Data_size = ahdr.dsize;
    Bss_size = ahdr.bsize;
    Bss_start = ahdr.bss_start;
    Gp_value = ahdr.gp_value;
    Text_entry = ahdr.entry;
#endif

    /* seek to the beginning of the first section header, the file header comes
       first, followed by the optional header (this is the aouthdr), the size
       of the aouthdr is given in Fdhr.f_opthdr */
    fseek(fobj, sizeof(struct ecoff_filehdr) + fhdr.f_opthdr, 0);

    debug("processing %d sections in `%s'...", fhdr.f_nscns, argv[0]);

    /* loop through the section headers */
    floc = ftell(fobj);
    for (i = 0; i < fhdr.f_nscns; i++)
      {
	char *p;

	if (fseek(fobj, floc, 0) == -1)
	  fatal("could not reset location in executable");
	if (fread(&shdr, sizeof(struct ecoff_scnhdr), 1, fobj) < 1)
	  fatal("could not read section %d from executable", i);
	floc = ftell(fobj);

	switch (shdr.s_flags)
	  {
	  case ECOFF_STYP_TEXT:
	    ld_text_size = ((shdr.s_vaddr + shdr.s_size) - MD_TEXT_BASE) 
	      + TEXT_TAIL_PADDING;

	    p = calloc(shdr.s_size, sizeof(char));
	    if (!p)
	      fatal("out of virtual memory");

	    if (fseek(fobj, shdr.s_scnptr, 0) == -1)
	      fatal("could not read `.text' from executable", i);
	    if (fread(p, shdr.s_size, 1, fobj) < 1)
	      fatal("could not read text section from executable");

	    /* copy program section into simulator target memory */
	    mem_bcopy(mem_access, mem, Write, shdr.s_vaddr, p, shdr.s_size);

	    /* create tail padding and copy into simulator target memory */
	    mem_bzero(mem_access, mem,
		      shdr.s_vaddr + shdr.s_size, TEXT_TAIL_PADDING);
  
	    /* release the section buffer */
	    free(p);

#if 0
	    Text_seek = shdr.s_scnptr;
	    Text_start = shdr.s_vaddr;
	    Text_size = shdr.s_size / 4;
	    /* there is a null routine after the supposed end of text */
	    Text_size += 10;
	    Text_end = Text_start + Text_size * 4;
	    /* create_text_reloc(shdr.s_relptr, shdr.s_nreloc); */
#endif
	    break;

	  case ECOFF_STYP_RDATA:
	    /* The .rdata section is sometimes placed before the text
	     * section instead of being contiguous with the .data section.
	     */
#if 0
	    Rdata_start = shdr.s_vaddr;
	    Rdata_size = shdr.s_size;
	    Rdata_seek = shdr.s_scnptr;
#endif
	    /* fall through */
	  case ECOFF_STYP_DATA:
#if 0
	    Data_seek = shdr.s_scnptr;
#endif
	    /* fall through */
	  case ECOFF_STYP_SDATA:
#if 0
	    Sdata_seek = shdr.s_scnptr;
#endif

	    p = calloc(shdr.s_size, sizeof(char));
	    if (!p)
	      fatal("out of virtual memory");

	    if (fseek(fobj, shdr.s_scnptr, 0) == -1)
	      fatal("could not read `.text' from executable", i);
	    if (fread(p, shdr.s_size, 1, fobj) < 1)
	      fatal("could not read text section from executable");

	    /* copy program section it into simulator target memory */
	    mem_bcopy(mem_access, mem, Write, shdr.s_vaddr, p, shdr.s_size);

	    /* release the section buffer */
	    free(p);

	    break;

	  case ECOFF_STYP_BSS:
	    break;

	  case ECOFF_STYP_SBSS:
	    break;
	  }
      }

    /* compute data segment size from data break point */
    ld_text_base = MD_TEXT_BASE;
    ld_data_base = MD_DATA_BASE;
    ld_prog_entry = ahdr.entry;
    ld_data_size = data_break - ld_data_base;

    /* done with the executable, close it */
    if (fclose(fobj))
      fatal("could not close executable `%s'", argv[0]);
  }
#endif /* BFD_LOADER */

  /* perform sanity checks on segment ranges */
  if (!ld_text_base || !ld_text_size)
    fatal("executable is missing a `.text' section");
  if (!ld_data_base || !ld_data_size)
    fatal("executable is missing a `.data' section");
  if (!ld_prog_entry)
    fatal("program entry point not specified");

  /* determine byte/words swapping required to execute on this host */
  sim_swap_bytes = (endian_host_byte_order() != endian_target_byte_order());
  if (sim_swap_bytes)
    {
#if 0 /* FIXME: disabled until further notice... */
      /* cross-endian is never reliable, why this is so is beyond the scope
	 of this comment, e-mail me for details... */
      fprintf(stderr, "sim: *WARNING*: swapping bytes to match host...\n");
      fprintf(stderr, "sim: *WARNING*: swapping may break your program!\n");
#else
      fatal("binary endian does not match host endian");
#endif
    }
  sim_swap_words = (endian_host_word_order() != endian_target_word_order());
  if (sim_swap_words)
    {
#if 0 /* FIXME: disabled until further notice... */
      /* cross-endian is never reliable, why this is so is beyond the scope
	 of this comment, e-mail me for details... */
      fprintf(stderr, "sim: *WARNING*: swapping words to match host...\n");
      fprintf(stderr, "sim: *WARNING*: swapping may break your program!\n");
#else
      fatal("binary endian does not match host endian");
#endif
    }

  /* write [argc] to stack */
  temp = MD_SWAPW(argc);
  mem_access(mem, Write, sp, &temp, sizeof(word_t));
  sp += sizeof(word_t);

  /* skip past argv array and NULL */
  argv_addr = sp;
  sp = sp + (argc + 1) * sizeof(md_addr_t);

  /* save space for envp array and NULL */
  envp_addr = sp;
  for (i=0; envp[i]; i++)
    sp += sizeof(md_addr_t);
  sp += sizeof(md_addr_t);

  /* fill in the argv pointer array and data */
  for (i=0; i<argc; i++)
    {
      /* write the argv pointer array entry */
      temp = MD_SWAPW(sp);
      mem_access(mem, Write, argv_addr + i*sizeof(md_addr_t),
		 &temp, sizeof(md_addr_t));
      /* and the data */
      mem_strcpy(mem_access, mem, Write, sp, argv[i]);
      sp += strlen(argv[i]) + 1;
    }
  /* terminate argv array with a NULL */
  mem_access(mem, Write, argv_addr + i*sizeof(md_addr_t),
	     &null_ptr, sizeof(md_addr_t));

  /* write envp pointer array and data to stack */
  for (i = 0; envp[i]; i++)
    {
      /* write the envp pointer array entry */
      temp = MD_SWAPW(sp);
      mem_access(mem, Write, envp_addr + i*sizeof(md_addr_t),
		 &temp, sizeof(md_addr_t));
      /* and the data */
      mem_strcpy(mem_access, mem, Write, sp, envp[i]);
      sp += strlen(envp[i]) + 1;
    }
  /* terminate the envp array with a NULL */
  mem_access(mem, Write, envp_addr + i*sizeof(md_addr_t),
	     &null_ptr, sizeof(md_addr_t));

  /* did we tromp off the stop of the stack? */
  if (sp > ld_stack_base)
    {
      /* we did, indicate to the user that MD_MAX_ENVIRON must be increased,
	 alternatively, you can use a smaller environment, or fewer
	 command line arguments */
      fatal("environment overflow, increase MD_MAX_ENVIRON in ss.h");
    }

  /* initialize the bottom of heap to top of data segment */
  ld_brk_point = ROUND_UP(ld_data_base + ld_data_size, MD_PAGE_SIZE);

  /* set initial minimum stack pointer value to initial stack value */
  ld_stack_min = regs->regs_R[MD_REG_SP];

  /* set up initial register state */
  regs->regs_R[MD_REG_SP] = ld_environ_base;
  regs->regs_PC = ld_prog_entry;

  debug("ld_text_base: 0x%08x  ld_text_size: 0x%08x",
	ld_text_base, ld_text_size);
  debug("ld_data_base: 0x%08x  ld_data_size: 0x%08x",
	ld_data_base, ld_data_size);
  debug("ld_stack_base: 0x%08x  ld_stack_size: 0x%08x",
	ld_stack_base, ld_stack_size);
  debug("ld_prog_entry: 0x%08x", ld_prog_entry);

  /* finally, predecode the text segment... */
  {
    md_addr_t addr;
    md_inst_t inst;
    enum md_fault_type fault;

    if (OP_MAX > 255)
      fatal("cannot perform fast decoding, too many opcodes");

    debug("sim: decoding text segment...");
    for (addr=ld_text_base;
	 addr < (ld_text_base+ld_text_size);
	 addr += sizeof(md_inst_t))
      {
	fault = mem_access(mem, Read, addr, &inst, sizeof(inst));
	if (fault != md_fault_none)
	  fatal("could not read instruction memory");
	inst.a = (inst.a & ~0xff) | (word_t)MD_OP_ENUM(MD_OPFIELD(inst));
	fault = mem_access(mem, Write, addr, &inst, sizeof(inst));
	if (fault != md_fault_none)
	  fatal("could not write instruction memory");
      }
  }
}
