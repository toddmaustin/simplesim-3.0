/* eventq.c - event queue manager routines */

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
#include "eventq.h"

int eventq_max_events;
int eventq_event_count;
struct eventq_desc *eventq_pending;
struct eventq_desc *eventq_free;

static EVENTQ_ID_TYPE next_ID = 1;

void
eventq_init(int max_events)
{
  eventq_max_events = max_events;
  eventq_event_count = 0;
  eventq_pending = NULL;
  eventq_free = NULL;
}

#define __QUEUE_EVENT(WHEN, ID, ACTION)					\
  struct eventq_desc *prev, *ev, *new;					\
  /* get a free event descriptor */					\
  if (!eventq_free)							\
    {									\
      if (eventq_max_events && eventq_event_count >= eventq_max_events)	\
	panic("too many events");					\
      eventq_free = calloc(1, sizeof(struct eventq_desc));		\
    }									\
  new = eventq_free;							\
  eventq_free = eventq_free->next;					\
  /* plug in event data */						\
  new->when = (WHEN); (ID) = new->id = next_ID++; ACTION;		\
  /* locate insertion point */						\
  for (prev=NULL,ev=eventq_pending;					\
       ev && ev->when < when;						\
       prev=ev, ev=ev->next);						\
  /* insert new record */						\
  if (prev)								\
    {									\
      /* insert middle or end */					\
      new->next = prev->next;						\
      prev->next = new;							\
    }									\
  else									\
    {									\
      /* insert beginning */						\
      new->next = eventq_pending;					\
      eventq_pending = new;						\
    }

EVENTQ_ID_TYPE
eventq_queue_setbit(SS_TIME_TYPE when,
		    BITMAP_ENT_TYPE *bmap, int sz, int bitnum)
{
  EVENTQ_ID_TYPE id;
  __QUEUE_EVENT(when, id,						\
		new->action = EventSetBit; new->data.bit.bmap = bmap;	\
		new->data.bit.sz = sz; new->data.bit.bitnum = bitnum);
  return id;
}

EVENTQ_ID_TYPE
eventq_queue_clearbit(SS_TIME_TYPE when,
		      BITMAP_ENT_TYPE *bmap, int sz, int bitnum)
{
  EVENTQ_ID_TYPE id;
  __QUEUE_EVENT(when, id,						\
		new->action = EventClearBit; new->data.bit.bmap = bmap;	\
		new->data.bit.sz = sz; new->data.bit.bitnum = bitnum);
  return id;
}

EVENTQ_ID_TYPE
eventq_queue_setflag(SS_TIME_TYPE when, int *pflag, int value)
{
  EVENTQ_ID_TYPE id;
  __QUEUE_EVENT(when, id,						\
		new->action = EventSetFlag;				\
		new->data.flag.pflag = pflag; new->data.flag.value = value);
  return id;
}

EVENTQ_ID_TYPE
eventq_queue_addop(SS_TIME_TYPE when, int *summand, int addend)
{
  EVENTQ_ID_TYPE id;
  __QUEUE_EVENT(when, id,						\
		new->action = EventAddOp;				\
		new->data.addop.summand = summand;			\
		new->data.addop.addend = addend);
  return id;
}

EVENTQ_ID_TYPE
eventq_queue_callback(SS_TIME_TYPE when,
		      void (*fn)(SS_TIME_TYPE time, int arg), int arg)
{
  EVENTQ_ID_TYPE id;
  __QUEUE_EVENT(when, id,						\
		new->action = EventCallback; new->data.callback.fn = fn;\
		new->data.callback.arg = arg);
  return id;
}

#define EXECUTE_ACTION(ev, now)						\
  /* execute action */							\
  switch (ev->action) {							\
  case EventSetBit:							\
    BITMAP_SET(ev->data.bit.bmap, ev->data.bit.sz, ev->data.bit.bitnum);\
    break;								\
  case EventClearBit:							\
    BITMAP_CLEAR(ev->data.bit.bmap, ev->data.bit.sz, ev->data.bit.bitnum);\
    break;								\
  case EventSetFlag:							\
    *ev->data.flag.pflag = ev->data.flag.value;				\
    break;								\
  case EventAddOp:							\
    *ev->data.addop.summand += ev->data.addop.addend;			\
    break;								\
  case EventCallback:							\
    (*ev->data.callback.fn)(now, ev->data.callback.arg);		\
    break;								\
  default:								\
    panic("bogus event action");					\
  }

/* execute an event immediately, returns non-zero if the event was
   located an deleted */
int
eventq_execute(EVENTQ_ID_TYPE id)
{
  struct eventq_desc *prev, *ev;

  for (prev=NULL,ev=eventq_pending; ev; prev=ev,ev=ev->next)
    {
      if (ev->id == id)
	{
	  if (prev)
	    {
	      /* middle of end of list */
	      prev->next = ev->next;
	    }
	  else /* !prev */
	    {
	      /* beginning of list */
	      eventq_pending = ev->next;
	    }

	  /* handle action, now is munged */
	  EXECUTE_ACTION(ev, 0);

	  /* put event on free list */
	  ev->next = eventq_free;
	  eventq_free = ev;

	  /* return success */
	  return TRUE;
	}
    }
  /* not found */
  return FALSE;
}

/* remove an event from the eventq, action is never performed, returns
   non-zero if the event was located an deleted */
int
eventq_remove(EVENTQ_ID_TYPE id)
{
  struct eventq_desc *prev, *ev;

  for (prev=NULL,ev=eventq_pending; ev; prev=ev,ev=ev->next)
    {
      if (ev->id == id)
	{
	  if (prev)
	    {
	      /* middle of end of list */
	      prev->next = ev->next;
	    }
	  else /* !prev */
	    {
	      /* beginning of list */
	      eventq_pending = ev->next;
	    }

	  /* put event on free list */
	  ev->next = eventq_free;
	  eventq_free = ev;

	  /* return success */
	  return TRUE;
	}
    }
  /* not found */
  return FALSE;
}

void
eventq_service_events(SS_TIME_TYPE now)
{
  while (eventq_pending && eventq_pending->when <= now)
    {
      struct eventq_desc *ev = eventq_pending;

      /* handle action */
      EXECUTE_ACTION(ev, now);

      /* return the event record to the free list */
      eventq_pending = ev->next;
      ev->next = eventq_free;
      eventq_free = ev;
  }
}

void
eventq_dump(FILE *stream)
{
  struct eventq_desc *ev;

  if (!stream)
    stream = stderr;

  fprintf(stream, "Pending Events: ");
  for (ev=eventq_pending; ev; ev=ev->next)
    {
      fprintf(stream, "@ %.0f:%s:",
	      (double)ev->when,
	      ev->action == EventSetBit ? "set bit"
	      : ev->action == EventClearBit ? "clear bit"
	      : ev->action == EventSetFlag ? "set flag"
	      : ev->action == EventAddOp ? "add operation"
	      : ev->action == EventCallback ? "call back"
	      : (abort(), ""));
      switch (ev->action) {
      case EventSetBit:
      case EventClearBit:
	fprintf(stream, "0x%p, %d, %d",
		ev->data.bit.bmap, ev->data.bit.sz, ev->data.bit.bitnum);
	break;
      case EventSetFlag:
	fprintf(stream, "0x%p, %d", ev->data.flag.pflag, ev->data.flag.value);
	break;
      case EventAddOp:
	fprintf(stream, "0x%p, %d",
		ev->data.addop.summand, ev->data.addop.addend);
	break;
      case EventCallback:
	fprintf(stream, "0x%p, %d",
		ev->data.callback.fn, ev->data.callback.arg);
	break;
      default:
	panic("bogus event action");
      }
      fprintf(stream, " ");
    }
}
