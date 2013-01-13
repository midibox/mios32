$Id$

Demo application for Multiple Character LCDs (CLCDs)
===============================================================================
Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17 module w/ one or more CLCDs

===============================================================================

This application demonstrates the CLCD driver for multiple CLCDs

The upper line of each LCD should show a (unique) number:
   CLCD #<X>.<Y>
and some animated bars

The lower line will show "READY."


The number of connected CLCDs has to be configured with the Bootloader Update app
See also http://www.ucapps.de/mios32_bootstrap_newbies.html

The first two CLCDs are connected to J15A and J15B

Remaining CLCDs are connected in parallel to J15 data and control lines, with one
exception: the E input of each CLCD is connected to dedicated output pins of a
MBHP_DOUT module which is plugged into the J28 port of the MBHP_CORE_LPC17 module.


Configuration Example:
in order to access 34 2x20 CLCDs (requires a MBHP_DOUTX4 module connected to J28),
enter following commands into the MIOS32 Bootloader Update application:

set lcd_num_x 2
set lcd_num_y 17
set lcd_width 20
set lcd_height 2
store

===============================================================================
