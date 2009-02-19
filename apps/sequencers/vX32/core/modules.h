/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


#ifndef _MODULES_H
#define _MODULES_H

#define max_moduletypes 4		// maximum 254

#define max_porttypes 3			// don't go crazy here, keep it minimal


#define mod_porttype_timestamp 0			// must be max_porttypes defines here
#define mod_porttype_value 1
#define mod_porttype_flag 2




#define dead_moduletype (max_moduletypes+1)	// don't change
#define dead_porttype (max_porttypes+1)		// don't change
#define dead_value (signed char)(-128)		// don't change
#define dead_timestamp 0xFFFFFFF0			// don't change
#define reset_timestamp 0xFFFFFFFF			// don't change





extern void (*const mod_init[max_moduletypes]) (unsigned char nodeid);

extern void (*const mod_process_type[max_moduletypes]) (unsigned char nodeid);

extern void (*const mod_uninit_type[max_moduletypes]) (unsigned char nodeid);


extern const unsigned char mod_ports[max_moduletypes];

extern const unsigned char *mod_porttypes[max_moduletypes];

// now a macro 
// extern unsigned char mod_get_port_type(unsigned char nodeid, unsigned char port);
#define mod_get_port_type(nodeid, port) ((mod_porttypes[(node[nodeid].moduletype)])[port]);

extern void (*const mod_xlate_table[(max_porttypes*max_porttypes)]) (unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port); // Do it like this: return mod_xlate_table[(tail_port_type*max_porttypes)+head_port_type)];

extern void (*const mod_deadport_table[(max_porttypes)]) (unsigned char nodeid, unsigned char port);



extern const unsigned char mod_outbuffer_count[max_moduletypes];





extern void mod_preprocess(unsigned char startnodeid);


void mod_process(unsigned char nodeid);

void mod_propagate(unsigned char nodeid);



extern void mod_tick(void);


extern void mod_setnexttick(unsigned char nodeid, u32 timestamp, u32 ( *mod_reset_type)(unsigned char resetnodeid));

void mod_uninit(unsigned char nodeid);


void mod_init_graph(unsigned char nodeid, unsigned char moduletype);

void mod_uninit_graph(unsigned char nodeid);

extern unsigned char mod_reprocess;										// counts number of modules which have requested reprocessing (eg after distributing a reset timestamp)


#include "mod_send.h"

#include "mod_xlate.h"


#include "mod_sclk.h"
#include "mod_seq.h"
#include "mod_midiout.h"
#include "mod_sxh.h"


#endif /* _MODULES_H */
