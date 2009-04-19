/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


#ifndef _MOD_MIDIOUT_H
#define _MOD_MIDIOUT_H




/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define MOD_MIDIOUT_PRIVVARS 0                                                  // number of private var bytes in this module
#define MOD_MIDIOUT_PORTS 12                                                    // number of port bytes in this module
#define MOD_MIDIOUT_BUFFERS 3                                                   // outbuffer count

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern mod_moduledata_t mod_MIDIOut_ModuleData;                                 // array holding this module's data

extern mod_portdata_t mod_MIDIOut_PortTypes[MOD_MIDIOUT_PORTS];                 // array holding this module's ports' data

extern mod_portdata_t mod_MIDIOut_PrivVarTypes[MOD_MIDIOUT_PRIVVARS];           // array holding this module's privvars' data



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

void Mod_Init_MIDIOut(unsigned char nodeID);                                    // function to init this module

void Mod_Proc_MIDIOut(unsigned char nodeID);                                    // do stuff with inputs and push to the outputs 

void Mod_Tick_MIDIOut(unsigned char nodeID);                                    // set a new timestamp

void Mod_UnInit_MIDIOut(unsigned char nodeID);                                  // function to uninit this module



#endif /* _MOD_MIDIOUT_H */
