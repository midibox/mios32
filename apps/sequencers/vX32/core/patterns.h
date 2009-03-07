/* $Id:  $ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


#ifndef _PATTERNS_H
#define _PATTERNS_H



/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#ifndef max_vpatterns															// number of value patterns allowed to be created
#define max_vpatterns 0xfe														// maximum 254
#endif

#ifndef max_bpatterns															// number of binary patterns allowed to be created
#define max_bpatterns 0xfe														// maximum 254
#endif


																				// do not change defines below here
#define dead_vpattern (max_vpatterns+1)											// don't change
#define dead_bpattern (max_bpatterns+1)											// don't change



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern signed char *vpattern[max_vpatterns];									// array of pointers to value patterns

extern signed char *bpattern[max_bpatterns];									// array of pointers to binary patterns


#endif /* _PATTERNS_H */
