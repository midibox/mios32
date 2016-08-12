// $Id$
//! \defgroup ESP8266
//!
//!  ESP8266 Firmware Access Routines
//!
//!  Inspired from "esptool.py":
//!  -> https://github.com/themadinventor/esptool
//!
//!  Firmware image format and serial protocol:
//!  -> https://github.com/themadinventor/esptool/wiki
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2016 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */


/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <string.h>

#include "esp8266.h"
#include "esp8266_fw.h"


//! Note: it's possible to disable the FW access part if not required
#if ESP8266_FW_ACCESS_ENABLED


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via MIOS32_MIDI_SendDebug*
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 2

#ifndef DEBUG_MSG
# define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#endif


/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// These are the currently known commands supported by the ROM
#define ESP_FLASH_BEGIN 0x02
#define ESP_FLASH_DATA  0x03
#define ESP_FLASH_END   0x04
#define ESP_MEM_BEGIN   0x05
#define ESP_MEM_END     0x06
#define ESP_MEM_DATA    0x07
#define ESP_SYNC        0x08
#define ESP_WRITE_REG   0x09
#define ESP_READ_REG    0x0a

// Maximum block sized for RAM and Flash writes, respectively.
// Reduced to 0x100 so that the buffers fit in stack memory
#define ESP_RAM_BLOCK   0x100
#define ESP_FLASH_BLOCK 0x100

// First byte of the application image
#define ESP_IMAGE_MAGIC 0xe9

//Initial state for the checksum routine
#define ESP_CHECKSUM_MAGIC 0xef

// OTP ROM addresses
#define ESP_OTP_MAC0    0x3ff00050
#define ESP_OTP_MAC1    0x3ff00054
#define ESP_OTP_MAC3    0x3ff0005c


/////////////////////////////////////////////////////////////////////////////
// Firmware images which should be downloaded
/////////////////////////////////////////////////////////////////////////////

#if ESP8266_FW_DEFAULT_FIRMWARE
#include "ESP8266_AT_V0.51/0x00000.inc"
#include "ESP8266_AT_V0.51/0x40000.inc"
#else
// must be provided by the user
# include "esp8266_fw.inc"
#endif


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static volatile u16 timeout_ctr;


/////////////////////////////////////////////////////////////////////////////
//! Will be called from ESP8266_Periodic_mS to handle the timeout counter
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_Periodic_mS(void)
{
  // timeout counter
  if( timeout_ctr )
    --timeout_ctr;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Read bytes from the serial port while performing SLIP unescaping.
//! Note: terminalMode has to be disabled before calling this function!
//! \param[in] buffer contains the bytes which should be read
//! \param[in] size how many bytes should be read
//! \return < 0 on errors
//! \return >= 0: number of read bytes
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_Read(u8 *buffer, u32 size)
{
  u8 dev_uart = ESP8266_UartGet();
  if( !dev_uart )
    return -1; // no UART assigned

  u8 uart = dev_uart & 0xf;

  u32 pos;
  for(pos=0; pos<size; ++pos) {
    s32 c;
    timeout_ctr = 100; // wait for up to 100 uS
    while( (c=MIOS32_UART_RxBufferGet(uart)) == -2 ) {
      if( timeout_ctr == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[ESP8266_FW_Read] TIMEOUT (1)\n");
#endif
	return -2; // timeout
      }
    }

    if( c < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_Read] transfer failed, status %d\n", c);
#endif
      return -3; // transfer failed
    }

    if( c == 0xdb ) {
      timeout_ctr = 100; // wait for up to 100 uS
      while( (c=MIOS32_UART_RxBufferGet(uart)) == -2 ) {
	if( timeout_ctr == 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	  DEBUG_MSG("[ESP8266_FW_Read] TIMEOUT (2)\n");
#endif
	  return -2; // timeout
	}
      }

      if( c < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[ESP8266_FW_Read] additional read transfer failed, status %d\n", c);
#endif
	return -4; // transfer failed
      }

      if( c == 0xdd )
	buffer[pos] = 0xdb;
      else if( c == 0xdc )
	buffer[pos] = 0xc0;
      else {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[ESP8266_FW_Read] SLIP protocol failed, received 0x%02x instead of 0xdc or oxdd!\n", c);
#endif
	return -5; // invalid slip escape
      }
    } else {
      buffer[pos] = c;
    }
  }

  return pos; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Write bytes to the serial port while performing SLIP escaping.
//! Note: terminalMode has to be disabled before calling this function!
//! \param[in] buffer contains the bytes which should be written
//! \param[in] size how many bytes should be written
//! \param[in] buffer2 optional second data buffer
//! \param[in] size2 how many bytes should be written
//! \return < 0 on errors
//! \return >= 0: number of written bytes
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_Write(u8 *buffer, u32 size, u8 *buffer2, u32 size2)
{
  u8 dev_uart = ESP8266_UartGet();
  if( !dev_uart )
    return -1; // no UART assigned

  u8 uart = dev_uart & 0xf;

  MIOS32_UART_TxBufferPut(uart, 0xc0);

  // first buffer
  s32 status = 0;
  u32 pos;
  for(pos=0; pos<size; ++pos) {
    u8 c = buffer[pos];
    if( c == 0xdb ) {
      status |= MIOS32_UART_TxBufferPut(uart, 0xdb);
      status |= MIOS32_UART_TxBufferPut(uart, 0xdd);
    } else if( c == 0xc0 ) {
      status |= MIOS32_UART_TxBufferPut(uart, 0xdb);
      status |= MIOS32_UART_TxBufferPut(uart, 0xdc);
    } else {
      status |= MIOS32_UART_TxBufferPut(uart, c);
    }
  }

  // optional second buffer
  if( buffer2 && size2 ) {
    for(pos=0; pos<size2; ++pos) {
      u8 c = buffer2[pos];
      if( c == 0xdb ) {
	status |= MIOS32_UART_TxBufferPut(uart, 0xdb);
	status |= MIOS32_UART_TxBufferPut(uart, 0xdd);
      } else if( c == 0xc0 ) {
	status |= MIOS32_UART_TxBufferPut(uart, 0xdb);
	status |= MIOS32_UART_TxBufferPut(uart, 0xdc);
      } else {
	status |= MIOS32_UART_TxBufferPut(uart, c);
      }
    }
  }

  status |= MIOS32_UART_TxBufferPut(uart, 0xc0);

#if DEBUG_VERBOSE_LEVEL >= 2
  if( status < 0 ) {
    DEBUG_MSG("[ESP8266_FW_Write] failed transfer, status %d (combined)\n", status);
  }
#endif

  return pos; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Calculate checksum of a blob, as it is defined by the ROM.
//! \return < 0 on errors
//! \return >= 0: the checksum
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_Checksum(u8 *buffer, u32 size, u8 initial_chk)
{
  u8 chk = initial_chk;
  u32 pos;
  for(pos=0; pos<size; ++pos) {
    chk ^= *(buffer++);
  }
  return chk;
}


/////////////////////////////////////////////////////////////////////////////
//! receive a response to a command
//! \return < 0 on errors
//! \return >= 0: number of received bytes (in buffer)
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_ReceiveReply(u8 *op_ret, u32 *val_ret, u8 *buffer, u32 buffer_max_size)
{
  *op_ret = 0;
  *val_ret = 0;

  {
    u8 c;
    s32 status;
    if( (status=ESP8266_FW_Read(&c, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_ReceiveReply] initial read failed, status %d\n", status);
#endif
      return -2; // failed read
    }

    if( c != 0xc0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_ReceiveReply] initial read returned 0x%02x instead of 0xc0\n", c);
#endif
      return -3; // invalid header start
    }
  }

  u8 len_ret = 0;
  {
    u8 hdr[8];
    s32 status;
    if( (status=ESP8266_FW_Read(hdr, 8)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_ReceiveReply] header read failed, status %d\n", status);
#endif
      return -4; // failed read
    }

    if( hdr[0] != 0x01 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_ReceiveReply] invalid header indicator!\n");
      MIOS32_MIDI_SendDebugHexDump(hdr, 8);
#endif
      return -5; // invalid header indicator
    }

    *op_ret=hdr[1];
    len_ret = ((u32)hdr[2] << 0) | ((u32)hdr[3] << 8);
    *val_ret = ((u32)hdr[4] << 0) | ((u32)hdr[5] << 8) | ((u32)hdr[6] << 16) | ((u32)hdr[7] << 24);

    //MIOS32_MIDI_SendDebugHexDump(hdr, 8);
  }

  if( len_ret ) {

    if( len_ret > buffer_max_size ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_ReceiveReply] FW wants to send %d bytes, but app limits to %d bytes\n", len_ret, buffer_max_size);
#endif
      return -6; // too many reply bytes
    }

    s32 status;
    if( (status=ESP8266_FW_Read(buffer, len_ret)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_ReceiveReply] failed to read payload, status %d\n", status);
#endif
      return -7; // failed read
    }
    //MIOS32_MIDI_SendDebugHexDump((u8 *)buffer, len_ret);
  }

  {
    u8 c;
    s32 status;
    if( (status=ESP8266_FW_Read(&c, 1)) < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_ReceiveReply] failed to read terminator, status %d\n", status);
#endif
      return -8; // failed read
    }

    if( c != 0xc0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_ReceiveReply] terminator read returned 0x%02x instead of 0xc0\n", c);
#endif
      return -9; // invalid termination
    }
  }

  return len_ret; // no error: return length of buffer
}

/////////////////////////////////////////////////////////////////////////////
//! send a request and read the response
//! \return < 0 on errors
//! \return >= 0: number of written bytes
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_SendCommand(u8 op, u8 *buffer, u32 size, u8 chk, u32 *reply_val, u8 *reply_buffer, u32 reply_buffer_max_size)
{
  if( op ) {
    u8 cmd[8];

    cmd[0] = 0x00;
    cmd[1] = op;
    cmd[2] = (size >> 0) & 0xff;
    cmd[3] = (size >> 8) & 0xff;
    cmd[4] = chk;
    cmd[5] = 0;
    cmd[6] = 0;
    cmd[7] = 0;

    if( ESP8266_FW_Write(cmd, 8, buffer, size) < 0 )
      return -2; // transfer failed
  }

  // tries to get a response until that response has the
  // same operation as the request or a retries limit has
  // exceeded. This is needed for some esp8266s that
  // reply with more sync responses than expected.
  int retries = (op == ESP_SYNC) ? 5 : 50;
  for(; retries > 0; --retries) {
    u8 op_ret;
    s32 status = ESP8266_FW_ReceiveReply(&op_ret, reply_val, reply_buffer, reply_buffer_max_size);

    if( op == 0 || op_ret == op ) {
      return status; // == length of reply_buffer
    }
  }

  return -5; // no valid response
}

/////////////////////////////////////////////////////////////////////////////
//! Perform a connection test
//! \return < 0 on errors
//! \return >= 0: number of written bytes
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_Sync(void)
{
  u8 cmd[36];
  cmd[0] = 0x07;
  cmd[1] = 0x07;
  cmd[2] = 0x12;
  cmd[3] = 0x20;
  {
    int i;
    for(i=0; i<0x20; ++i) {
      cmd[4+i] = 0x55;
    }
  }

  {
    u32 reply_val;
    u8 reply_buffer[2];

    s32 status = ESP8266_FW_SendCommand(ESP_SYNC, cmd, sizeof(cmd), 0x00, &reply_val, reply_buffer, sizeof(reply_buffer));

    if( status < 0 )
      return -2; // failed to send command
    int i;
    for(i=0; i<7; ++i) {
      s32 status = ESP8266_FW_SendCommand(0, NULL, 0, 0x00, &reply_val, reply_buffer, sizeof(reply_buffer));

      if( status < 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
	DEBUG_MSG("[ESP8266_SwSync] didn't get reply #%d", i+1);
#endif
	return -3; // failed to receive replies
      }
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Read memory address in target
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_ReadReg(u32 addr, u32 *ret_value)
{
  u8 cmd[4] = {
    (addr >> 0) & 0xff,
    (addr >> 8) & 0xff,
    (addr >> 16) & 0xff,
    (addr >> 24) & 0xff
  };

  {
    u8 reply_buffer[2];
    *ret_value = 0;

    s32 status = ESP8266_FW_SendCommand(ESP_READ_REG, cmd, sizeof(cmd), 0x00, ret_value, reply_buffer, sizeof(reply_buffer));

    if( status < 0 )
      return -2; // failed to send command

    if( status != 2 || reply_buffer[0] != 0 || reply_buffer[1] != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_ReadReg] unexpected reply:\n");
      MIOS32_MIDI_SendDebugHexDump((u8 *)reply_buffer, status);
#endif
      return -3;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Write to memory address in target
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_WriteReg(u32 addr, u32 value, u32 mask, u32 delay_us)
{
  u8 cmd[16] = {
    (addr >> 0) & 0xff,
    (addr >> 8) & 0xff,
    (addr >> 16) & 0xff,
    (addr >> 24) & 0xff,
    (value >> 0) & 0xff,
    (value >> 8) & 0xff,
    (value >> 16) & 0xff,
    (value >> 24) & 0xff,
    (mask >> 0) & 0xff,
    (mask >> 8) & 0xff,
    (mask >> 16) & 0xff,
    (mask >> 24) & 0xff,
    (delay_us >> 0) & 0xff,
    (delay_us >> 8) & 0xff,
    (delay_us >> 16) & 0xff,
    (delay_us >> 24) & 0xff
  };

  {
    u8 reply_buffer[2];

    u32 dummy_value = 0;
    s32 status = ESP8266_FW_SendCommand(ESP_WRITE_REG, cmd, sizeof(cmd), 0x00, &dummy_value, reply_buffer, sizeof(reply_buffer));

    if( status < 0 )
      return -2; // failed to send command

    if( status != 2 || reply_buffer[0] != 0 || reply_buffer[1] != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_WriteReg] unexpected reply:\n");
      MIOS32_MIDI_SendDebugHexDump((u8 *)reply_buffer, status);
#endif
      return -3;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Start downloading an application image to RAM
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_MemBegin(u32 size, u32 blocks, u32 blocksize, u32 offset)
{
  u8 cmd[16] = {
    (size >> 0) & 0xff,
    (size >> 8) & 0xff,
    (size >> 16) & 0xff,
    (size >> 24) & 0xff,
    (blocks >> 0) & 0xff,
    (blocks >> 8) & 0xff,
    (blocks >> 16) & 0xff,
    (blocks >> 24) & 0xff,
    (blocksize >> 0) & 0xff,
    (blocksize >> 8) & 0xff,
    (blocksize >> 16) & 0xff,
    (blocksize >> 24) & 0xff,
    (offset >> 0) & 0xff,
    (offset >> 8) & 0xff,
    (offset >> 16) & 0xff,
    (offset >> 24) & 0xff
  };

  {
    u8 reply_buffer[2];

    u32 dummy_value = 0;
    s32 status = ESP8266_FW_SendCommand(ESP_MEM_BEGIN, cmd, sizeof(cmd), 0x00, &dummy_value, reply_buffer, sizeof(reply_buffer));

    if( status < 0 )
      return -2; // failed to send command

    if( status != 2 || reply_buffer[0] != 0 || reply_buffer[1] != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_MemBegin] unexpected reply:\n");
      MIOS32_MIDI_SendDebugHexDump((u8 *)reply_buffer, status);
#endif
      return -3;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Send a block of an image to RAM
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_MemBlock(u8 *buffer, u32 size, u32 seq)
{
  // 256 bytes maximum
  if( size > ESP_RAM_BLOCK ) {
    return -4;
  }

  u8 cmd[ESP_RAM_BLOCK+16] = { // preload first 16 byte
    (size >> 0) & 0xff,
    (size >> 8) & 0xff,
    (size >> 16) & 0xff,
    (size >> 24) & 0xff,
    (seq >> 0) & 0xff,
    (seq >> 8) & 0xff,
    (seq >> 16) & 0xff,
    (seq >> 24) & 0xff,
    0, 0, 0, 0,
    0, 0, 0, 0
  };

  // copy remaining bytes
  memcpy(&cmd[16], buffer, size);

  // create checksum
  u8 chk = ESP8266_FW_Checksum(buffer, size, ESP_CHECKSUM_MAGIC);

  // send block
  {
    u8 reply_buffer[2];

    u32 dummy_value = 0;
    s32 status = ESP8266_FW_SendCommand(ESP_MEM_DATA, cmd, 16+size, chk, &dummy_value, reply_buffer, sizeof(reply_buffer));

    if( status < 0 )
      return -2; // failed to send command

    if( status != 2 || reply_buffer[0] != 0 || reply_buffer[1] != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_MemBlock] unexpected reply:\n");
      MIOS32_MIDI_SendDebugHexDump((u8 *)reply_buffer, status);
#endif
      return -3;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Leave download mode and run the application
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_MemFinish(u32 entry_point)
{
  u8 cmd[8] = {
    (entry_point == 0) ? 1 : 0, 0, 0, 0,
    (entry_point >> 0) & 0xff,
    (entry_point >> 8) & 0xff,
    (entry_point >> 16) & 0xff,
    (entry_point >> 24) & 0xff
  };

  {
    u8 reply_buffer[2];

    u32 dummy_value = 0;
    s32 status = ESP8266_FW_SendCommand(ESP_MEM_END, cmd, sizeof(cmd), 0x00, &dummy_value, reply_buffer, sizeof(reply_buffer));

    if( status < 0 )
      return -2; // failed to send command

    if( status != 2 || reply_buffer[0] != 0 || reply_buffer[1] != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_MemFinish] unexpected reply:\n");
      MIOS32_MIDI_SendDebugHexDump((u8 *)reply_buffer, status);
#endif
      return -3;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Start downloading an application image to Flash (performs an erase)
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_FlashBegin(u32 size, u32 offset)
{
  u32 num_blocks = (size + ESP_FLASH_BLOCK - 1) / ESP_FLASH_BLOCK;
  u32 sectors_per_block = 16;
  u32 sector_size = 4096;
  u32 num_sectors = (size + sector_size - 1) / sector_size;
  u32 start_sector = offset / sector_size;

  u32 head_sectors = sectors_per_block - (start_sector % sectors_per_block);
  if( num_sectors < head_sectors )
    head_sectors = num_sectors;

  u32 erase_size;
  if( num_sectors < 2 * head_sectors )
    erase_size = (num_sectors + 1) / 2 * sector_size;
  else
    erase_size = (num_sectors - head_sectors) * sector_size;

  u8 cmd[16] = {
    (erase_size >> 0) & 0xff,
    (erase_size >> 8) & 0xff,
    (erase_size >> 16) & 0xff,
    (erase_size >> 24) & 0xff,
    (num_blocks >> 0) & 0xff,
    (num_blocks >> 8) & 0xff,
    (num_blocks >> 16) & 0xff,
    (num_blocks >> 24) & 0xff,
    (ESP_FLASH_BLOCK >> 0) & 0xff,
    (ESP_FLASH_BLOCK >> 8) & 0xff,
    (ESP_FLASH_BLOCK >> 16) & 0xff,
    (ESP_FLASH_BLOCK >> 24) & 0xff,
    (offset >> 0) & 0xff,
    (offset >> 8) & 0xff,
    (offset >> 16) & 0xff,
    (offset >> 24) & 0xff
  };

  {
    u8 reply_buffer[2];

    u32 dummy_value = 0;
    s32 status = ESP8266_FW_SendCommand(ESP_FLASH_BEGIN, cmd, sizeof(cmd), 0x00, &dummy_value, reply_buffer, sizeof(reply_buffer));

    if( status < 0 )
      return -2; // failed to send command

    if( status != 2 || reply_buffer[0] != 0 || reply_buffer[1] != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_MemFlash] Failed to enter flash download mode:\n");
      MIOS32_MIDI_SendDebugHexDump((u8 *)reply_buffer, status);
#endif
      return -3;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Send a block of an image to Flash
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_FlashBlock(u8 *buffer, u32 size, u32 seq)
{
  // 1024 bytes maximum
  if( size > ESP_FLASH_BLOCK ) {
    return -4;
  }

  u8 cmd[ESP_FLASH_BLOCK+16] = { // preload first 16 byte
    (size >> 0) & 0xff,
    (size >> 8) & 0xff,
    (size >> 16) & 0xff,
    (size >> 24) & 0xff,
    (seq >> 0) & 0xff,
    (seq >> 8) & 0xff,
    (seq >> 16) & 0xff,
    (seq >> 24) & 0xff,
    0, 0, 0, 0,
    0, 0, 0, 0
  };

  // copy remaining bytes
  memcpy(&cmd[16], buffer, size);

  // create checksum
  u8 chk = ESP8266_FW_Checksum(buffer, size, ESP_CHECKSUM_MAGIC);

  // send block
  {
    u8 reply_buffer[2];

    u32 dummy_value = 0;
    s32 status = ESP8266_FW_SendCommand(ESP_FLASH_DATA, cmd, 16+size, chk, &dummy_value, reply_buffer, sizeof(reply_buffer));

    if( status < 0 )
      return -2; // failed to send command

    if( status != 2 || reply_buffer[0] != 0 || reply_buffer[1] != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_FlashBlock] unexpected reply:\n");
      MIOS32_MIDI_SendDebugHexDump((u8 *)reply_buffer, status);
#endif
      return -3;
    }
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Leave flash mode and run/reboot
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_FlashFinish(u32 reboot)
{
  u8 cmd[4] = {
    (reboot == 0) ? 1 : 0, 0, 0, 0
  };

  {
    u8 reply_buffer[2];

    u32 dummy_value = 0;
    s32 status = ESP8266_FW_SendCommand(ESP_FLASH_END, cmd, sizeof(cmd), 0x00, &dummy_value, reply_buffer, sizeof(reply_buffer));

    if( status < 0 )
      return -2; // failed to send command

    if( status != 2 || reply_buffer[0] != 0 || reply_buffer[1] != 0 ) {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_FlashFinish] unexpected reply:\n");
      MIOS32_MIDI_SendDebugHexDump((u8 *)reply_buffer, status);
#endif
      return -3;
    }
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Perform a chip erase of SPI flash
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_FlashErase(void)
{
  // This is hacky: we don't have a custom stub, instead we trick
  // the bootloader to jump to the SPIEraseChip() routine and then halt/crash
  // when it tries to boot an unconfigured system.
  if( ESP8266_FW_FlashBegin(0, 0) >= 0 &&
      ESP8266_FW_MemBegin(0, 0, 0, 0x40100000) >= 0 &&
      ESP8266_FW_MemFinish(0x40004984) >= 0 ) {
    // success

    // there is no way to detect iw we really successed...
  } else {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_ReadChipId] failed to read MAC registers\n");
#endif
      return -1;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Programs the SPI flash
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_FlashProg(u8 *image, u32 size, u32 addr)
{
  s32 status = 0;

#if DEBUG_VERBOSE_LEVEL >= 2
  DEBUG_MSG("[ESP8266_FW_FlashProg] Erasing Flash at 0x%06x...\n", addr);
#endif
  u32 blocks = (size / ESP_FLASH_BLOCK) + (((size % ESP_FLASH_BLOCK) != 0) ? 1 : 0); // rounding upwards
  status |=  ESP8266_FW_FlashBegin(blocks * ESP_FLASH_BLOCK, addr);

  u32 seq = 0;
  u32 offset = 0;
  for(offset=0; offset<size && status >= 0; offset += ESP_FLASH_BLOCK, ++seq) {
    DEBUG_MSG("[ESP8266_FW_FlashProg] Writing at 0x%06x... (%d%%)\n", addr + seq*ESP_FLASH_BLOCK, 100 * (seq+1) / blocks);
    status |= ESP8266_FW_FlashBlock((u8 *)&image[offset], ESP_FLASH_BLOCK, seq);
  }

  if( status < 0 ) {
    DEBUG_MSG("[ESP8266_FW_FlashProg] Failure during block programming!\n");
    return -1;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Run application code in flash
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_Run(u32 reboot)
{
  // fake flash begin immediately followed by flash end
  s32 status = ESP8266_FW_FlashBegin(0, 0);
  status |= ESP8266_FW_FlashFinish(reboot);
  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Read MAC from OTP ROM
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_ReadMac(u8 mac[6])
{
  u32 mac0, mac1, mac3;
  if( ESP8266_FW_ReadReg(ESP_OTP_MAC0, &mac0) >= 0 &&
      ESP8266_FW_ReadReg(ESP_OTP_MAC1, &mac1) >= 0 &&
      ESP8266_FW_ReadReg(ESP_OTP_MAC3, &mac3) >= 0 ) {

    if( mac3 != 0 ) {
      mac[0] = (mac3 >> 16) & 0xff;
      mac[1] = (mac3 >> 8) & 0xff;
      mac[2] = mac3 & 0xff;
    } else if( ((mac1 >> 16) & 0xff) == 0 ) {
      mac[0] = 0x18;
      mac[1] = 0xfe;
      mac[2] = 0x34;
    } else if( ((mac1 >> 16) & 0xff) == 1 ) {
      mac[0] = 0xac;
      mac[1] = 0xd0;
      mac[2] = 0x74;
    } else {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_ReadMac] unknown OUI!\n");
#endif
      return -2;
    }

    mac[3] = (mac1 >> 8) & 0xff;
    mac[4] = mac1 & 0xff;
    mac[5] = (mac0 >> 24) & 0xff;
  } else {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_ReadMac] failed to read MAC registers\n");
#endif
      return -1;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Read ChipID from OTP ROM
//! see http://esp8266-re.foogod.com/wiki/System_get_chip_id_%28IoT_RTOS_SDK_0.9.9%29
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_ReadChipId(u32 *chip_id)
{
  u32 mac0, mac1;
  if( ESP8266_FW_ReadReg(ESP_OTP_MAC0, &mac0) >= 0 &&
      ESP8266_FW_ReadReg(ESP_OTP_MAC1, &mac1) >= 0 ) {

    *chip_id = (mac0 >> 24) | ((mac1 & 0xffffff) << 8);
  } else {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_ReadChipId] failed to read MAC registers\n");
#endif
      return -1;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Read Flash ID
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_ReadFlashId(u32 *flash_id)
{
  if( ESP8266_FW_FlashBegin(0, 0) >= 0 &&
      ESP8266_FW_WriteReg(0x60000240, 0x00000000, ~0, 0) >= 0 &&
      ESP8266_FW_WriteReg(0x60000200, 0x10000000, ~0, 0) >= 0 &&
      ESP8266_FW_ReadReg(0x60000240, flash_id) >= 0 ) {
    // success

    // Disabled: this would reboot the FW (regardless if reboot flag is set or not...)
    // ESP8266_FW_FlashFinish(0);
  } else {
#if DEBUG_VERBOSE_LEVEL >= 2
      DEBUG_MSG("[ESP8266_FW_ReadChipId] failed to read MAC registers\n");
#endif
      return -1;
  }

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Executes the ESP8266 firmware
//! \param[in] cmd the command which should be executed
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 ESP8266_FW_Exec(esp8266_fw_exec_cmd_t exec_cmd)
{
  u8 dev_uart = ESP8266_UartGet();
  if( !dev_uart )
    return -1; // no UART configured

  u8 uart = dev_uart & 0xf;

  // disable terminal so that we can directly receive messages
  ESP8266_TerminalModeSet(0);

  // allow to send debug message
  ESP8266_MUTEX_MIDIOUT_TAKE;

  DEBUG_MSG("Connecting to ESP8266 in BootROM mode\n");

  int fail_ctr = 0;
  {
    int i;

    for(i=0; i<4; ++i) {
      DEBUG_MSG("Sync #%d", i+1);

      // flush receive buffer
      timeout_ctr = 100; // wait for up to 100 uS
      while( timeout_ctr )
	MIOS32_UART_RxBufferGet(uart);

      // send Sync command
      s32 status = ESP8266_FW_Sync();
      if( status < 0 ) {
	++fail_ctr;
	if( i == 0 )
	  DEBUG_MSG("Sync failed with status=%d, this can happen during first try - don't panic, we try again!\n", status);
	else
	  DEBUG_MSG("ERROR: Sync failed with status=%d\n", status);
      }
    }
  }

  if( fail_ctr >= 2 ) {
    DEBUG_MSG("ERROR: More than 2 fails - connection error!\n");
    DEBUG_MSG("Probably the UART bootloader isn't running!\n");
    DEBUG_MSG("Please:\n");
    DEBUG_MSG("  - set the ESP8266 GPIO0 pin to ground\n");
    DEBUG_MSG("  - trigger ESP8266 RST pin (shortly connect ground)\n");
    DEBUG_MSG("  - disconnect the ESP8266 GPIO0 pin from ground\n");
    DEBUG_MSG("Then try again.\n");
  } else {
    fail_ctr = 0;

    // Show MAC
    {
      u8 mac[6];
      if( ESP8266_FW_ReadMac(mac) < 0 ) {
	++fail_ctr;
	DEBUG_MSG("ERROR: failed to determine MAC!\n");
      } else {
	DEBUG_MSG("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
		  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      }
    }

    // Show ChipID
    {
      u32 chip_id;
      if( ESP8266_FW_ReadChipId(&chip_id) < 0 ) {
	++fail_ctr;
	DEBUG_MSG("ERROR: failed to determine Chip ID!\n");
      } else {
	DEBUG_MSG("Chip ID: 0x%08X\n", chip_id);
      }
    }

    // Show Flash Manufacturer and Device ID
    {
      u32 flash_id;
      if( ESP8266_FW_ReadFlashId(&flash_id) < 0 ) {
	++fail_ctr;
	DEBUG_MSG("ERROR: failed to determine Flash ID!\n");
      } else {
	DEBUG_MSG("Flash ID: 0x%08X\n", flash_id);
	DEBUG_MSG("Flash Size: %dk\n", (1 << (flash_id >> 16)) / 1024);
      }
    }
  }


  switch( exec_cmd ) {
  case ESP8266_FW_EXEC_CMD_QUERY: {
    if( fail_ctr ) {
      DEBUG_MSG("ERROR: bootloader communication wasn't successfull!\n");
    } else {
      DEBUG_MSG("Bootloader communication successfull!\n");
    }
  } break;

  case ESP8266_FW_EXEC_CMD_ERASE_FLASH: {
    if( fail_ctr ) {
      DEBUG_MSG("ERROR: due to communication failures the flash won't be erased!\n");
    } else {
      DEBUG_MSG("Erasing flash...\n");
      if( ESP8266_FW_FlashErase() < 0 ) {
	DEBUG_MSG("ERROR: flash erase failed!\n");
      } else {
	DEBUG_MSG("Wait some seconds, thereafter reset the ESP8266 device in bootloader mode!\n");
      }
    }
  } break;

  case ESP8266_FW_EXEC_CMD_PROG_FLASH: {
    if( fail_ctr ) {
      DEBUG_MSG("ERROR: due to communication failures the flash won't be programmed!\n");
    } else {
      DEBUG_MSG("Programming firmware...\n");

      if( ESP8266_FW_FlashProg((u8 *)esp8266_fw_img_0x00000, sizeof(esp8266_fw_img_0x00000), 0x00000) < 0 ) {
	DEBUG_MSG("ERROR: failed to program fw_img_0x00000!\n");
#if ESP8266_FW_FIRMWARE_CONFIG_BASE_ADDRESS > 0
      } else if( ESP8266_FW_FlashProg((u8 *)esp8266_fw_img_config, sizeof(esp8266_fw_img_config), ESP8266_FW_FIRMWARE_CONFIG_BASE_ADDRESS) < 0 ) {
	DEBUG_MSG("ERROR: failed to program fw_img_config!\n");
#endif
#if ESP8266_FW_FIRMWARE_UPPER_BASE_ADDRESS > 0
      } else if( ESP8266_FW_FlashProg((u8 *)esp8266_fw_img_upper, sizeof(esp8266_fw_img_upper), ESP8266_FW_FIRMWARE_UPPER_BASE_ADDRESS) < 0 ) {
	DEBUG_MSG("ERROR: failed to program fw_img_upper!\n");
#endif
      } else {
	DEBUG_MSG("Programming done...\n");
	DEBUG_MSG("Booting new firmware...\n");
	ESP8266_FW_FlashFinish(1);
      }
    }
  } break;

  default:
    DEBUG_MSG("ERROR: unexpected EXEC_CMD: %d\n", exec_cmd);
  }

  ESP8266_MUTEX_MIDIOUT_GIVE;

  // enable terminal again
  ESP8266_TerminalModeSet(1);

  return fail_ctr ? -2 : 0;
}

#endif /* ESP8266_FW_ACCESS_ENABLED */

//! \}
