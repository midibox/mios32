/* $Id$ */
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

mod_moduledata_t mod_MIDIOut_ModuleData = {
    &Mod_Init_MIDIOut,                                                          // name of functions to initialise
    &Mod_Proc_MIDIOut,                                                          // name of functions to process
    &Mod_Tick_MIDIOut,                                                          // name of functions to tick
    &Mod_UnInit_MIDIOut,                                                        // name of functions to uninitialise
    
    MOD_MIDIOUT_PORTS,                                                          // size of char array to allocate
    MOD_MIDIOUT_PRIVVARS,                                                       // size of char array to allocate
    
    MOD_MIDIOUT_BUFFERS,                                                        // number of buffers to allocate
    MOD_SEND_TYPE_MIDI,                                                         // type of those buffers (references the function to send them, and their size)
    
    mod_MIDIOut_PortTypes,                                                      // pointer to port type lists
    mod_MIDIOut_PrivVarTypes,                                                   // pointer to private var type lists
    "MIDIOut",                                                                  // 8 character name
};



mod_portdata_t mod_MIDIOut_PortTypes[MOD_MIDIOUT_PORTS] = {
    MOD_PORTTYPE_VALUE, "HW Buss ",                                             //hardware port
    MOD_PORTTYPE_VALUE, "Byte 0  ",                                             //byte 1 (status)
    MOD_PORTTYPE_VALUE, "Byte 1  ",                                             //byte 2 (note #)
    MOD_PORTTYPE_VALUE, "Byte 2  ",                                             //byte 3 (velocity)
    MOD_PORTTYPE_TIMESTAMP, "Length  ",                                         //length
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!", 
    MOD_PORTTYPE_TIMESTAMP, "Clock In",                                         //next tick
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!", 
    DEAD_PORTTYPE, "NoPatch!", 
};

mod_portdata_t mod_MIDIOut_PrivVarTypes[MOD_MIDIOUT_PRIVVARS] = {
};


/////////////////////////////////////////////////////////////////////////////
// local prototypes
/////////////////////////////////////////////////////////////////////////////

void Mod_Init_MIDIOut(unsigned char nodeID) {                                   // initialize a midi out module
}

void Mod_Proc_MIDIOut(unsigned char nodeID) {                                   // do stuff with inputs and push to the outputs 
    Mod_Tick_MIDIOut(nodeID);
}

void Mod_Tick_MIDIOut(unsigned char nodeID) {                                   // set a new timestamp
}

void Mod_UnInit_MIDIOut(unsigned char nodeID) {                                 // uninitialize a midi out module
}

