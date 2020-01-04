// $Id: mios32_usb_com.h 1156 2011-03-27 18:45:18Z tk $
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

#ifndef _MIOS32_USB_HID_H
#define _MIOS32_USB_HID_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// size of buffer
#ifndef MIOS32_USB_HID_DATA_SIZE
#define MIOS32_USB_HID_DATA_SIZE   8
#endif

// HID Process Delay
#ifndef MIOS32_USB_HID_MIN_POLL
//#define MIOS32_USB_HID_MIN_POLL    5
#endif


typedef union{
  struct {
    u8 connected:1;
    u8 buttons:7;
    u16 axis;   // do not use
    u16 wheel;   // do not use
  };
  struct
  {
    u8   reserved:1;
    u8   left:1;
    u8   right:1;
    u8   middle:1;
    u8   back:1;
    u8   forward:1;
    u8   dummy:2;
    s8   x;
    s8   y;
    s8   z;
    s8   tilt;
  };
}
mios32_mouse_data_t;

typedef union {
  struct {
    u16 ALL;
  };
  struct {
    u8 connected:1;
    u8 locks:7;
    u8 modifiers;
  };
  struct
  {
    u8   dummy:1;
    u8   num_lock:1;
    u8   caps_lock:1;
    u8   scroll_lock:1;
    u8   reserved:4;
    u8   left_ctrl:1;
    u8   left_shift:1;
    u8   left_alt:1;
    u8   left_gui:1;
    u8   right_ctrl:1;
    u8   right_shift:1;
    u8   right_alt:1;
    u8   right_gui:1;
  };
}mios32_kbd_state_t;

typedef struct
{
  u8    code;
  u8    value;
  char  character;
}
mios32_kbd_key_t;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_USB_HID_Init(u32 mode);

extern s32 MIOS32_USB_HID_CheckAvailable(void);

extern s32 MIOS32_USB_HID_MouseCallback_Init(void (*mouse_callback)(mios32_mouse_data_t mouse_data));
extern s32 MIOS32_USB_HID_KeyboardCallback_Init(void (*keyboard_callback)(mios32_kbd_state_t kbd_state, mios32_kbd_key_t kbd_key));

////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_USB_HID_H */
