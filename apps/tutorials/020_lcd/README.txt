$Id$

MIOS32 Tutorial #020: LCD Output
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17
   o a character or graphical LCD

===============================================================================

Back to the basics: this tutorial demonstrates the usage of the MIOS32_LCD
driver, which allows to output characters on following supported display types:

clcd     a common HD44780 compatible character LCD
dog      similar to CLCD, but with support for special DOG display commands
dog_g    a graphical DOG LCD
ks0108   a KS0108/0107 based graphical LCD
pcd8544  a PCD8544 based graphical LCD ("Nokia Display")
st7637   a ST7637 based graphical colour LCD as used for the STM32 Primer
t6963_h  a T6963 based graphical LCD; characters displayed in horizontal orientation
t6963_v  a T6963 based graphical LCD; characters displayed in vertical orientation

See also http://www.ucapps.de/mbhp_lcd.html for schematics


The LCD driver is located under $MIOS32_PATH/modules/app_lcd, and selected
with the environment variable MIOS32_LCD, which has to be set to the correct
value before the Makefile is executed.

Usually it is set to clcd for a common character display:
  export MIOS32_LCD=clcd

This variable setting (made in the command shell), is taken over in the Makefile
with:
-------------------------------------------------------------------------------
LCD       =     $(MIOS32_LCD)
-------------------------------------------------------------------------------


Finally the LCD driver is selected with:
-------------------------------------------------------------------------------
# application specific LCD driver (selected via makefile variable)
include $(MIOS32_PATH)/modules/app_lcd/$(LCD)/app_lcd.mk
-------------------------------------------------------------------------------

It is recommented not to change this mechanism, as it provides highest flexibility:
the (experienced) user specifies the LCD type connected to his core inside
the command shell (or in his ~/.bashrc_profile), so that he will be able to 
compile any application with the correct LCD driver w/o touching the Makefile!

Only if an application is dedicated to a certain LCD type, the LCD variable
assignment should be explicitely set to the correct type in the Makefile
(-> see $MIOS32_PATH/apps/mios32_test/app_lcd)


All available LCD functions are listed under:
  http://www.midibox.org/mios32/manual/group___m_i_o_s32___l_c_d.html

Note that MIOS32_LCD_Init(0) is already called by the programming model before
APP_Init() will be executed. After APP_Init() the boot screen will be
displayed for two seconds (as specified in your local mios32_config.h file).
Thereafter the LCD output is under control of the application.


Most important LCD functions:
  o MIOS32_LCD_Clear()
    clears the display

  o MIOS32_LCD_CursorSet(u16 column, u16 line)
    sets the cursor position (Column/Line == X/Y)

  o MIOS32_LCD_PrintChar(char c)
    prints a single character

  o MIOS32_LCD_PrintString(char *str)
    prints a string

  o MIOS32_LCD_PrintFormattedString(char *format,...)
    prints a formatted string


MIOS32_LCD_PrintFormattedString() works similar to "printf", but doesn't
provide all formatting possibilities to save program memory and
speed up execution.

Here a general overview of formats:
http://www.pixelbeat.org/programming/gcc/format_specs.html

Supported conversion specifiers:
%s (string)
%d (signed int)
%x (unsigned hex int with lower-case letters)
%X (unsigned hex int with upper-case letters)
%u (unsigned int)
%c (single character)

Supported flags:
0[field with] (pad with zeroes, e.g. %03d for 000..999)
- (to print item left-aligned)


Following code of the example in app.c:
-------------------------------------------------------------------------------
    // print system time
    MIOS32_LCD_CursorSet(0, 0); // X, Y
    MIOS32_LCD_PrintFormattedString("System Time");

    MIOS32_LCD_CursorSet(0, 1); // X, Y
    mios32_sys_time_t t = MIOS32_SYS_TimeGet();
    int hours = t.seconds / 3600;
    int minutes = (t.seconds % 3600) / 60;
    int seconds = (t.seconds % 3600) % 60;
    int milliseconds = t.fraction_ms;
    MIOS32_LCD_PrintFormattedString("%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
-------------------------------------------------------------------------------

will lead to following display output:

+----------------+
|System Time     |
|00:01:42.123    |
+----------------+


Exercise
--------

Print the digital state of J5A.A0-A3, J5B.A4-A7 and J5.C.A8-A11 on the
display.

Note that the 005_polling_j5_pins tutorial explains how to configure these
pins in INPUT mode, and how to poll the pins via MIOS32_BOARD_J5_PinGet()

===============================================================================
