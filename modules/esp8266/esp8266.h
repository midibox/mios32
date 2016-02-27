// $Id$
/*
 * Header file for ESP8266 driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _ESP8266_H
#define _ESP8266_H

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

//! support for direct send command? (! in terminal)
#ifndef ESP8266_TERMINAL_DIRECT_SEND_CMD
#define ESP8266_TERMINAL_DIRECT_SEND_CMD 0
#endif

//! it's recommended to assign the MIDIOUT mutex used by the application in mios32_config.h
#ifndef ESP8266_MUTEX_MIDIOUT_TAKE
#define ESP8266_MUTEX_MIDIOUT_TAKE { }
#endif
#ifndef ESP8266_MUTEX_MIDIOUT_GIVE
#define ESP8266_MUTEX_MIDIOUT_GIVE { }
#endif


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 ESP8266_Init(u32 mode);

extern s32 ESP8266_InitUart(mios32_midi_port_t port);
extern s32 ESP8266_DeInitUart(void);

extern s32 ESP8266_TerminalHelp(void *_output_function);
extern s32 ESP8266_TerminalParseLine(char *input, void *_output_function);

extern s32 ESP8266_SendCommand(const char* cmd);

extern s32 ESP8266_SendOscTestMessage(u8 chn, u8 cc, u8 value);


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif /* _ESP8266_H */
