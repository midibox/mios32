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
 * Must be at least 4 bytes smaller than MIOS32_ENC28J60_MAX_FRAME_SIZE
 *
 * \hideinitializer
 */
#define UIP_CONF_BUFFER_SIZE     1500

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
#define UIP_CONF_UDP             1

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


/* Don't use a fixed IP address as we optionally use DHCP */
#define UIP_CONF_FIXEDADDR		0


/**
 * set language of ntp ascii time conversion
 *
 * \hideinitializer
 */
#define NTP_LANG_UK

/**
 * set ntp time zone
 *
 * \hideinitializer
 */
#define NTP_TZ   +0

/**
 * set ntp request cycle in seconds (max 65536/CLOCK_CONF_SECOND)
 *
 * \hideinitializer
 */
#define NTP_REQ_CYCLE 600

/**
 * set number of subsequent ntp requests to asure short turn around time
 *
 * \hideinitializer
 */
#define NTP_REPEAT  4


/* Here we include the header file for the application(s) we use in
   our project. */
/*#include "smtp.h"*/
/*#include "hello-world.h"*/
/*#include "telnetd.h"*/
#include "webserver.h"
#include "dhcpc.h"
#include "ntpclient.h"
//#include "resolv.h"
/*#include "webclient.h"*/

#include "uip_task.h"
#define UIP_UDP_APPCALL UIP_TASK_UDP_AppCall

#endif /* __UIP_CONF_H__ */
