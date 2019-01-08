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
   o 1 - 8 Banksticks / 1 - 16 FM24Cxxx devices

Optional hardware:
   o LC Display
   o DIN buttons

===============================================================================

IMPORTANT NOTE: RUNING THIS APPLICATION WILL DESTROY ALL DATA ON YOUR BANKSTICKS!!!

The application writes data blocks to connected banksticks or FRAM devices, the bytes of 
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

// enable banksticks. must be equal or more than BS_CHECK_NUM_DEVICES (app.h)
// will be ignored if BS_CHECK_USE_FRAM_LAYER is set
#define MIOS32_IIC_BS_NUM 1

The size of the connected devices, the number of devices the application
should scan, and the number of test-runs has to be configured in app.h:

// define the number of devices you want to check (1 - 8 for banksticks)
#define BS_CHECK_NUM_DEVICES 1

// data block size (max. 64 if BS_CHECK_USE_FRAM_LAYER==0)
#define BS_CHECK_DATA_BLOCKSIZE 64

// define the number of blocks per device
// each block is 64 bytes (or BS_CHECK_FRAM_BLOCKSIZE if BS_CHECK_USE_FRAM_LAYER != 0)
// 24LC512: 1024
// 24LC256: 512
#define BS_CHECK_NUM_BLOCKS_PER_DEVICE 1024

// define the number of test-runs you want the application to go through.
// can be used to test IIC stability.
#define BS_CHECK_NUM_RUNS 3


Use the FRAM - module instead of the bankstick-layer:

// instead of the bankstick layer, the FRAM-layer (module) can be used
// ( FM24Cxxx devices). This enables to address up to 32 devices if
// the multiplex-option of the FRAM module is used.
#define BS_CHECK_USE_FRAM_LAYER 0




===============================================================================
