// $Id$
/*
 * Local MIOS32 configuration file
 *
 * this file allows to disable (or re-configure) default functions of MIOS32
 * available switches are listed in $MIOS32_PATH/modules/mios32/MIOS32_CONFIG.txt
 *
 */

#ifndef _MIOS32_CONFIG_H
#define _MIOS32_CONFIG_H


#if 0
// optional COM output for debugging of seq.c
#define MIOS32_UART1_ASSIGNMENT 2
#define MIOS32_UART1_BAUDRATE 115200
#define MIOS32_COM_DEFAULT_PORT UART1
#endif


#endif /* _MIOS32_CONFIG_H */
