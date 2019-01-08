// $Id: opl3.h $
/*
 * Header file for MBHP_OPL3 module driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2013-2015 Sauraen (sauraen@gmail.com)
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

#ifndef _OPL3_H
#define _OPL3_H

#ifdef __cplusplus
extern "C" {
#endif

/*
OPL3 Programming Information
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To simplify programming an interface for this driver, the operator-channel
mapping used externally here has been made much more obvious than the way
they are connected inside the actual OPL3. This driver does the conversion.

OPL3 has 36 operators, and up to 20 channels (in 2-op percussion mode). Six
pairs of channels may be merged together individually, to make up to 6 four-
voice channels. The operator and channel numbers are rearranged in this driver
so that the connected ones are always next to each other.

If you are using two or more OPL3s, add 18*n to the channel numbers and 36*n
to the operator numbers to access those spots for OPL3 index n. (0<=n<#OPL3s)

==================================+==================================+
       Operators you use          |     Actual operators in OPL3     |
Chan | 2-Op Mode |   4-Op Mode    |Chan | 2-Op Mode |   4-Op Mode    |
=====+===========+================+=====+===========+================+
  0  |  0,  1    |  0,  1,  2,  3 |  0  |  0,  3    |  0,  3,  6,  9 |
  1  |  2,  3    |        --      |  3  |  6,  9    |        --      |
  2  |  4,  5    |  4,  5,  6,  7 |  1  |  1,  4    |  1,  4,  7, 10 |
  3  |  6,  7    |        --      |  4  |  7, 10    |        --      |
  4  |  8,  9    |  8,  9, 10, 11 |  2  |  2,  5    |  2,  5,  8, 11 |
  5  | 10, 11    |        --      |  5  |  8, 11    |        --      |
  6  | 12, 13    | 12, 13, 14, 15 |  9  | 18, 21    | 18, 21, 24, 27 |
  7  | 14, 15    |        --      | 12  | 24, 27    |        --      |
  8  | 16, 17    | 16, 17, 18, 19 | 10  | 19, 22    | 19, 22, 25, 28 |
  9  | 18, 19    |        --      | 13  | 25, 28    |        --      |
 10  | 20, 21    | 20, 21, 22, 23 | 11  | 20, 23    | 20, 23, 26, 29 |
 11  | 22, 23    |        --      | 14  | 26, 29    |        --      |
-----+-----------+----------------+-----+-----------+----------------+
 12  | 24, 25    |        --      | 15  | 30, 33    |        --      |
 13  | 26, 27    |        --      | 16  | 31, 34    |        --      |
 14  | 28, 29    |        --      | 17  | 32, 35    |        --      |
-----+-----------+----------------+-----+-----------+----------------+
 15  | 30, 31    | 30+31: BD      |  6  | 12, 15    | 12+15: BD      |
 16  | 32, 33    | 32: HH  33: SD |  7  | 13, 16    | 13: HH  16: SD |
 17  | 34, 35    | 34: TT  35: CY |  8  | 14, 17    | 14: TT  17: CY |
=====+===========+================+=====+===========+================+

*/


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#if !defined(MIOS32_BOARD_STM32F4DISCOVERY) && !defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
#error "OPL3 module only supported for STM32F4 MCU!"
#endif

#ifndef OPL3_COUNT
#define OPL3_COUNT 1       //2
#endif

#ifndef OPL3_RS_PORT
#define OPL3_RS_PORT  GPIOB
#endif
#ifndef OPL3_RS_PIN
#define OPL3_RS_PIN   GPIO_Pin_8
#endif

//These are the J10 pin numbers
#ifndef OPL3_CS_PINS
#define OPL3_CS_PINS  {13} //{12,13}
#endif
//These are 1s shifted to the Port E pin numbers
#ifndef OPL3_CS_MASKS
#define OPL3_CS_MASKS {1<<5} //{1<<4,1<<5}
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
  u8 ALL[5];
  struct {
    //Byte 0: 0x20 + operator offset
    u8 fmult:4;     //Multiplier for channel frequency
    u8 ksr:1;       //How much to scale amp EG with pitch
    u8 dosustain:1; //Whether to sustain the note while gate is held
    u8 vibrato:1;   //Whether to use the frequency LFO
    u8 tremelo:1;   //Whether to use the amplitude LFO
    
    //Byte 1: 0x40 + operator offset
    u8 volume:6;    //Volume level of operator, or how much to have it affect the carrier (if it's a modulator)
    u8 ksl:2;       //How much to scale volume with pitch
    
    //Byte 2: 0x60 + operator offset
    u8 decay:4;     //Decay rate for amp EG
    u8 attack:4;    //Attack rate for amp EG
    
    //Byte 3: 0x80 + operator offset
    u8 release:4;   //Release rate for amp EG
    u8 sustain:4;   //Sustain level for amp EG; only held at this value after decay if dosustain == 1, otherwise, immediately goes to release
    
    //Byte 4: 0xF0 + operator offset
    u8 waveform:3;  //Waveform select
    u8 dummy:5;     //Bits ignored by OPL3
  };
} opl3_operator_t;

typedef union {
  u8 ALL[3];
  struct {
    //Byte 0: 0xA0 + channel offset
    u8 fnum_low;    //Lower 8 bits of frequency number
    
    //Byte 1: 0xB0 + operator offset
    u8 fnum_high:2; //Higher 2 bits of frequency number
    u8 block:3;     //Block number
    u8 keyon:1;     //Key on (gate)
    u8 dummy:2;     //Bits ignored by OPL3
    
    union{
      struct {
        //Byte 2: 0xC0 + operator offset
        u8 synthtype:1; //Algorithm selection
        u8 feedback:3;  //Feedback; only works in some operators (one per channel)
        u8 left:1;      //Whether to send this signal to output 1 (A) (left)
        u8 right:1;     //Whether to send this signal to output 2 (B) (right)
        u8 out3:1;      //Whether to send this signal to output 3 (C)
        u8 out4:1;      //Whether to send this signal to output 4 (D)
      };
      struct {
        u8 dummy2:4;
        u8 dest:4;
      };
    };
  };
} opl3_channel_t;

typedef union {
  u8 ALL[4];
  struct {
    //Byte 0: 0x05 only in higher register set
    u8 opl3mode:1;  //Whether to act like an OPL3 as opposed to OPL2
    u8 dummy:7;     //Bits ignored by OPL3
    
    //Byte 1: 0x08 only in lower register set
    u8 dummy2:6;    //Bits ignored by OPL3
    u8 notesel:1;   //Which bits of the frequency register to use to determine where to break up the keyboard for key scaling
    u8 csw:1;       //Composite sine wave mode; may not be supported by OPL3 (only OPL2), and nobody on the Internet knows how to use it even there
    
    //Byte 2: 0xBD only in lower register set
    u8 hh:1;        //If in percussion mode, play hi-hat  (operator 32 in this driver)
    u8 cy:1;        //If in percussion mode, play cymbal  (operator 35 in this driver)
    u8 tt:1;        //If in percussion mode, play tom-tom (operator 34 in this driver)
    u8 sd:1;        //If in percussion mode, play snare   (operator 33 in this driver)
    u8 bd:1;        //If in percussion mode, play bass drum (operators 30 and 31 in this driver)
    u8 percussion:1; //Percussion mode
    u8 vibratodepth:1; //How deep the vibrato (frequency LFO) should be
    u8 tremelodepth:1; //How deep the tremelo (amplitude LFO) should be
    
    //Byte 3: 0x04 only in higher register set
    u8 fourop; //Lower six bits are whether four-op mode is enabled for six channels which support them
  };
} opl3_chip_t;


/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern opl3_operator_t opl3_operators[36*OPL3_COUNT];
extern opl3_channel_t opl3_channels[18*OPL3_COUNT];
extern opl3_chip_t opl3_chip[OPL3_COUNT];


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

// Call this when setting up the microcontroller. Sets up GPIO pins, internals,
// calls OPL3_Reset().
extern s32 OPL3_Init(void);

// Sends reset signal to OPL3 and then refreshes it with current data.
// Why a synth would have to use this except in initialization I don't know.
// (It's automatically called by OPL3_Init().)
extern s32 OPL3_Reset(void);

// Refreshes all OPL3 registers. Call this after your app is done initializing.
extern s32 OPL3_RefreshAll(void);

// Sends a demo (organ) patch to the registers in all channels in all OPL3s.
// This bypasses the internal copies of values in the driver that the OPL3_SetX
// functions use, sending data directly to the sound chip. This has the
// disadvantage of making the internal values inconsistent with the current
// state of the chip, but the advantage of not relying on anything else to work
// right.
extern void OPL3_SendDemoPatch(void);

// Call this after every control refresh. Refreshes any OPL3 registers that have
// changed since last time.
extern s32 OPL3_OnFrame(void);

// Convenience functions for interacting with OPL3
// You MUST use these functions to write data to OPL3 or the OPL3 will not be
// refreshed with the data on the next frame!
// All these functions will check for identical data being given to them, and
// not update in that case, except SetFrequency().

//For an operator
extern s32 OPL3_SetFMult(u8 op, u8 value);
extern s32 OPL3_SetWaveform(u8 op, u8 value);
extern s32 OPL3_SetVibrato(u8 op, u8 value);

extern s32 OPL3_SetVolume(u8 op, u8 value); //This is also "corrected" so that 0 is lowest volume and 63 is highest
extern s32 OPL3_SetTremelo(u8 op, u8 value);
extern s32 OPL3_SetKSL(u8 op, u8 value);

//These functions "correct" the envelope values to the traditional ones:
//for A, D, and R, small values are faster than large ones,
//and for S, larger values are louder than small ones.
extern s32 OPL3_SetAttack(u8 op, u8 value);
extern s32 OPL3_SetDecay(u8 op, u8 value);
extern s32 OPL3_DoSustain(u8 op, u8 value);
extern s32 OPL3_SetSustain(u8 op, u8 value);
extern s32 OPL3_SetRelease(u8 op, u8 value);
extern s32 OPL3_SetKSR(u8 op, u8 value);

//For a channel
extern s32 OPL3_Gate(u8 chan, u8 value);
extern s32 OPL3_SetFrequency(u8 chan, u16 fnum, u8 block); //Does not check for identical data
extern s32 OPL3_SetFeedback(u8 chan, u8 value);
extern s32 OPL3_SetAlgorithm(u8 chan, u8 value); //If chan is the first of a 4op pair, value ranges 0:3 and sets algorithm of whole 4op voice

extern s32 OPL3_OutLeft(u8 chan, u8 value);
extern s32 OPL3_OutRight(u8 chan, u8 value);
extern s32 OPL3_Out3(u8 chan, u8 value);
extern s32 OPL3_Out4(u8 chan, u8 value);
extern s32 OPL3_SetDest(u8 chan, u8 value); //Set all four at once! MSB 000043RL LSB

extern s32 OPL3_SetFourOp(u8 chan, u8 value);

//For the whole OPL3
extern s32 OPL3_SetOpl3Mode(u8 chip, u8 value);
extern s32 OPL3_SetNoteSel(u8 chip, u8 value);
extern s32 OPL3_SetCSW(u8 chip, u8 value);

extern s32 OPL3_SetVibratoDepth(u8 chip, u8 value);
extern s32 OPL3_SetTremeloDepth(u8 chip, u8 value);

extern s32 OPL3_SetPercussionMode(u8 chip, u8 value);
extern s32 OPL3_TriggerBD(u8 chip, u8 value);
extern s32 OPL3_TriggerSD(u8 chip, u8 value);
extern s32 OPL3_TriggerTT(u8 chip, u8 value);
extern s32 OPL3_TriggerHH(u8 chip, u8 value);
extern s32 OPL3_TriggerCY(u8 chip, u8 value);

//Convenience functions for reading
extern u8 OPL3_IsChannel4Op(u8 chan); //Returns 0 for no, 1 for yes, and 2 for second-half-of-4op
extern u8 OPL3_IsChannelPerc(u8 chan); //If channel 15-17 and OPL3 is in percussion mode
extern u8 OPL3_IsOperatorCarrier(u8 op); //If 0, it's a modulator
extern u8 OPL3_GetAlgorithm(u8 chan); //For the second channel in a 4op, returns same as first chan

#ifdef __cplusplus
}
#endif

#endif /* _OPL3_H */
