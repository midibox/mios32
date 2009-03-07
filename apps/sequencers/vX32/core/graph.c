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



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

node_t node[max_nodes];															// array of sructs to hold node/module info

nodelist_t *topolisthead;														// list of topologically sorted nodeids

unsigned char node_count;														// count of active nodes

unsigned char nodeidinuse[nodeidinuse_bytes];									// array for storing active nodes


/////////////////////////////////////////////////////////////////////////////
// local prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Initialise the graph
/////////////////////////////////////////////////////////////////////////////
void graph_init(void) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Initialising graph\n");
#endif

	node_count = 0;
	unsigned char n = 0;
	for (n = 0; n < nodeidinuse_bytes; n++) {
		nodeidinuse[n] = 0;
	}
	
	for (n = 0; n < max_nodes; n++) {											// init this array
		node[n].name[0] = 'E';													// so that all nodes start off dead
		node[n].name[1] = 'M';
		node[n].name[2] = 'P';
		node[n].name[3] = 'T';
		node[n].name[4] = 'Y';
		node[n].name[5] = '\0';
		node[n].moduletype = dead_moduletype;
		node[n].process_req = 0;
		node[n].indegree = dead_indegree;
		node[n].indegree_uv = dead_indegree;
		node[n].ports = NULL;
		node[n].privvars = NULL;
		node[n].edgelist = NULL;
		node[n].edgelist_in = NULL;
		
	}
	
	topolisthead = NULL;														// make topo list not exist yet
}



/////////////////////////////////////////////////////////////////////////////
// Add a node
// in: module type in moduletype
/////////////////////////////////////////////////////////////////////////////

unsigned char node_add(unsigned char moduletype) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Adding new node with module type %d\n", moduletype);
#endif

	nodelist_t *topoinsert;
	unsigned char newnodeid;
	if (node_count < max_nodes-1) {												// handle max nodes
		if (moduletype < max_moduletypes) {										// make sure the moduletype is legal
#if DEBUG_VERBOSE_LEVEL >= 2
		DEBUG_MSG("[vX] Getting new node ID\n");
#endif
			newnodeid = nodeid_gen();											// grab a new nodeid and save it
			if (newnodeid < max_nodes) {										// if no error grabbing the ID
#if DEBUG_VERBOSE_LEVEL >= 2
		DEBUG_MSG("[vX] Initing graph for new node\n");
#endif
				topoinsert = topolisthead;										// save the topolist root
				topolisthead = pvPortMalloc(sizeof(nodelist_t));				// replace the topolist root with new space
				topolisthead->next = topoinsert;								// restore the old root to the next pointer
				topolisthead->nodeid = newnodeid;								// and insert the new node at the root
				mod_init_graph(newnodeid, moduletype);							// initialise the module hosted by this node
				node[newnodeid].process_req++;
				mod_preprocess(newnodeid);										// if it's sorted ok, process from here down
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Done, new node ID is %d\n", newnodeid);
#endif
				return newnodeid;												// return the ID of the new node
			} else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Failed, now node ID invalid\n");
#endif

				return dead_nodeid;
			}
			
		} else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Failed, invalid module type\n");
#endif

			return dead_nodeid;
		}
		
	} else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Faild, no nodes available\n");
#endif

		return dead_nodeid;
	}
	
}


/////////////////////////////////////////////////////////////////////////////
// Generate a node ID
// out: new node ID
/////////////////////////////////////////////////////////////////////////////

unsigned char nodeid_gen(void) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Generating new node ID\n");
#endif
	unsigned char indexbit;
	unsigned char arrayindex;
	unsigned char newnodeid;
	unsigned char a = nodeidinuse_bytes+1;										// prep the error handler
	if (node_count < (max_nodes-1)) {											// if were not full
		for (arrayindex = 0; arrayindex < nodeidinuse_bytes; arrayindex++) {	// step through the array
			if (nodeidinuse[arrayindex] != 0xff) {								// until we find an available slot
				a = arrayindex;
				break;
			}
			
		}
		
		if (a == nodeidinuse_bytes+1) {
			
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Failed, no free byte found\n");
#endif
			return dead_nodeid;
		}
		
#if DEBUG_VERBOSE_LEVEL >= 2
		DEBUG_MSG("[vX] Found available byte %d\n", a);
#endif
		
		if (a <= nodeidinuse_bytes) {											// if in finds
			for (indexbit = 0; indexbit < 8; indexbit++) {						// step through the byte
				if (!(nodeidinuse[a] & (1<<indexbit))) {						// if we find a 0
					nodeidinuse[a] |= (1<<indexbit);							// set the bit
					node_count++;												// increment the node count
					newnodeid = ((8*(a)) + (indexbit));
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Found free bit %d, returning new node ID %d\n", indexbit, newnodeid);
#endif
					return newnodeid;											// and return the new nodeid
				}
				
			}
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Failed to find free bit. This should never happen\n");
#endif
			
		} else {																// otherwise
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Available byte is invalid. This should never happen\n");
#endif
			return dead_nodeid;													// nothing free do some error handling
		}
		
	} else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Failed, no more nodes available\n");
#endif

		return dead_nodeid;														// error handling if were full
	}
}



/////////////////////////////////////////////////////////////////////////////
// Delete a node
// in: node ID to delete
/////////////////////////////////////////////////////////////////////////////

unsigned char node_del(unsigned char delnodeid) {
	unsigned char returnval = 0;												// prep error handler to return successful
	edge_t *edgepointer;
	edge_t *prevedge;
	
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Deleting node %d\n", delnodeid);
#endif
	if (node_count > 0) {														// handle node_count 0
		if (node[delnodeid].indegree < dead_indegree) {							// handle node dead indegree 0xff
			
			node[delnodeid].nexttick = dead_timestamp;							// prepare timestamps
			node[delnodeid].downstreamtick = dead_timestamp;					// for tickpriority
			node[delnodeid].deleting = 1;										// flag node as being deleted now so that it will be ignored
			
#if DEBUG_VERBOSE_LEVEL >= 2
		DEBUG_MSG("[vX] Deleting inward edges\n");
#endif
			if (node[delnodeid].indegree > 0) {									// if this node has any inward edges
				edgepointer = node[delnodeid].edgelist_in;						// load the edgelist into a pointer
				while (edgepointer != NULL) {									// for the current pointer
					prevedge = edgepointer;										// save the current edge
					edgepointer = edgepointer->head_next;						// step to the next edge
					returnval += edge_del(prevedge, 0);							// delete the previous edge without doing a toposort
					
				}
				
			}
			
#if DEBUG_VERBOSE_LEVEL >= 2
		DEBUG_MSG("[vX] Deleting outward edges\n");
#endif
			if (node[delnodeid].edgelist != NULL) {								// if this node has outward edges
				edgepointer = node[delnodeid].edgelist;							// point to the first edge
				while (edgepointer != NULL) {									// until weve done this to the last edge
					prevedge = edgepointer;										// save the current edge
					edgepointer = edgepointer->next;							// step to the next edge
					returnval += edge_del(prevedge, 0);							// delete the previous edge without doing a toposort
				}
				
			}
			
#if DEBUG_VERBOSE_LEVEL >= 2
		DEBUG_MSG("[vX] Freeing node ID\n");
#endif
			if (nodeid_free(delnodeid) != dead_nodeid) {						// free the node id
				mod_uninit_graph(delnodeid);									// uninit the module
			} else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Failed, node ID could not be freed\n");
#endif
				returnval = 9;													// freeing the nodeid failed
			}
			
			returnval += toposort();											// catch up with the toposort we skipped and return error if it fails
			mod_preprocess(dead_nodeid);										// and preprocess
			
		} else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Failed, node already marked dead\n");
#endif
		returnval = 8;
		}
		
	} else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Failed, global node count is 0\n");
#endif
	returnval = 7;
	}
	
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Done, returning %d\n", returnval);
#endif
	return returnval;
}



/////////////////////////////////////////////////////////////////////////////
// Free a node ID
// out: freed node ID
/////////////////////////////////////////////////////////////////////////////

unsigned char nodeid_free(unsigned char nodeid) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Freeing node ID %d\n", nodeid);
#endif
	unsigned char arrayindex = nodeid/8;										// generate the array index
	unsigned char indexbit = nodeid%8;											// and bit
	
	if ((node[nodeid].indegree) < dead_indegree) {								// check that indegree != 0xff (marked dead)
		if (node_count > 0) {													// if there are nodes to remove
			if ((nodeidinuse[arrayindex]) & (1<<indexbit)) {					// if this nodes bit is set
				nodeidinuse[arrayindex] &= (~(1<<indexbit));					// clear it
				node_count--;													// decrement the node count
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Done, new node count %d\n", node_count);
#endif
				return nodeid;													// and return the node we just removed
			} else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Failed, bit %d of byte %d of node ID matrix is not set\n", indexbit, arrayindex);
#endif
				return dead_nodeid;												// error handling if this nodeid is free
			}
			
		} else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Failed, global node count is 0\n");
#endif
			return dead_nodeid;													// error handling if there are no nodes
		}
		
	} else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Failed, node already marked dead\n");
#endif
		return dead_nodeid;														// error handling if the nodes marked dead
	}
}



/////////////////////////////////////////////////////////////////////////////
// Add an edge
// in: tail node ID and port, head node ID and port
/////////////////////////////////////////////////////////////////////////////

edge_t *edge_add(unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port) {
	edge_t *newedge = NULL;
	
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Adding edge from node %d port %d to node %d port %d\n", tail_nodeid, tail_port, head_nodeid, head_port);
#endif
	
	if (tail_nodeid != head_nodeid) {											// if we're not trying to loop back to the same module
		if (node[tail_nodeid].indegree < dead_indegree) {						// if tail node exists
			if (node[head_nodeid].indegree < max_indegree) {					// if head node exists and we aren't about to bust the maximum indegree
				if ((tail_port < mod_ports[(node[tail_nodeid].moduletype)]) 
				&& (head_port < mod_ports[(node[head_nodeid].moduletype)])) { 	// if the ports are valid
					
					edge_t *curredge = NULL;									// declare pointer to current edge
					edge_t *newedge = pvPortMalloc(sizeof(edge_t));				// malloc a new edge_t, store pointer in temp var
					edge_t *edgepointer = node[tail_nodeid].edgelist; 			// load up the edge list header
					
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Inserting edge into tail node edge list\n");
#endif
					edgepointer = node[tail_nodeid].edgelist;
					if (edgepointer == NULL) {									// if there are no edges
						node[tail_nodeid].edgelist = newedge;					// store new edge pointer as list header
					} else {													// if there is at least one edge already
						while ((edgepointer->next) != NULL) {					// if there is more than one edge
							edgepointer = edgepointer->next;					// search until we find the last edge
						}														// otherwise stay with the first and only edge
						
						edgepointer->next = newedge;							// store new edge pointer in last edge
					}
					
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Inserting edge into head node edge list\n");
#endif
					edgepointer = node[head_nodeid].edgelist_in;
					if (edgepointer == NULL) {									// if there are no inward edges
						node[head_nodeid].edgelist_in = newedge;				// store new edge pointer as list header
					} else {													// if there is at least one inward edge already
						while ((edgepointer->head_next) != NULL) {				// if there is more than one edge
							edgepointer = edgepointer->head_next;				// search until we find the last edge
						}														// otherwise stay with the first and only edge
						
						edgepointer->head_next = newedge;						// store new edge pointer in last edge
					}
					
					newedge->tailnodeid = tail_nodeid;							// save data for this edge
					newedge->tailport = tail_port;
					newedge->headnodeid = head_nodeid;
					newedge->headport = head_port;
					newedge->msgxlate = edge_get_xlator(tail_nodeid, tail_port, 
														head_nodeid, head_port);
					
					(node[head_nodeid].indegree)++;								// increment the indegree of the head node
					
					
					if (newedge->msgxlate == NULL) {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Edge add failed, cannot be translated\n");
#endif
						edge_del(newedge, 0);									// if this edge can't be translated, delete it
						return NULL;
					}
					
					
					mod_tickpriority(tail_nodeid);								// fix downstreamticks
					
					
					if (toposort() != 0) {										// topo search will barf on a cycle
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Topo sort failed, deleting edge\n");
#endif
						if (edge_del(newedge, 1) != 0) {						// if it does delete the culprit and try again.
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Topo sort failed again, this should never happen\n");
#endif
							return NULL;										// if it doesn't sort cleanly like before... something has gone very wrong
						}
						
						return NULL;											// nILS - "when the user creates a cycle just pop up a message saying "Singular matrix. Fuck you."" ...LOL!
					} else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Edge added successfully, marking nodes for preprocessing\n");
#endif
						node[tail_nodeid].process_req++;
						mod_preprocess(tail_nodeid);							// if it's sorted ok, process from here down
						return newedge;											// and return the pointer to the new edge
					}
					
				} else {
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Edge add failed, invalid port\n");
#endif
					return NULL; 												// if the module ports are not available
				}
				
			} else {															// if the maximum indegree of the head node has been reached
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Edge add failed, head node full or dead\n");
#endif
				return NULL; 													// or head node does not exist
			}
			
		} else {																// if the maximum indegree of the tail node has been reached
		
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Edge add failed, tail node full or dead\n");
#endif
			return NULL; 														// or tail node does not exist
		}
		
	} else {
		
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Edge add failed, you're attempting to loop back to the same node\n");
#endif

		return NULL;															// Handle attempt to self-feedback
	}
	
	return newedge;
}



/////////////////////////////////////////////////////////////////////////////
// Deleted an edge
// in: pointer to the edge, flag whether to topo sort or not. 
//   usually the dosort flag should be 1,
//   if it's not you must do the topo sort manually
/////////////////////////////////////////////////////////////////////////////

unsigned char edge_del(edge_t *deledge, unsigned char dosort) {
	edge_t *thisedge;
	edge_t *prevedge;
	unsigned char tailnodeid = deledge->tailnodeid;
	unsigned char headnodeid = deledge->headnodeid;
	
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Deleting an edge from node %d port %d", tailnodeid, deledge->tailport);
		DEBUG_MSG("[vX] ... to node %d port %d\n", headnodeid, deledge->headport);
#endif
	
	if (tailnodeid < max_nodes){
		if (deledge != NULL) {
			if (node[tailnodeid].indegree < dead_indegree) {					// handle dead tailnode 
				
				thisedge = node[tailnodeid].edgelist;							// fix the edge list on the tail node
				if (thisedge == deledge) {										// if are we deleting the first (or only) edge
					node[tailnodeid].edgelist = deledge->next;					// fix the pointer to point at next edge (NULL if no next edge)
				} else {
					while (thisedge != deledge) {								// skip through the list
						prevedge = thisedge;									// keep track of this edge
						thisedge = thisedge->next;								// then advance to the next edge
						if (thisedge == NULL) {									// bail out if we don't find the edge
							
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Edge not found on tail node\n");
#endif
							return 5;
						}
					}															// repeat until we find the edge to be deleted and have its previous edge
					prevedge->next = deledge->next;								// otherwise fix the pointer on the previous edge
				}
				
				
				
				thisedge = node[headnodeid].edgelist_in;						// fix the inward edge list on the head node
				if (thisedge == deledge) {										// if are we deleting the first (or only) edge
					node[headnodeid].edgelist_in = deledge->head_next;			// fix the pointer to point at next edge (NULL if no next edge)
				} else {
					while (thisedge != deledge) {								// skip through the list
						prevedge = thisedge;									// keep track of this edge
						thisedge = thisedge->head_next;							// then advance to the next edge
						if (thisedge == NULL) {									// bail out if we don't find the edge
							
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Edge not found on head node\n");
#endif
							return 5;
						}
					}															// repeat until we find the edge to be deleted and have its previous edge
					prevedge->head_next = deledge->head_next;					// otherwise fix the pointer on the previous edge
				}
				
				
				
				unsigned char head_port_type = 
				mod_get_port_type((deledge->headnodeid), (deledge->headport));	// Find out what port type the head port is
				
				(*mod_deadport_table[head_port_type])
				((deledge->headnodeid), (deledge->headport));					// and fill it with a dead value
				
				(node[(deledge->headnodeid)].indegree)--;						// decrement the head node's indegree
				vPortFree(deledge);												// delete the edge
				
				
				mod_tickpriority(tailnodeid);									// fix downstreamticks
				
				if (dosort > 0) {
					return toposort();											// redo the toposort if signalled to
					mod_preprocess(tailnodeid);									// and preprocess
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Delete successful, topo sort and preprocess done\n");
#endif
				} else {
					return 0;													// otherwise just return successful
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Delete successful, topo sort and preprocess required\n");
#endif
					
				}
				
				
				
			} else {
				
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Delete failed, tailnode is marked dead\n");
#endif
				return 6;
			}
			
		} else {
			
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Delete failed, edge not specified\n");
#endif
			return 5;
		}
		
	} else {
		
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Delete failed, tail node does not exist\n");
#endif
		return 5;
	}
	
}



/////////////////////////////////////////////////////////////////////////////
// Get edge translator function
// in: tail node ID and port, head node ID and port
// out: function pointer to the appropriate function for translation
//      between the port types input
/////////////////////////////////////////////////////////////////////////////

void (*edge_get_xlator(unsigned char tail_nodeid, unsigned char tail_port
					, unsigned char head_nodeid, unsigned char head_port)) 
					(unsigned char tail_nodeid, unsigned char tail_port
					, unsigned char head_nodeid, unsigned char head_port) {
	
	unsigned char tail_port_type = mod_get_port_type(tail_nodeid, tail_port);	// get the tail port type
	unsigned char head_port_type = mod_get_port_type(head_nodeid, head_port);	// get the head port type
	if (tail_port_type == dead_porttype) return NULL;							// bail out if the ports are dead
	if (head_port_type == dead_porttype) return NULL;
	return mod_xlate_table[((tail_port_type*max_porttypes)+head_port_type)];	// otherwise return the pointer
}



/////////////////////////////////////////////////////////////////////////////
// Topological sort
// out: error code. 0 is good.
/////////////////////////////////////////////////////////////////////////////

unsigned char toposort(void) {
	unsigned char n;
	unsigned char returnval = 0;
	unsigned char test_node_count = 0;
	unsigned char sorted_node_count = 0;
	unsigned char indegreezero_count = 0;
	
	nodelist_t *indegreezerohead = NULL;										// create a node list for nodes with indegree 0
	nodelist_t *indegreezerotail = NULL;										// prep the list
	nodelist_t *indegreezero = indegreezerohead;
	
	edge_t *edgepointer;
	
	nodelist_t *topolist;
	nodelist_t *topolisttail;
	
	topolist_clear();
	
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Starting Topological Sort\n");
#endif

	if (node_count > 0) {														// if there are active nodes to sort
		for (n = 0; n < max_nodes; n++) {										// for all the nodes
			if (node[n].indegree < dead_indegree) {								// if the nodes not dead
#if DEBUG_VERBOSE_LEVEL >= 3
		DEBUG_MSG("[vX] Found live node %d\n", n);
#endif
				if (node[n].indegree == 0) {									// if it has indegree 0
#if DEBUG_VERBOSE_LEVEL >= 3
		DEBUG_MSG("[vX] It is a root node\n");
#endif
					indegreezero = indegreezerohead;
					indegreezerohead = pvPortMalloc(sizeof(nodelist_t));		// create a node list for nodes with indegree 0
					if (indegreezero_count == 0) 
					indegreezerotail = indegreezerohead;						// and set the tail of the list to that entry. well use that later
					indegreezerohead->nodeid = n;
					indegreezerohead->next = indegreezero;
					indegreezero_count++;
				}
				
				node[n].indegree_uv = node[n].indegree;							// set the unvisited indegree to match the real indegree
				
				test_node_count++;
				
			}
			
		}
		
#if DEBUG_VERBOSE_LEVEL >= 2
		DEBUG_MSG("[vX] Scanned all nodes. Total count %d, Root node count %d\n", test_node_count, indegreezero_count);
#endif
		
		if (test_node_count == node_count) {									// and if it matches the expected node count
			while (indegreezerohead != NULL) {									// while there's anything in indegreezero list
				topolist = pvPortMalloc(sizeof(nodelist_t));
				if (sorted_node_count == 0) {
					topolisthead = topolist;
				} else {
					topolisttail->next = topolist;
				}
				
				topolist->nodeid = indegreezerohead->nodeid;					// copy this node out of indegreezero into topolist	
				topolist->next = NULL;											// prepare the topo list pointer for the next node to be sorted
				topolisttail = topolist;
				sorted_node_count++;											// count the sorted nodes
				
#if DEBUG_VERBOSE_LEVEL >= 3
		DEBUG_MSG("[vX] Added node %d to list\n", indegreezerohead->nodeid);
#endif
				
				edgepointer = node[(indegreezerohead->nodeid)].edgelist;		// load up the first edge for this node
				while (edgepointer != NULL) { 									// for each outward edge on this node
					if (--(node[(edgepointer->headnodeid)].indegree_uv) == 0) {	// visit the headnode, and if this is the last inward edge
						indegreezerotail->next = 
									pvPortMalloc(sizeof(nodelist_t));			// add a new entry to the tail end of the indegreezero list
						indegreezerotail = indegreezerotail->next;				// fix the pointer to the tail end of the indegreezero list
						indegreezerotail->nodeid = edgepointer->headnodeid;		// add node id to the tail end of the indegreezero list
						indegreezerotail->next = NULL;							// init the new entry
						
					}
					edgepointer = edgepointer->next;
				}
				indegreezero = indegreezerohead;								// backup the pointer before we...
				indegreezerohead = indegreezerohead->next;						// then restore the pointer to the new head which cantains the next node with unvisited inward edges
				vPortFree(indegreezero);										// remove this node from the head of indegreezero
			}
			
#if DEBUG_VERBOSE_LEVEL >= 2
		DEBUG_MSG("[vX] Sorted %d nodes\n", sorted_node_count);
#endif
			
			if (sorted_node_count != test_node_count) {
			
#if DEBUG_VERBOSE_LEVEL >= 2
		DEBUG_MSG("[vX] Sort failed, Sorted nodes does not match found node count. Clearing list\n");
#endif
			
				topolist_clear();												// mark this topo list dead
				returnval = 4;													// they weren't all sorted so there's a cycle 
			}
			
		} else {
			
#if DEBUG_VERBOSE_LEVEL >= 2
		DEBUG_MSG("[vX] Sort failed, Found nodes does not match global count. Clearing list\n");
#endif
			topolist_clear();													// mark this topo list dead
			returnval = 2;														// handle mismatched node counts 
		}																		// scanning all nodes found different number than reported by add_and node_del
		
	} else {
		
#if DEBUG_VERBOSE_LEVEL >= 2
		DEBUG_MSG("[vX] Sort failed, nothing to sort. Clearing list\n");
#endif
		topolist_clear();														// mark this topo list dead
		returnval = 1;															// handle node count 0
	}
	
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Topological Sort Complete\n");
#endif
	
	return returnval;															// if all went well, this is still 0
}



/////////////////////////////////////////////////////////////////////////////
// Topological sort list clear
/////////////////////////////////////////////////////////////////////////////

void topolist_clear(void) {
	
#if DEBUG_VERBOSE_LEVEL >= 1
		DEBUG_MSG("[vX] Clearing Topological Sort List\n");
#endif
	nodelist_t *topolist;
	while (topolisthead != NULL) {												// lets play trash the topo list
		topolist = topolisthead;												// backup the pointer
		topolisthead = topolisthead->next;										// advance to the next entry
		vPortFree(topolist);													// free up the previous entry
	}																			// rinse... repeat
	
}

// todo

// better memory allocation is a must
// Fix return values
