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
#include <seq_bpm.h>


/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global prototypes
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Simple UI functions. CS Should access these.
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Set tempo
// in: BPM (float)
/////////////////////////////////////////////////////////////////////////////

void UI_SetBPM(float bpm) {
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {
            MClock_User_SetBPM(bpm);
            MUTEX_GRAPH_GIVE;
            break;
        }
        
        vTaskDelay((portTickType)1);
        
    }
    
}



/////////////////////////////////////////////////////////////////////////////
// Start the sequencer from 0
/////////////////////////////////////////////////////////////////////////////

void UI_Start(void) {
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {
            MClock_User_Start();
            MUTEX_GRAPH_GIVE;
            break;
        }
        
        vTaskDelay((portTickType)1);
        
    }
    
}



/////////////////////////////////////////////////////////////////////////////
// Continue playing from the current position
/////////////////////////////////////////////////////////////////////////////

void UI_Continue(void) {
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {
            MClock_User_Continue();
            MUTEX_GRAPH_GIVE;
            break;
        }
        
        vTaskDelay((portTickType)1);
        
    }
    
}



/////////////////////////////////////////////////////////////////////////////
// Stop the sequencer here (pause)
/////////////////////////////////////////////////////////////////////////////

void UI_Stop(void) {
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {
            MClock_User_Stop();
            MUTEX_GRAPH_GIVE;
            break;
        }
        
        vTaskDelay((portTickType)1);
        
    }
    
}



/////////////////////////////////////////////////////////////////////////////
// insert a new module into the rack
// in: type of module
// out: new module ID
/////////////////////////////////////////////////////////////////////////////

unsigned char UI_NewModule(unsigned char moduletype) {
    unsigned char returnID = DEAD_NODEID;
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph
            returnID = Node_Add(moduletype);
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        }
        
    }
    
    return returnID;
        
}



/////////////////////////////////////////////////////////////////////////////
// Insert a new patch cable between two modules
// in: module ID and port type for each end of the patch
// out: cable ID (edge pointer)
/////////////////////////////////////////////////////////////////////////////

edge_t *UI_NewCable(unsigned char tail_nodeID, unsigned char tail_port
                , unsigned char head_nodeID, unsigned char head_port) {
    edge_t *newedge = NULL;
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph
            newedge = Edge_Add(tail_nodeID, tail_port, head_nodeID, head_port);
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        }
        
    }
    
    return newedge;
        
}


/////////////////////////////////////////////////////////////////////////////
// Remove an active module from the rack
// in: module ID
// out: error code. 0 means success
/////////////////////////////////////////////////////////////////////////////

unsigned char UI_RemoveModule(unsigned char nodeID) {
    unsigned char error = 90;
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph
            error = Node_Del(nodeID);
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        }
        
    }
    
    return error;
        
}



/////////////////////////////////////////////////////////////////////////////
// Remove a patch cable
// in: mode ID's and ports for each end of the patch cable
// out: error code. 0 means success
/////////////////////////////////////////////////////////////////////////////

unsigned char UI_RemoveCable(unsigned char tail_nodeID, unsigned char tail_port
                , unsigned char head_nodeID, unsigned char head_port) {
    unsigned char error = 90;
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph
            error = Edge_Del(
                    Edge_GetID(tail_nodeID, tail_port, head_nodeID, head_port), 
                    DO_TOPOSORT);
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        }
        
    }
    
    return error;
        
}


/////////////////////////////////////////////////////////////////////////////
// Write value to port according to its type
// these can't cause any subsequent changes by themselves
// unlike for eg node deletion, which can trigger edge deletion
// therefore they can write direct to the graph
// provided that they can take the mutex of course!
// in: input value - cast to u32, node, and port you want to write to
/////////////////////////////////////////////////////////////////////////////

void UI_SetPort(u32 input, unsigned char nodeID, unsigned char port) {
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph

            Mod_SetPort(input, nodeID, port);
            
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        } else {
            vTaskDelay((portTickType)1);
        }
        
    }
    
}



/////////////////////////////////////////////////////////////////////////////
// Set the name of a module
// in: module ID, string to write
// out: name
/////////////////////////////////////////////////////////////////////////////

char *UI_SetModuleName(unsigned char nodeID, char *newname) {
    char *returnstr = NULL;
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph
            returnstr = Node_SetName(nodeID, newname);
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        }
        
    }
    
    return returnstr;
        
}



/////////////////////////////////////////////////////////////////////////////
// Get a value of a port
// in: module ID, port
// out: value of port, cast to u32
/////////////////////////////////////////////////////////////////////////////

u32 UI_GetPort(unsigned char nodeID, unsigned char port) {
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph

            Mod_GetPort(nodeID, port);
            
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        } else {
            vTaskDelay((portTickType)1);
        }
        
    }
    
}



/////////////////////////////////////////////////////////////////////////////
// Get the port type (value, trigger, etc)
// in: module ID, port
// out: port type
/////////////////////////////////////////////////////////////////////////////

unsigned char UI_GetPortType(unsigned char nodeID, unsigned char port) {
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    unsigned char returntype;
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph

            returntype = Mod_GetPortType(nodeID, port);
            
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        } else {
            vTaskDelay((portTickType)1);
        }
        
    }
    
    return returntype;
    
}



/////////////////////////////////////////////////////////////////////////////
// Get the name of a port type
// in: port type
// out: name
/////////////////////////////////////////////////////////////////////////////

const char *UI_GetPortTypeName(unsigned char porttype) {
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    const char *returnstring;
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph

            returnstring = Mod_GetPortTypeName(porttype);
            
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        } else {
            vTaskDelay((portTickType)1);
        }
        
    }
    
    return returnstring;
    
}



/////////////////////////////////////////////////////////////////////////////
// Get module ID by name
// will return the first hit
// in: name string, length of string to test
// out: module ID
/////////////////////////////////////////////////////////////////////////////

unsigned char UI_GetModuleID(char *name, unsigned char namelength) {
    unsigned char returnID = DEAD_NODEID;
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph
            returnID = Node_GetID(name, namelength);
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        }
        
    }
    
    return returnID;
    
}



/////////////////////////////////////////////////////////////////////////////
// Get module type
// in: module ID
// out: module type
/////////////////////////////////////////////////////////////////////////////

unsigned char UI_GetModuleType(unsigned char nodeID) {
    unsigned char returntype = DEAD_MODULETYPE;
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph
            returntype = Node_GetType(nodeID);
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        }
        
    }
    
    return returntype;
    
}



/////////////////////////////////////////////////////////////////////////////
// Get module name
// in: module ID
// out: name
/////////////////////////////////////////////////////////////////////////////

char *UI_GetModuleName(unsigned char nodeID) {
    char *returnstr = NULL;
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph
            returnstr = Node_GetName(nodeID);
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        }
        
    }
    
    return returnstr;
    
}



/////////////////////////////////////////////////////////////////////////////
// Get name of module type
// in: module type
// out: name
/////////////////////////////////////////////////////////////////////////////

const char *UI_GetModuleTypeName(unsigned char moduletype) {
    const char *returnstr = NULL;
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph
            returnstr = Node_GetTypeName(moduletype);
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        }
        
    }
    
    return returnstr;
    
}



/////////////////////////////////////////////////////////////////////////////
// Get module type by name
// returns the first hit
// in: name, length to test
// out: module type
/////////////////////////////////////////////////////////////////////////////

unsigned char UI_GetModuleTypeID(char *getname, unsigned char length) {
    unsigned char returntype = DEAD_MODULETYPE;
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph
            returntype = Node_GetTypeID(getname, length);
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        }
        
    }
    
    return returntype;
    
}





/////////////////////////////////////////////////////////////////////////////
// Get source module of a cable
// in: cable pointer
// out: module ID
/////////////////////////////////////////////////////////////////////////////

unsigned char UI_GetCableSrcModule(edge_t * edge) {
    unsigned char returnID = DEAD_NODEID;
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph
            returnID = Edge_GetTailNode(edge);
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        }
        
    }
    
    return returnID;
    
}



/////////////////////////////////////////////////////////////////////////////
// Get destination module of a cable
// in: cable pointer
// out: module ID
/////////////////////////////////////////////////////////////////////////////

unsigned char UI_GetCableDstModule(edge_t * edge) {
    unsigned char returnID = DEAD_NODEID;
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph
            returnID = Edge_GetHeadNode(edge);
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        }
        
    }
    
    return returnID;
    
}



/////////////////////////////////////////////////////////////////////////////
// Get source port of a cable
// in: cable pointer
// out: port
/////////////////////////////////////////////////////////////////////////////

unsigned char UI_GetCableSrcPort(edge_t * edge) {
    unsigned char returnID = DEAD_PORTTYPE;
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph
            returnID = Edge_GetTailPort(edge);
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        }
        
    }
    
    return returnID;
    
}



/////////////////////////////////////////////////////////////////////////////
// Get destination port of a cable
// in: cable pointer
// out: port
/////////////////////////////////////////////////////////////////////////////

unsigned char UI_GetCableDstPort(edge_t * edge) {
    unsigned char returnID = DEAD_PORTTYPE;
    unsigned int timeout = UI_GRAPHMOD_TIMEOUT;
    
    while (--timeout > 0) {
        if (MUTEX_GRAPH_TAKE == pdTRUE) {                                       // Take the Mutex to write to the graph
            returnID = Edge_GetHeadPort(edge);
            MUTEX_GRAPH_GIVE;                                                   // Give back the Mutex to write to the graph
            break;
        }
        
    }
    
    return returnID;
        
}





/////////////////////////////////////////////////////////////////////////////
// Internal functions
/////////////////////////////////////////////////////////////////////////////

void UI_Tick(void) {

    // Remote CS login, should return a unique CS ID to the requesting device

    // rec'v incoming commands stamped with CS ID and a request ID, followed by the command, payload and checksum
    // add them to a queue

    // read from the queue, and use the above functions according to command
    // send ack/nack with request ID and CS ID, then payload

}




// notes
// UI:

//                                  port type  / port      / port      / node      / edge
// reader function                  (node port / node port / node priv / node name / edge ID tailnode tailport headnode headport)


//                                  port           / privvar (port)/ node
// modifier function                (node port val / node priv val / node name)

//                                  node        /   edge
// create function                  (moduletype / tailnode tailport headnode headport)

//                                  node    /       edge
// delete function                  (nodeID / edge pointer)

// needs:



//          A                        edge                   B               
//  node ID=module name                              node ID=module name    
//  mod type=type name                               mod type=type name     
                                                                            
//  port ID=port name                ID              port ID=port name      
//  port type=type name                              port type=type name    



// master clock functions
// start stop continue set bpm

