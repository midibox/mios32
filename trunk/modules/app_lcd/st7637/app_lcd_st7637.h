// $Id$
/*
 * ST7637 specific defines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _APP_LCD_ST7637_H
#define _APP_LCD_ST7637_H

// from STM primer code

/* LCD Commands */
#define DISPLAY_ON               0xAF
#define DISPLAY_OFF              0xAE
#define START_LINE               0xC0
#define START_COLUMN             0x00
#define CLOCKWISE_OUTPUT         0xA0
#define DYNAMIC_DRIVE            0xA4
#define DUTY_CYCLE               0xA9
#define READ_MODIFY_WRITE_OFF    0xEE
#define SOFTWARE_RESET           0xE2

#define ST7637_NOP               0x00
#define ST7637_SWRESET           0x01
#define ST7637_RDDID             0x04
#define ST7637_RDDST             0x09
#define ST7637_RDDPM             0x0A
#define ST7637_RDDMADCTR         0x0B
#define ST7637_RDDCOLMOD         0x0C
#define ST7637_RDDIM             0x0D
#define ST7637_RDDSM             0x0E
#define ST7637_RDDSDR            0x0F

#define ST7637_SLPIN             0x10
#define ST7637_SLPOUT            0x11
#define ST7637_PTLON             0x12
#define ST7637_NORON             0x13

#define ST7637_INVOFF            0x20
#define ST7637_INVON             0x21
#define ST7637_APOFF             0x22
#define ST7637_APON              0x23
#define ST7637_WRCNTR            0x25
#define ST7637_DISPOFF           0x28
#define ST7637_DISPON            0x29
#define ST7637_CASET             0x2A
#define ST7637_RASET             0x2B
#define ST7637_RAMWR             0x2C
#define ST7637_RGBSET            0x2D
#define ST7637_RAMRD             0x2E

#define ST7637_PTLAR             0x30
#define ST7637_SCRLAR            0x33
#define ST7637_TEOFF             0x34
#define ST7637_TEON              0x35
#define ST7637_MADCTR            0x36
#define ST7637_VSCSAD            0x37
#define ST7637_IDMOFF            0x38
#define ST7637_IDMON             0x39
#define ST7637_COLMOD            0x3A

#define ST7637_RDID1             0xDA
#define ST7637_RDID2             0xDB
#define ST7637_RDID3             0xDC

#define ST7637_DUTYSET           0xB0
#define ST7637_FIRSTCOM          0xB1
#define ST7637_OSCDIV            0xB3
#define ST7637_PTLMOD            0xB4
#define ST7637_NLINVSET          0xB5
#define ST7637_COMSCANDIR        0xB7
#define ST7637_RMWIN             0xB8
#define ST7637_RMWOUT            0xB9

#define ST7637_VOPSET            0xC0
#define ST7637_VOPOFSETINC       0xC1
#define ST7637_VOPOFSETDEC       0xC2
#define ST7637_BIASSEL           0xC3
#define ST7637_BSTBMPXSEL        0xC4
#define ST7637_BSTEFFSEL         0xC5
#define ST7637_VOPOFFSET         0xC7
#define ST7637_VGSORCSEL         0xCB

#define ST7637_ID1SET            0xCC
#define ST7637_ID2SET            0xCD
#define ST7637_ID3SET            0xCE

#define ST7637_ANASET            0xD0
#define ST7637_AUTOLOADSET       0xD7
#define ST7637_RDTSTSTATUS       0xDE

#define ST7637_EPCTIN            0xE0
#define ST7637_EPCTOUT           0xE1
#define ST7637_EPMWR             0xE2
#define ST7637_EPMRD             0xE3
#define ST7637_MTPSEL            0xE4
#define ST7637_ROMSET            0xE5
#define ST7637_HPMSET            0xEB

#define ST7637_FRMSEL            0xF0
#define ST7637_FRM8SEL           0xF1
#define ST7637_TMPRNG            0xF2
#define ST7637_TMPHYS            0xF3
#define ST7637_TEMPSEL           0xF4
#define ST7637_THYS              0xF7
#define ST7637_FRAMESET          0xF9
                                 
#define ST7637_MAXCOL            0x83
#define ST7637_MAXPAG            0x83

#define V9_MADCTRVAL                0x90     /*!< Left orientation value.     */
#define V12_MADCTRVAL               0x30     /*!< Up orientation value.       */
#define V3_MADCTRVAL                0x50     /*!< Right orientation value.    */
#define V6_MADCTRVAL                0xF0     /*!< Bottom orientation value.   */

#endif /* _APP_LCD_ST7637_H */
