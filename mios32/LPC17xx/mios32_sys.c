// $Id$
//! \defgroup MIOS32_SYS
//!
//! System Initialisation for MIOS32
//! 
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

#ifndef MIOS32_DONT_USE_FREERTOS
#include <FreeRTOS.h>
#include <portmacro.h>
#endif

#include <sbl_iap.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_SYS)

// specified in .ld file
extern u32 mios32_sys_isr_vector;


/////////////////////////////////////////////////////////////////////////////
// Local Macros
/////////////////////////////////////////////////////////////////////////////

#define MEM32(addr) (*((volatile u32 *)(addr)))
#define MEM16(addr) (*((volatile u16 *)(addr)))
#define MEM8(addr)  (*((volatile u8  *)(addr)))


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
#define NUM_DMA_CHANNELS 8
static void (*dma_callback[NUM_DMA_CHANNELS])(void);


/////////////////////////////////////////////////////////////////////////////
// to enter IAP routines
/////////////////////////////////////////////////////////////////////////////
static void iap_entry(unsigned param_tab[],unsigned result_tab[])
{
  void (*iap)(unsigned [],unsigned []);

  iap = (void (*)(unsigned [],unsigned []))IAP_ADDRESS;
  iap(param_tab,result_tab);
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes the System for MIOS32:<BR>
//! <UL>
//!   <LI>enables clock for IO ports
//!   <LI>configures pull-ups for all IO pins
//!   <LI>initializes PLL for 72 MHz @ 12 MHz ext. clock<BR>
//!       (skipped if PLL already running - relevant for proper software 
//!        reset, e.g. so that USB connection can survive)
//!   <LI>sets base address of vector table
//!   <LI>configures the suspend flags in DBGMCU_CR as specified in 
//!       MIOS32_SYS_STM32_DBGMCU_CR (can be overruled in mios32_config.h)
//!       to simplify debugging via JTAG
//!   <LI>enables the system realtime clock via MIOS32_SYS_Time_Init(0)
//! </UL>
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SYS_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // clock setup
  LPC_SC->SCS       = (1 << 5);          // enable main oscillator
  while( (LPC_SC->SCS & (1<<6) ) == 0 ); // wait for oscillator ready

  LPC_SC->CCLKCFG   = 4-1;               // CPU clock divider (f_pll / 4) -> 100 MHz

  // peripheral clock selection:
  // all are clocked with CCLK/4 (25 MHz)
  // except for CANs, SPIs and I2S which are clocked at full speed (100 MHz)
  LPC_SC->PCLKSEL0  = (1 << 30) | (1 << 28) | (1 << 26) | (1 << 16) | (1 << 20);
  LPC_SC->PCLKSEL1  = (1 << 10) | (1 << 22);

  LPC_SC->CLKSRCSEL = 1;                 // select PLL0 as clock source



  // PLL0
  // check also consistency with mios32_sys.h
#if MIOS32_SYS_CPU_FREQUENCY == 100000000
  LPC_SC->PLL0CFG   = ((6-1)<<16) | (((100-1)<<0)); // PLL config: N=6, M=100 -> 12 MHz * 2 * 100 / 6 -> 400 MHz
#elif MIOS32_SYS_CPU_FREQUENCY == 120000000
  LPC_SC->PLL0CFG   = ((6-1)<<16) | (((120-1)<<0)); // PLL config: N=6, M=120 -> 12 MHz * 2 * 120 / 6 -> 480 MHz
#else
# error "Please adapt MIOS32_SYS_Init() for the selected MIOS32_SYS_CPU_FREQUENCY!"
#endif

  LPC_SC->PLL0FEED  = 0xaa;
  LPC_SC->PLL0FEED  = 0x55;

  LPC_SC->PLL0CON   = 0x01;              // PLL0 enable
  LPC_SC->PLL0FEED  = 0xaa;
  LPC_SC->PLL0FEED  = 0x55;
  while( !(LPC_SC->PLL0STAT & (1<<26)) ); // wait for PLL lock

  LPC_SC->PLL0CON   = 0x03;              // PLL0 enable & connect
  LPC_SC->PLL0FEED  = 0xaa;
  LPC_SC->PLL0FEED  = 0x55;
  while( !(LPC_SC->PLL0STAT & ((1<<25) | (1<<24))) ); // wait for PLLC0_STAT & PLLE0_STAT


  // PLL1
  LPC_SC->PLL1CFG   = ((2-1)<<5) | (((4-1)<<0)); // PLL config: P=2, M=4 -> 12 MHz * 2 * 5 / 2 -> 48 MHz
  LPC_SC->PLL1FEED  = 0xaa;
  LPC_SC->PLL1FEED  = 0x55;

  LPC_SC->PLL1CON   = 0x01;             // PLL1 enable
  LPC_SC->PLL1FEED  = 0xaa;
  LPC_SC->PLL1FEED  = 0x55;
  while( !(LPC_SC->PLL1STAT & (1<<10)) ); // wait for PLOCK1

  LPC_SC->PLL1CON   = 0x03;             // PLL1 enable & connect
  LPC_SC->PLL1FEED  = 0xaa;
  LPC_SC->PLL1FEED  = 0x55;
  while( !(LPC_SC->PLL1STAT & ((1<< 9) | (1<< 8))) );/* Wait for PLLC1_STAT & PLLE1_STAT */


  // Power control: enable all peripherals
  LPC_SC->PCONP     = 0xffffffff;

  // clock output configuration
  LPC_SC->CLKOUTCFG = 0x00000000;

#if MIOS32_SYS_CPU_FREQUENCY <= 100000000 || (defined(MIOS32_PROCESSOR_LPC1769) && MIOS32_SYS_CPU_FREQUENCY <= 120000000)
  // Flash: set access time to 5 CPU clocks (suitable for 100 MHz, resp. 120 MHz LPC1769 is used)
  LPC_SC->FLASHCFG  = (LPC_SC->FLASHCFG & ~0x0000F000) | ((5-1) << 12);
#else
# error "Please adapt MIOS32_SYS_Init() for the selected MIOS32_SYS_CPU_FREQUENCY!"
#endif

  // Set the Vector Table base address as specified in .ld file (-> mios32_sys_isr_vector)
  SCB->VTOR = (u32)&mios32_sys_isr_vector;
  SCB->AIRCR = 0x05fa0000 | MIOS32_IRQ_PRIGROUP;

#ifndef MIOS32_SYS_DONT_INIT_RTC
  // initialize system clock
  mios32_sys_time_t t = { .seconds=0, .fraction_ms=0 };
  MIOS32_SYS_TimeSet(t);
#endif

  // disable DMA callbacks
  int i;
  for(i=0; i<NUM_DMA_CHANNELS; ++i)
    dma_callback[i] = NULL;

  // enable DMA
  LPC_GPDMA->DMACConfig = (1 << 0);

  // enable DMA interrupts
  MIOS32_IRQ_Install(DMA_IRQn, MIOS32_IRQ_GLOBAL_DMA_PRIORITY);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Shutdown MIOS32 and reset the microcontroller:<BR>
//! <UL>
//!   <LI>disable all RTOS tasks
//!   <LI>print "Bootloader Mode " message if LCD enabled (since MIOS32 will enter this mode after reset)
//!   <LI>wait until all MIDI OUT buffers are empty (TODO)
//!   <LI>disable all interrupts
//!   <LI>turn off all board LEDs
//!   <LI>send all-0 to DOUT chain (TODO)
//!   <LI>send all-0 to MF control chain (if enabled) (TODO)
//!   <LI>reset STM32
//! </UL>
//! \return < 0 if reset failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SYS_Reset(void)
{
  // disable all RTOS tasks
#ifndef MIOS32_DONT_USE_FREERTOS
  portENTER_CRITICAL(); // port specific FreeRTOS function to disable tasks (nested)
#endif

  // print reboot message if LCD enabled
#ifndef MIOS32_DONT_USE_LCD
  // TODO: here we should select the normal font - but only if available!
  // MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL);
  MIOS32_LCD_BColourSet(0xffffff);
  MIOS32_LCD_FColourSet(0x000000);

  MIOS32_LCD_DeviceSet(0);
  MIOS32_LCD_Clear();
  MIOS32_LCD_CursorSet(0, 0);
  MIOS32_LCD_PrintString("Bootloader Mode "); // 16 chars
#endif

  // wait until all MIDI OUT buffers are empty (TODO)

  // disable all interrupts
  MIOS32_IRQ_Disable();

  // turn off all board LEDs
  MIOS32_BOARD_LED_Set(0xffffffff, 0x00000000);

  // send all-0 to DOUT chain (TODO)

  // send all-0 to MF control chain (if enabled) (TODO)


  // reset some peripherals (unfortunately there is no system reset available)
  LPC_GPDMA->DMACConfig = 0; // disable DMA

  LPC_ADC->ADINTEN = 0; // ADC: disable interrupts
  LPC_ADC->ADCR = 0; // ADC: clear the PDN flag #21 (Power Down)

  LPC_CAN1->MOD = (1 << 0); // enter reset mode
  LPC_CAN2->MOD = (1 << 0); // enter reset mode

  // DAC: no SW reset

  LPC_EMAC->MAC1 = 0xff00; // set all reset flags

  // I2C: no reset

  // MCPWM/PWM: no reset

  // SPI: no reset
  // UART: no reset

  // ensure that at least UART interrupts are disabled
  LPC_UART0->IER = 0;
  LPC_UART1->IER = 0;
  LPC_UART2->IER = 0;
  LPC_UART3->IER = 0;

  // disable timer interrupts
  LPC_TIM0->MCR = 0;
  LPC_TIM1->MCR = 0;
  LPC_TIM2->MCR = 0;

  // I2S
  LPC_I2S->I2SDAO = (1 << 4); // set asynchronous reset
  
  // reset GPIOs?
  // yes! Otherwise the Boot Hold pin could be driven (for example)
  // Exception: leave USB pins at P0.29, P0.30 abd P2,9 untouched
  LPC_PINCON->PINSEL0 = 0;
  LPC_PINCON->PINSEL1 &= 0x3c000000;
  LPC_PINCON->PINSEL2 = 0;
  LPC_PINCON->PINSEL3 = 0;
  LPC_PINCON->PINSEL4 &= 0x000c0000;
  LPC_PINCON->PINSEL5 = 0;
  LPC_PINCON->PINSEL6 = 0;
  LPC_PINCON->PINSEL7 = 0;
  LPC_PINCON->PINSEL8 = 0;
  LPC_PINCON->PINSEL9 = 0;
  LPC_PINCON->PINSEL10 = 0;

  LPC_PINCON->PINMODE0 = 0;
  LPC_PINCON->PINMODE1 &= 0x3c000000;
  LPC_PINCON->PINMODE2 = 0;
  LPC_PINCON->PINMODE3 = 0;
  LPC_PINCON->PINMODE4 &= 0x000c0000;
  LPC_PINCON->PINMODE5 = 0;
  LPC_PINCON->PINMODE6 = 0;
  LPC_PINCON->PINMODE7 = 0;
  LPC_PINCON->PINMODE8 = 0;
  LPC_PINCON->PINMODE9 = 0;

  LPC_PINCON->PINMODE_OD0 &= (1 << 29) | (1 << 30);
  LPC_PINCON->PINMODE_OD1 = 0;
  LPC_PINCON->PINMODE_OD2 &= (1 << 9);
  LPC_PINCON->PINMODE_OD3 = 0;
  LPC_PINCON->PINMODE_OD4 = 0;

  // reset LPC17
  SCB->AIRCR = (0x5fa << SCB_AIRCR_VECTKEY_Pos) | (1 << SCB_AIRCR_VECTRESET_Pos);

  while( 1 );

  return -1; // we will never reach this point
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the Chip ID of the core
//! \return the chip ID
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_SYS_ChipIDGet(void)
{
  unsigned param_table[5];
  unsigned result_table[5];

  param_table[0] = READ_PART_ID;
  iap_entry(param_table, result_table);
  return result_table[1];
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the Flash size of the core
//! \return the Flash size in bytes
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_SYS_FlashSizeGet(void)
{
  // stored in the so called "electronic signature"
  //return (u32)MEM16(0x1ffff7e0) * 0x400;
  return 512*1024; // TODO
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the (data) RAM size of the core
//! \return the RAM size in bytes
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_SYS_RAMSizeGet(void)
{
  // stored in the so called "electronic signature"
  //return (u32)MEM16(0x1ffff7e2) * 0x400;
  return 64*1024; // TODO
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the serial number as a string
//! \param[out] str pointer to a string which can store at least 32 digits + zero terminator!
//! (24 digits returned for STM32)
//! \return < 0 if feature not supported
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SYS_SerialNumberGet(char *str)
{
  unsigned param_table[5];
  unsigned result_table[5];

  param_table[0] = 58; // READ_DEVICE_SERIAL_NUMBER not documented in iap.h
  iap_entry(param_table, result_table);

  // store 32 digits
  int i, j;
  for(i=0; i<4; ++i) {
    u32 w = result_table[1 + i];

    for(j=0; j<8; ++j) {
      u8 d = w & 0xf;
      str[i*8+j] = ((d > 9) ? ('A'-10) : '0') + d;
      w >>= 4;
    }
  }
  str[32] = 0;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Initializes/Resets the System Real Time Clock, so that MIOS32_SYS_Time() can
//! be used for microsecond accurate measurements.
//!
//! The time can be re-initialized the following way:
//! \code
//!   // set System Time to one hour and 30 minutes
//!   mios32_sys_time_t t = { .seconds=1*3600 + 30*60, .fraction_ms=0 };
//!   MIOS32_SYS_TimeSet(t);
//! \endcode
//!
//! After system reset it will always start with 0. A battery backup option is
//! not supported by MIOS32
//!
//! \param[in] t the time in seconds + fraction part (mS)<BR>
//! Note that this format isn't completely compatible to the NTP timestamp format,
//! as the fraction has only mS accuracy
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SYS_TimeSet(mios32_sys_time_t t)
{
  // reset CTC
  LPC_RTC->CCR = (1 << 1);
  LPC_RTC->CCR = 0;

  // configure time
  // only 24h are covered, although the LPC_RTC counts even years!
  // this is to keep it compatible with STM32 implementation
  //
  // big drawback of LPC variant: microseconds not available!
  // and an external 32kHz crystal required
  LPC_RTC->SEC  = t.seconds % 60;
  LPC_RTC->MIN  = (t.seconds % 3600) / 60;
  LPC_RTC->HOUR = (t.seconds % 3600) % 24;

  // enable CTC
  LPC_RTC->CCR = (1 << 0);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the System Real Time (with mS accuracy)
//!
//! Following example code converts the returned time into hours, minutes,
//! seconds and milliseconds:
//! \code
//!   mios32_sys_time_t t = MIOS32_SYS_TimeGet();
//!   int hours = t.seconds / 3600;
//!   int minutes = (t.seconds % 3600) / 60;
//!   int seconds = (t.seconds % 3600) % 60;
//!   int milliseconds = t.fraction_ms;
//! \endcode
//! \return the system time in a mios32_sys_time_t structure
/////////////////////////////////////////////////////////////////////////////
mios32_sys_time_t MIOS32_SYS_TimeGet(void)
{
  u32 ctime = LPC_RTC->CTIME0;

  // calculate time
  // only 24h are covered, although the LPC_RTC counts even years!
  // this is to keep it compatible with STM32 implementation
  //
  // big drawback of LPC variant: microseconds not available!
  // and an external 32kHz crystal required
  u32 seconds = (ctime & 0xff) + 60*((ctime>>8)&0xff) + 3600*((ctime>>16)&0xff);

  mios32_sys_time_t t = {
    .seconds = seconds,
    .fraction_ms = 0,
  };

  return t;
}


/////////////////////////////////////////////////////////////////////////////
//! Installs a DMA callback function which is invoked on DMA interrupts\n
//! Available for LTC17xx (and not STM32) since it only provides a single DMA
//! interrupt which is shared by all channels.
//! \param[in] dma the DMA number (currently always 0)
//! \param[in] chn the DMA channel (0..7)
//! \param[in] callback the callback function which will be invoked by DMA ISR
//! \return -1 if function not implemented for this MIOS32_PROCESSOR
//! \return -2 if invalid DMA number is selected
//! \return -2 if invalid DMA channel selected
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SYS_DMA_CallbackSet(u8 dma, u8 chn, void *callback)
{
  if( dma > 0 )
    return -2; // invalid DMA number selected

  if( chn >= NUM_DMA_CHANNELS )
    return -3; // invalid DMA channel selected

  dma_callback[chn] = callback;

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// The DMA ISR which is shared by all DMA channels
// (unfortunately... a LPC17xx limitation)
/////////////////////////////////////////////////////////////////////////////
void DMA_IRQHandler(void)
{
  // this shared handling will cost some cycles... :-/
  // TODO: check if DMA interrupt really invoked again
  // if new flags are set after read and while the handler is executed
  u32 int_stat = LPC_GPDMA->DMACIntStat;

  int chn;
  u32 mask = 0x0001;
  for(chn=0; chn<NUM_DMA_CHANNELS; ++chn, mask<<=1) {
    if( int_stat & mask ) {
      void (*_callback)(void) = dma_callback[chn];
      if( _callback != NULL )
	_callback();
      LPC_GPDMA->DMACIntTCClear = mask;
      LPC_GPDMA->DMACIntErrClr = mask;
    }
  }
}

//! \}

#endif /* MIOS32_DONT_USE_SYS */
