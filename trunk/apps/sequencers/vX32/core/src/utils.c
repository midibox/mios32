/* $Id$ */
/*
vX32 pre-alpha
not for any use whatsoever
copyright stryd_one
bite me corp 2008

big props to nILS for being my fourth eye and TK for obvious reasons
stay tuned for UI prototyping courtesy of lucem!

*/


/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <FreeRTOS.h>
#include <portmacro.h>

#include <mios32.h>


#include "tasks.h"
#include "app.h"
#include "graph.h"
#include "mclock.h"
#include "modules.h"
#include "patterns.h"
#include "utils.h"
#include "ui.h"



/////////////////////////////////////////////////////////////////////////////
// Global prototypes
/////////////////////////////////////////////////////////////////////////////

/* 
// -128..0..127  --->  0..127         //         0 --> 64
u8 util_s8tou7(s8 input) {
    u8 out = (u8)(input+64);
    out = out%128;
    return out;
}
*/


/* 
// -128..0..127  --->  0..63          //        -32 --> 0        0 --> 32        32 --> 64
u8 util_s8tou6(s8 input) {
    u8 out = (u8)(input+32);
    out = out%64;
    return out;
}
 */


/* 
// -128..0..127  --->  0..63          //        -32 --> 0        0 --> 32        32 --> 64
u8 util_s8tou4(s8 input) {
    u8 out = (u8)(input+8);
    out = out%16;
    return out;
}
*/

