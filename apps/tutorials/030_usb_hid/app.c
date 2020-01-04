// $Id$
/*
 * MIOS32 Application Template
 *
 * ==========================================================================
 *
 *  Copyright (C) <year> <your name> (<your email address>)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "app.h"

/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////
static void APP_MOUSE_NotifyChange(mios32_mouse_data_t mouse_data);
static void APP_KBD_NotifyChange(mios32_kbd_state_t kbd_state, mios32_kbd_key_t kbd_key);

/////////////////////////////////////////////////////////////////////////////
// local variables
/////////////////////////////////////////////////////////////////////////////
// Mouse
static u8 app_mouse_connected = 0;
static u8 app_cc7_value = 0;   // mouse left button
static u8 app_cc8_value = 0;   // mouse right button
static u8 app_cc9_value = 0;   // mouse x axis
static u8 app_cc10_value = 0;  // mouse y axis
static u8 app_cc11_value = 0;  // mouse wheel(z)
// Keyboard
static u8 app_kbd_connected = 0;
static u8 app_last_octave = 4;

/////////////////////////////////////////////////////////////////////////////
// This hook is called after startup to initialize the application
/////////////////////////////////////////////////////////////////////////////
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);
  
  // Debug Output via UART0
#ifdef APP_DEBUG_VERBOSE
  MIOS32_MIDI_DebugPortSet(UART0);
#endif

  // install HID Mouse & Keyboard callback function
  MIOS32_USB_HID_MouseCallback_Init(APP_MOUSE_NotifyChange);
  MIOS32_USB_HID_KeyboardCallback_Init(APP_KBD_NotifyChange);
}


/////////////////////////////////////////////////////////////////////////////
// This task is running endless in background
/////////////////////////////////////////////////////////////////////////////
void APP_Background(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the main task which also handles DIN, ENC
// and AIN events. You could add more jobs here, but they shouldn't consume
// more than 300 uS to ensure the responsiveness of buttons, encoders, pots.
// Alternatively you could create a dedicated task for application specific
// jobs as explained in $MIOS32_PATH/apps/tutorials/006_rtos_tasks
/////////////////////////////////////////////////////////////////////////////
void APP_Tick(void)
{
  // PWM modulate the status LED (this is a sign of life)
  u32 timestamp = MIOS32_TIMESTAMP_Get();
  MIOS32_BOARD_LED_Set(1, (timestamp % 20) <= ((timestamp / 100) % 10));
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called each mS from the MIDI task which checks for incoming
// MIDI events. You could add more MIDI related jobs here, but they shouldn't
// consume more than 300 uS to ensure the responsiveness of incoming MIDI.
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_Tick(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a MIDI package has been received
/////////////////////////////////////////////////////////////////////////////
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // forward midi package to UARTs for testing USB MIDI
  MIOS32_MIDI_SendPackage(UART0, midi_package);
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called before the shift register chain is scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServicePrepare(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called after the shift register chain has been scanned
/////////////////////////////////////////////////////////////////////////////
void APP_SRIO_ServiceFinish(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a button has been toggled
// pin_value is 1 when button released, and 0 when button pressed
/////////////////////////////////////////////////////////////////////////////
void APP_DIN_NotifyToggle(u32 pin, u32 pin_value)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when an encoder has been moved
// incrementer is positive when encoder has been turned clockwise, else
// it is negative
/////////////////////////////////////////////////////////////////////////////
void APP_ENC_NotifyChange(u32 encoder, s32 incrementer)
{
}


/////////////////////////////////////////////////////////////////////////////
// This hook is called when a pot has been moved
/////////////////////////////////////////////////////////////////////////////
void APP_AIN_NotifyChange(u32 pin, u32 pin_value)
{
}

/////////////////////////////////////////////////////////////////////////////
// This hook is called on HID Keyboard change
/////////////////////////////////////////////////////////////////////////////
void APP_MOUSE_NotifyChange(mios32_mouse_data_t mouse_data)
{
  // Is Mouse connected?
  if(app_mouse_connected!=mouse_data.connected){
    app_mouse_connected=mouse_data.connected;
#ifdef APP_DEBUG_VERBOSE
    DEBUG_MSG("[APP_MOUSE_NotifyChange]HID Mouse %s.", mouse_data.connected?"connected":"disconnected");
#endif
  }
  /* uses buttons, axis and wheel to send some CCs */
  // left button to CC#7
  u8 val = app_cc7_value;
  app_cc7_value = mouse_data.left? 127:0;
  if(val!=app_cc7_value){
    // forward CC to UART0
    MIOS32_MIDI_SendCC(UART0, 0, 7, app_cc7_value);
#ifdef APP_DEBUG_VERBOSE
    DEBUG_MSG("[APP_MOUSE_NotifyChange]left button %s.", app_cc7_value?"pressed":"released");
#endif
  }
  // right button to CC#8
  val = app_cc8_value;
  app_cc8_value = mouse_data.right? 127:0;
  if(val!=app_cc8_value){
    // forward CC to UART0
    MIOS32_MIDI_SendCC(UART0, 0, 8, app_cc8_value);
#ifdef APP_DEBUG_VERBOSE
    DEBUG_MSG("[APP_MOUSE_NotifyChange]right button %s.", app_cc8_value?"pressed":"released");
#endif
  }
  // x axis to CC#9
  if(mouse_data.x!=0){
    if((app_cc9_value + mouse_data.x)<0)app_cc9_value=0; else app_cc9_value += mouse_data.x;
    if(app_cc9_value>127)app_cc9_value=127;
    // forward CC to UART0
    MIOS32_MIDI_SendCC(UART0, 0, 9, app_cc9_value);
#ifdef APP_DEBUG_VERBOSE
    DEBUG_MSG("[APP_MOUSE_NotifyChange]x axis = %d.", mouse_data.x);
#endif
  }
  // y axis to CC#10
  if(mouse_data.y!=0){
    if((app_cc10_value + mouse_data.y)<0)app_cc10_value=0; else app_cc10_value += mouse_data.y;
    if(app_cc10_value>127)app_cc10_value=127;
    // forward CC to UART0
    MIOS32_MIDI_SendCC(UART0, 0, 10, app_cc10_value);
#ifdef APP_DEBUG_VERBOSE
    DEBUG_MSG("[APP_MOUSE_NotifyChange]y axis = %d.", mouse_data.y);
#endif
  }
  // wheel(z axis) to CC#11
  if(mouse_data.z!=0){
    if((app_cc11_value + mouse_data.z)<0)app_cc11_value=0; else app_cc11_value += mouse_data.z;
    if(app_cc11_value>127)app_cc11_value=127;
    // forward CC to UART0
    MIOS32_MIDI_SendCC(UART0, 0, 10, app_cc11_value);
#ifdef APP_DEBUG_VERBOSE
    DEBUG_MSG("[APP_MOUSE_NotifyChange]wheel(y axis) = %d.", mouse_data.z);
#endif
  }
}

/////////////////////////////////////////////////////////////////////////////
// This hook is called on HID Keyboard change
/////////////////////////////////////////////////////////////////////////////
void APP_KBD_NotifyChange(mios32_kbd_state_t kbd_state, mios32_kbd_key_t kbd_key)
{
  // Is Keyboard connected?
  if(app_kbd_connected!=kbd_state.connected){
    app_kbd_connected=kbd_state.connected;
#ifdef APP_DEBUG_VERBOSE
    DEBUG_MSG("[APP_KBD_NotifyChange]HID Keyboard %s.", kbd_state.connected?"connected":"disconnected");
#endif
  }
  
  /* uses some of the key to play some notes */
  // Octave/Semi input
  switch (kbd_key.code) {
    case 0x1e ... 0x27:    // 1 ... 0 for Octave -2 to 7, only with capd_lock On
      if(kbd_key.value && kbd_state.caps_lock)app_last_octave = kbd_key.code-0x1e;
      break;
    case 0x3a ... 0x45:    // F1 ... F12 for Semitone value, only with capd_lock On
      if(kbd_state.caps_lock){
        u8 note = app_last_octave*12 + kbd_key.code-0x3a;
        if(note>127)note =127;
        if(kbd_key.value){
          // forward noteOn to UARTs
          MIOS32_MIDI_SendNoteOn(UART0, 0, note, 100);
        }else{
          // forward noteOff to UARTs
          MIOS32_MIDI_SendNoteOff(UART0, 0, note, 0);
        }
      }
      break;
    default:
      break;
  }
#ifdef APP_DEBUG_VERBOSE
  DEBUG_MSG("[APP_KBD_NotifyChange]Locks:%d, Modifiers:%d, Key:0x%02x[%c]%s\n", kbd_state.locks, kbd_state.modifiers, kbd_key.code, kbd_key.character, kbd_key.value? "pushed":"released");
#endif


}
