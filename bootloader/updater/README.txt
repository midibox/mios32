$Id$

Bootloader Update V1.007
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mios32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17

===============================================================================


This application has different purposes:

1) you would like to program the bootloader on a virgin STM32 or LPC17 device via JTAG
or COM interface as described under http://www.ucapps.de/mios32_bootstrap_experts.html

In this case, just use the <board>/project.bin or <board>/project.hex file.
Once uploaded, you are able to update applications via MIOS Studio


2) you got a STM32 or LPC17 with preprogrammed bootloader, but you are not sure if it
is up-to-date. Just upload the <board>/project.hex file via MIOS Studio to
check this, and to update the bootloader if it is not up-to-date


3) you know that you need to update the bootloader.
Just proceed as described under 2)


4) you want to enable the "fastboot" option, change the LCD type, 
Device ID, customize the USB Device Name, etc...



Some messages are print on LCD, but some more verbose messages are send
to the MIOS Terminal, which can be opened in MIOS Studio


If the bootloader is up-to-date, you will get following messages:

| ====================
| Bootloader V1.007
| ====================
| 
| Checking Bootloader...
| No mismatches found.
| The bootloader is up-to-date!
| You can upload another application now!
| Or type 'help' in MIOS Terminal for additional options!



If the bootloader has to be updated, you will probably get following messages:

| ====================
| Bootloader V1.007
| ====================
| 
| Checking Bootloader...
| Mismatch at address 0x00a0
| Mismatch at address 0x00a8
| Mismatch at address 0x00a9
| Mismatch at address 0x00fc
| Mismatch at address 0x0104
| Mismatch at address 0x0105
| Mismatch at address 0x0122
| Mismatch at address 0x0128
| Mismatch at address 0x012e
| Too many mismatches, no additional messages will be print...
| Bootloader requires an update...
| Starting Update - don't power off!!!
| Checking Bootloader...
| The bootloader has been successfully updated!
| You can upload another application now!
| Or type 'help' in MIOS Terminal for additional options!

IMPORTANT: DON'T POWER-OFF THE CORE DURING THE BOOTLOADER IS PROGRAMMED!
THIS CAN LEAD TO DATA CORRUPTION, SO THAT THE A JTAG OR COM INTERFACE IS
REQUIRED TO INSTALL THE BOOTLOADER AGAIN!

The status LED mounted on the MBHP_CORE_STM32/LPC17 board will flicker when
the flash erase/programming procedure is in progress.

Once the LED is permanently on, another application can be uploaded.

===============================================================================

Customizing the MIOS32 application:

The bootloader provides a permanent storage for parameters which are referenced
by (most) applications. These parameters can be customized from the MIOS Terminal
(part of MIOS Studio) by uploading this application.

Just type "help" in the MIOS Terminal to get a list of available commands.

Explanation of the most important parameters:

- fastboot: normaly after power-on the bootloader waits for an upload request for 
  ca. 3 seconds before the actual application will be started.
  This is a fail-safe measure which is mainly relevant for developers who don't
  want to open their MIDIbox and stuff the "BSL Hold" jumper (J27) if the
  application crashed during the initialisation phase.

  However, for common users this wait phase shouldn't be really necessary, especially
  if they are using a stable application.

  Therefore: enter
     set fastboot 1
  in the MIOS Terminal to skip this phase, and to start the application immediately!
  You will like this option! :-)

  Note that no MIDI upload request will be sent during power-on anymore!
  Please consider this when reading documentation about MIDI troubleshooting.


- USB device name: it's possible to assign a dedicated name for your MBHP_CORE_STM32
  or MBHP_CORE_LPC17 module which is used for the USB connection.

  This is especially useful if multiple MIDIboxes running with the same application
  are connected to your computer, so that you are able to differ between them.

  The USB name can be permanently changed with:
    set name <name>
  e.g.
    set name MIDIbox SEQ V4 - 1
  or
    set name MIDIbox SEQ V4 - 2


- Device ID: this ID is relevant once multiple cores are available on the same MIDI port,
  or if you are using your MIOS32 based core as a USB<->MIDI / OSC<->MIDI gateway to a
  PIC based MBHP_CORE.

  MIOS Studio won't be able to differ between the cores in this case if they have the
  same Device ID, therefore it's recommended to change the Device ID of the MIOS32 core

  Enter:
     set device_id 127
  to assign a new device id
  Again: this is only relevant if multiple cores are connected to the same MIDI port

  IMPORTANT NOTE: don't change the Device ID if you are using MIOS Studio 2.2.1 or lower!
  Device IDs are properly supported with MIOS Studio 2.2.2 and higher!


- LCD Type: applications which are compiled with the "universal" LCD driver can handle
  various character/graphical LCD types and display dimensions.

  It's recommended to store these parameters of your MIDIbox in the bootloader info range.

  Following commands are available:
    set lcd_type <value>       the LCD type number (enter "lcd_types" to get a list of available types)
    set lcd_num_x <value>      number of LCDs in X direction
    set lcd_num_y <value>      number of LCDs in Y direction
    set lcd_width <value>      width of a single LCD (*)
    set lcd_height <value>     height of a single LCD (*)

  (*) CLCDs: number of characters, GLCD: number of pixels

  Example: a single HD44780 based 2x20 character LCD is connected to your MIDIbox (default)
  Enter:
     lcd_type CLCD
     lcd_num_x 1
     lcd_num_y 1
     lcd_width 20
     lcd_height 2

  Example2: two HD44780 based 2x40 character LCDs are connected to your MIDIbox
  Enter:
     lcd_type CLCD
     lcd_num_x 2
     lcd_num_y 1
     lcd_width 40
     lcd_height 2

  Example3: five SSD1306 based 128x64 OLEDs are connected to your MIDIbox in vertical direction
  Enter:
     lcd_type GLCD_SSD1306
     lcd_num_x 1
     lcd_num_y 5
     lcd_width 128
     lcd_height 64


  Please note: some applications (like MIDIbox SEQ V4) could overrule the predefined
  parameters if they can't handle smaller (or larger) LCD sizes.

  Please note also: some applications could have been released with a different
  LCD driver (MIOS32_LCD != "universal") which don't consider these parameters.
  It's up to the developer to document this limitation.


In any case, please enter 'help' in MIOS Terminal to get more informations

===============================================================================
