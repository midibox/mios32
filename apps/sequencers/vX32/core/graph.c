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


#include "tasks.h"
#include "app.h"
#include "graph.h"
#include "mclock.h"
#include "modules.h"
#include "patterns.h"
#include "utils.h"
#include "ui.h"

#include <string.h>

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

node_t node[MAX_NODES];                                                         // array of sructs to hold node/module info

nodelist_t *topoListHead;                                                       // list of topologically sorted nodeIDs

unsigned char node_Count;                                                       // count of active nodes

unsigned char nodeIDInUse[NODEIDINUSE_BYTES];                                   // array for storing active nodes


/////////////////////////////////////////////////////////////////////////////
// local prototypes
/////////////////////////////////////////////////////////////////////////////

unsigned char NodeID_Gen(void);                                                 // returns the first available node ID

unsigned char NodeID_Free(unsigned char nodeID);                                // mark this node ID available



/////////////////////////////////////////////////////////////////////////////
// Initialise the graph
/////////////////////////////////////////////////////////////////////////////

void Graph_Init(void) {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX] Initialising graph\n");
#endif

    node_Count = 0;
    unsigned char n = 0;
    for (n = 0; n < NODEIDINUSE_BYTES; n++) {
        nodeIDInUse[n] = 0;
    }
    
    for (n = 0; n < MAX_NODES; n++) {                                           // init this array
        node[n].name[0] = 'E';                                                  // so that all nodes start off dead
        node[n].name[1] = 'M';
        node[n].name[2] = 'P';
        node[n].name[3] = 'T';
        node[n].name[4] = 'Y';
        node[n].name[5] = '\0';
        node[n].moduletype = DEAD_MODULETYPE;
        node[n].process_req = 0;
        node[n].indegree = DEAD_INDEGREE;
        node[n].indegree_uv = DEAD_INDEGREE;
        node[n].ports = NULL;
        node[n].privvars = NULL;
        node[n].edgelist = NULL;
        node[n].edgelist_in = NULL;
        
    }
    
    topoListHead = NULL;                                                        // make topo list not exist yet
    
    
}



/////////////////////////////////////////////////////////////////////////////
// Add a node
// in: module type in moduletype
// out: new node ID
/////////////////////////////////////////////////////////////////////////////

unsigned char Node_Add(unsigned char moduletype) {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][node] Adding new node with module type %d\n", moduletype);
#endif

    nodelist_t *topoinsert;
    unsigned char newnodeID;
    if (node_Count < MAX_NODES-1) {                                             // handle max nodes
        if (moduletype < MAX_MODULETYPES) {                                     // make sure the moduletype is legal
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX] Getting new node ID\n");
#endif
            newnodeID = NodeID_Gen();                                           // grab a new nodeID and save it
            if (newnodeID < MAX_NODES) {                                        // if no error grabbing the ID
#if vX_DEBUG_VERBOSE_LEVEL >= 3
        DEBUG_MSG("[vX] Initing graph for new node\n");
#endif
                topoinsert = topoListHead;                                      // save the topolist root
                topoListHead = pvPortMalloc(sizeof(nodelist_t));                // replace the topolist root with new space
                topoListHead->next = topoinsert;                                // restore the old root to the next pointer
                topoListHead->nodeID = newnodeID;                               // and insert the new node at the root
                Mod_Init_Graph(newnodeID, moduletype);                          // initialise the module hosted by this node
                node[newnodeID].process_req++;
                Mod_PreProcess(newnodeID);                                      // if it's sorted ok, process from here down
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][node] Done, new node ID is %d\n", newnodeID);
#endif
                return newnodeID;                                               // return the ID of the new node
            } else {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][node] Failed, now node ID invalid\n");
#endif

                return DEAD_NODEID;
            }
            
        } else {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][node] Failed, invalid module type\n");
#endif

            return DEAD_NODEID;
        }
        
    } else {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][node] Faild, no nodes available\n");
#endif

        return DEAD_NODEID;
    }
    
}


/////////////////////////////////////////////////////////////////////////////
// Generate a node ID
// needs to be freed again using NodeID_Free()
// out: new node ID
/////////////////////////////////////////////////////////////////////////////

unsigned char NodeID_Gen(void) {
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][nodeid] Generating new node ID\n");
#endif
    unsigned char indexbit;
    unsigned char arrayindex;
    unsigned char newnodeID;
    unsigned char a = NODEIDINUSE_BYTES+1;                                      // prep the error handler
    if (node_Count < (MAX_NODES-1)) {                                           // if were not full
        for (arrayindex = 0; arrayindex < NODEIDINUSE_BYTES; arrayindex++) {    // step through the array
            if (nodeIDInUse[arrayindex] != 0xff) {                              // until we find an available slot
                a = arrayindex;
                break;
            }
            
        }
        
        if (a == NODEIDINUSE_BYTES+1) {
            
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][nodeid] Failed, no free byte found\n");
#endif
            return DEAD_NODEID;
        }
        
#if vX_DEBUG_VERBOSE_LEVEL >= 3
        DEBUG_MSG("[vX][nodeid] Found available byte %d\n", a);
#endif
        
        if (a <= NODEIDINUSE_BYTES) {                                           // if in finds
            for (indexbit = 0; indexbit < 8; indexbit++) {                      // step through the byte
                if (!(nodeIDInUse[a] & (1<<indexbit))) {                        // if we find a 0
                    nodeIDInUse[a] |= (1<<indexbit);                            // set the bit
                    node_Count++;                                               // increment the node count
                    newnodeID = ((8*(a)) + (indexbit));
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][nodeid] Found free bit %d, returning new node ID %d\n", indexbit, newnodeID);
#endif
                    return newnodeID;                                           // and return the new nodeID
                }
                
            }
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][nodeid] Failed to find free bit. This should never happen\n");
#endif
            
        } else {                                                                // otherwise
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][nodeid] Available byte is invalid. This should never happen\n");
#endif
            return DEAD_NODEID;                                                 // nothing free do some error handling
        }
        
    } else {
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][nodeid] Failed, no more nodes available\n");
#endif

        return DEAD_NODEID;                                                     // error handling if were full
    }
    
        return DEAD_NODEID;                                                     // error handling if were full
}



/////////////////////////////////////////////////////////////////////////////
// Delete a node
// in: node ID to delete
// out: error code. 0 if successful.
/////////////////////////////////////////////////////////////////////////////

unsigned char Node_Del(unsigned char delnodeID) {
    unsigned char returnval = 0;                                                // prep error handler to return successful
    edge_t *edgepointer;
    edge_t *prevedge;
    
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][node] Deleting node %d\n", delnodeID);
#endif
    if (node_Count > 0) {                                                       // handle node_Count 0
        if (node[delnodeID].indegree < DEAD_INDEGREE) {                         // handle node dead indegree 0xff
            
            node[delnodeID].nexttick = DEAD_TIMESTAMP;                          // prepare timestamps
            node[delnodeID].downstreamtick = DEAD_TIMESTAMP;                    // for tickpriority
            node[delnodeID].status.deleting = 1;                                // flag node as being deleted now so that it will be ignored
            
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX] Deleting inward edges\n");
#endif
            if (node[delnodeID].indegree > 0) {                                 // if this node has any inward edges
                edgepointer = node[delnodeID].edgelist_in;                      // load the edgelist into a pointer
                while (edgepointer != NULL) {                                   // for the current pointer
                    prevedge = edgepointer;                                     // save the current edge
                    edgepointer = edgepointer->head_next;                       // step to the next edge
                    returnval += Edge_Del(prevedge, DONT_TOPOSORT);             // delete the previous edge without doing a TopoSort
                    
                }
                
            }
            
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX] Deleting outward edges\n");
#endif
            if (node[delnodeID].edgelist != NULL) {                             // if this node has outward edges
                edgepointer = node[delnodeID].edgelist;                         // point to the first edge
                while (edgepointer != NULL) {                                   // until weve done this to the last edge
                    prevedge = edgepointer;                                     // save the current edge
                    edgepointer = edgepointer->next;                            // step to the next edge
                    returnval += Edge_Del(prevedge, DONT_TOPOSORT);             // delete the previous edge without doing a TopoSort
                }
                
            }
            
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX] Freeing node ID\n");
#endif
            if (NodeID_Free(delnodeID) != DEAD_NODEID) {                        // free the node id
                Mod_UnInit_Graph(delnodeID);                                    // uninit the module
            } else {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][node] Failed, node ID could not be freed\n");
#endif
                returnval = 9;                                                  // freeing the nodeID failed
            }
            
            returnval += TopoSort();                                            // catch up with the TopoSort we skipped and return error if it fails
            Mod_PreProcess(DEAD_NODEID);                                        // and preprocess
            
        } else {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][node] Failed, node already marked dead\n");
#endif
        returnval = 8;
        }
        
    } else {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][node] Failed, global node count is 0\n");
#endif
    returnval = 7;
    }
    
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][node] Done, returning %d\n", returnval);
#endif
    return returnval;
}



/////////////////////////////////////////////////////////////////////////////
// Free a node ID
// in: node ID to free, as generated by NodeID_Gen()
// out: freed node ID
/////////////////////////////////////////////////////////////////////////////

unsigned char NodeID_Free(unsigned char nodeID) {
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][nodeid] Freeing node ID %d\n", nodeID);
#endif
    unsigned char arrayindex = nodeID/8;                                        // generate the array index
    unsigned char indexbit = nodeID%8;                                          // and bit
    
    if ((node[nodeID].indegree) < DEAD_INDEGREE) {                              // check that indegree != 0xff (marked dead)
        if (node_Count > 0) {                                                   // if there are nodes to remove
            if ((nodeIDInUse[arrayindex]) & (1<<indexbit)) {                    // if this nodes bit is set
                nodeIDInUse[arrayindex] &= (~(1<<indexbit));                    // clear it
                node_Count--;                                                   // decrement the node count
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][nodeid] Done, new node count %d\n", node_Count);
#endif
                return nodeID;                                                  // and return the node we just removed
            } else {
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][nodeid] Failed, bit %d of byte %d of node ID matrix is not set\n", indexbit, arrayindex);
#endif
                return DEAD_NODEID;                                             // error handling if this nodeID is free
            }
            
        } else {
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][nodeid] Failed, global node count is 0\n");
#endif
            return DEAD_NODEID;                                                 // error handling if there are no nodes
        }
        
    } else {
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][nodeid] Failed, node already marked dead\n");
#endif
        return DEAD_NODEID;                                                     // error handling if the nodes marked dead
    }
}




/////////////////////////////////////////////////////////////////////////////
// Set a node name
// in: node ID to set, string to copy (must be MODULE_NAME_STRING_LENGTH)
// out: pointer to name, NULL on failure
/////////////////////////////////////////////////////////////////////////////

char *Node_SetName(unsigned char nodeID, char *newname) {
    if (nodeID < DEAD_NODEID) {
        if (node[nodeID].indegree < DEAD_INDEGREE) {
            strncpy(node[nodeID].name, newname, MODULE_NAME_STRING_LENGTH);
            return node[nodeID].name;
        }
    }
    
    return NULL;
    
}



/////////////////////////////////////////////////////////////////////////////
// Find a node by name
// note that only 'length' of the input will be checked,
// up to a maximum of MODULE_NAME_STRING_LENGTH
// and only the first node in numerical (not topological) order is returned
// in: pointer to string, length to check
// out: node ID
/////////////////////////////////////////////////////////////////////////////

unsigned char Node_GetID(char *getname, unsigned char length) {
    length %= MODULE_NAME_STRING_LENGTH;
    unsigned char nodeid;
    for (nodeid = 0; nodeid < node_Count; nodeid++) {
        if (node[nodeid].indegree < DEAD_INDEGREE) {
            if (!(strncasecmp(node[nodeid].name, getname, length))) {
                return nodeid;
            }
            
        }
        
    }
    
    return DEAD_NODEID;
    
}



/////////////////////////////////////////////////////////////////////////////
// Find a node type by ID
// in: node ID
// out: node type
/////////////////////////////////////////////////////////////////////////////

unsigned char Node_GetType(unsigned char nodeID) {
    if (nodeID > DEAD_NODEID) return node[nodeID].moduletype;
    return DEAD_MODULETYPE;
}



/////////////////////////////////////////////////////////////////////////////
// Find a moduletype name by type
// in: moduletype
// out: name
/////////////////////////////////////////////////////////////////////////////

const char *Node_GetTypeName(unsigned char moduletype) {
    if (moduletype < DEAD_MODULETYPE) {
        return mod_ModuleData_Type[moduletype]->name;
    }
    
    return NULL;
    
}




/////////////////////////////////////////////////////////////////////////////
// Find a moduletype by name
// note that only 'length' of the input will be checked,
// up to a maximum of MODULE_NAME_STRING_LENGTH
// and only the first node in numerical (not topological) order is returned
// in: pointer to string, length to check
// out: moduletype
/////////////////////////////////////////////////////////////////////////////

unsigned char Node_GetTypeID(char *getname, unsigned char length) {
    length %= MODULE_NAME_STRING_LENGTH;
    unsigned char moduletype;
    for (moduletype = 0; moduletype < DEAD_MODULETYPE; moduletype++) {
        if (!(strncasecmp(mod_ModuleData_Type[moduletype]->name, getname, length))) {
            return moduletype;
        }
        
    }
    
    return DEAD_MODULETYPE;
    
}






/////////////////////////////////////////////////////////////////////////////
// Find a node name by ID
// in: node ID
// out: name
/////////////////////////////////////////////////////////////////////////////

char *Node_GetName(unsigned char nodeID) {
    if (nodeID > DEAD_NODEID) return node[nodeID].name;
    return NULL;
}



/////////////////////////////////////////////////////////////////////////////
// Add an edge
// in: tail node ID and port, head node ID and port
// out: pointer to new edge
/////////////////////////////////////////////////////////////////////////////

edge_t *Edge_Add(unsigned char tail_nodeID, unsigned char tail_port, unsigned char head_nodeID, unsigned char head_port) {
    edge_t *newedge = NULL;
    
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Adding edge from node %d port %d to node %d port %d\n", tail_nodeID, tail_port, head_nodeID, head_port);
#endif
    
    if (tail_nodeID != head_nodeID) {                                           // if we're not trying to loop back to the same module
        if ((node[tail_nodeID].indegree < DEAD_INDEGREE) ||
            (tail_nodeID == DEAD_NODEID)) {                       // if tail node exists
            if ((node[head_nodeID].indegree < MAX_INDEGREE) ||
                (head_nodeID == DEAD_NODEID)) {                    // if head node exists and we aren't about to bust the maximum indegree
                if ((tail_port < mod_Ports[(node[tail_nodeID].moduletype)]) 
                && (head_port < mod_Ports[(node[head_nodeID].moduletype)])) {   // if the ports are valid
                    
                    edge_t *newedge = pvPortMalloc(sizeof(edge_t));             // malloc a new edge_t, store pointer in temp var
                    edge_t *edgepointer = node[tail_nodeID].edgelist;           // load up the edge list header
                    
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX] Inserting edge into tail node edge list\n");
#endif
                    edgepointer = node[tail_nodeID].edgelist;
                    if (edgepointer == NULL) {                                  // if there are no edges
                        node[tail_nodeID].edgelist = newedge;                   // store new edge pointer as list header
                    } else {                                                    // if there is at least one edge already
                        while ((edgepointer->next) != NULL) {                   // if there is more than one edge
                            edgepointer = edgepointer->next;                    // search until we find the last edge
                        }                                                       // otherwise stay with the first and only edge
                        
                        edgepointer->next = newedge;                            // store new edge pointer in last edge
                    }
                    
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX] Inserting edge into head node edge list\n");
#endif
                    edgepointer = node[head_nodeID].edgelist_in;
                    if (edgepointer == NULL) {                                  // if there are no inward edges
                        node[head_nodeID].edgelist_in = newedge;                // store new edge pointer as list header
                    } else {                                                    // if there is at least one inward edge already
                        while ((edgepointer->head_next) != NULL) {              // if there is more than one edge
                            edgepointer = edgepointer->head_next;               // search until we find the last edge
                        }                                                       // otherwise stay with the first and only edge
                        
                        edgepointer->head_next = newedge;                       // store new edge pointer in last edge
                    }
                    
                    newedge->tailnodeID = tail_nodeID;                          // save data for this edge
                    newedge->tailport = tail_port;
                    newedge->headnodeID = head_nodeID;
                    newedge->headport = head_port;
                    newedge->msgxlate = Edge_Get_Xlator(tail_nodeID, tail_port, 
                                                        head_nodeID, head_port);
                    
                    (node[head_nodeID].indegree)++;                             // increment the indegree of the head node
                    
                    
                    if (newedge->msgxlate == NULL) {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Edge add failed, cannot be translated\n");
#endif
                        Edge_Del(newedge, DONT_TOPOSORT);                       // if this edge can't be translated, delete it
                        return NULL;
                    }
                    
                    
                    Mod_TickPriority(head_nodeID);                              // fix downstreamticks
                    
                    
                    if (TopoSort() != 0) {                                      // topo search will barf on a cycle
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Topo sort failed, deleting edge\n");
#endif
#if vX_DEBUG_VERBOSE_LEVEL >= 9
        DEBUG_MSG("[vX][edge] Singular matrix, fuck you! (coder humour, sorry)\n");
#endif
                        if (Edge_Del(newedge, DO_TOPOSORT) != 0) {              // if it does delete the culprit and try again.
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Topo sort failed again, this should never happen\n");
#endif
                            return NULL;                                        // if it doesn't sort cleanly like before... something has gone very wrong
                        }
                        
                        return NULL;                                            // nILS - "when the user creates a cycle just pop up a message saying "Singular matrix. Fuck you."" ...LOL!
                    } else {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Edge added successfully, marking nodes for preprocessing\n");
#endif
                        node[tail_nodeID].process_req++;
                        Mod_PreProcess(tail_nodeID);                            // if it's sorted ok, process from here down
                        return newedge;                                         // and return the pointer to the new edge
                    }
                    
                } else {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Edge add failed, invalid port\n");
#endif
                    return NULL;                                                // if the module ports are not available
                }
                
            } else {                                                            // if the maximum indegree of the head node has been reached
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Edge add failed, head node full or dead\n");
#endif
                return NULL;                                                    // or head node does not exist
            }
            
        } else {                                                                // if the maximum indegree of the tail node has been reached
        
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Edge add failed, tail node full or dead\n");
#endif
            return NULL;                                                        // or tail node does not exist
        }
        
    } else {
        
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Edge add failed, you're attempting to loop back to the same node\n");
#endif

        return NULL;                                                            // Handle attempt to self-feedback
    }
    
    return newedge;
}



/////////////////////////////////////////////////////////////////////////////
// Deleted an edge
// in: pointer to the edge, flag whether to topo sort or not. 
//   usually the dosort flag should be 1,
//   if it's not you must do the topo sort manually
// out: error code, 0 is success
/////////////////////////////////////////////////////////////////////////////

unsigned char Edge_Del(edge_t *deledge, unsigned char dosort) {
    edge_t *thisedge;
    edge_t *prevedge;
    unsigned char tailnodeID = deledge->tailnodeID;
    unsigned char headnodeID = deledge->headnodeID;
    
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Deleting an edge from node %d port %d", tailnodeID, deledge->tailport);
        DEBUG_MSG("[vX][edge] ... to node %d port %d\n", headnodeID, deledge->headport);
#endif
    
    if (tailnodeID < MAX_NODES){
        if (deledge != NULL) {
            if (node[tailnodeID].indegree < DEAD_INDEGREE) {                    // handle dead tailnode 
                
                thisedge = node[tailnodeID].edgelist;                           // fix the edge list on the tail node
                if (thisedge == deledge) {                                      // if are we deleting the first (or only) edge
                    node[tailnodeID].edgelist = deledge->next;                  // fix the pointer to point at next edge (NULL if no next edge)
                } else {
                    while (thisedge != deledge) {                               // skip through the list
                        prevedge = thisedge;                                    // keep track of this edge
                        thisedge = thisedge->next;                              // then advance to the next edge
                        if (thisedge == NULL) {                                 // bail out if we don't find the edge
                            
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Edge not found on tail node\n");
#endif
                            return 5;
                        }
                    }                                                           // repeat until we find the edge to be deleted and have its previous edge
                    prevedge->next = deledge->next;                             // otherwise fix the pointer on the previous edge
                }
                
                
                
                thisedge = node[headnodeID].edgelist_in;                        // fix the inward edge list on the head node
                if (thisedge == deledge) {                                      // if are we deleting the first (or only) edge
                    node[headnodeID].edgelist_in = deledge->head_next;          // fix the pointer to point at next edge (NULL if no next edge)
                } else {
                    while (thisedge != deledge) {                               // skip through the list
                        prevedge = thisedge;                                    // keep track of this edge
                        thisedge = thisedge->head_next;                         // then advance to the next edge
                        if (thisedge == NULL) {                                 // bail out if we don't find the edge
                            
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Edge not found on head node\n");
#endif
                            return 5;
                        }
                    }                                                           // repeat until we find the edge to be deleted and have its previous edge
                    prevedge->head_next = deledge->head_next;                   // otherwise fix the pointer on the previous edge
                }
                
                
                Mod_DeadPort((deledge->headnodeID), (deledge->headport));       // fill the head port with a dead value
                
                (node[(deledge->headnodeID)].indegree)--;                       // decrement the head node's indegree
                vPortFree(deledge);                                             // delete the edge
                
                
                Mod_TickPriority(tailnodeID);                                   // fix downstreamticks
                
                if (dosort > 0) {
                    return TopoSort();                                          // redo the TopoSort if signalled to
                    Mod_PreProcess(tailnodeID);                                 // and preprocess
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Delete successful, topo sort and preprocess done\n");
#endif
                } else {
                    return 0;                                                   // otherwise just return successful
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Delete successful, topo sort and preprocess required\n");
#endif
                    
                }
                
                
                
            } else {
                
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Delete failed, tailnode is marked dead\n");
#endif
                return 6;
            }
            
        } else {
            
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Delete failed, edge not specified\n");
#endif
            return 5;
        }
        
    } else {
        
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][edge] Delete failed, tail node does not exist\n");
#endif
        return 5;
    }
    
}


/////////////////////////////////////////////////////////////////////////////
// Get edge pointer
// in: tail node ID and port, head node ID and port
// out: pointer to the edge
/////////////////////////////////////////////////////////////////////////////

edge_t *Edge_GetID(unsigned char tail_nodeID, unsigned char tail_port
                        , unsigned char head_nodeID, unsigned char head_port) {

    if ((tail_nodeID < DEAD_NODEID) && (head_nodeID < DEAD_NODEID)) {
        edge_t *edgepointer = node[tail_nodeID].edgelist;                       // load up the edge list for the tail node
        while (edgepointer != NULL) {                                           // for each edge on this node
            if (edgepointer->headnodeID == head_nodeID) {
                if (edgepointer->headport == head_port) {
                    if (edgepointer->tailport == tail_port) {
                        return edgepointer;
                    }
                }
            }
            edgepointer = edgepointer->next;
        }
    }
    
    return NULL;
}



/////////////////////////////////////////////////////////////////////////////
// Returns the tail node that this edge connects from
// in: edge pointer
// out: tail node
/////////////////////////////////////////////////////////////////////////////

unsigned char Edge_GetTailNode(edge_t * edge) {
    if (edge != NULL) {
        unsigned char tempnodeID = edge->tailnodeID;
        if (node[tempnodeID].indegree < DEAD_INDEGREE) {
            return tempnodeID;
        }
        
    }
    
    return DEAD_NODEID;
    
}



/////////////////////////////////////////////////////////////////////////////
// Returns the head node that this edge connects to
// in: edge pointer
// out: head node
/////////////////////////////////////////////////////////////////////////////

unsigned char Edge_GetHeadNode(edge_t * edge) {
    if (edge != NULL) {
        unsigned char tempnodeID = edge->headnodeID;
        if (node[tempnodeID].indegree < DEAD_INDEGREE) {
            return tempnodeID;
        }
        
    }
    
    return DEAD_NODEID;
    
}



/////////////////////////////////////////////////////////////////////////////
// Returns the port of the tail node that this edge connects from
// in: edge pointer
// out: port of tail node
/////////////////////////////////////////////////////////////////////////////

unsigned char Edge_GetTailPort(edge_t * edge) {
    if (edge != NULL) {
        unsigned char tempnodeID = edge->tailnodeID;
        unsigned char tempport = edge->tailport;
        if (node[tempnodeID].indegree < DEAD_INDEGREE) {
            return tempport;
        }
        
    }
    
    return DEAD_PORTTYPE;
    
}



/////////////////////////////////////////////////////////////////////////////
// Returns the port of the head node that this edge connects to
// in: edge pointer
// out: port of head node
/////////////////////////////////////////////////////////////////////////////

unsigned char Edge_GetHeadPort(edge_t * edge) {
    if (edge != NULL) {
        unsigned char tempnodeID = edge->headnodeID;
        unsigned char tempport = edge->headport;
        if (node[tempnodeID].indegree < DEAD_INDEGREE) {
            return tempport;
        }
        
    }
    
    return DEAD_PORTTYPE;
    
}



/////////////////////////////////////////////////////////////////////////////
// Get edge translator function
// in: tail node ID and port, head node ID and port
// out: function pointer to the appropriate function for translation
//      between the port types input
/////////////////////////////////////////////////////////////////////////////

void (*Edge_Get_Xlator(unsigned char tail_nodeID, unsigned char tail_port
                    , unsigned char head_nodeID, unsigned char head_port)) 
                    (unsigned char tail_nodeID, unsigned char tail_port
                    , unsigned char head_nodeID, unsigned char head_port) {
    
    unsigned char tail_port_type = Mod_GetPortType(tail_nodeID, tail_port);   // get the tail port type
    unsigned char head_port_type = Mod_GetPortType(head_nodeID, head_port);   // get the head port type
    if (tail_port_type == DEAD_PORTTYPE) return NULL;                           // bail out if the ports are dead
    if (head_port_type == DEAD_PORTTYPE) return NULL;
    return mod_Xlate_Table[tail_port_type][head_port_type];                     // otherwise return the pointer
}



/////////////////////////////////////////////////////////////////////////////
// Topological sort
// out: error code. 0 is good.
/////////////////////////////////////////////////////////////////////////////

unsigned char TopoSort(void) {
    unsigned char n;
    unsigned char returnval = 0;
    unsigned char test_node_Count = 0;
    unsigned char sorted_node_Count = 0;
    unsigned char indegreezero_count = 0;
    
    nodelist_t *indegreezerohead = NULL;                                        // create a node list for nodes with indegree 0
    nodelist_t *indegreezerotail = NULL;                                        // prep the list
    nodelist_t *indegreezero = indegreezerohead;
    
    edge_t *edgepointer;
    
    nodelist_t *topolist;
    nodelist_t *topolisttail = NULL;
    
    TopoList_Clear();
    
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][topo] Starting Topological Sort\n");
#endif

    if (node_Count > 0) {                                                       // if there are active nodes to sort
        for (n = 0; n < MAX_NODES; n++) {                                       // for all the nodes
            if (node[n].indegree < DEAD_INDEGREE) {                             // if the nodes not dead
#if vX_DEBUG_VERBOSE_LEVEL >= 3
        DEBUG_MSG("[vX] Found live node %d\n", n);
#endif
                if (node[n].indegree == 0) {                                    // if it has indegree 0
#if vX_DEBUG_VERBOSE_LEVEL >= 3
        DEBUG_MSG("[vX] It is a root node\n");
#endif
                    indegreezero = indegreezerohead;
                    indegreezerohead = pvPortMalloc(sizeof(nodelist_t));        // create a node list for nodes with indegree 0
                    if (indegreezero_count == 0) 
                    indegreezerotail = indegreezerohead;                        // and set the tail of the list to that entry. well use that later
                    indegreezerohead->nodeID = n;
                    indegreezerohead->next = indegreezero;
                    indegreezero_count++;
                }
                
                node[n].indegree_uv = node[n].indegree;                         // set the unvisited indegree to match the real indegree
                
                test_node_Count++;
                
            }
            
        }
        
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX] Scanned all nodes. Total count %d, Root node count %d\n", test_node_Count, indegreezero_count);
#endif
        
        if (test_node_Count == node_Count) {                                    // and if it matches the expected node count
            while (indegreezerohead != NULL) {                                  // while there's anything in indegreezero list
                topolist = pvPortMalloc(sizeof(nodelist_t));
                if (sorted_node_Count == 0) {
                    topoListHead = topolist;
                } else {
                    topolisttail->next = topolist;
                }
                
                topolist->nodeID = indegreezerohead->nodeID;                    // copy this node out of indegreezero into topolist 
                topolist->next = NULL;                                          // prepare the topo list pointer for the next node to be sorted
                topolisttail = topolist;
                sorted_node_Count++;                                            // count the sorted nodes
                
#if vX_DEBUG_VERBOSE_LEVEL >= 3
        DEBUG_MSG("[vX] Added node %d to list\n", indegreezerohead->nodeID);
#endif
                
                edgepointer = node[(indegreezerohead->nodeID)].edgelist;        // load up the first edge for this node
                while (edgepointer != NULL) {                                   // for each outward edge on this node
                    if (--(node[(edgepointer->headnodeID)].indegree_uv) == 0) { // visit the headnode, and if this is the last inward edge
                        indegreezerotail->next = 
                                    pvPortMalloc(sizeof(nodelist_t));           // add a new entry to the tail end of the indegreezero list
                        indegreezerotail = indegreezerotail->next;              // fix the pointer to the tail end of the indegreezero list
                        indegreezerotail->nodeID = edgepointer->headnodeID;     // add node id to the tail end of the indegreezero list
                        indegreezerotail->next = NULL;                          // init the new entry
                        
                    }
                    edgepointer = edgepointer->next;
                }
                indegreezero = indegreezerohead;                                // backup the pointer before we...
                indegreezerohead = indegreezerohead->next;                      // then restore the pointer to the new head which cantains the next node with unvisited inward edges
                vPortFree(indegreezero);                                        // remove this node from the head of indegreezero
            }
            
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX] Sorted %d nodes\n", sorted_node_Count);
#endif
            
            if (sorted_node_Count != test_node_Count) {
            
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][topo] Sort failed, Sorted nodes does not match found node count. Clearing list\n");
#endif
            
                TopoList_Clear();                                               // mark this topo list dead
                returnval = 4;                                                  // they weren't all sorted so there's a cycle 
            }
            
        } else {
            
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][topo] Sort failed, Found nodes does not match global count. Clearing list\n");
#endif
            TopoList_Clear();                                                   // mark this topo list dead
            returnval = 2;                                                      // handle mismatched node counts 
        }                                                                       // scanning all nodes found different number than reported by add_and Node_Del
        
    } else {
        
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][topo] Sort failed, nothing to sort. Clearing list\n");
#endif
        TopoList_Clear();                                                       // mark this topo list dead
        returnval = 1;                                                          // handle node count 0
    }
    
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][topo] Topological Sort Complete\n");
#endif
    
    return returnval;                                                           // if all went well, this is still 0
}



/////////////////////////////////////////////////////////////////////////////
// Topological sort list clear
/////////////////////////////////////////////////////////////////////////////

void TopoList_Clear(void) {
    
#if vX_DEBUG_VERBOSE_LEVEL >= 2
        DEBUG_MSG("[vX][topo] Clearing Topological Sort List\n");
#endif
    nodelist_t *topolist;
    while (topoListHead != NULL) {                                              // lets play trash the topo list
        topolist = topoListHead;                                                // backup the pointer
        topoListHead = topoListHead->next;                                      // advance to the next entry
        vPortFree(topolist);                                                    // free up the previous entry
    }                                                                           // rinse... repeat
    
}

// todo

// better memory allocation is a must... or is it? heheheh
