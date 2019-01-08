$Id: README.txt 69 2008-10-06 22:24:22Z tk $

Demo application for MIOS32_SRIO driver
===============================================================================
Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or STM32 Primer
   o at least one MBHP_DIN module
   o at least one MBHP_DOUT module

===============================================================================

This application demonstrates the MIOS32_SRIO driver

- DIN pins will be forwarded to DOUT pins
- DIN pins send a MIDI message (Note Events)
- DOUT pins controllable via Note Events
- Encoders at SR 13/14 will send CC events

Pin connections:
  o RCLK (A4)      -> RC pin of MBHP_DIN/DOUT modules
  o SPI1_SCLK (A5) -> SC pin of MBHP_DIN/DOUT modules
  o SPI1_MISO (A6) -> SI pin of MBHP_DIN module
  o SPI1_MOSI (A7) -> SO Pin of MBHP_DOUT module

===============================================================================
