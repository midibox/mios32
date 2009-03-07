/* $Id:  $ */
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

#define mod_midiout_privvars 0													// number of private var bytes in this module
#define mod_midiout_ports 12													// number of port bytes in this module



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern const mod_moduledata_t mod_midiout_moduledata;							// array holding this module's data


extern const mod_portdata_t mod_midiout_porttypes[mod_midiout_ports];			// array holding this module's ports' data



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

void mod_init_midiout(unsigned char nodeid);									// function to init this module

void mod_proc_midiout(unsigned char nodeid);									// do stuff with inputs and push to the outputs 

void mod_uninit_midiout(unsigned char nodeid);									// function to uninit this module



#endif /* _MOD_MIDIOUT_H */
