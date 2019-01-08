/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


#ifndef _UI_H
#define _UI_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define UI_GRAPHMOD_TIMEOUT 0x00ff

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////


extern void UI_SetBPM(float bpm);

extern void UI_Start(void);

extern void UI_Continue(void);

extern void UI_Stop(void);



extern unsigned char UI_NewModule(unsigned char moduletype);

extern edge_t *UI_NewCable(unsigned char tail_nodeID, unsigned char tail_port
                , unsigned char head_nodeID, unsigned char head_port);

extern unsigned char UI_RemoveModule(unsigned char nodeID);

extern unsigned char UI_RemoveCable(unsigned char tail_nodeID, unsigned char tail_port
                , unsigned char head_nodeID, unsigned char head_port);




extern void UI_SetPort(u32 input, unsigned char nodeID, unsigned char port);

extern char *UI_SetModuleName(unsigned char nodeID, char *newname);



extern u32 UI_GetPort(unsigned char nodeID, unsigned char port);

extern unsigned char UI_GetPortType(unsigned char nodeID, unsigned char port);

extern const char *UI_GetPortTypeName(unsigned char porttype);

extern unsigned char UI_GetModuleID(char *name, unsigned char namelength);

extern unsigned char UI_GetModuleType(unsigned char nodeID);

extern char *UI_GetModuleName(unsigned char nodeID);

extern const char *UI_GetModuleTypeName(unsigned char moduletype);

extern unsigned char UI_GetModuleTypeID(char *getname, unsigned char length);

extern unsigned char UI_GetCableSrcModule(edge_t * edge);

extern unsigned char UI_GetCableDstModule(edge_t * edge);

extern unsigned char UI_GetCableSrcPort(edge_t * edge);

extern unsigned char UI_GetCableDstPort(edge_t * edge);


extern void UI_Tick(void);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _UI_H */
