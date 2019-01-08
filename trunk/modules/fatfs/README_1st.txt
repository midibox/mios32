
FatFs has been developed by ChaN and is provided as freeware

See src/00readme.txt for details

This package has been downloaded from:
   http://elm-chan.org/fsw/ff/00index_e.html

Version R0.07e from Nov 3, 2009 is used


Changes:
  - TRUE/FALSE declaration moved from integer.h to ff.c/diskio.c to avoid clash with STM32 library
  - integrated MIOS32 functions into src/diskio.c
  - customized ffconf.h
  - FATFS_USE_LFN and FATFS_MAX_LFN options included from mios32_config.h, 
    so that long filename support can be selected for application (disabled by default)
  - src/option/ccsbcs.c: disabled check for _USE_LFN


TODO:
  o testing the driver w/ MBSEQ
