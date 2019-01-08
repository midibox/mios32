/*
    microvga.c
    MIOS32/STM32 hardware low-level routines

    Copyright (c) 2008-9 SECONS s.r.o., http://www.MicroVGA.com
	MIOS32 modifications Copyright (c) 2010 Phil Taylor
	
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

   _putch, _getch, _kbhit must be defined for lib (conio.c and ui.c)
*/

#include <mios32.h>

#include "conio.h"

#ifndef MICROVGA_PORT
  #define MICROVGA_PORT 2
#endif


#if (MICROVGA_PORT == 0) 
  #define RCLK2_PORT	GPIOC
  #define RCLK2_PIN	GPIO_Pin_15
#endif

#if (MICROVGA_PORT == 2)
  #define RCLK2_PORT	GPIOC
  #define RCLK2_PIN	GPIO_Pin_14
#endif

#define KEYBUF_SIZE 10
unsigned char keybuf[KEYBUF_SIZE];
unsigned char khead, ktail;

char kbhit;

int _getch()
{
   int key;
   
   while (!_kbhit()) ;

   key = keybuf[khead];
   khead++;
   khead %= KEYBUF_SIZE;
   
   if (key == 0) {
       key = 0x100 | keybuf[khead];
       khead++;
       khead %= KEYBUF_SIZE;
   }
   return key;
}

int _kbhit()
{
  if (khead == ktail)
      _putch(0);
      
  if (khead == ktail)
     return 0;
     
  if (keybuf[khead] == 0 && ((khead+1)%KEYBUF_SIZE) == ktail) {
     _putch(0);
     if (keybuf[khead] == 0 && ((khead+1)%KEYBUF_SIZE) == ktail) 
       return 0;
  }
      
  return 1;
}


void _putch(char ch)
{
   unsigned char b;

   MIOS32_SPI_RC_PinSet(MICROVGA_PORT,0,0); // Set RCLK1 to 0 (enable)
   
   // wait for RDY signal (RCLK2) to go low!
#if (MICROVGA_PORT != 1)
   while ((RCLK2_PORT->IDR & RCLK2_PIN)); // Wait for ready input
#else
	// The maximum time to ready I have seen in my LA is about 130uS 
	MIOS32_DELAY_Wait_uS(150); // Simulate waiting for ready pin (untested)
#endif

   b = MIOS32_SPI_TransferByte(MICROVGA_PORT,ch);
   
   // for  s/w spi we must reverse the bit order, __RBIT uses a single thumb2 instruction for 
   //reversing 32 bit values. For other platforms use a (slow) multiply/shift operation.
#if (MICROVGA_PORT==2)
#ifdef MIOS32_FAMILY_STM32F10x 
   b = (__RBIT(b)>>24)&0xff;
#else
   b = b * 0x0802LU & 0x22110LU) | (b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16; // Reverse bits!
#endif
#endif
   if (b != 0xFF) {
      MIOS32_MIDI_SendDebugMessage("RX = 0x%02X\n", b);

      // Put received byte into keyboard buffer.
      keybuf[ktail] = b;
      ktail++;
      ktail %= KEYBUF_SIZE;
      kbhit = 1;
   } 
   
   MIOS32_SPI_RC_PinSet(MICROVGA_PORT,0,1); // Set RCLK1 to 1 (disable)
} 


void MicroVGA_Init()
{
    kbhit = 0;
    khead = 0;
    ktail = 0;
	
#ifdef MICROVGA_SPI_OUTPUTS_OD
    MIOS32_SPI_IO_Init(MICROVGA_PORT, MIOS32_SPI_PIN_DRIVER_STRONG_OD);
#else
    MIOS32_SPI_IO_Init(MICROVGA_PORT, MIOS32_SPI_PIN_DRIVER_STRONG);
#endif
	// Once SPI is initialized, we need to reconfig RCLK2 as input.
	// Suggest this is added as an option to SPI_IO_Init
#if (MICROVGA_PORT != 1)
	GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin  = RCLK2_PIN;
    GPIO_Init(RCLK2_PORT, &GPIO_InitStructure);
#endif
	// PRESCALER_8 is the fastest the microvga supports. 
	// Prescaler is ignored in s/w spi, it just sends as fast as it can!
	MIOS32_SPI_TransferModeInit(MICROVGA_PORT,MIOS32_SPI_MODE_CLK0_PHASE1,MIOS32_SPI_PRESCALER_8); 
	
}
