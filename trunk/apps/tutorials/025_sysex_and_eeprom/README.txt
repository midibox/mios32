$Id$

MIOS32 Tutorial #025: SysEx Parser and EEPROM Emulation
===============================================================================
Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17 or MBHP_CORE_STM32F4 or similar

Optional hardware:
   o LCD and DIN to "fire" a dump
   o one or more 32k BankSticks (external IIC EEPROMs)

===============================================================================

This tutorial application demonstrates a SysEx parser for handling "patch dumps", 
which are stored into an external EEPROM (BankStick), or into an internal
"emulated" EEPROM (see also http://www.midibox.org/mios32/manual/group___e_e_p_r_o_m.html )

In addition, it provides a function to dump out the EEPROM content in the same
SysEx format, so that data can be easily stored/restored with an external SysEx
tool like MIDI-Ox (Windows), Snoize SysEx Librarian (MacOS), or similar.

The aim is to provide a template for programmers who want to integrate such a
dump mechanism into their application. Due to different requirements, some
compile-options are provided to increase the flexibility.

The handler has been written for robustness, therefore it consumes a bit more
code than a slight solution, which might already fit your needs. On the other
hand, it will make your application more user-friendly, as erroneous MIDI
streams are detected, and acknowledged with an error message.

===============================================================================

Description about the most important files:

   - app.c: the main program with all MIOS hooks and branches to SYSEX_*
     routines.
     In addition, a small button/LCD interface is provided (Exec/Inc/Dec button)
     which allows to send a SysEx dump of a selected patch manually.

   - sysex.c: SysEx parser and sender routine

   - sysex.h: check the compile options

   - patch.c: abstraction layer for patch read/write from RAM, and load/store
     from EEPROM. 
     The patch_structure[] array is usually replaced by the data structure(s)
     used in your application.

===============================================================================

Dump Format
~~~~~~~~~~~

SysEx streams are starting with F0, and ending with F7
Inside such streams, only 7bit values can be sent, because once the 8th bit
is set (0x80..0xf7), the byte will be interpreted as the beginning of a new
MIDI event; values >= 0xf8 will be interpreted as realtime messages.

This coding of the MIDI protocol requires special measures when transfering
data from/to an EEPROM

a) if the 8th bit is not relevant for your application (e.g. CC values are
stored in EEPROM), you could easily discard the 8th bit during a patch exchange
by masking it out

b) if the 8th bit is relevant, you could send the 8bit values in two pieces,
eg. as a "low and high nibble" (a nibble contains 4bit). The remaining 4bit
are always 0

c) you could scramble/serialize the data dump, so that all 7bits are utilized
for 8bit values


This application supports method a) and b)
(-> sysex.h, SYSEX_FORMAT constant)

Method c) - which is used by MIOS for optimized data exchange - is not 
supported here, as it would make the handler even bigger (code would look 
more complicated for a programmer newbie...). Another disadvantage: this
format is only editable with a decoder/encoder at the PC side


Checksum
~~~~~~~~

In order to provide some extra protection against a bad MIDI connection (e.g.
caused by flaws of a MIDI interface or driver), a checksum can be optionally
inserted at the end of a SysEx dump. 

Advantage: a certain protection, that corrupted (wrong) data values will be
detected.

Disadvantage: if a SysEx dump should be edited with a common editor at the PC
side, the checksum has to be recalculated, otherwise the dump won't be accepted
by the application anymore (-> checksum error)

Checksum calculation: (<sum of all bytes> & 0x7f) ^ 0x7f
(so called "2s complement")


SysEx Header
~~~~~~~~~~~~

Following SysEx structure is used:

<header> <command> <bank> <patch> <dump> [<checksum>] <footer>
or
<header> <command> <bank> <patch> <footer>
or
<header> <command> <your data> <footer>



<header>: F0 00 00 7E 7F by default. In order to make the SysEx stream header
of your application unique (to avoid incompatibilities with other apps), please 
change the numbers after F0
"F0 00 00 7E" is used by TK's MIDIbox firmwares. Others could use the same
header where only the last value is changed, but this should be documented under
http://svnmios.midibox.org/filedetails.php?repname=svn.mios&path=%2Ftrunk%2Fdoc%2FSYSEX_HEADERS

<command>: see next chapter

<bank>: as preparation for BankStick accesses, will be forwarded to the
Load/Store functions in patch.c

<patch>: dito

<dump>: the data dump, either 256 bytes (7bit format) or 512 bytes (2*4bit format)
depending on your selection in sysex.h
The size can be optionally decreased by changing the SYSEX_PATCH_SIZE constant in sysex.h

<checksum>: can be optionally enabled in sysex.h

<footer>: always F7

<your-data>: for the case that your Command handler interprets the data on
a different way



SysEx Commands
~~~~~~~~~~~~~~

The SysEx header is followed by a command which allows to select a SYSEX_Cmd_*
function in sysex.c - by doing so, one header can be used for multiple purposes

 - 01: Request dump:
   F0 00 00 7E 7F 01 <bank> <patch> F7
   Sends back the specified patch in the given bank

   The dump itself contains command number 02 (so that it can be sent back to
   store the patch)


  - 02: Write dump:
    F0 00 00 7E 7F 02 <bank> <patch> <dump> [<checksum>] F7
    Writes a dump into EEPROM. Dump size and optional checksum depends on settings
    in sysex.h

  - 0F: Ping
    F0 00 00 7E 7F 0F F7
    Sends back an acknowledge to say "I'm here"


Error Codes
~~~~~~~~~~~

Commands will be acknowledged (ok) or disacknowledged (error)

Acknowledge string:
    F0 00 00 7E 7F 0F 00 F7

Disacknowledge String:
    F0 00 00 7E 7F 0E <error> F7

Error Codes are documented in sysex.h:
SYSEX_DISACK_LESS_BYTES_THAN_EXP  0x01
SYSEX_DISACK_MORE_BYTES_THAN_EXP  0x02
SYSEX_DISACK_WRONG_CHECKSUM       0x03
SYSEX_DISACK_BS_NOT_AVAILABLE     0x0a
SYSEX_DISACK_INVALID_COMMAND      0x0c


Recommented Adaptions
~~~~~~~~~~~~~~~~~~~~~

patch.c has to be adapted to your application. 

Especially the patch_structure[] array should be replaced by an already
existing data structure, or you have to store the informations there
to avoid redundancy (unnecessary RAM allocation)

preload.asm should contain the default data which matches with the
requirements of your application.


Optional Adaptions
~~~~~~~~~~~~~~~~~~

sysex.h:
SYSEX_CHECKSUM_PROTECTION
SYSEX_FORMAT
SYSEX_PATCH_SIZE

patch.h:
PATCH_USE_BANKSTICK

sysex.c:
more commands can be hooked into the SYSEX_Cmd() function

patch.c:
access layer to data structures (PATCH_ReadByte/WriteByte) and
EEPROM store operations (PATCH_Load/Store) can be changed as desired


Warning
~~~~~~~

EEPROM store operations are blocking operations. There is a danger
for MIDI IN buffer overruns if additional MIDI data is received
while programming is in progress. 

There are two possibilities to overcome this limitation:

a) ensure, that no additional MIDI data is sent during EEPROM programming
is in progress. For 256 bytes it could take up to 25 mS (if programmed
into internal EEPROM) or ca. 200 mS (if programmed into BankStick)

This is a common limitation also known from most synthesizers, therefore
tools like MIDI-Ox and SysEx Librarian allow you to insert delays after
the F7 (SysEx footer byte) has been sent.
Recommented delay is 750 mS to be at the secure side.


b) write a tool which uses the acknowledge message for handshaking.
The acknowledge is sent *after* the EEPROM has been programmed. Since this
time is variable - e.g., if to be programmed bytes are already located in
EEPROM, they won't be written again to save time (and lifetime)) - programming
could be much shorter.

Such a feature is especially nice if multiple patches are sent, one after
another. It can reduce the download time dramatically.

The tool could also handle the error messages, e.g. retry to send a patch
if an error has been received, or at least print the error out in human
readable form. 

Example: MIOS Studio (smart mode) and MIDIbox SID V2 Editor - both are Java
applications.

===============================================================================
