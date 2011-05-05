/*
 *  ACHarmonizer.h
 *  ACSensorizer
 *
 *  Created by audiocommander on 01.12.06.
 *  Copyright 2006 Michael Markert, http://www.audiocommander.de
 *
 *  v0.0.1	01.12.06	first version
 *	v0.0.2	11.08.07	added base note support
 *
 *
 *	description:
 *		notes get harmonized to next possible harmonic notes
 *		a base-note and a scale set are used
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


#ifndef _ACHARMONIZER_H
#define _ACHARMONIZER_H

#include "ACMidiDefines.h"									// for Base-Notes
#include "ACToolbox.h"										// for Random



// ***** DEFINES *****

#define HARMONIZER_ENABLED			1

#define SCALE_MAX					21						// number of available scales
#define SCALE_DEFAULT				20						// 0 for not harmonized, default: 20 (spanish)
#define BASE_DEFAULT				MIDI_NOTE_H				// MIDI_NOTE_x, default: MIDI_NOTE_H


#if HARMONIZER_ENABLED

// ***** GLOBALS *****

extern volatile unsigned char	harmonizer_base;			// 0 = C .. 11 = H
extern volatile unsigned char	harmonizer_scale;			// 0 .. SCALEMAX

extern volatile unsigned char	harmonizer_listen;			// listen for next base note

extern const unsigned char		noteNames[2][12];			// note to name table
extern const unsigned char		scaleNames[SCALE_MAX][5];	// scale name table


// ***** FUNCTIONS *****

extern void ACHarmonizer_Init(void);

extern void ACHarmonizer_SetBase(unsigned char value);			// sets new base note, 0..127
extern void ACHarmonizer_SetScale(unsigned char value);		// 0..SCALE_MAX:scale# or >SCALE_MAX:next
extern void ACHarmonizer_ToggleScale(unsigned char direction);	// 0:down, 1:up
extern void ACHarmonizer_Random(void);									// set new random base & scale

extern unsigned char ACHarmonize(unsigned char value);			// 0..127 in, returns harmonized value

#endif /* HARMONIZER_ENABLED */


#endif /* _ACHARMONIZER_H */
