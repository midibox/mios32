/* $Id:  $ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
*/


#ifndef _MOD_MIDIOUT_H
#define _MOD_MIDIOUT_H




void mod_init_midiout(unsigned char nodeid);

void mod_proc_midiout(unsigned char nodeid); 					//do stuff with inputs and push to the outputs 

void mod_uninit_midiout(unsigned char nodeid);



#endif /* _MOD_MIDIOUT_H */
