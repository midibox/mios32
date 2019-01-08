/*
	FreeRTOS Emu
*/


#include <mios32.h>

#include <JUCEtimer.h>



s32 MIOS32_TIMER_Init(u8 timer, u32 period, void *_irq_handler, u8 irq_priority) {
	// priority is lost in the emulation
	return JUCE_TIMER_Init(timer, period, (void (*)())_irq_handler);
}

s32 MIOS32_TIMER_ReInit(u8 timer, u32 period) {
	return JUCE_TIMER_ReInit((char)timer, (long)period);
}




