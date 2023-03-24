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

/* only enable a minimal set of systen call proxies if on limited
   hosts or if in cross endian live execution mode */
#ifndef MIN_SYSCALL_MODE
#if defined(_MSC_VER) || defined(__CYGWIN32__) || defined(MD_CROSS_ENDIAN)
#define MIN_SYSCALL_MODE
#endif
#endif /* !MIN_SYSCALL_MODE */

/* live execution only support on same-endian hosts... */
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
#ifndef _MSC_VER
#include <sys/file.h>
#endif
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
#if defined(__FreeBSD__)
#include <utime.h>
#include <dirent.h>
#endif
#if defined(linux)
#include <utime.h>
#include <dirent.h>
#include <sys/vfs.h>
#endif
#if defined(_AIX)
#include <sys/statfs.h>
#else /* !_AIX */
#ifndef _MSC_VER
#include <sys/mount.h>
#endif
#endif /* !_AIX */
#if !defined(linux) && !defined(sparc) && !defined(hpux) && !defined(__hpux) && !defined(__CYGWIN32__) && !defined(ultrix)
#ifndef _MSC_VER
#include <sys/select.h>
#endif
#endif
#ifdef linux
#include <sgtty.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#endif /* linux */

#ifdef __FreeBSD__
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#endif /* __FreeBSD__ */

#if defined(__svr4__)
#include <sys/dirent.h>
#include <sys/filio.h>
#elif defined(__osf__)
#include <dirent.h>
/* -- For some weird reason, getdirentries() is not declared in any
 * -- header file under /usr/include on the Alpha boxen that I tried
 * -- SS-Alpha on. But the function exists in the libraries.
 */
int getdirentries(int fd, char *buf, int nbytes, long *basep);
#endif

#if defined(__svr4__) || defined(__osf__)
#include <sys/statvfs.h>
#define statfs statvfs
#include <sys/time.h>
#include <utime.h>
#include <sgtty.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
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
#else
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

#include <sys/socket.h>
#include <sys/poll.h>

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

#define OSF_SYS_syscall     0
/* OSF_SYS_exit moved to alpha.h */
#define OSF_SYS_fork        2
#define OSF_SYS_read        3
/* OSF_SYS_write moved to alpha.h */
#define OSF_SYS_old_open    5       /* 5 is old open */
#define OSF_SYS_close       6
#define OSF_SYS_wait4       7
#define OSF_SYS_old_creat   8       /* 8 is old creat */
#define OSF_SYS_link        9
#define OSF_SYS_unlink      10
#define OSF_SYS_execv       11
#define OSF_SYS_chdir       12
#define OSF_SYS_fchdir      13
#define OSF_SYS_mknod       14
#define OSF_SYS_chmod       15
#define OSF_SYS_chown       16
#define OSF_SYS_obreak      17
#define OSF_SYS_getfsstat   18
#define OSF_SYS_lseek       19
#define OSF_SYS_getpid      20
#define OSF_SYS_mount       21
#define OSF_SYS_unmount     22
#define OSF_SYS_setuid      23
#define OSF_SYS_getuid      24
#define OSF_SYS_exec_with_loader    25
#define OSF_SYS_ptrace      26
#ifdef  COMPAT_43
#define OSF_SYS_nrecvmsg    27
#define OSF_SYS_nsendmsg    28
#define OSF_SYS_nrecvfrom   29
#define OSF_SYS_naccept     30
#define OSF_SYS_ngetpeername        31
#define OSF_SYS_ngetsockname        32
#else
#define OSF_SYS_recvmsg     27
#define OSF_SYS_sendmsg     28
#define OSF_SYS_recvfrom    29
#define OSF_SYS_accept      30
#define OSF_SYS_getpeername 31
#define OSF_SYS_getsockname 32
#endif
#define OSF_SYS_access      33
#define OSF_SYS_chflags     34
#define OSF_SYS_fchflags    35
#define OSF_SYS_sync        36
#define OSF_SYS_kill        37
#define OSF_SYS_old_stat    38      /* 38 is old stat */
#define OSF_SYS_setpgid     39
#define OSF_SYS_old_lstat   40      /* 40 is old lstat */
#define OSF_SYS_dup 41
#define OSF_SYS_pipe        42
#define OSF_SYS_set_program_attributes      43
#define OSF_SYS_profil      44
#define OSF_SYS_open        45
                                /* 46 is obsolete osigaction */
#define OSF_SYS_getgid      47
#define OSF_SYS_sigprocmask 48
#define OSF_SYS_getlogin    49
#define OSF_SYS_setlogin    50
#define OSF_SYS_acct        51
#define OSF_SYS_sigpending  52
#define OSF_SYS_ioctl       54
#define OSF_SYS_reboot      55
#define OSF_SYS_revoke      56
#define OSF_SYS_symlink     57
#define OSF_SYS_readlink    58
#define OSF_SYS_execve      59
#define OSF_SYS_umask       60
#define OSF_SYS_chroot      61
#define OSF_SYS_old_fstat   62      /* 62 is old fstat */
#define OSF_SYS_getpgrp     63
#define OSF_SYS_getpagesize 64
#define OSF_SYS_mremap      65
#define OSF_SYS_vfork       66
#define OSF_SYS_stat        67
#define OSF_SYS_lstat       68
#define OSF_SYS_sbrk        69
#define OSF_SYS_sstk        70
#define OSF_SYS_mmap        71
#define OSF_SYS_ovadvise    72
#define OSF_SYS_munmap      73
#define OSF_SYS_mprotect    74
#define OSF_SYS_madvise     75
#define OSF_SYS_old_vhangup 76      /* 76 is old vhangup */
#define OSF_SYS_kmodcall    77
#define OSF_SYS_mincore     78
#define OSF_SYS_getgroups   79
#define OSF_SYS_setgroups   80
#define OSF_SYS_old_getpgrp 81      /* 81 is old getpgrp */
#define OSF_SYS_setpgrp     82
#define OSF_SYS_setitimer   83
#define OSF_SYS_old_wait    84      /* 84 is old wait */
#define OSF_SYS_table       85
#define OSF_SYS_getitimer   86
#define OSF_SYS_gethostname 87
#define OSF_SYS_sethostname 88
#define OSF_SYS_getdtablesize       89
#define OSF_SYS_dup2        90
#define OSF_SYS_fstat       91
#define OSF_SYS_fcntl       92
#define OSF_SYS_select      93
#define OSF_SYS_poll        94
#define OSF_SYS_fsync       95
#define OSF_SYS_setpriority 96
#define OSF_SYS_socket      97
#define OSF_SYS_connect     98
#ifdef  COMPAT_43
#define OSF_SYS_accept      99
#else
#define OSF_SYS_old_accept  99      /* 99 is old accept */
#endif
#define OSF_SYS_getpriority 100
#ifdef  COMPAT_43
#define OSF_SYS_send        101
#define OSF_SYS_recv        102
#else
#define OSF_SYS_old_send    101     /* 101 is old send */
#define OSF_SYS_old_recv    102     /* 102 is old recv */
#endif
#define OSF_SYS_sigreturn   103
#define OSF_SYS_bind        104
#define OSF_SYS_setsockopt  105
#define OSF_SYS_listen      106
#define OSF_SYS_plock       107
#define OSF_SYS_old_sigvec  108     /* 108 is old sigvec */
#define OSF_SYS_old_sigblock        109     /* 109 is old sigblock */
#define OSF_SYS_old_sigsetmask      110     /* 110 is old sigsetmask */
#define OSF_SYS_sigsuspend  111
#define OSF_SYS_sigstack    112
#ifdef  COMPAT_43
#define OSF_SYS_recvmsg     113
#define OSF_SYS_sendmsg     114
#else
#define OSF_SYS_old_recvmsg 113     /* 113 is old recvmsg */
#define OSF_SYS_old_sendmsg 114     /* 114 is old sendmsg */
#endif
                                /* 115 is obsolete vtrace */
#define OSF_SYS_gettimeofday        116
#define OSF_SYS_getrusage   117
#define OSF_SYS_getsockopt  118
#define OSF_SYS_readv       120
#define OSF_SYS_writev      121
#define OSF_SYS_settimeofday        122
#define OSF_SYS_fchown      123
#define OSF_SYS_fchmod      124
#ifdef  COMPAT_43
#define OSF_SYS_recvfrom    125
#else
#define OSF_SYS_old_recvfrom        125     /* 125 is old recvfrom */
#endif
#define OSF_SYS_setreuid    126
#define OSF_SYS_setregid    127
#define OSF_SYS_rename      128
#define OSF_SYS_truncate    129
#define OSF_SYS_ftruncate   130
#define OSF_SYS_flock       131
#define OSF_SYS_setgid      132
#define OSF_SYS_sendto      133
#define OSF_SYS_shutdown    134
#define OSF_SYS_socketpair  135
#define OSF_SYS_mkdir       136
#define OSF_SYS_rmdir       137
#define OSF_SYS_utimes      138
                                /* 139 is obsolete 4.2 sigreturn */
#define OSF_SYS_adjtime     140
#ifdef  COMPAT_43
#define OSF_SYS_getpeername 141
#else
#define OSF_SYS_old_getpeername     141     /* 141 is old getpeername */
#endif
#define OSF_SYS_gethostid   142
#define OSF_SYS_sethostid   143
#define OSF_SYS_getrlimit   144
#define OSF_SYS_setrlimit   145
#define OSF_SYS_old_killpg  146     /* 146 is old killpg */
#define OSF_SYS_setsid      147
#define OSF_SYS_quotactl    148
#define OSF_SYS_oldquota    149
#ifdef  COMPAT_43
#define OSF_SYS_getsockname 150
#else
#define OSF_SYS_old_getsockname     150     /* 150 is old getsockname */
#endif
#define OSF_SYS_pid_block   153
#define OSF_SYS_pid_unblock 154
#define OSF_SYS_sigaction   156
#define OSF_SYS_sigwaitprim 157
#define OSF_SYS_nfssvc      158
#define OSF_SYS_getdirentries       159
#define OSF_SYS_statfs      160
#define OSF_SYS_fstatfs     161
#define OSF_SYS_async_daemon        163
#define OSF_SYS_getfh       164
#define OSF_SYS_getdomainname       165
#define OSF_SYS_setdomainname       166
#define OSF_SYS_exportfs    169
#define OSF_SYS_alt_plock   181     /* 181 is alternate plock */
#define OSF_SYS_getmnt      184
#define OSF_SYS_alt_sigpending      187     /* 187 is alternate sigpending */
#define OSF_SYS_alt_setsid  188     /* 188 is alternate setsid */
#define OSF_SYS_swapon      199
#define OSF_SYS_msgctl      200
#define OSF_SYS_msgget      201
#define OSF_SYS_msgrcv      202
#define OSF_SYS_msgsnd      203
#define OSF_SYS_semctl      204
#define OSF_SYS_semget      205
#define OSF_SYS_semop       206
#define OSF_SYS_uname       207
#define OSF_SYS_lchown      208
#define OSF_SYS_shmat       209
#define OSF_SYS_shmctl      210
#define OSF_SYS_shmdt       211
#define OSF_SYS_shmget      212
#define OSF_SYS_mvalid      213
#define OSF_SYS_getaddressconf      214
#define OSF_SYS_msleep      215
#define OSF_SYS_mwakeup     216
#define OSF_SYS_msync       217
#define OSF_SYS_signal      218
#define OSF_SYS_utc_gettime 219
#define OSF_SYS_utc_adjtime 220
#define OSF_SYS_security    222
#define OSF_SYS_kloadcall   223
#define OSF_SYS_getpgid     233
#define OSF_SYS_getsid      234
#define OSF_SYS_sigaltstack 235
#define OSF_SYS_waitid      236
#define OSF_SYS_priocntlset 237
#define OSF_SYS_sigsendset  238
#define OSF_SYS_set_speculative     239
#define OSF_SYS_msfs_syscall        240
#define OSF_SYS_sysinfo     241
#define OSF_SYS_uadmin      242
#define OSF_SYS_fuser       243
#define OSF_SYS_proplist_syscall    244
#define OSF_SYS_ntp_adjtime 245
#define OSF_SYS_ntp_gettime 246
#define OSF_SYS_pathconf    247
#define OSF_SYS_fpathconf   248
#define OSF_SYS_uswitch     250
#define OSF_SYS_usleep_thread       251
#define OSF_SYS_audcntl     252
#define OSF_SYS_audgen      253
#define OSF_SYS_sysfs       254
#define OSF_SYS_subOSF_SYS_info 255
#define OSF_SYS_getsysinfo  256
#define OSF_SYS_setsysinfo  257
#define OSF_SYS_afs_syscall 258
#define OSF_SYS_swapctl     259
#define OSF_SYS_memcntl     260
#define OSF_SYS_fdatasync   261

/* translate system call arguments */
struct xlate_table_t
{
  int target_val;
  int host_val;
};

int
xlate_arg(int target_val, struct xlate_table_t *map, int map_sz, char *name)
{
  int i;

  for (i=0; i < map_sz; i++)
    {
      if (target_val == map[i].target_val)
	return map[i].host_val;
    }

  /* not found, issue warning and return target_val */
  warn("could not translate argument for `%s': %d", name, target_val);
  return target_val;
}

/* internal system call buffer size, used primarily for file name arguments,
   argument larger than this will be truncated */
#define MAXBUFSIZE 		1024

/* total bytes to copy from a valid pointer argument for ioctl() calls,
   syscall.c does not decode ioctl() calls to determine the size of the
   arguments that reside in memory, instead, the ioctl() proxy simply copies
   NUM_IOCTL_BYTES bytes from the pointer argument to host memory */
#define NUM_IOCTL_BYTES		128

/* OSF ioctl() requests */
#define OSF_TIOCGETP		0x40067408
#define OSF_FIONREAD		0x4004667f

/* target stat() buffer definition, the host stat buffer format is
   automagically mapped to/from this format in syscall.c */
struct  osf_statbuf
{
  word_t osf_st_dev;
  word_t osf_st_ino;
  word_t osf_st_mode;
  half_t osf_st_nlink;
  half_t pad0;			/* to match Alpha/AXP padding... */
  word_t osf_st_uid;
  word_t osf_st_gid;
  word_t osf_st_rdev;
  word_t pad1;			/* to match Alpha/AXP padding... */
  qword_t osf_st_size;
  word_t osf_st_atime;
  word_t osf_st_spare1;
  word_t osf_st_mtime;
  word_t osf_st_spare2;
  word_t osf_st_ctime;
  word_t osf_st_spare3;
  word_t osf_st_blksize;
  word_t osf_st_blocks;
  word_t osf_st_gennum;
  word_t osf_st_spare4;
};

struct osf_sgttyb {
  byte_t sg_ispeed;	/* input speed */
  byte_t sg_ospeed;	/* output speed */
  byte_t sg_erase;	/* erase character */
  byte_t sg_kill;	/* kill character */
  shalf_t sg_flags;	/* mode flags */
};

#define OSF_NSIG		32

#define OSF_SIG_BLOCK		1
#define OSF_SIG_UNBLOCK		2
#define OSF_SIG_SETMASK		3

struct osf_sigcontext {
  qword_t sc_onstack;              /* sigstack state to restore */
  qword_t sc_mask;                 /* signal mask to restore */
  qword_t sc_pc;                   /* pc at time of signal */
  qword_t sc_ps;                   /* psl to retore */
  qword_t sc_regs[32];             /* processor regs 0 to 31 */
  qword_t sc_ownedfp;              /* fp has been used */
  qword_t sc_fpregs[32];           /* fp regs 0 to 31 */
  qword_t sc_fpcr;                 /* floating point control register */
  qword_t sc_fp_control;           /* software fpcr */
};

struct osf_statfs {
  shalf_t f_type;		/* type of filesystem (see below) */
  shalf_t f_flags;		/* copy of mount flags */
  word_t f_fsize;		/* fundamental filesystem block size */
  word_t f_bsize;		/* optimal transfer block size */
  word_t f_blocks;		/* total data blocks in file system, */
  /* note: may not represent fs size. */
  word_t f_bfree;		/* free blocks in fs */
  word_t f_bavail;		/* free blocks avail to non-su */
  word_t f_files;		/* total file nodes in file system */
  word_t f_ffree;		/* free file nodes in fs */
  qword_t f_fsid;		/* file system id */
  word_t f_spare[9];		/* spare for later */
};

struct osf_timeval
{
  sword_t osf_tv_sec;		/* seconds */
  sword_t osf_tv_usec;		/* microseconds */
};

struct osf_timezone
{
  sword_t osf_tz_minuteswest;	/* minutes west of Greenwich */
  sword_t osf_tz_dsttime;	/* type of dst correction */
};

/* target getrusage() buffer definition, the host stat buffer format is
   automagically mapped to/from this format in syscall.c */
struct osf_rusage
{
  struct osf_timeval osf_ru_utime;
  struct osf_timeval osf_ru_stime;
  sword_t osf_ru_maxrss;
  sword_t osf_ru_ixrss;
  sword_t osf_ru_idrss;
  sword_t osf_ru_isrss;
  sword_t osf_ru_minflt;
  sword_t osf_ru_majflt;
  sword_t osf_ru_nswap;
  sword_t osf_ru_inblock;
  sword_t osf_ru_oublock;
  sword_t osf_ru_msgsnd;
  sword_t osf_ru_msgrcv;
  sword_t osf_ru_nsignals;
  sword_t osf_ru_nvcsw;
  sword_t osf_ru_nivcsw;
};

struct osf_rlimit
{
  qword_t osf_rlim_cur;		/* current (soft) limit */
  qword_t osf_rlim_max;		/* maximum value for rlim_cur */
};

struct osf_sockaddr
{
  half_t sa_family;		/* address family, AF_xxx */
  byte_t sa_data[24];		/* 14 bytes of protocol address */
};

struct osf_iovec
{
  md_addr_t iov_base;		/* starting address */
  word_t iov_len;		/* length in bytes */
  word_t pad;
};

/* returns size of DIRENT structure */
#define OSF_DIRENT_SZ(STR)						\
  (sizeof(word_t) + 2*sizeof(half_t) + (((strlen(STR) + 1) + 3)/4)*4)
  /* was: (sizeof(word_t) + 2*sizeof(half_t) + strlen(STR) + 1) */

struct osf_dirent
{
  word_t d_ino;			/* file number of entry */
  half_t d_reclen;		/* length of this record */
  half_t d_namlen;		/* length of string in d_name */
  char d_name[256];		/* DUMMY NAME LENGTH */
				/* the real maximum length is */
				/* returned by pathconf() */
                                /* At this time, this MUST */
                                /* be 256 -- the kernel */
                                /* requires it */
};

/* open(2) flags for Alpha/AXP OSF target, syscall.c automagically maps
   between these codes to/from host open(2) flags */
#define OSF_O_RDONLY		0x0000
#define OSF_O_WRONLY		0x0001
#define OSF_O_RDWR		0x0002
#define OSF_O_NONBLOCK		0x0004
#define OSF_O_APPEND		0x0008
#define OSF_O_CREAT		0x0200
#define OSF_O_TRUNC		0x0400
#define OSF_O_EXCL		0x0800
#define OSF_O_NOCTTY		0x1000
#define OSF_O_SYNC		0x4000

/* open(2) flags translation table for SimpleScalar target */
struct {
  int osf_flag;
  int local_flag;
} osf_flag_table[] = {
  /* target flag */	/* host flag */
#ifdef _MSC_VER
  { OSF_O_RDONLY,	_O_RDONLY },
  { OSF_O_WRONLY,	_O_WRONLY },
  { OSF_O_RDWR,		_O_RDWR },
  { OSF_O_APPEND,	_O_APPEND },
  { OSF_O_CREAT,	_O_CREAT },
  { OSF_O_TRUNC,	_O_TRUNC },
  { OSF_O_EXCL,		_O_EXCL },
#ifdef _O_NONBLOCK
  { OSF_O_NONBLOCK,	_O_NONBLOCK },
#endif
#ifdef _O_NOCTTY
  { OSF_O_NOCTTY,	_O_NOCTTY },
#endif
#ifdef _O_SYNC
  { OSF_O_SYNC,		_O_SYNC },
#endif
#else /* !_MSC_VER */
  { OSF_O_RDONLY,	O_RDONLY },
  { OSF_O_WRONLY,	O_WRONLY },
  { OSF_O_RDWR,		O_RDWR },
  { OSF_O_APPEND,	O_APPEND },
  { OSF_O_CREAT,	O_CREAT },
  { OSF_O_TRUNC,	O_TRUNC },
  { OSF_O_EXCL,		O_EXCL },
  { OSF_O_NONBLOCK,	O_NONBLOCK },
  { OSF_O_NOCTTY,	O_NOCTTY },
#ifdef O_SYNC
  { OSF_O_SYNC,		O_SYNC },
#endif
#endif /* _MSC_VER */
};
#define OSF_NFLAGS	(sizeof(osf_flag_table)/sizeof(osf_flag_table[0]))

qword_t sigmask = 0;

qword_t sigaction_array[OSF_NSIG] =
 {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/* setsockopt option names */
#define OSF_SO_DEBUG		0x0001
#define OSF_SO_ACCEPTCONN	0x0002
#define OSF_SO_REUSEADDR	0x0004
#define OSF_SO_KEEPALIVE	0x0008
#define OSF_SO_DONTROUTE	0x0010
#define OSF_SO_BROADCAST	0x0020
#define OSF_SO_USELOOPBACK	0x0040
#define OSF_SO_LINGER		0x0080
#define OSF_SO_OOBINLINE	0x0100
#define OSF_SO_REUSEPORT	0x0200

struct xlate_table_t sockopt_map[] =
{
  { OSF_SO_DEBUG,	SO_DEBUG },
#ifdef SO_ACCEPTCONN
  { OSF_SO_ACCEPTCONN,	SO_ACCEPTCONN },
#endif
  { OSF_SO_REUSEADDR,	SO_REUSEADDR },
  { OSF_SO_KEEPALIVE,	SO_KEEPALIVE },
  { OSF_SO_DONTROUTE,	SO_DONTROUTE },
  { OSF_SO_BROADCAST,	SO_BROADCAST },
#ifdef SO_USELOOPBACK
  { OSF_SO_USELOOPBACK,	SO_USELOOPBACK },
#endif
  { OSF_SO_LINGER,	SO_LINGER },
  { OSF_SO_OOBINLINE,	SO_OOBINLINE },
#ifdef SO_REUSEPORT
  { OSF_SO_REUSEPORT,	SO_REUSEPORT }
#endif
};

/* setsockopt TCP options */
#define OSF_TCP_NODELAY		0x01 /* don't delay send to coalesce packets */
#define OSF_TCP_MAXSEG		0x02 /* maximum segment size */
#define OSF_TCP_RPTR2RXT	0x03 /* set repeat count for R2 RXT timer */
#define OSF_TCP_KEEPIDLE	0x04 /* secs before initial keepalive probe */
#define OSF_TCP_KEEPINTVL	0x05 /* seconds between keepalive probes */
#define OSF_TCP_KEEPCNT		0x06 /* num of keepalive probes before drop */
#define OSF_TCP_KEEPINIT	0x07 /* initial connect timeout (seconds) */
#define OSF_TCP_PUSH		0x08 /* set push bit in outbnd data packets */
#define OSF_TCP_NODELACK	0x09 /* don't delay send to coalesce packets */

struct xlate_table_t tcpopt_map[] =
{
  { OSF_TCP_NODELAY,	TCP_NODELAY },
  { OSF_TCP_MAXSEG,	TCP_MAXSEG },
#ifdef TCP_RPTR2RXT
  { OSF_TCP_RPTR2RXT,	TCP_RPTR2RXT },
#endif
#ifdef TCP_KEEPIDLE
  { OSF_TCP_KEEPIDLE,	TCP_KEEPIDLE },
#endif
#ifdef TCP_KEEPINTVL
  { OSF_TCP_KEEPINTVL,	TCP_KEEPINTVL },
#endif
#ifdef TCP_KEEPCNT
  { OSF_TCP_KEEPCNT,	TCP_KEEPCNT },
#endif
#ifdef TCP_KEEPINIT
  { OSF_TCP_KEEPINIT,	TCP_KEEPINIT },
#endif
#ifdef TCP_PUSH
  { OSF_TCP_PUSH,	TCP_PUSH },
#endif
#ifdef TCP_NODELACK
  { OSF_TCP_NODELACK,	TCP_NODELACK }
#endif
};

/* setsockopt level names */
#define OSF_SOL_SOCKET		0xffff	/* options for socket level */
#define OSF_SOL_IP		0	/* dummy for IP */
#define OSF_SOL_TCP		6	/* tcp */
#define OSF_SOL_UDP		17	/* user datagram protocol */

struct xlate_table_t socklevel_map[] =
{
#if defined(__svr4__) || defined(__osf__) || defined(__FreeBSD__)
  { OSF_SOL_SOCKET,	SOL_SOCKET },
  { OSF_SOL_IP,		IPPROTO_IP },
  { OSF_SOL_TCP,	IPPROTO_TCP },
  { OSF_SOL_UDP,	IPPROTO_UDP }
#else
  { OSF_SOL_SOCKET,	SOL_SOCKET },
  { OSF_SOL_IP,		SOL_IP },
  { OSF_SOL_TCP,	SOL_TCP },
  { OSF_SOL_UDP,	SOL_UDP }
#endif
};

/* socket() address families */
#define OSF_AF_UNSPEC		0
#define OSF_AF_UNIX		1	/* Unix domain sockets */
#define OSF_AF_INET		2	/* internet IP protocol */
#define OSF_AF_IMPLINK		3	/* arpanet imp addresses */
#define OSF_AF_PUP		4	/* pup protocols: e.g. BSP */
#define OSF_AF_CHAOS		5	/* mit CHAOS protocols */
#define OSF_AF_NS		6	/* XEROX NS protocols */
#define OSF_AF_ISO		7	/* ISO protocols */

struct xlate_table_t family_map[] =
{
  { OSF_AF_UNSPEC,	AF_UNSPEC },
  { OSF_AF_UNIX,	AF_UNIX },
  { OSF_AF_INET,	AF_INET },
#ifdef AF_IMPLINK
  { OSF_AF_IMPLINK,	AF_IMPLINK },
#endif
#ifdef AF_PUP
  { OSF_AF_PUP,		AF_PUP },
#endif
#ifdef AF_CHAOS
  { OSF_AF_CHAOS,	AF_CHAOS },
#endif
#ifdef AF_NS
  { OSF_AF_NS,		AF_NS },
#endif
#ifdef AF_ISO
  { OSF_AF_ISO,		AF_ISO }
#endif
};

/* socket() socket types */
#define OSF_SOCK_STREAM		1	/* stream (connection) socket */
#define OSF_SOCK_DGRAM		2	/* datagram (conn.less) socket */
#define OSF_SOCK_RAW		3	/* raw socket */
#define OSF_SOCK_RDM		4	/* reliably-delivered message */
#define OSF_SOCK_SEQPACKET	5	/* sequential packet socket */

struct xlate_table_t socktype_map[] =
{
  { OSF_SOCK_STREAM,	SOCK_STREAM },
  { OSF_SOCK_DGRAM,	SOCK_DGRAM },
  { OSF_SOCK_RAW,	SOCK_RAW },
  { OSF_SOCK_RDM,	SOCK_RDM },
  { OSF_SOCK_SEQPACKET,	SOCK_SEQPACKET }
};

/* OSF table() call. Right now, we only support TBL_SYSINFO queries */
#define OSF_TBL_SYSINFO		12
struct osf_tbl_sysinfo 
{
  long si_user;		/* user time */
  long si_nice;		/* nice time */
  long si_sys;		/* system time */
  long si_idle;		/* idle time */
  long si_hz;
  long si_phz;
  long si_boottime;	/* boot time in seconds */
  long wait;		/* wait time */
};


/* OSF SYSCALL -- standard system call sequence
   the kernel expects arguments to be passed with the normal C calling
   sequence; v0 should contain the system call number; on return from the
   kernel mode, a3 will be 0 to indicate no error and non-zero to indicate an
   error; if an error occurred v0 will contain an errno; if the kernel return
   an error, setup a valid gp and jmp to _cerror */

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
  qword_t syscode = regs->regs_R[MD_REG_V0];

  /* fix for syscall() which uses CALL_PAL CALLSYS for making system calls */
  if (syscode == OSF_SYS_syscall)
    syscode = regs->regs_R[MD_REG_A0];

  /* first, check if an EIO trace is being consumed... */
  if (traceable && sim_eio_fd != NULL)
    {
      eio_read_trace(sim_eio_fd, sim_num_insn, regs, mem_fn, mem, inst);

      /* kludge fix for sigreturn(), it modifies all registers */
      if (syscode == OSF_SYS_sigreturn)
	{
	  int i;
	  struct osf_sigcontext sc;
	  md_addr_t sc_addr = regs->regs_R[MD_REG_A0];

	  mem_bcopy(mem_fn, mem, Read, sc_addr, 
		    &sc, sizeof(struct osf_sigcontext));
	  regs->regs_NPC = sc.sc_pc;
	  for (i=0; i < 32; ++i)
	    regs->regs_R[i] = sc.sc_regs[i];
	  for (i=0; i < 32; ++i)
	    regs->regs_F.q[i] = sc.sc_fpregs[i];
	  regs->regs_C.fpcr = sc.sc_fpcr;
	}

      /* fini... */
      return;
    }

  /* no, OK execute the live system call... */
  switch (syscode)
    {
    case OSF_SYS_exit:
      /* exit jumps to the target set in main() */
      longjmp(sim_exit_buf,
	      /* exitcode + fudge */(regs->regs_R[MD_REG_A0] & 0xff) + 1);
      break;

    case OSF_SYS_read:
      {
	char *buf;

	/* allocate same-sized input buffer in host memory */
	if (!(buf =
	      (char *)calloc(/*nbytes*/regs->regs_R[MD_REG_A2], sizeof(char))))
	  fatal("out of memory in SYS_read");

	/* read data from file */
	do {
	  /*nread*/regs->regs_R[MD_REG_V0] =
	    read(/*fd*/regs->regs_R[MD_REG_A0], buf,
	         /*nbytes*/regs->regs_R[MD_REG_A2]);
	} while (/*nread*/regs->regs_R[MD_REG_V0] == -1
	         && errno == EAGAIN);

	/* check for error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* copy results back into host memory */
	mem_bcopy(mem_fn, mem, Write,
		  /*buf*/regs->regs_R[MD_REG_A1], buf, /*nread*/regs->regs_R[MD_REG_A2]);

	/* done with input buffer */
	free(buf);
      }
      break;

    case OSF_SYS_write:
      {
	char *buf;

	/* allocate same-sized output buffer in host memory */
	if (!(buf =
	      (char *)calloc(/*nbytes*/regs->regs_R[MD_REG_A2], sizeof(char))))
	  fatal("out of memory in SYS_write");

	/* copy inputs into host memory */
	mem_bcopy(mem_fn, mem, Read, /*buf*/regs->regs_R[MD_REG_A1], buf,
		  /*nbytes*/regs->regs_R[MD_REG_A2]);

	/* write data to file */
	if (sim_progfd && MD_OUTPUT_SYSCALL(regs))
	  {
	    /* redirect program output to file */

	    /*nwritten*/regs->regs_R[MD_REG_V0] =
	      fwrite(buf, 1, /*nbytes*/regs->regs_R[MD_REG_A2], sim_progfd);
	  }
	else
	  {
	    /* perform program output request */

	    do {
	      /*nwritten*/regs->regs_R[MD_REG_V0] =
	        write(/*fd*/regs->regs_R[MD_REG_A0],
		      buf, /*nbytes*/regs->regs_R[MD_REG_A2]);
	    } while (/*nwritten*/regs->regs_R[MD_REG_V0] == -1
		     && errno == EAGAIN);
	  }

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] == regs->regs_R[MD_REG_A2])
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* done with output buffer */
	free(buf);
      }
      break;

#if !defined(MIN_SYSCALL_MODE)
      /* ADDED BY CALDER 10/27/99 */
    case OSF_SYS_getdomainname:
      /* get program scheduling priority */
      {
	char *buf;

	buf = malloc(/* len */(size_t)regs->regs_R[MD_REG_A1]);
	if (!buf)
	  fatal("out of virtual memory in gethostname()");

	/* result */regs->regs_R[MD_REG_V0] =
	  getdomainname(/* name */buf,
		      /* len */(size_t)regs->regs_R[MD_REG_A1]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* copy string back to simulated memory */
	mem_bcopy(mem_fn, mem, Write,
		  /* name */regs->regs_R[MD_REG_A0],
		  buf, /* len */regs->regs_R[MD_REG_A1]);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
      /* ADDED BY CALDER 10/27/99 */
    case OSF_SYS_flock:
      /* get flock() information on the file */
      {
	regs->regs_R[MD_REG_V0] =
	  flock(/*fd*/(int)regs->regs_R[MD_REG_A0],
		/*cmd*/(int)regs->regs_R[MD_REG_A1]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
      /* ADDED BY CALDER 10/27/99 */
    case OSF_SYS_bind:
      {
	const struct sockaddr a_sock;

	mem_bcopy(mem_fn, mem, Read, /* serv_addr */regs->regs_R[MD_REG_A1],
		  (void *)&a_sock, /* addrlen */(int)regs->regs_R[MD_REG_A2]);

      regs->regs_R[MD_REG_V0] =
	bind((int) regs->regs_R[MD_REG_A0],
	     &a_sock,(int) regs->regs_R[MD_REG_A2]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
      /* ADDED BY CALDER 10/27/99 */
    case OSF_SYS_sendto:
      {
	char *buf = NULL;
	struct sockaddr d_sock;
	int buf_len = 0;

	buf_len = regs->regs_R[MD_REG_A2];

	if (buf_len > 0)
	  buf = (char *) malloc(buf_len*sizeof(char));

	mem_bcopy(mem_fn, mem, Read, /* serv_addr */regs->regs_R[MD_REG_A1],
		  buf, /* addrlen */(int)regs->regs_R[MD_REG_A2]);

	if (regs->regs_R[MD_REG_A5] > 0) 
	  mem_bcopy(mem_fn, mem, Read, regs->regs_R[MD_REG_A4],
		    &d_sock, (int)regs->regs_R[MD_REG_A5]);

	regs->regs_R[MD_REG_V0] =
	  sendto((int) regs->regs_R[MD_REG_A0],
		 buf,(int) regs->regs_R[MD_REG_A2],
		 (int) regs->regs_R[MD_REG_A3],
		 &d_sock,(int) regs->regs_R[MD_REG_A5]);

	mem_bcopy(mem_fn, mem, Write, /* serv_addr */regs->regs_R[MD_REG_A1],
		  buf, /* addrlen */(int)regs->regs_R[MD_REG_A2]);

	/* maybe copy back whole size of sockaddr */
	if (regs->regs_R[MD_REG_A5] > 0)
	  mem_bcopy(mem_fn, mem, Write, regs->regs_R[MD_REG_A4],
		    &d_sock, (int)regs->regs_R[MD_REG_A5]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	if (buf != NULL) 
	  free(buf);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
      /* ADDED BY CALDER 10/27/99 */
    case OSF_SYS_old_recvfrom:
    case OSF_SYS_recvfrom:
      {
	int addr_len;
	char *buf;
	struct sockaddr *a_sock;
      
	buf = (char *) malloc(sizeof(char)*regs->regs_R[MD_REG_A2]);

	mem_bcopy(mem_fn, mem, Read, /* serv_addr */regs->regs_R[MD_REG_A1],
		  buf, /* addrlen */(int)regs->regs_R[MD_REG_A2]);

	mem_bcopy(mem_fn, mem, Read, /* serv_addr */regs->regs_R[MD_REG_A5],
		  &addr_len, sizeof(int));

	a_sock = (struct sockaddr *)malloc(addr_len);

	mem_bcopy(mem_fn, mem, Read, regs->regs_R[MD_REG_A4],
		  a_sock, addr_len);

	regs->regs_R[MD_REG_V0] =
	  recvfrom((int) regs->regs_R[MD_REG_A0],
		   buf,(int) regs->regs_R[MD_REG_A2],
		   (int) regs->regs_R[MD_REG_A3], a_sock,&addr_len);

	mem_bcopy(mem_fn, mem, Write, regs->regs_R[MD_REG_A1],
		  buf, (int) regs->regs_R[MD_REG_V0]);

	mem_bcopy(mem_fn, mem, Write, /* serv_addr */regs->regs_R[MD_REG_A5],
		  &addr_len, sizeof(int));

	mem_bcopy(mem_fn, mem, Write, regs->regs_R[MD_REG_A4],
		  a_sock, addr_len);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
	if (buf != NULL)
	  free(buf);
      }
      break;
#endif

    case OSF_SYS_open:
      {
	char buf[MAXBUFSIZE];
	unsigned int i;
	int osf_flags = regs->regs_R[MD_REG_A1], local_flags = 0;

	/* translate open(2) flags */
	for (i=0; i < OSF_NFLAGS; i++)
	  {
	    if (osf_flags & osf_flag_table[i].osf_flag)
	      {
		osf_flags &= ~osf_flag_table[i].osf_flag;
		local_flags |= osf_flag_table[i].local_flag;
	      }
	  }
	/* any target flags left? */
	if (osf_flags != 0)
	  fatal("syscall: open: cannot decode flags: 0x%08x", osf_flags);

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	/* open the file */
	/*fd*/regs->regs_R[MD_REG_V0] =
	  open(buf, local_flags, /*mode*/regs->regs_R[MD_REG_A2]);
	
	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;

    case OSF_SYS_close:
      /* don't close stdin, stdout, or stderr as this messes up sim logs */
      if (/*fd*/regs->regs_R[MD_REG_A0] == 0
	  || /*fd*/regs->regs_R[MD_REG_A0] == 1
	  || /*fd*/regs->regs_R[MD_REG_A0] == 2)
	{
	  regs->regs_R[MD_REG_A3] = 0;
	  break;
	}

      /* close the file */
      regs->regs_R[MD_REG_V0] = close(/*fd*/regs->regs_R[MD_REG_A0]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;

#if 0
    case OSF_SYS_creat:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	/* create the file */
	/*fd*/regs->regs_R[MD_REG_V0] =
	  creat(buf, /*mode*/regs->regs_R[MD_REG_A1]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;
#endif

    case OSF_SYS_unlink:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	/* delete the file */
	/*result*/regs->regs_R[MD_REG_V0] = unlink(buf);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;

    case OSF_SYS_chdir:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	/* change the working directory */
	/*result*/regs->regs_R[MD_REG_V0] = chdir(buf);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;

    case OSF_SYS_chmod:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	/* chmod the file */
	/*result*/regs->regs_R[MD_REG_V0] =
	  chmod(buf, /*mode*/regs->regs_R[MD_REG_A1]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;

    case OSF_SYS_chown:
#ifdef _MSC_VER
      warn("syscall chown() not yet implemented for MSC...");
      regs->regs_R[MD_REG_A3] = 0;
#else /* !_MSC_VER */
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem,Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	/* chown the file */
	/*result*/regs->regs_R[MD_REG_V0] =
	  chown(buf, /*owner*/regs->regs_R[MD_REG_A1],
		/*group*/regs->regs_R[MD_REG_A2]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
#endif /* _MSC_VER */
      break;

    case OSF_SYS_sbrk:
      {
	sqword_t delta;
	md_addr_t addr;

	delta = regs->regs_R[MD_REG_A0];
	addr = ld_brk_point + delta;

	if (verbose)
	  myfprintf(stderr, "SYS_sbrk: delta: 0x%012p (%ld)\n", delta, delta);

	ld_brk_point = addr;
	regs->regs_R[MD_REG_V0] = ld_brk_point;
	regs->regs_R[MD_REG_A3] = 0;

	if (verbose)
	  myfprintf(stderr, "ld_brk_point: 0x%012p\n", ld_brk_point);

#if 0
	/* check whether heap area has merged with stack area */
	if (/* addr >= ld_brk_point && */ addr < regs->regs_R[MD_REG_SP])
	  {
	    regs->regs_R[MD_REG_A3] = 0;
	    ld_brk_point = addr;
	  }
	else
	  {
	    /* out of address space, indicate error */
	    regs->regs_R[MD_REG_A3] = -1;
	  }
#endif
      }
      break;

    case OSF_SYS_obreak:
      {
        md_addr_t addr;

        /* round the new heap pointer to the its page boundary */
#if 0
        addr = ROUND_UP(/*base*/regs->regs_R[MD_REG_A0], MD_PAGE_SIZE);
#endif
        addr = /*base*/regs->regs_R[MD_REG_A0];

	if (verbose)
	  myfprintf(stderr, "SYS_obreak: addr: 0x%012p\n", addr);

	ld_brk_point = addr;
	regs->regs_R[MD_REG_V0] = ld_brk_point;
	regs->regs_R[MD_REG_A3] = 0;

	if (verbose)
	  myfprintf(stderr, "ld_brk_point: 0x%012p\n", ld_brk_point);
      }
      break;

    case OSF_SYS_lseek:
      /* seek into file */
      regs->regs_R[MD_REG_V0] =
	lseek(/*fd*/regs->regs_R[MD_REG_A0],
	      /*off*/regs->regs_R[MD_REG_A1], /*dir*/regs->regs_R[MD_REG_A2]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;

    case OSF_SYS_getpid:
      /* get the simulator process id */
      /*result*/regs->regs_R[MD_REG_V0] = getpid();

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;

    case OSF_SYS_getuid:
#ifdef _MSC_VER
      warn("syscall getuid() not yet implemented for MSC...");
      regs->regs_R[MD_REG_A3] = 0;
#else /* !_MSC_VER */
      /* get current user id */
      /*first result*/regs->regs_R[MD_REG_V0] = getuid();
      /*second result*/regs->regs_R[MD_REG_A4] = geteuid();

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
#endif /* _MSC_VER */
      break;

    case OSF_SYS_access:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fName*/regs->regs_R[MD_REG_A0], buf);

	/* check access on the file */
	/*result*/regs->regs_R[MD_REG_V0] =
	  access(buf, /*mode*/regs->regs_R[MD_REG_A1]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;

    case OSF_SYS_stat:
    case OSF_SYS_lstat:
      {
	char buf[MAXBUFSIZE];
	struct osf_statbuf osf_sbuf;
#ifdef _MSC_VER
	struct _stat sbuf;
#else /* !_MSC_VER */
	struct stat sbuf;
#endif /* _MSC_VER */

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fName*/regs->regs_R[MD_REG_A0], buf);

	/* stat() the file */
	if (syscode == OSF_SYS_stat)
	  /*result*/regs->regs_R[MD_REG_V0] = stat(buf, &sbuf);
	else /* syscode == OSF_SYS_lstat */
	  {
#ifdef _MSC_VER
            warn("syscall lstat() not yet implemented for MSC...");
            regs->regs_R[MD_REG_A3] = 0;
            break;
#else /* !_MSC_VER */
	    /*result*/regs->regs_R[MD_REG_V0] = lstat(buf, &sbuf);
#endif /* _MSC_VER */
	  }

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* translate from host stat structure to target format */
	osf_sbuf.osf_st_dev = MD_SWAPW(sbuf.st_dev);
	osf_sbuf.osf_st_ino = MD_SWAPW(sbuf.st_ino);
	osf_sbuf.osf_st_mode = MD_SWAPW(sbuf.st_mode);
	osf_sbuf.osf_st_nlink = MD_SWAPH(sbuf.st_nlink);
	osf_sbuf.osf_st_uid = MD_SWAPW(sbuf.st_uid);
	osf_sbuf.osf_st_gid = MD_SWAPW(sbuf.st_gid);
	osf_sbuf.osf_st_rdev = MD_SWAPW(sbuf.st_rdev);
	osf_sbuf.osf_st_size = MD_SWAPQ(sbuf.st_size);
	osf_sbuf.osf_st_atime = MD_SWAPW(sbuf.st_atime);
	osf_sbuf.osf_st_mtime = MD_SWAPW(sbuf.st_mtime);
	osf_sbuf.osf_st_ctime = MD_SWAPW(sbuf.st_ctime);
#ifndef _MSC_VER
	osf_sbuf.osf_st_blksize = MD_SWAPW(sbuf.st_blksize);
	osf_sbuf.osf_st_blocks = MD_SWAPW(sbuf.st_blocks);
#endif /* !_MSC_VER */

	/* copy stat() results to simulator memory */
	mem_bcopy(mem_fn, mem, Write, /*sbuf*/regs->regs_R[MD_REG_A1],
		  &osf_sbuf, sizeof(struct osf_statbuf));
      }
      break;

    case OSF_SYS_dup:
      /* dup() the file descriptor */
      /*fd*/regs->regs_R[MD_REG_V0] = dup(/*fd*/regs->regs_R[MD_REG_A0]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;

#if 0
    case OSF_SYS_pipe:
      {
	int fd[2];

	/* copy pipe descriptors to host memory */;
	mem_bcopy(mem_fn, mem, Read, /*fd's*/regs->regs_R[MD_REG_A0],
		  fd, sizeof(fd));

	/* create a pipe */
	/*result*/regs->regs_R[7] = pipe(fd);

	/* copy descriptor results to result registers */
	/*pipe1*/regs->regs_R[MD_REG_V0] = fd[0];
	/*pipe 2*/regs->regs_R[3] = fd[1];

	/* check for an error condition */
	if (regs->regs_R[7] == (qword_t)-1)
	  {
	    regs->regs_R[MD_REG_V0] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;
#endif

    case OSF_SYS_getgid:
#ifdef _MSC_VER
      warn("syscall getgid() not yet implemented for MSC...");
      regs->regs_R[MD_REG_A3] = 0;
#else /* !_MSC_VER */
      /* get current group id */
      /*first result*/regs->regs_R[MD_REG_V0] = getgid();
      /*second result*/regs->regs_R[MD_REG_A4] = getegid();

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
#endif /* _MSC_VER */
      break;

    case OSF_SYS_ioctl:
      switch (/* req */regs->regs_R[MD_REG_A1])
	{
#if !defined(TIOCGETP) && (defined(linux) || defined(__CYGWIN32__))
	case OSF_TIOCGETP: /* <Out,TIOCGETP,6> */
	  {
	    struct termios lbuf;
	    struct osf_sgttyb buf;

	    /* result */regs->regs_R[MD_REG_V0] =
			  tcgetattr(/* fd */(int)regs->regs_R[MD_REG_A0],
				    &lbuf);

	    /* translate results */
	    buf.sg_ispeed = lbuf.c_ispeed;
	    buf.sg_ospeed = lbuf.c_ospeed;
	    buf.sg_erase = lbuf.c_cc[VERASE];
	    buf.sg_kill = lbuf.c_cc[VKILL];
	    buf.sg_flags = 0;	/* FIXME: this is wrong... */

	    mem_bcopy(mem_fn, mem, Write,
		      /* buf */regs->regs_R[MD_REG_A2], &buf,
		      sizeof(struct osf_sgttyb));

	    if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	      regs->regs_R[MD_REG_A3] = 0;
	    else /* probably not a typewriter, return details */
	      {
		regs->regs_R[MD_REG_A3] = -1;
		regs->regs_R[MD_REG_V0] = errno;
	      }
	  }
	  break;
#endif
#ifdef TIOCGETP
	case OSF_TIOCGETP: /* <Out,TIOCGETP,6> */
	  {
	    struct sgttyb lbuf;
	    struct osf_sgttyb buf;

	    /* result */regs->regs_R[MD_REG_V0] =
	      ioctl(/* fd */(int)regs->regs_R[MD_REG_A0],
		    /* req */TIOCGETP,
		    &lbuf);

	    /* translate results */
	    buf.sg_ispeed = lbuf.sg_ispeed;
	    buf.sg_ospeed = lbuf.sg_ospeed;
	    buf.sg_erase = lbuf.sg_erase;
	    buf.sg_kill = lbuf.sg_kill;
	    buf.sg_flags = lbuf.sg_flags;
	    mem_bcopy(mem_fn, mem, Write,
		      /* buf */regs->regs_R[MD_REG_A2], &buf,
		      sizeof(struct osf_sgttyb));

	    if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	      regs->regs_R[MD_REG_A3] = 0;
	    else /* not a typewriter, return details */
	      {
		regs->regs_R[MD_REG_A3] = -1;
		regs->regs_R[MD_REG_V0] = errno;
	      }
	  }
	  break;
#endif
#ifdef FIONREAD
	case OSF_FIONREAD:
	  {
	    int nread;

	    /* result */regs->regs_R[MD_REG_V0] =
	      ioctl(/* fd */(int)regs->regs_R[MD_REG_A0],
		    /* req */FIONREAD,
		    /* arg */&nread);

	    mem_bcopy(mem_fn, mem, Write,
		      /* arg */regs->regs_R[MD_REG_A2],
		      &nread, sizeof(nread));

	    if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	      regs->regs_R[MD_REG_A3] = 0;
	    else /* not a typewriter, return details */
	      {
		regs->regs_R[MD_REG_A3] = -1;
		regs->regs_R[MD_REG_V0] = errno;
	      }
	  }
	  break;
#endif
#ifdef FIONBIO
	case /*FIXME*/FIONBIO:
	  {
	    int arg = 0;

	    if (regs->regs_R[MD_REG_A2])
	      mem_bcopy(mem_fn, mem, Read,
		      /* arg */regs->regs_R[MD_REG_A2],
		      &arg, sizeof(arg));

#ifdef NOTNOW
	    fprintf(stderr, "FIONBIO: %d, %d\n",
		    (int)regs->regs_R[MD_REG_A0],
		    arg);
#endif
	    /* result */regs->regs_R[MD_REG_V0] =
	      ioctl(/* fd */(int)regs->regs_R[MD_REG_A0],
		    /* req */FIONBIO,
		    /* arg */&arg);

	    if (regs->regs_R[MD_REG_A2])
	      mem_bcopy(mem_fn, mem, Write,
		      /* arg */regs->regs_R[MD_REG_A2],
		      &arg, sizeof(arg));

	    if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	      regs->regs_R[MD_REG_A3] = 0;
	    else /* not a typewriter, return details */
	      {
		regs->regs_R[MD_REG_A3] = -1;
		regs->regs_R[MD_REG_V0] = errno;
	      }
	  }
	  break;
#endif
	default:
	  warn("unsupported ioctl call: ioctl(%ld, ...)",
	       regs->regs_R[MD_REG_A1]);
	  regs->regs_R[MD_REG_A3] = 0;
	  break;
	}
      break;

#if 0
      {
	char buf[NUM_IOCTL_BYTES];
	int local_req = 0;

	/* convert target ioctl() request to host ioctl() request values */
	switch (/*req*/regs->regs_R[MD_REG_A1]) {
/* #if !defined(__CYGWIN32__) */
	case SS_IOCTL_TIOCGETP:
	  local_req = TIOCGETP;
	  break;
	case SS_IOCTL_TIOCSETP:
	  local_req = TIOCSETP;
	  break;
	case SS_IOCTL_TCGETP:
	  local_req = TIOCGETP;
	  break;
/* #endif */
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
	case SS_IOCTL_TIOCGWINSZ:
	  local_req = TIOCGWINSZ;
	  break;
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

	if (!local_req)
	  {
	    /* FIXME: could not translate the ioctl() request, just warn user
	       and ignore the request */
	    warn("syscall: ioctl: ioctl code not supported d=%d, req=%d",
		 regs->regs_R[MD_REG_A0], regs->regs_R[MD_REG_A1]);
	    regs->regs_R[MD_REG_V0] = 0;
	    regs->regs_R[7] = 0;
	  }
	else
	  {
	    /* ioctl() code was successfully translated to a host code */

	    /* if arg ptr exists, copy NUM_IOCTL_BYTES bytes to host mem */
	    if (/*argp*/regs->regs_R[MD_REG_A2] != 0)
	      mem_bcopy(mem_fn, mem, Read, /*argp*/regs->regs_R[MD_REG_A2],
			buf, NUM_IOCTL_BYTES);

	    /* perform the ioctl() call */
	    /*result*/regs->regs_R[MD_REG_V0] =
	      ioctl(/*fd*/regs->regs_R[MD_REG_A0], local_req, buf);

	    /* if arg ptr exists, copy NUM_IOCTL_BYTES bytes from host mem */
	    if (/*argp*/regs->regs_R[MD_REG_A2] != 0)
	      mem_bcopy(mem_fn, mem, Write, regs->regs_R[MD_REG_A2],
			buf, NUM_IOCTL_BYTES);

	    /* check for an error condition */
	    if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	      regs->regs_R[7] = 0;
	    else
	      {	
		/* got an error, return details */
		regs->regs_R[MD_REG_V0] = errno;
		regs->regs_R[7] = 1;
	      }
	  }
      }
      break;
#endif

    case OSF_SYS_fstat:
      {
	struct osf_statbuf osf_sbuf;
#ifdef _MSC_VER
        struct _stat sbuf;
#else /* !_MSC_VER */
	struct stat sbuf;
#endif /* _MSC_VER */

	/* fstat() the file */
	/*result*/regs->regs_R[MD_REG_V0] =
	  fstat(/*fd*/regs->regs_R[MD_REG_A0], &sbuf);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* translate the stat structure to host format */
	osf_sbuf.osf_st_dev = MD_SWAPW(sbuf.st_dev);
	osf_sbuf.osf_st_ino = MD_SWAPW(sbuf.st_ino);
	osf_sbuf.osf_st_mode = MD_SWAPW(sbuf.st_mode);
	osf_sbuf.osf_st_nlink = MD_SWAPH(sbuf.st_nlink);
	osf_sbuf.osf_st_uid = MD_SWAPW(sbuf.st_uid);
	osf_sbuf.osf_st_gid = MD_SWAPW(sbuf.st_gid);
	osf_sbuf.osf_st_rdev = MD_SWAPW(sbuf.st_rdev);
	osf_sbuf.osf_st_size = MD_SWAPQ(sbuf.st_size);
	osf_sbuf.osf_st_atime = MD_SWAPW(sbuf.st_atime);
	osf_sbuf.osf_st_mtime = MD_SWAPW(sbuf.st_mtime);
	osf_sbuf.osf_st_ctime = MD_SWAPW(sbuf.st_ctime);
#ifndef _MSC_VER
	osf_sbuf.osf_st_blksize = MD_SWAPW(sbuf.st_blksize);
	osf_sbuf.osf_st_blocks = MD_SWAPW(sbuf.st_blocks);
#endif /* !_MSC_VER */

	/* copy fstat() results to simulator memory */
	mem_bcopy(mem_fn, mem, Write, /*sbuf*/regs->regs_R[MD_REG_A1],
		  &osf_sbuf, sizeof(struct osf_statbuf));
      }
      break;

    case OSF_SYS_getpagesize:
      /* get target pagesize */
      regs->regs_R[MD_REG_V0] = /* was: getpagesize() */MD_PAGE_SIZE;

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;

    case OSF_SYS_setitimer:
      /* FIXME: the sigvec system call is ignored */
      warn("syscall: setitimer ignored");
      regs->regs_R[MD_REG_A3] = 0;
      break;

    case OSF_SYS_table:
      {
	qword_t table_id, table_index, buf_addr, num_elem, size_elem;
	struct osf_tbl_sysinfo sysinfo;
	
	table_id = regs->regs_R[MD_REG_A1];
	table_index = regs->regs_R[MD_REG_A2];
	buf_addr = regs->regs_R[MD_REG_A3];
	num_elem = regs->regs_R[MD_REG_A4];
	size_elem = regs->regs_R[MD_REG_A5];
	
	switch(table_id)
	{
	case OSF_TBL_SYSINFO:
	  if (table_index != 0)
	    {
	      panic("table: table id TBL_SYSINFO requires 0 index, got %08d",
		    table_index );
	    }
	  else if (num_elem != 1)
	    {
	      panic("table: table id TBL_SYSINFO requires 1 elts, got %08d",
		    num_elem );
	    }
	  else
	    {
	      struct rusage rusage_info;
	      
	      /* use getrusage() to determine user & system time */
	      if (getrusage(RUSAGE_SELF, &rusage_info) < 0)
		{
		  /* abort the system call */
		  regs->regs_R[MD_REG_A3] = -1;
		  /* not kosher to pass off errno of getrusage() as errno
		     of table(), but what the heck... */
		  regs->regs_R[MD_REG_V0] = errno;
		  break;
		}
	      
	      /* use sysconf() to determine clock tick frequency */
	      sysinfo.si_hz = sysconf(_SC_CLK_TCK);

	      /* convert user and system time into clock ticks */
	      sysinfo.si_user = rusage_info.ru_utime.tv_sec * sysinfo.si_hz + 
		(rusage_info.ru_utime.tv_usec * sysinfo.si_hz) / 1000000UL;
	      sysinfo.si_sys = rusage_info.ru_stime.tv_sec * sysinfo.si_hz + 
		(rusage_info.ru_stime.tv_usec * sysinfo.si_hz) / 1000000UL;

	      /* following can't be determined in a portable manner and
		 are ignored */
	      sysinfo.si_nice = 0;
	      sysinfo.si_idle = 0;
	      sysinfo.si_phz = 0;
	      sysinfo.si_boottime = 0;
	      sysinfo.wait = 0;

	      /* copy structure into simulator memory */
	      mem_bcopy(mem_fn, mem, Write, buf_addr,
			&sysinfo, sizeof(struct osf_tbl_sysinfo));

	      /* return success */
	      regs->regs_R[MD_REG_A3] = 0;
	    }
	  break;

	default:
	  warn("table: unsupported table id %d requested, ignored", table_id);
	  regs->regs_R[MD_REG_A3] = 0;
	}
      }
      break;

    case OSF_SYS_getdtablesize:
#if defined(_AIX) || defined(__alpha)
      /* get descriptor table size */
      regs->regs_R[MD_REG_V0] = getdtablesize();

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
#elif defined(ultrix)
      {
	/* no comparable system call found, try some reasonable defaults */
	warn("syscall: called getdtablesize\n");
	regs->regs_R[MD_REG_V0] = 16;
	regs->regs_R[MD_REG_A3] = 0;
      }
#elif defined(MIN_SYSCALL_MODE)
      {
	/* no comparable system call found, try some reasonable defaults */
	warn("syscall: called getdtablesize\n");
	regs->regs_R[MD_REG_V0] = 16;
	regs->regs_R[MD_REG_A3] = 0;
      }
#else
      {
	struct rlimit rl;

	/* get descriptor table size in rlimit structure */
	if (getrlimit(RLIMIT_NOFILE, &rl) != (qword_t)-1)
	  {
	    regs->regs_R[MD_REG_V0] = rl.rlim_cur;
	    regs->regs_R[MD_REG_A3] = 0;
	  }
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
#endif
      break;

    case OSF_SYS_dup2:
      /* dup2() the file descriptor */
      regs->regs_R[MD_REG_V0] =
	dup2(/*fd1*/regs->regs_R[MD_REG_A0], /*fd2*/regs->regs_R[MD_REG_A1]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;

    case OSF_SYS_fcntl:
#ifdef _MSC_VER
      warn("syscall fcntl() not yet implemented for MSC...");
      regs->regs_R[MD_REG_A3] = 0;
#else /* !_MSC_VER */
      /* get fcntl() information on the file */
      regs->regs_R[MD_REG_V0] =
	fcntl(/*fd*/regs->regs_R[MD_REG_A0],
	      /*cmd*/regs->regs_R[MD_REG_A1], /*arg*/regs->regs_R[MD_REG_A2]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
#endif /* _MSC_VER */
      break;

#if 0
    case OSF_SYS_sigvec:
      /* FIXME: the sigvec system call is ignored */
      warn("syscall: sigvec ignored");
      regs->regs_R[MD_REG_A3] = 0;
      break;
#endif

#if 0
    case OSF_SYS_sigblock:
      /* FIXME: the sigblock system call is ignored */
      warn("syscall: sigblock ignored");
      regs->regs_R[MD_REG_A3] = 0;
      break;
#endif

#if 0
    case OSF_SYS_sigsetmask:
      /* FIXME: the sigsetmask system call is ignored */
      warn("syscall: sigsetmask ignored");
      regs->regs_R[MD_REG_A3] = 0;
      break;
#endif

    case OSF_SYS_gettimeofday:
#ifdef _MSC_VER
      warn("syscall gettimeofday() not yet implemented for MSC...");
      regs->regs_R[MD_REG_A3] = 0;
#else /* _MSC_VER */
      {
	struct osf_timeval osf_tv;
	struct timeval tv, *tvp;
	struct osf_timezone osf_tz;
	struct timezone tz, *tzp;

	if (/*timeval*/regs->regs_R[MD_REG_A0] != 0)
	  {
	    /* copy timeval into host memory */
	    mem_bcopy(mem_fn, mem, Read, /*timeval*/regs->regs_R[MD_REG_A0],
		      &osf_tv, sizeof(struct osf_timeval));

	    /* convert target timeval structure to host format */
	    tv.tv_sec = MD_SWAPW(osf_tv.osf_tv_sec);
	    tv.tv_usec = MD_SWAPW(osf_tv.osf_tv_usec);
	    tvp = &tv;
	  }
	else
	  tvp = NULL;

	if (/*timezone*/regs->regs_R[MD_REG_A1] != 0)
	  {
	    /* copy timezone into host memory */
	    mem_bcopy(mem_fn, mem, Read, /*timezone*/regs->regs_R[MD_REG_A1],
		      &osf_tz, sizeof(struct osf_timezone));

	    /* convert target timezone structure to host format */
	    tz.tz_minuteswest = MD_SWAPW(osf_tz.osf_tz_minuteswest);
	    tz.tz_dsttime = MD_SWAPW(osf_tz.osf_tz_dsttime);
	    tzp = &tz;
	  }
	else
	  tzp = NULL;

	/* get time of day */
	/*result*/regs->regs_R[MD_REG_V0] = gettimeofday(tvp, tzp);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	if (/*timeval*/regs->regs_R[MD_REG_A0] != 0)
	  {
	    /* convert host timeval structure to target format */
	    osf_tv.osf_tv_sec = MD_SWAPW(tv.tv_sec);
	    osf_tv.osf_tv_usec = MD_SWAPW(tv.tv_usec);

	    /* copy timeval to target memory */
	    mem_bcopy(mem_fn, mem, Write, /*timeval*/regs->regs_R[MD_REG_A0],
		      &osf_tv, sizeof(struct osf_timeval));
	  }

	if (/*timezone*/regs->regs_R[MD_REG_A1] != 0)
	  {
	    /* convert host timezone structure to target format */
	    osf_tz.osf_tz_minuteswest = MD_SWAPW(tz.tz_minuteswest);
	    osf_tz.osf_tz_dsttime = MD_SWAPW(tz.tz_dsttime);

	    /* copy timezone to target memory */
	    mem_bcopy(mem_fn, mem, Write, /*timezone*/regs->regs_R[MD_REG_A1],
		      &osf_tz, sizeof(struct osf_timezone));
	  }
      }
#endif /* !_MSC_VER */
      break;

    case OSF_SYS_getrusage:
#if defined(__svr4__) || defined(__USLC__) || defined(hpux) || defined(__hpux) || defined(_AIX)
      {
	struct tms tms_buf;
	struct osf_rusage rusage;

	/* get user and system times */
	if (times(&tms_buf) != (qword_t)-1)
	  {
	    /* no error */
	    regs->regs_R[MD_REG_V0] = 0;
	    regs->regs_R[MD_REG_A3] = 0;
	  }
	else /* got an error, indicate result */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* initialize target rusage result structure */
#if defined(__svr4__)
	memset(&rusage, '\0', sizeof(struct osf_rusage));
#else /* !defined(__svr4__) */
	bzero(&rusage, sizeof(struct osf_rusage));
#endif

	/* convert from host rusage structure to target format */
	rusage.osf_ru_utime.osf_tv_sec = MD_SWAPW(tms_buf.tms_utime/CLK_TCK);
	rusage.osf_ru_utime.osf_tv_sec =
	  MD_SWAPW(rusage.osf_ru_utime.osf_tv_sec);
	rusage.osf_ru_utime.osf_tv_usec = 0;
	rusage.osf_ru_stime.osf_tv_sec = MD_SWAPW(tms_buf.tms_stime/CLK_TCK);
	rusage.osf_ru_stime.osf_tv_sec =
	  MD_SWAPW(rusage.osf_ru_stime.osf_tv_sec);
	rusage.osf_ru_stime.osf_tv_usec = 0;

	/* copy rusage results into target memory */
	mem_bcopy(mem_fn, mem, Write, /*rusage*/regs->regs_R[MD_REG_A1],
		  &rusage, sizeof(struct osf_rusage));
      }
#elif defined(__unix__)
      {
	struct rusage local_rusage;
	struct osf_rusage rusage;

	/* get rusage information */
	/*result*/regs->regs_R[MD_REG_V0] =
	  getrusage(/*who*/regs->regs_R[MD_REG_A0], &local_rusage);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* convert from host rusage structure to target format */
	rusage.osf_ru_utime.osf_tv_sec =
	  MD_SWAPW(local_rusage.ru_utime.tv_sec);
	rusage.osf_ru_utime.osf_tv_usec =
	  MD_SWAPW(local_rusage.ru_utime.tv_usec);
	rusage.osf_ru_utime.osf_tv_sec =
	  MD_SWAPW(local_rusage.ru_utime.tv_sec);
	rusage.osf_ru_utime.osf_tv_usec =
	  MD_SWAPW(local_rusage.ru_utime.tv_usec);
	rusage.osf_ru_stime.osf_tv_sec =
	  MD_SWAPW(local_rusage.ru_stime.tv_sec);
	rusage.osf_ru_stime.osf_tv_usec =
	  MD_SWAPW(local_rusage.ru_stime.tv_usec);
	rusage.osf_ru_stime.osf_tv_sec =
	  MD_SWAPW(local_rusage.ru_stime.tv_sec);
	rusage.osf_ru_stime.osf_tv_usec =
	  MD_SWAPW(local_rusage.ru_stime.tv_usec);
	rusage.osf_ru_maxrss = MD_SWAPW(local_rusage.ru_maxrss);
	rusage.osf_ru_ixrss = MD_SWAPW(local_rusage.ru_ixrss);
	rusage.osf_ru_idrss = MD_SWAPW(local_rusage.ru_idrss);
	rusage.osf_ru_isrss = MD_SWAPW(local_rusage.ru_isrss);
	rusage.osf_ru_minflt = MD_SWAPW(local_rusage.ru_minflt);
	rusage.osf_ru_majflt = MD_SWAPW(local_rusage.ru_majflt);
	rusage.osf_ru_nswap = MD_SWAPW(local_rusage.ru_nswap);
	rusage.osf_ru_inblock = MD_SWAPW(local_rusage.ru_inblock);
	rusage.osf_ru_oublock = MD_SWAPW(local_rusage.ru_oublock);
	rusage.osf_ru_msgsnd = MD_SWAPW(local_rusage.ru_msgsnd);
	rusage.osf_ru_msgrcv = MD_SWAPW(local_rusage.ru_msgrcv);
	rusage.osf_ru_nsignals = MD_SWAPW(local_rusage.ru_nsignals);
	rusage.osf_ru_nvcsw = MD_SWAPW(local_rusage.ru_nvcsw);
	rusage.osf_ru_nivcsw = MD_SWAPW(local_rusage.ru_nivcsw);

	/* copy rusage results into target memory */
	mem_bcopy(mem_fn, mem, Write, /*rusage*/regs->regs_R[MD_REG_A1],
		  &rusage, sizeof(struct osf_rusage));
      }
#elif /* defined(__CYGWIN32__) || */ defined(_MSC_VER)
	    warn("syscall: called getrusage\n");
            regs->regs_R[7] = 0;
#else
#error No getrusage() implementation!
#endif
      break;

    case OSF_SYS_utimes:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	if (/*timeval*/regs->regs_R[MD_REG_A1] == 0)
	  {
#if defined(hpux) || defined(__hpux) || defined(linux)
	    /* no utimes() in hpux, use utime() instead */
	    /*result*/regs->regs_R[MD_REG_V0] = utime(buf, NULL);
#elif defined(_MSC_VER)
            /* no utimes() in MSC, use utime() instead */
	    /*result*/regs->regs_R[MD_REG_V0] = utime(buf, NULL);
#elif defined(__svr4__) || defined(__USLC__) || defined(unix) || defined(_AIX) || defined(__alpha)
	    /*result*/regs->regs_R[MD_REG_V0] = utimes(buf, NULL);
#elif defined(__CYGWIN32__)
	    warn("syscall: called utimes\n");
#else
#error No utimes() implementation!
#endif
	  }
	else
	  {
	    struct osf_timeval osf_tval[2];
#ifndef _MSC_VER
	    struct timeval tval[2];
#endif /* !_MSC_VER */

	    /* copy timeval structure to host memory */
	    mem_bcopy(mem_fn, mem, Read, /*timeout*/regs->regs_R[MD_REG_A1],
		      osf_tval, 2*sizeof(struct osf_timeval));

#ifndef _MSC_VER
	    /* convert timeval structure to host format */
	    tval[0].tv_sec = MD_SWAPW(osf_tval[0].osf_tv_sec);
	    tval[0].tv_usec = MD_SWAPW(osf_tval[0].osf_tv_usec);
	    tval[1].tv_sec = MD_SWAPW(osf_tval[1].osf_tv_sec);
	    tval[1].tv_usec = MD_SWAPW(osf_tval[1].osf_tv_usec);
#endif /* !_MSC_VER */

#if defined(hpux) || defined(__hpux) || defined(__svr4__)
	    /* no utimes() in hpux, use utime() instead */
	    {
	      struct utimbuf ubuf;

	      ubuf.actime = MD_SWAPW(tval[0].tv_sec);
	      ubuf.modtime = MD_SWAPW(tval[1].tv_sec);

	      /* result */regs->regs_R[MD_REG_V0] = utime(buf, &ubuf);
	    }
#elif defined(_MSC_VER)
            /* no utimes() in hpux, use utime() instead */
            {
              struct _utimbuf ubuf;

              ubuf.actime = MD_SWAPW(osf_tval[0].osf_tv_sec);
              ubuf.modtime = MD_SWAPW(osf_tval[1].osf_tv_sec);

              /* result */regs->regs_R[MD_REG_V0] = utime(buf, &ubuf);
            }
#elif defined(__USLC__) || defined(unix) || defined(_AIX) || defined(__alpha)
	    /* result */regs->regs_R[MD_REG_V0] = utimes(buf, tval);
#elif defined(__CYGWIN32__)
	    warn("syscall: called utimes\n");
#else
#error No utimes() implementation!
#endif
	  }

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;

    case OSF_SYS_getrlimit:
    case OSF_SYS_setrlimit:
#ifdef _MSC_VER
      warn("syscall get/setrlimit() not yet implemented for MSC...");
      regs->regs_R[MD_REG_A3] = 0;
#elif defined(__CYGWIN32__)
      {
	warn("syscall: called get/setrlimit\n");
	regs->regs_R[MD_REG_A3] = 0;
      }
#else
      {
	struct osf_rlimit osf_rl;
	struct rlimit rl;

	/* copy rlimit structure to host memory */
	mem_bcopy(mem_fn, mem, Read, /*rlimit*/regs->regs_R[MD_REG_A1],
		  &osf_rl, sizeof(struct osf_rlimit));

	/* convert rlimit structure to host format */
	rl.rlim_cur = MD_SWAPQ(osf_rl.osf_rlim_cur);
	rl.rlim_max = MD_SWAPQ(osf_rl.osf_rlim_max);

	/* get rlimit information */
	if (syscode == OSF_SYS_getrlimit)
	  /*result*/regs->regs_R[MD_REG_V0] =
	    getrlimit(regs->regs_R[MD_REG_A0], &rl);
	else /* syscode == OSF_SYS_setrlimit */
	  /*result*/regs->regs_R[MD_REG_V0] =
	    setrlimit(regs->regs_R[MD_REG_A0], &rl);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* convert rlimit structure to target format */
	osf_rl.osf_rlim_cur = MD_SWAPQ(rl.rlim_cur);
	osf_rl.osf_rlim_max = MD_SWAPQ(rl.rlim_max);

	/* copy rlimit structure to target memory */
	mem_bcopy(mem_fn, mem, Write, /*rlimit*/regs->regs_R[MD_REG_A1],
		  &osf_rl, sizeof(struct osf_rlimit));
      }
#endif
      break;

    case OSF_SYS_sigprocmask:
      {
	static int first = TRUE;

	if (first)
	  {
	    warn("partially supported sigprocmask() call...");
	    first = FALSE;
	  }

	/* from klauser@cs.colorado.edu: there are a couple bugs in the
	   sigprocmask implementation; here is a fix: the problem comes from an
	   impedance mismatch between application/libc interface and
	   libc/syscall interface, the former of which you read about in the
	   manpage, the latter of which you actually intercept here.  The
	   following is mostly correct, but does not capture some minor
	   details, which you only get right if you really let the kernel
	   handle it. (e.g. you can't really ever block sigkill etc.) */

        regs->regs_R[MD_REG_V0] = sigmask;
        regs->regs_R[MD_REG_A3] = 0;

        switch (regs->regs_R[MD_REG_A0])
	  {
          case OSF_SIG_BLOCK:
            sigmask |= regs->regs_R[MD_REG_A1];
            break;
          case OSF_SIG_UNBLOCK:
            sigmask &= ~regs->regs_R[MD_REG_A1];
            break;
          case OSF_SIG_SETMASK:
            sigmask = regs->regs_R[MD_REG_A1];
            break;
          default:
            regs->regs_R[MD_REG_V0] = EINVAL;
            regs->regs_R[MD_REG_A3] = 1;
	  }

#if 0 /* FIXME: obsolete... */
	if (regs->regs_R[MD_REG_A2] > /* FIXME: why? */0x10000000)
	  mem_bcopy(mem_fn, mem, Write, regs->regs_R[MD_REG_A2],
		    &sigmask, sizeof(sigmask));

	if (regs->regs_R[MD_REG_A1] != 0)
	  {
	    switch (regs->regs_R[MD_REG_A0])
	      {
	      case OSF_SIG_BLOCK:
		sigmask |= regs->regs_R[MD_REG_A1];
		break;
	      case OSF_SIG_UNBLOCK:
		sigmask &= regs->regs_R[MD_REG_A1]; /* I think */
	      break;
	      case OSF_SIG_SETMASK:
		sigmask = regs->regs_R[MD_REG_A1]; /* I think */
		break;
	      default:
		panic("illegal how value to sigprocmask()");
	      }
	  }
	regs->regs_R[MD_REG_V0] = 0;
	regs->regs_R[MD_REG_A3] = 0;
#endif
      }
      break;

    case OSF_SYS_sigaction:
      {
	int signum;
	static int first = TRUE;

	if (first)
	  {
	    warn("partially supported sigaction() call...");
	    first = FALSE;
	  }

	signum = regs->regs_R[MD_REG_A0];
	if (regs->regs_R[MD_REG_A1] != 0)
	  sigaction_array[signum] = regs->regs_R[MD_REG_A1];

	if (regs->regs_R[MD_REG_A2])
	  regs->regs_R[MD_REG_A2] = sigaction_array[signum];

	regs->regs_R[MD_REG_V0] = 0;

	/* for some reason, __sigaction expects A3 to have a 0 return value */
	regs->regs_R[MD_REG_A3] = 0;
  
	/* FIXME: still need to add code so that on a signal, the 
	   correct action is actually taken. */

	/* FIXME: still need to add support for returning the correct
	   error messages (EFAULT, EINVAL) */
      }
      break;

    case OSF_SYS_sigstack:
      warn("unsupported sigstack() call...");
      regs->regs_R[MD_REG_A3] = 0;
      break;

    case OSF_SYS_sigreturn:
      {
	int i;
	struct osf_sigcontext sc;
	static int first = TRUE;

	if (first)
	  {
	    warn("partially supported sigreturn() call...");
	    first = FALSE;
	  }

	mem_bcopy(mem_fn, mem, Read, /* sc */regs->regs_R[MD_REG_A0],
		  &sc, sizeof(struct osf_sigcontext));

	sigmask = MD_SWAPQ(sc.sc_mask); /* was: prog_sigmask */
	regs->regs_NPC = MD_SWAPQ(sc.sc_pc);

	/* FIXME: should check for the branch delay bit */
	/* FIXME: current->nextpc = current->pc + 4; not sure about this... */
	for (i=0; i < 32; ++i)
	  regs->regs_R[i] = sc.sc_regs[i];
	for (i=0; i < 32; ++i)
	  regs->regs_F.q[i] = sc.sc_fpregs[i];
	regs->regs_C.fpcr = sc.sc_fpcr;
      }
      break;

    case OSF_SYS_uswitch:
      warn("unsupported uswitch() call...");
      regs->regs_R[MD_REG_V0] = regs->regs_R[MD_REG_A1]; 
      break;

    case OSF_SYS_setsysinfo:
      warn("unsupported setsysinfo() call...");
      regs->regs_R[MD_REG_V0] = 0; 
      break;

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_getdirentries:
      {
	int i, cnt, osf_cnt;
	struct dirent *p;
	sword_t fd = regs->regs_R[MD_REG_A0];
	md_addr_t osf_buf = regs->regs_R[MD_REG_A1];
	char *buf;
	sword_t osf_nbytes = regs->regs_R[MD_REG_A2];
	md_addr_t osf_pbase = regs->regs_R[MD_REG_A3];
	sqword_t osf_base;
	long base = 0;

	/* number of entries in simulated memory */
	if (!osf_nbytes)
	  warn("attempting to get 0 directory entries...");

	/* allocate local memory, whatever fits */
	buf = calloc(1, osf_nbytes);
	if (!buf)
	  fatal("out of virtual memory");

	/* get directory entries */
#if defined(__svr4__)
	base = lseek ((int)fd, (off_t)0, SEEK_CUR);
	regs->regs_R[MD_REG_V0] =
	  getdents((int)fd, (struct dirent *)buf, (size_t)osf_nbytes);
#else /* !__svr4__ */
	regs->regs_R[MD_REG_V0] =
	  getdirentries((int)fd, buf, (size_t)osf_nbytes, &base);
#endif

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  {
	    regs->regs_R[MD_REG_A3] = 0;

	    /* anything to copy back? */
	    if (regs->regs_R[MD_REG_V0] > 0)
	      {
		/* copy all possible results to simulated space */
		for (i=0, cnt=0, osf_cnt=0, p=(struct dirent *)buf;
		     cnt < regs->regs_R[MD_REG_V0] && p->d_reclen > 0;
		     i++, cnt += p->d_reclen, p=(struct dirent *)(buf+cnt))
		  {
		    struct osf_dirent osf_dirent;

		    osf_dirent.d_ino = MD_SWAPW(p->d_ino);
		    osf_dirent.d_namlen = MD_SWAPH(strlen(p->d_name));
		    strcpy(osf_dirent.d_name, p->d_name);
		    osf_dirent.d_reclen = MD_SWAPH(OSF_DIRENT_SZ(p->d_name));

		    mem_bcopy(mem_fn, mem, Write,
			      osf_buf + osf_cnt,
			      &osf_dirent, OSF_DIRENT_SZ(p->d_name));
		    osf_cnt += OSF_DIRENT_SZ(p->d_name);
		  }

		if (osf_pbase != 0)
		  {
		    osf_base = (sqword_t)base;
		    mem_bcopy(mem_fn, mem, Write, osf_pbase,
			      &osf_base, sizeof(osf_base));
		  }

		/* update V0 to indicate translated read length */
		regs->regs_R[MD_REG_V0] = osf_cnt;
	      }
	  }
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	free(buf);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_truncate:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	/* truncate the file */
	/*result*/regs->regs_R[MD_REG_V0] =
	  truncate(buf, /* length */(size_t)regs->regs_R[MD_REG_A1]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;
#endif

#if /*!defined(__CYGWIN32__) &&*/ !defined(_MSC_VER)
    case OSF_SYS_ftruncate:
      /* truncate the file */
      /*result*/regs->regs_R[MD_REG_V0] =
	ftruncate(/* fd */(int)regs->regs_R[MD_REG_A0],
		 /* length */(size_t)regs->regs_R[MD_REG_A1]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_statfs:
      {
	char buf[MAXBUFSIZE];
	struct osf_statfs osf_sbuf;
	struct statfs sbuf;

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fName*/regs->regs_R[MD_REG_A0], buf);

	/* statfs() the fs */
	/*result*/regs->regs_R[MD_REG_V0] = statfs(buf, &sbuf);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* translate from host stat structure to target format */
#if defined(__svr4__) || defined(__osf__)
	osf_sbuf.f_type = MD_SWAPH(0x6969) /* NFS, whatever... */;
#else /* !__svr4__ */
	osf_sbuf.f_type = MD_SWAPH(sbuf.f_type);
#endif
	osf_sbuf.f_fsize = MD_SWAPW(sbuf.f_bsize);
	osf_sbuf.f_blocks = MD_SWAPW(sbuf.f_blocks);
	osf_sbuf.f_bfree = MD_SWAPW(sbuf.f_bfree);
	osf_sbuf.f_bavail = MD_SWAPW(sbuf.f_bavail);
	osf_sbuf.f_files = MD_SWAPW(sbuf.f_files);
	osf_sbuf.f_ffree = MD_SWAPW(sbuf.f_ffree);
	/* osf_sbuf.f_fsid = MD_SWAPW(sbuf.f_fsid); */

	/* copy stat() results to simulator memory */
	mem_bcopy(mem_fn, mem, Write, /*sbuf*/regs->regs_R[MD_REG_A1],
		  &osf_sbuf, sizeof(struct osf_statbuf));
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_setregid:
      /* set real and effective group ID */

      /*result*/regs->regs_R[MD_REG_V0] =
	setregid(/* rgid */(gid_t)regs->regs_R[MD_REG_A0],
		 /* egid */(gid_t)regs->regs_R[MD_REG_A1]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_setreuid:
      /* set real and effective user ID */

      /*result*/regs->regs_R[MD_REG_V0] =
	setreuid(/* ruid */(uid_t)regs->regs_R[MD_REG_A0],
		 /* euid */(uid_t)regs->regs_R[MD_REG_A1]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_socket:
      /* create an endpoint for communication */

      /* result */regs->regs_R[MD_REG_V0] =
	socket(/* domain */xlate_arg((int)regs->regs_R[MD_REG_A0],
				     family_map, N_ELT(family_map),
				     "socket(family)"),
	       /* type */xlate_arg((int)regs->regs_R[MD_REG_A1],
				   socktype_map, N_ELT(socktype_map),
				   "socket(type)"),
	       /* protocol */xlate_arg((int)regs->regs_R[MD_REG_A2],
				       family_map, N_ELT(family_map),
				       "socket(proto)"));

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_connect:
      {
	struct osf_sockaddr osf_sa;

	/* initiate a connection on a socket */

	/* get the socket address */
	if (regs->regs_R[MD_REG_A2] > sizeof(struct osf_sockaddr))
	  {
	    fatal("sockaddr size overflow: addrlen = %d",
		  regs->regs_R[MD_REG_A2]);
	  }
	/* copy sockaddr structure to host memory */
	mem_bcopy(mem_fn, mem, Read, /* serv_addr */regs->regs_R[MD_REG_A1],
		  &osf_sa, /* addrlen */(int)regs->regs_R[MD_REG_A2]);
#if 0
	int i;
	sa.sa_family = osf_sa.sa_family;
	for (i=0; i < regs->regs_R[MD_REG_A2]; i++)
	  sa.sa_data[i] = osf_sa.sa_data[i];
#endif
	/* result */regs->regs_R[MD_REG_V0] =
	  connect(/* sockfd */(int)regs->regs_R[MD_REG_A0],
		  (void *)&osf_sa, /* addrlen */(int)regs->regs_R[MD_REG_A2]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_uname:
      /* get name and information about current kernel */

      regs->regs_R[MD_REG_A3] = -1;
      regs->regs_R[MD_REG_V0] = EPERM;
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_writev:
      {
	int i;
	char *buf;
	struct iovec *iov;

	/* allocate host side I/O vectors */
	iov =
	  (struct iovec *)malloc(/* iovcnt */regs->regs_R[MD_REG_A2]
				 * sizeof(struct iovec));
	if (!iov)
	  fatal("out of virtual memory in SYS_writev");

	/* copy target side I/O vector buffers to host memory */
	for (i=0; i < /* iovcnt */regs->regs_R[MD_REG_A2]; i++)
	  {
	    struct osf_iovec osf_iov;

	    /* copy target side pointer data into host side vector */
	    mem_bcopy(mem_fn, mem, Read,
		      (/*iov*/regs->regs_R[MD_REG_A1]
		       + i*sizeof(struct osf_iovec)),
		      &osf_iov, sizeof(struct osf_iovec));

	    iov[i].iov_len = MD_SWAPW(osf_iov.iov_len);
	    if (osf_iov.iov_base != 0 && osf_iov.iov_len != 0)
	      {
		buf = (char *)calloc(MD_SWAPW(osf_iov.iov_len), sizeof(char));
		if (!buf)
		  fatal("out of virtual memory in SYS_writev");
		mem_bcopy(mem_fn, mem, Read, MD_SWAPQ(osf_iov.iov_base),
			  buf, MD_SWAPW(osf_iov.iov_len));
		iov[i].iov_base = buf;
	      }
	    else
	      iov[i].iov_base = NULL;
	  }

	/* perform the vector'ed write */
	do {
	  /*result*/regs->regs_R[MD_REG_V0] =
	    writev(/* fd */(int)regs->regs_R[MD_REG_A0], iov,
		   /* iovcnt */(size_t)regs->regs_R[MD_REG_A2]);
	} while (/*result*/regs->regs_R[MD_REG_V0] == -1
	         && errno == EAGAIN);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* free all the allocated memory */
	for (i=0; i < /* iovcnt */regs->regs_R[MD_REG_A2]; i++)
	  {
	    if (iov[i].iov_base)
	      {
		free(iov[i].iov_base);
		iov[i].iov_base = NULL;
	      }
	  }
	free(iov);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_readv:
      {
	int i;
	char *buf = NULL;
	struct osf_iovec *osf_iov;
	struct iovec *iov;

	/* allocate host side I/O vectors */
	osf_iov =
	  calloc(/* iovcnt */regs->regs_R[MD_REG_A2],
		 sizeof(struct osf_iovec));
	if (!osf_iov)
	  fatal("out of virtual memory in SYS_readv");

	iov =
	  calloc(/* iovcnt */regs->regs_R[MD_REG_A2], sizeof(struct iovec));
	if (!iov)
	  fatal("out of virtual memory in SYS_readv");

	/* copy host side I/O vector buffers */
	for (i=0; i < /* iovcnt */regs->regs_R[MD_REG_A2]; i++)
	  {
	    /* copy target side pointer data into host side vector */
	    mem_bcopy(mem_fn, mem, Read,
		      (/*iov*/regs->regs_R[MD_REG_A1]
		       + i*sizeof(struct osf_iovec)),
		      &osf_iov[i], sizeof(struct osf_iovec));

	    iov[i].iov_len = MD_SWAPW(osf_iov[i].iov_len);
	    if (osf_iov[i].iov_base != 0 && osf_iov[i].iov_len != 0)
	      {
		buf =
		  (char *)calloc(MD_SWAPW(osf_iov[i].iov_len), sizeof(char));
		if (!buf)
		  fatal("out of virtual memory in SYS_readv");
		iov[i].iov_base = buf;
	      }
	    else
	      iov[i].iov_base = NULL;
	  }

	/* perform the vector'ed read */
	do {
	  /*result*/regs->regs_R[MD_REG_V0] =
	    readv(/* fd */(int)regs->regs_R[MD_REG_A0], iov,
		  /* iovcnt */(size_t)regs->regs_R[MD_REG_A2]);
	} while (/*result*/regs->regs_R[MD_REG_V0] == -1
		 && errno == EAGAIN);

	/* copy target side I/O vector buffers to host memory */
	for (i=0; i < /* iovcnt */regs->regs_R[MD_REG_A2]; i++)
	  {
	    if (osf_iov[i].iov_base != 0)
	      {
		mem_bcopy(mem_fn, mem, Write, MD_SWAPQ(osf_iov[i].iov_base),
			  iov[i].iov_base, MD_SWAPW(osf_iov[i].iov_len));
	      }
	  }

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* free all the allocated memory */
	for (i=0; i < /* iovcnt */regs->regs_R[MD_REG_A2]; i++)
	  {
	    if (iov[i].iov_base)
	      {
		free(iov[i].iov_base);
		iov[i].iov_base = NULL;
	      }
	  }

	if (osf_iov)
	  free(osf_iov);
	if (iov)
	  free(iov);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_setsockopt:
      {
	char *buf;
	struct xlate_table_t *map;
	int nmap;

 	/* set options on sockets */

	/* copy optval to host memory */
	if (/* optval */regs->regs_R[MD_REG_A3] != 0
	    && /* optlen */regs->regs_R[MD_REG_A4] != 0)
	  {
	    buf = calloc(1, /* optlen */(size_t)regs->regs_R[MD_REG_A4]);
	    if (!buf)
	      fatal("cannot allocate memory in OSF_SYS_setsockopt");
	    
	    /* copy target side pointer data into host side vector */
	    mem_bcopy(mem_fn, mem, Read,
		      /* optval */regs->regs_R[MD_REG_A3],
		      buf, /* optlen */(int)regs->regs_R[MD_REG_A4]);
	  }
	else
	  buf = NULL;

	/* pick the correct translation table */
	if ((int)regs->regs_R[MD_REG_A1] == OSF_SOL_SOCKET)
	  {
	    map = sockopt_map;
	    nmap = N_ELT(sockopt_map);
	  }
	else if ((int)regs->regs_R[MD_REG_A1] == OSF_SOL_TCP)
	  {
	    map = tcpopt_map;
	    nmap = N_ELT(tcpopt_map);
	  }
	else
	  {
	    warn("no translation map available for `setsockopt()': %d",
		 (int)regs->regs_R[MD_REG_A1]);
	    map = sockopt_map;
	    nmap = N_ELT(sockopt_map);
	  }

	/* result */regs->regs_R[MD_REG_V0] =
	  setsockopt(/* sock */(int)regs->regs_R[MD_REG_A0],
		     /* level */xlate_arg((int)regs->regs_R[MD_REG_A1],
					  socklevel_map, N_ELT(socklevel_map),
					  "setsockopt(level)"),
		     /* optname */xlate_arg((int)regs->regs_R[MD_REG_A2],
					    map, nmap,
					    "setsockopt(opt)"),
		     /* optval */buf,
		     /* optlen */regs->regs_R[MD_REG_A4]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	if (buf != NULL)
	  free(buf);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_old_getsockname:
      {
	/* get socket name */
	char *buf;
	word_t osf_addrlen;
	int addrlen;

	/* get simulator memory parameters to host memory */
	mem_bcopy(mem_fn, mem, Read,
		  /* paddrlen */regs->regs_R[MD_REG_A2],
		  &osf_addrlen, sizeof(osf_addrlen));
	addrlen = (int)osf_addrlen;
	if (addrlen != 0)
	  {
	    buf = calloc(1, addrlen);
	    if (!buf)
	      fatal("cannot allocate memory in OSF_SYS_old_getsockname");
	  }
	else
	  buf = NULL;
	
	/* result */regs->regs_R[MD_REG_V0] =
	  getsockname(/* sock */(int)regs->regs_R[MD_REG_A0],
		      /* name */(struct sockaddr *)buf,
		      /* namelen */&addrlen);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* copy results to simulator memory */
	if (addrlen != 0)
	  mem_bcopy(mem_fn, mem, Write,
		    /* addr */regs->regs_R[MD_REG_A1],
		    buf, addrlen);
	osf_addrlen = (qword_t)addrlen;
	mem_bcopy(mem_fn, mem, Write,
		  /* paddrlen */regs->regs_R[MD_REG_A2],
		  &osf_addrlen, sizeof(osf_addrlen));

	if (buf != NULL)
	  free(buf);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_old_getpeername:
      {
	/* get socket name */
	char *buf;
	word_t osf_addrlen;
	int addrlen;

	/* get simulator memory parameters to host memory */
	mem_bcopy(mem_fn, mem, Read,
		  /* paddrlen */regs->regs_R[MD_REG_A2],
		  &osf_addrlen, sizeof(osf_addrlen));
	addrlen = (int)osf_addrlen;
	if (addrlen != 0)
	  {
	    buf = calloc(1, addrlen);
	    if (!buf)
	      fatal("cannot allocate memory in OSF_SYS_old_getsockname");
	  }
	else
	  buf = NULL;
	
	/* result */regs->regs_R[MD_REG_V0] =
	  getpeername(/* sock */(int)regs->regs_R[MD_REG_A0],
		      /* name */(struct sockaddr *)buf,
		      /* namelen */&addrlen);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* copy results to simulator memory */
	if (addrlen != 0)
	  mem_bcopy(mem_fn, mem, Write,
		    /* addr */regs->regs_R[MD_REG_A1],
		    buf, addrlen);
	osf_addrlen = (qword_t)addrlen;
	mem_bcopy(mem_fn, mem, Write,
		  /* paddrlen */regs->regs_R[MD_REG_A2],
		  &osf_addrlen, sizeof(osf_addrlen));

	if (buf != NULL)
	  free(buf);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_setgid:
      /* set group ID */

      /*result*/regs->regs_R[MD_REG_V0] =
	setgid(/* gid */(gid_t)regs->regs_R[MD_REG_A0]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_setuid:
      /* set user ID */

      /*result*/regs->regs_R[MD_REG_V0] =
	setuid(/* uid */(uid_t)regs->regs_R[MD_REG_A0]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_getpriority:
      /* get program scheduling priority */

      /*result*/regs->regs_R[MD_REG_V0] =
	getpriority(/* which */(int)regs->regs_R[MD_REG_A0],
		    /* who */(int)regs->regs_R[MD_REG_A1]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_setpriority:
      /* set program scheduling priority */

      /*result*/regs->regs_R[MD_REG_V0] =
	setpriority(/* which */(int)regs->regs_R[MD_REG_A0],
		    /* who */(int)regs->regs_R[MD_REG_A1],
		    /* prio */(int)regs->regs_R[MD_REG_A2]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_select:
      {
	fd_set readfd, writefd, exceptfd;
	fd_set *readfdp, *writefdp, *exceptfdp;
	struct timeval timeout, *timeoutp;

	/* copy read file descriptor set into host memory */
	if (/* readfds */regs->regs_R[MD_REG_A1] != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read,
		      /* readfds */regs->regs_R[MD_REG_A1],
		      &readfd, sizeof(fd_set));
	    readfdp = &readfd;
	  }
	else
	  readfdp = NULL;

	/* copy write file descriptor set into host memory */
	if (/* writefds */regs->regs_R[MD_REG_A2] != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read,
		      /* writefds */regs->regs_R[MD_REG_A2],
		      &writefd, sizeof(fd_set));
	    writefdp = &writefd;
	  }
	else
	  writefdp = NULL;

	/* copy exception file descriptor set into host memory */
	if (/* exceptfds */regs->regs_R[MD_REG_A3] != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read,
		      /* exceptfds */regs->regs_R[MD_REG_A3],
		      &exceptfd, sizeof(fd_set));
	    exceptfdp = &exceptfd;
	  }
	else
	  exceptfdp = NULL;

	/* copy timeout value into host memory */
	if (/* timeout */regs->regs_R[MD_REG_A4] != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read,
		      /* timeout */regs->regs_R[MD_REG_A4],
		      &timeout, sizeof(struct timeval));
	    timeoutp = &timeout;
	  }
	else
	  timeoutp = NULL;

#if defined(hpux) || defined(__hpux)
	/* select() on the specified file descriptors */
	/* result */regs->regs_R[MD_REG_V0] =
	  select(/* nfds */regs->regs_R[MD_REG_A0],
		 (int *)readfdp, (int *)writefdp, (int *)exceptfdp, timeoutp);
#else
	/* select() on the specified file descriptors */
	/* result */regs->regs_R[MD_REG_V0] =
	  select(/* nfds */regs->regs_R[MD_REG_A0],
		 readfdp, writefdp, exceptfdp, timeoutp);
#endif

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* copy read file descriptor set to target memory */
	if (/* readfds */regs->regs_R[MD_REG_A1] != 0)
	  mem_bcopy(mem_fn, mem, Write,
		    /* readfds */regs->regs_R[MD_REG_A1],
		    &readfd, sizeof(fd_set));

	/* copy write file descriptor set to target memory */
	if (/* writefds */regs->regs_R[MD_REG_A2] != 0)
	  mem_bcopy(mem_fn, mem, Write,
		    /* writefds */regs->regs_R[MD_REG_A2],
		    &writefd, sizeof(fd_set));

	/* copy exception file descriptor set to target memory */
	if (/* exceptfds */regs->regs_R[MD_REG_A3] != 0)
	  mem_bcopy(mem_fn, mem, Write,
		    /* exceptfds */regs->regs_R[MD_REG_A3],
		    &exceptfd, sizeof(fd_set));

	/* copy timeout value result to target memory */
	if (/* timeout */regs->regs_R[MD_REG_A4] != 0)
	  mem_bcopy(mem_fn, mem, Write,
		    /* timeout */regs->regs_R[MD_REG_A4],
		    &timeout, sizeof(struct timeval));
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_shutdown:
      /* shuts down socket send and receive operations */

      /*result*/regs->regs_R[MD_REG_V0] =
	shutdown(/* sock */(int)regs->regs_R[MD_REG_A0],
		 /* how */(int)regs->regs_R[MD_REG_A1]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_poll:
      {
	int i;
	struct pollfd *fds;

	/* allocate host side I/O vectors */
	fds = calloc(/* nfds */regs->regs_R[MD_REG_A1], sizeof(struct pollfd));
	if (!fds)
	  fatal("out of virtual memory in SYS_poll");

	/* copy target side I/O vector buffers to host memory */
	for (i=0; i < /* nfds */regs->regs_R[MD_REG_A1]; i++)
	  {
	    /* copy target side pointer data into host side vector */
	    mem_bcopy(mem_fn, mem, Read,
		      (/* fds */regs->regs_R[MD_REG_A0]
		       + i*sizeof(struct pollfd)),
		      &fds[i], sizeof(struct pollfd));
	  }

	/* perform the vector'ed write */
	/* result */regs->regs_R[MD_REG_V0] =
	  poll(/* fds */fds,
	       /* nfds */(unsigned long)regs->regs_R[MD_REG_A1],
	       /* timeout */(int)regs->regs_R[MD_REG_A2]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* copy target side I/O vector buffers to host memory */
	for (i=0; i < /* nfds */regs->regs_R[MD_REG_A1]; i++)
	  {
	    /* copy target side pointer data into host side vector */
	    mem_bcopy(mem_fn, mem, Write,
		      (/* fds */regs->regs_R[MD_REG_A0]
		       + i*sizeof(struct pollfd)),
		      &fds[i], sizeof(struct pollfd));
	  }

	/* free all the allocated memory */
	free(fds);
      }
      break;
#endif

    case OSF_SYS_usleep_thread:
#if 0
      fprintf(stderr, "usleep(%d)\n", (unsigned int)regs->regs_R[MD_REG_A0]);
#endif
#ifdef alpha
      regs->regs_R[MD_REG_V0] = usleep((unsigned int)regs->regs_R[MD_REG_A0]);
#else
      usleep((unsigned int)regs->regs_R[MD_REG_A0]);
      regs->regs_R[MD_REG_V0] = 0;
#endif
      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
        regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
        {
          regs->regs_R[MD_REG_A3] = -1;
          regs->regs_R[MD_REG_V0] = errno;
        }
#if 0
      warn("unsupported usleep_thread() call...");
      regs->regs_R[MD_REG_V0] = 0; 
#endif
      break;
      
#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_gethostname:
      /* get program scheduling priority */
      {
	char *buf;

	buf = malloc(/* len */(size_t)regs->regs_R[MD_REG_A1]);
	if (!buf)
	  fatal("out of virtual memory in gethostname()");

	/* result */regs->regs_R[MD_REG_V0] =
	  gethostname(/* name */buf,
		      /* len */(size_t)regs->regs_R[MD_REG_A1]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* copy string back to simulated memory */
	mem_bcopy(mem_fn, mem, Write,
		  /* name */regs->regs_R[MD_REG_A0],
		  buf, /* len */regs->regs_R[MD_REG_A1]);
      }
      break;
#endif

    case OSF_SYS_madvise:
      warn("unsupported madvise() call ignored...");
      regs->regs_R[MD_REG_V0] = 0;
      break;

    default:
      warn("invalid/unimplemented syscall %ld, PC=0x%08p, RA=0x%08p, winging it",
	   syscode, regs->regs_PC, regs->regs_R[MD_REG_RA]);
      regs->regs_R[MD_REG_A3] = -1;
      regs->regs_R[MD_REG_V0] = 0;
#if 0
      fatal("invalid/unimplemented system call encountered, code %d", syscode);
#endif
    }

  if (verbose)
    fprintf(stderr, "syscall(%d): returned %d:%d...\n",
            (int)syscode, (int)regs->regs_R[MD_REG_A3],
            (int)regs->regs_R[MD_REG_V0]);
}
