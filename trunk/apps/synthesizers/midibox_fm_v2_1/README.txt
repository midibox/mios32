$Id: README.txt 1573 2012-12-03 23:03:38Z tk $

MIDIbox FM V2.0
===============================================================================
Copyright (C) 2014 Sauraen (sauraen@gmail.com)
Licensed for personal non-commercial use only.
All other rights reserved.

Copyright (C) 2012 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

TODO list:

FM mode proper:

- Wavetable isn't made yet
- VARI being assignable to CC
- Modulators 3,4
- Full CC control (there's a RPN or something for pitch bend range)
- Allow modulation of a modulation depth (e.g. modwheel -> pitch LFO depth)
X Put OPL3_OnFrame() on a new thread with priority 1 so loading from SD card doesn't "hang" playback
- Feedback not limited, light refresh broken
- Voice lights stop refreshing on some but not all holdmodes

MIDI mode and voice allocation:

- Enforce LINK -> DUPL hierarchy
/ Have Follow as an option for LINK
- Check all saving options
- Implement dynamic voice allocation mode
- MIDI output options (control surface mode, MIDI thru, etc.)

Percussion:

- Velocity sens per instrument
- Preset different for two drum sets if two OPL3s

Sequencer:

- entire sequencer

Wavetable editor:

- WT ED mode
- FM mode wavetable time as modulation dest

Other:

- AOUT support

Hardware:

- Noise in R channel
- Chorus clipping/distortion
- Crosstalk between L/R, 3/4
- Entire filter
- Sticky playback buttons
