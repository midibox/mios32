/*
 *  ACHarmonizer.c
 *  ACSensorizer
 *
 */

#include "ACHarmonizer.h"

#include <mios32.h>
#include "app.h"


#if HARMONIZER_ENABLED


// ***** GLOBALS *****

volatile unsigned char	harmonizer_base;	// 0 = C .. 11 = H
volatile unsigned char	harmonizer_scale;	// 0 .. SCALEMAX

volatile unsigned char	harmonizer_listen;	// listen for next base note

const unsigned char	noteNames[2][12] = {
	{ 'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B' },
	{ ' ', '#', ' ', '#', ' ', ' ', '#', ' ', '#', ' ', '#', ' ' }
};

const unsigned char	scaleNames[SCALE_MAX][5] = {
	{ "----" }, // not harmonized		0
	{ "MAJ " },	// Major				1
	{ "min " },	// Natural Minor		2
	{ "harm" },	// Harmonic Minor		3
	{ "blue" },	// Blues Minor			4
	{ "BLUE" },	// Blues Major			5
	{ "DOR " },	// Dorian				6
	{ "JAP " },	// Japanese				7
	{ "JAPd" },	// Japanese Diminshed	8
	{ "KUMO" },	// Kumoi				9
	{ "LOC " },	// Locrian				10
	{ "LYD " },	// Lydian				11
	{ "MISH" },	// Mi-Sheberach			12
	{ "MIX " },	// Mixolydian			13
	{ "PLOG" },	// Pelog				14
	{ "PENT" },	// Pentatonic Neutral	15
	{ "PBLU" },	// Pentatonic Blues		16
	{ "PMAJ" },	// Pentatonic Major		17
	{ "Pmin" },	// Pentatonic Minor		18
	{ "PHRY" },	// Phrygian				19
	{ "SPAN" }	// Spanish				20
};



// ***** LOCALS *****

const unsigned char scale_set[SCALE_MAX][12] = {
	//	C	C#	D	D#	E	F	F#	G	G#	A	A#	B
	{	0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11	},	// not harmonized
	{	0,	0,	2,	4,	4,	5,	5,	7,	9,	9,	11,	11	},	// Major
	{	0,	0,	2,	3,	3,	5,	5,	7,	8,	8,	10,	10	},	// Natural Minor = Aeolian
	{	0,	0,	2,	3,	3,	5,	5,	7,	8,	8,	11,	11	},	// Harmonic Minor
	{	0,	0,	3,	3,	5,	5,	6,	7,	7,	10,	10,	10	},	// Blues Minor
	{	0,	0,	3,	3,	4,	4,	7,	7,	9,	9,	10,	10	},	// Blues Major
	{	0,	2,	2,	3,	3,	5,	5,	7,	7,	9,	10,	10	},	// Dorian
	{	0,	0,	2,	2,	5,	5,	5,	7,	7,	7,	10,	10	},	// Japanese
	{	0,	1,	1,	1,	5,	5,	5,	7,	7,	7,	10,	10	},	// Japanese Diminished
	{	0,	2,	2,	3,	3,	3,	7,	7,	7,	9,	9,	9	},	// Kumoi
	{	0,	0,	2,	2,	4,	4,	6,	7,	7,	9,	9,	11	},	// Lydian
	{	0,	1,	1,	3,	3,	5,	6,	6,	8,	8,	10,	10	},	// Locrian
	{	0,	0,	2,	3,	3,	6,	6,	7,	7,	9,	10,	10	},	// Mi-Sheberach (Jewish)
	{	0,	0,	2,	2,	4,	5,	5,	7,	7,	9,	10,	10	},	// Mixolydian
	{	0,	1,	1,	3,	3,	3,	7,	7,	8,	8,	8,	8	},	// Pelog
	{	0,	0,	2,	2,	2,	5,	5,	7,	7,	7,	10,	10	},	// Pentatonic Neutral
	{	0,	0,	3,	3,	5,	5,	6,	7,	7,	7,	10,	10	},	// Pentatonic Blues
	{	0,	0,	2,	2,	4,	4,	7,	7,	7,	9,	9,	9	},	// Pentatonic Major
	{	0,	0,	3,	3,	5,	5,	5,	7,	7,	7,	10,	10	},	// Pentatonic Minor
	{	0,	1,	1,	3,	3,	5,	5,	7,	8,	8,	10,	10	},	// Phrygian
	{	0,	1,	1,	4,	4,	5,	5,	7,	8,	8,	10,	10	}	// Spanish
	//	C	Db	D	Eb	E	F	Gb	G	Ab	A	Bb	B
};




// ***** FUNCTIONS *****

void ACHarmonizer_Init(void) {
	// initialize harmonizer
	harmonizer_base = BASE_DEFAULT;
	harmonizer_scale = SCALE_DEFAULT;
	harmonizer_listen = FALSE;
}


void ACHarmonizer_SetBase(unsigned char value) {
	// sets new base note
	unsigned char c;
	for(c=0;c<127;c++) {
		if(value < 12) { 
			harmonizer_base = value;
			return;
		} else {
			value = value - 12;
		}
	}
}

void ACHarmonizer_SetScale(unsigned char value) {
	// select current scale
	if(value > SCALE_MAX) {
		// switch to next scale
		ACHarmonizer_ToggleScale(1);
	} else {
		harmonizer_scale = value;
	}
}

void ACHarmonizer_ToggleScale(unsigned char direction) {
	// 0:down, 1:up
	if(direction) {
		// switch to next scale
		harmonizer_scale++;
		if(harmonizer_scale > (SCALE_MAX - 1)) {
			harmonizer_scale = 0;
		}
	} else {
		// switch to previous scale
		if(harmonizer_scale == 0) {
			harmonizer_scale = (SCALE_MAX - 1);
		} else {
			harmonizer_scale--;
		}
	}
}

void ACHarmonizer_Random(void) {
	// set new random base & scale
	unsigned char rb;
	unsigned char rs;
	rb = ACMath_RandomInRange(11);
	rs = ACMath_RandomInRange((SCALE_MAX - 1));
	ACHarmonizer_SetBase(rb);
	ACHarmonizer_SetScale(rs);
}

unsigned char ACHarmonize(unsigned char value) {
	// 0..127 in, returns harmonized value
	unsigned char c;
	unsigned char note_interval = 0;
	unsigned char note_num = value - harmonizer_base;
	unsigned char harmonized = value;
	// make life easier
	if(harmonizer_scale == 0) return value;
	// get note number from value
	for(c=0;c<=value;c++) {
		if(note_num < 12) {
			// got value! now harmonize:
			harmonized = scale_set[harmonizer_scale][note_num];
			harmonized = harmonized + note_interval;
			break;
		} else {
			note_num = note_num - 12;
			note_interval = note_interval + 12;
		}
	}
	return harmonized + harmonizer_base;
}


#endif /* HARMONIZER_ENABLED */


