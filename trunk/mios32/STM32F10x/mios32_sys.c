// $Id$
//! \defgroup MIOS32_SYS
//!
//! System Initialisation for MIOS32
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

#ifndef MIOS32_DONT_USE_FREERTOS
#include <FreeRTOS.h>
#include <portmacro.h>
#endif

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


#define EXT_CRYSTAL_FRQ 12000000  // used for MBHP_CORE_STM32, should we define this somewhere else or select via MIOS32_BOARD?
#define RTC_PREDIVIDER  (EXT_CRYSTAL_FRQ/128)


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
  ErrorStatus HSEStartUpStatus = ERROR;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // Enable GPIOA, GPIOB, GPIOC, GPIOD, GPIOE and AFIO clocks
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC
			 | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO, ENABLE);

  // Activate pull-ups on all pins by default
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin   = 0xffff & ~GPIO_Pin_11 & ~GPIO_Pin_12; // exclude USB pins
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin   = 0xffff;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  //  GPIO_Init(GPIOE, &GPIO_InitStructure);

#ifdef MIOS32_BOARD_STM32_PRIMER
  // special measure for STM32 Primer
  // ensure that pull-down is active on USB detach pin
  GPIO_InitStructure.GPIO_Pin   = 0xffff & ~GPIO_Pin_12;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
#else
  GPIO_Init(GPIOB, &GPIO_InitStructure);
#endif

  // init clock system if chip doesn't already run with PLL
  if( RCC_GetSYSCLKSource() != 0x08 ) {
    // Start with the clocks in their expected state
    RCC_DeInit();

    // Enable HSE (high speed external clock)
    RCC_HSEConfig(RCC_HSE_ON);

    // Wait till HSE is ready
    HSEStartUpStatus = RCC_WaitForHSEStartUp();

    if( HSEStartUpStatus == SUCCESS ) {
      // Enable Prefetch Buffer
      FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

      // Flash 2 wait state
      FLASH_SetLatency(FLASH_Latency_2);

      // HCLK = SYSCLK
      RCC_HCLKConfig(RCC_SYSCLK_Div1);

      // PCLK2 = HCLK
      RCC_PCLK2Config(RCC_HCLK_Div1);

      // PCLK1 = HCLK/2
      RCC_PCLK1Config(RCC_HCLK_Div2);

      // ADCCLK = PCLK2/6
      RCC_ADCCLKConfig(RCC_PCLK2_Div6);

// check consistency with mios32_sys.h
#if MIOS32_SYS_CPU_FREQUENCY != 72000000ULL
# error "Please adapt MIOS32_SYS_Init() for the selected MIOS32_SYS_CPU_FREQUENCY!"
#endif

#ifdef STM32F10X_CL
      // PLL2 configuration: PLL2CLK = (HSE / 3) * 10 = 40 MHz
      RCC_PREDIV2Config(RCC_PREDIV2_Div3);
      RCC_PLL2Config(RCC_PLL2Mul_10);

      // Enable PLL2
      RCC_PLL2Cmd(ENABLE);

      // Wait till PLL2 is ready
      while (RCC_GetFlagStatus(RCC_FLAG_PLL2RDY) == RESET);

      // PLL configuration: PLLCLK = (PLL2 / 5) * 9 = 72 MHz
      RCC_PREDIV1Config(RCC_PREDIV1_Source_PLL2, RCC_PREDIV1_Div5);
      RCC_PLLConfig(RCC_PLLSource_PREDIV1, RCC_PLLMul_9);
#else
      // PLLCLK = 12MHz * 6 = 72 MHz
      RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_6);
#endif

      // Enable PLL
      RCC_PLLCmd(ENABLE);

      // Wait till PLL is ready
      while( RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET );

      // Select PLL as system clock source
      RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

      // Wait till PLL is used as system clock source
      while( RCC_GetSYSCLKSource() != 0x08 );
    }
  }

  // Set the Vector Table base address as specified in .ld file (-> mios32_sys_isr_vector)
  NVIC_SetVectorTable((u32)&mios32_sys_isr_vector, 0x0);
  NVIC_PriorityGroupConfig(MIOS32_IRQ_PRIGROUP);

  // Configure HCLK clock as SysTick clock source
  SysTick_CLKSourceConfig( SysTick_CLKSource_HCLK );

  // configure debug control register DBGMCU_CR (we want to stop timers in CPU HALT mode)
  // flags can be overruled in mios32_config.h
  MEM32(0xe0042004) = MIOS32_SYS_STM32_DBGMCU_CR;

#ifndef MIOS32_SYS_DONT_INIT_RTC
  // initialize system clock
  mios32_sys_time_t t = { .seconds=0, .fraction_ms=0 };
  MIOS32_SYS_TimeSet(t);
#endif

  // error during clock configuration?
  return HSEStartUpStatus == SUCCESS ? 0 : -1;
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

  // disable all interrupts
  MIOS32_IRQ_Disable();

  // turn off all board LEDs
  MIOS32_BOARD_LED_Set(0xffffffff, 0x00000000);

  // wait for 50 mS to ensure that all ongoing operations (e.g. DMA driver SPI transfers) are finished
  {
    int i;
    for(i=0; i<50; ++i)
      MIOS32_DELAY_Wait_uS(1000);
  }

  // reset STM32
#if 0
  // doesn't work for some reasons...
  // in addition, causes a USB disconnect on Primer (probably since GPIOB is reseted?)
  NVIC_GenerateSystemReset();
#else
  //  NVIC_GenerateSystemReset();
#ifdef MIOS32_BOARD_STM32_PRIMER
  RCC_APB2PeriphResetCmd(0xfffffff0, ENABLE); // Primer: don't reset GPIOA/AF + GPIOB due to USB detach pin
#else
  RCC_APB2PeriphResetCmd(0xfffffff8, ENABLE); // MBHP_CORE_STM32: don't reset GPIOA/AF due to USB pins
#endif
  RCC_APB1PeriphResetCmd(0xff7fffff, ENABLE); // don't reset USB, so that the connection can survive!
  RCC_APB2PeriphResetCmd(0xffffffff, DISABLE);
  RCC_APB1PeriphResetCmd(0xffffffff, DISABLE);

#if 0
  // v2.0.1
  NVIC_GenerateCoreReset();
#endif
#if 0
  // not available in v3.0.0 library anymore? - copy from v2.0.1
  SCB->AIRCR = NVIC_AIRCR_VECTKEY | (1 << NVIC_VECTRESET);
#endif
#if 1
  // and this is the code for v3.3.0
  SCB->AIRCR = (0x5fa << SCB_AIRCR_VECTKEY_Pos) | (1 << SCB_AIRCR_VECTRESET_Pos);
#endif

#endif

  while( 1 );

  return -1; // we will never reach this point
}


/////////////////////////////////////////////////////////////////////////////
//! Returns the Chip ID of the core
//! \return the chip ID
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_SYS_ChipIDGet(void)
{
  // stored in DBGMCU_IDCODE register
  return MEM32(0xe0042000);
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the Flash size of the core
//! \return the Flash size in bytes
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_SYS_FlashSizeGet(void)
{
  // stored in the so called "electronic signature"
  return (u32)MEM16(0x1ffff7e0) * 0x400;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the (data) RAM size of the core
//! \return the RAM size in bytes
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_SYS_RAMSizeGet(void)
{
  // stored in the so called "electronic signature"
  return (u32)MEM16(0x1ffff7e2) * 0x400;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the serial number as a string
//! \param[out] str pointer to a string which can store at least 32 digits + zero terminator!
//! (24 digits returned for STM32)
//! \return < 0 if feature not supported
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SYS_SerialNumberGet(char *str)
{
  int i;

  // stored in the so called "electronic signature"
  for(i=0; i<24; ++i) {
    u8 b = MEM8(0x1ffff7e8 + (i/2));
    if( !(i & 1) )
      b >>= 4;
    b &= 0x0f;

    str[i] = ((b > 9) ? ('A'-10) : '0') + b;
  }
  str[i] = 0;

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
  // taken from STM32 example "RTC/Calendar"
  // adapted to clock RTC via HSE  oscillator

  // Enable PWR and BKP clocks
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

  // Allow access to BKP Domain
  PWR_BackupAccessCmd(ENABLE);

  // Reset Backup Domain
  BKP_DeInit();

  // Select HSE (divided by 128) as RTC Clock Source
  RCC_RTCCLKConfig(RCC_RTCCLKSource_HSE_Div128);

  // Enable RTC Clock
  RCC_RTCCLKCmd(ENABLE);

  // Wait for RTC registers synchronization
  RTC_WaitForSynchro();

  // Wait until last write operation on RTC registers has finished
  RTC_WaitForLastTask();

  // Enable the RTC Second
  RTC_ITConfig(RTC_IT_SEC, ENABLE);

  // Wait until last write operation on RTC registers has finished
  RTC_WaitForLastTask();

  // Set RTC prescaler: set RTC period to 1sec
  RTC_SetPrescaler(RTC_PREDIVIDER-1);

  // Wait until last write operation on RTC registers has finished
  RTC_WaitForLastTask();

  // Change the current time
  // (fraction not taken into account here)
  RTC_SetCounter(t.seconds);

  // Wait until last write operation on RTC registers has finished
  RTC_WaitForLastTask();

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
  u32 seconds, divider;
  u32 last_seconds, last_divider;

  // counter values are changing with a period of 1/(12 MHz / 128) = ca. 11 uS
  // in order to ensure, that we are reading a consistent counter set, the
  // accesses have to be repeated until we read two times the same values

  // use direct register accesses instead ofto RTC_GetCounter()/RTC_GetDivider()
  // to speed up routine

  // note: we could have an issue here if routine is permanently interrupted,
  // so that we never reach the same values...

  // therefore interrupts are disabled
  // Disadvantage: bad for interrupt latency...
  // However, expected execution time is ca. 500 nS for two loops (best case),
  // and 750 nS for three loops (worst case)

  MIOS32_IRQ_Disable();
  seconds = divider = 0;
  do {
    last_seconds = seconds;
    last_divider = divider;
    seconds = ((u32)RTC->CNTH << 16) | RTC->CNTL;
    divider = ((u32)RTC->DIVH << 16) | RTC->DIVL;
  } while( seconds != last_seconds && divider != last_divider );
  MIOS32_IRQ_Enable();

  mios32_sys_time_t t = {
    .seconds = seconds,
    .fraction_ms = (1000 * (RTC_PREDIVIDER-1-divider)) / RTC_PREDIVIDER
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
  return -1; // function not implemented for this MIOS32_PROCESSOR
}

//! \}

#endif /* MIOS32_DONT_USE_SYS */
