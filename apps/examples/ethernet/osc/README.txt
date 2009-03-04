$Id$

Demo application OSC Server/Client with direct Ethernet access
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or STM32 Primer
   o MBHP_CORE_ETH module
     Connected to J16 (no pull resistors, jumpered for 3.3V!)

Optional:
   o one or more DIN modules + buttons
     Connected to J8/9 (1k or 220 Ohm pull resistors, jumpered for 5V)
   o one or more DOUT modules + LEDs
     Connected to J8/9 (1k or 220 Ohm pull resistors, jumpered for 5V)

   o one or more pots/(motor)faders
     Connected to J5A/B

   o MBHP_MF module to control motorfaders
     Connected to J19 (no pull resistors, jumpered for 3.3V)

===============================================================================

This application demonstrates the implementation of an OSC server and client,
which sends OSC packets directly to an Ethernet connection without the need
for a proxy.

  - Ethernet packets are handled in uip_task.c

  - OSC packets are handled in osc_server.c
    This file also contains OSC methods which are called on received packets

  - Outgoing OSC packets are created in osc_client.c, before they are
    forwarded to osc_server.c

  - DIN/AIN pin changes are forwarded to the OSC client in app.c


The ethernet configuration (IP address, netmask, gateway, OSC remote IP and port)
can be adapted in mios32_config.h if desired


Supported OSC messages
======================

Outgoing:
  /cs/button/state <button-number> <0 or 1>
  Sent on button changes

  /cs/pot/value <pot-number> <0.0000..1.0000>
  Sent on fader movements


Incoming:
  o /cs/led/set <led-number> <value>
    <led-number> can be float32 or int32
    <value> can be float32 (<1: clear, >=1: set), int32 (0 or != 0) or TRUE/FALSE


  o /cs/led/set_all <value>
    <value> can be float32 (<1: clear, >=1: set), int32 (0 or != 0) or TRUE/FALSE

  o /cs/mf/set <motorfader-number> <value>
    <value> has to be a float32 (range 0.00000..1.00000)

  o /cs/mf/set_all <value>
    <value> has to be a float32 (range 0.00000..1.00000)


  o /cfg/mf/deadband <motorfader-number> <value>
  o /cfg/mf/pwm_period <motorfader-number> <value>
  o /cfg/mf/pwm_duty_cycle_down <motorfader-number> <value>
  o /cfg/mf/pwm_duty_cycle_up <motorfader-number> <value>
    <value> can be float32 or int32

===============================================================================
