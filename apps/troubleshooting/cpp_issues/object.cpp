

#include <mios32.h>
#include "object.h"
#include "function.h"

Object::Object(u8 index){
    i = index;
    function(1, i);
}

Object::~Object(){
    function(0, i);
}
