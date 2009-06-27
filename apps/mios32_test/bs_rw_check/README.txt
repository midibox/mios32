Bankstick r/w check
IMPORTANT NOTE: RUNING THIS APPLICATION WILL DESTROY ALL DATA ON YOUR BANKSTICKS!!!

===============================================================================
Copyright (C) 2008 Matthias Mächler (thismaechler@gmx.ch / maechler@mm-computing.ch)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or STM32 Primer
   o 1 - 8 Banksticks

Optional hardware:
   o LC Display
   o DIN buttons

===============================================================================

IMPORTANT NOTE: RUNING THIS APPLICATION WILL DESTROY ALL DATA ON YOUR BANKSTICKS!!!

The application writes 64 byte blocks to connected banksticks, the bytes of 
a block are calculated by following formula, where bs is the bankstick number, 
block is the block number and i is the byte offset in the block:

b = ((bs + block + i) % 256)

In the next phase, those blocks are read again, the read result is checked against
the same formula use generate bytes for the blocks written. If this compare fails,
something went wrong during the write / read process.

All relevant information is outputed to the MIOS-Studio-console, during write /
read-compare, every second a progress-message is outputed, this information is
also displayd on a optionaly connected LC display.

The testrun can be started or re-started by pushing either a MIDI-note on a 
connected MIDI-device (e.g. MIOS-Studio MIDI-keyboard), or any connected DIN-
button.

Configuration:
----------
All configuration values showed here are default values

Bankssticks have to be enabled in mios32_config.h:

// enable banksticks. must be equal or more than BS_CHECK_NUM_BS (app.h)
#define MIOS32_IIC_BS_NUM 1

The size of the connected banksticks, the number of banksticks the application
should scan, and the number of test-runs has to be configured in app.h:

// define the number of banksticks you want to check (1 - 8)
#define BS_CHECK_NUM_BS 1

// define the number of 64-byte blocks per bankstick
// 24LC512: 1014
// 24LC256: 512
#define BS_CHECK_NUM_BLOCKS_PER_BS 1024

// define the number of test-runs you want the application to go through.
// can be used to test IIC stability.
#define BS_CHECK_NUM_RUNS 3


===============================================================================
