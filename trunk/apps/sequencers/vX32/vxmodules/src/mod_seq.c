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

mod_moduledata_t mod_Seq_ModuleData = {
    &Mod_Init_Seq,                                                              // name of functions to initialise
    &Mod_Proc_Seq,                                                              // name of functions to process
    &Mod_Tick_Seq,                                                              // name of functions to tick
    &Mod_UnInit_Seq,                                                            // name of functions to uninitialise
    
    MOD_SEQ_PORTS,                                                              // size of char array to allocate
    MOD_SEQ_PRIVVARS,                                                           // size of char array to allocate
    
    MOD_SEQ_BUFFERS,                                                            // number of buffers to allocate
    MOD_SEND_TYPE_MIDI,                                                         // type of those buffers (references the function to send them, and their size)
    
    mod_Seq_PortTypes,                                                          // pointer to port type lists
    mod_Seq_PrivVarTypes,                                                       // pointer to private var type lists
    "Seqencer",                                                                 // 8 character name
};



mod_portdata_t mod_Seq_PortTypes[MOD_SEQ_PORTS] = {
    {MOD_PORTTYPE_VALUE, "Status  "},                                           //status
    {MOD_PORTTYPE_VALUE, "Channel "},                                           //chan
    {MOD_PORTTYPE_VALUE, "BaseNote"},                                           //note
    {MOD_PORTTYPE_VALUE, "BaseVel"},                                            //vel
    {MOD_PORTTYPE_TIMESTAMP, "LengthIn"},                                       //length reference
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},                                                //length reference
    {MOD_PORTTYPE_TIMESTAMP, "Clock In"},                                       //next tick
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},                                                //next tick
    {MOD_PORTTYPE_VALUE, "BaseStep"},                                           //currentstep
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},                                                //currentstep
    {MOD_PORTTYPE_VALUE, "Len Mult"},                                           //length mult
    {MOD_PORTTYPE_VALUE, "Len Div "},                                           //length div
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},                                                //dummies until i get around to making proper modules heheh
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
};

mod_portdata_t mod_Seq_PrivVarTypes[MOD_SEQ_PRIVVARS] = {
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},                                                //dummies until i get around to making proper modules heheh
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
    {DEAD_PORTTYPE, "NoPatch!"},
};




/////////////////////////////////////////////////////////////////////////////
// Global prototypes
/////////////////////////////////////////////////////////////////////////////

void Mod_Init_Seq(unsigned char nodeID) {                                       // initialize a seq module
    u8 *portcurrentstep;
    portcurrentstep =  (u8 *) &node[nodeID].ports[MOD_SEQ_PORT_CURRENTSTEP];
    u8 *portchan;
    portchan =  (u8 *) &node[nodeID].ports[MOD_SEQ_PORT_NOTE0_CHAN];
    u8 *portnote;
    portnote =  (u8 *) &node[nodeID].ports[MOD_SEQ_PORT_NOTE0_NOTE];
    u8 *portvel;
    portvel =  (u8 *) &node[nodeID].ports[MOD_SEQ_PORT_NOTE0_VEL];
    s8 *portlenh;
    portlenh =  (s8 *) &node[nodeID].ports[MOD_SEQ_PORT_NOTE0_LENH];
    s8 *portlenl;
    portlenl =  (s8 *) &node[nodeID].ports[MOD_SEQ_PORT_NOTE0_LENL];
    u32 *portlenref;
    portlenref =  (u32 *) &node[nodeID].ports[MOD_SEQ_PORT_NOTE0_LENREF];
    u32 *portnexttick;
    portnexttick =  (u32 *) &node[nodeID].ports[MOD_SEQ_PORT_NEXTTICK];
    
    
                //FIXME TESTING
    *portcurrentstep = 0;
    *portchan = 0x09;
    *portnote = -24+(12+nodeID);
    *portvel = 100;
    *portlenh = 8;              //  1
    *portlenl = 0;              // 32nd notes
                //*portlenl = 31;               // 64th notes
                //*portlenl = -32;              // whole notes
    
    *portlenref = DEAD_TIMESTAMP;
    
    *portnexttick = DEAD_TIMESTAMP;
    
}

void Mod_Proc_Seq(unsigned char nodeID) {                                       // do stuff with inputs and push to the outputs 
    u8 *portcurrentstep;
    portcurrentstep =  (u8 *) &node[nodeID].ports[MOD_SEQ_PORT_CURRENTSTEP];
    u8 *portchan;
    portchan =  (u8 *) &node[nodeID].ports[MOD_SEQ_PORT_NOTE0_CHAN];
    u8 *portnote;
    portnote =  (u8 *) &node[nodeID].ports[MOD_SEQ_PORT_NOTE0_NOTE];
    u8 *portvel;
    portvel =  (u8 *) &node[nodeID].ports[MOD_SEQ_PORT_NOTE0_VEL];
    s8 *portlenh;
    portlenh =  (s8 *) &node[nodeID].ports[MOD_SEQ_PORT_NOTE0_LENH];
    s8 *portlenl;
    portlenl =  (s8 *) &node[nodeID].ports[MOD_SEQ_PORT_NOTE0_LENL];
    u32 *portlenref;
    portlenref =  (u32 *) &node[nodeID].ports[MOD_SEQ_PORT_NOTE0_LENREF];
    u32 *portnexttick;
    portnexttick =  (u32 *) &node[nodeID].ports[MOD_SEQ_PORT_NEXTTICK];
    
    
    //Mod_SetNextTick(nodeID, *portnexttick, &Mod_Reset_Seq); 
    Mod_Tick_Seq(nodeID);
    
    
    if (node[nodeID].ticked) {
        if (++(*portcurrentstep) > 15) (*portcurrentstep) = 0;
        node[nodeID].ticked--;
    }
    
    
    
    
    node[nodeID].outbuffer[MOD_SEND_MIDI_NOTE0_CHAN] = *portchan;
    
    node[nodeID].outbuffer[MOD_SEND_MIDI_NOTE0_NOTE] = 
                                                *portnote + *portcurrentstep;
    
    node[nodeID].outbuffer[MOD_SEND_MIDI_NOTE0_VEL] = *portvel;
    
    u8 lenh = (util_s8tou4(*portlenh)+1);
    u8 lenl = (util_s8tou6(*portlenl)+1);
    
    
    u32 lengthref;
    
    if (*portlenref >= DEAD_TIMESTAMP) {
        lengthref = mClock.cyclelen;
    } else {
        lengthref = *portlenref;
    }
    
    unsigned int length = (u16) (lenh * (lengthref / lenl));
    
    node[nodeID].outbuffer[MOD_SEND_MIDI_NOTE0_LENH] = 
                                            (signed char)((length>>8)&0x00ff);
    node[nodeID].outbuffer[MOD_SEND_MIDI_NOTE0_LENL] = 
                                            (signed char)(length&0x00ff);
    
    node[nodeID].outbuffer_req++;
    
}



void Mod_Tick_Seq(unsigned char nodeID) {                                       // set a new timestamp
    u32 *portnexttick;
    portnexttick =  (u32 *) &node[nodeID].ports[MOD_SEQ_PORT_NEXTTICK];
    
    
    Mod_SetNextTick(nodeID, *portnexttick, &Mod_Reset_Seq); 
    
}


void Mod_UnInit_Seq(unsigned char nodeID) {                                     // uninitialize a seq module
}

u32 Mod_Reset_Seq(unsigned char nodeID) {                                       // reset a seq module
    u8 *portcurrentstep;
    portcurrentstep =  (u8 *) &node[nodeID].ports[MOD_SEQ_PORT_CURRENTSTEP];
    u32 *portnexttick;
    portnexttick = (u32 *) &node[nodeID].ports[MOD_SEQ_PORT_NEXTTICK];
    
    *portcurrentstep = 0;
    
    return *portnexttick;
    
}
