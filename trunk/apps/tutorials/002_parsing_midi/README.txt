$Id$

MIOS32 Tutorial #002: Parsing MIDI
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17 or MBHP_CORE_STM32F4 or similar

===============================================================================

The second part of the tutorial demonstrates, how to parse incoming MIDI
events to control the Status LED.

See Tutorial #001 (001_forwarding_midi) for details about the application
structure, the APP_Init() and APP_MIDI_NotifyPackage() hooks


Let's continue with the MIDI notification hook, which is called by
the programming model whenever MIOS32 receives a MIDI package:
-------------------------------------------------------------------------------
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
...
-------------------------------------------------------------------------------

This hook gets the port and midi_package as argument.
The appr. variable types (mios32_midi_port_t and mios32_midi_package_t) are
defined under $MIOS32_PATH/include/mios32/mios32_midi.h

Available ports are currently DEFAULT, DEBUG, USBx, UARTx and IICx, 
where x is a number from 0 to 15. However, physically available are
only USB0, UART0 and UART1 by default.

DEFAULT should be the prefered port, it is an alias to a physical 
interface which can be changed by the user during runtime.
DEFAULT usually points to the "USB0" port.

DEBUG is used to transmit/receive debug messages (see 003_debug_messages)
DEBUG usually points to the "USB0" port as well.

More USB ports can be optionally added (see later tutorials).
IIC ports require additional hardware (see later tutorials).


The coding of "mios32_midi_package_t" is inspired from the USB MIDI 
specification, and used for all MIDI ports. Using this format simplifies
the processing of MIDI events a lot, especially because all events (beside
of SysEx streams) arrive in a de-serialized format which simplifies parsing
and routing of multiple ports.

The basic structure of a MIDI package:

  struct {
    unsigned type:4;
    unsigned cable:4;
    unsigned evnt0:8;
    unsigned evnt1:8;
    unsigned evnt2:8;
  };

List of 16 possible midi_package.types:

Type  Size  Name     Description
-------------------------------------------------------------------------------
0x0	3  -		  reserved
0x1	3  -		  reserved
0x2	2  -		  two-byte system common messages like MTC, Song Select, etc.
0x3	3  -		  three-byte system common messages like SPP, etc.
0x4	3  -		  SysEx starts or continues
0x5	1  -	          Single-byte system common message or sysex sends with following single byte
0x6	2  -		  SysEx sends with following two bytes
0x7	3  -	          SysEx sends with following three bytes
0x8	3  NoteOff	  Note Off event
0x9	3  NoteOn	  Note On event
0xa	3  PolyPressure   Poly-Key Press event
0xb	3  CC		  Control Change event
0xc	2  ProgramChange  Program Change event
0xd	2  Aftertouch	  Channel Preassure event
0xe	3  PitchBend	  PitchBend Change event
0xf	1  -		  Single Byte
-------------------------------------------------------------------------------

midi_package.cable are used by MIOS32_MIDI functions internally before
MIDI events will be sent (part of the USB MIDI spec to specifiy the
port number of USB0...USB15)

The content of midi_package.evnt0/evnt1/evnt2 depends on midi_package.type

E.g., with midi_package.type == 0x9:
   - midi_package.event1 contains the MIDI Event Status (0x9 as well)
     and Channel Number (0x0..0xf),
   - midi_package.evnt1 contains the note number
   - midi_package.evnt2 contains the velocity

More details about MIDI events can be found in the MIDI specification:
http://www.midibox.org/dokuwiki/doku.php?id=midi_specification


In order to improve readability, mios32_midi_package_t provides
some alternative views (aliases) for members of the 4-byte structure.

E.g.:
  struct {
    unsigned cin:4;
    unsigned cable:4;
    mios32_midi_chn_t chn:4;
    mios32_midi_event_t event:4;
    unsigned note:8;
    unsigned velocity:8;
  };

allows you to access Note/Velocity as:
   midi_package.chn
   midi_package.note
   midi_package.velocity
instead of midi_package.evnt0, midi_package.evnt1 and midi_package.evnt2


With this knowledge, you should be able to change the control of the Status
LED, so that it won't be toggled on *each* incoming MIDI event, but only if
a MIDI Note Event has been received.


1) the LED should be turned on whenever a Note On Event with velocity > 0
has been received

2) the LED should be turned off whenever a Note Off Event or a Note On
event with velocity == 0 has been received (the MIDI spec says, that velocity 0
should be handled like Note Off)

The appr. code can be found in app.c:
-------------------------------------------------------------------------------
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  if( midi_package.type == NoteOn && midi_package.velocity > 0 )
    MIOS32_BOARD_LED_Set(1, 1);
  else if( midi_package.type == NoteOff ||
	   midi_package.type == NoteOn && midi_package.velocity == 0 )
    MIOS32_BOARD_LED_Set(1, 0);
}
-------------------------------------------------------------------------------

Hint: MIDI Notes can be sent with the virtual keyboard of MIOS Studio, or
alternatively by connecting a MIDI keyboard to MIDI IN1 or IN2 of your
core module.


Exercise
--------

Enhance the APP_MIDI_NotifyPackage() hook, so that the LED is only controlled
by Note events received over MIDI Channel #4

===============================================================================
