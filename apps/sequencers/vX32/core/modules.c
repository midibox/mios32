/* $Id$ */
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

#include "app.h"
#include "graph.h"
#include "mclock.h"
#include "modules.h"
#include "patterns.h"
#include "utils.h"
#include "ui.h"

#include <seq_midi_out.h>





void (*const mod_init[max_moduletypes]) (unsigned char nodeid) = {
	&mod_init_sxh,													// names of functions to initialise
	&mod_init_seq,													// ports from each module type
	&mod_init_midiout												// must be max_moduletypes of elements
};

void (*const mod_process_type[max_moduletypes]) (unsigned char nodeid) = {
	&mod_proc_sxh,													// names of functions to process
	&mod_proc_seq,													// ports from each module type
	&mod_proc_midiout												// must be max_moduletypes of elements
};

void (*const mod_uninit_type[max_moduletypes]) (unsigned char nodeid) = {
	&mod_uninit_sxh,												// names of functions to uninitialise
	&mod_uninit_seq,												// ports from each module type
	&mod_uninit_midiout												// must be max_moduletypes of elements
};


const unsigned char mod_ports[max_moduletypes] = {
	8,		//mod_proc_sxh											// size of char array to allocate
	32,		//mod_proc_seq											// for each module type
	8		//mod_proc_midiout										// must be max_moduletypes of elements
};

const unsigned char mod_privvars[max_moduletypes] = {
	8,		//mod_proc_sxh											// size of char array to allocate
	32,		//mod_proc_seq											// for each module type
	8		//mod_proc_midiout										// must be max_moduletypes of elements
};






void mod_set_clocksource(unsigned char nodeid, unsigned char subclock) {
	if (node[nodeid].clocksource < max_sclocks) {
		--(sclock[(node[nodeid].clocksource)].patched);
	}
	
	node[nodeid].clocksource = subclock;
	
	if (subclock < max_sclocks) {
		node[nodeid].clocksource = subclock;
		++(sclock[subclock].patched);
	}
	
	node[nodeid].process_req++;
	nodes_proc(nodeid);
	
}






// Never call directly! Use nodes_proc instead!
void mod_process(unsigned char nodeid) {
	if ((node[nodeid].moduletype) < dead_moduletype) {
		mod_process_type[(node[nodeid].moduletype)](nodeid);		// unititialise according to the moduletype
	}
	
}



// Never call directly! Use nodes_proc instead!
void mod_propagate(unsigned char nodeid) {	
	if (nodeid < dead_nodeid) {
		if ((node[nodeid].indegree) < dead_indegree) {				// handle node isnt dead with indegree 0xff
			edge_t *edgepointer = node[nodeid].edgelist; 			// load up the list header
			while (edgepointer != NULL) {							// if there's any edges, then...
				(node[(edgepointer->headnodeid)].ports[(edgepointer->headport)]) = (node[nodeid].ports[(edgepointer->tailport)]);	//copy ports from tail to head
				(node[(edgepointer->headnodeid)].process_req)++;	// request processing
				edgepointer = edgepointer->next;					// search through the list
			}														// until all edges are done
			
		}
		
	}
	
}



void mod_tick(void){
	unsigned char i;
	nodelist_t *topolist;
	unsigned char ticknodeid;
	unsigned char module_ticked = dead_nodeid;

	if (node_count > 0) {											// handle no nodes
		topolist = topolisthead;									// follow list of topo sorted nodeids
		
		while (topolist != NULL) {									// for each entry in the topolist
			ticknodeid = topolist->nodeid;							// get the nodeid
			if (ticknodeid < max_nodes) {
				if (node[ticknodeid].indegree < dead_indegree) {
					if (node[ticknodeid].clocksource < max_sclocks) {						// if this module is clocked
						if (sclock[(node[ticknodeid].clocksource)].ticked > 0) {			// if that clock has ticked
							if ((node[ticknodeid].outbuffer_req) > 0) {						// and there's anything to output
								mod_send_buffer(ticknodeid);								// dump all its outbuffers
							}
							
							++(sclock[(node[ticknodeid].clocksource)].ticksent);		// mark the nodes clock source as having recieved its tick
							++(node[ticknodeid].ticked);								// mark the node as ticked
							++(node[ticknodeid].process_req);							// request to process the node to clear the outbuffer and change params ready for next preprocessing
							if (module_ticked == dead_nodeid) module_ticked = ticknodeid;				// remember the first node we hit
							
						}
						
					}
					
				}
				
			}
			
		topolist = topolist->next;									// and move onto the next node in the sorted list
		}
		
		for (i = 0; i < max_sclocks; i++) {							// i is the number of SubClocks
			if (sclock[(i)].status > 0x7F) {						// only do this if the SubClock is active
				//FIXME clocks should tick and be cleared regardless of whether theyre patched
				if ((sclock[(i)].ticksent > 0) && (sclock[(i)].patched > 0)) {
					(sclock[(i)].ticked)--;
					(sclock[(i)].ticksent) = 0;
				}
				
			}
			
		}
		
		if (module_ticked != dead_nodeid) {
			nodes_proc(module_ticked);	// if any subclock ticked, process from the first one down
		}
		
	}

}







void mod_init_graph(unsigned char nodeid, unsigned char moduletype) {			// do stuff with inputs and push to the outputs 
	
	node[nodeid].moduletype = moduletype;							// store the module type
	node[nodeid].indegree = 0;										// set indegree to 0 this is new
	node[nodeid].indegree_uv = 0;									// set indegree to 0 this is new
	node[nodeid].process_req = 0;									// set process req to 0 this is new
	node[nodeid].ports = pvPortMalloc((sizeof(char))*(mod_ports[moduletype]));				// allocate memory for ports array, elements according to moduletype
	node[nodeid].privvars = pvPortMalloc((sizeof(char))*(mod_privvars[moduletype]));		// allocate memory for internal use, elements according to moduletype
	node[nodeid].ticked = 0;										// clear the outbuffer tick its a bit early for that
	node[nodeid].clocksource = dead_sclock;
	
	node[nodeid].edgelist = NULL;
	
	node[nodeid].outbuffer_type = mod_outbuffer_type[moduletype];
	node[nodeid].outbuffer_count = mod_outbuffer_count[moduletype];
	
	if (node[nodeid].outbuffer_type < dead_buffertype) {
		node[nodeid].outbuffer_size = mod_send_buffer_size[node[nodeid].outbuffer_type];
		
		if ((node[nodeid].outbuffer_size) > 0) {
			node[nodeid].outbuffer = pvPortMalloc((node[nodeid].outbuffer_size) * (node[nodeid].outbuffer_count));			// allocate memory for ports array, elements according to moduletype
		} else {
			node[nodeid].outbuffer = NULL;							// clear the outbuffer pointer as it will be allocated as required in this types init
		}
		
	}

	
	node[nodeid].outbuffer_req = 0;
	
	
	mod_init[moduletype](nodeid);								// initialise the module hosted by this node
	
}




void mod_uninit(unsigned char nodeid) {
	mod_uninit_type[(node[nodeid].moduletype)](nodeid);				// unititialise according to the moduletype
}


void mod_uninit_graph(unsigned char nodeid) {						// do stuff with inputs and push to the outputs 
	mod_uninit(nodeid);		 										// uninit the module
	
	node[nodeid].moduletype = dead_moduletype;						// mark node as empty
	node[nodeid].indegree = dead_indegree;							// mark node as empty
	node[nodeid].process_req = 0;									// set process req to 0 this is dead
	
	vPortFree((node[nodeid].ports));		 						// free up the ports
	node[nodeid].ports = NULL;										// fix the pointer
	
	vPortFree((node[nodeid].privvars));		 						// free up the private vars
	node[nodeid].privvars = NULL;									// fix the pointer
	
	vPortFree((node[nodeid].outbuffer));
	node[nodeid].outbuffer = NULL;
	node[nodeid].outbuffer_size = 0;
	node[nodeid].outbuffer_req = 0;
	
	node[nodeid].ticked = 0;										// clear the outbuffer tick
	
}












// todo

// enums for module and outbuffer types!!

// error returns on inits

// fix up the clocks



// module organisation
// internal

/*
masterclock

valuepatterns

bitpatterns

portal constants

// modules


portalinlet   --,___
portaloutlet  --'    constant

lfo
env

AND
NAND
OR
NOR
XOR
XNOR

Mix
Distribute
Mux
filter
mask
divider


scale
chord
arp
delay

binary seq



http://algoart.com/help/artwonk/index.htm


// 0x00 system
	// 
	
// 0x01 sequencers
	// drumseq
	// longseq

// 


// 


// 


// 


// 0xf0
	// XOR
	// AND

*/
