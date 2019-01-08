$Id$

MIOS32 Tutorial #016: Using AOUTs and a Notestack
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
     (STM32: DAC voltages are output at pin RA4 (J16:RC1) and RA5 (J16:SC)

Optional hardware:
   o MBHP_AOUT, MBHP_AOUT_LC or MBHP_AOUT_NG module

===============================================================================

This tutorial lesson is the first one which gets use of module drivers,
which are located under $MIOS32_PATH/modules

  o $MIOS32_PATH/modules/aout: allows to control CV voltages of the internal
    DAC channels, an MBHP_AOUT, MBHP_AOUT_LC or MBHP_AOUT_NG module

  o $MIOS32_PATH/modules/notestack: a note stack handler which stores
    received Notes in a stack, so that it is possible to recall the
    note number and velocity of still played notes when a key is released

Modules are added to the project from the Makefile (-> see comments there)


AOUT Driver
-----------

See also the documentation at this page:
  http://www.midibox.org/mios32/manual/group___a_o_u_t.html


The AOUT module is initialized in APP_Init():
-------------------------------------------------------------------------------
  // configure interface
  // see AOUT module documentation for available interfaces and options
  aout_config_t config;
  config = AOUT_ConfigGet();
  config.if_type = AOUT_IF_INTDAC;
  config.if_option = 0;
  config.num_channels = 8; // INTDAC: only 2 channels supported, 8 channels pre-configured for your own comfort
  config.chn_inverted = 0;
  AOUT_ConfigSet(config);
  AOUT_IF_Init(0);
-------------------------------------------------------------------------------

If a different AOUT module should be tested, an adaption in Init() (-> app.c)
is required. Change config.if_type to one of the following values:
  - AOUT_IF_MAX525
  - AOUT_IF_74HC595
  - AOUT_IF_TLV5630
  - AOUT_IF_INTDAC


AOUT_IF_INTDAC will output analog values (0..3.3V) at pin RA4 (J16:RC1 and
J16:SC)

Common AOUT modules are connected to J19 of the MBHP_CORE_STM32 module:
   J19:Vs -> AOUT Vs
   J19:Vd -> AOUT Vd
   J19:SO -> AOUT SI
   J19:SC -> AOUT SC
   J19:RC1 -> AOUT CS
Note that this is *not* an 1:1 pin assignment (an adapter has to be used)!

The voltage configuration jumper of J19 has to be set to 5V, and
a 4x1k Pull-Up resistor array should be installed, since the IO pins
are configured in open-drain mode for 3.3V->5V level shifting.



Notestack Driver
----------------

The notestack is defined in the app.h header:
-------------------------------------------------------------------------------
#define NOTESTACK_SIZE 16

static notestack_t notestack;
static notestack_item_t notestack_items[NOTESTACK_SIZE];
-------------------------------------------------------------------------------

and initialized in APP_Init():
-------------------------------------------------------------------------------
  // initialize the Notestack
  NOTESTACK_Init(&notestack, NOTESTACK_MODE_PUSH_TOP, &notestack_items[0], NOTESTACK_SIZE);
-------------------------------------------------------------------------------
The available notestack modes are documented under
  http://www.midibox.org/mios32/manual/group___n_o_t_e_s_t_a_c_k.html
NOTESTACK_MODE_PUSH_TOP is the most common one.

Whenever a Note On is received, it has to be pushed onto the stack with:
-------------------------------------------------------------------------------
    NOTESTACK_Push(&notestack, midi_package.note, midi_package.velocity);
-------------------------------------------------------------------------------

Whenever a Note Off is received, it has to be removed from the stack with:
-------------------------------------------------------------------------------
    NOTESTACK_Pop(&notestack, midi_package.note);
-------------------------------------------------------------------------------

Now you can check if a Note is still in the stack or not, and set the
CV voltages accordingly.

-------------------------------------------------------------------------------
    if( notestack.len ) {
      // take first note of stack
      // we have to convert the 7bit value to a 16bit value
      u16 note_cv = notestack_items[0].note << 9;
      u16 velocity_cv = notestack_items[0].tag << 9;

      ... set CV voltage ...
      ... set gate to 1 ...
    } else {
      ... set gate to 0 ...
    }
-------------------------------------------------------------------------------


IMPORTANT
---------

Before voltage changes are transfered to the external hardware, the 
AOUT_PinSet function compares the new value with the current one.
If equal, the register transfer will be omitted, otherwise it
will be requested and performed once AOUT_Update() is called.

This method has two advantages:
  o if AOUT_PinSet doesn't notice value changes, the appr. AOUT channels
     won't be updated to save CPU performance
  o all CV pins will be updated at the same moment


So, don't forget to execute:
-------------------------------------------------------------------------------
    AOUT_Update();
-------------------------------------------------------------------------------

either immediately after pin changes, or periodically from a timed task.


Testing the application
-----------------------

Even if no AOUT module is connected, you can evaluate the notestack handling
by opening the MIOS Terminal. Whenever a Note is received, the new content
of the Notestack will be send to the terminal as plain text messages.

The Board LED should turn on as long as a key is pressed on your keyboard.
It should turn off once no key is played anymore.


With an AOUT module connected, the first CV channel should output a
voltage depending on the note number, and the second CV channel outputs
the velocity.

The remaining (6) channels can be controlled with CC#16..CC#21

===============================================================================
