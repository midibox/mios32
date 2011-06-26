//*****************************************************************************
//
// sound_drv.c
// Copyright (C) 2007 by Nic Birsan & Ionut Tarsa
//
// Driver for sound generation
// Now is using PWM1 for output wave and Timer0 for making samplerate
//*****************************************************************************
#include "sounddrv.h"


volatile unsigned short snd_ix;			// index to current sample generated
volatile unsigned short max_ix;			// index to final sample
volatile unsigned char sound_stopped;	// flag for sound driver state

//internal driver buffer
#define MAXIM_BUF 1024
unsigned char* MCU_out_ptr;
unsigned char* MCU_out_end;
unsigned char MCU_outbuf[MAXIM_BUF];

//=============================================================================
// Sound Handler
// Output samples to PWM1
// if max_ix is reached the generation is stoped
//=============================================================================
void IntSoundHdl(void) {
	unsigned short m_ind = snd_ix;
	//use a register as apointer to SFRs
	unsigned long* p_gen = (unsigned long*) TIMER0_BASE;
	p_gen[((TIMER_O_ICR)/4)] = 	TIMER_TIMA_TIMEOUT;
	if(m_ind != max_ix){
		unsigned long c =  295-MCU_outbuf[m_ind++];
		snd_ix = m_ind & (MAXIM_BUF -1);
		p_gen = (unsigned long*) (PWM_BASE + PWM_GEN_1);
		p_gen[((PWM_O_X_CMPA)/4)] = c;
		p_gen[((PWM_O_X_CMPB)/4)] = c;
	}else{
		//stop generation (disable timer interrupt)
		//HWREG(NVIC_DIS0) =  1 << (INT_TIMER0A - INT_GPIOA);
		sound_stopped = 1;	
	}
}
//*****************************************************************************
// Internal functions
//*****************************************************************************
void
sound_start(void)
{//=========================================================
// internal function for validating sound generation
	snd_ix = 0;
	sound_stopped = 0;
	IntEnable(INT_TIMER0A);
}



//*****************************************************************************
// Public/Interface functions
//*****************************************************************************

//=============================================================================
// sound_init()
// Initialize peripherals needed for generation
// must be called from main 
//=============================================================================
void
sound_init(void)
{
	unsigned long ulPeriod;

	SysCtlPWMClockSet(SYSCTL_PWMDIV_1);
	//
    // Enable the peripherals (PWM, PWM port and Timer0.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	//
    // Make the pin(s) be peripheral controlled
	// (manual for controlling directly strength).
    //
    GPIODirModeSet(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_DIR_MODE_HW);
    // Set the pad(s) for standard push-pull operation.
    //
    GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1,
		GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
    //
    // Compute the PWM period based on the system clock.
    //
	/*ulPeriod = SysCtlClockGet();
	if(ulPeriod < 12300000){
		ulPeriod /=22050;
	}else if(ulPeriod < 24600000){
		ulPeriod /=48000;
	}else{
		ulPeriod /= 96000;
	} */
	ulPeriod = 300;	// direct divisor, independent of SysClck
	//
	//Set PWM mode (only down) and period
	//
    PWMGenConfigure(PWM_BASE, PWM_GEN_1,
                    PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM_BASE, PWM_GEN_1, ulPeriod);

    //
    // Set zero values initially.
    //
    PWMPulseWidthSet(PWM_BASE, PWM_OUT_2, 0/*ulPeriod / 4*/);
    PWMPulseWidthSet(PWM_BASE, PWM_OUT_3, 0/*ulPeriod * 3 / 4*/);

    //
    // Enable the PWM0 and PWM1 output signals.
    //
    PWMOutputState(PWM_BASE, PWM_OUT_2_BIT | PWM_OUT_3_BIT, true);

    //
    // Enable the PWM generator.
    //
    PWMGenEnable(PWM_BASE, PWM_GEN_1);
	PWMGenIntTrigEnable(PWM_BASE, PWM_GEN_1,PWM_INT_CNT_LOAD);
	PWMIntEnable(PWM_BASE, PWM_GEN_1);       //numai pentru intrerupere de la PWM

	//Set SysTick Counter period
	SysTickPeriodSet(SysCtlClockGet()/2);

	//Fineshed Configuration
	//Start SisTick
	SysTickEnable();SysTickIntEnable();
	snd_ix = 0;
	//
	// Timer0 is making the samplerate.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_PER);
	TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet()/8000);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	TimerEnable(TIMER0_BASE, TIMER_A);
	sound_stopped = 1;
}


//=============================================================================
// task_sound_driver()
// The only function that must be scheduled from the main loop in order to have
// the driver alive
//   - schedulling must be done at a rate of 8000/(MAXIM_BUF/2)
//            = 64ms for MAXIM_BUF = 1024
//=============================================================================
void
task_sound_driver(void)
{
	static unsigned short prev_ix=0;
	unsigned short curr_ix = snd_ix & (MAXIM_BUF/2);
	int filled = 0;
	if(sound_stopped){							//if the generation was stopped
		MCU_out_ptr = MCU_outbuf;				//fill the buffer from start
		MCU_out_end = MCU_out_ptr + MAXIM_BUF;	//to the end
		filled = MCU_WavegenFill(1);
		if(-1 == filled) return;				//if nothing to be played return
				//set max_ix:
				// - at the end of the buffer if nothing else to be played
				// - past the buffer if there are more samples waiting
		max_ix = (filled == 0)? (MAXIM_BUF-1):(MAXIM_BUF+1);
		curr_ix = snd_ix = 0;
				//validate generation
		sound_start();
	}else if( (curr_ix != prev_ix) ){
				// driver is playing
				// fill next half buffer only
		MCU_out_ptr = MCU_outbuf + prev_ix;
		MCU_out_end = MCU_out_ptr + (MAXIM_BUF/2);
		filled = MCU_WavegenFill(1);
		if(-1 == filled){
				//nothing to play, stop at the end of current buffer
			max_ix = prev_ix;
			return;
		}//filled means stop at the end
		max_ix = (filled == 0)? (MCU_out_end-MCU_outbuf-1):(MAXIM_BUF+1);
	}else{
				//nothing to do
		filled = 0;
	}
	prev_ix = curr_ix;
}

//=============================================================================
// Function for getting current state of the driver
//=============================================================================
unsigned char
get_sound_state(void){
	return sound_stopped;
}

