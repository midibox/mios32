// $Id$
/*
 * Access functions to network device
 *
 */

#include <mios32.h>
#include <string.h>
#include "uip.h"
#include "network-device.h"
#include "lpc17xx_emac.h"


/////////////////////////////////////////////////////////////////////////////
// for optional debugging messages via DEBUG_MSG (defined in mios32_config.h)
/////////////////////////////////////////////////////////////////////////////

#define DEBUG_VERBOSE_LEVEL 1


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static u8 netdev_available;

static u8 mac_addr[6] = {
  MIOS32_ENC28J60_MY_MAC_ADDR1, MIOS32_ENC28J60_MY_MAC_ADDR2, MIOS32_ENC28J60_MY_MAC_ADDR3,
  MIOS32_ENC28J60_MY_MAC_ADDR4, MIOS32_ENC28J60_MY_MAC_ADDR5, MIOS32_ENC28J60_MY_MAC_ADDR6
};


/////////////////////////////////////////////////////////////////////////////
// Network Device Functions
/////////////////////////////////////////////////////////////////////////////

void network_device_init(void)
{
  s32 status = 0;

  // MAC Address: check for all-zero, derive MAC from serial number in this case
  int i;
  int ored = 0;
  for(i=0; i<6; ++i)
    ored |= mac_addr[i];

  if( ored == 0 ) {
    // get serial number
    char serial[32];
    MIOS32_SYS_SerialNumberGet(serial);
    int len = strlen(serial);
    if( len < 12 ) {
      // no serial number or not large enough - we should at least set the MAC address to != 0
      for(i=0; i<6; ++i)
	mac_addr[i] = i;
    } else {
#if 0     
      for(i=0; i<6; ++i) {
	// convert hex string to dec
	char digitl = serial[len-i*2 - 1];
	char digith = serial[len-i*2 - 2];
	mac_addr[i] = ((digitl >= 'A') ? (digitl-'A'+10) : (digitl-'0')) |
	  (((digith >= 'A') ? (digith-'A'+10) : (digith-'0')) << 4);
#else
      // TK: for some reasons, my Fritzbox router doesn't accept MACs that are not starting with 0x00
      mac_addr[0] = 0x00;
      for(i=1; i<6; ++i) {
	// convert hex string to dec
	char digitl = serial[len-(i-1)*2 - 1];
	char digith = serial[len-(i-1)*2 - 2];
	mac_addr[i] = ((digitl >= 'A') ? (digitl-'A'+10) : (digitl-'0')) |
	  (((digith >= 'A') ? (digith-'A'+10) : (digith-'0')) << 4);
#endif
      }
    }
  }

  if( !EMAC_Init((uint8_t *)mac_addr) != SUCCESS )
    status = -1;

  netdev_available = (status >= 0);
#if DEBUG_VERBOSE_LEVEL >= 1
  MIOS32_MIDI_SendDebugMessage("[network_device_init] status %d, available: %d\n", status, netdev_available);
#endif
}

void network_device_check(void)
{
  // hold initial netdev_available status
}

int network_device_available(void)
{
  return netdev_available;
}

int network_device_read(void)
{
  s32 status = 0;

  status = EMAC_ReadPacket(uip_buf);

#if DEBUG_VERBOSE_LEVEL >= 2
  if( status > 0 ) {
    MIOS32_MIDI_SendDebugMessage("[network_device_read] received %d bytes\n", status);
  }
#endif

#if DEBUG_VERBOSE_LEVEL >= 3
  if( status ) {
    MIOS32_MIDI_SendDebugHexDump((u8 *)uip_buf, status);
  }
#endif

  return status;
}

void network_device_send(void)
{
  u16 header_len = UIP_LLH_LEN + UIP_TCPIP_HLEN;
  s32 status;

  do {
    status = EMAC_SendPacket((u8 *)uip_buf, (uip_len >= header_len) ? header_len : uip_len,
			     (u8 *)uip_appdata, (uip_len > header_len) ? (uip_len-header_len) : 0);

#if DEBUG_VERBOSE_LEVEL >= 1
    if( status == 0 ) {
      MIOS32_MIDI_SendDebugMessage("[network_device_send] retry\n");
    }
#endif
  } while( status == 0 );

#if DEBUG_VERBOSE_LEVEL >= 2
  if( status > 0 ) {
    MIOS32_MIDI_SendDebugMessage("[network_device_send] sent %d bytes\n", uip_len);
  }
#endif
}


unsigned char *network_device_mac_addr(void)
{
  return (unsigned char *)mac_addr;
}
