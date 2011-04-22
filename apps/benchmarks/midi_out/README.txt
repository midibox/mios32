$Id$

Benchmark for MIDI Out Ports
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

This benchmark sends 128 Note On events and 128 Note Off Events over different
MIDI Ports (USB0, UART0, IIC0, OSC) with and without Running Status optimisation,
and measures the transfer time.

The notes can be recorded with a sequencer for visualisation, see also this
forum posting: http://www.midibox.org/forum/index.php/topic,13542.0.html


Results STM32F103RE @ 72 MHz:
- USB0 with RS disabled:                   1.6 mS
- USB0 with RS enabled:                    1.6 mS
- UART0 with RS disabled:                245.2 mS
- UART0 with RS enabled:                 163.3 mS
- OSC with one datagram per event:        36.6 mS
- OSC with 8 events bundled in datagram:  11.4 mS


Results LPC1768 @ 100 MHz:
- USB0 with RS disabled:                   1.5 mS
- USB0 with RS enabled:                    1.5 mS
- UART0 with RS disabled:                245.1 mS
- UART0 with RS enabled:                 163.9 mS
- OSC with one datagram per event:        10.4 mS
- OSC with 8 events bundled in datagram:   3.6 mS

Results LPC1769 @ 120 MHz:
- USB0 with RS disabled:                   1.4 mS
- USB0 with RS enabled:                    1.4 mS
- UART0 with RS disabled:                245.1 mS
- UART0 with RS enabled:                 163.9 mS
- OSC with one datagram per event:         8.8 mS
- OSC with 8 events bundled in datagram:   2.5 mS

===============================================================================
