// $Id: opl3.c $
/*
 * MBHP_OPL3 module driver
 * 
 * ==========================================================================
 *
 *  Copyright (C) 2012 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *  
 *  Partially based on SID module driver from:
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

#include "opl3.h"

/////////////////////////////////////////////////////////////////////////////
// Help Macros
/////////////////////////////////////////////////////////////////////////////

# define OPL3_PIN_RS_0  { MIOS32_SYS_STM_PINSET(OPL3_RS_PORT, OPL3_RS_PIN, 0); }
# define OPL3_PIN_RS_1  { MIOS32_SYS_STM_PINSET(OPL3_RS_PORT, OPL3_RS_PIN, 1); }

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

opl3_operator_t opl3_operators[36*OPL3_COUNT];
opl3_channel_t opl3_channels[18*OPL3_COUNT];
opl3_chip_t opl3_chip[OPL3_COUNT];

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

// Queues for what registers need to be updated
static u16 opl3_op_queue[36*5*OPL3_COUNT];
static u8 opl3_chan_queue[18*3*OPL3_COUNT];
static u8 opl3_chip_queue[4*OPL3_COUNT];
static u16 op_queue_idx;
static u8 chan_queue_idx;
static u8 chip_queue_idx;


/////////////////////////////////////////////////////////////////////////////
// Lookup tables
/////////////////////////////////////////////////////////////////////////////

//Put defined pins/masks from preprocessor into Flash
static const u32 OPL3CSPins [OPL3_COUNT] = OPL3_CS_PINS;
static const u32 OPL3CSMasks[OPL3_COUNT] = OPL3_CS_MASKS;

static const u8 OPL3OperRegBegin[5] = {
  0x20, 0x40, 0x60, 0x80, 0xE0
};

static const u8 OPL3ChanRegBegin[3] = {
  0xA0, 0xB0, 0xC0
};

static const u8 OPL3ChipReg[4] = {
  0x05, 0x08, 0xBD, 0x04
};
static const u8 OPL3ChipRegHigh[4] = {
  0x01, 0x00, 0x00, 0x01
};

static const u8 OPL3RegOffset[18] = {
  //For OPL3's own channel:
  0x00, 0x03, //0
  0x01, 0x04, //1
  0x02, 0x05, //2
  0x08, 0x0B, //3
  0x09, 0x0C, //4
  0x0A, 0x0D, //5
  
  0x10, 0x13, //6
  0x11, 0x14, //7
  0x12, 0x15  //8
};

static const u8 OPL3ChannelMap[18] = {
   0,  3,  1,  4,  2,  5,
   9, 12, 10, 13, 11, 14,
  15, 16, 17,
   6,  7,  8
};



/////////////////////////////////////////////////////////////////////////////
// Functions
/////////////////////////////////////////////////////////////////////////////

/*
  Pinout:
  OPL3 D7:0 -> PE15:8
  OPL3 A1:0 -> PE7:6
*/

s32 OPL3_SendAddrData(u8 chip, u8 addrhigh, u8 addr, u8 data){
  //Turn off interrupts
  MIOS32_IRQ_Disable();
  //-------------------------------------------------------------
  //Write address, high address bit, and appropriate #CS low
  //-------------------------------------------------------------
  u32 porte = GPIOE->ODR;
  porte &= 0xFFFF003F; //Mask out the things we will set
  u32 a = addr; //Put in address
  a <<= 1; //Make room for high bit
  a |= (addrhigh > 0) & 1; //Add high bit
  a <<= 7; //Move over into place, A0 == 0
  porte |= a; //Write to our temp copy
  u32 mask = OPL3CSMasks[chip]; //Get value to mask to set CS low
  a = ~mask; //Invert mask
  porte &= a; //AND mask, bringing CS low
  GPIOE->ODR = porte; //Write
  porte |= mask; //OR mask, bringing CS high
  a = ~a; //Waste a cycle
  a = ~a; //Waste a cycle
  GPIOE->ODR = porte; //Write
  //-------------------------------------------------------------
  //Wait 32 times clock rate (at 14 Mhz, this would be 2.3 us)
  //-------------------------------------------------------------
  MIOS32_DELAY_Wait_uS(1);
  MIOS32_DELAY_Wait_uS(1);
  MIOS32_DELAY_Wait_uS(1);
  //-------------------------------------------------------------
  //Write data
  //-------------------------------------------------------------
  porte = GPIOE->ODR;
  porte &= 0xFFFF00BF; //Mask out the things we will set
  a = data; //Put in data
  a <<= 8; //Move into place
  a |= (1 << 6); //Or in bit saying it's data, A0 == 1
  porte |= a; //Write to our temp copy
  a = ~mask; //Invert mask
  porte &= a; //AND mask, bringing CS low
  GPIOE->ODR = porte; //Write
  porte |= mask; //OR mask, bringing CS high
  a = ~a; //Waste a cycle
  a = ~a; //Waste a cycle
  GPIOE->ODR = porte; //Write
  //-------------------------------------------------------------
  //Wait enough time that this won't be called again before 32 times clock rate
  //-------------------------------------------------------------
  MIOS32_DELAY_Wait_uS(1);
  //Turn on interrupts
  MIOS32_IRQ_Enable();
  return 0;
}

s32 OPL3_RefreshOperator(u8 op, u8 reg){
  if(op >= 36*OPL3_COUNT) return -1;
  if(reg >= 5) return -1;
  u8 chip = op / 36;
  u8 chipop = op % 36; //Op within the chip
  u8 chan = OPL3ChannelMap[chipop >> 1]; //Channel mapped to OPL3 way
  u8 addrhigh = chan >= 9; //Use second half of chip
  u8 index = ((chan % 9) << 1) + (chipop & 1); //Which operator, with channels mapped OPL3 way but ops not yet mapped
  u8 addr = OPL3OperRegBegin[reg] + OPL3RegOffset[index];
  u8 data = opl3_operators[op].ALL[reg];
  OPL3_SendAddrData(chip, addrhigh, addr, data);
  return 0;
}

s32 OPL3_RefreshChannel(u8 chan, u8 reg){
  if(chan >= 18*OPL3_COUNT) return -1;
  if(reg >= 3) return -1;
  u8 chip = chan / 18;
  u8 chipchan = chan % 18;
  u8 mappedchan = OPL3ChannelMap[chipchan];
  u8 addrhigh = mappedchan >= 9;
  u8 addr = OPL3ChanRegBegin[reg] + (mappedchan % 9);
  u8 data = opl3_channels[chan].ALL[reg];
  OPL3_SendAddrData(chip, addrhigh, addr, data);
  return 0;
}

s32 OPL3_RefreshChip(u8 chip, u8 reg){
  if(reg >= 4*OPL3_COUNT) return -1;
  u8 addrhigh = OPL3ChipRegHigh[reg];
  u8 addr = OPL3ChipReg[reg];
  u8 data = opl3_chip[chip].ALL[reg];
  OPL3_SendAddrData(chip, addrhigh, addr, data);
  return 0;
}

s32 OPL3_Reset(){
  //Reset
  OPL3_PIN_RS_0;
  //According to the datasheet, wait for 28 us
  MIOS32_DELAY_Wait_uS(100);
  //Unreset
  OPL3_PIN_RS_1;
  MIOS32_DELAY_Wait_uS(100);
  //Refresh all registers
  OPL3_RefreshAll();
  return 0;
}

s32 OPL3_Init(){
  u8 i;
  //========Set up GPIO (General Purpose Input/Output)
  //Data lines
  for(i=0; i<8; i++){
    MIOS32_BOARD_J10_PinInit(i, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
  }
  //A0,A1
  MIOS32_BOARD_J10_PinInit(14, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
  MIOS32_BOARD_J10_PinInit(15, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
  //RS
  //Copied from MIOS32_BOARD_PinInitHlp(OPL3_RS_PORT, OPL3_RS_PIN, MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
  {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = OPL3_RS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(OPL3_RS_PORT, &GPIO_InitStructure);
  }
  OPL3_PIN_RS_1;
  //CSes
  for(i=0; i<OPL3_COUNT; i++){
    MIOS32_BOARD_J10_PinInit(OPL3CSPins[i], MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
    MIOS32_BOARD_J10_PinSet(OPL3CSPins[i], 1);
  }
  //Reset
  OPL3_Reset();
  return 0;
}

s32 OPL3_RefreshAll(){
  MIOS32_MIDI_SendDebugMessage("OPL3_RefreshAll");
  u8 i, j;
  for(i=0; i<OPL3_COUNT; i++){
    for(j=0; j<4; j++){
      OPL3_RefreshChip(i, j);
    }
  }
  MIOS32_MIDI_SendDebugMessage("OPL3_RefreshAll: Refresh operators");
  for(i=0; i<36*OPL3_COUNT; i++){
    for(j=0; j<5; j++){
      OPL3_RefreshOperator(i, j);
    }
  }
  MIOS32_MIDI_SendDebugMessage("OPL3_RefreshAll: Refresh channels");
  for(i=0; i<18*OPL3_COUNT; i++){
    for(j=0; j<3; j++){
      OPL3_RefreshChannel(i, j);
    }
  }
  MIOS32_MIDI_SendDebugMessage("OPL3_RefreshAll: Demo patch");
  OPL3_SendDemoPatch();
  return 0;
}

void OPL3_SendDemoPatch(){
  u8 chip, high, i, j, k;
  for(chip=0; chip<OPL3_COUNT; chip++){
    OPL3_SendAddrData(chip, 0, 0x01, 0x20);
    OPL3_SendAddrData(chip, 1, 0x05, 0x01);
    OPL3_SendAddrData(chip, 0, 0xBD, 0xC0);
    for(high=0; high<=1; high++){
      for(j=0; j<3; j++){
        for(i=0; i<3; i++){
          k = i+(8*j);
          //Operator 1
          OPL3_SendAddrData(chip, high, 0x20 + k, 0x62);
          OPL3_SendAddrData(chip, high, 0x40 + k, 0x00);
          OPL3_SendAddrData(chip, high, 0x60 + k, 0xF7);
          OPL3_SendAddrData(chip, high, 0x80 + k, 0x0B);
          OPL3_SendAddrData(chip, high, 0xE0 + k, 0x00);
          //Operator 2
          OPL3_SendAddrData(chip, high, 0x23 + k, 0xA1);
          OPL3_SendAddrData(chip, high, 0x43 + k, 0x00);
          OPL3_SendAddrData(chip, high, 0x63 + k, 0xF3);
          OPL3_SendAddrData(chip, high, 0x83 + k, 0x0B);
          OPL3_SendAddrData(chip, high, 0xE3 + k, 0x00);
          //Channel 1
          OPL3_SendAddrData(chip, high, 0xC0 + i + (3*j), 0xF1);
        }
      }
    }
  }
  
}

u8 toggle;
s32 OPL3_OnFrame(){
  u8 op, chan, chip, reg;
  u16 val, q=0;
  while(q < op_queue_idx){
    if(q >= 36*5*OPL3_COUNT){
      DEBUG_MSG("PANIC!! [opl3.c] Op queue overflow q==%d!", q);
    }
    val = opl3_op_queue[q];
    op = val / 5;
    reg = val % 5;
    OPL3_RefreshOperator(op, reg);
    q++;
  }
  op_queue_idx = 0;
  q = 0;
  if(chan_queue_idx > 20){
    DEBUG_MSG("chan_queue_idx=%d", chan_queue_idx);
  }
  while(q < chan_queue_idx){
    if(q >= 18*3*OPL3_COUNT){
      DEBUG_MSG("PANIC!! [opl3.c] Chan queue overflow q==%d!", q);
    }
    val = opl3_chan_queue[q];
    chan = val / 3;
    reg = val % 3;
    OPL3_RefreshChannel(chan, reg);
    q++;
  }
  chan_queue_idx = 0;
  q = 0;
  while(q < chip_queue_idx){
    if(q >= 4*OPL3_COUNT){
      DEBUG_MSG("PANIC!! [opl3.c] Chip queue overflow q==%d!", q);
    }
    val = opl3_chip_queue[q];
    chip = val / 4;
    reg = val % 4;
    OPL3_RefreshChip(chip, reg);
    q++;
  }
  chip_queue_idx = 0;
  return 0;
}


s32 OPL3_AddOperQueue(u8 op, u8 reg){
  if(op >= 36*OPL3_COUNT){
    DEBUG_MSG("PANIC!! [opl3.c] Invalid op %d passed to OPL3_AddOperQueue! op_queue_idx=%d", op, op_queue_idx);
    return -9001;
  }
  if(reg >= 5){
    DEBUG_MSG("PANIC!! [opl3.c] Invalid reg %d passed to OPL3_AddOperQueue! op_queue_idx=%d", reg, op_queue_idx);
    return -9001;
  }
  u16 val = (0x0005*((u16)op))+reg;
  //See if it's on the queue
  u16 q;
  for(q=0; q<op_queue_idx; q++){
    if(opl3_op_queue[q] == val){
      return 0;
    }
  }
  opl3_op_queue[op_queue_idx] = val;
  op_queue_idx++;
  if(op_queue_idx >= 36*5*OPL3_COUNT){
    DEBUG_MSG("PANIC!! [opl3.c] Op queue overflow on write op_queue_idx==%d!", op_queue_idx);
    op_queue_idx = (36*5*OPL3_COUNT)-1;
  }
  return 0;
}

s32 OPL3_AddChanQueue(u8 chan, u8 reg){
  if(chan >= 18*OPL3_COUNT){
    DEBUG_MSG("PANIC!! [opl3.c] Invalid chan %d passed to OPL3_AddChanQueue! chan_queue_idx=%d", chan, chan_queue_idx);
    return -9001;
  }
  if(reg >= 3){
    DEBUG_MSG("PANIC!! [opl3.c] Invalid reg %d passed to OPL3_AddChanQueue! chan_queue_idx=%d", reg, chan_queue_idx);
    return -9001;
  }
  u8 val = (3*chan)+reg;
  //See if it's on the queue
  u8 q;
  for(q=0; q<chan_queue_idx; q++){
    if(opl3_chan_queue[q] == val){
      return 0;
    }
  }
  opl3_chan_queue[chan_queue_idx] = val;
  chan_queue_idx++;
  if(chan_queue_idx >= 18*3*OPL3_COUNT){
    DEBUG_MSG("PANIC!! [opl3.c] Chan queue overflow on write chan_queue_idx==%d!", chan_queue_idx);
    chan_queue_idx = (18*3*OPL3_COUNT)-1;
  }
  return 0;
}

s32 OPL3_AddChipQueue(u8 chip, u8 reg){
  if(chip >= OPL3_COUNT){
    DEBUG_MSG("PANIC!! [opl3.c] Invalid chip %d passed to OPL3_AddChipQueue! chip_queue_idx=%d", chip, chip_queue_idx);
    return -9001;
  }
  if(reg >= 4){
    DEBUG_MSG("PANIC!! [opl3.c] Invalid reg %d passed to OPL3_AddChipQueue! chip_queue_idx=%d", reg, chip_queue_idx);
    return -9001;
  }
  u8 val = (4*chip)+reg;
  //See if it's on the queue
  u8 q;
  for(q=0; q<chip_queue_idx; q++){
    if(opl3_chip_queue[q] == val){
      return 0;
    }
  }
  opl3_chip_queue[chip_queue_idx] = val;
  chip_queue_idx++;
  if(chip_queue_idx >= 4*OPL3_COUNT){
    DEBUG_MSG("PANIC!! [opl3.c] Chip queue overflow on write chip_queue_idx==%d!", chip_queue_idx);
    chip_queue_idx = 4*OPL3_COUNT;
  }
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// OPL3-setting functions
/////////////////////////////////////////////////////////////////////////////

//For an operator
s32 OPL3_SetFMult(u8 op, u8 value){
  u8 tmpval;
  if(op >= 36*OPL3_COUNT) return -1;
  tmpval = value & 15;
  if(opl3_operators[op].fmult != tmpval){
    opl3_operators[op].fmult = tmpval;
    OPL3_AddOperQueue(op, 0);
  }
  return 0;
}
s32 OPL3_SetWaveform(u8 op, u8 value){
  u8 tmpval;
  if(op >= 36*OPL3_COUNT) return -1;
  tmpval = value & 7;
  if(opl3_operators[op].waveform != tmpval){
    opl3_operators[op].waveform = tmpval;
    OPL3_AddOperQueue(op, 4);
  }
  return 0;
}
s32 OPL3_SetVibrato(u8 op, u8 value){
  u8 tmpval;
  if(op >= 36*OPL3_COUNT) return -1;
  tmpval = (value > 0) & 1;
  if(opl3_operators[op].vibrato != tmpval){
    opl3_operators[op].vibrato = tmpval;
    OPL3_AddOperQueue(op, 0);
  }
  return 0;
}

s32 OPL3_SetVolume(u8 op, u8 value){
  u8 tmpval;
  if(op >= 36*OPL3_COUNT) return -1;
  tmpval = 63 - (value & 63);
  if(opl3_operators[op].volume != tmpval){
    opl3_operators[op].volume = tmpval;
    OPL3_AddOperQueue(op, 1);
  }
  return 0;
}
s32 OPL3_SetTremelo(u8 op, u8 value){
  u8 tmpval;
  if(op >= 36*OPL3_COUNT) return -1;
  tmpval = (value > 0) & 1;
  if(opl3_operators[op].tremelo != tmpval){
    opl3_operators[op].tremelo = tmpval;
    OPL3_AddOperQueue(op, 0);
  }
  return 0;
}
s32 OPL3_SetKSL(u8 op, u8 value){
  u8 tmpval;
  if(op >= 36*OPL3_COUNT) return -1;
  tmpval = ((value & 1) << 1) | ((value & 2) >> 1);
  if(opl3_operators[op].ksl != tmpval){
    opl3_operators[op].ksl = tmpval;
    OPL3_AddOperQueue(op, 1);
  }
  return 0;
}

s32 OPL3_SetAttack(u8 op, u8 value){
  u8 tmpval;
  if(op >= 36*OPL3_COUNT) return -1;
  tmpval = 15 - (value & 15);
  if(opl3_operators[op].attack != tmpval){
    opl3_operators[op].attack = tmpval;
    OPL3_AddOperQueue(op, 2);
  }
  return 0;
}
s32 OPL3_SetDecay(u8 op, u8 value){
  u8 tmpval;
  if(op >= 36*OPL3_COUNT) return -1;
  tmpval = 15 - (value & 15);
  if(opl3_operators[op].decay != tmpval){
    opl3_operators[op].decay = tmpval;
    OPL3_AddOperQueue(op, 2);
  }
  return 0;
}
s32 OPL3_DoSustain(u8 op, u8 value){
  u8 tmpval;
  if(op >= 36*OPL3_COUNT) return -1;
  tmpval = (value > 0) & 1;
  if(opl3_operators[op].dosustain != tmpval){
    opl3_operators[op].dosustain = tmpval;
    OPL3_AddOperQueue(op, 0);
  }
  return 0;
}
s32 OPL3_SetSustain(u8 op, u8 value){
  u8 tmpval;
  if(op >= 36*OPL3_COUNT) return -1;
  tmpval = 15 - (value & 15);
  if(opl3_operators[op].sustain != tmpval){
    opl3_operators[op].sustain = tmpval;
    OPL3_AddOperQueue(op, 3);
  }
  return 0;
}
s32 OPL3_SetRelease(u8 op, u8 value){
  u8 tmpval;
  if(op >= 36*OPL3_COUNT) return -1;
  tmpval = 15 - (value & 15);
  if(opl3_operators[op].release != tmpval){
    opl3_operators[op].release = tmpval;
    OPL3_AddOperQueue(op, 3);
  }
  return 0;
}
s32 OPL3_SetKSR(u8 op, u8 value){
  u8 tmpval;
  if(op >= 36*OPL3_COUNT) return -1;
  tmpval = (value > 0) & 1;
  if(opl3_operators[op].ksr != tmpval){
    opl3_operators[op].ksr = tmpval;
    OPL3_AddOperQueue(op, 0);
  }
  return 0;
}

//For a channel
s32 OPL3_Gate(u8 chan, u8 value){
  u8 tmpval;
  if(chan >= 18*OPL3_COUNT) return -1;
  tmpval = (value > 0) & 1;
  if(opl3_channels[chan].keyon != tmpval){
    opl3_channels[chan].keyon = tmpval;
    OPL3_AddChanQueue(chan, 1);
  }
  return 0;
}
s32 OPL3_SetFrequency(u8 chan, u16 fnum, u8 block){
  if(chan >= 18*OPL3_COUNT) return -1;
  opl3_channels[chan].fnum_low = fnum & 0xFF;
  opl3_channels[chan].fnum_high = (fnum >> 8) & 3;
  opl3_channels[chan].block = block & 7;
  OPL3_AddChanQueue(chan, 0);
  OPL3_AddChanQueue(chan, 1);
  return 0;
}
s32 OPL3_SetFeedback(u8 chan, u8 value){
  u8 tmpval;
  if(chan >= 18*OPL3_COUNT) return -1;
  tmpval = value & 7;
  if(opl3_channels[chan].feedback != tmpval){
    opl3_channels[chan].feedback = tmpval;
    OPL3_AddChanQueue(chan, 2);
  }
  return 0;
}
s32 OPL3_SetAlgorithm(u8 chan, u8 value){
  u8 tmpval;
  if(chan >= 18*OPL3_COUNT) return -1;
  if(OPL3_IsChannel4Op(chan) == 1){
    tmpval = (value >> 1) & 1;
    if(opl3_channels[chan].synthtype != tmpval){
      opl3_channels[chan].synthtype = tmpval;
      OPL3_AddChanQueue(chan, 2);
    }
    tmpval = value & 1;
    if(opl3_channels[chan+1].synthtype != tmpval){
      opl3_channels[chan+1].synthtype = tmpval;
      OPL3_AddChanQueue(chan+1, 2);
    }
  }else{
    tmpval = (value > 0) & 1;
    if(opl3_channels[chan].synthtype != tmpval){
      opl3_channels[chan].synthtype = tmpval;
      OPL3_AddChanQueue(chan, 2);
    }
  }
  return 0;
}

s32 OPL3_OutLeft(u8 chan, u8 value){
  u8 tmpval;
  if(chan >= 18*OPL3_COUNT) return -1;
  tmpval = (value > 0) & 1;
  if(opl3_channels[chan].left != tmpval){
    opl3_channels[chan].left = tmpval;
    OPL3_AddChanQueue(chan, 2);
  }
  return 0;
}
s32 OPL3_OutRight(u8 chan, u8 value){
  u8 tmpval;
  if(chan >= 18*OPL3_COUNT) return -1;
  tmpval = (value > 0) & 1;
  if(opl3_channels[chan].right != tmpval){
    opl3_channels[chan].right = tmpval;
    OPL3_AddChanQueue(chan, 2);
  }
  return 0;
}
s32 OPL3_Out3(u8 chan, u8 value){
  u8 tmpval;
  if(chan >= 18*OPL3_COUNT) return -1;
  tmpval = (value > 0) & 1;
  if(opl3_channels[chan].out3 != tmpval){
    opl3_channels[chan].out3 = tmpval;
    OPL3_AddChanQueue(chan, 2);
  }
  return 0;
}
s32 OPL3_Out4(u8 chan, u8 value){
  u8 tmpval;
  if(chan >= 18*OPL3_COUNT) return -1;
  tmpval = (value > 0) & 1;
  if(opl3_channels[chan].out3 != tmpval){
    opl3_channels[chan].out3 = tmpval;
    OPL3_AddChanQueue(chan, 2);
  }
  return 0;
}
s32 OPL3_SetDest(u8 chan, u8 value){
  u8 tmpval;
  if(chan >= 18*OPL3_COUNT) return -1;
  tmpval = value & 15;
  if(opl3_channels[chan].dest != tmpval){
    opl3_channels[chan].dest = tmpval;
    OPL3_AddChanQueue(chan, 2);
  }
  return 0;
}

s32 OPL3_SetFourOp(u8 chan, u8 value){
  u8 chip = chan / 18;
  u8 chipchan = chan % 18;
  u8 tmpval = 1 << (chipchan / 2);
  if(chipchan > 10) return -1; //Must be 0-10
  if(chipchan & 1) return -1; //Must be even
  if(!(!(opl3_chip[chip].fourop & tmpval)) ^ !(!value)){ //Talk about a write-only language...
    if(value){
      opl3_chip[chip].fourop |= tmpval;
    }else{
      opl3_chip[chip].fourop &= ~tmpval;
    }
    OPL3_AddChipQueue(chip, 3);
  }
  return 0;
}

//For the whole OPL3
s32 OPL3_SetOpl3Mode(u8 chip, u8 value){
  u8 tmpval = (value > 0) & 1;
  if(opl3_chip[chip].opl3mode != tmpval){
    opl3_chip[chip].opl3mode = tmpval;
    OPL3_AddChipQueue(chip, 0);
  }
  return 0;
}
s32 OPL3_SetNoteSel(u8 chip, u8 value){
  u8 tmpval = (value > 0) & 1;
  if(opl3_chip[chip].notesel != tmpval){
    opl3_chip[chip].notesel = tmpval;
    OPL3_AddChipQueue(chip, 1);
  }
  return 0;
}
s32 OPL3_SetCSW(u8 chip, u8 value){
  u8 tmpval = (value > 0) & 1;
  if(opl3_chip[chip].csw != tmpval){
    opl3_chip[chip].csw = tmpval;
    OPL3_AddChipQueue(chip, 1);
  }
  return 0;
}

s32 OPL3_SetVibratoDepth(u8 chip, u8 value){
  u8 tmpval = (value > 0) & 1;
  if(opl3_chip[chip].vibratodepth != tmpval){
    opl3_chip[chip].vibratodepth = tmpval;
    OPL3_AddChipQueue(chip, 2);
  }
  return 0;
}
s32 OPL3_SetTremeloDepth(u8 chip, u8 value){
  u8 tmpval = (value > 0) & 1;
  if(opl3_chip[chip].tremelodepth != tmpval){
    opl3_chip[chip].tremelodepth = tmpval;
    OPL3_AddChipQueue(chip, 2);
  }
  return 0;
}

s32 OPL3_SetPercussionMode(u8 chip, u8 value){
  u8 tmpval = (value > 0) & 1;
  if(opl3_chip[chip].percussion != tmpval){
    opl3_chip[chip].percussion = tmpval;
    OPL3_AddChipQueue(chip, 2);
  }
  return 0;
}
s32 OPL3_TriggerBD(u8 chip, u8 value){
  u8 tmpval = (value > 0) & 1;
  if(opl3_chip[chip].bd != tmpval){
    opl3_chip[chip].bd = tmpval;
    OPL3_AddChipQueue(chip, 2);
  }
  return 0;
}
s32 OPL3_TriggerSD(u8 chip, u8 value){
  u8 tmpval = (value > 0) & 1;
  if(opl3_chip[chip].sd != tmpval){
    opl3_chip[chip].sd = tmpval;
    OPL3_AddChipQueue(chip, 2);
  }
  return 0;
}
s32 OPL3_TriggerTT(u8 chip, u8 value){
  u8 tmpval = (value > 0) & 1;
  if(opl3_chip[chip].tt != tmpval){
    opl3_chip[chip].tt = tmpval;
    OPL3_AddChipQueue(chip, 2);
  }
  return 0;
}
s32 OPL3_TriggerHH(u8 chip, u8 value){
  u8 tmpval = (value > 0) & 1;
  if(opl3_chip[chip].hh != tmpval){
    opl3_chip[chip].hh = tmpval;
    OPL3_AddChipQueue(chip, 2);
  }
  return 0;
}
s32 OPL3_TriggerCY(u8 chip, u8 value){
  u8 tmpval = (value > 0) & 1;
  if(opl3_chip[chip].cy != tmpval){
    opl3_chip[chip].cy = tmpval;
    OPL3_AddChipQueue(chip, 2);
  }
  return 0;
}

//Reading functions
u8 OPL3_IsChannel4Op(u8 chan){
  if(chan >= 18*OPL3_COUNT) return 0;
  u8 chip = chan / 18;
  u8 chipchan = chan % 18;
  if(chipchan > 11) return 0; //Must be 0-11
  if(chipchan & 1) return OPL3_IsChannel4Op(chan-1) ? 2 : 0; //If potentially second half, look at last one
  return (opl3_chip[chip].fourop & (1 << (chipchan >> 1))) > 0;
}
u8 OPL3_IsChannelPerc(u8 chan){
  if(chan >= 18*OPL3_COUNT) return 0;
  u8 chip = chan / 18;
  u8 chipchan = chan % 18;
  if(chipchan < 15) return 0;
  return opl3_chip[chip].percussion;
}
u8 OPL3_IsOperatorCarrier(u8 op){
  if(op >= 36*OPL3_COUNT) return 0;
  u8 chan = op>>1, subop = op & 3;
  //Percussion--all carriers
  if(OPL3_IsChannelPerc(chan) && chan%18 != 15) return 1;
  //Voices
  if(OPL3_IsChannel4Op(chan)){
    chan &= 0xFE; //If asked for an op in the second half of a 4-op channel, look at the first half
    //4th op is always carrier
    if(subop == 3) return 1;
    //Check complete algorithm
    switch(OPL3_GetAlgorithm(chan)){
    case 0:
      //Alg 1: 0->1->2->3
      return 0;
    case 1:
      //Alg 2: 0->1 + 2->3
      return subop == 1;
    case 2:
      //Alg 3: 0 + 1->2->3
      return subop == 0;
    case 3:
      //Alg 4: 0 + 1->2 + 3
      return subop != 1;
    default:
      //never be here
      return 0;
    }
  }else{
    return (op&1) || opl3_channels[chan].synthtype;
  }
}

u8 OPL3_GetAlgorithm(u8 chan){
  u8 fourop;
  if(chan >= 18*OPL3_COUNT) return 0;
  fourop = OPL3_IsChannel4Op(chan);
  if(fourop == 2){
    chan--;
    fourop--;
  }
  if(fourop){
    return (opl3_channels[chan].synthtype << 1) | opl3_channels[chan+1].synthtype;
  }else{
    return opl3_channels[chan].synthtype;
  }
  return 0;
}
