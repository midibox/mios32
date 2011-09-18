$Id$

MIOS32 Tutorial #001: Forwarding MIDI
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17

===============================================================================

The first part of the tutorial demonstrates, how to route MIDI data from
the USB based interface to the MIDI OUT port and vice versa (MIDI IN to USB)

Before you will experience, how easy this can be implemented, I would like to
give you a general introduction about the program structure in a MIOS32 based
application.

An application mainly consists of:
  - the FreeRTOS layer: a Realtime Operating System which provides
    preemptive task handling and synchronisation mechanisms.
    FreeRTOS sources are located under $MIOS32_PATH/FreeRTOS
    Documentation: http://www.freertos.org/

  - the STM32 driver layer under $MIOS32_PATH/$MIOS32_FAMILY
    (e.g. $MIOS32_PATH/STM32F10x)
    Documentation: http://www.stm.com/stm32

  - the MIOS32 layer which is on top of the driver layer, so
    that an application doesn't need to access chip dependent
    low-level routines. This makes it portable for future devices.
    MIOS32 routines are located under $MIOS32_PATH/mios32
    Documentation: http://www.midibox.org/mios32/manual

  - the so called "programming model" which combines MIOS32
    functions with FreeRTOS tasks.
    It is located under $MIOS32_PATH/programming_model/traditional

  - additional driver modules that are less commonly used,
    or where different variants exist or which have been
    provided as a package by third parties 
    (e.g. DOSFS or uIP)
    Documentation can mostly be found in README.txt files

  - and finally the application itself, the part on which you are
    working on.
    The application is located in app.c


Whenever the application is started (after power-on or chip reset),
APP_Init() will be called to initialize the application:

-------------------------------------------------------------------------------
void APP_Init(void)
{
  // initialize all LEDs
  MIOS32_BOARD_LED_Init(0xffffffff);
}
-------------------------------------------------------------------------------

The programming model already initialized MIOS32 specific resources and
handlers which are running in background (see MIOS32_PATH/programming_model/traditional/main.c),
This hook allows you to do some additional, application specific initialisation
before FreeRTOS is booted, and tasks are starting to execute.

We want to initialize the green status LED of the MBHP_CORE_STM32 module.
Instead of accessing the LED pin (PD2) directly, a function of the MIOS32_BOARD
class is used. You will like this way of abstraction once you are trying
to run your application on a different core module (later), which could
have a different pin layout.
You will also like this programming style if you want to emulate your 
application on a PC or Mac, since STM32 specific driver functions are not 
portable (for obvious reasons).

Each LED has a dedicated bit, and since we don't know how many LEDs are
directly available on the board, we initialize them all by setting all
32 bits of the function argument, which results into the hexadecimal
value 0xffffffff (well prepared for future extensions ;)

Next step: let's fill the hook which is called by the programming
model whenever MIOS32 receives a MIDI package:

-------------------------------------------------------------------------------
void APP_MIDI_NotifyPackage(mios32_midi_port_t port, mios32_midi_package_t midi_package)
{
  // toggle Status LED on each incoming MIDI package
  MIOS32_BOARD_LED_Set(1, ~MIOS32_BOARD_LED_Get());

  // forward USB0 <-> UART0
  switch( port ) {
    case USB0:  MIOS32_MIDI_SendPackage(UART0, midi_package); break;
    case UART0: MIOS32_MIDI_SendPackage(USB0,  midi_package); break;
  }
}
-------------------------------------------------------------------------------

This hook gets the port and midi_package as argument.
The content of these variables are explained in more detail in the 
second tutorial 002_parsing_midi!


At function entry, we toggle the Status LED by reading the current
value (-> MIOS32_BOARD_LED_Get()), inverting it (-> "~"), and writing
the new value back for the first LED only via MIOS32_BOARD_LED_Set(1, ...)

This part of code could also be written as:
-------------------------------------------------------------------------------
  if( MIOS32_BOARD_LED_Get() & 1 )
    MIOS32_BOARD_LED_Set(1, 0); // Toggle from 0 to 1
  else
    MIOS32_BOARD_LED_Set(1, 1); // Toggle from 1 to 0
-------------------------------------------------------------------------------
...but the single line solution looks more compact.


After toggling the LED, we branch depending on the MIDI port which
received the MIDI package.

-------------------------------------------------------------------------------
  // forward USB0->UART0 and UART0->USB0
  switch( port ) {
    case USB0:  MIOS32_MIDI_SendPackage(UART0, midi_package); break;
    case UART0: MIOS32_MIDI_SendPackage(USB0,  midi_package); break;
  }
-------------------------------------------------------------------------------

Whenever the MIDI IN port of USB0 received a package, it will be forwarded
to the MIDI OUT of UART0 (MIDI IN1/OUT1 of the MBHP_CORE_STM32 module)

Whenever the MIDI IN port of UART0 received a package, it will be forwarded
to MIDI OUT of USB0


The next tutorial 002_parsing_midi demonstrates, how to parse the MIDI events.

===============================================================================
