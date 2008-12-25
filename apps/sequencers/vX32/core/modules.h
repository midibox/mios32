/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
*/


#ifndef _MODULES_H
#define _MODULES_H


#define max_moduletypes 3		// maximum 254
#define max_buffertypes 3		// maximum 254
#define max_patterns 0xfe		// maximum 254

#define dead_moduletype (max_moduletypes+1)	// don't change
#define dead_buffertype (max_buffertypes+1)	// don't change
#define dead_pattern (max_patterns+1)		// don't change
#define dead_value (-128)		// don't change


extern void (*const mod_init[max_moduletypes]) (unsigned char nodeid);

extern void (*const mod_process_type[max_moduletypes]) (unsigned char nodeid);

extern void (*const mod_uninit_type[max_moduletypes]) (unsigned char nodeid);

extern void (*const mod_send_buffer_type[max_buffertypes]) (unsigned char nodeid);

extern const unsigned char mod_ports[max_moduletypes];

extern const unsigned char mod_outbuffer_size[max_moduletypes];



void mod_uninit(unsigned char nodeid);


void mod_init_graph(unsigned char nodeid, unsigned char moduletype);

void mod_uninit_graph(unsigned char nodeid, unsigned char moduletype);


extern void mod_set_clocksource(unsigned char nodeid, unsigned char subclock);



extern void mod_process(unsigned char nodeid);

extern void mod_propagate(unsigned char nodeid);

extern void mod_send_buffer(unsigned char nodeid);



void mod_send_midi(unsigned char nodeid);
	
void mod_send_com(unsigned char nodeid);
	
void mod_send_dummy(unsigned char nodeid);



extern void mod_tick(void);




extern signed char *mod_pattern[max_patterns];





void mod_init_clk(unsigned char nodeid);

void mod_proc_clk(unsigned char nodeid);						//do stuff with inputs and push to the outputs 

void mod_uninit_clk(unsigned char nodeid);



void mod_init_seq(unsigned char nodeid);

void mod_proc_seq(unsigned char nodeid);						//do stuff with inputs and push to the outputs 

void mod_uninit_seq(unsigned char nodeid);


#define MOD_SEQ_PORT_MIN_LEN 0
#define MOD_SEQ_PORT_NOTE0_NOTE 1
#define MOD_SEQ_PORT_NOTE0_VEL 2
#define MOD_SEQ_PORT_NOTE0_LEN 3

void mod_init_midiout(unsigned char nodeid);

void mod_proc_midiout(unsigned char nodeid); 					//do stuff with inputs and push to the outputs 

void mod_uninit_midiout(unsigned char nodeid);



#endif /* _MODULES_H */
