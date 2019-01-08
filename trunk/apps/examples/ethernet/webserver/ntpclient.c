/*
 * ntpclient.c  -  ntpclient for uip-1.0
 *
 * This file includes a simple implementation of the ntp protocol
 * as well as some helper functions to keep track of of the accurate 
 * time and some time format conversion functions.
 *
 * This code does not follow the uip philisophy of using at most
 * 16 bit arithemtics. For simplicity reasons it also uses 32 bit
 * arithmetic and thus requires a compiler supporting this. Gcc does.
 *
 * (c) 2006 by Till Harbaum <till@harbaum.org>
 * time conversion part based on code published by 
 * peter dannegger - danni(at)specs.de on mikrocontroller.net
 */

#include "uip-conf.h"

#include <stdio.h>
#include <string.h>

#include "uip.h"
#include "timer.h"
#include "pt.h"
#include "uip_task.h"
// enable this to get plenty of debug output via stdout
#define NTP_DEBUG

#define FIRSTYEAR     1900    // start year
#define FIRSTDAY      1	      // 1.1.1900 was a Monday (0 = Sunday)

#if UIP_BYTE_ORDER == UIP_BIG_ENDIAN
static inline uint32_t ntp_read_u32(uint32_t val) { return val; }
#else
static inline uint32_t ntp_read_u32(uint32_t val) { 
  uint8_t tmp, *d = (uint8_t*)&val;
  tmp = d[0]; d[0] = d[3]; d[3] = tmp;
  tmp = d[1]; d[1] = d[2]; d[2] = tmp;
  return val; 
}
#endif

static struct udp_state s;

#define NTP_PORT    123

#define FLAGS_LI_NOWARN    (0<<6)   
#define FLAGS_LI_61SEC     (1<<6)   
#define FLAGS_LI_59SEC     (2<<6)
#define FLAGS_LI_ALARM     (3<<6)

#define FLAGS_VN(n)        ((n)<<3)

// we only support client mode
#define FLAGS_MODE_CLIENT  (3)

// the ntp time message
struct ntp_msg {
  uint8_t flags;
  uint8_t stratum;
  uint8_t poll;
  int8_t precision;

  uint16_t root_delay[2];
  uint16_t root_dispersion[2];
  uint32_t reference_identifier;

  // timestamps:
  uint32_t reference_timestamp[2];
  uint32_t originate_timestamp[2];
  uint32_t receive_timestamp[2];
  uint32_t transmit_timestamp[2];
};

/*---------------------------------------------------------------------------*/
const uint8_t DayOfMonth[] = { 31, 29, 31, 30, 31, 30, 
			       31, 31, 30, 31, 30, 31 };

void 
ntp_dst(struct ntp_tm *tm)
{
  uint8_t hour, day, weekday, month;	// locals for faster access

  hour = tm->hour;
  day = tm->day;
  weekday = tm->weekday;
  month = tm->month;

  if( month < 3 || month > 10 )		// month 1, 2, 11, 12
    return;				// -> Winter

  // after last Sunday 2:00
  if( day - weekday >= 25 && (weekday || hour >= 2) ){
    if( month == 10 )		       // October -> Winter
      return;
  } else {			       // before last Sunday 2:00
    if( month == 3 )		       // March -> Winter
      return;
  }

  // increment all affected parts
  hour++;				// add one hour
  if( hour == 24 ){		        // next day
    hour = 0;
    weekday++;				// next weekday
    if( weekday == 7 )
      weekday = 0;
    if( day == DayOfMonth[month-1] ){	// next month
      day = 0;
      month++;
    }
    day++;
  }

  // write updated values
  tm->dst = 1;
  tm->month = month;
  tm->hour = hour;
  tm->day = day;
  tm->weekday = weekday;
}
/*---------------------------------------------------------------------------*/
void
ntp_tmtime(uint32_t sec, struct ntp_tm *tm) {
  uint16_t day;
  uint8_t year;
  uint16_t dayofyear;
  uint8_t leap400;
  uint8_t month;

  // adjust timezone
  sec += 3600 * NTP_TZ;

  tm->second = sec % 60;
  sec /= 60;
  tm->minute = sec % 60;
  sec /= 60;
  tm->hour = sec % 24;
  day = sec / 24;

  tm->weekday = (day + FIRSTDAY) % 7;		// weekday

  year = FIRSTYEAR % 100;			// 0..99
  leap400 = 4 - ((FIRSTYEAR - 1) / 100 & 3);	// 4, 3, 2, 1

  for(;;){
    dayofyear = 365;
    if( (year & 3) == 0 ){
      dayofyear = 366;					// leap year
      if( year == 0 || year == 100 || year == 200 )	// 100 year exception
        if( --leap400 )					// 400 year exception
          dayofyear = 365;
    }
    if( day < dayofyear )
      break;
    day -= dayofyear;
    year++;					// 00..136 / 99..235
  }
  tm->year = year + FIRSTYEAR / 100 * 100;	// + century

  if( dayofyear & 1 && day > 58 )		// no leap year and after 28.2.
    day++;					// skip 29.2.

  for( month = 1; day >= DayOfMonth[month-1]; month++ )
    day -= DayOfMonth[month-1];

  tm->month = month;				// 1..12
  tm->day = day + 1;				// 1..31
}
/*---------------------------------------------------------------------------*/
static void
send_request(void)
{
  struct ntp_time time;
  struct ntp_msg *m = (struct ntp_msg *)uip_appdata;

  // build ntp request
  m->flags = FLAGS_LI_ALARM | FLAGS_VN(4) | FLAGS_MODE_CLIENT;
  m->stratum = 0;                    // unavailable
  m->poll = 4;                       // 2^4 = 16 sec
  m->precision = -6;                 // 2^-6 = 0.015625 sec = ~1/100 sec

  m->root_delay[0] = HTONS(1);       // 1.0 sec
  m->root_delay[1] = 0;

  m->root_dispersion[0] = HTONS(1);  // 1.0 sec
  m->root_dispersion[1] = 0;

  // we don't have a reference clock
  m->reference_identifier = 0;
  m->reference_timestamp[0] = m->reference_timestamp[1] = 0;

  // we don't know any time
  m->originate_timestamp[0] = m->originate_timestamp[1] = 0;
  m->receive_timestamp[0]   = m->receive_timestamp[1]   = 0;

  // put our own time into the transmit frame (whatever our own time is)
  clock_get_ntptime(&time);
  m->transmit_timestamp[0]  = ntp_read_u32(time.seconds);
  m->transmit_timestamp[1]  = ntp_read_u32(time.fraction);

  // and finally send request
  uip_send(uip_appdata, sizeof(struct ntp_msg));
}
/*---------------------------------------------------------------------------*/
static uint8_t
parse_msg(struct ntp_time *time)
{
  struct ntp_msg *m = (struct ntp_msg *)uip_appdata;

  // check correct time len
  if(uip_datalen() != sizeof(struct ntp_msg))
    return 0;

  // adjust endianess
  time->seconds = ntp_read_u32(m->transmit_timestamp[0]);
  time->fraction = ntp_read_u32(m->transmit_timestamp[1]) >> 16;

  return 1;
}
/*---------------------------------------------------------------------------*/
/* shift time value right one bit (divide by two) */
void
ntp_time_shr(struct ntp_time *a) {
  a->fraction >>= 1;
  if(a->seconds & 1) a->fraction |= 0x8000;
  a->seconds >>= 1;
}
/*---------------------------------------------------------------------------*/
void
ntp_time_diff(struct ntp_time *a, struct ntp_time *b, struct ntp_time *diff) {
  diff->seconds = a->seconds - b->seconds;
  diff->fraction = a->fraction - b->fraction;
  
  // check for underflow in fraction and honour it
  if(diff->fraction > a->fraction) 
    diff->seconds -= 1;
}
/*---------------------------------------------------------------------------*/
void
ntp_time_add(struct ntp_time *a, struct ntp_time *diff) {
  uint32_t tmp_frac = a->fraction;

  a->seconds += diff->seconds;
  a->fraction += diff->fraction;
  
  // check for overflow in fraction and honour it
  if(a->fraction < tmp_frac)
    a->seconds += 1;
}
/*---------------------------------------------------------------------------*/
void
ntp_time_multiply(struct ntp_time *time, ntp_adjust_t adjust) {
  uint32_t a, b, c;

  /* do the entire calculation in chunks of 16*16->32 bit multiplications */
  a = ((time->seconds  >> 16) & 0xffff) * adjust;
  b = ((time->seconds  >>  0) & 0xffff) * adjust;
  c = ((uint32_t)time->fraction) * adjust;

  /* and store it back to the time struct */
  time->seconds = a + (b >> 16);
  time->fraction = b + (c >> 16);

  /* add carry/sign */
  if(adjust >= 0) {
      if(time->fraction < (b&0xffff))
        time->seconds++;
  } else {
    if(time->fraction > (b&0xffff))
        time->seconds--;
    time->seconds |= 0xffff0000;
  }
}
/*---------------------------------------------------------------------------*/
ntp_adjust_t
ntp_time_divide(struct ntp_time *dividend, struct ntp_time *divisor) {
  struct ntp_time remainder;
  struct ntp_time quotient;
  struct ntp_time d, t;
  int i, q, bit, n=1;

  uint8_t num_bits = 48;

  if(!dividend->seconds && !dividend->fraction)
      return 0;

  // shift fraction up 16 bits
  dividend->seconds = (dividend->seconds<<16) + dividend->fraction;
  dividend->fraction = 0;

  remainder.seconds = 0;
  remainder.fraction = 0;

  // check negative
  if(dividend->seconds & 0x80000000) {
    dividend->seconds = -dividend->seconds;
    n = -1;
  }

  // this usually cannot happen, test it anyway
  if((dividend->seconds  == divisor->seconds) &&
     (dividend->fraction == divisor->fraction)) {
    return 0;
  }

  // this is a 48 bit division
  num_bits = 48;

  quotient.seconds = 0;
  quotient.fraction = 0;

  // while(remainder < divisor)
  while((remainder.seconds < divisor->seconds) ||
        ((remainder.seconds == divisor->seconds) &&
         (remainder.fraction < divisor->fraction))) {

    bit = (dividend->seconds & 0x80000000)?1:0;

    // shift remainder up one bit
    remainder.seconds = (remainder.seconds << 1);
    if(remainder.fraction & 0x8000) remainder.seconds |= 1;
    remainder.fraction = (remainder.fraction << 1) | bit;

    d = *dividend;

    // dividend = dividend << 1;
    dividend->seconds = (dividend->seconds << 1);
    if(dividend->fraction & 0x8000) dividend->seconds |= 1;
    dividend->fraction <<= 1;

    num_bits--;
  }

  /* undo last step */
  *dividend = d;
  remainder.fraction >>= 1;
  if(remainder.seconds & 1) remainder.fraction |= 0x8000;
  remainder.seconds >>= 1;

  num_bits++;
  if(num_bits > 48) return 0;

  for (i = 0; i < num_bits; i++) {
    int bit = (dividend->seconds & 0x80000000) >> 31;

    // remainder = (remainder << 1) | bit;
    remainder.seconds <<= 1;
    if(remainder.fraction & 0x8000) remainder.seconds |= 1;
    remainder.fraction = (remainder.fraction << 1) | bit;

    // t = remainder - divisor;
    t.seconds = remainder.seconds - divisor->seconds;
    t.fraction = remainder.fraction - divisor->fraction;
    if(t.fraction > remainder.fraction) t.seconds -= 1;

    // q = !((t & 0x80000000) >> 31);
    q = (t.seconds & 0x80000000)?0:1;

    // dividend = dividend << 1;
    dividend->seconds <<= 1;
    if(dividend->fraction & 0x8000) dividend->seconds |= 1;
    dividend->fraction <<= 1;

    // quotient = (quotient << 1) | q;
    quotient.seconds <<= 1;
    if(quotient.fraction & 0x8000) quotient.seconds |= 1;
    quotient.fraction = (quotient.fraction << 1) | q;

    if (q)
      remainder = t;
  }

  return n * quotient.fraction;
}
/*---------------------------------------------------------------------------*/
void
ntp_get_adjusted(struct ntp_time *time) {
  struct ntp_time diff;

  /* get current hw time */
  clock_get_ntptime(time);
  
  /* don't do anything further if we can't/don't need to adjust */
  if(!s.adjust) return;

  /* calculate time since last ntp update */
  ntp_time_diff(time, &s.last_sync, &diff);

  /* and adjust it according to current adjustment factor */
  ntp_time_multiply(&diff, s.adjust);
  
  /* and add it to current hw time */
  ntp_time_add(time, &diff);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(handle_ntp(void))
{
  static struct ntp_time tx, rx;
  static struct ntp_time stime, ltime;
  static uint8_t cnt;

  static struct ntp_time time, diff, diff2;
  static uint16_t turn_around;

  PT_BEGIN(&s.pt);

  /* no valid time yet */
  cnt = 0;

  /* init correcture state */
  s.last_sync.seconds = 0;
  s.last_sync.fraction = 0;
  s.adjust = 0;
  turn_around = 0xffff;

  /* try_again: */
  s.ticks = CLOCK_SECOND;

  while(1) {
    // save own tx time to a) adopt to transmission delay if time
    // signal and b) determine the "best" time signal
    clock_get_ntptime(&tx);

    send_request();
    timer_set(&s.timer, s.ticks);
    PT_WAIT_UNTIL(&s.pt, uip_newdata() || timer_expired(&s.timer));

    clock_get_ntptime(&rx);    
  
    if(uip_newdata() && parse_msg(&stime)) {
      cnt++;

      /* calculate time between send/receive */
      ntp_time_diff(&rx, &tx, &diff);
#ifdef NTP_DEBUG
      MIOS32_MIDI_SendDebugMessage("NTP: received reply after = %d.%05u sec\n", 
	     diff.seconds, 99999*diff.fraction/65535);
#endif

      /* don't use a signal that traveled longer than one second */
      if(!diff.seconds) {

	if(diff.fraction < turn_around) {
#ifdef NTP_DEBUG
	  MIOS32_MIDI_SendDebugMessage("NTP: shortest turn around time so far\n");
#endif
	  turn_around = diff.fraction;
	  time = stime;

	  /* keep track of our local time at which that time */
	  /* signal has been sent by server */
	  
	  /* divide difference by two */
	  ntp_time_shr(&diff);
#ifdef NTP_DEBUG
	  MIOS32_MIDI_SendDebugMessage("NTP: turn around half diff = %d.%05u (%u,0x%x)\n", 
		 diff.seconds, 99999*diff.fraction/65535, 
		 diff.fraction, diff.fraction);
#endif
	  
	  /* and add this to the received time to adjust to the fact */
	  /* that the time signal transfer has taken some time */
	  ltime = tx;
	  ntp_time_add(&ltime, &diff);

	  /* time now contains the local time at which the time signal */
	  /* has been sent by the server (local requenst tx time plus */
	  /* half the turn around time) */

#ifdef NTP_DEBUG
	  MIOS32_MIDI_SendDebugMessage("NTP: adjusted ltime = %lx/%x\n", ltime.seconds, ltime.fraction);
#endif
	}
      }

      if(cnt == NTP_REPEAT) {

	if(s.last_sync.seconds) {
	  /* already synced, so calculate adjustment */ 
	  /* calc factor = 1 - ((T_NTP - T_SYNC) / (T_HW - T_SYNC)) */
	  /* or ((T_NTP - T_HW)/(T_HW - T_SYNC)) */

#ifdef NTP_DEBUG
	  MIOS32_MIDI_SendDebugMessage("NTP: ntp = %d,%u, ltime = %d,%u\n", 
		 time.seconds, time.fraction, ltime.seconds, ltime.fraction);
#endif
	  
	  /* assume rx contains the physical time */
	  ntp_time_diff(&time, &ltime, &diff);          /* T_NTP - T_HW  */
#ifdef NTP_DEBUG
	  MIOS32_MIDI_SendDebugMessage("NTP: diff = %d.%05u (%u)\n", diff.seconds, 
		 99999*diff.fraction/65535, diff.fraction);
#endif

	  ntp_time_diff(&ltime, &s.last_sync, &diff2);  /* T_HW - T_SYNC */

	  s.adjust = ntp_time_divide(&diff, &diff2);		
#ifdef NTP_DEBUG
	  MIOS32_MIDI_SendDebugMessage("NTP: adjust = %d (%d)\n", s.adjust, s.adjust / 65535.0);
#endif

	  s.state = NTP_STATE_SYNCHONIZED;
	}
	
#ifdef NTP_DEBUG
	// just print time for debugging reasons
	{
	  struct ntp_tm tm;
	  const char *month_names[] = {
	    "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
	    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	  
	  const char *weekday_names[] = {
	    "Sunday", "Monday", "Tuesday", "Wednesday", 
	    "Thursday", "Friday", "Saturday" };

	  // convert ntp seconds since 1900 into useful time structure
	  ntp_tmtime(time.seconds, &tm);

	  // honour dailight savings
	  ntp_dst(&tm);

	  MIOS32_MIDI_SendDebugMessage("NTP: time: %s %d. %s %d %d:%02d:%02d (%s)\n", 
		 weekday_names[tm.weekday],
		 tm.day, month_names[tm.month-1], tm.year,
		 tm.hour, tm.minute, tm.second,
		 tm.dst?"DST":"STD");
	}
	MIOS32_MIDI_SendDebugMessage("NTP: setting hw time\n");
#endif

	/* set time */
	
	/* ltime contains local time at which ntp time has been */
	/* generated and rx is quite exact "now" */
	
	/* calulate time since the time server has created its */
	/* time signal ... */
	ntp_time_diff(&rx, &ltime, &diff); 
	
	/* ... and add the difference to time signal */
	ntp_time_add(&time, &diff); 
	
	/* and use that as the new local hardware time */
	s.last_sync.seconds = time.seconds;
	s.last_sync.fraction = time.fraction;
	clock_set_ntptime(&time);
      
	/* reset for next round */
	cnt = 0;
	turn_around = 0xffff;

	if(s.state == NTP_STATE_NO_TIME) {
	  /* next request sequence after 30 seconds */
	  s.ticks = CLOCK_SECOND * 30; 
	  s.state = NTP_STATE_INITIALIZED;
	} else {
	  /* next request sequence after XX seconds */
	  s.ticks = (CLOCK_SECOND * NTP_REQ_CYCLE); 
	}
	
	timer_set(&s.timer, s.ticks);
	PT_WAIT_UNTIL(&s.pt, timer_expired(&s.timer));
      } else {
	/* next in-sequence request after 1/10 second */
	s.ticks = CLOCK_SECOND/10; 
	timer_set(&s.timer, s.ticks);
	PT_WAIT_UNTIL(&s.pt, timer_expired(&s.timer));
      }
      
    } else {
#ifdef NTP_DEBUG
      MIOS32_MIDI_SendDebugMessage("NTP: timeout\n");
#endif
      
      // double the time every trial ... until it reaches 60 seconds
      if(s.ticks < CLOCK_SECOND * 60) {
	s.ticks *= 2;
      }
    }
  }

/*  timer_stop(&s.timer); */

  /*
   * PT_END restarts the thread so we do this instead. 
   */
  while(1) {
    PT_YIELD(&s.pt);
  }

  PT_END(&s.pt);
}

/*---------------------------------------------------------------------------*/
void
ntpclient_init(void)
{
  uip_ipaddr_t addr;

  s.state = NTP_STATE_NO_TIME;

  //uip_ipaddr(addr, 192,168,1,10);    /* home server elara */
  uip_ipaddr(addr, 147,83,123,136);  /* pool.ntp.org */
  s.conn = uip_udp_new(&addr, HTONS(NTP_PORT));
  if(s.conn != NULL) {
    uip_udp_bind(s.conn, HTONS(NTP_PORT));
  }
  PT_INIT(&s.pt);
}
/*---------------------------------------------------------------------------*/
void
ntpclient_appcall(void)
{
  handle_ntp();
}
/*---------------------------------------------------------------------------*/
