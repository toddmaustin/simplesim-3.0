/* eventq.h - event queue manager interfaces */

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


#ifndef EVENTQ_H
#define EVENTQ_H

#include <stdio.h>

#include "host.h"
#include "misc.h"
#include "bitmap.h"

/* This module implements a time ordered event queue.  Users insert
 *
 */

/* event actions */
enum eventq_action {
  EventSetBit,			/* set a bit: int *, bit # */
  EventClearBit,		/* clear a bit: int *, bit # */
  EventSetFlag,			/* set a flag: int *, value */
  EventAddOp,			/* add a value to a summand */
  EventCallback,		/* call event handler: fn * */
};

/* ID zero (0) is unused */
typedef unsigned int EVENTQ_ID_TYPE;

/* event descriptor */
struct eventq_desc {
  struct eventq_desc *next;	/* next event in the sorted list */
  SS_TIME_TYPE when;		/* time to schedule write back event */
  EVENTQ_ID_TYPE id;		/* unique event ID */
  enum eventq_action action;	/* action on event occurrance */
  union eventq_data {
    struct {
      BITMAP_ENT_TYPE *bmap;	/* bitmap to access */
      int sz;			/* bitmap size */
      int bitnum;		/* bit to set */
    } bit;
    struct {
      int *pflag;		/* flag to set */
      int value;
    } flag;
    struct {
      int *summand;		/* value to add to */
      int addend;		/* amount to add */
    } addop;
    struct {
      void (*fn)(SS_TIME_TYPE time, int arg);	/* function to call */
      int arg;			/* argument to pass */
    } callback;
  } data;
};

/* initialize the event queue module, MAX_EVENT is the most events allowed
   pending, pass a zero if there is no limit */
void eventq_init(int max_events);

/* schedule an action that occurs at WHEN, action is visible at WHEN,
   and invisible before WHEN */
EVENTQ_ID_TYPE eventq_queue_setbit(SS_TIME_TYPE when,
				   BITMAP_ENT_TYPE *bmap, int sz, int bitnum);
EVENTQ_ID_TYPE eventq_queue_clearbit(SS_TIME_TYPE when, BITMAP_ENT_TYPE *bmap,
				     int sz, int bitnum);
EVENTQ_ID_TYPE eventq_queue_setflag(SS_TIME_TYPE when,
				    int *pflag, int value);
EVENTQ_ID_TYPE eventq_queue_addop(SS_TIME_TYPE when,
				  int *summand, int addend);
EVENTQ_ID_TYPE eventq_queue_callback(SS_TIME_TYPE when,
				     void (*fn)(SS_TIME_TYPE time, int arg),
				     int arg);

/* execute an event immediately, returns non-zero if the event was
   located an deleted */
int eventq_execute(EVENTQ_ID_TYPE id);

/* remove an event from the eventq, action is never performed, returns
   non-zero if the event was located an deleted */
int eventq_remove(EVENTQ_ID_TYPE id);

/* service all events in order of occurrance until and at NOW */
void eventq_service_events(SS_TIME_TYPE now);

void eventq_dump(FILE *stream);

#endif /* EVENT_H */
