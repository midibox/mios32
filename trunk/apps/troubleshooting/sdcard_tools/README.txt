$Id$

SD Card Tools
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32
   o a SD Card connected to J16 of the core module

Optional hardware:
   o a LCD to display result
     (however, MIOS Terminal output is much more verbose!)

===============================================================================

This application simply turns on the LED of the MBHP_CORE_STM32 module
if a SD Card is connected.

In addition, the CSD and CSI informations are read out and print on
the MIOS Studio Terminal.

A Simple console is provided with various commands, help shows a list 

===============================================================================
