/*
 * MBHP_Genesis module driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2016 Sauraen (sauraen@gmail.com)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include "genesis.h"

/////////////////////////////////////////////////////////////////////////////
// Help Macros
/////////////////////////////////////////////////////////////////////////////

//One OPN2 clock is, worst case (7 MHz), 24 CPU cycles
//So one OPN2 internal clock (divide by 6) is 144 CPU cycles
//Wait 160 cycles to give it a little extra time
#define GENESIS_OPN2_WRITEWAIT do { u8 temp = 0; while(++temp <= (GENESIS_OPN2_WRITETIMEOUT >> 2)); } while(false)
//Wait timeout, roughly 100 ns, for PSG/board bits reads/writes
#define GENESIS_PSG_WRITEWAIT do { u8 temp = 0; while(++temp <= (GENESIS_PSG_WRITETIMEOUT >> 2)); } while(false)

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

genesis_t genesis[GENESIS_COUNT];

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Lookup tables
/////////////////////////////////////////////////////////////////////////////

static const u8 OPN2GlobalRegs[16] = {
    //0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27
      0xFF, 0x00, 0x01, 0xFF, 0x02, 0x03, 0x04, 0x05,
    //0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F
      0x06, 0xFF, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF
};

static const u8 OPN2Ch3Regs[8] = {
    //0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF
        14,   10,   12, 0xFF,   15,   11,   13, 0xFF
};

/////////////////////////////////////////////////////////////////////////////
// Functions
/////////////////////////////////////////////////////////////////////////////

void Genesis_Init(){
    //========Set up GPIO (General Purpose Input/Output)
    /*
    MBHP_Genesis:J10    CORE_STM32F4    STM32F4
    -------------------------------------------
    D0                  J10A:D0         PE8
    D1                  J10A:D1         PE9
    D2                  J10A:D2         PE10
    D3                  J10A:D3         PE11
    D4                  J10A:D4         PE12
    D5                  J10A:D5         PE13
    D6                  J10A:D6         PE14
    D7                  J10A:D7         PE15
    /CS                 J10B:D8         PC13
    /RD                 J10B:D9         PC14
    /WR                 J10B:D10        PC15
    A0                  J10B:D11        PE2
    A1                  J10B:D12        PE4
    A2                  J10B:D13        PE5
    A3                  J10B:D14        PE6
    A4                  J10B:D15        PE7
    Registers MODER, OSPEEDR, and PUPDR have two bits per I/O pin
    Register OTYPER has one (lower 16 bits only)
    */
    //Port E
    GPIOE->MODER &= 0x000000CF;     //Set 15 downto 4 & 2 to inputs
    GPIOE->MODER |= 0x00005510;     //Set 7 downto 4 & 2 to outputs (don't care about initial state)
    GPIOE->OTYPER &= 0xFFFF000B;    //Set all to push-pull (D7:0 will be when outputs)
    GPIOE->OSPEEDR |= 0xFFFFFF30;   //50 MHz GPIO speed (max)
    GPIOE->PUPDR &= 0x000000CF;     //Turn off all pull-ups
    //Port C
    GPIOC->ODR |= 0x0000E000;       //Set /CS, /RD, /WR all to 1
    GPIOC->MODER &= 0x03FFFFFF;     //Set 15 downto 13 to inputs
    GPIOC->MODER |= 0x54000000;     //Set 15 downto 13 to outputs
    GPIOC->OTYPER &= 0xFFFF1FFF;    //Set all to push-pull
    GPIOC->OSPEEDR |= 0xFC000000;   //GOTTA GO FAST
    GPIOC->PUPDR &= 0x03FFFFFF;     //Turn off all pull-ups
    //Reset all (also resets internal chip state)
    for(i=0; i<GENESIS_COUNT; i++){
        Genesis_Reset(i);
    }
}

void Genesis_OPN2Write(u8 board, u8 addrhi, u8 address, u8 data){
    board &= 0x03;
    addrhi &= 0x01;
    //Save chip state
    u8 chan, op, reg;
    if(address <= 0x2F){
        if(address >= 0x20 && !addrhi){
            //OPN2 global register
            reg = OPN2GlobalRegs[address - 0x20];
            if(reg < 0xFF){
                genesis[board].opn2.ALL[reg] = data;
            }
        }
    }else if(address <= 0x9F){
        chan = (address & 0x03);
        if(chan != 0x03){ //No channel 4 in first half
            chan += addrhi + (addrhi << 1); //Add 3 for channels 4, 5, 6
            reg = ((address & 0xF0) >> 4) - 3;
            op = ((address & 0x80) >> 3) | ((address & 0x40) >> 1); //Ops 1,2,3,4: 0x30, 0x38, 0x34, 0x3C
            genesis[board].opn2.chan[chan].op[op].ALL[reg] = data;
        }
    }else if(address <= 0xAE && address >= 0xA8){
        //Channel 3 extra frequency
        reg = OPN2Ch3Regs[address - 0xA8];
        if(reg < 0xFF){
            genesis[board].opn2.ALL[reg] = data;
        }
    }else if(address <= 0xB6){
        chan = (address & 0x03);
        if(chan != 0x03){ //No channel 4 in first half
            chan += addrhi + (addrhi << 1); //Add 3 for channels 4, 5, 6
            reg = ((address & 0x10) >> 3) | ((address & 0x04) >> 2); //Regs 0,1,2,3: A0, A4, B0, B4
            genesis[board].opn2.chan[chan].ALL[reg] = data;
        }
    }//else { not a register; }
    //Perform chip write
    MIOS32_IRQ_Disable(); //Turn off interrupts
    GPIOE->MODER &= 0x0000FFFF; //Set data pins to inputs (in case not already)
    u32 porte = GPIOE->ODR;
    porte &= 0xFFFF000B; //Mask out the things we will set
    u32 a = address;
    a <<= 2; //Make room for board number
    a |= board;
    a <<= 2; //A2 = 0 for OPN2 write, A1 = addrhi
    a |= addrhi;
    a <<= 4; //Move over into place, A0 = 0 for address write
    porte |= a; //Write to our temp copy
    GPIOE->ODR = porte; //Write
    u32 portc = GPIOC->ODR;
    portc &= 0xFFFF5FFF; //Write /CS and /WR low
    GPIOC->ODR = portc; //Write
    GPIOE->MODER |= 0x55550000; //Set data pins to outputs
    GENESIS_OPN2_WRITEWAIT; //Wait for 1 OPN2 internal cycle
    GPIOE->MODER &= 0x0000FFFF; //Set data pins to inputs
    portc |= 0x0000A000; //Write /CS and /WR high
    GPIOC->ODR = portc; //Write
    porte &= 0xFFFF00FF;
    porte |= ((u32)data << 8);
    porte |= 4; //A0 = 1 for data write
    GPIOE->ODR = porte; //Write
    portc &= 0xFFFF5FFF; //Write /CS and /WR low
    GPIOC->ODR = portc; //Write
    GENESIS_OPN2_WRITEWAIT; //Wait for 1 OPN2 internal cycle
    portc |= 0x0000A000; //Write /CS and /WR high
    GPIOC->ODR = portc; //Write
    MIOS32_IRQ_Enable(); //Turn on interrupts
}

void Genesis_PSGWrite(u8 board, u8 data){
    board &= 0x03;
    //Save chip state
    u8 addr, voice;
    if(data & 0x80){
        addr = (data & 0x70) >> 4;
        genesis[board].psg.latchedaddr = addr;
        voice = (addr >> 1);
        if(addr & 1){
            //Attenuation
            if(voice == 3){
                genesis[board].psg.noise.atten = (data & 0x0F);
            }else{
                genesis[board].psg.square[voice].atten = (data & 0x0F);
            }
        }else{
            if(voice == 3){
                //Noise parameters
                genesis[board].psg.noise.ALL = (genesis[board].psg.noise.ALL & 0xF0) | (data & 0x0F);
            }else{
                genesis[board].psg.square[voice].freq = (genesis[board].psg.square[voice].freq & 0xFFF0) | (data & 0x0F);
            }
        }
    }else{
        //TODO does the PSG ignore second byte frequency writes if the first byte
        //was the square's attenuation, not its frequency? I.e. does it latch just
        //the voice number, or the voice and also whether we are writing frequency
        //or attenuation?
        voice = (genesis[board].psg.latchedaddr >> 1);
        if(voice != 3){
            genesis[board].psg.square[voice].freq = (genesis[board].psg.square[voice].freq & 0x000F) | ((u16)(data & 0x3F) << 4);
        }
    }
    //Perform chip write
    MIOS32_IRQ_Disable(); //Turn off interrupts
    GPIOE->MODER &= 0x0000FFFF; //Set data pins to inputs (in case not already)
    u32 porte = GPIOE->ODR;
    porte &= 0xFFFF000B; //Mask out the things we will set
    u32 a = data;
    a <<= 2; //Make room for board number
    a |= board;
    a <<= 6; //Move into place
    a |= 0x20; //A2 = 1 for PSG write, A1 = 0 for PSG not output bits, A0 = -
    porte |= a; //Write to our temp copy
    GPIOE->ODR = porte; //Write
    u32 portc = GPIOC->ODR;
    portc &= 0xFFFF5FFF; //Write /CS and /WR low
    GPIOC->ODR = portc; //Write
    GPIOE->MODER |= 0x55550000; //Set data pins to outputs
    GENESIS_PSG_WRITEWAIT; //Wait for the glue logic to catch up
    GPIOE->MODER &= 0x0000FFFF; //Set data pins to inputs
    portc |= 0x0000A000; //Write /CS and /WR high
    GPIOC->ODR = portc; //Write
    MIOS32_IRQ_Enable(); //Turn on interrupts
}

bool Genesis_CheckOPN2Busy(u8 board){
    board &= 0x03;
    if(genesis[board].opn2.test_readdat){
        //Switch back to read status mode
        genesis[board].opn2.test_readdat = 0;
        Genesis_OPN2Write(board, 0, 0x21, genesis[board].opn2.testreg21);
        //Don't wait for busy
    }
    MIOS32_IRQ_Disable(); //Turn off interrupts
    GPIOE->MODER &= 0x0000FFFF; //Set data pins to inputs (in case not already)
    u32 porte = GPIOE->ODR;
    porte &= 0xFFFF000B; //Mask out the things we will set
    u32 a = address;
    a <<= 2; //Make room for board number
    a |= board;
    a <<= 6; //Move into place; A2 = 0 for OPN2 read, A1 = -, A0 = -
    porte |= a; //Write to our temp copy
    GPIOE->ODR = porte; //Write
    u32 portc = GPIOC->ODR;
    portc &= 0xFFFF3FFF; //Write /CS and /RD low
    GPIOC->ODR = portc; //Write
    GENESIS_OPN2_WRITEWAIT; //Wait for 1 OPN2 internal cycle
    u8 res = ((GPIOE->IDR >> 8) && 0xFF); //Read OPN2 data
    portc |= 0x0000C000; //Write /CS and /WR high
    GPIOC->ODR = portc; //Write
    MIOS32_IRQ_Enable(); //Turn on interrupts
    return ((res & 0x80) > 0);
}

bool Genesis_CheckPSGBusy(u8 board){
    board &= 0x03;
    MIOS32_IRQ_Disable(); //Turn off interrupts
    GPIOE->MODER &= 0x0000FFFF; //Set data pins to inputs (in case not already)
    u32 porte = GPIOE->ODR;
    porte &= 0xFFFF000B; //Mask out the things we will set
    u32 a = address;
    a <<= 2; //Make room for board number
    a |= board;
    a <<= 6; //Move into place
    a |= 0x40; //A2 = 1 for board bits read, A1 = -, A0 = -
    porte |= a; //Write to our temp copy
    GPIOE->ODR = porte; //Write
    u32 portc = GPIOC->ODR;
    portc &= 0xFFFF3FFF; //Write /CS and /RD low
    GPIOC->ODR = portc; //Write
    GENESIS_PSG_WRITEWAIT; //Wait for the glue logic to catch up
    genesis[board].board.readbits = ((GPIOE->IDR >> 8) && 0xFF); //Read board bits data
    portc |= 0x0000C000; //Write /CS and /WR high
    GPIOC->ODR = portc; //Write
    MIOS32_IRQ_Enable(); //Turn on interrupts
    return genesis[board].board.psg_ready;
}

void Genesis_WriteBoardBits(u8 board){
    board &= 0x03;
    MIOS32_IRQ_Disable(); //Turn off interrupts
    GPIOE->MODER &= 0x0000FFFF; //Set data pins to inputs (in case not already)
    u32 porte = GPIOE->ODR;
    porte &= 0xFFFF000B; //Mask out the things we will set
    u32 a = genesis[board].board.writebits;
    a <<= 2; //Make room for board number
    a |= board;
    a <<= 6; //Move into place
    a |= 0x30; //A2 = 1 for PSG write, A1 = 1 for board bits, A0 = -
    porte |= a; //Write to our temp copy
    GPIOE->ODR = porte; //Write
    u32 portc = GPIOC->ODR;
    portc &= 0xFFFF5FFF; //Write /CS and /WR low
    GPIOC->ODR = portc; //Write
    GPIOE->MODER |= 0x55550000; //Set data pins to outputs
    GENESIS_PSG_WRITEWAIT; //Wait for the glue logic to catch up
    GPIOE->MODER &= 0x0000FFFF; //Set data pins to inputs
    portc |= 0x0000A000; //Write /CS and /WR high
    GPIOC->ODR = portc; //Write
    MIOS32_IRQ_Enable(); //Turn on interrupts
}

void Genesis_Reset(u8 board){
    board &= 0x03;
    //Clear internal state
    for(j=0; j<154; j++){
        genesis[board].ALL[j] = 0;
    }
    for(j=0; j<6; j++){
        genesis[board].opn2.chan[j].lfooutreg = 0xE0; //Output bits initialized to 1
    }
    genesis[board].board.test_dir = 1;
    //Do chip reset
    genesis[board].board.reset = 0;
    Genesis_WriteBoardBits(board);
    MIOS32_DELAY_Wait_uS(GENESIS_RESETTIMEOUTUS);
    genesis[board].board.reset = 1;
    Genesis_WriteBoardBits(board);
    MIOS32_DELAY_Wait_uS(GENESIS_RESETTIMEOUTUS);
}





