/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
*/


/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "app.h"

#include "graph.h"
#include "mclock.h"

#include <FreeRTOS.h>
#include <portmacro.h>


mclock_t mclock;													// Allocate the Master Clock struct

sclock_t sclock[max_sclocks];										// Allocate the SubClock Array

void clocks_init(void) {
	unsigned char i;
	
	mclock.status = 0;												// bit 7 is run/stop
	mclock.ticked = 0;
	mclock.timesigu = 4;
	mclock.timesigl = 4;											// Lower value of the Master Time Signature, min 2
	mclock.res = ppqn;
	mclock_setbpm(90);												// Master BPM
	
	mclock_init();
	
	for (i = 0; i < max_sclocks; i++) {								// i is the number of SubClocks
		sclock[i].status = 0;										// bit 7 is run/stop
		sclock[i].ticked = 0;
		sclock[i].countmclock = 0;
				
		sclock_init(i, 16, 1, 1, 1);
	}	
}

void mclock_init(void) {
    mclock.cyclelen = (((mclock.res << 2)*mclock.timesigu)/mclock.timesigl);		// Length of master track measured in ticks.
}


void sclock_init(unsigned char SC, unsigned char steps1, unsigned char steps2, unsigned char loops1, unsigned char loops2) {
    /*
    - SubClocks -
    Each SubClock has two pairs of parameters, presented to the user as two fractions in the form of Steps/Loops.
    (IE, the subclock will tick (steps) times per every (loops) cycles of the master clock

    When one of the Steps per Loop values is changed either
		The two Steps values are multiplied to produce the SubClock Numerator
	or
		The two Loops values are multiplied to produce the SubClock Denominator (effect is similar to a clock divider)
	*/
	
	if (SC < dead_sclock) {
		sclock[SC].steps1 = steps1;
		sclock[SC].loops1 = loops1;
		sclock[SC].steps2 = steps2;
		sclock[SC].loops2 = loops2;
		

		sclock[(SC)].numerator = sclock[(SC)].steps1 * sclock[(SC)].steps2;
		sclock[(SC)].denominator = sclock[(SC)].loops1 * sclock[(SC)].loops2;			// And then the SubClock Initialisation is run:

		
		sclock[(SC)].cyclelen = mclock.cyclelen * sclock[(SC)].denominator;				// The Master Clock Cycle Length is multiplied by the SubClock Denominator to obtain the SubClock Cycle Length
		sclock[(SC)].quotient = sclock[(SC)].cyclelen / sclock[(SC)].numerator;			// The SubClock Cycle Length is then divided by the SubClock Numerator
		sclock[(SC)].modulus = sclock[(SC)].cyclelen % sclock[(SC)].numerator;			// to produce the SubClock Quotient and SubClock Modulus (often called a ‘remainder’)
		sclock[(SC)].modcounter = sclock[(SC)].modulus;									// The SubClock Modulus Counter is set to equal the SubClock Modulus
		
		sclock[(SC)].ticked = 0;
		sclock[(SC)].ticksent = 0;		
		sclock[(SC)].patched = 0;
	}
	
}


void mclock_setbpm(unsigned char bpm) {
	u32 period;
	
	mclock.bpm = bpm;												// store the tempo
	period = qnmpp/bpm;												// period ==
	if (MIOS32_TIMER_Init(mclocktimernumber, period, mclock_timer, MIOS32_IRQ_PRIO_MID) < 0) {
		mclock.status = 0;											// handle failure by disabling master clock
	}
	
}


void mclock_timer(void) {
    if (mclock.status) (mclock.ticked)++;							// Just increment the master tick counter
}



void mclock_tick(void) {
	unsigned char i;
	if (mclock.status) {
	    if (mclock.ticked) {
			MIOS32_IRQ_Disable();									// port specific FreeRTOS function to disable IRQs (nested)
			--(mclock.ticked);
			MIOS32_IRQ_Enable();									// port specific FreeRTOS function to disable IRQs (nested)
			// Send 0xF8
			
			for (i = 0; i < max_sclocks; i++) {						// i is the number of SubClocks
				if (sclock[(i)].status > 0x7F) {					// only do this if the SubClock is active
					if ((++(sclock[(i)].countmclock)) > (sclock[(i)].countdown)){	// increment the master tick counter
						(sclock[(i)].ticked)++;						// increment the subclock tick counter
						sclock[(i)].countmclock = sclock[(i)].countmclock - sclock[(i)].countdown;
						SCCDCalc(i);								// calculate ticks till next step
						
					}
					
				}
				
			}
			
		}
		
	}
	
}




/////////////////////////////////////////////////////////////////////////////
/*
 - SubClock Countdown Calculation Sequence –
This calculates how many master clock ticks until the next SubClock tick. It is called as required
(When a SubClock has been patched to it’s assigned tracks and the tick is cleared, and we want to calculate
how long until the next tick). This does not run over all SubClocks, just one, which must be specified. SubClock number is input.
*/
/////////////////////////////////////////////////////////////////////////////

void SCCDCalc(unsigned char SC) {

    sclock[(SC)].countdown = sclock[(SC)].quotient;
	
    if (sclock[(SC)].modulus) {										// only do this if there is a remainder
    	sclock[(SC)].modcounter = sclock[(SC)].modcounter + sclock[(SC)].modulus;		// The SubClock Modulus is added to the SubClock Modulus Counter
	    if (sclock[(SC)].modcounter > sclock[(SC)].modulus) {		// If the SubClock Modulus Counter is greater than SubClock Modulus then
            (sclock[(SC)].countdown)++;							// The SubClock Countdown is incremented by 1 (this is the distribution of the modulus)
	        sclock[(SC)].modcounter = sclock[(SC)].modcounter - sclock[(SC)].numerator;	// The SubClock Numerator is subtracted from SubClock Modulus Counter (this controls how often the modulus is distributed)
			
	    }
		
    }
	
}


// todo

// fix sclock inits for new structure