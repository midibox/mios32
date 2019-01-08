/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


#ifndef _MOD_SCLK_H
#define _MOD_SCLK_H




/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define mod_sclk_ports 16                                                       // number of port bytes in this module
#define mod_sclk_privvars 16                                                    // number of private var bytes in this module
#define MOD_SCLK_BUFFERS 0                                                      // outbuffer count


#define MOD_SCLK_PORT_STATUS 0
#define MOD_SCLK_PORT_NUMERATOR 1
#define MOD_SCLK_PORT_DENOMINATOR 2
#define MOD_SCLK_PORT_UNUSED 3 //padding for byte alignment paranoia
#define MOD_SCLK_PORT_CYCLELEN 4
//u32
#define MOD_SCLK_PORT_NEXTTICK 8
//u32
#define MOD_SCLK_PORT_SYNCTICK 12
//u32


#define MOD_SCLK_PRIV_QUOTIENT 0
//int
#define MOD_SCLK_PRIV_MODULUS 2
//int
#define MOD_SCLK_PRIV_MODCOUNTER 4
//int
#define MOD_SCLK_PRIV_COUNTDOWN 6
//int
#define MOD_SCLK_PRIV_STATUS 8
//char
#define MOD_SCLK_PRIV_UNUSED 9 //padding for byte alignment paranoia
//char
#define MOD_SCLK_PRIV_UNUSED0 10 //padding for byte alignment paranoia
//char
#define MOD_SCLK_PRIV_UNUSED1 11 //padding for byte alignment paranoia
//char
#define MOD_SCLK_PRIV_LASTSYNC 12
//u32

#define MOD_SCLK_PORT_STATUS_BIT_ACTIVE 7
#define MOD_SCLK_PORT_STATUS_BIT_SYNC 6
#define MOD_SCLK_PORT_STATUS_BIT_HARDSYNC 5
#define MOD_SCLK_PORT_STATUS_BIT_GOTSYNC 4



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern mod_moduledata_t mod_SClk_ModuleData;                              // array holding this module's data

extern mod_portdata_t mod_SClk_PortTypes[mod_sclk_ports];                 // array holding this module's ports' data

extern mod_portdata_t mod_SClk_PrivVarTypes[mod_sclk_privvars];           // array holding this module's privvars' data



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

void Mod_Init_SClk(unsigned char nodeID);                                       // function to init this module

void Mod_Proc_SClk(unsigned char nodeID);                                       // do stuff with inputs and push to the outputs 

void Mod_Tick_SClk(unsigned char nodeID);                                       // set a new timestamp

void Mod_UnInit_SClk(unsigned char nodeID);                                     // function to uninit this module

u32 Mod_Reset_SClk(unsigned char nodeID);                                       // function to reset this module


u32 Mod_SClk_GetNextTick(unsigned char nodeID);                                 // module-specific functions

void Mod_SClk_ResetCounters(unsigned char nodeID);                              // module-specific functions


#endif /* _MOD_SCLK_H */
