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




#define mod_seq_privvars 32
#define mod_seq_ports 32

extern const unsigned char mod_seq_porttypes[mod_seq_ports];



void mod_init_seq(unsigned char nodeid);

void mod_proc_seq(unsigned char nodeid);						//do stuff with inputs and push to the outputs 

void mod_uninit_seq(unsigned char nodeid);

u32 mod_reset_seq(unsigned char nodeid);

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




#endif /* _MOD_SEQ_H */
