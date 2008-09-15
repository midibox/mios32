$Id$

Checks the handling of a button/LED matrix
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
   o at least one MBHP_DOUT module

===============================================================================

This application checks the handling of a Button/LED matrix

See also the original PIC based application:
http://svnmios.midibox.org/listing.php?repname=svn.mios&path=%2Ftrunk%2Fapps%2Fexamples%2Fbutton_duoled_matrix%2F

Note, tat the MIOS32 based BLM driver also supports Single, Duo and Triple (RGB) LEDs
(see SR definitions in $MIOS32_PATH/modules/blm/blm.h)

In addition, the two LEDs of STM32 Primer are flashing to send a "sign of life"

Pin connections:
  o SPI1_SCLK (A5) -> Sc pin of MBHP_DIN/DOUT modules
  o SPI1_MISO (A6) -> Si pin of MBHP_DIN module
  o SPI1_MOSI (A7) -> So Pin of MBHP_DOUT module
  o RCLK (A8)      -> Rc pin of MBHP_DIN/DOUT modules

===============================================================================
