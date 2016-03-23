$Id$

COM Terminal for ESP8266 WiFi module
===============================================================================
Copyright (C) 2016 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17 or MBHP_CORE_STM32F4
   o MBHP_ESP8266

===============================================================================

Currently only tested with MBHP_CORE_STM32F4

Connections: see http://www.ucapps.de/mbhp/mbhp_esp8266.pdf



1) Default AT firmware
~~~~~~~~~~~~~~~~~~~~~~

The ESP-01 module comes with a default firmware which provides an so called
AT command interface.

See also
   http://www.pridopia.co.uk/pi-doc/ESP8266ATCommandsSet.pdf for the command set
   and https://cdn.sparkfun.com/assets/learn_tutorials/4/0/3/4A-ESP8266__AT_Instruction_Set__EN_v0.30.pdf
for further informations.


The MIOS32 app allows to send commands from the MIOS32 (MIDI) Terminal with:
!<command>
and it will display received messages.

E.g.
   !AT
should return
   OK


Initialisation Example for WLAN interface initialisation:
   !AT+CWMODE=3
   !AT+CWLAP
   !AT+CWJAP="<wifi-net>","<password>"
   !AT+CIFSR


2) Flash Programming
~~~~~~~~~~~~~~~~~~~~

But we are not planning to use the AT firmware, but would like to flash our
own firmware with dedicated features instead.

The MBHP_ESP8266 based adaptor provides a special "bootloader" button to
enter flash programming mode.

It can be activated the following way:

- on the MBHP_ESP8266 module, press & hold the "Bootloader" button
- shortly trigger the reset button of this module
- now release the bootloader button

AT commands won't work anymore.

Instead, you can now use the "esp8266" commands provided by the MIOS32 terminal:
[334522.043]   esp8266 baudrate <baudrate>       changes the baudrate used by the MIOS32 core (current: 115200)
[334522.043]   esp8266 reset                     resets the chip
[334522.043]   esp8266 bootloader_query          checks ESP8266 bootloader communication
[334522.043]   esp8266 bootloader_erase_flash    erases the SPI Flash
[334522.043]   esp8266 bootloader_prog_flash     programs a new ESP8266 firmware


Enter following command to check the connection to the ESP8266 bootloader:
   esp8266 bootloader_query

You should get following messages:
[334706.848] Connecting to ESP8266 in BootROM mode
[334706.848] Sync #1
[334706.848] [ESP8266_FW_Read] TIMEOUT (1)
[334706.848] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334706.848] [ESP8266_FW_Read] TIMEOUT (1)
[334706.848] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334707.048] [ESP8266_FW_Read] TIMEOUT (1)
[334707.048] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334707.048] [ESP8266_FW_Read] TIMEOUT (1)
[334707.048] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334707.148] [ESP8266_FW_Read] TIMEOUT (1)
[334707.148] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334707.148] Sync failed with status=-2, this can happen during first try - don't panic, we try again!
[334707.148] Sync #2
[334707.510] Sync #3
[334707.510] Sync #4
[334707.510] MAC: 18:FE:34:D2:CE:25
[334707.510] Chip ID: 0x00D2CE25
[334707.511] Flash ID: 0x001440E0
[334707.511] Flash Size: 1024k
[334707.511] Bootloader communication successfull!

It is normal that the first read operations timeout.
This is due to the auto-baudrate detection.

But at the end you should see the "bootloader communication successfull!" message.


In order to program our own firmware, enter following command:
   esp8266 bootloader_prog_flash

This will take several seconds - you should see messages like:

[334826.521] Connecting to ESP8266 in BootROM mode
[334826.521] Sync #1
[334826.521] Sync #2
[334826.521] Sync #3
[334826.521] Sync #4
[334826.521] MAC: 18:FE:34:D2:CE:25
[334826.521] Chip ID: 0x00D2CE25
[334826.633] Flash ID: 0x001440E0
[334826.633] Flash Size: 1024k
[334826.633] Programming firmware...
[334826.633] [ESP8266_FW_FlashProg] Erasing Flash at 0x000000...
[334826.633] [ESP8266_FW_Read] TIMEOUT (1)
[334826.633] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334826.833] [ESP8266_FW_Read] TIMEOUT (1)
[334826.833] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334826.833] [ESP8266_FW_Read] TIMEOUT (1)
[334826.833] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334827.033] [ESP8266_FW_Read] TIMEOUT (1)
[334827.033] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334827.033] [ESP8266_FW_Read] TIMEOUT (1)
[334827.033] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334827.240] [ESP8266_FW_Read] TIMEOUT (1)
[334827.240] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334827.241] [ESP8266_FW_FlashProg] Writing at 0x000000... (0%)
[334827.241] [ESP8266_FW_FlashProg] Writing at 0x000100... (1%)
[334827.349] [ESP8266_FW_FlashProg] Writing at 0x000200... (1%)
[334827.349] [ESP8266_FW_FlashProg] Writing at 0x000300... (2%)
...
[334831.471] [ESP8266_FW_FlashProg] Writing at 0x009a00... (99%)
[334831.471] [ESP8266_FW_FlashProg] Writing at 0x009b00... (100%)
[334831.472] [ESP8266_FW_FlashProg] Erasing Flash at 0x020000...
[334831.671] [ESP8266_FW_Read] TIMEOUT (1)
[334831.671] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334831.672] [ESP8266_FW_Read] TIMEOUT (1)
[334831.672] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334831.871] [ESP8266_FW_Read] TIMEOUT (1)
...
[334833.071] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334833.072] [ESP8266_FW_Read] TIMEOUT (1)
[334833.072] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[334833.217] [ESP8266_FW_FlashProg] Writing at 0x020000... (0%)
[334833.217] [ESP8266_FW_FlashProg] Writing at 0x020100... (0%)
...
[334854.740] [ESP8266_FW_FlashProg] Writing at 0x051200... (99%)
[334854.740] [ESP8266_FW_FlashProg] Writing at 0x051300... (100%)
[334854.769] Programming done...
[334854.769] Booting new firmware...


Again: it's normal that some timeout messages will appear.
At the end you should see a "programming done" message (and no error message).


The firmware will print messages such as:
[334854.773] sl
[334854.804] tail 12
[334854.805] chksum 0xd1
[334854.807] ho 0 tail 12 room 4
[334854.810] load 0x3ffe8000, len 2168, room 12 
[334854.813] tail 12
[334854.814] chksum 0xeb
[334854.816] ho 0 tail 12 room 4
[334854.819] load 0x3ffe8880, len 6520, room 12 
[334854.826] tail 12
[334854.827] chksum 0x7e
[334854.828] csum 0x7e
[334855.030] 
[334855.030] 
[334855.035] ESP-Open-SDK ver: 0.0.1 compiled @ Mar 23 2016 22:30:53
[334855.037] phy ver: 273, pp ver: 8.3
[334855.037] 
[334855.039] SDK version:0.9.9
[334856.041] mode : softAP(1a:fe:34:d2:ce:25)
[334856.042] add if1
[334856.043] bcn 100

Which tells you: the upload was successful!


3) Alternative Firmware
~~~~~~~~~~~~~~~~~~~~~~~

Currently we are using a selfbuilt binary based on the SuperHouse esp-open-rtos
-> https://github.com/SuperHouse/esp-open-rtos

It's the access_point example:
https://github.com/SuperHouse/esp-open-rtos/tree/master/examples/access_point

Which opens an AP with the name "esp-open-rtos AP"

Connect to this AP with the WLAN interface of your computer.

Enter the password "esp-open-rtos"


Now you can connect to the AP with following telnet command:
   telnet 172.16.0.1

you should get following messages:
---
Trying 172.16.0.1...
Connected to 172.16.0.1.
Escape character is '^]'.
Uptime 354 seconds
Free heap 32936 bytes
Your address is 172.16.0.2

Connection closed by foreign host.
---


Thats all for now...

Plan: add a OSC<->MIDI bridge to this firmware

===============================================================================
