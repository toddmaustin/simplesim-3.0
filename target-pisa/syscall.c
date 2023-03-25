/* syscall.c - proxy system call handler routines */

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
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "sim.h"
#include "endian.h"
#include "eio.h"
#include "syscall.h"

/* live execution only support on same-endian hosts... */
#ifndef MD_CROSS_ENDIAN

#ifdef _MSC_VER
#include <io.h>
#else /* !_MSC_VER */
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <sys/param.h>
#endif
#include <errno.h>
#include <time.h>
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#ifndef _MSC_VER
#include <sys/resource.h>
#endif
#include <signal.h>

/* #include <sys/file.h> */

#include <sys/stat.h>
#ifndef _MSC_VER
#include <sys/uio.h>
#endif
#include <setjmp.h>
#ifndef _MSC_VER
#include <sys/times.h>
#endif
#include <limits.h>
#ifndef _MSC_VER
#include <sys/ioctl.h>
#endif
#if !defined(linux) && !defined(sparc) && !defined(hpux) && !defined(__hpux) && !defined(__CYGWIN32__) && !defined(ultrix)
#ifndef _MSC_VER
#include <sys/select.h>
#endif
#endif
#ifdef linux
#include <utime.h>
#include <sgtty.h>
#endif /* linux */

#if defined(hpux) || defined(__hpux)
#include <sgtty.h>
#endif

#ifdef __svr4__
#include "utime.h"
#endif

#if defined(sparc) && defined(__unix__)
#if defined(__svr4__) || defined(__USLC__)
#include <dirent.h>
#else
#include <sys/dir.h>
#endif

/* dorks */
#undef NL0
#undef NL1
#undef CR0
#undef CR1
#undef CR2
#undef CR3
#undef TAB0
#undef TAB1
#undef TAB2
#undef XTABS
#undef BS0
#undef BS1
#undef FF0
#undef FF1
#undef ECHO
#undef NOFLSH
#undef TOSTOP
#undef FLUSHO
#undef PENDIN
#endif

#if defined(hpux) || defined(__hpux)
#undef CR0
#endif

#ifdef __FreeBSD__
#include <termios.h>
/*#include <sys/ioctl_compat.h>*/
#else /* !__FreeBSD__ */
#ifndef _MSC_VER
#include <termio.h>
#endif
#endif

#if defined(hpux) || defined(__hpux)
/* et tu, dorks! */
#undef HUPCL
#undef ECHO
#undef B50
#undef B75
#undef B110
#undef B134
#undef B150
#undef B200
#undef B300
#undef B600
#undef B1200
#undef B1800
#undef B2400
#undef B4800
#undef B9600
#undef B19200
#undef B38400
#undef NL0
#undef NL1
#undef CR0
#undef CR1
#undef CR2
#undef CR3
#undef TAB0
#undef TAB1
#undef BS0
#undef BS1
#undef FF0
#undef FF1
#undef EXTA
#undef EXTB
#undef B900
#undef B3600
#undef B7200
#undef XTABS
#include <sgtty.h>
#include <utime.h>
#endif

#ifdef _MSC_VER
#define access		_access
#define chmod		_chmod
#define chdir		_chdir
#define unlink		_unlink
#define open		_open
#define creat		_creat
#define pipe		_pipe
#define dup		_dup
#define dup2		_dup2
#define stat		_stat
#define fstat		_fstat
#define lseek		_lseek
#define read		_read
#define write		_write
#define close		_close
#define getpid		_getpid
#define utime		_utime
#include <sys/utime.h>
#endif /* _MSC_VER */

/* SimpleScalar SStrix (a derivative of Ultrix) system call codes, note these
   codes reside in register $r2 at the point a `syscall' inst is executed,
   not all of these codes are implemented, see the main switch statement in
   syscall.c for a list of implemented system calls */

#define SS_SYS_syscall		0
/* SS_SYS_exit was moved to pisa.h */
#define	SS_SYS_fork		2
#define	SS_SYS_read		3
/* SS_SYS_write was moved to pisa.h */
#define	SS_SYS_open		5
#define	SS_SYS_close		6
						/*  7 is old: wait */
#define	SS_SYS_creat		8
#define	SS_SYS_link		9
#define	SS_SYS_unlink		10
#define	SS_SYS_execv		11
#define	SS_SYS_chdir		12
						/* 13 is old: time */
#define	SS_SYS_mknod		14
#define	SS_SYS_chmod		15
#define	SS_SYS_chown		16
#define	SS_SYS_brk		17		/* 17 is old: sbreak */
						/* 18 is old: stat */
#define	SS_SYS_lseek		19
#define	SS_SYS_getpid		20
#define	SS_SYS_mount		21
#define	SS_SYS_umount		22
						/* 23 is old: setuid */
#define	SS_SYS_getuid		24
						/* 25 is old: stime */
#define	SS_SYS_ptrace		26
						/* 27 is old: alarm */
						/* 28 is old: fstat */
						/* 29 is old: pause */
						/* 30 is old: utime */
						/* 31 is old: stty */
						/* 32 is old: gtty */
#define	SS_SYS_access		33
						/* 34 is old: nice */
						/* 35 is old: ftime */
#define	SS_SYS_sync		36
#define	SS_SYS_kill		37
#define	SS_SYS_stat		38
						/* 39 is old: setpgrp */
#define	SS_SYS_lstat		40
#define	SS_SYS_dup		41
#define	SS_SYS_pipe		42
						/* 43 is old: times */
#define	SS_SYS_profil		44
						/* 45 is unused */
						/* 46 is old: setgid */
#define	SS_SYS_getgid		47
						/* 48 is old: sigsys */
						/* 49 is unused */
						/* 50 is unused */
#define	SS_SYS_acct		51
						/* 52 is old: phys */
						/* 53 is old: syslock */
#define	SS_SYS_ioctl		54
#define	SS_SYS_reboot		55
						/* 56 is old: mpxchan */
#define	SS_SYS_symlink		57
#define	SS_SYS_readlink		58
#define	SS_SYS_execve		59
#define	SS_SYS_umask		60
#define	SS_SYS_chroot		61
#define	SS_SYS_fstat		62
						/* 63 is unused */
#define	SS_SYS_getpagesize 	64
#define	SS_SYS_mremap		65
#define SS_SYS_vfork		66		/* 66 is old: vfork */
						/* 67 is old: vread */
						/* 68 is old: vwrite */
#define	SS_SYS_sbrk		69
#define	SS_SYS_sstk		70
#define	SS_SYS_mmap		71
#define SS_SYS_vadvise		72		/* 72 is old: vadvise */
#define	SS_SYS_munmap		73
#define	SS_SYS_mprotect		74
#define	SS_SYS_madvise		75
#define	SS_SYS_vhangup		76
						/* 77 is old: vlimit */
#define	SS_SYS_mincore		78
#define	SS_SYS_getgroups	79
#define	SS_SYS_setgroups	80
#define	SS_SYS_getpgrp		81
#define	SS_SYS_setpgrp		82
#define	SS_SYS_setitimer	83
#define	SS_SYS_wait3		84
#define	SS_SYS_wait		SYS_wait3
#define	SS_SYS_swapon		85
#define	SS_SYS_getitimer	86
#define	SS_SYS_gethostname	87
#define	SS_SYS_sethostname	88
#define	SS_SYS_getdtablesize	89
#define	SS_SYS_dup2		90
#define	SS_SYS_getdopt		91
#define	SS_SYS_fcntl		92
#define	SS_SYS_select		93
#define	SS_SYS_setdopt		94
#define	SS_SYS_fsync		95
#define	SS_SYS_setpriority	96
#define	SS_SYS_socket		97
#define	SS_SYS_connect		98
#define	SS_SYS_accept		99
#define	SS_SYS_getpriority	100
#define	SS_SYS_send		101
#define	SS_SYS_recv		102
#define SS_SYS_sigreturn	103		/* new sigreturn */
						/* 103 was socketaddr */
#define	SS_SYS_bind		104
#define	SS_SYS_setsockopt	105
#define	SS_SYS_listen		106
						/* 107 was vtimes */
#define	SS_SYS_sigvec		108
#define	SS_SYS_sigblock		109
#define	SS_SYS_sigsetmask	110
#define	SS_SYS_sigpause		111
#define	SS_SYS_sigstack		112
#define	SS_SYS_recvmsg		113
#define	SS_SYS_sendmsg		114
						/* 115 is old vtrace */
#define	SS_SYS_gettimeofday	116
#define	SS_SYS_getrusage	117
#define	SS_SYS_getsockopt	118
						/* 119 is old resuba */
#define	SS_SYS_readv		120
#define	SS_SYS_writev		121
#define	SS_SYS_settimeofday	122
#define	SS_SYS_fchown		123
#define	SS_SYS_fchmod		124
#define	SS_SYS_recvfrom		125
#define	SS_SYS_setreuid		126
#define	SS_SYS_setregid		127
#define	SS_SYS_rename		128
#define	SS_SYS_truncate		129
#define	SS_SYS_ftruncate	130
#define	SS_SYS_flock		131
						/* 132 is unused */
#define	SS_SYS_sendto		133
#define	SS_SYS_shutdown		134
#define	SS_SYS_socketpair	135
#define	SS_SYS_mkdir		136
#define	SS_SYS_rmdir		137
#define	SS_SYS_utimes		138
#define SS_SYS_sigcleanup  	139		/* From 4.2 longjmp */
                                                /* same as SYS_sigreturn */
#define	SS_SYS_adjtime		140
#define	SS_SYS_getpeername	141
#define	SS_SYS_gethostid	142
#define	SS_SYS_sethostid	143
#define	SS_SYS_getrlimit	144
#define	SS_SYS_setrlimit	145
#define	SS_SYS_killpg		146
						/* 147 is unused */
#define	SS_SYS_setquota		148
#define	SS_SYS_quota		149
#define	SS_SYS_getsockname	150

#define SS_SYS_sysmips     	151		/* floating point control */

/* formerly mips local system calls */

#define SS_SYS_cacheflush  	152
#define SS_SYS_cachectl    	153
#define SS_SYS_atomic_op   	155

/* nfs releated system calls */
#define SS_SYS_debug       	154

#define SS_SYS_statfs      	160
#define SS_SYS_fstatfs     	161
#define SS_SYS_unmount     	162

#define SS_SYS_quotactl    	168
/* #define SS_SYS_mount       170 */

#define SS_SYS_hdwconf     	171

/* try to keep binary compatibility with mips */

#define SS_SYS_nfs_svc		158
#define SS_SYS_nfssvc		158 /* cruft - delete when kernel fixed */
#define SS_SYS_nfs_biod		163
#define SS_SYS_async_daemon	163 /* cruft - delete when kernel fixed */
#define SS_SYS_nfs_getfh	164
#define SS_SYS_getfh		164 /* cruft - delete when kernel fixed */
#define SS_SYS_getdirentries	159
#define SS_SYS_getdomainname	165
#define SS_SYS_setdomainname	166
#define SS_SYS_exportfs		169

#define SS_SYS_msgctl		172
#define SS_SYS_msgget		173
#define SS_SYS_msgrcv		174
#define SS_SYS_msgsnd		175
#define SS_SYS_semctl		176
#define SS_SYS_semget		177
#define SS_SYS_semop		178
#define SS_SYS_uname		179
#define SS_SYS_shmsys		180
#define SS_SYS_plock		181
#define SS_SYS_lockf		182
#define SS_SYS_ustat		183
#define SS_SYS_getmnt		184
#define	SS_SYS_sigpending	187
#define	SS_SYS_setsid		188
#define	SS_SYS_waitpid		189

#define	SS_SYS_utc_gettime	233	 /* 233 -- same as OSF/1 */
#define SS_SYS_utc_adjtime	234	 /* 234 -- same as OSF/1 */
#define SS_SYS_audcntl		252
#define SS_SYS_audgen		253
#define SS_SYS_startcpu		254	 /* 254 -- Ultrix Private */
#define SS_SYS_stopcpu		255	 /* 255 -- Ultrix Private */
#define SS_SYS_getsysinfo	256	 /* 256 -- Ultrix Private */
#define SS_SYS_setsysinfo	257	 /* 257 -- Ultrix Private */

/* SStrix ioctl values */
#define SS_IOCTL_TIOCGETP	1074164744
#define SS_IOCTL_TIOCSETP	-2147060727
#define SS_IOCTL_TCGETP		1076130901
#define SS_IOCTL_TCGETA		1075082331
#define SS_IOCTL_TIOCGLTC	1074164852
#define SS_IOCTL_TIOCSLTC	-2147060619
#define SS_IOCTL_TIOCGWINSZ	1074295912
#define SS_IOCTL_TCSETAW	-2146143143
#define SS_IOCTL_TIOCGETC	1074164754
#define SS_IOCTL_TIOCSETC	-2147060719
#define SS_IOCTL_TIOCLBIC	-2147191682
#define SS_IOCTL_TIOCLBIS	-2147191681
#define SS_IOCTL_TIOCLGET	0x4004747c
#define SS_IOCTL_TIOCLSET	-2147191683

/* internal system call buffer size, used primarily for file name arguments,
   argument larger than this will be truncated */
#define MAXBUFSIZE 		1024

/* total bytes to copy from a valid pointer argument for ioctl() calls,
   syscall.c does not decode ioctl() calls to determine the size of the
   arguments that reside in memory, instead, the ioctl() proxy simply copies
   NUM_IOCTL_BYTES bytes from the pointer argument to host memory */
#define NUM_IOCTL_BYTES		128

/* target stat() buffer definition, the host stat buffer format is
   automagically mapped to/from this format in syscall.c */
struct ss_statbuf
{
  shalf_t ss_st_dev;
  shalf_t ss_pad;
  word_t ss_st_ino;
  half_t ss_st_mode;
  shalf_t ss_st_nlink;
  shalf_t ss_st_uid;
  shalf_t ss_st_gid;
  shalf_t ss_st_rdev;
  shalf_t ss_pad1;
  sword_t ss_st_size;
  sword_t ss_st_atime;
  sword_t ss_st_spare1;
  sword_t ss_st_mtime;
  sword_t ss_st_spare2;
  sword_t ss_st_ctime;
  sword_t ss_st_spare3;
  sword_t ss_st_blksize;
  sword_t ss_st_blocks;
  word_t ss_st_gennum;
  sword_t ss_st_spare4;
};

struct ss_sgttyb {
  byte_t sg_ispeed;     /* input speed */
  byte_t sg_ospeed;     /* output speed */
  byte_t sg_erase;      /* erase character */
  byte_t sg_kill;       /* kill character */
  shalf_t sg_flags;     /* mode flags */
};

struct ss_timeval
{
  sword_t ss_tv_sec;		/* seconds */
  sword_t ss_tv_usec;		/* microseconds */
};

/* target getrusage() buffer definition, the host stat buffer format is
   automagically mapped to/from this format in syscall.c */
struct ss_rusage
{
  struct ss_timeval ss_ru_utime;
  struct ss_timeval ss_ru_stime;
  sword_t ss_ru_maxrss;
  sword_t ss_ru_ixrss;
  sword_t ss_ru_idrss;
  sword_t ss_ru_isrss;
  sword_t ss_ru_minflt;
  sword_t ss_ru_majflt;
  sword_t ss_ru_nswap;
  sword_t ss_ru_inblock;
  sword_t ss_ru_oublock;
  sword_t ss_ru_msgsnd;
  sword_t ss_ru_msgrcv;
  sword_t ss_ru_nsignals;
  sword_t ss_ru_nvcsw;
  sword_t ss_ru_nivcsw;
};

struct ss_timezone
{
  sword_t ss_tz_minuteswest;	/* minutes west of Greenwich */
  sword_t ss_tz_dsttime;	/* type of dst correction */
};

struct ss_rlimit
{
  int ss_rlim_cur;		/* current (soft) limit */
  int ss_rlim_max;		/* maximum value for rlim_cur */
};

/* open(2) flags for SimpleScalar target, syscall.c automagically maps
   between these codes to/from host open(2) flags */
#define SS_O_RDONLY		0
#define SS_O_WRONLY		1
#define SS_O_RDWR		2
#define SS_O_APPEND		0x0008
#define SS_O_CREAT		0x0200
#define SS_O_TRUNC		0x0400
#define SS_O_EXCL		0x0800
#define SS_O_NONBLOCK		0x4000
#define SS_O_NOCTTY		0x8000
#define SS_O_SYNC		0x2000

/* open(2) flags translation table for SimpleScalar target */
struct {
  int ss_flag;
  int local_flag;
} ss_flag_table[] = {
  /* target flag */	/* host flag */
#ifdef _MSC_VER
  { SS_O_RDONLY,	_O_RDONLY },
  { SS_O_WRONLY,	_O_WRONLY },
  { SS_O_RDWR,		_O_RDWR },
  { SS_O_APPEND,	_O_APPEND },
  { SS_O_CREAT,		_O_CREAT },
  { SS_O_TRUNC,		_O_TRUNC },
  { SS_O_EXCL,		_O_EXCL },
#ifdef _O_NONBLOCK
  { SS_O_NONBLOCK,	_O_NONBLOCK },
#endif
#ifdef _O_NOCTTY
  { SS_O_NOCTTY,	_O_NOCTTY },
#endif
#ifdef _O_SYNC
  { SS_O_SYNC,		_O_SYNC },
#endif
#else /* !_MSC_VER */
  { SS_O_RDONLY,	O_RDONLY },
  { SS_O_WRONLY,	O_WRONLY },
  { SS_O_RDWR,		O_RDWR },
  { SS_O_APPEND,	O_APPEND },
  { SS_O_CREAT,		O_CREAT },
  { SS_O_TRUNC,		O_TRUNC },
  { SS_O_EXCL,		O_EXCL },
  { SS_O_NONBLOCK,	O_NONBLOCK },
  { SS_O_NOCTTY,	O_NOCTTY },
#ifdef O_SYNC
  { SS_O_SYNC,		O_SYNC },
#endif
#endif /* _MSC_VER */
};
#define SS_NFLAGS	(sizeof(ss_flag_table)/sizeof(ss_flag_table[0]))

#endif /* !MD_CROSS_ENDIAN */


/* syscall proxy handler, architect registers and memory are assumed to be
   precise when this function is called, register and memory are updated with
   the results of the sustem call */
void
sys_syscall(struct regs_t *regs,	/* registers to access */
	    mem_access_fn mem_fn,	/* generic memory accessor */
	    struct mem_t *mem,		/* memory space to access */
	    md_inst_t inst,		/* system call inst */
	    int traceable)		/* traceable system call? */
{
  word_t syscode = regs->regs_R[2];

  /* first, check if an EIO trace is being consumed... */
  if (traceable && sim_eio_fd != NULL)
    {
      eio_read_trace(sim_eio_fd, sim_num_insn, regs, mem_fn, mem, inst);

      /* fini... */
      return;
    }
#ifdef MD_CROSS_ENDIAN
  else if (syscode == SS_SYS_exit)
    {
      /* exit jumps to the target set in main() */
      longjmp(sim_exit_buf, /* exitcode + fudge */regs->regs_R[4]+1);
    }
  else
    fatal("cannot execute PISA system call on cross-endian host");

#else /* !MD_CROSS_ENDIAN */

  /* no, OK execute the live system call... */
  switch (syscode)
    {
    case SS_SYS_exit:
      /* exit jumps to the target set in main() */
      longjmp(sim_exit_buf, /* exitcode + fudge */regs->regs_R[4]+1);
      break;

#if 0
    case SS_SYS_fork:
      /* FIXME: this is broken... */
      regs->regs_R[2] = fork();
      if (regs->regs_R[2] != -1)
	{
	  regs->regs_R[7] = 0;
	  /* parent process */
	  if (regs->regs_R[2] != 0)
	  regs->regs_R[3] = 0;
	}
      else
	fatal("SYS_fork failed");
      break;
#endif

#if 0
    case SS_SYS_vfork:
      /* FIXME: this is broken... */
      int r31_parent = regs->regs_R[31];
      /* pid */regs->regs_R[2] = vfork();
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	fatal("vfork() in SYS_vfork failed");
      if (regs->regs_R[2] != 0)
	{
	  regs->regs_R[3] = 0;
	  regs->regs_R[7] = 0;
	  regs->regs_R[31] = r31_parent;
	}
      break;
#endif

    case SS_SYS_read:
      {
	char *buf;

	/* allocate same-sized input buffer in host memory */
	if (!(buf = (char *)calloc(/*nbytes*/regs->regs_R[6], sizeof(char))))
	  fatal("out of memory in SYS_read");

	/* read data from file */
	/*nread*/regs->regs_R[2] =
	  read(/*fd*/regs->regs_R[4], buf, /*nbytes*/regs->regs_R[6]);

	/* check for error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* copy results back into host memory */
	mem_bcopy(mem_fn, mem,
		  Write, /*buf*/regs->regs_R[5],
		  buf, /*nread*/regs->regs_R[2]);

	/* done with input buffer */
	free(buf);
      }
      break;

    case SS_SYS_write:
      {
	char *buf;

	/* allocate same-sized output buffer in host memory */
	if (!(buf = (char *)calloc(/*nbytes*/regs->regs_R[6], sizeof(char))))
	  fatal("out of memory in SYS_write");

	/* copy inputs into host memory */
	mem_bcopy(mem_fn, mem,
		  Read, /*buf*/regs->regs_R[5],
		  buf, /*nbytes*/regs->regs_R[6]);

	/* write data to file */
	if (sim_progfd && MD_OUTPUT_SYSCALL(regs))
	  {
	    /* redirect program output to file */

	    /*nwritten*/regs->regs_R[2] =
	      fwrite(buf, 1, /*nbytes*/regs->regs_R[6], sim_progfd);
	  }
	else
	  {
	    /* perform program output request */

	    /*nwritten*/regs->regs_R[2] =
	      write(/*fd*/regs->regs_R[4],
		    buf, /*nbytes*/regs->regs_R[6]);
	  }

	/* check for an error condition */
	if (regs->regs_R[2] == regs->regs_R[6])
	  /*result*/regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* done with output buffer */
	free(buf);
      }
      break;

    case SS_SYS_open:
      {
	char buf[MAXBUFSIZE];
	unsigned int i;
	int ss_flags = regs->regs_R[5], local_flags = 0;

	/* translate open(2) flags */
	for (i=0; i<SS_NFLAGS; i++)
	  {
	    if (ss_flags & ss_flag_table[i].ss_flag)
	      {
		ss_flags &= ~ss_flag_table[i].ss_flag;
		local_flags |= ss_flag_table[i].local_flag;
	      }
	  }
	/* any target flags left? */
	if (ss_flags != 0)
	  fatal("syscall: open: cannot decode flags: 0x%08x", ss_flags);

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[4], buf);

	/* open the file */
	/*fd*/regs->regs_R[2] =
	  open(buf, local_flags, /*mode*/regs->regs_R[6]);
	
	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;

    case SS_SYS_close:
      /* don't close stdin, stdout, or stderr as this messes up sim logs */
      if (/*fd*/regs->regs_R[4] == 0
	  || /*fd*/regs->regs_R[4] == 1
	  || /*fd*/regs->regs_R[4] == 2)
	{
	  regs->regs_R[7] = 0;
	  break;
	}

      /* close the file */
      regs->regs_R[2] = close(/*fd*/regs->regs_R[4]);

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
      break;

    case SS_SYS_creat:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[4], buf);

	/* create the file */
	/*fd*/regs->regs_R[2] = creat(buf, /*mode*/regs->regs_R[5]);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;

    case SS_SYS_unlink:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[4], buf);

	/* delete the file */
	/*result*/regs->regs_R[2] = unlink(buf);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;

    case SS_SYS_chdir:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[4], buf);

	/* change the working directory */
	/*result*/regs->regs_R[2] = chdir(buf);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;

    case SS_SYS_chmod:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[4], buf);

	/* chmod the file */
	/*result*/regs->regs_R[2] = chmod(buf, /*mode*/regs->regs_R[5]);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;

    case SS_SYS_chown:
#ifdef _MSC_VER
      warn("syscall chown() not yet implemented for MSC...");
      regs->regs_R[7] = 0;
#else /* !_MSC_VER */
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[4], buf);

	/* chown the file */
	/*result*/regs->regs_R[2] = chown(buf, /*owner*/regs->regs_R[5],
				    /*group*/regs->regs_R[6]);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
#endif /* _MSC_VER */
      break;

    case SS_SYS_brk:
      {
	md_addr_t addr;

	/* round the new heap pointer to the its page boundary */
	addr = ROUND_UP(/*base*/regs->regs_R[4], MD_PAGE_SIZE);

	/* check whether heap area has merged with stack area */
	if (addr >= ld_brk_point && addr < (md_addr_t)regs->regs_R[29])
	  {
	    regs->regs_R[2] = 0;
	    regs->regs_R[7] = 0;
	    ld_brk_point = addr;
	  }
	else
	  {
	    /* out of address space, indicate error */
	    regs->regs_R[2] = ENOMEM;
	    regs->regs_R[7] = 1;
	  }
      }
      break;

    case SS_SYS_lseek:
      /* seek into file */
      regs->regs_R[2] =
	lseek(/*fd*/regs->regs_R[4],
	      /*off*/regs->regs_R[5], /*dir*/regs->regs_R[6]);

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
      break;

    case SS_SYS_getpid:
      /* get the simulator process id */
      /*result*/regs->regs_R[2] = getpid();

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
      break;

    case SS_SYS_getuid:
#ifdef _MSC_VER
      warn("syscall getuid() not yet implemented for MSC...");
      regs->regs_R[7] = 0;
#else /* !_MSC_VER */
      /* get current user id */
      /*first result*/regs->regs_R[2] = getuid();

      /* get effective user id */
      /*second result*/regs->regs_R[3] = geteuid();

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
#endif /* _MSC_VER */
      break;

    case SS_SYS_access:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fName*/regs->regs_R[4], buf);

	/* check access on the file */
	/*result*/regs->regs_R[2] = access(buf, /*mode*/regs->regs_R[5]);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;

    case SS_SYS_stat:
    case SS_SYS_lstat:
      {
	char buf[MAXBUFSIZE];
	struct ss_statbuf ss_sbuf;
#ifdef _MSC_VER
	struct _stat sbuf;
#else /* !_MSC_VER */
	struct stat sbuf;
#endif /* _MSC_VER */

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fName*/regs->regs_R[4], buf);

	/* stat() the file */
	if (syscode == SS_SYS_stat)
	  /*result*/regs->regs_R[2] = stat(buf, &sbuf);
	else /* syscode == SS_SYS_lstat */
	  {
#ifdef _MSC_VER
	    warn("syscall lstat() not yet implemented for MSC...");
	    regs->regs_R[7] = 0;
	    break;
#else /* !_MSC_VER */
	    /*result*/regs->regs_R[2] = lstat(buf, &sbuf);
#endif /* _MSC_VER */
	  }

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* translate from host stat structure to target format */
	ss_sbuf.ss_st_dev = MD_SWAPH(sbuf.st_dev);
	ss_sbuf.ss_pad = 0;
	ss_sbuf.ss_st_ino = MD_SWAPW(sbuf.st_ino);
	ss_sbuf.ss_st_mode = MD_SWAPH(sbuf.st_mode);
	ss_sbuf.ss_st_nlink = MD_SWAPH(sbuf.st_nlink);
	ss_sbuf.ss_st_uid = MD_SWAPH(sbuf.st_uid);
	ss_sbuf.ss_st_gid = MD_SWAPH(sbuf.st_gid);
	ss_sbuf.ss_st_rdev = MD_SWAPH(sbuf.st_rdev);
	ss_sbuf.ss_pad1 = 0;
	ss_sbuf.ss_st_size = MD_SWAPW(sbuf.st_size);
	ss_sbuf.ss_st_atime = MD_SWAPW(sbuf.st_atime);
	ss_sbuf.ss_st_spare1 = 0;
	ss_sbuf.ss_st_mtime = MD_SWAPW(sbuf.st_mtime);
	ss_sbuf.ss_st_spare2 = 0;
	ss_sbuf.ss_st_ctime = MD_SWAPW(sbuf.st_ctime);
	ss_sbuf.ss_st_spare3 = 0;
#ifndef _MSC_VER
	ss_sbuf.ss_st_blksize = MD_SWAPW(sbuf.st_blksize);
	ss_sbuf.ss_st_blocks = MD_SWAPW(sbuf.st_blocks);
#endif /* !_MSC_VER */
	ss_sbuf.ss_st_gennum = 0;
	ss_sbuf.ss_st_spare4 = 0;

	/* copy stat() results to simulator memory */
	mem_bcopy(mem_fn, mem, Write, /*sbuf*/regs->regs_R[5],
		  &ss_sbuf, sizeof(struct ss_statbuf));
      }
      break;

    case SS_SYS_dup:
      /* dup() the file descriptor */
      /*fd*/regs->regs_R[2] = dup(/*fd*/regs->regs_R[4]);

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
      break;

#ifndef _MSC_VER
    case SS_SYS_pipe:
      {
	int fd[2];

	/* copy pipe descriptors to host memory */;
	mem_bcopy(mem_fn, mem, Read, /*fd's*/regs->regs_R[4], fd, sizeof(fd));

	/* create a pipe */
	/*result*/regs->regs_R[7] = pipe(fd);

	/* copy descriptor results to result registers */
	/*pipe1*/regs->regs_R[2] = fd[0];
	/*pipe 2*/regs->regs_R[3] = fd[1];

	/* check for an error condition */
	if (regs->regs_R[7] == -1)
	  {
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;
#endif

    case SS_SYS_getgid:
#ifdef _MSC_VER
      warn("syscall getgid() not yet implemented for MSC...");
      regs->regs_R[7] = 0;
#else /* !_MSC_VER */
      /* get current group id */
      /*first result*/regs->regs_R[2] = getgid();

      /* get current effective group id */
      /*second result*/regs->regs_R[3] = getegid();

	/* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
#endif /* _MSC_VER */
      break;

    case SS_SYS_ioctl:
      {
	char buf[NUM_IOCTL_BYTES];
	int local_req = 0;

	/* convert target ioctl() request to host ioctl() request values */
	switch (/*req*/regs->regs_R[5]) {
#ifdef TIOCGETP
	case SS_IOCTL_TIOCGETP:
	  local_req = TIOCGETP;
	  break;
#endif
#ifdef TIOCSETP
	case SS_IOCTL_TIOCSETP:
	  local_req = TIOCSETP;
	  break;
#endif
#ifdef TIOCGETP
	case SS_IOCTL_TCGETP:
	  local_req = TIOCGETP;
	  break;
#endif
#ifdef TCGETA
	case SS_IOCTL_TCGETA:
	  local_req = TCGETA;
	  break;
#endif
#ifdef TIOCGLTC
	case SS_IOCTL_TIOCGLTC:
	  local_req = TIOCGLTC;
	  break;
#endif
#ifdef TIOCSLTC
	case SS_IOCTL_TIOCSLTC:
	  local_req = TIOCSLTC;
	  break;
#endif
#ifdef TIOCGWINSZ
	case SS_IOCTL_TIOCGWINSZ:
	  local_req = TIOCGWINSZ;
	  break;
#endif
#ifdef TCSETAW
	case SS_IOCTL_TCSETAW:
	  local_req = TCSETAW;
	  break;
#endif
#ifdef TIOCGETC
	case SS_IOCTL_TIOCGETC:
	  local_req = TIOCGETC;
	  break;
#endif
#ifdef TIOCSETC
	case SS_IOCTL_TIOCSETC:
	  local_req = TIOCSETC;
	  break;
#endif
#ifdef TIOCLBIC
	case SS_IOCTL_TIOCLBIC:
	  local_req = TIOCLBIC;
	  break;
#endif
#ifdef TIOCLBIS
	case SS_IOCTL_TIOCLBIS:
	  local_req = TIOCLBIS;
	  break;
#endif
#ifdef TIOCLGET
	case SS_IOCTL_TIOCLGET:
	  local_req = TIOCLGET;
	  break;
#endif
#ifdef TIOCLSET
	case SS_IOCTL_TIOCLSET:
	  local_req = TIOCLSET;
	  break;
#endif
	}

#if !defined(TIOCGETP) && (defined(linux) || defined(__CYGWIN32__))
        if (!local_req && /*req*/regs->regs_R[5] == SS_IOCTL_TIOCGETP)
          {
            struct termios lbuf;
            struct ss_sgttyb buf;

            /* result */regs->regs_R[2] =
                          tcgetattr(/* fd */(int)regs->regs_R[4], &lbuf);

            /* translate results */
            buf.sg_ispeed = lbuf.c_ispeed;
            buf.sg_ospeed = lbuf.c_ospeed;
            buf.sg_erase = lbuf.c_cc[VERASE];
            buf.sg_kill = lbuf.c_cc[VKILL];
            buf.sg_flags = 0;   /* FIXME: this is wrong... */

            mem_bcopy(mem_fn, mem, Write,
                      /* buf */regs->regs_R[6], &buf,
                      sizeof(struct ss_sgttyb));

            if (regs->regs_R[2] != -1)
              regs->regs_R[7] = 0;
            else /* probably not a typewriter, return details */
              {
                regs->regs_R[2] = errno;
                regs->regs_R[7] = 1;
              }
          }
        else
#endif

	if (!local_req)
	  {
	    /* FIXME: could not translate the ioctl() request, just warn user
	       and ignore the request */
	    warn("syscall: ioctl: ioctl code not supported d=%d, req=%d",
		 regs->regs_R[4], regs->regs_R[5]);
	    regs->regs_R[2] = 0;
	    regs->regs_R[7] = 0;
	  }
	else
	  {
#ifdef _MSC_VER
	    warn("syscall getgid() not yet implemented for MSC...");
	    regs->regs_R[7] = 0;
	    break;
#else /* !_MSC_VER */

#if 0 /* FIXME: needed? */
#ifdef TIOCGETP
	    if (local_req == TIOCGETP && sim_progfd)
	      {
		/* program I/O has been redirected to file, make
		   termios() calls fail... */

		/* got an error, return details */
		regs->regs_R[2] = ENOTTY;
		regs->regs_R[7] = 1;
		break;
	      }
#endif
#endif
	    /* ioctl() code was successfully translated to a host code */

	    /* if arg ptr exists, copy NUM_IOCTL_BYTES bytes to host mem */
	    if (/*argp*/regs->regs_R[6] != 0)
	      mem_bcopy(mem_fn, mem,
			Read, /*argp*/regs->regs_R[6], buf, NUM_IOCTL_BYTES);

	    /* perform the ioctl() call */
	    /*result*/regs->regs_R[2] =
	      ioctl(/*fd*/regs->regs_R[4], local_req, buf);

	    /* if arg ptr exists, copy NUM_IOCTL_BYTES bytes from host mem */
	    if (/*argp*/regs->regs_R[6] != 0)
	      mem_bcopy(mem_fn, mem, Write, regs->regs_R[6],
			buf, NUM_IOCTL_BYTES);

	    /* check for an error condition */
	    if (regs->regs_R[2] != -1)
	      regs->regs_R[7] = 0;
	    else
	      {	
		/* got an error, return details */
		regs->regs_R[2] = errno;
		regs->regs_R[7] = 1;
	      }
#endif /* _MSC_VER */
	  }
      }
      break;

    case SS_SYS_fstat:
      {
	struct ss_statbuf ss_sbuf;
#ifdef _MSC_VER
	struct _stat sbuf;
#else /* !_MSC_VER */
	struct stat sbuf;
#endif /* _MSC_VER */

	/* fstat() the file */
	/*result*/regs->regs_R[2] = fstat(/*fd*/regs->regs_R[4], &sbuf);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* translate the stat structure to host format */
	ss_sbuf.ss_st_dev = MD_SWAPH(sbuf.st_dev);
	ss_sbuf.ss_pad = 0;
	ss_sbuf.ss_st_ino = MD_SWAPW(sbuf.st_ino);
	ss_sbuf.ss_st_mode = MD_SWAPH(sbuf.st_mode);
	ss_sbuf.ss_st_nlink = MD_SWAPH(sbuf.st_nlink);
	ss_sbuf.ss_st_uid = MD_SWAPH(sbuf.st_uid);
	ss_sbuf.ss_st_gid = MD_SWAPH(sbuf.st_gid);
	ss_sbuf.ss_st_rdev = MD_SWAPH(sbuf.st_rdev);
	ss_sbuf.ss_pad1 = 0;
	ss_sbuf.ss_st_size = MD_SWAPW(sbuf.st_size);
	ss_sbuf.ss_st_atime = MD_SWAPW(sbuf.st_atime);
        ss_sbuf.ss_st_spare1 = 0;
	ss_sbuf.ss_st_mtime = MD_SWAPW(sbuf.st_mtime);
        ss_sbuf.ss_st_spare2 = 0;
	ss_sbuf.ss_st_ctime = MD_SWAPW(sbuf.st_ctime);
        ss_sbuf.ss_st_spare3 = 0;
#ifndef _MSC_VER
	ss_sbuf.ss_st_blksize = MD_SWAPW(sbuf.st_blksize);
	ss_sbuf.ss_st_blocks = MD_SWAPW(sbuf.st_blocks);
#endif /* !_MSC_VER */
        ss_sbuf.ss_st_gennum = 0;
        ss_sbuf.ss_st_spare4 = 0;

	/* copy fstat() results to simulator memory */
	mem_bcopy(mem_fn, mem, Write, /*sbuf*/regs->regs_R[5],
		  &ss_sbuf, sizeof(struct ss_statbuf));
      }
      break;

    case SS_SYS_getpagesize:
      /* get target pagesize */
      regs->regs_R[2] = /* was: getpagesize() */MD_PAGE_SIZE;

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
      break;

    case SS_SYS_setitimer:
      /* FIXME: the sigvec system call is ignored */
      regs->regs_R[2] = regs->regs_R[7] = 0;
      warn("syscall: setitimer ignored");
      break;

    case SS_SYS_getdtablesize:
#if defined(_AIX)
      /* get descriptor table size */
      regs->regs_R[2] = getdtablesize();

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
#elif defined(__CYGWIN32__) || defined(ultrix) || defined(_MSC_VER)
      {
	/* no comparable system call found, try some reasonable defaults */
	warn("syscall: called getdtablesize()\n");
	regs->regs_R[2] = 16;
	regs->regs_R[7] = 0;
      }
#else
      {
	struct rlimit rl;

	/* get descriptor table size in rlimit structure */
	if (getrlimit(RLIMIT_NOFILE, &rl) != -1)
	  {
	    regs->regs_R[2] = rl.rlim_cur;
	    regs->regs_R[7] = 0;
	  }
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
#endif
      break;

    case SS_SYS_dup2:
      /* dup2() the file descriptor */
      regs->regs_R[2] =
	dup2(/* fd1 */regs->regs_R[4], /* fd2 */regs->regs_R[5]);

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
      break;

    case SS_SYS_fcntl:
#ifdef _MSC_VER
      warn("syscall fcntl() not yet implemented for MSC...");
      regs->regs_R[7] = 0;
#else /* !_MSC_VER */
      /* get fcntl() information on the file */
      regs->regs_R[2] =
	fcntl(/*fd*/regs->regs_R[4], /*cmd*/regs->regs_R[5],
	      /*arg*/regs->regs_R[6]);

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
#endif /* _MSC_VER */
      break;

    case SS_SYS_select:
#ifdef _MSC_VER
      warn("syscall select() not yet implemented for MSC...");
      regs->regs_R[7] = 0;
#else /* !_MSC_VER */
      {
	fd_set readfd, writefd, exceptfd;
	fd_set *readfdp, *writefdp, *exceptfdp;
	struct timeval timeout, *timeoutp;
	word_t param5;

	/* FIXME: swap words? */

	/* read the 5th parameter (timeout) from the stack */
	mem_bcopy(mem_fn, mem,
		  Read, regs->regs_R[29]+16, &param5, sizeof(word_t));

	/* copy read file descriptor set into host memory */
	if (/*readfd*/regs->regs_R[5] != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read, /*readfd*/regs->regs_R[5],
		      &readfd, sizeof(fd_set));
	    readfdp = &readfd;
	  }
	else
	  readfdp = NULL;

	/* copy write file descriptor set into host memory */
	if (/*writefd*/regs->regs_R[6] != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read, /*writefd*/regs->regs_R[6],
		      &writefd, sizeof(fd_set));
	    writefdp = &writefd;
	  }
	else
	  writefdp = NULL;

	/* copy exception file descriptor set into host memory */
	if (/*exceptfd*/regs->regs_R[7] != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read, /*exceptfd*/regs->regs_R[7],
		      &exceptfd, sizeof(fd_set));
	    exceptfdp = &exceptfd;
	  }
	else
	  exceptfdp = NULL;

	/* copy timeout value into host memory */
	if (/*timeout*/param5 != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read, /*timeout*/param5,
		      &timeout, sizeof(struct timeval));
	    timeoutp = &timeout;
	  }
	else
	  timeoutp = NULL;

#if defined(hpux) || defined(__hpux)
	/* select() on the specified file descriptors */
	/*result*/regs->regs_R[2] =
	  select(/*nfd*/regs->regs_R[4],
		 (int *)readfdp, (int *)writefdp, (int *)exceptfdp, timeoutp);
#else
	/* select() on the specified file descriptors */
	/*result*/regs->regs_R[2] =
	  select(/*nfd*/regs->regs_R[4],
		 readfdp, writefdp, exceptfdp, timeoutp);
#endif

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* copy read file descriptor set to target memory */
	if (/*readfd*/regs->regs_R[5] != 0)
	  mem_bcopy(mem_fn, mem, Write, /*readfd*/regs->regs_R[5],
		    &readfd, sizeof(fd_set));

	/* copy write file descriptor set to target memory */
	if (/*writefd*/regs->regs_R[6] != 0)
	  mem_bcopy(mem_fn, mem, Write, /*writefd*/regs->regs_R[6],
		    &writefd, sizeof(fd_set));

	/* copy exception file descriptor set to target memory */
	if (/*exceptfd*/regs->regs_R[7] != 0)
	  mem_bcopy(mem_fn, mem, Write, /*exceptfd*/regs->regs_R[7],
		    &exceptfd, sizeof(fd_set));

	/* copy timeout value result to target memory */
	if (/* timeout */param5 != 0)
	  mem_bcopy(mem_fn, mem, Write, /*timeout*/param5,
		    &timeout, sizeof(struct timeval));
      }
#endif
      break;

    case SS_SYS_sigvec:
      /* FIXME: the sigvec system call is ignored */
      regs->regs_R[2] = regs->regs_R[7] = 0;
      warn("syscall: sigvec ignored");
      break;

    case SS_SYS_sigblock:
      /* FIXME: the sigblock system call is ignored */
      regs->regs_R[2] = regs->regs_R[7] = 0;
      warn("syscall: sigblock ignored");
      break;

    case SS_SYS_sigsetmask:
      /* FIXME: the sigsetmask system call is ignored */
      regs->regs_R[2] = regs->regs_R[7] = 0;
      warn("syscall: sigsetmask ignored");
      break;

#if 0
    case SS_SYS_sigstack:
      /* FIXME: this is broken... */
      /* do not make the system call; instead, modify (the stack
	 portion of) the simulator's main memory, ignore the 1st
	 argument (regs->regs_R[4]), as it relates to signal handling */
      if (regs->regs_R[5] != 0)
	{
	  (*maf)(Read, regs->regs_R[29]+28, (unsigned char *)&temp, 4);
	  (*maf)(Write, regs->regs_R[5], (unsigned char *)&temp, 4);
	}
      regs->regs_R[2] = regs->regs_R[7] = 0;
      break;
#endif

    case SS_SYS_gettimeofday:
#ifdef _MSC_VER
      warn("syscall gettimeofday() not yet implemented for MSC...");
      regs->regs_R[7] = 0;
#else /* _MSC_VER */
      {
	struct ss_timeval ss_tv;
	struct timeval tv, *tvp;
	struct ss_timezone ss_tz;
	struct timezone tz, *tzp;

	if (/*timeval*/regs->regs_R[4] != 0)
	  {
	    /* copy timeval into host memory */
	    mem_bcopy(mem_fn, mem, Read, /*timeval*/regs->regs_R[4],
		      &ss_tv, sizeof(struct ss_timeval));

	    /* convert target timeval structure to host format */
	    tv.tv_sec = MD_SWAPW(ss_tv.ss_tv_sec);
	    tv.tv_usec = MD_SWAPW(ss_tv.ss_tv_usec);
	    tvp = &tv;
	  }
	else
	  tvp = NULL;

	if (/*timezone*/regs->regs_R[5] != 0)
	  {
	    /* copy timezone into host memory */
	    mem_bcopy(mem_fn, mem, Read, /*timezone*/regs->regs_R[5],
		      &ss_tz, sizeof(struct ss_timezone));

	    /* convert target timezone structure to host format */
	    tz.tz_minuteswest = MD_SWAPW(ss_tz.ss_tz_minuteswest);
	    tz.tz_dsttime = MD_SWAPW(ss_tz.ss_tz_dsttime);
	    tzp = &tz;
	  }
	else
	  tzp = NULL;

	/* get time of day */
	/*result*/regs->regs_R[2] = gettimeofday(tvp, tzp);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, indicate result */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	if (/*timeval*/regs->regs_R[4] != 0)
	  {
	    /* convert host timeval structure to target format */
	    ss_tv.ss_tv_sec = MD_SWAPW(tv.tv_sec);
	    ss_tv.ss_tv_usec = MD_SWAPW(tv.tv_usec);

	    /* copy timeval to target memory */
	    mem_bcopy(mem_fn, mem, Write, /*timeval*/regs->regs_R[4],
		      &ss_tv, sizeof(struct ss_timeval));
	  }

	if (/*timezone*/regs->regs_R[5] != 0)
	  {
	    /* convert host timezone structure to target format */
	    ss_tz.ss_tz_minuteswest = MD_SWAPW(tz.tz_minuteswest);
	    ss_tz.ss_tz_dsttime = MD_SWAPW(tz.tz_dsttime);

	    /* copy timezone to target memory */
	    mem_bcopy(mem_fn, mem, Write, /*timezone*/regs->regs_R[5],
		      &ss_tz, sizeof(struct ss_timezone));
	  }
      }
#endif /* !_MSC_VER */
      break;

    case SS_SYS_getrusage:
#if defined(__svr4__) || defined(__USLC__) || defined(hpux) || defined(__hpux) || defined(_AIX)
      {
	struct tms tms_buf;
	struct ss_rusage rusage;

	/* get user and system times */
	if (times(&tms_buf) != -1)
	  {
	    /* no error */
	    regs->regs_R[2] = 0;
	    regs->regs_R[7] = 0;
	  }
	else
	  {
	    /* got an error, indicate result */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* initialize target rusage result structure */
#if defined(__svr4__)
	memset(&rusage, '\0', sizeof(struct ss_rusage));
#else /* !defined(__svr4__) */
	bzero(&rusage, sizeof(struct ss_rusage));
#endif

	/* convert from host rusage structure to target format */
	rusage.ss_ru_utime.ss_tv_sec = tms_buf.tms_utime/CLK_TCK;
	rusage.ss_ru_utime.ss_tv_sec = MD_SWAPW(rusage.ss_ru_utime.ss_tv_sec);
	rusage.ss_ru_utime.ss_tv_usec = 0;
	rusage.ss_ru_stime.ss_tv_sec = tms_buf.tms_stime/CLK_TCK;
	rusage.ss_ru_stime.ss_tv_sec = MD_SWAPW(rusage.ss_ru_stime.ss_tv_sec);
	rusage.ss_ru_stime.ss_tv_usec = 0;

	/* copy rusage results into target memory */
	mem_bcopy(mem_fn, mem, Write, /*rusage*/regs->regs_R[5],
		  &rusage, sizeof(struct ss_rusage));
      }
#elif defined(__unix__) || defined(unix)
      {
	struct rusage local_rusage;
	struct ss_rusage rusage;

	/* get rusage information */
	/*result*/regs->regs_R[2] =
	  getrusage(/*who*/regs->regs_R[4], &local_rusage);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, indicate result */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* convert from host rusage structure to target format */
	rusage.ss_ru_utime.ss_tv_sec = local_rusage.ru_utime.tv_sec;
	rusage.ss_ru_utime.ss_tv_usec = local_rusage.ru_utime.tv_usec;
	rusage.ss_ru_utime.ss_tv_sec = MD_SWAPW(local_rusage.ru_utime.tv_sec);
	rusage.ss_ru_utime.ss_tv_usec =
	  MD_SWAPW(local_rusage.ru_utime.tv_usec);
	rusage.ss_ru_stime.ss_tv_sec = local_rusage.ru_stime.tv_sec;
	rusage.ss_ru_stime.ss_tv_usec = local_rusage.ru_stime.tv_usec;
	rusage.ss_ru_stime.ss_tv_sec =
	  MD_SWAPW(local_rusage.ru_stime.tv_sec);
	rusage.ss_ru_stime.ss_tv_usec =
	  MD_SWAPW(local_rusage.ru_stime.tv_usec);
	rusage.ss_ru_maxrss = MD_SWAPW(local_rusage.ru_maxrss);
	rusage.ss_ru_ixrss = MD_SWAPW(local_rusage.ru_ixrss);
	rusage.ss_ru_idrss = MD_SWAPW(local_rusage.ru_idrss);
	rusage.ss_ru_isrss = MD_SWAPW(local_rusage.ru_isrss);
	rusage.ss_ru_minflt = MD_SWAPW(local_rusage.ru_minflt);
	rusage.ss_ru_majflt = MD_SWAPW(local_rusage.ru_majflt);
	rusage.ss_ru_nswap = MD_SWAPW(local_rusage.ru_nswap);
	rusage.ss_ru_inblock = MD_SWAPW(local_rusage.ru_inblock);
	rusage.ss_ru_oublock = MD_SWAPW(local_rusage.ru_oublock);
	rusage.ss_ru_msgsnd = MD_SWAPW(local_rusage.ru_msgsnd);
	rusage.ss_ru_msgrcv = MD_SWAPW(local_rusage.ru_msgrcv);
	rusage.ss_ru_nsignals = MD_SWAPW(local_rusage.ru_nsignals);
	rusage.ss_ru_nvcsw = MD_SWAPW(local_rusage.ru_nvcsw);
	rusage.ss_ru_nivcsw = MD_SWAPW(local_rusage.ru_nivcsw);

	/* copy rusage results into target memory */
	mem_bcopy(mem_fn, mem, Write, /*rusage*/regs->regs_R[5],
		  &rusage, sizeof(struct ss_rusage));
      }
#elif defined(__CYGWIN32__) || defined(_MSC_VER)
	    warn("syscall: called getrusage()\n");
            regs->regs_R[7] = 0;
#else
#error No getrusage() implementation!
#endif
      break;

    case SS_SYS_writev:
#ifdef _MSC_VER
      warn("syscall writev() not yet implemented for MSC...");
      regs->regs_R[7] = 0;
#else /* !_MSC_VER */
      {
	int i;
	char *buf;
	struct iovec *iov;

	/* allocate host side I/O vectors */
	iov =
	  (struct iovec *)malloc(/*iovcnt*/regs->regs_R[6]
				 * sizeof(struct iovec));
	if (!iov)
	  fatal("out of virtual memory in SYS_writev");

	/* copy target side pointer data into host side vector */
	mem_bcopy(mem_fn, mem, Read, /*iov*/regs->regs_R[5],
		  iov, /*iovcnt*/regs->regs_R[6] * sizeof(struct iovec));

	/* copy target side I/O vector buffers to host memory */
	for (i=0; i < /*iovcnt*/regs->regs_R[6]; i++)
	  {
	    iov[i].iov_base = (char *)MD_SWAPW((unsigned long)iov[i].iov_base);
	    iov[i].iov_len = MD_SWAPW(iov[i].iov_len);
	    if (iov[i].iov_base != NULL)
	      {
		buf = (char *)calloc(iov[i].iov_len, sizeof(char));
		if (!buf)
		  fatal("out of virtual memory in SYS_writev");
		mem_bcopy(mem_fn, mem, Read, (md_addr_t)iov[i].iov_base,
			  buf, iov[i].iov_len);
		iov[i].iov_base = buf;
	      }
	  }

	/* perform the vector'ed write */
	/*result*/regs->regs_R[2] =
	  writev(/*fd*/regs->regs_R[4], iov, /*iovcnt*/regs->regs_R[6]);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, indicate results */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* free all the allocated memory */
	for (i=0; i < /*iovcnt*/regs->regs_R[6]; i++)
	  {
	    if (iov[i].iov_base)
	      {
		free(iov[i].iov_base);
		iov[i].iov_base = NULL;
	      }
	  }
	free(iov);
      }
#endif /* !_MSC_VER */
      break;

    case SS_SYS_utimes:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[4], buf);

	if (/*timeval*/regs->regs_R[5] == 0)
	  {
#if defined(hpux) || defined(__hpux) || defined(linux)
	    /* no utimes() in hpux, use utime() instead */
	    /*result*/regs->regs_R[2] = utime(buf, NULL);
#elif defined(_MSC_VER)
	    /* no utimes() in MSC, use utime() instead */
	    /*result*/regs->regs_R[2] = utime(buf, NULL);
#elif defined(__svr4__) || defined(__USLC__) || defined(unix) || defined(_AIX) || defined(__alpha)
	    /*result*/regs->regs_R[2] = utimes(buf, NULL);
#elif defined(__CYGWIN32__)
	    warn("syscall: called utimes()\n");
#else
#error No utimes() implementation!
#endif
	  }
	else
	  {
	    struct ss_timeval ss_tval[2];
#ifndef _MSC_VER
	    struct timeval tval[2];
#endif /* !_MSC_VER */

	    /* copy timeval structure to host memory */
	    mem_bcopy(mem_fn, mem, Read, /*timeout*/regs->regs_R[5],
		      ss_tval, 2*sizeof(struct ss_timeval));

#ifndef _MSC_VER
	    /* convert timeval structure to host format */
	    tval[0].tv_sec = MD_SWAPW(ss_tval[0].ss_tv_sec);
	    tval[0].tv_usec = MD_SWAPW(ss_tval[0].ss_tv_usec);
	    tval[1].tv_sec = MD_SWAPW(ss_tval[1].ss_tv_sec);
	    tval[1].tv_usec = MD_SWAPW(ss_tval[1].ss_tv_usec);
#endif /* !_MSC_VER */

#if defined(hpux) || defined(__hpux) || defined(__svr4__)
	    /* no utimes() in hpux, use utime() instead */
	    {
	      struct utimbuf ubuf;

	      ubuf.actime = tval[0].tv_sec;
	      ubuf.modtime = tval[1].tv_sec;

	      /* result */regs->regs_R[2] = utime(buf, &ubuf);
	    }
#elif defined(_MSC_VER)
	    /* no utimes() in MSC, use utime() instead */
	    {
	      struct _utimbuf ubuf;

	      ubuf.actime = ss_tval[0].ss_tv_sec;
	      ubuf.modtime = ss_tval[1].ss_tv_sec;

	      /* result */regs->regs_R[2] = utime(buf, &ubuf);
	    }
#elif defined(__USLC__) || defined(unix) || defined(_AIX) || defined(__alpha)
	    /* result */regs->regs_R[2] = utimes(buf, tval);
#elif defined(__CYGWIN32__)
	    warn("syscall: called utimes()\n");
#else
#error No utimes() implementation!
#endif
	  }

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, indicate results */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;

    case SS_SYS_getrlimit:
    case SS_SYS_setrlimit:
#ifdef _MSC_VER
      warn("syscall get/setrlimit() not yet implemented for MSC...");
      regs->regs_R[7] = 0;
#elif defined(__CYGWIN32__)
      warn("syscall: called get/setrlimit()\n");
      regs->regs_R[7] = 0;
#else
      {
	/* FIXME: check this..., was: struct rlimit ss_rl; */
	struct ss_rlimit ss_rl;
	struct rlimit rl;

	/* copy rlimit structure to host memory */
	mem_bcopy(mem_fn, mem, Read, /*rlimit*/regs->regs_R[5],
		  &ss_rl, sizeof(struct ss_rlimit));

	/* convert rlimit structure to host format */
	rl.rlim_cur = MD_SWAPW(ss_rl.ss_rlim_cur);
	rl.rlim_max = MD_SWAPW(ss_rl.ss_rlim_max);

	/* get rlimit information */
	if (syscode == SS_SYS_getrlimit)
	  /*result*/regs->regs_R[2] = getrlimit(regs->regs_R[4], &rl);
	else /* syscode == SS_SYS_setrlimit */
	  /*result*/regs->regs_R[2] = setrlimit(regs->regs_R[4], &rl);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, indicate results */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* convert rlimit structure to target format */
	ss_rl.ss_rlim_cur = MD_SWAPW(rl.rlim_cur);
	ss_rl.ss_rlim_max = MD_SWAPW(rl.rlim_max);

	/* copy rlimit structure to target memory */
	mem_bcopy(mem_fn, mem, Write, /*rlimit*/regs->regs_R[5],
		  &ss_rl, sizeof(struct ss_rlimit));
      }
#endif
      break;

#if 0
    case SS_SYS_getdirentries:
      /* FIXME: this is currently broken due to incompatabilities in
	 disk directory formats */
      {
	unsigned int i;
	char *buf;
	int base;

	buf = (char *)calloc(/* nbytes */regs->regs_R[6] + 1, sizeof(char));
	if (!buf)
	  fatal("out of memory in SYS_getdirentries");

	/* copy in */
	for (i=0; i</* nbytes */regs->regs_R[6]; i++)
	  (*maf)(Read, /* buf */regs->regs_R[5]+i,
		 (unsigned char *)&buf[i], 1);
	(*maf)(Read, /* basep */regs->regs_R[7], (unsigned char *)&base, 4);

	/*cc*/regs->regs_R[2] =
	  getdirentries(/*fd*/regs->regs_R[4], buf,
			/*nbytes*/regs->regs_R[6], &base);

	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* copy out */
	for (i=0; i</* nbytes */regs->regs_R[6]; i++)
	  (*maf)(Write, /* buf */regs->regs_R[5]+i,
		 (unsigned char *)&buf[i], 1);
	(*maf)(Write, /* basep */regs->regs_R[7], (unsigned char *)&base, 4);

	free(buf);
      }
      break;
#endif

    default:
      panic("invalid/unimplemented system call encountered, code %d", syscode);
    }

#endif /* MD_CROSS_ENDIAN */

}
