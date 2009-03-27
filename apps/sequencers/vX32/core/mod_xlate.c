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

const char mod_PortType_Name[MAX_PORTTYPES][PORTTYPE_NAME_STRING_LENGTH] = {
    "Trigger ",                                                                 // timestamp
    "Value   ",                                                                 // value
    "Package ",                                                                 // package
    "Flags   ",                                                                 // flag
};
    
void (*const mod_Xlate_Table[MAX_PORTTYPES][MAX_PORTTYPES]) 
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port) = {
    &Mod_Xlate_Time_Time,                                                       // timestamp - timestamp
    &Mod_Xlate_Time_Value,                                                      // timestamp - value
    &Mod_Xlate_Time_Package,                                                    // timestamp - value
    &Mod_Xlate_Time_Flag,                                                       // timestamp - flag

    &Mod_Xlate_Value_Time,                                                      // value - timestamp
    &Mod_Xlate_Value_Value,                                                     // value - value
    &Mod_Xlate_Value_Package,                                                   // value - value
    &Mod_Xlate_Value_Flag,                                                      // value - flag

    &Mod_Xlate_Package_Time,                                                    // value - timestamp
    &Mod_Xlate_Package_Value,                                                   // value - value
    &Mod_Xlate_Package_Package,                                                 // value - value
    &Mod_Xlate_Package_Flag,                                                    // value - flag

    &Mod_Xlate_Flag_Time,                                                       // flag - timestamp
    &Mod_Xlate_Flag_Value,                                                      // flag - value
    &Mod_Xlate_Flag_Package,                                                    // flag - value
    &Mod_Xlate_Flag_Flag,                                                       // flag - flag
};


void (*const mod_DeadPort_Table[(MAX_PORTTYPES)]) 
                        (unsigned char nodeID, unsigned char port) = {
    &Mod_DeadPort_Time,                                                         // timestamp
    &Mod_DeadPort_Value,                                                        // value
    &Mod_DeadPort_Package,                                                      // package
    &Mod_DeadPort_Flag,                                                         // flag
};


void (*const mod_SetPort_Table[(MAX_PORTTYPES)]) 
                    (u32 input, unsigned char nodeID, unsigned char port) = {
    &Mod_SetPort_Time,                                                          // timestamp
    &Mod_SetPort_Value,                                                         // value
    &Mod_SetPort_Package,                                                       // package
    &Mod_SetPort_Flag,                                                          // flag
};


u32 (*const mod_GetPort_Table[(MAX_PORTTYPES)]) 
                    (unsigned char nodeID, unsigned char port) = {
    &Mod_GetPort_Time,                                                          // timestamp
    &Mod_GetPort_Value,                                                         // value
    &Mod_GetPort_Package,                                                       // package
    &Mod_GetPort_Flag,                                                          // flag
};






/////////////////////////////////////////////////////////////////////////////
// Returns the port type for a given port
// in port number and node ID
// out: port type
/////////////////////////////////////////////////////////////////////////////

unsigned char Mod_GetPortType(unsigned char nodeID, unsigned char port) {
    if (nodeID < DEAD_NODEID) {
        if (node[nodeID].indegree < DEAD_INDEGREE) {
            if (node[nodeID].moduletype < DEAD_MODULETYPE) {
                return (mod_PortTypes[(node[nodeID].moduletype)])
                        [port].porttype;
            }
        }
    }
    
    return DEAD_PORTTYPE;
    
}

/////////////////////////////////////////////////////////////////////////////
// Returns the port type name for a given port
// in port type
// out: port type name
/////////////////////////////////////////////////////////////////////////////

const char *Mod_GetPortTypeName(unsigned char porttype) {
    if (porttype < DEAD_PORTTYPE) {
        return mod_PortType_Name[porttype];
    }
    
    return NULL;
    
}



/////////////////////////////////////////////////////////////////////////////
// Port translator functions
// for use by the engine, don't call these
/////////////////////////////////////////////////////////////////////////////

void Mod_Xlate_Time_Time(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // timestamp - timestamp
    u32 *from, *to;
    from = (u32 *) &(node[tail_nodeID].ports[tail_port]);
    to = (u32 *) &(node[head_nodeID].ports[head_port]);
    if (*to != *from) {
        *to = *from;
        node[head_nodeID].process_req++;                                        // request processing
    }
    
}

void Mod_Xlate_Time_Value(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // timestamp - value
}

void Mod_Xlate_Time_Package(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // timestamp - package
}

void Mod_Xlate_Time_Flag(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // timestamp - flag
}



void Mod_Xlate_Value_Time(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // value - timestamp
}

void Mod_Xlate_Value_Value(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // value - value
    u8 *from, *to;
    from = (u8 *) &(node[tail_nodeID].ports[tail_port]);
    to = (u8 *) &(node[head_nodeID].ports[head_port]);
    if (*to != *from) {
        *to = *from;
        node[head_nodeID].process_req++;                                        // request processing
    }
}

void Mod_Xlate_Value_Package(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // value - package
}

void Mod_Xlate_Value_Flag(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // value - flag
}



void Mod_Xlate_Package_Time(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // package - timestamp
}

void Mod_Xlate_Package_Value(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // package - value
}

void Mod_Xlate_Package_Package(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // package - package
}

void Mod_Xlate_Package_Flag(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // package - flag
}



void Mod_Xlate_Flag_Time(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // flag - timestamp
}

void Mod_Xlate_Flag_Value(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // flag - value
}

void Mod_Xlate_Flag_Package(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // flag - package
}

void Mod_Xlate_Flag_Flag(unsigned char tail_nodeID, unsigned char tail_port, 
                        unsigned char head_nodeID, unsigned char head_port) {   // flag - flag
}



/////////////////////////////////////////////////////////////////////////////
// Write dead value to port
// in: node ID, port
/////////////////////////////////////////////////////////////////////////////

void Mod_DeadPort(unsigned char nodeID, unsigned char port) {
    unsigned char head_port_type = Mod_GetPortType(nodeID, port);               // Find out what port type this is
    if (head_port_type < DEAD_PORTTYPE) {
        (*mod_DeadPort_Table[head_port_type])(nodeID, port);                    // and fill it with a dead value
    }
}

/////////////////////////////////////////////////////////////////////////////
// Write dead values to ports according to their type
// in: node ID, port
/////////////////////////////////////////////////////////////////////////////

void Mod_DeadPort_Time(unsigned char nodeID, unsigned char port) {
    u32 *deadport;
    deadport = (u32 *) &node[nodeID].ports[port];
    if (*deadport != DEAD_TIMESTAMP) {
        *deadport = DEAD_TIMESTAMP;
        node[nodeID].process_req++;                                             // request processing
    }
    
}

void Mod_DeadPort_Value(unsigned char nodeID, unsigned char port) {
    s8 *deadport;
    deadport = (s8 *)&node[nodeID].ports[port];
    if (*deadport != DEAD_VALUE) {
        *deadport = DEAD_VALUE;
        node[nodeID].process_req++;                                             // request processing
    }
    
}

void Mod_DeadPort_Package(unsigned char nodeID, unsigned char port) {
    s32 *deadport;
    deadport = (s32 *)&node[nodeID].ports[port];
    if (*deadport != DEAD_PACKAGE) {
        *deadport = DEAD_PACKAGE;
        node[nodeID].process_req++;                                             // request processing
    }
    
}

void Mod_DeadPort_Flag(unsigned char nodeID, unsigned char port) {
    u8 *deadport;
    deadport = (u8 *) &node[nodeID].ports[port];
    if (*deadport != DEAD_VALUE) {
        *deadport = DEAD_VALUE;
        node[nodeID].process_req++;                                             // request processing
    }
    
}




/////////////////////////////////////////////////////////////////////////////
// Write value to port
// in: value to write, cast to u32, node ID, port to write
/////////////////////////////////////////////////////////////////////////////

void Mod_SetPort(u32 input, unsigned char nodeID, unsigned char port) {
    unsigned char port_type = Mod_GetPortType(nodeID, port);                    // Find out what port type this is
    if (port_type < DEAD_PORTTYPE) {
        (*mod_SetPort_Table[port_type])(input, nodeID, port);                   // and fill it with a dead value
    }
}

/////////////////////////////////////////////////////////////////////////////
// Called from Mod_SetPort, according to port type
// in: value to write, cast to u32, node ID, port to write
/////////////////////////////////////////////////////////////////////////////
void Mod_SetPort_Time(u32 input, unsigned char nodeID, unsigned char port) {
    u32 *destPort;
    destPort = (u32 *) &node[nodeID].ports[port];
    if (*destPort != input) {
        *destPort = input;
        node[nodeID].process_req++;                                             // request processing
    }
    
}

void Mod_SetPort_Value(u32 input, unsigned char nodeID, unsigned char port) {
    s8 castinput = (s8)input;
    s8 *destPort;
    destPort = (s8 *)&node[nodeID].ports[port];
    if (*destPort != castinput) {
        *destPort = castinput;
        node[nodeID].process_req++;                                             // request processing
    }
    
}

void Mod_SetPort_Package(u32 input, unsigned char nodeID, unsigned char port) {
    u32 *destPort;
    destPort = (u32 *)&node[nodeID].ports[port];
    if (*destPort != input) {
        *destPort = input;
        node[nodeID].process_req++;                                             // request processing
    }
    
}

void Mod_SetPort_Flag(u32 input, unsigned char nodeID, unsigned char port) {
    u8 castinput = (u8)input;
    u8 *destPort;
    destPort = (u8 *) &node[nodeID].ports[port];
    if (*destPort != castinput) {
        *destPort = castinput;
        node[nodeID].process_req++;                                             // request processing
    }
    
}



/////////////////////////////////////////////////////////////////////////////
// Get value from port
// in: node ID, port
// out: value cast to u32
/////////////////////////////////////////////////////////////////////////////

u32 Mod_GetPort(unsigned char nodeID, unsigned char port) {
    unsigned char port_type = Mod_GetPortType(nodeID, port);                    // Find out what port type this is
    if (port_type < DEAD_PORTTYPE) {
        return (*mod_GetPort_Table[port_type])(nodeID, port);                   // and fill it with a dead value
    }
    
    return DEAD_TIMESTAMP;
}

/////////////////////////////////////////////////////////////////////////////
// Called from Mod_GetPort, according to port type
// in: node ID, port
/////////////////////////////////////////////////////////////////////////////

u32 Mod_GetPort_Time(unsigned char nodeID, unsigned char port) {
    u32 *destPort;
    destPort = (u32 *) &node[nodeID].ports[port];
    return *destPort;    
}

u32 Mod_GetPort_Value(unsigned char nodeID, unsigned char port) {
    s8 *destPort;
    destPort = (s8 *) &node[nodeID].ports[port];
    return (u32) *destPort;
}

u32 Mod_GetPort_Package(unsigned char nodeID, unsigned char port) {
    u32 *destPort;
    destPort = (u32 *) &node[nodeID].ports[port];
    return (u32) *destPort;
}

u32 Mod_GetPort_Flag(unsigned char nodeID, unsigned char port) {
    u8 *destPort;
    destPort = (u8 *) &node[nodeID].ports[port];
    return *destPort;
}
