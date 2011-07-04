// $Id$
/*
 * Header file for Mass Storage Device Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MSD_H
#define _MSD_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

/* defines how many bytes are sent per packet */
#define MSD_BULK_MAX_PACKET_SIZE  0x40

/* max. number of supported logical units */
#define MSD_NUM_LUN 1

/* defines how many endpoints are used by the device */
#define MSD_EP_NUM 3

/* buffer table base address */

#define MSD_BTABLE_ADDRESS      (0x00)

/* EP0  */
/* rx/tx buffer base address */
#define MSD_ENDP0_RXADDR        (0x18)
#define MSD_ENDP0_TXADDR        (0x58)

/* EP1  */
/* tx buffer base address */
#define MSD_ENDP1_TXADDR        (0x98)

/* EP2  */
/* Rx buffer base address */
#define MSD_ENDP2_RXADDR        (0xD8)



/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MSD_Init(u32 mode);
extern s32 MSD_Periodic_mS(void);

extern s32 MSD_CheckAvailable(void);

extern s32 MSD_LUN_AvailableSet(u8 lun, u8 available);
extern s32 MSD_LUN_AvailableGet(u8 lun);

extern s32 MSD_RdLEDGet(u16 lag_ms);
extern s32 MSD_WrLEDGet(u16 lag_ms);




/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MSD_H */
