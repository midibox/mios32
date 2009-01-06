/* $Id:  $ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
*/


#ifndef _MOD_SEQ_H
#define _MOD_SEQ_H


void mod_init_seq(unsigned char nodeid);

void mod_proc_seq(unsigned char nodeid);						//do stuff with inputs and push to the outputs 

void mod_uninit_seq(unsigned char nodeid);


#define MOD_SEQ_PORT_NOTE0_CHAN 0
#define MOD_SEQ_PORT_NOTE0_NOTE 1
#define MOD_SEQ_PORT_NOTE0_VEL 2
#define MOD_SEQ_PORT_NOTE0_LENH 3
#define MOD_SEQ_PORT_NOTE0_LENL 4




#endif /* _MOD_SEQ_H */
