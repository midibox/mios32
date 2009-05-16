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
   o MBHP_CORE_STM32 or STM32 Primer
   o at least one MBHP_DIN module
   o at least one MBHP_DOUT module

===============================================================================

This application checks the handling of a Button/LED matrix

See also the original PIC based application:
http://svnmios.midibox.org/listing.php?repname=svn.mios&path=%2Ftrunk%2Fapps%2Fexamples%2Fbutton_duoled_matrix%2F

Note, tat the MIOS32 based BLM driver also supports Single, Duo and Triple (RGB) LEDs
(see SR definitions in $MIOS32_PATH/modules/blm/blm.h)

In addition, the two LEDs of STM32 Primer are flashing to send a "sign of life"


The BLM configuration is done in mios32_config.h

===============================================================================
