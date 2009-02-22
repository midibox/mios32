// $Id$
/*
 * Access functions to network device
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#include <mios32.h>
#include "uip.h"
#include "network-device.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 0


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static u8 netdev_available;


/////////////////////////////////////////////////////////////////////////////
// Network Device Functions
/////////////////////////////////////////////////////////////////////////////

void network_device_init(void)
{
  s32 status;

  status = MIOS32_ENC28J60_Init(0);
  netdev_available = (status >= 0);
#if DEBUG_VERBOSE_LEVEL >= 1
  MIOS32_MIDI_SendDebugMessage("[network_device_init] status %d, available: %d\n", status, netdev_available);
#endif
}

void network_device_check(void)
{
  u8 prev_netdev_available = netdev_available;
  netdev_available = MIOS32_ENC28J60_CheckAvailable(prev_netdev_available);

  if( netdev_available && !prev_netdev_available ) {
    MIOS32_MIDI_SendDebugMessage("[network_device_check] ENC28J60 has been connected\n");
  } else if( !netdev_available && prev_netdev_available ) {
    MIOS32_MIDI_SendDebugMessage("[network_device_check] ENC28J60 has been disconnected\n");
  }
}

int network_device_available(void)
{
  return netdev_available;
}

int network_device_read(void)
{
  s32 status;

  if( (status=MIOS32_ENC28J60_PackageReceive((u8 *)uip_buf, UIP_BUFSIZE)) < 0 ) {
    netdev_available = 0;
#if DEBUG_VERBOSE_LEVEL >= 1
    MIOS32_MIDI_SendDebugMessage("[network_device_read] ERROR %d\n", status);
#endif
    return 0;
  }

#if DEBUG_VERBOSE_LEVEL >= 2
  if( status ) {
    MIOS32_MIDI_SendDebugMessage("[network_device_read] received %d bytes\n", status);
  }
#endif

  return status;
}

void network_device_send(void)
{
  u16 header_len = UIP_LLH_LEN + UIP_TCPIP_HLEN;
  s32 status = MIOS32_ENC28J60_PackageSend((u8 *)uip_buf, header_len,
					   (u8 *)uip_appdata, (uip_len > header_len) ? (uip_len-header_len) : 0);

  if( status < 0 ) {
    netdev_available = 0;
#if DEBUG_VERBOSE_LEVEL >= 1
    MIOS32_MIDI_SendDebugMessage("[network_device_send] ERROR %d\n", status);
#endif
  } else {
#if DEBUG_VERBOSE_LEVEL >= 2
    if( status ) {
      MIOS32_MIDI_SendDebugMessage("[network_device_send] sent %d bytes\n", uip_len);
    }
#endif
  }
}
