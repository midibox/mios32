/*
 *  ACMidi.h
 *  m5_sensorizer
 *
 *  Created by audiocommander on 29.07.05.
 *  Adapted to MIOS on 21.05.06.
 *  Copyright 2006 Michael Markert, http://www.audiocommander.de
 *
 */

/*	
 * Released under GNU General Public License
 * http://www.gnu.org/licenses/gpl.html
 * 
 * This program is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation
 *
 * YOU ARE ALLOWED TO COPY AND CHANGE 
 * BUT YOU MUST RELEASE THE SOURCE TOO (UNDER GNU GPL) IF YOU RELEASE YOUR PRODUCT 
 * YOU ARE NOT ALLOWED TO USE IT WITHIN PROPRIETARY CLOSED-SOURCE PROJECTS
 */

#ifndef _ACMIDI_H
#define _ACMIDI_H


#include "ACMidiDefines.h"





// ***** DEFINES *****

// global MIDI channel
#define MIDI_GLOBAL_CH				0	// application wide Midi-Channel: 0-15




// ***** PROTOTYPES *****

// send Messages
extern void ACMidi_SendEvent(unsigned char evnt0, unsigned char evnt1, unsigned char evnt2);
extern void ACMidi_SendNote(unsigned char channel, unsigned char note, unsigned char velo);
extern void ACMidi_SendCC(unsigned char channel, unsigned char cc, unsigned char sevenBitValue);
extern void ACMidi_SendPRG_CH(unsigned char bank, unsigned char program);

// send PANIC! If channel > 15 send panic on all channels (self recursive)
extern void ACMidi_SendPanic(unsigned char channel);


#endif	/* _ACMIDI_H	*/
