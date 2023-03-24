/* misc.h - miscellaneous interfaces */

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


#ifndef MISC_H
#define MISC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>

/* boolean value defs */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* various useful macros */
#ifndef MAX
#define MAX(a, b)    (((a) < (b)) ? (b) : (a))
#endif
#ifndef MIN
#define MIN(a, b)    (((a) < (b)) ? (a) : (b))
#endif

/* for printing out "long long" vars */
#define LLHIGH(L)		((int)(((L)>>32) & 0xffffffff))
#define LLLOW(L)		((int)((L) & 0xffffffff))

/* size of an array, in elements */
#define N_ELT(ARR)   (sizeof(ARR)/sizeof((ARR)[0]))

/* rounding macros, assumes ALIGN is a power of two */
#define ROUND_UP(N,ALIGN)	(((N) + ((ALIGN)-1)) & ~((ALIGN)-1))
#define ROUND_DOWN(N,ALIGN)	((N) & ~((ALIGN)-1))

/* verbose output flag */
extern int verbose;

#ifdef DEBUG
/* active debug flag */
extern int debugging;
#endif /* DEBUG */

/* register a function to be called when an error is detected */
void
fatal_hook(void (*hook_fn)(FILE *stream));	/* fatal hook function */

#ifdef __GNUC__
/* declare a fatal run-time error, calls fatal hook function */
#define fatal(fmt, args...)	\
  _fatal(__FILE__, __FUNCTION__, __LINE__, fmt, ## args)

void
_fatal(char *file, const char *func, int line, char *fmt, ...)
__attribute__ ((noreturn));
#else /* !__GNUC__ */
void
fatal(char *fmt, ...);
#endif /* !__GNUC__ */

#ifdef __GNUC__
/* declare a panic situation, dumps core */
#define panic(fmt, args...)	\
  _panic(__FILE__, __FUNCTION__, __LINE__, fmt, ## args)

void
_panic(char *file, const char *func, int line, char *fmt, ...)
__attribute__ ((noreturn));
#else /* !__GNUC__ */
void
panic(char *fmt, ...);
#endif /* !__GNUC__ */

#ifdef __GNUC__
/* declare a warning */
#define warn(fmt, args...)	\
  _warn(__FILE__, __FUNCTION__, __LINE__, fmt, ## args)

void
_warn(char *file, const char *func, int line, char *fmt, ...);
#else /* !__GNUC__ */
void
warn(char *fmt, ...);
#endif /* !__GNUC__ */

#ifdef __GNUC__
/* print general information */
#define info(fmt, args...)	\
  _info(__FILE__, __FUNCTION__, __LINE__, fmt, ## args)

void
_info(char *file, const char *func, int line, char *fmt, ...);
#else /* !__GNUC__ */
void
info(char *fmt, ...);
#endif /* !__GNUC__ */

#ifdef DEBUG

#ifdef __GNUC__
/* print a debugging message */
#define debug(fmt, args...)	\
    do {                        \
        if (debugging)         	\
            _debug(__FILE__, __FUNCTION__, __LINE__, fmt, ## args); \
    } while(0)

void
_debug(char *file, const char *func, int line, char *fmt, ...);
#else /* !__GNUC__ */
void
debug(char *fmt, ...);
#endif /* !__GNUC__ */

#else /* !DEBUG */

#ifdef __GNUC__
#define debug(fmt, args...)
#else /* !__GNUC__ */
/* the optimizer should eliminate this call! */
static void debug(char *fmt, ...) {}
#endif /* !__GNUC__ */

#endif /* !DEBUG */

/* seed the random number generator */
void
mysrand(unsigned int seed);	/* random number generator seed */

/* get a random number */
int myrand(void);		/* returns random number */

/* copy a string to a new storage allocation (NOTE: many machines are missing
   this trivial function, so I funcdup() it here...) */
char *				/* duplicated string */
mystrdup(char *s);		/* string to duplicate to heap storage */

/* find the last occurrence of a character in a string */
char *
mystrrchr(char *s, char c);

/* case insensitive string compare (NOTE: many machines are missing this
   trivial function, so I funcdup() it here...) */
int				/* compare result, see strcmp() */
mystricmp(char *s1, char *s2);	/* strings to compare, case insensitive */

/* allocate some core, this memory has overhead no larger than a page
   in size and it cannot be released. the storage is returned cleared */
void *getcore(int nbytes);

/* return log of a number to the base 2 */
int log_base2(int n);

/* return string describing elapsed time, passed in SEC in seconds */
char *elapsed_time(long sec);

/* assume bit positions numbered 31 to 0 (31 high order bit), extract num bits
   from word starting at position pos (with pos as the high order bit of those
   to be extracted), result is right justified and zero filled to high order
   bit, for example, extractl(word, 6, 3) w/ 8 bit word = 01101011 returns
   00000110 */
unsigned int
extractl(int word,		/* the word from which to extract */
         int pos,		/* bit positions 31 to 0 */
         int num);		/* number of bits to extract */

#if defined(sparc) && !defined(__svr4__)
#define strtoul strtol
#endif

/* portable 64-bit I/O package */

/* portable vsprintf with qword support, returns end pointer */
char *myvsprintf(char *obuf, char *format, va_list v);

/* portable sprintf with qword support, returns end pointer */
char *mysprintf(char *obuf, char *format, ...);

/* portable vfprintf with qword support, returns end pointer */
void myvfprintf(FILE *stream, char *format, va_list v);

/* portable fprintf with qword support, returns end pointer */
void myfprintf(FILE *stream, char *format, ...);

#ifdef HOST_HAS_QWORD

/* convert a string to a signed result */
sqword_t myatosq(char *nptr, char **endp, int base);

/* convert a string to a unsigned result */
qword_t myatoq(char *nptr, char **endp, int base);

#endif /* HOST_HAS_QWORD */

/* same semantics as fopen() except that filenames ending with a ".gz" or ".Z"
   will be automagically get compressed */
FILE *gzopen(char *fname, char *type);

/* close compressed stream */
void gzclose(FILE *fd);

/* update the CRC on the data block one byte at a time */
word_t crc(word_t crc_accum, word_t data);

#endif /* MISC_H */
