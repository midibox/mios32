/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


#ifndef _MOD_XLATE_H
#define _MOD_XLATE_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#define MAX_PORTTYPES 4                                                         // number of port types. don't go crazy here, keep it minimal

#define MOD_PORTTYPE_TIMESTAMP 0                                                // must be MAX_PORTTYPES defines here
#define MOD_PORTTYPE_VALUE 1
#define MOD_PORTTYPE_PACKAGE 2
#define MOD_PORTTYPE_FLAG 3


                                                                                // don't change defines below here
#define PORTTYPE_NAME_STRING_LENGTH 8                                           // number of characters in a module's name string
#define DEAD_PORTTYPE (MAX_PORTTYPES+1)                                         // don't change
#define DEAD_VALUE (signed char)(-128)                                          // don't change
#define DEAD_PACKAGE 0xFFFFFFFF                                                 // don't change
#define DEAD_TIMESTAMP 0xFFFFFFF0                                               // don't change
#define RT_TIMESTAMP 0xFFFFFFFE                                                 // don't change
#define RESET_TIMESTAMP 0xFFFFFFFF                                              // don't change



/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern const char mod_PortType_NameS[PORTTYPE_NAME_STRING_LENGTH];              // array of strings which are names for the port types

extern void (*const mod_Xlate_Table[MAX_PORTTYPES][MAX_PORTTYPES])              // array of translator functions from any port type to any port type
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // Do it like this: return mod_Xlate_Table[tail_port_type][head_port_type];

extern void (*const mod_DeadPort_Table[(MAX_PORTTYPES)]) 
                        (unsigned char nodeID, unsigned char port);             // array of functions for writing dead values to ports

extern void (*const mod_SetPort_Table[(MAX_PORTTYPES)]) 
                    (u32 input, unsigned char nodeID, unsigned char port);      // array of functions for writing values to ports

extern u32 (*const mod_GetPort_Table[(MAX_PORTTYPES)]) 
                    (unsigned char nodeID, unsigned char port);                 // array of functions for getting values from ports


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////


unsigned char Mod_GetPortType
                        (unsigned char nodeID, unsigned char port);             // returns port type

extern const char *Mod_GetPortTypeName
                        (unsigned char porttype);                               // returns the name of the port type


extern void Mod_Xlate_Time_Time 
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // timestamp - timestamp

extern void Mod_Xlate_Time_Value
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // timestamp - value

extern void Mod_Xlate_Time_Package
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // timestamp - value

extern void Mod_Xlate_Time_Flag
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // timestamp - flag


extern void Mod_Xlate_Value_Time
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // value - timestamp

extern void Mod_Xlate_Value_Value
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // value - value

extern void Mod_Xlate_Value_Package
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // value - value

extern void Mod_Xlate_Value_Flag
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // value - flag


extern void Mod_Xlate_Package_Time
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // value - timestamp

extern void Mod_Xlate_Package_Value
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // value - value

extern void Mod_Xlate_Package_Package
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // value - value

extern void Mod_Xlate_Package_Flag
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // value - flag


extern void Mod_Xlate_Flag_Time
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // flag - timestamp

extern void Mod_Xlate_Flag_Value
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // flag - value

extern void Mod_Xlate_Flag_Package
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // flag - value

extern void Mod_Xlate_Flag_Flag
                        (unsigned char tail_nodeID, unsigned char tail_port,
                        unsigned char head_nodeID, unsigned char head_port);    // flag - flag




extern void Mod_DeadPort(unsigned char nodeID, unsigned char port);

extern void Mod_DeadPort_Time(unsigned char nodeID, unsigned char port);

extern void Mod_DeadPort_Value(unsigned char nodeID, unsigned char port);

extern void Mod_DeadPort_Package(unsigned char nodeID, unsigned char port);

extern void Mod_DeadPort_Flag(unsigned char nodeID, unsigned char port);



extern void Mod_SetPort
                        (u32 input, unsigned char nodeID, unsigned char port);

extern void Mod_SetPort_Time
                        (u32 input, unsigned char nodeID, unsigned char port);

extern void Mod_SetPort_Value
                        (u32 input, unsigned char nodeID, unsigned char port);

extern void Mod_SetPort_Package
                        (u32 input, unsigned char nodeID, unsigned char port);

extern void Mod_SetPort_Flag
                        (u32 input, unsigned char nodeID, unsigned char port);



extern u32 Mod_GetPort(unsigned char nodeID, unsigned char port);

extern u32 Mod_GetPort_Time(unsigned char nodeID, unsigned char port);

extern u32 Mod_GetPort_Value(unsigned char nodeID, unsigned char port);

extern u32 Mod_GetPort_Package(unsigned char nodeID, unsigned char port);

extern u32 Mod_GetPort_Flag(unsigned char nodeID, unsigned char port);




#endif /* _MOD_XLATE_H */
