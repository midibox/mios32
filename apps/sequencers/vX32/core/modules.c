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
#include "utils.h"

#include <seq_midi_out.h>





void (*const mod_init[max_moduletypes]) (unsigned char nodeid) = {
	&mod_init_clk,													// names of functions to initialise
	&mod_init_seq,													// ports from each module type
	&mod_init_midiout												// must be max_moduletypes of elements
};

void (*const mod_process_type[max_moduletypes]) (unsigned char nodeid) = {
	&mod_proc_clk,													// names of functions to process
	&mod_proc_seq,													// ports from each module type
	&mod_proc_midiout												// must be max_moduletypes of elements
};

void (*const mod_uninit_type[max_moduletypes]) (unsigned char nodeid) = {
	&mod_uninit_clk,												// names of functions to uninitialise
	&mod_uninit_seq,												// ports from each module type
	&mod_uninit_midiout												// must be max_moduletypes of elements
};


const unsigned char mod_ports[max_moduletypes] = {
	8,		//mod_proc_clk											// size of char array to allocate
	32,		//mod_proc_seq											// for each module type
	8		//mod_proc_midiout										// must be max_moduletypes of elements
};

const unsigned char mod_privvars[max_moduletypes] = {
	8,		//mod_proc_clk											// size of char array to allocate
	32,		//mod_proc_seq											// for each module type
	8		//mod_proc_midiout										// must be max_moduletypes of elements
};



void (*const mod_send_buffer_type[max_buffertypes]) (unsigned char nodeid) = {
	&mod_send_midi,													// names of functions to send 
	&mod_send_midi,													// outbuffers according to type
	&mod_send_dummy													// must be max_buffertypes of elements
};

const unsigned char mod_outbuffer_size[max_moduletypes] = {
	0,		//mod_proc_clk											// number of outbuffer bytes
	3,		//mod_proc_seq											// for each module type
	3		//mod_proc_midiout										// must be max_moduletypes of elements
};





void mod_process(unsigned char nodeid) {
	if ((node[nodeid].moduletype) < dead_moduletype) {
		mod_process_type[(node[nodeid].moduletype)](nodeid);		// unititialise according to the moduletype
	}
	
}



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



void mod_send_buffer(unsigned char nodeid) {
	if ((node[nodeid].outbuffer_type) < dead_buffertype) {
		mod_send_buffer_type[(node[nodeid].outbuffer_type)](nodeid);			// unititialise according to the moduletype
	}
	
}


void mod_send_midi(unsigned char nodeid) {
	unsigned char i;
	unsigned char count = (node[nodeid].outbuffer_req);
	unsigned char size = (node[nodeid].outbuffer_size);
	
	for (i = 0; i < count; (i += size)) {
		if ((node[nodeid].outbuffer[i+0] != dead_value) &&						// Paranoia
			(node[nodeid].outbuffer[i+1] != dead_value) &&
			(node[nodeid].outbuffer[i+2] != dead_value) &&
			util_s8tou7((node[nodeid].ports[MOD_SEQ_PORT_MIN_LEN]) > 0) &&
			(node[nodeid].clocksource != dead_sclock) &&
			(sclock[node[nodeid].clocksource].countdown > 0)) {
								
			u8 note = util_s8tou7(node[nodeid].outbuffer[i+0]);
			node[nodeid].outbuffer[i+0] = dead_value;
			u8 velocity = util_s8tou7(node[nodeid].outbuffer[i+1]);
			node[nodeid].outbuffer[i+1] = dead_value;
			u8 length = util_s8tonotelen((node[nodeid].outbuffer[i+2]), (node[nodeid].ports[MOD_SEQ_PORT_MIN_LEN]), sclock[node[nodeid].clocksource].cyclelen);
			node[nodeid].outbuffer[i+2] = dead_value;
			
			if( note && velocity && length ) {						// put note into queue if all values are != 0
				mios32_midi_package_t midi_package;
				midi_package.type     = NoteOn; 					// package type must match with event!
				midi_package.event    = NoteOn;
				
				midi_package.note     = note;
				midi_package.velocity = velocity;
				
				SEQ_MIDI_OUT_Send(USB0, midi_package, SEQ_MIDI_OUT_OnEvent, mod_tick_timestamp);
				midi_package.velocity = 0;							// Note Off event: just change velocity to 0
				SEQ_MIDI_OUT_Send(USB0, midi_package, SEQ_MIDI_OUT_OffEvent, mod_tick_timestamp + length);
				
			}
			
		}
		
	(node[nodeid].outbuffer_req) = 0;
	}
	
}
	
void mod_send_com(unsigned char nodeid) {
	unsigned char i;
	unsigned char count = (node[nodeid].outbuffer_req);
	for (i = 0; i < count; i+node[nodeid].outbuffer_size) {
		if (node[nodeid].outbuffer[i] != dead_value) {
			node[nodeid].outbuffer[i] = dead_value;
			(node[nodeid].outbuffer_req)--;
		}
		
	}
	
}
	
void mod_send_dummy(unsigned char nodeid) {
	unsigned char i;
	unsigned char count = (node[nodeid].outbuffer_req);
	for (i = 0; i < count; i+node[nodeid].outbuffer_size) {
		if (node[nodeid].outbuffer[i] != dead_value) {
			// it's a dummy, dummy 
			// MIOS32_COM_SendChar(COM_USB0, util_s8tou7(node[nodeid].outbuffer[i]));
			node[nodeid].outbuffer[i] = dead_value;
			(node[nodeid].outbuffer_req)--;
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
			if (ticknodeid < dead_nodeid) {
				if (node[ticknodeid].indegree < dead_indegree) {
					if (node[ticknodeid].clocksource < dead_sclock) {						// if this module is clocked
						if (sclock[(node[ticknodeid].clocksource)].ticked > 0) {			// if that clock has ticked
							if ((node[ticknodeid].outbuffer_req) > 0) {						// and there's anything to output
								mod_send_buffer(ticknodeid);								// dump all its outbuffers	
								++(sclock[(node[ticknodeid].clocksource)].ticksent);		// mark the nodes clock source as having recieved its tick
								++(node[ticknodeid].ticked);								// mark the node as ticked
								++(node[ticknodeid].process_req);							// request to process the node to clear the outbuffer and change params ready for next preprocessing
								if (module_ticked == dead_nodeid) module_ticked = ticknodeid;				// remember the first node we hit
							}
							
						}
						
					}
					
				}
				
			}
			
		topolist = topolist->next;									// and move onto the next node in the sorted list
		}
		
		for (i = 0; i < max_sclocks; i++) {							// i is the number of SubClocks
			if (sclock[(i)].status > 0x7F) {						// only do this if the SubClock is active
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




void mod_graph_init(unsigned char nodeid, unsigned char moduletype) {			// do stuff with inputs and push to the outputs 
	
	node[nodeid].moduletype = moduletype;							// store the module type
	node[nodeid].indegree = 0;										// set indegree to 0 this is new
	node[nodeid].indegree_uv = 0;									// set indegree to 0 this is new
	node[nodeid].process_req = 0;									// set process req to 0 this is new
	node[nodeid].ports = pvPortMalloc((sizeof(char))*(mod_ports[moduletype]));				// allocate memory for ports array, elements according to moduletype
	node[nodeid].privvars = pvPortMalloc((sizeof(char))*(mod_privvars[moduletype]));		// allocate memory for internal use, elements according to moduletype
	node[nodeid].ticked = 0;										// clear the outbuffer tick its a bit early for that
	node[nodeid].clocksource = dead_sclock;
	
	node[nodeid].edgelist = NULL;
	node[nodeid].outbuffer_size = mod_outbuffer_size[moduletype];
	
	if ((node[nodeid].outbuffer_size) > 0) {
		node[nodeid].outbuffer = pvPortMalloc(node[nodeid].outbuffer_size);			// allocate memory for ports array, elements according to moduletype
	} else {
		node[nodeid].outbuffer = NULL;								// clear the outbuffer pointer as it will be allocated as required in this types init
	}
	
	node[nodeid].outbuffer_req = 0;
	
}



void mod_set_clocksource(unsigned char nodeid, unsigned char subclock) {
	if (node[nodeid].clocksource < dead_sclock) {
		--(sclock[(node[nodeid].clocksource)].patched);
	}
	
	node[nodeid].clocksource = subclock;
	
	if (subclock < dead_sclock) {
		++(sclock[subclock].patched);
	}
	
}

void mod_uninit(unsigned char nodeid) {
	mod_uninit_type[(node[nodeid].moduletype)](nodeid);				// unititialise according to the moduletype
}


void mod_graph_uninit(unsigned char nodeid, unsigned char moduletype) {						// do stuff with inputs and push to the outputs 
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









void mod_init_clk(unsigned char nodeid) {							// initialize a clock module
	mod_graph_init(nodeid, 0);										// do mandatory graph inits
	
}

void mod_proc_clk(unsigned char nodeid) {							// do stuff with inputs and push to the outputs 

}

void mod_uninit_clk(unsigned char nodeid) {							// do stuff with inputs and push to the outputs 
	mod_graph_uninit(nodeid, 0);									// do mandatory graph uninits
}






void mod_init_seq(unsigned char nodeid) {							// initialize a seq module
	mod_graph_init(nodeid, 1);										// do mandatory graph inits
}

void mod_proc_seq(unsigned char nodeid) {							// do stuff with inputs and push to the outputs 
	if (node[nodeid].ticked) if (++(node[nodeid].ports[MOD_SEQ_PORT_NOTE0_NOTE]) > 16) (node[nodeid].ports[MOD_SEQ_PORT_NOTE0_NOTE]) = 0;
	
	(node[nodeid].ports[MOD_SEQ_PORT_MIN_LEN]) = 0;
	(node[nodeid].ports[MOD_SEQ_PORT_NOTE0_VEL]) = 0;
	(node[nodeid].ports[MOD_SEQ_PORT_NOTE0_LEN]) = 16;

	node[nodeid].outbuffer[0] = (node[nodeid].ports[MOD_SEQ_PORT_NOTE0_NOTE]);
	node[nodeid].outbuffer[1] = (node[nodeid].ports[MOD_SEQ_PORT_NOTE0_VEL]);
	node[nodeid].outbuffer[2] = (node[nodeid].ports[MOD_SEQ_PORT_NOTE0_LEN]);
	node[nodeid].outbuffer_req=1;
}

void mod_uninit_seq(unsigned char nodeid) {							// do stuff with inputs and push to the outputs 
	mod_graph_uninit(nodeid, 1);									// do mandatory graph uninits
}






void mod_init_midiout(unsigned char nodeid) {						// initialize a midi out module
	mod_graph_init(nodeid, 2);										// do mandatory graph inits	
}

void mod_proc_midiout(unsigned char nodeid) { 						// do stuff with inputs and push to the outputs 
	
}

void mod_uninit_midiout(unsigned char nodeid) { 					// do stuff with inputs and push to the outputs 
	mod_graph_uninit(nodeid, 2);									// do mandatory graph uninits
}




// todo


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
