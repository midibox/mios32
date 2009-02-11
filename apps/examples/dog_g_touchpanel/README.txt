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


Strategy:

All 4 pins are connected to J5 pins.
Left/Right and Bottom/Top IOs are alternated between Analog Input and PushPull Mode.
Channel 0 sets the Y coordinate when Left=0V and Right=3.3V
Channel 1 sets the X coordinate when Top=0V and Bottom=3.3V

Between switching voltages I allow a setup time of 10 mS before the analog voltage 
is converted.

If Y coordinate <= 16, it can be assumed that the touchpanel isn't pressed.

A marker is displayed on the GLCD which shows the current position.
X/Y Coordinates are displayed as well.

When the touch panel is depressed, you will notice that the marker quickly
moves to the center screen. A filter algorithm has to be implemented to ignore this.

Also a calibration routine hasn't been implemented yet.
Plan: 
   - print 'X' at upper left corner, which has to be pressed by the user.
   - print 'X' at lower right corner, which has to be pressed by the user.
   -> use the determine X/Y coordinates for scaling

===============================================================================
