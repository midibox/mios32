#ifndef FNRTIME_H_
#define FNRTIME_H_


typedef unsigned long	fnr_clock_t;
typedef unsigned long 	fnr_time_t;



struct tm
{
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
};

unsigned char fnr_days_in_month(struct tm *tim_p);
fnr_time_t fnr_mktime(struct tm *t);
struct tm * fnr_gmtime(fnr_time_t *t,struct tm * gmtm);


#endif /*FNRTIME_H_*/
