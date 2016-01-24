

#include <mios32.h>
#include "function.h"


void function(u8 mode, u8 i){
    if(mode){
        MIOS32_MIDI_SendDebugMessage("Created object %d!", i);
    }else{
        MIOS32_MIDI_SendDebugMessage("Destroyed object %d!", i);
    }
}
