/*
 * MBHP_DMX512 module driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Phil Taylor (phil@taylor.org.uk)
 *		As usual with MASSIVE help from TK!
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * The default configuration is currently to use the first serial port for DMX
 * This can be changed in dmx.h
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <FreeRTOS.h>
#include <portmacro.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <FreeRTOS.h>
#include <portmacro.h>

#include "dmx.h"

#define PRIORITY_TASK_DMX		( tskIDLE_PRIORITY + 3 )

xSemaphoreHandle xDMXSemaphore;

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////
static u8 dmx_tx_buffer[DMX_UNIVERSE_SIZE]; // Buffer for outgoing DMX universe.

static u16 dmx_baudrate_brr;		// This stores the contents of the BRR register for DMX sending
static u16 break_baudrate_brr;	// This is the BRR register when sending a break.
static volatile u16	dmx_current_channel;
static volatile u8 dmx_state;

static void TASK_DMX(void *pvParameters);

////////////////////////////////////////////////////////////////////////////
//! Initialize DMX Interface
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 DMX_Init(u32 mode)
{

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode
  // configure UART pins
	GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

  // outputs as push-pull
  GPIO_InitStructure.GPIO_Pin = DMX_TX_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(DMX_TX_PORT, &GPIO_InitStructure);	
	
  // inputs with internal pull-up
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = DMX_RX_PIN;
  GPIO_Init(DMX_RX_PORT, &GPIO_InitStructure);
	
  // enable USART clock
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	// Set DMX data format and baud rate (8 bit, 2 stop bits and 250000 baud)
  USART_InitTypeDef USART_InitStructure;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_2;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

  USART_InitStructure.USART_BaudRate = BREAK_BAUDRATE;
  USART_Init(DMX, &USART_InitStructure);
  break_baudrate_brr=DMX->BRR;	// Store the BRR value for quick changes.

  USART_InitStructure.USART_BaudRate = DMX_BAUDRATE;
  USART_Init(DMX, &USART_InitStructure);
  dmx_baudrate_brr=DMX->BRR;	// Store the BRR value for quick changes.
	
	// configure and enable UART interrupts
  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_InitStructure.NVIC_IRQChannel = DMX_IRQ_CHANNEL;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 11; //MIOS32_IRQ_UART_PRIORITY; // defined in mios32_irq.h
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	USART_Cmd(DMX, ENABLE); 
	dmx_state=DMX_IDLE;
	dmx_current_channel=0;
	// Create timer to send DMX universe.
	vSemaphoreCreateBinary(xDMXSemaphore);
  xTaskCreate(TASK_DMX, "DMX", configMINIMAL_STACK_SIZE, NULL, PRIORITY_TASK_DMX, NULL);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// This task handles sending the DMX universe
/////////////////////////////////////////////////////////////////////////////
static void TASK_DMX(void *pvParameters)
{
  portTickType xLastExecutionTime;

  // Initialise the xLastExecutionTime variable on task entry
  xLastExecutionTime = xTaskGetTickCount();

  while( 1 ) {
    vTaskDelayUntil(&xLastExecutionTime, 35 / portTICK_RATE_MS);
		if (dmx_state==DMX_IDLE)
		{
		  if (xSemaphoreTake(xDMXSemaphore, 10)==pdTRUE) { // Stop the universe from being changed while we are sending it.
		    // send break and MAB with slow baudrate
		    DMX->SR &= ~USART_FLAG_TC;
		    USART_ITConfig(DMX, USART_IT_TC, ENABLE); // enable TC interrupt - triggered when transmission of break/MAB is completed
		    DMX->BRR=break_baudrate_brr;		// Set baudrate to 90.9K so send break/MAB
		    dmx_state=DMX_BREAK;
		    //DMX->DR = 0x80;                           // start transmission (MSB used for MAB)
		    DMX->DR = 0x00;                           // start transmission (stop bits provide MAB)
		  }
		}
  }
}

/////////////////////////////////////////////////////////////////////////////
// Change the value of a single channel
/////////////////////////////////////////////////////////////////////////////
s32 DMX_SetChannel(u16 channel, u8 value)
{
	
	if (channel<DMX_UNIVERSE_SIZE) {
			//if (xSemaphoreTake(xDMXSemaphore, 1)==pdTRUE)
			//{
				dmx_tx_buffer[channel]=value;
			//	xSemaphoreGive(xDMXSemaphore);
			//}
	}
	else 
		return -1;
	return 0;
	
	}

/////////////////////////////////////////////////////////////////////////////
// Get the value of a single channel
/////////////////////////////////////////////////////////////////////////////
s32 DMX_GetChannel(u16 channel)
{
	if (channel<DMX_UNIVERSE_SIZE) {
		u32 val=(s32)dmx_tx_buffer[channel];
		return val;
	}
	else 
		return -1;
}



/////////////////////////////////////////////////////////////////////////////
// Interrupt handler for DMX UART
/////////////////////////////////////////////////////////////////////////////
signed portBASE_TYPE x=pdFALSE;
DMX_IRQHANDLER_FUNC
{
  if( (dmx_state == DMX_BREAK) && (DMX->SR & USART_FLAG_TC) ) { // Transmission Complete flag
    // the combined break/MAB has been sent - disable TC interrupt, clear current TXE and enable TXE for next byte
    USART_ITConfig(DMX, USART_IT_TC, DISABLE);
    DMX->SR &= ~USART_FLAG_TXE;
    USART_ITConfig(DMX, USART_IT_TXE, ENABLE);

    dmx_current_channel=0;
    DMX->BRR=dmx_baudrate_brr;		// Set baudrate to 250K to send universe
    DMX->DR = 0x00; // start code
    dmx_state=DMX_SENDING;
  }

  if( DMX->SR & USART_FLAG_TXE ) {
    // send next byte
    if( (dmx_state==DMX_SENDING) && (dmx_current_channel < DMX_UNIVERSE_SIZE) ) {
      s32 c=DMX_GetChannel(dmx_current_channel);
      DMX->DR=c; 
      dmx_current_channel++;
    } else {
      // all bytes have been sent
      dmx_state = DMX_IDLE;
      USART_ITConfig(DMX, USART_IT_TXE, DISABLE);
      MIOS32_BOARD_LED_Set(0xffffffff, ~MIOS32_BOARD_LED_Get());
      xSemaphoreGiveFromISR(xDMXSemaphore,&x);
    }
  }
}


	
