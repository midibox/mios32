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
#include "modules.h"

#include <FreeRTOS.h>
#include <portmacro.h>



node_t node[max_nodes];												// array of sructs to hold node/module info

nodelist_t *topolisthead;											// list of topologically sorted nodeids

unsigned char node_count;

unsigned char nodeidinuse[(max_nodes+1)/8];


void graph_init(void) {
	node_count = 0;
	unsigned char n = 0;
	for (n = 0; n <= (max_nodes+1)/8; n++) {
		nodeidinuse[n] = 0;
	}
	
	for (n = 0; n <= max_nodes; n++) {								// init this array 	
		node[n].name[0] = 'E';										// so that all nodes start off dead
		node[n].name[1] = 'M';
		node[n].name[2] = 'P';
		node[n].name[3] = 'T';
		node[n].name[4] = 'Y';
		node[n].name[5] = 0;
		node[n].moduletype = dead_moduletype;
		node[n].process_req = 0;
		node[n].indegree = dead_indegree;
		node[n].indegree_uv = dead_indegree;
		node[n].ports = NULL;
		node[n].privvars = NULL;
		node[n].edgelist = NULL;
		
	}
	
	topolisthead = NULL;
}



unsigned char node_add(unsigned char moduletype) {
	nodelist_t *topoinsert;
	unsigned char newnodeid;
	if (node_count < max_nodes-1) {									// handle max nodes
		if (moduletype <= num_moduletypes) {						// make sure the moduletype is logal
			newnodeid = node_count+1; //FIXME TESTING
			node_count++; //FIXME TESTING !!!!!!!! 
			// fix nodeid_gen !!!!!
			// fix nodeid_gen !!!!!
			// fix nodeid_gen !!!!!
			//FIXME TESTING newnodeid = nodeid_gen();							// grab a new nodeid and save it
			if (newnodeid < dead_nodeid) {							// if no error grabbing the ID
				mod_init[moduletype](newnodeid);					// initialise the module hosted by this node
				topoinsert = topolisthead;							// save the topolist root
				topolisthead = pvPortMalloc(sizeof(nodelist_t));	// replace the topolist root with new space
				topolisthead->next = topoinsert;					// restore the old root to the next pointer
				topolisthead->nodeid = newnodeid;					// and insert the new node at the root
				node[newnodeid].process_req++;
				nodes_proc(newnodeid);								// if it's sorted ok, process from here down			return newnodeid;											// return the ID of the new node
			} else {
				return dead_nodeid;
			}
			
		} else {
			return dead_nodeid;
		}
		
	} else {
		return dead_nodeid;
	}
	
}


unsigned char nodeid_gen(void) {
	unsigned char indexbit;
	unsigned char arrayindex;
	unsigned char a = dead_nodeid;									// prep the error handler
	if (node_count < (max_nodes-1)) {								// if were not full
		for (arrayindex = 0; arrayindex < 32; arrayindex++) {		// step through the array
			if (nodeidinuse[arrayindex] != 0xff) {					// until we find an available slot
				a = arrayindex;
				break;
			}
			
		}
		
		if (a < 0xff) {												// if there are errors
			for (indexbit = 0; indexbit < 8; indexbit++) {			// step through the byte
				if (!(nodeidinuse[a] & (1<<indexbit))) {			// if we find a 0
					nodeidinuse[a] != (1<<indexbit);				// set the bit
					node_count++;									// increment the node count
					return ((8*(a)) + (indexbit));					// and return the new nodeid
				}
				
			}
			
		} else {													// otherwise
			return dead_nodeid;										// nothing free do some error handling
		}
		
	} else {
		return dead_nodeid;											// error handling if were full
	}
}





unsigned char node_del(unsigned char delnodeid) {
	unsigned char returnval;										// prep error handler
	unsigned char n;
	edge_t *edgepointer;
	edge_t *prevedge;

	if (node_count > 0) {											// handle node_count 0
		if (node[delnodeid].indegree < dead_indegree) {				// handle node dead indegree 0xff
			if (node[delnodeid].indegree > 0) {						// if this node has any inward edges
				for (n = 0; n < max_nodes; n++) {					// check all nodes for 
					if (node[n].edgelist != NULL) {					// if this node has any edges
						edgepointer = node[n].edgelist;				// load the edgelist into a pointer
						while (edgepointer != NULL) {							// for the current pointer
							if (edgepointer->headnodeid = delnodeid) {			// if its head is the edge were deleting
								edge_del(n, edgepointer, 0);		// get rid of it or it'll lead to nowhere
							}
							
							edgepointer = edgepointer->next;		// do that for every edge
						}
						
					}
					
				}
				
			}
			
			if (node[delnodeid].edgelist != NULL) {					// if this node has outward edges
				edgepointer = node[delnodeid].edgelist;				// point to the first edge
				while (edgepointer->next != NULL) {					// until weve done this to the last edge
					prevedge = edgepointer;							// save the current edge
					edgepointer = edgepointer->next;				// step to the next edge
					returnval += edge_del(delnodeid, prevedge, 0);	// delete the previous edge without doing a toposort
				}
				
			}
			
			if (nodeid_free(delnodeid) != dead_nodeid) {			// free the node id
				returnval = 0;										// return successful
				mod_uninit(delnodeid);		 						// uninit the module
			} else {
				returnval = 9;										// freeing the nodeid failed
			}
			
			returnval += toposort();								// catch up with the toposort we skipped and return error if it fails
			
		} else {
		returnval = 8;
		}
		
	} else {
	returnval = 7;
	}
	
	return returnval;
}



unsigned char nodeid_free(unsigned char nodeid) {
	unsigned char arrayindex = nodeid/32;							// generate the array index
	unsigned char indexbit = nodeid%32;								// and bit
	
	if ((node[nodeid].indegree) < dead_indegree) {					// check that indegree != 0xff (marked dead)
		if (node_count > 0) {										// if there are nodes to remove
			if ((nodeidinuse[arrayindex]) & (1<<indexbit)) {		// if this nodes bit is set
				nodeidinuse[arrayindex] &= (~(1<<indexbit));		// clear it
				node_count--;										// decrement the node count
				return nodeid;										// and return the node we just removed
			} else {
				return dead_nodeid;									// error handling if this nodeid is free
			}
			
		} else {
			return dead_nodeid;										// error handling if there are no nodes
		}
		
	} else {
		return dead_nodeid;											// error handling if the nodes marked dead
	}
}





edge_t *edge_add(unsigned char tail_nodeid, unsigned char tail_moduleport, unsigned char head_nodeid, unsigned char head_moduleport) {
	edge_t *newedge = NULL;
	
	if (tail_nodeid != head_nodeid) {
		
		if (node[tail_nodeid].indegree < dead_indegree) {			// if tail node exists
			if (node[head_nodeid].indegree < max_indegree) {		// if head node exists and we aren't about to bust the maximum indegree
				if ((tail_moduleport <= mod_ports[(node[tail_nodeid].moduletype)]) && (head_moduleport <= mod_ports[(node[head_nodeid].moduletype)])) {
					edge_t *curredge = NULL;						// declare pointer to previous edge
					edge_t *newedge = pvPortMalloc(sizeof(edge_t));	// malloc a new edge_t, store pointer in temp var
					edge_t *edgepointer = node[tail_nodeid].edgelist; 			// load up the list header
					
					edgepointer = node[tail_nodeid].edgelist;
					if (edgepointer == NULL) {						// if there are no edges
						node[tail_nodeid].edgelist = newedge;		// store new edge pointer as list header
					} else {										// if there is at least one edge already
						while ((edgepointer->next) != NULL) {		// if there is more than one edge
							edgepointer = edgepointer->next;		// search until we find the last edge
						}											// otherwise stay with the first and only edge
							
						edgepointer->next = newedge;				// store new edge pointer in last edge
					}
					
					newedge->tailport = tail_moduleport;
					newedge->headnodeid = head_nodeid;
					newedge->headport = head_moduleport;
					
					(node[head_nodeid].indegree)++;					// increment the indegree of the head node
					
					if (toposort() != 0) {							// topo search will barf on a cycle
						if (edge_del(tail_nodeid, newedge, 1) != 0) {			// if it does delete the culprit and try again.
							return NULL;							// if it doesn't sort cleanly like before... something has gone very wrong
						}
						
						return NULL;								// nILS - when the user creates a cycle just pop up a message saying "Singular matrix. Fuck you."
					} else {
						node[tail_nodeid].process_req++;
						nodes_proc(tail_nodeid);					// if it's sorted ok, process from here down
						return newedge;								// and return the pointer to the new edge
					}
					
				} else {
					return NULL; 									// if the module ports are not available
				}
				
			} else {												// if the maximum indegree of the head node has been reached
				return NULL; 										// or head node does not exist
			}
			
		} else {													// if the maximum indegree of the tail node has been reached
			return NULL; 											// or tail node does not exist
		}
		
	} else {
		return NULL;												// Handle attempt to self-feedback
	}
	
	return newedge;
}



unsigned char edge_del(unsigned char tailnodeid, edge_t *deledge, unsigned char dosort) {
	edge_t *thisedge;
	edge_t *prevedge;

	if (deledge != NULL) {
		if (node[tailnodeid].indegree < dead_indegree) {			// handle dead tailnode 
			thisedge = node[tailnodeid].edgelist;
			if (thisedge == deledge) {								// if are we deleting the first (or only) edge
				node[tailnodeid].edgelist = deledge->next;			// fix the pointer to point at next edge (NULL if no next edge)
			} else {
				while (thisedge != deledge) {						// skip through the list
					prevedge = thisedge;							// keep track of this edge
					thisedge = thisedge->next;						// then advance to the next edge
				}													// repeat until we find the edge to be deleted and have its previous edge
				prevedge->next = deledge->next;						// otherwise fix the pointer on the previous edge
			}
			
			(node[(deledge->headnodeid)].indegree)--;				// decrement the indegree
			vPortFree(deledge);										// free up ram
			if (dosort > 0) {
				return toposort();									// redo the toposort if signalled to
			} else {
				return 0;											// otherwise just return successful
			}
			
		} else {
			return 6;
		}
		
	} else {
		return 5;
	}
	
}





void nodes_proc(unsigned char startnodeid){
	nodelist_t *topolist;
	unsigned char procnodeid;
	if (node_count > 0) {											// handle no nodes
		topolist = topolisthead;									// follow list of topo sorted nodeids
		if (topolist != NULL) {										// handle dead list
			//if (topolist->nodeid < dead_nodeid) {
			if (startnodeid < dead_nodeid) {						// if we are not processing from the root
				while ((topolist->nodeid) != startnodeid) {			// search for the start node
					topolist = topolist->next;						// through each entry in the topolist
				}													// then we'll process from the root of the topo list
				
			}														// or from 
			
			while (topolist != NULL) {								// for each entry in the topolist from there on
				procnodeid = topolist->nodeid;						// get the nodeid
				if (procnodeid < dead_nodeid) {
					if (node[procnodeid].process_req > 0) {			// only process nodes which request it
						mod_process(procnodeid);					// crunch the numbers according to function pointer
						
						mod_propagate(procnodeid);					// spread out the ports to the downstream nodes
						
						node[procnodeid].process_req = 0;			// clear the process request
					}
					
				}
				
				topolist = topolist->next;							// and move onto the next node in the sorted list
			}
			
			//}
			
		}
		
	}
	
}





unsigned char toposort(void) {
	unsigned char n;	
	unsigned char returnval = 0;
	unsigned char test_node_count = 0;
	unsigned char sorted_node_count = 0;
	unsigned char indegreezero_count = 0;
	
	nodelist_t *indegreezerohead = NULL;							// create a node list for nodes with indegree 0
	nodelist_t *indegreezerotail = NULL;							// prep the list
	nodelist_t *indegreezero = indegreezerohead;
	
	edge_t *edgepointer;
	
	nodelist_t *topolist;
	nodelist_t *topolisttail;
	
	topolist_clear();
	
	if (node_count > 0) {											// if there are active nodes to sort
		for (n = 0; n < max_nodes; n++) {							// for all the nodes
			if (node[n].indegree < dead_indegree) {					// if the nodes not dead
				if (node[n].indegree == 0) {						// if it has indegree 0
					indegreezero = indegreezerohead;
					indegreezerohead = pvPortMalloc(sizeof(nodelist_t));		// create a node list for nodes with indegree 0
					if (indegreezero_count == 0) indegreezerotail = indegreezerohead;			// and set the tail of the list to that entry. well use that later
					indegreezerohead->nodeid = n;
					indegreezerohead->next = indegreezero;
					indegreezero_count++;
				}
				
				node[n].indegree_uv = node[n].indegree;				// set the unvisited indegree to match the real indegree
				
				test_node_count++;
				
			}
			
		}
		
		if (test_node_count == node_count) {						// and if it matches the expected node count
			while (indegreezerohead != NULL) {  					// while there's anything in indegreezero list
				topolist = pvPortMalloc(sizeof(nodelist_t));
				if (sorted_node_count == 0) {
					topolisthead = topolist;
				} else {
					topolisttail->next = topolist;
				}
				
				topolist->nodeid = indegreezerohead->nodeid;		// copy this node out of indegreezero into topolist	
				topolist->next = NULL;								// prepare the topo list pointer for the next node to be sorted
				topolisttail = topolist;
				sorted_node_count++;								// count the sorted nodes
				
				
				edgepointer = node[(indegreezerohead->nodeid)].edgelist;		// load up the first edge for this node
				while (edgepointer != NULL) { 									// for each outward edge on this node
					if (--(node[(edgepointer->headnodeid)].indegree_uv) == 0) {	// visit the headnode, and if this is the last inward edge
						indegreezerotail->next = pvPortMalloc(sizeof(nodelist_t));			// add a new entry to the tail end of the indegreezero list
						indegreezerotail = indegreezerotail->next;				// fix the pointer to the tail end of the indegreezero list
						indegreezerotail->nodeid = edgepointer->headnodeid;		// add node id to the tail end of the indegreezero list
						indegreezerotail->next = NULL;							// init the new entry
						//indegreezero = indegreezero->next;		// prepare the pointer for the next item in the indegreezero list
					}
					edgepointer = edgepointer->next;
				}
				indegreezero = indegreezerohead;					// backup the pointer before we...
				indegreezerohead = indegreezerohead->next;			// then restore the pointer to the new head which cantains the next node with unvisited inward edges
				vPortFree(indegreezero);							// remove this node from the head of indegreezero
			}
			
			if (sorted_node_count != test_node_count) {
				topolist_clear();									// mark this topo list dead
				returnval = 4;										// they weren't all sorted so there's a cycle 
			}
			
		} else {
			topolist_clear();										// mark this topo list dead
			returnval = 2;											// handle mismatched node counts 
		}															// scanning all nodes found different number than reported by add_and node_del
		
	} else {
		topolist_clear();											// mark this topo list dead
		returnval = 1;												// handle node count 0
	}

	return returnval;												// if all went well, this is still 0
}


void topolist_clear(void) {
	nodelist_t *topolist;
	while (topolisthead != NULL) {							// lets play trash the topo list
		topolist = topolisthead;							// backup the pointer
		topolisthead = topolisthead->next;					// advance to the next entry
		vPortFree(topolist);								// free up the previous entry
	}														// rinse... repeat
	
}

// todo

//Fix return values
