$Id$

Bootloader Update V1.2
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



Some messages are print on LCD, but some more verbose messages are send
to the MIOS Terminal, which can be opened in MIOS Studio


If the bootloader is up-to-date, you will get following messages:

| ====================
| Bootloader V1.0
| ====================
| 
| Checking Bootloader...
| No mismatches found.
| The bootloader is up-to-date!
| ... (messages repeat until a new application is uploaded)



If the bootloader has to be updated, you will probably get following messages:

| ====================
| Bootloader V1.0
| ====================
| 
| Checking Bootloader...
| Mismatch at address 0x0090
| Mismatch at address 0x0091
| Mismatch at address 0x00d4
| Mismatch at address 0x00d5
| Mismatch at address 0x00dc
| Mismatch at address 0x00dd
| Mismatch at address 0x02ac
| Mismatch at address 0x02ad
| Mismatch at address 0x02c0
| Too many mismatches, no additional messages will be print...
| Bootloader requires an update...
| Bootloader update in 10 seconds!
| Bootloader update in 9 seconds!
| Bootloader update in 8 seconds!
| Bootloader update in 7 seconds!
| Bootloader update in 6 seconds!
| Bootloader update in 5 seconds!
| Bootloader update in 4 seconds!
| Bootloader update in 3 seconds!
| Bootloader update in 2 seconds!
| Bootloader update in 1 seconds!
| Bootloader update in 0 seconds!
| Starting Update - don't power off!!!
| Checking Bootloader...
| The bootloader has been successfully updated!
| You can upload another application now!
| The bootloader has been successfully updated!
| You can upload another application now!
| ... (messages repeat until a new application is uploaded)


As you can see, the bootloader waits for 10 seconds before the flash section
is reprogrammed.

IMPORTANT: DON'T POWER-OFF THE CORE DURING THE BOOTLOADER IS PROGRAMMED!
THIS CAN LEAD TO DATA CORRUPTION, SO THAT THE A JTAG OR COM INTERFACE IS
REQUIRED TO INSTALL THE BOOTLOADER AGAIN!

The status LED mounted on the MBHP_CORE_STM32/LPC17 board will flicker 
if a check, the countdown or the flash erase/programming procedure is in progress.
As long as this happens, the core shouldn't be powered off or rebooted!

Once the LED is permanently on, another application can be uploaded.

===============================================================================
