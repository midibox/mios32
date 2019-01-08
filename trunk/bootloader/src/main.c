// $Id: main.c 42 2008-09-30 21:49:43Z tk $
/*
 * MIOS32 Bootloader
 *
 * ==========================================================================
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
#include "bsl_sysex.h"

#if defined(MIOS32_FAMILY_STM32F10x)
#include <usb_lib.h>
#elif defined(MIOS32_FAMILY_STM32F4xx)
#include <usb_core.h>

// imported from mios32_usb.c
extern USB_OTG_CORE_HANDLE  USB_OTG_dev;
#endif


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

// LEDs used by BSL to notify the status
#define BSL_LED_MASK 0x00000001 // green LED

// the "hold" pin (low-active)
// we are using Boot1 = B2, since it suits perfectly for this purpose:
//   o jumper already available (J27)
//   o pull-up already available
//   o normaly used in Open Drain mode, so that no short circuit if jumper is stuffed
#if defined(MIOS32_FAMILY_STM32F10x)
# define BSL_HOLD_PORT        GPIOB
# define BSL_HOLD_PIN         GPIO_Pin_2
# define BSL_HOLD_STATE       ((BSL_HOLD_PORT->IDR & BSL_HOLD_PIN) ? 0 : 1)
#elif defined(MIOS32_FAMILY_STM32F4xx)
# define BSL_HOLD_PORT        GPIOA
# define BSL_HOLD_PIN         GPIO_Pin_0
# define BSL_HOLD_STATE       ((BSL_HOLD_PORT->IDR & BSL_HOLD_PIN) ? 1 : 0) // the "User Button" has a pull-down device
#else
# define BSL_HOLD_INIT        { MIOS32_SYS_LPC_PINSEL(1, 27, 0); MIOS32_SYS_LPC_PINDIR(1, 27, 0); MIOS32_SYS_LPC_PINMODE(1, 27, 0); }
# define BSL_HOLD_STATE       ((LPC_GPIO1->FIOPIN & (1 << 27)) ? 0 : 1)
#endif


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// note: adaptions also have to be done in MIOS32_BOARD_J5_(Set/Get),
// since these functions access the ports directly
#if defined(MIOS32_FAMILY_STM32F10x) || defined(MIOS32_FAMILY_STM32F4xx)
typedef struct {
  GPIO_TypeDef *port;
  u16 pin_mask;
} srio_pin_t;
#elif defined(MIOS32_FAMILY_LPC17xx)
typedef struct {
  u8 port;
  u8 pin;
} srio_pin_t;
#endif

#if defined(MIOS32_BOARD_MBHP_CORE_STM32)
#define SRIO_RESET_SUPPORTED 1

#define SRIO_GPIO_MODE GPIO_Mode_Out_OD

#define SRIO_NUM_SCLK_PINS 3
static const srio_pin_t srio_sclk_pin[SRIO_NUM_SCLK_PINS] = {
  { GPIOA, GPIO_Pin_5  }, // SPI0
  { GPIOB, GPIO_Pin_13 }, // SPI1
  { GPIOB, GPIO_Pin_6  }, // SPI2
};

#define SRIO_NUM_RCLK_PINS 5
static const srio_pin_t srio_rclk_pin[SRIO_NUM_RCLK_PINS] = {
  { GPIOA, GPIO_Pin_4  }, // SPI0, RCLK1
  { GPIOC, GPIO_Pin_15 }, // SPI0, RCLK2
  { GPIOB, GPIO_Pin_12 }, // SPI1, RCLK1
  { GPIOC, GPIO_Pin_13 }, // SPI2, RCLK1
  { GPIOC, GPIO_Pin_14 }, // SPI2, RCLK2
};

#define SRIO_NUM_MOSI_PINS 3
static const srio_pin_t srio_mosi_pin[SRIO_NUM_MOSI_PINS] = {
  { GPIOA, GPIO_Pin_7  }, // SPI0
  { GPIOB, GPIO_Pin_15 }, // SPI1
  { GPIOB, GPIO_Pin_5  }, // SPI2
};


#elif defined(MIOS32_BOARD_STM32F4DISCOVERY) || defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
#define SRIO_RESET_SUPPORTED 1

#define SRIO_GPIO_MODE  GPIO_Mode_OUT
#define SRIO_GPIO_OTYPE GPIO_OType_PP

#define SRIO_NUM_SCLK_PINS 3
static const srio_pin_t srio_sclk_pin[SRIO_NUM_SCLK_PINS] = {
  { GPIOA, GPIO_Pin_5  }, // SPI0
  { GPIOB, GPIO_Pin_13 }, // SPI1
  { GPIOB, GPIO_Pin_3  }, // SPI2
};

#define SRIO_NUM_RCLK_PINS 6
static const srio_pin_t srio_rclk_pin[SRIO_NUM_RCLK_PINS] = {
  { GPIOB, GPIO_Pin_2  }, // SPI0, RCLK1
  { GPIOD, GPIO_Pin_11 }, // SPI0, RCLK2
  { GPIOB, GPIO_Pin_12 }, // SPI1, RCLK1
  { GPIOD, GPIO_Pin_10 }, // SPI1, RCLK2
  { GPIOA, GPIO_Pin_15 }, // SPI2, RCLK1
  { GPIOB, GPIO_Pin_8  }, // SPI2, RCLK2
};

#define SRIO_NUM_MOSI_PINS 3
static const srio_pin_t srio_mosi_pin[SRIO_NUM_MOSI_PINS] = {
  { GPIOA, GPIO_Pin_7  }, // SPI0
  { GPIOB, GPIO_Pin_15 }, // SPI1
  { GPIOB, GPIO_Pin_5  }, // SPI2
};


#elif defined(MIOS32_BOARD_MBHP_CORE_LPC17)
#define SRIO_NUM_SCLK_PINS 3
static const srio_pin_t srio_sclk_pin[SRIO_NUM_SCLK_PINS] = {
  { 1, 20 }, // SPI0
  { 0,  7 }, // SPI1
  { 0, 15 }, // SPI2
};

#define SRIO_NUM_RCLK_PINS 6
static const srio_pin_t srio_rclk_pin[SRIO_NUM_RCLK_PINS] = {
  { 1, 22 }, // SPI0, RCLK1
  { 1, 21 }, // SPI0, RCLK2
  { 0,  6 }, // SPI1, RCLK1
  { 0,  5 }, // SPI1, RCLK2
  { 0, 16 }, // SPI2, RCLK1
  { 0, 21 }, // SPI2, RCLK2
};

#define SRIO_NUM_MOSI_PINS 3
static const srio_pin_t srio_mosi_pin[SRIO_NUM_MOSI_PINS] = {
  { 1, 24 }, // SPI0
  { 0,  9 }, // SPI1
  { 0, 18 }, // SPI2
};

#define SRIO_RESET_SUPPORTED 1
#else
#define SRIO_RESET_SUPPORTED 0
#endif

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////
static s32 ResetSRIOChains(void);


/////////////////////////////////////////////////////////////////////////////
// Main function
/////////////////////////////////////////////////////////////////////////////
int main(void)
{
  ///////////////////////////////////////////////////////////////////////////
  // initialize system
  ///////////////////////////////////////////////////////////////////////////
  MIOS32_SYS_Init(0);
  MIOS32_DELAY_Init(0);

  MIOS32_BOARD_LED_Init(BSL_LED_MASK);
#if !defined(MIOS32_FAMILY_STM32F10x) && !defined(MIOS32_FAMILY_STM32F4xx)
  BSL_HOLD_INIT; // e.g. for LPC17xx
#endif

  ResetSRIOChains();

  // initialize stopwatch which is used to measure a 2 second delay 
  // before application will be started
  MIOS32_STOPWATCH_Init(100); // 100 uS accuracy


  ///////////////////////////////////////////////////////////////////////////
  // get and store state of hold pin
  ///////////////////////////////////////////////////////////////////////////
  u8 hold_mode_active_after_reset = BSL_HOLD_STATE;


  ///////////////////////////////////////////////////////////////////////////
  // initialize USB only if already done (-> not after Power On) or Hold mode enabled
  ///////////////////////////////////////////////////////////////////////////
  u8 usb_was_initialized = MIOS32_USB_IsInitialized();
  if( usb_was_initialized || BSL_HOLD_STATE ) {
    // if initialized, this function will only set some variables - it won't re-init the peripheral.
    // if hold mode activated via external pin, force re-initialisation by resetting USB
    if( hold_mode_active_after_reset ) {
#if defined(MIOS32_FAMILY_STM32F10x)
      RCC_APB1PeriphResetCmd(0x00800000, ENABLE); // reset USB device
      RCC_APB1PeriphResetCmd(0x00800000, DISABLE);
#elif defined(MIOS32_FAMILY_STM32F4xx)
      RCC_AHB2PeriphResetCmd(0x00000080, ENABLE); // reset USB device
      RCC_AHB2PeriphResetCmd(0x00000080, DISABLE);
#endif
    }

    MIOS32_USB_Init(0);
  }


  ///////////////////////////////////////////////////////////////////////////
  // initialize remaining peripherals
  ///////////////////////////////////////////////////////////////////////////
  MIOS32_MIDI_Init(0); // will also initialise UART


  ///////////////////////////////////////////////////////////////////////////
  // initialize SysEx parser
  ///////////////////////////////////////////////////////////////////////////
  BSL_SYSEX_Init(0);


  ///////////////////////////////////////////////////////////////////////////
  // check for optional fast boot
  ///////////////////////////////////////////////////////////////////////////
  u8 fastboot = 0;
#ifdef MIOS32_SYS_ADDR_BSL_INFO_BEGIN
  if( !usb_was_initialized && !BSL_HOLD_STATE ) {
    // read from bootloader info range
    u8 *bsl_info_fastboot_confirm = (u8 *)MIOS32_SYS_ADDR_FASTBOOT_CONFIRM;
    u8 *bsl_info_fastboot = (u8 *)MIOS32_SYS_ADDR_FASTBOOT;
    if( *bsl_info_fastboot_confirm == 0x42 )
      fastboot = *bsl_info_fastboot;
  }
#endif


  ///////////////////////////////////////////////////////////////////////////
  // send upload request to USB and UART MIDI
  ///////////////////////////////////////////////////////////////////////////
  if( !fastboot ) {
#if !defined(MIOS32_DONT_USE_UART_MIDI)
    BSL_SYSEX_SendUploadReq(UART0);    
#endif
    if( usb_was_initialized )
      BSL_SYSEX_SendUploadReq(USB0);
  }


  ///////////////////////////////////////////////////////////////////////////
  // reset stopwatch timer and start wait loop
  if( !fastboot ) {
    MIOS32_STOPWATCH_Reset();
    do {
      // This is a simple way to pulsate a LED via PWM
      // A timer in incrementer mode is used as reference, the counter value is incremented each 100 uS
      // Within the given PWM period, we define a duty cycle based on the current counter value
      // We periodically sweep the PWM duty cycle 100 steps up, and 100 steps down
      u32 cnt = MIOS32_STOPWATCH_ValueGet();  // the reference counter (incremented each 100 uS)
      const u32 pwm_period = 50;       // *100 uS -> 5 mS
      const u32 pwm_sweep_steps = 100; // * 5 mS -> 500 mS
      u32 pwm_duty = ((cnt / pwm_period) % pwm_sweep_steps) / (pwm_sweep_steps/pwm_period);
      if( (cnt % (2*pwm_period*pwm_sweep_steps)) > pwm_period*pwm_sweep_steps )
	pwm_duty = pwm_period-pwm_duty; // negative direction each 50*100 ticks
      u32 led_on = ((cnt % pwm_period) > pwm_duty) ? 1 : 0;
      MIOS32_BOARD_LED_Set(BSL_LED_MASK, led_on ? BSL_LED_MASK : 0);


      // call periodic hook each mS (!!! important - not shorter due to timeout counters which are handled here)
      if( (cnt % 10) == 0 ) {
	MIOS32_MIDI_Periodic_mS();
      }

      // check for incoming MIDI messages - no hooks are used
      // SysEx requests will be parsed by MIOS32 internally, BSL_SYSEX_Cmd() will be called
      // directly by MIOS32 to enhance command set
      MIOS32_MIDI_Receive_Handler(NULL);
    } while( MIOS32_STOPWATCH_ValueGet() < 20000 ||             // wait for 2 seconds
	     BSL_SYSEX_HaltStateGet() ||                        // BSL not halted due to flash write operation
	     (hold_mode_active_after_reset && BSL_HOLD_STATE)); // BSL not actively halted by pin
  }

#if defined(MIOS32_FAMILY_STM32F10x) || defined(MIOS32_FAMILY_STM32F4xx)
  // ensure that flash write access is locked
  FLASH_Lock();
#endif

  // turn off LED
  MIOS32_BOARD_LED_Set(BSL_LED_MASK, 0);

  // branch to application if reset vector is valid (should be inside flash range)
#if defined(MIOS32_FAMILY_STM32F10x) || defined(MIOS32_FAMILY_STM32F4xx)
  u32 *reset_vector = (u32 *)0x08004004;
  if( (*reset_vector >> 24) == 0x08 ) {
    // reset all peripherals
#if defined(MIOS32_FAMILY_STM32F4xx)
    RCC_AHB1PeriphResetCmd(0xfffffff8, ENABLE); // don't reset GPIOA/AF due to USB pins
    RCC_AHB2PeriphResetCmd(0xffffff7f, ENABLE); // don't reset OTG_FS, so that the connectuion can survive
    RCC_APB1PeriphResetCmd(0xffffffff, ENABLE);
    RCC_APB2PeriphResetCmd(0xffffffff, ENABLE);
    RCC_AHB1PeriphResetCmd(0xffffffff, DISABLE);
    RCC_AHB2PeriphResetCmd(0xffffffff, DISABLE);
    RCC_APB1PeriphResetCmd(0xffffffff, DISABLE);
    RCC_APB2PeriphResetCmd(0xffffffff, DISABLE);
#else
# ifdef MIOS32_BOARD_STM32_PRIMER
    RCC_APB2PeriphResetCmd(0xfffffff0, ENABLE); // Primer: don't reset GPIOA/AF + GPIOB due to USB detach pin
# else
    RCC_APB2PeriphResetCmd(0xfffffff8, ENABLE); // MBHP_CORE_STM32: don't reset GPIOA/AF due to USB pins
# endif
    RCC_APB1PeriphResetCmd(0xff7fffff, ENABLE); // don't reset USB, so that the connection can survive!
    RCC_APB2PeriphResetCmd(0xffffffff, DISABLE);
    RCC_APB1PeriphResetCmd(0xffffffff, DISABLE);
#endif

    if( hold_mode_active_after_reset ) {
      // if hold mode activated via external pin, force re-initialisation by resetting USB
#if defined(MIOS32_FAMILY_STM32F4xx)
      RCC_AHB2PeriphResetCmd(0x00000080, ENABLE); // reset OTG_FS
      RCC_AHB2PeriphResetCmd(0x00000080, DISABLE);
#else
      RCC_APB1PeriphResetCmd(0x00800000, ENABLE); // reset USB device
      RCC_APB1PeriphResetCmd(0x00800000, DISABLE);
#endif
    } else {
      // no hold mode: ensure that USB interrupts won't be triggered while jumping into application
#if defined(STM32F10X_CL)
      if( MIOS32_USB_IsInitialized() )
	OTGD_FS_DisableGlobalInt();
#elif defined(MIOS32_FAMILY_STM32F4xx)
      if( MIOS32_USB_IsInitialized() && USB_OTG_dev.dev.class_cb != NULL ) {
	USB_OTG_DisableGlobalInt(&USB_OTG_dev);
      }
#else
      _SetCNTR(0); // clear USB interrupt mask
      _SetISTR(0); // clear all USB interrupt requests
#endif
    }

    // change stack pointer
    u32 *stack_pointer = (u32 *)0x08004000;
#elif defined(MIOS32_FAMILY_LPC17xx)
  u32 *reset_vector = (u32 *)0x00004004;
  if( (*reset_vector >> 24) == 0x00 ) {
    // reset all peripherals
    // -> no reset triggers available!!! :-(

    // ensure that at least commonly used interrupt functions are disabled
    LPC_UART0->IER = 0;
    LPC_UART1->IER = 0;
    LPC_UART2->IER = 0;
    LPC_UART3->IER = 0;

    // change stack pointer
    u32 *stack_pointer = (u32 *)0x00004000;
#else
# error "please adapt reset sequence for this family"
#endif


    u32 stack_pointer_value = *stack_pointer;
    __asm volatile (		       \
		    "   msr msp, %0\n" \
                    :: "r" (stack_pointer_value) \
                    );

    // branch to application
    void (*application)(void) = (void *)*reset_vector;
    application();
  }

  // otherwise flash LED fast (BSL failed to start application)
  while( 1 ) {
    MIOS32_STOPWATCH_Reset();
    MIOS32_BOARD_LED_Set(BSL_LED_MASK, BSL_LED_MASK);
    while( MIOS32_STOPWATCH_ValueGet() < 500 );

    MIOS32_STOPWATCH_Reset();
    MIOS32_BOARD_LED_Set(BSL_LED_MASK, 0);
    while( MIOS32_STOPWATCH_ValueGet() < 500 );
  }

  return 0; // will never be reached
}



/////////////////////////////////////////////////////////////////////////////
// This function resets the SRIO chains at J8/9, J16 and J19 by clocking
// 128 times and strobing the RCLK
//
// It's required since no reset line is available for MBHP_DIN/DOUT modules
//
// It doesn't hurt to shift the clocks to SPI devices as well, since the
// CS line is deactivated during the clocks are sent.
//
// All ports are configured for open drain mode. If push pull is expected, clocks
// won't have an effect as well (no real issue, since SRIOs are always
// used in OD mode)
/////////////////////////////////////////////////////////////////////////////
static s32 ResetSRIOChains(void)
{
#if SRIO_RESET_SUPPORTED == 0
  return -1; // not supported
#else

#if defined(MIOS32_FAMILY_STM32F10x) || defined(MIOS32_FAMILY_STM32F4xx)
  int i;

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = SRIO_GPIO_MODE;
#ifdef SRIO_GPIO_OTYPE
  GPIO_InitStructure.GPIO_OType = SRIO_GPIO_OTYPE;
#endif

  // init GPIO driver modes
  for(i=0; i<SRIO_NUM_SCLK_PINS; ++i) {
    MIOS32_SYS_STM_PINSET_1(srio_sclk_pin[i].port, srio_sclk_pin[i].pin_mask); // SCLK=1 by default
    GPIO_InitStructure.GPIO_Pin = srio_sclk_pin[i].pin_mask;
    GPIO_Init(srio_sclk_pin[i].port, &GPIO_InitStructure);
  }

  for(i=0; i<SRIO_NUM_RCLK_PINS; ++i) {
    MIOS32_SYS_STM_PINSET_1(srio_rclk_pin[i].port, srio_rclk_pin[i].pin_mask); // RCLK=1 by default
    GPIO_InitStructure.GPIO_Pin = srio_rclk_pin[i].pin_mask;
    GPIO_Init(srio_rclk_pin[i].port, &GPIO_InitStructure);
  }

  for(i=0; i<SRIO_NUM_MOSI_PINS; ++i) {
    MIOS32_SYS_STM_PINSET_0(srio_mosi_pin[i].port, srio_mosi_pin[i].pin_mask); // MOSI=0 by default
    GPIO_InitStructure.GPIO_Pin = srio_mosi_pin[i].pin_mask;
    GPIO_Init(srio_mosi_pin[i].port, &GPIO_InitStructure);
  }

#if defined(MIOS32_BOARD_STM32F4DISCOVERY) || defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
	// set RE3=1 to ensure that the on-board MEMs is disabled
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOE, &GPIO_InitStructure);	
	MIOS32_SYS_STM_PINSET_1(GPIOE, GPIO_Pin_3);
#else
# warning "Please doublecheck if RE3 has to be set to 1 to disable MEMs"
#endif

  // send 128 clocks to all SPI ports
  int cycle;
  for(cycle=0; cycle<128; ++cycle) {
    for(i=0; i<SRIO_NUM_SCLK_PINS; ++i) {
      MIOS32_SYS_STM_PINSET_0(srio_sclk_pin[i].port, srio_sclk_pin[i].pin_mask); // SCLK=0
    }

    MIOS32_DELAY_Wait_uS(1);

    for(i=0; i<SRIO_NUM_SCLK_PINS; ++i) {
      MIOS32_SYS_STM_PINSET_1(srio_sclk_pin[i].port, srio_sclk_pin[i].pin_mask); // SCLK=1
    }

    MIOS32_DELAY_Wait_uS(1);
  }

  // latch values
  for(i=0; i<SRIO_NUM_RCLK_PINS; ++i) {
    MIOS32_SYS_STM_PINSET_0(srio_rclk_pin[i].port, srio_rclk_pin[i].pin_mask); // RCLK=0
  }

  MIOS32_DELAY_Wait_uS(1);

  for(i=0; i<SRIO_NUM_RCLK_PINS; ++i) {
    MIOS32_SYS_STM_PINSET_1(srio_rclk_pin[i].port, srio_rclk_pin[i].pin_mask); // RCLK=1
  }

  MIOS32_DELAY_Wait_uS(1);

  // switch back driver modes to input with pull-up enabled
#if defined(MIOS32_FAMILY_STM32F10x)
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
#else
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
#endif

  // init GPIO driver modes
  for(i=0; i<SRIO_NUM_SCLK_PINS; ++i) {
    GPIO_InitStructure.GPIO_Pin = srio_sclk_pin[i].pin_mask;
    GPIO_Init(srio_sclk_pin[i].port, &GPIO_InitStructure);
  }

  for(i=0; i<SRIO_NUM_RCLK_PINS; ++i) {
    GPIO_InitStructure.GPIO_Pin = srio_rclk_pin[i].pin_mask;
    GPIO_Init(srio_rclk_pin[i].port, &GPIO_InitStructure);
  }

  for(i=0; i<SRIO_NUM_MOSI_PINS; ++i) {
    GPIO_InitStructure.GPIO_Pin = srio_mosi_pin[i].pin_mask;
    GPIO_Init(srio_mosi_pin[i].port, &GPIO_InitStructure);
  }

  return 0; // no error
#else
  int i;

  // init GPIO driver modes
  for(i=0; i<SRIO_NUM_SCLK_PINS; ++i) {
    MIOS32_SYS_LPC_PINSET(srio_sclk_pin[i].port, srio_sclk_pin[i].pin, 1); // SCLK=1 by default
    MIOS32_SYS_LPC_PINSEL(srio_sclk_pin[i].port, srio_sclk_pin[i].pin, 0); // select GPIO
    MIOS32_SYS_LPC_PINDIR(srio_sclk_pin[i].port, srio_sclk_pin[i].pin, 1); // enable output driver
  }

  for(i=0; i<SRIO_NUM_RCLK_PINS; ++i) {
    MIOS32_SYS_LPC_PINSET(srio_rclk_pin[i].port, srio_rclk_pin[i].pin, 1); // RCLK=1 by default
    MIOS32_SYS_LPC_PINSEL(srio_rclk_pin[i].port, srio_rclk_pin[i].pin, 0); // select GPIO
    MIOS32_SYS_LPC_PINDIR(srio_rclk_pin[i].port, srio_rclk_pin[i].pin, 1); // enable output driver
  }

  for(i=0; i<SRIO_NUM_MOSI_PINS; ++i) {
    MIOS32_SYS_LPC_PINSET(srio_mosi_pin[i].port, srio_mosi_pin[i].pin, 0); // MOSI=0 by default
    MIOS32_SYS_LPC_PINSEL(srio_mosi_pin[i].port, srio_mosi_pin[i].pin, 0); // select GPIO
    MIOS32_SYS_LPC_PINDIR(srio_mosi_pin[i].port, srio_mosi_pin[i].pin, 1); // enable output driver
  }

  // send 128 clocks to all SPI ports
  int cycle;
  for(cycle=0; cycle<128; ++cycle) {
    for(i=0; i<SRIO_NUM_SCLK_PINS; ++i)
      MIOS32_SYS_LPC_PINSET(srio_sclk_pin[i].port, srio_sclk_pin[i].pin, 0); // SCLK=0

    MIOS32_DELAY_Wait_uS(1);

    for(i=0; i<SRIO_NUM_SCLK_PINS; ++i)
      MIOS32_SYS_LPC_PINSET(srio_sclk_pin[i].port, srio_sclk_pin[i].pin, 1); // SCLK=1

    MIOS32_DELAY_Wait_uS(1);
  }

  // latch values
  for(i=0; i<SRIO_NUM_RCLK_PINS; ++i)
    MIOS32_SYS_LPC_PINSET(srio_rclk_pin[i].port, srio_rclk_pin[i].pin, 0); // RCLK=0

  MIOS32_DELAY_Wait_uS(1);

  for(i=0; i<SRIO_NUM_RCLK_PINS; ++i)
    MIOS32_SYS_LPC_PINSET(srio_rclk_pin[i].port, srio_rclk_pin[i].pin, 1); // RCLK=1

  MIOS32_DELAY_Wait_uS(1);

  // switch back driver modes to input with pull-up enabled

  // init GPIO driver modes
  for(i=0; i<SRIO_NUM_SCLK_PINS; ++i)
    MIOS32_SYS_LPC_PINDIR(srio_sclk_pin[i].port, srio_sclk_pin[i].pin, 0); // disable output driver

  for(i=0; i<SRIO_NUM_RCLK_PINS; ++i)
    MIOS32_SYS_LPC_PINDIR(srio_rclk_pin[i].port, srio_rclk_pin[i].pin, 0); // disable output driver

  for(i=0; i<SRIO_NUM_MOSI_PINS; ++i)
    MIOS32_SYS_LPC_PINDIR(srio_mosi_pin[i].port, srio_mosi_pin[i].pin, 0); // disable output driver

  return 0; // no error
#endif

#endif
}
