/* Copyright (c) 2013-2016 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "timing.h"

DEFINE_VECTOR(mTimingEventList, struct mTimingEvent*);

void mTimingInit(struct mTiming* timing, int32_t* relativeCycles, int32_t* nextEvent) {
	mTimingEventListInit(&timing->events, 0);
	timing->masterCycles = 0;
	timing->relativeCycles = relativeCycles;
	timing->nextEvent = nextEvent;
}

void mTimingDeinit(struct mTiming* timing) {
	mTimingEventListDeinit(&timing->events);
}

void mTimingClear(struct mTiming* timing) {
	mTimingEventListClear(&timing->events);
	timing->masterCycles = 0;
}

void mTimingSchedule(struct mTiming* timing, struct mTimingEvent* event, int32_t when) {
	int32_t nextEvent = when + *timing->relativeCycles;
	event->when = nextEvent + timing->masterCycles;
	if (nextEvent < *timing->nextEvent) {
		*timing->nextEvent = nextEvent;
	}
	size_t e;
	for (e = 0; e < mTimingEventListSize(&timing->events); ++e) {
		struct mTimingEvent* next = *mTimingEventListGetPointer(&timing->events, e);
		int32_t nextWhen = next->when - timing->masterCycles;
		if (nextWhen > when) {
			mTimingEventListUnshift(&timing->events, e, 1);
			*mTimingEventListGetPointer(&timing->events, e) = event;
			return;
		}
	}
	*mTimingEventListAppend(&timing->events) = event;
}

void mTimingDeschedule(struct mTiming* timing, struct mTimingEvent* event) {
	size_t e;
	for (e = 0; e < mTimingEventListSize(&timing->events); ++e) {
		struct mTimingEvent* next = *mTimingEventListGetPointer(&timing->events, e);
		if (next == event) {
			mTimingEventListShift(&timing->events, e, 1);
			return;
		}
	}
}

void mTimingTick(struct mTiming* timing, int32_t cycles) {
	timing->masterCycles += cycles;
	while (mTimingEventListSize(&timing->events)) {
		struct mTimingEvent* next = *mTimingEventListGetPointer(&timing->events, 0);
		int32_t nextWhen = next->when - timing->masterCycles;
		if (nextWhen > 0) {
			*timing->nextEvent = nextWhen;
			return;
		}
		mTimingEventListShift(&timing->events, 0, 1);
		next->callback(timing, next->context, -nextWhen);
	}
}

int32_t mTimingNextEvent(struct mTiming* timing) {
	if (!mTimingEventListSize(&timing->events)) {
		return INT_MAX;
	}
	struct mTimingEvent* next = *mTimingEventListGetPointer(&timing->events, 0);
	return next->when - timing->masterCycles;
}