// $Id$
/*
 * Header file for ESP8266 Firmware Access Routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _ESP8266_FW_H
#define _ESP8266_FW_H

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

//! firmware (bootloader) access functions enabled?
#ifndef ESP8266_FW_ACCESS_ENABLED
#define ESP8266_FW_ACCESS_ENABLED 0
#endif


//! if set to 0, we expect that a esp8266_fw_img_0x00000 and esp8266_fw_0x40000 array
//! defined in esp8266_fw.inc (which will be included by esp8266_fw.c, and can be located in your app directory)
#ifndef ESP8266_FW_DEFAULT_FIRMWARE
#define ESP8266_FW_DEFAULT_FIRMWARE 1
#endif

//! allows to change the address of the upper binary (which is usually located at 0x40000,
//! but sometimes lower if a bigger OS kernel image is used)
#ifndef ESP8266_FW_FIRMWARE_UPPER_BASE_ADDRESS
#define ESP8266_FW_FIRMWARE_UPPER_BASE_ADDRESS 0x40000
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef enum {
  ESP8266_FW_EXEC_CMD_QUERY = 0,
  ESP8266_FW_EXEC_CMD_ERASE_FLASH,
  ESP8266_FW_EXEC_CMD_PROG_FLASH
} esp8266_fw_exec_cmd_t;


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

#if ESP8266_FW_ACCESS_ENABLED
extern s32 ESP8266_FW_Periodic_mS(void);

extern s32 ESP8266_FW_Read(u8 *buffer, u32 size);
extern s32 ESP8266_FW_Write(u8 *buffer, u32 size, u8 *buffer2, u32 size2);
extern s32 ESP8266_FW_Checksum(u8 *buffer, u32 size, u8 initial_chk);
extern s32 ESP8266_FW_ReceiveFwReply(u8 *op_ret, u32 *val_ret, u8 *buffer, u32 buffer_max_size);
extern s32 ESP8266_FW_SendFwCommand(u8 op, u8 *buffer, u32 size, u8 chk, u32 *reply_val, u8 *reply_buffer, u32 reply_buffer_max_size);

extern s32 ESP8266_FW_Sync(void);
extern s32 ESP8266_FW_ReadReg(u32 addr, u32 *ret_value);
extern s32 ESP8266_FW_WriteReg(u32 addr, u32 value, u32 mask, u32 delay_us);
extern s32 ESP8266_FW_MemBegin(u32 size, u32 blocks, u32 blocksize, u32 offset);
extern s32 ESP8266_FW_MemBlock(u8 *buffer, u32 size, u32 seq);
extern s32 ESP8266_FW_MemFinish(u32 entrypoint);
extern s32 ESP8266_FW_FlashBegin(u32 size, u32 offset);
extern s32 ESP8266_FW_FlashBlock(u8 *buffer, u32 size, u32 seq);
extern s32 ESP8266_FW_FlashFinish(u32 reboot);
extern s32 ESP8266_FW_FlashErase(void);
extern s32 ESP8266_FW_FlashProg(u8 *image, u32 size, u32 addr);
extern s32 ESP8266_FW_Run(u32 reboot);
extern s32 ESP8266_FW_ReadMac(u8 mac[6]);
extern s32 ESP8266_FW_ReadChipId(u32 *chip_id);
extern s32 ESP8266_FW_ReadFlashId(u32 *flash_id);

extern s32 ESP8266_FW_Exec(esp8266_fw_exec_cmd_t exec_cmd);
#endif

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif /* _ESP8266_H */
