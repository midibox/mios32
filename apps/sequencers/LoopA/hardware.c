// LoopA Hardware Mapping

#include "commonIncludes.h"
#include "screen.h"

// --- LEDs ---

// physical hardware LEDs (three colors per LED)
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

const u8 HW_LED_GREEN_RUNSTOP = 3;
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

const u8 HW_LED_SCENE_SWITCH_ALL = 23;
const u8 HW_LED_SCENE_1 = 15;
const u8 HW_LED_SCENE_2 = 7;
const u8 HW_LED_SCENE_3 = 6;
const u8 HW_LED_SCENE_4 = 5;
const u8 HW_LED_SCENE_5 = 2;
const u8 HW_LED_SCENE_6 = 1;
const u8 HW_LED_SCENE_SWITCH_CLIP = 0;

const u8 HW_LED_LIVEMODE_TRANSPOSE = 39;
const u8 HW_LED_LIVEMODE_1 = 47;
const u8 HW_LED_LIVEMODE_2 = 48;
const u8 HW_LED_LIVEMODE_3 = 49;
const u8 HW_LED_LIVEMODE_4 = 50;
const u8 HW_LED_LIVEMODE_5 = 53;
const u8 HW_LED_LIVEMODE_6 = 54;
const u8 HW_LED_LIVEMODE_BEATLOOP = 55;

// --- Switches ---

const u8 sw_gp1 = 12;
const u8 sw_gp2 = 7;
const u8 sw_gp3 = 6;
const u8 sw_gp4 = 30;
const u8 sw_gp5 = 29;
const u8 sw_gp6 = 28;

const u8 sw_runstop = 15;
const u8 sw_armrecord = 14;
const u8 sw_menu = 23;
const u8 sw_shift = 13;
const u8 sw_copy = 22;
const u8 sw_paste = 21;
const u8 sw_delete = 20;

const u8 sw_enc_scene = 3;
const u8 sw_enc_select = 2;
const u8 sw_enc_live = 26;
const u8 sw_enc_value = 16;

// --- Encoders ----

const u8 enc_scene_id = 0;
const u8 enc_scene = 4;

const u8 enc_select_id = 1;
const u8 enc_select = 10;

const u8 enc_live_id = 2;
const u8 enc_live = 24;

const u8 enc_value_id = 3;
const u8 enc_value = 18;


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
 * dout 39 = live mode: transpose
 * dout 40 = paste blue
 * dout 41 = paste green
 * dout 42 = paste red
 * dout 43 = gp6 blue
 * dout 44 = gp6 green
 * dout 45 = gp6 red
 * dout 46 = delete blue
 * dout 47 = live mode 1
 * dout 48 = live mode 2
 * dout 49 = live mode 3
 * dout 50 = live mode 4
 * dout 51 = delete red
 * dout 52 = delete green
 * dout 53 = live mode 5
 * dout 54 = live mode 6
 * dout 55 = live mode: beatloop
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

static u16 testmodeCounter_;
static u8 testmodeLEDFlashCounter_ = 0;


/**
 * Hardware test mode switch/encoder interaction: flash all LEDs for a short amount of time
 *
 */
void testmodeFlashAllLEDs()
{
   testmodeLEDFlashCounter_ = 55; // Flash, don't turn off LEDs for this many cycles
}
// -------------------------------------------------------------------------------------------------


/**
 * Hardware test mode: cycle all LEDs
 *
 */
void hardwareTestmodeIterateLEDs()
{
   // Enable frontside LEDs for testing
   MIOS32_BOARD_LED_Set(0x000F, ~MIOS32_BOARD_LED_Get());

   testmodeCounter_++;

   u16 cycle;

   if (testmodeLEDFlashCounter_)
      cycle = testmodeCounter_; // fast flash
   else
      cycle = testmodeCounter_ / 64; // slower progression

   switch (cycle % 55)
   {
      case 0:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_BEATLOOP, 0);
         MIOS32_DOUT_PinSet(HW_LED_RED_GP1, 1);
         break;
      case 1:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_RED_GP1, 0);
         MIOS32_DOUT_PinSet(HW_LED_RED_GP2, 1);
         break;
      case 2:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_RED_GP2, 0);
         MIOS32_DOUT_PinSet(HW_LED_RED_GP3, 1);
         break;
      case 3:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_RED_GP3, 0);
         MIOS32_DOUT_PinSet(HW_LED_RED_GP4, 1);
         break;
      case 4:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_RED_GP4, 0);
         MIOS32_DOUT_PinSet(HW_LED_RED_GP5, 1);
         break;
      case 5:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_RED_GP5, 0);
         MIOS32_DOUT_PinSet(HW_LED_RED_GP6, 1);
         break;
      case 6:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_RED_GP6, 0);
         MIOS32_DOUT_PinSet(HW_LED_GREEN_GP1, 1);
         break;
      case 7:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_GREEN_GP1, 0);
         MIOS32_DOUT_PinSet(HW_LED_GREEN_GP2, 1);
         break;
      case 8:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_GREEN_GP2, 0);
         MIOS32_DOUT_PinSet(HW_LED_GREEN_GP3, 1);
         break;
      case 9:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_GREEN_GP3, 0);
         MIOS32_DOUT_PinSet(HW_LED_GREEN_GP4, 1);
         break;
      case 10:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_GREEN_GP4, 0);
         MIOS32_DOUT_PinSet(HW_LED_GREEN_GP5, 1);
         break;
      case 11:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_GREEN_GP5, 0);
         MIOS32_DOUT_PinSet(HW_LED_GREEN_GP6, 1);
         break;
      case 12:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_GREEN_GP6, 0);
         MIOS32_DOUT_PinSet(HW_LED_BLUE_GP1, 1);
         break;
      case 13:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_BLUE_GP1, 0);
         MIOS32_DOUT_PinSet(HW_LED_BLUE_GP2, 1);
         break;
      case 14:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_BLUE_GP2, 0);
         MIOS32_DOUT_PinSet(HW_LED_BLUE_GP3, 1);
         break;
      case 15:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_BLUE_GP3, 0);
         MIOS32_DOUT_PinSet(HW_LED_BLUE_GP4, 1);
         break;
      case 16:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_BLUE_GP4, 0);
         MIOS32_DOUT_PinSet(HW_LED_BLUE_GP5, 1);
         break;
      case 17:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_BLUE_GP5, 0);
         MIOS32_DOUT_PinSet(HW_LED_BLUE_GP6, 1);
         break;
      case 18:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_BLUE_GP6, 0);
         MIOS32_DOUT_PinSet(HW_LED_RED_RUNSTOP, 1);
         break;
      case 19:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_RED_RUNSTOP, 0);
         MIOS32_DOUT_PinSet(HW_LED_RED_ARM, 1);
         break;
      case 20:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_RED_ARM, 0);
         MIOS32_DOUT_PinSet(HW_LED_RED_SHIFT, 1);
         break;
      case 21:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_RED_SHIFT, 0);
         MIOS32_DOUT_PinSet(HW_LED_RED_MENU, 1);
         break;
      case 22:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_RED_MENU, 0);
         MIOS32_DOUT_PinSet(HW_LED_RED_COPY, 1);
         break;
      case 23:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_RED_COPY, 0);
         MIOS32_DOUT_PinSet(HW_LED_RED_PASTE, 1);
         break;
      case 24:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_RED_PASTE, 0);
         MIOS32_DOUT_PinSet(HW_LED_RED_DELETE, 1);
         break;
      case 25:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_RED_DELETE, 0);
         MIOS32_DOUT_PinSet(HW_LED_GREEN_RUNSTOP, 1);
         break;
      case 26:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_GREEN_RUNSTOP, 0);
         MIOS32_DOUT_PinSet(HW_LED_GREEN_ARM, 1);
         break;
      case 27:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_GREEN_ARM, 0);
         MIOS32_DOUT_PinSet(HW_LED_GREEN_SHIFT, 1);
         break;
      case 28:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_GREEN_SHIFT, 0);
         MIOS32_DOUT_PinSet(HW_LED_GREEN_MENU, 1);
         break;
      case 29:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_GREEN_MENU, 0);
         MIOS32_DOUT_PinSet(HW_LED_GREEN_COPY, 1);
         break;
      case 30:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_GREEN_COPY, 0);
         MIOS32_DOUT_PinSet(HW_LED_GREEN_PASTE, 1);
         break;
      case 31:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_GREEN_PASTE, 0);
         MIOS32_DOUT_PinSet(HW_LED_GREEN_DELETE, 1);
         break;
      case 32:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_GREEN_DELETE, 0);
         MIOS32_DOUT_PinSet(HW_LED_BLUE_RUNSTOP, 1);
         break;
      case 33:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_BLUE_RUNSTOP, 0);
         MIOS32_DOUT_PinSet(HW_LED_BLUE_ARM, 1);
         break;
      case 34:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_BLUE_ARM, 0);
         MIOS32_DOUT_PinSet(HW_LED_BLUE_SHIFT, 1);
         break;
      case 35:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_BLUE_SHIFT, 0);
         MIOS32_DOUT_PinSet(HW_LED_BLUE_MENU, 1);
         break;
      case 36:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_BLUE_MENU, 0);
         MIOS32_DOUT_PinSet(HW_LED_BLUE_COPY, 1);
         break;
      case 37:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_BLUE_COPY, 0);
         MIOS32_DOUT_PinSet(HW_LED_BLUE_PASTE, 1);
         break;
      case 38:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_BLUE_PASTE, 0);
         MIOS32_DOUT_PinSet(HW_LED_BLUE_DELETE, 1);
         break;
      case 39:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_BLUE_DELETE, 0);
         MIOS32_DOUT_PinSet(HW_LED_SCENE_SWITCH_ALL, 1);
         break;
      case 40:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_SCENE_SWITCH_ALL, 0);
         MIOS32_DOUT_PinSet(HW_LED_SCENE_1, 1);
         break;
      case 41:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_SCENE_1, 0);
         MIOS32_DOUT_PinSet(HW_LED_SCENE_2, 1);
         break;
      case 42:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_SCENE_2, 0);
         MIOS32_DOUT_PinSet(HW_LED_SCENE_3, 1);
         break;
      case 43:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_SCENE_3, 0);
         MIOS32_DOUT_PinSet(HW_LED_SCENE_4, 1);
         break;
      case 44:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_SCENE_4, 0);
         MIOS32_DOUT_PinSet(HW_LED_SCENE_5, 1);
         break;
      case 45:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_SCENE_5, 0);
         MIOS32_DOUT_PinSet(HW_LED_SCENE_6, 1);
         break;
      case 46:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_SCENE_6, 0);
         MIOS32_DOUT_PinSet(HW_LED_SCENE_SWITCH_CLIP, 1);
         break;
      case 47:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_SCENE_SWITCH_CLIP, 0);
         MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_TRANSPOSE, 1);
         break;
      case 48:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_TRANSPOSE, 0);
         MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_1, 1);
         break;
      case 49:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_1, 0);
         MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_2, 1);
         break;
      case 50:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_2, 0);
         MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_3, 1);
         break;
      case 51:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_3, 0);
         MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_4, 1);
         break;
      case 52:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_4, 0);
         MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_5, 1);
         break;
      case 53:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_5, 0);
         MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_6, 1);
         break;
      case 54:
         if (!testmodeLEDFlashCounter_)
            MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_6, 0);
         MIOS32_DOUT_PinSet(HW_LED_LIVEMODE_BEATLOOP, 1);
         break;
   }
   if (testmodeLEDFlashCounter_)
      testmodeLEDFlashCounter_--;
}
// -------------------------------------------------------------------------------------------------
