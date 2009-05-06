$Id$

MIDIbox SEQ V4 Hardware Configuration Files

Copy one of these files into the root directory of the SD Card.

  - assign LED/Button/Encoder functions
  - enable the Button/LED Matrix
  - etc.

So long no SD Card is connected, or the file cannot be found, the
standard V4 definitions will be used as documented in hwcfg/standard_v4/MBSEQ_HW.V4


The MBSEQ_HW.V4 file can be edited with a common text editor.

It will be loaded (only once!) after startup.

In distance to other configuration files, it won't be loaded again
if the SD Card is reconnected to avoid sequencer hick-ups during
runtime, and to cover the special case where files should be loaded
from a SD card which contains a MBSEQ_HW.V4 file for a different
configuration.
