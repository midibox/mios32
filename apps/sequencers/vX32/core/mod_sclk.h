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




#define mod_sclk_ports 16
#define mod_sclk_privvars 16


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


extern const unsigned char mod_sclk_porttypes[mod_sclk_ports];


void mod_init_sclk(unsigned char nodeid);

void mod_proc_sclk(unsigned char nodeid); 					//do stuff with inputs and push to the outputs 

void mod_uninit_sclk(unsigned char nodeid);

u32 mod_reset_sclk(unsigned char nodeid);

u32 mod_sclk_getnexttick(unsigned char nodeid);

void mod_sclk_resetcounters(unsigned char nodeid);

#endif /* _MOD_SCLK_H */
