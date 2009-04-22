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
from left to right instead of top to bottom.
The number of columns is only limited by the number of DIN's and DOUT's you have
connected to the core.
The module also supports multi-color-LED's (or just x LED's per matrix-crosspoint). One
common application is to control three-color LED's for each button.

The number of rows (the select lines) is common to LED's and buttons, the number
of columns can be configured individually for LED's and buttons.

Serial registers
----------------
You will need one DOUT serial-register for the select-lines. If there are less than
5 rows, the first nibble will be mirrored to the second one (pin 0-3 == pin 4-7).

For the buttons you will need [ceil(BLM_X_BTN_NUM_COLS / 8)] DIN SR's. 

For the LED's you will need [ceil(BLM_X_LED_NUM_COLS * BLM_X_LED_NUM_COLORS / 8)] DOUT SR's. 
The colors will be just chained up ([LED-col 0, color 0][LED-col 1, color 0]....[LED-col 1, color0]...).
If you want to work directly on the virtual SR's, you may prefer to choose BLM_X_LED_NUM_COLS values like
4,8,12,16 etc. If you set LED states only by module functions, you don't have to care.


Debouncing
----------
Debounce-modes 1 and 2 are available: mode 1 uses a single counter, which decrements each scan-cycle
(BLM_X_NUM_ROWS rows scanned), and will be set to debounce_delay again each time a button-state was
changed. Until the counter is 0, all button changes will be ignored. 
Debounce-mode 2 uses individual counters for each button, which consumes a bit more memory and 
performance.


Configuration
-------------
The module can be configured by overriding defines. The values shown here are default-values:

// number of rows for button/LED matrix (max. 8)
#define BLM_X_NUM_ROWS 4

// number of cols for DIN matrix.
// this value affects the number of DIN serial-registers used
#define BLM_X_BTN_NUM_COLS 4

// number of cols for DOUT matrix.
// this value affects the number of DOUT serial-registers used
#define BLM_X_LED_NUM_COLS 4

// number of colors / different LEDS used at one matrix-crosspoint.
// this value affects the number of DOUT serial-registers used
#define BLM_X_LED_NUM_COLORS 3

// DOUT shift register to which the cathodes of the LEDs are connected (row selectors).
// if less than 5 rows are defined, the higher nibble of the SR outputs will be always 
// identical to the lower nibble.
#define BLM_X_ROWSEL_DOUT_SR	0

// first DOUT shift register to which the anodes of the LEDs are connected. the number
// of registers used is ceil(BLM_X_LED_NUM_COLS*BLM_X_LED_NUM_COLORS / 8), subsequent
// registers will be used
#define BLM_X_LED_FIRST_DOUT_SR	1


// first DIN shift registers to which the button matrix is connected.
// subsequent shift registers will be used, if more than 8 cols are defined.
#define BLM_X_BTN_FIRST_DIN_SR	0


Module Functions
----------------

s32 BLM_X_Init(void);
Initializes the module. Returns 0 on success, -1 on error (e.g. bad configuration)


s32 BLM_X_PrepareRow(void);
Prepares the next row in the matrix (sets row-selector DOUT - values and LED DOUT values
for the row). 
This hook should be called in APP_SRIO_ServicePrepare() in your application.


s32 BLM_X_GetRow(void);
Reads the the values of the buttons DIN registers for the currently selected row, and
writes it to the internal button-state registers. 
This hook should be called in APP_SRIO_ServiceFinish() in your application.


s32 BLM_X_BtnHandler(void *notify_hook)
This hook checks the internal button-state registers for changes and calls 
notify_hook(u32 btn, u32 value) for each changed button. 
This hook should be called regulary by your application, e.g. in a separate task.


s32 BLM_X_BtnGet(u32 btn);
Returns the state of a button (1 = open / 0 = closed), or -1 if the button is
not available.


u8 BLM_X_BtnSRGet(u8 row, u8 sr);
Gets the virtual button-serial-register value.


s32 BLM_X_LEDSet(u32 led, u32 color, u32 value);
Sets the status of a LED. Sets just a single color/single LED @ matrix-crosspoint
"led". Color is the color offset < BLM_X_LED_NUM_COLORS.


s32 BLM_X_LEDColorSet(u32 led, u32 color_mask);
Sets the status of all colors/LED's of a LED / matrix-crosspoint "led".
color_mask provides the states, the least significant bit represents the color 0.


s32 BLM_X_LEDSRSet(u8 row, u8 sr, u8 sr_value);
Sets a virtual LED-register's value. Returns 0 on success, -1 if the
sr is not available.


extern s32 BLM_X_LEDGet(u32 led, u32 color);
Returns the status of a LED/color (1 if set), -1 if the LED/color is not available.


extern u8 BLM_X_LEDSRGet(u8 row, u8 sr);
Gets a virtual LED-register's value. Returns 0 if the register is not available.


extern s32 BLM_X_DebounceDelaySet(u8 delay);
Sets the debounce-delay (number of scan-cycles to ignore button changes after
a change).


extern u8 BLM_X_DebounceDelayGet(void);
Gets the debounce-delay (number of scan-cycles to ignore button changes after
a change).



Performance
-----------
The whole SRIO service takes ca. 280uS without the BLM-X module hooks, and ca.
300uS with the hooks (tested so far with 4x4x3 matrix and the expensive debounce-mode 2), so
all this is really no matter of performance.

