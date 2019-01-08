/*
	FreeRTOS Emu
*/

/*-----------------------------------------------------------
 * Portable layer API.  Each function must be defined for each port.
 *----------------------------------------------------------*/

#ifndef PORTABLE_H
#define PORTABLE_H

/* Include the macro file relevant to the port being used. */

#ifdef MIOS32_FAMILY_MIOSJUCE
	#include "../portable/GCC/MIOSJUCE/portmacro.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef long portTickType;

void *pvPortMalloc( size_t xSize );
void vPortFree( void *pv );
void vPortInitialiseBlocks( void );


#ifdef __cplusplus
}
#endif

#endif /* PORTABLE_H */

