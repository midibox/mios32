/*
 * Header file for MBHP_Genesis module driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 * 
 * See Readme.txt for information on how to use this driver.
 * 
 */

#ifndef _GENESIS_H
#define _GENESIS_H

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

#if !defined(MIOS32_BOARD_STM32F4DISCOVERY) && !defined(MIOS32_BOARD_MBHP_CORE_STM32F4)
#error "MBHP_Genesis module only supported for STM32F4 MCU!"
#endif

#ifndef GENESIS_COUNT
#define GENESIS_COUNT 1       //4
#endif

/*
OPN2 Timeout Test Results
235: still unstable
256: appears to get every message
*/
#ifndef GENESIS_OPN2_WRITETIMEOUT
#define GENESIS_OPN2_WRITETIMEOUT 256 //256 
#endif

#ifndef GENESIS_PSG_WRITETIMEOUT
#define GENESIS_PSG_WRITETIMEOUT 28 //12
#endif

#ifndef GENESIS_SHORTWAITTIME
#define GENESIS_SHORTWAITTIME 28 //16
#endif

#ifndef GENESIS_RESETTIMEOUTUS
#define GENESIS_RESETTIMEOUTUS 1000
#endif


/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////

typedef union {
    u8 ALL[7];
    struct {
        //Byte 0: 0x30 + operator offset
        union {
            u8 freqreg;
            struct {
                u8 fmult:4;     //Multiplier for channel frequency
                u8 detune:3;    //Operator frequency detune
                u8 dummy3:1;    //Bits ignored by OPN2
            };
        };

        //Byte 1: 0x40 + operator offset
        union {
            u8 tlreg;
            struct {
                u8 totallevel:7;//Total level of operator
                u8 dummy4:1;    //Bits ignored by OPN2
            };
        };

        //Byte 2: 0x50 + operator offset
        union {
            u8 atkrsreg;
            struct {
                u8 attackrate:5;//Attack rate for amp EG
                u8 dummy5:1;    //Bits ignored by OPN2
                u8 ratescale:2; //Depth of rate scaling by key for amp EG
            };
        };

        //Byte 3: 0x60 + operator offset
        union {
            u8 dec1amreg;
            struct {
                u8 decay1rate:5;//First decay rate for amp EG
                u8 dummy6:2;    //Bits ignored by OPN2
                u8 amplfo:1;    //Flag for LFO modulating amp of this operator
            };
        };

        //Byte 4: 0x70 + operator offset
        union {
            u8 dec2reg;
            struct {
                u8 decay2rate:5;//Second decay rate for amp EG
                u8 dummy7:3;    //Bits ignored by OPN2
            };
        };
        
        //Byte 5: 0x80 + operator offset
        union {
            u8 rellvlreg;
            struct {
                u8 releaserate:4;//Release rate for amp EG
                u8 decaylevel:4;//First decay level for amp EG
            };
        };
        
        //Byte 6: 0x90 + operator offset
        union {
            u8 ssgreg;
            struct {
                u8 ssg_hold:1;  //SSG-EG mode, hold level after first cycle
                u8 ssg_toggle:1;//SSG-EG mode, toggle level after each cycle
                u8 ssg_init:1;  //SSG-EG mode, invert first cycle
                u8 ssg_enable:1;//Enable SSG-EG mode
                u8 dummy9:4;    //Bits ignored by OPN2
            };
        };
    };
} opn2_operator_t;

typedef union {
    u8 ALL[32];
    struct {
        //Byte 0: 0xA0 + channel offset
        u8 fnum_low;    //Lower 8 bits of frequency number

        //Byte 1: 0xA4 + operator offset
        union {
            u8 fhireg;
            struct {
                u8 fnum_high:3; //Higher 3 bits of frequency number
                u8 block:3;     //Block number
                u8 dummya:2;    //Bits ignored by OPN2
            };
        };

        //Byte 2: 0xB0 + operator offset
        union {
            u8 algfbreg;
            struct {
                u8 algorithm:3; //FM algorithm, connection of operators
                u8 feedback:3;  //Feedback level for operator 1
                u8 dummyb:2;    //Bits ignored by OPN2
            };
        };
        
        //Byte 3: 0xB4 + operator offset
        union {
            u8 lfooutreg;
            struct {
                u8 lfofreqd:3;  //Depth of LFO modulating channel frequency
                u8 dummyb4:1;   //Bits ignored by OPN2
                u8 lfoampd:2;   //Depth of LFO modulating operator amplitudes
                u8 out_r:1;     //Enable right channel output
                u8 out_l:1;     //Enable left channel output
            };
        };
        
        //Four operators
        opn2_operator_t op[4];
    };
} opn2_channel_t;

typedef union {
    u8 ALL[144];
    struct {
        //Byte 0: 0x21 only in lower register set
        union {
            u8 testreg21;
            struct {
                u8 test_selrd14:1;  //Test register 0x21 bit 0: select which of two signals is read as bit 14 of the test read output
                u8 test_lfo_unk:1;  //Test register 0x21 bit 1: some LFO control, unknown function
                u8 test_tmrspd:1;   //Test register 0x21 bit 2: run timers 24 times faster
                u8 test_nopg:1;     //Test register 0x21 bit 3: freezes PG (disables phase register writebacks)
                u8 test_ugly:1;     //Test register 0x21 bit 4: inverts MSB of operators, produces fun ugly (but pitched) sounds
                u8 test_noeg:1;     //Test register 0x21 bit 5: freeses EG (disables EG phase counter writebacks)
                u8 test_readdat:1;  //Test register 0x21 bit 6: enable reading test data (channel/operator) instead of status flags
                u8 test_readmsb:1;  //Test register 0x21 bit 7: select LSB or MSB of read test data
            };
        };
        
        //Byte 1: 0x22 only in lower register set
        union {
            u8 lforeg;
            struct {
                u8 lfo_freq:3;      //LFO global frequency
                u8 lfo_enabled:1;   //LFO enable (start/stop)
                u8 dummy2:4;        //Bits ignored by OPN2
            };
        };
        
        //Byte 2: 0x24 only in lower register set
        u8 timera_high;     //MSBs of Timer A frequency
        
        //Byte 3: 0x25 only in lower register set
        union {
            u8 timeralowreg;
            struct {
                u8 timera_low:2;    //LSBs of Timer A frequency
                u8 dummy5:6;        //Bits ignored by OPN2
            };
        };
        
        //Byte 4: 0x26 only in lower register set
        u8 timerb;          //Timer B frequency
        
        //Byte 5: 0x27 only in lower register set
        union {
            u8 timerctrlreg;
            struct {
                u8 timera_load:1;   //Timer A load
                u8 timerb_load:1;   //Timer B load
                u8 timera_enable:1; //Timer A enable
                u8 timerb_enable:1; //Timer B enable
                u8 timera_reset:1;  //Timer A reset
                u8 timerb_reset:1;  //Timer B reset
                u8 ch3_mode:2;      //Channel 3 mode: 00 normal, 01 or 11 4-frequency, 10 CSM mode
            };
        };
        
        //Byte 6: 0x28 only in lower register set
        union {
            u8 gatereg;
            struct {
                u8 gate_channel:3;  //Select channel to gate
                u8 dummy8:1;        //Bits ignored by OPN2
                u8 gate_op1:1;      //Gate operator 1
                u8 gate_op2:1;      //Gate operator 2
                u8 gate_op3:1;      //Gate operator 3
                u8 gate_op4:1;      //Gate operator 4
            };
        };
        
        //Byte 7: 0x2A only in lower register set
        u8 dac_high;        //Bits 8 downto 1 of Channel 6 DAC value
        
        //Byte 8: 0x2B only in lower register set
        union {
            u8 dacenablereg;
            struct {
                u8 dummyb:7;        //Bits ignored by OPN2
                u8 dac_enable:1;    //Replace Channel 6 FM output by DAC output
            };
        };
        
        //Byte 9: 0x2C only in lower register set
        union {
            u8 testreg2C;
            struct {
                u8 dummyc:3;        //Test register 0x2C bits 2 downto 0: ignored by OPN2, confirmed from die shot
                u8 dac_low:1;       //Test register 0x2C bit 3: Bit 0 of Channel 6 DAC value
                u8 test_readop:1;   //Test register 0x2C bit 4: Read 14-bit operator output instead of 9-bit channel output
                u8 dac_override:1;  //Test register 0x2C bit 5: Play DAC output over all channels (maybe except Channel 5?)
                u8 test_pinctrl:1;  //Test register 0x2C bit 6: select function of TEST pin input, unknown function
                u8 test_pindir:1;   //Test register 0x2C bit 7: Set the TEST pin to be an input rather than an output
            };
        };
        
        //Byte 10: 0xA9 only in lower register set
        u8 ch3op1_fnum_low;     //Lower 8 bits of frequency number

        //Byte 11: 0xAD only in lower register set
        union {
            u8 ch3op1_fhireg;
            struct {
                u8 ch3op1_fnum_high:3;  //Higher 3 bits of frequency number
                u8 ch3op1_block:3;      //Block number
                u8 ch3op1_dummy:2;      //Bits ignored by OPN2
            };
        };
        
        //Byte 12: 0xAA only in lower register set
        u8 ch3op2_fnum_low;     //Lower 8 bits of frequency number

        //Byte 13: 0xAE only in lower register set
        union {
            u8 ch3op2_fhireg;
            struct {
                u8 ch3op2_fnum_high:3;  //Higher 3 bits of frequency number
                u8 ch3op2_block:3;      //Block number
                u8 ch3op2_dummy:2;      //Bits ignored by OPN2
            };
        };
        
        //Byte 14: 0xA8 only in lower register set
        u8 ch3op3_fnum_low;     //Lower 8 bits of frequency number

        //Byte 15: 0xAC only in lower register set
        union {
            u8 ch3op3_fhireg;
            struct {
                u8 ch3op3_fnum_high:3;  //Higher 3 bits of frequency number
                u8 ch3op3_block:3;      //Block number
                u8 ch3op3_dummy:2;      //Bits ignored by OPN2
            };
        };
        
        //Channels
        opn2_channel_t chan[6];
    };
} opn2_chip_t;

typedef union {
    u16 ALL;
    struct {
        u16 freq:10;    //Frequency
        u8 dummy:2;     //Spacing only, not in PSG
        u8 atten:4;     //Attenuation, with 0xF = off
    };
} psg_square_t;

typedef union {
    u8 ALL;
    struct {
        u8 rate:2;      //Noise rate: 00 high, 01 mid, 10 low, 11 channel 3 clock
        u8 type:1;      //Noise mode: 0 "periodic" (1/16 pulse), 1 "white"
        u8 dummy1:1;    //Bits ignored by PSG
        u8 atten:4;     //Attenuation, with 0xF = off
    };
} psg_noise_t;

typedef union {
    u8 ALL[8];
    struct {
        psg_square_t square[3];
        psg_noise_t noise;
        u8 latchedaddr; //Last address written to, for frequency MSB commands
    };
} psg_chip_t;

typedef union {
    u8 ALL[2];
    struct {
        union {
            u8 writebits;
            struct {
                u8 cap_psg:1;   //PSG filter capacitor enabled
                u8 dummy1:2;
                u8 cap_opn2:1;  //OPN2 filter capacitor enabled
                u8 dummy2:1;
                u8 test_dir:1;  //OPN2 Test pin data direction: 1=output (from OPN2), 0=input (to OPN2)
                u8 test_out:1;  //OPN2 Test pin data to send to OPN2, when OPN2 pin is in input mode
                u8 reset:1;     //OPN2 and PSG reset signal, active-low
            };
        };
        union {
            u8 readbits;
            struct {
                u8 dummy3:4;
                u8 opn2_irq:1;  //OPN2 /IRQ pin state (active low)
                u8 dummy4:1;
                u8 test_pin:1;  //OPN2 TEST pin state
                u8 psg_ready:1; //PSG READY pin state
            };
        };
    };
} genesis_board_t;

typedef union {
    u8 ALL[154];
    struct {
        opn2_chip_t opn2;
        psg_chip_t psg;
        genesis_board_t board;
    };
} genesis_t;

//Sample use cases of these data structures:
//u8 is_ssg_toggle = genesis[3].opn2.chan[4].op[0].ssg_toggle;
//u16 psg_freq = genesis[2].psg.square[1].freq;
//u8 got_timer_irq = genesis[0].board.opn2_irq;

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

extern genesis_t genesis[GENESIS_COUNT];

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

// Call this when setting up the microcontroller. Sets up GPIO pins, internals,
// resets all boards.
extern void Genesis_Init(void);

// Write a value to an OPN2.
extern void Genesis_OPN2Write(u8 board, u8 addrhi, u8 address, u8 data);

// Write a value to a PSG.
extern void Genesis_PSGWrite(u8 board, u8 data);

// Check whether an OPN2 is busy. This is relatively costly in time (a couple
// microseconds).
extern u8 Genesis_CheckOPN2Busy(u8 board);

// Check whether a PSG is busy. Also loads the current status bits into
// genesis[board].board.readbits .
extern u8 Genesis_CheckPSGBusy(u8 board);

// Reset (and un-reset) a board.
extern void Genesis_Reset(u8 board);

// Write the contents of genesis[board].board.writebits to the board. Call
// after e.g. changing filter capacitor settings.
extern void Genesis_WriteBoardBits(u8 board);



#ifdef __cplusplus
}
#endif

#endif /* _GENESIS_H */
