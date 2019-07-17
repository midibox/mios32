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
s8 gcFontType_ = 'a';
s8 gcInvertOLED_ = 0;
s8 gcBeatLEDsEnabled_ = 0;
s8 gcBeatDisplayEnabled_ = 0;
s8 gcNumberOfActiveUserInstruments_ = 0;

mios32_midi_port_t gcMetronomePort_ = 0;
u8 gcMetronomeChannel_ = 0;
u8 gcMetronomeNoteM_ = 0x25; // C#1
u8 gcMetronomeNoteB_ = 0x25; // C#1
u8 gcScreensaverAfterMinutes_ = 1;

// --- Global config settings ---
SetupParameter setupParameters_[SETUP_NUM_ITEMS] =
        {
                {"System Font", "Type", "", "", ""},
                {"Beat LEDs", "Toggle", "", "", ""},
                {"Beat Display", "Toggle", "", "", ""},
                // {"Command Help", "Toggle", "", "", ""}, TODO Later
                {"Screensaver", "Minutes", "", "", ""},
                {"Invert OLED", "Toggle", "", "", ""},
                {"Metronome", "Port", "Chn", "Meas.", "Beat"},
                // {"Tempo Up/Dn", "BPM/Sec", "", "", ""}, TODO Later

                {"MCLK DIN IN",  "Toggle",  "Toggle",  "Toggle", "Toggle"},
                {"MCLK DIN OUT", "Toggle", "Toggle", "Toggle", "Toggle"},
                {"MCLK USB IN",  "Toggle", "Toggle", "Toggle", "Toggle"},
                {"MCLK USB OUT", "Toggle", "Toggle", "Toggle", "Toggle"},

                /* {"Default Tr.1", "Port", "Chn", "Length", "Forward"}, TODO later
                {"Default Tr.2", "Port", "Chn", "Length", "Forward"},
                {"Default Tr.3", "Port", "Chn", "Length", "Forward"},
                {"Default Tr.4", "Port", "Chn", "Length", "Forward"},
                {"Default Tr.5", "Port", "Chn", "Length", "Forward"},
                {"Default Tr.6", "Port", "Chn", "Length", "Forward"} */

                //{"Instrument 1", "Name", Port", "Chn", ""}, TODO later, when we have a keyboard/string entry editor
                //{"Instrument 2", "name", "Port", "Chn", ""},
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
      DEBUG_MSG("readSetup() read: %s", line_buffer_);

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

                  DEBUG_MSG("readSetup() number of userInstruments now: %d", gcNumberOfActiveUserInstruments_);
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

