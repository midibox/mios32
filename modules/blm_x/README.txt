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
connected to the core and MIOS32_SRIO_NUM_SR configuration.
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

In color-mode 0 ,the colors will be mapped to the registers grouped by colors (standard mode):
  [LED-col 0, color 0][LED-col 1, color 0][LED-col 2, color 0]....[LED-col 0, color1][LED-col 1, color1]...
In color-mode 1 ,the colors will be mapped to the registers grouped by LED-column:
  [LED-col 0, color 0][LED-col 0, color 1][LED-col 0, color 2]....[LED-col 1, color0][LED-col 1, color1]...
Choose the mode that fits your plans to wire the LED-matrix on the hardware side.

If you want to work directly on the virtual SR's (bypass LED-set / -get functions), you may prefer to 
choose BLM_X_LED_NUM_COLS values like 4,8,12,16 etc. and color-mode 0. If you set LED states only by 
module functions, you don't have to care about this on the software side.

Note that serial register assignment / color mode can be re-configured by software ( BLM_X_ConfigSet(..) ).


Debouncing
----------
Debounce-modes 1 and 2 are available: mode 1 uses a single counter, which decrements each scan-cycle
(BLM_X_NUM_ROWS rows scanned), and will be set to debounce_delay again each time a button-state was
changed. Until the counter is 0, all button changes will be ignored. 
Debounce-mode 2 uses individual counters for each button, which consumes a bit more memory and 
performance. 
You can disable debouncing completly by setting BLM_X_DEBOUNCE_MODE = 0

Debounce-delay can be configured by software ( BLM_X_ConfigSet(..) ), and will be set to 0 by default.

Configuration
-------------
The module can be configured by overriding defines. The values shown here are default-values:

// Number of rows for button/LED matrix (max. 8)
#define BLM_X_NUM_ROWS 4

// Number of cols for DIN matrix.
// This value affects the number of DIN serial-registers used.
#define BLM_X_BTN_NUM_COLS 4

// Number of cols for DOUT matrix.
// This value affects the number of DOUT serial-registers used.
#define BLM_X_LED_NUM_COLS 4

// Number of colors / different LEDS used at one matrix-crosspoint.
// This value affects the number of DOUT serial-registers used.
#define BLM_X_LED_NUM_COLORS 3

// DOUT shift register to which the cathodes of the LEDs are connected (row selectors).
// If less than 5 rows are defined, the higher nibble of the SR outputs will be always 
// identical to the lower nibble. Note that SR's are counted from 1.
//
// This option can be re-configured by software ( BLM_X_ConfigSet(..) )
#define BLM_X_ROWSEL_DOUT_SR  1

// First DOUT shift register to which the anodes of the LEDs are connected. The number
// of registers used is: ceil(BLM_X_LED_NUM_COLS*BLM_X_LED_NUM_COLORS / 8), subsequent
// registers will be used.
// SR's are counted from 1. Value 0 disables LED handling (only use button matrix). Note
// that if you disable LED's, you can't reconfigure this by software.
//
// This option can be re-configured by software ( BLM_X_ConfigSet(..) )
#define BLM_X_LED_FIRST_DOUT_SR  2


// First DIN shift registers to which the button matrix is connected.
// Subsequent shift registers will be used, if more than 8 cols are defined.
// SR's are counted from 1. Value 0 disables button handling (only use LED matrix). Note
// that if you disable buttons, you can't reconfigure this by software.
//
// This option can be re-configured by software ( BLM_X_ConfigSet(..) )
#define BLM_X_BTN_FIRST_DIN_SR  1

// Set an inversion mask for the row selection shift registers if sink drivers (transistors)
// have been added to the cathode lines. 
// Note: with no sink drivers connected, the LED's brightnes may be affected by the number
// of active LED's in the same row.
// Settings: 0x00 - no sink drivers
//           0xff - sink drivers connected to D7..D0
//           0x0f - sink drivers connected to D3..D0
//           0xf0 - sink drivers connected to D7..D4
//
// This option can be re-configured by software ( BLM_X_ConfigSet(..) )
#define BLM_X_ROWSEL_INV_MASK  0x00


// 0: no debouncing
// 1: cheap debouncing (all buttons the same time)
// 2: individual debouncing of all buttons
#define BLM_X_DEBOUNCE_MODE 0

// 0: colors will be mapped to serial registers grouped by color (see section "Serial registers")
// 1: colors will be mapped to serial registers grouped by LED-columns (see section "Serial registers")
//
// This option can be re-configured by software ( BLM_X_ConfigSet(..) )
BLM_X_COLOR_MODE 0


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
SR is not available.


s32 BLM_X_LEDGet(u32 led, u32 color);
Returns the status of a LED/color (1 if set), -1 if the LED/color is not available.


32 BLM_X_LEDColorGet(u32 led);
Returns the color-mask of a LED. Bit 0 (LSB) represents color 0, Bit 1 color 1 etc.
The function will return 0 if the LED is not available.


u8 BLM_X_LEDSRGet(u8 row, u8 sr);
Gets a virtual LED-register's value. Returns 0 if the register is not available.


s32 BLM_X_ConfigSet(blm_x_config_t config);
Sets the blm_x configuration structure, it has the following members:
  .cfg.rowsel_dout_sr; //see description of BLM_X_ROWSEL_DOUT_SR define. default = BLM_X_ROWSEL_DOUT_SR
  .cfg.led_first_dout_sr; //see description of BLM_X_LED_FIRST_DOUT_SR define. default = BLM_X_LED_FIRST_DOUT_SR
  .cfg.btn_first_din_sr; //see description of BLM_X_BTN_FIRST_DIN_SR define. default = BLM_X_BTN_FIRST_DIN_SR
  .cfg.rowsel_inv_mask; //see description of BLM_X_ROWSEL_INV_MASK define. default = BLM_X_ROWSEL_INV_MASK
  .cfg.color_mode; //see description of BLM_X_COLOR_MODE define. default = BLM_X_COLOR_MODE
  .cfg.debounce_delay; //number of scan cycles to ignore button changes after change. default = 0


blm_x_config_t BLM_X_ConfigGet(void);
Gets the blm_x configuration structure. See also BLM_X_ConfigSet for description of struct members.


Performance
-----------
The whole SRIO service takes ca. 
- ca. 280uS without the BLM-X module hooks
- ca.290uS with 4 rows, 4 cols, 3 colors, debounce mode 2 (LEDs & buttons)
- ca. 320uS with 8 rows, 16 cols, 3 colors, debounce mode 2 (LEDs & buttons)

Most of this additional time is consumed by heavy debouncing (mode 2). Without any
debouncing (not required in most applications, 4 rows == 4mS scan cycle), this
additional time will be dramatically lower.

