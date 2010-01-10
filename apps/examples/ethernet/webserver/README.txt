$Id: README.txt 388 2009-03-04 23:28:41Z tk $

Demo application Webserver
===============================================================================
Copyright (C) 2010 Phil Taylor (phil@taylor.org.uk)
Based on uIP webserver Copyright (c) 2001, Swedish Institute of Computer Science.

Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or STM32 Primer
   o MBHP_CORE_ETH module
     Connected to J16 (no pull resistors, jumpered for 3.3V!)
   o MBHP_CORE_SDCARD module
	 Connected as above with card inserted
	 
Optional:
   o Any other MIDIbox module as at the moment it won't be used!
   
===============================================================================

This is a simple webserver application!

the /htdocs directory must be copied to the SD card before use.

The main problem so far is with the script support. It optionally can use dynamic memory
to load the script but that seems to randomly fail. The default is to use a static 2K buffer
but this limits the maximum size of the script to 2K.

If you want to type the dynamic allocation, add the following to your mios32_config.h:
#define WEBSERVER_USE_MALLOC				1



===============================================================================
