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




#define mod_midiout_privvars 0
#define mod_midiout_ports 12

extern const unsigned char mod_midiout_porttypes[mod_midiout_ports];



void mod_init_midiout(unsigned char nodeid);

void mod_proc_midiout(unsigned char nodeid); 					//do stuff with inputs and push to the outputs 

void mod_uninit_midiout(unsigned char nodeid);



#endif /* _MOD_MIDIOUT_H */
