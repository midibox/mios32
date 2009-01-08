#ifndef JSW_RAND_H
#define JSW_RAND_H

/* Seed the RNG. Must be called first */
void          jsw_seed ( unsigned long s );

/* Return a 32-bit random number */
unsigned long jsw_rand ( void );

/* Seed with current system time */
unsigned      jsw_time_seed();

#endif
