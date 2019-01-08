/*
 * Header file for Portable "Mersenne Twister" rand() functions
 * ==========================================================================
 *
 * By Julienne Walker (happyfrosty@hotmail.com)
 * License: Public Domain
 * 
 * ==========================================================================
 */
 
#ifndef JSW_RAND_H
#define JSW_RAND_H

#ifdef __cplusplus
extern "C" {
#endif

/* Seed the RNG. Must be called first */
extern void          jsw_seed ( unsigned long s );

/* Return a 32-bit random number */
extern unsigned long jsw_rand ( void );

/* Seed with current system time */
extern unsigned      jsw_time_seed();

#ifdef __cplusplus
}
#endif

#endif
