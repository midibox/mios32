/*
	FreeRTOS Emu
*/


#include <juce.h>

#include <mios32.h>

#include <JUCEtimer.h>
#include <JUCEtimer.hpp>


#define dummyreturn 0 // just something to keep the compiler quiet for now

JUCETimer *timerarray[NUM_TIMERS];

long JUCE_TIMER_Init(unsigned char timer, long period, void (*_irq_handler)())
{
	if (timer >= NUM_TIMERS) return -1;
	if (timerarray[timer] != NULL) 
	{
		delete timerarray[timer];
	}
	timerarray[timer] = new JUCETimer(_irq_handler);
	timerarray[timer]-> startTimer(period/1000);
	return 0;
}

long JUCE_TIMER_ReInit(unsigned char timer, long period)
{
	if (timerarray[timer] != NULL)
	{
		timerarray[timer]-> startTimer(period/1000);
		return 0;
	}
	else
	{
		return -1;
	}
}

