/**
 *         uIP configuration for MIOS32 application
 */

#ifndef __UIP_CONF_H__
#define __UIP_CONF_H__

#include <mios32.h>


/**
 * 8 bit datatype
 *
 * This typedef defines the 8-bit type used throughout uIP.
 *
 * \hideinitializer
 */
typedef u8 u8_t;

/**
 * 16 bit datatype
 *
 * This typedef defines the 16-bit type used throughout uIP.
 *
 * \hideinitializer
 */
typedef u16 u16_t;

/**
 * Statistics datatype
 *
 * This typedef defines the dataype used for keeping statistics in
 * uIP.
 *
 * \hideinitializer
 */
typedef u16 uip_stats_t;

/**
 * Maximum number of TCP connections.
 *
 * \hideinitializer
 */
#define UIP_CONF_MAX_CONNECTIONS 10

/**
 * Maximum number of listening TCP ports.
 *
 * \hideinitializer
 */
#define UIP_CONF_MAX_LISTENPORTS 10

/**
 * uIP buffer size.
 *
 * \hideinitializer
 */
#define UIP_CONF_BUFFER_SIZE     420

/**
 * CPU byte order.
 *
 * \hideinitializer
 */
#define UIP_CONF_BYTE_ORDER      LITTLE_ENDIAN

/**
 * Logging on or off
 *
 * \hideinitializer
 */
#define UIP_CONF_LOGGING         1

/**
 * UDP support on or off
 *
 * \hideinitializer
 */
#define UIP_CONF_UDP             0

/**
 * UDP checksums on or off
 *
 * \hideinitializer
 */
#define UIP_CONF_UDP_CHECKSUMS   1

/**
 * uIP statistics on or off
 *
 * \hideinitializer
 */
#define UIP_CONF_STATISTICS      1


/**
 * Ping IP address asignment.
 *
 * uIP uses a "ping" packets for setting its own IP address if this
 * option is set. If so, uIP will start with an empty IP address and
 * the destination IP address of the first incoming "ping" (ICMP echo)
 * packet will be used for setting the hosts IP address.
 *
 * \hideinitializer
 */
#define UIP_CONF_PINGADDRCONF 0

/* Here we include the header file for the application(s) we use in
   our project. */
/*#include "smtp.h"*/
/*#include "hello-world.h"*/
#include "telnet.h"
/*#include "webserver.h"*/
/*#include "dhcpc.h"*/
/*#include "resolv.h"*/
/*#include "webclient.h"*/

#endif /* __UIP_CONF_H__ */
