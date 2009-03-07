/* $Id:  $ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
*/


#ifndef _MOD_SXH_H
#define _MOD_SXH_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define mod_sxh_privvars 8														// number of private var bytes in this module
#define mod_sxh_ports 8															// number of port bytes in this module



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern const mod_moduledata_t mod_sxh_moduledata;								// array holding this module's data

extern const mod_portdata_t mod_sxh_porttypes[mod_sxh_ports];					// array holding this module's ports' data



void mod_init_sxh(unsigned char nodeid);										// function to init this module

void mod_proc_sxh(unsigned char nodeid); 										//do stuff with inputs and push to the outputs 

void mod_uninit_sxh(unsigned char nodeid);										// function to uninit this module



#endif /* _MOD_SXH_H */
