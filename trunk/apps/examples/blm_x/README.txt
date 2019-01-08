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

After startup, the application will walk through all LED matrix-crosspoints and 
set all possible color combinations. When the last possible color is reached,
it starts again with the first color. With tripple-color-LED's you get 7 possible
color comibnations (without all off).

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
sent to the MIOS-Studio console. In the console and on the LCD you will also 
see the set LED-colors as hex-number.

The number of button-changes is also counted and reported to LCD / MIOS-Studio
console. This value helps you to detect bouncing buttons. Use the BLM_X_ConfigGet(..) /
BLM_X_ConfigSet(..) functions to set/modify the debounce_delay (number of scan-cycles 
to ignore button changes after a change). 
By default, debouncing is disabled (BLM_X_DEBOUNCE_MODE=0), as the scan cycle 
(1mS x BLM_X_NUM_ROWS) is enough delay for most of applications / hardware. 
Note that there's also the choice between debounce mode 1 (global button debouncing),
and mode 2 (individual button debouncing). Mode 2 is a bit more resource-consuming. 
You can set mode by defining it in your mios32_config.h file.

Refer also the README.txt file in the modules/blm_x folder for descriptions to all available 
configuration options.

Performance
-----------
The whole SRIO service takes ca. 
- ca. 280uS without the BLM-X module hooks
- ca.290uS with 4 rows, 4 cols, 3 colors, debounce mode 2 (LEDs & buttons)
- ca. 320uS with 8 rows, 16 cols, 3 colors, debounce mode 2 (LEDs & buttons)

Most of this additional time is consumed by heavy debouncing (mode 2). Without any
debouncing (not required in most applications, 4 rows == 4mS scan cycle), this
additional time will be dramatically lower.



Connecting the Button-LED matrix to the serial registers
---------------------------------------------------------

***TODO***


===============================================================================
