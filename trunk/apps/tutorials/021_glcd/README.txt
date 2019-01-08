$Id$

MIOS32 Tutorial #021: Graphical LCD Output
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17 or MBHP_CORE_STM32F4 or similar
   o a graphical LCD

===============================================================================

This tutorial requires on of the following graphical LCDs:

dog_g    a graphical DOG LCD
ks0108   a KS0108/0107 based graphical LCD
pcd8544  a PCD8544 based graphical LCD ("Nokia Display")
st7637   a ST7637 based graphical colour LCD as used for the STM32 Primer
t6963_h  a T6963 based graphical LCD; characters displayed in horizontal orientation
t6963_v  a T6963 based graphical LCD; characters displayed in vertical orientation

See also http://www.ucapps.de/mbhp_lcd.html for schematics


Before executing the Makefile, the LCD type has to be written into the MIOS32_LCD
environment variable.
Example:
  export MIOS32_LCD=dog_g

Note that MIOS32 settings can also be done in the ~/.bashrc_profile, so that
they will be already available on each new terminal session.


The same functions like for character LCDs are available:
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


In addition, following functions are only relevant for graphical LCDs:
  o MIOS32_LCD_FontInit(u8 *font)
    switches to another LCD font (or icon set)

  o MIOS32_LCD_GCursorSet(u16 x, u16 y)
    sets the graphical cursor

  o MIOS32_LCD_BColourSet(u8 r, u8 g, u8 b)
    sets the background colour (currently only supported for ST7637 based GLCD)

  o MIOS32_LCD_FColourSet(u8 r, u8 g, u8 b)
    sets the foreground colour (currently only supported for ST7637 based GLCD)

All available LCD functions are listed under:
  http://www.midibox.org/mios32/manual/group___m_i_o_s32___l_c_d.html



Some prepared Fonts/Icons are located under $MIOS32_PATH/modules/glcd_font
They are already compiled into the project if a GLCD driver has been
selected (no need to extend the Makefile)

  o GLCD_FONT_NORMAL (6x8 character set)
  o GLCD_FONT_SMALL (4x8 character set)
  o GLCD_FONT_BIG (16x24 character set)
  o GLCD_FONT_KNOB_ICONS (28x24 round knobs, 12 different icons)
  o GLCD_FONT_METER_ICONS_H (8x28 horizontal meter bars, 28 icons, 14 with/14 without overload marker)
  o GLCD_FONT_METER_ICONS_V (32x8 vertical meter bars, 28 icons, 14 with/14 without overload marker)


References to these fonts are available with following header file:
-------------------------------------------------------------------------------
#include <glcd_font.h>
-------------------------------------------------------------------------------



Following example (located in app.c) prints up to 16 horizontal meter bars
to display the last received ModWheel CC values for each MIDI channel (the 
displayed number of meters depends on the display size):
-------------------------------------------------------------------------------
    for(i=0; i<16; ++i) {
      MIOS32_LCD_GCursorSet(0, i*8);
      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_NORMAL); // 6x8 font
      MIOS32_LCD_PrintFormattedString("%03d ", modwheel_values[i]);

      MIOS32_LCD_FontInit((u8 *)GLCD_FONT_METER_ICONS_H); // memo: 28 icons, 14 used, icon size: 28x8
      MIOS32_LCD_PrintChar((14 * modwheel_values[i]) / 128);
    }
-------------------------------------------------------------------------------


Background informations about the MIOS32 font format
----------------------------------------------------

See the normal font, located under $MIOS32_PATH/modules/glcd_font/glcd_font_normal.c
as example:
-------------------------------------------------------------------------------
const u8 GLCD_FONT_NORMAL[] = {
6, 1*8, 0, 6, // width, height, X0 offset, char offset
0x00,0x80,0x80,0x80,0x80,0x80,0x00,0xc0,0xc0,0xc0,0xc0,0xc0,0x00,0xe0,0xe0,0xe0,
0xe0,0xe0,0x00,0xf0,0xf0,0xf0,0xf0,0xf0,0x00,0xf8,0xf8,0xf8,0xf8,0xf8,0x00,0xfc,
0xfc,0xfc,0xfc,0xfc,0x00,0xfe,0xfe,0xfe,0xfe,0xfe,0x00,0xff,0xff,0xff,0xff,0xff,
...
... a lot of additional bytes
...
-------------------------------------------------------------------------------

The first four bytes define:
   - the first byte defines the font width
   - the second byte defines the font height, which *must* be multiples of 8
   - the third byte defines an optional offset at which the GLCD driver starts
     to copy the font (see glcd_font_knob_icons.c)
   - the fourth byte defines the number of bytes between each character/icon
   - the following bytes define the bitmap, starting with character 0x00
     which can be print with MIOS32_LCD_PrintChar(0x00);

In this example, the next character will follow at +6 offset, the following 
at +12, etc.

So, let's search for the graphical information of character 'A', ASCII code 65,
which is located at offset 4 + 6*65 = 394 according to given constraints:
-------------------------------------------------------------------------------
0x00,0x32,0x49,0x79,0x41,0x3e,0x00,0x7e,0x11,0x11,0x11,0x7e,0x00,0x7f,0x49,0x49,
                              =============================
-------------------------------------------------------------------------------

Let's convert these numbers into binary format:
0x00 == 0b00000000
0x7e == 0b01111110
0x11 == 0b00010001
0x11 == 0b00010001
0x11 == 0b00010001
0x7e == 0b01111110

These bytes are transfered to the (vertically oriented) display into 6 bitline
columns. Let's rotate the bit pattern by 90° anti-clockwise to see the result
(1 == *, 0 == .)
..***.
.*...*
.*...*
.*...*
.*****
.*...*
.*...*
......

Looks like an 'A', no?


Of course, while creating the character/icon sets, I haven't hacked them
bytewise into the .c file. Instead I used "xpaint", a small pixel oriented
drawing software which I found comfortable enough, especially because of the
very useful zoom function.
Bitmaps have been stored in .xpm format (these files are available in the 
SVN repository as well), and converted with a selfwritten perl script
into the MIOS32 format.

Alternatively, the GLCD character/Icon editor of Captain Hastings could
be used, which is available under: http://compiler.kaustic.net/

Unfortunately it doesn't support the C format of MIOS32 yet!
(but somebody could write an assembly->C format converter)


Exercise
--------

Change the code in APP_Background(), so that modwheel_values[]
are displayed with vertical bars.

===============================================================================
