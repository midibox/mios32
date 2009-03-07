/* $Id:  $ */
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

#define mod_sclk_ports 16														// number of port bytes in this module
#define mod_sclk_privvars 16													// number of private var bytes in this module


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

extern const mod_moduledata_t mod_sclk_moduledata;								// array holding this module's data

extern const mod_portdata_t mod_sclk_porttypes[mod_sclk_ports];					// array holding this module's ports' data



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

void mod_init_sclk(unsigned char nodeid);										// function to init this module

void mod_proc_sclk(unsigned char nodeid); 										//do stuff with inputs and push to the outputs 

void mod_uninit_sclk(unsigned char nodeid);										// function to uninit this module

u32 mod_reset_sclk(unsigned char nodeid);										// function to reset this module


u32 mod_sclk_getnexttick(unsigned char nodeid);									// module-specific functions

void mod_sclk_resetcounters(unsigned char nodeid);								// module-specific functions


#endif /* _MOD_SCLK_H */
