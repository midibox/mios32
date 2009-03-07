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



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

const mod_moduledata_t *mod_moduledata_type[max_moduletypes] = {
	&mod_sclk_moduledata,														// moduletype 0
	&mod_seq_moduledata,														// moduletype 1
	&mod_midiout_moduledata,													// moduletype 2
	&mod_sxh_moduledata,														// moduletype 3
};




void (*mod_init[max_moduletypes]) (unsigned char nodeid);						// array of init function for module types

void (*mod_process_type[max_moduletypes]) (unsigned char nodeid);				// array of preprocess function for module types

void (*mod_uninit_type[max_moduletypes]) (unsigned char nodeid);				// array of uninit function for module types


unsigned char mod_ports[max_moduletypes];										// array of port byte sizes per module type

unsigned char mod_privvars[max_moduletypes];									// array of private byte sizes per module type

const mod_portdata_t *mod_porttypes[max_moduletypes];							// array that holds pointers to the module data for each module type


void (*const mod_xlate_table[(max_porttypes*max_porttypes)]) 
						(unsigned char tail_nodeid, unsigned char tail_port,
						unsigned char head_nodeid, unsigned char head_port) = {
	&mod_xlate_time_time,														// timestamp - timestamp
	&mod_xlate_time_value,														// timestamp - value
	&mod_xlate_time_flag,														// timestamp - flag
	&mod_xlate_value_time,														// value - timestamp
	&mod_xlate_value_value,														// value - value
	&mod_xlate_value_flag,														// value - flag
	&mod_xlate_flag_time,														// flag - timestamp
	&mod_xlate_flag_value,														// flag - value
	&mod_xlate_flag_flag,														// flag - flag
};


void (*const mod_deadport_table[(max_porttypes)]) 
						(unsigned char nodeid, unsigned char port) = {
	&mod_deadport_time,															// timestamp
	&mod_deadport_value,														// value
	&mod_deadport_flag,															// flag
};



unsigned char mod_reprocess = 0;												// counts number of modules which have requested reprocessing (eg after distributing a reset timestamp)



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Initialises module type data. called at init
/////////////////////////////////////////////////////////////////////////////

void mod_init_moduledata(void) {
	unsigned char thistype;
	
	for (thistype = 0; thistype < max_moduletypes; thistype++) {
		mod_init[thistype] = mod_moduledata_type[thistype]->init_fn;
		mod_process_type[thistype] = mod_moduledata_type[thistype]->proc_fn;
		mod_uninit_type[thistype] = mod_moduledata_type[thistype]->uninit_fn;
		mod_ports[thistype] = mod_moduledata_type[thistype]->ports;
		mod_privvars[thistype] = mod_moduledata_type[thistype]->privvars;
		mod_porttypes[thistype] = mod_moduledata_type[thistype]->porttypes;
	}
	
}


/////////////////////////////////////////////////////////////////////////////
// Returns the port type for a given port
// in port number and node ID
// out: port type
/////////////////////////////////////////////////////////////////////////////

unsigned char mod_get_port_type(unsigned char nodeid, unsigned char port) {
	return (mod_porttypes[(node[nodeid].moduletype)])[port].porttype;
	
}



/////////////////////////////////////////////////////////////////////////////
// Processes a single module
// in: node ID
// Never call directly! Use mod_preprocess instead! (scroll down)
/////////////////////////////////////////////////////////////////////////////

void mod_process(unsigned char nodeid) {
	if ((node[nodeid].moduletype) < dead_moduletype) {
		mod_process_type[(node[nodeid].moduletype)](nodeid);					// process according to the moduletype
	}
	
}



/////////////////////////////////////////////////////////////////////////////
// Propagates all ports to their connected ports via the patch cables
// in: node ID
// Never call directly! Use mod_preprocess instead! (scroll down)
/////////////////////////////////////////////////////////////////////////////

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



/////////////////////////////////////////////////////////////////////////////
// Preprocess all nodes from a given node down
// in: node ID to start from
/////////////////////////////////////////////////////////////////////////////

void mod_preprocess(unsigned char startnodeid) {
	nodelist_t *topolist;
	unsigned char procnodeid;
	do {
		if (node_count > 0) {													// handle no nodes
			topolist = topolisthead;											// follow list of topo sorted nodeids
			if (topolist != NULL) {												// handle dead list
				if (!(util_getbit((mclock.status), MCLOCK_STATUS_RESETREQ))		// force processing from root if global reset requested
					&& (startnodeid < dead_nodeid)) {							// otherwise if we are not requested to process from the root
					while ((topolist->nodeid) != startnodeid) {					// search for the start node
						topolist = topolist->next;								// through each entry in the topolist
					}															// until we have the one we want
					
				}
				
				while (topolist != NULL) {										// for each entry in the topolist from there on
					procnodeid = topolist->nodeid;								// get the nodeid
					if (procnodeid < max_nodes) {								// only process nodes which...
						if (
							(
							 (node[procnodeid].nexttick == reset_timestamp) ||	// have unprocessed resets
							 (util_getbit(
								(mclock.status), MCLOCK_STATUS_RESETREQ))		// or if there is a master reset request
							)
							 ||													// otherwise only preprocess if...
							(
							 (
							  (node[procnodeid].process_req > 0) ||				// they request it
							  (node[procnodeid].ticked > 0)						// or need it because they've got unprocessed ticks
							 )
							  &&
							 (
							  (node[procnodeid].downstreamtick 
							  >= node[procnodeid].nexttick) ||					// but only if it's safe for the downstream modules
							  (node[procnodeid].nexttick >= dead_timestamp)		// or if this module needs attention
							 )
							)
							) 
							{
							
							mod_process(procnodeid);							// crunch the numbers according to function pointer
							
							mod_propagate(procnodeid);							// spread out the ports to the downstream nodes
							
							node[procnodeid].process_req = 0;					// clear the process request here, propagation has marked downstream nodes
						}
						
					}
					
					topolist = topolist->next;									// and move onto the next node in the sorted list
				}
				
			}
			
		}
		
	} while (mod_reprocess > 0);
	
}



/////////////////////////////////////////////////////////////////////////////
// Checks to see if modules have ticked 
// according to the master clock and each module's nexttick timestamp
/////////////////////////////////////////////////////////////////////////////

void mod_tick(void){
	unsigned char i;
	nodelist_t *topolist;
	unsigned char ticknodeid;
	unsigned char module_ticked = dead_nodeid;
	
	if (util_getbit((mclock.status), MCLOCK_STATUS_RUN)) {						// if we're playing
		if (node_count > 0) {													// handle no nodes
		topolist = topolisthead;												// follow list of topo sorted nodeids
			while (topolist != NULL) {											// for each entry in the topolist
			ticknodeid = topolist->nodeid;										// get the nodeid
				if (ticknodeid < max_nodes) {									// if the nodeid is valid
					if (node[ticknodeid].indegree < dead_indegree) {			// if the module is active
						if (node[ticknodeid].nexttick < dead_timestamp) {		// if the module is clocked
							if (node[ticknodeid].nexttick 
								<= mod_tick_timestamp) {						// if that clock has ticked
								if ((node[ticknodeid].outbuffer_req) > 0) {		// and there's anything to output
									mod_send_buffer(ticknodeid);				// dump all its outbuffers
								}
								
								++(node[ticknodeid].ticked);					// mark the node as ticked
								++(node[ticknodeid].process_req);				// request to process the node to clear the outbuffer and change params ready for next preprocessing
								if (module_ticked == dead_nodeid) 
									module_ticked = ticknodeid;					// remember the first node we hit
								
							}
							
						}
						
					}
					
				}
				
				topolist = topolist->next;										// and move onto the next node in the sorted list
				
			}
			
			
			if (module_ticked != dead_nodeid) {
				mod_preprocess(module_ticked);									// if any subclock ticked, process from the first one down
			}
			
		}
		
	}

}



/////////////////////////////////////////////////////////////////////////////
// function for setting the .nexttick element correctly
// never simply write to that variable, use this function
// in: node ID, timestamp you want (may not be what you get!) and
//     function pointer to reset the module
//     if this function deems it necessary
/////////////////////////////////////////////////////////////////////////////

void mod_setnexttick
	(unsigned char nodeid, u32 timestamp
	, u32 ( *mod_reset_type)(unsigned char resetnodeid)) {
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[vX] Setting nexttick for node %d\n", nodeid);
#endif
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[vX] Incoming timestamp is %u\n", timestamp);
#endif
	if (!(util_getbit((mclock.status), MCLOCK_STATUS_RESETREQ))) {				// if no global reset
		
		if (node[nodeid].nexttick == timestamp) {								// same as before?
			return; 															// nothing to do then
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[vX] No change");
#endif
		}
		
		if (timestamp != reset_timestamp) {
			
			if (node[nodeid].nexttick == reset_timestamp) {						// If this is a reprocess? check nexttick for reset timestamp and if it is then
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[vX] Resetting node due to local reset request\n");
#endif
				mod_reprocess--;												// decrement mod_reprocess and 
				node[nodeid].nexttick = mod_reset_type(nodeid);					// calculate the new nexttick
			} else {															// otherwise use the passed timestamp
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[vX] Writing timestamp\n");
#endif
				node[nodeid].nexttick = timestamp;								// write it to node[nodeid].nexttick (output ports should be written elsewhere)
			}
			
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[vX] Setting downstream ticks\n");
#endif
			mod_tickpriority(nodeid);											// then do upstream nodes' timestamps
			
		} else {																// Request reprocessing after distributing a reset
			
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[vX] Incoming reset request\n", timestamp);
#endif
																				// Does not bother recalculating the ticks upstream because it should be done on the reprocess run
			node[nodeid].nexttick = timestamp;									// write the incoming reset timestamp to node[nodeid].nexttick (as well as to output ports)
			mod_reprocess++;													// increment mod_reprocess to cause mod_preprocess to re-run
		}
		
	} else {
		
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[vX] Resetting node due to global reset request\n");
#endif
		node[nodeid].nexttick = ( *mod_reset_type)(nodeid);
		mod_reprocess = 0; 
		
	}
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[vX] New nexttick for node %d is %u\n",nodeid ,node[nodeid].nexttick);
#endif
	
}



/////////////////////////////////////////////////////////////////////////////
// function for setting the .downstreamtick variable
// never simply write to that variable, use this function to find it
// in: node ID to start at. It'll go upstream by itself.
/////////////////////////////////////////////////////////////////////////////

void mod_tickpriority(unsigned char nodeid) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Setting downstream ticks above %d\n", nodeid);
#endif
	nodelist_t *upstreamnodes = NULL;
	nodelist_t *upstreamhead = NULL;
	nodelist_t *upstreamtail = NULL;
	u32 temptimestamp = dead_timestamp;
	unsigned char tempnodeid;
	unsigned char tempnodeid_ds;
	
	
	if (nodeid < dead_nodeid) {
		if ((node[nodeid].indegree) < dead_indegree) {							// handle node isnt dead with indegree 0xff
			u32 soonesttick_ds = dead_timestamp;								// this var is used to hold the earliest of all downstream nodes' ticks
			
			edge_t *edgepointer = node[nodeid].edgelist; 						// load up the list header
			while (edgepointer != NULL) {										// if there's any outward edges, then...
#if DEBUG_VERBOSE_LEVEL >= 3
DEBUG_MSG("[vX] Finding earliest downstream tick from this node\n");
#endif
				tempnodeid_ds = (edgepointer->headnodeid);						// for each downstream node
				
				if ((node[tempnodeid_ds].deleting > 0)							// if it's not being deleted
				 && (node[tempnodeid_ds].indegree < dead_indegree)) {			// or already dead (shouldn't happen)
					 
					temptimestamp = node[tempnodeid_ds].downstreamtick;
					
					if ((node[tempnodeid_ds].nexttick) < temptimestamp) 
						temptimestamp = (node[tempnodeid_ds].nexttick);			// get the soonest tick out of it and all it's downstream nodes
				}
				
#if DEBUG_VERBOSE_LEVEL >= 4
DEBUG_MSG("[vX] Node %d soonest tick is %u\n", tempnodeid_ds, temptimestamp);
#endif
				
				if (temptimestamp < soonesttick_ds) 
					soonesttick_ds = temptimestamp;								// and save it if it's the lowest one downstream of here
				
#if DEBUG_VERBOSE_LEVEL >= 4
DEBUG_MSG("[vX] Next edge... \n");
#endif
			edgepointer = edgepointer->next;
			}																	// until all edges are done
			
			node[nodeid].downstreamtick = soonesttick_ds;						// then write it. that's it for downstream... here we go upstream...
			
			
			
			if (node[nodeid].edgelist_in != NULL) {								// if there are any upstream nodes
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[vX] Found upstream nodes\n");
#endif
				
				edge_t *edgepointer = node[nodeid].edgelist_in;					// load up the list header
				
				tempnodeid = (edgepointer->tailnodeid);
				
				upstreamtail = pvPortMalloc(sizeof(nodelist_t));				// make a new nodelist
				
				upstreamnodes = upstreamhead = upstreamtail;					// and save it
				
				upstreamtail->nodeid = tempnodeid;								// add the first edge's tail node
				upstreamtail->next = NULL;
				
				edgepointer = edgepointer->head_next;
				
				while (edgepointer != NULL) {									// if there's any more edges, then...
#if DEBUG_VERBOSE_LEVEL >= 3
	DEBUG_MSG("[vX] Node %d is upstream\n", tempnodeid);
#endif
					upstreamtail->next = pvPortMalloc(sizeof(nodelist_t));		// add the tailnodes there too
					upstreamtail = upstreamtail->next;
					upstreamtail->nodeid = (edgepointer->tailnodeid);
					upstreamtail->next = NULL;
					
#if DEBUG_VERBOSE_LEVEL >= 4
	DEBUG_MSG("[vX] Next edge... \n");
#endif
				edgepointer = edgepointer->head_next;
				
				}																// until all upstream nodes are listed
				
				
				
				u32 soonesttick = dead_timestamp;								// this var is used to hold the earliest of the current nodes ticks
				
				upstreamnodes = upstreamhead;									// starting with the head of the list
				
				while (upstreamnodes != NULL) {									// for each of the nodes to be updated
					
					tempnodeid = (upstreamnodes->nodeid);
					
					soonesttick = node[tempnodeid].nexttick;
					if ((node[tempnodeid].downstreamtick < soonesttick)) {
						soonesttick = (node[tempnodeid].downstreamtick);
					}															// remember the current node's earliest timestamp as seen from above
#if DEBUG_VERBOSE_LEVEL >= 3
	DEBUG_MSG("[vX] Found soonest tick for this node %d\n", tempnodeid);
	DEBUG_MSG("[vX] Which is %u\n", soonesttick);
#endif
					
					u32 soonesttick_ds = dead_timestamp;						// this var is used to hold the earliest of all downstream nodes ticks
					
					edge_t *edgepointer = node[tempnodeid].edgelist;			// load up the list header
					while (edgepointer != NULL) {								// if there's any edges, then...
#if DEBUG_VERBOSE_LEVEL >= 3
	DEBUG_MSG("[vX] Finding earliest downstream tick from this node\n");
#endif
						tempnodeid_ds = (edgepointer->headnodeid);				// for each downstream node
						
						if ((node[tempnodeid_ds].deleting > 0)					// if it's not being deleted
						 && (node[tempnodeid_ds].indegree < dead_indegree)) {	// or already dead (shouldn't happen)
							 
							temptimestamp = node[tempnodeid_ds].downstreamtick;
							
							if ((node[tempnodeid_ds].nexttick) < temptimestamp) 
								temptimestamp = (node[tempnodeid_ds].nexttick);	// get the soonest tick for each one
						}
						
#if DEBUG_VERBOSE_LEVEL >= 4
	DEBUG_MSG("[vX] Node %d soonest tick is %u\n", tempnodeid_ds, temptimestamp);
#endif
						
						if (temptimestamp < soonesttick_ds) 
							soonesttick_ds = temptimestamp;						// and save it if it's the lowest one downstream of here
						
#if DEBUG_VERBOSE_LEVEL >= 4
	DEBUG_MSG("[vX] Next edge... \n");
#endif
					edgepointer = edgepointer->next;
					}															// until all edges are done
					
					
#if DEBUG_VERBOSE_LEVEL >= 3
		DEBUG_MSG("[vX] Node %d soonest tick downstream is %u\n", tempnodeid, soonesttick_ds);
#endif
					node[tempnodeid].downstreamtick = soonesttick_ds;			// save the lowest downstream tick to this node
					
					soonesttick_ds = node[tempnodeid].nexttick;					// we can reuse this var for a moment
					if ((node[tempnodeid].downstreamtick < soonesttick_ds)) {
						soonesttick_ds = (node[tempnodeid].downstreamtick);
					}															// to get the new soonest tick here
					
					if (soonesttick != soonesttick_ds) {						// and if it's changed
						
						edge_t *edgepointer = node[tempnodeid].edgelist_in;		// we better tell the nodes upstream about it
						
						while (edgepointer != NULL) {							// so if there's any nodes upstream, then...
#if DEBUG_VERBOSE_LEVEL >= 4
	DEBUG_MSG("[vX] Node %d is upstream\n", tempnodeid);
#endif
							upstreamtail->next = 
									pvPortMalloc(sizeof(nodelist_t));			// add them to the tail of the existing nodelist
							upstreamtail = upstreamtail->next;
							upstreamtail->nodeid = (edgepointer->tailnodeid);
							upstreamtail->next = NULL;
							
#if DEBUG_VERBOSE_LEVEL >= 4
	DEBUG_MSG("[vX] Next edge... \n");
#endif
						edgepointer = edgepointer->head_next;
						
						}														// until all upstream nodes are listed
						
					}
#if DEBUG_VERBOSE_LEVEL >= 3
		DEBUG_MSG("[vX] Moving onto the next node\n");
#endif
					upstreamhead = upstreamnodes;
					upstreamnodes = upstreamnodes->next;						// then advance to the next node in the list
					vPortFree(upstreamhead);									// and delete this one because we are done with it
					
				}
				
			} else {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[vX] No upstream nodes\n");
#endif
			
			}
			
		} else {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[vX] Node is marked dead\n");
#endif
		
		}
		
	} else {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[vX] Node ID invalid\n");
#endif
		
	}
	
#if DEBUG_VERBOSE_LEVEL >= 1
	DEBUG_MSG("[vX] Done\n");
#endif
}




void mod_init_graph(unsigned char nodeid, unsigned char moduletype) {			// do stuff with inputs and push to the outputs 
	
	node[nodeid].moduletype = moduletype;										// store the module type
	node[nodeid].indegree = 0;													// set indegree to 0 this is new
	node[nodeid].indegree_uv = 0;												// set indegree to 0 this is new
	node[nodeid].process_req = 0;												// set process req to 0 this is new
	if (mod_ports[moduletype] > 0) node[nodeid].ports = 
			pvPortMalloc((sizeof(char))*(mod_ports[moduletype]));				// allocate memory for ports array, elements according to moduletype
	if (mod_privvars[moduletype] > 0) node[nodeid].privvars = 
			pvPortMalloc((sizeof(char))*(mod_privvars[moduletype]));			// allocate memory for internal use, elements according to moduletype
	node[nodeid].ticked = 0;													// clear the outbuffer tick its a bit early for that
	node[nodeid].deleting = 0;
	
	node[nodeid].nexttick = dead_timestamp;
	node[nodeid].downstreamtick = dead_timestamp;
	
	node[nodeid].edgelist = NULL;
	node[nodeid].edgelist_in = NULL;
	
	node[nodeid].outbuffer_type = mod_outbuffer_type[moduletype];
	node[nodeid].outbuffer_count = mod_outbuffer_count[moduletype];
	
	if (node[nodeid].outbuffer_type < dead_buffertype) {
		node[nodeid].outbuffer_size = 
							mod_send_buffer_size[node[nodeid].outbuffer_type];
		
		if ((node[nodeid].outbuffer_size) > 0) {
			node[nodeid].outbuffer = 
					pvPortMalloc((node[nodeid].outbuffer_size) 
					* (node[nodeid].outbuffer_count));							// allocate memory for ports array, elements according to moduletype
		} else {
			node[nodeid].outbuffer = NULL;										// clear the outbuffer pointer as it will be allocated as required in this types init
		}
		
	}
	
	node[nodeid].outbuffer_req = 0;
	
	mod_init[moduletype](nodeid);												// initialise the module hosted by this node
	
}



/////////////////////////////////////////////////////////////////////////////
// uninitialise a module after deletion as per it's module type
// in: node ID
/////////////////////////////////////////////////////////////////////////////

void mod_uninit(unsigned char nodeid) {
	mod_uninit_type[(node[nodeid].moduletype)](nodeid);				// unititialise according to the moduletype
}


/////////////////////////////////////////////////////////////////////////////
// mandatory graph uninits for a module after deletion
// in: node ID
/////////////////////////////////////////////////////////////////////////////

void mod_uninit_graph(unsigned char nodeid) {						// do stuff with inputs and push to the outputs 
	mod_uninit(nodeid);		 										// uninit the module
	
	node[nodeid].moduletype = dead_moduletype;						// mark node as empty
	node[nodeid].indegree = dead_indegree;							// mark node as empty
	node[nodeid].indegree_uv = 0;									// set indegree to 0 this is new
	node[nodeid].edgelist = NULL;
	node[nodeid].edgelist_in = NULL;
	node[nodeid].process_req = 0;									// set process req to 0 this is dead
	node[nodeid].deleting = 0;
	
	vPortFree((node[nodeid].ports));		 						// free up the ports
	node[nodeid].ports = NULL;										// fix the pointer
	
	vPortFree((node[nodeid].privvars));		 						// free up the private vars
	node[nodeid].privvars = NULL;									// fix the pointer
	
	vPortFree((node[nodeid].outbuffer));
	node[nodeid].outbuffer = NULL;
	node[nodeid].outbuffer_size = 0;
	node[nodeid].outbuffer_req = 0;
	node[nodeid].outbuffer_count = 0;
	
	node[nodeid].nexttick = dead_timestamp;
	node[nodeid].downstreamtick = dead_timestamp;
	node[nodeid].ticked = 0;										// clear the outbuffer tick
	
}









// todo

// debug messages

// error returns on inits






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
