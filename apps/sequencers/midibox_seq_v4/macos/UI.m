// $Id$
//
//  UI.m
//  midibox_seq_v4
//
//  Created by Thorsten Klose on 28.09.08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "UI.h"
#include <mios32.h>

@implementation UI

// struct for MIDI parsing
typedef struct {
  mios32_midi_package_t package;
  u8 running_status;
  u8 expected_bytes;
  u8 wait_bytes;
  u8 sysex_ctr;
} midi_rec_t;

// local variables to bridge objects to C functions
#define NUM_LCD 2
CLCDView *LCD[NUM_LCD];

#define NUM_LEDS 17
NSColorWell *LED[NUM_LEDS];
u8 ledState[NUM_LEDS]; // for dual-colour option

NSObject *_self;

NSButton *_buttonTrack1;
NSButton *_buttonTrack2;
NSButton *_buttonTrack3;
NSButton *_buttonTrack4;

NSButton *_buttonGroup1;
NSButton *_buttonGroup2;
NSButton *_buttonGroup3;
NSButton *_buttonGroup4;

NSButton *_buttonPLayerA;
NSButton *_buttonPLayerB;
NSButton *_buttonPLayerC;

NSButton *_buttonTLayerA;
NSButton *_buttonTLayerB;
NSButton *_buttonTLayerC;

NSButton *_buttonEdit;
NSButton *_buttonMute;
NSButton *_buttonPattern;
NSButton *_buttonSong;

NSButton *_buttonSolo;
NSButton *_buttonFast;
NSButton *_buttonAll;

NSButton *_buttonStepView;

NSButton *_buttonPlay;
NSButton *_buttonStop;
NSButton *_buttonPause;

PYMIDIVirtualDestination *virtualMIDI_IN[NUM_MIDI_IN];
PYMIDIVirtualSource *virtualMIDI_OUT[NUM_MIDI_OUT];

// handler data structure
static midi_rec_t midi_rec[NUM_MIDI_IN];

// LCD selection
u8 selectedLCD;

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

// this global array is read from MIOS32_IIC_MIDI and MIOS32_UART_MIDI to
// determine the number of MIDI bytes which are part of a package
const u8 mios32_midi_pcktype_num_bytes[16] = {
  0, // 0: invalid/reserved event
  0, // 1: invalid/reserved event
  2, // 2: two-byte system common messages like MTC, Song Select, etc.
  3, // 3: three-byte system common messages like SPP, etc.
  3, // 4: SysEx starts or continues
  1, // 5: Single-byte system common message or sysex sends with following single byte
  2, // 6: SysEx sends with following two bytes
  3, // 7: SysEx sends with following three bytes
  3, // 8: Note Off
  3, // 9: Note On
  3, // a: Poly-Key Press
  3, // b: Control Change
  2, // c: Program Change
  2, // d: Channel Pressure
  3, // e: PitchBend Change
  1  // f: single byte
};

// Number if expected bytes for a common MIDI event - 1
const u8 mios32_midi_expected_bytes_common[8] = {
  2, // Note On
  2, // Note Off
  2, // Poly Preasure
  2, // Controller
  1, // Program Change
  1, // Channel Preasure
  2, // Pitch Bender
  0, // System Message - must be zero, so that mios32_midi_expected_bytes_system[] will be used
};

// // Number if expected bytes for a system MIDI event - 1
const u8 mios32_midi_expected_bytes_system[16] = {
  1, // SysEx Begin (endless until SysEx End F7)
  1, // MTC Data frame
  2, // Song Position
  1, // Song Select
  0, // Reserved
  0, // Reserved
  0, // Request Tuning Calibration
  0, // SysEx End

  // Note: just only for documentation, Realtime Messages don't change the running status
  0, // MIDI Clock
  0, // MIDI Tick
  0, // MIDI Start
  0, // MIDI Continue
  0, // MIDI Stop
  0, // Reserved
  0, // Active Sense
  0, // Reset
};


//////////////////////////////////////////////////////////////////////////////
// LCD access functions
//////////////////////////////////////////////////////////////////////////////
s32 MIOS32_LCD_Init(u32 mode)
{
	// select first device
	MIOS32_LCD_DeviceSet(0);

	// clear screen
	MIOS32_LCD_Clear();

	// set cursor to initial position
	MIOS32_LCD_CursorSet(0, 0);
	
	return 0; // no error
}

s32 MIOS32_LCD_DeviceSet(u8 device)
{
	if( device >= NUM_LCD )
		return -1; // unsupported LCD

	selectedLCD = device;
	
	return 0; // no error
}

u8 MIOS32_LCD_DeviceGet(void)
{
	return selectedLCD;
}

s32 MIOS32_LCD_CursorSet(u16 column, u16 line)
{
	[LCD[selectedLCD] setLCDCursorX:column];
	[LCD[selectedLCD] setLCDCursorY:line];
	
	return 0; // no error
}

s32 MIOS32_LCD_Clear(void)
{
	[LCD[selectedLCD] LCDClear];
	
	return 0; // no error
}

s32 MIOS32_LCD_PrintChar(char c)
{
	[LCD[selectedLCD] LCDPrintChar:c];

	return 0; // no error
}

s32 MIOS32_LCD_PrintString(char *str)
{
	[LCD[selectedLCD] LCDPrintString:str];

	return 0; // no error
}

s32 MIOS32_LCD_PrintFormattedString(char *format, ...)
{
	u8 buffer[64];
	va_list args;

	va_start(args, format);
	vsprintf((char *)buffer, format, args);
	[LCD[selectedLCD] LCDPrintString:buffer];

	return 0; // no error
}

s32 MIOS32_LCD_SpecialCharInit(u8 num, u8 table[8])
{
	[LCD[selectedLCD] LCDSpecialCharInit:num:table];
	
	return 0; // no error
}

s32 MIOS32_LCD_SpecialCharsInit(u8 table[64])
{
	[LCD[selectedLCD] LCDSpecialCharsInit:table];
	
	return 0; // no error
}


//////////////////////////////////////////////////////////////////////////////
// COM functions
//////////////////////////////////////////////////////////////////////////////
s32 MIOS32_COM_SendChar(u8 port, char c)
{
	// empty stub - no COM terminal implemented yet
	return 0; // no error
}


//////////////////////////////////////////////////////////////////////////////
// MIDI functions
//////////////////////////////////////////////////////////////////////////////
s32 MIOS32_MIDI_CheckAvailable(mios32_midi_port_t port)
{
  // if default port: select mapped port
  if( !(port & 0xf0) ) {
    port = MIOS32_MIDI_DEFAULT_PORT;
  }

  if( port >= 0x10 && port <= (0x10 + NUM_MIDI_OUT) )
	return 1; // port available

  return 0; // port not available
}

s32 MIOS32_MIDI_SendPackage(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // if default port: select mapped port
  if( !(port & 0xf0) ) {
    port = MIOS32_MIDI_DEFAULT_PORT;
  }

  // insert subport number into package
  package.cable = port & 0xf;

  if( port >= 0x10 && port <= (0x10 + NUM_MIDI_OUT) ) {
	int outNum = port & 0xf;

  	MIDIPacketList packetList;
	MIDIPacket *packet = MIDIPacketListInit(&packetList);

	u8 len = mios32_midi_pcktype_num_bytes[package.cin];
	if( len ) {
		u8 buffer[3] = {package.evnt0, package.evnt1, package.evnt2};
	
		packet = MIDIPacketListAdd(&packetList, sizeof(packetList), packet,
			     0, // timestamp
				 len, buffer);
		[virtualMIDI_OUT[outNum] addSender:_self];
		[virtualMIDI_OUT[outNum] processMIDIPacketList:&packetList sender:_self];
		[virtualMIDI_OUT[outNum] removeSender:_self];
	}

	return 0; // packet sent successfully
  }

  return -1; // port not available
}

s32 MIOS32_MIDI_SendEvent(mios32_midi_port_t port, u8 evnt0, u8 evnt1, u8 evnt2)
{
  mios32_midi_package_t package;

  // MEMO: don't optimize this function by calling MIOS32_MIDI_SendSpecialEvent
  // from here, because the 4 * u8 parameter list of this function leads
  // to best compile results (4*u8 combined to a single u32)

  package.type  = evnt0 >> 4;
  package.evnt0 = evnt0;
  package.evnt1 = evnt1;
  package.evnt2 = evnt2;
  return MIOS32_MIDI_SendPackage(port, package);
}

s32 MIOS32_MIDI_SendNoteOff(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 vel)
{ return MIOS32_MIDI_SendEvent(port, 0x80 | chn, note, vel); }

s32 MIOS32_MIDI_SendNoteOn(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 vel)
{ return MIOS32_MIDI_SendEvent(port, 0x90 | chn, note, vel); }

s32 MIOS32_MIDI_SendPolyPressure(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 note, u8 val)
{ return MIOS32_MIDI_SendEvent(port, 0xa0 | chn, note, val); }

s32 MIOS32_MIDI_SendCC(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 cc, u8 val)
{ return MIOS32_MIDI_SendEvent(port, 0xb0 | chn, cc,   val); }

s32 MIOS32_MIDI_SendProgramChange(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 prg)
{ return MIOS32_MIDI_SendEvent(port, 0xc0 | chn, prg,  0x00); }

s32 MIOS32_MIDI_SendAftertouch(mios32_midi_port_t port, mios32_midi_chn_t chn, u8 val)
{ return MIOS32_MIDI_SendEvent(port, 0xd0 | chn, val,  0x00); }

s32 MIOS32_MIDI_SendPitchBend(mios32_midi_port_t port, mios32_midi_chn_t chn, u16 val)
{ return MIOS32_MIDI_SendEvent(port, 0xe0 | chn, val & 0x7f, val >> 7); }


s32 MIOS32_MIDI_SendSpecialEvent(mios32_midi_port_t port, u8 type, u8 evnt0, u8 evnt1, u8 evnt2)
{
  mios32_midi_package_t package;

  package.type  = type;
  package.evnt0 = evnt0;
  package.evnt1 = evnt1;
  package.evnt2 = evnt2;
  return MIOS32_MIDI_SendPackage(port, package);
}

s32 MIOS32_MIDI_SendMTC(mios32_midi_port_t port, u8 val)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x2, 0xf1, val, 0x00); }

s32 MIOS32_MIDI_SendSongPosition(mios32_midi_port_t port, u16 val)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x3, 0xf2, val & 0x7f, val >> 7); }

s32 MIOS32_MIDI_SendSongSelect(mios32_midi_port_t port, u8 val)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x2, 0xf3, val, 0x00); }

s32 MIOS32_MIDI_SendTuneRequest(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xf6, 0x00, 0x00); }

s32 MIOS32_MIDI_SendClock(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xf8, 0x00, 0x00); }

s32 MIOS32_MIDI_SendTick(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xf9, 0x00, 0x00); }

s32 MIOS32_MIDI_SendStart(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfa, 0x00, 0x00); }

s32 MIOS32_MIDI_SendStop(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfb, 0x00, 0x00); }

s32 MIOS32_MIDI_SendContinue(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfc, 0x00, 0x00); }

s32 MIOS32_MIDI_SendActiveSense(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xfe, 0x00, 0x00); }

s32 MIOS32_MIDI_SendReset(mios32_midi_port_t port)
{ return MIOS32_MIDI_SendSpecialEvent(port, 0x5, 0xff, 0x00, 0x00); }


s32 MIOS32_MIDI_SendSysEx(mios32_midi_port_t port, u8 *stream, u32 count)
{
  s32 res;
  u32 offset;
  mios32_midi_package_t package;

  // MEMO: have a look into the project.lss file - gcc optimizes this code pretty well :)

  for(offset=0; offset<count;) {
    // package type depends on number of remaining bytes
    switch( count-offset ) {
      case 1: 
	package.type = 0x5; // SysEx ends with following single byte. 
	package.evnt0 = stream[offset++];
	package.evnt1 = 0x00;
	package.evnt2 = 0x00;
	break;
      case 2:
	package.type = 0x6; // SysEx ends with following two bytes.
	package.evnt0 = stream[offset++];
	package.evnt1 = stream[offset++];
	package.evnt2 = 0x00;
	break;
      case 3:
	package.type = 0x7; // SysEx ends with following three bytes. 
	package.evnt0 = stream[offset++];
	package.evnt1 = stream[offset++];
	package.evnt2 = stream[offset++];
	break;
      default:
	package.type = 0x4; // SysEx starts or continues
	package.evnt0 = stream[offset++];
	package.evnt1 = stream[offset++];
	package.evnt2 = stream[offset++];
    }

    res=MIOS32_MIDI_SendPackage(port, package);

    // expection? (e.g., port not available)
    if( res < 0 )
      return res;
  }

  return 0;
}


//////////////////////////////////////////////////////////////////////////////
// LED access functions
//////////////////////////////////////////////////////////////////////////////
s32 MIOS32_DOUT_PinSet(u32 pin, u32 value)
{
	int gp_led = -1;
	
	// GP LEDs
	if( pin >= 16 && pin < 32 ) {
		gp_led = pin - 16;
		
		if( value )
			ledState[gp_led] |= (1 << 0); // set first color
		else
			ledState[gp_led] &= ~(1 << 0); // clear first color
		// color mapping at the end of this function

	} else if( pin >= 112 && pin < 128 ) {
		gp_led = pin - 112;
		
		if( value )
			ledState[gp_led] |= (1 << 1); // set second color
		else
			ledState[gp_led] &= ~(1 << 1); // clear second color
		// color mapping at the end of this function

	} else if( pin == 7 ) { // Beat LED
		if( value )
			[LED[16] setColor:[NSColor colorWithCalibratedRed:0.1 green:1.0 blue:0.1 alpha:1.0]];
		else
			[LED[16] setColor:[NSColor colorWithCalibratedRed:0.1 green:0.3 blue:0.1 alpha:1.0]];
	} else {
		// remaining LED functions via button highlighting
		// TODO: find solution to update this after button has been released!
		switch( pin ) {
			case  0: [_buttonTrack1 highlight:(value ? YES : NO)]; break;
			case  1: [_buttonTrack2 highlight:(value ? YES : NO)]; break;
			case  2: [_buttonTrack3 highlight:(value ? YES : NO)]; break;
			case  3: [_buttonTrack4 highlight:(value ? YES : NO)]; break;

			case  4: [_buttonPLayerA highlight:(value ? YES : NO)]; break;
			case  5: [_buttonPLayerB highlight:(value ? YES : NO)]; break;
			case  6: [_buttonPLayerC highlight:(value ? YES : NO)]; break;

			case 80: [_buttonGroup1 highlight:(value ? YES : NO)]; break;
			case 82: [_buttonGroup2 highlight:(value ? YES : NO)]; break;
			case 84: [_buttonGroup3 highlight:(value ? YES : NO)]; break;
			case 86: [_buttonGroup4 highlight:(value ? YES : NO)]; break;

			case 88: [_buttonTLayerA highlight:(value ? YES : NO)]; break;
			case 89: [_buttonTLayerB highlight:(value ? YES : NO)]; break;
			case 90: [_buttonTLayerC highlight:(value ? YES : NO)]; break;

			case  8: [_buttonEdit highlight:(value ? YES : NO)]; break;
			case  9: [_buttonMute highlight:(value ? YES : NO)]; break;
			case 10: [_buttonPattern highlight:(value ? YES : NO)]; break;
			case 11: [_buttonSong highlight:(value ? YES : NO)]; break;
			
			case 12: [_buttonSolo highlight:(value ? YES : NO)]; break;
			case 13: [_buttonFast highlight:(value ? YES : NO)]; break;
			case 14: [_buttonAll highlight:(value ? YES : NO)]; break;
			
			case 91: [_buttonPlay highlight:(value ? YES : NO)]; break;
			case 92: [_buttonStop highlight:(value ? YES : NO)]; break;
			case 93: [_buttonPause highlight:(value ? YES : NO)]; break;
			
			case 95: [_buttonStepView highlight:(value ? YES : NO)]; break;
		}
	}

	// handle dual colour option of GP LEDs
	if( gp_led >= 0 ) {
		switch( ledState[gp_led] ) {
			case 1:  [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:0.1 green:1.0 blue:0.1 alpha:1.0]]; break;
			case 2:  [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:1.0 green:0.3 blue:0.1 alpha:1.0]]; break;
			case 3:  [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:1.0 green:1.0 blue:0.1 alpha:1.0]]; break;
			default: [LED[gp_led] setColor:[NSColor colorWithCalibratedRed:0.1 green:0.3 blue:0.1 alpha:1.0]];
		}
	}

	return 0;
}

s32 MIOS32_DOUT_SRSet(u32 sr, u8 value)
{
	int i;

	for(i=0; i<8; ++i)
		MIOS32_DOUT_PinSet(sr*8+i, (value & (1 << i)) ? 1 : 0);

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
// Stubs for Board specific functions
//////////////////////////////////////////////////////////////////////////////
s32 MIOS32_BOARD_LED_Init(u32 leds)
{
	return -1; // not implemented
}

s32 MIOS32_BOARD_LED_Set(u32 leds, u32 value)
{
	return -1; // not implemented
}

u32 MIOS32_BOARD_LED_Get(void)
{
	return 0; // not implemented, return all-zero
}


//////////////////////////////////////////////////////////////////////////////
// The background task
// called each mS - in MIOS32 it's called whenever nothing else is to do
//////////////////////////////////////////////////////////////////////////////
- (void)backgroundTask:(NSTimer *)aTimer
{
	APP_Background();
}


//////////////////////////////////////////////////////////////////////////////
// The periodic 1mS task
//////////////////////////////////////////////////////////////////////////////
- (void)periodic1mSTask:(NSTimer *)aTimer
{
	// -> forward to application
	SEQ_TASK_Period1mS();
}


//////////////////////////////////////////////////////////////////////////////
// Called when a MIDI event has been received
//////////////////////////////////////////////////////////////////////////////
void sendMIDIMessageToApp(mios32_midi_port_t port, mios32_midi_package_t package)
{
  // from mios32_midi.c
  // remove cable number from package (MIOS32_MIDI passes it's own port number)
  package.cable = 0;

  // branch depending on package type
  if( package.type >= 0x8 ) {
	APP_NotifyReceivedEvent(port, package);
  } else {
	switch( package.type ) {
  	  case 0x0: // reserved, ignore
	  case 0x1: // cable events, ignore
	    break;

	  case 0x2: // Two-byte System Common messages like MTC, SongSelect, etc. 
	  case 0x3: // Three-byte System Common messages like SPP, etc. 
		APP_NotifyReceivedEvent(port, package); // -> forwarded as event
		break;
	  case 0x4: // SysEx starts or continues (3 bytes)
		APP_NotifyReceivedSysEx(port, package.evnt0); // -> forwarded as SysEx
		APP_NotifyReceivedSysEx(port, package.evnt1); // -> forwarded as SysEx
		APP_NotifyReceivedSysEx(port, package.evnt2); // -> forwarded as SysEx
	    break;
	  case 0x5: // Single-byte System Common Message or SysEx ends with following single byte. 
	    if( package.evnt0 >= 0xf8 )
		  APP_NotifyReceivedEvent(port, package); // -> forwarded as event
	    else
		  APP_NotifyReceivedSysEx(port, package.evnt0); // -> forwarded as SysEx
	    break;
	  case 0x6: // SysEx ends with following two bytes.
		APP_NotifyReceivedSysEx(port, package.evnt0); // -> forwarded as SysEx
		APP_NotifyReceivedSysEx(port, package.evnt1); // -> forwarded as SysEx
		break;
	  case 0x7: // SysEx ends with following three bytes.
		APP_NotifyReceivedSysEx(port, package.evnt0); // -> forwarded as SysEx
		APP_NotifyReceivedSysEx(port, package.evnt1); // -> forwarded as SysEx
		APP_NotifyReceivedSysEx(port, package.evnt2); // -> forwarded as SysEx
		break;
	}
  }
}

- (void)handleMIDIMessage:(Byte*)message ofSize:(int)size
{
  // from mios32_uart_midi.c
  // parses the next incoming byte(s), once a complete MIDI event has been 
  // received, forward it to APP_NotifyReceivedEvent or APP_NotifyReceivedSysEx

  u8 uart_port = 0; // currently only a single MIDI IN port is supported
  u8 my_port = 0x10; // port ID of MIDI IN
  midi_rec_t *midix = &midi_rec[uart_port];// simplify addressing of midi record
  int i;
  for(i=0; i<size; ++i) {
    u8 byte = message[i];

    if( byte & 0x80 ) { // new MIDI status
      if( byte >= 0xf8 ) { // events >= 0xf8 don't change the running status and can just be forwarded
		// Realtime messages don't change the running status and can be sent immediately
		midix->package.cin = 0xf; // F: single byte
		midix->package.evnt0 = byte;
		midix->package.evnt1 = 0x00;
		midix->package.evnt2 = 0x00;
		sendMIDIMessageToApp(my_port, midix->package);
      } else {
		midix->running_status = byte;
		midix->expected_bytes = mios32_midi_expected_bytes_common[(byte >> 4) & 0x7];

	    if( !midix->expected_bytes ) { // System Message, take number of bytes from expected_bytes_system[] array
		  midix->expected_bytes = mios32_midi_expected_bytes_system[byte & 0xf];

		  if( byte == 0xf0 ) {
	        midix->package.evnt0 = 0xf0; // midix->package.evnt0 only used by SysEx handler for continuous data streams!
	        midix->sysex_ctr = 0x01;
		  } else if( byte == 0xf7 ) {
		    switch( midix->sysex_ctr ) {
 	          case 0:
		        midix->package.cin = 5; // 5: SysEx ends with single byte
		        midix->package.evnt0 = 0xf7;
		        midix->package.evnt1 = 0x00;
		        midix->package.evnt2 = 0x00;
		        break;
	          case 1:
		        midix->package.cin = 6; // 6: SysEx ends with two bytes
		        // midix->package.evnt0 = // already stored
		        midix->package.evnt1 = 0xf7;
		        midix->package.evnt2 = 0x00;
		        break;
	          default:
		        midix->package.cin = 7; // 7: SysEx ends with three bytes
		        // midix->package.evnt0 = // already stored
		        // midix->package.evnt1 = // already stored
		        midix->package.evnt2 = 0xf7;
		        break;
	        }
			sendMIDIMessageToApp(my_port, midix->package);
	        midix->sysex_ctr = 0x00; // ensure that next F7 will just send F7
	      }
	    }

	    midix->wait_bytes = midix->expected_bytes;
      }
    } else {
      if( midix->running_status == 0xf0 ) {
	    switch( ++midix->sysex_ctr ) {
  	      case 1:
	        midix->package.evnt0 = byte; 
	        break;
	      case 2: 
	        midix->package.evnt1 = byte; 
	        break;
	      default: // 3
	        midix->package.evnt2 = byte;

	        // Send three-byte event
	        midix->package.cin = 4;  // 4: SysEx starts or continues
			sendMIDIMessageToApp(my_port, midix->package);
	        midix->sysex_ctr = 0x00; // reset and prepare for next packet
	    }
      } else { // Common MIDI message or 0xf1 >= status >= 0xf7
	    if( !midix->wait_bytes ) {
	      midix->wait_bytes = midix->expected_bytes - 1;
	    } else {
	      --midix->wait_bytes;
	    }

		if( midix->expected_bytes == 1 ) {
	      midix->package.evnt1 = byte;
	      midix->package.evnt2 = 0x00;
	    } else {
	      if( midix->wait_bytes )
	        midix->package.evnt1 = byte;
	      else
	        midix->package.evnt2 = byte;
	    }
	
	    if( !midix->wait_bytes ) {
	      if( (midix->running_status & 0xf0) != 0xf0 ) {
	        midix->package.cin = midix->running_status >> 4; // common MIDI message
	      } else {
	        switch( midix->expected_bytes ) { // MEMO: == 0 comparison was a bug in original MBHP_USB code
  	          case 0: 
		        midix->package.cin = 5; // 5: SysEx common with one byte
		        break;
  	          case 1: 
		        midix->package.cin = 2; // 2: SysEx common with two bytes
		        break;
			  default: 
		        midix->package.cin = 3; // 3: SysEx common with three bytes
		        break;
	        }
	      }

		  midix->package.evnt0 = midix->running_status;
	      // midix->package.evnt1 = // already stored
	      // midix->package.evnt2 = // already stored
		  sendMIDIMessageToApp(my_port, midix->package);
		}
      }
    }
  }
}

- (void)processMIDIPacketList:(const MIDIPacketList*)packets sender:(id)sender
{
	// from http://svn.notahat.com/simplesynth/trunk/AudioSystem.m
    int						i, j;
    const MIDIPacket*		packet;
    Byte					message[256];
    int						messageSize = 0;
    
    // Step through each packet
    packet = packets->packet;
    for (i = 0; i < packets->numPackets; i++) {
        for (j = 0; j < packet->length; j++) {
            if (packet->data[j] >= 0xF8) continue;				// skip over real-time data
            
            // Hand off the packet for processing when the next one starts
            if ((packet->data[j] & 0x80) != 0 && messageSize > 0) {
                [self handleMIDIMessage:message ofSize:messageSize];
                messageSize = 0;
            }
            
            message[messageSize++] = packet->data[j];			// push the data into the message
        }
        
        packet = MIDIPacketNext (packet);
    }
    
    if (messageSize > 0)
        [self handleMIDIMessage:message ofSize:messageSize];
}


//////////////////////////////////////////////////////////////////////////////
// init local variables
//////////////////////////////////////////////////////////////////////////////
- (void) awakeFromNib
{
	int i;

	_self = self;
	
	// make object specific pointers visible for native C
	LCD[0]=lcdView1;
	LCD[1]=lcdView2;
	selectedLCD = 0;

	LED[0]=LED1;
	LED[1]=LED2;
	LED[2]=LED3;
	LED[3]=LED4;
	LED[4]=LED5;
	LED[5]=LED6;
	LED[6]=LED7;
	LED[7]=LED8;
	LED[8]=LED9;
	LED[9]=LED10;
	LED[10]=LED11;
	LED[11]=LED12;
	LED[12]=LED13;
	LED[13]=LED14;
	LED[14]=LED15;
	LED[15]=LED16;
	LED[16]=LEDBeat;

	// (only buttons with "LED" function)
	_buttonTrack1 = buttonTrack1;
	_buttonTrack2 = buttonTrack2;
	_buttonTrack3 = buttonTrack3;
	_buttonTrack4 = buttonTrack4;

	_buttonGroup1 = buttonGroup1;
	_buttonGroup2 = buttonGroup2;
	_buttonGroup3 = buttonGroup3;
	_buttonGroup4 = buttonGroup4;

	_buttonPLayerA = buttonPLayerA;
	_buttonPLayerB = buttonPLayerB;
	_buttonPLayerC = buttonPLayerC;

	_buttonTLayerA = buttonTLayerA;
	_buttonTLayerB = buttonTLayerB;
	_buttonTLayerC = buttonTLayerC;

	_buttonEdit = buttonEdit;
	_buttonMute = buttonMute;
	_buttonPattern = buttonPattern;
	_buttonSong = buttonSong;

	_buttonSolo = buttonSolo;
	_buttonFast = buttonFast;
	_buttonAll = buttonAll;
	_buttonStepView = buttonStepView;

	_buttonPlay = buttonPlay;
	_buttonStop = buttonStop;
	_buttonPause = buttonPause;

	// clear LED states
	for(i=0; i<NUM_LEDS; ++i)
		ledState[i] = 0;
	
	// create virtual MIDI ports
	for(i=0; i<NUM_MIDI_IN; ++i) {
		NSMutableString *portName = [[NSMutableString alloc] init];
		[portName appendFormat:@"vMBSEQ IN%d", i+1];
		virtualMIDI_IN[i] = [[PYMIDIVirtualDestination alloc] initWithName:portName];

		midi_rec[i].package.ALL = 0;
		midi_rec[i].running_status = 0x00;
		midi_rec[i].expected_bytes = 0x00;
		midi_rec[i].wait_bytes = 0x00;
		midi_rec[i].sysex_ctr = 0x00;

		[virtualMIDI_IN[i] addReceiver:self];
	}

	for(i=0; i<NUM_MIDI_OUT; ++i) {
		NSMutableString *portName = [[NSMutableString alloc] init];
		[portName appendFormat:@"vMBSEQ OUT%d", i+1];
		virtualMIDI_OUT[i] = [[PYMIDIVirtualSource alloc] initWithName:portName];
	}

	// call init function of application
	Init(0);
	
	// install background task for all modes
	NSTimer *timer1 = [NSTimer timerWithTimeInterval:0.001 target:self selector:@selector(backgroundTask:) userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer: timer1 forMode: NSRunLoopCommonModes];

	// install 1mS task
	NSTimer *timer2 = [NSTimer timerWithTimeInterval:0.001 target:self selector:@selector(periodic1mSTask:) userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer: timer2 forMode: NSRunLoopCommonModes];	
}

@end
