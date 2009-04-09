/****************************************************************************
 * nI2S Digital Toy Synth - FILTER MODULE                                   *
 *                                                                          *
 * Very simple approach w/o any oversampling, anti-aliasing, ...            *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *  Copyright (C) 2009 nILS Podewski (nils@podewski.de)                     *
 *                                                                          *
 *  Licensed for personal non-commercial use only.                          *
 *  All other rights reserved.                                              *
 *                                                                          *
 ****************************************************************************/
 
/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <math.h>

#include "engine.h"
#include "defs.h"
#include "filter.h"

/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////

static s32 buf0, buf1;
static u32 fb;

/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

u16 FILTER_resonantLP(u16 in, u16 cutoff);
u16 FILTER_simpleLP(u16 in, u16 cutoff);
u16 FILTER_moogLP(u16 in, u16 resonance, u16 cutoff);
s16 FILTER_bandpass(s16 in);

void FILTER_resonantLP_Init();
void FILTER_bandpass_Init();

/////////////////////////////////////////////////////////////////////////////
// inits the feedback for the resonant lowpass filter
/////////////////////////////////////////////////////////////////////////////
void FILTER_resonantLP_Init() {
	u16 c;
	u16 a;

	c = (filter.cutoff == 0) ? 1 : filter.cutoff;
	
	// fb = q + q/(1.0 - f);
	a = filter.resonance;
	// b = (32768 - cutoff >> 1);
	a /= c;
	fb = filter.resonance + a;
}

void FILTER_initFilter() {
	switch (filter.filterType) {
		case (FILTER_MOOG_LP):
			FILTER_resonantLP_Init();
			break;
		case (FILTER_BANDPASS):
			FILTER_bandpass_Init();
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// returns the filtered input depending on selected filter type
/////////////////////////////////////////////////////////////////////////////
u16 FILTER_filter(u16 in, u16 cutoff) {
	switch (filter.filterType) {
		case FILTER_LP:
			return FILTER_simpleLP(in, cutoff);
		case FILTER_RES_LP:
			return FILTER_resonantLP(in, cutoff);
		case FILTER_MOOG_LP:
			return FILTER_moogLP(in, filter.resonance, cutoff);
		case FILTER_BANDPASS:
			return FILTER_bandpass(in);
		default: 
			return in;
	}
}

/////////////////////////////////////////////////////////////////////////////
// simple resonant low pass filter
/////////////////////////////////////////////////////////////////////////////
u16 FILTER_resonantLP(u16 in, u16 cutoff) {
	s32 d, e, f;

	cutoff = (cutoff == 0) ? 1 : cutoff;
	
	s32 inp = in;
	inp -= 32768;

	d = (inp - buf0);
	e = (buf0 - buf1);
	
	f = e;
	f *= fb;
	f /= 16384;
	f += d;
	f /= 4;
	
	buf0 += (cutoff * f) / 65536;
	buf1 += (cutoff * e) / 65536;
	
	if (buf0 < -32768) buf0 = -32768;
	if (buf0 >  32767) buf0 =  32767;
	if (buf0 < -32768) buf1 = -32768;
	if (buf0 >  32767) buf1 =  32767;

	d = buf1 + 32768;
	return (u16) d;
}

/////////////////////////////////////////////////////////////////////////////
// basic low pass filter
/////////////////////////////////////////////////////////////////////////////
u16 FILTER_simpleLP(u16 in, u16 cutoff) {
	u16 c = 65535 - cutoff;
	
	if (buf0 < in) {
		buf0 = in - buf0;
		buf0 *= c;
		buf0 >>= 16;
		buf0 = in - buf0;
	} else {
		buf0 = buf0 - in;
		buf0 *= c;
		buf0 >>= 16;
		buf0 = in + buf0;
	}
	
	return buf0;
}

s32		stg2in = 0; // Input value for the second stage 
s32		stg3in = 0; // Input value for the third stage
s32		stg4in = 0; // Input value for the fourth stage

s32		stg1out = 0; // output value of the first stage
s32		stg2out = 0; // output value of the second stage
s32		stg3out = 0; // output value of the third stage
s32 	stg4out = 0; // output value of the fourth stage

s32		wr = 0; // handy-dandy working register type thingy

static const u16 tanh_table[256] = {
	0x0000,0x0266,0x04CD,0x0733,0x0998,0x0BFE,0x0E62,0x10C7,	0x132A,0x158C,0x17EE,0x1A4E,0x1CAE,0x1F0C,0x2168,0x23C4,
	0x261D,0x2875,0x2ACB,0x2D20,0x2F72,0x31C2,0x3410,0x365C,	0x38A5,0x3AED,0x3D31,0x3F73,0x41B2,0x43EF,0x4629,0x485F,
	0x4A93,0x4CC4,0x4EF1,0x511C,0x5343,0x5567,0x5787,0x59A4,	0x5BBD,0x5DD3,0x5FE5,0x61F3,0x63FE,0x6604,0x6807,0x6A06,
	0x6C01,0x6DF8,0x6FEB,0x71DA,0x73C5,0x75AC,0x778E,0x796C,	0x7B46,0x7D1C,0x7EEE,0x80BB,0x8284,0x8448,0x8608,0x87C4,
	0x897C,0x8B2F,0x8CDD,0x8E87,0x902D,0x91CE,0x936B,0x9504,	0x9698,0x9827,0x99B2,0x9B39,0x9CBB,0x9E39,0x9FB3,0xA128,
	0xA298,0xA405,0xA56D,0xA6D1,0xA830,0xA98B,0xAAE2,0xAC34,	0xAD82,0xAECC,0xB012,0xB154,0xB292,0xB3CB,0xB500,0xB631,
	0xB75F,0xB888,0xB9AD,0xBACE,0xBBEB,0xBD05,0xBE1A,0xBF2C,	0xC03A,0xC144,0xC24A,0xC34D,0xC44C,0xC547,0xC63F,0xC733,
	0xC824,0xC911,0xC9FA,0xCAE1,0xCBC3,0xCCA3,0xCD7F,0xCE58,	0xCF2E,0xD000,0xD0CF,0xD19B,0xD264,0xD32A,0xD3ED,0xD4AD,
	0xD56A,0xD623,0xD6DB,0xD78F,0xD840,0xD8EF,0xD99B,0xDA44,	0xDAEA,0xDB8E,0xDC2F,0xDCCE,0xDD6A,0xDE03,0xDE9A,0xDF2F,
	0xDFC1,0xE051,0xE0DE,0xE16A,0xE1F2,0xE279,0xE2FE,0xE380,	0xE400,0xE47E,0xE4FA,0xE574,0xE5EB,0xE661,0xE6D5,0xE747,
	0xE7B7,0xE825,0xE891,0xE8FC,0xE964,0xE9CB,0xEA30,0xEA93,	0xEAF5,0xEB55,0xEBB4,0xEC10,0xEC6B,0xECC5,0xED1D,0xED74,
	0xEDC9,0xEE1C,0xEE6E,0xEEBF,0xEF0E,0xEF5C,0xEFA9,0xEFF4,	0xF03E,0xF087,0xF0CE,0xF114,0xF159,0xF19D,0xF1DF,0xF221,
	0xF261,0xF2A0,0xF2DE,0xF31B,0xF357,0xF391,0xF3CB,0xF404,	0xF43B,0xF472,0xF4A8,0xF4DD,0xF510,0xF543,0xF575,0xF5A6,
	0xF5D7,0xF606,0xF634,0xF662,0xF68F,0xF6BB,0xF6E6,0xF711,	0xF73B,0xF764,0xF78C,0xF7B3,0xF7DA,0xF800,0xF826,0xF84A,
	0xF86F,0xF892,0xF8B5,0xF8D7,0xF8F9,0xF91A,0xF93A,0xF95A,	0xF979,0xF997,0xF9B6,0xF9D3,0xF9F0,0xFA0D,0xFA29,0xFA44,
	0xFA5F,0xFA79,0xFA93,0xFAAD,0xFAC6,0xFADF,0xFAF7,0xFB0E,	0xFB26,0xFB3C,0xFB53,0xFB69,0xFB7F,0xFB94,0xFBA9,0xFBFF
};

/////////////////////////////////////////////////////////////////////////////
// antti's moog low pass filter
/////////////////////////////////////////////////////////////////////////////
u16 FILTER_moogLP(u16 in, u16 resonance, u16 cutoff)
{
	#define RES_SCALE 32767
	#define CUT_SCALE 65535
	cutoff /= 3;
	
	s16 tan; // index for tanh lookup
	s8 tanSign;

	// offset to +- values
	s32 inputvalue = in;
	inputvalue -= 32768;
	
	// Step 1: Calculate feedback signal
	// calculate how much of the output is being fed back into the input 
	// and then add the two together.  stg4out is the output value of the filter and, at this point
	// in the function will still hold the value from the previous sample.  Notice that the
	// feedback value is _subtracted_ from the input.  I'm not 100% on this, but I know that in the
	// analog world the output of the 4 pole filter is 180 degrees out of phase from the input, and 
	// when the output is combined with the input it is inverted to produce positive feedback.  This 
	// does imply that this implementation requires the use of signed values
	//
	// x(n) - r * yd(n-1)
	//
	// x(n) is the input value
	// r is the resonance factor 0<r<1 
	// yd(n-1) is the output from the previous sample
	
	// wr = inputvalue - (resonance * stg4out); 	
	wr = resonance * stg4out;
	wr /= RES_SCALE; // resonance in 0..1 range
	wr = inputvalue - wr;

	// --------------------------------------------------------------------
	// Step 2: Stage 1 Input
	// --------------------------------------------------------------------	
	//
	// We've calculated the combined input value and stored it into wr. 
	// Now we need to calculate what the input of the first filter stage will see
	// ie: the input stage to the filter isn't a simple linear function of the input value
	//
	// tanh(x(n) - r * yd(n-1)) - wa(n-1)
	//
	// wr = x(n) - r * yd(n-1) as per the previous step
	// wa(n-1) is the input value to the second stage from the previous sample	
	
	// wr = tanh(wr) - stg2in; 	// tanh() should be implemented as a look-up table.  Google it.
	tan = (wr > 0) ? wr : -wr; 	// mirror for neagtive values
	tanSign = (wr > 0) ? 1 : -1; 	// mirror for neagtive values
	tan >>= 8;					// tanh_table has 256 entries	
	wr = tanh_table[tan];		// look up	
	wr *= tanSign;
	
	wr -= stg2in;


	// --------------------------------------------------------------------
	// Step 3: Stage 1 Output
	// --------------------------------------------------------------------
	//
	// wr is now holding a value representing what the input of the first filter stage "sees"
	// now let's calculate what the output value of the first stage will be:
	//
	// ya(n) = ya(n-1) + g * (tanh(x(n) - r * yd(n-1)) - wa(n-1))
	//
	// wr = (tanh(x(n) - r * yd(n-1)) - wa(n-1)) as per the previous step
	// g = control value, I'm not sure what the range of this should be.
	// ya(n-1) is the output value of the first stage for the previous sample
	
	// wr = stg1out + cutoff * wr;
	wr *= cutoff;		
	wr /= CUT_SCALE;	
	wr += stg1out;
	
	// stg1out = wr; // stored for use in the next sample
	stg1out = wr; 		// stored for use in the next sample
	
	
	
	// --------------------------------------------------------------------	
	// Step 4: Stage 2 Input
	// --------------------------------------------------------------------
	//
	// wr is now holding a value representing the output of the first filter stage
	// now lets use that to calculate what the input of the second stage will "see"
	//
	// wa(n) = tanh(ya(n))
	//
	// wr = tanh(ya(n)) as per the previous step
	
	// wr = tanh(wr);
	tan = (wr > 0) ? wr : -wr;		// mirror negative values for lookup
	tan >>= 8;						// shift to 8 bits
	tanSign = (wr > 0) ? 1 : -1; 	// mirror for neagtive values
	wr = tanh_table[tan];			// look up
	wr *= tanSign;	
	// stg2in = wr; // stored for use in the next sample
	stg2in = wr; 					// stored for use in the next sample
	
	
	
	// --------------------------------------------------------------------	
	// Step 5: Stage 2 Ouput
	// --------------------------------------------------------------------
	//
	// at this point it's wash, rinse, repeat
	// calculating the input and output values of the remaining stages
	//
	// yb(n) = yb(n-1) + g * (wa(n) - wb(n-1))
	// wr = wa(n) as per the previous step
	
	// wr = stg2out + cutoff * (wr - stg3in);
	wr -= stg3in;
	wr *= cutoff;
	wr /= CUT_SCALE;	
	wr += stg2out;
	
	// stg2out = wr; // stored for use in the next sample
	stg2out = wr; // stored for use in the next sample
	
	

	// --------------------------------------------------------------------
	// Step 6: Stage 3 Input
	// --------------------------------------------------------------------
	//
	// wb(n) = tanh(yb(n)
	// wr = yb(n) as per previous step
	
	// wr = tanh(wr);
	tan = (wr > 0) ? wr : -wr; 		// ...
	tan >>= 8;
	tanSign = (wr > 0) ? 1 : -1; 	// mirror for neagtive values
	wr = tanh_table[tan];
	wr *= tanSign;
	
	// stg3in = wr; // stored for use in the next sample
	stg3in = wr; 					// stored for use in the next sample


	
	// --------------------------------------------------------------------	
	// Step 7: Stage 3 Output 	
	// --------------------------------------------------------------------
	//
	// yc(n) = yc(n-1) + g * (wb(n) - wc(n-1))
	// wr = wb(n) as per previous step
	
	// wr = stg3out + cutoff * (wr - stg4in)
	wr -= stg4in;
	wr *= cutoff;
	wr /= CUT_SCALE;	
	wr += stg3out;
	
	// stage3out = wr;
	stg3out = wr;
	
	
	// --------------------------------------------------------------------	
	// Step 8: Stage 4 Input	
	// --------------------------------------------------------------------
	//
	// wc(n) = tanh(yc(n))
	// wr = yc(n) as per previous step
	
	// wr = tanh(wr);
	tan = (wr > 0) ? wr : -wr; 		// ...
	tan >>= 8;
	tanSign = (wr > 0) ? 1 : -1; 	// mirror for neagtive values
	wr = tanh_table[tan];	
	wr *= tanSign;
	
	// stg4in = wr;
	stg4in = wr;
	
	
	
	// --------------------------------------------------------------------	
	// Step 9: Additional value for Stage 4 Input
	// --------------------------------------------------------------------
	//
	// wd(n) - tanh(yd(n-1)
	// wr = wc(n) as per previous step
	// the equation says that we should tanh(stg4out), however krue's implementation does tanh(resonance).. I think..
	/*
		From Krue's code, r18 was last used to hold the resonance value:
		
		; wd(n) - tanh(yd(n-1))
		mov r30,stg4out
		lpm r18,z
  		sub r16,r18
	*/
	
	// wr = wr - tanh(stg4out);
	tan = (stg4out > 0) ? stg4out : -stg4out; 		// ...
	tan >>= 8;
	tanSign = (stg4out > 0) ? 1 : -1; 	// mirror for neagtive values
	wr -= tanSign * tanh_table[tan];		

	
	// --------------------------------------------------------------------	
	// Step 10: Stage 4 Output
	// --------------------------------------------------------------------
	//
	// yd(n) = yd(n-1) + g * (wc(n) - tanh(yd(n-1)))
	//
	// wr = wc(n) - tanh(yd(n-1)) as per the previous step
	
	// wr = stg4out + cutoff * wr;
	wr *= cutoff;
	wr /= CUT_SCALE;	
	wr += stg4out;
	
	// stg4out = wr;
	stg4out = wr;
	
	// return stg4out;
	// de-offset the output
	return (u16) (wr + 32768);
}

/////////////////////////////////////////////////////////////////////////////
// sets the filter's cutoff
/////////////////////////////////////////////////////////////////////////////
void FILTER_setCutoff(u16 c) {
	filter.cutoff = c;
	
	FILTER_initFilter();
}

/////////////////////////////////////////////////////////////////////////////
// sets the filter's resonance
/////////////////////////////////////////////////////////////////////////////
void FILTER_setResonance(u16 r) {
	filter.resonance = r;
	
	FILTER_resonantLP_Init();
}

/////////////////////////////////////////////////////////////////////////////
// sets the filter type
/////////////////////////////////////////////////////////////////////////////
void FILTER_setFilter(u8 f) {
	if (f < FILTER_TYPES)
		filter.filterType = f;

	FILTER_initFilter();
	
	MIOS32_MIDI_SendDebugMessage("filter type: %d", f);
}


s32 lp1, hp1, bp1, f; 
u16 bandpass_cutoff;

/////////////////////////////////////////////////////////////////////////////
// bandpass filter (stripped down version of the SM svf3)
/////////////////////////////////////////////////////////////////////////////
s16 FILTER_bandpass(s16 input) {
	s32 bandpass, lowpass, highpass;
	s32 in;

	in = input;

	lp1 += (f * bp1) / 65535; 
	hp1 = in - lp1 - bp1; 
	bp1 += (f * hp1) / 65535; 
//	lowpass  = lp1; 
//	highpass = hp1; 
	bandpass = bp1; 

	lp1 += (f * bp1) / 65535; 
	hp1 = in - lp1 - bp1; 
	bp1 += (f * hp1) / 65535; 
//	lowpass  += lp1; 
//	highpass += hp1; 
	bandpass += bp1; 

	lp1 += (f * bp1) / 65535; 
	hp1 = in - lp1 - bp1; 
	bp1 += (f * hp1) / 65535; 
//	lowpass  += lp1; 
//	highpass += hp1; 
	bandpass += bp1;
//	lowpass /= 3; 
//	highpass /= 3; 
	bandpass /= 3; 

	return bandpass;
}

void FILTER_bandpass_Init() {
	u32 x, x2, x3, x5, x7; 

	// cutoff = max(cut, 0.0004); 
	// cutoff = min(cutoff, 1.0);

	bandpass_cutoff = (filter.cutoff > 2047) ? filter.cutoff : 2048;

	f  = bandpass_cutoff / 4;
	/*
	// x = cutoff * 0.52..
	x  = bandpass_cutoff / 4;
	x2 = (x * x)   >> 16;		
	x3 = (x2 * x)  >> 16;
	x5 = (x3 * x2) >> 16;		
	x7 = (x5 * x2) >> 16;		

	// f = 2 * (x - (x3 * 1/6) + (x5 * 1/120) - (x7 * 1/5040)); 
	f  = x * 2;
	f -= (x3 / 3);
	f += (x5 / 60);
	f -= (x7 / 2520);
	*/
}
