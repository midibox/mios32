/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


#ifndef _GRAPH_H
#define _GRAPH_H


#define max_nodes 0x3f		// Maximum 254. The basic struct is statically allocated
#define max_indegree 0xfe		// Maximum 254, They're dynamically allocated

#define dead_nodeid (max_nodes+1)	// Do not change
#define dead_indegree (max_indegree+1)	// Do not change

#define nodeidinuse_mod (max_nodes%8)	// Do not change
#if nodeidinuse_mod > 0
#define nodeidinuse_bytes ((max_nodes/8)+1)
#else
#define nodeidinuse_bytes (max_nodes/8)
#endif



typedef struct edge_t {
	unsigned char tailnodeid;										// node we're going to
	unsigned char tailport;											// port we're going from (we already know the node id, it's hosting this edge
	unsigned char headnodeid;										// node we're going to
	unsigned char headport;											// port we're going to
	void (*msgxlate) (unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port);				// function pointer calculated at edge creation based on port types
	struct edge_t *next;											// pointer to next edge in the list
}edge_t;


typedef struct {
	mios32_midi_port_t port;
	mios32_midi_package_t midi_package;
}outbuffer_t;

typedef struct {
	struct edge_t *edgelist;
	unsigned char indegree;
	unsigned char indegree_uv;
	signed char *ports;
	signed char *privvars;
	signed char *outbuffer;
	unsigned char outbuffer_count;
	unsigned char outbuffer_size;
	unsigned char outbuffer_type;
	unsigned char outbuffer_req;
	unsigned char moduletype;
	unsigned char name[8];
	unsigned char ticked;
	unsigned char process_req;
	u32 nexttick;
	u32 downstreamtick;
	
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


extern edge_t *edge_add(unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port);

extern unsigned char edge_del(unsigned char tailnodeid, edge_t *deledge, unsigned char dosort);

void (*edge_get_xlator(unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port)) (unsigned char tail_nodeid, unsigned char tail_port, unsigned char head_nodeid, unsigned char head_port);



extern unsigned char toposort(void);

void topolist_clear(void);


extern unsigned char node_count;

#endif /* _GRAPH_H */
