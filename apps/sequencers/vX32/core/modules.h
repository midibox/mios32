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



/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define max_moduletypes 4														// number of module types. maximum 254

#define max_porttypes 3															// number of port types. don't go crazy here, keep it minimal


#define mod_porttype_timestamp 0												// must be max_porttypes defines here
#define mod_porttype_value 1
#define mod_porttype_flag 2



																				// don't change defines below here
#define dead_moduletype (max_moduletypes+1)										// don't change
#define dead_porttype (max_porttypes+1)											// don't change
#define dead_value (signed char)(-128)											// don't change
#define dead_timestamp 0xFFFFFFF0												// don't change
#define reset_timestamp 0xFFFFFFFF												// don't change



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef struct {																// port type and name
	unsigned char porttype;
	unsigned char portname[8];
} mod_portdata_t;


typedef struct {																// module type data like functions for processing etc
	void (*init_fn) (unsigned char nodeid);
	void (*proc_fn) (unsigned char nodeid);
	void (*uninit_fn) (unsigned char nodeid);
	unsigned char ports;
	unsigned char privvars;
	const mod_portdata_t *porttypes;
	unsigned char name[8];
} mod_moduledata_t;



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

const mod_moduledata_t *mod_moduledata_type[max_moduletypes];					// array of module data


extern void (*mod_init[max_moduletypes]) (unsigned char nodeid);				// array of module init functions

extern void (*mod_process_type[max_moduletypes]) (unsigned char nodeid);		// array of module processing functions

extern void (*mod_uninit_type[max_moduletypes]) (unsigned char nodeid);			// array of module init functions


extern unsigned char mod_ports[max_moduletypes];								// array of sizes in bytes of ports per module type

extern unsigned char mod_privvars[max_moduletypes];								// array of sizes in bytes of private vars per module type

extern const mod_portdata_t *mod_porttypes[max_moduletypes];					// array of port doto per module type



extern void (*const mod_xlate_table[(max_porttypes*max_porttypes)]) 			// array of translator functions from any port type to any port type
						(unsigned char tail_nodeid, unsigned char tail_port,
						unsigned char head_nodeid, unsigned char head_port);	// Do it like this: return mod_xlate_table[(tail_port_type*max_porttypes)+head_port_type)];

extern void (*const mod_deadport_table[(max_porttypes)]) 
						(unsigned char nodeid, unsigned char port);				// array of functions for writing dead values to ports



extern const unsigned char mod_outbuffer_count[max_moduletypes];				// array of outbuffer counts per module type



extern void mod_init_moduledata(void);											// function for initialising module data on startup

extern unsigned char mod_get_port_type
									(unsigned char nodeid, unsigned char port);	// returns port type


extern void mod_preprocess(unsigned char startnodeid);							// does preprocessing for all modules


void mod_process(unsigned char nodeid);											// does preprocessing for one module

void mod_propagate(unsigned char nodeid);										// pushes data out from ports on a module to all modules it is patched into



extern void mod_tick(void);														// check for ticks on modules


extern void mod_setnexttick														// set the node[nodeid].nexttick value and handle it right. never write that value directly.
							(unsigned char nodeid, u32 timestamp, 
							u32 ( *mod_reset_type)(unsigned char resetnodeid));

extern void mod_tickpriority(unsigned char nodeid);								// sets node[nodeid].downstream tick for this node and all nodes that patch in to it

void mod_uninit(unsigned char nodeid);											// ununitialised a module according to it's module type


void mod_init_graph(unsigned char nodeid, unsigned char moduletype);			// mandatory graph inits, gets called automatically

void mod_uninit_graph(unsigned char nodeid);									// mandatory graph uninits, gets called automatically

extern unsigned char mod_reprocess;												// counts number of modules which have requested reprocessing (eg after distributing a reset timestamp)


																				// includes for required functions
#include "mod_send.h"

#include "mod_xlate.h"

																				// includes for modules
#include "mod_sclk.h"
#include "mod_seq.h"
#include "mod_midiout.h"
#include "mod_sxh.h"
																				// modules just need to be added here and to the mod_moduledata_type array to include them in the app

#endif /* _MODULES_H */
