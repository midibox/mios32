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
// Global Variables
/////////////////////////////////////////////////////////////////////////////

mod_moduledata_t mod_SxH_ModuleData = {
    &Mod_Init_SxH,                                                              // name of functions to initialise
    &Mod_Proc_SxH,                                                              // name of functions to process
    &Mod_Tick_SxH,                                                              // name of functions to tick
    &Mod_UnInit_SxH,                                                            // name of functions to uninitialise
    
    MOD_SXH_PORTS,                                                              // size of char array to allocate
    MOD_SXH_PRIVVARS,                                                           // size of char array to allocate
    
    MOD_SXH_BUFFERS,                                                            // number of buffers to allocate
    MOD_SEND_TYPE_DUMMY,                                                        // type of those buffers (references the function to send them, and their size)
    
    mod_SxH_PortTypes,                                                          // pointer to port type lists
    mod_SxH_PrivVarTypes,                                                       // pointer to private var type lists
    "SamplHld",                                                                 // 8 character name
};



mod_portdata_t mod_SxH_PortTypes[MOD_SXH_PORTS] = {
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!",                                                  //dummies until i do something real
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!", 
};


mod_portdata_t mod_SxH_PrivVarTypes[MOD_SXH_PRIVVARS] = {
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!",                                                  //dummies until i do something real
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!", 
};




/////////////////////////////////////////////////////////////////////////////
// Global prototypes
/////////////////////////////////////////////////////////////////////////////

void Mod_Init_SxH(unsigned char nodeID) {                                       // initialize a sample and hold module
}

void Mod_Proc_SxH(unsigned char nodeID) {                                       // do stuff with inputs and push to the outputs 
    Mod_Tick_SxH(nodeID);
}

void Mod_Tick_SxH(unsigned char nodeID) {                                       // set a new timestamp
}

void Mod_UnInit_SxH(unsigned char nodeID) {                                     // uninitialize a sample and hold module
}

u32 Mod_Reset_SxH(unsigned char nodeID) {                                       // function to reset this module
}
