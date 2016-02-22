$Id$

COM Terminal for ESP8266 WiFi module
===============================================================================
Copyright (C) 2016 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17 or MBHP_CORE_STM32F4
   o ..

===============================================================================

Currently only tested with MBHP_CORE_STM32F4

Connections:
- ESP8266 GND -> GND
- ESP8266 Vcc -> 3V
- ESP8266 TXD -> PB7 (UART2 RX, normally used for MIDI IN3)
- ESP8266 RXD -> PC6 (UART2 TX, normally used for MIDI OUT3)
- ESP8266 CH_PD to 3V (will later be connected to a GPIO to turn the module on/off)


commands can be sent from the MIOS32 (MIDI) Terminal with:
!<command>

E.g.
!AT
should return
OK

See http://www.pridopia.co.uk/pi-doc/ESP8266ATCommandsSet.pdf for the command set
and https://cdn.sparkfun.com/assets/learn_tutorials/4/0/3/4A-ESP8266__AT_Instruction_Set__EN_v0.30.pdf


Initialisation Example for OSC:

!AT+CWMODE=3
!AT+CWLAP
!AT+CWJAP="<wifi-net>","<password>"
!AT+CIFSR
!AT+CIPMUX=0
!AT+CIPSTART="UDP","0",0,10000,2

enter "osctest" to send a message
received messages will be displayed as hex dump

===============================================================================
