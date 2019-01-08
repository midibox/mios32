#include "commonIncludes.h"

// --- LEDs ---

const u8 HW_LED_RED_GP1 = 11;
const u8 HW_LED_RED_GP2 = 18;
const u8 HW_LED_RED_GP3 = 25;
const u8 HW_LED_RED_GP4 = 32;
const u8 HW_LED_RED_GP5 = 38;
const u8 HW_LED_RED_GP6 = 45;

const u8 HW_LED_GREEN_GP1 = 10;
const u8 HW_LED_GREEN_GP2 = 17;
const u8 HW_LED_GREEN_GP3 = 24;
const u8 HW_LED_GREEN_GP4 = 30;
const u8 HW_LED_GREEN_GP5 = 37;
const u8 HW_LED_GREEN_GP6 = 44;

const u8 HW_LED_BLUE_GP1 = 9;
const u8 HW_LED_BLUE_GP2 = 16;
const u8 HW_LED_BLUE_GP3 = 22;
const u8 HW_LED_BLUE_GP4 = 29;
const u8 HW_LED_BLUE_GP5 = 36;
const u8 HW_LED_BLUE_GP6 = 43;

const u8 HW_LED_RED_RUNSTOP = 8;
const u8 HW_LED_RED_ARM = 14;
const u8 HW_LED_RED_SHIFT = 21;
const u8 HW_LED_RED_MENU = 28;
const u8 HW_LED_RED_COPY = 35;
const u8 HW_LED_RED_PASTE = 42;
const u8 HW_LED_RED_DELETE = 51;

const u8 HW_LED_GREEN_RUNSTOP = 127;
const u8 HW_LED_GREEN_ARM = 13;
const u8 HW_LED_GREEN_SHIFT = 20;
const u8 HW_LED_GREEN_MENU = 27;
const u8 HW_LED_GREEN_COPY = 34;
const u8 HW_LED_GREEN_PASTE = 41;
const u8 HW_LED_GREEN_DELETE = 52;

const u8 HW_LED_BLUE_RUNSTOP = 4;
const u8 HW_LED_BLUE_ARM = 12;
const u8 HW_LED_BLUE_SHIFT = 19;
const u8 HW_LED_BLUE_MENU = 26;
const u8 HW_LED_BLUE_COPY = 33;
const u8 HW_LED_BLUE_PASTE = 40;
const u8 HW_LED_BLUE_DELETE = 46;

// LOGICAL LED STATES
const u8 LED_OFF = 0;
const u8 LED_RED = 1;
const u8 LED_GREEN = 2;
const u8 LED_BLUE = 4;

const u8 led_scene1 = 15;
const u8 led_scene2 = 7;
const u8 led_scene3 = 6;
const u8 led_scene4 = 5;
const u8 led_scene5 = 2;
const u8 led_scene6 = 1;

const u8 led_page_main = 39;
const u8 led_page_1 = 47;
const u8 led_page_2 = 48;
const u8 led_page_3 = 49;
const u8 led_page_4 = 50;
const u8 led_page_5 = 53;
const u8 led_page_6 = 54;



const u8 led_scene_up = 3;
const u8 led_scene_down = 0;

const u8 led_copy = 34;
const u8 led_paste = 41;

const u8 led_beat0 = 3;
const u8 led_beat1 = 127;
const u8 led_beat2 = 127;
const u8 led_beat3 = 127;


// --- Switches ---

const u8 sw_runstop = 15;
const u8 sw_armrecord = 14;
const u8 sw_encoder2 = 4;

const u8 sw_gp1 = 12;
const u8 sw_gp2 = 7;
const u8 sw_gp3 = 6;
const u8 sw_gp4 = 30;
const u8 sw_gp5 = 29;
const u8 sw_gp6 = 28;

const u8 sw_menu = 23;
const u8 sw_shift = 13;
const u8 sw_copy = 22;
const u8 sw_paste = 21;
const u8 sw_delete = 20;


// -- Encoders ---

const u8 enc_scene_id = 0;
const u8 enc_scene = 4;

const u8 enc_track_id = 1;
const u8 enc_track = 10;

const u8 enc_page_id = 2;
const u8 enc_page = 24;

const u8 enc_data_id = 3;
const u8 enc_data = 18;


/* LED mapping
 * -----------
 *
 * dout  0 = scene switch mode: clip
 * dout  1 = scene 6
 * dout  2 = scene 5
 * dout  3 = run/stop green
 * dout  4 = run/stop blue
 * dout  5 = scene 4
 * dout  6 = scene 3
 * dout  7 = scene 2
 * dout  8 = run/stop red
 * dout  9 = gp1 blue
 * dout 10 = gp1 green
 * dout 11 = gp1 red
 * dout 12 = arm blue
 * dout 13 = arm green
 * dout 14 = arm red
 * dout 15 = scene 1
 * dout 16 = gp2 blue
 * dout 17 = gp2 green
 * dout 18 = gp2 red
 * dout 19 = shift blue
 * dout 20 = shift green
 * dout 21 = shift red
 * dout 22 = gp3 blue
 * dout 23 = scene switch mode: all
 * dout 24 = gp3 green
 * dout 25 = gp3 red
 * dout 26 = menu blue
 * dout 27 = menu green
 * dout 28 = menu red
 * dout 29 = gp4 blue
 * dout 30 = gp4 green
 * dout 31 = reserved
 * dout 32 = gp4 red
 * dout 33 = copy blue
 * dout 34 = copy green
 * dout 35 = copy red
 * dout 36 = gp5 blue
 * dout 37 = gp5 green
 * dout 38 = gp5 red
 * dout 39 = menu mode: edit
 * dout 40 = paste blue
 * dout 41 = paste green
 * dout 42 = paste red
 * dout 43 = gp6 blue
 * dout 44 = gp6 green
 * dout 45 = gp6 red
 * dout 46 = delete blue
 * dout 47 = page 1
 * dout 48 = page 2
 * dout 49 = page 3
 * dout 50 = page 4
 * dout 51 = delete red
 * dout 52 = delete green
 * dout 53 = page 5
 * dout 54 = page 6
 * dout 55 = menu mode: session
 *
 * BUTTON/ENCODER MAPPING
 * ----------------------
 *
 * gp1      = din 12
 * gp2      = din  7
 * gp3      = din  6
 * gp4      = din 30
 * gp5      = din 29
 * gp6      = din 28
 * run/stop = din 15
 * arm      = din 14
 * shift    = din 13
 * menu     = din 23
 * copy     = din 22
 * paste    = din 21
 * delete   = din 20
 *
 * scene encoder = din 4/5
 * scene button  = din 3
 * track/select encoder = din 10/11
 * track/select button = din 2
 * page encoder  = din 24/25
 * page button = din 26
 * data encoder = din 18/19
 * data button = din 16
 */