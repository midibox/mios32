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
   o LCD to display result

===============================================================================

This benchmark plays a MIDI song over a dummy MIDI Out port, and measures
the time until the song has been completed.

Play a note to start the measuring!

The BPM generator is bypassed, ticks are incremented immediately to reach
maximum throughput. The usage of a dummy interface ensures, that interface
timings don't falsify the results.

Different memory allocation methods can be selected in mios32_config.h

Here the results - than lower the value, than faster the handling:
0: internal static allocation with one byte for each flag     617.8 mS
1: internal static allocation with 8bit flags                 622.7 mS
2: internal static allocation with 16bit flags                625.2 mS
3: internal static allocation with 32bit flags                618.7 mS
4: FreeRTOS based pvPortMalloc                                646.7 mS
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
