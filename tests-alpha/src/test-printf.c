/* Copyright (C) 1991, 1992, 1993 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

#ifdef	BSD
#include </usr/include/stdio.h>
#define EXIT_SUCCESS 0
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

/* #include <float.h> */

struct {
  int a;
  double b;
  double c;
  char d;
} xxyyz[100];

int zzz;

double
FooBar(int a)
{
  return xxyyz[a].b;
}

double uxxe[100];

double
FooFoo(int a)
{
  zzz++;
  return uxxe[a];
}

static char *sccsid = "this is a test...";

void
fmtchk(char *fmt)
{
  static char *sccsid = "this is a test...";
  (void) fputs(fmt, stdout);
  (void) printf(":\t`");
  (void) printf(fmt, 0x12);
  (void) printf("'\n");
}

void
fmtst1chk(char *fmt)
{
  (void) fputs(fmt, stdout);
  (void) printf(":\t`");
  (void) printf(fmt, 4, 0x12);
  (void) printf("'\n");
}

void
fmtst2chk(char *fmt)
{
  (void) fputs(fmt, stdout);
  (void) printf(":\t`");
  (void) printf(fmt, 4, 4, 0x12);
  (void) printf("'\n");
}

/* This page is covered by the following copyright: */

/* (C) Copyright C E Chew
 *
 * Feel free to copy, use and distribute this software provided:
 *
 *	1. you do not pretend that you wrote it
 *	2. you leave this copyright notice intact.
 */

/*
 * Extracted from exercise.c for glibc-1.05 bug report by Bruce Evans.
 */

#define DEC -123
#define INT 255
#define UNS (~0)

/* Formatted Output Test
 *
 * This exercises the output formatting code.
 */

void
fp_test(void)
{
  int i, j, k, l;
  char buf[7];
  char *prefix = buf;
  char tp[20];

  puts("\nFormatted output test");
  printf("prefix  6d      6o      6x      6X      6u\n");
  strcpy(prefix, "%");
  for (i = 0; i < 2; i++) {
    for (j = 0; j < 2; j++) {
      for (k = 0; k < 2; k++) {
	for (l = 0; l < 2; l++) {
	  strcpy(prefix, "%");
	  if (i == 0) strcat(prefix, "-");
	  if (j == 0) strcat(prefix, "+");
	  if (k == 0) strcat(prefix, "#");
	  if (l == 0) strcat(prefix, "0");
	  printf("%5s |", prefix);
	  strcpy(tp, prefix);
	  strcat(tp, "6d |");
	  printf(tp, DEC);
	  strcpy(tp, prefix);
	  strcat(tp, "6o |");
	  printf(tp, INT);
	  strcpy(tp, prefix);
	  strcat(tp, "6x |");
	  printf(tp, INT);
	  strcpy(tp, prefix);
	  strcat(tp, "6X |");
	  printf(tp, INT);
	  strcpy(tp, prefix);
	  strcat(tp, "6u |");
	  printf(tp, UNS);
	  printf("\n");
	}
      }
    }
  }
  printf("%10s\n", (char *) NULL);
  printf("%-10s\n", (char *) NULL);
}

int
main(void)
{
  static char shortstr[] = "Hi, Z.";
  static char longstr[] = "Good morning, Doctor Chandra.  This is Hal.  \
I am ready for my first lesson today.";

  fmtchk("%.4x");
  fmtchk("%04x");
  fmtchk("%4.4x");
  fmtchk("%04.4x");
  fmtchk("%4.3x");
  fmtchk("%04.3x");

  fmtst1chk("%.*x");
  fmtst1chk("%0*x");
  fmtst2chk("%*.*x");
  fmtst2chk("%0*.*x");

#ifndef	BSD
  printf("bad format:\t\"%z\"\n");
  printf("nil pointer (padded):\t\"%10p\"\n", (void *) NULL);
#endif

  printf("decimal negative:\t\"%d\"\n", -2345);
  printf("octal negative:\t\"%o\"\n", -2345);
  printf("hex negative:\t\"%x\"\n", -2345);
  printf("long decimal number:\t\"%ld\"\n", -123456);
  printf("long octal negative:\t\"%lo\"\n", -2345L);
  printf("long unsigned decimal number:\t\"%lu\"\n", -123456);
  printf("zero-padded LDN:\t\"%010ld\"\n", -123456);
  printf("left-adjusted ZLDN:\t\"%-010ld\"\n", -123456);
  printf("space-padded LDN:\t\"%10ld\"\n", -123456);
  printf("left-adjusted SLDN:\t\"%-10ld\"\n", -123456);

  printf("zero-padded string:\t\"%010s\"\n", shortstr);
  printf("left-adjusted Z string:\t\"%-010s\"\n", shortstr);
  printf("space-padded string:\t\"%10s\"\n", shortstr);
  printf("left-adjusted S string:\t\"%-10s\"\n", shortstr);
  printf("null string:\t\"%s\"\n", (char *)NULL);
#if 0
  printf("limited string:\t\"%.22s\"\n", longstr);
#endif

  printf("e-style >= 1:\t\"%e\"\n", 12.34);
  printf("e-style >= .1:\t\"%e\"\n", 0.1234);
  printf("e-style < .1:\t\"%e\"\n", 0.001234);
  printf("e-style big:\t\"%.60e\"\n", 1e20);
  printf ("e-style == .1:\t\"%e\"\n", 0.1);
  printf("f-style >= 1:\t\"%f\"\n", 12.34);
  printf("f-style >= .1:\t\"%f\"\n", 0.1234);
  printf("f-style < .1:\t\"%f\"\n", 0.001234);
  printf("g-style >= 1:\t\"%g\"\n", 12.34);
  printf("g-style >= .1:\t\"%g\"\n", 0.1234);
  printf("g-style < .1:\t\"%g\"\n", 0.001234);
  printf("g-style big:\t\"%.60g\"\n", 1e20);

  {
    char buf[128];
    sprintf(buf, "new test: %.15g", 99.85);
    printf(":%s:\n", buf);
  }

  printf (" %6.5lf\n", .099999999860301614);
  printf (" %6.5lf\n", .1);
  printf ("x%5.4lfx\n", .5);

  printf ("%#03x\n", 1);

#if 0				/* XXX printf_fp still broken */
  {
    double d = FLT_MIN;
    int niter = 17;
    
    while (niter-- != 0)
      printf ("%.17e\n", d / 2);
    fflush (stdout);
  }

  printf ("%15.5e\n", 4.9406564584124654e-324);
#endif

#define FORMAT "|%12.4f|%12.4e|%12.4g|\n"
  printf (FORMAT, 0.0, 0.0, 0.0);
  printf (FORMAT, 1.0, 1.0, 1.0);
  printf (FORMAT, -1.0, -1.0, -1.0);
  printf (FORMAT, 100.0, 100.0, 100.0);
  printf (FORMAT, 1000.0, 1000.0, 1000.0);
  printf (FORMAT, 10000.0, 10000.0, 10000.0);
  printf (FORMAT, 12345.0, 12345.0, 12345.0);
  printf (FORMAT, 100000.0, 100000.0, 100000.0);
  printf (FORMAT, 123456.0, 123456.0, 123456.0);
#undef	FORMAT

#if 0
  {
    char buf[20];
    printf ("snprintf (\"%%30s\", \"foo\") == %d, \"%.*s\"\n",
	    snprintf (buf, sizeof (buf), "%30s", "foo"), sizeof (buf), buf);
  }
#endif

  fp_test ();
  fp_test ();
  fp_test ();
  fp_test ();
  fp_test ();
  fp_test ();
  fp_test ();
  fp_test ();

  printf ("%e should be 1.234568e+06\n", 1234567.8);
  printf ("%f should be 1234567.800000\n", 1234567.8);
  printf ("%g should be 1.23457e+06\n", 1234567.8);
  printf ("%g should be 123.456\n", 123.456);
  printf ("%g should be 1e+06\n", 1000000.0);
  printf ("%g should be 10\n", 10.0);
  printf ("%g should be 0.02\n", 0.02);

  {
    char buf[128];
    sprintf(buf, "%#.*f", 4, (double)0.70000);
    printf("testing parametric fields: %s\n", buf);
  }

  return(0);
}
