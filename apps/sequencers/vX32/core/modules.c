/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

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
	&mod_init_sclk,													// names of functions to initialise
	&mod_init_seq,													// ports from each module type
	&mod_init_midiout,												// must be max_moduletypes of elements
	&mod_init_sxh,													
};

void (*const mod_process_type[max_moduletypes]) (unsigned char nodeid) = {
	&mod_proc_sclk,													// names of functions to process
	&mod_proc_seq,													// ports from each module type
	&mod_proc_midiout,												// must be max_moduletypes of elements
	&mod_proc_sxh,													
};

void (*const mod_uninit_type[max_moduletypes]) (unsigned char nodeid) = {
	&mod_uninit_sclk,												// names of functions to uninitialise
	&mod_uninit_seq,												// ports from each module type
	&mod_uninit_midiout,												// must be max_moduletypes of elements
	&mod_uninit_sxh,												
};


const unsigned char mod_ports[max_moduletypes] = {
	mod_sclk_ports,													// size of char array to allocate
	mod_seq_ports,													// for each module type
	mod_midiout_ports,												// must be max_moduletypes of elements
	mod_sxh_ports,													
};

const unsigned char mod_privvars[max_moduletypes] = {
	mod_sclk_privvars,												// size of char array to allocate
	mod_seq_privvars,												// for each module type
	mod_midiout_privvars,											// must be max_moduletypes of elements
	mod_sxh_privvars,												
};

const unsigned char *mod_porttypes[max_moduletypes] = {
	mod_sclk_porttypes,												// pointers to port type lists
	mod_seq_porttypes,												// for each module type
	mod_midiout_porttypes,											// must be max_moduletypes of elements
	mod_sxh_porttypes,												
};



void (*const mod_xlate_table[(max_porttypes*max_porttypes)]) (unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port) = {
	&mod_xlate_time_time,	 // timestamp - timestamp
	&mod_xlate_time_value,	 // timestamp - value
	&mod_xlate_time_flag,	 // timestamp - flag
	&mod_xlate_value_time,	 // value - timestamp
	&mod_xlate_value_value,	 // value - value
	&mod_xlate_value_flag,	 // value - flag
	&mod_xlate_flag_time,	 // flag - timestamp
	&mod_xlate_flag_value,	 // flag - value
	&mod_xlate_flag_flag,	 // flag - flag
};


void (*const mod_deadport_table[(max_porttypes)]) (unsigned char nodeid, unsigned char port) = {
	&mod_deadport_time,		// timestamp
	&mod_deadport_value,	// value
	&mod_deadport_flag,		// flag
};

// Now a define, see the header
/*
unsigned char mod_get_port_type(unsigned char nodeid, unsigned char port) {
	return (mod_porttypes[(node[nodeid].moduletype)])[port];
}
*/


unsigned char mod_reprocess = 0;										// counts number of modules which have requested reprocessing (eg after distributing a reset timestamp)



// Never call directly! Use mod_preprocess instead! (scroll down)
void mod_process(unsigned char nodeid) {
	if ((node[nodeid].moduletype) < dead_moduletype) {
		mod_process_type[(node[nodeid].moduletype)](nodeid);		// process according to the moduletype
	}
	
}



// Never call directly! Use mod_preprocess instead! (scroll down)
void mod_propagate(unsigned char nodeid) {
	if (nodeid < dead_nodeid) {
		if ((node[nodeid].indegree) < dead_indegree) {				// handle node isnt dead with indegree 0xff
			edge_t *edgepointer = node[nodeid].edgelist; 			// load up the list header
			while (edgepointer != NULL) {							// if there's any edges, then...
				if (edgepointer->msgxlate != NULL) {
					edgepointer->msgxlate(nodeid, edgepointer->tailport, edgepointer->headnodeid, edgepointer->headport);	//copy ports from tail to head
				}
				(node[(edgepointer->headnodeid)].process_req)++;	// request processing
				edgepointer = edgepointer->next;					// search through the list
			}														// until all edges are done
			
		}
		
	}
	
}




void mod_preprocess(unsigned char startnodeid) {
	nodelist_t *topolist;
	unsigned char procnodeid;
	do {
		if (node_count > 0) {											// handle no nodes
			topolist = topolisthead;									// follow list of topo sorted nodeids
			if (topolist != NULL) {										// handle dead list
				if (!(util_getbit((mclock.status), MCLOCK_STATUS_RESETREQ))				// force processing from root if global reset requested
					&& (startnodeid < dead_nodeid)) {					// otherwise if we are not requested to process from the root
					while ((topolist->nodeid) != startnodeid) {			// search for the start node
						topolist = topolist->next;						// through each entry in the topolist
					}													// until we have the one we want
					
				}
				
				while (topolist != NULL) {								// for each entry in the topolist from there on
					procnodeid = topolist->nodeid;						// get the nodeid
					if (procnodeid < max_nodes) {
						if ((node[procnodeid].process_req > 0) ||		// only process nodes which request it
							(node[procnodeid].ticked > 0) ||			// or nodes which need it because they've got unprocessed ticks
							(node[procnodeid].nexttick == reset_timestamp) ||			// or unprocessed resets
							(util_getbit((mclock.status), MCLOCK_STATUS_RESETREQ))) {	 // unless there is a master reset request
							
							mod_process(procnodeid);					// crunch the numbers according to function pointer
							
							mod_propagate(procnodeid);					// spread out the ports to the downstream nodes
							
							node[procnodeid].process_req = 0;			// clear the process request here, propagation has marked downstream nodes
						}
						
					}
					
					topolist = topolist->next;							// and move onto the next node in the sorted list
				}
				
			}
			
		}
		
	} while (mod_reprocess > 0);
	
}





void mod_tick(void){
	unsigned char i;
	nodelist_t *topolist;
	unsigned char ticknodeid;
	unsigned char module_ticked = dead_nodeid;
	
	if (util_getbit((mclock.status), MCLOCK_STATUS_RUN)) {			// if we're playing
		if (node_count > 0) {										// handle no nodes
		topolist = topolisthead;									// follow list of topo sorted nodeids
			while (topolist != NULL) {								// for each entry in the topolist
			ticknodeid = topolist->nodeid;							// get the nodeid
				if (ticknodeid < max_nodes) {						// if the nodeid is valid
					if (node[ticknodeid].indegree < dead_indegree) {						// if the module is active
						if (node[ticknodeid].nexttick < dead_timestamp) {					// if the module is clocked
							if (node[ticknodeid].nexttick <= mod_tick_timestamp) {			// if that clock has ticked
								if ((node[ticknodeid].outbuffer_req) > 0) {					// and there's anything to output
									mod_send_buffer(ticknodeid);							// dump all its outbuffers
								}
								
								++(node[ticknodeid].ticked);								// mark the node as ticked
								++(node[ticknodeid].process_req);							// request to process the node to clear the outbuffer and change params ready for next preprocessing
								if (module_ticked == dead_nodeid) module_ticked = ticknodeid;				// remember the first node we hit
								
							}
							
						}
						
					}
					
				}
				
				topolist = topolist->next;							// and move onto the next node in the sorted list
				
			}
			
			
			if (module_ticked != dead_nodeid) {
				mod_preprocess(module_ticked);							// if any subclock ticked, process from the first one down
			}
			
		}
		
	}

}


	// FIXME needs a function for setting the .nexttick element (rather than just writing it)
	// which will:
void mod_setnexttick(unsigned char nodeid, u32 timestamp, u32 ( *mod_reset_type)(unsigned char resetnodeid)) {

	if (util_getbit((mclock.status), MCLOCK_STATUS_RESETREQ)) {
		node[nodeid].nexttick = ( *mod_reset_type)(nodeid);
		mod_reprocess = 0; 
		return;
		
	} else {
		//already dead?
		//passing dead?
		//already reset?
		//passing reset?
		//lots of things to think about here
		if (node[nodeid].nexttick == timestamp) return; 				// nothing to do
		
		// 1 FIXME request reprocessing after distributing a reset
		// this function should act upon incoming reset timestamps as follows:
		// write a reset timestamp to node[nodeid].nexttick (as well as to output ports)
		// increment mod_reprocess to cause mod_preprocess to re-run
		// and NOT bother recalculating the ticks downstream like in 3 FIXME because 
		// it should be done on the reproccess run
		
		if (timestamp == reset_timestamp) {
			mod_reprocess++;
			node[nodeid].nexttick = timestamp;
		} else {															// don't do both, the next stuff happens on reprocess after propagation
		
		// 2 FIXME is this a reprocess?
		// check nexttick for reset timestamp and if it is then
		// decrement mod_reprocess
		// calculate the new nexttick and 
		// write it to node[nodeid].nexttick (as well as to output ports)
		// then do 3 FIXME 
		
			if (node[nodeid].nexttick == reset_timestamp) {
				mod_reprocess--;
				node[nodeid].nexttick = mod_reset_type(nodeid);
			} else {
				node[nodeid].nexttick = timestamp;
			}
			
		}
		
	}
	
	// 3 FIXME ensure downstream nodes are ready for preprocessing
	// ie: only preprocess this modules data (timestamps always get preprocessed!)
	// if the earliest nexttick on a downstream module,
	// is >= this module's nexttick
	//
	// trigger recalculation of earliest downstream tick
	// for all nodes upstream of here 
	// that should be stored in node[nodeid].downstreamtick
	// and could be calculated by 
	// scan through all the inward edges on this node and all attached nodes, 
	// replacing their .downstreamtick with this one
	// etc - needs work on edge creation too
	//
	// Perhaps calculating the downstream tick will be too processor heavy
	// it will allow preprocessing which is good
	// but at what cost?
	// i think maybe the preprocessing can go... i dunno... 
	// only real world tests will show me
	
}


void mod_init_graph(unsigned char nodeid, unsigned char moduletype) {			// do stuff with inputs and push to the outputs 
	
	node[nodeid].moduletype = moduletype;							// store the module type
	node[nodeid].indegree = 0;										// set indegree to 0 this is new
	node[nodeid].indegree_uv = 0;									// set indegree to 0 this is new
	node[nodeid].process_req = 0;									// set process req to 0 this is new
	if (mod_ports[moduletype] > 0) node[nodeid].ports = pvPortMalloc((sizeof(char))*(mod_ports[moduletype]));				// allocate memory for ports array, elements according to moduletype
	if (mod_privvars[moduletype] > 0) node[nodeid].privvars = pvPortMalloc((sizeof(char))*(mod_privvars[moduletype]));		// allocate memory for internal use, elements according to moduletype
	node[nodeid].ticked = 0;										// clear the outbuffer tick its a bit early for that
	node[nodeid].nexttick = dead_timestamp;
	
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
