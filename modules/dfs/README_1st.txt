
DOSFS has been developed by Lewin Edwards, and is provided as freeware.

See README.txt for details.

This package has been downloaded from:
   http://www.larwe.com/zws/products/dosfs/

Version 1.03 from 9/30/06 is used.


dfs_sdcard has been added as access layer between DFS functions (located
in dosemu.c) and MIOS32_SDCARD functions (located in ../mios32/mios32_sdcard.c)

The original usage examples can be found under unused/main.c

MIOS32 based usage examples can be found under
   $MIOS32_PATH/apps/examples/sdcard


Addendum:

TK 2008-12-18: 
DFS_Seek was running endless, applied a patch which has been posted at
http://reza.net/wordpress/?p=110

TK 2008-12-18: 
patched the patch: endcluster wasn't calculated correctly

TK 2008-12-18: 
added 'DFS_CachingEnabledSet(uint8_t enable)' function to enable a simple 
caching mechanism. This feature has to be explicitely enabled, as it isn't 
reentrant and requires to use the same buffer pointer whenever reading a file!
