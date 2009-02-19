/* $Id: modules.h 251 2009-01-06 13:30:53Z stryd_one $ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/

extern void mod_xlate_time_time (unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port);	 // timestamp - timestamp
extern void mod_xlate_time_value(unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port);	 // timestamp - value
extern void mod_xlate_time_flag(unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port);	 // timestamp - flag
extern void mod_xlate_value_time(unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port);	 // value - timestamp
extern void mod_xlate_value_value(unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port);	 // value - value
extern void mod_xlate_value_flag(unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port);	 // value - flag
extern void mod_xlate_flag_time(unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port);	 // flag - timestamp
extern void mod_xlate_flag_value(unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port);	 // flag - value
extern void mod_xlate_flag_flag(unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port);	 // flag - flag


extern void mod_deadport_time(unsigned char nodeid, unsigned char port);
extern void mod_deadport_value(unsigned char nodeid, unsigned char port);
extern void mod_deadport_flag(unsigned char nodeid, unsigned char port);
