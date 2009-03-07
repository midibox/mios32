/* $Id:  $ */
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

#define mod_seq_privvars 32														// number of private var bytes in this module
#define mod_seq_ports 32														// number of port bytes in this module



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

extern const mod_moduledata_t mod_seq_moduledata;								// array holding this module's data

extern const mod_portdata_t mod_seq_porttypes[mod_seq_ports];					// array holding this module's ports' data




/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

void mod_init_seq(unsigned char nodeid);										// function to init this module

void mod_proc_seq(unsigned char nodeid);										//do stuff with inputs and push to the outputs 

void mod_uninit_seq(unsigned char nodeid);										// function to uninit this module

u32 mod_reset_seq(unsigned char nodeid);										// function to reset this module



#endif /* _MOD_SEQ_H */
