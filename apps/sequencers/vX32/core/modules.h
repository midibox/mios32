/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
*/


#ifndef _MODULES_H
#define _MODULES_H

#define max_moduletypes 3		// maximum 254

#define dead_moduletype (max_moduletypes+1)	// don't change
#define dead_value (signed char)(-128)		// don't change


extern void (*const mod_init[max_moduletypes]) (unsigned char nodeid);

extern void (*const mod_process_type[max_moduletypes]) (unsigned char nodeid);

extern void (*const mod_uninit_type[max_moduletypes]) (unsigned char nodeid);

extern const unsigned char mod_ports[max_moduletypes];

extern const unsigned char mod_outbuffer_count[max_moduletypes];







extern void mod_set_clocksource(unsigned char nodeid, unsigned char subclock);




//FIXME remove the externs here, these two should be local
extern void mod_process(unsigned char nodeid);

extern void mod_propagate(unsigned char nodeid);



extern void mod_tick(void);





void mod_uninit(unsigned char nodeid);


void mod_init_graph(unsigned char nodeid, unsigned char moduletype);

void mod_uninit_graph(unsigned char nodeid);




#include "mod_send.h"



#include "mod_sxh.h"
#include "mod_seq.h"
#include "mod_midiout.h"


#endif /* _MODULES_H */
