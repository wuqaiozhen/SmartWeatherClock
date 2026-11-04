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

 /* 只读: 取当前时间, 不推进时钟 (给日志等模块用)
    * 返回 true = 时间有效(已校准过); false = 还没校准 */
 bool Clock_Get(clock_time_t *out); 
       
void Clock_Set(const clock_time_t *t);
bool Clock_IsSet(void);
bool Clock_Tick(clock_time_t *out);


#endif