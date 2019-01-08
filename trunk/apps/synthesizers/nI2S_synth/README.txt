				  OOOOOOOOOO  OOOOOOOOOOOOOOO        OOOOOOOOOOOOOOO 
				  O........O O...............OO    OO...............O
				  O........O O......OOOOO......O  O......OOOOO......O
				  OOO....OOO OOOOOOO     O.....O  O.....O     OOOOOOO
					O....O               O.....O  O.....O            
 OOOOO OOOOOO       O....O               O.....O  O.....O            
 O....O......OO     O....O            OOOO....O    O.....OOO         
 O.............O    O....O       OOOOO......OO      OO......OOOOO    
 O.....OOOO.....O   O....O     OO........OOO          OOO........OO  
 O....O    O....O   O....O    O.....OOOOO                OOOOO.....O 
 O....O    O....O   O....O   O.....O                          O.....O
 O....O    O....O   O....O   O....O                           O.....O
 O....O    O....O OOO....OOO O....O        OOOOOO OOOOOOO     O.....O
 O....O    O....O O........O O....OOOOOOOOO.....O O......OOOOO......O
 O....O    O....O O........O O..................O O...............OO 
 OOOOOO    OOOOOO OOOOOOOOOO OOOOOOOOOOOOOOOOOOOO  OOOOOOOOOOOOOOO   

 n I L S '    D I G I T A L    I 2 S    T O Y    S Y N T H E S I Z E R

===============================================================================
Copyright (C) 2009 nILS Podewski (nils@podewski.de)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or STM32 Primer
   o a I2S compatible audio DAC (like TDA1543 or PCM1725U)

===============================================================================

I2S chip is connected to J8 of the core module
The system clock is available at J15B:E (not for STM32 Primer, as GLCD is 
connected to this IO)

Accordingly, STM32 Primer requires an I2S DAC without "master clock", like TDA1543
For MBHP_CORE_STM32, all "common" I2S DACs like PCM1725U can be used


Detailed pinning:
   - VSS -> Vss (J8:Vs)
   - VCC -> 5V (J8:Vd)
   - LRCIN (WS)   -> PB12 (J8:RC)
   - BCKIN (CK)   -> PB13 (J8:SC)
   - DIN   (SD)   -> PB15 (J8:SO)
   - SCKI  (MCLK) -> PC6 (J15B:E)

_______________________________________________________________________________

Sysex Implementation Chart
_______________________________________________________________________________

1) Direct write of parameters:
	F0 00 00 7E 4D 00 <address> <data> F7
	
	<address> 	16bit value split over 3x 7bit 
				[MSB=15 14] [13 12 11 10 9 8 7] [6 5 4 3 2 1 LSB=0]	
	<value> 	16bit value split over 3x 7bit 
				[MSB=15 14] [13 12 11 10 9 8 7] [6 5 4 3 2 1 LSB=0]
				
	For addresses see sysex.c, documentation will follow once most of the 
	feature and addresses are unlikely to change anymore.

_______________________________________________________________________________

CC Address Chart (incomplete)
_______________________________________________________________________________

Address	| Range		|
========+===========+==========================================================
--------+-----------+ ENVELOPE 1 ----------------------------------------------
0x0100  | 0..65535  | Attack
0x0101  | 0..65535  | Decay 
0x0102  | 0..65535  | Sustain
0x0103  | 0..65535  | Release
        |           |
--------+-----------+ ENVELOPE 2 ----------------------------------------------
0x0200  | 0..65535  | Attack
0x0201  | 0..65535  | Decay 
0x0202  | 0..65535  | Sustain
0x0203  | 0..65535  | Release
        |           |
--------+-----------+ OSCILLATOR 1 --------------------------------------------
0x0300  | 0..65535  | Bits set waveform enable (0=off, 1=on)
0x0301  | 0..65535  | Pulsewidth                             
0x0302  | 0..127    | Transpose (-64..+63)
0x0303  | 0..65535  | Volume
        |           |
--------+-----------+ OSCILLATOR 2 --------------------------------------------
0x0400  | 0..65535  | Bits set waveform enable (0=off, 1=on)
0x0401  | 0..65535  | Pulsewidth                             
0x0402  | 0..127    | Transpose (-64..+63)
0x0403  | 0..65535  | Volume
        |           |
        |           |
	
_______________________________________________________________________________

Modulation
_______________________________________________________________________________

<placeholder>
_______________________________________________________________________________

Trigger
_______________________________________________________________________________

<placeholder>
