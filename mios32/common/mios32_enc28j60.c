// $Id$
//! \defgroup MIOS32_ENC28J60
//!
//! ENC28J60 is accessed via SPI1 (J16) or alternatively via SPI2 (J8/9)
//! A bitbanging solution (for using alternative ports) will be provided later.
//!
//! The chip has to be supplied with 3.3V!
//!
//! MIOS32_ENC28J60_Init(0) has to be called only once to initialize the driver.
//!
//! MIOS32_ENC28J60_CheckAvailable() should be called to connect with the device.
//! If 0 is returned, it can be assumed that no ENC28J60 chip is connected. The 
//! function can be called periodically from a low priority task to retry a 
//! connection, resp. for an auto-detection during runtime
//!
//! Most parts of the driver have been taken from ENC28J60.c file of the
//! Microchip TCP/IP stack, and adapted to MIOS32 routines.
//! 
//! philetaylor (07 Jan 10): I have added mutex support. As many functions
//! are not used externally they rely on the mutex being taked/given by the
//! calling function. Please bear this in mind if calling functions directly.
//! Only MIOS32_ENC28J60_Init(), MIOS32_ENC28J60_CheckAvailable() 
//! MIOS32_ENC28J60_PackageSend() and MIOS32_ENC28J60_PackageReceive()
//! Will handle mutexes themselves all other functions will not.
//!
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
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

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_ENC28J60)

#include <mios32_enc28j60_regs.h>

#if !defined(MIOS32_ENC28J60_MUTEX_TAKE)
#define MIOS32_ENC28J60_MUTEX_TAKE
#define MIOS32_ENC28J60_MUTEX_GIVE
#endif


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// ENC28J60 Opcodes (to be ORed with a 5 bit address)
#define WCR (0x2<<5)          // Write Control Register command
#define BFS (0x4<<5)          // Bit Field Set command
#define BFC (0x5<<5)          // Bit Field Clear command
#define RCR (0x0<<5)          // Read Control Register command
#define RBM ((0x1<<5) | 0x1A) // Read Buffer Memory command
#define WBM ((0x3<<5) | 0x1A) // Write Buffer Memory command
#define SR  ((0x7<<5) | 0x1F) // System Reset command does not use an address.  


// macros to select/deselect device
#define CSN_0 { MIOS32_SPI_RC_PinSet(MIOS32_ENC28J60_SPI, MIOS32_ENC28J60_SPI_RC_PIN, 0); }
#define CSN_1 { MIOS32_SPI_RC_PinSet(MIOS32_ENC28J60_SPI, MIOS32_ENC28J60_SPI_RC_PIN, 1); }


// /* RX & TX memory organization */
#define RAMSIZE 0x2000          
#define RXSTART 0x0000 // should be an even memory address, must be 0 for errata
#define RXEND   0x0FFF // odd for errata workaround
#define TXSTART 0x1000
#define TXEND   0x1fff
#define RXSTOP  ((TXSTART - 2) | 0x0001) // odd for errata workaround
#define RXSIZE  (RXSTOP - RXSTART + 1)


/////////////////////////////////////////////////////////////////////////////
// Local type definitions
/////////////////////////////////////////////////////////////////////////////

// A header appended at the start of all RX frames by the hardware
typedef struct __attribute__ ((packed)) // __attribute__((aligned(2), packed))
{
  u16       NextPacketPointer;
  RXSTATUS  StatusVector;

  // not used - forwarded to UIP anyhow
  //  MAC_ADDR        DestMACAddr;
  //  MAC_ADDR        SourceMACAddr;
  //  WORD_VAL        Type;
} ENC_PREAMBLE;


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u8 WasDiscarded;
static u16 NextPacketLocation;

static u8 rev_id;

static u8 mac_addr[6];


/////////////////////////////////////////////////////////////////////////////
//! Initializes SPI pins and peripheral to access ENC28J60
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_Init(u32 mode)
{
  s32 ret;
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // set default mac address
  // if all-0, 
  u8 default_mac_addr[6] = {
    MIOS32_ENC28J60_MY_MAC_ADDR1, MIOS32_ENC28J60_MY_MAC_ADDR2, MIOS32_ENC28J60_MY_MAC_ADDR3,
    MIOS32_ENC28J60_MY_MAC_ADDR4, MIOS32_ENC28J60_MY_MAC_ADDR5, MIOS32_ENC28J60_MY_MAC_ADDR6
  };
  memcpy(mac_addr, default_mac_addr, 6);
  MIOS32_ENC28J60_MUTEX_TAKE
  ret=MIOS32_ENC28J60_PowerOn();
  MIOS32_ENC28J60_MUTEX_GIVE
  return ret;
  
}


/////////////////////////////////////////////////////////////////////////////
//! Connects to ENC28J60 chip
//! \return < 0 if initialisation sequence failed
//! \return -16 if clock not ready after system reset
//! \return -17 if unsupported revision ID
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_PowerOn(void)
{
  s32 status;

  
  // deactivate chip select
  CSN_1;

  // ensure that fast pin drivers are activated
  MIOS32_SPI_IO_Init(MIOS32_ENC28J60_SPI, MIOS32_SPI_PIN_DRIVER_STRONG);

  // init SPI port for fast frequency access (ca. 18 MBit/s)
  if( (status=MIOS32_SPI_TransferModeInit(MIOS32_ENC28J60_SPI, MIOS32_SPI_MODE_CLK0_PHASE0, MIOS32_SPI_PRESCALER_4)) < 0 ) 
    return status;
  
  // send system reset command

  // RESET the entire ENC28J60, clearing all registers
  // Also wait for CLKRDY to become set.
  // Bit 3 in ESTAT is an unimplemented bit.  If it reads out as '1' that
  // means the part is in RESET or there is something wrong with the SPI 
  // connection.  This routine makes sure that we can communicate with the 
  // ENC28J60 before proceeding.
  if( (status=MIOS32_ENC28J60_SendSystemReset()) < 0 )
    return status;

  // check if chip is accessible (it should after ca. 1 mS)
  if( (status=MIOS32_ENC28J60_ReadETHReg(ESTAT)) < 0 )
    return status;

  if( (status & 0x08) || !(status & ESTAT_CLKRDY) )
    return -16; // no access to chip

  // read and check revision ID, this gives us another check if device is available
  MIOS32_ENC28J60_BankSel(EREVID);
  if( (status=MIOS32_ENC28J60_ReadMACReg((u8)EREVID)) < 0 )
    return status;

  rev_id = status;

  if( !rev_id || rev_id >= 32 ) {
    return -17; // unsupported revision ID
  }

  // Start up in Bank 0 and configure the receive buffer boundary pointers 
  // and the buffer write protect pointer (receive buffer read pointer)
  WasDiscarded = 1;
  NextPacketLocation = RXSTART;

  MIOS32_ENC28J60_BankSel(ERXSTL);
  status |= MIOS32_ENC28J60_WriteReg(ERXSTL,   (RXSTART) & 0xff);
  status |= MIOS32_ENC28J60_WriteReg(ERXSTH,   ((RXSTART) >> 8) & 0xff);
  status |= MIOS32_ENC28J60_WriteReg(ERXRDPTL, (RXSTOP) & 0xff);
  status |= MIOS32_ENC28J60_WriteReg(ERXRDPTH, ((RXSTOP) >> 8) & 0xff);
  status |= MIOS32_ENC28J60_WriteReg(ERXNDL,   (RXSTOP) & 0xff);
  status |= MIOS32_ENC28J60_WriteReg(ERXNDH,   ((RXSTOP) >> 8) & 0xff);
  status |= MIOS32_ENC28J60_WriteReg(ETXSTL,   (TXSTART) & 0xff);
  status |= MIOS32_ENC28J60_WriteReg(ETXSTH,   ((TXSTART) >> 8) & 0xff);

  // Write a permanant per packet control byte of 0x00
  status |= MIOS32_ENC28J60_WriteReg(EWRPTL,   (TXSTART) & 0xff);
  status |= MIOS32_ENC28J60_WriteReg(EWRPTH,   ((TXSTART) >> 8) & 0xff);
  status |= MIOS32_ENC28J60_MACPut(0x00);

  // Enter Bank 1 and configure Receive Filters 
  // (No need to reconfigure - Unicast OR Broadcast with CRC checking is 
  // acceptable)
  // Write ERXFCON_CRCEN only to ERXFCON to enter promiscuous mode

  // Promiscious mode example:
  // status |= MIOS32_ENC28J60_BankSel(ERXFCON);
  // status |= MIOS32_ENC28J60_WriteReg((u8)ERXFCON, ERXFCON_CRCEN);
        
  // Enter Bank 2 and configure the MAC
  status |= MIOS32_ENC28J60_BankSel(MACON1);

  // Enable the receive portion of the MAC
  status |= MIOS32_ENC28J60_WriteReg((u8)MACON1, MACON1_TXPAUS | MACON1_RXPAUS | MACON1_MARXEN);
        
  // Pad packets to 60 bytes, add CRC, and check Type/Length field.
#if MIOS32_ENC28J60_FULL_DUPLEX
  status |= MIOS32_ENC28J60_WriteReg((u8)MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN | MACON3_FULDPX);
  status |= MIOS32_ENC28J60_WriteReg((u8)MABBIPG, 0x15);  
#else
  status |= MIOS32_ENC28J60_WriteReg((u8)MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN);
  status |= MIOS32_ENC28J60_WriteReg((u8)MABBIPG, 0x12);  
#endif

  // Allow infinite deferals if the medium is continuously busy 
  // (do not time out a transmission if the half duplex medium is 
  // completely saturated with other people's data)
  status |= MIOS32_ENC28J60_WriteReg((u8)MACON4, MACON4_DEFER);

  // Late collisions occur beyond 63+8 bytes (8 bytes for preamble/start of frame delimiter)
  // 55 is all that is needed for IEEE 802.3, but ENC28J60 B5 errata for improper link pulse 
  // collisions will occur less often with a larger number.
  status |= MIOS32_ENC28J60_WriteReg((u8)MACLCON2, 63);
        
  // Set non-back-to-back inter-packet gap to 9.6us.  The back-to-back 
  // inter-packet gap (MABBIPG) is set by MACSetDuplex() which is called 
  // later.
  status |= MIOS32_ENC28J60_WriteReg((u8)MAIPGL, 0x12);
  status |= MIOS32_ENC28J60_WriteReg((u8)MAIPGH, 0x0C);

  // Set the maximum packet size which the controller will accept
  status |= MIOS32_ENC28J60_WriteReg((u8)MAMXFLL, (MIOS32_ENC28J60_MAX_FRAME_SIZE) & 0xff);
  status |= MIOS32_ENC28J60_WriteReg((u8)MAMXFLH, ((MIOS32_ENC28J60_MAX_FRAME_SIZE) >> 8) & 0xff);
        
  // initialize physical MAC address registers
  status |= MIOS32_ENC28J60_MAC_AddrSet(mac_addr);

  // Enter Bank 3 and Disable the CLKOUT output to reduce EMI generation
  status |= MIOS32_ENC28J60_BankSel(ECOCON);
  status |= MIOS32_ENC28J60_WriteReg((u8)ECOCON, 0x00);   // Output off (0V)
  //status |= MIOS32_ENC28J60_WriteReg((u8)ECOCON, 0x01); // 25.000MHz
  //status |= MIOS32_ENC28J60_WriteReg((u8)ECOCON, 0x03); // 8.3333MHz (*4 with PLL is 33.3333MHz)

  // Disable half duplex loopback in PHY.  Bank bits changed to Bank 2 as a 
  // side effect.
  status |= MIOS32_ENC28J60_WritePHYReg(PHCON2, PHCON2_HDLDIS);

  // Configure LEDA to display LINK status, LEDB to display TX/RX activity
  status |= MIOS32_ENC28J60_WritePHYReg(PHLCON, 0x3472);

  // Set the MAC and PHY into the proper duplex state
#if MIOS32_ENC28J60_FULL_DUPLEX
  status |= MIOS32_ENC28J60_WritePHYReg(PHCON1, PHCON1_PDPXMD);
#else
  status |= MIOS32_ENC28J60_WritePHYReg(PHCON1, 0x0000);
#endif
  status |= MIOS32_ENC28J60_BankSel(ERDPTL);                // Return to default Bank 0

  // Enable packet reception
  status |= MIOS32_ENC28J60_BFSReg(ECON1, ECON1_RXEN);

  return (status < 0) ? status : 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Disconnects from ENC28J60
//! \return < 0 on errors
//! \todo not implemented yet
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_PowerOff(void)
{
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Checks if the chip is still connected by reading the revision ID.<BR>
//! This takes ca. 2 uS
//!
//! If the ID value is within the range of 0x01..0x1f, MIOS32_ENC28J60_PowerOn()
//! will be called by this function to initialize the device completely.
//!
//! Example for Connection/Disconnection detection:
//! \code
//! // this function is called each second from a low-priority task
//! // If multiple tasks are accessing the ENC28J60 chip, add a semaphore/mutex
//! //  to avoid IO access collisions with other tasks!
//! u8 enc28j60_available;
//! s32 ETHERNET_CheckENC28J60(void)
//! {
//!   // check if ENC28J60 is connected
//!   u8 prev_enc28j60_available = enc28j60_available;
//!   enc28j60_available = MIOS32_ENC28J60_CheckAvailable(prev_enc28j60_available);
//! 
//!   if( enc28j60_available && !prev_enc28j60_available ) {
//!     // ENC28J60 has been connected
//! 
//!     // now it's possible to receive/send packages
//! 
//!   } else if( !enc28j60_available && prev_enc28j60_available ) {
//!     // ENC28J60 has been disconnected
//! 
//!     // here you can notify your application about this state
//!   }
//! 
//!   return 0; // no error
//! }
//! \endcode
//! \param[in] was_available should only be set if the ENC28J60 was previously available
//! \return 0 if no response from ENC28J60
//! \return 1 if ENC28J60 is accessible
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_CheckAvailable(u8 was_available)
{
  s32 status = 0;
  MIOS32_ENC28J60_MUTEX_TAKE
  if( (status=MIOS32_SPI_TransferModeInit(MIOS32_ENC28J60_SPI, MIOS32_SPI_MODE_CLK0_PHASE0, MIOS32_SPI_PRESCALER_4)) < 0 ) {
    MIOS32_ENC28J60_MUTEX_GIVE
    return 0;
  }
  
  // read revision ID to check if ENC28J60 is connected
  MIOS32_ENC28J60_BankSel(EREVID);
  if( (status=MIOS32_ENC28J60_ReadMACReg((u8)EREVID)) < 0 ) {
    MIOS32_ENC28J60_MUTEX_GIVE
    return 0;
  }

  // chip not connected if value < 1 or >= 32
  if( !status || status >= 32 ) {
    MIOS32_ENC28J60_MUTEX_GIVE
    return 0;
  }
  // initialize chip if it has been detected
  if( !was_available ) {
    // run power-on sequence
    if( MIOS32_ENC28J60_PowerOn() < 0 ) { 
      MIOS32_ENC28J60_MUTEX_GIVE
  	  return 0; // ENC28J60 not available anymore
	}
  }
  
  MIOS32_ENC28J60_MUTEX_GIVE
  return 1; // ENC28J60 available
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the PHSTAT1.LLSTAT bit (ethernet cable connected)
//! \return 0 if ethernet cable not connected
//! \return 1 if ethernet cable connected and link available
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_LinkAvailable(void)
{
  s32 status = MIOS32_ENC28J60_ReadPHYReg(PHSTAT1);

  if( status < 0 )
    return 0; // e.g. ENC28J60 not connected

  return (status & PHSTAT1_LLSTAT) ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
//! Returns ENC28J60 Revision ID<BR>
//! Value is only valid if MIOS32_ENC28J60_PowerOn() passed without errors.
//! \return revision id (8-bit value)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_RevIDGet(void)
{
  return rev_id;
}


/////////////////////////////////////////////////////////////////////////////
//! Changes the MAC address.
//!
//! Usually called by MIOS32_ENC28J60_PowerOn(), but could also be used
//! during runtime.
//!
//! The default MAC address is predefined by MIOS32_ENC28J60_MY_MAC_ADDR[123456]
//! and can be overruled in mios32_config.h if desired.
//!
//! \param[in] new_mac_addr an array with 6 bytes which define the MAC
//! address. If all bytes are 0 (default), the serial number of STM32 will be
//! taken instead, which should be unique in your private network.
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_MAC_AddrSet(u8 new_mac_addr[6])
{
  s32 status;

  // re-init SPI port for fast frequency access (ca. 18 MBit/s)
  // this is required for the case that the SPI port is shared with other devices
  if( (status=MIOS32_SPI_TransferModeInit(MIOS32_ENC28J60_SPI, MIOS32_SPI_MODE_CLK0_PHASE0, MIOS32_SPI_PRESCALER_4)) < 0 )
    return status;

  // check for all-zero
  int i;
  int ored = 0;
  for(i=0; i<6; ++i)
    ored |= new_mac_addr[i];

  if( ored ) {
    // MAC address != 0 -> take over new address
    memcpy(mac_addr, new_mac_addr, 6);
  } else {
    // get serial number
    char serial[32];
    MIOS32_SYS_SerialNumberGet(serial);
    int len = strlen(serial);
    if( len < 12 ) {
      // no serial number or not large enough - we should at least set the MAC address to != 0
      for(i=0; i<6; ++i)
	mac_addr[i] = i;
    } else {
      for(i=0; i<6; ++i) {
	// convert hex string to dec
	char digitl = serial[len-i*2 - 1];
	char digith = serial[len-i*2 - 2];
	mac_addr[i] = ((digitl >= 'A') ? (digitl-'A'+10) : (digitl-'0')) |
	  (((digith >= 'A') ? (digith-'A'+10) : (digith-'0')) << 4);
      }
    }
  }

  status |= MIOS32_ENC28J60_BankSel(MAADR1);
  status |= MIOS32_ENC28J60_WriteReg((u8)MAADR1, mac_addr[0]);
  status |= MIOS32_ENC28J60_WriteReg((u8)MAADR2, mac_addr[1]);
  status |= MIOS32_ENC28J60_WriteReg((u8)MAADR3, mac_addr[2]);
  status |= MIOS32_ENC28J60_WriteReg((u8)MAADR4, mac_addr[3]);
  status |= MIOS32_ENC28J60_WriteReg((u8)MAADR5, mac_addr[4]);
  status |= MIOS32_ENC28J60_WriteReg((u8)MAADR6, mac_addr[5]);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the current MAC address
//! \return u8 pointer with 6 bytes
/////////////////////////////////////////////////////////////////////////////
u8 *MIOS32_ENC28J60_MAC_AddrGet(void)
{
  return mac_addr;
}


/////////////////////////////////////////////////////////////////////////////
//! Sends a package to the ENC28J60 chip
//! param[in] buffer Pointer to buffer which contains the playload
//! param[in] len number of bytes which should be sent
//! param[in] buffer2 Pointer to optional second buffer which contains additional playload
//!           (can be NULL if no additional data)
//! param[in] len2 number of bytes in second buffer
//!           (should be 0 if no additional data)
//! \return < 0 on errors
//! \return -16 if previous package hasn't been sent yet (this is checked 
//!             1000 times before the function exits)
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_PackageSend(u8 *buffer, u16 len, u8 *buffer2, u16 len2)
{
  s32 status = 0;

  MIOS32_ENC28J60_MUTEX_TAKE

  // re-init SPI port for fast frequency access (ca. 18 MBit/s)
  // this is required for the case that the SPI port is shared with other devices
  if( (status=MIOS32_SPI_TransferModeInit(MIOS32_ENC28J60_SPI, MIOS32_SPI_MODE_CLK0_PHASE0, MIOS32_SPI_PRESCALER_4)) < 0 ) {
    MIOS32_ENC28J60_MUTEX_GIVE
    return status;
  }
  
  // wait until a new package can be transmitted
  int timeout_ctr = 1000;
  while( --timeout_ctr > 0 ) {
    status = MIOS32_ENC28J60_ReadETHReg(ECON1);
    if( status < 0 ) {
      MIOS32_ENC28J60_MUTEX_GIVE
      return -1;
	}
    if( !(status & ECON1_TXRTS) )
      break;
  }

  if( timeout_ctr == 0 ) {
    MIOS32_ENC28J60_MUTEX_GIVE
    return -16;
  }
  // Set the SPI write pointer to the beginning of the transmit buffer
  status |= MIOS32_ENC28J60_BankSel(EWRPTL);
  status |= MIOS32_ENC28J60_WriteReg(EWRPTL, TXSTART & 0xff);
  status |= MIOS32_ENC28J60_WriteReg(EWRPTH, (TXSTART >> 8) & 0xff);

  if( status < 0 ) {
    MIOS32_ENC28J60_MUTEX_GIVE
    return status;
  }
  // Calculate where to put the TXND pointer
  u16 end_addr = TXSTART + len + len2; // package control byte has already been considered in this calculation (+1 .. -1)

  // Write the TXND pointer into the registers, given the dataLen given
  status |= MIOS32_ENC28J60_WriteReg(ETXNDL, end_addr & 0xff);
  status |= MIOS32_ENC28J60_WriteReg(ETXNDH, (end_addr >> 8) & 0xff);

  if( status < 0 ) {
    MIOS32_ENC28J60_MUTEX_GIVE
    return status;
  }
	
  // per-packet control byte:
  status |= MIOS32_ENC28J60_MACPut(0x07); // enable CRC calculation and padding to 60 bytes

  // send buffer
  status |= MIOS32_ENC28J60_MACPutArray(buffer, len);

  // send second buffer if available
  if( buffer2 != NULL && len2 ) {
    status |= MIOS32_ENC28J60_MACPutArray(buffer2, len2);
  }

  // Reset transmit logic if a TX Error has previously occured
  // This is a silicon errata workaround
  status |= MIOS32_ENC28J60_BFSReg(ECON1, ECON1_TXRST);
  status |= MIOS32_ENC28J60_BFCReg(ECON1, ECON1_TXRST);
  status |= MIOS32_ENC28J60_BFCReg(EIR, EIR_TXERIF | EIR_TXIF);

  // Start the transmission
  status |= MIOS32_ENC28J60_BFSReg(ECON1, ECON1_TXRTS);

  if( status < 0 ) {
    MIOS32_ENC28J60_MUTEX_GIVE
    return status;
  }
  // We have finished with the mutex.
  MIOS32_ENC28J60_MUTEX_GIVE
	
  // Revision B5 and B7 silicon errata workaround
  if( rev_id == 0x05 || rev_id == 0x06 ) {
    // TODO --- add a lot of code here
  }

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Receives a package from ENC28J60 chip
//! param[in] buffer Pointer to buffer which gets the playload
//! param[in] buffer_size Max. number of bytes which can be received
//! \return < 0 on errors
//! \return -16 if inconsistencies have been detected, and the ENC28J60 device has been reseted
//! \return 0 if no package has been received
//! \return > 0 number of received bytes
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_PackageReceive(u8 *buffer, u16 buffer_size)
{
  s32 status = 0;
  s32 package_count;

  MIOS32_ENC28J60_MUTEX_TAKE

  // re-init SPI port for fast frequency access (ca. 18 MBit/s)
  // this is required for the case that the SPI port is shared with other devices
  if( (status=MIOS32_SPI_TransferModeInit(MIOS32_ENC28J60_SPI, MIOS32_SPI_MODE_CLK0_PHASE0, MIOS32_SPI_PRESCALER_4)) < 0 ) {
  	MIOS32_ENC28J60_MUTEX_GIVE
    return status;
  }
  // Test if at least one packet has been received and is waiting
  status |= MIOS32_ENC28J60_BankSel(EPKTCNT);
  package_count = MIOS32_ENC28J60_ReadETHReg((u8)EPKTCNT);
  status |= MIOS32_ENC28J60_BankSel(ERDPTL);

  if( status < 0 ) {
	MIOS32_ENC28J60_MUTEX_GIVE
    return status;
  }
  if( package_count <= 0 ) {
  	 MIOS32_ENC28J60_MUTEX_GIVE
    return package_count;
  }
  // Make absolutely certain that any previous packet was discarded
  if( !WasDiscarded ) {
    status = MIOS32_ENC28J60_MACDiscardRx();
 	MIOS32_ENC28J60_MUTEX_GIVE
    return (status < 0) ? status : 0;
  }

  // Set the SPI read pointer to the beginning of the next unprocessed packet
  u16 CurrentPacketLocation = NextPacketLocation;
  status |= MIOS32_ENC28J60_WriteReg(ERDPTL, CurrentPacketLocation & 0xff);
  status |= MIOS32_ENC28J60_WriteReg(ERDPTH, (CurrentPacketLocation >> 8) & 0xff);

  if( status < 0 ) {
	MIOS32_ENC28J60_MUTEX_GIVE
    return status;
  }
  
  // Obtain the MAC header from the Ethernet buffer
  ENC_PREAMBLE header;
  status |= MIOS32_ENC28J60_MACGetArray((u8 *)&header, sizeof(header));

  // Validate the data returned from the ENC28J60.  Random data corruption, 
  // such as if a single SPI bit error occurs while communicating or a 
  // momentary power glitch could cause this to occur in rare circumstances.
  if( header.NextPacketPointer > RXSTOP ||
      (header.NextPacketPointer & 1) ||
      header.StatusVector.bits.Zero ||
      header.StatusVector.bits.ByteCount > 1518u ) {

    MIOS32_MIDI_SendDebugMessage("[MIOS32_ENC28J60_PackageReceive] glitch detected - Ptr: %04x, Status: %04x %02x%02x\n",
				 header.NextPacketPointer,
				 header.StatusVector.bits.ByteCount,
				 header.StatusVector.v[3], header.StatusVector.v[2]);
    // Reset device (must keep mutex)
    MIOS32_ENC28J60_PowerOn();
    // no packet received
    MIOS32_ENC28J60_MUTEX_GIVE				 
    return -16; // notify this as an error

  }

  // Save the location where the hardware will write the next packet to
  NextPacketLocation = header.NextPacketPointer;

  // Mark this packet as discardable
  WasDiscarded = 0;

  // empty package or CRC/symbol errors?
  u16 packet_len = header.StatusVector.bits.ByteCount;
  if( !packet_len || header.StatusVector.bits.CRCError || !header.StatusVector.bits.ReceiveOk ) {
    status = MIOS32_ENC28J60_MACDiscardRx(); // discard package immediately
    MIOS32_ENC28J60_MUTEX_GIVE
    return (status < 0) ? status : 0;
  }

  // ensure that we don't read more bytes than the buffer can store
  if( packet_len > buffer_size )
    packet_len = buffer_size;

  // read bytes into buffer
  status = MIOS32_ENC28J60_MACGetArray(buffer, packet_len);

  // discard package immediately
  status |= MIOS32_ENC28J60_MACDiscardRx();
  
  // No more processing so can safely give mutex back.
  MIOS32_ENC28J60_MUTEX_GIVE

  if( status < 0 )
    return status;

  return packet_len; // no error: return packet length
}


/////////////////////////////////////////////////////////////////////////////
//! Marks the last received packet (obtained using MIOS32_ENC28J60_PackageReceive())
//! as being processed and frees the buffer memory associated with it
//! 
//! It is safe to call this function multiple times between MIOS32_ENC28J60_PackageReceive()
//! calls.  Extra packets won't be thrown away until MIOS32_ENC28J60_PackageReceive()
//! makes it available.
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_MACDiscardRx(void)
{
  s32 status = 0;

  // Make sure the current packet was not already discarded
  if( WasDiscarded )
    return 0;

  WasDiscarded = 1;
        
  // Decrement the next packet pointer before writing it into 
  // the ERXRDPT registers.  This is a silicon errata workaround.
  // RX buffer wrapping must be taken into account if the 
  // NextPacketLocation is precisely RXSTART.
  u16 NewRXRDLocation = NextPacketLocation - 1;
  if( NewRXRDLocation > RXSTOP )
    NewRXRDLocation = RXSTOP;

  // Decrement the RX packet counter register, EPKTCNT
  status |= MIOS32_ENC28J60_BFSReg(ECON2, ECON2_PKTDEC);

  // Move the receive read pointer to unwrite-protect the memory used by the 
  // last packet.  The writing order is important: set the low byte first, 
  // high byte last.
  status |= MIOS32_ENC28J60_BankSel(ERXRDPTL);
  status |= MIOS32_ENC28J60_WriteReg(ERXRDPTL, NewRXRDLocation & 0xff);
  status |= MIOS32_ENC28J60_WriteReg(ERXRDPTH, (NewRXRDLocation >> 8) & 0xff);

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Sends the 8 bit RCR opcode/Address byte over the SPI and then retrives 
//! the register contents in the next 8 SPI clocks.
//!
//! This routine cannot be used to access MAC/MII or PHY registers.
//! Use MIOS32_ENC28J60_ReadMACReg() or MIOS32_ENC28J60_ReadPHYReg() for that
//! purpose.
//! \param[in] address 5 bit address of the ETH control register to read from.
//! \return < 0 on errors
//! \return >= 0: Byte read from the Ethernet controller's ETH register.
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_ReadETHReg(u8 address)
{
  s32 status = 0;

  // send opcode
  CSN_0;

  status |= MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, RCR | address);

  // skip if something already failed
  if( status >= 0 ) {
    // read register
    status = MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, 0xff);
  }
  CSN_1;

  return status;
}

/////////////////////////////////////////////////////////////////////////////
//! Sends the 8 bit RCR opcode/Address byte as well as a dummy byte over the 
//! SPI and then retrives the register contents in the last 8 SPI clocks.
//!
//! This routine cannot be used to access ETH or PHY registers.
//! Use MIOS32_ENC28J60_ReadETHReg() or MIOS32_ENC28J60_ReadPHYReg() for that
//! purpose.
//! \param[in] address 5 bit address of the MAC or MII register to read from.
//! \return < 0 on errors
//! \return >= 0: Byte read from the Ethernet controller's MAC/MII register.
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_ReadMACReg(u8 address)
{
  s32 status = 0;

  // send opcode
  CSN_0;
  status |= MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, RCR | address);
  status |= MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, 0xff); // dummy byte

  if( status >= 0 ) {
    // read register
    status = MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, 0xff);
  }

  CSN_1;

  return status;
}

/////////////////////////////////////////////////////////////////////////////
//! Performs an MII read operation.  While in progress, it simply polls the 
//! MII BUSY bit wasting time (10.24us).
//!
//! This routine cannot be used to access ETH or PHY registers.
//! Use MIOS32_ENC28J60_ReadETHReg() or MIOS32_ENC28J60_ReadPHYReg() for that
//! purpose.
//! \param[in] reg Address of the PHY register to read from.
//! \return < 0 on errors
//! \return >= 0: u16 read from the Ethernet controller's MAC/MII register.
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_ReadPHYReg(u8 reg)
{
  s32 status = 0;

  // set the right bank to start register read operation
  status |= MIOS32_ENC28J60_BankSel(MIREGADR);
  status |= MIOS32_ENC28J60_WriteReg((u8)MIREGADR, reg);
  status |= MIOS32_ENC28J60_WriteReg((u8)MICMD, MICMD_MIIRD);

  // exit if something already failed
  if( status < 0 ) {
    MIOS32_ENC28J60_BankSel(ERDPTL); // return to bank 0
    return status;
  }

  // Loop to wait until the PHY register has been read through the MII
  // This requires 10.24us
  // TODO: add timeout
  status |= MIOS32_ENC28J60_BankSel(MISTAT);
  do {
    status = MIOS32_ENC28J60_ReadMACReg((u8)MISTAT);

    if( status < 0 ) {
      MIOS32_ENC28J60_BankSel(ERDPTL); // return to bank 0
      return status;
    }
  } while( status & MISTAT_BUSY );

  // stop reading
  status |= MIOS32_ENC28J60_BankSel(MIREGADR);
  status |= MIOS32_ENC28J60_WriteReg((u8)MICMD, 0x00);

  u16 hword = 0;
  status = MIOS32_ENC28J60_ReadMACReg((u8)MIRDL);
  if( status >= 0 )
    hword = status;
  status = MIOS32_ENC28J60_ReadMACReg((u8)MIRDL);
  if( status >= 0 )
    hword |= status << 8;

  // return to bank 0
  MIOS32_ENC28J60_BankSel(ERDPTL);

  // exit if error
  if( status < 0 )
    return status;

  // return read value
  return hword;
}


/////////////////////////////////////////////////////////////////////////////
//! Sends the 8 bit WCR opcode/Address byte over the SPI and then sends 
//! the data in the next 8 SPI clocks
//!
//! This routine is almost identical to the MIOS32_ENC28J60_BFCReg() and
//! MIOS32_ENC28J60_BFSReg() functions.  It is separate to maximize speed.
//!
//! Unlike the MIOS32_ENC28J60_ReadETHReg/MIOS32_ENC28J60_ReadMACReg functions, 
//! MIOS32_ENC28J60_WriteReg() can write to any ETH or MAC register.  Writing 
//! to PHY registers must be accomplished with MIOS32_ENC28J60_WritePHYReg().
//! \param[in] address 5 bit address of the ETH, MAC, or MII register to modify.
//!            The top 3 bits must be 0
//! \param[in] data Byte to be written into register
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_WriteReg(u8 address, u8 data)
{
  s32 status = 0;

  CSN_0;
  status |= MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, WCR | address);
  status |= MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, data);
  CSN_1;

  return status;
}

/////////////////////////////////////////////////////////////////////////////
//! Sends the 8 bit BFC opcode/Address byte over the SPI and then sends 
//! the data in the next 8 SPI clocks
//!
//! This routine is almost identical to the MIOS32_ENC28J60_WriteReg() and
//! MIOS32_ENC28J60_BFSReg() functions.  It is separate to maximize speed.
//!
//! MIOS32_ENC28J60_BFCReg() must only be used on ETH registers.
//! \param[in] address 5 bit address of the register to modify. The top 3 bits must be 0
//! \param[in] data Byte to be used with the Bit Field Clear operation
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_BFCReg(u8 address, u8 data)
{
  s32 status = 0;

  CSN_0;
  status |= MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, BFC | address);
  status |= MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, data);
  CSN_1;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Sends the 8 bit BFS opcode/Address byte over the SPI and then sends 
//! the data in the next 8 SPI clocks
//!
//! This routine is almost identical to the MIOS32_ENC28J60_WriteReg() and
//! MIOS32_ENC28J60_BFCReg() functions.  It is separate to maximize speed.
//!
//! MIOS32_ENC28J60_BFSReg() must only be used on ETH registers.
//! \param[in] address 5 bit address of the register to modify. The top 3 bits must be 0
//! \param[in] data Byte to be used with the Bit Field Set operation
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_BFSReg(u8 address, u8 data)
{
  s32 status = 0;

  CSN_0;
  status |= MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, BFS | address);
  status |= MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, data);
  CSN_1;

  return status;
}

/////////////////////////////////////////////////////////////////////////////
//! Performs an MII write operation.
//!
//! While in progress, it simply polls the MII BUSY bit wasting time.
//!
//! Alters bank bits to point to Bank 3
//! \param[in] reg Address of the PHY register to write to.
//! \param[in] data 16 bits of data to write to PHY register.
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_WritePHYReg(u8 reg, u16 data)
{
  s32 status = 0;

  // transfer data word into register
  status |= MIOS32_ENC28J60_BankSel(MIREGADR);
  status |= MIOS32_ENC28J60_WriteReg((u8)MIREGADR, reg);
  status |= MIOS32_ENC28J60_WriteReg((u8)MIWRL, data & 0xff);
  status |= MIOS32_ENC28J60_WriteReg((u8)MIWRH, data >> 8);

  status |= MIOS32_ENC28J60_BankSel(MISTAT);

  // skip if something already failed
  if( status >= 0 ) {
    // wait until PHY register has been written
    // TODO: add timeout
    do {
      status = MIOS32_ENC28J60_ReadMACReg((u8)MISTAT);
    } while( status >= 0 && (status & MISTAT_BUSY) );
  }

  return status;
}

/////////////////////////////////////////////////////////////////////////////
//! Takes the high byte of a register address and changes the bank select 
//! bits in ETHCON1 to match.
//!
//! \param[in] reg Register address with the high byte containing the two bank select bits.
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_BankSel(u16 reg)
{
  s32 status = 0;

  status |= MIOS32_ENC28J60_BFCReg(ECON1, ECON1_BSEL1 | ECON1_BSEL0);
  status |= MIOS32_ENC28J60_BFSReg(ECON1, (u8)(reg>>8));

  return status;
}

/////////////////////////////////////////////////////////////////////////////
//! Sends the System Reset SPI command to the Ethernet controller.
//! It resets all register contents (except for ECOCON) and returns the 
//! device to the power on default state.
//!
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_SendSystemReset(void)
{
  s32 status = 0;
#if 0
  // sequence taken from SendSystemReset() of Microchip driver

  // Note: The power save feature may prevent the reset from executing, so 
  // we must make sure that the device is not in power save before issuing 
  // a reset.
  if( (status=MIOS32_ENC28J60_BFCReg(ECON2, ECON2_PWRSV)) < 0 )
    return status;

  // Give some opportunity for the regulator to reach normal regulation and 
  // have all clocks running
  MIOS32_DELAY_Wait_uS(1000); // 1 mS
#endif
  // Execute the System Reset command
  status = 0;
  CSN_0;
  status |= MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, SR);
  CSN_1;
  if( status < 0 )
    return status;

  // Wait for the oscillator start up timer and PHY to become ready
  MIOS32_DELAY_Wait_uS(1000); // 1 mS

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the byte pointed to by ERDPT and increments ERDPT so MIOS32_ENC28J60_MACGet()
//! can be called again. The increment will follow the receive buffer wrapping boundary.
//!
//! EWRPT is incremented after the read.
//! \return < 0 on errors
//! \return >= 0: Byte read from the ENC28J60's RAM
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_MACGet(void)
{
  s32 status;

  CSN_0;
  status = MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, RBM);
  if( status >= 0 )
    status = MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, 0xff);
  CSN_1;

  return status;
}

/////////////////////////////////////////////////////////////////////////////
//! Burst reads several sequential bytes from the data buffer and places them
//! into local memory.  With SPI burst support, it performs much faster than 
//! multiple MIOS32_ENC28J60_MACGet() calls.
//!
//! ERDPT is incremented after each byte, following the same rules as MIOS32_ENC28J60_MACGet()
//! \param[in] buffer target buffer
//! \param[in] len number of bytes which should be read
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_MACGetArray(u8 *buffer, u16 len)
{
  s32 status = 0;

  CSN_0;
  status |= MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, RBM);
  status |= MIOS32_SPI_TransferBlock(MIOS32_ENC28J60_SPI, NULL, buffer, len, NULL);
  CSN_1;

  return status;
}


/////////////////////////////////////////////////////////////////////////////
//! Outputs the Write Buffer Memory opcode/constant (8 bits) and data to 
//! write (8 bits) over the SPI.  
//!
//! EWRPT is incremented after the write.
//! \param[in] value Byte to write into the ENC28J60 buffer memory
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_MACPut(u8 value)
{
  s32 status = 0;

  CSN_0;
  status |= MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, WBM);
  status |= MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, value);
  CSN_1;

  return status;
}

/////////////////////////////////////////////////////////////////////////////
//! writes several sequential bytes to the ENC28J60 RAM. It performs faster 
//! than multiple MIOS32_ENC28J60_MACPut() calls.
//!
//! EWRPT is incremented by len.
//! \param[in] buffer source buffer
//! \param[in] len number of bytes which should be written
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_ENC28J60_MACPutArray(u8 *buffer, u16 len)
{
  s32 status = 0;

  CSN_0;
  status |= MIOS32_SPI_TransferByte(MIOS32_ENC28J60_SPI, WBM);
  status |= MIOS32_SPI_TransferBlock(MIOS32_ENC28J60_SPI, buffer, NULL, len, NULL);
  CSN_1;

  return status;
}


//! \}

#endif /* MIOS32_DONT_USE_ENC28J60 */

