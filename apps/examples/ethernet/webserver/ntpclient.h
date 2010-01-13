/*
 * ntpclient.h
 */
#ifndef __NTPCLIENT_H__
#define __NTPCLIENT_H__

#include "timer.h"
#include "pt.h"

#define NTP_STATE_NO_TIME         0
#define NTP_STATE_INITIALIZED     1
#define NTP_STATE_SYNCHONIZED     2

#define NTP_PORT    123

struct ntp_time {
  uint32_t seconds;   // full seconds since 1900
  uint16_t fraction;  // fractions of a second in x/65536
};

// this type describes the correction factor generated
// from the ntp time signal
typedef int16_t ntp_adjust_t;


struct ntp_tm {
  // date
  uint16_t year;   // year
  uint8_t month;   // month within year (1..12)
  uint8_t day;     // day within month (1..31)
  uint8_t weekday; // day within week (0..6)
  // time
  uint8_t hour;    // hour within day (0..23)
  uint8_t minute;  // minute within hour (0..59)
  uint8_t second;  // second within minute (0..59)

  uint8_t dst;     // daylight savings time (+1 hour)
};

extern void ntpclient_init(void);
extern void ntpclient_appcall(void);
extern void ntp_dst(struct ntp_tm *tm);
extern void ntp_tmtime(uint32_t sec, struct ntp_tm *tm);

// to be provided by clock-arch:
extern void clock_get_ntptime(struct ntp_time *time);
extern void clock_set_ntptime(struct ntp_time *time);

//typedef struct ntpclient_state uip_udp_appstate_t;
//#define UIP_UDP_APPCALL ntpclient_appcall

#endif /* __NTPCLIENT_H__ */
