/*
 * ACSyncronizer
 * MIDI clock module definitions
 *
 * ==========================================================================
 *
 * Copyright (C) 2005  Thorsten Klose (tk@midibox.org)
 * Addional Code Copyright (C) 2007  Michael Markert, http://www.audiocommander.de
 * 
 * ==========================================================================
 * 
 * This file is part of a MIOS application
 *
 * This application is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This application is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with This application; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ==========================================================================
 */

#ifndef _ACSYNC_H
#define _ACSYNC_H

#include "ACMidiDefines.h"
#include "ACHandTracker.h"
#include "IIC_SpeakJet.h"


// Defines
#define ACSYNC_GARBAGE_SERVICE		0		// if a reset should be done each ACSYNC_GARBAGE_SERVICE_CTR
#define ACSYNC_GARBAGE_SERVICE_CTR	23		// each x bars a reset is done, default: 23 (1..23)
#define ACSYNC_GARBAGE_SERVICE_HARDRESET 1	// if a hard- or soft-reset of the SJ is done, default: 1

// better not change these
#define HANDTRACKER_POLL_AIN		0		// if AIN should be polled by timer
#define MASTER_IS_ALIVE_COUNTER		4095	// detect timeout at MIDI_CLOCK receive
#define CTR_MEASURE_MAX				32		// reset counters & SJ, startup voice if inactive, default: 32
#define HANDTRACKER_QUANT			32		// currently implemented: 4, 8, 16, 32

// don't change these
#define ACSYNC_QUANT_4				0
#define ACSYNC_QUANT_8				1
#define ACSYNC_QUANT_16				2
#define ACSYNC_QUANT_32				3
#define ACSYNC_QUANT_MAX			ACSYNC_QUANT_32


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

// status of midi clock
typedef union {
  struct {
    unsigned ALL			:8;
  };
  struct {
    unsigned RUN			:1;				// in run mode
    unsigned PAUSE			:1;				// in pause mode
    unsigned START_REQ		:1;				// start request
    unsigned STOP_REQ		:1;				// stop request
    unsigned CONT_REQ		:1;				// continue request
	unsigned free			:2;				// unassigned
	unsigned IS_MASTER		:1;				// in master mode
  };
} mclock_state_t;

// status of handTracker
typedef union {
  struct {
    unsigned ALL			:8;
  };
  struct {
	unsigned TRIGGER_LOCK	:1;				// lock triggering, eg. for startup sounds
    unsigned CLOCK_REQ		:1;				// clock request
	unsigned UPDATE_REQ		:1;				// update gesture
	unsigned MESSAGE_REQ	:1;				// startup message request
	unsigned RESET_REQ		:1;				// reset request (called frequently as repair service)
	unsigned RESET_DONE		:1;				// used to create a delay between hard-reset & starup msg
	unsigned free			:2;				// unassigned
  };
} mclock_matrix_state_t;



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern mclock_state_t			mclock_state;
extern mclock_matrix_state_t	mclock_matrix_state;

extern unsigned char			mclock_ctr_24;
extern unsigned char			mclock_ctr_beats;
extern unsigned int				mclock_ctr_measures;

extern volatile unsigned char	quantize;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

void ACSYNC_Init(void);
void ACSYNC_Tick(void);
void ACSYNC_Timer(void);

void ACSYNC_ReceiveClock(unsigned char byte);

void ACSYNC_BPMSet(unsigned char _bpm);
unsigned char ACSYNC_BPMGet(void);

void ACSYNC_ResetMeter(void);
void ACSYNC_SendMeter(void);

void ACSYNC_DoStop(void);
void ACSYNC_DoPause(void);
void ACSYNC_DoRun(void);
void ACSYNC_DoRew(void);
void ACSYNC_DoFwd(void);

unsigned int ACSYNC_GetTimerValue(unsigned char bpm);

void ACSYNC_ToggleQuantize(void);


#endif /* _ACSYNC_H */
