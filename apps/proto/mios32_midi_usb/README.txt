$Id$

Demo application for MIOS32_MIDI and MIOS32_USB driver
===============================================================================
Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

Currently only STM32 Primer supported!

===============================================================================

Required hardware:
   o STM32 Primer (or upcoming MBHP_CORE_STM32 module)
   o at least one MBHP_DIN module
   o optionally one MBHP_DOUT module

===============================================================================

This application demonstrates the MIOS32_MIDI driver

DIN pins send various MIDI messages over USB (see main.c, DIN_NotifyToggle function)
Incoming MIDI events will be forwarded to the OUT port for testing bidirectional transfers

In addition, the two LEDs of STM32 Primer are flashing to send a "sign of life"

Pin connections:
  o SPI1_SCLK (A5) -> Sc pin of MBHP_DIN/DOUT modules
  o SPI1_MISO (A6) -> Si pin of MBHP_DIN module
  o SPI1_MOSI (A7) -> So Pin of MBHP_DOUT module
  o RCLK (A8)      -> Rc pin of MBHP_DIN/DOUT modules

===============================================================================
