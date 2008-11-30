/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
*/


#ifndef _MODULES_H
#define _MODULES_H


#define num_moduletypes 3		// maximum 254
#define dead_moduletype 0xff	// don't change
#define max_patterns 0xff		// maximum 0xff


extern void (*mod_init[num_moduletypes]) (unsigned char nodeid);

extern void (*mod_process_type[num_moduletypes]) (unsigned char nodeid);

extern void (*mod_uninit_type[num_moduletypes]) (unsigned char nodeid);

extern unsigned char mod_ports[num_moduletypes];




void mod_uninit(unsigned char nodeid);


void mod_init_graph(unsigned char nodeid, unsigned char moduletype);

void mod_uninit_graph(unsigned char nodeid, unsigned char moduletype);


extern void mod_set_clocksource(unsigned char nodeid, unsigned char subclock);


extern void mod_process(unsigned char nodeid);

extern void mod_propagate(unsigned char nodeid);


extern void mod_tick(void);

signed char mod_pattern[max_patterns];





void mod_init_clk(unsigned char nodeid);

void mod_proc_clk(unsigned char nodeid);						//do stuff with inputs and push to the outputs 

void mod_uninit_clk(unsigned char nodeid);



void mod_init_seq(unsigned char nodeid);

void mod_proc_seq(unsigned char nodeid);						//do stuff with inputs and push to the outputs 

void mod_uninit_seq(unsigned char nodeid);

	

void mod_init_midiout(unsigned char nodeid);

void mod_proc_midiout(unsigned char nodeid); 					//do stuff with inputs and push to the outputs 

void mod_uninit_midiout(unsigned char nodeid);



#endif /* _MODULES_H */
