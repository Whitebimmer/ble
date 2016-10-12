#ifndef JIFFIES_H
#define JIFFIES_H


extern volatile unsigned long jiffies;


#define time_after(a,b)						 ((long)(b) - (long)(a) < 0)
#define time_before(a,b)			     time_after(b, a)


#define msecs_to_jiffies(msec)       ((msec)/10)
#define jiffies_to_msecs(j)       ((j)*10)





#endif

