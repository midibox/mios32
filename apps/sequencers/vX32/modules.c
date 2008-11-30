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

#include <mios32.h>
#include "app.h"

#include "graph.h"
#include "mclock.h"
#include "modules.h"

#include <FreeRTOS.h>
#include <portmacro.h>





void (*mod_init[num_moduletypes]) (unsigned char nodeid) = {
	&mod_init_clk,													// names of functions to process
	&mod_init_seq,													// ports from each module type
	&mod_init_midiout												// must be num_moduletypes of elements
};

void (*mod_process_type[num_moduletypes]) (unsigned char nodeid) = {
	&mod_proc_clk,													// names of functions to process
	&mod_proc_seq,													// ports from each module type
	&mod_proc_midiout												// must be num_moduletypes of elements
};

void (*mod_uninit_type[num_moduletypes]) (unsigned char nodeid) = {
	&mod_uninit_clk,												// names of functions to process
	&mod_uninit_seq,												// ports from each module type
	&mod_uninit_midiout												// must be num_moduletypes of elements
};


unsigned char mod_ports[num_moduletypes] = {
	8,		//mod_proc_clk											// size of char array to allocate
	32,		//mod_proc_seq											// for each module type
	8		//mod_proc_midiout										// must be num_moduletypes of elements
};

unsigned char mod_privvars[num_moduletypes] = {
	8,		//mod_proc_clk											// size of char array to allocate
	32,		//mod_proc_seq											// for each module type
	8		//mod_proc_midiout										// must be num_moduletypes of elements
};

unsigned char mod_outbuffer_size[num_moduletypes] = {
	2,		//mod_proc_clk											// size of char array to allocate
	2,		//mod_proc_seq											// for each module type
	4		//mod_proc_midiout										// must be num_moduletypes of elements
};





void mod_process(unsigned char nodeid) {
	mod_process_type[(node[nodeid].moduletype)](nodeid);			// unititialise according to the moduletype
}



void mod_propagate(unsigned char nodeid) {	
	if (nodeid < dead_nodeid) {
		if ((node[nodeid].indegree) < dead_indegree) {					// handle node isnt dead with indegree 0xff
			edge_t *edgepointer = node[nodeid].edgelist; 				// load up the list header
			while (edgepointer != NULL) {								// if there's any edges, then...
				(node[(edgepointer->headnodeid)].ports[(edgepointer->headport)]) = (node[nodeid].ports[(edgepointer->tailport)]);	//copy ports from tail to head
				(node[(edgepointer->headnodeid)].process_req)++;		// request processing
				edgepointer = edgepointer->next;						// search through the list
			}															// until all edges are done
			
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
						if (sclock[(node[ticknodeid].clocksource)].ticked > 0) {			// if its ticked		
							for (i = 0; i < node[ticknodeid].outbuffer_req; i++) {			// dump all its outbuffers
								MIOS32_COM_SendChar(COM_USB0, node[ticknodeid].outbuffer[i].port); // FIXME TESTING
							}
							
							++(sclock[(node[ticknodeid].clocksource)].ticksent);			// mark the nodes clock source as having recieved its tick
							++(node[ticknodeid].ticked);									// mark the node as ticked
							++(node[ticknodeid].process_req);								// request to process the node to clear the outbuffer and change params ready for next preprocessing
							if (module_ticked == dead_nodeid) module_ticked = ticknodeid;	// remember the first node we hit
						}
						
					}
					
				}
				
			}
			
		topolist = topolist->next;									// and move onto the next node in the sorted list
		}
		
		for (i = 0; i < max_sclocks; i++) {							// i is the number of SubClocks
			if (sclock[(i)].status > 0x7F) {						// only do this if the SubClock is active
				if ((sclock[(i)].ticksent > 0) || (sclock[(i)].patched > 0)) {
					(sclock[(i)].ticked)--;
					(sclock[(i)].ticksent) = 0;
				}
				
			}
			
		}
		
	if (module_ticked != dead_nodeid) nodes_proc(module_ticked);	// if any subclock ticked, process from the first one down
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
		node[nodeid].outbuffer = pvPortMalloc((node[nodeid].outbuffer_size)*(sizeof(outbuffer_t)));			// allocate memory for ports array, elements according to moduletype
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
