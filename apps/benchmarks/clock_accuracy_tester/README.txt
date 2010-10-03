$Id$

Clock Accuracy Tester
===============================================================================
Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://svnmios.midibox.org/filedetails.php?repname=svn.mios32&path=%2Ftrunk%2Fdoc%2FMEMO

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or STM32 Primer

===============================================================================

This program measures the delay between incoming MIDI clock events and prints:
  - the current BPM
  - the minimum/average/maximum measured delay between MIDI clocks (0xf8)


Measurement results are sent to the MIOS terminal.

Type "reset" in MIOS terminal to clear the measurement results (e.g. after
BPM rate has changed)

===============================================================================

Measurements:

We send 120 BPM - expected delay between ticks is 0.02083 seconds


Reaktor sends 120 BPM via USB:
>>> BPM 120.724  -  tick min/avg/max = 0.017/0.020/0.022

MBSEQ V4 in Master Mode sends 120 BPM via MIDI:
>>> BPM 120.0  -  tick min/avg/max = 0.020/0.020/0.021

Logic9 sends 120 BPM via USB:
>>> BPM 120.0  -  tick min/avg/max = 0.020/0.020/0.022

MBSEQ V4 in Slave Mode receives 120 BPM from Logic via USB and forwards the clock via MIDI:
>>> BPM 120.0  -  tick min/avg/max = 0.019/0.020/0.022

MBSEQ V4 in Slave Mode receives 120 BPM from Logic via OSC (*) and forwards the clock via MIDI:
(*) Mac->WiFi->Router->MBSEQ Ethernet
>>> BPM 120.240  -  tick min/avg/max = 0.014/0.020/0.027

===============================================================================
