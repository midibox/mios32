/*
 *  ACToolbox.c
 *  kII
 *
 *  Created by audiocommander on 25.02.07.
 *  Copyright 2006 Michael Markert, http://www.audiocommander.de
 *
 */

#include "ACToolbox.h"



// **** globals ****
unsigned int random_seed = 0xdeadbabe;



//////////////////////////////////////////////////////////////////////////////////////////
//	This function divides two integers; There may be better ASM optimized 
//	solutions for this, but the generated ASM code doesn't look any more
//	complicated than the ASM example I've found  ;)
//	Thanks to goule: http://www.midibox.org/forum/index.php?topic=5944.0
//////////////////////////////////////////////////////////////////////////////////////////

unsigned int ACMath_Divide(unsigned int a, unsigned int b)
{
	// divdes a / b
	if(b > 0) {	
		return ((unsigned int)(a / b));
	} else {
		return 0;
	}
}



//////////////////////////////////////////////////////////////////////////////////////////
// This function generates a 16 bit random number
// The code is taken from TK @ http://www.midibox.org/forum/index.php?topic=6211.0
// The 8bit uchar number is returned immediately
// An 16bit int can be returned by combining (random_seed_l & random_seed_h)
//////////////////////////////////////////////////////////////////////////////////////////


unsigned char ACMath_Random(void) {
  return random_seed = jsw_rand() % 256;
}


unsigned char ACMath_RandomInRange(unsigned char rmax) {
  return ACMath_Random() % (rmax+1);
}




