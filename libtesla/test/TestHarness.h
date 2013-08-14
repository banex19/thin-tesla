/*-
 * Copyright (c) 2013 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <memory>
#include <queue>

#include <libtesla.h>
#include <assert.h>

#ifndef NULL
#define NULL 0
#endif

typedef struct tesla_class Class;
typedef struct tesla_instance Instance;
typedef struct tesla_key Key;
typedef struct tesla_transition Trans;
typedef struct tesla_transitions TransSet;


enum EventType {
	NewInstance,
	Transition,
	Clone,
	NoInstance,
	BadTransition,
	Err,
	Accept,
	Ignored,
	Invalid
};

class Event;

class LibTeslaTest
{
public:
	LibTeslaTest();
	void Ev(Event*);

protected:
	std::queue<EventType> expectedEvents;
	std::auto_ptr<Event> lastEvent;
};

LibTeslaTest *Test = NULL;


class Event {
public:
	static void NewInstanceEvent(Class *c, Instance *i)
	{
		Test->Ev(new Event(NewInstance, c, i));
	}

	static void TransitionEvent(Class *c, Instance *i, const Trans *t)
	{
		Test->Ev(new Event(Transition, c, i, NULL, NULL, t));
	}

	static void CloneEvent(Class *c, Instance *orig, Instance *copy,
	                       const Trans *t)
	{
		Test->Ev(new Event(Clone, c, orig, copy, NULL, t));
	}

	static void NoInstanceEvent(Class *c, const Key *k, const TransSet *t)
	{
		Test->Ev(new Event(NoInstance, c, NULL, NULL, k, NULL, t));
	}

	static void BadTransitionEvent(Class *c, Instance *i, const TransSet *t)
	{
		Test->Ev(new Event(BadTransition, c, i, NULL, NULL, NULL, t));
	}

	static void ErrEvent(Class *c, int32_t code, const char *message)
	{
		Test->Ev(new Event(Err, c, NULL, NULL,
				NULL, NULL, NULL, code, message));
	}

	static void AcceptEvent(Class *c, Instance *i)
	{
		Test->Ev(new Event(Accept, c, i));
	}

	static void IgnoredEvent(const Class *c, const Key *k,
	                         const TransSet *t)
	{
		Test->Ev(new Event(Ignored, c, NULL, NULL, k, NULL, t));
	}

	static struct tesla_event_handlers handlers;

	const EventType type;
	const Class *cls;
	const Instance *inst;
	const Instance *copy;
	const Key *key;
	const Trans *transition;
	const TransSet *transitions;
	const int32_t err;
	const char *errMessage;

	Event(const EventType type, const Class *cls,
	      const Instance *inst = NULL, const Instance *copy = NULL,
	      const Key *key = NULL, const Trans *transition = NULL,
	      const TransSet *transitions = NULL,
	      const int32_t err = -1, const char *errMessage = NULL)
		: type(type), cls(cls), inst(inst), copy(copy), key(key),
		  transition(transition), transitions(transitions),
		  err(err), errMessage(errMessage)
	{
	}
};


struct tesla_event_handlers Event::handlers = {
	.teh_init		= NewInstanceEvent,
	.teh_transition		= TransitionEvent,
	.teh_clone		= CloneEvent,
	.teh_fail_no_instance	= NoInstanceEvent,
	.teh_bad_transition	= BadTransitionEvent,
	.teh_err		= ErrEvent,
	.teh_accept		= AcceptEvent,
	.teh_ignored		= IgnoredEvent,
};


LibTeslaTest::LibTeslaTest()
{
	Test = this;
	assert(tesla_set_event_handler(&Event::handlers) == TESLA_SUCCESS);
}

void LibTeslaTest::Ev(Event *e)
{
	assert(!expectedEvents.empty());
	assert(e->type == expectedEvents.front());
	expectedEvents.pop();

	lastEvent.reset(e);
}

#endif
