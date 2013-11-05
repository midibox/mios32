$Id: README.txt 1185 2011-04-22 18:32:15Z tk $

Benchmark for MIDI Parser
===============================================================================
Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or STM32 Primer or MBHP_CORE_LPC17

===============================================================================

This benchmark searches a SysEx string with 9 bytes in a search storage of 256 entries.

The storage is organized in a linear way, no binary search tree or similar is
used to simplify the implementation.

Two different methods are used: storage with fixed lenght entries, and with
variable length entries (has the advantage that more entries could be stored if
some only specify 2 bytes like for common MIDI events...)

The benchmark demonstrates, that this implementation is sufficient, because the
SysEx string is found in much less time than it's transmitted over MIDI (9 bytes == 2.9 mS)

For LPC17 we also check if it makes a difference if the storage is located in CPU or AHB RAM.


Results STM32F103RE @ 72 MHz:
- Testing linear search in RAM                          0.451 mS
- Testing linear search with known length in RAM        0.535 mS (+16%)

Results LPC1769 @ 120 MHz:
- Testing linear search in RAM                          0.176 mS
- Testing linear search in AHB RAM                      0.193 mS (+8%)
- Testing linear search with known length in RAM        0.242 mS (+27%)
- Testing linear search with known length in AHB RAM    0.242 mS (no change compared to CPU RAM - interesting...)

Results STM32F407 @ 168 MHz:
- Testing linear search in RAM                          0.140 mS
- Testing linear search with known length in RAM        0.171 mS

===============================================================================
