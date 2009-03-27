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

#include <seq_midi_out.h>



/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

mod_moduledata_t *mod_ModuleData_Type[MAX_MODULETYPES] = {
    &mod_SClk_ModuleData,                                                       // moduletype 0
    &mod_Seq_ModuleData,                                                        // moduletype 1
    &mod_MIDIOut_ModuleData,                                                    // moduletype 2
    &mod_SxH_ModuleData,                                                        // moduletype 3
};




void (*mod_Init_Type[MAX_MODULETYPES]) (unsigned char nodeID);                  // array of init functions for module types

void (*mod_Process_Type[MAX_MODULETYPES]) (unsigned char nodeID);               // array of preprocess functions for module types

void (*mod_Tick_Type[MAX_MODULETYPES]) (unsigned char nodeID);                  // array of preprocess functions for module types

void (*mod_UnInit_Type[MAX_MODULETYPES]) (unsigned char nodeID);                // array of uninit functions for module types


unsigned char mod_Ports[MAX_MODULETYPES];                                       // array of port byte sizes per module type

unsigned char mod_PrivVars[MAX_MODULETYPES];                                    // array of private byte sizes per module type


mod_portdata_t *mod_PortTypes[MAX_MODULETYPES];                                 // array that holds pointers to the module data for each module type

mod_portdata_t *mod_PrivVarTypes[MAX_MODULETYPES];                              // array that holds pointers to the module data for each module type


unsigned char mod_ReProcess = 0;                                                // counts number of modules which have requested reprocessing (eg after distributing a reset timestamp)



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Initialises module type data. called at init
/////////////////////////////////////////////////////////////////////////////

void Mod_Init_ModuleData(void) {
    unsigned char thistype;
    
    for (thistype = 0; thistype < MAX_MODULETYPES; thistype++) {
        
        mod_Init_Type[thistype] = mod_ModuleData_Type[thistype]->init_fn;
        mod_Process_Type[thistype] = mod_ModuleData_Type[thistype]->proc_fn;
        mod_Tick_Type[thistype] = mod_ModuleData_Type[thistype]->tick_fn;
        mod_UnInit_Type[thistype] = mod_ModuleData_Type[thistype]->uninit_fn;
        
        mod_Ports[thistype] = mod_ModuleData_Type[thistype]->ports;
        mod_PrivVars[thistype] = mod_ModuleData_Type[thistype]->privvars;
        
        mod_PortTypes[thistype] = mod_ModuleData_Type[thistype]->porttypes;
        mod_PrivVarTypes[thistype] = mod_ModuleData_Type[thistype]->privvartypes;
        
        mod_Send_Type[thistype] = 
            mod_SendData_Type[mod_ModuleData_Type[thistype]->buffertype].send_fn;
        
    }
    
}



/////////////////////////////////////////////////////////////////////////////
// Processes a single module
// in: node ID
// Never call directly! Use Mod_PreProcess instead! (scroll down)
/////////////////////////////////////////////////////////////////////////////

void Mod_Process(unsigned char nodeID) {
    if ((node[nodeID].moduletype) < DEAD_MODULETYPE) {
        mod_Process_Type[(node[nodeID].moduletype)](nodeID);                    // process according to the moduletype
    }
    
}

/////////////////////////////////////////////////////////////////////////////
// Tick a single module
// in: node ID
// Never call directly! Called by Mod_Tick()
/////////////////////////////////////////////////////////////////////////////

void Mod_Tick_Node(unsigned char nodeID) {
    if ((node[nodeID].moduletype) < DEAD_MODULETYPE) {
        
        ++(node[nodeID].ticked);                                                // mark the node as ticked
        ++(node[nodeID].process_req);                                           // request to process the node to clear the outbuffer and change params ready for next preprocessing
        
        mod_Tick_Type[(node[nodeID].moduletype)](nodeID);                       // process timestamps according to the moduletype
        
        Mod_Propagate(nodeID);                                                  // and propagate in case of a change in outgoing nexttick
        
    }
    
}



/////////////////////////////////////////////////////////////////////////////
// Propagates all ports to their connected ports via the patch cables
// in: node ID
// Never call directly! Use Mod_PreProcess instead! (scroll down)
/////////////////////////////////////////////////////////////////////////////

void Mod_Propagate(unsigned char nodeID) {
    if (nodeID < DEAD_NODEID) {
        if ((node[nodeID].indegree) < DEAD_INDEGREE) {                          // handle node isnt dead with indegree 0xff
            edge_t *edgepointer = node[nodeID].edgelist;                        // load up the list header
            while (edgepointer != NULL) {                                       // if there's any edges, then...
                if (edgepointer->msgxlate != NULL) {
                    edgepointer->msgxlate                                       // according to the source and destination port types
                        (nodeID, edgepointer->tailport
                        , edgepointer->headnodeID, edgepointer->headport);      // send data from tail to head
                }
                edgepointer = edgepointer->next;                                // search through the list
            }                                                                   // until all edges are done
            
        }
        
    }
    
}



/////////////////////////////////////////////////////////////////////////////
// Preprocess all nodes from a given node down
// in: node ID to start from
/////////////////////////////////////////////////////////////////////////////

void Mod_PreProcess(unsigned char startnodeID) {
    nodelist_t *topolist;
    unsigned char procnodeID;
    do {
        if (node_Count > 0) {                                                   // handle no nodes
            topolist = topoListHead;                                            // follow list of topo sorted nodeIDs
            if (topolist != NULL) {                                             // handle dead list
                if ((mClock.status.reset_req == 0)                              // force processing from root if global reset requested
                    && (startnodeID < DEAD_NODEID)) {                           // otherwise if we are not requested to process from the root
                    while ((topolist->nodeID) != startnodeID) {                 // search for the start node
                        topolist = topolist->next;                              // through each entry in the topolist
                    }                                                           // until we have the one we want
                    
                }
                
                while (topolist != NULL) {                                      // for each entry in the topolist from there on
                    procnodeID = topolist->nodeID;                              // get the nodeID
                    if (procnodeID < MAX_NODES) {                               // only process nodes which...
                        if (
                            (
                             (node[procnodeID].nexttick == RESET_TIMESTAMP) ||  // have unprocessed resets
                             (mClock.status.reset_req > 0)                      // or if there is a master reset request
                            )
                             ||                                                 // otherwise only preprocess if...
                            (
                             (
                              (node[procnodeID].process_req > 0) ||             // they request it
                              (node[procnodeID].ticked > 0)                     // or need it because they've got unprocessed ticks
                             )
                              &&
                             (
                              (node[procnodeID].downstreamtick 
                              >= node[procnodeID].nexttick) ||                  // but only if it's safe for the downstream modules
                              (node[procnodeID].nexttick >= DEAD_TIMESTAMP)     // or if this module needs attention
                             )
                            )
                           )
                        {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[vX][prep] PreProcessing node %d\n", procnodeID);
#endif
                            Mod_Process(procnodeID);                            // crunch the numbers according to function pointer
                            
                            Mod_Propagate(procnodeID);                          // spread out the ports to the downstream nodes
                            
                            node[procnodeID].process_req = 0;                   // clear the process request here, propagation has marked downstream nodes
                        }
                        
                    }
                    
                    topolist = topolist->next;                                  // and move onto the next node in the sorted list
                }
                
            }
            
        }
        
    } while (mod_ReProcess > 0);
    
}



/////////////////////////////////////////////////////////////////////////////
// Checks to see if modules have ticked 
// according to the master clock and each module's nexttick timestamp
/////////////////////////////////////////////////////////////////////////////

void Mod_Tick(void) {
    unsigned char i;
    nodelist_t *topolist;
    unsigned char ticknodeID;
    unsigned char module_ticked = DEAD_NODEID;
    
    if (mClock.status.run > 0) {                                                // if we're playing
        if (node_Count > 0) {                                                   // handle no nodes
            topolist = topoListHead;                                            // follow list of topo sorted nodeIDs, to send their outbuffers
            while (topolist != NULL) {                                          // for each entry in the topolist
                ticknodeID = topolist->nodeID;                                  // get the nodeID
                if (ticknodeID < MAX_NODES) {                                   // if the nodeID is valid
                    if (node[ticknodeID].indegree < DEAD_INDEGREE) {            // if the module is active
                        if (node[ticknodeID].nexttick < DEAD_TIMESTAMP) {       // if the module is clocked
                            if (node[ticknodeID].nexttick 
                                <= mod_Tick_Timestamp) {                        // if that clock has ticked
#if vX_DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[vX] Ticking node %d\n", ticknodeID);
#endif
                                if ((mClock.status.spp_hunt == 0) &&             // and we aren't hunting for song position
                                    (node[ticknodeID].outbuffer_req > 0))     // if there's anything to output on this node
                                {
                                    Mod_Send_Buffer(ticknodeID);                // dump all its outbuffers
                                }
                                
                                node[ticknodeID].status.tickseen = 1;           // mark the node as ticked, we will use this shortly
                                
                            }
                            
                        }
                        
                    }
                    
                }
                
                topolist = topolist->next;                                      // and move onto the next node in the sorted list
            }
            
            
            
            topolist = topoListHead;                                            // follow list of topo sorted nodeIDs, this time to deal with the timestamps
            while (topolist != NULL) {                                          // for each entry in the topolist
                ticknodeID = topolist->nodeID;                                  // get the nodeID
                if (node[ticknodeID].status.tickseen > 0) {                     // if that clock has ticked
#if vX_DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[vX] Ticking node %d\n", ticknodeID);
#endif
                    
                    Mod_Tick_Node(ticknodeID);
                    
                    if (module_ticked == DEAD_NODEID) 
                        module_ticked = ticknodeID;                             // remember the first node we hit
                    
                    node[ticknodeID].status.tickseen = 0;
                }
                
                topolist = topolist->next;                                      // and move onto the next node in the sorted list
            }
            
            
            if (module_ticked != DEAD_NODEID) {
                Mod_PreProcess(module_ticked);                                  // if any subclock ticked, process from the first one down
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

void Mod_SetNextTick(unsigned char nodeID, u32 timestamp
                    , u32 ( *mod_reset_type)(unsigned char resetnodeID)) {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[vX][setk] Setting nexttick for node %d\n", nodeID);
#endif
#if vX_DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[vX] Incoming timestamp is %u\n", timestamp);
#endif
    if (mClock.status.reset_req == 0) {                                         // if no global reset
        if (node[nodeID].nexttick == timestamp) {                               // same as before?
#if vX_DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[vX][setk] No change");
#endif
            return;                                                             // nothing to do then
        }
        
        if (timestamp != RESET_TIMESTAMP) {
            
            if (node[nodeID].nexttick == RESET_TIMESTAMP) {                     // If this is a reprocess? check nexttick for reset timestamp and if it is then
#if vX_DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[vX] Resetting node due to local reset request\n");
#endif
                mod_ReProcess--;                                                // decrement mod_ReProcess and 
                if (mod_reset_type != NULL) {
                    node[nodeID].nexttick = (*mod_reset_type)(nodeID);          // calculate the new nexttick
                } else {
                    node[nodeID].nexttick = DEAD_TIMESTAMP;
                }
            } else {                                                            // otherwise use the passed timestamp
#if vX_DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[vX] Writing timestamp\n");
#endif
                node[nodeID].nexttick = timestamp;                              // write it to node[nodeID].nexttick (output ports should be written elsewhere)
            }
            
#if vX_DEBUG_VERBOSE_LEVEL >= 3
    DEBUG_MSG("[vX] Setting downstream ticks\n");
#endif
            Mod_TickPriority(nodeID);                                           // then do upstream nodes' timestamps
            
        } else {                                                                // Request reprocessing after distributing a reset
            
#if vX_DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[vX] Incoming reset request\n", timestamp);
#endif
                                                                                // Does not bother recalculating the ticks upstream because it should be done on the reprocess run
            node[nodeID].nexttick = timestamp;                                  // write the incoming reset timestamp to node[nodeID].nexttick (as well as to output ports)
            mod_ReProcess++;                                                    // increment mod_ReProcess to cause Mod_PreProcess to re-run
        }
        
    } else {
        
#if vX_DEBUG_VERBOSE_LEVEL >= 2
    DEBUG_MSG("[vX] Resetting node due to global reset request\n");
#endif
        if (mod_reset_type != NULL) {
            node[nodeID].nexttick = (*mod_reset_type)(nodeID);                  // calculate the new nexttick
        } else {
            node[nodeID].nexttick = DEAD_TIMESTAMP;
        }
        
        mod_ReProcess = 0; 
        
    }
#if vX_DEBUG_VERBOSE_LEVEL >= 1
    DEBUG_MSG("[vX][setk] New nexttick for node %d is %u\n",nodeID ,node[nodeID].nexttick);
#endif
    
}



/////////////////////////////////////////////////////////////////////////////
// function for setting the node[n].downstreamtick variable
// never simply write to that variable, use this function to find it
// that variable holds the lowest (soonest) nexttick timestamp out of
// all the nodes downstream of here
// in: node ID to start at. It'll go upstream by itself.
/////////////////////////////////////////////////////////////////////////////

void Mod_TickPriority(unsigned char nodeID) {
#if vX_DEBUG_VERBOSE_LEVEL >= 1
        DEBUG_MSG("[vX][prio] Setting downstream ticks above %d\n", nodeID);
#endif    
    if (nodeID < DEAD_NODEID) {
        if ((node[nodeID].indegree) < DEAD_INDEGREE) {                          // handle node isnt dead with indegree 0xff
            edge_t *edgepointer_in = node[nodeID].edgelist_in;                  // load up the list header
            while (edgepointer_in != NULL) {                                    // if there's any more edges, then...
                unsigned char tempnodeID_us = edgepointer_in->tailnodeID;
                
                edge_t *edgepointer_out = node[tempnodeID_us].edgelist;         // load up the list header
                u32 temptimestamp = DEAD_TIMESTAMP;
                while (edgepointer_out != NULL) {                               // if there's any more edges, then...
                    unsigned char tempnodeID = edgepointer_out->headnodeID;
                    if (node[tempnodeID].nexttick < temptimestamp)
                        temptimestamp = node[tempnodeID].nexttick;
                    edgepointer_out = edgepointer_out->next;
                    
                }
                
                node[tempnodeID_us].downstreamtick = temptimestamp;
#if vX_DEBUG_VERBOSE_LEVEL >= 1
DEBUG_MSG("[vX][prio] Node %d downstream tick is %u\n", tempnodeID_us, temptimestamp);
#endif
                edgepointer_in = edgepointer_in->head_next;
                
            }                                                                   // until all upstream nodes are listed
            
        }
        
    }
    
}



/////////////////////////////////////////////////////////////////////////////
// function for initialising the graph on new node creation
// in: new node ID, new module type
/////////////////////////////////////////////////////////////////////////////


void Mod_Init_Graph(unsigned char nodeID, unsigned char moduletype) {           // Mandatory graph initialisation
    
    node[nodeID].moduletype = moduletype;                                       // store the module type
    node[nodeID].indegree = 0;                                                  // set indegree to 0 this is new
    node[nodeID].indegree_uv = 0;                                               // set indegree to 0 this is new
    node[nodeID].process_req = 0;                                               // set process req to 0 this is new
    if (mod_Ports[moduletype] > 0) node[nodeID].ports = 
            pvPortMalloc((sizeof(char))*(mod_Ports[moduletype]));               // allocate memory for ports array, elements according to moduletype
    if (mod_PrivVars[moduletype] > 0) node[nodeID].privvars = 
            pvPortMalloc((sizeof(char))*(mod_PrivVars[moduletype]));            // allocate memory for internal use, elements according to moduletype
    node[nodeID].ticked = 0;                                                    // clear the outbuffer tick its a bit early for that
    node[nodeID].status.all = 0;
    
    node[nodeID].nexttick = DEAD_TIMESTAMP;
    node[nodeID].downstreamtick = DEAD_TIMESTAMP;
    
    node[nodeID].edgelist = NULL;
    node[nodeID].edgelist_in = NULL;
    
    node[nodeID].outbuffer_size = mod_SendData_Type[(mod_ModuleData_Type[moduletype]->buffertype)].buffer_size;
    
    if ((mod_ModuleData_Type[moduletype]->buffers) > 0) {
        
        if ((node[nodeID].outbuffer_size) > 0) {
            node[nodeID].outbuffer = 
                pvPortMalloc(
                    node[nodeID].outbuffer_size
                    * (mod_ModuleData_Type[moduletype]->buffers));              // allocate memory for buffer array, size according to moduletype and it's correstponding outbuffer type and count
        } else {
            node[nodeID].outbuffer = NULL;                                      // clear the outbuffer pointer as it will be allocated as required in this types init
        }
        
    }
    
    node[nodeID].outbuffer_req = 0;
    
    mod_Init_Type[moduletype](nodeID);                                          // initialise the module hosted by this node
    
}



/////////////////////////////////////////////////////////////////////////////
// uninitialise a module after deletion as per it's module type
// in: node ID
/////////////////////////////////////////////////////////////////////////////

void Mod_UnInit(unsigned char nodeID) {
    mod_UnInit_Type[(node[nodeID].moduletype)](nodeID);                         // unititialise according to the moduletype
}


/////////////////////////////////////////////////////////////////////////////
// mandatory graph uninits for a module after deletion
// in: node ID
/////////////////////////////////////////////////////////////////////////////

void Mod_UnInit_Graph(unsigned char nodeID) {                                   // do stuff with inputs and push to the outputs 
    Mod_UnInit(nodeID);                                                         // uninit the module
    
    node[nodeID].moduletype = DEAD_MODULETYPE;                                  // mark node as empty
    node[nodeID].indegree = DEAD_INDEGREE;                                      // mark node as empty
    node[nodeID].indegree_uv = 0;                                               // set indegree to 0 this is new
    node[nodeID].edgelist = NULL;
    node[nodeID].edgelist_in = NULL;
    node[nodeID].process_req = 0;                                               // set process req to 0 this is dead
    node[nodeID].status.all = 0;
    
    vPortFree((node[nodeID].ports));                                            // free up the ports
    node[nodeID].ports = NULL;                                                  // fix the pointer
    
    vPortFree((node[nodeID].privvars));                                         // free up the private vars
    node[nodeID].privvars = NULL;                                               // fix the pointer
    
    vPortFree((node[nodeID].outbuffer));
    node[nodeID].outbuffer = NULL;
    
    node[nodeID].nexttick = DEAD_TIMESTAMP;
    node[nodeID].downstreamtick = DEAD_TIMESTAMP;
    node[nodeID].ticked = 0;                                                    // clear the outbuffer tick
    
}









// todo

// realtime message propagation control (refuse to send until earliest downstreamtick)









// module organisation
// internal
/*
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
Mux one to many
Mux many to one

filter range
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
