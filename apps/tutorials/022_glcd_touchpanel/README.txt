$Id$

MIOS32 Tutorial #022: Graphical LCD with Touchpanel
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or STM32 Primer
   o a DOGM128 based graphical LCD
   o a resistive touchpanel "EA TOUCH128", which is available as accessory to DOGM128

===============================================================================

This tutorial requires a graphical LCD with matching touchpanel (which are
mostly available separately from the same vendor).

It has only be tested with a DOGM128 display and "EA TOUCH128" touchpanel.
Before compiling this application, please ensure that the correct LCD driver
is selected:
  export MIOS32_LCD=dog_g


The 4 pins of the touchpanel have to be connected to J5A of the MBHP_CORE_STM32
module:

J5A.A0: Right pin
J5A.A1: Top pin
J5A.A2: Left pin
J5A.A3: Bottom pin


Strategy:

The AIN_ServicePrepare() callback routine is hooked into the AIN driver. It
is called before the analog channels are scanned, so that voltages between
the Left/Right and Top/Bottom pins can be alternated between Analog Input
and PushPull Mode:
  o AIN Channel 0 sets the Y coordinate when Left=0V and Right=3.3V
  o AIN Channel 1 sets the X coordinate when Top=0V and Bottom=3.3V

Whenever the voltage configuration has been switched, a setup time of 10 mS
is inserted to ensure proper conversion results. This is done by returning
the value 1 (-> scan will be skipped). The scan is started when 
AIN_ServicePrepare() returns 0.

New conversion values are reported to APP_AIN_NotifyChange(). If the 
determined Y coordinate is <= 16, it can be assumed that the touchpanel 
isn't pressed. 

APP_Background() displays a marker on the GLCD which shows the current 
position.
X/Y Coordinates are displayed as well.

Exercises
~~~~~~~~~

1) When the touch panel is depressed, you will notice that the marker quickly
moves to the center screen. A filter algorithm has to be implemented to 
ignore this.


2) Write a calibration routine:
   - print 'X' at upper left corner, which has to be pressed by the user.
   - print 'X' at lower right corner, which has to be pressed by the user.
   -> use the determine X/Y coordinates for scaling

===============================================================================
