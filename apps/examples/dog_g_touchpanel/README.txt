$Id$

Demo application for DOGM128 GLCD + Touchpanel
===============================================================================
Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 module
   o a DOGM128 based graphical LCD
   o a resistive touchpanel "EA TOUCH128", which is available as accessory to DOGM128


===============================================================================

This application demonstrates how to scan the optional touchpanel of DOGM128

The 4 pins have to be connected to J5A of the MBHP_CORE_STM32 module:

J5.A0: Right pin
J5.A1: Top pin
J5.A2: Left pin
J5.A3: Bottom pin

===============================================================================
