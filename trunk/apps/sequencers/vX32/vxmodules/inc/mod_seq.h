/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


#ifndef _MOD_SEQ_H
#define _MOD_SEQ_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define MOD_SEQ_PRIVVARS 32                                                     // number of private var bytes in this module
#define MOD_SEQ_PORTS 32                                                        // number of port bytes in this module
#define MOD_SEQ_BUFFERS 3                                                       // outbuffer count



#define MOD_SEQ_PORT_NOTE0_STATUS 0
#define MOD_SEQ_PORT_NOTE0_CHAN 1
#define MOD_SEQ_PORT_NOTE0_NOTE 2
#define MOD_SEQ_PORT_NOTE0_VEL 3
#define MOD_SEQ_PORT_NOTE0_LENREF 4
//long
#define MOD_SEQ_PORT_NEXTTICK 8
//long
#define MOD_SEQ_PORT_CURRENTSTEP 12
//long
#define MOD_SEQ_PORT_NOTE0_LENH 16
#define MOD_SEQ_PORT_NOTE0_LENL 17



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern mod_moduledata_t mod_Seq_ModuleData;                                     // array holding this module's data

extern mod_portdata_t mod_Seq_PortTypes[MOD_SEQ_PORTS];                         // array holding this module's ports' data

extern mod_portdata_t mod_Seq_PrivVarTypes[MOD_SEQ_PRIVVARS];                   // array holding this module's privvars' data




/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

void Mod_Init_Seq(unsigned char nodeID);                                        // function to init this module

void Mod_Proc_Seq(unsigned char nodeID);                                        // do stuff with inputs and push to the outputs 

void Mod_Tick_Seq(unsigned char nodeID);                                        // set a new timestamp

void Mod_UnInit_Seq(unsigned char nodeID);                                      // function to uninit this module

u32 Mod_Reset_Seq(unsigned char nodeID);                                        // function to reset this module



#endif /* _MOD_SEQ_H */
