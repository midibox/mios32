// $Id: mios32_usb_com.c 1800 2013-06-02 22:09:03Z tk $
//! \defgroup MIOS32_USB_HID
//!
//! USB HID layer for MIOS32
//! 
//! Only supported for STM32F4
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files & defines
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_USB_HOST) && !defined(MIOS32_DONT_USE_USB_HID)

#include <usb_conf.h>
#include <usbh_stdreq.h>
#include <usbh_ioreq.h>
#include <usb_bsp.h>
#include <usbh_core.h>
#include <usbh_conf.h>
#include <usb_hcd_int.h>
#include <usbh_hcs.h>

#include <string.h>

#define  LE32(addr)             (((u32)(*((u8 *)(addr))))\
                                + (((u32)(*(((u8 *)(addr)) + 1))) << 8)\
                                + (((u32)(*(((u8 *)(addr)) + 2))) << 16)\
                                + (((u32)(*(((u8 *)(addr)) + 3))) << 24))

#define  LES32(addr)             (((s32)(*((u8 *)(addr))))\
                                + (((s32)(*(((u8 *)(addr)) + 1))) << 8)\
                                + (((s32)(*(((u8 *)(addr)) + 2))) << 16)\
                                + (((s32)(*(((u8 *)(addr)) + 3))) << 24))

#define USB_HID_BOOT_CODE            0x01
#define USB_HID_KEYBRD_BOOT_CODE     0x01
#define USB_HID_MOUSE_BOOT_CODE      0x02
#define USB_HID_GAMPAD_BOOT_CODE     0x05

#define USB_HID_REQ_GET_REPORT       0x01
#define USB_HID_GET_IDLE             0x02
#define USB_HID_GET_PROTOCOL         0x03
#define USB_HID_SET_REPORT           0x09
#define USB_HID_SET_IDLE             0x0A
#define USB_HID_SET_PROTOCOL         0x0B

#define HID_MIN_POLL                 10

#define KBD_LEFT_CTRL                0x01
#define KBD_LEFT_SHIFT               0x02
#define KBD_LEFT_ALT                 0x04
#define KBD_LEFT_GUI                 0x08
#define KBD_RIGHT_CTRL               0x10
#define KBD_RIGHT_SHIFT              0x20
#define KBD_RIGHT_ALT                0x40
#define KBD_RIGHT_GUI                0x80

#define KBR_MAX_NBR_PRESSED          6

#ifndef MIOS32_USB_HID_QWERTY_KEYBOARD
#define MIOS32_USB_HID_AZERTY_KEYBOARD
#endif

/////////////////////////////////////////////////////////////////////////////
// Local constants
/////////////////////////////////////////////////////////////////////////////
static const uint8_t USB_HID_MouseStatus[]    = "[USBH_HID]Mouse connected\n";
static const uint8_t USB_HID_KeybrdStatus[]   = "[USBH_HID]Keyboard connected\n";
//static const uint8_t USB_HID_GampadStatus[]   = "> GamePad connected\n";

static  const  uint8_t  HID_KEYBRD_Codes[] = {
    0,    0,    0,    0,   31,   50,   48,   33,
   19,   34,   35,   36,   24,   37,   38,   39,       /* 0x00 - 0x0F */
   52,   51,   25,   26,   17,   20,   32,   21,
   23,   49,   18,   47,   22,   46,    2,    3,       /* 0x10 - 0x1F */
    4,    5,    6,    7,    8,    9,   10,   11,
   43,  110,   15,   16,   61,   12,   13,   27,       /* 0x20 - 0x2F */
   28,   29,   42,   40,   41,    1,   53,   54,
   55,   30,  112,  113,  114,  115,  116,  117,       /* 0x30 - 0x3F */
  118,  119,  120,  121,  122,  123,  124,  125,
  126,   75,   80,   85,   76,   81,   86,   89,       /* 0x40 - 0x4F */
   79,   84,   83,   90,   95,  100,  105,  106,
  108,   93,   98,  103,   92,   97,  102,   91,       /* 0x50 - 0x5F */
   96,  101,   99,  104,   45,  129,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,       /* 0x60 - 0x6F */
    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,       /* 0x70 - 0x7F */
    0,    0,    0,    0,    0,  107,    0,   56,
    0,    0,    0,    0,    0,    0,    0,    0,       /* 0x80 - 0x8F */
    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,       /* 0x90 - 0x9F */
    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,       /* 0xA0 - 0xAF */
    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,       /* 0xB0 - 0xBF */
    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,       /* 0xC0 - 0xCF */
    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,       /* 0xD0 - 0xDF */
   58,   44,   60,  127,   64,   57,   62,  128        /* 0xE0 - 0xE7 */
};

#ifdef QWERTY_KEYBOARD
static  const  char  HID_KEYBRD_Key[] = {
  '\0',  '`',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',
   '-',  '=', '\0', '\r', '\t',  'q',  'w',  'e',  'r',  't',  'y',  'u',
   'i',  'o',  'p',  '[',  ']', '\\', '\0',  'a',  's',  'd',  'f',  'g',
   'h',  'j',  'k',  'l',  ';', '\'', '\0', '\n', '\0', '\0',  'z',  'x',
   'c',  'v',  'b',  'n',  'm',  ',',  '.',  '/', '\0', '\0', '\0', '\0',
  '\0',  ' ', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\r', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0',  '7',  '4',  '1', '\0',  '/',
   '8',  '5',  '2',  '0',  '*',  '9',  '6',  '3',  '.',  '-',  '+', '\0',
  '\n', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
};
static  const  char  HID_KEYBRD_ShiftKey[] = {
  '\0',  '~',  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',
   '_',  '+', '\0', '\0', '\0', 'Q',   'W',  'E',  'R',  'T',  'Y',  'U',
   'I',  'O',  'P',  '{',  '}',  '|', '\0',  'A',  'S',  'D',  'F',  'G',
   'H',  'J',  'K',  'L',  ':',  '"', '\0', '\n', '\0', '\0',  'Z',  'X',
   'C',  'V',  'B',  'N',  'M',  '<',  '>',  '?', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
};
#else
static  const  char  HID_KEYBRD_Key[] = {
  '\0',  '`',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',
   '-',  '=', '\0', '\r', '\t',  'a',  'z',  'e',  'r',  't',  'y',  'u',
   'i',  'o',  'p',  '[',  ']', '\\', '\0',  'q',  's',  'd',  'f',  'g',
   'h',  'j',  'k',  'l',  'm', '\0', '\0', '\n', '\0', '\0',  'w',  'x',
   'c',  'v',  'b',  'n',  ',',  ';',  ':',  '!', '\0', '\0', '\0', '\0',
  '\0',  ' ', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\r', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0',  '7',  '4',  '1', '\0',  '/',
   '8',  '5',  '2',  '0',  '*',  '9',  '6',  '3',  '.',  '-',  '+', '\0',
  '\n', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
};
static  const  char  HID_KEYBRD_ShiftKey[] = {
  '\0',  '~',  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',
   '_',  '+', '\0', '\0', '\0',  'A',  'Z',  'E',  'R',  'T',  'Y',  'U',
   'I',  'O',  'P',  '{',  '}',  '*', '\0',  'Q',  'S',  'D',  'F',  'G',
   'H',  'J',  'K',  'L',  'M',  '%', '\0', '\n', '\0', '\0',  'W',  'X',
   'C',  'V',  'B',  'N',  '?',  '.',  '/', '\0',  '\0', '\0','\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
};
#endif

/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////
/* States for HID State Machine */
typedef enum
{
  HID_IDLE= 0,
  HID_SEND_DATA,
  HID_BUSY,
  HID_GET_DATA,
  HID_SYNC,
  HID_POLL,
  HID_ERROR,
}
HID_State;

typedef enum
{
  HID_REQ_IDLE = 0,
  HID_REQ_GET_REPORT_DESC,
  HID_REQ_GET_HID_DESC,
  HID_REQ_SET_IDLE,
  HID_REQ_SET_PROTOCOL,
  HID_REQ_SET_REPORT,
}
HID_CtlState;

typedef struct HID_cb
{
  void  (*Init)   (USB_OTG_CORE_HANDLE *pdev , void  *phost);
  void  (*Decode) (USB_OTG_CORE_HANDLE *pdev , void  *phost, uint8_t *data);

} USB_HID_cb_t;

/* Structure for HID process */
typedef struct _HID_Process
{
  uint8_t              transfer_possible;
  volatile uint8_t     start_toggle;
  uint8_t              buff[MIOS32_USB_HID_DATA_SIZE];
  uint8_t              hc_num_in;
  uint8_t              hc_num_out;
  HID_State            state;
  uint8_t              HIDIntOutEp;
  uint8_t              HIDIntInEp;
  HID_CtlState         ctl_state;
  uint16_t             length;
  uint8_t              ep_addr;
  uint16_t             poll;
  __IO uint16_t        timer;
  USB_HID_cb_t         *cb;
}
USB_HID_machine_t;


/////////////////////////////////////////////////////////////////////////////
// Variables
/////////////////////////////////////////////////////////////////////////////

__ALIGN_BEGIN USB_HID_machine_t  	USB_HID_machine __ALIGN_END ;
__ALIGN_BEGIN USBH_HIDDesc_TypeDef  USB_HID_Desc __ALIGN_END ;


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static void  MIOS32_USB_HID_Mouse_Init (USB_OTG_CORE_HANDLE *pdev , void  *phost);
static void  MIOS32_USB_HID_Mouse_Decode(USB_OTG_CORE_HANDLE *pdev , void  *phost, uint8_t *data);

USB_HID_cb_t HID_MOUSE_cb=
{
	MIOS32_USB_HID_Mouse_Init,
	MIOS32_USB_HID_Mouse_Decode
};

mios32_mouse_data_t Mouse_Data;


static void  MIOS32_USB_HID_Keyboard_Init (USB_OTG_CORE_HANDLE *pdev , void  *phost);
static void  MIOS32_USB_HID_Keyboard_Decode(USB_OTG_CORE_HANDLE *pdev , void  *phost, uint8_t *pbuf);

USB_HID_cb_t HID_KEYBRD_cb=
{
	MIOS32_USB_HID_Keyboard_Init,
	MIOS32_USB_HID_Keyboard_Decode
};

mios32_kbd_state_t Keyboard_State;
static u8 keys_last[KBR_MAX_NBR_PRESSED];
static u8 Keyboard_LastLocks;

static USBH_Status USBH_Set_Idle (USB_OTG_CORE_HANDLE *pdev,
                                  USBH_HOST *phost,
                                  uint8_t duration,
                                  uint8_t reportId);
static USBH_Status USBH_Set_Report (USB_OTG_CORE_HANDLE *pdev,
                                    USBH_HOST *phost,
                                    uint8_t reportType,
                                    uint8_t reportId,
                                    uint8_t reportLen,
                                    uint8_t* reportBuff);


// callbacks
static void (*mouse_callback_func)(mios32_mouse_data_t mouse_data);
static void (*keyboard_callback_func)(mios32_kbd_state_t kbd_state, mios32_kbd_key_t kbd_key);


/**
 * @brief  USR_MOUSE_Init
 *         Init Mouse window
 * @param  None
 * @retval None
 */
void MIOS32_USB_HID_Mouse_Init(USB_OTG_CORE_HANDLE *pdev , void  *phost)
{
  Mouse_Data.buttons= 0;
  Mouse_Data.x      = 0;
  Mouse_Data.y      = 0;
  Mouse_Data.z      = 0;
  Mouse_Data.tilt   = 0;
  
  Mouse_Data.connected =1;
  if(mouse_callback_func != NULL)mouse_callback_func(Mouse_Data);
#ifdef MIOS32_MIDI_USBH_DEBUG
  DEBUG_MSG((void*)USB_HID_MouseStatus);
  DEBUG_MSG("[USBH_HID]\n\n\n\n\n\n\n\n");
#endif
}

/**
 * @brief  USR_MOUSE_ProcessData
 *         Process Mouse data
 * @param  data : Mouse data to be displayed
 * @retval None
 */
void MIOS32_USB_HID_Mouse_Decode(USB_OTG_CORE_HANDLE *pdev , void  *phost, uint8_t *data)
{
  // store current buttons state
  u8 buttons = Mouse_Data.buttons;
  // set new buttons state
	Mouse_Data.buttons = data[0];

	Mouse_Data.x      = (s8)data[1];
	Mouse_Data.y      = (s8)data[2];
  Mouse_Data.z      = (s8)data[3];
  Mouse_Data.tilt   = (s8)data[4];
  
  u8 cb_flag = 0;
  
  // axis
  if ((Mouse_Data.x != 0) || (Mouse_Data.y != 0))
  {
    cb_flag++;
#ifdef MIOS32_MIDI_USBH_DEBUG
    DEBUG_MSG("[USBH_HID]Mouse position, x:%d, y:%d\n", Mouse_Data.x, Mouse_Data.y, Mouse_Data.z, Mouse_Data.tilt) ;
#endif
  }
  // wheel
  if ((Mouse_Data.z != 0) || (Mouse_Data.tilt != 0))
  {
    cb_flag++;
#ifdef MIOS32_MIDI_USBH_DEBUG
    DEBUG_MSG("[USBH_HID]Mouse wheel, z:%d, tilt:%d\n", Mouse_Data.z, Mouse_Data.tilt) ;
#endif
  }
  // buttons
  if(buttons!=Mouse_Data.buttons)cb_flag++;
#ifdef MIOS32_MIDI_USBH_DEBUG
  uint8_t idx;
	for ( idx = 0 ; idx < 3 ; idx ++)
	{

		if(Mouse_Data.buttons & (1 << idx) && (buttons & (1 << idx))==0)
		{
        DEBUG_MSG("[USBH_HID]Mouse button %d pressed \n", idx);
  	}else if((Mouse_Data.buttons & (1 << idx))==0 && buttons & (1 << idx))
		{
        DEBUG_MSG("[USBH_HID]Mouse button %d released \n", idx);
		}
	}
#endif
  /* call user process handle */
  if(cb_flag)if(mouse_callback_func != NULL)mouse_callback_func(Mouse_Data);
  
}

/**
 * @brief  USR_KEYBRD_Init
 *         Init Keyboard window
 * @param  None
 * @retval None
 */
void MIOS32_USB_HID_Keyboard_Init (USB_OTG_CORE_HANDLE *pdev , void  *phost)
{
  int i;
  Keyboard_State.ALL=0;
  Keyboard_State.connected=1;
  // send report
//  USBH_HOST *pphost = phost;
//  u8 locks = 0x07;
//  USBH_Set_Report(pdev, pphost, 0x02, 0x00, 0x01, &locks);
  for (i=0; i<KBR_MAX_NBR_PRESSED; i++) {
    keys_last[i]= 0;
  }
//  locks = (u8)Keyboard_State.locks;
  Keyboard_LastLocks=(u8)Keyboard_State.locks;;
//  USBH_Set_Report(pdev, pphost, 0x02, 0x00, 0x01, &locks);
  /* call user process handle */
  mios32_kbd_key_t key;
  key.code = 0;
  key.value = 0;
  if(keyboard_callback_func != NULL)keyboard_callback_func(Keyboard_State, key);
 
#ifdef MIOS32_MIDI_USBH_DEBUG
  DEBUG_MSG((void*)USB_HID_KeybrdStatus);
  
  DEBUG_MSG("[USBH_HID]Use Keyboard to tape characters: \n\n");
  DEBUG_MSG("[USBH_HID]\n\n\n\n\n\n");
#endif

}


/**
 * @brief  USR_KEYBRD_ProcessData
 *         Process Keyboard data
 * @param  data : Keyboard data to be displayed
 * @retval None
 */
void MIOS32_USB_HID_Keyboard_Decode (USB_OTG_CORE_HANDLE *pdev , void  *phost, uint8_t *pbuf)
{
  int i;
	u8 keys_new[KBR_MAX_NBR_PRESSED];
  
	uint8_t   error;
 
  USBH_Status status = USBH_OK;
  USBH_HOST *pphost = phost;
 
#ifdef MIOS32_MIDI_USBH_DEBUG
    DEBUG_MSG("[USBH_HID]pbuf: %02x %02x %02x %02x %02x %02x %02x %02x", pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5], pbuf[6], pbuf[7]);
#endif

	/* Store new modifiers */
  Keyboard_State.modifiers = pbuf[0];

	/* Check for errors */
  error = 0;
	for (i = 2; i < 2 + KBR_MAX_NBR_PRESSED; i++) {
		if ((pbuf[i] == 0x01) ||
				(pbuf[i] == 0x02) ||
				(pbuf[i] == 0x03)) {
			error = 1;
		}
	}
	if (error == 1) {
		return;
	}
 
  // structure for changes storage
  mios32_kbd_key_t key_changes[6];
  u8 key_changes_count = 0;
  
  // copy the new keys
  memcpy(&keys_new, &pbuf[2], 6);
  
  // look for released keys
  for (i=0; i<6; i++) {
    if(keys_last[i] != 0){
      if(memchr(&keys_new, keys_last[i], 6)==NULL){
        key_changes[key_changes_count].code = keys_last[i];
        key_changes[key_changes_count].value = 0;
        key_changes_count++;
      }
    }else break;
  }
  // look for pushed keys
  for (i=0; i<6; i++) {
    if(keys_new[i] != 0){
      if(memchr(&keys_last, keys_new[i], 6)==NULL){
        key_changes[key_changes_count].code = keys_new[i];
        key_changes[key_changes_count].value = 1;
        key_changes_count++;
      }
    }else break;
  }
#if 0
#ifdef MIOS32_MIDI_USBH_DEBUG
  DEBUG_MSG("[USBH_HID]keys_last: %02x %02x %02x %02x %02x %02x", keys_last[0], keys_last[1], keys_last[2], keys_last[3], keys_last[4], keys_last[5]);
  DEBUG_MSG("[USBH_HID]keys_new : %02x %02x %02x %02x %02x %02x", keys_new[0], keys_new[1], keys_new[2], keys_new[3], keys_new[4], keys_new[5]);
#endif
#endif
  
  for (i=0; i<key_changes_count; i++) {
    switch (key_changes[i].code) {
      case 0x53:
        // caps_lock
        if (key_changes[i].value)Keyboard_State.num_lock = ~Keyboard_State.num_lock;
        key_changes[i].character =  HID_KEYBRD_Key[HID_KEYBRD_Codes[key_changes[i].code]];
        break;
      case 0x39:
        // caps_lock
        if (key_changes[i].value)Keyboard_State.caps_lock = ~Keyboard_State.caps_lock;
        key_changes[i].character =  HID_KEYBRD_Key[HID_KEYBRD_Codes[key_changes[i].code]];
        break;
      case 0x47:
        // scroll_lock
        if (key_changes[i].value)Keyboard_State.scroll_lock = ~Keyboard_State.scroll_lock;
        key_changes[i].character =  HID_KEYBRD_Key[HID_KEYBRD_Codes[key_changes[i].code]];
        break;
      default:
        if (((Keyboard_State.left_shift || Keyboard_State.right_shift)&& (Keyboard_State.caps_lock==0)) || (((Keyboard_State.left_shift==0) && (Keyboard_State.right_shift==0))&& (Keyboard_State.caps_lock==1))) {
          key_changes[i].character =  HID_KEYBRD_ShiftKey[HID_KEYBRD_Codes[key_changes[i].code]];
        } else {
          key_changes[i].character =  HID_KEYBRD_Key[HID_KEYBRD_Codes[key_changes[i].code]];
        }
        break;
    }
    
    /* call user process handle */
    if(keyboard_callback_func != NULL)keyboard_callback_func(Keyboard_State, key_changes[i]);
  }
  
  // send set report
  if(Keyboard_LastLocks != Keyboard_State.locks){
    u8 locks = (u8)Keyboard_State.locks;
    status = USBH_Set_Report(pdev, pphost, 0x02, 0x00, 0x01, &locks);
    if(status==0)Keyboard_LastLocks = Keyboard_State.locks;
#ifdef MIOS32_MIDI_USBH_DEBUG
    DEBUG_MSG("[USBH_HID]Set_Report stat:%d", (u8)(status));
#endif
  }
  
  // store new keys
  memcpy(&keys_last, &keys_new, 6);

}



  

/////////////////////////////////////////////////////////////////////////////
//! Initializes USB HID layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_HID_Init(u32 mode)
{
	USB_HID_machine.start_toggle = 0;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! This function is called by the USB driver on cable connection/disconnection
//! \param[in] device number (0 for OTG_FS, 1 for OTG_HS)
//! \return < 0 on errors
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
static s32 MIOS32_USB_HID_ChangeConnectionState(u8 connected)
{
	  if( connected ) {
		  USB_HID_machine.transfer_possible = 1;
	  } else {
		// device disconnected: disable transfers
		  USB_HID_machine.transfer_possible = 0;
      if(Keyboard_State.connected){
        Keyboard_State.connected = 0;
        /* call user process handle */
        mios32_kbd_key_t key;
        key.code = 0;
        key.value = 0;
        if(keyboard_callback_func != NULL)keyboard_callback_func(Keyboard_State, key);
      }
      if(Mouse_Data.connected){
        Mouse_Data.connected = 0;
        /* call user process handle */
        Mouse_Data.buttons= 0;
        Mouse_Data.x      = 0;
        Mouse_Data.y      = 0;
        Mouse_Data.z      = 0;
        Mouse_Data.tilt   = 0;
        if(mouse_callback_func != NULL)mouse_callback_func(Mouse_Data);
      }
	  }
	return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the connection status of the USB COM interface
//! \param[in] device number (0 for OTG_FS, 1 for OTG_HS)
//! \return 1: interface available
//! \return 0: interface not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_HID_CheckAvailable(void)
{
  return USB_HID_machine.transfer_possible ? 1 : 0;
}



//////////////////////////////////////////////////////////////////////////
//! Installs an optional Mouse callback
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_HID_MouseCallback_Init(void (*mouse_callback)(mios32_mouse_data_t mouse_data))
{
  mouse_callback_func = mouse_callback;
  return 0; // no error
}


//////////////////////////////////////////////////////////////////////////
//! Installs an optional Keyboard callback
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_HID_KeyboardCallback_Init(void (*keyboard_callback)(mios32_kbd_state_t kbd_state, mios32_kbd_key_t kbd_key))
{
  keyboard_callback_func = keyboard_callback;
  return 0; // no error
}


/**
* @brief  USBH_Get_HID_ReportDescriptor
*         Issue report Descriptor command to the device. Once the response
*         received, parse the report descriptor and update the status.
* @param  pdev   : Selected device
* @param  Length : HID Report Descriptor Length
* @retval USBH_Status : Response for USB HID Get Report Descriptor Request
*/
static USBH_Status USBH_Get_HID_ReportDescriptor (USB_OTG_CORE_HANDLE *pdev,
                                                  USBH_HOST *phost,
                                                  uint16_t length)
{

  USBH_Status status;

  status = USBH_GetDescriptor(pdev,
                              phost,
                              USB_REQ_RECIPIENT_INTERFACE
                                | USB_REQ_TYPE_STANDARD,
                                USB_DESC_HID_REPORT,
                                pdev->host.Rx_Buffer,
                                length);

  /* HID report descriptor is available in pdev->host.Rx_Buffer.
  In case of USB Boot Mode devices for In report handling ,
  HID report descriptor parsing is not required.
  In case, for supporting Non-Boot Protocol devices and output reports,
  user may parse the report descriptor*/


  return status;
}

/**
* @brief  USBH_Get_HID_Descriptor
*         Issue HID Descriptor command to the device. Once the response
*         received, parse the report descriptor and update the status.
* @param  pdev   : Selected device
* @param  Length : HID Descriptor Length
* @retval USBH_Status : Response for USB HID Get Report Descriptor Request
*/
static USBH_Status USBH_Get_HID_Descriptor (USB_OTG_CORE_HANDLE *pdev,
                                            USBH_HOST *phost,
                                            uint16_t length)
{

  USBH_Status status;

  status = USBH_GetDescriptor(pdev,
                              phost,
                              USB_REQ_RECIPIENT_INTERFACE
                                | USB_REQ_TYPE_STANDARD,
                                USB_DESC_HID,
                                pdev->host.Rx_Buffer,
                                length);

  return status;
}

/**
* @brief  USBH_Set_Idle
*         Set Idle State.
* @param  pdev: Selected device
* @param  duration: Duration for HID Idle request
* @param  reportID : Targetted report ID for Set Idle request
* @retval USBH_Status : Response for USB Set Idle request
*/
USBH_Status USBH_Set_Idle (USB_OTG_CORE_HANDLE *pdev,
                                  USBH_HOST *phost,
                                  uint8_t duration,
                                  uint8_t reportId)
{

  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;


  phost->Control.setup.b.bRequest = USB_HID_SET_IDLE;
  phost->Control.setup.b.wValue.w = (duration << 8 ) | reportId;

  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = 0;

  return USBH_CtlReq(pdev, phost, 0 , 0 );
}


/**
* @brief  USBH_Set_Report
*         Issues Set Report
* @param  pdev: Selected device
* @param  reportType  : Report type to be sent
* @param  reportID    : Targetted report ID for Set Report request
* @param  reportLen   : Length of data report to be send
* @param  reportBuff  : Report Buffer
* @retval USBH_Status : Response for USB Set Idle request
*/
USBH_Status USBH_Set_Report (USB_OTG_CORE_HANDLE *pdev,
                                 USBH_HOST *phost,
                                    uint8_t reportType,
                                    uint8_t reportId,
                                    uint8_t reportLen,
                                    uint8_t* reportBuff)
{

  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;


  phost->Control.setup.b.bRequest = USB_HID_SET_REPORT;
  phost->Control.setup.b.wValue.w = (reportType << 8 ) | reportId;

  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = reportLen;

  return USBH_CtlReq(pdev, phost, reportBuff , reportLen );
}


/**
* @brief  USBH_Set_Protocol
*         Set protocol State.
* @param  pdev: Selected device
* @param  protocol : Set Protocol for HID : boot/report protocol
* @retval USBH_Status : Response for USB Set Protocol request
*/
static USBH_Status USBH_Set_Protocol(USB_OTG_CORE_HANDLE *pdev,
                                     USBH_HOST *phost,
                                     uint8_t protocol)
{


  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;


  phost->Control.setup.b.bRequest = USB_HID_SET_PROTOCOL;

  if(protocol != 0)
  {
    /* Boot Protocol */
    phost->Control.setup.b.wValue.w = 0;
  }
  else
  {
    /*Report Protocol*/
    phost->Control.setup.b.wValue.w = 1;
  }

  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = 0;

  return USBH_CtlReq(pdev, phost, 0 , 0 );

}

/**
* @brief  USBH_ParseHIDDesc
*         This function Parse the HID descriptor
* @param  buf: Buffer where the source descriptor is available
* @retval None
*/
static void  USBH_ParseHIDDesc (USBH_HIDDesc_TypeDef *desc, uint8_t *buf)
{

  USB_HID_Desc.bLength                  = *(uint8_t  *) (buf + 0);
  USB_HID_Desc.bDescriptorType          = *(uint8_t  *) (buf + 1);
  USB_HID_Desc.bcdHID                   =  LE16  (buf + 2);
  USB_HID_Desc.bCountryCode             = *(uint8_t  *) (buf + 4);
  USB_HID_Desc.bNumDescriptors          = *(uint8_t  *) (buf + 5);
  USB_HID_Desc.bReportDescriptorType    = *(uint8_t  *) (buf + 6);
  USB_HID_Desc.wItemLength              =  LE16  (buf + 7);

}




/** @defgroup USBH_HID_CORE_Private_Functions
* @{
*/

/**
* @brief  USBH_HID_InterfaceInit
*         The function init the HID class.
* @param  pdev: Selected device
* @param  hdev: Selected device property
* @retval  USBH_Status :Response for USB HID driver intialization
*/
static USBH_Status USBH_InterfaceInit ( USB_OTG_CORE_HANDLE *pdev, void *phost)
{
	uint8_t maxEP;
	USBH_HOST *pphost = phost;

	uint8_t num =0;
	

	MIOS32_USB_HID_ChangeConnectionState(0);
	int i;
	USB_HID_machine.state = HID_ERROR;

	for(i=0; i<pphost->device_prop.Cfg_Desc.bNumInterfaces && i < USBH_MAX_NUM_INTERFACES; ++i) {

		if(pphost->device_prop.Itf_Desc[i].bInterfaceSubClass  == USB_HID_BOOT_CODE)
		{
			/*Decode Bootclass Protocl: Mouse or Keyboard*/
			if(pphost->device_prop.Itf_Desc[i].bInterfaceProtocol == USB_HID_KEYBRD_BOOT_CODE)
			{
				USB_HID_machine.cb = &HID_KEYBRD_cb;
			}
			else if(pphost->device_prop.Itf_Desc[i].bInterfaceProtocol  == USB_HID_MOUSE_BOOT_CODE)
			{
				USB_HID_machine.cb = &HID_MOUSE_cb;
			}

			USB_HID_machine.state     = HID_IDLE;
			USB_HID_machine.ctl_state = HID_REQ_IDLE;
			USB_HID_machine.ep_addr   = pphost->device_prop.Ep_Desc[i][0].bEndpointAddress;
			USB_HID_machine.length    = pphost->device_prop.Ep_Desc[i][0].wMaxPacketSize;
			USB_HID_machine.poll      = pphost->device_prop.Ep_Desc[i][0].bInterval ;

			if (USB_HID_machine.poll  < HID_MIN_POLL)
			{
				USB_HID_machine.poll = HID_MIN_POLL;
			}


			/* Check fo available number of endpoints */
			/* Find the number of EPs in the Interface Descriptor */
			/* Choose the lower number in order not to overrun the buffer allocated */
			maxEP = ( (pphost->device_prop.Itf_Desc[i].bNumEndpoints <= USBH_MAX_NUM_ENDPOINTS) ?
					pphost->device_prop.Itf_Desc[i].bNumEndpoints :
					USBH_MAX_NUM_ENDPOINTS);


			/* Decode endpoint IN and OUT address from interface descriptor */
			for (num=0; num < maxEP; num++)
			{
				if(pphost->device_prop.Ep_Desc[i][num].bEndpointAddress & 0x80)
				{
					USB_HID_machine.HIDIntInEp = (pphost->device_prop.Ep_Desc[i][num].bEndpointAddress);
					USB_HID_machine.hc_num_in = USBH_Alloc_Channel(pdev, USB_HID_machine.HIDIntInEp);

					/* Open channel for IN endpoint */
					USBH_Open_Channel  (pdev,
							USB_HID_machine.hc_num_in,
							pphost->device_prop.address,
							pphost->device_prop.speed,
							EP_TYPE_INTR,
							USB_HID_machine.length);
				}
				else
				{
					USB_HID_machine.HIDIntOutEp = (pphost->device_prop.Ep_Desc[i][num].bEndpointAddress);
					USB_HID_machine.hc_num_out = USBH_Alloc_Channel(pdev, USB_HID_machine.HIDIntOutEp);

					/* Open channel for OUT endpoint */
					USBH_Open_Channel  (pdev,
							USB_HID_machine.hc_num_out,
							pphost->device_prop.address,
							pphost->device_prop.speed,
							EP_TYPE_INTR,
							USB_HID_machine.length);
				}

			}

		    MIOS32_USB_HID_ChangeConnectionState(1);
		    USB_HID_machine.start_toggle = 0;
		    break;
		}
	}
	if( MIOS32_USB_HID_CheckAvailable() == 0 ) {
		pphost->usr_cb->DeviceNotSupported();
		return USBH_NOT_SUPPORTED;
	}
	return USBH_OK;

}



/**
* @brief  USBH_HID_InterfaceDeInit
*         The function DeInit the Host Channels used for the HID class.
* @param  pdev: Selected device
* @param  hdev: Selected device property
* @retval None
*/
void USBH_InterfaceDeInit ( USB_OTG_CORE_HANDLE *pdev,
		void *phost)
{
	

	if(USB_HID_machine.hc_num_in != 0x00)
	{
		USB_OTG_HC_Halt(pdev, USB_HID_machine.hc_num_in);
		USBH_Free_Channel  (pdev, USB_HID_machine.hc_num_in);
		USB_HID_machine.hc_num_in = 0;     /* Reset the Channel as Free */
	}

	if(USB_HID_machine.hc_num_out != 0x00)
	{
		USB_OTG_HC_Halt(pdev, USB_HID_machine.hc_num_out);
		USBH_Free_Channel  (pdev, USB_HID_machine.hc_num_out);
		USB_HID_machine.hc_num_out = 0;     /* Reset the Channel as Free */
	}
	MIOS32_USB_HID_ChangeConnectionState(0);
	USB_HID_machine.start_toggle = 0;
}

/**
* @brief  USBH_HID_ClassRequest
*         The function is responsible for handling HID Class requests
*         for HID class.
* @param  pdev: Selected device
* @param  hdev: Selected device property
* @retval  USBH_Status :Response for USB Set Protocol request
*/
static USBH_Status USBH_ClassRequest(USB_OTG_CORE_HANDLE *pdev ,
		void *phost)
{
	USBH_HOST *pphost = phost;
	USBH_Status status         = USBH_BUSY;
	USBH_Status classReqStatus = USBH_BUSY;

	/* Switch HID state machine */
	switch (USB_HID_machine.ctl_state)
	{
	case HID_IDLE:
	case HID_REQ_GET_HID_DESC:

		/* Get HID Desc */
		if (USBH_Get_HID_Descriptor (pdev, pphost, USB_HID_DESC_SIZE)== USBH_OK)
		{

			USBH_ParseHIDDesc(&USB_HID_Desc, pdev->host.Rx_Buffer);
			USB_HID_machine.ctl_state = HID_REQ_GET_REPORT_DESC;
		}

		break;
	case HID_REQ_GET_REPORT_DESC:


		/* Get Report Desc */
		if (USBH_Get_HID_ReportDescriptor(pdev , pphost, USB_HID_Desc.wItemLength) == USBH_OK)
		{
			USB_HID_machine.ctl_state = HID_REQ_SET_IDLE;
		}

		break;

	case HID_REQ_SET_IDLE:

		classReqStatus = USBH_Set_Idle (pdev, pphost, 0, 0);

		/* set Idle */
		if (classReqStatus == USBH_OK)
		{
			USB_HID_machine.ctl_state = HID_REQ_SET_PROTOCOL;
		}
		else if(classReqStatus == USBH_NOT_SUPPORTED)
		{
			USB_HID_machine.ctl_state = HID_REQ_SET_PROTOCOL;
		}
		break;

	case HID_REQ_SET_PROTOCOL:
		/* set protocol */
		if (USBH_Set_Protocol (pdev ,pphost, 0) == USBH_OK)
		{
			USB_HID_machine.ctl_state = HID_REQ_IDLE;

			/* all requests performed*/
			status = USBH_OK;
		}
		break;

	default:
		break;
	}

	return status;
}


/**
* @brief  USBH_HID_Handle
*         The function is for managing state machine for HID data transfers
* @param  pdev: Selected device
* @param  hdev: Selected device property
* @retval USBH_Status
*/
static USBH_Status USBH_Handle(USB_OTG_CORE_HANDLE *pdev , void   *phost)
{
	USBH_HOST *pphost = phost;
	USBH_Status status = USBH_OK;
	
  switch (USB_HID_machine.state)
  {

  case HID_IDLE:
    USB_HID_machine.cb->Init(pdev, phost);
    USB_HID_machine.state = HID_SYNC;

  case HID_SYNC:

    /* Sync with start of Even Frame */
    if(USB_OTG_IsEvenFrame(pdev) == TRUE)
    {
      USB_HID_machine.state = HID_GET_DATA;
    }
    break;

  case HID_GET_DATA:

    USBH_InterruptReceiveData(pdev,
        USB_HID_machine.buff,
        USB_HID_machine.length,
        USB_HID_machine.hc_num_in);
    USB_HID_machine.start_toggle = 1;

    USB_HID_machine.state = HID_POLL;
    USB_HID_machine.timer = HCD_GetCurrentFrame(pdev);
    break;

  case HID_POLL:
    if(( HCD_GetCurrentFrame(pdev) - USB_HID_machine.timer) >= USB_HID_machine.poll)
    {
      USB_HID_machine.state = HID_GET_DATA;
    }
    else if(HCD_GetURB_State(pdev , USB_HID_machine.hc_num_in) == URB_DONE)
    {
      if(USB_HID_machine.start_toggle == 1) /* handle data once */
      {
        USB_HID_machine.start_toggle = 0;
        USB_HID_machine.cb->Decode(pdev, phost, USB_HID_machine.buff);
      }
    }
    else if(HCD_GetURB_State(pdev, USB_HID_machine.hc_num_in) == URB_STALL) /* IN Endpoint Stalled */
    {

      /* Issue Clear Feature on interrupt IN endpoint */
      if( (USBH_ClrFeature(pdev,
          pphost,
          USB_HID_machine.ep_addr,
          USB_HID_machine.hc_num_in)) == USBH_OK)
      {
        /* Change state to issue next IN token */
        USB_HID_machine.state = HID_GET_DATA;

      }

    }
    break;

  default:
    break;
  }
  return status;
}

const USBH_Class_cb_TypeDef MIOS32_HID_USBH_Callbacks = {
  USBH_InterfaceInit,
  USBH_InterfaceDeInit,
  USBH_ClassRequest,
  USBH_Handle
};

//! \}

#endif /* !defined(MIOS32_DONT_USE_USB_HOST) || !defined(MIOS32_DONT_USE_USB_HID) */
