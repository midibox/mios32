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


#define EXT_CRYSTAL_FRQ 8000000  // used for MBHP_CORE_STM32, should we define this somewhere else or select via MIOS32_BOARD?

#if EXT_CRYSTAL_FRQ != 8000000
# error "Please provide alternative PLL config"
#endif
/* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N */
#define PLL_M      8
#define PLL_N      336

/* SYSCLK = PLL_VCO / PLL_P */
#define PLL_P      2

/* USB OTG FS, SDIO and RNG Clock =  PLL_VCO / PLLQ */
#define PLL_Q      7


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

  // Enable GPIOA, GPIOB, GPIOC, GPIOD, GPIOE and AFIO clocks
  RCC_AHB1PeriphClockCmd(
			 RCC_AHB1Periph_GPIOA | 
			 RCC_AHB1Periph_GPIOB | 
			 RCC_AHB1Periph_GPIOC |
			 RCC_AHB1Periph_GPIOD | 
			 RCC_AHB1Periph_GPIOE
			 , ENABLE);

#if 0
  // Activate pull-ups on all pins by default
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
  //GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; // not required for input
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin   = 0xffff & ~GPIO_Pin_11 & ~GPIO_Pin_12; // exclude USB pins
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin   = 0xffff;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  //  GPIO_Init(GPIOE, &GPIO_InitStructure);
#endif

  /* FPU settings ------------------------------------------------------------*/
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
#endif

  // init clock system if chip doesn't already run with PLL
  __IO uint32_t HSEStatus = 0;
  if( (RCC->CFGR & (uint32_t)RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL ) {
    HSEStatus = SUCCESS;
  } else {
    /* Reset the RCC clock configuration to the default reset state ------------*/
    /* Set HSION bit */
    RCC->CR |= (uint32_t)0x00000001;

    /* Reset CFGR register */
    RCC->CFGR = 0x00000000;

    /* Reset HSEON, CSSON and PLLON bits */
    RCC->CR &= (uint32_t)0xFEF6FFFF;

    /* Reset PLLCFGR register */
    RCC->PLLCFGR = 0x24003010;

    /* Reset HSEBYP bit */
    RCC->CR &= (uint32_t)0xFFFBFFFF;

    /* Disable all interrupts */
    RCC->CIR = 0x00000000;

    /* Configure the System clock source, PLL Multiplier and Divider factors, 
       AHB/APBx prescalers and Flash settings ----------------------------------*/

    /******************************************************************************/
    /*            PLL (clocked by HSE) used as System clock source                */
    /******************************************************************************/
    __IO uint32_t StartUpCounter = 0;
  
    /* Enable HSE */
    RCC->CR |= ((uint32_t)RCC_CR_HSEON);
 
    /* Wait till HSE is ready and if Time out is reached exit */
    do {
      HSEStatus = RCC->CR & RCC_CR_HSERDY;
      StartUpCounter++;
    } while((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

    if ((RCC->CR & RCC_CR_HSERDY) != RESET) {
      HSEStatus = (uint32_t)0x01;
    } else {
      HSEStatus = (uint32_t)0x00;
    }

    if (HSEStatus == (uint32_t)0x01) {
      /* Select regulator voltage output Scale 1 mode, System frequency up to 168 MHz */
      RCC->APB1ENR |= RCC_APB1ENR_PWREN;
      PWR->CR |= PWR_CR_VOS;

      /* HCLK = SYSCLK / 1*/
      RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
      
      /* PCLK2 = HCLK / 2*/
      RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;
    
      /* PCLK1 = HCLK / 4*/
      RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;

      /* Configure the main PLL */
      RCC->PLLCFGR = PLL_M | (PLL_N << 6) | (((PLL_P >> 1) -1) << 16) |
	             (RCC_PLLCFGR_PLLSRC_HSE) | (PLL_Q << 24);

      /* Enable the main PLL */
      RCC->CR |= RCC_CR_PLLON;

      /* Wait till the main PLL is ready */
      while((RCC->CR & RCC_CR_PLLRDY) == 0);
   
      /* Configure Flash prefetch, Instruction cache, Data cache and wait state */
      FLASH->ACR = FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_5WS;

      /* Select the main PLL as system clock source */
      RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
      RCC->CFGR |= RCC_CFGR_SW_PLL;

      /* Wait till the main PLL is used as system clock source */
      while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
    } else {
      /* If HSE fails to start-up, the application will have wrong clock
         configuration. User can add here some code to deal with this error */
    }
  }

  // Set the Vector Table base address as specified in .ld file (-> mios32_sys_isr_vector)
  NVIC_SetVectorTable((u32)&mios32_sys_isr_vector, 0x0);
  NVIC_PriorityGroupConfig(MIOS32_IRQ_PRIGROUP);

#ifndef MIOS32_SYS_DONT_INIT_RTC
  // initialize system clock
  mios32_sys_time_t t = { .seconds=0, .fraction_ms=0 };
  MIOS32_SYS_TimeSet(t);
#endif

  // error during clock configuration?
  return HSEStatus == SUCCESS ? 0 : -1;
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

  // reset peripherals
  RCC_AHB1PeriphResetCmd(0xfffffffe, ENABLE); // don't reset GPIOA due to USB pins
  RCC_AHB2PeriphResetCmd(0xffffff7f, ENABLE); // don't reset OTG_FS, so that the connectuion can survive
  RCC_APB1PeriphResetCmd(0xffffffff, ENABLE);
  RCC_APB2PeriphResetCmd(0xffffffff, ENABLE);
  RCC_AHB1PeriphResetCmd(0xffffffff, DISABLE);
  RCC_AHB2PeriphResetCmd(0xffffffff, DISABLE);
  RCC_APB1PeriphResetCmd(0xffffffff, DISABLE);
  RCC_APB2PeriphResetCmd(0xffffffff, DISABLE);

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
  return (u32)MEM16(0x1fff7a22) * 0x400;
}

/////////////////////////////////////////////////////////////////////////////
//! Returns the (data) RAM size of the core
//! \return the RAM size in bytes
/////////////////////////////////////////////////////////////////////////////
u32 MIOS32_SYS_RAMSizeGet(void)
{
  // stored in the so called "electronic signature"
#if defined(MIOS32_PROCESSOR_STM32F407VG)
  return 192*1024; // unfortunately not stored in signature...
#else
# error "Please define RAM size here"
#endif
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
    u8 b = MEM8(0x1fff7a10 + (i/2));
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
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

  // Allow access to BKP Domain
  PWR_BackupAccessCmd(ENABLE);

  // Select HSE (divided by 16) as RTC Clock Source
#if EXT_CRYSTAL_FRQ != 8000000
# error "Please configure alternative clock divider here"
#endif
  RCC_RTCCLKConfig(RCC_RTCCLKSource_HSE_Div16); // -> each 1/(8 MHz / 16) = 2 uS

  // Enable RTC Clock
  RCC_RTCCLKCmd(ENABLE);

  // initialize RTC
  RTC_InitTypeDef RTC_InitStruct;
  RTC_StructInit(&RTC_InitStruct);

  // Set RTC prescaler: set RTC period from 2 uS to 1 S
  RTC_InitStruct.RTC_AsynchPrediv = 100 - 1; // 7bit maximum
  RTC_InitStruct.RTC_SynchPrediv = 5000 - 1; // 13 bit maximum
  RTC_Init(&RTC_InitStruct);

  // Change the current time
  RTC_TimeTypeDef RTC_TimeStruct;
  RTC_TimeStructInit(&RTC_TimeStruct);
  RTC_TimeStruct.RTC_Hours = t.seconds / 3600;
  RTC_TimeStruct.RTC_Minutes = (t.seconds % 3600) / 60;
  RTC_TimeStruct.RTC_Seconds = t.seconds % 60; 
  RTC_SetTime(RTC_Format_BIN, &RTC_TimeStruct);
  // (fraction not taken into account here)

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
  RTC_TimeTypeDef RTC_TimeStruct;
  RTC_GetTime(RTC_Format_BIN, &RTC_TimeStruct);

  mios32_sys_time_t t = {
    .seconds = RTC_TimeStruct.RTC_Hours * 3600 + RTC_TimeStruct.RTC_Minutes * 60 + RTC_TimeStruct.RTC_Seconds,
    .fraction_ms = 0 // not supported
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
