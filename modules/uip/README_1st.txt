$Id$

uIP has been developed by Adam Dunkels, and is provided as freeware.

See README for details.

This package has been downloaded from:
   http://www.sics.se/~adam/uip/index.php/Main_Page

Version uip-1-0 is used.


Following files have been added:
  - uip.mk: has to be included into Makefile to add uIP sources
  - mios32/clock-arch*: clock function for MIOS32 projects
  - mios32/network-device*: access layer to MIOS32_ENC28J60 routines


MIOS32 based usage examples can be found under
   $MIOS32_PATH/apps/examples/ethernet


Modifications:
  - uip/uip-neighbor.c: commented out "printf"
  - delected doc/html directory (ca. 7 MB), since the files are 
    available on the web under 
      http://www.sics.se/~adam/uip/uip-1.0-refman/
  - Applied patch to uip.c to correct problem with uip_len, as per the
      known bugs page on the uIP website
