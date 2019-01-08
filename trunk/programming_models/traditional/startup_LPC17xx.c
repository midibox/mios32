//$Id$
/**
 ******************************************************************************
 * @file      startup_LPC17xx.c
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
typedef void( *const intfunc )( void );

/* Private define ------------------------------------------------------------*/
#define WEAK __attribute__ ((weak))
#define ALIAS(f) __attribute__ ((weak, alias (#f)))

/* Private macro -------------------------------------------------------------*/
extern unsigned long _etext;
/* start address for the initialization values of the .data section. 
defined in linker script */
extern unsigned long _sidata;

/* start address for the .data section. defined in linker script */    
extern unsigned long _sdata;

/* end address for the .data section. defined in linker script */    
extern unsigned long _edata;
    
/* start address for the .bss section. defined in linker script */
extern unsigned long _sbss;
extern unsigned long _sbss_ahb;

/* end address for the .bss section. defined in linker script */      
extern unsigned long _ebss;  
extern unsigned long _ebss_ahb;  
    
/* init value for the stack pointer. defined in linker script */
extern unsigned long _estack;

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void Reset_Handler(void) __attribute__((__interrupt__));
extern int main(void);
extern void __libc_init_array(void);  /* calls CTORS of static objects */
void __Init_Data(void);

// for FreeRTOS
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);
extern void vPortSVCHandler( void );

/******************************************************************************
*
* Forward declaration of the default fault handlers.
*
*******************************************************************************/
void WEAK Reset_Handler(void);
void WEAK NMI_Handler(void);
void WEAK HardFault_Handler(void);
void WEAK MemManage_Handler(void);
void WEAK BusFault_Handler(void);
void WEAK UsageFault_Handler(void);
void WEAK MemManage_Handler(void);
void WEAK SVC_Handler(void);
void WEAK DebugMon_Handler(void);
void WEAK PendSV_Handler(void);
void WEAK SysTick_Handler(void);

void WEAK Default_Handler(void);

/* External Interrupts */
void WDT_IRQHandler(void) ALIAS(Default_Handler);
void TIMER0_IRQHandler(void) ALIAS(Default_Handler);
void TIMER1_IRQHandler(void) ALIAS(Default_Handler);
void TIMER2_IRQHandler(void) ALIAS(Default_Handler);
void TIMER3_IRQHandler(void) ALIAS(Default_Handler);
void UART0_IRQHandler(void) ALIAS(Default_Handler);
void UART1_IRQHandler(void) ALIAS(Default_Handler);
void UART2_IRQHandler(void) ALIAS(Default_Handler);
void UART3_IRQHandler(void) ALIAS(Default_Handler);
void PWM1_IRQHandler(void) ALIAS(Default_Handler);
void I2C0_IRQHandler(void) ALIAS(Default_Handler);
void I2C1_IRQHandler(void) ALIAS(Default_Handler);
void I2C2_IRQHandler(void) ALIAS(Default_Handler);
void SPI_IRQHandler(void) ALIAS(Default_Handler);
void SSP0_IRQHandler(void) ALIAS(Default_Handler);
void SSP1_IRQHandler(void) ALIAS(Default_Handler);
void PLL0_IRQHandler(void) ALIAS(Default_Handler);
void RTC_IRQHandler(void) ALIAS(Default_Handler);
void EINT0_IRQHandler(void) ALIAS(Default_Handler);
void EINT1_IRQHandler(void) ALIAS(Default_Handler);
void EINT2_IRQHandler(void) ALIAS(Default_Handler);
void EINT3_IRQHandler(void) ALIAS(Default_Handler);
void ADC_IRQHandler(void) ALIAS(Default_Handler);
void BOD_IRQHandler(void) ALIAS(Default_Handler);
void USB_IRQHandler(void) ALIAS(Default_Handler);
void CAN_IRQHandler(void) ALIAS(Default_Handler);
void DMA_IRQHandler(void) ALIAS(Default_Handler);
void I2S_IRQHandler(void) ALIAS(Default_Handler);
void ENET_IRQHandler(void) ALIAS(Default_Handler);
void RIT_IRQHandler(void) ALIAS(Default_Handler);
void MCPWM_IRQHandler(void) ALIAS(Default_Handler);
void QEI_IRQHandler(void) ALIAS(Default_Handler);
void PLL1_IRQHandler(void) ALIAS(Default_Handler);
void USBActivity_IRQHandler(void) ALIAS(Default_Handler);
void CANActivity_IRQHandler(void) ALIAS(Default_Handler);

/* Private functions ---------------------------------------------------------*/
/******************************************************************************
*
* The minimal vector table for a Cortex M3.  Note that the proper constructs
* must be placed on this to ensure that it ends up at physical address
* 0x0000.0000.
*
******************************************************************************/

__attribute__ ((section(".isr_vector")))
void (* const g_pfnVectors[])(void) =
{       
    (intfunc)((unsigned long)&_estack), /* The initial stack pointer */
    Reset_Handler,              /* Reset Handler */
    NMI_Handler,                /* NMI Handler */
    HardFault_Handler,          /* Hard Fault Handler */
    MemManage_Handler,          /* MPU Fault Handler */
    BusFault_Handler,           /* Bus Fault Handler */
    UsageFault_Handler,         /* Usage Fault Handler */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */

#if 0
    SVC_Handler,                /* SVCall Handler */
#else
    // for FreeRTOS
    vPortSVCHandler,
#endif

    DebugMon_Handler,           /* Debug Monitor Handler */
    0,                          /* Reserved */

#if 0
    PendSV_Handler,             /* PendSV Handler */
    SysTick_Handler,            /* SysTick Handler */
#else
    // for FreeRTOS
    xPortPendSVHandler,
    xPortSysTickHandler,
#endif

    /* External Interrupts */
    WDT_IRQHandler,							// 16, 0x40 - WDT
    TIMER0_IRQHandler,						// 17, 0x44 - TIMER0
    TIMER1_IRQHandler,						// 18, 0x48 - TIMER1
    TIMER2_IRQHandler,						// 19, 0x4c - TIMER2
    TIMER3_IRQHandler,						// 20, 0x50 - TIMER3
    UART0_IRQHandler,						// 21, 0x54 - UART0
    UART1_IRQHandler,						// 22, 0x58 - UART1
    UART2_IRQHandler,						// 23, 0x5c - UART2
    UART3_IRQHandler,						// 24, 0x60 - UART3
    PWM1_IRQHandler,						// 25, 0x64 - PWM1
    I2C0_IRQHandler,						// 26, 0x68 - I2C0
    I2C1_IRQHandler,						// 27, 0x6c - I2C1
    I2C2_IRQHandler,						// 28, 0x70 - I2C2
    SPI_IRQHandler,							// 29, 0x74 - SPI
    SSP0_IRQHandler,						// 30, 0x78 - SSP0
    SSP1_IRQHandler,						// 31, 0x7c - SSP1
    PLL0_IRQHandler,						// 32, 0x80 - PLL0 (Main PLL)
    RTC_IRQHandler,							// 33, 0x84 - RTC
    EINT0_IRQHandler,						// 34, 0x88 - EINT0
    EINT1_IRQHandler,						// 35, 0x8c - EINT1
    EINT2_IRQHandler,						// 36, 0x90 - EINT2
    EINT3_IRQHandler,						// 37, 0x94 - EINT3
    ADC_IRQHandler,							// 38, 0x98 - ADC
    BOD_IRQHandler,							// 39, 0x9c - BOD
    USB_IRQHandler,							// 40, 0xA0 - USB
    CAN_IRQHandler,							// 41, 0xa4 - CAN
    DMA_IRQHandler,							// 42, 0xa8 - GP DMA
    I2S_IRQHandler,							// 43, 0xac - I2S
    ENET_IRQHandler,						// 44, 0xb0 - Ethernet
    RIT_IRQHandler,							// 45, 0xb4 - RITINT
    MCPWM_IRQHandler,						// 46, 0xb8 - Motor Control PWM
    QEI_IRQHandler,							// 47, 0xbc - Quadrature Encoder
    PLL1_IRQHandler,						// 48, 0xc0 - PLL1 (USB PLL)
    USBActivity_IRQHandler,					// 49, 0xc4 - USB Activity interrupt to wakeup
    CANActivity_IRQHandler, 				// 50, 0xc8 - CAN Activity interrupt to wakeup
};

/**
 * @brief  This is the code that gets called when the processor first
 *          starts execution following a reset event. Only the absolutely
 *          necessary set is performed, after which the application
 *          supplied main() routine is called. 
 * @param  None
 * @retval : None
*/

void Reset_Handler(void)
{
  /* Initialize data and bss */
  unsigned long *pulSrc, *pulDest;

  /* Copy the data segment initializers from flash to SRAM */
  pulSrc = &_sidata;

  for(pulDest = &_sdata; pulDest < &_edata; )
  {
    *(pulDest++) = *(pulSrc++);
  }

  /* Zero fill the bss segment. */
  for(pulDest = &_sbss; pulDest < &_ebss; )
  {
    *(pulDest++) = 0;
  }

  /* Zero fill the bss_ahb segment. */
  for(pulDest = &_sbss_ahb; pulDest < &_ebss_ahb; )
  {
    *(pulDest++) = 0;
  }

  /* Call the application's entry point.*/
  main();

  //
  // we should never reach this point
  // however, wait endless...
  //
  while( 1 );
}

// dummy for newer gcc versions
void _init()
{
}

/*******************************************************************************
*
* Provide weak aliases for each Exception handler to the Default_Handler. 
* As they are weak aliases, any function with the same name will override 
* this definition.
*
*******************************************************************************/
#pragma weak NMI_Handler = Default_Handler
#pragma weak MemManage_Handler = Default_Handler
#pragma weak BusFault_Handler = Default_Handler
#pragma weak HardFault_Handler = Default_Handler
#pragma weak UsageFault_Handler = Default_Handler
#pragma weak SVC_Handler = Default_Handler
#pragma weak DebugMon_Handler = Default_Handler
#pragma weak PendSV_Handler = Default_Handler
#pragma weak SysTick_Handler = Default_Handler

/**
 * @brief  This is the code that gets called when the processor receives an 
 *         unexpected interrupt.  This simply enters an infinite loop, preserving
 *         the system state for examination by a debugger.
 *
 * @param  None     
 * @retval : None       
*/

void Default_Handler(void) 
{
  /* Go into an infinite loop. */
  while (1) 
  {
    // TK: TODO - insert an error notification here
    // We could send a debug message via USB, but this could be critical if there is
    // an issue in the MIDI or USB handler or related application hooks
    //
    // Alternatively we could flash the On-Board LED - it's safe!
  }
}

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
