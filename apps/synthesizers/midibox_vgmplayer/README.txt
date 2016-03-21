MIDIbox VGM Player
===============================================================================
Copyright (C) 2016 Sauraen (sauraen@gmail.com)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mios32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32F4
   o SD card containing uncompressed VGM files in the root directory, whose
     names are 8 characters or less and end in .VGM
   o MBHP_Genesis (http://wiki.midibox.org/doku.php?id=mbhp_genesis)
   o MBHP_Genesis_LS (http://wiki.midibox.org/doku.php?id=mbhp_genesis_ls)
   o 2x16 or larger character LCD
   o DIN shift register with encoder connected to pins 0 and 1, and buttons
     connected to the remaining pins--the buttons are Enter, Back, and S1-S4

===============================================================================

This application plays VGM files from the SD card on a MBHP_Genesis module.
TODO

===============================================================================
