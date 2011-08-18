$Id$

Bootloader Update V1.5
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


4) you want to change the Device ID or custize the USB Device Name



Some messages are print on LCD, but some more verbose messages are send
to the MIOS Terminal, which can be opened in MIOS Studio


If the bootloader is up-to-date, you will get following messages:

| ====================
| Bootloader V1.5
| ====================
| 
| Checking Bootloader...
| No mismatches found.
| The bootloader is up-to-date!
| You can upload another application now!
| Or type 'help' in MIOS Terminal for additional options!



If the bootloader has to be updated, you will probably get following messages:

| ====================
| Bootloader V1.5 
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

For programming Device ID or USB Device Name:
enter 'help' in MIOS Terminal (part of MIOS Studio) to get more informations

===============================================================================
