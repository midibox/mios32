$Id$

Benchmark for MIOS32_IIC routine
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or STM32 Primer
   o a IIC EEPROM 24LC256 or 24LC512

Optional hardware:
   o a LCD to display result
     (however, MIOS Terminal output is much more verbose!)

===============================================================================

This benchmark consists of three parts:

IIC Reads: transfers
   - one byte for IIC address
   - two bytes for EEPROM address
   - reads one data byte from EEPROM
Access is repeated 1000 times


IIC Block Reads: transfers
   - one byte for IIC address
   - two bytes for EEPROM address
   - reads 64 data bytes from EEPROM
Access is repeated 1000 times


IIC Writes: transfers
   - one byte for IIC address
   - two bytes for EEPROM address
(no data byte, EEPROM won't be overwritten!)
Access is repeated 1000 times


Results displayed in MIOS Terminal:

400 kHz selected in mios32_config.h
---------------------------------------------------------------
00000000155272 ms | ====================
00000000155274 ms | IIC Access Benchmark
00000000155277 ms | ====================
00000000155278 ms | 
00000000155279 ms | Settings:
00000000155282 ms | #define MIOS32_IIC_BUS_FREQUENCY 400000
00000000155283 ms | 
00000000155285 ms | Play any MIDI note to start the benchmark
00000000159036 ms | Reads:   132.2 mS / 1000
00000000159037 ms | -> 7564 read accesses per second
00000000159038 ms | -> transfered 30256 bytes per second
00000000159039 ms | Block Reads:  1549.6 mS / 1000
00000000159040 ms | -> 645 block read accesses per second
00000000159041 ms | -> transfered 43215 bytes per second
00000000159042 ms | Writes:   78.4 mS / 1000
00000000159043 ms | -> 12755 write accesses per second
00000000159044 ms | -> transfered 38265 bytes per second
---------------------------------------------------------------


1 MHz selected in mios32_config.h
---------------------------------------------------------------
00000000199520 ms | ====================
00000000199522 ms | IIC Access Benchmark
00000000199525 ms | ====================
00000000199526 ms | 
00000000199528 ms | Settings:
00000000199530 ms | #define MIOS32_IIC_BUS_FREQUENCY 1000000
00000000199533 ms | 
00000000199535 ms | Play any MIDI note to start the benchmark
00000000202283 ms | Reads:    67.3 mS / 1000
00000000202286 ms | -> 14858 read accesses per second
00000000202288 ms | -> transfered 59432 bytes per second
00000000202291 ms | Block Reads:   634.2 mS / 1000
00000000202293 ms | -> 1576 block read accesses per second
00000000202296 ms | -> transfered 105592 bytes per second
00000000202298 ms | Writes:   39.7 mS / 1000
00000000202301 ms | -> 25188 write accesses per second
00000000202303 ms | -> transfered 75564 bytes per second
---------------------------------------------------------------


===============================================================================
