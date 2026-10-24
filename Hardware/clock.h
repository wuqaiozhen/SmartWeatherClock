#ifndef __CLOCK_H
#define __CLOCK_H

 #include <stdbool.h>
 #include <stdint.h>

  typedef struct {
       int year;
       int mon;
       int day;
       int hour;
       int min;
       int sec;
       int wday;
   } clock_time_t;    
       
void Clock_Set(const clock_time_t *t);
bool Clock_IsSet(void);
bool Clock_Tick(clock_time_t *out);

#endif