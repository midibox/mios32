//*****************************************************************************
//
// sounddrv.h 
// Copyright (C) 2007 by Nic Birsan & Ionut Tarsa
//
// Functions for making sounds on LM3S811 microcontrollers
//*****************************************************************************

#ifndef __SOUND_DRV_H__
#define __SOUND_DRV_H__

//#include "hw_memmap.h"
//#include "hw_types.h"
//#include "debug.h"
//#include "sysctl.h"
//#include "gpio.h"
//#include "pwm.h"
//#include "hw_pwm.h"
//#include "sysctl.h"
//#include "systick.h"
//#include "interrupt.h"
//#include "hw_ints.h"
//#include "timer.h"
//#include "hw_timer.h"
//#include "hw_nvic.h"
//#include "../diag.h"
//#include "../osram96x16.h"

void sound_init(void);
//void sound_start(void);
void PlayWave(unsigned char* wavedata, unsigned long sample_no);
void task_sound_driver(void);
unsigned char get_sound_state(void);

//function that must be declared in the application
int MCU_WavegenFill(int fill_zeros);
extern unsigned char* MCU_out_ptr;
extern unsigned char* MCU_out_end;

#endif //__SOUND_DRV_H__
