// $Id$
/*
 * Header file for USB COM Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_USB_COM_H
#define _MIOS32_USB_COM_H


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// number of USB_COM interfaces (0..1)
#ifndef MIOS32_USB_COM_NUM
#define MIOS32_USB_COM_NUM 1
#endif

// size of IN/OUT pipe
#ifndef MIOS32_USB_COM_DATA_IN_SIZE
#define MIOS32_USB_COM_DATA_IN_SIZE            64
#endif
#ifndef MIOS32_USB_COM_DATA_OUT_SIZE
#define MIOS32_USB_COM_DATA_OUT_SIZE           64
#endif

#ifndef MIOS32_USB_COM_INT_IN_SIZE
#define MIOS32_USB_COM_INT_IN_SIZE             64
#endif


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_USB_COM_Init(u32 mode);

extern s32 MIOS32_USB_COM_CheckAvailable(void);

extern s32 MIOS32_USB_COM_ChangeConnectionState(u8 connected);
extern void MIOS32_USB_COM_EP4_IN_Callback(void);
extern void MIOS32_USB_COM_EP3_OUT_Callback(void);

extern s32 MIOS32_USB_COM_RxBufferFree(u8 usb_com);
extern s32 MIOS32_USB_COM_RxBufferUsed(u8 usb_com);
extern s32 MIOS32_USB_COM_RxBufferGet(u8 usb_com);
extern s32 MIOS32_USB_COM_RxBufferPeek(u8 usb_com);
extern s32 MIOS32_USB_COM_TxBufferFree(u8 usb_com);
extern s32 MIOS32_USB_COM_TxBufferUsed(u8 usb_com);
extern s32 MIOS32_USB_COM_TxBufferPut(u8 usb_com, u8 b);
extern s32 MIOS32_USB_COM_TxBufferPutMore(u8 usb_com, u8 *buffer, u16 len);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* _MIOS32_USB_COM_H */
