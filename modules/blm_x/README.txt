Button/LED matrix driver for MIOS32
===============================================================================
Copyright (C) 2009 Matthias Mächler (maechler@mm-computing.ch, thismaechler@gmx.ch)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

This module is able to handle a button/LED matrix with X rows (up to 8), X cols
and X colors (up to 32).

The module uses the MIOS32 DIN/DOUT module to drive the button/LED - matrix. You
can configure 1 up to 8 row-select-lines. I decided to call the select-lines "rows"
instead of "cols", because I think it's more natural to read the button/LED numbers
from left to right and from top to bottom instead top-bottom-left-right.
The number of columns is only limited by the number of DIN's and DOUT's you have
connected to the core.
The module also supports multi-color-LED's (or just x LED's per matrix-point). One
common application is to control three-color LED's for each button.

The number of rows (the select lines) is common to LED's and buttons, the number
of columns can be configured individually for LED's and buttons.

You will need one DOUT serial-register for the select-lines. If there are less than
5 rows, the first nibble will be mirrored to the second one (pin 0-3 == pin 4-7).

For the buttons you will need [ceil(num_cols / 8)] DIN SR's. 

For the LED's you will need [ceil(num_cols*num_colors / 8)] DOUT SR's. The colors
will be just chained up ([LED-col 0, color 0][LED-col 1, color 0]....[LED-col 1, color0]...).
If you want to work directly on the virtual SR's, you may prefer to choose num_cols values like
4,8,12,16 etc. If you set LED states only by module functions, you don't have to care.

Performance: The whole SRIO service takes ca. 280uS without the BLM-X module hooks, and ca.
300uS with the hooks (tested so far with 4x4x3 matrix and the expensive debounce-mode 2), so
all this is really no matter of performance.

*** more info will follow soon ***