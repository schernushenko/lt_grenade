/*****************************************************

*****************************************************/


#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#include "globals.h"

//итого 16-ти битный таймер с точностью 0,016сек.
struct timer {
	uchar active;
	uint start_time;
	
	uint delta;
	uint limit;
	void (*fn_done)();
};

void process_timer(struct timer *timer);
void start_timer(struct timer *timer, uint _limit, void(*fn)());
void stop_timer(struct timer *timer);


#endif
