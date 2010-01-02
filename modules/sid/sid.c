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

#include "sid.h"
#include "sidemu.h"


/////////////////////////////////////////////////////////////////////////////
// Help Macros
/////////////////////////////////////////////////////////////////////////////

#define PIN_SER(b)  { SID_SER_PORT->BSRR = (b) ? SID_SER_PIN : (SID_SER_PIN << 16); }

#define PIN_RCLK_0  { SID_RCLK_PORT->BRR  = SID_RCLK_PIN; }
#define PIN_RCLK_1  { SID_RCLK_PORT->BSRR = SID_RCLK_PIN; }

#define PIN_SCLK_0  { SID_SCLK_PORT->BRR  = SID_SCLK_PIN; }
#define PIN_SCLK_1  { SID_SCLK_PORT->BSRR = SID_SCLK_PIN; }


/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

sid_regs_t sid_regs[SID_NUM];


/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static sid_regs_t sid_regs_shadow[SID_NUM]; // to determine register changes

static const u8 update_order[SID_REGS_NUM] = { 
   0,  1,  2,  3,  5,  6, // voice 1 w/o osc control register
   7,  8,  9, 10, 12, 13, // voice 2 w/o osc control register
  14, 15, 16, 17, 19, 20, // voice 3 w/o osc control register
   4, 11, 18,             // voice 1/2/3 control registers
  21, 22, 23, 24,         // remaining SID registers
  25, 26, 27, 28, 29, 30, 31 // SwinSID registers
};



typedef struct {
  GPIO_TypeDef *port;
  u16 pin_mask;
} sid_cs_pin_t;

static const sid_cs_pin_t sid_cs_pin[SID_NUM] = {
  { SID_CSN0_PORT, SID_CSN0_PIN },
#if SID_NUM >= 2
  { SID_CSN1_PORT, SID_CSN1_PIN },
#endif
#if SID_NUM >= 3
  { SID_CSN2_PORT, SID_CSN2_PIN },
#endif
#if SID_NUM >= 4
  { SID_CSN3_PORT, SID_CSN3_PIN },
#endif
#if SID_NUM >= 5
  { SID_CSN4_PORT, SID_CSN4_PIN },
#endif
#if SID_NUM >= 6
  { SID_CSN5_PORT, SID_CSN5_PIN },
#endif
#if SID_NUM >= 7
  { SID_CSN6_PORT, SID_CSN6_PIN },
#endif
#if SID_NUM >= 8
  { SID_CSN7_PORT, SID_CSN7_PIN },
#endif
};


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

static inline void SID_UpdateReg(sid_cs_pin_t *cs_pin0, sid_cs_pin_t *cs_pin1, u8 cs, u8 addr, u8 data, u8 reset);


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

#ifdef SIDEMU_ENABLED
  SYNTH_Init(0);
#endif

#ifndef SIDPHYS_DISABLED
  // GPIO initialisation
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);

  // initial pin state
  PIN_SCLK_1;
  PIN_RCLK_1;
  PIN_SER(1);
  for(sid=0; sid<SID_NUM; ++sid)
    sid_cs_pin[sid].port->BSRR = sid_cs_pin[sid].pin_mask;

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
  GPIO_InitStructure.GPIO_Pin = SID_CSN2_PIN;
  GPIO_Init(SID_CSN2_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = SID_CSN3_PIN;
  GPIO_Init(SID_CSN3_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = SID_CSN4_PIN;
  GPIO_Init(SID_CSN4_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = SID_CSN5_PIN;
  GPIO_Init(SID_CSN5_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = SID_CSN6_PIN;
  GPIO_Init(SID_CSN6_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = SID_CSN7_PIN;
  GPIO_Init(SID_CSN7_PORT, &GPIO_InitStructure);

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
  // Note: whenever PWM channel is changed, the synchronisation code in SID_UpdateReg has to be updated as well!
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 72 / 2; // 1:1 duty cycle @ 72 MHz (TODO: determine frequency via RCC_GetClocksFreq)
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // not Low! This ensures, that TIM_FLAG_CC2 is set on *rising* edge
  TIM_OC2Init(TIM4, &TIM_OCInitStructure);
  TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);

  TIM_ARRPreloadConfig(TIM4, ENABLE);
  TIM_Cmd(TIM4, ENABLE);
#endif

  // clear all SID registers
  for(sid=0; sid<SID_NUM; ++sid)
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

  // trigger reset?
  if( mode == 2 ) {
    // (cs pins don't need to be set during reset, but the SID_UpdateReg() routine requires the pointers)
    sid_cs_pin_t *cs_pin0 = (sid_cs_pin_t *)&sid_cs_pin[0];
    sid_cs_pin_t *cs_pin1 = (sid_cs_pin_t *)&sid_cs_pin[1];
    SID_UpdateReg(cs_pin0, cs_pin1, 0xff, 0x00, 0x00, 1); // CS lines, address, data, reset
  }

  // if register update should be forced, just inverse all shadow register values
  if( mode >= 1 ) { // (also for reset mode)
    for(sid=0; sid<SID_NUM; ++sid)
      for(reg=0; reg<SID_REGS_NUM; ++reg)
	sid_regs_shadow[sid].ALL[reg] = ~sid_regs[sid].ALL[reg];
  }

  // this loop should run so fast as possible, 
  // we consider to update two SIDs at once if values are identical
  for(sid=0; sid<SID_NUM; sid+=2) {
    u8 *update_order_ptr = (u8 *)&update_order[0];
    u8 *sidl = (u8 *)&sid_regs[sid+0].ALL[0];
    u8 *sidl_shadow = (u8 *)&sid_regs_shadow[sid+0].ALL[0];
    u8 *sidr = (u8 *)&sid_regs[sid+1].ALL[0];
    u8 *sidr_shadow = (u8 *)&sid_regs_shadow[sid+1].ALL[0];
    u8 cs_both = (3 << (2*sid));
    u8 cs_l_only = (1 << (2*sid));
    u8 cs_r_only = (2 << (2*sid));
    sid_cs_pin_t *cs_pin0 = (sid_cs_pin_t *)&sid_cs_pin[2*sid+0];
    sid_cs_pin_t *cs_pin1 = (sid_cs_pin_t *)&sid_cs_pin[2*sid+1];

    for(i=0; i<SID_REGS_NUM; ++i, update_order_ptr++) {
      u8 data;

      reg = *update_order_ptr;

      // check if update of left/right channel SID are required
      // partly duplicated code ensures best performance in all cases!
      if( (data=sidl[reg]) != sidl_shadow[reg] ) {
	// check if the value of the second SID is identical
	if( data == sidr[reg] ) {
	  SID_UpdateReg(cs_pin0, cs_pin1, cs_both, reg, data, 0); // CS lines, address, data, reset
	  sidl_shadow[reg] = data;
	  sidr_shadow[reg] = data;
	} else {
	  SID_UpdateReg(cs_pin0, NULL, cs_l_only, reg, data, 0); // CS lines, address, data, reset
	  sidl_shadow[reg] = data;

	  if( (data=sidr[reg]) != sidr_shadow[reg] ) {
	    // individual update for second SID required
	    SID_UpdateReg(cs_pin1, NULL, cs_r_only, reg, data, 0); // CS lines, address, data, reset
	    sidr_shadow[reg] = data;
	  }
	}
      } else if( (data=sidr[reg]) != sidr_shadow[reg] ) {
	// individual update for second SID required
	SID_UpdateReg(cs_pin1, NULL, cs_r_only, reg, data, 0); // CS lines, address, data, reset
	sidr_shadow[reg] = data;
      }
    }
  }

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
static inline void SID_UpdateReg(sid_cs_pin_t *cs_pin0, sid_cs_pin_t *cs_pin1, u8 cs, u8 addr, u8 data, u8 reset)
{
  //  SID_Wrapper_Write(cs, addr, data, reset);

#ifdef SIDEMU_ENABLED
  // currently only single SID available
  if( (cs & 1) && addr <= 24 )
    SIDEMU_setRegister(addr, data);
#endif

  // low-active reset is connected to "A6" of the first SR
  if( !reset )
    addr |= (1 << 5);

#ifndef SIDPHYS_DISABLED
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
  TIM4->SR &= ~TIM_FLAG_CC2;
  while( !(TIM4->SR & TIM_FLAG_CC2) );

  // set CS lines
  cs_pin0->port->BRR = cs_pin0->pin_mask;
  if( cs_pin1 ) // Note: only second pin is optional! A second if() check could lead to an additional unwanted delay of 1 SID cycle!!!
    cs_pin1->port->BRR = cs_pin1->pin_mask;

  // at scope we can see, that we've reached the falling clock edge here

  // CS/data has to be stable during *falling* edge of SID clock
  // Write data hold time is 350 nS (according to datasheet)

  // synchronize with next rising edge of SID clock
  TIM4->SR &= ~TIM_FLAG_CC2;
  while( !(TIM4->SR & TIM_FLAG_CC2) );

  // release CS lines
  cs_pin0->port->BSRR = cs_pin0->pin_mask;
  if( cs_pin1 ) // Note: only second pin is optional! A second if() check can lead to an additional unwanted delay of 1 SID cycle!!!
    cs_pin1->port->BSRR = cs_pin1->pin_mask;

  // enable interrupts again
  MIOS32_IRQ_Enable();
#endif
}

