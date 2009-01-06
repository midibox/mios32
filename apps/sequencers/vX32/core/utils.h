/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
*/


#ifndef _UTILS_H
#define _UTILS_H


#define util_getbit(var, bit) ((var & (1 << bit)) != 0) // Returns true if bit is set
#define util_setbit(var, bit) (var |= (1 << bit))
#define util_clrbit(var, bit) (var &= ~(1 << bit))
#define util_flipbit(var, bit) (var ^= (1 << bit))


#define util_s8tou8(input) (u8)(input+128)


// -128..0..127  --->  0..16		  // 		 -8 --> 0 		 0 --> 8 		 8 --> 16
#define util_s8tou4(input) ( (u8) (((u8)(input+8)) %16) )
// -128..0..127  --->  0..63		  // 		-32 --> 0 		 0 --> 32 		 32 --> 64
#define util_s8tou6(input) ( (u8) (((u8)(input+32)) %64) )
// -128..0..127  --->  0..127		  // 		-64 --> 0 		 0 --> 64 		 64 --> 128
#define util_s8tou7(input) ( (u8) (((u8)(input+64)) %128) )


/* 
extern u8 util_s8tou7(s8 input);

extern u8 util_s8tou6(s8 input);

extern u8 util_s8tou4(s8 input);
 */

#endif /* _UTILS_H */