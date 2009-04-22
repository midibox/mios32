Checks the handling of a button/LED matrix driven by the blm_x driver
===============================================================================
Copyright (C) 2009 Matthias Mächler (maechler@mm-computing.ch, thismaechler@gmx.ch)
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

This application checks the handling of a Button/LED matrix driven by the blm_x
module. The first push on a button resets the LED at this button to 0 (off). Each
subsequent push on the same button switches to the next color. If all possible
color-combinations are walked through, the LED's go all to 0 (off) again.
The color (mix of all available LED's at one matrix point) is set by a 32bit
integer, each bit represents a LED. Each button push just increments this value
until the number of configured colors is reached, and wraps back to 0.

Note that you could theoretically control up to 32 LED's each matrix-point! 
The most usual application will be to drive just two- or three-color-LED's.

*** more information will be added soon ***


===============================================================================
