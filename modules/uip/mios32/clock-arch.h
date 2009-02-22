// $Id$
/*
 * Header file for architecture-specific clock functionality
 *
 * ==========================================================================
 *
 *  Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */


#ifndef __CLOCK_ARCH_H__
#define __CLOCK_ARCH_H__

typedef unsigned int clock_time_t;
#define CLOCK_CONF_SECOND 1000

extern clock_time_t clock_time_tick(void);

#endif /* __CLOCK_ARCH_H__ */
