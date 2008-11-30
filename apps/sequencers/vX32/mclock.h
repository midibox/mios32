/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
*/


#ifndef _MCLOCK_H
#define _MCLOCK_H


#define ppqn 96
#define minute 240000000
#define qnmpp (minute/ppqn)
#define mclocktimernumber 0

#define max_sclocks 16
#define dead_sclock max_sclocks+1


typedef struct {
	unsigned char status;											// bit 7 is run/stop
	unsigned char ticked;											// Counter to show how many ticks have been triggered

	unsigned char timesigu;											// Upper value of the Master Time Signature, max 16
	unsigned char timesigl;											// Lower value of the Master Time Signature, min 2
	unsigned char res;												// MIDI Clock PPQN
	unsigned char bpm;												// Master BPM

	unsigned int cyclelen;											// Length of master track measured in ticks.
}mclock_t;


typedef struct {													// See mclock.c for explanation
	unsigned char status;											// bit 7 is run/stop
	unsigned char ticked;
	unsigned char ticksent;
	unsigned char patched;
  
	unsigned char steps1;
	unsigned char steps2;
	unsigned int numerator;
  
	unsigned char loops1;
	unsigned char loops2;
	unsigned int denominator;
  
	unsigned int quotient;
	unsigned int modulus;
	unsigned long cyclelen;
  
	unsigned int countmclock;
  
	unsigned int modcounter;
	unsigned int countdown;
}sclock_t;


extern void clocks_init(void);

extern void mclock_init(void);

extern void sclock_init(unsigned char SC, unsigned char steps1, unsigned char steps2, unsigned char loops1, unsigned char loops2);

extern void mclock_setbpm(unsigned char bpm);

extern void mclock_timer(void);

extern void mclock_tick(void);

void SCCDCalc(unsigned char SC);


extern mclock_t mclock;												// Allocate the Master Clock struct

extern sclock_t sclock[max_sclocks];								// Allocate the SubClock Array


#endif /* _MCLOCK_H */
