/* bitmap.h - bit manipulation macros */

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


#ifndef BITMAP_H
#define BITMAP_H

/* BITMAPs:
     BMAP: int * to an array of ints
     SZ: number of ints in the bitmap
*/

/* declare a bitmap type */
#define BITMAP_SIZE(BITS)	(((BITS)+31)/32)
#define BITMAP_TYPE(BITS, NAME)	unsigned int (NAME)[BITMAP_SIZE(BITS)]

typedef unsigned int BITMAP_ENT_TYPE;
typedef unsigned int *BITMAP_PTR_TYPE;

/* set entire bitmap */
#define BITMAP_SET_MAP(BMAP, SZ)				\
  { int i; for (i=0; i<(SZ); i++) (BMAP)[i] = 0xffffffff; }

/* clear entire bitmap */
#define BITMAP_CLEAR_MAP(BMAP, SZ)				\
  { int i; for (i=0; i<(SZ); i++) (BMAP)[i] = 0; }

/* set bit BIT in bitmap BMAP, returns BMAP */
#define BITMAP_SET(BMAP, SZ, BIT)				\
  (((BMAP)[(BIT)/32] |= (1 << ((BIT) % 32))), (BMAP))

/* clear bit BIT in bitmap BMAP, returns BMAP */
#define BITMAP_CLEAR(BMAP, SZ, BIT)				\
  (((BMAP)[(BIT)/32] &= ~(1 << ((BIT) % 32))), (BMAP))

/* copy bitmap SRC to DEST */
#define BITMAP_COPY(DESTMAP, SRCMAP, SZ)			\
  { int i; for (i=0; i<(SZ); i++) (DESTMAP)[i] = (SRCMAP)[i]; }

/* store bitmap B2 OP B3 into B1 */
#define __BITMAP_OP(B1, B2, B3, SZ, OP)				\
  { int i; for (i=0; i<(SZ); i++) (B1)[i] = (B2)[i] OP (B3)[i]; }

/* store bitmap B2 | B3 into B1 */
#define BITMAP_IOR(B1, B2, B3, SZ)				\
  __BITMAP_OP(B1, B2, B3, SZ, |)

/* store bitmap B2 ^ B3 into B1 */
#define BITMAP_XOR(B1, B2, B3, SZ)				\
  __BITMAP_OP(B1, B2, B3, SZ, ^)

/* store bitmap B2 & B3 into B1 */
#define BITMAP_AND(B1, B2, B3, SZ)				\
  __BITMAP_OP(B1, B2, B3, SZ, &)

/* store ~B2 into B1 */
#define BITMAP_NOT(B1, B2, SZ)					\
  { int i; for (i=0; i<(SZ); i++) (B1)[i] = ~((B2)[i]); }

/* return non-zero if bitmap is empty */
#define BITMAP_EMPTY_P(BMAP, SZ)				\
  ({ int i, res=0; for (i=0; i<(SZ); i++) res |= (BMAP)[i]; !res; })

/* return non-zero if the intersection of bitmaps B1 and B2 is non-empty */
#define BITMAP_DISJOINT_P(B1, B2, SZ)				\
  ({ int i, res=0; for (i=0; i<(SZ); i++) res |= (B1)[i] & (B2)[i]; !res; })

/* return non-zero if bit BIT is set in bitmap BMAP */
#define BITMAP_SET_P(BMAP, SZ, BIT)				\
  (((BMAP)[(BIT)/32] & (1 << ((BIT) % 32))) != 0)

/* return non-zero if bit BIT is clear in bitmap BMAP */
#define BITMAP_CLEAR_P(BMAP, SZ, BIT)				\
  (!BMAP_SET_P((BMAP), (SZ), (BIT)))

/* count the number of bits set in BMAP */
#define BITMAP_COUNT_ONES(BMAP, SZ)				\
({								\
  int i, j, n = 0;						\
  for (i = 0; i < (SZ) ; i++)					\
    {								\
      unsigned int word = (BMAP)[i];				\
      for (j=0; j < (sizeof(unsigned int)*8); j++)		\
        {							\
          unsigned int new_val, old_val = word;			\
          word >>= 1;						\
          new_val = word << 1;					\
          if (old_val != new_val)				\
            n++;						\
        }							\
    }								\
  n;								\
})

#endif /* BITMAP_H */

