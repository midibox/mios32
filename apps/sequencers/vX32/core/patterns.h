/* $Id:  $ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
*/


#ifndef _PATTERNS_H
#define _PATTERNS_H

#define max_vpatterns 0xfe		// maximum 254
#define dead_vpattern (max_vpatterns+1)		// don't change

#define max_bpatterns 0xfe		// maximum 254
#define dead_bpattern (max_bpatterns+1)		// don't change

extern signed char *vpattern[max_vpatterns];

extern signed char *bpattern[max_bpatterns];


#endif /* _PATTERNS_H */
