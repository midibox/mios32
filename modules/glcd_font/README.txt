$Id$

; ==========================================================================

GLCD Fonts and Icons for MIOS32 GLCD drivers

; ==========================================================================

Copyright 2002-2008 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.

; ==========================================================================

Created in December 2002 for MIDIbox LC project, which was the very
first MIOS application

The .xpm files are manually created with "xpaint"

The appr. inc files have been converted with convpix_d.pl (located in this directory)

The .asm files are wrappers for relocatable objects, used in
C applications



Integration Hints
~~~~~~~~~~~~~~~~~

C based Applications (example for glcd_font_big)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  1) Makefile: ensure that a GLCD driver has been included, e.g.

---
include $(MIOS32_PATH)/modules/app_lcd/st7637/app_lcd.mk
---

  2) .c program: add
---
#include <glcd_font.h>
---
     and select font with:
---
  MIOS32_LCD_FontInit(GLCD_FONT_BIG);
---


Integration Examples
~~~~~~~~~~~~~~~~~~~~

  -> see $MIOS32_PATH/apps/proto/lcd_st7637
