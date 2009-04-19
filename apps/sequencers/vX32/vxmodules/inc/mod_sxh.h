/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


#ifndef _MOD_SXH_H
#define _MOD_SXH_H




/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define MOD_SXH_PRIVVARS 8                                                      // number of private var bytes in this module
#define MOD_SXH_PORTS 8                                                         // number of port bytes in this module
#define MOD_SXH_BUFFERS 0                                                       // outbuffer count



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern mod_moduledata_t mod_SxH_ModuleData;                                     // array holding this module's data

extern mod_portdata_t mod_SxH_PortTypes[MOD_SXH_PORTS];                         // array holding this module's ports' data

extern mod_portdata_t mod_SxH_PrivVarTypes[MOD_SXH_PRIVVARS];                   // array holding this module's privvars' data



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

void Mod_Init_SxH(unsigned char nodeID);                                        // function to init this module

void Mod_Proc_SxH(unsigned char nodeID);                                        // do stuff with inputs and push to the outputs 

void Mod_Tick_SxH(unsigned char nodeID);                                        // set a new timestamp

void Mod_UnInit_SxH(unsigned char nodeID);                                      // function to uninit this module

u32 Mod_Reset_SxH(unsigned char nodeID);                                        // function to reset this module



#endif /* _MOD_SXH_H */
