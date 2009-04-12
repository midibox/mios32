/*
	FreeRTOS Emu
*/

#ifndef JUCETIMER_H
#define JUCETIMER_H

#define NUM_TIMERS (unsigned char)20

#ifdef __cplusplus
extern "C" {
#endif

extern long JUCE_TIMER_Init(unsigned char timer, long period, void (*_irq_handler)());

extern long JUCE_TIMER_ReInit(unsigned char timer, long period);


#ifdef __cplusplus
}
#endif


#endif /* JUCETIMER_H */

