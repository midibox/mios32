// $Id: mios32_com.h 307 2009-01-17 17:00:40Z tk $
/*
 * Header file for COM layer
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_COM_H
#define _MIOS32_COM_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// the default COM port for COM output
#ifndef MIOS32_COM_DEFAULT_PORT
#define MIOS32_COM_DEFAULT_PORT USB0
#endif

// the default COM port for debugging output via printf
#ifndef MIOS32_COM_DEBUG_PORT
#define MIOS32_COM_DEBUG_PORT USB0
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////


typedef enum {
  COM_DEFAULT = 0x00,
  COM_DEBUG = 0x01,

  COM_USB0 = 0x10,
  COM_USB1 = 0x11,
  COM_USB2 = 0x12,
  COM_USB3 = 0x13,
  COM_USB4 = 0x14,
  COM_USB5 = 0x15,
  COM_USB6 = 0x16,
  COM_USB7 = 0x17,

  COM_UART0 = 0x20,
  COM_UART1 = 0x21,

  COM_IIC0 = 0x30,
  COM_IIC1 = 0x31,
  COM_IIC2 = 0x32,
  COM_IIC3 = 0x33,
  COM_IIC4 = 0x34,
  COM_IIC5 = 0x35,
  COM_IIC6 = 0x36,
  COM_IIC7 = 0x37
} mios32_com_port_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_COM_Init(u32 mode);

extern s32 MIOS32_COM_CheckAvailable(mios32_com_port_t port);

extern s32 MIOS32_COM_SendChar_NonBlocking(mios32_com_port_t port, char c);
extern s32 MIOS32_COM_SendChar(mios32_com_port_t port, char c);
extern s32 MIOS32_COM_SendBuffer_NonBlocking(mios32_com_port_t port, u8 *buffer, u16 len);
extern s32 MIOS32_COM_SendBuffer(mios32_com_port_t port, u8 *buffer, u16 len);
extern s32 MIOS32_COM_SendString_NonBlocking(mios32_com_port_t port, char *str);
extern s32 MIOS32_COM_SendString(mios32_com_port_t port, char *str);
extern s32 MIOS32_COM_SendFormattedString_NonBlocking(mios32_com_port_t port, char *format, ...);
extern s32 MIOS32_COM_SendFormattedString(mios32_com_port_t port, char *format, ...);

extern s32 MIOS32_COM_Receive_Handler(void *callback);

extern s32 MIOS32_COM_DefaultPortSet(mios32_com_port_t port);
extern mios32_com_port_t MIOS32_COM_DefaultPortGet(void);

extern s32 MIOS32_COM_DebugPortSet(mios32_com_port_t port);
extern mios32_com_port_t MIOS32_COM_DebugPortGet(void);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_COM_H */
