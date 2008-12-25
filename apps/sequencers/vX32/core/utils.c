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

#include <FreeRTOS.h>
#include <portmacro.h>

#include <mios32.h>

#include "utils.h"

u8 util_s8tou7(s8 input) {
	u8 out = (u8)(input+64);
	if (out > 127) out -= 128;
	return out;
}



// EG if signedmin converts to 32:

// 		0		1/32
// 		1		1/16
// 		2		3/32
// 		3		1/8
// 		4		5/32
// 		5		3/16
// 		6		7/32
// 		7		1/4
// 		8		9/32
// 		9		5/16
// 		10		11/32
// 		11		3/8
// 		12		13/32
// 		13		7/16
// 		14		15/32
// 		15		1/2

u32 util_s8tonotelen(s8 signedlen, s8 signedmin, unsigned long cyclelen) {
	unsigned char umin = (((unsigned char)(signedmin)) - 128);
	unsigned int imin = umin + 1;
	unsigned char uinput = (((unsigned char)(signedlen)) - 128);
	unsigned int iinput = uinput + 1;
	u32 len = cyclelen * iinput / imin;
	return len;
}

