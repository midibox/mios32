/* $Id$ */
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

#ifndef MAX_VPATTERNS                                                           // number of value patterns allowed to be created
#define MAX_VPATTERNS 0xfe                                                      // maximum 254
#endif

#ifndef MAX_BPATTERNS                                                           // number of binary patterns allowed to be created
#define MAX_BPATTERNS 0xfe                                                      // maximum 254
#endif


                                                                                // do not change defines below here
#define DEAD_VPATTERN (MAX_VPATTERNS+1)                                         // don't change
#define DEAD_BPATTERN (MAX_BPATTERNS+1)                                         // don't change



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern signed char *vPattern[MAX_VPATTERNS];                                    // array of pointers to value patterns

extern signed char *bPattern[MAX_BPATTERNS];                                    // array of pointers to binary patterns


#endif /* _PATTERNS_H */
