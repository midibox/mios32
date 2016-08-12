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



Again: it's normal that some timeout messages will appear.
At the end you should see a "programming done" message (and no error message).


The firmware will print messages such as:
[239048.362] Connecting to ESP8266 in BootROM mode
[239048.362] Sync #1
[239048.362] [ESP8266_FW_Read] TIMEOUT (1)
[239048.363] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239048.363] [ESP8266_FW_Read] TIMEOUT (1)
[239048.363] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239048.562] [ESP8266_FW_Read] TIMEOUT (1)
[239048.562] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239048.563] [ESP8266_FW_Read] TIMEOUT (1)
[239048.563] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239048.662] [ESP8266_FW_Read] TIMEOUT (1)
[239048.662] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239048.663] Sync failed with status=-2, this can happen during first try - don't panic, we try again!
[239048.663] Sync #2
[239049.025] Sync #3
[239049.025] Sync #4
[239049.025] MAC: 18:FE:34:D2:CE:25
[239049.025] Chip ID: 0x00D2CE25
[239049.025] Flash ID: 0x001440E0
[239049.025] Flash Size: 1024k
[239049.026] Programming firmware...
[239049.026] [ESP8266_FW_FlashProg] Erasing Flash at 0x000000...
[239049.208] [ESP8266_FW_Read] TIMEOUT (1)
[239049.208] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239049.208] [ESP8266_FW_FlashProg] Writing at 0x000000... (7%)
[239049.208] [ESP8266_FW_FlashProg] Writing at 0x000100... (15%)
[239049.319] [ESP8266_FW_FlashProg] Writing at 0x000200... (23%)
[239049.319] [ESP8266_FW_FlashProg] Writing at 0x000300... (30%)
[239049.319] [ESP8266_FW_FlashProg] Writing at 0x000400... (38%)
[239049.319] [ESP8266_FW_FlashProg] Writing at 0x000500... (46%)
[239049.427] [ESP8266_FW_FlashProg] Writing at 0x000600... (53%)
[239049.427] [ESP8266_FW_FlashProg] Writing at 0x000700... (61%)
[239049.427] [ESP8266_FW_FlashProg] Writing at 0x000800... (69%)
[239049.427] [ESP8266_FW_FlashProg] Writing at 0x000900... (76%)
[239049.508] [ESP8266_FW_FlashProg] Writing at 0x000a00... (84%)
[239049.508] [ESP8266_FW_FlashProg] Writing at 0x000b00... (92%)
[239049.508] [ESP8266_FW_FlashProg] Writing at 0x000c00... (100%)
[239049.682] [ESP8266_FW_FlashProg] Erasing Flash at 0x001000...
[239049.682] [ESP8266_FW_Read] TIMEOUT (1)
[239049.682] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239049.682] [ESP8266_FW_FlashProg] Writing at 0x001000... (12%)
[239049.789] [ESP8266_FW_FlashProg] Writing at 0x001100... (25%)
[239049.789] [ESP8266_FW_FlashProg] Writing at 0x001200... (37%)
[239049.789] [ESP8266_FW_FlashProg] Writing at 0x001300... (50%)
[239049.789] [ESP8266_FW_FlashProg] Writing at 0x001400... (62%)
[239049.870] [ESP8266_FW_FlashProg] Writing at 0x001500... (75%)
[239049.870] [ESP8266_FW_FlashProg] Writing at 0x001600... (87%)
[239049.870] [ESP8266_FW_FlashProg] Writing at 0x001700... (100%)
[239050.096] [ESP8266_FW_FlashProg] Erasing Flash at 0x002000...
[239050.096] [ESP8266_FW_Read] TIMEOUT (1)
[239050.096] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239050.096] [ESP8266_FW_Read] TIMEOUT (1)
[239050.097] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239050.296] [ESP8266_FW_Read] TIMEOUT (1)
[239050.296] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239050.296] [ESP8266_FW_Read] TIMEOUT (1)
[239050.297] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239050.496] [ESP8266_FW_Read] TIMEOUT (1)
[239050.496] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239050.496] [ESP8266_FW_Read] TIMEOUT (1)
[239050.497] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239050.696] [ESP8266_FW_Read] TIMEOUT (1)
[239050.696] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239050.696] [ESP8266_FW_Read] TIMEOUT (1)
[239050.697] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239050.896] [ESP8266_FW_Read] TIMEOUT (1)
[239050.896] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239050.897] [ESP8266_FW_Read] TIMEOUT (1)
[239050.897] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239051.096] [ESP8266_FW_Read] TIMEOUT (1)
[239051.096] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239051.096] [ESP8266_FW_Read] TIMEOUT (1)
[239051.097] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239051.296] [ESP8266_FW_Read] TIMEOUT (1)
[239051.296] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239051.297] [ESP8266_FW_Read] TIMEOUT (1)
[239051.297] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239051.496] [ESP8266_FW_Read] TIMEOUT (1)
[239051.496] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239051.496] [ESP8266_FW_Read] TIMEOUT (1)
[239051.497] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239051.696] [ESP8266_FW_Read] TIMEOUT (1)
[239051.696] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239051.696] [ESP8266_FW_Read] TIMEOUT (1)
[239051.697] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239051.917] [ESP8266_FW_Read] TIMEOUT (1)
[239051.917] [ESP8266_FW_ReceiveReply] initial read failed, status -2
[239051.918] [ESP8266_FW_FlashProg] Writing at 0x002000... (0%)
[239051.918] [ESP8266_FW_FlashProg] Writing at 0x002100... (0%)
[239052.028] [ESP8266_FW_FlashProg] Writing at 0x002200... (0%)
[239052.028] [ESP8266_FW_FlashProg] Writing at 0x002300... (0%)
[239052.029] [ESP8266_FW_FlashProg] Writing at 0x002400... (0%)
[239052.029] [ESP8266_FW_FlashProg] Writing at 0x002500... (0%)
...
[239077.929] [ESP8266_FW_FlashProg] Writing at 0x03d400... (99%)
[239077.929] [ESP8266_FW_FlashProg] Writing at 0x03d500... (99%)
[239077.985] [ESP8266_FW_FlashProg] Writing at 0x03d600... (100%)
[239077.985] Programming done...
[239077.985] Booting new firmware...
[239077.985] 00000000  01 06                                            ..
[239077.990] rl
[239077.992] tail 4
[239077.993] chksum 0x57
[239077.997] load 0x3ffe8000, len 772, room 4 
[239077.998] tail 0
[239077.999] chksum 0x0b
[239078.000] csum 0x0b
[239078.000] 
[239078.004] rBoot v1.4.0 - richardaburton@gmail.com
[239078.006] Flash Size:   16 Mbit
[239078.007] Flash Mode:   QIO
[239078.009] Flash Speed:  40 MHz
[239078.012] rBoot Option: Big flash
[239078.014] rBoot Option: RTC data
[239078.014] 
[239078.018] Writing default boot config.
[239078.114] Booting rom 0.
[239078.339] 
[239078.339] 
[239078.344] ESP-Open-SDK ver: 0.0.1 compiled @ Aug 12 2016 20:55:25
[239078.346] phy ver: 273, pp ver: 8.3
[239078.346] 
[239078.348] SDK version:0.9.9
[239078.351] mode : sta(18:fe:34:d2:ce:25)
[239078.351] add if0
[239078.456] scandone
[239080.955] add 0
[239080.955] aid 2
[239080.956] cnt 
[239080.972] 
[239080.975] connected with MB, channel 3
[239080.977] dhcp client start...
[239082.840] ip:192.168.1.134,mask:255.255.255.0,gw:192.168.1.1


Which tells you: the upload was successful!


3) MBHP_WIFI_BRIDGE Firmware
~~~~~~~~~~~~~~~~~~~~~~~~~~~

I started to implement a firmware which allows to transfer OSC packets via WIFI.
The firmware is called MBHP_WIFI_BRIDGE and based on the SuperHouse esp-open-rtos
-> https://github.com/SuperHouse/esp-open-rtos


Once booted, enter the "!help" command -> this will forward "help" to the ESP8266 device.
It should print:
---
[239252.557] !help
[239252.563] Welcome to MBHP_WIFI_BRIDGE V0.1
[239252.566] Following commands are available:
[239252.572]   set network <network> <password>  connects to a WIFI network
[239252.576]   system:                           print system info
[239252.580]   help:                             this page
---

Enter:
   !set network <network> <password>
to specify your WIFI network credentials, e.g.:
   !set network MyWiFi MySecretPassword

This command only has to be executed once, the credentials will be permanently stored in flash


Enter:
   !system
to get some system information which might be interesting:
---
[239378.020] !system
[239378.025] MBHP_WIFI_BRIDGE V0.1
[239378.027] Chip ID: 0x00d2ce25
[239378.029] CPU Frequency: 80 MHz
[239378.032] WIFI Connection Status: #5: GOT_IP
[239378.035] IP Address: 192.168.1.134
[239378.037] IP Mask: 255.255.255.0
[239378.039] IP Gateway: 192.168.1.1
[239378.042] MAC Address: 18:FE:34:D2:CE:25
---


Currently the firmware just echos UDP packets received at port 10000

Other applications will be able to send to any UDP port!

TODO: configurable UDP receive ports

TODO: optionally turn the ESP8266 into an AP

===============================================================================
