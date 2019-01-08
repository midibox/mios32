// --- LEDs ---

#include "loopa_datatypes.h"

// PHYSICAL HARDWARE LEDs (three colors per LED)
extern const u8 HW_LED_RED_GP1;
extern const u8 HW_LED_RED_GP2;
extern const u8 HW_LED_RED_GP3;
extern const u8 HW_LED_RED_GP4;
extern const u8 HW_LED_RED_GP5;
extern const u8 HW_LED_RED_GP6;

extern const u8 HW_LED_GREEN_GP1;
extern const u8 HW_LED_GREEN_GP2;
extern const u8 HW_LED_GREEN_GP3;
extern const u8 HW_LED_GREEN_GP4;
extern const u8 HW_LED_GREEN_GP5;
extern const u8 HW_LED_GREEN_GP6;

extern const u8 HW_LED_BLUE_GP1;
extern const u8 HW_LED_BLUE_GP2;
extern const u8 HW_LED_BLUE_GP3;
extern const u8 HW_LED_BLUE_GP4;
extern const u8 HW_LED_BLUE_GP5;
extern const u8 HW_LED_BLUE_GP6;

extern const u8 HW_LED_RED_RUNSTOP;
extern const u8 HW_LED_RED_ARM;
extern const u8 HW_LED_RED_SHIFT;
extern const u8 HW_LED_RED_MENU;
extern const u8 HW_LED_RED_COPY;
extern const u8 HW_LED_RED_PASTE;
extern const u8 HW_LED_RED_DELETE;

extern const u8 HW_LED_GREEN_RUNSTOP;
extern const u8 HW_LED_GREEN_ARM;
extern const u8 HW_LED_GREEN_SHIFT;
extern const u8 HW_LED_GREEN_MENU;
extern const u8 HW_LED_GREEN_COPY;
extern const u8 HW_LED_GREEN_PASTE;
extern const u8 HW_LED_GREEN_DELETE;

extern const u8 HW_LED_BLUE_RUNSTOP;
extern const u8 HW_LED_BLUE_ARM;
extern const u8 HW_LED_BLUE_SHIFT;
extern const u8 HW_LED_BLUE_MENU;
extern const u8 HW_LED_BLUE_COPY;
extern const u8 HW_LED_BLUE_PASTE;
extern const u8 HW_LED_BLUE_DELETE;

// LOGICAL LED STATES
extern const u8 LED_OFF;
extern const u8 LED_RED;
extern const u8 LED_GREEN;
extern const u8 LED_BLUE;

// LOGICAL LEDs (can be set to multiple colors)
enum MatiasLEDs
{
   LED_GP1, LED_GP2, LED_GP3, LED_GP4, LED_GP5, LED_GP6,
   LED_RUNSTOP, LED_ARM, LED_SHIFT, LED_MENU,
   LED_COPY, LED_PASTE, LED_DELETE
};

extern const u8 led_scene1;
extern const u8 led_scene2;
extern const u8 led_scene3;
extern const u8 led_scene4;
extern const u8 led_scene5;
extern const u8 led_scene6;

extern const u8 led_page_main;
extern const u8 led_page_1;
extern const u8 led_page_2;
extern const u8 led_page_3;
extern const u8 led_page_4;
extern const u8 led_page_5;
extern const u8 led_page_6;


extern const u8 led_scene_up;
extern const u8 led_scene_down;

extern const u8 led_copy;
extern const u8 led_paste;

extern const u8 led_beat0;
extern const u8 led_beat1;
extern const u8 led_beat2;
extern const u8 led_beat3;


// --- Switches ---

extern const u8 sw_runstop;
extern const u8 sw_armrecord;
extern const u8 sw_encoder2;

extern const u8 sw_gp1;
extern const u8 sw_gp2;
extern const u8 sw_gp3;
extern const u8 sw_gp4;
extern const u8 sw_gp5;
extern const u8 sw_gp6;

extern const u8 sw_menu;
extern const u8 sw_shift;
extern const u8 sw_copy;
extern const u8 sw_paste;
extern const u8 sw_delete;


// -- Encoders ---

extern const u8 enc_scene_id;
extern const u8 enc_scene;

extern const u8 enc_track_id;
extern const u8 enc_track;

extern const u8 enc_page_id;
extern const u8 enc_page;

extern const u8 enc_data_id;
extern const u8 enc_data;

