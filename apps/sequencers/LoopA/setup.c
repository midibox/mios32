#include "commonIncludes.h"

#include "setup.h"

// --- Globals ---
u8 configChangesToBeWritten_ = 0;


/**
 * (After a configuration change), write the global setup file to disk
 * 
 */
void writeSetup()
{
   configChangesToBeWritten_ = 0;
   MUTEX_SDCARD_TAKE;

   if ((FILE_WriteOpen(configFilePath, 1)) < 0)
   {
      FILE_WriteClose(); // important to free memory given by malloc
      MUTEX_SDCARD_GIVE;
      return;
   }

   char line_buffer[128];

   // WRITE MIDI ROUTER CONFIG
   {
      u8 node;
      midi_router_node_entry_t *n = &midi_router_node[0];
      for (node = 0; node < MIDI_ROUTER_NUM_NODES; ++node, ++n)
      {
         sprintf(line_buffer, "MIDI_RouterNode %d %s %d %s %d\n",
                 node,
                 MIDI_PORT_InNameGet(MIDI_PORT_InIxGet((mios32_midi_port_t) n->src_port)),
                 n->src_chn,
                 MIDI_PORT_OutNameGet(MIDI_PORT_InIxGet((mios32_midi_port_t) n->dst_port)),
                 n->dst_chn);

         FILE_WriteBuffer((u8 *)line_buffer, strlen(line_buffer));
      }
   }

   // close file
   FILE_WriteClose();

   MUTEX_SDCARD_GIVE;

}
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
 * Read global setup from disk (usually only at startup time, called from MUTEX locked environment)
 *
 */
void readSetup()
{
   file_t file;
   s32 status;

   if (FILE_ReadOpen(&file, configFilePath) < 0)
   {
      return;
   }

   // read config values
   char line_buffer[128];
   do
   {
      status = FILE_ReadLine((u8 *) line_buffer, 128);
      /// DEBUG_MSG("readSetup() read: %s", line_buffer);

      if (status > 1)
      {
         // sscanf consumes too much memory, therefore we parse directly
         char *separators = " \t";
         char *brkt;
         char *parameter;

         if ((parameter = strtok_r(line_buffer, separators, &brkt)))
         {
            if (*parameter == '#')
            {
               // ignore comments
            }
            else
            {
               char *word = strtok_r(NULL, separators, &brkt);

               if (strcmp(parameter, "MIDI_RouterNode") == 0)
               {
                  int values[5];

                  s32 value = get_dec_range(word, parameter, 0, 255);
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
            }
         }
      }
   } while (status >= 1);

   // close file
   FILE_ReadClose(&file);
}
// ----------------------------------------------------------------------------------------
