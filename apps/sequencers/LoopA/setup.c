#include <mios32.h>// LoopA Setup/Config routines

#include "commonIncludes.h"

#include "loopa.h"
#include "setup.h"
#include "ui.h"

// --- Globals ---
u8 configChangesToBeWritten_ = 0;
char line_buffer_[128];  // single global line buffer for reading/writing from/to files

// --- Global config variables ---
s16 gcLastUsedSessionNumber_ = -1;
s8 gcNumberOfActiveUserInstruments_ = 0;

s8 gcFontType_ = 'a';
s8 gcInvertOLED_ = 0;
s8 gcBeatLEDsEnabled_ = 0;
s8 gcBeatDisplayEnabled_ = 0;
u8 gcScreensaverAfterMinutes_ = 1;
mios32_midi_port_t gcMetronomePort_ = 0;
u8 gcMetronomeChannel_ = 0;
u8 gcMetronomeNoteM_ = 0x25; // C#1
u8 gcMetronomeNoteB_ = 0x25; // C#1
enum TempodeltaTypeEnum gcTempodeltaType_ = TEMPODELTA_NORMAL;
u8 gcFootswitchesInversionmask_ = 0x00; // binary encoded: fsw1 = rightmost bit, fsw2 = second bit, ...
enum FootswitchActionEnum gcFootswitch1Action_ = FOOTSWITCH_CURSORERASE;
enum FootswitchActionEnum gcFootswitch2Action_ = FOOTSWITCH_JUMPTOPRECOUNT;
s8 gcInvertMuteLEDs_ = 0;
enum TrackswitchTypeEnum gcTrackswitchType_ = TRACKSWITCH_NORMAL;
enum FollowtrackTypeEnum gcFollowtrackType_ = FOLLOWTRACK_ON_UNMUTE;
s8 gcLEDNotes_ = 1;

// --- Global config settings ---
SetupParameter setupParameters_[SETUP_NUM_ITEMS] =
        {
                { "System Font", "Type", "", "", "" },
                { "Beat LEDs", "Toggle", "", "", "" },
                { "Beat Display", "Toggle", "", "", "" },
                { "Screensaver", "Minutes", "", "", "" },
                { "Invert OLED", "Toggle", "", "", "" },
                { "Metronome", "Port", "Chn", "Meas.", "Beat" },
                { "Tempo Up/Dn", "Type", "", "", "" },
                { "Inv. Footsw.", "Toggle",  "Toggle", "", "" },
                { "Footsw. 1", "Action", "", "", "" },
                { "Footsw. 2", "Action", "", "", "" },
                { "Inv. MuteLED", "Toggle", "", "", "" },
                { "Track Switch", "Type", "", "", "" },
                { "Follow Track", "Type", "", "", "" },
                { "LED Notes", "Toggle", "", "", "" },

                { "MCLK DIN IN",  "Toggle",  "Toggle",  "Toggle", "Toggle" },
                { "MCLK DIN OUT", "Toggle", "Toggle", "Toggle", "Toggle" },
                { "MCLK USB IN",  "Toggle", "Toggle", "Toggle", "Toggle" },
                { "MCLK USB OUT", "Toggle", "Toggle", "Toggle", "Toggle" },
        };


// --- User defined instrument names ---
UserInstrument userInstruments_[SETUP_NUM_USERINSTRUMENTS] =
        {
                { "Synth_A", UART0, 0},
                { "Synth_B", UART0, 0},
                { "Synth_C", UART0, 0},
                { "Synth_D", UART0, 0},
                { "Synth_E", UART0, 0},
                { "Synth_F", UART0, 0},
                { "Synth_G", UART0, 0},
                { "Synth_H", UART0, 0},
                { "Synth_I", UART0, 0},
                { "Synth_J", UART0, 0},
                { "Synth_K", UART0, 0},
                { "Synth_L", UART0, 0},
                { "Synth_M", UART0, 0},
                { "Synth_N", UART0, 0},
                { "Synth_O", UART0, 0},
                { "Synth_P", UART0, 0},
                { "Synth_Q", UART0, 0},
                { "Synth_R", UART0, 0},
                { "Synth_S", UART0, 0},
                { "Synth_T", UART0, 0},
                { "Synth_U", UART0, 0},
                { "Synth_V", UART0, 0},
                { "Synth_W", UART0, 0},
                { "Synth_X", UART0, 0},
                { "Synth_Y", UART0, 0},
                { "Synth_Z", UART0, 0},
                { "Synth_AA", UART0, 0},
                { "Synth_AB", UART0, 0},
                { "Synth_AC", UART0, 0},
                { "Synth_AD", UART0, 0},
                { "Synth_AE", UART0, 0},
                { "Synth_AF", UART0, 0},
        };

// --- Tempodelta types ---
TempodeltaDescription tempodeltaDescriptions_[SETUP_NUM_TEMPODELTATYPES] =
        {
                { "Slower", "very slow tempo change", 0.00625 },
                { "Slow", "slow tempo change", 0.025 },
                { "Normal", "normal tempo change", 0.1 },
                { "Fast", "fast tempo change", 0.4 },
                { "Faster", "very fast tempo change", 1.6 },
        };


// --- Footswitch actions ---
FootswitchActionDescription footswitchActionDescriptions_[SETUP_NUM_FOOTSWITCHACTIONS] =
        {
                { "CursorErase", "Erase under cursor" },
                { "RunStop", "RunStop the sequencer" },
                { "Arm", "Arm (toggle)" },
                { "ClearClip", "Clear the current Clip" },
                { "JumpToStart", "Jump to Start" },
                { "JumpToPrecount", "Jump to Precount" },
                { "Metronome", "Metronome (toggle)" },
                { "PreviousScene", "Previous Scene" },
                { "NextScene", "Next Scene" },
                { "PreviousTrack", "Previous Track" },
                { "NextTrack", "Next Track" },
        };

// --- Trackswitch types ---
TrackswitchDescription trackswitchDescriptions_[SETUP_NUM_TRACKSWITCHTYPES] =
        {
                { "Normal", "via SELECT encoder" },
                { "HoldMuteKeyFast", "hold mute key (0.2s)" },
                { "HoldMuteKey", "hold mute key (0.4s)" },
        };

// --- Followtrack types ---
FollowtrackDescription followtrackDescriptions_[SETUP_NUM_FOLLOWTRACKTYPES] =
        {
                { "Disabled", "disabled" },
                { "FollowOnUnmute", "when unmuting" },
                { "FollowOnMuteOrUnmute", "when muting or unmuting" },
        };

// ----------------------------------------------------------------------------------------

/**
 * Help function which parses a decimal or hex value
 *
 */
static s32 get_dec(const char *word)
{
   if (word == NULL)
      return -1;

   char *next;
   long l = strtol(word, &next, 0);

   if (word == next)
      return -256; // modification for SEQ_FILE_C: return -256 if value invalid, since we read signed bytes

   return l; // value is valid
}
// ----------------------------------------------------------------------------------------


/**
 * Another help function which also returns an error message if range is violated
 *
 */
static s32 get_dec_range(char *word, char *parameter, int min, int max)
{
   s32 value = get_dec(word);
   if (value < -255 || value < min || value > max)
   {
#if DEBUG_VERBOSE_LEVEL >= 1
      DEBUG_MSG("[SEQ_FILE_C] ERROR invalid value '%s' for parameter '%s'\n", word, parameter);
#endif
      return -255; // invalid value or range
   }
   return value;
}
// ----------------------------------------------------------------------------------------


/**
 * Returns the mios32_port_t if the name matches with a MIDI Port (either name or value such as 0x10 for USB1)
 * Returns -1 if no matching port has been found
 *
 */
s32 MIDI_PORT_InPortFromNameGet(const char* name)
{
   int port_ix;

   for(port_ix=0; port_ix<MIDI_PORT_InNumGet(); ++port_ix)
   {
      // terminate port name at first space
      const char *port_name = MIDI_PORT_InNameGet(port_ix);
      const char *search_name = name;

      while (*port_name != 0 && *port_name == *search_name )
      {
         ++port_name;
         ++search_name;

         if (*search_name == 0 && (*port_name == ' ' || *port_name == 0))
            return MIDI_PORT_InPortGet(port_ix);
      }
   }

   s32 value = get_dec(name);
   if (value >= 0 && value <= 0xff)
      return value;

   return -1; // port not found
}
// ----------------------------------------------------------------------------------------


/**
 * Returns the mios32_port_t if the name matches with a MIDI Port (either name or value such as 0x10 for USB1)
 * Returns -1 if no matching port has been found
 *
 */
s32 MIDI_PORT_OutPortFromNameGet(const char* name)
{
   int port_ix;

   for(port_ix=0; port_ix<MIDI_PORT_OutNumGet(); ++port_ix)
   {
      // terminate port name at first space
      const char *port_name = MIDI_PORT_OutNameGet(port_ix);
      const char *search_name = name;

      while (*port_name != 0 && *port_name == *search_name )
      {
         ++port_name;
         ++search_name;

         if (*search_name == 0 && (*port_name == ' ' || *port_name == 0))
            return MIDI_PORT_OutPortGet(port_ix);
      }
   }

   s32 value = get_dec(name);
   if (value >= 0 && value <= 0xff)
      return value;

   return -1; // port not found
}
// ----------------------------------------------------------------------------------------


/**
 * (After a configuration change), write the global setup file to disk
 *
 */
void writeSetup()
{
   configChangesToBeWritten_ = 0;
   MUTEX_SDCARD_TAKE;

   if ((FILE_WriteOpen(CONFIG_FILE_PATH, 1)) < 0)
   {
      FILE_WriteClose(); // important to free memory given by malloc
      MUTEX_SDCARD_GIVE;
      return;
   }

   // write user instruments config
   {
      u8 i;
      for (i = 0; i < SETUP_NUM_USERINSTRUMENTS; i++)
      {
         sprintf(line_buffer_, "INSTRUMENT %d %s %s %d\n",
                 i,
                 userInstruments_[i].name,
                 MIDI_PORT_OutNameGet(MIDI_PORT_InIxGet(userInstruments_[i].port)),
                 userInstruments_[i].channel
         );

         FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));
      }
   }

   // write MIDI router config
   {
      u8 node;
      midi_router_node_entry_t *n = &midi_router_node[0];
      for (node = 0; node < MIDI_ROUTER_NUM_NODES; ++node, ++n)
      {
         sprintf(line_buffer_, "MIDI_RouterNode %d %s %d %s %d\n",
                 node,
                 MIDI_PORT_InNameGet(MIDI_PORT_InIxGet((mios32_midi_port_t) n->src_port)),
                 n->src_chn,
                 MIDI_PORT_OutNameGet(MIDI_PORT_InIxGet((mios32_midi_port_t) n->dst_port)),
                 n->dst_chn);

         FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));
      }
   }

   // write MIDI IN MCLK ports config
   sprintf(line_buffer_, "MIDI_IN_MClock_Ports 0x%08x\n", (u32) midi_router_mclk_in);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write MIDI OUT mclk ports config
   sprintf(line_buffer_, "MIDI_OUT_MClock_Ports 0x%08x\n", (u32) midi_router_mclk_out);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write last used session number
   sprintf(line_buffer_, "SETUP_Last_Used_Session_Number %d\n", (s8) gcLastUsedSessionNumber_);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write BEAT LED config
   sprintf(line_buffer_, "SETUP_Beat_LEDs_Enabled %d\n", (s8) gcBeatLEDsEnabled_);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write BEAT display config
   sprintf(line_buffer_, "SETUP_Beat_Display_Enabled %d\n", (s8) gcBeatDisplayEnabled_);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write metronome port
   sprintf(line_buffer_, "SETUP_Metronome_Port %d\n", (u32) gcMetronomePort_);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write metronome channel
   sprintf(line_buffer_, "SETUP_Metronome_Channel %d\n", (u32) gcMetronomeChannel_);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write metronome measure note
   sprintf(line_buffer_, "SETUP_Metronome_NoteM %d\n", (u32) gcMetronomeNoteM_);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write metronome beat note
   sprintf(line_buffer_, "SETUP_Metronome_NoteB %d\n", (u32) gcMetronomeNoteB_);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write screensaver-after-minutes
   sprintf(line_buffer_, "SETUP_Screensaver_Minutes %d\n", (u32) gcScreensaverAfterMinutes_);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write font type
   sprintf(line_buffer_, "SETUP_Font_Type %c\n", (s8) gcFontType_);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write oled inversion config
   sprintf(line_buffer_, "SETUP_Invert_OLED_Enabled %d\n", (s8) gcInvertOLED_);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write oled inversion config
   sprintf(line_buffer_, "SETUP_Invert_OLED_Enabled %d\n", (s8) gcInvertOLED_);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write footswitch inversion config
   sprintf(line_buffer_, "SETUP_Footswitches_Inversionmask 0x%08x\n", (s8) gcFootswitchesInversionmask_);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write footswitch actions config
   sprintf(line_buffer_, "SETUP_Footswitch1_Action %s\n", footswitchActionDescriptions_[gcFootswitch1Action_].configname);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));
   sprintf(line_buffer_, "SETUP_Footswitch2_Action %s\n", footswitchActionDescriptions_[gcFootswitch2Action_].configname);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write mute leds inversion config
   sprintf(line_buffer_, "SETUP_Invert_MUTE_LEDs %d\n", (s8) gcInvertMuteLEDs_);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write track switch config
   sprintf(line_buffer_, "SETUP_Trackswitch_Type %s\n", trackswitchDescriptions_[gcTrackswitchType_].configname);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write tempodelta config
   sprintf(line_buffer_, "SETUP_Tempodelta_Type %s\n", tempodeltaDescriptions_[gcTempodeltaType_].configname);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write followtrack config
   sprintf(line_buffer_, "SETUP_Followtrack_Type %s\n", followtrackDescriptions_[gcFollowtrackType_].configname);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // write LEDnotes config
   sprintf(line_buffer_, "SETUP_LED_Notes %d\n", (s8) gcLEDNotes_);
   FILE_WriteBuffer((u8 *)line_buffer_, strlen(line_buffer_));

   // close file
   FILE_WriteClose();

   MUTEX_SDCARD_GIVE;

}
// ----------------------------------------------------------------------------------------


/**
 * Read global setup from disk (usually only at startup time, called from MUTEX locked environment)
 *
 */
void readSetup()
{
   file_t file;
   s32 status;
   s32 value;
   gcNumberOfActiveUserInstruments_ = 0;

   if (FILE_ReadOpen(&file, CONFIG_FILE_PATH) < 0)
   {
      return;
   }

   // read config values
   do
   {
      status = FILE_ReadLine((u8 *) line_buffer_, 128);
      /// DEBUG_MSG("readSetup() read: %s", line_buffer_);

      if (status > 1)
      {
         // sscanf consumes too much memory, therefore we parse directly
         char *separators = " \t";
         char *brkt;
         char *parameter;

         if ((parameter = strtok_r(line_buffer_, separators, &brkt)))
         {
            if (*parameter == '#')
            {
               // ignore comments
            }
            else
            {
               char *word = strtok_r(NULL, separators, &brkt);

               if (strcmp(parameter, "INSTRUMENT") == 0)
               {
                  s32 instrumentNumber = get_dec_range(word, parameter, 0, 255);
                  if (instrumentNumber < 0 || instrumentNumber >= SETUP_NUM_USERINSTRUMENTS)
                     continue;

                  word = strtok_r(NULL, separators, &brkt);
                  strncpy(userInstruments_[instrumentNumber].name, word, 8);

                  word = strtok_r(NULL, separators, &brkt);
                  userInstruments_[instrumentNumber].port = (mios32_midi_port_t) MIDI_PORT_OutPortFromNameGet(word);

                  word = strtok_r(NULL, separators, &brkt);
                  userInstruments_[instrumentNumber].channel = get_dec(word);

                  if (userInstruments_[instrumentNumber].channel > 0)
                     gcNumberOfActiveUserInstruments_++;

                  /// DEBUG_MSG("readSetup() number of userInstruments now: %d", gcNumberOfActiveUserInstruments_);
               }
               else if (strcmp(parameter, "MIDI_RouterNode") == 0)
               {
                  int values[5];

                  value = get_dec_range(word, parameter, 0, 255);
                  if (value < 0)
                     continue;

                  values[0] = value;
                  int i;
                  for (i = 1; i < 5; ++i)
                  {
                     word = strtok_r(NULL, separators, &brkt);

                     if (i == 1)
                     {
                        values[i] = MIDI_PORT_InPortFromNameGet(word);
                     }
                     else if (i == 3)
                     {
                        values[i] = MIDI_PORT_OutPortFromNameGet(word);
                     }
                     else
                     {
                        values[i] = get_dec(word);
                     }
                     if (values[i] < 0)
                     {
                        break;
                     }
                  }

                  if (i == 5)
                  {
                     if (values[0] < MIDI_ROUTER_NUM_NODES)
                     {
                        midi_router_node_entry_t *n = &midi_router_node[values[0]];
                        n->src_port = (u8) values[1];
                        n->src_chn = (u8) values[2];
                        n->dst_port = (u8) values[3];
                        n->dst_chn = (u8) values[4];
                     }
                  }
               }
               else if (strcmp(parameter, "MIDI_IN_MClock_Ports") == 0)
               {
                  value = get_dec_range(word, parameter, 0, 0x7fffffff);
                  if (value >= 0)
                     midi_router_mclk_in = value;
               }
               else if (strcmp(parameter, "MIDI_OUT_MClock_Ports") == 0)
               {
                  value = get_dec_range(word, parameter, 0, 0x7fffffff);
                  if (value >= 0)
                     midi_router_mclk_out = value;
               }
               else if (strcmp(parameter, "SETUP_Last_Used_Session_Number") == 0)
               {
                  value = get_dec_range(word, parameter, 0, 0x7fffffff);
                  if (value > 0)
                     sessionNumber_ = value;
               }
               else if (strcmp(parameter, "SETUP_Beat_LEDs_Enabled") == 0)
               {
                  value = get_dec_range(word, parameter, 0, 0x7fffffff);
                  gcBeatLEDsEnabled_ = value;
               }
               else if (strcmp(parameter, "SETUP_Beat_Display_Enabled") == 0)
               {
                  value = get_dec_range(word, parameter, 0, 0x7fffffff);
                  gcBeatDisplayEnabled_ = value;
               }
               else if (strcmp(parameter, "SETUP_Metronome_Port") == 0)
               {
                  value = get_dec_range(word, parameter, 0, 0x7fffffff);
                  gcMetronomePort_ = (mios32_midi_port_t)value;
               }
               else if (strcmp(parameter, "SETUP_Metronome_Channel") == 0)
               {
                  value = get_dec_range(word, parameter, 0, 0x7fffffff);
                  gcMetronomeChannel_ = value;
               }
               else if (strcmp(parameter, "SETUP_Metronome_NoteM") == 0)
               {
                  value = get_dec_range(word, parameter, 0, 0x7fffffff);
                  gcMetronomeNoteM_ = value;
               }
               else if (strcmp(parameter, "SETUP_Metronome_NoteB") == 0)
               {
                  value = get_dec_range(word, parameter, 0, 0x7fffffff);
                  gcMetronomeNoteB_ = value;
               }
               else if (strcmp(parameter, "SETUP_Screensaver_Minutes") == 0)
               {
                  value = get_dec_range(word, parameter, 0, 0x7fffffff);
                  gcScreensaverAfterMinutes_ = value;
               }
               else if (strcmp(parameter, "SETUP_Font_Type") == 0)
               {
                  gcFontType_ = word[0];
               }
               else if (strcmp(parameter, "SETUP_Invert_OLED_Enabled") == 0)
               {
                  value = get_dec_range(word, parameter, 0, 0x7fffffff);
                  gcInvertOLED_ = value;
               }
               else if (strcmp(parameter, "SETUP_Footswitches_Inversionmask") == 0)
               {
                  value = get_dec_range(word, parameter, 0, 0x7fffffff);
                  gcFootswitchesInversionmask_ = value;
               }
               else if (strcmp(parameter, "SETUP_Footswitch1_Action") == 0)
               {
                  u8 i;

                  for (i = 0; i < SETUP_NUM_FOOTSWITCHACTIONS; i++)
                  {
                     if (strcasecmp(word, footswitchActionDescriptions_[i].configname) == 0)
                     {
                        gcFootswitch1Action_ = (enum FootswitchActionEnum)i;
                     }
                  }
               }
               else if (strcmp(parameter, "SETUP_Footswitch2_Action") == 0)
               {
                  u8 i;

                  for (i = 0; i < SETUP_NUM_FOOTSWITCHACTIONS; i++)
                  {
                     if (strcasecmp(word, footswitchActionDescriptions_[i].configname) == 0)
                     {
                        gcFootswitch2Action_ = (enum FootswitchActionEnum)i;
                     }
                  }
               }
               else if (strcmp(parameter, "SETUP_Invert_MUTE_LEDs") == 0)
               {
                  value = get_dec_range(word, parameter, 0, 0x7fffffff);
                  gcInvertMuteLEDs_ = value;
               }
               else if (strcmp(parameter, "SETUP_Trackswitch_Type") == 0)
               {
                  u8 i;

                  for (i = 0; i < SETUP_NUM_TRACKSWITCHTYPES; i++)
                  {
                     if (strcasecmp(word, trackswitchDescriptions_[i].configname) == 0)
                     {
                        gcTrackswitchType_ = (enum TrackswitchTypeEnum)i;
                     }
                  }
               }
               else if (strcmp(parameter, "SETUP_Tempodelta_Type") == 0)
               {
                  u8 i;

                  for (i = 0; i < SETUP_NUM_TEMPODELTATYPES; i++)
                  {
                     if (strcasecmp(word, tempodeltaDescriptions_[i].configname) == 0)
                     {
                        gcTempodeltaType_ = (enum TempodeltaTypeEnum)i;
                     }
                  }
               }
               else if (strcmp(parameter, "SETUP_Followtrack_Type") == 0)
               {
                  u8 i;

                  for (i = 0; i < SETUP_NUM_FOLLOWTRACKTYPES; i++)
                  {
                     if (strcasecmp(word, followtrackDescriptions_[i].configname) == 0)
                     {
                        gcFollowtrackType_ = (enum FollowtrackTypeEnum)i;
                     }
                  }
               }
               else if (strcmp(parameter, "SETUP_LED_Notes") == 0)
               {
                  value = get_dec_range(word, parameter, 0, 0x7fffffff);
                  gcLEDNotes_ = value;
               }

            }
         }
      }
   } while (status >= 1);

   // close file
   FILE_ReadClose(&file);
}
// ----------------------------------------------------------------------------------------


/**
 * Setup screen: parameter button depressed
 *
 */
void setupParameterDepressed(u8 parameterNumber)
{
   mios32_midi_port_t port;
   u8 enable;
   switch (setupActiveItem_)
   {
      case SETUP_MCLK_DIN_IN:
         port = parameterNumber == 1 ? UART0 : (parameterNumber == 2 ? UART1 : (parameterNumber == 3 ? UART2 : UART3));
         enable = MIDI_ROUTER_MIDIClockInGet(port);
         enable = !enable;
         MIDI_ROUTER_MIDIClockInSet(port, enable);
         command_ = COMMAND_SETUP_SELECT;
         break;

      case SETUP_MCLK_DIN_OUT:
         port = parameterNumber == 1 ? UART0 : (parameterNumber == 2 ? UART1 : (parameterNumber == 3 ? UART2 : UART3));
         enable = MIDI_ROUTER_MIDIClockOutGet(port);
         enable = !enable;
         MIDI_ROUTER_MIDIClockOutSet(port, enable);
         command_ = COMMAND_SETUP_SELECT;
         break;

      case SETUP_MCLK_USB_IN:
         port = parameterNumber == 1 ? USB0 : (parameterNumber == 2 ? USB1 : (parameterNumber == 3 ? USB2 : USB3));
         enable = MIDI_ROUTER_MIDIClockInGet(port);
         enable = !enable;
         MIDI_ROUTER_MIDIClockInSet(port, enable);
         command_ = COMMAND_SETUP_SELECT;
         break;

      case SETUP_MCLK_USB_OUT:
         port = parameterNumber == 1 ? USB0 : (parameterNumber == 2 ? USB1 : (parameterNumber == 3 ? USB2 : USB3));
         enable = MIDI_ROUTER_MIDIClockOutGet(port);
         enable = !enable;
         MIDI_ROUTER_MIDIClockOutSet(port, enable);
         command_ = COMMAND_SETUP_SELECT;
         break;

      case SETUP_BEAT_LEDS_ENABLED:
         gcBeatLEDsEnabled_ = !gcBeatLEDsEnabled_;
         command_ = COMMAND_SETUP_SELECT;
         break;

      case SETUP_BEAT_DISPLAY_ENABLED:
         gcBeatDisplayEnabled_ = !gcBeatDisplayEnabled_;
         command_ = COMMAND_SETUP_SELECT;
         break;

      case SETUP_INVERT_OLED:
         gcInvertOLED_ = !gcInvertOLED_;
         command_ = COMMAND_SETUP_SELECT;
         break;

      case SETUP_INVERT_FOOTSWITCHES:
         if (parameterNumber == 1)
            gcFootswitchesInversionmask_ ^= 0x1;
         if (parameterNumber == 2)
            gcFootswitchesInversionmask_ ^= 0x2;
         break;

      case SETUP_INVERT_MUTE_LEDS:
         gcInvertMuteLEDs_ = !gcInvertMuteLEDs_;
         command_ = COMMAND_SETUP_SELECT;
         break;

      case SETUP_LED_NOTES:
         gcLEDNotes_ = !gcLEDNotes_;
         command_ = COMMAND_SETUP_SELECT;
         break;
   }

   configChangesToBeWritten_ = 1;
}
// ----------------------------------------------------------------------------------------

/**
 * Setup screen: parameter encoder turned
 *
 */
void setupParameterEncoderTurned(u8 parameterNumber, s32 incrementer)
{
   s16 newNote;

   switch (setupActiveItem_)
   {
      case SETUP_FONT_TYPE:
         gcFontType_ = gcFontType_ == 'a' ? gcFontType_ = 'b' : 'a';
         break;

      case SETUP_BEAT_LEDS_ENABLED:
         gcBeatLEDsEnabled_ = !gcBeatLEDsEnabled_;
         break;

      case SETUP_BEAT_DISPLAY_ENABLED:
         gcBeatDisplayEnabled_ = !gcBeatDisplayEnabled_;
         break;

      case SETUP_METRONOME:
         if (command_ == COMMAND_SETUP_PAR1)
         {
            gcMetronomePort_ = adjustLoopAPortNumber(gcMetronomePort_, incrementer);
         }
         else if (command_ == COMMAND_SETUP_PAR2)
         {
            s8 newChannel = gcMetronomeChannel_ += incrementer;
            newChannel = newChannel > 15 ? 15 : newChannel;
            newChannel = newChannel < 0 ? 0 : newChannel;

            gcMetronomeChannel_ = (u8)newChannel;
         }
         else if (command_ == COMMAND_SETUP_PAR3)
         {
            newNote = gcMetronomeNoteM_ + incrementer;

            if (newNote < 1)
               newNote = 1;

            if (newNote >= 127)
               newNote = 127;

            gcMetronomeNoteM_ = newNote;
         }
         else if (command_ == COMMAND_SETUP_PAR4)
         {
            s16 newNote = gcMetronomeNoteB_ + incrementer;

            if (newNote < 1)
               newNote = 1;

            if (newNote >= 127)
               newNote = 127;

            gcMetronomeNoteB_ = newNote;
         }
         break;

      case SETUP_TEMPODELTA:
         if (command_ == COMMAND_SETUP_PAR1)
         {
            s8 newTempodeltaType = gcTempodeltaType_ += incrementer;
            newTempodeltaType = newTempodeltaType >= SETUP_NUM_TEMPODELTATYPES ? SETUP_NUM_TEMPODELTATYPES - 1 : newTempodeltaType;
            newTempodeltaType = newTempodeltaType < 0 ? 0 : newTempodeltaType;
            gcTempodeltaType_ = (enum TempodeltaTypeEnum)newTempodeltaType;
         }
         break;

      case SETUP_SCREENSAVER_MINUTES:
         if (command_ == COMMAND_SETUP_PAR1)
         {
            s8 newScreensaver = gcScreensaverAfterMinutes_ + incrementer;

            if (newScreensaver < 1)
               newScreensaver = 1;

            if (newScreensaver >= 120)
               newScreensaver = 120;

            gcScreensaverAfterMinutes_ = newScreensaver;
         }
         break;

      case SETUP_INVERT_OLED:
         gcInvertOLED_ = !gcInvertOLED_;
         break;

      case SETUP_INVERT_FOOTSWITCHES:
         if (command_ == COMMAND_SETUP_PAR1)
            gcFootswitchesInversionmask_ ^= 0x1;
         else if (command_ == COMMAND_SETUP_PAR2)
            gcFootswitchesInversionmask_ ^= 0x2;
         break;

      case SETUP_FOOTSWITCH1_ACTION:
         if (command_ == COMMAND_SETUP_PAR1)
         {
            s8 newAction = gcFootswitch1Action_ += incrementer;
            newAction = newAction >= SETUP_NUM_FOOTSWITCHACTIONS ? SETUP_NUM_FOOTSWITCHACTIONS - 1 : newAction;
            newAction = newAction < 0 ? 0 : newAction;
            gcFootswitch1Action_ = (enum FootswitchActionEnum)newAction;
         }
         break;

      case SETUP_FOOTSWITCH2_ACTION:
         if (command_ == COMMAND_SETUP_PAR1)
         {
            s8 newAction = gcFootswitch2Action_ += incrementer;
            newAction = newAction >= SETUP_NUM_FOOTSWITCHACTIONS ? SETUP_NUM_FOOTSWITCHACTIONS - 1 : newAction;
            newAction = newAction < 0 ? 0 : newAction;
            gcFootswitch2Action_ = (enum FootswitchActionEnum)newAction;
         }
         break;

      case SETUP_INVERT_MUTE_LEDS:
         gcInvertMuteLEDs_ = !gcInvertMuteLEDs_;
         break;

      case SETUP_TRACK_SWITCH_TYPE:
         if (command_ == COMMAND_SETUP_PAR1)
         {
            s8 newTrackswitchType = gcTrackswitchType_ += incrementer;
            newTrackswitchType = newTrackswitchType >= SETUP_NUM_TRACKSWITCHTYPES ? SETUP_NUM_TRACKSWITCHTYPES - 1 : newTrackswitchType;
            newTrackswitchType = newTrackswitchType < 0 ? 0 : newTrackswitchType;
            gcTrackswitchType_ = (enum TrackswitchTypeEnum)newTrackswitchType;
         }
         break;

      case SETUP_FOLLOW_TRACK_TYPE:
         if (command_ == COMMAND_SETUP_PAR1)
         {
            s8 newFollowtrackType = gcFollowtrackType_ += incrementer;
            newFollowtrackType = newFollowtrackType >= SETUP_NUM_FOLLOWTRACKTYPES ? SETUP_NUM_FOLLOWTRACKTYPES - 1 : newFollowtrackType;
            newFollowtrackType = newFollowtrackType < 0 ? 0 : newFollowtrackType;
            gcFollowtrackType_ = (enum FollowtrackTypeEnum)newFollowtrackType;
         }
         break;

      case SETUP_LED_NOTES:
         gcLEDNotes_ = !gcLEDNotes_;
         break;

   }

   configChangesToBeWritten_ = 1;
}
// ----------------------------------------------------------------------------------------


/**
 * Adjust internal LoopA port number by incrementer change (negative LoopA port numbers are user instruments, positve are mios ports)
 *
 */
extern s8 adjustLoopAPortNumber(s8 loopaPortNumber, s32 incrementer)
{
   s8 newPortNumber = loopaPortNumber;

   /// DEBUG_MSG("------------------");
   /// DEBUG_MSG("port number before: %d", loopaPortNumber);

   if (loopaPortNumber > 0) // LoopA port number is a mios port number (positive), handle changes
   {
      /// DEBUG_MSG("is a normal mios port number");
      s8 newPortIndex = (s8)(MIDI_PORT_OutIxGet((mios32_midi_port_t)loopaPortNumber) + incrementer);

      // Limit "min port"
      newPortIndex = (s8)(newPortIndex < 1 ? 1 : newPortIndex);

      if (newPortIndex < MIDI_PORT_OutNumGet()-1-4)
         newPortNumber = MIDI_PORT_OutPortGet((u8)newPortIndex);
      else
      {
         // Transition to user instruments, if we have user instruments, switch to first one (loopa port number -1)
         if (gcNumberOfActiveUserInstruments_)
         {
            /// DEBUG_MSG("transition to user instrument port");
            newPortNumber = -1;
         }
      }
   }
   else // LoopA port number is a user instrument (negative), handle changes
   {
      /// DEBUG_MSG("is a user instrument port number");
      newPortNumber = loopaPortNumber - incrementer; // negative instrument port numbers, apply negative incrementers to increase abs value

      // Limit absolute "min port"
      if (newPortNumber < -gcNumberOfActiveUserInstruments_)
         newPortNumber = -gcNumberOfActiveUserInstruments_;

      if (newPortNumber >= 0)
      {
         /// DEBUG_MSG("transition to mios port number");
         // Transition back to mios port numbers, switch to the first one
         newPortNumber = MIDI_PORT_OutPortGet(MIDI_PORT_OutIxGet(UART2));
      }
   }

   /// DEBUG_MSG("port number after: %d", newPortNumber);

   return newPortNumber;
}
// ----------------------------------------------------------------------------------------


/**
 * Return true, if loopaPortNumber is a user defined instrument (don't print channel number then)
 *
 */
u8 isInstrument(s8 loopaPortNumber)
{
   return loopaPortNumber < 0;
}
// ----------------------------------------------------------------------------------------


/**
 * Get verbal port or instrument name from loopaPortNumber
 *
 * @param loopaPortNumber
 * @return string name of port or user instrument name based on portIndex
 */
char* getPortOrInstrumentNameFromLoopAPortNumber(s8 loopaPortNumber)
{
   if (isInstrument(loopaPortNumber))
   {
      u8 lookupIndex = ((s8)-1 -loopaPortNumber); // 1st user instrument has index 0

      if (lookupIndex < SETUP_NUM_USERINSTRUMENTS && lookupIndex < gcNumberOfActiveUserInstruments_)
         return userInstruments_[lookupIndex].name;
      else
         loopaPortNumber = UART0;
   }

   return MIDI_PORT_OutNameGet(MIDI_PORT_OutIxGet((mios32_midi_port_t)loopaPortNumber));
}
// ----------------------------------------------------------------------------------------


/**
 * Get numeric mios port id from loopaPortNumber
 *
 * @param loopaPortNumber
 * @return mios32 id of port or user instrument port based on portIndex
 */
mios32_midi_port_t getMIOSPortNumberFromLoopAPortNumber(s8 loopaPortNumber)
{
   if (loopaPortNumber > 0)
      return loopaPortNumber;

   u8 lookupIndex = ((s8)-1 -loopaPortNumber); // 1st user instrument has index 0

   if (lookupIndex < SETUP_NUM_USERINSTRUMENTS && lookupIndex < gcNumberOfActiveUserInstruments_)
      return userInstruments_[lookupIndex].port;

   // Could not resolve, return original MIOS port number
   return UART0;
}
// ----------------------------------------------------------------------------------------


/**
 * Get channel number from loopaPortNumber
 */
u8 getInstrumentChannelNumberFromLoopAPortNumber(s8 loopaPortNumber)
{
   if (loopaPortNumber > 0)
      return 0;

   u8 lookupIndex = ((s8)-1 -loopaPortNumber); // 1st user instrument has index 0

   if (lookupIndex < SETUP_NUM_USERINSTRUMENTS && lookupIndex < gcNumberOfActiveUserInstruments_)
      return userInstruments_[lookupIndex].channel - 1; // mios channels start at index 0

   return 0;
}
// ----------------------------------------------------------------------------------------

