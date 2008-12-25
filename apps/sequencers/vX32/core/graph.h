/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
*/


#ifndef _GRAPH_H
#define _GRAPH_H


#define max_nodes 0x3f		// Maximum 254
#define max_indegree 0xfe		// Maximum 254

#define dead_nodeid (max_nodes+1)	// Do not change
#define dead_indegree (max_indegree+1)	// Do not change



typedef struct edge_t {
	unsigned char tailport;
	unsigned char headnodeid;
	unsigned char headport;
	struct edge_t *next;
}edge_t;


typedef struct {
	mios32_midi_port_t port;
	mios32_midi_package_t midi_package;
}outbuffer_t;

typedef struct {
	unsigned char indegree;
	unsigned char indegree_uv;
	unsigned char clocksource;
	unsigned char ticked;
	struct edge_t *edgelist;
	signed char *ports;
	signed char *privvars;
	unsigned char *outbuffer;
	unsigned char outbuffer_size;
	unsigned char outbuffer_type;
	unsigned char outbuffer_req;
	unsigned char process_req;
	unsigned char moduletype;
	unsigned char name[8];
	
}node_t;


typedef struct nodelist_t {
	unsigned char nodeid;
	struct nodelist_t *next;
}nodelist_t;

extern nodelist_t *topolisthead;									// list of topologically sorted nodeids


extern node_t node[max_nodes];										// array of sructs to hold node/module info

extern void graph_init(void);


extern unsigned char node_add(unsigned char moduletype);

unsigned char nodeid_gen(void);


extern unsigned char node_del(unsigned char delnodeid);

unsigned char nodeid_free(unsigned char nodeid);


extern edge_t *edge_add(unsigned char tail_nodeid, unsigned char tail_moduleport, unsigned char head_nodeid, unsigned char head_moduleport);

extern unsigned char edge_del(unsigned char tailnodeid, edge_t *deledge, unsigned char dosort);


extern void nodes_proc(unsigned char startnodeid);


extern unsigned char toposort(void);

void topolist_clear(void);


extern unsigned char node_count;

#endif /* _GRAPH_H */
