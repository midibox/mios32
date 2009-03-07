/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


#ifndef _MCLOCK_H
#define _MCLOCK_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#ifndef vxppqn
#define vxppqn 384																// internal clock resolution. should be a multiple of 24
#endif


#ifndef mclocktimernumber
#define mclocktimernumber 0														// used to force a specific timer to be used for master clock. not in use. 
//#define SEQ_BPM_MIOS32_TIMER_NUM mclocktimernumber
#endif



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {
	unsigned char status;														// bit 7 is run/stop
	unsigned int ticked;														// Counter to show how many ticks are waiting to be processed
	
	unsigned char timesigu;														// Upper value of the Master Time Signature, max 16
	unsigned char timesigl;														// Lower value of the Master Time Signature, min 2
	unsigned int res;															// MIDI Clock PPQN
	unsigned char bpm;															// Master BPM
	
	unsigned int cyclelen;														// Length of master track measured in ticks.
}mclock_t;

																				// bits of the status char above
#define MCLOCK_STATUS_RUN 7														// bit 7 is run/stop request
#define MCLOCK_STATUS_RESETREQ 0												// bit 0 is master reset request



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern mclock_t mclock;															// Allocate the Master Clock struct

extern u32 mod_tick_timestamp;													// holds the master clock timestamp



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern void clocks_init(void);

extern void mclock_init(void);

extern void mclock_setbpm(unsigned char bpm);

extern void mclock_tick(void);


extern void mclock_stop(void);

extern void mclock_continue(void);

extern void mclock_reset(void);



#endif /* _MCLOCK_H */
