// LoopA Hardware Mapping

#include "loopa_datatypes.h"

// --- LEDs ---

// logical LED states
enum LEDStateEnum
{
   LED_OFF = 0,
   LED_RED = 1,
   LED_GREEN = 2,
   LED_BLUE = 4
};

// physical hardware LEDs (three colors per LED)
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

const u8 HW_LED_SCENE_SWITCH_ALL;
extern const u8 HW_LED_SCENE_1;
extern const u8 HW_LED_SCENE_2;
extern const u8 HW_LED_SCENE_3;
extern const u8 HW_LED_SCENE_4;
extern const u8 HW_LED_SCENE_5;
extern const u8 HW_LED_SCENE_6;
const u8 HW_LED_SCENE_SWITCH_CLIP;

extern const u8 HW_LED_LIVEMODE_TRANSPOSE;
extern const u8 HW_LED_LIVEMODE_1;
extern const u8 HW_LED_LIVEMODE_2;
extern const u8 HW_LED_LIVEMODE_3;
extern const u8 HW_LED_LIVEMODE_4;
extern const u8 HW_LED_LIVEMODE_5;
extern const u8 HW_LED_LIVEMODE_6;
extern const u8 HW_LED_LIVEMODE_BEATLOOP;

// logical matias superflux LEDs (can be set to multiple colors)
enum MatiasLEDs
{
   LED_GP1, LED_GP2, LED_GP3, LED_GP4, LED_GP5, LED_GP6,
   LED_RUNSTOP, LED_ARM, LED_SHIFT, LED_MENU, LED_COPY, LED_PASTE, LED_DELETE
};

// --- Switches ---

extern const u8 sw_gp1;
extern const u8 sw_gp2;
extern const u8 sw_gp3;
extern const u8 sw_gp4;
extern const u8 sw_gp5;
extern const u8 sw_gp6;

extern const u8 sw_runstop;
extern const u8 sw_armrecord;
extern const u8 sw_menu;
extern const u8 sw_shift;
extern const u8 sw_copy;
extern const u8 sw_paste;
extern const u8 sw_delete;

extern const u8 sw_enc_scene;
extern const u8 sw_enc_select;
extern const u8 sw_enc_live;
extern const u8 sw_enc_value;

extern const u8 sw_footswitch1;
extern const u8 sw_footswitch2;

// --- Encoders ---

extern const u8 enc_scene_id;
extern const u8 enc_scene;

extern const u8 enc_select_id;
extern const u8 enc_select;

extern const u8 enc_live_id;
extern const u8 enc_live;

extern const u8 enc_value_id;
extern const u8 enc_value;

// --- Functions (hardware testmode) ---

extern void testmodeFlashAllLEDs();
extern void hardwareTestmodeIterateLEDs();