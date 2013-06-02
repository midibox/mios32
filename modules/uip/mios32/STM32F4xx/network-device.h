// $Id$
/*
 * Header file for access functions to network device
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */


#ifndef __NETWORK_DEVICE_H__
#define __NETWORK_DEVICE_H__

extern void network_device_init(void);
extern void network_device_check(void);
extern int network_device_available(void);
extern int network_device_read(void);
extern void network_device_send(void);
extern unsigned char *network_device_mac_addr(void);

#endif /* __NETWORK_DEVICE_H__ */
