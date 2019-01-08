$Id$

Benchmark for MIDI Out Scheduler
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or STM32 Primer

Optional hardware:
   o a LCD to display result
     (however, MIOS Terminal output is much more verbose!)

===============================================================================

This benchmark plays a MIDI song over a dummy MIDI Out port, and measures
the time until the song has been completed.

Play a note to start the measuring!

The BPM generator is bypassed, ticks are incremented immediately to reach
maximum throughput. The usage of a dummy interface ensures, that interface
timings don't falsify the results.

Different memory allocation methods can be selected in mios32_config.h

Here the results - than lower the value, than faster the handling:
0: internal static allocation with one byte for each flag     589.3 mS
1: internal static allocation with 8bit flags                 589.5 mS
2: internal static allocation with 16bit flags                602.5 mS
3: internal static allocation with 32bit flags                590.9 mS
4: FreeRTOS based pvPortMalloc                                619.4 mS
5: malloc provided by library                                 setup not done for Newlib


Please note: like each benchmark, the results cannot give an answer to the
real benefits of a certain method. E.g., while method 0..3 are using a
static heap to ensure, that MIDI events won't be skipped because nonavailable
memory, method 4 could result into less memory consumption, but also to even
worse performance once a garbage collection is added, or once different tasks are 
allocating and deallocating memory so that the heap gets fragmented.

Therefore, please see this benchmark as an evaluation platform for different
malloc algorithms.

Possible enhancements:
   - remove the portENTER_CRITICAL/portEXIT_CRITICAL statements while running
     the benchmark, and start other tasks with higher priority which are
     randomly allocating and releasing memory
   - play MIDI events over USB interface (requires to remove the *CRITICAL
     functions as well)

===============================================================================

Updates for LPC1768 @ 100 MHz
0: internal static allocation with one byte for each flag     335.0 mS
1: internal static allocation with 8bit flags                 343.1 mS
2: internal static allocation with 16bit flags                339.9 mS
3: internal static allocation with 32bit flags                337.5 mS
4: FreeRTOS based pvPortMalloc                                356,5 mS
5: malloc provided by library                                 setup not done for Newlib

Updates for LPC1769 @ 120 MHz
3: internal static allocation with 32bit flags                303.5 mS

Updates for STM32F407 @ 168 MHz
3: internal static allocation with 32bit flags                162.3 mS

===============================================================================
