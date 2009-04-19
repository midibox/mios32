
===============================================================================
MIDIbox vX32 Sequencer
===============================================================================


Copyright (C) 2008, 2009 stryd_one (stryd.one@gmail.com)
not for any use whatsoever
All rights reserved forever and blah blah. No you cannot use this. At all.

===============================================================================


===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or STM32 Primer

Optional hardware:
   o LCD

===============================================================================

This application is...

Not ready yet

===============================================================================

Description about the most important files:...

You're reading it

===============================================================================

Info...

Kill...


===============================================================================
Scrap notes go here:
===============================================================================
SysEx ID:

F0 00 08 60 00    MIDIbox vX32 Sequencer    (F0 00 08 60 nn is stryd_one's Manufacturer ID where nn is the device ID)

===============================================================================
SVN Download:

svn co svn+ssh://svn@svnmios.midibox.org/mios32 mios32

===============================================================================
Jucetice:

Install/build:

Dependencies:
sudo aptitude install build-essential libX11-dev libxinerama-dev libpthread-stubs0-dev libfreetype6-dev libgl1-mesa-dev libasound2-dev liblua50-dev lua50 libjack-dev jackd libuuid1 uuid-dev xorg-dev dssi-dev dssi-utils ladspa-sdk liblo0



Checkout and build:
Juced:

svn checkout http://juced.googlecode.com/svn/trunk/ juced
cd ./juced/tools/linux
./manage make all (or just ./manage make lib)
cd ../../juce/extras/the jucer/build/linux
make

........or........
JUCE:
mkdir juced
cd juced
svn co https://juce.svn.sourceforge.net/svnroot/juce/tags/version_1_46 juce 
cd build/linux
make CONFIG=Release
make CONFIG=Debug
cd ../../juce/extras/the jucer/build/linux
make




Skeleton files as per:

/juced/juce/extras/example projects/common
(no longer using the ones from http://www.rawmaterialsoftware.com/juceforum/viewtopic.php?t=1935)



References:

Juced at Google code:
http://code.google.com/p/juced/

Forum index:
http://www.rawmaterialsoftware.com/juceforum/index.php

Jucetice blog:
http://www.anticore.org/jucetice/

File share:
http://www.anticore.org/juce/

Juce upstream home:
http://rawmaterialsoftware.com/juce/download.php

Juce API doco:
http://www.rawmaterialsoftware.com/juce/api/main.html

Get MIDI Devs example:
http://rawmaterialsoftware.com/juceforum/viewtopic.php?t=2365

Codeblocks Project to makefile python script:
http://pix.test.at/wiki/cbp2makefile
python cbp2makefile.py vXJuce.cbp Makefile


===============================================================================
SVN Commands:

To add Id in file header like this:
/* $Id: modules.h 390 2009-03-07 12:24:00Z stryd_one $ */

In each dir:
svn ps svn:keywords Id *.c
svn ps svn:keywords Id *.h
svn ps svn:keywords Id makefile
svn ps svn:keywords Id Makefile

And in each file just put this at the top:
/* $Id:  $ */

It'll do the rest.


===============================================================================
JTAG Commands:

OpenOCD:
Primer:
sudo openocd -f $MIOS32_PATH/etc/openocd/interface/010/rlink.cfg -f $MIOS32_PATH/etc/openocd/target/010/Primer.cfg
Core32:
sudo openocd -f $MIOS32_PATH/etc/openocd/interface/010/arm-usb-ocd.cfg -f $MIOS32_PATH/etc/openocd/target/010/STM32.cfg

Sudo needed to avoid this:
Error: usb_claim_interface: could not claim interface 0: Operation not permitted
Error: detach kernel driver: could not detach kernel driver from interface 0: Operation not permitted

Also, Primer must be connected direct - no USB hubs!


Telnet:

telnet localhost 4444

reset
halt
flash probe 0
stm32x unlock 0
stm32x mass_erase 0
flash write_image /home/stryd/_mios32/mios32/trunk/apps/sequencers/vX32/mios32/vX32.hex



GDB:

arm-none-eabi-gdb

file /home/stryd/_mios32/mios32/trunk/apps/sequencers/vX32/mios32/vX32.elf
target remote localhost:3333
monitor reset
monitor halt
monitor flash probe 0
monitor stm32x unlock 0
monitor stm32x mass_erase 0
monitor flash write_image /home/stryd/_mios32/mios32/trunk/apps/sequencers/vX32/mios32/vX32.hex


===============================================================================

Geany tag generation:

cd $MIOS32_PATH
geany -g ~/.config/geany/tags/vX32.c.tags $( find $MIOS32_PATH/apps/sequencers/vX32/ -iname *.h )
geany -g ~/.config/geany/tags/vX32.C.tags $( find $MIOS32_PATH/apps/sequencers/vX32/ -iname *.h )

Then restart geany
See also:
http://www.midibox.org/forum/index.php/topic,13218.0/topicseen.html

===============================================================================

