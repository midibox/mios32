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
	o Button and/or LED matrix connected to DIN's / DOUT's

===============================================================================

This application checks the handling of a Button/LED matrix driven by the blm_x
module. 

The first push on a button resets all LED's at this button to 0 (off). Each
subsequent push on the same button switches to the next color (combination
of LED's at this button). If all possible color-combinations are walked through, 
the LED's go all to 0 (off) again. 
The color (mix of all available LED's at one matrix point) is set by a 32bit
integer, each bit represents a LED. Each button push just increments this value
until the number of configured colors is reached, and wraps back to 0.

Note that you could theoretically control up to 32 LED's each matrix-point! 
The most usual application will be to drive just two- or three-color-LED's.

If a button-state-change occurs, it will be displayed on a connected LCD and
sent to the MIOS-Studio console. In the console you will also receive the set
LED-colors as 32-bit hex-number.

The number of button-changes is also counted and reported to LCD / MIOS-Studio
console. This value helps you to detect bouncing buttons. Use the BLM_X_DebounceDelaySet(..)
function to set/modify the debounce_delay (number of scan-cycles to ignore button changes
after a change). The default value is 0, as the scan cycle (1mS x BLM_X_NUM_ROWS) could be
enough delay for a lot of applications. Note that there's also the choice between debounce
mode 1 (global button debouncing), and mode 2 (individual button debouncing). Mode 2 is a 
bit more resource-consuming. You can set mode by defining it in your mios32_config.h file.

Refer also the README.txt file in the modules/blm_x folder for descriptions to all available 
configuration options.


Connecting the Button-LED matrix to the serial registers
---------------------------------------------------------

***TODO***


===============================================================================
