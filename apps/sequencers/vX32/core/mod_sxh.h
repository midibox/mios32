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



#define mod_sxh_privvars 8
#define mod_sxh_ports 8

extern const unsigned char mod_sxh_porttypes[mod_sxh_ports];



void mod_init_sxh(unsigned char nodeid);

void mod_proc_sxh(unsigned char nodeid); 					//do stuff with inputs and push to the outputs 

void mod_uninit_sxh(unsigned char nodeid);



#endif /* _MOD_SXH_H */
