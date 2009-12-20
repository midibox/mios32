// $Id$
/*
 * MBHP_SID module driver
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

#include <FreeRTOS.h>
#include <portmacro.h>

#include "sid.h"


/////////////////////////////////////////////////////////////////////////////
// Help Macros
/////////////////////////////////////////////////////////////////////////////

#define PIN_SER(b)  { SID_SER_PORT->BSRR = (b) ? SID_SER_PIN : (SID_SER_PIN << 16); }

#define PIN_RCLK_0  { SID_RCLK_PORT->BRR  = SID_RCLK_PIN; }
#define PIN_RCLK_1  { SID_RCLK_PORT->BSRR = SID_RCLK_PIN; }

#define PIN_SCLK_0  { SID_SCLK_PORT->BRR  = SID_SCLK_PIN; }
#define PIN_SCLK_1  { SID_SCLK_PORT->BSRR = SID_SCLK_PIN; }

#define PIN_CSN0_0  { SID_CSN0_PORT->BRR  = SID_CSN0_PIN; }
#define PIN_CSN0_1  { SID_CSN0_PORT->BSRR = SID_CSN0_PIN; }
#define PIN_CSN1_0  { SID_CSN1_PORT->BRR  = SID_CSN1_PIN; }
#define PIN_CSN1_1  { SID_CSN1_PORT->BSRR = SID_CSN1_PIN; }



/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

sid_regs_t sid_regs[SID_NUM];
u8 sid_gate_update_done[SID_NUM];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

sid_regs_t sid_regs_shadow[SID_NUM]; // to determine register changes


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static void SID_SerWrite(u8 cs, u8 addr, u8 data, u8 reset);


/////////////////////////////////////////////////////////////////////////////
// Initializes the SID module
// IN: <mode>: currently only mode 0 supported
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 SID_Init(u32 mode)
{
  int sid, reg;

  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  // GPIO initialisation
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);

  // initial pin state
  PIN_SCLK_1;
  PIN_RCLK_1;
  PIN_SER(1);
  PIN_CSN0_1;
  PIN_CSN1_1;

  // configure push-pull pins
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; // weak driver to reduce transients

  GPIO_InitStructure.GPIO_Pin = SID_SCLK_PIN;
  GPIO_Init(SID_SCLK_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = SID_RCLK_PIN;
  GPIO_Init(SID_RCLK_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = SID_SER_PIN;
  GPIO_Init(SID_SER_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = SID_CSN0_PIN;
  GPIO_Init(SID_CSN0_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = SID_CSN1_PIN;
  GPIO_Init(SID_CSN1_PORT, &GPIO_InitStructure);

  // CLK pin outputs 1 MHz clock via Timer4 Channel 2
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Pin = SID_CLK_PIN;
  GPIO_Init(SID_CLK_PORT, &GPIO_InitStructure);

  // TIM2 clock enable
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_OCInitTypeDef TIM_OCInitStructure;

  // Time base configuration
  TIM_TimeBaseStructure.TIM_Period = 72; // generates 1 MHz @ 72 MHz (TODO: determine frequency via RCC_GetClocksFreq)
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

  // PWM1 Mode configuration: Channel2
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 72 / 2; // 1:1 duty cycle @ 72 MHz (TODO: determine frequency via RCC_GetClocksFreq)
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OC2Init(TIM4, &TIM_OCInitStructure);
  TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);

  TIM_ARRPreloadConfig(TIM4, ENABLE);
  TIM_Cmd(TIM4, ENABLE);


  // clear all SID registers
  for(sid=0; sid<SID_NUM; ++sid)
    sid_gate_update_done[sid] = 0x00;
    for(reg=0; reg<SID_REGS_NUM; ++reg) {
      sid_regs[sid].ALL[reg] = 0;
      sid_regs_shadow[sid].ALL[reg] = 0;
    }

  // force reset and transfer all values to SID registers
  SID_Update(2);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Updates all SID registers
// IN: <mode>: if 0: only register changes will be transfered to SID(s)
//             if 1: force transfer of all registers
//             if 2: reset SID, thereafter force transfer of all registers
// OUT: returns < 0 if update failed
/////////////////////////////////////////////////////////////////////////////
s32 SID_Update(u32 mode)
{
  int sid, reg, i;
  u8 update_order[SID_REGS_NUM] = { 
     0,  1,  2,  3,  5,  6, // voice 1 w/o osc control register
     7,  8,  9, 10, 12, 13, // voice 2 w/o osc control register
    14, 15, 16, 17, 19, 20, // voice 3 w/o osc control register
     4, 11, 18,             // voice 1/2/3 control registers
    21, 22, 23, 24,         // remaining SID registers
    25, 26, 27, 28, 29, 30, 31 // SwinSID registers
  };

  // trigger reset?
  if( mode == 2 )
    SID_SerWrite(0xff, 0x00, 0x00, 1); // CS lines, address, data, reset

  // if register update should be forced, just inverse all shadow register values
  if( mode >= 1 ) { // (also for reset mode)
    for(sid=0; sid<SID_NUM; ++sid)
      for(reg=0; reg<SID_REGS_NUM; ++reg)
	sid_regs_shadow[sid].ALL[reg] = ~sid_regs[sid].ALL[reg];
  }

  // TODO: here we should disable the SID SE update interrupt - once available

  // this loop should run so fast as possible, 
  // we consider to update two SIDs at once if values are identical
  // currently only optimized for 2 SIDs!
  for(i=0; i<SID_REGS_NUM; ++i) {
    u8 data;

    reg = update_order[i];

    // check if we have to update the first SID
    if( (data=sid_regs[0].ALL[reg]) != sid_regs_shadow[0].ALL[reg] ) {
      // check if the value of the second SID is identical
      if( data == sid_regs[1].ALL[reg] ) {
	SID_SerWrite(0x03, reg, data, 0); // CS lines, address, data, reset
        sid_regs_shadow[0].ALL[reg] = data;
        sid_regs_shadow[1].ALL[reg] = data;
      } else {
	SID_SerWrite(0x01, reg, data, 0); // CS lines, address, data, reset
        sid_regs_shadow[0].ALL[reg] = data;
      }
    }

    // update for second SID required?
    if( (data=sid_regs[1].ALL[reg]) != sid_regs_shadow[1].ALL[reg] ) {
      SID_SerWrite(0x02, reg, data, 0); // CS lines, address, data, reset
      sid_regs_shadow[1].ALL[reg] = data;
    }
    sid_gate_update_done[i] = 0xff;
  }

  // TODO: here we should enable the SID SE update interrupt - once available


  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Transfers a address and data byte to the 74HC595 registers of the 
// MBHP_SID module
// IN: chip select pins which should be activated in <cs> (inversion done internally)
//     8bit data in <data>
//     5bit address in <addr> (bit 7..5 *must* be 0!)
//     reset pin in <reset> (inversion done internally)
// OUT: -
/////////////////////////////////////////////////////////////////////////////
static void SID_SerWrite(u8 cs, u8 addr, u8 data, u8 reset)
{
  // low-active reset is connected to "A6" of the first SR
  if( !reset )
    addr |= (1 << 5);

  // shift data
  PIN_SER(data & 0x01); // D0
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(data & 0x02); // D1
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(data & 0x04); // D2
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(data & 0x08); // D3
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(data & 0x10); // D4
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(data & 0x20); // D5
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(data & 0x40); // D6
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(data & 0x80); // D7
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;

  PIN_SER(addr & 0x01); // A0
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(addr & 0x02); // A1
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(addr & 0x04); // A2
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(addr & 0x08); // A3
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(addr & 0x10); // A4
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(addr & 0x20); // RESET#
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(addr & 0x40); // n.c
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;
  PIN_SER(addr & 0x80); // n.c
  PIN_SCLK_0; // setup delay
  PIN_SCLK_1;

  // transfer to output register
  PIN_RCLK_1;
  PIN_RCLK_1;
  PIN_RCLK_1;
  PIN_RCLK_0;

  // timing critical routine: we have to avoid setup or hold violations when triggering
  // the CS line, otherwise a OSC gate could be triggered twice
  // TODO: check if this is only required for OSC control registers - it could save some time :)

  MIOS32_IRQ_Disable();

  // synchronize with rising edge of SID clock
  while(  (SID_CLK_PORT->IDR & SID_CLK_PIN) ); // wait for 0
  while( !(SID_CLK_PORT->IDR & SID_CLK_PIN) ); // wait for 1

  // set CS lines
  if( cs & 0x01 ) PIN_CSN0_0;
  if( cs & 0x02 ) PIN_CSN1_0;

  // at scope we can see, that we've reached the falling clock edge here

  // wait for 1.2 S (> one SID clock cycle)
  volatile u32 delay_ctr;
  for(delay_ctr=0; delay_ctr<4; ++delay_ctr); // measured with scope
  // TODO: delay could change with different gcc versions - check for better approach
  // Note that we shouldn't generate bus accesses while waiting, since they could be delayed by DMA transfers

  // release CS lines
  PIN_CSN0_1;
  PIN_CSN1_1;
  MIOS32_IRQ_Enable();
}

