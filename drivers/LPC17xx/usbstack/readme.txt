Code Red Technologies RDB1768 Board - USB Stack and Examples
============================================================

The USB stack and associated examples provided with RDB1768 board are
based on the open source LPCUSB stack, originally written for the NXP
LPC214x microcontrollers. The original sources for the USB stack and
and examples can be found at:

  http://sourceforge.net/projects/lpcusb/

with additional information at:

  http://wiki.sikken.nl/index.php?title=LPCUSB

We have tried to keep the changes to the codebase down to a minimum, and
have commented where changes have been made as much as possible
 
The USB projects also use the 'semihosting' option in the debug
configuration so that 'printf' messages from the target are
reported in the IDE Console Window.

USBstack
--------
This is the main project which builds the LPCUSB stack as a library.
This is then used in building the various USB examples detailed below.
	
	
USB_HID Example
---------------
This is the standard LPCUSB Human Interface Device example, ported to
the LPC1768/RDB1768.

The example provides a simple HID driver. When executed on the RDB1768,
this will register itself with the connected PC as a joystick (games
controller). This can be seen by looking in the Windows Device Manager
in System Properties.


USB_HID_Mouse
-------------
This example is based on the LPCUSB USB_HID example, but has been extended
so that the RDB1768 will act as a mouse to the connected PC. To move the 
cursor, use the joystick on the RDB1768 board. Clicking on the joystick will 
act as pressing the mouse's left button.


USB_MSC Example
---------------
This is the standard LPCUSB mass storage device example, ported to
the LPC1768/RDB1768.

This example turns the RDB1768 into a mass storage device, by connecting
the onboard micro-SD card system to USB sub-system via the SPI interface
of the LPC1768. When the example is downloaded and run, the board will
appear within Windows as a mass storage drive, which can be read from and
written to as required.

Note that you will require a micro-SD card to make use of this example. 
This should be inserted into the socket underneath the RDB1768's LCD screen
before executing the example. Hot-plugging of cards is not supported.
  
  
USB_Serial Example
------------------
This example provides a minimal implementation of a USB serial port, using
the CDC class. This therefore allows you to connect to the board over USB from
a terminal program, such as hyperterminal.

By default this port of the example will simply echo everything it receives 
right back to the host PC, but incremented by 1. Thus if you type 'a' on the
host PC, the program will send back 'b'. 

The original behaviour of echoing back the same character ('a' is sent back
when 'a' was received) can be restored by changing the value of the #define of
'INCREMENT_ECHO_BY' from '1' to '0'.

In order to connect to the board from hyperterminal, or similar, copy the 
usbser.inf file (provided as part of USB_Serial project) to a temporary 
location (C:\temp is a good place).

Then plug the USB cable into the USB device port of the RDB1768 and when
requested (after downloading and starting execution of the example) direct
Windows to the temporary directory containing the usbser.inf file. Windows
then creates an extra COMx port that you can open in hyperterminal (or 
similar).

When configuring your terminal program, you should set it to append line
feeds to incoming line ends, and to echo typed characters locally. You 
will then be able to see the character you type, as well as the character
returned by the example running on the board.

USB_Serial_LCD Example
----------------------
This example is based on the LPCUSB USB_Serial example, but has been 
extended so that the code outputs status messages and characters 
received from the terminal program running on the host PC in a 
"terminal window" on the LCD screen of the RDB1768.

This project requires that a version of the RDB1768_LCDlib project 
that implements the LCD terminal functions is available within the
same workspace as this project (in addition to the USB stack 
project).

By default, this version of the code will not echo the character
back to the host (as evidence of it being received can be seen 
on the LCD).


 